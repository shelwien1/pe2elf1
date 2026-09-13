
// mode: 0=encode, 1=decode -- the same parameter sh_v1y.inc's Rangecoder
// takes, and it is passed straight through to it.
//
// stage: which halves of the pipeline this instance runs
//   0 = both  (c/d)
//   1 = model half only, RLE stream on the other side  (cr/dr)
//   2 = coder half only, RLE stream on the other side  (cc/dc)
//
// Model contains both PSRC stages:
//
//   encode:  file -> [ model -> qbuf -> bitsort/RLE -> rangecoder ] -> file
//   decode:  file -> [ rangecoder -> RLE expand -> qbuf -> model ] -> file
//
// The stage boundary is unchanged -- it is just a call now instead of a
// coroutine switch through CoroutinePair.  The model still speaks only
// quant bytes (qbuf), never the rangecoder, so splitting the two back apart
// (threads, or W parallel coders) means cutting at the same two call sites:
// the rle_put() at the end of a model_enc() block, and the rle_get() at the
// start of a model_dec() block.  stage 1 and 2 cut exactly there and put a
// file in between -- see NOTES.md for the intermediate format.
//
// Pin use is disjoint, which is why the merge needs no extra buffers:
//   encode -- model reads pin[0] (file), coder writes pin[1] (file)
//   decode -- coder reads pin[0] (file), model writes pin[1] (file)
// qbuf replaces CoroutinePair::midbuf and is the only stage-to-stage state.

template< int mode, int stage=0 >
struct Model : Coroutine {

  enum{ CNUM=256 };
  enum{ qbufsize = (stage==2) ? tmpbufsize_cod : (mode ? tmpbufsize_dec : tmpbufsize_enc) };

  ALIGN(32)
  Counter f0[CNUM];       // order0 freqs
  ALIGN(32)
  Counter f1[CNUM][CNUM]; // order1 freqs

  uint f_len;

  // --- coder stage (was struct rle_class) ---
  #include "model_sort.inc"
  #include "model_freq.inc"
  #include "model_rleproc.inc"
  #include "model_rle_get.inc"
  #include "model_rle_put.inc"

  // quant pool: encode writes 1 byte per binary decision here and the coder
  // stage rewrites it in place (bitsort scratch lives past quantbufsize);
  // decode expands the RLE chunks into it, behind the rle_state table.
  // stage 2 has no quant pool -- there it holds one block of RLE bytes.
  ALIGN(4096)
  byte qbuf[qbufsize+16];

  #include "model_bufio.inc"

  #include "model-dec.inc"
  #include "model-enc.inc"
  #include "model-code.inc"

  void do_process( void ) {

    init_qmar();

    uint i,j;
    for( i=0; i<CNUM; i++ ) {
      f0[i].P = Max(1,Min(int(mSCALE),M_f0P0));
      for( j=0; j<CNUM; j++ ) f1[i][j].P=Max(1,Min(int(mSCALE),M_f1P0));
    }

    rle_init();

    ALIGN(16)
    Rangecoder<mode> rc;
    if( stage!=1 ) rc.rc_Init(this); // get() yields 1 on decode; stage 1 has no rc

    if( stage==2 ) {
      mode ? code_dec(rc) : code_enc(rc);
    } else {
      mode ? model_dec(rc) : model_enc(rc);
    }

    if( (mode==0) && (stage!=1) ) {
      rc.Process( this, mSCALE, 1 ); // encode EOF
      rc.rc_Quit(this);
    }

    yield(this,0);
  }

};
