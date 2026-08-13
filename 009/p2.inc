// p2.inc -- the deep-plane model around its block: the filter, and the two
// directions of its eight-bit body.

int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], CtxWeights* w, int32_t mode) {
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
      mixed[j][k] = mix[0]*w->f0[0][j][k];
      for( i = 1; i<6; i++ )
        mixed[j][k] += mix[i]*w->f0[i][j][k];
    }
  for( k = 0; k<4; k++ ) {
    acc[k] = mixed[0][k]*a2[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += mixed[j][k]*a2[j][k];
  }
  prediction = bmf_hsum4(acc)+centre;
  a2[7][2] = prediction;
  if( *(uint32_t*)&_this[15][0] ) {
    for( k = 0; k<4; k++ ) {
      acc[k] = _this[0][k]*a2[0][k];
      for( j = 1; j<7; j++ )
        acc[k] += _this[j][k]*a2[j][k];
    }
    own = centre+bmf_hsum4(acc);
    a2[7][1] = own;
    return (int32_t)(prediction+((own-prediction)*_this[14][0])/_this[14][1]);
  }
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      _this[j][k] = mixed[j][k]*bmf_p2_decay;
      _this[j+7][k] = w->f0[0][j+7][k]*bmf_p2_seed;
    }
  _this[14][0] = 47.0f;
  _this[14][1] = 169.2f;
  _this[14][2] = 1.0f;
  a2[7][1] = prediction;
  return (int32_t)prediction;
}

