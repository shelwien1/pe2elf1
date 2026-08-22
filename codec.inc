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
    int32_t cur0, cur1, cur3;
    int32_t want2;
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
  int32_t choose_plane_coding(BmfImage* img) {
    alignas(16) int32_t hists[8*1024];
    const HistRows hr = hist_rows(hists);
    int32_t*const hist_flat = hr.r0, *const hist_a = hr.r1;
    int32_t*const hist_b = hr.r2, *const hist_c = hr.r3;
    uint32_t cand_cost[3][4];
    PlaneDesc cand_desc[3][4];
    uint32_t next_plane, row;
    int32_t result, xform, wt8, wt4, pred;
    uint32_t best_cost, best;
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
      if( n_planes>=3 ) {
        xform = 0;
        uint32_t cheapest = cost_candidate(img, 0, cand_desc[0], cand_cost[0]);
        for( int32_t cand = 1; cand<3; ++cand ) {
          const uint32_t c = cost_candidate(img, cand, cand_desc[cand], cand_cost[0]);
          if( c<cheapest ) {
            cheapest = c;
            xform = cand;
          }
        }
        memcpy(plane_desc, cand_desc[xform], sizeof cand_desc[xform]);
        best_cost = cand_cost[xform][0];
        int32_t plane_a = plane_desc[0].src_plane-xform;
        wt8 = plane_desc[xform].weight1;
        int32_t plane_b = plane_desc[1].src_plane-xform;
        wt4 = plane_desc[xform].weight0;
        {
          const uint8_t* plane0 = &img[0].pixels[xform];
          WeightSearch search = {this, img, plane0, plane_a, plane_b, {wt4, wt8}, best_cost};
          int32_t wt_step = 4;
          search.descend(wt_step, -1);
          search.descend(wt_step, +1);
          wt8 = search.w[1];
          wt4 = search.w[0];
          best_cost = search.cost;
        }
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
            int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
            ++hist_flat[bin0];
            ++hist_a[c0-c1];
            ++hist_b[c0-c2];
            ++hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
          } while( px<data_end );
        }
        const uint32_t slack = umin32(best_cost>>7, kSlackMax);
        best = slack+best_cost;
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
        plane_desc[xform].dc = widest_window(hists+1024*pred, 4, 1024, -1)+1;
        result = widest_window(hist_c+1024, 0, 512, 255)+1;
        plane_desc[plane_b+xform].dc = result;
        if( n_planes>=4 ) {
          choose_alpha_plane(img, hists, data_end, row_stride);
        }
      }
    }
    return result;
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
          *side++ = (uint8_t)(*p-t.dc-blend);
      }
    }
  }
  template <int32_t f_DEC, typename T> void code_field(T &v, int32_t bits, int32_t bias) {
    if constexpr( f_DEC )
      v = (T)((int32_t)unpack_bits(bits)-bias);
    else
      pack_bits((uint32_t)((int32_t)v+bias), bits);
  }
  void code_image_body(BmfImage* p_i, BmfImage &hdr) {
    if( (p_i[0].depth&depth_bits)<=4 ) {
      stream_open(p_i[0].data_size);
      plane_predictor = 0;
      plane_alt_model = 0;
      alphabet_reduced = 0;
      model_plane(p_i, p_i[0].pixels, p_i[0].pixels);
      return;
    }
    const uint32_t filtered = search_filter(p_i);
    hdr.flags |= flags_descriptors;
    const uint32_t data_bytes = p_i[0].data_size;
    if( p_i[0].flags&flags_transposed ) {
      hdr.header_from(p_i);
      hdr.flags = flags_coded|flags_descriptors|flags_slow|p_i[0].flags;
    }
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
      code_field<f_DEC>(d.weight0, 8, 64);
      code_field<f_DEC>(d.weight1, 8, 64);
      if( d.nrefs>2u )
        code_field<f_DEC>(d.weight2, 8, 64);
    }
  }
  void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane) {
    code_colour_plane<0>(img, dst, plane);
  }
  int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
    BmfImage hdr;
    if( !arc_in[0].io.fp&&!arc_in[0].io.buf )
      return 0;
    if( extra_blk )
      p_i[0].flags |= flags_tail;
    near_lossless_q = 0;
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
      if( (uint32_t)coded_bytes<p_i[0].data_size ) {
        const uint8_t ok = write_member_head(&hdr, extra_blk, &arc_in[0].io);
        int32_t ok_all = arc_in[0].io.write_exact(stream.buf, (size_t)coded_bytes)&ok;
        free(stream.buf);
        if( ok_all&&(p_i[0].depth&depth_palette)!=0 )
          arc_in[0].io.write_all(&p_i[0].pixels[p_i[0].data_size], pal_bytes);
        arc_in[0].io.flush();
        if( ok_all )
          return (int32_t)hdr.data_size;
        return ok_all;
      }
      free(stream.buf);
      if( p_i[0].flags&flags_transposed )
        transpose_image(p_i, plane_count);
    }
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
        int32_t dz = diag-(*q+west);
        sxz = sxz+(double)dx*(double)dz;
        ++hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
        syz = syz+(double)dy*(double)dz;
        ++hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
        int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;
        ++hist_zp[bin];
      } while( p<img_end );
    }
    double inv = 128.0/(0.1-sxy*sxy+sxx*syy);
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
    stream.size = hdr.data_size;
    stream.buf = (uint8_t*)bmf_new(hdr.data_size);
    packer_reset();
    uint32_t want = hdr.data_size;
    bool ok = io[0].read(stream.buf, hdr.data_size)==want;
    if( ok ) {
      if( (img_at[0].depth&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
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
    uint32_t pal_bytes = 3<<(hdr.depth&31);
    if( !(hdr.depth&depth_palette) )
      pal_bytes = 0;
    if( !hdr.width||!hdr.height||(uint32_t)hdr.width*(uint32_t)((((hdr.depth&depth_bits)+7)>>3))>0xFFFFu ) {
      return arc_in[0].fail();
    }
    BmfImage* img_at = alloc_image(hdr.width, hdr.height, hdr.depth&depth_bits, (hdr.depth&depth_palette)!=0, 1);
    img_at[0].depth = hdr.depth;
    uint8_t has_coded = p_coded_buf&&p_coded_buf[0];
    img_at[0].flags |= (hdr.flags&flags_transposed)|(has_coded ? flags_tail : 0);
    plane_count = ((hdr.depth&depth_bits)+7)>>3;
    if( plane_count<1||plane_count>4 ) {
      return arc_in[0].fail();
    }
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
      uint8_t* pal_at = (img_at[0].depth&depth_palette) ? img_at[0].palette() : nullptr;
      uint32_t got = (uint32_t)io[0].read(pal_at, pal_bytes);
      if( got!=pal_bytes ) {
        return arc_in[0].fail();
      }
    }
    if( img_at[0].flags&flags_transposed ) {
      if( (uint64_t)img_at[0].width*img_at[0].height*(uint32_t)plane_count>(uint64_t)img_at[0].data_size ) {
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
      *stream.pk.word = stream.pk.acc|(2*(word<<((31-stream.pk.free_bits)&31)));
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
    *stream.pk.word = stream.pk.acc;
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
    uint32_t w = *(uint32_t*)stream.cur;
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
    t.mode = plane_desc[plane].nrefs;
    t.dc = plane_desc[plane].dc;
    t.wgt1 = plane_desc[plane].weight1;
    t.wgt0 = plane_desc[plane].weight0;
    t.wgt2 = plane_desc[plane].weight2;
    t.by_weights = false;
    if( t.mode==2&&t.wgt0+t.wgt1==128 ) {
      if( !t.wgt1 ) {
        t.by_weights = true;
      } else if( !t.wgt0 ) {
        t.to_ref0 = plane_desc[1].src_plane-plane;
        t.by_weights = true;
      }
    }
    return t;
  }
  uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height) {
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
        const int32_t code = (uint8_t)fold[(uint8_t)(*p-pred)];
        p[0] = code;
        hist_bump(code);
      }
      --up;
      --p;
      last = (uint8_t)fold[(uint8_t)(*p-p[-width])];
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
        last = (uint8_t)(*q-*((int8_t*)q-1));
        q[0] = fold[(uint8_t)last];
      }
    }
    return last;
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
  uint32_t search_filter(BmfImage* img) {
    PlaneDesc saved[4];
    uint32_t plane_cost[4];
    uint8_t* tile_buf;
    int32_t tile_h, y0, cand, pl_k;
    int32_t tile_w = img[0].width;
    tile_h = img[0].height;
    if( tile_w<4||tile_h<3 ) {
      reset_descriptors();
      return 0;
    }
    choose_plane_coding(img);
    BmfImage* tile_img = alloc_image(tile_w, tile_h, img[0].depth&depth_bits, 0, 0);
    stream_open(tile_img[0].data_size);
    tile_buf = (uint8_t*)bmf_new(tile_h*tile_w);
    y0 = (img[0].height-tile_h)>>1;
    int32_t dx = img[0].width-tile_w;
    int32_t off_y = y0*img[0].stride;
    uint8_t* srcp = img[0].pixels+plane_count*(dx>>1)+off_y;
    uint8_t* tile_src = tile_img[0].pixels;
    if( tile_h>0 ) {
      int32_t row_bytes = tile_img[0].stride;
      uint8_t* dstp = tile_img[0].pixels;
      for( int32_t y = 0; y<tile_h; ++y ) {
        memcpy(dstp, srcp, row_bytes);
        dstp += row_bytes;
        srcp += img[0].stride;
      }
    }
    PlaneSearch ps = search_planes(tile_img, tile_buf, plane_cost);
    int32_t best_bits = ps.bits;
    {
      transpose_image(tile_img, plane_count);
      int32_t bits_d = 0;
      int32_t pk = 0;
      while( pk<plane_count ) {
        pl_k = plane_desc[pk].src_plane;
        model_planes(tile_img, tile_buf, pl_k);
        const int32_t bits = 8*(stream.cur-stream.buf);
        packer_flush();
        bits_d += bits;
        packer_reset();
        if( (uint32_t)(bits-(bits>>8))>plane_cost[pl_k] )
          break;
        ++pk;
      }
      if( pk<plane_count )
        bits_d += best_bits+1;
      if( bits_d+(bits_d>>12)>=best_bits ) {
        transpose_image(tile_img, plane_count);
      } else {
        best_bits = bits_d;
        transpose_image(img, plane_count);
      }
    }
    free(tile_buf);
    if( plane_count>2 ) {
      if( ps.n_p1 ) {
        save_descriptors(saved);
        offer_to_all(try_p1);
        int32_t bits_e = transform_cost(tile_img);
        if( bits_e<=best_bits ) {
          best_bits = bits_e;
          cand = 0;
        } else {
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
          cand = 0;
        } else {
          const int32_t bits_a = transform_cost(tile_img);
          if( bits_a<=best_bits ) {
            best_bits = bits_a;
            ps.n_p2 = bits_a;
            if( plane_count-1==ps.n_refs ) {
              cand = 0;
            } else {
              save_descriptors(saved);
              allow_refs_where_present();
              int32_t bits2 = transform_cost(tile_img);
              if( bits2>bits_a )
                restore_descriptors(saved);
              cand = 0;
            }
          } else {
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
        restore_descriptors(saved);
      } else {
        best_bits = bits_b;
        cand = 0;
      }
      if( ps.n_refs+ps.n_p1 ) {
        save_descriptors(saved);
        clear_flags_on_all();
        memcpy(tile_src, tile_copy, tile_img[0].data_size);
        int32_t bits_c = transform_cost(tile_img);
        if( bits_c<=best_bits ) {
          cand = 0;
        } else {
          restore_descriptors(saved);
        }
      }
      free(tile_copy);
    }
    free(stream.buf);
    free((uint8_t*)tile_img);
    return cand;
  }
  void stream_open(uint32_t data_size) {
    stream.size = data_size+0x20000;
    stream.buf = (uint8_t*)bmf_new(stream.size);
    packer_reset();
  }
  int32_t transform_cost(BmfImage* tile_img) {
    transform_planes(tile_img);
    const int32_t bits = 8*(stream.cur-stream.buf);
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
  int32_t weight_pair_cost(const BmfImage* img, const uint8_t* plane0, int32_t plane_a, int32_t plane_b, int32_t w4, int32_t w8) {
    int32_t hist[512];
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
    return estimate_cost(hist, 512);
  }
  PlaneSearch search_planes(BmfImage* tile_img, uint8_t* tile_buf, uint32_t* plane_cost) {
    int32_t best_cost = 0, best_flags = 0;
    auto probe_plane = [&](int32_t pl, int32_t flags) {
                         plane_desc[pl].flags = flags;
                         model_planes(tile_img, tile_buf, pl);
                         int32_t cost = 8*(stream.cur-stream.buf);
                         packer_flush();
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
    for( int32_t pi = 0; pi<plane_count; ++pi ) {
      const int32_t plane_i = plane_desc[pi].src_plane;
      if( out.n_p2 ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(plane_i, try_mode0);
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          best_flags = 0;
      }
      const int32_t bits_f5 = probe_keep(plane_i, try_p1);
      if( bits_f5<best_cost+(best_cost>>5)||out.n_p2 )
        probe_keep(plane_i, try_p2);
      int32_t pred;
      if( pi ) {
        probe_keep(plane_i, try_refs);
        const int32_t bits_f13 = probe_keep(plane_i, try_refs_p1);
        pred = best_flags&desc_predictor;
        if( pred==pred_p2||best_cost+(best_cost>>5)>bits_f13 ) {
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
    plane_desc[3].weight0 = wa_slot;
    plane_desc[3].weight1 = wb;
    plane_desc[3].weight2 = wc;
    plane_desc[3].src_plane = 3;
    plane_desc[3].nrefs = 3;
    plane_desc[3].dc = widest_window(hists+1024*(4+pred4), 4, 1024, -1)+1;
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
    BMFState::Init();
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
    BMFState::Init();
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
