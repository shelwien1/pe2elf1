// predict.inc -- the predictors and the colour transform -- what is subtracted from
// an image before it is modelled, and added back after.

void __expand_predictor_mode0(uint8_t* unread_src, int32_t i, int32_t unread_h) {
}

uint32_t __predict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done;
  int32_t rows_left, x_left, north, northwest, code, pairs, ofs;
  uint8_t pred, * q;
  uint32_t j, last, k;
  uint8_t* p, * up;
  alignas(16) uint8_t fold[272];
  p = pixels+height*width;
  up = &p[-width];
  for( j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*(int32_t)j);
  }
  rows_left = height-1;
  if( height!=1 ) {
    do {
      x_left = width-1;
      if( width!=1 ) {
        do {
          north = (uint8_t)*--up;
          pred = (uint8_t)*(--p-1);
          northwest = (uint8_t)p[-width-1];
          if( pred<north ) {
            if( northwest<pred )
              pred = (uint8_t)north;
            else if( northwest<=north )
              pred = (uint8_t)(north+pred-northwest);
          } else {
            if( northwest>pred )
              pred = (uint8_t)north;
            else if( northwest>=north )
              pred = (uint8_t)(north+pred-northwest);
          }
          code = (uint8_t)fold[(uint8_t)(*p-pred)];
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
    pairs = (width-1)/2;
    if( pairs ) {
      k = 0;
      ofs = 0;
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
      q = p-done;
      last = (uint8_t)(*q-*((int8_t*)q-1));
      *q = fold[(uint8_t)last];
    }
  }
  return last;
}

uint8_t*__interleave_plane(uint8_t* img, uint8_t* src, int32_t plane, int8_t unread_flag) {
  uint8_t* ref;
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3, x3, left2, x2, mode, to_ref0, wgt2, to_ref1, wgt0, stride, dc, n;
  uint8_t* base, * p3, * p2;
  if( !(plane_desc[plane+1].flags&8) ) {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if( plane_count==1 )
      return (uint8_t*)memcpy(&((BmfImage*)img)->pixels[plane], src, n_flat);
    img += plane;
    i = 0;
    ofs = 0;
    do {
      img[ofs+16] = src[i];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane-plane;
  to_ref1 = plane_desc[2].src_plane-plane;
  mode = plane_desc[plane+1].nrefs;
  dc = plane_desc[plane+1].dc;
  base = &((BmfImage*)img)->pixels[plane];
  wgt1 = plane_desc[plane+1].weight1;
  wgt0 = plane_desc[plane+1].weight0;
  wgt2 = plane_desc[plane+1].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[2].src_plane-plane;
      by_weights = true;
    }
  }
  if( !by_weights ) {
    if( plane_desc[plane+1].nrefs!=1 ) {
      if( mode==2 ) {
        left2 = bmf_pixels(img);
        p2 = base;
        do {
          x2 = dc+(uint8_t)*src++;
          *p2 = (uint8_t)(((wgt0*(uint8_t)p2[to_ref0]+wgt1*(uint32_t)(uint8_t)p2[to_ref1]+40)>>7)+x2);
          p2 += stride;
          --left2;
        } while( left2 );
      } else if( mode==3 ) {
        left3 = bmf_pixels(img);
        p3 = base;
        do {
          x3 = dc+(uint8_t)*src++;
          *p3 = (uint8_t)(((wgt1*(uint8_t)*(p3-2)+wgt0*(uint8_t)*(p3-3)+wgt2*(uint32_t)(uint8_t)*(p3-1)+63)>>7)+x3);
          p3 += stride;
          --left3;
        } while( left3 );
      }
      return img;
    }
  }
  img += plane;
  ref = &img[to_ref0];
  i2 = 0;
  ofs2 = 0;
  do {
    img[ofs2+16] = ref[ofs2+16]+dc+src[i2];
    ofs2 += stride;
    ++i2;
  } while( i2<n );
  return img;
}

uint8_t*__colour_transform(uint8_t* img, uint8_t* dst, int32_t plane, int8_t unread_flag) {
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3, x3, wsum, w12sum, left2, x2, mode, to_ref0, wgt2, to_ref1, wgt0, dc, stride, n;
  uint8_t* src, * p3, * p2;
  if( !(plane_desc[plane+1].flags&8) ) {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if( plane_count==1 )
      return (uint8_t*)memcpy(dst, &((const BmfImage*)img)->pixels[plane], n_flat);
    img += plane;
    i = 0;
    ofs = 0;
    do {
      dst[i] = img[ofs+16];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane-plane;
  to_ref1 = plane_desc[2].src_plane-plane;
  mode = plane_desc[plane+1].nrefs;
  dc = plane_desc[plane+1].dc;
  src = &((BmfImage*)img)->pixels[plane];
  wgt1 = plane_desc[plane+1].weight1;
  wgt0 = plane_desc[plane+1].weight0;
  wgt2 = plane_desc[plane+1].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[2].src_plane-plane;
      by_weights = true;
    }
  }
  if( by_weights||plane_desc[plane+1].nrefs==1 ) {
    img += plane;
    uint8_t* ref = &img[to_ref0];
    i2 = 0;
    ofs2 = 0;
    do {
      uint8_t d = img[ofs2+16]-dc-ref[ofs2+16];
      ofs2 += stride;
      dst[i2++] = d;
    } while( i2<n );
    return img;
  }
  if( mode==2 ) {
    left2 = bmf_pixels(img);
    p2 = src;
    do {
      x2 = *p2-dc;
      uint32_t blend = wgt0*p2[to_ref0]+wgt1*p2[to_ref1]+40;
      p2 += stride;
      *dst++ = (uint8_t)(x2-(blend>>7));
      --left2;
    } while( left2 );
  } else if( mode==3 ) {
    left3 = bmf_pixels(img);
    p3 = src;
    do {
      x3 = *p3-dc;
      wsum = wgt1**(p3-2)+wgt0**(p3-3);
      w12sum = wgt2**(p3-1);
      p3 += stride;
      *dst++ = (uint8_t)(x3-((uint32_t)(wsum+w12sum+63)>>7));
      --left3;
    } while( left3 );
  }
  return img;
}

uint8_t*__unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done;
  uint8_t cur, pred;
  int32_t rows_left, north, northwest;
  uint32_t j, row_rest, pairs, k, x_left;
  uint8_t* p, * up;
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
    pairs = (width-1)/2;
    if( pairs ) {
      for( k = 0; k<pairs; ++k ) {
        cur = pixels[2*k]+unfold[pixels[2*k+1]];
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
    up = &p[-width];
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
        pred = (uint8_t)*(p-1);
        north = (uint8_t)*up;
        northwest = (uint8_t)p[-width-1];
        if( pred<north ) {
          if( northwest<pred )
            pred = (uint8_t)north;
          else if( northwest<=north )
            pred = (uint8_t)(north+pred-northwest);
        } else {
          if( northwest>pred )
            pred = (uint8_t)north;
          else if( northwest>=north )
            pred = (uint8_t)(north+pred-northwest);
        }
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

void __transform_planes(BmfImage* p_i, int8_t unread_flag) {
  uint8_t* arc, * hdr, * tmp, * dst;
  int32_t k, plane, predictor, alt, n, stride, ofs, i;
  memset(hist_scratch, 0, 4096);
  arc = (uint8_t*)coded_buf;
  hdr = (uint8_t*)coded_buf+16;
  *(BmfImage*)hdr = *p_i;
  uint8_t* src_pixels = (uint8_t*)((uint16_t*)p_i->pixels);
  memcpy(arc+32, p_i->pixels, p_i->data_size);
  tmp = (uint8_t*)bmf_new(p_i->width*p_i->height);
  if( plane_count>0 ) {
    k = 0;
    do {
      ++k;
      plane = plane_desc[k].src_plane;
      predictor = plane_desc[plane+1].flags&3;
      plane_predictor = predictor;
      alt = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
      plane_alt_model = alt;
      if( ((plane_desc[plane+1].flags&8)!=0||predictor)&&!alt ) {
        __colour_transform(hdr, tmp, plane, 0);
        if( plane_predictor!=2 ) {
          if( plane_predictor==1 ) {
            __predict_med(tmp, p_i->width, p_i->height);
          }
        }
        n = p_i->width*p_i->height;
        stride = plane_count;
        if( plane_count==1 ) {
          memcpy(&p_i->pixels[plane], tmp, n);
        } else {
          dst = (uint8_t*)p_i+plane;
          ofs = 0;
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
    __model_plane(p_i, src_pixels, src_pixels);
  }
}
