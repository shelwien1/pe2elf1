
// jpgcoder -- take a JPEG apart into its coefficients and put it back together.
//
//   jpgcoder c in.jpg out.hdr out.coef
//   jpgcoder d in.hdr out.jpg in.coef
//
// The .coef file is the image as the encoder actually saw it: one 16-bit signed
// value per DCT coefficient, in zigzag order, band by band -- every coefficient
// 0 of every block, then every coefficient 1, and so on.  That is the shape
// worth having: neighbouring blocks' like coefficients sit next to each other,
// which is what any model of them wants, and nothing about the entropy coding
// is left in the way.  The .hdr file is everything else -- the marker segments,
// and what it takes to put the entropy coding back exactly.
//
// "Exactly" is the whole difficulty.  Decoding a scan and re-encoding it gives
// a stream that means the same thing but need not be the same bytes: where an
// EOB run is flushed, how a run of sixteen zeroes is split, which bit pads the
// last byte are all choices, and the file being reproduced made them once
// already.  So this does not assume: it re-encodes every scan at compress time
// and compares against the original bytes.  A scan that comes back identical is
// stored as coefficients; one that does not is left in the header verbatim.
// That way the round trip is exact for every input, and what varies is only how
// much of the file reached the coefficient form -- which the summary says.
//
// Built from pjpg: it walks the markers, decodes the entropy data, and hands
// over both the coefficients and the scan state the encoder needs.

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <stdarg.h>

#include "common.inc"

#include "coro3b.inc"
#include "cinfo.inc"
#include "pjpg0j.inc"
#include "pjpg1.inc"
#include "jpgenc.inc"

//=============================================================================

#define UJP_MAGIC   "UJP2"
#define UJP_END     "END!"
#define UJP_VERSION 1

enum { JC_MAX_SCANS = 256 };

static void put_u32( FILE* g, uint v )  { uint i; for(i=0;i<4;i++) putc((v>>(8*i))&0xFF,g); }
static void put_u64( FILE* g, qword v ) { uint i; for(i=0;i<8;i++) putc(uint(v>>(8*i))&0xFF,g); }

static uint f_bad;
static uint get_u32( FILE* f )  { uint i,v,c; v=0; for(i=0;i<4;i++){ c=uint(getc(f)); if(c==uint(EOF)){f_bad=1;return 0;} v|=(c&0xFF)<<(8*i);} return v; }
static qword get_u64( FILE* f ) { uint i,c; qword v=0; for(i=0;i<8;i++){ c=uint(getc(f)); if(c==uint(EOF)){f_bad=1;return 0;} v|=qword(c&0xFF)<<(8*i);} return v; }

// Not a cryptographic hash: it is here so "d" can prove it rebuilt the file it
// was given rather than a plausible-looking one.
struct Fnv {
  qword h;
  void init( void ) { h = 14695981039346656037ULL; }
  void add( const byte* p, qword n ) { qword i; for(i=0;i<n;i++){ h^=p[i]; h*=1099511628211ULL; } }
};

//=============================================================================
// One scan, as both halves of the job see it.
//=============================================================================

struct Scan {
  qword beg, end;        // its entropy data in the original file
  qword splice;          // where it goes back, as an offset into the header
  uint  coef;            // stored as coefficients rather than left in the header
  uint  padbit;          // the bit that padded its last byte
  uint  prog, Ss, Se, Ah, Al;   // what it was, for -v
  uint  arith;                  // arithmetic-coded: there is no encoder for it
  const char* why;              // why it did not re-encode to the same bytes
};

static Scan   scan[JC_MAX_SCANS];
static uint   n_scan;
static uint   n_coef_scan;

static byte*  filebuf;      // the whole input, on the compress side
static qword  filelen;

static byte*  hdrbuf;       // the header being built, or read back
static qword  hdrlen, hdrcap;

static byte*  encbuf;       // one re-encoded scan
static qword  enccap;

