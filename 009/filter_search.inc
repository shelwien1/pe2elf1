// filter_search.inc -- search the filters, which is what -Q9 spends its time on.

// The six plane descriptors the per-plane search probes.  `probe_plane` is
// called with each in turn and the same literal is also the answer stored back
// into `best_flags`, so every one of these numbers appeared twice with nothing
// saying what it selected.
//
// Each probe pairs its predictor with `desc_alt_model`, which is why 5 and 6
// rather than 1 and 2.  These are not the only descriptors that reach the wire:
// the three whole-image trials below start from what the per-plane search chose
// and edit it -- forcing `try_p1` or `try_p2` across all planes, adding
// `desc_has_refs` where `nrefs` is non-zero, or clearing `desc_alt_model` --
// and keep the result if it codes smaller.  Clearing the alt bit is what puts
// a bare 1 or 9 in a descriptor.
enum : int32_t {
  try_mode0    = pred_mode0,                                 //  0
  try_p1       = pred_p1|desc_alt_model,                     //  5
  try_p2       = pred_p2|desc_alt_model,                     //  6
  try_refs     = desc_has_refs,                              //  8
  try_refs_p1  = desc_has_refs|pred_p1|desc_alt_model,       // 13
  try_refs_p2  = desc_has_refs|pred_p2|desc_alt_model,       // 14
};

