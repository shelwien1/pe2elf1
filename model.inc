
enum {
  SCALElog = 12,
  SCALE = 1<<SCALElog,
  hSCALE = SCALE/2
};

struct Predictor {
  enum{ N=0x100 };

  word p1[N]; // Probability of 1, aggressive update
  word p2[N]; // less aggressive update
  iMixer mix[N];
  int cxt; // Context: last 0-8 bits with a leading 1

  Predictor() {
    cxt = 1;
    for( int i = 0; i < N; i++ ) {
      p1[i] = hSCALE;
      p2[i] = hSCALE;
    }
  }

  int P() {
    return mix[cxt].Mixup( p1[cxt], p2[cxt] );
//    return ((p1[cxt] + p2[cxt]) >> 1); // Combine two probabilities
  }

  void update(int y) {

    mix[cxt].Update( y, 32768/6,100, 100, p1[cxt], p2[cxt] );

    if (y) {
      p1[cxt] += ((SCALE - p1[cxt]) >> 4);
      p2[cxt] += ((SCALE - p2[cxt]) >> 8);
    } else {
      p1[cxt] -= (p1[cxt] >> 4);
      p2[cxt] -= (p2[cxt] >> 8);
    }

    cxt += cxt + y;

    if( cxt>=0x100 ) cxt=1;
  }
};
