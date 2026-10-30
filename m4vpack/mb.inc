// mb.inc -- syntax.inc's MB: a macroblock's record, and its walk (the bits read or written).

// A macroblock's record: its fields as the codes have them (m4vtsv's), and the walks of its parts
// and of it
struct MB {
  int x, y, st, sk, mb, t, mc, ac, dq, dt, fp;
  int nfs, fs;     // field selects: count, bits (first one in the highest bit)
  int nmv, mv[2 * MB_MVS];
  Blk b[MB_BLOCKS];

  void clear() {  // the optional fields absent (NA): where one read or decoded starts
    st = sk = 0;
    mb = t = mc = ac = dq = dt = fp = NA;
    nfs = fs = nmv = 0;
  }

  // Which fields a macroblock has, for the walks below and the model alike: field_prediction
  // (interlaced, an inter macroblock with a vector of its own: not direct, not GMC) and the field
  // selects after it, and whether it says its dquant (I-, P- and S-VOPs: in MCBPC; a B-VOP's
  // dbquant: a macroblock not direct, with coded blocks)
  int has_fp(const Ctx& c, const Vop& v) const {
    return c.interlaced && (v.type == PT_B ? t != MB_DIRECT : t == MB_INTER && mc != 1);
  }
  int fs_count(const Vop& v) const { return v.type == PT_B ? 2 * (fwd_dir(t) + bwd_dir(t)) : 2; }
  int has_dq(const Vop& v, int cbp) const { return v.type != PT_B || (t != MB_DIRECT && cbp); }

  int pattern() const {  // coded flags, block 0 in the top bit (of MB_BLOCKS)
    int i, cbp = 0;
    for (i = 0; i < MB_BLOCKS; i++) cbp = cbp * 2 + (b[i].n > 0);
    return cbp;
  }
  int has_zero_level() const {  // a level of 0 (which escape mode 3 allows: the model codes none)
    int i, j;
    for (i = 0; i < MB_BLOCKS; i++)
      for (j = 0; j < b[i].n; j++)
        if (!b[i].level[j]) return 1;
    return 0;
  }

  // n vectors' differences (x, y), into mv from *k on
  template <class IO>
  int mvs_walk(IO& io, int* k, int n, int f_code) {
    const int p0 = io.pos();
    int i;
    for (i = 0; i < 2 * n; i++, ++*k)
      if (!mv_walk(io, &mv[*k], f_code)) return 0;
    io.count(PB_MV, p0);
    return 1;
  }

  template <class IO>
  int blocks_walk(IO& io, int cbp, int intra, int dcvlc) {
    int i;
    for (i = 0; i < MB_BLOCKS; i++, cbp += cbp)
      if (!b[i].walk(io, i, cbp & 1 << (MB_BLOCKS - 1), intra, dcvlc)) return 0;
    return 1;
  }

  // dquant (I-, P- and S-VOPs: 2 bits, an index into quant_tab) or a B-VOP's dbquant (a code of
  // dbquant_tab); the quantiser in v follows it
  template <class IO>
  int dquant_walk(IO& io, Vop& v) {
    const int bvop = v.type == PT_B;
    int s = IO::W ? (bvop ? index_of(dbquant_dq, dq) : index_of(quant_tab, dq)) : 0;
    if (bvop ? !io.vlc(&s, vlc_dbquant) : !io.bits(&s, DQUANT_BITS)) return 0;
    dq = bvop ? dbquant_dq[s] : quant_tab[s];
    v.qscale = clip(v.qscale + dq, QS_MIN, QS_MAX);
    return 1;
  }

  // an intra macroblock after its MCBPC (cbpc; dquant: whether it says one): ac_pred_flag, cbpy,
  // dquant, dct_type, the blocks (the DCs coded apart below the VOP's threshold, as the quantiser was)
  template <class IO>
  int intra_walk(IO& io, const Ctx& c, Vop& v, int cbpc, int dquant) {
    const int dcvlc = v.qscale < v.dcthr;
    int cbpy = IO::W ? pattern() >> CBP_CHROMA_BITS : 0;
    if (!io.bit(&ac) || !io.vlc(&cbpy, vlc_cbpy)) return 0;
    if (!dquant) dq = 0;
    else if (!dquant_walk(io, v)) return 0;
    if (c.mb_has_dt(1, 0) && !io.bit(&dt)) return 0;
    return blocks_walk(io, (cbpc & MCBPC_CHROMA) | cbpy << CBP_CHROMA_BITS, 1, dcvlc);
  }

