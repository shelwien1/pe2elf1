// codec.inc -- BMFCodec: the plane pipeline, the mode search and the container layer
// Included from bmf.cpp; not a standalone translation unit.

struct BMFCodec : BMFState {
  void Init() {
    BMFState::Init();
    p1_blocks.Init();
    p2_blocks.Init();
    model_blocks.Init();
  }

  struct WeightSearch {
    BMFCodec* cx;
    const BmfImage* img;
    const uint8_t* plane0;
    int32_t plane_a, plane_b;
    int32_t w[2];
    uint32_t cost;
    int32_t trial(int32_t axis, int32_t cand, int32_t step) {
      int32_t pair[2] = {w[0], w[1]};
      pair[axis] = cand;
      const uint32_t c = cx[0].weight_pair_cost(img, plane0, plane_a, plane_b, pair[0], pair[1]);
      if( c<cost ) {
        cost = c;
        w[axis] = cand;
      }
      return w[axis]+step;
    }

    void descend(int32_t step, int32_t dir) {
      int32_t at[2] = {w[0]+dir, w[1]+dir};
      int32_t end[2] = {w[0]+dir*step, w[1]+dir*step};
      while( dir*at[0]<dir*end[0]||dir*at[1]<dir*end[1] ) {
        for( int32_t k = 0; k<2; ++k )
          if( dir*at[k]<dir*end[k]&&(dir<0 ? at[k]>=kWeightMin : at[k]<kWeightMax+1) )
            end[k] = trial(k, at[k], dir*step);
        at[0] += dir;
        at[1] += dir;
      }
    }
  };

  BlockPool<AltP1Block, 4> p1_blocks;
  BlockPool<AltP2Block, 4> p2_blocks;
  BlockPool<ModelBlock, 1> model_blocks;

  template <class T, int32_t N> int32_t planes_free(BlockPool<T, N> &pool, T** plane) {
    for( int32_t k = 0; k<plane_count; ++k ) pool.give(plane[k]);
    return plane_count;
  }

