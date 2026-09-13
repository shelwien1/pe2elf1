
// Bulk / word access to the file pins, for the RLE intermediate stream that
// the half-pipeline commands (cr/dr/cc/dc) read and write.
//
// putbuf() is the point of this file: rather than copying a whole RLE block
// through the 64K outbuf a byte at a time, it hands the block buffer itself
// to CoroFileProc as the output pin and yields -- processfile() then does one
// g.writ() straight out of qbuf and re-attaches its own outbuf afterwards.

void flushout( void ) {
  yield( this, 2 ); // CoroFileProc writes getoutsize() bytes and re-addout()s
}

void putbuf( byte* p, uint l ) {
  flushout();       // push whatever is still pending in outbuf
  addout( p, l );
  outptr = outend;  // getoutsize()==l
  yield( this, 2 ); // one write, straight out of p[]
}

void getbuf( byte* p, uint l ) {
  while( l>0 ) {
    if( inpptr>=inpend ) {
      if( pin[0].f_quit() ) { memset(p,0,l); break; } // truncated input
      yield( this, 1 );
      continue;
    }
    uint n = Min( l, uint(inpend-inpptr) );
    memcpy( p, inpptr, n );
    inpptr += n; p += n; l -= n;
  }
}

void put32( uint x ) {
  put( x&0xFF ); put( (x>>8)&0xFF ); put( (x>>16)&0xFF ); put( (x>>24)&0xFF );
}

uint get32( void ) {
  uint x;
  x  = get();
  x |= get()<<8;
  x |= get()<<16;
  x |= get()<<24;
  return x;
}

void put_count( uint (&count)[probN] ) {
  for( uint i=0; i<probN; i++ ) put32( count[i] );
}

uint get_count( uint (&count)[probN] ) { // returns the sum = RLE bytes in the block
  uint i,x;
  for( i=0,x=0; i<probN; i++ ) x += (count[i]=get32());
  return x;
}
