// bmf_state.inc -- BMFState: coder-facing state, the symbol tree and the shared model tables
// Included from bmf.cpp; not a standalone translation unit.

struct FreqPair {
  uint16_t f[2];
};

uint32_t halve_pair(FreqPair* pair, int32_t go) {
  halve_counts(pair[0].f, 2);
  return pair[0].f[go];
}

void end_plane_stream() {
}

// Derives from RangeCoder: the coder and the stream it writes into are part of
// the coder-facing state, so models holding a BMFState* call encode/decode
// directly instead of reaching through a member.
struct BMFState : RangeCoder {
  alignas(16) CodedTail* coded_block;
  alignas(16) int32_t plane_predictor;
  alignas(16) int32_t plane_alt_model;
  int32_t desc_slow_mode;
  int32_t alphabet_reduced;
  PlaneDesc plane_desc[4];
  int32_t plane_count;
  int32_t near_lossless_q;
  uint8_t model_geometry[128];
  int8_t exclusion_gen;
  alignas(16) int8_t exclusion_mask[(no_symbol+1+15)&~15];
  int32_t mode_symbol[5];
  int32_t alt_freq_limit;
  int32_t alt_freq_init;
  LevelGeom level_geom[8];
  int32_t ctx_bias[4];
  int32_t deadzone_hi;
  int32_t deadzone_lo;
  uint8_t* hist_scratch;
  // ---- Plot mode -----------------------------------------------------------
  //
  // `bmf p in.bmp out.bmp` codes the image exactly as `bmf c` does and writes,
  // instead of the stream, one byte per colour component holding what that
  // component cost: 4.4 fixed point, four integer bits of codelength and four
  // fractional, so 0x28 is 2.5 bits.  The coding is the encoder's own, sample
  // for sample -- plot is f_DEC==2 and every branch that asks "am I decoding?"
  // answers no -- so the numbers are the ones the shipped stream is made of.
  uint8_t* plot_map;          // one byte per component, or null when not plotting
  uint16_t plot_w, plot_h;    // the geometry the coding used
  uint8_t plot_flags, plot_pad;
  const uint8_t* plot_origin; // the buffer the model walks
  uint32_t plot_len;          // components in plot_map
  int32_t plot_base;          // component of the first sample the model codes
  int32_t plot_stride;        // components between one sample and the next

  // Point the map at the buffer a model is about to walk.  Interleaved coding
  // walks the image itself, so base 0 and stride 1; planar coding walks one
  // plane's own buffer, so the plane's offset and one pixel's worth of stride.
  void plot_walk(const uint8_t* origin, int32_t base, int32_t stride) {
    plot_origin = origin;
    plot_base = base;
    plot_stride = stride;
    plot_bits = 0.0;
  }

  // Charge everything coded since the last call to n components from `idx`.
  // A run coded as one symbol spreads its cost evenly over the pixels it
  // covers, which is what makes a run-coded flat region read as flat rather
  // than as one expensive pixel followed by free ones.
  void plot_note(int32_t idx, int32_t n) {
    const double each = plot_bits/(double)(n<1 ? 1 : n);
    plot_bits = 0.0;
    if( !plot_map )
      return;
    // 4.4 saturates at 15.9375 bits, which is beyond what a byte-valued
    // component can cost, so only a pathological sample clips.
    const double q = each*16.0+0.5;
    const uint8_t v = (uint8_t)(q<=0.0 ? 0 : (q>=255.0 ? 255 : (int32_t)q));
    for( int32_t k = 0; k<n; ++k ) {
      const int64_t at = (int64_t)plot_base+(int64_t)(idx+k)*(int64_t)plot_stride;
      if( at>=0&&(uint64_t)at<(uint64_t)plot_len )
        plot_map[at] = v;
    }
  }

  void plot_note_ptr(const uint8_t* p, int32_t n) {
    plot_note((int32_t)(p-plot_origin), n);
  }

  alignas(16) uint16_t model_table_store[kModelTableBytes/sizeof(uint16_t)];
  uint16_t* model_tables;
  void hist_bump(int32_t k) {
    uint32_t n;
    memcpy(&n, &hist_scratch[4*k], sizeof n);
    ++n;
    memcpy(&hist_scratch[4*k], &n, sizeof n);
  }

  // Every model reaches its 254-entry frequency strip through here, and the
  // index is a context assembled from IDX layout parameters.  One bound at the
  // choke point covers alt-P1, alt-P2, the counters and the slow model; without
  // it a swept field width sends the pointer past the arena.
  // Both from bmf_util.inc, where the arena is sized as strips*CtxPair_Volume;
  // reading them back rather than restating 254 keeps the cut and the size the
  // same statement.
  static const uint32_t kModelStrips = kModelTableBytes/(kModelStripSize*sizeof(uint16_t));
  uint16_t*model_strip(uint32_t k) {
    return model_tables+kModelStripSize*idx_bound(k, kModelStrips);
  }

