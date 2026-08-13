// alt_p2_context.inc -- the deep-plane model's context: which of its counters the next
// sample is coded against.

int32_t __alt_p2_context(AltP2Block* blk, AltP2Block* refa, AltP2Block* refb) {
  P2Ctx* unused_p, * row0;
  CtxWeights weights;
  int32_t dv_now, mag_ref1, ctx15, mag_ref0, d_run0b, g3pair;
  P2Ctx* ra1, * ra0, * rb1, * rb0, * rb2, * ra2, * sub0_row, * row2c;
  int32_t sum_all;
  P2Ctx* cx1, * cx3;
  uint32_t q39, q10;
  float (**up_row)[4];
  int32_t dtop2, run_dv3, dv_now4, run_dv4, run_up4;
  int8_t rate1;
  uint8_t* cx2p;
  P2Ctx* nb4;
  int32_t num_b, band, num_d;
  int32_t gA, gB, gC, gD, den_d, nb_slot;
  P2Ctx* cx0;
  P2Ctx* cx2;
  int32_t up5, lap, magl;
  int32_t dvsum2, magu;
  P2Ctx* nb0, * nb3, * nb2, * cx4, * nb3x;
  float (*wrow)[4];
  bool in_band, no_ref;
  int16_t* nb2w, * nb2w2;
  P2Ctx* cursor1, * nb1, * nb2x;
  float (**cur)[4];
  int32_t sum4, lane5, nb_id, next_id, plane, in63, c_lo, mode, c_mid, filt, lane3, g3sum, run_s, lane2, ctx0, bank0, pred0, sum_c, run0, ctx1, bank1, w1c, one1, pred1, run1, cx2_val0, ctx2, bank2, pred2, run2, bank3, w3c, pred3, run3, bank4, pred4, band_lo, band_hi, d_run4, magsum, run_s2, mag, flat_a, flat_b, d_run4b, d_rb_rb1, d_rb_left, ra0_val, d_ra_ra1b, d_ra_leftb, ctx_idx, d_ra_ra1, d_ra_left;
  uint32_t ctx0_lo, q24, q10a, q10b, q9;
  int32_t sum_u, run, nb4_4, run4, sum_ul, d_up, d_ra;
  int32_t sum_ur, d_up5, dv1, d_run0, cx1_val, magsum_s;
  row0 = blk->cursor[0];
  cursor1 = blk->cursor[1];
  sub0_row = (P2Ctx*)blk->cursor[2];
  sum_ul = 21*sub0_row[-1].dupleft+12*cursor1[3].dupleft+16*cursor1[2].dupleft+22*cursor1[1].dupleft+(23*row0[-1].dupleft)+20*cursor1->dupleft+ctx_bias[0]+14*row0[-2].dupleft;
  sum_ur = 17*sub0_row[-2].dupright+21*cursor1[2].dupright+15*cursor1[1].dupright+25*cursor1->dupright+9*cursor1[-1].dupright+22*row0[-1].dupright+ctx_bias[1]+19*row0[-2].dupright;
  sum4 = 17*cursor1[3].dleft+15*cursor1[2].dleft+21*cursor1[1].dleft+18*cursor1->dleft+16*cursor1[-1].dleft+22*row0[-1].dleft+ctx_bias[2]+19*row0[-2].dleft;
  lane5 = sub0_row->dup;
  sum_u = 14*cursor1[3].dup+23*cursor1[1].dup+19*cursor1->dup+25*row0[-1].dup+ctx_bias[3]+17*row0[-2].dup+15*(blk->cursor[3]->dup+lane5);
  sum_all = sum4+sum_u+sum_ul+sum_ur;
  band = (8*sum4>43*sum_u)+(8*sum4>17*sum_u)+(8*sum4>9*sum_u)+(8*sum4>5*sum_u)+(8*sum4>2*sum_u);
  den_d = 2*row0[-2].val+2*row0[-1].val;
  num_b = cursor1[1].val+(cursor1->val+2*row0[-1].val);
  num_d = 16*(sub0_row[2].val+sub0_row->val+cursor1[1].val+sub0_row[-1].val);
  gA = (sum_all>bmf_p2_thresholds[band][12])+(sum_all>bmf_p2_thresholds[band][11])+(sum_all>bmf_p2_thresholds[band][10])+(sum_all>bmf_p2_thresholds[band][9]);
  gB = (num_b>bmf_p2_thresholds[band][8])+(num_b>bmf_p2_thresholds[band][7])+(num_b>bmf_p2_thresholds[band][6]);
  gC = (16*sum_ur>sum_ul*bmf_p2_thresholds[band][5])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][4])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][3]);
  gD = (num_d>bmf_p2_thresholds[band][2]*den_d)+(num_d>den_d*bmf_p2_thresholds[band][1])+(num_d>den_d*bmf_p2_thresholds[band][0]);
  // Mixed radix, and only three of the five terms are bit fields.  `gB`, `gC`
  // and `gD` each sum three comparisons, so each is 0..3 and each is two bits.
  // `gA` sums *four*, so it is 0..4 -- five values with a stride of 64, which
  // is not a field however it is written, and `band` strides 320, which is
  // 5 * 64 and not a power of two either.  Both stay `raw`.
  //
  // Writing them as `bits<6, 2>(gA)` is wrong and was tried: it masks gA to
  // two bits, so the one neighbourhood in the corpus that reaches gA == 4
  // lands in slot 0 instead of slot 256.  Three streams moved and the gate
  // said so.  1916 slots is what `nb_id[]` is sized for, and 5*320 = 1600 plus
  // the rest is what fills it.
  nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                    .raw(64*gA).raw(320*band);
  nb_id = blk->nb_id[(uint32_t)nb_slot];
  if( blk->nb_id[(uint32_t)nb_slot] ) {
    wrow = blk->nb_weights[nb_id];
    blk->nb_cur = wrow;
    {
      float acc[4], err, floor_;
      int32_t j, k;
      for( k = 0; k<4; ++k ) {
        acc[k] = wrow[0][k]*blk->p2_row[0][k];
        for( j = 1; j<7; ++j )
          acc[k] += wrow[j][k]*blk->p2_row[j][k];
      }
      err = ((float)row0[-1].val-(bmf_hsum4(acc)+blk->bias[0]))*2.0999999f;
      floor_ = 7744.0f*wrow[14][2];
      for( j = 0; j<7; ++j )
        for( k = 0; k<4; ++k ) {
          float x = blk->p2_row[j][k];
          float ms = wrow[7+j][k]+(x*x-wrow[7+j][k])*bmf_p2_ms_rate;
          wrow[7+j][k] = ms;
          wrow[j][k] += bmf_p2_rate[j][k]*err*x/(ms+floor_);
        }
    }
  } else {
    next_id = blk->nb_id_used;
    blk->nb_id_used = ++next_id;
    blk->nb_id[(uint32_t)nb_slot] = next_id;
    blk->nb_cur = blk->nb_weights[(int16_t)next_id];
  }
  nb1 = (P2Ctx*)blk->cursor[1];
  blk->p2_row[0][0] = (float)nb1->dval;
  nb0 = (P2Ctx*)blk->cursor[0];
  blk->p2_row[0][1] = (float)nb1[1].dval;
  blk->p2_row[0][2] = (float)(nb0[-1].dval+nb1->dval-nb1[-1].dval);
  row2c = (P2Ctx*)blk->cursor[2];
  blk->p2_row[0][3] = (float)(nb0[-2].dval+(nb1->dval-nb1[-2].dval));
  nb3 = (P2Ctx*)blk->cursor[3];
  blk->p2_row[1][0] = (float)(nb1[-1].dval+nb1->dval-row2c[-1].dval);
  blk->p2_row[1][1] = (float)(-3*(row2c->dval-nb1->dval)+nb3->dval);
  blk->p2_row[1][2] = (float)(nb0[-1].dval+nb1[2].dval-nb1[1].dval);
  blk->p2_row[1][3] = (float)(nb0[-2].dval+nb1[1].dval-nb1[-1].dval);
  blk->p2_row[2][0] = (float)(2*nb0[-1].dval-nb0[-2].dval);
  nb2 = (P2Ctx*)(row2c);
  blk->p2_row[2][1] = (float)(nb0[-3].dval+nb1->dval-nb1[-3].dval);
  blk->p2_row[2][2] = (float)(nb2->dval+nb1[1].dval-nb3[1].dval);
  blk->p2_row[2][3] = (float)nb0[-3].dval;
  blk->p2_row[3][0] = (float)(nb1[-2].dval+nb1->dval-nb2[-2].dval);
  blk->p2_row[3][1] = (float)(nb0[-2].dval+nb1[-1].dval-nb1[-3].dval);
  blk->p2_row[3][2] = (float)(nb1[1].dval+((nb1[2].dval+nb1->dval)>>1)-nb2[2].dval);
  blk->p2_row[3][3] = (float)nb3->dval;
  if( refa ) {
    ra0 = (P2Ctx*)refa->cursor[0]-1;
    ra1 = (P2Ctx*)((int16_t*)(refa->cursor[1]-1));
    ra2 = (P2Ctx*)((int16_t*)(refa->cursor[2]-1));
    rb0 = (P2Ctx*)refb->cursor[0]-1;
    no_ref = blk->has_ref==0;
    rb1 = refb->cursor[1]-1;
    rb2 = refb->cursor[2]-1;
    if( !no_ref ) {
      {
        float bias = (float)nb0->dval;
        int32_t j, k;
        for( j = 0; j<4; ++j )
          for( k = 0; k<4; ++k )
            blk->p2_row[j][k] += bias;
      }
      nb0 = (P2Ctx*)blk->cursor[0];
      nb1 = (P2Ctx*)blk->cursor[1];
    }
    plane = blk->plane_idx;
    if( plane ) {
      if( plane==1 ) {
        blk->p2_row[4][0] = (float)(nb0->dval+nb1[3].dval);
        blk->p2_row[4][1] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        blk->p2_row[4][2] = (float)(nb1[1].val+ra0[-1].val-ra1->val);
        blk->p2_row[4][3] = (float)(nb0[-1].val+ra1->val-ra1[-1].val);
        blk->p2_row[5][0] = (float)(nb1[1].val+(rb1[2].val-((P2Ctx*)(rb2))[3].val));
        blk->p2_row[5][1] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        blk->p2_row[5][2] = (float)(nb1->val+rb0->val-rb1->val);
        no_ref = blk->has_ref==0;
        blk->p2_row[5][3] = (float)(nb0[-2].val+ra0->err);
        if( no_ref ) {
          nb2w = (int16_t*)blk->cursor[2];
          blk->p2_row[6][0] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
          blk->p2_row[6][1] = (float)(*nb2w+ra0->val-((P2Ctx*)(ra2))->val);
          blk->p2_row[6][2] = (float)(ra2->val+ra0->val-*nb2w+2*(nb1->val-ra1->val));
          in63 = *(int16_t*)((uint8_t*)nb0-18)+nb1[-1].val+ra1[-2].val+ra0->val-ra0[-1].val-ra1[-1].val-nb1[-2].val;
        } else {
          blk->p2_row[6][0] = (float)nb1->val;
          blk->p2_row[6][1] = (float)nb0[-3].val;
          blk->p2_row[6][2] = (float)(nb0[-1].val+ra0[-1].val-ra0[-2].val);
          in63 = nb0[-3].val+nb0[-1].val-nb0[-4].val;
        }
        blk->p2_row[6][3] = (float)in63;
      } else {
        blk->p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
        blk->p2_row[4][1] = (float)(nb0[-1].val+ra0[-1].val-ra0[-2].val);
        blk->p2_row[4][2] = (float)(nb1[1].val+ra1->val-ra2[1].val);
        blk->p2_row[4][3] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        blk->p2_row[5][0] = (float)(nb1->val+ra0[-2].val-ra1[-2].val);
        blk->p2_row[5][1] = (float)(nb1->val+ra0->val-ra1->val);
        blk->p2_row[5][2] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
        nb2w2 = (int16_t*)blk->cursor[2];
        blk->p2_row[5][3] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        blk->p2_row[6][0] = (float)(*nb2w2+rb0->val-((P2Ctx*)(rb2))->val);
        blk->p2_row[6][1] = (float)(rb0[-2].val+rb0->val-nb0[-2].val+2*(nb0[-1].val-rb0[-1].val));
        blk->p2_row[6][2] = (float)(nb0[-2].val+rb0->err);
        blk->p2_row[6][3] = (float)(*nb2w2+ra0->err);
      }
    } else {
      blk->p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
      nb3x = blk->cursor[3];
      blk->p2_row[4][1] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
      nb2x = (P2Ctx*)blk->cursor[2];
      blk->p2_row[4][2] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
      blk->p2_row[4][3] = (float)((nb2x->val+3*nb1[1].val-4*nb2x[1].val)-(((nb1[2].val-nb1->val-(nb3x[2].val-nb3x->val))>>1)-nb3x[1].val));
      blk->p2_row[5][0] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
      blk->p2_row[5][1] = (float)(nb1[1].val+ra1[1].val-ra2[2].val);
      blk->p2_row[5][2] = (float)(nb1[-2].val+ra1[2].val-ra2->val);
      blk->p2_row[5][3] = (float)((ra0[-2].val+ra0->val-nb0[-2].val)+2*(nb0[-1].val-ra0[-1].val));
      blk->p2_row[6][0] = (float)(nb2x[1].val+rb0->val-rb2[1].val);
      blk->p2_row[6][1] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
      blk->p2_row[6][2] = (float)(nb0[-1].val+rb1[1].val-rb1->val);
      blk->p2_row[6][3] = (float)(nb1[1].val+rb1[2].val-rb2[3].val);
    }
  } else {
    blk->p2_row[4][0] = (float)(nb1[3].val+nb1->val-nb2[3].val);
    row2c = nb2;
    blk->p2_row[4][1] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
    blk->p2_row[4][2] = (float)((nb2->val+3*nb1[1].val-4*nb2[1].val)-((((nb1[2].val-nb1->val)-(nb3[2].val-nb3->val))>>1)-nb3[1].val));
    nb4 = (P2Ctx*)blk->cursor[4];
    blk->p2_row[4][3] = (float)(row2c[-1].val+nb1[1].val-nb3->val);
    blk->p2_row[5][0] = (float)(nb3[1].val+nb1->val-nb4[1].val);
    blk->p2_row[5][1] = (float)nb1[3].val;
    blk->p2_row[5][2] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
    blk->p2_row[5][3] = (float)(nb0[-1].val+nb3->val-nb3[-1].val);
    blk->p2_row[6][0] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
    blk->p2_row[6][1] = (float)nb4->val;
    blk->p2_row[6][2] = (float)(nb0[-5].val+nb0[-1].val-nb0[-6].val);
    blk->p2_row[6][3] = (float)nb1[-6].val;
    rb2 = (P2Ctx*)(nullptr);
    rb1 = (P2Ctx*)(nullptr);
    rb0 = nullptr;
    ra2 = (P2Ctx*)(nullptr);
    ra1 = (P2Ctx*)(nullptr);
    ra0 = nullptr;
  }
  cur = blk->cur;
  weights.f0[0] = cur[-1];
  up_row = blk->above;
  weights.f0[1] = up_row[1];
  weights.f0[2] = up_row[2];
  weights.f0[3] = cur[-2];
  weights.f0[4] = up_row[0];
  weights.f0[5] = cur[0];
  c_lo = 14*sum_u;
  mode = 1;
  c_mid = 13*sum_ur;
  if( 16*sum4<=14*sum_u ) {
    c_lo = 16*sum4;
    mode = 0;
  }
  if( c_lo>c_mid )
    mode = 2;
  else
    c_mid = c_lo;
  if( c_mid>11*sum_ul )
    mode = 3;
  filt = __alt_p2_filter(blk->nb_cur, (float (*)[4]) blk->p2_row, &weights, mode);
  cx0 = (P2Ctx*)blk->cursor[0];
  cx1 = (P2Ctx*)blk->cursor[1];
  *(int32_t*)&blk->pred_prev = filt;
  lane3 = cx1->aerr;
  g3pair = cx0[-1].aerr+lane3;
  if( refa )
    g3pair += (rb0->aerr+ra0->aerr)>>1;
  run = filt;
  cx3 = (P2Ctx*)(blk->cursor[3]);
  g3sum = cx1[3].aerr+cx1[-1].aerr+cx0[-1].aerr+cx0[-3].aerr+cx0[-4].aerr+cx0[-2].aerr;
  run_s = run;
  ctx0_lo = (cx1[1].err+cx1->err+cx1[-1].err+cx0[-1].err)&0x80000|(cx3->err+cx0[-2].err+2*cx0[-4].err)&0x40000|(cx1[-3].err+cx0[-3].err+cx0[-5].err+cx0[-7].err)&0x20000|cx0[-1].err&0x10000|(uint16_t)cx0[-3].err&0x8000|(((run>3536)+(run>720)+(run>288))<<13)|((((uint32_t)(752-(g3pair+g3sum))>>31)+((uint32_t)(400-(g3pair+g3sum))>>31)+((uint32_t)(240-(g3pair+g3sum))>>31))<<11);
  blk->bank_ctx[0] = ctx0_lo;
  if( refa ) {
    lane2 = ra0->err;
    run = run_s;
    ctx0 = rb0[-1].err&0x2000000|rb0->err&0x1000000|ra0[-1].err&0x800000|(rb0->err+rb0[-2].err)&0x400000|(ra0[-2].err+lane2)&0x200000|cx0[-2].err&0x100000|ctx0_lo;
    cx2 = (P2Ctx*)blk->cursor[2];
  } else {
    run = run_s;
    cx2 = (P2Ctx*)blk->cursor[2];
    ctx0 = (cx1[3].err+cx0[-3].err+cx0[-7].err+cx0[-5].err)&0x2000000|(cx2[1].err+cx2->err+cx0[-4].err+cx0[-8].err)&0x1000000|(cx0[-4].err+cx0[-6].err)&0x800000|cx0[-4].err&0x400000|cx0[-5].err&0x200000|cx0[-7].err&0x100000|ctx0_lo;
  }
  bank0 = ctx0>>11;
  blk->bank_ctx[0] = bank0;
  pred0 = p2_pred(blk->p2_ctr[bank0].weighted, blk->p2_ctr[bank0].rate);
  sum_c = sum_all;
  cx4 = blk->cursor[4];
  blk->nb_sum[1] = pred0;
  run0 = pred0+run_s;
  blk->nb_sum[0] = run0;
  nb4_4 = cx4[4].val;
  sum_all = ((g3pair<<9)+sum_c)>>13;
  d_run0 = cx0[-5].val-run0;
  q24 = ((uint32_t)(24-sum_all)>>20)&0xFFFFF800;
  d_up = nb4_4-run0;
  up5 = cx1[5].val;
  d_up5 = up5-run0;
  q39 = ((uint32_t)(39-sum_all)>>20)&0xFFFFF800;
  if( refa ) {
    q10 = ((uint32_t)(10-sum_all)>>20)&0xFFFFF800;
    dv_now = cx0[0].dval;
    q10a = q10;
    ctx1 = (cx0[-1].val+ra0->val-ra0[-1].val-run0)&0x2000000|(ra2->dval+ra1->dval-2*ra0->dval)&0x1000000|((dv_now+cx0[-3].dval-run0-(cx1[2].dval-cx1[5].dval))&0x800000|(dv_now+cx0[-5].dval-run0)&0x400000|(dv_now+cx1->dval+rb1[2].dval-rb2[2].dval-run0)&0x200000|(dv_now+cx0[-1].dval+rb0->dval-rb0[-1].dval-run0)&0x100000|(cx2[-1].dval+dv_now+ra0->dval-ra2[-1].dval-run0)&0x80000|(dv_now+cx1[2].dval+ra0->dval-ra1[2].dval-run0)&0x40000|d_up5&0x20000|d_up&0x10000|d_run0&0x8000|(((run0>2256)+(run0>1056)+(run0>144))<<13)|((((uint32_t)(55-sum_all)>>20)&0xFFFFF800)+q10+q24));
  } else {
    q10 = ((uint32_t)(10-sum_all)>>20)&0xFFFFF800;
    d_run0b = run0-cx0[-3].val;
    ctx1 = (cx1[-5].val-cx1[-2].val+d_run0b)&0x1000000|(d_run0b+cx1[2].val-up5)&0x800000|(cx4[3].val-cx2[2].val+run0-cx2[1].val)&0x400000|(cx4[-1].val-cx3->val+run0-cx1[-1].val)&0x200000|(cx1->val-cx1[2].val+run0-cx0[-2].val)&0x100000| -d_up5&0x80000| -d_up&0x40000|(run0-cx4[2].val)&0x20000|(run0-cx4[-3].val)&0x10000| -d_run0&0x8000|(((run0>2400)+(run0>1024)+(run0>240))<<13)|(q39+q24+(((uint32_t)(11-sum_all)>>20)&0xFFFFF800))|(nb4_4-cx3->val+run0-cx1[4].val)&0x2000000;
    q10a = q10;
  }
  q10 = q10a;
  bank1 = ctx1>>11;
  blk->bank_ctx[1] = bank1;
  rate1 = blk->p2_ctr[bank1+32768].rate;
  w1c = blk->p2_ctr[bank1+32768].weighted;
  one1 = 1<<((rate1+31)&31);
  pred1 = (one1+w1c)>>(rate1&31);
  run1 = pred1+run0;
  blk->nb_sum[2] = run1;
  blk->nb_sum[3] = pred1;
  cx2_val0 = cx2->val;
  cx1_val = cx1->val;
  dvsum2 = cx0[-2].val-run1;
  lap = cx0[-2].val+run1-2*cx0[-1].val;
  dtop2 = cx2_val0-run1;
  if( refa ) {
    dv1 = cx1[1].dval;
    d_ra = ra0->val-run1;
    q10b = q10;
    ctx2 = (cx0[0].dval+cx1[-1].dval-run1-(cx2->dval-dv1))&0x2000000|(cx0[0].dval+cx0[-1].dval-run1-(cx1->dval-dv1))&0x1000000|(rb0->val-run1+cx2_val0-rb2->val)&0x800000|(d_ra+cx2_val0-ra2->val)&0x400000|(d_ra+cx1[-1].val-ra1[-1].val)&0x200000|((cx0[0].dval-cx0[-2].dval+2*cx0[-1].dval-run1)&0x100000|(2*cx1_val-run1-cx2_val0)&0x80000| -lap&0x40000|dtop2&0x20000|dvsum2&0x10000|(208-rb0->val)&0x8000|(((run1>2576)+(run1>1280)+(run1>640))<<13)|((((uint32_t)(33-sum_all)>>31)+((uint32_t)(12-sum_all)>>31)+((uint32_t)(4-sum_all)>>31))<<11));
  } else {
    q10b = q10;
    ctx2 = (run1+3*(cx2_val0-cx1_val)-cx3->val)&0x2000000|(run1+cx4->val-(cx2[2].val+cx2[-2].val))&0x1000000|(cx3[2].val-cx2[-1].val+run1-cx1[3].val)&0x800000|(cx3[1].val-cx1[1].val-dtop2)&0x400000|(cx3->val-cx1_val-dtop2)&0x200000|lap&0x100000|(run1-cx4[3].val)&0x80000|(cx3[1].val-run1)&0x40000|(run1-cx4->val)&0x20000|(run1-cx3[-2].val)&0x10000| -dvsum2&0x8000|(((run1>2464)+(run1>1216)+(run1>688))<<13)|((((uint32_t)(58-sum_all)>>31)+((uint32_t)(25-sum_all)>>31)+((uint32_t)(13-sum_all)>>31))<<11);
  }
  q10 = q10b;
  bank2 = ctx2>>11;
  blk->bank_ctx[2] = bank2;
  pred2 = p2_pred(blk->p2_ctr[bank2+65536].weighted, blk->p2_ctr[bank2+65536].rate);
  blk->nb_sum[5] = pred2;
  run2 = pred2+run1;
  blk->nb_sum[4] = run2;
  run_dv3 = run2-cx0[0].dval;
  q9 = 9-sum_all;
  sum_all = -sum_all;
  bank3 = (int32_t)((3*(cx0[-2].val-cx0[-1].val)+run2-cx0[-3].val)&0x2000000|(cx2[1].dval-((uint32_t)(cx1[1].dval+cx1[2].dval+cx1[-1].dval+cx1->dval)>>1)+run_dv3)&0x1000000|(cx2[-3].val-cx1[-2].val+run2-cx1[-1].val)&0x800000| -(cx2[2].val+run2-2*cx1[1].val)&0x400000|(cx1[-2].dval-cx0[-2].val+run2-cx1->dval)&0x200000|(run2-cx1[3].val)&0x100000|(run_dv3-cx2[1].dval)&0x80000|(run2-cx3->val)&0x40000|(2*run2-cx1->dval-(cx1->val+cx0[0].dval))&0x20000|(run2-cx0[-1].dval-cx0[0].dval)&0x10000|(run2-cx0[-3].val)&0x8000|(((run2>2896)+(run2>1568)+(run2>592))<<13)|((((uint32_t)(sum_all+37)>>31)+((uint32_t)(sum_all+19)>>31)+(q9>>31))<<11))>>11;
  blk->bank_ctx[3] = bank3;
  w3c = blk->p2_ctr[bank3+98304].weighted;
  pred3 = p2_pred(w3c, blk->p2_ctr[bank3+98304].rate);
  blk->nb_sum[7] = pred3;
  run3 = pred3+run2;
  run = run3;
  blk->nb_sum[6] = run3;
  dv_now4 = cx0[0].dval;
  run_dv4 = run3-dv_now4;
  run_up4 = run3+cx3->val;
  bank4 = (int32_t)((run-((uint32_t)(cx1[1].val+cx1[-1].val+2*cx1->val)>>2))&0x2000000|(run_up4-cx0[-2].val-(cx3[2].dval+dv_now4))&0x1000000|(run-2*cx0[-2].dval-(dv_now4-cx0[-4].dval))&0x800000|(run-cx0[-3].dval-dv_now4-(cx1->val-cx1[-3].val))&0x400000|(run_up4-dv_now4-(cx1[1].val+cx2[-1].dval))&0x200000|(cx2[-2].val+run-2*cx1[-1].val)&0x100000|(cx2->dval-2*cx1->dval+run_dv4)&0x80000|((run3-cx0[-1].val)-(cx1->val-cx1[-1].val))&0x40000|(run_dv4-cx1[4].dval)&0x20000|(run_dv4-cx1[-2].dval)&0x10000|(((uint16_t)run3-(uint16_t)cx0[-4].dval-(uint16_t)dv_now4)&0x8000)|((((run3>3056)+(run3>1952)+(run3>368))<<13)|(q39+(((uint32_t)(sum_all+21)>>20)&0xFFFFF800)+q10)))>>11;
  blk->bank_ctx[4] = bank4;
  pred4 = p2_pred(blk->p2_ctr[bank4+131072].weighted, blk->p2_ctr[bank4+131072].rate);
  cx2p = (uint8_t*)cx2;
  blk->nb_sum[9] = pred4;
  run4 = run+pred4;
  blk->nb_sum[8] = run+pred4;
  magsum_s = cx1[5].mag+cx1[4].mag+cx1[-3].mag+cx1[-4].mag+3*(cx1[2].mag+cx2[1].mag)+7*cx1->mag+6*cx1[1].mag+cx0[-6].mag+cx0[-7].mag+cx0[-8].mag+8*cx0[-1].mag+cx4[2].mag+cx4[1].mag+cx4->mag+cx4[-1].mag+cx0[-4].mag+cx0[-5].mag+(cx3[2].mag+cx3[1].mag+cx3[-1].mag+cx3[-2].mag+cx4[-2].mag+cx2p[107]+cx2p[89]+cx2p[71]+(*(cx2p-19))+*(cx2p-37))+4*(cx1[-1].mag+cx0[-2].mag+cx2p[17])+2*(cx1[-2].mag+cx1[3].mag+cx0[-3].mag+cx3->mag+*(cx2p-1)+cx2p[53]);
  band_lo = blk->band_lo;
  blk->ctx_w[0].sel = (run4<1840)+(run4<272);
  band_hi = blk->band_hi;
  blk->ctx_w[1].sel = (run4-cx1->val<=band_hi)+(run4-cx1->val<band_lo);
  d_run4 = run4-cx0[-1].val;
  in_band = d_run4<=band_hi;
  blk->ctx_w[2].sel = in_band+(d_run4<band_lo);
  blk->ctx_w[3].sel = cx1->sign;
  blk->ctx_w[4].sel = cx0[-1].sign;
  magsum = magsum_s;
  magu = cx2[0].mag+cx1->mag;
  run_s2 = run;
  magl = cx0[-1].mag+cx0[-2].mag;
  if( refa ) {
    mag = rb0->mag;
    mag_ref1 = rb1->mag+ra1->mag;
    mag_ref0 = mag+ra0->mag;
    magsum = mag_ref1+magsum_s+4*mag_ref0+2*(ra0[-1].mag+rb0[-1].mag);
    flat_a = mag_ref0+magl+rb0[-2].mag+rb0[-1].mag+ra0[-2].mag+ra0[-1].mag;
    run_s2 = run;
    if( blk->has_ref ) {
      d_run4b = run4-cx1->dval-cx0[0].dval;
      if( d_run4b<band_lo||d_run4b>band_hi ) {
        run4 = run4-cx0[-1].dval-cx0[0].dval;
        flat_b = run4>=band_lo&&band_hi>=run4;
      } else {
        flat_b = 1;
      }
    } else {
      flat_b = mag_ref1+mag_ref0+magu+rb2->mag+ra2->mag;
    }
    if( blk->plane_idx==1 ) {
      magsum_s = magsum;
      d_ra_ra1 = ra0->val-ra1->val;
      blk->ctx_w[3].sel = (d_ra_ra1<=band_hi)+(d_ra_ra1<band_lo);
      d_ra_left = ra0->val-ra0[-1].val;
      magsum = magsum_s;
      run_s2 = run;
      blk->ctx_w[4].sel = (d_ra_left<=band_hi)+(d_ra_left<band_lo);
    } else if( *(int32_t*)&blk->plane_idx>1 ) {
      magsum_s = magsum;
      d_rb_rb1 = rb0->val-rb1->val;
      blk->ctx_w[3].sel = (d_rb_rb1<=band_hi)+(d_rb_rb1<band_lo);
      d_rb_left = rb0->val-rb0[-1].val;
      in_band = d_rb_left<=band_hi;
      magsum = magsum_s;
      blk->ctx_w[4].sel = in_band+(d_rb_left<band_lo);
      ra0_val = ra0->val;
      d_ra_ra1b = ra0_val-((P2Ctx*)(ra1))->val;
      in_band = band_lo<=d_ra_ra1b;
      run_s2 = run;
      if( in_band&&d_ra_ra1b<=band_hi ) {
        flat_a = 1;
      } else {
        d_ra_leftb = ra0_val-ra0[-1].val;
        flat_a = d_ra_leftb>=band_lo&&d_ra_leftb<=band_hi;
      }
    }
  } else {
    flat_a = magl+cx0[-3].mag+cx0[-4].mag+cx0[-5].mag;
    flat_b = cx4->mag+cx3->mag+magu+cx0[0].mag;
  }
  if( magsum>=960 ) {
    ctx15 = 15;
    blk->ctx = 15;
  } else {
    ctx15 = blk->nb_ctx[magsum>>3];
    blk->ctx = ctx15;
  }
  ctx_idx = (run_s2+pred4+7)>>4;
  if( ctx_idx>=255 )
    ctx_idx = 255;
  if( ctx_idx<0 )
    ctx_idx = 0;
  blk->ctx_pair[0] = blk->ctx_delta[ctx_idx+4]+ctx15;
  blk->ctx_pair[1] = ctx15+blk->ctx_delta[ctx_idx];
  blk->ctx = ctx15+32*(flat_b==0)+16*(flat_a==0)+blk->ctx_w[4].w[blk->ctx_w[4].sel]+blk->ctx_w[3].w[blk->ctx_w[3].sel]+blk->ctx_w[2].w[blk->ctx_w[2].sel]+blk->ctx_w[1].w[blk->ctx_w[1].sel]+blk->ctx_w[0].w[blk->ctx_w[0].sel];
  return ctx_idx;
}
