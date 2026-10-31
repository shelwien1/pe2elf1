// motion.inc -- the vectors as the decoder makes them, m4vdec's (a port of FFmpeg's MPEG-4 Part 2
// decoder): a macroblock's from the differences coded and their prediction (h263_pred_motion, and
// what it does at a video packet's start), a B-VOP's predictors (last_mv) and direct mode (the next
// anchor's vectors scaled by the times between), GMC's vector (get_amv) from the warp
// (mo_sprite.inc), and the display times as FFmpeg keeps them (16 bits). One state for the model,
// which keeps it at every level and codes the differences against its predictions, with the vectors
// around for contexts, and for the decoder in the loop of level -9 (recon.inc), which moves the
// prediction by its vectors.
//
// The model drives it as it codes: vop_begin() at a VOP's header, vp_begin() at a video packet's,
// vop_end() after the VOP; a macroblock's mb_begin(), kind() once its kind is known, its vectors
// (pred() and set(); in a B-VOP bpred() and bset(), or direct()), mb_end().

// FFmpeg's flags: a motion's directions, a macroblock's type (MB_TYPE_*); how its vectors go (one,
// one per 8x8 block, one per field)
enum { MV_DIR_FORWARD = 1, MV_DIR_BACKWARD = 2, MV_DIRECT = 4 };
enum {
  MBT_INTRA = 0x0001, MBT_16x16 = 0x0002, MBT_16x8 = 0x0004, MBT_8x8 = 0x0008, MBT_INTERLACED = 0x0010,
  MBT_DIRECT2 = 0x0020, MBT_GMC = 0x0080, MBT_SKIP = 0x0100, MBT_FWD = 0x0200,
  MBT_BWD = 0x0400, MBT_BIDIR = MBT_FWD | MBT_BWD
};
enum { MV_16X16 = 0, MV_8X8 = 1, MV_FIELD = 2 };

// FFmpeg's MpegEncContext's layout: a row of macroblocks' entries and one more (mb_stride), a row
// of 8x8 blocks' and one more (b8_stride); the tables' padding (MB_PAD entries after the
// macroblocks, MV_PAD vectors before the 8x8 blocks'); a macroblock's field selects (ref_index)
constexpr int MB_PAD = 2, MV_PAD = 4, REF_INDEX = 4;
// direct mode: the co-located vectors' components whose scalings are made once per VOP (-32..31)
constexpr int DIRECT_MVS = 64;
// the field times FFmpeg falls back on, where the field distances come out wrong
constexpr int FIELD_PB_TIME = 2, FIELD_PP_TIME = 4;
// a B-VOP's times where they are out of order (FFmpeg doesn't decode it): halfway, for the model
constexpr int MO_PB_NONE = 1, MO_PP_NONE = 2;

static inline int mid_pred(int a, int b, int c) {
  if (a > b) {
    if (c > b) b = c > a ? a : c;
  } else if (b > c)
    b = c > a ? c : a;
  return b;
}
// FFmpeg's ROUNDED_DIV and RSHIFT
static inline S64 rounded_div(S64 a, S64 b) { return (a >= 0 ? a + (b >> 1) : a - (b >> 1)) / b; }
static inline int rshift(int a, int b) { return a > 0 ? (a + ((1 << b) >> 1)) >> b : (a + ((1 << b) >> 1) - 1) >> b; }

// a picture's vectors on the 8x8 grid (b8_stride a row, MV_PAD before), and what direct mode reads
// of an anchor: its macroblocks' types (MBT_*) and field selects (REF_INDEX a macroblock)
struct MoPic {
  S16 (*mvbase)[2];
  S16 (*mv)[2];
  U16* mbtype;
  U8* ref_index;

  // its tables at m, b8s 8x8 blocks' entries and mbs macroblocks': the bytes after them
  U8* place(U8* m, int b8s, int mbs) {
    mvbase = (S16(*)[2])m;
    m += (size_t)b8s * 2 * sizeof(S16);
    mv = mvbase + MV_PAD;
    mbtype = (U16*)m;
    m += (size_t)mbs * sizeof(U16);
    ref_index = m;
    return m + (size_t)REF_INDEX * mbs;
  }
};

