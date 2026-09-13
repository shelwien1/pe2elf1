
// PSRC decode side.  rle_get() is the fused path used by `d': decode a run
// byte and expand it to bits right away.  The split commands use the two
// halves instead:
//   rle_dcod() -- rangecoder -> RLE bytes           (dc: second part, inverse of rle_code)
//   rle_expd() -- RLE bytes (from pin[0]) -> chunks (dr: first part, inverse of rle_sort)

template< class Rangecoder, class IOBase >
NOINLINE
void rle_get( Rangecoder& rc, IOBase* that, byte* p ) {
  uint i,j,x,y;
  uint count[probN];  // RLE byte counts

  rle_state& rs = (rle_state&)p[0];

  // fetch quant pool size table
  freq_proc(rc,that, &count[0],probN, max_quants );

  byte* restrict q = &p[sizeof(rle_state)];
  for( j=0; j<probN; j++ ) {
    uint k = f_dir ? probN-1-j : j;
    rs.ptr[k] = q;
    uint y = count[k];
    uint bit0 = (k&1), bit1 = bit0^1;
    for( i=0; i<y; i++ ) {
      uint c = procbyte(rc,that,0,k);
      byte* r = q+c;
      while( q<r ) *q++=bit1;
      if( c<255 ) *q++=bit0;
    }
    // add max_trail_FFs after each segment
    for( i=0; i<255*(max_trail_FFs+1); i++ ) *q++=bit1;
  }

  f_dir ^= 1;
}

// dc: decode one block's RLE bytes into p[] in ascending class order (the
// layout rle_sort() produces), so that the intermediate file is the same
// stream either way.  count[] comes back filled, return value is its sum.
template< class Rangecoder, class IOBase >
NOINLINE
uint rle_dcod( Rangecoder& rc, IOBase* that, byte* p, uint (&count)[probN] ) {
  uint i,j,x;

  freq_proc(rc,that, &count[0],probN, max_quants );

  uint offs[probN+1];
  for( i=0,x=0; i<probN; i++ ) offs[i]=x,x+=count[i]; offs[i]=x;
  for( j=0; j<probN; j++ ) {
    uint k = f_dir ? probN-1-j : j;
    for( i=offs[k]; i<offs[k+1]; i++ ) p[i] = procbyte(rc,that,0,k);
  }

  f_dir ^= 0;

  return x;
}

// dr: expand one block, reading the RLE bytes from the input pin instead of
// the rangecoder.  Ascending class order = the file layout; the chunks are
// independent, so where each one lands in p[] only matters to rs.ptr[].
NOINLINE
void rle_expd( byte* p, uint (&count)[probN] ) {
  uint i,j;

  rle_state& rs = (rle_state&)p[0];

  byte* restrict q = &p[sizeof(rle_state)];
  for( j=0; j<probN; j++ ) {
    uint k = j;
    rs.ptr[k] = q;
    uint y = count[k];
    uint bit0 = (k&1), bit1 = bit0^1;
    for( i=0; i<y; i++ ) {
      uint c = get();
      byte* r = q+c;
      while( q<r ) *q++=bit1;
      if( c<255 ) *q++=bit0;
    }
    // add max_trail_FFs after each segment
    for( i=0; i<255*(max_trail_FFs+1); i++ ) *q++=bit1;
  }
}
