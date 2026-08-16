// alt_p2.inc -- the predictor-2 model around its block: the filter, and the two
// directions of its eight-bit body.

// Predict one sample from the seven neighbourhood vectors, and answer with
// three numbers rather than one.  `nb[7][0..2]` receive the centre, this row's
// own prediction and the mixed one; the return value is what the coder uses.
// That third row is one past the `[7][4]` the caller passes -- see the
// `offsetof` assertion beside `p2_row` in alt_p2_block.inc, which is what pins
// the two together.
//
// The first call on a row has no weights of its own to predict with, so it
// seeds them from the mixture instead and returns the mixed prediction
// unchanged.  `uses` is what tells the two apart.
inline int32_t NbRow::predict(float (*nb)[4], CtxWeights* sets, int32_t mode) {
  const float* mix = bmf_p2_mix[mode];
  float acc[4], mixed[7][4], centre, prediction, own;
  int32_t i, j, k;
  for( k = 0; k<4; k++ ) {
    acc[k] = bmf_p2_coef[0][k]*nb[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += bmf_p2_coef[j][k]*nb[j][k];
  }
  centre = bmf_hsum4(acc);
  nb[7][0] = centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ )
      nb[j][k] -= centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      mixed[j][k] = mix[0]*sets->row[0]->w[j][k];
      for( i = 1; i<6; i++ )
        mixed[j][k] += mix[i]*sets->row[i]->w[j][k];
    }
  for( k = 0; k<4; k++ ) {
    acc[k] = mixed[0][k]*nb[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += mixed[j][k]*nb[j][k];
  }
  prediction = bmf_hsum4(acc)+centre;
  nb[7][2] = prediction;
  if( uses ) {
    for( k = 0; k<4; k++ ) {
      acc[k] = w[0][k]*nb[0][k];
      for( j = 1; j<7; j++ )
        acc[k] += w[j][k]*nb[j][k];
    }
    own = centre+bmf_hsum4(acc);
    nb[7][1] = own;
    return (int32_t)(prediction+((own-prediction)*w[14][0])/w[14][1]);
  }
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      w[j][k] = mixed[j][k]*bmf_p2_decay;
      w[j+7][k] = sets->row[0]->w[j+7][k]*bmf_p2_seed;
    }
  w[14][0] = 47.0f;
  w[14][1] = 169.2f;
  w[14][2] = 1.0f;
  nb[7][1] = prediction;
  return (int32_t)prediction;
}

