// alt_p2_context.inc -- the predictor-2 model's context: which of its counters the next
// sample is coded against.

// The two low fields of every bank context: the running prediction quantised
// against three thresholds into bits 13-14, and the activity sum against three
// more into bits 11-12.  Six of these, and the thresholds are the model --
// what is not the model is the shape around them, written out six times.
//
// Two of the six spelled the sum's three tests `(uint32_t)(K-sum)>>31`, which
// is the sign bit of `K-sum`, which is `sum > K`.  The same predicate as the
// other four, in the form MSVC left when it compiled a comparison one way here
// and another way there.  The identity is exact for the values these take --
// `sum_all` is a sum of magnitudes and `K` is at most 58, so `K-sum` cannot
// overflow -- and it was checked against the other spelling before the two
// were written as one.
static inline int32_t ctx_quant(int32_t run, int32_t r0, int32_t r1, int32_t r2,
                                int32_t sum, int32_t s0, int32_t s1, int32_t s2) {
  return (((run>r0)+(run>r1)+(run>r2))<<13)|(((sum>s0)+(sum>s1)+(sum>s2))<<11);
}

int32_t AltP2Block::alt_p2_context(AltP2Block* refa, AltP2Block* refb) {
  CtxWeights weights;
  int32_t ctx15;
  P2Ctx *ra1, *ra0, *rb1, *rb0, *rb2, *ra2, *row2c;
  int32_t sum_all;
  NbRow** up_row;
  int32_t nb_slot;
  P2Ctx* cx2;
  P2Ctx *nb0;
  NbRow* wrow;
  bool in_band, no_ref;
  P2Ctx *nb1;
  NbRow** cur_l;
  int32_t in63, c_lo, mode, c_mid, ctx0, ctx1, pred1, run1, ctx2, run2, bank3, run3, bank4, magsum, run_s2, flat_a, flat_b, ctx_idx;
  uint32_t ctx0_lo;
  int32_t run, run4;
  int32_t magsum_s;
  P2Ctx *row0p = cursor[0];
  P2Ctx *cursor1 = cursor[1];
  P2Ctx *sub0_row = (P2Ctx*)cursor[2];
  int32_t sum_ul = 21*sub0_row[-1].dupleft+12*cursor1[3].dupleft+16*cursor1[2].dupleft+22*cursor1[1].dupleft+(23*row0p[-1].dupleft)+20*cursor1->dupleft+ctx_bias[0]+14*row0p[-2].dupleft;
  int32_t sum_ur = 17*sub0_row[-2].dupright+21*cursor1[2].dupright+15*cursor1[1].dupright+25*cursor1->dupright+9*cursor1[-1].dupright+22*row0p[-1].dupright+ctx_bias[1]+19*row0p[-2].dupright;
  int32_t sum4 = 17*cursor1[3].dleft+15*cursor1[2].dleft+21*cursor1[1].dleft+18*cursor1->dleft+16*cursor1[-1].dleft+22*row0p[-1].dleft+ctx_bias[2]+19*row0p[-2].dleft;
  int32_t lane5 = sub0_row->dup;
  int32_t sum_u = 14*cursor1[3].dup+23*cursor1[1].dup+19*cursor1->dup+25*row0p[-1].dup+ctx_bias[3]+17*row0p[-2].dup+15*(cursor[3]->dup+lane5);
  sum_all = sum4+sum_u+sum_ul+sum_ur;
  int32_t band = (8*sum4>43*sum_u)+(8*sum4>17*sum_u)+(8*sum4>9*sum_u)+(8*sum4>5*sum_u)+(8*sum4>2*sum_u);
  int32_t den_d = 2*row0p[-2].val+2*row0p[-1].val;
  int32_t num_b = cursor1[1].val+(cursor1->val+2*row0p[-1].val);
  int32_t num_d = 16*(sub0_row[2].val+sub0_row->val+cursor1[1].val+sub0_row[-1].val);
  int32_t gA = (sum_all>bmf_p2_thresholds[band][12])+(sum_all>bmf_p2_thresholds[band][11])+(sum_all>bmf_p2_thresholds[band][10])+(sum_all>bmf_p2_thresholds[band][9]);
  int32_t gB = (num_b>bmf_p2_thresholds[band][8])+(num_b>bmf_p2_thresholds[band][7])+(num_b>bmf_p2_thresholds[band][6]);
  int32_t gC = (16*sum_ur>sum_ul*bmf_p2_thresholds[band][5])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][4])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][3]);
  int32_t gD = (num_d>bmf_p2_thresholds[band][2]*den_d)+(num_d>den_d*bmf_p2_thresholds[band][1])+(num_d>den_d*bmf_p2_thresholds[band][0]);
  // Mixed radix, and only three of the five terms are bit fields.  `gB`, `gC`
  // and `gD` each sum three comparisons, so each is 0..3 and each is two bits.
  // `gA` sums *four*, so it is 0..4 -- five values with a stride of 64 -- and
  // `band` sums five, so it is 0..5 with a stride of 320, which is 5 * 64.
  // Neither is a field however it is written: `bits<6, 2>(gA)` was tried, and
  // the one neighbourhood in the corpus that reaches `gA == 4` landed in slot
  // 0 instead of 256.  Three streams moved and the gate said so.
  //
  // So they are digits, with their radices written down, and `320 * 6 = 1920`
  // is what `nb_id[]` is sized for.
  nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                    .digit<64, 5>(gA).digit<320, 6>(band);
  int32_t nb_at = nb_id[(uint32_t)nb_slot];
  if( nb_id[(uint32_t)nb_slot] ) {
    wrow = &nb_weights[nb_at];
    nb_cur = wrow;
    {
      float acc[4], err, floor_;
      int32_t j, k;
      for( k = 0; k<4; ++k ) {
        acc[k] = wrow->w[0][k]*p2_row[0][k];
        for( j = 1; j<7; ++j )
          acc[k] += wrow->w[j][k]*p2_row[j][k];
      }
      err = ((float)row0p[-1].val-(bmf_hsum4(acc)+bias[0]))*2.0999999f;
      floor_ = 7744.0f*wrow->w[14][2];
      for( j = 0; j<7; ++j )
        for( k = 0; k<4; ++k ) {
          float x = p2_row[j][k];
          float ms = wrow->w[7+j][k]+(x*x-wrow->w[7+j][k])*bmf_p2_ms_rate;
          wrow->w[7+j][k] = ms;
          wrow->w[j][k] += bmf_p2_rate[j][k]*err*x/(ms+floor_);
        }
    }
  } else {
    int32_t next_id = nb_id_used;
    nb_id_used = ++next_id;
    nb_id[(uint32_t)nb_slot] = next_id;
    nb_cur = &nb_weights[(int16_t)next_id];
  }
  // The filter's 28 inputs, four to a row.  Rows 0..3 are differences of `dval`
  // -- the second difference of the plane -- at widening offsets from the
  // current pixel: row 0 the four nearest, row 1 the next ring, rows 2 and 3
  // the ones two and three records out.  Rows 4..6 are the cross-plane terms
  // and are only filled when this plane has references; without them the
  // filter sees this plane alone.
  //
  // Written out one subscript at a time because that is what the original did,
  // and left that way: the offsets are not a pattern a loop could generate --
  // `nb0[-1] + nb1->dval - nb1[-1]` and `nb1[-2] + nb1->dval - nb2[-2]` are
  // different shapes -- so a table of offsets would be 28 rows of data plus a
  // loop, which is the same information one indirection further away.
  nb1 = (P2Ctx*)cursor[1];
  p2_row[0][0] = (float)nb1->dval;
  nb0 = (P2Ctx*)cursor[0];
  p2_row[0][1] = (float)nb1[1].dval;
  p2_row[0][2] = (float)(nb0[-1].dval+nb1->dval-nb1[-1].dval);
  row2c = (P2Ctx*)cursor[2];
  p2_row[0][3] = (float)(nb0[-2].dval+(nb1->dval-nb1[-2].dval));
  P2Ctx *nb3 = (P2Ctx*)cursor[3];
  p2_row[1][0] = (float)(nb1[-1].dval+nb1->dval-row2c[-1].dval);
  p2_row[1][1] = (float)(-3*(row2c->dval-nb1->dval)+nb3->dval);
  p2_row[1][2] = (float)(nb0[-1].dval+nb1[2].dval-nb1[1].dval);
  p2_row[1][3] = (float)(nb0[-2].dval+nb1[1].dval-nb1[-1].dval);
  p2_row[2][0] = (float)(2*nb0[-1].dval-nb0[-2].dval);
  P2Ctx *nb2 = (P2Ctx*)(row2c);
  p2_row[2][1] = (float)(nb0[-3].dval+nb1->dval-nb1[-3].dval);
  p2_row[2][2] = (float)(nb2->dval+nb1[1].dval-nb3[1].dval);
  p2_row[2][3] = (float)nb0[-3].dval;
  p2_row[3][0] = (float)(nb1[-2].dval+nb1->dval-nb2[-2].dval);
  p2_row[3][1] = (float)(nb0[-2].dval+nb1[-1].dval-nb1[-3].dval);
  p2_row[3][2] = (float)(nb1[1].dval+((nb1[2].dval+nb1->dval)>>1)-nb2[2].dval);
  p2_row[3][3] = (float)nb3->dval;
  if( refa ) {
    ra0 = (P2Ctx*)refa->cursor[0]-1;
    ra1 = (P2Ctx*)((int16_t*)(refa->cursor[1]-1));
    ra2 = (P2Ctx*)((int16_t*)(refa->cursor[2]-1));
    rb0 = (P2Ctx*)refb->cursor[0]-1;
    no_ref = has_ref==0;
    rb1 = refb->cursor[1]-1;
    rb2 = refb->cursor[2]-1;
    if( !no_ref ) {
      {
        float bias_l = (float)nb0->dval;
        int32_t j, k;
        for( j = 0; j<4; ++j )
          for( k = 0; k<4; ++k )
            p2_row[j][k] += bias_l;
      }
      nb0 = (P2Ctx*)cursor[0];
      nb1 = (P2Ctx*)cursor[1];
    }
    int32_t plane = plane_idx;
    if( plane ) {
      if( plane==1 ) {
        p2_row[4][0] = (float)(nb0->dval+nb1[3].dval);
        p2_row[4][1] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        p2_row[4][2] = (float)(nb1[1].val+ra0[-1].val-ra1->val);
        p2_row[4][3] = (float)(nb0[-1].val+ra1->val-ra1[-1].val);
        p2_row[5][0] = (float)(nb1[1].val+(rb1[2].val-((P2Ctx*)(rb2))[3].val));
        p2_row[5][1] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        p2_row[5][2] = (float)(nb1->val+rb0->val-rb1->val);
        no_ref = has_ref==0;
        p2_row[5][3] = (float)(nb0[-2].val+ra0->err);
        if( no_ref ) {
          int16_t *nb2w = (int16_t*)cursor[2];
          p2_row[6][0] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
          p2_row[6][1] = (float)(*nb2w+ra0->val-((P2Ctx*)(ra2))->val);
          p2_row[6][2] = (float)(ra2->val+ra0->val-*nb2w+2*(nb1->val-ra1->val));
          in63 = *(int16_t*)((uint8_t*)nb0-18)+nb1[-1].val+ra1[-2].val+ra0->val-ra0[-1].val-ra1[-1].val-nb1[-2].val;
        } else {
          p2_row[6][0] = (float)nb1->val;
          p2_row[6][1] = (float)nb0[-3].val;
          p2_row[6][2] = (float)(nb0[-1].val+ra0[-1].val-ra0[-2].val);
          in63 = nb0[-3].val+nb0[-1].val-nb0[-4].val;
        }
        p2_row[6][3] = (float)in63;
      } else {
        p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
        p2_row[4][1] = (float)(nb0[-1].val+ra0[-1].val-ra0[-2].val);
        p2_row[4][2] = (float)(nb1[1].val+ra1->val-ra2[1].val);
        p2_row[4][3] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        p2_row[5][0] = (float)(nb1->val+ra0[-2].val-ra1[-2].val);
        p2_row[5][1] = (float)(nb1->val+ra0->val-ra1->val);
        p2_row[5][2] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
        int16_t *nb2w2 = (int16_t*)cursor[2];
        p2_row[5][3] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        p2_row[6][0] = (float)(*nb2w2+rb0->val-((P2Ctx*)(rb2))->val);
        p2_row[6][1] = (float)(rb0[-2].val+rb0->val-nb0[-2].val+2*(nb0[-1].val-rb0[-1].val));
        p2_row[6][2] = (float)(nb0[-2].val+rb0->err);
        p2_row[6][3] = (float)(*nb2w2+ra0->err);
      }
    } else {
      p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
      P2Ctx *nb3x = cursor[3];
      p2_row[4][1] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
      P2Ctx *nb2x = (P2Ctx*)cursor[2];
      p2_row[4][2] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
      p2_row[4][3] = (float)((nb2x->val+3*nb1[1].val-4*nb2x[1].val)-(((nb1[2].val-nb1->val-(nb3x[2].val-nb3x->val))>>1)-nb3x[1].val));
      p2_row[5][0] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
      p2_row[5][1] = (float)(nb1[1].val+ra1[1].val-ra2[2].val);
      p2_row[5][2] = (float)(nb1[-2].val+ra1[2].val-ra2->val);
      p2_row[5][3] = (float)((ra0[-2].val+ra0->val-nb0[-2].val)+2*(nb0[-1].val-ra0[-1].val));
      p2_row[6][0] = (float)(nb2x[1].val+rb0->val-rb2[1].val);
      p2_row[6][1] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
      p2_row[6][2] = (float)(nb0[-1].val+rb1[1].val-rb1->val);
      p2_row[6][3] = (float)(nb1[1].val+rb1[2].val-rb2[3].val);
    }
  } else {
    p2_row[4][0] = (float)(nb1[3].val+nb1->val-nb2[3].val);
    row2c = nb2;
    p2_row[4][1] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
    p2_row[4][2] = (float)((nb2->val+3*nb1[1].val-4*nb2[1].val)-((((nb1[2].val-nb1->val)-(nb3[2].val-nb3->val))>>1)-nb3[1].val));
    P2Ctx* nb4 = (P2Ctx*)cursor[4];
    p2_row[4][3] = (float)(row2c[-1].val+nb1[1].val-nb3->val);
    p2_row[5][0] = (float)(nb3[1].val+nb1->val-nb4[1].val);
    p2_row[5][1] = (float)nb1[3].val;
    p2_row[5][2] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
    p2_row[5][3] = (float)(nb0[-1].val+nb3->val-nb3[-1].val);
    p2_row[6][0] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
    p2_row[6][1] = (float)nb4->val;
    p2_row[6][2] = (float)(nb0[-5].val+nb0[-1].val-nb0[-6].val);
    p2_row[6][3] = (float)nb1[-6].val;
    rb2 = (P2Ctx*)(nullptr);
    rb1 = (P2Ctx*)(nullptr);
    rb0 = nullptr;
    ra2 = (P2Ctx*)(nullptr);
    ra1 = (P2Ctx*)(nullptr);
    ra0 = nullptr;
  }
  cur_l = cur;
  weights.f0[0] = cur_l[-1];
  up_row = above;
  weights.f0[1] = up_row[1];
  weights.f0[2] = up_row[2];
  weights.f0[3] = cur_l[-2];
  weights.f0[4] = up_row[0];
  weights.f0[5] = cur_l[0];
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
  int32_t filt = __alt_p2_filter(nb_cur, (float (*)[4]) p2_row, &weights, mode);
  P2Ctx* cx0 = (P2Ctx*)cursor[0];
  P2Ctx *cx1 = (P2Ctx*)cursor[1];
  // A 32-bit store at `&pred_prev`, which is a `uint16_t` followed by two
  // bytes of padding: the original wrote the whole word and the top half
  // landed in the pad, which nothing reads.  So it is the low half.
  pred_prev = (uint16_t)filt;
  int32_t lane3 = cx1->aerr;
  int32_t g3pair = cx0[-1].aerr+lane3;
  if( refa )
    g3pair += (rb0->aerr+ra0->aerr)>>1;
  run = filt;
  P2Ctx *cx3 = (P2Ctx*)(cursor[3]);
  int32_t g3sum = cx1[3].aerr+cx1[-1].aerr+cx0[-1].aerr+cx0[-3].aerr+cx0[-4].aerr+cx0[-2].aerr;
  int32_t run_s = run;
  // Four of the five masks are a sign test the types prove: a sum of `w` of
  // these `int16_t` members is under `w * 2^15`, so every bit from
  // `15 + ceil(log2 w)` up is a copy of the sign.  The fifth is a sign too, and
  // for a reason from outside that rule -- the cast makes it bit 15 of a
  // *sixteen-bit* value -- so it stays as it was written.  tools/ctxidx.py.
  ctx0_lo = CtxIdx{}.bit<19>(cx1[1].err+cx1->err+cx1[-1].err+cx0[-1].err < 0)
            .bit<18>(cx3->err+cx0[-2].err+2*cx0[-4].err < 0)
            .bit<17>(cx1[-3].err+cx0[-3].err+cx0[-5].err+cx0[-7].err < 0)
            .bit<16>(cx0[-1].err < 0)
            .raw(((uint16_t)cx0[-3].err)&0x8000)
            .bits<13, 2>((run>3536)+(run>720)+(run>288))
            .bits<11, 2>(((g3pair+g3sum)>752)+((g3pair+g3sum)>400)+((g3pair+g3sum)>240));
  bank_ctx[0] = ctx0_lo;
  if( refa ) {
    int32_t lane2 = ra0->err;
    run = run_s;
    ctx0 = CtxIdx{}.bit<25>(rb0[-1].err < 0)
           .bit<24>(rb0->err < 0)
           .bit<23>(ra0[-1].err < 0)
           .bit<22>(rb0->err+rb0[-2].err < 0)
           .bit<21>(ra0[-2].err+lane2 < 0)
           .bit<20>(cx0[-2].err < 0)
           .raw(ctx0_lo);
    cx2 = (P2Ctx*)cursor[2];
  } else {
    run = run_s;
    cx2 = (P2Ctx*)cursor[2];
    ctx0 = CtxIdx{}.bit<25>(cx1[3].err+cx0[-3].err+cx0[-7].err+cx0[-5].err < 0)
           .bit<24>(cx2[1].err+cx2->err+cx0[-4].err+cx0[-8].err < 0)
           .bit<23>(cx0[-4].err+cx0[-6].err < 0)
           .bit<22>(cx0[-4].err < 0)
           .bit<21>(cx0[-5].err < 0)
           .bit<20>(cx0[-7].err < 0)
           .raw(ctx0_lo);
  }
  int32_t bank0 = ctx0>>11;
  bank_ctx[0] = bank0;
  int32_t pred0 = p2_pred(p2_ctr[bank0].weighted, p2_ctr[bank0].rate);
  int32_t sum_c = sum_all;
  P2Ctx *cx4 = cursor[4];
  nb_sum[1] = pred0;
  int32_t run0 = pred0+run_s;
  nb_sum[0] = run0;
  int32_t nb4_4 = cx4[4].val;
  sum_all = ((g3pair<<9)+sum_c)>>13;
  int32_t d_run0 = cx0[-5].val-run0;
  int32_t d_up = nb4_4-run0;
  int32_t up5 = cx1[5].val;
  int32_t d_up5 = up5-run0;
  // The four words below -- two spellings of `ctx1`, two of `ctx2`, and the two
  // `bank` words after them -- keep their masks, and this is the measurement
  // that says they must.
  //
  // Every `expr & <one bit>` here is a sign test *if* `|expr|` stays under that
  // bit, and `tools/ctxidx.py` converts the ones where the types say so: a sum
  // of `w` `int16_t` members is under `w * 2^15`.  These are not those.  Every
  // term carries a `run` -- `run0 = pred0 + run_s`, `run1 = pred1 + run0`, and
  // on down -- which is an accumulated prediction with no type bound, and the
  // masks reach as low as bit 15.
  //
  // Instrumented over the corpus, four of them are *not* sign tests at all:
  //
  //   ctx1  bit 15  `d_run0`                        reaches 36157
  //   bank3 bit 15  `run2-cx0[-3].val`              reaches 35550
  //   bank3 bit 24  `... - ((uint32_t)(...)>>1) ...`  is unsigned throughout
  //   bank4 bit 15  `(uint16_t)run3 - ...`          reaches 69572 as an int
  //
  // and twelve more come within a factor of four of their bit, which makes
  // them sign tests by how the corpus happened to run and not by anything the
  // code says.  Rewriting any of these as `< 0` would change what the program
  // computes.  `tools/ctxidx.py --line N` reports; it declines all six.
  if( refa ) {
    int32_t dv_now = cx0[0].dval;
    ctx1 = (cx0[-1].val+ra0->val-ra0[-1].val-run0)&0x2000000             // bit 25
         |(ra2->dval+ra1->dval-2*ra0->dval)&0x1000000                    // bit 24
         |((dv_now+cx0[-3].dval-run0-(cx1[2].dval-cx1[5].dval))&0x800000 // bit 23
         |(dv_now+cx0[-5].dval-run0)&0x400000                            // bit 22
         |(dv_now+cx1->dval+rb1[2].dval-rb2[2].dval-run0)&0x200000       // bit 21
         |(dv_now+cx0[-1].dval+rb0->dval-rb0[-1].dval-run0)&0x100000     // bit 20
         |(cx2[-1].dval+dv_now+ra0->dval-ra2[-1].dval-run0)&0x80000      // bit 19
         |(dv_now+cx1[2].dval+ra0->dval-ra1[2].dval-run0)&0x40000        // bit 18
         |d_up5&0x20000                                                  // bit 17
         |d_up&0x10000                                                   // bit 16
         |d_run0&0x8000                                                  // bit 15
         |ctx_quant(run0, 2256, 1056, 144, sum_all, 55, 10, 24));
  } else {
    int32_t d_run0b = run0-cx0[-3].val;
    ctx1 = (cx1[-5].val-cx1[-2].val+d_run0b)&0x1000000                   // bit 24
         |(d_run0b+cx1[2].val-up5)&0x800000                              // bit 23
         |(cx4[3].val-cx2[2].val+run0-cx2[1].val)&0x400000               // bit 22
         |(cx4[-1].val-cx3->val+run0-cx1[-1].val)&0x200000               // bit 21
         |(cx1->val-cx1[2].val+run0-cx0[-2].val)&0x100000                // bit 20
         | -d_up5&0x80000                                                // bit 19
         | -d_up&0x40000                                                 // bit 18
         |(run0-cx4[2].val)&0x20000                                      // bit 17
         |(run0-cx4[-3].val)&0x10000                                     // bit 16
         | -d_run0&0x8000                                                // bit 15
         |ctx_quant(run0, 2400, 1024, 240, sum_all, 39, 24, 11)|(nb4_4-cx3->val+run0-cx1[4].val)&0x2000000;   // bit 25
  }
  int32_t bank1 = ctx1>>11;
  bank_ctx[1] = bank1;
  int8_t rate1 = p2_ctr[bank1+32768].rate;
  int32_t w1c = p2_ctr[bank1+32768].weighted;
  int32_t one1 = 1<<((rate1+31)&31);
  pred1 = (one1+w1c)>>(rate1&31);
  run1 = pred1+run0;
  nb_sum[2] = run1;
  nb_sum[3] = pred1;
  int32_t cx2_val0 = cx2->val;
  int32_t cx1_val = cx1->val;
  int32_t dvsum2 = cx0[-2].val-run1;
  int32_t lap = cx0[-2].val+run1-2*cx0[-1].val;
  int32_t dtop2 = cx2_val0-run1;
  if( refa ) {
    int32_t dv1 = cx1[1].dval;
    int32_t d_ra = ra0->val-run1;
    ctx2 = (cx0[0].dval+cx1[-1].dval-run1-(cx2->dval-dv1))&0x2000000     // bit 25
         |(cx0[0].dval+cx0[-1].dval-run1-(cx1->dval-dv1))&0x1000000      // bit 24
         |(rb0->val-run1+cx2_val0-rb2->val)&0x800000                     // bit 23
         |(d_ra+cx2_val0-ra2->val)&0x400000                              // bit 22
         |(d_ra+cx1[-1].val-ra1[-1].val)&0x200000                        // bit 21
         |((cx0[0].dval-cx0[-2].dval+2*cx0[-1].dval-run1)&0x100000       // bit 20
         |(2*cx1_val-run1-cx2_val0)&0x80000                              // bit 19
         | -lap&0x40000                                                  // bit 18
         |dtop2&0x20000                                                  // bit 17
         |dvsum2&0x10000                                                 // bit 16
         |(208-rb0->val)&0x8000                                          // bit 15
         |ctx_quant(run1, 2576, 1280, 640, sum_all, 33, 12, 4));
  } else {
    ctx2 = (run1+3*(cx2_val0-cx1_val)-cx3->val)&0x2000000                // bit 25
         |(run1+cx4->val-(cx2[2].val+cx2[-2].val))&0x1000000             // bit 24
         |(cx3[2].val-cx2[-1].val+run1-cx1[3].val)&0x800000              // bit 23
         |(cx3[1].val-cx1[1].val-dtop2)&0x400000                         // bit 22
         |(cx3->val-cx1_val-dtop2)&0x200000                              // bit 21
         |lap&0x100000                                                   // bit 20
         |(run1-cx4[3].val)&0x80000                                      // bit 19
         |(cx3[1].val-run1)&0x40000                                      // bit 18
         |(run1-cx4->val)&0x20000                                        // bit 17
         |(run1-cx3[-2].val)&0x10000                                     // bit 16
         | -dvsum2&0x8000                                                // bit 15
         |ctx_quant(run1, 2464, 1216, 688, sum_all, 58, 25, 13);
  }
  int32_t bank2 = ctx2>>11;
  bank_ctx[2] = bank2;
  int32_t pred2 = p2_pred(p2_ctr[bank2+65536].weighted, p2_ctr[bank2+65536].rate);
  nb_sum[5] = pred2;
  run2 = pred2+run1;
  nb_sum[4] = run2;
  int32_t run_dv3 = run2-cx0[0].dval;
  // `sum_all = -sum_all` was here, and the six thresholds below read `< -K`
  // because of it.  Nothing else used the negated value: it existed so that
  // `q9`, `q39` and `q10` -- computed from the positive one, before this line
  // -- could be added to comparisons made after it.  Those are written out
  // now, so the negation and the six sign flips cancel.
  bank3 = (int32_t)((3*(cx0[-2].val-cx0[-1].val)+run2-cx0[-3].val)&0x2000000 // bit 25
       |(cx2[1].dval-((uint32_t)(cx1[1].dval+cx1[2].dval+cx1[-1].dval+cx1->dval)>>1)+run_dv3)&0x1000000 // bit 24
       |(cx2[-3].val-cx1[-2].val+run2-cx1[-1].val)&0x800000              // bit 23
       | -(cx2[2].val+run2-2*cx1[1].val)&0x400000                        // bit 22
       |(cx1[-2].dval-cx0[-2].val+run2-cx1->dval)&0x200000               // bit 21
       |(run2-cx1[3].val)&0x100000                                       // bit 20
       |(run_dv3-cx2[1].dval)&0x80000                                    // bit 19
       |(run2-cx3->val)&0x40000                                          // bit 18
       |(2*run2-cx1->dval-(cx1->val+cx0[0].dval))&0x20000                // bit 17
       |(run2-cx0[-1].dval-cx0[0].dval)&0x10000                          // bit 16
       |(run2-cx0[-3].val)&0x8000                                        // bit 15
       |ctx_quant(run2, 2896, 1568, 592, sum_all, 37, 19, 9))>>11;
  bank_ctx[3] = bank3;
  int32_t w3c = p2_ctr[bank3+98304].weighted;
  int32_t pred3 = p2_pred(w3c, p2_ctr[bank3+98304].rate);
  nb_sum[7] = pred3;
  run3 = pred3+run2;
  run = run3;
  nb_sum[6] = run3;
  int32_t dv_now4 = cx0[0].dval;
  int32_t run_dv4 = run3-dv_now4;
  int32_t run_up4 = run3+cx3->val;
  bank4 = (int32_t)((run-((uint32_t)(cx1[1].val+cx1[-1].val+2*cx1->val)>>2))&0x2000000 // bit 25
       |(run_up4-cx0[-2].val-(cx3[2].dval+dv_now4))&0x1000000            // bit 24
       |(run-2*cx0[-2].dval-(dv_now4-cx0[-4].dval))&0x800000             // bit 23
       |(run-cx0[-3].dval-dv_now4-(cx1->val-cx1[-3].val))&0x400000       // bit 22
       |(run_up4-dv_now4-(cx1[1].val+cx2[-1].dval))&0x200000             // bit 21
       |(cx2[-2].val+run-2*cx1[-1].val)&0x100000                         // bit 20
       |(cx2->dval-2*cx1->dval+run_dv4)&0x80000                          // bit 19
       |((run3-cx0[-1].val)-(cx1->val-cx1[-1].val))&0x40000              // bit 18
       |(run_dv4-cx1[4].dval)&0x20000                                    // bit 17
       |(run_dv4-cx1[-2].dval)&0x10000                                   // bit 16
       |(((uint16_t)run3-(uint16_t)cx0[-4].dval-(uint16_t)dv_now4)&0x8000 // bit 15
       )|(ctx_quant(run3, 3056, 1952, 368, sum_all, 39, 21, 10)))>>11;
  bank_ctx[4] = bank4;
  int32_t pred4 = p2_pred(p2_ctr[bank4+131072].weighted, p2_ctr[bank4+131072].rate);
  uint8_t* cx2p = (uint8_t*)cx2;
  nb_sum[9] = pred4;
  run4 = run+pred4;
  nb_sum[8] = run+pred4;
  magsum_s = cx1[5].mag+cx1[4].mag+cx1[-3].mag+cx1[-4].mag+3*(cx1[2].mag+cx2[1].mag)+7*cx1->mag+6*cx1[1].mag+cx0[-6].mag+cx0[-7].mag+cx0[-8].mag+8*cx0[-1].mag+cx4[2].mag+cx4[1].mag+cx4->mag+cx4[-1].mag+cx0[-4].mag+cx0[-5].mag+(cx3[2].mag+cx3[1].mag+cx3[-1].mag+cx3[-2].mag+cx4[-2].mag+cx2p[107]+cx2p[89]+cx2p[71]+(*(cx2p-19))+*(cx2p-37))+4*(cx1[-1].mag+cx0[-2].mag+cx2p[17])+2*(cx1[-2].mag+cx1[3].mag+cx0[-3].mag+cx3->mag+*(cx2p-1)+cx2p[53]);
  int32_t lo_band = band_lo;
  ctx_w[0].sel = (run4<1840)+(run4<272);
  int32_t hi_band = band_hi;
  ctx_w[1].sel = (run4-cx1->val<=hi_band)+(run4-cx1->val<lo_band);
  int32_t d_run4 = run4-cx0[-1].val;
  in_band = d_run4<=hi_band;
  ctx_w[2].sel = in_band+(d_run4<lo_band);
  ctx_w[3].sel = cx1->sign;
  ctx_w[4].sel = cx0[-1].sign;
  magsum = magsum_s;
  int32_t magu = cx2[0].mag+cx1->mag;
  run_s2 = run;
  int32_t magl = cx0[-1].mag+cx0[-2].mag;
  if( refa ) {
    int32_t mag = rb0->mag;
    int32_t mag_ref1 = rb1->mag+ra1->mag;
    int32_t mag_ref0 = mag+ra0->mag;
    magsum = mag_ref1+magsum_s+4*mag_ref0+2*(ra0[-1].mag+rb0[-1].mag);
    flat_a = mag_ref0+magl+rb0[-2].mag+rb0[-1].mag+ra0[-2].mag+ra0[-1].mag;
    run_s2 = run;
    if( has_ref ) {
      int32_t d_run4b = run4-cx1->dval-cx0[0].dval;
      if( d_run4b<lo_band||d_run4b>hi_band ) {
        run4 = run4-cx0[-1].dval-cx0[0].dval;
        flat_b = run4>=lo_band&&hi_band>=run4;
      } else {
        flat_b = 1;
      }
    } else {
      flat_b = mag_ref1+mag_ref0+magu+rb2->mag+ra2->mag;
    }
    if( plane_idx==1 ) {
      magsum_s = magsum;
      int32_t d_ra_ra1 = ra0->val-ra1->val;
      ctx_w[3].sel = (d_ra_ra1<=hi_band)+(d_ra_ra1<lo_band);
      int32_t d_ra_left = ra0->val-ra0[-1].val;
      magsum = magsum_s;
      run_s2 = run;
      ctx_w[4].sel = (d_ra_left<=hi_band)+(d_ra_left<lo_band);
    } else if( *(int32_t*)&plane_idx>1 ) {
      magsum_s = magsum;
      int32_t d_rb_rb1 = rb0->val-rb1->val;
      ctx_w[3].sel = (d_rb_rb1<=hi_band)+(d_rb_rb1<lo_band);
      int32_t d_rb_left = rb0->val-rb0[-1].val;
      in_band = d_rb_left<=hi_band;
      magsum = magsum_s;
      ctx_w[4].sel = in_band+(d_rb_left<lo_band);
      int32_t ra0_val = ra0->val;
      int32_t d_ra_ra1b = ra0_val-((P2Ctx*)(ra1))->val;
      in_band = lo_band<=d_ra_ra1b;
      run_s2 = run;
      if( in_band&&d_ra_ra1b<=hi_band ) {
        flat_a = 1;
      } else {
        int32_t d_ra_leftb = ra0_val-ra0[-1].val;
        flat_a = d_ra_leftb>=lo_band&&d_ra_leftb<=hi_band;
      }
    }
  } else {
    flat_a = magl+cx0[-3].mag+cx0[-4].mag+cx0[-5].mag;
    flat_b = cx4->mag+cx3->mag+magu+cx0[0].mag;
  }
  if( magsum>=960 ) {
    ctx15 = 15;
    ctx = 15;
  } else {
    ctx15 = nb_ctx[magsum>>3];
    ctx = ctx15;
  }
  ctx_idx = (run_s2+pred4+7)>>4;
  if( ctx_idx>=255 )
    ctx_idx = 255;
  if( ctx_idx<0 )
    ctx_idx = 0;
  ctx_pair[0] = ctx_delta[ctx_idx+4]+ctx15;
  ctx_pair[1] = ctx15+ctx_delta[ctx_idx];
  ctx = ctx15+32*(flat_b==0)+16*(flat_a==0)+ctx_w[4].w[ctx_w[4].sel]+ctx_w[3].w[ctx_w[3].sel]+ctx_w[2].w[ctx_w[2].sel]+ctx_w[1].w[ctx_w[1].sel]+ctx_w[0].w[ctx_w[0].sel];
  return ctx_idx;
}
