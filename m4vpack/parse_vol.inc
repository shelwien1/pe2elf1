// parse_vol.inc -- syntax.inc's parse_vol(): the VOL header.

// VOL header, as m4vdec's decode_vol_header: records what the VOP syntax depends on
static void parse_vol(Ctx* c, GB* gb) {
  int vo_ver_id, vo_type, w, h, time_res, k;
  c->have_vol = 1;
  c->unsupported = 0;
  c->dp = 0;
  c->vol_count++;
  gb_skip(gb, 1);                       // random access
  c->vo_type = vo_type = (int)gb_get(gb, VO_TYPE_BITS);
  if (vo_type == VO_TYPE_STUDIO || vo_type == VO_TYPE_STUDIO2) { c->unsupported = "studio profile"; return; }
  if (gb_get1(gb)) {                    // is_ol_id
    vo_ver_id = (int)gb_get(gb, VERID_BITS);
    gb_skip(gb, PRIORITY_BITS);
  } else
    vo_ver_id = 1;
  if (gb_get(gb, ASPECT_BITS) == ASPECT_EXTENDED) gb_skip(gb, PAR_BITS);   // aspect_ratio_info, extended PAR
  if (gb_get1(gb)) {                    // vol_control_parameters
    gb_skip(gb, CHROMA_LOWDELAY_BITS);  // chroma_format, low_delay
    if (gb_get1(gb)) gb_skip(gb, VBV_BITS);
  }
  if (gb_get(gb, SHAPE_BITS)) { c->unsupported = "non-rectangular shapes"; return; }
  gb_skip(gb, 1);
  time_res = (int)gb_get(gb, TIME_RES_BITS);
  if (!time_res) { c->unsupported = "a time resolution of 0"; return; }
  c->time_res = time_res;
  c->time_inc_bits = ilog2((U32)(time_res - 1)) + 1;
  if (c->time_inc_bits < 1) c->time_inc_bits = 1;
  gb_skip(gb, 1);
  c->fixed_inc = gb_get1(gb) ? (int)gb_get(gb, c->time_inc_bits) : 0;  // fixed_vop_time_increment
  gb_skip(gb, 1);
  w = (int)gb_get(gb, VOL_SIZE_BITS);
  gb_skip(gb, 1);
  h = (int)gb_get(gb, VOL_SIZE_BITS);
  gb_skip(gb, 1);
  if (w && h) { c->width = w; c->height = h; }
  c->interlaced = gb_get1(gb);
  gb_skip(gb, 1);                       // obmc disable
  c->sprite_usage = vo_ver_id == 1 ? gb_get1(gb) : (int)gb_get(gb, SPRITE_BITS);
  c->warp_points = 0;
  if (c->sprite_usage == STATIC_SPRITE) c->unsupported = "static sprites";
  if (c->sprite_usage == STATIC_SPRITE || c->sprite_usage == GMC_SPRITE) {
    if (c->sprite_usage == STATIC_SPRITE) gb_skip(gb, SPRITE_DIM_BITS);
    c->warp_points = (int)gb_get(gb, WARP_POINTS_BITS);
    if (c->warp_points > WARP_POINTS_MAX) { c->warp_points = 0; c->unsupported = "more than 3 sprite warping points"; }
    c->warp_acc = (int)gb_get(gb, WARP_ACC_BITS);
    if (gb_get1(gb)) c->unsupported = "sprite brightness change";
    if (c->sprite_usage == STATIC_SPRITE) gb_skip(gb, 1);
  }
  if (gb_get1(gb)) {                    // not_8_bit
    c->quant_precision = (int)gb_get(gb, QUANT_PREC_BITS);
    gb_skip(gb, BPP_BITS);              // bits_per_pixel
    if (c->quant_precision < QUANT_PREC_MIN || c->quant_precision > QUANT_PREC_MAX)
      c->quant_precision = QUANT_PREC_DEFAULT;
  } else
    c->quant_precision = QUANT_PREC_DEFAULT;
  if ((c->mpeg_quant = gb_get1(gb))) {  // matrices: the defaults, or what follows (the last value repeated)
    for (k = 0; k < BLK_COEFS; k++) {
      c->intra_matrix[k] = (U16)default_intra_matrix[k];
      c->inter_matrix[k] = (U16)default_non_intra_matrix[k];
    }
    for (k = 0; k < 2; k++) {
      if (gb_get1(gb)) {
        U16* q = k ? c->inter_matrix : c->intra_matrix;
        int i, v, last = 0;
        for (i = 0; i < BLK_COEFS; i++) {
          if (gb_left(gb) < MATRIX_BITS) { c->unsupported = "a damaged VOL header"; return; }
          if (!(v = (int)gb_get(gb, MATRIX_BITS))) break;
          q[zigzag_direct[i]] = (U16)(last = v);
        }
        for (; i < BLK_COEFS; i++) q[zigzag_direct[i]] = (U16)last;
      }
    }
  }
  c->qpel = vo_ver_id != 1 ? gb_get1(gb) : 0;
  if (gb_left(gb) < VOL_TAIL_MIN) { c->unsupported = "a damaged VOL header"; return; }
  c->cplx_i = c->cplx_p = c->cplx_b = 0;
  if (!gb_get1(gb)) {                   // complexity estimation (methods 0 and 1; 1 has two more)
    constexpr int D = DCECS_BITS;
    int pos = gb->pos, method = (int)gb_get(gb, CPLX_METHOD_BITS);
    if (method <= 1) {
      if (!gb_get1(gb)) { for (k = 0; k < CPLX_SHAPE_FLAGS; k++) c->cplx_i += D * gb_get1(gb); }
      if (!gb_get1(gb)) {
        c->cplx_i += D * gb_get1(gb); c->cplx_p += D * gb_get1(gb);
        c->cplx_p += D * gb_get1(gb); c->cplx_i += D * gb_get1(gb);
      }
      if (!gb_get1(gb)) { gb->pos = pos; c->cplx_i = c->cplx_p = c->cplx_b = 0; goto no_cplx; }
      if (!gb_get1(gb)) {
        c->cplx_i += D * gb_get1(gb); c->cplx_i += D * gb_get1(gb);
        c->cplx_i += D * gb_get1(gb); c->cplx_i += DCECS_SHORT_BITS * gb_get1(gb);
      }
      if (!gb_get1(gb)) {
        c->cplx_p += D * gb_get1(gb); c->cplx_p += D * gb_get1(gb); c->cplx_b += D * gb_get1(gb);
        c->cplx_p += D * gb_get1(gb); c->cplx_p += D * gb_get1(gb); c->cplx_p += D * gb_get1(gb);
      }
      if (!gb_get1(gb)) { gb->pos = pos; c->cplx_i = c->cplx_p = c->cplx_b = 0; goto no_cplx; }
      if (method == 1) { c->cplx_i += D * gb_get1(gb); c->cplx_p += D * gb_get1(gb); }
    }
  }
no_cplx:
  gb_skip(gb, 1);                       // resync_marker_disable
  if ((c->dp = gb_get1(gb))) gb_skip(gb, 1);   // data_partitioned, reversible_vlc
  if (vo_ver_id != 1) {
    if (gb_get1(gb)) { c->unsupported = "newpred"; gb_skip(gb, NEWPRED_BITS); }
    if (gb_get1(gb)) c->unsupported = "reduced resolution VOPs";
  }
  if (gb_get1(gb)) {                    // scalability
    int hn, hm, vn, vm;
    gb_skip(gb, HIERARCHY_BITS);
    hn = (int)gb_get(gb, SAMPLING_BITS); hm = (int)gb_get(gb, SAMPLING_BITS);
    vn = (int)gb_get(gb, SAMPLING_BITS); vm = (int)gb_get(gb, SAMPLING_BITS);
    if (hn && hm && vn && vm) c->unsupported = "scalability";
  }
  c->mb_w = (c->width + MB_W - 1) >> MB_SHIFT;
  c->mb_h = (c->height + MB_W - 1) >> MB_SHIFT;
  c->mb_num = c->mb_w * c->mb_h;
  if (!c->mb_num) c->unsupported = "a frame size of 0";
  c->mb_num_bits = c->mb_num > 1 ? ilog2((U32)(c->mb_num - 1)) + 1 : 1;
}
