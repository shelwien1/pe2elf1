// model_hdr.inc -- the model's headers (model.inc): the chunks kept as bytes, the VOP and video
// packet headers, and what follows a VOP's last macroblock; the hdr and raw families.

template <int f_DEC, class LV>
struct ModelHdr : ModelCore {
  static constexpr int ENC = f_DEC == 0;
  Coder<f_DEC>* c;  // the run's coder (which the families have too)
  M_FAM(hdr)
  M_FAM(raw)
  Last last;  // the values before

  void cat(int k) { c->cat = k; }  // what is coded next counts as category k (the statistics': -s)
  // x, checked: a value decompressed outside [lo, hi] is damaged data (the coder's error set: the
  // walk fails the chunk), lo then
  INLINE i64 checked(i64 x, i64 lo, i64 hi) {
    if (UNLIKELY(x < lo || x > hi)) {
      c->err = 1;
      return lo;
    }
    return x;
  }

  // as the run starts
  void hdr_start(void) {
    int i;
    memset(&last, 0, sizeof(last));
    last.tib = TIB_MIN;
    for (i = 0; i < PT_N; i++) { last.quant[i] = kQuantStart; last.fcode[i] = FCODE_MIN; }
    last.bcode = FCODE_MIN;
  }

  // the hdr family, a field (fld) of the VOP of type ty (0 I, 1 P, 2 B, 3 S, TY_NONE none) at a
  // time, whose last value is p1
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
    cat(CAT_RAW);
    k = (int)checked(hdr_val(F_KIND, TY_NONE, last.kind, 0, k), CK_END, CK_RAW);
    return last.kind = k;
  }

  // a chunk kept as bytes: its length, then the bytes (the raw family: the bytes before, the
  // chunk's start code, the place in it), which may come a piece at a time
  size_t code_raw_len(size_t n) {
    cat(CAT_RAW);
    last.rawat = last.rawcode = 0;
    last.rawhd = 0;
    return (size_t)checked(hdr_val(F_RAWLEN, TY_NONE, 0, 0, (i64)n), 0, kRawLenMax);
  }
  void code_raw_bytes(U8* p, size_t n) {
    M_raw_v D;
    size_t i;
    cat(CAT_RAW);
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

  void code_vop_hdr(const Ctx& cx, VopHdr& h) {
    const int ty0 = last.type ? last.type - PT_I : TY_NONE;
    int i, d, mask, ty;
    cat(CAT_HDR);
    h.type = PT_I + (int)checked(hdr_val(F_VTYPE, ty0, ty0, 0, h.type - PT_I), 0, PT_S - PT_I);
    ty = h.type - PT_I;
    h.mtb = (int)checked(hdr_val(F_MTB, ty, last.mtb, 0, h.mtb), 0, RUN_MAX);
    h.tib = (int)checked(hdr_val(F_TIB, ty, last.tib, 0, h.tib), TIB_MIN, TIB_MAX);
    mask = (int)((1u << h.tib) - 1);
    d = (h.tinc - last.tinc) & mask;
    if (hdr_flag(F_TSAME, ty, 0, 0, d == (last.dinc[h.type] & mask))) d = last.dinc[h.type] & mask;
    else d = (int)checked(hdr_val(F_TINC, ty, 0, 0, d), 0, mask);
    h.tinc = (last.tinc + d) & mask;
    last.type = h.type;
    last.mtb = h.mtb;
    last.tib = h.tib;
    last.tinc = h.tinc;
    last.dinc[h.type] = d;
    h.coded = hdr_flag(F_CODED, ty, 0, 0, h.coded);
    if (!ENC) h.clear();
    if (!h.coded) return;
    if (cx.vop_has_rnd(h.type)) last.rnd = h.rnd = hdr_flag(F_RND, ty, last.rnd, 0, h.rnd);
    h.ncplx = cx.vop_ncplx(h.type);
    for (i = 0; i < h.ncplx; i++)
      h.cplx[i] = (char)('0' + hdr_flag(F_CPLX, ty, 0, cap(i, kPosCap), h.cplx[i] == '1'));
    last.dcthr = h.dcthr = (int)checked(hdr_val(F_DCTHR, ty, last.dcthr, 0, h.dcthr), 0, DCTHR_MAX);
    if (cx.interlaced) {
      last.tff = h.tff = hdr_flag(F_TFF, ty, last.tff, 0, h.tff);
      last.alt = h.alt = hdr_flag(F_ALT, ty, last.alt, 0, h.alt);
    }
    if (cx.vop_gmc(h.type))
      for (i = 0; i < 2 * cx.warp_points; i++)
        h.warp[i] = (int)checked(hdr_sval(F_WARP, ty, 0, i & 1, h.warp[i]), -WARP_MAX, WARP_MAX);
    h.quant = last.quant[h.type] + (int)hdr_sval(F_QUANT, ty, 0, 0, h.quant - last.quant[h.type]);
    h.quant = (int)checked(h.quant, QS_MIN, QUANT_MAX);
    last.quant[h.type] = h.quant;
    if (h.type != PT_I) {
      h.fcode = (int)checked(hdr_val(F_FCODE, ty, last.fcode[h.type], 0, h.fcode), FCODE_MIN, FCODE_MAX);
      last.fcode[h.type] = h.fcode;
    }
    if (h.type == PT_B) {
      h.bcode = (int)checked(hdr_val(F_BCODE, ty, last.bcode, 0, h.bcode), FCODE_MIN, FCODE_MAX);
      last.bcode = h.bcode;
    }
  }

  void code_vp(const Ctx& cx, VState& s, VpHdr& k, int expect) {
    Vop& v = s.v;
    const int ty = v.type - PT_I;
    int i;
    cat(CAT_HDR);
    k.st = (int)checked(hdr_val(F_VPST, ty, 0, 0, k.st), 0, RUN_MAX);
    if (hdr_flag(F_VPMBSAME, ty, 0, 0, k.mbnum == expect)) k.mbnum = expect;
    else k.mbnum = (int)checked(hdr_val(F_VPMB, ty, 0, 0, k.mbnum), 0, MAX_MBS);
    k.quant = (int)checked(v.qscale + hdr_sval(F_VPQ, ty, 0, 0, k.quant - v.qscale), 0, QUANT_MAX);
    k.hec = hdr_flag(F_HEC, ty, 0, 0, k.hec);
    if (k.quant) v.qscale = k.quant;
    v.qscale = clip(v.qscale, QS_MIN, QS_MAX);
    if (!ENC) k.clear();
    if (!k.hec) return;
    k.mtb = (int)checked(hdr_val(F_HMTB, ty, 0, 0, k.mtb), 0, RUN_MAX);
    k.tinc = (int)checked(hdr_val(F_HTINC, ty, 0, 0, k.tinc), 0, (1 << v.tib) - 1);
    k.type = (int)checked(hdr_val(F_HTYPE, ty, 0, 0, k.type), 0, PT_S - PT_I);
    k.dcthr = (int)checked(hdr_val(F_HDCTHR, ty, 0, 0, k.dcthr), 0, DCTHR_MAX);
    if (v.gmc)
      for (i = 0; i < 2 * cx.warp_points; i++)
        k.warp[i] = (int)checked(hdr_sval(F_HWARP, ty, 0, i & 1, k.warp[i]), -WARP_MAX, WARP_MAX);
    if (v.type != PT_I) k.fcode = (int)checked(hdr_val(F_HFCODE, ty, 0, 0, k.fcode), FCODE_MIN, FCODE_MAX);
    if (v.type == PT_B) k.bcode = (int)checked(hdr_val(F_HBCODE, ty, 0, 0, k.bcode), FCODE_MIN, FCODE_MAX);
  }

  // what follows the last macroblock: nb bits up to the byte boundary, then *n bytes
  int code_tail(int nb, int* bits, int* n, U8* bytes, int max) {
    int i, canon = 0;
    cat(CAT_HDR);
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
};
