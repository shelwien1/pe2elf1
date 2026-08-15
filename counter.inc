
//static const int SCALElog = 15;
//static const int SCALE    = 1<<SCALElog;

static const int C0_K = (64544+0) * (1);

static const int C0_P0 = (31344+1) * (1);
static const int C0_wrA = (7+0) * (1);
static const int C0_wrB = (32+0) * (1);
static const int C0_mw = (13534+1) * (1);

template< class zzz >
void InitCounters( zzz* p, int size, int P0 ) {
  uint i;
  //P0 = Max(1,Min(int(mSCALE),P0));
  for( i=0; i<size; i++ ) p[i].P=P0;
}

struct Counter {
  word P; 
};

// C.P = C_Update( C.P, bit, C0_wrA*128+C0_wrB, C0_mw );
uint C_Update( int P, const int bit, const int wr, const int mw ) {
  int dp = mw + ((SCALE-mw-mw) & (bit-1));
  dp = ((P-dp)*wr) >> SCALElog;
  P -= dp - bit;
  if_e0( P<1 ) P=1;
  if_e0( P>mSCALE ) P=mSCALE;
  return P;
}

