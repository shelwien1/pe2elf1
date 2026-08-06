
  /* alpha index plane: word match, then per-texel indices through p^. */
  template<int F>
  D5C_FORCE_INLINE void plane_ai(BlkRef& q) {
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
    //---------------- alpha indices (DXT5 / BC4 / BC5)
    if(walp) {
      u8 ap[8] = {}; apalN(cur.a0, cur.a1, ap, npa);
      u32 mode = cur.a0 > cur.a1;
      u32 d = u32(cur.a0 > cur.a1 ? cur.a0 - cur.a1 : cur.a1 - cur.a0);

      scat = 2;
      u64 wwd = W.aw, nwd = N.aw;
      u32 hh = (u32(cur.a0) * 2654435761u ^ u32(cur.a1) * 0x9E3779B1u) >> (32 - hbAW);
      const u8 tg1 = u8(0x80 | ((u32(cur.a0) * 0x2545F491u ^ u32(cur.a1) * 0x9E3779B1u) >> 25));
      bool hv = HAWv[hh] == tg1;
      u64 hwd = HAW[hh];
      u32 hh2 = u32((wwd * 0x9E3779B97F4A7C15ull ^ nwd * 0xC2B2AE3D27D4EB4Full) >> (64 - hbAW2));
      const u8 tg2 = u8(0x80 | u32((wwd * 0x2545F4914F6CDD1Dull ^ nwd * 0xC2B2AE3D27D4EB4Full) >> 57));
      bool h2v = HAW2v[hh2] == tg2;
      u64 h2wd = HAW2[hh2];
      u64 aw = 0;
      if(enc) for(int i = 0; i < nwb; i++) aw |= u64(bp[2 + i]) << (8 * i);
      int actual;
      if(enc)
        actual = hasW && aw == wwd ? 0
               : hasN && aw == nwd && !(hasW && nwd == wwd) ? 1
               : hv && aw == hwd && !(hasW && hwd == wwd) && !(hasN && hwd == nwd) ? 2
               : h2v && aw == h2wd && !(hasW && h2wd == wwd) && !(hasN && h2wd == nwd) && !(hv && h2wd == hwd) ? 3 : -2;
      else actual = -1;
      int m = match<u64>(hasW, wwd, hasN, nwd, hv, hwd, h2v, h2wd, hAW, mtAW,
                         tAW1, tAW2, tAN1w, tAN2w, tAWH, tAWH2, mxAWW, mxAWN, mxAWH, mxAWH2, actual);
      if(m == 0) aw = wwd; else if(m == 1) aw = nwd; else if(m == 2) aw = hwd; else if(m == 3) aw = h2wd;
      scat = 3;
      int stw = (cur.a0 > W.a0 ? cur.a0 - W.a0 : W.a0 - cur.a0)
              + (cur.a1 > W.a1 ? cur.a1 - W.a1 : W.a1 - cur.a1);
      u32 sq = hasW ? u32(lgb(stw)) : 9u; // A4 stability bucket, 9 = no W
      int prev = prevAi;
      for(int t = 0; t < 16; t++) {
        int gx = int(bx) * 4 + (t & 3), gy = int(by) * 4 + (t >> 2);
        int v;
        if(m >= 0) v = int(aw >> (nbi * t) & (npa - 1));
        else {
          int al = gx ? pA[size_t(gy) * iw + gx - 1] : 0;
          int au = gy ? pA[size_t(gy - 1) * iw + gx] : 0;
          int pl = 0, pu = 0, bd;
          bd = DIST_A_MAX; for(int k = 0; k < npa; k++) { int dd = al - ap[k]; if(dd < 0) dd = -dd; if(dd < bd) { bd = dd; pl = k; } }
          int bdl = bd;
          bd = DIST_A_MAX; for(int k = 0; k < 8; k++) { int dd = au - ap[k]; if(dd < 0) dd = -dd; if(dd < bd) { bd = dd; pu = k; } }
          int bdu = bd;
          u32 cfl = u32(lgb(bdl)), cfu = u32(lgb(bdu)); // v1 s2.1 p^ confidence
          // A1: parent-predicted alpha -> nearest current-palette index
          u32 ppa = 8u, paf = 6u, pai = 8u;
          if(hasP) {
            int pr, pg, pb, pav; par_px(gx, gy, pr, pg, pb, pav);
            int bb = 999, bk = 0;
            for(int k = 0; k < npa; k++) { int dd = pav - ap[k]; if(dd < 0) dd = -dd; if(dd < bb) { bb = dd; bk = k; } }
            ppa = u32(bk); paf = u32(lgb(bb));
            pai = qAI[size_t(gy * 2) * qw + gx * 2];
          }
          int gla = clampi(pl - prevPlA, -7, 7) + 7;    // A5 gradient
          int spa = clampi(2 * pl - int(W.ai[t]), 0, 7); // A5 slope predictor
          int aul = gx && gy ? pA[size_t(gy - 1) * iw + gx - 1] : 0;
          int pql = 0;
          bd = DIST_A_MAX; for(int k = 0; k < npa; k++) { int dd = aul - ap[k]; if(dd < 0) dd = -dd; if(dd < bd) { bd = dd; pql = k; } }
          int aur = gy && u32(gx + 1) < iw ? pA[size_t(gy - 1) * iw + gx + 1] : 0;
          int pqr = 0;
          bd = DIST_A_MAX; for(int k = 0; k < npa; k++) { int dd = aur - ap[k]; if(dd < 0) dd = -dd; if(dd < bd) { bd = dd; pqr = k; } }
          u32 h1c = u32(hv ? hwd >> (nbi * t) & u64(npa - 1) : 0),
              h2c = u32(h2v ? h2wd >> (nbi * t) & u64(npa - 1) : 0);
          u32 rawL = gx ? pAI[size_t(gy) * iw + gx - 1] : 0, rawU = gy ? pAI[size_t(gy - 1) * iw + gx] : 0;
          Ctr* cs[14] = {
            tAI1.row(int(MakeAI1(u32(pl), u32(pu), mode, d))),
            tAI2.row(int(MakeAI2(u32(prev), u32(t)))),
            tAI3.row(int(MakeAI3(W.ai[t], u32(pl)))),
            tAI4.row(int(MakeAI4(N.ai[t], u32(pu)))),
            tAI0.row(int(MakeAI0(u32(d == 0), mode, d))),
            tAI5.row(int(MakeAI5(h1c, u32(pl), u32(hv)))),
            tAI6.row(int(MakeAI6(h2c, u32(pl), u32(h2v)))),
            tAI9.row(int(MakeAI9(rawL, rawU))),
            tAI8.row(int(MakeAI8(u32(pl), u32(pu), u32(pql == pl)))),
            tAIb.row(int(MakeAIb(u32(pl), u32(pu), u32(pql), u32(pqr)))),
            tAIe.row(int(MakeAIe(u32(pl), u32(pu), cfl, cfu))),
            tAIf.row(int(MakeAIf(u32(gla), u32(spa), u32(prev)))),
            tAIp.row(int(MakeAIp(ppa, u32(pl), u32(pu)))),
            tAIq.row(int(MakeAIq(ppa, paf, pai)))};
          const int nas = hasP ? 14 : 12; // parent rows only when a parent exists
          int nd = 1;
          int mctx = int(MakeAIM(u32(pl == pu), mode, d, sq, u32(t), cfl)) * 8;
          int actx = int(MakeAPA(u32(pl == pu), u32(pl), mode, sq)) * 8;
          int actx2 = int(MakeAPA2(u32(pl), u32(pu), W.ai[t])) * 8;
          int slc = int(MakeSLA(u32(pl), u32(pu), mode, d)) * 8;
          for(int k = nbi - 1; k >= 0; k--) {
            Ctr* cc[14] = {cs[0] + nd, cs[1] + nd, cs[2] + nd, cs[3] + nd, cs[4] + nd,
                           cs[5] + nd, cs[6] + nd, cs[7] + nd, cs[8] + nd, cs[9] + nd,
                           cs[10] + nd, cs[11] + nd, cs[12] + nd, cs[13] + nd};
            slowin = &sAI1[slc + nd];
            int b = hasP
              ? mbit<14>(mxAI, mctx + nd, cc, &apAI, actx + nd, enc ? int(aw >> (nbi * t + k)) & 1 : -1,
                         &apAI2, actx2 + nd, G0_AB_AI, G0_AB_AI2)
              : mbit<12>(mxAI, mctx + nd, cc, &apAI, actx + nd, enc ? int(aw >> (nbi * t + k)) & 1 : -1,
                         &apAI2, actx2 + nd, G0_AB_AI, G0_AB_AI2);
            nd = nd + nd + b;
          }
          v = nd - npa;
          prevPlA = pl;
          if(!enc) aw |= u64(v) << (nbi * t);
        }
        cur.ai[t] = u8(v);
        pA[size_t(gy) * iw + gx] = ap[v]; pAI[size_t(gy) * iw + gx] = u8(v);
        prev = v;
      }
      prevAi = prev;
      cur.aw = aw;
      HAW[hh] = aw; HAWv[hh] = tg1; HAW2[hh2] = aw; HAW2v[hh2] = tg2;
      if(!enc) { bp[0] = cur.a0; bp[1] = cur.a1; for(int i = 0; i < nwb; i++) bp[2 + i] = u8(aw >> (8 * i)); }
    }

  }
