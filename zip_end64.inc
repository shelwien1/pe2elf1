
// ZIP64 End Of Central Directory record (signature 0x06064b50) and the
// ZIP64 End Of Central Directory locator (signature 0x07064b50). These appear
// just before the regular End Of Central Directory record (PK\5\6) whenever an
// archive needs ZIP64 (entry count, central-dir size or central-dir offset
// overflowing their 32-bit slots). They must be copied through, with the
// central-directory / record offsets patched the same way the 32-bit EOCD's
// DirOffs is, so the output stays a valid ZIP64 stream.

struct ZIP64ENDLOCATOR {

  enum{ l_data_max = 1<<16 };

  uint  sign;        // 0x06064b50
  qword recsize;     // size of this record counting from just after this field
  word  versMade;
  word  versExtr;
  uint  DiskNumber;
  uint  StartDiskNumber;
  qword EntriesOnDisk;
  qword EntriesInDirectory;
  qword DirSize;
  qword DirOffs;
  uint  l_data;            // length of the extensible data sector (recsize-44)
  byte  data[l_data_max];  // zip64 extensible data sector

  enum{ fixed = 2*2 + 2*4 + 4*8 }; // 44: fields after the recsize field

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=1 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',6,6>::n;
    recsize           = z.get_Lqword();
    versMade          = z.get_Lword();
    versExtr          = z.get_Lword();
    DiskNumber        = z.get_Luint();
    StartDiskNumber   = z.get_Luint();
    EntriesOnDisk     = z.get_Lqword();
    EntriesInDirectory= z.get_Lqword();
    DirSize           = z.get_Lqword();
    DirOffs           = z.get_Lqword();
    l_data = (recsize>=fixed) ? uint(recsize-fixed) : 0;
    z.get_string( data, sizeof(data), l_data );
    return 4 + 8 + uint(recsize); // sign + recsize field + record body
  };

  template< class zipdump >
  void Store( zipdump* _z ) {
    zipdump& z = *_z;
    z.put_Luint( sign );
    z.put_Lqword( recsize );
    z.put_Lword( versMade );
    z.put_Lword( versExtr );
    z.put_Luint( DiskNumber );
    z.put_Luint( StartDiskNumber );
    z.put_Lqword( EntriesOnDisk );
    z.put_Lqword( EntriesInDirectory );
    z.put_Lqword( DirSize );
    z.put_Lqword( DirOffs );
    z.put_string( data, sizeof(data), l_data );
  };

  void Dump( void ) {
    printf( " EntriesInDirectory=%llu; ", (unsigned long long)EntriesInDirectory );
    printf( "DirSize=%llu; ", (unsigned long long)DirSize );
    printf( "DirOffs=%llX\n", (unsigned long long)DirOffs );
  }

};

struct ZIP64LOCATOR {

  uint  sign;          // 0x07064b50
  uint  DiskNumber;    // disk holding the start of the zip64 EOCD record
  qword Z64EOCDOffs;   // offset of the zip64 EOCD record
  uint  TotalDisks;

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=1 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',6,7>::n;
    DiskNumber  = z.get_Luint();
    Z64EOCDOffs = z.get_Lqword();
    TotalDisks  = z.get_Luint();
    return 4 + 4 + 8 + 4; // 20
  };

  template< class zipdump >
  void Store( zipdump* _z ) {
    zipdump& z = *_z;
    z.put_Luint( sign );
    z.put_Luint( DiskNumber );
    z.put_Lqword( Z64EOCDOffs );
    z.put_Luint( TotalDisks );
  };

  void Dump( void ) {
    printf( " Z64EOCDOffs=%llX; TotalDisks=%u\n", (unsigned long long)Z64EOCDOffs, TotalDisks );
  }

};
