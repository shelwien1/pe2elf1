// alt_p2_model.inc -- the predictor-2 model itself, and the largest body in the program.
// "p2" is `plane_predictor == 2`; it is not about depth.  t8g is a single eight-bit
// plane and codes here.  MODELS.md section 7.

// The per-bank counter walk: five banks of context counters, each stepped
// toward this sample's residual, and the mirror and rotate neighbours of each
// stepped with it at lighter weights.
//
// 321 lines inside `alt_p2_model`, and the reason first recorded for leaving
// them there -- "threading about thirty locals through a signature" -- was a
// guess.  What the loop actually needs from the enclosing scope is small and
// nothing it touches is read after it; the compiler was asked rather than a
// regex, twice, because the regex undercounted both times.
void AltP2Block::code_banks(int32_t sample16) {
  uint8_t b_top3;
  bool go;
  int32_t bctx, res, w0, w0b, res2, w1, e1, neg,
      w_top, e_top, bump, res_s, w_top3, res_c;
  for( uint32_t bank = 0; bank<5; ++bank ) {
      bctx = (uint32_t)bank_ctx[bank];
      res = sample16-(nb_sum[2*bank]);
      P2Count* node0 = &p2_ctr[32768*bank+bctx];
      w0 = res+(uint16_t)node0->weighted;
      node0->weighted = w0;
      int32_t countdown = node0->b1;
      if( countdown ) {
        w0b = w0+4*((res>deadzone_hi)-(res<deadzone_lo));
        node0->weighted = w0b;
        uint32_t ctxw = bctx;
        if( (int32_t)abs32(res)<38 ) {
          if( (uint8_t)-- countdown ) {
            node0->b1 = countdown;
          } else {
            if( (uint8_t)node0->rate<8u ) {
              uint8_t b0n = node0->rate+1;
              node0->rate = b0n;
              node0->b1 = p2_b1_reload[b0n-1];
              node0->weighted = 2*w0b;
            } else {
              node0->b1 = countdown;
            }
          }
        }
        if( !alphabet_reduced ) {
          // The mirrored context: the same counter with the top bits of its
          // index flipped, which is the model's guess at what a symmetric
          // neighbourhood would have used.
          //
          // This was `mir_top`, a `uint16_t*` walking `P2Count` two words at a
          // time -- `[0]` the rate byte, `[1]` the weight, `[2]`/`[3]` the next
          // record's, `[-2]`/`[-1]` the previous one's.  It is a `P2Count*`, and
          // the name is reused a hundred lines below for a `uint16_t*` that
          // genuinely does walk a `P2Freq` as words, which is why one name did
          // two jobs.
          //
          // The decompilation also had `prefetch(&mir_top)` *before* the
          // assignment, which asks for the cache line holding a local the
          // function is about to write anyway.  Every other prefetch below is of
          // the counter the next few lines will read; this one is now too.
          P2Count *mir = &p2_ctr[32768*bank+(ctxw^0x7FF0)];
          __builtin_prefetch(mir, 0, 1);
          res2 = (nb_sum[2*bank+1])+res;
          int32_t lowbits = ctxw&3;
          if( (uint32_t)lowbits>=3||(w1 = node0[1].weighted, e1 = res2-p2_pred(w1, (uint8_t)node0[1].rate), go = lowbits<=0, node0[1].weighted = (int16_t)w1+((32*((e1>deadzone_hi)-(uint32_t)(e1<deadzone_lo))+e1+1)>>1), !go) ) {
            p2_update(&node0[-1], res2, 2);
          }
          // Three update blocks follow, one per branch, and each walks these
          // eleven bits in order doing four or five things to each: bump the
          // direct counter at shift 2, the rotated one at 3, nudge the record
          // after the direct one, bump the record before it, and bump the
          // mirrored one with the residual negated.  Which of the five, and at
          // which bit, is the whole of what separates the three.
          //
          // For several rounds this said they were **not** rolled into loops,
          // "a decision rather than an omission", and listed four exceptions at
          // k = 0, 4, 9 and 10.  Every one of the four has since dissolved, and
          // none of them dissolved by being argued with:
          //
          //   * "k = 0's nudge and k = 4's and k = 9's mirror bumps are written
          //     longhand" -- they were, and they are `p2_nudge` and `p2_update`
          //     now.  Two of the three had already been converted; the last two
          //     went when `p2_nudge` learned its shift instead of fixing it
          //     at 2.
          //   * "k = 4 subtracts into the residual variable itself" -- it does,
          //     through `p2_update_into`, and *nothing reads the residual
          //     afterwards*.  A write-back nobody reads is `p2_update`.
          //   * "k = 9 in the third block uses `res_t`, a different residual" --
          //     `res_t = res_c` two lines above and `res_c = res_t` two lines
          //     below.  It is the same residual under a spill's name.
          //   * "k = 10's mirror bump straddles the join and happens once" --
          //     true, and both arms reach it with `-res_c`, so it is one line
          //     after the join rather than a name each arm has to set.
          //
          // What settled it was said to be that "hoisting the longhand cases
          // out of their place reorders counter updates against each other" --
          // `direct`, `mirror` and `rot` index one bank and can name the same
          // record for different k, so the order is not free.  That is true and
          // it is why nothing is hoisted: each loop below does the same five
          // things in the same order the unrolled text did, and the only lines
          // that moved are the four `-res_c` under four names.
          P2Count* bankp = &p2_ctr[32768*bank];
          res_c = res2;
          // The eleven context bits this counter bank tracks, and the three
          // counters each one names: `direct` at the context with that bit
          // flipped, `mirror` at the same one with the top bits flipped as
          // well, and `rot` at the context with its low nibble rotated by
          // `p2_ctx_rotate`.
          //
          // Sixty-three lines, thirty-three names -- `direct[0]`, `mirror[0]`,
          // `rot[0]`, `direct[1]` and so on down -- and eleven of the thirty-three
          // indices were written as a hex constant apiece: `ctxw^0x3FF0`,
          // `ctxw^0x5FF0`, `ctxw^0x77F0`.  Every one of those eleven is
          // `ctxw ^ bit ^ 0x7FF0`, which is what "the top bits flipped" means
          // and what none of the constants said.  The last two were spelled a
          // third way again, through `x0020` and `x0010`.
          //
          // The prefetches were interleaved through the thirty-three
          // declarations in the order MSVC scheduled the loads; they are hints
          // and their order is not observable.
          static const uint32_t p2_bank_bits[11] = {
            0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40,
            0x20, 0x10,
          };
          P2Count *direct[11], *mirror[11], *rot[11];
          for( int32_t k = 0; k<11; ++k ) {
            const uint32_t x = ctxw^p2_bank_bits[k];
            direct[k] = &bankp[x];
            mirror[k] = &bankp[x^0x7FF0];
            rot[k] = &bankp[p2_ctx_rotate[(x>>2)&3]+(x&0xFFFFFFF3)];
            __builtin_prefetch(direct[k], 0, 1);
            __builtin_prefetch(mirror[k], 0, 1);
            __builtin_prefetch(rot[k], 0, 1);
          }
          // The second and third arms differ in one line of five: whether the
          // record after the direct counter is nudged.  `direct[0]` and
          // `rot[0]` are bumped before the branch that chooses between them and
          // `mirror[10]` after the two join, which is why this skips both ends.
          auto walk_bits = [&](bool nudge) {
            for( int32_t k = 0; k<11; ++k ) {
              if( k ) {
                p2_update(direct[k], res_c, 2);
                p2_update(rot[k], res_c, 3);
              }
              if( nudge )
                p2_nudge(&direct[k][1], res_c);
              p2_update(&direct[k][-1], res_c, 3);
              if( k<10 )
                p2_update(mirror[k], -res_c, 3);
            }
          };
          neg = -res_c;
          __builtin_prefetch(direct[8], 0, 1);
          w_top = mir[0].weighted;
          __builtin_prefetch(mirror[8], 0, 1);
          e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
          __builtin_prefetch(rot[8], 0, 1);
          // `e_top > deadzone_hi`, which is what the five siblings of this line
          // in this file already say.  It arrived as the x86 flags the compare
          // sets -- `!(lt ^ of | zf)` over `__OFSUB__` -- because signed
          // less-than *is* `SF != OF`, so `(e_top-deadzone_hi<0) ^ overflow` is
          // the less-than, and not-less-and-not-equal is greater.
          bump = 32*((e_top>deadzone_hi)-(e_top<deadzone_lo));
          __builtin_prefetch(direct[9], 0, 1);
          __builtin_prefetch(mirror[9], 0, 1);
          res_s = res_c;
          uint32_t w_new = bump+e_top+2;
          go = lowbits<3;
          // The partial store this was written as -- `LOWORD(w_new) = w_top +
          // (w_new>>2)` -- kept a high half that the next line throws away.
          // Sixteen bits in and sixteen bits out.
          mir[0].weighted = (int16_t)(w_top+(w_new>>2));
          __builtin_prefetch((rot[9]), 0, 1);
          __builtin_prefetch(direct[10], 0, 1);
          __builtin_prefetch(mirror[10], 0, 1);
          __builtin_prefetch(rot[10], 0, 1);
          if( go&&(w_top3 = mir[1].weighted, b_top3 = (uint8_t)mir[1].rate, res_c = res_s, go = lowbits<=0, mir[1].weighted = (int16_t)(((uint32_t)(-res_s-p2_pred(w_top3, b_top3)+2)>>2)+w_top3), go) ) {
            res_c = res_s;
            // The first arm owns both ends and never touches the record
            // before the direct counter, so it is its own loop.
            for( int32_t k = 0; k<11; ++k ) {
              p2_update(direct[k], res_c, 2);
              p2_update(rot[k], res_c, 3);
              p2_nudge(&direct[k][1], res_c);
              p2_update(mirror[k], -res_c, 3);
            }
          } else {
            res_c = res_s;
            p2_nudge(&mir[-1], -res_s, 3);
            p2_update(direct[0], res_c, 2);
            go = lowbits<3;
            p2_update(rot[0], res_c, 3);
            walk_bits(go);
            // The one counter whose update straddles the join.  Both arms
            // reached it with `-res_c` under a name of their own.
            p2_update(mirror[10], -res_c, 3);
          }
        }
      }
  }
}

