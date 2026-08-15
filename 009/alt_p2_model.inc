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
  P2Count* mir;
  uint8_t b_top3;
  bool go;
  int32_t bctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, x0020, x0010, neg,
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
          mir = &p2_ctr[32768*bank+(ctxw^0x7FF0)];
          __builtin_prefetch(mir, 0, 1);
          res2 = (nb_sum[2*bank+1])+res;
          int32_t lowbits = ctxw&3;
          if( (uint32_t)lowbits>=3||(w1 = node0[1].weighted, e1 = res2-p2_pred(w1, (uint8_t)node0[1].rate), go = lowbits<=0, node0[1].weighted = (int16_t)w1+((32*((e1>deadzone_hi)-(uint32_t)(e1<deadzone_lo))+e1+1)>>1), !go) ) {
            wnode0m1a = node0[-1].weighted;
            enode0m1a = res2-p2_pred(wnode0m1a, (uint8_t)node0[-1].rate);
            node0[-1].weighted = (int16_t)wnode0m1a+((32*((enode0m1a>deadzone_hi)-(uint32_t)(enode0m1a<deadzone_lo))+enode0m1a+2)>>2);
          }
          P2Count* bankp = &p2_ctr[32768*bank];
          res_c = res2;
          __builtin_prefetch(&bankp[(ctxw^0x4000)], 0, 1);
          P2Count *d4000 = &bankp[(ctxw^0x4000)];
          P2Count *m4000 = &bankp[(ctxw^0x3FF0)];
          __builtin_prefetch(m4000, 0, 1);
          P2Count *r4000 = &bankp[p2_ctx_rotate[((ctxw^0x4000)>>2)&3]+((ctxw^0x4000)&0xFFFFFFF3)];
          __builtin_prefetch(r4000, 0, 1);
          __builtin_prefetch(&bankp[(ctxw^0x2000)], 0, 1);
          P2Count *d2000 = &bankp[(ctxw^0x2000)];
          __builtin_prefetch(&bankp[(ctxw^0x5FF0)], 0, 1);
          P2Count *m2000 = &bankp[(ctxw^0x5FF0)];
          P2Count *r2000 = &bankp[p2_ctx_rotate[((ctxw^0x2000)>>2)&3]+((ctxw^0x2000)&0xFFFFFFF3)];
          __builtin_prefetch(r2000, 0, 1);
          __builtin_prefetch(&bankp[(ctxw^0x1000)], 0, 1);
          P2Count *d1000 = &bankp[(ctxw^0x1000)];
          __builtin_prefetch(&bankp[(ctxw^0x6FF0)], 0, 1);
          P2Count *m1000 = &bankp[(ctxw^0x6FF0)];
          P2Count *r1000 = &bankp[p2_ctx_rotate[((ctxw^0x1000)>>2)&3]+((ctxw^0x1000)&0xFFFFFFF3)];
          __builtin_prefetch(r1000, 0, 1);
          P2Count *d0800 = &bankp[(ctxw^0x800)];
          P2Count *m0800 = &bankp[(ctxw^0x77F0)];
          P2Count *r0800 = &bankp[p2_ctx_rotate[((ctxw^0x800)>>2)&3]+((ctxw^0x800)&0xFFFFFFF3)];
          P2Count *d0400 = &bankp[(ctxw^0x400)];
          __builtin_prefetch(d0800, 0, 1);
          __builtin_prefetch(m0800, 0, 1);
          __builtin_prefetch(r0800, 0, 1);
          P2Count *m0400 = &bankp[(ctxw^0x7BF0)];
          P2Count *r0400 = &bankp[p2_ctx_rotate[((ctxw^0x400)>>2)&3]+((ctxw^0x400)&0xFFFFFFF3)];
          P2Count *d0200 = &bankp[(ctxw^0x200)];
          __builtin_prefetch(d0400, 0, 1);
          P2Count *m0200 = &bankp[(ctxw^0x7DF0)];
          P2Count *r0200 = &bankp[p2_ctx_rotate[((ctxw^0x200)>>2)&3]+((ctxw^0x200)&0xFFFFFFF3)];
          __builtin_prefetch(m0400, 0, 1);
          __builtin_prefetch(r0400, 0, 1);
          P2Count *d0100 = &bankp[(ctxw^0x100)];
          uint32_t ri0100 = p2_ctx_rotate[((ctxw^0x100)>>2)&3]+((ctxw^0x100)&0xFFFFFFF3);
          P2Count *m0100 = &bankp[(ctxw^0x7EF0)];
          __builtin_prefetch(d0200, 0, 1);
          P2Count *r0100 = &bankp[ri0100];
          P2Count *d0080 = &bankp[(ctxw^0x80)];
          __builtin_prefetch(m0200, 0, 1);
          __builtin_prefetch(r0200, 0, 1);
          P2Count *m0080 = &bankp[(ctxw^0x7F70)];
          P2Count *r0080 = &bankp[p2_ctx_rotate[((ctxw^0x80)>>2)&3]+((ctxw^0x80)&0xFFFFFFF3)];
          P2Count *d0040 = &bankp[(ctxw^0x40)];
          __builtin_prefetch(d0100, 0, 1);
          P2Count *m0040 = &bankp[(ctxw^0x7FB0)];
          P2Count *r0040 = &bankp[p2_ctx_rotate[((ctxw^0x40)>>2)&3]+((ctxw^0x40)&0xFFFFFFF3)];
          __builtin_prefetch(m0100, 0, 1);
          __builtin_prefetch(r0100, 0, 1);
          x0020 = ctxw^0x20;
          x0010 = ctxw^0x10;
          P2Count *d0020 = &bankp[x0020];
          P2Count *m0020 = &bankp[(x0020^0x7FF0)];
          __builtin_prefetch(d0080, 0, 1);
          __builtin_prefetch(m0080, 0, 1);
          P2Count *r0020 = &bankp[p2_ctx_rotate[(x0020>>2)&3]+(x0020&0xFFFFFFF3)];
          P2Count *d0010 = &bankp[x0010];
          uint32_t ri0010 = p2_ctx_rotate[(x0010>>2)&3]+(x0010&0xFFFFFFF3);
          P2Count *m0010 = &bankp[(x0010^0x7FF0)];
          __builtin_prefetch(r0080, 0, 1);
          P2Count *r0010 = &bankp[ri0010];
          neg = -res_c;
          __builtin_prefetch(d0040, 0, 1);
          w_top = mir[0].weighted;
          __builtin_prefetch(m0040, 0, 1);
          e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
          __builtin_prefetch(r0040, 0, 1);
          // `e_top > deadzone_hi`, which is what the five siblings of this line
          // in this file already say.  It arrived as the x86 flags the compare
          // sets -- `!(lt ^ of | zf)` over `__OFSUB__` -- because signed
          // less-than *is* `SF != OF`, so `(e_top-deadzone_hi<0) ^ overflow` is
          // the less-than, and not-less-and-not-equal is greater.
          bump = 32*((e_top>deadzone_hi)-(e_top<deadzone_lo));
          __builtin_prefetch(d0020, 0, 1);
          __builtin_prefetch(m0020, 0, 1);
          res_s = res_c;
          uint32_t w_new = bump+e_top+2;
          go = lowbits<3;
          // The partial store this was written as -- `LOWORD(w_new) = w_top +
          // (w_new>>2)` -- kept a high half that the next line throws away.
          // Sixteen bits in and sixteen bits out.
          mir[0].weighted = (int16_t)(w_top+(w_new>>2));
          __builtin_prefetch(((P2Count*)(r0020)), 0, 1);
          __builtin_prefetch(d0010, 0, 1);
          __builtin_prefetch(m0010, 0, 1);
          __builtin_prefetch(r0010, 0, 1);
          if( go&&(w_top3 = mir[1].weighted, b_top3 = (uint8_t)mir[1].rate, res_c = res_s, go = lowbits<=0, mir[1].weighted = (int16_t)(((uint32_t)(-res_s-p2_pred(w_top3, b_top3)+2)>>2)+w_top3), go) ) {
            res_c = res_s;
            p2_update(d4000, res_s, 2);
            p2_update(r4000, res_s, 3);
            wd4000p1a = d4000[1].weighted;
            pd4000p1a = p2_pred(wd4000p1a, (uint8_t)d4000[1].rate);
            int32_t nres3 = -res_s;
            d4000[1].weighted = (int16_t)((uint32_t)(res_s-pd4000p1a+2)>>2)+wd4000p1a;
            p2_update(m4000, -res_s, 3);
            p2_update(d2000, res_c, 2);
            p2_update(r2000, res_c, 3);
            p2_nudge(&d2000[1], res_c);
            p2_update(m2000, nres3, 3);
            p2_update(d1000, res_c, 2);
            p2_update(r1000, res_c, 3);
            p2_nudge(&d1000[1], res_c);
            p2_update(m1000, nres3, 3);
            p2_update(d0800, res_c, 2);
            p2_update(r0800, res_c, 3);
            p2_nudge(&d0800[1], res_c);
            p2_update(m0800, nres3, 3);
            p2_update(d0400, res_c, 2);
            p2_update(r0400, res_c, 3);
            p2_nudge(&d0400[1], res_c);
            // Not `p2_update`: this subtracts into `nres3` itself, and the
            // residual is read again below.  A helper taking it by value
            // would drop that write.
            wm0400z = m0400->weighted;
            nres3 -= p2_pred(wm0400z, m0400->rate);
            m0400->weighted = p2_bump(wm0400z, nres3, 3);
            p2_update(d0200, res_c, 2);
            p2_update(r0200, res_c, 3);
            p2_nudge(&d0200[1], res_c);
            int32_t nres2 = -res_c;
            p2_update(m0200, -res_c, 3);
            p2_update(d0100, res_c, 2);
            p2_update(r0100, res_c, 3);
            p2_nudge(&d0100[1], res_c);
            p2_update(m0100, nres2, 3);
            p2_update(d0080, res_c, 2);
            p2_update(r0080, res_c, 3);
            p2_nudge(&d0080[1], res_c);
            p2_update(m0080, nres2, 3);
            p2_update(d0040, res_c, 2);
            p2_update(r0040, res_c, 3);
            p2_nudge(&d0040[1], res_c);
            p2_update(m0040, nres2, 3);
            p2_update(d0020, res_c, 2);
            p2_update(r0020, res_c, 3);
            p2_nudge(&d0020[1], res_c);
            wm0020a = m0020->weighted;
            pm0020a = p2_pred(wm0020a, m0020->rate);
            m0020->weighted = wm0020a+((32*((nres2-pm0020a>deadzone_hi)-(uint32_t)(nres2-pm0020a<deadzone_lo))+nres2-pm0020a+4)>>3);
            p2_update(d0010, res_c, 2);
            p2_update(r0010, res_c, 3);
            p2_nudge(&d0010[1], res_c);
            p2_update(m0010, -res_c, 3);
          } else {
            w_topm1 = mir[-1].weighted;
            res_c = res_s;
            mir[-1].weighted = (int16_t)(((uint32_t)(-res_s-p2_pred(w_topm1, (uint8_t)mir[-1].rate)+4)>>3)+w_topm1);
            p2_update(d4000, res_c, 2);
            go = lowbits<3;
            p2_update(r4000, res_c, 3);
            if( go ) {
              p2_nudge(&d4000[1], res_c);
              neg_c = -res_c;
              p2_update(&d4000[-1], res_c, 3);
              p2_update(m4000, neg_c, 3);
              p2_update(d2000, res_c, 2);
              p2_update(r2000, res_c, 3);
              p2_nudge(&d2000[1], res_c);
              p2_update(&d2000[-1], res_c, 3);
              p2_update(m2000, neg_c, 3);
              p2_update(d1000, res_c, 2);
              p2_update(r1000, res_c, 3);
              p2_nudge(&d1000[1], res_c);
              p2_update(&d1000[-1], res_c, 3);
              p2_update(m1000, neg_c, 3);
              p2_update(d0800, res_c, 2);
              p2_update(r0800, res_c, 3);
              p2_nudge(&d0800[1], res_c);
              p2_update(&d0800[-1], res_c, 3);
              p2_update(m0800, neg_c, 3);
              p2_update(d0400, res_c, 2);
              p2_update(r0400, res_c, 3);
              p2_nudge(&d0400[1], res_c);
              p2_update(&d0400[-1], res_c, 3);
              int32_t nres5 = -res_c;
              p2_update(m0400, -res_c, 3);
              p2_update(d0200, res_c, 2);
              p2_update(r0200, res_c, 3);
              p2_nudge(&d0200[1], res_c);
              p2_update(&d0200[-1], res_c, 3);
              p2_update(m0200, nres5, 3);
              p2_update(d0100, res_c, 2);
              p2_update(r0100, res_c, 3);
              p2_nudge(&d0100[1], res_c);
              p2_update(&d0100[-1], res_c, 3);
              p2_update(m0100, nres5, 3);
              p2_update(d0080, res_c, 2);
              p2_update(r0080, res_c, 3);
              p2_nudge(&d0080[1], res_c);
              p2_update(&d0080[-1], res_c, 3);
              p2_update(m0080, nres5, 3);
              p2_update(d0040, res_c, 2);
              p2_update(r0040, res_c, 3);
              p2_nudge(&d0040[1], res_c);
              neg_d = -res_c;
              p2_update(&d0040[-1], res_c, 3);
              p2_update(m0040, neg_d, 3);
              p2_update(d0020, res_c, 2);
              p2_update(r0020, res_c, 3);
              p2_nudge(&d0020[1], res_c);
              p2_update(&d0020[-1], res_c, 3);
              p2_update(m0020, neg_d, 3);
              p2_update(d0010, res_c, 2);
              p2_update(r0010, res_c, 3);
              p2_nudge(&d0010[1], res_c);
              p2_update(&d0010[-1], res_c, 3);
              res_m0010 = neg_d;
            } else {
              p2_update(&d4000[-1], res_c, 3);
              int32_t nres1 = -res_c;
              p2_update(m4000, -res_c, 3);
              p2_update(d2000, res_c, 2);
              p2_update(r2000, res_c, 3);
              p2_update(&d2000[-1], res_c, 3);
              p2_update(m2000, nres1, 3);
              p2_update(d1000, res_c, 2);
              p2_update(r1000, res_c, 3);
              p2_update(&d1000[-1], res_c, 3);
              p2_update(m1000, nres1, 3);
              p2_update(d0800, res_c, 2);
              p2_update(r0800, res_c, 3);
              p2_update(&d0800[-1], res_c, 3);
              p2_update(m0800, nres1, 3);
              p2_update(d0400, res_c, 2);
              p2_update(r0400, res_c, 3);
              p2_update(&d0400[-1], res_c, 3);
              // Not `p2_update`: this subtracts into `nres1` itself, and the
              // residual is read again below.  A helper taking it by value
              // would drop that write.
              wm0400x = m0400->weighted;
              nres1 -= p2_pred(wm0400x, m0400->rate);
              m0400->weighted = p2_bump(wm0400x, nres1, 3);
              p2_update(d0200, res_c, 2);
              p2_update(r0200, res_c, 3);
              neg_b = -res_c;
              p2_update(&d0200[-1], res_c, 3);
              p2_update(m0200, neg_b, 3);
              p2_update(d0100, res_c, 2);
              p2_update(r0100, res_c, 3);
              p2_update(&d0100[-1], res_c, 3);
              p2_update(m0100, neg_b, 3);
              p2_update(d0080, res_c, 2);
              p2_update(r0080, res_c, 3);
              p2_update(&d0080[-1], res_c, 3);
              p2_update(m0080, neg_b, 3);
              p2_update(d0040, res_c, 2);
              p2_update(r0040, res_c, 3);
              p2_update(&d0040[-1], res_c, 3);
              p2_update(m0040, neg_b, 3);
              res_t = res_c;
              p2_update(d0020, res_c, 2);
              res_c = res_t;
              p2_update(r0020, res_t, 3);
              p2_update(&d0020[-1], res_t, 3);
              int32_t nres4 = -res_t;
              p2_update(m0020, -res_t, 3);
              p2_update(d0010, res_c, 2);
              p2_update(r0010, res_c, 3);
              p2_update(&d0010[-1], res_c, 3);
              res_m0010 = nres4;
            }
            // The one counter whose update straddles the join: both arms choose a
            // residual, and the bump happens once, after them.
            p2_update(m0010, res_m0010, 3);
          }
        }
      }
  }
}

