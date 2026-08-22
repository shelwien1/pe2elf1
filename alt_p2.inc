// alt_p2.inc -- AltP2Block: the NLMS predictor, bias-bank cascade and residual coder (predictor 2)
// Included from bmf.cpp; not a standalone translation unit.

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
    if( budget<=0x100 ) {
      if( step<=0x20u )
        cut = budget>16 ? 2 : 0;
      else
        cut = 32;
      step = budget-cut;
    } else {
      step >>= 1;
    }
    return cut;
  }

  template <int32_t f_DEC> int32_t code_three_way(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    uint32_t cum;
    uint16_t st, * slot;
    uint16_t down;
    uint16_t* base = &f[0];
    int32_t c01 = f[1]+f[0];
    uint32_t tot = c01+f[2];
    if constexpr( f_DEC ) {
      int32_t target = cx[0].rc.get_freq(tot);
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
    uint32_t tot_1 = cum+slot[0];
    if constexpr( f_DEC )
      cx[0].rc.decode(cum, tot_1, tot);
    else
      cx[0].rc.encode(cum, tot_1, tot);
    uint16_t f_before = slot[0];
    if( slot[0]>0x4000u ) {
      halve_counts(f, 3);
      st = step;
      if( st<=256 ) {
        if( st<=32 )
          down = st>16 ? 2 : 0;
        else
          down = 32;
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

  void encode_symbol(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    code_three_way<0>(cx, ctx_pair, sym);
  }

  int32_t decode_symbol(BMFState* cx, const uint32_t* ctx_pair) {
    return code_three_way<1>(cx, ctx_pair, 0);
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
  r[0].f[slot] += (num*(uint32_t)r[0].step)>>4;
}

void p2_bump_sides(P2Freq* group, int32_t ctx15, int32_t lo, int32_t slot, uint32_t up, uint32_t dn) {
  if( ctx15<15 ) p2_freq_add(&group[1], slot, up);
  if( ctx15>lo ) p2_freq_add(&group[-1], slot, dn);
}

void p2_nudge(P2Count* p, int32_t res, int32_t shift = 2) {
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
    w[14][0] = 47.0f;
    w[14][1] = 169.2f;
    w[14][2] = 1.0f;
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

int32_t ctx_quant(int32_t run, int32_t r0, int32_t r1, int32_t r2, int32_t sum, int32_t s0, int32_t s1, int32_t s2) {
  return CtxIdx{}.bits<13, 2>((run>r0)+(run>r1)+(run>r2)).bits<11, 2>((sum>s0)+(sum>s1)+(sum>s2));
}

int32_t over_thresholds(int32_t n, const int32_t* t, int32_t lo, int32_t hi, int32_t scale) {
  int32_t over = 0;
  for( int32_t k = hi; k>=lo; --k ) over += n>scale*t[k];
  return over;
}

const int32_t p2_band_edges[5] = {43, 17, 9, 5, 2};

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
  NbRow nb_weights[1088];
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
  int16_t nb_id[1920];
  static const int32_t kP2CounterCount = 5*32768;
  P2Count p2_ctr[kP2CounterCount];

  static uint32_t slot(int32_t bank, int32_t ctx) {
    return CtxIdx{}.bits<0, 15>(ctx).digit<32768, 5>(bank);
  }

  void seat_nb_row(int32_t nb_slot, const P2Ctx* c0) {
    const int32_t at = nb_id[nb_slot];
    if( !at ) {
      nb_id[nb_slot] = ++nb_id_used;

      nb_cur = &nb_weights[nb_id_used];
      return;
    }
    NbRow*const wrow = &nb_weights[at];
    nb_cur = wrow;
    const float err = ((float)c0[-1].val-(nb_dot(wrow[0].w, p2_row)+bias[0]))*2.0999999f;
    const float floor_ = 7744.0f*wrow[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        nlms_step(wrow, j, k, p2_row[j][k], err, bmf_p2_ms_rate, floor_);
  }

  int32_t p2_pred_at(int32_t bank, int32_t bctx) {
    const P2Count &c = p2_ctr[slot(bank, bctx)];
    return p2_pred(c.weighted, c.rate);
  }

  int32_t step_bank(int32_t bank_no, uint32_t word, int32_t run_in) {
    const int32_t bank = (int32_t)(word>>11);
    bank_ctx[bank_no] = bank;
    const int32_t pred = p2_pred_at(bank_no, bank);
    nb_sum[2*bank_no+1] = pred;
    const int32_t run = pred+run_in;
    nb_sum[2*bank_no] = run;
    return run;
  }

  static const int32_t kP2FreqCount = 15552;
  P2Freq freq[kP2FreqCount];
  uint8_t _pad3[8];

  void nlms_step(NbRow* row, int32_t j, int32_t k, float x, float err, float ms_rate, float floor_) {
    const float ms = row[0].w[7+j][k]+(x*x-row[0].w[7+j][k])*ms_rate;
    row[0].w[7+j][k] = ms;
    row[0].w[j][k] += cx[0].p2_rate[j][k]*err*x/(ms+floor_);
  }

  void nlms_track_two_rows(NbRow* fast, NbRow* slow, float err_fast, float err_slow, float conf) {
    const float floor_fast = 26896.0f*fast[0].w[14][2];
    const float floor_slow = 5041.0f*slow[0].w[14][2];
    const float rate_slow = 0.013f*conf;
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k ) {
        const float x = p2_row[j][k];
        nlms_step(fast, j, k, x, err_fast, 0.05f, floor_fast);
        nlms_step(slow, j, k, x, err_slow, rate_slow, floor_slow);
      }
  }

  void nlms_predict_and_correct(NbRow* row, float sample, float bias0) {
    const float err = sample-(bias0+nb_dot(row[0].w, p2_row));
    const float ms_scale = row[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        row[0].w[j][k] += cx[0].p2_rate[j][k]*err*p2_row[j][k]/(row[0].w[7+j][k]+ms_scale*529.0f);
    ++row[0].uses;
    row[0].w[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
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
    ctx_w[0].sel = (run4<1840)+(run4<272);
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
    const int32_t ctx15 = magsum>=960 ? 15 : nb_ctx[magsum>>3];
    const int32_t ctx_idx = clamp32((run4+7)>>4, 0, 255);
    ctx_pair[0] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx+4]);
    ctx_pair[1] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx]);
    ctx = CtxIdx{}.bits<0, 4>(ctx15).bit<4>(flat_a==0).bit<5>(flat_b==0).raw(mixer_fwd(ctx_w, 5));
    return ctx_idx;
  }
  void code_banks(int32_t sample16) {
    for( uint32_t bank = 0; bank<5; ++bank ) {
      const int32_t bctx = bank_ctx[bank];
      const int32_t res = sample16-nb_sum[2*bank];
      P2Count* node0 = &p2_ctr[slot(bank, bctx)];
      const int32_t w0 = res+(uint16_t)node0[0].weighted;
      node0[0].weighted = w0;
      int32_t countdown = node0[0].b1;
      if( !countdown )
        continue;
      const int32_t w0b = w0+4*((res>cx[0].deadzone_hi)-(res<cx[0].deadzone_lo));
      node0[0].weighted = w0b;
      uint32_t ctxw = bctx;
      if( (int32_t)abs32(res)<38 ) {
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
      P2Count* mir = &p2_ctr[slot(bank, ctxw^kBankMirrorMask)];
      __builtin_prefetch(mir, 0, 1);
      const int32_t res2 = nb_sum[2*bank+1]+res;
      int32_t lowbits = ctxw&3;
      if( lowbits<3 )
        p2_update(cx, &node0[1], res2, 1);
      if( lowbits )
        p2_update(cx, &node0[-1], res2, 2);
      P2Count* bankp = &p2_ctr[slot(bank, 0)];
      static const uint32_t p2_bank_bits[11] = {
        0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10,
      };
      P2Count* direct[11], * mirror[11], * rot[11];
      for( int32_t k = 0; k<11; ++k ) {
        const uint32_t x = ctxw^p2_bank_bits[k];
        direct[k] = &bankp[x];
        mirror[k] = &bankp[x^kBankMirrorMask];
        rot[k] = &bankp[p2_ctx_rotate[(x>>2)&3]+(x&0xFFFFFFF3)];
        __builtin_prefetch(direct[k], 0, 1);
        __builtin_prefetch(mirror[k], 0, 1);
        __builtin_prefetch(rot[k], 0, 1);
      }
      const int32_t neg = -res2;
      const int32_t w_top = mir[0].weighted;
      const int32_t e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
      const int32_t bump = 32*((e_top>cx[0].deadzone_hi)-(e_top<cx[0].deadzone_lo));
      uint32_t w_new = bump+e_top+2;
      mir[0].weighted = (int16_t)(w_top+(w_new>>2));
      if( lowbits<3 )
        p2_nudge(&mir[1], -res2);
      if( lowbits<=0 ) {
        for( int32_t k = 0; k<11; ++k ) {
          p2_update(cx, direct[k], res2, 2);
          p2_update(cx, rot[k], res2, 3);
          p2_nudge(&direct[k][1], res2);
          p2_update(cx, mirror[k], -res2, 3);
        }
      } else {
        p2_nudge(&mir[-1], -res2, 3);
        p2_update(cx, direct[0], res2, 2);
        p2_update(cx, rot[0], res2, 3);
        walk_bank_bits(direct, mirror, rot, res2, lowbits<3);
        p2_update(cx, mirror[10], -res2, 3);
      }
    }
  }
  void walk_bank_bits(P2Count*const* direct, P2Count*const* mirror, P2Count*const* rot, int32_t res, bool nudge) {
    for( int32_t k = 0; k<11; ++k ) {
      if( k ) {
        p2_update(cx, direct[k], res, 2);
        p2_update(cx, rot[k], res, 3);
      }
      if( nudge )
        p2_nudge(&direct[k][1], res);
      p2_update(cx, &direct[k][-1], res, 3);
      if( k<10 )
        p2_update(cx, mirror[k], -res, 3);
    }
  }

  void bump_bank(int32_t k, uint32_t step_v, int32_t ctx15, int32_t fold_sel, int32_t is_dec) {
    const int32_t sel = ctx_w[k].sel;
    if( sel==1 ) {
      p2_freq_add(&freq[ctx_w[k].w[0]+step_v-ctx_w[k].w[1]], fold_sel, 4);
      const uint32_t off = ctx-ctx_w[k].w[1];
      P2Freq*const group = &freq[off+ctx_w[k].w[0]];
      const uint32_t next_group = ctx_w[k].w[2]+off;
      p2_freq_add(&group[0], is_dec, 3);
      if( ctx15<15 ) {
        p2_freq_add(&group[1], is_dec, 4);
        p2_freq_add(&freq[next_group+1], is_dec, 2);
      }
      if( ctx15>2 )
        p2_freq_add(&group[-1], is_dec, 6);
    } else {
      p2_freq_add(&freq[step_v+ctx_w[k].w[1]-ctx_w[k].w[2-sel]], fold_sel, 7);
      P2Freq*const group = &freq[ctx_w[k].w[1]+(ctx-ctx_w[k].w[sel])];
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
        ctx_pair[0] = ctx+ctx_delta[q2+4];
        ctx_pair[1] = ctx+ctx_delta[q2];
        P2Freq &fr = freq[ctx+ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+ctx_w[2].w[rec[-2].sign]+ctx_w[1].w[rec[-1].sign]+ctx_w[0].w[(q2<115)+(q2<17)]+ctx_w[4].w[1]];
        if constexpr( f_DEC ) {
          out[0] = (uint8_t)(((uint16_t)rec[-1].val>>4)+(uint8_t)unfold[fr.decode_symbol(cx, ctx_pair)]);
        } else {
          code0 = fold_or_refuse(out, p[0], (uint16_t)rec[-1].val>>4, fold, fold_hi, unfold);
          fr.encode_symbol(cx, ctx_pair, code0);
          ++p;
        }
        int32_t val = 16*(uint8_t)*out;
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
            code2 = code = freq[ctx].decode_symbol(cx, pair);
            outp[k] = (uint8_t)(pred+(unfold[code]));
          } else {
            code = fold_or_refuse(&outp[k], srcp[k], pred, fold, fold_hi, unfold);
            code2 = code;
            freq[ctx].encode_symbol(cx, pair, code);
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
    nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD).digit<64, 5>(gA).digit<320, 6>(band);
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
    ctx0_lo = CtxIdx{}.bit<19>(c1[1].err+c1[0].err+c1[-1].err+c0[-1].err<0).bit<18>(c3[0].err+c0[-2].err+2*c0[-4].err<0).bit<17>(c1[-3].err+c0[-3].err+c0[-5].err+c0[-7].err<0).bit<16>(c0[-1].err<0).bit_of<15>((uint16_t)c0[-3].err).bits<13, 2>((filt>3536)+(filt>720)+(filt>288)).bits<11, 2>(((g3pair+g3sum)>752)+((g3pair+g3sum)>400)+((g3pair+g3sum)>240));
    if( refa ) {
      ctx0 = CtxIdx{}.bit<25>(rb0[-1].err<0).bit<24>(rb0[0].err<0).bit<23>(ra0[-1].err<0).bit<22>(rb0[0].err+rb0[-2].err<0).bit<21>(ra0[-2].err+ra0[0].err<0).bit<20>(c0[-2].err<0).raw(ctx0_lo);
    } else {
      ctx0 = CtxIdx{}.bit<25>(c1[3].err+c0[-3].err+c0[-7].err+c0[-5].err<0).bit<24>(c2[1].err+c2[0].err+c0[-4].err+c0[-8].err<0).bit<23>(c0[-4].err+c0[-6].err<0).bit<22>(c0[-4].err<0).bit<21>(c0[-5].err<0).bit<20>(c0[-7].err<0).raw(ctx0_lo);
    }
    const int32_t run0 = step_bank(0, ctx0, filt);
    int32_t nb4_4 = c4[4].val;
    sum_all = ((g3pair<<9)+sum_all)>>13;
    int32_t d_run0 = c0[-5].val-run0;
    int32_t d_up = nb4_4-run0;
    int32_t up5 = c1[5].val;
    int32_t d_up5 = up5-run0;
    if( refa ) {
      int32_t dv_now = c0[0].dval;
      ctx1 = CtxIdx{}.bit_of<25>(c0[-1].val+ra0[0].val-ra0[-1].val-run0).bit<24>(ra2[0].dval+ra1[0].dval-2*ra0[0].dval<0).bit_of<23>(dv_now+c0[-3].dval-run0-(c1[2].dval-c1[5].dval)).bit_of<22>(dv_now+c0[-5].dval-run0).bit_of<21>(dv_now+c1[0].dval+rb1[2].dval-rb2[2].dval-run0).bit_of<20>(dv_now+c0[-1].dval+rb0[0].dval-rb0[-1].dval-run0).bit_of<19>(c2[-1].dval+dv_now+ra0[0].dval-ra2[-1].dval-run0).bit_of<18>(dv_now+c1[2].dval+ra0[0].dval-ra1[2].dval-run0).bit_of<17>(d_up5).bit_of<16>(d_up).bit_of<15>(d_run0).raw(ctx_quant(run0, 2256, 1056, 144, sum_all, 55, 10, 24));
    } else {
      int32_t d_run0b = run0-c0[-3].val;
      ctx1 = CtxIdx{}.bit_of<25>(nb4_4-c3[0].val+run0-c1[4].val).bit_of<24>(c1[-5].val-c1[-2].val+d_run0b).bit_of<23>(d_run0b+c1[2].val-up5).bit_of<22>(c4[3].val-c2[2].val+run0-c2[1].val).bit_of<21>(c4[-1].val-c3[0].val+run0-c1[-1].val).bit_of<20>(c1[0].val-c1[2].val+run0-c0[-2].val).bit_of<19>(-d_up5).bit_of<18>(-d_up).bit_of<17>(run0-c4[2].val).bit_of<16>(run0-c4[-3].val).bit_of<15>(-d_run0).raw(ctx_quant(run0, 2400, 1024, 240, sum_all, 39, 24, 11));
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
      ctx2 = CtxIdx{}.bit_of<25>(c0[0].dval+c1[-1].dval-run1-(c2[0].dval-dv1)).bit_of<24>(c0[0].dval+c0[-1].dval-run1-(c1[0].dval-dv1)).bit_of<23>(rb0[0].val-run1+cx2_val0-rb2[0].val).bit_of<22>(d_ra+cx2_val0-ra2[0].val).bit_of<21>(d_ra+c1[-1].val-ra1[-1].val).bit_of<20>(c0[0].dval-c0[-2].dval+2*c0[-1].dval-run1).bit_of<19>(2*cx1_val-run1-cx2_val0).bit_of<18>(-lap).bit_of<17>(dtop2).bit_of<16>(dvsum2).bit_of<15>(208-rb0[0].val).raw(ctx_quant(run1, 2576, 1280, 640, sum_all, 33, 12, 4));
    } else {
      ctx2 = CtxIdx{}.bit_of<25>(run1+3*(cx2_val0-cx1_val)-c3[0].val).bit_of<24>(run1+c4[0].val-(c2[2].val+c2[-2].val)).bit_of<23>(c3[2].val-c2[-1].val+run1-c1[3].val).bit_of<22>(c3[1].val-c1[1].val-dtop2).bit_of<21>(c3[0].val-cx1_val-dtop2).bit_of<20>(lap).bit_of<19>(run1-c4[3].val).bit_of<18>(c3[1].val-run1).bit_of<17>(run1-c4[0].val).bit_of<16>(run1-c3[-2].val).bit_of<15>(-dvsum2).raw(ctx_quant(run1, 2464, 1216, 688, sum_all, 58, 25, 13));
    }
    const int32_t run2 = step_bank(2, ctx2, run1);
    int32_t run_dv3 = run2-c0[0].dval;
    const uint32_t ctx3 = CtxIdx{}.bit_of<25>(3*(c0[-2].val-c0[-1].val)+run2-c0[-3].val).bit_of<24>(c2[1].dval-((uint32_t)(c1[1].dval+c1[2].dval+c1[-1].dval+c1[0].dval)>>1)+run_dv3).bit_of<23>(c2[-3].val-c1[-2].val+run2-c1[-1].val).bit_of<22>(-(c2[2].val+run2-2*c1[1].val)).bit_of<21>(c1[-2].dval-c0[-2].val+run2-c1[0].dval).bit_of<20>(run2-c1[3].val).bit_of<19>(run_dv3-c2[1].dval).bit_of<18>(run2-c3[0].val).bit_of<17>(2*run2-c1[0].dval-(c1[0].val+c0[0].dval)).bit_of<16>(run2-c0[-1].dval-c0[0].dval).bit_of<15>(run2-c0[-3].val).raw(ctx_quant(run2, 2896, 1568, 592, sum_all, 37, 19, 9));
    const int32_t run3 = step_bank(3, ctx3, run2);
    int32_t dv_now4 = c0[0].dval;
    int32_t run_dv4 = run3-dv_now4;
    int32_t run_up4 = run3+c3[0].val;
    const uint32_t ctx4 = CtxIdx{}.bit_of<25>(run3-((uint32_t)(c1[1].val+c1[-1].val+2*c1[0].val)>>2)).bit_of<24>(run_up4-c0[-2].val-(c3[2].dval+dv_now4)).bit_of<23>(run3-2*c0[-2].dval-(dv_now4-c0[-4].dval)).bit_of<22>(run3-c0[-3].dval-dv_now4-(c1[0].val-c1[-3].val)).bit_of<21>(run_up4-dv_now4-(c1[1].val+c2[-1].dval)).bit_of<20>(c2[-2].val+run3-2*c1[-1].val).bit_of<19>(c2[0].dval-2*c1[0].dval+run_dv4).bit_of<18>((run3-c0[-1].val)-(c1[0].val-c1[-1].val)).bit_of<17>(run_dv4-c1[4].dval).bit_of<16>(run_dv4-c1[-2].dval).bit_of<15>((uint16_t)run3-(uint16_t)c0[-4].dval-(uint16_t)dv_now4).raw(ctx_quant(run3, 3056, 1952, 368, sum_all, 39, 21, 10));
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
    const float cov = (((err_own*d_pred)-wrow_cur[0].w[14][0])*0.001f)+wrow_cur[0].w[14][0];
    const float var = (wrow_cur[0].w[14][1]+0.000099999997f)+(((d_pred*d_pred)-wrow_cur[0].w[14][1])*0.001f);
    wrow_cur[0].w[14][1] = var;
    float cov_kept = 0.1f*var;
    if( cov_kept<=cov )
      cov_kept = fminf(var, cov);
    wrow_cur[0].w[14][0] = cov_kept;
    const float conf = (1.0f-(cov_kept/(var+576.0f)))*2.0f;
    nlms_track_two_rows(wrow_cur, wrow_b, (sample-mixed)*2.5999999f, err_own*conf, conf);
    nlms_predict_and_correct(wrow_cur, sample, centre);
    cur[0] = nb_cur;
    ++cur;
    ++above;
    code_banks(sample16);
    int32_t ctx_l = ctx;
    ++cursor[0];
    P2Freq* frec = &freq[ctx_l];
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
    if( freq[ctx].step<=0x1Au )
      return ctx;
    int32_t fold_sel = 2-(fold[(uint8_t)-resid_in]&1);
    if( !fold[(uint8_t)-resid_in] )
      fold_sel = fold[(uint8_t)-resid_in];
    const uint32_t step_ctx = CtxIdx{}.bits<0, 6>(ctx).raw(mixer_rev(ctx_w, 5));
    P2Freq* frec_step = &freq[step_ctx];
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
    if( !plane_idx||freq[ctx].step>0x100u ) {
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
    if( was-recon<-16||was-recon>16 ) {
      code = fold_hi[resid];
    } else {
      sample = clean;
      out[at] = recon;
    }
    freq[ctx].encode_symbol(cx, ctx_pair, code);
    alt_p2_model(sample, code, sample-pred);
  }

  void decode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t code = freq[ctx].decode_symbol(cx, ctx_pair);
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

  AltP2Block*alt_p2_alloc(BMFState* state, int32_t img_w, int32_t plane) {
    cx = state;
    uint32_t done;
    uint32_t j, p, n;
    plane_idx = plane;
    for( j = 0; j<(uint32_t)kP2CounterCount; ++j ) {
      p2_ctr[j].weighted = 0;
      p2_ctr[j].rate = 5;
      p2_ctr[j].b1 = 2;
    }
    for( uint32_t e = 0; e<(uint32_t)kP2FreqCount; ++e ) {
      freq[e].f[0] = 2048;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
    }

    int32_t dz = 4*cx[0].near_lossless_q+1;
    int32_t band = 16*cx[0].near_lossless_q;
    has_ref = (cx[0].plane_desc[cx[0].plane_desc[plane_idx].src_plane].flags&desc_has_refs)!=0;
    cx[0].deadzone_hi = dz;
    cx[0].deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = row0_store;
    nb_weights[0].w[14][2] = 1.0f;
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
    int32_t lvl = 0;
    cursor[0] = buf[0]+8;
    for( uint32_t pair = 0; pair<0x82; ++pair ) {
      ctx_delta[2*pair] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl);
      const int32_t lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl1);
      lvl = (2*pair+1==p2_ctx_edges[lvl1])+lvl1;
    }
    int32_t len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      int32_t len1 = (2*n==p2_len_edges[len])+len;
      nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    ctx = 15;
    cx[0].alt_init_tables(fold, unfold);
    for( int32_t c = 0, w1 = 64; c<5; ++c, w1 *= 3 ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    return this;
  }

  P2Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  NbRow* row0_store[kMaxWidth+4];
  NbRow* row1_store[kMaxWidth+4];
  BMFState* cx;
};
