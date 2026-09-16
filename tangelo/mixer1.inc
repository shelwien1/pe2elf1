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

  // update() is the first thing a model step does to this mixer, which is what
  // lets everything it and the rest of the step will write be journaled here,
  // once, as whole arrays: the inputs, the five contexts, their probabilities,
  // and the weight row of each context that is about to be trained. add(),
  // set() and p() then write into them unmarked. For the 73-input mixer that is
  // four records and five weight rows instead of some ninety separate cells,
  // and it is the one thing this route can do that track.pl's cannot - see
  // write.inc for why journaling a location once, early, is enough.
  void update() {
    W_block( &tx[0],  sizeof(tx)  );
    W_block( &cxt[0], sizeof(cxt) );
    W_block( &pr[0],  sizeof(pr)  );
    for( int i=0; i<ncxt; i++ ) {
      int err = ((y << 12) - pr[i]) * 7;
      if( err ) W_block( &wx[cxt[i] * N], N*sizeof(short) );
      train(&tx[0], &wx[cxt[i] * N], nx, err);
    }
    W(nx) = W(base) = W(ncxt) = 0;
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