uint32_t AltP2Block::alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in) {
  P2Ctx* cursor0;
  int32_t n2_half;
  // The frequency record whose bins this symbol's decision moves.  It was
  // `mir_top` again -- a `uint16_t*` over a `P2Freq`, where index 1 is `f[0]`
  // and `3-is_dec` is `f[2]` or `f[1]`.  Every one of its thirteen
  // assignments was a `P2Freq*` with the cast written on it, and every use was
  // one of those two bins, so there was never anything a word pointer was for.
  P2Freq* bin;
  P2Freq* grp;
  uint32_t rec_idx;
  int32_t hi_nibble;
  P2Freq* p2_rec;
  uint32_t pair_ctx;
  int32_t fold_sel2, is_dec;
  uint32_t idx0;
  float sample, d_bias2;
  P2Freq* frecg2, * frecg1, * frecg0, * frecg3, * gtop, * frec;
  uint32_t off0, off1, off2, off3, off4;
  NbRow* wrow_cur;
  NbRow* wrow_b;
  float ms1, bias2, bias1, d_bias, ms_a, ms_b, ms_b10, conf;
  int32_t ctx_lo, ctx_l, ctx15, fold_sel,
      sel_alt, sel0, sel1, sel2, sel3, sel4;
  int32_t dl, du, dul, dur, resid;
  P2Freq* frec_step, * frec4c;
  P2Freq* frec2, * frec3, * frec4b, * frec5, * frec6, * frec7, * prec0, * g0, * g1, * g2, * g3, * g4, * h4, * nxt4, * prec_m1, * h3, * nxt3, * h2, * nxt2, * h1, * nxt1, * h0, * nxt0;
  uint32_t step_v, step10, step13;
  ctx_lo = ctx&0xF;
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
  cursor0 = cursor[0];
  sample = (float)sample16;
  dl = sample16-cursor0[-1].val;
  cursor0->dleft = abs32(dl);
  du = sample16-cursor[1]->val;
  cursor[0]->dup = abs32(du);
  dul = sample16-cursor[1][-1].val;
  cursor[0]->dupleft = abs32(dul);
  dur = sample16-cursor[1][1].val;
  cursor[0]->dupright = abs32(dur);
  resid = (int16_t)(sample16-pred_prev);
  cursor[0]->err = resid;
  cursor[0]->aerr = abs32(resid);
  wrow_cur = nb_cur;
  ms1 = wrow_cur->w[14][1]+0.000099999997f;
  wrow_b = cur[-1];
  bias2 = bias[2];
  float n2_bias = bias[0];
  d_bias2 = sample-bias2;
  bias1 = bias[1];
  d_bias = bias1-bias2;
  ms_a = ((((sample-bias2)*(bias1-bias2))-wrow_cur->w[14][0])*0.001f)+wrow_cur->w[14][0];
  ms_b = ms1+(((d_bias*d_bias)-wrow_cur->w[14][1])*0.001f);
  wrow_cur->w[14][1] = ms_b;
  ms_b10 = 0.1f*ms_b;
  if( (0.1f*ms_b)<=ms_a )
    ms_b10 = fminf(ms_b, ms_a);
  wrow_cur->w[14][0] = ms_b10;
  conf = (1.0f-(ms_b10/(ms_b+576.0f)))*2.0f;
  nlms_track_two_rows(wrow_cur, wrow_b, (sample-bias1)*2.5999999f, d_bias2*conf, conf);
  nlms_predict_and_correct(wrow_cur, sample, n2_bias);
  *cur = nb_cur;
  ++cur;
  ++above;
  code_banks(sample16);
  ctx_l = ctx;
  ++cursor[0];
  frec = &freq[ctx_l];
  ++cursor[1];
  ++cursor[2];
  ++cursor[3];
  ++cursor[4];
  step_v = frec[0].step;
  if( step_v>0x10 ) {
    is_dec = a4&1;
    ctx15 = ctx_l&0xF;
    pair_ctx = ctx_pair[a4&1];
    if( ctx15<15 ) {
      bin = &frec[1];
      p2_rescale(&frec[1]);
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
        fold_sel2 = fold_sel;
        step_v = ctx_w[4].w[2-ctx_w[4].sel]+ctx_w[3].w[2-ctx_w[3].sel]+ctx_w[2].w[2-ctx_w[2].sel]+ctx_w[1].w[2-ctx_w[1].sel]+(ctx_w[0].w[1]+(ctx&0x3F));
        frec_step = &freq[step_v];
        p2_rec = frec_step;
        if( ctx15<15 ) {
          frec2 = &frec_step[1];
          bin = frec2;
          p2_freq_add(frec2, fold_sel2, 4);
          if( ctx15<=0 )
            goto counters_done;
        } else {
        }
        bin = &p2_rec[-1];
        p2_freq_add(&p2_rec[-1], fold_sel2, 4);
counters_done:
        if( a4 ) {
          n2_half = (a4-1)>>1;
          hi_nibble = (uint8_t)pair_ctx&0xF0;
          if( hi_nibble>=0xF0||(update_binary_pair(model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
            update_binary_pair(model_strip(pair_ctx-16), n2_half);
          }
        }
        prec0 = &p2_rec[0];
        p2_freq_add(&p2_rec[0], fold_sel2, 6);
        if( !plane_idx||freq[ctx].step>0x100u ) {
          sel_alt = 2-is_dec;
          if( !a4 )
            sel_alt = 0;
          sel0 = ctx_w[0].sel;
          is_dec = sel_alt;
          if( sel0==1 ) {
            h0 = &freq[ctx_w[0].w[0]+step_v-ctx_w[0].w[1]];
            p2_freq_add(h0, fold_sel2, 4);
            off0 = ctx-ctx_w[0].w[1];
            frecg0 = &freq[off0+ctx_w[0].w[0]];
            idx0 = ctx_w[0].w[2]+off0;
            grp = frecg0;
            frec3 = &frecg0[0];
            bin = frec3;
            p2_freq_add(frec3, is_dec, 3);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 4);
              nxt0 = &freq[idx0+1];
              p2_freq_add(&freq[idx0+1], is_dec, 2);
            }
            if( ctx15>2 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          } else {
            g0 = &freq[step_v+ctx_w[0].w[1]-ctx_w[0].w[2-sel0]];
            p2_freq_add(g0, fold_sel2, 7);
            grp = (&freq[ctx_w[0].w[1]+(ctx-ctx_w[0].w[ctx_w[0].sel])]);
            bin = &grp[0];
            p2_freq_add(&grp[0], is_dec, 7);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 5);
            }
            if( ctx15>0 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          }
          sel1 = ctx_w[1].sel;
          if( sel1==1 ) {
            frec4b = &freq[ctx_w[1].w[0]+step_v-ctx_w[1].w[1]];
            bin = frec4b;
            p2_freq_add(frec4b, fold_sel2, 4);
            off1 = ctx-ctx_w[1].w[1];
            frecg1 = &freq[off1+ctx_w[1].w[0]];
            idx0 = ctx_w[1].w[2]+off1;
            grp = frecg1;
            h1 = &frecg1[0];
            p2_freq_add(h1, is_dec, 3);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 4);
              nxt1 = &freq[idx0+1];
              p2_freq_add(&freq[idx0+1], is_dec, 2);
            }
            if( ctx15>2 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          } else {
            g1 = &freq[step_v+ctx_w[1].w[1]-ctx_w[1].w[2-sel1]];
            p2_freq_add(g1, fold_sel2, 7);
            grp = (&freq[ctx_w[1].w[1]+(ctx-ctx_w[1].w[ctx_w[1].sel])]);
            bin = &grp[0];
            p2_freq_add(&grp[0], is_dec, 7);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 5);
            }
            if( ctx15>0 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          }
          sel2 = ctx_w[2].sel;
          if( sel2==1 ) {
            frec5 = &freq[ctx_w[2].w[0]+step_v-ctx_w[2].w[1]];
            bin = frec5;
            p2_freq_add(frec5, fold_sel2, 4);
            off2 = ctx-ctx_w[2].w[1];
            frecg2 = &freq[off2+ctx_w[2].w[0]];
            idx0 = ctx_w[2].w[2]+off2;
            grp = frecg2;
            h2 = &frecg2[0];
            p2_freq_add(h2, is_dec, 3);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 4);
              nxt2 = &freq[idx0+1];
              p2_freq_add(&freq[idx0+1], is_dec, 2);
            }
            if( ctx15>2 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          } else {
            g2 = &freq[step_v+ctx_w[2].w[1]-ctx_w[2].w[2-sel2]];
            p2_freq_add(g2, fold_sel2, 7);
            grp = (&freq[ctx_w[2].w[1]+(ctx-ctx_w[2].w[ctx_w[2].sel])]);
            bin = &grp[0];
            p2_freq_add(&grp[0], is_dec, 7);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 5);
            }
            if( ctx15>0 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          }
          sel3 = ctx_w[3].sel;
          if( sel3==1 ) {
            frec6 = &freq[ctx_w[3].w[0]+step_v-ctx_w[3].w[1]];
            bin = frec6;
            p2_freq_add(frec6, fold_sel2, 4);
            off3 = ctx-ctx_w[3].w[1];
            frecg3 = &freq[off3+ctx_w[3].w[0]];
            idx0 = ctx_w[3].w[2]+off3;
            grp = frecg3;
            h3 = &frecg3[0];
            p2_freq_add(h3, is_dec, 3);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 4);
              nxt3 = &freq[idx0+1];
              p2_freq_add(&freq[idx0+1], is_dec, 2);
            }
            if( ctx15>2 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          } else {
            g3 = &freq[step_v+ctx_w[3].w[1]-ctx_w[3].w[2-sel3]];
            p2_freq_add(g3, fold_sel2, 7);
            grp = (&freq[ctx_w[3].w[1]+(ctx-ctx_w[3].w[ctx_w[3].sel])]);
            bin = &grp[0];
            p2_freq_add(&grp[0], is_dec, 7);
            if( ctx15<15 ) {
              bin = &grp[1];
              p2_freq_add(&grp[1], is_dec, 5);
            }
            if( ctx15>0 ) {
              bin = &grp[-1];
              p2_freq_add(&grp[-1], is_dec, 6);
            }
          }
          sel4 = ctx_w[4].sel;
          if( sel4==1 ) {
            frec7 = &freq[ctx_w[4].w[0]+step_v-ctx_w[4].w[1]];
            bin = frec7;
            p2_freq_add(frec7, fold_sel2, 4);
            off4 = ctx-ctx_w[4].w[1];
            frec4c = &freq[off4+ctx_w[4].w[0]];
            rec_idx = ctx_w[4].w[2]+off4;
            p2_rec = frec4c;
            h4 = &frec4c[0];
            p2_freq_add(h4, is_dec, 3);
            if( ctx15<15 ) {
              bin = &p2_rec[1];
              p2_freq_add(&p2_rec[1], is_dec, 4);
              nxt4 = &freq[rec_idx+1];
              p2_freq_add(nxt4, is_dec, 2);
            }
            if( ctx15>2 ) {
              prec_m1 = &p2_rec[-1];
              p2_freq_add(&p2_rec[-1], is_dec, 6);
            }
          } else {
            g4 = &freq[step_v+ctx_w[4].w[1]-ctx_w[4].w[2-sel4]];
            p2_freq_add(g4, fold_sel2, 7);
            gtop = (&freq[ctx_w[4].w[1]+(ctx-ctx_w[4].w[ctx_w[4].sel])]);
            p2_freq_add(&gtop[0], is_dec, 7);
            if( ctx15<15 ) {
              p2_freq_add(&gtop[1], is_dec, 5);
            }
            if( ctx15>0 ) {
              p2_freq_add(&gtop[-1], is_dec, 6);
            }
          }
        }
        return step_v;
      }
      frec = &freq[ctx];
    }
    bin = &frec[-1];
    p2_rescale(&frec[-1]);
    step13 = (13*(uint32_t)frec[-1].step)>>4;
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
