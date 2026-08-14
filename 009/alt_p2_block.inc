// alt_p2_block.inc -- the alternate model for predictor 2: its counters, its frequency
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
    uint32_t cut;
    f[0] -= f[0]>>1;
    f[1] -= f[1]>>1;
    cut = f[2]>>1;
    f[2] -= cut;
    uint32_t budget = step;
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
  // Named apart from `SymList::code_symbol`, which is a different coder in a
  // different model.  Two methods with one name is not an error the compiler
  // can see, and it made `unstale.py` measure the wrong one.
  template<int f_DEC>
  int32_t code_three_way(const uint32_t* ctx_pair, int32_t sym) {
    uint16_t f_before;
    uint32_t cum;
    uint16_t st, *slot;
    uint16_t down;
    uint16_t *base = &f[0];
    int32_t c01 = f[1]+f[0];
    uint32_t tot = c01+f[2];
    if( f_DEC ) {
      int32_t target = rc.get_freq(tot);
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
    uint32_t tot_1 = cum+*slot;
    if( f_DEC )
      rc.decode(cum, tot_1, tot);
    else
      rc.encode(cum, tot_1, tot);
    f_before = *slot;
    if( *slot>0x4000u ) {
      uint32_t f1_old = f[1];
      uint32_t f2_old = f[2];
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
    int32_t idx = slot-base;
    if( idx )
      return idx+2*__code_symbol_tree<f_DEC>(model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(const uint32_t* ctx_pair, int32_t sym) {
    code_three_way<0>(ctx_pair, sym);
  }

  int32_t decode_symbol(const uint32_t* ctx_pair) {
    return code_three_way<1>(ctx_pair, 0);
  }
};

static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<((rate+31)&31)))>>(rate&31);
}

static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>deadzone_hi)-(err<deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

// One counter update: predict from the weight, bump it by how wrong that was.
// 115 of `alt_p2_model`'s 117 `p2_bump` sites were these three statements
// written out, with a pair of temporaries each.
//
// Eight of those 115 read the rate as `*(uint8_t*)&p->rate`, and `rate` is
// `int8_t` -- so they took the byte unsigned where this takes it signed.  That
// is the same value here and only here: `p2_pred` uses the rate as two shift
// counts and masks both with `&31`, and the two readings differ by 256, which
// is a multiple of 32.
static inline void p2_update(P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p->weighted;
  p->weighted = p2_bump(w, res-p2_pred(w, p->rate), shift);
}

// The same counter, updated in place at a fixed shift of 2 and *without*
// `p2_bump`'s dead-zone kick.  21 sites fold it into one `+=` on the next
// counter in the bank; the rounding term is the only thing they share with
// `p2_update`, and folding them together would put the kick where the original
// has none.
// One count of a three-way record, bumped by `num` sixteenths of the record's
// step, with the rescale that keeps the three counts from overflowing.  35 of
// the 41 sites in `alt_p2_model` wrote both steps out.
//
// It takes the numerator rather than the increment because `rescale_three_way`
// steps `step` down as well as halving the counts, and the bump reads `step`
// *after* it.  Passing a computed increment would read it before.
//
// The increments in the file are 2, 3, 4, 5, 6, 7, 10 and 13 sixteenths and
// nothing else, which is only visible once they are written as sixteenths:
// `(step&0xFFFC)>>2` and `(step&0xFFF8)>>3` are `(4*step)>>4` and `(2*step)>>4`,
// the masks clearing bits the shift discards anyway.
// The rescale on its own.  Two of `alt_p2_model`'s fifty sites compute the
// increment and then add it *conditionally*, so the add cannot move into the
// helper; the guard still can.
static inline void p2_rescale(P2Freq* r) {
  if( r->f[0]+r->f[1]+r->f[2]>29696 )
    r->rescale_three_way();
}

static inline void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num) {
  p2_rescale(r);
  r->f[slot] += (num*(uint32_t)r->step)>>4;
}

