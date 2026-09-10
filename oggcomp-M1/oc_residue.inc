static INLINE u32 tc_gcd(u32 a, u32 b) {
  while(b) {
    u32 t = a % b;
    a = b;
    b = t;
  }
  return a;
}
void oc_model::part_begin(u32 rno, u32 pss, u32 jv, u32 pc, const vb_res *r, const vb_book *bk, u32 bn, u32 cls) {
  u32 j = R.cidx[jv];
  sz sl = (sz)jv * R.w + pc;
  sz span = hist.dg_span[rno];
  sz hb = ((sz)(hist.tc_blk * 8 + pss) * hist.tc_nchan + j) * span;
  sz base = (sz)((rno * 8 + pss) * hist.tc_nchan + j);
  u32 dim = bk->dim, ilv = R.ilv;
  P.rno = rno;
  P.pss = pss;
  P.j = j;
  P.pc = pc;
  P.psz = r->psz;
  P.cls = cls;
  P.dim = dim;
  P.ilv = ilv;
  P.i = 0;
  P.vmod = 0;
  P.smod = ilv ? (u32)(((sz)pc * r->psz) % ilv) : 0;
  P.bkq = tc_qlog(bk->off);
  P.clrun = sl < sizeof hist.cl_run_buf ? hist.cl_run_buf[sl] : 0;
  P.clmatch = sl < sizeof hist.cl_same ? hist.cl_same[sl] : 0;
  P.span = span;
  P.hist = span ? hist.dg_hist[rno] + hb : nullptr;
  P.hist2 = span ? hist.dg_hist2[rno] + hb : nullptr;
  P.avg = span ? hist.dg_avg[rno] + hb : nullptr;
  P.pp = span ? hist.dg_pp[rno] + (sz)j * span : nullptr;
  P.ps = span ? hist.dg_ps[rno] + (sz)j * span : nullptr;
  P.pn = span ? hist.dg_pn[rno] + (sz)j * span : nullptr;
  P.q1 = hist.dg_q1 + base;
  P.q2 = hist.dg_q2 + base;
  P.zr = hist.dg_zr + base;
  P.str = ilv && dim ? dim / tc_gcd(dim, ilv) * ilv : dim;
  P.bk = bk;
  P.pt = tcp.book[bn].ok ? &tcp.book[bn] : nullptr;
  tc_stage = STG_DIGIT;
  fam_dig.pc.off();
}
i64 oc_model::digit(i64 dg) {
  u32 i = P.i++;
  sz slot = (sz)P.pc * P.psz + i;
  int ok = P.hist && slot < P.span;
  tcx v;
  tc_dv d;
  i32 t1 = ok ? P.hist[slot] : 0;
  i32 t2 = ok ? P.hist2[slot] : 0;
  i32 n1 = ok && slot + P.str < P.span ? P.hist[slot + P.str] : 0;
  i32 w1 = ok && slot >= P.str ? P.hist[slot - P.str] : 0;
  sz xs = P.ilv > 1 ? slot - P.smod + (P.smod ? P.smod - 1 : P.ilv - 1) : 0;
  i32 x1 = ok && P.ilv > 1 && xs < P.span ? P.hist[xs] : 0;
  i32 av = ok ? P.avg[slot] : 0;
  i32 p0 = ok ? P.pp[slot] : 0;
  i32 psum = ok ? P.ps[slot] : 0;
  i32 pnn = ok ? P.pn[slot] : 0;
  d.rno = (int)P.rno;
  d.pass = (int)P.pss;
  d.band = tc_qlog(P.pc);
  d.col = tc_qlog(i);
  d.vpos = (int)P.vmod;
  d.q1 = tc_qlog(*P.q1);
  d.q2 = tc_qlog(*P.q2);
  d.q1s = tc_sq(*P.q1);
  d.q2s = tc_sq(*P.q2);
  d.t1 = tc_qlog(t1);
  d.t1s = tc_sq(t1);
  d.t2 = tc_qlog(t2);
  d.n1 = tc_qlog(n1);
  d.w1 = tc_qlog(w1);
  d.p0 = tc_qlog(p0);
  d.p0s = tc_sq(p0);
  d.pn = pnn;
  d.x1 = tc_qlog(x1);
  d.x1s = tc_sq(x1);
  d.ps = tc_sq(psum);
  d.chn = (int)(P.ilv ? P.smod : P.j);
  d.crun = tc_qlog(P.clrun);
  d.cm = P.clmatch;
  d.cls = (int)P.cls;
  d.zrun = tc_qlog(*P.zr);
  d.blk = (int)hist.tc_blk;
  d.bkq = P.bkq;
  d.pq = fam_dig.pc.axis();
  d.sq = 0;
  {
    i64 ex = (i64)(av >> 4) * TC_dig_avA + (i64)tc_abs(t1) * TC_dig_avT1 + (i64)tc_abs(t2) * TC_dig_avT2 + (i64)tc_abs(n1) * TC_dig_avN1 + (i64)tc_abs(w1) * TC_dig_avW1 + (i64)tc_abs(*P.q1) * TC_dig_avQ1 + (i64)tc_abs(*P.q2) * TC_dig_avQ2 + (i64)tc_abs(x1) * TC_dig_avX1;
    d.ax = tc_qlog((ex * TC_dig_avC) >> 12);
  }
  tc_make_dig(d, v);
  if(!tc_enc)
    dg = 0;
  {
    i64 mg = fam_dig.code(v, tc_enc ? (dg < 0 ? -dg : dg) : 0);
    if(mg) {
      tcx w;
      int sn;
      int spr = fam_dig.pc.sign(mg);
      d.mg = tc_qlog(mg);
      d.sq = tcp_bucket(spr);
      tc_make_sgn(d, w);
      fam_sgn.select(w);
      fam_sgn.prp = spr;
      sn = fam_sgn.bit(0, tc_enc && dg < 0);
      fam_sgn.prp = -1;
      dg = sn ? -mg : mg;
    } else
      dg = 0;
  }
  if(fam_dig.pc.on) {
    i64 gv = dg + (i64)P.bk->off;
    fam_dig.pc.step(P.bk->inv && gv >= 0 && gv < (i64)P.bk->base && P.bk->inv[gv] != (u32)-1 ? (int)P.bk->inv[gv] : -1);
  }
  if(ok) {
    i16 c = (i16)(dg < -32768 ? -32768 : dg > 32767 ? 32767 : dg);
    i32 a = tc_abs(c);
    P.hist2[slot] = P.hist[slot];
    P.hist[slot] = c;
    P.avg[slot] = (((u32)P.avg[slot] * (u32)TC_dig_avD) >> 8) + (u32)(a > 4095 ? 4095 : a) * 16;
    P.pp[slot] = c;
    P.pn[slot] = (u8)(pnn < 7 ? pnn + 1 : 7);
    {
      i32 t = psum + c;
      P.ps[slot] = (i16)(t < -32768 ? -32768 : t > 32767 ? 32767 : t);
    }
  }
  *P.q2 = *P.q1;
  *P.q1 = (i32)dg;
  *P.zr = dg ? 0 : *P.zr + 1;
  if(P.dim && ++P.vmod == P.dim)
    P.vmod = 0;
  if(P.ilv && ++P.smod == P.ilv)
    P.smod = 0;
  if(tc_verbose)
    tc_syms[STG_DIGIT]++;
  return dg;
}
u32 oc_model::classify(u32 rno, u32 j, u32 slot, u32 cin) {
  tcx v;
  u8 *ch = hist.cl_np[rno] ? hist.cl_hist[rno] + ((sz)hist.tc_blk * hist.tc_nchan + j) * hist.cl_np[rno] : nullptr;
  i32 t1 = (ch && slot < hist.cl_np[rno]) ? ch[slot] : 0;
  i32 tn = (ch && slot + 1 < hist.cl_np[rno]) ? ch[slot + 1] : 0;
  i64 c;
  tc_stage = STG_CLASS;
  tc_make_cls((int)rno, tc_qlog(slot), hist.cl_last[rno][j], t1, (int)hist.tc_blk, tn, hist.cl_last2[rno][j], fam_cls.pc.axis(), v);
  c = tc_enc ? (i64)cin : 0;
  c = fam_cls.code(v, c);
  FATAL_UNLESS(c >= 0 && c < 16, "coded stream: residue class %" PRId64, c);
  cl_run[rno] = (c == hist.cl_last[rno][j]) ? cl_run[rno] + 1 : 0;
  if(ch && slot < hist.cl_np[rno])
    ch[slot] = (u8)c;
  hist.cl_last2[rno][j] = hist.cl_last[rno][j];
  hist.cl_last[rno][j] = (i32)c;
  if(tc_verbose)
    tc_syms[STG_CLASS]++;
  return (u32)c;
}
void oc_model::residue_begin(u32 rno, const vb_res *r, const u8 *nz, u32 nch, u32 n) {
  const vb_book *cb = vb->cur->bk + r->cbook;
  u32 end, i, k, nc = 0;
  for(i = 0; i < nch; i++)
    if(nz[i])
      nc++;
  if(r->type == 2) {
    R.vch = 1;
    R.ilv = nch;
    R.cidx[0] = 0;
    end = ogc_min(r->end, n * nch);
  } else {
    R.vch = nc;
    R.ilv = 0;
    end = ogc_min(r->end, n);
    for(i = 0, k = 0; i < nch; i++)
      if(nz[i])
        R.cidx[k++] = (u8)i;
  }
  R.np = end > r->beg ? (end - r->beg) / r->psz : 0;
  R.pv = cb->dim;
  R.w = R.np + R.pv;
  FATAL_UNLESS((sz)R.vch * R.w <= sizeof hist.cl_run_buf, "residue needs %" PRIu64 " classification slots, limit %" PRIu64, (u64)((sz)R.vch * R.w), (u64)sizeof hist.cl_run_buf);
  memset(hist.dg_q1 + (sz)rno * 8 * hist.tc_nchan, 0, (sz)8 * hist.tc_nchan * sizeof(i32));
  memset(hist.dg_q2 + (sz)rno * 8 * hist.tc_nchan, 0, (sz)8 * hist.tc_nchan * sizeof(i32));
  memset(hist.dg_zr + (sz)rno * 8 * hist.tc_nchan, 0, (sz)8 * hist.tc_nchan * sizeof(i32));
  if(hist.dg_span[rno]) {
    memset(hist.dg_pp[rno], 0, (sz)hist.tc_nchan * hist.dg_span[rno] * sizeof(i16));
    memset(hist.dg_ps[rno], 0, (sz)hist.tc_nchan * hist.dg_span[rno] * sizeof(i16));
    memset(hist.dg_pn[rno], 0, (sz)hist.tc_nchan * hist.dg_span[rno]);
  }
}
u32 oc_model::cls(u32 rno, u32 jv, u32 pc, u32 k, u32 cin) {
  u32 j = R.cidx[jv];
  sz sl = (sz)jv * R.w + pc + k;
  u8 *ch = hist.cl_np[rno] ? hist.cl_hist[rno] + ((sz)hist.tc_blk * hist.tc_nchan + j) * hist.cl_np[rno] : nullptr;
  i32 was = (ch && pc + k < hist.cl_np[rno]) ? ch[pc + k] : -1;
  i32 run = cl_run[rno];
  u32 c;
  if(k == 0)
    fam_cls.pc.start(tcp.cls[rno].ok ? &tcp.cls[rno] : nullptr);
  c = classify(rno, j, pc + k, cin);
  if(sl < sizeof hist.cl_run_buf) {
    hist.cl_run_buf[sl] = (u8)(run > 255 ? 255 : run);
    hist.cl_same[sl] = (u8)(was == (i32)c);
  }
  fam_cls.pc.step((int)c);
  return c;
}