static uint   f_verbose;

//=============================================================================
// The parser, driven over a byte range that is already in memory.
//=============================================================================

struct Run {
  pjpg P;
  byte outb[16];

  uint go( const byte* p, qword n, void* ctx, void (*hook)( void*, pjpg*, uint ) ) {
    uint r;
    qword done;
    P.coro_init();
    P.level = 0;
    P.sub   = 0;
    P.f_entropy = 1;
    P.f_quiet   = 1;
    P.sc_ctx    = ctx;
    P.sc_hook   = hook;
    P.stream_base = 0;
    P.addout( outb, uint(sizeof(outb)) );
    done = 0;
    P.addinp( (byte*)p, 0 );
    while(1) {
      r = P.coro_call( &P );
      P.stream_base += qword( P.pin[0].ptr - P.pin[0].beg );
      if( r!=1 ) break;
      if( done>=n ) { P.f_quit = 1; P.addinp( (byte*)p, 0 ); continue; }
      // One block: the input is already in memory, so there is nothing to
      // stage it through.
      P.addinp( (byte*)p+done, uint(n-done) );
      done = n;
    }
    return (P.err_code==0);
  }
};

static Run R;

//=============================================================================
// Encoding one scan from the coefficients, with pjpg's state describing it.
//=============================================================================

static uint encode_scan( pjpg& P, uint padbit, qword* out_len ) {
  e_tbl   dc[4], ac[4];
  AcState st;
  BitOut  o;
  int     last_dc[4];
  uint    ci,cc,blkn,used_dc,used_ac,i;
  uint    row,col,rst_to_go,rst_num;
  short*  b;

  // per_scan_setup() is what fills in the MCU geometry, and pjpg runs it inside
  // decode_scan -- which on this path has not happened and never will.  It only
  // computes, so running it here is free and idempotent.
  if( !P.per_scan_setup() ) return 0;

  o.init( encbuf, enccap );
  st.reset();
  for( i=0; i<4; i++ ) last_dc[i] = 0;
  used_dc = used_ac = 0;

  // Build the encoder's codes from the very tables the decoder used, so there
  // is nothing to choose: T.81 C.2 fixes the assignment given the DHT.
  for( ci=0; ci<uint(P.comps_in_scan); ci++ ) {
    cc = P.cs_cmp[ci];
    if( cc>=4 ) return 0;
    uint dn = uint(P.comp_info[cc].dc_tbl_no) & 3;
    uint an = uint(P.comp_info[cc].ac_tbl_no) & 3;
    if( !P.progressive_mode || ((P.Ss==0) && (P.Ah==0)) ) {
      if( !(used_dc>>dn & 1) ) { if( !make_etbl( P.dc_huff_tbl_ptrs[dn], dc[dn] ) ) return 0; used_dc |= 1u<<dn; }
    }
    if( !P.progressive_mode || (P.Ss!=0) ) {
      if( !(used_ac>>an & 1) ) { if( !make_etbl( P.ac_huff_tbl_ptrs[an], ac[an] ) ) return 0; used_ac |= 1u<<an; }
    }
  }

  rst_to_go = P.restart_interval;
  rst_num   = 0;

  for( row=0; row<P.MCU_rows_in_scan; row++ ) {
    for( col=0; col<P.MCUs_per_row; col++ ) {

      if( P.restart_interval ) {
        if( rst_to_go==0 ) {
          // An EOB run may not span a restart interval.
          if( P.progressive_mode && (P.Ss!=0) ) { if( !enc_eobrun(o,ac[P.comp_info[P.cs_cmp[0]].ac_tbl_no & 3],st) ) return 0; }
          o.restart( rst_num, padbit );
          rst_num++;
          st.reset();
          for( i=0; i<4; i++ ) last_dc[i] = 0;
          rst_to_go = P.restart_interval;
        }
        rst_to_go--;
      }

      P.mcu_row = row; P.mcu_col = col;

      if( !P.progressive_mode ) {
        for( blkn=0; blkn<uint(P.blocks_in_MCU); blkn++ ) {
          ci = uint(P.MCU_membership[blkn]);
          cc = P.cs_cmp[ci];
          b  = P.co_block( blkn );
          if( b==0 ) return 0;
          if( !enc_block_seq( o, dc[P.comp_info[cc].dc_tbl_no & 3],
                                 ac[P.comp_info[cc].ac_tbl_no & 3], b, &last_dc[ci] ) ) return 0;
        }
      } else if( P.Ss==0 ) {
        for( blkn=0; blkn<uint(P.blocks_in_MCU); blkn++ ) {
          ci = uint(P.MCU_membership[blkn]);
          cc = P.cs_cmp[ci];
          b  = P.co_block( blkn );
          if( b==0 ) return 0;
          if( P.Ah==0 ) { if( !enc_dc_first( o, dc[P.comp_info[cc].dc_tbl_no & 3], b, &last_dc[ci], uint(P.Al) ) ) return 0; }
          else            enc_dc_refine( o, b, uint(P.Al) );
        }
      } else {
        // An AC scan is always single-component, so there is one block per MCU.
        cc = P.cs_cmp[0];
        b  = P.co_block( 0 );
        if( b==0 ) return 0;
        if( P.Ah==0 ) { if( !enc_ac_first ( o, ac[P.comp_info[cc].ac_tbl_no & 3], b, st, uint(P.Ss), uint(P.Se), uint(P.Al) ) ) return 0; }
        else          { if( !enc_ac_refine( o, ac[P.comp_info[cc].ac_tbl_no & 3], b, st, uint(P.Ss), uint(P.Se), uint(P.Al) ) ) return 0; }
      }
      if( o.bad ) return 0;
    }
  }

  if( P.progressive_mode && (P.Ss!=0) )
    if( !enc_eobrun( o, ac[P.comp_info[P.cs_cmp[0]].ac_tbl_no & 3], st ) ) return 0;
  o.flush( padbit );
  if( o.bad ) return 0;
  *out_len = o.len;
  return 1;
}

