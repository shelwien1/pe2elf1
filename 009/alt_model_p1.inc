// alt_model_p1.inc -- one-bit planes, both directions.
//
// The two bodies this replaces were 263 and 212 lines and shared 134 of them,
// in three runs that carry most of the function: the header parse and the
// per-plane block allocation, the 57 lines of per-row rotation -- five buffers
// moved round, five cursors reseated, the two context sums rebuilt from the
// rows above -- and the teardown.  None of that has a direction.
//
// `buf` is the encoder's `src` and the decoder's `out`: the same walk over the
// interleaved planes, read in one direction and written in the other.
//
// What is one-sided is the near-lossless drift control and the side the colour
// transform falls on.  The encoder takes a sample, subtracts the transform,
// folds the residual, reconstructs it, and if the reconstruction has drifted
// more than sixteen it re-sends through `fold_hi` and leaves the original byte
// in `buf`.  The decoder is told which code was used, and adds the transform
// back after the fact -- which is why its stores sit one plane behind, between
// a plane's cursor advance and the next plane's context.
//
// Two spellings differed and are the same value.  The encoder wrote a plane's
// magnitude as `(BYTE4(err) ^ err) - BYTE4(err)` over an `int64_t` and the
// decoder as `abs32(val - pred)`; `P1Ctx::mag` is a `uint8_t`, and the two
// agree in the low eight bits, which is all of them that are kept.  And the
// decoder held `blk->ctx[0]` in a local across the `ctx[2]` update, which
// cannot change it -- they are different elements.

