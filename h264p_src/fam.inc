// A model family (oggcomp's tc_fam, balrogg-P-hash tc_fam.inc): the integer
// coder every IDX family is. A value is a cascade of binary decisions --
// zero? one? two? then the length of the rest in unary and its bits below
// the top one -- and each decision is predicted from ten counters (tables
// A-E, H-L under ten contexts), an APM, three mixers and a second APM, all
// indexed by the decision's node in the cascade. Decisions outside the
// cascade use bit() with nodes of their own, and the sign of a value has its
// own counters (G). prp, when set, is one more prediction for the mixer;
// hint, a magnitude the value is expected near, one more set of counters.
//
// Changed from oggcomp: no codebook-derived priors (tc_pcur), and errors
// are exceptions.

namespace h264p {

constexpr int TC_MIXN = 24;
constexpr int TC_NODE = 13;
constexpr int TC_MTREE = 6;
constexpr int TC_MPER = (1 << TC_MTREE) - 1 + 13;
constexpr int TC_MNODE = 13 * TC_MPER;
constexpr int TC_NBMAX = 62;
static INLINE int tc_node_len(int k) { return 3 + (k < 9 ? k : 9); }
//  A mantissa bit's node: by the length, and for the top mt bits (up to
//  TC_MTREE) by the bits above them (pref, a 1 and those), the rest by
//  position
static INLINE int tc_node_man(int nb, int p, u64 pref, int mt) {
  return (nb > 12 ? 12 : nb) * TC_MPER + (p < mt ? (int)pref - 1 : (1 << TC_MTREE) - 1 + (p > 12 ? 12 : p));
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  if(!(vol > 0 && vol < ((sz)1 << 31) && nodes > 0))
    cm_fail("an IDX family gives the %s an index of %" PRIu64 " rows; a Volume must stay under 2^31", what, (u64)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  if(!(vol * mult < ((sz)1 << 31)))
    cm_fail("an IDX family gives the %s %" PRIu64 " rows of %" PRIu64 "; the product must stay under 2^31", what, (u64)vol, (u64)mult);
}
//  Where the bits went, by stage (-v)
static double tc_bits[32];
static u64 tc_syms[32];
static int tc_stage, tc_verbose;
//  The context indices of one value: what idx2inc.pl's MakeIndex builds
struct tcx {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
};

//  The direction is a template parameter, f_DEC as in Rangecoder: 0
//  codes, 1 decodes.  One bit through the one coder there is,
//  Model<f_DEC>::rc -- a static member reached by type, so a family needs
//  no pointer to the model it belongs to.  The model is defined after the
//  families it holds; the name is dependent, so it is looked up when this
//  is instantiated, by which time it is there.
template <int f_DEC> struct Model;
template <int f_DEC> static INLINE int tc_bit(int p, int bit) {
  uint b = (uint)bit;
  if(!f_DEC && tc_verbose)
    tc_bits[tc_stage] -= log2(bit ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
  Model<f_DEC>::rc.rc_Process((uint)(p << 3), b);
  return (int)b;
}
#ifndef H264P_NO_THREADS
//  The compressor's coding calls, in order, for a thread of their own: the
//  model and the reconstruction run ahead of the families and the coder,
//  which take only what was recorded, so the stream is the same. (Not the
//  decompressor's: what it decodes steers its model.)
struct TcDefer {
  enum Op : uint8_t { SEL, BIT, CODE, CODES, SIGN, RAW, END };
  struct Rec {
    Op op;
    bool pa;    // prpa copied
    int node;   // the node; the sign context
    i64 val;    // the bit, value or byte
    int prp;
    i64 hint;
    void *obj;  // the family (RAW: the model)
    tcx x;
    int prpa[32];
  };
  static constexpr size_t N = 4096;  // records in the ring, a power of 2
  std::unique_ptr<Rec[]> ring{new Rec[N]};
  void (*exec)(Rec &) = nullptr;       // a record coded
  void (*pre)(const Rec &) = nullptr;  // the rows it will read, on their way into the cache
  std::function<void()> idle;          // the coder's thread between batches
  std::thread th;
  // (each side's own on lines of their own)
  alignas(64) std::atomic<size_t> head{0};  // recorded and published by the model
  alignas(64) std::atomic<size_t> tail{0};  // coded
  alignas(64) size_t h = 0, room = N;       // the model's next record, and how far it may go
  alignas(64) std::string err;              // what the coding failed with
  bool failed = false;
  // a side with nothing to do sleeps (after a moment's spinning) until the
  // other has work for it, enough to be worth the waking: the coder for
  // kWake records, the model for kWake records' room
  static constexpr size_t kWake = 1024;
  std::mutex mu;
  std::condition_variable cv;
  std::atomic<bool> csleep{false}, msleep{false}, fin{false}, quit{false};
  std::atomic<size_t> cfrom{0}, mfrom{0};  // where the coder sleeps, the model

  static void spin() {
    for(int i = 0; i < 16; i++)
#ifdef H264P_AVX2
      _mm_pause();
#else
      std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
  }
  void wake() {
    std::lock_guard<std::mutex> lk(mu);
    cv.notify_all();
  }
  INLINE Rec &next() {
    if(UNLIKELY(h >= room))
      wait_room();
    return ring[h & (N - 1)];
  }
  void wait_room() {
    publish();  // (the ring full: enough for the coder, should it sleep)
    for(int k = 0; k < 16; k++, spin())
      if(h < (room = tail.load() + N))
        return;
    std::unique_lock<std::mutex> lk(mu);
    mfrom = h;
    msleep = true;
    cv.wait(lk, [&] { return tail.load() + N >= h + kWake; });
    msleep = false;
    room = tail.load() + N;
  }
  INLINE void put() {
    if((++h & 31) == 0)
      publish();
  }
  void publish() {
    head.store(h);
    if(csleep.load() && h - cfrom.load() >= kWake)
      wake();
  }
  void done(size_t t) {
    tail.store(t);
    if(msleep.load() && t + N >= mfrom.load() + kWake)
      wake();
  }
  INLINE void sel(void *f, const tcx &x) {
    Rec &r = next();
    r.op = SEL, r.obj = f, r.x = x;
    put();
  }
  INLINE void bit(void *f, int node, int b, int pp) {
    Rec &r = next();
    r.op = BIT, r.obj = f, r.node = node, r.val = b, r.prp = pp;
    put();
  }
  INLINE void code(void *f, Op op, const tcx &x, i64 v, u32 sc, int prp, const int *prpa, int nd, i64 hint) {
    Rec &r = next();
    r.op = op, r.obj = f, r.x = x, r.val = v, r.node = (int)sc, r.prp = prp, r.hint = hint, r.pa = prpa != nullptr;
    if(prpa)
      memcpy(r.prpa, prpa, sizeof(int) * (size_t)(nd < 32 ? nd : 32));
    put();
  }
  INLINE void sign(void *f, u32 sc, int s) {
    Rec &r = next();
    r.op = SIGN, r.obj = f, r.node = (int)sc, r.val = s;
    put();
  }
  INLINE void raw(void *m, u32 b) {
    Rec &r = next();
    r.op = RAW, r.obj = m, r.val = b;
    put();
  }
  void start() {
    th = std::thread([this] { run(); });
  }
  void run() {
    size_t t = 0;
    for(int k = 0;; k++) {
      size_t hh = head.load();
      if(hh == t) {
        if(quit.load() || (fin.load() && head.load() == t))
          return;
        if(k < 16) {
          spin();
          continue;
        }
        std::unique_lock<std::mutex> lk(mu);
        cfrom = t;
        csleep = true;
        cv.wait(lk, [&] { return head.load() - t >= kWake || fin.load() || quit.load(); });
        csleep = false;
        continue;
      }
      k = 0;
      for(; t < hh; t++) {
        Rec &r = ring[t & (N - 1)];
        if(t + 8 < hh)
          pre(ring[(t + 8) & (N - 1)]);
        if(r.op == END) {
          done(t + 1);
          return;
        }
        if(!failed) {
          try {
            exec(r);
          } catch(const std::exception &e) {
            failed = true;
            err = e.what();
          }
        }
        if((t & 63) == 63)
          done(t + 1);
      }
      done(t);
      if(idle && !failed)
        idle();
    }
  }
  //  The model is done: all of it coded, or what the coding failed with
  //  thrown
  void finish() {
    Rec &r = next();
    r.op = END;
    put();
    publish();
    fin = true;
    wake();
    th.join();
    if(failed)
      throw CmError(err);
  }
  ~TcDefer() {
    if(th.joinable()) {  // the model failed: what it recorded is coded, for nothing
      quit = true;
      wake();
      th.join();
    }
  }
};
#endif

template <int f_DEC> struct tc_fam {
  static constexpr int ENC = (f_DEC == 0);
  int hA, hB, hD, hE, hH, hI, hJ, hK, hL, hS, hF, hM, hT, hN;
  cm_cnt2 *A, *B, *C, *D, *E, *H, *I, *J, *K, *L;
  cm_cnt *G, *T;
  i16 *S, *S2;
  i32 *W;
  cm_apm ap, ap2;
  cm_mix<TC_MIXN> mx, mx2, mx3;
  cm_mix<4> mxf;
  cm_mix<3> mxm;
  int prp;          // an outside prediction of P(0) for the mixers, -1 for none
  const int *prpa;  // or one per node
  //  A magnitude code() expects the value near, -1 none: each decision
  //  then also sees the hint's own decision there while the value agrees
  //  with it so far, or which side of it the value went (hs: 0 no hint,
  //  1/2 the hint decides 0/1, 3 the value is above it, 4 below), through
  //  counters of its own by the t index and node (Z; ZM for the mantissa).
  i64 hint;
  int hs;
  //  what the coding takes (prp, prpa and hint as they were at the call,
  //  or as recorded)
  int cprp;
  const int *cprpa;
  i64 chint;
#ifndef H264P_NO_THREADS
  TcDefer *dq = nullptr;  // the compressor's coder's thread, null none
#endif
  u32 bz;
  std::vector<cm_cnt2> Z;
  std::vector<cm_cnt> ZM;
  sz ba, bb, bc, bd, be, bh, bi, bj, bk, bl;
  u32 bs, bf, bm, bw2, bt, bn, bg;
  int rA, rB, rC, rD, rE, rH, rI, rJ, rK, rL, rT, rG;
  int mwA, mwB, mwC, mwD, mwE, mwH, mwI, mwJ, mwK, mwL, mwT, mwG;
  int rS1, rS2;
  //  the ten counters' limits and targets (bit 0, 1) as 32-bit lanes, two
  //  each (cn_v: their arrays in this order in memory, coded at once)
  int cn_lim[24], cn_tg[2][24];
  bool cn_v = false;
  int lr, mb, lrm, mbm;
  int bw;
  int mt;  // the mantissa bits whose nodes have the bits above them
  int ng;
  int nd, nm;
  void wire(cm_cnt2 *a, sz va, cm_cnt2 *b, sz vb, cm_cnt2 *c, sz vc, cm_cnt2 *d, sz vd, cm_cnt2 *e, sz ve, cm_cnt2 *h, sz vh, cm_cnt2 *i_, sz vi, cm_cnt2 *j_, sz vj, cm_cnt2 *k_, sz vk, cm_cnt2 *l_, sz vl, cm_cnt *t, sz vt, i16 *s, u8 *sc, sz vs, i16 *s2, u8 *sc2, sz vf, i32 *w, sz vm, cm_cnt *g, sz vg, sz vgi, i32 *wm, sz vn, u8 *wc, u8 *wmc, i32 *w2, u8 *w2c, sz vw, i32 *w3, u8 *w3c, i32 *wf, u8 *wfc, int ra, int rb, int rc_, int rd, int re, int rh, int ri, int rj, int rk, int rl, int rt, int rg, int mwa, int mwb, int mwc, int mwd, int mwe, int mwh, int mwi, int mwj, int mwk, int mwl, int mwtt, int mwg, int rs1, int rs2, int lrate, int mbt, int lrm2, int mbm2, int bwt, int mtr, int qqs, int qqf, int ndp, int nmp, int ha, int hb, int hd, int he, int hh, int hi, int hj, int hk, int hl, int hs, int hf, int hm, int ht, int hn) {
    A = a;
    B = b;
    C = c;
    D = d;
    E = e;
    H = h;
    I = i_;
    J = j_;
    K = k_;
    L = l_;
    T = t;
    S = s;
    S2 = s2;
    W = w;
    G = g;
    ng = (int)vg;
    nd = ndp;
    nm = nmp;
    //  Hashed where the Volume outgrows 2^h rows, direct otherwise; the
    //  rows are what the arrays were declared with (tc_rows).
    hA = va > ((sz)1 << tc_hbits(ha)) ? tc_hbits(ha) : 0;
    hB = vb > ((sz)1 << tc_hbits(hb)) ? tc_hbits(hb) : 0;
    hD = vd > ((sz)1 << tc_hbits(hd)) ? tc_hbits(hd) : 0;
    hE = ve > ((sz)1 << tc_hbits(he)) ? tc_hbits(he) : 0;
    hH = vh > ((sz)1 << tc_hbits(hh)) ? tc_hbits(hh) : 0;
    hI = vi > ((sz)1 << tc_hbits(hi)) ? tc_hbits(hi) : 0;
    hJ = vj > ((sz)1 << tc_hbits(hj)) ? tc_hbits(hj) : 0;
    hK = vk > ((sz)1 << tc_hbits(hk)) ? tc_hbits(hk) : 0;
    hL = vl > ((sz)1 << tc_hbits(hl)) ? tc_hbits(hl) : 0;
    hS = vs > ((sz)1 << tc_hbits(hs)) ? tc_hbits(hs) : 0;
    hF = vf > ((sz)1 << tc_hbits(hf)) ? tc_hbits(hf) : 0;
    hM = vm > ((sz)1 << tc_hbits(hm)) ? tc_hbits(hm) : 0;
    hT = vt > ((sz)1 << tc_hbits(ht)) ? tc_hbits(ht) : 0;
    hN = vn > ((sz)1 << tc_hbits(hn)) ? tc_hbits(hn) : 0;
    {
      sz ka = tc_rows((long long)va, ha), kb = tc_rows((long long)vb, hb), kd = tc_rows((long long)vd, hd);
      sz ke = tc_rows((long long)ve, he), kh = tc_rows((long long)vh, hh);
      sz ki = tc_rows((long long)vi, hi), kj = tc_rows((long long)vj, hj);
      sz kk = tc_rows((long long)vk, hk), kl = tc_rows((long long)vl, hl);
      sz ks = tc_rows((long long)vs, hs), kf = tc_rows((long long)vf, hf), km = tc_rows((long long)vm, hm);
      sz kt = tc_rows((long long)vt, ht), kn = tc_rows((long long)vn, hn);
      tc_vfits("counter A", ka, nd);
      tc_vfits("counter B", kb, nd);
      tc_vfits("counter C", vc, nd);
      tc_vfits("counter D", kd, nd);
      tc_vfits("counter E", ke, nd);
      tc_vfits("counter H", kh, nd);
      tc_vfits("counter I", ki, nd);
      tc_vfits("counter J", kj, nd);
      tc_vfits("counter K", kk, nd);
      tc_vfits("counter L", kl, nd);
      tc_vfits("counter T", kt, nm);
      if(G)
        tc_vfits("counter G", vg, vgi);
      tc_ifits("APM 1", ks, (sz)nd * qqs);
      tc_ifits("APM 2", kf, (sz)nd * qqf);
      tc_ifits("mixer", km, (sz)nd * TC_MIXN);
      tc_ifits("mantissa mixer", kn, (sz)nm * 3);
      ap.init(S, sc, ks * nd, qqs);
      ap2.init(S2, sc2, kf * nd, qqf);
      mx.init(W, wc, km * nd);
      tc_ifits("mixer 2", vw, (sz)nd * TC_MIXN);
      mx2.init(w2, w2c, vw * nd);
      mx3.init(w3, w3c, ks * nd);
      mxf.init(wf, wfc, nd * 5);
      mxm.init(wm, wmc, kn * nm);
    }
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rE = tc_clamp(re, 1, CM_TMAX);
    rH = tc_clamp(rh, 1, CM_TMAX);
    rI = tc_clamp(ri, 1, CM_TMAX);
    rJ = tc_clamp(rj, 1, CM_TMAX);
    rK = tc_clamp(rk, 1, CM_TMAX);
    rL = tc_clamp(rl, 1, CM_TMAX);
    rT = tc_clamp(rt, 1, CM_TMAX);
    rG = tc_clamp(rg, 1, CM_TMAX);
    mwA = tc_clamp(mwa * 64, 0, CM_CONE / 4);
    mwB = tc_clamp(mwb * 64, 0, CM_CONE / 4);
    mwC = tc_clamp(mwc * 64, 0, CM_CONE / 4);
    mwD = tc_clamp(mwd * 64, 0, CM_CONE / 4);
    mwE = tc_clamp(mwe * 64, 0, CM_CONE / 4);
    mwH = tc_clamp(mwh * 64, 0, CM_CONE / 4);
    mwI = tc_clamp(mwi * 64, 0, CM_CONE / 4);
    mwJ = tc_clamp(mwj * 64, 0, CM_CONE / 4);
    mwK = tc_clamp(mwk * 64, 0, CM_CONE / 4);
    mwL = tc_clamp(mwl * 64, 0, CM_CONE / 4);
    mwT = tc_clamp(mwtt * 64, 0, CM_CONE / 4);
    mwG = tc_clamp(mwg * 64, 0, CM_CONE / 4);
    {
      const int lim[10] = {rA, rB, rC, rD, rE, rH, rI, rJ, rK, rL}, mw[10] = {mwA, mwB, mwC, mwD, mwE, mwH, mwI, mwJ, mwK, mwL};
      const cm_cnt2 *arr[10] = {A, B, C, D, E, H, I, J, K, L};
      cn_v = true;
      for(int k = 0; k < 12; k++)
        for(int i = 0; i < 2; i++) {
          cn_lim[2 * k + i] = k < 10 ? lim[k] : 0;
          cn_tg[0][2 * k + i] = k < 10 ? CM_CONE - 1 - mw[k] : 0;
          cn_tg[1][2 * k + i] = k < 10 ? mw[k] : 0;
        }
      for(int k = 0; k + 1 < 10; k++)
        cn_v &= arr[k] < arr[k + 1];
    }
    rS1 = cm_apm::limit(tc_clamp(rs1, 1, 15));
    rS2 = cm_apm::limit(tc_clamp(rs2, 1, 15));
    lr = tc_clamp(lrate, 1, 64);
    mb = tc_clamp(mbt, 0, 255);
    lrm = tc_clamp(lrm2, 1, 64);
    mbm = tc_clamp(mbm2, 0, 255);
    bw = tc_clamp(bwt, 0, 16);
    mt = tc_clamp(mtr, 0, TC_MTREE);
    prp = cprp = -1;
    prpa = cprpa = nullptr;
    hint = chint = -1;
    hs = 0;
    zrows = tc_rows((long long)vt, ht);
  }
  sz zrows;
  //  The coding calls of the model: coded at once, or recorded for the
  //  coder's thread (whose results they know: the values they code)
  INLINE void select(const tcx &x) {
#ifndef H264P_NO_THREADS
    if(!f_DEC && dq)
      return dq->sel(this, x);
#endif
    select_x(x);
  }
  INLINE int bit(int node, int b) {
    if(UNLIKELY((unsigned)node >= (unsigned)nd))
      cm_fail("internal: decision node %d of a family of %d", node, nd);
#ifndef H264P_NO_THREADS
    if(!f_DEC && dq)
      return dq->bit(this, node, b, prpa ? prpa[node] : prp), b;
#endif
    cprp = prp, cprpa = prpa;
    return bit_x(node, b);
  }
  i64 code(const tcx &v, i64 x) {
#ifndef H264P_NO_THREADS
    if(!f_DEC && dq) {
      if(UNLIKELY(x < 0))
        cm_fail("internal: a negative magnitude");
      return dq->code(this, TcDefer::CODE, v, x, 0, prp, prpa, nd, hint), x;
    }
#endif
    cprp = prp, cprpa = prpa, chint = hint;
    return code_x(v, x);
  }
  i64 codes(const tcx &v, i64 x, u32 sc) {
#ifndef H264P_NO_THREADS
    if(!f_DEC && dq)
      return dq->code(this, TcDefer::CODES, v, x, sc, prp, prpa, nd, hint), x;
#endif
    cprp = prp, cprpa = prpa, chint = hint;
    return codes_x(v, x, sc);
  }
  int sign(u32 sc, int s) {
#ifndef H264P_NO_THREADS
    if(!f_DEC && dq)
      return dq->sign(this, sc, s), s;
#endif
    return sign_x(sc, s);
  }
  //  The rows select() and the first decision will read, on their way
  //  into the cache (the coder's thread, records ahead)
  INLINE void pre(const tcx &x) const {
    CM_PREFETCH(A + (sz)(hA ? tc_hrow((u32)x.a, hA) : (u32)x.a) * nd);
    CM_PREFETCH(B + (sz)(hB ? tc_hrow((u32)x.b, hB) : (u32)x.b) * nd);
    CM_PREFETCH(C + (sz)x.c * nd);
    CM_PREFETCH(D + (sz)(hD ? tc_hrow((u32)x.d, hD) : (u32)x.d) * nd);
    CM_PREFETCH(E + (sz)(hE ? tc_hrow((u32)x.e, hE) : (u32)x.e) * nd);
    CM_PREFETCH(H + (sz)(hH ? tc_hrow((u32)x.h, hH) : (u32)x.h) * nd);
    CM_PREFETCH(I + (sz)(hI ? tc_hrow((u32)x.i, hI) : (u32)x.i) * nd);
    CM_PREFETCH(J + (sz)(hJ ? tc_hrow((u32)x.j, hJ) : (u32)x.j) * nd);
    CM_PREFETCH(K + (sz)(hK ? tc_hrow((u32)x.k, hK) : (u32)x.k) * nd);
    CM_PREFETCH(L + (sz)(hL ? tc_hrow((u32)x.l, hL) : (u32)x.l) * nd);
    u32 s = (hS ? tc_hrow((u32)x.s, hS) : (u32)x.s) * nd, f = (hF ? tc_hrow((u32)x.f, hF) : (u32)x.f) * nd;
    u32 m = (hM ? tc_hrow((u32)x.m, hM) : (u32)x.m) * nd;
    CM_PREFETCH(ap.t + (sz)s * (u32)ap.q);
    CM_PREFETCH(ap.c + (sz)s * (u32)ap.q);
    CM_PREFETCH(ap2.t + (sz)f * (u32)ap2.q);
    CM_PREFETCH(ap2.c + (sz)f * (u32)ap2.q);
    CM_PREFETCH(mx.w + (sz)m * TC_MIXN);
    CM_PREFETCH(mx2.w + (sz)((u32)x.w * nd) * TC_MIXN);
    CM_PREFETCH(mx3.w + (sz)s * TC_MIXN);
  }
  INLINE void select_x(const tcx &x) {
    ba = (sz)(hA ? tc_hrow((u32)x.a, hA) : (u32)x.a) * nd;
    bb = (sz)(hB ? tc_hrow((u32)x.b, hB) : (u32)x.b) * nd;
    bc = (sz)x.c * nd;
    bd = (sz)(hD ? tc_hrow((u32)x.d, hD) : (u32)x.d) * nd;
    be = (sz)(hE ? tc_hrow((u32)x.e, hE) : (u32)x.e) * nd;
    bh = (sz)(hH ? tc_hrow((u32)x.h, hH) : (u32)x.h) * nd;
    bi = (sz)(hI ? tc_hrow((u32)x.i, hI) : (u32)x.i) * nd;
    bj = (sz)(hJ ? tc_hrow((u32)x.j, hJ) : (u32)x.j) * nd;
    bk = (sz)(hK ? tc_hrow((u32)x.k, hK) : (u32)x.k) * nd;
    bl = (sz)(hL ? tc_hrow((u32)x.l, hL) : (u32)x.l) * nd;
    bs = (hS ? tc_hrow((u32)x.s, hS) : (u32)x.s) * nd;
    bf = (hF ? tc_hrow((u32)x.f, hF) : (u32)x.f) * nd;
    bm = (hM ? tc_hrow((u32)x.m, hM) : (u32)x.m) * nd;
    bw2 = (u32)x.w * nd;
    bz = hT ? tc_hrow((u32)x.t, hT) : (u32)x.t;
    bt = bz * nm;
    bn = (hN ? tc_hrow((u32)x.n, hN) : (u32)x.n) * nm;
    bg = (u32)x.g;
    fetch(0);
  }
  //  The rows a decision of the node reads, on their way into the cache
  //  (for the first node from select(), for the next while one is coded)
  INLINE void fetch(int node) {
    CM_PREFETCH(A + ba + node);
    CM_PREFETCH(B + bb + node);
    CM_PREFETCH(C + bc + node);
    CM_PREFETCH(D + bd + node);
    CM_PREFETCH(E + be + node);
    CM_PREFETCH(H + bh + node);
    CM_PREFETCH(I + bi + node);
    CM_PREFETCH(J + bj + node);
    CM_PREFETCH(K + bk + node);
    CM_PREFETCH(L + bl + node);
    CM_PREFETCH(ap.t + (sz)(bs + node) * (u32)ap.q);
    CM_PREFETCH(ap.c + (sz)(bs + node) * (u32)ap.q);
    CM_PREFETCH(ap2.t + (sz)(bf + node) * (u32)ap2.q);
    CM_PREFETCH(ap2.c + (sz)(bf + node) * (u32)ap2.q);
    const i32 *w1 = mx.w + (sz)(bm + node) * TC_MIXN, *w2 = mx2.w + (sz)(bw2 + node) * TC_MIXN;
    const i32 *w3 = mx3.w + (sz)(bs + node) * TC_MIXN;
    CM_PREFETCH(w1);
    CM_PREFETCH(w1 + TC_MIXN - 1);
    CM_PREFETCH(w2);
    CM_PREFETCH(w2 + TC_MIXN - 1);
    CM_PREFETCH(w3);
    CM_PREFETCH(w3 + TC_MIXN - 1);
    CM_PREFETCH(mx.c + bm + node);
    CM_PREFETCH(mx2.c + bw2 + node);
    CM_PREFETCH(mx3.c + bs + node);
  }
  INLINE int bit_x(int node, int b) {
    if(node + 1 < nd)
      fetch(node + 1);
    cm_cnt2 *const cp[10] = {A + ba + node, B + bb + node, C + bc + node, D + bd + node, E + be + node,
                             H + bh + node, I + bi + node, J + bj + node, K + bk + node, L + bl + node};
    //  The mixers' inputs: the ten counters' two probabilities each (in
    //  the order of the arrays), the APM's refinement of A's, the outside
    //  prediction, the bias and the hint's counter (stretched); the three
    //  mixers' are the same, packed once
    cm_x16<TC_MIXN> xp;
    cm_cnt2 cn[10];
    int pa;
#ifdef H264P_AVX2
    //  (cn_v: the counters all at once, as 64-bit lanes p, t, q and the next
    //  counter's p, four to a vector; their probabilities as 32-bit lanes,
    //  P and Q of each in turn)
    __m256i cv[3];
    if(cn_v) {
      auto ld2 = [&](int k) {
        return _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)cp[k]), _mm_loadl_epi64((const __m128i *)cp[k + 1]));
      };
      cv[0] = _mm256_inserti128_si256(_mm256_castsi128_si256(ld2(0)), ld2(2), 1);
      cv[1] = _mm256_inserti128_si256(_mm256_castsi128_si256(ld2(4)), ld2(6), 1);
      cv[2] = _mm256_inserti128_si256(_mm256_setzero_si256(), ld2(8), 0);
      const __m256i m16 = _mm256_set1_epi32(0xFFFF), z = _mm256_setzero_si256();
      __m256i st[3];
      for(int j = 0; j < 3; j++) {
        // (cm_cnt2::P and Q, then cm_stretch)
        __m256i t = _mm256_srli_epi32(_mm256_shuffle_epi32(cv[j], 0xA0), 16);
        __m256i v = _mm256_max_epi32(_mm256_srli_epi32(_mm256_and_si256(cv[j], m16), CM_CBITS - CM_PBITS), _mm256_set1_epi32(1));
        v = _mm256_blendv_epi8(v, _mm256_set1_epi32(CM_PHALF), _mm256_cmpeq_epi32(t, z));
        if(!j)
          pa = _mm256_cvtsi256_si32(v);
        st[j] = _mm256_srai_epi32(_mm256_slli_epi32(_mm256_i32gather_epi32((const int *)cm_st, v, 2), 16), 16);
      }
      xp.v[0] = _mm256_packs_epi32(st[0], st[1]);
      xp.t = _mm256_castsi256_si128(st[2]);
    } else
#endif
    {
      for(int k = 0; k < 10; k++) {
        cn[k] = *cp[k];
        mx.set(2 * k, cm_stretch(cn[k].P()));
        mx.set(2 * k + 1, cm_stretch(cn[k].Q()));
      }
      pa = cn[0].P();
    }
    int p1 = ap.pp(pa, bs + (u32)node);
    if(p1 > CM_PONE - 1)
      p1 = CM_PONE - 1;
    int pp = cprpa ? cprpa[node] : cprp;
    cm_cnt2 *zc = hs ? &Z[((sz)bz * nd + (u32)node) * 5 + (u32)hs] : nullptr;
    mx.set(20, cm_stretch(p1));
    mx.set(21, pp >= 0 ? cm_stretch(pp) : 0);
    mx.set(22, 256);
    mx.set(23, zc ? cm_stretch(zc->P()) : 0);
#ifdef H264P_AVX2
    if(cn_v)
      xp.t = _mm_packs_epi32(xp.t, _mm_loadu_si128((const __m128i *)(mx.x + 20)));
    else
      xp.pack(mx.x);
    const cm_x16<TC_MIXN> *xq = &xp;
#else
    const cm_x16<TC_MIXN> *xq = nullptr;
#endif
    {
      mxf.set(0, cm_stretch(mx.mix(bm + (u32)node, mx.x, xq)));
      mxf.set(1, cm_stretch(mx2.mix(bw2 + (u32)node, mx.x, xq)));
      mxf.set(2, cm_stretch(mx3.mix(bs + (u32)node, mx.x, xq)));
      mxf.set(3, 256);
      int pm = mxf.mix((u32)node * 5 + (u32)hs);  // by node and hint state
      int pf = ap2.pp(pm, bf + (u32)node);
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit<f_DEC>(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b);
    }
    mx.upd(b, lr, mb, mx.x, xq);
    mx2.upd(b, lr, mb, mx.x, xq);
    mx3.upd(b, lr, mb, mx.x, xq);
    mxf.upd(b, lr, mb);
    ap.upd(b, rS1);
    ap2.upd(b, rS2);
#ifdef H264P_AVX2
    if(cn_v) {
      // as cm_cnt2::upd, each counter's q through its p's arithmetic with
      // the rate 8192 / 65536; written back in the order of the arrays,
      // each with the next counter's p as it was (rewritten after, where
      // the next counter is one of these)
      const __m256i m16 = _mm256_set1_epi32(0xFFFF), z = _mm256_setzero_si256();
      const __m256i even = _mm256_setr_epi32(-1, 0, -1, 0, -1, 0, -1, 0);
      for(int j = 0; j < 3; j++) {
        __m256i t = _mm256_srli_epi32(_mm256_shuffle_epi32(cv[j], 0xA0), 16);
        __m256i cur = _mm256_blendv_epi8(_mm256_and_si256(cv[j], m16), _mm256_set1_epi32(CM_CONE / 2), _mm256_cmpeq_epi32(t, z));
        __m256i t1 = _mm256_sub_epi32(t, _mm256_cmpgt_epi32(_mm256_loadu_si256((const __m256i *)(cn_lim + 8 * j)), t));
        __m256i rec = _mm256_and_si256(_mm256_mask_i32gather_epi32(_mm256_set1_epi32(8192), (const int *)cm_rec, t1, even, 2), m16);
        __m256i tg = _mm256_loadu_si256((const __m256i *)(cn_tg[b] + 8 * j));
        __m256i nv = _mm256_add_epi32(cur, _mm256_srai_epi32(_mm256_mullo_epi32(_mm256_sub_epi32(tg, cur), rec), CM_CBITS));
        __m256i up = _mm256_blend_epi32(_mm256_slli_epi32(t1, 16), _mm256_andnot_si256(m16, cv[j]), 0xAA);
        __m256i o = _mm256_or_si256(_mm256_and_si256(nv, m16), up);
        __m128i lo = _mm256_castsi256_si128(o);
        _mm_storel_epi64((__m128i *)cp[4 * j], lo);
        _mm_storel_epi64((__m128i *)cp[4 * j + 1], _mm_unpackhi_epi64(lo, lo));
        if(j < 2) {
          __m128i hi = _mm256_extracti128_si256(o, 1);
          _mm_storel_epi64((__m128i *)cp[4 * j + 2], hi);
          _mm_storel_epi64((__m128i *)cp[4 * j + 3], _mm_unpackhi_epi64(hi, hi));
        }
      }
    } else
#endif
    {
      const int lim[10] = {rA, rB, rC, rD, rE, rH, rI, rJ, rK, rL}, mw[10] = {mwA, mwB, mwC, mwD, mwE, mwH, mwI, mwJ, mwK, mwL};
      for(int k = 0; k < 10; k++) {
        cn[k].upd(b, lim[k], mw[k]);
        *cp[k] = cn[k];
      }
    }
    if(zc)
      zc->upd(b, rT, mwT);
    return b;
  }
  INLINE int bitm(int node, int b) {
    if(UNLIKELY((unsigned)node >= (unsigned)nm))
      cm_fail("internal: mantissa node %d of a family of %d", node, nm);
    cm_cnt t = T[bt + node];
    if(hs && (cprp < 0 || cprpa)) {
      // with the hint's counter, through the mantissa mixer the value
      // would not use otherwise
      cm_cnt &z = ZM[((sz)bz * nm + (u32)node) * 5 + (u32)hs];
      mxm.set(0, cm_stretch(t.P()));
      mxm.set(1, cm_stretch(z.P()));
      mxm.set(2, 256);
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit<f_DEC>(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
      mxm.upd(b, lrm, mbm);
      t.upd(b, rT, mwT);
      T[bt + node] = t;
      z.upd(b, rT, mwT);
      return b;
    }
    if(cprp < 0 || cprpa) {
      b = tc_bit<f_DEC>(t.P(), b);
      t.upd(b, rT, mwT);
      T[bt + node] = t;
      return b;
    }
    mxm.set(0, cm_stretch(t.P()));
    mxm.set(1, cm_stretch(cprp));
    mxm.set(2, 256);
    {
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit<f_DEC>(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
    }
    mxm.upd(b, lrm, mbm);
    t.upd(b, rT, mwT);
    T[bt + node] = t;
    return b;
  }
  i64 code_x(const tcx &v, i64 x) {
    int k, p, nb = 0, hst = 0, hnb = 0;
    u64 u, hu = 0;
    select_x(v);
    if(UNLIKELY(ENC && x < 0))
      cm_fail("internal: a negative magnitude");
    const i64 hint = chint;
    if(hint >= 0) {
      // hst: 1 the value agrees with the hint so far, 3 it is above, 4 below
      hst = 1;
      if(Z.empty())
        Z.assign(zrows * (sz)nd * 5, cm_cnt2{}), ZM.assign(zrows * (sz)nm * 5, cm_cnt{});
      hu = hint >= 3 ? (u64)(hint - 3) + 1 : 0;
      for(u64 t = hu; t; t >>= 1)
        hnb++;
    }
    for(k = 0; k < 3; k++) {
      int hb = hint == k;
      hs = hst == 1 ? 1 + hb : hst;
      int b = bit_x(k, ENC && x == k);
      if(hst == 1 && b != hb)
        hst = b ? 4 : 3;
      if(b) {
        hs = 0;
        return k;
      }
    }
    u = ENC ? (u64)(x - 3) + 1 : 1;
    if(ENC) {
      u64 t = u;
      while(t) {
        nb++;
        t >>= 1;
      }
    }
    for(k = 0;; k++) {
      int hb = hnb == k + 1;
      hs = hst == 1 ? 1 + hb : hst;
      int b = bit_x(tc_node_len(k), ENC && nb == k + 1);
      if(hst == 1 && b != hb)
        hst = b ? 4 : 3;
      if(b) {
        nb = k + 1;
        break;
      }
      if(UNLIKELY(k + 2 > TC_NBMAX))
        cm_fail("%s a value of 2^%d or more", ENC ? "internal:" : "corrupt stream:", TC_NBMAX);
    }
    {
      u64 pref = 1;
      for(p = 0; p < nb - 1; p++) {
        int hb = (int)(hu >> (nb - 2 - p) & 1);
        hs = hst == 1 ? 1 + hb : hst;
        int mb = bitm(tc_node_man(nb, p, pref, mt), ENC && ((u >> (nb - 2 - p)) & 1) != 0);
        if(hst == 1 && mb != hb)
          hst = mb > hb ? 3 : 4;
        pref = pref * 2 + (u64)mb;
      }
      if(!ENC)
        u = pref;
    }
    hs = 0;
    return (i64)(u - 1) + 3;
  }
  //  The sign of a nonzero value, under the g index of the last select()
  //  and sign context sc: 1 negative.
  int sign_x(u32 sc, int s) {
    if(!G)
      cm_fail("internal: a family with no sign counter coded a sign");
    cm_cnt g = G[bg * (u32)ng + sc];
    s = tc_bit<f_DEC>(g.P(), ENC && s);
    g.upd(s, rG, mwG);
    G[bg * (u32)ng + sc] = g;
    return s;
  }
  i64 codes_x(const tcx &v, i64 x, u32 sc) {
    i64 m = code_x(v, ENC ? (x < 0 ? -x : x) : 0);
    if(!m)
      return 0;
    return sign_x(sc, x < 0) ? -m : m;
  }
};

}  // namespace h264p
