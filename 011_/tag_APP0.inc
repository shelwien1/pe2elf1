
byte d[16];
uint i;
uint f_RGB; 
short xs,ys;

void pa_APP0( void ) { CORO_START;

  f_RGB = 0;

  d[0] = coro2_get();
  d[1] = coro2_get();
  d[2] = ' ';
  d[3] = ' ';

  if( (d[0]=='J') && (d[1]=='F') ) {
    d[2] = coro2_get();
    d[3] = coro2_get();
    d[4] = coro2_get();
  }

  printf( "APP0: %c%c%c%c\n", d[0],d[1],d[2],d[3] );

  if( (d[2]=='I') && (d[3]=='F') && (d[4]==0) ) {
    saw_JFIF_marker = 1;
    JFIF_major_version = coro2_get();
    JFIF_minor_version = coro2_get();
    density_unit = coro2_get();
    X_density = coro2_get2();
    Y_density = coro2_get2();
    printf( "JFIF APP0 marker: version %d.%02d, density %dx%d  unit=%d\n", JFIF_major_version, JFIF_minor_version, X_density, Y_density, density_unit );
    f_RGB = 1;
  }

  if( (d[2]=='X') && (d[3]=='X') && (d[4]==0) ) {
    d[5] = coro2_get();
    switch( d[5] ) {
    case 0x10: printf( "JFXX APP0 marker: JPEG-compressed thumbnail image\n" ); break;
    case 0x11: printf( "JFXX APP0 marker: palette thumbnail image\n" ); break;
    case 0x13: printf( "JFXX APP0 marker: RGB thumbnail image\n" ); f_RGB=1; break;
    default:   printf( "JFXX APP0 marker: type 0x%02x\n", d[5] ); break;
    }
  }

  if( f_RGB ) {
    xs = coro2_get();
    ys = coro2_get();
    printf( "    with %d x %d thumbnail image\n", xs, ys );
    if( l_tag != xs*ys*3 )
      printf( "Warning: thumbnail image size does not match; xs*ys*3=%i l_tag=%i\n", xs*ys*3, l_tag );
  }

  CORO_END;
}


/*
			case 0x11:
				ubyte xThumbnail;
				ubyte yThumbnail;
				struct {
					uchar r, g, b;
				} palette[256];
				uchar pixel[szSection - 8];
				break;
*/
