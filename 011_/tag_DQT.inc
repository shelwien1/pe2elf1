
//uint i;
uint n;
uint prec;
uint tmp;

void pa_DQT( void ) { CORO_START;

  while(1) {

    // Pq/Tq byte plus 64 coefficients of 1 or 2 bytes each.
    if( l_tag<1 ) break;

    prec = coro2_getH();
    n    = coro2_getL();

    printf( "Define Quantization Table %d  precision %d\n", n, prec );

    // The original used a bare 'return' here.  That skipped CORO_END, so state
    // stayed non-zero, the outer loop kept feeding, and every remaining payload
    // byte re-entered at the same resume point and printed another line -- a
    // 65 KB segment produced 65004 lines of output.
    if( n>=4 )    { pjpg_err( E_BADDQT, "quantisation table index >= 4" ); CORO_EXIT; }
    if( prec>1 )  { pjpg_err( E_BADDQT, "quantisation table precision > 1" ); CORO_EXIT; }
    if( l_tag < 64u*(1+prec) ) { pjpg_err( E_BADDQT, "quantisation table shorter than 64 coefficients" ); CORO_EXIT; }

    for( i=0; i<64; i++ ) {
      if( prec ) {
        tmp = coro2_get2();
      } else {
        tmp = coro2_get();
      }
      quant_tbl[n].quantval[jpeg_natural_order[i]] = tmp;
    }

    quant_tbl[n].f_undef = 0;
  }

  if( l_tag>0 ) pjpg_warn( "trailing bytes after the last quantisation table" );

  coro_done: CORO_END;
}
