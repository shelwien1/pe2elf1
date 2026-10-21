// The pictures as a decoder reconstructs them (H.264 8.2-8.7), for the
// contexts of the model that come from the samples. The model hands over
// each macroblock as it is coded or decoded, so both sides reconstruct the
// same pictures; where the reconstruction differs from a real decoder's
// (streams it does not support), only the contexts suffer.
//
// Supported: frames (no field pictures or MBAFF), 4:2:0, 8 bits, flat
// scaling matrices; CAVLC and CABAC, I, P and B slices, all macroblock
// types, weighted prediction, long-term references, list modification,
// spatial and temporal direct, deblocking. Other streams are not
// reconstructed. SP and SI slices (Extended profile) are reconstructed as
// P and I slices, without SP's requantization (8.6.1, 8.6.2), as ffmpeg
// does: such a picture differs from what a conforming decoder makes, and
// the pictures predicted from it with it, which costs only compression.

namespace h264p {

// --- tables

// 4x4 and 8x8 zigzag scans (frame): scan position -> raster position
const uint8_t kZz4[16] = {0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15};
const uint8_t kZz8[64] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                          41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                          30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
// dequantization (8.5.9): normAdjust4x4 and normAdjust8x8
const int kNorm4[6][3] = {{10, 16, 13}, {11, 18, 14}, {13, 20, 16}, {14, 23, 18}, {16, 25, 20}, {18, 29, 23}};
const int kNorm8[6][6] = {{20, 18, 32, 19, 25, 24}, {22, 19, 35, 21, 28, 26}, {26, 23, 42, 24, 33, 31},
                          {28, 25, 45, 26, 35, 33}, {32, 28, 51, 30, 40, 38}, {36, 32, 58, 34, 46, 43}};
// x264's quantization factors of the 4x4 transform, by qp % 6 and the
// position's class as kNorm4's (the first, the DC's, also the 8x8's and
// the DC blocks')
const int kQuant4[6][3] = {{13107, 5243, 8066}, {11916, 4660, 7490}, {10082, 4194, 6554},
                           {9362, 3647, 5825},  {8192, 3355, 5243},  {7282, 2893, 4559}};
// QPc from qPI (table 8-15) for qPI >= 30
const uint8_t kQpc[22] = {29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39};
// deblocking (tables 8-16, 8-17)
const uint8_t kAlpha[52] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   4,   4,
                            5,  6,  7,  8,  9,  10, 12, 13, 15, 17, 20, 22, 25, 28, 32, 36,  40,  45,
                            50, 56, 63, 71, 80, 90, 101, 113, 127, 144, 162, 182, 203, 226, 255, 255};
const uint8_t kBeta[52] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
                           6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18};
