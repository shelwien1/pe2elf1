// model_core.inc -- what the model's elements share (model.inc): the coder, the state of the
// macroblocks and blocks around the one being coded, the motion state (motion.inc), the decoder in
// the loop (recon.inc) with its intra estimate and template matches; and the helpers they all use.

// a family F of the level, in an element of the model: its coder (f_F) and its tables (t_F)
#define M_FAM(F)                       \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;

struct ModelCore {
  Coder* c;  // the run's coder
  BlkCtx rowb[MAX_MB_W][MB_BLOCKS];
  // per macroblock, mb_cap of them: the current VOP's and the last anchor VOP's
  MBCtx *mbcur, *mbref;
  size_t mb_cap;
  int ref_mbs;  // size of the VOP in mbref, 0 if none
  Motion mo;    // the vectors as the decoder makes them (motion.inc)
  Recon rec;    // the decoder in the loop (the levels that have it: recon)
  RcEst est;    // its intra estimate for the macroblock (est.ok: there is one)
  int tmk[2], tmx[2], tmy[2];  // its vectors by template matching, forward and backward (tmk: found)
  int est_err, est_nb;         // how far the macroblock's blocks so far were from the estimate (quarter levels)

  // as the run is made, and as it ends
  void core_init(int recon) {
    mbcur = mbref = 0;
    mb_cap = 0;
    mo.init();
    if (recon) rc_init(&rec), rec.mo = &mo;
  }
  void core_quit(int recon) {
    free(mbcur);
    free(mbref);
    mbcur = mbref = 0;
    mb_cap = 0;
    mo.quit();
    if (recon) rc_quit(&rec);
  }
  // room for n macroblocks (0: no memory)
  int mb_room(int n) {
    size_t r[2] = { mb_cap, mb_cap };  // (the two the same: the same growth)
    if ((size_t)n <= mb_cap) return 1;
    if (!grow(&mbcur, &r[0], n) || !grow(&mbref, &r[1], n)) return 0;
    memset(mbref + mb_cap, 0, (r[1] - mb_cap) * sizeof(MBCtx));
    mb_cap = r[0];
    return 1;
  }
  void rowb_clear(int x) {
    int i;
    memset(rowb[x], 0, sizeof(rowb[x]));
    for (i = 0; i < MB_BLOCKS; i++) rowb[x][i].last = rowb[x][i].dcs = -1;
  }

  // family f's contexts from its variables D, selected: what its bit()s code with
  template <class F, class V>
  static INLINE void sel(F& f, const V& D) {
    tcx X;
    f.make(D, X);
    f.select(X);
  }
  // x, checked: a value decompressed outside [lo, hi] is damaged data (the coder's error set: the
  // walk fails the chunk), lo then
  INLINE i64 checked(i64 x, i64 lo, i64 hi) {
    if (UNLIKELY(x < lo || x > hi)) {
      c->err = 1;
      return lo;
    }
    return x;
  }

  // (level -9) an estimated level (quarter levels, signed) as a variable: 1 + its magnitude, capped
  static int est_level(int e4) { return 1 + cap(e4 < 0 ? -e4 : e4, kEstCap); }
  // how far to trust the estimate (0 no estimate): before the macroblock's first block by its
  // template's miss (kTqNoTemplate none, else kTqMiss + its mean difference, quarter samples, on
  // tc_qlog's scale), then by how far the blocks so far were from it (kTqErr + their mean
  // difference per block, levels, on tc_qlog's scale)
  static constexpr int kTqNoTemplate = 1, kTqMiss = 2, kTqErr = 24;
  int est_trust() const {
    if (!est.ok) return 0;
    if (est_nb) return kTqErr + cap(tc_qlog(est_err / (kQ * est_nb)), kQlogCap - 1);
    return est.miss < 0 ? kTqNoTemplate : kTqMiss + cap(tc_qlog(est.miss), kTqErr - kTqMiss - 1);
  }

  // how far an intra block's levels as coded (cl, by raster position) were from the estimate (E4)
  void est_account(const int* E4, const int* cl) {
    int i, e = 0;
    for (i = 0; i < BLK_COEFS; i++) e += abs(kQ * cl[i] - E4[i]);
    est_err += e;
    est_nb++;
  }
};
