/* xad_codec_bits.inc -- part of xadpcm.cpp; not standalone.
 *
 * part of Codec<MD>: one mixed bit, one ADPCM code, and the header /
 * literal coders.  Included inside the class body. */
  // one mixed bit
  INLINE int mbit(Ctr** cs, int ncs, CtrS* sl, int mctx, int actxA, int actxB, int b) {
    mxC.nx = 0;
    mxC.add(G0_BIAS);
    for( int i = 0; i<ncs; i++ )
      mxC.add(STT[cs[i]->pr()]);
    if( sl )
      mxC.add(STT[sl->pr()]);
    int p = mxC.mix(mctx);
    int pa = apA.pp(p, actxA);
    int a1 = pclamp(G0_AB1, 0, 4), a2 = pclamp(G0_AB2, 0, 4);
    int pf = (a1*pa+(4-a1)*p)>>2;
    pf = pf<P_MIN ? P_MIN : pf>P_MAX ? P_MAX : pf;
    int pb = apB.pp(pf, actxB);
    pf = (a2*pb+(4-a2)*pf)>>2;
    pf = pf<P_MIN ? P_MIN : pf>P_MAX ? P_MAX : pf;
    b = rc.bit(pf, b);
    stat[scat] += CLT[pf][b];
    for( int i = 0; i<ncs; i++ )
      cs[i]->upd(b, RS_CODE);
    if( sl )
      sl->upd(b);
    mxC.upd(b);
    apA.upd(b);
    apB.upd(b);
    return b;
  }

  /* One ADPCM code, MSB-first over its raw bits (sign, then magnitude).

     FMT and BPS are compile-time here.  BPS gives the tree's trip count and the
     alphabet width that scales twenty-odd row indices below; FMT selects the
     scale-context table's height, and nothing else -- which is why it is FMT
     and not the height itself that is the parameter: M0_Md_Volume is
     `1*M0_d0.Size` in the Debug build, i.e. load-time initialised from a
     mapping object, so it is not a constant expression there.

     The three enum/const declarations shadow the runtime members with exactly
     the values begin_seg() puts in them for this segment.  The ALLOCATION sites
     in reset_stats() keep using the runtime amaxsym / amaxnib, which are the
     maxima over every segment in a solid stream and are >= these. */
  template<int FMT, int BPS>
  int code_symbol(int k, int qhat, int conf, int ib, int xch, int q2, int q3, int sym, int xch2, int qhatB, int confB) {
    enum { nsym = 1<<BPS, nbit = BPS };
    const int nib_rows = FMT ? M0_Md_Volume : I0_Ib_Volume;
    Ch &c = ch[k];
    Ctr* rQ = tQ.row(qhat*NCONF+conf);
    CtrS* rQs = tQs.row(qhat*NCONF+conf);
    Ctr* rI = tI.row(ib);
    Ctr* rP = tP.row(c.prev);
    Ctr* rP2 = tP2.row(c.prev*nsym+c.prev2);
    Ctr* rX = tX.row(xch);
    Ctr* rQP = tQP.row(qhat*nsym+c.prev);
    Ctr* rQI = tQI.row(qhat*nib_rows+ib);
    Ctr* rQX = tQX.row(qhat*(nsym+1)+xch);
    Ctr* rQ2 = tQ2.row((qhat*nsym+q2)*NCONF+conf);
    Ctr* rQ3 = tQ3.row((qhat*nsym+q2)*nsym+q3);
    Ctr* rIP = tIP.row((ib*nsym+c.prev)*NCONF+conf);
    Ctr* rPX = tPX.row(c.prev*(nsym+1)+xch);
    Ctr* rXX = tXX.row(xch*(nsym+1)+xch2);
    Ctr* rPPX = tPPX.row((c.prev*nsym+c.prev2)*(nsym+1)+xch);
    Ctr* rPX2 = tPX2.row((c.prev*(nsym+1)+xch)*(nsym+1)+xch2);
    Ctr* rP3 = tP3.row((c.prev*nsym+c.prev2)*nsym+c.prev3);
    Ctr* rPPXX = tPPXX.row(((c.prev*nsym+c.prev2)*(nsym+1)+xch)*(nsym+1)+xch2);
    Ctr* rP4 = tP4.row(((c.prev*nsym+c.prev2)*nsym+c.prev3)*nsym+c.prev4);
    Ctr* rQPX = tQPX.row((qhat*nsym+c.prev)*(nsym+1)+xch);
    u32 hj = u32(((c.prev*nsym+c.prev2)*nsym+c.prev3)*(nsym+1)+xch)*u32(nsym+1)+u32(xch2);
    Ctr* rHJ = tHJ.row(int((hj*0x9E3779B1u)>>(32-hj_bits)));
    Ctr* rQB = tQB.row(qhatB*NCONF+confB);
    Ctr* rQAB = tQAB.row(qhat*nsym+qhatB);
    /* One row per (length bucket, expected bit) plus row 0 for "no useful
       prediction", which covers both no match at all and a match the bits
       already decoded have contradicted.  Keying on the expected BIT rather
       than feeding the expectation as a sign means the counter learns how far
       to trust a match of this length, separately for each node -- and when
       the match is wrong, row 0 keeps that damage out of the trusted rows. */
    int slb = MakeMl(int(mmS.len)), llb = MakeMl(int(mmL.len));
    int sex = mmS.expect, lex = mmL.expect;
    int sok = sex>=0, lok = lex>=0;
    int msel = G0_MATCH_MIXSEL ? lok : 0;
    scat = ST_CODE;
    int nd = 1;
    for( int j = nbit-1; j>=0; j-- ) {
      int srow = 0, lrow = 0, sbit = 0, lbit = 0;
      if( sok ) {
        sbit = (sex>>j)&1;
        srow = 1+2*slb+sbit;
      }
      if( lok ) {
        lbit = (lex>>j)&1;
        lrow = 1+2*llb+lbit;
      }
      Ctr* cs[MIX_NIN];
      int n = 0;
      cs[n++] = tMS.row(srow)+nd;
      cs[n++] = tML.row(lrow)+nd;
      cs[n++] = rQ+nd;
      cs[n++] = rI+nd;
      cs[n++] = rP+nd;
      cs[n++] = rP2+nd;
      cs[n++] = rX+nd;
      cs[n++] = rQP+nd;
      cs[n++] = rQI+nd;
      cs[n++] = rQX+nd;
      cs[n++] = rIP+nd;
      cs[n++] = rQ2+nd;
      cs[n++] = rQ3+nd;
      cs[n++] = rPX+nd;
      cs[n++] = rXX+nd;
      cs[n++] = rPPX+nd;
      cs[n++] = rPX2+nd;
      cs[n++] = rP3+nd;
      cs[n++] = rPPXX+nd;
      cs[n++] = rP4+nd;
      cs[n++] = rQPX+nd;
      cs[n++] = rHJ+nd;
      cs[n++] = rQB+nd;
      cs[n++] = rQAB+nd;
      int b = mbit(cs, n, rQs+nd, (nd*NCONF+conf)*(G0_MATCH_MIXSEL ? 2 : 1)+msel, qhat*nsym+nd, ib*nsym+nd, enc ? (sym>>j)&1 : -1);
      // a match the decoded bits have contradicted says nothing about the rest
      if( sok&&b!=sbit )
        sok = 0;
      if( lok&&b!=lbit )
        lok = 0;
      nd = nd+nd+b;
    }
    int out = nd-nsym;
    mmS.update(out);
    mmL.update(out);
    return out;
  }

  // signed value as magnitude class + mantissa (zigzag)
  i64 code_signed(int model, i64 v) {
    u64 u = enc ? ((u64(v)<<1)^u64(v>>63)) : 0;
    int nb = 0;
    if( enc )
      while( nb<sgn_maxnb&&(u>>nb) )
        nb++;
    scat = ST_HDR;
    Ctr* r = tHnb.row(model);
    int nd = 1;
    for( int j = SGN_CBITS-1; j>=0; j-- ) {
      Ctr* c = r+nd;
      int p = c->pr();
      int b = rc.bit(p, enc ? (nb>>j)&1 : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_HDR);
      nd = nd+nd+b;
    }
    nb = nd-SGN_NODES;
    if( nb>sgn_maxnb )
      nb = sgn_maxnb;
    if( !enc )
      u = nb ? (u64(1)<<(nb-1)) : 0;
    for( int j = nb-2; j>=0; j-- ) {   // mantissa below the leading one
      Ctr* c = tHmn.row(model*sgn_ncls+(nb<sgn_ncls ? nb : sgn_ncls-1))+(j<SGN_NODES-1 ? j+1 : SGN_NODES-1);
      int p = c->pr();
      int b = rc.bit(p, enc ? int((u>>j)&1) : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_HDR);
      if( !enc )
        u |= u64(u32(b))<<j;
    }
    return i64(u>>1)^-i64(u&1);
  }

  int code_byte(int stream, int prev, int v) {
    scat = ST_LIT;
    Ctr* r = tLit.row(stream*LIT_W+prev);
    int nd = 1;
    for( int j = 7; j>=0; j-- ) {
      Ctr* c = r+nd;
      int p = c->pr();
      int b = rc.bit(p, enc ? (v>>j)&1 : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_LIT);
      nd = nd+nd+b;
    }
    return nd-LIT_W;
  }

  void code_bytes(int stream, u8* d, size_t n) {
    int prev = 0;
    for( size_t i = 0; i<n; i++ ) {
      int v = code_byte(stream, prev, enc ? d[i] : 0);
      if( !enc )
        d[i] = u8(v);
      prev = v;
    }
  }
