
// Codes yielded to the frontend.  CoroFileProc treats 1 as "give me input" and
// 2 as "flush output"; anything else ends the run.
enum { PJPG_QUIT=0, PJPG_ERR=3 };

struct pjpg : Coroutine, pjpg0 {

  byte f_lentag[256];
  byte f_ptag[256];

  uint n_restart;
  uint n_scan;

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
  uint scan_FF( void ) {
    byte* p;
    uint av;
    while(1) {
      av = inp_fill(); if( av==0 ) return 0;
      p = (byte*)memchr( pin[0].ptr, 0xFF, av );
      if( p ) { pin[0].ptr = p+1; return 1; }
      pin[0].ptr = pin[0].end;
    }
  }

  //--------------------------------------------------------------------------

  // Frame state that must not survive into the next image in a multi-image
  // file.  Nothing used to reset any of this at SOI, so a second image parsed
  // its scans against the first image's frame.
  void image_reset( void ) {
    uint k;
    num_components = 0;
    comps_in_scan  = 0;
    saw_SOF = 0;
    saw_JFIF_marker = 0;
    saw_Adobe_marker = 0;
    progressive_mode = 0;
    arith_code = 0;
    restart_interval = 0;
    eoi_reached = 0;
    input_scan_number = 0;
    image_width = 0; image_height = 0;
    data_precision = 0;
    f_SOS = 0;
    bzero( comp_info );
    bzero( cur_comp_info );
    for( k=0; k<uint(DIM(quant_tbl)); k++ ) quant_tbl[k].f_undef = 1;
    bzero( dc_huff_tbl_ptrs );
    bzero( ac_huff_tbl_ptrs );
  }

  void do_process( void ) {
    uint c,r,lo,hi;

    init_lentag();
    pjpg0_init();
    image_reset();

    tag_id=0; l_tag=0;
    n_restart=0; n_scan=0;
    coro2_init();

    while(1) {

      // --- find the next marker prefix -------------------------------------
      // In state "scanning" the only byte that matters is 0xFF, so look for it
      // with memchr over whole buffers rather than one loop iteration per byte.
      // This is the hot path: it covers all entropy-coded scan data.
      if( scan_FF()==0 ) break;

      // A run of 0xFF bytes is legal padding before a marker.
      do {
        c = get(); if( c==uint(-1) ) goto at_eof;
      } while( c==0xFF );

      if( c==0x00 ) continue;             // FF 00 -- a stuffed data byte

      if( (c&0xF8)==M_RST0 ) {            // RST0..RST7: no payload, not printed
        n_restart++;
        continue;
      }

      tag_id = c;
      printf( "!tag=%02X!\n", tag_id );

      if( c==M_TEM ) continue;            // TEM has no payload
      if( c==M_SOI ) { image_reset(); continue; }
      if( c==M_EOI ) { eoi_reached = 1; continue; }

      if( f_lentag[c]==0 ) continue;      // standalone or reserved marker

      // --- segment length --------------------------------------------------
      hi = get(); if( hi==uint(-1) ) goto truncated;
      lo = get(); if( lo==uint(-1) ) goto truncated;
      l_tag = (hi<<8) | lo;

      if( l_tag<2 ) {
        pjpg_warn( "segment length below the 2 bytes of the length field itself" );
        l_tag = 0; tag_id = 0;
        continue;
      }
      l_tag -= 2;                          // payload size

      // --- payload ---------------------------------------------------------
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
        if( skip(l_tag)!=l_tag ) { l_tag=0; goto truncated; }
        l_tag = 0;
      }
    }

  at_eof:
    if( saw_SOF && !eoi_reached ) pjpg_warn( "input ended before EOI" );
    goto fin;

  truncated:
    pjpg_err( E_TRUNCATED, "input ended inside a segment" );
    goto fin;

  fin:
    if( err_code ) {
      printf( "\nparse finished with %u error(s) and %u warning(s); first error: %s (%s)\n",
              n_err, n_warn, err_msg, jpeg_err_name(err_code) );
    } else if( n_warn ) {
      printf( "\nparse completed with %u warning(s)\n", n_warn );
    }

    // do_process must never return -- the frontend's stack frame is gone.
    while(1) yield( this, err_code ? PJPG_ERR : PJPG_QUIT );
  }

};
