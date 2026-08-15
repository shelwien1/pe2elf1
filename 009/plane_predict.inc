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
          uint8_t pred = med_predict((uint8_t)p[-1], north, (uint8_t)p[-width-1]);
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
    int32_t pairs = (width-1)/2;
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

void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane) {
  int32_t to_ref0;
  uint8_t* base = &img->pixels[plane];
  if( !(plane_desc[plane].flags&8) ) {
    int32_t n_flat = bmf_pixels(img);
    int32_t step = plane_count;
    if( plane_count==1 ) {
      memcpy(base, src, n_flat);
      return;
    }
    int32_t i = 0;
    int32_t ofs = 0;
    do {
      base[ofs] = src[i];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return;
  }
  int32_t stride = plane_count;
  int32_t n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[0].src_plane-plane;
  int32_t to_ref1 = plane_desc[1].src_plane-plane;
  int32_t mode = plane_desc[plane].nrefs;
  int32_t dc = plane_desc[plane].dc;
  int32_t wgt1 = plane_desc[plane].weight1;
  int32_t wgt0 = plane_desc[plane].weight0;
  int32_t wgt2 = plane_desc[plane].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[1].src_plane-plane;
      by_weights = true;
    }
  }
  if( !by_weights ) {
    if( plane_desc[plane].nrefs!=1 ) {
      if( mode==2 ) {
        int32_t left2 = bmf_pixels(img);
        uint8_t *p2 = base;
        do {
          int32_t x2 = dc+(uint8_t)*src++;
          *p2 = (uint8_t)(((wgt0*(uint8_t)p2[to_ref0]+wgt1*(uint32_t)(uint8_t)p2[to_ref1]+40)>>7)+x2);
          p2 += stride;
          --left2;
        } while( left2 );
      } else if( mode==3 ) {
        int32_t left3 = bmf_pixels(img);
        uint8_t *p3 = base;
        do {
          int32_t x3 = dc+(uint8_t)*src++;
          *p3 = (uint8_t)(((wgt1*(uint8_t)*(p3-2)+wgt0*(uint8_t)*(p3-3)+wgt2*(uint32_t)(uint8_t)*(p3-1)+63)>>7)+x3);
          p3 += stride;
          --left3;
        } while( left3 );
      }
      return;
    }
  }
  uint8_t *ref = base+to_ref0;
  int32_t i2 = 0;
  int32_t ofs2 = 0;
  do {
    base[ofs2] = ref[ofs2]+dc+src[i2];
    ofs2 += stride;
    ++i2;
  } while( i2<n );
}

// Subtract from one plane whatever the other planes predict of it, and write
// the residual to `dst`.  Four modes, chosen by `flags & 8` and `nrefs`:
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
  int32_t to_ref0;
  uint8_t *src = &img->pixels[plane];
  if( !(plane_desc[plane].flags&8) ) {
    int32_t n_flat = bmf_pixels(img);
    int32_t step = plane_count;
    if( plane_count==1 ) {
      memcpy(dst, src, n_flat);
      return;
    }
    int32_t i = 0;
    int32_t ofs = 0;
    do {
      dst[i] = src[ofs];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return;
  }
  int32_t stride = plane_count;
  int32_t n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[0].src_plane-plane;
  int32_t to_ref1 = plane_desc[1].src_plane-plane;
  int32_t mode = plane_desc[plane].nrefs;
  int32_t dc = plane_desc[plane].dc;
  int32_t wgt1 = plane_desc[plane].weight1;
  int32_t wgt0 = plane_desc[plane].weight0;
  int32_t wgt2 = plane_desc[plane].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[1].src_plane-plane;
      by_weights = true;
    }
  }
  if( by_weights||plane_desc[plane].nrefs==1 ) {
    uint8_t* ref = src+to_ref0;
    int32_t i2 = 0;
    int32_t ofs2 = 0;
    do {
      uint8_t d = src[ofs2]-dc-ref[ofs2];
      ofs2 += stride;
      dst[i2++] = d;
    } while( i2<n );
    return;
  }
  if( mode==2 ) {
    int32_t left2 = bmf_pixels(img);
    uint8_t *p2 = src;
    do {
      int32_t x2 = *p2-dc;
      uint32_t blend = wgt0*p2[to_ref0]+wgt1*p2[to_ref1]+40;
      p2 += stride;
      *dst++ = (uint8_t)(x2-(blend>>7));
      --left2;
    } while( left2 );
  } else if( mode==3 ) {
    int32_t left3 = bmf_pixels(img);
    uint8_t *p3 = src;
    do {
      int32_t x3 = *p3-dc;
      int32_t wsum = wgt1**(p3-2)+wgt0**(p3-3);
      int32_t w12sum = wgt2**(p3-1);
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
  uint8_t *p;
  alignas(16) uint8_t unfold[256];
  p = pixels+1;
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
    uint32_t pairs = (width-1)/2;
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
        uint8_t pred = med_predict((uint8_t)p[-1], (uint8_t)*up, (uint8_t)p[-width-1]);
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
  uint8_t *arc = (uint8_t*)stream.buf;
  BmfImage *hdr = (BmfImage*)(stream.buf+16);
  *hdr = *p_i;
  uint8_t* src_pixels = (uint8_t*)((uint16_t*)p_i->pixels);
  memcpy(arc+32, p_i->pixels, p_i->data_size);
  uint8_t *tmp = (uint8_t*)bmf_new(p_i->width*p_i->height);
  if( plane_count>0 ) {
    int32_t k = 0;
    do {
      ++k;
      int32_t plane = plane_desc[k-1].src_plane;
      int32_t predictor = plane_desc[plane].flags&3;
      plane_predictor = predictor;
      int32_t alt = (uint8_t)(plane_desc[plane].flags&4)>>2;
      plane_alt_model = alt;
      if( ((plane_desc[plane].flags&8)!=0||predictor)&&!alt ) {
        colour_transform(hdr, tmp, plane);
        if( plane_predictor!=2 ) {
          if( plane_predictor==1 ) {
            predict_med(tmp, p_i->width, p_i->height);
          }
        }
        int32_t n = p_i->width*p_i->height;
        int32_t stride = plane_count;
        if( plane_count==1 ) {
          memcpy(&p_i->pixels[plane], tmp, n);
        } else {
          uint8_t *dst = (uint8_t*)p_i+plane;
          int32_t ofs = 0;
          for( i = 0; i<n; ++i ) {
            dst[ofs+16] = tmp[i];
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
