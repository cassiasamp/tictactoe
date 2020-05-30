#ifndef TICTACTOE_HH
#define TICTACTOE_HH

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <cassert>
#include <bitset>
#include <execution>
#include <list>
#include "semantic.hh"
#include "boarddata.hh"
#include "state.hh"

template<typename T, typename F>
optional<T> operator||(optional<T> first, F func) {
  if (first.has_value()) {
    return first;
  }
  return func();
}

template<typename T>
concept Strategy = requires (T x) {
  { x(Mark::X, bitset<125>()) } -> same_as<optional<Position>>;
};

template<int N, int D, Strategy S>
class GameEngine;

template<int N, int D>
class ForcingMove {
 public:
  explicit ForcingMove(const State<N, D>& state) : state(state) {
  }
  const State<N, D>& state;
  constexpr static Line line_size = BoardData<N, D>::line_size;

  template<typename T>
  optional<Position> find_forcing_move(
      const T& current,
      const T& opponent,
      const Bitfield<N, D>& open_positions) {
    for (Line i = 0_line; i < line_size; ++i) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        Position trial = state.get_xor_table(i);
        if (open_positions[trial]) {
          return trial;
        }
      }
    }
    // Can't return directly because of g++ bug.
    optional<Position> empty = {};
    return empty;
  }

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    const auto& current = state.get_current(mark);
    const auto& opponent = state.get_opponent(mark);
    return
        find_forcing_move(current, opponent, open_positions) ||
        [&](){ return find_forcing_move(opponent, current, open_positions); };
  }
};

template<int N, int D>
class ForcingStrategy {
 public:
  explicit ForcingStrategy(
    const State<N, D>& state, const BoardData<N, D>& data) :
      state(state), data(data) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename T>
  optional<Position> find_forcing_move(
      const T& current,
      const T& opponent,
      const Bitfield<N, D>& open_positions) {
    for (Position pos : open_positions.all()) {
      for (const auto& [line_a, line_b] : data.crossings()[pos]) {
        if (current[line_a] == N - 2 && opponent[line_a] == 0 &&
            current[line_b] == N - 2 && opponent[line_b] == 0) {
          return pos;
        }
      }
    }
    // Can't return directly because of g++ bug.
    optional<Position> empty = {};
    return empty;
  }

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    const auto& current = state.get_current(mark);
    const auto& opponent = state.get_opponent(mark);
    return
        find_forcing_move(current, opponent, open_positions) ||
        [&](){ return find_forcing_move(opponent, current, open_positions); };
  }
};

template<int N, int D>
class BiasedRandom {
 public:
  BiasedRandom(const State<N, D>& state, default_random_engine& generator)
      : state(state), generator(generator) {
  }
  const State<N, D>& state;
  default_random_engine& generator;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    int total = 0;
    for (Position pos : open_positions.all()) {
      total += state.get_current_accumulation(pos);
    }
    uniform_int_distribution<int> random_position(0, total - 1);
    int chosen = random_position(generator);
    int previous = 0;
    for (Position pos : open_positions.all()) {
      int current = previous + state.get_current_accumulation(pos);
      if (chosen < current) {
        return pos;
      }
      previous = current;
    }
    assert(false);
  }
};

template<Strategy A, Strategy B>
class Combiner {
 public:
  Combiner(A a, B b) : a(a), b(b) {
  }
  A a;
  B b;
  template<typename T>
  optional<Position> operator()(Mark mark, const T& open_positions) {
    return a(mark, open_positions) ||
        [&](){ return b(mark, open_positions); };
  }
};

template<Strategy A, Strategy B>
constexpr auto operator>>(A a, B b) {
  return Combiner<A, B>(a, b);
}

template<int N, int D>
class HeatMap {
 public:
  HeatMap(
    const State<N, D>& state,
    const BoardData<N, D>& data,
    default_random_engine& generator,
    int trials,
    bool print_board = false)
      : state(state), data(data), generator(generator),
        trials(trials), print_board(print_board) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  default_random_engine& generator;
  int trials;
  bool print_board;
  constexpr static Line line_size = BoardData<N, D>::line_size;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    vector<Position> open = open_positions.get_vector();
    vector<int> score = get_scores(mark, open);
    if (print_board) {
      vector<int> norm = normalize_score(score);
      print(open, norm);
    }
    auto winner = max_element(begin(score), end(score));
    return open[distance(begin(score), winner)];
  }

  vector<int> get_scores(Mark mark, const vector<Position>& open) {
    Mark flipped = flip(mark);
    vector<int> score(open.size());
    transform(execution::par_unseq, begin(open), end(open), begin(score),
        [&](Position pos) {
      return monte_carlo(mark, flipped, pos);
    });
    return score;
  }

  vector<int> normalize_score(const vector<int>& score) {
    auto [vmin, vmax] = minmax_element(begin(score), end(score));
    double range = *vmax - *vmin;
    vector<int> norm(score.size());
    transform(begin(score), end(score), begin(norm), [&, vmin = vmin](int s) {
      if (range == 0.0) {
        return 9;
      } else {
        return static_cast<int>((s - *vmin) / range * 9.99);
      }
    });
    return norm;
  }

  int monte_carlo(Mark mark, Mark flipped, Position pos) {
    vector<int> win_counts(3);
    for (int i = 0; i < trials; ++i) {
      State<N, D> cloned(state);
      cloned.play(pos, mark);
      auto s =
          ForcingMove<N, D>(cloned) >>
          ForcingStrategy<N, D>(cloned, data) >>
          BiasedRandom<N, D>(cloned, generator);
      GameEngine engine(generator, cloned, s);
      Mark winner = engine.play(flipped);
      win_counts[static_cast<int>(winner)]++;
    }
    return win_counts[static_cast<int>(mark)] -
           win_counts[static_cast<int>(flipped)];
  }

  void print(const vector<Position>& open, const vector<int>& norm) {
    data.print(board_size, [&](Position pos) {
      return data.decode(pos);
    }, [&](Position pos) {
      if (state.get_board(pos) != Mark::empty) {
        string color = "\x1b[32m"s;
        return color + (state.get_board(pos) == Mark::X ? 'X' : 'O');
      } else {
        string color = "\x1b[37m"s;
        auto it = find(begin(open), end(open), pos);
        if (it != end(open)) {
          return color + static_cast<char>(norm[distance(begin(open), it)] + '0');
        }
      }
      return "\x1b[30m."s;
    });
    cout << "\x1b[0m\n"s;
  }
};

