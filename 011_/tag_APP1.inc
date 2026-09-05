
//byte d[16];
//uint i;
uint f_LE,pos,len;
uint tagMark,ofs1st, n_entries,tagnum,format,d_elem,d_ofs,nextofs;
uint thumbOfs=0,thumbLen=0,thumbTyp=10;

void pa_APP1( void ) { CORO_START;

  d[0] = coro2_get();
  d[1] = coro2_get();
  d[2] = ' ';
  d[3] = ' ';

  if( (d[0]=='E') && (d[1]=='x') ) {
    d[2] = coro2_get();
    d[3] = coro2_get();
    d[4] = coro2_get();
    d[5] = coro2_get();
  }

  printf( "APP1: %c%c%c%c\n", d[0],d[1],d[2],d[3] );

  if( (d[2]=='i') && (d[3]=='f') && (d[4]==0) && (d[5]==0) ) {
    pos = 0; len = l_tag;
    (word&)d[6] = coro2_get2E(0,pos);

    printf( "align: %c%c\n", d[6],d[7] );
    f_LE = (d[6]=='I'); // 'I'=little endian, 'M'=big endian

    tagMark = coro2_get2E(f_LE,pos);
    ofs1st  = coro2_get4E(f_LE,pos);

    printf( "tagMark=%X ofs1st=%X pos=%X len=%X\n", tagMark,ofs1st,pos,len );

    while( ofs1st>=pos ) {

      for(; pos<ofs1st; pos++ ) {
        coro2_get();
      }

      n_entries = coro2_get2E(f_LE,pos);
      printf( "n_entries=%i\n", n_entries );
      for( i=0; i<n_entries; i++ ) {
        tagnum = coro2_get2E(f_LE,pos);
        format = coro2_get2E(f_LE,pos);
        d_elem = coro2_get4E(f_LE,pos);
        d_ofs  = coro2_get4E(f_LE,pos);
        printf( "%04X/%04X: %04X %04X %04X %04X\n", pos,len, tagnum,format,d_elem,d_ofs );
        switch( tagnum ) {
        case 0x0103: thumbTyp = f_LE ? word(d_ofs) : d_ofs>>16; break;
        case 0x0111: case 0x0201: thumbOfs = d_ofs; break;
        case 0x0117: case 0x0202: thumbLen = d_ofs; break;
        }
      }
      nextofs  = coro2_get4E(f_LE,pos);
      printf( "nextofs=%X; pos=%04X/%04X\n",  nextofs, pos,len );

      ofs1st=nextofs;
    }

    printf( "thumbOfs=%X thumbLen=%X thumbTyp=%i; pos=%04X/%04X\n",  thumbOfs,thumbLen,thumbTyp, pos,len );

    // typ=6: jpeg; typ=1: uncompressed; others unknown?
    if( (thumbLen>0) && (thumbOfs>=pos) && (thumbOfs+thumbLen<=len) ) {
      for(; pos<thumbOfs; pos++ ) {
        coro2_get();
      }
#if 0
      FILE* dbg = fopen("exif_thumbnail.jpg","wb");
      for( i=0; i<thumbLen; i++,pos++ ) putc( coro2_get(), dbg );
      fclose(dbg);
#else
      for( i=0; i<thumbLen; i++,pos++ ) {
        coro2_get();
      }
#endif
    }

    printf( "exif_done; pos=%04X/%04X\n",  pos,len );

    // main IFD
  }

  CORO_END;
}

#if 0
Compression                = 0x0103,

StripOffsets               = 0x0111,
ThumbnailOffset            = 0x0201,

ThumbnailLength            = 0x0202,
StripByteCounts            = 0x0117,

          case Compression:
            compression = ifdThumbnailImage.dirEntry[i].usValue;
            break;
          case ThumbnailOffset:
          case StripOffsets:
            thumbOffset = ifdThumbnailImage.dirEntry[i].ulValue;
            break;
          case ThumbnailLength:
          case StripByteCounts:
            thumbLength = ifdThumbnailImage.dirEntry[i].ulValue;
            break;

#endif

