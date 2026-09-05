
void pa_DRI( void ) { CORO_START;

  restart_interval = coro2_get2();

  printf( "Define Restart Interval %u\n", restart_interval );

  CORO_END;
}


