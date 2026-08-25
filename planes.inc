// planes.inc -- plane-level helpers shared by the codec: cost estimation, the
// MED predictor, the inter-plane blends, and the search's small result records.
// Included from bmf.cpp; not a standalone translation unit.

// Cost of coding a residual histogram, in bits, as an *adaptive* order-0 code
// rather than as an entropy: the Krichevsky-Trofimov sequential estimator over
// the bins the residual actually occupies.  KT's redundancy over the empirical
// entropy is the learning cost -- about (K-1)/2 * log2(N) bits for K occupied
// bins -- which is the term a plain entropy sum leaves out, and leaving it out
// is what makes a wider or more finely conditioned residual look free.  It is
// still not the code the coder will run, but it is the same *kind* of code, and
// what this is used for is ranking.
int32_t estimate_cost(const int32_t* bin, int32_t n) {
  double total = 0.0, terms = 0.0;
  int32_t used = 0;
  for( int32_t i = 0; i<n; ++i )
    if( bin[i] ) {
      total += (double)bin[i];
      terms += lgamma((double)bin[i]+0.5);
      ++used;
    }
  if( used<2 )
    return 0;
  const double half_k = 0.5*(double)used;
  // lgamma(0.5) is log(sqrt(pi)), once per occupied bin.
  const double bits = lgamma(total+half_k)-lgamma(half_k)-(terms-(double)used*0.5723649429247001);
  return (int32_t)(bits*1.442695040888963);
}
uint32_t plane_mix2(const PlaneDesc &d, const uint8_t* p0, const uint8_t* p1) {
  return (d.weight0**p0+d.weight1*(uint32_t)p1[0]+CD_weight_round)>>kWeightShift;
}

