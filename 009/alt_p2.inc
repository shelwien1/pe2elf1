// alt_p2.inc -- the predictor-2 model around its block: the filter, and the two
// directions of its eight-bit body.

int32_t __alt_p2_filter(NbRow* _this, float (*a2)[4], CtxWeights* w, int32_t mode) {
  const float* mix = bmf_p2_mix[mode];
  float acc[4], mixed[7][4], centre, prediction, own;
  int32_t i, j, k;
  for( k = 0; k<4; k++ ) {
    acc[k] = bmf_p2_coef[0][k]*a2[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += bmf_p2_coef[j][k]*a2[j][k];
  }
  centre = bmf_hsum4(acc);
  a2[7][0] = centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ )
      a2[j][k] -= centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      mixed[j][k] = mix[0]*w->f0[0]->w[j][k];
      for( i = 1; i<6; i++ )
        mixed[j][k] += mix[i]*w->f0[i]->w[j][k];
    }
  for( k = 0; k<4; k++ ) {
    acc[k] = mixed[0][k]*a2[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += mixed[j][k]*a2[j][k];
  }
  prediction = bmf_hsum4(acc)+centre;
  a2[7][2] = prediction;
  if( _this->uses ) {
    for( k = 0; k<4; k++ ) {
      acc[k] = _this->w[0][k]*a2[0][k];
      for( j = 1; j<7; j++ )
        acc[k] += _this->w[j][k]*a2[j][k];
    }
    own = centre+bmf_hsum4(acc);
    a2[7][1] = own;
    return (int32_t)(prediction+((own-prediction)*_this->w[14][0])/_this->w[14][1]);
  }
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      _this->w[j][k] = mixed[j][k]*bmf_p2_decay;
      _this->w[j+7][k] = w->f0[0]->w[j+7][k]*bmf_p2_seed;
    }
  _this->w[14][0] = 47.0f;
  _this->w[14][1] = 169.2f;
  _this->w[14][2] = 1.0f;
  a2[7][1] = prediction;
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
template<int f_DEC>
void AltP2Block::alt_p2_d8_body(uint8_t* src, uint8_t* out,
                                int32_t width, int32_t height) {
  P2Ctx *pix;
  int32_t j;
  int32_t code0;
  uint8_t *outp, code2;
  NbRow **cur_at, **r1;
  NbRow *last, **r0;
  int32_t code;
  uint32_t k;
  uint8_t* src_end;
  src_end = src;
  if( f_DEC )
    __rc_begin_decode();
  else
    __rc_begin_encode();
  pix = cursor[0];
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
        int32_t q = (uint16_t)rec[-1].val>>4;
        int32_t resid = (uint8_t)(*p-q);
        code0 = fold[resid];
        uint8_t recon = unfold[code0]+q;
        int32_t drift = (uint8_t)*out-(uint8_t)(recon+*out-*p);
        if( drift<-16||drift>16 ) {
          *out = *p;
          code0 = fold_hi[resid];
        } else {
          *out = recon;
        }
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
  {
    P2Ctx*const here = cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  P2Ctx*const rec3 = cursor[0]+(-width);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  int32_t row_bytes = 18*width+234;
  memcpy(buf[1], buf[0], row_bytes);
  memcpy(buf[2], buf[0], row_bytes);
  memcpy(buf[3], buf[0], row_bytes);
  if( height>1 ) {
    uint32_t* pair = ctx_pair;
    outp = out;
    uint32_t y = 0;
    do {
      cur_at = cur;
      last = cur_at[-1];
      uint8_t *srcp = src_end;
      cur_at[1] = last;
      *cur = last;
      r0 = row0;
      r1 = row1;
      row0 = r1;
      row1 = r0;
      r1 += 2;
      cur = r1;
      r0 += 2;
      above = r0;
      r1[-1] = *r0;
      cur[-2] = *r0;
      memset(p2_row, 0, 15);
      bias[0] = 0.0f;
      bias[1] = 0.0f;
      bias[2] = 0.0f;
      bias[3] = 0.0f;
      __builtin_memset(p2_row, 0, sizeof p2_row);
      {
        P2Ctx*const here = cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      ring_advance(buf, cursor, 8);
      cursor[0][-1] = cursor[1][0];
      {
        P2Ctx*const here = cursor[0];
        P2Ctx*const up = cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      cursor[0]->dval = 0;
      if( width>0 ) {
        for( k = 0; k<(uint32_t)width; ++k ) {
          int32_t pred = alt_p2_context((AltP2Block*)nullptr, (AltP2Block*)nullptr);
          if( f_DEC ) {
            code2 = code = freq[ctx].decode_symbol(pair);
            outp[k] = (uint8_t)(pred+(unfold[code]));
          } else {
            uint8_t want = srcp[k];
            int32_t resid2 = (uint8_t)(want-pred);
            uint8_t recon2 = pred+unfold[fold[resid2]];
            int32_t drift2 = (uint8_t)outp[k]-(uint8_t)(recon2+outp[k]-want);
            code = fold[resid2];
            if( drift2<-16||drift2>16 ) {
              outp[k] = want;
              code = fold_hi[(uint8_t)(want-pred)];
            } else {
              outp[k] = recon2;
            }
            code2 = code;
            freq[ctx].encode_symbol(pair, code);
          }
          alt_p2_model((uint8_t)outp[k], code2, (uint8_t)outp[k]-pred);
        }
        if( !f_DEC )
          src_end = &srcp[width];
        outp = &outp[width];
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  if( f_DEC )
    __rc_end_decode();
  else
    __rc_end_encode();
}

// One block allocated, one body run, one block freed.  Only the middle line
// differs, and the two bodies take their arguments in different orders --
// which is the decompiler's record of two call sites, not two intentions.
template<int f_DEC>
void __alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
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

void __alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  __alt_model_p2_d8<0>(src, i, height, out);
}

void __alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
  __alt_model_p2_d8<1>(nullptr, i, height, out);
}
