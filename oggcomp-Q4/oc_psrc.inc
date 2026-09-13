//  PSRC -- the probability-sorted coder, in place of putting every model
//  bit straight through the rangecoder.
//
//  The model does not change and neither does the rangecoder.  What changes
//  is what goes between them.  Before, tc_bit() handed the mixed
//  probability and the bit to rc_Process() and that was the whole coder.
//  Now tc_bit() only quantizes the probability to a class and writes one
//  byte -- class, and the bit -- into a pool.  When the pool is full the
//  block is coded in one go:
//
//      the bits are sorted by class, so that every bit in a chunk was
//      predicted at about the same probability;
//      each chunk, being a run of one near-certain symbol broken by the
//      rare one, is run-length coded;
//      the run bytes, and the per-class counts that say how many there
//      are, are what actually reaches the rangecoder.
//
//  Decoding runs it backwards: a whole block is read and expanded before
//  the model asks for its first bit of it, and then each bit is a byte read
//  from the cursor of its class.  The model is adaptive but causal -- the
//  state it predicts bit i with depends only on bits 0..i-1 -- so both
//  sides put each bit in the same block and give it the same class, which
//  is all the two have to agree on.
//
//  Two things fall out of it.  The rangecoder is no longer called once per
//  model bit but once per run byte and a few times per block -- measured,
//  about 0.8 calls per model bit on this corpus, not the large factor the
//  scheme suggests, because this model's output is not lopsided enough for
//  the runs to be long (PSRC.md section 8).  And the coder no longer codes
//  at the probability the model named: it codes at the probability its class
//  turns out to have, learned by the run model, which is a secondary
//  estimate over the model's own output and measures very slightly better
//  than it.
//
//  The class is the quantized probability with the more likely symbol
//  folded onto one side: magnitude |p - 1/2| in PS_MAGN steps, and a
//  mirror bit saying which side of 1/2 p was on.  The byte in the pool is
//  the class with bit 7 holding (mirror ^ bit), so bit 7 is 1 for whichever
//  outcome the model thought likely -- a run of 1s is a run of the model
//  being right, which is what makes the chunks compress.  The decoder
//  never has to undo the fold: a chunk of class k expands to runs of
//  (k & 1) ^ 1 broken by (k & 1), and since k's low bit is the mirror that
//  comes out as the original bit.
//
//  Where this differs from PSRC-006, and why:
//
//    - the quantizer is derived rather than imported.  PSRC's qmaq table is
//      hill-climbed on enwik8 and is not monotone; measured against this
//      model's probabilities the two are within 0.03% of each other, so the
//      derived one wins on being explicable.  It is uniform in the stretch
//      domain, which is what bounds the relative loss of bucketing (see
//      PSRC.md).
//    - the run-byte model is contexted on the class.  PSRC codes the chunks
//      in class order and lets one set of counters drift from chunk to
//      chunk, alternating the direction each block so the drift carries
//      over; with a counter set per class there is nothing to carry and the
//      order stops mattering.
//    - the counters are this tree's cm_cnt, which clamps its probability to
//      [1, 4095] and starts at a state-count-driven rate, rather than
//      PSRC's fixed-rate Counter.
//    - the block length is coded, because a stream's length in model bits
//      is not known in advance here as PSRC's f_len is.
//    - the decoder bounds every write, because a .oc may be damaged.
#ifndef OC_PSRC
#define OC_PSRC 1
#endif
#if OC_PSRC
//  Magnitude levels, so 2 * PS_MAGN classes.  64 keeps the bucketing loss
//  under 0.03% of the ideal code length on every file measured and leaves
//  the class in 7 bits, which the pool byte and the bitsort both want.
#ifndef PS_MAGN
#define PS_MAGN 64
#endif
constexpr int PS_NCLS = 2 * PS_MAGN;
static_assert(PS_NCLS <= 128, "the pool byte keeps the class in 7 bits");
//  Model bits per block.  The per-block header is one length and PS_NCLS
//  counts, a couple of hundred bytes, so at 8M bits it is 0.02% of the
//  block's output; the cost of raising it is the two buffers below.
#ifndef PS_BLOCKLOG
#define PS_BLOCKLOG 23
#endif
constexpr u32 PS_BLOCK = (u32)1 << PS_BLOCKLOG;
//  A run of 255 * PS_TRAILFF or less at the end of a chunk is not coded --
//  it is whatever the 32-bit alignment padding left -- so the decoder puts
//  that many back, and one more byte's worth for the run the padding was
//  attached to.
constexpr u32 PS_TRAILFF = 2;
constexpr u32 PS_PAD = 255 * (PS_TRAILFF + 1);
//  Run bytes one block can make.  Every byte covers at least one bit of
//  the chunk it came out of, and a chunk is its bits rounded up to a word,
//  so it is the block plus the rounding.
constexpr u32 PS_RLEMAX = PS_BLOCK + 31 * PS_NCLS;
//  What one block expands to: a byte per bit, the 32-bit alignment each
//  chunk is rounded up to, and the tail padding.
constexpr sz PS_EXPCAP = (sz)PS_BLOCK + (sz)PS_NCLS * (32 + PS_PAD) + 64;
//  And what is mapped for it.  A damaged stream cannot make the expansion
//  itself overrun -- every write is checked against PS_EXPCAP -- but it can
//  make the model read one class's cursor much further than that class's
//  chunk holds.  A block serves at most PS_BLOCK bits, so no cursor can get
//  more than that past where it started: one block's worth of slack after
//  the expansion is enough that a damaged .oc decodes to nonsense rather
//  than out of the mapping.  It is never written, so it costs no pages.
constexpr sz PS_DECCAP = PS_EXPCAP + PS_BLOCK;
//  The encoder's pool, and the bitsort's word buffer behind it: one word
//  per 32 bits of a chunk, rounded up per chunk.  The word buffer starts
//  past the room the run bytes can take, not just past the pool, so that
//  the in-place rewrite can never reach a word it has not read yet.
constexpr sz PS_TMPOFF = ((sz)PS_RLEMAX + 63) & ~(sz)63;
constexpr sz PS_ENCCAP = PS_TMPOFF + (sz)PS_BLOCK / 8 + (sz)4 * PS_NCLS + 64;
static_assert(PS_TMPOFF >= PS_RLEMAX, "the run bytes would reach the word buffer");
//  How fast the counters follow.  cm_cnt's rate is 1/(t+2) until t stops at
//  the limit, so these are the settled rates.  They are also, for a counter
//  whose probability goes right to the edge, how close to the edge it can
//  get: the update is integer, so it stalls once (target - p) * 65536/(t+2)
//  falls under 1, which is roughly t away from the edge.  A limit of 4095
//  therefore pins a near-certain flag at about 0.09 bits instead of the
//  0.0004 the 12-bit probability could say -- which is why these are low,
//  and why a flag asked where the answer is never in doubt is not free (see
//  PS_FFMAG).  Swept over the corpus; the plateau is broad, ~0.01% wide.
#ifndef PS_LIM_TREE
#define PS_LIM_TREE 255
#endif
#ifndef PS_LIM_UN
#define PS_LIM_UN 1023
#endif
#ifndef PS_LIM_FF
#define PS_LIM_FF 255
#endif
#ifndef PS_LIM_HDR
#define PS_LIM_HDR 255
#endif
//  How much evidence the run model's priors are worth, in observations.
//  0 turns them off and starts every counter at one in two.  Read it against
//  the limits above: a counter's rate is 1/(t+2) and t stops at its limit, so
//  a prior of 128 against a limit of 255 is half the counter's whole
//  adaptation budget spent on what the class already says it is.  That is
//  deliberate -- the prior is not a guess, it is the definition of the class
//  -- but the two move together and re-tuning one without the other is how
//  this goes wrong.
#ifndef PS_PRIOR
#define PS_PRIOR 128
#endif
static_assert(PS_PRIOR >= 0 && PS_PRIOR <= CM_TMAX, "the prior is a cm_cnt state count and indexes cm_rec");
//  How a run byte is coded.  A run is geometric in the class's own rare-bit
//  probability, so its length is one parameter and almost nothing else: a
//  run of "is it longer than this" decisions, one counter each, is what an
//  adaptive Golomb code is, and it costs the class a dozen counters rather
//  than a tree of 255.  That matters -- a tree per class measured 2% over
//  what the classes say the floor is, nearly all of it the price of
//  learning counters that a dozen would have done.  Past PS_UNARY the byte
//  goes through a tree after all, on a context of a few classes at a time,
//  because up there the remainder of a long run is near enough uniform that
//  what the tree learns is shared.
#ifndef PS_UNARY
#define PS_UNARY 12
#endif
#ifndef PS_TREECTX
#define PS_TREECTX 4
#endif
//  Magnitudes below this get the "the run goes on" flag; above it a run of
//  255 is rare enough that asking about it on every byte costs more than
//  the few times it happens, and those go through the escape instead.
#ifndef PS_FFMAG
#define PS_FFMAG (PS_MAGN / 2)
#endif
constexpr int PS_NTREE = 1 << PS_TREECTX;
//  -DOC_PSRC_STATS: where the output went, by part of the coder, against
//  what the classes say it could have been.  Measuring only; no effect on
//  the stream.
#ifdef OC_PSRC_STATS
enum { PSS_HDR, PSS_FF, PSS_UN, PSS_TREE, PSS_N };
static const char *const PS_PART[PSS_N] = {"block header", "run goes on", "run length", "long run"};
static double ps_cost[PSS_N];
static u64 ps_ops[PSS_N];
static int ps_cat;
static u64 ps_cls_n[PS_NCLS], ps_cls_u[PS_NCLS];
static int ps_stats_armed;
static void ps_stats_dump(void) {
  int i;
  double tot = 0, idl = 0;
  for(i = 0; i < PSS_N; i++)
    tot += ps_cost[i];
  for(i = 0; i < PS_NCLS; i++) {
    double n = (double)ps_cls_n[i], u = (double)ps_cls_u[i], q;
    if(n < 1 || u < 1 || u >= n)
      continue;
    q = u / n;
    idl += n * -(q * log2(q) + (1 - q) * log2(1 - q));
  }
  fprintf(stderr, "psrc: %.0f bytes coded, %.0f bytes the classes say is the floor (%+.2f%%)\n", tot / 8, idl / 8, idl > 0 ? 100.0 * (tot - idl) / idl : 0.0);
  for(i = 0; i < PSS_N; i++)
    fprintf(stderr, "  %-14s %10.0f bytes  %5.2f%%  %" PRIu64 " decisions, %.4f bits each\n", PS_PART[i], ps_cost[i] / 8, tot > 0 ? 100.0 * ps_cost[i] / tot : 0.0, ps_ops[i], ps_ops[i] ? ps_cost[i] / (double)ps_ops[i] : 0.0);
  for(i = 0; i < PS_NCLS; i++)
    if(ps_cls_n[i])
      fprintf(stderr, "  class %3d  %12" PRIu64 " bits  %12" PRIu64 " rare  q=%.6f\n", i, ps_cls_n[i], ps_cls_u[i], (double)ps_cls_u[i] / (double)ps_cls_n[i]);
}
#define PS_CAT(x) ps_cat = (x)
#else
#define PS_CAT(x)
#endif
//  p (12 bits, the probability of a 0 as everything here counts it) to the
//  class, with bit 7 of ps_qs holding the mirror.  ps_qr is the same
//  without it, which is what the decoder indexes its cursors by.
static u8 ps_qs[CM_PONE], ps_qr[CM_PONE];
//  And what each magnitude means: the rare-bit probability at the middle of
//  its stretch bucket.  The run model starts from this rather than from
//  "one in two", which is the only thing it could otherwise assume about a
//  class whose whole definition is how often the rare bit comes.
static u16 ps_q[PS_MAGN];
static int ps_qready;
//  What the coder actually spent, for -v: the model's own report adds up
//  what its probabilities say the file should have cost, and with PSRC in
//  between that is no longer what was written.  Measured only when -v asks
//  for it, as tc_bit's own accounting is.
static double ps_coded;
//  Uniform in the stretch domain over [1/4096, 1/2], read off cm_st so that
//  the quantizer and the model agree on what stretch means.  cm_stretch is
//  monotone, so this is too, and neighbouring classes are neighbouring
//  probabilities -- which is what lets the run model treat a class as a
//  probability and nothing else.
static void ps_tables(void) {
  int p;
  if(ps_qready)
    return;
  //  cm_st, which the loop below reads, is built here.  Called twice it
  //  builds the same tables again; not called at all it would leave this
  //  quantizer all zeroes, which still round-trips and compresses terribly,
  //  so it is not left to the caller's order.
  cm_tables();
  for(p = 1; p < CM_PONE; p++) {
    int mir = p > CM_PHALF;
    int pm = mir ? CM_PONE - p : p;
    int mag = ((CM_STMAX + cm_stretch(pm)) * PS_MAGN) >> 11;
    int k;
    mag = mag < 0 ? 0 : mag > PS_MAGN - 1 ? PS_MAGN - 1 : mag;
    k = 2 * mag + mir;
    ps_qs[p] = (u8)(k | (mir << 7));
    ps_qr[p] = (u8)k;
  }
  ps_qs[0] = ps_qs[1];
  ps_qr[0] = ps_qr[1];
  for(p = 0; p < PS_MAGN; p++) {
    int s = (2 * p + 1) * 2048 / (2 * PS_MAGN) - CM_STMAX;
    int q = cm_squash(s);
    ps_q[p] = (u16)(q < 1 ? 1 : q > CM_PHALF ? CM_PHALF : q);
  }
  ps_qready = 1;
}
//  floor(log2(x)), x >= 1.
static INLINE int ps_log2i(u32 x) { return (int)ogc_ilog(x) - 1; }
//  The pool, sorted by class and run-length coded in place.  buf holds l
//  pool bytes on the way in and the run bytes on the way out; tmp is the
//  word buffer, which is behind the pool rather than inside it so that the
//  in-place rewrite never reads what it has already written.  count[k] ends
//  up holding the run bytes of class k, and they sit in buf in class order.
static void ps_bitsort(u8 *buf, u32 *tmp, u32 l, u32 (&count)[PS_NCLS]) {
  u32 i, x;
  u32 bofs[PS_NCLS + 1];
  u32 freq[256 + 8];
  //  Where each class's words go.  A pool byte is class | (bit << 7), so a
  //  class's count is the two halves of the histogram added together.
  for(i = 0; i < DIM(freq); i++)
    freq[i] = 0;
  for(i = 0; i < l; i++)
    freq[1 + buf[i]]++;
  for(i = 1; i <= PS_NCLS; i++)
    freq[i] += freq[128 + i];
  for(i = 1; i <= PS_NCLS; i++)
    freq[i] = (freq[i] + 31) / 32;
  for(i = 0; i < PS_NCLS; i++)
    freq[i + 1] += freq[i];
  for(i = 0; i <= PS_NCLS; i++)
    bofs[i] = freq[i];
  {
    //  The bits of a class, packed 32 to a word, most significant first.
    //  The accumulator starts at 1 and a word is out when that 1 falls off
    //  the top, which is how the loop knows 32 bits have gone in without
    //  counting them.
    u32 code[PS_NCLS];
    for(i = 0; i < PS_NCLS; i++)
      code[i] = 1;
    for(i = 0; i < l; i++) {
      u32 c = buf[i];
      u32 p = c & 0x7F, bit = c >> 7;
      u32 r = code[p];
      u32 q = r + r + bit;
      if((q >> 1) < r) {
        tmp[freq[p]++] = q;
        q = 1;
      }
      code[p] = q;
    }
    //  The last, short word of each class, left-aligned and padded with 1s
    //  -- the symbol a run is made of, so the padding joins the run that
    //  was in progress instead of ending it.
    for(i = 0; i < PS_NCLS; i++) {
      u32 c = code[i];
      if(c > 1)
        tmp[freq[i]] = ~((~c) << (32 - ps_log2i(c)));
    }
  }
  //  And the runs.  A byte is how many 1s came before the next 0; 255 means
  //  255 of them and the run goes on.  The run a class ends on is the
  //  alignment padding and is dropped down to PS_TRAILFF bytes' worth,
  //  which the decoder puts back without being told.
  for(i = 0, x = 0; i < PS_NCLS; i++) {
    u32 rl = 0, y = x;
    u32 j = bofs[i], z = bofs[i + 1];
    for(; j < z; j++) {
      u32 k, c = tmp[bofs[i]++];
      for(k = 0; k < 32; k++, c += c) {
        if((i32)c < 0)
          rl++;
        else {
          while(rl >= 255) {
            buf[x++] = 0xFF;
            rl -= 255;
          }
          buf[x++] = (u8)rl;
          rl = 0;
        }
      }
    }
    while(rl > 255 * PS_TRAILFF) {
      buf[x++] = 0xFF;
      rl -= 255;
    }
    count[i] = x - y;
  }
}
template <int f_DEC> struct oc_model;
//  One direction's coder: the counters it learns, the block it is in the
//  middle of, and the buffer that block lives in.  One object per
//  direction, the way oc_model's rangecoder is one per direction, so that
//  neither has to be reached through the model.
template <int f_DEC> struct oc_psrc {
  static constexpr int ENC = (f_DEC == 0);
  //  The run-byte model: the flag that says the run goes on, the chain that
  //  says where it ends, and the tree the long ones escape to.
  //  Contexted on the magnitude, not the class: a class is a magnitude and
  //  a mirror bit, and the mirror says which way round the bits were
  //  written, not how often the rare one comes.  The two classes of a
  //  magnitude have the same rare-bit rate by construction, so giving them
  //  one set of counters is not an approximation -- it is the same
  //  statistic with twice the evidence.
  cm_cnt ff[PS_MAGN];
  cm_cnt un[PS_MAGN][PS_UNARY];
  cm_cnt tr[PS_NTREE][256];
  //  The block header: the length, and the counts.  Contexted on the depth
  //  of the count recursion -- how big a count can be at all is a function
  //  of it, and it measured better than taking the ceiling itself.
  enum { PS_LEVELS = 8 };
  cm_cnt hfull;
  cm_cnt hlen[32];
  cm_cnt csum[PS_LEVELS][32];
  cm_cnt cmax[PS_LEVELS], cmin[PS_LEVELS];
  u8 *buf;
  u8 *bend;
  int primed;            //  the run counters hold their priors, not zeroes
  u32 qn;                //  encode: pool bytes written
  u32 left;              //  decode: bits of this block not yet served
  u8 *ptr[PS_NCLS];      //  decode: where each class's next bit is
  static oc_psrc inst;
  //  One binary decision through the rangecoder, which is all that is left
  //  of the old path: p is a 12-bit probability of a 0, as tc_bit's was.
  INLINE u32 rcbit(u32 p, u32 b) {
    oc_model<f_DEC>::rc.rc_Process(p << 3, b);
    if(ENC && tc_verbose)
      ps_coded -= log2(b ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
#ifdef OC_PSRC_STATS
    if(ENC) {
      ps_cost[ps_cat] -= log2(b ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
      ps_ops[ps_cat]++;
    }
#endif
    return b;
  }
  INLINE u32 cbit(cm_cnt &c, u32 b, int lim) {
    b = rcbit((u32)c.P(), b);
    c.upd((int)b, lim, 0);
    return b;
  }
  //  A number in (lo, hi), bit by bit from the top, with the bits that the
  //  bounds already decide not coded at all.  tab is contexted on the
  //  prefix, as far as it reaches.
  u32 num(u32 n, int clen, u32 lo, u32 hi, cm_cnt *tab, int lim) {
    int i;
    u32 z = 0;
    for(i = clen - 1; i >= 0; i--) {
      u32 j = (((u32)1 << clen) + z) >> (i + 1);
      u32 k = (n >> i) & 1;
      if(z + ((u32)1 << i) - 1 <= lo)
        k = 1;
      else if(z + ((u32)1 << i) >= hi)
        k = 0;
      else if(j < 32)
        k = cbit(tab[j], k, lim);
      else
        k = rcbit(CM_PHALF, k);
      z |= k << i;
    }
    return z;
  }
  //  freq[0..N-1], every one of them, as a sum and then the split of that
  //  sum between the halves.  A subtree whose sum is zero costs one bit and
  //  says nothing more, which is what makes this cheap on a block where
  //  most classes are empty.  known says the sum is already agreed --
  //  the right half of a split, whose sum is what the left one left.
  u32 freq(u32 *f, int N, u32 max, u32 sum, int known, int level) {
    int lv = level < PS_LEVELS ? level : PS_LEVELS - 1;
    if(!known) {
      if(max == 0)
        sum = 0;
      else {
        u32 flag;
        int i;
        if(ENC)
          for(i = 0, sum = 0; i < N; i++)
            sum += f[i];
        PS_CAT(PSS_HDR);
        flag = cbit(cmax[lv], (u32)(sum == max), PS_LIM_HDR);
        if(flag)
          sum = max;
        else if(max == 1)
          sum = 0;
        else {
          flag = cbit(cmin[lv], (u32)(sum == 0), PS_LIM_HDR);
          sum = flag ? 0 : num(sum, ps_log2i(max - 1) + 1, 0, max, csum[lv], PS_LIM_HDR);
        }
      }
    }
    if(N <= 1) {
      f[0] = sum;
      return sum;
    }
    if(sum == 0) {
      if(!ENC)
        memset(f, 0, (sz)N * sizeof *f);
      return 0;
    }
    {
      int h = N >> 1;
      u32 s0 = freq(f, h, sum, 0, 0, level + 1);
      freq(f + h, N - h, 0, sum - s0, 1, level + 1);
    }
    return sum;
  }
  //  One run byte.  In the magnitudes the model is surest about, 0xFF -- a
  //  run that goes on -- is most of the stream, so it gets its own flag
  //  rather than eight decisions; above PS_FFMAG the runs are short, the
  //  flag would never be anything but no, and asking is what costs.
  INLINE u32 rbyte(u32 k, u32 c) {
    u32 i, z, g, hi, m = k >> 1;
    int b;
    PS_CAT(PSS_FF);
    if(m < PS_FFMAG && cbit(ff[m], (u32)(c == 255), PS_LIM_FF))
      return 255;
    PS_CAT(PSS_UN);
    for(i = 0; i < PS_UNARY; i++)
      if(cbit(un[m][i], (u32)(c == i), PS_LIM_UN))
        return i;
    //  PS_UNARY .. 254, or .. 255 where the flag above was not asked and 255
    //  is still a byte this can be.  Most significant bit first; a bit whose
    //  every completion would fall outside that range is not a decision and
    //  is not coded.
    PS_CAT(PSS_TREE);
    g = (m * PS_NTREE) / PS_MAGN;
    hi = m < PS_FFMAG ? 255 : 256;
    for(z = 0, b = 7; b >= 0; b--) {
      u32 t = 1;
      if(z + ((u32)1 << b) >= hi)
        t = 0;
      else if(z + ((u32)1 << b) - 1 >= PS_UNARY)
        t = cbit(tr[g][(256 + z) >> (b + 1)], (c >> b) & 1, PS_LIM_TREE);
      z |= t << b;
    }
    return z;
  }
  u32 len(u32 n) {
    PS_CAT(PSS_HDR);
    if(cbit(hfull, (u32)(n == PS_BLOCK), PS_LIM_HDR))
      return PS_BLOCK;
    return num(n, PS_BLOCKLOG, 0, PS_BLOCK, hlen, PS_LIM_HDR);
  }
  //  A counter that starts somewhere other than "one in two", with PS_PRIOR
  //  worth of evidence behind it -- cm_cnt's rate is 1/(t+2), so t is
  //  exactly how many observations the prior is being given the weight of.
  static void seed(cm_cnt &c, int p) {
    c.p = (u16)(tc_clamp(p, 1, CM_PONE - 1) << (CM_CBITS - CM_PBITS));
    c.t = PS_PRIOR;
  }
  //  What the run model would know if it had read the quantizer.  A run in a
  //  class of rare-bit probability q is geometric in q, so every "is the run
  //  over here" decision is q before any evidence at all, and a byte is 255
  //  exactly when the run gets past 255.  The escape tree is seeded from the
  //  same geometric, truncated to the range the escape covers.
  //
  //  It matters most where there is least to learn from: on a short stream
  //  the run model never warms up, and a cold counter at one in two is a
  //  poor guess for a class that is right 4095 times in 4096.
  void prime(void) {
    int m, i, g;
    if(!PS_PRIOR)
      return;
    for(m = 0; m < PS_MAGN; m++) {
      double q = ps_q[m] / (double)CM_PONE;
      seed(ff[m], (int)(CM_PONE * (1.0 - pow(1.0 - q, 255.0)) + 0.5));
      for(i = 0; i < PS_UNARY; i++)
        seed(un[m][i], (int)(CM_PONE * (1.0 - q) + 0.5));
    }
    for(g = 0; g < PS_NTREE; g++) {
      //  The magnitude in the middle of the group, and the mass the escape
      //  can still hold, summed downward so a node is a ratio of two sums.
      double q = ps_q[(2 * g + 1) * PS_MAGN / (2 * PS_NTREE)] / (double)CM_PONE;
      int top = (2 * g + 1) * PS_MAGN / (2 * PS_NTREE) < PS_FFMAG ? 255 : 256;
      double w[257];
      int b, z;
      w[256] = 0;
      for(i = 255; i >= 0; i--)
        w[i] = w[i + 1] + (i >= PS_UNARY && i < top ? pow(1.0 - q, (double)i) * q : 0.0);
      for(b = 7; b >= 0; b--)
        for(z = 0; z < 256; z += 1 << (b + 1)) {
          double lo = w[z] - w[z + (1 << b)], hi = w[z + (1 << b)] - w[z + (1 << (b + 1))];
          if(lo + hi > 0)
            seed(tr[g][(256 + z) >> (b + 1)], (int)(CM_PONE * lo / (lo + hi) + 0.5));
        }
    }
  }
  //  Everything the coder learns, back to where a fresh process has it.
  //  Called where the model's own tables are cleared, and not between the
  //  streams of a solid run, which keep what the ones before them learned.
  //  Everything but the mapping goes, by size rather than by name: a
  //  counter array added to this struct and forgotten here would leave the
  //  second stream of a run coded against the first one's statistics, which
  //  a round trip in one process cannot see and a fresh decoder cannot read.
  void reset(void) {
    u8 *b = buf, *e = bend;
    memset((void *)this, 0, sizeof *this);
    buf = b;
    bend = e;
    ps_tables();
    prime();
    primed = 1;
  }
  //  A stream starts: no block in hand, and the buffer mapped if this is
  //  the first one.  The map is demand-zero and nothing outside the block
  //  in hand is ever touched, so a short file pays for a few pages of it.
  void begin(void) {
    ps_tables();
    //  Kept on this object rather than read off the shared table flag: the
    //  second direction to run in a process finds the tables already built,
    //  and a stream that came in as solid was never reset, so neither is
    //  evidence that THIS object has its priors.  Priming zeroed counters is
    //  what is wanted; priming over a solid run's learned ones is not.
    if(!primed) {
      prime();
      primed = 1;
    }
    if(!buf) {
      buf = (u8 *)tc_map(ENC ? PS_ENCCAP : PS_DECCAP);
      bend = buf + (ENC ? PS_TMPOFF : PS_EXPCAP);
    }
    qn = 0;
    left = 0;
  }
  //  The block in the pool, coded: its length, its counts, and its runs in
  //  class order.
  NOINLINE void flush(void) {
    u32 count[PS_NCLS];
    u32 n = qn, i, k, o;
    qn = 0;
    if(!n)
      return;
    ps_bitsort(buf, (u32 *)(buf + PS_TMPOFF), n, count);
    for(k = 0, o = 0; k < PS_NCLS; k++)
      o += count[k];
    //  freq() below codes the total against this same bound and would
    //  quietly clamp it rather than overflow, so a block that broke the
    //  bound would desync the decoder with nothing to show for it.
    FATAL_UNLESS(o <= n + 31 * PS_NCLS, "internal: a block of %u model bits made %u run bytes", n, o);
    len(n);
    freq(count, PS_NCLS, n + 31 * PS_NCLS, 0, 0, 0);
    for(k = 0, o = 0; k < PS_NCLS; k++)
      for(i = 0; i < count[k]; i++, o++)
        rbyte(k, buf[o]);
  }
  //  And back: one block read and expanded into the cursors the model then
  //  reads its bits through.  Class k's chunk is runs of (k & 1) ^ 1 broken
  //  by (k & 1) -- the low bit of the class is the mirror, so what comes
  //  out is the bit the model coded and not the folded one.
  NOINLINE void load(void) {
    u32 count[PS_NCLS];
    u32 n, i, k;
    u8 *q = buf;
    n = len(0);
    FATAL_UNLESS(n >= 1 && n <= PS_BLOCK, "coded stream: a block of %u model bits", n);
    freq(count, PS_NCLS, n + 31 * PS_NCLS, 0, 0, 0);
    for(k = 0; k < PS_NCLS; k++) {
      u32 bit0 = k & 1, bit1 = bit0 ^ 1;
      ptr[k] = q;
      for(i = 0; i < count[k]; i++) {
        u32 c = rbyte(k, 0);
        FATAL_UNLESS(q + c + 1 <= bend, "coded stream: a block expands past the %" PRIu64 " bytes it can hold", (u64)PS_EXPCAP);
        memset(q, (int)bit1, c);
        q += c;
        if(c < 255)
          *q++ = (u8)bit0;
      }
      FATAL_UNLESS(q + PS_PAD <= bend, "coded stream: a block expands past the %" PRIu64 " bytes it can hold", (u64)PS_EXPCAP);
      memset(q, (int)bit1, PS_PAD);
      q += PS_PAD;
    }
    left = n;
  }
  //  One model bit.  Encoding, it is a byte in the pool and nothing else
  //  until the pool fills; decoding, it is a byte read from its class.
  INLINE u32 bit(u32 p, u32 b) {
    if(ENC) {
#ifdef OC_PSRC_STATS
      { u32 k = ps_qr[p]; if(!ps_stats_armed) ps_stats_armed = 1, atexit(ps_stats_dump); ps_cls_n[k]++; ps_cls_u[k] += (((u32)ps_qs[p] >> 7) ^ b) ^ 1; }
#endif
      buf[qn++] = (u8)(ps_qs[p] ^ (b << 7));
      if(UNLIKELY(qn == PS_BLOCK))
        flush();
      return b;
    }
    if(UNLIKELY(!left))
      load();
    left--;
    return *ptr[ps_qr[p]]++;
  }
};
template <int f_DEC> oc_psrc<f_DEC> oc_psrc<f_DEC>::inst;
#endif
