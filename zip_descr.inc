
// Defintes the Data descriptor
struct ZIPDATADESCR {
  uint sign; //0x08074b50
  uint crc32;
  uint cSize;
  uint uSize;

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=0 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',7,8>::n;
    crc32 = z.get_Luint();
    cSize = z.get_Luint();
    uSize = z.get_Luint();
    return 4*4;
  };

  template< class zipdump >
  void Store( zipdump* _z, uint flag=0 ) {
    zipdump& z = *_z;
//    if( flag ) 
    z.put_Luint( sign );
    z.put_Luint( crc32 );
    z.put_Luint( cSize );
    z.put_Luint( uSize );
  };

  void Dump( void ) {
//    printf( "sign=%08X; ", sign );
    printf( " crc32=%08X; ", crc32 );
    printf( "cSize=%i; ", cSize );
    printf( "uSize=%i; \n", uSize );
  }

};
