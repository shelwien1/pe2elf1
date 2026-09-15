// Mixer - combines models using neural networkss
template< int n, int m, int s = 1, int w = 0 >
struct Mixer1 {
  static const int N = (n + 7) & -8;
  static const int M = m;
  static const int S = s;

  int ncxt, base, nx;

  ALIGN(16) short tx[N];
  ALIGN(16) short wx[N*M];
  int cxt[S];
  int pr[S];

  void Init( void ) {
    ncxt=0; base=0; nx=0;
    for( int i=0; i<S; i++ ) pr[i] = 2048;
    for( int j=0; j<N*M; j++ ) wx[j] = w;
  }

  void update() {
    for( int i=0; i<ncxt; i++ ) {
      int err = ((y << 12) - pr[i]) * 7;
      train(&tx[0], &wx[cxt[i] * N], nx, err);
    }
    nx = base = ncxt = 0;
  }

  void add( int x ) { tx[nx++] = x; }

  void set( int cx, int range ) {
    cxt[ncxt++] = base + cx;
    base += range;
  }

  int p() {
    while (nx & 7) tx[nx++] = 0;
    return pr[0] = squash(dot_product(&tx[0], &wx[0], nx) >> 8);
  }

};
