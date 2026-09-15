// Precomputed table for pt(i) = 16384 / (i + i + 3)
struct Ptable {

//  Array<int> t;
  int t[1024];

  int operator()(word x) const { return t[x]; }

  Ptable() /*:t(1024)*/ {
    for (int i = 0; i < 1024; ++i) {
      t[i] = 16384 / (i + i + 3);
    }
  }

};

extern Ptable pt;   // defined once, in ../tangelo_w.cpp
