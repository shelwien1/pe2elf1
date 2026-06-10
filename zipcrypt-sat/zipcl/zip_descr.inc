
// Defintes the Data descriptor. For ZIP64 archives the compressed and
// uncompressed sizes are 8 bytes each (APPNOTE 4.3.9); zip64=1 selects that.
struct ZIPDATADESCR {
  uint  sign; //0x08074b50
  uint  crc32;
  qword cSize;
  qword uSize;
  uint  zip64;

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=0, uint z64=0 ) {
    zipdump& z = *_z;
    zip64 = z64;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',7,8>::n;
    crc32 = z.get_Luint();
    cSize = z64 ? z.get_Lqword() : z.get_Luint();
    uSize = z64 ? z.get_Lqword() : z.get_Luint();
    return 4 + 4 + (z64 ? 16 : 8);
  };

  template< class zipdump >
  void Store( zipdump* _z, uint flag=0 ) {
    zipdump& z = *_z;
//    if( flag )
    z.put_Luint( sign );
    z.put_Luint( crc32 );
    if( zip64 ) { z.put_Lqword( cSize ); z.put_Lqword( uSize ); }
    else        { z.put_Luint( (uint)cSize ); z.put_Luint( (uint)uSize ); }
  };

  void Dump( void ) {
//    printf( "sign=%08X; ", sign );
    printf( " crc32=%08X; ", crc32 );
    printf( "cSize=%llu; ", (unsigned long long)cSize );
    printf( "uSize=%llu; \n", (unsigned long long)uSize );
  }

};