// the sprite's warping: the virtual points' precision (sixteenths of a sample: SPR_SUB), the
// warp's fixed point (FFmpeg's 16 - shift), FFmpeg's rho (3 - the warping accuracy)
constexpr int SPR_SUB = 16, SPR_FIX = 16, SPR_RHO = 3;
// a warp's corners (the VOP's), the number of them
constexpr int SPR_CORNERS = 4;

struct Motion {
  // the stream: the picture's size, its macroblocks and their layout; the VOL's quarter samples,
  // interlacing and warp; the Ctx's counts of VOL and GOP headers applied
  int width, height, mb_w, mb_h, mb_num, mb_stride, b8_stride;
  int qpel, progressive, sprite_usage, warp_points, warp_acc, divx413;
  int vol_seen, gop_seen;
  // the times (decode_vop_header's); a B-VOP's in order (times_ok: FFmpeg decodes it), and the
  // ones direct mode scales by (the halfway ones else), with its scalings of the co-located
  // vectors' components
  int time_base, last_time_base, t_frame;
  S64 time, last_non_b_time;
  U16 pp_time, pb_time, pp_field_time, pb_field_time;
  int times_ok, dpb, dpp;
  S16 direct_scale_mv[2][DIRECT_MVS];
  // GMC's warp (sprite(); sprite_ok 0 where it overflows)
  int sprite_offset[2][2], sprite_delta[2][2], sprite_shift[2], real_warp_points, sprite_ok;
  // the VOP, the video packet, the macroblock (its 8x8 blocks' entries in the grid)
  int pict_type, f_code, b_code, top_field_first;
  int mb_x, mb_y, resync_mb_x, resync_mb_y, first_slice_line, block_index[MB_LUMA];
  // the macroblock's vectors ([forward, backward][block or field][x, y]) as its prediction takes
  // them (mv_dir, mv_type), its field selects, whether it is intra, GMC (mcsel); a B-VOP's
  // predictors ([direction][field][x, y])
  int mv_dir, mv_type, intra, mcsel;
  int mv[2][MB_LUMA][2], field_select[2][2], last_mv[2][2][2];
  S16 (*p_field_mv)[2][2];  // [macroblock][field][x, y]: the last P-VOP's field vectors
  MoPic cur, ref;    // the VOP's, and the last anchor's before it
  U8* mem;           // all of the above, in one block

  void init() { memset(this, 0, sizeof *this); }
  void quit() {
    free(mem);
    mem = 0;
  }

  // the tables for a picture of w x h, zeroed; 0 if there is no memory for them
  int layout(int w, int h) {
    int mb_array, b8_array;
    if (mem && width == w && height == h) return 1;
    free(mem);
    mem = 0;
    width = w;
    height = h;
    mb_w = (w + MB_W - 1) >> MB_SHIFT;
    mb_h = (h + MB_W - 1) >> MB_SHIFT;
    mb_num = mb_w * mb_h;
    mb_stride = mb_w + 1;
    b8_stride = mb_w * 2 + 1;  // (2 8x8 blocks in a macroblock's row)
    mb_array = mb_stride * mb_h + MB_PAD;
    b8_array = b8_stride * mb_h * 2 + MV_PAD;
    {
      const size_t pic = (size_t)b8_array * 2 * sizeof(S16) + (size_t)mb_array * sizeof(U16) + (size_t)REF_INDEX * mb_array;
      U8* m = (U8*)calloc(1, 2 * pic + (size_t)mb_array * 2 * 2 * sizeof(S16));
      if (!m) return 0;
      mem = m;
      m = cur.place(m, b8_array, mb_array);
      m = ref.place(m, b8_array, mb_array);
      p_field_mv = (S16(*)[2][2])m;
    }
    return 1;
  }

#include "mo_sprite.inc"  // sprite(): GMC's warp

