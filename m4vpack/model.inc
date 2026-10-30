// model.inc -- what codes and decodes the records of a stream, at a level (levels.inc): each
// decision goes to one of the level's families (fam.inc), whose contexts its IDX/l<L>/*.idx
// builds from the variables the model gives it (famvars.inc). The same functions code and decode
// every record: compressing (ENC) they take the values, decompressing they return them, so the
// two directions can't drift apart.

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.

template <int f_DEC, class LV>
struct Model {
  static constexpr int ENC = f_DEC == 0;
  Coder* c;  // the run's coder
  // each family's coder (f_hdr ...) and tables (t_hdr ...)
#define M_FAM(F)                       \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;
  M4V_FAMS(M_FAM)
#undef M_FAM
  // (a level reads no variable its parts cannot make: famvars.inc)
#define M_PARTS(F)                                                         \
  static_assert(LV::recon || !(LV::F::Uses & M_##F##_needs<NEED_REC>()), \
                "a level without the decoder reads a variable of " #F " that needs it");
  M4V_FAMS(M_PARTS)
#undef M_PARTS
  Last last;
  BlkCtx rowb[MAX_MB_W][MB_BLOCKS];
  // per macroblock, mb_cap of them: the current VOP's and the last anchor VOP's
  MBCtx *mbcur, *mbref;
  size_t mb_cap;
  int ref_mbs;  // size of the VOP in mbref, 0 if none
  U8 tailbuf[TAIL_MAX];
  MB mrec;      // the macroblock vop_walk() has
  Motion mo;    // the vectors as the decoder makes them (motion.inc)
  Recon rec;    // the decoder in the loop (LV::recon)
  RcEst est;    // its intra estimate for the macroblock (est.ok: there is one)
  int tmk[2], tmx[2], tmy[2];  // its vectors by template matching, forward and backward (tmk: found)
  int est_err, est_nb;         // how far the macroblock's blocks so far were from the estimate (quarter levels)

  void Init(void) {
#define M_INIT(F) f_##F.Init(), t_##F = nullptr;
    M4V_FAMS(M_INIT)
#undef M_INIT
    mbcur = mbref = 0;
    mb_cap = 0;
    mo_init(&mo);
    if (LV::recon) rc_init(&rec), rec.mo = &mo;
  }
  void Quit(void) {
#define M_QUIT(F)                                                                                    \
  if (t_##F) LV::F::quit(t_##F), tc_unmap(t_##F, sizeof(typename LV::F::T)), t_##F = nullptr; \
  f_##F.Quit();
    M4V_FAMS(M_QUIT)
#undef M_QUIT
    free(mbcur);
    free(mbref);
    mbcur = mbref = 0;
    mb_cap = 0;
    mo_quit(&mo);
    if (LV::recon) rc_quit(&rec);
  }
  // the run starts: the families' tables mapped and wired to the coder co
  void start(Coder* co) {
    int i;
    c = co;
#define M_MAP(F) t_##F = tc_table<typename LV::F::T>(), LV::F::init(t_##F), f_##F.wire(t_##F, c);
    M4V_FAMS(M_MAP)
#undef M_MAP
    memset(&last, 0, sizeof(last));
    last.tib = TIB_MIN;
    for (i = 0; i < PT_N; i++) { last.quant[i] = kQuantStart; last.fcode[i] = FCODE_MIN; }
    last.bcode = FCODE_MIN;
    ref_mbs = 0;
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

  // --- the headers: the hdr family, a field (fld) of the VOP of type ty (0 I, 1 P, 2 B, 3 S,
  // TY_NONE none) at a time, whose last value is p1
  INLINE void hdr_vars(M_hdr_v& D, int fld, int ty, i64 p1, int pos) {
    D.fld = fld;
    D.ty = ty;
    D.p1 = (int)clip(p1, 0, kHdrCap);
    D.pos = pos;
  }
  i64 hdr_val(int fld, int ty, i64 p1, int pos, i64 x) {  // x >= 0
    M_hdr_v D;
    tcx X;
    hdr_vars(D, fld, ty, p1, pos);
    f_hdr.make(D, X);
    return f_hdr.code(X, ENC ? x : 0);
  }
  i64 hdr_sval(int fld, int ty, i64 p1, int pos, i64 x) {  // signed
    M_hdr_v D;
    tcx X;
    hdr_vars(D, fld, ty, p1, pos);
    f_hdr.make(D, X);
    return f_hdr.codes(X, ENC ? x : 0, (u32)fld);
  }
  int hdr_flag(int fld, int ty, i64 p1, int pos, int b) {
    M_hdr_v D;
    hdr_vars(D, fld, ty, p1, pos);
    sel(f_hdr, D);
    return f_hdr.bit(0, ENC && b);
  }

  int code_kind(int k) {  // CK_*
    c->cat = CAT_RAW;
    k = (int)checked(hdr_val(F_KIND, TY_NONE, last.kind, 0, k), CK_END, CK_RAW);
    return last.kind = k;
  }

  // a chunk kept as bytes: its length, then the bytes (the raw family: the bytes before, the
  // chunk's start code, the place in it), which may come a piece at a time
  size_t code_raw_len(size_t n) {
    c->cat = CAT_RAW;
    last.rawat = last.rawcode = 0;
    last.rawhd = 0;
    return (size_t)checked(hdr_val(F_RAWLEN, TY_NONE, 0, 0, (i64)n), 0, kRawLenMax);
  }
  void code_raw_bytes(U8* p, size_t n) {
    M_raw_v D;
    size_t i;
    c->cat = CAT_RAW;
    for (i = 0; i < n; i++) {
      int x = 1, k;
      D.c1 = last.rawprev;
      D.c2 = last.rawprev2;
      D.code = last.rawcode;
      D.at = tc_qlog(last.rawat);
      sel(f_raw, D);
      for (k = BYTE_BITS - 1; k >= 0; k--) x = x * 2 + f_raw.bit(x, ENC && ((p[i] >> k) & 1));
      p[i] = (U8)(x - BYTE_VALUES);
      last.rawprev2 = last.rawprev;
      last.rawprev = p[i];
      if (last.rawat < SC_PREFIX) last.rawhd = last.rawhd << BYTE_BITS | p[i];
      else if (last.rawat == SC_PREFIX) last.rawcode = last.rawhd == SC_START ? sc_class(p[i]) : SCC_NONE;
      last.rawat++;
    }
  }

  void code_vop_hdr(const Ctx* cx, VopHdr* h) {
    const int ty0 = last.type ? last.type - PT_I : TY_NONE;
    int i, d, mask, ty;
    c->cat = CAT_HDR;
    h->type = PT_I + (int)checked(hdr_val(F_VTYPE, ty0, ty0, 0, h->type - PT_I), 0, PT_S - PT_I);
    ty = h->type - PT_I;
    h->mtb = (int)checked(hdr_val(F_MTB, ty, last.mtb, 0, h->mtb), 0, RUN_MAX);
    h->tib = (int)checked(hdr_val(F_TIB, ty, last.tib, 0, h->tib), TIB_MIN, TIB_MAX);
    mask = (int)((1u << h->tib) - 1);
    d = (h->tinc - last.tinc) & mask;
    if (hdr_flag(F_TSAME, ty, 0, 0, d == (last.dinc[h->type] & mask))) d = last.dinc[h->type] & mask;
    else d = (int)checked(hdr_val(F_TINC, ty, 0, 0, d), 0, mask);
    h->tinc = (last.tinc + d) & mask;
    last.type = h->type;
    last.mtb = h->mtb;
    last.tib = h->tib;
    last.tinc = h->tinc;
    last.dinc[h->type] = d;
    h->coded = hdr_flag(F_CODED, ty, 0, 0, h->coded);
    if (!ENC) vop_hdr_clear(h);
    if (!h->coded) return;
    if (vop_has_rnd(cx, h->type)) last.rnd = h->rnd = hdr_flag(F_RND, ty, last.rnd, 0, h->rnd);
    h->ncplx = vop_ncplx(cx, h->type);
    for (i = 0; i < h->ncplx; i++)
      h->cplx[i] = (char)('0' + hdr_flag(F_CPLX, ty, 0, cap(i, kPosCap), h->cplx[i] == '1'));
    last.dcthr = h->dcthr = (int)checked(hdr_val(F_DCTHR, ty, last.dcthr, 0, h->dcthr), 0, DCTHR_MAX);
    if (cx->interlaced) {
      last.tff = h->tff = hdr_flag(F_TFF, ty, last.tff, 0, h->tff);
      last.alt = h->alt = hdr_flag(F_ALT, ty, last.alt, 0, h->alt);
    }
    if (vop_gmc(cx, h->type))
      for (i = 0; i < 2 * cx->warp_points; i++)
        h->warp[i] = (int)checked(hdr_sval(F_WARP, ty, 0, i & 1, h->warp[i]), -WARP_MAX, WARP_MAX);
    h->quant = last.quant[h->type] + (int)hdr_sval(F_QUANT, ty, 0, 0, h->quant - last.quant[h->type]);
    h->quant = (int)checked(h->quant, QS_MIN, QUANT_MAX);
    last.quant[h->type] = h->quant;
    if (h->type != PT_I) {
      h->fcode = (int)checked(hdr_val(F_FCODE, ty, last.fcode[h->type], 0, h->fcode), FCODE_MIN, FCODE_MAX);
      last.fcode[h->type] = h->fcode;
    }
    if (h->type == PT_B) {
      h->bcode = (int)checked(hdr_val(F_BCODE, ty, last.bcode, 0, h->bcode), FCODE_MIN, FCODE_MAX);
      last.bcode = h->bcode;
    }
  }

  void code_vp(const Ctx* cx, VState* s, VpHdr* k, int expect) {
    Vop* v = &s->v;
    const int ty = v->type - PT_I;
    int i;
    c->cat = CAT_HDR;
    k->st = (int)checked(hdr_val(F_VPST, ty, 0, 0, k->st), 0, RUN_MAX);
    if (hdr_flag(F_VPMBSAME, ty, 0, 0, k->mbnum == expect)) k->mbnum = expect;
    else k->mbnum = (int)checked(hdr_val(F_VPMB, ty, 0, 0, k->mbnum), 0, MAX_MBS);
    k->quant = (int)checked(v->qscale + hdr_sval(F_VPQ, ty, 0, 0, k->quant - v->qscale), 0, QUANT_MAX);
    k->hec = hdr_flag(F_HEC, ty, 0, 0, k->hec);
    if (k->quant) v->qscale = k->quant;
    v->qscale = clip(v->qscale, QS_MIN, QS_MAX);
    if (!ENC) vp_clear(k);
    if (!k->hec) return;
    k->mtb = (int)checked(hdr_val(F_HMTB, ty, 0, 0, k->mtb), 0, RUN_MAX);
    k->tinc = (int)checked(hdr_val(F_HTINC, ty, 0, 0, k->tinc), 0, (1 << v->tib) - 1);
    k->type = (int)checked(hdr_val(F_HTYPE, ty, 0, 0, k->type), 0, PT_S - PT_I);
    k->dcthr = (int)checked(hdr_val(F_HDCTHR, ty, 0, 0, k->dcthr), 0, DCTHR_MAX);
    if (v->gmc)
      for (i = 0; i < 2 * cx->warp_points; i++)
        k->warp[i] = (int)checked(hdr_sval(F_HWARP, ty, 0, i & 1, k->warp[i]), -WARP_MAX, WARP_MAX);
    if (v->type != PT_I) k->fcode = (int)checked(hdr_val(F_HFCODE, ty, 0, 0, k->fcode), FCODE_MIN, FCODE_MAX);
    if (v->type == PT_B) k->bcode = (int)checked(hdr_val(F_HBCODE, ty, 0, 0, k->bcode), FCODE_MIN, FCODE_MAX);
  }


  // what follows the last macroblock: nb bits up to the byte boundary, then *n bytes
  int code_tail(int nb, int* bits, int* n, U8* bytes, int max) {
    int i, canon = 0;
    c->cat = CAT_HDR;
    if (ENC) canon = nb ? *bits == stuffing_bits(nb) && !*n : *n == 1 && bytes[0] == stuffing_bits(BYTE_BITS);
    if (hdr_flag(F_TAILC, TY_NONE, 0, nb, canon)) {  // the usual stuffing, to the byte boundary
      if (nb) { *bits = stuffing_bits(nb); *n = 0; }
      else { *bits = 0; *n = 1; bytes[0] = (U8)stuffing_bits(BYTE_BITS); }
      return 0;
    }
    *bits = (int)checked(hdr_val(F_TAILB, TY_NONE, 0, nb, *bits), 0, (1 << nb) - 1);
    *n = (int)checked(hdr_val(F_TAILN, TY_NONE, 0, 0, *n), 0, max + 1);
    if (*n > max) return -1;
    for (i = 0; i < *n; i++)
      bytes[i] = (U8)checked(hdr_val(F_TAILBYTE, TY_NONE, i ? bytes[i - 1] >> kTailCtx : 0, 0, bytes[i]), 0, BYTE_MASK);
    return 0;
  }

  // intra DC difference (the dcs family): its magnitude, then its sign. Contexts: the previous
  // block's size, the left and above blocks' (and their coefficient counts), whether the block has
  // AC coefficients, the quantiser; for the sign, the left and above blocks' and the previous one's
  // (e4: the intra estimate's difference, quarter levels, if eok; tq how far to trust it)
  int code_dc(VState* s, int lc, int v, const BlkCtx* Lb, const BlkCtx* Ab, int coded, int qs, int eok, int e4,
              int tq) {
    M_dcs_v D;
    tcx X;
    i64 a;
    int neg;
    const int ea = e4 < 0 ? -e4 : e4;
    D.lc = lc;
    D.prev = s->dc_prev[lc];
    D.ls = Lb && Lb->dcs >= 0 ? Lb->dcs : kDcSizeNone;
    D.as = Ab && Ab->dcs >= 0 ? Ab->dcs : kDcSizeNone;
    D.coded = coded;
    D.qs = qs;
    D.lbn = Lb ? cap(Lb->n, kCap) : kNone;
    D.abn = Ab ? cap(Ab->n, kCap) : kNone;
    D.ls2 = Lb && Lb->dcs > 0 ? 1 + Lb->dcsg : 0;
    D.as2 = Ab && Ab->dcs > 0 ? 1 + Ab->dcsg : 0;
    D.psg = s->dc_sgn[lc];
    D.sz = 0;
    D.ie = eok ? 1 + cap(ea, kDcEstCap) : 0;
    D.es = eok && ea >= kQ / 2 ? 1 + (e4 < 0) : 0;
    D.tq = tq;
    f_dcs.make(D, X);
    f_dcs.hint = eok ? cap(q4_round(ea), kDcMax) : -1;
    a = checked(f_dcs.code(X, ENC ? (v < 0 ? -v : v) : 0), 0, kDcMax);
    f_dcs.hint = -1;
    s->dc_prev[lc] = bitlen((int)a);
    if (!a) return 0;
    D.sz = bitlen((int)a);
    sel(f_dcs, D);
    neg = f_dcs.bit(TC_NODE, ENC && v < 0);
    s->dc_sgn[lc] = 1 + neg;
    return neg ? -(int)a : (int)a;
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

  // (level -9) skipping's variables, zt and ze (0 none)
  void skip_vars(int* zt, int* ze) {
    int a, b;
    if (mo.pict_type == PT_B) mo_direct(&mo, 0, 0);  // (what rc_skip_feat() predicts by)
    rc_skip_feat(&rec, &a, &b);
    *zt = a < 0 ? 0 : qlog1(a);
    *ze = b < 0 ? 0 : qlog1(b);
  }

  // how far an intra block's levels as coded (cl, by raster position) were from the estimate (E4)
  void est_account(const int* E4, const int* cl) {
    int i, e = 0;
    for (i = 0; i < BLK_COEFS; i++) e += abs(kQ * cl[i] - E4[i]);
    est_err += e;
    est_nb++;
  }

  // a block's coefficients: significance, levels, signs, escape modes (code_block.inc)
  void code_block(VState* s, Blk* b, int n, int intra, int dcvlc, int coded, int cls, int nctx, int qs,
                  int mvc, int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx* out, const BlkCtx* Xb);

  // coded block pattern, block 0 in bit 5 (the cbp family, the block its node). Contexts: the left
  // and above 8x8 blocks (in this macroblock or a neighbour), the size of the motion vector
  // differences (mvc, kNone for none), which components of the vector are sub-pel (frac, 4 for none),
  // the co-located macroblock of the last anchor VOP, the blocks coded so far, the quantiser (qs:
  // before this macroblock's dquant, which comes after the pattern), and lq: whether the left and
  // above macroblocks agree on having coded blocks (0), else 1 + whether the co-located one has some.
  // (level -9: cie, the intra estimate's AC coefficients per block; tq its trust; ctx, cem an inter
  // block's prediction's texture and its edge miss)
  int code_cbp(int cls3, const MBCtx* L, const MBCtx* A, const MBCtx* R, int mvc, int frac, int qs, int lq, int cbp,
               const int* cie, int tq, const int* ctx, const int* cem) {
    M_cbp_v D;
    int b, v = 0, n = 0;
    D.tq = tq;
    D.c3 = cls3;
    D.mvc = mvc;
    D.frac = frac;
    D.qs = qs;
    D.lq = lq;
    for (b = 0; b < MB_BLOCKS; b++) {
      int l, u, bit;
      if (b < MB_LUMA) {  // (b & 1: the right column, b & 2: the lower row)
        l = (b & 1) ? cbp_bit(v, b - 1) : L ? cbp_bit(L->cbp, b + 1) : kTriNone;
        u = (b & 2) ? cbp_bit(v, b - 2) : A ? cbp_bit(A->cbp, b + 2) : kTriNone;
      } else {
        l = L ? cbp_bit(L->cbp, b) : kTriNone;
        u = A ? cbp_bit(A->cbp, b) : kTriNone;
      }
      D.lu = tri2(l, u);
      D.ie = cie[b];
      D.tx = ctx[b];
      D.em = cem[b];
      D.rbit = R ? cbp_bit(R->cbp, b) : kTriNone;
      D.cnt = n;
      D.pat = v >> (MB_BLOCKS - b) & kPatMask;
      sel(f_cbp, D);
      bit = f_cbp.bit(b, ENC && cbp_bit(cbp, b));
      v |= bit << (MB_BLOCKS - 1 - b);
      n += bit;
    }
    return v;
  }

  // neighbouring blocks' coefficient counts (CAVLC's nC)
  static int nz_ctx(const MBCtx* me, const MBCtx* L, const MBCtx* A, int b) {
    int na, nb, m;
    if (b < MB_LUMA) {
      na = (b & 1) ? me->nz[b - 1] : L ? L->nz[b + 1] : -1;
      nb = (b & 2) ? me->nz[b - 2] : A ? A->nz[b + 2] : -1;
    } else {
      na = L ? L->nz[b] : -1;
      nb = A ? A->nz[b] : -1;
    }
    if (na < 0 && nb < 0) return 0;
    m = na < 0 ? nb : nb < 0 ? na : (na + nb + 1) >> 1;
    return 1 + cap(m, kCap);  // (0 none)
  }

  // A motion vector difference as coded, like CABAC's mvd: its magnitude in units of the VLC code
  // (so f_code doesn't change its statistics; the mvd family), its sign (mvg), the f_code residual
  // bits (mvd). Contexts: the neighbours' magnitudes (their sum, and each), the other component
  // (oth: of this vector for y, of the previous one for x, kNone for none), the co-located macroblock
  // of the last anchor VOP, and d: the difference that would give this vector the co-located one's
  // value (dok 0: none), whose magnitude is the magnitude's hint; with the decoder, td the difference
  // that would give it the vector the template matches best (tok 0: none), the hint then.
  int code_mvd(int kind, int comp, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, int oth, int dok, int d,
               int sp, int v, int* q, int tok, int td) {
    const int shift = fcode - 1, a = v < 0 ? -v : v;
    const int ad = (d < 0 ? -d : d) - 1, dq = d ? (ad >> shift) + 1 : 0;
    const int ds = !dok || !d ? 0 : d > 0 ? 1 : 2;
    const int tq = td ? (((td < 0 ? -td : td) - 1) >> shift) + 1 : 0, ts = !tok || !td ? 0 : td > 0 ? 1 : 2;
    M_mvd_v D;
    tcx X;
    int neg, r, i;
    i64 x;
    D.kind = kind;
    D.comp = comp;
    D.ns = mvq((L ? L->mva[comp] : 0) + (A ? A->mva[comp] : 0));
    D.oth = oth;
    D.rs = R ? mvq(R->mva[comp]) : kNone;
    D.dz = dok ? mvq(dq) : kNone;
    D.sp = sp;
    D.lq = L ? mvq(L->mva[comp]) : kNone;
    D.aq = A ? mvq(A->mva[comp]) : kNone;
    D.ds = ds;
    D.fc = shift;
    D.rbi = D.rsame = 0;
    D.tz = tok ? mvq(tq) : kNone;
    D.ts = ts;
    f_mvd.make(D, X);
    f_mvd.hint = ds ? dq : tok ? tq : -1;  // (the magnitude that repeats the co-located vector, or the template's)
    x = checked(f_mvd.code(X, ENC ? (a ? ((a - 1) >> shift) + 1 : 0) : 0), 0, MV_VLC_MAX);
    f_mvd.hint = -1;
    *q = (int)x;
    if (!x) return 0;
    {
      M_mvg_v G;
      G.kind = kind;
      G.comp = comp;
      G.ls = L ? L->mvs[comp] : 0;
      G.as = A ? A->mvs[comp] : 0;
      G.ds = ds;
      G.dz = D.dz;
      G.mag = mvq((int)x);
      G.hm = !ds ? HM_NONE : x < dq ? HM_BELOW : x == dq ? HM_EQUAL : HM_ABOVE;
      G.ts = ts;
      G.thm = !ts ? HM_NONE : x < tq ? HM_BELOW : x == tq ? HM_EQUAL : HM_ABOVE;
      sel(f_mvg, G);
      neg = f_mvg.bit(0, ENC && v < 0);
    }
    for (i = shift - 1, r = 0; i >= 0; i--) {  // residual bits, predicted by d's while they agree so far
      const int same = ds == 1 + neg && x == dq && ad >> (i + 1) == ((int)(x - 1) << (shift - i - 1) | r);
      D.rbi = i;
      D.rsame = same ? 1 + ((ad >> i) & 1) : 0;
      sel(f_mvd, D);
      r |= f_mvd.bit(TC_NODE + i, ENC && ((a - 1) >> i) & 1) << i;
    }
    x = ((x - 1) << shift | r) + 1;
    return neg ? -(int)x : (int)x;
  }

  // one vector (x, y) of kind kind, predicted by (px, py) from candidates with spreads (sx, sy) (-1:
  // none), with the co-located vector (tx, ty) if tok; tm: the template's vector (tmk[], tmx[], tmy[])
  // of that direction, or -1
  void code_mv(MB* m, int* mvi, int kind, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, MBCtx* me, int* sumq,
               int px, int py, int sx, int sy, int tok, int tx, int ty, int* mx, int* my, int tm = -1) {
    const int first = *mvi == 0, mk = tm >= 0 && tmk[tm];
    int v, q;
    v = code_mvd(kind, 0, fcode, L, A, R, first ? kNone : mvq(me->mva[0]), tok, mv_wrap(tx - px, fcode),
                 mv_spread(sx, fcode), m->mv[*mvi], &q, mk, mk ? mv_wrap(tmx[tm] - px, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *mx = mo_vector(px, v, fcode);
    if (first) { me->mva[0] = (U8)q; me->mvs[0] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
    v = code_mvd(kind, 1, fcode, L, A, R, mvq(q), tok, mv_wrap(ty - py, fcode), mv_spread(sy, fcode),
                 m->mv[*mvi], &q, mk, mk ? mv_wrap(tmy[tm] - py, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *my = mo_vector(py, v, fcode);
    if (first) { me->mva[1] = (U8)q; me->mvs[1] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
  }

  // the macroblock's kind and flags: a decision of the mbt family, its node, with the variables of
  // the neighbours of the kind it asks of
  int mbt_bit(int node, M_mbt_v& D, int b) {
    sel(f_mbt, D);
    return f_mbt.bit(node, ENC && b);
  }
  // how many of the macroblocks are of the kind k
  static int count_of(int k, const MBCtx* a, const MBCtx* b) { return (a && a->cls == k) + (b && b->cls == k); }

  // one macroblock record; in decoding mode m is filled in (code_mb.inc)
  void code_mb(const Ctx* cx, VState* s, MB* m, int idx);

  // One VOP (after its start code). W_VERIFY: parse it and write it again (into pb); W_ENCODE:
  // parse it and code the records; W_DECODE: decode the records and write the VOP (with its start
  // code). W_FAIL: it can't be (parsed, written: unsupported or damaged data), W_NOMEM: no memory.
  // (vop_walk.inc)
  int vop_walk(Ctx* cx, int mode, GB* gb, PB* pb);
};

// its longest functions, in files of their own
#include "code_block.inc"
#include "code_mb.inc"
#include "vop_walk.inc"
