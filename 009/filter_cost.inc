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
// `buf_1[4]` was `hist_w[512]`, 2048 bytes.  `memset(buf_1, 0, 2048)` and
// `estimate_cost(buf_1, 512)` both said so, and what the four-byte declaration
// made of the other 2044 bytes was the eleven members after it: `img_end`,
// eight offset slots, and the head of a 2008-byte pad.  Measured at both
// widths rather than reasoned about -- the region is 24576..26623 either way,
// and at 32 bits it stops one byte short of the next live member, which is
// what that pad was sized for.
//
// Nothing read those eleven after the `memset`, which is why clearing them was
// invisible: `img_end` is a local here, the eight offsets were the
// decompiler's spill slots for neighbour offsets that are `const` locals in
// the walk, and `nplanes` is `plane_count`.  The histogram itself has moved
// again since: it belongs to `weight_pair_cost` now, which is the walk that
// fills it and the four copies of that walk in `choose_plane_coding`.  So the whole of the frame after
// the histograms was padding, and there is no frame.
//
// **The output was a byte pointer too.**  `pd` was `uint8_t *desc`, written as
// `desc[16*k]`, `desc[33]` and `*(uint32_t *)&desc[16*cand+4]`.  It is four
// `PlaneDesc` -- one candidate's row of `choose_plane_coding`'s 3x16 table,
// which that function copies into `plane_desc` with
// `memcpy(…, 4*sizeof(PlaneDesc))` once a candidate wins.  Byte 16*k+0 is
// `nrefs`, +1 is `src_plane`, +4 and +8 are the two weights, and the whole of
// this function's output is those four fields.  No cast is left in this file.
// One residual quantised into the 512-bin histogram the filter search scores.
// `own` is this plane's gradient at the sample; `a` and `b` are the two
// reference planes' gradients at the same sample, blended by the candidate's
// two weights, rounded, and taken as a nine-bit offset from 256.
//
// Five copies of a hundred and fifty characters -- this one and four in
// `choose_plane_coding` -- and two of the differences between them are not
// what they look like.
//
//   * The second gradient carries a `(uint32_t)` on its last term at every one
//     of the five sites, and it is load-bearing: it makes the blend unsigned,
//     so the `>> 7` is a logical shift rather than an arithmetic one and a
//     negative blend rounds the other way.  `b` is `uint32_t` here for that
//     reason, and the call sites keep the cast that puts it there.
//   * Two of the five cast `own` to `int16_t` and three to `uint16_t`.  That
//     one cannot show, for all inputs and not merely on this corpus: the two
//     readings differ by exactly 65536, which is 128 times 512, so they agree
//     in the nine bits `& 0x1FF` keeps.
// The rows of a histogram block, cut out by index.  Three functions here and in
// plane_choose.inc keep their histograms in one `int32_t[n*1024]` -- because
// each of them also picks a row *by index*, which is what the original's byte
// offsets said -- and each then cut the same cursors out of it a row at a time.
// Six lines, four lines and four lines, and `hists + 3*1024` written out in all
// three.  The stride lives here now and the call sites name their rows.
//
// Six rather than four because `cost_candidate` uses six; the two callers that
// want four ignore the tail, which costs nothing and keeps this one function.
struct HistRows {
  int32_t *r0, *r1, *r2, *r3, *r4, *r5;
};

static HistRows hist_rows(int32_t* hists) {
  return { hists, hists+1*1024, hists+2*1024, hists+3*1024,
           hists+4*1024, hists+5*1024 };
}

static inline int32_t residual_bin(int32_t own, int32_t w0, int32_t a,
                                   int32_t w1, uint32_t b) {
  return ((uint16_t)own-(uint16_t)((w0*a+w1*b+40)>>7)-256)&0x1FF;
}

// Cost one plane under a candidate pair of transform weights: walk every sample
// after the first row, quantise its residual into a 512-bin histogram, and
// score the histogram.
//
// Five copies: `cost_candidate`'s second walk, below, and four in
// `choose_plane_coding` of twenty-six lines each, differing in nothing but the
// two weights and which of four `[512]` locals they cleared.  Those four
// locals are gone with them; the histogram belongs to the walk.
//
// `plane_a` and `plane_b` are byte offsets from this plane to the two it
// references, which is why they are added to the same base rather than passed
// as pointers.
static int32_t weight_pair_cost(const BmfImage* img, const uint8_t* plane0,
                                int32_t plane_a, int32_t plane_b,
                                int32_t w4, int32_t w8) {
  int32_t hist[512];
  memset(hist, 0, sizeof hist);
  const int32_t stride = img->stride;
  const int32_t left = -bmf_cx.plane_count;
  const int32_t ofs_up = -stride;
  const int32_t ofs_ul = -stride-bmf_cx.plane_count;
  const uint8_t *p = plane0-ofs_ul;
  const uint8_t *q = plane0+plane_a-ofs_ul;
  const uint8_t *r = plane0+plane_b-ofs_ul;
  // One pixel short of the rows below the first, and never zero: the caller
  // only reaches here for an image with at least two rows and two columns.
  for( int32_t n = img->width*(img->height-1)-1; n; --n ) {
    const int32_t cur = *q;
    ++hist[residual_bin(p[ofs_ul]+*p-p[ofs_up]-p[left],
                        w4, q[ofs_ul]+cur-q[ofs_up]-q[left],
                        w8, r[ofs_ul]+*r-r[ofs_up]-(uint32_t)r[left])];
    q -= left;
    p -= left;
    r -= left;
  }
  return estimate_cost(hist, 512);
}