  // A VOP's header: its times (a not-coded VOP's too: decode_vop_header's), then, for a coded one, its
  // tables and what its vectors depend on. -1: no memory for the tables
  int vop_begin(const Ctx& cx, const VopHdr& h) {
    int i;
    if (!cx.have_vol || cx.unsupported || cx.width <= 0 || cx.height <= 0) return 0;
    if (!layout(cx.width, cx.height)) return -1;
    if (vol_seen != cx.vol_count) {  // a VOL header since the last VOP
      vol_seen = cx.vol_count;
      t_frame = 0;
    }
    if (gop_seen != cx.gop_count) {
      gop_seen = cx.gop_count;
      time_base = cx.gop_time;
    }
    pict_type = h.type;
    progressive = !cx.interlaced;
    if (h.type != PT_B) {
      last_time_base = time_base;
      time_base += h.mtb;
      time = time_base * (S64)cx.time_res + h.tinc;
      pp_time = (U16)(time - last_non_b_time);
      last_non_b_time = time;
    } else {
      time = (last_time_base + h.mtb) * (S64)cx.time_res + h.tinc;
      pb_time = (U16)(pp_time - (last_non_b_time - time));
      times_ok = !(pp_time <= pb_time || pp_time <= pp_time - pb_time || pp_time <= 0);
      dpb = times_ok ? pb_time : MO_PB_NONE;
      dpp = times_ok ? pp_time : MO_PP_NONE;
      for (i = 0; i < DIRECT_MVS; i++) {
        direct_scale_mv[0][i] = (S16)((i - DIRECT_MVS / 2) * dpb / dpp);
        direct_scale_mv[1][i] = (S16)((i - DIRECT_MVS / 2) * (dpb - dpp) / dpp);
      }
      if (times_ok) {
        if (t_frame == 0) t_frame = pb_time;
        if (t_frame == 0) t_frame = 1;
        pp_field_time = (U16)((rounded_div(last_non_b_time, t_frame) -
                               rounded_div(last_non_b_time - pp_time, t_frame)) * 2);
        pb_field_time = (U16)((rounded_div(time, t_frame) -
                               rounded_div(last_non_b_time - pp_time, t_frame)) * 2);
        if (pp_field_time <= pb_field_time || pb_field_time <= 1) {
          pb_field_time = FIELD_PB_TIME;
          pp_field_time = FIELD_PP_TIME;
          if (!progressive) times_ok = 0;
        }
      }
    }
    if (!h.coded) return 0;
    qpel = cx.qpel;
    sprite_usage = cx.sprite_usage;
    warp_points = cx.warp_points;
    warp_acc = cx.warp_acc;
    top_field_first = !progressive ? h.tff : 0;
    // DivX 4.13's warp: as m4vdec has it, the header's before the encoders' bugs are worked out, after
    // which a DivX build next to an XviD one is forgotten (the video packets' and get_amv's)
    divx413 = cx.divx_version == DIVX413_VERSION && cx.divx_build == DIVX413_BUILD;
    sprite_ok = 1;
    if (pict_type == PT_S) {
      if (sprite_usage == GMC_SPRITE) sprite_ok = sprite(h.warp);
      else {
        memset(sprite_offset, 0, sizeof(sprite_offset));
        memset(sprite_delta, 0, sizeof(sprite_delta));
      }
    }
    if (cx.xvid_build >= 0) divx413 = 0;
    f_code = pict_type != PT_I ? h.fcode : 1;
    b_code = pict_type == PT_B ? h.bcode : 1;
    memset(cur.mvbase, 0, (b8_stride * mb_h * 2 + MV_PAD) * sizeof(*cur.mvbase));
    memset(cur.mbtype, 0, (mb_stride * mb_h + MB_PAD) * sizeof(U16));
    mb_x = mb_y = 0;
    first_slice_line = 1;
    resync_mb_x = resync_mb_y = 0;
    return 0;
  }

