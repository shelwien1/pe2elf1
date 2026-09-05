
// jpegdet -- find the JPEG images in an arbitrary byte stream, extract each one
// as a file a decoder will accept, and keep enough beside them to put the
// original stream back together byte for byte.
//
//   jpegdet c <input> <prefix>     ->  <prefix>00000000.jpg, ... and <prefix>.jdm
//   jpegdet d <prefix> <output>    ->  the original <input>, exactly
//
// The detector is pjpg: every candidate offset is handed to the same parser
// that pjpg.cpp drives over a whole file, and a candidate is a JPEG only if
// that parser walks it from SOI to EOI without a fatal error.  That is a much
// stronger test than a signature scan -- it validates every segment length,
// every table definition and (unless -s is given) every entropy-coded scan --
// and it is also what tells us where the image *ends*, which a signature scan
// cannot: the end of the last scan's entropy data is only knowable by decoding
// it, and EOI is a two-byte pattern that occurs inside entropy data all the
// time.
//
// The structure follows rawdet.cpp: scan for a candidate, run the decoder on
// it, and on a match write the matched bytes to their own file while the bytes
// in between accumulate as literal data.  Where rawdet writes its literals to
// one container file and its gap lengths to another, jpegdet keeps both in a
// single metainfo file, so restoration needs only the prefix.

// Streams big enough to be worth carving are routinely bigger than 2 GB, and
// this has to be set before the first system header, not after it.
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <stdarg.h>   // for pjpg0::pf(), the indenting printf

#include "common.inc"

// fseek/ftell take and return long, which is 32 bits on Windows and on 32-bit
// Linux.  Everything here addresses the input by absolute offset, so a 2 GB
// wrap would not fail loudly -- it would carve the wrong bytes.
#if defined(_MSC_VER)
  #define jd_seek(f,o)  _fseeki64( (f), (__int64)(o), SEEK_SET )
  #define jd_seekend(f) _fseeki64( (f), 0, SEEK_END )
  #define jd_tell(f)    _ftelli64( (f) )
  typedef __int64 jd_off;
#else
  #define jd_seek(f,o)  fseeko( (f), (off_t)(o), SEEK_SET )
  #define jd_seekend(f) fseeko( (f), 0, SEEK_END )
  #define jd_tell(f)    ftello( (f) )
  typedef off_t jd_off;
#endif

#include "coro3b.inc"

#include "cinfo.inc"

#include "pjpg0j.inc"
#include "pjpg1.inc"

//=============================================================================
// Little-endian scalar I/O.
//
// Written a byte at a time on purpose: the metainfo file is read back by a
// build that may not share this one's word size, alignment rules or byte
// order, and a struct written with fwrite would agree with none of them.
//=============================================================================

static void put_u32( FILE* g, uint v ) {
  uint i; for( i=0; i<4; i++ ) putc( (v>>(8*i)) & 0xFF, g );
}

static void put_u64( FILE* g, qword v ) {
  uint i; for( i=0; i<8; i++ ) putc( uint(v>>(8*i)) & 0xFF, g );
}

// Every getter reports failure through f_bad rather than a return value, so a
// truncated metainfo file is caught once at the end of a record instead of at
// every field.
static uint f_bad;

static uint get_u32( FILE* f ) {
  uint i,v,c; v=0;
  for( i=0; i<4; i++ ) { c=uint(getc(f)); if( c==uint(EOF) ) { f_bad=1; return 0; } v |= (c&0xFF)<<(8*i); }
  return v;
}

static qword get_u64( FILE* f ) {
  uint i,c; qword v; v=0;
  for( i=0; i<8; i++ ) { c=uint(getc(f)); if( c==uint(EOF) ) { f_bad=1; return 0; } v |= qword(c&0xFF)<<(8*i); }
  return v;
}

//=============================================================================
// FNV-1a, 64-bit.
//
// Not a cryptographic hash and not meant to be one: it exists so that "d" can
// prove it rebuilt the stream it was given rather than a plausible-looking one,
// and so that an edited .jpg between "c" and "d" is reported instead of
// silently baked into the output.
//=============================================================================

struct Fnv {
  qword h;
  void init( void ) { h = 14695981039346656037ULL; }
  void add( const byte* p, uint n ) {
    uint i;
    for( i=0; i<n; i++ ) { h ^= p[i]; h *= 1099511628211ULL; }
  }
};

