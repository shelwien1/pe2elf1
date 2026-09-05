
//uint i;
uint n;
uint prec;
uint tmp;

void pa_DQT( void ) { CORO_START;

  while(1) {

    prec = coro2_getH();
    n    = coro2_getL();

    printf( "Define Quantization Table %d  precision %d\n", n, prec );

    if( n>=4 ) return; // Bogus DQT index

    //JQUANT_TBL& q = quant_tbl[n];
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

  CORO_END;
}


