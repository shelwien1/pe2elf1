// palette.inc -- coding the colour table.
//
// The palette used to ship verbatim after the coded body: three bytes per
// entry, 768 of them for an 8-bit image, which on a small image is a fifth of
// the whole member.  Real palettes are nothing like random -- greyscale ramps,
// smooth gradients, long runs of unused zero entries -- so a handful of
// adaptive bit counters pays for itself many times over.
//
// The segment is self-contained: its own range coder over its own buffer,
// written after the body with a two-byte length in front.  A length of
// kPaletteRaw means the model lost and the verbatim bytes follow instead, so
// the worst case costs two bytes rather than the coder's framing.
//
// Included from bmf.cpp; not a standalone translation unit.

// Palettes of fewer than 16 entries are too small for a coded segment's framing
// to pay for itself; those still ship verbatim, with no length in front.
const uint32_t kPaletteRaw = 0xFFFF;

bool palette_is_coded(uint8_t depth) {
  return (depth&depth_palette)!=0&&(depth&31)>=4;
}

// One adaptive binary counter.  The pair feeds RangeCoder::encode_bit directly,
// so the two counts are the frequencies rather than a probability.
struct PalCtr {
  static const int32_t kStep = 24;
  static const int32_t kLimit = 4096;
  uint16_t n[2];
  void Init() {
    n[0] = 1;
    n[1] = 1;
  }
  template <int32_t f_DEC> int32_t code(RangeCoder* rc, int32_t bit) {
    if constexpr( f_DEC )
      bit = rc[0].decode_bit(n[0], n[1]);
    else
      rc[0].encode_bit(n[0], n[1], bit);
    n[bit] = (uint16_t)(n[bit]+kStep);
    if( n[0]+n[1]>kLimit ) {
      n[0] = (uint16_t)((n[0]+1)>>1);
      n[1] = (uint16_t)((n[1]+1)>>1);
    }
    return bit;
  }
};

struct PaletteModel {
  // A byte goes out through a binary tree: 255 internal nodes, indexed the
  // usual way from the root at 1.
  static const int32_t kTree = 256;
  // Residual contexts: one per channel, split on whether the previous residual
  // in that channel was zero.
  static const int32_t kResCtx = 6;
  PalCtr repeat[2];        // this entry repeats the previous one
  PalCtr channel_eq[2];    // this channel repeats the one before it in the entry
  PalCtr res[kResCtx][kTree];
  uint8_t last_zero[3];

  void Init() {
    for( int32_t k = 0; k<2; ++k ) {
      repeat[k].Init();
      channel_eq[k].Init();
    }
    for( int32_t c = 0; c<kResCtx; ++c )
      for( int32_t k = 0; k<kTree; ++k )
        res[c][k].Init();
    last_zero[0] = 1;
    last_zero[1] = 1;
    last_zero[2] = 1;
  }

  template <int32_t f_DEC> int32_t code_byte(RangeCoder* rc, int32_t ctx, int32_t v) {
    PalCtr*const tree = res[ctx];
    int32_t node = 1;
    for( int32_t k = 7; k>=0; --k )
      node = 2*node+tree[node].template code<f_DEC>(rc, (v>>k)&1);
    return node&255;
  }

  // Codes `entries` three-byte entries in place.  The predictor is the previous
  // entry in the same channel, which turns a ramp or a gradient into a constant
  // residual; the two flags in front of it take the two shapes that a plain
  // residual handles badly -- an unused tail of identical entries, and a
  // greyscale entry whose three channels are equal.
  template <int32_t f_DEC> void code(RangeCoder* rc, uint8_t* pal, int32_t entries) {
    Init();
    uint8_t prev[3] = {0, 0, 0};
    int32_t prev_repeat = 0;
    for( int32_t i = 0; i<entries; ++i ) {
      uint8_t*const e = pal+3*i;
      if( i>0 ) {
        int32_t same = 0;
        if constexpr( !f_DEC )
          same = (e[0]==prev[0])&(e[1]==prev[1])&(e[2]==prev[2]);
        same = repeat[prev_repeat].template code<f_DEC>(rc, same);
        prev_repeat = same;
        if( same ) {
          if constexpr( f_DEC ) {
            e[0] = prev[0];
            e[1] = prev[1];
            e[2] = prev[2];
          }
          continue;
        }
      }
      for( int32_t c = 0; c<3; ++c ) {
        if( c>0 ) {
          int32_t eq = 0;
          if constexpr( !f_DEC )
            eq = e[c]==e[c-1];
          eq = channel_eq[c-1].template code<f_DEC>(rc, eq);
          if( eq ) {
            if constexpr( f_DEC )
              e[c] = e[c-1];
            last_zero[c] = (uint8_t)(e[c]==prev[c]);
            continue;
          }
        }
        int32_t d = 0;
        if constexpr( !f_DEC )
          d = (e[c]-prev[c])&255;
        d = code_byte<f_DEC>(rc, 2*c+last_zero[c], d);
        if constexpr( f_DEC )
          e[c] = (uint8_t)(prev[c]+d);
        last_zero[c] = (uint8_t)(d==0);
      }
      prev[0] = e[0];
      prev[1] = e[1];
      prev[2] = e[2];
    }
  }
};