//=============================================================================
// The metainfo file.
//
//   "JPEGDET1"  u32 version
//   then a sequence of records, each introduced by one tag byte:
//     'I'  u64 gap_len, gap_len bytes, u64 img_len, u32 add_len, u32 flags, u32 index
//     'E'  u64 tail_len, tail_len bytes, u64 orig_size, u64 orig_hash, "JDMEND"
//
// Record-tagged rather than counted so that "c" can write it in one forward
// pass: the number of images is not known until the input is exhausted, and a
// header field patched by seeking back is a field that is wrong whenever the
// program is killed.  The size and hash live in the terminator for the same
// reason.
//
// The image bytes themselves are not here -- they are the .jpg files, which is
// the whole point.  gap_len is the run of stream data before the image, img_len
// is how much of the .jpg file belongs to the stream (see JDF_ADDED_EOI), and
// index names the file rather than leaving it implicit in record order, so a
// deleted or renamed .jpg is an error message instead of a silently
// misassembled output.
//=============================================================================

#define JDM_MAGIC   "JPEGDET1"
#define JDM_END     "JDMEND"
#define JDM_VERSION 1

enum {
  JDR_IMAGE = 'I',
  JDR_END   = 'E',
};

enum {
  JDF_ADDED_EOI = 1,   // the .jpg carries an EOI that the stream did not
  JDF_BAD_SCAN  = 2,   // accepted under -r with entropy data that did not decode
  JDF_MULTI_SOI = 4,   // cut short by the next image's SOI, not by an EOI
};

//=============================================================================
// One speculative parse.
//
// pjpg is a coroutine: it asks for input by yielding 1 and is otherwise driven
// exactly the way CoroFileProc drives it over a whole file.  The one thing
// added here is the absolute position, which the frontend can compute at any
// yield because addinp() sets beg=ptr=buf, so ptr-beg is what the parser has
// consumed out of the block it was last given.
//=============================================================================

enum { JD_INPBUF = 1<<16 };

struct JpegProbe {

  pjpg P;

  ALIGN(4096) byte inpbuf[JD_INPBUF];
  byte outbuf[16];              // pjpg never writes, but the pin must be valid

  uint  f_entropy;              // decode the scans (slow, certain) or skip them
  uint  f_relaxed;              // accept an image whose entropy data is damaged
  uint  f_tail;                 // accept a final image the stream cut short
  qword sof_limit;              // give up on a candidate that has read this far
                                // without producing a frame header

  // Filled in by probe():
  qword end_pos;                // absolute offset one past the last byte used
  uint  f_eoi;                  // the parse ended on a real EOI
  uint  f_scan_bad;             // some scan did not decode cleanly
  uint  f_multi_soi;            // two images ran together inside this one
  const char* why;              // why the candidate was turned down, for -v

  void init( void ) {
    f_entropy = 1; f_relaxed = 0; f_tail = 0;
    sof_limit = 16u<<20;
    why = "";
  }

  uint no( const char* reason ) { why = reason; return 0; }

  // Every component of a DCT frame has to name a quantisation table that some
  // DQT actually defined.  pjpg does not need the values -- it never
  // dequantises -- so nothing in the parse trips over a missing one, but
  // libjpeg stops dead ("Quantization table 0x00 was not defined"), which makes
  // this the difference between carving a file that opens and one that does
  // not.  Lossless and hierarchical frames carry no quantisation at all.
  uint quant_tables_present( void ) {
    uint i,q;
    if( P.sof_lossless || P.sof_hier ) return 1;
    if( P.num_components<1 ) return 0;
    for( i=0; i<uint(P.num_components); i++ ) {
      q = uint(P.comp_info[i].quant_tbl_no);
      if( q>=uint(DIM(P.quant_tbl)) ) return 0;   // pa_SOF rejects this already
      if( P.quant_tbl[q].f_undef ) return 0;
    }
    return 1;
  }