  // a video packet starting at macroblock k.mbnum (decode_video_packet_header's and the resync):
  // the trajectory its header repeats, the B-VOP predictors cleared (ff_mpeg4_clean_buffers')
  void vp_begin(const VpHdr& k) {
    if (k.mbnum <= 0 || k.mbnum >= mb_num) return;
    mb_x = k.mbnum % mb_w;
    mb_y = k.mbnum / mb_w;
    if (k.hec && pict_type == PT_S && sprite_usage == GMC_SPRITE) sprite_ok = sprite(k.warp);
    last_mv[0][0][0] = last_mv[0][0][1] = last_mv[1][0][0] = last_mv[1][0][1] = 0;
    first_slice_line = 1;
    resync_mb_x = mb_x;
    resync_mb_y = mb_y;
  }

  // macroblock idx starts (decode_slice's part; a B-VOP's row starts with no predictors)
  void mb_begin(int idx) {
    const int bs = b8_stride;
    mb_x = idx % mb_w;
    mb_y = idx / mb_w;
    block_index[0] = bs * (mb_y * 2) + mb_x * 2;
    block_index[1] = block_index[0] + 1;
    block_index[2] = bs * (mb_y * 2 + 1) + mb_x * 2;
    block_index[3] = block_index[2] + 1;
    if (resync_mb_x == mb_x && resync_mb_y + 1 == mb_y) first_slice_line = 0;
    mv_dir = MV_DIR_FORWARD;
    mv_type = MV_16X16;
    intra = mcsel = 0;
    if (pict_type == PT_B && mb_x == 0) memset(last_mv, 0, sizeof(last_mv));
  }

  // the vector of block i of the macroblock in the last anchor (the co-located one)
  const S16* ref_mv(int i) const { return ref.mv[block_index[i]]; }

  // GMC's vector (component n): the warp's mean over the macroblock, clipped to what f_code reaches
  int amv(int n) const {
    int x, y, mb_v, sum, dx, dy, shift;
    const int len = MV_VLC_MAX << (f_code - FCODE_MIN), a = warp_acc;
    if (real_warp_points == 1) {
      if (divx413 && a >= qpel) sum = sprite_offset[0][n] / (1 << (a - qpel));
      else sum = rshift(sprite_offset[0][n] * (1 << qpel), a);
    } else {
      dx = sprite_delta[n][0];
      dy = sprite_delta[n][1];
      shift = sprite_shift[0];
      if (n) dy -= 1 << (shift + a + 1);
      else dx -= 1 << (shift + a + 1);
      mb_v = (int)(sprite_offset[0][n] + dx * mb_x * (unsigned)MB_W + dy * mb_y * (unsigned)MB_W);
      sum = 0;
      for (y = 0; y < MB_W; y++) {
        int v = (int)(mb_v + (unsigned)dy * y);
        for (x = 0; x < MB_W; x++) {
          sum += v >> shift;
          v += dx;
        }
      }
      sum = rshift(sum, a + 2 * MB_SHIFT - qpel);  // (the mean of the macroblock's samples' vectors)
    }
    if (sum < -len) sum = -len;
    else if (sum >= len) sum = len - 1;
    return sum;
  }

