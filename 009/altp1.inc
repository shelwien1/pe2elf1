// altp1.inc -- the alternate model for one-bit planes.

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
  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    P1Ctx* nb0_row, * nb0_row2, * cursor1, * cursor2;
    P1Ctx* cur, * cursor4;
    bool pick, is_zero, is_neg;
    int32_t west, northwest, act, nb0_a, act_q, s1, hi, lo, s3, s5, g6, h6, g7, h7, s8, result, step, act_all, s2, s4, s6, s7;
    uint32_t quiet, s0;
    int32_t guess, plane_a, plane_b, act_lvl;
    cursor1 = cursor[1];
    guess = cursor1->sym;
    cur = cursor[0];
    west = cur[-1].sym;
    northwest = cursor1[-1].sym;
    if( west<guess ) {
      if( northwest>=west ) {
        plane_b = guess+west-northwest;
        pick = northwest<guess;
        guess = cur[-1].sym;
        if( pick )
          guess = plane_b;
      }
    } else if( northwest<=west ) {
      plane_a = guess+west-northwest;
      pick = northwest<=guess;
      guess = cur[-1].sym;
      if( !pick )
        guess = plane_a;
    }
    pred = guess;
    cursor2 = cursor[2];
    act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2->mag)+6*cur[-1].mag+4*(cursor1->mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
    if( nb0 ) {
      if( nb1 ) {
        nb0_row = nb0->cursor[0];
        act_all = act+2*(nb1->cursor[0][-1].mag+nb0_row[-1].mag);
        ctx_w[5].sel = cursor1->sym-(uint32_t)guess+nb0_row[-1].sym-nb0->cursor[1][-1].sym;
        nb0_row2 = nb0->cursor[0];
        nb0_a = nb0_row2[-1].sym;
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
      cursor4 = cursor[4];
      act_all = cursor4->mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
      ctx_w[5].sel = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
      ctx_w[6].sel = 2*cursor1->sym-cursor2->sym-(uint32_t)guess;
      ctx_w[7].sel = -guess-cursor1->sym+cursor1[1].sym+cur[-1].sym;
      ctx_w[8].sel = -3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess;
      ctx_w[3].sel = cursor1[2].sym-(uint32_t)guess;
      quiet = cur->mag+cursor4->mag+cursor[3]->mag+cursor2->mag+cursor1->mag==0;
    }
    act_q = (act_all+7)>>4;
    act_lvl = level_of[act_q];
    step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    ctx_w[0].sel = ((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31);
    s0 = ((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31);
    s1 = (cursor1[-1].sym-cursor1->sym>=0)+(cursor1[-1].sym>(int32_t)cursor1->sym);
    ctx_w[1].sel = s1;
    s2 = (cursor1[-1].sym-cur[-1].sym>=0)+(cursor1[-1].sym>(int32_t)cur[-1].sym);
    ctx_w[2].sel = s2;
    is_zero = ctx_w[3].sel==0;
    is_neg = ctx_w[3].sel<0;
    s4 = (cursor1[1].sym-guess>=-step)+(cursor1[1].sym-guess>step);
    ctx_w[4].sel = s4;
    hi = step<ctx_w[5].sel;
    lo = -step<=ctx_w[5].sel;
    s3 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[3].sel = s3;
    s5 = lo+hi;
    is_zero = ctx_w[6].sel==0;
    is_neg = ctx_w[6].sel<0;
    ctx_w[5].sel = s5;
    g6 = !is_neg&&!is_zero;
    h6 = !is_neg;
    is_zero = ctx_w[7].sel==0;
    is_neg = ctx_w[7].sel<0;
    s6 = h6+g6;
    ctx_w[6].sel = h6+g6;
    g7 = !is_neg&&!is_zero;
    h7 = !is_neg;
    is_zero = ctx_w[8].sel==0;
    is_neg = ctx_w[8].sel<0;
    s7 = h7+g7;
    ctx_w[7].sel = h7+g7;
    s8 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[8].sel = s8;
    result = ctx_w[0].w[s0]+ctx_w[1].w[s1]+ctx_w[2].w[s2]+ctx_w[3].w[s3]+ctx_w[4].w[s4]+ctx_w[5].w[s5]+ctx_w[6].w[s6]+ctx_w[7].w[s7]+ctx_w[8].w[s8]+16*quiet+8*(ctx[3+ctx[2]]==0)+act_lvl;
    ctx[0] = result;
    return result;
  }

  void d8_encode_body(uint8_t* src, uint8_t* out) {
    P1Ctx* cursor2, * cursor4, * cursor0, * b4, * buf3, * buf2, * buf1, * b0;
    uint8_t guess, recon;
    int32_t y, resid, code, drift, val, x;
    __rc_begin_encode();
    if( height>0 ) {
      y = 0;
      do {
        ++y;
        {
          P1Ctx*const here = (P1Ctx*)cursor[0];
          here[0] = here[-1];
          here[1] = here[-2];
          here[2] = here[-3];
          here[3] = here[-4];
          here[4] = here[-5];
          here[5] = here[-6];
        }
        b4 = buf[4];
        buf3 = buf[3];
        buf2 = buf[2];
        buf1 = buf[1];
        b0 = buf[0];
        buf[4] = buf3;
        buf[3] = buf2;
        buf[2] = buf1;
        buf[1] = b0;
        buf[0] = b4;
        b4 += 4;
        cursor[0] = b4;
        b0 += 4;
        cursor[1] = b0;
        cursor[2] = buf1+4;
        cursor[3] = buf2+4;
        cursor[4] = buf3+4;
        ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
        {
          P1Ctx*const here = (P1Ctx*)cursor[0];
          P1Ctx*const up = (P1Ctx*)cursor[1];
          here[-3] = up[2];
          here[-2] = up[1];
          here[-1] = up[0];
        }
        cursor2 = cursor[2];
        cursor4 = cursor[4];
        ctx[2] = 0;
        cursor0 = cursor[0];
        ctx[3] = cursor2[-2].mag+cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
        ctx[4] = cursor2[-1].mag+cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
        if( !(width<=0) ) {
          uint8_t* q = out;
          x = 0;
          do {
            ++x;
            (this)->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
            guess = (uint8_t)pred;
            resid = (uint8_t)(*src-guess);
            recon = *(fold[resid]+unfold)+guess;
            code = fold[resid];
            drift = (uint8_t)*q-(uint8_t)(recon+*q-*src);
            if( drift<-16||drift>16 ) {
              *q = *src;
              code = fold_hi[resid];
            } else {
              *q = recon;
            }
            __alt_p1_encode_symbol(&counters[ctx[0]].total, 16*ctx[0], ctx[1], code);
            val = (uint8_t)*q;
            int64_t err = val-pred;
            cursor[0]->sym = val;
            cursor[0]->mag = (BYTE4(err)^err)-BYTE4(err);
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
    CounterNode* node_alt, * node_up, * node_dn;
    int32_t alti8;
    uint32_t w11, w12, w13, w14, w15, w16, w17, w18, alti1, ctx_alt, alti0, alti2, alti3, alti4, alti5, alti6, alti7, x2, x3, x4, x5, x6, x7;
    CounterNode* opp0, * opp1, * opp2, * opp3, * opp4, * opp5, * opp6, * opp7, * opp8;
    int16_t tot_up, tot_dn;
    int32_t sel1_top, code_r, sel2_top, slot_f, slot_r, ctx_dn, sel0, ctx0, sel1, ctx1, sel2, ctx2, sel3, ctx3, sel4, ctx4, sel5, ctx5, sel6, ctx6, sel7, ctx7, sel8, midn8, ctx8, mid7, mid6, mid5, mid4, mid3, mid2, mid1, mid0, tree_sym;
    CounterNode* lo0, * hi0, * midn0, * altn0, * lo1, * hi1, * midn1, * altn1, * lo2, * hi2, * midn2, * altn2, * lo3, * hi3, * midn3, * altn3, * lo4, * hi4, * midn4, * altn4, * lo5, * hi5, * midn5, * altn5, * lo6, * hi6, * midn6, * altn6, * lo7, * hi7, * midn7, * altn7, * altn8, * x8;
    uint32_t code_f, ctx_up;
    code_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    sel1_top = ctx_w[1].sel;
    code_r = *((uint8_t)((uint8_t)pred-cursor[0]->sym)+fold);
    sel2_top = ctx_w[2].sel;
    tree_sym = (int32_t)(code_f-5)>>1;
    slot_f = 6-(code_f&1);
    if( code_f<5 )
      slot_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    slot_r = 6-(code_r&1);
    if( code_r<5 )
      slot_r = code_r;
    ctx_alt = ctx_w[8].w[2-ctx_w[8].sel]+ctx_w[7].w[2-ctx_w[7].sel]+ctx_w[6].w[2-ctx_w[6].sel]+ctx_w[5].w[2-ctx_w[5].sel]+ctx_w[4].w[2-ctx_w[4].sel]+ctx_w[3].w[2-ctx_w[3].sel]+ctx_w[2].w[2-sel2_top]+ctx_w[1].w[2-sel1_top]+ctx_w[0].w[1]+(ctx[0]&0x1F);
    node_alt = &counters[ctx_alt];
    node_alt[0].c[slot_r] += 17;
    node_alt[0].total += 17;
    result = ctx[0];
    if( (result&7)!=7 ) {
      node_up = &counters[result];
      ctx_up = (((ctx[1]&7u)-7)>>31)+ctx[1];
      node_up[1].c[slot_f] += 11;
      tot_up = node_up[1].total+11;
      node_up[1].total = tot_up;
      if( slot_f>=5 )
        __update_binary_pair(model_strip(128*(slot_f&1)+ctx_up+(((((tot_up&0x7FFF)+node_up[1].c[0]-2*(uint32_t)node_up[1].c[slot_f])>>25)&0xFFFFFFC0))), (int32_t)(code_f-5)>>1);
      result = ctx[0];
    }
    if( result&7 ) {
      node_dn = &counters[result];
      ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].c[slot_f] += 13;
      tot_dn = node_dn[-1].total+13;
      node_dn[-1].total = tot_dn;
      if( slot_f>=5 )
        __update_binary_pair(model_strip(128*(uint32_t)(slot_f&1)+(uint32_t)ctx_dn+(((((tot_dn&0x7FFF)+node_dn[-1].c[0]-2*(uint32_t)node_dn[-1].c[slot_f])>>25)&0xFFFFFFC0))), tree_sym);
      result = ctx[0];
    }
    if( counters[result].total<0xCCCu ) {
      if( (result&7u)<7 ) {
        node_alt[1].c[slot_r] += 7;
        node_alt[1].total += 7;
        result = ctx[0];
      }
      if( result&7 ) {
        node_alt[-1].c[slot_r] += 5;
        node_alt[-1].total += 5;
        result = ctx[0];
      }
      if( slot_f>=5 ) {
        uint32_t key = ctx[1]+(((counters[result].c[0]+(counters[result].total&0x7FFF)-2*(uint32_t)counters[result].c[slot_f])>>25)&0xFFFFFFC0)+((slot_f&1)<<7);
        if( (key&0x38)>=0x38||(__update_binary_pair(model_strip(128*(slot_f&1)+ctx[1]+((((counters[result].c[0]+(counters[result].total&0x7FFF)-2*(uint32_t)counters[result].c[slot_f])>>25)&0xFFFFFFC0))+8), tree_sym), (key&0x38)!=0) ) {
          __update_binary_pair(model_strip(key-8), tree_sym);
        }
        result = ctx[0];
      }
      sel0 = ctx_w[0].sel;
      if( sel0==1 ) {
        mid0 = result-ctx_w[0].w[1];
        lo0 = &counters[mid0+ctx_w[0].w[0]];
        hi0 = &counters[ctx_w[0].w[2]+mid0];
        lo0[0].c[slot_f] += 6;
        lo0[0].total += 6;
        hi0[0].c[slot_f] += 6;
        hi0[0].total += 6;
        ctx0 = ctx[0];
        if( (ctx0&7)!=7 ) {
          lo0[1].c[slot_f] += 4;
          lo0[1].total += 4;
          hi0[1].c[slot_f] += 4;
          hi0[1].total += 4;
          ctx0 = ctx[0];
        }
        if( ctx0&7 ) {
          lo0[-1].c[slot_f] += 3;
          lo0[-1].total += 3;
          hi0[-1].c[slot_f] += 3;
          hi0[-1].total += 3;
          ctx0 = ctx[0];
        }
      } else {
        opp0 = &counters[ctx_w[0].w[2-sel0]+result-ctx_w[0].w[sel0]];
        opp0[0].c[slot_f] += 7;
        opp0[0].total += 7;
        alti0 = ctx_alt+ctx_w[0].w[0]-ctx_w[0].w[1];
        midn0 = &counters[ctx_w[0].w[1]+ctx[0]-ctx_w[0].w[ctx_w[0].sel]];
        midn0[0].c[slot_f] += 6;
        midn0[0].total += 6;
        altn0 = &counters[alti0];
        altn0[0].c[slot_r] += 4;
        altn0[0].total += 4;
        ctx0 = ctx[0];
        if( (ctx0&7)!=7 ) {
          midn0[1].c[slot_f] += 4;
          midn0[1].total += 4;
          altn0[1].c[slot_r] += 2;
          altn0[1].total += 2;
          ctx0 = ctx[0];
        }
        if( ctx0&7 ) {
          midn0[-1].c[slot_f] += 3;
          midn0[-1].total += 3;
          altn0[-1].c[slot_r] += 2;
          altn0[-1].total += 2;
          ctx0 = ctx[0];
        }
      }
      sel1 = ctx_w[1].sel;
      if( sel1==1 ) {
        mid1 = ctx0-ctx_w[1].w[1];
        lo1 = &counters[mid1+ctx_w[1].w[0]];
        hi1 = &counters[ctx_w[1].w[2]+mid1];
        lo1[0].c[slot_f] += 6;
        lo1[0].total += 6;
        hi1[0].c[slot_f] += 6;
        hi1[0].total += 6;
        ctx1 = ctx[0];
        if( (ctx1&7)!=7 ) {
          lo1[1].c[slot_f] += 4;
          lo1[1].total += 4;
          hi1[1].c[slot_f] += 4;
          hi1[1].total += 4;
          ctx1 = ctx[0];
        }
        if( ctx1&7 ) {
          lo1[-1].c[slot_f] += 3;
          lo1[-1].total += 3;
          hi1[-1].c[slot_f] += 3;
          hi1[-1].total += 3;
          ctx1 = ctx[0];
        }
      } else {
        opp1 = &counters[ctx_w[1].w[2-sel1]+ctx0-ctx_w[1].w[sel1]];
        opp1[0].c[slot_f] += 7;
        opp1[0].total += 7;
        w11 = ctx_w[1].w[1];
        alti1 = ctx_alt+w11-ctx_w[1].w[2-ctx_w[1].sel];
        midn1 = &counters[(w11+ctx[0]-ctx_w[1].w[ctx_w[1].sel])];
        midn1[0].c[slot_f] += 6;
        midn1[0].total += 6;
        altn1 = &counters[alti1];
        altn1[0].c[slot_r] += 4;
        altn1[0].total += 4;
        ctx1 = ctx[0];
        if( (ctx1&7)!=7 ) {
          midn1[1].c[slot_f] += 4;
          midn1[1].total += 4;
          altn1[1].c[slot_r] += 2;
          altn1[1].total += 2;
          ctx1 = ctx[0];
        }
        if( ctx1&7 ) {
          midn1[-1].c[slot_f] += 3;
          midn1[-1].total += 3;
          altn1[-1].c[slot_r] += 2;
          altn1[-1].total += 2;
          ctx1 = ctx[0];
        }
      }
      sel2 = ctx_w[2].sel;
      if( sel2==1 ) {
        mid2 = ctx1-ctx_w[2].w[1];
        lo2 = &counters[mid2+ctx_w[2].w[0]];
        hi2 = &counters[ctx_w[2].w[2]+mid2];
        lo2[0].c[slot_f] += 6;
        lo2[0].total += 6;
        hi2[0].c[slot_f] += 6;
        hi2[0].total += 6;
        ctx2 = ctx[0];
        if( (ctx2&7)!=7 ) {
          lo2[1].c[slot_f] += 4;
          lo2[1].total += 4;
          hi2[1].c[slot_f] += 4;
          hi2[1].total += 4;
          ctx2 = ctx[0];
        }
        if( ctx2&7 ) {
          lo2[-1].c[slot_f] += 3;
          lo2[-1].total += 3;
          hi2[-1].c[slot_f] += 3;
          hi2[-1].total += 3;
          ctx2 = ctx[0];
        }
      } else {
        opp2 = &counters[ctx_w[2].w[2-sel2]+ctx1-ctx_w[2].w[sel2]];
        opp2[0].c[slot_f] += 7;
        opp2[0].total += 7;
        w12 = ctx_w[2].w[1];
        x2 = w12+ctx[0]-ctx_w[2].w[ctx_w[2].sel];
        alti2 = ctx_alt+w12-ctx_w[2].w[2-ctx_w[2].sel];
        midn2 = &counters[x2];
        midn2[0].c[slot_f] += 6;
        midn2[0].total += 6;
        altn2 = &counters[alti2];
        altn2[0].c[slot_r] += 4;
        altn2[0].total += 4;
        ctx2 = ctx[0];
        if( (ctx2&7)!=7 ) {
          midn2[1].c[slot_f] += 4;
          midn2[1].total += 4;
          altn2[1].c[slot_r] += 2;
          altn2[1].total += 2;
          ctx2 = ctx[0];
        }
        if( ctx2&7 ) {
          midn2[-1].c[slot_f] += 3;
          midn2[-1].total += 3;
          altn2[-1].c[slot_r] += 2;
          altn2[-1].total += 2;
          ctx2 = ctx[0];
        }
      }
      sel3 = ctx_w[3].sel;
      if( sel3==1 ) {
        mid3 = ctx2-ctx_w[3].w[1];
        lo3 = &counters[mid3+ctx_w[3].w[0]];
        hi3 = &counters[ctx_w[3].w[2]+mid3];
        lo3[0].c[slot_f] += 6;
        lo3[0].total += 6;
        hi3[0].c[slot_f] += 6;
        hi3[0].total += 6;
        ctx3 = ctx[0];
        if( (ctx3&7)!=7 ) {
          lo3[1].c[slot_f] += 4;
          lo3[1].total += 4;
          hi3[1].c[slot_f] += 4;
          hi3[1].total += 4;
          ctx3 = ctx[0];
        }
        if( ctx3&7 ) {
          lo3[-1].c[slot_f] += 3;
          lo3[-1].total += 3;
          hi3[-1].c[slot_f] += 3;
          hi3[-1].total += 3;
          ctx3 = ctx[0];
        }
      } else {
        opp3 = &counters[ctx_w[3].w[2-sel3]+ctx2-ctx_w[3].w[sel3]];
        opp3[0].c[slot_f] += 7;
        opp3[0].total += 7;
        w13 = ctx_w[3].w[1];
        x3 = w13+ctx[0]-ctx_w[3].w[ctx_w[3].sel];
        alti3 = ctx_alt+w13-ctx_w[3].w[2-ctx_w[3].sel];
        midn3 = &counters[x3];
        midn3[0].c[slot_f] += 6;
        midn3[0].total += 6;
        altn3 = &counters[alti3];
        altn3[0].c[slot_r] += 4;
        altn3[0].total += 4;
        ctx3 = ctx[0];
        if( (ctx3&7)!=7 ) {
          midn3[1].c[slot_f] += 4;
          midn3[1].total += 4;
          altn3[1].c[slot_r] += 2;
          altn3[1].total += 2;
          ctx3 = ctx[0];
        }
        if( ctx3&7 ) {
          midn3[-1].c[slot_f] += 3;
          midn3[-1].total += 3;
          altn3[-1].c[slot_r] += 2;
          altn3[-1].total += 2;
          ctx3 = ctx[0];
        }
      }
      sel4 = ctx_w[4].sel;
      if( sel4==1 ) {
        mid4 = ctx3-ctx_w[4].w[1];
        lo4 = &counters[mid4+ctx_w[4].w[0]];
        hi4 = &counters[ctx_w[4].w[2]+mid4];
        lo4[0].c[slot_f] += 6;
        lo4[0].total += 6;
        hi4[0].c[slot_f] += 6;
        hi4[0].total += 6;
        ctx4 = ctx[0];
        if( (ctx4&7)!=7 ) {
          lo4[1].c[slot_f] += 4;
          lo4[1].total += 4;
          hi4[1].c[slot_f] += 4;
          hi4[1].total += 4;
          ctx4 = ctx[0];
        }
        if( ctx4&7 ) {
          lo4[-1].c[slot_f] += 3;
          lo4[-1].total += 3;
          hi4[-1].c[slot_f] += 3;
          hi4[-1].total += 3;
          ctx4 = ctx[0];
        }
      } else {
        opp4 = &counters[ctx_w[4].w[2-sel4]+ctx3-ctx_w[4].w[sel4]];
        opp4[0].c[slot_f] += 7;
        opp4[0].total += 7;
        w14 = ctx_w[4].w[1];
        x4 = w14+ctx[0]-ctx_w[4].w[ctx_w[4].sel];
        alti4 = ctx_alt+w14-ctx_w[4].w[2-ctx_w[4].sel];
        midn4 = &counters[x4];
        midn4[0].c[slot_f] += 6;
        midn4[0].total += 6;
        altn4 = &counters[alti4];
        altn4[0].c[slot_r] += 4;
        altn4[0].total += 4;
        ctx4 = ctx[0];
        if( (ctx4&7)!=7 ) {
          midn4[1].c[slot_f] += 4;
          midn4[1].total += 4;
          altn4[1].c[slot_r] += 2;
          altn4[1].total += 2;
          ctx4 = ctx[0];
        }
        if( ctx4&7 ) {
          midn4[-1].c[slot_f] += 3;
          midn4[-1].total += 3;
          altn4[-1].c[slot_r] += 2;
          altn4[-1].total += 2;
          ctx4 = ctx[0];
        }
      }
      sel5 = ctx_w[5].sel;
      if( sel5==1 ) {
        mid5 = ctx4-ctx_w[5].w[1];
        lo5 = &counters[mid5+ctx_w[5].w[0]];
        hi5 = &counters[ctx_w[5].w[2]+mid5];
        lo5[0].c[slot_f] += 6;
        lo5[0].total += 6;
        hi5[0].c[slot_f] += 6;
        hi5[0].total += 6;
        ctx5 = ctx[0];
        if( (ctx5&7)!=7 ) {
          lo5[1].c[slot_f] += 4;
          lo5[1].total += 4;
          hi5[1].c[slot_f] += 4;
          hi5[1].total += 4;
          ctx5 = ctx[0];
        }
        if( ctx5&7 ) {
          lo5[-1].c[slot_f] += 3;
          lo5[-1].total += 3;
          hi5[-1].c[slot_f] += 3;
          hi5[-1].total += 3;
          ctx5 = ctx[0];
        }
      } else {
        opp5 = &counters[ctx_w[5].w[2-sel5]+ctx4-ctx_w[5].w[sel5]];
        opp5[0].c[slot_f] += 7;
        opp5[0].total += 7;
        w15 = ctx_w[5].w[1];
        x5 = w15+ctx[0]-ctx_w[5].w[ctx_w[5].sel];
        alti5 = ctx_alt+w15-ctx_w[5].w[2-ctx_w[5].sel];
        midn5 = &counters[x5];
        midn5[0].c[slot_f] += 6;
        midn5[0].total += 6;
        altn5 = &counters[alti5];
        altn5[0].c[slot_r] += 4;
        altn5[0].total += 4;
        ctx5 = ctx[0];
        if( (ctx5&7)!=7 ) {
          midn5[1].c[slot_f] += 4;
          midn5[1].total += 4;
          altn5[1].c[slot_r] += 2;
          altn5[1].total += 2;
          ctx5 = ctx[0];
        }
        if( ctx5&7 ) {
          midn5[-1].c[slot_f] += 3;
          midn5[-1].total += 3;
          altn5[-1].c[slot_r] += 2;
          altn5[-1].total += 2;
          ctx5 = ctx[0];
        }
      }
      sel6 = ctx_w[6].sel;
      if( sel6==1 ) {
        mid6 = ctx5-ctx_w[6].w[1];
        lo6 = &counters[mid6+ctx_w[6].w[0]];
        hi6 = &counters[ctx_w[6].w[2]+mid6];
        lo6[0].c[slot_f] += 6;
        lo6[0].total += 6;
        hi6[0].c[slot_f] += 6;
        hi6[0].total += 6;
        ctx6 = ctx[0];
        if( (ctx6&7)!=7 ) {
          lo6[1].c[slot_f] += 4;
          lo6[1].total += 4;
          hi6[1].c[slot_f] += 4;
          hi6[1].total += 4;
          ctx6 = ctx[0];
        }
        if( ctx6&7 ) {
          lo6[-1].c[slot_f] += 3;
          lo6[-1].total += 3;
          hi6[-1].c[slot_f] += 3;
          hi6[-1].total += 3;
          ctx6 = ctx[0];
        }
      } else {
        opp6 = &counters[ctx_w[6].w[2-sel6]+ctx5-ctx_w[6].w[sel6]];
        opp6[0].c[slot_f] += 7;
        opp6[0].total += 7;
        w16 = ctx_w[6].w[1];
        x6 = w16+ctx[0]-ctx_w[6].w[ctx_w[6].sel];
        alti6 = ctx_alt+w16-ctx_w[6].w[2-ctx_w[6].sel];
        midn6 = &counters[x6];
        midn6[0].c[slot_f] += 6;
        midn6[0].total += 6;
        altn6 = &counters[alti6];
        altn6[0].c[slot_r] += 4;
        altn6[0].total += 4;
        ctx6 = ctx[0];
        if( (ctx6&7)!=7 ) {
          midn6[1].c[slot_f] += 4;
          midn6[1].total += 4;
          altn6[1].c[slot_r] += 2;
          altn6[1].total += 2;
          ctx6 = ctx[0];
        }
        if( ctx6&7 ) {
          midn6[-1].c[slot_f] += 3;
          midn6[-1].total += 3;
          altn6[-1].c[slot_r] += 2;
          altn6[-1].total += 2;
          ctx6 = ctx[0];
        }
      }
      sel7 = ctx_w[7].sel;
      if( sel7==1 ) {
        mid7 = ctx6-ctx_w[7].w[1];
        lo7 = &counters[mid7+ctx_w[7].w[0]];
        hi7 = &counters[ctx_w[7].w[2]+mid7];
        lo7[0].c[slot_f] += 6;
        lo7[0].total += 6;
        hi7[0].c[slot_f] += 6;
        hi7[0].total += 6;
        ctx7 = ctx[0];
        if( (ctx7&7)!=7 ) {
          lo7[1].c[slot_f] += 4;
          lo7[1].total += 4;
          hi7[1].c[slot_f] += 4;
          hi7[1].total += 4;
          ctx7 = ctx[0];
        }
        if( ctx7&7 ) {
          lo7[-1].c[slot_f] += 3;
          lo7[-1].total += 3;
          hi7[-1].c[slot_f] += 3;
          hi7[-1].total += 3;
          ctx7 = ctx[0];
        }
      } else {
        opp7 = &counters[ctx_w[7].w[2-sel7]+ctx6-ctx_w[7].w[sel7]];
        opp7[0].c[slot_f] += 7;
        opp7[0].total += 7;
        w17 = ctx_w[7].w[1];
        x7 = w17+ctx[0]-ctx_w[7].w[ctx_w[7].sel];
        alti7 = ctx_alt+w17-ctx_w[7].w[2-ctx_w[7].sel];
        midn7 = &counters[x7];
        midn7[0].c[slot_f] += 6;
        midn7[0].total += 6;
        altn7 = &counters[alti7];
        altn7[0].c[slot_r] += 4;
        altn7[0].total += 4;
        ctx7 = ctx[0];
        if( (ctx7&7)!=7 ) {
          midn7[1].c[slot_f] += 4;
          midn7[1].total += 4;
          altn7[1].c[slot_r] += 2;
          altn7[1].total += 2;
          ctx7 = ctx[0];
        }
        if( ctx7&7 ) {
          midn7[-1].c[slot_f] += 3;
          midn7[-1].total += 3;
          altn7[-1].c[slot_r] += 2;
          altn7[-1].total += 2;
          ctx7 = ctx[0];
        }
      }
      sel8 = ctx_w[8].sel;
      if( sel8==1 ) {
        midn8 = ctx7-ctx_w[8].w[1];
        altn8 = &counters[midn8+ctx_w[8].w[0]];
        node = &counters[ctx_w[8].w[2]+midn8];
        altn8[0].c[slot_f] += 6;
        altn8[0].total += 6;
        node[0].c[slot_f] += 6;
        node[0].total += 6;
        ctx8 = ctx[0];
        if( (ctx8&7)!=7 ) {
          altn8[1].c[slot_f] += 4;
          altn8[1].total += 4;
          node[1].c[slot_f] += 4;
          node[1].total += 4;
          ctx8 = ctx[0];
        }
        if( ctx8&7 ) {
          altn8[-1].c[slot_f] += 3;
          altn8[-1].total += 3;
          node[-1].c[slot_f] += 3;
          node[-1].total += 3;
        }
      } else {
        opp8 = &counters[ctx_w[8].w[2-sel8]+ctx7-ctx_w[8].w[sel8]];
        opp8[0].c[slot_f] += 7;
        opp8[0].total += 7;
        w18 = ctx_w[8].w[1];
        node = &counters[w18+ctx[0]-ctx_w[8].w[ctx_w[8].sel]];
        x8 = &counters[w18-ctx_w[8].w[2-ctx_w[8].sel]+ctx_alt];
        node[0].c[slot_f] += 6;
        node[0].total += 6;
        x8[0].c[slot_r] += 4;
        x8[0].total += 4;
        alti8 = ctx[0];
        if( (alti8&7)!=7 ) {
          node[1].c[slot_f] += 4;
          node[1].total += 4;
          x8[1].c[slot_r] += 2;
          x8[1].total += 2;
          alti8 = ctx[0];
        }
        if( alti8&7 ) {
          node[-1].c[slot_f] += 3;
          node[-1].total += 3;
          result = (uint16_t)x8[-1].c[slot_r]+2;
          x8[-1].c[slot_r] = result;
          x8[-1].total += 2;
        }
      }
    }
    return result;
  }

  AltP1Block*alt_p1_alloc(int32_t img_w, int32_t img_h, int32_t plane) {
    int32_t lvl, grp, k, lvl1, odd, grp1, slot, slot1, bump, wid, r, plane_hi;
    uint32_t ctr, pair, row;
    ctr = 0;
    width = img_w;
    height = img_h;
    do
      __init_counter_node(&counters[ctr++]);
    while( ctr<0x99C60 );
    pred = 0;
    lvl = 0;
    grp = 0;
    k = 0;
    plane_hi = plane<<8;
    do {
      level_of[2*k] = lvl;
      lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = plane_hi|grp;
      level_of[2*k+1] = lvl1;
      odd = 2*k+1;
      grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = grp1|plane_hi;
      grp = (odd==p1_group_edges[grp1])+grp1;
      ++k;
    } while( (uint32_t)k<0x100 );
    pair = 0;
    slot = 0;
    do {
      slot_of[2*pair] = 8*slot;
      slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = 8*slot1;
      bump = 2*pair+++1==p1_slot_edges[slot1];
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
    row = 0;
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
      r = 0;
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

  AltP1Block*alt_p1_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    if( do_free&1 )
      free(this);
    return this;
  }
};
