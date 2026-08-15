// plane_predict.inc -- the predictors and the colour transform -- what is subtracted from
// an image before it is modelled, and added back after.

// Predictor mode 0 undoes nothing: an image coded without a predictor has
// nothing subtracted from it.  The body is empty in BMF.exe too -- the encoder
// side calls through a table of three, and this is the entry the table needs at
// index 0.
void expand_predictor_mode0(int32_t i) {
}

// The MED / LOCO-I predictor, which JPEG-LS calls the median edge detector:
// pick `min(N,W)` at a rising edge, `max(N,W)` at a falling one, and the plane
// `N + W - NW` in between.  Written out twice, once in each direction, in the
// two nested comparisons the original compiled to -- which is the same rule
// with the `min`/`max` inlined into the branch.
//
// A published predictor whose name appears nowhere is the worst kind of
// duplication: two copies, and a reader who does not already recognise it has
// to derive the rule from the branches, twice.
static inline uint8_t med_predict(int32_t west, int32_t north, int32_t northwest) {
  if( west<north ) {
    if( northwest<west )
      return (uint8_t)north;
    if( northwest<=north )
      return (uint8_t)(north+west-northwest);
  } else {
    if( northwest>west )
      return (uint8_t)north;
    if( northwest>=north )
      return (uint8_t)(north+west-northwest);
  }
  return (uint8_t)west;
}

uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done;
  uint32_t j, last;
  alignas(16) uint8_t fold[272];
  uint8_t *p = pixels+height*width;
  uint8_t *up = &p[-width];
  for( j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*(int32_t)j);
  }
  int32_t rows_left = height-1;
  if( height!=1 ) {
    do {
      int32_t x_left = width-1;
      if( width!=1 ) {
        do {
          int32_t north = (uint8_t)*--up;
          --p;
          uint8_t pred = med_predict(p[-1], north, p[-width-1]);
          int32_t code = (uint8_t)fold[(uint8_t)(*p-pred)];
          *p = code;
          ++*(uint32_t*)&hist_scratch[4*code];
          --x_left;
        } while( x_left );
      }
      --up;
      --p;
      last = (uint8_t)fold[(uint8_t)(*p-p[-width])];
      *p = last;
      ++*(uint32_t*)&hist_scratch[4*last];
      --rows_left;
    } while( rows_left );
  }
  if( width!=1 ) {
    last = width-1;
    int32_t pairs = (last)/2;
    if( pairs ) {
      uint32_t k = 0;
      int32_t ofs = 0;
      do {
        uint8_t left = p[ofs-2];
        ++k;
        p[ofs-1] = fold[(uint8_t)(p[ofs-1]-left)];
        p[ofs-2] = fold[(uint8_t)(left-p[ofs-3])];
        ofs -= 2;
      } while( k<(uint32_t)pairs );
      done = 2*k+1;
    } else {
      done = 1;
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
static void deinterleave_plane(const BmfImage* img, uint8_t* dst, int32_t plane) {
  const uint8_t* src = &img->pixels[plane];
  int32_t n = bmf_pixels(img);
  if( plane_count==1 ) {
    memcpy(dst, src, n);
    return;
  }
  for( int32_t i = 0, ofs = 0; i<n; ++i, ofs += plane_count )
    dst[i] = src[ofs];
}

// What a plane's descriptor says about the transform between it and the planes
// it references: which plane, the DC offset, and the three weights.
//
// `interleave_plane` and `colour_transform` each decoded this in the same
// nineteen lines, and those nineteen are the whole of what the two share --
// the loops under them are inverses and have nothing else in common.
//
// `by_weights` is the case worth naming.  A two-reference plane whose weights
// are 128 and 0 reads only one plane after all, and which one depends on which
// of the two is zero; that is why a descriptor saying `nrefs == 2` can take the
// one-reference path below.
struct PlaneTransform {
  int32_t to_ref0, to_ref1, mode, dc, wgt0, wgt1, wgt2;
  bool by_weights;
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

void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane) {
  uint8_t* base = &img->pixels[plane];
  if( !(plane_desc[plane].flags&desc_has_refs) ) {
    int32_t n_flat = bmf_pixels(img);
    if( plane_count==1 ) {
      memcpy(base, src, n_flat);
      return;
    }
    for( int32_t i = 0, ofs = 0; i<n_flat; ++i, ofs += plane_count )
      base[ofs] = src[i];
    return;
  }
  int32_t stride = plane_count;
  int32_t n = bmf_pixels(img);
  const PlaneTransform t = plane_transform(plane);
  if( !t.by_weights ) {
    if( plane_desc[plane].nrefs!=1 ) {
      if( t.mode==2 ) {
        int32_t left2 = bmf_pixels(img);
        uint8_t *p2 = base;
        do {
          int32_t x2 = t.dc+(uint8_t)*src++;
          *p2 = (uint8_t)(((t.wgt0*p2[t.to_ref0]+t.wgt1*(uint32_t)p2[t.to_ref1]+40)>>7)+x2);
          p2 += stride;
          --left2;
        } while( left2 );
      } else if( t.mode==3 ) {
        int32_t left3 = bmf_pixels(img);
        uint8_t *p3 = base;
        do {
          int32_t x3 = t.dc+(uint8_t)*src++;
          *p3 = (uint8_t)(((t.wgt1*(uint8_t)*(p3-2)+t.wgt0*(uint8_t)*(p3-3)+t.wgt2*(uint32_t)(uint8_t)*(p3-1)+63)>>7)+x3);
          p3 += stride;
          --left3;
        } while( left3 );
      }
      return;
    }
  }
  uint8_t *ref = base+t.to_ref0;
  int32_t i2 = 0;
  int32_t ofs2 = 0;
  do {
    base[ofs2] = ref[ofs2]+t.dc+src[i2];
    ofs2 += stride;
    ++i2;
  } while( i2<n );
}

// Subtract from one plane whatever the other planes predict of it, and write
// the residual to `dst`.  Four modes, chosen by `desc_has_refs` and `nrefs`:
//
//   no transform      copy the plane out, interleaved to flat
//   one reference     `x - dc - ref`, a plain difference
//   two references    `x - dc - (w0*a + w1*b + 40)/128`
//   three references  `x - dc - (w0*a + w1*b + w2*c + 63)/128`
//
// The two-reference case degenerates to the one-reference case when one weight
// is 128 and the other 0, and the code takes that path explicitly -- which is
// the `by_weights` flag below, and why a two-reference descriptor can end up
// reading only one plane.
void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane) {
  uint8_t *src = &img->pixels[plane];
  if( !(plane_desc[plane].flags&desc_has_refs) ) {
    deinterleave_plane(img, dst, plane);
    return;
  }
  int32_t stride = plane_count;
  int32_t n = bmf_pixels(img);
  const PlaneTransform t = plane_transform(plane);
  if( t.by_weights||plane_desc[plane].nrefs==1 ) {
    uint8_t* ref = src+t.to_ref0;
    int32_t i2 = 0;
    int32_t ofs2 = 0;
    do {
      uint8_t d = src[ofs2]-t.dc-ref[ofs2];
      ofs2 += stride;
      dst[i2++] = d;
    } while( i2<n );
    return;
  }
  if( t.mode==2 ) {
    int32_t left2 = bmf_pixels(img);
    uint8_t *p2 = src;
    do {
      int32_t x2 = *p2-t.dc;
      uint32_t blend = t.wgt0*p2[t.to_ref0]+t.wgt1*p2[t.to_ref1]+40;
      p2 += stride;
      *dst++ = (uint8_t)(x2-(blend>>7));
      --left2;
    } while( left2 );
  } else if( t.mode==3 ) {
    int32_t left3 = bmf_pixels(img);
    uint8_t *p3 = src;
    do {
      int32_t x3 = *p3-t.dc;
      int32_t wsum = t.wgt1**(p3-2)+t.wgt0**(p3-3);
      int32_t w12sum = t.wgt2**(p3-1);
      p3 += stride;
      *dst++ = (uint8_t)(x3-((uint32_t)(wsum+w12sum+63)>>7));
      --left3;
    } while( left3 );
  }
}

uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done;
  int32_t rows_left;
  uint32_t j, row_rest, k, x_left;
  alignas(16) uint8_t unfold[256];
  uint8_t *p = pixels+1;
  unfold[0] = 0;
  for( j = 0; j<127; ++j ) {
    unfold[2*j+1] = (uint8_t)(-1-(int32_t)j);
    unfold[2*j+2] = (uint8_t)(1+j);
  }
  unfold[255] = (uint8_t)(-1-127);
  if( width==1 ) {
    rows_left = height-1;
    if( height==1 )
      return p;
    row_rest = 0;
  } else {
    row_rest = width-1;
    uint32_t pairs = (row_rest)/2;
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
    rows_left = height-1;
    if( height==1 )
      return p;
  }
  {
    uint8_t *up = &p[-width];
    do {
      while( 1 ) {
        ++up;
        *p = unfold[(uint8_t)*p]+p[-width];
        x_left = row_rest;
        ++p;
        if( width!=1 )
          break;
        if( !--rows_left )
          return p;
      }
      do {
        uint8_t pred = med_predict(p[-1], (uint8_t)*up, p[-width-1]);
        *p = pred+unfold[(uint8_t)*p];
        ++up;
        ++p;
        --x_left;
      } while( x_left );
      --rows_left;
    } while( rows_left );
  }
  return p;
}