  template <int32_t f_DEC> int32_t code_symbol_tree(uint16_t* freq, int32_t sym) {
    uint16_t add;
    uint32_t cum, j;
    int32_t lvl, result, go, span;
    uint16_t fq, * slot;
    uint16_t* f0 = freq+2;
    uint32_t tot = freq[0];
    if constexpr( f_DEC==1 ) {
      uint32_t target = get_freq(tot);
      slot = f0;
      lvl = 0;
      cum = f0[0];
      if( cum<=target ) {
        do {
          ++slot;
          ++lvl;
          cum += slot[0];
        } while( cum<=target );
      }
      decode(cum-slot[0], cum, tot);
    } else {
      lvl = model_geometry[sym];
      cum = 0;
      for( j = 0; j<(uint32_t)lvl; j++ )
        cum += f0[j];
      slot = &f0[lvl];
      uint32_t cum_hi = cum+slot[0];
      result = encode(cum, cum_hi, tot);
    }
    if( freq[0]>0x4000u ) {
      halve_counts(freq+2, 8);
      uint16_t total = 0;
      for( int32_t k = 2; k<10; ++k )
        total += freq[k];

      int32_t esc = freq[1];
      freq[0] = total;
      if( esc<=4*alt_freq_limit ) {
        result = 4*(esc>alt_freq_limit);
        add = esc-result;
        freq[1] = add;
      } else {
        add = esc-16;
        freq[1] = add;
      }
    } else {
      add = freq[1];
    }
    slot[0] += add;
    freq[0] += freq[1];
    if( lvl<2 )
      return lvl;
    int32_t mask = level_geom[lvl].half;
    int32_t path = 0;
    if constexpr( f_DEC!=1 )
      path = sym-level_geom[lvl].first;
    int32_t node = 0;
    for( span = 1;; span *= 2 ) {
      FreqPair* pair = bit_tree(freq, lvl)+span+node;
      int32_t f1 = pair[0].f[1];
      int32_t fa = pair[0].f[0];
      if constexpr( f_DEC==1 ) {
        go = decode_bit(fa, f1);
      } else {
        go = (mask&path)!=0;
        result = encode_bit(fa, f1, go);
      }
      fq = pair[0].f[go];
      if( fq>0x4000u ) fq = halve_pair(pair, go);
      pair[0].f[go] = alt_freq_init+fq;
      mask >>= 1;
      node = go+2*node;
      if( !mask ) return node+level_geom[lvl].first;
    }
  }
  void encode_symbol_tree(uint16_t* freq, int32_t sym) {
    code_symbol_tree<0>(freq, sym);
  }
  int32_t decode_symbol_tree(uint16_t* freq) {
    return code_symbol_tree<1>(freq, 0);
  }
  int32_t update_binary_pair(uint16_t* _this, int32_t symbol) {
    int32_t node;
    uint32_t step, f;
    int32_t tot = _this[0];
    if( (uint32_t)tot<=0x8000 ) {
      int32_t lvl = model_geometry[symbol];
      step = (_this[1]>>2)&0xFFFFFFE0;
      if( plane_predictor==pred_p2 )
        step = 15*(_this[1]>>5);
      _this[lvl+2] += step+4;
      tot = _this[0]+step+4;
      _this[0] = tot;
      if( symbol>=2 ) {
        int32_t mask = level_geom[lvl].half;
        int32_t path = symbol-level_geom[lvl].first;
        node = 0;
        FreqPair* tbl = bit_tree(_this, lvl);
        int32_t span = 1;
        do {
          FreqPair* pair = tbl+node+span;
          int32_t go = (mask&path)!=0;
          f = pair[0].f[go];
          if( f>0x2000 )
            f = halve_pair(pair, go);
          span *= 2;
          mask >>= 1;
          node = go+2*node;
          pair[0].f[go] = f+((alt_freq_init*((uint32_t)(plane_predictor==pred_p2)+5))>>3);
        } while( mask );
        return 0;
      }
    }
    return tot;
  }
  uint32_t alt_init_tables(uint8_t* fold, int8_t* unfold) {
    uint8_t even, odd;
    uint32_t done;
    int32_t in_bucket, bucket;
    uint8_t half;
    uint32_t i, b;

    int32_t bucket_size = 2*near_lossless_q+1;
    unfold[0] = 0;
    unfold[255] = 0x80;
    for( i = 0; i<0x3F; ++i ) {
      unfold[4*i+2] = (int8_t)(2*i+1);
      unfold[4*i+1] = (int8_t)(-2*i-1);
      unfold[4*i+4] = (int8_t)(2*i+2);
      unfold[4*i+3] = (int8_t)(-2*i-2);
    }
    unfold[254] = 127;
    unfold[253] = -127;
    fold[0] = 0;
    uint32_t lo = 1;
    fold[128] = (uint8_t)-1;
    {
      uint32_t span = 128-lo;
      uint32_t pairs = (128-lo)/2;
      half = 0;
      in_bucket = 1;
      {
        uint32_t k = 0;
        int32_t ofs = 0;
        uint8_t* pos = &fold[lo];
        uint8_t* neg = fold-lo;
        for(; k<pairs; ++k, ofs -= 2 ) {
          bucket = bucket_size;
          int32_t bucket_1 = in_bucket-1;
          if( bucket_1 )
            bucket = bucket_1;
          else
            ++half;
          even = 2*half;
          pos[2*k] = even;
          odd = 2*half-1;
          in_bucket = bucket-1;
          neg[ofs+256] = odd;
          if( bucket==1 ) {
            in_bucket = bucket_size;
            even = 2*++half;
            odd = 2*half-1;
          }
          pos[2*k+1] = even;
          neg[ofs+255] = odd;
        }
        done = 2*k+1;
      }
      if( (done-1)<span ) {
        if( in_bucket==1 ) half = half+1;
        fold[lo-1+done] = 2*half;
        fold[257-lo-done] = 2*half-1;
      }
    }
    for( b = 0; b<0x80; ++b ) {
      fold[(uint8_t)((uint8_t*)unfold)[2*b]+256] = 2*b;
      fold[(uint8_t)((uint8_t*)unfold)[2*b+1]+256] = 2*b+1;
    }
    return b;
  }
  FreqPair*bit_tree(uint16_t* freq, int32_t lvl) {
  return (FreqPair*)&freq[2*level_geom[lvl].tbl_base+8];
  }

