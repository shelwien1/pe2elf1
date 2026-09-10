struct oc_floor {
  u8 pcl[VB_MAXPOST], ppos[VB_MAXPOST];
  u8 rnk[VB_MAXPOST];
  u8 lo[VB_MAXPOST], hi[VB_MAXPOST];
};
static oc_floor oc_fl[VB_MAXFLOOR];
static void oc_floor_tables(const vb_floor *f, oc_floor *d) {
  u32 i, j, k, posts = 2;
  memset(d, 0, sizeof *d);
  for(i = 0; i < f->parts; i++)
    for(j = 0; j < f->cdim[f->pcls[i]]; j++) {
      if(posts >= VB_MAXPOST)
        break;
      d->pcl[posts] = f->pcls[i];
      d->ppos[posts] = (u8)j;
      posts++;
    }
  for(i = 0; i < f->posts; i++)
    d->rnk[f->srt[i]] = (u8)i;
  for(i = 0; i + 2 < f->posts; i++) {
    u32 l = 0, h = 1, lx = 0, hx = f->x[1], cx = f->x[i + 2];
    for(k = 0; k < i + 2; k++) {
      if(f->x[k] > lx && f->x[k] < cx) {
        l = k;
        lx = f->x[k];
      }
      if(f->x[k] < hx && f->x[k] > cx) {
        h = k;
        hx = f->x[k];
      }
    }
    d->lo[i] = (u8)l;
    d->hi[i] = (u8)h;
  }
}
static i32 vd_render(i32 x0, i32 x1, i32 y0, i32 y1, i32 X) {
  i32 dy = y1 - y0, adx = x1 - x0;
  i32 ady = dy < 0 ? -dy : dy;
  i32 off;
  if(adx <= 0)
    return y0;
  off = (i32)(((i64)ady * (X - x0)) / adx);
  return dy < 0 ? y0 - off : y0 + off;
}
static i32 vd_fold(i32 y, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 d = y - pred;
  i32 a = d >= 0 ? 2 * d : -2 * d - 1;
  if(!d)
    return 0;
  if(a < room)
    return a;
  return hiroom > lowroom ? d + lowroom : hiroom - 1 - d;
}
static i32 vd_unfold(i32 val, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 y;
  if(!val)
    y = pred;
  else if(val >= room)
    y = hiroom > lowroom ? val - lowroom + pred : pred - val + hiroom - 1;
  else if(val & 1)
    y = pred - ((val + 1) >> 1);
  else
    y = pred + (val >> 1);
  return y < 0 ? 0 : y > quant - 1 ? quant - 1 : y;
}
static u8 tc_used[VB_MAXCH];
static u8 oc_fno[VB_MAXCH];
static i16 fl_cur[VB_MAXPOST];
static i16 fl_fy[VB_MAXPOST];
static void oc_flr(u32 k, u32 fno, const vb_floor *f, u32 *y, int &used) {
  const oc_floor *fd = oc_fl + fno;
  sz row = (sz)tc_blk * tc_nchan + k;
  i16 *hp = fl_hist + row * VB_MAXPOST;
  i16 *hy = fl_yhist + row * VB_MAXPOST;
  int mine = fl_who[row] == (i32)fno;
  int hasn = k && tc_used[k - 1] && fl_who[row - 1] == (i32)oc_fno[k - 1];
  u32 u, p;
  i32 quant = (i32)f->quant;
  oc_fno[k] = (u8)fno;
  tc_stage = STG_AUD;
  u = (u32)tc_auxc(F_USED, tc_used[k], tc_enc ? (i64)used : 0, 0);
  FATAL_UNLESS(u < 2, "coded stream: flr.used is %u", u);
  tc_used[k] = (u8)u;
  used = (int)u;
  if(!u)
    return;
  tc_stage = STG_FLOOR;
  fam_flr.pc.off();
  if(tc_enc)
    for(p = 0; p < f->posts; p++)
      fl_cur[p] = (i16)y[p];
  for(p = 0; p < f->posts; p++) {
    tcx v;
    i64 yy;
    i32 pred = 0, room = 0, hl = 0, lov = 0, hiv = 0, ep = 0;
    i32 o1 = hasn ? hp[p - (sz)VB_MAXPOST] : 0;
    i32 od = hasn && mine ? hy[p - (sz)VB_MAXPOST] - hy[p] : 0;
    if(p >= 2) {
      u32 l = fd->lo[p - 2], h = fd->hi[p - 2];
      i32 hiroom, lowroom;
      u32 c = fd->pcl[p];
      if(!fd->ppos[p])
        fam_flr.pc.start(tcp_flr[fno][c].ok ? &tcp_flr[fno][c] : nullptr);
      pred = vd_render((i32)f->x[l], (i32)f->x[h], fl_fy[l], fl_fy[h], (i32)f->x[p]);
      if(pred < 0)
        pred = 0;
      if(pred > quant - 1)
        pred = quant - 1;
      hiroom = quant - pred;
      lowroom = pred;
      room = (hiroom < lowroom ? hiroom : lowroom) << 1;
      hl = hiroom > lowroom ? 1 : hiroom < lowroom ? 2 : 0;
      lov = fl_cur[l];
      hiv = fl_cur[h];
      ep = mine ? vd_fold(hy[p], pred, quant) : 0;
      if(ep < 0)
        ep = 0;
    }
    tc_make_flr((int)fno, (int)fd->rnk[p], tc_qlog(mine ? hp[p] : 0), tc_qlog(mine ? hy[p] : 0), p < 2, (int)tc_blk, tc_qlog(o1), tc_qlog(pred), tc_sq(od), (int)k, tc_qlog(room), hl, tc_qlog(lov), tc_qlog(hiv), tcp_axis(fam_flr.pc), tc_qlog(ep), v);
    yy = tc_enc ? fl_cur[p] : 0;
    yy = fam_flr.codes(v, yy, 0);
    FATAL_UNLESS(yy >= 0 && yy < 32768, "coded stream: floor post %" PRId64, yy);
    fl_cur[p] = (i16)yy;
    fl_fy[p] = (i16)(p < 2 ? (yy > quant - 1 ? quant - 1 : (i32)yy) : vd_unfold((i32)yy, pred, quant));
    if(fam_flr.pc.on)
      fam_flr.pc.step(yy >= 0 && yy < (i64)fam_flr.pc.t->nsym ? (int)yy : -1);
    if(tc_verbose)
      tc_syms[STG_FLOOR]++;
  }
  if(!tc_enc)
    for(p = 0; p < f->posts; p++)
      y[p] = (u32)fl_cur[p];
  for(p = 0; p < f->posts; p++) {
    hp[p] = fl_cur[p];
    hy[p] = fl_fy[p];
  }
  fl_who[row] = (i32)fno;
}
