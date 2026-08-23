// alt_p1.inc -- AltP1Block: the MED-predictor residual model (predictor 1)
// Included from bmf.cpp; not a standalone translation unit.

struct AltP1Block {
  static const int32_t kRowMargin = P1_row_margin;
  static const int32_t kRowTail = P1_row_tail;
  int32_t width;
  int32_t height;
  int32_t pred;
  int32_t ctx[5];
  Apm slot_apm;
  CtxWeight ctx_w[9];
  P1Ctx* buf[5];
  P1Ctx* cursor[5];
  uint8_t level_of[512];
  uint8_t slot_of[256];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  uint8_t unfold[256];
  int32_t group_of[512];
  // Sized by its own index declaration.  The Volume folds in both builds:
  // ctxw_range is frozen, being 3^9 rather than a knob of its own.
  static const int32_t kP1CounterCount = P1_P1Result_Volume;
  CounterNode counters[kP1CounterCount];

  // The selector mixer's weights are seeded geometrically and then added to and
  // subtracted from a context to reach a neighbouring cell, so a swept seed or
  // ratio walks past the end of the table.  Every access goes through here; the
  // shipping build folds the bound's operand, not the compare, because the
  // index is data.
  CounterNode &ctr_at(uint32_t i) {
    return counters[idx_bound(i, kP1CounterCount)];
  }
  void advance_row() {
    {
      P1Ctx*const here = cursor[0];
      for( int32_t k = 0; k<kRowTail; ++k )
        here[k] = here[-1-k];
    }
    ring_advance(buf, cursor, kRowMargin);
    cursor[0][-4] = cursor[1][3];
    {
      P1Ctx*const here = cursor[0];
      P1Ctx*const up = cursor[1];
      for( int32_t i = 0; i<3; ++i )
        here[-3+i] = up[2-i];
    }
  }

  void seed_activity(bool signed_mirror) {
    const P1Ctx*const row0 = cursor[0], *const row2 = cursor[2], *const row4 = cursor[4];
    ctx[2] = 0;
    for( int32_t lane = 0; lane<2; ++lane ) {
      const int32_t mirror = lane-2;
      int32_t sum = signed_mirror ? (int8_t)row2[mirror].mag+(int8_t)row4[mirror].mag : row2[mirror].mag+row4[mirror].mag;
      for( int32_t k = 0; k<3; ++k )
        sum += row2[lane+2*k].mag+row4[lane+2*k].mag;
      ctx[3+lane] = sum+row0[lane-4].mag+row0[lane-2].mag;
    }
  }

  int32_t encode_sample(uint8_t* dst, int32_t at, int32_t cur) {
    const int32_t p = (uint8_t)pred;
    const int32_t resid = (uint8_t)(cur-p);
    int32_t code = fold[resid];
    const int32_t recon = (uint8_t)(unfold[code]+p);
    const int32_t out = (uint8_t)(recon+dst[at]-cur);
    if( dst[at]-out<-CN_drift_limit||dst[at]-out>CN_drift_limit ) {
      code = fold_hi[resid];
    } else {
      cur = recon;
      dst[at] = out;
    }
    alt_p1_encode_symbol(cx, &ctr_at(ctx[0]), ctx[1], code, &slot_apm);
    return cur;
  }

  int32_t decode_sample() {
    const int32_t code = alt_p1_decode_symbol(cx, &ctr_at(ctx[0]), ctx[1], &slot_apm);
    return (uint8_t)((uint8_t)pred+unfold[code]);
  }

  void record_sample(int32_t val) {
    cursor[0][0].sym = val;
    cursor[0][0].mag = abs32(val-pred);
    ctx[3+ctx[2]] += cursor[0][0].mag-cursor[0][-4].mag-(cursor[4][-2].mag-cursor[4][6].mag)-(cursor[2][-2].mag-cursor[2][6].mag);
    ctx[2] = ctx[2]==0;
    if( ctr_at(ctx[0]).total<0x4000u )
      update_model();
    for( int32_t k = 0; k<5; ++k )
      ++cursor[k];
  }

