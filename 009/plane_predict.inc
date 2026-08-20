// plane_predict.inc -- the predictors and the colour transform -- what is subtracted from
// an image before it is modelled, and added back after.

// Predictor mode 0 undoes nothing: an image coded without a predictor has
// nothing subtracted from it.  The body is empty in BMF.exe too -- the encoder
// side calls through a table of three, and this is the entry the table needs at
// index 0.
// The MED / LOCO-I predictor, which JPEG-LS calls the median edge detector, and
// now written as the rule rather than as the branch tree MSVC compiled it to:
// pick `min(N,W)` at a rising edge, `max(N,W)` at a falling one, and the plane
// `N + W - NW` in between.
//
// Proven, not argued.  Both spellings evaluated over all 16,777,216 byte triples
// -- 0 disagreements -- which covers the two tie boundaries the case analysis
// has to get right and which a corpus run does not reach on purpose.
//
// The `(uint8_t)` casts here are provably non-wrapping, unlike the folding casts
// elsewhere in this tree: on the third path the gradient lies in `[lo, hi]` and
// on the other two the result is a neighbour.  No `clamp(0, 255)` is needed, and
// adding one -- as one of the plans supplying this stage proposes -- would be a
// no-op here and would break the bitstream anywhere the wrap *is* the algorithm.
static inline uint8_t med_predict(int32_t west, int32_t north, int32_t northwest) {
  const int32_t lo = min32(west, north), hi = max32(west, north);
  if( northwest>=hi )
    return (uint8_t)lo;
  if( northwest<=lo )
    return (uint8_t)hi;
  return (uint8_t)(west+north-northwest);
}

// The zigzag a MED residual takes on its way to a symbol, and its inverse:
// 0, -1, 1, -2, 2, ... becomes 0, 1, 2, 3, 4, ..., so the sign rides in the low
// bit and a small residual stays a small symbol.
//
// Both were built inline, five lines apiece, `fold` counting up in twos from
// each end of the table and `unfold` filling the odd and even slots in one
// pass.  Two spellings of one table and its inverse, and neither said so.
//
// Not the alt models' `alt_init_tables` pair, which buckets by
// `near_lossless_q` and is a different table with the same job.
static void med_fold_table(uint8_t* fold) {
  for( int32_t j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*j);
  }
}

static void med_unfold_table(uint8_t* unfold) {
  unfold[0] = 0;
  for( int32_t j = 0; j<127; ++j ) {
    unfold[2*j+1] = (uint8_t)(-1-j);
    unfold[2*j+2] = (uint8_t)(1+j);
  }
  unfold[255] = (uint8_t)(-1-127);
}

uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done, last;
  alignas(16) uint8_t fold[272];
  uint8_t *p = pixels+height*width;
  uint8_t *up = &p[-width];
  med_fold_table(fold);
  // Same again a row up: `height - 1` rows, so `if( height != 1 )` is the
  // count being non-zero.
  for( int32_t rows_left = height-1; rows_left; --rows_left ) {
    // The `if( width != 1 )` guard and the `do` inside it are one statement:
    // the count is `width - 1`, so the guard is the loop's own test.
    for( int32_t x_left = width-1; x_left; --x_left ) {
      const int32_t north = (uint8_t)*--up;
      --p;
      const uint8_t pred = med_predict(p[-1], north, p[-width-1]);
      const int32_t code = (uint8_t)fold[(uint8_t)(*p-pred)];
      *p = code;
      hist_bump(code);
    }
    // The first column has no west neighbour, so it takes a plain vertical
    // difference.
    --up;
    --p;
    last = (uint8_t)fold[(uint8_t)(*p-p[-width])];
    *p = last;
    hist_bump(last);
  }
  if( width!=1 ) {
    last = width-1;
    const int32_t pairs = last/2;
    {
      uint32_t k = 0;
      for( int32_t ofs = 0; k<(uint32_t)pairs; ++k, ofs -= 2 ) {
        const uint8_t left = p[ofs-2];
        p[ofs-1] = fold[(uint8_t)(p[ofs-1]-left)];
        p[ofs-2] = fold[(uint8_t)(left-p[ofs-3])];
      }
      done = 2*k+1;
    }
    if( last>(done-1) ) {
      uint8_t *q = p-done;
      last = (uint8_t)(*q-*((int8_t*)q-1));
      *q = fold[(uint8_t)last];
    }
  }
  return last;
}

