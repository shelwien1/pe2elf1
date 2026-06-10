
uint get_uint( void ) {
  uint a=0, b;
  b = get(); a=b;
  b = get(); (a<<=8) |= b;
  b = get(); (a<<=8) |= b;
  b = get(); (a<<=8) |= b;
  return a;
}

uint get_Luint( void ) {
  uint a=0, b;
  b = get(); a=b;
  b = get(); a |= b<<8;
  b = get(); a |= b<<16;
  b = get(); a |= b<<24;
  return a;
}

void put_Luint( uint a ) {
  put( byte(a) );
  put( byte(a>>8) );
  put( byte(a>>16) );
  put( byte(a>>24) );
}

uint get_Lword( void ) {
  uint a = 0, b;
  b = get(); a=b;
  b = get(); a |= b<<8;
  return a;
}

void put_Lword( uint a ) {
  put( byte(a) );
  put( byte(a>>8) );
}

qword get_Lqword( void ) {
  qword a = 0;
  uint i;
  for( i=0; i<8; i++ ) a |= qword(get()) << (8*i);
  return a;
}

void put_Lqword( qword a ) {
  uint i;
  for( i=0; i<8; i++ ) put( byte(a>>(8*i)) );
}

uint get_byte( void ) { return get(); }

void put_byte( uint c ) { put(c); }

void skip_string( uint size ) {
  uint i;
  for( i=0; i<size; i++ ) get_byte();
}

void copy_string( uint size ) {
  uint i;
  for( i=0; i<size; i++ ) put( get() );
}

void get_string( byte* buf, uint size, uint toread ) {
  uint i;
  if( size<=toread ) {
    for( i=0; i<size; i++ ) buf[i]=get_byte();
    skip_string( toread-size );
  } else {
    for( i=0; i<toread; i++ ) buf[i]=get_byte();
    for( i=i; i<size; i++ ) buf[i]=0; // clear unread bytes
  }
}

void put_string( byte* buf, uint size, uint toread ) {
  uint i;
  if( size<=toread ) {
    for( i=0; i<size; i++ ) put( buf[i] );
    for( i=i; i<toread; i++ ) put( 0 );
  } else {
    for( i=0; i<toread; i++ ) put( buf[i] );
  }
}