uint32_t search_filter(BmfImage* img, int8_t mode) {
  // The frame was one union of two arms and 36 bytes of pad, and the two arms
  // never overlap: everything in the scalar arm is dead by the `free(tile_buf)`
  // below, and `saved` is first written four lines later.  So it is MSVC
  // sharing one 64-byte slot between the per-plane search and the
  // whole-image trials, and the two are plain locals.
  //
  // Two of those scalars did double duty inside the arm as well.  `pi` was the
  // plane loop's index and then a pixel stride, and `costs[0]` -- the first
  // entry of a four-plane cost table -- was that same stride twice more; all
  // three were the `planes*(height-1)` of a pixel transpose, and the transpose
  // is `transpose_image` now.
  //
  // `rows[2]` and `dims[2]` were not arrays either.  Every one of their six
  // roles is a copy of a local that is still live at the read -- `bits_f13`,
  // `pl_k`, `bits_a`, `y0`, `plane_count`, `tile_h`, `plane_i` -- and
  // `dims[0]` was written twice and never read.  `uncopy.py` cannot see any of
  // it: its rule is about a named local, and a subscript is not one.
  PlaneDesc saved[4];
  uint32_t plane_cost[4];
  uint8_t* tile_buf;
  int32_t plane_i, pi, flag8, best_flags;
  int32_t n_hard, bits_total, n_flagged, best_bits;
  uint8_t* tile_src;
  BmfImage* tile_img;
  bool deep2;
  uint8_t f0, f1, f2, f3;
  uint8_t *srcp, *tile_copy;
  int32_t hard, bits_a, bits_b;
  int32_t flags_s, desc_k;
  int32_t tile_w, tile_h, nplanes, y0, dx, off_y, best_cost, bits_f5, f5, pred, cand, nplanes_c, pl_a, pi0, pl_b, pi1, bits2, plane, pi3, bits_e, nplanes_b, pl_c, pi4, pl_d, bits_c, bits_d, pk, pl_k, bits, bits_f8, f8, bits_f13, f13, cost_f14, f14, bits_f6, f6;
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
  tile_img = alloc_image(tile_w, tile_h, img->depth&depth_bits, 0, 0);
  stream.size = tile_img->data_size+0x20000;
  stream.buf = (uint8_t*)bmf_new(stream.size);
  packer_reset();
  tile_buf = (uint8_t*)bmf_new(tile_h*tile_w);
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
    model_planes(tile_img, tile_buf, pl);
    int32_t cost = 8*(stream.cur-stream.buf);
    packer_flush();
    return cost;
  };
  nplanes = plane_count;
  y0 = (img->height-tile_h)>>1;
  dx = img->width-tile_w;
  off_y = y0*img->stride;
  srcp = img->pixels+plane_count*(dx>>1)+off_y;
  tile_src = tile_img->pixels;
  // The tile is the middle of the image, `tile_h` rows of it.  The condition
  // was `y0 < tile_h+y0` and the bound `dims[1]+((img->height-dims[1])>>1)`,
  // which is `tile_h+y0` again with `y0` spelled out; the loop runs `tile_h`
  // times either way.
  if( tile_h>0 ) {
    int32_t row_bytes = (uint16_t)tile_img->stride;
    uint8_t *dstp = tile_img->pixels;
    for( int32_t y = 0; y<tile_h; ++y ) {
      memcpy(dstp, srcp, row_bytes);
      dstp += row_bytes;
      srcp += img->stride;
    }
    nplanes = plane_count;
  }
  n_hard = 0;
  bits_total = 0;
  n_flagged = 0;
  best_bits = 0;
  if( nplanes>0 ) {
    hard = n_hard;
    best_flags = 0;
    pi = 0;
    while( 1 ) {
      plane_i = plane_desc[pi].src_plane;
      if( hard ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(plane_i, try_mode0);
        flags_s = best_flags;
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          flags_s = 0;
        best_flags = flags_s;
      }
      bits_f5 = probe_plane(plane_i, try_p1);
      packer_reset();
      f5 = best_flags;
      if( bits_f5<best_cost ) {
        best_cost = bits_f5;
        f5 = try_p1;
      }
      best_flags = f5;
      if( bits_f5<best_cost+(best_cost>>5)||hard ) {
        bits_f6 = probe_plane(plane_i, try_p2);
        f6 = best_flags;
        packer_reset();
        if( bits_f6<best_cost ) {
          best_cost = bits_f6;
          f6 = try_p2;
        }
        best_flags = f6;
        if( pi ) {
          {
try_flag8:
            bits_f8 = probe_plane(plane_i, try_refs);
            f8 = best_flags;
            packer_reset();
            if( bits_f8<best_cost ) {
              best_cost = bits_f8;
              f8 = try_refs;
            }
            best_flags = f8;
          }
          bits_f13 = probe_plane(plane_i, try_refs_p1);
          f13 = best_flags;
          packer_reset();
          if( bits_f13<best_cost ) {
            best_cost = bits_f13;
            f13 = try_refs_p1;
          }
          best_flags = f13;
          pred = f13&desc_predictor;
          // `bits_f13` was also kept in `rows[0]` for this one comparison,
          // which is the value the line above it already holds.
          if( pred==pred_p2||best_cost+(best_cost>>5)>bits_f13 ) {
            cost_f14 = probe_plane(plane_i, try_refs_p2);
            deep2 = cost_f14<best_cost;
            if( cost_f14<best_cost )
              best_cost = cost_f14;
            packer_reset();
            f14 = best_flags;
            if( deep2 )
              f14 = try_refs_p2;
            best_flags = f14;
            if( deep2 )
              pred = pred_p2;
          }
          goto keep_flag8;
        }
        pred = f6&desc_predictor;
      } else {
        if( pi )
          goto try_flag8;
        pred = (uint8_t)best_flags&desc_predictor;
      }
keep_flag8: { flag8 = (uint8_t)best_flags&desc_has_refs; }
      best_bits += best_cost;
      // `pred` is read for the p2 count before the next line rewrites it, so
      // the count is taken first.
      int32_t chose_p2 = pred==pred_p2;
      if( pred!=pred_p1 )
        pred = pred_mode0;
      hard += chose_p2;
      bits_total += pred;
      plane_cost[plane_i] = best_cost;
      n_flagged += flag8!=0;
      plane_desc[plane_i].flags = (uint8_t)best_flags;
      if( ++pi>=plane_count ) {
        n_hard = hard;
        break;
      }
    }
  }
  // The transposed tile, costed against the per-plane figures the search just
  // took.  Three pixel transposes here, all `transpose_image`: this one to try
  // the other orientation, and one of the two below to put back whichever
  // orientation lost.
  {
    transpose_image(tile_img, plane_count);
    bits_d = 0;
    if( plane_count>0 ) {
      pk = 0;
      while( 1 ) {
        pl_k = plane_desc[pk].src_plane;
        model_planes(tile_img, tile_buf, pl_k);
        bits = 8*(stream.cur-stream.buf);
        packer_flush();
        bits_d += bits;
        packer_reset();
        if( (uint32_t)(bits-(bits>>8))>plane_cost[pl_k] )
          break;
        if( ++pk>=plane_count ) {
          goto tile_restored;
        }
      }
      bits_d += best_bits+1;
    }
tile_restored:
    if( bits_d+(bits_d>>12)>=best_bits ) {
      // Transposed is no better: put the tile back and leave the image alone.
      transpose_image(tile_img, plane_count);
    } else {
      // Transposed is better, so the whole image turns with it and the tile
      // stays as it is.
      best_bits = bits_d;
      transpose_image(img, plane_count);
    }
  }
  free(tile_buf);
  if( plane_count>2 ) {
    if( bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        saved[desc_k] = plane_desc[desc_k];
      if( plane_count>0 ) {
        plane = 0;
        do {
          pi3 = plane;
          f2 = plane_desc[plane++].flags&desc_has_refs|try_p1;
          plane_desc[pi3].flags = f2;
        } while( plane<plane_count );
      }
      transform_planes(tile_img);
      bits_e = 8*(stream.cur-stream.buf);
      deep2 = bits_e<=best_bits;
      packer_flush();
      packer_reset();
      if( deep2 ) {
        best_bits = bits_e;
        nplanes_b = plane_count;
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k] = saved[desc_k];
        nplanes_b = plane_count;
        cand = 1;
      }
      if( nplanes_b<=2 )
        goto planes_done;
    } else {
      cand = 1;
    }
    if( n_hard+(cand==0) ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        saved[desc_k] = plane_desc[desc_k];
      nplanes_c = plane_count;
      if( plane_count>0 ) {
        pl_a = 0;
        do {
          pi0 = pl_a;
          f0 = plane_desc[pl_a++].flags&desc_has_refs|try_p2;
          plane_desc[pi0].flags = f0;
          nplanes_c = plane_count;
        } while( pl_a<plane_count );
      }
      if( nplanes_c==n_hard&&nplanes_c-1==n_flagged ) {
        cand = 0;
      } else {
        transform_planes(tile_img);
        bits_a = 8*(stream.cur-stream.buf);
        deep2 = bits_a<=best_bits;
        packer_flush();
        packer_reset();
        if( deep2 ) {
          best_bits = bits_a;
          n_hard = bits_a;
          if( plane_count-1==n_flagged ) {
            cand = 0;
          } else {
            for( desc_k = 0; desc_k<4; ++desc_k )
              saved[desc_k] = plane_desc[desc_k];
            if( plane_count>0 ) {
              pl_b = 0;
              do {
                pi1 = pl_b;
                f1 = plane_desc[pl_b++].nrefs;
                plane_desc[pi1].flags |= f1!=0 ? desc_has_refs : 0;
              } while( pl_b<plane_count );
            }
            transform_planes(tile_img);
            bits2 = 8*(stream.cur-stream.buf);
            deep2 = bits2<=bits_a;
            packer_flush();
            packer_reset();
            if( deep2 ) {
              cand = 0;
            } else {
              for( desc_k = 0; desc_k<4; ++desc_k )
                plane_desc[desc_k] = saved[desc_k];
              cand = 0;
            }
          }
        } else {
          for( desc_k = 0; desc_k<4; ++desc_k )
            plane_desc[desc_k] = saved[desc_k];
        }
      }
    }
  } else {
    cand = 1;
  }
