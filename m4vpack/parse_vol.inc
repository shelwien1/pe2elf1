// parse_vol.inc -- ctx.inc's Ctx::parse_vol(): the VOL header (included in Ctx's body).

  // VOL header, as m4vdec's decode_vol_header: records what the VOP syntax depends on
  void parse_vol(GB& gb) {
    int vo_ver_id, w, h, res, k;
    have_vol = 1;
    unsupported = 0;
    dp = 0;
    vol_count++;
    gb.skip(1);                         // random access
    vo_type = (int)gb.get(VO_TYPE_BITS);
    if (vo_type == VO_TYPE_STUDIO || vo_type == VO_TYPE_STUDIO2) { unsupported = "studio profile"; return; }
    if (gb.get1()) {                    // is_ol_id
      vo_ver_id = (int)gb.get(VERID_BITS);
      gb.skip(PRIORITY_BITS);
    } else
      vo_ver_id = 1;
    if (gb.get(ASPECT_BITS) == ASPECT_EXTENDED) gb.skip(PAR_BITS);   // aspect_ratio_info, extended PAR
    if (gb.get1()) {                    // vol_control_parameters
      gb.skip(CHROMA_LOWDELAY_BITS);    // chroma_format, low_delay
      if (gb.get1()) gb.skip(VBV_BITS);
    }
    if (gb.get(SHAPE_BITS)) { unsupported = "non-rectangular shapes"; return; }
    gb.skip(1);
    res = (int)gb.get(TIME_RES_BITS);
    if (!res) { unsupported = "a time resolution of 0"; return; }
    time_res = res;
    time_inc_bits = ilog2((U32)(res - 1)) + 1;
    if (time_inc_bits < 1) time_inc_bits = 1;
    gb.skip(1);
    fixed_inc = gb.get1() ? (int)gb.get(time_inc_bits) : 0;  // fixed_vop_time_increment
    gb.skip(1);
    w = (int)gb.get(VOL_SIZE_BITS);
    gb.skip(1);
    h = (int)gb.get(VOL_SIZE_BITS);
    gb.skip(1);
    if (w && h) { width = w; height = h; }
    interlaced = gb.get1();
    gb.skip(1);                         // obmc disable
    sprite_usage = vo_ver_id == 1 ? gb.get1() : (int)gb.get(SPRITE_BITS);
    warp_points = 0;
    if (sprite_usage == STATIC_SPRITE) unsupported = "static sprites";
    if (sprite_usage == STATIC_SPRITE || sprite_usage == GMC_SPRITE) {
      if (sprite_usage == STATIC_SPRITE) gb.skip(SPRITE_DIM_BITS);
      warp_points = (int)gb.get(WARP_POINTS_BITS);
      if (warp_points > WARP_POINTS_MAX) { warp_points = 0; unsupported = "more than 3 sprite warping points"; }
      warp_acc = (int)gb.get(WARP_ACC_BITS);
      if (gb.get1()) unsupported = "sprite brightness change";
      if (sprite_usage == STATIC_SPRITE) gb.skip(1);
    }
    if (gb.get1()) {                    // not_8_bit
      quant_precision = (int)gb.get(QUANT_PREC_BITS);
      gb.skip(BPP_BITS);                // bits_per_pixel
      if (quant_precision < QUANT_PREC_MIN || quant_precision > QUANT_PREC_MAX)
        quant_precision = QUANT_PREC_DEFAULT;
    } else
      quant_precision = QUANT_PREC_DEFAULT;
    if ((mpeg_quant = gb.get1())) {     // matrices: the defaults, or what follows (the last value repeated)
      for (k = 0; k < BLK_COEFS; k++) {
        intra_matrix[k] = (U16)default_intra_matrix[k];
        inter_matrix[k] = (U16)default_non_intra_matrix[k];
      }
      for (k = 0; k < 2; k++) {
        if (gb.get1()) {
          U16* q = k ? inter_matrix : intra_matrix;
          int i, v, last = 0;
          for (i = 0; i < BLK_COEFS; i++) {
            if (gb.left() < MATRIX_BITS) { unsupported = "a damaged VOL header"; return; }
            if (!(v = (int)gb.get(MATRIX_BITS))) break;
            q[zigzag_direct[i]] = (U16)(last = v);
          }
          for (; i < BLK_COEFS; i++) q[zigzag_direct[i]] = (U16)last;
        }
      }
    }
    qpel = vo_ver_id != 1 ? gb.get1() : 0;
    if (gb.left() < VOL_TAIL_MIN) { unsupported = "a damaged VOL header"; return; }
    cplx_i = cplx_p = cplx_b = 0;
    if (!gb.get1()) {                   // complexity estimation (methods 0 and 1; 1 has two more)
      constexpr int D = DCECS_BITS;
      int pos = gb.pos, method = (int)gb.get(CPLX_METHOD_BITS);
      if (method <= 1) {
        if (!gb.get1()) { for (k = 0; k < CPLX_SHAPE_FLAGS; k++) cplx_i += D * gb.get1(); }
        if (!gb.get1()) {
          cplx_i += D * gb.get1(); cplx_p += D * gb.get1();
          cplx_p += D * gb.get1(); cplx_i += D * gb.get1();
        }
        if (!gb.get1()) { gb.pos = pos; cplx_i = cplx_p = cplx_b = 0; goto no_cplx; }
        if (!gb.get1()) {
          cplx_i += D * gb.get1(); cplx_i += D * gb.get1();
          cplx_i += D * gb.get1(); cplx_i += DCECS_SHORT_BITS * gb.get1();
        }
        if (!gb.get1()) {
          cplx_p += D * gb.get1(); cplx_p += D * gb.get1(); cplx_b += D * gb.get1();
          cplx_p += D * gb.get1(); cplx_p += D * gb.get1(); cplx_p += D * gb.get1();
        }
        if (!gb.get1()) { gb.pos = pos; cplx_i = cplx_p = cplx_b = 0; goto no_cplx; }
        if (method == 1) { cplx_i += D * gb.get1(); cplx_p += D * gb.get1(); }
      }
    }
  no_cplx:
    gb.skip(1);                         // resync_marker_disable
    if ((dp = gb.get1())) gb.skip(1);   // data_partitioned, reversible_vlc
    if (vo_ver_id != 1) {
      if (gb.get1()) { unsupported = "newpred"; gb.skip(NEWPRED_BITS); }
      if (gb.get1()) unsupported = "reduced resolution VOPs";
    }
    if (gb.get1()) {                    // scalability
      int hn, hm, vn, vm;
      gb.skip(HIERARCHY_BITS);
      hn = (int)gb.get(SAMPLING_BITS); hm = (int)gb.get(SAMPLING_BITS);
      vn = (int)gb.get(SAMPLING_BITS); vm = (int)gb.get(SAMPLING_BITS);
      if (hn && hm && vn && vm) unsupported = "scalability";
    }
    mb_w = (width + MB_W - 1) >> MB_SHIFT;
    mb_h = (height + MB_W - 1) >> MB_SHIFT;
    mb_num = mb_w * mb_h;
    if (!mb_num) unsupported = "a frame size of 0";
    mb_num_bits = mb_num > 1 ? ilog2((U32)(mb_num - 1)) + 1 : 1;
  }
