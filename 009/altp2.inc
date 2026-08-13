// altp2.inc -- the alternate model for deep planes: its counters, its frequency
// records and the block they live in.

struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};

struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way() {
    uint32_t cut, budget;
    f[0] -= f[0]>>1;
    f[1] -= f[1]>>1;
    cut = f[2]>>1;
    f[2] -= cut;
    budget = step;
    if( budget<=0x100 ) {
      if( step<=0x20u )
        cut = ((16-budget)>>30)&0xFFFFFFFE;
      else
        cut = 32;
      step = budget-cut;
    } else {
      step >>= 1;
    }
    return cut;
  }

  // One symbol of the deep model's three-way counter, and then the tree.  The
  // encoder is handed a symbol and picks the slot it names; the decoder is
  // handed a point in the range and finds the slot containing it.  They agree
  // on `idx = slot - base`, which is what makes the tail one piece of code:
  // slot 0 is the symbol, and slots 1 and 2 escape into the level tree with
  // `idx & 1` choosing the context and `idx` the parity of what comes back.
  //
  // The encoder said `ctx_pair[sym & 1]` and `(sym - 1) >> 1` where this says
  // `ctx_pair[idx & 1]`; they are the same, because `idx` is 1 for odd symbols
  // and 2 for even ones above zero.
  template<int f_DEC>
  int32_t code_symbol(const uint32_t* ctx_pair, int32_t sym) {
    uint16_t f_before;
    uint32_t cum;
    uint16_t st, * slot, * base;
    uint32_t tot, tot_1, f1_old, f2_old;
    uint16_t down;
    int32_t idx, target, c01;
    base = &f[0];
    c01 = f[1]+f[0];
    tot = c01+f[2];
    if( f_DEC ) {
      target = rc.get_freq(tot);
      cum = f[0];
      if( (uint32_t)target>=cum ) {
        if( target>=c01 ) {
          cum = c01;
          slot = &f[2];
        } else {
          slot = &f[1];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    } else {
      cum = c01;
      if( sym ) {
        if( sym&1 ) {
          cum = f[0];
          slot = &f[1];
        } else {
          slot = &f[2];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    }
    tot_1 = cum+*slot;
    if( f_DEC )
      rc.decode(cum, tot_1, tot);
    else
      rc.encode(cum, tot_1, tot);
    f_before = *slot;
    if( *slot>0x4000u ) {
      f1_old = f[1];
      f2_old = f[2];
      f[0] -= f[0]>>1;
      f[1] = f1_old-(f1_old>>1);
      f[2] = f2_old-(f2_old>>1);
      st = step;
      if( st<=256 ) {
        if( st<=32 )
          down = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
        else
          down = 32;
        st = st-down;
        step = st;
        f_before = *slot;
      } else {
        st = (uint32_t)st>>1;
        step = st;
        f_before = *slot;
      }
    } else {
      st = step;
    }
    *slot = st+f_before;
    idx = slot-base;
    if( idx )
      return idx+2*__code_symbol_tree<f_DEC>(model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(const uint32_t* ctx_pair, int32_t sym) {
    code_symbol<0>(ctx_pair, sym);
  }

  int32_t decode_symbol(const uint32_t* ctx_pair) {
    return code_symbol<1>(ctx_pair, 0);
  }
};

static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<((rate+31)&31)))>>(rate&31);
}

static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>deadzone_hi)-(err<deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

struct P2Ctx {
  int16_t val;
  int16_t dval;
  int16_t err;
  int16_t aerr;
  int16_t dleft;
  int16_t dup;
  int16_t dupleft;
  int16_t dupright;
  uint8_t sign;
  uint8_t mag;
};

struct AltP2Block {
  float nb_weights[1088][16][4];
  union {
    float p2_row[7][4];
    struct {
      uint8_t p2_row0_head[15];
      uint8_t _u0_0_4[97];
      float bias[4];
      float (*nb_cur)[4];
      float (**row0)[4];
      float (**row1)[4];
      float (**cur)[4];
      float (**above)[4];
      int32_t bank_ctx[5];
      int32_t _unused278696;
      uint16_t pred_prev;
      uint8_t _u0_0_12[2];
      uint32_t ctx;
      uint32_t ctx_pair[2];
      int32_t nb_id_used;
      int32_t band_lo;
      int32_t band_hi;
      uint32_t plane_idx;
      int32_t has_ref;
      P2Ctx* cursor[5];
      P2Ctx* buf[5];
      CtxWeight ctx_w[5];
      uint8_t _u0_0_tail[8];
    };
  };
  uint8_t _pad1[40];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  int16_t nb_id[1916];
  uint8_t _pad2[8];
  P2Count p2_ctr[163840];
  P2Freq freq[15552];
  uint8_t _pad3[8];
  // Defined out of line, in alt_p2_context.inc and alt_p2_model.inc.  They are
  // 400 and 1100 lines and belong in their own files, which is what the split
  // is for; every other method in this tree is defined in its class body
  // because every other method fits there.  They were free functions taking
  // the block as a first argument, which is what a method is.
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb);
  uint32_t alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in);

  AltP2Block*alt_p2_alloc(int32_t img_w, int32_t plane) {
    uint32_t done, e;
    void* buf1;
    int32_t dz, band, row_bytes, lvl, lvl1, bump, len, len1;
    uint32_t j, k, ctr, pairs, p, row, pair, n;
    plane_idx = plane;
    for( j = 0; j<0x14000; ++j ) {
      p2_ctr[2*j].weighted = 0;
      p2_ctr[2*j+1].weighted = 0;
    }
    for( k = 0; k<0x14000; ++k ) {
      p2_ctr[2*k].rate = 5;
      p2_ctr[2*k].b1 = 2;
      p2_ctr[2*k+1].rate = 5;
      p2_ctr[2*k+1].b1 = 2;
    }
    ctr = 0;
    do {
      e = 2*ctr;
      freq[e].f[0] = 2048;
      ++ctr;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
      freq[e+1].f[0] = 2048;
      freq[e+1].f[1] = 2816;
      freq[e+1].f[2] = 2816;
      freq[e+1].step = 4096;
    } while( ctr<0x1E60 );
    dz = 4*near_lossless_q+1;
    band = 16*near_lossless_q;
    has_ref = (uint8_t)(plane_desc[plane_desc[plane_idx+1].src_plane+1].flags&8)>>3;
    deadzone_hi = dz;
    deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = (float (**)[4])bmf_new(sizeof(float (*)[4])*(img_w+4));
    buf1 = bmf_new(sizeof(float (*)[4])*(img_w+4));
    nb_weights[0][14][2] = 1.0f;
    row1 = (float (**)[4]) buf1;
    cur = row0+img_w+2;
    if( img_w>-4 ) {
      pairs = (img_w+4)/2;
      if( pairs ) {
        for( p = 0; p<pairs; ++p ) {
          row1[2*p] = nb_weights[0];
          row0[2*p] = nb_weights[0];
          row1[2*p+1] = nb_weights[0];
          row0[2*p+1] = nb_weights[0];
        }
        done = 2*p+1;
      } else {
        done = 1;
      }
      if( (uint32_t)(img_w+4)>(done-1) ) {
        row1[done-1] = nb_weights[0];
        row0[done-1] = nb_weights[0];
      }
    }
    row = 0;
    row_bytes = 18*(img_w<3 ? 3 : img_w)+234;
    do
      buf[row++] = (P2Ctx*)bmf_new(row_bytes);
    while( row<5 );
    memset(buf[0], 0, row_bytes);
    ctx_bias[3] = 0;
    lvl = 0;
    ctx_bias[2] = 0;
    ctx_bias[1] = 0;
    pair = 0;
    ctx_bias[0] = 0;
    cursor[0] = buf[0]+8;
    do {
      ctx_delta[2*pair] = (plane_idx<<8)|(16*lvl);
      lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = (plane_idx<<8)|(16*lvl1);
      bump = 2*pair+++1==p2_ctx_edges[lvl1];
      lvl = bump+lvl1;
    } while( pair<0x82 );
    len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      len1 = (2*n==p2_len_edges[len])+len;
      nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    ctx = 15;
    __alt_init_tables(fold, unfold);
    ctx_w[4].w[0] = 0;
    ctx_w[0].w[1] = 64;
    ctx_w[3].w[0] = 0;
    ctx_w[0].w[2] = 128;
    ctx_w[2].w[0] = 0;
    ctx_w[1].w[1] = 192;
    ctx_w[1].w[0] = 0;
    ctx_w[1].w[2] = 384;
    ctx_w[0].w[0] = 0;
    ctx_w[2].w[1] = 576;
    ctx_w[2].w[2] = 1152;
    ctx_w[3].w[1] = 1728;
    ctx_w[3].w[2] = 3456;
    ctx_w[4].w[1] = 5184;
    ctx_w[4].w[2] = 10368;
    return this;
  }

  AltP2Block*alt_p2_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    free(row0);
    free(row1);
    if( do_free&1 )
      bmf_page_free(this);
    return this;
  }
};

struct CtxWeights {
  float (*f0[6])[4];
};
