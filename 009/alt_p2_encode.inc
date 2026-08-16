// alt_p2_encode.inc -- the predictor-2 model, encoding.

// `P2Coef` in alt_p2_block.inc is the coefficient-table borrow both directions
// do for the length of one image.  This file's header used to say the encoder
// did it and the decoder did not; the decoder does, in the same six lines, and
// the two are one object now.
int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
  AltP2Block* plane[4];
  uint32_t i, src1;
  int16_t seed1, l7c, seed2;
  int32_t nplanes, want0, off0, seed0, at0;
  uint8_t* out = a2;
  P2Coef p2_coef;
  p2_coef.fold();
  int32_t row_i = p_i->width;
  int32_t height = p_i->height;
  alt_p2_planes_alloc(plane, row_i);
  int32_t xf0 = plane_desc[plane_desc[1].src_plane].flags&desc_has_refs;
  int32_t np = plane_desc[plane_desc[2].src_plane].flags&desc_has_refs;
  int32_t y = (plane_desc[plane_desc[3].src_plane].flags)&desc_has_refs;
  rc_begin_encode();
  if( height>0 ) {
    src1 = 0;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        alt_p2_start_row(plane, src1, row_i);
        nplanes = plane_count;
      }
      if( row_i<=0 ) {
        for( int32_t j = 0; j<4; ++j ) ctx_bias[j] = 0;
      } else {
        uint32_t xf4 = src1;
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
          AltP2Block *blk0 = (AltP2Block*)(plane[0]);
          blk0->encode_sample(out, off0b, plane[2], plane[1]);
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
          blk1->encode_sample(out, off1, plane[0], plane[2]);
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
          blk2->encode_sample(out, off2, plane[0], plane[1]);
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
            l4c->encode_sample(out, alpha_src, plane[2], plane[0]);
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
  return alt_p2_planes_free(plane);
}
