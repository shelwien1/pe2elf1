
  /* colour endpoint pair (c0,c1) as three 5/6/5 channel literals. */
  template<int F>
  D5C_FORCE_INLINE void plane_cep(BlkRef& q) {
    constexpr dxt_fmt fmt = dxt_fmt(F);
    constexpr int  nbi  = fmt_ibits(fmt);
    constexpr int  npa  = fmt_npal(fmt);
    constexpr int  nwb  = nbi * 16 / 8;
    constexpr bool wcol = fmt_color(fmt);
    constexpr bool walp = fmt_ach(fmt) > 0;
    (void)nbi; (void)npa; (void)nwb; (void)wcol; (void)walp;
    u8P bp = q.bp; const u32 bx = q.bx, by = q.by;
    const bool hasW = q.hasW, hasN = q.hasN;
    const Blk &W = q.W, &N = q.N, &NW = q.NW, &NE = q.NE, &Z = q.Z;
    Blk& cur = q.cur;
    (void)bp; (void)bx; (void)by; (void)hasW; (void)hasN;
    (void)W; (void)N; (void)NW; (void)NE; (void)Z; (void)cur;
    //---------------- color endpoints
    if(!wcol) { /* BC4/BC5: no 565 half */ }
    else if(epx) {
      u8* cb = bp + (fmt == FMT_DXT1 ? 0 : 8);
      if(enc) {
        int c0 = cb[0] | cb[1] << 8, c1 = cb[2] | cb[3] << 8;
        epx_put(epa0, epa1, c0, c1);
        cur.c0 = u16(c0); cur.c1 = u16(c1);
      } else {
        int c0, c1;
        if(!epx_get(epa0, epa1, c0, c1)) { epx_fail = true; c0 = c1 = 0; }
        cb[0] = u8(c0); cb[1] = u8(c0 >> 8); cb[2] = u8(c1); cb[3] = u8(c1 >> 8);
        cur.c0 = u16(c0); cur.c1 = u16(c1);
        if(fmt == FMT_DXT5) { bp[0] = u8(epa0); bp[1] = u8(epa1); cur.a0 = u8(epa0); cur.a1 = u8(epa1); }
      }
    } else {
      scat = 1;
      u8* cb = bp + (fmt == FMT_DXT1 ? 0 : 8);
      const Blk& P = hasP ? par_blk(bx, by) : Z; // A1: co-located parent block
      u32 pp = u32(P.c0) | u32(P.c1) << 16;
      u32 pw = u32(W.c0) | u32(W.c1) << 16, pn = u32(N.c0) | u32(N.c1) << 16;
      u32 hh = (pw * 2654435761u ^ pn * 0x9E3779B1u) >> (32 - hbC);
      u64 he = HC[hh];
      bool hv = (he >> 32) & 1;
      u32 ph = u32(he);
      int c0 = 0, c1 = 0, actual;
      if(enc) {
        c0 = cb[0] | cb[1] << 8; c1 = cb[2] | cb[3] << 8;
        u32 pc = u32(c0) | u32(c1) << 16;
        actual = hasW && pc == pw ? 0
               : hasN && pc == pn && !(hasW && pn == pw) ? 1
               : hv && pc == ph && !(hasW && ph == pw) && !(hasN && ph == pn) ? 2
               : hasP && pc == pp && !(hasW && pp == pw) && !(hasN && pp == pn) && !(hv && pp == ph) ? 3 : -2;
      } else actual = -1;
      int m = match<u32>(hasW, pw, hasN, pn, hv, ph, hasP, pp, hCM, mtCM,
                         tCM1, tCM2, tCN1, tCN2, tCMH, tCMP, mxCMW, mxCMN, mxCMH, mxCMP, actual);
      if(m == 0) { c0 = W.c0; c1 = W.c1; }
      else if(m == 1) { c0 = N.c0; c1 = N.c1; }
      else if(m == 2) { c0 = int(ph & 0xFFFF); c1 = int(ph >> 16); }
      else if(m == 3) { c0 = P.c0; c1 = P.c1; }
      else {
        // channel fields: r 5, g 6, b 5 (values, not wire bit order)
        /* D6: per-channel max/min of the eight decoded texels touching this
         * block (W's right column, N's bottom row).  0 when absent. */
        int eex[3] = {0,0,0}, een[3] = {255,255,255};
        {
          const u8P pl_[3] = { pR, pG, pB };
          for(int k = 0; k < 3; k++) {
            if(hasW) for(int j = 0; j < 4; j++) { int v = pl_[k][size_t(by * 4 + j) * iw + bx * 4 - 1];
                                                  if(v > eex[k]) eex[k] = v; if(v < een[k]) een[k] = v; }
            if(hasN) for(int i = 0; i < 4; i++) { int v = pl_[k][size_t(by * 4 - 1) * iw + bx * 4 + i];
                                                  if(v > eex[k]) eex[k] = v; if(v < een[k]) een[k] = v; }
            if(!hasW && !hasN) { eex[k] = 0; een[k] = 0; }
          }
        }
        int wv[6] = {W.c0 >> 11 & 31, W.c0 >> 5 & 63, W.c0 & 31, W.c1 >> 11 & 31, W.c1 >> 5 & 63, W.c1 & 31};
        int qv[6] = {NE.c0 >> 11 & 31, NE.c0 >> 5 & 63, NE.c0 & 31, N.c1 >> 11 & 31, N.c1 >> 5 & 63, N.c1 & 31};
        int nv[6] = {N.c0 >> 11 & 31, N.c0 >> 5 & 63, N.c0 & 31, N.c1 >> 11 & 31, N.c1 >> 5 & 63, N.c1 & 31};
        // A1: per-channel range of the parent-predicted 4x4 block, quantized
        // to the channel width -- brackets the endpoint pair the encoder chose
        int pmn[3] = {255, 255, 255}, pmx[3] = {0, 0, 0};
        if(hasP) for(int t = 0; t < 16; t++) {
          int pr, pg, pb, pa; par_px(int(bx) * 4 + (t & 3), int(by) * 4 + (t >> 2), pr, pg, pb, pa);
          int q[3] = {pr, pg, pb};
          for(int c = 0; c < 3; c++) { if(q[c] < pmn[c]) pmn[c] = q[c]; if(q[c] > pmx[c]) pmx[c] = q[c]; }
        }
        int pvn[6], pvx[6];
        for(int c = 0; c < 6; c++) {
          int ch = c % 3, sh = ch == 1 ? 2 : 3;
          pvn[c] = hasP ? pmn[ch] >> sh : 0; pvx[c] = hasP ? pmx[ch] >> sh : 0;
        }
        u32 hc0 = ph & 0xFFFF, hc1 = ph >> 16;
        int hcv[6] = {int(hc0 >> 11 & 31), int(hc0 >> 5 & 63), int(hc0 & 31),
                      int(hc1 >> 11 & 31), int(hc1 >> 5 & 63), int(hc1 & 31)};
        int uv[6] = {NW.c0 >> 11 & 31, NW.c0 >> 5 & 63, NW.c0 & 31, NW.c1 >> 11 & 31, NW.c1 >> 5 & 63, NW.c1 & 31};
        static const int bits[6] = {5, 6, 5, 5, 6, 5};
        int ev[6];
        u32 lum = 0;
        for(int c = 0; c < 6; c++) {
          int hi = (1 << bits[c]) - 1;
          int p2 = clampi(wv[c] + nv[c] - uv[c], 0, hi);
          int cx = wv[c] > nv[c] ? wv[c] : nv[c], ci = wv[c] < nv[c] ? wv[c] : nv[c];
          int med = uv[c] >= cx ? ci : uv[c] <= ci ? cx : wv[c] + nv[c] - uv[c];
          if(c == 3) lum = u32((ev[0] + ev[1] + ev[2]) >> 3);
          int x = enc ? (c < 3 ? c0 : c1) >> (c % 3 == 0 ? 11 : c % 3 == 1 ? 5 : 0) & hi : -1;
          int base = c < 3 ? 0 : 3;
          Ctr* r3 = c % 3 == 0 ? tE[c][3].row(0)
                  : c % 3 == 1 ? tE[c][3].row(int(MakeEX1(u32(ev[base]))))
                               : tE[c][3].row(int(MakeEX2(u32(ev[base]), u32(ev[base + 1]))));
          if(c < 3) {
            ev[c] = tree(mxE[c], tE[c][0].row(int(MakeE0W(u32(wv[c])))),
                         tE[c][1].row(int(MakeE0N(u32(nv[c])))),
                         tE[c][2].row(int(MakeE0P(u32(p2)))), r3, bits[c], x,
                         tE[c][4].row(int(MakeE0Q(u32(qv[c])))),
                         tE[c][5].row(int(MakeE0J(u32(wv[c]), u32(nv[c])))),
                         tE[c][6].row(int(MakeE0M(u32(med)))),
                         tE[c][7].row(int(MakeE0H(hv ? u32(hcv[c]) : 0u, u32(hv)))),
                         tE[c][9].row(int(MakeE0E(u32(eex[c % 3])))),
                         hasP ? tE[c][8].row(int(MakeE0PP(u32(pvx[c]), u32(pvn[c]), u32(hasP)))) : nullptr);
          } else {
            ev[c] = tree(mxE[c], tE[c][0].row(int(MakeE1A(u32(ev[c - 3]), lum))),
                         tE[c][1].row(int(MakeE1W(u32(wv[c])))),
                         tE[c][2].row(int(MakeE1P(u32(p2)))), r3, bits[c], x,
                         tE[c][4].row(int(MakeE1Q(u32(qv[c])))),
                         tE[c][5].row(int(MakeE1J(u32(ev[c - 3]), u32(wv[c])))),
                         tE[c][6].row(int(MakeE1M(u32(med)))),
                         tE[c][7].row(int(MakeE1H(hv ? u32(hcv[c]) : 0u, u32(hv)))),
                         tE[c][9].row(int(MakeE1E(u32(een[c % 3])))),
                         hasP ? tE[c][8].row(int(MakeE1PP(u32(pvx[c]), u32(pvn[c]), u32(hasP)))) : nullptr);
          }
        }
        c0 = ev[0] << 11 | ev[1] << 5 | ev[2];
        c1 = ev[3] << 11 | ev[4] << 5 | ev[5];
      }
      cur.c0 = u16(c0); cur.c1 = u16(c1);
      HC[hh] = 1ull << 32 | u32(c0) | u32(c1) << 16;
      if(!enc) { cb[0] = u8(c0); cb[1] = u8(c0 >> 8); cb[2] = u8(c1); cb[3] = u8(c1 >> 8); }
    }

  }