  // Parse from absolute offset start.  Returns 1 if what is there is a JPEG.
  uint probe( FILE* f, qword start, qword file_size ) {
    uint r,l;
    qword pos_base;

    P.coro_init();
    P.level         = 0;
    P.sub           = 0;         // no thumbnail recursion: a thumbnail travels
                                 // inside its parent's bytes, so parsing it
                                 // separately would only cost time
    P.f_entropy      = f_entropy;
    P.f_quiet        = 1;        // a detector is not a reporter
    P.f_stop_at_eoi  = 1;        // stop with the pointer exactly past the D9
    P.f_bail_on_err  = 1;        // a wrong guess must be cheap
    P.f_stop_at_soi2 = 1;        // cut an EOI-less image where the next begins
    P.f_check_scans  = !f_entropy;  // -s still checks what costs nothing
    P.addout( outbuf, uint(sizeof(outbuf)) );

    end_pos = start; f_eoi = 0; f_scan_bad = 0; f_multi_soi = 0; why = "";

    if( jd_seek( f, start )!=0 ) return no( "cannot seek to the candidate" );
    pos_base = start;
    P.addinp( inpbuf, 0 );

    while(1) {
      r = P.coro_call( &P );
      // Everything handed over has been consumed by the time the parser asks
      // for more, but ptr-beg is the honest figure either way.
      pos_base += qword( P.pin[0].ptr - P.pin[0].beg );
      if( r!=1 ) break;

      // A candidate that is not a JPEG can still parse for a very long way
      // without a fatal error: any byte in C0..FE is a marker with a length,
      // so random data reads a random length and skips it, over and over.  The
      // one thing it will not do is produce a frame header, and a real image
      // has one within its first few segments -- APPn payloads are capped at
      // 64 KB each, so even a file carrying a chunked ICC profile and an MPF
      // index gets there in well under this.  Without the cap, a stream with
      // many false signatures costs a full read of the tail for each one.
      if( (P.saw_SOF==0) && (pos_base-start > sof_limit) ) return no( "no frame header within the probe limit" );

      l = uint( fread( inpbuf, 1, JD_INPBUF, f ) );
      if( l==0 ) P.f_quit = 1;   // get() and friends do not survive a second
      P.addinp( inpbuf, l );     // failed read, so tell them once
    }

    end_pos     = pos_base;
    f_eoi       = P.eoi_reached;
    f_scan_bad  = (P.n_scan_bad!=0);
    f_multi_soi = (P.n_soi>1) && (P.eoi_reached==0);

    // The parse stopped on the SOI of the next image, having consumed its two
    // bytes.  This one ends just before them.
    if( f_multi_soi ) {
      if( end_pos < start+2 ) return no( "second SOI overlaps the first" );
      end_pos -= 2;
    }

    // --- the acceptance predicate ------------------------------------------
    // A real image starts with SOI, declares a frame, has at least one scan,
    // ends on EOI, and holds nothing a decoder would refuse.
    //
    // err_code comes first because it explains the rest: when a parser gave up
    // on a malformed field, "bad Huffman table" is why the candidate is not a
    // JPEG, and "no frame header" is only what that error prevented.
    why = "";
    if( P.fatal_for_decoding() ) return no( jpeg_err_name(P.err_code) );
    if( !P.saw_SOI )     return no( "no SOI" );
    if( !P.saw_SOF )     return no( "no frame header" );
    if( P.n_scan<1 )     return no( "no scan" );
    if( end_pos<=start ) return no( "empty" );

    // Anything a conforming decoder refuses outright.  pjpg walks past a
    // reserved marker or a sub-minimal segment length and says so; a file
    // carved with one in it would not open, so it is not carved.
    if( P.n_nonconf )    return no( "a segment a conforming decoder refuses" );
    if( !quant_tables_present() ) return no( "a component's quantisation table was never defined" );

    // Y==0 in a frame header means the height arrives later, in a DNL segment
    // (T.81 B.2.2), and pa_DNL writes it into image_height when it does.  Still
    // zero at the end of the image means no DNL ever came, so the frame never
    // had a height at all -- libjpeg calls that "Empty JPEG image".
    if( P.image_height==0 ) return no( "frame height is 0 and no DNL supplied one" );

    // Then the entropy data, when we decoded it.  n_scan_bad counts scans that
    // were decodable in principle and did not decode; scans pjpg cannot decode
    // at all -- lossless, hierarchical, 12-bit -- are never counted, so a valid
    // SOF3 image is still accepted here.
    if( P.eoi_reached ) {
      if( f_scan_bad && !f_relaxed ) return no( "entropy data did not decode" );
      return 1;
    }

    // No EOI.  The image ends somewhere the parser cannot see by itself, so it
    // is only carved where the end is knowable some other way:
    //
    //  * the next image's SOI, which stopped the parse above.  A real boundary,
    //    so this is done by default.
    //  * the end of the stream, under -t.  "The parser ran out of input" and
    //    "the image ends here" are the same statement only for the last thing
    //    in the file; anywhere else it would swallow everything that follows.
    //
    // Either way the last scan was cut off mid-data, so its failure to decode
    // is the truncation itself rather than evidence of a bad guess -- which is
    // why f_scan_bad is not consulted here.  At least one scan must still have
    // decoded, so that something was verified rather than merely parsed.
    if( !f_multi_soi ) {
      if( !f_tail )            return no( "no EOI (use -t to carve a truncated last image)" );
      if( end_pos!=file_size ) return no( "no EOI, and not the last thing in the stream" );
    }
    if( f_entropy && (P.n_scan_dec<1) ) return no( "no EOI and no scan decoded" );

    return 1;
  }
};

