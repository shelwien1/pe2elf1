
struct iMixer {
  word w;

  enum { SCALElog=15, SCALE=1<<SCALElog };

  iMixer( int W=SCALE ) : w(W) {}

  int Mixup( int p1, int p2 ) {
    return p1+(((p2-p1)*w)>>SCALElog);
  }

  void Update( int s, int wr,int mw, int Limit, int p1_,int p2_ ) {
    int p1 = SCALE-(p1_<<3);
    int p2 = SCALE-(p2_<<3);
    if( (p2>Limit+p1) || (p2<p1-Limit) ) {
      int P = s ? mw : SCALE-mw;
      w = __max(0,__min(SCALE, (SCALE-wr)*w/SCALE + wr*(P-p1)/(p2-p1) ));
    }
  }

};