  // The macroblock's kind, from its record m (decode_mb's): skipped (in a P-VOP no vector, or GMC's;
  // in a B-VOP the co-located one was skipped: no vector), intra, GMC, or the vectors it codes (one,
  // one per field: their field selects, one per block; in a B-VOP forward, backward or both, or direct
  // mode)
  void kind(const MB& m) {
    const rp<U16> mbtype = &cur.mbtype[mb_x + mb_y * mb_stride];
    mv_dir = MV_DIR_FORWARD;
    mv_type = MV_16X16;
    if (pict_type != PT_B) {
      const int gmc = pict_type == PT_S && sprite_usage == GMC_SPRITE;
      if (m.t == MB_INTRA && !m.sk) {
        intra = 1;
        *mbtype = MBT_INTRA;
      } else if (m.sk || m.mc == 1) {
        mcsel = gmc;
        *mbtype = (m.sk ? MBT_SKIP : 0) | (mcsel ? MBT_GMC : 0) | MBT_16x16 | MBT_FWD;
        mv[0][0][0] = mcsel ? amv(0) : 0;
        mv[0][0][1] = mcsel ? amv(1) : 0;
      } else if (m.t == MB_4MV) {
        mv_type = MV_8X8;
        *mbtype = MBT_8x8 | MBT_FWD;
      } else if (m.fp == 1) {
        mv_type = MV_FIELD;
        field_select[0][0] = (m.fs >> 1) & 1;
        field_select[0][1] = m.fs & 1;
        *mbtype = MBT_16x8 | MBT_FWD | MBT_INTERLACED;
      } else
        *mbtype = MBT_16x16 | MBT_FWD;
      return;
    }
    if (m.sk) {
      mv[0][0][0] = mv[0][0][1] = mv[1][0][0] = mv[1][0][1] = 0;
      *mbtype = MBT_SKIP | MBT_16x16 | MBT_FWD;
      return;
    }
    mv_dir = 0;
    if (m.mb == MODB_NONE || m.t == MB_DIRECT) {  // (direct() makes the vectors)
      mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
      *mbtype = MBT_DIRECT2 | MBT_BIDIR | (m.mb == MODB_NONE ? MBT_SKIP : 0);
      return;
    }
    mv_dir = (fwd_dir(m.t) ? MV_DIR_FORWARD : 0) | (bwd_dir(m.t) ? MV_DIR_BACKWARD : 0);
    *mbtype = (fwd_dir(m.t) ? MBT_FWD : 0) | (bwd_dir(m.t) ? MBT_BWD : 0);
    if (m.fp == 1) {
      int b = m.nfs, dir;
      mv_type = MV_FIELD;
      *mbtype |= MBT_16x8 | MBT_INTERLACED;
      for (dir = 0; dir < 2; dir++)
        if (mv_dir & (MV_DIR_FORWARD << dir)) {
          field_select[dir][0] = (m.fs >> --b) & 1;
          field_select[dir][1] = (m.fs >> --b) & 1;
        }
    } else
      *mbtype |= MBT_16x16;
  }

  // The prediction of block's vector in a P- or S-VOP (FFmpeg's h263_pred_motion): the median of the
  // left, above and above right blocks' (in a video packet's first row of macroblocks those before it
  // left out: the left one, or none); at the packet's start, block 2 zeroes the left macroblock's
  // vector in the grid, as FFmpeg does (direct mode reads it later). *sx, *sy: the spread of the
  // median's candidates (-1: the prediction is one of them, or none), for the model's contexts.
  void pred(int block, pInt px, pInt py, pInt sx, pInt sy) {
    static const int off[MB_LUMA] = { 2, 1, 1, -1 };  // (the above right candidate's place)
    static const S16 zero[2] = { 0, 0 };
    const int wrap = b8_stride;
    S16 (*mot_val)[2] = cur.mv + block_index[block];
    S16* A = mot_val[-1];
    const S16* c[3] = { A, zero, zero };  // (the left, above, above right: as far as they are taken)
    int n = 3, k;
    if (first_slice_line && block < 3) {
      if (block == 0) {
        if (mb_x == resync_mb_x) n = 0;
        else if (mb_x + 1 == resync_mb_x) {
          c[2] = mot_val[off[block] - wrap];
          if (mb_x == 0) n = 1, c[0] = c[2];
        } else n = 1;
      } else if (block == 1) {
        if (mb_x + 1 == resync_mb_x) c[2] = mot_val[off[block] - wrap];
        else n = 1;
      } else {
        if (mb_x == resync_mb_x) A[0] = A[1] = 0;
        c[1] = mot_val[-wrap];
        c[2] = mot_val[off[block] - wrap];
      }
    } else {
      c[1] = mot_val[-wrap];
      c[2] = mot_val[off[block] - wrap];
    }
    if (n == 3) {
      *px = mid_pred(c[0][0], c[1][0], c[2][0]);
      *py = mid_pred(c[0][1], c[1][1], c[2][1]);
      for (k = 0; k < 2; k++) {
        const int a = c[0][k], b = c[1][k], d = c[2][k];
        (k ? *sy : *sx) = (a > b ? (a > d ? a : d) : (b > d ? b : d)) - (a < b ? (a < d ? a : d) : (b < d ? b : d));
      }
    } else {
      *px = n ? c[0][0] : 0;
      *py = n ? c[0][1] : 0;
      *sx = *sy = -1;
    }
  }

