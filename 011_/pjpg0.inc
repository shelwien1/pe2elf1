
// The next nesting level is a full pjpg (declared in pjpg1.inc): a thumbnail is
// an ordinary JPEG, so it is parsed by an ordinary parser.  These members are
// declared here because the marker parsers below are pjpg0 members, but their
// bodies have to wait until pjpg is a complete type.
struct pjpg;

struct pjpg0 : cinfo {

  void* state;
  uint A,B,C,D;
  uint coro2_c;
  uint coro2_hl;

  uint tag_id;
  uint l_tag;
  uint f_SOS;  

  // Where the segment being parsed starts, and how big its payload is.  The
  // outer loop sets both once per segment, so a parser can say where in the
  // stream any byte it has read came from without anything being counted on
  // the byte-at-a-time path:  l_tag is what is left of the payload, so
  // seg_payload - l_tag is what has been eaten, and the payload itself begins
  // four bytes past the FF (marker, then the two-byte length).
  qword seg_pos;
  uint  seg_payload;
  qword tag_at( void ) { return seg_pos + 4 + qword(seg_payload - l_tag); }

  uint c_tag[256];

  // Recursion into an embedded JPEG thumbnail.  sub is the next level's parser,
  // or 0 at the deepest level, wired up by pjpg1 in pjpg.cpp.
  pjpg* sub;
  uint  sub_active;
  uint  n_thumb;

  // Where each thumbnail sits, for a caller that wants to lift it out.  All
  // offsets are in this parser's own input, so level 0 reports them in the
  // stream and a nested level reports them inside the thumbnail it is parsing
  // -- which composes, because that thumbnail's own position is in the record
  // one level up.
  //
  // Only recorded when f_thumb_spans is set, and only for a thumbnail that
  // parsed as a JPEG: one that hit the depth limit was never checked, so
  // nothing here vouches for it.
  struct thumb_span {
    qword pos, len;      // the thumbnail's bytes
    qword seg, seg_end;  // the segment carrying it: its FF, and one past its end
    qword fixup;         // the Exif thumbnail-length tag's value field, else 0
    uint  carrier;       // M_APP0 (JFXX) or M_APP1 (Exif)
    uint  ok;            // it parsed cleanly, so it can stand on its own
  };
  enum { MAX_THUMBS = 8 };
  thumb_span thumbs[MAX_THUMBS];
  uint n_thumbs;
  uint f_thumb_spans;
  uint f_thumbs_lost;    // more than MAX_THUMBS: the list is no longer complete

  // Thumbnail bytes arrive one at a time, because that is how a marker parser
  // is fed, but handing them over one at a time would cost a coroutine switch
  // -- two stack memcpys -- per byte.  They are batched through this buffer
  // instead, which keeps the handover streaming and bounded: a thumbnail is
  // never held whole.
  byte  sub_buf[4096];
  uint  sub_n;

  // Open a nested parse of nbytes of JPEG data; returns 0 if the depth limit
  // stops us, in which case the caller just discards the bytes as before.
  uint sub_begin( const char* what, uint nbytes );
  void sub_flush( void );
  void sub_end( void );

  void sub_feed( uint c ) {
    sub_buf[sub_n++] = byte(c);
    if( sub_n >= uint(DIM(sub_buf)) ) sub_flush();
  }

  // One thumbnail byte: to the nested parser if one is open, else discarded.
  void thumb_byte( uint c ) { if( sub_active ) sub_feed(c); }

  // Nesting depth: 0 for the file itself, 1 for a thumbnail inside it, and so
  // on.  Every line this parser prints is indented by it, so nested output is
  // visually contained in the segment that carried it.
  uint level;