//=============================================================================
// c: take it apart
//=============================================================================

static void c_hook( void* ctx, pjpg* P, uint phase ) {
  (void)ctx;
  if( n_scan >= JC_MAX_SCANS ) return;
  if( phase==0 ) {
    // The scan header has been consumed, so the entropy data starts here.
    scan[n_scan].beg    = P->at();
    scan[n_scan].end    = 0;
    scan[n_scan].coef   = 0;
    scan[n_scan].padbit = 1;
    scan[n_scan].prog   = uint(P->progressive_mode);
    scan[n_scan].Ss = uint(P->Ss); scan[n_scan].Se = uint(P->Se);
    scan[n_scan].Ah = uint(P->Ah); scan[n_scan].Al = uint(P->Al);
    scan[n_scan].arith = uint(P->arith_code);
    scan[n_scan].why   = "";
  } else {
    // e_end is where the FF that ended the data sits.  A scan pjpg could not
    // decode leaves it unset; the marker walk below closes those.
    scan[n_scan].end = P->e_end;
    n_scan++;
  }
}

// Everything the encoder cannot know: which bit padded the last byte.  It is
// whatever the bits after the last coded one are, and they are all the same in
// a conforming file.
static uint padbit_of( const byte* p, qword beg, qword end, qword coded_bits ) {
  qword nb;
  uint  last,used,mask;
  (void)beg;
  if( end<=beg ) return 1;
  nb = coded_bits & 7;
  if( nb==0 ) return 1;                       // nothing was padded
  last = p[end-1];
  used = uint(nb);
  mask = (1u<<(8-used)) - 1;
  if( (last & mask)==mask ) return 1;
  if( (last & mask)==0    ) return 0;
  return 2;                                   // mixed: not reproducible
}

