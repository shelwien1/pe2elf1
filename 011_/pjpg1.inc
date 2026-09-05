
// Codes yielded to the frontend.  CoroFileProc treats 1 as "give me input" and
// 2 as "flush output"; anything else ends the run.
enum { PJPG_QUIT=0, PJPG_ERR=3 };

struct pjpg : Coroutine, pjpg0 {

  byte f_lentag[256];
  byte f_ptag[256];

  // Absolute offset of the first byte of the block pin[0] is holding.  Whoever
  // hands this parser input advances it, so stream_base + (ptr-beg) is where
  // the next byte to be read lives -- the file frontend counts in the file, and
  // sub_flush() counts in the thumbnail it is feeding down.
  qword stream_base;

  void feed( byte* p, uint n ) {
    stream_base += qword( pin[0].ptr - pin[0].beg );
    addinp( p, n );
  }
  qword at( void ) { return stream_base + qword( pin[0].ptr - pin[0].beg ); }

  // A scan sink.  pjpg walks the markers and decodes the entropy data; a
  // transcoder has to do something at each scan -- note where it is in the
  // file, or write a re-encoded one out -- and the moment to do it is with this
  // parser's state still in hand: the scan's components, its spectral range,
  // its tables, its restart interval.  Called with phase 0 once the scan header
  // is parsed and before its data is touched, and phase 1 once the data is
  // done.  Null unless a frontend sets it, so pjpg itself is unaffected.
  void* sc_ctx;
  void (*sc_hook)( void* ctx, struct pjpg* P, uint phase );

  uint n_restart;
  uint n_scan;
  uint n_frame;        // frame headers in this image, DHP not counted
  uint f_hier_seen;    // a DHP introduced them, so more than one is legal
  uint n_soi;          // SOI markers seen.  More than one means a stream that
                       // ran two images together without an EOI between them,
                       // which a detector needs to know about: it will have
                       // parsed straight through the first into the second.
  uint f_entropy;      // decode entropy-coded scans (1) or skip them (0)

  #include "entropy.inc"

  void init_lentag( void ) {
    uint i;

    // T.81 B.1.1.3: every marker in C0..FE carries a 2-byte length, except the
    // standalone ones RST0..RST7 (D0..D7), SOI (D8) and EOI (D9).  TEM (01) and
    // the reserved range 02..BF are standalone too.
    //
    // This used to be a hand-written table that omitted 11 length-bearing
    // markers (SOF3/5/6/7/11/13/14/15, DNL, DHP, EXP).  Their payloads were then
    // scanned as data, so any FF xx inside one became a fabricated marker.
    bzero( f_lentag );
    for( i=0xC0; i<=0xFE; i++ ) f_lentag[i]=1;
    for( i=0xD0; i<=0xD9; i++ ) f_lentag[i]=0;

    bzero( f_ptag );
    for( i=0; parsedtags[i]; i++ ) f_ptag[parsedtags[i]]=1;
  }

  //--------------------------------------------------------------------------
  // Bulk input helpers.  Strictly sequential: they only ever advance the input
  // pointer, and they yield for a refill exactly the way get() does.  These are
  // what keep the parser fast -- entropy-coded data and unparsed segments are
  // the bulk of a JPEG and neither needs to be examined a byte at a time.
  //--------------------------------------------------------------------------

  // Bytes available right now without yielding.
  uint inp_avail( void ) { return uint( pin[0].end - pin[0].ptr ); }

  // Make at least one byte available; returns 0 at end of input.
  uint inp_fill( void ) {
    while( pin[0].ptr >= pin[0].end ) {
      if( pin[0].f_quit() ) return 0;
      pin[0].yield_r();
    }
    return inp_avail();
  }

  // Discard n bytes.  Returns the number actually discarded, which is less than
  // n only at end of input.
  uint skip( uint n ) {
    uint av,take,done;
    done = 0;
    while( done<n ) {
      av = inp_fill(); if( av==0 ) break;
      take = (n-done < av) ? (n-done) : av;
      pin[0].ptr += take;
      done += take;
    }
    return done;
  }

  // Advance to the next 0xFF and consume it.  Returns 0 at end of input.
  // n_skipped counts the bytes passed over: after a segment those are garbage
  // between it and the next marker, which is worth reporting.
  uint n_skipped;