//=============================================================================
// The candidate scanner.
//
// SOI immediately followed by a marker: FF D8 FF.  A conforming stream cannot
// put anything else after SOI (T.81 B.1.1.3 allows only FF fill bytes, which
// this still matches), and anchoring on the third byte is what keeps the number
// of speculative parses down -- FF D8 alone turns up about 64 times as often in
// high-entropy data, and each one costs a parse.
//=============================================================================

enum { JD_SCANBUF = 1<<16, JD_SIGLEN = 3 };

struct SigScan {
  uint  siglen;    // 3 = FF D8 FF, 2 = FF D8
  FILE* f;
  byte  buf[JD_SCANBUF];
  qword base;      // absolute offset of buf[0]
  uint  fill;      // valid bytes in buf

  void init( FILE* _f, uint _siglen ) { f=_f; siglen=_siglen; base=0; fill=0; }

  // First offset >= from that starts FF D8 FF, or all-ones if there is none.
  //
  // The window is kept across calls, because the caller comes back here after
  // every rejected candidate -- at from = s+2, two bytes into the buffer it
  // just searched.  Re-reading from disk each time would make a stream full of
  // false signatures cost O(n) reads per candidate instead of O(n) in total.
  qword find( qword from, qword file_size ) {
    uint i,l,off;

    while(1) {

      if( from+siglen>file_size ) return ~qword(0);

      // Already covered by what is in hand?
      if( (fill>=siglen) && (from>=base) && (from+siglen<=base+fill) ) {
        off = uint(from-base);
        for( i=off; i+siglen<=fill; i++ ) {
          if( buf[i]!=0xFF ) continue;
          if( buf[i+1]!=0xD8 ) continue;
          if( (siglen>2) && (buf[i+2]!=0xFF) ) continue;
          return base+i;
        }
        // Not in this window.  A signature may still straddle its end, so the
        // next one can only start in the last siglen-1 bytes.
        from = base+fill-(siglen-1);
        if( from+siglen>file_size ) return ~qword(0);
      }

      // Refill from `from`.  Every refill advances base by at least
      // JD_SCANBUF-(JD_SIGLEN-1), so the whole scan reads the input once.
      if( jd_seek( f, from )!=0 ) { fill=0; return ~qword(0); }
      base = from;
      l = uint( fread( buf, 1, JD_SCANBUF, f ) );
      fill = l;
      if( l<siglen ) return ~qword(0);
    }
  }
};

//=============================================================================

static uint f_verbose = 0;

// fclose is the last chance a buffered write has to fail, and on a full disk it
// is usually the only one that does.  Everything written here is either a file
// the caller will rely on being complete or a metainfo file that claims the
// rest are -- so no output file is closed anywhere except through this.
static uint close_ok( FILE* f ) {
  uint bad;
  bad = (fflush(f)!=0) || ferror(f);
  if( fclose(f)!=0 ) bad = 1;
  return !bad;
}