static int cmd_c( const char* inp, const char* hdrname, const char* coefname ) {
  FILE *f,*g;
  qword i,cut,elen,total_coef;
  uint  s,c,ok,ncomp;
  Fnv   H;

  f = fopen( inp, "rb" );
  if( f==0 ) { fprintf( stderr, "jpgcoder: cannot read %s\n", inp ); return 2; }
  fseeko( f, 0, SEEK_END ); filelen = qword(ftello(f)); rewind( f );
  if( filelen==0 ) { fprintf( stderr, "jpgcoder: %s is empty\n", inp ); fclose(f); return 2; }
  filebuf = (byte*)malloc( size_t(filelen) );
  if( filebuf==0 || fread( filebuf, 1, size_t(filelen), f )!=filelen ) {
    fprintf( stderr, "jpgcoder: cannot read %s\n", inp ); fclose(f); return 2;
  }
  fclose( f );

  n_scan = 0;
  R.P.co_want = 1;
  ok = R.go( filebuf, filelen, 0, c_hook );

  // A file the parser could not get through is still a file that has to come
  // back byte for byte, and so is one with no decodable image data in it at
  // all.  Both simply keep every scan in the header: the round trip is a copy,
  // and the summary says nothing reached the coefficient form.
  if( !ok || R.P.co_failed || (R.P.co_buf==0) ) {
    if( f_verbose )
      fprintf( stderr, "jpgcoder: %s -- %s, so nothing is stored as coefficients\n", inp,
               ok ? "no decodable image data" : (R.P.err_msg ? R.P.err_msg : "does not parse") );
    n_scan = 0;
  }

  // A scan whose end is unknown -- one pjpg could not decode -- stays in the
  // header whole, so it needs no end.
  for( s=0; s<n_scan; s++ ) if( (scan[s].end==0) || (scan[s].end<scan[s].beg) ) scan[s].end = scan[s].beg;

  // --- try each scan as coefficients ---------------------------------------
  enccap = filelen + (filelen>>2) + (1<<16);
  encbuf = (byte*)malloc( size_t(enccap) );
  if( encbuf==0 ) { fprintf( stderr, "jpgcoder: out of memory\n" ); return 2; }

  n_coef_scan = 0;
  if( n_scan ) {
    struct Try { uint idx; } t;
    t.idx = 0;
    // Walk the file again, this time to re-encode: the parse gives back the
    // same scan state in the same order, and the coefficients are already in
    // hand from the first pass.
    struct Ctx { uint i; } cx; cx.i = 0;
    R.P.co_want = 0;                       // keep the coefficients from pass one
    short* keep = R.P.co_buf; qword keepn = R.P.co_nblk;
    qword kb[4]; uint kw[4],kh[4];
    for( c=0; c<4; c++ ) { kb[c]=R.P.co_base[c]; kw[c]=R.P.co_bw[c]; kh[c]=R.P.co_bh[c]; }
    R.P.co_buf = 0;                        // the reset in the next parse must not free it
    (void)t;

    struct Enc {
      short* buf; qword nblk; qword base[4]; uint bw[4],bh[4];
      uint i;
    } E;
    E.buf = keep; E.nblk = keepn; E.i = 0;
    for( c=0; c<4; c++ ) { E.base[c]=kb[c]; E.bw[c]=kw[c]; E.bh[c]=kh[c]; }

    struct H2 {
      static void hook( void* ctx, pjpg* P, uint phase ) {
        Enc& e = *(Enc*)ctx;
        qword len;
        uint  pb;
        if( phase!=0 ) return;
        if( e.i >= n_scan ) { e.i++; return; }
        // Point the parser at the coefficients pass one produced.
        P->co_buf = e.buf; P->co_nblk = e.nblk;
        for( uint c=0; c<4; c++ ) { P->co_base[c]=e.base[c]; P->co_bw[c]=e.bw[c]; P->co_bh[c]=e.bh[c]; }
        Scan& S = scan[e.i];
        S.why = S.arith ? "arithmetic" : "the encoder gave up";
        for( pb=0; (pb<2) && !S.arith; pb++ ) {
          if( !encode_scan( *P, pb, &len ) ) continue;
          if( len != S.end-S.beg ) { S.why = "re-encoded to a different length"; continue; }
          if( memcmp( encbuf, filebuf+S.beg, size_t(len) )!=0 ) { S.why = "re-encoded to different bytes"; continue; }
          S.coef = 1; S.padbit = pb; S.why = ""; n_coef_scan++;
          break;
        }
        // Hand the buffer back before decode_scan runs: it is about to walk
        // this scan again and would write over what it is holding.
        P->co_buf = 0; P->co_nblk = 0;
        e.i++;
      }
    };
    R.P.co_want = 0;
    ok = R.go( filebuf, filelen, &E, H2::hook );
    (void)ok;
    // The second parse allocated nothing; put pass one's coefficients back.
    R.P.co_buf = keep; R.P.co_nblk = keepn;
    for( c=0; c<4; c++ ) { R.P.co_base[c]=kb[c]; R.P.co_bw[c]=kw[c]; R.P.co_bh[c]=kh[c]; }
  }

  // --- the header: the file, minus the scans that became coefficients -------
  hdrcap = filelen + 16; hdrlen = 0;
  hdrbuf = (byte*)malloc( size_t(hdrcap) );
  if( hdrbuf==0 ) { fprintf( stderr, "jpgcoder: out of memory\n" ); return 2; }
  cut = 0;
  for( s=0; s<n_scan; s++ ) {
    if( !scan[s].coef ) continue;
    memcpy( hdrbuf+hdrlen, filebuf+cut, size_t(scan[s].beg-cut) );
    hdrlen += scan[s].beg-cut;
    scan[s].splice = hdrlen;                 // where it goes back
    cut = scan[s].end;
  }
  memcpy( hdrbuf+hdrlen, filebuf+cut, size_t(filelen-cut) );
  hdrlen += filelen-cut;

  // --- write it out ---------------------------------------------------------
  H.init(); H.add( filebuf, filelen );

  g = fopen( hdrname, "wb" );
  if( g==0 ) { fprintf( stderr, "jpgcoder: cannot write %s\n", hdrname ); return 2; }
  fwrite( UJP_MAGIC, 1, 4, g );
  put_u32( g, UJP_VERSION );
  ncomp = (R.P.co_buf && (n_scan>0)) ? uint(R.P.num_components) : 0;
  put_u32( g, ncomp );
  for( c=0; c<ncomp; c++ ) { put_u32( g, R.P.co_bw[c] ); put_u32( g, R.P.co_bh[c] ); }
  put_u32( g, n_scan );
  for( s=0; s<n_scan; s++ ) {
    put_u32( g, scan[s].coef );
    put_u64( g, scan[s].coef ? scan[s].splice : 0 );
    put_u32( g, scan[s].padbit );
  }
  put_u64( g, hdrlen );
  fwrite( hdrbuf, 1, size_t(hdrlen), g );
  put_u64( g, filelen );
  put_u64( g, H.h );
  fwrite( UJP_END, 1, 4, g );
  if( fflush(g)!=0 || ferror(g) || fclose(g)!=0 ) {
    fprintf( stderr, "jpgcoder: write error on %s\n", hdrname ); return 2;
  }

  // The coefficients, band by band: every block's coefficient 0, then every
  // block's coefficient 1, and so on, per component.
  g = fopen( coefname, "wb" );
  if( g==0 ) { fprintf( stderr, "jpgcoder: cannot write %s\n", coefname ); return 2; }
  total_coef = 0;
  for( c=0; c<ncomp; c++ ) {
    qword nb = qword(R.P.co_bw[c]) * R.P.co_bh[c];
    for( uint k=0; k<64; k++ ) {
      for( i=0; i<nb; i++ ) {
        short v = R.P.co_buf[ (R.P.co_base[c]+i)*64 + k ];
        putc( uint(v)&0xFF, g ); putc( (uint(v)>>8)&0xFF, g );
      }
    }
    total_coef += nb*64;
  }
  if( fflush(g)!=0 || ferror(g) || fclose(g)!=0 ) {
    fprintf( stderr, "jpgcoder: write error on %s\n", coefname ); return 2;
  }

  {
    uint n_ar = 0;
    for( s=0; s<n_scan; s++ ) if( scan[s].arith ) n_ar++;
    printf( "%u scan(s), %u as coefficients", n_scan, n_coef_scan );
    // Arithmetic coding has a decoder here but no encoder, so those scans can
    // never be re-encoded and are not a shortfall to go looking for.
    if( n_ar ) printf( " (%u arithmetic, which this cannot re-encode)", n_ar );
    printf( "; %llu coefficients, header %llu of %llu bytes\n",
            (unsigned long long)total_coef,
            (unsigned long long)hdrlen, (unsigned long long)filelen );
  }
  if( f_verbose )
    for( s=0; s<n_scan; s++ )
      printf( "  scan %-3u %10llu..%-10llu  %-8s Ss=%-2u Se=%-2u Ah=%-2u Al=%-2u  %s\n", s,
              (unsigned long long)scan[s].beg, (unsigned long long)scan[s].end,
              scan[s].arith ? (scan[s].prog ? "arith-p" : "arith") : (scan[s].prog ? "prog" : "seq"),
              scan[s].Ss, scan[s].Se, scan[s].Ah, scan[s].Al,
              scan[s].coef ? "coefficients" : scan[s].why );
  return 0;
}

