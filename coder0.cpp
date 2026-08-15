
// -------------------------------------------------------------
// LOGWR+UVROT port (2026-07-15): wr optimized in log space, in the
// rotated basis u=(ln wr0+ln wr1)/2 [memory length], v=(ln wr0-ln wr1)/2
// [hit/miss asymmetry], each with a dedicated updater class (Config_U /
// Config_V); relaxed wr box [0.00145, 0.3125]. Flat version: no globals,
// no tiers, no cross-Hessian/2x2 -- diagonal Newton only, shared h00+h11
// curvature for both axes. RTRL traces stay in linear wr space.
// gcc 13.3 -O3 -march=haswell -ffast-math (linux, relative deltas only):
//   book1        345650 -> 344968 (-682,  -0.197%)
//   wcc386       313265 -> 311301 (-1964, -0.627%)
//   book1_wcc386 660884 -> 658352 (-2532, -0.383%)   total -5178 (-0.392%)
// Isolated contributions on top of baseline: LOGWR+UVROT alone (old box)
// -443; + relaxed box -4722 cum; + v/u step & clip re-tune -5178 cum.
// New C0_*_u / C0_*_v constants defined in sh_model-C0_h.inc style.
// Also fixed: trailing RTRL leakage lines now #if ADAPT_WR guarded
// (ADAPT_WR=0 didn't compile before).

#include "sh_common.inc"
#include "sh_counter0.inc"

static const uint CNUM = 256;

ALIGN(64) Rangecoder rc;
Counter o1[256][256];

int main( int argc, char** argv ) {
  uint f_DEC, i, j, c, f_len, f_pos, cxt, bit, p;
  FILE* f;
  FILE* g;

#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

  if( argc < 4 ) {
    print_usage:
    printf(
      "O1 Compressor - Order-1 adaptive lossless compression\n"
      "\n"
      "Usage: %s <mode> <input> <output>\n"
      "\n"
      "Arguments:\n"
      "  <mode>    'c' for compress, 'd' for decompress\n"
      "  <input>   Input file path\n"
      "  <output>  Output file path\n"
      "\n", argv[0]
    );
    return 1;
  }

  f_DEC = (argv[1][0]=='d');
  f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  if( f_DEC==0 ) {
    f_len = flen(f);
    fwrite( &f_len, 1,sizeof(f_len), g );
    rc.StartEncode(g);
  } else {
    f_len = 0;
    fread( &f_len, 1,sizeof(f_len), f );
    rc.StartDecode(f);
  }

  // Initialize Order-1 Predictor array
  for( i=0; i<CNUM; i++) for( j=0; j<CNUM; j++ ) o1[i][j].Init();

  int last_c = 0; 

  for( f_pos=0; f_pos<f_len; f_pos++ ) {
    if( f_DEC==0 ) c = getc(f);

    for( cxt=1; cxt<CNUM; ) {
      if( f_DEC==0 ) bit=(c>>7)&1;

      p = o1[last_c][cxt].Predict();
      
      bit = rc.rc_BProcess( p, bit );

      o1[last_c][cxt].C_Update( bit );

      c<<=1; cxt+=cxt+bit;
    }

    cxt = byte(cxt);

    if( f_DEC==1 ) putc(cxt,g);

    last_c = cxt; 
  }

  if( f_DEC==0 ) rc.FinishEncode();

  fclose(g);
  fclose(f);

  return 0;
}
