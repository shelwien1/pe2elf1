//=============================================================================
// Arithmetic entropy encoding (JPEG T.81 Annex D and F): the mirror of the
// arithmetic decoders in entropy.inc, the way jpgenc.inc mirrors the Huffman
// ones.
//
// Ported from libjpeg's jcarith.c, which is where entropy.inc's decoders came
// from.  The QM coder's termination (T.81 D.1.8) has more than one conforming
// answer -- any C in the final interval works -- and libjpeg's "Pacman" flush,
// which picks the value with the most trailing zeroes and then drops them, is
// the one the files this has to reproduce were made with.  Everything else in
// here is forced by the standard, so there is nothing to choose.
//
// Two departures from jcarith.c, both because of what pjpg keeps:
//   - coefficients are stored in zigzag order, so where libjpeg indexes
//     jpeg_natural_order[k] this indexes k directly;
//   - the point transform on a first scan is already applied to the stored
//     value (the decoders store coefficients as coded), so where libjpeg
//     shifts by Al on the way out, this does not.
//=============================================================================

struct AriOut {
  byte* buf;
  qword len, cap;
  uint  bad;

  // The coder's registers.  C is unsigned so that its shifts stay defined;
  // it never exceeds 28 bits, which is the register width T.81 D.1.3 gives it.
  uint c;
  int  a, ct, buffer, sc, zc;

  // Per-scan statistics, exactly libjpeg's areas.
  byte dc_stats[16][64];
  byte ac_stats[16][256];
  byte fix[4];                  // the non-adapting bin, T.851 10.3
  int  last_dc[4];
  int  dc_ctx[4];

  void put( uint v ) { if( len<cap ) buf[len++] = byte(v); else bad = 1; }
  void put_zc( void ) { while( zc>0 ) { put(0x00); zc--; } }

  void reset_coder( void ) {
    c=0; a=0x10000; sc=0; zc=0; ct=11; buffer=-1;   // buffer<0 == empty
    fix[0]=113; fix[1]=0; fix[2]=0; fix[3]=0;
  }

  void init( byte* p, qword n ) { buf=p; cap=n; len=0; bad=0; reset_coder(); }

  // libjpeg's arith_encode: the QM coder of T.81 D.1.4 through D.1.6.
  void encode( byte* st, int val ) {
    int qe,temp,sv;
    byte nl,nm;

    sv = *st;
    qe = pjpg::jaritab[ sv & 0x7F ];
    nl = byte(qe & 0xFF); qe >>= 8;     // Next_Index_LPS + Switch_MPS
    nm = byte(qe & 0xFF); qe >>= 8;     // Next_Index_MPS

    a -= qe;
    if( val != (sv >> 7) ) {
      // The less probable symbol.  When its interval is the larger of the
      // two the roles are swapped, which is the whole conditional-exchange
      // trick of D.1.4.
      if( a >= qe ) { c += uint(a); a = qe; }
      *st = byte( (sv & 0x80) ^ nl );
    } else {
      if( a >= 0x8000 ) return;         // nothing to renormalize
      if( a < qe ) { c += uint(a); a = qe; }
      *st = byte( (sv & 0x80) ^ nm );
    }

    // Renormalization and byte output, D.1.6.  A byte is not emitted the
    // moment it is ready: a later carry can still increment it, so 0xFF runs
    // (which would propagate the carry) are counted in sc and zero runs in zc
    // until something settles them.
    do {
      a <<= 1;
      c <<= 1;
      if( --ct == 0 ) {
        temp = int( c >> 19 );
        if( temp > 0xFF ) {
          // The carry landed: the held byte goes up by one, and every stacked
          // 0xFF becomes 0x00.
          if( buffer >= 0 ) {
            put_zc();
            put( uint(buffer + 1) );
            if( buffer + 1 == 0xFF ) put( 0x00 );
          }
          zc += sc; sc = 0;
          // The three spacer bits in C guarantee this cannot itself be 0xFF.
          buffer = temp & 0xFF;
        } else if( temp == 0xFF ) {
          sc++;                         // stack it; a carry may still change it
        } else {
          // Nothing can carry into what is held any more, so flush it all.
          if( buffer == 0 ) zc++;
          else if( buffer >= 0 ) { put_zc(); put( uint(buffer) ); }
          if( sc ) {
            put_zc();
            do { put(0xFF); put(0x00); } while( --sc );
          }
          buffer = temp & 0xFF;
        }
        c &= 0x7FFFF;
        ct += 8;
      }
    } while( a < 0x8000 );
  }

