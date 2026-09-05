
void pa_DRI( void ) { CORO_START;

  if( l_tag<2 ) { pjpg_err( E_BADLEN, "DRI segment shorter than 2 bytes" ); CORO_EXIT; }

  restart_interval = coro2_get2();

  pf( "Define Restart Interval %u\n", restart_interval );

  if( l_tag>0 ) pjpg_warn( "trailing bytes after the restart interval" );

  coro_done: CORO_END;
}
