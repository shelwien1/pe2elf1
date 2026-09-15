// The driver, shared by every model in this repository (fpaq0mw.cpp,
// tangelo_w.cpp). It is model-agnostic: it only needs a Coder that offers
// rc_Init / P / encode_sim / flush, and a probability scale SCALE.
//
// Per input byte: walk the prefix tree speculatively to get the code length of
// all 256 possible bytes (track.inc), turn those into frequencies, code the
// byte in ONE multi-symbol range coder step, and only then update the model for
// real with the eight bits that were actually coded.
//
// Compiled only into the normal build; the -DSIM_FUNC build stops before this
// and contributes just the instrumented encode_sim.

#ifndef SIM_FUNC

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

#include "log2lut.inc"
#include "track.inc"

//////////////////////////// main ////////////////////////////

Coder E;

int main(int argc, char** argv) {

  if( argc!=4 || (argv[1][0]!='c' && argv[1][0]!='d') ) {
    printf("To compress:   %s c input output\n"
           "To decompress: %s d input output\n", argv[0], argv[0]);
    exit(1);
  }

  uint f_DEC = (argv[1][0]!='c');

  FILE *in=fopen(argv[2], "rb");  if( in==0 ) perror(argv[2]), exit(1);
  FILE *out=fopen(argv[3], "wb"); if( out==0 ) perror(argv[3]), exit(1);
  int c,d;
  uint i,j,f_len,r;
  int code,low,total=0,freq[1+256];

  Track_Init();

  if( f_DEC==0 ) {
    f_len=flen(in); fwrite( &f_len,1,4,out );
    E.rc_Init(COMPRESS,out);
  } else {
    f_len=0; if( fread( &f_len,1,4,in )!=4 ) f_len=0;
    E.rc_Init(DECOMPRESS,in);
  }

  for( i=0; i<f_len; i++ ) {

    uint clen[0x200];
    test_encode( E, clen );

    // The walk must have journaled and rolled back something, or it was not the
    // instrumented copy of the model step that ran (see track.pl's weak-symbol
    // check) and every speculative branch has been leaking into the real model.
    // That failure is invisible in a round trip - both sides corrupt the model
    // identically - and shows up only as bad compression, so check it here.
    if( (i==0) && (trk_undone==0) ) {
      fprintf( stderr, "track: the speculative walk journaled nothing - the instrumented\n"
                       "  encode_sim() is not the one being called. Rebuild with build.sh.\n" );
      exit(6);
    }

    for( d=0,low=0; d<256; d++ ) {
      r = LOG2(SCALE)*8 - clen[0x100+d];

      if( r>0xFFFFF ) r=0xFFFFF;
      freq[d]=unlog[r];
      low += freq[d];
    } freq[256] = total = low;

    if( f_DEC==0 ) {

      c = byte(getc(in));
      for( j=0,low=0; j<uint(c); j++ ) low+=freq[j];

    } else {

      code = E.rc.rc_GetFreq(total);
      for( c=0,low=0; low+freq[c]<=code; c++ ) low+=freq[c];
      putc( byte(c), out );

    }

    E.rc.rc_Process(low,freq[c],total);

    // actually update the model with the symbol that was coded
    for( int j=7; j>=0; --j ) E.encode_sim( (c>>j)&1 );

  } // for i

  if( f_DEC==0 ) E.flush();

  fclose(out);
  fclose(in);

  return 0;
}

#endif
