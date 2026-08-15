// filter_cost.inc -- what one filter would cost on this image.

// Two walks over the image and seven histograms, which is all the frame ever
// held.  The first walk fits two weights by least squares and fills six
// histograms of pixel differences; the second re-walks with the fitted weights
// and fills the seventh.  `estimate_cost` turns each into bits, and the
// cheapest combination is the answer.
//
// **What the frame was hiding.**  Two of its members were the wrong size and
// the wrong type, and each was written through a `uint32_t *` because of it.
//
// `buf[4096]` is `hist_x[1024]`.  `*(uint32_t *)&buf[4*dx+2048]` is
// `hist_x[dx+512]`, and `dx` is a difference of two byte sums, so it is
// -510..510 and the index is 2..1022: it never leaves the member.  That is
// what `liftframe.py` declined this frame for -- its rule flags any computed
// index, because it cannot bound one, and the decline was recorded as though
// the index had been shown to escape.
//
// `buf_1[4]` is `hist_w[512]`, 2048 bytes.  `memset(buf_1, 0, 2048)` and
// `estimate_cost(buf_1, 512)` both say so, and what the four-byte declaration
// made of the other 2044 bytes was the eleven members after it: `img_end`,
// eight offset slots, and the head of a 2008-byte pad.  Measured at both
// widths rather than reasoned about -- the region is 24576..26623 either way,
// and at 32 bits it stops one byte short of the next live member, which is
// what that pad was sized for.
//
// Nothing read those eleven after the `memset`, which is why clearing them was
// invisible: `img_end` is a local here, the eight offsets were the
// decompiler's spill slots for neighbour offsets that are `const` locals in
// the walk, and `nplanes` is `plane_count`.  So the whole of the frame after
// the histograms was padding, and there is no frame.
int32_t cost_candidate(BmfImage* img, int32_t cand, uint8_t* desc, uint32_t* costs) {
  // Six histograms of a difference, indexed `value + 512`, and a seventh of
  // the second walk's residual, indexed by nine bits.  The first six were one
  // 24,576-byte `memset`, being contiguous members; six is the same clear.
  alignas(16) int32_t hist_x[1024];   // dx, the first reference plane
  alignas(16) int32_t hist_y[1024];   // dy, the second
  alignas(16) int32_t hist_yx[1024];  // dy - dx
  alignas(16) int32_t hist_zx[1024];  // dz - dx
  alignas(16) int32_t hist_zy[1024];  // dz - dy
  alignas(16) int32_t hist_zp[1024];  // dz - (dx + dy)/2
  alignas(16) int32_t hist_w[512];    // dz with the fitted weights taken out
  memset(hist_x, 0, sizeof hist_x);
  memset(hist_y, 0, sizeof hist_y);
  memset(hist_yx, 0, sizeof hist_yx);
  memset(hist_zx, 0, sizeof hist_zx);
  memset(hist_zy, 0, sizeof hist_zy);
  memset(hist_zp, 0, sizeof hist_zp);

  const int32_t planes = plane_count;
  // `*(const uint16_t*)&img->stride` -- the low half of a `uint32_t`, which
  // is a truncation on a little-endian machine and a different field on any
  // other.  Cast the value.
  //
  // `uint32_t` because that is the type the first walk's offsets were computed
  // in: the original stashed this in the four bytes of `buf_1` and read it
  // back through a `uint32_t *`.  The stash, the reload and the store back are
  // gone; the type they gave the value stays, because every `d - row_b` below
  // was unsigned.
  const uint32_t row_b = (uint16_t)img->stride;
  // The two other planes of the pixel, as offsets from this one.  `cand` is
  // 0, 1 or 2, so the pair is (1, 2), (1, -1) or (-2, -1).
  const int32_t d1 = (cand+1)%3-cand;
  int32_t d2 = (cand+2)%3-cand;
  uint8_t* img_end = &img->pixels[img->data_size];
  uint8_t* base = img->pixels;
  double syz = 0.0, syy = 0.0, sxz = 0.0, sxy = 0.0, sxx = 0.0;

  const int32_t rec_ofs = 16*cand;
  desc[rec_ofs] = 2;
  desc[33] = (uint8_t)cand;
  uint8_t* p = &base[cand+row_b+planes];
  if( p<img_end ) {
    // Eight neighbour offsets, constant for the whole walk.  The decompiler
    // kept them in frame slots and read them back through `frame.` in the hot
    // loop, which is what a register spill looks like once it has a struct
    // around it; as `const` locals they say what they are -- the north, west and
    // north-west of the two reference planes, and the step to the next pixel.
    const int32_t a_n = d1-row_b,           a_w = d1-planes,     a_nw = d1-row_b-planes;
    const int32_t b_n = d2-row_b,           b_w = d2-planes,     b_nw = d2-row_b-planes;
    const int32_t step = row_b+planes;
    do {
      int32_t dx = p[a_nw]+p[d1]-(p[a_n]+p[a_w]);
      ++hist_x[dx+512];
      int32_t dy = p[b_nw]+p[d2]-(p[b_n]+p[b_w]);
      sxx = sxx+(double)dx*(double)dx;
      ++hist_y[dy+512];
      syy = syy+(double)dy*(double)dy;
      sxy = sxy+(double)dx*(double)dy;
      ++hist_yx[((uint16_t)dy-(uint16_t)dx-512)&0x3FF];
      int32_t diag = p[-step]+*p;
      int32_t west = p[-planes];
      uint8_t *q = p-row_b;
      p += planes;
      int32_t dz = diag-(*q+west);
      sxz = sxz+(double)dx*(double)dz;
      ++hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
      syz = syz+(double)dy*(double)dz;
      ++hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
      int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;
      ++hist_zp[bin];
    } while( p<img_end );
  }
  // The least-squares solution for `dz ~ wa*dx + wb*dy`, in 1/128ths, clamped
  // to what the descriptor's byte can carry.  The 0.1 keeps the determinant
  // off zero for an image with no gradient at all.
  double inv = 128.0/(0.1-sxy*sxy+sxx*syy);
  double w1f = (syy*sxz-sxy*syz)*inv;
  double w2f = inv*(sxx*syz-sxy*sxz);
  int32_t wa = (int32_t)w1f;
  if( wa>=191 )
    wa = 191;
  if( wa<-64 )
    wa = -64;
  int32_t wb = (int32_t)w2f;
  if( wb>=191 )
    wb = 191;
  if( wb<-64 )
    wb = -64;

  memset(hist_w, 0, sizeof hist_w);
  int32_t img_w = (uint16_t)img->stride;
  // The same three neighbours as above, for the plane this walk reads, and
  // `plane_count` is the step to the next pixel.  The original held the west
  // offset as `nstep` and stepped by `-nstep`.
  const int32_t ofs_n = -img_w, ofs_w = -planes, ofs_nw = -img_w-planes;
  int32_t rows = ((img->height-1)*img->width)-1;
  uint8_t *r0 = &base[cand+img_w+planes], *r1 = r0+d1, *r2 = r0+d2;
  do {
    int32_t e0 = r0[ofs_nw];
    int32_t e1 = e0+*r0-r0[ofs_n]-r0[ofs_w];
    int32_t e2 = *r1;
    int32_t bin2 = ((uint16_t)e1-(uint16_t)((wa*(r1[ofs_nw]+e2-r1[ofs_n]-r1[ofs_w])+wb*(r2[ofs_nw]+*r2-r2[ofs_n]-(uint32_t)r2[ofs_w])+40)>>7)-256)&0x1FF;
    ++hist_w[bin2];
    r0 += planes;
    r1 += planes;
    r2 += planes;
  } while( --rows!=0 );

  int32_t pick = d1;
  int32_t rec = 4*cand;
  costs[rec] = estimate_cost(hist_w, 512);
  costs[rec+1] = estimate_cost(hist_zx, 1024);
  costs[rec+2] = estimate_cost(hist_zy, 1024);
  costs[rec+3] = estimate_cost(hist_zp, 1024);
  // Which of the two reference planes to predict from.  `s1` and `s2` are the
  // two orders' costs; taking the second swaps everything that names an order
  // -- the two planes, the two weights, and the two cost slots.
  int32_t cost = estimate_cost(hist_x, 1024);
  int32_t cost_y = estimate_cost(hist_y, 1024);
  int32_t cost_yx = estimate_cost(hist_yx, 1024);
  int32_t c2 = cost_yx;
  int32_t lo1 = c2;
  if( cost<c2 )
    lo1 = cost;
  if( cost_y<c2 )
    c2 = cost_y;
  int32_t s1 = cost_y+lo1;
  int32_t s2 = cost+c2;
  int32_t best = s2;
  if( s1<s2 ) {
    best = s1;
    int32_t tmp = pick;
    pick = d2;
    cost = cost_y;
    d2 = tmp;
    uint32_t swap = costs[rec+1];
    costs[rec+1] = costs[rec+2];
    costs[rec+2] = swap;
    int32_t w1s = wa;
    wa = wb;
    wb = w1s;
  }
  *(uint32_t*)&desc[rec_ofs+4] = wa;
  *(uint32_t*)&desc[rec_ofs+8] = wb;
  int32_t idx1 = cand+pick;
  int32_t idx2 = cand+d2;
  desc[16*idx1] = 0;
  desc[1] = (uint8_t)idx1;
  desc[16*idx2] = 1;
  desc[17] = (uint8_t)idx2;
  // A `bool deep = 0;` guarded this, and nothing assigned it -- so `!deep` was
  // a constant the compiler folded and a condition a reader had to check.
  if( img->data_size>0x1000000u )
    return cost+cost_yx+costs[rec];
  uint32_t lo2 = costs[rec];
  uint32_t lo3 = costs[rec+2];
  if( lo2>=costs[rec+1] )
    lo2 = costs[rec+1];
  if( lo3>=costs[rec+3] )
    lo3 = costs[rec+3];
  if( lo2<lo3 )
    lo3 = lo2;
  return best+lo3;
}