  void encode_fixed( int val ) { encode( fix, val ); }

  // libjpeg's finish_pass: T.81 D.1.8, picking the shortest C in the final
  // interval and dropping the trailing zeroes it does not need.
  void finish( void ) {
    uint temp;
    temp = uint(a - 1) + c;
    temp &= 0xFFFF0000u;
    c = (temp < c) ? (temp + 0x8000) : temp;
    c <<= ct;
    if( c & 0xF8000000u ) {
      // One final carry to push through.
      if( buffer >= 0 ) {
        put_zc();
        put( uint(buffer + 1) );
        if( buffer + 1 == 0xFF ) put( 0x00 );
      }
      zc += sc; sc = 0;
    } else {
      if( buffer == 0 ) zc++;
      else if( buffer >= 0 ) { put_zc(); put( uint(buffer) ); }
      if( sc ) {
        put_zc();
        do { put(0xFF); put(0x00); } while( --sc );
      }
    }
    // Trailing zero bytes carry no information, so they are simply not
    // written -- that is what makes this the shortest conforming stream.
    if( c & 0x7FFF800u ) {
      put_zc();
      put( (c>>19) & 0xFF );
      if( ((c>>19) & 0xFF)==0xFF ) put( 0x00 );
      if( c & 0x7F800u ) {
        put( (c>>11) & 0xFF );
        if( ((c>>11) & 0xFF)==0xFF ) put( 0x00 );
      }
    }
  }
};

// ---- per-scan statistics ----------------------------------------------------
// The mirror of entropy.inc's arith_reset_stats(): which areas exist, and
// which are cleared, depends on what the scan actually codes.

static void ari_reset_stats( AriOut& o, pjpg& P ) {
  uint i;
  for( i=0; i<uint(P.comps_in_scan); i++ ) {
    uint c = P.cs_cmp[i];
    if( (P.progressive_mode==0) || ((P.Ss==0) && (P.Ah==0)) ) {
      bzero( o.dc_stats[ uint(P.comp_info[c].dc_tbl_no) & 15 ] );
      o.last_dc[i] = 0; o.dc_ctx[i] = 0;
    }
    if( (P.progressive_mode==0) || P.Ss )
      bzero( o.ac_stats[ uint(P.comp_info[c].ac_tbl_no) & 15 ] );
  }
}

static void ari_restart( AriOut& o, pjpg& P, uint rst_num ) {
  o.finish();
  o.put( 0xFF );
  o.put( 0xD0 + (rst_num & 7) );
  ari_reset_stats( o, P );
  o.reset_coder();
}

// ---- DC ---------------------------------------------------------------------
// libjpeg's Encode_DC_DIFF (T.81 Figure F.4) with the conditioning of
// F.1.4.4.1.2.  val is the coefficient as it was coded, so the point transform
// on a progressive first scan has already been applied.

