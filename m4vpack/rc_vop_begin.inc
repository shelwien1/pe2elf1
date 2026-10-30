// rc_vop_begin.inc -- recon.inc's rc_vop_begin(): what a VOP's header sets up in the decoder.

// A VOP's header (m4vdec's decode_vop_header and decode_picture), after the motion state's (its
// times and warp): for a coded VOP its pictures. d->on: it is reconstructed (a VOL that can be,
// B-VOPs with references and times FFmpeg would decode). -1: no memory for the tables (which the
// run must not go on without: the other direction may have it)
static int rc_vop_begin(Recon* d, const Ctx* cx, const VopHdr* h) {
  const Motion* o = d->mo;
  d->on = 0;
  if (!cx->have_vol || cx->unsupported || cx->width <= 0 || cx->height <= 0) return 0;
  if (!rc_layout(d)) return -1;
  d->progressive = !cx->interlaced;
  d->mpeg_quant = cx->mpeg_quant;
  memcpy(d->intra_matrix, cx->intra_matrix, sizeof d->intra_matrix);
  memcpy(d->inter_matrix, cx->inter_matrix, sizeof d->inter_matrix);
  d->qpel = cx->qpel;
  d->divx_version = cx->divx_version;
  d->divx_build = cx->divx_build;
  d->xvid_build = cx->xvid_build;
  d->lavc_build = cx->lavc_build;
  d->pict_type = h->type;
  if (d->pict_type == PT_B && !o->times_ok) return 0;  // (out of order)
  if (!h->coded) return 0;
  d->no_rounding = (d->pict_type == PT_P || (d->pict_type == PT_S && o->sprite_usage == GMC_SPRITE)) ? h->rnd : 0;
  d->alternate_scan = !d->progressive ? h->alt : 0;
  if (d->alternate_scan) {
    d->intra_scan = alternate_vertical_scan;
    d->intra_h_scan = alternate_vertical_scan;
  } else {
    d->intra_scan = zigzag_direct;
    d->intra_h_scan = alternate_horizontal_scan;
  }
  d->intra_v_scan = alternate_vertical_scan;
  if (d->pict_type == PT_S && o->sprite_usage == GMC_SPRITE && !o->sprite_ok) return 0;  // (its warp overflows)
  d->qscale = d->chroma_qscale = h->quant;
  rc_workaround_bugs(d);
  if (d->bugs & BUG_EDGE) {
    d->h_edge = d->width;
    d->v_edge = d->height;
  } else {
    d->h_edge = d->mb_w * MB_W;
    d->v_edge = d->mb_h * MB_W;
  }
  if (d->pict_type == PT_B && !d->last) return 0;  // no forward reference yet
  if (d->pict_type != PT_B) {
    d->last = d->next;
    d->cur = rc_free_pic(d);
    d->next = d->cur;
  } else
    d->cur = rc_free_pic(d);
  if (!d->last && d->pict_type != PT_I) d->last = rc_gray_pic(d);
  memset(d->mb_done, 0, d->mb_stride * d->mb_h);
  d->mb_x = d->mb_y = 0;
  d->interlaced_dct = 0;
  rc_set_qscale(d, d->qscale);
  d->on = 1;
  return 0;
}