// All-ones if the length cannot be learned.  Distinguishing that from a length
// of zero matters: an empty file is a perfectly good input that carves to an
// empty metainfo, while a pipe or a character device is one this cannot carve
// at all -- every candidate is revisited by seeking -- and reporting success
// after writing nothing would be a silent loss.
static qword file_size_of( FILE* f ) {
  jd_off n;
  if( jd_seekend(f)!=0 ) return ~qword(0);
  n = jd_tell( f );
  if( n<0 ) return ~qword(0);
  return qword(n);
}

// Copy [from,to) out of the input.  Every byte of the stream goes through here
// exactly once, on its way either to the metainfo file or to a .jpg, so this is
// also where the hash of the original is taken.
//
// A short return says only that the copy did not finish.  f_wrerr says which
// end gave out, because the two mean opposite things: a short read is an input
// that changed under us, a short write is an output that will be wrong.
static byte copybuf[1<<16];
static uint f_wrerr;

static qword copy_range( FILE* f, qword from, qword to, FILE* g, Fnv* h ) {
  qword done,n;
  uint want,got;
  done = 0; f_wrerr = 0;
  if( to<=from ) return 0;
  if( jd_seek( f, from )!=0 ) return 0;
  n = to-from;
  while( done<n ) {
    want = uint( (n-done < qword(sizeof(copybuf))) ? (n-done) : qword(sizeof(copybuf)) );
    got  = uint( fread( copybuf, 1, want, f ) );
    if( got==0 ) break;
    if( h ) h->add( copybuf, got );
    if( g ) if( fwrite( copybuf, 1, got, g )!=got ) { f_wrerr = 1; return done; }
    done += got;
  }
  return done;
}

//=============================================================================
// c: detect and extract
//=============================================================================

static JpegProbe PR;
static SigScan   SC;

struct Opts {
  uint  f_entropy;   // -s clears: skip the entropy-coded scans
  uint  f_relaxed;   // -r sets:   accept damaged entropy data
  uint  f_tail;      // -t sets:   accept a final image the stream cut short
  qword min_size;    // -m N
  qword sof_limit;   // -L N
  uint  siglen;      // 3 = anchor on FF D8 FF, 2 = FF D8
};