static void ari_enc_dc( AriOut& o, pjpg& P, uint ci, uint tbl, int val ) {
  byte* st;
  int v,v2,m;

  st = o.dc_stats[tbl] + o.dc_ctx[ci];

  if( (v = val - o.last_dc[ci]) == 0 ) {
    o.encode( st, 0 );
    o.dc_ctx[ci] = 0;                   // zero difference
    return;
  }
  o.last_dc[ci] = val;
  o.encode( st, 1 );

  // Figure F.7: the sign, which also picks the magnitude's context
  if( v > 0 ) { o.encode( st+1, 0 ); st += 2; o.dc_ctx[ci] = 4; }
  else        { v = -v; o.encode( st+1, 1 ); st += 3; o.dc_ctx[ci] = 8; }

  // Figure F.8: the magnitude category, coded as a unary run
  m = 0;
  if( v -= 1 ) {
    o.encode( st, 1 );
    m = 1; v2 = v;
    st = o.dc_stats[tbl] + 20;          // Table F.4: X1
    while( v2 >>= 1 ) { o.encode( st, 1 ); m <<= 1; st += 1; }
  }
  o.encode( st, 0 );

  if( m < ((1 << P.arith_dc_L[tbl]) >> 1) )      o.dc_ctx[ci] = 0;
  else if( m > ((1 << P.arith_dc_U[tbl]) >> 1) ) o.dc_ctx[ci] += 8;

  // Figure F.9: the bits below the category's leading one
  st += 14;
  while( m >>= 1 ) o.encode( st, (m & v) ? 1 : 0 );
}

// ---- AC ---------------------------------------------------------------------
// The magnitude half of Figure F.5, shared by the sequential and AC-first
// paths.  v is the coefficient with the point transform applied and the sign
// already coded; st points at the coefficient's own bin.

static void ari_enc_ac_val( AriOut& o, pjpg& P, uint tbl, byte* st, int k, int v ) {
  int v2,m;
  st += 2;
  m = 0;
  if( v -= 1 ) {
    o.encode( st, 1 );
    m = 1; v2 = v;
    if( v2 >>= 1 ) {
      o.encode( st, 1 );
      m <<= 1;
      st = o.ac_stats[tbl] + ( (k <= int(P.arith_ac_K[tbl])) ? 189 : 217 );
      while( v2 >>= 1 ) { o.encode( st, 1 ); m <<= 1; st += 1; }
    }
  }
  o.encode( st, 0 );
  st += 14;
  while( m >>= 1 ) o.encode( st, (m & v) ? 1 : 0 );
}

// ---- MCU encoders -----------------------------------------------------------
// One per scan shape, matching entropy.inc's ar_mcu_*.  Each returns 0 only
// when the coefficients it needs are not there.

static uint ari_mcu_seq( AriOut& o, pjpg& P ) {
  uint blkn,ci,cc,tbl; int k,ke,v; byte* st; short* b;
  for( blkn=0; blkn<uint(P.blocks_in_MCU); blkn++ ) {
    ci = uint(P.MCU_membership[blkn]);
    cc = P.cs_cmp[ci];
    b = P.co_block( blkn );
    if( b==0 ) return 0;

    tbl = uint(P.comp_info[cc].dc_tbl_no) & 15;
    ari_enc_dc( o, P, ci, tbl, int(b[0]) );

    tbl = uint(P.comp_info[cc].ac_tbl_no) & 15;
    for( ke=63; ke>0; ke-- ) if( b[ke] ) break;

    for( k=1; k<=ke; k++ ) {
      st = o.ac_stats[tbl] + 3*(k-1);
      o.encode( st, 0 );                        // not the end of the block
      while( (v = int(b[k]))==0 ) { o.encode( st+1, 0 ); st += 3; k++; }
      o.encode( st+1, 1 );
      if( v > 0 ) o.encode_fixed( 0 );
      else        { v = -v; o.encode_fixed( 1 ); }
      ari_enc_ac_val( o, P, tbl, st, k, v );
    }
    if( k<=63 ) o.encode( o.ac_stats[tbl] + 3*(k-1), 1 );   // end of block
  }
  return 1;
}

static uint ari_mcu_dc_first( AriOut& o, pjpg& P ) {
  uint blkn,ci,cc,tbl; short* b;
  for( blkn=0; blkn<uint(P.blocks_in_MCU); blkn++ ) {
    ci = uint(P.MCU_membership[blkn]);
    cc = P.cs_cmp[ci];
    b = P.co_block( blkn );
    if( b==0 ) return 0;
    tbl = uint(P.comp_info[cc].dc_tbl_no) & 15;
    // The decoder stored the value already scaled by the point transform, so
    // undoing it is the arithmetic shift libjpeg spells IRIGHT_SHIFT.
    ari_enc_dc( o, P, ci, tbl, int(b[0]) >> P.Al );
  }
  return 1;
}

