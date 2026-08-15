// filter_search.inc -- search the filters, which is what -Q9 spends its time on.

uint32_t search_filter(BmfImage* img, int8_t mode) {
  struct alignas(16) SearchFilterFrame {
    uint8_t* base;
    uint8_t marks[16];
    int32_t rows[2];
    int32_t dims[2];
    union {
      PlaneDesc saved[4];
      struct {
        int32_t plane_i;
        uint32_t costs[4];
        int32_t pi;
        void* tile_buf;
        int32_t flag8;
        int32_t best_flags;
        uint8_t _pad0[28];
      };
    };
    void* tile_src;
    int32_t n_hard;
    int32_t bits_total;
    int32_t n_flagged;
    int32_t best_bits;
    BmfImage* tile_img;
    uint8_t _pad1[36];
  } frame;
  uint8_t* base2;
  bool deep2;
  uint8_t f0, f1, f2, f3;
  uint8_t *srcp, *p2, *tile_copy, *tile_a, *p4, *p5, *tile_c, *p12, *p13;
  int32_t hard, bits_a, bits_b, cost_f13;
  int32_t p3, p6, p7, p10, p11, p14;
  int32_t flags_s, desc_k;
  int32_t tile_w, tile_h, nplanes, y0, dx, off_y, row_bytes, best_cost, bits_f5, cost_f5, f5, pred, c0, c1, c2, c3, cand, nplanes_c, pl_a, pi0, pl_b, pi1, bits2, plane, pi3, bits_e, nplanes_b, pl_c, pi4, pl_d, bits_c, img_h1, y1, off1, x1, bits_d, pk, pl_k, bits, img_h3, y3, off3, pl_i, rows_x_planes, y2, off2, x2, pl2, bits_f8, cost_f8, f8, pl3, bits_f13, f13, cost_f14, f14, pl0, bits_f6, cost_f6, f6;
  uint16_t img_h3b, img_h2, img_h1b;
  uint8_t* p8, * p9;
  tile_w = img->width;
  tile_h = img->height;
  if( tile_w<4||tile_h<3 ) {
    if( plane_count>0 ) {
      int32_t pl = 0;
      do {
        plane_desc[pl].flags = 0;
        plane_desc[pl].src_plane = pl;
        plane_desc[pl].nrefs = pl;
        ++pl;
      } while( pl<plane_count );
    }
    return 0;
  }
  choose_plane_coding(img);
  frame.tile_img = alloc_image(tile_w, tile_h, img->depth&depth_bits, 0, 0);
  stream.size = frame.tile_img->data_size+0x20000;
  stream.buf = (uint8_t*)bmf_new(stream.size);
  packer_reset();
  frame.dims[0] = tile_h*tile_w;
  frame.tile_buf = bmf_new(tile_h*tile_w);
  // Encode one plane of the trial tile under `flags` and return what it cost,
  // in bits.  Seven sites wrote this out: set the descriptor, call the model,
  // measure the bytes written, flush the packer's last partial word.  Four take
  // it as written; the other three differ in an argument or in what they do
  // between the measurement and the flush.
  //
  // The measurement is real coded bytes and not an estimate, which is what
  // makes comparing candidates worth anything -- and is also the whole cost of
  // `-Q9`.
  auto probe_plane = [&](int32_t pl, int32_t flags) {
    plane_desc[pl].flags = flags;
    model_planes((uint8_t*)frame.tile_img, (uint8_t*)frame.tile_buf, pl);
    int32_t cost = 8*(stream.cur-stream.buf);
    *(uint32_t*)stream.pk.word = stream.pk.acc;
    return cost;
  };
  nplanes = plane_count;
  y0 = (img->height-tile_h)>>1;
  dx = img->width-tile_w;
  off_y = y0*img->stride;
  frame.rows[1] = y0;
  srcp = img->pixels+plane_count*(dx>>1)+off_y;
  frame.tile_src = frame.tile_img->pixels;
  if( y0<tile_h+y0 ) {
    row_bytes = (uint16_t)frame.tile_img->stride;
    frame.dims[1] = tile_h;
    int32_t y = frame.rows[1];
    uint8_t *dstp = frame.tile_img->pixels;
    do {
      memcpy(dstp, srcp, row_bytes);
      row_bytes = (uint16_t)frame.tile_img->stride;
      dstp += row_bytes;
      srcp += img->stride;
      ++y;
    } while( y<frame.dims[1]+((img->height-frame.dims[1])>>1) );
    nplanes = plane_count;
  }
  frame.n_hard = 0;
  frame.bits_total = 0;
  frame.n_flagged = 0;
  frame.best_bits = 0;
  if( nplanes>0 ) {
    hard = frame.n_hard;
    frame.best_flags = 0;
    frame.pi = 0;
    frame.dims[0] *= 8;
    while( 1 ) {
      frame.dims[1] = plane_desc[frame.pi].src_plane;
      frame.plane_i = frame.dims[1];
      if( hard ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(frame.dims[1], 0);
        flags_s = frame.best_flags;
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          flags_s = 0;
        frame.best_flags = flags_s;
      }
      bits_f5 = probe_plane(frame.dims[1], 5);
      cost_f5 = bits_f5;
      packer_reset();
      f5 = frame.best_flags;
      if( cost_f5<best_cost ) {
        best_cost = cost_f5;
        f5 = 5;
      }
      frame.best_flags = f5;
      if( cost_f5<best_cost+(best_cost>>5)||hard ) {
        pl0 = frame.dims[1];
        bits_f6 = probe_plane(pl0, 6);
        cost_f6 = bits_f6;
        f6 = frame.best_flags;
        packer_reset();
        if( cost_f6<best_cost ) {
          best_cost = cost_f6;
          f6 = 6;
        }
        frame.best_flags = f6;
        if( frame.pi ) {
          {
try_flag8:
            pl2 = frame.dims[1];
            bits_f8 = probe_plane(pl2, 8);
            cost_f8 = bits_f8;
            f8 = frame.best_flags;
            packer_reset();
            if( cost_f8<best_cost ) {
              best_cost = cost_f8;
              f8 = 8;
            }
            frame.best_flags = f8;
          }
          pl3 = frame.dims[1];
          bits_f13 = probe_plane(pl3, 13);
          cost_f13 = bits_f13;
          frame.rows[0] = cost_f13;
          f13 = frame.best_flags;
          packer_reset();
          if( cost_f13<best_cost ) {
            best_cost = (int32_t)cost_f13;
            f13 = 13;
          }
          frame.best_flags = f13;
          pred = f13&3;
          if( pred==2||best_cost+(best_cost>>5)>frame.rows[0] ) {
            // `dims[1]` and `plane_i` are the same plane -- both were set from
            // `plane_desc[pi].src_plane` at the top of this loop -- and the
            // first `cost_f14` was overwritten by the next line.
            cost_f14 = probe_plane(frame.plane_i, 14);
            deep2 = cost_f14<best_cost;
            if( cost_f14<best_cost )
              best_cost = cost_f14;
            packer_reset();
            f14 = frame.best_flags;
            if( deep2 )
              f14 = 14;
            frame.best_flags = f14;
            if( deep2 )
              pred = 2;
          }
          goto keep_flag8;
        }
        pred = f6&3;
      } else {
        if( frame.pi )
          goto try_flag8;
        pred = (uint8_t)frame.best_flags&3;
      }
keep_flag8: { frame.flag8 = (uint8_t)frame.best_flags&8; }
      frame.best_bits += best_cost;
      c0 = pred==2;
      if( pred!=1 )
        pred = 0;
      c1 = frame.plane_i;
      hard += c0;
      frame.bits_total += pred;
      c2 = frame.flag8;
      frame.costs[frame.dims[1]] = best_cost;
      c3 = frame.pi;
      frame.n_flagged += c2!=0;
      plane_desc[c1].flags = (uint8_t)frame.best_flags;
      frame.pi = c3+1;
      if( c3+1>=plane_count ) {
        frame.n_hard = hard;
        break;
      }
    }
  }
  {
    p2 = (uint8_t*)bmf_new(frame.tile_img->data_size);
    img_h1 = frame.tile_img->height;
    frame.rows[1] = plane_count;
    frame.pi = plane_count*(img_h1-1);
    memcpy(p2, (uint8_t*)frame.tile_src, frame.tile_img->data_size);
    img_h1b = frame.tile_img->height;
    if( img_h1b ) {
      p5 = p2;
      p3 = frame.rows[1];
      y1 = 0;
      tile_a = (uint8_t*)frame.tile_img;
      off1 = 0;
      do {
        x1 = ((const BmfImage*)tile_a)->width;
        p4 = &tile_a[off1+16];
        do {
          p6 = p3;
          do {
            *p4++ = *p5++;
            --p6;
          } while( p6 );
          p4 += frame.pi;
          --x1;
        } while( x1 );
        tile_a = (uint8_t*)frame.tile_img;
        img_h1b = frame.tile_img->height;
        off1 += p3;
        ++y1;
      } while( y1<img_h1b );
    }
    transpose_image_in_place(frame.tile_img, frame.rows[1]);
    free(p2);
    bits_d = 0;
    if( plane_count>0 ) {
      pk = 0;
      while( 1 ) {
        pl_k = plane_desc[pk].src_plane;
        frame.rows[0] = pl_k;
        model_planes((uint8_t*)frame.tile_img, (uint8_t*)frame.tile_buf, pl_k);
        bits = 8*(stream.cur-stream.buf);
        *(uint32_t*)stream.pk.word = stream.pk.acc;
        bits_d += bits;
        packer_reset();
        if( (uint32_t)(bits-(bits>>8))>frame.costs[frame.rows[0]] )
          break;
        if( ++pk>=plane_count ) {
          goto tile_restored;
        }
      }
      bits_d += frame.best_bits+1;
    }
tile_restored:
    if( bits_d+(bits_d>>12)>=frame.best_bits ) {
      base2 = (uint8_t*)bmf_new(frame.tile_img->data_size);
      rows_x_planes = plane_count*(frame.tile_img->height-1);
      frame.rows[1] = plane_count;
      memcpy(base2, (uint8_t*)frame.tile_src, frame.tile_img->data_size);
      img_h2 = frame.tile_img->height;
      if( img_h2 ) {
        frame.costs[0] = rows_x_planes;
        p11 = frame.rows[1];
        y2 = 0;
        tile_c = (uint8_t*)frame.tile_img;
        off2 = 0;
        p13 = base2;
        do {
          x2 = ((const BmfImage*)tile_c)->width;
          p12 = &tile_c[off2+16];
          do {
            p14 = p11;
            do {
              *p12++ = *p13++;
              --p14;
            } while( p14 );
            p12 += frame.costs[0];
            --x2;
          } while( x2 );
          tile_c = (uint8_t*)frame.tile_img;
          img_h2 = frame.tile_img->height;
          off2 += p11;
          ++y2;
        } while( y2<img_h2 );
      }
      transpose_image_in_place(frame.tile_img, frame.rows[1]);
      free(base2);
    } else {
      frame.best_bits = bits_d;
      frame.base = (uint8_t*)bmf_new(img->data_size);
      img_h3 = img->height;
      frame.rows[1] = plane_count;
      frame.costs[0] = plane_count*(img_h3-1);
      memcpy(frame.base, img->pixels, img->data_size);
      img_h3b = img->height;
      if( img_h3b ) {
        p7 = frame.rows[1];
        y3 = 0;
        off3 = 0;
        p9 = frame.base;
        do {
          pl_i = img->width;
          p8 = img->pixels+off3;
          do {
            p10 = p7;
            do {
              *p8++ = *p9++;
              --p10;
            } while( p10 );
            p8 += frame.costs[0];
            --pl_i;
          } while( pl_i );
          img_h3b = img->height;
          off3 += p7;
          ++y3;
        } while( y3<img_h3b );
      }
      transpose_image_in_place(img, frame.rows[1]);
      free(frame.base);
    }
  }
  free(frame.tile_buf);
  if( plane_count>2 ) {
    if( frame.bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        frame.saved[desc_k] = plane_desc[desc_k];
      if( plane_count>0 ) {
        plane = 0;
        do {
          pi3 = plane;
          f2 = plane_desc[plane++].flags&8|5;
          plane_desc[pi3].flags = f2;
        } while( plane<plane_count );
      }
      transform_planes(frame.tile_img);
      bits_e = 8*(stream.cur-stream.buf);
      deep2 = bits_e<=frame.best_bits;
      *(uint32_t*)stream.pk.word = stream.pk.acc;
      packer_reset();
      if( deep2 ) {
        frame.best_bits = bits_e;
        nplanes_b = plane_count;
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k] = frame.saved[desc_k];
        nplanes_b = plane_count;
        cand = 1;
      }
      if( nplanes_b<=2 )
        goto planes_done;
    } else {
      cand = 1;
    }
    if( frame.n_hard+(cand==0) ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        frame.saved[desc_k] = plane_desc[desc_k];
      nplanes_c = plane_count;
      if( plane_count>0 ) {
        pl_a = 0;
        do {
          pi0 = pl_a;
          f0 = plane_desc[pl_a++].flags&8|6;
          plane_desc[pi0].flags = f0;
          nplanes_c = plane_count;
        } while( pl_a<plane_count );
      }
      if( nplanes_c==frame.n_hard&&nplanes_c-1==frame.n_flagged ) {
        cand = 0;
      } else {
        transform_planes(frame.tile_img);
        bits_a = 8*(stream.cur-stream.buf);
        deep2 = bits_a<=frame.best_bits;
        frame.rows[0] = bits_a;
        *(uint32_t*)stream.pk.word = stream.pk.acc;
        packer_reset();
        if( deep2 ) {
          frame.best_bits = bits_a;
          frame.n_hard = bits_a;
          if( plane_count-1==frame.n_flagged ) {
            cand = 0;
          } else {
            for( desc_k = 0; desc_k<4; ++desc_k )
              frame.saved[desc_k] = plane_desc[desc_k];
            if( plane_count>0 ) {
              pl_b = 0;
              do {
                pi1 = pl_b;
                f1 = plane_desc[pl_b++].nrefs;
                plane_desc[pi1].flags |= 8*(f1!=0);
              } while( pl_b<plane_count );
            }
            transform_planes(frame.tile_img);
            bits2 = 8*(stream.cur-stream.buf);
            deep2 = bits2<=frame.rows[0];
            *(uint32_t*)stream.pk.word = stream.pk.acc;
            packer_reset();
            if( deep2 ) {
              cand = 0;
            } else {
              for( desc_k = 0; desc_k<4; ++desc_k )
                plane_desc[desc_k] = frame.saved[desc_k];
              cand = 0;
            }
          }
        } else {
          for( desc_k = 0; desc_k<4; ++desc_k )
            plane_desc[desc_k] = frame.saved[desc_k];
        }
      }
    }
  } else {
    cand = 1;
  }
