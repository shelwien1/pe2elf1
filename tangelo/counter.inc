//  StateMap - maps a context to a probability               
template< int N >
struct StateMap {
//  const int N;
  int cxt;
//  Array<uint> t;  
  uint t[N];

//  StateMap(int n = 256):N(n), cxt(0), t(n) {

  void Init( void ) {
    cxt = 0;
    for (int i = 0; i < N; ++i) {
      t[i] = 1 << 31;
    }
  }

  int p(int cx) {
    uint *p = &t[cxt], p0 = p[0];
    int n = p0 & 1023, pr = p0 >> 10;
    if (n < 1023) ++p0;
    else p0 = (p0 & 0xfffffc00) | 1023;
    p0 += (((y << 22) - pr) >> 3) * pt(n) & 0xfffffc00;
    p[0] = p0;
    return t[cxt = cx] >> 20;
  }

};
