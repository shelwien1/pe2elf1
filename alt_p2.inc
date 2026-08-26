// alt_p2.inc -- AltP2Block: the NLMS predictor, bias-bank cascade and residual coder (predictor 2)
// Included from bmf.cpp; not a standalone translation unit.

// The bias banks fan out over the set bits of kBankMirrorMask, highest first.
// The fan width and the eleven bit values used to be twelve more numbers
// declared beside the mask -- which is the shape of defect CD_weight_bits was:
// values carrying no gradient of their own that an edit or a sweep can put out
// of step with the one thing they must agree with.  Derived from the mask they
// cannot disagree with it.  (P2_fan_mirror, how many of these positions also
// take the mirror update, is a real parameter and stays one.)
struct BankFan {
  uint32_t bit[32];
  int32_t n;
};

static constexpr BankFan bank_fan_of(uint32_t mask) {
  BankFan f{};
  for( int32_t i = 31; i>=0; --i )
    if( mask&(1u<<i) )
      f.bit[f.n++] = 1u<<i;
  return f;
}

static constexpr BankFan kBankFanBits = bank_fan_of(kBankMirrorMask);
static constexpr int32_t kBankFan = kBankFanBits.n;
static const uint32_t*const p2_bank_bits = kBankFanBits.bit;
static_assert(kBankFan>0, "the bias banks need at least one bit to mirror over");

// Seats the sign map keeps, one per low cell context.
static const int32_t kSeatCount = 64;

// Pack a bank's binary tests into the word its mask reads, most significant
// first -- the same shift-or the Index used to spell as N `ADD 2` factors.
// Moving it here is the whole point: as separate factors the tests are fixed,
// as one masked word the optimizer can drop any of them, and a dropped bit lets
// the compiler delete the comparison that fed it.
template <class... B> static inline uint32_t p2_flags(B... bits) {
  uint32_t v = 0;
  ((v = (v<<1)|((uint32_t)bits&1u)), ...);
  return v;
}

struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};

struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way() {
    uint32_t cut = f[2]>>1;
    halve_counts(f, 3);
    uint32_t budget = step;
    if( budget<=(uint32_t)P2_step_hi ) {
      if( step<=(uint32_t)P2_step_lo )
        cut = budget>(uint32_t)P2_step_tiny ? P2_step_down_lo : 0;
      else
        cut = P2_step_down_hi;
      step = budget-cut;
    } else {
      step >>= 1;
    }
    return cut;
  }

  template <int32_t f_DEC> int32_t code_three_way(BMFState* cx, const uint32_t* ctx_pair, int32_t sym, Apm* apm,
                                                 int32_t apm_ctx, int32_t mix_ctx, Mix4* mixer, uint32_t* seat, uint32_t* signs, uint32_t* fast,
                                                 Apm* sign_apm, int32_t sign_ctx) {
    uint16_t st, * slot;
    uint16_t down;
    uint16_t*const base = &f[0];
    const uint32_t tot = (uint32_t)f[0]+f[1]+f[2];
    int32_t p16 = (int32_t)(((uint32_t)f[0]*65536u+(tot>>1))/tot);
    if( p16<kProbFloor )
      p16 = kProbFloor;
    if( p16>kProbCeil )
      p16 = kProbCeil;
    // Three estimates of the same decision, mixed: this cell, and two coarse
    // views of it -- one keyed on the seat and level alone, one on the
    // neighbours' sign classes alone.  The cell is keyed on both together and
    // so is the sparsest of the three; the coarse ones carry it while it fills.
    const int32_t p_in[4] = {p16, adapt_p(seat[0]), adapt_p(signs[0]), (int32_t)(fast[0]>>16)};
    const int32_t p_mix = mixer[0].mix(p_in, mix_ctx);
    const int32_t p_ref = apm[0].refine(p_mix, apm_ctx);
    int32_t zero;
    if constexpr( f_DEC ) {
      zero = cx[0].decode_bit(65536-p_ref, p_ref);
    } else {
      zero = sym==0;
      cx[0].encode_bit(65536-p_ref, p_ref, zero);
    }
    apm[0].update(zero);
    mixer[0].update(zero, p_mix);
    const int32_t zero_target = zero ? 65535 : 0;
    adapt_bump(seat[0], zero_target);
    adapt_bump(signs[0], zero_target);
    // A second, deliberately fast estimate of the same cell: the frequency
    // triple anneals as it fills, so it tracks a change of texture slowly.  One
    // fixed-rate probability beside it is section 4's dual-rate counter, and
    // the mixer decides how much to believe it.
    fast[0] = (uint32_t)((int32_t)(fast[0]>>16)+((zero_target-(int32_t)(fast[0]>>16))>>idx_clamp(P2_fast_shift, 1, 15)))<<16;
    if( zero ) {
      slot = &f[0];
    } else {
      int32_t hi_class;
      // Which side of zero the residual fell on.  The cell's two counts are
      // close to even on continuous tone -- 0.89 to 0.97 bits a symbol on the
      // LPCB photographs, a quarter of everything the model spends -- because
      // every context axis this cell has is built from *bands* of the running
      // prediction and from neighbour signs, and none of them can see which way
      // the prediction leaned inside the pixel it was rounded to.
      //
      // predict() carries run4 at 16x and seat_symbol_context rounds it,
      // pred = (run4+7)>>4.  The four bits that rounding throws away are a
      // direct statement about the sign: run4 just above a pixel boundary means
      // the true value sits below the prediction, and the residual leans
      // negative.  Keep them and refine the cell's estimate through a map keyed
      // on them, the way section 3 of IMPROVEMENTS.md does the zero decision.
      {
        const uint32_t f12 = (uint32_t)f[1]+(uint32_t)f[2];
        int32_t p16 = (int32_t)(((uint32_t)f[2]*65536u+(f12>>1))/f12);
        if( p16<kProbFloor ) p16 = kProbFloor;
        if( p16>kProbCeil ) p16 = kProbCeil;
        const int32_t p_ref = sign_apm[0].refine(p16, sign_ctx);
        if constexpr( f_DEC ) {
          hi_class = cx[0].decode_bit(65536-p_ref, p_ref);
        } else {
          hi_class = !(sym&1);
          cx[0].encode_bit(65536-p_ref, p_ref, hi_class);
        }
        sign_apm[0].update(hi_class);
      }
      slot = hi_class ? &f[2] : &f[1];
    }
    uint16_t f_before = slot[0];
    if( slot[0]>(uint32_t)P2_cell_cap ) {
      halve_counts(f, 3);
      st = step;
      if( st<=P2_step_hi ) {
        if( st<=P2_step_lo )
          down = st>P2_step_tiny ? P2_step_down_lo : 0;
        else
          down = P2_step_down_hi;
        st = st-down;
        step = st;
        f_before = slot[0];
      } else {
        st = (uint32_t)st>>1;
        step = st;
        f_before = slot[0];
      }
    } else {
      st = step;
    }
    slot[0] = st+f_before;
    int32_t idx = slot-base;
    if( idx )
      return idx+2*cx[0].template code_symbol_tree<f_DEC>(cx[0].model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(BMFState* cx, const uint32_t* ctx_pair, int32_t sym, Apm* apm, int32_t apm_ctx, int32_t mix_ctx,
                     Mix4* mixer, uint32_t* seat, uint32_t* signs, uint32_t* fast, Apm* sign_apm, int32_t sign_ctx) {
    code_three_way<0>(cx, ctx_pair, sym, apm, apm_ctx, mix_ctx, mixer, seat, signs, fast, sign_apm, sign_ctx);
  }

  int32_t decode_symbol(BMFState* cx, const uint32_t* ctx_pair, Apm* apm, int32_t apm_ctx, int32_t mix_ctx,
                        Mix4* mixer, uint32_t* seat, uint32_t* signs, uint32_t* fast, Apm* sign_apm, int32_t sign_ctx) {
    return code_three_way<1>(cx, ctx_pair, 0, apm, apm_ctx, mix_ctx, mixer, seat, signs, fast, sign_apm, sign_ctx);
  }
};

int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<(rate-1)))>>rate;
}

int16_t p2_bump(BMFState* cx, int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>cx[0].deadzone_hi)-(err<cx[0].deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

void p2_update(BMFState* cx, P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p[0].weighted;
  p[0].weighted = p2_bump(cx, w, res-p2_pred(w, p[0].rate), shift);
}

void p2_update_into(BMFState* cx, P2Count* p, int32_t &res, int32_t shift) {
  const int32_t w = p[0].weighted;
  res -= p2_pred(w, p[0].rate);
  p[0].weighted = p2_bump(cx, w, res, shift);
}

void p2_rescale(P2Freq* r) {
  if( r[0].f[0]+r[0].f[1]+r[0].f[2]>kP2FreqRescaleTotal ) r[0].rescale_three_way();
}

void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num) {
  p2_rescale(r);
  r[0].f[slot] += (num*(uint32_t)r[0].step)>>idx_clamp(P2_add_shift, 0, 15);
}

