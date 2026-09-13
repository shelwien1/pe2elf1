
struct LinearMixer {

  word w;

//  LinearMixer( int W=SCALE/2 ) : w(W) {}

  int Mixup( int p1, int p2 ) {
    return p1+(((p2-p1)*w)>>SCALElog);
  }

};

struct Counter {

  word P; 

//    q = (q*(SCALE-wr) + dp*wr) >> SCALElog;
  void update( const int c, const int wr, const int mw ) {
    int dp = c ? mw : (SCALE-mw);
    dp = ((P-dp)*wr) >> SCALElog;
    int q = P - dp + c;
    P = q & mSCALE;
  }

};
