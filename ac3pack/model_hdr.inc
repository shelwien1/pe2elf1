// model_hdr.inc -- the model's fields (model.inc): the frame's header and trailer (the hdr family)
// and the blocks' side information (sid), each value against its substream's last ones; the bytes
// kept as bytes and the raw fields (raw); the bits after the last block; the container's chunks.

template <int f_DEC, class LV>
struct ModelHdr {
  static constexpr int ENC = f_DEC == 0;
  static constexpr int PEND_MAX = 1024, SPEND_MAX = 2048;
  Coder<f_DEC>* c;  // the run's coder (which the families have too)
  M_FAM(hdr)
  M_FAM(sid)
  M_FAM(raw)
  Hist hist[NHIST];
  int h;   // the frame's history (H_GLOBAL until its substream is known)
  int sk;  // hdr's sk: its substream (4 not known yet)
  int strmtyp;
  // the frame's hdr values, kept as it ends: the history, the field, its place, 1 + the value
  struct Pend {
    U8 h, fld, occ;
    S32 v;
  };
  Pend pend[PEND_MAX];
  int npend;
  // the block's sid values (1 + the value, 0 none), and the places to keep as it ends
  S32 scur[NSF][SOCC];
  struct SPend {
    U8 fld, occ;
  };
  SPend spend[SPEND_MAX];
  int nspend;
  U8 occ[F_N];     // each field's places so far in its unit (the frame's header, a block, the trailer)
  S8 sstate[NSF];  // sid: whether the field's place before in the block kept its last value (st's values)
  int st, nch;     // the unit's: whether the field before kept its last value, how many changed
  int blk;         // the block
  int lastkind, lastkind2;  // the chunks before's kinds (1 + the kind, 0 none)
  int rawprev, rawprev2;    // the raw bytes before
  i64 rawat;                // the bytes of the chunk kept as bytes coded so far

  void cat(int k) { c->cat = k; }  // what is coded next counts as category k (the statistics': -s)
  // x, checked: a value decompressed outside [lo, hi] is damaged data (the coder's error set: the
  // walk fails the frame), lo then
  INLINE i64 checked(i64 x, i64 lo, i64 hi) {
    if (UNLIKELY(x < lo || x > hi)) {
      c->err = 1;
      return lo;
    }
    return x;
  }
  // family f's contexts from its variables D, selected: what its bit()s code with
  template <class F, class V>
  static INLINE void sel(F& f, const V& D) {
    tcx X;
    f.make(D, X);
    f.select(X);
  }

  // as the run starts (the histories are zeroed pages: none)
  void hdr_start(void) {
    h = H_GLOBAL;
    sk = 4;
    strmtyp = 0;
    npend = nspend = 0;
    memset(occ, 0, sizeof occ);
    memset(sstate, 0, sizeof sstate);
    st = nch = blk = 0;
    lastkind = lastkind2 = 0;
    rawprev = rawprev2 = 0;
    rawat = 0;
  }
  // a unit starts: the frame's header, a block, the trailer
  void unit_reset(void) {
    memset(occ, 0, sizeof occ);
    st = nch = 0;
  }
  // the unit's state after a value: whether it kept its last one (A, 1 + it; 0 none)
  INLINE int kept(S32 A, i64 v) {
    const int k = !A ? 0 : v == A - 1 ? 1 : 2;
    st = k;
    nch += k == 2;
    return k;
  }

