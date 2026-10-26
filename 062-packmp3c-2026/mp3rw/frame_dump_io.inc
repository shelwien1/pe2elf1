
// The records are padded to blocks of dump_align_size, so that none spans
// two; FRAME_DUMP_ALIGN 0 (mp3pack, which cuts the dump into parts) pads
// none.
#ifndef FRAME_DUMP_ALIGN
#define FRAME_DUMP_ALIGN (1<<20)
#endif
enum {
  max_frame_size = 
  (offsetof(mp3frame,Garbage) + sizeof(((mp3frame*)0)->Garbage))
 -(offsetof(mp3frame,GrbLen)  + sizeof(((mp3frame*)0)->GrbLen))+10+10,
  dump_align_size = FRAME_DUMP_ALIGN
};

qword fd_pos;

void init_frame_dump( void ) {
  fd_pos = 0;
//printf( "!max_frame_size=%i!\n", max_frame_size );
}

qword getv( void ) {
  qword r = 0;
  for( uint l=0,c=0; (c&0x80)==0; l+=7 ) {
    c = get(); fd_pos++;
    r |= qword(c&0x7F)<<l;
  }
  r-=1; // Cover GrbLen=-1
  return r;
}

void putv( qword c ) {
  c+=1; // Cover GrbLen=-1
  for( uint flag=0; flag==0; c>>=7 ) put( (c&0x7F) | (flag=(c<128)<<7) ), fd_pos++;
}

uint read_frame_dump( qword& binlen, uint& f_EOF ) {
  uint i,l,n, flag=1;

//printf( "!rfd: %I64X!\n", fd_pos );

  GrbLen = getv();
  if( f_quit || (GrbLen>=4096*8) ) {
    if( f_quit ) binlen=-1ULL; else binlen = getv(); 
    f_EOF=binlen&1; binlen/=2;
  } else {
    byte* p = ((byte*)&GrbLen);
    l = (GrbLen+7)/8; 
    n = (((byte*)&Garbage) - p) + l;
    flag = 0;
    for( i=4; i<n; ) {
      chkinp(); l=getinplen(); if( i+l>n ) l=n-i;
      memcpy( &p[i], (byte*)inpptr, l ); i+=l; inpptr+=l; fd_pos+=l;
      if( f_quit ) break;
    }
  }

  // calc remaining bytes in aligned block
  n = dump_align_size ? dump_align_size-(fd_pos%(dump_align_size|!dump_align_size)) : max_frame_size;
  // pad to block size if next frame might not fit
  if( n<max_frame_size ) {
    // read n bytes to reach aligned pos
    for( i=0; i<n; ) {
      chkinp(); l=getinplen(); if( i+l>n ) l=n-i;
      i+=l; inpptr+=l; fd_pos+=l;
      if( f_quit ) break;
    }
//printf( "!rfd: %08I64X!\n", fd_pos );
  }

  return flag;
}

void write_frame_dump( qword binlen=0, uint f_END=0 ) {
  uint i,l,n;
//printf( "!wfd: %I64X!\n", fd_pos );

  if( f_END ) {
    if( binlen!=-1ULL ) {
      putv( 4096*8 ); // for GrbLen
      putv( binlen );
    }
  } else {

    putv( GrbLen );
    byte* p = ((byte*)&GrbLen);
    l = (GrbLen+7)/8; 
    n = (((byte*)&Garbage) - p) + l;

    for( i=4; i<n; ) {
      l=getoutlen(); if( i+l>n ) l=n-i;
      memcpy( (byte*)outptr, &p[i], l ); i+=l; outptr+=l; fd_pos+=l;
      chkout(); 
    }
  }

  // calc remaining bytes in aligned block
  n = dump_align_size ? dump_align_size-(fd_pos%(dump_align_size|!dump_align_size)) : max_frame_size;
  // pad to block size if next frame might not fit
  if( n<max_frame_size ) {
    // read n bytes to reach aligned pos
    for( i=0; i<n; ) {
      l=getoutlen(); if( i+l>n ) l=n-i;
//memset( (byte*)outptr, 0xCC, l );
      i+=l; outptr+=l; fd_pos+=l;
      chkout(); 
    }
//printf( "!wfd: %08I64X!\n", fd_pos );
  }
}
