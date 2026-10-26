
struct Node2ii {

  word P;  
  word T;

  void Init_Node2i() {
    P = hSCALE; T = 1;
  }

  enum{ NORMAL, NOUPDATE, ONLYUPDATE };

  template< class RangeCoder >
  void Process( RangeCoder& rc, int &c, const word* MTb, const int Mw, int flag=NORMAL ) {

    if( flag!=ONLYUPDATE ) {
      rc.BProcess( P, c );
    }

    if( flag!=NOUPDATE ) {
/*  
      uint t = MTb[T+1]; T += t>>SCALElog;
      int wr1 = t&mSCALE;
      int dp = P + Mw-SCALE + ((SCALE-2*Mw)&(-c));
      int q = P - ((dp*wr1)>>SCALElog);
      P = q - ((q&SCALE)>>1);
*/
      Update( c, MTb, Mw );
    }

  }

  void Update( int c, const word* MTb, const int Mw ) {
//    Process( *(RangeCoder*)0, c, MTb, Mw, ONLYUPDATE );
    T = __min( MTb[0], T+1 );
    int wr1 = MTb[T];
    int omp = (wr1*Mw)>>SCALElog;//*0.5;
    int q = (P*(SCALE-wr1)+mSCALE)>>SCALElog;
    q += (c?omp:wr1-omp);
    q = __max(1,__min(mSCALE, q ));
    P = q;
  }

};

template <int len>
struct Periodic1ii {

  Node2ii P[len];
  int     l;
  int     c;

  void Init_Periodic1i() {
    Init_Node2ii( P );
    l=0; c=0;
  }

  template< class RangeCoder >
  void Process( RangeCoder& rc, int &b, const word* wr, const int mw ) {
    P[l].Process( rc, b, wr, mw );
    if( l<len-1 ) l++;
    if( l!=1 && c!=b ) l=0;
    c = b;
  }

};


template <class T>                   static void Init_Node2ii( T& x ) { x.Init_Node2i(); };
template <class T,int N>             static void Init_Node2ii( T (&x)[N] ) { for( uint i=0; i<N; i++ ) Init_Node2ii(x[i]); };
template <class T,int N,int M>       static void Init_Node2ii( T (&x)[N][M] ) { for( uint i=0; i<N; i++ ) Init_Node2ii(x[i]); };
template <class T,int N,int M,int L> static void Init_Node2ii( T (&x)[N][M][L] ) { for( uint i=0; i<N; i++ ) Init_Node2ii(x[i]); };

template <class T>                   static void Init_Periodic1ii( T& x ) { x.Init_Periodic1i(); };
template <class T,int N>             static void Init_Periodic1ii( T (&x)[N] ) { for( uint i=0; i<N; i++ ) Init_Periodic1ii(x[i]); };
template <class T,int N,int M>       static void Init_Periodic1ii( T (&x)[N][M] ) { for( uint i=0; i<N; i++ ) Init_Periodic1ii(x[i]); };
template <class T,int N,int M,int L> static void Init_Periodic1ii( T (&x)[N][M][L] ) { for( uint i=0; i<N; i++ ) Init_Periodic1ii(x[i]); };
