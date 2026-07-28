/* xad_match.inc -- part of xadpcm.cpp; not standalone.
 *
 * MatchModel<ORD,HB,TB> -- the two repeat/high-order context models. */

/* Match model.
   The context models above all look a handful of codes back.  What they cannot
   see is that a passage occurred BEFORE -- a looped sample, a repeated sound
   effect, a long silence, the same asset appearing twice in a solid stream --
   and that is exactly the redundancy a general-purpose compressor finds with
   LZ and this codec was blind to.

   It works on the coded symbols in coding order (channels interleaved, blocks
   concatenated), because that is the sequence that actually repeats: two
   stretches of identical audio produce identical codes only when the encoder
   state matches too, and ADPCM resynchronises that state at every block
   header, so real repeats show up as exact runs here.

   Both sides build the history from symbols they already have, so nothing
   about the match is transmitted.  In a solid stream the history deliberately
   survives a wav boundary -- a sound effect shared by two files is the case
   this is for -- while the match length resets, since the new file's opening
   does not continue the old file's ending. */
/* The window length, the history-ring size and the hash-table size are
   template parameters, because the two instances differ in nothing else and
   because push() -- which runs once per coded symbol -- reads `order`, `bpow`
   and `hmask` on every call.  bpow is B^(order-1): as a member it was a load,
   as an IDXC it is an immediate in the Const build.  (Folding the loop that
   computes it saves nothing on its own; init() runs once.)

   MORD/MHB/MTB rather than ORD/HB/TB: HB is already the predictor's history
   ring in xad_pred.inc, and a template parameter that shadows a file-scope
   constant of the same name is exactly the kind of thing that reads correctly
   and compiles into something else. */
template<IDXP(MORD), IDXP(MHB), IDXP(MTB)>
struct MatchModel {
  vector<u8> hist; // coded symbols, in coding order
  vector<u32> tab; // hash of the last `order` symbols -> position
  u32 pos = 0;                // where the next symbol lands
  u32 ptr = 0;                // history position the prediction reads
  u32 len = 0;                // symbols matched so far, 0 = no match
  u32 h = 0;                  // rolling window hash
  u32 floor_ = 0;             // matches before here are out of bounds
  int expect = -1;

  static constexpr u32 B = 0x9E3779B1u;
  static constexpr u32 bpow_of(int ord) {
    u32 p = 1;
    for( int i = 1; i<ord; i++ )
      p *= B;
    return p;
  }

  IDXC(int, order, pclamp(MORD, 2, 64));
  IDXC(int, hbits, pclamp(MHB, 8, 28));
  IDXC(int, tbits, pclamp(MTB, 8, 26));
  IDXC(u32, hmask, u32((size_t(1)<<pclamp(MHB, 8, 28))-1));
  IDXC(u32, tmask, u32((size_t(1)<<pclamp(MTB, 8, 26))-1));
  IDXC(u32, bpow, bpow_of(pclamp(MORD, 2, 64)));

  void init() {
    hist.assign(size_t(1)<<hbits, 0);
    tab.assign(size_t(1)<<tbits, 0);
    pos = ptr = len = h = 0;
    floor_ = 0;
    expect = -1;
  }

  /* Forget everything without touching the tables.  Clearing a 16 MB hash per
     wav would dominate the run on a directory of short files, and it is not
     needed: raising the floor makes every entry from before this point fail the
     bounds test, so old positions can never be matched into. */
  void barrier() {
    floor_ = pos;
    len = 0;
    expect = -1;
  }

  void newseg() { // keep what was learned, drop the position
    len = 0;
    expect = -1;
  }

  /* An exact window hash: the symbol leaving the window is subtracted before
     the new one shifts in, so the hash names the last `order` symbols and
     nothing older -- unlike a decayed multiplicative hash, whose effective
     order drifts with the multiplier. */
  INLINE void push(int sym) {
    u32 old = pos>=u32(order) ? u32(hist[(pos-u32(order))&hmask])+1 : 0;
    hist[pos&hmask] = u8(sym);
    h = (h-old*bpow)*B+u32(sym)+1;
    pos++;
  }

  INLINE u32 slot() const {
    return (h*0x2545F491u)&tmask;
  }

  // after the real symbol is known: extend the run, or look for a new one
  INLINE void update(int sym) {
    if( len&&int(hist[ptr&hmask])==sym ) {
      ptr++;
      if( len<0xFFFF )
        len++;
    } else
      len = 0;
    push(sym);
    u32 sl = slot();
    if( !len&&pos>u32(order) ) {
      u32 cand = tab[sl];
      if( cand>floor_&&pos-cand<=hmask ) {
        ptr = cand;
        len = u32(order); // the hash asserts this much; prediction proves it
      }
    }
    tab[sl] = pos;
    expect = len ? int(hist[ptr&hmask]) : -1;
  }
};
