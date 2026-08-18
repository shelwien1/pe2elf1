#include <string.h>

struct mapping {
  int  left;
  int  Size, MSize;
  int* pmap;
  volatile int value;

  NOINLINE
  mapping(int L, const char* S1) {
    const char* S = S1;
    S += strlen(S) + 1;
    left = L - 1;
    MSize = strlen(S);
    pmap = new int[MSize + 1];
    int i, j;
    value = 0;
    for (i = 0, j = 0; i < MSize; i++) {
      pmap[i] = j;
      j += (S[i] & 1);
      // Split to avoid Clang's -Wunsequenced error
      value <<= 1;
      value += (S[i] & 1);
    }
    pmap[i] = j;
    Size = j + 1;
    value += L;
  }

  int map(int x) {
    x -= left;
    if (x < 0) x = 0;
    if (x > MSize) x = MSize;
    return pmap[x];
  }

  template<class INT> void inc(INT& y, int r) { y = y * Size + map(r); }

  template<class INT> void inc(INT& y, int r, int f_cat) { y = y * 1 + map(r); }
};

struct masking {
  int  Size, SMSize, bits;
  int* pmap;

  masking(const char* S1) {
    const char* S = S1;
    S += strlen(S) + 1;
    int MSize = strlen(S);
    unsigned int mask; // Fixed undefined 'uint'
    int i, j;
    mask = 0;
    bits = 0;
    for (i = 0; i < MSize; i++) {
      mask <<= 1;
      mask += (S[i] & 1); 
      bits += (S[i] & 1);
    }
    SMSize = 1 << MSize;
    pmap = new int[SMSize];
    Size = 1 << bits;
    for (i = 0, j = 0; i < Size; i++) {
      pmap[j] = i;
      j = ((j | (~mask)) + 1) & mask;
    }
    for (i = 0; i < SMSize; i++) pmap[i] = pmap[i & mask];
  }

  int map(int x) {
    if (x < 0) x = 0;
    if (x >= SMSize) x = SMSize - 1;
    return pmap[x];
  }

  void inc(int& y, int r) {
    r &= SMSize - 1;
    y <<= bits;
    y += pmap[r];
  }
};

struct masking_b {
  int  Size, SMSize;
  int* pmap;

  masking_b(const char* S1) {
    const char* S = S1;
    S += strlen(S) + 1;
    int MSize = strlen(S);
    unsigned int mask;
    int b, i, j, k;
    mask = 0;
    for (i = 0; i < MSize; i++) {
      mask <<= 1;
      mask += (S[i] & 1); 
    }
    SMSize = 1 << MSize;
    pmap = new int[SMSize];
    for (i = 0; i < SMSize; i++) pmap[i] = 0;
    for (b = MSize - 1, k = 0; b >= 0; b--) {
      for (i = (1 << (MSize - 1 - b)); i < (2 << (MSize - 1 - b)); i++) {
        j = (i & (int(~mask) >> (b + 1)));
        pmap[i] = (j < i) ? pmap[j] : k++;
      }
    }
    Size = k;
  }

  int map(int x) {
    if (x < 1) x = 1;
    if (x >= SMSize) x = SMSize - 1;
    return pmap[x];
  }

  void inc(int& y, int r) {
    y *= Size;
    y += map(r);
  }
};

// Kept just in case you rely on them elsewhere
#define GCCBUG1(X) #X
#define GCCBUG2(X,Y) X##Y

// Profiling version: Safely refactored string concatenation
#define pdesc(Desc, Ofs, Map) mapping Desc(Ofs, "!MAP!" #Desc "!" #Ofs "\x00" Map)

#define pmask(Desc, Map) masking Desc("!MAP!" #Desc "!0\x00" Map)

#define pmask2(Desc, Map) masking_b Desc("!MAP!" #Desc "!0\x00" Map)


// Frozen variants (IDX-FORMAT.md section 10).  Same objects, but the descriptor
// string does not begin with "!MAP!", so opt.pl's scan of the executable never
// finds them and the knob is out of the search space.  The two strings must
// stay separate literals: "\x00" ends one and Map begins the next, because
// \x escapes are greedy in hex digits and a single literal would swallow the
// pattern's leading zeroes into the escape.
#define mdesc(Desc, Ofs, Map) mapping Desc(Ofs, "map." #Desc "." #Ofs "\x00" Map)

#define mmask(Desc, Map) masking Desc("map." #Desc ".0\x00" Map)

#define mmask2(Desc, Map) masking_b Desc("map." #Desc ".0\x00" Map)
