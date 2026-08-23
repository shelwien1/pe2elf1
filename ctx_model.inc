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
  static const int32_t kMaxNeighbours = CM_max_neighbours;
  static const int32_t kOrders = CM_orders;
  static const int32_t kInputs = kOrders+1;
  // Table widths are heap-allocated from a runtime count, so unlike the
  // sizes above they do not have to be constant expressions and stay live
  // for the optimizer.  The ceilings are what bounds the sweep's memory: the
  // longest order costs 4*nodes bytes per row, so 22 bits is about 250 MB and
  // 26 would be four gigabytes -- the clamp is the contract, and here it is
  // the difference between a slow tuning pass and an OOM.
  // Declared here, defined below the class.  In the tuning build an IDX value
  // is a read from a patchable object, and an in-class initializer has to be a
  // constant expression; an out-of-class definition may be dynamic.  The
  // shipping build initializes the same names from literals and folds them.
  static const int32_t kMatchBits;
  static const int32_t kMatchLens = CM_match_lens;
  static const int32_t kShortNb;
  static const int32_t kTableBits;
  static const int32_t kMidBits;
  // A context here sees tens of samples, not thousands, so a fixed adaptation
  // rate is the wrong shape: it is far too slow while a bucket is new.  Each
  // node carries its own update count and adapts at 1/(n+2), annealing to a
  // fixed floor -- the schedule the KT estimator implies.
  // recip[] is sized by the frozen ceiling; the cap itself stays live
  // underneath it, so the optimizer can move the anneal point without the
  // member array's size following it.
  static const int32_t kCountCapMax = CM_count_cap_max;
  static const int32_t kCountCap;
  static const int32_t kBlend;
  static const int32_t kMaxAlpha = CM_max_alpha;

  uint8_t to_idx[256];
  uint8_t to_val[kMaxAlpha];
  int32_t alpha;
  int32_t bits;
  int32_t nodes;
  int32_t nb_for_bits[5];
  int32_t chosen_nb;
  int32_t nb[kOrders];
  int32_t table_bits[kOrders];
  uint32_t* ctr[kOrders];
  uint32_t* tags[kOrders];
  int32_t recip[kCountCapMax+1];
  Apm apm;
  Mix5 mixer;
  int32_t* match_tab;
  uint32_t match_p[kMatchLens];
  uint64_t win;
  uint64_t win_mask;
  int32_t match_nb;
  int32_t match_ptr;
  int32_t match_len;
  int32_t match_pred;
  bool match_path;
  BMFState* cx;
  uint64_t measured;

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
    for( int32_t k = 0; k<=kCountCapMax; ++k )
      recip[k] = 65536/(k+idx_clamp(CM_adapt_offset, 1, 64));
    // The longest neighbourhood, by alphabet.  The key packs one index per
    // neighbour, so how many fit is bounded -- but the useful count is bounded
    // well below that, and for different reasons at each size: at four values
    // the table saturates, at eight and sixteen the learning cost turns it
    // around first, at two values it keeps paying out to 24.
    nb_for_bits[0] = 0;
    // Clamped to the neighbour table's length: these index dx[]/dy[], and the
    // probe multiplies them up before set_alphabet sees them.
    nb_for_bits[1] = idx_clamp(CM_nb_for_bits_1, 1, kMaxNeighbours);
    nb_for_bits[2] = idx_clamp(CM_nb_for_bits_2, 1, kMaxNeighbours);
    nb_for_bits[3] = idx_clamp(CM_nb_for_bits_3, 1, kMaxNeighbours);
    nb_for_bits[4] = idx_clamp(CM_nb_for_bits_4, 1, kMaxNeighbours);
    chosen_nb = 0;
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
    match_tab = nullptr;
    for( int32_t k = 0; k<kMatchLens; ++k )
      match_p[k] = (uint32_t)CM_match_seed<<16;
    win = 0;
    win_mask = 0;
    match_nb = 0;
    match_ptr = -1;
    match_len = 0;
    match_pred = 0;
    match_path = false;
    cx = nullptr;
    measured = 0;
  }

  void set_alphabet(int32_t a) {
    alpha = a;
    bits = 1;
    while( (1<<bits)<alpha )
      ++bits;
    nodes = (1<<bits)-1;
    // Four orders spread evenly from four neighbours to the longest.  Order 0
    // is direct-indexed and never evicted; the rest are hashed.
    // The match model's window, in pixels: long enough that a hit means a real
    // repeat rather than a common local pattern the context tables already have.
    match_nb = CM_match_span/bits;
    if( match_nb>CM_match_cap )
      match_nb = CM_match_cap;
    win_mask = match_nb*bits>=64 ? ~0ull : (((uint64_t)1<<(match_nb*bits))-1);
    const int32_t longest = idx_clamp(chosen_nb ? chosen_nb : nb_for_bits[bits],
                                     kShortNb, kMaxNeighbours);
    for( int32_t k = 0; k<kOrders; ++k ) {
      nb[k] = kShortNb+(longest-kShortNb)*k/(kOrders-1);
      table_bits[k] = k==0 ? bits*kShortNb : (k==kOrders-1 ? kTableBits : kMidBits);
    }
    for( int32_t v = 0; v<256; ++v )
      to_idx[v] = 0;
    for( int32_t k = 0; k<alpha; ++k )
      to_idx[to_val[k]] = (uint8_t)k;
  }


  // Everything that adapts, back to where a fresh plane starts.  The encoder
  // prices several neighbourhood lengths before it codes, and each of those
  // passes leaves state behind; the decoder only ever makes the coding pass, so
  // every pass has to begin identically or the two walk different models.
  void reset_state() {
    apm.Init();
    mixer.Init();
    // Start the mixer where a count-weighted blend would be: a weight set whose
    // longest bucket has seen n samples begins at n/(n+24) on that table and
    // splits the rest between the shorter ones.  Descent finds this in a few
    // thousand samples anyway, but the sets for a barely-filled bucket see the
    // fewest samples and are the ones that matter most.  The match model starts
    // with no weight at all: it has to earn it.
    for( int32_t b = 0; b<8; ++b ) {
      const int32_t n = (b<<CM_nbucket_shift)+(1<<idx_clamp(CM_nbucket_shift-1, 0, 12));
      int32_t weights[kInputs];
      for( int32_t k = 0; k<kOrders-1; ++k )
        weights[k] = 65536*kBlend/(n+kBlend)/(kOrders-1);
      weights[kOrders-1] = 65536*n/(n+kBlend);
      weights[kOrders] = 0;
      for( int32_t node = 0; node<16; ++node )
        mixer.seed((b<<4)|node, weights);
    }
    for( int32_t k = 0; k<kMatchLens; ++k )
      match_p[k] = (uint32_t)CM_match_seed<<16;
    win = 0;
    match_ptr = -1;
    match_len = 0;
    match_pred = 0;
    match_path = false;
  }

  void alloc_tables() {
    match_tab = (int32_t*)bmf_new(((size_t)1<<kMatchBits)*sizeof(int32_t));
    memset(match_tab, 0xFF, ((size_t)1<<kMatchBits)*sizeof(int32_t));
    for( int32_t k = 0; k<kOrders; ++k ) {
      const size_t count = (size_t)1<<table_bits[k];
      ctr[k] = (uint32_t*)bmf_new(count*(size_t)nodes*sizeof(uint32_t));
      if( k ) {
        tags[k] = (uint32_t*)bmf_new(count*sizeof(uint32_t));
        memset(tags[k], 0, count*sizeof(uint32_t));
      } else {
        for( size_t j = 0; j<count*(size_t)nodes; ++j )
          ctr[0][j] = (uint32_t)CM_cell_seed<<16;
      }
    }
  }

  void Quit() {
    free(match_tab);
    match_tab = nullptr;
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
    const uint32_t hashed = key*(uint32_t)CM_key_hash;
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

  // Before coding position `at`: is there a symbol to predict, and does the
  // tree path start out agreeing with it?
  void match_begin(const uint8_t* px, int32_t at) {
    match_path = match_ptr>=0&&match_ptr<at&&match_len>0;
    match_pred = match_path ? to_idx[px[match_ptr]] : 0;
  }

  // After coding it: extend the match if it held, otherwise look up where this
  // window last occurred, and record that this window now ends here.
  void match_update(const uint8_t* px, int32_t at) {
    const int32_t idx = to_idx[px[at]];
    win = ((win<<bits)|(uint64_t)(uint32_t)idx)&win_mask;
    const uint32_t slot = (uint32_t)((win*0x9E3779B97F4A7C15ull)>>(64-kMatchBits));
    if( match_ptr>=0&&match_ptr<at&&to_idx[px[match_ptr]]==idx ) {
      ++match_ptr;
      ++match_len;
    } else {
      match_len = 0;
      match_ptr = match_tab[slot];
    }
    match_tab[slot] = at+1;
  }

  void bump(uint32_t &cell, int32_t target) {
    int32_t p = (int32_t)(cell>>16);
    int32_t n = (int32_t)(cell&0xFFFFu);
    p += (int32_t)(((int64_t)(target-p)*recip[n])>>16);
    n += n<kCountCap;
    cell = ((uint32_t)p<<16)|(uint32_t)n;
  }

  // f_MEASURE prices the plane without running the coder, so a choice that has
  // to go on the wire can be made on a crop for a fraction of a trial encode.
  template <int32_t f_DEC, int32_t f_MEASURE> int32_t code_symbol(uint32_t** rows, int32_t sym) {
    int32_t node = 1;
    const int32_t len_at = match_len<kMatchLens ? match_len : kMatchLens-1;
    for( int32_t b = bits-1; b>=0; --b ) {
      const int32_t at = node-1;
      int32_t p_in[kInputs];
      for( int32_t k = 0; k<kOrders; ++k )
        p_in[k] = (int32_t)(rows[k][at]>>16);
      // The match model's opinion: while the tree path still agrees with the
      // symbol that followed the last occurrence of this window, predict that
      // symbol's next bit, with a confidence learned per match length.  Once
      // the path diverges it says nothing.
      const int32_t pred_bit = match_path ? (match_pred>>b)&1 : 0;
      const int32_t conf = (int32_t)(match_p[len_at]>>16);
      p_in[kOrders] = !match_path ? 32768 : pred_bit ? conf : 65536-conf;
      const int32_t n_long = (int32_t)(rows[kOrders-1][at]&0xFFFFu);
      const int32_t p = mixer.mix(p_in, ((n_long>>idx_clamp(CM_nbucket_shift, 1, 12))<<4)|at);
      // The map's context is the tree node and nothing else: the mixer has
      // already used everything else available here.
      const int32_t coded = apm.refine(p, at);
      int32_t bit;
      if constexpr( f_MEASURE ) {
        bit = (sym>>b)&1;
        measured += (uint64_t)bit_cost(coded, bit);
      } else if constexpr( f_DEC ) {
        bit = cx[0].decode_bit(65536-coded, coded);
      } else {
        bit = (sym>>b)&1;
        cx[0].encode_bit(65536-coded, coded, bit);
      }
      apm.update(bit);
      mixer.update(bit, p);
      if( match_path ) {
        bump(match_p[len_at], bit==pred_bit ? 65535 : 0);
        match_path = bit==pred_bit;
      }
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
      chosen_nb = (int32_t)cx[0].decode_flat(kMaxNeighbours)+1;
      set_alphabet(a);
      for( int32_t k = 0; k<alpha; ++k )
        to_val[k] = (uint8_t)cx[0].decode_flat(256);
      set_alphabet(a);
    } else {
      cx[0].encode((uint32_t)alpha-1, (uint32_t)alpha, kMaxAlpha);
      cx[0].encode((uint32_t)nb[kOrders-1]-1u, (uint32_t)nb[kOrders-1], kMaxNeighbours);
      for( int32_t k = 0; k<alpha; ++k )
        cx[0].encode(to_val[k], (uint32_t)to_val[k]+1u, 256);
    }
  }

  // Which neighbourhood length to use, priced on a bounded centred crop rather
  // than assumed.  Unlike orientation (which is a whole-raster property and
  // cannot be decided on a crop), this is a question about local statistics,
  // and a megapixel answers it as well as nine.  The winner goes on the wire.
  static const int32_t kProbePixels;
  int32_t choose_neighbours(BMFState* state, const uint8_t* px, int32_t w, int32_t h) {
    bit_cost_init();
    // Bands spread over the whole raster rather than one centred block: a
    // centre crop of x_ai does not look like x_ai, and picking on one costs
    // that file 2000 bytes.  Full width, so only the band joins break a
    // vertical context.
    const int32_t bands = idx_clamp(CM_probe_bands, 1, 64);
    const int32_t pw = w;
    int32_t ph = h;
    if( (int64_t)pw*ph>kProbePixels )
      ph = (int32_t)(kProbePixels/pw);
    if( ph<bands*2 )
      ph = h<bands*2 ? h : bands*2;
    if( ph>h )
      ph = h;
    const int32_t per = ph/bands;
    uint8_t*const crop = (uint8_t*)bmf_new((size_t)pw*ph);
    int32_t at = 0;
    for( int32_t b = 0; b<bands&&at<ph; ++b ) {
      const int32_t rows = b==bands-1 ? ph-at : per;
      const int32_t from = h<=ph ? at : (int32_t)((int64_t)b*(h-rows)/(bands-1));
      for( int32_t y = 0; y<rows&&at<ph; ++y, ++at )
        memcpy(crop+(size_t)at*pw, px+(size_t)(from+y)*w, (size_t)pw);
    }
    ph = at;
    const int32_t longest = nb_for_bits[bits];
    const int32_t want[4] = {longest,
                             (longest*CM_probe_num_1+CM_probe_round_1)/idx_clamp(CM_probe_den_1, 1, 64),
                             (longest*CM_probe_num_2+CM_probe_round_2)/idx_clamp(CM_probe_den_2, 1, 64),
                             (longest*CM_probe_num_3+CM_probe_round_3)/idx_clamp(CM_probe_den_3, 1, 64)};
    int32_t best = longest;
    uint64_t best_cost = ~(uint64_t)0;
    for( int32_t k = 0; k<4; ++k ) {
      const int32_t n = idx_clamp(want[k], 2, kMaxNeighbours);
      if( k&&n==want[k-1] )
        continue;
      chosen_nb = n;
      set_alphabet(alpha);
      measured = 0;
      code_plane<0, 1>(state, crop, pw, ph);
      bmf_log("    ctx model: %2d neighbours prices the crop at %.1f bytes\n", n, (double)measured/2048.0);
      // The probe sees a fraction of the plane, and a longer context needs more
      // data to pay off, so the probe is biased toward short ones.  A shorter
      // candidate has to win by more than 1/64 to be believed.
      const uint64_t priced = k ? measured+(measured>>idx_clamp(CM_probe_bias, 1, 20)) : measured;
      if( priced<best_cost ) {
        best_cost = priced;
        best = n;
      }
    }
    free(crop);
    chosen_nb = best;
    set_alphabet(alpha);
    return best;
  }

  template <int32_t f_DEC, int32_t f_MEASURE = 0> void code_plane(BMFState* state, uint8_t* px, int32_t w, int32_t h) {
    cx = state;
    if constexpr( !f_MEASURE ) {
      if constexpr( f_DEC )
        cx[0].rc_begin_decode();
      else
        cx[0].rc_begin_encode();
      code_alphabet<f_DEC>();
    }
    reset_state();
    alloc_tables();
    for( int32_t y = 0; y<h; ++y ) {
      uint8_t*const row = px+(size_t)y*w;
      for( int32_t x = 0; x<w; ++x ) {
        const int32_t at = (int32_t)((size_t)y*w+x);
        match_begin(px, at);
        const uint32_t key = context_key(px, w, h, x, y);
        uint32_t* rows[kOrders];
        for( int32_t k = 0; k<kOrders; ++k ) {
          const uint32_t sub = nb[k]>=nb[kOrders-1] ? key : key&(((uint32_t)1<<(bits*nb[k]))-1u);
          rows[k] = bucket(k, sub, k ? rows[k-1] : nullptr);
        }
        if constexpr( f_DEC&&!f_MEASURE )
          row[x] = to_val[code_symbol<1, 0>(rows, 0)];
        else
          code_symbol<0, f_MEASURE>(rows, to_idx[row[x]]);
        match_update(px, at);
      }
    }
    Quit();
    if constexpr( !f_MEASURE ) {
      if constexpr( f_DEC )
        cx[0].rc_end_decode();
      else
        cx[0].rc_end_encode();
    }
  }
};

const int32_t CtxModel::kMatchBits = idx_clamp(CM_match_bits, 8, 24);
const int32_t CtxModel::kShortNb = idx_clamp(CM_short_nb, 1, CM_max_neighbours);
const int32_t CtxModel::kTableBits = idx_clamp(CM_table_bits, 8, 22);
const int32_t CtxModel::kMidBits = idx_clamp(CM_mid_bits, 8, 22);
const int32_t CtxModel::kCountCap = idx_clamp(CM_count_cap, 1, CtxModel::kCountCapMax);
const int32_t CtxModel::kBlend = CM_blend;
const int32_t CtxModel::kProbePixels = 1<<idx_clamp(CM_probe_shift, 12, 24);