  uint scan_FF( void ) {
    byte* p;
    uint av;
    n_skipped = 0;
    while(1) {
      av = inp_fill(); if( av==0 ) return 0;
      p = (byte*)memchr( pin[0].ptr, 0xFF, av );
      if( p ) { n_skipped += uint(p - pin[0].ptr); pin[0].ptr = p+1; return 1; }
      n_skipped += av;
      pin[0].ptr = pin[0].end;
    }
  }

  //--------------------------------------------------------------------------

  // T.81 4.5: a quantisation table has to be defined BEFORE the scan that
  // references it, and libjpeg latches the tables at each scan (jdinput.c
  // latch_quant_tables, JERR_NO_QUANT_TABLE).  Asking at the end of the file
  // instead accepts a stream whose DQT arrives after the scan that uses it --
  // which parses perfectly, decodes perfectly, and does not open.  The
  // Huffman side of the same rule is already enforced per scan, by
  // scan_tables_ready().  Lossless and hierarchical frames carry no
  // quantisation at all.
  void check_scan_quant( void ) {
    uint i,c,q;
    if( sof_lossless || sof_hier ) return;
    for( i=0; i<uint(comps_in_scan); i++ ) {
      c = cs_cmp[i];
      if( c>=uint(num_components) ) continue;
      q = uint(comp_info[c].quant_tbl_no);
      if( q>=uint(DIM(quant_tbl)) ) continue;      // pa_SOF rejects this already
      if( quant_tbl[q].f_undef ) {
        pjpg_warn( "scan uses a quantisation table no DQT has defined yet" );
        n_nonconf++;
      }
    }
  }

  // T.81 fixes the length of these segments exactly -- SOF is 8+3*Nf, SOS is
  // 6+2*Ns, DRI is 4, EXP is 3, DNL is 4 -- or, for DQT/DHT/DAC, makes it a
  // whole number of table definitions.  A byte left over when the parser is
  // done means the segment is malformed, and libjpeg stops on it ("Bogus
  // marker length").  APPn and COM payloads are the exception: they are opaque,
  // pjpg reads the signature it recognises and leaves the rest, so a leftover
  // there says nothing at all.
  uint exact_len_marker( uint m ) {
    if( (m>=M_APP0) && (m<=M_APP15) ) return 0;
    if( m==M_COM ) return 0;
    return f_ptag[m];
  }

  // C0..CF are frame headers apart from DHT (C4), JPG (C8) and DAC (CC); DHP
  // shares the SOF syntax and starts a hierarchical frame too.
  uint is_frame_marker( uint m ) {
    if( m==M_DHP ) return 1;
    if( (m<M_SOF0) || (m>M_SOF15) ) return 0;
    if( (m==M_DHT) || (m==M_JPG) || (m==M_DAC) ) return 0;
    return 1;
  }

  // Frame state that must not survive into the next image in a multi-image
  // file.  Nothing used to reset any of this at SOI, so a second image parsed
  // its scans against the first image's frame.
  void image_reset( void ) {
    uint k;
    num_components = 0;
    comps_in_scan  = 0;
    saw_SOF = 0;
    saw_SOI = 0;
    saw_JFIF_marker = 0;
    saw_Adobe_marker = 0;
    progressive_mode = 0;
    arith_code = 0;
    restart_interval = 0;
    eoi_reached = 0;
    input_scan_number = 0;
    n_frame = 0; f_hier_seen = 0;
    image_width = 0; image_height = 0;
    data_precision = 0;
    f_SOS = 0;
    bzero( comp_info );
    bzero( cur_comp_info );
    for( k=0; k<uint(DIM(quant_tbl)); k++ ) quant_tbl[k].f_undef = 1;
    bzero( dc_huff_tbl_ptrs );
    bzero( ac_huff_tbl_ptrs );
    bzero( dtbl_ok[0] ); bzero( dtbl_ok[1] );
    nz_free();
    co_free();
  }

