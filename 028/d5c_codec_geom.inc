/* d5c_codec_geom.inc -- included INSIDE struct Codec; not standalone.
 *
 * Level geometry: achan_swap, level_init, and the parent-level
 * accessors par_px / par_x / par_blk. */

  void achan_swap() {
    std::swap(rowP, rowP2); std::swap(rowC, rowC2);
    std::swap(cBlk, cBlk2); std::swap(qBlk, qBlk2);
    std::swap(pA, pA2); std::swap(pAI, pAI2); std::swap(qA, qA2); std::swap(qAI, qAI2);
    std::swap(prevAi, prevAi2); std::swap(prevPlA, prevPlA2);
    std::swap(hAM, hAM2); std::swap(hAW, hAW2);
    std::swap(mtAM, mtAM2); std::swap(mtAW, mtAW2);
    std::swap(hasP, hasP2);
  }

  void level_init(u32 bw_, u32 bh_) {
    // A1: the level just finished is the parent (2x finer) of the one starting
    hasP = iw != 0 && iw == bw_ * 8 && ih == bh_ * 8; // strict 2x, else no parent
    const bool par = hasP;   // both BC5 channels share this geometry
    if(hasP) {
      std::swap(qR, pR); std::swap(qG, pG); std::swap(qB, pB);
      std::swap(qA, pA); std::swap(qX, pX);
      std::swap(qCI, pCI); std::swap(qAI, pAI);
      std::swap(qBlk, cBlk); qw = iw; qh = ih; qbw = bw; qbh = bh;
    }
    bw = bw_; bh = bh_; iw = bw * 4; ih = bh * 4;
    const size_t np = size_t(iw) * ih;
    memset(cBlk, 0, size_t(bw) * bh * sizeof(Blk));
    memset(rowP, 0, size_t(bw) * sizeof(Blk)); memset(rowC, 0, size_t(bw) * sizeof(Blk));
    memset(pR, 0, np); memset(pG, 0, np);
    memset(pB, 0, np); memset(pA, 0, np);
    if(fmt == FMT_DXT3) memset(pX, 0, np);
    memset(pCI, 0, np); memset(pAI, 0, np);
    prevAi = prevCi = 0;
    prevPlA = prevPlC = 0;
    if(fmt == FMT_BC5) { // give channel Y the same fresh state, then park it
      achan_swap();
      /* was: hasP = iw != 0 && iw == bw_*8 && ih == bh_*8 -- but iw had already
       * been updated to bw_*4 sixteen lines above, so the test could never be
       * true and channel Y silently never got a mip parent. */
      hasP = par;
      if(hasP) { std::swap(qA, pA); std::swap(qAI, pAI); std::swap(qBlk, cBlk); }
      memset(cBlk, 0, size_t(bw) * bh * sizeof(Blk));
      memset(rowP, 0, size_t(bw) * sizeof(Blk)); memset(rowC, 0, size_t(bw) * sizeof(Blk));
      memset(pA, 0, np); memset(pAI, 0, np);
      prevAi = 0; prevPlA = 0;
      achan_swap();
    }
  }

  // A1: 2x2 box average of the co-located parent pixels for texel (gx,gy)
  inline void par_px(int gx, int gy, int& r, int& g, int& b, int& a) const {
    int x = gx * 2, y = gy * 2, s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for(int j = 0; j < 2; j++) for(int i = 0; i < 2; i++) {
      size_t o = size_t(y + j) * qw + (x + i);
      s0 += qR[o]; s1 += qG[o]; s2 += qB[o]; s3 += qA[o];
    }
    r = s0 >> 2; g = s1 >> 2; b = s2 >> 2; a = s3 >> 2;
  }
  inline int par_x(int gx, int gy) const { // dxt3 nibble parent average
    int x = gx * 2, y = gy * 2, s = 0;
    for(int j = 0; j < 2; j++) for(int i = 0; i < 2; i++) s += qX[size_t(y + j) * qw + (x + i)];
    return s >> 2;
  }
  inline const Blk& par_blk(u32 bx, u32 by) const { // co-located parent block
    u32 px = bx * 2 < qbw ? bx * 2 : qbw - 1, py = by * 2 < qbh ? by * 2 : qbh - 1;
    return qBlk[size_t(py) * qbw + px];
  }

  CtrS* slowin = nullptr;
