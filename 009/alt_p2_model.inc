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
  int32_t bctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, neg,
      w_top, e_top, bump, res_s, w_top3, w_topm1, wm0400x, neg_b, res_t, res_m0010,
      neg_c, neg_d, wd4000p1a, pd4000p1a, wm0400z, wm0020a, pm0020a, res_c;
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
            wnode0m1a = node0[-1].weighted;
            enode0m1a = res2-p2_pred(wnode0m1a, (uint8_t)node0[-1].rate);
            node0[-1].weighted = (int16_t)wnode0m1a+((32*((enode0m1a>deadzone_hi)-(uint32_t)(enode0m1a<deadzone_lo))+enode0m1a+2)>>2);
          }
          // Three update blocks follow, one per branch, and each walks these
          // eleven bits in order doing four or five things to each: bump the
          // direct counter at shift 2, the rotated one at 3, nudge the record
          // after the direct one, bump the record before it, and bump the
          // mirrored one with the residual negated.  Which of the five, and at
          // which bit, is the whole of what separates the three.
          //
          // **They are not rolled into loops, and that is a decision rather
          // than an omission.**  Every one of the three has exceptions -- at
          // k = 0, 4, 9 and 10 -- and they are not decoration:
          //
          //   * k = 0's nudge and k = 4's and k = 9's mirror bumps are written
          //     longhand at some of the sites, because they are partial stores
          //     or subtract into the residual variable itself;
          //   * k = 9 in the third block uses `res_t`, a *different* residual
          //     from the other ten;
          //   * k = 10's mirror bump straddles the join of two arms and
          //     happens once, after them.
          //
          // A loop would carry four `if( k == … )` arms, and -- the reason
          // that settles it -- hoisting the longhand cases out of their place
          // in the sequence reorders counter updates against each other.
          // `direct`, `mirror` and `rot` are all indices into one bank and can
          // name the same record for different k, and every update reads
          // before it writes, so the order is not free.  The indices above are
          // what makes the shape readable; rolling it is what would make it
          // wrong.
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
          __builtin_prefetch(((P2Count*)(rot[9])), 0, 1);
          __builtin_prefetch(direct[10], 0, 1);
          __builtin_prefetch(mirror[10], 0, 1);
          __builtin_prefetch(rot[10], 0, 1);
          if( go&&(w_top3 = mir[1].weighted, b_top3 = (uint8_t)mir[1].rate, res_c = res_s, go = lowbits<=0, mir[1].weighted = (int16_t)(((uint32_t)(-res_s-p2_pred(w_top3, b_top3)+2)>>2)+w_top3), go) ) {
            res_c = res_s;
            p2_update(direct[0], res_s, 2);
            p2_update(rot[0], res_s, 3);
            wd4000p1a = direct[0][1].weighted;
            pd4000p1a = p2_pred(wd4000p1a, (uint8_t)direct[0][1].rate);
            int32_t nres3 = -res_s;
            direct[0][1].weighted = (int16_t)((uint32_t)(res_s-pd4000p1a+2)>>2)+wd4000p1a;
            p2_update(mirror[0], -res_s, 3);
            p2_update(direct[1], res_c, 2);
            p2_update(rot[1], res_c, 3);
            p2_nudge(&direct[1][1], res_c);
            p2_update(mirror[1], nres3, 3);
            p2_update(direct[2], res_c, 2);
            p2_update(rot[2], res_c, 3);
            p2_nudge(&direct[2][1], res_c);
            p2_update(mirror[2], nres3, 3);
            p2_update(direct[3], res_c, 2);
            p2_update(rot[3], res_c, 3);
            p2_nudge(&direct[3][1], res_c);
            p2_update(mirror[3], nres3, 3);
            p2_update(direct[4], res_c, 2);
            p2_update(rot[4], res_c, 3);
            p2_nudge(&direct[4][1], res_c);
            // Not `p2_update`: this subtracts into `nres3` itself, and the
            // residual is read again below.  A helper taking it by value
            // would drop that write.
            wm0400z = mirror[4]->weighted;
            nres3 -= p2_pred(wm0400z, mirror[4]->rate);
            mirror[4]->weighted = p2_bump(wm0400z, nres3, 3);
            p2_update(direct[5], res_c, 2);
            p2_update(rot[5], res_c, 3);
            p2_nudge(&direct[5][1], res_c);
            int32_t nres2 = -res_c;
            p2_update(mirror[5], -res_c, 3);
            p2_update(direct[6], res_c, 2);
            p2_update(rot[6], res_c, 3);
            p2_nudge(&direct[6][1], res_c);
            p2_update(mirror[6], nres2, 3);
            p2_update(direct[7], res_c, 2);
            p2_update(rot[7], res_c, 3);
            p2_nudge(&direct[7][1], res_c);
            p2_update(mirror[7], nres2, 3);
            p2_update(direct[8], res_c, 2);
            p2_update(rot[8], res_c, 3);
            p2_nudge(&direct[8][1], res_c);
            p2_update(mirror[8], nres2, 3);
            p2_update(direct[9], res_c, 2);
            p2_update(rot[9], res_c, 3);
            p2_nudge(&direct[9][1], res_c);
            wm0020a = mirror[9]->weighted;
            pm0020a = p2_pred(wm0020a, mirror[9]->rate);
            mirror[9]->weighted = wm0020a+((32*((nres2-pm0020a>deadzone_hi)-(uint32_t)(nres2-pm0020a<deadzone_lo))+nres2-pm0020a+4)>>3);
            p2_update(direct[10], res_c, 2);
            p2_update(rot[10], res_c, 3);
            p2_nudge(&direct[10][1], res_c);
            p2_update(mirror[10], -res_c, 3);
          } else {
            w_topm1 = mir[-1].weighted;
            res_c = res_s;
            mir[-1].weighted = (int16_t)(((uint32_t)(-res_s-p2_pred(w_topm1, (uint8_t)mir[-1].rate)+4)>>3)+w_topm1);
            p2_update(direct[0], res_c, 2);
            go = lowbits<3;
            p2_update(rot[0], res_c, 3);
            if( go ) {
              p2_nudge(&direct[0][1], res_c);
              neg_c = -res_c;
              p2_update(&direct[0][-1], res_c, 3);
              p2_update(mirror[0], neg_c, 3);
              p2_update(direct[1], res_c, 2);
              p2_update(rot[1], res_c, 3);
              p2_nudge(&direct[1][1], res_c);
              p2_update(&direct[1][-1], res_c, 3);
              p2_update(mirror[1], neg_c, 3);
              p2_update(direct[2], res_c, 2);
              p2_update(rot[2], res_c, 3);
              p2_nudge(&direct[2][1], res_c);
              p2_update(&direct[2][-1], res_c, 3);
              p2_update(mirror[2], neg_c, 3);
              p2_update(direct[3], res_c, 2);
              p2_update(rot[3], res_c, 3);
              p2_nudge(&direct[3][1], res_c);
              p2_update(&direct[3][-1], res_c, 3);
              p2_update(mirror[3], neg_c, 3);
              p2_update(direct[4], res_c, 2);
              p2_update(rot[4], res_c, 3);
              p2_nudge(&direct[4][1], res_c);
              p2_update(&direct[4][-1], res_c, 3);
              int32_t nres5 = -res_c;
              p2_update(mirror[4], -res_c, 3);
              p2_update(direct[5], res_c, 2);
              p2_update(rot[5], res_c, 3);
              p2_nudge(&direct[5][1], res_c);
              p2_update(&direct[5][-1], res_c, 3);
              p2_update(mirror[5], nres5, 3);
              p2_update(direct[6], res_c, 2);
              p2_update(rot[6], res_c, 3);
              p2_nudge(&direct[6][1], res_c);
              p2_update(&direct[6][-1], res_c, 3);
              p2_update(mirror[6], nres5, 3);
              p2_update(direct[7], res_c, 2);
              p2_update(rot[7], res_c, 3);
              p2_nudge(&direct[7][1], res_c);
              p2_update(&direct[7][-1], res_c, 3);
              p2_update(mirror[7], nres5, 3);
              p2_update(direct[8], res_c, 2);
              p2_update(rot[8], res_c, 3);
              p2_nudge(&direct[8][1], res_c);
              neg_d = -res_c;
              p2_update(&direct[8][-1], res_c, 3);
              p2_update(mirror[8], neg_d, 3);
              p2_update(direct[9], res_c, 2);
              p2_update(rot[9], res_c, 3);
              p2_nudge(&direct[9][1], res_c);
              p2_update(&direct[9][-1], res_c, 3);
              p2_update(mirror[9], neg_d, 3);
              p2_update(direct[10], res_c, 2);
              p2_update(rot[10], res_c, 3);
              p2_nudge(&direct[10][1], res_c);
              p2_update(&direct[10][-1], res_c, 3);
              res_m0010 = neg_d;
            } else {
              p2_update(&direct[0][-1], res_c, 3);
              int32_t nres1 = -res_c;
              p2_update(mirror[0], -res_c, 3);
              p2_update(direct[1], res_c, 2);
              p2_update(rot[1], res_c, 3);
              p2_update(&direct[1][-1], res_c, 3);
              p2_update(mirror[1], nres1, 3);
              p2_update(direct[2], res_c, 2);
              p2_update(rot[2], res_c, 3);
              p2_update(&direct[2][-1], res_c, 3);
              p2_update(mirror[2], nres1, 3);
              p2_update(direct[3], res_c, 2);
              p2_update(rot[3], res_c, 3);
              p2_update(&direct[3][-1], res_c, 3);
              p2_update(mirror[3], nres1, 3);
              p2_update(direct[4], res_c, 2);
              p2_update(rot[4], res_c, 3);
              p2_update(&direct[4][-1], res_c, 3);
              // Not `p2_update`: this subtracts into `nres1` itself, and the
              // residual is read again below.  A helper taking it by value
              // would drop that write.
              wm0400x = mirror[4]->weighted;
              nres1 -= p2_pred(wm0400x, mirror[4]->rate);
              mirror[4]->weighted = p2_bump(wm0400x, nres1, 3);
              p2_update(direct[5], res_c, 2);
              p2_update(rot[5], res_c, 3);
              neg_b = -res_c;
              p2_update(&direct[5][-1], res_c, 3);
              p2_update(mirror[5], neg_b, 3);
              p2_update(direct[6], res_c, 2);
              p2_update(rot[6], res_c, 3);
              p2_update(&direct[6][-1], res_c, 3);
              p2_update(mirror[6], neg_b, 3);
              p2_update(direct[7], res_c, 2);
              p2_update(rot[7], res_c, 3);
              p2_update(&direct[7][-1], res_c, 3);
              p2_update(mirror[7], neg_b, 3);
              p2_update(direct[8], res_c, 2);
              p2_update(rot[8], res_c, 3);
              p2_update(&direct[8][-1], res_c, 3);
              p2_update(mirror[8], neg_b, 3);
              res_t = res_c;
              p2_update(direct[9], res_c, 2);
              res_c = res_t;
              p2_update(rot[9], res_t, 3);
              p2_update(&direct[9][-1], res_t, 3);
              int32_t nres4 = -res_t;
              p2_update(mirror[9], -res_t, 3);
              p2_update(direct[10], res_c, 2);
              p2_update(rot[10], res_c, 3);
              p2_update(&direct[10][-1], res_c, 3);
              res_m0010 = nres4;
            }
            // The one counter whose update straddles the join: both arms choose a
            // residual, and the bump happens once, after them.
            p2_update(mirror[10], res_m0010, 3);
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
        P2Freq *p2_rec = frec_step;
        if( ctx15<15 ) {
          P2Freq *frec2 = &frec_step[1];
          bin = frec2;
          p2_freq_add(frec2, fold_sel, 4);
          if( ctx15<=0 )
            goto counters_done;
        } else {
        }
        bin = &p2_rec[-1];
        p2_freq_add(bin, fold_sel, 4);
counters_done:
        if( a4 ) {
          int32_t n2_half = (a4-1)>>1;
          int32_t hi_nibble = (uint8_t)pair_ctx&0xF0;
          if( hi_nibble>=0xF0||(update_binary_pair(model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
            update_binary_pair(model_strip(pair_ctx-16), n2_half);
          }
        }
        P2Freq *prec0 = &p2_rec[0];
        p2_freq_add(prec0, fold_sel, 6);
        if( !plane_idx||freq[ctx].step>0x100u ) {
          sel_alt = 2-is_dec;
          if( !a4 )
            sel_alt = 0;
          is_dec = sel_alt;
          // The five context banks, bumped.  Each bank's selector picks one of
          // two shapes: a selector of 1 bumps the group's record, the one after
          // it, the first record of the next group and the one before -- four
          // counts at 3, 4, 2 and 6 sixteenths -- and any other selector bumps
          // three, at 7, 5 and 6.  The bank the step came from is bumped first
          // either way, at 4 or 7.
          //
          // Written out five times, thirty-six lines apiece, over `ctx_w[0]`
          // through `ctx_w[4]` with a fresh set of temporaries in each:
          // `frec3`/`frec4b`/`frec5`/`frec4c` for the first record,
          // `idx0`/`rec_idx` for the next group, `grp`/`p2_rec`/`gtop` for the
          // group.  Nothing else differed between the five, and not one of
          // those names outlives the copy it was written in.
          for( int32_t k = 0; k<5; ++k ) {
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
              if( ctx15<15 )
                p2_freq_add(&group[1], is_dec, 5);
              if( ctx15>0 )
                p2_freq_add(&group[-1], is_dec, 6);
            }
          }
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