  void do_process( void ) {
    uint c,r,lo,hi,pending,was_sos,was_sof,this_tag;
    uint f_expect_marker;

    init_lentag();
    pjpg0_init();
    image_reset();

    tag_id=0; l_tag=0; pending=0; f_expect_marker=0;
    n_restart=0; n_scan=0; n_scan_dec=0; n_scan_bad=0; n_soi=0;
    nz_free();
    coro2_init();

    while(1) {

      // A detector parses speculatively at every candidate offset in a stream,
      // so a wrong guess must cost as little as possible: give up at the first
      // fatal error instead of reporting every problem in what is not a JPEG.
      if( f_bail_on_err && fatal_for_decoding() ) goto at_eof;

      // The entropy decoder consumes the marker that ends a scan, so take that
      // one rather than scanning for the next FF.
      if( pending ) { c = pending; pending = 0; goto have_marker; }

      // --- find the next marker prefix -------------------------------------
      // f_expect_marker is set once a segment has been fully consumed: the very
      // next byte should be FF, so anything skipped before it is stray data.
      // In state "scanning" the only byte that matters is 0xFF, so look for it
      // with memchr over whole buffers rather than one loop iteration per byte.
      // This is the hot path: it covers all entropy-coded scan data.
      if( scan_FF()==0 ) break;
      if( f_expect_marker && n_skipped ) {
        pf( "Warning: %u extraneous byte(s) before the next marker\n", n_skipped );
        n_warn++;
      }
      f_expect_marker = 0;

      // A run of 0xFF bytes is legal padding before a marker.
      do {
        c = get(); if( c==uint(-1) ) goto at_eof;
      } while( c==0xFF );

    have_marker:
      if( c==0x00 ) continue;             // FF 00 -- a stuffed data byte

      if( (c&0xF8)==M_RST0 ) {            // RST0..RST7: no payload, not printed
        n_restart++;
        continue;
      }

      tag_id = c;
      // The FF and the marker byte have both been read, so the segment starts
      // two back.  A run of fill FFs before it leaves the extra ones outside,
      // which is what a caller cutting the segment out wants anyway.
      seg_pos = at() - 2;
      pf( "!tag=%02X!\n", tag_id );

      // T.81 B.1.1.3 reserves 02..BF (RES), C8 (JPG) and F0..FD (JPG0..JPG13)
      // for extensions that were never defined.  Between segments a conforming
      // stream carries none of them, and libjpeg gives up on whichever it meets
      // ("Unsupported marker type 0x..", or for C8 "Unsupported JPEG process:
      // SOF type 0xc8").  TEM (01) is legal and handled just below.
      if( ((c>=0x02) && (c<=0xBF)) || (c==M_JPG) || ((c>=M_JPG0) && (c<=M_JPG13)) ) n_nonconf++;

      if( c==M_TEM ) continue;            // TEM has no payload
      if( c==M_SOI ) {
        if( saw_SOI && !eoi_reached ) {
          pjpg_warn( "second SOI before the previous image ended" );
          // Leave before image_reset(), so what the first image established --
          // its frame, its scans, its errors -- is still there to be read.  The
          // input pointer is two bytes past the start of this SOI (the marker
          // byte, and the FF that introduced it), which is where the first
          // image has to be cut.
          if( f_stop_at_soi2 ) { n_soi++; goto fin; }
        }
        image_reset(); saw_SOI = 1; n_soi++; continue;
      }
      // Stopping straight at fin leaves the input pointer just past the D9, which
      // is what makes the end of an image an exact byte offset for a detector.
      // The FF and the D9 have both been consumed on every path that gets here:
      // by the get() under scan_FF(), or by the entropy decoder's read-ahead,
      // which parks the marker it ran into in e_marker rather than pushing it
      // back (see fill_bit_buffer/next_marker_now in entropy.inc).
      if( c==M_EOI ) { eoi_reached = 1; if( f_stop_at_eoi ) goto fin; continue; }

      if( f_lentag[c]==0 ) continue;      // standalone or reserved marker

      // --- segment length --------------------------------------------------
      hi = get(); if( hi==uint(-1) ) goto truncated;
      lo = get(); if( lo==uint(-1) ) goto truncated;
      l_tag = (hi<<8) | lo;

      if( l_tag<2 ) {
        pjpg_warn( "segment length below the 2 bytes of the length field itself" );
        n_nonconf++;                       // libjpeg: "Bogus marker length"
        l_tag = 0; tag_id = 0;
        continue;
      }
      l_tag -= 2;                          // payload size
      seg_payload = l_tag;

      // --- payload ---------------------------------------------------------
      was_sos  = (tag_id==M_SOS);
      was_sof  = is_frame_marker(tag_id);
      this_tag = tag_id;                   // the parser below clears tag_id
      if( f_ptag[tag_id] ) {

        if( tag_id==M_SOS ) n_scan++;

        coro2_init();
        r = coro2_process();               // run the parser up to its first read

        // Feed the parser one byte at a time for as long as it wants more.
        // l_tag is the parser's view of how much of the segment is left, and it
        // is decremented before the byte is handed over, so inside a parser
        // l_tag means "bytes remaining after this one".
        while( r && (l_tag>0) ) {
          c = get(); if( c==uint(-1) ) goto truncated;
          l_tag--;
          coro2_c = c;
          r = coro2_process();
          if( r==0 ) break;                // parser finished, or gave up on a bad field
        }

        if( pjpg0::state!=0 ) {
          // The parser still wanted data when the segment ended.  Every parser
          // bounds itself against l_tag, so this means the segment really was
          // shorter than its contents claimed.
          pjpg_warn( "segment ended while its parser was still reading" );
          coro2_init();
        }

        tag_id = 0;
      }

      // Whatever the parser did not consume -- trailing bytes, or the whole
      // payload of a marker we do not parse -- is skipped in bulk.
      if( l_tag>0 ) {
        if( exact_len_marker(this_tag) ) n_nonconf++;
        if( skip(l_tag)!=l_tag ) { l_tag=0; goto truncated; }
        l_tag = 0;
      }

      f_expect_marker = 1;

      // Entropy-coded data follows a scan header.  Decode it when we can;
      // otherwise fall through and let scan_FF skip it as before.
      // A new frame header means new geometry, so the coefficient map sized for
      // the previous frame is wrong.  A hierarchical stream has several frames
      // between one SOI and EOI, and image_reset() only runs at SOI, so without
      // this the second frame's refinement scans decode against a map sized for
      // the first -- silently, because nz_get/nz_set bounds-check and return 0.
      if( was_sof ) {
        nz_free();
        if( this_tag==M_DHP ) f_hier_seen = 1;
        else {
          n_frame++;
          // T.81 B.2: a non-hierarchical stream carries exactly one frame.  A
          // second frame header with no DHP to introduce it is what libjpeg
          // calls "Invalid JPEG file structure: two SOF markers", and it stops.
          if( (n_frame>1) && !f_hier_seen ) n_nonconf++;
        }
      }

      if( was_sos ) f_expect_marker = 0;
      // Before anything that depends on the tables, and in every mode: this
      // costs one pass over at most four components and it is the difference
      // between carving a file that opens and one that does not.
      if( was_sos && f_SOS ) check_scan_quant();


      if( was_sos && f_SOS && f_entropy ) {
        if( scan_decodable() ) {
          // After scan_decodable(), because that is what runs per_scan_setup():
          // a sink that means to encode this scan needs its MCU geometry, and
          // one that means to note where it is needs the input pointer still
          // sitting on its first entropy byte.  Both hold here.
          if( sc_hook ) sc_hook( sc_ctx, this, 0 );
          pending = decode_scan();
          if( sc_hook ) sc_hook( sc_ctx, this, 1 );
          if( pending==0 ) goto at_eof;
        } else if( arith_code && sof_lossless ) {
          pjpg_warn( "lossless arithmetic scan, entropy data not decoded" );
        } else if( sof_lossless ) {
          pjpg_warn( "lossless scan, entropy data not decoded" );
        }
      } else if( was_sos && f_SOS && f_check_scans ) {
        // Same header checks, none of the cost: scan_decodable() validates the
        // scan parameters against the frame and scan_tables_ready() builds the
        // decoding tables, which is where a Huffman table that does not form a
        // prefix code is caught ("Bogus Huffman table definition").  Neither
        // reads a byte of entropy data.
        if( scan_decodable() )
          if( !scan_tables_ready() ) pjpg_err( E_BADDHT, "a Huffman table this scan uses is missing or bogus" );
      }
    }

  at_eof:
    // libjpeg refuses a file that does not begin with SOI ("Not a JPEG file").
    // pjpg parses whatever it finds, but a stream with no SOI at all is worth
    // saying out loud.
    if( !saw_SOI ) pjpg_warn( "no SOI marker anywhere in the input" );
    if( saw_SOF && !eoi_reached ) pjpg_warn( "input ended before EOI" );
    goto fin;

  truncated:
    pjpg_err( E_TRUNCATED, "input ended inside a segment" );
    goto fin;

  fin:
    if( err_code ) {
      pf( "\nparse finished with %u error(s) and %u warning(s); first error: %s (%s)\n",
              n_err, n_warn, err_msg, jpeg_err_name(err_code) );
    } else if( n_warn ) {
      pf( "\nparse completed with %u warning(s)\n", n_warn );
    }

    // do_process must never return -- the frontend's stack frame is gone.
    while(1) yield( this, err_code ? PJPG_ERR : PJPG_QUIT );
  }

};

