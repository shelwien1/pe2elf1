// rc_vop_begin.inc -- recon.inc's Recon::vop_begin(): what a VOP's header sets up in the decoder
// (included in Recon's body).

  // A VOP's header (m4vdec's decode_vop_header and decode_picture), after the motion state's (its
  // times and warp): for a coded VOP its pictures. on: it is reconstructed (a VOL that can be,
  // B-VOPs with references and times FFmpeg would decode). -1: no memory for the tables (which the
  // run must not go on without: the other direction may have it)
  int vop_begin(const Ctx& cx, const VopHdr& h) {
    const Motion& o = *mo;
    on = 0;
    if (!cx.have_vol || cx.unsupported || cx.width <= 0 || cx.height <= 0) return 0;
    if (!layout()) return -1;
    progressive = !cx.interlaced;
    mpeg_quant = cx.mpeg_quant;
    memcpy(intra_matrix, cx.intra_matrix, sizeof intra_matrix);
    memcpy(inter_matrix, cx.inter_matrix, sizeof inter_matrix);
    qpel = cx.qpel;
    divx_version = cx.divx_version;
    divx_build = cx.divx_build;
    xvid_build = cx.xvid_build;
    lavc_build = cx.lavc_build;
    pict_type = h.type;
    if (pict_type == PT_B && !o.times_ok) return 0;  // (out of order)
    if (!h.coded) return 0;
    no_rounding = (pict_type == PT_P || (pict_type == PT_S && o.sprite_usage == GMC_SPRITE)) ? h.rnd : 0;
    alternate_scan = !progressive ? h.alt : 0;
    if (alternate_scan) {
      intra_scan = alternate_vertical_scan;
      intra_h_scan = alternate_vertical_scan;
    } else {
      intra_scan = zigzag_direct;
      intra_h_scan = alternate_horizontal_scan;
    }
    intra_v_scan = alternate_vertical_scan;
    if (pict_type == PT_S && o.sprite_usage == GMC_SPRITE && !o.sprite_ok) return 0;  // (its warp overflows)
    qscale = chroma_qscale = h.quant;
    workaround_bugs();
    if (bugs & BUG_EDGE) {
      h_edge = width;
      v_edge = height;
    } else {
      h_edge = mb_w * MB_W;
      v_edge = mb_h * MB_W;
    }
    if (pict_type == PT_B && !last.y) return 0;  // no forward reference yet
    if (pict_type != PT_B) {
      last = next;
      cur = free_pic();
      next = cur;
    } else
      cur = free_pic();
    if (!last.y && pict_type != PT_I) last = gray_pic();
    memset(mb_done, 0, mb_stride * mb_h);
    mb_x = mb_y = 0;
    interlaced_dct = 0;
    set_qscale(qscale);
    on = 1;
    return 0;
  }