// One plane out of the interleaved image and back again: `dst[i]` is
// `pixels[plane + i*plane_count]`.  This is what both coders degenerate to
// when the plane has no reference planes to predict from, and what
// `expand_image` does inline for the one plane it decodes on its own -- three
// copies of the same two loops, each with the one-plane case spelled out as
// the `memcpy` it is.
//
// `for` rather than the original `do`/`while`, which ran once whatever the
// count: an image with no pixels does not reach here, and the bound is the
// thing worth being able to read.
// `n` bytes from `src` to `dst`, one of the two strided by `plane_count` and
// the other flat -- so a `memcpy` exactly when the image has one plane and
// both strides are one.
static void strided_copy(uint8_t* dst, int32_t dstep,
                         const uint8_t* src, int32_t sstep, int32_t n) {
  if( dstep==1&&sstep==1 ) {
    memcpy(dst, src, n);
    return;
  }
  for( int32_t i = 0; i<n; ++i, dst += dstep, src += sstep )
    *dst = *src;
}

static void deinterleave_plane(const BmfImage* img, uint8_t* dst, int32_t plane) {
  strided_copy(dst, 1, &img->pixels[plane], plane_count, bmf_pixels(img));
}

// The inverse.  `colour_transform` reached for the gather by name and
// `interleave_plane` wrote the scatter out, so the two did not look like a
// pair; writing them both this way round is what shows that the only
// difference is which side carries the stride.
static void interleave_flat(BmfImage* img, const uint8_t* src, int32_t plane) {
  strided_copy(&img->pixels[plane], plane_count, src, 1, bmf_pixels(img));
}

// What a plane's descriptor says about the transform between it and the planes
// it references: which plane, the DC offset, and the three weights.
//
// `interleave_plane` and `colour_transform` each decoded this in the same
// nineteen lines.  The round that named this said those nineteen were the whole
// of what the two share, "the loops under them are inverses and have nothing
// else in common" -- which was a sentence about how they were spelled.  Each
// loop is one blend of the referenced planes and then a sign: the forward
// writes `x - dc - blend` and the inverse `blend + dc + x`, over the same
// operands in the same order.  They are `code_colour_plane<f_DEC>` below.
//
// `by_weights` is the case worth naming.  A two-reference plane whose weights
// are 128 and 0 reads only one plane after all, and which one depends on which
// of the two is zero; that is why a descriptor saying `nrefs == 2` can take the
// one-reference path below.
struct PlaneTransform {
  int32_t to_ref0, to_ref1, mode, dc, wgt0, wgt1, wgt2;
  bool by_weights;

  // What the referenced planes predict of this one at `p`, in the units the
  // residual is in.  Two references read the two planes the descriptor names;
  // three read the three bytes before `p`, which are the other components of
  // the same pixel.  Only these two modes reach here -- one reference is the
  // `by_weights` path above and no references is the flat copy.
  //
  // Both were written four different ways across the encoder and the decoder,
  // one casting to `uint32_t` mid-expression, one summing into an `int32_t`
  // and casting the total, two splitting the sum across an extra local.  All
  // four are the same thirty-two bits; unsigned throughout is the one that
  // cannot overflow signed.
  //
  // The **63** in the three-reference form is not a typo for 64.  Model B
  // computes this same blend for itself, on the interleaved path, in
  // `alt_p1_code.inc`'s `plane_mix3` -- and rounds it with 64.  The
  // two-reference forms agree at 40; only this one differs, by one.  Merging
  // the two helpers changes the output of every four-plane image, so they stay
  // apart.  See the note over `plane_mix2` for the reachability count.
  uint32_t blend(const uint8_t* p) const {
    if( mode==2 )
      return (wgt0*(uint32_t)p[to_ref0]+wgt1*(uint32_t)p[to_ref1]+40)>>7;
    return (wgt1*(uint32_t)p[-2]+wgt0*(uint32_t)p[-3]+wgt2*(uint32_t)p[-1]+63)>>7;
  }
};