  // The hdr family's value of field fld (a flag where bit1, else in 0..hi), its last values A and
  // A2 (1 + them, 0 none), at place pos
  i64 hdr_code(int fld, int bit1, i64 v, S32 A, S32 A2, int pos, i64 hi) {
    M_hdr_v D;
    tcx X;
    D.fld = fld;
    D.lv = vvar(A, kVal);
    D.l2 = !A2 ? 0 : A2 == A ? 1 : 2;
    D.pos = cap(pos, 31);
    D.st = st;
    D.nch = cap(nch, 3);
    D.sk = sk;
    f_hdr.make(D, X);
    f_hdr.select(X);
    if (bit1) v = f_hdr.bit(HN_BIT, ENC && v);
    else if (A && f_hdr.bit(HN_SAME, ENC && v == A - 1)) v = A - 1;
    else if (A2 && A2 != A && f_hdr.bit(HN_SAME2, ENC && v == A2 - 1)) v = A2 - 1;
    else {
      const i64 base = A ? A - 1 : 0;
      v = checked(base + f_hdr.codes(X, ENC ? v - base : 0, (u32)fld), 0, hi);
    }
    kept(A, v);
    return v;
  }
  // a frame's field of the hdr family (local number fld) at place o of history hh, n bits: against
  // the history's last two values there, kept as the frame ends
  int hdr_hist(int fld, int o, int hh, int n, int v) {
    Hist& H = hist[hh];
    v = (int)hdr_code(fld, n == 1, v, H.h1[fld][o], H.h2[fld][o], o, ((i64)1 << n) - 1);
    if (npend < PEND_MAX) pend[npend++] = Pend{(U8)hh, (U8)fld, (U8)o, v + 1};
    return v;
  }
  // a field of the frame's header or trailer (fid) at its next place: the frame's first ones
  // (bsid, strmtyp, substreamid) against the frames' before, whatever their substream
  int hdr_field(int fid, int n, int v) {
    const int o = cap(occ[fid], HOCC - 1);
    if (occ[fid] < UINT8_MAX) occ[fid]++;
    const int g = fid == F_bsid || fid == F_strmtyp || fid == F_substreamid;
    return hdr_hist(fnums.hdr[fid], o, g ? H_GLOBAL : h, n, v);
  }
  // the frame's hdr values into the histories, as it ends
  void hdr_commit(void) {
    for (int i = 0; i < npend; i++) {
      const Pend& p = pend[i];
      Hist& H = hist[p.h];
      H.h2[p.fld][p.occ] = H.h1[p.fld][p.occ];
      H.h1[p.fld][p.occ] = p.v;
    }
    npend = 0;
  }

  // A block's field of the sid family (fid) at its next place, n bits (in lo..hi): against the last
  // block's value there and the field's place before in the block
  int sid_code(int fid, int n, int v, int lo, int hi) {
    const int fld = fnums.sid[fid], o = cap(occ[fid], SOCC - 1);
    if (occ[fid] < UINT8_MAX) occ[fid]++;
    const S32 A = hist[h].s1[fld][o], B = o ? scur[fld][o - 1] : 0;
    M_sid_v D;
    tcx X;
    D.fld = fld;
    D.lv = vvar(A, kVal);
    D.pos = cap(o, 31);
    D.st = st;
    D.nch = cap(nch, 3);
    D.sk = sk;
    D.blk = blk;
    D.pq = vvar(B, kVal);
    D.pr = o ? sstate[fld] : 0;
    D.lb = !A || !B ? 0 : A == B ? 1 : 2;
    f_sid.make(D, X);
    f_sid.select(X);
    if (n == 1) v = f_sid.bit(SN_BIT, ENC && v);
    else if (A && f_sid.bit(SN_SAME, ENC && v == A - 1)) v = A - 1;
    else if (B && B != A && f_sid.bit(SN_SAMEP, ENC && v == B - 1)) v = B - 1;
    else {
      const int base = A ? A - 1 : B ? B - 1 : 0;
      v = (int)checked(base + f_sid.codes(X, ENC ? v - base : 0, (u32)fld), lo, hi);
    }
    sstate[fld] = (S8)kept(A, v);
    if (!scur[fld][o] && nspend < SPEND_MAX) spend[nspend++] = SPend{(U8)fld, (U8)o};
    scur[fld][o] = v + 1;
    return v;
  }
  // the block's sid values into the history, as it ends
  void sid_commit(void) {
    Hist& H = hist[h];
    for (int i = 0; i < nspend; i++) {
      const SPend& p = spend[i];
      H.s1[p.fld][p.occ] = scur[p.fld][p.occ];
      scur[p.fld][p.occ] = 0;
    }
    nspend = 0;
  }

