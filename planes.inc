// planes.inc -- plane-level helpers shared by the codec: cost estimation, the
// MED predictor, the inter-plane blends, and the search's small result records.
// Included from bmf.cpp; not a standalone translation unit.

int32_t estimate_cost(const int32_t* bin, int32_t n) {
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  int32_t i;
  for( i = 0; i+1<n; i += 2 ) {
    if( bin[i] ) {
      sum_even += (double)bin[i];
      ent_even += (double)bin[i]*log((double)bin[i]);
    }
    if( bin[i+1] ) {
      sum_odd += (double)bin[i+1];
      ent_odd += (double)bin[i+1]*log((double)bin[i+1]);
    }
  }
  double entropy = ent_even+ent_odd;
  double total = sum_even+sum_odd;
  for(; i<n; i++ )
    if( bin[i] ) {
      total += (double)bin[i];
      entropy += (double)bin[i]*log((double)bin[i]);
    }
  if( total!=0.0 )
    total = total*log(total);
  return (int32_t)((total-entropy)*1.442695040888963);
}
uint32_t plane_mix2(const PlaneDesc &d, const uint8_t* p0, const uint8_t* p1) {
  return (d.weight0**p0+d.weight1*(uint32_t)p1[0]+40)>>7;
}

int32_t plane_mix3(const PlaneDesc &d, const uint8_t* p) {
  return (d.weight1*p[-2]+d.weight0*p[-3]+d.weight2*p[-1]+64)>>7;
}
uint8_t med_predict(int32_t west, int32_t north, int32_t northwest) {
  const int32_t lo = min32(west, north), hi = max32(west, north);
  if( northwest>=hi )
    return (uint8_t)lo;
  if( northwest<=lo )
    return (uint8_t)hi;
  return (uint8_t)(west+north-northwest);
}

void med_fold_table(uint8_t* fold) {
  for( int32_t j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*j);
  }
}

void med_unfold_table(uint8_t* unfold) {
  unfold[0] = 0;
  for( int32_t j = 0; j<127; ++j ) {
    unfold[2*j+1] = (uint8_t)(-1-j);
    unfold[2*j+2] = (uint8_t)(1+j);
  }
  unfold[255] = (uint8_t)(-1-127);
}
void strided_copy(uint8_t* dst, int32_t dstep, const uint8_t* src, int32_t sstep, int32_t n) {
  if( dstep==1&&sstep==1 ) {
    memcpy(dst, src, n);
    return;
  }
  for( int32_t i = 0; i<n; ++i, dst += dstep, src += sstep )
    *dst = src[0];
}
struct PlaneTransform {
  int32_t to_ref0, to_ref1, mode, dc, wgt0, wgt1, wgt2;
  bool by_weights;
  uint32_t blend(const uint8_t* p) const {
    if( mode==2 )
      return (wgt0*(uint32_t)p[to_ref0]+wgt1*(uint32_t)p[to_ref1]+40)>>7;
    return (wgt1*(uint32_t)p[-2]+wgt0*(uint32_t)p[-3]+wgt2*(uint32_t)p[-1]+63)>>7;
  }
};
uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done, k, x_left;
  alignas(16) uint8_t unfold[256];
  uint8_t* p = pixels+1;
  med_unfold_table(unfold);
  int32_t rows_left = height-1;
  if( width!=1 ) {
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
    for(; rows_left; --rows_left ) {
      p[0] = unfold[(uint8_t)p[0]]+p[-width];
      ++p;
    }
    return p;
  }
  uint8_t* up = &p[-width];
  for(; rows_left; --rows_left ) {
    ++up;
    p[0] = unfold[(uint8_t)p[0]]+p[-width];
    ++p;
    for( x_left = width-1; x_left; --x_left ) {
      const uint8_t pred = med_predict(p[-1], (uint8_t)up[0], p[-width-1]);
      p[0] = pred+unfold[(uint8_t)p[0]];
      ++up;
      ++p;
    }
  }
  return p;
}
struct HistRows {
  int32_t* r0, * r1, * r2, * r3, * r4, * r5;
};

