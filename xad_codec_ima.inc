/* xad_codec_ima.inc -- part of xadpcm.cpp; not standalone.
 *
 * part of Codec<MD>: the payload dispatch and the IMA block walk.
 * Included inside the class body. */
  // ---------------------------------------------------------------- payload

  /* One pass over the ADPCM data in block order.  Encoder and decoder walk the
     same geometry and run the same predictor, so nothing about the layout or
     the prediction has to be stored. */

  /* The one runtime dispatch: it runs once per segment, and everything the
     walk is going to be asked about the geometry is settled by it.  MS is
     4-bit by definition (analyze and get_seg both refuse anything else), so it
     needs only the stereo split; IMA gets one instance per code width as well.
     The bps default arm is 4 -- it is unreachable, since both entry points
     validate bps to [2,5]. */
  void code_data(u8P data, size_t n) {
    const int xst = pm.chans>1;
    if( pm.format==FMT_MS ) {
      if( xst )
        code_data_ms<1>(data, n);
      else
        code_data_ms<0>(data, n);
      return;
    }
    switch( pm.bps ) {
    case 2:
      xst ? code_data_ima<2, 1>(data, n) : code_data_ima<2, 0>(data, n);
      break;
    case 3:
      xst ? code_data_ima<3, 1>(data, n) : code_data_ima<3, 0>(data, n);
      break;
    case 5:
      xst ? code_data_ima<5, 1>(data, n) : code_data_ima<5, 0>(data, n);
      break;
    default:
      xst ? code_data_ima<4, 1>(data, n) : code_data_ima<4, 0>(data, n);
      break;
    }
  }

  template<int BPS, int XST>
  void code_data_ima(u8P data, size_t n) {
    enum { bps = BPS };
    const int nch = pm.chans, hdr = nch*4, ba = pm.block_align;
    build_ima_tables<BPS>();
    /* The shared scratch slabs -- see the note above Codec in xad_codec.inc for
       why they live at namespace scope and not here, and why not on the stack.
       Cleared per channel and per call, exactly as assign(n, 0) did, and only
       over the live prefix.  The clear is load-bearing rather than hygiene:
       put_bits ORs into the nibble buffer, so a stale high nibble left in a
       block's final partial byte by an earlier segment would reach the output. */
    u8 (&cbuf)[MAX_CHANS][BA_MAX+8] = scr_nib;
    int (&code)[MAX_CHANS][SCR_CODE_N] = scr_code;
    for( int k = 0; k<nch; k++ ) {
      memset(cbuf[k], 0, size_t(ba)+8);
      memset(code[k], 0, (size_t(ba)*8/size_t(bps)+8)*sizeof(int));
    }

    for( size_t pos = 0; pos<n; ) {
      size_t left = n-pos, len = left<size_t(ba) ? left : size_t(ba);
      u8P blk = data+pos;

      if( len<size_t(hdr) ) { // runt tail
        code_bytes(LIT_MISC, blk, len);
        break;
      }

      // ---- block header: first sample, step index, reserved byte
      for( int k = 0; k<nch; k++ ) {
        Ch &c = ch[k];
        i32 pred = c.pd.predicted_sample();
        i32 val = i32(code_signed(SM_SAMP, enc ? i64(i16_of(blk+k*4))-pred : 0)+pred);
        val = val<-32768 ? -32768 : val>32767 ? 32767 : val;
        int ix = int(code_signed(SM_INDEX, enc ? i64(blk[k*4+2])-c.index : 0)+c.index)&0xFF;
        int rsv = enc ? (blk[k*4+3]!=0) : 0;
        scat = ST_FLAG;
        CtrP rf = tRsv.row(0)+1;
        int p = rf->pr();
        rsv = rc.bit(p, enc ? rsv : -1);
        XSTAT(ST_FLAG, p, rsv);
        rf->upd(rsv, RS_LIT);
        int rb = rsv ? code_byte(LIT_MISC, 0, enc ? blk[k*4+3] : 0) : 0;
        if( !enc ) {
          blk[k*4] = u8(val);
          blk[k*4+1] = u8(u32(val)>>8);
          blk[k*4+2] = u8(ix);
          blk[k*4+3] = u8(rb);
        }
        c.pcm = val;
        c.index = ix>88 ? 88 : ix;
        c.pd.update(val);
        c.pdB.update(val); // the block's own first sample
      }

      size_t body = len-size_t(hdr), groups = body/size_t(hdr), rem = body%size_t(hdr);
      size_t cbytes = groups*4, bits = cbytes*8, ncode = bits/size_t(bps), tailbits = bits%size_t(bps);

      if( enc )
        for( int k = 0; k<nch; k++ ) {
          for( size_t j = 0; j<cbytes; j++ )
            cbuf[k][j] = blk[size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3)];
          for( size_t i = 0; i<ncode; i++ )
            code[k][i] = int(get_bits(&cbuf[k][0], i*size_t(bps), bps));
        }

      // ---- codes, in time order so a channel can see the others at t
      for( size_t i = 0; i<ncode; i++ )
        for( int k = 0; k<nch; k++ ) {
          Ch &c = ch[k];
          int step = step_table[c.index];
          /* Hoisted above the cascades purely so the early rows can be
             prefetched behind them.  These are pure functions of state that is
             already final at this point -- no reordering of anything that
             writes. */
          const int ib = MakeIb(c.index);
          Ch &o = ch[k ? 0 : nch-1];
          enum { NSYM = 1<<BPS };
          int xch = XST ? o.cur+1 : 0, xch2 = XST ? o.prev2+1 : 0;
          xch = xch>NSYM ? NSYM : xch;
          xch2 = xch2>NSYM ? NSYM : xch2;
          prefetch_early<0, BPS>(c, ib, xch, xch2);
          int leftover = 0;
          /* XST folds the whole cross-channel apparatus away for mono, where
             ch[k-1] is ch[k] itself and every one of these reads back zero. */
          i32 cross = 0, crossB = 0;
          if constexpr( XST ) {
            cross = (k||!G0_XPRED) ? ch[(k+nch-1)%nch].pd.d0last : ch[nch-1].pd.predict_own(G0_XPRED);
            crossB = ch[(k+nch-1)%nch].pdB.d0last;
          }
          /* v4 §2.2 proposes interleaving the two cascades, which are genuinely
             independent.  Hoisting both predicts adjacent so the scheduler can
             overlap them was MEASURED: neutral under clang and ~1.8% worse
             under gcc (register pressure -- both cascades' p1..p4 then live
             across the whole pair).  Left in the original order.  A real
             interleave would have to fuse the dot loops themselves, which is a
             different change; after the SIMD work the NLMS block is no longer
             where the time is, so it is not obviously worth it. */
          c.pd.set_cross(cross);
          i32 dhat = c.pd.predict();
          int qhat = ima_quantize<BPS>(dhat, step, leftover);
          c.pdB.set_cross(crossB);
          int loB = 0;
          int qhatB = ima_quantize<BPS>(c.pdB.predict(), step, loB);
          int q2 = ima_quantize_nl<BPS>(c.pd.p1+c.pd.p2, step);
          int q3 = ima_quantize_nl<BPS>(c.pd.p1, step);
          const int base = step>>(BPS-1);
          int conf = ima_conf(leftover, base);
          int confB = ima_conf(loB, base);
          int sym = code_symbol<0, BPS>(k, qhat, conf, ib, xch, q2, q3, enc ? code[k][i] : 0, xch2, qhatB, confB);
          if( !enc )
            code[k][i] = sym;
          const int ix = c.index;
          i32 np = c.pcm+ima_mag[ix][sym];
          c.pcm = np<-32768 ? -32768 : np>32767 ? 32767 : np;
          c.index = ima_nxt[ix][sym];
          c.prev4 = c.prev3;
          c.prev3 = c.prev2;
          c.prev2 = c.prev;
          c.prev = sym;
          c.cur = sym;
          c.pd.update(c.pcm);
          c.pdB.update(c.pcm);
        }

      if( !enc )
        for( int k = 0; k<nch; k++ ) {
          memset(&cbuf[k][0], 0, cbytes);
          for( size_t i = 0; i<ncode; i++ )
            put_bits(&cbuf[k][0], i*size_t(bps), bps, u32(code[k][i]));
        }

      // ---- unused trailing bits of the last group, then any partial group
      if( tailbits )
        for( int k = 0; k<nch; k++ ) {
          int v = enc ? int(get_bits(&cbuf[k][0], ncode*size_t(bps), int(tailbits))) : 0;
          v = code_byte(LIT_MISC, (pclamp(G0_LIT_CTX_TAILBITS, 0, LIT_W-MAX_CHANS)+k)&(LIT_W-1), v);
          if( !enc )
            put_bits(&cbuf[k][0], ncode*size_t(bps), int(tailbits), u32(v));
        }

      if( !enc )
        for( int k = 0; k<nch; k++ )
          for( size_t j = 0; j<cbytes; j++ )
            blk[size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3)] = cbuf[k][j];

      if( rem )
        code_bytes(LIT_MISC, blk+size_t(hdr)+groups*size_t(hdr), rem);

      pos += len;
    }
  }
