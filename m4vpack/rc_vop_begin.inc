// rc_vop_begin.inc -- recon.inc's rc_vop_begin(): what a VOP's header sets up in the decoder.

// A VOP's header (m4vdec's decode_vop_header and decode_picture): its times (a not-coded VOP's
// too), then, for a coded one, its pictures. d->on: it is reconstructed (a VOL that can be,
// B-VOPs with references and times FFmpeg would decode). -1: no memory for the tables (which the
// run must not go on without: the other direction may have it)
static int rc_vop_begin(Recon* d, const Ctx* cx, const VopHdr* h) {
  int i;
  d->on = 0;
  if (!cx->have_vol || cx->unsupported || cx->width <= 0 || cx->height <= 0) return 0;
  if (!rc_layout(d, cx->width, cx->height)) return -1;
  if (d->vol_seen != cx->vol_count) {  // a VOL header since the last VOP
    d->vol_seen = cx->vol_count;
    d->t_frame = 0;
  }
  if (d->gop_seen != cx->gop_count) {
    d->gop_seen = cx->gop_count;
    d->time_base = cx->gop_time;
  }
  d->progressive = !cx->interlaced;
  d->mpeg_quant = cx->mpeg_quant;
  memcpy(d->intra_matrix, cx->intra_matrix, sizeof d->intra_matrix);
  memcpy(d->inter_matrix, cx->inter_matrix, sizeof d->inter_matrix);
  d->qpel = cx->qpel;
  d->sprite_usage = cx->sprite_usage;
  d->warp_points = cx->warp_points;
  d->warp_acc = cx->warp_acc;
  d->divx_version = cx->divx_version;
  d->divx_build = cx->divx_build;
  d->xvid_build = cx->xvid_build;
  d->lavc_build = cx->lavc_build;
  d->pict_type = h->type;
  d->mcsel = 0;
  if (d->pict_type != PT_B) {
    d->last_time_base = d->time_base;
    d->time_base += h->mtb;
    d->time = d->time_base * (S64)cx->time_res + h->tinc;
    d->pp_time = (U16)(d->time - d->last_non_b_time);
    d->last_non_b_time = d->time;
  } else {
    d->time = (d->last_time_base + h->mtb) * (S64)cx->time_res + h->tinc;
    d->pb_time = (U16)(d->pp_time - (d->last_non_b_time - d->time));
    if (d->pp_time <= d->pb_time || d->pp_time <= d->pp_time - d->pb_time || d->pp_time <= 0) return 0;  // order
    for (i = 0; i < DIRECT_MVS; i++) {
      d->direct_scale_mv[0][i] = (S16)((i - DIRECT_MVS / 2) * d->pb_time / d->pp_time);
      d->direct_scale_mv[1][i] = (S16)((i - DIRECT_MVS / 2) * (d->pb_time - d->pp_time) / d->pp_time);
    }
    if (d->t_frame == 0) d->t_frame = d->pb_time;
    if (d->t_frame == 0) d->t_frame = 1;
    d->pp_field_time = (U16)((rounded_div(d->last_non_b_time, d->t_frame) -
                              rounded_div(d->last_non_b_time - d->pp_time, d->t_frame)) * 2);
    d->pb_field_time = (U16)((rounded_div(d->time, d->t_frame) -
                              rounded_div(d->last_non_b_time - d->pp_time, d->t_frame)) * 2);
    if (d->pp_field_time <= d->pb_field_time || d->pb_field_time <= 1) {
      d->pb_field_time = FIELD_PB_TIME;
      d->pp_field_time = FIELD_PP_TIME;
      if (!d->progressive) return 0;
    }
  }
  if (!h->coded) return 0;
  d->no_rounding = (d->pict_type == PT_P || (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE)) ? h->rnd : 0;
  if (!d->progressive) {
    d->top_field_first = h->tff;
    d->alternate_scan = h->alt;
  } else
    d->alternate_scan = 0;
  if (d->alternate_scan) {
    d->intra_scan = alternate_vertical_scan;
    d->intra_h_scan = alternate_vertical_scan;
  } else {
    d->intra_scan = zigzag_direct;
    d->intra_h_scan = alternate_horizontal_scan;
  }
  d->intra_v_scan = alternate_vertical_scan;
  if (d->pict_type == PT_S) {
    if (d->sprite_usage == GMC_SPRITE) {
      if (!rc_sprite(d, h->warp)) return 0;
    } else {
      memset(d->sprite_offset, 0, sizeof(d->sprite_offset));
      memset(d->sprite_delta, 0, sizeof(d->sprite_delta));
    }
  }
  d->f_code = d->pict_type != PT_I ? h->fcode : 1;
  d->b_code = d->pict_type == PT_B ? h->bcode : 1;
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
  memset(d->cur->mbskip, 0, d->mb_stride * d->mb_h + MB_PAD);
  memset(d->cur->mvbase, 0, (d->b8_stride * d->mb_h * 2 + MV_PAD) * sizeof(*d->cur->mvbase));
  memset(d->mb_done, 0, d->mb_stride * d->mb_h);
  d->mb_x = d->mb_y = 0;
  d->interlaced_dct = 0;
  d->first_slice_line = 1;
  d->resync_mb_x = d->resync_mb_y = 0;
  rc_set_qscale(d, d->qscale);
  d->on = 1;
  return 0;
}
