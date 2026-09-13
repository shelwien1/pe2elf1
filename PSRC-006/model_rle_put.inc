
// PSRC encode side, split at the stage boundary:
//   rle_sort() -- quant pool -> bitsorted RLE bytes, in place   (cr: first part)
//   rle_code() -- RLE bytes -> rangecoder                       (cc: second part)
//   rle_put()  -- both, as before                               (c)
// The RLE bytes sit in p[] in ascending class order -- chunk k occupies
// offs[k]..offs[k+1); only the *coding* order is serpentine (f_dir).

NOINLINE
uint rle_sort( byte* p, uint l, uint (&count)[probN] ) {
  uint i,x;

  bitsort( p, (uint*)&p[quantbufsize], l, count );

  for( i=0,x=0; i<probN; i++ ) x+=count[i];

  return x;
}

template< class Rangecoder, class IOBase >
NOINLINE
uint rle_code( Rangecoder& rc, IOBase* that, byte* p, uint (&count)[probN] ) {
  uint i,j,x;

  freq_proc(rc,that, &count[0],probN, max_quants );

  uint offs[probN+1];
  for( i=0,x=0; i<probN; i++ ) offs[i]=x,x+=count[i]; offs[i]=x;
  for( j=0; j<probN; j++ ) {
    uint k = f_dir ? probN-1-j : j;
    for( i=offs[k]; i<offs[k+1]; i++ ) procbyte(rc,that,p[i],k);
  }

  f_dir ^= 1;

  return x;
}

template< class Rangecoder, class IOBase >
NOINLINE
uint rle_put( Rangecoder& rc, IOBase* that, byte* p, uint l ) {
  uint count[probN];  // RLE byte counts

  rle_sort( p, l, count );

  return rle_code( rc,that, p, count );
}
