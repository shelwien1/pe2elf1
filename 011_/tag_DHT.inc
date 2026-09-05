
//uint i;
uint index;
uint count;

void pa_DHT( void ) { CORO_START;

  while(1) {

    if( l_tag<17 ) {
      if( l_tag>0 ) pjpg_warn( "trailing bytes after the last Huffman table" );
      break;
    }

    index = coro2_get(); 
    printf( "Define Huffman Table 0x%02x\n", index );

    // Tc (high nibble) selects DC/AC, Th (low nibble) the table slot.
    if( (index&15)>=4 ) { pjpg_err( E_BADDHT, "Huffman table index >= 4" ); CORO_EXIT; }
    if( (index>>4)>=2 ) { pjpg_err( E_BADDHT, "Huffman table class > 1" ); CORO_EXIT; }

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

    // THE fix for the huffval[256] overflow.  bits[1..16] are 16 unconstrained
    // bytes, so count reached 16*255 = 4080 and the symbol loop below wrote
    // 3824 bytes past a byte[256], straight through the rest of cinfo.  This is
    // the check the original left commented out, plus the segment-length bound.
    if( count > uint(DIM(h[index&15].huffval)) ) { pjpg_err( E_BADDHT, "Huffman table declares more than 256 symbols" ); CORO_EXIT; }
    if( count > l_tag )                    { pjpg_err( E_BADDHT, "Huffman table longer than its segment" ); CORO_EXIT; }

    for( i=0; i<count; i++ ) {
      #define huffval h[index&15].huffval
      huffval[i] = coro2_get();
      #undef huffval
    }

    h[index&15].sent_table = 1;

    #undef h
  }

  coro_done: CORO_END;
}
