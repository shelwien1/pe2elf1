
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
//   "JPEGDET2"  u32 version
//   then a sequence of records, each introduced by one tag byte:
//     'C'  an image definition, for a thumbnail
//     'I'  u64 gap_len, gap_len bytes, then an image definition
//     'E'  u64 tail_len, tail_len bytes, u64 orig_size, u64 orig_hash, "JDMEND"
//
//   an image definition:
//     u32 index      which <prefix>NNNNNNNN.jpg holds it
//     u64 file_len   bytes of that file that are image data
//     u32 add_len    bytes of it that are not (a synthesised EOI)
//     u32 flags
//     u64 orig_len   what putting the thumbnails back has to produce
//     u32 n_thumbs, then for each:
//       u64 cut_at     offset in the reassembled image where the span goes back
//       u32 pre_len, pre_len bytes      what sat before the thumbnail in it
//       u32 child      the index of the file holding the thumbnail
//       u64 child_len  its own reassembled length
//       u32 post_len, post_len bytes    what sat after it
//       u32 n_fix, then for each:
//         u64 offset, u32 len, len bytes    what the patch overwrote
//
// Record-tagged rather than counted so that "c" can write it in one forward
// pass: the number of images is not known until the input is exhausted, and a
// header field patched by seeking back is a field that is wrong whenever the
// program is killed.  The size and hash live in the terminator for the same
// reason.  A thumbnail's definition is written before the definition of the
// image that carried it, so a single forward pass through the file is also all
// "d" needs -- everything a definition refers to has already been read.
//
// The image bytes themselves are not here -- they are the .jpg files, which is
// the whole point.  What is here is everything needed to undo the two things
// "c" does to an image: the gap of stream data before it, and the thumbnails
// lifted out of it.  index names the file rather than leaving it implicit in
// record order, so a deleted or renamed .jpg is an error message instead of a
// silently misassembled output.
//=============================================================================

#define JDM_MAGIC   "JPEGDET2"
#define JDM_END     "JDMEND"
#define JDM_VERSION 2

enum {
  JDR_IMAGE = 'I',      // the gap before a top-level image
  JDR_IMAGE_DEF = 'D',  // that image's own definition, straight after the gap
  JDR_CHILD = 'C',      // a thumbnail lifted out of another image
  JDR_END   = 'E',
};

enum {
  JDF_ADDED_EOI = 1,   // the .jpg carries an EOI that the stream did not
  JDF_BAD_SCAN  = 2,   // accepted under -r with entropy data that did not decode
  JDF_MULTI_SOI = 4,   // cut short by the next image's SOI, not by an EOI
  JDF_THUMB     = 8,   // this image is a thumbnail lifted out of another
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
  uint  f_thumbs;               // record where the thumbnails are
  qword sof_limit;              // give up on a candidate that has read this far
                                // without producing a frame header

  // Filled in by probe():
  qword end_pos;                // absolute offset one past the last byte used
  uint  f_eoi;                  // the parse ended on a real EOI
  uint  f_scan_bad;             // some scan did not decode cleanly
  uint  f_multi_soi;            // two images ran together inside this one
  const char* why;              // why the candidate was turned down, for -v

