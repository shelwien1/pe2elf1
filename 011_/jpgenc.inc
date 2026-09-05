
//=============================================================================
// Huffman entropy encoding: the mirror of the decoders in entropy.inc.
//
// The decoders read a scan and pjpg keeps the coefficients; these write those
// coefficients back out.  The whole point is that the result must be the SAME
// BYTES, not merely an equivalent stream, so every choice the encoder makes has
// to be the one the original encoder made -- and where that cannot be known,
// the caller compares and stores the scan verbatim instead.
//
// Which is why these mirror libjpeg's jchuff.c and jcphuff.c rather than being
// written afresh: the files this has to reproduce were overwhelmingly made by
// libjpeg, and its choices -- when an EOB run is flushed, how a run of sixteen
// zeroes is split, which bit pads the last byte -- are what "the same bytes"
// means in practice.
//=============================================================================

// ---- canonical code assignment ---------------------------------------------
//
// T.81 C.2: codes are assigned in order of increasing length, and within a
// length in the order the symbols appear in huffval.  So the DHT segment that
// the decoder built its tables from determines the encoder's codes completely
// -- there is nothing to choose.

struct e_tbl {
  uint code[256];
  uint size[256];        // 0 = the table does not define this symbol
  uint ok;
  uint max_eobrun;       // the longest EOB run the table can express
};

static uint make_etbl( const JHUFF_TBL& h, e_tbl& t ) {
  uint p,i,l,si,code,n;
  byte huffsize[257];
  bzero( t.code ); bzero( t.size );
  t.ok = 0; t.max_eobrun = 0;

  p = 0;
  for( l=1; l<=16; l++ ) {
    n = h.bits[l];
    if( p+n > 256 ) return 0;
    for( i=0; i<n; i++ ) huffsize[p++] = byte(l);
  }
  huffsize[p] = 0;
  n = p;

  code = 0; si = huffsize[0]; p = 0;
  while( huffsize[p] ) {
    while( huffsize[p]==si ) {
      if( p>=n ) break;
      t.code[ h.huffval[p] ] = code;
      t.size[ h.huffval[p] ] = si;
      code++; p++;
    }
    if( code >= (1u<<si) ) return 0;      // not a prefix code
    code <<= 1; si++;
    if( si>16 ) break;
  }

  // The EOBn symbols are 0x00, 0x10 .. 0xE0; the largest one the table defines
  // bounds how long a single EOB run may be.
  for( i=0; i<15; i++ ) if( t.size[i<<4] ) t.max_eobrun = (1u<<i) - 1 + (1u<<i);
  t.max_eobrun = 0;
  for( i=14; ; i-- ) { if( t.size[i<<4] ) { t.max_eobrun = (2u<<i) - 1; break; } if( i==0 ) break; }
  t.ok = 1;
  return 1;
}

// ---- the bit writer --------------------------------------------------------
//
// FF has to be followed by a stuffed 00 inside entropy-coded data, because
// otherwise it reads as a marker (T.81 B.1.1.5).

struct BitOut {
  byte* buf;
  qword len, cap;
  uint  acc, nbits;      // bits waiting, most significant first
  uint  bad;

  void init( byte* p, qword c ) { buf=p; cap=c; len=0; acc=0; nbits=0; bad=0; }

  void byte_out( uint c ) {
    if( len >= cap ) { bad = 1; return; }
    buf[len++] = byte(c);
    if( c==0xFF ) { if( len>=cap ) { bad=1; return; } buf[len++] = 0; }
  }

  void put_bits( uint code, uint size ) {
    if( size==0 || size>24 ) { if(size) bad=1; return; }
    acc = (acc << size) | (code & ((1u<<size)-1));
    nbits += size;
    while( nbits >= 8 ) { nbits -= 8; byte_out( (acc >> nbits) & 0xFF ); }
  }

  // T.81 F.1.2.3: the last byte of a scan is padded out.  Which value is not
  // fixed by the standard -- libjpeg pads with ones, others pad with zeroes --
  // so the bit that was there is recorded when the file is taken apart and
  // handed back here.
  void flush( uint padbit ) {
    if( nbits>0 ) {
      uint n = 8-nbits;
      put_bits( padbit ? ((1u<<n)-1) : 0, n );
    }
    acc = 0; nbits = 0;
  }

