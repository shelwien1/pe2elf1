/*  IDX's runtime, from Shelwien's psrc tree by way of oggcomp
    (balrogg-P-hash sh_mapping.inc): `mapping`, the threshold quantization,
    and `masking`, the bit selection, which the code IDX/idx2inc.pl
    generates builds its maps with.  See IDX/IDX-FORMAT.md sec.13.

    `mapping` is handed a descriptor string and, after it, the pattern --
    `S += strlen(S)+1` steps over the first to reach the second -- and builds
    the bucket map from the '1' positions (Init(), as the program starts:
    mapping_make() makes one).  That indirection is the whole trick the
    format turns on.  `pdesc` puts "!MAP!" at the front of the
    descriptor, so IDX/opt.pl can find the pattern by scanning the
    executable and hill-climb it *in the binary*, with no parse and no
    rebuild.  `mdesc` builds the identical object without the marker, which
    is how a knob is frozen out of the search space; `pmask` and `mmask` the
    same for a masking.  (The library's masking_b, for IDX's "b&" lines,
    which h264p's IDX has none of, is left out.)  */

namespace h264p {

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
    pmap = (int*)mem_alloc((MSize + 1) * sizeof(int));
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

};

// the selected bits of a value, in order (an index's "&" line)
struct masking {
  int  Size, SMSize, bits;
  int* pmap;

  void Init(const char* S1) {
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
    pmap = (int*)mem_alloc(SMSize * sizeof(int));
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

// Tunable: the descriptor starts with "!MAP!", so IDX/opt.pl's scan of the
// executable finds the pattern and hill-climbs it in place.
#define pdesc(Desc, Ofs, Map) mapping Desc = mapping_make(Ofs, "!MAP!" #Desc "!" #Ofs "\x00" Map)
#define pmask(Desc, Map) masking Desc = masking_make("!MAP!" #Desc "!0\x00" Map)

// Frozen (IDX-FORMAT.md sec.10): the same object built from the same
// pattern, but the descriptor carries no "!MAP!" marker, so the optimizer's
// scan never finds it and the value compiles in exactly as written.  The
// pattern has to stay a separate literal after the "\x00" -- \x escapes are
// greedy in hex digits, so one merged literal would swallow its leading
// zeroes -- and Init() still steps over the descriptor with
// S += strlen(S)+1, so both strings must be there.
#define mdesc(Desc, Ofs, Map) mapping Desc = mapping_make(Ofs, #Desc "!" #Ofs "\x00" Map)
#define mmask(Desc, Map) masking Desc = masking_make(#Desc "!0\x00" Map)

}  // namespace h264p
