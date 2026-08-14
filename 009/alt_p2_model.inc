// alt_p2_model.inc -- the predictor-2 model itself, and the largest body in the program.
// "p2" is `plane_predictor == 2`; it is not about depth.  t8g is a single eight-bit
// plane and codes here.  MODELS.md section 7.

uint32_t AltP2Block::alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in) {
  P2Ctx* unused_c, * cursor0;
  int32_t n2_half;
  uint16_t* mir_top;
  P2Freq* grp;
  uint32_t rec_idx;
  int32_t hi_nibble;
  P2Freq* p2_rec;
  uint32_t pair_ctx;
  uint32_t step_s, bank_off2, ctxw_s;
  int32_t fold_sel2, is_dec;
  int32_t res_c;
  uint32_t idx0;
  float sample, d_bias2;
  uint8_t b_top3;
  P2Freq* frecg2, * frecg1, * frecg0, * frecg3, * gtop, * frec;
  uint32_t off0, off1, off2, off3, off4, ctxw;
  float (*wrow_cur)[4];
  float (*wrow_b)[4];
  bool go;
  float ms1, bias2, bias1, d_bias, ms_a, ms_b, ms_b10, conf;
  int32_t ctx_lo, bctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, x0020, x0010, neg,
      w_top, e_top, bump, res_s, w_top3, w_topm1, wm0400x, neg_b, res_t, res_m0010,
      neg_c, neg_d, wd4000p1a, pd4000p1a, wm0400z, wm0020a, pm0020a, ctx_l, ctx15, fold_sel,
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
  cursor[0]->sign = (resid_in<=(int32_t)(((uint32_t)(6-ctx_lo)>>31)+((uint32_t)(4-ctx_lo)>>31)+2*((uint32_t)(9-ctx_lo)>>31)))+(resid_in<(int32_t)-(((uint32_t)(6-ctx_lo)>>31)+((uint32_t)(4-ctx_lo)>>31)+2*((uint32_t)(9-ctx_lo)>>31)));
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
  ms1 = wrow_cur[14][1]+0.000099999997f;
  wrow_b = cur[-1];
  bias2 = bias[2];
  float n2_bias = bias[0];
  d_bias2 = sample-bias2;
  bias1 = bias[1];
  d_bias = bias1-bias2;
  ms_a = ((((sample-bias2)*(bias1-bias2))-wrow_cur[14][0])*0.001f)+wrow_cur[14][0];
  ms_b = ms1+(((d_bias*d_bias)-wrow_cur[14][1])*0.001f);
  wrow_cur[14][1] = ms_b;
  ms_b10 = 0.1f*ms_b;
  if( (0.1f*ms_b)<=ms_a )
    ms_b10 = fminf(ms_b, ms_a);
  wrow_cur[14][0] = ms_b10;
  conf = (1.0f-(ms_b10/(ms_b+576.0f)))*2.0f;
  uint32_t bank = 0;
  {
    const float err_a = (sample-bias1)*2.5999999f;
    const float err_b = d_bias2*conf;
    const float floor_a = 26896.0f*wrow_cur[14][2];
    const float floor_b = 5041.0f*wrow_b[14][2];
    const float ms_rate_b = 0.013f*conf;
    int32_t j, k;
    for( j = 0; j<7; ++j )
      for( k = 0; k<4; ++k ) {
        float x = p2_row[j][k];
        float ms;
        ms = wrow_cur[7+j][k]+(x*x-wrow_cur[7+j][k])*0.05f;
        wrow_cur[7+j][k] = ms;
        wrow_cur[j][k] += bmf_p2_rate[j][k]*err_a*x/(ms+floor_a);
        ms = wrow_b[7+j][k]+(x*x-wrow_b[7+j][k])*ms_rate_b;
        wrow_b[7+j][k] = ms;
        wrow_b[j][k] += bmf_p2_rate[j][k]*err_b*x/(ms+floor_b);
      }
  }
  {
    float acc[4], pred, err, ms_scale;
    int32_t j, k;
    for( k = 0; k<4; ++k ) {
      acc[k] = wrow_cur[0][k]*p2_row[0][k];
      for( j = 1; j<7; ++j )
        acc[k] += wrow_cur[j][k]*p2_row[j][k];
    }
    pred = n2_bias+bmf_hsum4(acc);
    err = sample-pred;
    ms_scale = wrow_cur[14][2];
    for( j = 0; j<7; ++j )
      for( k = 0; k<4; ++k )
        wrow_cur[j][k] += bmf_p2_rate[j][k]*err*p2_row[j][k]/(wrow_cur[7+j][k]+ms_scale*529.0f);
    ++*(int32_t*)&wrow_cur[15][0];
    wrow_cur[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }
  *cur = nb_cur;
  ++cur;
  ++above;
  do {
    bctx = (uint32_t)bank_ctx[bank];
    res = sample16-(nb_sum[2*bank]);
    uint32_t bank_off = bank<<17;
    P2Count* node0 = &p2_ctr[32768*bank+bctx];
    w0 = res+(uint16_t)node0->weighted;
    node0->weighted = w0;
    int32_t countdown = node0->b1;
    if( countdown ) {
      ctxw_s = bctx;
      w0b = w0+4*((res>deadzone_hi)-(res<deadzone_lo));
      node0->weighted = w0b;
      ctxw = ctxw_s;
      if( (int32_t)abs32(res)<38 ) {
        if( (uint8_t)-- countdown ) {
          node0->b1 = countdown;
        } else {
          if( *(uint8_t*)&node0->rate<8u ) {
            uint8_t b0n = node0->rate+1;
            node0->rate = b0n;
            node0->b1 = *((uint8_t*)&p2_float_pool+b0n+3);
            node0->weighted = 2*w0b;
          } else {
            node0->b1 = countdown;
          }
        }
      }
      if( !alphabet_reduced ) {
        __builtin_prefetch(&mir_top, 0, 1);
        mir_top = (uint16_t*)((uint8_t*)&p2_ctr[(ctxw^0x7FF0)]+bank_off);
        res2 = (nb_sum[2*bank+1])+res;
        int32_t lowbits = ctxw&3;
        if( (uint32_t)lowbits>=3||(bank_off2 = bank_off, ctxw_s = ctxw, w1 = node0[1].weighted, e1 = res2-p2_pred(w1, *(uint8_t*)&node0[1].rate), go = lowbits<=0, *(uint16_t*)&node0[1].weighted = w1+((32*((e1>deadzone_hi)-(uint32_t)(e1<deadzone_lo))+e1+1)>>1), ctxw = ctxw_s, !go) ) {
          bank_off2 = bank_off;
          ctxw_s = ctxw;
          wnode0m1a = node0[-1].weighted;
          enode0m1a = res2-p2_pred(wnode0m1a, *(uint8_t*)&node0[-1].rate);
          *(uint16_t*)&node0[-1].weighted = wnode0m1a+((32*((enode0m1a>deadzone_hi)-(uint32_t)(enode0m1a<deadzone_lo))+enode0m1a+2)>>2);
          ctxw = ctxw_s;
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
        uint16_t* mir_top2 = mir_top;
        neg = -res_c;
        __builtin_prefetch(d0040, 0, 1);
        w_top = (int16_t)mir_top2[1];
        __builtin_prefetch(m0040, 0, 1);
        e_top = neg-p2_pred(w_top, (uint8_t)mir_top[0]);
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
        // (w_new>>2)` -- kept a high half that the next line throws away:
        // `mir_top` is `uint16_t *`.  Sixteen bits in and sixteen bits out.
        mir_top[1] = w_top+(w_new>>2);
        __builtin_prefetch(((P2Count*)(r0020)), 0, 1);
        __builtin_prefetch(d0010, 0, 1);
        __builtin_prefetch(m0010, 0, 1);
        __builtin_prefetch(r0010, 0, 1);
        if( go&&(w_top3 = (int16_t)mir_top[3], b_top3 = (uint8_t)mir_top[2], res_c = res_s, go = lowbits<=0, mir_top[3] = ((uint32_t)(-res_s-p2_pred(w_top3, b_top3)+2)>>2)+w_top3, go) ) {
          res_c = res_s;
          p2_update(d4000, res_s, 2);
          p2_update(r4000, res_s, 3);
          wd4000p1a = d4000[1].weighted;
          pd4000p1a = p2_pred(wd4000p1a, *(uint8_t*)&d4000[1].rate);
          int32_t nres3 = -res_s;
          *(uint16_t*)&d4000[1].weighted = ((uint32_t)(res_s-pd4000p1a+2)>>2)+wd4000p1a;
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
          w_topm1 = (int16_t)mir_top[-1];
          res_c = res_s;
          mir_top[-1] = ((uint32_t)(-res_s-p2_pred(w_topm1, ((uint8_t)mir_top[-2]))+4)>>3)+w_topm1;
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
    ++bank;
  } while( bank<5 );
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
      mir_top = (uint16_t*)&frec[1];
      if( frec[1].f[2]+frec[1].f[1]+frec[1].f[0]>29696 )
        frec[1].rescale_three_way();
      step10 = (10*(uint32_t)frec[1].step)>>4;
      if( a4 ) {
        mir_top[3-is_dec] += step10;
        __update_binary_pair(model_strip(pair_ctx+1), (a4-1)>>1);
      } else {
        mir_top[1] += step10;
      }
      if( ctx15<=0 ) {
LABEL_37:
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
          mir_top = (uint16_t*)frec2;
          if( frec2->f[2]+frec_step[1].f[1]+frec_step[1].f[0]>29696 ) {
            step_s = step_v;
            frec2->rescale_three_way();
            step_v = step_s;
          }
          mir_top[fold_sel2+1] += (p2_rec[1].step&0xFFFC)>>2;
          if( ctx15<=0 )
            goto LABEL_48;
        } else {
        }
        mir_top = (uint16_t*)&p2_rec[-1];
        if( p2_rec[-1].f[2]+p2_rec[-1].f[1]+p2_rec[-1].f[0]>29696 ) {
          step_s = step_v;
          p2_rec[-1].rescale_three_way();
          step_v = step_s;
        }
        mir_top[fold_sel2+1] += (uint16_t)(p2_rec[-1].step&0xFFFC)>>2;
LABEL_48:
        if( a4 ) {
          n2_half = (a4-1)>>1;
          hi_nibble = (uint8_t)pair_ctx&0xF0;
          if( hi_nibble>=0xF0||(step_s = step_v, __update_binary_pair(model_strip(pair_ctx+16), n2_half), step_v = step_s, hi_nibble>0) ) {
            step_s = step_v;
            __update_binary_pair(model_strip(pair_ctx-16), n2_half);
            step_v = step_s;
          }
        }
        prec0 = &p2_rec[0];
        if( p2_rec[0].f[2]+p2_rec[0].f[1]+p2_rec[0].f[0]>29696 ) {
          step_s = step_v;
          p2_rec[0].rescale_three_way();
          step_v = step_s;
        }
        prec0->f[fold_sel2] += (6*(uint32_t)p2_rec[0].step)>>4;
        if( !plane_idx||freq[ctx].step>0x100u ) {
          sel_alt = 2-is_dec;
          if( !a4 )
            sel_alt = 0;
          sel0 = ctx_w[0].sel;
          is_dec = sel_alt;
          if( sel0==1 ) {
            h0 = &freq[ctx_w[0].w[0]+step_v-ctx_w[0].w[1]];
            if( h0->f[2]+(freq[ctx_w[0].w[0]+step_v-ctx_w[0].w[1]].f[1])+(freq[ctx_w[0].w[0]+step_v-ctx_w[0].w[1]].f[0])>29696 ) {
              step_s = step_v;
              h0->rescale_three_way();
              step_v = step_s;
            }
            h0->f[fold_sel2] += (uint16_t)(h0->step&0xFFFC)>>2;
            off0 = ctx-ctx_w[0].w[1];
            frecg0 = &freq[off0+ctx_w[0].w[0]];
            idx0 = ctx_w[0].w[2]+off0;
            grp = frecg0;
            frec3 = &frecg0[0];
            mir_top = (uint16_t*)frec3;
            if( frec3->f[2]+(frec3->f[1]+frec3->f[0])>29696 ) {
              step_s = step_v;
              frec3->rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (3*grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt0 = &freq[idx0+1];
              if( freq[idx0+1].f[2]+freq[idx0+1].f[1]+freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt0->f[is_dec] += (nxt0->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g0 = &freq[step_v+ctx_w[0].w[1]-ctx_w[0].w[2-sel0]];
            if( freq[step_v+ctx_w[0].w[1]-ctx_w[0].w[2-sel0]].f[2]+freq[step_v+ctx_w[0].w[1]-ctx_w[0].w[2-sel0]].f[1]+freq[step_v+ctx_w[0].w[1]-ctx_w[0].w[2-sel0]].f[0]>29696 ) {
              step_s = step_v;
              g0->rescale_three_way();
              step_v = step_s;
            }
            g0->f[fold_sel2] += (7*(uint32_t)g0->step)>>4;
            grp = (&freq[ctx_w[0].w[1]+(ctx-ctx_w[0].w[ctx_w[0].sel])]);
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              step_s = step_v;
              mir_top[is_dec+1] = mir_top[is_dec+1]+((5*(uint32_t)grp[1].step)>>4);
              step_v = step_s;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel1 = ctx_w[1].sel;
          if( sel1==1 ) {
            frec4b = &freq[ctx_w[1].w[0]+step_v-ctx_w[1].w[1]];
            mir_top = (uint16_t*)frec4b;
            if( frec4b->f[2]+(freq[ctx_w[1].w[0]+step_v-ctx_w[1].w[1]].f[1])+(freq[ctx_w[1].w[0]+step_v-ctx_w[1].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec4b->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off1 = ctx-ctx_w[1].w[1];
            frecg1 = &freq[off1+ctx_w[1].w[0]];
            idx0 = ctx_w[1].w[2]+off1;
            grp = frecg1;
            h1 = &frecg1[0];
            if( h1->f[2]+h1->f[1]+h1->f[0]>29696 ) {
              step_s = step_v;
              h1->rescale_three_way();
              step_v = step_s;
            }
            h1->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt1 = &freq[idx0+1];
              if( freq[idx0+1].f[2]+freq[idx0+1].f[1]+freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt1->f[is_dec] += (nxt1->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g1 = &freq[step_v+ctx_w[1].w[1]-ctx_w[1].w[2-sel1]];
            if( freq[step_v+ctx_w[1].w[1]-ctx_w[1].w[2-sel1]].f[2]+freq[step_v+ctx_w[1].w[1]-ctx_w[1].w[2-sel1]].f[1]+freq[step_v+ctx_w[1].w[1]-ctx_w[1].w[2-sel1]].f[0]>29696 ) {
              step_s = step_v;
              g1->rescale_three_way();
              step_v = step_s;
            }
            g1->f[fold_sel2] += (7*(uint32_t)g1->step)>>4;
            grp = (&freq[ctx_w[1].w[1]+(ctx-ctx_w[1].w[ctx_w[1].sel])]);
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel2 = ctx_w[2].sel;
          if( sel2==1 ) {
            frec5 = &freq[ctx_w[2].w[0]+step_v-ctx_w[2].w[1]];
            mir_top = (uint16_t*)frec5;
            if( frec5->f[2]+(freq[ctx_w[2].w[0]+step_v-ctx_w[2].w[1]].f[1])+(freq[ctx_w[2].w[0]+step_v-ctx_w[2].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec5->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off2 = ctx-ctx_w[2].w[1];
            frecg2 = &freq[off2+ctx_w[2].w[0]];
            idx0 = ctx_w[2].w[2]+off2;
            grp = frecg2;
            h2 = &frecg2[0];
            if( h2->f[2]+h2->f[1]+h2->f[0]>29696 ) {
              step_s = step_v;
              h2->rescale_three_way();
              step_v = step_s;
            }
            h2->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt2 = &freq[idx0+1];
              if( freq[idx0+1].f[2]+freq[idx0+1].f[1]+freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt2->f[is_dec] += (nxt2->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g2 = &freq[step_v+ctx_w[2].w[1]-ctx_w[2].w[2-sel2]];
            if( freq[step_v+ctx_w[2].w[1]-ctx_w[2].w[2-sel2]].f[2]+freq[step_v+ctx_w[2].w[1]-ctx_w[2].w[2-sel2]].f[1]+freq[step_v+ctx_w[2].w[1]-ctx_w[2].w[2-sel2]].f[0]>29696 ) {
              step_s = step_v;
              g2->rescale_three_way();
              step_v = step_s;
            }
            g2->f[fold_sel2] += (7*(uint32_t)g2->step)>>4;
            grp = (&freq[ctx_w[2].w[1]+(ctx-ctx_w[2].w[ctx_w[2].sel])]);
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel3 = ctx_w[3].sel;
          if( sel3==1 ) {
            frec6 = &freq[ctx_w[3].w[0]+step_v-ctx_w[3].w[1]];
            mir_top = (uint16_t*)frec6;
            if( frec6->f[2]+(freq[ctx_w[3].w[0]+step_v-ctx_w[3].w[1]].f[1])+(freq[ctx_w[3].w[0]+step_v-ctx_w[3].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec6->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off3 = ctx-ctx_w[3].w[1];
            frecg3 = &freq[off3+ctx_w[3].w[0]];
            idx0 = ctx_w[3].w[2]+off3;
            grp = frecg3;
            h3 = &frecg3[0];
            if( h3->f[2]+h3->f[1]+h3->f[0]>29696 ) {
              step_s = step_v;
              h3->rescale_three_way();
              step_v = step_s;
            }
            h3->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt3 = &freq[idx0+1];
              if( freq[idx0+1].f[2]+freq[idx0+1].f[1]+freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt3->f[is_dec] += (nxt3->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g3 = &freq[step_v+ctx_w[3].w[1]-ctx_w[3].w[2-sel3]];
            if( freq[step_v+ctx_w[3].w[1]-ctx_w[3].w[2-sel3]].f[2]+freq[step_v+ctx_w[3].w[1]-ctx_w[3].w[2-sel3]].f[1]+freq[step_v+ctx_w[3].w[1]-ctx_w[3].w[2-sel3]].f[0]>29696 ) {
              step_s = step_v;
              g3->rescale_three_way();
              step_v = step_s;
            }
            g3->f[fold_sel2] += (7*(uint32_t)g3->step)>>4;
            grp = (&freq[ctx_w[3].w[1]+(ctx-ctx_w[3].w[ctx_w[3].sel])]);
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel4 = ctx_w[4].sel;
          if( sel4==1 ) {
            frec7 = &freq[ctx_w[4].w[0]+step_v-ctx_w[4].w[1]];
            mir_top = (uint16_t*)frec7;
            if( frec7->f[2]+(freq[ctx_w[4].w[0]+step_v-ctx_w[4].w[1]].f[1])+(freq[ctx_w[4].w[0]+step_v-ctx_w[4].w[1]].f[0])>29696 )
              frec7->rescale_three_way();
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off4 = ctx-ctx_w[4].w[1];
            frec4c = &freq[off4+ctx_w[4].w[0]];
            rec_idx = ctx_w[4].w[2]+off4;
            p2_rec = frec4c;
            h4 = &frec4c[0];
            if( frec4c[0].f[2]+frec4c[0].f[1]+frec4c[0].f[0]>29696 )
              h4->rescale_three_way();
            h4->f[is_dec] += (3*(uint32_t)p2_rec[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&p2_rec[1];
              if( p2_rec[1].f[2]+(p2_rec[1].f[1]+p2_rec[1].f[0])>29696 )
                p2_rec[1].rescale_three_way();
              mir_top[is_dec+1] += (uint16_t)(p2_rec[1].step&0xFFFC)>>2;
              nxt4 = &freq[rec_idx+1];
              if( nxt4->f[2]+nxt4->f[1]+nxt4->f[0]>29696 )
                nxt4->rescale_three_way();
              step_v = (nxt4->step&0xFFF8)>>3;
              nxt4->f[is_dec] += step_v;
            }
            if( ctx15>2 ) {
              prec_m1 = &p2_rec[-1];
              if( p2_rec[-1].f[2]+p2_rec[-1].f[1]+p2_rec[-1].f[0]>29696 )
                p2_rec[-1].rescale_three_way();
              prec_m1->f[is_dec] += (6*(uint32_t)p2_rec[-1].step)>>4;
            }
          } else {
            g4 = &freq[step_v+ctx_w[4].w[1]-ctx_w[4].w[2-sel4]];
            if( freq[step_v+ctx_w[4].w[1]-ctx_w[4].w[2-sel4]].f[2]+freq[step_v+ctx_w[4].w[1]-ctx_w[4].w[2-sel4]].f[1]+freq[step_v+ctx_w[4].w[1]-ctx_w[4].w[2-sel4]].f[0]>29696 )
              g4->rescale_three_way();
            g4->f[fold_sel2] += (7*(uint32_t)g4->step)>>4;
            gtop = (&freq[ctx_w[4].w[1]+(ctx-ctx_w[4].w[ctx_w[4].sel])]);
            if( gtop[0].f[2]+gtop[0].f[1]+gtop[0].f[0]>29696 )
              gtop[0].rescale_three_way();
            gtop[0].f[is_dec] += (7*(uint32_t)gtop[0].step)>>4;
            if( ctx15<15 ) {
              if( gtop[1].f[2]+gtop[1].f[1]+gtop[1].f[0]>29696 )
                gtop[1].rescale_three_way();
              step_v = gtop[1].step;
              gtop[1].f[is_dec] += (5*step_v)>>4;
            }
            if( ctx15>0 ) {
              if( gtop[-1].f[2]+gtop[-1].f[1]+gtop[-1].f[0]>29696 )
                gtop[-1].rescale_three_way();
              step_v = gtop[-1].step;
              gtop[-1].f[is_dec] += (6*step_v)>>4;
            }
          }
        }
        return step_v;
      }
      frec = &freq[ctx];
    }
    mir_top = (uint16_t*)&frec[-1];
    if( frec[-1].f[2]+frec[-1].f[1]+frec[-1].f[0]>29696 )
      frec[-1].rescale_three_way();
    step13 = (13*(uint32_t)frec[-1].step)>>4;
    if( a4 ) {
      mir_top[3-is_dec] += step13;
      __update_binary_pair(model_strip(pair_ctx-1), (a4-1)>>1);
    } else {
      mir_top[1] += step13;
    }
    goto LABEL_37;
  }
  return step_v;
}
