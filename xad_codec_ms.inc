/* xad_codec_ms.inc -- part of xadpcm.cpp; not standalone.
 *
 * part of Codec<MD>: the MS block walk.  Included inside the class body. */
  /* MS blocks: 7 header bytes per channel laid out field by field (coefficient
     set, iDelta, iSamp1, iSamp2, each planar across the channels), then plain
     nibbles, high one first, interleaved by channel one sample at a time -- no
     group shuffle to undo.  iSamp2 precedes iSamp1 in time; both are coded as
     residuals against the signal predictor, which is then fed with them in
     that order so the body starts from the real state. */

  /* MS is 4-bit by definition, so XST is the only compile-time parameter this
     walk needs; BPS is spelled 4 at every call into the shared bit tree. */
  template<int XST>
  void code_data_ms(u8* data, size_t n) {
    const int nch = pm.chans;
    const size_t hdr = size_t(7*nch), ba = size_t(pm.block_align);
    vector<u8> nib(ba*2+8, 0);

    for( size_t pos = 0; pos<n; ) {
      size_t left = n-pos, len = left<ba ? left : ba;
      u8* blk = data+pos;

      if( len<hdr ) { // runt tail
        code_bytes(LIT_MISC, blk, len);
        break;
      }

      // ---- block header
      for( int k = 0; k<nch; k++ ) { // coefficient set, order-1
        Ch &c = ch[k];
        int bp = code_byte(LIT_BPRED, c.bprev, enc ? blk[k] : 0);
        if( !enc )
          blk[k] = u8(bp);
        c.bprev = bp;
        int ci = bp<pm.ncoef ? bp : 0;
        c.c1 = pm.coef[ci][0];
        c.c2 = pm.coef[ci][1];
      }
      /* iDelta.  Two guesses are available and which one wins depends on the
         encoder: ffmpeg simply carries its running state across the block
         boundary (clamped to 16), making that residual exactly zero, while
         encoders that re-fit iDelta per block track their own previous header
         more closely.  Both sides can score the two from decoded data alone,
         so pick per channel by decayed absolute error rather than by guessing
         at build time -- it costs a couple of adds and picks up either. */
      for( int k = 0; k<nch; k++ ) {
        Ch &c = ch[k];
        size_t o = size_t(nch)+size_t(2*k);
        i32 pr = c.rdelta<16 ? 16 : c.rdelta, ph = c.hdelta;
        int run = c.errR<=c.errH;
        i32 p = run ? pr : ph;
        i32 v = i32(code_signed(run ? SM_DELTA : SM_DELTAH, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        int dsh = pclamp(M0_MSDSH, 1, 20);
        c.errR += u64(pr>v ? pr-v : v-pr)-(c.errR>>dsh);
        c.errH += u64(ph>v ? ph-v : v-ph)-(c.errH>>dsh);
        c.hdelta = v;
        c.set_delta(v);
      }
      for( int k = 0; k<nch; k++ ) { // iSamp2 -- first in time
        Ch &c = ch[k];
        size_t o = size_t(5*nch)+size_t(2*k);
        i32 p = c.pd.predicted_sample();
        i32 v = clip16(code_signed(SM_SAMP, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        c.samp2 = v;
        c.pd.update(v);
        c.pdB.update(v);
      }
      for( int k = 0; k<nch; k++ ) { // iSamp1 -- second in time
        Ch &c = ch[k];
        size_t o = size_t(3*nch)+size_t(2*k);
        i32 p = c.pd.predicted_sample();
        i32 v = clip16(code_signed(SM_SAMP1, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        c.samp1 = v;
        c.pd.update(v);
        c.pdB.update(v);
      }

      // ---- nibbles
      size_t body = len-hdr, nn = body*2, ngrp = nn/size_t(nch);

      if( enc )
        for( size_t j = 0; j<body; j++ ) {
          nib[2*j] = blk[hdr+j]>>4;
          nib[2*j+1] = blk[hdr+j]&15;
        }

      for( size_t i = 0; i<ngrp; i++ )
        for( int k = 0; k<nch; k++ ) {
          Ch &c = ch[k];
          size_t at = i*size_t(nch)+size_t(k);
          i32 P = ms_predict(c.samp1, c.samp2, c.c1, c.c2);
          const i32 dl = c.dl;
          const u32 rcp = c.rcp;
          i64 e = 0;
          i32 cross = 0, crossB = 0;
          if constexpr( XST ) {
            cross = (k||!G0_XPRED) ? ch[(k+nch-1)%nch].pd.d0last : ch[nch-1].pd.predict_own(G0_XPRED);
            crossB = ch[(k+nch-1)%nch].pdB.d0last;
          }
          c.pd.set_cross(cross);
          i32 d = c.pd.predict();
          int qhat = ms_quantize_r(i64(clip16(i64(c.pd.last)+d))-P, dl, rcp, e);
          c.pdB.set_cross(crossB);
          i64 eB = 0;
          int qhatB = ms_quantize_r(i64(clip16(i64(c.pdB.last)+c.pdB.predict()))-P, dl, rcp, eB);
          int q2 = ms_quantize_nq(i64(clip16(i64(c.pd.last)+c.pd.p1+c.pd.p2))-P, dl, rcp);
          int q3 = ms_quantize_nq(i64(clip16(i64(c.pd.last)+c.pd.p1))-P, dl, rcp);
          int conf = ms_conf(e, dl);
          int confB = ms_conf(eB, dl);
          int ib = ms_bucket(dl);
          Ch &o = ch[k ? 0 : nch-1];
          enum { NSYM = 16 };
          int xch = XST ? o.cur+1 : 0, xch2 = XST ? o.prev2+1 : 0;
          xch = xch>NSYM ? NSYM : xch;
          xch2 = xch2>NSYM ? NSYM : xch2;
          int sym = code_symbol<1, 4>(k, MS_M[qhat], conf, ib, xch, MS_M[q2], MS_M[q3], enc ? int(MS_M[nib[at]]) : 0, xch2, MS_M[qhatB], confB);
          int nb = MS_U[sym];
          if( !enc )
            nib[at] = u8(nb);
          i32 s = ms_apply(P, nb, c.delta);
          c.samp2 = c.samp1;
          c.samp1 = s;
          c.set_delta(ms_delta_update(c.delta, nb));
          c.prev4 = c.prev3;
          c.prev3 = c.prev2;
          c.prev2 = c.prev;
          c.prev = sym;
          c.cur = sym;
          c.pd.update(s);
          c.pdB.update(s);
        }

      for( size_t at = ngrp*size_t(nch); at<nn; at++ ) {   // a nibble left over
        int v = code_byte(LIT_MISC, pclamp(G0_LIT_CTX_NIBBLE, 0, LIT_W-1), enc ? nib[at] : 0);
        if( !enc )
          nib[at] = u8(v&15);
      }

      if( !enc )
        for( size_t j = 0; j<body; j++ )
          blk[hdr+j] = u8((nib[2*j]<<4)|nib[2*j+1]);

      for( int k = 0; k<nch; k++ )
        ch[k].rdelta = ch[k].delta;

      pos += len;
    }
  }
