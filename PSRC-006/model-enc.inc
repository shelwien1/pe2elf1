
NOINLINE
void model_enc( Rangecoder<mode>& rc ) {

  enum { shift=5, mask=(1<<shift)-1, sSCALE=1<<shift };

  LinearMixer mix; mix.w=M_mW0;

  uint pp=0, p=0;

  for( uint k=0; k<f_len; ) {
    uint l = k>0 ? Min(k+max_quants/8,f_len)-k : Min(fast_quants/8,f_len); // block length
    k += l;

    byte* restrict q = qbuf; // quant pool cursor (was put0 into midbuf)

    for( uint i=0; i<l; i++ ) {

      const uint c = get();

      uint ctx=1; int b=(c<<24);
      do {

        uint p1 = mix.Mixup(f0[ctx].P,f1[p][ctx].P);
        p1 = qmas[(p1+mask)>>shift];
        
        if( b<0 ) { 
          *q++ = p1+0x80;
          f0[ctx].update( 1, M_f0wr, M_f0mw );   
          f1[p][ctx].update( 1, M_f1wr, M_f1mw ); 
          ctx += ctx + 1;
        } else {
          *q++ = p1;
          f0[ctx].update( 0, M_f0wr, M_f0mw );   
          f1[p][ctx].update( 0, M_f1wr, M_f1mw ); 
          ctx += ctx + 0;
        }

        b <<= 1;
      } while( ctx<256 );

      pp = p; p = c;
    }

    // coder stage: the block is complete, sort+RLE+code it (was yield(2)
    // into rle_class through CoroutinePair).  l>=1 always, so the quant
    // pool is never empty here.
    if( stage==1 ) {
      // cr: stop at the stage boundary -- store the block's RLE bytes
      uint count[probN];
      uint x = rle_sort( qbuf, q-qbuf, count );
      put_count( count );
      putbuf( qbuf, x );
    } else {
      rc.Process( this, mSCALE, 0 ); // not EOF
      rle_put( rc,this, qbuf, q-qbuf );
    }
  }
}
