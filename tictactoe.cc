#include <iostream>
#include <vector>
#include <algorithm>
#include <ctgmath>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <cassert>
#include "semantic.hh"

using namespace std;

SEMANTIC_INDEX(Position, pos)
SEMANTIC_INDEX(Side, side)
SEMANTIC_INDEX(Line, line)
SEMANTIC_INDEX(Dim, dim)
SEMANTIC_INDEX(SymLine, sym)

enum class Direction {
  equal,
  up,
  down
};

constexpr initializer_list<Direction> all_directions {
  Direction::equal,
  Direction::up,
  Direction::down
};

template<int N, int D>
class Geometry {
 public:
  Geometry()
      : _accumulation_points(board_size), _lines_through_position(board_size) {
    construct_unique_terrains();
    construct_winning_lines();
    construct_accumulation_points();
    construct_lines_through_position();
    construct_xor_table();
  }

  constexpr static Position board_size =
      static_cast<Position>(pow(N, D));

  constexpr static Line line_size =
      static_cast<Line>((pow(N + 2, D) - pow(N, D)) / 2);

  const vector<vector<Line>>& lines_through_position() const {
    return _lines_through_position;
  }

  const vector<vector<Position>>& winning_lines() const {
    return _winning_lines;
  }

  const vector<int>& accumulation_points() const {
    return _accumulation_points;
  }

  const vector<int>& xor_table() const {
    return _xor_table;
  }

  vector<Side> decode(Position pos) const {
    vector<Side> ans;
    for (Dim i = 0_dim; i < D; ++i) {
      ans.push_back(Side{pos % N});
      pos /= N;
    }
    return ans;
  }

  void apply_permutation(const vector<Position>& source, vector<Position>& dest,
      const vector<Side>& permutation) const {
    transform(begin(source), end(source), begin(dest), [&](Position pos) {
      return apply_permutation(permutation, pos);
    });
  }

  void print_line(const vector<Position>& line) {
    print(N, [&](int k) {
      return decode(line[k]);
    }, [&](int k) {
      return 'X';
    });
  }