//-----------------------------------------------------------------------------
// Recursive thumbnail parsing.
//
// A JPEG can carry a JPEG thumbnail in its Exif APP1 or JFXX APP0 segment, and
// that thumbnail can carry one of its own.  Rather than special-casing it, the
// thumbnail is handed to the next level's parser, which is a plain pjpg: it
// does its own marker scanning, its own bounds checking and its own error
// reporting, and can recurse again in turn.
//
// The bytes arrive one at a time because that is how a marker parser is fed, so
// each one is pushed through the sub-parser's own input pin.  The level above
// plays exactly the role CoroFileProc plays for level 0: hand over input, run
// the coroutine until it asks for more, repeat, then signal end of input.
//-----------------------------------------------------------------------------

uint pjpg0::sub_begin( const char* what, uint nbytes ) {

  // Record the span before the depth check, not after: a caller that means to
  // lift the thumbnail out wants to know it is there whether or not this parser
  // has a level left to look inside it with.  ok stays 0 until something has
  // actually read it as a JPEG -- either sub_end below, or the caller itself.
  if( f_thumb_spans ) {
    if( n_thumbs >= uint(MAX_THUMBS) ) f_thumbs_lost = 1;
    else {
      thumb_span& t = thumbs[n_thumbs];
      t.pos     = tag_at();              // the next byte the caller will read
      t.len     = nbytes;
      t.seg     = seg_pos;
      t.seg_end = seg_pos + 4 + seg_payload;
      t.fixup   = 0;
      t.carrier = tag_id;
      t.ok      = 0;
      n_thumbs++;
    }
  }

  if( sub==0 ) { pjpg_warn( "thumbnail nesting limit reached, not parsing it" ); return 0; }
  n_thumb++;
  pf( ">>> %s: %u bytes, parsing as JPEG at level %u\n", what, nbytes, level+1 );
  sub->coro_init();
  sub->level = level+1;
  sub->stream_base = 0;                  // this level counts inside the thumbnail
  sub->f_thumb_spans = f_thumb_spans;    // and records its own, if we record ours
  sub_active = 1;
  sub_n = 0;
  return 1;
}

