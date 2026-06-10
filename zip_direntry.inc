
struct ZIPDIRENTRY {

  enum{ l_name_max = 1<<16, l_extra_max = 1<<16, l_comm_max = 1<<16 };

  uint sign;    //0x02014b50
  word versMade;
  word versExtr;
  word flags;
  word method;
  word FileTime; // DOSTIME
  word FileDate; // DOSDATE
  uint crc32;
  uint cSize;
  uint uSize;
  word l_name;
  word l_extra;
  word l_comment;

  word DiskNumberStart;
  word IntAttr;
  uint ExtAttr;
  uint HeaderOffs;

  byte name[l_name_max]; // frFileNameLength
  byte extra[l_extra_max]; // frExtraFieldLength
  byte comment[l_comm_max];

  // ZIP64 extended-info handling. In a central-directory entry the fields
  // present in id 0x0001 are, in order, those whose 32-bit slot holds the
  // overflow marker: uSize, cSize (0xFFFFFFFF), HeaderOffs (0xFFFFFFFF),
  // DiskStart (0xFFFF). We remember the byte positions of the cSize and
  // HeaderOffs fields so they can be patched in place.
  qword cSize64, uSize64, HeaderOffs64;
  int   cSize64_pos, offs64_pos;

  qword rd8( uint p ) {
    qword v = 0;
    for( uint k=0; k<8; k++ ) v |= qword(extra[p+k]) << (8*k);
    return v;
  }
  void wr8( uint p, qword v ) {
    for( uint k=0; k<8; k++ ) extra[p+k] = byte(v>>(8*k));
  }

  void parse_zip64( void ) {
    cSize64 = cSize; uSize64 = uSize; HeaderOffs64 = HeaderOffs;
    cSize64_pos = -1; offs64_pos = -1;
    uint i = 0;
    while( i+4 <= l_extra ) {
      uint id  = extra[i] | (extra[i+1]<<8);
      uint len = extra[i+2] | (extra[i+3]<<8);
      uint body = i+4;
      if( body+len > l_extra ) break;
      if( id == 0x0001 ) {
        uint p = body, e = body+len;
        if( uSize      == 0xFFFFFFFFu && p+8 <= e ) { uSize64 = rd8(p); p += 8; }
        if( cSize      == 0xFFFFFFFFu && p+8 <= e ) { cSize64 = rd8(p); cSize64_pos = p; p += 8; }
        if( HeaderOffs == 0xFFFFFFFFu && p+8 <= e ) { HeaderOffs64 = rd8(p); offs64_pos = p; p += 8; }
        break;
      }
      i = body + len;
    }
  }

  // Real (64-bit) local-header offset used as the lookup key into the offset
  // table, transparently whether it lives in the 32-bit slot or the extra.
  qword realOffs( void ) { return HeaderOffs64; }

  // Apply an output local-header offset, writing it to whichever slot holds it.
  void setOffs( qword v ) {
    HeaderOffs64 = v;
    if( offs64_pos>=0 ) wr8( offs64_pos, v ); else HeaderOffs = (uint)v;
  }

  // Apply a signed size delta (encryption header ±12) to cSize, in place.
  void addSize( int delta ) {
    cSize64 += delta;
    if( cSize64_pos>=0 ) wr8( cSize64_pos, cSize64 ); else cSize = (uint)cSize64;
  }

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=1 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',1,2>::n;
    versMade = z.get_Lword();
    versExtr = z.get_Lword();
    flags = z.get_Lword();
    method = z.get_Lword();
    FileTime = z.get_Lword();
    FileDate = z.get_Lword();
    crc32 = z.get_Luint();
    cSize = z.get_Luint();
    uSize = z.get_Luint();
    l_name = z.get_Lword();
    l_extra = z.get_Lword();
    l_comment = z.get_Lword();

    DiskNumberStart = z.get_Lword();
    IntAttr = z.get_Lword();
    ExtAttr = z.get_Luint();
    HeaderOffs = z.get_Luint();

    z.get_string( name,    sizeof(name),    l_name  );
    z.get_string( extra,   sizeof(extra),   l_extra );
    z.get_string( comment, sizeof(comment), l_comment );

    parse_zip64();

    return 11*2+6*4+l_name+l_extra+l_comment;
  };

  template< class zipdump >
  void Store( zipdump* _z ) {
    zipdump& z = *_z;
    z.put_Luint( sign );      
    z.put_Lword( versMade );  
    z.put_Lword( versExtr );  
    z.put_Lword( flags );     
    z.put_Lword( method );    
    z.put_Lword( FileTime );  
    z.put_Lword( FileDate );  
    z.put_Luint( crc32 );     
    z.put_Luint( cSize );     
    z.put_Luint( uSize );     
    z.put_Lword( l_name );    
    z.put_Lword( l_extra );   
    z.put_Lword( l_comment ); 
    z.put_Lword( DiskNumberStart );
    z.put_Lword( IntAttr );
    z.put_Luint( ExtAttr );
    z.put_Luint( HeaderOffs );
    z.put_string( name,  sizeof(name),  l_name  );
    z.put_string( extra, sizeof(extra), l_extra );
    z.put_string( comment, sizeof(comment), l_comment );
  };

  void Dump( void ) {
//    printf( "sign=%08X; ", sign );
    printf( " versMade=%i; ", versMade );
    printf( "versExtr=%i; ", versExtr );
    printf( "flags=%04X; ", flags );
    printf( "method=%04X; ", method );
    printf( "FileTime=%04X; ", FileTime );
    printf( "FileDate=%04X\n", FileDate );
    printf( " crc32=%08X; ", crc32 );
    printf( "cSize=%i; ", cSize );
    printf( "uSize=%i; ", uSize );
    printf( "l_name=%i; ", l_name );
    printf( "l_extra=%i; ", l_extra );
    printf( "l_comment=%i\n", l_comment );

    printf( " DiskNumberStart=%i; ", DiskNumberStart );
    printf( "IntAttr=%i; ", IntAttr );
    printf( "ExtAttr=%i; ", ExtAttr );
    printf( "HeaderOffs=%X\n", HeaderOffs );

    printf( " name[] = " ); typestr( name, l_name );
  }

};