static inline void p2_nudge(P2Count* p, int32_t res) {
  *(uint16_t*)&p->weighted += (uint32_t)(res-p2_pred(p->weighted, p->rate)+2)>>2;
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
      int32_t _pad0;
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
  // 6 bands * 320.  Declared `[1916]` with the four entries after it split off
  // as `uint8_t _pad2[8]` -- the recovery could not see them indexed, and the
  // index reaches them: `63 + 64*gA + 320*band` with `gA` 0..4 and `band` 0..5
  // is 1919 at its largest, which eight of the seventeen corpus images hit on
  // every run.  The overrun landed in the padding and stopped there, exactly,
  // because four `int16_t` is eight bytes.  Same size, same offsets, and the
  // accesses are in bounds now.  CONTEXT-INDEX.md section 1.
  int16_t nb_id[1920];
  P2Count p2_ctr[163840];
  P2Freq freq[15552];
  uint8_t _pad3[8];
  // Defined out of line, in alt_p2_context.inc and alt_p2_model.inc.  They are
  // 400 and 1100 lines and belong in their own files, which is what the split
  // is for; every other method in this tree is defined in its class body
  // because every other method fits there.  They were free functions taking
  // the block as a first argument, which is what a method is.

  // The NLMS filter, in two halves, and each was a bare braced block inside
  // `alt_p2_model`'s 674 lines.
  //
  // A weight row is `float w[16][4]`: rows 0..6 are the filter taps, rows 7..13
  // the running mean square of each tap's input -- which is what makes this
  // *normalised* least mean squares, each tap's step divided by the energy it
  // has been seeing -- and row 14 holds three scalars the two halves share.
  //
  // Both halves are written as `[7][4]` nests rather than 28 scalars because
  // that is the shape the original's SSE was in; keeping it is what lets a
  // compiler put it back.

  // Track the input energy and step both rows' weights toward their own error.
  // Two rows, not one: `cur` adapts fast against this sample and `prev` slowly
  // against the confidence-scaled one, and the pair is the mixer.
  void nlms_track_two_rows(float (*cur)[4], float (*prev)[4],
                           float err_cur, float err_prev, float conf) {
    const float floor_cur = 26896.0f*cur[14][2];
    const float floor_prev = 5041.0f*prev[14][2];
    const float rate_prev = 0.013f*conf;
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k ) {
        const float x = p2_row[j][k];
        float ms = cur[7+j][k]+(x*x-cur[7+j][k])*0.05f;
        cur[7+j][k] = ms;
        cur[j][k] += bmf_p2_rate[j][k]*err_cur*x/(ms+floor_cur);
        ms = prev[7+j][k]+(x*x-prev[7+j][k])*rate_prev;
        prev[7+j][k] = ms;
        prev[j][k] += bmf_p2_rate[j][k]*err_prev*x/(ms+floor_prev);
      }
  }

  // Predict this sample from the fast row, then step that row toward the error
  // it just made.  `w[14][2]` decays toward 10 and scales the step floor, so
  // the filter starts cautious and loosens; `w[15][0]` counts the samples this
  // row has seen and is an `int32_t` living in a `float` slot.
  void nlms_predict_and_correct(float (*w)[4], float sample, float bias0) {
    float acc[4];
    for( int32_t k = 0; k<4; ++k ) {
      acc[k] = w[0][k]*p2_row[0][k];
      for( int32_t j = 1; j<7; ++j )
        acc[k] += w[j][k]*p2_row[j][k];
    }
    const float err = sample-(bias0+bmf_hsum4(acc));
    const float ms_scale = w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        w[j][k] += bmf_p2_rate[j][k]*err*p2_row[j][k]/(w[7+j][k]+ms_scale*529.0f);
    ++*(int32_t*)&w[15][0];
    w[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }

  template<int f_DEC>
  void alt_p2_d8_body(uint8_t* src, uint8_t* out,
                      int32_t width, int32_t height);
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb);
  uint32_t alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in);

  AltP2Block*alt_p2_alloc(int32_t img_w, int32_t plane) {
    uint32_t done;
    int32_t lvl, len;
    uint32_t j, p, n;
    plane_idx = plane;
    // One pass, not two 2x-unrolled ones.  The original walked the 163,840
    // counters twice -- once zeroing `weighted`, once seeding `rate` and `b1`
    // -- with each loop doing two records an iteration.  This runs once per
    // plane, and the difference is not measurable: 9.7s either way on `x_ep`.
    for( j = 0; j<0x28000; ++j ) {
      p2_ctr[j].weighted = 0;
      p2_ctr[j].rate = 5;
      p2_ctr[j].b1 = 2;
    }
    for( uint32_t e = 0; e<0x3CC0; ++e ) {
      freq[e].f[0] = 2048;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
    }
    int32_t dz = 4*near_lossless_q+1;
    int32_t band = 16*near_lossless_q;
    has_ref = (uint8_t)(plane_desc[plane_desc[plane_idx].src_plane].flags&8)>>3;
    deadzone_hi = dz;
    deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = (float (**)[4])bmf_new(sizeof(float (*)[4])*(img_w+4));
    void* buf1 = bmf_new(sizeof(float (*)[4])*(img_w+4));
    nb_weights[0][14][2] = 1.0f;
    row1 = (float (**)[4]) buf1;
    cur = row0+img_w+2;
    if( img_w>-4 ) {
      uint32_t pairs = (img_w+4)/2;
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
    uint32_t row = 0;
    int32_t row_bytes = 18*(img_w<3 ? 3 : img_w)+234;
    do
      buf[row++] = (P2Ctx*)bmf_new(row_bytes);
    while( row<5 );
    memset(buf[0], 0, row_bytes);
    ctx_bias[3] = 0;
    lvl = 0;
    ctx_bias[2] = 0;
    ctx_bias[1] = 0;
    uint32_t pair = 0;
    ctx_bias[0] = 0;
    cursor[0] = buf[0]+8;
    do {
      ctx_delta[2*pair] = (plane_idx<<8)|(16*lvl);
      int32_t lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = (plane_idx<<8)|(16*lvl1);
      int32_t bump = 2*pair+++1==p2_ctx_edges[lvl1];
      lvl = bump+lvl1;
    } while( pair<0x82 );
    len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      int32_t len1 = (2*n==p2_len_edges[len])+len;
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