void p2_bump_sides(P2Freq* group, int32_t ctx15, int32_t lo, int32_t slot, uint32_t up, uint32_t dn) {
  if( ctx15<15 ) p2_freq_add(&group[1], slot, up);
  if( ctx15>lo ) p2_freq_add(&group[-1], slot, dn);
}

void p2_nudge(P2Count* p, int32_t res, int32_t shift = P2_nudge_shift) {
  p[0].weighted = (int16_t)((uint16_t)p[0].weighted+((uint32_t)(res-p2_pred(p[0].weighted, p[0].rate)+(1<<(shift-1)))>>shift));
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

// NbRow and CtxWeights are mutually recursive: CtxWeights holds NbRow pointers,
// and NbRow::predict blends through a CtxWeights.  One of the two must be
// introduced ahead of the other.
struct NbRow;

struct CtxWeights {
  NbRow* row[6];
};

struct NbRow {
  float w[15][4];
  uint32_t uses;
  uint32_t _pad[3];
  inline int32_t predict(BMFState* cx, float (*nb)[4], CtxWeights* sets, int32_t mode) {
    const float* mix = bmf_p2_mix[mode];
    float mixed[7][4], centre, prediction, own;
    int32_t i, j, k;
    centre = nb_dot(cx[0].p2_coef, nb);
    nb[7][0] = centre;
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ )
        nb[j][k] -= centre;
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ ) {
        mixed[j][k] = mix[0]*sets[0].row[0][0].w[j][k];
        for( i = 1; i<6; i++ )
          mixed[j][k] += mix[i]*sets[0].row[i][0].w[j][k];
      }
    prediction = nb_dot(mixed, nb)+centre;
    nb[7][2] = prediction;
    if( uses ) {
      own = centre+nb_dot(w, nb);
      nb[7][1] = own;
      return (int32_t)(prediction+((own-prediction)*w[14][0])/w[14][1]);
    }
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ ) {
        w[j][k] = mixed[j][k]*bmf_p2_decay;
        w[j+7][k] = sets[0].row[0][0].w[j+7][k]*bmf_p2_seed;
      }
    w[14][0] = idx_f(P2_cov_seed, 0);
    w[14][1] = idx_f(P2_var_seed, 16);
    w[14][2] = idx_f(P2_msscale_seed, 0);
    nb[7][1] = prediction;
    return (int32_t)prediction;
  }
};

struct P2Coef {
  float saved_coef[7][4];
  float saved_rate[3][4];
  void fold(BMFState* cx) {
    memcpy(saved_coef, cx[0].p2_coef, sizeof saved_coef);
    memcpy(saved_rate, cx[0].p2_rate[4], sizeof saved_rate);
    for( int32_t k = 0; k<4; ++k ) {
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_coef[i][k] += cx[0].p2_coef[4+i][k];
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_coef[4+i][k] = 0;
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_rate[4+i][k] = bmf_p2_rate_reset;
    }
  }

  void restore(BMFState* cx) {
    memcpy(cx[0].p2_coef, saved_coef, sizeof saved_coef);
    memcpy(cx[0].p2_rate[4], saved_rate, sizeof saved_rate);
  }
};

int32_t over_thresholds(int32_t n, const int32_t* t, int32_t lo, int32_t hi, int32_t scale) {
  int32_t over = 0;
  for( int32_t k = hi; k>=lo; --k ) over += n>scale*t[k];
  return over;
}

const int32_t p2_band_edges[5] = {P2_band_edge_0, P2_band_edge_1, P2_band_edge_2, P2_band_edge_3, P2_band_edge_4};

int32_t grad(int32_t a, int32_t b, int32_t c) {
  return a+b-c;
}

template <int16_t P2Ctx::* Field> struct RowOf {
  const P2Ctx* row;
  int32_t operator()(int32_t i) const {
    return row[i].*Field;
  }
};

using DvalRow = RowOf<&P2Ctx::dval>;

using ValRow = RowOf<&P2Ctx::val>;

struct AltP2Block {
  static const int32_t kRowMargin = 8;
  static const int32_t kRowTail = 5;
  static const int32_t kNbRowCount = 1088;
  NbRow nb_weights[kNbRowCount];
  float p2_row[7][4];
  float bias[4];
  NbRow* nb_cur;
  NbRow** row0;
  NbRow** row1;
  NbRow** cur;
  NbRow** above;
  int32_t bank_ctx[5];
  int32_t _pad0;
  uint16_t pred_prev;
  uint8_t _pad2[2];
  uint32_t ctx;
  uint32_t ctx_pair[2];
  int32_t sign_map_ctx = 0;
  Apm apm;
  Apm sign_apm;
  Mix4 class_mix;
  uint32_t pz_seat[kSeatCount];
  uint32_t pz_signs[243];
  int32_t nb_id_used;
  int32_t band_lo;
  int32_t band_hi;
  uint32_t plane_idx;
  int32_t has_ref;
  P2Ctx* cursor[5];
  P2Ctx* buf[5];
  CtxWeight ctx_w[5];
  uint8_t _pad1[48];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  int16_t nb_id[P2_NbSlot_Volume];
  // Sized by its own index declaration rather than beside it.
  static const int32_t kP2CounterCount = 163840;  // bias counters
  P2Count p2_ctr[kP2CounterCount+1];

  // The map's context.  ctx's low digit is the activity class the rest of the
  // model already reads as one (the deadzone in alt_p2_model is derived from
  // it), so it is the axis most likely to carry a systematic bias the frequency
  // cell cannot see.
  int32_t apm_context() const {
    // The zero decision wants the phase too: a prediction that landed close to
    // a lattice point is likelier to have hit it exactly.
    return (int32_t)((((uint32_t)sign_map_ctx>>3)&15u)<<4)|(int32_t)MakeCellMap(ctx);
  }

  static uint32_t slot(int32_t bank, int32_t ctx) {
    return MakeBankSlot((uint32_t)bank, (uint32_t)ctx);
  }

  void seat_nb_row(int32_t nb_slot, const P2Ctx* c0) {
    int32_t at = nb_id[nb_slot];
    if( !at ) {
      if( nb_id_used+1<kNbRowCount ) {
        nb_id[nb_slot] = ++nb_id_used;
        nb_cur = &nb_weights[nb_id_used];
        return;
      }
      // nb_slot spans 1920 values but the pool holds only kNbRowCount-1 rows,
      // so a plane using more than that many distinct contexts would seat past
      // the end of nb_weights.  Share the last seated row instead.  Encoder and
      // decoder reach this the same way, so the stream stays decodable; only
      // prediction quality degrades.
      at = nb_id_used;
      nb_id[nb_slot] = at;
    }
    NbRow*const wrow = &nb_weights[at];
    nb_cur = wrow;
    const float err = ((float)c0[-1].val-(nb_dot(wrow[0].w, p2_row)+bias[0]))*idx_f(P2_seat_gain, 21);
    const float floor_ = idx_f(P2_seat_floor, 0)*wrow[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        nlms_step(wrow, j, k, p2_row[j][k], err, bmf_p2_ms_rate, floor_);
  }

  int32_t p2_pred_at(int32_t bank, int32_t bctx) {
    const P2Count &c = ctr_at(slot(bank, bctx));
    return p2_pred(c.weighted, c.rate);
  }

  int32_t step_bank(int32_t bank_no, uint32_t word, int32_t run_in) {
    // The context arrives as the bank value.  It used to arrive shifted up by
    // eleven and be shifted straight back down here, from when the context was
    // assembled at absolute bit positions; an Index declaration places its
    // factors by order, so the shift has nothing left to do.
    const int32_t bank = (int32_t)word;
    bank_ctx[bank_no] = bank;
    const int32_t pred = p2_pred_at(bank_no, bank);
    nb_sum[2*bank_no+1] = pred;
    const int32_t run = pred+run_in;
    nb_sum[2*bank_no] = run;
    return run;
  }

  // The frozen capacity, not the live Volume: see freq_max in the .idx.
  static const int32_t kP2FreqCount = 15552;   // cells in the P2 frequency lattice
  P2Freq freq[kP2FreqCount+1];
  uint32_t pz_fast[kP2FreqCount+1];

  // Every access to the two big tables goes through these.  The context that
  // indexes them is built from the IX layout parameters and then has offsets
  // added to it, so a swept bit position walks off the end -- and unlike the
  // slow model's tables there is no single choke point to bound, the index is
  // assembled at a dozen sites.  One compare beside a table lookup that already
  // misses cache; the shipping build folds the size, not the compare, because
  // the index is data.
  // Bounded to the capacity, with one spare element beyond it: several callers
  // reach a neighbouring cell with [+1] off what this returns, and bounding to
  // the last element would put that one past the end.  The [-1] side is
  // guarded by the callers, which only take it when the low context bits are
  // non-zero.
  P2Freq &freq_at(int32_t i) {
    return freq[idx_bound((uint32_t)i, kP2FreqCount)];
  }

  uint32_t &fast_at(int32_t i) {
    return pz_fast[idx_bound((uint32_t)i, kP2FreqCount)];
  }

  P2Count &ctr_at(uint32_t i) {
    return p2_ctr[idx_bound(i, kP2CounterCount)];
  }
  uint8_t _pad3[8];

  void nlms_step(NbRow* row, int32_t j, int32_t k, float x, float err, float ms_rate, float floor_) {
    const float ms = row[0].w[7+j][k]+(x*x-row[0].w[7+j][k])*ms_rate;
    row[0].w[7+j][k] = ms;
    row[0].w[j][k] += cx[0].p2_rate[j][k]*err*x/(ms+floor_);
  }

  void nlms_track_two_rows(NbRow* fast, NbRow* slow, float err_fast, float err_slow, float conf) {
    const float floor_fast = idx_f(P2_floor_fast, 0)*fast[0].w[14][2];
    const float floor_slow = idx_f(P2_floor_slow, 0)*slow[0].w[14][2];
    const float rate_slow = idx_f(P2_rate_slow, 28)*conf;
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k ) {
        const float x = p2_row[j][k];
        nlms_step(fast, j, k, x, err_fast, idx_f(P2_ms_rate_fast, 28), floor_fast);
        nlms_step(slow, j, k, x, err_slow, rate_slow, floor_slow);
      }
  }

