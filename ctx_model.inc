// ctx_model.inc -- CtxModel: a direct context model for small-alphabet planes.
//
// The slow model reaches a pixel by ranking its four causal neighbours and
// coding which one matched, escaping to a candidate scan when none did.  That
// is the right shape for continuous-tone data, where a neighbour's *value* is a
// good prediction and the alphabet is too large to condition on directly.  It
// is the wrong shape for a plane with a handful of distinct values -- a
// screenshot, a map, a dithered or posterised image -- where the useful thing
// is not "which neighbour matched" but the joint pattern of what the
// neighbours *are*.
//
// Measured on this corpus, as a static conditional entropy plus the
// Krichevsky-Trofimov learning cost, against what the codec ships today:
//
//   x_ci (4 values, 4.5 Mpixel)   613.4 KiB shipped   566.3 KiB at 7 neighbours
//   x_ai (8 values, 4.5 Mpixel)   141.2 KiB shipped   120.4 KiB at 7 neighbours
//
// So this is a coding mode, offered to planes of at most sixteen values and
// chosen by the same per-plane trial that chooses between the other three.
//
// Included from bmf.cpp; not a standalone translation unit.

struct CtxModel {
  // Causal neighbours, in the order the entropy sweep found useful: W, N, NW,
  // NE, WW, NN, NEE.  Anything off the raster reads as index 0.
  static const int32_t kMaxNeighbours = 24;
  int32_t neighbours;
  int32_t nb_for_bits[5];
  static const int32_t kTableBits = 20;
  // A four-neighbour table sits under the long one, direct-indexed and never
  // evicted.  A fresh long bucket is seeded from it rather than from an even
  // coin: a context here sees tens of samples, so where it starts matters as
  // much as how fast it moves.  Both tables are updated on every sample.
  static const int32_t kShortNb = 4;
  int32_t short_size;
  static const int32_t kTableSize = 1<<kTableBits;
  // A context here sees tens of samples, not thousands, so a fixed adaptation
  // rate is the wrong shape: it is far too slow while a bucket is new.  Each
  // node carries its own update count and adapts at 1/(n+2), annealing to a
  // fixed floor -- the same schedule the KT estimator implies, which is what
  // the entropy sweep above was measured against.
  static const int32_t kCountCap = 60;
  static const int32_t kBlend = 24;
  static const int32_t kMaxAlpha = 16;

  uint8_t to_idx[256];
  uint8_t to_val[kMaxAlpha];
  int32_t alpha;
  int32_t bits;
  int32_t nodes;
  uint32_t* ctr;
  uint32_t* ctr_short;
  Apm apm;
  int32_t recip[kCountCap+1];
  uint32_t* tag;
  BMFState* cx;

  // The distinct values in a plane, in ascending order.  Returns how many there
  // are, which may exceed kMaxAlpha -- the caller decides.
  static int32_t scan_alphabet(const uint8_t* p, int32_t n, uint8_t* vals) {
    uint8_t seen[256];
    memset(seen, 0, sizeof seen);
    for( int32_t i = 0; i<n; ++i )
      seen[p[i]] = 1;
    int32_t a = 0;
    for( int32_t v = 0; v<256; ++v )
      if( seen[v] ) {
        if( a<kMaxAlpha )
          vals[a] = (uint8_t)v;
        ++a;
      }
    return a;
  }

  void Init() {
    for( int32_t k = 0; k<=kCountCap; ++k )
      recip[k] = 65536/(k+2);
    apm.Init();
    neighbours = 0;
    short_size = 0;
    nb_for_bits[0] = 0;
    nb_for_bits[1] = 24;
    nb_for_bits[2] = 10;
    nb_for_bits[3] = 7;
    nb_for_bits[4] = 7;
    alpha = 0;
    bits = 0;
    nodes = 0;
    ctr = nullptr;
    ctr_short = nullptr;
    tag = nullptr;
    cx = nullptr;
  }

  void set_alphabet(int32_t a) {
    alpha = a;
    bits = 1;
    while( (1<<bits)<alpha )
      ++bits;
    nodes = (1<<bits)-1;
    // The neighbourhood grows as the alphabet shrinks: a binary plane can
    // afford far more neighbours per context than a sixteen-value one, and the
    // useful count is not simply "as many as the key can hold" -- eight
    // neighbours lose to seven at four and eight values, because the extra
    // contexts cost more to learn than they save.
    neighbours = nb_for_bits[bits];
    short_size = 1<<(bits*kShortNb);
    for( int32_t v = 0; v<256; ++v )
      to_idx[v] = 0;
    for( int32_t k = 0; k<alpha; ++k )
      to_idx[to_val[k]] = (uint8_t)k;
  }

  void alloc_tables() {
    ctr = (uint32_t*)bmf_new((size_t)kTableSize*(size_t)nodes*sizeof(uint32_t));
    tag = (uint32_t*)bmf_new((size_t)kTableSize*sizeof(uint32_t));
    memset(tag, 0, (size_t)kTableSize*sizeof(uint32_t));
    ctr_short = (uint32_t*)bmf_new((size_t)short_size*(size_t)nodes*sizeof(uint32_t));
    for( size_t k = 0; k<(size_t)short_size*(size_t)nodes; ++k )
      ctr_short[k] = 32768u<<16;
  }

  void Quit() {
    free(ctr);
    free(ctr_short);
    free(tag);
    ctr = nullptr;
    ctr_short = nullptr;
    tag = nullptr;
  }

