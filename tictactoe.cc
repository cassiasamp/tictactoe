#include <iostream>
#include <vector>
#include <algorithm>
#include <ctgmath>
#include <random>
#include <chrono>
#include <set>

using namespace std;

using Code = int;

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
  Geometry() : _accumulation_points(pow(N, D)), _winning_positions(pow(N, D)) {
    construct_unique_terrains();
    construct_winning_lines();
    construct_accumulation_points();
    construct_winning_positions();
    construct_xor_table();
  }

  const vector<vector<int>>& winning_positions() const {
    return _winning_positions;
  }

  const vector<vector<Code>>& winning_lines() const {
    return _winning_lines;
  }

  const vector<int>& accumulation_points() const {
    return _accumulation_points;
  }

  const vector<int>& xor_table() const {
    return _xor_table;
  }

  vector<int> decode(Code code) const {
    vector<int> ans;
    for (int i = 0; i < D; i++) {
      ans.push_back(code % N);
      code /= N;
    }
    return ans;
  }

  void apply_permutation(const vector<Code>& source, vector<Code>& dest,
      const vector<int>& permutation) const {
    transform(begin(source), end(source), begin(dest), [&](Code code) {
      return apply_permutation(permutation, code);
    });
  }

  void print_line(const vector<Code>& line) {
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
      for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
          cout << board[k][j][i];
        }
        cout << "\n";
      }
      cout << "\n";
    }
  }

  void print_points() {
    print(pow(N, D), [&](int k) {
      return decode(k);
    }, [&](int k) {
      int points = _accumulation_points[k];
      return encode_points(points);
    });
  }

 private:
  Code apply_permutation(const vector<int>& permutation, Code code) const {
    auto decoded = decode(code);
    transform(begin(decoded), end(decoded), begin(decoded), [&](int x) {
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
      vector<vector<int>> current_line, int pos) {
    if (pos == D) {
      vector<Code> line(N);
      for (int j = 0; j < N; j++) {
        line[j] = encode(current_line[j]);
      }
      sort(begin(line), end(line));
      _winning_lines.push_back(line);
      return;
    }
    switch (terrain[pos]) {
      case Direction::up: 
        for (int i = 0; i < N; i++) {
          current_line[i][pos] = i;
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::down: 
        for (int i = 0; i < N; i++) {
          current_line[i][pos] = N - i - 1;
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::equal:
        for (int j = 0; j < N; j++) {
          for (int i = 0; i < N; i++) {
            current_line[i][pos] = j;
          }
          generate_lines(terrain, current_line, pos + 1);
        }
        break;
    }
  }

  Code encode(vector<int> dim_index) const {
    Code ans = 0;
    int factor = 1;
    for (int i = 0; i < D; i++) {
      ans += dim_index[i] * factor;
      factor *= N;
    }
    return ans;
  }

  void construct_winning_lines() {
    vector<vector<int>> current_line(N, vector<int>(D));
    for (auto terrain : unique_terrains) {
      generate_lines(terrain, current_line, 0);
    }
    sort(begin(_winning_lines), end(_winning_lines));
  }

  void construct_accumulation_points() {
    for (const auto& line : _winning_lines) {
      for (const auto code : line) {
        _accumulation_points[code]++;
      }
    }
  }

  char encode_points(int points) const {
    return points < 10 ? '0' + points : 
        points < 10 + 26 ? 'A' + points - 10 : '-';
  }

  void construct_winning_positions() {
    int size = static_cast<int>(_winning_lines.size());
    for (int i = 0; i < size; i++) {
      for (auto code : _winning_lines[i]) {
        _winning_positions[code].push_back(i);
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
  vector<vector<Code>> _winning_lines;
  vector<int> _accumulation_points;
  vector<vector<int>> _winning_positions;
  vector<int> _xor_table;
};

enum class Mark {
  X,
  O,
  empty
};

template<int N, int D>
class Symmetry {
 public:
  explicit Symmetry(const Geometry<N, D>& geom) : geom(geom) {
    generate_all_rotations();
    generate_all_eviscerations();
    multiply_groups();
  }

  const vector<vector<Code>>& symmetries() const {
    return _symmetries;
  }

 private:
  void multiply_groups() {
    set<vector<Code>> unique;
    for (const auto& rotation : rotations) {
      for (const auto& evisceration : eviscerations) {
        vector<Code> symmetry(pow(N, D));
        for (int i = 0; i < pow(N, D); i++) {
          symmetry[rotation[evisceration[i]]] = i;
        }
        unique.insert(symmetry);
      }
    }
    copy(begin(unique), end(unique), back_inserter(_symmetries));
  }

  void generate_all_eviscerations() {
    vector<int> index(N);
    iota(begin(index), end(index), 0);
    do {
      if (validate_evisceration(index)) {
        generate_evisceration(index);
      }
    } while (next_permutation(begin(index), end(index)));
  }

  void generate_evisceration(const vector<int>& index) {
    vector<Code> symmetry(pow(N, D));
    iota(begin(symmetry), end(symmetry), 0);
    geom.apply_permutation(symmetry, symmetry, index);
    eviscerations.push_back(symmetry);
  }

  bool validate_evisceration(const vector<int>& index) {
    for (const auto& line : geom.winning_lines()) {
      vector<Code> transformed(N);
      geom.apply_permutation(line, transformed, index);
      sort(begin(transformed), end(transformed));
      if (!search_line(transformed)) {
        return false;
      }
    }
    return true;
  }

  bool search_line(const vector<Code>& transformed) {
    return binary_search(
        begin(geom.winning_lines()), end(geom.winning_lines()), transformed);
  }

  void generate_all_rotations() {
    vector<int> index(D);
    iota(begin(index), end(index), 0);
    do {
      for (int i = 0; i < (1 << D); i++) {
        rotations.push_back(generate_rotation(index, i));
      }
    } while (next_permutation(begin(index), end(index)));
  }

  vector<Code> generate_rotation(const vector<int>& index, int bits) {
    vector<Code> symmetry;
    for (Code i = 0; i < pow(N, D); i++) {
      auto decoded = geom.decode(i);
      Code ans = 0;
      int current_bits = bits;
      for (int j = 0; j < D; j++) {
        int column = decoded[index[j]];
        ans = ans * N + ((current_bits & 1) == 0 ? column : N - column - 1);
        current_bits >>= 1;
      }
      symmetry.push_back(ans);
    }
    return symmetry;
  }

  void print_symmetry(const vector<Code>& symmetry) {
    geom.print(pow(N, D), [&](int k) {
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
  vector<vector<Code>> _symmetries;
  vector<vector<Code>> rotations;
  vector<vector<Code>> eviscerations;
};


template<int N, int D>
class State {
 public:
  State(const Geometry<N, D>& geom, const Symmetry<N, D>& sym) :
      geom(geom),
      sym(sym),
      board(pow(N, D), Mark::empty),
      x_marks_on_line(geom.winning_lines().size()),
      o_marks_on_line(geom.winning_lines().size()),
      xor_table(geom.xor_table()),
      active_line(geom.winning_lines().size(), true),
      current_accumulation(geom.accumulation_points()),
      has_symmetry(true) {
    open_positions.reserve(pow(N, D));
    accepted.reserve(sym.symmetries().size());
  }
  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  vector<Mark> board;
  vector<int> x_marks_on_line, o_marks_on_line;
  vector<int> xor_table;
  vector<bool> active_line;
  vector<int> current_accumulation;
  vector<Code> open_positions;
  vector<vector<Mark>> accepted;
  bool has_symmetry;

  bool play(Code pos, Mark mark) {
    board[pos] = mark;
    for (auto line : geom.winning_positions()[pos]) {      
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
        for (int j = 0; j < N; j++) {
          current_accumulation[geom.winning_lines()[line][j]]--;
        }
      }
    }
    return false;
  }

  const vector<Code>& get_open_positions(Mark mark) {
    open_positions.erase(begin(open_positions), end(open_positions));
    if (has_symmetry) {
      accepted.erase(begin(accepted), end(accepted));
      vector<Mark> current(board);
      vector<Mark> rotated(current.size());
      has_symmetry = false;
      for (int i = 0; i < pow(N, D); i++) {
        if (board[i] == Mark::empty && current_accumulation[i] > 0) {
          current[i] = mark;
          if (find_symmetry(current, rotated, accepted)) {
            has_symmetry = true;
          } else {
            accepted.push_back(current);
            open_positions.push_back(i);
          }
          current[i] = Mark::empty;
        }
      }
    } else {
      for (int i = 0; i < pow(N, D); i++) {
        if (board[i] == Mark::empty && current_accumulation[i] > 0) {
          open_positions.push_back(i);
        }
      }
    }
    return open_positions;
  }

  bool find_symmetry(const vector<Mark>& current, vector<Mark>& rotated,
      const vector<vector<Mark>>& accepted) {
    for (const auto& symmetry : sym.symmetries()) {
      for (int i = 0; i < pow(N, D); i++) {
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
    geom.print(pow(N, D), [&](int k) {
      return geom.decode(k);
    }, [&](int k) {
      return encode_position(board[k]);
    });
  }

  void print_accumulation() {
    geom.print(pow(N, D), [&](int k) {
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

  bool play(Code pos, Mark mark) {
    return state.play(pos, mark);
  }

  Code random_open_position(const vector<Code>& open_positions) {
    uniform_int_distribution<int> random_position(
        0, open_positions.size() - 1);
    return open_positions[random_position(generator)];
  }

  optional<Code> find_forcing_move(
      const vector<int>& current, const vector<int>& opponent,
      const vector<Code>& open_positions) {
    for (int i = 0; i < static_cast<int>(geom.winning_lines().size()); i++) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        Code trial = state.xor_table[i];
        auto found = find(begin(open_positions), end(open_positions), trial);
        if (found != end(open_positions)) {
          return state.xor_table[i];
        }
      }
    }
    return {};
  }

  Code choose_position(Mark mark, const vector<Code>& open_positions) {
    const vector<int>& current = state.get_current(mark);
    const vector<int>& opponent = state.get_opponent(mark);
    optional<Code> attack = find_forcing_move(current, opponent, open_positions);
    if (attack.has_value()) {
      return *attack;
    }
    optional<Code> defend = find_forcing_move(opponent, current, open_positions);
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
      int i = choose_position(current_mark, open_positions);
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
  Geometry<4, 3> geom;
  Symmetry sym(geom);
  //sym.print();
  cout << "num symmetries " << sym.symmetries().size() << "\n";
  vector<int> search_tree(geom.winning_positions().size());
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  int max_plays = 100;
  vector<int> win_counts(3);
  //geom.print_points();
  cout << "winning lines " << geom.winning_lines().size() << "\n";
  for (int i = 0; i < max_plays; i++) {
    GameEngine b(geom, sym, generator, search_tree);
    win_counts[static_cast<int>(b.play())]++;
    //cout << "\n---\n";
    //b.print();
  }
  double total = 0;
  for (int i = 0; i < static_cast<int>(search_tree.size()); i++) {
    double level = static_cast<double>(search_tree[i]) / max_plays;
    cout << "level " << i << " : " << level << "\n";
    if (level > 0) {
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
