static int squash( int d ) {
  static const int t[33] = {
    1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,2047,2549,2994,3348,
    3607,3785,3901,3975,4022,4050,4068,4079,4085,4089,4092,4093,4094};
  if (d > 2047) return 4095;
  if (d < -2047) return 0;
  int w = d & 127;
  d = (d >> 7) + 16;
  return (t[d] * (128 - w) + t[(d + 1)] * w + 64) >> 7;
}

struct Stretch {

//  Array<short> t;
  short t[4096];

  int operator()(int x) const { return t[x]; }

  Stretch() /*:t(4096)*/ {
    int j = 0;
    for (int x = -2047; x <= 2047; ++x) {
      int i = squash(x);
      while (j <= i) t[j++] = x;
    }
    t[4095] = 2047;
  }

};

extern Stretch stretch;   // defined once, in ../tangelo_w.cpp