  void nlms_predict_and_correct(NbRow* row, float sample, float bias0) {
    const float err = sample-(bias0+nb_dot(row[0].w, p2_row));
    const float ms_scale = row[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        row[0].w[j][k] += cx[0].p2_rate[j][k]*err*p2_row[j][k]/(row[0].w[7+j][k]+ms_scale*idx_f(P2_ms_floor, 0));
    ++row[0].uses;
    row[0].w[14][2] = ms_scale+((idx_f(P2_msscale_target, 0)-ms_scale)*idx_f(P2_msscale_rate, 36));
  }

  struct P2RefRows {
    P2Ctx* r0, * r1, * r2;
  };

  struct P2Refs {
    P2RefRows a, b;
  };

  static P2RefRows ref_rows(P2Ctx*const* ring) {
    return {ring[0]-1, ring[1]-1, ring[2]-1};
  }

  P2Refs fill_row_inputs(AltP2Block* refa, AltP2Block* refb) {
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    const DvalRow d0{c0}, d1{c1}, d2{c2}, d3{c3};
    const ValRow v0{c0}, v1{c1}, v2{c2}, v3{c3}, v4{c4};
    p2_row[0][0] = (float)d1(0);
    p2_row[0][1] = (float)d1(1);
    p2_row[0][2] = (float)grad(d0(-1), d1(0), d1(-1));
    p2_row[0][3] = (float)grad(d0(-2), d1(0), d1(-2));
    p2_row[1][0] = (float)grad(d1(-1), d1(0), d2(-1));
    p2_row[1][1] = (float)(-3*(d2(0)-d1(0))+d3(0));
    p2_row[1][2] = (float)grad(d0(-1), d1(2), d1(1));
    p2_row[1][3] = (float)grad(d0(-2), d1(1), d1(-1));
    p2_row[2][0] = (float)(2*d0(-1)-d0(-2));
    p2_row[2][1] = (float)grad(d0(-3), d1(0), d1(-3));
    p2_row[2][2] = (float)grad(d2(0), d1(1), d3(1));
    p2_row[2][3] = (float)d0(-3);
    p2_row[3][0] = (float)grad(d1(-2), d1(0), d2(-2));
    p2_row[3][1] = (float)grad(d0(-2), d1(-1), d1(-3));
    p2_row[3][2] = (float)grad(d1(1), (d1(2)+d1(0))>>1, d2(2));
    p2_row[3][3] = (float)d3(0);
    if( refa ) {
      const P2Refs refs{ref_rows(refa[0].cursor), ref_rows(refb[0].cursor)};
      P2Ctx*const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
      P2Ctx*const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
      const ValRow a0{ra0}, a1{ra1}, a2{ra2}, b0{rb0}, b1{rb1}, b2{rb2};
      const bool no_ref = has_ref==0;
      if( !no_ref ) {
        const float bias_l = (float)c0[0].dval;
        for( int32_t j = 0; j<4; ++j )
          for( int32_t k = 0; k<4; ++k )
            p2_row[j][k] += bias_l;
      }
      const int32_t plane = (int32_t)plane_idx;
      if( plane ) {
        if( plane==1 ) {
          p2_row[4][0] = (float)(d0(0)+d1(3));
          p2_row[4][1] = (float)grad(v0(-2), a1(2), a1(0));
          p2_row[4][2] = (float)grad(v1(1), a0(-1), a1(0));
          p2_row[4][3] = (float)grad(v0(-1), a1(0), a1(-1));
          p2_row[5][0] = (float)grad(v1(1), b1(2), b2(3));
          p2_row[5][1] = (float)grad(v0(-2), b0(0), b0(-2));
          p2_row[5][2] = (float)grad(v1(0), b0(0), b1(0));
          p2_row[5][3] = (float)(v0(-2)+ra0[0].err);
          if( no_ref ) {
            p2_row[6][0] = (float)grad(v0(-2), a0(0), a0(-2));
            p2_row[6][1] = (float)grad(v2(0), a0(0), a2(0));
            p2_row[6][2] = (float)(grad(a2(0), a0(0), v2(0))+2*(v1(0)-a1(0)));
            p2_row[6][3] = (float)(v0(-1)+v1(-1)+a1(-2)+a0(0)-a0(-1)-a1(-1)-v1(-2));
          } else {
            p2_row[6][0] = (float)v1(0);
            p2_row[6][1] = (float)v0(-3);
            p2_row[6][2] = (float)grad(v0(-1), a0(-1), a0(-2));
            p2_row[6][3] = (float)grad(v0(-3), v0(-1), v0(-4));
          }
        } else {
          p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
          p2_row[4][1] = (float)grad(v0(-1), a0(-1), a0(-2));
          p2_row[4][2] = (float)grad(v1(1), a1(0), a2(1));
          p2_row[4][3] = (float)grad(v0(-2), a1(2), a1(0));
          p2_row[5][0] = (float)grad(v1(0), a0(-2), a1(-2));
          p2_row[5][1] = (float)grad(v1(0), a0(0), a1(0));
          p2_row[5][2] = (float)grad(v0(-2), a0(0), a0(-2));
          p2_row[5][3] = (float)grad(v0(-2), b0(0), b0(-2));
          p2_row[6][0] = (float)grad(v2(0), b0(0), b2(0));
          p2_row[6][1] = (float)(grad(b0(-2), b0(0), v0(-2))+2*(v0(-1)-b0(-1)));
          p2_row[6][2] = (float)(v0(-2)+rb0[0].err);
          p2_row[6][3] = (float)(v2(0)+ra0[0].err);
        }
      } else {
        p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
        p2_row[4][1] = (float)grad(v0(-5), v1(0), v1(-5));
        p2_row[4][2] = (float)grad(v0(-4), v1(0), v1(-4));
        p2_row[4][3] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
        p2_row[5][0] = (float)grad(v0(-2), a0(0), a0(-2));
        p2_row[5][1] = (float)grad(v1(1), a1(1), a2(2));
        p2_row[5][2] = (float)grad(v1(-2), a1(2), a2(0));
        p2_row[5][3] = (float)(grad(a0(-2), a0(0), v0(-2))+2*(v0(-1)-a0(-1)));
        p2_row[6][0] = (float)grad(v2(1), b0(0), b2(1));
        p2_row[6][1] = (float)grad(v0(-2), b0(0), b0(-2));
        p2_row[6][2] = (float)grad(v0(-1), b1(1), b1(0));
        p2_row[6][3] = (float)grad(v1(1), b1(2), b2(3));
      }
      return refs;
    } else {
      p2_row[4][0] = (float)grad(v1(3), v1(0), v2(3));
      p2_row[4][1] = (float)grad(v0(-4), v1(0), v1(-4));
      p2_row[4][2] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
      p2_row[4][3] = (float)grad(v2(-1), v1(1), v3(0));
      p2_row[5][0] = (float)grad(v3(1), v1(0), v4(1));
      p2_row[5][1] = (float)v1(3);
      p2_row[5][2] = (float)grad(v0(-3), v0(-1), v0(-4));
      p2_row[5][3] = (float)grad(v0(-1), v3(0), v3(-1));
      p2_row[6][0] = (float)grad(v0(-5), v1(0), v1(-5));
      p2_row[6][1] = (float)v4(0);
      p2_row[6][2] = (float)grad(v0(-5), v0(-1), v0(-6));
      p2_row[6][3] = (float)v1(-6);
      return {};
    }
  }
  int32_t seat_symbol_context(int32_t run4, const P2Refs &refs) {
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    int32_t flat_a, flat_b;
    int32_t magsum_own = c1[5].mag+c1[4].mag+c1[-3].mag+c1[-4].mag+3*(c1[2].mag+c2[1].mag)+7*c1[0].mag+6*c1[1].mag+c0[-6].mag+c0[-7].mag+c0[-8].mag+8*c0[-1].mag+c4[2].mag+c4[1].mag+c4[0].mag+c4[-1].mag+c0[-4].mag+c0[-5].mag+(c3[2].mag+c3[1].mag+c3[-1].mag+c3[-2].mag+c4[-2].mag+c2[5].mag+c2[4].mag+c2[3].mag+c2[-2].mag+c2[-3].mag)+4*(c1[-1].mag+c0[-2].mag+c2[0].mag)+2*(c1[-2].mag+c1[3].mag+c0[-3].mag+c3[0].mag+c2[-1].mag+c2[2].mag);
    const int32_t lo_band = band_lo, hi_band = band_hi;
    // Clamped to the weight row: a wider bitmap means more buckets than the
    // three weights a selector has.
    ctx_w[0].sel = (int32_t)idx_bound(MakeRun4Sel(-run4), 3);
    ctx_w[1].sel = (run4-c1[0].val<=hi_band)+(run4-c1[0].val<lo_band);
    int32_t d_run4 = run4-c0[-1].val;
    bool in_band = d_run4<=hi_band;
    ctx_w[2].sel = in_band+(d_run4<lo_band);
    ctx_w[3].sel = c1[0].sign;
    ctx_w[4].sel = c0[-1].sign;
    int32_t magsum = magsum_own;
    int32_t magu = c2[0].mag+c1[0].mag;
    int32_t magl = c0[-1].mag+c0[-2].mag;
    if( refs.a.r0 ) {
      int32_t mag = refs.b.r0[0].mag;
      int32_t mag_ref1 = refs.b.r1[0].mag+refs.a.r1[0].mag;
      int32_t mag_ref0 = mag+refs.a.r0[0].mag;
      magsum = mag_ref1+magsum_own+4*mag_ref0+2*(refs.a.r0[-1].mag+refs.b.r0[-1].mag);
      flat_a = mag_ref0+magl+refs.b.r0[-2].mag+refs.b.r0[-1].mag+refs.a.r0[-2].mag+refs.a.r0[-1].mag;
      if( has_ref ) {
        const int32_t d_run4b = run4-c1[0].dval-c0[0].dval;
        if( d_run4b<lo_band||d_run4b>hi_band ) {
          const int32_t d_run4c = run4-c0[-1].dval-c0[0].dval;
          flat_b = d_run4c>=lo_band&&hi_band>=d_run4c;
        } else {
          flat_b = 1;
        }
      } else {
        flat_b = mag_ref1+mag_ref0+magu+refs.b.r2[0].mag+refs.a.r2[0].mag;
      }
      if( plane_idx==1 ) {
        int32_t d_ra_ra1 = refs.a.r0[0].val-refs.a.r1[0].val;
        ctx_w[3].sel = (d_ra_ra1<=hi_band)+(d_ra_ra1<lo_band);
        int32_t d_ra_left = refs.a.r0[0].val-refs.a.r0[-1].val;
        ctx_w[4].sel = (d_ra_left<=hi_band)+(d_ra_left<lo_band);
      } else if( (int32_t)plane_idx>1 ) {
        int32_t d_rb_rb1 = refs.b.r0[0].val-refs.b.r1[0].val;
        ctx_w[3].sel = (d_rb_rb1<=hi_band)+(d_rb_rb1<lo_band);
        int32_t d_rb_left = refs.b.r0[0].val-refs.b.r0[-1].val;
        in_band = d_rb_left<=hi_band;
        ctx_w[4].sel = in_band+(d_rb_left<lo_band);
        int32_t ra0_val = refs.a.r0[0].val;
        int32_t d_ra_ra1b = ra0_val-(refs.a.r1)[0].val;
        in_band = lo_band<=d_ra_ra1b;
        if( in_band&&d_ra_ra1b<=hi_band ) {
          flat_a = 1;
        } else {
          int32_t d_ra_leftb = ra0_val-refs.a.r0[-1].val;
          flat_a = d_ra_leftb>=lo_band&&d_ra_leftb<=hi_band;
        }
      }
    } else {
      flat_a = magl+c0[-3].mag+c0[-4].mag+c0[-5].mag;
      flat_b = c4[0].mag+c3[0].mag+magu+c0[0].mag;
    }
    const int32_t ctx15 = magsum>=P2_magsum_cap ? 15 : nb_ctx[magsum>>idx_clamp(P2_magsum_shift, 1, 12)];
    const int32_t ctx_idx = clamp32((run4+7)>>4, 0, 255);
    // The sub-pixel remainder the rounding above discards, kept for the sign
    // decision; see code_three_way.  Sixteen fractions by eight activity
    // classes is 128 of the map's 256 contexts, which measured better here than
    // the fraction alone (16) or the full activity class (256).
    sign_map_ctx = (int32_t)((((uint32_t)(run4+7)&15u)<<3)|((uint32_t)ctx15>>1));
    ctx_pair[0] = MakeCtxPair((uint32_t)ctx_delta[ctx_idx+4], (uint32_t)ctx15);
    ctx_pair[1] = MakeCtxPair((uint32_t)ctx_delta[ctx_idx], (uint32_t)ctx15);
    ctx = (int32_t)MakeP2Ctx(mixer_fwd(ctx_w, 5)>>kP2MixShift, (uint32_t)(flat_b==0),
                            (uint32_t)(flat_a==0), (uint32_t)ctx15);
    return ctx_idx;
  }
  void code_banks(int32_t sample16) {
    for( uint32_t bank = 0; bank<5; ++bank ) {
      const int32_t bctx = bank_ctx[bank];
      const int32_t res = sample16-nb_sum[2*bank];
      P2Count* node0 = &ctr_at(slot(bank, bctx));
      const int32_t w0 = res+(uint16_t)node0[0].weighted;
      node0[0].weighted = w0;
      int32_t countdown = node0[0].b1;
      if( !countdown )
        continue;
      const int32_t w0b = w0+4*((res>cx[0].deadzone_hi)-(res<cx[0].deadzone_lo));
      node0[0].weighted = w0b;
      uint32_t ctxw = bctx;
      if( (int32_t)abs32(res)<P2_flat_resid ) {
        --countdown;
        if( !countdown&&(uint8_t)node0[0].rate<8u ) {
          const int32_t from = node0[0].rate;
          node0[0].rate = (uint8_t)(from+1);
          node0[0].b1 = p2_b1_reload[from-5];
          node0[0].weighted = 2*w0b;
        } else {
          node0[0].b1 = countdown;
        }
      }
      if( cx[0].alphabet_reduced )
        continue;
      P2Count* mir = &ctr_at(slot(bank, ctxw^kBankMirrorMask));
      bmf_prefetch<pf_all>(mir);
      const int32_t res2 = nb_sum[2*bank+1]+res;
      int32_t lowbits = ctxw&3;
      if( lowbits<3 )
        p2_update(cx, &node0[1], res2, P2_up_near);
      if( lowbits )
        p2_update(cx, &node0[-1], res2, P2_up_direct);
      P2Count* direct[kBankFan], * mirror[kBankFan], * rot[kBankFan];
      for( int32_t k = 0; k<kBankFan; ++k ) {
        // Through slot() rather than as an offset off bankp: the base is
        // bounded but base+x is not, and x follows the context, which a
        // widened threshold bitmap can grow past the table.
        const uint32_t x = ctxw^p2_bank_bits[k];
        direct[k] = &ctr_at(slot(bank, (int32_t)x));
        mirror[k] = &ctr_at(slot(bank, (int32_t)(x^kBankMirrorMask)));
        rot[k] = &ctr_at(slot(bank, p2_ctx_rotate[(x>>2)&3]+(int32_t)(x&0xFFFFFFF3)));
        bmf_prefetch<pf_all>(direct[k]);
        bmf_prefetch<pf_all>(mirror[k]);
        bmf_prefetch<pf_all>(rot[k]);
      }
      const int32_t neg = -res2;
      const int32_t w_top = mir[0].weighted;
      const int32_t e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
      const int32_t bump = P2_kick*((e_top>cx[0].deadzone_hi)-(e_top<cx[0].deadzone_lo));
      uint32_t w_new = bump+e_top+P2_kick_round;
      mir[0].weighted = (int16_t)(w_top+(w_new>>idx_clamp(P2_kick_shift, 0, 15)));
      if( lowbits<3 )
        p2_nudge(&mir[1], -res2);
      if( lowbits<=0 ) {
        for( int32_t k = 0; k<kBankFan; ++k ) {
          p2_update(cx, direct[k], res2, P2_up_direct);
          p2_update(cx, rot[k], res2, P2_up_rot);
          p2_nudge(&direct[k][1], res2);
          p2_update(cx, mirror[k], -res2, P2_up_rot);
        }
      } else {
        p2_nudge(&mir[-1], -res2, P2_nudge_far);
        p2_update(cx, direct[0], res2, P2_up_direct);
        p2_update(cx, rot[0], res2, P2_up_rot);
        walk_bank_bits(direct, mirror, rot, res2, lowbits<3);
        p2_update(cx, mirror[kBankFan-1], -res2, P2_up_rot);
      }
    }
  }
  void walk_bank_bits(P2Count*const* direct, P2Count*const* mirror, P2Count*const* rot, int32_t res, bool nudge) {
    for( int32_t k = 0; k<kBankFan; ++k ) {
      if( k ) {
        p2_update(cx, direct[k], res, P2_up_direct);
        p2_update(cx, rot[k], res, P2_up_rot);
      }
      if( nudge )
        p2_nudge(&direct[k][1], res);
      p2_update(cx, &direct[k][-1], res, P2_up_rot);
      if( k<P2_fan_mirror )
        p2_update(cx, mirror[k], -res, P2_up_rot);
    }
  }

  void bump_bank(int32_t k, uint32_t step_v, int32_t ctx15, int32_t fold_sel, int32_t is_dec) {
    const int32_t sel = ctx_w[k].sel;
    if( sel==1 ) {
      p2_freq_add(&freq_at(ctx_w[k].w[0]+step_v-ctx_w[k].w[1]), fold_sel, P2_add_up);
      const uint32_t off = ctx-ctx_w[k].w[1];
      P2Freq*const group = &freq_at(off+ctx_w[k].w[0]);
      const uint32_t next_group = ctx_w[k].w[2]+off;
      p2_freq_add(&group[0], is_dec, 3);
      if( ctx15<15 ) {
        p2_freq_add(&group[1], is_dec, 4);
        p2_freq_add(&freq_at(next_group+1), is_dec, P2_add_dn);
      }
      if( ctx15>2 )
        p2_freq_add(&group[-1], is_dec, 6);
    } else {
      p2_freq_add(&freq_at(step_v+ctx_w[k].w[1]-ctx_w[k].w[2-sel]), fold_sel, P2_add_step);
      P2Freq*const group = &freq_at(ctx_w[k].w[1]+(ctx-ctx_w[k].w[sel]));
      p2_freq_add(&group[0], is_dec, 7);
      p2_bump_sides(group, ctx15, 0, is_dec, 5, 6);
    }
  }

  template <int32_t f_DEC> void alt_p2_d8_body(uint8_t* src, uint8_t* out, int32_t width, int32_t height) {
    int32_t j, code0, code;
    uint8_t* outp, code2;
    uint32_t k;
    uint8_t* src_end = src;
    if constexpr( f_DEC )
      cx[0].rc_begin_decode();
    else
      cx[0].rc_begin_encode();
    P2Ctx* pix = cursor[0];
    if( width>0 ) {
      uint8_t* p = src;
      for( j = 0; j<width; ++j ) {
        P2Ctx* rec = cursor[0];
        int32_t q2 = rec[-1].val>>4;
        // The delta is stored as its own digit, so reaching the strip means
        // putting it back in place -- the same shift MakeCtxPair applies.
        ctx_pair[0] = ctx+MakeCtxPair((uint32_t)ctx_delta[q2+4], 0);
        ctx_pair[1] = ctx+MakeCtxPair((uint32_t)ctx_delta[q2], 0);
        P2Freq &fr = freq_at(ctx+ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+ctx_w[2].w[rec[-2].sign]+ctx_w[1].w[rec[-1].sign]+ctx_w[0].w[idx_bound(MakeQ2Sel(-q2), 3)]+ctx_w[4].w[1]);
        if constexpr( f_DEC ) {
          out[0] = (uint8_t)(((uint16_t)rec[-1].val>>4)+(uint8_t)unfold[fr.decode_symbol(cx, ctx_pair, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx)]);
        } else {
          code0 = fold_or_refuse(out, p[0], (uint16_t)rec[-1].val>>4, fold, fold_hi, unfold);
          fr.encode_symbol(cx, ctx_pair, code0, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx);
          ++p;
        }
        int32_t val = 16*(uint8_t)out[0];
        cursor[0][0].val = val;
        cursor[0][0].dval = val;
        ++out;
        int32_t err = (int16_t)(rec[0].val-rec[-1].val);
        rec[0].err = err;
        int32_t adiff = abs32(err);
        cursor[0][0].aerr = adiff;
        cursor[0][0].dupright = adiff;
        cursor[0][0].dupleft = adiff;
        cursor[0][0].dup = adiff;
        cursor[0][0].dleft = (uint32_t)cursor[0][0].dup>>1;
        cursor[0][0].mag = 2;
        cursor[0][0].sign = (cursor[0][0].err<=0)+(cursor[0][0].err<0);
        pix = cursor[0]+1;
        cursor[0] = pix;
      }
      if constexpr( !f_DEC )
        src_end = p;
    }
    seed_history(width);
    if( height>1 ) {
      uint32_t* pair = ctx_pair;
      outp = out;
      for( uint32_t y = 0; y<(uint32_t)(height-1); ++y ) {
        uint8_t* srcp = src_end;
        start_row();
        if( width<=0 )
          continue;
        for( k = 0; k<(uint32_t)width; ++k ) {
          int32_t pred = alt_p2_context(nullptr, nullptr);
          if constexpr( f_DEC ) {
            code2 = code = freq_at(ctx).decode_symbol(cx, pair, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx);
            outp[k] = (uint8_t)(pred+(unfold[code]));
          } else {
            code = fold_or_refuse(&outp[k], srcp[k], pred, fold, fold_hi, unfold);
            code2 = code;
            freq_at(ctx).encode_symbol(cx, pair, code, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx);
          }
          alt_p2_model(outp[k], code2, outp[k]-pred);
        }
        if constexpr( !f_DEC )
          src_end = &srcp[width];
        outp = &outp[width];
      }
    }
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb) {
    CtxWeights weights;
    int32_t nb_slot;
    uint32_t ctx0, ctx1, ctx2;
    uint32_t ctx0_lo;
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    int32_t sum_ul = 21*c2[-1].dupleft+12*c1[3].dupleft+16*c1[2].dupleft+22*c1[1].dupleft+(23*c0[-1].dupleft)+20*c1[0].dupleft+cx[0].ctx_bias[0]+14*c0[-2].dupleft;
    int32_t sum_ur = 17*c2[-2].dupright+21*c1[2].dupright+15*c1[1].dupright+25*c1[0].dupright+9*c1[-1].dupright+22*c0[-1].dupright+cx[0].ctx_bias[1]+19*c0[-2].dupright;
    int32_t sum4 = 17*c1[3].dleft+15*c1[2].dleft+21*c1[1].dleft+18*c1[0].dleft+16*c1[-1].dleft+22*c0[-1].dleft+cx[0].ctx_bias[2]+19*c0[-2].dleft;
    int32_t sum_u = 14*c1[3].dup+23*c1[1].dup+19*c1[0].dup+25*c0[-1].dup+cx[0].ctx_bias[3]+17*c0[-2].dup+15*(c3[0].dup+c2[0].dup);
    int32_t sum_all = sum4+sum_u+sum_ul+sum_ur;
    int32_t band = over_thresholds(8*sum4, p2_band_edges, 0, 4, sum_u);
    int32_t den_d = 2*c0[-2].val+2*c0[-1].val;
    int32_t num_b = c1[1].val+(c1[0].val+2*c0[-1].val);
    int32_t num_d = 16*(c2[2].val+c2[0].val+c1[1].val+c2[-1].val);
    int32_t gA = over_thresholds(sum_all, bmf_p2_thresholds[band], 9, 12, 1);
    int32_t gB = over_thresholds(num_b, bmf_p2_thresholds[band], 6, 8, 1);
    int32_t gC = over_thresholds(16*sum_ur, bmf_p2_thresholds[band], 3, 5, sum_ul);
    int32_t gD = over_thresholds(num_d, bmf_p2_thresholds[band], 0, 2, den_d);
    nb_slot = (int32_t)MakeNbSlot((uint32_t)band, (uint32_t)gA, (uint32_t)gB, (uint32_t)gC, (uint32_t)gD);
    seat_nb_row(nb_slot, c0);
    const P2Refs refs = fill_row_inputs(refa, refb);
    P2Ctx*const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
    P2Ctx*const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
    weights.row[0] = cur[-1];
    weights.row[1] = above[1];
    weights.row[2] = above[2];
    weights.row[3] = cur[-2];
    weights.row[4] = above[0];
    weights.row[5] = cur[0];
    const int32_t a = 16*sum4, b = 14*sum_u, c = 13*sum_ur, d = 11*sum_ul;
    const int32_t best01 = min32(a, b);
    int32_t mode = (best01==a) ? 0 : 1;
    if( best01>c )
      mode = 2;
    if( min32(best01, c)>d )
      mode = 3;
    const int32_t filt = nb_cur[0].predict(cx, (float (*)[4]) p2_row, &weights, mode);
    pred_prev = (uint16_t)filt;
    int32_t g3pair = c0[-1].aerr+c1[0].aerr;
    if( refa )
      g3pair += (rb0[0].aerr+ra0[0].aerr)>>1;
    int32_t g3sum = c1[3].aerr+c1[-1].aerr+c0[-1].aerr+c0[-3].aerr+c0[-4].aerr+c0[-2].aerr;
    ctx0_lo = MakeCtx0Lo(p2_flags((uint32_t)(c1[1].err+c1[0].err+c1[-1].err+c0[-1].err<0), (uint32_t)(c3[0].err+c0[-2].err+2*c0[-4].err<0), (uint32_t)(c1[-3].err+c0[-3].err+c0[-5].err+c0[-7].err<0), (uint32_t)(c0[-1].err<0), (uint32_t)(((uint32_t)((uint16_t)c0[-3].err)>>IX_p2_ctx0_lo_e4)&1u)), filt, g3pair+g3sum);
    if( refa ) {
      ctx0 = MakeCtx0a(p2_flags((uint32_t)(rb0[-1].err<0), (uint32_t)(rb0[0].err<0), (uint32_t)(ra0[-1].err<0), (uint32_t)(rb0[0].err+rb0[-2].err<0), (uint32_t)(ra0[-2].err+ra0[0].err<0), (uint32_t)(c0[-2].err<0)), (uint32_t)(ctx0_lo));
    } else {
      ctx0 = MakeCtx0b(p2_flags((uint32_t)(c1[3].err+c0[-3].err+c0[-7].err+c0[-5].err<0), (uint32_t)(c2[1].err+c2[0].err+c0[-4].err+c0[-8].err<0), (uint32_t)(c0[-4].err+c0[-6].err<0), (uint32_t)(c0[-4].err<0), (uint32_t)(c0[-5].err<0), (uint32_t)(c0[-7].err<0)), (uint32_t)(ctx0_lo));
    }
    const int32_t run0 = step_bank(0, ctx0, filt);
    int32_t nb4_4 = c4[4].val;
    sum_all = ((g3pair<<idx_clamp(P2_g3_up_shift, 0, 20))+sum_all)>>idx_clamp(P2_g3_dn_shift, 1, 20);
    int32_t d_run0 = c0[-5].val-run0;
    int32_t d_up = nb4_4-run0;
    int32_t up5 = c1[5].val;
    int32_t d_up5 = up5-run0;
    if( refa ) {
      int32_t dv_now = c0[0].dval;
      ctx1 = MakeCtx1a(p2_flags((uint32_t)(((uint32_t)(c0[-1].val+ra0[0].val-ra0[-1].val-run0)>>IX_p2_ctx1_e0)&1u), (uint32_t)(ra2[0].dval+ra1[0].dval-2*ra0[0].dval<0), (uint32_t)(((uint32_t)(dv_now+c0[-3].dval-run0-(c1[2].dval-c1[5].dval))>>IX_p2_ctx1_e2)&1u), (uint32_t)(((uint32_t)(dv_now+c0[-5].dval-run0)>>IX_p2_ctx1_e3)&1u), (uint32_t)(((uint32_t)(dv_now+c1[0].dval+rb1[2].dval-rb2[2].dval-run0)>>IX_p2_ctx1_e4)&1u), (uint32_t)(((uint32_t)(dv_now+c0[-1].dval+rb0[0].dval-rb0[-1].dval-run0)>>IX_p2_ctx1_e5)&1u), (uint32_t)(((uint32_t)(c2[-1].dval+dv_now+ra0[0].dval-ra2[-1].dval-run0)>>IX_p2_ctx1_e6)&1u), (uint32_t)(((uint32_t)(dv_now+c1[2].dval+ra0[0].dval-ra1[2].dval-run0)>>IX_p2_ctx1_e7)&1u), (uint32_t)(((uint32_t)(d_up5)>>IX_p2_ctx1_e8)&1u), (uint32_t)(((uint32_t)(d_up)>>IX_p2_ctx1_e9)&1u), (uint32_t)(((uint32_t)(d_run0)>>IX_p2_ctx1_e10)&1u)), run0, sum_all);
    } else {
      int32_t d_run0b = run0-c0[-3].val;
      ctx1 = MakeCtx1b(p2_flags((uint32_t)(((uint32_t)(nb4_4-c3[0].val+run0-c1[4].val)>>IX_p2_ctx11_e0)&1u), (uint32_t)(((uint32_t)(c1[-5].val-c1[-2].val+d_run0b)>>IX_p2_ctx11_e1)&1u), (uint32_t)(((uint32_t)(d_run0b+c1[2].val-up5)>>IX_p2_ctx11_e2)&1u), (uint32_t)(((uint32_t)(c4[3].val-c2[2].val+run0-c2[1].val)>>IX_p2_ctx11_e3)&1u), (uint32_t)(((uint32_t)(c4[-1].val-c3[0].val+run0-c1[-1].val)>>IX_p2_ctx11_e4)&1u), (uint32_t)(((uint32_t)(c1[0].val-c1[2].val+run0-c0[-2].val)>>IX_p2_ctx11_e5)&1u), (uint32_t)(((uint32_t)(-d_up5)>>IX_p2_ctx11_e6)&1u), (uint32_t)(((uint32_t)(-d_up)>>IX_p2_ctx11_e7)&1u), (uint32_t)(((uint32_t)(run0-c4[2].val)>>IX_p2_ctx11_e8)&1u), (uint32_t)(((uint32_t)(run0-c4[-3].val)>>IX_p2_ctx11_e9)&1u), (uint32_t)(((uint32_t)(-d_run0)>>IX_p2_ctx11_e10)&1u)), run0, sum_all);
    }
    const int32_t run1 = step_bank(1, ctx1, run0);
    int32_t cx2_val0 = c2[0].val;
    int32_t cx1_val = c1[0].val;
    int32_t dvsum2 = c0[-2].val-run1;
    int32_t lap = c0[-2].val+run1-2*c0[-1].val;
    int32_t dtop2 = cx2_val0-run1;
    if( refa ) {
      int32_t dv1 = c1[1].dval;
      int32_t d_ra = ra0[0].val-run1;
      ctx2 = MakeCtx2a(p2_flags((uint32_t)(((uint32_t)(c0[0].dval+c1[-1].dval-run1-(c2[0].dval-dv1))>>IX_p2_ctx2_e0)&1u), (uint32_t)(((uint32_t)(c0[0].dval+c0[-1].dval-run1-(c1[0].dval-dv1))>>IX_p2_ctx2_e1)&1u), (uint32_t)(((uint32_t)(rb0[0].val-run1+cx2_val0-rb2[0].val)>>IX_p2_ctx2_e2)&1u), (uint32_t)(((uint32_t)(d_ra+cx2_val0-ra2[0].val)>>IX_p2_ctx2_e3)&1u), (uint32_t)(((uint32_t)(d_ra+c1[-1].val-ra1[-1].val)>>IX_p2_ctx2_e4)&1u), (uint32_t)(((uint32_t)(c0[0].dval-c0[-2].dval+2*c0[-1].dval-run1)>>IX_p2_ctx2_e5)&1u), (uint32_t)(((uint32_t)(2*cx1_val-run1-cx2_val0)>>IX_p2_ctx2_e6)&1u), (uint32_t)(((uint32_t)(-lap)>>IX_p2_ctx2_e7)&1u), (uint32_t)(((uint32_t)(dtop2)>>IX_p2_ctx2_e8)&1u), (uint32_t)(((uint32_t)(dvsum2)>>IX_p2_ctx2_e9)&1u), (uint32_t)(((uint32_t)(P2_rb_bias-rb0[0].val)>>IX_p2_ctx2_e10)&1u)), run1, sum_all);
    } else {
      ctx2 = MakeCtx2b(p2_flags((uint32_t)(((uint32_t)(run1+3*(cx2_val0-cx1_val)-c3[0].val)>>IX_p2_ctx21_e0)&1u), (uint32_t)(((uint32_t)(run1+c4[0].val-(c2[2].val+c2[-2].val))>>IX_p2_ctx21_e1)&1u), (uint32_t)(((uint32_t)(c3[2].val-c2[-1].val+run1-c1[3].val)>>IX_p2_ctx21_e2)&1u), (uint32_t)(((uint32_t)(c3[1].val-c1[1].val-dtop2)>>IX_p2_ctx21_e3)&1u), (uint32_t)(((uint32_t)(c3[0].val-cx1_val-dtop2)>>IX_p2_ctx21_e4)&1u), (uint32_t)(((uint32_t)(lap)>>IX_p2_ctx21_e5)&1u), (uint32_t)(((uint32_t)(run1-c4[3].val)>>IX_p2_ctx21_e6)&1u), (uint32_t)(((uint32_t)(c3[1].val-run1)>>IX_p2_ctx21_e7)&1u), (uint32_t)(((uint32_t)(run1-c4[0].val)>>IX_p2_ctx21_e8)&1u), (uint32_t)(((uint32_t)(run1-c3[-2].val)>>IX_p2_ctx21_e9)&1u), (uint32_t)(((uint32_t)(-dvsum2)>>IX_p2_ctx21_e10)&1u)), run1, sum_all);
    }
    const int32_t run2 = step_bank(2, ctx2, run1);
    int32_t run_dv3 = run2-c0[0].dval;
    const uint32_t ctx3 = MakeCtx3(p2_flags((uint32_t)(((uint32_t)(3*(c0[-2].val-c0[-1].val)+run2-c0[-3].val)>>IX_p2_ctx3_e0)&1u), (uint32_t)(((uint32_t)(c2[1].dval-((uint32_t)(c1[1].dval+c1[2].dval+c1[-1].dval+c1[0].dval)>>1)+run_dv3)>>IX_p2_ctx3_e1)&1u), (uint32_t)(((uint32_t)(c2[-3].val-c1[-2].val+run2-c1[-1].val)>>IX_p2_ctx3_e2)&1u), (uint32_t)(((uint32_t)(-(c2[2].val+run2-2*c1[1].val))>>IX_p2_ctx3_e3)&1u), (uint32_t)(((uint32_t)(c1[-2].dval-c0[-2].val+run2-c1[0].dval)>>IX_p2_ctx3_e4)&1u), (uint32_t)(((uint32_t)(run2-c1[3].val)>>IX_p2_ctx3_e5)&1u), (uint32_t)(((uint32_t)(run_dv3-c2[1].dval)>>IX_p2_ctx3_e6)&1u), (uint32_t)(((uint32_t)(run2-c3[0].val)>>IX_p2_ctx3_e7)&1u), (uint32_t)(((uint32_t)(2*run2-c1[0].dval-(c1[0].val+c0[0].dval))>>IX_p2_ctx3_e8)&1u), (uint32_t)(((uint32_t)(run2-c0[-1].dval-c0[0].dval)>>IX_p2_ctx3_e9)&1u), (uint32_t)(((uint32_t)(run2-c0[-3].val)>>IX_p2_ctx3_e10)&1u)), run2, sum_all);
    const int32_t run3 = step_bank(3, ctx3, run2);
    int32_t dv_now4 = c0[0].dval;
    int32_t run_dv4 = run3-dv_now4;
    int32_t run_up4 = run3+c3[0].val;
    const uint32_t ctx4 = MakeCtx4(p2_flags((uint32_t)(((uint32_t)(run3-((uint32_t)(c1[1].val+c1[-1].val+2*c1[0].val)>>2))>>IX_p2_ctx4_e0)&1u), (uint32_t)(((uint32_t)(run_up4-c0[-2].val-(c3[2].dval+dv_now4))>>IX_p2_ctx4_e1)&1u), (uint32_t)(((uint32_t)(run3-2*c0[-2].dval-(dv_now4-c0[-4].dval))>>IX_p2_ctx4_e2)&1u), (uint32_t)(((uint32_t)(run3-c0[-3].dval-dv_now4-(c1[0].val-c1[-3].val))>>IX_p2_ctx4_e3)&1u), (uint32_t)(((uint32_t)(run_up4-dv_now4-(c1[1].val+c2[-1].dval))>>IX_p2_ctx4_e4)&1u), (uint32_t)(((uint32_t)(c2[-2].val+run3-2*c1[-1].val)>>IX_p2_ctx4_e5)&1u), (uint32_t)(((uint32_t)(c2[0].dval-2*c1[0].dval+run_dv4)>>IX_p2_ctx4_e6)&1u), (uint32_t)(((uint32_t)((run3-c0[-1].val)-(c1[0].val-c1[-1].val))>>IX_p2_ctx4_e7)&1u), (uint32_t)(((uint32_t)(run_dv4-c1[4].dval)>>IX_p2_ctx4_e8)&1u), (uint32_t)(((uint32_t)(run_dv4-c1[-2].dval)>>IX_p2_ctx4_e9)&1u), (uint32_t)(((uint32_t)((uint16_t)run3-(uint16_t)c0[-4].dval-(uint16_t)dv_now4)>>IX_p2_ctx4_e10)&1u)), run3, sum_all);
    const int32_t run4 = step_bank(4, ctx4, run3);
    return seat_symbol_context(run4, refs);
  }
  uint32_t alt_p2_model(int32_t sample_in, uint8_t code, int32_t resid_in) {
    int32_t ctx_lo = ctx&0xF;
    int32_t sample16 = 16*sample_in;
    P2Ctx*const c0 = cursor[0];
    P2Ctx*const c1 = cursor[1];
    c0[0].val = 16*sample_in;
    c0[0].dval = c0[0].val-c0[0].dval;
    c0[1].dval = 0;
    const int32_t dead = (ctx_lo>6)+(ctx_lo>4)+2*(ctx_lo>9);
    c0[0].sign = (resid_in<=dead)+(resid_in<-dead);
    c0[0].mag = abs32(resid_in);
    float sample = (float)sample16;
    int32_t dl = sample16-c0[-1].val;
    c0[0].dleft = abs32(dl);
    int32_t du = sample16-c1[0].val;
    c0[0].dup = abs32(du);
    int32_t dul = sample16-c1[-1].val;
    c0[0].dupleft = abs32(dul);
    int32_t dur = sample16-c1[1].val;
    c0[0].dupright = abs32(dur);
    int32_t resid = (int16_t)(sample16-pred_prev);
    c0[0].err = resid;
    c0[0].aerr = abs32(resid);
    NbRow* wrow_cur = nb_cur;
    NbRow* wrow_b = cur[-1];
    const float centre = bias[0];
    const float mixed = bias[1];
    const float own = bias[2];
    const float err_own = sample-own;
    const float d_pred = mixed-own;
    const float cov = (((err_own*d_pred)-wrow_cur[0].w[14][0])*idx_f(P2_cov_rate, 33))+wrow_cur[0].w[14][0];
    const float var = (wrow_cur[0].w[14][1]+idx_f(P2_var_floor, 37))+(((d_pred*d_pred)-wrow_cur[0].w[14][1])*idx_f(P2_var_rate, 33));
    wrow_cur[0].w[14][1] = var;
    float cov_kept = idx_f(P2_cov_keep, 27)*var;
    if( cov_kept<=cov )
      cov_kept = fminf(var, cov);
    wrow_cur[0].w[14][0] = cov_kept;
    const float conf = (1.0f-(cov_kept/(var+idx_f(P2_conf_floor, 0))))*idx_f(P2_conf_gain, 0);
    nlms_track_two_rows(wrow_cur, wrow_b, (sample-mixed)*idx_f(P2_mix_gain, 21), err_own*conf, conf);
    nlms_predict_and_correct(wrow_cur, sample, centre);
    cur[0] = nb_cur;
    ++cur;
    ++above;
    code_banks(sample16);
    int32_t ctx_l = ctx;
    ++cursor[0];
    P2Freq* frec = &freq_at(ctx_l);
    ++cursor[1];
    ++cursor[2];
    ++cursor[3];
    ++cursor[4];
    const uint32_t step0 = frec[0].step;
    if( step0<=0x10 )
      return step0;
    int32_t is_dec = code&1;
    int32_t ctx15 = ctx_l&0xF;
    uint32_t pair_ctx = ctx_pair[code&1];
    auto nudge = [&](int32_t at, uint32_t num, uint32_t pair_at) {
                   P2Freq*const bin = &frec[at];
                   p2_rescale(bin);
                   const uint32_t step10 = (num*(uint32_t)frec[at].step)>>4;
                   if( code ) {
                     bin[0].f[2-is_dec] += step10;
                     cx[0].update_binary_pair(cx[0].model_strip(pair_at), (code-1)>>1);
                   } else {
                     bin[0].f[0] += step10;
                   }
                 };
    if( ctx15<15 )
      nudge(1, 10, pair_ctx+1);
    if( ctx15>0 )
      nudge(-1, 13, pair_ctx-1);
    if( freq_at(ctx).step<=0x1Au )
      return ctx;
    int32_t fold_sel = 2-(fold[(uint8_t)-resid_in]&1);
    if( !fold[(uint8_t)-resid_in] )
      fold_sel = fold[(uint8_t)-resid_in];
    const uint32_t step_ctx = MakeP2Step(mixer_rev(ctx_w, 5)>>kP2MixShift, (uint32_t)ctx&63u);
    P2Freq* frec_step = &freq_at(step_ctx);
    p2_bump_sides(frec_step, ctx15, 0, fold_sel, 4, 4);
    if( code ) {
      int32_t n2_half = (code-1)>>1;
      int32_t hi_nibble = (uint8_t)pair_ctx&0xF0;
      if( hi_nibble>=0xF0||(cx[0].update_binary_pair(cx[0].model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
        cx[0].update_binary_pair(cx[0].model_strip(pair_ctx-16), n2_half);
      }
    }
    P2Freq* prec0 = &frec_step[0];
    p2_freq_add(prec0, fold_sel, 6);
    if( !plane_idx||freq_at(ctx).step>0x100u ) {
      is_dec = code ? 2-is_dec : 0;
      for( int32_t k = 0; k<5; ++k )
        bump_bank(k, step_ctx, ctx15, fold_sel, is_dec);
    }
    return step_ctx;
  }
  void encode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    int32_t sample = out[at];
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t resid = (uint8_t)(sample-pred);
    int32_t code = fold[resid];
    const int32_t was = out[at];
    const int32_t clean = (uint8_t)(pred+unfold[code]);
    const int32_t recon = (uint8_t)(clean+was-sample);
    if( was-recon<-CN_drift_limit||was-recon>CN_drift_limit ) {
      code = fold_hi[resid];
    } else {
      sample = clean;
      out[at] = recon;
    }
    freq_at(ctx).encode_symbol(cx, ctx_pair, code, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx);
    alt_p2_model(sample, code, sample-pred);
  }

  void decode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t code = freq_at(ctx).decode_symbol(cx, ctx_pair, &apm, apm_context(), (int32_t)MakeCellMap(ctx), &class_mix, &pz_seat[idx_bound(MakeSeat(ctx), kSeatCount)], &pz_signs[(ctx>>6)%243], &fast_at(ctx), &sign_apm, sign_map_ctx);
    const int32_t sample = (uint8_t)(pred+unfold[code]);
    alt_p2_model(sample, code, sample-pred);
    out[at] = sample;
  }

  template <int32_t f_DEC> void code_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    if constexpr( f_DEC )
      decode_sample(out, at, refa, refb);
    else
      encode_sample(out, at, refa, refb);
  }

