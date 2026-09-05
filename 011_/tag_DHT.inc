
//uint i;
uint index;
uint count;

void pa_DHT( void ) { CORO_START;
  while(1) {
    index = coro2_get(); 
    printf( "Define Huffman Table 0x%02x\n", index );
    if( (index&15)>=4 ) break; // Bogus DHT index

    #define h       ((index & 0x10) ? ac_huff_tbl_ptrs : dc_huff_tbl_ptrs)

    count = 0;
    for( i=0; i<=16; i++ ) {
      #define bits  h[index&15].bits
      if( i>0 ) {
        bits[i] = coro2_get();
        count += bits[i];
      } else {
        bits[0] = 0;
      }
      #undef bits
    }

//      if( count>256 || count>length ) printf( "Bogus Huffman table definition\n" ), exit(1);

    for( i=0; i<count; i++ ) {
      #define huffval h[index&15].huffval
      huffval[i] = coro2_get();
    }

    #undef h
  }

  CORO_END;
}

