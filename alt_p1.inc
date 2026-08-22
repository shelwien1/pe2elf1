// alt_p1.inc -- AltP1Block: the MED-predictor residual model (predictor 1)
// Included from bmf.cpp; not a standalone translation unit.

struct AltP1Block {
  static const int32_t kRowMargin = 4;
  static const int32_t kRowTail = 6;
  int32_t width;
  int32_t height;
  int32_t pred;
  int32_t ctx[5];
  CtxWeight ctx_w[9];
  P1Ctx* buf[5];
  P1Ctx* cursor[5];
  uint8_t level_of[512];
  uint8_t slot_of[256];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  uint8_t unfold[256];
  int32_t group_of[512];
  static const int32_t kP1CounterCount = 629856;
  CounterNode counters[kP1CounterCount];
  void advance_row() {
    {
      P1Ctx*const here = cursor[0];
      for( int32_t k = 0; k<6; ++k )
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
    if( dst[at]-out<-16||dst[at]-out>16 ) {
      code = fold_hi[resid];
    } else {
      cur = recon;
      dst[at] = out;
    }
    alt_p1_encode_symbol(cx, &counters[ctx[0]], ctx[1], code);
    return cur;
  }

  int32_t decode_sample() {
    const int32_t code = alt_p1_decode_symbol(cx, &counters[ctx[0]], ctx[1]);
    return (uint8_t)((uint8_t)pred+unfold[code]);
  }

  void record_sample(int32_t val) {
    cursor[0][0].sym = val;
    cursor[0][0].mag = abs32(val-pred);
    ctx[3+ctx[2]] += cursor[0][0].mag-cursor[0][-4].mag-(cursor[4][-2].mag-cursor[4][6].mag)-(cursor[2][-2].mag-cursor[2][6].mag);
    ctx[2] = ctx[2]==0;
    if( counters[ctx[0]].total<0x4000u )
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
        quiet = (r1[-1].mag+r0[-1].mag<16);
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
    int32_t act_q = (act_all+7)>>4;
    int32_t act_lvl = level_of[act_q];
    int32_t step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    ctx_w[0].sel = (guess>216)+(guess>22);
    ctx_w[1].sel = tri_sign(cursor1[-1].sym-cursor1[0].sym);
    ctx_w[2].sel = tri_sign(cursor1[-1].sym-cur[-1].sym);
    ctx_w[3].sel = tri_sign(ctx_w[3].sel);
    ctx_w[4].sel = tri_sign(cursor1[1].sym-guess, step);
    ctx_w[5].sel = tri_sign(ctx_w[5].sel, step);
    for( int32_t k = 6; k<=8; ++k )
      ctx_w[k].sel = tri_sign(ctx_w[k].sel);
    int32_t result = CtxIdx{}.bits<0, 3>(act_lvl).bit<3>(ctx[3+ctx[2]]==0).bit<4>(quiet).raw(mixer_fwd(ctx_w, 9));
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
            const int32_t code = alt_p1_decode_symbol(cx, &counters[ctx[0]], ctx[1]);
            q[0] = (uint8_t)((uint8_t)pred+unfold[(uint8_t)code]);
          } else {
            const int32_t code = fold_or_refuse(q, *src, (uint8_t)pred, fold, fold_hi, unfold);
            alt_p1_encode_symbol(cx, &counters[ctx[0]], ctx[1], code);
            ++src;
          }
          record_sample((uint8_t)*q);
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
      CounterNode* lo = &counters[mid+cw.w[0]];
      CounterNode* hi = &counters[cw.w[2]+mid];
      lo[0].bump(slot_f, 6);
      hi[0].bump(slot_f, 6);
      if( (here&7)!=7 ) {
        lo[1].bump(slot_f, 4);
        hi[1].bump(slot_f, 4);
      }
      if( here&7 ) {
        lo[-1].bump(slot_f, 3);
        hi[-1].bump(slot_f, 3);
      }
    } else {
      counters[cw.w[2-sel]+here-cw.w[sel]].bump(slot_f, 7);
      CounterNode* mid = &counters[cw.w[1]+here-cw.w[sel]];
      CounterNode* alt = &counters[ctx_alt+cw.w[alt_lo]-cw.w[alt_hi]];
      mid[0].bump(slot_f, 6);
      alt[0].bump(slot_r, 4);
      if( (here&7)!=7 ) {
        mid[1].bump(slot_f, 4);
        alt[1].bump(slot_r, 2);
      }
      if( here&7 ) {
        mid[-1].bump(slot_f, 3);
        alt[-1].bump(slot_r, 2);
      }
    }
  }

  void update_model() {
    const uint32_t code_f = fold[(uint8_t)(cursor[0][0].sym-(uint8_t)pred)];
    const int32_t code_r = fold[(uint8_t)((uint8_t)pred-cursor[0][0].sym)];
    const int32_t tree_sym = (int32_t)(code_f-5)>>1;
    const int32_t slot_f = sym_slot(code_f);
    const int32_t slot_r = sym_slot(code_r);
    uint32_t ctx_alt = CtxIdx{}.bits<0, 5>(ctx[0]).raw(mixer_rev(ctx_w, 9));
    CounterNode* node_alt = &counters[ctx_alt];
    node_alt[0].bump(slot_r, 17);
    const int32_t result = ctx[0];
    if( (result&7)!=7 ) {
      CounterNode* node_up = &counters[result];
      uint32_t ctx_up = ((ctx[1]&7)<7)+ctx[1];
      node_up[1].bump(slot_f, 11);
      if( slot_f>=5 )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_up[1].escape_high(slot_f)).raw(ctx_up)), (int32_t)(code_f-5)>>1);
    }
    if( result&7 ) {
      CounterNode* node_dn = &counters[result];
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, 13);
      if( slot_f>=5 )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_dn[-1].escape_high(slot_f)).raw(ctx_dn)), tree_sym);
    }
    if( counters[result].total>=kCounterNudgeLimit )
      return;
    if( (result&7u)<7 ) {
      node_alt[1].bump(slot_r, 7);
    }
    if( result&7 ) {
      node_alt[-1].bump(slot_r, 5);
    }
    if( slot_f>=5 ) {
      const uint32_t key = CtxIdx{}.bit<7>(slot_f&1).bit<6>(counters[result].escape_high(slot_f)).raw(ctx[1]);
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
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      init_counter_node(&counters[ctr++]);
    while( ctr<kP1CounterCount );
    pred = 0;
    int32_t lvl = 0;
    int32_t grp = 0;
    int32_t k = 0;
    for( k = 0; k<0x100; ++k ) {
      level_of[2*k] = lvl;
      const int32_t lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp);
      level_of[2*k+1] = lvl1;
      const int32_t odd = 2*k+1;
      const int32_t grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp1);
      grp = (odd==p1_group_edges[grp1])+grp1;
    }
    int32_t slot = 0;
    for( uint32_t pair = 0; pair<0x80; ++pair ) {
      slot_of[2*pair] = CtxIdx{}.above<3>(slot);
      const int32_t slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = CtxIdx{}.above<3>(slot1);
      slot = (2*pair+1==p1_slot_edges[slot1])+slot1;
    }
    for( int32_t c = 0, w1 = 32; c<9; ++c, w1 *= 3 ) {
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
