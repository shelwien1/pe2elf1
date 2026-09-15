// Logarithm

struct Ilog {

//  Array<byte> t;
  byte t[65536];

  int operator()(word x) const { return t[x]; }

  Ilog() /*:t(65536)*/ {
    uint x = 14155776;
    for (int i = 2; i < 65536; ++i) {
      x += 774541002 / (i * 2 - 1);
      t[i] = x >> 24;
    }
  }

};

extern Ilog ilog;   // defined once, in ../tangelo_w.cpp