// One plane of eight-bit samples, coded either way.  Of the two bodies this
// replaces, about ninety lines were the same: the first row's record update,
// the mirrors written past both ends of the row, the three memcpys that seed
// the history buffers, and the whole of the per-row rotation -- five buffers
// and two weight rows moved round, the biases cleared, eight records copied
// down from the row above.  None of that has a direction.
//
// Both loops index off `blk->cursor[0]`, which is what lets them share the
// context: the encoder called it `rec` and the decoder `prev`, and the
// decoder's `pix[-1]` and the encoder's `rec[-1]` are the same record.  So the
// quantiser, the context pair and the frequency record are computed once, for
// both, and only the symbol is one-sided.
//
// What is genuinely one-sided is the near-lossless drift control.  The encoder
// folds the residual, reconstructs it, and if the reconstruction has drifted
// more than sixteen from the source it sends the sample through `fold_hi`
// instead and writes the original byte out.  The decoder has none of that --
// it is told which code was used -- and that asymmetry is the reason these two
// were written separately in the first place.
template<int32_t f_DEC>
void AltP2Block::alt_p2_d8_body(uint8_t* src, uint8_t* out,
                                int32_t width, int32_t height) {
  int32_t j, code0, code;
  uint8_t *outp, code2;
  uint32_t k;
  uint8_t *src_end = src;
  if( f_DEC )
    rc_begin_decode();
  else
    rc_begin_encode();
  P2Ctx *pix = cursor[0];
  if( width>0 ) {
    uint8_t* p = src;
    for( j = 0; j<width; ++j ) {
      P2Ctx *rec = cursor[0];
      int32_t q2 = rec[-1].val>>4;
      ctx_pair[0] = ctx+ctx_delta[q2+4];
      ctx_pair[1] = ctx+ctx_delta[q2];
      P2Freq&fr = freq[ctx+ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+ctx_w[2].w[rec[-2].sign]+ctx_w[1].w[rec[-1].sign]+ctx_w[0].w[(q2<115)+(q2<17)]+ctx_w[4].w[1]];
      if( f_DEC ) {
        *out = (uint8_t)(((uint16_t)rec[-1].val>>4)+(uint8_t)unfold[fr.decode_symbol(ctx_pair)]);
      } else {
        code0 = fold_or_refuse(out, *p, (uint16_t)rec[-1].val>>4, fold,
                               fold_hi, unfold);
        fr.encode_symbol(ctx_pair, code0);
        ++p;
      }
      int32_t val = 16*(uint8_t)*out;
      cursor[0]->val = val;
      cursor[0]->dval = val;
      ++out;
      int32_t err = (int16_t)(rec->val-rec[-1].val);
      rec->err = err;
      // The signed difference goes to `err`, its magnitude to the four
      // difference fields.  It arrived as a partial store -- the magnitude
      // written over `err`'s low word after `rec->err` had taken the signed
      // value -- and every one of the five destinations is sixteen bits wide,
      // so the upper half the store preserved was never read.
      int32_t adiff = abs32(err);
      cursor[0]->aerr = adiff;
      cursor[0]->dupright = adiff;
      cursor[0]->dupleft = adiff;
      cursor[0]->dup = adiff;
      cursor[0]->dleft = (uint32_t)cursor[0]->dup>>1;
      cursor[0]->mag = 2;
      cursor[0]->sign = (cursor[0]->err<=0)+(cursor[0]->err<0);
      pix = cursor[0]+1;
      cursor[0] = pix;
    }
    if( !f_DEC )
      src_end = p;
  }
  seed_history(width);
  if( height>1 ) {
    uint32_t* pair = ctx_pair;
    outp = out;
    uint32_t y = 0;
    do {
      uint8_t *srcp = src_end;
      start_row();
      if( width>0 ) {
        for( k = 0; k<(uint32_t)width; ++k ) {
          int32_t pred = alt_p2_context(nullptr, nullptr);
          if( f_DEC ) {
            code2 = code = freq[ctx].decode_symbol(pair);
            outp[k] = (uint8_t)(pred+(unfold[code]));
          } else {
            code = fold_or_refuse(&outp[k], srcp[k], pred, fold, fold_hi,
                                  unfold);
            code2 = code;
            freq[ctx].encode_symbol(pair, code);
          }
          alt_p2_model(outp[k], code2, outp[k]-pred);
        }
        if( !f_DEC )
          src_end = &srcp[width];
        outp = &outp[width];
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  if( f_DEC )
    rc_end_decode();
  else
    rc_end_encode();
}

// One block allocated, one body run, one block freed.  Only the middle line
// differs, and the two bodies take their arguments in different orders --
// which is the decompiler's record of two call sites, not two intentions.
template<int32_t f_DEC>
void alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  AltP2Block *blk;
  AltP2Block *raw = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
  if( raw )
    blk = raw->alt_p2_alloc(i, 0);
  else
    blk = nullptr;
  if( f_DEC )
    blk->alt_p2_d8_body<1>(nullptr, out, i, height);
  else
    blk->alt_p2_d8_body<0>(src, out, i, height);
  if( blk )
    blk->alt_p2_free(1);
}

void alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  alt_model_p2_d8<0>(src, i, height, out);
}

void alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
  alt_model_p2_d8<1>(nullptr, i, height, out);
}
