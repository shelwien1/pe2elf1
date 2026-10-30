// fam.inc -- a model family (h264p's tc_fam, from oggcomp, balrogg-P-hash tc_fam.inc): the
// integer coder every IDX family is. A value is a cascade of binary decisions -- zero? one? two?
// then the length of the rest in unary and its bits below the top one -- and each decision is
// predicted from up to ten counters (tables A-E, H-L under as many contexts), an APM, three
// mixers and a second APM, all indexed by the decision's node in the cascade; a level's families
// may have fewer (four counters and one mixer at -1: the shape is the family's class's,
// famcfg.inc). Decisions outside the cascade use bit() with nodes of their own, and the sign of a
// value has its own counters (G). prp, when set, is one more prediction for the mixer; hint, a
// magnitude the value is expected near, one more set of counters.
//
// Changed from h264p's: the bits go through m4vpack's range coder (Coder, coder.inc), a value
// too long to be one marks the coder's error (damaged data: the walk fails its chunk) instead of
// jumping to a trap, and what can only be a bug ends the program (fatal()).

//  A value's cascade: the values below TC_SMALL decided one by one (zero? one? two?), then the
//  rest's length in unary (its first TC_ULEN decisions by their own nodes, the rest by one more),
//  then its bits below the top one; the nodes of the first two parts
constexpr int TC_SMALL = 3, TC_ULEN = 9;
constexpr int TC_NODE = TC_SMALL + TC_ULEN + 1;
//  the mantissa's nodes: its lengths (TC_MLEN, the longer as the last), its tree over its top bits
//  (TC_MTREE deep), its other bits by place (TC_MPOS, the later as the last)
constexpr int TC_MLEN = 13, TC_MTREE = 6, TC_MPOS = 13;
constexpr int TC_MPER = (1 << TC_MTREE) - 1 + TC_MPOS;
constexpr int TC_MNODE = TC_MLEN * TC_MPER;
constexpr int TC_NBMAX = 62;
constexpr sz TC_VOL_MAX = (sz)1 << 31;  // a table's rows (times its nodes), at most
constexpr int TC_NC_MAX = 10;           // a family's counters, at most
//  the hint's states (hs): none, it decides 0, it decides 1, the value is above it, below it
enum { HS_NONE, HS_AT0, HS_AT1, HS_ABOVE, HS_BELOW, HS_N };
//  the mixers' constant input, the bias
constexpr int TC_BIAS = 256;
//  the final probability: the mixing's share of it against APM 2's, in TC_BW_ONE-ths (bw)
constexpr int TC_BW_BITS = 4, TC_BW_ONE = 1 << TC_BW_BITS;
//  the final mixer's inputs (the three mixers', the bias), the mantissa mixer's (counter T's, the
//  hint's counter's, the bias)
constexpr int TC_FIN = 4, TC_MIN = 3;
//  the mixers' inputs besides the counters' (A's APM refinement, the outside prediction, the bias,
//  the hint's counter), all of them padded to TC_LANES (AVX2's 32-bit lanes)
constexpr int TC_XIN = 4, TC_LANES = 8;
constexpr int tc_nin(int nc) { return (2 * nc + TC_XIN + TC_LANES - 1) / TC_LANES * TC_LANES; }
//  the .idx parameters' ranges: a counter's target margin (in TC_MW_UNIT-ths of CM_CONE, to
//  TC_MW_MAX), a mixer's learning rate (1 to TC_LR_MAX) and boost (to TC_MB_MAX)
constexpr int TC_MW_UNIT = 64, TC_MW_MAX = CM_CONE / 4, TC_LR_MAX = 64, TC_MB_MAX = 255;
//  AVX2: the counters a vector holds (a 64-bit lane each), the counters' lanes padded to vectors
constexpr int TC_CPV = 4;
constexpr int TC_NC_PAD = (TC_NC_MAX + TC_CPV - 1) / TC_CPV * TC_CPV;
static INLINE int tc_node_len(int k) { return TC_SMALL + (k < TC_ULEN ? k : TC_ULEN); }
//  A mantissa bit's node: by the length, and for the top mt bits (up to
//  TC_MTREE) by the bits above them (pref, a 1 and those), the rest by
//  position
static INLINE int tc_node_man(int nb, int p, u64 pref, int mt) {
  return (nb > TC_MLEN - 1 ? TC_MLEN - 1 : nb) * TC_MPER +
         (p < mt ? (int)pref - 1 : (1 << TC_MTREE) - 1 + (p > TC_MPOS - 1 ? TC_MPOS - 1 : p));
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  if(!(vol > 0 && vol < TC_VOL_MAX && nodes > 0))
    fatal("an IDX family gives the %s an index of %llu rows; a Volume must stay under 2^31", what, (unsigned long long)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  if(!(vol * mult < TC_VOL_MAX))
    fatal("an IDX family gives the %s %llu rows of %llu; the product must stay under 2^31", what, (unsigned long long)vol,
          (unsigned long long)mult);
}
//  The context indices of one value: what idx2inc.pl's MakeIndex builds
//  (c: the counters', A-E, H-L in turn)
struct tcx {
  int c[TC_NC_MAX];
  int s, f, m, w, t, n, g;
};
//  A family's tables, in the order tc_fam takes them (its class's
//  tables(), famcfg.inc): those its shape has (the counters' of its
//  class's type, C::Cnt)
template <class Cnt>
struct tc_tabs {
  Cnt *cnt[TC_NC_MAX];
  cm_cnt *T, *G;
  i32 *W, *WM, *W2, *W3, *WF;
  u8 *WC, *WMC, *W2C, *W3C, *WFC;
  i16 *S, *F;
  u8 *SC, *FC;
  void Init() { memset((void *)this, 0, sizeof *this); }  // (none)
};

//  One bit through the coder of the run (coder.inc), p the probability of a 0 (12 bits)
static INLINE int tc_bit(Coder *rc, int p, int bit) { return code_bit(rc, (U32)(CM_PONE - p) << (CO_PBITS - CM_PBITS), bit); }

//  The dump (IDX/famtune.cpp): what each family is asked to code as the
//  compressor runs, written by a build with M4VPACK_DUMP to
//  $M4VPACK_DUMP/<family>.dump -- R where a stream starts, then the
//  variables of each value (M, at make()), each select() (S), and each
//  decision with what reaches it from outside the family: the outside
//  prediction and the hint's state (B a decision of bit_x(); m a mantissa
//  bit, by its place in the number, so that the tree's depth mt can
//  change; G a sign). None of it depends on the family's parameters, so
//  that famtune codes one dump again under each of them. A record is its
//  letter and its numbers, each a zigzag varint (a byte for those within
//  -64..63), in the order of tc_dump's calls below.
constexpr int TC_VBITS = BYTE_BITS - 1;   // a varint's bits per byte,
constexpr int TC_VMORE = 1 << TC_VBITS;   // and its flag of more to come
static INLINE void tc_dump_put(FILE *f, i64 v) {
  u64 u = ((u64)v << 1) ^ (u64)(v >> (U64_BITS - 1));
  for(; u >= TC_VMORE; u >>= TC_VBITS)
    putc((int)(u & (TC_VMORE - 1)) | TC_VMORE, f);
  putc((int)u, f);
}
static INLINE i64 tc_dump_get(const char *&p, const char *e) {
  u64 u = 0;
  for(int s = 0;; s += TC_VBITS) {
    if(UNLIKELY(p == e || s > U64_BITS - 1))
      fatal("a dump cut short");
    u8 c = (u8)*p++;
    u |= (u64)(c & (TC_VMORE - 1)) << s;
    if(!(c & TC_VMORE))
      break;
  }
  return (i64)(u >> 1) ^ -(i64)(u & 1);
}
#ifdef M4VPACK_DUMP
//  (each family's own, for each model: R as its model starts; none for a
//  decompressor's, fam null)
struct tc_dump {
  FILE *f;
  long start;  // where this model's records begin
  void Init(const char *fam) {
    f = nullptr;
    start = 0;
    const char *dir = getenv("M4VPACK_DUMP");
    if(!fam || !dir || !*dir)
      return;
    size_t n = strlen(dir) + strlen(fam) + sizeof "/.dump";
    char *fn = (char *)xmalloc(n);
    snprintf(fn, n, "%s/%s.dump", dir, fam);
    f = fopen(fn, "ab");
    if(!f)
      fatal("cannot write %s", fn);
    free(fn);
    fseek(f, 0, SEEK_END);
    start = ftell(f);
    putc('R', f);
  }
  void Quit() {
    if(f)
      fclose(f);
    f = nullptr;
  }
  //  a model whose compression starts over (kRestart): its records go
  void discard() {
    if(!f)
      return;
    fflush(f);
#ifdef _WIN32
    _chsize_s(_fileno(f), start);
#else
    if(ftruncate(fileno(f), start))
      fatal("cannot cut a dump short");
#endif
    putc('R', f);  // (appended: at the end, where it was cut)
  }
  template <class... A> void rec(char tag, A... a) {
    if(!f)
      return;
    putc(tag, f);
    int unused[] = {0, (tc_dump_put(f, (i64)a), 0)...};
    (void)unused;
  }
  void vars(const int *v, int n) {  // M
    if(!f)
      return;
    putc('M', f);
    for(int i = 0; i < n; i++)
      tc_dump_put(f, v[i]);
  }
};
#define TC_DUMP(x)                                                                                                     \
  do {                                                                                                                 \
    if(ENC)                                                                                                            \
      dump_.x;                                                                                                         \
  } while(0)
#else
#define TC_DUMP(x)                                                                                                     \
  do {                                                                                                                 \
  } while(0)
#endif

//  A counter's probabilities stretched: looked up (cm_cnt2), or as the
//  counter keeps them (cm_cnt2s)
static INLINE int tc_sp(const cm_cnt2 &c) { return cm_stretch(c.P()); }
static INLINE int tc_sq(const cm_cnt2 &c) { return cm_stretch(c.Q()); }
static INLINE int tc_sp(const cm_cnt2s &c) { return c.sp; }
static INLINE int tc_sq(const cm_cnt2s &c) { return c.sq; }
#ifdef M4V_AVX2
//  The same of four counters at once (four false: two, the upper lanes 0),
//  as the 32-bit lanes P and Q of each in turn (v: P and Q so)
constexpr int TC_I16_BITS = BYTE_BITS * sizeof(i16);  // (a 32-bit lane's upper half)
constexpr int TC_DUP_EVEN = _MM_SHUFFLE(2, 2, 0, 0);    // (each 64-bit lane's low 32 bits, twice)
constexpr int TC_ODD32 = 0xAA;                         // (a blend's odd 32-bit lanes)
static INLINE __m256i tc_st4(cm_cnt2 *const *, bool, __m256i v) {
  return _mm256_srai_epi32(_mm256_slli_epi32(_mm256_i32gather_epi32((const int *)cm_st, v, sizeof(i16)), TC_I16_BITS),
                           TC_I16_BITS);
}
static INLINE __m256i tc_st4(cm_cnt2s *const *c, bool four, __m256i) {
  auto ld = [&](int k) {  // (sp and sq)
    int v;
    static_assert(sizeof v == 2 * sizeof(i16), "tc_st4: two 16-bit lanes in a 32-bit one");
    memcpy(&v, &c[k]->sp, sizeof v);
    return v;
  };
  return _mm256_cvtepi16_epi32(four ? _mm_setr_epi32(ld(0), ld(1), ld(2), ld(3)) : _mm_setr_epi32(ld(0), ld(1), 0, 0));
}
//  At their update, what a counter's 8-byte store writes above its q, in
//  the odd lanes' upper halves: the next counter's p as it was (cm_cnt2,
//  from cv, the counters as read), or its new P stretched (cm_cnt2s, from
//  nv, the new p and q, and t1, the new counts; sn: P and Q stretched)
static INLINE __m256i tc_upper(cm_cnt2 *const *, __m256i cv, __m256i, __m256i, __m256i &) {
  return _mm256_andnot_si256(_mm256_set1_epi32(UINT16_MAX), cv);
}
static INLINE __m256i tc_upper(cm_cnt2s *const *, __m256i, __m256i nv, __m256i t1, __m256i &sn) {
  const __m256i m16 = _mm256_set1_epi32(UINT16_MAX);
  __m256i v = _mm256_max_epi32(_mm256_srli_epi32(_mm256_and_si256(nv, m16), CM_CBITS - CM_PBITS), _mm256_set1_epi32(1));
  v = _mm256_blendv_epi8(v, _mm256_set1_epi32(CM_PHALF), _mm256_cmpeq_epi32(t1, _mm256_setzero_si256()));
  sn = _mm256_and_si256(_mm256_i32gather_epi32((const int *)cm_st, v, sizeof(i16)), m16);
  return _mm256_slli_epi64(_mm256_and_si256(sn, _mm256_set1_epi64x(UINT16_MAX)), 3 * TC_I16_BITS);  // (the top lane)
}
//  and cm_cnt2s's Q stretched, after the store (the 16-bit lanes of the counters' Qs)
static INLINE void tc_put_sq(cm_cnt2 *const *, bool, __m256i) {}
static INLINE void tc_put_sq(cm_cnt2s *const *c, bool four, __m256i sn) {
  c[0]->sq = (i16)_mm256_extract_epi16(sn, 2);
  c[1]->sq = (i16)_mm256_extract_epi16(sn, 6);
  if(four) {
    c[2]->sq = (i16)_mm256_extract_epi16(sn, 10);
    c[3]->sq = (i16)_mm256_extract_epi16(sn, 14);
  }
}
#endif
//  counter A's P stretched, as the mixers see it without APMs (p1 = its P)
static INLINE int tc_x20(const cm_cnt2 *, int p1) { return cm_stretch(p1); }
static INLINE int tc_x20(const cm_cnt2s *c, int) { return c->sp; }

//  A family of a level: C is its configuration class (famcfg.inc), whose
//  constants (the shape, the rates, which indices are hashed) shape the
//  code here and, in the shipping build, are folded into it
template <int f_DEC, class C> struct tc_fam {
  static constexpr int ENC = (f_DEC == 0);
  static const int NC = C::NC, NIN = C::NIN, ND = C::ND, MND = C::MND;
  static_assert(NC % 2 == 0 && NC >= 2 && NC <= TC_NC_MAX, "tc_fam: an even count of counters, up to 10");
  static_assert(C::NMIX == 1 || C::NMIX == TC_FIN - 1, "tc_fam: one mixer, or three and the final one");
  static_assert(C::NAPM == 0 || C::NAPM == 2, "tc_fam: no APM, or two");
  typedef typename C::Cnt Cnt;  // cm_cnt2, or cm_cnt2s
  Coder *rc;  // the coder of the run it belongs to
#ifdef M4VPACK_DUMP
  tc_dump dump_;
#endif
  Cnt *cnt[NC];  // the counters' tables: A-E, H-L, the first NC
  cm_cnt *G, *T;
  cm_apm ap, ap2;                  // (with APMs)
  cm_mix<NIN> mx, mx2, mx3;        // (mx2, mx3: three mixers)
  cm_mix<TC_FIN> mxf;              // (three mixers)
  cm_mix<TC_MIN> mxm;
  int prp;          // an outside prediction of P(0) for the mixers, -1 for none
  const int *prpa;  // or one per node
  //  A magnitude code() expects the value near, -1 none: each decision
  //  then also sees the hint's own decision there while the value agrees
  //  with it so far, or which side of it the value went (hs: HS_*),
  //  through counters of its own by the t index and node (Z; ZM for the
  //  mantissa).
  i64 hint;
  int hs;
  u32 bz;
  Buf<cm_cnt2> Z;
  Buf<cm_cnt> ZM;
  sz cb[NC];  // the rows of the counters' contexts (select())
  u32 bs, bf, bm, bw2, bt, bn, bg;
  //  the counters' limits and targets (bit 0, 1) as 32-bit lanes, two each,
  //  for TC_NC_PAD of them (cn_v: their arrays in this order in memory,
  //  coded at once)
  int cn_lim[2 * TC_NC_PAD], cn_tg[2][2 * TC_NC_PAD];
  bool cn_v;
  //  (Init(), then wire() to its tables as the model starts; on zeroed
  //  memory: what it sets not, as it codes, is 0)
  void Init() {
    Z.Init();
    ZM.Init();
    cn_v = false;
#ifdef M4VPACK_DUMP
    dump_.Init(ENC ? C::name() : nullptr);
#endif
  }
  void Quit() {
    Z.Quit();
    ZM.Quit();
#ifdef M4VPACK_DUMP
    dump_.Quit();
#endif
  }
  void wire(typename C::T *t, Coder *r) {
    rc = r;
    tc_tabs<Cnt> b;
    b.Init();
    C::tables(t, b);
    for(int k = 0; k < NC; k++)
      cnt[k] = b.cnt[k];
    T = b.T;
    G = b.G;
    {
      static const char *const name[TC_NC_MAX] = {"counter A", "counter B", "counter C", "counter D", "counter E",
                                                  "counter H", "counter I", "counter J", "counter K", "counter L"};
      for(int k = 0; k < NC; k++)
        tc_vfits(name[k], C::kc[k], ND);
      // (and their rows times nodes: bt and bg, below, are 32-bit products)
      tc_ifits("counter T", C::kT, MND);
      tc_ifits("counter G", C::kG, C::NG);
      tc_ifits("mixer", C::kM, (sz)ND * NIN);
      tc_ifits("mantissa mixer", C::kN, (sz)MND * TC_MIN);
      mx.init(b.W, b.WC);
      if(C::NMIX > 1) {
        tc_ifits("mixer 2", C::kW, (sz)ND * NIN);
        tc_ifits("mixer 3", C::kS, (sz)ND * NIN);
        mx2.init(b.W2, b.W2C);
        mx3.init(b.W3, b.W3C);
        mxf.init(b.WF, b.WFC);
      }
      if(C::NAPM) {
        tc_ifits("APM 1", C::kS, (sz)ND * C::qs);
        tc_ifits("APM 2", C::kF, (sz)ND * C::qf);
        ap.init(b.S, b.SC, C::qs);
        ap2.init(b.F, b.FC, C::qf);
      }
      mxm.init(b.WM, b.WMC);
    }
    //  (the inputs past the last are 0 for good)
    for(int i = 0; i < NIN; i++)
      mx.x[i] = 0;
    cn_v = true;
    for(int k = 0; k < TC_NC_PAD; k++)
      for(int i = 0; i < 2; i++) {
        cn_lim[2 * k + i] = k < NC ? C::lim[k] : 0;
        cn_tg[0][2 * k + i] = k < NC ? CM_CONE - 1 - C::tg[k] : 0;
        cn_tg[1][2 * k + i] = k < NC ? C::tg[k] : 0;
      }
    for(int k = 0; k + 1 < NC; k++)
      cn_v &= cnt[k] < cnt[k + 1];
    prp = -1;
    prpa = nullptr;
    hint = -1;
    hs = 0;
  }
  //  The coding calls of the model: make() a value's indices from its
  //  variables (C::make; a dump records them), select() the rows they
  //  pick, then code it (code(), codes(), bit(), sign())
  INLINE void make(const typename C::V &D, tcx &X) {
    TC_DUMP(vars(reinterpret_cast<const int *>(&D), sizeof D / sizeof(int)));
    C::make(D, X);
  }
  INLINE void select(const tcx &x) {
    TC_DUMP(rec('S'));
    for(int k = 0; k < NC; k++)
      cb[k] = (sz)(C::hc[k] ? tc_hrow((u32)x.c[k], C::hc[k]) : (u32)x.c[k]) * ND;
    if(C::NMIX > 1 || C::NAPM)
      bs = (C::hS ? tc_hrow((u32)x.s, C::hS) : (u32)x.s) * ND;
    if(C::NAPM)
      bf = (C::hF ? tc_hrow((u32)x.f, C::hF) : (u32)x.f) * ND;
    bm = (C::hM ? tc_hrow((u32)x.m, C::hM) : (u32)x.m) * ND;
    if(C::NMIX > 1)
      bw2 = (u32)x.w * ND;
    bz = C::hT ? tc_hrow((u32)x.t, C::hT) : (u32)x.t;
    bt = bz * MND;
    bn = (C::hN ? tc_hrow((u32)x.n, C::hN) : (u32)x.n) * MND;
    bg = (u32)x.g;
    fetch(0);
  }
  INLINE int bit(int node, int b) {
    if(UNLIKELY((unsigned)node >= (unsigned)ND))
      fatal("internal: decision node %d of a family of %d", node, ND);
    return bit_x(node, b);
  }
  void dump_discard() { TC_DUMP(discard()); }
  i64 codes(const tcx &v, i64 x, u32 sc) {
    i64 m = code(v, ENC ? (x < 0 ? -x : x) : 0);
    if(!m)
      return 0;
    return sign(sc, x < 0) ? -m : m;
  }
  //  The sign of a nonzero value, under the g index of the last select()
  //  and sign context sc: 1 negative.
  int sign(u32 sc, int s) {
    if(UNLIKELY(sc >= (u32)C::NG))
      fatal("internal: sign context %u of a family of %d", sc, C::NG);
    TC_DUMP(rec('G', sc, s));
    cm_cnt g = G[bg * (u32)C::NG + sc];
    s = tc_bit(rc, g.P(), ENC && s);
    g.upd(s, C::rG, C::mwG);
    G[bg * (u32)C::NG + sc] = g;
    return s;
  }
  //  The rows a decision of the node reads, on their way into the cache
  //  (for the first node from select(), for the next while one is coded)
  INLINE void fetch(int node) {
    for(int k = 0; k < NC; k++)
      CM_PREFETCH(cnt[k] + cb[k] + node);
    if(C::NAPM) {
      CM_PREFETCH(ap.t + (sz)(bs + node) * (u32)ap.q);
      CM_PREFETCH(ap.c + (sz)(bs + node) * (u32)ap.q);
      if(C::bw < TC_BW_ONE) {
        CM_PREFETCH(ap2.t + (sz)(bf + node) * (u32)ap2.q);
        CM_PREFETCH(ap2.c + (sz)(bf + node) * (u32)ap2.q);
      }
    }
    const i32 *w1 = mx.w + (sz)(bm + node) * NIN;
    CM_PREFETCH(w1);
    CM_PREFETCH(w1 + NIN - 1);
    if(C::NMIX > 1) {
      const i32 *w2 = mx2.w + (sz)(bw2 + node) * NIN, *w3 = mx3.w + (sz)(bs + node) * NIN;
      CM_PREFETCH(w2);
      CM_PREFETCH(w2 + NIN - 1);
      CM_PREFETCH(w3);
      CM_PREFETCH(w3 + NIN - 1);
    }
    CM_PREFETCH(mx.c + bm + node);
    if(C::NMIX > 1) {
      CM_PREFETCH(mx2.c + bw2 + node);
      CM_PREFETCH(mx3.c + bs + node);
    }
  }
  //  (No -Wmaybe-uninitialized here: cnt_inputs sets the counters' vectors
  //  and cnt_update reads them, both where cn_v, but GCC at -O3 with AVX2
  //  cannot tell that the coding in between leaves cn_v as it was. Reading
  //  cn_v once instead, so that it could, measured slower in h264p.)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
  //  A decision's counters (bit_x): their rows (cp), and their 64-bit lanes
  //  in vectors of four (cv) with their probabilities stretched (st), where
  //  cn_v, else copies of them (cn)
  struct CntRows {
    Cnt *cp[NC];
    Cnt cn[NC];
#ifdef M4V_AVX2
    __m256i cv[(NC + TC_CPV - 1) / TC_CPV], st[(NC + TC_CPV - 1) / TC_CPV];
#endif
  };
  INLINE int bit_x(int node, int b) {
    TC_DUMP(rec('B', node, b, prpa ? prpa[node] : prp, hs));
    if(node + 1 < ND)
      fetch(node + 1);
    CntRows r;
    for(int k = 0; k < NC; k++)
      r.cp[k] = cnt[k] + cb[k] + node;
    //  The mixers' inputs: the counters' two probabilities each (in the
    //  order of the arrays), the APM's refinement of A's (A's own without
    //  APMs), the outside prediction, the bias and the hint's counter
    //  (stretched), then zeros to a multiple of 8; the mixers' are the same,
    //  packed once
    int pa = cnt_inputs(r);
    int p1 = C::NAPM ? ap.pp(pa, bs + (u32)node) : pa;
    if(p1 > CM_PONE - 1)
      p1 = CM_PONE - 1;
    int pp = prpa ? prpa[node] : prp;
    cm_cnt2 *zc = hs ? &Z[((sz)bz * ND + (u32)node) * HS_N + (u32)hs] : nullptr;
    const int x20 = C::NAPM ? cm_stretch(p1) : tc_x20(r.cp[0], p1), x21 = pp >= 0 ? cm_stretch(pp) : 0,
              x23 = zc ? cm_stretch(zc->P()) : 0;
    cm_x16<NIN> xp;
    mix_inputs(r, x20, x21, x23, xp);
    b = mix_code(node, b, xp);
    cnt_update(r, b);
    if(zc)
      zc->upd(b, C::rT, C::mwT);
    return b;
  }
  //  the counters' probabilities, as the mixers' inputs (where cn_v, in
  //  r.st); A's (the first counter's) P
  INLINE int cnt_inputs(CntRows &r) {
    int pa;
    constexpr int NV = (NC + TC_CPV - 1) / TC_CPV;  // the counters' vectors: four in each, the last two or four
#ifdef M4V_AVX2
    //  (cn_v: the counters all at once, as 64-bit lanes p, t, q and the next
    //  counter's p, cm_cnt2s's own P stretched, four to a vector; their
    //  probabilities as 32-bit lanes, P and Q of each in turn)
    if(cn_v) {
      auto ld2 = [&](int k) {
        return _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)r.cp[k]), _mm_loadl_epi64((const __m128i *)r.cp[k + 1]));
      };
      constexpr int V = TC_CPV;
      for(int j = 0; j < NV; j++)
        r.cv[j] = V * j + 2 < NC ? _mm256_inserti128_si256(_mm256_castsi128_si256(ld2(V * j)), ld2(V * j + 2), 1)
                               : _mm256_inserti128_si256(_mm256_setzero_si256(), ld2(V * j), 0);
      const __m256i m16 = _mm256_set1_epi32(UINT16_MAX), z = _mm256_setzero_si256();
      for(int j = 0; j < NV; j++) {
        // (cm_cnt2::P and Q, then cm_stretch, or as cm_cnt2s keeps them; t: the counts)
        __m256i t = _mm256_srli_epi32(_mm256_shuffle_epi32(r.cv[j], TC_DUP_EVEN), TC_I16_BITS);
        __m256i v = _mm256_max_epi32(_mm256_srli_epi32(_mm256_and_si256(r.cv[j], m16), CM_CBITS - CM_PBITS), _mm256_set1_epi32(1));
        v = _mm256_blendv_epi8(v, _mm256_set1_epi32(CM_PHALF), _mm256_cmpeq_epi32(t, z));
        if(!j)
          pa = _mm256_cvtsi256_si32(v);
        r.st[j] = tc_st4(r.cp + V * j, V * j + 2 < NC, v);
      }
    } else
