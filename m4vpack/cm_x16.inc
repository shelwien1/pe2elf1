// cm_x16.inc -- cm.inc's cm_x16: a mixer's inputs packed to 16 bits, once for the mixers sharing
// them.

// The inputs of a mixer of N (a multiple of 8) as 16-bit lanes, packed once
// for the mixers that share them: 16 at a time in the order
// _mm256_packs_epi32 leaves them (the weights are packed alike), the last 8
// in a half
template <int N> struct cm_x16 {
#ifdef M4V_AVX2
  __m256i v[N / CM_V16 ? N / CM_V16 : 1];
  __m128i t;
  INLINE void pack(pcInt x) {
    for(int i = 0; i + CM_V16 <= N; i += CM_V16)
      v[i / CM_V16] = _mm256_packs_epi32(_mm256_loadu_si256((const __m256i *)(x + i)),
                                         _mm256_loadu_si256((const __m256i *)(x + i + CM_V32)));
    if(N % CM_V16)
      t = _mm_packs_epi32(_mm_loadu_si128((const __m128i *)(x + N - CM_V32)), _mm_loadu_si128((const __m128i *)(x + N - CM_H32)));
  }
#else
  INLINE void pack(const int *) {}
#endif
};
