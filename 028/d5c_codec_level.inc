
/* Everything a per-plane coder needs from the block loop.  The five planes
 * below are genuinely independent given this: each declares its own locals
 * (the a0/c0/actual/x/d that look shared are redeclared in every section),
 * and the only value that flows between them is `cur`, which the endpoint
 * planes fill and the index planes read.
 *
 * The helpers are D5C_FORCE_INLINE, and that is not decoration.  This is the
 * hottest loop in the codec; when the per-format split multiplied the
 * instantiations GCC silently stopped inlining mbit -- 3 out-of-line copies
 * became 26, one call per coded bit -- and turned a win into a loss.  Carving
 * a 500-line function into helpers puts the same pressure on the same
 * heuristic, so the intent is stated rather than left to a budget. */
struct BlkRef {
  u8P bp; u32 bx, by;
  bool hasW, hasN;
  const Blk &W, &N, &NW, &NE, &Z;
  Blk& cur;
};


#include "d5c_codec_plane_aep.inc"

#include "d5c_codec_plane_x4.inc"

#include "d5c_codec_plane_cep.inc"

#include "d5c_codec_plane_ai.inc"

#include "d5c_codec_plane_ci.inc"



/* d5c_codec_level.inc -- included INSIDE struct Codec; not standalone.
*
* Per-level coding: code_level's format dispatch, the per-subformat
* code_level_chan, and the order-1 tail coder. */

//------------------------------------------------- per-level coding
// enc: data -> rc; dec: rc -> data. data = pointer to this level's blocks.
void code_level(u8P data) {
  const int nch = fmt_ach(fmt);
  // BC5: one full pass per channel, so each sees a clean causal neighbourhood
  for(int ch = 0; ch < (nch == 2 ? 2 : 1); ch++) {
    if(ch) achan_swap();
    switch(fmt) {
      case FMT_DXT1: code_level_chan<FMT_DXT1>(data, ch); break;
      case FMT_DXT3: code_level_chan<FMT_DXT3>(data, ch); break;
      case FMT_DXT5: code_level_chan<FMT_DXT5>(data, ch); break;
      case FMT_BC4:  code_level_chan<FMT_BC4 >(data, ch); break;
      case FMT_BC5:  code_level_chan<FMT_BC5 >(data, ch); break;
      default:       code_level_chan<FMT_BTC >(data, ch); break;
    }
    if(ch) achan_swap();
  }
}

/* Per-subformat instantiation.  `fmt` here is a function-local constexpr
 * that shadows the Codec member of the same name, so every `fmt ==` test,
 * every fmt_*() call and both index-loop bounds below fold at compile time
 * and the body needed no editing.  Dispatch is the switch in code_level(),
 * once per level and channel.
 *
 * The state deliberately does NOT split: a solid archive mixes formats in
 * one stream over ONE model, and that carry-over is the solid gain.  So the
 * CODE is per-format and the tables stay shared. */
template<int F>
void code_level_chan(u8P data, int ch) {
  constexpr dxt_fmt fmt = dxt_fmt(F);
  const int bsz = fmt == FMT_BTC ? 4 : (fmt == FMT_DXT1 || fmt == FMT_BC4) ? 8 : 16;
  const int aoff = ch * 8;          // BC5 channel Y lives at byte 8
  const int nbi = fmt_ibits(fmt);   // index width: 3, or 1 for BTC
  const int npa = fmt_npal(fmt);    // palette entries: 8, or 2
  const int nwb = nbi * 16 / 8;     // index-word bytes: 6, or 2
  const bool wcol = fmt_color(fmt); // no 565 half at all in BC4/BC5
  const bool walp = fmt_ach(fmt) > 0;
  Blk Z = {};
  for(u32 by = 0; by < bh; by++) {
    for(u32 bx = 0; bx < bw; bx++) {
      u8* bp = data + (size_t(by) * bw + bx) * bsz + aoff;
      bool hasW = bx > 0, hasN = by > 0;
      const Blk& W = hasW ? rowC[bx - 1] : Z;
      const Blk& N = hasN ? rowP[bx] : Z;
      const Blk& NW = (hasW && hasN) ? rowP[bx - 1] : Z;
      const Blk& NE = (hasN && bx + 1 < bw) ? rowP[bx + 1] : Z;
      Blk cur = {};

      BlkRef q{bp, bx, by, hasW, hasN, W, N, NW, NE, Z, cur};
      plane_aep<F>(q);
      plane_x4 <F>(q);
      plane_cep<F>(q);
      plane_ai <F>(q);
      plane_ci <F>(q);
      rowC[bx] = cur; cBlk[size_t(by) * bw + bx] = cur;
      if(blk_lim && size_t(by) * bw + bx + 1 >= blk_lim) return;   // truncated
    }
    std::swap(rowP, rowC);
  }
}

void code_tail(u8P d, size_t n) { int prev = 0; code_tail(d, n, prev); }

/* `prev` carried across calls: the order-1 tail model must see one
 * continuous byte sequence even when the caller feeds it in chunks (which
 * is how a container gap is streamed out without being buffered). */
void code_tail(u8P d, size_t n, int& prev) {
  scat = 7;
  for(size_t i = 0; i < n; i++) {
    CtrP r = tTail.row(int(MakeTL(u32(prev))));
    int nd = 1;
    for(int k = 7; k >= 0; k--) {
      Ctr* c = r + nd;
      int p = c->pr();
      int b = rc.bit(p, enc ? (d[i] >> k) & 1 : -1);
      stat[scat] += CLT[p][b];
      c->upd(b);
      nd = nd + nd + b;
    }
    if(!enc) d[i] = u8(nd - 256);
    prev = d[i];
  }
}
