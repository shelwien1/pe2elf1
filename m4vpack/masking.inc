// masking.inc -- masking, IDX's bit selection (an index's "&" line), and its pmask and mmask,
// tunable and frozen as mapping.inc's pdesc and mdesc are (mapping.inc says how).

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
    pmap = (int*)xmalloc(SMSize * sizeof(int));
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

// (one made, for a mask's static)
NOINLINE masking masking_make(const char* S1) {
  masking m;
  m.Init(S1);
  return m;
}

// tunable (the descriptor starts with "!MAP!") and frozen, as pdesc and mdesc
#define pmask(Desc, Map) masking Desc = masking_make("!MAP!" #Desc "!0\x00" Map)
#define mmask(Desc, Map) masking Desc = masking_make(#Desc "!0\x00" Map)
