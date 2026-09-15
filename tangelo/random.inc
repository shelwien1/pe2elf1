// Random generator

struct Random {

//  Array<uint> table;
  uint table[64];
  int i;

  Random() {
    table[0] = 123456789;
    table[1] = 987654321;
    for (int j = 0; j < 62; j++) {
      table[j + 2] = table[j + 1] * 11 + table[j] * 23 / 16;
    }
    i = 0;
  }

  uint operator()() {
    return ++i, table[i & 63] = table[(i - 24) & 63] ^ table[(i - 55) & 63];
  }

};

extern Random rnd;   // defined once, in ../tangelo_w.cpp
