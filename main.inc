// The driver, shared by every model in this repository (fpaq0mw.cpp,
// tangelo_w.cpp). It is model-agnostic: everything about turning a bitwise
// model into a byte distribution lives in ByteModel (bytemodel.inc), and this
// file only codes the symbols that distribution describes.
//
// Per input byte: ask the model for P(next byte) over all 256 values, code the
// byte in ONE multi-symbol range coder step, then update the model for real
// with the eight bits that were actually coded.
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
#include "bytemodel.inc"

//////////////////////////// main ////////////////////////////

ByteModel<Coder> M;

// One binary decision at even odds, straight into the range coder. The alphabet
// map is 256 of these, so it costs exactly 32 bytes.
static uint code_flag( Rangecoder& rc, uint f_DEC, uint bit ) {
  if( f_DEC ) bit = (rc.rc_GetFreq(2)!=0);
  rc.rc_Process( bit, 1, 2 );
  return bit;
}

#ifdef TRACK_VERIFY
// Self-check, built with -DTRACK_VERIFY=<n>. Two different checks, and it is
// worth being clear about which one catches what, because neither failure
// breaks a round trip - they only cost compression.
//
//   The snapshot. For the checked bytes, every byte of model state is copied
//   aside, the walk runs, and the state is compared. This is the escape
//   detector: if any write dodged the journal, UNDO() leaves the model changed
//   and this names the offset. It is thorough and expensive - the whole model,
//   twice, per checked byte - so it is checked for the first n input bytes and,
//   with -DTRACK_VERIFY_EVERY=<k>, every k-th byte after that. Sampling matters:
//   a model's later paths differ from its first (nothing is evicted from a cold
//   hash table), so a prefix-only run leaves them untested.
//
//   The prediction. For EVERY byte, the eight real per-bit code lengths are
//   summed and required to equal the walk's entry for that symbol. This is a
//   correspondence check between the simulated step and the real one, not an
//   escape detector: it catches whatever makes the two disagree - a global that
//   exists twice, the two translation units disagreeing on a layout - and it
//   catches an escaped write only when that write happens to change one of the
//   eight predictions the check compares. Cheap enough to leave on for the
//   whole file.
//
// A program lists any model state living outside the Coder object in
// TRACK_VERIFY_RANGES (each entry ", {name, address, size}"). Note that
// ByteModel's own clen/freq/live are NOT model state: the walk writes them and
// is meant to.
#ifndef TRACK_VERIFY_RANGES
 #define TRACK_VERIFY_RANGES
#endif
#ifndef TRACK_VERIFY_EVERY
 #define TRACK_VERIFY_EVERY 0
#endif

// Which input bytes get the (expensive) state snapshot.
static int verify_this( uint i ) {
  return (i<TRACK_VERIFY) || (TRACK_VERIFY_EVERY && ((i%TRACK_VERIFY_EVERY)==0));
}

