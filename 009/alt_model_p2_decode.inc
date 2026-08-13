// alt_model_p2_decode.inc -- deep planes, decoded.

int32_t __alt_model_p2_decode(uint16_t* p_i, uint8_t* out) {
  P2Ctx* cur0, * cur1, * cur2, * cur3, * buf3, * bb4, * buf2, * buf1, * bb0;
  int32_t xf4, val3;
  AltP2Block* plane[4], ** w;
  uint32_t row_bytes, i_1, first, i_2, np;
  int32_t back;
  AltP2Block* blk_r, * src3, * blk_k, * blk2, * blk3, * blk1;
  int16_t seed1;
  uint32_t i;
  int16_t seed3, seed2;
  P2Freq* freq;
  AltP2Block* blk0;
  uint32_t src2;
  AltP2Block** planep;
  float saved_p2_coef[7][4];
  float (**cur)[4], (**r1)[4];
  bool xf3, off;
  float (*b4)[4], (**b0)[4];
  int32_t pl2, dc_flag, xf0, raw, pl, nplanes, xf1, xf2, pred0, code0, val0, pred1, code1, val1, l7a, l4a, l5a, pred2, code2, val2, l7b, l4b, l5b, pred3, code3, l7c, l4c, l5c, nplanes2, pl3;
  void* made, * src1, ** plane_p;
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_coef[0][k] += bmf_p2_coef[4][k];
    bmf_p2_coef[1][k] += bmf_p2_coef[5][k];
    bmf_p2_coef[2][k] += bmf_p2_coef[6][k];
    bmf_p2_coef[4][k] = 0;
    bmf_p2_coef[5][k] = 0;
    bmf_p2_coef[6][k] = 0;
  }
  float saved_p2_rate[3][4];
  __builtin_memcpy(saved_p2_rate, bmf_p2_rate[4], sizeof saved_p2_rate);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_rate[4][k] = bmf_p2_rate_reset;
    bmf_p2_rate[5][k] = bmf_p2_rate_reset;
    bmf_p2_rate[6][k] = bmf_p2_rate_reset;
  }
  i = *p_i;
  raw = p_i[1];
  if( plane_count>0 ) {
    pl = 0;
    do {
      made = bmf_page_alloc(sizeof(AltP2Block));
      if( made )
        src1 = ((AltP2Block*)made)->alt_p2_alloc(i, pl);
      else
        src1 = nullptr;
      plane[pl++] = (AltP2Block*)src1;
    } while( pl<plane_count );
  }
  xf0 = plane_desc[plane_desc[2].src_plane+1].flags&8;
  dc_flag = plane_desc[plane_desc[3].src_plane+1].flags&8;
  xf4 = (plane_desc[plane_desc[4].src_plane+1].flags)&8;
  __rc_begin_decode(dc_flag);
  if( raw>0 ) {
    src2 = 0;
    back = -i;
    nplanes = plane_count;
    row_bytes = 18*i+234;
    np = i+13;
    do {
      if( nplanes>0 ) {
        first = src2;
        pl2 = 0;
        i_1 = i;
        do {
          ++pl2;
          if( first ) {
            planep = &plane[pl2];
            if( first==1 ) {
              blk_k = (AltP2Block*)(*(planep-1));
              {
                P2Ctx*const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              P2Ctx*const rec2 = blk_k->cursor[0]+back;
              rec2[-1] = rec2[0];
              rec2[-2] = rec2[1];
              rec2[-3] = rec2[2];
              rec2[-4] = rec2[3];
              rec2[-5] = rec2[4];
              rec2[-6] = rec2[5];
              rec2[-7] = rec2[6];
              rec2[-8] = rec2[7];
              memcpy(blk_k->buf[1], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[2], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[3], blk_k->buf[0], row_bytes);
            }
          } else {
            src3 = (AltP2Block*)(plane[pl2-1]);
            w = &plane[pl2];
            xf1 = 0;
            do {
              xf2 = xf1;
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
            memcpy(src3->buf[1], src3->buf[0], row_bytes);
            memcpy(src3->buf[2], src3->buf[0], row_bytes);
            memcpy(src3->buf[3], src3->buf[0], row_bytes);
            src3->cursor[0] = src3->buf[0]+i+8;
            src3->cursor[1] = src3->buf[1]+i+8;
            src3->cursor[2] = src3->buf[2]+i+8;
            src3->cursor[3] = src3->buf[3]+i+8;
            src3->cursor[4] = src3->buf[4]+i+8;
          }
          blk_r = (AltP2Block*)(*(planep-1));
          cur = blk_r->cur;
          b4 = cur[-1];
          cur[1] = b4;
          *blk_r->cur = b4;
          b0 = blk_r->row0;
          r1 = blk_r->row1;
          blk_r->row0 = r1;
          blk_r->row1 = b0;
          r1 += 2;
          blk_r->cur = r1;
          b0 += 2;
          blk_r->above = b0;
          r1[-1] = *b0;
          blk_r->cur[-2] = *b0;
          memset(blk_r->p2_row0_head, 0, sizeof blk_r->p2_row0_head);
          blk_r->bias[0] = 0.0f;
          blk_r->bias[1] = 0.0f;
          blk_r->bias[2] = 0.0f;
          blk_r->bias[3] = 0.0f;
          __builtin_memset(blk_r->p2_row, 0, sizeof blk_r->p2_row);
          {
            P2Ctx*const here = blk_r->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
          }
          buf3 = blk_r->buf[3];
          bb4 = blk_r->buf[4];
          buf2 = blk_r->buf[2];
          buf1 = blk_r->buf[1];
          bb0 = blk_r->buf[0];
          blk_r->buf[4] = buf3;
          blk_r->buf[3] = buf2;
          blk_r->buf[2] = buf1;
          blk_r->buf[0] = bb4;
          blk_r->buf[1] = bb0;
          bb4 += 8;
          blk_r->cursor[0] = bb4;
          bb0 += 8;
          blk_r->cursor[1] = bb0;
          blk_r->cursor[2] = buf1+8;
          blk_r->cursor[3] = buf2+8;
          blk_r->cursor[4] = buf3+8;
          ((P2Ctx*)bb4)[-1] = ((P2Ctx*)bb0)[0];
          {
            P2Ctx*const here = blk_r->cursor[0];
            P2Ctx*const up = blk_r->cursor[1];
            here[-2] = up[1];
            here[-3] = up[2];
            here[-4] = up[3];
            here[-5] = up[4];
            here[-6] = up[5];
            here[-7] = up[6];
            here[-8] = up[7];
          }
          blk_r->cursor[0]->dval = 0;
          nplanes = plane_count;
        } while( plane_count>pl2 );
        src2 = first;
        i = i_1;
      }
      ctx_bias[3] = 0;
      ctx_bias[2] = 0;
      ctx_bias[1] = 0;
      ctx_bias[0] = 0;
      if( i>0 ) {
        first = src2;
        i_2 = 0;
        i_1 = i;
        do {
          ctx_bias[0] >>= 3;
          ctx_bias[1] >>= 3;
          ctx_bias[2] >>= 3;
          ctx_bias[3] >>= 3;
          blk0 = (AltP2Block*)(plane[0]);
          pred0 = __alt_p2_context(plane[0], plane[2], plane[1]);
          freq = &blk0->freq[blk0->ctx];
          code0 = freq->decode_symbol(blk0->ctx_pair);
          val0 = (uint8_t)(pred0+(blk0->unfold[code0]));
          __alt_p2_model(blk0, val0, code0, val0-pred0);
          cur0 = blk0->cursor[0];
          ctx_bias[0] += 32*cur0[-1].dupleft;
          ctx_bias[1] += 32*cur0[-1].dupright;
          ctx_bias[2] += 32*cur0[-1].dleft;
          ctx_bias[3] += 32*cur0[-1].dup;
          off = xf0==0;
          out[plane_desc[1].src_plane] = val0;
          if( off )
            seed1 = 0;
          else
            seed1 = 16*out[plane_desc[1].src_plane];
          blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          pred1 = __alt_p2_context((AltP2Block*)blk1, plane[0], plane[2]);
          code1 = blk1->freq[blk1->ctx].decode_symbol(blk1->ctx_pair);
          val1 = (uint8_t)(pred1+blk1->unfold[code1]);
          __alt_p2_model((AltP2Block*)blk1, val1, code1, val1-pred1);
          cur1 = blk1->cursor[0];
          l7a = cur1[-1].dupright;
          l4a = cur1[-1].dleft;
          ctx_bias[0] += 32*cur1[-1].dupleft;
          l5a = cur1[-1].dup;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          off = dc_flag==0;
          out[plane_desc[2].src_plane] = val1;
          if( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[3].src_plane+1].weight1*out[plane_desc[2].src_plane]+plane_desc[plane_desc[3].src_plane+1].weight0*out[plane_desc[1].src_plane])>>3;
          blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          pred2 = __alt_p2_context((AltP2Block*)blk2, plane[0], plane[1]);
          code2 = blk2->freq[blk2->ctx].decode_symbol(blk2->ctx_pair);
          val2 = (uint8_t)(pred2+blk2->unfold[code2]);
          __alt_p2_model((AltP2Block*)blk2, val2, code2, val2-pred2);
          cur2 = blk2->cursor[0];
          l7b = cur2[-1].dupright;
          l4b = cur2[-1].dleft;
          ctx_bias[0] += 32*cur2[-1].dupleft;
          l5b = cur2[-1].dup;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          out[plane_desc[3].src_plane] = val2;
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( xf4 )
              seed3 = (plane_desc[plane_desc[4].src_plane+1].weight2*out[2]+plane_desc[plane_desc[4].src_plane+1].weight1*out[1]+plane_desc[plane_desc[4].src_plane+1].weight0**out)>>3;
            else
              seed3 = 0;
            blk3 = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = seed3;
            pred3 = __alt_p2_context((AltP2Block*)blk3, plane[2], plane[0]);
            code3 = blk3->freq[blk3->ctx].decode_symbol(blk3->ctx_pair);
            val3 = (uint8_t)(pred3+blk3->unfold[code3]);
            __alt_p2_model((AltP2Block*)blk3, val3, code3, val3-pred3);
            cur3 = blk3->cursor[0];
            l7c = cur3[-1].dupright;
            l4c = cur3[-1].dleft;
            l5c = cur3[-1].dup;
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
  __rc_end_decode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  nplanes2 = plane_count;
  if( plane_count>0 ) {
    pl3 = 0;
    do {
      plane_p = (void**)plane[pl3];
      if( plane_p ) {
        ((AltP2Block*)plane_p)->alt_p2_free(1);
        nplanes2 = plane_count;
      }
      ++pl3;
    } while( pl3<nplanes2 );
  }
  return nplanes2;
}
