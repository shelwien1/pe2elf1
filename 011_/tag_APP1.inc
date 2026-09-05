
//byte d[16];
//uint i;
uint f_LE,pos,len;
uint tagMark,ofs1st, n_entries,tagnum,format,d_elem,d_ofs,nextofs;
uint thumbOfs,thumbLen,thumbTyp;
qword ofsTagAt,lenTagAt;
uint max_entries;
uint f_thumbjpg;

void pa_APP1( void ) { CORO_START;

  // These used to be in-class initialisers, so they were set once at program
  // start and never cleared.  A second Exif APP1 with no thumbnail tags then
  // reported -- and acted on -- the first marker's thumbnail offset and length.
  thumbOfs = 0; thumbLen = 0; thumbTyp = 10;
  ofsTagAt = 0; lenTagAt = 0;

  bzero( d );

  if( l_tag<2 ) { pjpg_warn( "APP1 segment too short for a signature" ); CORO_EXIT; }

  d[0] = coro2_get();
  d[1] = coro2_get();
  d[2] = ' ';
  d[3] = ' ';

  if( (d[0]=='E') && (d[1]=='x') ) {
    if( l_tag<4 ) { pjpg_warn( "truncated Exif signature" ); CORO_EXIT; }
    d[2] = coro2_get();
    d[3] = coro2_get();
    d[4] = coro2_get();
    d[5] = coro2_get();
  }

  pf( "APP1: %c%c%c%c\n", pr(d[0]),pr(d[1]),pr(d[2]),pr(d[3]) );

  if( (d[2]=='i') && (d[3]=='f') && (d[4]==0) && (d[5]==0) ) {

    pos = 0; len = l_tag;

    if( len<8 ) { pjpg_err( E_BADEXIF, "Exif block shorter than a TIFF header" ); CORO_EXIT; }

    // The TIFF byte-order mark, read as two plain bytes.  The original stored a
    // big-endian-assembled word through a (word&) cast over d[], which put the
    // SECOND byte in d[6] on a little-endian host -- so the printed align field
    // was reversed and f_LE was decided by the wrong byte.  It was also a
    // strict-aliasing violation and host-endianness dependent.
    d[6] = coro2_get();
    d[7] = coro2_get();
    pos += 2;

    pf( "align: %c%c\n", pr(d[6]),pr(d[7]) );
    f_LE = (d[6]=='I'); // 'I'=little endian, 'M'=big endian

    if( !((d[6]=='I') && (d[7]=='I')) && !((d[6]=='M') && (d[7]=='M')) ) {
      pjpg_warn( "TIFF byte-order mark is neither II nor MM" );
    }

    tagMark = coro2_get2E(f_LE,pos);
    ofs1st  = coro2_get4E(f_LE,pos);

    pf( "tagMark=%X ofs1st=%X pos=%X len=%X\n", tagMark,ofs1st,pos,len );

    if( tagMark!=0x2A ) { pjpg_err( E_BADEXIF, "TIFF magic is not 42" ); CORO_EXIT; }

    // Walk the IFD chain.  Every bound below is written so it cannot wrap: the
    // offsets come straight from the file.
    while( (ofs1st>=pos) && (ofs1st<len) ) {

      for(; pos<ofs1st; pos++ ) {
        (void)coro2_get();
      }

      if( len-pos < 2 ) { pjpg_warn( "IFD entry count runs past the Exif block" ); break; }
      n_entries = coro2_get2E(f_LE,pos);
      pf( "n_entries=%i\n", n_entries );

      // Each directory entry is 12 bytes and the chain pointer another 4.
      max_entries = (len-pos)/12;
      if( n_entries > max_entries ) {
        pjpg_warn( "IFD declares more entries than the Exif block can hold" );
        n_entries = max_entries;
      }

      for( i=0; i<n_entries; i++ ) {
        tagnum = coro2_get2E(f_LE,pos);
        format = coro2_get2E(f_LE,pos);
        d_elem = coro2_get4E(f_LE,pos);
        d_ofs  = coro2_get4E(f_LE,pos);
        pf( "%04X/%04X: %04X %04X %04X %04X\n", pos,len, tagnum,format,d_elem,d_ofs );
        switch( tagnum ) {
        case 0x0103: thumbTyp = f_LE ? word(d_ofs) : d_ofs>>16; break;
        case 0x0111: case 0x0201: thumbOfs = d_ofs; ofsTagAt = tag_at()-4; break;
        // Where this tag's 4-byte value field is, so a caller lifting the
        // thumbnail out can set the length to zero rather than leave an IFD
        // pointing at bytes that are no longer there.  coro2_get4E has just
        // read it, so it ends where the parser now stands.
        case 0x0117: case 0x0202: thumbLen = d_ofs; lenTagAt = tag_at()-4; break;
        }
      }

      if( len-pos < 4 ) { pjpg_warn( "IFD chain pointer runs past the Exif block" ); break; }
      nextofs  = coro2_get4E(f_LE,pos);
      pf( "nextofs=%X; pos=%04X/%04X\n",  nextofs, pos,len );

      if( (nextofs!=0) && (nextofs<pos) ) { pjpg_warn( "IFD chain points backwards" ); break; }

      ofs1st=nextofs;
    }

    pf( "thumbOfs=%X thumbLen=%X thumbTyp=%i; pos=%04X/%04X\n",  thumbOfs,thumbLen,thumbTyp, pos,len );

    // typ=6: jpeg; typ=1: uncompressed; others unknown?
    // thumbOfs+thumbLen would wrap 32 bits, so a huge offset with a small
    // length used to pass the bound it should have failed.
    if( (thumbLen>0) && (thumbOfs>=pos) && (thumbOfs<=len) && (thumbLen<=len-thumbOfs) ) {

      for(; pos<thumbOfs; pos++ ) {
        (void)coro2_get();
      }

      // Compression tag 0x0103 == 6 means the thumbnail is a JPEG, so hand it
      // to the next level instead of discarding it.  Type 1 is uncompressed
      // pixels and anything else is unknown; both are skipped as before.
      f_thumbjpg = (thumbTyp==6) ? sub_begin( "Exif thumbnail", thumbLen ) : 0;
      // sub_begin has already filed the span, so the tag whose value has to be
      // zeroed for the Exif to still make sense without a thumbnail goes on it.
      if( (thumbTyp==6) && f_thumb_spans && n_thumbs && (n_thumbs <= uint(MAX_THUMBS)) )
        thumbs[n_thumbs-1].fixup = lenTagAt;

      for( i=0; i<thumbLen; i++,pos++ ) {
        thumb_byte( coro2_get() );
      }

      if( f_thumbjpg ) sub_end();
    }

    pf( "exif_done; pos=%04X/%04X\n",  pos,len );
  }

  coro_done: CORO_END;
}