void transform_planes(BmfImage* p_i) {
  int32_t i;
  memset(hist_scratch, 0, 4096);
  // A scratch image in the coded buffer: sixteen bytes reserved, then a copy
  // of the header, then the pixels.  `BmfImage` is sixteen bytes, so the
  // second 16 and the 32 are the same offset said twice -- `hdr->pixels` is
  // `stream.buf+32`, which is what the copy below was addressing by number.
  BmfImage *hdr = (BmfImage*)(stream.buf+16);
  *hdr = *p_i;
  uint8_t* src_pixels = p_i->pixels;
  memcpy(hdr->pixels, p_i->pixels, p_i->data_size);
  uint8_t *tmp = (uint8_t*)bmf_new(p_i->width*p_i->height);
  if( plane_count>0 ) {
    int32_t k = 0;
    do {
      ++k;
      int32_t plane = plane_desc[k-1].src_plane;
      int32_t predictor = plane_desc[plane].flags&desc_predictor;
      plane_predictor = predictor;
      int32_t alt = (plane_desc[plane].flags&desc_alt_model)!=0;
      plane_alt_model = alt;
      if( ((plane_desc[plane].flags&desc_has_refs)!=0||predictor)&&!alt ) {
        colour_transform(hdr, tmp, plane);
        if( plane_predictor!=pred_p2 ) {
          if( plane_predictor==pred_p1 ) {
            predict_med(tmp, p_i->width, p_i->height);
          }
        }
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
    } while( k<plane_count );
  }
  free(tmp);
  {
    model_plane(p_i, src_pixels, src_pixels);
  }
}
