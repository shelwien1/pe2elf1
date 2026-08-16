// plane_choose.inc -- decide how each plane of an image is to be coded.

// The 256-wide window of a histogram with the largest total, scanned from
// `first` and answering where that window starts -- or `miss` when nothing
// after the first window beats it.  That start, plus one, is the plane's DC
// offset.
//
// Three copies, and the differences between them are three constants: two scan
// a whole 1024-entry row from offset 4 and default to -1, and the third scans
// the first half of a row from offset 0 and defaults to 255.
//
// Two of the three then narrowed the answer through an `int8_t` before adding
// one and the third did not, which cannot show: `dc` is a `uint8_t`, and
// `(int8_t)w + 1` and `w + 1` agree modulo 256.
static int32_t widest_window(const int32_t* hist, int32_t first, int32_t end,
                             int32_t miss) {
  int32_t sum = 0;
  for( int32_t i = 0; i<256; ++i )
    sum += hist[first+i];
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

int32_t choose_plane_coding(BmfImage* img) {
  // Eight 1024-entry histograms in one block, because both searches below
  // pick a row *by index*.  The decompilation had it as `uint8_t buf[32768]`
  // with three of its rows named and the last five under one name carrying a
  // 5120-entry array, and the two searches reached their rows as
  // `&buf[4096*pred]` and `&hist_c[1024*pred4b+1024]` -- rows 0..3 and rows
  // 4..7 of this, spelled in bytes.  Every write went through
  // `*(uint32_t *)&buf[4*bin]`, the byte array being `estimate_cost`'s old
  // parameter type reaching back into the declaration.
  //
  // The four names below are cursors into the one block, which is what that
  // index arithmetic says they are.
  alignas(16) int32_t hists[8*1024];
  int32_t *const hist_flat = hists;          // row 0
  int32_t *const hist_a = hists+1*1024;      // row 1
  int32_t *const hist_b = hists+2*1024;      // row 2
  int32_t *const hist_c = hists+3*1024;      // row 3, and rows 4..7 after it
  // Four 512-entry histograms of `bin` values, one per arm of the weight
  // search.  The first was the frame's `buf_1`, a `uint8_t[2048]` for the same
  // reason, with the first four bytes doubling as a stash for `wt8_dn_end`
  // across a loop that does not touch it.
  // The weight search's two working weights, the two other planes it reads
  // against as offsets from this one, and the row in bytes.  All five were
  // frame slots, because MSVC kept them in registers it had to spill around
  // the `estimate_cost` calls in the middle of each arm.
  int32_t wt4_cur;
  uint32_t wt_slot;
  // The three candidates' costs and their descriptors, as the two tables they
  // are.  This was one `alignas(16) uint8_t scratch[256]` with an
  // `int32_t (&)[4]` and an `int32_t (&)[16]` bound onto it at offsets a probe
  // had measured, because the body reaches every row of both by index and the
  // decompilation had given each row a name -- `acc0`, `acc1`, `d4`, `tbl16`,
  // `tbl64a`, `tbl64b` -- of which two named a region nothing reads through.
  //
  // `cost_candidate` fills one row of each: four costs, and four `PlaneDesc`
  // that become the image's plane order if that candidate wins.  What made the
  // one buffer necessary was that every access spelled its own offset --
  // `acc0[4*xform+2]`, `&tbl16[16*cand]`, `*(uint32_t *)&scratch[16*xform+20]`
  // -- and those are `[xform]` and `[cand]` on a two-dimensional table.
  //
  // Eight bytes of the original block sit before the costs and eight between
  // them and the descriptors; nothing reads either, which is why these can be
  // two arrays.  A round that missed that put `tbl16` eight bytes early over
  // `d4`, and for `xform != 0` the plane order came from whatever followed it
  // -- `testfiles/xform1.bmp` is the image that picks a non-zero candidate and
  // is in the corpus because of it.
  uint32_t cand_cost[3][4];
  PlaneDesc cand_desc[3][4];
  uint32_t uu;
  int32_t wa_slot;
  double dv0, dv1, dv2;
  uint32_t n_quads, next_plane, row, slack;
  uint16_t g1w;
  int32_t result, xform, wt8, wt4, pred, wt4_dn_end, wt8_dn_end, dv3, wa, wb, wc, g0, g1, alpha, wa_pick, pred4, pred4b, wt4_best_dn, wt8_best_dn, wt4_up_end, wt8_up_end, wt4_best, wt8_best;
  uint32_t best_cost, best, quad, best4;
  int32_t n_planes = plane_count;
  int32_t data_size = img->data_size;
  alphabet_reduced = 1;
  int32_t row_stride = (uint16_t)img->stride;
  const uint8_t* data_end = &img->pixels[data_size];
  memset(hists, 0, sizeof hists);
  // The descriptors only; the costs are written before they are read.  The
  // loop this replaces started at byte 192 of the block and stopped before
  // reaching the cost table.
  memset(cand_desc, 0, sizeof cand_desc);
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
    // `next_plane` is unsigned and `result` is not, so naming the value here
    // is what makes the conversion visible; it was always happening.
    if( (uint32_t)n_planes>(uint32_t)result ) {
      plane_desc[next_plane-1].src_plane = result;
      plane_desc[next_plane-1].nrefs = result;
    }
    if( n_planes>=3 ) {
      // The three candidate transforms, costed, cheapest kept.  The
      // decompilation wrote the three calls out, each with the row of the
      // descriptor table it fills spelled as a byte offset; they are one loop
      // over `cand_desc[cand]`.  The cost argument is the *base* of the cost
      // table in all three -- `cost_candidate` indexes it by `4*cand` itself.
      //
      // Ties go to the lower index, which is what `cost1 >= cost0` keeping
      // candidate 0 did.  The original compared the first pair signed and the
      // third candidate unsigned -- `(uint32_t)f(2) < cost1` -- which are the
      // same relation for costs that are estimated bit counts and so never
      // negative.  One relation here.
      xform = 0;
      uint32_t cheapest = cost_candidate(img, 0, cand_desc[0], cand_cost[0]);
      for( int32_t cand = 1; cand<3; ++cand ) {
        const uint32_t c = cost_candidate(img, cand, cand_desc[cand],
                                          cand_cost[0]);
        if( c<cheapest ) {
          cheapest = c;
          xform = cand;
        }
      }
      // The winning candidate's four descriptors, copied in.  This was eight
      // `uint64_t` stores through a `(uint8_t*)&plane_desc[0]+off` accessor,
      // two turns of a loop counting down by eight: offsets 56, 48, 40, 32
      // then 24, 16, 8, 0.  Sixty-four bytes, which is one row of the table
      // and `4*sizeof(PlaneDesc)`, so it is one copy.
      memcpy(plane_desc, cand_desc[xform], sizeof cand_desc[xform]);
      best_cost = cand_cost[xform][0];
      int32_t plane_a = plane_desc[0].src_plane-xform;
      wt8 = plane_desc[xform].weight1;
      int32_t plane_b = plane_desc[1].src_plane-xform;
      wt4 = plane_desc[xform].weight0;
      // Four arms of one search over the two weights, each a full pass over
      // the image costing one candidate pair.  They were written out four
      // times and the first two had already been cleaned; these are the same
      // loop, and the eleven `sN` and six `pN` slots the last two ran on were
      // each a copy of a local that never changes under them.
      {
        wt_slot = wt8;
        wt4_cur = wt4;
        int32_t wt_step = 4;
        int32_t wt4_dn = wt4-1;
        int32_t wt8_dn = wt8-1;
        wt4_dn_end = wt4-wt_step;
        uint8_t *plane0 = &img->pixels[xform];
        wt8_dn_end = wt8-wt_step;
        while( 1 ) {
          if( wt4_dn<=wt4_dn_end ) {
            if( wt8_dn<=wt8_dn_end ) {
              int32_t wt4_up = wt4_cur+1;
              int32_t wt8_up = wt_slot+1;
              wt4_up_end = wt4_cur+wt_step;
              wt8_up_end = wt_slot+wt_step;
              while( 1 ) {
                if( wt4_up>=wt4_up_end&&wt8_up>=wt8_up_end ) {
                  wt8 = wt_slot;
                  wt4 = wt4_cur;
                  goto weights_chosen;
                }
                if( wt4_up<wt4_up_end&&wt4_up<192 ) {
                  uint32_t cost_a = weight_pair_cost(img, plane0, plane_a,
                                      plane_b, wt4_up, wt_slot);
                  wt4_best = wt4_cur;
                  if( cost_a<best_cost ) {
                    best_cost = cost_a;
                    wt4_best = wt4_up;
                  }
                  wt4_cur = wt4_best;
                  wt4_up_end = wt_step+wt4_best;
                }
                if( wt8_up<wt8_up_end&&wt8_up<192 ) {
                  uint32_t cost_b = weight_pair_cost(img, plane0, plane_a,
                                      plane_b, wt4_cur, wt8_up);
                  wt8_best = wt_slot;
                  if( cost_b<best_cost ) {
                    best_cost = cost_b;
                    wt8_best = wt8_up;
                  }
                  wt_slot = wt8_best;
                  wt8_up_end = wt_step+wt8_best;
                }
                ++wt4_up;
                ++wt8_up;
              }
            }
          }
          if( wt4_dn>wt4_dn_end&&wt4_dn>=-64 ) {
            uint32_t cost_c = weight_pair_cost(img, plane0, plane_a,
                                plane_b, wt4_dn, wt_slot);
            wt4_best_dn = wt4_cur;
            if( cost_c<best_cost ) {
              best_cost = cost_c;
              wt4_best_dn = wt4_dn;
            }
            wt4_cur = wt4_best_dn;
            wt4_dn_end = wt4_best_dn-wt_step;
          }
          if( wt8_dn>wt8_dn_end&&wt8_dn>=-64 ) {
            uint32_t cost_d = weight_pair_cost(img, plane0, plane_a,
                                plane_b, wt4_cur, wt8_dn);
            wt8_best_dn = wt_slot;
            if( cost_d<best_cost ) {
              best_cost = cost_d;
              wt8_best_dn = wt8_dn;
            }
            wt_slot = wt8_best_dn;
            wt8_dn_end = wt8_best_dn-wt_step;
          }
          --wt4_dn;
          --wt8_dn;
        }
      }
weights_chosen:
      uint8_t *px = &img->pixels[row_stride+n_planes+xform];
      if( px<data_end ) {
        wt_slot = wt8;
        wt4_cur = wt4;
        do {
          int32_t c2 = px[plane_b];
          int32_t c2w = c2*wt_slot;
          int32_t c1 = px[plane_a];
          int32_t c1w = c1*wt4_cur;
          ++hist_c[c2-c1+1280];
          int32_t c0 = *px+512;
          px += n_planes;
          int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
          ++hist_flat[bin0];
          ++hist_a[c0-c1];
          ++hist_b[c0-c2];
          ++hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
        } while( px<data_end );
        wt8 = wt_slot;
        wt4 = wt4_cur;
      }
      slack = best_cost>>7;
      if( slack>=0x4000 )
        slack = 0x4000;
      best = slack+best_cost;
      // The winning candidate's other three costs, against the fitted pair:
      // flat, plane-b only, and the two averaged.  Each was its own byte
      // offset into the block and they are one row of four.
      uint32_t cost_flat = cand_cost[xform][1];
      bool cheaper = cost_flat<best;
      if( cost_flat<best ) {
        best = cost_flat;
        wt4 = 128;
        wt8 = 0;
      }
      pred = cheaper;
      if( cand_cost[xform][2]<best ) {
        best = cand_cost[xform][2];
        pred = 2;
        wt4 = 0;
        wt8 = 128;
      }
      bool keep3 = best<=cand_cost[xform][3];
      if( best>cand_cost[xform][3] )
        pred = 3;
      if( !keep3 ) {
        wt4 = 64;
        wt8 = 64;
      }
      plane_desc[xform].weight0 = wt4;
      plane_desc[xform].weight1 = wt8;
      // The widest 256-entry window of one histogram row, and `pos` is its
      // right edge -- truncated to `int8_t`, which is what makes `dc` a byte.
      //
      // `win` started at `(uintptr_t)hist & 0xF`, the low four bits of a stack
      // address: the frame was `alignas(16)` and `buf` sat four bytes into it
      // behind a `_pad0[4]`, so the answer was always 4 and the window always
      // began at entry 4 rather than 0.  Reading the compiler's stack layout
      // as an array index is not something any gate here can see -- all
      // nineteen images produce byte-identical streams at 0 and at 4, checked
      // -- so the measured value is what stays, with the 4 written down.
      plane_desc[xform].dc = widest_window(hists+1024*pred, 4, 1024, -1)+1;
      // The same window over row 4, which is `hist_c[1024..2047]`.
      result = widest_window(hist_c+1024, 0, 512, 255)+1;
      plane_desc[plane_b+xform].dc = result;
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
        // frame's unions -- `q0` and `q1` overlaid `row_stride`, `plane_b`,
        // `plane_a` and `nplanes_s`, and were safe only because those four are
        // last read twenty lines above.  Nine named doubles say the same
        // arithmetic and take the round trip out, and the two unions that lent
        // the slots had no reader left.
        double Sxx = 0.0, Sxy = 0.0, Sxz = 0.0;
        double Syy = 0.0, Syz = 0.0, Szz = 0.0;
        double Sxw = 0.0, Syw = 0.0, Szw = 0.0;
        memset(hists, 0, sizeof hists);
        uint8_t *pp = &img->pixels[row_stride];
        n_quads = (data_end-1-pp)/4;
        uint8_t *end_px = &img->pixels[row_stride+4];
        if( end_px<data_end ) {
          uint32_t i4 = 0;
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
        double d1 = Sxx*Szz-Sxz*Sxz;
        double d2 = 0.0-Sxx*Syz+Sxz*Sxy;
        double d3 = Sxz*Syz-Sxy*Szz;
        double inv = 128.0/(Syy*d1+Syz*d2+Sxy*d3+0.1);
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
            int32_t nx0 = img->pixels[4*quad+4];
            int32_t dg0 = img->pixels[4*quad]+pp[4*quad+4];
            int32_t dn0 = pp[4*quad];
            int32_t ad0 = nx0+dn0;
            int32_t dnx1 = pp[4*quad+5];
            g0 = dg0-ad0;
            int32_t dn1 = pp[4*quad+1];
            int32_t nx2 = img->pixels[4*quad+6];
            g1 = img->pixels[4*quad+1]+dnx1-(img->pixels[4*quad+5]+dn1);
            int32_t dnx2 = pp[4*quad+6];
            int32_t g2 = img->pixels[4*quad+2]+dnx2-(nx2+pp[4*quad+2]);
            g1w = (uint16_t)(img->pixels[4*quad+3]+pp[4*quad+7]-pp[4*quad+3]-img->pixels[4*quad+7]);
            int16_t g1_lo = (int16_t)g1;
            g1w = g1w-512;
            int32_t pa = g1*wb+g0*wa_slot;
            int32_t pb = g2*wc;
            ++hist_a[(g1w-(int16_t)g0)&0x3FF];
            int32_t bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
            ++hist_flat[bin_lin];
            ++hist_b[(g1w-g1_lo)&0x3FF];
            g1w = g1w-g2;
            ++hist_c[g1w&0x3FF];
            alpha = pp[4*quad+7]+256;
            int32_t bin_lin2 = ((uint16_t)alpha-(uint16_t)((wb*pp[4*quad+5]+wa_slot*pp[4*quad+4]+wc*(uint32_t)pp[4*quad+6]+63)>>7)+256)&0x3FF;
            ++hist_c[bin_lin2+1024];
            int32_t r0 = alpha-pp[4*quad+4];
            ++hist_c[r0+2048];
            int32_t r1 = alpha-pp[4*quad+5];
            ++hist_c[r1+3072];
            int32_t r2 = alpha-pp[4*quad+6];
            ++hist_c[r2+4096];
            quad = quad+1;
          } while( quad<uu );
        }
        uint32_t cost_lin = estimate_cost(hist_flat, 1024);
        best4 = (cost_lin>>7)+cost_lin;
        uint32_t cost_c0 = estimate_cost(hist_a, 1024);
        bool pick0 = cost_c0<best4;
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
        uint32_t cost_c1 = estimate_cost(hist_b, 1024);
        pred4 = pick0;
        if( cost_c1<best4 ) {
          best4 = cost_c1;
          pred4 = 2;
          wb = 128;
          wc = 0;
          wa_slot = 0;
        }
        uint32_t cost_c2 = estimate_cost(hist_c, 1024);
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
        // Rows 4..7, by the same window and the same layout-read constant
        // as the search above.
        plane_desc[3].dc = widest_window(hists+1024*(4+pred4b), 4, 1024, -1)+1;
      }
    }
  }
  return result;
}