HistRows hist_rows(int32_t* hists) {
  return {hists, hists+1*1024, hists+2*1024, hists+3*1024, hists+4*1024, hists+5*1024};
}

int32_t residual_bin(int32_t own, int32_t w0, int32_t a, int32_t w1, uint32_t b) {
  return ((uint16_t)own-(uint16_t)((w0*a+w1*b+40)>>7)-256)&0x1FF;
}
int32_t widest_window(const int32_t* hist, int32_t first, int32_t end, int32_t miss) {
  int32_t sum = 0;
  for( int32_t i = 0; i<256; ++i ) sum += hist[first+i];
  int32_t best = sum, at = miss;
  for( int32_t w = first+256; w<end; ++w ) {
    sum += hist[w]-hist[w-256];
    if( sum>=best ) {
      at = w;
      best = sum;
    }
  }
  return at;
}



struct AlphaWeights {
  int32_t a, b, c;
};

AlphaWeights fit_alpha_weights(const uint8_t* px, const uint8_t* pp, uint32_t uu) {
  double Sxx = 0.0, Sxy = 0.0, Sxz = 0.0;
  double Syy = 0.0, Syz = 0.0, Szz = 0.0;
  double Sxw = 0.0, Syw = 0.0, Szw = 0.0;
  for( uint32_t i4 = 0; i4<uu; ++i4 ) {
    const double dv0 = (double)(px[4*i4]+pp[4*i4+4]-(px[4*i4+4]+pp[4*i4]));
    const double dv1 = (double)(px[4*i4+1]+pp[4*i4+5]-(px[4*i4+5]+pp[4*i4+1]));
    const double dv2 = (double)(px[4*i4+2]+pp[4*i4+6]-(px[4*i4+6]+pp[4*i4+2]));
    const int32_t dv3 = px[4*i4+3]+pp[4*i4+7]-(px[4*i4+7]+pp[4*i4+3]);
    Sxx += dv0*dv0;
    Sxy += dv0*dv1;
    Sxz += dv0*dv2;
    Syy += dv1*dv1;
    Syz += dv1*dv2;
    Szz += dv2*dv2;
    Sxw += dv0*(double)dv3;
    Syw += dv1*(double)dv3;
    Szw += dv2*(double)dv3;
  }
  double d1 = Sxx*Szz-Sxz*Sxz;
  double d2 = 0.0-Sxx*Syz+Sxz*Sxy;
  double d3 = Sxz*Syz-Sxy*Szz;
  double inv = 128.0/(Syy*d1+Syz*d2+Sxy*d3+0.1);
  const int32_t wa = clamp_weight((int32_t)(((Sxz*Syz-Sxy*Szz)*Syw+(0.0-Sxz*Syy+Sxy*Syz)*Szw+(Szz*Syy-Syz*Syz)*Sxw)*inv));
  const int32_t wb = clamp_weight((int32_t)((d1*Syw+d2*Szw+d3*Sxw)*inv));
  const int32_t wc = clamp_weight((int32_t)(inv*((Sxx*Szw-Sxz*Sxw)*Syy+(0.0-Sxx*Syw+Sxw*Sxy)*Syz+(Sxz*Syw-Sxy*Szw)*Sxy)));
  return {wa, wb, wc};
}
enum : int32_t {
  try_mode0 = pred_mode0,
  try_p1 = pred_p1|desc_alt_model,
  try_p2 = pred_p2|desc_alt_model,
  try_refs = desc_has_refs,
  try_refs_p1 = desc_has_refs|pred_p1|desc_alt_model,
  try_refs_p2 = desc_has_refs|pred_p2|desc_alt_model,
};
struct PlaneSearch {
  int32_t bits;
  int32_t n_p1;
  int32_t n_p2;
  int32_t n_refs;
};