  // Hooks for a caller that parses to locate rather than to report -- jpegdet
  // runs this parser speculatively at every candidate offset in a byte stream.
  // All three default to 0, which is exactly pjpg's own behaviour, and none of
  // them is touched by pjpg0_init(): the frontend sets them once, before the
  // parse, the way it sets level and sub.
  uint f_quiet;        // swallow every pf() line
  uint f_stop_at_eoi;  // end the parse the moment EOI has been consumed, so the
                       // input pointer marks the exact end of one image
  uint f_bail_on_err;  // end the parse at the first fatal error, rather than
                       // carrying on to report every problem in the file
  uint f_check_scans;  // validate scan headers and their Huffman tables even
                       // when the entropy data is not being decoded.  Neither
                       // check needs the data, and skipping the data is no
                       // reason to accept a scan a decoder would reject.
  uint f_stop_at_soi2; // end the parse where a second image begins.  An image
                       // whose EOI is missing does not end anywhere a parser
                       // can see -- it just runs on into whatever follows --
                       // so the next SOI is the only honest boundary, and
                       // stopping there is what lets both images be recovered
                       // instead of one file holding the two of them

  // Error state.  A parser that detects a malformed field records it here and
  // exits via CORO_EXIT; the outer loop in pjpg1.inc then yields PJPG_ERR.
  uint  err_code;
  const char* err_msg;
  uint  n_err;
  uint  n_warn;

  // Things a conforming decoder refuses outright but that pjpg only notes and
  // walks past.  pjpg's job is to describe what is in a file; a detector's job
  // is to decide whether the file is one, and for that the difference between
  // "unusual" and "libjpeg stops here" is the whole question.
  uint  n_nonconf;

  // Frame flags derived from the SOF marker code, set by do_process() before
  // pa_SOF() runs.  They are members rather than parameters so that a resumed
  // parser never depends on its arguments being re-supplied.
  uint sof_prog, sof_arith, sof_lossless, sof_diff, sof_hier;

  void pjpg0_init( void ) {
    bzero(c_tag);
    err_code = 0; err_msg = 0; n_err = 0; n_warn = 0; n_nonconf = 0;
    sof_prog = sof_arith = sof_lossless = sof_diff = sof_hier = 0;
    // A parser object is reused across parses by jpegdet, so nothing may
    // survive into the next one.  sub_active in particular: a parse abandoned
    // inside a thumbnail would otherwise leave it set, and the next parse's
    // thumb_byte() would feed a sub-parser that was never sub_begin()'d.
    sub_active = 0; sub_n = 0; n_thumb = 0;
    n_thumbs = 0; f_thumbs_lost = 0;
    seg_pos = 0; seg_payload = 0;
  }

  void coro2_init( void ) {
    state=0;
  }

  int coro2_process( void ) {
    do_process();
    return state!=0;
  }

  // A malformed field.  The segment's declared length still tells us where the
  // next marker starts, so parsing continues from there -- we abandon only this
  // segment, and report every problem in the file rather than the first.  The
  // run still ends with a PJPG_ERR yield and a non-zero exit status.
  void pjpg_err( uint code, const char* msg ) {
    n_err++;
    if( err_code==0 ) { err_code=code; err_msg=msg; }
    pf( "ERROR [tag %02X]: %s\n", tag_id, msg );
  }

  // Is what went wrong something that stops a decoder?  A malformed Exif block
  // is not: it sits inside an APP1 payload that every decoder skips whole, so
  // the image decodes regardless of what the TIFF structure inside it says.
  // pjpg reports it because describing the file is its job; a detector deciding
  // whether these bytes are a usable image must not hold it against them.
  uint fatal_for_decoding( void ) { return err_code && (err_code!=E_BADEXIF); }

  // Marker signatures are arbitrary file bytes; never send them to a terminal raw.
  byte pr( uint c ) { return ((c>=0x20) && (c<=0x7E)) ? byte(c) : byte('.'); }

  // Every parser writes through this rather than printf, so a nested level's
  // output is indented under the marker that contained it.
  #ifdef __GNUC__
  __attribute__((format(printf,2,3)))
  #endif
  void pf( const char* fmt, ... ) {
    uint k;
    if( f_quiet ) return;
    for( k=0; k<level; k++ ) printf( "    " );
    va_list ap; va_start( ap, fmt ); vprintf( fmt, ap ); va_end( ap );
  }

