// rc_mb_inter.inc -- recon.inc's rc_mb_inter(): an inter macroblock's prediction.

// An inter macroblock (or a skipped one) of m: its vectors from the differences coded (decode_mb's
// part), then its prediction made in the picture (reconstruct_mb's)
static void rc_mb_inter(Recon* d, const MB* m) {
  const int xy = d->mb_x + d->mb_y * d->mb_stride;
  RPic* cur = d->cur;
  int i, pred_x, pred_y, k = 0;
  d->mb_intra = 0;
  d->mb_skipped = 0;
  if (d->pict_type == PT_P || d->pict_type == PT_S) {
    if (m->sk) {
      d->mv_dir = MV_DIR_FORWARD;
      d->mv_type = MV_16X16;
      if (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE) {
        cur->mbtype[xy] = MBT_SKIP | MBT_GMC | MBT_16x16 | MBT_FWD;
        d->mcsel = 1;
        d->mv[0][0][0] = rc_get_amv(d, 0);
        d->mv[0][0][1] = rc_get_amv(d, 1);
        cur->mbskip[xy] = 0;
      } else {
        cur->mbtype[xy] = MBT_SKIP | MBT_16x16 | MBT_FWD;
        d->mcsel = 0;
        d->mv[0][0][0] = d->mv[0][0][1] = 0;
        cur->mbskip[xy] = 1;
        d->mb_skipped = 1;
      }
    } else {
      d->mcsel = d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE && m->t == 0 ? m->mc == 1 : 0;
      d->mv_dir = MV_DIR_FORWARD;
      if (m->t == 0) {
        if (d->mcsel) {
          cur->mbtype[xy] = MBT_GMC | MBT_16x16 | MBT_FWD;
          d->mv_type = MV_16X16;
          d->mv[0][0][0] = rc_get_amv(d, 0);
          d->mv[0][0][1] = rc_get_amv(d, 1);
        } else if (m->fp == 1) {
          cur->mbtype[xy] = MBT_16x8 | MBT_FWD | MBT_INTERLACED;
          d->mv_type = MV_FIELD;
          d->field_select[0][0] = (m->fs >> 1) & 1;
          d->field_select[0][1] = m->fs & 1;
          rc_pred_motion(d, 0, &pred_x, &pred_y);
          for (i = 0; i < 2; i++) {
            d->mv[0][i][0] = rc_motion(pred_x, m->mv[k++], d->f_code);
            d->mv[0][i][1] = rc_motion(pred_y / 2, m->mv[k++], d->f_code);
          }
        } else {
          cur->mbtype[xy] = MBT_16x16 | MBT_FWD;
          d->mv_type = MV_16X16;
          rc_pred_motion(d, 0, &pred_x, &pred_y);
          d->mv[0][0][0] = rc_motion(pred_x, m->mv[0], d->f_code);
          d->mv[0][0][1] = rc_motion(pred_y, m->mv[1], d->f_code);
        }
      } else {
        cur->mbtype[xy] = MBT_8x8 | MBT_FWD;
        d->mv_type = MV_8X8;
        for (i = 0; i < MB_LUMA; i++) {
          S16* mot_val = rc_pred_motion(d, i, &pred_x, &pred_y);
          d->mv[0][i][0] = rc_motion(pred_x, m->mv[k++], d->f_code);
          d->mv[0][i][1] = rc_motion(pred_y, m->mv[k++], d->f_code);
          mot_val[0] = (S16)d->mv[0][i][0];
          mot_val[1] = (S16)d->mv[0][i][1];
        }
      }
    }
  } else {  // B-VOP
    int mb_type, mx, my;
    d->mcsel = 0;
    if (d->mb_x == 0) {
      for (i = 0; i < 2; i++)
        d->last_mv[i][0][0] = d->last_mv[i][0][1] = d->last_mv[i][1][0] = d->last_mv[i][1][1] = 0;
    }
    if (m->sk) {  // the co-located macroblock skipped
      d->mb_skipped = 1;
      d->mv_dir = MV_DIR_FORWARD;
      d->mv_type = MV_16X16;
      d->mv[0][0][0] = d->mv[0][0][1] = d->mv[1][0][0] = d->mv[1][0][1] = 0;
      cur->mbtype[xy] = MBT_SKIP | MBT_16x16 | MBT_FWD;
    } else {
      if (m->mb == MODB_NONE) mb_type = MBT_DIRECT2 | MBT_SKIP | MBT_BIDIR;
      else {
        static const int mb_type_b_map[] = { MBT_DIRECT2 | MBT_BIDIR, MBT_BIDIR | MBT_16x16, MBT_BWD | MBT_16x16,
                                             MBT_FWD | MBT_16x16 };  // (by MB_DIRECT ... MB_FWD)
        mb_type = mb_type_b_map[m->t & (countof(mb_type_b_map) - 1)];
        if (!(mb_type & MBT_DIRECT2) && m->fp == 1) {
          int b = m->nfs;
          mb_type |= MBT_16x8 | MBT_INTERLACED;
          mb_type &= ~MBT_16x16;
          if (mb_type & MBT_FWD) {
            d->field_select[0][0] = (m->fs >> --b) & 1;
            d->field_select[0][1] = (m->fs >> --b) & 1;
          }
          if (mb_type & MBT_BWD) {
            d->field_select[1][0] = (m->fs >> --b) & 1;
            d->field_select[1][1] = (m->fs >> --b) & 1;
          }
        }
      }
      d->mv_dir = 0;
      if ((mb_type & (MBT_DIRECT2 | MBT_INTERLACED)) == 0) {
        d->mv_type = MV_16X16;
        if (mb_type & MBT_FWD) {
          d->mv_dir = MV_DIR_FORWARD;
          mx = rc_motion(d->last_mv[0][0][0], m->mv[k++], d->f_code);
          my = rc_motion(d->last_mv[0][0][1], m->mv[k++], d->f_code);
          d->last_mv[0][1][0] = d->last_mv[0][0][0] = d->mv[0][0][0] = mx;
          d->last_mv[0][1][1] = d->last_mv[0][0][1] = d->mv[0][0][1] = my;
        }
        if (mb_type & MBT_BWD) {
          d->mv_dir |= MV_DIR_BACKWARD;
          mx = rc_motion(d->last_mv[1][0][0], m->mv[k++], d->b_code);
          my = rc_motion(d->last_mv[1][0][1], m->mv[k++], d->b_code);
          d->last_mv[1][1][0] = d->last_mv[1][0][0] = d->mv[1][0][0] = mx;
          d->last_mv[1][1][1] = d->last_mv[1][0][1] = d->mv[1][0][1] = my;
        }
      } else if (!(mb_type & MBT_DIRECT2)) {
        d->mv_type = MV_FIELD;
        if (mb_type & MBT_FWD) {
          d->mv_dir = MV_DIR_FORWARD;
          for (i = 0; i < 2; i++) {
            mx = rc_motion(d->last_mv[0][i][0], m->mv[k++], d->f_code);
            my = rc_motion(d->last_mv[0][i][1] / 2, m->mv[k++], d->f_code);
            d->last_mv[0][i][0] = d->mv[0][i][0] = mx;
            d->last_mv[0][i][1] = (d->mv[0][i][1] = my) * 2;
          }
        }
        if (mb_type & MBT_BWD) {
          d->mv_dir |= MV_DIR_BACKWARD;
          for (i = 0; i < 2; i++) {
            mx = rc_motion(d->last_mv[1][i][0], m->mv[k++], d->b_code);
            my = rc_motion(d->last_mv[1][i][1] / 2, m->mv[k++], d->b_code);
            d->last_mv[1][i][0] = d->mv[1][i][0] = mx;
            d->last_mv[1][i][1] = (d->mv[1][i][1] = my) * 2;
          }
        }
      }
      if (mb_type & MBT_DIRECT2) {
        if (mb_type & MBT_SKIP) mx = my = 0;
        else {
          mx = rc_motion(0, m->mv[0], 1);
          my = rc_motion(0, m->mv[1], 1);
        }
        d->mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
        mb_type |= rc_set_direct_mv(d, mx, my);
      }
      cur->mbtype[xy] = (U16)mb_type;
    }
  }
  {  // the prediction
    U8* dest_y = cur->y + (size_t)d->mb_y * MB_W * d->ly + d->mb_x * MB_W;
    U8* dest_cb = cur->u + (size_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
    U8* dest_cr = cur->v + (size_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
    int op = (!d->no_rounding || d->pict_type == PT_B) ? OP_PUT : OP_PUT_NORND;
    if (d->mv_dir & MV_DIR_FORWARD) {
      rc_mpv_motion(d, dest_y, dest_cb, dest_cr, 0, d->last, op);
      op = OP_AVG;
    }
    if (d->mv_dir & MV_DIR_BACKWARD) rc_mpv_motion(d, dest_y, dest_cb, dest_cr, 1, d->next, op);
  }
}