//=============================================================================
// d: put it back
//=============================================================================

struct DCtx {
  uint   i;              // which scan the walk is up to
  FILE*  fcoef;
  uint   ncomp;
  uint   bw[4], bh[4];
  qword  base[4];
  short* buf;
  qword  nblk;
  uint   loaded;
  uint   failed;
  qword* out_len;        // per-scan encoded length, filled in as we go
  byte** out_buf;
};

static uint load_coef( DCtx& D ) {
  uint c,k;
  qword i,nb;
  int lo,hi;
  D.nblk = 0;
  for( c=0; c<D.ncomp; c++ ) { D.base[c] = D.nblk; D.nblk += qword(D.bw[c])*D.bh[c]; }
  if( (D.nblk==0) || (D.nblk > (qword(1)<<28)) ) return 0;
  D.buf = (short*)calloc( size_t(D.nblk)*64, sizeof(short) );
  if( D.buf==0 ) return 0;
  for( c=0; c<D.ncomp; c++ ) {
    nb = qword(D.bw[c])*D.bh[c];
    for( k=0; k<64; k++ ) {
      for( i=0; i<nb; i++ ) {
        lo = getc( D.fcoef ); hi = getc( D.fcoef );
        if( (lo==EOF) || (hi==EOF) ) return 0;
        D.buf[ (D.base[c]+i)*64 + k ] = short( uint(lo) | (uint(hi)<<8) );
      }
    }
  }
  D.loaded = 1;
  return 1;
}

