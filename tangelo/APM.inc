// APM - stationary map combining a context and an input probability.
template< int N >
struct APM {
  int index;
  word t[N*33];

  void Init( void ) {
    index=0;
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < 33; ++j) {
        t[i * 33 + j] = i == 0 ? squash((j - 16) * 128) * 16 : t[j];
      }
    }
  }

  int p(int pr = 2048, int cxt = 0, int rate = 7) {
    pr = stretch(pr);
    int g = (y << 16) + (y << rate) - y - y;
    W(t[index]) += (g - t[index]) >> rate;
    W(t[index + 1]) += (g - t[index + 1]) >> rate;
    const int w = pr & 127;
    W(index) = ((pr + 2048) >> 7) + cxt * 33;
    return(t[index] * (128 - w) + t[index + 1] * w) >> 11;
  }

};
