
word coeffsU[32] = {
  U_stem00, U_stem01, U_stem02, U_stem03, U_stem04, U_stem05, U_stem06, U_stem07,
  U_stem08, U_stem09, U_stem10, U_stem11, U_stem12, U_stem13, U_stem14, U_stem15,
  U_stem16, U_stem17, U_stem18, U_stem19, U_stem20, U_stem21, U_stem22, U_stem23,
  U_stem24, U_stem25, U_stem26, U_stem27, U_stem28, U_stem29, U_stem30, U_stem31
};

/*
word coeffsV[32] = {
  V_stem00, V_stem01, V_stem02, V_stem03, V_stem04, V_stem05, V_stem06, V_stem07,
  V_stem08, V_stem09, V_stem10, V_stem11, V_stem12, V_stem13, V_stem14, V_stem15,
  V_stem16, V_stem17, V_stem18, V_stem19, V_stem20, V_stem21, V_stem22, V_stem23,
  V_stem24, V_stem25, V_stem26, V_stem27, V_stem28, V_stem29, V_stem30, V_stem31
};
*/

word coeffsW[32] = {
  W_stem00, W_stem01, W_stem02, W_stem03, W_stem04, W_stem05, W_stem06, W_stem07,
  W_stem08, W_stem09, W_stem10, W_stem11, W_stem12, W_stem13, W_stem14, W_stem15,
  W_stem16, W_stem17, W_stem18, W_stem19, W_stem20, W_stem21, W_stem22, W_stem23,
  W_stem24, W_stem25, W_stem26, W_stem27, W_stem28, W_stem29, W_stem30, W_stem31
};

struct valmap {

  word stems[32+1];
  word sq[hSCALE+1];
  word st[hSCALE+1];

  valmap( word* coeffs ) {
    int i,j,c, k,r;

    for( i=0; i<32; i++ ) stems[i]=coeffs[i];
    for( i=32-2; i>=0; i-- ) if( stems[i]>stems[i+1] ) stems[i]=stems[i+1];
    stems[32] = hSCALE;

    enum {
      STEM_COUNT = 33, // number of vertices (on the left side, x<0)
      DELTA = hSCALE/(STEM_COUNT-1)
    };

    for( i=0,k=0; i<hSCALE; i+=DELTA,k++ )
      for( r=0; r<=DELTA; r++ ) sq[i+r] = ((DELTA-r)*stems[k+0] + r*stems[k+1])/DELTA;

    for( i=0; i<hSCALE; i++ ) for( j=sq[i]; j<sq[i+1]; j++ ) st[j]=hSCALE-i;
    st[0] = hSCALE;
  }

  int Stretch( int x ) { return x<hSCALE ? -st[x] : st[SCALE-x]; }

  int Squash0( int x ) { return x>=0 ? sq[x] : 0; }
  int Squash( int x )  { 
    return x>=0 ? SCALE-Squash0(hSCALE-x) : Squash0(hSCALE+x);
  }

};

valmap U(coeffsU);
//valmap V(coeffsV);
valmap W(coeffsW);

struct iMixer {
  word w;  // weights

  void Init_Mixer( int w0 ) { 
    w = w0 + hSCALE;
  }

  int rdiv( int x, int a, int d ) {  
    return x>=0 ? (x+a)>>d : -((-x+a)>>d);
  }

  static int st( int p ) { return U.Stretch(p); }
  static int sq( int p ) { return W.Squash(p);  }

  int Mixup( int s0, int s1, int wr ) {
    int x = s1 + rdiv((w-hSCALE)*(s0-s1),1<<(SCALElog-1),SCALElog);
    x = rdiv( (x*wr), 1<<(SCALElog-1), SCALElog );
    return x;
  }

  void Update( int y, int p0,int p1, int wq, int pm ) {
    int py = SCALE - (y<<SCALElog);
    int e = (py-pm);
    int d = rdiv( e*(p0-p1), 1<<(SCALElog-1), SCALElog );
    d = rdiv( d*wq, 1<<(SCALElog-1), SCALElog );
    w += d;
  }
};

template< class zzz >
void InitMixers( zzz& T, int P0 ) {
  uint i, size = sizeof(T);
  P0 = Max(0,Min(int(SCALE),P0));
  size /= sizeof(iMixer);
  iMixer* p = (iMixer*)&T;
  for( i=0; i<size; i++ ) p[i].Init_Mixer(P0);
}

template< class zzz >
void InitMixers( zzz* p, int size, int P0 ) {
  uint i;
  P0 = Max(0,Min(int(SCALE),P0));
  for( i=0; i<size; i++ ) p[i].Init_Mixer(P0);
}