  // A macroblock (the idx-th; FFmpeg's mpeg4_decode_mb): MCBPC stuffing; in a P- or S-VOP not_coded;
  // MCBPC (I-, P-, S-VOPs) or modb, mb_type and the coded pattern (B-VOPs), and the fields they say;
  // the vectors; the blocks. The quantiser in v follows its dquant.
  template <class IO>
  int walk(IO& io, const Ctx& c, Vop& v, int idx) {
    const int cbp0 = IO::W ? pattern() : 0;  // (the writer's coded pattern)
    int cbpc = 0, cbpy = 0, cbp = cbp0, s = 0, k = 0;  // (k: the vectors so far)
    if (!IO::W) clear();
    if (io.over()) return 0;
    if (v.type == PT_I) {
      if (IO::W) cbpc = (dq ? MCBPC_IQUANT : 0) | (cbp0 & MCBPC_CHROMA);
      if (!io.stuffing(&st, v.type) || !io.vlc(&cbpc, vlc_intra_mcbpc)) return 0;
      t = MB_INTRA;
      return intra_walk(io, c, v, cbpc, cbpc & MCBPC_IQUANT);
    }
    if (v.type != PT_B) {  // P- and S-VOPs
      if (!io.stuffing(&st, v.type) || !io.bit(&sk)) return 0;
      if (sk) return 1;
      if (IO::W)
        cbpc = (t == MB_INTRA ? MCBPC_INTRA : t == MB_4MV ? MCBPC_4MV : 0) | (dq ? MCBPC_DQUANT : 0) |
               (cbp0 & MCBPC_CHROMA);
      if (!io.vlc(&cbpc, vlc_inter_mcbpc)) return 0;
      if (cbpc & MCBPC_INTRA) {
        t = MB_INTRA;
        return intra_walk(io, c, v, cbpc, cbpc & MCBPC_DQUANT);
      }
      t = cbpc & MCBPC_4MV ? MB_4MV : MB_INTER;
      if (v.gmc && t == MB_INTER && !io.bit(&mc)) return 0;
      if (IO::W) cbpy = (cbp0 >> CBP_CHROMA_BITS) ^ CBPY_INTER;
      if (!io.vlc(&cbpy, vlc_cbpy)) return 0;
      cbp = (cbpc & MCBPC_CHROMA) | (cbpy ^ CBPY_INTER) << CBP_CHROMA_BITS;
      if (!(cbpc & MCBPC_DQUANT)) dq = 0;
      else if (!dquant_walk(io, v)) return 0;
    } else {  // B-VOPs: no bits where the co-located macroblock of the backward reference was skipped
      if (!IO::W) sk = c.skip_ref[idx];
      if (sk) return 1;
      if (IO::W) s = mb == MODB_NONE;
      if (!io.bit(&s)) return 0;
      if (s) {
        mb = MODB_NONE;
        t = MB_DIRECT;
        return blocks_walk(io, 0, 0, 0);
      }
      if (IO::W) s = mb == MODB_NOCBP;
      if (!io.bit(&s)) return 0;
      mb = s ? MODB_NOCBP : MODB_CBP;
      if (!io.vlc(&t, vlc_mb_type_b)) return 0;
      if (mb == MODB_NOCBP) cbp = 0;
      else if (!io.bits(&cbp, MB_BLOCKS)) return 0;
      if (has_dq(v, cbp) && !dquant_walk(io, v)) return 0;
    }
    if (c.mb_has_dt(0, cbp) && !io.bit(&dt)) return 0;
    if (has_fp(c, v) && !io.bit(&fp)) return 0;
    if (fp == 1) {
      nfs = fs_count(v);
      if (!io.bits(&fs, nfs)) return 0;
    }
    if (v.type != PT_B) {  // a vector, a field's each, or a block's each (4MV); none in GMC
      if (t == MB_4MV) {
        if (!mvs_walk(io, &k, MB_LUMA, v.fcode)) return 0;
      } else if (mc != 1 && !mvs_walk(io, &k, fp == 1 ? 2 : 1, v.fcode))
        return 0;
    } else if (t != MB_DIRECT) {  // forward, then backward
      const int n = fp == 1 ? 2 : 1;
      if (fwd_dir(t) && !mvs_walk(io, &k, n, v.fcode)) return 0;
      if (bwd_dir(t) && !mvs_walk(io, &k, n, v.bcode)) return 0;
    } else if (!mvs_walk(io, &k, 1, FCODE_MIN))  // direct mode's delta vector
      return 0;
    nmv = k;
    return blocks_walk(io, cbp, 0, 0);
  }
};
