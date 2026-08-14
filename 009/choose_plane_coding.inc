// choose_plane_coding.inc -- decide how each plane of an image is to be coded.

int32_t __choose_plane_coding(BmfImage* img, int32_t unused_h, int8_t unused_c) {
  struct alignas(16) ChoosePlaneCodingFrame {
    uint8_t _pad0[4];
    union {
      uint8_t buf[32768];
      struct {
        uint8_t _buf_head[4096];
        int32_t hist_a[1024];
        int32_t hist_b[1024];
        int32_t hist_c[5120];
      };
    };
    union {
      uint8_t buf_1[2048];
      struct {
        uint8_t _buf_1_head[4];
        int32_t s0;
        int32_t s1;
        int32_t s2;
        uint8_t* p0;
        uint8_t* p1;
        int32_t s3;
        int32_t s4;
        uint8_t* p2;
        int32_t s5;
        int32_t s6;
        int32_t s7;
        uint8_t* p3;
        uint8_t* p4;
        int32_t s8;
        int32_t s9;
        uint8_t* p5;
        int32_t s10;
        uint8_t _pad3[1976];
      };
    };
    union {
      int64_t q0;
      struct {
        int32_t row_stride;
        int32_t plane_b;
      };
    };
    union {
      int64_t q1;
      struct {
        int32_t plane_a;
        uint32_t nplanes_s;
      };
    };
    union {
      double d0;
      uint32_t wt_slot;
    };
    uint8_t _pad6[4];
    uint8_t _pad5[4];
    uint8_t _pad7[28];
  } __frame;
  int32_t best0, best1, best2;
  uint8_t buf_3[2048], buf_4[2048], buf_2[2048];
  alignas(16) int32_t x0[4], x1[4], x2[4], x3[4], x4[4], x5[4];
  double d1, d2, d3;
  // One run of the original frame, and the body indexes across every boundary
  // in it, so the six are bound onto one buffer at the offsets they had:
  //
  //     0   acc0[4]     the three candidates' costs are one 3x4 table --
  //    16   acc1[5]     `acc0[4*xform+2]` and `scratch[16*xform+16]` read
  //    56   d4          rows 1 and 2 of it from inside `acc1`
  //    64   tbl16[16]   and the three 64-byte tables are one 3x16 table:
  //   128   tbl64a[64]  `&tbl16[16*xform]` selects a row by walking off the
  //   192   tbl64b[64]  end of the first, and the loop above zeroes all 192
  //                     bytes of it as one block -- `scratch[64..255]`
  //
  // `acc0` and `acc1` were bound here when the frame was lifted; the tables
  // were left as separate locals, and `tbl16` was placed 8 bytes early, over
  // `d4`.  So for `xform != 0` the 64 bytes this copies into `plane_desc` --
  // the image's plane order -- came from whatever the compiler put after
  // `tbl16`, and three planes claimed the same source while two were never
  // coded.  Every image in the corpus picks `xform == 0`, where the row is the
  // buffer `__cost_candidate` was actually given, so the gate said nothing;
  // `testfiles/xform1.bmp` is the image that does not.
  alignas(16) uint8_t scratch[256];
  int32_t(&acc0)[4] = *(int32_t(*)[4])&scratch[0];
  uint64_t(&acc1)[5] = *(uint64_t(*)[5])&scratch[16];
  double&d4 = *(double*)&scratch[56];
  int32_t(&tbl16)[16] = *(int32_t(*)[16])&scratch[64];
  uint8_t(&tbl64a)[64] = *(uint8_t(*)[64])&scratch[128];
  uint8_t(&tbl64b)[64] = *(uint8_t(*)[64])&scratch[192];
  int64_t q2;
  double d5, d6;
  uint8_t* pp;
  uint32_t uu;
  int32_t wa_slot;
  bool keep0, cheaper, keep3, pick0;
  int8_t unread0, unread1, unread2, pos, pos3;
  uint8_t* hist;
  double sum22, sum11, dv0, dv1, dv2, sum01, sum02, sum12, sum02_c, inv;
  int16_t g1_lo;
  uint32_t n_quads, next_plane, row, slack;
  uint16_t g1w;
  int32_t n_planes, data_size, result, pick01, xform, dw, *win_row, wt8, wt4, c2, c2w, c1, c1w, c0, bin0, pred, xform_row, win, sum, best_sum, i, pos2, best_sum2, sum2, wt4_dn, wt8_dn, wt4_dn_end, wt8_dn_end, dv3, wa, wb, wc, nx0, dg0, dn0, ad0, dnx1, g0, dn1, nx2, g1, dnx2, g2, pa, pb, bin_lin, quad_r, alpha, bin_lin2, r0, r1, r2, wa_pick, pred4, pred4b, *hist2, sum3, best_sum3, n_c, wt4_best_dn, n_d, wt8_best_dn, wt4_up, wt8_up, wt4_up_end, wt8_up_end, wt4_best, wt8_best;
  uint32_t cost0, cost1, best_cost, best, cost_flat, i4, quad, cost_lin, best4, cost_c0, cost_c1, cost_c2;
  uint8_t *px, *end_px, *r_c, *p_c, *r_d, *p_d;
  n_planes = plane_count;
  data_size = img->data_size;
  BmfImage* img_a = (BmfImage*)((uint8_t*)img);
  alphabet_reduced = 1;
  __frame.row_stride = (uint16_t)img->stride;
  const uint8_t* data_end = &img->pixels[data_size];
  memset(__frame.buf, 0, 0x8000);
  result = 192;
  do {
    bmf_zero16(&scratch[result+48]);
    bmf_zero16(&scratch[result+32]);
    bmf_zero16(&scratch[result+16]);
    bmf_zero16(&scratch[result]);
    result -= 64;
  } while( result );
  if( n_planes>0 ) {
    if( n_planes/2 ) {
      uint32_t pair = 0;
      do {
        uint8_t pl_even = 2*pair;
        row = 2*pair;
        plane_desc[row+1].src_plane = 2*pair;
        uint8_t pl_odd = 2*pair+++1;
        plane_desc[row+1].nrefs = pl_even;
        plane_desc[row+2].src_plane = pl_odd;
        plane_desc[row+2].nrefs = pl_odd;
      } while( pair<(uint32_t)(n_planes/2) );
      next_plane = 2*pair+1;
    } else {
      next_plane = 1;
    }
    result = next_plane-1;
    if( (uint32_t)n_planes>(next_plane-1) ) {
      plane_desc[next_plane].src_plane = result;
      plane_desc[next_plane].nrefs = result;
    }
    if( n_planes>=3 ) {
      cost0 = __cost_candidate((uint8_t*)img_a, 0, (uint8_t*)tbl16, unread0, best0, best1, best2, (uint32_t*)&acc0[2]);
      cost1 = __cost_candidate((uint8_t*)img_a, 1, tbl64a, unread1, best0, best1, best2, (uint32_t*)&acc0[2]);
      keep0 = cost1>=cost0;
      if( cost1>=cost0 )
        cost1 = cost0;
      pick01 = !keep0;
      x3[2] = pick01;
      cheaper = (uint32_t)(__cost_candidate((uint8_t*)img_a, 2, tbl64b, unread2, best0, best1, best2, (uint32_t*)&acc0[2]))<cost1;
      xform = pick01;
      if( cheaper )
        xform = 2;
      x3[2] = xform;
      dw = 16;
      win_row = &tbl16[16*xform];
      do {
        *(uint64_t*)(bmf_plane_desc(dw*4-8)) = *(uint64_t*)&win_row[dw-2];
        *(uint64_t*)(bmf_plane_desc(dw*4-16)) = *(uint64_t*)&win_row[dw-4];
        *(uint64_t*)(bmf_plane_desc(dw*4-24)) = *(uint64_t*)&win_row[dw-6];
        *(uint64_t*)(bmf_plane_desc(dw*4-32)) = *(uint64_t*)&win_row[dw-8];
        dw -= 8;
      } while( dw*4 );
      best_cost = acc0[4*x3[2]+2];
      x3[1] = 16*x3[2];
      __frame.plane_a = plane_desc[1].src_plane-x3[2];
      wt8 = plane_desc[x3[2]+1].weight1;
      __frame.plane_b = plane_desc[2].src_plane-x3[2];
      wt4 = plane_desc[x3[2]+1].weight0;
      {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        __frame.nplanes_s = n_planes;
        int32_t wt_step = 4;
        __frame.s1 = wt4-1;
        wt4_dn = wt4-1;
        __frame.s6 = wt8-1;
        wt8_dn = wt8-1;
        __frame.s0 = wt4-wt_step;
        wt4_dn_end = __frame.s0;
        *(uint32_t*)__frame.buf_1 = wt8-wt_step;
        uint8_t *plane0 = &img_a->pixels[x3[2]];
        wt8_dn_end = wt8-wt_step;
        while( 1 ) {
          if( wt4_dn<=wt4_dn_end ) {
            if( wt8_dn<=wt8_dn_end ) {
              x1[0] = x5[3]+1;
              wt4_up = x5[3]+1;
              x3[3] = __frame.wt_slot+1;
              wt8_up = __frame.wt_slot+1;
              x0[3] = x5[3]+wt_step;
              wt4_up_end = x5[3]+wt_step;
              x0[2] = __frame.wt_slot+wt_step;
              wt8_up_end = __frame.wt_slot+wt_step;
              while( 1 ) {
                if( wt4_up>=wt4_up_end&&wt8_up>=wt8_up_end ) {
                  wt8 = __frame.wt_slot;
                  wt4 = x5[3];
                  n_planes = __frame.nplanes_s;
                  goto LABEL_19;
                }
                if( wt4_up<wt4_up_end&&wt4_up<192 ) {
                  x0[2] = wt8_up_end;
                  memset(__frame.buf_1, 0, 2048);
                  int32_t npix_a = img_a->width*(img_a->height-1);
                  int32_t stride_a = (uint16_t)img_a->stride;
                  x3[3] = wt8_up;
                  x1[0] = wt4_up;
                  int32_t n_a = npix_a-1;
                  int32_t ofs_up_a = -stride_a;
                  int32_t left_a = -plane_count;
                  int32_t ofs_ul_a = -stride_a-plane_count;
                  uint8_t *p_a = plane0-ofs_ul_a;
                  uint8_t *q_a = plane0+__frame.plane_a-ofs_ul_a;
                  uint8_t *row_c = plane0+__frame.plane_b-ofs_ul_a;
                  do {
                    int32_t ul_a = p_a[ofs_ul_a];
                    int32_t g_a = ul_a+*p_a-p_a[ofs_up_a]-p_a[left_a];
                    int32_t cur_a = *q_a;
                    int32_t bin_a = ((int16_t)g_a-(uint16_t)((x1[0]*(q_a[ofs_ul_a]+cur_a-q_a[ofs_up_a]-q_a[left_a])+__frame.wt_slot*(row_c[ofs_ul_a]+*row_c-row_c[ofs_up_a]-(uint32_t)row_c[left_a])+40)>>7)-256)&0x1FF;
                    ++*(uint32_t*)&__frame.buf_1[4*bin_a];
                    q_a -= left_a;
                    p_a -= left_a;
                    row_c -= left_a;
                    --n_a;
                  } while( n_a );
                  wt8_up = x3[3];
                  wt4_up = x1[0];
                  uint32_t cost_a = __estimate_cost((uint8_t*)__frame.buf_1, 512);
                  wt8_up_end = x0[2];
                  wt4_best = x5[3];
                  if( cost_a<best_cost ) {
                    best_cost = cost_a;
                    wt4_best = wt4_up;
                  }
                  x5[3] = wt4_best;
                  wt4_up_end = wt_step+wt4_best;
                }
                if( wt8_up<wt8_up_end&&wt8_up<192 ) {
                  {
                    x0[3] = wt4_up_end;
                    memset(buf_2, 0, 2048);
                    int32_t npix_b = img_a->width*(img_a->height-1);
                    int32_t stride_b = (uint16_t)img_a->stride;
                    x3[3] = wt8_up;
                    x1[0] = wt4_up;
                    int32_t n_b = npix_b-1;
                    int32_t ofs_up_b = -stride_b;
                    int32_t left_b = -plane_count;
                    int32_t ofs_ul_b = -stride_b-plane_count;
                    uint8_t *p_b = plane0-ofs_ul_b;
                    uint8_t *q_b = plane0+__frame.plane_a-ofs_ul_b;
                    uint8_t *row_d = plane0+__frame.plane_b-ofs_ul_b;
                    do {
                      int32_t ul_b = p_b[ofs_ul_b];
                      int32_t g_b = ul_b+*p_b-p_b[ofs_up_b]-p_b[left_b];
                      int32_t cur_b = *q_b;
                      int32_t bin_b = ((int16_t)g_b-(uint16_t)((x5[3]*(q_b[ofs_ul_b]+cur_b-q_b[ofs_up_b]-q_b[left_b])+x3[3]*(row_d[ofs_ul_b]+*row_d-row_d[ofs_up_b]-(uint32_t)row_d[left_b])+40)>>7)-256)&0x1FF;
                      ++*(uint32_t*)&buf_2[4*bin_b];
                      q_b -= left_b;
                      p_b -= left_b;
                      row_d -= left_b;
                      --n_b;
                    } while( n_b );
                    wt8_up = x3[3];
                    wt4_up = x1[0];
                    uint32_t cost_b = __estimate_cost((uint8_t*)buf_2, 512);
                    wt4_up_end = x0[3];
                    wt8_best = __frame.wt_slot;
                    if( cost_b<best_cost ) {
                      best_cost = cost_b;
                      wt8_best = wt8_up;
                    }
                    __frame.wt_slot = wt8_best;
                    wt8_up_end = wt_step+wt8_best;
                  }
                }
                ++wt4_up;
                ++wt8_up;
              }
            }
          }
          if( wt4_dn>wt4_dn_end&&wt4_dn>=-64 ) {
            *(uint32_t*)__frame.buf_1 = wt8_dn_end;
            memset(buf_3, 0, 2048);
            int32_t npix_c = img_a->width*(img_a->height-1);
            int32_t stride_c = (uint16_t)img_a->stride;
            __frame.s6 = wt8_dn;
            __frame.s1 = wt4_dn;
            __frame.s2 = npix_c-1;
            __frame.s4 = -stride_c;
            int32_t left_c = -plane_count;
            __frame.s3 = -stride_c-plane_count;
            __frame.p1 = plane0-__frame.s3;
            __frame.p0 = plane0+__frame.plane_a-__frame.s3;
            uint8_t *q_c = __frame.p0;
            r_c = plane0+__frame.plane_b-__frame.s3;
            p_c = __frame.p1;
            do {
              int32_t ul_c = p_c[__frame.s3];
              __frame.p0 = q_c;
              __frame.p1 = p_c;
              __frame.p2 = r_c;
              int32_t g_c = ul_c+*p_c-p_c[__frame.s4]-p_c[left_c];
              int32_t cur_c = *q_c;
              __frame.s5 = g_c;
              r_c = &__frame.p2[-left_c];
              n_c = __frame.s2;
              int32_t bin_c = ((uint16_t)__frame.s5-(uint16_t)((__frame.s1*(q_c[__frame.s3]+cur_c-q_c[__frame.s4]-q_c[left_c])+__frame.wt_slot*(__frame.p2[__frame.s3]+*__frame.p2-__frame.p2[__frame.s4]-(uint32_t)__frame.p2[left_c])+40)>>7)-256)&0x1FF;
              ++*(uint32_t*)&buf_3[4*bin_c];
              q_c -= left_c;
              p_c = &__frame.p1[-left_c];
              __frame.s2 = n_c-1;
            } while( n_c!=1 );
            wt8_dn = __frame.s6;
            wt4_dn = __frame.s1;
            uint32_t cost_c = __estimate_cost((uint8_t*)buf_3, 512);
            wt8_dn_end = *(uint32_t*)__frame.buf_1;
            wt4_best_dn = x5[3];
            if( cost_c<best_cost ) {
              best_cost = cost_c;
              wt4_best_dn = wt4_dn;
            }
            x5[3] = wt4_best_dn;
            wt4_dn_end = wt4_best_dn-wt_step;
          }
          if( wt8_dn>wt8_dn_end&&wt8_dn>=-64 ) {
            {
              __frame.s0 = wt4_dn_end;
              memset(buf_4, 0, 2048);
              int32_t npix_d = img_a->width*(img_a->height-1);
              int32_t stride_d = (uint16_t)img_a->stride;
              __frame.s6 = wt8_dn;
              __frame.s1 = wt4_dn;
              __frame.s7 = npix_d-1;
              __frame.s9 = -stride_d;
              int32_t left_d = -plane_count;
              __frame.s8 = -stride_d-plane_count;
              __frame.p4 = plane0-__frame.s8;
              uint8_t *q_d = plane0+__frame.plane_a-__frame.s8;
              __frame.p3 = q_d;
              r_d = plane0+__frame.plane_b-__frame.s8;
              p_d = __frame.p4;
              do {
                int32_t ul_d = p_d[__frame.s8];
                __frame.p3 = q_d;
                __frame.p4 = p_d;
                __frame.p5 = r_d;
                int32_t g_d = ul_d+*p_d-p_d[__frame.s9]-p_d[left_d];
                int32_t cur_d = *q_d;
                __frame.s10 = g_d;
                r_d = &__frame.p5[-left_d];
                n_d = __frame.s7;
                int32_t bin_d = ((uint16_t)__frame.s10-(uint16_t)((x5[3]*(q_d[__frame.s8]+cur_d-q_d[__frame.s9]-q_d[left_d])+__frame.s6*(__frame.p5[__frame.s8]+*__frame.p5-__frame.p5[__frame.s9]-(uint32_t)__frame.p5[left_d])+40)>>7)-256)&0x1FF;
                ++*(uint32_t*)&buf_4[4*bin_d];
                q_d -= left_d;
                p_d = &__frame.p4[-left_d];
                __frame.s7 = n_d-1;
              } while( n_d!=1 );
              wt8_dn = __frame.s6;
              wt4_dn = __frame.s1;
              uint32_t cost_d = __estimate_cost((uint8_t*)buf_4, 512);
              wt4_dn_end = __frame.s0;
              wt8_best_dn = __frame.wt_slot;
              if( cost_d<best_cost ) {
                best_cost = cost_d;
                wt8_best_dn = wt8_dn;
              }
              __frame.wt_slot = wt8_best_dn;
              wt8_dn_end = wt8_best_dn-wt_step;
            }
          }
          --wt4_dn;
          --wt8_dn;
        }
      }
LABEL_19:
      px = &((uint8_t*)img_a)[__frame.row_stride+16+n_planes+x3[2]];
      if( px<data_end ) {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        __frame.nplanes_s = n_planes;
        do {
          c2 = px[__frame.plane_b];
          c2w = c2*__frame.wt_slot;
          c1 = px[__frame.plane_a];
          c1w = c1*x5[3];
          ++__frame.hist_c[c2-c1+1280];
          c0 = *px+512;
          px += __frame.nplanes_s;
          bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
          ++*(uint32_t*)&__frame.buf[4*bin0];
          ++__frame.hist_a[c0-c1];
          ++__frame.hist_b[c0-c2];
          ++__frame.hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
        } while( px<data_end );
        wt8 = __frame.wt_slot;
        wt4 = x5[3];
        n_planes = __frame.nplanes_s;
      }
      slack = best_cost>>7;
      if( best_cost>>7>=0x4000 )
        slack = 0x4000;
      best = slack+best_cost;
      cost_flat = *(int32_t*)((uint8_t*)&acc0[3]+x3[1]);
      cheaper = cost_flat<best;
      if( cost_flat<best ) {
        best = *(int32_t*)((uint8_t*)&acc0[3]+x3[1]);
        wt4 = 128;
        wt8 = 0;
      }
      pred = cheaper;
      if( *(uint32_t*)&scratch[x3[1]+16]<best ) {
        best = *(uint32_t*)&scratch[x3[1]+16];
        pred = 2;
        wt4 = 0;
        wt8 = 128;
      }
      keep3 = best<=*(uint32_t*)&scratch[x3[1]+20];
      if( best>*(uint32_t*)&scratch[x3[1]+20] )
        pred = 3;
      xform_row = x3[2];
      if( !keep3 ) {
        wt4 = 64;
        wt8 = 64;
      }
      plane_desc[x3[2]+1].weight0 = wt4;
      plane_desc[xform_row+1].weight1 = wt8;
      hist = &__frame.buf[4096*pred];
      win = (uintptr_t)hist&0xF;
      sum = 0;
      for( i = 0; i<256; ++i )
        sum += *(int32_t*)&hist[4*(win+i)];
      win += 256;
      best_sum = sum;
      pos = -1;
      if( win<1024 ) {
        __frame.nplanes_s = n_planes;
        do {
          sum = *(uint32_t*)&hist[4*win]+sum-*(uint32_t*)&hist[4*win-1024];
          if( sum>=best_sum ) {
            pos = win;
            best_sum = sum;
          }
          ++win;
        } while( win<1024 );
        n_planes = __frame.nplanes_s;
      }
      plane_desc[x3[2]+1].dc = pos+1;
      best_sum2 = 0;
      for( i = 0; i<0x100; ++i )
        best_sum2 += __frame.hist_c[i+1024];
      pos2 = 255;
      for( sum2 = best_sum2; i<512; ++i ) {
        sum2 = __frame.hist_c[i+1024]+sum2-__frame.hist_c[i+768];
        if( sum2>=best_sum2 ) {
          pos2 = i;
          best_sum2 = sum2;
        }
      }
      result = pos2+1;
      plane_desc[__frame.plane_b+x3[2]+1].dc = result;
      if( n_planes>=4 ) {
        __builtin_memset(x0, 0, 16);
        __builtin_memset(x1, 0, 16);
        __builtin_memset(x2, 0, 16);
        __builtin_memset(x3, 0, 16);
        __builtin_memset(x4, 0, 16);
        __builtin_memset(x5, 0, 16);
        memset(__frame.buf, 0, 0x8000);
        pp = &((uint8_t*)img_a)[__frame.row_stride];
        n_quads = (data_end-17-pp)/4;
        end_px = &((uint8_t*)img_a)[__frame.row_stride+20];
        if( data_end<=end_px ) {
          sum01 = *(double*)&x0[2];
          sum02 = *(double*)x1;
          sum12 = *(double*)&x2[2];
          __frame.q1 = *(int64_t*)&x0[2];
          __frame.q0 = *(int64_t*)&x2[2];
          d4 = *(double*)x0;
          q2 = *(int64_t*)&x4[2];
          sum02_c = *(double*)x1;
        } else {
          sum22 = *(double*)x4;
          sum11 = *(double*)x2;
          q2 = *(int64_t*)&x4[2];
          d4 = *(double*)x0;
          d5 = *(double*)&x0[2];
          d2 = *(double*)x1;
          d6 = *(double*)&x2[2];
          d3 = *(double*)x5;
          d1 = *(double*)&x5[2];
          i4 = 0;
          uu = (data_end-17-pp)/4;
          do {
            dv0 = (double)(((uint8_t*)img_a)[4*i4+16]+pp[4*i4+20]-(((uint8_t*)img_a)[4*i4+20]+pp[4*i4+16]));
            dv1 = (double)(((uint8_t*)img_a)[4*i4+17]+pp[4*i4+21]-(((uint8_t*)img_a)[4*i4+21]+pp[4*i4+17]));
            dv2 = (double)(((uint8_t*)img_a)[4*i4+18]+pp[4*i4+22]-(((uint8_t*)img_a)[4*i4+22]+pp[4*i4+18]));
            dv3 = ((uint8_t*)img_a)[4*i4+19]+pp[4*i4+23]-(((uint8_t*)img_a)[4*i4+23]+pp[4*i4+19]);
            d4 = d4+dv0*dv0;
            ++i4;
            d5 = d5+dv0*dv1;
            d2 = d2+dv0*dv2;
            sum11 = sum11+dv1*dv1;
            d6 = d6+dv1*dv2;
            sum22 = sum22+dv2*dv2;
            *(double*)&q2 = *(double*)&q2+dv0*(double)dv3;
            d3 = d3+dv1*(double)dv3;
            d1 = d1+dv2*(double)dv3;
          } while( i4<uu );
          sum01 = d5;
          sum02 = d2;
          sum12 = d6;
          n_quads = uu;
          *(double*)&x5[2] = d1;
          *(double*)x5 = d3;
          *(int64_t*)&x4[2] = q2;
          *(double*)x4 = sum22;
          *(double*)&x2[2] = d6;
          *(double*)x2 = sum11;
          *(double*)x1 = d2;
          *(double*)&x0[2] = d5;
          *(double*)x0 = d4;
          sum02_c = d2;
          *(double*)&__frame.q0 = d6;
          *(double*)&__frame.q1 = d5;
        }
        d5 = sum01;
        d6 = sum12;
        __frame.d0 = sum02_c;
        *(double*)&x1[2] = sum01;
        *(double*)x3 = sum02;
        *(double*)&x3[2] = sum12;
        d1 = d4**(double*)x4-sum02_c*sum02;
        d2 = 0.0-d4*sum12+sum02**(double*)&__frame.q1;
        d3 = sum02_c*sum12-*(double*)&__frame.q1**(double*)x4;
        inv = 128.0/(*(double*)x2*d1+*(double*)&__frame.q0*d2+sum01*d3+0.1);
        *(double*)acc0 = inv;
        wa = (int32_t)(((sum02**(double*)&__frame.q0-sum01**(double*)x4)**(double*)x5+(0.0-sum02**(double*)x2+sum01*d6)**(double*)&x5[2]+(*(double*)x4**(double*)x2-d6**(double*)&__frame.q0)**(double*)&q2)*inv);
        if( wa>=191 )
          wa = 191;
        if( wa<-64 )
          wa = -64;
        wa_slot = wa;
        wb = (int32_t)((d1**(double*)x5+d2**(double*)&x5[2]+d3**(double*)&q2)*inv);
        if( wb>=191 )
          wb = 191;
        if( wb<-64 )
          wb = -64;
        wc = (int32_t)(*(double*)acc0*((d4**(double*)&x5[2]-__frame.d0**(double*)&q2)**(double*)x2+(0.0-d4**(double*)x5+*(double*)&q2**(double*)&__frame.q1)**(double*)&__frame.q0+(__frame.d0**(double*)x5-*(double*)&__frame.q1**(double*)&x5[2])*d5));
        if( wc>=191 )
          wc = 191;
        if( wc<-64 )
          wc = -64;
        if( end_px<data_end ) {
          *(int64_t*)x0 = __PAIR64__(wc, wb);
          uu = n_quads;
          quad = 0;
          do {
            nx0 = ((uint8_t*)img_a)[4*quad+20];
            dg0 = ((uint8_t*)img_a)[4*quad+16]+pp[4*quad+20];
            dn0 = pp[4*quad+16];
            x0[2] = quad;
            ad0 = nx0+dn0;
            dnx1 = pp[4*quad+21];
            g0 = dg0-ad0;
            dn1 = pp[4*quad+17];
            x0[3] = g0;
            nx2 = ((uint8_t*)img_a)[4*quad+22];
            g1 = ((uint8_t*)img_a)[4*quad+17]+dnx1-(((uint8_t*)img_a)[4*quad+21]+dn1);
            dnx2 = pp[4*quad+22];
            x1[0] = g1;
            g2 = ((uint8_t*)img_a)[4*quad+18]+dnx2-(nx2+pp[4*quad+18]);
            g1w = (uint16_t)(((uint8_t*)img_a)[4*quad+19]+pp[4*quad+23]-pp[4*quad+19]-((uint8_t*)img_a)[4*quad+23]);
            g1_lo = ((int16_t*)x1)[0];
            g1w = g1w-512;
            pa = x1[0]*x0[0]+x0[3]*wa_slot;
            pb = g2*x0[1];
            ++__frame.hist_a[(g1w-((int16_t*)x0)[6])&0x3FF];
            bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
            ++*(uint32_t*)&__frame.buf[4*bin_lin];
            ++__frame.hist_b[(g1w-g1_lo)&0x3FF];
            g1w = g1w-g2;
            quad_r = x0[2];
            ++__frame.hist_c[g1w&0x3FF];
            alpha = pp[4*quad_r+23]+256;
            bin_lin2 = ((uint16_t)alpha-(uint16_t)((x0[0]*pp[4*quad_r+21]+wa_slot*pp[4*quad_r+20]+x0[1]*(uint32_t)pp[4*quad_r+22]+63)>>7)+256)&0x3FF;
            ++__frame.hist_c[bin_lin2+1024];
            r0 = alpha-pp[4*quad_r+20];
            ++__frame.hist_c[r0+2048];
            r1 = alpha-pp[4*quad_r+21];
            ++__frame.hist_c[r1+3072];
            r2 = alpha-pp[4*quad_r+22];
            ++__frame.hist_c[r2+4096];
            quad = quad_r+1;
          } while( quad<uu );
          wc = x0[1];
          wb = x0[0];
        }
        cost_lin = __estimate_cost((uint8_t*)__frame.buf, 1024);
        best4 = (cost_lin>>7)+cost_lin;
        cost_c0 = __estimate_cost((uint8_t*)__frame.hist_a, 1024);
        pick0 = cost_c0<best4;
        if( cost_c0<best4 )
          best4 = cost_c0;
        wa_pick = wa_slot;
        if( pick0 ) {
          wa_pick = 128;
          wc = 0;
          wb = 0;
        }
        wa_slot = wa_pick;
        x0[0] = pick0;
        cost_c1 = __estimate_cost((uint8_t*)__frame.hist_b, 1024);
        pred4 = x0[0];
        if( cost_c1<best4 ) {
          best4 = cost_c1;
          pred4 = 2;
          wb = 128;
          wc = 0;
          wa_slot = 0;
        }
        x0[0] = pred4;
        cost_c2 = __estimate_cost((uint8_t*)__frame.hist_c, 1024);
        pred4b = x0[0];
        if( cost_c2<best4 ) {
          pred4b = 3;
          wc = 128;
          wb = 0;
          wa_slot = 0;
        }
        plane_desc[4].weight0 = wa_slot;
        plane_desc[4].weight1 = wb;
        plane_desc[4].weight2 = wc;
        plane_desc[4].src_plane = 3;
        plane_desc[4].nrefs = 3;
        hist2 = &__frame.hist_c[1024*pred4b+1024];
        result = (uintptr_t)hist2&0xF;
        sum3 = 0;
        for( i = 0; i<256; ++i )
          sum3 += hist2[result+i];
        result += 256;
        best_sum3 = sum3;
        for( pos3 = -1; result<1024; ++result ) {
          sum3 = hist2[result]+sum3-hist2[result-256];
          if( sum3>=best_sum3 ) {
            pos3 = result;
            best_sum3 = sum3;
          }
        }
        plane_desc[4].dc = pos3+1;
      }
    }
  }
  return result;
}