  int32_t cross_grad(int32_t here, int32_t guess, const AltP1Block* nb) {
    const P1Ctx* r = nb[0].cursor[0];
    return here-(uint32_t)guess+r[-1].sym-r[-2].sym;
  }

  int32_t nb_resid(const AltP1Block* nb) {
    return nb[0].cursor[0][-1].sym-nb[0].pred;
  }

  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    bool pick;
    int32_t act_all;
    uint32_t quiet;
    P1Ctx* cursor1 = cursor[1];
    int32_t guess = cursor1[0].sym;
    P1Ctx* cur = cursor[0];
    int32_t west = cur[-1].sym;
    int32_t northwest = cursor1[-1].sym;
    if( west<guess ) {
      if( northwest>=west ) {
        int32_t plane_b = guess+west-northwest;
        pick = northwest<guess;
        guess = cur[-1].sym;
        if( pick )
          guess = plane_b;
      }
    } else if( northwest<=west ) {
      int32_t plane_a = guess+west-northwest;
      pick = northwest<=guess;
      guess = cur[-1].sym;
      if( !pick )
        guess = plane_a;
    }
    pred = guess;
    P1Ctx* cursor2 = cursor[2];
    int32_t act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2[0].mag)+6*cur[-1].mag+4*(cursor1[0].mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
    const int32_t west_grad = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
    const int32_t north_grad = -guess-cursor1[0].sym+cursor1[1].sym+cur[-1].sym;
    if( nb0 ) {
      const P1Ctx* r0 = nb0[0].cursor[0];
      if( nb1 ) {
        const P1Ctx* r1 = nb1[0].cursor[0];
        act_all = act+2*(r1[-1].mag+r0[-1].mag);
        ctx_w[5].sel = cursor1[0].sym-(uint32_t)guess+r0[-1].sym-nb0[0].cursor[1][-1].sym;
        ctx_w[6].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[7].sel = cross_grad(cur[-1].sym, guess, nb1);
        ctx_w[8].sel = nb_resid(nb1);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r1[-1].mag+r0[-1].mag<P1_quiet_thr);
      } else {
        act_all = cursor1[3].mag+act+3*r0[-1].mag;
        ctx_w[5].sel = west_grad;
        ctx_w[6].sel = west_grad;
        ctx_w[7].sel = north_grad;
        ctx_w[8].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r0[-1].mag<8);
      }
    } else {
      P1Ctx* cursor4 = cursor[4];
      act_all = cursor4[0].mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
      ctx_w[5].sel = west_grad;
      ctx_w[6].sel = 2*cursor1[0].sym-cursor2[0].sym-(uint32_t)guess;
      ctx_w[7].sel = north_grad;
      ctx_w[8].sel = -3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess;
      ctx_w[3].sel = cursor1[2].sym-(uint32_t)guess;
      quiet = cur[0].mag+cursor4[0].mag+cursor[3][0].mag+cursor2[0].mag+cursor1[0].mag==0;
    }
    // Bounded to the ladder tables: a shallower shift leaves more of the
    // activity sum in the index than level_of and group_of have room for.
    int32_t act_q = (int32_t)idx_bound((uint32_t)((act_all+P1_act_round)>>idx_clamp(P1_act_shift, 1, 8)),
                                      sizeof level_of);
    int32_t act_lvl = level_of[act_q];
    int32_t step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    ctx_w[0].sel = (guess>P1_guess_thr_0)+(guess>P1_guess_thr_1);
    ctx_w[1].sel = tri_sign(cursor1[-1].sym-cursor1[0].sym);
    ctx_w[2].sel = tri_sign(cursor1[-1].sym-cur[-1].sym);
    ctx_w[3].sel = tri_sign(ctx_w[3].sel);
    ctx_w[4].sel = tri_sign(cursor1[1].sym-guess, step);
    ctx_w[5].sel = tri_sign(ctx_w[5].sel, step);
    for( int32_t k = 6; k<=8; ++k )
      ctx_w[k].sel = tri_sign(ctx_w[k].sel);
    // The mixer sum is the most significant factor, so it enters as its own
    // digit rather than as an offset: its weights are multiples of
    // 1<<ctxw_shift, and the three packed factors fill exactly that many
    // low bits.  alt_p1_alloc checks the two agree.
    int32_t result = (int32_t)MakeP1Result(mixer_fwd(ctx_w, 9)>>kP1MixShift,
                                          (uint32_t)quiet, (uint32_t)(ctx[3+ctx[2]]==0),
                                          (uint32_t)act_lvl);
    ctx[0] = result;
    return result;
  }

  template <int32_t f_DEC> void d8_body(uint8_t* src, uint8_t* out) {
    if constexpr( f_DEC )
      cx[0].rc_begin_decode();
    else
      cx[0].rc_begin_encode();
    for( int32_t y = 0; y<height; ++y ) {
      advance_row();
      seed_activity(false);
      {
        uint8_t* q = out;
        for( int32_t x = 0; x<width; ++x ) {
          ctx_of(nullptr, nullptr);
          if constexpr( f_DEC ) {
            const int32_t code = alt_p1_decode_symbol(cx, &ctr_at(ctx[0]), ctx[1], &slot_apm);
            q[0] = (uint8_t)((uint8_t)pred+unfold[(uint8_t)code]);
          } else {
            const int32_t code = fold_or_refuse(q, src[0], (uint8_t)pred, fold, fold_hi, unfold);
            alt_p1_encode_symbol(cx, &ctr_at(ctx[0]), ctx[1], code, &slot_apm);
            ++src;
          }
          record_sample((uint8_t)q[0]);
          ++q;
        }
        out = q;
      }
    }
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }

  void update_selector(int32_t k, int32_t slot_f, int32_t slot_r, uint32_t ctx_alt, int32_t alt_lo, int32_t alt_hi) {
    const CtxWeight &cw = ctx_w[k];
    const int32_t sel = cw.sel;
    const int32_t here = ctx[0];
    if( sel==1 ) {
      int32_t mid = here-cw.w[1];
      CounterNode* lo = &ctr_at(mid+cw.w[0]);
      CounterNode* hi = &ctr_at(cw.w[2]+mid);
      lo[0].bump(slot_f, P1_sel1_ctr);
      hi[0].bump(slot_f, P1_sel1_ctr);
      if( (here&7)!=7 ) {
        lo[1].bump(slot_f, P1_sel1_up);
        hi[1].bump(slot_f, P1_sel1_up);
      }
      if( here&7 ) {
        lo[-1].bump(slot_f, P1_sel1_dn);
        hi[-1].bump(slot_f, P1_sel1_dn);
      }
    } else {
      ctr_at(cw.w[2-sel]+here-cw.w[sel]).bump(slot_f, P1_selN_far);
      CounterNode* mid = &ctr_at(cw.w[1]+here-cw.w[sel]);
      CounterNode* alt = &ctr_at(ctx_alt+cw.w[alt_lo]-cw.w[alt_hi]);
      mid[0].bump(slot_f, P1_selN_mid);
      alt[0].bump(slot_r, P1_selN_alt);
      if( (here&7)!=7 ) {
        mid[1].bump(slot_f, P1_selN_mid_up);
        alt[1].bump(slot_r, P1_selN_alt_up);
      }
      if( here&7 ) {
        mid[-1].bump(slot_f, P1_selN_mid_dn);
        alt[-1].bump(slot_r, P1_selN_alt_dn);
      }
    }
  }

  void update_model() {
    const uint32_t code_f = fold[(uint8_t)(cursor[0][0].sym-(uint8_t)pred)];
    const int32_t code_r = fold[(uint8_t)((uint8_t)pred-cursor[0][0].sym)];
    const int32_t tree_sym = (int32_t)(code_f-CN_slot_split)>>1;
    const int32_t slot_f = sym_slot(code_f);
    const int32_t slot_r = sym_slot(code_r);
    uint32_t ctx_alt = MakeP1Alt(mixer_rev(ctx_w, 9)>>kP1MixShift, (uint32_t)ctx[0]&31u);
    CounterNode* node_alt = &ctr_at(ctx_alt);
    node_alt[0].bump(slot_r, P1_bump_alt);
    const int32_t result = ctx[0];
    if( (result&7)!=7 ) {
      CounterNode* node_up = &ctr_at(result);
      uint32_t ctx_up = ((ctx[1]&7)<7)+ctx[1];
      node_up[1].bump(slot_f, P1_bump_up);
      if( slot_f>=CN_slot_split )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<IX_p1_ix_p0>(slot_f&1).bit<IX_p1_ix_p1>(node_up[1].escape_high(slot_f)).raw(ctx_up)), tree_sym);
    }
    if( result&7 ) {
      CounterNode* node_dn = &ctr_at(result);
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, P1_bump_dn);
      if( slot_f>=CN_slot_split )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<IX_p1_ix1_p0>(slot_f&1).bit<IX_p1_ix1_p1>(node_dn[-1].escape_high(slot_f)).raw(ctx_dn)), tree_sym);
    }
    if( ctr_at(result).total>=CN_nudge_limit )
      return;
    if( (result&7u)<7 ) {
      node_alt[1].bump(slot_r, P1_nudge_up);
    }
    if( result&7 ) {
      node_alt[-1].bump(slot_r, P1_nudge_dn);
    }
    if( slot_f>=CN_slot_split ) {
      const uint32_t key = CtxIdx{}.bit<IX_p1_key_p0>(slot_f&1).bit<IX_p1_key_p1>(ctr_at(result).escape_high(slot_f)).raw(ctx[1]);
      if( (key&0x38)>=0x38||(cx[0].update_binary_pair(cx[0].model_strip(key+8), tree_sym), (key&0x38)!=0) ) {
        cx[0].update_binary_pair(cx[0].model_strip(key-8), tree_sym);
      }
    }
    update_selector(0, slot_f, slot_r, ctx_alt, 0, 1);
    for( int32_t k = 1; k<9; ++k )
      update_selector(k, slot_f, slot_r, ctx_alt, 1, 2-ctx_w[k].sel);
  }

  AltP1Block*alt_p1_alloc(BMFState* state, int32_t img_w, int32_t img_h, int32_t plane) {
    cx = state;
    slot_apm.Init();
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      init_counter_node(&counters[ctr++]);
    while( ctr<kP1CounterCount );
    pred = 0;
    // The three ladders come from IDX threshold mappings: MakeLevel(x) is the
    // number of level edges below x, which is exactly what walking the edge
    // list a step at a time used to compute.
    int32_t k = 0;
    for( k = 0; k<0x200; ++k ) {
      level_of[k] = (uint8_t)MakeLevel((uint32_t)k);
      group_of[k] = CtxIdx{}.above<IX_p1_group_of_p0>(plane).bits<IX_p1_group_of_p1, IX_p1_group_of_w1>(MakeGroup((uint32_t)k));
    }
    for( k = 0; k<0x100; ++k )
      slot_of[k] = (uint8_t)CtxIdx{}.above<IX_p1_slot_of_p0>(MakeSlot((uint32_t)k));
    for( int32_t c = 0, w1 = 1<<kP1MixShift; c<9; ++c, w1 *= P1_ctxw_ratio ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    for( int32_t row = 0; row<5; ++row )
      buf[row] = row_store[row];
    cx[0].alt_init_tables(fold, (int8_t*)unfold);
    for( int32_t r = 0; r<width+10; ++r ) {
      for( int32_t i = 0; i<5; ++i )
        buf[4-i][r].sym = 72;
      for( int32_t i = 0; i<5; ++i )
        buf[4-i][r].mag = 0;
    }
    for( int32_t i = 0; i<5; ++i )
      cursor[i] = buf[i]+width+4;
    return this;
  }

  P1Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  BMFState* cx;
};