static int cmd_c( const char* inpname, const char* prefix, const Opts& O ) {
  char name[1024];
  FILE *f,*g,*gj;
  qword file_size, pos, committed, s, e, gap, n_gap, n_img;
  uint idx, flags, add_len, ok;
  Fnv H;

  f = fopen( inpname, "rb" );
  if( f==0 ) { fprintf( stderr, "jpegdet: cannot read %s\n", inpname ); return 2; }
  file_size = file_size_of( f );
  if( file_size==~qword(0) ) {
    fprintf( stderr, "jpegdet: %s is not seekable, so its images cannot be located\n", inpname );
    fclose(f); return 2;
  }

  if( snprintf( name, sizeof(name), "%s.jdm", prefix )>=int(sizeof(name)) ) {
    fprintf( stderr, "jpegdet: prefix too long\n" ); fclose(f); return 2;
  }
  g = fopen( name, "wb" );
  if( g==0 ) { fprintf( stderr, "jpegdet: cannot write %s\n", name ); fclose(f); return 2; }

  fwrite( JDM_MAGIC, 1, 8, g );
  put_u32( g, JDM_VERSION );

  PR.init();
  PR.f_entropy = O.f_entropy;
  PR.f_relaxed = O.f_relaxed;
  PR.f_tail    = O.f_tail;
  if( O.sof_limit ) PR.sof_limit = O.sof_limit;
  SC.init( f, O.siglen );
  H.init();

  pos = 0; committed = 0; idx = 0; n_gap = 0; n_img = 0;

  while(1) {
    s = SC.find( pos, file_size );
    if( s==~qword(0) ) break;

    ok = PR.probe( f, s, file_size );
    e  = PR.end_pos;

    // Too small to be worth carving, the same guard rawdet applies with
    // blk_minsize: a handful of bytes that happen to parse is noise, and every
    // one of them would cost a file.
    if( ok && (e-s < O.min_size) ) { ok = 0; PR.why = "shorter than the minimum size"; }

    if( !ok ) {
      if( f_verbose )
        printf( "          %12llu    rejected: %s\n",
                (unsigned long long)s, PR.why[0] ? PR.why : "not a JPEG" );
      pos = s+2;
      continue;
    }

    // --- a match ----------------------------------------------------------
    flags = 0; add_len = 0;
    if( !PR.f_eoi ) { flags |= JDF_ADDED_EOI; add_len = 2; }
    if( PR.f_scan_bad )  flags |= JDF_BAD_SCAN;
    if( PR.f_multi_soi ) flags |= JDF_MULTI_SOI;

    if( snprintf( name, sizeof(name), "%s%08X.jpg", prefix, idx )>=int(sizeof(name)) ) {
      fprintf( stderr, "jpegdet: prefix too long\n" ); fclose(g); fclose(f); return 2;
    }
    gj = fopen( name, "wb" );
    if( gj==0 ) { fprintf( stderr, "jpegdet: cannot write %s\n", name ); fclose(g); fclose(f); return 2; }

    // The gap before the image, hashed and stored inline in the metainfo.
    gap = s-committed;
    putc( JDR_IMAGE, g );
    put_u64( g, gap );
    if( copy_range( f, committed, s, g, &H )!=gap ) {
      fprintf( stderr, f_wrerr ? "jpegdet: write error on the metainfo file\n"
                               : "jpegdet: short read on the gap before image %u\n", idx );
      fclose(gj); fclose(g); fclose(f); return 2;
    }
    n_gap += gap;

    // The image itself, byte for byte out of the stream.
    if( copy_range( f, s, e, gj, &H )!=e-s ) {
      fprintf( stderr, f_wrerr ? "jpegdet: write error on %s%08X.jpg\n"
                               : "jpegdet: short read on image %s%08X\n", prefix, idx );
      fclose(gj); fclose(g); fclose(f); return 2;
    }
    // A stream that ends mid-image leaves a file no decoder will open.  Two
    // bytes fix that, and add_len tells "d" to leave them behind.
    if( flags & JDF_ADDED_EOI ) { putc( 0xFF, gj ); putc( 0xD9, gj ); }
    if( !close_ok( gj ) ) {
      fprintf( stderr, "jpegdet: write error on %s%08X.jpg\n", prefix, idx );
      fclose(g); fclose(f); return 2;
    }

    put_u64( g, e-s );
    put_u32( g, add_len );
    put_u32( g, flags );
    put_u32( g, idx );

    if( f_verbose ) {
      printf( "%08X  %12llu .. %-12llu  %10llu bytes%s%s%s\n", idx,
              (unsigned long long)s, (unsigned long long)e,
              (unsigned long long)(e-s),
              (flags & JDF_ADDED_EOI) ? "  [EOI added]"   : "",
              (flags & JDF_BAD_SCAN)  ? "  [damaged scan]": "",
              (flags & JDF_MULTI_SOI) ? "  [cut at the next image]" : "" );
    }

    n_img += e-s;
    idx++;
    committed = e;
    pos = e;
  }

  // Everything after the last image.
  gap = file_size-committed;
  putc( JDR_END, g );
  put_u64( g, gap );
  if( copy_range( f, committed, file_size, g, &H )!=gap ) {
    fprintf( stderr, f_wrerr ? "jpegdet: write error on the metainfo file\n"
                             : "jpegdet: short read on the trailing data\n" );
    fclose(g); fclose(f); return 2;
  }
  n_gap += gap;
  put_u64( g, file_size );
  put_u64( g, H.h );
  fwrite( JDM_END, 1, 6, g );

  if( !close_ok( g ) ) { fprintf( stderr, "jpegdet: write error on the metainfo file\n" ); fclose(f); return 2; }
  fclose( f );

  printf( "%u image(s), %llu image byte(s), %llu literal byte(s), %llu total\n",
          idx, (unsigned long long)n_img, (unsigned long long)n_gap,
          (unsigned long long)file_size );

  return 0;
}

//=============================================================================
// d: restore
//=============================================================================

