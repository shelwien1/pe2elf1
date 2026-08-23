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

// Tunable: the descriptor starts with "!MAP!", so IDX/opt.pl's scan of the
// executable finds the pattern and hill-climbs it in place.
#define pdesc_live(Desc, Ofs, Map) mapping Desc(Ofs, "!MAP!" #Desc "!" #Ofs "\x00" Map)
#define pmask_live(Desc, Map) masking Desc("!MAP!" #Desc "!0\x00" Map)
#define pmask2_live(Desc, Map) masking_b Desc("!MAP!" #Desc "!0\x00" Map)

// Frozen (IDX-FORMAT.md sec.10): the same object built from the same
// pattern, but the descriptor carries no "!MAP!" marker, so the optimizer's
// scan never finds it and the value compiles in exactly as written.  The
// pattern has to stay a separate literal after the "\x00" -- \x escapes are
// greedy in hex digits, so one merged literal would swallow its leading
// zeroes -- and the constructor still steps over the descriptor with
// S += strlen(S)+1, so both strings must be there.
#define mdesc(Desc, Ofs, Map) mapping Desc(Ofs, #Desc "!" #Ofs "\x00" Map)
#define mmask(Desc, Map) masking Desc(#Desc "!0\x00" Map)
#define mmask2(Desc, Map) masking_b Desc(#Desc "!0\x00" Map)

// The indirection lets a consumer freeze a whole generated header without
// editing it: #undef pdesc / #define pdesc(D,O,M) mdesc(D,O,M) around the
// #include, then point it back at pdesc_live.  bmpc does that to coder0's
// C0 block, which is shared and was tuned on text.
#define pdesc(Desc, Ofs, Map) pdesc_live(Desc, Ofs, Map)
#define pmask(Desc, Map) pmask_live(Desc, Map)
#define pmask2(Desc, Map) pmask2_live(Desc, Map)

