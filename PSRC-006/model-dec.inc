
NOINLINE
void model_dec( Rangecoder<mode>& rc ) {
  enum { shift=5, mask=(1<<shift)-1, sSCALE=1<<shift };

  LinearMixer mix; mix.w=M_mW0;
  rle_state rs; 

  uint pp=0, p=0;

  for( uint k=0; k<f_len; ) {
    uint l = k>0 ? Min(k+max_quants/8,f_len)-k : Min(fast_quants/8,f_len); // block length
    k += l;

    // coder stage: decode the next block into qbuf (was yield(1), waiting
    // for rle_class to hand a block over through CoroutinePair).  The EOF
    // flag is read but not acted on -- f_len drives the block schedule; it
    // only turns 1 after the last block, which we never reach.
    if( stage==1 ) {
      // dr: the RLE bytes come from the file, not from the rangecoder
      uint count[probN];
      get_count( count );
      rle_expd( qbuf, count );
    } else {
      rc.Process( this, mSCALE, 0 );
      rle_get( rc,this, qbuf );
    }
    memcpy( &rs, qbuf, sizeof(rs) ); // byte*->rle_state; also dodges the
                                     // implicit copy-assign that clang
                                     // rejects for restrict-qualified
                                     // array members

    for( uint i=0; i<l; i++ ) {

      uint ctx=1; 
      do {

        const uint p0 = mix.Mixup(f0[ctx].P,f1[p][ctx].P);

//        const uint p1 = qmas[(p0+mask)>>shift] & 0x7F;
        const uint p1 = qmar[(p0+mask)>>shift];
        const uint bit = *rs.ptr[p1]++;

        if( bit ) {
          f0[ctx].update( 1, M_f0wr, M_f0mw );   
          f1[p][ctx].update( 1, M_f1wr, M_f1mw ); 
          ctx += ctx + 1;
        } else {
          f0[ctx].update( 0, M_f0wr, M_f0mw );   
          f1[p][ctx].update( 0, M_f1wr, M_f1mw ); 
          ctx += ctx + 0;
        }

      } while( ctx<256 );

      ctx = byte(ctx);
      put( ctx );

      pp = p; p = ctx;
    }

  }
}