static int cmd_d( const char* prefix, const char* outname ) {
  char name[1024], magic[9];
  FILE *f,*g,*fj;
  qword gap, img_len, orig_size, orig_hash, written, have;
  uint tag, version, add_len, flags, idx, n;
  Fnv H;

  if( snprintf( name, sizeof(name), "%s.jdm", prefix )>=int(sizeof(name)) ) {
    fprintf( stderr, "jpegdet: prefix too long\n" ); return 2;
  }
  f = fopen( name, "rb" );
  if( f==0 ) { fprintf( stderr, "jpegdet: cannot read %s\n", name ); return 2; }

  if( fread( magic, 1, 8, f )!=8 || memcmp( magic, JDM_MAGIC, 8 )!=0 ) {
    fprintf( stderr, "jpegdet: %s is not a jpegdet metainfo file\n", name );
    fclose(f); return 2;
  }
  f_bad = 0;
  version = get_u32( f );
  if( f_bad || version!=JDM_VERSION ) {
    fprintf( stderr, "jpegdet: metainfo version %u, this build writes %u\n", version, JDM_VERSION );
    fclose(f); return 2;
  }

  g = fopen( outname, "wb" );
  if( g==0 ) { fprintf( stderr, "jpegdet: cannot write %s\n", outname ); fclose(f); return 2; }

  H.init();
  written = 0;
  n = 0;

  while(1) {
    tag = uint( getc(f) );
    if( tag==uint(EOF) ) { fprintf( stderr, "jpegdet: metainfo ends without a terminator\n" ); goto fail; }

    if( tag==JDR_END ) {
      gap = get_u64( f );
      if( f_bad ) goto truncated;
      // The trailing literal data, straight out of the metainfo.
      while( gap>0 ) {
        uint want = uint( (gap < qword(sizeof(copybuf))) ? gap : qword(sizeof(copybuf)) );
        uint got  = uint( fread( copybuf, 1, want, f ) );
        if( got==0 ) goto truncated;
        H.add( copybuf, got );
        if( fwrite( copybuf, 1, got, g )!=got ) goto wrfail;
        written += got; gap -= got;
      }
      orig_size = get_u64( f );
      orig_hash = get_u64( f );
      if( f_bad ) goto truncated;
      if( fread( magic, 1, 6, f )!=6 || memcmp( magic, JDM_END, 6 )!=0 ) {
        fprintf( stderr, "jpegdet: metainfo terminator is corrupt\n" ); goto fail;
      }
      break;
    }

    if( tag!=JDR_IMAGE ) {
      fprintf( stderr, "jpegdet: unknown record type '%c' in the metainfo\n", (tag>=0x20 && tag<0x7F) ? int(tag) : '?' );
      goto fail;
    }

    // The literal run before this image.
    gap = get_u64( f );
    if( f_bad ) goto truncated;
    while( gap>0 ) {
      uint want = uint( (gap < qword(sizeof(copybuf))) ? gap : qword(sizeof(copybuf)) );
      uint got  = uint( fread( copybuf, 1, want, f ) );
      if( got==0 ) goto truncated;
      H.add( copybuf, got );
      if( fwrite( copybuf, 1, got, g )!=got ) goto wrfail;
      written += got; gap -= got;
    }

    img_len = get_u64( f );
    add_len = get_u32( f );
    flags   = get_u32( f );
    idx     = get_u32( f );
    if( f_bad ) goto truncated;
    (void)flags;

    if( snprintf( name, sizeof(name), "%s%08X.jpg", prefix, idx )>=int(sizeof(name)) ) {
      fprintf( stderr, "jpegdet: prefix too long\n" ); goto fail;
    }
    fj = fopen( name, "rb" );
    if( fj==0 ) {
      fprintf( stderr, "jpegdet: %s is missing -- cannot rebuild the stream without it\n", name );
      goto fail;
    }
    // The file must be exactly what "c" wrote: the stream's bytes, plus
    // whatever was appended to make it decodable.  Anything else means it was
    // edited or replaced, and quietly splicing it in would corrupt the output.
    have = file_size_of( fj );
    if( have != img_len+add_len ) {
      fprintf( stderr, "jpegdet: %s is %llu bytes, the metainfo says %llu\n",
               name, (unsigned long long)have, (unsigned long long)(img_len+add_len) );
      fclose(fj); goto fail;
    }
    rewind( fj );
    {
      qword left = img_len;
      while( left>0 ) {
        uint want = uint( (left < qword(sizeof(copybuf))) ? left : qword(sizeof(copybuf)) );
        uint got  = uint( fread( copybuf, 1, want, fj ) );
        if( got==0 ) { fprintf( stderr, "jpegdet: short read on %s\n", name ); fclose(fj); goto fail; }
        H.add( copybuf, got );
        if( fwrite( copybuf, 1, got, g )!=got ) { fclose(fj); goto wrfail; }
        written += got; left -= got;
      }
    }
    fclose( fj );
    n++;
  }

  if( !close_ok( g ) ) goto wrfail_closed;
  fclose( f );

  if( written!=orig_size ) {
    fprintf( stderr, "jpegdet: rebuilt %llu bytes, the original was %llu\n",
             (unsigned long long)written, (unsigned long long)orig_size );
    return 1;
  }
  if( H.h!=orig_hash ) {
    fprintf( stderr, "jpegdet: the rebuilt stream does not match the original's hash\n" );
    return 1;
  }

  printf( "%u image(s), %llu bytes restored and verified\n", n, (unsigned long long)written );
  return 0;

truncated:
  fprintf( stderr, "jpegdet: the metainfo file is truncated\n" );
  goto fail;
wrfail:
  fclose(g);
wrfail_closed:
  fprintf( stderr, "jpegdet: write error on %s\n", outname );
  fclose(f);
  return 2;
fail:
  fclose(g); fclose(f);
  return 2;
}