int32_t cost_candidate(BmfImage* img, int32_t cand, PlaneDesc* pd, uint32_t* costs) {
  // Six histograms of a difference, indexed `value + 512`, in one block --
  // which is what the original had: a single 24,576-byte `memset` over six
  // contiguous members.  A round that named the six turned that clear into
  // six, and six declarations and six `memset`s were the two longest runs
  // `dupblock.py` had left in this file.  The names are cursors into the
  // block, the shape `choose_plane_coding` already uses for its eight.
  alignas(16) int32_t hists[6*1024];
  const HistRows hr = hist_rows(hists);
  int32_t *const hist_x = hr.r0;      // dx, the first reference plane
  int32_t *const hist_y = hr.r1;      // dy, the second
  int32_t *const hist_yx = hr.r2;     // dy - dx
  int32_t *const hist_zx = hr.r3;     // dz - dx
  int32_t *const hist_zy = hr.r4;     // dz - dy
  int32_t *const hist_zp = hr.r5;     // dz - (dx + dy)/2
  memset(hists, 0, sizeof hists);

  const int32_t planes = bmf_cx.plane_count;
  // `*(const uint16_t*)&img->stride` -- the low half of a `uint32_t`, which
  // is a truncation on a little-endian machine and a different field on any
  // other.  Cast the value.
  //
  // `uint32_t` because that is the type the first walk's offsets were computed
  // in: the original stashed this in the four bytes of `buf_1` and read it
  // back through a `uint32_t *`.  The stash, the reload and the store back are
  // gone; the type they gave the value stays, because every `d - row_b` below
  // was unsigned.
  const uint32_t row_b = img->stride;
  // The two other planes of the pixel, as offsets from this one.  `cand` is
  // 0, 1 or 2, so the pair is (1, 2), (1, -1) or (-2, -1).
  // The two other planes, as offsets from this one.  Not `const`: the swap
  // below exchanges them when the second turns out to be the better first
  // reference, and both names mean the same thing throughout -- "the offset of
  // the reference in slot N".  A third local held the first of them across that
  // swap and `tools/uncopy.py` reported it for what it was, a copy: `d1` is not
  // read again after the swap, so the two were never live at once.
  int32_t d1 = (cand+1)%3-cand;
  int32_t d2 = (cand+2)%3-cand;
  uint8_t* img_end = &img->pixels[img->data_size];
  uint8_t* base = img->pixels;
  double syz = 0.0, syy = 0.0, sxz = 0.0, sxy = 0.0, sxx = 0.0;

  pd[cand].nrefs = 2;
  pd[2].src_plane = (uint8_t)cand;
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
  // Not `const`: the swap forty lines down exchanges them when the two
  // reference planes come out in the other order.
  int32_t wa = clamp_weight((int32_t)w1f);
  int32_t wb = clamp_weight((int32_t)w2f);

  int32_t rec = 4*cand;
  costs[rec] = weight_pair_cost(img, &base[cand], d1, d2, wa, wb);
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
    // The two reference planes come out the other way round, so everything
    // that names one of them exchanges: the plane, its cost, and its weight.
    best = s1;
    cost = cost_y;
    swap32(d1, d2);
    swap32(costs[rec+1], costs[rec+2]);
    swap32(wa, wb);
  }
  pd[cand].weight0 = wa;
  pd[cand].weight1 = wb;
  // The plane order, as the permutation and its inverse -- `src_plane[rank]`
  // and `nrefs[plane]` -- which is the pair `expand_image` builds from the
  // wire.  `d1` and `d2` are offsets from this plane, so both indices are
  // 0, 1 or 2 whichever candidate this is.
  int32_t idx1 = cand+d1;
  int32_t idx2 = cand+d2;
  pd[idx1].nrefs = 0;
  pd[0].src_plane = (uint8_t)idx1;
  pd[idx2].nrefs = 1;
  pd[1].src_plane = (uint8_t)idx2;
  // A `bool deep = 0;` guarded this, and nothing assigned it -- so `!deep` was
  // a constant the compiler folded and a condition a reader had to check.
  if( img->data_size>0x1000000u )
    return cost+cost_yx+costs[rec];
  uint32_t lo2 = costs[rec];
  uint32_t lo3 = costs[rec+2];
  lo2 = umin32(lo2, costs[rec+1]);
  lo3 = umin32(lo3, costs[rec+3]);
  lo3 = umin32(lo3, lo2);
  return best+lo3;
}
