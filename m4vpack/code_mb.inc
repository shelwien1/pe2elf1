// code_mb.inc -- model_mb.inc's ModelMb::code_mb(): one macroblock record; in decoding mode m is
// filled in.

template <int f_DEC, class LV>
void ModelMb<f_DEC, LV>::code_mb(const Ctx* cx, VState* s, MB* m, int idx) {
  Vop* v = &s->v;
  const int mb_x = idx % s->mb_w;
  const MBCtx* L = mb_x > 0 && mbcur[idx - 1].cls ? &mbcur[idx - 1] : 0;
  const MBCtx* A = idx >= s->mb_w && mbcur[idx - s->mb_w].cls ? &mbcur[idx - s->mb_w] : 0;
  const MBCtx* R = ref_mbs ? &mbref[idx] : 0;
  const MBCtx* AL = mb_x > 0 && idx > s->mb_w && mbcur[idx - s->mb_w - 1].cls ? &mbcur[idx - s->mb_w - 1] : 0;
  const MBCtx* AR =
      mb_x + 1 < s->mb_w && idx >= s->mb_w && mbcur[idx - s->mb_w + 1].cls ? &mbcur[idx - s->mb_w + 1] : 0;
  const int rb = cap(s->run, kCap);
  const int ty = v->type - PT_I;
  const Around n = { L, A, AL, AR, R };
  MBCtx* me = &mbcur[idx];
  const int dcvlc = v->qscale < v->dcthr;
  int intra = 0, cbp, cls3, i, k, mvi = 0, sumq = 0, px = 0, py = 0, sx = -1, sy = -1, frac = kFracNone;
  int col[2][2] = { { 0, 0 }, { 0, 0 } };  // (B-VOP: the co-located vector as direct mode scales it, each way)
  M_mbt_v T;
  memset(&T, 0, sizeof(T));
  T.st = v->type - PT_I;
  T.rc = R ? R->cls : 0;
  if (!ENC) mb_clear(m);
  memset(me, 0, sizeof(*me));
  est.ok = 0;
  tmk[0] = tmk[1] = 0;
  mo_mb_begin(&mo, idx);
  if (LV::recon && rec.on) rc_mb_begin(&rec, idx);
  c->cat = CAT_MB;
  if (v->type != PT_I && v->type != PT_B) mo_pred(&mo, 0, &px, &py, &sx, &sy);
  if (v->type == PT_I) {
    m->st = (int)checked(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
    m->t = MB_INTRA;
    intra = 1;
    cls3 = C3_INTRA;
  } else if (v->type != PT_B) {
    M_skp_v K;
    m->st = (int)checked(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
    K.st = v->gmc ? SKS_GMC : SKS_P;
    around(K, n, C_SKIP, tri_of(R, C_SKIP));
    K.rc = R ? R->cls : 0;
    K.run = rb;
    K.pz = px || py;
    K.rz = !R ? kTriNone : mo_ref_mv(&mo, 0)[0] || mo_ref_mv(&mo, 0)[1];
    K.bsk = K.rr = 0;
    K.zt = K.ze = 0;
    if (LV::recon && rec.on) skip_vars(&K.zt, &K.ze);
    T.zt = K.zt;
    T.ze = K.ze;
    sel(f_skp, K);
    if ((m->sk = f_skp.bit(SKP_P, ENC && m->sk))) {
      me->bsk = !v->gmc;
      s->run++;
      return mb_bare(me, m, C_SKIP, mb_x);
    }
    s->run = 0;
    around(T, n, C_INTRA, rt_of(R, C_INTRA, C_SKIP));
    if (mbt_bit(MBN_INTRA, T, m->t == MB_INTRA)) {
      m->t = MB_INTRA;
      intra = 1;
      cls3 = C3_INTRA;
    } else {
      around(T, n, C_4MV, rt_of(R, C_4MV, C_INTER));
      m->t = mbt_bit(MBN_4MV, T, m->t == MB_4MV) ? MB_4MV : MB_INTER;
      if (v->gmc && m->t == MB_INTER) {
        around(T, n, C_GMC, rt_of(R, C_GMC, C_GMC));
        m->mc = mbt_bit(MBN_GMC, T, m->mc);
      }
      cls3 = C3_P;
    }
  } else {
    M_skp_v K;
    memset(&K, 0, sizeof(K));
    K.st = SKS_B;
    K.rc = R ? R->cls : 0;
    K.bsk = R ? R->bsk : kTriNone;
    sel(f_skp, K);
    if ((m->sk = f_skp.bit(SKP_B, ENC && m->sk))) return mb_bare(me, m, C_SKIP, mb_x);
    around(K, n, C_BDIR0, tri_of(R, C_BDIR0));
    K.run = rb;
    K.rr = !R || R->cls == C_SKIP ? RR_NONE : R->cls == C_INTRA ? RR_INTRA : RR_CODED;
    if (LV::recon && rec.on) skip_vars(&K.zt, &K.ze);
    T.zt = K.zt;
    T.ze = K.ze;
    sel(f_skp, K);
    if (f_skp.bit(SKP_BDIR, ENC && m->mb == MODB_NONE)) {
      m->mb = MODB_NONE;
      m->t = MB_DIRECT;
      for (i = 0; i < MB_BLOCKS; i++) m->b[i].n = 0;
      s->run++;
      return mb_bare(me, m, C_BDIR0, mb_x);
    }
    s->run = 0;
    T.cl = (L && L->cbp) + (A && A->cbp);
    T.c4 = T.cl;
    T.rt = R ? R->cbp != 0 : kTriNone;
    m->mb = mbt_bit(MBN_BCBP, T, m->mb == MODB_CBP) ? MODB_CBP : MODB_NOCBP;
    T.cd = count_of(C_BDIR, L, A);
    around(T, n, C_BFWD, tri_of(R, C_BFWD));
    if (mbt_bit(MBN_BDIRECT, T, m->t == MB_DIRECT)) m->t = MB_DIRECT;
    else if (mbt_bit(MBN_BFWD, T, m->t == MB_FWD)) m->t = MB_FWD;
    else {
      around(T, n, C_BBWD, tri_of(R, C_BBWD));
      m->t = mbt_bit(MBN_BBWD, T, m->t == MB_BWD) ? MB_BWD : MB_BI;
    }
    cls3 = C3_B;
  }
  T.cd = 0;
  T.rt = 0;
  if (intra) {
    T.cl = (L && L->ac) + (A && A->ac);
    T.c4 = T.cl;
    T.intra = 1;
    m->ac = mbt_bit(MBN_AC, T, m->ac);
    if (LV::recon && rec.on) rc_mb_intra(&rec, m->ac), rc_intra_est(&rec, &est), est_err = est_nb = 0;
  }
  if (mb_has_fp(cx, v, m)) {
    T.cl = (L && L->fp) + (A && A->fp);
    T.c4 = T.cl;
    m->fp = mbt_bit(MBN_FIELD, T, m->fp);
    if (m->fp == 1) {
      m->nfs = mb_nfs(v, m);  // (a select per field)
      for (i = m->nfs - 1, k = 0; i >= 0; i--) {
        T.fsn = cap(i, kFsnCap);
        k |= mbt_bit(MBN_FSEL, T, (m->fs >> i) & 1) << i;
      }
      m->fs = k;
      T.fsn = 0;
    }
  }
  c->cat = CAT_MV;
  mo_kind(&mo, m);
  if (v->type == PT_B && !intra) mo_colocated(&mo, &col[0][0], &col[0][1], &col[1][0], &col[1][1]);
  if (LV::recon && rec.on && !intra) {  // the vectors the template matches best
    if (v->type != PT_B) {
      if (m->t == MB_4MV || (m->mc != 1 && m->fp != 1))
        tmk[0] = rc_tm_search(&rec, rec.last, px, py, mo_ref_mv(&mo, 0)[0], mo_ref_mv(&mo, 0)[1], &tmx[0], &tmy[0]);
    } else if (m->t != MB_DIRECT && m->fp != 1) {
      int ppx, ppy;
      if (fwd_dir(m->t)) {
        mo_bpred(&mo, 0, 0, 1, &ppx, &ppy);
        tmk[0] = rc_tm_search(&rec, rec.last, ppx, ppy, col[0][0], col[0][1], &tmx[0], &tmy[0]);
      }
      if (bwd_dir(m->t)) {
        mo_bpred(&mo, 1, 0, 1, &ppx, &ppy);
        tmk[1] = rc_tm_search(&rec, rec.next, ppx, ppy, col[1][0], col[1][1], &tmx[1], &tmy[1]);
      }
    }
  }
  if (!intra) {  // the vectors as the decoder gets them, from their predictions (motion.inc)
    const int tok = R != 0, fm = cx->qpel ? kQpelMask : kHpelMask;
    int mx = 0, my = 0;
    if (v->type != PT_B) {
      if (m->t == MB_4MV)
        for (i = 0; i < MB_LUMA; i++) {
          if (i) mo_pred(&mo, i, &px, &py, &sx, &sy);
          code_mv(m, &mvi, MVK_P4, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mo_ref_mv(&mo, i)[0],
                  mo_ref_mv(&mo, i)[1], &mx, &my, 0);
          mo_set(&mo, 0, i, mx, my);
        }
      else if (m->fp == 1)
        for (i = 0; i < 2; i++) {
          code_mv(m, &mvi, MVK_PFIELD, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mo_ref_mv(&mo, 0)[0],
                  mo_ref_mv(&mo, 0)[1] / 2, &mx, &my);
          mo_set(&mo, 0, i, mx, my);
        }
      else if (m->mc != 1) {
        code_mv(m, &mvi, MVK_P, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mo_ref_mv(&mo, 0)[0],
                mo_ref_mv(&mo, 0)[1], &mx, &my, 0);
        mo_set(&mo, 0, 0, mx, my);
      }
    } else if (m->t != MB_DIRECT) {  // B-VOP: each predicted by the last of its direction in the row
      const int n = m->fp == 1 ? 2 : 1;
      int dir, ppx, ppy;
      for (dir = 0; dir < 2; dir++) {
        if (!(dir ? bwd_dir(m->t) : fwd_dir(m->t))) continue;
        for (i = 0; i < n; i++) {
          mo_bpred(&mo, dir, i, n, &ppx, &ppy);
          code_mv(m, &mvi, MVK_BFWD + dir, dir ? v->bcode : v->fcode, L, A, R, me, &sumq, ppx, ppy, -1, -1, tok,
                  col[dir][0], n == 2 ? col[dir][1] / 2 : col[dir][1], &mx, &my, n == 1 ? dir : -1);
          mo_bset(&mo, dir, i, n, mx, my);
        }
      }
    } else {  // direct mode's delta
      code_mv(m, &mvi, MVK_BDIR, FCODE_MIN, L, A, R, me, &sumq, 0, 0, -1, -1, 0, 0, 0, &mx, &my);
      mo_direct(&mo, mx, my);
    }
    mo_mb_end(&mo);
    if (v->type != PT_B && m->mc != 1) {  // (the macroblock's vector as the grid has it)
      mx = mo.cur->mv[mo.block_index[0]][0];
      my = mo.cur->mv[mo.block_index[0]][1];
    }
    if ((v->type != PT_B && m->mc != 1) || (v->type == PT_B && m->t != MB_DIRECT))
      frac = ((mx & fm) != 0) + 2 * ((my & fm) != 0);
    if (LV::recon && rec.on) rc_mb_inter(&rec);  // (its prediction in the picture)
  } else
    mo_mb_end(&mo);
  c->cat = CAT_CBP;
  int cie[MB_BLOCKS] = {0}, ctx[MB_BLOCKS] = {0}, cem[MB_BLOCKS] = {0};
  if (LV::recon && est.ok)
    for (i = 0; i < MB_BLOCKS; i++) {  // the estimate's AC coefficients of half a level or more
      int r, nn = 0;
      for (r = 1; r < BLK_COEFS; r++) nn += abs(rc_est_q4(&rec, &est, i, r)) >= kQ / 2;
      cie[i] = 1 + cap(nn, kCap);
    }
  if (LV::recon && rec.on && !intra && !(v->type == PT_B && m->mb == MODB_NOCBP))
    for (i = 0; i < MB_BLOCKS; i++) {  // the prediction's texture and edge miss (its edges on other macroblocks)
      int tx, em;
      rc_pred_feat(&rec, i, 0, 0, &tx, &em);
      ctx[i] = qlog1(tx);
      cem[i] = em < 0 ? 0 : qlog1(em);
    }
  cbp = v->type == PT_B && m->mb == MODB_NOCBP
            ? 0
            : code_cbp(cls3, L, A, R, mvi ? mvq(sumq) : kNone, frac, v->qscale,
                       (L ? L->cbp != 0 : kTriNone) == (A ? A->cbp != 0 : kTriNone) ? 0 : 1 + (R && R->cbp), ENC ? cbp_of(m) : 0,
                       cie, est_trust(), ctx, cem);
  c->cat = CAT_MB;
  if (mb_has_dq(v, m, cbp)) {
    M_qpd_v Q;
    const int node = v->type == PT_B ? QPD_B : QPD_P;
    Q.c3 = cls3;
    Q.pdq = s->prev_dq;
    Q.qs = v->qscale;
    sel(f_qpd, Q);
    k = f_qpd.bit(node, ENC && m->dq != 0);
    if (!k) m->dq = 0;
    else if (v->type != PT_B) {  // which of -1, -2, 1, 2 (quant_tab), in a tree of two bits
      const int x = ENC ? index_of(quant_tab, m->dq) : 0, b1 = f_qpd.bit(QPD_IDX, x >> 1);
      m->dq = quant_tab[b1 * 2 + f_qpd.bit(QPD_IDX_LO + b1, x & 1)];
    } else
      m->dq = f_qpd.bit(QPD_BSIGN, ENC && m->dq > 0) ? DQ_B : -DQ_B;
    s->prev_dq = k;
    v->qscale = clip(v->qscale + m->dq, QS_MIN, QS_MAX);
    if (LV::recon && rec.on) rc_set_qscale(&rec, v->qscale);
  }
  if (mb_has_dt(cx, intra, cbp)) {
    T.cl = (L && L->dt) + (A && A->dt);
    T.c4 = T.cl;
    T.intra = intra;
    m->dt = mbt_bit(MBN_DCT, T, m->dt);
    if (LV::recon) rec.interlaced_dct = m->dt;
    if (LV::recon && est.ok && m->dt == 1) rc_est_luma(&rec, &est, 1);  // (the estimate's luma in fields)
  }
  m->nmv = mvi;
  {
    for (i = 0; i < MB_BLOCKS; i++) {
      const int luma = i < MB_LUMA;  // (a luma block: i & 1 the right column, i & 2 the lower row)
      const int cls = (intra ? (m->ac == 1 ? CL_INTRA_AC : CL_INTRA) : v->type == PT_B ? CL_B : CL_INTER) + !luma;
      const BlkCtx* Lb = luma && (i & 1) ? &rowb[mb_x][i - 1] : !L ? 0 : &rowb[mb_x - 1][luma ? i + 1 : i];
      const BlkCtx* Ab = luma && (i & 2) ? &rowb[mb_x][i - 2] : !A ? 0 : &rowb[mb_x][luma ? i + 2 : i];
      code_block(s, &m->b[i], i, intra, intra && dcvlc, cbp_bit(cbp, i), cls, nz_ctx(me, L, A, i), v->qscale,
                 mvi ? mvq(sumq) : kMvcNone, R ? cap(R->nz[i], kCap) : kNone, Lb, Ab, &rowb[mb_x][i],
                 i == BLK_CR ? &rowb[mb_x][BLK_CB] : 0);
      me->nz[i] = (U8)cap(m->b[i].n, BLK_LAST);
      if (LV::recon && rec.on) rc_block(&rec, i, &m->b[i], intra, intra && dcvlc);
    }
  }
  if (LV::recon && rec.on) rc_mb_end(&rec);
  me->cbp = (U8)cbp;
  me->ac = intra && m->ac == 1;
  me->dt = m->dt == 1;
  me->fp = m->fp == 1;
  if (intra) me->cls = C_INTRA;
  else if (v->type != PT_B) me->cls = m->t == MB_4MV ? C_4MV : m->mc == 1 ? C_GMC : C_INTER;
  else me->cls = m->t == MB_DIRECT ? C_BDIR : m->t == MB_FWD ? C_BFWD : m->t == MB_BWD ? C_BBWD : C_BBI;
}