  template<typename X, typename T>
  void print(int limit, X decoder, T func) const {
    static_assert(D == 3);
    vector<vector<vector<char>>> board(N, vector<vector<char>>(
        N, vector<char>(N, '.')));
    for (int k = 0; k < limit; k++) {
      vector<int> decoded = decoder(k);
      board[decoded[0]][decoded[1]][decoded[2]] = func(k);
    }
    for (int k = 0; k < N; k++) {
      for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
          cout << board[k][j][i];
        }
        cout << "\n";
      }
      cout << "\n";
    }
  }

  void print_points() {
    print(board_size, [&](Position k) {
      return decode(k);
    }, [&](int k) {
      int points = _accumulation_points[k];
      return encode_points(points);
    });
  }

 private:
  Position apply_permutation(const vector<Side>& permutation, Position pos) const {
    auto decoded = decode(pos);
    transform(begin(decoded), end(decoded), begin(decoded), [&](Side x) {
      return permutation[x];
    });
    return encode(decoded);
  }

  void fill_terrain(vector<Direction>& terrain, int pos) {
    if (pos == D) {
      auto it = find_if(begin(terrain), end(terrain),
          [](const auto& elem) { return elem != Direction::equal; });
      if (it != end(terrain) && *it == Direction::up) {
        unique_terrains.push_back(terrain);
      }
      return;
    }
    for (auto dir : all_directions) {
      terrain[pos] = dir;
      fill_terrain(terrain, pos + 1);
    }
  }

  void construct_unique_terrains() {
    vector<Direction> terrain(D);
    fill_terrain(terrain, 0);
  }

  void generate_lines(const vector<Direction>& terrain,
      vector<vector<Side>> current_line, int pos) {
    if (pos == D) {
      vector<Position> line(N);
      for (Side j = 0_side; j < N; ++j) {
        line[j] = encode(current_line[j]);
      }
      sort(begin(line), end(line));
      _winning_lines.push_back(line);
      return;
    }
    switch (terrain[pos]) {
      case Direction::up:
        for (Side i = 0_side; i < N; ++i) {
          current_line[i][pos] = Side{i};
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::down:
        for (Side i = 0_side; i < N; ++i) {
          current_line[i][pos] = Side{N - i - 1};
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::equal:
        for (Side j = 0_side; j < N; ++j) {
          for (Side i = 0_side; i < N; ++i) {
            current_line[i][pos] = Side{j};
          }
          generate_lines(terrain, current_line, pos + 1);
        }
        break;
    }
  }

  Position encode(const vector<Side>& dim_index) const {
    Position ans = 0_pos;
    int factor = 1;
    for (Dim i = 0_dim; i < D; ++i) {
      ans += dim_index[i] * factor;
      factor *= N;
    }
    return ans;
  }

  void construct_winning_lines() {
    vector<vector<Side>> current_line(N, vector<Side>(D, Side{0}));
    for (auto terrain : unique_terrains) {
      generate_lines(terrain, current_line, 0);
    }
    sort(begin(_winning_lines), end(_winning_lines));
    assert(static_cast<int>(_winning_lines.size()) == line_size);
  }

  void construct_accumulation_points() {
    for (const auto& line : _winning_lines) {
      for (const auto pos : line) {
        _accumulation_points[pos]++;
      }
    }
  }

  char encode_points(int points) const {
    return points < 10 ? '0' + points :
        points < 10 + 26 ? 'A' + points - 10 : '-';
  }

  void construct_lines_through_position() {
    for (Line i = 0_line; i < line_size; ++i) {
      for (auto pos : _winning_lines[i]) {
        _lines_through_position[pos].push_back(i);
      }
    }
  }

  void construct_xor_table() {
    for (auto& line : _winning_lines) {
      int ans = accumulate(begin(line), end(line), 0, [](auto x, auto y) {
        return x ^ y;
      });
      _xor_table.push_back(ans);
    }
  }

  vector<vector<Direction>> unique_terrains;
  vector<vector<Position>> _winning_lines;
  vector<int> _accumulation_points;
  vector<vector<Line>> _lines_through_position;
  vector<int> _xor_table;
};

enum class Mark {
  X,
  O,
  empty
};

class SymmeTrie {
 public:
  explicit SymmeTrie(const vector<vector<Position>>& symmetries) 
      : board_size(symmetries[0].size()) {
    construct_trie(symmetries);
  }

  const vector<int>& similar(SymLine line) const {
    return nodes[line].similar;
  }

  SymLine next(SymLine line, Position pos) const {
    return nodes[line].next[pos];
  }

  void print() {
    for (const auto& node : nodes) {
      cout << " --- \n";
      print_node(node);
      for (Position j = 0_pos; j < board_size; ++j) {
        cout << j << " -> ";
        print_node(nodes[node.next[j]]);
      }
    }
  }

 private:
  struct Node {
    vector<int> similar;
    vector<SymLine> next;
  };
  vector<Node> nodes;
  int board_size;

  void print_node(const Node& node) {
    for (int i = 0; i < static_cast<int>(node.similar.size()); ++i) {
      cout << node.similar[i] << " ";
    }
    cout << "\n";
  }

  void construct_trie(const vector<vector<Position>>& symmetries) {
    vector<int> root(symmetries.size());
    iota(begin(root), end(root), 0);
    queue<SymLine> pool;
    nodes.push_back(Node{root, vector<SymLine>(board_size)});
    pool.push(0_sym);
    while (!pool.empty()) {
      SymLine current_node = pool.front();
      vector<int> current = nodes[current_node].similar;
      pool.pop();
      for (Position i = 0_pos; i < board_size; ++i) {
        vector<int> next_similar;
        for (int j = 0; j < static_cast<int>(current.size()); ++j) {
          if (i == symmetries[current[j]][i]) {
            next_similar.push_back(j);
          }
        }
        auto it = find_if(begin(nodes), end(nodes), [&](const auto& x) {
          return x.similar == next_similar;
        });
        if (it == end(nodes)) {
          nodes.push_back(Node{next_similar, vector<SymLine>(board_size)});
          SymLine last_node = static_cast<SymLine>(nodes.size() - 1);
          pool.push(last_node);
          nodes[current_node].next[i] = last_node;
        } else {
          nodes[current_node].next[i] =
              SymLine{static_cast<SymLine>(distance(begin(nodes), it))};
        }
      }
    }
  }
};

template<int N, int D>
class Symmetry {
 public:
  explicit Symmetry(const Geometry<N, D>& geom) : geom(geom) {
    generate_all_rotations();
    generate_all_eviscerations();
    multiply_groups();
  }

  const vector<vector<Position>>& symmetries() const {
    return _symmetries;
  }

 private:
  void multiply_groups() {
    set<vector<Position>> unique;
    for (const auto& rotation : rotations) {
      for (const auto& evisceration : eviscerations) {
        vector<Position> symmetry(geom.board_size);
        for (Position i = 0_pos; i < geom.board_size; ++i) {
          symmetry[rotation[evisceration[i]]] = i;
        }
        unique.insert(symmetry);
      }
    }
    copy(begin(unique), end(unique), back_inserter(_symmetries));
    sort(begin(_symmetries), end(_symmetries));
  }

  void generate_all_eviscerations() {
    vector<Side> index(N);
    iota(begin(index), end(index), 0_side);
    do {
      if (validate_evisceration(index)) {
        generate_evisceration(index);
      }
    } while (next_permutation(begin(index), end(index)));
  }

  void generate_evisceration(const vector<Side>& index) {
    vector<Position> symmetry(geom.board_size);
    iota(begin(symmetry), end(symmetry), 0_pos);
    geom.apply_permutation(symmetry, symmetry, index);
    eviscerations.push_back(symmetry);
  }

  bool validate_evisceration(const vector<Side>& index) {
    for (const auto& line : geom.winning_lines()) {
      vector<Position> transformed(N);
      geom.apply_permutation(line, transformed, index);
      sort(begin(transformed), end(transformed));
      if (!search_line(transformed)) {
        return false;
      }
    }
    return true;
  }

  bool search_line(const vector<Position>& transformed) {
    return binary_search(
        begin(geom.winning_lines()), end(geom.winning_lines()), transformed);
  }

  void generate_all_rotations() {
    vector<int> index(D);
    iota(begin(index), end(index), 0);
    do {
      for (int i = 0; i < (1 << D); ++i) {
        rotations.push_back(generate_rotation(index, i));
      }
    } while (next_permutation(begin(index), end(index)));
  }

  vector<Position> generate_rotation(const vector<int>& index, int bits) {
    vector<Position> symmetry;
    for (Position i = 0_pos; i < geom.board_size; ++i) {
      auto decoded = geom.decode(i);
      Position ans = 0_pos;
      int current_bits = bits;
      for (int j = 0; j < D; ++j) {
        int column = decoded[index[j]];
        ans = Position{ans * N + ((current_bits & 1) == 0 ? column : N - column - 1)};
        current_bits >>= 1;
      }
      symmetry.push_back(ans);
    }
    return symmetry;
  }

  void print_symmetry(const vector<Position>& symmetry) {
    geom.print(geom.board_size, [&](int k) {
      return geom.decode(k);
    }, [&](int k) {
      return geom.encode_points(symmetry[k]);
    });
  }

  void print() {
    for (auto& symmetry : symmetries) {
      print_symmetry(symmetry);
      cout << "\n---\n\n";
    }
  }

  const Geometry<N, D>& geom;
  vector<vector<Position>> _symmetries;
  vector<vector<Position>> rotations;
  vector<vector<Position>> eviscerations;
};

template<int N, int D>
class State {
 public:
  State(const Geometry<N, D>& geom, const Symmetry<N, D>& sym) :
      geom(geom),
      sym(sym),
      trie(sym.symmetries()),
      board(geom.board_size, Mark::empty),
      x_marks_on_line(geom.line_size),
      o_marks_on_line(geom.line_size),
      xor_table(geom.xor_table()),
      active_line(geom.line_size, true),
      current_accumulation(geom.accumulation_points()),
      has_symmetry(true),
      trie_node(0_sym) {
    open_positions.reserve(geom.board_size);
    accepted.reserve(sym.symmetries().size());
  }
  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  const SymmeTrie trie;
  vector<Mark> board;
  vector<int> x_marks_on_line, o_marks_on_line;
  vector<int> xor_table;
  vector<bool> active_line;
  vector<int> current_accumulation;
  vector<Position> open_positions;
  vector<vector<Mark>> accepted;
  bool has_symmetry;
  SymLine trie_node;

  bool play(Position pos, Mark mark) {
    board[pos] = mark;
    for (auto line : geom.lines_through_position()[pos]) {
      vector<int>& current = get_current(mark);
      current[line]++;
      xor_table[line] ^= pos;
      if (current[line] == N) {
        return true;
      }
      if (x_marks_on_line[line] > 0 &&
          o_marks_on_line[line] > 0 &&
          active_line[line]) {
        active_line[line] = false;
        for (Side j = 0_side; j < N; ++j) {
          current_accumulation[geom.winning_lines()[line][j]]--;
        }
      }
    }
    return false;
  }

  const vector<Position>& get_open_positions(Mark mark) {
    open_positions.erase(begin(open_positions), end(open_positions));
    vector<bool> checked(geom.board_size, false);
    for (Position i = 0_pos; i < geom.board_size; ++i) {
      if (board[i] == Mark::empty &&
          current_accumulation[i] > 0 && !checked[i]) {
        checked[i] = true;
        open_positions.push_back(i);
      }
    }
    return open_positions;
  }

  bool find_symmetry(const vector<Mark>& current, vector<Mark>& rotated,
      const vector<vector<Mark>>& accepted) {
    for (const auto& symmetry : sym.symmetries()) {
      for (Position i = 0_pos; i < geom.board_size; ++i) {
        rotated[i] = current[symmetry[i]];
      }
      if (find(begin(accepted), end(accepted), rotated) != end(accepted)) {
        return true;
      }
    }
    return false;
  }

  vector<int>& get_current(Mark mark) {
    return mark == Mark::X ? x_marks_on_line : o_marks_on_line;
  }

  vector<int>& get_opponent(Mark mark) {
    return mark == Mark::X ? o_marks_on_line : x_marks_on_line;
  }

  void print() {
    geom.print(geom.board_size, [&](int k) {
      return geom.decode(k);
    }, [&](int k) {
      return encode_position(board[k]);
    });
  }

  void print_accumulation() {
    geom.print(geom.board_size, [&](int k) {
      return geom.decode(k);
    }, [&](int k) {
      return geom.encode_points(current_accumulation[k]);
    });
  }

  char encode_position(Mark pos) {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O'
         : '.';
  }
};

template<int N, int D>
class GameEngine {
 public:
  GameEngine(const Geometry<N, D>& geom,
    const Symmetry<N, D>& sym,
    default_random_engine& generator,
    vector<int>& search_tree) :
      geom(geom),
      sym(sym),
      generator(generator),
      state(geom, sym),
      search_tree(search_tree) {
  }

  bool play(Position pos, Mark mark) {
    return state.play(pos, mark);
  }

  Position random_open_position(const vector<Position>& open_positions) {
    uniform_int_distribution<int> random_position(
        0, open_positions.size() - 1);
    return open_positions[random_position(generator)];
  }

  optional<Position> find_forcing_move(
      const vector<int>& current, const vector<int>& opponent,
      const vector<Position>& open_positions) {
    for (Line i = 0_line; i < geom.line_size; ++i) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        Position trial = Position{state.xor_table[i]};
        auto found = find(begin(open_positions), end(open_positions), trial);
        if (found != end(open_positions)) {
          return trial;
        }
      }
    }
    return {};
  }

  Position choose_position(Mark mark, const vector<Position>& open_positions) {
    const vector<int>& current = state.get_current(mark);
    const vector<int>& opponent = state.get_opponent(mark);
    optional<Position> attack = find_forcing_move(current, opponent, open_positions);
    if (attack.has_value()) {
      return *attack;
    }
    optional<Position> defend = find_forcing_move(opponent, current, open_positions);
    if (defend.has_value()) {
      return *defend;
    }
    return random_open_position(open_positions);
  }

  Mark play() {
    Mark current_mark = Mark::X;
    int level = 0;
    while (true) {
      auto open_positions = state.get_open_positions(current_mark);
      if (open_positions.empty()) {
        return Mark::empty;
      }
      search_tree[level++] += open_positions.size();
      Position i = choose_position(current_mark, open_positions);
      auto result = play(i, current_mark);
      /*state.print();
      state.print_accumulation();
      cout << "\n------\n\n";*/
      if (result) {
        return current_mark;
      }
      current_mark = flip(current_mark);
    }
  }

  Mark flip(Mark mark) {
    return mark == Mark::X ? Mark::O : Mark::X;
  }

  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  default_random_engine& generator;
  State<N, D> state;
  vector<int>& search_tree;
};

