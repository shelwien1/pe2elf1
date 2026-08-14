// filter_cost.inc -- what one filter would cost on this image.

int32_t __cost_candidate(uint8_t* img, int32_t cand, uint8_t* desc, int8_t unread4, int32_t unread5, int32_t unread6, int32_t unread7, uint32_t* costs) {
  struct alignas(16) CostCandidateFrame {
    uint8_t buf[4096];
    int32_t hist_y[1024];
    int32_t hist_yx[1024];
    int32_t hist_zx[1024];
    int32_t hist_zy[1024];
    int32_t hist_zp[1024];
    uint8_t buf_1[4];
    uint8_t* img_end;
    int32_t step_d;
    int32_t off_b0;
    int32_t off_b1;
    int32_t off_a0;
    int32_t off_a1;
    int32_t nplanes;
    int32_t off_a2;
    int32_t off_b2;
    uint8_t _pad0[2008];
    int32_t cand_i;
    uint8_t* desc_f;
    BmfImage* img_f;
    int32_t rec_off;
    int32_t _pad91;
    int32_t d1_f;
    int32_t best;
    int32_t rows;
    int32_t d2_f;
    int32_t wb;
    int32_t wa;
    uint8_t* r1_f;
    uint8_t* r0_f;
    uint8_t* cursor;
    int32_t off_up;
    uint8_t _pad1[28];
  } __frame;
  int32_t cost;
  double syz, syy, sxz, sxy, sxx;
  int32_t w1, w2, left, pick, c2, lo1;
  uint32_t lo2, lo3;
  uint8_t *r0, *r2;
  __frame.desc_f = desc;
  __frame.nplanes = plane_count;
  int32_t row_b = *(const uint16_t*)&((const BmfImage*)img)->stride;
  __frame.cand_i = cand;
  __frame.img_f = (BmfImage*)img;
  __frame.d1_f = (cand+1)%3-cand;
  __frame.d2_f = (cand+2)%3-cand;
  __frame.img_end = &__frame.img_f->pixels[__frame.img_f->data_size];
  int32_t d1 = __frame.d1_f;
  *(uint32_t*)__frame.buf_1 = row_b;
  memset(__frame.buf, 0, 24576);
  uint32_t row_b2 = *(uint32_t*)__frame.buf_1;
  int32_t cand2 = __frame.cand_i;
  uint8_t *base = __frame.img_f->pixels;
  syz = 0;
  syy = 0.0;
  sxz = 0.0;
  sxy = 0.0;
  sxx = 0.0;
  __frame.rec_off = 16*__frame.cand_i;
  __frame.desc_f[16*__frame.cand_i] = 2;
  __frame.desc_f[33] = (uint8_t)cand2;
  uint8_t *p = &base[cand2+row_b2+__frame.nplanes];
  if( p<__frame.img_end ) {
    // Eight neighbour offsets, constant for the whole walk.  The decompiler
    // kept them in frame slots and read them back through `__frame.` in the hot
    // loop, which is what a register spill looks like once it has a struct
    // around it; as `const` locals they say what they are -- the north, west and
    // north-west of the two reference planes, and the step to the next pixel.
    const int32_t planes = __frame.nplanes;
    const int32_t d2 = __frame.d2_f;
    const int32_t a_n = d1-row_b2,          a_w = d1-planes,     a_nw = d1-row_b2-planes;
    const int32_t b_n = d2-row_b2,          b_w = d2-planes,     b_nw = d2-row_b2-planes;
    const int32_t step = row_b2+planes;
    __frame.d1_f = d1;
    *(uint32_t*)__frame.buf_1 = row_b2;
    do {
      int32_t dx = p[a_nw]+p[d1]-(p[a_n]+p[a_w]);
      ++*(uint32_t*)&__frame.buf[4*dx+2048];
      int32_t dy = p[b_nw]+p[d2]-(p[b_n]+p[b_w]);
      sxx = sxx+(double)dx*(double)dx;
      ++__frame.hist_y[dy+512];
      syy = syy+(double)dy*(double)dy;
      sxy = sxy+(double)dx*(double)dy;
      ++__frame.hist_yx[((uint16_t)dy-(uint16_t)dx-512)&0x3FF];
      int32_t diag = p[-step]+*p;
      int32_t west = p[-planes];
      uint8_t *q = p-row_b2;
      p += planes;
      int32_t dz = diag-(*q+west);
      sxz = sxz+(double)dx*(double)dz;
      ++__frame.hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
      syz = syz+(double)dy*(double)dz;
      ++__frame.hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
      int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;
      ++__frame.hist_zp[bin];
    } while( p<__frame.img_end );
  }
  double inv = 128.0/(0.1-sxy*sxy+sxx*syy);
  double w1f = (syy*sxz-sxy*syz)*inv;
  double w2f = inv*(sxx*syz-sxy*sxz);
  w1 = (int32_t)w1f;
  if( (int32_t)w1f>=191 )
    w1 = 191;
  if( w1<-64 )
    w1 = -64;
  __frame.wa = w1;
  w2 = (int32_t)w2f;
  if( (int32_t)w2f>=191 )
    w2 = 191;
  if( w2<-64 )
    w2 = -64;
  __frame.wb = w2;
  memset(__frame.buf_1, 0, 2048);
  int32_t img_w = (uint16_t)__frame.img_f->stride;
  __frame.d1_f = d1;
  __frame.rows = ((__frame.img_f->height-1)*__frame.img_f->width)-1;
  __frame.best = -img_w;
  int32_t nstep = -plane_count;
  __frame.off_up = -img_w-plane_count;
  r0 = &base[cand2+img_w+plane_count];
  __frame.r0_f = r0;
  uint8_t *r1 = r0+d1;
  __frame.r1_f = r1;
  r2 = r0+__frame.d2_f;
  __frame.cursor = r2;
  do {
    int32_t e0 = r0[__frame.off_up];
    __frame.r1_f = r1;
    __frame.r0_f = r0;
    __frame.cursor = r2;
    int32_t e1 = e0+*r0-r0[__frame.best]-r0[nstep];
    int32_t e2 = *r1;
    left = __frame.rows;
    r2 = &__frame.cursor[-nstep];
    int32_t bin2 = ((uint16_t)e1-(uint16_t)((__frame.wa*(r1[__frame.off_up]+e2-r1[__frame.best]-r1[nstep])+__frame.wb*(__frame.cursor[__frame.off_up]+*__frame.cursor-__frame.cursor[__frame.best]-(uint32_t)__frame.cursor[nstep])+40)>>7)-256)&0x1FF;
    r1 -= nstep;
    ++*(uint32_t*)&__frame.buf_1[4*bin2];
    r0 = &__frame.r0_f[-nstep];
    __frame.rows = left-1;
  } while( left!=1 );
  pick = __frame.d1_f;
  int32_t c0 = __estimate_cost((uint8_t*)__frame.buf_1, 512);
  int32_t rec = 4*__frame.cand_i;
  costs[rec] = c0;
  costs[rec+1] = __estimate_cost((uint8_t*)__frame.hist_zx, 1024);
  costs[rec+2] = __estimate_cost((uint8_t*)__frame.hist_zy, 1024);
  costs[rec+3] = __estimate_cost((uint8_t*)__frame.hist_zp, 1024);
  cost = __estimate_cost((uint8_t*)__frame.buf, 1024);
  __frame.d1_f = __estimate_cost((uint8_t*)__frame.hist_y, 1024);
  c2 = __estimate_cost((uint8_t*)__frame.hist_yx, 1024);
  __frame.rows = c2;
  lo1 = c2;
  if( cost<c2 )
    lo1 = cost;
  if( __frame.d1_f<c2 )
    c2 = __frame.d1_f;
  int32_t s1 = __frame.d1_f+lo1;
  int32_t s2 = cost+c2;
  __frame.best = s2;
  int32_t c2b = __frame.rows;
  if( s1<s2 ) {
    __frame.best = s1;
    int32_t tmp = pick;
    pick = __frame.d2_f;
    cost = __frame.d1_f;
    __frame.d2_f = tmp;
    uint32_t swap = costs[rec+1];
    costs[rec+1] = costs[rec+2];
    int32_t w2s = __frame.wb;
    costs[rec+2] = swap;
    int32_t w1s = __frame.wa;
    __frame.wa = w2s;
    __frame.wb = w1s;
  }
  int32_t off = __frame.rec_off;
  uint8_t* descp = __frame.desc_f;
  *(uint32_t*)&__frame.desc_f[__frame.rec_off+4] = __frame.wa;
  *(uint32_t*)&descp[off+8] = __frame.wb;
  int32_t idx1 = __frame.cand_i+pick;
  int32_t idx2 = __frame.cand_i+__frame.d2_f;
  descp[16*idx1] = 0;
  descp[1] = (uint8_t)idx1;
  bool deep = 0;
  descp[16*idx2] = 1;
  descp[17] = (uint8_t)idx2;
  if( !deep&&__frame.img_f->data_size>0x1000000u )
    return cost+c2b+costs[rec];
  lo2 = costs[rec];
  lo3 = costs[rec+2];
  if( lo2>=costs[rec+1] )
    lo2 = costs[rec+1];
  if( lo3>=costs[rec+3] )
    lo3 = costs[rec+3];
  if( lo2<lo3 )
    lo3 = lo2;
  return __frame.best+lo3;
}
