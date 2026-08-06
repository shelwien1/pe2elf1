
  /* alpha endpoint pair (a0,a1): match candidates then MSB-first literals. */
  template<int F>
  D5C_FORCE_INLINE void plane_aep(BlkRef& q) {
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
    //---------------- alpha endpoints (DXT5)
    if(walp && epx && !wcol) {
      /* Alpha-only formats (BC4, BC5, BTC) have no colour plane, so the record
       * has to be written HERE -- for DXT5 it is plane_cep that calls
       * epx_put/epx_get, and it also re-writes bp[0..1] afterwards to undo the
       * staging above.  BC5 runs this once per channel per block, so its stream
       * carries two records per block: all of channel 0 for a surface, then all
       * of channel 1, which is the order code_level walks them in. */
      if(enc) { epx_put(bp[0], bp[1], 0, 0); cur.a0 = bp[0]; cur.a1 = bp[1]; }
      else {
        int a0 = 0, a1 = 0, c0, c1;
        if(!epx_get(a0, a1, c0, c1)) epx_fail = true;
        bp[0] = u8(a0); bp[1] = u8(a1); cur.a0 = u8(a0); cur.a1 = u8(a1);
      }
    } else if(walp && epx) {
      // colour+alpha: stage here, plane_cep commits the RGBA record
      if(enc) { epa0 = bp[0]; epa1 = bp[1]; }
      else    { bp[0] = u8(epa0); bp[1] = u8(epa1); }
      cur.a0 = u8(epa0); cur.a1 = u8(epa1);
    } else if(walp) {
      scat = 0;
      int a0 = enc ? bp[0] : 0, a1 = enc ? bp[1] : 0;
      u16 pw = u16(W.a0 | W.a1 << 8), pn = u16(N.a0 | N.a1 << 8);
      u32 hh = (u32(pw) * 2654435761u ^ u32(pn) * 0x9E3779B1u) >> (32 - hbA);
      u32 he = HA[hh];
      bool hv = (he >> 16) & 1;
      u16 ph = u16(he);
      int actual;
      if(enc) {
        u16 pc = u16(a0 | a1 << 8);
        actual = hasW && pc == pw ? 0
               : hasN && pc == pn && !(hasW && pn == pw) ? 1
               : hv && pc == ph && !(hasW && ph == pw) && !(hasN && ph == pn) ? 2 : -2;
      } else actual = -1;
      int m = match<u16>(hasW, pw, hasN, pn, hv, ph, false, 0, hAM, mtAM,
                         tAM1, tAM2, tAN1, tAN2, tAMH, tAMH, mxAMW, mxAMN, mxAMH, mxAMH, actual);
      if(m == 0) { a0 = W.a0; a1 = W.a1; }
      else if(m == 1) { a0 = N.a0; a1 = N.a1; }
      else if(m == 2) { a0 = ph & 255; a1 = ph >> 8; }
      else {
        int amn = 255, amx = 0;
        if(hasP) for(int t = 0; t < 16; t++) {
          int pr, pg, pb, pav; par_px(int(bx) * 4 + (t & 3), int(by) * 4 + (t >> 2), pr, pg, pb, pav);
          if(pav < amn) amn = pav; if(pav > amx) amx = pav;
        } else { amn = 0; amx = 0; }
        /* D6: max/min of the eight decoded texels physically touching this
         * block -- W's right column and N's bottom row.  255/0 when absent, so
         * the row degenerates to a constant on the first row/column. */
        int emx = 0, emn = 255;
        if(hasW) for(int j = 0; j < 4; j++) { int v = pA[size_t(by * 4 + j) * iw + bx * 4 - 1];
                                              if(v > emx) emx = v; if(v < emn) emn = v; }
        if(hasN) for(int i = 0; i < 4; i++) { int v = pA[size_t(by * 4 - 1) * iw + bx * 4 + i];
                                              if(v > emx) emx = v; if(v < emn) emn = v; }
        if(!hasW && !hasN) { emx = 0; emn = 0; }
        /* D7: one 8-bit projection of the whole causal neighbourhood.  Q8
         * integer coefficients from a least-squares fit; no floating point
         * touches a coded bit.  A fitted predictor from a SINGLE input would be
         * worthless -- a table keyed on that input already represents any
         * function of it -- so the point is purely that a 256-row table can key
         * on one projection of nine inputs but not on nine jointly. */
        const int lp0 = clampi((A0_L0K + A0_L0Wa0 * W.a0 + A0_L0Na0 * N.a0 + A0_L0Xa0 * NW.a0
                              + A0_L0Wa1 * W.a1 + A0_L0Na1 * N.a1 + A0_L0Xa1 * NW.a1
                              + A0_L0Emn * emn + A0_L0Emx * emx) >> 8, 0, 255);
        int p2 = clampi(W.a0 + N.a0 - NW.a0, 0, 255);
        int x0 = W.a0 > N.a0 ? W.a0 : N.a0, i0 = W.a0 < N.a0 ? W.a0 : N.a0;
        int med0 = NW.a0 >= x0 ? i0 : NW.a0 <= i0 ? x0 : W.a0 + N.a0 - NW.a0;
        a0 = tree(mxA0, tA0W.row(int(MakeAE0W(W.a0))), tA0N.row(int(MakeAE0N(N.a0))),
                  tA0P.row(int(MakeAE0P(u32(p2)))), tA0o.row(0), 8, enc ? a0 : -1,
                  tA0M.row(int(MakeAE0M(u32(med0)))),
                  tA0H.row(int(MakeAE0H(hv ? u32(ph & 255) : 0u, u32(hv)))),
                  tA0E.row(int(MakeAE0E(u32(emx)))),
                  tA0L.row(int(MakeAE0L(u32(lp0)))),
                  hasP ? tA0P2.row(int(MakeAE0P2(u32(amx), u32(amn)))) : nullptr);
        const int lp1 = clampi((A0_L1K + A0_L1A0 * a0 + A0_L1Wa1 * W.a1 + A0_L1Na1 * N.a1
                              + A0_L1Xa1 * NW.a1 + A0_L1Wa0 * W.a0 + A0_L1Na0 * N.a0
                              + A0_L1Emn * emn + A0_L1Emx * emx) >> 8, 0, 255);
        int p3 = clampi(W.a1 + N.a1 - NW.a1, 0, 255);
        int x1 = W.a1 > N.a1 ? W.a1 : N.a1, i1 = W.a1 < N.a1 ? W.a1 : N.a1;
        int med1 = NW.a1 >= x1 ? i1 : NW.a1 <= i1 ? x1 : W.a1 + N.a1 - NW.a1;
        a1 = tree(mxA1, tA1W.row(int(MakeAE1W(W.a1))), tA1A.row(int(MakeAE1A(u32(a0)))),
                  tA1P.row(int(MakeAE1P(u32(p3)))), tA1o.row(0), 8, enc ? a1 : -1,
                  tA1M.row(int(MakeAE1M(u32(med1)))),
                  tA1H.row(int(MakeAE1H(hv ? u32(ph >> 8) : 0u, u32(hv)))),
                  tA1E.row(int(MakeAE1E(u32(emn)))),
                  tA1L.row(int(MakeAE1L(u32(lp1)))),
                  hasP ? tA1P2.row(int(MakeAE1P2(u32(amx), u32(amn)))) : nullptr);
      }
      cur.a0 = u8(a0); cur.a1 = u8(a1);
      HA[hh] = 1u << 16 | u32(a0) | u32(a1) << 8;
    }

  }
