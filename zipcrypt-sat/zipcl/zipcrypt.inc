
struct zipcrypt {

  uint k0,k1,k2;

  byte pad[12];

  uint CRCTab[256];

  void InitCRC( void ) {
    uint i, j, c;
    for( i=0; i<256; i++ ) {
      for( c=i,j=0; j<8; j++ ) c=(c&1) ? (c>>1)^0xEDB88320L : (c>>1);
      CRCTab[i^0xFF] = c ^ 0xFF000000;
    }
  }

  uint CRC32Update( uint x, byte c ) { return CRCTab[byte(x)^c] ^ (x>>8); }

  uint CRC32( uint c, uint x ) {
    return ~CRC32Update(~c,x);
  }

  int update_keys( int c ) {
    k0 = CRC32( k0, c );
    k1 = (k1 + (k0 & 0xff)) * 134775813L + 1;
    k2 = CRC32( k2, k1>>24 );
    return c;
  }

  void init_keys( char* p ) {
    k0 = 305419896L;
    k1 = 591751049L;
    k2 = 878082192L;
    while( *p ) update_keys( *p++ );
  }

  uint decrypt_byte( void ) {
    uint temp,r;

    temp = (k2 & 0xFFFF) | 2;
    temp *= (temp^1);

    r = byte( temp >> 8 );

//    printf( "!%02X!\n", r );

    return r;
  }

  void crypt( byte* p, uint l, uint mode=1 ) {
    uint i,c;
    for( i=0; i<l; i++ ) {
      c = p[i] ^ decrypt_byte();
      update_keys( mode ? c : p[i] );
      p[i] = c;
    }
  }

};
