// model_hdr.inc -- the model's fields (model.inc): the frame's and the subframes' headers (the hdr
// family), each value against the stream's last ones; the bytes kept as bytes and the bits after
// the subframes (raw); the container's chunks. (ac3pack's, with one history: a core stream has one
// kind of frame.)

template <int f_DEC, class LV>
struct ModelHdr {
  static constexpr int ENC = f_DEC == 0;
  static constexpr int PEND_MAX = 4096;
  Coder<f_DEC>* c;  // the run's coder (which the families have too)
  M_FAM(hdr)
  M_FAM(raw)
  Hist hist;
  // the frame's hdr values, kept as it ends: the field, its place, 1 + the value
  struct Pend {
    U8 fld, occ;
    S32 v;
  };
  Pend pend[PEND_MAX];
  int npend;
  U8 occ[F_N];  // each field's places so far in the frame
  int st, nch;  // the frame's: whether the field before kept its last value, how many changed
  int lastkind, lastkind2;  // the chunks before's kinds (1 + the kind, 0 none)
  int rawprev, rawprev2;    // the raw bytes before
  S64 rawat;                // the bytes of the chunk kept as bytes coded so far

  void cat(int k) {
    c->cat = k;
#ifdef DTSD_FSTAT
    if (k == CAT_RAW) c->fid = F_N;
#endif
  }  // what is coded next counts as category k (the statistics': -s)
  // x, checked: a value decompressed outside [lo, hi] is damaged data (the coder's error set: the
  // walk fails the frame), lo then
  INLINE S64 checked(S64 x, S64 lo, S64 hi) {
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

  // as the run starts (the history is zeroed pages: none)
  void hdr_start(void) {
    npend = 0;
    memset(occ, 0, sizeof occ);
    st = nch = 0;
    lastkind = lastkind2 = 0;
    rawprev = rawprev2 = 0;
    rawat = 0;
  }
  // a frame starts
  void unit_reset(void) {
    memset(occ, 0, sizeof occ);
    st = nch = 0;
  }
  // the unit's state after a value: whether it kept its last one (A, 1 + it; 0 none)
  INLINE int kept(S32 A, S64 v) {
    const int k = !A ? 0 : v == A - 1 ? 1 : 2;
    st = k;
    nch += k == 2;
    return k;
  }

  // A value v of family f's (hdr's or sid's: their first nodes are the same), its contexts X
  // selected: a flag where bit1, else whether it is c1 - 1, whether c2 - 1 (1 + those values, 0
  // none), else its difference from base (codes(), by sign context sc), in lo..hi
  template <class F>
  INLINE S64 code_val(F& f, const tcx& X, int bit1, S64 v, S32 c1, S32 c2, S64 base, u32 sc, S64 lo, S64 hi) {
    if (bit1) return f.bit(HN_BIT, ENC && v);
    if (c1 && f.bit(HN_SAME, ENC && v == c1 - 1)) return c1 - 1;
    if (c2 && c2 != c1 && f.bit(HN_SAME2, ENC && v == c2 - 1)) return c2 - 1;
    return checked(base + f.codes(X, ENC ? v - base : 0, sc), lo, hi);
  }

  // The hdr family's value of field fld (a flag where bit1, else in 0..hi), its last values A and
  // A2 (1 + them, 0 none), at place pos
  S64 hdr_code(int fld, int bit1, S64 v, S32 A, S32 A2, int pos, S64 hi) {
    M_hdr_v D;
    tcx X;
    D.fld = fld;
    D.lv = vvar(A, kVal);
    D.l2 = !A2 ? 0 : A2 == A ? 1 : 2;
    D.pos = imin(pos, 31);
    D.st = st;
    D.nch = imin(nch, 3);
    f_hdr.make(D, X);
    f_hdr.select(X);
    v = code_val(f_hdr, X, bit1, v, A, A2, A ? A - 1 : 0, (u32)fld, 0, hi);
    kept(A, v);
    return v;
  }
  // a field of the hdr family (local number fld) at place o, in 0..hi (a flag where hi is 1):
  // against the last two values there, kept as the frame ends
  S64 hdr_hist(int fld, int o, S64 hi, S64 v) {
    v = hdr_code(fld, hi == 1, v, hist.h1[fld][o], hist.h2[fld][o], o, hi);
    if (npend < PEND_MAX) pend[npend++] = Pend{(U8)fld, (U8)o, (S32)(v + 1)};
    return v;
  }
  // a field of the frame's or a subframe's header (fid), in 0..hi, at its next place in the frame
  S64 hdr_field(int fid, S64 hi, S64 v) {
    const int o = imin(occ[fid], HOCC - 1);
    if (occ[fid] < UINT8_MAX) occ[fid]++;
    return hdr_hist(fnums.hdr[fid], o, hi, v);
  }
  // the frame's hdr values into the history, as it ends
  void hdr_commit(void) {
    for (int i = 0; i < npend; i++) {
      const Pend& p = pend[i];
      hist.h2[p.fld][p.occ] = hist.h1[p.fld][p.occ];
      hist.h1[p.fld][p.occ] = p.v;
    }
    npend = 0;
  }

  // n raw bytes at p, of the kind code (RC_*): a bit at a time by its place in the byte's binary
  // tree (nodes 1-255), after the bytes before, by its place (at0 + its own, or from the end where
  // back), and the byte of the last field of the kind at its place (cl[i + clof], ncl of them; cl
  // none: none)
  void raw_bytes(pU8 p, size_t n, int code, pcU8 cl, long clof, long ncl, int back, S64 at0 = 0) {
    M_raw_v D;
    D.code = code;
    for (size_t i = 0; i < n; i++) {
      const long j = (long)i + clof;
      int x = 1;
      D.c1 = rawprev;
      D.c2 = rawprev2;
      D.at = tc_qlog(back ? (S64)(n - 1 - i) : at0 + (S64)i);
      D.cl = cl && j >= 0 && j < ncl ? 1 + cl[j] : 0;
      sel(f_raw, D);
      for (int k = BYTE_BITS - 1; k >= 0; k--) x = x * 2 + f_raw.bit(x, ENC && ((p[i] >> k) & 1));
      p[i] = (U8)(x - BYTE_VALUES);
      rawprev2 = rawprev;
      rawprev = p[i];
    }
  }

  // The bits after the subframes (nbits at p, the last byte's unused bits 0), or (kind 1) those
  // before the extension, or (2-4) XXCH's other bits (its headers' after their fields, its channel
  // set's after its subframes): a flag whether they are all 0, else how many of their last bytes
  // are not (from the first that isn't), and those bytes, against the last ones' of the kind at the
  // same places from the end
  void code_tail(pU8 p, int nbits, int kind) {
    const int nb = (nbits + 7) / 8, zf = NHFRAME + (kind ? HX_GAPZ : HX_TAILZ), nf = NHFRAME + (kind ? HX_GAPNZ : HX_TAILNZ);
    const int o = kind ? kind - 1 : 0;  // (the hdr fields' place: each kind its own)
    U8* const last = hist.tail[kind];
    S32& lastn = hist.tailn[kind];
    int nz = 0;
    if (!nb) return;
    if (ENC) {
      int i = 0;
      while (i < nb && !p[i]) i++;
      nz = nb - i;
    }
    if (hdr_hist(zf, o, 1, nz == 0)) {
      if (!ENC) memset(p, 0, (size_t)nb);
      return;
    }
    nz = (int)checked(hdr_hist(nf, o, 0xFFFF, nz), 1, nb);
    if (!ENC) memset(p, 0, (size_t)(nb - nz));
    raw_bytes(p + nb - nz, (size_t)nz, kind ? RC_GAP : RC_TAIL, last, lastn - nz, lastn, 1);
    memcpy(last, p + nb - nz, (size_t)nz);
    lastn = nz;
  }

  // the container: a chunk's kind (CK_*), the length of one kept as bytes, its bytes
  int code_kind(int k) {
    cat(CAT_RAW);
    k = (int)hdr_code(NHFRAME + HX_KIND, 0, k, lastkind, lastkind2, 0, CK_N - 1);
    lastkind2 = lastkind;
    lastkind = k + 1;
    return k;
  }
  size_t code_raw_len(size_t n) {
    cat(CAT_RAW);
    rawat = 0;
    return (size_t)hdr_code(NHFRAME + HX_RAWLEN, 0, (S64)n, 0, 0, 0, kRawLenMax);
  }
  // n bytes at p of a chunk of the kind k (CK_RAW, CK_BAD), which may come a piece at a time (at:
  // the bytes before in the chunk)
  void code_raw_bytes(pU8 p, size_t n, int k) {
    cat(CAT_RAW);
    raw_bytes(p, n, k == CK_BAD ? RC_BAD : RC_CHUNK, nullptr, 0, 0, 0, rawat);
    rawat += (S64)n;
  }
};
