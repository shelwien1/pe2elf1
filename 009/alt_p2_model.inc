// alt_p2_model.inc -- the deep-plane model itself, and the largest body in the program.

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
  P2Count* m0080, * r0400, * r0800, * m0800, * d1000, * r1000, * m1000, * d2000, * r2000, * m2000, * d4000, * r4000, * m4000;
  int32_t lowbits, nres1, nres2, nres3, res_c;
  P2Count* d0800, * d0400, * m0400, * d0200, * m0200, * d0100, * m0100, * r0100, * d0080, * r0080, * d0040, * m0040, * d0020, * m0020, * r0020, * d0010, * m0010;
  int32_t nres4, nres5;
  uint32_t idx0;
  float sample, d_bias2;
  uint8_t b_top3, b0n;
  P2Freq* frecg2, * frecg1, * frecg0, * frecg3, * gtop, * frec;
  P2Count* r0200, * r0040, * r0010;
  uint32_t off0, off1, off2, off3, off4, ctxw;
  float (*wrow_cur)[4];
  float (*wrow_b)[4];
  bool go;
  float ms1, bias2, bias1, d_bias, ms_a, ms_b, ms_b10, conf;
  int32_t ctx_lo, bctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, x0020, x0010, neg, w_top, e_top, bump, res_s, w_top3, w_topm1, wd4000a, ed4000a, wr4000b, er4000b, wd4000m1b, ed4000m1b, wm4000c, em4000c, wd2000c, ed2000c, wr2000c, er2000c, wd2000m1b, ed2000m1b, wm2000c, em2000c, wd1000c, ed1000c, wr1000c, er1000c, wd1000m1b, ed1000m1b, wm1000c, em1000c, wd0800c, ed0800c, wr0800c, er0800c, wd0800m1b, ed0800m1b, wm0800c, em0800c, wd0400c, ed0400c, wr0400c, er0400c, wd0400m1b, ed0400m1b, wm0400x, wd0200c, ed0200c, wr0200c, er0200c, wd0200m1b, ed0200m1b, neg_b, wm0200c, em0200c, wd0100c, ed0100c, wr0100c, er0100c, wd0100m1b, ed0100m1b, wm0100c, em0100c, wd0080c, ed0080c, wr0080c, er0080c, wd0080m1b, ed0080m1b, wm0080c, em0080c, wd0040c, ed0040c, wr0040c, er0040c, wd0040m1b, ed0040m1b, wm0040c, em0040c, wd0020c, res_t, ed0020c, wr0020c, er0020c, wd0020m1b, ed0020m1b, wm0020c, em0020c, wd0010c, ed0010c, wr0010c, er0010c, wd0010m1b, ed0010m1b, wm0010b, em0010b, wd4000m1a,
          ed4000m1a, neg_c, wm4000b, em4000b, wd2000b, ed2000b, wr2000b, er2000b, wd2000m1a, ed2000m1a, wm2000b, em2000b, wd1000b, ed1000b, wr1000b, er1000b, wd1000m1a, ed1000m1a, wm1000b, em1000b, wd0800b, ed0800b, wr0800b, er0800b, wd0800m1a, ed0800m1a, wm0800b, em0800b, wd0400b, ed0400b, wr0400b, er0400b, wd0400m1a, ed0400m1a, wm0400y, nb_slot, wd0200b, ed0200b, wr0200b, er0200b, wd0200m1a, ed0200m1a, wm0200b, em0200b, wd0100b, ed0100b, wr0100b, er0100b, wd0100m1a, ed0100m1a, wm0100b, em0100b, wd0080b, ed0080b, wr0080b, er0080b, wd0080m1a, ed0080m1a, wm0080b, em0080b, wd0040b, ed0040b, wr0040b, er0040b, wd0040m1a, ed0040m1a, neg_d, wm0040b, em0040b, wd0020b, ed0020b, wr0020b, er0020b, wd0020m1a, ed0020m1a, wm0020b, em0020b, wd0010b, ed0010b, wr0010b, er0010b, wd0010m1a, ed0010m1a, wd4000b, ed4000b, wr4000a, er4000a, wd4000p1a, pd4000p1a, wm4000a, em4000a, wd2000a, ed2000a, wr2000a, er2000a, wm2000a, em2000a, wd1000a, ed1000a, wr1000a, er1000a, wm1000a, em1000a, wd0800a, ed0800a,
          wr0800a, er0800a, wm0800a, em0800a, wd0400a, ed0400a, wr0400a, er0400a, wm0400z, wd0200a, ed0200a, wr0200a, er0200a, wm0200a, em0200a, wd0100a, ed0100a, wr0100a, er0100a, wm0100a, em0100a, wd0080a, ed0080a, wr0080a, er0080a, wm0080a, em0080a, wd0040a, ed0040a, wr0040a, er0040a, wm0040a, em0040a, wd0020a, ed0020a, wr0020a, er0020a, wm0020a, pm0020a, wd0010a, ed0010a, wr0010a, er0010a, wm0010a, em0010a, ctx_l, ctx15, fold_sel, sel_alt, sel0, sel1, sel2, sel3, sel4;
  int32_t dl, du, dul, dur, resid;
  P2Freq* frec_step, * frec4c;
  P2Freq* frec2, * frec3, * frec4b, * frec5, * frec6, * frec7, * prec0, * g0, * g1, * g2, * g3, * g4, * h4, * nxt4, * prec_m1, * h3, * nxt3, * h2, * nxt2, * h1, * nxt1, * h0, * nxt0;
  uint32_t bank_off, ri0100, ri0010, w_new, step_v, step10, step13;
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
    bank_off = bank<<17;
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
            b0n = node0->rate+1;
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
        lowbits = ctxw&3;
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
        d4000 = &bankp[(ctxw^0x4000)];
        m4000 = &bankp[(ctxw^0x3FF0)];
        __builtin_prefetch(m4000, 0, 1);
        r4000 = &bankp[p2_ctx_rotate[((ctxw^0x4000)>>2)&3]+((ctxw^0x4000)&0xFFFFFFF3)];
        __builtin_prefetch(r4000, 0, 1);
        __builtin_prefetch(&bankp[(ctxw^0x2000)], 0, 1);
        d2000 = &bankp[(ctxw^0x2000)];
        __builtin_prefetch(&bankp[(ctxw^0x5FF0)], 0, 1);
        m2000 = &bankp[(ctxw^0x5FF0)];
        r2000 = &bankp[p2_ctx_rotate[((ctxw^0x2000)>>2)&3]+((ctxw^0x2000)&0xFFFFFFF3)];
        __builtin_prefetch(r2000, 0, 1);
        __builtin_prefetch(&bankp[(ctxw^0x1000)], 0, 1);
        d1000 = &bankp[(ctxw^0x1000)];
        __builtin_prefetch(&bankp[(ctxw^0x6FF0)], 0, 1);
        m1000 = &bankp[(ctxw^0x6FF0)];
        r1000 = &bankp[p2_ctx_rotate[((ctxw^0x1000)>>2)&3]+((ctxw^0x1000)&0xFFFFFFF3)];
        __builtin_prefetch(r1000, 0, 1);
        d0800 = &bankp[(ctxw^0x800)];
        m0800 = &bankp[(ctxw^0x77F0)];
        r0800 = &bankp[p2_ctx_rotate[((ctxw^0x800)>>2)&3]+((ctxw^0x800)&0xFFFFFFF3)];
        d0400 = &bankp[(ctxw^0x400)];
        __builtin_prefetch(d0800, 0, 1);
        __builtin_prefetch(m0800, 0, 1);
        __builtin_prefetch(r0800, 0, 1);
        m0400 = &bankp[(ctxw^0x7BF0)];
        r0400 = &bankp[p2_ctx_rotate[((ctxw^0x400)>>2)&3]+((ctxw^0x400)&0xFFFFFFF3)];
        d0200 = &bankp[(ctxw^0x200)];
        __builtin_prefetch(d0400, 0, 1);
        m0200 = &bankp[(ctxw^0x7DF0)];
        r0200 = &bankp[p2_ctx_rotate[((ctxw^0x200)>>2)&3]+((ctxw^0x200)&0xFFFFFFF3)];
        __builtin_prefetch(m0400, 0, 1);
        __builtin_prefetch(r0400, 0, 1);
        d0100 = &bankp[(ctxw^0x100)];
        ri0100 = p2_ctx_rotate[((ctxw^0x100)>>2)&3]+((ctxw^0x100)&0xFFFFFFF3);
        m0100 = &bankp[(ctxw^0x7EF0)];
        __builtin_prefetch(d0200, 0, 1);
        r0100 = &bankp[ri0100];
        d0080 = &bankp[(ctxw^0x80)];
        __builtin_prefetch(m0200, 0, 1);
        __builtin_prefetch(r0200, 0, 1);
        m0080 = &bankp[(ctxw^0x7F70)];
        r0080 = &bankp[p2_ctx_rotate[((ctxw^0x80)>>2)&3]+((ctxw^0x80)&0xFFFFFFF3)];
        d0040 = &bankp[(ctxw^0x40)];
        __builtin_prefetch(d0100, 0, 1);
        m0040 = &bankp[(ctxw^0x7FB0)];
        r0040 = &bankp[p2_ctx_rotate[((ctxw^0x40)>>2)&3]+((ctxw^0x40)&0xFFFFFFF3)];
        __builtin_prefetch(m0100, 0, 1);
        __builtin_prefetch(r0100, 0, 1);
        x0020 = ctxw^0x20;
        x0010 = ctxw^0x10;
        d0020 = &bankp[x0020];
        m0020 = &bankp[(x0020^0x7FF0)];
        __builtin_prefetch(d0080, 0, 1);
        __builtin_prefetch(m0080, 0, 1);
        r0020 = &bankp[p2_ctx_rotate[(x0020>>2)&3]+(x0020&0xFFFFFFF3)];
        d0010 = &bankp[x0010];
        ri0010 = p2_ctx_rotate[(x0010>>2)&3]+(x0010&0xFFFFFFF3);
        m0010 = &bankp[(x0010^0x7FF0)];
        __builtin_prefetch(r0080, 0, 1);
        r0010 = &bankp[ri0010];
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
        w_new = bump+e_top+2;
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
          wd4000b = d4000->weighted;
          res_c = res_s;
          ed4000b = res_s-p2_pred(wd4000b, (d4000->rate));
          d4000->weighted = p2_bump(wd4000b, ed4000b, 2);
          wr4000a = r4000->weighted;
          er4000a = res_s-p2_pred(wr4000a, r4000->rate);
          r4000->weighted = p2_bump(wr4000a, er4000a, 3);
          wd4000p1a = d4000[1].weighted;
          pd4000p1a = p2_pred(wd4000p1a, *(uint8_t*)&d4000[1].rate);
          nres3 = -res_s;
          *(uint16_t*)&d4000[1].weighted = ((uint32_t)(res_s-pd4000p1a+2)>>2)+wd4000p1a;
          wm4000a = m4000->weighted;
          em4000a = -res_s-p2_pred(wm4000a, m4000->rate);
          m4000->weighted = p2_bump(wm4000a, em4000a, 3);
          wd2000a = d2000->weighted;
          ed2000a = res_c-p2_pred(wd2000a, d2000->rate);
          d2000->weighted = p2_bump(wd2000a, ed2000a, 2);
          wr2000a = r2000->weighted;
          er2000a = res_c-p2_pred(wr2000a, r2000->rate);
          r2000->weighted = p2_bump(wr2000a, er2000a, 3);
          *(uint16_t*)&d2000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d2000[1].weighted, *(uint8_t*)&d2000[1].rate)+2)>>2;
          wm2000a = m2000->weighted;
          em2000a = nres3-p2_pred(wm2000a, m2000->rate);
          m2000->weighted = p2_bump(wm2000a, em2000a, 3);
          wd1000a = d1000->weighted;
          ed1000a = res_c-p2_pred(wd1000a, d1000->rate);
          d1000->weighted = p2_bump(wd1000a, ed1000a, 2);
          wr1000a = r1000->weighted;
          er1000a = res_c-p2_pred(wr1000a, r1000->rate);
          r1000->weighted = p2_bump(wr1000a, er1000a, 3);
          *(uint16_t*)&d1000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d1000[1].weighted, *(uint8_t*)&d1000[1].rate)+2)>>2;
          wm1000a = m1000->weighted;
          em1000a = nres3-p2_pred(wm1000a, m1000->rate);
          m1000->weighted = p2_bump(wm1000a, em1000a, 3);
          wd0800a = d0800->weighted;
          ed0800a = res_c-p2_pred(wd0800a, d0800->rate);
          d0800->weighted = p2_bump(wd0800a, ed0800a, 2);
          wr0800a = r0800->weighted;
          er0800a = res_c-p2_pred(wr0800a, r0800->rate);
          r0800->weighted = p2_bump(wr0800a, er0800a, 3);
          *(uint16_t*)&d0800[1].weighted += (uint32_t)(res_c-p2_pred(d0800[1].weighted, d0800[1].rate)+2)>>2;
          wm0800a = m0800->weighted;
          em0800a = nres3-p2_pred(wm0800a, m0800->rate);
          m0800->weighted = p2_bump(wm0800a, em0800a, 3);
          wd0400a = d0400->weighted;
          ed0400a = res_c-p2_pred(wd0400a, d0400->rate);
          d0400->weighted = p2_bump(wd0400a, ed0400a, 2);
          wr0400a = r0400->weighted;
          er0400a = res_c-p2_pred(wr0400a, r0400->rate);
          r0400->weighted = p2_bump(wr0400a, er0400a, 3);
          *(uint16_t*)&d0400[1].weighted += (uint32_t)(res_c-p2_pred(d0400[1].weighted, d0400[1].rate)+2)>>2;
          wm0400z = m0400->weighted;
          nres3 -= p2_pred(wm0400z, m0400->rate);
          m0400->weighted = p2_bump(wm0400z, nres3, 3);
          wd0200a = d0200->weighted;
          ed0200a = res_c-p2_pred(wd0200a, d0200->rate);
          d0200->weighted = p2_bump(wd0200a, ed0200a, 2);
          wr0200a = r0200->weighted;
          er0200a = res_c-p2_pred(wr0200a, r0200->rate);
          r0200->weighted = p2_bump(wr0200a, er0200a, 3);
          *(uint16_t*)&d0200[1].weighted += (uint32_t)(res_c-p2_pred(d0200[1].weighted, d0200[1].rate)+2)>>2;
          wm0200a = m0200->weighted;
          nres2 = -res_c;
          em0200a = -res_c-p2_pred(wm0200a, m0200->rate);
          m0200->weighted = p2_bump(wm0200a, em0200a, 3);
          wd0100a = d0100->weighted;
          ed0100a = res_c-p2_pred(wd0100a, d0100->rate);
          d0100->weighted = p2_bump(wd0100a, ed0100a, 2);
          wr0100a = r0100->weighted;
          er0100a = res_c-p2_pred(wr0100a, r0100->rate);
          r0100->weighted = p2_bump(wr0100a, er0100a, 3);
          *(uint16_t*)&d0100[1].weighted += (uint32_t)(res_c-p2_pred(d0100[1].weighted, d0100[1].rate)+2)>>2;
          wm0100a = m0100->weighted;
          em0100a = nres2-p2_pred(wm0100a, m0100->rate);
          m0100->weighted = p2_bump(wm0100a, em0100a, 3);
          wd0080a = d0080->weighted;
          ed0080a = res_c-p2_pred(wd0080a, d0080->rate);
          d0080->weighted = p2_bump(wd0080a, ed0080a, 2);
          wr0080a = r0080->weighted;
          er0080a = res_c-p2_pred(wr0080a, r0080->rate);
          r0080->weighted = p2_bump(wr0080a, er0080a, 3);
          *(uint16_t*)&d0080[1].weighted += (uint32_t)(res_c-p2_pred(d0080[1].weighted, d0080[1].rate)+2)>>2;
          wm0080a = m0080->weighted;
          em0080a = nres2-p2_pred(wm0080a, m0080->rate);
          m0080->weighted = p2_bump(wm0080a, em0080a, 3);
          wd0040a = d0040->weighted;
          ed0040a = res_c-p2_pred(wd0040a, d0040->rate);
          d0040->weighted = p2_bump(wd0040a, ed0040a, 2);
          wr0040a = r0040->weighted;
          er0040a = res_c-p2_pred(wr0040a, r0040->rate);
          r0040->weighted = p2_bump(wr0040a, er0040a, 3);
          *(uint16_t*)&d0040[1].weighted += (uint32_t)(res_c-p2_pred(d0040[1].weighted, d0040[1].rate)+2)>>2;
          wm0040a = m0040->weighted;
          em0040a = nres2-p2_pred(wm0040a, m0040->rate);
          m0040->weighted = p2_bump(wm0040a, em0040a, 3);
          wd0020a = d0020->weighted;
          ed0020a = res_c-p2_pred(wd0020a, d0020->rate);
          d0020->weighted = p2_bump(wd0020a, ed0020a, 2);
          wr0020a = r0020->weighted;
          er0020a = res_c-p2_pred(wr0020a, r0020->rate);
          r0020->weighted = p2_bump(wr0020a, er0020a, 3);
          *(uint16_t*)&d0020[1].weighted += (uint32_t)(res_c-p2_pred(d0020[1].weighted, d0020[1].rate)+2)>>2;
          wm0020a = m0020->weighted;
          pm0020a = p2_pred(wm0020a, m0020->rate);
          m0020->weighted = wm0020a+((32*((nres2-pm0020a>deadzone_hi)-(uint32_t)(nres2-pm0020a<deadzone_lo))+nres2-pm0020a+4)>>3);
          wd0010a = d0010->weighted;
          ed0010a = res_c-p2_pred(wd0010a, d0010->rate);
          d0010->weighted = p2_bump(wd0010a, ed0010a, 2);
          wr0010a = r0010->weighted;
          er0010a = res_c-p2_pred(wr0010a, r0010->rate);
          r0010->weighted = p2_bump(wr0010a, er0010a, 3);
          *(uint16_t*)&d0010[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d0010[1].weighted, *(uint8_t*)&d0010[1].rate)+2)>>2;
          wm0010a = m0010->weighted;
          em0010a = -res_c-p2_pred(wm0010a, m0010->rate);
          m0010->weighted = p2_bump(wm0010a, em0010a, 3);
        } else {
          w_topm1 = (int16_t)mir_top[-1];
          res_c = res_s;
          mir_top[-1] = ((uint32_t)(-res_s-p2_pred(w_topm1, ((uint8_t)mir_top[-2]))+4)>>3)+w_topm1;
          wd4000a = d4000->weighted;
          ed4000a = res_c-p2_pred(wd4000a, d4000->rate);
          d4000->weighted = p2_bump(wd4000a, ed4000a, 2);
          wr4000b = r4000->weighted;
          go = lowbits<3;
          er4000b = res_c-p2_pred(wr4000b, r4000->rate);
          r4000->weighted = p2_bump(wr4000b, er4000b, 3);
          if( go ) {
            *(uint16_t*)&d4000[1].weighted += (uint32_t)(res_c-p2_pred(d4000[1].weighted, *(uint8_t*)&d4000[1].rate)+2)>>2;
            wd4000m1a = d4000[-1].weighted;
            ed4000m1a = res_c-p2_pred(wd4000m1a, *(uint8_t*)&d4000[-1].rate);
            neg_c = -res_c;
            d4000[-1].weighted = p2_bump(wd4000m1a, ed4000m1a, 3);
            wm4000b = m4000->weighted;
            em4000b = neg_c-p2_pred(wm4000b, m4000->rate);
            m4000->weighted = p2_bump(wm4000b, em4000b, 3);
            wd2000b = d2000->weighted;
            ed2000b = res_c-p2_pred(wd2000b, d2000->rate);
            d2000->weighted = p2_bump(wd2000b, ed2000b, 2);
            wr2000b = r2000->weighted;
            er2000b = res_c-p2_pred(wr2000b, r2000->rate);
            r2000->weighted = p2_bump(wr2000b, er2000b, 3);
            *(uint16_t*)&d2000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d2000[1].weighted, *(uint8_t*)&d2000[1].rate)+2)>>2;
            wd2000m1a = d2000[-1].weighted;
            ed2000m1a = res_c-p2_pred(wd2000m1a, *(uint8_t*)&d2000[-1].rate);
            d2000[-1].weighted = p2_bump(wd2000m1a, ed2000m1a, 3);
            wm2000b = m2000->weighted;
            em2000b = neg_c-p2_pred(wm2000b, m2000->rate);
            m2000->weighted = p2_bump(wm2000b, em2000b, 3);
            wd1000b = d1000->weighted;
            ed1000b = res_c-p2_pred(wd1000b, d1000->rate);
            d1000->weighted = p2_bump(wd1000b, ed1000b, 2);
            wr1000b = r1000->weighted;
            er1000b = res_c-p2_pred(wr1000b, r1000->rate);
            r1000->weighted = p2_bump(wr1000b, er1000b, 3);
            *(uint16_t*)&d1000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d1000[1].weighted, *(uint8_t*)&d1000[1].rate)+2)>>2;
            wd1000m1a = d1000[-1].weighted;
            ed1000m1a = res_c-p2_pred(wd1000m1a, *(uint8_t*)&d1000[-1].rate);
            d1000[-1].weighted = p2_bump(wd1000m1a, ed1000m1a, 3);
            wm1000b = m1000->weighted;
            em1000b = neg_c-p2_pred(wm1000b, m1000->rate);
            m1000->weighted = p2_bump(wm1000b, em1000b, 3);
            wd0800b = d0800->weighted;
            ed0800b = res_c-p2_pred(wd0800b, d0800->rate);
            d0800->weighted = p2_bump(wd0800b, ed0800b, 2);
            wr0800b = r0800->weighted;
            er0800b = res_c-p2_pred(wr0800b, r0800->rate);
            r0800->weighted = p2_bump(wr0800b, er0800b, 3);
            *(uint16_t*)&d0800[1].weighted += (uint32_t)(res_c-p2_pred(d0800[1].weighted, d0800[1].rate)+2)>>2;
            wd0800m1a = d0800[-1].weighted;
            ed0800m1a = res_c-p2_pred(wd0800m1a, d0800[-1].rate);
            d0800[-1].weighted = p2_bump(wd0800m1a, ed0800m1a, 3);
            wm0800b = m0800->weighted;
            em0800b = neg_c-p2_pred(wm0800b, m0800->rate);
            m0800->weighted = p2_bump(wm0800b, em0800b, 3);
            wd0400b = d0400->weighted;
            ed0400b = res_c-p2_pred(wd0400b, d0400->rate);
            d0400->weighted = p2_bump(wd0400b, ed0400b, 2);
            wr0400b = r0400->weighted;
            er0400b = res_c-p2_pred(wr0400b, r0400->rate);
            r0400->weighted = p2_bump(wr0400b, er0400b, 3);
            *(uint16_t*)&d0400[1].weighted += (uint32_t)(res_c-p2_pred(d0400[1].weighted, d0400[1].rate)+2)>>2;
            wd0400m1a = d0400[-1].weighted;
            ed0400m1a = res_c-p2_pred(wd0400m1a, d0400[-1].rate);
            d0400[-1].weighted = p2_bump(wd0400m1a, ed0400m1a, 3);
            wm0400y = m0400->weighted;
            nres5 = -res_c;
            nb_slot = -res_c-p2_pred(wm0400y, m0400->rate);
            m0400->weighted = p2_bump(wm0400y, nb_slot, 3);
            wd0200b = d0200->weighted;
            ed0200b = res_c-p2_pred(wd0200b, d0200->rate);
            d0200->weighted = p2_bump(wd0200b, ed0200b, 2);
            wr0200b = r0200->weighted;
            er0200b = res_c-p2_pred(wr0200b, r0200->rate);
            r0200->weighted = p2_bump(wr0200b, er0200b, 3);
            *(uint16_t*)&d0200[1].weighted += (uint32_t)(res_c-p2_pred(d0200[1].weighted, d0200[1].rate)+2)>>2;
            wd0200m1a = d0200[-1].weighted;
            ed0200m1a = res_c-p2_pred(wd0200m1a, d0200[-1].rate);
            d0200[-1].weighted = p2_bump(wd0200m1a, ed0200m1a, 3);
            wm0200b = m0200->weighted;
            em0200b = nres5-p2_pred(wm0200b, m0200->rate);
            m0200->weighted = p2_bump(wm0200b, em0200b, 3);
            wd0100b = d0100->weighted;
            ed0100b = res_c-p2_pred(wd0100b, d0100->rate);
            d0100->weighted = p2_bump(wd0100b, ed0100b, 2);
            wr0100b = r0100->weighted;
            er0100b = res_c-p2_pred(wr0100b, r0100->rate);
            r0100->weighted = p2_bump(wr0100b, er0100b, 3);
            *(uint16_t*)&d0100[1].weighted += (uint32_t)(res_c-p2_pred(d0100[1].weighted, d0100[1].rate)+2)>>2;
            wd0100m1a = d0100[-1].weighted;
            ed0100m1a = res_c-p2_pred(wd0100m1a, d0100[-1].rate);
            d0100[-1].weighted = p2_bump(wd0100m1a, ed0100m1a, 3);
            wm0100b = m0100->weighted;
            em0100b = nres5-p2_pred(wm0100b, m0100->rate);
            m0100->weighted = p2_bump(wm0100b, em0100b, 3);
            wd0080b = d0080->weighted;
            ed0080b = res_c-p2_pred(wd0080b, d0080->rate);
            d0080->weighted = p2_bump(wd0080b, ed0080b, 2);
            wr0080b = r0080->weighted;
            er0080b = res_c-p2_pred(wr0080b, r0080->rate);
            r0080->weighted = p2_bump(wr0080b, er0080b, 3);
            *(uint16_t*)&d0080[1].weighted += (uint32_t)(res_c-p2_pred(d0080[1].weighted, d0080[1].rate)+2)>>2;
            wd0080m1a = d0080[-1].weighted;
            ed0080m1a = res_c-p2_pred(wd0080m1a, d0080[-1].rate);
            d0080[-1].weighted = p2_bump(wd0080m1a, ed0080m1a, 3);
            wm0080b = m0080->weighted;
            em0080b = nres5-p2_pred(wm0080b, m0080->rate);
            m0080->weighted = p2_bump(wm0080b, em0080b, 3);
            wd0040b = d0040->weighted;
            ed0040b = res_c-p2_pred(wd0040b, d0040->rate);
            d0040->weighted = p2_bump(wd0040b, ed0040b, 2);
            wr0040b = r0040->weighted;
            er0040b = res_c-p2_pred(wr0040b, r0040->rate);
            r0040->weighted = p2_bump(wr0040b, er0040b, 3);
            *(uint16_t*)&d0040[1].weighted += (uint32_t)(res_c-p2_pred(d0040[1].weighted, d0040[1].rate)+2)>>2;
            wd0040m1a = d0040[-1].weighted;
            ed0040m1a = res_c-p2_pred(wd0040m1a, d0040[-1].rate);
            neg_d = -res_c;
            d0040[-1].weighted = p2_bump(wd0040m1a, ed0040m1a, 3);
            wm0040b = m0040->weighted;
            em0040b = neg_d-p2_pred(wm0040b, m0040->rate);
            m0040->weighted = p2_bump(wm0040b, em0040b, 3);
            wd0020b = d0020->weighted;
            ed0020b = res_c-p2_pred(wd0020b, d0020->rate);
            d0020->weighted = p2_bump(wd0020b, ed0020b, 2);
            wr0020b = r0020->weighted;
            er0020b = res_c-p2_pred(wr0020b, r0020->rate);
            r0020->weighted = p2_bump(wr0020b, er0020b, 3);
            *(uint16_t*)&d0020[1].weighted += (uint32_t)(res_c-p2_pred(d0020[1].weighted, d0020[1].rate)+2)>>2;
            wd0020m1a = d0020[-1].weighted;
            ed0020m1a = res_c-p2_pred(wd0020m1a, d0020[-1].rate);
            d0020[-1].weighted = p2_bump(wd0020m1a, ed0020m1a, 3);
            wm0020b = m0020->weighted;
            em0020b = neg_d-p2_pred(wm0020b, m0020->rate);
            m0020->weighted = p2_bump(wm0020b, em0020b, 3);
            wd0010b = d0010->weighted;
            ed0010b = res_c-p2_pred(wd0010b, d0010->rate);
            d0010->weighted = p2_bump(wd0010b, ed0010b, 2);
            wr0010b = r0010->weighted;
            er0010b = res_c-p2_pred(wr0010b, r0010->rate);
            r0010->weighted = p2_bump(wr0010b, er0010b, 3);
            *(uint16_t*)&d0010[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d0010[1].weighted, *(uint8_t*)&d0010[1].rate)+2)>>2;
            wd0010m1a = d0010[-1].weighted;
            ed0010m1a = res_c-p2_pred(wd0010m1a, *(uint8_t*)&d0010[-1].rate);
            d0010[-1].weighted = p2_bump(wd0010m1a, ed0010m1a, 3);
            wm0010b = m0010->weighted;
            em0010b = neg_d-p2_pred(wm0010b, m0010->rate);
          } else {
            wd4000m1b = d4000[-1].weighted;
            ed4000m1b = res_c-p2_pred(wd4000m1b, *(uint8_t*)&d4000[-1].rate);
            d4000[-1].weighted = p2_bump(wd4000m1b, ed4000m1b, 3);
            wm4000c = m4000->weighted;
            nres1 = -res_c;
            em4000c = -res_c-p2_pred(wm4000c, m4000->rate);
            m4000->weighted = p2_bump(wm4000c, em4000c, 3);
            wd2000c = d2000->weighted;
            ed2000c = res_c-p2_pred(wd2000c, d2000->rate);
            d2000->weighted = p2_bump(wd2000c, ed2000c, 2);
            wr2000c = r2000->weighted;
            er2000c = res_c-p2_pred(wr2000c, r2000->rate);
            r2000->weighted = p2_bump(wr2000c, er2000c, 3);
            wd2000m1b = d2000[-1].weighted;
            ed2000m1b = res_c-p2_pred(wd2000m1b, *(uint8_t*)&d2000[-1].rate);
            d2000[-1].weighted = p2_bump(wd2000m1b, ed2000m1b, 3);
            wm2000c = m2000->weighted;
            em2000c = nres1-p2_pred(wm2000c, m2000->rate);
            m2000->weighted = p2_bump(wm2000c, em2000c, 3);
            wd1000c = d1000->weighted;
            ed1000c = res_c-p2_pred(wd1000c, d1000->rate);
            d1000->weighted = p2_bump(wd1000c, ed1000c, 2);
            wr1000c = r1000->weighted;
            er1000c = res_c-p2_pred(wr1000c, r1000->rate);
            r1000->weighted = p2_bump(wr1000c, er1000c, 3);
            wd1000m1b = d1000[-1].weighted;
            ed1000m1b = res_c-p2_pred(wd1000m1b, *(uint8_t*)&d1000[-1].rate);
            d1000[-1].weighted = p2_bump(wd1000m1b, ed1000m1b, 3);
            wm1000c = m1000->weighted;
            em1000c = nres1-p2_pred(wm1000c, m1000->rate);
            m1000->weighted = p2_bump(wm1000c, em1000c, 3);
            wd0800c = d0800->weighted;
            ed0800c = res_c-p2_pred(wd0800c, d0800->rate);
            d0800->weighted = p2_bump(wd0800c, ed0800c, 2);
            wr0800c = r0800->weighted;
            er0800c = res_c-p2_pred(wr0800c, r0800->rate);
            r0800->weighted = p2_bump(wr0800c, er0800c, 3);
            wd0800m1b = d0800[-1].weighted;
            ed0800m1b = res_c-p2_pred(wd0800m1b, d0800[-1].rate);
            d0800[-1].weighted = p2_bump(wd0800m1b, ed0800m1b, 3);
            wm0800c = m0800->weighted;
            em0800c = nres1-p2_pred(wm0800c, m0800->rate);
            m0800->weighted = p2_bump(wm0800c, em0800c, 3);
            wd0400c = d0400->weighted;
            ed0400c = res_c-p2_pred(wd0400c, d0400->rate);
            d0400->weighted = p2_bump(wd0400c, ed0400c, 2);
            wr0400c = r0400->weighted;
            er0400c = res_c-p2_pred(wr0400c, r0400->rate);
            r0400->weighted = p2_bump(wr0400c, er0400c, 3);
            wd0400m1b = d0400[-1].weighted;
            ed0400m1b = res_c-p2_pred(wd0400m1b, d0400[-1].rate);
            d0400[-1].weighted = p2_bump(wd0400m1b, ed0400m1b, 3);
            wm0400x = m0400->weighted;
            nres1 -= p2_pred(wm0400x, m0400->rate);
            m0400->weighted = p2_bump(wm0400x, nres1, 3);
            wd0200c = d0200->weighted;
            ed0200c = res_c-p2_pred(wd0200c, d0200->rate);
            d0200->weighted = p2_bump(wd0200c, ed0200c, 2);
            wr0200c = r0200->weighted;
            er0200c = res_c-p2_pred(wr0200c, r0200->rate);
            r0200->weighted = p2_bump(wr0200c, er0200c, 3);
            wd0200m1b = d0200[-1].weighted;
            ed0200m1b = res_c-p2_pred(wd0200m1b, d0200[-1].rate);
            neg_b = -res_c;
            d0200[-1].weighted = p2_bump(wd0200m1b, ed0200m1b, 3);
            wm0200c = m0200->weighted;
            em0200c = neg_b-p2_pred(wm0200c, m0200->rate);
            m0200->weighted = p2_bump(wm0200c, em0200c, 3);
            wd0100c = d0100->weighted;
            ed0100c = res_c-p2_pred(wd0100c, d0100->rate);
            d0100->weighted = p2_bump(wd0100c, ed0100c, 2);
            wr0100c = r0100->weighted;
            er0100c = res_c-p2_pred(wr0100c, r0100->rate);
            r0100->weighted = p2_bump(wr0100c, er0100c, 3);
            wd0100m1b = d0100[-1].weighted;
            ed0100m1b = res_c-p2_pred(wd0100m1b, d0100[-1].rate);
            d0100[-1].weighted = p2_bump(wd0100m1b, ed0100m1b, 3);
            wm0100c = m0100->weighted;
            em0100c = neg_b-p2_pred(wm0100c, m0100->rate);
            m0100->weighted = p2_bump(wm0100c, em0100c, 3);
            wd0080c = d0080->weighted;
            ed0080c = res_c-p2_pred(wd0080c, d0080->rate);
            d0080->weighted = p2_bump(wd0080c, ed0080c, 2);
            wr0080c = r0080->weighted;
            er0080c = res_c-p2_pred(wr0080c, r0080->rate);
            r0080->weighted = p2_bump(wr0080c, er0080c, 3);
            wd0080m1b = d0080[-1].weighted;
            ed0080m1b = res_c-p2_pred(wd0080m1b, d0080[-1].rate);
            d0080[-1].weighted = p2_bump(wd0080m1b, ed0080m1b, 3);
            wm0080c = m0080->weighted;
            em0080c = neg_b-p2_pred(wm0080c, m0080->rate);
            m0080->weighted = p2_bump(wm0080c, em0080c, 3);
            wd0040c = d0040->weighted;
            ed0040c = res_c-p2_pred(wd0040c, d0040->rate);
            d0040->weighted = p2_bump(wd0040c, ed0040c, 2);
            wr0040c = r0040->weighted;
            er0040c = res_c-p2_pred(wr0040c, r0040->rate);
            r0040->weighted = p2_bump(wr0040c, er0040c, 3);
            wd0040m1b = d0040[-1].weighted;
            ed0040m1b = res_c-p2_pred(wd0040m1b, d0040[-1].rate);
            d0040[-1].weighted = p2_bump(wd0040m1b, ed0040m1b, 3);
            wm0040c = m0040->weighted;
            em0040c = neg_b-p2_pred(wm0040c, m0040->rate);
            m0040->weighted = p2_bump(wm0040c, em0040c, 3);
            wd0020c = d0020->weighted;
            res_t = res_c;
            ed0020c = res_c-p2_pred(wd0020c, d0020->rate);
            d0020->weighted = p2_bump(wd0020c, ed0020c, 2);
            wr0020c = r0020->weighted;
            er0020c = res_t-p2_pred(wr0020c, r0020->rate);
            res_c = res_t;
            r0020->weighted = p2_bump(wr0020c, er0020c, 3);
            wd0020m1b = d0020[-1].weighted;
            ed0020m1b = res_t-p2_pred(wd0020m1b, d0020[-1].rate);
            d0020[-1].weighted = p2_bump(wd0020m1b, ed0020m1b, 3);
            wm0020c = m0020->weighted;
            nres4 = -res_t;
            em0020c = -res_t-p2_pred(wm0020c, m0020->rate);
            m0020->weighted = p2_bump(wm0020c, em0020c, 3);
            wd0010c = d0010->weighted;
            ed0010c = res_c-p2_pred(wd0010c, d0010->rate);
            d0010->weighted = p2_bump(wd0010c, ed0010c, 2);
            wr0010c = r0010->weighted;
            er0010c = res_c-p2_pred(wr0010c, r0010->rate);
            r0010->weighted = p2_bump(wr0010c, er0010c, 3);
            wd0010m1b = d0010[-1].weighted;
            ed0010m1b = res_c-p2_pred(wd0010m1b, *(uint8_t*)&d0010[-1].rate);
            d0010[-1].weighted = p2_bump(wd0010m1b, ed0010m1b, 3);
            wm0010b = m0010->weighted;
            em0010b = nres4-p2_pred(wm0010b, m0010->rate);
          }
          m0010->weighted = p2_bump(wm0010b, em0010b, 3);
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
