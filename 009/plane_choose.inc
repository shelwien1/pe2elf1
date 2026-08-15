// plane_choose.inc -- decide how each plane of an image is to be coded.

int32_t choose_plane_coding(BmfImage* img) {
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
  } frame;
  // Three 512-entry histograms of `bin` values, one per candidate arm.
  // They were `uint8_t[2048]` written as `*(uint32_t*)&buf[4*bin]` and
  // read by `estimate_cost`, which takes bytes -- so the byte array was
  // the parameter's type reaching back into the declaration.
  uint32_t hist_arm_b[512], hist_arm_c[512], hist_arm_d[512];
  // What is left of the spill area: five values the nested weight search
  // carries, each the working copy of the local it is read into.  The
  // arrays are gone -- nothing crossed between their slots, so they were
  // five independent variables under four array names.
  int32_t wt4_cur, wt4_up_cur, wt4_up_end_cur, wt8_up_cur, wt8_up_end_cur;
  double d1, d2, d3;
  // One run of the original frame, and the body indexes across every boundary
  // in it, so the six are bound onto one buffer at the offsets they had:
  //
  //     0   acc0[4]     the three candidates' costs are one 3x4 table --
  //    16   acc1[5]     `acc0[4*xform+2]` and `scratch[16*xform+16]` read
  //    56   d4          rows 1 and 2 of it from inside `acc1`
  //    64   tbl16[16]   and the three 64-byte tables are one 3x16 table:
  //   128   (row 1)     `&tbl16[16*cand]` selects a row by walking off the
  //   192   (row 2)     end of the first, and the loop above zeroes all 192
  //                     bytes of it as one block -- `scratch[64..255]`
  //
  // Rows 1 and 2 had names, `tbl64a` and `tbl64b`, until the candidate loop
  // started reaching all three by index.  Like `acc1` they then bound a region
  // nothing read through, which is a comment the compiler has to be told to
  // ignore; the map is the comment.
  //
  // `acc1` is not bound below.  Nothing reads that region through a name --
  // `acc0[4*xform+2]` reaches rows 1 and 2 of the cost table from inside it,
  // which is the whole of its traffic -- so a reference there was a comment
  // the compiler had to be told to ignore.  The map above is the comment.
  //
  // `acc0` and `acc1` were bound here when the frame was lifted; the tables
  // were left as separate locals, and `tbl16` was placed 8 bytes early, over
  // `d4`.  So for `xform != 0` the 64 bytes this copies into `plane_desc` --
  // the image's plane order -- came from whatever the compiler put after
  // `tbl16`, and three planes claimed the same source while two were never
  // coded.  Every image in the corpus picks `xform == 0`, where the row is the
  // buffer `cost_candidate` was actually given, so the gate said nothing;
  // `testfiles/xform1.bmp` is the image that does not.
  alignas(16) uint8_t scratch[256];
  int32_t(&acc0)[4] = *(int32_t(*)[4])&scratch[0];
  int32_t(&tbl16)[16] = *(int32_t(*)[16])&scratch[64];
  uint8_t* pp;
  uint32_t uu;
  int32_t wa_slot;
  bool cheaper, keep3, pick0;
  int8_t pos, pos3;
  uint8_t* hist;
  double dv0, dv1, dv2, inv;
  int16_t g1_lo;
  uint32_t n_quads, next_plane, row, slack;
  uint16_t g1w;
  int32_t n_planes, data_size, result, xform, *win_row, wt8, wt4, c2, c2w, c1, c1w, c0, bin0, pred, win, sum, best_sum, i, pos2, best_sum2, sum2, wt4_dn, wt8_dn, wt4_dn_end, wt8_dn_end, dv3, wa, wb, wc, nx0, dg0, dn0, ad0, dnx1, g0, dn1, nx2, g1, dnx2, g2, pa, pb, bin_lin, alpha, bin_lin2, r0, r1, r2, wa_pick, pred4, pred4b, *hist2, sum3, best_sum3, n_c, wt4_best_dn, n_d, wt8_best_dn, wt4_up, wt8_up, wt4_up_end, wt8_up_end, wt4_best, wt8_best;
  uint32_t best_cost, best, cost_flat, i4, quad, cost_lin, best4, cost_c0, cost_c1, cost_c2;
  uint8_t *px, *end_px, *r_c, *p_c, *r_d, *p_d;
  n_planes = plane_count;
  data_size = img->data_size;
  alphabet_reduced = 1;
  frame.row_stride = (uint16_t)img->stride;
  const uint8_t* data_end = &img->pixels[data_size];
  memset(frame.buf, 0, 0x8000);
  // The three 64-byte tables, as one block: `scratch[64..255]`.  The first 64
  // bytes are `acc0`, `acc1` and `d4` and are deliberately left alone -- the
  // loop this replaces started at 192 and stopped before reaching them.
  memset(&scratch[64], 0, 192);
  if( n_planes>0 ) {
    if( n_planes/2 ) {
      uint32_t pair = 0;
      do {
        uint8_t pl_even = 2*pair;
        row = 2*pair;
        plane_desc[row].src_plane = 2*pair;
        uint8_t pl_odd = 2*pair+++1;
        plane_desc[row].nrefs = pl_even;
        plane_desc[row+1].src_plane = pl_odd;
        plane_desc[row+1].nrefs = pl_odd;
      } while( pair<(uint32_t)(n_planes/2) );
      next_plane = 2*pair+1;
    } else {
      next_plane = 1;
    }
    result = next_plane-1;
    if( (uint32_t)n_planes>(next_plane-1) ) {
      plane_desc[next_plane-1].src_plane = result;
      plane_desc[next_plane-1].nrefs = result;
    }
    if( n_planes>=3 ) {
      // The three candidate transforms, costed, cheapest kept.  Their
      // descriptor buffers are the three rows of one 3x16 table: `tbl16`,
      // `tbl64a` and `tbl64b` name `scratch[64]`, `[128]` and `[192]`, which
      // is `&tbl16[16*cand]` -- the same walking-off-the-end the winning row
      // is selected by twenty lines below.  So the three calls the
      // decompilation wrote out are one loop.
      //
      // Ties go to the lower index, which is what `cost1 >= cost0` keeping
      // candidate 0 did.  The original compared the first pair signed and the
      // third candidate unsigned -- `(uint32_t)f(2) < cost1` -- which are the
      // same relation for costs that are estimated bit counts and so never
      // negative.  One relation here.
      xform = 0;
      uint32_t cheapest = cost_candidate(img, 0, (uint8_t*)tbl16,
                                           (uint32_t*)&acc0[2]);
      for( int32_t cand = 1; cand<3; ++cand ) {
        const uint32_t c = cost_candidate(img, cand,
                                            (uint8_t*)&tbl16[16*cand], (uint32_t*)&acc0[2]);
        if( c<cheapest ) {
          cheapest = c;
          xform = cand;
        }
      }
      // The winning candidate's four descriptors, copied in.  This was eight
      // `uint64_t` stores through a `(uint8_t*)&plane_desc[0]+off` accessor,
      // two turns of a loop counting down by eight: offsets 56, 48, 40, 32
      // then 24, 16, 8, 0, from `win_row[14]` down to `win_row[0]`.  Bytes
      // 0..63 of the descriptors from the sixteen words of the row -- which
      // is 4*sizeof(PlaneDesc), so it is one copy.
      //
      // `&tbl16[16*xform]` walks off the end of `tbl16` on purpose: the three
      // tables are one 3x16 array of candidates that the decompilation gave
      // three names.
      win_row = &tbl16[16*xform];
      memcpy(&plane_desc[0], win_row, 4*sizeof(PlaneDesc));
      best_cost = acc0[4*xform+2];
      // The chosen transform's row in the 3x16 cost table, as a byte
      // offset: `acc0` is `int32_t`, so sixteen bytes is the four-word
      // row `acc0[4*xform + k]` reaches by index twenty lines above.
      int32_t cost_row_ofs = 16*xform;
      frame.plane_a = plane_desc[0].src_plane-xform;
      wt8 = plane_desc[xform].weight1;
      frame.plane_b = plane_desc[1].src_plane-xform;
      wt4 = plane_desc[xform].weight0;
      {
        frame.wt_slot = wt8;
        wt4_cur = wt4;
        frame.nplanes_s = n_planes;
        int32_t wt_step = 4;
        frame.s1 = wt4-1;
        wt4_dn = wt4-1;
        frame.s6 = wt8-1;
        wt8_dn = wt8-1;
        frame.s0 = wt4-wt_step;
        wt4_dn_end = frame.s0;
        *(uint32_t*)frame.buf_1 = wt8-wt_step;
        uint8_t *plane0 = &img->pixels[xform];
        wt8_dn_end = wt8-wt_step;
        while( 1 ) {
          if( wt4_dn<=wt4_dn_end ) {
            if( wt8_dn<=wt8_dn_end ) {
              wt4_up_cur = wt4_cur+1;
              wt4_up = wt4_cur+1;
              wt8_up_cur = frame.wt_slot+1;
              wt8_up = frame.wt_slot+1;
              wt4_up_end_cur = wt4_cur+wt_step;
              wt4_up_end = wt4_cur+wt_step;
              wt8_up_end_cur = frame.wt_slot+wt_step;
              wt8_up_end = frame.wt_slot+wt_step;
              while( 1 ) {
                if( wt4_up>=wt4_up_end&&wt8_up>=wt8_up_end ) {
                  wt8 = frame.wt_slot;
                  wt4 = wt4_cur;
                  n_planes = frame.nplanes_s;
                  goto weights_chosen;
                }
                if( wt4_up<wt4_up_end&&wt4_up<192 ) {
                  wt8_up_end_cur = wt8_up_end;
                  memset(frame.buf_1, 0, 2048);
                  int32_t npix_a = img->width*(img->height-1);
                  int32_t stride_a = (uint16_t)img->stride;
                  wt8_up_cur = wt8_up;
                  wt4_up_cur = wt4_up;
                  int32_t n_a = npix_a-1;
                  int32_t ofs_up_a = -stride_a;
                  int32_t left_a = -plane_count;
                  int32_t ofs_ul_a = -stride_a-plane_count;
                  uint8_t *p_a = plane0-ofs_ul_a;
                  uint8_t *q_a = plane0+frame.plane_a-ofs_ul_a;
                  uint8_t *row_c = plane0+frame.plane_b-ofs_ul_a;
                  do {
                    int32_t ul_a = p_a[ofs_ul_a];
                    int32_t g_a = ul_a+*p_a-p_a[ofs_up_a]-p_a[left_a];
                    int32_t cur_a = *q_a;
                    int32_t bin_a = ((int16_t)g_a-(uint16_t)((wt4_up_cur*(q_a[ofs_ul_a]+cur_a-q_a[ofs_up_a]-q_a[left_a])+frame.wt_slot*(row_c[ofs_ul_a]+*row_c-row_c[ofs_up_a]-(uint32_t)row_c[left_a])+40)>>7)-256)&0x1FF;
                    ++*(uint32_t*)&frame.buf_1[4*bin_a];
                    q_a -= left_a;
                    p_a -= left_a;
                    row_c -= left_a;
                    --n_a;
                  } while( n_a );
                  wt8_up = wt8_up_cur;
                  wt4_up = wt4_up_cur;
                  uint32_t cost_a = estimate_cost((uint8_t*)frame.buf_1, 512);
                  wt8_up_end = wt8_up_end_cur;
                  wt4_best = wt4_cur;
                  if( cost_a<best_cost ) {
                    best_cost = cost_a;
                    wt4_best = wt4_up;
                  }
                  wt4_cur = wt4_best;
                  wt4_up_end = wt_step+wt4_best;
                }
                if( wt8_up<wt8_up_end&&wt8_up<192 ) {
                  {
                    wt4_up_end_cur = wt4_up_end;
                    memset(hist_arm_b, 0, sizeof hist_arm_b);
                    int32_t npix_b = img->width*(img->height-1);
                    int32_t stride_b = (uint16_t)img->stride;
                    wt8_up_cur = wt8_up;
                    wt4_up_cur = wt4_up;
                    int32_t n_b = npix_b-1;
                    int32_t ofs_up_b = -stride_b;
                    int32_t left_b = -plane_count;
                    int32_t ofs_ul_b = -stride_b-plane_count;
                    uint8_t *p_b = plane0-ofs_ul_b;
                    uint8_t *q_b = plane0+frame.plane_a-ofs_ul_b;
                    uint8_t *row_d = plane0+frame.plane_b-ofs_ul_b;
                    do {
                      int32_t ul_b = p_b[ofs_ul_b];
                      int32_t g_b = ul_b+*p_b-p_b[ofs_up_b]-p_b[left_b];
                      int32_t cur_b = *q_b;
                      int32_t bin_b = ((int16_t)g_b-(uint16_t)((wt4_cur*(q_b[ofs_ul_b]+cur_b-q_b[ofs_up_b]-q_b[left_b])+wt8_up_cur*(row_d[ofs_ul_b]+*row_d-row_d[ofs_up_b]-(uint32_t)row_d[left_b])+40)>>7)-256)&0x1FF;
                      ++hist_arm_b[bin_b];
                      q_b -= left_b;
                      p_b -= left_b;
                      row_d -= left_b;
                      --n_b;
                    } while( n_b );
                    wt8_up = wt8_up_cur;
                    wt4_up = wt4_up_cur;
                    uint32_t cost_b = estimate_cost((uint8_t*)hist_arm_b, 512);
                    wt4_up_end = wt4_up_end_cur;
                    wt8_best = frame.wt_slot;
                    if( cost_b<best_cost ) {
                      best_cost = cost_b;
                      wt8_best = wt8_up;
                    }
                    frame.wt_slot = wt8_best;
                    wt8_up_end = wt_step+wt8_best;
                  }
                }
                ++wt4_up;
                ++wt8_up;
              }
            }
          }
          if( wt4_dn>wt4_dn_end&&wt4_dn>=-64 ) {
            *(uint32_t*)frame.buf_1 = wt8_dn_end;
            memset(hist_arm_c, 0, sizeof hist_arm_c);
            int32_t npix_c = img->width*(img->height-1);
            int32_t stride_c = (uint16_t)img->stride;
            frame.s6 = wt8_dn;
            frame.s1 = wt4_dn;
            frame.s2 = npix_c-1;
            frame.s4 = -stride_c;
            int32_t left_c = -plane_count;
            frame.s3 = -stride_c-plane_count;
            frame.p1 = plane0-frame.s3;
            frame.p0 = plane0+frame.plane_a-frame.s3;
            uint8_t *q_c = frame.p0;
            r_c = plane0+frame.plane_b-frame.s3;
            p_c = frame.p1;
            do {
              int32_t ul_c = p_c[frame.s3];
              frame.p0 = q_c;
              frame.p1 = p_c;
              frame.p2 = r_c;
              int32_t g_c = ul_c+*p_c-p_c[frame.s4]-p_c[left_c];
              int32_t cur_c = *q_c;
              frame.s5 = g_c;
              r_c = &frame.p2[-left_c];
              n_c = frame.s2;
              int32_t bin_c = ((uint16_t)frame.s5-(uint16_t)((frame.s1*(q_c[frame.s3]+cur_c-q_c[frame.s4]-q_c[left_c])+frame.wt_slot*(frame.p2[frame.s3]+*frame.p2-frame.p2[frame.s4]-(uint32_t)frame.p2[left_c])+40)>>7)-256)&0x1FF;
              ++hist_arm_c[bin_c];
              q_c -= left_c;
              p_c = &frame.p1[-left_c];
              frame.s2 = n_c-1;
            } while( n_c!=1 );
            wt8_dn = frame.s6;
            wt4_dn = frame.s1;
            uint32_t cost_c = estimate_cost((uint8_t*)hist_arm_c, 512);
            wt8_dn_end = *(uint32_t*)frame.buf_1;
            wt4_best_dn = wt4_cur;
            if( cost_c<best_cost ) {
              best_cost = cost_c;
              wt4_best_dn = wt4_dn;
            }
            wt4_cur = wt4_best_dn;
            wt4_dn_end = wt4_best_dn-wt_step;
          }
          if( wt8_dn>wt8_dn_end&&wt8_dn>=-64 ) {
            {
              frame.s0 = wt4_dn_end;
              memset(hist_arm_d, 0, sizeof hist_arm_d);
              int32_t npix_d = img->width*(img->height-1);
              int32_t stride_d = (uint16_t)img->stride;
              frame.s6 = wt8_dn;
              frame.s1 = wt4_dn;
              frame.s7 = npix_d-1;
              frame.s9 = -stride_d;
              int32_t left_d = -plane_count;
              frame.s8 = -stride_d-plane_count;
              frame.p4 = plane0-frame.s8;
              uint8_t *q_d = plane0+frame.plane_a-frame.s8;
              frame.p3 = q_d;
              r_d = plane0+frame.plane_b-frame.s8;
              p_d = frame.p4;
              do {
                int32_t ul_d = p_d[frame.s8];
                frame.p3 = q_d;
                frame.p4 = p_d;
                frame.p5 = r_d;
                int32_t g_d = ul_d+*p_d-p_d[frame.s9]-p_d[left_d];
                int32_t cur_d = *q_d;
                frame.s10 = g_d;
                r_d = &frame.p5[-left_d];
                n_d = frame.s7;
                int32_t bin_d = ((uint16_t)frame.s10-(uint16_t)((wt4_cur*(q_d[frame.s8]+cur_d-q_d[frame.s9]-q_d[left_d])+frame.s6*(frame.p5[frame.s8]+*frame.p5-frame.p5[frame.s9]-(uint32_t)frame.p5[left_d])+40)>>7)-256)&0x1FF;
                ++hist_arm_d[bin_d];
                q_d -= left_d;
                p_d = &frame.p4[-left_d];
                frame.s7 = n_d-1;
              } while( n_d!=1 );
              wt8_dn = frame.s6;
              wt4_dn = frame.s1;
              uint32_t cost_d = estimate_cost((uint8_t*)hist_arm_d, 512);
              wt4_dn_end = frame.s0;
              wt8_best_dn = frame.wt_slot;
              if( cost_d<best_cost ) {
                best_cost = cost_d;
                wt8_best_dn = wt8_dn;
              }
              frame.wt_slot = wt8_best_dn;
              wt8_dn_end = wt8_best_dn-wt_step;
            }
          }
          --wt4_dn;
          --wt8_dn;
        }
      }
weights_chosen:
      px = &img->pixels[frame.row_stride+n_planes+xform];
      if( px<data_end ) {
        frame.wt_slot = wt8;
        wt4_cur = wt4;
        frame.nplanes_s = n_planes;
        do {
          c2 = px[frame.plane_b];
          c2w = c2*frame.wt_slot;
          c1 = px[frame.plane_a];
          c1w = c1*wt4_cur;
          ++frame.hist_c[c2-c1+1280];
          c0 = *px+512;
          px += frame.nplanes_s;
          bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
          ++*(uint32_t*)&frame.buf[4*bin0];
          ++frame.hist_a[c0-c1];
          ++frame.hist_b[c0-c2];
          ++frame.hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
        } while( px<data_end );
        wt8 = frame.wt_slot;
        wt4 = wt4_cur;
        n_planes = frame.nplanes_s;
      }
      slack = best_cost>>7;
      if( best_cost>>7>=0x4000 )
        slack = 0x4000;
      best = slack+best_cost;
      cost_flat = *(int32_t*)((uint8_t*)&acc0[3]+cost_row_ofs);
      cheaper = cost_flat<best;
      if( cost_flat<best ) {
        best = *(int32_t*)((uint8_t*)&acc0[3]+cost_row_ofs);
        wt4 = 128;
        wt8 = 0;
      }
      pred = cheaper;
      if( *(uint32_t*)&scratch[cost_row_ofs+16]<best ) {
        best = *(uint32_t*)&scratch[cost_row_ofs+16];
        pred = 2;
        wt4 = 0;
        wt8 = 128;
      }
      keep3 = best<=*(uint32_t*)&scratch[cost_row_ofs+20];
      if( best>*(uint32_t*)&scratch[cost_row_ofs+20] )
        pred = 3;
      if( !keep3 ) {
        wt4 = 64;
        wt8 = 64;
      }
      plane_desc[xform].weight0 = wt4;
      plane_desc[xform].weight1 = wt8;
      hist = &frame.buf[4096*pred];
      win = (uintptr_t)hist&0xF;
      sum = 0;
      for( i = 0; i<256; ++i )
        sum += *(int32_t*)&hist[4*(win+i)];
      win += 256;
      best_sum = sum;
      pos = -1;
      if( win<1024 ) {
        frame.nplanes_s = n_planes;
        do {
          sum = *(uint32_t*)&hist[4*win]+sum-*(uint32_t*)&hist[4*win-1024];
          if( sum>=best_sum ) {
            pos = win;
            best_sum = sum;
          }
          ++win;
        } while( win<1024 );
        n_planes = frame.nplanes_s;
      }
      plane_desc[xform].dc = pos+1;
      best_sum2 = 0;
      for( i = 0; i<0x100; ++i )
        best_sum2 += frame.hist_c[i+1024];
      pos2 = 255;
      for( sum2 = best_sum2; i<512; ++i ) {
        sum2 = frame.hist_c[i+1024]+sum2-frame.hist_c[i+768];
        if( sum2>=best_sum2 ) {
          pos2 = i;
          best_sum2 = sum2;
        }
      }
      result = pos2+1;
      plane_desc[frame.plane_b+xform].dc = result;
      if( n_planes>=4 ) {
        // The alpha plane's weights, by least squares against the other three.
        //
        // `dv0`, `dv1` and `dv2` are the three colour planes' gradients at one
        // pixel and `dv3` the alpha plane's; these are the nine covariances of
        // that four-vector, `S<a><b> = sum over pixels of d<a>*d<b>`, with `w`
        // for the alpha term.  The 3x3 system `S * weights = Sw` is solved by
        // cofactors below.
        //
        // The original kept them in six SSE registers and spilled them to the
        // stack, which arrived here as `int32_t x0[4]` .. `x5[4]` read through
        // forty `*(double*)` casts, plus three more slots borrowed from the
        // frame's unions -- `frame.q0` and `q1` overlay `row_stride`,
        // `plane_b`, `plane_a` and `nplanes_s`, and were safe only because
        // those four are last read twenty lines above.  Nine named doubles say
        // the same arithmetic and take the round trip out.
        double Sxx = 0.0, Sxy = 0.0, Sxz = 0.0;
        double Syy = 0.0, Syz = 0.0, Szz = 0.0;
        double Sxw = 0.0, Syw = 0.0, Szw = 0.0;
        memset(frame.buf, 0, 0x8000);
        pp = &img->pixels[frame.row_stride];
        n_quads = (data_end-1-pp)/4;
        end_px = &img->pixels[frame.row_stride+4];
        if( end_px<data_end ) {
          i4 = 0;
          uu = (data_end-1-pp)/4;
          do {
            dv0 = (double)(img->pixels[4*i4]+pp[4*i4+4]-(img->pixels[4*i4+4]+pp[4*i4]));
            dv1 = (double)(img->pixels[4*i4+1]+pp[4*i4+5]-(img->pixels[4*i4+5]+pp[4*i4+1]));
            dv2 = (double)(img->pixels[4*i4+2]+pp[4*i4+6]-(img->pixels[4*i4+6]+pp[4*i4+2]));
            dv3 = img->pixels[4*i4+3]+pp[4*i4+7]-(img->pixels[4*i4+7]+pp[4*i4+3]);
            Sxx += dv0*dv0;
            ++i4;
            Sxy += dv0*dv1;
            Sxz += dv0*dv2;
            Syy += dv1*dv1;
            Syz += dv1*dv2;
            Szz += dv2*dv2;
            Sxw += dv0*(double)dv3;
            Syw += dv1*(double)dv3;
            Szw += dv2*(double)dv3;
          } while( i4<uu );
          n_quads = uu;
        }
        // The three cofactors of the first row of `S`, and one over its
        // determinant -- scaled by 128 because the weights are eighths of a
        // 128th and clamped to [-64, 191] below.  The `+0.1` keeps a singular
        // system (a flat image, every gradient zero) from dividing by zero.
        d1 = Sxx*Szz-Sxz*Sxz;
        d2 = 0.0-Sxx*Syz+Sxz*Sxy;
        d3 = Sxz*Syz-Sxy*Szz;
        inv = 128.0/(Syy*d1+Syz*d2+Sxy*d3+0.1);
        wa = (int32_t)(((Sxz*Syz-Sxy*Szz)*Syw+(0.0-Sxz*Syy+Sxy*Syz)*Szw+(Szz*Syy-Syz*Syz)*Sxw)*inv);
        if( wa>=191 )
          wa = 191;
        if( wa<-64 )
          wa = -64;
        wa_slot = wa;
        wb = (int32_t)((d1*Syw+d2*Szw+d3*Sxw)*inv);
        if( wb>=191 )
          wb = 191;
        if( wb<-64 )
          wb = -64;
        wc = (int32_t)(inv*((Sxx*Szw-Sxz*Sxw)*Syy+(0.0-Sxx*Syw+Sxw*Sxy)*Syz+(Sxz*Syw-Sxy*Szw)*Sxy));
        if( wc>=191 )
          wc = 191;
        if( wc<-64 )
          wc = -64;
        if( end_px<data_end ) {
          // `x0[0..1]` used to be written here and read back forty lines
          // below -- one 64-bit store, which the decompiler wrote as a pair
          // because MSVC had both halves in registers, and `__PAIR64__` is its
          // name for that.  The two slots are `wb` and `wc` and nothing
          // between the store and the reload writes either.  So the spill is
          // the two names.
          //
          // The loop below spilled three more registers to that same area --
          // `quad`, `g0` and `g1`, each already a local one line up -- and
          // those stores are gone with it.
          uu = n_quads;
          quad = 0;
          do {
            nx0 = img->pixels[4*quad+4];
            dg0 = img->pixels[4*quad]+pp[4*quad+4];
            dn0 = pp[4*quad];
            ad0 = nx0+dn0;
            dnx1 = pp[4*quad+5];
            g0 = dg0-ad0;
            dn1 = pp[4*quad+1];
            nx2 = img->pixels[4*quad+6];
            g1 = img->pixels[4*quad+1]+dnx1-(img->pixels[4*quad+5]+dn1);
            dnx2 = pp[4*quad+6];
            g2 = img->pixels[4*quad+2]+dnx2-(nx2+pp[4*quad+2]);
            g1w = (uint16_t)(img->pixels[4*quad+3]+pp[4*quad+7]-pp[4*quad+3]-img->pixels[4*quad+7]);
            g1_lo = (int16_t)g1;
            g1w = g1w-512;
            pa = g1*wb+g0*wa_slot;
            pb = g2*wc;
            ++frame.hist_a[(g1w-(int16_t)g0)&0x3FF];
            bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
            ++*(uint32_t*)&frame.buf[4*bin_lin];
            ++frame.hist_b[(g1w-g1_lo)&0x3FF];
            g1w = g1w-g2;
            ++frame.hist_c[g1w&0x3FF];
            alpha = pp[4*quad+7]+256;
            bin_lin2 = ((uint16_t)alpha-(uint16_t)((wb*pp[4*quad+5]+wa_slot*pp[4*quad+4]+wc*(uint32_t)pp[4*quad+6]+63)>>7)+256)&0x3FF;
            ++frame.hist_c[bin_lin2+1024];
            r0 = alpha-pp[4*quad+4];
            ++frame.hist_c[r0+2048];
            r1 = alpha-pp[4*quad+5];
            ++frame.hist_c[r1+3072];
            r2 = alpha-pp[4*quad+6];
            ++frame.hist_c[r2+4096];
            quad = quad+1;
          } while( quad<uu );
        }
        cost_lin = estimate_cost((uint8_t*)frame.buf, 1024);
        best4 = (cost_lin>>7)+cost_lin;
        cost_c0 = estimate_cost((uint8_t*)frame.hist_a, 1024);
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
        // `x0[0] = pick0;` before this call and `pred4 = x0[0];` after it was
        // MSVC spilling the register across the call, and `estimate_cost`
        // cannot reach a local of this body.  Same shape twice more below.
        cost_c1 = estimate_cost((uint8_t*)frame.hist_b, 1024);
        pred4 = pick0;
        if( cost_c1<best4 ) {
          best4 = cost_c1;
          pred4 = 2;
          wb = 128;
          wc = 0;
          wa_slot = 0;
        }
        cost_c2 = estimate_cost((uint8_t*)frame.hist_c, 1024);
        pred4b = pred4;
        if( cost_c2<best4 ) {
          pred4b = 3;
          wc = 128;
          wb = 0;
          wa_slot = 0;
        }
        plane_desc[3].weight0 = wa_slot;
        plane_desc[3].weight1 = wb;
        plane_desc[3].weight2 = wc;
        plane_desc[3].src_plane = 3;
        plane_desc[3].nrefs = 3;
        hist2 = &frame.hist_c[1024*pred4b+1024];
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
        plane_desc[3].dc = pos3+1;
      }
    }
  }
  return result;
}
