/* d5c_codec_misc.inc -- included INSIDE struct Codec; not standalone.
 *
 * The two non-block coders: BC6H/BC7 mode blocks and FMT_RAW rasters. */

  //------------------------------------------------- mode-based blocks (BC6H/BC7)
  /* BC7 and BC6H have no fixed field layout: a mode field at the bottom of
   * byte 0 (the position of its lowest set bit) selects one of 8 / 14 layouts
   * that disagree about partition, endpoint and index widths, so there is
   * nothing to split into planes the way BC1-5 are split.  What they do have
   * is strong structure per mode and per byte position, and the same spatial
   * correlation between neighbouring blocks as any other texture.  So the
   * block is coded byte-wise with (mode, position in block, W block's byte at
   * the same position, N block's byte, the previous byte, the 2D gradient) as
   * context -- the same tree(), mixer and counters as everything else, and
   * far better than storing the block verbatim.  A field-aware model per mode
   * would be the next step; this is the shape it would build on. */
  Ctr* tP[6];
  Mixer mxP;
  u8  sModeRow[2][size_t(MAX_B) * MAX_BSZ];
  u8  mbW[MAX_BSZ];        // previous block
  u8 *mbP = sModeRow[0], *mbRow = sModeRow[1];  // previous / current row of blocks
  static int bc7_mode(u8 b0) { for(int i = 0; i < 8; i++) if(b0 >> i & 1) return i; return 8; }
  void code_mode_blocks(u8P data, u32 bw, u32 bh, u32 bsz) {
    memset(mbW, 0, bsz);
    memset(mbP, 0, size_t(bw) * bsz);
    u8* rowc = mbRow;
    memset(rowc, 0, size_t(bw) * bsz);
    scat = 0;
    for(u32 by = 0; by < bh; by++) {
      for(u32 bx = 0; bx < bw; bx++) {
        u8* bp = data + (size_t(by) * bw + bx) * bsz;
        const bool hasW = bx > 0, hasN = by > 0;
        int mode = 0;
        for(u32 k = 0; k < bsz; k++) {
          int Wv = hasW ? mbW[k] : 0;
          int Nv = hasN ? mbP[size_t(bx) * bsz + k] : 0;
          int NWv = (hasW && hasN) ? mbP[size_t(bx - 1) * bsz + k] : 0;
          int prev = k ? bp[k - 1] : (hasW ? mbW[bsz - 1] : 0);
          int grad = clampi(Wv + Nv - NWv, 0, 255);
          int kk = int(k) & 15;
          int v = enc ? bp[k] : -1;
          v = tree(mxP, &tP[0][size_t(MakeP1(u32(Wv), u32(kk))) * 256],
                        &tP[1][size_t(MakeP2(u32(Nv), u32(kk))) * 256],
                        &tP[2][size_t(MakeP3(u32(prev), u32(kk))) * 256],
                        &tP[3][size_t(MakeP4(u32(grad), u32(kk))) * 256], 8, v,
                        &tP[4][size_t(MakeP5(u32(mode), u32(kk))) * 256],
                        &tP[5][size_t(MakeP6(u32(Wv), u32(Nv), u32(mode))) * 256]);
          if(!enc) bp[k] = u8(v);
          if(k == 0) mode = bc7_mode(u8(v));   // every later byte sees the mode
          rowc[size_t(bx) * bsz + k] = u8(v);
        }
        memcpy(mbW, bp, bsz);
      }
      std::swap(mbP, rowc);
    }
    mbRow = rowc;   // hand the slab pointers back, whichever way they ended up
  }

  //------------------------------------------------- raster coding (FMT_RAW)
  /* A bitmap-like dds has no endpoints-and-indices structure: every pixel IS
   * an endpoint.  So the raster goes straight through the endpoint literal
   * machinery -- the same tree(), the same mixer, the same 256-row context
   * tables -- with the spatial contexts an endpoint plane already uses: W, N,
   * the 2D gradient clamp(W+N-NW), the MED/LOCO-I predictor, and for a
   * multi-channel pixel the previous channel of the same pixel (the
   * cross-channel r->g->b link the colour endpoints use).  One byte plane per
   * channel, so a 16-bit channel is simply two planes that each see their own
   * statistics -- the low byte's noise never pollutes the high byte's model. */
  u8  sRaster[2][size_t(MAX_DIM) * MAX_BPP];
  u8 *rwP = sRaster[0], *rwC = sRaster[1];   // previous / current row, bpp bytes per pixel
  u32 rww = 0, rwbpp = 0;
  void raw_init(u32 w, u32 bpp) {
    rww = w; rwbpp = bpp;
    memset(rwP, 0, size_t(w) * bpp);
    memset(rwC, 0, size_t(w) * bpp);
  }

  /* Advance the match model past one coded byte.
   *
   * The table maps "the last RM_ORDER bytes" to the position of the byte that
   * FOLLOWED them, so a hit predicts the next byte directly.  The history must
   * be a bounded window: hashing the whole prefix (which the first attempt did)
   * gives every position a unique key and the table never hits at all. */
  D5C_FORCE_INLINE void rm_step(int c) {
    if(rm_have && u32(rm_buf[rm_match]) == u32(c)) {
      rm_match++; if(rm_len < 63) rm_len++;
      if(rm_match >= rm_cur) rm_have = false;      // caught up with ourselves
    } else { rm_have = false; rm_len = 0; }
    rm_cur++;
    rm_hist = (rm_hist << 8) | u64(u8(c));
    const u64 ctx = rm_hist & ((1ull << (8 * RM_ORDER)) - 1);
    const u32 h = u32((ctx * 0x9E3779B97F4A7C15ull) >> (64 - RM_BITS));
    if(!rm_have) {
      const u32 cand = rmh[h];
      if(cand && cand < rm_cur) { rm_match = cand; rm_have = true; rm_len = 0; }
    }
    rmh[h] = u32(rm_cur);
  }
  void code_raster(u8P data, u32 w, u32 h, u32 bpp) {
    raw_init(w, bpp);
    memset(rmh, 0, (size_t(1) << RM_BITS) * sizeof(u32));
    rm_buf = data; rm_match = rm_cur = 0; rm_len = 0; rm_have = false; rm_hist = 0;
    scat = 0;
    for(u32 y = 0; y < h; y++) {
      u8* row = data + size_t(y) * w * bpp;
      for(u32 x = 0; x < w; x++) {
        const bool hasW = x > 0, hasN = y > 0;
        for(u32 c = 0; c < bpp; c++) {
          const size_t o = size_t(x) * bpp + c;
          int Wv = hasW ? rwC[o - bpp] : 0;
          int Nv = hasN ? rwP[o] : 0;
          int NWv = (hasW && hasN) ? rwP[o - bpp] : 0;
          int prevc = c ? rwC[o - 1] : (hasW ? rwC[o - bpp] : 0);
          int NEv = (hasN && x + 1 < w) ? rwP[o + bpp] : 0;
          int NNv = hasN ? rwP[o] : 0;              // rwP is the row above
          int p2 = clampi(Wv + Nv - NWv, 0, 255);
          /* Local activity: how rough the causal neighbourhood is.  A flat
           * region and a busy one want different weights, and nothing in the
           * raster contexts carried that. */
          int actv = (Wv > NWv ? Wv - NWv : NWv - Wv) + (Nv > NWv ? Nv - NWv : NWv - Nv)
                   + (Nv > NEv ? Nv - NEv : NEv - Nv);
          const u32 act = u32(actv > 255 ? 15 : actv >> 4);
          const u32 neq = u32((NEv >> 5) * 8 + (NNv >> 5));
          int mx_ = Wv > Nv ? Wv : Nv, mn_ = Wv < Nv ? Wv : Nv;
          int med = NWv >= mx_ ? mn_ : NWv <= mn_ ? mx_ : Wv + Nv - NWv;
          int v = enc ? row[o] : -1;
          /* Match prediction for this byte: pb is what followed the same recent
           * context last time it occurred.  lb buckets the verified length so
           * the model can learn how much to trust it. */
          const int pb = rm_have ? int(rm_buf[rm_match]) : -1;
          const u32 lb = !rm_have ? 0u : rm_len == 0 ? 1u : rm_len < 4 ? 2u
                       : rm_len < 8 ? 3u : rm_len < 16 ? 4u : rm_len < 32 ? 5u
                       : rm_len < 63 ? 6u : 7u;
          v = tree_rm(mxA0, tA0W.row(int(MakeAE0W(u32(Wv)))), tA0N.row(int(MakeAE0N(u32(Nv)))),
                      tA0P.row(int(MakeAE0P(u32(p2)))), tA0o.row(0), 8, v,
                      tA0M.row(int(MakeAE0M(u32(med)))),
                      tA1A.row(int(MakeAE1A(u32(prevc)))), pb, lb, act, neq);
          if(!enc) row[o] = u8(v);
          rwC[o] = u8(v);
          rm_step(v);
        }
      }
      std::swap(rwP, rwC);
    }
  }