static uint ari_mcu_dc_refine( AriOut& o, pjpg& P ) {
  uint blkn; short* b;
  for( blkn=0; blkn<uint(P.blocks_in_MCU); blkn++ ) {
    b = P.co_block( blkn );
    if( b==0 ) return 0;
    o.encode_fixed( (int(b[0]) >> P.Al) & 1 );
  }
  return 1;
}

// An AC scan is always single-component, so one block per MCU.
static uint ari_mcu_ac_first( AriOut& o, pjpg& P ) {
  uint tbl; int k,ke,v; byte* st; short* b;
  b = P.co_block( 0 );
  if( b==0 ) return 0;
  tbl = uint(P.comp_info[P.cs_cmp[0]].ac_tbl_no) & 15;

  // The point transform on an AC coefficient rounds towards zero, so the
  // magnitude is shifted and the sign put back afterwards.
  for( ke=int(P.Se); ke>0; ke-- ) {
    v = int(b[ke]); if( v<0 ) v = -v;
    if( v >> P.Al ) break;
  }

  for( k=int(P.Ss); k<=ke; k++ ) {
    st = o.ac_stats[tbl] + 3*(k-1);
    o.encode( st, 0 );
    for(;;) {
      v = int(b[k]);
      if( v >= 0 ) { if( (v >>= P.Al)!=0 ) { o.encode( st+1, 1 ); o.encode_fixed( 0 ); break; } }
      else         { v = -v; if( (v >>= P.Al)!=0 ) { o.encode( st+1, 1 ); o.encode_fixed( 1 ); break; } }
      o.encode( st+1, 0 ); st += 3; k++;
    }
    ari_enc_ac_val( o, P, tbl, st, k, v );
  }
  if( k <= int(P.Se) ) o.encode( o.ac_stats[tbl] + 3*(k-1), 1 );
  return 1;
}

static uint ari_mcu_ac_refine( AriOut& o, pjpg& P ) {
  uint tbl; int k,ke,kex,v; byte* st; short* b;
  b = P.co_block( 0 );
  if( b==0 ) return 0;
  tbl = uint(P.comp_info[P.cs_cmp[0]].ac_tbl_no) & 15;

  for( ke=int(P.Se); ke>0; ke-- ) {
    v = int(b[ke]); if( v<0 ) v = -v;
    if( v >> P.Al ) break;
  }
  // Where the previous scan's block ended: below that index the decoder knows
  // there is more to come, so no end-of-block decision is coded.
  for( kex=ke; kex>0; kex-- ) {
    v = int(b[kex]); if( v<0 ) v = -v;
    if( v >> P.Ah ) break;
  }

  for( k=int(P.Ss); k<=ke; k++ ) {
    st = o.ac_stats[tbl] + 3*(k-1);
    if( k > kex ) o.encode( st, 0 );
    for(;;) {
      v = int(b[k]);
      if( v >= 0 ) {
        if( (v >>= P.Al)!=0 ) {
          if( v >> 1 ) o.encode( st+2, v & 1 );           // already nonzero
          else { o.encode( st+1, 1 ); o.encode_fixed( 0 ); }
          break;
        }
      } else {
        v = -v;
        if( (v >>= P.Al)!=0 ) {
          if( v >> 1 ) o.encode( st+2, v & 1 );
          else { o.encode( st+1, 1 ); o.encode_fixed( 1 ); }
          break;
        }
      }
      o.encode( st+1, 0 ); st += 3; k++;
    }
  }
  if( k <= int(P.Se) ) o.encode( o.ac_stats[tbl] + 3*(k-1), 1 );
  return 1;
}