struct VRange { const char* name; byte* p; uint n; };
VRange vrange[] = { {"Coder",(byte*)&M.E,sizeof(Coder)} TRACK_VERIFY_RANGES };
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
  uint i,c,low,f_len;
  byte cmap[256];

  Track_Init();

  if( f_DEC==0 ) {
    f_len=flen(in); fwrite( &f_len,1,4,out );
    M.Init(COMPRESS,out);
  } else {
    f_len=0; if( fread( &f_len,1,4,in )!=4 ) f_len=0;
    M.Init(DECOMPRESS,in);
  }

  // The alphabet map. Every byte value absent from the input can be dropped
  // from the distribution, which removes both the probability mass its minimum
  // frequency was taking and the tree that leads to it. It is only worth its 32
  // bytes on a long enough file, so the encoder decides and spends one bit
  // saying so - it is the side that knows how many values are missing.
  uint use_cmap = 0;
  for( i=0; i<256; i++ ) cmap[i] = 1;

  if( f_DEC==0 ) {
    uint dead = 256;
    for( i=0; i<256; i++ ) cmap[i] = 0;
    for( i=0; i<f_len; i++ ) { uint b=byte(getc(in)); dead -= (cmap[b]==0); cmap[b]=1; }
    fseek( in, 0, SEEK_SET );
    // Each dead symbol holds one of the ~PSCALE frequency units. Dropping them
    // saves log2(PSCALE/(PSCALE-dead)) bits on every byte of the file; the map
    // itself costs 256 bits, and the decision to send it one more.
    double gain = double(f_len) * log2( double(PSCALE)/double(PSCALE-dead) );
    use_cmap = (dead>0) && (gain>257.0);
  }

  use_cmap = code_flag( M.E.rc, f_DEC, use_cmap );

  if( use_cmap ) {
    for( i=0; i<256; i++ ) cmap[i] = code_flag( M.E.rc, f_DEC, cmap[i] );
    M.SetAlphabet( cmap );
  }

  for( i=0; i<f_len; i++ ) {

    // The walk explores seven levels and reads the eighth off P(), so it only
    // means anything from a byte boundary; entering mid-byte would silently
    // produce a distribution for the wrong bits. Nothing else here would
    // notice: the walk and the real update would stay consistently mis-aligned,
    // so the round trip, the journal check and both halves of -DTRACK_VERIFY
    // all still pass, and only the compression suffers. Hence a direct check.
    if( !M.AtByteBoundary() ) {
      fprintf( stderr, "track: the speculative walk entered mid-byte at input byte %u -\n"
                       "  the byte distribution it produces is meaningless.\n", i );
      exit(8);
    }

#ifdef TRACK_VERIFY
    if( verify_this(i) ) verify_save();
#endif

    M.Predict();

#ifdef TRACK_VERIFY
    if( verify_this(i) ) verify_check(i);
#endif

    // The walk must have journaled and rolled back something, or it was not the
    // instrumented copy of the model step that ran (see track.pl's check that
    // this translation unit exports nothing but its one entry point) and every
    // speculative branch has been leaking into the real model.
    // That failure is invisible in a round trip - both sides corrupt the model
    // identically - and shows up only as bad compression, so check it here.
    if( (i==0) && (trk_undone==0) ) {
      fprintf( stderr, "track: the speculative walk journaled nothing - the instrumented\n"
                       "  encode_sim() is not the one being called. Rebuild with build.sh.\n" );
      exit(6);
    }

    if( f_DEC==0 ) {
      c   = byte(getc(in));
      low = M.Cum(c);
    } else {
      c   = M.Symbol( M.E.rc.rc_GetFreq(M.Total()), low );
      putc( byte(c), out );
    }

    M.E.rc.rc_Process( low, M.Freq(c), M.Total() );

    // actually update the model with the symbol that was coded
#ifdef TRACK_VERIFY
    uint sum = 0;
    for( int j=7; j>=0; --j ) {
      uint bit = (c>>j)&1;
      uint pp  = M.E.encode_sim( bit );
      sum += bit ? LOG2(pp) : LOG2(SCALE-pp);
    }
    // The walk claimed this byte would cost CodeLength(c); the model just said
    // what it really costs. They must agree exactly, unless the byte's prefix
    // was pruned (its leaf is still the 0xFFFFF the array was filled with).
    if( (M.CodeLength(c)!=0xFFFFF) && (sum!=M.CodeLength(c)) ) {
      fprintf( stderr, "track: at input byte %u the walk predicted %u for symbol %u "
                       "but the model then produced %u\n"
                       "  the simulated step is not reproducing the real one\n",
               i, M.CodeLength(c), c, sum );
      exit(7);
    }
#else
    M.Update( c );
#endif

  } // for i

  if( f_DEC==0 ) M.E.flush();

  fclose(out);
  fclose(in);

  return 0;
}

#endif