  // a vector from its prediction and its difference as coded (m4vdec's decode_motion)
  static int vector(int p, int diff, int fcode) { return diff ? mv_wrap(p + diff, fcode) : p; }

  // vector i (a block's, a field's; 0: the macroblock's) of direction dir is (mx, my); a block's goes
  // into the grid at once (its neighbours' predictions read it)
  void set(int dir, int i, int mx, int my) {
    mv[dir][i][0] = mx;
    mv[dir][i][1] = my;
    if (mv_type == MV_8X8) {
      const rp<S16> g = cur.mv[block_index[i]];
      g[0] = (S16)mx;
      g[1] = (S16)my;
    }
  }

  // a B-VOP's prediction of the vector of direction dir, field i of n (1: the macroblock's): the last
  // one of that direction in the row (a field's y in frame lines); and the vector coded, which then
  // predicts the next
  void bpred(int dir, int i, int n, pInt px, pInt py) const {
    *px = last_mv[dir][i][0];
    *py = n == 2 ? last_mv[dir][i][1] / 2 : last_mv[dir][i][1];
  }
  void bset(int dir, int i, int n, int mx, int my) {
    mv[dir][i][0] = mx;
    mv[dir][i][1] = my;
    if (n == 2) {
      last_mv[dir][i][0] = mx;
      last_mv[dir][i][1] = my * 2;
    } else {
      last_mv[dir][1][0] = last_mv[dir][0][0] = mx;
      last_mv[dir][1][1] = last_mv[dir][0][1] = my;
    }
  }

  // direct mode's vectors of block i: the co-located block's (p) scaled by the times, plus the delta
  // (mx, my) forward; backward the forward less p where there is a delta, else p scaled the other way
  void one_direct(int mx, int my, pcS16 p, pInt f, pInt b) const {
    constexpr int H = DIRECT_MVS / 2;
    int k;
    for (k = 0; k < 2; k++) {
      const int pk = p[k], dk = k ? my : mx;
      if ((unsigned)(pk + H) < DIRECT_MVS) {
        f[k] = direct_scale_mv[0][pk + H] + dk;
        b[k] = dk ? f[k] - pk : direct_scale_mv[1][pk + H];
      } else {
        f[k] = pk * dpb / dpp + dk;
        b[k] = dk ? f[k] - pk : pk * (dpb - dpp) / dpp;
      }
    }
  }

  // the co-located macroblock's vector scaled as direct mode does without a delta: forward (*fx, *fy)
  // and backward (*bx, *by), for the model's contexts
  void colocated(pInt fx, pInt fy, pInt bx, pInt by) const {
    int f[2], b[2];
    one_direct(0, 0, ref_mv(0), f, b);
    *fx = f[0], *fy = f[1], *bx = b[0], *by = b[1];
  }