//=============================================================================

static void usage( const char* me ) {
  fprintf( stderr,
    "usage: %s [options] c <input> <prefix>    extract the JPEGs from a stream\n"
    "       %s d <prefix> <output>             put the stream back together\n"
    "\n"
    "\"c\" writes one <prefix>NNNNNNNN.jpg per image plus <prefix>.jdm, which holds\n"
    "everything else in the stream.  \"d\" needs only those and rebuilds the input\n"
    "byte for byte, verifying its size and hash.\n"
    "\n"
    "  -s    structure only: do not decode the entropy-coded scans.  Much faster.\n"
    "        Scan headers and Huffman tables are still checked, but nothing\n"
    "        verifies the data itself, so a candidate is likelier to be believed.\n"
    "  -r    relaxed: accept an image whose entropy data does not decode cleanly.\n"
    "  -t    also carve a final image that the stream cut short, appending the EOI\n"
    "        it needs to open (\"d\" removes it again).\n"
    "  -A    anchor only on FF D8 FF.  Faster on a big stream, but misses an\n"
    "        image whose SOI is followed by something other than a marker.\n"
    "  -m N  ignore anything shorter than N bytes (default 128).\n"
    "  -L N  give up on a candidate that reads N bytes without a frame header\n"
    "        (default 16777216).\n"
    "  -v    list every image as it is found, and every candidate turned down.\n", me, me );
}

int main( int argc, char** argv ) {

  Opts O;
  int argi = 1;

  O.f_entropy = 1;
  O.f_relaxed = 0;
  O.f_tail    = 0;
  O.min_size  = 128;
  O.sof_limit = 0;             // 0 = leave JpegProbe's default in place
  O.siglen    = 2;

  while( (argi<argc) && (argv[argi][0]=='-') && argv[argi][1] ) {
    const char* o = argv[argi]+1;
    if( o[1]!=0 ) { usage( argv[0] ); return 2; }
    switch( o[0] ) {
    case 's': O.f_entropy = 0; argi++; continue;
    case 'r': O.f_relaxed = 1; argi++; continue;
    case 't': O.f_tail    = 1; argi++; continue;
    case 'v': f_verbose   = 1; argi++; continue;
    case 'A': O.siglen    = 3; argi++; continue;
    case 'm': if( argi+1<argc ) { O.min_size  = qword(strtoull(argv[argi+1],0,0)); argi+=2; continue; } break;
    case 'L': if( argi+1<argc ) { O.sof_limit = qword(strtoull(argv[argi+1],0,0)); argi+=2; continue; } break;
    }
    usage( argv[0] ); return 2;
  }

  if( argi+3 > argc ) { usage( argv[0] ); return 2; }

  if( (argv[argi][0]=='c') && (argv[argi][1]==0) )
    return cmd_c( argv[argi+1], argv[argi+2], O );

  if( (argv[argi][0]=='d') && (argv[argi][1]==0) )
    return cmd_d( argv[argi+1], argv[argi+2] );

  usage( argv[0] );
  return 2;
}
