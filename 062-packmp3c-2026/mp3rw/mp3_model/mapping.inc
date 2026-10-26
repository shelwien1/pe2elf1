/*  IDX's runtime (IDX/IDX-FORMAT.md sec.13): aacpack's mapping.inc --
    `mapping`, the threshold quantization, and `masking`, the bit selection --
    with the library's `masking_b` for mp3c's "b&" lines (sec.16), and what
    the code IDX/idx2inc.pl generates asks of its host: tc_vmul for the
    Volumes, and tc_map / tc_unmap / tbl_n for the tables of the tuning build
    (UseNew 1, where Table() members are allocated as the model starts).

    `mapping` is handed a descriptor string and, after it, the pattern --
    `S += strlen(S)+1` steps over the first to reach the second -- and builds
    the bucket map from the '1' positions.  `pdesc` puts "!MAP!" at the front
    of the descriptor, so IDX/opt.pl can find the pattern by scanning the
    executable and hill-climb it in the binary, with no parse and no rebuild;
    `mdesc` builds the identical object without the marker, which is how a
    knob is frozen out of the search space (sec.10).  `pmask`/`mmask` and
    `pmask2`/`mmask2` the same for a masking and a masking_b.  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct mapping {
  int  left;
  int  Size, MSize;
  int* pmap;
  volatile int value;

  NOINLINE
  void Init(int L, const char* S1) {
    const char* S = S1;
    S += strlen(S) + 1;
    left = L - 1;
    MSize = strlen(S);
    pmap = (int*)malloc((MSize + 1) * sizeof(int));
    int i, j;
    unsigned v = 0;  // (a Number's: its bits, with L; a longer pattern's is not read)
    for (i = 0, j = 0; i < MSize; i++) {
      pmap[i] = j;
      j += (S[i] & 1);
      v = (v << 1) + (unsigned)(S[i] & 1);
    }
    pmap[i] = j;
    Size = j + 1;
    value = (int)(v + (unsigned)L);
  }

  int map(int x) {
    x -= left;
    if (x < 0) x = 0;
    if (x > MSize) x = MSize;
    return pmap[x];
  }

  template<class INT> void inc(INT& y, int r) { y = y * Size + map(r); }

};

// the selected bits of a value, in order (an index's "&" line)
struct masking {
  int  Size, SMSize, bits;
  int* pmap;

  void Init(const char* S1) {
    const char* S = S1;
    S += strlen(S) + 1;
    int MSize = strlen(S);
    unsigned int mask;
    int i, j;
    mask = 0;
    bits = 0;
    for (i = 0; i < MSize; i++) {
      mask <<= 1;
      mask += (S[i] & 1);
      bits += (S[i] & 1);
    }
    SMSize = 1 << MSize;
    pmap = (int*)malloc(SMSize * sizeof(int));
    Size = 1 << bits;
    for (i = 0, j = 0; i < Size; i++) {
      pmap[j] = i;
      j = ((j | (~mask)) + 1) & mask;
    }
    for (i = 0; i < SMSize; i++) pmap[i] = pmap[i & mask];
  }

  void inc(int& y, int r) {
    r &= SMSize - 1;
    y <<= bits;
    y += pmap[r];
  }
};

// the node of a binary tree, 1 then 2-3, 4-7, ... (a "b&" line): a 1 in the
// pattern ignores the decision at that depth, so that the nodes differing
// only there share a row; Size is the rows left
struct masking_b {
  int  Size, SMSize;
  int* pmap;

  void Init(const char* S1) {
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
    pmap = (int*)malloc(SMSize * sizeof(int));
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

// (one made, for a map's static: its maps last as long as the program)
NOINLINE mapping mapping_make(int L, const char* S1) {
  mapping m;
  m.Init(L, S1);
  return m;
}
NOINLINE masking masking_make(const char* S1) {
  masking m;
  m.Init(S1);
  return m;
}
NOINLINE masking_b masking_b_make(const char* S1) {
  masking_b m;
  m.Init(S1);
  return m;
}

// Tunable: the descriptor starts with "!MAP!", so IDX/opt.pl's scan of the
// executable finds the pattern and hill-climbs it in place.
#define pdesc(Desc, Ofs, Map) mapping Desc = mapping_make(Ofs, "!MAP!" #Desc "!" #Ofs "\x00" Map)
#define pmask(Desc, Map) masking Desc = masking_make("!MAP!" #Desc "!0\x00" Map)
#define pmask2(Desc, Map) masking_b Desc = masking_b_make("!MAP!" #Desc "!0\x00" Map)

// Frozen (IDX-FORMAT.md sec.10): the same object built from the same
// pattern, but the descriptor carries no "!MAP!" marker, so the optimizer's
// scan never finds it and the value compiles in exactly as written.  The
// pattern has to stay a separate literal after the "\x00" -- \x escapes are
// greedy in hex digits, so one merged literal would swallow its leading
// zeroes -- and Init() still steps over the descriptor with
// S += strlen(S)+1, so both strings must be there.
#define mdesc(Desc, Ofs, Map) mapping Desc = mapping_make(Ofs, #Desc "!" #Ofs "\x00" Map)
#define mmask(Desc, Map) masking Desc = masking_make(#Desc "!0\x00" Map)
#define mmask2(Desc, Map) masking_b Desc = masking_b_make(#Desc "!0\x00" Map)

// A Volume is a product of factor sizes, folded through a saturating
// multiply, so that a product too large reaches whatever checks it as at
// least what was asked for (IDX-FORMAT.md sec.6).
constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }

// A table's count, checked (the tuning build's tables, sized by Volumes
// opt.pl can widen), and its memory: zeroed, as the shipping build's
// tables are, members of a static object.
static inline unsigned long long tbl_n(long long n) {
  if (n < 0 || n > ((long long)1 << 34)) {
    fprintf(stderr, "mp3c: an IDX table of %lld entries\n", n);
    exit(1);
  }
  return (unsigned long long)n;
}
static void* tc_map(unsigned long long n) {
  void* p = calloc(1, n ? (size_t)n : 1);
  if (!p) {
    fprintf(stderr, "mp3c: no memory for an IDX table of %llu bytes\n", n);
    exit(1);
  }
  return p;
}
static void tc_unmap(void* p, unsigned long long) { free(p); }