  void pjpg_warn( const char* msg ) {
    n_warn++;
    pf( "Warning [tag %02X]: %s\n", tag_id, msg );
  }

  #define coro2_get2E(f_LE,pos) ( (void)coro2_get2(), pos+=2, (f_LE?(B<<8)|A:(A<<8)|B) )
  #define coro2_get4E(f_LE,pos) ( (void)coro2_get4(), pos+=4, (f_LE?(D<<24)|(C<<16)|(B<<8)|A:(A<<24)|(B<<16)|(C<<8)|D) )

  #define coro2_getH() (byte(coro2_hl=coro2_get())>>4)
  #define coro2_getL() (byte(coro2_hl&0xF))

  // Leave a parser early.  A bare 'return' would skip CORO_END, leaving state
  // non-zero, so coro2_process() would report "wants more bytes" and the outer
  // loop would keep feeding the abandoned parser one byte at a time.
  #define CORO_EXIT goto coro_done

  // Bytes still unread in the current segment, as seen from inside a parser:
  // l_tag counts what is left AFTER the byte being processed right now.
  #define coro2_left() (l_tag)

  void do_process( void ) {

    switch( tag_id ) {
    case 0:      /*proc_MCU();*/ break;
    case M_DRI:  pa_DRI(); break;
    case M_DHT:  pa_DHT(); break;
    case M_DQT:  pa_DQT(); break;
    case M_DAC:  pa_DAC(); break;
    case M_DNL:  pa_DNL(); break;
    case M_EXP:  pa_EXP(); break;
    case M_COM:  pa_COM(); break;
    case M_SOS:  pa_SOS(); break;
    case M_APP0: pa_APP0(); break;
    case M_APP1: pa_APP1(); break;
    case M_APP2: case M_APP3: case M_APP4: case M_APP5:
    case M_APP6: case M_APP7: case M_APP8: case M_APP9:
    case M_APP10:case M_APP11:case M_APP12:case M_APP13:
    case M_APP14:case M_APP15: pa_APPn(); break;

    // Every frame header, plus DHP which shares the SOF syntax.  The process
    // flags follow directly from the marker code: bit 3 = arithmetic coding,
    // bit 2 = differential, low 2 bits = 0/1 sequential, 2 progressive,
    // 3 lossless.  DHT (C4), JPG (C8) and DAC (CC) are the holes in that range
    // and are dispatched above.
    case M_SOF0:  case M_SOF1:  case M_SOF2:  case M_SOF3:
    case M_SOF5:  case M_SOF6:  case M_SOF7:
    case M_SOF9:  case M_SOF10: case M_SOF11:
    case M_SOF13: case M_SOF14: case M_SOF15:
    case M_DHP:
      sof_hier     = (tag_id==M_DHP);
      sof_arith    = !sof_hier && ((tag_id&8)!=0);
      sof_diff     = !sof_hier && ((tag_id&4)!=0);
      sof_prog     = !sof_hier && ((tag_id&3)==2);
      sof_lossless = !sof_hier && ((tag_id&3)==3);
      pa_SOF();
      break;
    }

    c_tag[byte(tag_id)] += (state==0);
  }

  const char* sof_name( void ) {
    if( sof_hier ) return "Define Hierarchical Progression";
    if( sof_lossless ) return sof_diff ? "Differential lossless frame" : "Lossless frame";
    if( sof_prog )     return sof_diff ? "Differential progressive frame" : "Progressive frame";
    return sof_diff ? "Differential sequential frame" : "Sequential frame";
  }

  #include "tag_APP0.inc"
  #include "tag_APP1.inc"
  #include "tag_APPn.inc"
  #include "tag_DHT.inc"
  #include "tag_SOF.inc"
  #include "tag_DQT.inc"
  #include "tag_DAC.inc"
  #include "tag_SOS.inc"
  #include "tag_DRI.inc"
  #include "tag_misc.inc"

};
