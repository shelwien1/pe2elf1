// alt_p2_encode.inc -- the predictor-2 model, encoding.

// `P2Coef` in alt_p2_block.inc is the coefficient-table borrow both directions
// do for the length of one image.  This file's header used to say the encoder
// did it and the decoder did not; the decoder does, in the same six lines, and
// the two are one object now.
int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
  AltP2Block* plane[4];
  uint32_t xf4, i;
  bool xf2;
  int16_t seed1, l7c, seed2;
  uint32_t src1;
  AltP2Block* blk0, ** xf3;
  NbRow **cur, **r1;
  NbRow *b4, **b0;
  int32_t nplanes, want0, off0, seed0, at0, cur0, code0, cur1, code1, cur2, code2, l5c, seed3, nplanes2;
  uint8_t* out = a2;
  P2Coef p2_coef;
  p2_coef.fold();
  int32_t row_i = p_i->width;
  int32_t height = p_i->height;
  if( plane_count>0 ) {
    int32_t pl = 0;
    do {
      AltP2Block* page = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
      plane[pl] = page ? page->alt_p2_alloc(row_i, pl) : nullptr;
      ++pl;
    } while( pl<plane_count );
  }
  int32_t xf0 = plane_desc[plane_desc[1].src_plane].flags&8;
  int32_t np = plane_desc[plane_desc[2].src_plane].flags&8;
  int32_t y = (plane_desc[plane_desc[3].src_plane].flags)&8;
  rc_begin_encode();
  if( height>0 ) {
    src1 = 0;
    int32_t first = -row_i;
    uint32_t row_bytes = 18*row_i+234;
    uint32_t code3 = row_i+13;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        xf4 = src1;
        int32_t pl2 = 0;
        do {
          ++pl2;
          if( xf4 ) {
            xf3 = &plane[pl2];
            if( xf4==1 ) {
              AltP2Block *blk_k = (AltP2Block*)(*(xf3-1));
              {
                P2Ctx*const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              P2Ctx*const rec4 = blk_k->cursor[0]+first;
              rec4[-1] = rec4[0];
              rec4[-2] = rec4[1];
              rec4[-3] = rec4[2];
              rec4[-4] = rec4[3];
              rec4[-5] = rec4[4];
              rec4[-6] = rec4[5];
              rec4[-7] = rec4[6];
              rec4[-8] = rec4[7];
              memcpy(blk_k->buf[1], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[2], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[3], blk_k->buf[0], row_bytes);
            }
          } else {
            AltP2Block *src2 = (AltP2Block*)(plane[pl2-1]);
            AltP2Block** back = &plane[pl2];
            int32_t src3 = 0;
            do {
              int32_t xf1 = src3;
              src2->buf[0][xf1].val = 256;
              xf2 = (uint32_t)(++src3)<code3;
              src2->buf[0][xf1].dval = 256;
              src2->buf[0][xf1].err = -16;
              src2->buf[0][xf1].sign = 1;
              src2->buf[0][xf1].mag = 3;
              src2->buf[0][xf1].aerr = 512;
              src2->buf[0][xf1].dupright = 512;
              src2->buf[0][xf1].dupleft = 512;
              src2->buf[0][xf1].dup = 1024;
              src2->buf[0][xf1].dleft = 256;
            } while( xf2 );
            xf3 = back;
            memcpy(src2->buf[1], src2->buf[0], row_bytes);
            memcpy(src2->buf[2], src2->buf[0], row_bytes);
            memcpy(src2->buf[3], src2->buf[0], row_bytes);
            src2->cursor[0] = src2->buf[0]+row_i+8;
            src2->cursor[1] = src2->buf[1]+row_i+8;
            src2->cursor[2] = src2->buf[2]+row_i+8;
            src2->cursor[3] = src2->buf[3]+row_i+8;
            src2->cursor[4] = src2->buf[4]+row_i+8;
          }
          AltP2Block *blk_r = (AltP2Block*)(*(xf3-1));
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
          memset(blk_r->p2_row, 0, 15);
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
          P2Ctx *buf3 = blk_r->buf[3];
          P2Ctx *bb4 = blk_r->buf[4];
          P2Ctx *buf2 = blk_r->buf[2];
          P2Ctx *buf1 = blk_r->buf[1];
          P2Ctx *bb0 = blk_r->buf[0];
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
          (bb4)[-1] = (bb0)[0];
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
        src1 = xf4;
      }
      if( row_i<=0 ) {
        ctx_bias[0] = 0;
        ctx_bias[1] = 0;
        ctx_bias[2] = 0;
        ctx_bias[3] = 0;
      } else {
        xf4 = src1;
        want0 = 0;
        off0 = 0;
        seed0 = 0;
        at0 = 0;
        for( i = 0; i<(uint32_t)row_i; ++i ) {
          int32_t off0b = plane_desc[0].src_plane;
          ctx_bias[0] = at0>>3;
          ctx_bias[1] = seed0>>3;
          ctx_bias[2] = off0>>3;
          ctx_bias[3] = want0>>3;
          blk0 = (AltP2Block*)(plane[0]);
          cur0 = out[plane_desc[0].src_plane];
          int32_t pred0 = plane[0]->alt_p2_context(plane[2], plane[1]);
          int32_t resid0 = (uint8_t)(cur0-pred0);
          code0 = (uint8_t)blk0->fold[resid0];
          int32_t out0 = out[off0b];
          int32_t recon0b = (uint8_t)(pred0+(blk0->unfold[code0]));
          int32_t recon0 = (uint8_t)(recon0b+out0-cur0);
          int32_t drift0 = out0-recon0;
          if( drift0<-16||drift0>16 ) {
            code0 = (uint8_t)blk0->fold_hi[resid0];
          } else {
            cur0 = recon0b;
            out[off0b] = recon0;
          }
          blk0->freq[blk0->ctx].encode_symbol(blk0->ctx_pair, code0);
          blk0->alt_p2_model(cur0, code0, cur0-pred0);
          P2Ctx *rec0 = blk0->cursor[0];
          int32_t l7a = rec0[-1].dupright;
          int32_t l4a = rec0[-1].dleft;
          int32_t l5a = rec0[-1].dup;
          ctx_bias[0] += 32*rec0[-1].dupleft;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          if( xf0 )
            seed1 = 16*out[plane_desc[0].src_plane];
          else
            seed1 = 0;
          AltP2Block *blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          int32_t off1 = plane_desc[1].src_plane;
          cur1 = out[plane_desc[1].src_plane];
          int32_t pred1 = blk1->alt_p2_context(plane[0], plane[2]);
          int32_t resid1 = (uint8_t)(cur1-pred1);
          code1 = blk1->fold[resid1];
          int32_t out1 = out[off1];
          int32_t recon1b = (uint8_t)(pred1+blk1->unfold[code1]);
          int32_t recon1 = (uint8_t)(recon1b+out1-cur1);
          int32_t drift1 = out1-recon1;
          if( drift1<-16||drift1>16 ) {
            code1 = blk1->fold_hi[resid1];
          } else {
            cur1 = recon1b;
            out[off1] = recon1;
          }
          blk1->freq[blk1->ctx].encode_symbol(blk1->ctx_pair, code1);
          blk1->alt_p2_model(cur1, code1, cur1-pred1);
          P2Ctx *rec1 = blk1->cursor[0];
          int32_t l7b = rec1[-1].dupright;
          int32_t l4b = rec1[-1].dleft;
          int32_t l5b = rec1[-1].dup;
          ctx_bias[0] += 32*rec1[-1].dupleft;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          if( np )
            seed2 = (plane_desc[plane_desc[2].src_plane].weight1*out[plane_desc[1].src_plane]+plane_desc[plane_desc[2].src_plane].weight0*out[plane_desc[0].src_plane])>>3;
          else
            seed2 = 0;
          AltP2Block *blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          int32_t off2 = plane_desc[2].src_plane;
          cur2 = out[plane_desc[2].src_plane];
          int32_t pred2 = blk2->alt_p2_context(plane[0], plane[1]);
          int32_t resid2 = (uint8_t)(cur2-pred2);
          code2 = blk2->fold[resid2];
          int32_t out2 = out[off2];
          int32_t code2b = (uint8_t)(pred2+blk2->unfold[code2]);
          int32_t recon2 = (uint8_t)(code2b+out2-cur2);
          int32_t drift2 = out2-recon2;
          if( drift2<-16||drift2>16 ) {
            code2 = blk2->fold_hi[resid2];
          } else {
            cur2 = code2b;
            out[off2] = recon2;
          }
          blk2->freq[blk2->ctx].encode_symbol(blk2->ctx_pair, code2);
          blk2->alt_p2_model(cur2, code2, cur2-pred2);
          P2Ctx *rec2 = blk2->cursor[0];
          nplanes = plane_count;
          at0 = ctx_bias[0]+32*rec2[-1].dupleft;
          seed0 = ctx_bias[1]+32*rec2[-1].dupright;
          off0 = ctx_bias[2]+32*rec2[-1].dleft;
          want0 = ctx_bias[3]+32*rec2[-1].dup;
          if( plane_count>=4 ) {
            ctx_bias[0] += 32*rec2[-1].dupleft;
            ctx_bias[1] = seed0;
            ctx_bias[2] = off0;
            ctx_bias[3] = want0;
            if( y )
              l7c = (plane_desc[plane_desc[3].src_plane].weight2*out[2]+plane_desc[plane_desc[3].src_plane].weight1*out[1]+plane_desc[plane_desc[3].src_plane].weight0**out)>>3;
            else
              l7c = 0;
            AltP2Block *l4c = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = l7c;
            int32_t alpha_src = plane_desc[3].src_plane;
            l5c = out[plane_desc[3].src_plane];
            int32_t x = l4c->alt_p2_context(plane[2], plane[0]);
            int32_t w = (uint8_t)(l5c-x);
            seed3 = l4c->fold[w];
            int32_t blk3 = out[alpha_src];
            int32_t cur3 = (uint8_t)(x+l4c->unfold[seed3]+blk3-l5c);
            int32_t drift3 = blk3-cur3;
            if( drift3<-16||drift3>16 ) {
              seed3 = l4c->fold_hi[w];
            } else {
              l5c = (uint8_t)(x+l4c->unfold[seed3]);
              out[alpha_src] = cur3;
            }
            l4c->freq[l4c->ctx].encode_symbol(l4c->ctx_pair, seed3);
            l4c->alt_p2_model(l5c, seed3, l5c-x);
            P2Ctx *rec3 = l4c->cursor[0];
            nplanes = plane_count;
            at0 = ctx_bias[0]+32*rec3[-1].dupleft;
            seed0 = ctx_bias[1]+32*rec3[-1].dupright;
            off0 = ctx_bias[2]+32*rec3[-1].dleft;
            want0 = ctx_bias[3]+32*rec3[-1].dup;
          }
          out += nplanes;
        }
        src1 = xf4;
        ctx_bias[0] = at0;
        ctx_bias[1] = seed0;
        ctx_bias[2] = off0;
        ctx_bias[3] = want0;
      }
      ++src1;
    } while( src1<(uint32_t)height );
  }
  rc_end_encode();
  p2_coef.restore();
  nplanes2 = plane_count;
  if( plane_count>0 ) {
    int32_t pl3 = 0;
    do {
      if( plane[pl3] ) {
        plane[pl3]->alt_p2_free(1);
        nplanes2 = plane_count;
      }
      ++pl3;
    } while( pl3<nplanes2 );
  }
  return nplanes2;
}