  uint16_t*begin_plane_stream() {
    if( plane_predictor==pred_p2 ) {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    } else {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    level_geom[2].half = 1;
    level_geom[2].first = 2;
    level_geom[2].tbl_base = 0;
    memset(&model_geometry[2], 2, 2);
    int32_t slot = 4;
    for( int32_t lvl = 3; lvl<8; ++lvl ) {
      level_geom[lvl].half = 1<<(lvl-2);
      level_geom[lvl].first = (uint8_t)slot;
      level_geom[lvl].tbl_base = (uint8_t)slot-lvl;
      memset(&model_geometry[slot], lvl, 2*level_geom[lvl].half);
      slot += 2*level_geom[lvl].half;
    }
    uint16_t* tbl = model_table_store;
    if( tbl ) {
      int32_t k = 0;
      uint16_t* row = tbl;
      const uint16_t seed_row[10] = {
        635, (uint16_t)(24*alt_freq_limit), 205, 124, 147, 83, 48, 16, 8, 4,
      };
      for( k = 0; k<0x400; ++k, row += 254 ) {
        memcpy(row, seed_row, sizeof seed_row);
        FreqPair* seed = (FreqPair*)&row[10];
        for( uint32_t i = 0; i<0x7A; ++i ) {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
      }
    }
    model_tables = tbl;
    return tbl;
  }
  // These six stay here rather than in RangeCoder: a plane's range-coded
  // segment is bracketed by model work the coder knows nothing about — handing
  // the shared buffer over from the bit packer, and seeding/retiring the
  // per-plane symbol tables.  The pure coder primitives they call
  // (enc_init/dec_init/flush/finish) are inherited.
  template <int32_t f_DEC> uint16_t*rc_begin() {
    if constexpr( f_DEC!=1 ) stream.pk.word[0] = stream.pk.acc;
    stream.packer_rewind();
    uint16_t* tbl = plane_alt_model ? begin_plane_stream() : nullptr;
    if constexpr( f_DEC==1 )
      dec_init();
    else
      enc_init();
    return tbl;
  }
  void rc_begin_decode() {
    rc_begin<1>();
  }
  uint16_t*rc_begin_encode() {
    return rc_begin<0>();
  }
  template <int32_t f_DEC> void rc_end() {
    if constexpr( f_DEC==1 )
      finish();
    else
      flush();
    end_plane_stream();
  }
  void rc_end_decode() {
    rc_end<1>();
  }
  void rc_end_encode() {
    rc_end<0>();
  }
  bool ref_transformed(int32_t k) {
    return (plane_desc[plane_desc[k].src_plane].flags&desc_has_refs)!=0;
  }
  alignas(16) float p2_coef[7][4];
  alignas(16) float p2_rate[7][4];
  SymEntry byte_list_ent[16*256];
  SymEntry gap_list_ent[257];
  void Init() {
    memset(static_cast<void*>(this), 0, sizeof(BMFState));
    bmf_set_denormal_mode();
    memcpy(p2_coef, bmf_p2_coef_init, sizeof p2_coef);
    memcpy(p2_rate, bmf_p2_rate_init, sizeof p2_rate);
  }

  BMFState &operator=(const BMFState &) = delete;
};
