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
// reconstructed.

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

static INLINE int clip1(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
static INLINE int clip3(int lo, int hi, int v) { return v < lo ? lo : v > hi ? hi : v; }
static int chroma_qp(int qpy, int off) {
  int qpi = clip3(0, 51, qpy + off);
  return qpi < 30 ? qpi : kQpc[qpi - 30];
}

// --- pictures

// A decoded picture, and what decoding later pictures needs of it.
struct RPic {
  int w = 0, h = 0, mbw = 0, mbh = 0;  // luma size (whole macroblocks), in macroblocks
  std::vector<uint8_t> y, u, v;
  int64_t id = 0;  // which picture it is (a reference picture of a block is compared by it)
  int poc = 0, frame_num = 0, lt_idx = -1;
  bool ref = false, lt = false;  // used for reference, long-term
  // motion of the macroblocks, for direct prediction (8.4.1.2): per
  // macroblock whether intra, per 8x8 block and list refIdx (-1 none) and the
  // id of the picture it refers to, per 4x4 block and list the vector
  std::vector<uint8_t> intra;
  std::vector<int8_t> rf;      // [(mb * 4 + b8) * 2 + list]
  std::vector<int64_t> rpid;   // likewise
  std::vector<int16_t> mv;     // [((mb * 16 + blk) * 2 + list) * 2 + comp]
  bool lt_ref[2][33] = {};     // whether its references were long-term, per list and index

  void alloc(int mw, int mh) {
    mbw = mw;
    mbh = mh;
    w = mw * 16;
    h = mh * 16;
    y.assign((size_t)w * h, 0);
    u.assign((size_t)w * h / 4, 128);
    v.assign((size_t)w * h / 4, 128);
    intra.assign((size_t)mw * mh, 1);
    rf.assign((size_t)mw * mh * 8, -1);
    rpid.assign((size_t)mw * mh * 8, -1);
    mv.assign((size_t)mw * mh * 64, 0);
  }
};

// What deblocking needs of a macroblock of the current picture.
struct DbMb {
  int slice = -1;  // slice number in the picture, -1 not decoded
  uint8_t intra, t8, pcm, qp, disable;
  int8_t alpha_off, beta_off, cqp[2];
  uint8_t nz[16];          // 4x4 blocks with coefficients (raster)
  int64_t rp[2][4];        // reference pictures of the 8x8 blocks, -1 none
  int16_t mv[2][16][2];
};

struct Recon {
  bool on = false;       // the current stream is reconstructed
  bool active = false;   // the current picture is
  std::vector<std::unique_ptr<RPic>> dpb;  // reference pictures
  std::unique_ptr<RPic> cur;               // the picture being decoded
  std::vector<std::unique_ptr<RPic>> spare;  // pictures to reuse
  int64_t next_id = 1;
  std::vector<DbMb> db;
  int slice_no = 0;
  // the current picture's header values that marking needs
  int cur_idr = 0, cur_ref_idc = 0, cur_lt_flag = 0, cur_mmco5 = 0;
  std::vector<std::vector<int64_t>> cur_mmco;
  int max_frame_num = 16, max_num_ref = 1, max_lt_idx = -1;  // max_lt_idx: MaxLongTermFrameIdx, -1 none
  // POC state (8.2.1)
  int prev_poc_msb = 0, prev_poc_lsb = 0, prev_frame_num = 0;
  int64_t prev_frame_num_offset = 0;
  // the current slice
  RPic* list[2][33] = {};
  int nlist[2] = {0, 0};
  int curr_pic_num = 0;
  // weighted prediction of the slice: per list and reference, luma w, o,
  // Cb w, o, Cr w, o; logWD luma, chroma; 0 default, 1 explicit, 2 implicit
  int wmode = 0, lwd = 0, cwd = 0;
  int wt[2][32][6];
  int implicit_w[32][32];  // implicit bi-prediction weight of list 0 (64 - w for list 1)
  int cqp_off[2] = {0, 0};
  int stype = 0, disable = 0, alpha_off = 0, beta_off = 0;
  bool constrained_intra = false, direct_spatial = true, direct8x8 = true;
  // what the model sees of the last picture decoded: per macroblock the
  // energy of its luma (4 log2 of 1 + the sum of squared deviations), the
  // mean of that over the picture, and how much the means of its 4x4 blocks
  // changed from the picture before (16 x the mean absolute difference; -1
  // none)
  std::vector<int> energy, change;
  int energy_mean = 0;
  std::vector<int> means;  // 16 x the means of the 4x4 blocks, for the next picture's change
  int maps_w = 0, maps_h = 0;

  // 4 log2 v, rounded down to quarters (integers only: the model must come
  // out the same everywhere)
  static int log2q(int64_t v) {
    int n = 0;
    while (v >> (n + 1)) n++;
    return 4 * n + (n >= 2 ? (int)(v >> (n - 2) & 3) : n == 1 ? (int)(v & 1) * 2 : 0);
  }
  void make_maps() {
    int mbw = cur->mbw, mbh = cur->mbh, w = cur->w, n = mbw * mbh;
    bool same = maps_w == mbw && maps_h == mbh && (int)means.size() == n * 16;
    energy.assign(n, 0);
    change.assign(n, -1);
    std::vector<int> m(n * 16);
    for (int a = 0; a < n; a++) {
      const uint8_t* p = cur->y.data() + (a / mbw) * 16 * w + (a % mbw) * 16;
      int64_t s1 = 0, s2 = 0;
      for (int b = 0; b < 16; b++) {
        int bs1 = 0;
        const uint8_t* q = p + (b >> 2) * 4 * w + (b & 3) * 4;
        for (int y = 0; y < 4; y++)
          for (int x = 0; x < 4; x++) {
            int v = q[y * w + x];
            bs1 += v;
            s2 += v * v;
          }
        s1 += bs1;
        m[a * 16 + b] = bs1;  // 16 x the mean
      }
      int64_t ssd = (s2 * 256 - s1 * s1) / 256;  // the sum of squared deviations
      energy[a] = log2q(ssd + 1);
      if (same) {
        int d = 0;
        for (int b = 0; b < 16; b++) d += abs(m[a * 16 + b] - means[a * 16 + b]);
        change[a] = d / 16;
      }
    }
    int64_t sum = 0;
    for (int a = 0; a < n; a++) sum += energy[a];
    energy_mean = n ? (int)(sum / n) : 0;
    means.swap(m);
    maps_w = mbw;
    maps_h = mbh;
  }

  // --- reference picture management (8.2.4, 8.2.5)
  int pic_num(const RPic* p) const { return p->lt ? p->lt_idx : (p->frame_num > cur->frame_num ? p->frame_num - max_frame_num : p->frame_num); }

  void finish_picture() {
    if (!cur) return;
    if (active) {
      deblock();
      make_maps();
    }
    // checking the reconstruction against a decoder's: H264P_RECDUMP=file
    // gets the pictures, in decoding order, as raw 4:2:0 of whole macroblocks
    static FILE* dump = getenv("H264P_RECDUMP") ? fopen(getenv("H264P_RECDUMP"), "wb") : nullptr;
    if (dump && active) {
      fwrite(cur->y.data(), 1, cur->y.size(), dump);
      fwrite(cur->u.data(), 1, cur->u.size(), dump);
      fwrite(cur->v.data(), 1, cur->v.size(), dump);
    }
    if (cur_ref_idc) mark();
    if (cur->ref)
      dpb.push_back(std::move(cur));
    else
      spare.push_back(std::move(cur));
    active = false;
  }

  void mark() {
    RPic* c = cur.get();
    if (cur_idr) {
      dpb.clear();
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
    bool lt_done = false;
    if (!cur_mmco.empty()) {
      for (auto& op : cur_mmco) {
        int64_t a = op.size() > 1 ? op[1] : 0;
        switch (op[0]) {
          case 1: {  // a short-term picture no longer used for reference
            int pn = curr_pic_num - (int)(a + 1);
            for (auto& p : dpb)
              if (p->ref && !p->lt && pic_num(p.get()) == pn) p->ref = false;
            break;
          }
          case 2:  // a long-term picture
            for (auto& p : dpb)
              if (p->ref && p->lt && p->lt_idx == a) p->ref = false;
            break;
          case 3: {  // a short-term picture becomes long-term
            int pn = curr_pic_num - (int)(a + 1);
            int idx = (int)(op.size() > 2 ? op[2] : 0);
            for (auto& p : dpb)
              if (p->ref && p->lt && p->lt_idx == idx) p->ref = false;
            for (auto& p : dpb)
              if (p->ref && !p->lt && pic_num(p.get()) == pn) {
                p->lt = true;
                p->lt_idx = idx;
              }
            break;
          }
          case 4:
            max_lt_idx = (int)a - 1;
            for (auto& p : dpb)
              if (p->ref && p->lt && p->lt_idx > max_lt_idx) p->ref = false;
            break;
          case 5:
            for (auto& p : dpb) p->ref = false;
            max_lt_idx = -1;
            break;
          case 6:
            for (auto& p : dpb)
              if (p->ref && p->lt && p->lt_idx == a) p->ref = false;
            c->lt = true;
            c->lt_idx = (int)a;
            lt_done = true;
            break;
        }
      }
    } else {
      // sliding window
      int ns = 0, nl = 0;
      for (auto& p : dpb)
        if (p->ref) (p->lt ? nl : ns)++;
      if (ns + nl >= std::max(max_num_ref, 1) && ns > 0) {
        RPic* old = nullptr;
        for (auto& p : dpb)
          if (p->ref && !p->lt && (!old || pic_num(p.get()) < pic_num(old))) old = p.get();
        if (old) old->ref = false;
      }
    }
    (void)lt_done;
    c->ref = true;
    // pictures no longer used for reference go
    std::vector<std::unique_ptr<RPic>> keep;
    for (auto& p : dpb)
      if (p->ref)
        keep.push_back(std::move(p));
      else
        spare.push_back(std::move(p));
    dpb.swap(keep);
    if (cur_mmco5) {
      c->frame_num = 0;
      c->poc = 0;
    }
  }

  // POC of the current picture (8.2.1)
  int poc(const SliceHeader& h) {
    const Sps& sps = *h.sps;
    bool idr = h.nal_unit_type == 5;
    if (sps.poc_type == 0) {
      int max_lsb = 1 << sps.log2_max_poc_lsb;
      int pm = idr ? 0 : prev_poc_msb, pl = idr ? 0 : prev_poc_lsb;
      int msb = pm;
      if (h.poc_lsb < pl && pl - h.poc_lsb >= max_lsb / 2)
        msb = pm + max_lsb;
      else if (h.poc_lsb > pl && h.poc_lsb - pl > max_lsb / 2)
        msb = pm - max_lsb;
      int top = msb + h.poc_lsb, bot = top + (int)h.delta_poc_bottom;
      if (h.nal_ref_idc) {
        prev_poc_msb = msb;
        prev_poc_lsb = h.poc_lsb;
        if (h.mmco5) {
          prev_poc_msb = 0;
          prev_poc_lsb = top - std::min(top, bot);
        }
      }
      return std::min(top, bot);
    }
    // types 1 and 2: FrameNumOffset
    int64_t fno = idr ? 0 : prev_frame_num_offset + (prev_frame_num > h.frame_num ? max_frame_num : 0);
    int64_t top, bot;
    if (sps.poc_type == 2) {
      top = bot = idr ? 0 : 2 * (fno + h.frame_num) - (h.nal_ref_idc == 0);
    } else {
      int n = (int)sps.offset_for_ref_frame.size();
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
    return (int)std::min(top, bot);
  }

  // the reference lists of a slice (8.2.4.2, 8.2.4.3)
  void build_lists(const SliceHeader& h) {
    nlist[0] = nlist[1] = 0;
    int t = h.type();
    if (t == kI || t == kSI) return;
    std::vector<RPic*> st, lt;
    for (auto& p : dpb)
      if (p->ref) (p->lt ? lt : st).push_back(p.get());
    std::sort(lt.begin(), lt.end(), [](const RPic* a, const RPic* b) { return a->lt_idx < b->lt_idx; });
    std::vector<RPic*> l[2];
    if (t == kB) {
      std::vector<RPic*> before, after;
      for (RPic* p : st) (p->poc < cur->poc ? before : after).push_back(p);
      std::sort(before.begin(), before.end(), [](const RPic* a, const RPic* b) { return a->poc > b->poc; });
      std::sort(after.begin(), after.end(), [](const RPic* a, const RPic* b) { return a->poc < b->poc; });
      l[0] = before;
      l[0].insert(l[0].end(), after.begin(), after.end());
      l[0].insert(l[0].end(), lt.begin(), lt.end());
      l[1] = after;
      l[1].insert(l[1].end(), before.begin(), before.end());
      l[1].insert(l[1].end(), lt.begin(), lt.end());
      if (l[1].size() > 1 && l[1] == l[0]) std::swap(l[1][0], l[1][1]);
    } else {
      std::sort(st.begin(), st.end(), [this](const RPic* a, const RPic* b) { return pic_num(a) > pic_num(b); });
      l[0] = st;
      l[0].insert(l[0].end(), lt.begin(), lt.end());
    }
    for (int li = 0; li < (t == kB ? 2 : 1); li++) {
      int n = h.num_ref_idx_active[li];
      std::vector<RPic*>& L = l[li];
      L.resize(std::max((int)L.size(), n), nullptr);
      L.resize(n);
      // modifications
      int pred = curr_pic_num, ri = 0;
      for (auto& op : h.rplm[li]) {
        if (ri >= n) break;
        RPic* pic = nullptr;
        if (op.first < 2) {
          int d = (int)op.second + 1;
          int nw = op.first == 0 ? pred - d : pred + d;
          if (nw < 0) nw += max_frame_num;
          if (nw >= max_frame_num) nw -= max_frame_num;
          pred = nw;
          int pn = nw > curr_pic_num ? nw - max_frame_num : nw;
          for (RPic* p : st)
            if (pic_num(p) == pn) pic = p;
        } else if (op.first == 2) {
          for (RPic* p : lt)
            if (p->lt_idx == op.second) pic = p;
        }
        if (!pic) continue;
        L.insert(L.begin() + ri, pic);
        ri++;
        for (int k = ri; k < (int)L.size(); k++)
          if (L[k] == pic) {
            L.erase(L.begin() + k);
            break;
          }
        L.resize(n);
      }
      nlist[li] = n;
      for (int k = 0; k < n && k < 33; k++) list[li][k] = L[k];
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
      lwd = h.luma_log2_wd;
      cwd = h.chroma_log2_wd;
      for (int l = 0; l < 2; l++)
        for (int i = 0; i < h.num_ref_idx_active[l] && i < 32; i++)
          for (int k = 0; k < 6; k++) wt[l][i][k] = (size_t)(i * 6 + k) < h.wt[l].size() ? h.wt[l][i * 6 + k] : 0;
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
  int mb_addr = 0;

  void mb_start(int addr) {
    mb_addr = addr;
    res.m4 = 0;
    res.m8 = res.mdc = res.mcdc = res.mcac[0] = res.mcac[1] = 0;
  }
  void coef(int cat, int idx, const int* c, int n) {
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
      int ls = 16 * kNorm4[m][(x & 1) && (y & 1) ? 1 : !(x & 1) && !(y & 1) ? 0 : 2];
      d[r] = qp >= 24 ? (c[k] * ls) << (q6 - 4) : (c[k] * ls + (1 << (3 - q6))) >> (4 - q6);
    }
  }
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
      d[r] = qp >= 36 ? (c[k] * ls) << (q6 - 6) : (c[k] * ls + (1 << (5 - q6))) >> (6 - q6);
    }
  }

  // neighbouring macroblocks for intra prediction: decoded, in the same
  // slice, and not inter with constrained_intra_pred
  int nb_mb(int addr, int dx, int dy) const {
    int x = addr % cur->mbw + dx, y = addr / cur->mbw + dy;
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
    int dc = 128;
    if (mode == 2) {
      int st = 0, sl = 0;
      for (int i = 0; i < n; i++) st += T[i], sl += L[i];
      int sh = n == 4 ? 2 : 3;
      dc = top && left ? (st + sl + n) >> (sh + 1) : left ? (sl + n / 2) >> sh : top ? (st + n / 2) >> sh : 128;
    }
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) {
        int v;
        switch (mode) {
          case 0:
            v = T[x];
            break;
          case 1:
            v = L[y];
            break;
          case 2:
            v = dc;
            break;
          case 3:
            v = x == n - 1 && y == n - 1 ? (T[2 * n - 2] + 3 * T[2 * n - 1] + 2) >> 2
                                         : (T[x + y] + 2 * T[x + y + 1] + T[x + y + 2] + 2) >> 2;
            break;
          case 4:
            v = x > y ? (P(x - y - 2, -1) + 2 * P(x - y - 1, -1) + P(x - y, -1) + 2) >> 2
                : x < y ? (P(-1, y - x - 2) + 2 * P(-1, y - x - 1) + P(-1, y - x) + 2) >> 2
                        : (P(0, -1) + 2 * P(-1, -1) + P(-1, 0) + 2) >> 2;
            break;
          case 5: {
            int z = 2 * x - y;
            if (z >= 0 && !(z & 1))
              v = (P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 1) >> 1;
            else if (z >= 0)
              v = (P(x - (y >> 1) - 2, -1) + 2 * P(x - (y >> 1) - 1, -1) + P(x - (y >> 1), -1) + 2) >> 2;
            else if (z == -1)
              v = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
            else
              v = (P(-1, y - 2 * x - 1) + 2 * P(-1, y - 2 * x - 2) + P(-1, y - 2 * x - 3) + 2) >> 2;
            break;
          }
          case 6: {
            int z = 2 * y - x;
            if (z >= 0 && !(z & 1))
              v = (P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 1) >> 1;
            else if (z >= 0)
              v = (P(-1, y - (x >> 1) - 2) + 2 * P(-1, y - (x >> 1) - 1) + P(-1, y - (x >> 1)) + 2) >> 2;
            else if (z == -1)
              v = (P(-1, 0) + 2 * P(-1, -1) + P(0, -1) + 2) >> 2;
            else
              v = (P(x - 2 * y - 1, -1) + 2 * P(x - 2 * y - 2, -1) + P(x - 2 * y - 3, -1) + 2) >> 2;
            break;
          }
          case 7:
            v = !(y & 1) ? (T[x + (y >> 1)] + T[x + (y >> 1) + 1] + 1) >> 1
                         : (T[x + (y >> 1)] + 2 * T[x + (y >> 1) + 1] + T[x + (y >> 1) + 2] + 2) >> 2;
            break;
          default: {
            int z = x + 2 * y, lim = 2 * n - 3;
            if (z < lim && !(z & 1))
              v = (L[y + (x >> 1)] + L[y + (x >> 1) + 1] + 1) >> 1;
            else if (z < lim)
              v = (L[y + (x >> 1)] + 2 * L[y + (x >> 1) + 1] + L[y + (x >> 1) + 2] + 2) >> 2;
            else if (z == lim)
              v = (L[n - 2] + 3 * L[n - 1] + 2) >> 2;
            else
              v = L[n - 1];
          }
        }
        d[y * stride + x] = (uint8_t)v;
      }
  }

  // the edge of the luma n x n block at (bx, by) (in units of n) of the
  // macroblock at addr; n 4 or 8. Returns whether the top and left are there.
  void luma_edge(int addr, int n, int bx, int by, int* e, bool& top, bool& left) {
    int w = cur->w, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    int x0 = mx + bx * n, y0 = my + by * n;
    const uint8_t* Y = cur->y.data();
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
    e[0] = tl ? Y[(y0 - 1) * w + x0 - 1] : 128;
    for (int i = 0; i < n; i++) e[1 + i] = top ? Y[(y0 - 1) * w + x0 + i] : 128;
    for (int i = 0; i < n; i++) e[1 + n + i] = tr ? Y[(y0 - 1) * w + x0 + n + i] : e[n];
    for (int i = 0; i < n; i++) e[1 + 2 * n + i] = left ? Y[(y0 + i) * w + x0 - 1] : 128;
    if (n == 8) {
      // reference sample filtering (8.3.2.2.1)
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
  }

  void pred16(int addr, int mode) {
    int w = cur->w, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    uint8_t* Y = cur->y.data();
    uint8_t* d = Y + my * w + mx;
    bool aA = intra_avail(addr, nb_mb(addr, -1, 0)), aB = intra_avail(addr, nb_mb(addr, 0, -1));
    bool aD = intra_avail(addr, nb_mb(addr, -1, -1));
    int T[16], L[16], Q = aD ? d[-w - 1] : 128;
    for (int i = 0; i < 16; i++) {
      T[i] = aB ? d[-w + i] : 128;
      L[i] = aA ? d[i * w - 1] : 128;
    }
    if (mode == 3) {
      int H = 0, V = 0;
      for (int i = 0; i < 8; i++) {
        H += (i + 1) * (T[8 + i] - (i == 7 ? Q : T[6 - i]));
        V += (i + 1) * (L[8 + i] - (i == 7 ? Q : L[6 - i]));
      }
      int a = 16 * (L[15] + T[15]), b = (5 * H + 32) >> 6, c = (5 * V + 32) >> 6;
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) d[y * w + x] = (uint8_t)clip1((a + b * (x - 7) + c * (y - 7) + 16) >> 5);
      return;
    }
    int dc = 128;
    if (mode == 2) {
      int st = 0, sl = 0;
      for (int i = 0; i < 16; i++) st += T[i], sl += L[i];
      dc = aA && aB ? (st + sl + 16) >> 5 : aA ? (sl + 8) >> 4 : aB ? (st + 8) >> 4 : 128;
    }
    for (int y = 0; y < 16; y++)
      for (int x = 0; x < 16; x++) d[y * w + x] = (uint8_t)(mode == 0 ? T[x] : mode == 1 ? L[y] : dc);
  }

  void pred_chroma(int addr, int mode) {
    int cw = cur->w / 2, mx = addr % cur->mbw * 8, my = addr / cur->mbw * 8;
    bool aA = intra_avail(addr, nb_mb(addr, -1, 0)), aB = intra_avail(addr, nb_mb(addr, 0, -1));
    bool aD = intra_avail(addr, nb_mb(addr, -1, -1));
    for (int c = 0; c < 2; c++) {
      uint8_t* d = (c ? cur->v.data() : cur->u.data()) + my * cw + mx;
      int T[8], L[8], Q = aD ? d[-cw - 1] : 128;
      for (int i = 0; i < 8; i++) {
        T[i] = aB ? d[-cw + i] : 128;
        L[i] = aA ? d[i * cw - 1] : 128;
      }
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
            for (int x = 0; x < 4; x++) d[(yo + y) * cw + xo + x] = (uint8_t)v;
        }
      } else if (mode == 3) {
        int H = 0, V = 0;
        for (int i = 0; i < 4; i++) {
          H += (i + 1) * (T[4 + i] - (i == 3 ? Q : T[2 - i]));
          V += (i + 1) * (L[4 + i] - (i == 3 ? Q : L[2 - i]));
        }
        int a = 16 * (L[7] + T[7]), b = (34 * H + 32) >> 6, cc = (34 * V + 32) >> 6;
        for (int y = 0; y < 8; y++)
          for (int x = 0; x < 8; x++) d[y * cw + x] = (uint8_t)clip1((a + b * (x - 3) + cc * (y - 3) + 16) >> 5);
      } else {
        for (int y = 0; y < 8; y++)
          for (int x = 0; x < 8; x++) d[y * cw + x] = (uint8_t)(mode == 1 ? L[y] : T[x]);
      }
    }
  }

  // --- inter prediction (8.4.2.2): a block of w x h luma samples at (x, y)
  // of ref, displaced by mv (quarter samples), into o (stride 16)
  static void mc_luma(const RPic* ref, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
    int W = ref->w, H = ref->h;
    const uint8_t* R = ref->y.data();
    int xi = x + (mvx >> 2), yi = y + (mvy >> 2), fx = mvx & 3, fy = mvy & 3;
    // the samples from (xi - 2, yi - 2) on: in the picture, or copied with the
    // edges repeated
    uint8_t tmp[21 * 21];
    const uint8_t* g;
    int gs;
    if (xi >= 2 && yi >= 2 && xi + w + 3 <= W && yi + h + 3 <= H) {
      g = R + (yi - 2) * W + xi - 2;
      gs = W;
    } else {
      for (int j = 0; j < h + 5; j++) {
        const uint8_t* row = R + clip3(0, H - 1, yi - 2 + j) * W;
        for (int i = 0; i < w + 5; i++) tmp[j * 21 + i] = row[clip3(0, W - 1, xi - 2 + i)];
      }
      g = tmp;
      gs = 21;
    }
    g += 2 * gs + 2;  // at (xi, yi)
    if (!fx && !fy) {
      for (int j = 0; j < h; j++) memcpy(o + j * 16, g + j * gs, w);
      return;
    }
#define T6(p, s) ((p)[-2 * (s)] - 5 * (p)[-(s)] + 20 * (p)[0] + 20 * (p)[s] - 5 * (p)[2 * (s)] + (p)[3 * (s)])
    int b1[21][17], bb[17][17], hh[16][17], jj[16][16];
    bool need_j = fx == 2 || fy == 2;
    bool need_b = fx != 0 || need_j, need_h = fy != 0 && !(fx == 2 && fy == 2 && false);
    if (need_b) {
      int j0 = need_j ? -2 : 0, j1 = need_j ? h + 3 : h + 1;
      for (int j = j0; j < j1; j++)
        for (int i = 0; i < w + 1; i++) b1[j + 2][i] = T6(g + j * gs + i, 1);
      for (int j = 0; j < h + 1; j++)
        for (int i = 0; i < w + 1; i++) bb[j][i] = clip1((b1[j + 2][i] + 16) >> 5);
    }
    if (need_h)
      for (int j = 0; j < h; j++)
        for (int i = 0; i < w + 1; i++) hh[j][i] = clip1((T6(g + j * gs + i, gs) + 16) >> 5);
    if (need_j)
      for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
          jj[j][i] = clip1((b1[j][i] - 5 * b1[j + 1][i] + 20 * b1[j + 2][i] + 20 * b1[j + 3][i] - 5 * b1[j + 4][i] +
                            b1[j + 5][i] + 512) >> 10);
#undef T6
    int c = fy * 4 + fx;
    for (int j = 0; j < h; j++) {
      const uint8_t* G0 = g + j * gs;
      uint8_t* out = o + j * 16;
      for (int i = 0; i < w; i++) {
        int v;
        switch (c) {
          case 1: v = (G0[i] + bb[j][i] + 1) >> 1; break;
          case 2: v = bb[j][i]; break;
          case 3: v = (bb[j][i] + G0[i + 1] + 1) >> 1; break;
          case 4: v = (G0[i] + hh[j][i] + 1) >> 1; break;
          case 5: v = (bb[j][i] + hh[j][i] + 1) >> 1; break;
          case 6: v = (bb[j][i] + jj[j][i] + 1) >> 1; break;
          case 7: v = (bb[j][i] + hh[j][i + 1] + 1) >> 1; break;
          case 8: v = hh[j][i]; break;
          case 9: v = (hh[j][i] + jj[j][i] + 1) >> 1; break;
          case 10: v = jj[j][i]; break;
          case 11: v = (jj[j][i] + hh[j][i + 1] + 1) >> 1; break;
          case 12: v = (hh[j][i] + G0[i + gs] + 1) >> 1; break;
          case 13: v = (hh[j][i] + bb[j + 1][i] + 1) >> 1; break;
          case 14: v = (jj[j][i] + bb[j + 1][i] + 1) >> 1; break;
          default: v = (hh[j][i + 1] + bb[j + 1][i] + 1) >> 1; break;
        }
        out[i] = (uint8_t)v;
      }
    }
  }
  // chroma: w x h samples at (x, y) of plane c (0 Cb, 1 Cr), mv in eighths
  static void mc_chroma(const RPic* ref, int c, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
    int W = ref->w / 2, H = ref->h / 2;
    const uint8_t* R = c ? ref->v.data() : ref->u.data();
    int xi = x + (mvx >> 3), yi = y + (mvy >> 3), fx = mvx & 7, fy = mvy & 7;
    if (xi >= 0 && yi >= 0 && xi + w < W && yi + h < H) {
      const uint8_t* r = R + yi * W + xi;
      if (!fx && !fy) {
        for (int j = 0; j < h; j++) memcpy(o + j * 8, r + j * W, w);
        return;
      }
      int cA = (8 - fx) * (8 - fy), cB = fx * (8 - fy), cC = (8 - fx) * fy, cD = fx * fy;
      for (int j = 0; j < h; j++, r += W)
        for (int i = 0; i < w; i++)
          o[j * 8 + i] = (uint8_t)((cA * r[i] + cB * r[i + 1] + cC * r[i + W] + cD * r[i + W + 1] + 32) >> 6);
      return;
    }
    for (int j = 0; j < h; j++) {
      int y0 = clip3(0, H - 1, yi + j), y1 = clip3(0, H - 1, yi + j + 1);
      for (int i = 0; i < w; i++) {
        int x0 = clip3(0, W - 1, xi + i), x1 = clip3(0, W - 1, xi + i + 1);
        int A = R[y0 * W + x0], B = R[y0 * W + x1], C = R[y1 * W + x0], D = R[y1 * W + x1];
        o[j * 8 + i] = (uint8_t)(((8 - fx) * (8 - fy) * A + fx * (8 - fy) * B + (8 - fx) * fy * C + fx * fy * D + 32) >> 6);
      }
    }
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

  // A macroblock, coded or skipped: cls as the model's classes, modes the
  // Intra4x4/8x8 modes (raster), rf/mv the motion (refIdx -1 none), nz the
  // walk's coefficient counts.
  void mb_end(int addr, int cls, int i16m, const uint8_t* modes, int cpred, int t8, int qp, const int8_t rf[2][4],
              const int16_t mv[2][16][2], const uint8_t nz[16]) {
    if (!active) return;
    int w = cur->w, cw = w / 2, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    uint8_t* Y = cur->y.data() + my * w + mx;
    uint8_t* U = cur->u.data() + my / 2 * cw + mx / 2;
    uint8_t* V = cur->v.data() + my / 2 * cw + mx / 2;
    bool intra = cls >= 6, pcm = cls == 8;
    DbMb& D = db[addr];
    D.slice = slice_no;
    D.intra = intra;
    D.pcm = pcm;
    D.t8 = (uint8_t)t8;
    D.qp = (uint8_t)qp;
    D.disable = (uint8_t)disable;
    D.alpha_off = (int8_t)alpha_off;
    D.beta_off = (int8_t)beta_off;
    D.cqp[0] = (int8_t)cqp_off[0];
    D.cqp[1] = (int8_t)cqp_off[1];
    int qpc[2] = {chroma_qp(qp, cqp_off[0]), chroma_qp(qp, cqp_off[1])};
    if (pcm) {
      for (int i = 0; i < 256; i++) Y[(i >> 4) * w + (i & 15)] = (uint8_t)pcm_s[i];
      for (int i = 0; i < 64; i++) U[(i >> 3) * cw + (i & 7)] = (uint8_t)pcm_s[256 + i];
      for (int i = 0; i < 64; i++) V[(i >> 3) * cw + (i & 7)] = (uint8_t)pcm_s[320 + i];
    } else if (intra) {
      if (cls == 7) {
        pred16(addr, i16m);
        int dcr[16] = {};
        if (res.mdc) {
          // inverse Hadamard of the DC (8.5.10), then their scaling
          int c[16], f[16];
          for (int k = 0; k < 16; k++) c[kZz4[k]] = res.dc[k];
          for (int i = 0; i < 4; i++) {
            int a = c[i * 4], b = c[i * 4 + 1], cc = c[i * 4 + 2], d = c[i * 4 + 3];
            f[i * 4] = a + b + cc + d;
            f[i * 4 + 1] = a + b - cc - d;
            f[i * 4 + 2] = a - b - cc + d;
            f[i * 4 + 3] = a - b + cc - d;
          }
          for (int j = 0; j < 4; j++) {
            int a = f[j], b = f[4 + j], cc = f[8 + j], d = f[12 + j];
            c[j] = a + b + cc + d;
            c[4 + j] = a + b - cc - d;
            c[8 + j] = a - b - cc + d;
            c[12 + j] = a - b + cc - d;
          }
          int ls = 16 * kNorm4[qp % 6][0], q6 = qp / 6;
          for (int i = 0; i < 16; i++) dcr[i] = qp >= 36 ? (c[i] * ls) << (q6 - 6) : (c[i] * ls + (1 << (5 - q6))) >> (6 - q6);
        }
        for (int b = 0; b < 16; b++) {
          int bx = blk_x(b), by = blk_y(b);
          add4(Y + by * 4 * w + bx * 4, w, b, qp, &dcr[by * 4 + bx]);
        }
      } else if (t8) {
        for (int b8 = 0; b8 < 4; b8++) {
          int bx = b8 & 1, by = b8 >> 1, e[25];
          bool top, left;
          luma_edge(addr, 8, bx, by, e, top, left);
          uint8_t* p = Y + by * 8 * w + bx * 8;
          pred_nxn(p, w, 8, modes[by * 8 + bx * 2], e, top, left);
          if (res.m8 >> b8 & 1) {
            int d[64];
            dequant8(res.y8[b8], d, qp);
            idct8_add(p, w, d);
          }
        }
      } else {
        for (int b = 0; b < 16; b++) {
          int bx = blk_x(b), by = blk_y(b), e[17];
          bool top, left;
          luma_edge(addr, 4, bx, by, e, top, left);
          uint8_t* p = Y + by * 4 * w + bx * 4;
          pred_nxn(p, w, 4, modes[by * 4 + bx], e, top, left);
          add4(p, w, b, qp, nullptr);
        }
      }
      pred_chroma(addr, cpred);
    } else {
      inter(addr, rf, mv, Y, U, V);
      if (t8) {
        for (int b8 = 0; b8 < 4; b8++)
          if (res.m8 >> b8 & 1) {
            int d[64];
            dequant8(res.y8[b8], d, qp);
            idct8_add(Y + (b8 >> 1) * 8 * w + (b8 & 1) * 8, w, d);
          }
      } else {
        for (int b = 0; b < 16; b++) add4(Y + blk_y(b) * 4 * w + blk_x(b) * 4, w, b, qp, nullptr);
      }
    }
    if (!pcm) {
      // chroma residual: the DC of each component (2x2 Hadamard), then the blocks
      for (int c = 0; c < 2; c++) {
        uint8_t* P = c ? V : U;
        int dcc[4] = {0, 0, 0, 0};
        if (res.mcdc >> c & 1) {
          int* x = res.cdc[c];
          int f0 = x[0] + x[1] + x[2] + x[3], f1 = x[0] - x[1] + x[2] - x[3];
          int f2 = x[0] + x[1] - x[2] - x[3], f3 = x[0] - x[1] - x[2] + x[3];
          int ls = 16 * kNorm4[qpc[c] % 6][0], q6 = qpc[c] / 6;
          dcc[0] = ((f0 * ls) << q6) >> 5;
          dcc[1] = ((f1 * ls) << q6) >> 5;
          dcc[2] = ((f2 * ls) << q6) >> 5;
          dcc[3] = ((f3 * ls) << q6) >> 5;
        }
        for (int b = 0; b < 4; b++) {
          if (!(res.mcac[c] >> b & 1) && !dcc[b]) continue;
          int d[16];
          if (res.mcac[c] >> b & 1)
            dequant4(res.cac[c][b], d, qpc[c], 1);
          else
            for (int i = 0; i < 16; i++) d[i] = 0;
          d[0] = dcc[b];
          idct4_add(P + (b >> 1) * 4 * cw + (b & 1) * 4, cw, d);
        }
      }
    }
    // for deblocking, and for the direct prediction of later pictures
    for (int b = 0; b < 16; b++) D.nz[b] = nz[b] != 0;
    if (t8 && !pcm)
      for (int b8 = 0; b8 < 4; b8++) {
        int i = (b8 >> 1) * 8 + (b8 & 1) * 2;
        uint8_t any = D.nz[i] | D.nz[i + 1] | D.nz[i + 4] | D.nz[i + 5];
        D.nz[i] = D.nz[i + 1] = D.nz[i + 4] = D.nz[i + 5] = any;
      }
    cur->intra[addr] = intra;
    for (int l = 0; l < 2; l++) {
      for (int b8 = 0; b8 < 4; b8++) {
        int r = intra ? -1 : rf[l][b8];
        const RPic* rp = r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
        D.rp[l][b8] = rp ? rp->id : -1;
        cur->rf[(addr * 4 + b8) * 2 + l] = (int8_t)(rp ? r : -1);
        cur->rpid[(addr * 4 + b8) * 2 + l] = rp ? rp->id : -1;
      }
      for (int b = 0; b < 16; b++)
        for (int k = 0; k < 2; k++) {
          int v = intra ? 0 : mv[l][b][k];
          D.mv[l][b][k] = (int16_t)v;
          cur->mv[((addr * 16 + b) * 2 + l) * 2 + k] = (int16_t)v;
        }
    }
  }

  // inter prediction of a macroblock (8.4.2), with the weights of 8.4.2.3
  void inter(int addr, const int8_t rf[2][4], const int16_t mv[2][16][2], uint8_t* Y, uint8_t* U, uint8_t* V) {
    int w = cur->w, cw = w / 2, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    uint8_t pl[2][256], pc[2][2][64];
    for (int l = 0; l < 2; l++) {
      bool any = false;
      for (int b8 = 0; b8 < 4; b8++) any |= rf[l][b8] >= 0;
      if (!any) continue;
      // in pieces of equal motion: the macroblock, 8x8 blocks, 4x4 blocks
      bool whole = true;
      for (int b8 = 1; b8 < 4; b8++) whole &= rf[l][b8] == rf[l][0];
      for (int b = 1; b < 16; b++) whole &= mv[l][b][0] == mv[l][0][0] && mv[l][b][1] == mv[l][0][1];
      for (int b8 = 0; b8 < 4; b8++) {
        if (whole && b8) break;
        int r = rf[l][b8];
        if (r < 0) continue;
        const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
        if (!ref) ref = cur.get();  // a missing reference: whatever is there
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
    // weighting, per 8x8 block
    for (int b8 = 0; b8 < 4; b8++) {
      int r0 = rf[0][b8], r1 = rf[1][b8];
      int x0 = (b8 & 1) * 8, y0 = (b8 >> 1) * 8;
      for (int comp = 0; comp < 3; comp++) {
        int n = comp ? 4 : 8, xs = comp ? x0 / 2 : x0, ys = comp ? y0 / 2 : y0;
        int ds = comp ? cw : w, ss = comp ? 8 : 16;
        uint8_t* dst = (comp == 0 ? Y : comp == 1 ? U : V) + ys * ds + xs;
        const uint8_t* a = (comp ? pc[0][comp - 1] : pl[0]) + ys * ss + xs;
        const uint8_t* b = (comp ? pc[1][comp - 1] : pl[1]) + ys * ss + xs;
        int k = comp ? 2 * comp : 0, lw = comp ? cwd : lwd;
        if (r0 >= 0 && r1 >= 0) {
          if (wmode == 1) {
            int w0 = wt[0][r0 & 31][k], o0 = wt[0][r0 & 31][k + 1], w1 = wt[1][r1 & 31][k], o1 = wt[1][r1 & 31][k + 1];
            int o = (o0 + o1 + 1) >> 1;
            for (int y = 0; y < n; y++)
              for (int x = 0; x < n; x++)
                dst[y * ds + x] = (uint8_t)clip1(((a[y * ss + x] * w0 + b[y * ss + x] * w1 + (1 << lw)) >> (lw + 1)) + o);
          } else if (wmode == 2) {
            int w0 = implicit_w[r0 & 31][r1 & 31], w1 = 64 - w0;
            for (int y = 0; y < n; y++)
              for (int x = 0; x < n; x++) dst[y * ds + x] = (uint8_t)clip1((a[y * ss + x] * w0 + b[y * ss + x] * w1 + 32) >> 6);
          } else {
            for (int y = 0; y < n; y++)
              for (int x = 0; x < n; x++) dst[y * ds + x] = (uint8_t)((a[y * ss + x] + b[y * ss + x] + 1) >> 1);
          }
        } else if (r0 >= 0 || r1 >= 0) {
          int l = r0 >= 0 ? 0 : 1, r = l ? r1 : r0;
          const uint8_t* p = l ? b : a;
          if (wmode == 1) {
            int ww = wt[l][r & 31][k], o = wt[l][r & 31][k + 1];
            for (int y = 0; y < n; y++)
              for (int x = 0; x < n; x++) {
                int v = p[y * ss + x];
                dst[y * ds + x] = (uint8_t)(lw >= 1 ? clip1(((v * ww + (1 << (lw - 1))) >> lw) + o) : clip1(v * ww + o));
              }
          } else {
            for (int y = 0; y < n; y++) memcpy(dst + y * ds, p + y * ss, n);
          }
        }
      }
    }
  }

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
    auto far = [](const int16_t* a, const int16_t* b) { return abs(a[0] - b[0]) >= 4 || abs(a[1] - b[1]) >= 4; };
    if (np == 1) {
      int lp = p0 >= 0 ? 0 : 1, lq = q0 >= 0 ? 0 : 1;
      if ((lp ? p1 : p0) != (lq ? q1 : q0)) return 1;
      return far(P.mv[lp][bp], Q.mv[lq][bq]);
    }
    // two vectors each: the same two pictures?
    if (!((p0 == q0 && p1 == q1) || (p0 == q1 && p1 == q0))) return 1;
    if (p0 != p1) {
      if (p0 == q0) return far(P.mv[0][bp], Q.mv[0][bq]) || far(P.mv[1][bp], Q.mv[1][bq]);
      return far(P.mv[0][bp], Q.mv[1][bq]) || far(P.mv[1][bp], Q.mv[0][bq]);
    }
    // both vectors of each refer to the same picture
    return (far(P.mv[0][bp], Q.mv[0][bq]) || far(P.mv[1][bp], Q.mv[1][bq])) &&
           (far(P.mv[0][bp], Q.mv[1][bq]) || far(P.mv[1][bp], Q.mv[0][bq]));
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
          int d = clip3(-tc, tc, (((q0 - p0) << 2) + (p1 - q1) + 4) >> 3);
          p[-s] = (uint8_t)clip1(p0 + d);
          p[0] = (uint8_t)clip1(q0 - d);
        } else {
          int p2 = p[-3 * s], q2 = p[2 * s];
          int ap = abs(p2 - p0), aq = abs(q2 - q0);
          int tc = tc0 + (ap < beta) + (aq < beta);
          int d = clip3(-tc, tc, (((q0 - p0) << 2) + (p1 - q1) + 4) >> 3);
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
  void deblock() {
    int mbw = cur->mbw, mbh = cur->mbh, w = cur->w, cw = w / 2;
    for (int addr = 0; addr < mbw * mbh; addr++) {
      const DbMb& Q = db[addr];
      if (Q.slice < 0 || Q.disable == 1) continue;
      int mx = addr % mbw, my = addr / mbw;
      for (int dir = 0; dir < 2; dir++) {  // vertical edges, then horizontal ones
        int na = dir ? (my ? addr - mbw : -1) : (mx ? addr - 1 : -1);
        bool mb_edge_ok = na >= 0 && db[na].slice >= 0 && (Q.disable == 0 || db[na].slice == Q.slice);
        for (int e = 0; e < 4; e++) {
          if (e == 0 && !mb_edge_ok) continue;
          if (e & 1 && Q.t8) continue;
          int pa = e ? addr : na;
          int b[4];
          bool any = false;
          for (int k = 0; k < 4; k++) {
            int bq = dir ? e * 4 + k : k * 4 + e;
            int bp = e ? (dir ? bq - 4 : bq - 1) : (dir ? 12 + k : k * 4 + 3);
            b[k] = bs(pa, addr, bp, bq, e == 0);
            any |= b[k] != 0;
          }
          if (!any) continue;
          const DbMb& P = db[pa];
          int qpp = P.pcm ? 0 : P.qp, qpq = Q.pcm ? 0 : Q.qp;
          uint8_t* Yq = cur->y.data() + (my * 16 + (dir ? e * 4 : 0)) * w + mx * 16 + (dir ? 0 : e * 4);
          for (int k = 0; k < 4; k++)
            if (b[k])
              filter(Yq + (dir ? k * 4 : k * 4 * w), dir ? w : 1, dir ? 1 : w, 4, b[k], (qpp + qpq + 1) >> 1,
                     Q.alpha_off, Q.beta_off, false);
          if (e & 1) continue;  // chroma edges at luma edges 0 and 8
          for (int c = 0; c < 2; c++) {
            int cp = chroma_qp(qpp, Q.cqp[c]), cq = chroma_qp(qpq, Q.cqp[c]);
            uint8_t* Cq = (c ? cur->v.data() : cur->u.data()) + (my * 8 + (dir ? e * 2 : 0)) * cw + mx * 8 + (dir ? 0 : e * 2);
            for (int k = 0; k < 4; k++)
              if (b[k])
                filter(Cq + (dir ? k * 2 : k * 2 * cw), dir ? cw : 1, dir ? 1 : cw, 2, b[k], (cp + cq + 1) >> 1,
                       Q.alpha_off, Q.beta_off, true);
          }
        }
      }
    }
  }

  // --- a slice begins
  static bool supported(const Sps& sps, const Pps& pps) {
    return sps.chroma_array_type() == 1 && sps.bit_depth_luma == 8 && sps.bit_depth_chroma == 8 &&
           sps.frame_mbs_only_flag && !sps.scaling_matrix && !pps.scaling_matrix && !sps.transform_bypass &&
           (int64_t)sps.width_mbs * sps.frame_height_mbs() <= 36864;  // 4096 x 2304
  }
  void slice_begin(const SliceHeader& h) {
    const Sps& sps = *h.sps;
    const Pps& pps = *h.pps;
    if (h.first_mb == 0 || !cur) {
      finish_picture();
      if (!supported(sps, pps)) {
        dpb.clear();
        return;
      }
      max_frame_num = 1 << sps.log2_max_frame_num;
      max_num_ref = sps.max_num_ref_frames;
      if (!spare.empty()) {
        cur = std::move(spare.back());
        spare.pop_back();
      } else {
        cur.reset(new RPic);
      }
      if (cur->mbw != sps.width_mbs || cur->mbh != sps.frame_height_mbs()) cur->alloc(sps.width_mbs, sps.frame_height_mbs());
      cur->ref = cur->lt = false;
      cur->lt_idx = -1;
      cur->id = next_id++;
      cur->frame_num = h.frame_num;
      cur->poc = poc(h);
      cur_idr = h.nal_unit_type == 5;
      cur_ref_idc = h.nal_ref_idc;
      cur_lt_flag = h.long_term_reference_flag;
      cur_mmco5 = h.mmco5;
      cur_mmco = h.mmco;
      DbMb none;
      memset(&none, 0, sizeof none);
      none.slice = -1;
      db.assign((size_t)cur->mbw * cur->mbh, none);
      slice_no = 0;
      active = true;
      // references of another size are no use
      std::vector<std::unique_ptr<RPic>> keep;
      for (auto& p : dpb)
        if (p->mbw == cur->mbw && p->mbh == cur->mbh) keep.push_back(std::move(p));
      dpb.swap(keep);
    } else {
      slice_no++;
    }
    if (!active) return;
    curr_pic_num = h.frame_num;
    stype = h.type();
    disable = h.disable_deblocking;
    alpha_off = h.alpha_offset;
    beta_off = h.beta_offset;
    cqp_off[0] = pps.chroma_qp_index_offset;
    cqp_off[1] = pps.second_chroma_qp_index_offset;
    constrained_intra = pps.constrained_intra_pred_flag;
    direct_spatial = h.direct_spatial_mv_pred_flag;
    direct8x8 = sps.direct_8x8_inference_flag;
    build_lists(h);
    weights(h, pps);
    for (int l = 0; l < 2; l++)
      for (int i = 0; i < 33; i++) cur->lt_ref[l][i] = i < nlist[l] && list[l][i] && list[l][i]->lt;
  }

  // the colocated block of the 4x4 block blk (raster) of macroblock addr,
  // for direct prediction (8.4.1.2.1): false when the first picture of list
  // 1 is not there or the block is intra; else its refIdx and vector, and
  // the picture it refers to
  bool colocated(int addr, int blk, int& ref, int* mv, int64_t& pid) const {
    const RPic* col = active && nlist[1] ? list[1][0] : nullptr;
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
  bool col_long_term() const { return nlist[1] && list[1][0] && list[1][0]->lt; }
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

  // The edge directions next to a block, as decoder-side intra mode
  // derivation (DIMD) finds them: Sobel gradients along the second row above
  // the macroblock (at the block's columns) and the second column to its
  // left (at the block's rows), where those macroblocks are decoded, each
  // voting with |Gx| + |Gy| for the Intra4x4 mode whose direction is closest
  // to the edge. Returns the strongest direction (0-8, not 2), 9 without
  // any; strength gets the sum of the votes.
  int dimd(int addr, int x0, int y0, int n, int& strength) const {
    strength = 0;
    if (!active) return 9;
    int mbw = cur->mbw, w = cur->w, mx = addr % mbw, my = addr / mbw;
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
      for (int x = std::max(x0, 1); x < std::min(x0 + n, 15); x++) vote(mx * 16 + x, my * 16 - 2);
    if (left >= 0 && db[left].slice >= 0)
      for (int y = std::max(y0, 1); y < std::min(y0 + n, 15); y++) vote(mx * 16 - 2, my * 16 + y);
    if (!strength) return 9;
    int best = 0;
    for (int k = 1; k < 9; k++)
      if (hist[k] > hist[best]) best = k;
    return best;
  }
};

}  // namespace h264p