  // A restart marker resets the coder, and it is written into the byte stream
  // unstuffed.
  void restart( uint n, uint padbit ) {
    flush( padbit );
    if( len+2 > cap ) { bad = 1; return; }
    buf[len++] = 0xFF;
    buf[len++] = byte(M_RST0 + (n & 7));
  }
};

// ---- the value encoders ----------------------------------------------------

// T.81 F.1.2.1: a value is coded as a category (how many bits it needs) plus
// those bits, with negatives stored one less than their magnitude complement.
static uint e_cat( int v ) {
  uint n; uint a = uint( (v<0) ? -v : v );
  n = 0; while( a ) { a >>= 1; n++; }
  return n;
}
static uint e_bits_of( int v, uint n ) {
  return uint( (v<0) ? (v - 1 + (1<<n)) : v ) & ((n<32) ? ((1u<<n)-1) : 0xFFFFFFFFu);
}

static uint e_emit( BitOut& o, e_tbl& t, uint sym ) {
  if( sym>255 || t.size[sym]==0 ) return 0;      // the table cannot say this
  o.put_bits( t.code[sym], t.size[sym] );
  return 1;
}

// ---- sequential ------------------------------------------------------------

static uint enc_block_seq( BitOut& o, e_tbl& dc, e_tbl& ac, const short* b, int* last_dc ) {
  int  k,run,v,diff;
  uint n;
  diff = int(b[0]) - *last_dc;
  *last_dc = int(b[0]);
  // No range check on the category beyond what a symbol can hold: T.81's 0..11
  // for DC and 1..10 for AC are the 8-bit sequential limits, and a differential
  // frame codes differences that run past them.  Whether a value can be written
  // is decided by whether the table the file itself supplied has a symbol for
  // it, which e_emit answers.
  n = e_cat( diff );
  if( n>15 ) return 0;
  if( !e_emit(o,dc,n) ) return 0;
  if( n ) o.put_bits( e_bits_of(diff,n), n );

  run = 0;
  for( k=1; k<64; k++ ) {
    v = int(b[k]);
    if( v==0 ) { run++; continue; }
    while( run>15 ) { if( !e_emit(o,ac,0xF0) ) return 0; run -= 16; }
    n = e_cat( v );
    if( n>15 ) return 0;
    if( !e_emit(o,ac,uint(run<<4)|n) ) return 0;
    o.put_bits( e_bits_of(v,n), n );
    run = 0;
  }
  if( run>0 ) { if( !e_emit(o,ac,0x00) ) return 0; }   // EOB
  return 1;
}

// ---- progressive DC --------------------------------------------------------

static uint enc_dc_first( BitOut& o, e_tbl& dc, const short* b, int* last_dc, uint Al ) {
  int diff,v; uint n;
  v = int(b[0]) >> Al;                 // arithmetic shift: the scan codes the top bits
  diff = v - *last_dc;
  *last_dc = v;
  n = e_cat( diff );
  if( n>15 ) return 0;
  if( !e_emit(o,dc,n) ) return 0;
  if( n ) o.put_bits( e_bits_of(diff,n), n );
  return 1;
}

static void enc_dc_refine( BitOut& o, const short* b, uint Al ) {
  o.put_bits( (uint(int(b[0]) >> Al)) & 1, 1 );
}

// ---- progressive AC --------------------------------------------------------
//
// This is where reproducing the original bytes stops being mechanical.  An EOB
// run spans blocks, and in a refinement scan the correction bits of the blocks
// it covers are held back and written when it is flushed -- so the encoder
// carries state across the whole scan, and when it flushes is a choice.
// libjpeg's choices are the ones followed here (jcphuff.c), because they are
// what the files being reproduced were made with.

enum { E_MAX_CORR_BITS = 1000 };   // libjpeg's MAX_CORR_BITS
enum { E_MAX_EOBRUN    = 0x7FFF }; // and the counter it forces out at

struct AcState {
  uint eobrun;
  byte corr[E_MAX_CORR_BITS + 64];   // correction bits waiting behind it
  uint ncorr;
  void reset( void ) { eobrun=0; ncorr=0; }
};

static void e_put_buffered( BitOut& o, const byte* p, uint n ) {
  uint i;
  for( i=0; i<n; i++ ) o.put_bits( p[i], 1 );
}

