
byte d[16];
uint i;
uint f_RGB; 
short xs,ys;

void pa_APP0( void ) { CORO_START;

  f_RGB = 0;

  // d[] is shared with pa_APP1; clear the bytes the identification tests read so
  // no value can survive from a previous marker.
  bzero( d );

  if( l_tag<2 ) { pjpg_warn( "APP0 segment too short for a signature" ); CORO_EXIT; }

  d[0] = coro2_get();
  d[1] = coro2_get();
  d[2] = ' ';
  d[3] = ' ';

  if( (d[0]=='J') && (d[1]=='F') ) {
    if( l_tag<3 ) { pjpg_warn( "truncated APP0 signature" ); CORO_EXIT; }
    d[2] = coro2_get();
    d[3] = coro2_get();
    d[4] = coro2_get();
  }

  printf( "APP0: %c%c%c%c\n", pr(d[0]),pr(d[1]),pr(d[2]),pr(d[3]) );

  if( (d[2]=='I') && (d[3]=='F') && (d[4]==0) ) {
    if( l_tag<7 ) { pjpg_warn( "truncated JFIF APP0 header" ); CORO_EXIT; }
    saw_JFIF_marker = 1;
    JFIF_major_version = coro2_get();
    JFIF_minor_version = coro2_get();
    density_unit = coro2_get();
    X_density = coro2_get2();
    Y_density = coro2_get2();
    printf( "JFIF APP0 marker: version %d.%02d, density %dx%d  unit=%d\n", JFIF_major_version, JFIF_minor_version, X_density, Y_density, density_unit );
    if( density_unit>2 ) pjpg_warn( "JFIF density unit is not 0, 1 or 2" );
    f_RGB = 1;
  }

  if( (d[2]=='X') && (d[3]=='X') && (d[4]==0) ) {
    if( l_tag<1 ) { pjpg_warn( "truncated JFXX APP0 header" ); CORO_EXIT; }
    d[5] = coro2_get();
    switch( d[5] ) {
    case 0x10: printf( "JFXX APP0 marker: JPEG-compressed thumbnail image\n" ); break;
    case 0x11: printf( "JFXX APP0 marker: palette thumbnail image\n" ); break;
    case 0x13: printf( "JFXX APP0 marker: RGB thumbnail image\n" ); f_RGB=1; break;
    default:   printf( "JFXX APP0 marker: type 0x%02x\n", d[5] ); break;
    }
  }

  if( f_RGB ) {
    if( l_tag<2 ) { pjpg_warn( "truncated APP0 thumbnail dimensions" ); CORO_EXIT; }
    xs = coro2_get();
    ys = coro2_get();
    printf( "    with %d x %d thumbnail image\n", xs, ys );
    if( l_tag != uint(xs*ys*3) )
      printf( "Warning: thumbnail image size does not match; xs*ys*3=%i l_tag=%i\n", xs*ys*3, l_tag );
  }

  coro_done: CORO_END;
}