planes_done:
  if( !frame.n_hard&&plane_count>1 ) {
    tile_copy = (uint8_t*)bmf_new(frame.tile_img->data_size);
    memcpy(tile_copy, (uint8_t*)frame.tile_src, frame.tile_img->data_size);
    for( desc_k = 0; desc_k<4; ++desc_k )
      frame.saved[desc_k] = plane_desc[desc_k];
    if( plane_count>0 ) {
      pl_c = 0;
      do {
        pi4 = pl_c;
        f3 = plane_desc[pl_c++].flags&0xFB;
        plane_desc[pi4].flags = f3;
      } while( pl_c<plane_count );
    }
    transform_planes(frame.tile_img);
    bits_b = 8*(stream.cur-stream.buf);
    frame.rows[1] = bits_b;
    *(uint32_t*)stream.pk.word = stream.pk.acc;
    packer_reset();
    if( bits_b>frame.best_bits ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        plane_desc[desc_k] = frame.saved[desc_k];
    } else {
      frame.best_bits = bits_b;
      cand = 0;
    }
    if( frame.n_flagged+frame.bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        frame.saved[desc_k] = plane_desc[desc_k];
      if( plane_count>0 ) {
        pl_d = 0;
        do
          plane_desc[pl_d++].flags = 0;
        while( pl_d<plane_count );
      }
      memcpy((uint8_t*)frame.tile_src, tile_copy, frame.tile_img->data_size);
      transform_planes(frame.tile_img);
      bits_c = 8*(stream.cur-stream.buf);
      deep2 = bits_c<=frame.best_bits;
      *(uint32_t*)stream.pk.word = stream.pk.acc;
      packer_reset();
      if( deep2 ) {
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k] = frame.saved[desc_k];
      }
    }
    free(tile_copy);
  }
  free(stream.buf);
  free((uint8_t*)frame.tile_img);
  return cand;
}