static void d_hook( void* ctx, pjpg* P, uint phase ) {
  DCtx& D = *(DCtx*)ctx;
  qword len;
  if( phase!=0 ) return;
  if( D.i >= n_scan ) { D.i++; return; }
  if( !scan[D.i].coef ) { D.i++; return; }

  // The first scan that needs them is the first moment the geometry is known,
  // which is exactly when pjpg has read the frame header.
  if( !D.loaded ) {
    if( !load_coef( D ) ) { D.failed = 1; D.i++; return; }
  }
  P->co_buf = D.buf; P->co_nblk = D.nblk;
  for( uint c=0; c<4; c++ ) { P->co_base[c]=D.base[c]; P->co_bw[c]=D.bw[c]; P->co_bh[c]=D.bh[c]; }

  uint ok = encode_scan( *P, scan[D.i].padbit, &len );
  // decode_scan is next, and on this side the scan's data is not in the header
  // at all -- it would decode the padding it finds instead and write that over
  // the coefficients just loaded.
  P->co_buf = 0; P->co_nblk = 0;
  if( !ok ) { D.failed = 1; D.i++; return; }
  D.out_buf[D.i] = (byte*)malloc( size_t(len)?size_t(len):1 );
  if( D.out_buf[D.i]==0 ) { D.failed = 1; D.i++; return; }
  memcpy( D.out_buf[D.i], encbuf, size_t(len) );
  D.out_len[D.i] = len;
  D.i++;
}

