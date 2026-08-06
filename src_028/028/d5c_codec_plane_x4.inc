
  /* DXT3's 4-bit alpha plane -- a plain gradient plane, not an index plane. */
  template<int F>
  D5C_FORCE_INLINE void plane_x4(BlkRef& q) {
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
    //---------------- dxt3 alpha nibbles
    if(fmt == FMT_DXT3) {
      scat = 6;
      u64 xw = 0;
      if(enc) for(int i = 0; i < 8; i++) xw |= u64(bp[i]) << (8 * i);
      for(int t = 0; t < 16; t++) {
        int gx = int(bx) * 4 + (t & 3), gy = int(by) * 4 + (t >> 2);
        int xl = gx ? pX[size_t(gy) * iw + gx - 1] : 0;
        int xu = gy ? pX[size_t(gy - 1) * iw + gx] : 0;
        int xul = gx && gy ? pX[size_t(gy - 1) * iw + gx - 1] : 0;
        int p2 = clampi(xl + xu - xul, 0, 15);
        int v = enc ? int(xw >> (4 * t) & 15) : -1;
        u32 xp = hasP ? u32(par_x(gx, gy)) : 0u; // A1: parent 2x2 average
        u32 xpr = 0; // A1: spread of the 4 parent nibbles (0 = flat -> avg exact)
        if(hasP) {
          int mn = 15, mx = 0;
          for(int j = 0; j < 2; j++) for(int i = 0; i < 2; i++) {
            int q = qX[size_t(gy * 2 + j) * qw + (gx * 2 + i)];
            if(q < mn) mn = q; if(q > mx) mx = q;
          }
          xpr = u32(mx - mn);
        }
        v = tree(mxX, tX[0].row(int(MakeX1(u32(xl)))), tX[1].row(int(MakeX2(u32(xu)))),
                 tX[2].row(int(MakeX3(u32(p2)))), tX[3].row(0), 4, v,
                 hasP ? tX[4].row(int(MakeX4(xp, xpr, u32(hasP)))) : nullptr,
                 hasP ? tX[5].row(int(MakeX5(xp, u32(xl), u32(xu)))) : nullptr);
        if(!enc) xw |= u64(v) << (4 * t);
        pX[size_t(gy) * iw + gx] = u8(v);
      }
      if(!enc) for(int i = 0; i < 8; i++) bp[i] = u8(xw >> (8 * i));
    }

  }