template<int f_DEC>
int32_t __alt_model_p1(uint16_t* hdr, uint8_t* buf) {
  P1Ctx* b4, * buf3, * buf2, * buf1, * b0, * cursor0;
  int32_t resid3, cur3;
  void* plane[4];
  AltP1Block*&plane1 = (AltP1Block*&)plane[1];
  AltP1Block*&plane2 = (AltP1Block*&)plane[2];
  AltP1Block*&plane3 = (AltP1Block*&)plane[3];
  uint32_t x, y;
  int32_t out1, resid0, off1, off0, resid2, cur0, off2, cur1, code1, off3;
  AltP1Block* blk_k, * raw;
  uint8_t fl1, fl2, fl3, pred1, pred2, pred3;
  AltP1Block* made;
  int32_t width, height, k, src1, src2, src3, dc2, xf1, xf2, xf3, w, n_planes, p, want0, keep0, code0, recon0, out0, drift0, err0, resid1, at1, recon1, drift1, code1x, want2, code2, recon2, out2, drift2, at2, code3, recon3, drift3, np, f;
  int64_t err1, err2, err3;
  AltP1Block* blk1, * blk2, * blk3;
  P1Ctx* cursor2, * cursor4;
  uint8_t pred0, want1, want3;
  width = *hdr;
  height = hdr[1];
  if( plane_count>0 ) {
    k = 0;
    do {
      raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
      if( raw )
        made = raw->alt_p1_alloc(width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    } while( k<plane_count );
  }
  src1 = plane_desc[2].src_plane;
  src2 = plane_desc[3].src_plane;
  src3 = plane_desc[4].src_plane;
  fl1 = plane_desc[src1+1].flags;
  fl2 = plane_desc[src2+1].flags;
  fl3 = plane_desc[src3+1].flags;
  dc2 = plane_desc[src2+1].dc;
  int32_t dc1 = plane_desc[src1+1].dc;
  xf1 = fl1&8;
  int8_t dc3 = plane_desc[src3+1].dc;
  xf2 = fl2&8;
  xf3 = fl3&8;
  if( f_DEC )
    __rc_begin_decode(xf2);
  else
    __rc_begin_encode();
  if( height>0 ) {
    y = 0;
    w = width;
    n_planes = plane_count;
    do {
      if( n_planes>0 ) {
        p = 0;
        do {
          ++p;
          blk_k = (AltP1Block*)plane[p-1];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          b4 = blk_k->buf[4];
          buf3 = blk_k->buf[3];
          buf2 = blk_k->buf[2];
          buf1 = blk_k->buf[1];
          b0 = blk_k->buf[0];
          blk_k->buf[4] = buf3;
          blk_k->buf[3] = buf2;
          blk_k->buf[2] = buf1;
          blk_k->buf[1] = b0;
          blk_k->buf[0] = b4;
          b4 += 4;
          blk_k->cursor[0] = b4;
          b0 += 4;
          blk_k->cursor[1] = b0;
          blk_k->cursor[2] = buf1+4;
          blk_k->cursor[3] = buf2+4;
          blk_k->cursor[4] = buf3+4;
          (b4)[-4] = ((P1Ctx*)b0)[3];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            P1Ctx*const up = (P1Ctx*)blk_k->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          cursor2 = blk_k->cursor[2];
          cursor4 = blk_k->cursor[4];
          cursor0 = blk_k->cursor[0];
          blk_k->ctx[2] = 0;
          blk_k->ctx[3] = (int8_t)cursor2[-2].mag+(int8_t)cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
          blk_k->ctx[4] = (int8_t)cursor2[-1].mag+(int8_t)cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
          n_planes = plane_count;
        } while( p<plane_count );
        w = width;
      }
      if( w>0 ) {
        x = 0;
        do {
          AltP1Block* blk0 = (AltP1Block*)plane[0];
          off0 = plane_desc[1].src_plane;
          if( !f_DEC ) {
            want0 = *(buf+plane_desc[1].src_plane);
            cur0 = want0;
          }
          ((AltP1Block*)plane[0])->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
          if( f_DEC ) {
            code0 = __alt_p1_decode_symbol((uint16_t*)&blk0->counters[blk0->ctx[0]], 0, blk0->ctx[1]);
            pred0 = (uint8_t)blk0->pred;
            cur0 = (uint8_t)(pred0+blk0->unfold[code0]);
            keep0 = cur0;
          } else {
            keep0 = cur0;
            pred0 = (uint8_t)blk0->pred;
            resid0 = (uint8_t)(cur0-pred0);
            code0 = blk0->fold[resid0];
            recon0 = (uint8_t)(blk0->unfold[code0]+pred0);
            out0 = (uint8_t)(recon0+*(buf+off0)-cur0);
            drift0 = *(buf+off0)-out0;
            if( drift0<-16||drift0>16 ) {
              code0 = blk0->fold_hi[resid0];
            } else {
              cur0 = recon0;
              *(buf+off0) = out0;
              keep0 = recon0;
            }
            __alt_p1_encode_symbol(&blk0->counters[blk0->ctx[0]].total, recon0, blk0->ctx[1], code0);
          }
          err0 = keep0-blk0->pred;
          blk0->cursor[0]->sym = cur0;
          blk0->cursor[0]->mag = abs32(err0);
          blk0->ctx[blk0->ctx[2]+3] = blk0->ctx[blk0->ctx[2]+3]+blk0->cursor[0]->mag-blk0->cursor[0][-4].mag-(blk0->cursor[4][-2].mag-blk0->cursor[4][6].mag+blk0->cursor[2][-2].mag-blk0->cursor[2][6].mag);
          blk0->ctx[2] = blk0->ctx[2]==0;
          if( blk0->counters[blk0->ctx[0]].total<0x4000u )
            blk0->update_model();
          ++blk0->cursor[0];
          ++blk0->cursor[1];
          ++blk0->cursor[2];
          ++blk0->cursor[3];
          ++blk0->cursor[4];
          off1 = plane_desc[2].src_plane;
          if( f_DEC ) {
            *(plane_desc[1].src_plane+buf) = cur0;
          } else {
            want1 = *(buf+plane_desc[2].src_plane);
            if( xf1 )
              want1 = want1-dc1-*(plane_desc[1].src_plane+buf);
            cur1 = want1;
          }
          blk1 = (AltP1Block*)(plane1);
          ((AltP1Block*)plane1)->ctx_of((AltP1Block*)plane[0], (AltP1Block*)0);
          if( f_DEC ) {
            code1 = __alt_p1_decode_symbol(&blk1->counters[blk1->ctx[0]].total, 0, blk1->ctx[1]);
            pred1 = (uint8_t)blk1->pred;
            cur1 = (uint8_t)(pred1+blk1->unfold[code1]);
          } else {
            pred1 = (uint8_t)blk1->pred;
            resid1 = (uint8_t)(cur1-pred1);
            code1 = blk1->fold[resid1];
            at1 = *(buf+off1);
            recon1 = (uint8_t)(blk1->unfold[code1]+pred1);
            out1 = (uint8_t)(recon1+*(buf+off1)-cur1);
            drift1 = at1-out1;
            code1x = code1;
            if( drift1<-16||drift1>16 ) {
              code1x = blk1->fold_hi[resid1];
            } else {
              *(buf+off1) = out1;
              cur1 = recon1;
            }
            __alt_p1_encode_symbol((uint16_t*)&blk1->counters[blk1->ctx[0]], 16*blk1->ctx[0], (int32_t)blk1->ctx[1], code1x);
          }
          err1 = cur1-blk1->pred;
          blk1->cursor[0]->sym = cur1;
          blk1->cursor[0]->mag = (BYTE4(err1)^err1)-BYTE4(err1);
          blk1->ctx[blk1->ctx[2]+3] += blk1->cursor[0]->mag-blk1->cursor[0][-4].mag-(blk1->cursor[4][-2].mag-blk1->cursor[4][6].mag)-(blk1->cursor[2][-2].mag-blk1->cursor[2][6].mag);
          blk1->ctx[2] = blk1->ctx[2]==0;
          if( blk1->counters[blk1->ctx[0]].total<0x4000u )
            (blk1)->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          off2 = plane_desc[3].src_plane;
          if( f_DEC ) {
            int32_t val1x = cur1;
            if( xf1 )
              val1x += dc1+*(plane_desc[1].src_plane+buf);
            *(buf+plane_desc[2].src_plane) = val1x;
          } else {
            want2 = *(plane_desc[3].src_plane+buf);
            if( xf2 )
              want2 = (uint8_t)(want2-dc2-((plane_desc[plane_desc[3].src_plane+1].weight0**(plane_desc[1].src_plane+buf)+plane_desc[plane_desc[3].src_plane+1].weight1*(uint32_t)*(plane_desc[2].src_plane+buf)+40)>>7));
          }
          blk2 = (AltP1Block*)(plane2);
          ((AltP1Block*)plane2)->ctx_of((AltP1Block*)plane1, (AltP1Block*)plane[0]);
          if( f_DEC ) {
            code2 = __alt_p1_decode_symbol((uint16_t*)&blk2->counters[blk2->ctx[0]], 0, (int32_t)blk2->ctx[1]);
            pred2 = (uint8_t)blk2->pred;
            want2 = (uint8_t)(pred2+blk2->unfold[code2]);
          } else {
            pred2 = (uint8_t)blk2->pred;
            resid2 = (uint8_t)(want2-pred2);
            code2 = blk2->fold[resid2];
            recon2 = (uint8_t)(blk2->unfold[code2]+pred2);
            out2 = (uint8_t)(recon2+*(off2+buf)-want2);
            drift2 = *(off2+buf)-out2;
            if( drift2<-16||drift2>16 ) {
              code2 = blk2->fold_hi[resid2];
            } else {
              want2 = recon2;
              *(off2+buf) = out2;
            }
            __alt_p1_encode_symbol((uint16_t*)&blk2->counters[blk2->ctx[0]], code2, (int32_t)blk2->ctx[1], code2);
          }
          err2 = want2-blk2->pred;
          blk2->cursor[0]->sym = want2;
          blk2->cursor[0]->mag = (BYTE4(err2)^err2)-BYTE4(err2);
          blk2->ctx[blk2->ctx[2]+3] += blk2->cursor[0]->mag-blk2->cursor[0][-4].mag-(blk2->cursor[4][-2].mag-blk2->cursor[4][6].mag)-(blk2->cursor[2][-2].mag-blk2->cursor[2][6].mag);
          at2 = blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2]==0;
          if( blk2->counters[at2].total<0x4000u )
            ((AltP1Block*)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          if( f_DEC ) {
            if( xf2 )
              *(plane_desc[3].src_plane+buf) = ((plane_desc[plane_desc[3].src_plane+1].weight0**(plane_desc[1].src_plane+buf)+plane_desc[plane_desc[3].src_plane+1].weight1*(uint32_t)*(plane_desc[2].src_plane+buf)+40)>>7)+dc2+want2;
            else
              *(plane_desc[3].src_plane+buf) = want2;
          }
          n_planes = plane_count;
          if( plane_count>=4 ) {
            off3 = plane_desc[4].src_plane;
            if( !f_DEC ) {
              if( xf3 )
                want3 = *(plane_desc[4].src_plane+buf)-dc3-((plane_desc[plane_desc[4].src_plane+1].weight1**(plane_desc[4].src_plane+buf-2)+plane_desc[plane_desc[4].src_plane+1].weight0**(plane_desc[4].src_plane+buf-3)+plane_desc[plane_desc[4].src_plane+1].weight2**(plane_desc[4].src_plane+buf-1)+64)>>7);
              else
                want3 = *(plane_desc[4].src_plane+buf);
              cur3 = want3;
            }
            blk3 = (AltP1Block*)(plane3);
            ((AltP1Block*)plane3)->ctx_of((AltP1Block*)plane2, (AltP1Block*)plane1);
            if( f_DEC ) {
              code3 = __alt_p1_decode_symbol((uint16_t*)&blk3->counters[blk3->ctx[0]], 0, (int32_t)blk3->ctx[1]);
              pred3 = (uint8_t)blk3->pred;
              cur3 = (uint8_t)(pred3+blk3->unfold[code3]);
            } else {
              pred3 = (uint8_t)blk3->pred;
              code3 = blk3->fold[(uint8_t)(cur3-pred3)];
              resid3 = (uint8_t)(cur3-pred3);
              recon3 = (uint8_t)(blk3->unfold[code3]+pred3);
              drift3 = *(off3+buf)-(uint8_t)(recon3+*(off3+buf)-cur3);
              uint8_t out3 = recon3+*(off3+buf)-cur3;
              if( drift3<-16||drift3>16 ) {
                code3 = blk3->fold_hi[resid3];
              } else {
                cur3 = recon3;
                *(off3+buf) = out3;
              }
              __alt_p1_encode_symbol((uint16_t*)&blk3->counters[blk3->ctx[0]], code3, (int32_t)blk3->ctx[1], code3);
            }
            err3 = cur3-blk3->pred;
            blk3->cursor[0]->sym = cur3;
            blk3->cursor[0]->mag = (BYTE4(err3)^err3)-BYTE4(err3);
            blk3->ctx[blk3->ctx[2]+3] += blk3->cursor[0]->mag-blk3->cursor[0][-4].mag-(blk3->cursor[4][-2].mag-blk3->cursor[4][6].mag)-(blk3->cursor[2][-2].mag-blk3->cursor[2][6].mag);
            blk3->ctx[2] = blk3->ctx[2]==0;
            if( blk3->counters[blk3->ctx[0]].total<0x4000u )
              ((AltP1Block*)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            if( f_DEC ) {
              int32_t val3x = cur3;
              if( xf3 )
                val3x += ((plane_desc[plane_desc[4].src_plane+1].weight1**(plane_desc[4].src_plane+buf-2)+plane_desc[plane_desc[4].src_plane+1].weight0**(plane_desc[4].src_plane+buf-3)+plane_desc[plane_desc[4].src_plane+1].weight2**(plane_desc[4].src_plane+buf-1)+64)>>7)+dc3;
              *(plane_desc[4].src_plane+buf) = val3x;
            }
            n_planes = plane_count;
          }
          buf += n_planes;
          ++x;
        } while( x<(uint32_t)width );
        w = width;
      }
      ++y;
    } while( y<(uint32_t)height );
  }
  if( f_DEC )
    __rc_end_decode();
  else
    __rc_end_encode();
  np = plane_count;
  if( plane_count>0 ) {
    f = 0;
    do {
      void** q = (void**)plane[f];
      if( q ) {
        ((AltP1Block*)q)->alt_p1_free(1);
        np = plane_count;
      }
      ++f;
    } while( f<np );
  }
  return np;
}

int32_t __alt_model_p1_encode(uint16_t* hdr, uint8_t* src) {
  return __alt_model_p1<0>(hdr, src);
}

int32_t __alt_model_p1_decode(uint16_t* hdr, uint8_t* out) {
  return __alt_model_p1<1>(hdr, out);
}
