// code_mb.inc -- model.inc's Model::code_mb(): one macroblock record; in decoding mode m is
// filled in.

template <int f_DEC, class LV>
void Model<f_DEC, LV>::code_mb(const Ctx* cx, VState* s, MB* m, int idx) {
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
  MBCtx* me = &mbcur[idx];
  const int dcvlc = v->qscale < v->dcthr;
  int intra = 0, cbp, cls3, i, k, mvi = 0, sumq = 0, px = 0, py = 0, sx = -1, sy = -1, frac = kFracNone;
  M_mbt_v T;
  memset(&T, 0, sizeof(T));
  T.st = v->type - PT_I;
  T.rc = R ? R->cls : 0;
  if (!ENC) {
    m->st = m->sk = 0;
    m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
    m->nfs = m->fs = m->nmv = 0;
  }
  memset(me, 0, sizeof(*me));
  est.ok = 0;
  tmk[0] = tmk[1] = 0;
  if (LV::recon && rec.on) rc_mb_begin(&rec, idx);
  c->cat = CAT_MB;
  if (v->type == PT_B && mb_x == 0) memset(s->lmv, 0, sizeof(s->lmv));
  if (v->type != PT_I && v->type != PT_B) pred_mv(s, idx, 0, &px, &py, &sx, &sy);
  if (v->type == PT_I) {
    m->st = (int)checked(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
    m->t = MB_INTRA;
    intra = 1;
    cls3 = C3_INTRA;
  } else if (v->type != PT_B) {
    M_skp_v K;
    m->st = (int)checked(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
    K.st = v->gmc ? SKS_GMC : SKS_P;
    K.cl = count_of(C_SKIP, L, A);
    K.c4 = K.cl + count_of(C_SKIP, AL, AR);
    K.rt = R ? R->cls == C_SKIP : kTriNone;
    K.rc = R ? R->cls : 0;
    K.run = rb;
    K.pz = px || py;
    K.rz = !R ? kTriNone : mvref[idx][0][0] || mvref[idx][0][1];
    K.bsk = K.rr = 0;
    K.zt = K.ze = 0;
    if (LV::recon && rec.on) skip_vars(&K.zt, &K.ze);
    T.zt = K.zt;
    T.ze = K.ze;
    sel(f_skp, K);
    if ((m->sk = f_skp.bit(SKP_P, ENC && m->sk))) {
      me->cls = C_SKIP;
      me->bsk = !v->gmc;
      rowb_clear(mb_x);
      s->run++;
      if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
      return;
    }
    s->run = 0;
    T.cl = count_of(C_INTRA, L, A);
    T.c4 = T.cl + count_of(C_INTRA, AL, AR);
    T.rt = !R ? RT_NONE : R->cls == C_INTRA ? RT_SAME : R->cls == C_SKIP ? RT_NEXT : RT_OTHER;
    if (mbt_bit(MBN_INTRA, T, m->t == MB_INTRA)) {
      m->t = MB_INTRA;
      intra = 1;
      cls3 = C3_INTRA;
    } else {
      T.cl = count_of(C_4MV, L, A);
      T.c4 = T.cl + count_of(C_4MV, AL, AR);
      T.rt = !R ? RT_NONE : R->cls == C_4MV ? RT_SAME : R->cls == C_INTER ? RT_NEXT : RT_OTHER;
      m->t = mbt_bit(MBN_4MV, T, m->t == MB_4MV) ? MB_4MV : MB_INTER;
      if (v->gmc && m->t == MB_INTER) {
        T.cl = count_of(C_GMC, L, A);
        T.c4 = T.cl + count_of(C_GMC, AL, AR);
        T.rt = !R ? RT_NONE : R->cls == C_GMC ? RT_SAME : RT_OTHER;
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
    if ((m->sk = f_skp.bit(SKP_B, ENC && m->sk))) {
      me->cls = C_SKIP;
      rowb_clear(mb_x);
      if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
      return;
    }
    K.cl = count_of(C_BDIR0, L, A);
    K.c4 = K.cl + count_of(C_BDIR0, AL, AR);
    K.rt = R ? R->cls == C_BDIR0 : kTriNone;
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
      me->cls = C_BDIR0;
      rowb_clear(mb_x);
      s->run++;
      if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
      return;
    }
    s->run = 0;
    T.cl = (L && L->cbp) + (A && A->cbp);
    T.c4 = T.cl;
    T.rt = R ? R->cbp != 0 : kTriNone;
    m->mb = mbt_bit(MBN_BCBP, T, m->mb == MODB_CBP) ? MODB_CBP : MODB_NOCBP;
    T.cd = count_of(C_BDIR, L, A);
    T.cl = count_of(C_BFWD, L, A);
    T.c4 = T.cl + count_of(C_BFWD, AL, AR);
    T.rt = R ? R->cls == C_BFWD : kTriNone;
    if (mbt_bit(MBN_BDIRECT, T, m->t == MB_DIRECT)) m->t = MB_DIRECT;
    else if (mbt_bit(MBN_BFWD, T, m->t == MB_FWD)) m->t = MB_FWD;
    else {
      T.cl = count_of(C_BBWD, L, A);
      T.c4 = T.cl + count_of(C_BBWD, AL, AR);
      T.rt = R ? R->cls == C_BBWD : kTriNone;
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
  if (!intra && cx->interlaced && (v->type == PT_B ? m->t != MB_DIRECT : m->t == MB_INTER && m->mc != 1)) {
    T.cl = (L && L->fp) + (A && A->fp);
    T.c4 = T.cl;
    m->fp = mbt_bit(MBN_FIELD, T, m->fp);
    if (m->fp == 1) {
      m->nfs = v->type == PT_B ? 2 * (fwd_dir(m->t) + bwd_dir(m->t)) : 2;  // (a select per field)
      for (i = m->nfs - 1, k = 0; i >= 0; i--) {
        T.fsn = cap(i, kFsnCap);
        k |= mbt_bit(MBN_FSEL, T, (m->fs >> i) & 1) << i;
      }
      m->fs = k;
      T.fsn = 0;
    }
  }
  c->cat = CAT_MV;
  if (LV::recon && rec.on && !intra) {  // the vectors the template matches best
    if (v->type != PT_B) {
      if (m->t == MB_4MV || (m->mc != 1 && m->fp != 1))
        tmk[0] = rc_tm_search(&rec, rec.last, px, py, mvref[idx][0][0], mvref[idx][0][1], &tmx[0], &tmy[0]);
    } else if (m->t != MB_DIRECT && m->fp != 1) {
      const int cx0 = R ? mvref[idx][0][0] : 0, cy0 = R ? mvref[idx][0][1] : 0;
      const int tfx = (int)((S64)cx0 * s->trb / s->trd), tfy = (int)((S64)cy0 * s->trb / s->trd);
      if (fwd_dir(m->t))
        tmk[0] = rc_tm_search(&rec, rec.last, s->lmv[0][0][0], s->lmv[0][0][1], tfx, tfy, &tmx[0], &tmy[0]);
      if (bwd_dir(m->t))
        tmk[1] = rc_tm_search(&rec, rec.next, s->lmv[1][0][0], s->lmv[1][0][1], tfx - cx0, tfy - cy0, &tmx[1], &tmy[1]);
    }
  }
  if (!intra) {
    const int tok = R != 0, fm = cx->qpel ? kQpelMask : kHpelMask;
    int mx = 0, my = 0, mx2, my2;
    if (v->type != PT_B) {  // the vectors as the decoder gets them; their prediction
      if (m->t == MB_4MV)
        for (i = 0; i < MB_LUMA; i++) {
          if (i) pred_mv(s, idx, i, &px, &py, &sx, &sy);
          code_mv(m, &mvi, MVK_P4, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][i][0],
                  mvref[idx][i][1], &mx, &my, 0);
          mvcur[idx][i][0] = (S16)mx;
          mvcur[idx][i][1] = (S16)my;
        }
      else {
        if (m->mc == 1) mx = px, my = py;  // GMC: the prediction stands in for the warped vector
        else if (m->fp == 1) {
          code_mv(m, &mvi, MVK_PFIELD, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1] / 2, &mx, &my);
          code_mv(m, &mvi, MVK_PFIELD, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1] / 2, &mx2, &my2);
          mx = ((mx + mx2) >> 1) | ((mx + mx2) & 1);
          my = my + my2;
        } else
          code_mv(m, &mvi, MVK_P, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1], &mx, &my, 0);
        for (i = 0; i < MB_LUMA; i++) { mvcur[idx][i][0] = (S16)mx; mvcur[idx][i][1] = (S16)my; }
      }
    } else if (m->t != MB_DIRECT) {  // B-VOP: predicted by the last vector in the row; the co-located one scaled as in
                        // direct mode
      const int n = m->fp == 1 ? 2 : 1, cx0 = R ? mvref[idx][0][0] : 0, cy0 = R ? mvref[idx][0][1] : 0;
      const int tfx = (int)((S64)cx0 * s->trb / s->trd), tfy = (int)((S64)cy0 * s->trb / s->trd);
      int dir;
      for (dir = 0; dir < 2; dir++) {
        const int code = dir ? v->bcode : v->fcode, tx = dir ? tfx - cx0 : tfx, tyv = dir ? tfy - cy0 : tfy;
        if (!(dir ? bwd_dir(m->t) : fwd_dir(m->t))) continue;
        for (i = 0; i < n; i++) {
          const int ppx = s->lmv[dir][i][0], ppy = n == 2 ? s->lmv[dir][i][1] / 2 : s->lmv[dir][i][1];
          code_mv(m, &mvi, MVK_BFWD + dir, code, L, A, R, me, &sumq, ppx, ppy, -1, -1, tok, tx, n == 2 ? tyv / 2 : tyv,
                  &mx, &my, n == 1 ? dir : -1);
          if (n == 1) s->lmv[dir][0][0] = s->lmv[dir][1][0] = mx, s->lmv[dir][0][1] = s->lmv[dir][1][1] = my;
          else s->lmv[dir][i][0] = mx, s->lmv[dir][i][1] = my * 2;
        }
      }
    } else
      code_mv(m, &mvi, MVK_BDIR, FCODE_MIN, L, A, R, me, &sumq, 0, 0, -1, -1, 0, 0, 0, &mx, &my);
    if (v->type != PT_B && m->mc != 1) mx = mvcur[idx][0][0], my = mvcur[idx][0][1];
    if ((v->type != PT_B && m->mc != 1) || (v->type == PT_B && m->t != MB_DIRECT))
      frac = ((mx & fm) != 0) + 2 * ((my & fm) != 0);
    if (LV::recon && rec.on) rc_mb_inter(&rec, m);  // (its prediction in the picture)
  }
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
  if (v->type != PT_B || (m->t != MB_DIRECT && cbp)) {
    M_qpd_v Q;
    const int node = v->type == PT_B ? QPD_B : QPD_P;
    Q.c3 = cls3;
    Q.pdq = s->prev_dq;
    Q.qs = v->qscale;
    sel(f_qpd, Q);
    k = f_qpd.bit(node, ENC && m->dq != 0);
    if (!k) m->dq = 0;
    else if (v->type != PT_B) {  // which of -1, -2, 1, 2 (quant_tab), in a tree of two bits
      const int x = ENC ? dq_index(m->dq) : 0, b1 = f_qpd.bit(QPD_IDX, x >> 1);
      m->dq = quant_tab[b1 * 2 + f_qpd.bit(QPD_IDX_LO + b1, x & 1)];
    } else
      m->dq = f_qpd.bit(QPD_BSIGN, ENC && m->dq > 0) ? DQ_B : -DQ_B;
    s->prev_dq = k;
    v->qscale = clip(v->qscale + m->dq, QS_MIN, QS_MAX);
    if (LV::recon && rec.on) rc_set_qscale(&rec, v->qscale);
  }
  if (cx->interlaced && (intra || cbp)) {
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
