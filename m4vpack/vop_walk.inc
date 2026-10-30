// vop_walk.inc -- model.inc's Model::vop_walk(): one VOP, parsed and written again, coded, or
// decoded and written (model.inc says how it is called).

template <int f_DEC, class LV>
int Model<f_DEC, LV>::vop_walk(Ctx* cx, int mode, GB* gb, PB* pb) {
  MB& m = mrec;
  VopHdr h;
  Vop vp, vw;
  VState s;
  int mb_x = 0, mb_y = 0, anchor, ret = 0, nb, tbits = 0, tn = 0, i;
  double mb_bits = 0;
  memset(&h, 0, sizeof(h));
  memset(&s, 0, sizeof(s));
  if (mode != W_DECODE && parse_vop_hdr(cx, gb, &h) < 0) return W_FAIL;
  if (mode != W_VERIFY) code_vop_hdr(cx, &h);
  if (mode == W_DECODE && c->err) return W_FAIL;
  if (LV::recon && mode != W_VERIFY && rc_vop_begin(&rec, cx, &h) < 0) return W_NOMEM;
  if (mode != W_ENCODE && put_vop_hdr(cx, pb, &h) < 0) return W_FAIL;
  if (h.coded) {
    if (!cx->have_vol || cx->unsupported || !cx->mb_num || cx->mb_num > MAX_MBS) return W_FAIL;
    if (!ctx_skip_room(cx, cx->mb_num) || !mb_room(cx->mb_num)) return W_NOMEM;
    vop_state(&vp, cx, &h);
    vw = s.v = vp;
    s.mb_w = cx->mb_w;
    s.scan = h.alt == 1 ? alternate_vertical_scan : zigzag_direct;
    anchor = h.type != PT_B;
    if (mode != W_DECODE && anchor) memset(cx->skip_cur, 0, cx->mb_num + 1);
    if (mode != W_VERIFY) {
      memset(mbcur, 0, cx->mb_num * sizeof(*mbcur));
      memset(mvcur, 0, cx->mb_num * sizeof(*mvcur));
      if (ref_mbs != cx->mb_num) ref_mbs = 0;
      s.trb = last.pb_time;
      s.trd = last.pp_time;
      if (s.trb <= 0 || s.trb >= s.trd) s.trb = 1, s.trd = 2;  // (inconsistent times: halfway)
    }
    for (;;) {
      const int idx = mb_y * cx->mb_w + mb_x;
      m.x = mb_x;
      m.y = mb_y;
      if (mode != W_DECODE) {
        const double b0 = pbits[PB_MV], b1 = pbits[PB_DC], b2 = pbits[PB_COEF];
        const int p0 = gb->pos;
        if (parse_mb(cx, gb, &vp, &m, idx) < 0 || (mode == W_VERIFY && !mb_codable(&m))) { ret = W_FAIL; break; }
        if (mode == W_ENCODE && c->stats) {
          c->orig[CAT_MV] += pbits[PB_MV] - b0;
          c->orig[CAT_DC] += pbits[PB_DC] - b1;
          c->orig[CAT_SIG] += pbits[PB_COEF] - b2;
          c->orig[CAT_MB] += gb->pos - p0 - (pbits[PB_MV] - b0) - (pbits[PB_DC] - b1) - (pbits[PB_COEF] - b2);
          mb_bits += gb->pos - p0;
        }
        if (anchor) cx->skip_cur[idx] = (U8)(m.sk == 1 && !vp.gmc);
      }
      if (mode != W_VERIFY) code_mb(cx, &s, &m, idx);
      if (mode == W_DECODE && (c->err || c->io->over > OVER_MAX)) { ret = W_FAIL; break; }  // damaged data
      if (mode != W_ENCODE && put_mb(cx, pb, &vw, &m) < 0) { ret = W_FAIL; break; }
      if (mode == W_DECODE && pb->err) { ret = W_FAIL; break; }
      if (++mb_x == cx->mb_w) { mb_x = 0; mb_y++; }
      if (mb_y >= cx->mb_h) break;
      {
        VpHdr k = VpHdr();  // (decompressing, what code_vp() would read of it for the compressor)
        int vpf = 0, nstuff = 0, next;
        if (mode != W_DECODE) {
          next = resync_ahead(cx, gb, &vp, &nstuff);
          vpf = next && !(idx + 1 < next && vp.type == PT_B && cx->skip_ref[idx + 1]);
        }
        if (mode != W_VERIFY) {
          c->cat = CAT_HDR;
          vpf = hdr_flag(F_VPFLAG, s.v.type - PT_I, 0, 0, vpf);
        }
        if (!vpf) continue;
        if (mode != W_DECODE) {
          if (nstuff && vp.type == PT_S) { ret = W_FAIL; break; }  // not MCBPC stuffing there
          k.st = nstuff;
          gb_skip(gb, nstuff * stuffing_len(vp.type));
          gb_skip(gb, 1);
          gb_align(gb);
          if (parse_vp(cx, gb, &vp, &k) < 0) { ret = W_FAIL; break; }
          vp.qscale = clipi(vp.qscale, QS_MIN, QS_MAX);
        }
        if (mode != W_VERIFY) {
          code_vp(cx, &s, &k, idx + 1);
          s.vp_start = k.mbnum;
          memset(s.lmv, 0, sizeof(s.lmv));
          if (LV::recon && rec.on && k.mbnum > 0 && k.mbnum < cx->mb_num) rc_vp_begin(&rec, &k);
        }
        if ((mode != W_ENCODE && put_vp(cx, pb, &vw, &k) < 0) || k.mbnum < 1 || k.mbnum >= cx->mb_num) {
          ret = W_FAIL;
          break;
        }
        mb_x = k.mbnum % cx->mb_w;
        mb_y = k.mbnum / cx->mb_w;
      }
    }
    if (mode != W_DECODE && anchor) memcpy(cx->skip_ref, cx->skip_cur, cx->mb_num + 1);
    if (LV::recon && mode != W_VERIFY) rc_vop_end(&rec);
    if (ret < 0) return W_FAIL;
    if (mode != W_VERIFY && anchor) {
      memcpy(mbref, mbcur, cx->mb_num * sizeof(*mbcur));
      memcpy(mvref, mvcur, cx->mb_num * sizeof(*mvcur));
      ref_mbs = cx->mb_num;
    }
  }
  // the rest: stuffing, normally
  if (mode != W_DECODE) {
    if (gb->pos > gb->size) return W_FAIL;
    nb = to_byte(gb->pos);
    tbits = (int)gb_get(gb, nb);
    tn = (gb->size - gb->pos) >> BYTE_SHIFT;
    if (tn > (int)TAIL_MAX) return W_FAIL;
    memcpy(tailbuf, gb->buf + (gb->pos >> BYTE_SHIFT), (size_t)tn);
  } else
    nb = to_byte(pb->n);
  if (mode != W_VERIFY && code_tail(nb, &tbits, &tn, tailbuf, TAIL_MAX) < 0) return W_FAIL;
  if (mode == W_DECODE && c->err) return W_FAIL;
  if (mode == W_ENCODE && c->stats) c->orig[CAT_HDR] += gb->size + SC_LEN * BYTE_BITS - mb_bits;
  if (mode != W_ENCODE) {
    pb_put(pb, (U32)tbits, nb);
    for (i = 0; i < tn; i++) pb_put(pb, tailbuf[i], BYTE_BITS);
  }
  return 0;
}
