// alt_p2_decode.inc -- the predictor-2 model, decoding.

// The mirror of `alt_p2_encode.inc`, minus the near-lossless drift control,
// which has nothing to decide here because the code says which fold was used.
//
// It used to say "minus two things", the second being the coefficient-table
// borrow.  That was wrong: this function did the borrow too, in six lines
// identical to the encoder's, 260 lines from the six that undo them.  Both are
// `P2Coef::fold`/`restore` now, which is also how the claim got checked.
int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
  AltP2Block* plane[4], ** w;
  uint32_t i_1, first;
  int16_t seed1;
  uint32_t i, src2;
  int16_t seed3, seed2;
  AltP2Block** planep;
  bool xf3, off;
  int32_t nplanes;
  P2Coef p2_coef;
  p2_coef.fold();
  i = p_i->width;
  int32_t raw = p_i->height;
  alt_p2_planes_alloc(plane, i);
  int32_t xf0 = plane_desc[plane_desc[1].src_plane].flags&desc_has_refs;
  int32_t dc_flag = plane_desc[plane_desc[2].src_plane].flags&desc_has_refs;
  int32_t xf4 = (plane_desc[plane_desc[3].src_plane].flags)&desc_has_refs;
  rc_begin_decode();
  if( raw>0 ) {
    src2 = 0;
    int32_t back = -i;
    nplanes = plane_count;
    uint32_t row_bytes = 18*i+234;
    uint32_t np = i+13;
    do {
      if( nplanes>0 ) {
        first = src2;
        int32_t pl2 = 0;
        i_1 = i;
        do {
          ++pl2;
          if( first ) {
            planep = &plane[pl2];
            if( first==1 ) {
              AltP2Block *blk_k = (AltP2Block*)(*(planep-1));
              {
                P2Ctx*const here = blk_k->cursor[0];
                for( int32_t j = 0; j<5; ++j ) here[j] = here[-1];
              }
              P2Ctx*const rec2 = blk_k->cursor[0]+back;
              for( int32_t j = 0; j<8; ++j ) rec2[-1-j] = rec2[j];
              for( int32_t j = 0; j<3; ++j ) memcpy(blk_k->buf[1+j], blk_k->buf[0], row_bytes);
            }
          } else {
            AltP2Block *src3 = (AltP2Block*)(plane[pl2-1]);
            w = &plane[pl2];
            int32_t xf1 = 0;
            do {
              int32_t xf2 = xf1;
              src3->buf[0][xf2].val = 256;
              xf3 = (uint32_t)(++xf1)<np;
              src3->buf[0][xf2].dval = 256;
              src3->buf[0][xf2].err = -16;
              src3->buf[0][xf2].sign = 1;
              src3->buf[0][xf2].mag = 3;
              src3->buf[0][xf2].aerr = 512;
              src3->buf[0][xf2].dupright = 512;
              src3->buf[0][xf2].dupleft = 512;
              src3->buf[0][xf2].dup = 1024;
              src3->buf[0][xf2].dleft = 256;
            } while( xf3 );
            planep = w;
            for( int32_t j = 0; j<3; ++j ) memcpy(src3->buf[1+j], src3->buf[0], row_bytes);
            for( int32_t j = 0; j<5; ++j ) src3->cursor[j] = src3->buf[j]+i+8;
          }
          AltP2Block *blk_r = (AltP2Block*)(*(planep-1));
          blk_r->start_row();
          nplanes = plane_count;
        } while( plane_count>pl2 );
        src2 = first;
        i = i_1;
      }
      for( int32_t j = 0; j<4; ++j ) ctx_bias[3-j] = 0;
      if( i>0 ) {
        first = src2;
        uint32_t i_2 = 0;
        i_1 = i;
        do {
          for( int32_t j = 0; j<4; ++j ) ctx_bias[j] >>= 3;
          AltP2Block* blk0 = (AltP2Block*)(plane[0]);
          int32_t pred0 = plane[0]->alt_p2_context(plane[2], plane[1]);
          P2Freq* freq = &blk0->freq[blk0->ctx];
          int32_t code0 = freq->decode_symbol(blk0->ctx_pair);
          int32_t val0 = (uint8_t)(pred0+(blk0->unfold[code0]));
          blk0->alt_p2_model(val0, code0, val0-pred0);
          P2Ctx *cur0 = blk0->cursor[0];
          ctx_bias[0] += 32*cur0[-1].dupleft;
          ctx_bias[1] += 32*cur0[-1].dupright;
          ctx_bias[2] += 32*cur0[-1].dleft;
          ctx_bias[3] += 32*cur0[-1].dup;
          off = xf0==0;
          out[plane_desc[0].src_plane] = val0;
          if( off )
            seed1 = 0;
          else
            seed1 = 16*out[plane_desc[0].src_plane];
          AltP2Block *blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          int32_t pred1 = blk1->alt_p2_context(plane[0], plane[2]);
          int32_t code1 = blk1->freq[blk1->ctx].decode_symbol(blk1->ctx_pair);
          int32_t val1 = (uint8_t)(pred1+blk1->unfold[code1]);
          blk1->alt_p2_model(val1, code1, val1-pred1);
          P2Ctx *cur1 = blk1->cursor[0];
          int32_t l7a = cur1[-1].dupright;
          int32_t l4a = cur1[-1].dleft;
          ctx_bias[0] += 32*cur1[-1].dupleft;
          int32_t l5a = cur1[-1].dup;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          off = dc_flag==0;
          out[plane_desc[1].src_plane] = val1;
          if( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[2].src_plane].weight1*out[plane_desc[1].src_plane]+plane_desc[plane_desc[2].src_plane].weight0*out[plane_desc[0].src_plane])>>3;
          AltP2Block *blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          int32_t pred2 = blk2->alt_p2_context(plane[0], plane[1]);
          int32_t code2 = blk2->freq[blk2->ctx].decode_symbol(blk2->ctx_pair);
          int32_t val2 = (uint8_t)(pred2+blk2->unfold[code2]);
          blk2->alt_p2_model(val2, code2, val2-pred2);
          P2Ctx *cur2 = blk2->cursor[0];
          int32_t l7b = cur2[-1].dupright;
          int32_t l4b = cur2[-1].dleft;
          ctx_bias[0] += 32*cur2[-1].dupleft;
          int32_t l5b = cur2[-1].dup;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          out[plane_desc[2].src_plane] = val2;
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( xf4 )
              seed3 = (plane_desc[plane_desc[3].src_plane].weight2*out[2]+plane_desc[plane_desc[3].src_plane].weight1*out[1]+plane_desc[plane_desc[3].src_plane].weight0**out)>>3;
            else
              seed3 = 0;
            AltP2Block *blk3 = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = seed3;
            int32_t pred3 = blk3->alt_p2_context(plane[2], plane[0]);
            int32_t code3 = blk3->freq[blk3->ctx].decode_symbol(blk3->ctx_pair);
            int32_t val3 = (uint8_t)(pred3+blk3->unfold[code3]);
            blk3->alt_p2_model(val3, code3, val3-pred3);
            P2Ctx *cur3 = blk3->cursor[0];
            int32_t l7c = cur3[-1].dupright;
            int32_t l4c = cur3[-1].dleft;
            int32_t l5c = cur3[-1].dup;
            ctx_bias[0] += 32*cur3[-1].dupleft;
            ctx_bias[1] += 32*l7c;
            ctx_bias[2] += 32*l4c;
            ctx_bias[3] += 32*l5c;
            out[3] = val3;
            nplanes = plane_count;
          }
          out += nplanes;
          ++i_2;
        } while( i_2<i_1 );
        src2 = first;
        i = i_1;
      }
      ++src2;
    } while( src2<(uint32_t)raw );
  }
  rc_end_decode();
  p2_coef.restore();
  return alt_p2_planes_free(plane);
}