  void copy_row0_down(int32_t width) {
    const int32_t row_bytes = sizeof(P2Ctx)*(width+kRowMargin+kRowTail);
    for( int32_t i = 0; i<3; ++i ) memcpy(buf[1+i], buf[0], row_bytes);
  }

  void seed_history(int32_t width) {
    P2Ctx*const here = cursor[0];
    for( int32_t i = 0; i<5; ++i ) here[i] = here[-1];
    P2Ctx*const up = cursor[0]-width;
    for( int32_t i = 0; i<8; ++i ) up[-1-i] = up[i];
    copy_row0_down(width);
  }

  void seed_row0(int32_t width) {
    for( int32_t x = 0; x<width+13; ++x ) {
      P2Ctx &rec = buf[0][x];
      rec.val = 256;
      rec.dval = 256;
      rec.err = -16;
      rec.sign = 1;
      rec.mag = 3;
      rec.aerr = 512;
      rec.dupright = 512;
      rec.dupleft = 512;
      rec.dup = 1024;
      rec.dleft = 256;
    }
    copy_row0_down(width);
    for( int32_t i = 0; i<5; ++i ) cursor[i] = buf[i]+width+8;
  }

  void add_bias(int32_t* acc) const {
    const P2Ctx &rec = cursor[0][-1];
    acc[0] += 32*rec.dupleft;
    acc[1] += 32*rec.dupright;
    acc[2] += 32*rec.dleft;
    acc[3] += 32*rec.dup;
  }