int32_t plane_mix3(const PlaneDesc &d, const uint8_t* p) {
  return (d.weight1*p[-2]+d.weight0*p[-3]+d.weight2*p[-1]+CD_weight_round3)>>kWeightShift;
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
  int32_t to_ref0, to_ref1, to_ref2, mode, dc, wgt0, wgt1, wgt2;
  bool by_weights;
  // The three-reference form used to read the three bytes *before* this one,
  // which is the same thing only while the plane taking three references is
  // physical plane 3 with the colour planes ahead of it in slot order.  It now
  // names the planes in coding slots 0, 1 and 2, and the weights are stored in
  // slot order to match.
  uint32_t blend(const uint8_t* p) const {
    if( mode==2 )
      return (wgt0*(uint32_t)p[to_ref0]+wgt1*(uint32_t)p[to_ref1]+40)>>7;
    return (wgt0*(uint32_t)p[to_ref0]+wgt1*(uint32_t)p[to_ref1]+wgt2*(uint32_t)p[to_ref2]+63)>>7;
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

// Gradient-domain second differences of two 8-bit planes span about +-1020, so
// the residual needs the same 1024 bins cost_candidate gives the copy and
// average options.  Folding the fitted blend into 512 instead merged bins for
// exactly the fits that wrapped -- the poor ones -- and merging bins can only
// lower an entropy, so the narrower range flattered the fits that deserved it
// least.
int32_t residual_bin(int32_t own, int32_t w0, int32_t a, int32_t w1, uint32_t b) {
  return ((uint16_t)own-(uint16_t)((w0*a+w1*b+40)>>7)-512)&0x3FF;
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
  double inv = (double)kWeightScale/(Syy*d1+Syz*d2+Sxy*d3
                                    +(1.0/(double)idx_clamp(CD_fit_ridge_den, 1, 1<<20)));
  const int32_t wa = clamp_weight((int32_t)(((Sxz*Syz-Sxy*Szz)*Syw+(0.0-Sxz*Syy+Sxy*Syz)*Szw+(Szz*Syy-Syz*Syz)*Sxw)*inv));
  const int32_t wb = clamp_weight((int32_t)((d1*Syw+d2*Szw+d3*Sxw)*inv));
  const int32_t wc = clamp_weight((int32_t)(inv*((Sxx*Szw-Sxz*Sxw)*Syy+(0.0-Sxx*Syw+Sxw*Sxy)*Syz+(Sxz*Syw-Sxy*Szw)*Sxy)));
  return {wa, wb, wc};
}
enum : int32_t {
  try_mode0 = pred_mode0,
  try_p1 = pred_p1|desc_alt_model,
  try_p2 = pred_p2|desc_alt_model,
  try_ctx = pred_ctx|desc_alt_model,
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

// Value-set renumbering.  Nine LPCB frames have been through a tone curve or a
// quantiser and use 74-248 of the 256 levels, spaced with gaps of 2 and 3 --
// every residual is then a multiple of the gap, and alt-P1 and alt-P2 spend a
// coding decision per sample on a low bit that never varies.  The slow model
// already renumbers (reduce_alphabet), but it is a much weaker coder, so the
// search can have renumbering or good modelling and never both.  Doing it here,
// before the plane search, gives the strong model a dense alphabet.
//
// Per plane: a 256-bit used-value mask, and the map is "rank among used values"
// -- monotone, so the neighbour differences the predictors work on keep their
// order.  The mask is the whole side information, 32 bytes a plane, and it goes
// out with the plane descriptors.
//
// Whether it pays is not decidable from the histogram: PIA13915 uses 248 levels
// of 256 and renumbering costs it 2.29%, while PIA13812 uses 120 and gains
// 21.9%.  So this is a trial like the transpose, not a
// heuristic -- see search_filter.
// Per-plane value histograms.  Both masks are cut from these, and the split
// threshold needs the counts, not just the occupancy.
void remap_hist(const BmfImage* img, int32_t planes, int32_t* hist) {
  memset(hist, 0, 256u*sizeof(int32_t)*(uint32_t)planes);
  const uint8_t* p = img[0].pixels;
  const uint8_t*const end = p+img[0].data_size;
  for(; p+planes<=end; p += planes )
    for( int32_t k = 0; k<planes; ++k )
      ++hist[256*k+p[k]];
}

// The values a plane's histogram puts at or above `floor_`.  At floor_ == 1
// that is the used-value mask; above it, the dense set of the split map.
void remap_mask_from_hist(const int32_t* hist, int32_t floor_, uint8_t* mask) {
  memset(mask, 0, 32);
  for( int32_t v = 0; v<256; ++v )
    if( hist[v]>=floor_ )
      mask[v>>3] |= (uint8_t)(1u<<(v&7));
}

inline int32_t remap_bit(const uint8_t* mask, int32_t v) {
  return (mask[v>>3]>>(v&7))&1;
}

// How many values each plane actually uses.  A plane at 256 has nothing to gain
// and is left alone, which is what makes the gate free on the 97 files of the
// corpus that have no comb at all.
int32_t remap_used(const uint8_t* mask) {
  int32_t n = 0;
  for( int32_t i = 0; i<32; ++i )
    for( int32_t b = 0; b<8; ++b )
      n += (mask[i]>>b)&1;
  return n;
}

// Holes strictly inside the used range -- which is the only thing renumbering
// can remove.  A plane using a contiguous 230 of 256 levels has none: mapping
// it to 0..229 just subtracts the minimum, and the predictors work on
// differences, so that is a no-op they already absorb.  Every olympus_xz1 frame
// is exactly that shape, so counting distinct values alone would put all 27 of
// them through a trial that cannot win.
int32_t remap_holes(const uint8_t* mask) {
  int32_t lo = -1, hi = -1, n = 0;
  for( int32_t v = 0; v<256; ++v )
    if( (mask[v>>3]>>(v&7))&1 ) {
      if( lo<0 ) lo = v;
      hi = v;
      ++n;
    }
  return n ? (hi-lo+1)-n : 0;
}

// value -> rank, and rank -> value, built from the masks alone so both sides
// derive the identical map from the identical bytes.
//
// `dense` is a subset of `used` and goes first, in value order, so the values
// that carry the plane's weight occupy a contiguous run 0..K-1; the rest of the
// used set follows at K..U-1.  Passing dense == used gives the plain rank map.
//
// The subset is what makes renumbering survive an image with a caption strip.
// PIA13882 is 1533x1484 of step-2 lattice -- 100, 95 and 85 values a plane --
// with two rows of off-lattice colour bar at the bottom.  Those two rows are
// 0.135% of the pixels and they carry 82, 91 and 92 values of their own.  A
// plain rank map interleaves them into the lattice, so a step of 2 in the
// original becomes a step of 1, 2 or 3 depending on where it lands: the map
// de-regularises 99.87% of the image to seat 0.13% of it.  The file goes from
// 8.38 bpp to 9.55.  Keeping the strip's values out of the dense run costs
// those two rows their prediction and gets the rest of the image its lattice
// back, which is worth 15.8% of that file.
void remap_build(const uint8_t* used, const uint8_t* dense, uint8_t* fwd, uint8_t* inv) {
  memset(fwd, 0, 256);
  int32_t n = 0;
  for( int32_t v = 0; v<256; ++v )
    if( remap_bit(used, v)&&remap_bit(dense, v) ) {
      fwd[v] = (uint8_t)n;
      inv[n] = (uint8_t)v;
      ++n;
    }
  for( int32_t v = 0; v<256; ++v )
    if( remap_bit(used, v)&&!remap_bit(dense, v) ) {
      fwd[v] = (uint8_t)n;
      inv[n] = (uint8_t)v;
      ++n;
    }
  // Ranks the image never reaches still need a defined image, or a decoder fed
  // a corrupt stream walks off inv[].
  if( !n ) {
    memset(inv, 0, 256);
    return;
  }
  for( int32_t k = n; k<256; ++k )
    inv[k] = inv[n-1];
}

// KT cost of one plane's MED residual under a candidate value map, in bits.
// This is what the split threshold is chosen with: it is not the coder's cost,
// but a value map that moves the file moves this by whole bits, and running it
// costs one pass over the plane instead of a full model encode.
int32_t remap_probe(const BmfImage* img, int32_t planes, int32_t k, const uint8_t* map) {
  int32_t bin[256];
  memset(bin, 0, sizeof bin);
  const int32_t w = img[0].width, h = img[0].height, stride = img[0].stride;
  const uint8_t* pix = img[0].pixels;
  for( int32_t y = 0; y<h; ++y ) {
    const uint8_t*const row = pix+(size_t)y*stride;
    const uint8_t*const up = y ? row-stride : nullptr;
    for( int32_t x = 0; x<w; ++x ) {
      const int32_t at = x*planes+k;
      const int32_t west = x ? map[row[at-planes]] : (up ? map[up[at]] : 0);
      const int32_t north = up ? map[up[at]] : west;
      const int32_t nw = (up&&x) ? map[up[at-planes]] : north;
      ++bin[(uint8_t)(map[row[at]]-med_predict(west, north, nw))];
    }
  }
  return estimate_cost(bin, 256);
}

void remap_apply(BmfImage* img, int32_t planes, const uint8_t* used,
                 const uint8_t* dense, int32_t inverse) {
  uint8_t map[4][256];
  for( int32_t k = 0; k<planes; ++k ) {
    uint8_t fwd[256], inv[256];
    remap_build(used+32*k, (dense ? dense : used)+32*k, fwd, inv);
    memcpy(map[k], inverse ? inv : fwd, 256);
  }
  uint8_t* p = img[0].pixels;
  const uint8_t*const end = p+img[0].data_size;
  for(; p+planes<=end; p += planes )
    for( int32_t k = 0; k<planes; ++k )
      p[k] = map[k][p[k]];
}


