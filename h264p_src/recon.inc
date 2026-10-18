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
    prx_ok = false;
    res.m4 = 0;
    res.m8 = res.mdc = res.mcdc = res.mcac[0] = res.mcac[1] = 0;
    mb_prepared = mb_dc = false;
    skip_in = false;
    mb_done4 = 0;
    mb_done8 = 0;
    mb_cdone[0] = mb_cdone[1] = 0;
    mb_cdc[0] = mb_cdc[1] = false;
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
  void luma_edge(int addr, int n, int bx, int by, int* e, bool& top, bool& left) const {
    int w = cur->w;
    const uint8_t* Y = cur->y.data();
    edge_of(addr, n, bx, by, e, top, left, [&](int x, int y) { return (int)Y[y * w + x]; });
  }
  // the same from samples at(x, y) (picture coordinates)
  template <class F>
  void edge_of(int addr, int n, int bx, int by, int* e, bool& top, bool& left, F at) const {
    int mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
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
    pred_l16(d, w, mode, T, L, Q, aA, aB);
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
  // The Intra16x16 mode (0 vertical, 1 horizontal, 2 DC, 3 plane) that
  // predicts the macroblock best in the proxy picture displaced by (dx, dy)
  // whole samples, from that picture's samples around it, by SATD; costs
  // gets each mode's (INT_MAX not possible); -1 no picture
  int proxy_i16mode(int dx, int dy, int* costs) const {
    if (!active) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int w = P->w;
    int x0 = clip3(0, w - 16, mb_addr % cur->mbw * 16 + dx), y0 = clip3(0, P->h - 16, mb_addr / cur->mbw * 16 + dy);
    bool aA = x0 > 0, aB = y0 > 0;
    const uint8_t* s = P->y.data() + y0 * w + x0;
    int T[16], L[16], Q = aA && aB ? s[-w - 1] : 128;
    for (int i = 0; i < 16; i++) {
      T[i] = aB ? s[-w + i] : 128;
      L[i] = aA ? s[i * w - 1] : 128;
    }
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
  // x264's SATD of an n x n block (n a multiple of 4) against a prediction
  // (strides sa, sb), by 4x4, halved
  static int satd_nxn(const uint8_t* a, int sa, const uint8_t* b, int sb, int n) {
    int s = 0;
    for (int by4 = 0; by4 < n; by4 += 4)
      for (int bx4 = 0; bx4 < n; bx4 += 4) {
        int t[16];
        for (int y = 0; y < 4; y++)
          for (int x = 0; x < 4; x++) t[y * 4 + x] = a[(by4 + y) * sa + bx4 + x] - b[(by4 + y) * sb + bx4 + x];
        for (int y = 0; y < 4; y++) {
          int* r = t + y * 4;
          int a0 = r[0] + r[1], a1 = r[0] - r[1], a2 = r[2] + r[3], a3 = r[2] - r[3];
          r[0] = a0 + a2, r[1] = a1 + a3, r[2] = a0 - a2, r[3] = a1 - a3;
        }
        for (int x = 0; x < 4; x++) {
          int a0 = t[x] + t[4 + x], a1 = t[x] - t[4 + x], a2 = t[8 + x] + t[12 + x], a3 = t[8 + x] - t[12 + x];
          s += abs(a0 + a2) + abs(a1 + a3) + abs(a0 - a2) + abs(a1 - a3);
        }
      }
    return (s + 1) >> 1;
  }
  // The chroma mode (0 DC, 1 horizontal, 2 vertical, 3 plane) that predicts
  // the chroma of the macroblock best in the proxy picture displaced by
  // (dx, dy) whole luma samples, from that picture's samples around it,
  // by SATD over both components; costs gets each mode's; -1 no picture
  int proxy_cmode(int dx, int dy, int* costs) const {
    if (!active) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int cw = P->w / 2, ch = P->h / 2;
    int x0 = clip3(0, cw - 8, mb_addr % cur->mbw * 8 + div2(dx)), y0 = clip3(0, ch - 8, mb_addr / cur->mbw * 8 + div2(dy));
    bool aA = x0 > 0, aB = y0 > 0;
    for (int m = 0; m < 4; m++) costs[m] = 0;
    for (int c = 0; c < 2; c++) {
      const uint8_t* s = (c ? P->v.data() : P->u.data()) + y0 * cw + x0;
      int T[8], L[8], Q = aA && aB ? s[-cw - 1] : 128;
      for (int i = 0; i < 8; i++) {
        T[i] = aB ? s[-cw + i] : 128;
        L[i] = aA ? s[i * cw - 1] : 128;
      }
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
        for (int i = 0; i < w; i++) b1[j + 2][i] = T6(g + j * gs + i, 1);
      for (int j = 0; j < h + 1; j++)
        for (int i = 0; i < w; i++) bb[j][i] = clip1((b1[j + 2][i] + 16) >> 5);
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

  // The macroblock being reconstructed: what its prediction needs, from
  // prepare() on, and which of its luma blocks are done.
  int mb_cls = 0, mb_i16m = 0, mb_cpred = 0, mb_t8 = 0;
  const uint8_t* mb_modes = nullptr;
  int8_t mb_rf[2][4];
  int16_t mb_mv[2][16][2];
  bool mb_prepared = false, mb_dc = false;
  // skip_miss put the prediction by this motion in the picture
  bool skip_in = false;
  int8_t skip_rf[2][4];
  int16_t skip_mv[2][16][2];
  uint16_t mb_done4 = 0;  // 4x4 blocks (luma4x4BlkIdx) done
  uint8_t mb_done8 = 0;
  int mb_dcr[16];         // Intra16x16 DC, scaled (raster)
  uint8_t mb_cdone[2];    // chroma blocks done, per component
  bool mb_cdc[2];         // their DC scaled
  int mb_dcc[2][4];

  uint8_t* luma_at(int x, int y) const {
    return cur->y.data() + ((mb_addr / cur->mbw) * 16 + y) * cur->w + (mb_addr % cur->mbw) * 16 + x;
  }

  // The prediction of the macroblock, as soon as its type, intra modes and
  // motion are known: inter and Intra16x16 luma and the chroma go to the
  // picture now, Intra4x4/8x8 luma block by block (luma_ready).
  void prepare(int cls, int i16m, const uint8_t* modes, int cpred, int t8, const int8_t rf[2][4],
               const int16_t mv[2][16][2]) {
    if (!active || mb_prepared) return;
    mb_prepared = true;
    mb_cls = cls;
    mb_i16m = i16m;
    mb_modes = modes;
    mb_cpred = cpred;
    mb_t8 = t8;
    memcpy(mb_rf, rf, sizeof mb_rf);
    memcpy(mb_mv, mv, sizeof mb_mv);
    int addr = mb_addr, w = cur->w, cw = w / 2, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    DbMb& D = db[addr];
    D.slice = slice_no;
    D.intra = cls >= 6;
    if (cls == 8) return;
    if (cls < 6) {
      if (!skip_in || memcmp(skip_rf, mb_rf, sizeof skip_rf) || memcmp(skip_mv, mb_mv, sizeof skip_mv))
        inter(addr, mb_rf, mb_mv, cur->y.data() + my * w + mx, cur->u.data() + my / 2 * cw + mx / 2,
              cur->v.data() + my / 2 * cw + mx / 2);
      return;
    }
    if (cls == 7) pred16(addr, i16m);
    pred_chroma(addr, cpred);
  }

  // an Intra4x4 (Intra8x8: blk the 8x8 block) block about to be coded: the
  // ones before it reconstructed, its prediction in the picture
  void luma_ready(int blk, int qp) {
    if (!active || mb_cls != 6) return;
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
    pred_nxn(luma_at(bx * 4, by * 4), cur->w, 4, mb_modes[by * 4 + bx], e, top, left);
  }
  void intra8(int b8) {
    int bx = b8 & 1, by = b8 >> 1, e[25];
    bool top, left;
    luma_edge(mb_addr, 8, bx, by, e, top, left);
    pred_nxn(luma_at(bx * 8, by * 8), cur->w, 8, mb_modes[by * 8 + bx * 2], e, top, left);
  }
  // the residual of a luma block, its prediction being in the picture
  void add4b(int b, int qp) {
    if (mb_cls == 7 && !mb_dc) dc16(qp);
    add4(luma_at(blk_x(b) * 4, blk_y(b) * 4), cur->w, b, qp, mb_cls == 7 ? &mb_dcr[blk_y(b) * 4 + blk_x(b)] : nullptr);
    mb_done4 |= (uint16_t)(1 << b);
  }
  void add8(int b8, int qp) {
    if (res.m8 >> b8 & 1) {
      int d[64];
      dequant8(res.y8[b8], d, qp);
      idct8_add(luma_at((b8 & 1) * 8, (b8 >> 1) * 8), cur->w, d);
    }
    mb_done8 |= (uint8_t)(1 << b8);
  }
  // a luma block's coefficients are known (cat as the walk's, blk its index)
  void luma_add(int cat, int blk, int qp) {
    if (!active || !mb_prepared || mb_cls == 8) return;
    if (cat == 1 || cat == 2) {
      if (!mb_t8 && !(mb_done4 >> blk & 1)) add4b(blk, qp);
    } else if (cat == 5) {
      if (mb_t8 && !(mb_done8 >> blk & 1)) add8(blk, qp);
    }
  }
  // the luma of the macroblock, all of it (its coefficients coded; t8
  // known by then, where mb_t8 may be from before)
  void luma_finish(int qp, int t8) {
    if (!active || !mb_prepared || mb_cls == 8) return;
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
    int* x = res.cdc[c];
    int f[4] = {x[0] + x[1] + x[2] + x[3], x[0] - x[1] + x[2] - x[3], x[0] + x[1] - x[2] - x[3], x[0] - x[1] - x[2] + x[3]};
    int ls = 16 * kNorm4[qpc % 6][0], q6 = qpc / 6;
    for (int b = 0; b < 4; b++) mb_dcc[c][b] = ((f[b] * ls) << q6) >> 5;
  }
  int chroma_qp_of(int c, int qp) const { return chroma_qp(qp, cqp_off[c]); }
  uint8_t* chroma_at(int c, int x, int y) const {
    int cw = cur->w / 2;
    return (c ? cur->v.data() : cur->u.data()) + ((mb_addr / cur->mbw) * 8 + y) * cw + (mb_addr % cur->mbw) * 8 + x;
  }
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
    idct4_add(chroma_at(c, (b & 1) * 4, (b >> 1) * 4), cur->w / 2, d);
  }
  // a chroma AC block about to be coded: the ones before it done
  void chroma_ready(int c, int b, int qp) {
    if (!active || !mb_prepared || mb_cls == 8) return;
    for (int k = 0; k < b; k++)
      if (!(mb_cdone[c] >> k & 1)) addc(c, k, qp);
  }
  void chroma_add(int c, int b, int qp) {
    if (!active || !mb_prepared || mb_cls == 8) return;
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
    for (int i = 0; i < 16; i++) mb_dcr[i] = qp >= 36 ? (c[i] * ls) << (q6 - 6) : (c[i] * ls + (1 << (5 - q6))) >> (6 - q6);
  }

  // The macroblock is coded or skipped: what is left of it (the luma blocks
  // without coefficients, the chroma residual, I_PCM samples), and what
  // deblocking and the direct prediction of later pictures need. nz: the
  // walk's coefficient counts.
  void mb_end(int addr, int cls, int i16m, const uint8_t* modes, int cpred, int t8, int qp, const int8_t rf[2][4],
              const int16_t mv[2][16][2], const uint8_t nz[16]) {
    if (!active) return;
    prepare(cls, i16m, modes, cpred, t8, rf, mv);
    int w = cur->w, cw = w / 2, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    uint8_t* Y = cur->y.data() + my * w + mx;
    uint8_t* U = cur->u.data() + my / 2 * cw + mx / 2;
    uint8_t* V = cur->v.data() + my / 2 * cw + mx / 2;
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

  // The texture of the prediction of a block of n x n samples at (x, y) of
  // the current macroblock in plane c (0 luma): the sum of the absolute
  // differences between neighbouring samples; -1 not known.
  // x264's AQ energy (ac_energy_mb) of macroblock addr of the current
  // picture: the luma 16x16 and the two chroma 8x8 variances, summed
  int64_t ac_energy(int addr) const {
    int w = cur->w, cw = w / 2, mx = addr % cur->mbw * 16, my = addr / cur->mbw * 16;
    int64_t e = 0;
    {
      const uint8_t* p = cur->y.data() + my * w + mx;
      int64_t s1 = 0, s2 = 0;
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) s1 += p[y * w + x], s2 += p[y * w + x] * p[y * w + x];
      e += s2 - (s1 * s1 >> 8);
    }
    for (int c = 0; c < 2; c++) {
      const uint8_t* p = (c ? cur->v.data() : cur->u.data()) + my / 2 * cw + mx / 2;
      int64_t s1 = 0, s2 = 0;
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) s1 += p[y * cw + x], s2 += p[y * cw + x] * p[y * cw + x];
      e += s2 - (s1 * s1 >> 6);
    }
    return e;
  }
  // the same of the proxy of the current macroblock (proxy_build); -1 none
  int64_t proxy_energy() const {
    if (!active || !prx_ok) return -1;
    int64_t e = 0, s1 = 0, s2 = 0;
    for (int i = 0; i < 256; i++) s1 += prx_y[i], s2 += prx_y[i] * prx_y[i];
    e += s2 - (s1 * s1 >> 8);
    for (int c = 0; c < 2; c++) {
      s1 = s2 = 0;
      for (int i = 0; i < 64; i++) s1 += prx_c[c][i], s2 += prx_c[c][i] * prx_c[c][i];
      e += s2 - (s1 * s1 >> 6);
    }
    return e;
  }
  // The prediction of the current macroblock were it skipped (rf, mv: the
  // motion of P_Skip or B_Skip), put in the picture where its own will go:
  // how well its luma continues the decoded samples above and to the left,
  // as edge_miss
  int skip_miss(const int8_t rf[2][4], const int16_t mv[2][16][2]) {
    if (!active || mb_prepared) return -1;
    int w = cur->w, cw = w / 2, mx = mb_addr % cur->mbw * 16, my = mb_addr / cur->mbw * 16;
    inter(mb_addr, rf, mv, cur->y.data() + my * w + mx, cur->u.data() + my / 2 * cw + mx / 2,
          cur->v.data() + my / 2 * cw + mx / 2);
    skip_in = true;
    memcpy(skip_rf, rf, sizeof skip_rf);
    memcpy(skip_mv, mv, sizeof skip_mv);
    return miss(0, 0, 0, 16);
  }
  // the sample at (x, y) of ref displaced by (vx, vy) quarter samples,
  // bilinear (as tm_search's)
  static int bil(const RPic* R, int x, int y, int vx, int vy) {
    int ix = x + (vx >> 2), iy = y + (vy >> 2), fx = vx & 3, fy = vy & 3, W = R->w, H = R->h;
    const uint8_t *a = R->y.data() + clip3(0, H - 1, iy) * W, *c = R->y.data() + clip3(0, H - 1, iy + 1) * W;
    int x1 = clip3(0, W - 1, ix), x2 = clip3(0, W - 1, ix + 1);
    return ((4 - fx) * (4 - fy) * a[x1] + fx * (4 - fy) * a[x2] + (4 - fx) * fy * c[x1] + fx * fy * c[x2] + 8) >> 4;
  }
  // How well the average of list 0's first reference by v0 and list 1's by
  // v1 predicts the decoded samples above (2 rows) and to the left (2
  // columns) of the macroblock: 4 x the mean absolute difference; -1 none
  int tm_bi(const int* v0, const int* v1) const {
    if (!active || nlist[0] < 1 || nlist[1] < 1 || !list[0][0] || !list[1][0]) return -1;
    int mbw = cur->mbw, x0 = mb_addr % mbw * 16, y0 = mb_addr / mbw * 16, w = cur->w, sum = 0, cnt = 0;
    bool top = y0 > 0 && db[mb_addr - mbw].slice >= 0, left = x0 > 0 && db[mb_addr - 1].slice >= 0;
    const RPic *R0 = list[0][0], *R1 = list[1][0];
    auto px = [&](int x, int y) {
      int p = (bil(R0, x, y, v0[0], v0[1]) + bil(R1, x, y, v1[0], v1[1]) + 1) >> 1;
      sum += abs(cur->y[(size_t)y * w + x] - p);
      cnt++;
    };
    if (top)
      for (int j = 1; j <= 2; j++)
        for (int i = 0; i < 16; i++) px(x0 + i, y0 - j);
    if (left)
      for (int j = 0; j < 16; j++)
        for (int i = 1; i <= 2; i++) px(x0 - i, y0 + j);
    return cnt ? sum * 4 / cnt : -1;
  }
  // How far apart the two predictions of a bi-predicted macroblock (rf,
  // mv) are: 4 x the mean absolute difference of their luma, from the
  // blocks of equal motion (8x8); -1 not bi-predicted anywhere
  int bi_diff(const int8_t rf[2][4], const int16_t mv[2][16][2]) const {
    if (!active) return -1;
    int mx = mb_addr % cur->mbw * 16, my = mb_addr / cur->mbw * 16, sum = 0, cnt = 0;
    uint8_t p[2][16 * 8];
    for (int b8 = 0; b8 < 4; b8++) {
      if (rf[0][b8] < 0 || rf[1][b8] < 0) continue;
      int x = (b8 & 1) * 8, y = (b8 >> 1) * 8, b = (y / 4) * 4 + x / 4;
      for (int l = 0; l < 2; l++) {
        int r = rf[l][b8];
        const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
        mc_luma(ref ? ref : cur.get(), mx + x, my + y, 8, 8, mv[l][b][0], mv[l][b][1], p[l]);
      }
      for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++) sum += abs(p[0][j * 16 + i] - p[1][j * 16 + i]);
      cnt += 64;
    }
    return cnt ? sum * 4 / cnt : -1;
  }
  // Template matching: the decoded samples above (2 rows) and to the left
  // (2 columns) of the current macroblock against their prediction by the
  // motion (rf, mv) of its blocks next to them, without weights: 4 x the
  // mean absolute difference (-1 none)
  int tm_miss(const int8_t rf[2][4], const int16_t mv[2][16][2]) const {
    if (!active) return -1;
    int mbw = cur->mbw, mx = mb_addr % mbw * 16, my = mb_addr / mbw * 16, w = cur->w;
    bool top = my > 0 && db[mb_addr - mbw].slice >= 0;
    bool left = mx > 0 && db[mb_addr - 1].slice >= 0;
    int sum = 0, cnt = 0;
    uint8_t p[2][64];
    for (int side = 0; side < 2; side++) {
      if (side == 0 ? !top : !left) continue;
      for (int k = 0; k < 4; k++) {
        int b = side == 0 ? k : k * 4, b8 = (b >> 3) * 2 + ((b & 3) >> 1);
        int x = side == 0 ? mx + k * 4 : mx - 2, y = side == 0 ? my - 2 : my + k * 4;
        int bw = side == 0 ? 4 : 2, bh = side == 0 ? 2 : 4, n = 0;
        for (int l = 0; l < 2; l++) {
          int r = rf[l][b8];
          if (r < 0) continue;
          const RPic* ref = r < nlist[l] ? list[l][r] : nullptr;
          mc_luma(ref ? ref : cur.get(), x, y, bw, bh, mv[l][b][0], mv[l][b][1], p[n++]);
        }
        if (!n) continue;
        const uint8_t* q = cur->y.data() + y * w + x;
        for (int j = 0; j < bh; j++)
          for (int i = 0; i < bw; i++) {
            int v = n == 2 ? (p[0][j * 16 + i] + p[1][j * 16 + i] + 1) >> 1 : p[0][j * 16 + i];
            sum += abs(q[j * w + i] - v);
            cnt++;
          }
      }
    }
    return cnt ? sum * 4 / cnt : -1;
  }
  // Template matching motion search for the luma block (bx, by, bw, bh)
  // of the current macroblock, on reference r of list l: from mv (quarter
  // samples), the vector whose prediction of the decoded samples above (2
  // rows) and to the left (2 columns) of the block misses least: whole
  // samples within 2, then halves, then quarters (only_y: the vertical
  // component only), cand the other vectors to start from; miss, 4 x the
  // mean absolute difference there. l < 0: on proxy_pic, with 16 rows and
  // columns and the block above right too (the proxy wants the content to
  // match, a vector to code wants a thin template). The template outside the
  // macroblock only, but for inner 1: the block itself, decoded, alone; 2:
  // and the blocks of the macroblock decoded so far. False where none of it
  // is decoded.
  static constexpr int TM_T = 2, TM_TP = 16;
  bool tm_search(int l, int r, int bx, int by, int bw, int bh, int* mv, bool only_y, const int (*cand)[2] = nullptr,
                 int ncand = 0, int* miss = nullptr, int lambda = 0, int finest = 1, int inner = 0) const {
    if (!active) return false;
    int mbw = cur->mbw, x0 = mb_addr % mbw * 16 + bx, y0 = mb_addr / mbw * 16 + by, w = cur->w;
    bool top = by == 0 && y0 > 0 && db[mb_addr - mbw].slice >= 0;
    bool left = bx == 0 && x0 > 0 && db[mb_addr - 1].slice >= 0;
    const RPic* ref = l < 0 ? proxy_pic() : r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
    if (inner == 1) top = left = false;
    int ninner = inner == 1 ? bw * bh : 0;
    if (inner == 2) {
      for (int b = 0; b < 16; b++) ninner += (mb_done4 >> b & 1) * 16;
      for (int b8 = 0; b8 < 4; b8++) ninner += (mb_done8 >> b8 & 1) * 64;
    }
    if ((!top && !left && !ninner) || !ref) return false;
    // the samples of a rectangle of the current picture against the
    // reference displaced by (vx, vy): bilinear between whole samples (not
    // the six taps of the prediction; fast)
    const uint8_t *R = ref->y.data(), *Y = cur->y.data();
    int W = ref->w, H = ref->h;
    // (stopping once it reaches budget)
    auto sad = [&](int xs, int ys, int rw, int rh, int vx, int vy, int budget) {
      int ix = xs + (vx >> 2), iy = ys + (vy >> 2), fx = vx & 3, fy = vy & 3, s = 0;
      int wa = (4 - fx) * (4 - fy), wb = fx * (4 - fy), wc = (4 - fx) * fy, wd = fx * fy;
      bool inside = ix >= 0 && iy >= 0 && ix + rw < W && iy + rh < H;
      for (int j = 0; j < rh && s < budget; j++) {
        const uint8_t* q = Y + (ys + j) * w + xs;
        if (inside) {
          const uint8_t *a = R + (iy + j) * W + ix, *c = a + W;
          if (!(fx | fy))
            for (int i = 0; i < rw; i++) s += abs(q[i] - a[i]);
          else
            for (int i = 0; i < rw; i++) s += abs(q[i] - ((wa * a[i] + wb * a[i + 1] + wc * c[i] + wd * c[i + 1] + 8) >> 4));
        } else {
          const uint8_t *a = R + clip3(0, H - 1, iy + j) * W, *c = R + clip3(0, H - 1, iy + j + 1) * W;
          for (int i = 0; i < rw; i++) {
            int x1 = clip3(0, W - 1, ix + i), x2 = clip3(0, W - 1, ix + i + 1);
            s += abs(q[i] - ((wa * a[x1] + wb * a[x2] + wc * c[x1] + wd * c[x2] + 8) >> 4));
          }
        }
      }
      return s;
    };
    const int px = mv[0], py = mv[1];
    const int T = l < 0 ? TM_TP : TM_T;
    bool tr = l < 0 && top && x0 + bw + T <= w && mb_addr % mbw + 1 < mbw && db[mb_addr - mbw + 1].slice >= 0;
    auto inner_sad = [&](int vx, int vy, int budget) {
      if (inner == 1) return sad(x0, y0, bw, bh, vx, vy, budget);
      int s = 0;
      for (int b = 0; b < 16 && s < budget; b++)
        if (mb_done4 >> b & 1) s += sad(x0 + blk_x(b) * 4, y0 + blk_y(b) * 4, 4, 4, vx, vy, budget - s);
      for (int b8 = 0; b8 < 4 && s < budget; b8++)
        if (mb_done8 >> b8 & 1) s += sad(x0 + (b8 & 1) * 8, y0 + (b8 >> 1) * 8, 8, 8, vx, vy, budget - s);
      return s;
    };
    // the cost of a vector, or something not below bound once it is not
    auto cost = [&](int vx, int vy, int bound = INT_MAX) {
      int c = lambda * (abs(vx - px) + abs(vy - py));
      if (top && c < bound) c += sad(x0, y0 - T, bw, T, vx, vy, bound - c);
      if (left && c < bound) c += sad(x0 - T, y0, T, bh, vx, vy, bound - c);
      if (tr && c < bound) c += sad(x0 + bw, y0 - T, T, T, vx, vy, bound - c);
      if (ninner && c < bound) c += inner_sad(vx, vy, bound - c);
      return c;
    };
    int vx = mv[0], vy = mv[1], best = cost(vx, vy);
    // other places to start from
    for (int k = 0; k < ncand; k++) {
      if (cand[k][0] == vx && cand[k][1] == vy) continue;
      int c = cost(cand[k][0], cand[k][1], best);
      if (c < best) best = c, vx = cand[k][0], vy = cand[k][1];
    }
    // a small diamond in whole samples while it finds better (up to 4
    // steps), then the 8 halves and quarters around
    static const int kD[4][2] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
    for (int it = 0; it < 4 && best; it++) {
      int cx = vx, cy = vy;
      for (int k = 0; k < 4; k++) {
        if (only_y && kD[k][0]) continue;
        int c = cost(cx + kD[k][0] * 4, cy + kD[k][1] * 4, best);
        if (c < best) best = c, vx = cx + kD[k][0] * 4, vy = cy + kD[k][1] * 4;
      }
      if (vx == cx && vy == cy) break;
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
    if (miss) *miss = best * 4 / ((top ? T * bw : 0) + (left ? T * bh : 0) + (tr ? T * T : 0) + ninner);
    return true;
  }
  // The intra mode that would predict the block best in the last picture:
  // the luma block of n x n at (bx, by) blocks of the current macroblock in
  // list 0's first reference (else the last reference decoded), predicted
  // from that picture's samples around it by each mode, the one that misses
  // least (sum of absolute differences); -1 no picture
  // a stand-in for the source of the current (intra) macroblock: its
  // prediction from proxy_pic by a vector (proxy_build), luma and chroma
  uint8_t prx_y[256], prx_c[2][64];
  bool prx_ok = false;
  bool proxy_build(const int* mv) {
    prx_ok = false;
    const RPic* P = active ? proxy_pic() : nullptr;
    if (!P) return false;
    int mx = mb_addr % cur->mbw * 16, my = mb_addr / cur->mbw * 16;
    mc_luma(P, mx, my, 16, 16, mv[0], mv[1], prx_y);
    for (int c = 0; c < 2; c++) mc_chroma(P, c, mx / 2, my / 2, 8, 8, mv[0], mv[1], prx_c[c]);
    prx_ok = true;
    return true;
  }
  // how far the luma of the current macroblock decoded so far is from the
  // proxy: 4 x the mean absolute difference; -1 none decoded or no proxy
  int proxy_acc() const {
    if (!active || !prx_ok) return -1;
    int w = cur->w, mx = mb_addr % cur->mbw * 16, my = mb_addr / cur->mbw * 16, sum = 0, cnt = 0;
    const uint8_t* Y = cur->y.data() + my * w + mx;
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
    const RPic* P = nlist[0] > 0 && list[0][0] ? list[0][0] : !dpb.empty() ? dpb.back().get() : nullptr;
    return P && P->w == cur->w && P->h == cur->h ? P : nullptr;
  }
  // The nonzero levels an encoder would have for the luma block of n x n at
  // (bx, by) blocks of the current macroblock in intra mode m were the
  // source and its surroundings the proxy picture there (displaced by dx,
  // dy); -1 no picture
  int proxy_nz(int bx, int by, int n, int m, int qp, int dx, int dy) const {
    if (!active) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int w = P->w, x0 = mb_addr % cur->mbw * 16 + bx * n, y0 = mb_addr / cur->mbw * 16 + by * n;
    x0 = clip3(0, w - n, x0 + dx);
    y0 = clip3(0, P->h - n, y0 + dy);
    const uint8_t* Y = P->y.data();
    bool top = y0 > 0, left = x0 > 0, tl = top && left, tr = top && x0 + 2 * n <= w;
    int e[25];
    e[0] = tl ? Y[(y0 - 1) * w + x0 - 1] : 128;
    for (int i = 0; i < n; i++) e[1 + i] = top ? Y[(y0 - 1) * w + x0 + i] : 128;
    for (int i = 0; i < n; i++) e[1 + n + i] = tr ? Y[(y0 - 1) * w + x0 + n + i] : e[n];
    for (int i = 0; i < n; i++) e[1 + 2 * n + i] = left ? Y[(y0 + i) * w + x0 - 1] : 128;
    if (n == 8) filter8(e, top, left, tl);
    uint8_t d[64];
    pred_nxn(d, 8, n, m, e, top, left);
    int r[64], lv[64], cnt = 0;
    for (int y = 0; y < n; y++)
      for (int x = 0; x < n; x++) r[y * n + x] = Y[(y0 + y) * w + x0 + x] - d[y * 8 + x];
    if (n == 8)
      fquant8(r, qp, lv);
    else
      fquant4(r, qp, lv);
    for (int k = 0; k < n * n; k++) cnt += abs(lv[k]) >= 3;
    return cnt;
  }
  int proxy_mode(int bx, int by, int n, int dx, int dy, int* sads = nullptr) const {
    if (!active) return -1;
    const RPic* P = proxy_pic();
    if (!P) return -1;
    int w = P->w, x0 = mb_addr % cur->mbw * 16 + bx * n, y0 = mb_addr / cur->mbw * 16 + by * n;
    x0 = clip3(0, w - n, x0 + dx);
    y0 = clip3(0, P->h - n, y0 + dy);
    const uint8_t* Y = P->y.data();
    bool top = y0 > 0, left = x0 > 0, tl = top && left, tr = top && x0 + 2 * n <= w;
    int e[25];
    e[0] = tl ? Y[(y0 - 1) * w + x0 - 1] : 128;
    for (int i = 0; i < n; i++) e[1 + i] = top ? Y[(y0 - 1) * w + x0 + i] : 128;
    for (int i = 0; i < n; i++) e[1 + n + i] = tr ? Y[(y0 - 1) * w + x0 + n + i] : e[n];
    for (int i = 0; i < n; i++) e[1 + 2 * n + i] = left ? Y[(y0 + i) * w + x0 - 1] : 128;
    if (n == 8) filter8(e, top, left, tl);
    int best = -1, bsad = 1 << 30;
    uint8_t d[64];
    for (int m = 0; m < 9; m++) {
      if (sads) sads[m] = INT_MAX;
      if (((m == 0 || m == 3 || m == 7) && !top) || ((m == 1 || m == 8) && !left) || ((m >= 4 && m <= 6) && !tl)) continue;
      pred_nxn(d, 8, n, m, e, top, left);
      // the sum of the absolute Hadamard transformed differences, by 4x4
      // (x264's measure), halved
      int sad = 0;
      {
        for (int by4 = 0; by4 < n; by4 += 4)
          for (int bx4 = 0; bx4 < n; bx4 += 4) {
            int t[16];
            for (int y = 0; y < 4; y++)
              for (int x = 0; x < 4; x++) t[y * 4 + x] = Y[(y0 + by4 + y) * w + x0 + bx4 + x] - d[(by4 + y) * 8 + bx4 + x];
            for (int y = 0; y < 4; y++) {
              int* r = t + y * 4;
              int a0 = r[0] + r[1], a1 = r[0] - r[1], a2 = r[2] + r[3], a3 = r[2] - r[3];
              r[0] = a0 + a2, r[1] = a1 + a3, r[2] = a0 - a2, r[3] = a1 - a3;
            }
            for (int x = 0; x < 4; x++) {
              int a0 = t[x] + t[4 + x], a1 = t[x] - t[4 + x], a2 = t[8 + x] + t[12 + x], a3 = t[8 + x] - t[12 + x];
              sad += abs(a0 + a2) + abs(a1 + a3) + abs(a0 - a2) + abs(a1 - a3);
            }
          }
        sad = (sad + 1) >> 1;
      }
      if (sads) sads[m] = sad;
      if (sad < bsad) bsad = sad, best = m;
    }
    return best;
  }
  // The forward 4x4 transform and quantization of residual r (raster): the
  // levels in zigzag order, in quarters, not rounded (an encoder rounding
  // intra levels up from 2/3 would code (3 |q| + 4) / 12)
  static void fquant4(const int* r, int qp, int* lv) {
    static const int mf[6][3] = {{13107, 5243, 8066}, {11916, 4660, 7490}, {10082, 4194, 6554},
                                 {9362, 3647, 5825},  {8192, 3355, 5243},  {7282, 2893, 4559}};
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
      int m = mf[qp % 6][(x & 1) && (y & 1) ? 1 : !(x & 1) && !(y & 1) ? 0 : 2];
      int q = (int)(((int64_t)abs(W[p]) * m) >> qb);
      lv[k] = W[p] < 0 ? -q : q;
    }
  }
  // the same for an 8x8 block (x264's dct8)
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
  // fquant4 or fquant8 at qp. plane 0 luma, 1 and 2 chroma; x, y in its
  // samples of the current macroblock. false: no proxy.
  bool proxy_levels(int plane, int x, int y, int qp, int* lv, int n = 4) const {
    if (!active || !prx_ok) return false;
    int w = plane ? cur->w / 2 : cur->w, m = plane ? 8 : 16;
    int mx = mb_addr % cur->mbw * m + x, my = mb_addr / cur->mbw * m + y;
    const std::vector<uint8_t>& cp = plane == 0 ? cur->y : plane == 1 ? cur->u : cur->v;
    const uint8_t* C = cp.data() + my * w + mx;
    const uint8_t* Q = (plane ? prx_c[plane - 1] : prx_y) + y * m + x;
    int r[64];
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) r[j * n + i] = Q[j * m + i] - C[j * w + i];
    if (n == 8)
      fquant8(r, qp, lv);
    else
      fquant4(r, qp, lv);
    return true;
  }
  // and for the DC blocks: Intra16x16's (plane 0: 16 levels in zigzag
  // order) and chroma's (planes 1, 2: 4 levels)
  bool proxy_dc(int plane, int qp, int* lv) const {
    if (!active || !prx_ok) return false;
    int w = plane ? cur->w / 2 : cur->w, m = plane ? 8 : 16, nb = m / 4;
    int mx = mb_addr % cur->mbw * m, my = mb_addr / cur->mbw * m;
    const std::vector<uint8_t>& cp = plane == 0 ? cur->y : plane == 1 ? cur->u : cur->v;
    const uint8_t* C = cp.data() + my * w + mx;
    const uint8_t* Q = plane ? prx_c[plane - 1] : prx_y;
    // the DC of each 4x4 block's transform: the sum of its residual
    int d[16], f[16];
    for (int by = 0; by < nb; by++)
      for (int bx = 0; bx < nb; bx++) {
        int sum = 0;
        for (int j = 0; j < 4; j++)
          for (int i = 0; i < 4; i++) sum += Q[(by * 4 + j) * m + bx * 4 + i] - C[(by * 4 + j) * w + bx * 4 + i];
        d[by * nb + bx] = sum;
      }
    static const int mf0[6] = {13107, 11916, 10082, 9362, 8192, 7282};
    int qb = 15 + qp / 6;
    if (nb == 2) {
      f[0] = d[0] + d[1] + d[2] + d[3];
      f[1] = d[0] - d[1] + d[2] - d[3];
      f[2] = d[0] + d[1] - d[2] - d[3];
      f[3] = d[0] - d[1] - d[2] + d[3];
      for (int k = 0; k < 4; k++) {
        int q = (int)(((int64_t)abs(f[k]) * mf0[qp % 6]) >> (qb - 1));
        lv[k] = f[k] < 0 ? -q : q;
      }
      return true;
    }
    int t[16];
    for (int i = 0; i < 4; i++) {
      const int* x = d + i * 4;
      t[i * 4] = x[0] + x[1] + x[2] + x[3];
      t[i * 4 + 1] = x[0] + x[1] - x[2] - x[3];
      t[i * 4 + 2] = x[0] - x[1] - x[2] + x[3];
      t[i * 4 + 3] = x[0] - x[1] + x[2] - x[3];
    }
    for (int j = 0; j < 4; j++) {
      f[j] = (t[j] + t[4 + j] + t[8 + j] + t[12 + j]) / 2;
      f[4 + j] = (t[j] + t[4 + j] - t[8 + j] - t[12 + j]) / 2;
      f[8 + j] = (t[j] - t[4 + j] - t[8 + j] + t[12 + j]) / 2;
      f[12 + j] = (t[j] - t[4 + j] + t[8 + j] - t[12 + j]) / 2;
    }
    for (int k = 0; k < 16; k++) {
      int v = f[kZz4[k]];
      int q = (int)(((int64_t)abs(v) * mf0[qp % 6]) >> (qb - 1));
      lv[k] = v < 0 ? -q : q;
    }
    return true;
  }
  // the same of reference r of list l displaced by mv (whole samples) at
  // the luma block (bx, by, bw, bh) of the current macroblock; -1 none
  int ref_texture(int l, int r, int bx, int by, int bw, int bh, const int* mv) const {
    const RPic* ref = active && r >= 0 && r < nlist[l] ? list[l][r] : nullptr;
    if (!ref) return -1;
    int W = ref->w, H = ref->h;
    int x0 = clip3(0, W - bw - 1, mb_addr % cur->mbw * 16 + bx + (mv[0] >> 2));
    int y0 = clip3(0, H - bh - 1, mb_addr / cur->mbw * 16 + by + (mv[1] >> 2));
    const uint8_t* p = ref->y.data() + y0 * W + x0;
    int g = 0;
    for (int j = 0; j < bh; j++)
      for (int i = 0; i < bw; i++) g += abs(p[j * W + i + 1] - p[j * W + i]) + abs(p[(j + 1) * W + i] - p[j * W + i]);
    return g;
  }
  int pred_texture(int c, int x, int y, int n) const {
    if (!active || !mb_prepared || mb_cls == 8) return -1;
    int w = c ? cur->w / 2 : cur->w, m = c ? 8 : 16;
    const uint8_t* p = (c == 0 ? cur->y.data() : c == 1 ? cur->u.data() : cur->v.data()) +
                       ((mb_addr / cur->mbw) * m + y) * w + (mb_addr % cur->mbw) * m + x;
    int g = 0;
    for (int j = 0; j < n; j++)
      for (int i = 0; i < n; i++) {
        if (i + 1 < n) g += abs(p[j * w + i + 1] - p[j * w + i]);
        if (j + 1 < n) g += abs(p[(j + 1) * w + i] - p[j * w + i]);
      }
    return g;
  }

  // For sign prediction: the samples next to the luma block of n x n at (x,
  // y) of the current macroblock, its prediction being in the picture: per
  // edge sample e (n above, then n to the left) the linear extrapolation of
  // the two decoded rows or columns beyond the edge less the prediction
  // there, times scale; avail[e] false where those are not decoded.
  bool sign_edges(int plane, int x, int y, int n, int64_t scale, int64_t* target, bool* avail) const {
    if (!active || !mb_prepared || mb_cls == 8) return false;
    return edges(plane, x, y, n, scale, target, avail);
  }
  bool edges(int plane, int x, int y, int n, int64_t scale, int64_t* target, bool* avail) const {
    int mbw = cur->mbw, mx = mb_addr % mbw, my = mb_addr / mbw;
    int w = plane ? cur->w / 2 : cur->w, m = plane ? 8 : 16;
    bool top = y > 0 ? true : my > 0 && db[mb_addr - mbw].slice >= 0;
    bool left = x > 0 ? true : mx > 0 && db[mb_addr - 1].slice >= 0;
    if (!top && !left) return false;
    const uint8_t* p = (plane == 0 ? cur->y.data() : plane == 1 ? cur->u.data() : cur->v.data()) + (my * m + y) * w + mx * m + x;
    for (int i = 0; i < n; i++) {
      avail[i] = top;
      target[i] = top ? (2 * p[-w + i] - p[-2 * w + i] - p[i]) * scale : 0;
      avail[n + i] = left;
      target[n + i] = left ? (2 * p[i * w - 1] - p[i * w - 2] - p[i * w]) * scale : 0;
    }
    return true;
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
      int ls = 16 * kNorm4[m][(x & 1) && (y & 1) ? 1 : !(x & 1) && !(y & 1) ? 0 : 2];
      return qp >= 24 ? ((int64_t)c * ls) << (q6 - 4) : ((int64_t)c * ls + (1 << (3 - q6))) >> (4 - q6);
    }
    int r = kZz8[k], ls = 16 * kNorm8[m][norm8_class(r >> 3, r & 7)];
    return qp >= 36 ? ((int64_t)c * ls) << (q6 - 6) : ((int64_t)c * ls + (1 << (5 - q6))) >> (6 - q6);
  }

  // How far the prediction of a block of n x n samples at (x, y) of plane c
  // is from continuing the decoded samples around it: the mean over its
  // top and left edge samples of |2 R1 - R2 - P|, in quarter samples; -1
  // not known.
  int edge_miss(int c, int x, int y, int n) const {
    if (!active || !mb_prepared || mb_cls == 8) return -1;
    return miss(c, x, y, n);
  }
  int miss(int c, int x, int y, int n) const {
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

}  // namespace h264p