  void start_row() {
    NbRow*const last = cur[-1];
    cur[1] = last;
    cur[0] = last;
    NbRow**const was_row0 = row0;
    row0 = row1;
    row1 = was_row0;
    cur = row0+2;
    above = row1+2;
    cur[-1] = above[0];
    cur[-2] = above[0];
    memset(p2_row, 0, 15);
    for( int32_t i = 0; i<4; ++i ) bias[i] = 0.0f;
    memset(p2_row, 0, sizeof p2_row);
    {
      P2Ctx*const here = cursor[0];
      for( int32_t i = 0; i<5; ++i ) here[i] = here[-1-i];
    }
    ring_advance(buf, cursor, kRowMargin);
    cursor[0][-1] = cursor[1][0];
    {
      P2Ctx*const here = cursor[0];
      P2Ctx*const up = cursor[1];
      for( int32_t i = 0; i<7; ++i ) here[-2-i] = up[1+i];
    }
    cursor[0][0].dval = 0;
  }

  // `keep` is set for every tile after the first of a tiled encode: the block
  // is the same one this plane used last tile, and everything it learned there
  // is still wanted.  What is redone either belongs to the new tile's geometry
  // (the row buffers, the cursors, the running context) or to its descriptors
  // (the reference flag, the deadzone, the bands), and the pure tables are
  // rebuilt because rebuilding them is cheaper than deciding not to.
  AltP2Block*alt_p2_alloc(BMFState* state, int32_t img_w, int32_t plane, int32_t keep = 0) {
    cx = state;
    uint32_t done;
    uint32_t j, p, n;
    plane_idx = plane;
    if( !keep ) {
      for( j = 0; j<(uint32_t)kP2CounterCount; ++j ) {
        p2_ctr[j].weighted = 0;
        p2_ctr[j].rate = 5;
        p2_ctr[j].b1 = 2;
      }
      for( uint32_t e = 0; e<(uint32_t)kP2FreqCount; ++e ) {
        pz_fast[e] = adapt_init();
        freq[e].f[0] = 2048;
        freq[e].f[1] = 2816;
        freq[e].f[2] = 2816;
        freq[e].step = 4096;
      }
    }

    int32_t dz = 4*cx[0].near_lossless_q+1;
    int32_t band = 16*cx[0].near_lossless_q;
    has_ref = (cx[0].plane_desc[cx[0].plane_desc[plane_idx].src_plane].flags&desc_has_refs)!=0;
    cx[0].deadzone_hi = dz;
    cx[0].deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = row0_store;
    if( !keep )
      nb_weights[0].w[14][2] = idx_f(P2_msscale_seed, 0);
    row1 = row1_store;
    cur = row0+img_w+2;
    if( img_w>-4 ) {
      uint32_t pairs = (img_w+4)/2;
      if( pairs ) {
        for( p = 0; p<pairs; ++p ) {
          row1[2*p] = &nb_weights[0];
          row0[2*p] = &nb_weights[0];
          row1[2*p+1] = &nb_weights[0];
          row0[2*p+1] = &nb_weights[0];
        }
        done = 2*p+1;
      } else {
        done = 1;
      }
      if( (uint32_t)(img_w+4)>(done-1) ) {
        row1[done-1] = &nb_weights[0];
        row0[done-1] = &nb_weights[0];
      }
    }
    uint32_t row = 0;

    int32_t row_bytes = 18*max32(img_w, 3)+kP2RowPad;
    for( row = 0; row<5; ++row )
      buf[row] = row_store[row];
    memset(buf[0], 0, row_bytes);
    for( int32_t i = 0; i<4; ++i )
      cx[0].ctx_bias[i] = 0;
    cursor[0] = buf[0]+8;
    for( uint32_t i = 0; i<0x104; ++i )
      ctx_delta[i] = (int32_t)MakeCtxDelta(plane_idx, MakeP2Ctx(i));
    for( n = 0; n<0x78; ++n )
      nb_ctx[n] = (int32_t)MakeP2Len((uint32_t)n);
    ctx = 15;
    adapt_tables_init();
    cx[0].alt_init_tables(fold, unfold);
    if( !keep ) {
      apm.Init();
      sign_apm.Init();
      class_mix.Init();
      // Start at "trust this cell": the two coarse views have to earn their
      // weight.  A mixer that starts at a third each has to unlearn that, and on
      // a plane of a few thousand pixels it never finishes -- which doubled the
      // cost of a 64x48 alpha gradient.
      for( int32_t k = 0; k<Mix4::kSets; ++k ) {
        const int32_t weights[4] = {65536, 0, 0, 0};
        class_mix.seed(k, weights);
      }
      for( int32_t k = 0; k<64; ++k )
        pz_seat[k] = adapt_init();
      for( int32_t k = 0; k<243; ++k )
        pz_signs[k] = adapt_init();
      for( int32_t c = 0, w1 = 1<<kP2MixShift; c<5; ++c, w1 *= P2_ctxw_ratio ) {
        ctx_w[c].w[0] = 0;
        ctx_w[c].w[1] = w1;
        ctx_w[c].w[2] = 2*w1;
      }
    }
    return this;
  }

  P2Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  NbRow* row0_store[kMaxWidth+4];
  NbRow* row1_store[kMaxWidth+4];
  BMFState* cx;
};
