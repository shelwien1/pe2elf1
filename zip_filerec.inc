
struct ZIPFILERECORD {

  enum{ l_name_max = 1<<16, l_extra_max = 1<<16 };

  uint sign;    //0x04034b50
  word vers;
  word flags;
  word method;
  word FileTime; // DOSTIME
  word FileDate; // DOSDATE
  uint crc32;
  uint cSize;
  uint uSize;
  word l_name;
  word l_extra;
  byte name[l_name_max]; // frFileNameLength
  byte extra[l_extra_max]; // frExtraFieldLength

  // Effective 64-bit sizes (filled from the ZIP64 extended-info extra field
  // when cSize/uSize are 0xFFFFFFFF, otherwise just copies of cSize/uSize).
  qword cSize64;
  qword uSize64;
  int   cSize64_pos; // byte offset of the 8-byte cSize inside extra[], or -1

  qword rd8( uint p ) {
    qword v = 0;
    for( uint k=0; k<8; k++ ) v |= qword(extra[p+k]) << (8*k);
    return v;
  }
  void wr8( uint p, qword v ) {
    for( uint k=0; k<8; k++ ) extra[p+k] = byte(v>>(8*k));
  }

  // Locate the ZIP64 extended-info field (header id 0x0001) inside extra[]
  // and lift the real 64-bit sizes out of it. A field is only present in the
  // record when its 32-bit slot holds the 0xFFFFFFFF overflow marker; they
  // appear in the fixed order uSize, cSize.
  void parse_zip64( void ) {
    cSize64 = cSize;
    uSize64 = uSize;
    cSize64_pos = -1;
    uint i = 0;
    while( i+4 <= l_extra ) {
      uint id  = extra[i] | (extra[i+1]<<8);
      uint len = extra[i+2] | (extra[i+3]<<8);
      uint body = i+4;
      if( body+len > l_extra ) break;
      if( id == 0x0001 ) {
        uint p = body;
        if( uSize == 0xFFFFFFFFu && p+8 <= body+len ) { uSize64 = rd8(p); p += 8; }
        if( cSize == 0xFFFFFFFFu && p+8 <= body+len ) { cSize64 = rd8(p); cSize64_pos = p; p += 8; }
        break;
      }
      i = body + len;
    }
  }

  // Write cSize64 back into the extra field when the size is stored there
  // (force_zip64 entry); otherwise keep the 32-bit cSize slot in sync.
  void sync_zip64( void ) {
    if( cSize64_pos>=0 ) wr8( cSize64_pos, cSize64 );
    else cSize = (uint)cSize64;
  }

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=1 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',3,4>::n;
    vers = z.get_Lword();
    flags = z.get_Lword();
    method = z.get_Lword();
    FileTime = z.get_Lword();
    FileDate = z.get_Lword();
    crc32 = z.get_Luint();
    cSize = z.get_Luint();
    uSize = z.get_Luint();
    l_name = z.get_Lword();
    l_extra = z.get_Lword();
    z.get_string( name,  sizeof(name),  l_name  );
    z.get_string( extra, sizeof(extra), l_extra );
//    z.skip_string( cSize );
    parse_zip64();
    return 7*2+4*4+l_name+l_extra;
  };

  template< class zipdump >
  void Store( zipdump* _z ) {
    zipdump& z = *_z;
    z.put_Luint( sign );
    z.put_Lword( vers );
    z.put_Lword( flags );
    z.put_Lword( method );
    z.put_Lword( FileTime );
    z.put_Lword( FileDate );
    z.put_Luint( crc32 );
    z.put_Luint( cSize );
    z.put_Luint( uSize );
    z.put_Lword( l_name );
    z.put_Lword( l_extra );
    z.put_string( name,  sizeof(name),  l_name  );
    z.put_string( extra, sizeof(extra), l_extra );
  };

  void Dump( void ) {
//    printf( "sign=%08X; ", sign );
    printf( " vers=%i; ", vers );
    printf( "flags=%04X; ", flags );
    printf( "method=%04X; ", method );
    printf( "FileTime=%04X; ", FileTime );
    printf( "FileDate=%04X\n", FileDate );
    printf( " crc32=%08X; ", crc32 );
    printf( "cSize=%i; ", cSize );
    printf( "uSize=%i; ", uSize );
    printf( "l_name=%i; ", l_name );
    printf( "l_extra=%i\n", l_extra );
//    printf( "IVData[%i] = ", IVSize ); typehex( IVData, IVSize );
    printf( " name[] = " ); typestr( name, l_name );
  }

};