template<int N, int D>
class MiniMax {
 public:
  explicit MiniMax(
    const State<N, D>& state, 
    const BoardData<N, D>& data,
    default_random_engine& generator)
    :  state(state), data(data), generator(generator),
       nodes_visited(0) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  default_random_engine& generator;
  int nodes_visited;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename B>
  optional<Mark> operator()(Mark mark, const B& open_positions) {
    return {};
  }

  optional<Mark> play(State<N, D>& current_state, Mark mark) {
    auto ans = play(current_state, mark, {}, flip(mark));
    cout << "Total nodes visited: " << nodes_visited << "\n";
    return ans;
  }

  optional<Mark> play(
      State<N, D>& current_state, Mark mark, vector<int> rank, Mark parent) {
    auto open_positions = current_state.get_open_positions(mark);
    report_progress(open_positions, rank);
    if (open_positions.none()) {
      return Mark::empty;
    }
    auto s =
        ForcingMove<N, D>(state) >>
        ForcingStrategy<N, D>(state, data);
    auto forcing = s(mark, open_positions);
    if (forcing.has_value()) {
      State<N, D> cloned(current_state);
      bool result = cloned.play(*forcing, mark);
      if (result) {
        return mark;
      }
      Mark flipped = flip(mark);
      rank.push_back(-1);
      auto x = play(cloned, flipped, rank, parent);
      rank.pop_back();
      return x;
    }
    vector<Position> open = open_positions.get_vector();
    vector<pair<int, Position>> paired(open.size());
    if (open_positions.count() < 7) {
      for (int i = 0; i < static_cast<int>(open.size()); ++i) {
        paired[i] = make_pair(0, open[i]);
      }
    } else {
      int trials = 5 * open_positions.count();
      HeatMap<N, D> heatmap(state, data, generator, trials);
      vector<int> scores = heatmap.get_scores(mark, open);
      for (int i = 0; i < static_cast<int>(open.size()); ++i) {
        paired[i] = make_pair(scores[i], open[i]);
      }
      sort(rbegin(paired), rend(paired));
    }
    Mark current_best = flip(mark);
    int x = 0;
    for (const auto [score, pos] : paired) {
      State<N, D> cloned(current_state);
      bool result = cloned.play(pos, mark);
      if (result) {
        return mark;
      } else {
        Mark flipped = flip(mark);
        rank.push_back(x);
        optional<Mark> new_result = play(cloned, flipped, rank, current_best);
        rank.pop_back();
        if (new_result.has_value()) {
          if (*new_result == mark) {
            return mark;
          }
          if (*new_result == Mark::empty) {
            if (parent == Mark::empty) {
              return Mark::empty;
            } else {
              current_best = *new_result;
            }
          }
        }        
      }
      x++;
    }
    return current_best;
  }

  template<typename B, typename T>
  void report_progress(const B& open_positions, const T& rank) {
    if ((nodes_visited % 1000) == 0) {
      cout << "id " << nodes_visited << " " << open_positions.count() << endl;
      cout << "rank ";
      for (int i : rank) {
        cout << i <<  " ";
      }
      cout << "\n";
    }
    nodes_visited++;
  }
};

template<int N, int D, Strategy S>
class GameEngine {
 public:
  GameEngine(
    default_random_engine& generator,
    State<N, D>& state,
    S strategy) :
      generator(generator),
      state(state),
      strategy(strategy) {
  }

  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename T, typename U>
  Mark play(Mark start, T pre_observer, U post_observer) {
    Mark current_mark = start;
    while (true) {
      const auto& open_positions = state.get_open_positions(current_mark);
      if (open_positions.none()) {
        return Mark::empty;
      }
      pre_observer(open_positions);
      optional<Position> pos = strategy(current_mark, open_positions);
      if (pos.has_value()) {
        auto result = state.play(*pos, current_mark);
        post_observer(state, pos);
        if (result) {
          return current_mark;
        }
      } else {
        post_observer(state, pos);
      }
      current_mark = flip(current_mark);
    }
  }

  Mark play(Mark start) {
    return play(start, [](auto x){}, [](const auto& x, auto y){});
  }

  default_random_engine& generator;
  State<N, D>& state;
  S strategy;
};

#endif