uint32_t AltP2Block::alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in) {
  // The frequency record whose bins this symbol's decision moves.  It was
  // `mir_top` again -- a `uint16_t*` over a `P2Freq`, where index 1 is `f[0]`
  // and `3-is_dec` is `f[2]` or `f[1]`.  Every one of its thirteen
  // assignments was a `P2Freq*` with the cast written on it, and every use was
  // one of those two bins, so there was never anything a word pointer was for.
  P2Freq* bin;
  int32_t is_dec;
  uint32_t step10;
  int32_t fold_sel, sel_alt;
  int32_t ctx_lo = ctx&0xF;
  int32_t sample16 = 16*sample_in;
  cursor[0]->val = 16*sample_in;
  cursor[0]->dval = cursor[0]->val-cursor[0]->dval;
  cursor[0][1].dval = 0;
  // The deadzone half-width for this context, and where the residual falls
  // against it.  `dead` climbs 0, 1, 2, 4 with the context's low nibble;
  // `sign` is 2 below the band, 1 inside or below the top, 0 above -- a
  // three-way split written as two comparisons summed.
  //
  // The decompilation had `dead` computed twice, both times as three sign
  // shifts of `(K-ctx_lo)`, and negated the unsigned sum before casting it to
  // `int32_t` -- which is `-dead` only because the conversion is
  // implementation-defined in the way gcc happens to define it.
  const int32_t dead = (ctx_lo>6)+(ctx_lo>4)+2*(ctx_lo>9);
  cursor[0]->sign = (resid_in<=dead)+(resid_in< -dead);
  cursor[0]->mag = abs32(resid_in);
  P2Ctx *cursor0 = cursor[0];
  float sample = (float)sample16;
  int32_t dl = sample16-cursor0[-1].val;
  cursor0->dleft = abs32(dl);
  int32_t du = sample16-cursor[1]->val;
  cursor[0]->dup = abs32(du);
  int32_t dul = sample16-cursor[1][-1].val;
  cursor[0]->dupleft = abs32(dul);
  int32_t dur = sample16-cursor[1][1].val;
  cursor[0]->dupright = abs32(dur);
  int32_t resid = (int16_t)(sample16-pred_prev);
  cursor[0]->err = resid;
  cursor[0]->aerr = abs32(resid);
  NbRow *wrow_cur = nb_cur;
  float ms1 = wrow_cur->w[14][1]+0.000099999997f;
  NbRow *wrow_b = cur[-1];
  float bias2 = bias[2];
  float n2_bias = bias[0];
  float d_bias2 = sample-bias2;
  float bias1 = bias[1];
  float d_bias = bias1-bias2;
  float ms_a = ((((sample-bias2)*(d_bias))-wrow_cur->w[14][0])*0.001f)+wrow_cur->w[14][0];
  float ms_b = ms1+(((d_bias*d_bias)-wrow_cur->w[14][1])*0.001f);
  wrow_cur->w[14][1] = ms_b;
  float ms_b10 = 0.1f*ms_b;
  if( (ms_b10)<=ms_a )
    ms_b10 = fminf(ms_b, ms_a);
  wrow_cur->w[14][0] = ms_b10;
  float conf = (1.0f-(ms_b10/(ms_b+576.0f)))*2.0f;
  nlms_track_two_rows(wrow_cur, wrow_b, (sample-bias1)*2.5999999f, d_bias2*conf, conf);
  nlms_predict_and_correct(wrow_cur, sample, n2_bias);
  *cur = nb_cur;
  ++cur;
  ++above;
  code_banks(sample16);
  int32_t ctx_l = ctx;
  ++cursor[0];
  P2Freq *frec = &freq[ctx_l];
  ++cursor[1];
  ++cursor[2];
  ++cursor[3];
  ++cursor[4];
  uint32_t step_v = frec[0].step;
  if( step_v>0x10 ) {
    is_dec = a4&1;
    int32_t ctx15 = ctx_l&0xF;
    uint32_t pair_ctx = ctx_pair[a4&1];
    if( ctx15<15 ) {
      bin = &frec[1];
      p2_rescale(bin);
      step10 = (10*(uint32_t)frec[1].step)>>4;
      if( a4 ) {
        bin->f[2-is_dec] += step10;
        update_binary_pair(model_strip(pair_ctx+1), (a4-1)>>1);
      } else {
        bin->f[0] += step10;
      }
      if( ctx15<=0 ) {
// The residual coder, entered two ways: by falling into it when the context's
// low nibble is zero, and by the `goto` at the bottom of this function, which
// jumps back *into* this block from outside it.
//
// It stays a label.  A structured form needs a loop around the `if` and a flag
// to say which way it was entered -- one more level of nesting and one more
// variable, to remove one `goto` that already says what it does.  That is the
// trade the refactoring plan names when it says to keep state-machine labels,
// and this is one.
code_residual:
        step_v = ctx;
        if( freq[step_v].step<=0x1Au )
          return step_v;
        fold_sel = 2-(fold[(uint8_t)-resid_in]&1);
        if( !fold[(uint8_t)-resid_in] )
          fold_sel = fold[(uint8_t)-resid_in];
        step_v = ctx_w[4].w[2-ctx_w[4].sel]+ctx_w[3].w[2-ctx_w[3].sel]+ctx_w[2].w[2-ctx_w[2].sel]+ctx_w[1].w[2-ctx_w[1].sel]+(ctx_w[0].w[1]+(ctx&0x3F));
        P2Freq *frec_step = &freq[step_v];
        if( ctx15<15 ) {
          P2Freq *frec2 = &frec_step[1];
          bin = frec2;
          p2_freq_add(frec2, fold_sel, 4);
          if( ctx15<=0 )
            goto counters_done;
        } else {
        }
        bin = &frec_step[-1];
        p2_freq_add(bin, fold_sel, 4);
counters_done:
        if( a4 ) {
          int32_t n2_half = (a4-1)>>1;
          int32_t hi_nibble = (uint8_t)pair_ctx&0xF0;
          if( hi_nibble>=0xF0||(update_binary_pair(model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
            update_binary_pair(model_strip(pair_ctx-16), n2_half);
          }
        }
        P2Freq *prec0 = &frec_step[0];
        p2_freq_add(prec0, fold_sel, 6);
        if( !plane_idx||freq[ctx].step>0x100u ) {
          sel_alt = 2-is_dec;
          if( !a4 )
            sel_alt = 0;
          is_dec = sel_alt;
          // The five context banks, bumped.  `bump_bank` is one bank's worth;
          // the five were written out in full, thirty-six lines apiece, and its
          // comment says what they had in common.
          for( int32_t k = 0; k<5; ++k )
            bump_bank(k, step_v, ctx15, fold_sel, is_dec);
        }
        return step_v;
      }
      frec = &freq[ctx];
    }
    bin = &frec[-1];
    p2_rescale(bin);
    uint32_t step13 = (13*(uint32_t)frec[-1].step)>>4;
    if( a4 ) {
      bin->f[2-is_dec] += step13;
      update_binary_pair(model_strip(pair_ctx-1), (a4-1)>>1);
    } else {
      bin->f[0] += step13;
    }
    goto code_residual;
  }
  return step_v;
}
