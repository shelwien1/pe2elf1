
// stage 2 (cc/dc): the coder half on its own.  No model here at all -- the
// block schedule is still derived from f_len, exactly as model_enc/model_dec
// derive it, so the rangecoder op sequence (per-block EOF flag, freq_proc,
// procbyte run, final EOF flag + flush) is bit-identical to `c'/`d'.

NOINLINE
void code_enc( Rangecoder<mode>& rc ) { // RLE stream -> archive

  uint count[probN];

  for( uint k=0; k<f_len; ) {
    uint l = k>0 ? Min(k+max_quants/8,f_len)-k : Min(fast_quants/8,f_len); // block length
    k += l;

    uint x = get_count( count );
    getbuf( qbuf, x );

    rc.Process( this, mSCALE, 0 ); // not EOF
    rle_code( rc,this, qbuf, count );
  }
}

NOINLINE
void code_dec( Rangecoder<mode>& rc ) { // archive -> RLE stream

  uint count[probN];

  for( uint k=0; k<f_len; ) {
    uint l = k>0 ? Min(k+max_quants/8,f_len)-k : Min(fast_quants/8,f_len); // block length
    k += l;

    rc.Process( this, mSCALE, 0 );
    uint x = rle_dcod( rc,this, qbuf, count );

    put_count( count );
    putbuf( qbuf, x );
  }
}