static int cmd_d( const char* hdrname, const char* outname, const char* coefname ) {
  FILE *f,*g;
  char  magic[8];
  qword orig_size, orig_hash, cut;
  uint  s,c,version;
  DCtx  D;
  Fnv   H;
  qword out_len[JC_MAX_SCANS];
  byte* out_buf[JC_MAX_SCANS];

  bzero( D );
  for( s=0; s<JC_MAX_SCANS; s++ ) { out_len[s]=0; out_buf[s]=0; }

  f = fopen( hdrname, "rb" );
  if( f==0 ) { fprintf( stderr, "jpgcoder: cannot read %s\n", hdrname ); return 2; }
  if( fread( magic, 1, 4, f )!=4 || memcmp( magic, UJP_MAGIC, 4 )!=0 ) {
    fprintf( stderr, "jpgcoder: %s is not a jpgcoder header file\n", hdrname ); fclose(f); return 2;
  }
  f_bad = 0;
  version = get_u32( f );
  if( f_bad || version!=UJP_VERSION ) {
    fprintf( stderr, "jpgcoder: header version %u, this build writes %u\n", version, UJP_VERSION );
    fclose(f); return 2;
  }
  // Zero components means the file was copied whole into the header: there are
  // no coefficients to put back, only bytes to write out.
  D.ncomp = get_u32( f );
  if( f_bad || D.ncomp>4 ) { fprintf( stderr, "jpgcoder: bad component count\n" ); fclose(f); return 2; }
  for( c=0; c<D.ncomp; c++ ) { D.bw[c] = get_u32( f ); D.bh[c] = get_u32( f ); }
  n_scan = get_u32( f );
  if( f_bad || n_scan>JC_MAX_SCANS ) { fprintf( stderr, "jpgcoder: bad scan count\n" ); fclose(f); return 2; }
  for( s=0; s<n_scan; s++ ) {
    scan[s].coef   = get_u32( f );
    scan[s].splice = get_u64( f );
    scan[s].padbit = get_u32( f );
  }
  hdrlen = get_u64( f );
  if( f_bad || hdrlen > (qword(1)<<32) ) { fprintf( stderr, "jpgcoder: bad header length\n" ); fclose(f); return 2; }
  hdrbuf = (byte*)malloc( size_t(hdrlen)?size_t(hdrlen):1 );
  if( hdrbuf==0 || fread( hdrbuf, 1, size_t(hdrlen), f )!=hdrlen ) {
    fprintf( stderr, "jpgcoder: %s is truncated\n", hdrname ); fclose(f); return 2;
  }
  orig_size = get_u64( f );
  orig_hash = get_u64( f );
  if( f_bad || fread( magic, 1, 4, f )!=4 || memcmp( magic, UJP_END, 4 )!=0 ) {
    fprintf( stderr, "jpgcoder: %s is truncated or corrupt\n", hdrname ); fclose(f); return 2;
  }
  fclose( f );

  D.fcoef = fopen( coefname, "rb" );
  if( D.fcoef==0 ) { fprintf( stderr, "jpgcoder: cannot read %s\n", coefname ); return 2; }
  D.out_len = out_len; D.out_buf = out_buf;

  // Walking the header alone gives the parser everything the encoder needs --
  // the frame, the tables, each scan's spectral range and components -- because
  // all of it lives in the segments, and only the entropy data was taken out.
  enccap = orig_size + (orig_size>>2) + (1<<16);
  encbuf = (byte*)malloc( size_t(enccap) );
  if( encbuf==0 ) { fprintf( stderr, "jpgcoder: out of memory\n" ); return 2; }
  R.P.co_want = 0;
  R.go( hdrbuf, hdrlen, &D, d_hook );
  fclose( D.fcoef );
  if( D.failed ) { fprintf( stderr, "jpgcoder: could not re-encode the image data\n" ); return 2; }

  g = fopen( outname, "wb" );
  if( g==0 ) { fprintf( stderr, "jpgcoder: cannot write %s\n", outname ); return 2; }
  H.init();
  cut = 0;
  for( s=0; s<n_scan; s++ ) {
    if( !scan[s].coef ) continue;
    if( scan[s].splice < cut || scan[s].splice > hdrlen ) {
      fprintf( stderr, "jpgcoder: the header says scan %u goes back at %llu, which is out of order\n",
               s, (unsigned long long)scan[s].splice );
      fclose(g); remove(outname); return 2;
    }
    fwrite( hdrbuf+cut, 1, size_t(scan[s].splice-cut), g );
    H.add( hdrbuf+cut, scan[s].splice-cut );
    fwrite( out_buf[s], 1, size_t(out_len[s]), g );
    H.add( out_buf[s], out_len[s] );
    cut = scan[s].splice;
  }
  fwrite( hdrbuf+cut, 1, size_t(hdrlen-cut), g );
  H.add( hdrbuf+cut, hdrlen-cut );

  if( fflush(g)!=0 || ferror(g) || fclose(g)!=0 ) {
    fprintf( stderr, "jpgcoder: write error on %s\n", outname ); remove(outname); return 2;
  }

  {
    qword n = hdrlen;
    for( s=0; s<n_scan; s++ ) if( scan[s].coef ) n += out_len[s];
    if( n!=orig_size ) {
      fprintf( stderr, "jpgcoder: rebuilt %llu bytes, the original was %llu\n",
               (unsigned long long)n, (unsigned long long)orig_size );
      return 1;
    }
  }
  if( H.h!=orig_hash ) {
    fprintf( stderr, "jpgcoder: the rebuilt file does not match the original's hash\n" );
    return 1;
  }
  printf( "%u scan(s) rebuilt, %llu bytes verified\n", n_scan, (unsigned long long)orig_size );
  return 0;
}

