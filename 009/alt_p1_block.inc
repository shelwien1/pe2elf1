// alt_p1_block.inc -- the alternate model for predictor 1: its block, its context
// and its counters.  "p1" is `plane_predictor == 1` and says nothing about depth --
// measured, this codes t24's and t32's *eight-bit* planes.  MODELS.md section 6.

struct CtxWeight {
  int32_t sel;
  uint32_t w[3];
};

struct P1Ctx {
  uint8_t sym;
  uint8_t mag;
};

struct AltP1Block {
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
  CounterNode counters[629856];
  // Start a row: mirror the last six records past the right edge, rotate the
  // five row buffers by one, reseat the five cursors, carry the last record of
  // the row above into the left margin, mirror three records back from it, and
  // rebuild the two activity sums the context reads.
  //
  // Twenty-five lines, written out three times -- here, in `alt_p1_code.inc`
  // and in `alt_p1.inc` -- because in the decompilation each of the three
  // callers had its own copy of the loop.
  //
  // The two activity sums that follow it stay at the call sites, because they
  // are *not* the same at all three: `alt_p1_code`'s reads the two mirrored
  // records' magnitudes through `(int8_t)` and the other two read them as the
  // `uint8_t` they are declared.  Whether that difference can ever show is a
  // question about how large a residual magnitude gets, and it is not one this
  // extraction needed to answer.
  void advance_row() {
    {
      P1Ctx*const here = cursor[0];
      for( int32_t k = 0; k<6; ++k )
        here[k] = here[-1-k];
    }
    P1Ctx *b4 = buf[4], *buf3 = buf[3], *buf2 = buf[2], *buf1 = buf[1], *b0 = buf[0];
    buf[4] = buf3;
    buf[3] = buf2;
    buf[2] = buf1;
    buf[1] = b0;
    buf[0] = b4;
    cursor[0] = b4+4;
    cursor[1] = b0+4;
    cursor[2] = buf1+4;
    cursor[3] = buf2+4;
    cursor[4] = buf3+4;
    cursor[0][-4] = cursor[1][3];
    {
      P1Ctx*const here = cursor[0];
      P1Ctx*const up = cursor[1];
      here[-3] = up[2];
      here[-2] = up[1];
      here[-1] = up[0];
    }
  }

  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    bool pick, is_zero, is_neg;
    int32_t act_all;
    uint32_t quiet;
    int32_t guess;
    P1Ctx *cursor1 = cursor[1];
    guess = cursor1->sym;
    P1Ctx *cur = cursor[0];
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
    P1Ctx *cursor2 = cursor[2];
    int32_t act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2->mag)+6*cur[-1].mag+4*(cursor1->mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
    if( nb0 ) {
      if( nb1 ) {
        P1Ctx *nb0_row = nb0->cursor[0];
        act_all = act+2*(nb1->cursor[0][-1].mag+nb0_row[-1].mag);
        ctx_w[5].sel = cursor1->sym-(uint32_t)guess+nb0_row[-1].sym-nb0->cursor[1][-1].sym;
        P1Ctx *nb0_row2 = nb0->cursor[0];
        int32_t nb0_a = nb0_row2[-1].sym;
        uint32_t nb0_b = nb0_row2[-2].sym;
        ctx_w[6].sel = cur[-1].sym-(uint32_t)guess+nb0_a-nb0_b;
        ctx_w[7].sel = cur[-1].sym-(uint32_t)guess+nb1->cursor[0][-1].sym-nb1->cursor[0][-2].sym;
        ctx_w[8].sel = nb1->cursor[0][-1].sym-nb1->pred;
        ctx_w[3].sel = nb0->cursor[0][-1].sym-nb0->pred;
        quiet = (nb1->cursor[0][-1].mag+(uint32_t)nb0->cursor[0][-1].mag-16)>>31;
      } else {
        act_all = cursor1[3].mag+act+3*nb0->cursor[0][-1].mag;
        ctx_w[5].sel = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
        ctx_w[6].sel = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
        ctx_w[7].sel = -guess-cursor1->sym+cursor1[1].sym+cur[-1].sym;
        ctx_w[8].sel = cur[-1].sym-(uint32_t)guess+nb0->cursor[0][-1].sym-nb0->cursor[0][-2].sym;
        ctx_w[3].sel = nb0->cursor[0][-1].sym-nb0->pred;
        quiet = ((uint32_t)nb0->cursor[0][-1].mag-8)>>31;
      }
    } else {
      P1Ctx *cursor4 = cursor[4];
      act_all = cursor4->mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
      ctx_w[5].sel = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
      ctx_w[6].sel = 2*cursor1->sym-cursor2->sym-(uint32_t)guess;
      ctx_w[7].sel = -guess-cursor1->sym+cursor1[1].sym+cur[-1].sym;
      ctx_w[8].sel = -3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess;
      ctx_w[3].sel = cursor1[2].sym-(uint32_t)guess;
      quiet = cur->mag+cursor4->mag+cursor[3]->mag+cursor2->mag+cursor1->mag==0;
    }
    int32_t act_q = (act_all+7)>>4;
    int32_t act_lvl = level_of[act_q];
    int32_t step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    ctx_w[0].sel = ((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31);
    uint32_t s0 = ((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31);
    int32_t s1 = (cursor1[-1].sym-cursor1->sym>=0)+(cursor1[-1].sym>(int32_t)cursor1->sym);
    ctx_w[1].sel = s1;
    int32_t s2 = (cursor1[-1].sym-cur[-1].sym>=0)+(cursor1[-1].sym>(int32_t)cur[-1].sym);
    ctx_w[2].sel = s2;
    is_zero = ctx_w[3].sel==0;
    is_neg = ctx_w[3].sel<0;
    int32_t s4 = (cursor1[1].sym-guess>=-step)+(cursor1[1].sym-guess>step);
    ctx_w[4].sel = s4;
    int32_t hi = step<ctx_w[5].sel;
    int32_t lo = -step<=ctx_w[5].sel;
    int32_t s3 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[3].sel = s3;
    int32_t s5 = lo+hi;
    is_zero = ctx_w[6].sel==0;
    is_neg = ctx_w[6].sel<0;
    ctx_w[5].sel = s5;
    int32_t g6 = !is_neg&&!is_zero;
    int32_t h6 = !is_neg;
    is_zero = ctx_w[7].sel==0;
    is_neg = ctx_w[7].sel<0;
    int32_t s6 = h6+g6;
    ctx_w[6].sel = h6+g6;
    int32_t g7 = !is_neg&&!is_zero;
    int32_t h7 = !is_neg;
    is_zero = ctx_w[8].sel==0;
    is_neg = ctx_w[8].sel<0;
    int32_t s7 = h7+g7;
    ctx_w[7].sel = h7+g7;
    int32_t s8 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[8].sel = s8;
    int32_t result = ctx_w[0].w[s0]+ctx_w[1].w[s1]+ctx_w[2].w[s2]+ctx_w[3].w[s3]+ctx_w[4].w[s4]+ctx_w[5].w[s5]+ctx_w[6].w[s6]+ctx_w[7].w[s7]+ctx_w[8].w[s8]+16*quiet+8*(ctx[3+ctx[2]]==0)+act_lvl;
    ctx[0] = result;
    return result;
  }

  void d8_encode_body(uint8_t* src, uint8_t* out) {
    int32_t code;
    __rc_begin_encode();
    if( height>0 ) {
      int32_t y = 0;
      do {
        ++y;
        advance_row();
        {
          P1Ctx *row0 = cursor[0], *row2 = cursor[2], *row4 = cursor[4];
          ctx[2] = 0;
          ctx[3] = row2[-2].mag+row4[-2].mag+row2[0].mag+row4[0].mag+row2[2].mag+row4[2].mag+row2[4].mag+row4[4].mag+row0[-4].mag+row0[-2].mag;
          ctx[4] = row2[-1].mag+row4[-1].mag+row2[1].mag+row4[1].mag+row2[3].mag+row4[3].mag+row2[5].mag+row4[5].mag+row0[-3].mag+row0[-1].mag;
        }
        if( !(width<=0) ) {
          uint8_t* q = out;
          int32_t x = 0;
          do {
            ++x;
            ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
            uint8_t guess = (uint8_t)pred;
            int32_t resid = (uint8_t)(*src-guess);
            uint8_t recon = *(fold[resid]+unfold)+guess;
            code = fold[resid];
            int32_t drift = (uint8_t)*q-(uint8_t)(recon+*q-*src);
            if( drift<-16||drift>16 ) {
              *q = *src;
              code = fold_hi[resid];
            } else {
              *q = recon;
            }
            __alt_p1_encode_symbol(&counters[ctx[0]].total, 16*ctx[0], ctx[1], code);
            int32_t val = (uint8_t)*q;
            int32_t err = val-pred;
            cursor[0]->sym = val;
            cursor[0]->mag = abs32(err);
            ctx[3+ctx[2]] = ctx[3+ctx[2]]+cursor[0]->mag-cursor[0][-4].mag-(cursor[4][-2].mag-cursor[4][6].mag+cursor[2][-2].mag-cursor[2][6].mag);
            ctx[2] = ctx[2]==0;
            if( counters[ctx[0]].total<0x4000u )
              update_model();
            ++cursor[0];
            ++q;
            ++cursor[1];
            ++cursor[2];
            ++cursor[3];
            ++cursor[4];
            ++src;
          } while( x<width );
          out = q;
        }
      } while( (uint32_t)y<*(uint32_t*)&height );
    }
    __rc_end_encode();
  }

  int32_t update_model() {
    int32_t result;
    CounterNode* node;
    int32_t alti8;
    int32_t slot_f, slot_r, ctx0, ctx1, ctx2, ctx3, ctx4, ctx5, ctx6, ctx7, ctx8;
    uint32_t code_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    int32_t sel1_top = ctx_w[1].sel;
    int32_t code_r = *((uint8_t)((uint8_t)pred-cursor[0]->sym)+fold);
    int32_t sel2_top = ctx_w[2].sel;
    int32_t tree_sym = (int32_t)(code_f-5)>>1;
    slot_f = 6-(code_f&1);
    if( code_f<5 )
      slot_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    slot_r = 6-(code_r&1);
    if( code_r<5 )
      slot_r = code_r;
    uint32_t ctx_alt = ctx_w[8].w[2-ctx_w[8].sel]+ctx_w[7].w[2-ctx_w[7].sel]+ctx_w[6].w[2-ctx_w[6].sel]+ctx_w[5].w[2-ctx_w[5].sel]+ctx_w[4].w[2-ctx_w[4].sel]+ctx_w[3].w[2-ctx_w[3].sel]+ctx_w[2].w[2-sel2_top]+ctx_w[1].w[2-sel1_top]+ctx_w[0].w[1]+(ctx[0]&0x1F);
    CounterNode *node_alt = &counters[ctx_alt];
    node_alt[0].bump(slot_r, 17);
    result = ctx[0];
    if( (result&7)!=7 ) {
      CounterNode *node_up = &counters[result];
      uint32_t ctx_up = (((ctx[1]&7u)-7)>>31)+ctx[1];
      node_up[1].bump(slot_f, 11);
      if( slot_f>=5 )
        __update_binary_pair(model_strip(128*(slot_f&1)+ctx_up+node_up[1].escape_bias(slot_f)), (int32_t)(code_f-5)>>1);
    }
    if( result&7 ) {
      CounterNode *node_dn = &counters[result];
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, 13);
      if( slot_f>=5 )
        __update_binary_pair(model_strip(128*(uint32_t)(slot_f&1)+(uint32_t)ctx_dn+node_dn[-1].escape_bias(slot_f)), tree_sym);
    }
    if( counters[result].total<0xCCCu ) {
      if( (result&7u)<7 ) {
        node_alt[1].bump(slot_r, 7);
      }
      if( result&7 ) {
        node_alt[-1].bump(slot_r, 5);
      }
      if( slot_f>=5 ) {
        uint32_t bias = counters[result].escape_bias(slot_f);
        uint32_t key = ctx[1]+bias+((slot_f&1)<<7);
        if( (key&0x38)>=0x38||(__update_binary_pair(model_strip(128*(slot_f&1)+ctx[1]+bias+8), tree_sym), (key&0x38)!=0) ) {
          __update_binary_pair(model_strip(key-8), tree_sym);
        }
      }
      int32_t sel0 = ctx_w[0].sel;
      if( sel0==1 ) {
        int32_t mid0 = result-ctx_w[0].w[1];
        CounterNode *lo0 = &counters[mid0+ctx_w[0].w[0]];
        CounterNode *hi0 = &counters[ctx_w[0].w[2]+mid0];
        lo0[0].bump(slot_f, 6);
        hi0[0].bump(slot_f, 6);
        ctx0 = ctx[0];
        if( (ctx0&7)!=7 ) {
          lo0[1].bump(slot_f, 4);
          hi0[1].bump(slot_f, 4);
          ctx0 = ctx[0];
        }
        if( ctx0&7 ) {
          lo0[-1].bump(slot_f, 3);
          hi0[-1].bump(slot_f, 3);
          ctx0 = ctx[0];
        }
      } else {
        CounterNode *opp0 = &counters[ctx_w[0].w[2-sel0]+result-ctx_w[0].w[sel0]];
        opp0[0].bump(slot_f, 7);
        uint32_t alti0 = ctx_alt+ctx_w[0].w[0]-ctx_w[0].w[1];
        CounterNode *midn0 = &counters[ctx_w[0].w[1]+ctx[0]-ctx_w[0].w[ctx_w[0].sel]];
        midn0[0].bump(slot_f, 6);
        CounterNode *altn0 = &counters[alti0];
        altn0[0].bump(slot_r, 4);
        ctx0 = ctx[0];
        if( (ctx0&7)!=7 ) {
          midn0[1].bump(slot_f, 4);
          altn0[1].bump(slot_r, 2);
          ctx0 = ctx[0];
        }
        if( ctx0&7 ) {
          midn0[-1].bump(slot_f, 3);
          altn0[-1].bump(slot_r, 2);
          ctx0 = ctx[0];
        }
      }
      int32_t sel1 = ctx_w[1].sel;
      if( sel1==1 ) {
        int32_t mid1 = ctx0-ctx_w[1].w[1];
        CounterNode *lo1 = &counters[mid1+ctx_w[1].w[0]];
        CounterNode *hi1 = &counters[ctx_w[1].w[2]+mid1];
        lo1[0].bump(slot_f, 6);
        hi1[0].bump(slot_f, 6);
        ctx1 = ctx[0];
        if( (ctx1&7)!=7 ) {
          lo1[1].bump(slot_f, 4);
          hi1[1].bump(slot_f, 4);
          ctx1 = ctx[0];
        }
        if( ctx1&7 ) {
          lo1[-1].bump(slot_f, 3);
          hi1[-1].bump(slot_f, 3);
          ctx1 = ctx[0];
        }
      } else {
        CounterNode *opp1 = &counters[ctx_w[1].w[2-sel1]+ctx0-ctx_w[1].w[sel1]];
        opp1[0].bump(slot_f, 7);
        uint32_t w11 = ctx_w[1].w[1];
        uint32_t alti1 = ctx_alt+w11-ctx_w[1].w[2-ctx_w[1].sel];
        CounterNode *midn1 = &counters[(w11+ctx[0]-ctx_w[1].w[ctx_w[1].sel])];
        midn1[0].bump(slot_f, 6);
        CounterNode *altn1 = &counters[alti1];
        altn1[0].bump(slot_r, 4);
        ctx1 = ctx[0];
        if( (ctx1&7)!=7 ) {
          midn1[1].bump(slot_f, 4);
          altn1[1].bump(slot_r, 2);
          ctx1 = ctx[0];
        }
        if( ctx1&7 ) {
          midn1[-1].bump(slot_f, 3);
          altn1[-1].bump(slot_r, 2);
          ctx1 = ctx[0];
        }
      }
      int32_t sel2 = ctx_w[2].sel;
      if( sel2==1 ) {
        int32_t mid2 = ctx1-ctx_w[2].w[1];
        CounterNode *lo2 = &counters[mid2+ctx_w[2].w[0]];
        CounterNode *hi2 = &counters[ctx_w[2].w[2]+mid2];
        lo2[0].bump(slot_f, 6);
        hi2[0].bump(slot_f, 6);
        ctx2 = ctx[0];
        if( (ctx2&7)!=7 ) {
          lo2[1].bump(slot_f, 4);
          hi2[1].bump(slot_f, 4);
          ctx2 = ctx[0];
        }
        if( ctx2&7 ) {
          lo2[-1].bump(slot_f, 3);
          hi2[-1].bump(slot_f, 3);
          ctx2 = ctx[0];
        }
      } else {
        CounterNode *opp2 = &counters[ctx_w[2].w[2-sel2]+ctx1-ctx_w[2].w[sel2]];
        opp2[0].bump(slot_f, 7);
        uint32_t w12 = ctx_w[2].w[1];
        uint32_t x2 = w12+ctx[0]-ctx_w[2].w[ctx_w[2].sel];
        uint32_t alti2 = ctx_alt+w12-ctx_w[2].w[2-ctx_w[2].sel];
        CounterNode *midn2 = &counters[x2];
        midn2[0].bump(slot_f, 6);
        CounterNode *altn2 = &counters[alti2];
        altn2[0].bump(slot_r, 4);
        ctx2 = ctx[0];
        if( (ctx2&7)!=7 ) {
          midn2[1].bump(slot_f, 4);
          altn2[1].bump(slot_r, 2);
          ctx2 = ctx[0];
        }
        if( ctx2&7 ) {
          midn2[-1].bump(slot_f, 3);
          altn2[-1].bump(slot_r, 2);
          ctx2 = ctx[0];
        }
      }
      int32_t sel3 = ctx_w[3].sel;
      if( sel3==1 ) {
        int32_t mid3 = ctx2-ctx_w[3].w[1];
        CounterNode *lo3 = &counters[mid3+ctx_w[3].w[0]];
        CounterNode *hi3 = &counters[ctx_w[3].w[2]+mid3];
        lo3[0].bump(slot_f, 6);
        hi3[0].bump(slot_f, 6);
        ctx3 = ctx[0];
        if( (ctx3&7)!=7 ) {
          lo3[1].bump(slot_f, 4);
          hi3[1].bump(slot_f, 4);
          ctx3 = ctx[0];
        }
        if( ctx3&7 ) {
          lo3[-1].bump(slot_f, 3);
          hi3[-1].bump(slot_f, 3);
          ctx3 = ctx[0];
        }
      } else {
        CounterNode *opp3 = &counters[ctx_w[3].w[2-sel3]+ctx2-ctx_w[3].w[sel3]];
        opp3[0].bump(slot_f, 7);
        uint32_t w13 = ctx_w[3].w[1];
        uint32_t x3 = w13+ctx[0]-ctx_w[3].w[ctx_w[3].sel];
        uint32_t alti3 = ctx_alt+w13-ctx_w[3].w[2-ctx_w[3].sel];
        CounterNode *midn3 = &counters[x3];
        midn3[0].bump(slot_f, 6);
        CounterNode *altn3 = &counters[alti3];
        altn3[0].bump(slot_r, 4);
        ctx3 = ctx[0];
        if( (ctx3&7)!=7 ) {
          midn3[1].bump(slot_f, 4);
          altn3[1].bump(slot_r, 2);
          ctx3 = ctx[0];
        }
        if( ctx3&7 ) {
          midn3[-1].bump(slot_f, 3);
          altn3[-1].bump(slot_r, 2);
          ctx3 = ctx[0];
        }
      }
      int32_t sel4 = ctx_w[4].sel;
      if( sel4==1 ) {
        int32_t mid4 = ctx3-ctx_w[4].w[1];
        CounterNode *lo4 = &counters[mid4+ctx_w[4].w[0]];
        CounterNode *hi4 = &counters[ctx_w[4].w[2]+mid4];
        lo4[0].bump(slot_f, 6);
        hi4[0].bump(slot_f, 6);
        ctx4 = ctx[0];
        if( (ctx4&7)!=7 ) {
          lo4[1].bump(slot_f, 4);
          hi4[1].bump(slot_f, 4);
          ctx4 = ctx[0];
        }
        if( ctx4&7 ) {
          lo4[-1].bump(slot_f, 3);
          hi4[-1].bump(slot_f, 3);
          ctx4 = ctx[0];
        }
      } else {
        CounterNode *opp4 = &counters[ctx_w[4].w[2-sel4]+ctx3-ctx_w[4].w[sel4]];
        opp4[0].bump(slot_f, 7);
        uint32_t w14 = ctx_w[4].w[1];
        uint32_t x4 = w14+ctx[0]-ctx_w[4].w[ctx_w[4].sel];
        uint32_t alti4 = ctx_alt+w14-ctx_w[4].w[2-ctx_w[4].sel];
        CounterNode *midn4 = &counters[x4];
        midn4[0].bump(slot_f, 6);
        CounterNode *altn4 = &counters[alti4];
        altn4[0].bump(slot_r, 4);
        ctx4 = ctx[0];
        if( (ctx4&7)!=7 ) {
          midn4[1].bump(slot_f, 4);
          altn4[1].bump(slot_r, 2);
          ctx4 = ctx[0];
        }
        if( ctx4&7 ) {
          midn4[-1].bump(slot_f, 3);
          altn4[-1].bump(slot_r, 2);
          ctx4 = ctx[0];
        }
      }
      int32_t sel5 = ctx_w[5].sel;
      if( sel5==1 ) {
        int32_t mid5 = ctx4-ctx_w[5].w[1];
        CounterNode *lo5 = &counters[mid5+ctx_w[5].w[0]];
        CounterNode *hi5 = &counters[ctx_w[5].w[2]+mid5];
        lo5[0].bump(slot_f, 6);
        hi5[0].bump(slot_f, 6);
        ctx5 = ctx[0];
        if( (ctx5&7)!=7 ) {
          lo5[1].bump(slot_f, 4);
          hi5[1].bump(slot_f, 4);
          ctx5 = ctx[0];
        }
        if( ctx5&7 ) {
          lo5[-1].bump(slot_f, 3);
          hi5[-1].bump(slot_f, 3);
          ctx5 = ctx[0];
        }
      } else {
        CounterNode *opp5 = &counters[ctx_w[5].w[2-sel5]+ctx4-ctx_w[5].w[sel5]];
        opp5[0].bump(slot_f, 7);
        uint32_t w15 = ctx_w[5].w[1];
        uint32_t x5 = w15+ctx[0]-ctx_w[5].w[ctx_w[5].sel];
        uint32_t alti5 = ctx_alt+w15-ctx_w[5].w[2-ctx_w[5].sel];
        CounterNode *midn5 = &counters[x5];
        midn5[0].bump(slot_f, 6);
        CounterNode *altn5 = &counters[alti5];
        altn5[0].bump(slot_r, 4);
        ctx5 = ctx[0];
        if( (ctx5&7)!=7 ) {
          midn5[1].bump(slot_f, 4);
          altn5[1].bump(slot_r, 2);
          ctx5 = ctx[0];
        }
        if( ctx5&7 ) {
          midn5[-1].bump(slot_f, 3);
          altn5[-1].bump(slot_r, 2);
          ctx5 = ctx[0];
        }
      }
      int32_t sel6 = ctx_w[6].sel;
      if( sel6==1 ) {
        int32_t mid6 = ctx5-ctx_w[6].w[1];
        CounterNode *lo6 = &counters[mid6+ctx_w[6].w[0]];
        CounterNode *hi6 = &counters[ctx_w[6].w[2]+mid6];
        lo6[0].bump(slot_f, 6);
        hi6[0].bump(slot_f, 6);
        ctx6 = ctx[0];
        if( (ctx6&7)!=7 ) {
          lo6[1].bump(slot_f, 4);
          hi6[1].bump(slot_f, 4);
          ctx6 = ctx[0];
        }
        if( ctx6&7 ) {
          lo6[-1].bump(slot_f, 3);
          hi6[-1].bump(slot_f, 3);
          ctx6 = ctx[0];
        }
      } else {
        CounterNode *opp6 = &counters[ctx_w[6].w[2-sel6]+ctx5-ctx_w[6].w[sel6]];
        opp6[0].bump(slot_f, 7);
        uint32_t w16 = ctx_w[6].w[1];
        uint32_t x6 = w16+ctx[0]-ctx_w[6].w[ctx_w[6].sel];
        uint32_t alti6 = ctx_alt+w16-ctx_w[6].w[2-ctx_w[6].sel];
        CounterNode *midn6 = &counters[x6];
        midn6[0].bump(slot_f, 6);
        CounterNode *altn6 = &counters[alti6];
        altn6[0].bump(slot_r, 4);
        ctx6 = ctx[0];
        if( (ctx6&7)!=7 ) {
          midn6[1].bump(slot_f, 4);
          altn6[1].bump(slot_r, 2);
          ctx6 = ctx[0];
        }
        if( ctx6&7 ) {
          midn6[-1].bump(slot_f, 3);
          altn6[-1].bump(slot_r, 2);
          ctx6 = ctx[0];
        }
      }
      int32_t sel7 = ctx_w[7].sel;
      if( sel7==1 ) {
        int32_t mid7 = ctx6-ctx_w[7].w[1];
        CounterNode *lo7 = &counters[mid7+ctx_w[7].w[0]];
        CounterNode *hi7 = &counters[ctx_w[7].w[2]+mid7];
        lo7[0].bump(slot_f, 6);
        hi7[0].bump(slot_f, 6);
        ctx7 = ctx[0];
        if( (ctx7&7)!=7 ) {
          lo7[1].bump(slot_f, 4);
          hi7[1].bump(slot_f, 4);
          ctx7 = ctx[0];
        }
        if( ctx7&7 ) {
          lo7[-1].bump(slot_f, 3);
          hi7[-1].bump(slot_f, 3);
          ctx7 = ctx[0];
        }
      } else {
        CounterNode *opp7 = &counters[ctx_w[7].w[2-sel7]+ctx6-ctx_w[7].w[sel7]];
        opp7[0].bump(slot_f, 7);
        uint32_t w17 = ctx_w[7].w[1];
        uint32_t x7 = w17+ctx[0]-ctx_w[7].w[ctx_w[7].sel];
        uint32_t alti7 = ctx_alt+w17-ctx_w[7].w[2-ctx_w[7].sel];
        CounterNode *midn7 = &counters[x7];
        midn7[0].bump(slot_f, 6);
        CounterNode *altn7 = &counters[alti7];
        altn7[0].bump(slot_r, 4);
        ctx7 = ctx[0];
        if( (ctx7&7)!=7 ) {
          midn7[1].bump(slot_f, 4);
          altn7[1].bump(slot_r, 2);
          ctx7 = ctx[0];
        }
        if( ctx7&7 ) {
          midn7[-1].bump(slot_f, 3);
          altn7[-1].bump(slot_r, 2);
          ctx7 = ctx[0];
        }
      }
      int32_t sel8 = ctx_w[8].sel;
      if( sel8==1 ) {
        int32_t midn8 = ctx7-ctx_w[8].w[1];
        CounterNode *altn8 = &counters[midn8+ctx_w[8].w[0]];
        node = &counters[ctx_w[8].w[2]+midn8];
        altn8[0].bump(slot_f, 6);
        node[0].bump(slot_f, 6);
        ctx8 = ctx[0];
        if( (ctx8&7)!=7 ) {
          altn8[1].bump(slot_f, 4);
          node[1].bump(slot_f, 4);
          ctx8 = ctx[0];
        }
        if( ctx8&7 ) {
          altn8[-1].bump(slot_f, 3);
          node[-1].bump(slot_f, 3);
        }
      } else {
        CounterNode *opp8 = &counters[ctx_w[8].w[2-sel8]+ctx7-ctx_w[8].w[sel8]];
        opp8[0].bump(slot_f, 7);
        uint32_t w18 = ctx_w[8].w[1];
        node = &counters[w18+ctx[0]-ctx_w[8].w[ctx_w[8].sel]];
        CounterNode *x8 = &counters[w18-ctx_w[8].w[2-ctx_w[8].sel]+ctx_alt];
        node[0].bump(slot_f, 6);
        x8[0].bump(slot_r, 4);
        alti8 = ctx[0];
        if( (alti8&7)!=7 ) {
          node[1].bump(slot_f, 4);
          x8[1].bump(slot_r, 2);
          alti8 = ctx[0];
        }
        if( alti8&7 ) {
          node[-1].bump(slot_f, 3);
          result = (uint16_t)x8[-1].c[slot_r]+2;
          x8[-1].c[slot_r] = result;
          x8[-1].total += 2;
        }
      }
    }
    return result;
  }

  AltP1Block*alt_p1_alloc(int32_t img_w, int32_t img_h, int32_t plane) {
    int32_t lvl, grp, slot, wid;
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      __init_counter_node(&counters[ctr++]);
    while( ctr<0x99C60 );
    pred = 0;
    lvl = 0;
    grp = 0;
    int32_t k = 0;
    int32_t plane_hi = plane<<8;
    do {
      level_of[2*k] = lvl;
      int32_t lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = plane_hi|grp;
      level_of[2*k+1] = lvl1;
      int32_t odd = 2*k+1;
      int32_t grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = grp1|plane_hi;
      grp = (odd==p1_group_edges[grp1])+grp1;
      ++k;
    } while( (uint32_t)k<0x100 );
    uint32_t pair = 0;
    slot = 0;
    do {
      slot_of[2*pair] = 8*slot;
      int32_t slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = 8*slot1;
      int32_t bump = 2*pair+++1==p1_slot_edges[slot1];
      slot = bump+slot1;
    } while( pair<0x80 );
    ctx_w[0].w[1] = 32;
    ctx_w[8].w[0] = 0;
    ctx_w[7].w[0] = 0;
    ctx_w[0].w[2] = 64;
    ctx_w[6].w[0] = 0;
    ctx_w[5].w[0] = 0;
    ctx_w[4].w[0] = 0;
    ctx_w[1].w[1] = 96;
    ctx_w[3].w[0] = 0;
    ctx_w[2].w[0] = 0;
    ctx_w[1].w[0] = 0;
    ctx_w[1].w[2] = 192;
    ctx_w[0].w[0] = 0;
    uint32_t row = 0;
    ctx_w[2].w[1] = 288;
    ctx_w[2].w[2] = 576;
    ctx_w[3].w[1] = 864;
    ctx_w[3].w[2] = 1728;
    ctx_w[4].w[1] = 2592;
    ctx_w[4].w[2] = 5184;
    ctx_w[5].w[1] = 7776;
    ctx_w[5].w[2] = 15552;
    ctx_w[6].w[1] = 23328;
    ctx_w[6].w[2] = 46656;
    ctx_w[7].w[1] = 69984;
    ctx_w[7].w[2] = 139968;
    ctx_w[8].w[1] = 209952;
    ctx_w[8].w[2] = 419904;
    do
      buf[row++] = (P1Ctx*)bmf_new(sizeof(P1Ctx)*(width+10));
    while( row<5 );
    __alt_init_tables(fold, (int8_t*)unfold);
    wid = width;
    if( width>-10 ) {
      int32_t r = 0;
      do {
        buf[4][r].sym = 72;
        buf[3][r].sym = 72;
        buf[2][r].sym = 72;
        buf[1][r].sym = 72;
        buf[0][r].sym = 72;
        buf[4][r].mag = 0;
        buf[3][r].mag = 0;
        buf[2][r].mag = 0;
        buf[1][r].mag = 0;
        buf[0][r].mag = 0;
        wid = width;
        ++r;
      } while( r<width+10 );
    }
    cursor[0] = buf[0]+wid+4;
    cursor[1] = buf[1]+wid+4;
    cursor[2] = buf[2]+wid+4;
    cursor[3] = buf[3]+wid+4;
    cursor[4] = buf[4]+wid+4;
    return this;
  }

  // Returns nothing.  The original returned `this` -- a compiler's habit of
  // leaving the object pointer in the return register, not a value anyone
  // wanted -- and after the `free(this)` above it is a dangling pointer that
  // g++ reports under `-Wuse-after-free`.  All three call sites discarded it;
  // the one that did not, `__alt_model_p1_d8_decode`, only passed it on to
  // another caller that did.
  void alt_p1_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    if( do_free&1 )
      free(this);
  }
};