  void init( void ) {
    f_entropy = 1; f_relaxed = 0; f_tail = 0; f_thumbs = 0;
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
    // No recursion is needed to find the thumbnails -- sub_begin() files a span
    // whether or not there is a level to descend into -- and none is wanted
    // either: each one is re-probed on its own, which both vouches for it and
    // finds what it carries in turn, so parsing it twice here would be waste.
    P.sub            = 0;
    P.f_thumb_spans  = f_thumbs;
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

//=============================================================================
// Lifting a thumbnail out of the image that carried it.
//
// A thumbnail is a whole JPEG living inside an APP segment of another one, so
// it can be written out as a file of its own -- but only if what is left behind
// is still a JPEG.  The two carriers need different surgery for that:
//
//   JFXX APP0   the segment is nothing but the thumbnail, so the whole segment
//               goes.  What remains is an ordinary JFIF file.
//   Exif APP1   the segment is metadata that should survive, so only the
//               thumbnail is cut, the segment length is corrected, and the
//               IFD's thumbnail-length tag is zeroed -- which is exactly what
//               an Exif block with no thumbnail looks like.
//
// Restoration puts the span back and undoes the byte edits, so the original is
// reproduced exactly either way.  Everything here is in the image's own
// coordinates; a thumbnail's own thumbnails are described the same way, in its
// coordinates, and the recursion composes.
//=============================================================================

enum { JD_MAX_TDEPTH = 32 };   // how deep the extraction will follow thumbnails

struct Fix {                   // bytes the patch overwrote, to be put back
  qword at;
  uint  len;
  byte  old[8];
  byte  neu[8];
};

struct Cut {                   // one thumbnail lifted out of one image
  qword at, end;               // the span removed from the image
  qword child_beg, child_end;  // the thumbnail inside that span
  uint  child;                 // the file it was written as
  qword child_len;             // its own reassembled length
  Fix   fix[2];
  uint  n_fix;
};

// Read len bytes at `at`; 0 if they are not there.
static uint read_at( FILE* f, qword at, byte* p, uint len ) {
  if( jd_seek( f, at )!=0 ) return 0;
  return uint( fread( p, 1, len, f ) )==len;
}

// Work out what has to change in the image at [beg,end) for the thumbnail
// `t` to be taken out of it.  Offsets in `t` are relative to beg.
static uint plan_cut( FILE* f, qword beg, const pjpg0::thumb_span& t,
                      qword child_end, Cut& c ) {
  qword seg, seg_end, tpos;
  uint  newlen;

  seg     = beg + t.seg;
  seg_end = beg + t.seg_end;
  tpos    = beg + t.pos;

  c.child_beg = tpos;
  c.child_end = child_end;
  c.n_fix     = 0;

  if( t.carrier == M_APP0 ) {
    // The whole segment exists to carry the thumbnail; nothing of it is worth
    // keeping, and nothing that remains needs correcting.
    c.at  = seg;
    c.end = seg_end;
    return 1;
  }

  // Exif: keep the segment, drop the thumbnail and whatever followed it inside
  // the segment, then make the header agree with what is left.
  if( tpos < seg+4 ) return 0;                  // not inside its own segment
  c.at  = tpos;
  c.end = seg_end;

  newlen = uint( tpos - seg ) - 2;              // the length field counts itself
  if( (newlen < 2) || (newlen > 0xFFFF) ) return 0;

  Fix& L = c.fix[c.n_fix];
  L.at = seg+2; L.len = 2;
  if( !read_at( f, L.at, L.old, 2 ) ) return 0;
  L.neu[0] = byte(newlen>>8); L.neu[1] = byte(newlen);
  c.n_fix++;

  // An IFD that still claims a thumbnail of N bytes, with the bytes gone, is
  // not an Exif block any reader should have to make sense of.  Zeroing the
  // length is how a file with no thumbnail says so.  A SHORT value lives in the
  // first two bytes of the same four, so zeroing all four covers both formats.
  if( t.fixup ) {
    Fix& T = c.fix[c.n_fix];
    T.at = beg + t.fixup; T.len = 4;
    if( !read_at( f, T.at, T.old, 4 ) ) return 0;
    T.neu[0] = T.neu[1] = T.neu[2] = T.neu[3] = 0;
    c.n_fix++;
  }
  return 1;
}

// Copy [from,to) of the input to g, hashing what was read rather than what was
// written -- the hash is of the original stream, and what goes out may be
// patched.  Returns 0 on a short read or a write error.
static uint pass_through( FILE* f, qword from, qword to, FILE* g, Fnv* h ) {
  if( to<=from ) return 1;
  return copy_range( f, from, to, g, h ) == to-from;
}

// Write the image at [beg,end) to g with `n` thumbnails taken out of it, in
// increasing order of where they were.
static uint write_patched( FILE* f, qword beg, qword end, Cut* cut, uint n, FILE* g, Fnv* h ) {
  qword p;
  uint  i,k;
  byte  scratch[8];

  p = beg;
  for( i=0; i<n; i++ ) {
    // The fixups of one cut always sit before the span it removes, so walking
    // the cuts in order walks every edit in order.
    for( k=0; k<cut[i].n_fix; k++ ) {
      Fix& F = cut[i].fix[k];
      if( !pass_through( f, p, F.at, g, h ) ) return 0;
      if( !read_at( f, F.at, scratch, F.len ) ) return 0;
      if( h ) h->add( scratch, F.len );          // the hash follows the original
      if( fwrite( F.neu, 1, F.len, g )!=F.len ) { f_wrerr=1; return 0; }
      p = F.at + F.len;
    }
    if( !pass_through( f, p, cut[i].at, g, h ) ) return 0;
    if( h ) {                                    // the span still counts, unwritten
      if( copy_range( f, cut[i].at, cut[i].end, 0, h ) != cut[i].end-cut[i].at ) return 0;
    }
    p = cut[i].end;
  }
  return pass_through( f, p, end, g, h );
}


// Write one image out as <prefix>NNNNNNNN.jpg with its thumbnails lifted into
// files of their own, and its definition into the metainfo.  A thumbnail's
// definition is written before the definition of the image that carried it, so
// reading the metainfo forwards resolves every reference as it is met.
//
// `sp` is this image's thumbnail list, snapshotted from the probe that found
// it -- the probe object is reused to look inside each thumbnail in turn, so
// its own list does not survive the first step down.
struct Opts;
static uint emit_image( FILE* f, const char* prefix, FILE* g,
                        qword beg, qword end, uint flags, uint add_len,
                        const pjpg0::thumb_span* sp, uint n_sp, uint spans_lost,
                        uint depth, uint* next_idx, Fnv* h,
                        const Opts& O, uint tag, uint* out_index, qword* out_file_len );

struct Opts {
  uint  f_entropy;   // -s clears: skip the entropy-coded scans
  uint  f_relaxed;   // -r sets:   accept damaged entropy data
  uint  f_tail;      // -t sets:   accept a final image the stream cut short
  qword min_size;    // -m N
  qword sof_limit;   // -L N
  uint  siglen;      // 3 = anchor on FF D8 FF, 2 = FF D8
  uint  f_nothumb;   // -n: leave thumbnails inside the images that carry them
};

static uint emit_image( FILE* f, const char* prefix, FILE* g,
                        qword beg, qword end, uint flags, uint add_len,
                        const pjpg0::thumb_span* sp, uint n_sp, uint spans_lost,
                        uint depth, uint* next_idx, Fnv* h,
                        const Opts& O, uint tag, uint* out_index, qword* out_file_len ) {
  char  name[1024];
  FILE* gj;
  Cut   cut[pjpg0::MAX_THUMBS];
  pjpg0::thumb_span kid[pjpg0::MAX_THUMBS];
  uint  n_cut,i,k,n_kid,kid_lost,cidx;
  qword file_len,taken,cbeg,cend,clen;

  cidx = (*next_idx)++;
  n_cut = 0;

  // Follow each thumbnail down.  The probe both vouches for it -- only a
  // thumbnail that parses as a JPEG on its own can be written out as one -- and
  // tells us what it carries in turn.
  if( !O.f_nothumb && (depth < JD_MAX_TDEPTH) ) {
    for( i=0; i<n_sp; i++ ) {
      if( n_cut >= uint(pjpg0::MAX_THUMBS) ) break;
      cbeg = beg + sp[i].pos;
      if( (cbeg < beg) || (cbeg >= end) ) continue;

      if( !PR.probe( f, cbeg, beg + sp[i].pos + sp[i].len ) ) continue;
      cend = PR.end_pos;
      if( (cend <= cbeg) || (cend > end) ) continue;

      n_kid = PR.P.n_thumbs; kid_lost = PR.P.f_thumbs_lost;
      if( n_kid > uint(pjpg0::MAX_THUMBS) ) n_kid = uint(pjpg0::MAX_THUMBS);
      for( k=0; k<n_kid; k++ ) kid[k] = PR.P.thumbs[k];

      if( !plan_cut( f, beg, sp[i], cend, cut[n_cut] ) ) continue;

      if( !emit_image( f, prefix, g, cbeg, cend, JDF_THUMB, 0,
                       kid, n_kid, kid_lost, depth+1, next_idx, 0, O,
                       JDR_CHILD, &cut[n_cut].child, &clen ) ) return 0;
      cut[n_cut].child_len = cend - cbeg;
      (void)clen;
      n_cut++;
    }
  }
  if( spans_lost && f_verbose )
    printf( "          %12llu    more thumbnails than the parser tracks, some left in place\n",
            (unsigned long long)beg );

  // --- the image file ------------------------------------------------------
  if( snprintf( name, sizeof(name), "%s%08X.jpg", prefix, cidx )>=int(sizeof(name)) ) {
    fprintf( stderr, "jpegdet: prefix too long\n" ); return 0;
  }
  gj = fopen( name, "wb" );
  if( gj==0 ) { fprintf( stderr, "jpegdet: cannot write %s\n", name ); return 0; }

  if( !write_patched( f, beg, end, cut, n_cut, gj, h ) ) {
    fprintf( stderr, f_wrerr ? "jpegdet: write error on %s\n"
                             : "jpegdet: short read while writing %s\n", name );
    fclose(gj); return 0;
  }
  if( flags & JDF_ADDED_EOI ) { putc( 0xFF, gj ); putc( 0xD9, gj ); }
  if( !close_ok( gj ) ) { fprintf( stderr, "jpegdet: write error on %s\n", name ); return 0; }

  taken = 0;
  for( i=0; i<n_cut; i++ ) taken += cut[i].end - cut[i].at;
  file_len = (end-beg) - taken;

  // --- its definition ------------------------------------------------------
  putc( tag, g );
  put_u32( g, cidx );
  put_u64( g, file_len );
  put_u32( g, add_len );
  put_u32( g, flags );
  put_u64( g, end-beg );
  put_u32( g, n_cut );
  for( i=0; i<n_cut; i++ ) {
    Cut& c = cut[i];
    put_u64( g, c.at - beg );
    put_u32( g, uint( c.child_beg - c.at ) );
    if( copy_range( f, c.at, c.child_beg, g, 0 ) != c.child_beg-c.at ) return 0;
    put_u32( g, c.child );
    put_u64( g, c.child_len );
    put_u32( g, uint( c.end - c.child_end ) );
    if( copy_range( f, c.child_end, c.end, g, 0 ) != c.end-c.child_end ) return 0;
    put_u32( g, c.n_fix );
    for( k=0; k<c.n_fix; k++ ) {
      put_u64( g, c.fix[k].at - beg );
      put_u32( g, c.fix[k].len );
      if( fwrite( c.fix[k].old, 1, c.fix[k].len, g )!=c.fix[k].len ) { f_wrerr=1; return 0; }
    }
  }

  if( f_verbose && (tag==JDR_CHILD) )
    printf( "          %08X  thumbnail %10llu bytes%s\n", cidx,
            (unsigned long long)(end-beg), n_cut ? "  (and its own)" : "" );

  *out_index    = cidx;
  *out_file_len = file_len;
  return 1;
}

static int cmd_c( const char* inpname, const char* prefix, const Opts& O ) {
  char name[1024];
  FILE *f,*g,*gj;
  qword file_size, pos, committed, s, e, gap, n_gap, n_img, this_len;
  uint idx, n_top, flags, add_len, ok, k, n_sp, sp_lost, this_idx;
  pjpg0::thumb_span sp[pjpg0::MAX_THUMBS];
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
  PR.f_thumbs  = !O.f_nothumb;
  PR.f_entropy = O.f_entropy;
  PR.f_relaxed = O.f_relaxed;
  PR.f_tail    = O.f_tail;
  if( O.sof_limit ) PR.sof_limit = O.sof_limit;
  SC.init( f, O.siglen );
  H.init();

  pos = 0; committed = 0; idx = 0; n_top = 0; n_gap = 0; n_img = 0;

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

    // The probe object is about to be reused on this image's thumbnails, so
    // take what it found before stepping down into them.
    n_sp = PR.P.n_thumbs; sp_lost = PR.P.f_thumbs_lost;
    if( n_sp > uint(pjpg0::MAX_THUMBS) ) n_sp = uint(pjpg0::MAX_THUMBS);
    for( k=0; k<n_sp; k++ ) sp[k] = PR.P.thumbs[k];

    // The gap before the image, hashed and stored inline in the metainfo.  It
    // goes out before the image's own record, and so before the records of the
    // thumbnails that come out of it -- but those carry no gap of their own, so
    // the literal data still reaches "d" in stream order.
    gap = s-committed;
    putc( JDR_IMAGE, g );
    put_u64( g, gap );
    if( copy_range( f, committed, s, g, &H )!=gap ) {
      fprintf( stderr, f_wrerr ? "jpegdet: write error on the metainfo file\n"
                               : "jpegdet: short read on the gap before image %u\n", idx );
      fclose(g); fclose(f); return 2;
    }
    n_gap += gap;

    if( !emit_image( f, prefix, g, s, e, flags, add_len, sp, n_sp, sp_lost,
                     0, &idx, &H, O, JDR_IMAGE_DEF, &this_idx, &this_len ) ) {
      fclose(g); fclose(f); return 2;
    }

    if( f_verbose ) {
      printf( "%08X  %12llu .. %-12llu  %10llu bytes%s%s%s\n", this_idx,
              (unsigned long long)s, (unsigned long long)e,
              (unsigned long long)(e-s),
              (flags & JDF_ADDED_EOI) ? "  [EOI added]"   : "",
              (flags & JDF_BAD_SCAN)  ? "  [damaged scan]": "",
              (flags & JDF_MULTI_SOI) ? "  [cut at the next image]" : "" );
    }

    n_img += e-s;      // emit_image allocated the indices, this one and its thumbnails
    n_top++;
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

  if( idx > n_top )
    printf( "%u image(s) and %u thumbnail(s), %llu image byte(s), %llu literal byte(s), %llu total\n",
            n_top, idx-n_top, (unsigned long long)n_img, (unsigned long long)n_gap,
            (unsigned long long)file_size );
  else
    printf( "%u image(s), %llu image byte(s), %llu literal byte(s), %llu total\n",
            n_top, (unsigned long long)n_img, (unsigned long long)n_gap,
            (unsigned long long)file_size );

  return 0;
}

//=============================================================================
// d: restore
//=============================================================================

//=============================================================================
// d: restore
//
// Reassembly is the mirror of emit_image: an image's file holds it with its
// thumbnails taken out, and its definition says where they went and what the
// patch overwrote.  Putting one back means walking its file, stopping at each
// recorded offset to undo a byte edit or to drop a thumbnail in -- and a
// thumbnail is an image, so dropping one in is the same walk again, one level
// down, written straight through to the output rather than assembled anywhere.
//=============================================================================

struct RFix { qword at; uint len; byte old[8]; };

struct RThumb {
  qword at;              // where the span goes back, in the reassembled image
  uint  pre_len;  byte* pre;
  uint  child;
  qword child_len;
  uint  post_len; byte* post;
  uint  n_fix;    RFix  fix[2];
};

struct RDef {
  uint   used;
  qword  file_len;
  uint   add_len, flags;
  qword  orig_len;
  uint   n_thumbs;
  RThumb* th;
};

static RDef*  rdef;
static uint   n_rdef;

static uint rdef_room( uint idx ) {
  uint want;
  RDef* p;
  if( idx < n_rdef ) return 1;
  if( idx > 0x00FFFFFF ) return 0;                 // an index that size is corruption
  want = idx + 64;
  p = (RDef*)realloc( rdef, want*sizeof(RDef) );
  if( p==0 ) return 0;
  rdef = p;
  memset( rdef+n_rdef, 0, (want-n_rdef)*sizeof(RDef) );
  n_rdef = want;
  return 1;
}

static void rdef_free( void ) {
  uint i,k;
  for( i=0; i<n_rdef; i++ ) {
    if( rdef[i].th ) {
      for( k=0; k<rdef[i].n_thumbs; k++ ) { free(rdef[i].th[k].pre); free(rdef[i].th[k].post); }
      free( rdef[i].th );
    }
  }
  free( rdef ); rdef = 0; n_rdef = 0;
}

// Read len bytes of the metainfo into a fresh block.  Zero length is a null
// pointer, not a zero-byte allocation, so the free path stays uniform.
static byte* get_blob( FILE* f, uint len ) {
  byte* p;
  if( len==0 ) return 0;
  p = (byte*)malloc( len );
  if( p==0 ) { f_bad=1; return 0; }
  if( fread( p, 1, len, f )!=len ) { f_bad=1; free(p); return 0; }
  return p;
}

// One image definition, as written by emit_image.
static uint read_def( FILE* f, uint* out_index ) {
  uint idx,i,k;
  RDef* D;

  idx = get_u32( f );
  if( f_bad || !rdef_room( idx ) ) return 0;
  D = &rdef[idx];
  if( D->used ) return 0;                          // the same index defined twice

  D->file_len = get_u64( f );
  D->add_len  = get_u32( f );
  D->flags    = get_u32( f );
  D->orig_len = get_u64( f );
  D->n_thumbs = get_u32( f );
  if( f_bad ) return 0;
  if( D->n_thumbs > uint(pjpg0::MAX_THUMBS) ) return 0;

  if( D->n_thumbs ) {
    D->th = (RThumb*)calloc( D->n_thumbs, sizeof(RThumb) );
    if( D->th==0 ) return 0;
  }
  for( i=0; i<D->n_thumbs; i++ ) {
    RThumb& T = D->th[i];
    T.at       = get_u64( f );
    T.pre_len  = get_u32( f );  if( f_bad ) return 0;
    T.pre      = get_blob( f, T.pre_len );  if( f_bad ) return 0;
    T.child    = get_u32( f );
    T.child_len= get_u64( f );
    T.post_len = get_u32( f );  if( f_bad ) return 0;
    T.post     = get_blob( f, T.post_len ); if( f_bad ) return 0;
    T.n_fix    = get_u32( f );  if( f_bad ) return 0;
    if( T.n_fix > uint(DIM(T.fix)) ) return 0;
    for( k=0; k<T.n_fix; k++ ) {
      T.fix[k].at  = get_u64( f );
      T.fix[k].len = get_u32( f );
      if( f_bad || (T.fix[k].len > uint(DIM(T.fix[k].old))) ) return 0;
      if( fread( T.fix[k].old, 1, T.fix[k].len, f )!=T.fix[k].len ) { f_bad=1; return 0; }
    }
    if( T.child >= n_rdef || !rdef[T.child].used ) return 0;   // must be defined already
  }

  D->used = 1;
  *out_index = idx;
  return 1;
}

// Copy n bytes of fj to g, hashing them.  Returns 0 short.
static uint copy_file( FILE* fj, qword n, FILE* g, Fnv* h ) {
  uint want,got;
  while( n>0 ) {
    want = uint( (n < qword(sizeof(copybuf))) ? n : qword(sizeof(copybuf)) );
    got  = uint( fread( copybuf, 1, want, fj ) );
    if( got==0 ) return 0;
    h->add( copybuf, got );
    if( fwrite( copybuf, 1, got, g )!=got ) { f_wrerr=1; return 0; }
    n -= got;
  }
  return 1;
}

static uint emit_blob( const byte* p, uint n, FILE* g, Fnv* h ) {
  if( n==0 ) return 1;
  h->add( p, n );
  if( fwrite( p, 1, n, g )!=n ) { f_wrerr=1; return 0; }
  return 1;
}

// Write image `idx`'s original bytes to g.
static uint assemble( const char* prefix, uint idx, FILE* g, Fnv* h, uint depth ) {
  char  name[1024];
  FILE* fj;
  RDef* D;
  uint  i,k,ok,below;
  qword have,orig_p,file_p,upto;

  if( depth > JD_MAX_TDEPTH+1 ) { fprintf( stderr, "jpegdet: thumbnails nested past the limit\n" ); return 0; }
  if( idx >= n_rdef || !rdef[idx].used ) {
    fprintf( stderr, "jpegdet: the metainfo refers to image %08X, which it never defines\n", idx );
    return 0;
  }
  D = &rdef[idx];

  if( snprintf( name, sizeof(name), "%s%08X.jpg", prefix, idx )>=int(sizeof(name)) ) {
    fprintf( stderr, "jpegdet: prefix too long\n" ); return 0;
  }
  fj = fopen( name, "rb" );
  if( fj==0 ) {
    fprintf( stderr, "jpegdet: %s is missing -- cannot rebuild the stream without it\n", name );
    return 0;
  }
  // The file has to be exactly what "c" wrote: the image with its thumbnails
  // taken out, plus whatever was appended to make it open.  Anything else means
  // it was edited or replaced, and splicing it in regardless would corrupt the
  // output without saying so.
  have = file_size_of( fj );
  if( have != D->file_len + D->add_len ) {
    fprintf( stderr, "jpegdet: %s is %llu bytes, the metainfo says %llu\n",
             name, (unsigned long long)have, (unsigned long long)(D->file_len + D->add_len) );
    fclose(fj); return 0;
  }
  rewind( fj );

  ok = 1; below = 0;
  orig_p = 0;                       // how much of the original has been written
  file_p = 0;                       // how much of the file has been read
  for( i=0; ok && (i<D->n_thumbs); i++ ) {
    RThumb& T = D->th[i];

    // Undo the byte edits the patch made.  They sit inside the part of the
    // image the file still holds, ahead of the span this thumbnail came out of.
    for( k=0; ok && (k<T.n_fix); k++ ) {
      RFix& F = T.fix[k];
      if( (F.at < orig_p) || (F.at + F.len > T.at) ) { ok = 0; break; }
      upto = F.at - orig_p;
      if( !copy_file( fj, upto, g, h ) ) { ok=0; break; }
      orig_p += upto; file_p += upto;
      if( jd_seek( fj, file_p + F.len )!=0 ) { ok=0; break; }   // drop the patched bytes
      if( !emit_blob( F.old, F.len, g, h ) ) { ok=0; break; }
      orig_p += F.len; file_p += F.len;
    }
    if( !ok ) break;

    if( T.at < orig_p ) { ok = 0; break; }
    upto = T.at - orig_p;
    if( !copy_file( fj, upto, g, h ) ) { ok=0; break; }
    orig_p += upto; file_p += upto;

    // The span itself: what framed the thumbnail, the thumbnail, and whatever
    // followed it inside the same segment.
    if( !emit_blob( T.pre, T.pre_len, g, h ) ) { ok=0; break; }
    if( !assemble( prefix, T.child, g, h, depth+1 ) ) { ok=0; below=1; break; }
    if( !emit_blob( T.post, T.post_len, g, h ) ) { ok=0; break; }
    orig_p += T.pre_len + T.child_len + T.post_len;
  }

  if( ok && (file_p <= D->file_len) ) {
    if( !copy_file( fj, D->file_len - file_p, g, h ) ) ok = 0;
    else orig_p += D->file_len - file_p;
  } else if( ok ) ok = 0;

  fclose( fj );
  if( !ok ) {
    // A thumbnail that could not be rebuilt has already said why; saying that
    // its parent therefore does not fit adds nothing but noise.
    if( f_wrerr || below ) return 0;
    fprintf( stderr, "jpegdet: %s does not fit the metainfo's description of it\n", name );
    return 0;
  }
  if( orig_p != D->orig_len ) {
    fprintf( stderr, "jpegdet: rebuilding image %08X gave %llu bytes, the metainfo says %llu\n",
             idx, (unsigned long long)orig_p, (unsigned long long)D->orig_len );
    return 0;
  }
  return 1;
}

static int cmd_d( const char* prefix, const char* outname ) {
  char magic[9];
  FILE *f,*g;
  qword gap, orig_size, orig_hash, written;
  uint tag, version, idx, n;
  Fnv H;

  {
    char name[1024];
    if( snprintf( name, sizeof(name), "%s.jdm", prefix )>=int(sizeof(name)) ) {
      fprintf( stderr, "jpegdet: prefix too long\n" ); return 2;
    }
    f = fopen( name, "rb" );
    if( f==0 ) { fprintf( stderr, "jpegdet: cannot read %s\n", name ); return 2; }
    if( fread( magic, 1, 8, f )!=8 || memcmp( magic, JDM_MAGIC, 8 )!=0 ) {
      fprintf( stderr, "jpegdet: %s is not a jpegdet metainfo file\n", name );
      fclose(f); return 2;
    }
  }
  f_bad = 0;
  version = get_u32( f );
  if( f_bad || version!=JDM_VERSION ) {
    fprintf( stderr, "jpegdet: metainfo version %u, this build writes %u\n", version, JDM_VERSION );
    fclose(f); return 2;
  }

  g = fopen( outname, "wb" );
  if( g==0 ) { fprintf( stderr, "jpegdet: cannot write %s\n", outname ); fclose(f); return 2; }

  H.init(); written = 0; n = 0; f_wrerr = 0;
  orig_size = 0; orig_hash = 0;

  while(1) {
    tag = uint( getc(f) );
    if( tag==uint(EOF) ) { fprintf( stderr, "jpegdet: metainfo ends without a terminator\n" ); goto fail; }

    if( tag==JDR_END ) {
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
      orig_size = get_u64( f );
      orig_hash = get_u64( f );
      if( f_bad ) goto truncated;
      if( fread( magic, 1, 6, f )!=6 || memcmp( magic, JDM_END, 6 )!=0 ) {
        fprintf( stderr, "jpegdet: metainfo terminator is corrupt\n" ); goto fail;
      }
      break;
    }

    if( tag==JDR_CHILD ) {                      // a thumbnail, defined for later
      if( !read_def( f, &idx ) ) goto badrec;
      continue;
    }

    if( tag==JDR_IMAGE ) {                      // the gap before a top-level image
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
      continue;
    }

    if( tag==JDR_IMAGE_DEF ) {                  // ... and the image itself
      if( !read_def( f, &idx ) ) goto badrec;
      if( !assemble( prefix, idx, g, &H, 0 ) ) { if( f_wrerr ) goto wrfail; goto fail; }
      written += rdef[idx].orig_len;
      n++;
      continue;
    }

    fprintf( stderr, "jpegdet: unknown record type '%c' in the metainfo\n",
             (tag>=0x20 && tag<0x7F) ? int(tag) : '?' );
    goto fail;
  }

  if( !close_ok( g ) ) goto wrfail_closed;
  fclose( f );
  rdef_free();

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

badrec:
  fprintf( stderr, f_bad ? "jpegdet: the metainfo file is truncated\n"
                         : "jpegdet: a malformed image definition in the metainfo\n" );
  goto fail;
truncated:
  fprintf( stderr, "jpegdet: the metainfo file is truncated\n" );
  goto fail;
wrfail:
  fclose(g);
wrfail_closed:
  fprintf( stderr, "jpegdet: write error on %s\n", outname );
  fclose(f); rdef_free();
  remove( outname );
  return 2;
fail:
  fclose(g); fclose(f); rdef_free();
  // Whatever was written is a prefix of the original at best.  Leaving it named
  // as the restoration invites it being taken for one.
  remove( outname );
  return 2;
}

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
    "  -n    leave thumbnails where they are.  By default a JPEG thumbnail is\n"
    "        written out as an image of its own and the segment that carried it\n"
    "        is patched, so what is left is still a JPEG -- without one.\n"
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
  O.f_nothumb = 0;

  while( (argi<argc) && (argv[argi][0]=='-') && argv[argi][1] ) {
    const char* o = argv[argi]+1;
    if( o[1]!=0 ) { usage( argv[0] ); return 2; }
    switch( o[0] ) {
    case 's': O.f_entropy = 0; argi++; continue;
    case 'r': O.f_relaxed = 1; argi++; continue;
    case 't': O.f_tail    = 1; argi++; continue;
    case 'n': O.f_nothumb = 1; argi++; continue;
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
