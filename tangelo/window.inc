// Buffer - array of n last bytes
template< int N >
struct Buf {

  int pos;
  byte b[N];

  void Init() {
    pos=0;
  }

  byte& operator[](int i) {
    return b[i & (N-1)];
  }

  int operator()(int i) const {
    return b[(pos - i) & (N-1)];
  }

  int size() const {
    return N;
  }

};
