#ifndef SEMANTIC_HH
#define SEMANTIC_HH

template<typename T>
class Index {
 public:
  constexpr explicit Index(int index) : index(index) {
  }
  constexpr operator int() {
    return index;
  }
  constexpr operator int() const {
    return index;
  }
  constexpr Index& operator/=(int i) {
    index /= i;
    return *this;
  }
  constexpr Index& operator+=(int i) {
    index += i;
    return *this;
  }
  constexpr Index& operator++() {
    index++;
    return *this;
  }
  constexpr Index operator++(int) {
    Index tmp(*this);
    operator++();
    return tmp;
  }
 private:
  int index;
};

#define SEMANTIC_INDEX(name, prefix) \
class name : public Index<name> { \
 public: \
  constexpr explicit name(int index) : Index(index) { \
  } \
  constexpr name() : Index(0) { \
  } \
}; \
constexpr name operator"" _##prefix(unsigned long long int index) { \
  return name{static_cast<int>(index)}; \
}

#endif