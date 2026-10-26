
struct SSEii_updstr {
  int   sseP;
  int   sw;
  word  *C1;
  word  *sseT;
};


template<int SSEQuant=7, int SSEInter=SSEQuant, int Width=0>
struct SSEii {

  enum { Q=SSEQuant,I=SSEInter };

  word sse_tot[1<<Width][SSEInter];
  word sse_mpc[1<<Width][SSEQuant];
  word ssebits;

  // ---------------------- 

  void Init( int Wi=0, int Ti=0 ) {
    Init_SSEi(Wi,Ti);
  }

  void Init_SSEi( int Wi=0, int Ti=0 ) {
    int i,j; 
    if( Ti<=0 ) Ti=50;
    ssebits = 0;
    for( j=0; j<(1<<Width); j++ ) {
      for( i=0; i<SSEQuant; i++ ) {
        int P = (SCALE*i)/(SSEQuant-1);
        P = ((SCALE-Wi)*P + hSCALE*Wi) / SCALE;
        sse_mpc[j][i] = P + 8192;
      }
      for( i=0; i<SSEInter; i++ ) sse_tot[j][i] = Ti;
    }
  }
  // ---------------------- 

  int SSE_Pred( int iP, SSEii_updstr& X ) {

    int sseFreq = ((SSEQuant-1)*iP)>>SCALElog;
    int sseTotl = ((SSEInter-1)*iP)>>SCALElog;
    word mask = ((1U<<Width)-1U);

    X.sw   = ((SSEQuant-1)*iP) & mSCALE;
    X.sseT = &sse_tot[ssebits&mask][sseTotl];
    X.C1   = &sse_mpc[ssebits&mask][sseFreq+0];
//    X.sseP = (((SCALE-X.sw)*X.C1[0]+X.sw*X.C1[1])>>SCALElog) - 8192;
    X.sseP = X.C1[0] + ((X.sw*(X.C1[1]-X.C1[0]))>>SCALElog) - 8192;

    int f = X.sseP;

    if( f<=0     ) f=1,      X.sseP=0;
    if( f>=SCALE ) f=mSCALE, X.sseP=SCALE;

    return f;
  }

  // ---------------------- 

  void SSE_Update( byte c, const word* Tbl, SSEii_updstr& X ) {

    uint t0 = Min<int>( Tbl[0], X.sseT[0]+1 );
    uint wr = Tbl[t0];
    X.sseT[0] = t0;

    X.sseP = X.sseP*(SCALE-wr)>>SCALElog; // X.sseP *= (1-wr1);

    if( c==0 ) X.sseP+=wr;
    ssebits += ssebits + c;

    int dC = (X.C1[0]-X.C1[1]);
    int sw_dC= ((X.sw*dC+mSCALE)>>SCALElog);
    X.C1[0] = X.sseP + sw_dC       +8192;
    X.C1[1] = X.sseP - (dC-sw_dC)  +8192;
  }

  template< class RangeCoder >
  void Process( RangeCoder& rc, Node2ii &M, const word* MTb, const int Mmw, int &s, const word* Tbl, uint f_update=0 ) {
//if( MTb!=TTable_100 ) return;
    SSEii_updstr X;
    int f = SSE_Pred( M.P, X );
    if( f_update==0 ) rc.BProcess( f, s );
    SSE_Update( s, Tbl, X );
    M.Update( s, MTb, Mmw );
  }

};


template <int len>
struct Number1ii {

  Node2ii P[1<<len];

  void Init_Number1i() {
    Init_Node2ii( P );
  }
 
  template <class INT, class RangeCoder >
  void Process( RangeCoder& rc, INT &n, int lo, int hi, const word* wr, const int mw ) {
    int i,j,k,z=0;
    for( i=len-1; i>=0; i-- ) {
      j = ((1<<len)+z)>>(i+1);
      k = (n>>i)&1;
      if( z+(1<<i)-1<=lo ) k=1; 
 else if( z+(1<<i)  >=hi ) k=0; 
 else P[j].Process( rc, k, wr, mw );
      z |= (k<<i);
    }
    n = z;
  }

};