const uint8_t kTc0[52][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                             {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                             {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
                             {0, 1, 1}, {0, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 2},
                             {1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 2, 3}, {1, 2, 3}, {2, 2, 3}, {2, 2, 4},
                             {2, 3, 4}, {2, 3, 4}, {3, 3, 5}, {3, 4, 6}, {3, 4, 6}, {4, 5, 7}, {4, 5, 8},
                             {4, 6, 9}, {5, 7, 10}, {6, 8, 11}, {6, 8, 13}, {7, 10, 14}, {8, 11, 16},
                             {9, 12, 18}, {10, 13, 20}, {11, 15, 23}, {13, 17, 25}};

static int chroma_qp(int qpy, int off) {
  int qpi = clip3(0, 51, qpy + off);
  return qpi < 30 ? qpi : kQpc[qpi - 30];
}

// --- pictures

// A decoded picture, and what decoding later pictures needs of it.
struct RPic {
  int w, h, mbw, mbh;  // luma size (whole macroblocks), in macroblocks
  Buf<uint8_t> y, u, v;
  int64_t id;  // which picture it is (a reference picture of a block is compared by it)
  int poc, frame_num, lt_idx;
  bool ref, lt;  // used for reference, long-term
  // motion of the macroblocks, for direct prediction (8.4.1.2): per
  // macroblock whether intra, per 8x8 block and list refIdx (-1 none) and the
  // id of the picture it refers to, per 4x4 block and list the vector
  Buf<uint8_t> intra;
  Buf<int8_t> rf;     // [(mb * 4 + b8) * 2 + list]
  Buf<int64_t> rpid;  // likewise
  Buf<int16_t> mv;    // [((mb * 16 + blk) * 2 + list) * 2 + comp]
  // the luma again with its edges repeated kPad samples out, so that the
  // reads of motion compensation and template matching near the edges
  // need no clipping (padded: made since the picture was finished)
  static constexpr int kPad = 32;
  Buf<uint8_t> yp;

  // and that transposed, its columns as rows (a template two samples wide
  // and 16 tall is then two rows of 16)
  Buf<uint8_t> yt;
  bool padded;

  void Init() {
    w = h = mbw = mbh = 0;
    y.Init(), u.Init(), v.Init();
    id = 0;
    poc = frame_num = 0, lt_idx = -1;
    ref = lt = false;
    intra.Init(), rf.Init(), rpid.Init(), mv.Init();
    yp.Init(), yt.Init();
    padded = false;
  }
  void Quit() {
    y.Quit(), u.Quit(), v.Quit();
    intra.Quit(), rf.Quit(), rpid.Quit(), mv.Quit();
    yp.Quit(), yt.Quit();
  }
  // a new one, and one freed
  static RPic* make() {
    RPic* p = (RPic*)mem_alloc(sizeof(RPic));
    p->Init();
    return p;
  }
  static void drop(RPic* p) {
    if (!p) return;
    p->Quit();
    mem_free(p);
  }

  void pad_alloc() {
    yp.resize((size_t)(w + 2 * kPad) * (h + 2 * kPad));
    yt.resize(yp.size());
  }
  // the padded copy of the luma, and it transposed (pad_alloc first)
  void pad() {
    int pw = w + 2 * kPad, ph = h + 2 * kPad;
    for (int j = 0; j < ph; j++) {
      const uint8_t* s = y.data() + (size_t)clip3(0, h - 1, j - kPad) * w;
      uint8_t* d = yp.data() + (size_t)j * pw;
      memset(d, s[0], kPad);
      memcpy(d + kPad, s, w);
      memset(d + kPad + w, s[w - 1], kPad);
    }
    for (int j = 0; j < ph; j += 16)
      for (int i = 0; i < pw; i += 16) transpose16(yp.data() + (size_t)j * pw + i, pw, yt.data() + (size_t)i * ph + j, ph);
  }
  // the luma as plane, its stride and how far out it may be read (the
  // padded copy where there is one)
  const uint8_t* luma(int& stride, int& margin) const {
    if (!padded) return stride = w, margin = 0, y.data();
    stride = w + 2 * kPad, margin = kPad;
    return yp.data() + (size_t)kPad * stride + kPad;
  }
  // the padded luma transposed (x down, y across); null none
  const uint8_t* luma_t(int& stride) const {
    stride = h + 2 * kPad;
    return padded ? yt.data() + (size_t)kPad * stride + kPad : nullptr;
  }

  // (px: with the samples; without, the motion alone)
  void alloc(int mw, int mh, bool px) {
    mbw = mw;
    mbh = mh;
    w = mw * 16;
    h = mh * 16;
    if (px) {
      y.fill((size_t)w * h, 0);
      u.fill((size_t)w * h / 4, 128);
      v.fill((size_t)w * h / 4, 128);
    }
    intra.fill((size_t)mw * mh, 1);
    rf.fill((size_t)mw * mh * 8, -1);
    rpid.fill((size_t)mw * mh * 8, -1);
    mv.fill((size_t)mw * mh * 64, 0);
  }
};

// What deblocking needs of a macroblock of the current picture.
struct DbMb {
  int slice;  // slice number in the picture, -1 not decoded
  uint8_t intra, t8, pcm, qp, disable;
  int8_t alpha_off, beta_off, cqp[2];
  uint8_t nz[16];          // 4x4 blocks with coefficients (raster)
  int64_t rp[2][4];        // reference pictures of the 8x8 blocks, -1 none
  int16_t mv[2][16][2];
};

// The decoded picture buffer (8.2.4, 8.2.5): the reference pictures and
// the current one, its POC (8.2.1), the reference lists of the current
// slice and their weights (8.4.2.3); what ReconT<PX> keeps whether it has
// the samples (Recon) or not (ReconRefs), and what it does as a picture
// ends (mark()) and as a slice begins (poc(), build_lists(), weights()).
struct Dpb {
  Buf<RPic*> dpb;    // reference pictures (16 at most: 15 and the last)
  RPic* cur;         // the picture being decoded
  Buf<RPic*> spare;  // pictures to reuse
  int64_t next_id;
  // the current picture's header values that marking needs
  int cur_idr, cur_ref_idc, cur_lt_flag, cur_mmco5;
  int cur_nmmco;
  Mmco cur_mmco[68];
  int max_frame_num, max_num_ref, max_lt_idx;  // max_lt_idx: MaxLongTermFrameIdx, -1 none
  // POC state (8.2.1)
  int prev_poc_msb, prev_poc_lsb, prev_frame_num;
  int64_t prev_frame_num_offset;
  // the current slice
  RPic* list[2][33];
  int nlist[2];
  int curr_pic_num;
  // weighted prediction of the slice: per list and reference, luma w, o,
  // Cb w, o, Cr w, o; logWD luma, chroma; 0 default, 1 explicit, 2 implicit
  int wmode, lwd, cwd;
  int wt[2][32][6];
  int implicit_w[32][32];  // implicit bi-prediction weight of list 0 (64 - w for list 1)

  void Init() {
    dpb.Init();
    cur = nullptr;
    spare.Init();
    next_id = 1;
    cur_idr = cur_ref_idc = cur_lt_flag = cur_mmco5 = 0;
    cur_nmmco = 0;
    max_frame_num = 16, max_num_ref = 1, max_lt_idx = -1;
    prev_poc_msb = prev_poc_lsb = prev_frame_num = 0;
    prev_frame_num_offset = 0;
    memset(list, 0, sizeof list);
    nlist[0] = nlist[1] = 0;
    curr_pic_num = 0;
    wmode = lwd = cwd = 0;
  }
  void Quit() {
    drop_refs();
    dpb.Quit();
    for (RPic* p : spare) RPic::drop(p);
    spare.Quit();
    RPic::drop(cur);
    cur = nullptr;
  }
  // the reference pictures gone
  void drop_refs() {
    for (RPic* p : dpb) RPic::drop(p);
    dpb.clear();
  }

  int pic_num(const RPic* p) const { return p->lt ? p->lt_idx : (p->frame_num > cur->frame_num ? p->frame_num - max_frame_num : p->frame_num); }

  void mark() {
    RPic* c = cur;
    if (cur_idr) {
      drop_refs();
      if (cur_lt_flag) {
        c->lt = true;
        c->lt_idx = 0;
        max_lt_idx = 0;
      } else {
        max_lt_idx = -1;
      }
      c->ref = true;
      return;
    }
    if (cur_nmmco) {
      for (int i = 0; i < cur_nmmco; i++) {
        const Mmco& op = cur_mmco[i];
        int64_t a = op.n > 0 ? op.arg[0] : 0;
        switch (op.op) {
          case 1: {  // a short-term picture no longer used for reference
            int pn = curr_pic_num - (int)(a + 1);
            for (RPic* p : dpb)
              if (p->ref && !p->lt && pic_num(p) == pn) p->ref = false;
            break;
          }
          case 2:  // a long-term picture
            for (RPic* p : dpb)
              if (p->ref && p->lt && p->lt_idx == a) p->ref = false;
            break;
          case 3: {  // a short-term picture becomes long-term
            int pn = curr_pic_num - (int)(a + 1);
            int idx = (int)(op.n > 1 ? op.arg[1] : 0);
            for (RPic* p : dpb)
              if (p->ref && p->lt && p->lt_idx == idx) p->ref = false;
            for (RPic* p : dpb)
              if (p->ref && !p->lt && pic_num(p) == pn) {
                p->lt = true;
                p->lt_idx = idx;
              }
            break;
          }
          case 4:
            max_lt_idx = (int)a - 1;
            for (RPic* p : dpb)
              if (p->ref && p->lt && p->lt_idx > max_lt_idx) p->ref = false;
            break;
          case 5:
            for (RPic* p : dpb) p->ref = false;
            max_lt_idx = -1;
            break;
          case 6:
            for (RPic* p : dpb)
              if (p->ref && p->lt && p->lt_idx == a) p->ref = false;
            c->lt = true;
            c->lt_idx = (int)a;
            break;
        }
      }
    } else {
      // sliding window
      int ns = 0, nl = 0;
      for (RPic* p : dpb)
        if (p->ref) (p->lt ? nl : ns)++;
      if (ns + nl >= Max(max_num_ref, 1) && ns > 0) {
        RPic* old = nullptr;
        for (RPic* p : dpb)
          if (p->ref && !p->lt && (!old || pic_num(p) < pic_num(old))) old = p;
        if (old) old->ref = false;
      }
    }
    c->ref = true;
    // pictures no longer used for reference go, and the oldest beyond the
    // 15 a conforming stream keeps besides the current one (a stream that
    // does not conform could make it keep any number)
    size_t keep = 0;  // (those kept move up, in order)
    for (size_t i = 0; i < dpb.size(); i++)
      if (dpb[i]->ref)
        dpb[keep++] = dpb[i];
      else
        spare.push_back(dpb[i]);
    dpb.n = keep;
    if (keep > 15) {
      for (size_t i = 0; i + 15 < keep; i++) spare.push_back(dpb[i]);
      dpb.erase_front(keep - 15);
    }
    if (cur_mmco5) {
      c->frame_num = 0;
      c->poc = 0;
    }
  }

  // POC of the current picture (8.2.1), held within +-(2^30 - 1): a stream
  // the spec allows has POC differences of 16 bits, and held so, any two
  // POCs subtract in an int
  static int poc_sat(int64_t v) {
    const int64_t m = ((int64_t)1 << 30) - 1;
    return (int)(v < -m ? -m : v > m ? m : v);
  }
  int poc(const SliceHeader& h) {
    const Sps& sps = *h.sps;
    bool idr = h.nal_unit_type == 5;
    if (sps.poc_type == 0) {
      int64_t max_lsb = (int64_t)1 << sps.log2_max_poc_lsb;
      int64_t pm = idr ? 0 : prev_poc_msb, pl = idr ? 0 : prev_poc_lsb, lsb = h.poc_lsb;
      int64_t msb = pm;
      if (lsb < pl && pl - lsb >= max_lsb / 2)
        msb = pm + max_lsb;
      else if (lsb > pl && lsb - pl > max_lsb / 2)
        msb = pm - max_lsb;
      int64_t top = msb + lsb, bot = top + h.delta_poc_bottom;
      if (h.nal_ref_idc) {
        prev_poc_msb = poc_sat(msb);
        prev_poc_lsb = (int)lsb;
        if (h.mmco5) {
          prev_poc_msb = 0;
          prev_poc_lsb = poc_sat(top - Min(top, bot));
        }
      }
      return poc_sat(Min(top, bot));
    }
  // types 1 and 2: FrameNumOffset
    int64_t fno = idr ? 0 : prev_frame_num_offset + (prev_frame_num > h.frame_num ? max_frame_num : 0);
    int64_t top, bot;
    if (sps.poc_type == 2) {
      top = bot = idr ? 0 : 2 * (fno + h.frame_num) - (h.nal_ref_idc == 0);
    } else {
      int n = sps.num_ref_frames_in_poc_cycle;
      int64_t abs_fn = n ? fno + h.frame_num : 0;
      if (h.nal_ref_idc == 0 && abs_fn > 0) abs_fn--;
      int64_t exp = 0;
      if (abs_fn > 0) {
        int64_t delta = 0;
        for (int i = 0; i < n; i++) delta += sps.offset_for_ref_frame[i];
        int64_t cycle = (abs_fn - 1) / n, in = (abs_fn - 1) % n;
        exp = cycle * delta;
        for (int i = 0; i <= in; i++) exp += sps.offset_for_ref_frame[i];
      }
      if (h.nal_ref_idc == 0) exp += sps.offset_for_non_ref_pic;
      top = exp + h.delta_poc[0];
      bot = top + sps.offset_for_top_to_bottom_field + h.delta_poc[1];
    }
    prev_frame_num_offset = h.mmco5 ? 0 : fno;
    prev_frame_num = h.mmco5 ? 0 : h.frame_num;
    return poc_sat(Min(top, bot));
  }

  // pictures p[0, n) in order by less, where they are not, in their
  // order before (an insertion sort: std::sort's for up to 16, as here)
  template <class Less>
  static void sort_pics(RPic** p, int n, Less less) {
    for (int i = 1; i < n; i++) {
      RPic* v = p[i];
      int j = i;
      for (; j > 0 && less(v, p[j - 1]); j--) p[j] = p[j - 1];
      p[j] = v;
    }
  }
  // the reference lists of a slice (8.2.4.2, 8.2.4.3)
  void build_lists(const SliceHeader& h) {
    nlist[0] = nlist[1] = 0;
    int t = h.type();
    if (t == kI || t == kSI) return;
    RPic *st[16], *lt[16];  // (the reference pictures: 16 at most)
    int nst = 0, nlt = 0;
    for (RPic* p : dpb)
      if (p->ref) (p->lt ? lt[nlt++] : st[nst++]) = p;
    sort_pics(lt, nlt, [](const RPic* a, const RPic* b) { return a->lt_idx < b->lt_idx; });
    RPic* l[2][33];  // (and one more, inserted)
    int nl[2] = {0, 0};
    if (t == kB) {
      RPic *before[16], *after[16];
      int nb = 0, na = 0;
      for (int i = 0; i < nst; i++) (st[i]->poc < cur->poc ? before[nb++] : after[na++]) = st[i];
      sort_pics(before, nb, [](const RPic* a, const RPic* b) { return a->poc > b->poc; });
      sort_pics(after, na, [](const RPic* a, const RPic* b) { return a->poc < b->poc; });
      for (int i = 0; i < nb; i++) l[0][nl[0]++] = before[i];
      for (int i = 0; i < na; i++) l[0][nl[0]++] = after[i];
      for (int i = 0; i < nlt; i++) l[0][nl[0]++] = lt[i];
      for (int i = 0; i < na; i++) l[1][nl[1]++] = after[i];
      for (int i = 0; i < nb; i++) l[1][nl[1]++] = before[i];
      for (int i = 0; i < nlt; i++) l[1][nl[1]++] = lt[i];
      if (nl[1] > 1 && !memcmp(l[1], l[0], nl[1] * sizeof(RPic*))) {
        RPic* p = l[1][0];
        l[1][0] = l[1][1], l[1][1] = p;
      }
    } else {
      sort_pics(st, nst, [this](const RPic* a, const RPic* b) { return pic_num(a) > pic_num(b); });
      for (int i = 0; i < nst; i++) l[0][nl[0]++] = st[i];
      for (int i = 0; i < nlt; i++) l[0][nl[0]++] = lt[i];
    }
    for (int li = 0; li < (t == kB ? 2 : 1); li++) {
      // (32 at most, as the parser reads it: list[][] and the weights are of
      // that many)
      int n = Min(h.num_ref_idx_active[li], 32);
      RPic** L = l[li];
      int& nL = nl[li];
      for (; nL < n; nL++) L[nL] = nullptr;
      nL = n;
      // modifications
      int pred = curr_pic_num, ri = 0;
      for (int m = 0; m < h.nrplm[li]; m++) {
        const Rplm& op = h.rplm[li][m];
        if (ri >= n) break;
        RPic* pic = nullptr;
        if (op.idc < 2) {
          int d = (int)op.arg + 1;
          int nw = op.idc == 0 ? pred - d : pred + d;
          if (nw < 0) nw += max_frame_num;
          if (nw >= max_frame_num) nw -= max_frame_num;
          pred = nw;
          int pn = nw > curr_pic_num ? nw - max_frame_num : nw;
          for (int i = 0; i < nst; i++)
            if (pic_num(st[i]) == pn) pic = st[i];
        } else if (op.idc == 2) {
          for (int i = 0; i < nlt; i++)
            if (lt[i]->lt_idx == op.arg) pic = lt[i];
        }
        if (!pic) continue;
        for (int k = nL; k > ri; k--) L[k] = L[k - 1];  // (inserted at ri)
        L[ri++] = pic;
        nL++;
        for (int k = ri; k < nL; k++)
          if (L[k] == pic) {  // (and taken out after)
            for (; k + 1 < nL; k++) L[k] = L[k + 1];
            nL--;
            break;
          }
        nL = n;
      }
      nlist[li] = n;
      for (int k = 0; k < n; k++) list[li][k] = L[k];
    }
  }

  // the weights of the slice (8.4.2.3)
  void weights(const SliceHeader& h, const Pps& pps) {
    int t = h.type();
    wmode = 0;
    if ((t == kP || t == kSP) && pps.weighted_pred_flag) wmode = 1;
    if (t == kB && pps.weighted_bipred_idc == 1) wmode = 1;
    if (t == kB && pps.weighted_bipred_idc == 2) wmode = 2;
    if (wmode == 1) {
      lwd = Min(h.luma_log2_wd, 7);  // (7 at most, as the parser reads them)
      cwd = Min(h.chroma_log2_wd, 7);
      for (int l = 0; l < 2; l++)
        for (int i = 0; i < h.num_ref_idx_active[l] && i < 32; i++)
          for (int k = 0; k < 6; k++) wt[l][i][k] = i * 6 + k < h.nwt[l] ? h.wt[l][i * 6 + k] : 0;
    } else if (wmode == 2) {
      for (int i0 = 0; i0 < nlist[0] && i0 < 32; i0++)
        for (int i1 = 0; i1 < nlist[1] && i1 < 32; i1++) {
          const RPic* p0 = list[0][i0];
          const RPic* p1 = list[1][i1];
          int w = 32;
          if (p0 && p1 && !p0->lt && !p1->lt) {
            int tb = clip3(-128, 127, cur->poc - p0->poc), td = clip3(-128, 127, p1->poc - p0->poc);
            if (td) {
              int tx = (16384 + abs(td / 2)) / td;
              int dsf = clip3(-1024, 1023, (tb * tx + 32) >> 6);
              if ((dsf >> 2) >= -64 && (dsf >> 2) <= 128) w = 64 - (dsf >> 2);
            }
          }
          implicit_w[i0][i1] = w;
        }
    }
  }
};

// --- The model's maps of the last picture finished (h264p_cm.md): per
// macroblock the energy of its luma (4 log2 of 1 + the sum of squared
// deviations), the mean of that over the picture, and how much the means
// of its 4x4 blocks changed from the picture before (16 x the mean
// absolute difference; -1 none); of w x h macroblocks (0 x 0 none yet)
struct PicMaps {
  Buf<int> energy, change;
  int energy_mean, w, h;
  Buf<int> means;                        // 16 x the means of the 4x4 blocks, for the next picture's change
  Buf<int> energy_n, change_n, means_n;  // (the next picture's, made in place)

  void Init() {
    energy.Init(), change.Init();
    energy_mean = w = h = 0;
    means.Init();
    energy_n.Init(), change_n.Init(), means_n.Init();
  }
  void Quit() {
    energy.Quit(), change.Quit();
    means.Quit();
    energy_n.Quit(), change_n.Quit(), means_n.Quit();
  }

  // 4 log2 v, rounded down to quarters (integers only: the model must come
  // out the same everywhere)
  static int log2q(int64_t v) {
    int n = 0;
    while (v >> (n + 1)) n++;
    return 4 * n + (n >= 2 ? (int)(v >> (n - 2) & 3) : n == 1 ? (int)(v & 1) * 2 : 0);
  }
  // a picture finished, of mbw x mbh macroblocks, its luma y (stride ys),
  // deblocked: its maps are the model's
  void add(const uint8_t* y, int ys, int mbw, int mbh) {
    int n = mbw * mbh;
    bool same = w == mbw && h == mbh && (int)means.size() == n * 16;
    energy_n.resize(n);
    change_n.fill(n, -1);
    means_n.resize((size_t)n * 16);
    int64_t sum = 0;
    for (int my = 0, a = 0; my < mbh; my++)
      for (int mx = 0; mx < mbw; mx++, a++) {
        int64_t s1, s2;
        int* m = &means_n[a * 16];  // (16 x the means of its 4x4 blocks)
        blk_sums(y + my * 16 * ys + mx * 16, ys, 16, s1, s2, m);
        int64_t ssd = (s2 * 256 - s1 * s1) / 256;  // the sum of squared deviations (s1² / 256 up; aq_energy rounds it down)
        energy_n[a] = log2q(ssd + 1);
        sum += energy_n[a];
        if (same) {
          int d = 0;
          for (int b = 0; b < 16; b++) d += abs(m[b] - means[a * 16 + b]);
          change_n[a] = d / 16;
        }
      }
    energy_mean = n ? (int)(sum / n) : 0;
    energy.swap(energy_n);
    change.swap(change_n);
    means.swap(means_n);
    w = mbw, h = mbh;
  }
};

// --- The decoder in the loop. K, what a level has of it (h264p_levels.md
// section 6): 2 the pictures with their samples (Recon); 1 the pictures'
// motion, POCs and reference lists alone, which direct prediction and the
// model's reference contexts need (ReconRefs); 0 nothing (ReconOff).
// What a level has not is a constant "not known", as while no picture is
// active: motion() and act() are constants false there, and the code that
// needs them folds away.
//
// The model drives it in the walk's order: slice_begin(); per macroblock
// mb_start(), prepare() as soon as its type, intra modes and motion are
// known (the first call counts; an inter macroblock's set_t8() comes
// after it), coef() per block of coefficients as they are coded (pcm_s
// for I_PCM), luma_ready() and chroma_ready() before a block's
// coefficients (the blocks before it are then in the picture) and
// luma_add() and chroma_add() after, luma_finish() where all of the luma
// is wanted, and mb_end(); end() as the stream ends. The model's queries
// (ReconT) see the macroblock as far as that has got.
template <int K>
struct MbDecoder : Dpb {
  static constexpr bool REFS = K >= 1, PX = K == 2;
  // a picture is current: its motion and reference lists are known
  // (motion()), and its samples (act())
  bool mact;
  bool motion() const { return REFS && mact; }
  bool act() const { return PX && mact; }
  // Where things are in the current picture: macroblock addr's first
  // column and row in a plane of m samples a macroblock (16 luma, 8
  // chroma, 1 the macroblocks themselves); plane c (0 Y, 1 Cb, 2 Cr) and
  // its stride; macroblock addr's top-left sample in plane c; the sample
  // at (x, y) of the current macroblock (mb_addr) in plane c
  int mb_x(int addr, int m = 16) const { return addr % cur->mbw * m; }
  int mb_y(int addr, int m = 16) const { return addr / cur->mbw * m; }
  uint8_t* plane(int c) const { return c == 0 ? cur->y.data() : c == 1 ? cur->u.data() : cur->v.data(); }
  int stride(int c) const { return c ? cur->w / 2 : cur->w; }
  uint8_t* mb_px(int addr, int c) const { return plane(c) + mb_y(addr, c ? 8 : 16) * stride(c) + mb_x(addr, c ? 8 : 16); }
  uint8_t* mb_at(int c, int x, int y) const { return mb_px(mb_addr, c) + y * stride(c) + x; }
  Buf<DbMb> db;
  int slice_no;
  int cqp_off[2];
  int disable, alpha_off, beta_off;
  bool constrained_intra, direct8x8;
  // what the model sees of the last picture finished
  PicMaps maps;
  // H264P_RECDUMP's file (finish_picture()), opened as the first picture is
  struct RecDump {
    FILE* f;
    bool tried;
    void Init() {
      f = nullptr;
      tried = false;
    }
    void Quit() {
      if (f) fclose(f);
      f = nullptr;
    }
    FILE* get() {
      if (!tried) {
        tried = true;
        const char* fn = getenv("H264P_RECDUMP");
        if (fn) f = fopen(fn, "wb");
      }
      return f;
    }
  } recdump;

  void Init() {
    Dpb::Init();
    mact = false;
    db.Init();
    slice_no = 0;
    cqp_off[0] = cqp_off[1] = 0;
    disable = alpha_off = beta_off = 0;
    constrained_intra = false, direct8x8 = true;
    maps.Init();
    recdump.Init();
    mb_addr = 0;
    mb_cls = mb_t8 = 0;
    mb_modes = nullptr;
    mb_prepared = mb_dc = false;
    skip_in = false;
    mb_done4 = 0;
    mb_done8 = 0;
    pq_pic = -1;
    pq_addr = -1;
    pq_ok = false;
    deblock = true;
  }
  void Quit() {
    Dpb::Quit();
    db.Quit();
    maps.Quit();
    recdump.Quit();
  }

  // --- finishing the picture as its decoding ends: deblocking it, then
  // its maps and (a reference picture's) padded luma. Until
  // then only the decoding reads the picture, as decoded, unfiltered, and
  // the deblocking records of the macroblocks above and to the left.
  // (Finished row by row two rows behind the decoding, as when a thread of
  // its own did it, it measured no faster on the test streams:
  // h264p_review.md, D3.)
  static DbMb db_none() {  // (zeros, but slice -1)
    DbMb d;
    memset((void*)&d, 0, sizeof d);
    d.slice = -1;
    return d;
  }
  // (the decoding's side) a picture begins
  void fin_begin() {
    if (cur_ref_idc) cur->pad_alloc();
  }
  // the picture ends
  void fin_end() {
    for (int row = 0; row < cur->mbh; row++) deblock_row(row);
    maps.add(cur->y.data(), cur->w, cur->mbw, cur->mbh);
    if (cur_ref_idc) cur->pad();
    db.fill(db.size(), db_none());  // (for the next picture)
    if (cur_ref_idc) cur->padded = true;
  }

  // --- reference picture management (Dpb), as a picture ends

  // the end of the stream: its last picture finished too
  void end() { finish_picture(); }
  // an inter macroblock's transform size, which comes after its prediction
  // (prepare()): its 8x8 blocks then go to the picture as they come
  void set_t8(int t8) {
    if (act() && mb_prepared) mb_t8 = t8;
  }
  void finish_picture() {
    if (!REFS || !cur) return;
    if (act()) fin_end();
    // checking the reconstruction against a decoder's: H264P_RECDUMP=file
    // gets the pictures, in decoding order, as raw 4:2:0 of whole macroblocks
    // (this model's: c or d without -t, the only model of the call)
    FILE* dump = act() ? recdump.get() : nullptr;
    if (dump) {
      fwrite(cur->y.data(), 1, cur->y.size(), dump);
      fwrite(cur->u.data(), 1, cur->u.size(), dump);
      fwrite(cur->v.data(), 1, cur->v.size(), dump);
    }
    if (cur_ref_idc) mark();
    if (cur->ref)  // (fin_end() padded it)
      dpb.push_back(cur);
    else
      spare.push_back(cur);
    cur = nullptr;
    mact = false;
    nlist[0] = nlist[1] = 0;  // marking may have dropped the pictures they hold
  }

  // --- reconstruction of a macroblock

  // its residual, as the model codes it: the coefficients in scan order
  struct Res {
    int y4[16][16];  // 4x4 blocks by luma4x4BlkIdx; AC blocks from position 1
    int y8[4][64];
    int dc[16];      // Intra16x16 DC
    int cdc[2][4];   // chroma DC
    int cac[2][4][16];
    uint16_t m4;     // which blocks are there
    uint8_t m8, mdc, mcdc, mcac[2];
  } res;
  int pcm_s[384];
  int mb_addr;
  void mb_start(int addr) {
    if (!REFS) return;
    // the motion of the next macroblock's colocated one (direct prediction),
    // on its way into the cache
    if (motion() && nlist[1] && list[1][0] && addr + 1 < list[1][0]->mbw * list[1][0]->mbh) {
      const RPic* c = list[1][0];
      int na = addr + 1;
      CM_PREFETCH(&c->intra[na]);
      CM_PREFETCH(&c->rf[(size_t)na * 8]);
      CM_PREFETCH(&c->rpid[(size_t)na * 8]);
      CM_PREFETCH(&c->mv[(size_t)na * 64]);
      CM_PREFETCH(&c->mv[(size_t)na * 64 + 32]);
    }
    if (act()) {
      // what mb_end will write of the macroblock, on its way into the cache
      const char* d = (const char*)&db[addr];
      for (int k = 0; k < (int)sizeof(DbMb); k += 64) CM_PREFETCHW(d + k);
      CM_PREFETCHW(d + sizeof(DbMb) - 1);
      CM_PREFETCHW(&cur->mv[(size_t)addr * 64]);
      CM_PREFETCHW(&cur->mv[(size_t)addr * 64 + 32]);
      CM_PREFETCHW(&cur->rpid[(size_t)addr * 8]);
      CM_PREFETCHW(&cur->rf[(size_t)addr * 8]);
    }
    mb_addr = addr;
    res.m4 = 0;
    res.m8 = res.mdc = res.mcdc = res.mcac[0] = res.mcac[1] = 0;
    mb_prepared = mb_dc = false;
    skip_in = false;
    mb_done4 = 0;
    mb_done8 = 0;
    mb_cdone[0] = mb_cdone[1] = 0;
    mb_cdc[0] = mb_cdc[1] = false;
    // the first references' samples at the next macroblock (the vector
    // zero), on their way into the cache for that one's prediction (rows of
    // 16 and 8 aligned so, each in a line)
    int na = addr + 1;
    if (act() && na < cur->mbw * cur->mbh)
      for (int l = 0; l < 2; l++) {
        const RPic* R = nlist[l] ? list[l][0] : nullptr;
        if (!R || R->mbw != cur->mbw || R->mbh != cur->mbh) continue;
        int ps, pm, x = mb_x(na), y = mb_y(na), cw = R->w / 2;
        const uint8_t* p = R->luma(ps, pm) + y * ps + x;
        for (int j = 0; j < 16; j++) CM_PREFETCH(p + j * ps);
        const uint8_t *u = R->u.data() + y / 2 * cw + x / 2, *v = R->v.data() + y / 2 * cw + x / 2;
        for (int j = 0; j < 8; j++) CM_PREFETCH(u + j * cw), CM_PREFETCH(v + j * cw);
      }
  }
  void coef(int cat, int idx, const int* c, int n) {
    if (!PX) return;
    switch (cat) {
      case 0:
        for (int i = 0; i < 16; i++) res.dc[i] = c[i];
        res.mdc = 1;
        break;
      case 1:
        res.y4[idx][0] = 0;
        for (int i = 0; i < 15; i++) res.y4[idx][i + 1] = c[i];
        res.m4 |= (uint16_t)(1 << idx);
        break;
      case 2:
        for (int i = 0; i < 16; i++) res.y4[idx][i] = c[i];
        res.m4 |= (uint16_t)(1 << idx);
        break;
      case 3:
        for (int i = 0; i < 4 && i < n; i++) res.cdc[idx & 1][i] = c[i];
        res.mcdc |= (uint8_t)(1 << (idx & 1));
        break;
      case 4: {
        int comp = idx >> 2 & 1, b = idx & 3;
        res.cac[comp][b][0] = 0;
        for (int i = 0; i < 15; i++) res.cac[comp][b][i + 1] = c[i];
        res.mcac[comp] |= (uint8_t)(1 << b);
        break;
      }
      case 5:
        for (int i = 0; i < 64; i++) res.y8[idx][i] = c[i];
        res.m8 |= (uint8_t)(1 << idx);
        break;
    }
  }

  // inverse transforms (8.5.12.2, 8.5.13.2) of dequantized coefficients d
  // (raster), added to the prediction at p
  static void idct4_add(uint8_t* p, int stride, int* d) {
    for (int i = 0; i < 4; i++) {
      int* r = d + i * 4;
      int e0 = r[0] + r[2], e1 = r[0] - r[2], e2 = (r[1] >> 1) - r[3], e3 = r[1] + (r[3] >> 1);
      r[0] = e0 + e3;
      r[1] = e1 + e2;
      r[2] = e1 - e2;
      r[3] = e0 - e3;
    }
    for (int j = 0; j < 4; j++) {
      int g0 = d[j], g1 = d[4 + j], g2 = d[8 + j], g3 = d[12 + j];
      int e0 = g0 + g2, e1 = g0 - g2, e2 = (g1 >> 1) - g3, e3 = g1 + (g3 >> 1);
      p[j] = (uint8_t)clip1(p[j] + ((e0 + e3 + 32) >> 6));
      p[stride + j] = (uint8_t)clip1(p[stride + j] + ((e1 + e2 + 32) >> 6));
      p[2 * stride + j] = (uint8_t)clip1(p[2 * stride + j] + ((e1 - e2 + 32) >> 6));
      p[3 * stride + j] = (uint8_t)clip1(p[3 * stride + j] + ((e0 - e3 + 32) >> 6));
    }
  }
  static void idct8_1d(int* x, int s) {
    int d0 = x[0], d1 = x[s], d2 = x[2 * s], d3 = x[3 * s], d4 = x[4 * s], d5 = x[5 * s], d6 = x[6 * s], d7 = x[7 * s];
    int a0 = d0 + d4, a4 = d0 - d4, a2 = (d2 >> 1) - d6, a6 = d2 + (d6 >> 1);
    int b0 = a0 + a6, b2 = a4 + a2, b4 = a4 - a2, b6 = a0 - a6;
    int a1 = -d3 + d5 - d7 - (d7 >> 1), a3 = d1 + d7 - d3 - (d3 >> 1);
    int a5 = -d1 + d7 + d5 + (d5 >> 1), a7 = d3 + d5 + d1 + (d1 >> 1);
    int b1 = a1 + (a7 >> 2), b7 = a7 - (a1 >> 2), b3 = a3 + (a5 >> 2), b5 = (a3 >> 2) - a5;
    x[0] = b0 + b7;
    x[s] = b2 + b5;
    x[2 * s] = b4 + b3;
    x[3 * s] = b6 + b1;
    x[4 * s] = b6 - b1;
    x[5 * s] = b4 - b3;
    x[6 * s] = b2 - b5;
    x[7 * s] = b0 - b7;
  }
  static void idct8_add(uint8_t* p, int stride, int* d) {
    for (int i = 0; i < 8; i++) idct8_1d(d + i * 8, 1);
    for (int j = 0; j < 8; j++) idct8_1d(d + j, 8);
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 8; j++) p[i * stride + j] = (uint8_t)clip1(p[i * stride + j] + ((d[i * 8 + j] + 32) >> 6));
  }
  // dequantization (8.5.12.1, flat matrices) of a 4x4 block in scan order
  // into raster order; from position `first` (1: the DC comes separately)
  static void dequant4(const int* c, int* d, int qp, int first) {
    int q6 = qp / 6, m = qp % 6;
    for (int i = 0; i < 16; i++) d[i] = 0;
    for (int k = first; k < 16; k++) {
      if (!c[k]) continue;
      int r = kZz4[k], y = r >> 2, x = r & 3;
      int ls = 16 * kNorm4[m][norm4_class(y, x)];
      d[r] = qp >= 24 ? c[k] * ls * (1 << (q6 - 4)) : (c[k] * ls + (1 << (3 - q6))) >> (4 - q6);
    }
  }
  // the class of the coefficient at row y, column x of a 4x4 block, and of
  // an 8x8 one: which of normAdjust's columns (8.5.9) and of the forward
  // quantizer's it takes
  static int norm4_class(int y, int x) { return (x & 1) && (y & 1) ? 1 : !(x & 1) && !(y & 1) ? 0 : 2; }
  static int norm8_class(int y, int x) {
    if (x % 4 == 0 && y % 4 == 0) return 0;
    if (x % 2 == 1 && y % 2 == 1) return 1;
    if (x % 4 == 2 && y % 4 == 2) return 2;
    if ((y % 4 == 0 && x % 2 == 1) || (y % 2 == 1 && x % 4 == 0)) return 3;
    if ((y % 4 == 0 && x % 4 == 2) || (y % 4 == 2 && x % 4 == 0)) return 4;
    return 5;
  }
  static void dequant8(const int* c, int* d, int qp) {
    int q6 = qp / 6, m = qp % 6;
    for (int i = 0; i < 64; i++) d[i] = 0;
    for (int k = 0; k < 64; k++) {
      if (!c[k]) continue;
      int r = kZz8[k], ls = 16 * kNorm8[m][norm8_class(r >> 3, r & 7)];
      d[r] = qp >= 36 ? c[k] * ls * (1 << (q6 - 6)) : (c[k] * ls + (1 << (5 - q6))) >> (6 - q6);
    }
  }

  // neighbouring macroblocks for intra prediction: decoded, in the same
  // slice, and not inter with constrained_intra_pred
  int nb_mb(int addr, int dx, int dy) const {
    int x = mb_x(addr, 1) + dx, y = mb_y(addr, 1) + dy;
    if (x < 0 || x >= cur->mbw || y < 0) return -1;
    return y * cur->mbw + x;
  }
  bool intra_avail(int addr, int n) const {
    return n >= 0 && db[n].slice == db[addr].slice && (!constrained_intra || db[n].intra);
  }

  // Intra4x4 and Intra8x8 prediction of an n x n block (n 4 or 8) from its
  // edge: e[0] p[-1,-1], e[1 + x] p[x,-1] (x < 2n), e[1 + 2n + y] p[-1,y]
  static void pred_nxn(uint8_t* d, int stride, int n, int mode, const int* e, bool top, bool left) {
    const int* T = e + 1;
    const int* L = e + 1 + 2 * n;
    auto P = [&](int x, int y) { return y < 0 ? (x < 0 ? e[0] : T[x]) : L[y]; };
    // (by mode, each a loop of its own)
#define H264P_PRED(EXPR)                                \
  for (int y = 0; y < n; y++, d += stride)              \
    for (int x = 0; x < n; x++) d[x] = (uint8_t)(EXPR); \
  return
    switch (mode) {
      case 0:
        H264P_PRED(T[x]);
      case 1:
        H264P_PRED(L[y]);
      case 2: {
        int st = 0, sl = 0;
        for (int i = 0; i < n; i++) st += T[i], sl += L[i];
        int sh = n == 4 ? 2 : 3;
        int dc = top && left ? (st + sl + n) >> (sh + 1) : left ? (sl + n / 2) >> sh : top ? (st + n / 2) >> sh : 128;
        H264P_PRED(dc);
      }
      case 3:
        H264P_PRED(x == n - 1 && y == n - 1 ? (T[2 * n - 2] + 3 * T[2 * n - 1] + 2) >> 2
                                             : (T[x + y] + 2 * T[x + y + 1] + T[x + y + 2] + 2) >> 2);
      case 4:
        H264P_PRED(x > y   ? (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >> 2
                   : x < y ? (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >> 2
                           : (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2);
      case 5: {
        auto v5 = [&](int x, int y) {
          int z = 2 * x - y;
          if (z >= 0 && !(z & 1)) return (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >> 1;
          if (z >= 0) return (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 2) >> 2;
          if (z == -1) return (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          return (P(-1, y - 2 * x - 1) + 2 * P(-1, y - 2 * x - 2) + P(-1, y - 2 * x - 3) + 2) >> 2;
        };
        H264P_PRED(v5(x, y));
      }
      case 6: {
        auto v6 = [&](int x, int y) {
          int z = 2 * y - x;
          if (z >= 0 && !(z & 1)) return (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >> 1;
          if (z >= 0) return (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 2) >> 2;
          if (z == -1) return (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
          return (P(x - 2 * y - 1, -1) + 2 * P(x - 2 * y - 2, -1) + P(x - 2 * y - 3, -1) + 2) >> 2;
        };
        H264P_PRED(v6(x, y));
      }
      case 7:
        H264P_PRED(!(y & 1) ? (T[x + (y >> 1)] + T[x + (y >> 1) + 1] + 1) >> 1
                            : (T[x + (y >> 1)] + 2 * T[x + (y >> 1) + 1] + T[x + (y >> 1) + 2] + 2) >> 2);
      default: {
        auto v8 = [&](int x, int y) {
          int z = x + 2 * y, lim = 2 * n - 3;
          if (z < lim && !(z & 1)) return (L[y + (x >> 1)] + L[y + (x >> 1) + 1] + 1) >> 1;
          if (z < lim) return (L[y + (x >> 1)] + 2 * L[y + (x >> 1) + 1] + L[y + (x >> 1) + 2] + 2) >> 2;
          if (z == lim) return (L[n - 2] + 3 * L[n - 1] + 2) >> 2;
          return L[n - 1];
        };
        H264P_PRED(v8(x, y));
      }
    }
#undef H264P_PRED
  }

  // the edge of the luma n x n block at (bx, by) (in units of n) of the
  // macroblock at addr; n 4 or 8. Returns whether the top and left are there.
  void luma_edge(int addr, int n, int bx, int by, int* e, bool& top, bool& left) const {
    int mx = mb_x(addr), my = mb_y(addr);
    int x0 = mx + bx * n, y0 = my + by * n;
    bool aA = intra_avail(addr, nb_mb(addr, -1, 0)), aB = intra_avail(addr, nb_mb(addr, 0, -1));
    bool aC = intra_avail(addr, nb_mb(addr, 1, -1)), aD = intra_avail(addr, nb_mb(addr, -1, -1));
    int per = 16 / n;  // blocks in a row of the macroblock
    left = bx > 0 || aA;
    top = by > 0 || aB;
    bool tl = bx > 0 ? (by > 0 || aB) : (by > 0 ? aA : aD);
    bool tr;
    if (by == 0)
      tr = bx < per - 1 ? aB : aC;
    else if (bx == per - 1)
      tr = false;
    else if (n == 8)
      tr = by == 1 && bx == 0;  // the top right 8x8 block is decoded before the bottom left one
    else {
      // inside the macroblock: decoded before, in zigzag order?
      int cur_i = (by >> 1) * 8 + (bx >> 1) * 4 + (by & 1) * 2 + (bx & 1);
      int nx = bx + 1, ny = by - 1;
      int nb_i = (ny >> 1) * 8 + (nx >> 1) * 4 + (ny & 1) * 2 + (nx & 1);
      tr = nb_i < cur_i;
    }
    int w = cur->w;
    const uint8_t* Y = cur->y.data();
    edge_fill(n, x0, y0, top, left, tl, tr, e, [&](int x, int y) { return (int)Y[y * w + x]; });
  }
  // the edge of an n x n block at (x0, y0) of a picture, its samples at(x,
  // y), where its top, left, top left and top right are there (else 128,
  // or for the top right the last top sample); filtered where n is 8
  template <class F>
  static void edge_fill(int n, int x0, int y0, bool top, bool left, bool tl, bool tr, int* e, F at) {
    e[0] = tl ? at(x0 - 1, y0 - 1) : 128;
    for (int i = 0; i < n; i++) e[1 + i] = top ? at(x0 + i, y0 - 1) : 128;
    for (int i = 0; i < n; i++) e[1 + n + i] = tr ? at(x0 + n + i, y0 - 1) : e[n];
    for (int i = 0; i < n; i++) e[1 + 2 * n + i] = left ? at(x0 - 1, y0 + i) : 128;
    if (n == 8) filter8(e, top, left, tl);
  }
  // reference sample filtering (8.3.2.2.1)
  static void filter8(int* e, bool top, bool left, bool tl) {
    int f[25];
    for (int i = 0; i < 25; i++) f[i] = e[i];
    if (top) {
      f[1] = tl ? (e[0] + 2 * e[1] + e[2] + 2) >> 2 : (3 * e[1] + e[2] + 2) >> 2;
      for (int x = 1; x < 15; x++) f[1 + x] = (e[x] + 2 * e[1 + x] + e[2 + x] + 2) >> 2;
      f[16] = (e[15] + 3 * e[16] + 2) >> 2;
    }
    if (tl) {
      if (!top || !left)
        f[0] = top ? (3 * e[0] + e[1] + 2) >> 2 : left ? (3 * e[0] + e[17] + 2) >> 2 : e[0];
      else
        f[0] = (e[1] + 2 * e[0] + e[17] + 2) >> 2;
    }
    if (left) {
      f[17] = tl ? (e[0] + 2 * e[17] + e[18] + 2) >> 2 : (3 * e[17] + e[18] + 2) >> 2;
      for (int y = 1; y < 7; y++) f[17 + y] = (e[16 + y] + 2 * e[17 + y] + e[18 + y] + 2) >> 2;
      f[24] = (e[23] + 3 * e[24] + 2) >> 2;
    }
    for (int i = 0; i < 25; i++) e[i] = f[i];
  }

  void pred16(int addr, int mode) {
    int w = cur->w, mx = mb_x(addr), my = mb_y(addr);
    uint8_t* Y = cur->y.data();
    uint8_t* d = Y + my * w + mx;
    bool aA = intra_avail(addr, nb_mb(addr, -1, 0)), aB = intra_avail(addr, nb_mb(addr, 0, -1));
    bool aD = intra_avail(addr, nb_mb(addr, -1, -1));
    int T[16], L[16], Q;
    around(d, w, 16, aB, aA, aD, T, L, Q);
    pred_l16(d, w, mode, T, L, Q, aA, aB);
  }
  // the n samples above d (T), to its left (L) and the one above left (Q),
  // where there are (top, left, tl), else 128
  static void around(const uint8_t* d, int stride, int n, bool top, bool left, bool tl, int* T, int* L, int& Q) {
    Q = tl ? d[-stride - 1] : 128;
    for (int i = 0; i < n; i++) {
      T[i] = top ? d[-stride + i] : 128;
      L[i] = left ? d[i * stride - 1] : 128;
    }
  }
  // the Intra16x16 prediction from the samples above (T), to the left (L)
  // and above left (Q)
  static void pred_l16(uint8_t* d, int stride, int mode, const int* T, const int* L, int Q, bool aA, bool aB) {
    if (mode == 3) {
      int H = 0, V = 0;
      for (int i = 0; i < 8; i++) {
        H += (i + 1) * (T[8 + i] - (i == 7 ? Q : T[6 - i]));
        V += (i + 1) * (L[8 + i] - (i == 7 ? Q : L[6 - i]));
      }
      int a = 16 * (L[15] + T[15]), b = (5 * H + 32) >> 6, c = (5 * V + 32) >> 6;
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) d[y * stride + x] = (uint8_t)clip1((a + b * (x - 7) + c * (y - 7) + 16) >> 5);
      return;
    }
    int dc = 128;
    if (mode == 2) {
      int st = 0, sl = 0;
      for (int i = 0; i < 16; i++) st += T[i], sl += L[i];
      dc = aA && aB ? (st + sl + 16) >> 5 : aA ? (sl + 8) >> 4 : aB ? (st + 8) >> 4 : 128;
    }
    for (int y = 0; y < 16; y++)
      for (int x = 0; x < 16; x++) d[y * stride + x] = (uint8_t)(mode == 0 ? T[x] : mode == 1 ? L[y] : dc);
  }

  void pred_chroma(int addr, int mode) {
    int cw = cur->w / 2, mx = mb_x(addr, 8), my = mb_y(addr, 8);
    bool aA = intra_avail(addr, nb_mb(addr, -1, 0)), aB = intra_avail(addr, nb_mb(addr, 0, -1));
    bool aD = intra_avail(addr, nb_mb(addr, -1, -1));
    for (int c = 0; c < 2; c++) {
      uint8_t* d = (c ? cur->v.data() : cur->u.data()) + my * cw + mx;
      int T[8], L[8], Q;
      around(d, cw, 8, aB, aA, aD, T, L, Q);
      pred_c8(d, cw, mode, T, L, Q, aA, aB);
    }
  }
  // the chroma prediction of an 8x8 block (4:2:0) from the samples above
  // (T), to the left (L) and above left (Q)
  static void pred_c8(uint8_t* d, int stride, int mode, const int* T, const int* L, int Q, bool aA, bool aB) {
    if (mode == 0) {  // DC, per 4x4 block
      for (int b = 0; b < 4; b++) {
        int xo = (b & 1) * 4, yo = (b >> 1) * 4, st = 0, sl = 0;
        for (int i = 0; i < 4; i++) st += T[xo + i], sl += L[yo + i];
        int v;
        if (xo == yo)
          v = aA && aB ? (st + sl + 4) >> 3 : aA ? (sl + 2) >> 2 : aB ? (st + 2) >> 2 : 128;
        else if (yo == 0)
          v = aB ? (st + 2) >> 2 : aA ? (sl + 2) >> 2 : 128;
        else
          v = aA ? (sl + 2) >> 2 : aB ? (st + 2) >> 2 : 128;
        for (int y = 0; y < 4; y++)
          for (int x = 0; x < 4; x++) d[(yo + y) * stride + xo + x] = (uint8_t)v;
      }
    } else if (mode == 3) {
      int H = 0, V = 0;
      for (int i = 0; i < 4; i++) {
        H += (i + 1) * (T[4 + i] - (i == 3 ? Q : T[2 - i]));
        V += (i + 1) * (L[4 + i] - (i == 3 ? Q : L[2 - i]));
      }
      int a = 16 * (L[7] + T[7]), b = (34 * H + 32) >> 6, cc = (34 * V + 32) >> 6;
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) d[y * stride + x] = (uint8_t)clip1((a + b * (x - 3) + cc * (y - 3) + 16) >> 5);
    } else {
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) d[y * stride + x] = (uint8_t)(mode == 1 ? L[y] : T[x]);
    }
  }

  // --- inter prediction (8.4.2.2): a block of w x h luma samples at (x, y)
  // of ref, displaced by mv (quarter samples), into o (stride 16)
  static void mc_luma(const RPic* ref, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
    int ps, pm;
    const uint8_t* RP = ref->luma(ps, pm);
    mc_luma_p(RP, ps, pm, ref->w, ref->h, x, y, w, h, mvx, mvy, o);
  }
  // the same of the transposed luma (the prediction is the same transposed,
  // the interpolation being symmetric): o its h x w transposed (stride 16);
  // false where ref has no transposed luma
  static bool mc_luma_t(const RPic* ref, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
    int pt;
    const uint8_t* RT = ref->luma_t(pt);
    if (!RT) return false;
    mc_luma_p(RT, pt, RPic::kPad, ref->h, ref->w, y, x, h, w, mvy, mvx, o);
    return true;
  }
  // chroma: w x h samples at (x, y) of plane c (0 Cb, 1 Cr), mv in eighths
  static void mc_chroma(const RPic* ref, int c, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
    int W = ref->w / 2, H = ref->h / 2;
    const uint8_t* R = c ? ref->v.data() : ref->u.data();
    int xi = x + (mvx >> 3), yi = y + (mvy >> 3), fx = mvx & 7, fy = mvy & 7;
    if (!fx && !fy && xi >= 0 && yi >= 0 && xi + w <= W && yi + h <= H) {
      copy_rows(o, 8, R + yi * W + xi, W, w, h);
      return;
    }
    const uint8_t* r;
    int rs = W;
    uint8_t t[9 * 16];
    if (xi >= 0 && yi >= 0 && xi + w < W && yi + h < H) {
      r = R + yi * W + xi;
    } else {
      // (out of the plane, or at its last row or column: the samples and
      // the row and the column after them, those outside the nearest
      // inside)
      int cx[9];
      for (int i = 0; i <= w; i++) cx[i] = clip3(0, W - 1, xi + i);
      for (int j = 0; j <= h; j++) {
        const uint8_t* s = R + clip3(0, H - 1, yi + j) * W;
        for (int i = 0; i <= w; i++) t[j * 16 + i] = s[cx[i]];
      }
      r = t, rs = 16;
    }
    if (!fx && !fy) {
      copy_rows(o, 8, r, rs, w, h);
      return;
    }
    int cA = (8 - fx) * (8 - fy), cB = fx * (8 - fy), cC = (8 - fx) * fy, cD = fx * fy;
#ifdef H264P_AVX2
    if (w == 8 || w == 4) {
      // two rows at a time: the byte pairs (r, r') and (s, s') of a row and
      // the next through pmaddubsw
      auto ld = [w](const uint8_t* p0, const uint8_t* p1) { return ld8x2(p0, p1, w); };
      const __m128i wab = _mm_set1_epi16((short)(cA | cB << 8)), wcd = _mm_set1_epi16((short)(cC | cD << 8));
      const __m128i r32 = _mm_set1_epi16(32);
      for (int j = 0; j < h; j += 2, r += 2 * rs) {
        __m128i v = bil_rows(ld(r, r + rs), ld(r + 1, r + rs + 1), ld(r + rs, r + 2 * rs), ld(r + rs + 1, r + 2 * rs + 1), wab, wcd, r32, 6);
        if (w == 8) {
          _mm_storel_epi64((__m128i*)(o + j * 8), v);
          _mm_storel_epi64((__m128i*)(o + j * 8 + 8), _mm_unpackhi_epi64(v, v));
        } else {
          int32_t u0 = _mm_cvtsi128_si32(v), u1 = _mm_cvtsi128_si32(_mm_srli_si128(v, 4));
          memcpy(o + j * 8, &u0, 4);
          memcpy(o + j * 8 + 8, &u1, 4);
        }
      }
      return;
    }
#endif
    for (int j = 0; j < h; j++, r += rs)
      for (int i = 0; i < w; i++)
        o[j * 8 + i] = (uint8_t)bil1(cA, cB, cC, cD, r[i], r[i + 1], r[i + rs], r[i + rs + 1], 32, 6);
  }
  // the residual of the luma 4x4 block blk (luma4x4BlkIdx) added at p
  void add4(uint8_t* p, int stride, int blk, int qp, const int* dcv) {
    if (!(res.m4 >> blk & 1) && !dcv) return;
    int d[16];
    if (res.m4 >> blk & 1)
      dequant4(res.y4[blk], d, qp, dcv ? 1 : 0);
    else
      for (int i = 0; i < 16; i++) d[i] = 0;
    if (dcv) d[0] = *dcv;
    idct4_add(p, stride, d);
  }

  // The macroblock being reconstructed: what its prediction needs, from
  // prepare() on, and which of its luma blocks are done.
  int mb_cls, mb_t8;
  const uint8_t* mb_modes;
  bool mb_prepared, mb_dc;
  // skip_miss put the prediction by this motion in the picture
  bool skip_in;
  int8_t skip_rf[2][4];
  int16_t skip_mv[2][16][2];
  uint16_t mb_done4;  // 4x4 blocks (luma4x4BlkIdx) done
  uint8_t mb_done8;
  int mb_dcr[16];         // Intra16x16 DC, scaled (raster)
  uint8_t mb_cdone[2];    // chroma blocks done, per component
  bool mb_cdc[2];         // their DC scaled
  int mb_dcc[2][4];

  // whether the macroblock above the current one is decoded, and the one
  // to its left
  bool above_done() const { return mb_addr >= cur->mbw && db[mb_addr - cur->mbw].slice >= 0; }
  bool left_done() const { return mb_x(mb_addr, 1) > 0 && db[mb_addr - 1].slice >= 0; }

  // The prediction of the macroblock, as soon as its type, intra modes and
  // motion are known: inter and Intra16x16 luma and the chroma go to the
  // picture now, Intra4x4/8x8 luma block by block (luma_ready).
  void prepare(int cls, int i16m, const uint8_t* modes, int cpred, int t8, const int8_t rf[2][4],
               const int16_t mv[2][16][2]) {
    if (!act() || mb_prepared) return;
    mb_prepared = true;
    mb_cls = cls;
    mb_modes = modes;
    mb_t8 = t8;
    int addr = mb_addr;
    DbMb& D = db[addr];
    D.slice = slice_no;
    D.intra = cls >= 6;
    if (cls == 8) return;
    if (cls < 6) {
      if (!skip_in || memcmp(skip_rf, rf, sizeof skip_rf) || memcmp(skip_mv, mv, sizeof skip_mv))
        inter(addr, rf, mv, mb_px(addr, 0), mb_px(addr, 1), mb_px(addr, 2));
      return;
    }
    if (cls == 7) pred16(addr, i16m);
    pred_chroma(addr, cpred);
  }

  // an Intra4x4 (Intra8x8: blk the 8x8 block) block about to be coded: the
  // ones before it reconstructed, its prediction in the picture
  void luma_ready(int blk, int qp) {
    if (!act() || !mb_prepared || mb_cls != 6) return;
    if (mb_t8) {
      for (int b8 = 0; b8 <= blk; b8++) {
        if (mb_done8 >> b8 & 1) continue;
        intra8(b8);
        if (b8 == blk) return;
        add8(b8, qp);
      }
    } else {
      for (int b = 0; b <= blk; b++) {
        if (mb_done4 >> b & 1) continue;
        intra4(b);
        if (b == blk) return;
        add4b(b, qp);
      }
    }
  }
  void intra4(int b) {
    int bx = blk_x(b), by = blk_y(b), e[17];
    bool top, left;
    luma_edge(mb_addr, 4, bx, by, e, top, left);
    pred_nxn(mb_at(0, bx * 4, by * 4), cur->w, 4, mb_modes[by * 4 + bx], e, top, left);
  }
  void intra8(int b8) {
    int bx = b8 & 1, by = b8 >> 1, e[25];
    bool top, left;
    luma_edge(mb_addr, 8, bx, by, e, top, left);
    pred_nxn(mb_at(0, bx * 8, by * 8), cur->w, 8, mb_modes[by * 8 + bx * 2], e, top, left);
  }
  // the residual of a luma block, its prediction being in the picture
  void add4b(int b, int qp) {
    if (mb_cls == 7 && !mb_dc) dc16(qp);
    add4(mb_at(0, blk_x(b) * 4, blk_y(b) * 4), cur->w, b, qp, mb_cls == 7 ? &mb_dcr[blk_y(b) * 4 + blk_x(b)] : nullptr);
    mb_done4 |= (uint16_t)(1 << b);
  }
  void add8(int b8, int qp) {
    if (res.m8 >> b8 & 1) {
      int d[64];
      dequant8(res.y8[b8], d, qp);
      idct8_add(mb_at(0, (b8 & 1) * 8, (b8 >> 1) * 8), cur->w, d);
    }
    mb_done8 |= (uint8_t)(1 << b8);
  }
  // a luma block's coefficients are known (cat as the walk's, blk its index)
  void luma_add(int cat, int blk, int qp) {
    if (!act() || !mb_prepared || mb_cls == 8) return;
    if (cat == 1 || cat == 2) {
      if (!mb_t8 && !(mb_done4 >> blk & 1)) add4b(blk, qp);
    } else if (cat == 5) {
      if (mb_t8 && !(mb_done8 >> blk & 1)) add8(blk, qp);
    }
  }
  // the luma of the macroblock, all of it (its coefficients coded; t8
  // known by then, where mb_t8 may be from before)
  void luma_finish(int qp, int t8) {
    if (!act() || !mb_prepared || mb_cls == 8) return;
    if (mb_cls == 6) luma_ready(t8 ? 3 : 15, qp);
    if (t8) {
      for (int b8 = 0; b8 < 4; b8++)
        if (!(mb_done8 >> b8 & 1)) add8(b8, qp);
    } else {
      for (int b = 0; b < 16; b++)
        if (!(mb_done4 >> b & 1)) add4b(b, qp);
    }
  }
  // chroma: the DC of a component (2x2 Hadamard), scaled
  void dcc(int c, int qpc) {
    mb_cdc[c] = true;
    for (int b = 0; b < 4; b++) mb_dcc[c][b] = 0;
    if (!(res.mcdc >> c & 1)) return;
    int f[4];
    hadamard2x2(res.cdc[c], f);
    int ls = 16 * kNorm4[qpc % 6][0], q6 = qpc / 6;
    for (int b = 0; b < 4; b++) mb_dcc[c][b] = (f[b] * ls * (1 << q6)) >> 5;
  }
  int chroma_qp_of(int c, int qp) const { return chroma_qp(qp, cqp_off[c]); }
  // the residual of chroma block b of component c
  void addc(int c, int b, int qp) {
    int qpc = chroma_qp_of(c, qp);
    if (!mb_cdc[c]) dcc(c, qpc);
    mb_cdone[c] |= (uint8_t)(1 << b);
    if (!(res.mcac[c] >> b & 1) && !mb_dcc[c][b]) return;
    int d[16];
    if (res.mcac[c] >> b & 1)
      dequant4(res.cac[c][b], d, qpc, 1);
    else
      for (int i = 0; i < 16; i++) d[i] = 0;
    d[0] = mb_dcc[c][b];
    idct4_add(mb_at(c + 1, (b & 1) * 4, (b >> 1) * 4), cur->w / 2, d);
  }
  // a chroma AC block about to be coded: the ones before it done
  void chroma_ready(int c, int b, int qp) {
    if (!act() || !mb_prepared || mb_cls == 8) return;
    for (int k = 0; k < b; k++)
      if (!(mb_cdone[c] >> k & 1)) addc(c, k, qp);
  }
  void chroma_add(int c, int b, int qp) {
    if (!act() || !mb_prepared || mb_cls == 8) return;
    if (!(mb_cdone[c] >> b & 1)) addc(c, b, qp);
  }
  int dcc_at(int c, int b, int qp) {
    if (!mb_cdc[c]) dcc(c, chroma_qp_of(c, qp));
    return mb_dcc[c][b];
  }

  // the inverse Hadamard of the Intra16x16 DC (8.5.10), then their scaling
  void dc16(int qp) {
    mb_dc = true;
    for (int i = 0; i < 16; i++) mb_dcr[i] = 0;
    if (!res.mdc) return;
    int c[16], f[16];
    for (int k = 0; k < 16; k++) c[kZz4[k]] = res.dc[k];
    for (int i = 0; i < 4; i++) hadamard4(c + i * 4, 1, f + i * 4, 1);
    for (int j = 0; j < 4; j++) hadamard4(f + j, 4, c + j, 4);
    int ls = 16 * kNorm4[qp % 6][0], q6 = qp / 6;
    for (int i = 0; i < 16; i++) mb_dcr[i] = qp >= 36 ? c[i] * ls * (1 << (q6 - 6)) : (c[i] * ls + (1 << (5 - q6))) >> (6 - q6);
  }

  // (the samples of the macroblock, and what deblocking needs of it)
  void mb_pixels(int addr, int cls, int i16m, const uint8_t* modes, int cpred, int t8, int qp, const int8_t rf[2][4],
                 const int16_t mv[2][16][2], const uint8_t nz[16]) {
    prepare(cls, i16m, modes, cpred, t8, rf, mv);
    int w = cur->w, cw = w / 2;
    uint8_t *Y = mb_px(addr, 0), *U = mb_px(addr, 1), *V = mb_px(addr, 2);
    bool intra = cls >= 6, pcm = cls == 8;
    DbMb& D = db[addr];
    D.pcm = pcm;
    D.t8 = (uint8_t)t8;
    D.qp = (uint8_t)qp;
    D.disable = (uint8_t)disable;
    D.alpha_off = (int8_t)alpha_off;
    D.beta_off = (int8_t)beta_off;
    D.cqp[0] = (int8_t)cqp_off[0];
    D.cqp[1] = (int8_t)cqp_off[1];
    if (pcm) {
      for (int i = 0; i < 256; i++) Y[(i >> 4) * w + (i & 15)] = (uint8_t)pcm_s[i];
      for (int i = 0; i < 64; i++) U[(i >> 3) * cw + (i & 7)] = (uint8_t)pcm_s[256 + i];
      for (int i = 0; i < 64; i++) V[(i >> 3) * cw + (i & 7)] = (uint8_t)pcm_s[320 + i];
    } else if (!intra && !res.m4 && !res.m8 && !res.mcdc && !res.mcac[0] && !res.mcac[1]) {
      // (no residual: the prediction is the macroblock)
      if (t8)
        mb_done8 = 15;
      else
        mb_done4 = 0xFFFF;
      mb_cdone[0] = mb_cdone[1] = 15;
    } else {
      luma_finish(qp, t8);
      // the chroma blocks left
      for (int c = 0; c < 2; c++)
        for (int b = 0; b < 4; b++)
          if (!(mb_cdone[c] >> b & 1)) addc(c, b, qp);
    }
    // for deblocking, and for the direct prediction of later pictures
    for (int b = 0; b < 16; b++) D.nz[b] = nz[b] != 0;
    if (t8 && !pcm)
      for (int b8 = 0; b8 < 4; b8++) {
        int i = (b8 >> 1) * 8 + (b8 & 1) * 2;
        uint8_t any = D.nz[i] | D.nz[i + 1] | D.nz[i + 4] | D.nz[i + 5];
        D.nz[i] = D.nz[i + 1] = D.nz[i + 4] = D.nz[i + 5] = any;
      }
  }
  // The macroblock is coded or skipped: what is left of it (the luma blocks
  // without coefficients, the chroma residual, I_PCM samples), and what
  // deblocking and the direct prediction of later pictures need. nz: the
  // walk's coefficient counts.
  void mb_end(int addr, int cls, int i16m, const uint8_t* modes, int cpred, int t8, int qp, const int8_t rf[2][4],
              const int16_t mv[2][16][2], const uint8_t nz[16]) {
    if (!motion()) return;
    bool intra = cls >= 6;
    if (PX) mb_pixels(addr, cls, i16m, modes, cpred, t8, qp, rf, mv, nz);
    // for the direct prediction of later pictures (and deblocking)
    cur->intra[addr] = intra;
    for (int l = 0; l < 2; l++) {
      for (int b8 = 0; b8 < 4; b8++) {
        int r = intra ? -1 : rf[l][b8];
        const RPic* rp = r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
        if (PX) db[addr].rp[l][b8] = rp ? rp->id : -1;
        cur->rf[(addr * 4 + b8) * 2 + l] = (int8_t)(rp ? r : -1);
        cur->rpid[(addr * 4 + b8) * 2 + l] = rp ? rp->id : -1;
      }
      if (PX) {
        if (intra)
          memset(db[addr].mv[l], 0, sizeof db[addr].mv[l]);
        else
          memcpy(db[addr].mv[l], mv[l], sizeof db[addr].mv[l]);
      }
    }
    // the picture's vectors: by block, those of both lists in turn
    int16_t* cm = &cur->mv[(size_t)addr * 64];
    if (intra) {
      memset(cm, 0, 64 * sizeof(int16_t));
    } else {
#ifdef H264P_AVX2
      for (int h = 0; h < 2; h++) {
        __m256i a = _mm256_loadu_si256((const __m256i*)mv[0][8 * h]), b = _mm256_loadu_si256((const __m256i*)mv[1][8 * h]);
        __m256i lo = _mm256_unpacklo_epi32(a, b), hi = _mm256_unpackhi_epi32(a, b);
        _mm256_storeu_si256((__m256i*)(cm + 32 * h), _mm256_permute2x128_si256(lo, hi, 0x20));
        _mm256_storeu_si256((__m256i*)(cm + 32 * h + 16), _mm256_permute2x128_si256(lo, hi, 0x31));
      }
#else
      for (int l = 0; l < 2; l++)
        for (int b = 0; b < 16; b++) memcpy(cm + (b * 2 + l) * 2, mv[l][b], 2 * sizeof(int16_t));
#endif
    }
  }

  // The predictions of a macroblock by each list, before the weights (luma
  // 16 x 16, chroma 8 x 8): as inter() made them last, for the motion
  // (pq_rf, pq_mv) of macroblock pq_addr of picture pq_pic; to be had again
  // (by bi_diff, then inter, of a skipped macroblock) where none of the
  // lists predicts from the current picture (a missing reference), which
  // changes
  uint8_t pq_l[2][256], pq_c[2][2][64];
  int64_t pq_pic;
  int pq_addr;
  bool pq_ok;
  int8_t pq_rf[2][4];
  int16_t pq_mv[2][16][2];
  bool predicted(int addr, const int8_t rf[2][4], const int16_t mv[2][16][2]) const {
    return pq_ok && pq_pic == cur->id && pq_addr == addr && !memcmp(pq_rf, rf, sizeof pq_rf) && !memcmp(pq_mv, mv, sizeof pq_mv);
  }
  void predict(int addr, const int8_t rf[2][4], const int16_t mv[2][16][2]) {
    int mx = mb_x(addr), my = mb_y(addr);
    uint8_t(*pl)[256] = pq_l;
    uint8_t(*pc)[2][64] = pq_c;
    pq_pic = cur->id, pq_addr = addr, pq_ok = true;
    memcpy(pq_rf, rf, sizeof pq_rf);
    memcpy(pq_mv, mv, sizeof pq_mv);
    for (int l = 0; l < 2; l++) {
      bool any = false;
      for (int b8 = 0; b8 < 4; b8++) any |= rf[l][b8] >= 0;
      if (!any) continue;
      // in pieces of equal motion: the macroblock, 8x8 blocks, 4x4 blocks
      bool whole = rf[l][1] == rf[l][0] && rf[l][2] == rf[l][0] && rf[l][3] == rf[l][0] && one_mv(mv[l]);
      for (int b8 = 0; b8 < 4; b8++) {
        if (whole && b8) break;
        int r = rf[l][b8];
        if (r < 0) continue;
        const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
        if (!ref) ref = cur, pq_ok = false;  // a missing reference: whatever is there
        int x0 = (b8 & 1) * 2, y0 = (b8 >> 1) * 2, n = whole ? 4 : 2;
        bool same = true;
        if (!whole)
          for (int k = 1; k < 4; k++) {
            int b = (y0 + (k >> 1)) * 4 + x0 + (k & 1);
            same &= mv[l][b][0] == mv[l][y0 * 4 + x0][0] && mv[l][b][1] == mv[l][y0 * 4 + x0][1];
          }
        int step = whole || same ? n : 1;
        for (int y = y0; y < y0 + n; y += step)
          for (int x = x0; x < x0 + n; x += step) {
            const int16_t* m = mv[l][y * 4 + x];
            mc_luma(ref, mx + x * 4, my + y * 4, step * 4, step * 4, m[0], m[1], pl[l] + y * 4 * 16 + x * 4);
            for (int c = 0; c < 2; c++)
              mc_chroma(ref, c, mx / 2 + x * 2, my / 2 + y * 2, step * 2, step * 2, m[0], m[1], pc[l][c] + y * 2 * 8 + x * 2);
          }
      }
    }
  }
  // How weighted sample prediction (8.4.2.3) makes a block with references
  // r0 and r1 (-1 none; one at least) of component k (0 Y, 2 Cb, 4 Cr; lw
  // its logWD) from its predictions a (list 0) and b (list 1): with
  // weights, (a m0 + b m1 + r) >> sh, plus o, clipped, where bi, else
  // (p m0 + r) >> sh, plus o, p the prediction of list l; without (plain),
  // (a + b + 1) >> 1 where bi, else p
  struct Wp {
    bool bi, plain;
    int l, m0, m1, r, sh, o;
  };
  Wp wp_of(int r0, int r1, int k, int lw) const {
    Wp q = {r0 >= 0 && r1 >= 0, false, r0 >= 0 ? 0 : 1, 0, 0, 0, 0, 0};
    int r = q.l ? r1 : r0;
    if (q.bi && wmode == 1) {
      q.m0 = wt[0][r0 & 31][k], q.m1 = wt[1][r1 & 31][k];
      q.r = 1 << lw, q.sh = lw + 1, q.o = (wt[0][r0 & 31][k + 1] + wt[1][r1 & 31][k + 1] + 1) >> 1;
    } else if (q.bi && wmode == 2) {
      q.m0 = implicit_w[r0 & 31][r1 & 31], q.m1 = 64 - q.m0;
      q.r = 32, q.sh = 6;
    } else if (!q.bi && wmode == 1) {
      q.m0 = wt[q.l][r & 31][k];
      q.r = lw >= 1 ? 1 << (lw - 1) : 0, q.sh = lw, q.o = wt[q.l][r & 31][k + 1];
    } else {
      q.plain = true;
    }
    return q;
  }
#ifdef H264P_AVX2
  // a vector of the samples weighted as q says, of the predictions ld(l)
  // loads (l the list)
  template <class V, class L>
  static INLINE typename V::T wp_vec(const Wp& q, const L& ld) {
    if (q.bi) return q.plain ? V::avg8(ld(0), ld(1)) : wsum<V>(ld(0), ld(1), q.m0, q.m1, q.r, q.sh, q.o);
    return q.plain ? ld(q.l) : wsum<V>(ld(q.l), V::zero(), q.m0, 0, q.r, q.sh, q.o);
  }
  // inter()'s weighting of a macroblock of one pair of references (r0, r1)
  // throughout, as a skipped macroblock has: 32 samples at a time (two rows
  // of luma, four of chroma) in 32-bit lanes through pmaddwd
  void inter_mb(int r0, int r1, uint8_t* const* dst) const {
    for (int comp = 0; comp < 3; comp++) {
      int n = comp ? 8 : 16, ds = comp ? cur->w / 2 : cur->w;
      const uint8_t* p[2] = {comp ? pq_c[0][comp - 1] : pq_l[0], comp ? pq_c[1][comp - 1] : pq_l[1]};
      const Wp q = wp_of(r0, r1, comp ? 2 * comp : 0, comp ? cwd : lwd);
      uint8_t* d = dst[comp];
      for (int y = 0; y < n; y += n == 16 ? 2 : 4) {
        // (rows y and y + 1 of luma, y to y + 3 of chroma)
        __m256i v = wp_vec<V256>(q, [&](int l) {
          return n == 16 ? ld16x2(p[l] + y * 16, p[l] + y * 16 + 16) : _mm256_loadu_si256((const __m256i*)(p[l] + y * 8));
        });
        if (n == 16) {
          _mm_storeu_si128((__m128i*)(d + y * ds), _mm256_castsi256_si128(v));
          _mm_storeu_si128((__m128i*)(d + (y + 1) * ds), _mm256_extracti128_si256(v, 1));
          continue;
        }
        __m128i lo = _mm256_castsi256_si128(v), hi = _mm256_extracti128_si256(v, 1);
        _mm_storel_epi64((__m128i*)(d + y * ds), lo);
        _mm_storel_epi64((__m128i*)(d + (y + 1) * ds), _mm_unpackhi_epi64(lo, lo));
        _mm_storel_epi64((__m128i*)(d + (y + 2) * ds), hi);
        _mm_storel_epi64((__m128i*)(d + (y + 3) * ds), _mm_unpackhi_epi64(hi, hi));
      }
    }
  }
#endif
  // inter()'s weighting of component comp (0 Y, 1 Cb, 2 Cr) of 8x8 block
  // b8 (references r0, r1) into dst
  void inter_b8(int b8, int comp, int r0, int r1, uint8_t* dst) const {
    if (r0 < 0 && r1 < 0) return;
    int n = comp ? 4 : 8, x0 = (b8 & 1) * n, y0 = (b8 >> 1) * n, ds = comp ? cur->w / 2 : cur->w, ss = comp ? 8 : 16;
    dst += y0 * ds + x0;
    const uint8_t* p[2] = {(comp ? pq_c[0][comp - 1] : pq_l[0]) + y0 * ss + x0, (comp ? pq_c[1][comp - 1] : pq_l[1]) + y0 * ss + x0};
    const Wp q = wp_of(r0, r1, comp ? 2 * comp : 0, comp ? cwd : lwd);
#ifdef H264P_AVX2
    // a block in one vector (8 x 8 luma in 16-bit lanes of two, 4 x 4
    // chroma in one), weighted in 32-bit lanes through pmaddwd
    for (int y = 0; y < n; y += n == 8 ? 2 : 4) {
      __m128i v = wp_vec<V128>(q, [&](int l) {
        const uint8_t* s = p[l];
        if (n == 8) return _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*)(s + y * ss)), _mm_loadl_epi64((const __m128i*)(s + (y + 1) * ss)));
        int32_t u[4];
        for (int j = 0; j < 4; j++) memcpy(&u[j], s + j * ss, 4);
        return _mm_setr_epi32(u[0], u[1], u[2], u[3]);
      });
      if (n == 8) {
        _mm_storel_epi64((__m128i*)(dst + y * ds), v);
        _mm_storel_epi64((__m128i*)(dst + (y + 1) * ds), _mm_unpackhi_epi64(v, v));
        continue;
      }
      uint8_t t[16];
      _mm_storeu_si128((__m128i*)t, v);
      for (int j = 0; j < 4; j++) memcpy(dst + j * ds, t + 4 * j, 4);
    }
#else
    const uint8_t *a = p[0], *b = p[1], *s = p[q.l];
    if (q.plain && !q.bi) {
      copy_rows(dst, ds, s, ss, n, n);
      return;
    }
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) {
        int u = q.bi ? a[y * ss + x] : s[y * ss + x], v = q.bi ? b[y * ss + x] : 0;
        dst[y * ds + x] = (uint8_t)(q.plain ? (u + v + 1) >> 1 : clip1(((u * q.m0 + v * q.m1 + q.r) >> q.sh) + q.o));
      }
#endif
  }
  // inter prediction of a macroblock (8.4.2), with the weights of 8.4.2.3
  void inter(int addr, const int8_t rf[2][4], const int16_t mv[2][16][2], uint8_t* Y, uint8_t* U, uint8_t* V) {
    if (!predicted(addr, rf, mv)) predict(addr, rf, mv);
    uint8_t* const dst[3] = {Y, U, V};
#ifdef H264P_AVX2
    if (rf[0][1] == rf[0][0] && rf[0][2] == rf[0][0] && rf[0][3] == rf[0][0] && rf[1][1] == rf[1][0] &&
        rf[1][2] == rf[1][0] && rf[1][3] == rf[1][0] && (rf[0][0] >= 0 || rf[1][0] >= 0)) {
      inter_mb(rf[0][0], rf[1][0], dst);
      return;
    }
#endif
    for (int b8 = 0; b8 < 4; b8++)
      for (int comp = 0; comp < 3; comp++) inter_b8(b8, comp, rf[0][b8], rf[1][b8], dst[comp]);
  }

#ifdef H264P_AVX2
  // filter() of the 16 lines of a luma edge at once: x[0-7] their samples
  // p3, p2, p1, p0, q0, q1, q2, q3 (line i in byte i), b[k] the bS of the
  // lines 4k-4k+3 (0 none), ia and ib the indexes of alpha and beta
  static void filter_lines16(__m128i* x, const int* b, int ia, int ib) {
    const int alpha = kAlpha[ia], beta = kBeta[ib];
    auto w = [](__m128i v) { return _mm256_cvtepu8_epi16(v); };
    auto n = [](__m256i v) { return _mm_packus_epi16(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1)); };
    auto tc0 = [&](int k) { return b[k] >= 1 && b[k] <= 3 ? kTc0[ia][b[k] - 1] : 0; };
    const __m256i P3 = w(x[0]), P2 = w(x[1]), P1 = w(x[2]), P0 = w(x[3]), Q0 = w(x[4]), Q1 = w(x[5]), Q2 = w(x[6]), Q3 = w(x[7]);
    const __m256i bS = _mm256_setr_epi16(b[0], b[0], b[0], b[0], b[1], b[1], b[1], b[1], b[2], b[2], b[2], b[2], b[3], b[3], b[3], b[3]);
    const int t0 = tc0(0), t1 = tc0(1), t2 = tc0(2), t3 = tc0(3);
    const __m256i tc0v = _mm256_setr_epi16(t0, t0, t0, t0, t1, t1, t1, t1, t2, t2, t2, t2, t3, t3, t3, t3);
    const __m256i av = _mm256_set1_epi16((short)alpha), bv = _mm256_set1_epi16((short)beta), z = _mm256_setzero_si256();
    auto lt = [](__m256i a, __m256i c) { return _mm256_cmpgt_epi16(c, a); };  // a < c
    auto clamp = [](__m256i v, __m256i t) { return _mm256_max_epi16(_mm256_min_epi16(v, t), _mm256_sub_epi16(_mm256_setzero_si256(), t)); };
    const __m256i d0 = _mm256_abs_epi16(_mm256_sub_epi16(P0, Q0));
    const __m256i fl = _mm256_and_si256(_mm256_and_si256(lt(d0, av), lt(_mm256_abs_epi16(_mm256_sub_epi16(P1, P0)), bv)),
                                        _mm256_and_si256(lt(_mm256_abs_epi16(_mm256_sub_epi16(Q1, Q0)), bv), _mm256_cmpgt_epi16(bS, z)));
    if (_mm256_testz_si256(fl, fl)) return;
    const __m256i apl = lt(_mm256_abs_epi16(_mm256_sub_epi16(P2, P0)), bv), aql = lt(_mm256_abs_epi16(_mm256_sub_epi16(Q2, Q0)), bv);
    const __m256i strong = _mm256_and_si256(fl, _mm256_cmpeq_epi16(bS, _mm256_set1_epi16(4))), normal = _mm256_andnot_si256(strong, fl);
    // bS < 4
    const __m256i tc = _mm256_sub_epi16(_mm256_sub_epi16(tc0v, apl), aql);
    const __m256i delta = clamp(_mm256_srai_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(_mm256_sub_epi16(Q0, P0), 2),
                                                                                    _mm256_sub_epi16(P1, Q1)),
                                                                   _mm256_set1_epi16(4)),
                                                  3),
                                tc);
    const __m256i avg = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(P0, Q0), _mm256_set1_epi16(1)), 1);
    const __m256i np1 = _mm256_add_epi16(P1, clamp(_mm256_srai_epi16(_mm256_sub_epi16(_mm256_add_epi16(P2, avg), _mm256_slli_epi16(P1, 1)), 1), tc0v));
    const __m256i nq1 = _mm256_add_epi16(Q1, clamp(_mm256_srai_epi16(_mm256_sub_epi16(_mm256_add_epi16(Q2, avg), _mm256_slli_epi16(Q1, 1)), 1), tc0v));
    // bS 4
    const __m256i sm = lt(d0, _mm256_set1_epi16((short)((alpha >> 2) + 2)));
    const __m256i sp = _mm256_and_si256(apl, sm), sq = _mm256_and_si256(aql, sm);
    const __m256i c2 = _mm256_set1_epi16(2), c4 = _mm256_set1_epi16(4);
    const __m256i p0q0 = _mm256_add_epi16(P0, Q0);
    // (p2 + 2p1 + 2p0 + 2q0 + q1 + 4) >> 3, (p2 + p1 + p0 + q0 + 2) >> 2, (2p3 + 3p2 + p1 + p0 + q0 + 4) >> 3, else (2p1 + p0 + q1 + 2) >> 2
    const __m256i sp0 = _mm256_blendv_epi8(
        _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(P1, 1), P0), _mm256_add_epi16(Q1, c2)), 2),
        _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(P2, _mm256_slli_epi16(_mm256_add_epi16(P1, p0q0), 1)), _mm256_add_epi16(Q1, c4)), 3), sp);
    const __m256i sp1 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(P2, P1), _mm256_add_epi16(p0q0, c2)), 2);
    const __m256i sp2 = _mm256_srli_epi16(
        _mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(_mm256_add_epi16(P3, P2), 1), _mm256_add_epi16(P2, P1)), _mm256_add_epi16(p0q0, c4)), 3);
    const __m256i sq0 = _mm256_blendv_epi8(
        _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(Q1, 1), Q0), _mm256_add_epi16(P1, c2)), 2),
        _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(Q2, _mm256_slli_epi16(_mm256_add_epi16(Q1, p0q0), 1)), _mm256_add_epi16(P1, c4)), 3), sq);
    const __m256i sq1 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(Q2, Q1), _mm256_add_epi16(p0q0, c2)), 2);
    const __m256i sq2 = _mm256_srli_epi16(
        _mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(_mm256_add_epi16(Q3, Q2), 1), _mm256_add_epi16(Q2, Q1)), _mm256_add_epi16(p0q0, c4)), 3);
    // each line its filter's samples
    auto pick = [](__m256i v, __m256i m, __m256i f) { return _mm256_blendv_epi8(v, f, m); };
    __m256i rp0 = pick(pick(P0, normal, _mm256_add_epi16(P0, delta)), strong, sp0);
    __m256i rq0 = pick(pick(Q0, normal, _mm256_sub_epi16(Q0, delta)), strong, sq0);
    __m256i rp1 = pick(pick(P1, _mm256_and_si256(normal, apl), np1), _mm256_and_si256(strong, sp), sp1);
    __m256i rq1 = pick(pick(Q1, _mm256_and_si256(normal, aql), nq1), _mm256_and_si256(strong, sq), sq1);
    __m256i rp2 = pick(P2, _mm256_and_si256(strong, sp), sp2), rq2 = pick(Q2, _mm256_and_si256(strong, sq), sq2);
    x[1] = n(rp2), x[2] = n(rp1), x[3] = n(rp0), x[4] = n(rq0), x[5] = n(rq1), x[6] = n(rq2);
  }
  // the luma edge at p (as filter(): s across, t along), its 16 lines in
  // segments of bS b[k]
  static void filter16(uint8_t* p, int s, int t, const int* b, int qpa, int aoff, int boff) {
    int ia = clip3(0, 51, qpa + aoff), ib = clip3(0, 51, qpa + boff);
    if (!kAlpha[ia] || !kBeta[ib]) return;
    __m128i x[8];
    if (t == 1) {
      // across rows: a row of the picture is a sample of each line
      for (int i = 0; i < 8; i++) x[i] = _mm_loadu_si128((const __m128i*)(p + (i - 4) * s));
      filter_lines16(x, b, ia, ib);
      for (int i = 1; i < 7; i++) _mm_storeu_si128((__m128i*)(p + (i - 4) * s), x[i]);
      return;
    }
    // across columns: the 16 rows of 8 samples transposed, and back
    __m128i r[16], u[8], v[8];
    for (int j = 0; j < 16; j++) r[j] = _mm_loadl_epi64((const __m128i*)(p - 4 + j * t));
    for (int k = 0; k < 8; k++) u[k] = _mm_unpacklo_epi8(r[2 * k], r[2 * k + 1]);
    for (int k = 0; k < 4; k++) v[k] = _mm_unpacklo_epi16(u[2 * k], u[2 * k + 1]), v[4 + k] = _mm_unpackhi_epi16(u[2 * k], u[2 * k + 1]);
    // (v[h * 4 + g]: columns 4h-4h+3 of rows 4g-4g+3)
    for (int h = 0; h < 2; h++) {
      __m128i a = _mm_unpacklo_epi32(v[4 * h], v[4 * h + 1]), c = _mm_unpackhi_epi32(v[4 * h], v[4 * h + 1]);
      __m128i d = _mm_unpacklo_epi32(v[4 * h + 2], v[4 * h + 3]), e = _mm_unpackhi_epi32(v[4 * h + 2], v[4 * h + 3]);
      x[4 * h] = _mm_unpacklo_epi64(a, d), x[4 * h + 1] = _mm_unpackhi_epi64(a, d);
      x[4 * h + 2] = _mm_unpacklo_epi64(c, e), x[4 * h + 3] = _mm_unpackhi_epi64(c, e);
    }
    filter_lines16(x, b, ia, ib);
    for (int k = 0; k < 4; k++) u[k] = _mm_unpacklo_epi8(x[2 * k], x[2 * k + 1]), u[4 + k] = _mm_unpackhi_epi8(x[2 * k], x[2 * k + 1]);
    // (u[k]: rows 0-7 of columns 2k, 2k + 1; u[4 + k] rows 8-15)
    for (int h = 0; h < 2; h++) {
      __m128i a = _mm_unpacklo_epi16(u[4 * h], u[4 * h + 1]), c = _mm_unpackhi_epi16(u[4 * h], u[4 * h + 1]);
      __m128i d = _mm_unpacklo_epi16(u[4 * h + 2], u[4 * h + 3]), e = _mm_unpackhi_epi16(u[4 * h + 2], u[4 * h + 3]);
      // (a: rows 8h-8h+3 of columns 0-3, d: of 4-7; c, e: rows 8h+4-8h+7)
      __m128i r01 = _mm_unpacklo_epi32(a, d), r23 = _mm_unpackhi_epi32(a, d), r45 = _mm_unpacklo_epi32(c, e), r67 = _mm_unpackhi_epi32(c, e);
      uint8_t* q = p - 4 + 8 * h * t;
      _mm_storel_epi64((__m128i*)q, r01), _mm_storel_epi64((__m128i*)(q + t), _mm_unpackhi_epi64(r01, r01));
      _mm_storel_epi64((__m128i*)(q + 2 * t), r23), _mm_storel_epi64((__m128i*)(q + 3 * t), _mm_unpackhi_epi64(r23, r23));
      _mm_storel_epi64((__m128i*)(q + 4 * t), r45), _mm_storel_epi64((__m128i*)(q + 5 * t), _mm_unpackhi_epi64(r45, r45));
      _mm_storel_epi64((__m128i*)(q + 6 * t), r67), _mm_storel_epi64((__m128i*)(q + 7 * t), _mm_unpackhi_epi64(r67, r67));
    }
  }
  // filter() of a chroma edge of both planes at once (8 lines each): pu, pv
  // at q0 of their first line (s across, t along), b[k] the bS of lines
  // 2k, 2k + 1, qa[c] the plane's qPav
  static void filter_c16(uint8_t* pu, uint8_t* pv, int s, int t, const int* b, const int* qa, int aoff, int boff) {
    int ia[2], ib[2];
    for (int c = 0; c < 2; c++) ia[c] = clip3(0, 51, qa[c] + aoff), ib[c] = clip3(0, 51, qa[c] + boff);
    if ((!kAlpha[ia[0]] || !kBeta[ib[0]]) && (!kAlpha[ia[1]] || !kBeta[ib[1]])) return;
    // x[0-3]: p1, p0, q0, q1 of the lines, those of U in bytes 0-7, of V in 8-15
    __m128i x[4], r[16];
    if (t == 1) {
      for (int i = 0; i < 4; i++)
        x[i] = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*)(pu + (i - 2) * s)), _mm_loadl_epi64((const __m128i*)(pv + (i - 2) * s)));
    } else {
      for (int j = 0; j < 16; j++) {
        int32_t u;
        memcpy(&u, (j < 8 ? pu : pv) - 2 + (j & 7) * t, 4);
        r[j] = _mm_cvtsi32_si128(u);
      }
      __m128i a[8], g[4];
      for (int k = 0; k < 8; k++) a[k] = _mm_unpacklo_epi8(r[2 * k], r[2 * k + 1]);
      for (int k = 0; k < 4; k++) g[k] = _mm_unpacklo_epi16(a[2 * k], a[2 * k + 1]);
      __m128i lo01 = _mm_unpacklo_epi32(g[0], g[1]), hi01 = _mm_unpackhi_epi32(g[0], g[1]);
      __m128i lo23 = _mm_unpacklo_epi32(g[2], g[3]), hi23 = _mm_unpackhi_epi32(g[2], g[3]);
      x[0] = _mm_unpacklo_epi64(lo01, lo23), x[1] = _mm_unpackhi_epi64(lo01, lo23);
      x[2] = _mm_unpacklo_epi64(hi01, hi23), x[3] = _mm_unpackhi_epi64(hi01, hi23);
    }
    auto w = [](__m128i v) { return _mm256_cvtepu8_epi16(v); };
    auto n = [](__m256i v) { return _mm_packus_epi16(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1)); };
    const __m256i P1 = w(x[0]), P0 = w(x[1]), Q0 = w(x[2]), Q1 = w(x[3]);
    short bl[16], al[16], be[16], tl[16];
    for (int i = 0; i < 16; i++) {
      int c = i >> 3, k = (i & 7) >> 1;
      bl[i] = (short)b[k];
      al[i] = kAlpha[ia[c]], be[i] = kBeta[ib[c]];
      tl[i] = (short)(b[k] >= 1 && b[k] <= 3 ? kTc0[ia[c]][b[k] - 1] + 1 : 0);
    }
    const __m256i bS = _mm256_loadu_si256((const __m256i*)bl), av = _mm256_loadu_si256((const __m256i*)al);
    const __m256i bv = _mm256_loadu_si256((const __m256i*)be), tc = _mm256_loadu_si256((const __m256i*)tl);
    auto lt = [](__m256i a, __m256i c) { return _mm256_cmpgt_epi16(c, a); };
    const __m256i fl = _mm256_and_si256(
        _mm256_and_si256(lt(_mm256_abs_epi16(_mm256_sub_epi16(P0, Q0)), av), lt(_mm256_abs_epi16(_mm256_sub_epi16(P1, P0)), bv)),
        _mm256_and_si256(lt(_mm256_abs_epi16(_mm256_sub_epi16(Q1, Q0)), bv), _mm256_cmpgt_epi16(bS, _mm256_setzero_si256())));
    if (_mm256_testz_si256(fl, fl)) return;
    const __m256i strong = _mm256_and_si256(fl, _mm256_cmpeq_epi16(bS, _mm256_set1_epi16(4))), normal = _mm256_andnot_si256(strong, fl);
    __m256i delta = _mm256_srai_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(_mm256_sub_epi16(Q0, P0), 2), _mm256_sub_epi16(P1, Q1)),
                                                       _mm256_set1_epi16(4)),
                                      3);
    delta = _mm256_max_epi16(_mm256_min_epi16(delta, tc), _mm256_sub_epi16(_mm256_setzero_si256(), tc));
    const __m256i c2 = _mm256_set1_epi16(2);
    __m256i sp0 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(P1, 1), P0), _mm256_add_epi16(Q1, c2)), 2);
    __m256i sq0 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(Q1, 1), Q0), _mm256_add_epi16(P1, c2)), 2);
    __m256i rp0 = _mm256_blendv_epi8(_mm256_blendv_epi8(P0, _mm256_add_epi16(P0, delta), normal), sp0, strong);
    __m256i rq0 = _mm256_blendv_epi8(_mm256_blendv_epi8(Q0, _mm256_sub_epi16(Q0, delta), normal), sq0, strong);
    x[1] = n(rp0), x[2] = n(rq0);
    if (t == 1) {
      for (int i = 1; i < 3; i++) {
        _mm_storel_epi64((__m128i*)(pu + (i - 2) * s), x[i]);
        _mm_storel_epi64((__m128i*)(pv + (i - 2) * s), _mm_unpackhi_epi64(x[i], x[i]));
      }
      return;
    }
    __m128i a = _mm_unpacklo_epi8(x[0], x[1]), c = _mm_unpackhi_epi8(x[0], x[1]);
    __m128i d = _mm_unpacklo_epi8(x[2], x[3]), e = _mm_unpackhi_epi8(x[2], x[3]);
    // (rows 0-3, 4-7 of U and of V, 4 samples each)
    uint8_t rows[64];
    _mm_storeu_si128((__m128i*)rows, _mm_unpacklo_epi16(a, d));
    _mm_storeu_si128((__m128i*)(rows + 16), _mm_unpackhi_epi16(a, d));
    _mm_storeu_si128((__m128i*)(rows + 32), _mm_unpacklo_epi16(c, e));
    _mm_storeu_si128((__m128i*)(rows + 48), _mm_unpackhi_epi16(c, e));
    for (int j = 0; j < 16; j++) memcpy((j < 8 ? pu : pv) - 2 + (j & 7) * t, rows + 4 * j, 4);
  }
