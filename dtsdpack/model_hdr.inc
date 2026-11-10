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
  U8 tq[MAX_FRAME];         // (code_tail()'s bytes)
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
  void raw_bytes(pU8 p, size_t n, int code, pcU8 cl, long clof, long ncl, int back, S64 at0 = 0, int kb0 = BYTE_BITS) {
    M_raw_v D;
    D.code = code;
    for (size_t i = 0; i < n; i++) {
      const long j = (long)i + clof;
      const int kb = i ? BYTE_BITS : kb0;  // (the first byte's bits: its high ones 0, not coded)
      int x = 1 << (BYTE_BITS - kb);
      D.c1 = rawprev;
      D.c2 = rawprev2;
      D.at = tc_qlog(back ? (S64)(n - 1 - i) : at0 + (S64)i);
      D.cl = cl && j >= 0 && j < ncl ? 1 + cl[j] : 0;
      sel(f_raw, D);
      for (int k = kb - 1; k >= 0; k--) x = x * 2 + f_raw.bit(x, ENC && ((p[i] >> k) & 1));
      p[i] = (U8)(x - BYTE_VALUES);
      rawprev2 = rawprev;
      rawprev = p[i];
    }
  }

  // The bits after the subframes (nbits at p, the last byte's unused bits 0), or (kind 1) those
  // before the extension, or (2-4) XXCH's other bits (its headers' after their fields, its channel
  // set's after its subframes): as the bytes they end in (their last bit a byte's last: the frame's
  // bytes, which their patterns go by, as the encoder's words before the extension's sync word and
  // its fill to a 32-bit boundary do), a flag whether they are all 0, else how many of those bytes
  // are not (from the first that isn't), how many of those end them as 0s, and the bytes between,
  // against the last ones' of the kind at the same places from the end (with auxiliary data, a0
  // its sync word's byte: from it)
  // a tail of the kind (nbits, its bytes q, nb of them, to TC_B) the cache's first, the last dropped
  void tail_cache_add(int kind, int nbits, const U8* q, int nb) {
    for (int j = Hist::TC_K - 1; j > 0; j--) hist.tcn[kind][j] = hist.tcn[kind][j - 1], memcpy(hist.tcb[kind][j], hist.tcb[kind][j - 1], Hist::TC_B);
    hist.tcn[kind][0] = nbits;
    memset(hist.tcb[kind][0], 0, Hist::TC_B);
    memcpy(hist.tcb[kind][0], q, (size_t)nb);
  }
  // The last 8 frames coded as records, as their bytes (frame_bytes(), as each is coded): an encoder
  // that leaves the bits before the extension's sync word as an earlier frame had them at the same
  // place (`ES 6.1 16bit.dts`'s: of a ring of 8 buffers, mostly the frame 8 before) makes them
  // those of a frame before (code_tail())
  U8 pfb[8][MAX_FRAME];
  S32 pfn[8];
  U32 pfk;
  U8 tcand[8][32];
  void frame_bytes(const U8* b, int n) {
    const int i = (int)(pfk & 7);
    memcpy(pfb[i], b, (size_t)n);
    pfn[i] = n;
    pfk++;
  }
  // the nbits bits of b at bit pos, as code_tail() has them (ending with c's last byte)
  static void bits_at(const U8* b, S64 pos, int nbits, U8* c) {
    const int nb = (nbits + 7) / 8, sh = 8 * nb - nbits;
    memset(c, 0, (size_t)nb);
    for (int j = 0; j < nbits; j++) {
      const S64 at = pos + j;
      const int k = sh + j;
      c[k >> 3] |= (U8)(((b[at >> 3] >> (7 - (at & 7))) & 1) << (7 - (k & 7)));
    }
  }
  // the nbits bits at p, the frame's from bit bitpos, of the kind (0 after the subframes, 1 before
  // the extension's sync word, 2-4 XXCH's: hdr's places by it)
  void code_tail(pU8 p, int nbits, int kind, int a0, S64 bitpos) {
    const int nb = (nbits + 7) / 8, zf = NHFRAME + (kind ? HX_GAPZ : HX_TAILZ), nf = NHFRAME + (kind ? HX_GAPNZ : HX_TAILNZ);
    // (the hdr fields' place: each kind its own; the bits after the subframes' and those before the
    // extension's, by their bits to the next 32-bit boundary: their fills' odds go by them)
    const int o = kind > 1 ? kind - 1 : (kind ? 4 : 0) + ((nbits & 31) >> 1);
    const int sh = 8 * nb - nbits, k0 = BYTE_BITS - sh;  // (the first byte's bits)
    U8* const last = hist.tail[kind];
    S32& lastn = hist.tailn[kind];
    U8* const q = tq;
    int nz = 0;
    if (!nb) return;
    if (ENC) {  // (the bits shifted to end with a byte)
      for (int i = 0; i < nb; i++) q[i] = (U8)(((i ? p[i - 1] << 8 : 0) | p[i]) >> sh);
      int i = 0;
      while (i < nb && !q[i]) i++;
      nz = nb - i;
    }
    if (hdr_hist(zf, o, 1, nz == 0)) {
      if (!ENC) memset(p, 0, (size_t)nb);
      return;
    }
    const int cache = nb <= Hist::TC_B;
    // (the bytes before the extension's sync word as a frame before had them at their place, the bits
    // before those 0: frames 8, then 1 to 7 back, a flag each, at the zero flag's places from 48,
    // where they aren't 0 or one before)
    if (kind == 1 && nb <= 32) {
      static const S8 BK[8] = {8, 1, 2, 3, 4, 5, 6, 7};
      int nc = 0;
      for (int t = 0; t < 8; t++) {
        const int b = BK[t], i = (int)((pfk - b) & 7);
        if ((U32)b > pfk || bitpos + nbits > (S64)pfn[i] * 8) continue;
        U8* const c = tcand[nc];
        bits_at(pfb[i], bitpos, nbits, c);
        if (nbits & 7) c[0] = 0;  // (the bits before its bytes, the rest of the core's last byte: 0)
        int skip = 1;
        for (int j = 0; j < nb && skip; j++) skip = !c[j];
        for (int u = 0; u < nc && !skip; u++) skip = !memcmp(tcand[u], c, (size_t)nb);
        if (skip) continue;
        nc++;
        if (!hdr_hist(zf, 48 + b, 1, ENC && !memcmp(c, q, (size_t)nb))) continue;
        if (!ENC) memcpy(q, c, (size_t)nb);
        int k = 0;
        while (k < nb && !q[k]) k++;
        nz = nb - k;
        memcpy(last, q + nb - nz, (size_t)nz);  // (the state the bytes' coding leaves)
        lastn = nz;
        if (cache) tail_cache_add(kind, nbits, q, nb);
        if (!ENC)
          for (int j = 0; j < nb; j++) p[j] = (U8)((q[j] << sh) | (j + 1 < nb && sh ? q[j + 1] >> (8 - sh) : 0));
        return;
      }
    }
    // (a tail of 8 bytes or more the same as one of the last ones of its kind and size, the last
    // first: which, a flag for each, at the zero flag's places from 32; the auxiliary data's, its
    // count, a fill to its 32-bit boundary and the same bytes, makes a few, one by the fill's size)
    for (int k = 0; cache && nb >= 8 && k < Hist::TC_K; k++) {
      if (hist.tcn[kind][k] != nbits) continue;
      U8* const cb = hist.tcb[kind][k];
      if (!hdr_hist(zf, 32 + imin(k, 15), 1, ENC && !memcmp(cb, q, (size_t)nb))) continue;
      if (!ENC) memcpy(q, cb, (size_t)nb);
      for (int j = k; j > 0; j--) hist.tcn[kind][j] = hist.tcn[kind][j - 1], memcpy(hist.tcb[kind][j], hist.tcb[kind][j - 1], Hist::TC_B);
      hist.tcn[kind][0] = nbits, memcpy(hist.tcb[kind][0], q, (size_t)nb);
      int i = 0;
      while (i < nb && !q[i]) i++;
      nz = nb - i;
      memcpy(last, q + nb - nz, (size_t)nz);  // (the state the bytes' coding leaves)
      lastn = nz;
      if (!kind) hist.tailsync = a0 >= nb - nz ? a0 - (nb - nz) : -1;
      if (!ENC)
        for (int j = 0; j < nb; j++) p[j] = (U8)((q[j] << sh) | (j + 1 < nb && sh ? q[j + 1] >> (8 - sh) : 0));
      return;
    }
    nz = (int)checked(hdr_hist(nf, o, 0xFFFF, nz), 1, nb);
    int tz = 0;  // (and how many of those end it as 0s: a 32-bit word of them, the fill's)
    if (ENC)
      while (tz < nz - 1 && !q[nb - 1 - tz]) tz++;
    tz = (int)checked(hdr_hist(NHFRAME + (kind ? HX_GAPTZ : HX_TAILTZ), o, 0xFFFF, tz), 0, nz - 1);
    if (!ENC) memset(q, 0, (size_t)nb);
    // (against the last's at the same places from the end; an aux tail's from its sync word, where
    // the last had one: a0, its byte here)
    S32& ls = hist.tailsync;
    const long clof = !kind && a0 >= 0 && ls >= 0 ? (long)ls + (nb - nz) - a0 : (long)lastn - nz;
    raw_bytes(q + nb - nz, (size_t)(nz - tz), kind ? RC_GAP : RC_TAIL, last, clof, lastn, 1, 0, nz == nb ? k0 : BYTE_BITS);
    memcpy(last, q + nb - nz, (size_t)nz);
    lastn = nz;
    if (!kind) ls = a0 >= nb - nz ? a0 - (nb - nz) : -1;
    if (cache) tail_cache_add(kind, nbits, q, nb);
    if (!ENC)  // (back as they were, from the first byte)
      for (int i = 0; i < nb; i++) p[i] = (U8)((q[i] << sh) | (i + 1 < nb && sh ? q[i + 1] >> (8 - sh) : 0));
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