// libjpeg's emit_eobrun: the EOBn symbol, its low bits, then every correction
// bit that was waiting behind it.
static uint enc_eobrun( BitOut& o, e_tbl& ac, AcState& st ) {
  uint n,r;
  if( st.eobrun>0 ) {
    r = 0; n = st.eobrun;
    while( n >>= 1 ) r++;
    if( r>14 ) return 0;
    if( !e_emit(o,ac,r<<4) ) return 0;
    if( r ) o.put_bits( st.eobrun, r );
    st.eobrun = 0;
    e_put_buffered( o, st.corr, st.ncorr );
    st.ncorr = 0;
  }
  return 1;
}

static uint enc_ac_first( BitOut& o, e_tbl& ac, const short* b, AcState& st,
                          uint Ss, uint Se, uint Al ) {
  int  k,r,t,v;
  uint n;
  r = 0;
  for( k=int(Ss); k<=int(Se); k++ ) {
    t = int(b[k]);
    if( t==0 ) { r++; continue; }
    // The point transform is a division rounding towards zero, so the shift
    // goes on the magnitude.  A coefficient can vanish at this precision.
    v = (t<0) ? -((-t) >> Al) : (t >> Al);
    if( v==0 ) { r++; continue; }
    if( !enc_eobrun(o,ac,st) ) return 0;
    while( r>15 ) { if( !e_emit(o,ac,0xF0) ) return 0; r -= 16; }
    n = e_cat( v );
    if( !e_emit(o,ac,uint(r<<4)|n) ) return 0;
    o.put_bits( e_bits_of(v,n), n );
    r = 0;
  }
  if( r>0 ) {
    st.eobrun++;
    if( st.eobrun == E_MAX_EOBRUN ) { if( !enc_eobrun(o,ac,st) ) return 0; }
  }
  return 1;
}

static uint enc_ac_refine( BitOut& o, e_tbl& ac, const short* b, AcState& st,
                           uint Ss, uint Se, uint Al ) {
  int  k,r,t,eob;
  uint i,nloc;
  int  av[64];
  byte loc[64];          // the correction bits THIS block contributes

  // The magnitudes at this scan's precision, and where the last newly-nonzero
  // one is: past that point nothing but corrections and zeroes remain, which
  // is what lets a run of sixteen zeroes be folded into an EOB instead of
  // being spelled out.
  eob = 0;
  for( k=int(Ss); k<=int(Se); k++ ) {
    t = int(b[k]); if( t<0 ) t = -t;
    av[k] = t >> Al;
    if( av[k]==1 ) eob = k;
  }

  r = 0; nloc = 0;
  for( k=int(Ss); k<=int(Se); k++ ) {
    if( av[k]==0 ) { r++; continue; }

    while( (r>15) && (k<=eob) ) {
      // The pending run goes first, and takes the correction bits of the
      // blocks it covers with it -- this block's are not among them yet.
      if( !enc_eobrun(o,ac,st) ) return 0;
      if( !e_emit(o,ac,0xF0) ) return 0;
      r -= 16;
      e_put_buffered( o, loc, nloc );
      nloc = 0;
    }

    if( av[k]>1 ) {
      // Already nonzero before this scan: one correction bit, which rides
      // behind whatever is written next.
      if( nloc < uint(DIM(loc)) ) loc[nloc++] = byte(av[k] & 1);
      continue;
    }

    if( !enc_eobrun(o,ac,st) ) return 0;
    if( !e_emit(o,ac,uint(r<<4)|1) ) return 0;
    o.put_bits( (int(b[k])<0) ? 0u : 1u, 1 );      // sign of the new coefficient
    e_put_buffered( o, loc, nloc );
    nloc = 0;
    r = 0;
  }

  if( (r>0) || (nloc>0) ) {
    st.eobrun++;
    // Only now do this block's correction bits join the ones already waiting:
    // an EOB run stands for the blocks behind it, and it has just grown to
    // include this one.
    for( i=0; i<nloc; i++ ) if( st.ncorr < uint(DIM(st.corr)) ) st.corr[st.ncorr++] = loc[i];
    // libjpeg forces the run out before the counter or the bit buffer can
    // overflow, and where it does so is part of the byte stream.
    if( (st.eobrun == E_MAX_EOBRUN) || (st.ncorr > uint(E_MAX_CORR_BITS-64+1)) ) {
      if( !enc_eobrun(o,ac,st) ) return 0;
    }
  }
  return 1;
}
