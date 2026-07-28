/* xad_ms.inc -- part of xadpcm.cpp; not standalone.
 *
 * MS ADPCM arithmetic.  Mirrors libavcodec/adpcm.c; drives prediction only,
 * so a mismatch would cost ratio, never correctness. */

static const int MS_MAXCOEF = 32;
static const i32 MS_DELTA_MAX = 2796202; // INT_MAX/768, ffmpeg's ceiling

static const short ms_adapt[16] = {230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230};

static const short ms_std_coef[7][2] = {{256, 0}, {512, -256}, {0, 0}, {192, 64}, {240, 0}, {460, -208}, {392, -232}};

static INLINE i32 clip16(i64 v) {
  return i32(v<-32768 ? -32768 : v>32767 ? 32767 : v);
}

// the block's 2-tap prediction.  The coefficients are the 256-scaled pair the
// fmt chunk carries and the division truncates toward zero, which is what
// ffmpeg's (s1*c1/4 + s2*c2/4)/64 comes to for any table with multiple-of-4
// entries -- the standard one included.
static INLINE i32 ms_predict(i32 s1, i32 s2, i32 c1, i32 c2) {
  return i32((i64(s1)*i64(c1)+i64(s2)*i64(c2))/256);
}

static INLINE i32 ms_apply(i32 pred, int nib, i32 delta) {
  return clip16(i64(pred)+i64((nib&8) ? nib-16 : nib)*i64(delta));
}

static INLINE i32 ms_delta_update(i32 delta, int nib) {
  i32 d = i32((i64(ms_adapt[nib])*i64(delta))>>8);
  return d<16 ? 16 : d>MS_DELTA_MAX ? MS_DELTA_MAX : d;
}

// the MS quantizer: round half away from zero into a 4-bit signed cell.  err
// is the signed leftover, i.e. where inside its cell the value sat -- MS
// rounds to nearest, so the sign of it says which neighbour is the near miss.
static INLINE int ms_quantize(i64 diff, i32 delta, i64 &err) {
  i32 d = delta<1 ? 1 : delta;
  i64 q = (diff+(diff>=0 ? d/2 : -(d/2)))/d;
  q = q<-8 ? -8 : q>7 ? 7 : q;
  err = diff-q*i64(d);
  return int(q&15);
}

/* iDelta has no small index the way the IMA step has, so it is read in the log
   domain -- the octave plus two bits below the leading one, four steps to the
   octave over the whole legal range -- and C0's threshold map decides how many
   of those steps actually become separate contexts.  The map is what the
   optimizer moves: it can widen the buckets where iDelta does not discriminate
   and keep them fine where it does, which no single shift can express. */
/* Resolution of the log reading, NOT a free parameter: MS_DLOG_MAX+1 is the
   number of input positions C0's Md pattern has to cover, so changing either
   of these means relengthening that pattern to match. */
static const int MS_DLOG_SUB = 2;                                                                     // bits kept below the leading one
static const int MS_DLOG_E0 = 4;                                                                      // iDelta floors at 16, so log2 >= 4
static const int MS_DLOG_EMAX = 21;                                                                   // iDelta <= MS_DELTA_MAX
static const int MS_DLOG_MAX = ((MS_DLOG_EMAX-MS_DLOG_E0)<<MS_DLOG_SUB)+(1<<MS_DLOG_SUB)-1;           // 71

static INLINE int ms_dlog(i32 delta) {
  u32 d = delta<16 ? 16u : u32(delta);
  int e = 0;
  for( u32 x = d; x >>= 1; )
    e++;
  int b = ((e-MS_DLOG_E0)<<MS_DLOG_SUB)+int((d>>(e-MS_DLOG_SUB))&((1<<MS_DLOG_SUB)-1));
  return b<0 ? 0 : b>MS_DLOG_MAX ? MS_DLOG_MAX : b;
}

static INLINE int ms_bucket(i32 delta) {
  return MakeMd(ms_dlog(delta));
}

/* Confidence: where inside its cell the prediction sat.  MS rounds to nearest,
   so the signed position carries a direction the IMA leftover cannot -- a
   prediction just below the upper edge misses upward.  MSCONF 1 keeps only the
   distance, for comparison. */

static INLINE int ms_conf(i64 e, i32 delta) {
  i32 d = delta<1 ? 1 : delta;
  int c;
  if( !M0_MSCONF )
    c = int(((e+i64(d/2))*NCONF)/(i64(d)+1));
  else {
    i64 a = e<0 ? -e : e;
    c = int((a*2*NCONF)/(i64(d)+1));
  }
  return c<0 ? 0 : c>NCONF-1 ? NCONF-1 : c;
}

/* Bit-tree order for the nibble.  The MS code is a two's-complement quantized
   residual, so its natural bit order splits on the sign and then runs the
   negative half backwards.  MSMAP picks what the tree asks first:
     0  raw two's complement
     1  sign, then magnitude  (the IMA layout)
     2  magnitude, then sign  (zigzag) */
static u8 MS_M[16], MS_U[16];

static void init_ms_map() {
  int m = pclamp(M0_MSMAP, 0, 2);
  for( int i = 0; i<16; i++ ) {
    int v = i<8 ? i : i-16, s;
    if( m==0 )
      s = i; // raw two's complement
    else if( m==1 )
      s = v>=0 ? v : 8+(-v-1);       // sign, then magnitude
    else
      s = v>=0 ? 2*v : -2*v-1;         // magnitude, then sign (zigzag)
    MS_M[i] = u8(s);
    MS_U[s] = u8(i);
  }
}
