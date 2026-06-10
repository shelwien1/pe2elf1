
struct ZIPENDLOCATOR {

  enum{ l_comm_max = 1<<16 };

  uint sign;    //0x06054b50
  word DiskNumber;
  word StartDiskNumber;
  word EntriesOnDisk;
  word EntriesInDirectory;
  uint DirSize;
  uint DirOffs;
  word l_comment;
  byte comment[l_comm_max];

  template< class zipdump >
  uint Parse( zipdump* _z, uint flag=1 ) {
    zipdump& z = *_z;
    if( flag ) sign = z.get_Luint(); else sign=B<'P','K',5,6>::n;
    DiskNumber = z.get_Lword();
    StartDiskNumber = z.get_Lword();
    EntriesOnDisk = z.get_Lword();
    EntriesInDirectory = z.get_Lword();
    DirSize = z.get_Luint();
    DirOffs = z.get_Luint();
    l_comment = z.get_Lword();
    z.get_string( comment, sizeof(comment), l_comment );

    return 5*2+3*4+l_comment;
  };

  template< class zipdump >
  void Store( zipdump* _z ) {
    zipdump& z = *_z;
    z.put_Luint( sign );      
    z.put_Lword( DiskNumber );
    z.put_Lword( StartDiskNumber );
    z.put_Lword( EntriesOnDisk );
    z.put_Lword( EntriesInDirectory );
    z.put_Luint( DirSize );
    z.put_Luint( DirOffs );
    z.put_Lword( l_comment ); 
    z.put_string( comment, sizeof(comment), l_comment );
  };

  void Dump( void ) {
//    printf( "sign=%08X; ", sign );
    printf( " DiskNumber=%i; ", DiskNumber );
    printf( "StartDiskNumber=%i; ", StartDiskNumber );
    printf( "EntriesOnDisk=%i; ", EntriesOnDisk );
    printf( "EntriesInDirectory=%i\n", EntriesInDirectory );
    printf( " DirSize=%i; ", DirSize );
    printf( "DirOffs=%X..%X\n", DirOffs,DirOffs+DirSize );
  }

};
