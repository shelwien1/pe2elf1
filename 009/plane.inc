// plane.inc -- one plane in and out of the model, whichever model the header names.

// Which of the two alternate models this plane's descriptor names, and the
// call it makes: p1 or p2, and within each the eight-bit body or the generic
// one.  Answers whether it handled the plane; `false` is the main model, which
// is the one thing the encoder and the decoder do differently -- 100 lines
// written out in `model_plane` against a call to `unmodel_plane_slow`.
//
// Sixteen lines apiece in the two dispatchers, and the only differences were
// `_encode` against `_decode` and the fourth argument the two eight-bit
// encoders take.  That is the whole of what `pairshare.py` measured as the
// `model_plane`/`unmodel_plane` pair's 14 shared lines of 170.
//
// `plane_alt_model` with a predictor that is neither 1 nor 2 codes nothing at
// all and does *not* fall through to the main model.  That is what the
// original's nesting says and it is why this answers `true` for any
// `plane_alt_model`, not just for the two arms that do something.
template<int32_t f_DEC>
static bool alt_model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  if( !plane_alt_model )
    return false;
  const bool d8 = (p_i->depth&depth_bits)==8;
  if( plane_predictor==pred_p1 ) {
    if( d8 ) {
      if( f_DEC )
        alt_model_p1_d8_decode(pixels, p_i->width, p_i->height);
      else
        alt_model_p1_d8_encode(pixels, p_i->width, p_i->height, raw);
    } else if( f_DEC ) {
      alt_model_p1_decode(p_i, pixels);
    } else {
      alt_model_p1_encode(p_i, pixels);
    }
  } else if( plane_predictor==pred_p2 ) {
    if( d8 ) {
      if( f_DEC )
        alt_model_p2_d8_decode(pixels, p_i->width, p_i->height);
      else
        alt_model_p2_d8_encode(pixels, p_i->width, p_i->height, raw);
    } else if( f_DEC ) {
      alt_model_p2_decode(p_i, pixels);
    } else {
      alt_model_p2_encode(p_i, pixels);
    }
  }
  return true;
}

void unmodel_plane(BmfImage* p_i, uint8_t* out) {
  if( alt_model_plane<1>(p_i, out, nullptr) )
    return;
  ModelBlock* blk = new_model_block(p_i->width, p_i->height,
                                    p_i->depth&depth_bits);
  blk->unmodel_plane_slow(out);
  if( blk )
    blk->free_workspace(1);
}

void model_planes(BmfImage* img, uint8_t* pixels, int32_t plane) {
  plane_predictor = plane_desc[plane].flags&desc_predictor;
  plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
  colour_transform(img, pixels, plane);
  // 1,024 bytes.  The original zeroed them as a `memset` of the unaligned head,
  // one aligned 16-byte store at the tail, and a loop of seven more stepping
  // back by 112 -- SSE, unrolled sevenfold, over a buffer whose start it had to
  // round up to 16 first.  The three ranges cover [0, 1023] whatever the
  // rounding takes: the head covers everything below the aligned start (at most
  // 15 bytes), the loop covers 1,008 from there, and the tail store covers what
  // the loop's own offset pushed past the end.  This runs once per plane.
  memset(hist_scratch, 0, 1024);
  {
    BmfImage hdr = *img;
    hdr.depth = depth_one_plane;
    if( plane_predictor==pred_p1&&!plane_alt_model )
      predict_med(pixels, img->width, img->height);
    model_plane(&hdr, pixels, pixels);
  }
}