  // n raw bytes at p, of the kind code (RC_*): a bit at a time by its place in the byte's binary
  // tree (nodes 1-255), after the bytes before, by its place (at0 + its own, or from the end where
  // back), and the byte of the last field of the kind at its place (cl[i + clof], ncl of them; cl
  // none: none)
  void raw_bytes(pU8 p, size_t n, int code, pcU8 cl, long clof, long ncl, int back, i64 at0 = 0) {
    M_raw_v D;
    D.code = code;
    D.sk = sk;
    for (size_t i = 0; i < n; i++) {
      const long j = (long)i + clof;
      int x = 1;
      D.c1 = rawprev;
      D.c2 = rawprev2;
      D.at = tc_qlog(back ? (i64)(n - 1 - i) : at0 + (i64)i);
      D.cl = cl && j >= 0 && j < ncl ? 1 + cl[j] : 0;
      sel(f_raw, D);
      for (int k = BYTE_BITS - 1; k >= 0; k--) x = x * 2 + f_raw.bit(x, ENC && ((p[i] >> k) & 1));
      p[i] = (U8)(x - BYTE_VALUES);
      rawprev2 = rawprev;
      rawprev = p[i];
    }
  }
  // a raw field of the frame (n bits at p, the last byte's unused bits 0), against the last of
  // its kind (last, *nlast bytes of room max): which it is then
  void raw_field(pU8 p, int nbits, int code, pU8 last, S16* nlast, int max) {
    const int n = (nbits + 7) / 8;
    raw_bytes(p, (size_t)n, code, last, 0, *nlast, 0);
    if (n <= max) {
      memcpy(last, p, (size_t)n);
      *nlast = (S16)n;
    }
  }

  // The bits after the last block (nbits at p, the last byte's unused bits 0): a flag whether they
  // are all 0, else how many of their last bytes are not (from the first that isn't), and those
  // bytes, against the last tail's at the same places from the end
  void code_tail(pU8 p, int nbits) {
    const int nb = (nbits + 7) / 8;
    Hist& H = hist[h];
    int nz = 0;
    if (!nb) return;
    if (ENC) {
      int i = 0;
      while (i < nb && !p[i]) i++;
      nz = nb - i;
    }
    if (hdr_hist(NHFRAME + HX_TAILZ, 0, h, 1, nz == 0)) {
      if (!ENC) memset(p, 0, (size_t)nb);
      return;
    }
    nz = (int)checked(hdr_hist(NHFRAME + HX_TAILNZ, 0, h, 16, nz), 1, nb);
    if (!ENC) memset(p, 0, (size_t)(nb - nz));
    raw_bytes(p + nb - nz, (size_t)nz, RC_TAIL, H.tail, H.tailn - nz, H.tailn, 1);
    memcpy(H.tail, p + nb - nz, (size_t)nz);
    H.tailn = nz;
  }

  // the container: a chunk's kind (CK_*), the length of one kept as bytes, its bytes
  int code_kind(int k) {
    cat(CAT_RAW);
    k = (int)checked(hdr_code(NHFRAME + HX_KIND, 0, k, lastkind, lastkind2, 0, CK_N - 1), CK_END, CK_N - 1);
    lastkind2 = lastkind;
    lastkind = k + 1;
    return k;
  }
  size_t code_raw_len(size_t n) {
    cat(CAT_RAW);
    rawat = 0;
    return (size_t)checked(hdr_code(NHFRAME + HX_RAWLEN, 0, (i64)n, 0, 0, 0, kRawLenMax), 0, kRawLenMax);
  }
  // n bytes at p of a chunk of the kind k (CK_RAW, CK_BAD), which may come a piece at a time (at:
  // the bytes before in the chunk)
  void code_raw_bytes(pU8 p, size_t n, int k) {
    cat(CAT_RAW);
    raw_bytes(p, n, k == CK_BAD ? RC_BAD : RC_CHUNK, nullptr, 0, 0, 0, rawat);
    rawat += (i64)n;
  }
};
