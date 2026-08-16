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

// The four descriptors, kept and put back.  Every whole-image trial below
// edits `plane_desc` in place, measures the result, and either keeps it or
// undoes it -- ten copies of these three lines between them, each with its own
// spelling of the loop index.
static void save_descriptors(PlaneDesc* saved) {
  for( int32_t k = 0; k<4; ++k )
    saved[k] = plane_desc[k];
}

static void restore_descriptors(const PlaneDesc* saved) {
  for( int32_t k = 0; k<4; ++k )
    plane_desc[k] = saved[k];
}

// Ask every plane for the second predictor, keeping nothing of what it was
// asked for before but whether it may use its references.
static void offer_p2_to_all() {
  for( int32_t k = 0; k<plane_count; ++k )
    plane_desc[k].flags = plane_desc[k].flags&desc_has_refs|try_p2;
}

// Let every plane that has references use them.
static void allow_refs_where_present() {
  for( int32_t k = 0; k<plane_count; ++k )
    plane_desc[k].flags |= plane_desc[k].nrefs!=0 ? desc_has_refs : 0;
}

// Transform the planes the way the descriptors currently say, and answer what
// that cost in bits.  Written out five times, and the order is the whole of
// it: the count has to be taken before the packer is flushed, and the packer
// has to be reset before the next trial.  Four of the five spelled the answer
// into a differently-named local, so the five did not look like one thing.
//
// Four callers then wrote `deep2 = bits <= best; if( deep2 )` -- one name,
// four unrelated one-shot conditions, none of them read again after the `if`
// that follows.  They are the conditions now.
static int32_t transform_cost(BmfImage* tile_img) {
  transform_planes(tile_img);
  const int32_t bits = 8*(stream.cur-stream.buf);
  packer_flush();
  packer_reset();
  return bits;
}

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
  int32_t bits_a;
  int32_t tile_h, y0, best_cost, pred, cand, plane, nplanes_b, pl_k, bits, bits_f13;
  int32_t tile_w = img->width;
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
  BmfImage *tile_img = alloc_image(tile_w, tile_h, img->depth&depth_bits, 0, 0);
  stream_open(tile_img->data_size);
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
  // Cost one candidate for this plane and keep it if it is cheaper than the
  // best so far.  Five sites wrote this out, each spelling the incumbent flags
  // into a private name (`f5`, `f6`, `f8`, `f13`, `f14`) that the `if` either
  // overwrote or handed straight back -- five names for one variable.
  //
  // `won` is separate from the returned cost because a tie leaves `best_cost`
  // equal to the candidate's cost without the candidate being taken, so the
  // one site that needs to know cannot read it back off `best_cost`.
  bool won = false;
  auto probe_keep = [&](int32_t pl, int32_t flags) {
    const int32_t cost = probe_plane(pl, flags);
    packer_reset();
    won = cost<best_cost;
    if( won ) {
      best_cost = cost;
      best_flags = flags;
    }
    return cost;
  };
  int32_t nplanes = plane_count;
  y0 = (img->height-tile_h)>>1;
  int32_t dx = img->width-tile_w;
  int32_t off_y = y0*img->stride;
  uint8_t *srcp = img->pixels+plane_count*(dx>>1)+off_y;
  uint8_t *tile_src = tile_img->pixels;
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
  int32_t n_hard = 0;
  int32_t bits_total = 0;
  int32_t n_flagged = 0;
  int32_t best_bits = 0;
  if( nplanes>0 ) {
    int32_t hard = n_hard;
    best_flags = 0;
    pi = 0;
    while( 1 ) {
      plane_i = plane_desc[pi].src_plane;
      if( hard ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(plane_i, try_mode0);
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          best_flags = 0;
      }
      int32_t bits_f5 = probe_keep(plane_i, try_p1);
      if( bits_f5<best_cost+(best_cost>>5)||hard ) {
        probe_keep(plane_i, try_p2);
        if( pi ) {
          {
try_flag8:
            probe_keep(plane_i, try_refs);
          }
          bits_f13 = probe_keep(plane_i, try_refs_p1);
          pred = best_flags&desc_predictor;
          // `bits_f13` was also kept in `rows[0]` for this one comparison,
          // which is the value the line above it already holds.
          if( pred==pred_p2||best_cost+(best_cost>>5)>bits_f13 ) {
            probe_keep(plane_i, try_refs_p2);
            if( won )
              pred = pred_p2;
          }
          goto keep_flag8;
        }
        pred = best_flags&desc_predictor;
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
    int32_t bits_d = 0;
    if( plane_count>0 ) {
      int32_t pk = 0;
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
      save_descriptors(saved);
      if( plane_count>0 ) {
        plane = 0;
        do {
          int32_t pi3 = plane;
          uint8_t f2 = plane_desc[plane++].flags&desc_has_refs|try_p1;
          plane_desc[pi3].flags = f2;
        } while( plane<plane_count );
      }
      int32_t bits_e = transform_cost(tile_img);
      if( bits_e<=best_bits ) {
        best_bits = bits_e;
        nplanes_b = plane_count;
        cand = 0;
      } else {
        restore_descriptors(saved);
        nplanes_b = plane_count;
        cand = 1;
      }
      if( nplanes_b<=2 )
        goto planes_done;
    } else {
      cand = 1;
    }
    if( n_hard+(cand==0) ) {
      save_descriptors(saved);
      offer_p2_to_all();
      // `nplanes_c` was `plane_count` assigned before this loop and again on
      // every turn of it, and nothing here writes the count -- three spellings
      // of one read.
      if( plane_count==n_hard&&plane_count-1==n_flagged ) {
        cand = 0;
      } else {
        bits_a = transform_cost(tile_img);
        if( bits_a<=best_bits ) {
          best_bits = bits_a;
          n_hard = bits_a;
          if( plane_count-1==n_flagged ) {
            cand = 0;
          } else {
            save_descriptors(saved);
            allow_refs_where_present();
            int32_t bits2 = transform_cost(tile_img);
            if( bits2>bits_a )
              restore_descriptors(saved);
            cand = 0;
          }
        } else {
          restore_descriptors(saved);
        }
      }
    }
  } else {
    cand = 1;
  }
planes_done:
  if( !n_hard&&plane_count>1 ) {
    uint8_t *tile_copy = (uint8_t*)bmf_new(tile_img->data_size);
    memcpy(tile_copy, tile_src, tile_img->data_size);
    save_descriptors(saved);
    if( plane_count>0 ) {
      int32_t pl_c = 0;
      do {
        int32_t pi4 = pl_c;
        uint8_t f3 = plane_desc[pl_c++].flags&~desc_alt_model;
        plane_desc[pi4].flags = f3;
      } while( pl_c<plane_count );
    }
    int32_t bits_b = transform_cost(tile_img);
    if( bits_b>best_bits ) {
      restore_descriptors(saved);
    } else {
      best_bits = bits_b;
      cand = 0;
    }
    if( n_flagged+bits_total ) {
      save_descriptors(saved);
      if( plane_count>0 ) {
        int32_t pl_d = 0;
        do
          plane_desc[pl_d++].flags = 0;
        while( pl_d<plane_count );
      }
      memcpy(tile_src, tile_copy, tile_img->data_size);
      int32_t bits_c = transform_cost(tile_img);
      if( bits_c<=best_bits ) {
        cand = 0;
      } else {
        restore_descriptors(saved);
      }
    }
    free(tile_copy);
  }
  free(stream.buf);
  free((uint8_t*)tile_img);
  return cand;
}
