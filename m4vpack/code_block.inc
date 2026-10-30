// code_block.inc -- model_res.inc's ModelRes::code_block(): a block's coefficients (included in
// ModelRes's body).

// Coefficients of a block, coded like H.264's CABAC residual: significance map in scan order
// (the sig family), then the levels in reverse order (lvl), signs (sgn), and the escape modes
// where they aren't the default (esc). The contexts: the neighbouring blocks' coefficient counts
// (CAVLC's nC), the coefficients found so far, and the left and above blocks' coefficients at the
// same position; the significance flags before (the last 8 by position, 16 hashed), the
// frequency neighbours left and above in the block; for the levels, the magnitudes coded before
// them and the neighbours right and below; for the signs, the signs coded before them and those
// of the frequency neighbours (a block's coefficients come in patterns, as an edge leaves them).
// cls: block class, nctx: neighbours' counts, qs: quantiser, mvc: motion vector size, rnz: the
// co-located block's count; Lb, Ab: left and above blocks or 0; out (which may be Ab) gets this one;
// Xb: for a Cr block, the Cb block of its macroblock (0 else)
  void code_block(VState& s, Blk& b, int n, int intra, int dcvlc, int coded, int cls, int nctx, int qs, int mvc,
                  int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx& out, const BlkCtx* Xb) {
    const RL& rl = intra ? rl_intra : rl_inter;
    const U8* sc = LV::recon && rec.on ? rec.scan(n, intra) : s.scan;  // (-9: an AC predicted block's own)
    BlkCtx o;
    U8 rm[BLK_COEFS];   // coefficients coded so far by raster position: 1 in the significance map, then magnitudes
    U8 nzr[BLK_COEFS];  // the significance map by raster position
    int pos[BLK_COEFS], lev[BLK_COEFS], k = 0, i, j, start, prev;
    U32 sh = 0;  // the significance flags so far, the last in bit 0
    // (level -9) E4: an intra block's estimate, the levels coded (the prediction taken off) by raster
    // position, quarter levels (eon), its trust tq; or an inter block's levels its edges ask for
    // (pon), its prediction's texture ptx and edge miss pem
    int E4[BLK_COEFS], eon = 0, tq = 0, ptx = 0, pem = 0, pon = 0, PF[BLK_COEFS], ptd = RC_TD_NONE;
    memset(&o, 0, sizeof(o));
    memset(rm, 0, sizeof(rm));
    o.last = o.dcs = -1;
    if (LV::recon && intra && est.ok) {
      int P[BLK_COEFS], dir;
      rec.intra_pred(n, &dir, P);
      for (i = 0; i < BLK_COEFS; i++) E4[i] = rec.est_q4(est, n, i) - kQ * P[i];
      eon = 1;
      tq = est_trust();
    }
    if (LV::recon && rec.on && !intra && coded) {
      rec.pred_feat(n, 1, n < MB_LUMA && rec.interlaced_dct, &ptx, &pem);
      ptx = qlog1(ptx);
      pem = pem < 0 ? 0 : qlog1(pem);
      pon = rec.edge_proj(n, E4);
      rec.pred_spec(n, n < MB_LUMA && rec.interlaced_dct, PF, &ptd);
    }
    if (dcvlc) {
      cat(CAT_DC);
      b.dc = code_dc(s, n >= MB_LUMA, b.dc, Lb, Ab, coded, qs, eon, eon ? E4[0] : 0, tq);
      o.dcs = (S8)bitlen(b.dc);
      o.dcsg = b.dc < 0;
    } else if (!ENC)
      b.dc = 0;
    if (!coded) {
      b.n = 0;
      out = o;
      if (eon) {
        int cl[BLK_COEFS];
        memset(cl, 0, sizeof cl);
        cl[0] = dcvlc ? b.dc : 0;
        est_account(E4, cl);
      }
      return;
    }
    start = dcvlc ? 1 : 0;
    if (dcvlc) mvc = kMvcIntra + o.dcs;  // (an intra block has no vector: the DC difference's size)
    if (ENC)
      for (j = 0, prev = start - 1; j < b.n; j++) { pos[j] = prev += b.run[j] + 1; lev[j] = b.level[j]; }
    cat(CAT_SIG);
    {
      M_sig_v S;
      S.cls = cls;
      S.nc = nctx;
      S.qs = qs;
      S.mvc = mvc;
      S.rnz = rnz;
      for (i = start, prev = start - 1; i < BLK_LAST; i++) {
        const int r = sc[i], u = r & (BLK_W - 1), v = r >> BLK_SHIFT, run = i - prev - 1;
        S.pos = i;
        S.kb = cap(k, kCap);
        S.nz = tri2(Lb ? Lb->mag[i] != 0 : kTriNone, Ab ? Ab->mag[i] != 0 : kTriNone);
        S.past = tri2(Lb ? i > Lb->last : kTriNone, Ab ? i > Ab->last : kTriNone);  // beyond their last coefficient
        S.run = cap(run, kCap);
        S.fp = (u ? rm[r - 1] : 0) << FP_L | (v ? rm[r - BLK_W] : 0) << FP_A | (u && v ? rm[r - BLK_W - 1] : 0) << FP_AL |
               (u > 1 ? rm[r - 2] : 0) << FP_L2 | (v > 1 ? rm[r - 2 * BLK_W] : 0) << FP_A2;
        S.xm = Xb ? cap(Xb->mag[i], kTinyCap) : kTinyCap + 1;
        S.h8 = (int)(sh & ((1u << kHist) - 1));
        S.h16 = (int)(sh & ((1u << kHistLong) - 1));
        S.sl = Lb ? cap(Lb->suf[i + 1], kCap) : kNone;
        S.sa = Ab ? cap(Ab->suf[i + 1], kCap) : kNone;
        S.ie = eon ? est_level(E4[sc[i]]) : 0;
        S.tq = tq;
        S.tx = ptx;
        S.em = pem;
        S.pe = pon ? est_level(E4[sc[i]]) : 0;
        S.td = ptd;
        S.pf = pon ? qlog1((PF[sc[i]] < 0 ? -PF[sc[i]] : PF[sc[i]]) / rec.qscale) : 0;
        sel(f_sig, S);
        if (!f_sig.bit(SIG_SIG, ENC && k < b.n && pos[k] == i)) {
          sh <<= 1;
          continue;
        }
        sh = sh << 1 | 1;
        pos[k] = prev = i;
        rm[r] = 1;
        if (f_sig.bit(SIG_LAST, ENC && k == b.n - 1)) {
          k++;
          break;
        }
        k++;
      }
    }
    if (i == BLK_LAST) pos[k++] = BLK_LAST;  // reached the end: the last coefficient is there
    rm[sc[pos[k - 1]]] = 1;
    memcpy(nzr, rm, sizeof(rm));
    memset(rm, 0, sizeof(rm));
    cat(CAT_LEV);
    {
      // lh: the magnitudes coded before, the last three, each at most 3 in 2 bits; ssum: their sum
      constexpr int kLhBase = 4, kLhMask = 63;
      int ngt1 = 0, neq1 = 0, pm = 0, lh = 0, ssum = 0;
      M_lvl_v L;
      L.cls = cls;
      L.qs = qs;
      L.kc = cap(k, kCountCap);
      L.mvc = mvc;
      for (j = k - 1; j >= 0; j--) {
        const int a = ENC ? (lev[j] < 0 ? -lev[j] : lev[j]) - 1 : 0, p = pos[j], r = sc[p], u = r & (BLK_W - 1),
                  v = r >> BLK_SHIFT;
        const int nm = (Lb ? Lb->mag[p] : 0) + (Ab ? Ab->mag[p] : 0);
        // the frequency neighbours right, below and below right, coded already
        const int ur = u < BLK_W - 1, vb = v < BLK_W - 1;  // (there are some)
        const int sum = (ur ? rm[r + 1] : 0) + (vb ? rm[r + BLK_W] : 0) + (ur && vb ? rm[r + BLK_W + 1] : 0);
        const int ra = ur ? cap(rm[r + 1], kRbCap) : 0, ba = vb ? cap(rm[r + BLK_W], kRbCap) : 0;
        tcx X;
        i64 t;
        L.pos = p;
        L.gs = ngt1 ? 0 : 1 + cap(neq1, kTinyCap - 1);
        L.nmb = cap(nm, kTinyCap);
        L.pm = pm;
        L.nb2 = cap(sum, kTinyCap);
        L.den = (ur ? nzr[r + 1] : 0) + (vb ? nzr[r + BLK_W] : 0) + (u ? nzr[r - 1] : 0) + (v ? nzr[r - BLK_W] : 0);
        L.lh = lh;
        L.rb = ra * (kRbCap + 1) + ba;
        L.ssb = ssum ? qlog1(ssum) : 0;
        L.ngt = cap(ngt1, kTinyCap);
        L.ie = eon ? est_level(E4[sc[p]]) : 0;
        L.tq = tq;
        L.tx = ptx;
        L.em = pem;
        L.pe = pon ? est_level(E4[sc[p]]) : 0;
        L.pf = pon ? qlog1((PF[sc[p]] < 0 ? -PF[sc[p]] : PF[sc[p]]) / rec.qscale) : 0;
        f_lvl.make(L, X);
        if (eon) {
          const int ea = E4[sc[p]] < 0 ? -E4[sc[p]] : E4[sc[p]];
          f_lvl.hint = q4_round(ea) > 1 ? q4_round(ea) - 1 : 0;
        }
        t = checked(f_lvl.code(X, a), 0, kLevelMax);
        f_lvl.hint = -1;
        if (t) ngt1++;
        else neq1++;
        lev[j] = (int)t + 1;
        pm = cap((int)t + 1, kTinyCap);
        lh = (lh * kLhBase + cap(pm, kLhBase - 1)) & kLhMask;
        ssum += lev[j];
        rm[sc[p]] = o.mag[p] = (U8)cap(lev[j], kMagMax);
      }
    }
    o.last = (S8)pos[k - 1];
    o.n = (U8)k;
    for (i = BLK_LAST; i >= 0; i--) o.suf[i] = (U8)(o.suf[i + 1] + (o.mag[i] != 0));
    cat(CAT_SIGN);
    {
      U8 srs[BLK_COEFS];   // the signs coded so far by raster position: 0 none, 1 +, 2 -
      int sgh = 0;  // the last 3 signs (1 +, 2 -) in base 3
      M_sgn_v G;
      RcSign sp;    // (level -9) the first K signs predicted, coded after the others
      const int K = LV::recon && rec.on && rec.sign_can(n) ? (k < RC_SPK ? k : RC_SPK) : 0;
      int t;
      memset(srs, 0, sizeof(srs));
      G.cls = cls;
      for (t = 0; t < k; t++) {
        int p, r, u, v;
        j = t < k - K ? K + t : t - (k - K);
        if (K && t == k - K) rec.sign_init(sp, n, intra, dcvlc, b.dc, k, pos, lev, K);
        p = pos[j], r = sc[p], u = r & (BLK_W - 1), v = r >> BLK_SHIFT;
        const int ls = Lb && Lb->mag[p] ? Lb->sgn[p] : kTriNone, as = Ab && Ab->mag[p] ? Ab->sgn[p] : kTriNone;
        // the frequency neighbours left, above (and 2 away), above left: coded before, in zigzag order
        const int fl = u ? srs[r - 1] : 0, fu = v ? srs[r - BLK_W] : 0, fd = u && v ? srs[r - BLK_W - 1] : 0;
        const int fl2 = u > 1 ? srs[r - 2] : 0, fu2 = v > 1 ? srs[r - 2 * BLK_W] : 0;
        int vote = 0;  // their signs, weighted by their magnitudes
        if (fl) vote += (fl == 1 ? kVoteNb : -kVoteNb) * (rm[r - 1] > 1 ? kVoteBig : 1);
        if (fu) vote += (fu == 1 ? kVoteNb : -kVoteNb) * (rm[r - BLK_W] > 1 ? kVoteBig : 1);
        if (fd) vote += fd == 1 ? -kVoteDiag : kVoteDiag;
        G.pos = p;
        G.first = j == 0;
        G.lsa = tri2(ls, as);
        G.gh = sgh;
        G.ps = j && lev[j - 1] < 0;
        G.flu = tri2(fl, fu);
        G.flu2 = tri2(fl2, fu2);
        G.vote = vote + kVoteMax;
        G.fdlu = tri2(tri2(fd, fl), fu);
        G.xs = Xb && Xb->mag[p] ? 1 + Xb->sgn[p] : 0;
        G.lam = sign_mag(Lb, p) * kSignMag + sign_mag(Ab, p);
        G.pp = G.mg = G.pp2 = G.mg2 = 0;
        G.rk = RC_SPK;
        G.ie = eon ? est_level(E4[sc[p]]) : 0;
        G.es = eon && (E4[sc[p]] >= kQ / 2 || E4[sc[p]] <= -kQ / 2) ? 1 + (E4[sc[p]] < 0) : 0;  // (half a level)
        G.pj = pon && (E4[sc[p]] >= kQ / 2 || E4[sc[p]] <= -kQ / 2) ? 1 + (E4[sc[p]] < 0) : 0;
        G.pe = pon ? est_level(E4[sc[p]]) : 0;
        if (j < K && sp.nc) {
          int mg;
          G.pp = 1 + sp.predict(0, j, &mg);
          G.mg = mg ? qlog1(mg) : 0;
          G.pp2 = 1 + sp.predict(1, j, &mg);
          G.mg2 = mg ? qlog1(mg) : 0;
          G.rk = j;
        }
        sel(f_sgn, G);
        if (f_sgn.bit(0, ENC && b.level[j] < 0)) lev[j] = -lev[j];
        if (j < K && sp.nc) sp.decide(j, lev[j] < 0);
        o.sgn[p] = lev[j] < 0;
        srs[r] = (U8)(1 + (lev[j] < 0));
        sgh = sgh * kTri % kGh + 1 + (lev[j] < 0);
      }
    }
    out = o;  // (after the signs: out may be Ab)
    if (eon) {
      int cl[BLK_COEFS];
      memset(cl, 0, sizeof cl);
      if (dcvlc) cl[0] = b.dc;
      for (j = 0; j < k; j++) cl[sc[pos[j]]] = lev[j];
      est_account(E4, cl);
    }
    cat(CAT_ESC);
    for (j = 0, prev = start - 1; j < k; j++) {
      const int run = pos[j] - prev - 1, al = lev[j] < 0 ? -lev[j] : lev[j];
      const int d = rl.esc_default(j == k - 1, run, al);
      int e = ENC ? b.esc[j] : d;
      M_esc_v E;
      E.dm = d;
      sel(f_esc, E);
      if (!f_esc.bit(ESCN_USUAL, e == d)) {  // which, in a tree of two bits
        const int b1 = f_esc.bit(ESCN_HI, ENC && (e >> 1) & 1);
        e = b1 * 2 + f_esc.bit(ESCN_LO + b1, ENC && e & 1);
      }
      if (!ENC) { b.run[j] = (U8)run; b.level[j] = (S16)lev[j]; b.esc[j] = (U8)e; }
      prev = pos[j];
    }
    if (!ENC) b.n = k;
  }