//=============================================================================

static void usage( const char* me ) {
  fprintf( stderr,
    "usage: %s [-v] c <in.jpg> <out.hdr> <out.coef>   take a JPEG apart\n"
    "       %s [-v] d <in.hdr> <out.jpg> <in.coef>    put it back together\n"
    "\n"
    "The .coef file holds the DCT coefficients as 16-bit values in zigzag\n"
    "order, band by band per component; the .hdr file holds the marker\n"
    "segments and what it takes to re-encode the entropy data exactly.\n"
    "A scan that does not re-encode to the same bytes is left in the header\n"
    "verbatim, so the round trip is exact whatever the input.\n", me, me );
}

int main( int argc, char** argv ) {
  int argi = 1;
  if( (argc>1) && (argv[1][0]=='-') && (argv[1][1]=='v') && (argv[1][2]==0) ) { f_verbose = 1; argi = 2; }
  if( argi+4 > argc ) { usage( argv[0] ); return 2; }
  if( (argv[argi][0]=='c') && (argv[argi][1]==0) )
    return cmd_c( argv[argi+1], argv[argi+2], argv[argi+3] );
  if( (argv[argi][0]=='d') && (argv[argi][1]==0) )
    return cmd_d( argv[argi+1], argv[argi+2], argv[argi+3] );
  usage( argv[0] );
  return 2;
}