  // The context key: the neighbours' alphabet indices packed four bits apiece.
  uint32_t context_key(const uint8_t* px, int32_t w, int32_t h, int32_t x, int32_t y) const {
    static const int8_t dx[kMaxNeighbours] = {-1, 0, -1, 1, -2, 0, 2, -2, -1, 1, -3, 0, 2, -2, 3, -3, -4, 0, 1, -1, 3, -3, 4, -4};
    static const int8_t dy[kMaxNeighbours] = {0, -1, -1, -1, 0, -2, -1, -1, -2, -2, 0, -3, -2, -2, -1, -1, 0, -4, -3, -3, -2, -2, -1, -1};
    uint32_t key = 0;
    for( int32_t k = 0; k<neighbours; ++k ) {
      const int32_t nx = x+dx[k], ny = y+dy[k];
      const uint32_t v = (nx<0||ny<0||nx>=w||ny>=h) ? 0u : to_idx[px[(size_t)ny*w+nx]];
      key |= v<<(bits*k);
    }
    return key;
  }

  // A direct-mapped table with a tag: a bucket whose tag does not match is
  // reset rather than shared, so a collision costs one context's history
  // instead of mixing two contexts' statistics together.
  uint32_t*bucket(uint32_t key, const uint32_t* seed) {
    const uint32_t hashed = key*2654435761u;
    const uint32_t slot = hashed>>(32-kTableBits);
    const uint32_t want = hashed|1u;
    uint32_t*const row = &ctr[(size_t)slot*(size_t)nodes];
    if( tag[slot]!=want ) {
      tag[slot] = want;
      for( int32_t k = 0; k<nodes; ++k )
        row[k] = seed[k]&0xFFFF0000u;
    }
    return row;
  }

  void bump(uint32_t &cell, int32_t target) {
    int32_t p = (int32_t)(cell>>16);
    int32_t n = (int32_t)(cell&0xFFFFu);
    p += (int32_t)(((int64_t)(target-p)*recip[n])>>16);
    n += n<kCountCap;
    cell = ((uint32_t)p<<16)|(uint32_t)n;
  }

  template <int32_t f_DEC> int32_t code_symbol(uint32_t* row, uint32_t* srow, int32_t sym) {
    int32_t node = 1;
    for( int32_t b = bits-1; b>=0; --b ) {
      const int32_t at = node-1;
      // Blend the two tables by how much the long context has seen: a fresh
      // bucket codes almost entirely from the short one and hands over as it
      // fills.  Seeding alone throws away the short context's later evidence.
      const int32_t p_long = (int32_t)(row[at]>>16);
      const int32_t n_long = (int32_t)(row[at]&0xFFFFu);
      const int32_t p_short = (int32_t)(srow[at]>>16);
      int32_t p = (p_long*n_long+p_short*kBlend)/(n_long+kBlend);
      if( p<32 )
        p = 32;
      if( p>65504 )
        p = 65504;
      // The map's context is how much the long bucket has seen and which node
      // of the tree this is -- neither of which the two tables are keyed on.
      const int32_t coded = apm.refine(p, at);
      int32_t bit;
      if constexpr( f_DEC ) {
        bit = cx[0].decode_bit(65536-coded, coded);
      } else {
        bit = (sym>>b)&1;
        cx[0].encode_bit(65536-coded, coded, bit);
      }
      apm.update(bit);
      const int32_t target = bit ? 65535 : 0;
      bump(row[at], target);
      bump(srow[at], target);
      node = 2*node+bit;
    }
    return node-(1<<bits);
  }

  // The alphabet goes on the wire ahead of the plane: one flat symbol for its
  // size and one per value.  At most seventeen bytes, against a plane the mode
  // only wins on when it is worth thousands.
  template <int32_t f_DEC> void code_alphabet() {
    if constexpr( f_DEC ) {
      set_alphabet((int32_t)cx[0].decode_flat(kMaxAlpha)+1);
      for( int32_t k = 0; k<alpha; ++k )
        to_val[k] = (uint8_t)cx[0].decode_flat(256);
      set_alphabet(alpha);
    } else {
      cx[0].encode((uint32_t)alpha-1, (uint32_t)alpha, kMaxAlpha);
      for( int32_t k = 0; k<alpha; ++k )
        cx[0].encode(to_val[k], (uint32_t)to_val[k]+1u, 256);
    }
  }

  template <int32_t f_DEC> void code_plane(BMFState* state, uint8_t* px, int32_t w, int32_t h) {
    cx = state;
    if constexpr( f_DEC )
      cx[0].rc_begin_decode();
    else
      cx[0].rc_begin_encode();
    code_alphabet<f_DEC>();
    alloc_tables();
    for( int32_t y = 0; y<h; ++y ) {
      uint8_t*const row = px+(size_t)y*w;
      for( int32_t x = 0; x<w; ++x ) {
        const uint32_t key = context_key(px, w, h, x, y);
        uint32_t*const srow = &ctr_short[(size_t)(key&(uint32_t)(short_size-1))*(size_t)nodes];
        uint32_t*const cell = bucket(key, srow);
        if constexpr( f_DEC )
          row[x] = to_val[code_symbol<1>(cell, srow, 0)];
        else
          code_symbol<0>(cell, srow, to_idx[row[x]]);
      }
    }
    Quit();
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }
};