#endif
  // --- deblocking (8.7) of the current picture
  int bs(int pa, int pb, int bp, int bq, bool mb_edge) const {
    const DbMb& P = db[pa];
    const DbMb& Q = db[pb];
    if (P.intra || Q.intra) return mb_edge ? 4 : 3;
    if (P.nz[bp] || Q.nz[bq]) return 2;
    int b8p = (bp >> 3) * 2 + ((bp & 3) >> 1), b8q = (bq >> 3) * 2 + ((bq & 3) >> 1);
    int64_t p0 = P.rp[0][b8p], p1 = P.rp[1][b8p], q0 = Q.rp[0][b8q], q1 = Q.rp[1][b8q];
    int np = (p0 >= 0) + (p1 >= 0), nq = (q0 >= 0) + (q1 >= 0);
    if (np != nq) return 1;
    auto apart = [](const int16_t* a, const int16_t* b) { return abs(a[0] - b[0]) >= 4 || abs(a[1] - b[1]) >= 4; };
    if (np == 1) {
      int lp = p0 >= 0 ? 0 : 1, lq = q0 >= 0 ? 0 : 1;
      if ((lp ? p1 : p0) != (lq ? q1 : q0)) return 1;
      return apart(P.mv[lp][bp], Q.mv[lq][bq]);
    }
    // two vectors each: the same two pictures?
    if (!((p0 == q0 && p1 == q1) || (p0 == q1 && p1 == q0))) return 1;
    if (p0 != p1) {
      if (p0 == q0) return apart(P.mv[0][bp], Q.mv[0][bq]) || apart(P.mv[1][bp], Q.mv[1][bq]);
      return apart(P.mv[0][bp], Q.mv[1][bq]) || apart(P.mv[1][bp], Q.mv[0][bq]);
    }
    // both vectors of each refer to the same picture
    return (apart(P.mv[0][bp], Q.mv[0][bq]) || apart(P.mv[1][bp], Q.mv[1][bq])) &&
           (apart(P.mv[0][bp], Q.mv[1][bq]) || apart(P.mv[1][bp], Q.mv[0][bq]));
  }
  // filters the samples across one edge of 4 (luma) or 2 (chroma) lines:
  // p points at q0 of the first line, s is the step across the edge, t the
  // step along it
  static void filter(uint8_t* p, int s, int t, int n, int b, int qpa, int aoff, int boff, bool chroma) {
    int ia = clip3(0, 51, qpa + aoff), ib = clip3(0, 51, qpa + boff);
    int alpha = kAlpha[ia], beta = kBeta[ib];
    for (int k = 0; k < n; k++, p += t) {
      int p0 = p[-s], p1 = p[-2 * s], q0 = p[0], q1 = p[s];
      if (!(abs(p0 - q0) < alpha && abs(p1 - p0) < beta && abs(q1 - q0) < beta)) continue;
      if (b < 4) {
        int tc0 = kTc0[ia][b - 1];
        if (chroma) {
          int tc = tc0 + 1;
          int d = clip3(-tc, tc, ((q0 - p0) * 4 + (p1 - q1) + 4) >> 3);
          p[-s] = (uint8_t)clip1(p0 + d);
          p[0] = (uint8_t)clip1(q0 - d);
        } else {
          int p2 = p[-3 * s], q2 = p[2 * s];
          int ap = abs(p2 - p0), aq = abs(q2 - q0);
          int tc = tc0 + (ap < beta) + (aq < beta);
          int d = clip3(-tc, tc, ((q0 - p0) * 4 + (p1 - q1) + 4) >> 3);
          p[-s] = (uint8_t)clip1(p0 + d);
          p[0] = (uint8_t)clip1(q0 - d);
          if (ap < beta) p[-2 * s] = (uint8_t)(p1 + clip3(-tc0, tc0, (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1));
          if (aq < beta) p[s] = (uint8_t)(q1 + clip3(-tc0, tc0, (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1));
        }
      } else if (chroma) {
        p[-s] = (uint8_t)((2 * p1 + p0 + q1 + 2) >> 2);
        p[0] = (uint8_t)((2 * q1 + q0 + p1 + 2) >> 2);
      } else {
        int p2 = p[-3 * s], q2 = p[2 * s], p3 = p[-4 * s], q3 = p[3 * s];
        int ap = abs(p2 - p0), aq = abs(q2 - q0);
        bool small = abs(p0 - q0) < ((alpha >> 2) + 2);
        if (ap < beta && small) {
          p[-s] = (uint8_t)((p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3);
          p[-2 * s] = (uint8_t)((p2 + p1 + p0 + q0 + 2) >> 2);
          p[-3 * s] = (uint8_t)((2 * p3 + 3 * p2 + p1 + p0 + q0 + 4) >> 3);
        } else {
          p[-s] = (uint8_t)((2 * p1 + p0 + q1 + 2) >> 2);
        }
        if (aq < beta && small) {
          p[0] = (uint8_t)((p1 + 2 * p0 + 2 * q0 + 2 * q1 + q2 + 4) >> 3);
          p[s] = (uint8_t)((p0 + q0 + q1 + q2 + 2) >> 2);
          p[2 * s] = (uint8_t)((2 * q3 + 3 * q2 + q1 + q0 + p0 + 4) >> 3);
        } else {
          p[0] = (uint8_t)((2 * q1 + q0 + p1 + 2) >> 2);
        }
      }
    }
  }
  // macroblock row row of the current picture (the rows above it done);
  // not where the level has the contexts see pictures unfiltered
  bool deblock;  // (Init(): true)
  void deblock_row(int row) {
    if (!deblock) return;
    int mbw = cur->mbw, w = cur->w, cw = w / 2;
    for (int addr = row * mbw; addr < (row + 1) * mbw; addr++) {
      const DbMb& Q = db[addr];
      if (Q.slice < 0 || Q.disable == 1) continue;
      int mx = mb_x(addr, 1), my = mb_y(addr, 1);
      // an inter macroblock of one motion, without coefficients, has no
      // edge inside it to filter (bS 0 everywhere)
      bool flat = !Q.intra;
      for (int k = 0; k < 16 && flat; k++) flat = !Q.nz[k];
      for (int l = 0; l < 2 && flat; l++)
        flat = Q.rp[l][1] == Q.rp[l][0] && Q.rp[l][2] == Q.rp[l][0] && Q.rp[l][3] == Q.rp[l][0] && one_mv(Q.mv[l]);
      for (int dir = 0; dir < 2; dir++) {  // vertical edges, then horizontal ones
        int na = dir ? (my ? addr - mbw : -1) : (mx ? addr - 1 : -1);
        bool mb_edge_ok = na >= 0 && db[na].slice >= 0 && (Q.disable == 0 || db[na].slice == Q.slice);
        for (int e = 0; e < 4; e++) {
          if (e == 0 && !mb_edge_ok) continue;
          if (e && flat) break;
          if (e & 1 && Q.t8) continue;
          int pa = e ? addr : na;
          int b[4];
          bool any = db[pa].intra || Q.intra;
          if (any)
            b[0] = b[1] = b[2] = b[3] = e ? 3 : 4;  // (as bs, whatever the blocks)
          else
            for (int k = 0; k < 4; k++) {
              int bq = dir ? e * 4 + k : k * 4 + e;
              int bp = e ? (dir ? bq - 4 : bq - 1) : (dir ? 12 + k : k * 4 + 3);
              b[k] = bs(pa, addr, bp, bq, e == 0);
              any |= b[k] != 0;
            }
          if (!any) continue;
          const DbMb& P = db[pa];
          int qpp = P.pcm ? 0 : P.qp, qpq = Q.pcm ? 0 : Q.qp;
          uint8_t* Yq = mb_px(addr, 0) + (dir ? e * 4 * w : e * 4);
#ifdef H264P_AVX2
          filter16(Yq, dir ? w : 1, dir ? 1 : w, b, (qpp + qpq + 1) >> 1, Q.alpha_off, Q.beta_off);
#else
          for (int k = 0; k < 4; k++)
            if (b[k])
              filter(Yq + (dir ? k * 4 : k * 4 * w), dir ? w : 1, dir ? 1 : w, 4, b[k], (qpp + qpq + 1) >> 1,
                     Q.alpha_off, Q.beta_off, false);
#endif
          if (e & 1) continue;  // chroma edges at luma edges 0 and 8
#ifdef H264P_AVX2
          {
            int qa[2];
            uint8_t* Cq[2];
            for (int c = 0; c < 2; c++) {
              qa[c] = (chroma_qp(qpp, Q.cqp[c]) + chroma_qp(qpq, Q.cqp[c]) + 1) >> 1;
              Cq[c] = mb_px(addr, c + 1) + (dir ? e * 2 * cw : e * 2);
            }
            filter_c16(Cq[0], Cq[1], dir ? cw : 1, dir ? 1 : cw, b, qa, Q.alpha_off, Q.beta_off);
          }
#else
          for (int c = 0; c < 2; c++) {
            int cp = chroma_qp(qpp, Q.cqp[c]), cq = chroma_qp(qpq, Q.cqp[c]);
            uint8_t* Cq = mb_px(addr, c + 1) + (dir ? e * 2 * cw : e * 2);
            for (int k = 0; k < 4; k++)
              if (b[k])
                filter(Cq + (dir ? k * 2 : k * 2 * cw), dir ? cw : 1, dir ? 1 : cw, 2, b[k], (cp + cq + 1) >> 1,
                       Q.alpha_off, Q.beta_off, true);
          }
#endif
        }
      }
    }
  }

  // --- a slice begins
  static bool supported(const Sps& sps, const Pps& pps) {
    return (!PX || (sps.chroma_array_type() == 1 && sps.bit_depth_luma == 8 && sps.bit_depth_chroma == 8 &&
                    !sps.scaling_matrix && !pps.scaling_matrix && !sps.transform_bypass)) &&
           sps.frame_mbs_only_flag && (int64_t)sps.width_mbs * sps.frame_height_mbs() <= 36864;  // 4096 x 2304
  }
  void slice_begin(const SliceHeader& h, bool newpic) {
    if (!REFS) return;
    const Sps& sps = *h.sps;
    const Pps& pps = *h.pps;
    // (a picture whose SPS or PPS is redefined between its slices, as one it
    // does not support, ends there: its reconstruction, as the samples' QP,
    // would be the wrong one's)
    if (newpic || !cur || cur->mbw != sps.width_mbs || cur->mbh != sps.frame_height_mbs() || !supported(sps, pps)) {
      finish_picture();
      if (!supported(sps, pps)) {
        drop_refs();
        return;
      }
      max_frame_num = 1 << sps.log2_max_frame_num;
      max_num_ref = sps.max_num_ref_frames;
      if (!spare.empty()) {
        cur = spare.back();
        spare.pop_back();
      } else {
        cur = RPic::make();
      }
      if (cur->mbw != sps.width_mbs || cur->mbh != sps.frame_height_mbs()) {
        cur->alloc(sps.width_mbs, sps.frame_height_mbs(), PX);
      } else {
        // (a picture of before: what no slice decodes has no motion, as in
        // a new one)
        cur->intra.fill(cur->intra.size(), 1);
        cur->rf.fill(cur->rf.size(), -1);
      }
      cur->ref = cur->lt = cur->padded = false;
      cur->lt_idx = -1;
      cur->id = next_id++;
      cur->frame_num = h.frame_num;
      cur->poc = poc(h);
      cur_idr = h.nal_unit_type == 5;
      cur_ref_idc = h.nal_ref_idc;
      cur_lt_flag = h.long_term_reference_flag;
      cur_mmco5 = h.mmco5;
      cur_nmmco = h.nmmco;
      memcpy(cur_mmco, h.mmco, h.nmmco * sizeof(Mmco));
      // (finishing the last picture left db as none)
      if (PX && db.size() != (size_t)cur->mbw * cur->mbh) db.fill((size_t)cur->mbw * cur->mbh, db_none());
      slice_no = 0;
      mact = true;
      // references of another size are no use
      size_t keep = 0;
      for (size_t i = 0; i < dpb.size(); i++)
        if (dpb[i]->mbw == cur->mbw && dpb[i]->mbh == cur->mbh)
          dpb[keep++] = dpb[i];
        else
          RPic::drop(dpb[i]);
      dpb.n = keep;
      if (PX) fin_begin();
    } else {
      slice_no++;
    }
    if (!mact) return;
    curr_pic_num = h.frame_num;
    disable = h.disable_deblocking;
    alpha_off = h.alpha_offset;
    beta_off = h.beta_offset;
    cqp_off[0] = pps.chroma_qp_index_offset;
    cqp_off[1] = pps.second_chroma_qp_index_offset;
    constrained_intra = pps.constrained_intra_pred_flag;
    direct8x8 = sps.direct_8x8_inference_flag;
    build_lists(h);
    weights(h, pps);
  }

  // the colocated block of the 4x4 block blk (raster) of macroblock addr,
  // for direct prediction (8.4.1.2.1): false when the first picture of list
  // 1 is not there or the block is intra; else its refIdx and vector, and
  // the picture it refers to
  bool colocated(int addr, int blk, int& ref, int* mv, int64_t& pid) const {
    const RPic* col = motion() && nlist[1] ? list[1][0] : nullptr;
    if (!col || addr >= col->mbw * col->mbh || col->intra[addr]) return false;
    if (direct8x8) {
      int x = blk & 3, y = blk >> 2;
      blk = (y < 2 ? 0 : 12) + (x < 2 ? 0 : 3);
    }
    int b8 = (blk >> 3) * 2 + ((blk & 3) >> 1);
    int l = col->rf[(addr * 4 + b8) * 2] >= 0 ? 0 : 1;
    ref = col->rf[(addr * 4 + b8) * 2 + l];
    pid = col->rpid[(addr * 4 + b8) * 2 + l];
    mv[0] = col->mv[((addr * 16 + blk) * 2 + l) * 2];
    mv[1] = col->mv[((addr * 16 + blk) * 2 + l) * 2 + 1];
    return ref >= 0;
  }
  bool col_long_term() const { return motion() && nlist[1] && list[1][0] && list[1][0]->lt; }
  // temporal direct (8.4.1.2.3): the list 0 reference and the scale factor
  int map_col(int64_t pid) const {
    for (int i = 0; i < nlist[0]; i++)
      if (list[0][i] && list[0][i]->id == pid) return i;
    return 0;
  }
  int dist_scale(int r0) const {
    const RPic* p0 = r0 < nlist[0] ? list[0][r0] : nullptr;
    const RPic* p1 = nlist[1] ? list[1][0] : nullptr;
    if (!p0 || !p1 || p0->lt) return 256;
    int tb = clip3(-128, 127, cur->poc - p0->poc), td = clip3(-128, 127, p1->poc - p0->poc);
    if (!td) return 256;
    int tx = (16384 + abs(td / 2)) / td;
    return clip3(-1024, 1023, (tb * tx + 32) >> 6);
  }
  int dc16_at(int qp, int blk) {
    if (!mb_dc) dc16(qp);
    return mb_dcr[blk_y(blk) * 4 + blk_x(blk)];
  }
  // a coefficient, dequantized: c at scan position k of a 4x4 or 8x8 block
  static int64_t dequant1(int c, int k, int qp, bool b8) {
    int q6 = qp / 6, m = qp % 6;
    if (!b8) {
      int r = kZz4[k], y = r >> 2, x = r & 3;
      int ls = 16 * kNorm4[m][norm4_class(y, x)];
      return qp >= 24 ? (int64_t)c * ls * ((int64_t)1 << (q6 - 4)) : ((int64_t)c * ls + (1 << (3 - q6))) >> (4 - q6);
    }
    int r = kZz8[k], ls = 16 * kNorm8[m][norm8_class(r >> 3, r & 7)];
    return qp >= 36 ? (int64_t)c * ls * ((int64_t)1 << (q6 - 6)) : ((int64_t)c * ls + (1 << (5 - q6))) >> (6 - q6);
  }
};
template <int K>
constexpr bool MbDecoder<K>::REFS;
template <int K>
constexpr bool MbDecoder<K>::PX;

// --- The model's queries of the samples decoded (h264p_cm.md): the
// left strip and the template searches with their memo, the skip
// prediction's miss, the intra proxy and its modes, levels and energy,
// textures, gradients and levels of the prediction, sign prediction's
// edges, DIMD and x264's AQ energy. Each answers "not known" where the
// picture has no samples (act()).
template <int K>
struct ReconT : MbDecoder<K> {
  typedef MbDecoder<K> D;
  // What the queries read of the decoder:
  // the current picture, its reference lists and the macroblock
  using D::act; using D::cur; using D::dpb; using D::list; using D::nlist; using D::mb_addr; using D::mb_x;
  using D::mb_y; using D::plane; using D::stride; using D::mb_px; using D::mb_at;
  // how far its decoding got
  using D::db; using D::above_done; using D::left_done; using D::mb_prepared; using D::mb_cls; using D::mb_done4;
  using D::mb_done8;
  // the inter prediction: the skip prediction skip_miss puts in the picture
  // (prepare() keeps it), the predictions' cache bi_diff shares with inter()
  using D::skip_in; using D::skip_rf; using D::skip_mv; using D::inter; using D::predict; using D::predicted;
  using D::pq_l; using D::pq_ok;
  // the decoder's kernels
  using D::mc_luma; using D::mc_luma_t; using D::mc_chroma; using D::pred_nxn; using D::pred_l16; using D::pred_c8; using D::around;
  using D::edge_fill; using D::norm4_class; using D::norm8_class;
  void Init() {
    D::Init();
    lst_ok = false;
    memset((void*)tmm, 0, sizeof tmm);
    tmm_gen = 1;
    prx_ok = false;
  }
  void Quit() { D::Quit(); }
  // (a macroblock begins: what the queries knew of the last is gone)
  void mb_start(int addr) {
    D::mb_start(addr);
    if (!D::PX) return;
    lst_ok = prx_ok = false;
    if (!++tmm_gen) {
      memset((void*)tmm, 0, sizeof tmm);
      tmm_gen = 1;
    }
  }

  // the two columns of samples left of the current macroblock, decoded,
  // as two rows of 16 (made at its first use in the macroblock)
  mutable uint8_t lst[32];
  mutable bool lst_ok;
  const uint8_t* left_strip() const {
    if (!lst_ok) {
      int w = cur->w;
      const uint8_t* q = mb_at(0, 0, 0);
      for (int j = 0; j < 16; j++) lst[j] = q[j * w - 2], lst[16 + j] = q[j * w - 1];
      lst_ok = true;
    }
    return lst;
  }
  // The Intra16x16 mode (0 vertical, 1 horizontal, 2 DC, 3 plane) that
  // predicts the macroblock best in the proxy picture displaced by (dx, dy)
  // whole samples, from that picture's samples around it, by SATD; costs
  // gets each mode's (INT_MAX not possible); -1 no picture
  int proxy_i16mode(int dx, int dy, int* costs) const {
    if (!act()) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int w = P->w;
    int x0 = clip3(0, w - 16, mb_x(mb_addr) + dx), y0 = clip3(0, P->h - 16, mb_y(mb_addr) + dy);
    bool aA = x0 > 0, aB = y0 > 0;
    const uint8_t* s = P->y.data() + y0 * w + x0;
    int T[16], L[16], Q;
    around(s, w, 16, aB, aA, aA && aB, T, L, Q);
    int best = -1;
    for (int m = 0; m < 4; m++) {
      costs[m] = INT_MAX;
      if ((m == 0 && !aB) || (m == 1 && !aA) || (m == 3 && !(aA && aB))) continue;
      uint8_t d[256];
      pred_l16(d, 16, m, T, L, Q, aA, aB);
      costs[m] = satd_nxn(s, w, d, 16, 16);
      if (best < 0 || costs[m] < costs[best]) best = m;
    }
    return best;
  }
  // The chroma mode (0 DC, 1 horizontal, 2 vertical, 3 plane) that predicts
  // the chroma of the macroblock best in the proxy picture displaced by
  // (dx, dy) whole luma samples, from that picture's samples around it,
  // by SATD over both components; costs gets each mode's; -1 no picture
  int proxy_cmode(int dx, int dy, int* costs) const {
    if (!act()) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int cw = P->w / 2, ch = P->h / 2;
    int x0 = clip3(0, cw - 8, mb_x(mb_addr, 8) + div2(dx)), y0 = clip3(0, ch - 8, mb_y(mb_addr, 8) + div2(dy));
    bool aA = x0 > 0, aB = y0 > 0;
    for (int m = 0; m < 4; m++) costs[m] = 0;
    for (int c = 0; c < 2; c++) {
      const uint8_t* s = (c ? P->v.data() : P->u.data()) + y0 * cw + x0;
      int T[8], L[8], Q;
      around(s, cw, 8, aB, aA, aA && aB, T, L, Q);
      for (int m = 0; m < 4; m++) {
        if ((m == 1 && !aA) || (m == 2 && !aB) || (m == 3 && !(aA && aB))) {
          costs[m] = INT_MAX;
          continue;
        }
        if (costs[m] == INT_MAX) continue;
        uint8_t d[64];
        pred_c8(d, 8, m, T, L, Q, aA, aB);
        costs[m] += satd_nxn(s, cw, d, 8, 8);
      }
    }
    int best = -1;
    for (int m = 0; m < 4; m++)
      if (costs[m] < INT_MAX && (best < 0 || costs[m] < costs[best])) best = m;
    return best;
  }
  static int div2(int v) { return v >= 0 ? v >> 1 : -((-v) >> 1); }

  // The edge directions next to a block, as decoder-side intra mode
  // derivation (DIMD) finds them: Sobel gradients along the second row above
  // the macroblock (at the block's columns) and the second column to its
  // left (at the block's rows), where those macroblocks are decoded, each
  // voting with |Gx| + |Gy| for the Intra4x4 mode whose direction is closest
  // to the edge. Returns the strongest direction (0-8, not 2), 9 without
  // any; strength gets the sum of the votes.
  int dimd(int addr, int x0, int y0, int n, int& strength) const {
    strength = 0;
    if (!act()) return 9;
    int mbw = cur->mbw, w = cur->w, mx = mb_x(addr, 1), my = mb_y(addr, 1);
    const uint8_t* Y = cur->y.data();
    int hist[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto vote = [&](int x, int y) {
      const uint8_t* p = Y + y * w + x;
      int gx = (p[-w + 1] + 2 * p[1] + p[w + 1]) - (p[-w - 1] + 2 * p[-1] + p[w - 1]);
      int gy = (p[w - 1] + 2 * p[w] + p[w + 1]) - (p[-w - 1] + 2 * p[-w] + p[-w + 1]);
      int m = abs(gx) + abs(gy);
      if (!m) return;
      // the edge runs along (ex, ey) = (-gy, gx); its angle from the x axis,
      // in [0, 180), to the nearest of the directions of the modes: 0
      // (horizontal), 26.6, 45, 63.4, 90 (vertical) degrees, and mirrored,
      // by the tangents of the angles halfway between
      int ex = -gy, ey = gx;
      if (ey < 0 || (ey == 0 && ex < 0)) ex = -ex, ey = -ey;
      int ax = abs(ex);
      int k = ey * 1000 < ax * 236 ? 0 : ey * 1000 < ax * 720 ? 1 : ey * 1000 < ax * 1388 ? 2 : ey * 1000 < ax * 4236 ? 3 : 4;
      static const int kRight[5] = {1, 6, 4, 5, 0}, kLeft[5] = {1, 8, 3, 7, 0};
      hist[ex < 0 ? kLeft[k] : kRight[k]] += m;
      strength += m;
    };
    int up = my ? addr - mbw : -1, left = mx ? addr - 1 : -1;
    if (up >= 0 && db[up].slice >= 0)
      for (int x = Max(x0, 1); x < Min(x0 + n, 15); x++) vote(mx * 16 + x, my * 16 - 2);
    if (left >= 0 && db[left].slice >= 0)
      for (int y = Max(y0, 1); y < Min(y0 + n, 15); y++) vote(mx * 16 - 2, my * 16 + y);
    if (!strength) return 9;
    int best = 0;
    for (int k = 1; k < 9; k++)
      if (hist[k] > hist[best]) best = k;
    return best;
  }

  // x264's AQ energy (ac_energy_mb) of a macroblock's samples: the luma
  // 16x16 (y, stride ys) and the two chroma 8x8 (u, v, stride cs)
  // variances, summed
  static int64_t aq_energy(const uint8_t* y, int ys, const uint8_t* u, const uint8_t* v, int cs) {
    int64_t s1, s2, e;
    blk_sums(y, ys, 16, s1, s2);
    e = s2 - (s1 * s1 >> 8);
    blk_sums(u, cs, 8, s1, s2);
    e += s2 - (s1 * s1 >> 6);
    blk_sums(v, cs, 8, s1, s2);
    return e + s2 - (s1 * s1 >> 6);
  }
  // that of macroblock addr of the current picture
  int64_t ac_energy(int addr) const {
    return aq_energy(mb_px(addr, 0), cur->w, mb_px(addr, 1), mb_px(addr, 2), cur->w / 2);
  }
  // the same of the proxy of the current macroblock (proxy_build); -1 none
  int64_t proxy_energy() const {
    if (!act() || !prx_ok) return -1;
    return aq_energy(prx_y, 16, prx_c[0], prx_c[1], 8);
  }
  // The prediction of the current macroblock were it skipped (rf, mv: the
  // motion of P_Skip or B_Skip), put in the picture where its own will go:
  // how well its luma continues the decoded samples above and to the left,
  // as edge_miss
  int skip_miss(const int8_t rf[2][4], const int16_t mv[2][16][2]) {
    if (!act() || mb_prepared) return -1;
    inter(mb_addr, rf, mv, mb_px(mb_addr, 0), mb_px(mb_addr, 1), mb_px(mb_addr, 2));
    skip_in = true;
    memcpy(skip_rf, rf, sizeof skip_rf);
    memcpy(skip_mv, mv, sizeof skip_mv);
    return miss(0, 0, 0, 16);
  }
  // How far apart the two predictions of a bi-predicted macroblock (rf,
  // mv) are: 4 x the mean absolute difference of their luma, from the
  // blocks of equal motion (8x8); -1 not bi-predicted anywhere
  int bi_diff(const int8_t rf[2][4], const int16_t mv[2][16][2]) {
    if (!act()) return -1;
    int mx = mb_x(mb_addr), my = mb_y(mb_addr), sum = 0, cnt = 0;
    uint8_t p[2][16 * 8];
    // (the macroblock's predictions, which inter() will want next, where
    // they serve: an 8x8 block of one vector a list)
    if (!predicted(mb_addr, rf, mv)) predict(mb_addr, rf, mv);
    bool uni[2] = {pq_ok && one_mv(mv[0]), pq_ok && one_mv(mv[1])};
    for (int b8 = 0; b8 < 4; b8++) {
      if (rf[0][b8] < 0 || rf[1][b8] < 0) continue;
      int x = (b8 & 1) * 8, y = (b8 >> 1) * 8, b = (y / 4) * 4 + x / 4;
      const uint8_t* q[2];  // the two predictions (stride 16)
      for (int l = 0; l < 2; l++) {
        bool one = pq_ok;
        for (int k = 1; k < 4 && one && !uni[l]; k++) {
          int c = b + (k >> 1) * 4 + (k & 1);
          one = mv[l][c][0] == mv[l][b][0] && mv[l][c][1] == mv[l][b][1];
        }
        if (one) {
          q[l] = pq_l[l] + y * 16 + x;
          continue;
        }
        int r = rf[l][b8];
        const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
        mc_luma(ref ? ref : cur, mx + x, my + y, 8, 8, mv[l][b][0], mv[l][b][1], p[l]);
        q[l] = p[l];
      }
      sum += sad_bil(q[0], 16, q[1], 16, 8, 8, 0, 0, INT_MAX);
      cnt += 64;
    }
    return cnt ? sum * 4 / cnt : -1;
  }
  // Template matching: the decoded samples above (2 rows) and to the left
  // (2 columns) of the current macroblock against their prediction by the
  // motion (rf, mv) of its blocks next to them, without weights: 4 x the
  // mean absolute difference (-1 none); per (if given), the same of each
  // list's prediction alone, where it is used
  int tm_miss(const int8_t rf[2][4], const int16_t mv[2][16][2], int* per = nullptr) const {
    if (per) per[0] = per[1] = -1;
    if (!act()) return -1;
    int mx = mb_x(mb_addr), my = mb_y(mb_addr), w = cur->w;
    bool top = above_done(), left = left_done();
    int sum = 0, cnt = 0, ls[2] = {0, 0}, lc[2] = {0, 0};
    // per list, the strip: the rows above, the columns left transposed (both
    // 2 x 16, stride 16)
    uint8_t p[2][32] = {};
    // (a list of one reference and one vector throughout predicts either
    // strip at once)
    bool uni[2];
    for (int l = 0; l < 2; l++) uni[l] = rf[l][1] == rf[l][0] && rf[l][2] == rf[l][0] && rf[l][3] == rf[l][0] && one_mv(mv[l]);
    for (int side = 0; side < 2; side++) {
      if (side == 0 ? !top : !left) continue;
      bool use[2][4];
      tm_pieces(side, mx, my, rf, mv, uni, p, use);
      // the decoded samples, alike
      const uint8_t* q = mb_at(0, 0, -2);
      int qs = w;
      if (side == 1) q = left_strip(), qs = 16;
      tm_compare(p, use, q, qs, per != nullptr, sum, cnt, ls, lc);
    }
    if (per)
      for (int l = 0; l < 2; l++) per[l] = lc[l] ? ls[l] * 4 / lc[l] : -1;
    return cnt ? sum * 4 / cnt : -1;
  }
  // tm_miss()'s strip of a side predicted by each list (into p[l]): the
  // pieces next to the 4x4 blocks 0-3 of the top row (side 0) or 0, 4, 8,
  // 12 of the left column, each run of pieces on the same reference with
  // the same vector at once; use[l][k]: list l predicts piece k
  void tm_pieces(int side, int mx, int my, const int8_t rf[2][4], const int16_t mv[2][16][2], const bool* uni,
                 uint8_t (*p)[32], bool (*use)[4]) const {
    // (the pieces' 4x4 blocks and their 8x8 blocks)
    static const uint8_t kBlk[2][4] = {{0, 1, 2, 3}, {0, 4, 8, 12}}, kB8[2][4] = {{0, 0, 1, 1}, {0, 0, 2, 2}};
    const uint8_t *blk = kBlk[side], *b8 = kB8[side];
    for (int l = 0; l < 2; l++) {
      if (uni[l]) {
        bool u = rf[l][0] >= 0;
        use[l][0] = use[l][1] = use[l][2] = use[l][3] = u;
        if (u) tm_strip(side, mx, my, l, 0, 4, rf[l][0], mv[l][0], p);
        continue;
      }
      for (int k = 0; k < 4;) {
        int b = blk[k], r = rf[l][b8[k]], e = k + 1;
        use[l][k] = r >= 0;
        if (r < 0) {
          k++;
          continue;
        }
        uint32_t v, u;
        memcpy(&v, mv[l][b], 4);
        for (; e < 4; e++) {
          memcpy(&u, mv[l][blk[e]], 4);
          if (rf[l][b8[e]] != r || u != v) break;
          use[l][e] = true;
        }
        tm_strip(side, mx, my, l, k, e, r, mv[l][b], p);
        k = e;
      }
    }
  }
  // the pieces k to e - 1 of a side's strip predicted by list l's
  // reference r with vector v (the macroblock at mx, my)
  void tm_strip(int side, int mx, int my, int l, int k, int e, int r, const int16_t* v, uint8_t (*p)[32]) const {
    const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
    if (!ref) ref = cur;
    int n4 = 4 * (e - k);
    if (side == 0) {
      mc_luma(ref, mx + k * 4, my - 2, n4, 2, v[0], v[1], p[l] + k * 4);
    } else if (!mc_luma_t(ref, mx - 2, my + k * 4, 2, n4, v[0], v[1], p[l] + k * 4)) {
      // (the current picture has no transposed luma: turned here)
      uint8_t t[16 * 16];
      mc_luma(ref, mx - 2, my + k * 4, 2, n4, v[0], v[1], t);
      for (int j = 0; j < n4; j++) p[l][k * 4 + j] = t[j * 16], p[l][16 + k * 4 + j] = t[j * 16 + 1];
    }
  }
  // tm_miss()'s samples of a side decoded (q, stride qs) against the
  // strip's prediction: of the pieces list 0 predicts (else 1), averaged
  // with list 1's where both do, those of neither out; the sums of their
  // absolute differences and the samples they are of, into sum and cnt,
  // and per list (per) ls and lc
  static void tm_compare(const uint8_t (*p)[32], const bool (*use)[4], const uint8_t* q, int qs, bool per, int& sum,
                         int& cnt, int* ls, int* lc) {
#ifdef H264P_AVX2
    {
      // the pieces as 32-bit lanes k and 4 + k: from list 0 where it is
      // used (else 1), averaged with list 1 where both are, those of
      // neither out
      int m1[4], m2[4], mu[4], u1[4];
      for (int k = 0; k < 4; k++) {
        m1[k] = use[0][k] ? 0 : -1;
        m2[k] = use[0][k] && use[1][k] ? -1 : 0;
        mu[k] = use[0][k] || use[1][k] ? -1 : 0;
        u1[k] = use[1][k] ? -1 : 0;
        cnt += mu[k] & 8;
        lc[0] += ~m1[k] & 8, lc[1] += u1[k] & 8;
      }
      __m256i P0 = _mm256_loadu_si256((const __m256i*)p[0]), P1 = _mm256_loadu_si256((const __m256i*)p[1]);
      __m256i M1 = _mm256_setr_epi32(m1[0], m1[1], m1[2], m1[3], m1[0], m1[1], m1[2], m1[3]);
      __m256i a = _mm256_blendv_epi8(P0, P1, M1);
      __m256i b = _mm256_blendv_epi8(a, P1, _mm256_setr_epi32(m2[0], m2[1], m2[2], m2[3], m2[0], m2[1], m2[2], m2[3]));
      __m256i m = _mm256_setr_epi32(mu[0], mu[1], mu[2], mu[3], mu[0], mu[1], mu[2], mu[3]);
      __m256i v = _mm256_and_si256(_mm256_avg_epu8(a, b), m), d = _mm256_and_si256(ld16x2(q, q + qs), m);
      sum += sad_sum(_mm256_sad_epu8(v, d));
      if (per) {
        __m256i U1 = _mm256_setr_epi32(u1[0], u1[1], u1[2], u1[3], u1[0], u1[1], u1[2], u1[3]);
        ls[0] += sad_sum(_mm256_sad_epu8(_mm256_andnot_si256(M1, P0), _mm256_andnot_si256(M1, d)));
        ls[1] += sad_sum(_mm256_sad_epu8(_mm256_and_si256(U1, P1), _mm256_and_si256(U1, d)));
      }
    }
#else
    (void)per;  // (the plain code sums per list anyway)
    for (int k = 0; k < 4; k++) {
      int n = use[0][k] + use[1][k];
      if (!n) continue;
      const uint8_t *p0 = (use[0][k] ? p[0] : p[1]) + k * 4, *p1 = p[1] + k * 4;
      for (int j = 0; j < 2; j++)
        for (int i = 0; i < 4; i++) {
          int v = n == 2 ? (p0[j * 16 + i] + p1[j * 16 + i] + 1) >> 1 : p0[j * 16 + i];
          sum += abs(q[j * qs + k * 4 + i] - v);
          cnt++;
          for (int l = 0; l < 2; l++)
            if (use[l][k]) ls[l] += abs(q[j * qs + k * 4 + i] - p[l][j * 16 + k * 4 + i]), lc[l]++;
        }
    }
#endif
  }
  // Template matching motion search for the luma block (bx, by, bw, bh)
  // of the current macroblock, on reference r of list l: from mv (quarter
  // samples), the vector whose prediction of the decoded samples above (2
  // rows) and to the left (2 columns) of the block misses least: whole
  // samples within 2, then halves, then quarters, down to steps of finest
  // (4: whole samples, 8: none, cand alone; only_y: the vertical component
  // only), cand the other vectors to start from; miss, 4 x the
  // mean absolute difference there. l < 0: on proxy_pic, with 16 rows and
  // columns and the block above right too (the proxy wants the content to
  // match, a vector to code wants a thin template). The template outside the
  // macroblock only, but for inner 1: the block itself, decoded, alone; 2:
  // and the blocks of the macroblock decoded so far. False where none of it
  // is decoded.
  static constexpr int TM_T = 2, TM_TP = 16;
  // The proxy's searches of a macroblock (proxy_mv's, then one per luma
  // block decoded, then one for the chroma) see the same template outside
  // the macroblock, and the same blocks inside but for those added since:
  // what a vector cost, by vector (direct mapped on its low bits), whole
  // sums only. A new macroblock (mb_start) makes it all stale.
  struct TmMemo {
    uint32_t gen;
    int vx, vy;
    int outer;       // the template outside the macroblock (-1 not known)
    int inner;       // the blocks inside: the 4x4 blocks of m4, 8x8 of m8
    uint16_t m4;
    uint8_t m8;
  };
  mutable TmMemo tmm[D::PX ? 1024 : 1];  // (none without samples)
  uint32_t tmm_gen;
  // tm_search's walk from mv (where it leaves the best): the candidates
  // (alt: the best of them but the best, of the start too; INT_MIN none),
  // a small diamond in whole samples while it finds better (up to 2 steps),
  // then the 8 halves and quarters around, down to steps of finest (4:
  // whole samples only, 8: the candidates alone). By cost(vx, vy, bound): a
  // vector's cost, or something not below bound once it is not. The best's
  // cost.
  template <class F>
  static INLINE int tm_walk(const F& cost, int* mv, bool only_y, const int (*cand)[2], int ncand, int finest, int* alt) {
    int vx = mv[0], vy = mv[1], best = cost(vx, vy, INT_MAX);
    int ax = INT_MIN, ay = 0, abest = INT_MAX;
    for (int k = 0; k < ncand; k++) {
      // (a vector seen already changes nothing)
      int j = -1;
      while (++j < k && (cand[j][0] != cand[k][0] || cand[j][1] != cand[k][1])) {
      }
      if (j < k || (cand[k][0] == mv[0] && cand[k][1] == mv[1])) continue;
      int c = cost(cand[k][0], cand[k][1], alt ? INT_MAX : best);
      if (c < best) {
        if (best < abest) abest = best, ax = vx, ay = vy;
        best = c, vx = cand[k][0], vy = cand[k][1];
      } else if (c < abest) {
        abest = c, ax = cand[k][0], ay = cand[k][1];
      }
    }
    if (alt) alt[0] = ax, alt[1] = ay;
    // (back: the step back to where the last came from, known worse)
    static const int kD[4][2] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
    for (int it = 0, back = -1; it < 2 && finest <= 4 && best; it++) {
      int cx = vx, cy = vy, to = -1;
      for (int k = 0; k < 4; k++) {
        if ((only_y && kD[k][0]) || k == back) continue;
        int c = cost(cx + kD[k][0] * 4, cy + kD[k][1] * 4, best);
        if (c < best) best = c, vx = cx + kD[k][0] * 4, vy = cy + kD[k][1] * 4, to = k;
      }
      if (to < 0) break;
      back = 3 - to;
    }
    for (int step = 2; step >= finest && best; step >>= 1) {
      int cx = vx, cy = vy;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          if ((!dx && !dy) || (only_y && dx)) continue;
          int c = cost(cx + dx * step, cy + dy * step, best);
          if (c < best) best = c, vx = cx + dx * step, vy = cy + dy * step;
        }
    }
    mv[0] = vx;
    mv[1] = vy;
    return best;
  }
  // A template search's context (tm_search): the templates of the block
  // at (x0, y0), bw x bh, of the current picture (Y, stride w): the T rows
  // above it (top), the T columns left of it (left; where T is 2 against
  // the transposed reference RT, qt the template transposed), T x T above
  // right (tr), and the blocks decoded inside it (inner 1: all of it, 2:
  // those done; ninner samples); the reference (R, W x H; RP padded,
  // stride ps, readable pm out of it; RT's stride pt). A vector's cost:
  // lambda times its distance from (px, py), plus the templates' sums of
  // absolute differences against the reference displaced by it, bilinear
  // between whole samples (not the six taps of the prediction: fast); or,
  // once not below bound, something not below bound. The search for an
  // intra macroblock's proxy (memo) keeps each vector's in tmm.
  struct TmCx {
    const ReconT* rec;
    const uint8_t *Y, *R, *RP, *RT, *qt;
    int w, W, H, ps, pm, pt, x0, y0, bw, bh, T, inner, ninner, lambda, px, py;
    bool top, left, tr, memo;
    // a rectangle of samples (budget once it reaches budget, as sad_bil)
    int sad(int xs, int ys, int rw, int rh, int vx, int vy, int budget) const {
      int ix = xs + (vx >> 2), iy = ys + (vy >> 2), fx = vx & 3, fy = vy & 3, s = 0;
      if (ix >= -pm && iy >= -pm && ix + rw < W + pm && iy + rh < H + pm)
        return sad_bil(Y + ys * w + xs, w, RP + iy * ps + ix, ps, rw, rh, fx, fy, budget);
      // across the edge: the samples outside are the nearest inside
      int wa = (4 - fx) * (4 - fy), wb = fx * (4 - fy), wc = (4 - fx) * fy, wd = fx * fy;
      for (int j = 0; j < rh && s < budget; j++) {
        const uint8_t* q = Y + (ys + j) * w + xs;
        const uint8_t *a = R + clip3(0, H - 1, iy + j) * W, *c = R + clip3(0, H - 1, iy + j + 1) * W;
        for (int i = 0; i < rw; i++) {
          int x1 = clip3(0, W - 1, ix + i), x2 = clip3(0, W - 1, ix + i + 1);
          s += abs(q[i] - bil1(wa, wb, wc, wd, a[x1], a[x2], c[x1], c[x2], 8, 4));
        }
      }
      return Min(s, budget);
    }
    int sad_left(int vx, int vy, int budget) const {
      int ix = x0 - T + (vx >> 2), iy = y0 + (vy >> 2);
      if (RT && ix >= -pm && iy >= -pm && ix + T < W + pm && iy + bh < H + pm)
        return sad_bil(qt, 16, RT + ix * pt + iy, pt, bh, 2, vy & 3, vx & 3, budget);
      return sad(x0 - T, y0, T, bh, vx, vy, budget);
    }
    // the templates outside the block
    int outer_sad(int vx, int vy, int budget) const {
      int s = 0;
#ifdef H264P_AVX2
      // (the top and the top right as one, rows of 32)
      if (tr && bw == T) {
        int ix = x0 + (vx >> 2), iy = y0 - T + (vy >> 2);
        if (ix >= -pm && iy >= -pm && ix + 2 * T < W + pm && iy + T < H + pm) {
          s = sad_bil32(Y + (y0 - T) * w + x0, w, RP + iy * ps + ix, ps, T, vx & 3, vy & 3, budget);
          if (left && s < budget) s += sad_left(vx, vy, budget - s);
          return s;
        }
      }
#endif
      if (top) s += sad(x0, y0 - T, bw, T, vx, vy, budget);
      if (left && s < budget) s += sad_left(vx, vy, budget - s);
      if (tr && s < budget) s += sad(x0 + bw, y0 - T, T, T, vx, vy, budget - s);
      return s;
    }
    // the whole sums of the 4x4 blocks of m4 and the 8x8 of m8
    int blocks_sad(int vx, int vy, unsigned m4, unsigned m8) const {
      int s = 0;
#ifdef H264P_AVX2
      int ix = x0 + (vx >> 2), iy = y0 + (vy >> 2), fx = vx & 3, fy = vy & 3;
      if (ix >= -pm && iy >= -pm && ix + 16 < W + pm && iy + 16 < H + pm) {
        // (the macroblock inside the margins)
        const uint8_t *q = Y + y0 * w + x0, *a = RP + iy * ps + ix;
        for (; m4; m4 &= m4 - 1) {
          int b = __builtin_ctz(m4), ox = blk_x(b) * 4, oy = blk_y(b) * 4;
          s += sad_bil_blk(q + oy * w + ox, w, a + oy * ps + ox, ps, 4, fx, fy);
        }
        for (; m8; m8 &= m8 - 1) {
          int b8 = __builtin_ctz(m8), ox = (b8 & 1) * 8, oy = (b8 >> 1) * 8;
          s += sad_bil_blk(q + oy * w + ox, w, a + oy * ps + ox, ps, 8, fx, fy);
        }
        return s;
      }
#endif
      for (int b = 0; b < 16; b++)
        if (m4 >> b & 1) s += sad(x0 + blk_x(b) * 4, y0 + blk_y(b) * 4, 4, 4, vx, vy, INT_MAX);
      for (int b8 = 0; b8 < 4; b8++)
        if (m8 >> b8 & 1) s += sad(x0 + (b8 & 1) * 8, y0 + (b8 >> 1) * 8, 8, 8, vx, vy, INT_MAX);
      return s;
    }
    // a vector's cost (the top right and the blocks inside only in the
    // proxy's search, memo: tm_cx())
    int operator()(int vx, int vy, int bound) const {
      int c = lambda * (abs(vx - px) + abs(vy - py));
      if (memo) return memo_cost(vx, vy, bound, c);
      if (top && c < bound) c += sad(x0, y0 - T, bw, T, vx, vy, bound - c);
      if (left && c < bound) c += sad_left(vx, vy, bound - c);
      return c;
    }
    // that of the proxy's search, with what tmm knows of the vector (c: its
    // distance's part)
    int memo_cost(int vx, int vy, int bound, int c) const {
      TmMemo& e = rec->tmm[((unsigned)vx & 31) | ((unsigned)vy & 31) << 5];
      if (e.gen != rec->tmm_gen || e.vx != vx || e.vy != vy) e = {rec->tmm_gen, vx, vy, -1, 0, 0, 0};
      if (top || left || tr) {
        // (the searches for the blocks inside will be back: whole; else
        // whole only where below bound, as below)
        if (e.outer < 0 && inner == 2) e.outer = outer_sad(vx, vy, INT_MAX);
        if (e.outer < 0) {
          if (c >= bound) return c;
          int s = outer_sad(vx, vy, bound - c);
          if (s >= bound - c) return c + s;
          e.outer = s;
        }
        c += e.outer;
      }
      if (!inner || c >= bound) return c;
      if (inner == 1) {
        // the block whole, decoded: what is known of it, and the rest
        if (e.m4 && e.m8) e.inner = e.m4 = e.m8 = 0;
        if (e.m8) {
          e.inner += blocks_sad(vx, vy, 0, ~e.m8 & 15), e.m8 = 15;
        } else if (e.m4) {
          e.inner += blocks_sad(vx, vy, ~e.m4 & 0xFFFF, 0), e.m4 = 0xFFFF;
        } else {
          int s = sad(x0, y0, 16, 16, vx, vy, bound - c);
          if (s >= bound - c) return c + s;
          e.inner = s, e.m4 = 0xFFFF;
        }
      } else {
        unsigned m4 = rec->mb_done4, m8 = rec->mb_done8;
        if ((e.m4 & ~m4) || (e.m8 & ~m8)) e.inner = e.m4 = e.m8 = 0;
        e.inner += blocks_sad(vx, vy, m4 & ~e.m4, m8 & ~e.m8);
        e.m4 = (uint16_t)m4, e.m8 = (uint8_t)m8;
      }
      return c + e.inner;
    }
  };
  // the cost where the templates are thin (T 2) and there is no inner one:
  // inside the margins (there the vectors' whole samples in [xl, xh) x
  // [yl, yh)) both parts at once, by sad_bil2 (ot, ol: where the top
  // template is in RP and the left in RT at the vector 0); TmCx's outside
  struct TmThin {
    const TmCx* cx;
    const uint8_t *RP, *RT;
    int ps, pt, lambda, px, py, xl, xh, yl, yh, ot, ol;
    bool top, left;
    Tpl2 qa, ql;
    void Init(const TmCx& c) {
      cx = &c;
      RP = c.RP, RT = c.RT;
      ps = c.ps, pt = c.pt, lambda = c.lambda, px = c.px, py = c.py;
      xl = 2 - c.pm - c.x0, xh = c.W + c.pm - c.bw - c.x0, yl = 2 - c.pm - c.y0, yh = c.H + c.pm - c.bh - c.y0;
      ot = (c.y0 - 2) * c.ps + c.x0, ol = (c.x0 - 2) * c.pt + c.y0;
      top = c.top, left = c.left;
      qa.Init(c.top ? c.Y + (c.y0 - 2) * c.w + c.x0 : nullptr, c.w, c.bw);
      ql.Init(c.left ? c.qt : nullptr, 16, c.bh);
    }
    int operator()(int vx, int vy, int bound) const {
      int ix = vx >> 2, iy = vy >> 2;
      if (ix < xl || ix >= xh || iy < yl || iy >= yh) return (*cx)(vx, vy, bound);
      int c = lambda * (abs(vx - px) + abs(vy - py));
      if (top) c += sad_bil2(qa, RP + (ot + iy * ps + ix), ps, vx & 3, vy & 3);
      if (left) c += sad_bil2(ql, RT + (ol + ix * pt + iy), pt, vy & 3, vx & 3);
      return c;
    }
  };
  bool tm_search(int l, int r, int bx, int by, int bw, int bh, int* mv, bool only_y, const int (*cand)[2] = nullptr,
                 int ncand = 0, int* miss = nullptr, int lambda = 0, int finest = 1, int inner = 0,
                 int* alt = nullptr) const {
    if (!act()) return false;
    TmCx c;
    if (!tm_cx(c, l, r, bx, by, bw, bh, mv, lambda, inner)) return false;
    // (thin templates but no inner one: TmThin)
    int best;
    if (c.T == 2 && !c.ninner && (!c.left || c.RT)) {
      TmThin th;
      th.Init(c);
      best = tm_walk(th, mv, only_y, cand, ncand, finest, alt);
    } else {
      best = tm_walk(c, mv, only_y, cand, ncand, finest, alt);
    }
    if (miss) *miss = best * 4 / ((c.top ? c.T * bw : 0) + (c.left ? c.T * bh : 0) + (c.tr ? c.T * c.T : 0) + c.ninner);
    return true;
  }
  // tm_search's context (as there); false: no template or no reference
  bool tm_cx(TmCx& c, int l, int r, int bx, int by, int bw, int bh, const int* mv, int lambda, int inner) const {
    int mbw = cur->mbw;
    c.rec = this;
    c.x0 = mb_x(mb_addr) + bx, c.y0 = mb_y(mb_addr) + by, c.w = cur->w, c.bw = bw, c.bh = bh;
    c.top = by == 0 && above_done(), c.left = bx == 0 && left_done();
    const RPic* ref = l < 0 ? proxy_pic() : r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
    if (inner == 1) c.top = c.left = false;
    c.inner = inner;
    c.ninner = inner == 1 ? bw * bh : 0;
    if (inner == 2) {
      for (int b = 0; b < 16; b++) c.ninner += (mb_done4 >> b & 1) * 16;
      for (int b8 = 0; b8 < 4; b8++) c.ninner += (mb_done8 >> b8 & 1) * 64;
    }
    if ((!c.top && !c.left && !c.ninner) || !ref) return false;
    c.R = ref->y.data(), c.Y = cur->y.data();
    c.W = ref->w, c.H = ref->h;
    c.RP = ref->luma(c.ps, c.pm);
    c.lambda = lambda, c.px = mv[0], c.py = mv[1];
    c.T = l < 0 ? TM_TP : TM_T;
    c.tr = l < 0 && c.top && c.x0 + bw + c.T <= c.w && mb_x(mb_addr, 1) + 1 < mbw && db[mb_addr - mbw + 1].slice >= 0;
    // a thin left template against the transposed reference: two rows of
    // bh (and the template transposed, qt)
    c.pt = 0;
    c.RT = c.left && c.T == 2 ? ref->luma_t(c.pt) : nullptr;
    c.qt = c.RT ? left_strip() + by : nullptr;
    c.memo = l < 0 && bx == 0 && by == 0 && bw == 16 && bh == 16;
    return true;
  }
  // a stand-in for the source of the current (intra) macroblock: its
  // prediction from proxy_pic by a vector (proxy_build), luma and chroma
  uint8_t prx_y[256], prx_c[2][64];
  bool prx_ok;
  bool proxy_build(const int* mv) {
    prx_ok = false;
    const RPic* P = act() ? proxy_pic() : nullptr;
    if (!P) return false;
    int mx = mb_x(mb_addr), my = mb_y(mb_addr);
    mc_luma(P, mx, my, 16, 16, mv[0], mv[1], prx_y);
    for (int c = 0; c < 2; c++) mc_chroma(P, c, mx / 2, my / 2, 8, 8, mv[0], mv[1], prx_c[c]);
    prx_ok = true;
    return true;
  }
  // how far the luma of the current macroblock decoded so far is from the
  // proxy: 4 x the mean absolute difference; -1 none decoded or no proxy
  int proxy_acc() const {
    if (!act() || !prx_ok) return -1;
    int w = cur->w, sum = 0, cnt = 0;
    const uint8_t* Y = mb_at(0, 0, 0);
#ifdef H264P_AVX2
    {
      // by rows of 4x4 blocks, those done (of the 4x4 ones, then of the 8x8)
      // as a mask of their bytes in each row
      uint8_t g[2][4] = {};
      for (int b = 0; b < 16; b++)
        if (mb_done4 >> b & 1) g[0][blk_y(b)] |= (uint8_t)(1 << blk_x(b)), cnt += 16;
      for (int b8 = 0; b8 < 4; b8++)
        if (mb_done8 >> b8 & 1) g[1][(b8 >> 1) * 2] |= (uint8_t)(3 << (b8 & 1) * 2), cnt += 64;
      for (int b8 = 0; b8 < 4; b8 += 2) g[1][b8 + 1] = g[1][b8];
      __m128i acc = _mm_setzero_si128();
      for (int k = 0; k < 2; k++)
        for (int by = 0; by < 4; by++) {
          int m = g[k][by];
          if (!m) continue;
          const __m128i mask = _mm_setr_epi32(-(m & 1), -(m >> 1 & 1), -(m >> 2 & 1), -(m >> 3 & 1));
          for (int y = by * 4; y < by * 4 + 4; y++) {
            __m128i a = _mm_loadu_si128((const __m128i*)(Y + y * w)), c = _mm_loadu_si128((const __m128i*)(prx_y + y * 16));
            __m128i d = _mm_and_si128(_mm_or_si128(_mm_subs_epu8(a, c), _mm_subs_epu8(c, a)), mask);
            acc = _mm_add_epi64(acc, _mm_sad_epu8(d, _mm_setzero_si128()));
          }
        }
      sum = _mm_cvtsi128_si32(_mm_add_epi64(acc, _mm_unpackhi_epi64(acc, acc)));
      return cnt ? sum * 4 / cnt : -1;
    }
#endif
    auto add = [&](int x0, int y0, int n) {
      for (int y = y0; y < y0 + n; y++)
        for (int x = x0; x < x0 + n; x++) sum += abs(Y[y * w + x] - prx_y[y * 16 + x]);
      cnt += n * n;
    };
    for (int b = 0; b < 16; b++)
      if (mb_done4 >> b & 1) add(blk_x(b) * 4, blk_y(b) * 4, 4);
    for (int b8 = 0; b8 < 4; b8++)
      if (mb_done8 >> b8 & 1) add((b8 & 1) * 8, (b8 >> 1) * 8, 8);
    return cnt ? sum * 4 / cnt : -1;
  }
  const RPic* proxy_pic() const {
    const RPic* P = nlist[0] > 0 && list[0][0] ? list[0][0] : !dpb.empty() ? dpb.back() : nullptr;
    return P && P->w == cur->w && P->h == cur->h ? P : nullptr;
  }
  // What proxy_nz() and proxy_mode() share: the block of the proxy picture
  // where the luma block of n x n at (bx, by) blocks of the current
  // macroblock is, displaced by dx, dy (within the picture): its place
  // (x0, y0), and its edge e as intra prediction has it, whether its top,
  // left and top left are there going by the picture's edges; null no
  // picture
  const RPic* proxy_block(int bx, int by, int n, int dx, int dy, int& x0, int& y0, int* e, bool& top, bool& left,
                          bool& tl) const {
    if (!act()) return nullptr;
    const RPic* P = proxy_pic();
    if (!P) return nullptr;
    int w = P->w;
    x0 = clip3(0, w - n, mb_x(mb_addr) + bx * n + dx);
    y0 = clip3(0, P->h - n, mb_y(mb_addr) + by * n + dy);
    const uint8_t* Y = P->y.data();
    top = y0 > 0, left = x0 > 0, tl = top && left;
    bool tr = top && x0 + 2 * n <= w;
    edge_fill(n, x0, y0, top, left, tl, tr, e, [&](int x, int y) { return (int)Y[y * w + x]; });
    return P;
  }
  // The nonzero levels an encoder would have for the luma block of n x n at
  // (bx, by) blocks of the current macroblock in intra mode m were the
  // source and its surroundings the proxy picture there (displaced by dx,
  // dy); -1 no picture
  int proxy_nz(int bx, int by, int n, int m, int qp, int dx, int dy) const {
    int x0, y0, e[25];
    bool top, left, tl;
    const RPic* P = proxy_block(bx, by, n, dx, dy, x0, y0, e, top, left, tl);
    if (!P) return -1;
    int w = P->w;
    const uint8_t* Y = P->y.data();
    uint8_t d[64];
    pred_nxn(d, 8, n, m, e, top, left);
    int r[64], lv[64], cnt = 0;
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) r[y * n + x] = Y[(y0 + y) * w + x0 + x] - d[y * 8 + x];
    fquant(r, n, qp, lv);
    for (int k = 0; k < n * n; k++) cnt += abs(lv[k]) >= 3;
    return cnt;
  }
  int proxy_mode(int bx, int by, int n, int dx, int dy, int* sads = nullptr) const {
    int x0, y0, e[25];
    bool top, left, tl;
    const RPic* P = proxy_block(bx, by, n, dx, dy, x0, y0, e, top, left, tl);
    if (!P) return -1;
    int w = P->w;
    const uint8_t* Y = P->y.data();
    int best = -1, bsad = 1 << 30;
    uint8_t d[64];
    for (int m = 0; m < 9; m++) {
      if (sads) sads[m] = INT_MAX;
      if (((m == 0 || m == 3 || m == 7) && !top) || ((m == 1 || m == 8) && !left) || ((m >= 4 && m <= 6) && !tl)) continue;
      pred_nxn(d, 8, n, m, e, top, left);
      // the sum of the absolute Hadamard transformed differences, by 4x4
      // (x264's measure), halved
      int sad = satd_nxn(Y + y0 * w + x0, w, d, 8, n);
      if (sads) sads[m] = sad;
      if (sad < bsad) bsad = sad, best = m;
    }
    return best;
  }
  // The forward transform and quantization of residual r (raster) of n x n
  // (4, 8): the levels in zigzag order, in quarters, not rounded (an
  // encoder rounding intra levels up from 2/3 would code (3 |q| + 4) / 12)
  static void fquant(const int* r, int n, int qp, int* lv) {
    if (n == 8)
      fquant8(r, qp, lv);
    else
      fquant4(r, qp, lv);
  }
  static void fquant4(const int* r, int qp, int* lv) {
    int t[16], W[16];
    for (int i = 0; i < 4; i++) {
      const int* x = r + i * 4;
      int a = x[0] + x[3], b = x[1] + x[2], c = x[1] - x[2], d = x[0] - x[3];
      t[i * 4] = a + b;
      t[i * 4 + 1] = 2 * d + c;
      t[i * 4 + 2] = a - b;
      t[i * 4 + 3] = d - 2 * c;
    }
    for (int j = 0; j < 4; j++) {
      int a = t[j] + t[12 + j], b = t[4 + j] + t[8 + j], c = t[4 + j] - t[8 + j], d = t[j] - t[12 + j];
      W[j] = a + b;
      W[4 + j] = 2 * d + c;
      W[8 + j] = a - b;
      W[12 + j] = d - 2 * c;
    }
    int qb = 13 + qp / 6;
    for (int k = 0; k < 16; k++) {
      int p = kZz4[k], y = p >> 2, x = p & 3;
      int m = kQuant4[qp % 6][norm4_class(y, x)];
      int q = (int)(((int64_t)abs(W[p]) * m) >> qb);
      lv[k] = W[p] < 0 ? -q : q;
    }
  }
  // (x264's dct8)
  static void fquant8(const int* r, int qp, int* lv) {
    static const int mf[6][6] = {{13107, 11428, 20972, 12222, 16777, 15481}, {11916, 10826, 19174, 11058, 14980, 14290},
                                 {10082, 8943, 15978, 9675, 12710, 11985},   {9362, 8228, 14913, 8931, 11984, 11259},
                                 {8192, 7346, 13159, 7740, 10486, 9777},     {7282, 6428, 11570, 6830, 9118, 8640}};
    int t[64], W[64];
    auto dct8 = [](const int* x, int xs, int* d, int ds) {
      int s07 = x[0] + x[7 * xs], s16 = x[xs] + x[6 * xs], s25 = x[2 * xs] + x[5 * xs], s34 = x[3 * xs] + x[4 * xs];
      int a0 = s07 + s34, a1 = s16 + s25, a2 = s07 - s34, a3 = s16 - s25;
      int d07 = x[0] - x[7 * xs], d16 = x[xs] - x[6 * xs], d25 = x[2 * xs] - x[5 * xs], d34 = x[3 * xs] - x[4 * xs];
      int a4 = d16 + d25 + (d07 + (d07 >> 1)), a5 = d07 - d34 - (d25 + (d25 >> 1));
      int a6 = d07 + d34 - (d16 + (d16 >> 1)), a7 = d16 - d25 + (d34 + (d34 >> 1));
      d[0] = a0 + a1;
      d[ds] = a4 + (a7 >> 2);
      d[2 * ds] = a2 + (a3 >> 1);
      d[3 * ds] = a5 + (a6 >> 2);
      d[4 * ds] = a0 - a1;
      d[5 * ds] = a6 - (a5 >> 2);
      d[6 * ds] = (a2 >> 1) - a3;
      d[7 * ds] = (a4 >> 2) - a7;
    };
    for (int i = 0; i < 8; i++) dct8(r + i * 8, 1, t + i * 8, 1);
    for (int j = 0; j < 8; j++) dct8(t + j, 8, W + j, 8);
    int qb = 14 + qp / 6;
    for (int k = 0; k < 64; k++) {
      int p = kZz8[k];
      int q = (int)(((int64_t)abs(W[p]) * mf[qp % 6][norm8_class(p >> 3, p & 7)]) >> qb);
      lv[k] = W[p] < 0 ? -q : q;
    }
  }
  // The levels of a block of n x n (4, 8) were the source the proxy
  // (proxy_build): the proxy less the prediction in the picture, through
  // fquant() at qp. c 0 luma, 1 and 2 chroma; x, y in its
  // samples of the current macroblock. false: no proxy.
  bool proxy_levels(int c, int x, int y, int qp, int* lv, int n = 4) const {
    if (!act() || !prx_ok) return false;
    int w = stride(c), m = c ? 8 : 16;
    const uint8_t* C = mb_at(c, x, y);
    const uint8_t* Q = (c ? prx_c[c - 1] : prx_y) + y * m + x;
    int r[64];
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) r[j * n + i] = Q[j * m + i] - C[j * w + i];
    fquant(r, n, qp, lv);
    return true;
  }
  // the levels of the prediction's horizontal and vertical gradients
  // (central differences, from the picture around the block too), as
  // fquant() at qp: what a residual of a small shift of the block
  // would have, per unit of shift each way
  bool pred_grad_levels(int c, int x, int y, int n, int qp, int* lx, int* ly) const {
    if (!act() || !mb_prepared || mb_cls == 8) return false;
    int w = stride(c), h = c ? cur->h / 2 : cur->h, m = c ? 8 : 16;
    int mx = mb_x(mb_addr, m) + x, my = mb_y(mb_addr, m) + y;
    const uint8_t* C = plane(c);
    auto at = [&](int i, int j) { return (int)C[clip3(0, h - 1, my + j) * w + clip3(0, w - 1, mx + i)]; };
    int gx[64], gy[64];
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) {
        gx[j * n + i] = at(i + 1, j) - at(i - 1, j);
        gy[j * n + i] = at(i, j + 1) - at(i, j - 1);
      }
    fquant(gx, n, qp, lx), fquant(gy, n, qp, ly);
    return true;
  }
  // the same of the prediction itself (in the picture, before the
  // residual): its texture by frequency, in quarter levels
  bool pred_levels(int c, int x, int y, int qp, int* lv, int n = 4) const {
    if (!act() || !mb_prepared || mb_cls == 8) return false;
    int w = stride(c);
    const uint8_t* C = mb_at(c, x, y);
    int r[64];
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) r[j * n + i] = C[j * w + i];
    fquant(r, n, qp, lv);
    return true;
  }
  // and for the DC blocks: Intra16x16's (plane 0: 16 levels in zigzag
  // order) and chroma's (planes 1, 2: 4 levels)
  bool proxy_dc(int c, int qp, int* lv) const {
    if (!act() || !prx_ok) return false;
    int w = stride(c), m = c ? 8 : 16, nb = m / 4;
    const uint8_t* C = mb_px(mb_addr, c);
    const uint8_t* Q = c ? prx_c[c - 1] : prx_y;
    // the DC of each 4x4 block's transform: the sum of its residual
    int d[16], f[16];
    for (int by = 0; by < nb; by++)
      for (int bx = 0; bx < nb; bx++) {
        int sum = 0;
        for (int j = 0; j < 4; j++)
          for (int i = 0; i < 4; i++) sum += Q[(by * 4 + j) * m + bx * 4 + i] - C[(by * 4 + j) * w + bx * 4 + i];
        d[by * nb + bx] = sum;
      }
    int qb = 15 + qp / 6, mf0 = kQuant4[qp % 6][0];
    if (nb == 2) {
      hadamard2x2(d, f);
      for (int k = 0; k < 4; k++) {
        int q = (int)(((int64_t)abs(f[k]) * mf0) >> (qb - 1));
        lv[k] = f[k] < 0 ? -q : q;
      }
      return true;
    }
    int t[16];
    for (int i = 0; i < 4; i++) hadamard4(d + i * 4, 1, t + i * 4, 1);
    for (int j = 0; j < 4; j++) hadamard4(t + j, 4, f + j, 4);
    for (int k = 0; k < 16; k++) {
      int v = f[kZz4[k]] / 2;
      int q = (int)(((int64_t)abs(v) * mf0) >> (qb - 1));
      lv[k] = v < 0 ? -q : q;
    }
    return true;
  }
  // the texture of the luma of the current macroblock in the picture (its
  // prediction, before the residual): the sum of the absolute differences
  // between neighbouring samples
  int mb_texture() const {
    if (!act()) return -1;
    int w = cur->w, gx, gy;
    grads(mb_at(0, 0, 0), w, 16, gx, gy);
    return gx + gy;
  }
  // the same of reference r of list l displaced by mv (whole samples) at
  // the luma block (bx, by, bw, bh) of the current macroblock; -1 none
  int ref_texture(int l, int r, int bx, int by, int bw, int bh, const int* mv, int dir = 3) const {
    const RPic* ref = act() && r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
    if (!ref || bw >= ref->w || bh >= ref->h) return -1;  // no room for the block and its gradients
    int W = ref->w, H = ref->h;
    int x0 = clip3(0, W - bw - 1, mb_x(mb_addr) + bx + (mv[0] >> 2));
    int y0 = clip3(0, H - bh - 1, mb_y(mb_addr) + by + (mv[1] >> 2));
    const uint8_t* p = ref->y.data() + y0 * W + x0;
    int g = 0;
#ifdef H264P_AVX2
    if (bw == 16 || bw == 8 || bw == 4) {
      auto ld = [bw](const uint8_t* q) {
        if (bw == 16) return _mm_loadu_si128((const __m128i*)q);
        if (bw == 8) return _mm_loadl_epi64((const __m128i*)q);
        int32_t u;
        memcpy(&u, q, 4);
        return _mm_cvtsi32_si128(u);
      };
      __m128i a = _mm_setzero_si128(), x = ld(p);
      for (int j = 0; j < bh; j++) {
        __m128i y = ld(p + (j + 1) * W);
        if (dir & 1) a = _mm_add_epi64(a, _mm_sad_epu8(x, ld(p + j * W + 1)));
        if (dir & 2) a = _mm_add_epi64(a, _mm_sad_epu8(x, y));
        x = y;
      }
      return _mm_cvtsi128_si32(_mm_add_epi64(a, _mm_unpackhi_epi64(a, a)));
    }
#endif
    for (int j = 0; j < bh; j++)
      for (int i = 0; i < bw; i++)
        g += (dir & 1 ? abs(p[j * W + i + 1] - p[j * W + i]) : 0) + (dir & 2 ? abs(p[(j + 1) * W + i] - p[j * W + i]) : 0);
    return g;
  }
  // The texture of the prediction of a block of n x n samples at (x, y) of
  // the current macroblock in plane c (0 luma): the sum of the absolute
  // differences between neighbouring samples; -1 not known.
  int pred_texture(int c, int x, int y, int n, int* gxy = nullptr) const {
    if (!act() || !mb_prepared || mb_cls == 8) return -1;
    int gx, gy;
    grads(mb_at(c, x, y), stride(c), n, gx, gy);
    if (gxy) gxy[0] = gx, gxy[1] = gy;
    return gx + gy;
  }

  // For sign prediction: the samples next to the luma block of n x n at (x,
  // y) of the current macroblock, its prediction being in the picture: per
  // edge sample e (n above, then n to the left) the linear extrapolation of
  // the two decoded rows or columns beyond the edge less the prediction
  // there, times scale; avail[e] false where those are not decoded.
  bool sign_edges(int c, int x, int y, int n, int64_t scale, int64_t* target, bool* avail) const {
    if (!act() || !mb_prepared || mb_cls == 8) return false;
    return edges(c, x, y, n, scale, target, avail);
  }
  // the sample at (x, y) of the current macroblock in plane c, the plane's
  // stride (w), and whether the samples above it are decoded (top) and
  // those left of it (left): what edges() and miss() look at
  const uint8_t* edge_at(int c, int x, int y, int& w, bool& top, bool& left) const {
    w = stride(c);
    top = y > 0 || above_done(), left = x > 0 || left_done();
    return mb_at(c, x, y);
  }
  // the same, the macroblock's prediction there or not (miss() too)
  bool edges(int c, int x, int y, int n, int64_t scale, int64_t* target, bool* avail) const {
    int w;
    bool top, left;
    const uint8_t* p = edge_at(c, x, y, w, top, left);
    if (!top && !left) return false;
    for (int i = 0; i < n; i++) {
      avail[i] = top;
      target[i] = top ? (2 * p[-w + i] - p[-2 * w + i] - p[i]) * scale : 0;
      avail[n + i] = left;
      target[n + i] = left ? (2 * p[i * w - 1] - p[i * w - 2] - p[i * w]) * scale : 0;
    }
    return true;
  }

  // How far the prediction of a block of n x n samples at (x, y) of plane c
  // is from continuing the decoded samples around it: the mean over its
  // top and left edge samples of |2 R1 - R2 - P|, in quarter samples; -1
  // not known.
  int edge_miss(int c, int x, int y, int n) const {
    if (!act() || !mb_prepared || mb_cls == 8) return -1;
    return miss(c, x, y, n);
  }
  int miss(int c, int x, int y, int n) const {
#ifdef H264P_AVX2
    if (n == 16 || n == 8) {
      // edges()' sums, the row above in 16-bit lanes
      int w;
      bool top, left;
      const uint8_t* p = edge_at(c, x, y, w, top, left);
      if (!top && !left) return -1;
      // (rows of 8 in the low half, the high half zero: a cast would leave it
      // undefined, and the sum below adds it in)
      auto ld = [n](const uint8_t* q) {
        return n == 16 ? _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)q))
                       : _mm256_inserti128_si256(_mm256_setzero_si256(), _mm_cvtepu8_epi16(_mm_loadl_epi64((const __m128i*)q)), 0);
      };
      // |2 R1 - R2 - P| in 16-bit lanes, the top's and the left's added
      __m256i a = _mm256_setzero_si256();
      if (top) {
        __m256i r1 = ld(p - w), v = _mm256_sub_epi16(_mm256_sub_epi16(_mm256_add_epi16(r1, r1), ld(p - 2 * w)), ld(p));
        a = _mm256_abs_epi16(v);
      }
      if (left) {
        // the columns as rows: of the samples left of a luma macroblock
        // gathered already, else here
        auto col = [n, w](const uint8_t* q) {
          if (n == 16)
            return _mm256_cvtepu8_epi16(_mm_setr_epi8(q[0], q[w], q[2 * w], q[3 * w], q[4 * w], q[5 * w], q[6 * w], q[7 * w], q[8 * w],
                                                      q[9 * w], q[10 * w], q[11 * w], q[12 * w], q[13 * w], q[14 * w], q[15 * w]));
          __m128i h = _mm_setr_epi8(q[0], q[w], q[2 * w], q[3 * w], q[4 * w], q[5 * w], q[6 * w], q[7 * w], 0, 0, 0, 0, 0, 0, 0, 0);
          return _mm256_inserti128_si256(_mm256_setzero_si256(), _mm_cvtepu8_epi16(h), 0);
        };
        __m256i r2, r1;
        if (c == 0 && x == 0) {
          const uint8_t* q = left_strip() + y;
          r2 = ld(q), r1 = ld(q + 16);
        } else {
          r2 = col(p - 2), r1 = col(p - 1);
        }
        a = _mm256_add_epi16(a, _mm256_abs_epi16(_mm256_sub_epi16(_mm256_sub_epi16(_mm256_add_epi16(r1, r1), r2), col(p))));
      }
      int64_t sum = 4 * (int64_t)sum32x8(_mm256_madd_epi16(a, _mm256_set1_epi16(1)));
      return (int)(sum / (n * (top + left)));
    }
#endif
    int64_t t[32];
    bool av[32];
    if (!edges(c, x, y, n, 4, t, av)) return -1;
    int64_t sum = 0;
    int cnt = 0;
    for (int e = 0; e < 2 * n; e++)
      if (av[e]) sum += t[e] < 0 ? -t[e] : t[e], cnt++;
    return cnt ? (int)(sum / cnt) : -1;
  }
};
typedef ReconT<2> Recon;
typedef ReconT<1> ReconRefs;
typedef ReconT<0> ReconOff;

}  // namespace h264p