template <int len, int len2=1<<len>
struct Number2ii {

  enum{ len1=(((1<<len)+len2)>>1)+1 };
  
  Node2ii P[len1];

  void Init_Number2i() {
    Init_Node2ii( P );
  }

  template <class SSE, class RangeCoder >
  void Process( RangeCoder& rc, int &n, int lo, int hi, const word* wr, const int mw, SSE (&S)[len1], const word* Swr, uint f_update=0 ) {
    int i,j,k,z=0;
    for( i=len-1; i>=0; i-- ) {
      j = ((1<<len)+z)>>(i+1);
      k = (n>>i)&1;
      if( z+(1<<i)-1<=lo ) k=1; 
 else if( z+(1<<i)  >=hi ) k=0; 
 else S[j].Process( rc, P[j], wr, mw, k, Swr, f_update );
      z |= (k<<i);
    }
    n = z;
  }

  template <class SSE, class RangeCoder >
  void Process( RangeCoder& rc, int &n, int lo, int hi, const word* wr, const int mw, SSE S,int xofs, const word* Swr, uint f_update=0 ) {
    int i,j,k,z=0;
    for( i=len-1; i>=0; i-- ) {
      j = ((1<<len)+z)>>(i+1);
      k = (n>>i)&1;
      if( z+(1<<i)-1<=lo ) k=1; 
 else if( z+(1<<i)  >=hi ) k=0; 
 else S[j+xofs].Process( rc, P[j], wr, mw, k, Swr, f_update );
      z |= (k<<i);
    }
    n = z;
  }

  template <class SSE, class RangeCoder >
  void Process1( RangeCoder& rc, int &n, int lo, int hi, const word* wr, const int mw, SSE S[len1][2], const word* Swr, int cc ) {
    int i,j,k,z=0;
    for( i=len-1; i>=0; i-- ) {
      j = ((1<<len)+z)>>(i+1);
      k = (n>>i)&1;
      if( z+(1<<i)-1<=lo ) k=1; 
 else if( z+(1<<i)  >=hi ) k=0; 
 else S[j][(cc>>i)&1].Process( rc, P[j], wr, mw, k, Swr );
      z |= (k<<i);
    }
    n = z;
  }

};


template <class T,int N>             static void Init_SSEii( T (&x)[N] ) { for( uint i=0; i<N; i++ ) x[i].Init_SSEi(); };
template <class T,int N,int M>       static void Init_SSEii( T (&x)[N][M] ) { for( uint i=0; i<N; i++ ) Init_SSEii(x[i]); };
template <class T,int N,int M,int L> static void Init_SSEii( T (&x)[N][M][L] ) { for( uint i=0; i<N; i++ ) Init_SSEii(x[i]); };

template <class T>                   static void Init_Number1ii( T& x ) { x.Init_Number1i(); };
template <class T,int N>             static void Init_Number1ii( T (&x)[N] ) { for( uint i=0; i<N; i++ ) Init_Number1ii(x[i]); };
template <class T,int N,int M>       static void Init_Number1ii( T (&x)[N][M] ) { for( uint i=0; i<N; i++ ) Init_Number1ii(x[i]); };
template <class T,int N,int M,int L> static void Init_Number1ii( T (&x)[N][M][L] ) { for( uint i=0; i<N; i++ ) Init_Number1ii(x[i]); };

template <class T>                   static void Init_Number2ii( T& x ) { x.Init_Number2i(); };
template <class T,int N>             static void Init_Number2ii( T (&x)[N] ) { for( uint i=0; i<N; i++ ) Init_Number2ii(x[i]); };
template <class T,int N,int M>       static void Init_Number2ii( T (&x)[N][M] ) { for( uint i=0; i<N; i++ ) Init_Number2ii(x[i]); };
template <class T,int N,int M,int L> static void Init_Number2ii( T (&x)[N][M][L] ) { for( uint i=0; i<N; i++ ) Init_Number2ii(x[i]); };



