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
// So this is a coding mode, offered to planes of at most sixteen values and
// chosen by the same per-plane trial that chooses between the other three.
// Four context orders are kept side by side -- four neighbours up to as many as
// the alphabet allows -- and their probabilities are mixed in the stretch
// domain with weights learned per (evidence, tree node), then refined through
// an adaptive probability map.
//
// Included from bmf.cpp; not a standalone translation unit.

struct CtxModel {
  // Causal neighbours, in the order an entropy sweep over the corpus found them
  // useful: W, N, NW, NE, WW, NN, NEE, NWW, and outward from there.
  static const int32_t kMaxNeighbours = 24;
  static const int32_t kOrders = 4;
  static const int32_t kShortNb = 4;
  static const int32_t kTableBits = 20;
  static const int32_t kMidBits = 18;
  // A context here sees tens of samples, not thousands, so a fixed adaptation
  // rate is the wrong shape: it is far too slow while a bucket is new.  Each
  // node carries its own update count and adapts at 1/(n+2), annealing to a
  // fixed floor -- the schedule the KT estimator implies.
  static const int32_t kCountCap = 60;
  static const int32_t kBlend = 24;
  static const int32_t kMaxAlpha = 16;

  uint8_t to_idx[256];
  uint8_t to_val[kMaxAlpha];
  int32_t alpha;
  int32_t bits;
  int32_t nodes;
  int32_t nb_for_bits[5];
  int32_t nb[kOrders];
  int32_t table_bits[kOrders];
  uint32_t* ctr[kOrders];
  uint32_t* tags[kOrders];
  int32_t recip[kCountCap+1];
  Apm apm;
  Mix4 mixer;
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
    // The longest neighbourhood, by alphabet.  The key packs one index per
    // neighbour, so how many fit is bounded -- but the useful count is bounded
    // well below that, and for different reasons at each size: at four values
    // the table saturates, at eight and sixteen the learning cost turns it
    // around first, at two values it keeps paying out to 24.
    nb_for_bits[0] = 0;
    nb_for_bits[1] = 24;
    nb_for_bits[2] = 10;
    nb_for_bits[3] = 7;
    nb_for_bits[4] = 7;
    alpha = 0;
    bits = 0;
    nodes = 0;
    for( int32_t k = 0; k<kOrders; ++k ) {
      nb[k] = 0;
      table_bits[k] = 0;
      ctr[k] = nullptr;
      tags[k] = nullptr;
    }
    apm.Init();
    mixer.Init();
    cx = nullptr;
  }

  void set_alphabet(int32_t a) {
    alpha = a;
    bits = 1;
    while( (1<<bits)<alpha )
      ++bits;
    nodes = (1<<bits)-1;
    // Four orders spread evenly from four neighbours to the longest.  Order 0
    // is direct-indexed and never evicted; the rest are hashed.
    const int32_t longest = nb_for_bits[bits];
    for( int32_t k = 0; k<kOrders; ++k ) {
      nb[k] = kShortNb+(longest-kShortNb)*k/(kOrders-1);
      table_bits[k] = k==0 ? bits*kShortNb : (k==kOrders-1 ? kTableBits : kMidBits);
    }
    for( int32_t v = 0; v<256; ++v )
      to_idx[v] = 0;
    for( int32_t k = 0; k<alpha; ++k )
      to_idx[to_val[k]] = (uint8_t)k;
    // Start the mixer where a count-weighted blend would be: a weight set whose
    // longest bucket has seen n samples begins at n/(n+24) on that table and
    // splits the rest between the shorter ones.  Descent finds this in a few
    // thousand samples anyway, but the sets for a barely-filled bucket see the
    // fewest samples and are the ones that matter most.
    for( int32_t b = 0; b<8; ++b ) {
      const int32_t n = 8*b+4;
      int32_t weights[kOrders];
      for( int32_t k = 0; k<kOrders-1; ++k )
        weights[k] = 65536*kBlend/(n+kBlend)/(kOrders-1);
      weights[kOrders-1] = 65536*n/(n+kBlend);
      for( int32_t node = 0; node<16; ++node )
        mixer.seed((b<<4)|node, weights);
    }
  }

  void alloc_tables() {
    for( int32_t k = 0; k<kOrders; ++k ) {
      const size_t count = (size_t)1<<table_bits[k];
      ctr[k] = (uint32_t*)bmf_new(count*(size_t)nodes*sizeof(uint32_t));
      if( k ) {
        tags[k] = (uint32_t*)bmf_new(count*sizeof(uint32_t));
        memset(tags[k], 0, count*sizeof(uint32_t));
      } else {
        for( size_t j = 0; j<count*(size_t)nodes; ++j )
          ctr[0][j] = 32768u<<16;
      }
    }
  }

  void Quit() {
    for( int32_t k = 0; k<kOrders; ++k ) {
      free(ctr[k]);
      free(tags[k]);
      ctr[k] = nullptr;
      tags[k] = nullptr;
    }
  }

  // The context key: the neighbours' alphabet indices packed `bits` apiece,
  // nearest first, so a shorter order is a prefix of a longer one.
  uint32_t context_key(const uint8_t* px, int32_t w, int32_t h, int32_t x, int32_t y) const {
    static const int8_t dx[kMaxNeighbours] = {-1, 0, -1, 1, -2, 0, 2, -2, -1, 1, -3, 0, 2, -2, 3, -3, -4, 0, 1, -1, 3, -3, 4, -4};
    static const int8_t dy[kMaxNeighbours] = {0, -1, -1, -1, 0, -2, -1, -1, -2, -2, 0, -3, -2, -2, -1, -1, 0, -4, -3, -3, -2, -2, -1, -1};
    uint32_t key = 0;
    for( int32_t k = 0; k<nb[kOrders-1]; ++k ) {
      const int32_t nx = x+dx[k], ny = y+dy[k];
      const uint32_t v = (nx<0||ny<0||nx>=w||ny>=h) ? 0u : to_idx[px[(size_t)ny*w+nx]];
      key |= v<<(bits*k);
    }
    return key;
  }

  // A direct-mapped table with a tag: a bucket whose tag does not match is
  // reset rather than shared, so a collision costs one context's history
  // instead of mixing two contexts' statistics together.  A fresh bucket starts
  // from the next shorter order rather than from an even coin.
  uint32_t*bucket(int32_t k, uint32_t key, const uint32_t* seed) {
    if( !k )
      return &ctr[0][(size_t)(key&(((uint32_t)1<<table_bits[0])-1u))*(size_t)nodes];
    const uint32_t hashed = key*2654435761u;
    const uint32_t slot = hashed>>(32-table_bits[k]);
    const uint32_t want = hashed|1u;
    uint32_t*const row = &ctr[k][(size_t)slot*(size_t)nodes];
    if( tags[k][slot]!=want ) {
      tags[k][slot] = want;
      for( int32_t j = 0; j<nodes; ++j )
        row[j] = seed[j]&0xFFFF0000u;
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

  template <int32_t f_DEC> int32_t code_symbol(uint32_t** rows, int32_t sym) {
    int32_t node = 1;
    for( int32_t b = bits-1; b>=0; --b ) {
      const int32_t at = node-1;
      int32_t p_in[kOrders];
      for( int32_t k = 0; k<kOrders; ++k )
        p_in[k] = (int32_t)(rows[k][at]>>16);
      const int32_t n_long = (int32_t)(rows[kOrders-1][at]&0xFFFFu);
      const int32_t p = mixer.mix(p_in, ((n_long>>3)<<4)|at);
      // The map's context is the tree node and nothing else: the mixer has
      // already used everything else available here.
      const int32_t coded = apm.refine(p, at);
      int32_t bit;
      if constexpr( f_DEC ) {
        bit = cx[0].decode_bit(65536-coded, coded);
      } else {
        bit = (sym>>b)&1;
        cx[0].encode_bit(65536-coded, coded, bit);
      }
      apm.update(bit);
      mixer.update(bit, p);
      const int32_t target = bit ? 65535 : 0;
      for( int32_t k = 0; k<kOrders; ++k )
        bump(rows[k][at], target);
      node = 2*node+bit;
    }
    return node-(1<<bits);
  }

  // The alphabet goes on the wire ahead of the plane: one flat symbol for its
  // size and one per value.  At most seventeen bytes, against a plane the mode
  // only wins on when it is worth thousands.
  template <int32_t f_DEC> void code_alphabet() {
    if constexpr( f_DEC ) {
      const int32_t a = (int32_t)cx[0].decode_flat(kMaxAlpha)+1;
      set_alphabet(a);
      for( int32_t k = 0; k<alpha; ++k )
        to_val[k] = (uint8_t)cx[0].decode_flat(256);
      set_alphabet(a);
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
        uint32_t* rows[kOrders];
        for( int32_t k = 0; k<kOrders; ++k ) {
          const uint32_t sub = nb[k]>=nb[kOrders-1] ? key : key&(((uint32_t)1<<(bits*nb[k]))-1u);
          rows[k] = bucket(k, sub, k ? rows[k-1] : nullptr);
        }
        if constexpr( f_DEC )
          row[x] = to_val[code_symbol<1>(rows, 0)];
        else
          code_symbol<0>(rows, to_idx[row[x]]);
      }
    }
    Quit();
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }
};