planes_done:
  if( !n_hard&&plane_count>1 ) {
    tile_copy = (uint8_t*)bmf_new(tile_img->data_size);
    memcpy(tile_copy, tile_src, tile_img->data_size);
    for( desc_k = 0; desc_k<4; ++desc_k )
      saved[desc_k] = plane_desc[desc_k];
    if( plane_count>0 ) {
      pl_c = 0;
      do {
        pi4 = pl_c;
        f3 = plane_desc[pl_c++].flags&~desc_alt_model;
        plane_desc[pi4].flags = f3;
      } while( pl_c<plane_count );
    }
    transform_planes(tile_img);
    bits_b = 8*(stream.cur-stream.buf);
    packer_flush();
    packer_reset();
    if( bits_b>best_bits ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        plane_desc[desc_k] = saved[desc_k];
    } else {
      best_bits = bits_b;
      cand = 0;
    }
    if( n_flagged+bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        saved[desc_k] = plane_desc[desc_k];
      if( plane_count>0 ) {
        pl_d = 0;
        do
          plane_desc[pl_d++].flags = 0;
        while( pl_d<plane_count );
      }
      memcpy(tile_src, tile_copy, tile_img->data_size);
      transform_planes(tile_img);
      bits_c = 8*(stream.cur-stream.buf);
      deep2 = bits_c<=best_bits;
      packer_flush();
      packer_reset();
      if( deep2 ) {
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k] = saved[desc_k];
      }
    }
    free(tile_copy);
  }
  free(stream.buf);
  free((uint8_t*)tile_img);
  return cand;
}
