
  /* colour index plane: word match, then per-texel indices through p^. */
  template<int F>
  D5C_FORCE_INLINE void plane_ci(BlkRef& q) {
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
    //---------------- color indices
    if(wcol) {
      bool mode4 = fmt != FMT_DXT1 || cur.c0 > cur.c1;
      u8 cp[4][3]; cpal4(cur.c0, cur.c1, mode4, cp);
      u32 spq = 0, stq = 0, mdq = 0;
      int nd4 = 1;
      for(int k = 1; k < 4; k++) {
        int uq = 1;
        for(int j = 0; j < k; j++)
          if(cp[k][0] == cp[j][0] && cp[k][1] == cp[j][1] && cp[k][2] == cp[j][2]) { uq = 0; break; }
        nd4 += uq;
      }
      u8* cb = bp + (fmt == FMT_DXT1 ? 0 : 8);

      // s2.4/A10: a->c coupling -- decoded alpha value bucket per texel
      u8 avq[16];
      if(fmt == FMT_DXT5) {
        u8 aap[8] = {}; apalN(cur.a0, cur.a1, aap, fmt_npal(fmt));
        for(int t = 0; t < 16; t++) avq[t] = u8(aap[cur.ai[t]] >> 6);
      } else for(int t = 0; t < 16; t++) avq[t] = 3;

      // A4: palette geometry (endpoint span) + stability vs W
      {
        int dr0 = int(cp[0][0]) - cp[1][0], dg0 = int(cp[0][1]) - cp[1][1], db0 = int(cp[0][2]) - cp[1][2];
        spq = u32(lgb(dr0 * dr0 + dg0 * dg0 + db0 * db0));
        int pd = 0;
        for(int c = 0; c < 6; c++) {
          int a = (c < 3 ? cur.c0 : cur.c1) >> (c % 3 == 0 ? 11 : c % 3 == 1 ? 5 : 0) & (c % 3 == 1 ? 63 : 31);
          int b = (c < 3 ? W.c0 : W.c1) >> (c % 3 == 0 ? 11 : c % 3 == 1 ? 5 : 0) & (c % 3 == 1 ? 63 : 31);
          pd += (a - b) * (a - b);
        }
        stq = hasW ? u32(lgb(pd)) : u32(NO_W_BUCKET);
        int md2 = DIST_C_MAX;
        for(int k = 1; k < 4; k++) for(int j = 0; j < k; j++) {
          int dr = int(cp[k][0]) - cp[j][0], dg = int(cp[k][1]) - cp[j][1], db = int(cp[k][2]) - cp[j][2];
          int dd = dr * dr + dg * dg + db * db; if(dd < md2) md2 = dd;
        }
        mdq = u32(lgb(md2));
      }

      scat = 4;
      int agrW = 0; // decoded-so-far agreement with W.ci (s5.x dynamic stability)
      u32 wwd = W.cw, nwd = N.cw;
      u32 hh = (u32(cur.c0) * 2654435761u ^ u32(cur.c1) * 0x9E3779B1u) >> (32 - hbCW);
      const u8 tg1 = u8(0x80 | ((u32(cur.c0) * 0x2545F491u ^ u32(cur.c1) * 0x9E3779B1u) >> 25));
      bool hv = HCWv[hh] == tg1;
      u32 hwd = u32(HCW[hh]);
      u32 hh2 = (wwd * 2654435761u ^ nwd * 0x9E3779B1u) >> (32 - hbCW2);
      const u8 tg2 = u8(0x80 | ((wwd * 0x2545F491u ^ nwd * 0xC2B2AE3Du) >> 25));
      bool h2v = HCW2v[hh2] == tg2;
      u32 h2wd = u32(HCW2[hh2]);
      u32 cw = 0;
      if(enc) cw = u32(cb[4]) | u32(cb[5]) << 8 | u32(cb[6]) << 16 | u32(cb[7]) << 24;
      int actual;
      if(enc)
        actual = hasW && cw == wwd ? 0
               : hasN && cw == nwd && !(hasW && nwd == wwd) ? 1
               : hv && cw == hwd && !(hasW && hwd == wwd) && !(hasN && hwd == nwd) ? 2
               : h2v && cw == h2wd && !(hasW && h2wd == wwd) && !(hasN && h2wd == nwd) && !(hv && h2wd == hwd) ? 3 : -2;
      else actual = -1;
      int m = match<u32>(hasW, wwd, hasN, nwd, hv, hwd, h2v, h2wd, hCW, mtCW,
                         tCW1, tCW2, tCN1w, tCN2w, tCWH, tCWH2, mxCWW, mxCWN, mxCWH, mxCWH2, actual);
      if(m == 0) cw = wwd; else if(m == 1) cw = nwd; else if(m == 2) cw = hwd; else if(m == 3) cw = h2wd;
      scat = 5;
      int prev = prevCi;
      for(int t = 0; t < 16; t++) {
        int gx = int(bx) * 4 + (t & 3), gy = int(by) * 4 + (t >> 2);
        int v;
        if(m >= 0) v = int(cw >> (2 * t) & 3);
        else {
          int pl = 0, pu = 0, bd;
          size_t oL = size_t(gy) * iw + gx - 1, oU = size_t(gy - 1) * iw + gx;
          int lr = gx ? pR[oL] : 0, lg = gx ? pG[oL] : 0, lb = gx ? pB[oL] : 0;
          int ur = gy ? pR[oU] : 0, ug = gy ? pG[oU] : 0, ub = gy ? pB[oU] : 0;
          bd = DIST_C_MAX;
          for(int k = 0; k < 4; k++) {
            int dr = lr - cp[k][0], dg = lg - cp[k][1], db = lb - cp[k][2];
            int dd = dr * dr + dg * dg + db * db;
            if(dd < bd) { bd = dd; pl = k; }
          }
          int bdl = bd;
          bd = DIST_C_MAX;
          for(int k = 0; k < 4; k++) {
            int dr = ur - cp[k][0], dg = ug - cp[k][1], db = ub - cp[k][2];
            int dd = dr * dr + dg * dg + db * db;
            if(dd < bd) { bd = dd; pu = k; }
          }
          int bdu = bd;
          u32 cfl = u32(lgb(bdl)), cfu = u32(lgb(bdu)); // v1 s2.1 p^ confidence
          int glc = clampi(pl - prevPlC, -3, 3) + 3;    // A5 gradient
          int spc = clampi(2 * pl - int(W.ci[t]), 0, 3); // A5 slope predictor
          u32 agq = (!hasW || t == 0) ? 5u : u32(agrW * 4 / t); // 0..4, 5 = no info
          // A1: parent-predicted pixel -> nearest current-palette index
          u32 ppc = 4u, pcf = 6u, pci = 4u, pcs = 4u; // 4 = no parent
          if(hasP) {
            int pr, pg, pb, pa; par_px(gx, gy, pr, pg, pb, pa);
            int bp2 = 1 << 30, bk = 0;
            for(int k = 0; k < 4; k++) {
              int dr = pr - cp[k][0], dg = pg - cp[k][1], db = pb - cp[k][2];
              int dd = dr * dr + dg * dg + db * db;
              if(dd < bp2) { bp2 = dd; bk = k; }
            }
            ppc = u32(bk); pcf = u32(lgb(bp2));
            // parent raw index at the co-located texel + its 2x2 agreement
            int x2 = gx * 2, y2 = gy * 2, q0 = qCI[size_t(y2) * qw + x2], same = 0;
            for(int j = 0; j < 2; j++) for(int i = 0; i < 2; i++)
              same += (qCI[size_t(y2 + j) * qw + (x2 + i)] == q0);
            pci = u32(q0); pcs = u32(same - 1); // 0..3
          }
          int pul = 0, pur = 0;
          size_t oUL = size_t(gy - 1) * iw + gx - 1, oUR = size_t(gy - 1) * iw + gx + 1;
          bool hUL = gx && gy, hUR = gy && u32(gx + 1) < iw;
          int ar = hUL ? pR[oUL] : 0, ag = hUL ? pG[oUL] : 0, ab = hUL ? pB[oUL] : 0;
          int er = hUR ? pR[oUR] : 0, eg = hUR ? pG[oUR] : 0, eb = hUR ? pB[oUR] : 0;
          bd = DIST_C_MAX;
          for(int k = 0; k < 4; k++) {
            int dr = ar - cp[k][0], dg = ag - cp[k][1], db = ab - cp[k][2];
            int dd = dr * dr + dg * dg + db * db;
            if(dd < bd) { bd = dd; pul = k; }
          }
          bd = DIST_C_MAX;
          for(int k = 0; k < 4; k++) {
            int dr = er - cp[k][0], dg = eg - cp[k][1], db = eb - cp[k][2];
            int dd = dr * dr + dg * dg + db * db;
            if(dd < bd) { bd = dd; pur = k; }
          }
          u32 h1c = u32(hv ? hwd >> (2 * t) & 3 : 0), h2c = u32(h2v ? h2wd >> (2 * t) & 3 : 0);
          u32 rawL = gx ? pCI[oL] : 0, rawU = gy ? pCI[oU] : 0;
          u32 rawUL = gx && gy ? pCI[size_t(gy - 1) * iw + gx - 1] : 0;
          Ctr* cs[19] = {
            tCI1.row(int(MakeCI1(u32(pl), u32(pu), W.ci[t], agq))),
            tCI2.row(int(MakeCI2(u32(prev), u32(t)))),
            tCI3.row(int(MakeCI3(W.ci[t], u32(pl)))),
            tCI4.row(int(MakeCI4(N.ci[t], u32(pu)))),
            tCI0.row(int(MakeCI0(u32(!mode4)))),
            tCI5.row(int(MakeCI5(u32(pul), u32(pur)))),
            tCI6.row(int(MakeCI6(h1c, u32(pl), u32(hv)))),
            tCI7.row(int(MakeCI7(h2c, u32(pl), u32(h2v)))),
            tCI9.row(int(MakeCI9(rawL, rawU))),
            tCI8.row(int(MakeCI8(u32(pl), u32(pu), u32(pul)))),
            tCIb.row(int(MakeCIb(u32(pl), u32(pu), u32(pul), u32(pur)))),
            tCIc.row(int(MakeCIc(u32(pl), u32(pu), u32(pul), u32(pur), W.ci[t]))),
            tCId.row(int(MakeCId(rawL, rawU, rawUL))),
            tCIe.row(int(MakeCIe(u32(pl), u32(pu), cfl, cfu))),
            tCIf.row(int(MakeCIf(u32(glc), u32(spc), u32(prev)))),
            tCIg.row(int(MakeCIg(u32(pl), u32(pu), u32(avq[t])))),
            tCIp.row(int(MakeCIp(ppc, u32(pl), u32(pu)))),
            tCIq.row(int(MakeCIq(ppc, pcf, u32(prev)))),
            tCIr.row(int(MakeCIr(pci, pcs, ppc)))};
          const int ncs = hasP ? 19 : 16; // parent rows only when a parent exists
          int nd = 1;
          int mctx = int(MakeCIM(u32(pl == pu), u32(fmt == FMT_DXT1 && !mode4), u32(nd4 - 1), spq, u32(t), stq, mdq, u32(avq[t]))) * 4;
          int actx = int(MakeAPC(u32(pl), u32(pu), u32(pul), stq, u32(avq[t]))) * 4;
          int actx2 = int(MakeAPC2(u32(t), u32(pl), mdq, agq)) * 4;
          int slc = int(MakeSLC(u32(pl), u32(pu), u32(pul), u32(pur))) * 4;
          for(int k = 1; k >= 0; k--) {
            Ctr* cc[19] = {cs[0] + nd, cs[1] + nd, cs[2] + nd, cs[3] + nd, cs[4] + nd, cs[5] + nd, cs[6] + nd,
                           cs[7] + nd, cs[8] + nd, cs[9] + nd, cs[10] + nd, cs[11] + nd, cs[12] + nd,
                           cs[13] + nd, cs[14] + nd, cs[15] + nd, cs[16] + nd, cs[17] + nd, cs[18] + nd};
            slowin = &sCI1[slc + nd];
            int b = hasP
              ? mbit<19>(mxCI, mctx + nd, cc, &apCI, actx + nd, enc ? int(cw >> (2 * t + k)) & 1 : -1,
                         &apCI2, actx2 + nd, G0_AB_CI, G0_AB_CI2)
              : mbit<16>(mxCI, mctx + nd, cc, &apCI, actx + nd, enc ? int(cw >> (2 * t + k)) & 1 : -1,
                         &apCI2, actx2 + nd, G0_AB_CI, G0_AB_CI2);
            nd = nd + nd + b;
          }
          v = nd - 4;
          agrW += (v == int(W.ci[t]));
          prevPlC = pl;
          if(!enc) cw |= u32(v) << (2 * t);
        }
        cur.ci[t] = u8(v);
        size_t o = size_t(gy) * iw + gx;
        pR[o] = cp[v][0]; pG[o] = cp[v][1]; pB[o] = cp[v][2]; pCI[o] = u8(v);
        prev = v;
      }
      prevCi = prev;
      cur.cw = cw;
      HCW[hh] = cw; HCWv[hh] = tg1; HCW2[hh2] = cw; HCW2v[hh2] = tg2;
      if(!enc) { cb[4] = u8(cw); cb[5] = u8(cw >> 8); cb[6] = u8(cw >> 16); cb[7] = u8(cw >> 24); }
    }

  }
