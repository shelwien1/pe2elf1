// alt_model_p1_decode.inc -- one-bit planes, decoded.

int32_t __alt_model_p1_decode(uint16_t* hdr, uint8_t* out) {
  P1Ctx* b4, * buf3, * buf2, * buf1, * b0, * cursor0;
  void* p0v;
  int32_t val3x;
  void* plane[4];
  AltP1Block*&plane1 = (AltP1Block*&)plane[1];
  AltP1Block*&plane2 = (AltP1Block*&)plane[2];
  AltP1Block*&plane3 = (AltP1Block*&)plane[3];
  int32_t dc1;
  uint32_t x;
  int32_t val1x;
  AltP1Block* blk1;
  int32_t code1;
  AltP1Block* blk_k, * raw;
  uint32_t at0;
  int32_t code0;
  uint32_t pred0, pred1;
  AltP1Block* blk2, * blk3;
  P1Ctx* cursor2, * cursor4;
  void** q;
  AltP1Block* made;
  int32_t width, height, k, src1, src2, src3, dc2, dc3, xf1, xf2, xf3, w, n_planes, p, val0, val1, code2, val2, at2, code3, val3, at3, np, f;
  uint32_t y;
  int32_t pred2, pred3;
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
  dc2 = plane_desc[src2+1].dc;
  dc3 = plane_desc[src3+1].dc;
  dc1 = plane_desc[src1+1].dc;
  xf1 = plane_desc[src1+1].flags&8;
  xf2 = plane_desc[src2+1].flags&8;
  xf3 = plane_desc[src3+1].flags&8;
  __rc_begin_decode(xf2);
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
          ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
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
          AltP1Block*const blk = (AltP1Block*)plane[0];
          blk->ctx_of(nullptr, nullptr);
          code0 = __alt_p1_decode_symbol((uint16_t*)&blk->counters[blk->ctx[0]], 0, blk->ctx[1]);
          pred0 = blk->pred;
          val0 = (uint8_t)(pred0+blk->unfold[code0]);
          blk->cursor[0]->sym = val0;
          blk->cursor[0]->mag = abs32(val0-pred0);
          blk->ctx[blk->ctx[2]+3] = blk->ctx[blk->ctx[2]+3]+blk->cursor[0]->mag-blk->cursor[0][-4].mag-(blk->cursor[4][-2].mag-blk->cursor[4][6].mag+blk->cursor[2][-2].mag-blk->cursor[2][6].mag);
          at0 = blk->ctx[0];
          blk->ctx[2] = blk->ctx[2]==0;
          if( blk->counters[at0].total<0x4000u )
            blk->update_model();
          ++blk->cursor[0];
          ++blk->cursor[1];
          ++blk->cursor[2];
          ++blk->cursor[3];
          ++blk->cursor[4];
          blk1 = (AltP1Block*)plane1;
          *(plane_desc[1].src_plane+out) = val0;
          ((AltP1Block*)blk1)->ctx_of((AltP1Block*)plane[0], (AltP1Block*)0);
          code1 = __alt_p1_decode_symbol(&blk1->counters[blk1->ctx[0]].total, 0, blk1->ctx[1]);
          pred1 = blk1->pred;
          val1 = (uint8_t)(pred1+blk1->unfold[code1]);
          val1x = val1;
          blk1->cursor[0]->sym = val1;
          blk1->cursor[0]->mag = abs32(val1-pred1);
          blk1->ctx[3+blk1->ctx[2]] = blk1->ctx[3+blk1->ctx[2]]+blk1->cursor[0]->mag-blk1->cursor[0][-4].mag-(blk1->cursor[4][-2].mag-blk1->cursor[4][6].mag+blk1->cursor[2][-2].mag-blk1->cursor[2][6].mag);
          blk1->ctx[2] = blk1->ctx[2]==0;
          if( blk1->counters[blk1->ctx[0]].total<0x4000u )
            blk1->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          if( xf1 )
            val1x += dc1+*(plane_desc[1].src_plane+out);
          blk2 = (AltP1Block*)(plane2);
          p0v = plane[0];
          *(out+plane_desc[2].src_plane) = val1x;
          ((AltP1Block*)blk2)->ctx_of((AltP1Block*)plane1, (AltP1Block*)p0v);
          code2 = __alt_p1_decode_symbol((uint16_t*)&blk2->counters[blk2->ctx[0]], 0, (int32_t)blk2->ctx[1]);
          pred2 = blk2->pred;
          val2 = (uint8_t)(pred2+blk2->unfold[code2]);
          blk2->cursor[0]->sym = val2;
          blk2->cursor[0]->mag = abs32(val2-pred2);
          blk2->ctx[blk2->ctx[2]+3] += (blk2->cursor[0]->mag-blk2->cursor[0][-4].mag-(blk2->cursor[4][-2].mag-blk2->cursor[4][6].mag)-(blk2->cursor[2][-2].mag-blk2->cursor[2][6].mag));
          at2 = blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2]==0;
          if( blk2->counters[at2].total<0x4000u )
            ((AltP1Block*)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          if( xf2 )
            *(plane_desc[3].src_plane+out) = ((plane_desc[plane_desc[3].src_plane+1].weight0**(plane_desc[1].src_plane+out)+plane_desc[plane_desc[3].src_plane+1].weight1*(uint32_t)*(plane_desc[2].src_plane+out)+40)>>7)+dc2+val2;
          else
            *(plane_desc[3].src_plane+out) = val2;
          n_planes = plane_count;
          if( plane_count>=4 ) {
            blk3 = (AltP1Block*)(plane3);
            ((AltP1Block*)plane3)->ctx_of((AltP1Block*)plane2, (AltP1Block*)plane1);
            code3 = __alt_p1_decode_symbol((uint16_t*)&blk3->counters[blk3->ctx[0]], 0, (int32_t)blk3->ctx[1]);
            pred3 = blk3->pred;
            val3 = (uint8_t)(pred3+blk3->unfold[code3]);
            val3x = val3;
            blk3->cursor[0]->sym = val3;
            blk3->cursor[0]->mag = abs32(val3-pred3);
            blk3->ctx[blk3->ctx[2]+3] += (blk3->cursor[0]->mag-blk3->cursor[0][-4].mag-(blk3->cursor[4][-2].mag-blk3->cursor[4][6].mag)-(blk3->cursor[2][-2].mag-blk3->cursor[2][6].mag));
            at3 = blk3->ctx[0];
            blk3->ctx[2] = blk3->ctx[2]==0;
            if( blk3->counters[at3].total<0x4000u )
              ((AltP1Block*)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            if( xf3 )
              val3x += ((plane_desc[plane_desc[4].src_plane+1].weight1**(plane_desc[4].src_plane+out-2)+plane_desc[plane_desc[4].src_plane+1].weight0**(plane_desc[4].src_plane+out-3)+plane_desc[plane_desc[4].src_plane+1].weight2**(plane_desc[4].src_plane+out-1)+64)>>7)+dc3;
            *(plane_desc[4].src_plane+out) = val3x;
            n_planes = plane_count;
          }
          out += n_planes;
          ++x;
        } while( x<(uint32_t)width );
        w = width;
      }
      ++y;
    } while( y<(uint32_t)height );
  }
  __rc_end_decode();
  np = plane_count;
  if( plane_count>0 ) {
    f = 0;
    do {
      q = (void**)plane[f];
      if( q ) {
        ((AltP1Block*)q)->alt_p1_free(1);
        np = plane_count;
      }
      ++f;
    } while( f<np );
  }
  return np;
}