#endif
    {
      for(int k = 0; k < NC; k++) {
        r.cn[k] = *r.cp[k];
        mx.set(2 * k, tc_sp(r.cn[k]));
        mx.set(2 * k + 1, tc_sq(r.cn[k]));
      }
      pa = r.cn[0].P();
    }
    (void)NV;
    return pa;
  }
  //  the mixers' inputs, the counters' and the four more, packed
  INLINE void mix_inputs(const CntRows &r, int x20, int x21, int x23, cm_x16<NIN> &xp) {
    (void)r;
#ifdef M4V_AVX2
    if(cn_v) {
      //  the 32-bit lanes in vectors of 8: the counters', then the four
      //  more (after the last two counters' where their count is not a
      //  multiple of 4), then zeros
      constexpr int NZ = NIN / CM_V32, V = TC_CPV;
      __m256i zz[NZ];
      const __m128i ex = _mm_setr_epi32(x20, x21, TC_BIAS, x23);
      for(int j = 0; j < NC / V; j++)
        zz[j] = r.st[j];
      if(NC % V)
        zz[NC / V] = _mm256_inserti128_si256(r.st[NC / V], ex, 1);
      else
        zz[NC / V] = _mm256_inserti128_si256(_mm256_setzero_si256(), ex, 0);
      for(int j = NC / V + 1; j < NZ; j++)
        zz[j] = _mm256_setzero_si256();
      for(int i = 0; i + CM_V16 <= NIN; i += CM_V16)
        xp.v[i / CM_V16] = _mm256_packs_epi32(zz[i / CM_V32], zz[i / CM_V32 + 1]);
      if(NIN % CM_V16)
        xp.t = _mm_packs_epi32(_mm256_castsi256_si128(zz[NZ - 1]), _mm256_extracti128_si256(zz[NZ - 1], 1));
    } else
#endif
    {
      mx.set(2 * NC, x20);
      mx.set(2 * NC + 1, x21);
      mx.set(2 * NC + 2, TC_BIAS);
      mx.set(2 * NC + 3, x23);
      xp.pack(mx.x);
    }
  }
  //  the mixing, the bit coded with it, and the mixers and APMs trained
  INLINE int mix_code(int node, int b, const cm_x16<NIN> &xp) {
    (void)xp;
#ifdef M4V_AVX2
    const cm_x16<NIN> *xq = &xp;
#else
    const cm_x16<NIN> *xq = nullptr;
#endif
    int pm;
    if(C::NMIX > 1) {
      mxf.set(0, cm_stretch(mx.mix(bm + (u32)node, mx.x, xq)));
      mxf.set(1, cm_stretch(mx2.mix(bw2 + (u32)node, mx.x, xq)));
      mxf.set(2, cm_stretch(mx3.mix(bs + (u32)node, mx.x, xq)));
      mxf.set(3, TC_BIAS);
      pm = mxf.mix((u32)node * HS_N + (u32)hs);  // by node and hint state
    } else {
      pm = mx.mix(bm + (u32)node, mx.x, xq);
    }
    int pf = C::NAPM && C::bw < TC_BW_ONE
                 ? (C::bw * pm + (TC_BW_ONE - C::bw) * ap2.pp(pm, bf + (u32)node) + TC_BW_ONE / 2) >> TC_BW_BITS
                 : pm;
    b = tc_bit(rc, pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b);
    mx.upd(b, C::lr, C::mb, mx.x, xq);
    if(C::NMIX > 1) {
      mx2.upd(b, C::lr, C::mb, mx.x, xq);
      mx3.upd(b, C::lr, C::mb, mx.x, xq);
      mxf.upd(b, C::lr, C::mb);
    }
    //  (APM 2 with no share, bw TC_BW_ONE, is neither read nor trained:
    //  what it would give goes nowhere)
    if(C::NAPM) {
      ap.upd(b, C::rS1);
      if(C::bw < TC_BW_ONE)
        ap2.upd(b, C::rS2);
    }
    return b;
  }
  //  the counters, trained
  INLINE void cnt_update(CntRows &r, int b) {
    constexpr int NV = (NC + TC_CPV - 1) / TC_CPV;
#ifdef M4V_AVX2
    if(cn_v) {
      // as cm_cnt2::upd, each counter's q through its p's arithmetic with
      // the rate 8192 / 65536 (1 / 2^CM_FAST_SHIFT); written back in the
      // order of the arrays, each with the next counter's p as it was
      // (rewritten after, where the next counter is one of these), or
      // cm_cnt2s with its new P and Q stretched (tc_upper, tc_put_sq)
      constexpr int V = TC_CPV;
      const __m256i m16 = _mm256_set1_epi32(UINT16_MAX), z = _mm256_setzero_si256();
      const __m256i even = _mm256_setr_epi32(-1, 0, -1, 0, -1, 0, -1, 0);
      for(int j = 0; j < NV; j++) {
        __m256i t = _mm256_srli_epi32(_mm256_shuffle_epi32(r.cv[j], TC_DUP_EVEN), TC_I16_BITS);
        __m256i cur = _mm256_blendv_epi8(_mm256_and_si256(r.cv[j], m16), _mm256_set1_epi32(CM_CONE / 2), _mm256_cmpeq_epi32(t, z));
        __m256i t1 = _mm256_sub_epi32(t, _mm256_cmpgt_epi32(_mm256_loadu_si256((const __m256i *)(cn_lim + 2 * V * j)), t));
        __m256i rec = _mm256_and_si256(_mm256_mask_i32gather_epi32(_mm256_set1_epi32(CM_CONE >> CM_FAST_SHIFT), (const int *)cm_rec,
                                                                   t1, even, sizeof(u16)), m16);
        __m256i tg = _mm256_loadu_si256((const __m256i *)(cn_tg[b] + 2 * V * j));
        __m256i nv = _mm256_add_epi32(cur, _mm256_srai_epi32(_mm256_mullo_epi32(_mm256_sub_epi32(tg, cur), rec), CM_CBITS));
        __m256i sn = z;
        __m256i up = _mm256_blend_epi32(_mm256_slli_epi32(t1, TC_I16_BITS), tc_upper(r.cp + V * j, r.cv[j], nv, t1, sn),
                                        TC_ODD32);
        __m256i o = _mm256_or_si256(_mm256_and_si256(nv, m16), up);
        __m128i lo = _mm256_castsi256_si128(o);
        _mm_storel_epi64((__m128i *)r.cp[V * j], lo);
        _mm_storel_epi64((__m128i *)r.cp[V * j + 1], _mm_unpackhi_epi64(lo, lo));
        if(V * j + 2 < NC) {
          __m128i hi = _mm256_extracti128_si256(o, 1);
          _mm_storel_epi64((__m128i *)r.cp[V * j + 2], hi);
          _mm_storel_epi64((__m128i *)r.cp[V * j + 3], _mm_unpackhi_epi64(hi, hi));
        }
        tc_put_sq(r.cp + V * j, V * j + 2 < NC, sn);
      }
    } else
#endif
    {
      for(int k = 0; k < NC; k++) {
        r.cn[k].upd(b, C::lim[k], C::tg[k]);
        *r.cp[k] = r.cn[k];
      }
    }
    (void)NV;
  }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
  INLINE int bitm(int node, int b) {
    if(UNLIKELY((unsigned)node >= (unsigned)MND))
      fatal("internal: mantissa node %d of a family of %d", node, MND);
    cm_cnt t = T[bt + node];
    // (no outside prediction: the families that code() a magnitude give
    // one per node of its head, prpa, or none)
    if(hs) {
      // with the hint's counter, through the mantissa mixer the value
      // would not use otherwise
      cm_cnt &z = ZM[((sz)bz * MND + (u32)node) * HS_N + (u32)hs];
      mxm.set(0, cm_stretch(t.P()));
      mxm.set(1, cm_stretch(z.P()));
      mxm.set(2, TC_BIAS);
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit(rc, pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
      mxm.upd(b, C::lrm, C::mbm);
      t.upd(b, C::rT, C::mwT);
      T[bt + node] = t;
      z.upd(b, C::rT, C::mwT);
      return b;
    }
    b = tc_bit(rc, t.P(), b);
    t.upd(b, C::rT, C::mwT);
    T[bt + node] = t;
    return b;
  }
  i64 code(const tcx &v, i64 x) {
    static_assert(ND >= TC_NODE, "tc_fam: code() takes nodes up to TC_NODE - 1");
    int k, p, nb = 0, hst = 0, hnb = 0;
    u64 u, hu = 0;
    select(v);
    if(UNLIKELY(ENC && x < 0))
      fatal("internal: a negative magnitude");
    if(hint >= 0) {
      // hst: HS_AT0 the value agrees with the hint so far, HS_ABOVE it is above, HS_BELOW below
      hst = HS_AT0;
      if(Z.empty())
        Z.fill(C::kT * (sz)ND * HS_N, cm_cnt2{}), ZM.fill(C::kT * (sz)MND * HS_N, cm_cnt{});
      hu = hint >= TC_SMALL ? (u64)(hint - TC_SMALL) + 1 : 0;
      for(u64 t = hu; t; t >>= 1)
        hnb++;
    }
    for(k = 0; k < TC_SMALL; k++) {
      int hb = hint == k;
      hs = hst == HS_AT0 ? HS_AT0 + hb : hst;
      int b = bit_x(k, ENC && x == k);
      if(hst == HS_AT0 && b != hb)
        hst = b ? HS_BELOW : HS_ABOVE;
      if(b) {
        hs = HS_NONE;
        return k;
      }
    }
    u = ENC ? (u64)(x - TC_SMALL) + 1 : 1;
    if(ENC) {
      u64 t = u;
      while(t) {
        nb++;
        t >>= 1;
      }
    }
    for(k = 0;; k++) {
      int hb = hnb == k + 1;
      hs = hst == HS_AT0 ? HS_AT0 + hb : hst;
      int b = bit_x(tc_node_len(k), ENC && nb == k + 1);
      if(hst == HS_AT0 && b != hb)
        hst = b ? HS_BELOW : HS_ABOVE;
      if(b) {
        nb = k + 1;
        break;
      }
      if(UNLIKELY(k + 2 > TC_NBMAX)) {
        if(ENC)
          fatal("internal: a value of 2^%d or more", TC_NBMAX);
        rc->err = 1;  // (damaged data: the walk fails the chunk)
        nb = 1;
        break;
      }
    }
    {
      u64 pref = 1;
      for(p = 0; p < nb - 1; p++) {
        int hb = (int)(hu >> (nb - 2 - p) & 1);
        hs = hst == HS_AT0 ? HS_AT0 + hb : hst;
        TC_DUMP(rec('m', nb, p, (int)((u >> (nb - 2 - p)) & 1), hs, prp, prpa != nullptr, pref));
        int mb = bitm(tc_node_man(nb, p, pref, C::mt), ENC && ((u >> (nb - 2 - p)) & 1) != 0);
        if(hst == HS_AT0 && mb != hb)
          hst = mb > hb ? HS_ABOVE : HS_BELOW;
        pref = pref * 2 + (u64)mb;
      }
      if(!ENC)
        u = pref;
    }
    hs = HS_NONE;
    return (i64)(u - 1) + TC_SMALL;

  }
};
