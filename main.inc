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

#ifdef TRACK_VERIFY
// Self-check, built with -DTRACK_VERIFY=<n>: for the first n input bytes,
// snapshot every byte of model state, run the speculative walk, and compare.
// If a single write escaped the journal, UNDO() leaves the model changed and
// this says exactly where. It also checks the other half of the contract - that
// the simulation predicts what the model really goes on to do - by adding up
// the eight real per-bit code lengths and requiring the sum to equal the walk's
// entry for that byte. Both failures are otherwise silent: they cost
// compression without ever breaking a round trip.
//
// A program lists any model state living outside the Coder object in
// TRACK_VERIFY_RANGES (each entry ", {name, address, size}").
#ifndef TRACK_VERIFY_RANGES
 #define TRACK_VERIFY_RANGES
#endif

struct VRange { const char* name; byte* p; uint n; };
VRange vrange[] = { {"Coder",(byte*)&E,sizeof(Coder)} TRACK_VERIFY_RANGES };
enum{ N_VRange = sizeof(vrange)/sizeof(vrange[0]) };
byte* vsnap = 0;

void verify_save( void ) {
  if( vsnap==0 ) {
    uint total=0; for( uint k=0; k<N_VRange; k++ ) total += vrange[k].n;
    vsnap = (byte*)malloc(total);
    if( vsnap==0 ) fprintf(stderr,"track: verify buffer\n"), exit(7);
  }
  byte* q = vsnap;
  for( uint k=0; k<N_VRange; k++ ) memcpy( q, vrange[k].p, vrange[k].n ), q+=vrange[k].n;
}

void verify_check( uint i ) {
  byte* q = vsnap;
  for( uint k=0; k<N_VRange; k++ ) {
    for( uint b=0; b<vrange[k].n; b++ ) if( q[b]!=vrange[k].p[b] ) {
      fprintf( stderr, "track: UNDO() did not restore %s+%u at input byte %u: %02X -> %02X\n"
                       "  a memory write in the model step is escaping the journal\n",
               vrange[k].name, b, i, q[b], vrange[k].p[b] );
      exit(7);
    }
    q += vrange[k].n;
  }
}
#endif

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

#ifdef TRACK_VERIFY
    if( i<TRACK_VERIFY ) verify_save();
#endif

    test_encode( E, clen );

#ifdef TRACK_VERIFY
    if( i<TRACK_VERIFY ) verify_check(i);
#endif

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
      // c<255 only bounds the scan: for a well-formed stream rc_GetFreq()
      // returns a value below total, so the scan always stops on its own. A
      // corrupt stream can hand it anything, and without the bound the scan
      // walks off the end of freq[].
      for( c=0,low=0; (c<255) && (low+freq[c]<=code); c++ ) low+=freq[c];
      putc( byte(c), out );

    }

    E.rc.rc_Process(low,freq[c],total);

    // actually update the model with the symbol that was coded
#ifdef TRACK_VERIFY
    uint sum = 0;
    for( int j=7; j>=0; --j ) {
      uint bit = (c>>j)&1;
      uint pp  = E.encode_sim( bit );
      sum += bit ? LOG2(pp) : LOG2(SCALE-pp);
    }
    // The walk claimed this byte would cost clen[0x100+c]; the model just said
    // what it really costs. They must agree exactly, unless the byte's prefix
    // was pruned (its leaf is still the 0xFFFFF the array was filled with).
    if( (clen[0x100+c]!=0xFFFFF) && (sum!=clen[0x100+c]) ) {
      fprintf( stderr, "track: at input byte %u the walk predicted %u for symbol %u "
                       "but the model then produced %u\n"
                       "  the simulated step is not reproducing the real one\n",
               i, clen[0x100+c], c, sum );
      exit(7);
    }
#else
    for( int j=7; j>=0; --j ) E.encode_sim( (c>>j)&1 );
#endif

  } // for i

  if( f_DEC==0 ) E.flush();

  fclose(out);
  fclose(in);

  return 0;
}

#endif