int main() {
  Geometry<5, 3> geom;
  Symmetry sym(geom);
  //SymmeTrie trie(sym.symmetries());
  //trie.print();
  //sym.print();
  cout << "num symmetries " << sym.symmetries().size() << "\n";
  vector<int> search_tree(geom.lines_through_position().size());
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  int max_plays = 100;
  vector<int> win_counts(3);
  //geom.print_points();
  cout << "winning lines " << geom.line_size << "\n";
  for (int i = 0; i < max_plays; ++i) {
    GameEngine b(geom, sym, generator, search_tree);
    win_counts[static_cast<int>(b.play())]++;
    //cout << "\n---\n";
    //b.print();
  }
  double total = 0.0;
  for (int i = 0; i < static_cast<int>(search_tree.size()); ++i) {
    double level = static_cast<double>(search_tree[i]) / max_plays;
    cout << "level " << i << " : " << level << "\n";
    if (level > 0.0) {
      double log_level = log10(level < 1.0 ? 1.0 : level);
      total += log_level;
      //cout << log_level << "\n";
    }
  }
  cout << "\ntotal : 10 ^ " << total << "\n";
  cout << "X wins : " << win_counts[static_cast<int>(Mark::X)] << "\n";
  cout << "O wins : " << win_counts[static_cast<int>(Mark::O)] << "\n";
  cout << "draws  : " << win_counts[static_cast<int>(Mark::empty)] << "\n";
  return 0;
}