static PlaneTransform plane_transform(int32_t plane) {
  PlaneTransform t;
  t.to_ref0 = plane_desc[0].src_plane-plane;
  t.to_ref1 = plane_desc[1].src_plane-plane;
  t.mode = plane_desc[plane].nrefs;
  t.dc = plane_desc[plane].dc;
  t.wgt1 = plane_desc[plane].weight1;
  t.wgt0 = plane_desc[plane].weight0;
  t.wgt2 = plane_desc[plane].weight2;
  t.by_weights = false;
  if( t.mode==2&&t.wgt0+t.wgt1==128 ) {
    if( !t.wgt1 ) {
      t.by_weights = true;
    } else if( !t.wgt0 ) {
      t.to_ref0 = plane_desc[1].src_plane-plane;
      t.by_weights = true;
    }
  }
  return t;
}

// One plane against the planes it references, in whichever direction.  Four
// modes, chosen by `desc_has_refs` and `nrefs`:
//
//   no transform      copy the plane, interleaved to flat or back
//   one reference     `x - dc - ref`, a plain difference
//   two references    `x - dc - (w0*a + w1*b + 40)/128`
//   three references  `x - dc - (w0*a + w1*b + w2*c + 63)/128`
//
// The two-reference case degenerates to the one-reference case when one weight
// is 128 and the other 0, and the code takes that path explicitly -- which is
// `plane_transform`'s `by_weights` flag, and why a two-reference descriptor can
// end up reading only one plane.
//
// `f_DEC` is which way: the encoder reads the image and writes residuals to
// `side`, the decoder reads residuals from `side` and writes the image back.
// Every `if constexpr( f_DEC )` in here is that and nothing else.
//
// The blends were written four different ways between the two bodies -- one
// cast to `uint32_t` mid-expression, one summing into an `int32_t` and casting
// the total, two splitting the sum across an extra local -- and all four are
// the same thirty-two bits.  Spelling them unsigned throughout is what let the
// two loops become one, and it takes a signed overflow out of the forward
// direction on the way.
template<int32_t f_DEC>
static void code_colour_plane(BmfImage* img, uint8_t* side, int32_t plane) {
  uint8_t* base = &img->pixels[plane];
  if( !(plane_desc[plane].flags&desc_has_refs) ) {
    if constexpr( f_DEC )
      interleave_flat(img, side, plane);
    else
      deinterleave_plane(img, side, plane);
    return;
  }
  int32_t stride = plane_count;
  int32_t n = bmf_pixels(img);
  const PlaneTransform t = plane_transform(plane);
  // The same dispatch `plane_transform` makes, written the same way round.
  // Here it was two nested `if`s with the one-reference path falling out of
  // the bottom of both, which is `by_weights || nrefs == 1` reached the long
  // way.
  if( t.by_weights||plane_desc[plane].nrefs==1 ) {
    const uint8_t *ref = base+t.to_ref0;
    for( int32_t i = 0, ofs = 0; i<n; ++i, ofs += stride ) {
      if constexpr( f_DEC )
        base[ofs] = ref[ofs]+t.dc+side[i];
      else
        side[i] = base[ofs]-t.dc-ref[ofs];
    }
    return;
  }
  // Two references and three were two loops of six lines that differed in one
  // expression, which is `PlaneTransform::blend`.
  if( t.mode==2||t.mode==3 ) {
    uint8_t *p = base;
    for( int32_t left = n; left; --left, p += stride ) {
      const uint32_t blend = t.blend(p);
      if constexpr( f_DEC )
        *p = (uint8_t)(blend+t.dc+*side++);
      else
        *side++ = (uint8_t)(*p-t.dc-blend);
    }
  }
}

void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane) {
  code_colour_plane<1>(img, src, plane);
}

void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane) {
  code_colour_plane<0>(img, dst, plane);
}

uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done, k, x_left;
  alignas(16) uint8_t unfold[256];
  uint8_t *p = pixels+1;
  med_unfold_table(unfold);
  int32_t rows_left = height-1;
  if( width!=1 ) {
    // The first row has nothing above it, so it is a plain left-to-right
    // difference.  The original unrolled it by two -- the `pairs` loop -- with
    // the odd pixel after it.
    uint32_t row_rest = width-1;
    uint32_t pairs = row_rest/2;
    if( pairs ) {
      for( k = 0; k<pairs; ++k ) {
        uint8_t cur = pixels[2*k]+unfold[pixels[2*k+1]];
        pixels[2*k+1] = cur;
        pixels[2*k+2] = cur+(unfold[pixels[2*k+2]]);
        p = &pixels[2*k+3];
      }
      done = 2*k+1;
    } else {
      done = 1;
    }
    if( row_rest>done-1 ) {
      pixels[done] = pixels[done-1]+unfold[pixels[done]];
      p = &pixels[done+1];
    }
  }
  if( height==1 )
    return p;
  if( width==1 ) {
    // One pixel to a row, so the row loop and the column loop are one loop and
    // `med_predict` never runs: a pixel's only neighbour is the one above it.
    //
    // The recovery said this with a `while( 1 )` wrapped round the first pixel
    // of a row which broke out of itself for a wide image and looped for a
    // narrow one -- one loop doing two jobs, with the narrow case's `return`
    // three levels in and its `x_left = 0` assigned every turn and never read.
    for( ; rows_left; --rows_left ) {
      *p = unfold[(uint8_t)*p]+p[-width];
      ++p;
    }
    return p;
  }
  uint8_t *up = &p[-width];
  for( ; rows_left; --rows_left ) {
    // The first pixel of a row has no west neighbour, so it takes the one
    // above it rather than a MED prediction.
    ++up;
    *p = unfold[(uint8_t)*p]+p[-width];
    ++p;
    for( x_left = width-1; x_left; --x_left ) {
      const uint8_t pred = med_predict(p[-1], (uint8_t)*up, p[-width-1]);
      *p = pred+unfold[(uint8_t)*p];
      ++up;
      ++p;
    }
  }
  return p;
}

// The k'th plane in descriptor order, with `plane_predictor` -- which is how a
// descriptor reaches the model at all -- set from it.  Both walks over the
// planes open with these two lines, the encoder here and the decoder in
// `expand_image`, and until this round both also spelled the loop around them
// as `++k` first and `plane_desc[k - 1]` after it under an `if( plane_count >
// 0 )` guard that a `for` does not need.
static int32_t plane_in_order(int32_t k) {
  const int32_t plane = plane_desc[k].src_plane;
  plane_predictor = plane_desc[plane].flags&desc_predictor;
  return plane;
}

void transform_planes(BmfImage* p_i) {
  int32_t i;
  memset(hist_scratch, 0, 4096);
  // A scratch image in the coded buffer: one header's worth of bytes reserved,
  // then a copy of the header, then the pixels.  The reserved run and the
  // header are the same size, which is why `hdr->pixels` is `stream.buf + 32`
  // -- and what the copy below was addressing by number.
  //
  // `sizeof(BmfImage)` rather than the 16 it is: this was the last place in
  // the tree where an object's layout was written as arithmetic instead of
  // declared, which is the row `shape.py` counts first.
  static_assert(sizeof(BmfImage) == 16, "the reserved run is one header");
  BmfImage *hdr = (BmfImage*)(stream.buf+sizeof(BmfImage));
  *hdr = *p_i;
  uint8_t* src_pixels = p_i->pixels;
  memcpy(hdr->pixels, p_i->pixels, p_i->data_size);
  uint8_t *tmp = (uint8_t*)bmf_new(p_i->width*p_i->height);
  for( int32_t k = 0; k<plane_count; ++k ) {
    const int32_t plane = plane_in_order(k);
    const int32_t predictor = plane_predictor;
    const int32_t alt = (plane_desc[plane].flags&desc_alt_model)!=0;
    plane_alt_model = alt;
    // A plane is transformed when it has references or a predictor, and not
    // when it carries the alternate model -- that family does its own.  The
    // condition held the whole of the loop body and had nothing after it, so it
    // is a name and an early exit.
    const bool transform_it =
        ((plane_desc[plane].flags&desc_has_refs)!=0||predictor)&&!alt;
    if( !transform_it )
      continue;
    colour_transform(hdr, tmp, plane);
    // `!= pred_p2 && == pred_p1` is `== pred_p1`: the two are distinct
    // values of the same two-bit field, so the outer test can only be
    // true when the inner one is.
    if( plane_predictor==pred_p1 )
      predict_med(tmp, p_i->width, p_i->height);
    int32_t n = p_i->width*p_i->height;
    int32_t stride = plane_count;
    if( plane_count==1 ) {
      memcpy(&p_i->pixels[plane], tmp, n);
    } else {
      uint8_t *dst = &p_i->pixels[plane];
      int32_t ofs = 0;
      for( i = 0; i<n; ++i ) {
        dst[ofs] = tmp[i];
        ofs += stride;
      }
    }
  }
  free(tmp);
  model_plane(p_i, src_pixels, src_pixels);
}