  // A direct mode macroblock's vectors, delta (mx, my) (ff_mpeg4_set_direct_mv): per block where the
  // co-located macroblock has one per block, per field where it has field vectors (scaled by the field
  // times, by the fields its selects took), else the macroblock's
  void direct(int mx, int my) {
    const int mb_index = mb_x + mb_y * mb_stride;
    const int coloc = ref.mbtype[mb_index];  // (the co-located macroblock's type)
    const rp<U16> mbtype = &cur.mbtype[mb_index];
    int i;
    if (coloc & MBT_8x8) {
      mv_type = MV_8X8;
      for (i = 0; i < MB_LUMA; i++) one_direct(mx, my, ref_mv(i), mv[0][i], mv[1][i]);
      *mbtype |= MBT_DIRECT2 | MBT_8x8 | MBT_BIDIR;
    } else if (coloc & MBT_INTERLACED) {
      mv_type = MV_FIELD;
      for (i = 0; i < 2; i++) {
        const int fsel = ref.ref_index[REF_INDEX * mb_index + 2 * i];
        const pcS16 pf = p_field_mv[mb_index][i];
        U16 time_pp, time_pb;
        field_select[0][i] = fsel;
        field_select[1][i] = i;
        if (top_field_first) {
          time_pp = (U16)(pp_field_time - fsel + i);
          time_pb = (U16)(pb_field_time - fsel + i);
        } else {
          time_pp = (U16)(pp_field_time + fsel - i);
          time_pb = (U16)(pb_field_time + fsel - i);
        }
        if (!time_pp) time_pp = 1;  // (only in streams FFmpeg would not decode: kept from dividing by 0)
        mv[0][i][0] = pf[0] * time_pb / time_pp + mx;
        mv[0][i][1] = pf[1] * time_pb / time_pp + my;
        mv[1][i][0] = mx ? mv[0][i][0] - pf[0] : pf[0] * (time_pb - time_pp) / time_pp;
        mv[1][i][1] = my ? mv[0][i][1] - pf[1] : pf[1] * (time_pb - time_pp) / time_pp;
      }
      *mbtype |= MBT_DIRECT2 | MBT_16x8 | MBT_BIDIR | MBT_INTERLACED;
    } else {
      one_direct(mx, my, ref_mv(0), mv[0][0], mv[1][0]);
      for (i = 1; i < MB_LUMA; i++) {
        mv[0][i][0] = mv[0][0][0];
        mv[0][i][1] = mv[0][0][1];
        mv[1][i][0] = mv[1][0][0];
        mv[1][i][1] = mv[1][0][1];
      }
      // FFmpeg tests avctx->workaround_bugs here (the user option), not the autodetected flags, so the
      // detected FF_BUG_DIRECT_BLOCKSIZE never takes effect; m4vdec matches that
      mv_type = !qpel ? MV_16X16 : MV_8X8;
      *mbtype |= MBT_DIRECT2 | MBT_16x16 | MBT_BIDIR;
    }
  }

  // the macroblock's end: in an anchor its vectors into the grid, as later macroblocks and B-VOPs read
  // them (ff_h263_update_motion_val): one for the macroblock (none intra; fields': their mean across,
  // their sum down, and the field vectors and selects kept for direct mode)
  void mb_end() {
    const int mb_xy = mb_y * mb_stride + mb_x, wrap = b8_stride, xy = block_index[0];
    const rp<S16[2]> grid = cur.mv;
    int motion_x, motion_y, i;
    if (pict_type == PT_B || mv_type == MV_8X8) return;
    if (intra) {
      motion_x = motion_y = 0;
    } else if (mv_type == MV_16X16) {
      motion_x = mv[0][0][0];
      motion_y = mv[0][0][1];
    } else {
      const pU8 ri = cur.ref_index + REF_INDEX * mb_xy;
      motion_x = mv[0][0][0] + mv[0][1][0];
      motion_y = mv[0][0][1] + mv[0][1][1];
      motion_x = (motion_x >> 1) | (motion_x & 1);
      for (i = 0; i < 2; i++) {
        p_field_mv[mb_xy][i][0] = (S16)mv[0][i][0];
        p_field_mv[mb_xy][i][1] = (S16)mv[0][i][1];
      }
      ri[0] = ri[1] = (U8)field_select[0][0];
      ri[2] = ri[3] = (U8)field_select[0][1];
    }
    grid[xy][0] = grid[xy + 1][0] = grid[xy + wrap][0] = grid[xy + 1 + wrap][0] = (S16)motion_x;
    grid[xy][1] = grid[xy + 1][1] = grid[xy + wrap][1] = grid[xy + 1 + wrap][1] = (S16)motion_y;
  }

  // the VOP's end: an anchor's vectors are the next VOPs' reference
  void vop_end() {
    if (pict_type != PT_B) {
      MoPic t = cur;
      cur = ref;
      ref = t;
    }
  }
};