void __alt_p2_d8_decode_body(AltP2Block* blk, int8_t unread_flag, uint8_t* out, int32_t width, int32_t height) {
  P2Ctx* pix, * prev, * rec, * buf3, * b4, * buf2, * buf1, * b0;
  int32_t x;
  uint32_t y;
  uint8_t* row;
  int32_t code;
  float (**cur)[4], (**r1)[4];
  bool more;
  int16_t val;
  float (*last)[4], (**r0)[4];
  int32_t wid, q, pred, val2;
  int64_t err;
  uint32_t j;
  __rc_begin_decode(unread_flag);
  wid = width;
  pix = blk->cursor[0];
  prev = pix;
  if( width>0 ) {
    x = 0;
    while( 1 ) {
      q = pix[-1].val>>4;
      blk->ctx_pair[0] = blk->ctx+(blk->ctx_delta[q+4]);
      blk->ctx_pair[1] = blk->ctx+(blk->ctx_delta[q]);
      val = (uint8_t)(((uint16_t)blk->cursor[0][-1].val>>4)+(uint8_t)blk->unfold[blk->freq[blk->ctx+blk->ctx_w[3].w[(prev[-1].val<=prev[-2].val)+(prev[-1].val<prev[-2].val)]+blk->ctx_w[2].w[prev[-2].sign]+blk->ctx_w[1].w[prev[-1].sign]+blk->ctx_w[0].w[(((uint32_t)(q-115)>>31)+((uint32_t)(q-17)>>31))]+blk->ctx_w[4].w[1]].decode_symbol(blk->ctx_pair)]);
      *out = val;
      val *= 16;
      blk->cursor[0]->val = val;
      ++out;
      blk->cursor[0]->dval = val;
      rec = blk->cursor[0];
      err = (int16_t)(rec->val-rec[-1].val);
      rec->err = err;
      LOWORD(err) = (WORD2(err)^err)-WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup>>1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err<=0)+(blk->cursor[0]->err<0);
      pix = (blk->cursor[0]+1);
      more = x+1<width;
      blk->cursor[0] = pix;
      ++x;
      if( !more )
        break;
      prev = blk->cursor[0];
    }
    wid = width;
  }
  {
    P2Ctx*const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  P2Ctx*const rec1 = blk->cursor[0]+(-wid);
  rec1[-1] = rec1[0];
  rec1[-2] = rec1[1];
  rec1[-3] = rec1[2];
  rec1[-4] = rec1[3];
  rec1[-5] = rec1[4];
  rec1[-6] = rec1[5];
  rec1[-7] = rec1[6];
  rec1[-8] = rec1[7];
  memcpy(blk->buf[1], blk->buf[0], 18*wid+234);
  memcpy(blk->buf[2], blk->buf[0], 18*wid+234);
  memcpy(blk->buf[3], blk->buf[0], 18*wid+234);
  if( height>1 ) {
    y = 0;
    do {
      cur = blk->cur;
      last = cur[-1];
      row = out;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      r0 += 2;
      blk->cur = r1;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;
      blk->bias[1] = 0.0f;
      blk->bias[2] = 0.0f;
      blk->bias[3] = 0.0f;
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      {
        P2Ctx*const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+8;
      blk->cursor[3] = buf2+8;
      blk->cursor[4] = buf3+8;
      ((P2Ctx*)b4)[-1] = ((P2Ctx*)b0)[0];
      {
        P2Ctx*const here = blk->cursor[0];
        P2Ctx*const up = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if( width>0 ) {
        for( j = 0; j<(uint32_t)width; ++j ) {
          pred = __alt_p2_context((AltP2Block*)blk, (AltP2Block*)nullptr, (AltP2Block*)nullptr);
          code = blk->freq[blk->ctx].decode_symbol(blk->ctx_pair);
          val2 = (uint8_t)(pred+(blk->unfold[code]));
          row[j] = val2;
          __alt_p2_model(blk, val2, code, val2-pred);
          out = &row[j+1];
        }
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  __rc_end_decode();
}

void __alt_p2_d8_encode_body(AltP2Block* blk, uint8_t* src, int32_t width, int32_t height, uint8_t* out) {
  int32_t val;
  P2Ctx* pix, * rec, * buf3, * b4, * buf2, * buf1, * b0;
  int32_t j;
  uint32_t y;
  uint8_t* p;
  int32_t code0;
  uint8_t* outp, want, * srcp, code2;
  uint32_t* pair;
  float (**cur)[4], (**r1)[4];
  uint8_t recon, recon2;
  float (*last)[4], (**r0)[4];
  int32_t q, resid, drift, q2, row_bytes, pred, resid2, drift2, code;
  int64_t err;
  P2Ctx* rec2;
  uint32_t k;
  uint8_t* src_end;
  src_end = src;
  __rc_begin_encode();
  pix = blk->cursor[0];
  if( width>0 ) {
    p = src;
    for( j = 0; j<width; ++j ) {
      q = (uint16_t)pix[-1].val>>4;
      resid = (uint8_t)(*p-q);
      code0 = blk->fold[resid];
      recon = blk->unfold[code0]+q;
      drift = (uint8_t)*out-(uint8_t)(recon+*out-*p);
      if( drift<-16||drift>16 ) {
        *out = *p;
        code0 = blk->fold_hi[resid];
      } else {
        *out = recon;
      }
      rec = blk->cursor[0];
      q2 = rec[-1].val>>4;
      blk->ctx_pair[0] = blk->ctx+blk->ctx_delta[q2+4];
      blk->ctx_pair[1] = blk->ctx+blk->ctx_delta[q2];
      blk->freq[blk->ctx+blk->ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+blk->ctx_w[2].w[rec[-2].sign]+blk->ctx_w[1].w[rec[-1].sign]+blk->ctx_w[0].w[((uint32_t)(q2-115)>>31)+((uint32_t)(q2-17)>>31)]+blk->ctx_w[4].w[1]].encode_symbol(blk->ctx_pair, code0);
      ++p;
      val = 16*(uint8_t)*out;
      blk->cursor[0]->val = val;
      blk->cursor[0]->dval = val;
      ++out;
      rec2 = blk->cursor[0];
      err = (int16_t)(rec2->val-rec2[-1].val);
      rec2->err = err;
      LOWORD(err) = (WORD2(err)^err)-WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup>>1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err<=0)+(blk->cursor[0]->err<0);
      pix = (blk->cursor[0]+1);
      blk->cursor[0] = pix;
    }
    src_end = p;
  }
  {
    P2Ctx*const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  P2Ctx*const rec3 = blk->cursor[0]+(-width);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  row_bytes = 18*width+234;
  memcpy(blk->buf[1], blk->buf[0], row_bytes);
  memcpy(blk->buf[2], blk->buf[0], row_bytes);
  memcpy(blk->buf[3], blk->buf[0], row_bytes);
  if( height>1 ) {
    pair = blk->ctx_pair;
    outp = out;
    y = 0;
    do {
      cur = blk->cur;
      last = cur[-1];
      srcp = src_end;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      blk->cur = r1;
      r0 += 2;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;
      blk->bias[1] = 0.0f;
      blk->bias[2] = 0.0f;
      blk->bias[3] = 0.0f;
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      {
        P2Ctx*const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+8;
      blk->cursor[3] = buf2+8;
      blk->cursor[4] = buf3+8;
      ((P2Ctx*)b4)[-1] = ((P2Ctx*)b0)[0];
      {
        P2Ctx*const here = blk->cursor[0];
        P2Ctx*const up = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if( width>0 ) {
        for( k = 0; k<(uint32_t)width; ++k ) {
          pred = __alt_p2_context((AltP2Block*)blk, (AltP2Block*)nullptr, (AltP2Block*)nullptr);
          want = srcp[k];
          resid2 = (uint8_t)(want-pred);
          recon2 = pred+blk->unfold[blk->fold[resid2]];
          drift2 = (uint8_t)outp[k]-(uint8_t)(recon2+outp[k]-want);
          code = blk->fold[resid2];
          if( drift2<-16||drift2>16 ) {
            outp[k] = want;
            code = blk->fold_hi[(uint8_t)(want-pred)];
          } else {
            outp[k] = recon2;
          }
          code2 = code;
          blk->freq[blk->ctx].encode_symbol(pair, code);
          __alt_p2_model((AltP2Block*)blk, (uint8_t)outp[k], code2, (uint8_t)outp[k]-pred);
        }
        src_end = &srcp[width];
        outp = &outp[width];
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  __rc_end_encode();
}

// One block allocated, one body run, one block freed.  Only the middle line
// differs, and the two bodies take their arguments in different orders --
// which is the decompiler's record of two call sites, not two intentions.
template<int f_DEC>
void __alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  AltP2Block* raw, * blk;
  raw = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
  if( raw )
    blk = raw->alt_p2_alloc(i, 0);
  else
    blk = nullptr;
  if( f_DEC )
    __alt_p2_d8_decode_body(blk, i, out, i, height);
  else
    __alt_p2_d8_encode_body(blk, src, i, height, out);
  if( blk )
    blk->alt_p2_free(1);
}

void __alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  __alt_model_p2_d8<0>(src, i, height, out);
}

void __alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
  __alt_model_p2_d8<1>(nullptr, i, height, out);
}