  void allow_refs_where_present() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags |= plane_desc[k].nrefs!=0 ? desc_has_refs : 0;
  }
  template <int32_t f_DEC> int32_t alt_model_p1(BmfImage* hdr, uint8_t* buf) {
    AltP1Block* plane[4];
    int32_t cur0 = 0, cur1 = 0, cur3 = 0;
    int32_t want2 = 0;
    int32_t width = hdr[0].width;
    int32_t height = hdr[0].height;
    for( int32_t k = 0; k<plane_count; ++k ) {
      AltP1Block* raw = p1_blocks.take(false);
      plane[k] = raw[0].alt_p1_alloc(this, width, height, k);
    }
    int32_t src1 = plane_desc[1].src_plane;
    int32_t src2 = plane_desc[2].src_plane;
    int32_t src3 = plane_desc[3].src_plane;
    uint8_t fl1 = plane_desc[src1].flags;
    uint8_t fl2 = plane_desc[src2].flags;
    uint8_t fl3 = plane_desc[src3].flags;
    int32_t dc2 = plane_desc[src2].dc;
    int32_t dc1 = plane_desc[src1].dc;
    int32_t xf1 = fl1&8;
    int8_t dc3 = plane_desc[src3].dc;
    int32_t xf2 = fl2&8;
    int32_t xf3 = fl3&8;
    if constexpr( f_DEC )
      rc_begin_decode();
    else
      rc_begin_encode();
    for( uint32_t y = 0; y<(uint32_t)height; ++y ) {
      for( int32_t p = 0; p<plane_count; ++p ) {
        plane[p][0].advance_row();
        plane[p][0].seed_activity(true);
      }
      {
        auto code_sample = [&](int32_t n, AltP1Block* a, AltP1Block* b, int32_t off, int32_t val) {
                             AltP1Block*const blk = plane[n];
                             blk[0].ctx_of(a, b);
                             val = f_DEC ? blk[0].decode_sample() : blk[0].encode_sample(buf, off, val);
                             blk[0].record_sample(val);
                             return val;
                           };
        auto fold_from = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix) {
                           const int32_t v = buf[off];
                           return xf ? v-dc-mix : v;
                         };
        auto unfold_to = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix, int32_t val) {
                           if( xf )
                             val += dc+mix;
                           buf[off] = val;
                         };
        for( uint32_t x = 0; x<(uint32_t)width; ++x ) {
          int32_t off0 = plane_desc[0].src_plane;
          if constexpr( !f_DEC )
            cur0 = fold_from(off0, 0, 0, 0);
          cur0 = code_sample(0, nullptr, nullptr, off0, cur0);
          int32_t off1 = plane_desc[1].src_plane;
          if constexpr( f_DEC )
            unfold_to(off0, 0, 0, 0, cur0);
          else
            cur1 = (uint8_t)fold_from(off1, xf1, dc1, buf[off0]);
          cur1 = code_sample(1, plane[0], nullptr, off1, cur1);
          int32_t off2 = plane_desc[2].src_plane;
          if constexpr( f_DEC )
            unfold_to(off1, xf1, dc1, buf[off0], cur1);
          else
            want2 = (uint8_t)fold_from(off2, xf2, dc2, plane_mix2(plane_desc[off2], buf+off0, buf+off1));
          want2 = code_sample(2, plane[1], plane[0], off2, want2);
          if constexpr( f_DEC )
            unfold_to(off2, xf2, dc2, plane_mix2(plane_desc[off2], buf+off0, buf+off1), want2);
          if( plane_count>=4 ) {
            int32_t off3 = plane_desc[3].src_plane;
            if constexpr( !f_DEC )
              cur3 = (uint8_t)fold_from(off3, xf3, dc3, plane_mix3(plane_desc[off3], buf+off3));
            cur3 = code_sample(3, plane[2], plane[1], off3, cur3);
            if constexpr( f_DEC )
              unfold_to(off3, xf3, dc3, plane_mix3(plane_desc[off3], buf+off3), cur3);
          }
          buf += plane_count;
        }
      }
    }
    if constexpr( f_DEC )
      rc_end_decode();
    else
      rc_end_encode();
    return planes_free(p1_blocks, plane);
  }
  int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out) {
    return alt_model_p1<1>(hdr, out);
  }
  int32_t alt_model_p1_encode(BmfImage* hdr, uint8_t* src) {
    return alt_model_p1<0>(hdr, src);
  }
  template <int32_t f_DEC> int32_t alt_model_p2(BmfImage* p_i, uint8_t* out) {
    AltP2Block* plane[4];
    P2Coef coef_session;
    coef_session.fold(this);
    const int32_t width = p_i[0].width;
    const int32_t height = p_i[0].height;
    alt_p2_planes_alloc(plane, width);
    const bool ref1_mixed = ref_transformed(1);
    const bool ref2_mixed = ref_transformed(2);
    const bool ref3_mixed = ref_transformed(3);
    if constexpr( f_DEC )
      rc_begin_decode();
    else
      rc_begin_encode();
    int32_t nplanes = plane_count;
    for( int32_t y = 0; y<height; ++y ) {
      if( nplanes>0 ) {
        alt_p2_start_row(plane, y, width);
        nplanes = plane_count;
      }
      for( int32_t j = 0; j<4; ++j )
        ctx_bias[j] = 0;
      for( int32_t x = 0; x<width; ++x ) {
        for( int32_t j = 0; j<4; ++j )
          ctx_bias[j] >>= 3;
        plane[0][0].code_sample<f_DEC>(out, plane_desc[0].src_plane, plane[2], plane[1]);
        plane[0][0].add_bias(ctx_bias);
        int16_t seed1 = 0;
        if( ref1_mixed )
          seed1 = 16*out[plane_desc[0].src_plane];
        plane[1][0].cursor[0][0].dval = seed1;
        plane[1][0].code_sample<f_DEC>(out, plane_desc[1].src_plane, plane[0], plane[2]);
        plane[1][0].add_bias(ctx_bias);
        int16_t seed2 = 0;
        if( ref2_mixed ) {
          const PlaneDesc &mix = plane_desc[plane_desc[2].src_plane];
          seed2 = (mix.weight1*out[plane_desc[1].src_plane]+mix.weight0*out[plane_desc[0].src_plane])>>3;
        }
        plane[2][0].cursor[0][0].dval = seed2;
        plane[2][0].code_sample<f_DEC>(out, plane_desc[2].src_plane, plane[0], plane[1]);
        plane[2][0].add_bias(ctx_bias);
        nplanes = plane_count;
        if( plane_count>=4 ) {
          int16_t seed3 = 0;
          if( ref3_mixed ) {
            const PlaneDesc &mix = plane_desc[plane_desc[3].src_plane];
            seed3 = (mix.weight2*out[2]+mix.weight1*out[1]+mix.weight0*out[0])>>3;
          }
          plane[3][0].cursor[0][0].dval = seed3;
          plane[3][0].code_sample<f_DEC>(out, f_DEC ? 3 : plane_desc[3].src_plane, plane[2], plane[0]);
          plane[3][0].add_bias(ctx_bias);
          nplanes = plane_count;
        }
        out += nplanes;
      }
    }
    if constexpr( f_DEC )
      rc_end_decode();
    else
      rc_end_encode();
    coef_session.restore(this);
    return planes_free(p2_blocks, plane);
  }
  template <int32_t f_DEC> void alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    AltP2Block* blk;
    AltP2Block* raw = p2_blocks.take(true);
    if( raw )
      blk = raw[0].alt_p2_alloc(this, i, 0);
    else
      blk = nullptr;
    if constexpr( f_DEC )
      blk[0].alt_p2_d8_body<1>(nullptr, out, i, height);
    else
      blk[0].alt_p2_d8_body<0>(src, out, i, height);
    if( blk )
      p2_blocks.give(blk);
  }
  void alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
    alt_model_p2_d8<1>(nullptr, i, height, out);
  }
  void alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    alt_model_p2_d8<0>(src, i, height, out);
  }
  int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
    return alt_model_p2<1>(p_i, out);
  }
  int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
    return alt_model_p2<0>(p_i, a2);
  }
  template <int32_t f_DEC> bool alt_model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    if( !plane_alt_model )
      return false;
    const bool p1 = plane_predictor==pred_p1;
    if( plane_predictor==pred_ctx ) {
      CtxModel ctx_model;
      ctx_model.Init();
      if constexpr( !f_DEC ) {
        ctx_model.set_alphabet(CtxModel::scan_alphabet(pixels, p_i[0].width*p_i[0].height, ctx_model.to_val));
        ctx_model.choose_neighbours(this, pixels, p_i[0].width, p_i[0].height);
      }
      ctx_model.template code_plane<f_DEC>(this, pixels, p_i[0].width, p_i[0].height);
      return true;
    }
    if( !p1&&plane_predictor!=pred_p2 )
      return true;
    const bool d8 = (p_i[0].depth&depth_bits)==8;
    if( d8 ) {
      if constexpr( f_DEC ) {
        if( p1 )
          alt_model_p1_d8_decode(pixels, p_i[0].width, p_i[0].height);
        else
          alt_model_p2_d8_decode(pixels, p_i[0].width, p_i[0].height);
      } else {
        if( p1 )
          alt_model_p1_d8_encode(pixels, p_i[0].width, p_i[0].height, raw);
        else
          alt_model_p2_d8_encode(pixels, p_i[0].width, p_i[0].height, raw);
      }
    } else if constexpr( f_DEC ) {
      if( p1 )
        alt_model_p1_decode(p_i, pixels);
      else
        alt_model_p2_decode(p_i, pixels);
    } else {
      if( p1 )
        alt_model_p1_encode(p_i, pixels);
      else
        alt_model_p2_encode(p_i, pixels);
    }
    return true;
  }
  AltP1Block*alt_p1_block_alloc(int32_t width, int32_t height) {
    AltP1Block* raw = p1_blocks.take(false);
    return raw ? raw[0].alt_p1_alloc(this, width, height, 0) : nullptr;
  }
  // Returns the cheapest whole-image estimate it saw, in the order-0
  // gradient-domain currency of cost_candidate -- zero when there are too few
  // planes for that estimate to exist.  It is a poor way to rank two coding
  // modes, but a fine way to gate an expensive trial.
  uint32_t choose_plane_coding(BmfImage* img) {
    alignas(16) int32_t hists[8*1024];
    const HistRows hr = hist_rows(hists);
    int32_t*const hist_flat = hr.r0, *const hist_a = hr.r1;
    int32_t*const hist_b = hr.r2, *const hist_c = hr.r3;
    uint32_t cand_cost[3][4];
    PlaneDesc cand_desc[3][4];
    uint32_t next_plane, row;
    int32_t result, xform, wt8, wt4, pred;
    uint32_t best_cost, best;
    uint32_t cheapest_est = 0;
    int32_t n_planes = plane_count;
    int32_t data_size = img[0].data_size;
    alphabet_reduced = 1;
    int32_t row_stride = img[0].stride;
    const uint8_t* data_end = &img[0].pixels[data_size];
    memset(hists, 0, sizeof hists);
    memset(cand_desc, 0, sizeof cand_desc);
    if( n_planes>0 ) {
      if( n_planes/2 ) {
        uint32_t pair = 0;
        for(; pair<(uint32_t)(n_planes/2); ++pair ) {
          const uint8_t pl_even = 2*pair, pl_odd = 2*pair+1;
          row = 2*pair;
          plane_desc[row].src_plane = pl_even;
          plane_desc[row].nrefs = pl_even;
          plane_desc[row+1].src_plane = pl_odd;
          plane_desc[row+1].nrefs = pl_odd;
        }
        next_plane = 2*pair+1;
      } else {
        next_plane = 1;
      }
      result = next_plane-1;
      if( (uint32_t)n_planes>(uint32_t)result ) {
        plane_desc[next_plane-1].src_plane = result;
        plane_desc[next_plane-1].nrefs = result;
      }
      if( n_planes<3 ) {
        bmf_log("[choose_plane_coding] %d plane%s: no inter-plane blend to search\n",
                n_planes, n_planes==1 ? "" : "s");
      }
      if( n_planes>=3 ) {
        uint32_t cand_total[3];
        xform = 0;
        uint32_t cheapest = cost_candidate(img, 0, cand_desc[0], cand_cost[0]);
        cand_total[0] = cheapest;
        for( int32_t cand = 1; cand<3; ++cand ) {
          const uint32_t c = cost_candidate(img, cand, cand_desc[cand], cand_cost[0]);
          cand_total[cand] = c;
          if( c<cheapest ) {
            cheapest = c;
            xform = cand;
          }
        }
        bmf_log("[choose_plane_coding] %d planes; order-0 entropy of gradient-domain residuals\n", n_planes);
        bmf_log("  which plane is predicted from the two others:\n");
        for( int32_t cand = 0; cand<3; ++cand )
          bmf_log("    plane %d     %12.1f bytes%s\n", cand, bmf_bytes(cand_total[cand]), cand==xform ? "  <<" : "");
        cheapest_est = cheapest;
        memcpy(plane_desc, cand_desc[xform], sizeof cand_desc[xform]);
        best_cost = cand_cost[xform][0];
        const uint32_t cost_fit = best_cost;
        int32_t plane_a = plane_desc[0].src_plane-xform;
        wt8 = plane_desc[xform].weight1;
        int32_t plane_b = plane_desc[1].src_plane-xform;
        wt4 = plane_desc[xform].weight0;
        const int32_t fit4 = wt4, fit8 = wt8;
        {
          const uint8_t* plane0 = &img[0].pixels[xform];
          // Seed the descent from the best of the fit and the three degenerate
          // forms.  The closed-form fit can land on (0,0) -- it does on t24 --
          // and a +-4 coordinate descent cannot walk out of that, so the search
          // would otherwise start from a predictor that predicts nothing and
          // the only thing saving the image would be the slack rule below.
          // The degenerate costs come from cost_candidate's own histograms, so
          // a chosen seed is re-priced in the currency the descent minimises.
          const int32_t degenerate[3][2] = {{kWeightScale, 0}, {0, kWeightScale},
                                            {kWeightScale/2, kWeightScale/2}};
          uint32_t seed_cost = cand_cost[xform][0];
          int32_t seed = -1;
          for( int32_t k = 0; k<3; ++k )
            if( cand_cost[xform][k+1]<seed_cost ) {
              seed_cost = cand_cost[xform][k+1];
              seed = k;
            }
          if( seed>=0 ) {
            wt4 = degenerate[seed][0];
            wt8 = degenerate[seed][1];
            best_cost = weight_pair_cost(img, plane0, plane_a, plane_b, wt4, wt8);
          }
          WeightSearch search = {this, img, plane0, plane_a, plane_b, {wt4, wt8}, best_cost};
          int32_t wt_step = 4;
          search.descend(wt_step, -1);
          search.descend(wt_step, +1);
          wt8 = search.w[1];
          wt4 = search.w[0];
          best_cost = search.cost;
        }
        const int32_t des4 = wt4, des8 = wt8;
        uint8_t* px = &img[0].pixels[row_stride+n_planes+xform];
        if( px<data_end ) {
          do {
            int32_t c2 = px[plane_b];
            int32_t c2w = c2*wt8;
            int32_t c1 = px[plane_a];
            int32_t c1w = c1*wt4;
            ++hist_c[c2-c1+1280];
            int32_t c0 = px[0]+512;
            px += n_planes;
            int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+CD_weight_round)>>kWeightShift))&0x3FF;
            ++hist_flat[bin0];
            ++hist_a[c0-c1];
            ++hist_b[c0-c2];
            ++hist_c[(c0-((uint32_t)((((c1+c2)*kWeightScale)>>1)+CD_weight_round)>>kWeightShift))];
          } while( px<data_end );
        }
        const uint32_t slack = umin32(best_cost>>idx_clamp(GL_slack_shift, 1, 24), kSlackMax);
        best = slack+best_cost;
        uint32_t cost_flat = cand_cost[xform][1];
        bool cheaper = cost_flat<best;
        if( cost_flat<best ) {
          best = cost_flat;
          wt4 = kWeightScale;
          wt8 = 0;
        }
        pred = cheaper;
        if( cand_cost[xform][2]<best ) {
          best = cand_cost[xform][2];
          pred = 2;
          wt4 = 0;
          wt8 = kWeightScale;
        }
        bool keep3 = best<=cand_cost[xform][3];
        if( best>cand_cost[xform][3] )
          pred = 3;
        if( !keep3 ) {
          wt4 = kWeightScale/2;
          wt8 = kWeightScale/2;
        }
        if( bmf_verbose ) {
          static const char*const blend[4] = {
            "least-squares fit", "copy first ref", "copy second ref", "average 64:64",
          };
          const uint32_t shown[4] = {best_cost, cand_cost[xform][1], cand_cost[xform][2], cand_cost[xform][3]};
          bmf_log("  plane %d predicted from plane %d (coded flat) and plane %d (coded as a difference):\n",
                  xform, plane_desc[0].src_plane, plane_desc[1].src_plane);
          for( int32_t k = 0; k<4; ++k )
            bmf_log("    %-18s %12.1f bytes%s\n", blend[k], bmf_bytes(shown[k]), k==pred ? "  <<" : "");
          bmf_log("    weights (%d,%d) fitted -> (%d,%d) after coordinate descent, %.1f -> %.1f bytes;"
                  " a degenerate form wins if it comes within %.1f bytes of that\n",
                  fit4, fit8, des4, des8, bmf_bytes(cost_fit), bmf_bytes(best_cost), bmf_bytes(slack));
        }
        plane_desc[xform].weight0 = wt4;
        plane_desc[xform].weight1 = wt8;
        plane_desc[xform].dc = widest_window(hists+1024*pred, CD_dc_window_lo, CD_dc_window_hi, -1)+1;
        result = widest_window(hist_c+1024, CD_dc_diff_lo, CD_dc_diff_hi, 255)+1;
        plane_desc[plane_b+xform].dc = result;
        if( n_planes>=4 ) {
          choose_alpha_plane(img, hists, data_end, row_stride);
        }
      }
    }
    return cheapest_est;
  }
  void clear_flags_on_all() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags = 0;
  }
  template <int32_t f_DEC> void code_colour_plane(BmfImage* img, uint8_t* side, int32_t plane) {
    uint8_t* base = &img[0].pixels[plane];
    if( !(plane_desc[plane].flags&desc_has_refs) ) {
      if constexpr( f_DEC )
        interleave_flat(img, side, plane);
      else
        deinterleave_plane(img, side, plane);
      return;
    }
    int32_t stride = plane_count;
    int32_t n = bmf_pixels(img);
    const PlaneTransform t = plane_transform(plane);
    if( t.by_weights||plane_desc[plane].nrefs==1 ) {
      const uint8_t* ref = base+t.to_ref0;
      for( int32_t i = 0, ofs = 0; i<n; ++i, ofs += stride ) {
        if constexpr( f_DEC )
          base[ofs] = ref[ofs]+t.dc+side[i];
        else
          side[i] = base[ofs]-t.dc-ref[ofs];
      }
      return;
    }
    if( t.mode==2||t.mode==3 ) {
      uint8_t* p = base;
      for( int32_t left = n; left; --left, p += stride ) {
        const uint32_t blend = t.blend(p);
        if constexpr( f_DEC )
          *p = (uint8_t)(blend+t.dc+*side++);
        else
          *side++ = (uint8_t)(p[0]-t.dc-blend);
      }
    }
  }
  template <int32_t f_DEC, typename T> void code_field(T &v, int32_t bits, int32_t bias) {
    if constexpr( f_DEC )
      v = (T)((int32_t)unpack_bits(bits)-bias);
    else
      pack_bits((uint32_t)((int32_t)v+bias), bits);
  }
  // One slow-model pass over the packed data, leaving the result in `stream`.
  int32_t code_packed_once(BmfImage* p_i) {
    stream_open(p_i[0].data_size);
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    model_plane(p_i, p_i[0].pixels, p_i[0].pixels);
    packer_flush();
    return (int32_t)(stream.cur-stream.buf);
  }

  // One pass over the packed data with the direct context model, which wants a
  // byte per pixel.  The plane goes out unpacked and comes back packed, so the
  // representation is invisible outside this function and the flag that records
  // it.
  int32_t code_packed_ctx_once(BmfImage* p_i) {
    const int32_t w = p_i[0].width, h = p_i[0].height;
    uint8_t*const flat = (uint8_t*)bmf_new((size_t)w*h);
    unpack_rows(p_i, flat);
    stream_open(p_i[0].data_size);
    plane_predictor = pred_ctx;
    plane_alt_model = 1;
    alphabet_reduced = 0;
    BmfImage hdr;
    hdr.header_from(p_i);
    hdr.flags = 0;
    hdr.depth = depth_one_plane;
    hdr.stride = (uint16_t)w;
    hdr.data_size = (uint32_t)w*(uint32_t)h;
    model_plane(&hdr, flat, flat);
    packer_flush();
    free(flat);
    return (int32_t)(stream.cur-stream.buf);
  }

  // The short path's candidate generators, in the order the long path applies
  // its own: geometry first, palette order last, so the decoder undoes them in
  // one reverse sweep for both paths.
  void apply_packed_op(BmfImage* p_i, int32_t op, const PaletteOrder &po) {
    if( op==0 )
      transpose_image(p_i, 1);
    else if( op==1 )
      flip_image_y(p_i, 1);
    else if( op==2 )
      flip_image_x(p_i, 1);
    else
      permute_indices(p_i, po.rank);
  }

  // Images of <=4 bpp never reach search_filter, so they never get the trials
  // the long path has -- and orientation and palette order are worth more on
  // this data than anywhere else in the codec.  Greedily offer each generator
  // on top of what has been accepted so far, then re-code the winner.  A
  // rejected trial is undone from a snapshot rather than by re-applying the
  // generator: transposing a packed image zeroes the row padding, so it is not
  // quite an involution at the byte level.
  void code_packed_body(BmfImage* p_i, BmfImage &hdr) {
    static const char*const op_name[4] = {
      "transposed", "flipped vertically", "flipped horizontally", "palette in luma order",
    };
    PaletteOrder po;
    const bool has_order = palette_order_of(p_i, &po);
    const bool bits_ok = packed_bits_ok(p_i);
    int32_t best_len = code_packed_once(p_i);
    free(stream.buf);
    bmf_log("[short path] %d bpp, whole-image trials:\n", p_i[0].depth&depth_bits);
    bmf_log("    %-22s %12.1f bytes\n", "as read", (double)best_len);
    uint8_t*const snap = (uint8_t*)bmf_new(p_i[0].data_size);
    for( int32_t op = 0; op<4; ++op ) {
      if( op==0 ? !packed_transpose_ok(p_i) : op==3 ? !has_order : !bits_ok )
        continue;
      const BmfImage keep_hdr = p_i[0];
      memcpy(snap, p_i[0].pixels, p_i[0].data_size);
      apply_packed_op(p_i, op, po);
      const int32_t len = code_packed_once(p_i);
      free(stream.buf);
      const bool win = len<best_len;
      bmf_log("    %-22s %12.1f bytes%s\n", op_name[op], (double)len, win ? "  <<" : "");
      if( win ) {
        best_len = len;
      } else {
        p_i[0] = keep_hdr;
        memcpy(p_i[0].pixels, snap, p_i[0].data_size);
      }
    }
    free(snap);
    // The direct context model, on the plane unpacked to a byte per pixel.  A
    // small alphabet is exactly what 1, 2 and 4 bpp data is.
    if( packed_rows_exact(p_i) ) {
      const int32_t ctx_len = code_packed_ctx_once(p_i);
      free(stream.buf);
      const bool win = ctx_len<best_len;
      bmf_log("    %-22s %12.1f bytes%s\n", "ctx model, unpacked", (double)ctx_len, win ? "  <<" : "");
      if( win ) {
        code_packed_ctx_once(p_i);
        p_i[0]._pad8[0] |= pad_ctx_packed;
        hdr.header_from(p_i);
        hdr.flags = flags_coded|flags_slow|p_i[0].flags;
        return;
      }
    }
    code_packed_once(p_i);
    hdr.header_from(p_i);
    hdr.flags = flags_coded|flags_slow|p_i[0].flags;
  }

  void code_image_body(BmfImage* p_i, BmfImage &hdr) {
    if( (p_i[0].depth&depth_bits)<=4 ) {
      code_packed_body(p_i, hdr);
      return;
    }
    const uint32_t filtered = search_filter(p_i);
    hdr.flags |= flags_descriptors;
    const uint32_t data_bytes = p_i[0].data_size;
    // search_filter may have transposed the image or reordered its palette, and
    // both show up as flags on p_i; take the geometry and the flags from it.
    hdr.header_from(p_i);
    hdr.flags = flags_coded|flags_descriptors|flags_slow|p_i[0].flags;
    stream_open(data_bytes);
    if( plane_count==1 ? (p_i[0].depth&depth_grey)!=0 : plane_count>2 ) {
      near_lossless_q = 0;
      pack_bits(0, 4);
    }
    alphabet_reduced = 0;
    write_plane_descs();
    if( filtered ) {
      hdr.flags |= flags_planar;
      uint8_t* plane_buf = (uint8_t*)bmf_new(p_i[0].width*p_i[0].height);
      for( int32_t k = 0; k<plane_count; ++k )
        model_planes(p_i, plane_buf, plane_desc[k].src_plane);
      free(plane_buf);
    } else {
      transform_planes(p_i);
    }
  }
  template <int32_t f_DEC> void code_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    if( alt_model_plane<f_DEC>(p_i, pixels, raw) )
      return;
    ModelBlock* blk = new_model_block(p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits);
    if constexpr( f_DEC )
      blk[0].unmodel_plane_slow(pixels);
    else
      blk[0].model_plane_slow(pixels);
    blk[0].free_workspace();
    model_blocks.give(blk);
  }
  template <int32_t f_DEC> void code_plane_descs() {
    for( int32_t pl = 0; pl<plane_count; ++pl ) {
      PlaneDesc &d = plane_desc[pl];
      uint32_t head = (uint32_t)((d.flags<<2)|d.nrefs);
      code_field<f_DEC>(head, 6, 0);
      if constexpr( f_DEC ) {
        d.flags = (uint8_t)(head>>2);
        d.nrefs = (uint8_t)(head&3);
        plane_desc[head&3].src_plane = (uint8_t)pl;
      }
      if( !(d.flags&desc_has_refs) )
        continue;
      code_field<f_DEC>(d.dc, 8, 0);
      if( d.nrefs<=1u )
        continue;
      // The wire field the fitted cross-plane weights travel in.  Format, not
      // tuning: a wider field only wastes header bits, and a narrower one
      // truncates the weight and shortens the header, so the decoder reads
      // every later field from the wrong offset.  The neighbouring 6 and 8
      // above are literals for the same reason.
      const int32_t kWeightBits = 8, kWeightBias = 64;
      code_field<f_DEC>(d.weight0, kWeightBits, kWeightBias);
      code_field<f_DEC>(d.weight1, kWeightBits, kWeightBias);
      if( d.nrefs>2u )
        code_field<f_DEC>(d.weight2, kWeightBits, kWeightBias);
    }
  }
  void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane) {
    code_colour_plane<0>(img, dst, plane);
  }
  // Writes the colour table after the coded body.  Small palettes go out
  // verbatim as they always did; larger ones get their own range-coded segment
  // behind a two-byte length, with kPaletteRaw naming the verbatim fallback so
  // an incompressible table costs two bytes rather than the coder's framing.
  void write_palette(BmfStream* io, const uint8_t* pal, uint32_t pal_bytes, uint8_t depth) {
    if( !palette_is_coded(depth) ) {
      io[0].write_all(pal, pal_bytes);
      return;
    }
    uint8_t*const copy = (uint8_t*)bmf_new(pal_bytes);
    memcpy(copy, pal, pal_bytes);
    stream.size = (int32_t)pal_bytes+1024;
    stream.buf = (uint8_t*)bmf_new(stream.size);
    stream.cur = stream.buf;
    enc_init();
    PaletteModel model;
    model.code<0>(this, copy, (int32_t)(pal_bytes/3));
    flush();
    const uint32_t coded = (uint32_t)(stream.cur-stream.buf);
    const bool win = coded<pal_bytes;
    bmf_log("  palette %u bytes -> %u coded  %s\n", pal_bytes, coded,
            win ? "<<" : "(shipping the table raw)");
    const uint16_t len = (uint16_t)(win ? coded : kPaletteRaw);
    io[0].write_all(&len, 2u);
    io[0].write_all(win ? stream.buf : pal, win ? coded : pal_bytes);
    free(stream.buf);
    stream.buf = nullptr;
    free(copy);
  }
  bool read_palette(BmfStream* io, uint8_t* pal, uint32_t pal_bytes, uint8_t depth) {
    if( !palette_is_coded(depth) )
      return io[0].read(pal, pal_bytes)==pal_bytes;
    uint16_t len = 0;
    if( !io[0].read_exact(&len, 2u) )
      return false;
    if( len==kPaletteRaw )
      return io[0].read(pal, pal_bytes)==pal_bytes;
    if( len<2u||len>pal_bytes )
      return false;
    stream.size = len;
    stream.buf = (uint8_t*)bmf_new(len);
    stream.cur = stream.buf;
    bool ok = io[0].read(stream.buf, len)==(size_t)len;
    if( ok ) {
      dec_init();
      PaletteModel model;
      model.code<1>(this, pal, (int32_t)(pal_bytes/3));
      finish();
      ok = stream.buf+len==stream.cur;
    }
    free(stream.buf);
    stream.buf = nullptr;
    return ok;
  }
  // Undoes every representation choice the search made, in the reverse of the
  // order it applied them, leaving the file's own bytes with no flag set.  The
  // raw fallback needs this: it ships the pixels as they are, and a half-undone
  // geometry does not compose back into the identity -- a transpose and a
  // mirror do not commute.
  void restore_representation(BmfImage* p_i) {
    if( p_i[0]._pad8[0]&pad_remap ) {
      remap_apply(p_i, plane_count, remap_mask, remap_split ? remap_dense : nullptr, 1);
      p_i[0]._pad8[0] &= (uint8_t)~(pad_remap|pad_remap_split);
      remap_active = 0;
      remap_split = 0;
    }
    if( p_i[0].flags&flags_pal_sorted ) {
      PaletteOrder po;
      if( palette_order_of(p_i, &po) )
        permute_indices(p_i, po.ord);
      else
        p_i[0].flags &= (uint8_t)~flags_pal_sorted;
    }
    if( p_i[0]._pad8[0]&pad_flip_x )
      flip_image_x(p_i, plane_count);
    if( p_i[0]._pad8[0]&pad_flip_y )
      flip_image_y(p_i, plane_count);
    if( p_i[0].flags&flags_transposed )
      transpose_image(p_i, plane_count);
  }
  int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
    BmfImage hdr;
    if( !arc_in[0].io.fp&&!arc_in[0].io.buf )
      return 0;
    if( extra_blk )
      p_i[0].flags |= flags_tail;
    near_lossless_q = 0;
    // Per image, beside the other coding state.  Only search_filter sets this,
    // and the <=4bpp path returns before reaching it, so without a reset here a
    // packed member following a renumbered one ships stale masks its own header
    // never announces.  No single-image encode can reach that today; the
    // invariant is worth holding anyway, because the failure is a silently
    // unreadable archive rather than a wrong pixel.
    remap_active = 0;
    remap_split = 0;
    hdr.header_from(p_i);
    hdr.flags = p_i[0].flags;
    plane_count = ((p_i[0].depth&depth_bits)+7)>>3;
    const uint32_t tag = bmf_tag(bmf_sig_image, '2', '0');
    if( !arc_in[0].io.write_exact(&tag, 4u) )
      return 0;
    uint8_t bpp = p_i[0].depth;
    uint32_t pal_bytes = 0;
    if( bpp&depth_palette )
      pal_bytes = 3<<(bpp&31);
    if( p_i[0].data_size>=0x10u ) {
      desc_slow_mode = 1;
      hdr.flags |= flags_coded|flags_slow;
      code_image_body(p_i, hdr);
      packer_flush();
      const int32_t coded_bytes = stream.cur-stream.buf;
      hdr.data_size = (uint32_t)coded_bytes;
      bmf_log("  coded body %d bytes vs %d raw: %s\n", coded_bytes, p_i[0].data_size,
              (uint32_t)coded_bytes<p_i[0].data_size ? "shipping the coded member" : "SHIPPING RAW, the coding lost");
      if( (uint32_t)coded_bytes<p_i[0].data_size ) {
        const uint8_t ok = write_member_head(&hdr, extra_blk, &arc_in[0].io);
        // The renumbering masks ride between the head and the coded body, the
        // way the palette rides after it: 32 bytes a plane of raw bitmap -- the
        // used set, and under pad_remap_split the dense set after it -- and
        // nothing in the stream depends on them but the final expansion.
        const uint8_t ok_mask = remap_active
            ? arc_in[0].io.write_exact(remap_mask, 32u*(size_t)plane_count) : 1;
        const uint8_t ok_dense = (remap_active&&remap_split)
            ? arc_in[0].io.write_exact(remap_dense, 32u*(size_t)plane_count) : 1;
        int32_t ok_all = arc_in[0].io.write_exact(stream.buf, (size_t)coded_bytes)&ok&ok_mask&ok_dense;
        free(stream.buf);
        if( ok_all&&(p_i[0].depth&depth_palette)!=0 )
          write_palette(&arc_in[0].io, &p_i[0].pixels[p_i[0].data_size], pal_bytes, p_i[0].depth);
        arc_in[0].io.flush();
        if( ok_all )
          return (int32_t)hdr.data_size;
        return ok_all;
      }
      free(stream.buf);
      restore_representation(p_i);
    }
    if( p_i[0].data_size<0x10u )
      bmf_log("  %d bytes: below the 16-byte floor, shipping raw without coding\n", p_i[0].data_size);
    const uint8_t ok_raw = write_member_head(p_i, extra_blk, &arc_in[0].io);
    uint32_t written = (uint32_t)arc_in[0].io.write(p_i[0].pixels, pal_bytes+p_i[0].data_size);
    int32_t data_size = p_i[0].data_size;
    if( !(ok_raw&(written==data_size+pal_bytes)) )
      return 0;
    return data_size;
  }
  int32_t cost_candidate(BmfImage* img, int32_t cand, PlaneDesc* pd, uint32_t* costs) {
    alignas(16) int32_t hists[6*1024];
    const HistRows hr = hist_rows(hists);
    int32_t*const hist_x = hr.r0;
    int32_t*const hist_y = hr.r1;
    int32_t*const hist_yx = hr.r2;
    int32_t*const hist_zx = hr.r3;
    int32_t*const hist_zy = hr.r4;
    int32_t*const hist_zp = hr.r5;
    memset(hists, 0, sizeof hists);
    const int32_t planes = plane_count;
    const uint32_t row_b = img[0].stride;
    int32_t d1 = (cand+1)%3-cand;
    int32_t d2 = (cand+2)%3-cand;
    uint8_t* img_end = &img[0].pixels[img[0].data_size];
    uint8_t* base = img[0].pixels;
    double syz = 0.0, syy = 0.0, sxz = 0.0, sxy = 0.0, sxx = 0.0;
    pd[cand].nrefs = 2;
    pd[2].src_plane = (uint8_t)cand;
    uint8_t* p = &base[cand+row_b+planes];
    if( p<img_end ) {
      const int32_t a_n = d1-row_b, a_w = d1-planes, a_nw = d1-row_b-planes;
      const int32_t b_n = d2-row_b, b_w = d2-planes, b_nw = d2-row_b-planes;
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
        int32_t diag = p[-step]+p[0];
        int32_t west = p[-planes];
        uint8_t* q = p-row_b;
        p += planes;
        int32_t dz = diag-(q[0]+west);
        sxz = sxz+(double)dx*(double)dz;
        ++hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
        syz = syz+(double)dy*(double)dz;
        ++hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
        int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)((((dx+dy)*kWeightScale)>>1)+CD_weight_round)>>kWeightShift)-512)&0x3FF;
        ++hist_zp[bin];
      } while( p<img_end );
    }
    double inv = (double)kWeightScale/((1.0/(double)idx_clamp(CD_fit_ridge_den, 1, 1<<20))-sxy*sxy+sxx*syy);
    double w1f = (syy*sxz-sxy*syz)*inv;
    double w2f = inv*(sxx*syz-sxy*sxz);
    int32_t wa = clamp_weight((int32_t)w1f);
    int32_t wb = clamp_weight((int32_t)w2f);
    int32_t rec = 4*cand;
    costs[rec] = weight_pair_cost(img, &base[cand], d1, d2, wa, wb);
    costs[rec+1] = estimate_cost(hist_zx, 1024);
    costs[rec+2] = estimate_cost(hist_zy, 1024);
    costs[rec+3] = estimate_cost(hist_zp, 1024);
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
      cost = cost_y;
      swap32(d1, d2);
      swap32(costs[rec+1], costs[rec+2]);
      swap32(wa, wb);
    }
    pd[cand].weight0 = wa;
    pd[cand].weight1 = wb;
    int32_t idx1 = cand+d1;
    int32_t idx2 = cand+d2;
    pd[idx1].nrefs = 0;
    pd[0].src_plane = (uint8_t)idx1;
    pd[idx2].nrefs = 1;
    pd[1].src_plane = (uint8_t)idx2;
    if( img[0].data_size>0x1000000u )
      return cost+cost_yx+costs[rec];
    uint32_t lo2 = costs[rec];
    uint32_t lo3 = costs[rec+2];
    lo2 = umin32(lo2, costs[rec+1]);
    lo3 = umin32(lo3, costs[rec+3]);
    lo3 = umin32(lo3, lo2);
    return best+lo3;
  }
  void deinterleave_plane(const BmfImage* img, uint8_t* dst, int32_t plane) {
    strided_copy(dst, 1, &img[0].pixels[plane], plane_count, bmf_pixels(img));
  }
  void drop_alt_model_from_all() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags &= ~desc_alt_model;
  }
  bool expand_coded(BmfStream* io, BmfImage* img_at, const BmfImage &hdr) {
    near_lossless_q = 0;
    alphabet_reduced = 0;
    if( !(hdr.flags&flags_slow) ) {
      printf("\nwritten in fast mode; this build only decodes -S streams\n");
      exit(3);
    }
    desc_slow_mode = 1;
    // The masks sit between the head and the body, so they come off the stream
    // before it, and the map is rebuilt from the same bytes the encoder wrote:
    // the used set, then the dense set when the header says the map is
    // two-tier.
    remap_active = (hdr._pad8[0]&pad_remap)!=0;
    remap_split = remap_active&&(hdr._pad8[0]&pad_remap_split)!=0;
    if( remap_active
        &&io[0].read(remap_mask, 32u*(size_t)plane_count)!=32u*(size_t)plane_count )
      return false;
    if( remap_split
        &&io[0].read(remap_dense, 32u*(size_t)plane_count)!=32u*(size_t)plane_count )
      return false;
    stream.size = hdr.data_size;
    stream.buf = (uint8_t*)bmf_new(hdr.data_size);
    packer_reset();
    uint32_t want = hdr.data_size;
    bool ok = io[0].read(stream.buf, hdr.data_size)==want;
    if( ok ) {
      if( (img_at[0].depth&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
        bmf_log("[decoding: short path] no descriptors in the stream, one plane\n");
        if( hdr._pad8[0]&pad_ctx_packed ) {
          const int32_t w = img_at[0].width, h = img_at[0].height;
          uint8_t*const flat = (uint8_t*)bmf_new((size_t)w*h);
          plane_predictor = pred_ctx;
          plane_alt_model = 1;
          BmfImage flat_hdr;
          flat_hdr.header_from(img_at);
          flat_hdr.flags = 0;
          flat_hdr.depth = depth_one_plane;
          flat_hdr.stride = (uint16_t)w;
          flat_hdr.data_size = (uint32_t)w*(uint32_t)h;
          unmodel_plane(&flat_hdr, flat);
          pack_rows(img_at, flat);
          free(flat);
          ok = stream.buf+hdr.data_size==stream.cur;
          free(stream.buf);
          stream.buf = nullptr;
          return ok;
        }
        plane_predictor = 0;
        plane_alt_model = 0;
        unmodel_plane(img_at, img_at[0].pixels);
      } else {
        unmodel_described(img_at, hdr.flags);
      }
      ok = stream.buf+hdr.data_size==stream.cur;
    }
    free(stream.buf);
    stream.buf = nullptr;
    return ok;
  }
  BmfImage*expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
    BmfImage hdr;
    CodedTail head;
    int32_t version;
    uint32_t magic;
    if( p_coded_buf )
      *p_coded_buf = nullptr;
    BmfStream* io = &arc_in[0].io;
    if( !io[0].fp&&!io[0].buf )
      return nullptr;
    while( 1 ) {
      if( !io[0].read_exact(&magic, 4u) ) {
        return io[0].eof() ? nullptr : arc_in[0].fail();
      }
      if( (uint16_t)magic!=bmf_sig_other )
        break;
      version = bmf_tag_version(magic);
      if( version!=bmf_version_20||!io[0].read_exact(&head, 8u) )
        break;
      io[0].seek(head.len, SEEK_CUR);
    }
    version = bmf_tag_version(magic);
    if( (uint16_t)magic!=bmf_sig_image||version!=bmf_version_20||!io[0].read_exact(&hdr, 0x10u) ) {
      return arc_in[0].fail();
    }
    if( hdr.flags&flags_tail ) {
      if( !io[0].read_exact(&head, 8u) )
        return arc_in[0].fail();
      if( p_coded_buf ) {
        if( head.len>io[0].bytes_left() ) {
          return arc_in[0].fail();
        }
        uint32_t pad_len = (head.len+(head.len==0)+3)&0xFFFFFFFC;
        CodedTail* blk = (CodedTail*)bmf_new(pad_len+sizeof(CodedTail));
        blk[0].tag = head.tag;
        blk[0].len = pad_len;
        blk[0].data[pad_len/4-1] = 0;
        p_coded_buf[0] = blk;
        if( !io[0].read_exact(blk[0].data, head.len) )
          return arc_in[0].fail();
      } else {
        io[0].seek(head.len, SEEK_CUR);
      }
    }
    if( hdr.data_size>io[0].bytes_left() ) {
      return arc_in[0].fail();
    }
    uint32_t pal_bytes = 0;
    if( hdr.depth&depth_palette ) {
      // alloc_image only reserves table space for 8 bits and under, so a header
      // claiming a palette at a greater depth would have the read run off the
      // end of the image buffer.
      if( (hdr.depth&depth_bits)>8 )
        return arc_in[0].fail();
      pal_bytes = 3u<<(hdr.depth&depth_bits);
    }
    if( !hdr.width||!hdr.height||(uint32_t)hdr.width*(uint32_t)((((hdr.depth&depth_bits)+7)>>3))>0xFFFFu ) {
      return arc_in[0].fail();
    }
    BmfImage* img_at = alloc_image(hdr.width, hdr.height, hdr.depth&depth_bits, (hdr.depth&depth_palette)!=0, 1);
    img_at[0].depth = hdr.depth;
    // The spare header bytes describe the pixels rather than the coding, so
    // they have to reach the caller: one of them carries the strip count.
    for( int32_t k = 0; k<4; ++k )
      img_at[0]._pad8[k] = hdr._pad8[k];
    uint8_t has_coded = p_coded_buf&&p_coded_buf[0];
    img_at[0].flags |= (hdr.flags&flags_transposed)|(has_coded ? flags_tail : 0);
    plane_count = ((hdr.depth&depth_bits)+7)>>3;
    if( plane_count<1||plane_count>4 ) {
      return arc_in[0].fail();
    }
    remap_active = 0;
    remap_split = 0;
    if( hdr.flags&flags_coded ) {
      if( !expand_coded(io, img_at, hdr) ) {
        return arc_in[0].fail();
      }
    } else {
      if( hdr.data_size>img_at[0].data_size ) {
        return arc_in[0].fail();
      }
      uint32_t want = hdr.data_size;
      if( io[0].read(img_at[0].pixels, hdr.data_size)!=want ) {
        return arc_in[0].fail();
      }
    }
    if( hdr.depth&depth_palette ) {
      uint8_t* pal_at = img_at[0].palette();
      // Only a coded body carries a coded palette; the raw path still writes
      // the table straight after the pixels in one piece.
      const bool coded_pal = (hdr.flags&flags_coded)!=0;
      if( !(coded_pal ? read_palette(io, pal_at, pal_bytes, hdr.depth)
                      : io[0].read(pal_at, pal_bytes)==pal_bytes) ) {
        return arc_in[0].fail();
      }
    }
    if( remap_active )
      remap_apply(img_at, plane_count, remap_mask, remap_split ? remap_dense : nullptr, 1);
    if( hdr._pad8[0]&pad_ycocg ) {
      // Written by a build that still had the YCoCg-R lifting.  Refusing is the
      // only honest answer: without the inverse these pixels cannot be
      // reproduced, and decoding them anyway would be silently wrong.
      printf("\nstream uses YCoCg-R lifting, which this build no longer implements\n");
      exit(3);
    }
    if( hdr.flags&flags_pal_sorted ) {
      // The table went out in the file's own order, so rebuilding the ordering
      // from it gives exactly the permutation the encoder applied.
      PaletteOrder po;
      if( !palette_order_of(img_at, &po) ) {
        return arc_in[0].fail();
      }
      permute_indices(img_at, po.ord);
    }
    // Undone in the reverse of the order search_filter applied them: the
    // transpose came first, so it comes off last.
    if( hdr._pad8[0]&pad_flip_x ) {
      flip_image_x(img_at, plane_count);
    }
    if( hdr._pad8[0]&pad_flip_y ) {
      flip_image_y(img_at, plane_count);
    }
    if( img_at[0].flags&flags_transposed ) {
      // Sub-byte-packed images transpose only when both orientations occupy the
      // same number of bytes; byte-interleaved ones need room for w*h*planes.
      const bool fits = (img_at[0].flags&flags_packed)
                        ? packed_transpose_ok(img_at)
                        : (uint64_t)img_at[0].width*img_at[0].height*(uint32_t)plane_count<=(uint64_t)img_at[0].data_size;
      if( !fits ) {
        return arc_in[0].fail();
      }
      transpose_image(img_at, plane_count);
    }
    return img_at;
  }
  void interleave_flat(BmfImage* img, const uint8_t* src, int32_t plane) {
    strided_copy(&img[0].pixels[plane], plane_count, src, 1, bmf_pixels(img));
  }
  void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane) {
    code_colour_plane<1>(img, src, plane);
  }
  void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    code_plane<0>(p_i, pixels, raw);
  }
  void model_planes(BmfImage* img, uint8_t* pixels, int32_t plane) {
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
    colour_transform(img, pixels, plane);
    memset(hist_scratch, 0, 1024);
    {
      BmfImage hdr = img[0];
      hdr.depth = depth_one_plane;
      if( plane_predictor==pred_p1&&!plane_alt_model )
        predict_med(pixels, img[0].width, img[0].height);
      model_plane(&hdr, pixels, pixels);
    }
  }
  ModelBlock*new_model_block(int32_t img_w, int32_t img_h, int32_t img_depth) {
    void* raw = model_blocks.take(false);
    return ((ModelBlock*)raw)[0].layout_workspace(this, img_w, img_h, img_depth);
  }
  void offer_to_all(int32_t want) {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags = (plane_desc[k].flags&desc_has_refs)|want;
  }
  void pack_bits(uint32_t word, int32_t n) {
    if( stream.pk.free_bits<n ) {
      stream.pk.word[0] = stream.pk.acc|(2*(word<<((31-stream.pk.free_bits)&31)));
      stream.pk.word = (uint32_t*)stream.cur;
      stream.cur += 4;
      stream.pk.acc = word>>(stream.pk.free_bits&31);
      stream.pk.free_bits += 32-n;
    } else {
      stream.pk.acc |= word<<(-stream.pk.free_bits&31);
      stream.pk.free_bits -= n;
    }
  }
  void packer_flush() {
    stream.pk.word[0] = stream.pk.acc;
  }
  void packer_reset() {
    stream.pk.free_bits = 0;
    stream.pk.acc = 0;
    stream.cur = stream.buf;
    stream.pk.word = (uint32_t*)stream.buf;
    hist_scratch = stream.buf+stream.size-4096;
  }
  uint32_t packer_word() {
    if( stream.cur+4>stream.buf+stream.size ) bmf_fatal(bmf_read_error);
    uint32_t w = ((uint32_t*)stream.cur)[0];
    stream.cur += 4;
    return w;
  }
  int32_t plane_in_order(int32_t k) {
    const int32_t plane = plane_desc[k].src_plane;
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    return plane;
  }
  PlaneTransform plane_transform(int32_t plane) {
    PlaneTransform t;
    t.to_ref0 = plane_desc[0].src_plane-plane;
    t.to_ref1 = plane_desc[1].src_plane-plane;
    t.to_ref2 = plane_desc[2].src_plane-plane;
    t.mode = plane_desc[plane].nrefs;
    t.dc = plane_desc[plane].dc;
    t.wgt1 = plane_desc[plane].weight1;
    t.wgt0 = plane_desc[plane].weight0;
    t.wgt2 = plane_desc[plane].weight2;
    t.by_weights = false;
    if( t.mode==2&&t.wgt0+t.wgt1==kWeightScale ) {
      if( !t.wgt1 ) {
        t.by_weights = true;
      } else if( !t.wgt0 ) {
        t.to_ref0 = plane_desc[1].src_plane-plane;
        t.by_weights = true;
      }
    }
    return t;
  }
  void predict_med(uint8_t* pixels, int32_t width, int32_t height) {
    uint32_t done, last;
    alignas(16) uint8_t fold[272];
    uint8_t* p = pixels+height*width;
    uint8_t* up = &p[-width];
    med_fold_table(fold);
    for( int32_t rows_left = height-1; rows_left; --rows_left ) {
      for( int32_t x_left = width-1; x_left; --x_left ) {
        const int32_t north = (uint8_t)*--up;
        --p;
        const uint8_t pred = med_predict(p[-1], north, p[-width-1]);
        const int32_t code = (uint8_t)fold[(uint8_t)(p[0]-pred)];
        p[0] = code;
        hist_bump(code);
      }
      --up;
      --p;
      last = (uint8_t)fold[(uint8_t)(p[0]-p[-width])];
      p[0] = last;
      hist_bump(last);
    }
    if( width!=1 ) {
      last = width-1;
      const int32_t pairs = last/2;
      {
        uint32_t k = 0;
        for( int32_t ofs = 0; k<(uint32_t)pairs; ++k, ofs -= 2 ) {
          const uint8_t left = p[ofs-2];
          p[ofs-1] = fold[(uint8_t)(p[ofs-1]-left)];
          p[ofs-2] = fold[(uint8_t)(left-p[ofs-3])];
        }
        done = 2*k+1;
      }
      if( last>(done-1) ) {
        uint8_t* q = p-done;
        last = (uint8_t)(q[0]-*((int8_t*)q-1));
        q[0] = fold[(uint8_t)last];
      }
    }
  }
  void read_plane_descs() {
    code_plane_descs<1>();
  }
  void reset_descriptors() {
    for( int32_t k = 0; k<plane_count; ++k ) {
      plane_desc[k].flags = 0;
      plane_desc[k].src_plane = k;
      plane_desc[k].nrefs = k;
    }
  }
  void restore_descriptors(const PlaneDesc* saved) {
    for( int32_t k = 0; k<4; ++k )
      plane_desc[k] = saved[k];
  }
  void save_descriptors(PlaneDesc* saved) {
    for( int32_t k = 0; k<4; ++k )
      saved[k] = plane_desc[k];
  }
  // The three generators of the eight rigid orientations of a raster.
  enum : int32_t {
    geo_transpose = 0,
    geo_flip_x = 1,
    geo_flip_y = 2,
  };
  void apply_geometry(BmfImage* img, int32_t op) {
    if( op==geo_transpose )
      transpose_image(img, plane_count);
    else if( op==geo_flip_x )
      flip_image_x(img, plane_count);
    else
      flip_image_y(img, plane_count);
  }
  // One whole-image geometry trial: apply the generator to the tile, give the
  // result its own full per-plane search, and keep it only if it measures
  // cheaper.  A rejected trial leaves no trace.
  bool try_geometry(int32_t op, const char* name, BmfImage* img, BmfImage* tile_img, uint8_t* tile_buf,
                    uint32_t* plane_cost, PlaneSearch &ps, int32_t &best_bits, uint32_t &cur_est) {
    PlaneDesc saved_desc[4];
    uint32_t saved_cost[4];
    save_descriptors(saved_desc);
    for( int32_t k = 0; k<4; ++k )
      saved_cost[k] = plane_cost[k];
    apply_geometry(tile_img, op);
    const uint32_t est = choose_plane_coding(tile_img);
    const PlaneSearch pt = search_planes(tile_img, tile_buf, plane_cost);
    const bool kept = pt.bits+(pt.bits>>kKeepShift)>=best_bits;
    bmf_log("    %-22s %12.1f bytes%s\n", name, bmf_bytes(pt.bits), kept ? "" : "  <<");
    if( kept ) {
      apply_geometry(tile_img, op);
      restore_descriptors(saved_desc);
      for( int32_t k = 0; k<4; ++k )
        plane_cost[k] = saved_cost[k];
      return false;
    }
    best_bits = pt.bits;
    ps = pt;
    cur_est = est;
    apply_geometry(img, op);
    return true;
  }
  // A centred crop of the image, one byte per plane per pixel, no palette.
  BmfImage*crop_tile(BmfImage* img, int32_t tile_w, int32_t tile_h) {
    BmfImage* tile = alloc_image(tile_w, tile_h, img[0].depth&depth_bits, 0, 0);
    const int32_t y0 = (img[0].height-tile_h)>>1;
    const int32_t dx = img[0].width-tile_w;
    const uint8_t* src = img[0].pixels+plane_count*(dx>>1)+(uint32_t)y0*img[0].stride;
    uint8_t* dst = tile[0].pixels;
    for( int32_t y = 0; y<tile_h; ++y ) {
      memcpy(dst, src, tile[0].stride);
      dst += tile[0].stride;
      src += img[0].stride;
    }
    return tile;
  }
  // Does this image carry a convention that makes alpha worth coding first?
  // Two are worth detecting, both a single scan: every fully transparent pixel
  // shares one colour (so that colour is a function of the alpha), or every
  // colour byte is bounded by the alpha (premultiplied).  Where neither holds,
  // alpha carries no information about the colour planes and putting it in slot
  // 0 only costs them a useful reference -- 15% on x_ep, 100% on t32.
  bool alpha_convention(const BmfImage* img) {
    const uint8_t* p = img[0].pixels;
    const uint8_t*const end = p+img[0].data_size;
    bool premultiplied = true, one_colour = true;
    uint32_t transparent = 0;
    uint8_t first[3] = {0, 0, 0};
    for(; p+4<=end; p += 4 ) {
      const int32_t a = p[3];
      if( p[0]>a||p[1]>a||p[2]>a )
        premultiplied = false;
      if( !a ) {
        if( !transparent ) {
          first[0] = p[0];
          first[1] = p[1];
          first[2] = p[2];
        } else if( p[0]!=first[0]||p[1]!=first[1]||p[2]!=first[2] ) {
          one_colour = false;
        }
        ++transparent;
      }
    }
    const bool worth_it = premultiplied||(one_colour&&transparent>=(img[0].data_size>>8));
    bmf_log("  alpha: %u fully transparent pixels, %s, %s -> %s\n", transparent,
            premultiplied ? "premultiplied" : "not premultiplied",
            one_colour ? "all one colour under alpha=0" : "many colours under alpha=0",
            worth_it ? "coding order searched" : "alpha stays last");
    return worth_it;
  }
  // Move physical plane 3 to a given coding slot, keeping the others in order.
  // Slot and reference count are the same field, so a plane's slot is also how
  // many planes it may reference: slot 0 references nothing, slots 1 and 2
  // reference the planes in slots 0 and 1, and slot 3 references all three.
  void move_alpha_to(int32_t slot) {
    uint8_t order[4];
    int32_t at = 0;
    for( int32_t s = 0; s<4; ++s ) {
      if( at==slot )
        ++at;
      if( plane_desc[s].src_plane!=3 )
        order[at++] = plane_desc[s].src_plane;
    }
    order[slot] = 3;
    for( int32_t s = 0; s<4; ++s )
      plane_desc[s].src_plane = order[s];
    for( int32_t s = 0; s<4; ++s )
      plane_desc[order[s]].nrefs = (uint8_t)s;
  }
  uint32_t search_filter(BmfImage* img) {
    PlaneDesc saved[4];
    uint32_t plane_cost[4];
    int32_t cand, pl_k;
    int32_t tile_w = img[0].width;
    int32_t tile_h = img[0].height;
    if( tile_w<4||tile_h<3 ) {
      reset_descriptors();
      return 0;
    }
    uint32_t cur_est = choose_plane_coding(img);
    bmf_log("[search_filter] %dx%d, %d plane%s, %d bits/pixel raw\n", tile_w, tile_h, plane_count, plane_count==1 ? "" : "s", 8*plane_count);
    stream_open(img[0].data_size);
    BmfImage* tile_img = crop_tile(img, tile_w, tile_h);
    uint8_t* tile_buf = (uint8_t*)bmf_new(tile_h*tile_w);
    uint8_t* tile_src = tile_img[0].pixels;
    PlaneSearch ps = search_planes(tile_img, tile_buf, plane_cost);
    int32_t best_bits = ps.bits;
    bmf_log("  whole-image trials (against the per-plane total):\n");
    bmf_log("    %-22s %12.1f bytes  (p1=%d p2=%d refs=%d)\n", "planar", bmf_bytes(ps.bits), ps.n_p1, ps.n_p2, ps.n_refs);
    // Value-set renumbering, tried before the geometry so that everything after
    // it searches the dense alphabet.  planes.inc says why it cannot be decided
    // from the histogram; this is the trial that decides it, and it costs a
    // pass only on an image that has a comb at all.
    remap_active = 0;
    remap_split = 0;
    if( plane_count<=4 ) {
      uint8_t masks[128], dense[128];
      int32_t hist[4*256];
      remap_hist(img, plane_count, hist);
      for( int32_t k = 0; k<plane_count; ++k )
        remap_mask_from_hist(hist+256*k, 1, masks+32*k);
      int32_t holes = 0;
      for( int32_t k = 0; k<plane_count; ++k )
        holes += remap_holes(masks+32*k);
      if( holes>0 ) {
        // Which values get the low, contiguous ranks.  The plain rank map is
        // the incumbent -- every value dense -- and each rung of the floor
        // ladder is a challenger that has to beat it on the MED probe by
        // kKeepShift of its own cost, the same margin the trials downstream
        // use.  A plane whose histogram has no light tail sees every rung
        // reproduce the incumbent's mask and keeps it.
        int32_t split_any = 0;
        const int64_t npix = (int64_t)img[0].width*img[0].height;
        for( int32_t k = 0; k<plane_count; ++k ) {
          uint8_t fwd[256], inv[256], cand[32];
          memcpy(dense+32*k, masks+32*k, 32);
          remap_build(masks+32*k, masks+32*k, fwd, inv);
          int32_t best_probe = remap_probe(img, plane_count, k, fwd);
          int32_t chose = -1;
          for( int32_t sh = kRareShiftLo; sh<=kRareShiftHi; ++sh ) {
            const int32_t floor_ = max32(2, (int32_t)(npix>>sh));
            remap_mask_from_hist(hist+256*k, floor_, cand);
            if( !memcmp(cand, masks+32*k, 32) )
              continue;
            remap_build(masks+32*k, cand, fwd, inv);
            const int32_t c = remap_probe(img, plane_count, k, fwd);
            if( c+(c>>kKeepShift)<best_probe ) {
              best_probe = c;
              chose = sh;
              memcpy(dense+32*k, cand, 32);
            }
          }
          if( chose>=0 ) {
            split_any = 1;
            bmf_log("    plane %d dense set cut at 1/%d of the samples: %d of %d values\n",
                    k, 1<<chose, remap_used(dense+32*k), remap_used(masks+32*k));
          }
        }
        remap_split = split_any;
        const uint8_t*const dense_at = split_any ? dense : nullptr;
        PlaneDesc saved_desc[4];
        uint32_t saved_cost[4];
        save_descriptors(saved_desc);
        for( int32_t k = 0; k<4; ++k )
          saved_cost[k] = plane_cost[k];
        remap_apply(tile_img, plane_count, masks, dense_at, 0);
        const uint32_t r_est = choose_plane_coding(tile_img);
        const PlaneSearch pr = search_planes(tile_img, tile_buf, plane_cost);
        // The masks are side information and the trial has to pay for them, and
        // then to clear kKeepShift on top like every other representation trial
        // -- the transpose, the palette order and the colour space all require
        // a challenger to beat the incumbent by a fraction of its own cost.  A
        // bare comparison is not enough here for a reason worth stating: on the
        // files that adopt the joint transform, the trial ledger is not the
        // stream that ships, so a 64-byte win on the ledger can be a 124-byte
        // loss in the file.  Every real winner clears this by percent.
        const int32_t mask_bits = 8*32*plane_count*(split_any ? 2 : 1);
        const int32_t r_bits = pr.bits+mask_bits;
        const bool kept_raw = r_bits+(r_bits>>kKeepShift)>=best_bits;
        bmf_log("    %-22s %12.1f bytes  (%d interior gap%s, %d bytes of mask%s)%s\n",
                "renumbered", bmf_bytes(r_bits), holes, holes==1 ? "" : "s",
                mask_bits>>3, split_any ? ", two-tier" : "", kept_raw ? "" : "  <<");
        if( kept_raw ) {
          remap_apply(tile_img, plane_count, masks, dense_at, 1);
          restore_descriptors(saved_desc);
          for( int32_t k = 0; k<4; ++k )
            plane_cost[k] = saved_cost[k];
          remap_split = 0;
        } else {
          best_bits = r_bits;
          ps = pr;
          cur_est = r_est;
          remap_apply(img, plane_count, masks, dense_at, 0);
          img[0]._pad8[0] |= pad_remap;
          if( split_any )
            img[0]._pad8[0] |= pad_remap_split;
          memcpy(remap_mask, masks, 32u*(uint32_t)plane_count);
          memcpy(remap_dense, dense, 32u*(uint32_t)plane_count);
          remap_active = 1;
        }
      }
    }
    // Only the transpose.  The mirrors were built and measured here too: they
    // are worth 0.04% of the corpus and cost 60% of encode time, because every
    // orientation needs its own full per-plane search and the decision cannot
    // be delegated to a crop -- a megapixel of x_ai does not show the 9.6% its
    // whole raster gets from a transpose.  They stay in the <=4 bpp short path,
    // where the searches are cheap and they are worth three times as much.
    try_geometry(geo_transpose, "transposed", img, tile_img, tile_buf, plane_cost, ps, best_bits, cur_est);
    {
      // Palette order.  Both sides derive the luminance ordering from the table
      // itself, so choosing it costs one flag bit and no side information; the
      // table always goes out in the file's original order.
      PaletteOrder po;
      if( palette_order_of(img, &po) ) {
        PlaneDesc saved_desc[4];
        uint32_t saved_cost[4];
        save_descriptors(saved_desc);
        for( int32_t k = 0; k<4; ++k )
          saved_cost[k] = plane_cost[k];
        permute_indices(tile_img, po.rank);
        choose_plane_coding(tile_img);
        const PlaneSearch pp = search_planes(tile_img, tile_buf, plane_cost);
        const bool kept_order = pp.bits+(pp.bits>>kKeepShift)>=best_bits;
        bmf_log("    %-22s %12.1f bytes%s\n", "palette in luma order", bmf_bytes(pp.bits), kept_order ? "" : "  <<");
        if( kept_order ) {
          permute_indices(tile_img, po.ord);
          restore_descriptors(saved_desc);
          for( int32_t k = 0; k<4; ++k )
            plane_cost[k] = saved_cost[k];
        } else {
          best_bits = pp.bits;
          ps = pp;
          permute_indices(img, po.rank);
        }
      }
    }
    if( plane_count==4&&alpha_convention(img) ) {
      // Alpha is assigned coding slot 3 rather than searched for one, so
      // nothing may ever reference it -- and on an image where the colour under
      // a transparent pixel is a function of the alpha, that is the one
      // reference worth having.  Offer the order that makes it available.
      PlaneDesc saved_desc[4];
      uint32_t saved_cost[4];
      save_descriptors(saved_desc);
      for( int32_t k = 0; k<4; ++k )
        saved_cost[k] = plane_cost[k];
      // Slot 2 rather than slot 0: references always come from the planes in
      // slots 0 and 1, so putting alpha there would cost every colour plane a
      // colour reference for an alpha one.  In slot 2 alpha keeps two colour
      // references for itself and becomes the third reference of slot 3.
      static const char*const where[2] = {"alpha in slot 2", "alpha coded first"};
      for( int32_t k = 0; k<2; ++k ) {
        move_alpha_to(k ? 0 : 2);
        const PlaneSearch pa = search_planes(tile_img, tile_buf, plane_cost);
        const bool kept_order = pa.bits+(pa.bits>>kKeepShift)>=best_bits;
        bmf_log("    %-22s %12.1f bytes%s\n", where[k], bmf_bytes(pa.bits), kept_order ? "" : "  <<");
        if( kept_order ) {
          restore_descriptors(saved_desc);
          for( int32_t j = 0; j<4; ++j )
            plane_cost[j] = saved_cost[j];
        } else {
          best_bits = pa.bits;
          ps = pa;
          save_descriptors(saved_desc);
          for( int32_t j = 0; j<4; ++j )
            saved_cost[j] = plane_cost[j];
        }
      }
    }
    free(tile_buf);
    if( plane_count>2 ) {
      if( ps.n_p1 ) {
        save_descriptors(saved);
        offer_to_all(try_p1);
        int32_t bits_e = transform_cost(tile_img);
        if( bits_e<=best_bits ) {
          bmf_log("    %-22s %12.1f bytes  <<\n", "joint alt-P1", bmf_bytes(bits_e));
          best_bits = bits_e;
          cand = 0;
        } else {
          bmf_log("    %-22s %12.1f bytes\n", "joint alt-P1", bmf_bytes(bits_e));
          restore_descriptors(saved);
          cand = 1;
        }
      } else {
        cand = 1;
      }
      if( ps.n_p2+(cand==0) ) {
        save_descriptors(saved);
        offer_to_all(try_p2);
        if( plane_count==ps.n_p2&&plane_count-1==ps.n_refs ) {
          bmf_log("    %-22s adopted unmeasured (every plane already chose it)\n", "joint alt-P2");
          cand = 0;
        } else {
          const int32_t bits_a = transform_cost(tile_img);
          if( bits_a<=best_bits ) {
            bmf_log("    %-22s %12.1f bytes  <<\n", "joint alt-P2", bmf_bytes(bits_a));
            best_bits = bits_a;
            ps.n_p2 = bits_a;
            if( plane_count-1==ps.n_refs ) {
              cand = 0;
            } else {
              save_descriptors(saved);
              allow_refs_where_present();
              int32_t bits2 = transform_cost(tile_img);
              bmf_log("    %-22s %12.1f bytes%s\n", "joint alt-P2 + refs", bmf_bytes(bits2), bits2>bits_a ? "" : "  <<");
              if( bits2>bits_a )
                restore_descriptors(saved);
              cand = 0;
            }
          } else {
            bmf_log("    %-22s %12.1f bytes\n", "joint alt-P2", bmf_bytes(bits_a));
            restore_descriptors(saved);
          }
        }
      }
    } else {
      cand = 1;
    }
    if( !ps.n_p2&&plane_count>1 ) {
      uint8_t* tile_copy = (uint8_t*)bmf_new(tile_img[0].data_size);
      memcpy(tile_copy, tile_src, tile_img[0].data_size);
      save_descriptors(saved);
      drop_alt_model_from_all();
      int32_t bits_b = transform_cost(tile_img);
      if( bits_b>best_bits ) {
        bmf_log("    %-22s %12.1f bytes\n", "joint, no alt model", bmf_bytes(bits_b));
        restore_descriptors(saved);
      } else {
        bmf_log("    %-22s %12.1f bytes  <<\n", "joint, no alt model", bmf_bytes(bits_b));
        best_bits = bits_b;
        cand = 0;
      }
      if( ps.n_refs+ps.n_p1 ) {
        save_descriptors(saved);
        clear_flags_on_all();
        memcpy(tile_src, tile_copy, tile_img[0].data_size);
        int32_t bits_c = transform_cost(tile_img);
        if( bits_c<=best_bits ) {
          bmf_log("    %-22s %12.1f bytes  <<\n", "joint, flags cleared", bmf_bytes(bits_c));
          cand = 0;
        } else {
          bmf_log("    %-22s %12.1f bytes\n", "joint, flags cleared", bmf_bytes(bits_c));
          restore_descriptors(saved);
        }
      }
      free(tile_copy);
    }
    free(stream.buf);
    free((uint8_t*)tile_img);
    bmf_log("  choice: %s, %.1f bytes (%.3f bpp) by the trials' own measure\n",
            cand ? "planar -- each plane coded separately" : "together -- one joint transform",
            bmf_bytes(best_bits), (double)best_bits/(double)(tile_w*tile_h));
    log_plane_descs("chosen descriptors");
    return cand;
  }

  // The descriptor table as it stands: which model each plane uses, in which
  // coding slot, with which reference blend.  Printed under -v on both sides.
  void log_plane_descs(const char* what) {
    if( !bmf_verbose )
      return;
    bmf_log("[%s]\n", what);
    for( int32_t pl = 0; pl<plane_count; ++pl ) {
      const PlaneDesc &d = plane_desc[pl];
      if( !(d.flags&desc_has_refs) ) {
        bmf_log("  plane %d: slot %d  %s\n", pl, d.nrefs, desc_flags_name(d.flags));
        continue;
      }
      bmf_log("  plane %d: slot %d  %-9s dc=%d", pl, d.nrefs, desc_flags_name(d.flags), (int32_t)d.dc);
      if( d.nrefs>1u )
        bmf_log("  w=(%d,%d", d.weight0, d.weight1);
      if( d.nrefs>2u )
        bmf_log(",%d", d.weight2);
      if( d.nrefs>1u )
        bmf_log(")");
      bmf_log("\n");
    }
  }
  void stream_open(uint32_t data_size) {
    stream.size = data_size+0x20000;
    stream.buf = (uint8_t*)bmf_new(stream.size);
    packer_reset();
  }
  int32_t transform_cost(BmfImage* tile_img) {
    transform_planes(tile_img);
    const int32_t bits = 8*(stream.cur-stream.buf)+descriptor_bits_all();
    packer_flush();
    packer_reset();
    return bits;
  }
  void transform_planes(BmfImage* p_i) {
    int32_t i;
    memset(hist_scratch, 0, 4096);

    BmfImage* hdr = (BmfImage*)(stream.buf+sizeof(BmfImage));
    hdr[0] = p_i[0];
    uint8_t* src_pixels = p_i[0].pixels;
    memcpy(hdr[0].pixels, p_i[0].pixels, p_i[0].data_size);
    uint8_t* tmp = (uint8_t*)bmf_new(p_i[0].width*p_i[0].height);
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_in_order(k);
      const int32_t predictor = plane_predictor;
      const int32_t alt = (plane_desc[plane].flags&desc_alt_model)!=0;
      plane_alt_model = alt;
      const bool transform_it = ((plane_desc[plane].flags&desc_has_refs)!=0||predictor)&&!alt;
      if( !transform_it )
        continue;
      colour_transform(hdr, tmp, plane);
      if( plane_predictor==pred_p1 )
        predict_med(tmp, p_i[0].width, p_i[0].height);
      int32_t n = p_i[0].width*p_i[0].height;
      int32_t stride = plane_count;
      if( plane_count==1 ) {
        memcpy(&p_i[0].pixels[plane], tmp, n);
      } else {
        uint8_t* dst = &p_i[0].pixels[plane];
        int32_t ofs = 0;
        for( i = 0; i<n; ++i ) {
          dst[ofs] = tmp[i];
          ofs += stride;
        }
      }
    }
    free(tmp);
    model_plane(p_i, src_pixels, src_pixels);
  }
  void unmodel_described(BmfImage* img_at, uint8_t flags) {
    if( plane_count==1 ? (img_at[0].depth&depth_grey)!=0 : plane_count>2 ) {
      uint32_t near_lossless = unpack_bits(4);
      if( near_lossless ) {
        printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
        exit(3);
      }
      near_lossless_q = 0;
    }
    read_plane_descs();
    log_plane_descs(flags&flags_planar ? "decoding: planar" : "decoding: together");
    uint8_t* plane_buf = (uint8_t*)bmf_new(img_at[0].width*img_at[0].height);
    if( flags&flags_planar )
      unmodel_planes_apart(img_at, plane_buf);
    else
      unmodel_planes_together(img_at, plane_buf);
    free(plane_buf);
  }
  void unmodel_plane(BmfImage* p_i, uint8_t* out) {
    code_plane<1>(p_i, out, nullptr);
  }
  void unmodel_planes_apart(BmfImage* img_at, uint8_t* plane_buf) {
    BmfImage plane_hdr = img_at[0];
    plane_hdr.depth = depth_one_plane;
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_desc[k].src_plane;
      plane_predictor = plane_desc[plane].flags&desc_predictor;
      plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
      unmodel_plane(&plane_hdr, plane_buf);
      if( plane_predictor==pred_p1&&!plane_alt_model )
        unpredict_med(plane_buf, img_at[0].width, img_at[0].height);
      interleave_plane(img_at, plane_buf, plane);
    }
  }
  void unmodel_planes_together(BmfImage* img_at, uint8_t* plane_buf) {
    plane_predictor = plane_desc[0].flags&desc_predictor;
    plane_alt_model = (plane_desc[0].flags&desc_alt_model)!=0;
    unmodel_plane(img_at, img_at[0].pixels);
    if( plane_alt_model )
      return;
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_in_order(k);
      const int32_t pred = plane_predictor;
      if( (plane_desc[plane].flags&desc_has_refs)==0&&!pred )
        continue;
      deinterleave_plane(img_at, plane_buf, plane);
      if( pred==pred_p1 )
        unpredict_med(plane_buf, img_at[0].width, img_at[0].height);
      interleave_plane(img_at, plane_buf, plane);
    }
  }
  uint32_t unpack_bits(int32_t n) {
    const uint32_t mask = (1u<<n)-1;
    uint32_t v;
    stream.pk.free_bits -= n;
    if( stream.pk.free_bits<0 ) {
      const uint32_t w = packer_word();
      v = stream.pk.acc|((w<<((stream.pk.free_bits+n)&31))&mask);
      stream.pk.acc = w>>(-stream.pk.free_bits&31);
      stream.pk.free_bits += 32;
    } else {
      v = stream.pk.acc&mask;
      stream.pk.acc = stream.pk.acc>>n;
    }
    return v;
  }
  void write_plane_descs() {
    code_plane_descs<0>();
  }
  // Set while a remapped image is being coded, so code_plane_descs knows to
  // carry the masks.  It is derived from the header flag on the way in and from
  // the search's decision on the way out.
  int32_t remap_active = 0;
  // Set when the map is two-tier, in which case remap_dense also ships.
  int32_t remap_split = 0;
  uint8_t remap_mask[128];
  uint8_t remap_dense[128];

  int32_t weight_pair_cost(const BmfImage* img, const uint8_t* plane0, int32_t plane_a, int32_t plane_b, int32_t w4, int32_t w8) {
    int32_t hist[1024];
    memset(hist, 0, sizeof hist);
    const int32_t stride = img[0].stride;
    const int32_t left = -plane_count;
    const int32_t ofs_up = -stride;
    const int32_t ofs_ul = -stride-plane_count;
    const uint8_t* p = plane0-ofs_ul;
    const uint8_t* q = plane0+plane_a-ofs_ul;
    const uint8_t* r = plane0+plane_b-ofs_ul;
    for( int32_t n = img[0].width*(img[0].height-1)-1; n; --n ) {
      const int32_t cur = q[0];
      ++hist[residual_bin(p[ofs_ul]+p[0]-p[ofs_up]-p[left], w4, q[ofs_ul]+cur-q[ofs_up]-q[left], w8, r[ofs_ul]+r[0]-r[ofs_up]-(uint32_t)r[left])];
      q -= left;
      p -= left;
      r -= left;
    }
    return estimate_cost(hist, 1024);
  }
  // What one plane's descriptor costs on the wire, in bits, for a given flags
  // byte -- see code_plane_descs.  The trials compare residual costs alone,
  // which on a small image is enough to buy a 20-byte gain with 32 bits of
  // header; a reference costs three or four bytes to describe and nothing in
  // the search accounted for it.
  int32_t descriptor_bits(int32_t plane, int32_t flags) {
    if( !(flags&desc_has_refs) )
      return 6;
    const int32_t nrefs = plane_desc[plane].nrefs;
    return 6+8+(nrefs>1 ? 16 : 0)+(nrefs>2 ? 8 : 0);
  }
  int32_t descriptor_bits_all() {
    int32_t bits = 0;
    for( int32_t k = 0; k<plane_count; ++k )
      bits += descriptor_bits(k, plane_desc[k].flags);
    return bits;
  }
  // Whether the direct context model can code this plane at all.  The plane
  // buffer is filled by the trial itself, so the alphabet is scanned from the
  // image: a single 8-bit plane is the interleaved buffer.
  bool ctx_model_ok(BmfImage* tile_img, uint8_t* tile_buf, int32_t plane) {
    (void)tile_buf;
    if( plane_count!=1||(tile_img[0].depth&depth_bits)!=8||plane!=0 )
      return false;
    uint8_t vals[CtxModel::kMaxAlpha];
    const int32_t n = tile_img[0].width*tile_img[0].height;
    return CtxModel::scan_alphabet(tile_img[0].pixels, n, vals)<=CtxModel::kMaxAlpha;
  }
  PlaneSearch search_planes(BmfImage* tile_img, uint8_t* tile_buf, uint32_t* plane_cost) {
    int32_t best_cost = 0, best_flags = 0;
    int32_t trial_flags[8], trial_cost[8], trial_n = 0;
    auto probe_plane = [&](int32_t pl, int32_t flags) {
                         plane_desc[pl].flags = flags;
                         model_planes(tile_img, tile_buf, pl);
                         int32_t cost = 8*(stream.cur-stream.buf)+descriptor_bits(pl, flags);
                         packer_flush();
                         if( bmf_verbose&&trial_n<8 ) {
                           trial_flags[trial_n] = flags;
                           trial_cost[trial_n] = cost;
                           ++trial_n;
                         }
                         return cost;
                       };
    bool won = false;
    auto probe_keep = [&](int32_t pl, int32_t flags) {
                        const int32_t cost = probe_plane(pl, flags);
                        packer_reset();
                        won = cost<best_cost;
                        if( won ) {
                          best_cost = cost;
                          best_flags = flags;
                        }
                        return cost;
                      };
    PlaneSearch out = {0, 0, 0, 0};
    bmf_log("  per-plane trials (whole-image tile, each plane coded on its own):\n");
    for( int32_t pi = 0; pi<plane_count; ++pi ) {
      const int32_t plane_i = plane_desc[pi].src_plane;
      bmf_log("    slot %d = plane %d:\n", pi, plane_i);
      if( out.n_p2 ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(plane_i, try_mode0);
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          best_flags = 0;
      }
      const int32_t bits_f5 = probe_keep(plane_i, try_p1);
      if( bits_f5<best_cost+(best_cost>>kModelShift)||out.n_p2 )
        probe_keep(plane_i, try_p2);
      // The direct context model, offered only to a single 8-bit plane of at
      // most sixteen distinct values -- that is what its context key can hold,
      // and outside that population it loses to the other three by a wide
      // enough margin that the trial is not worth its encode time.
      if( ctx_model_ok(tile_img, tile_buf, plane_i) )
        probe_keep(plane_i, try_ctx);
      int32_t pred;
      if( pi ) {
        probe_keep(plane_i, try_refs);
        const int32_t bits_f13 = probe_keep(plane_i, try_refs_p1);
        pred = best_flags&desc_predictor;
        if( pred==pred_p2||best_cost+(best_cost>>kModelShift)>bits_f13 ) {
          probe_keep(plane_i, try_refs_p2);
          if( won )
            pred = pred_p2;
        }
      } else {
        pred = best_flags&desc_predictor;
      }
      const int32_t flag8 = (uint8_t)best_flags&desc_has_refs;
      out.bits += best_cost;
      out.n_p2 += pred==pred_p2;
      if( pred!=pred_p1 )
        pred = pred_mode0;
      out.n_p1 += pred;
      plane_cost[plane_i] = best_cost;
      out.n_refs += flag8!=0;
      plane_desc[plane_i].flags = (uint8_t)best_flags;
      for( int32_t t = 0; t<trial_n; ++t )
        bmf_log("      %-9s %12.1f bytes%s\n", desc_flags_name(trial_flags[t]),
                bmf_bytes(trial_cost[t]), trial_flags[t]==best_flags ? "  <<" : "");
      trial_n = 0;
    }
    return out;
  }
  void choose_alpha_plane(BmfImage* img, int32_t (&hists)[8*1024], const uint8_t* data_end, int32_t row_stride) {
    const HistRows hr = hist_rows(hists);
    memset(hists, 0, sizeof hists);
    const uint8_t*const px = img[0].pixels;
    const uint8_t*const pp = &px[row_stride];
    const uint32_t uu = &px[row_stride+4]<data_end ? (uint32_t)((data_end-1-pp)/4) : 0;
    const AlphaWeights w = fit_alpha_weights(px, pp, uu);
    int32_t wa_slot = w.a, wb = w.b, wc = w.c;
    for( uint32_t quad = 0; quad<uu; ++quad ) {
      int32_t nx0 = px[4*quad+4];
      int32_t dg0 = px[4*quad]+pp[4*quad+4];
      int32_t dn0 = pp[4*quad];
      int32_t ad0 = nx0+dn0;
      int32_t dnx1 = pp[4*quad+5];
      const int32_t g0 = dg0-ad0;
      int32_t dn1 = pp[4*quad+1];
      int32_t nx2 = px[4*quad+6];
      const int32_t g1 = px[4*quad+1]+dnx1-(px[4*quad+5]+dn1);
      int32_t dnx2 = pp[4*quad+6];
      int32_t g2 = px[4*quad+2]+dnx2-(nx2+pp[4*quad+2]);
      uint16_t g1w = (uint16_t)(px[4*quad+3]+pp[4*quad+7]-pp[4*quad+3]-px[4*quad+7]);
      int16_t g1_lo = (int16_t)g1;
      g1w = g1w-512;
      int32_t pa = g1*wb+g0*wa_slot;
      int32_t pb = g2*wc;
      ++hr.r1[(g1w-(int16_t)g0)&0x3FF];
      int32_t bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
      ++hr.r0[bin_lin];
      ++hr.r2[(g1w-g1_lo)&0x3FF];
      g1w = g1w-g2;
      ++hr.r3[g1w&0x3FF];
      const int32_t alpha = pp[4*quad+7]+256;
      int32_t bin_lin2 = ((uint16_t)alpha-(uint16_t)((wb*pp[4*quad+5]+wa_slot*pp[4*quad+4]+wc*(uint32_t)pp[4*quad+6]+63)>>7)+256)&0x3FF;
      ++hr.r3[bin_lin2+1024];
      int32_t r0 = alpha-pp[4*quad+4];
      ++hr.r3[r0+2048];
      int32_t r1 = alpha-pp[4*quad+5];
      ++hr.r3[r1+3072];
      int32_t r2 = alpha-pp[4*quad+6];
      ++hr.r3[r2+4096];
    }
    uint32_t cost_lin = estimate_cost(hr.r0, 1024);
    uint32_t best4 = (cost_lin>>7)+cost_lin;
    const uint32_t cost_c0 = estimate_cost(hr.r1, 1024);
    const bool pick0 = cost_c0<best4;
    if( pick0 ) {
      best4 = cost_c0;
      wa_slot = 128;
      wc = 0;
      wb = 0;
    }
    const uint32_t cost_c1 = estimate_cost(hr.r2, 1024);
    int32_t pred4 = pick0;
    if( cost_c1<best4 ) {
      best4 = cost_c1;
      pred4 = 2;
      wb = 128;
      wc = 0;
      wa_slot = 0;
    }
    const uint32_t cost_c2 = estimate_cost(hr.r3, 1024);
    if( cost_c2<best4 ) {
      pred4 = 3;
      wc = 128;
      wb = 0;
      wa_slot = 0;
    }
    if( bmf_verbose ) {
      static const char*const mix[4] = {
        "least-squares fit", "copy channel 0", "copy channel 1", "copy channel 2",
      };
      const uint32_t shown[4] = {cost_lin, cost_c0, cost_c1, cost_c2};
      bmf_log("  alpha plane predicted from the pixel's three colour bytes:\n");
      for( int32_t k = 0; k<4; ++k )
        bmf_log("    %-18s %12.1f bytes%s\n", mix[k], bmf_bytes(shown[k]), k==pred4 ? "  <<" : "");
      bmf_log("    weights (%d,%d,%d) fitted, (%d,%d,%d) used\n", w.a, w.b, w.c, wa_slot, wb, wc);
    }
    // Stored in coding-slot order, which is what PlaneTransform::blend reads.
    // The fit is per physical plane: wa for plane 0, wb for 1, wc for 2.
    const int32_t fitted[4] = {wa_slot, wb, wc, 0};
    plane_desc[3].weight0 = fitted[plane_desc[0].src_plane&3];
    plane_desc[3].weight1 = fitted[plane_desc[1].src_plane&3];
    plane_desc[3].weight2 = fitted[plane_desc[2].src_plane&3];
    plane_desc[3].src_plane = 3;
    plane_desc[3].nrefs = 3;
    plane_desc[3].dc = widest_window(hists+1024*(4+pred4), CD_dc_window_lo, CD_dc_window_hi, -1)+1;
  }
  template <int32_t f_DEC> void alt_model_p1_d8(uint8_t* src, int32_t width, int32_t height, uint8_t* out) {
    AltP1Block* blk = alt_p1_block_alloc(width, height);
    blk[0].d8_body<f_DEC>(src, out);
    if( blk ) p1_blocks.give(blk);
  }
  void alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    alt_model_p1_d8<0>(src, i, height, out);
  }
  void alt_model_p1_d8_decode(uint8_t* out, int32_t i, int32_t height) {
    alt_model_p1_d8<1>(nullptr, i, height, out);
  }
  void alt_p2_planes_alloc(AltP2Block** plane, int32_t width) {
    for( int32_t pl = 0; pl<plane_count; ++pl ) {
      AltP2Block* page = p2_blocks.take(true);
      plane[pl] = page[0].alt_p2_alloc(this, width, pl);
    }
  }
  void alt_p2_start_row(AltP2Block** plane, int32_t row, int32_t width) {
    for( int32_t k = 0; k<plane_count; ++k ) {
      if( row==0 )
        plane[k][0].seed_row0(width);
      else if( row==1 )
        plane[k][0].seed_history(width);
      plane[k][0].start_row();
    }
  }
  uint8_t*compress_to_memory(BmfImage* p_i, size_t* out_len, const CodedTail* tail) {
    Init();
    out_len[0] = 0;
    BmfFile arc;
    arc.io = BmfStream::in_memory();
    const int32_t coded = compress_image(&arc, p_i, tail);
    if( !coded ) {
      arc.close();
      return nullptr;
    }
    uint8_t* out = arc.io.take(out_len);
    arc.close();
    return out;
  }
  BmfImage*expand_from_memory(const uint8_t* data, size_t n, CodedTail** tail) {
    Init();
    BmfFile arc;
    arc.io = BmfStream::over_memory(data, n);
    BmfImage* img = expand_image(&arc, &coded_block);
    arc.close();
    if( tail )
      *tail = coded_block;
    else
      free(coded_block);
    coded_block = nullptr;
    return img;
  }
};
