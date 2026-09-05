
// DNL, EXP, COM -- small segments with no state of their own.

uint dnl_lines;
uint exp_h, exp_v;
byte combuf[256];
uint comlen;

void pa_DNL( void ) { CORO_START;

  if( l_tag<2 ) { pjpg_err( E_BADLEN, "DNL segment shorter than 2 bytes" ); CORO_EXIT; }

  dnl_lines = coro2_get2();

  printf( "Define Number of Lines: %u\n", dnl_lines );

  if( dnl_lines==0 ) pjpg_warn( "DNL declares 0 lines" );
  if( image_height==0 ) image_height = dnl_lines;

  coro_done: CORO_END;
}

void pa_EXP( void ) { CORO_START;

  if( l_tag<1 ) { pjpg_err( E_BADLEN, "EXP segment shorter than 1 byte" ); CORO_EXIT; }

  exp_h = coro2_getH();
  exp_v = coro2_getL();

  printf( "Expand Reference Component: Eh=%u Ev=%u\n", exp_h, exp_v );

  if( (exp_h>1) || (exp_v>1) ) pjpg_warn( "EXP expansion flag is not 0 or 1" );

  coro_done: CORO_END;
}

void pa_COM( void ) { CORO_START;

  // Read at most sizeof(combuf)-1 bytes for display; the outer loop skips the
  // rest of the segment in bulk once this parser finishes.
  comlen = 0;
  while( l_tag>0 ) {
    if( comlen+1 >= uint(DIM(combuf)) ) break;
    combuf[comlen] = coro2_get();
    comlen++;
  }
  combuf[comlen] = 0;

  for( i=0; i<comlen; i++ ) {
    if( (combuf[i]<0x20) || (combuf[i]>0x7E) ) combuf[i]='.';
  }

  printf( "Comment (%u bytes): %s%s\n", comlen + l_tag, combuf, l_tag ? " ..." : "" );

  CORO_END;
}