// Hand the buffered bytes over and let that level run until it has consumed
// them and asks for more.  Anything other than "give me input" means the level
// is done with the stream -- it reached EOI, or it failed -- so stop driving it.
// The caller keeps consuming the remaining thumbnail bytes either way, so the
// outer segment stays correctly aligned.
void pjpg0::sub_flush( void ) {
  if( sub_n==0 ) return;
  if( sub_active ) {
    sub->feed( sub_buf, sub_n );
    if( sub->coro_call( sub )!=1 ) sub_active = 0;
  }
  sub_n = 0;
}

void pjpg0::sub_end( void ) {
  uint guard;
  if( sub==0 ) return;
  sub_flush();
  if( sub_active ) {
    sub->f_quit = 1;                       // no more thumbnail bytes are coming
    for( guard=0; guard<4; guard++ ) {
      sub->feed( sub_buf, 0 );
      if( sub->coro_call( sub )!=1 ) break;
    }
    sub_active = 0;
  }
  // A thumbnail whose compression tag says JPEG but which contains no JPEG at
  // all is a malformed Exif block, not just an empty parse.
  if( sub->saw_SOI==0 )  pjpg_warn( "thumbnail is declared as JPEG but has no SOI marker" );
  else if( sub->err_code ) pjpg_warn( "embedded thumbnail did not parse cleanly" );

  // This level has now read the thumbnail as a JPEG, so it can say whether it
  // stands up on its own -- which is what decides whether it can be lifted out
  // and still open.
  if( f_thumb_spans && n_thumbs && (n_thumbs <= uint(MAX_THUMBS)) )
    thumbs[n_thumbs-1].ok = sub->saw_SOI && sub->eoi_reached && (sub->err_code==0);

  pf( "<<< end of thumbnail (level %u)\n", level+1 );
}
