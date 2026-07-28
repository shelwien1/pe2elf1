/* xad_ima.inc -- part of xadpcm.cpp; not standalone.
 *
 * IMA ADPCM arithmetic.  Mirrors adpcm-lib; drives prediction only, so a
 * mismatch would cost ratio, never correctness. */

static const u16 step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

/* These four all take the code width, and all four sit on the per-symbol path:
   index_update ran a switch on it once per coded symbol, and the other three
   ran bps-1-trip loops.  The callers now know BPS at compile time, so it
   becomes a template parameter here too -- same arithmetic, no branch, the
   loops fully unrolled.  The tables are at file scope rather than function
   statics so the `if constexpr` arms do not each carry their own. */
static const signed char ima_t3[4] = {-1, -1, 1, 2};
static const signed char ima_t4[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static const signed char ima_t5[16] = {-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16};

template<int BPS>
static INLINE int index_update(int index, int code) {
  if constexpr( BPS==2 )
    index += (code&1)*3-1;
  else if constexpr( BPS==3 )
    index += ima_t3[code&3];
  else if constexpr( BPS==5 )
    index += ima_t5[code&15];
  else
    index += ima_t4[code&7];
  return index<0 ? 0 : index>88 ? 88 : index;
}

// magnitude a code reconstructs to: base + the set bit weights
template<int BPS>
static INLINE int code_magnitude(int code, int step) {
  const int nb = BPS-1;
  int mag = step>>nb;
  for( int i = 0; i<nb; i++ )
    if( code&(1<<(nb-1-i)) )
      mag += step>>i;
  return mag;
}

template<int BPS>
static INLINE i32 ima_apply(i32 pcm, int code, int step) {
  int mag = code_magnitude<BPS>(code, step);
  pcm += (code&(1<<(BPS-1))) ? -mag : mag;
  return pcm<-32768 ? -32768 : pcm>32767 ? 32767 : pcm;
}

// the standard IMA quantizer: greedy from the coarsest weight down.
// leftover says how far into its cell the value sat.
template<int BPS>
static INLINE int ima_quantize(i32 diff, int step, int &leftover) {
  const int nb = BPS-1;
  int code = 0;
  i32 a = diff<0 ? -diff : diff;
  for( int i = 0; i<nb; i++ ) {
    i32 w = step>>i;
    if( a>=w ) {
      code |= 1<<(nb-1-i);
      a -= w;
    }
  }
  leftover = int(a>0x7FFF ? 0x7FFF : a);
  return code|(diff<0 ? 1<<nb : 0);
}
