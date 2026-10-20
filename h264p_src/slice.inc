// Slice data: the macroblock layer, CAVLC or CABAC coded (H.264 7.3.4, 7.3.5).

namespace h264p {

// What later macroblocks of the slice need to know about a macroblock to
// parse their own syntax elements (neighbour contexts).
struct MbState {
  uint32_t slice;  // number of the slice it was coded in
  uint8_t skip, intra, pcm, i16, inxn, si, direct16, t8x8;
  uint8_t field;  // mb_field_decoding_flag (MBAFF)
  uint8_t cbp;    // CodedBlockPatternLuma | CodedBlockPatternChroma << 4
  uint8_t cpred;  // intra_chroma_pred_mode
  uint8_t cbf_dc[3];       // coded_block_flag of the DC blocks: Y (Intra16x16), Cb, Cr
  uint8_t nz[3][16];       // TotalCoeff of the 4x4 blocks, raster order: Y, Cb, Cr
  int8_t ref[2][4];        // ref_idx_lX of the 8x8 blocks when coded, else 0
  uint8_t mvd[2][16][2];   // |mvd_lX| of the 4x4 blocks, clipped to 127
};

// Macroblock states of the current picture, kept between slices.
struct MbStore {
  std::vector<MbState> mbs;
  uint32_t slices = 0;
};

// Parses the slice data following the slice header, handing the values to
// a channel, or writes it with the values of a channel. The slice data ends
// with the rbsp_stop_one_bit (written by the CABAC flush, or explicitly for
// CAVLC).
void read_slice_data(const SliceHeader& h, MbStore& store, BitReader& br, Chan& ch);
void write_slice_data(const SliceHeader& h, MbStore& store, Chan& ch, BitWriter& bw);

// 4x4 block position (in 4x4 units) of luma4x4BlkIdx
inline int blk_x(int b) { return (b & 1) | ((b >> 1) & 2); }
inline int blk_y(int b) { return ((b >> 1) & 1) | ((b >> 2) & 2); }

int count_nonzero(const int* c, int n) {
  int k = 0;
  for (int i = 0; i < n; i++) k += c[i] != 0;
  return k;
}

// Partition shapes in 4x4 block units
struct Part {
  int x, y, w, h;
};
const Part kParts16x16[1] = {{0, 0, 4, 4}};
const Part kParts16x8[2] = {{0, 0, 4, 2}, {0, 2, 4, 2}};
const Part kParts8x16[2] = {{0, 0, 2, 4}, {2, 0, 2, 4}};
const Part kSubParts[4][4] = {
    {{0, 0, 2, 2}},                                // 8x8
    {{0, 0, 2, 1}, {0, 1, 2, 1}},                  // 8x4
    {{0, 0, 1, 2}, {1, 0, 1, 2}},                  // 4x8
    {{0, 0, 1, 1}, {1, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 1}},  // 4x4
};
const int kNumSubParts[4] = {1, 2, 2, 4};
// B_8x8 sub_mb_type: prediction lists (1 L0, 2 L1, 3 both; 0 direct), shape
const int kBSubPred[13] = {0, 1, 2, 3, 1, 1, 2, 2, 3, 3, 1, 2, 3};
const int kBSubShape[13] = {0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 3, 3, 3};
// B 16x8 / 8x16 types 4..21: prediction lists of both partitions
const int kBPartPred[9][2] = {{1, 1}, {2, 2}, {1, 2}, {2, 1}, {1, 3}, {2, 3}, {3, 1}, {3, 2}, {3, 3}};

// first ctxIdx for each ctxBlockCat (tables 9-34 and 9-40): coded_block_flag,
// significant_coeff_flag and last_significant_coeff_flag (frame and field
// coded), coeff_abs_level_minus1
const int kCbfBase[14] = {85, 89, 93, 97, 101, 1012, 460, 464, 468, 1016, 472, 476, 480, 1020};
const int kSigBase[2][14] = {{105, 120, 134, 149, 152, 402, 484, 499, 513, 660, 528, 543, 557, 718},
                             {277, 292, 306, 321, 324, 436, 776, 791, 805, 675, 820, 835, 849, 733}};
const int kLastBase[2][14] = {{166, 181, 195, 210, 213, 417, 572, 587, 601, 690, 616, 631, 645, 748},
                              {338, 353, 367, 382, 385, 451, 864, 879, 893, 699, 908, 923, 937, 757}};
const int kAbsBase[14] = {227, 237, 247, 257, 266, 426, 952, 962, 972, 708, 982, 992, 1002, 766};
// ctxBlockCat of the DC, AC, 4x4 and 8x8 blocks of Y, Cb and Cr (4:4:4)
const int kCatDc[3] = {0, 6, 10}, kCatAc[3] = {1, 7, 11}, kCat4x4[3] = {2, 8, 12}, kCat8x8[3] = {5, 9, 13};

// ---------------------------------------------------------------------------
// slice group map (8.2.2)

std::vector<uint8_t> slice_group_map(const SliceHeader& h, int width, int size) {
  const Sps& sps = *h.sps;
  const Pps& pps = *h.pps;
  int units_h = sps.height_map_units;
  int units = width * units_h;
  std::vector<int> map(units, 0);
  int ngroups = pps.num_slice_groups;
  int dir = pps.slice_group_change_direction_flag;
  int64_t in_group0 = std::min<int64_t>((int64_t)h.slice_group_change_cycle * pps.slice_group_change_rate, units);
  int upper_left = (int)(dir ? units - in_group0 : in_group0);
  switch (pps.slice_group_map_type) {
    case 0: {
      int i = 0;
      do
        for (int g = 0; g < ngroups && i < units; i += pps.run_length_minus1[g++] + 1)
          for (int j = 0; j <= pps.run_length_minus1[g] && i + j < units; j++) map[i + j] = g;
      while (i < units);
      break;
    }
    case 1:
      for (int i = 0; i < units; i++) map[i] = ((i % width) + (((i / width) * ngroups) / 2)) % ngroups;
      break;
    case 2:
      for (int i = 0; i < units; i++) map[i] = ngroups - 1;
      for (int g = ngroups - 2; g >= 0; g--) {
        int y0 = pps.top_left[g] / width, x0 = pps.top_left[g] % width;
        int y1 = pps.bottom_right[g] / width, x1 = pps.bottom_right[g] % width;
        for (int y = y0; y <= y1; y++)
          for (int x = x0; x <= x1; x++)
            if (y * width + x < units) map[y * width + x] = g;
      }
      break;
    case 3: {
      for (int i = 0; i < units; i++) map[i] = 1;
      int x = (width - dir) / 2, y = (units_h - dir) / 2;
      int left = x, top = y, right = x, bottom = y;
      int xdir = dir - 1, ydir = dir;
      for (int64_t k = 0; k < in_group0;) {
        int vacant = map[y * width + x] == 1;
        if (vacant) map[y * width + x] = 0;
        if (xdir == -1 && x == left) {
          left = std::max(left - 1, 0);
          x = left;
          xdir = 0;
          ydir = 2 * dir - 1;
        } else if (xdir == 1 && x == right) {
          right = std::min(right + 1, width - 1);
          x = right;
          xdir = 0;
          ydir = 1 - 2 * dir;
        } else if (ydir == -1 && y == top) {
          top = std::max(top - 1, 0);
          y = top;
          xdir = 1 - 2 * dir;
          ydir = 0;
        } else if (ydir == 1 && y == bottom) {
          bottom = std::min(bottom + 1, units_h - 1);
          y = bottom;
          xdir = 2 * dir - 1;
          ydir = 0;
        } else {
          x += xdir;
          y += ydir;
        }
        k += vacant;
      }
      break;
    }
    case 4:
      for (int i = 0; i < units; i++) map[i] = i < upper_left ? dir : 1 - dir;
      break;
    case 5: {
      int k = 0;
      for (int j = 0; j < width; j++)
        for (int i = 0; i < units_h; i++) map[i * width + j] = k++ < upper_left ? dir : 1 - dir;
      break;
    }
    case 6:
      if ((int)pps.slice_group_id.size() != units) throw ParseError("slice_group_id does not match the picture size");
      for (int i = 0; i < units; i++) map[i] = pps.slice_group_id[i];
      break;
  }
  std::vector<uint8_t> mb(size);
  for (int i = 0; i < size; i++) {
    if (sps.frame_mbs_only_flag || h.field_pic_flag)
      mb[i] = (uint8_t)map[i];
    else if (sps.mb_adaptive_frame_field_flag)
      mb[i] = (uint8_t)map[i / 2];
    else
      mb[i] = (uint8_t)map[(i / (2 * width)) * width + (i % width)];
  }
  return mb;
}

// ---------------------------------------------------------------------------

struct SliceCoder {
  SliceCoder(const SliceHeader& h, MbStore& store, BitReader* br, BitWriter* bw, Chan& ch)
      : W(bw != nullptr), nobins_(W && ch.nobins), h_(h), sps_(*h.sps), pps_(*h.pps), store_(store), br_(br), bw_(bw),
        ch_(&ch), rec_(W ? nullptr : ch.rec) {}

  void run() {
    stype_ = h_.type();
    cabac_ = pps_.entropy_coding_mode_flag;
    field_ = h_.field_pic_flag;
    chroma_ = sps_.chroma_array_type();
    ch_h_ = chroma_ == 2 ? 4 : 2;
    ch_blocks_ = chroma_ == 2 ? 8 : 4;
    mbaff_ = sps_.mb_adaptive_frame_field_flag && !h_.field_pic_flag;
    if (h_.nal_unit_type != 1 && h_.nal_unit_type != 5) fail("unsupported NAL unit type");
    width_ = sps_.width_mbs;
    size_ = width_ * (sps_.frame_height_mbs() / (1 + field_));
    num_ref_[0] = h_.num_ref_idx_active[0];
    num_ref_[1] = h_.num_ref_idx_active[1];
    if (pps_.num_slice_groups > 1) group_ = slice_group_map(h_, width_, size_);
    if ((int)store_.mbs.size() < size_) store_.mbs.resize(size_, MbState());
    slice_id_ = ++store_.slices;
    if (h_.first_mb * (1 + mbaff_) >= size_) fail("first_mb_in_slice out of range");

    if (cabac_) {
      if (!W) {
        while (!br_->byte_aligned())
          if (!br_->bit()) fail("cabac_alignment_one_bit is 0");
      } else {
        while (bw_->pos() & 7) bw_->bit(1);
      }
      int qp = pps_.pic_init_qp + h_.slice_qp_delta;
      cabac_init_contexts(ctx_, qp, stype_ == kI || stype_ == kSI ? 0 : h_.cabac_init_idc + 1);
      if (!W)
        dec_init();
      else if (!nobins_)
        enc_.init(bw_);
      else if (ch_->rec)
        chk_ = ch_->rec->bins.data();
    }
    ch_->slice_begin(*this);
    loop();
    ch_->slice_end();
    if (cabac_ && rec_) rec_->bins.push_back(2);
    if (chk_ && *chk_ != 2) ch_->rec->bad = true;
    if (!cabac_) {
      if (!W) {
        // the rbsp_stop_one_bit
        if (!br_->bit()) fail("missing rbsp_stop_one_bit");
      } else {
        bw_->bit(1);
      }
    }
    if (rec_) rec_->slice = true, rec_->end = br_->pos();
  }

  // --- errors
  [[noreturn]] void fail(const std::string& msg) {
    if (!W) throw ParseError(msg);
    ch_->fail(msg);
    throw TsvError(msg);  // not reached
  }

  // --- CAVLC primitives
  int u(int n, int v) {
    if (!W) return (int)br_->u(n);
    if (v < 0 || v >= (1 << n)) fail("value " + std::to_string(v) + " out of range");
    bw_->put((uint32_t)v, n);
    return v;
  }
  int ue(int v, int hi) {
    if (!W) {
      uint32_t x = br_->ue();
      if (x > (uint32_t)hi) fail("value out of range");
      return (int)x;
    }
    if (v < 0 || v > hi) fail("value " + std::to_string(v) + " out of range");
    bw_->ue((uint32_t)v);
    return v;
  }
  int se(int v, int lim) {
    if (!W) {
      int64_t x = br_->se();
      if (x < -lim || x > lim) fail("value out of range");
      return (int)x;
    }
    if (v < -lim || v > lim) fail("value " + std::to_string(v) + " out of range");
    bw_->se(v);
    return v;
  }
  int te(int v, int range) {
    if (W && (v < 0 || v > range)) fail("value " + std::to_string(v) + " out of range");
    if (range == 1) return !u(1, !v);
    return ue(v, range);
  }

  // --- CABAC primitives
  int bin(int ctx, int v) {
    if (!W) return got(dec_.decision(ctx_[ctx]));
    if (!nobins_)
      enc_.decision(ctx_[ctx], v);
    else if (chk_)
      check(v);
    return v;
  }
  int bypass(int v) {
    if (!W) return got(dec_.bypass());
    if (!nobins_)
      enc_.bypass(v);
    else if (chk_)
      check(v);
    return v;
  }
  int term(int v) {
    if (!W) {
      int b = got(dec_.terminate());
      if (b && rec_) {  // the end of a run of CABAC data: its last bit
        size_t e = br_->pos();
        rec_->segs.push_back(e);
        if (!(br_->data_[(e - 1) >> 3] >> (7 - ((e - 1) & 7)) & 1)) rec_->stops = false;
      }
      return b;
    }
    if (!nobins_)
      enc_.terminate(v);
    else if (chk_)
      check(v);
    return v;
  }
  void dec_init() {
    if (rec_) rec_->segs.push_back(br_->pos());
    dec_.init(br_);
  }
  int got(int b) {
    if (rec_) rec_->bins.push_back((uint8_t)b);
    return b;
  }
  // (a bin that differs stays the next one, failing all that follow)
  void check(int v) {
    if (*chk_ == v)
      chk_++;
    else
      ch_->rec->bad = true;
  }

  int exp_golomb_bypass(int k, int v) {
    int x = 0;
    while (bypass(W ? v - x >= (1 << k) : 0)) {
      x += 1 << k;
      if (++k > 28) fail("invalid Exp-Golomb suffix");
    }
    while (k--) x += bypass(W ? ((v - x) >> k) & 1 : 0) << k;
    return x;
  }

  // --- neighbours
  const MbState* avail(int addr) const {
    return store_.mbs[addr].slice == slice_id_ ? &store_.mbs[addr] : nullptr;
  }

  // macroblock containing the sample (xN, yN) relative to the current
  // macroblock, maxW x maxH samples (6.4.12; for MBAFF frames table 6-4), and
  // the sample position in it; xN or yN may be -1 (neighbours A and B only)
  const MbState* nb_loc(int xN, int yN, int maxW, int maxH, int& xW, int& yW) const {
    if (xN >= 0 && yN >= 0) {
      xW = xN;
      yW = yN;
      return cur_;
    }
    if (!mbaff_) {
      xW = xN < 0 ? xN + maxW : xN;
      yW = yN < 0 ? yN + maxH : yN;
      return xN < 0 ? left_ : above_;
    }
    bool top = !(cur_addr_ & 1), curf = cur_->field;
    const MbState* n;
    int yM;
    if (xN < 0) {  // 0 <= yN < maxH
      if (!left_) return nullptr;
      const MbState* a = left_;  // top macroblock of the left pair, a + 1 its bottom one
      if (!curf) {
        if (!a->field) {
          n = top ? a : a + 1;
          yM = yN;
        } else {
          n = (yN & 1) ? a + 1 : a;
          yM = top ? yN >> 1 : (yN + maxH) >> 1;
        }
      } else if (!a->field) {
        int y = (yN << 1) + !top;
        n = y < maxH ? a : a + 1;
        yM = y < maxH ? y : y - maxH;
      } else {
        n = top ? a : a + 1;
        yM = yN;
      }
      xW = xN + maxW;
    } else {  // yN < 0, 0 <= xN < maxW
      if (!curf && !top) {
        n = cur_ - 1;  // top macroblock of the same pair
        yM = yN;
      } else {
        if (!above_) return nullptr;
        if (curf && top && above_->field) {
          n = above_;
          yM = yN;
        } else {
          n = above_ + 1;
          yM = curf && top ? 2 * yN : yN;
        }
      }
      xW = xN;
    }
    yW = yM < 0 ? yM + maxH : yM;
    return n;
  }

  // macroblock and 4x4 block containing the luma 4x4 block (x, y) of the
  // current macroblock, x or y may be -1
  const MbState* nb4(int x, int y, int& idx) const {
    int xW = 0, yW = 0;
    const MbState* n = nb_loc(x < 0 ? -1 : 4 * x, y < 0 ? -1 : 4 * y, 16, 16, xW, yW);
    idx = (yW >> 2) * 4 + (xW >> 2);
    return n;
  }
  // the same for chroma 4x4 blocks (2 wide, ch_h_ high)
  const MbState* nbc(int x, int y, int& idx) const {
    int xW = 0, yW = 0;
    const MbState* n = nb_loc(x < 0 ? -1 : 4 * x, y < 0 ? -1 : 4 * y, 8, 4 * ch_h_, xW, yW);
    idx = (yW >> 2) * 2 + (xW >> 2);
    return n;
  }
  bool field_mb() const { return field_ || cur_->field; }

  // --- slice data
  //
  // A macroblock at a time: whether it is skipped, the macroblock, and
  // whether the slice ends after it. With CAVLC, the skipped macroblocks
  // are runs (mb_skip_run) before the coded ones: parsing, a run is read
  // before the first macroblock and after each coded one that is not the
  // last; writing, the skipped macroblocks are counted and their run is
  // written before the next coded one, or at the end.
  void loop() {
    bool intra = stype_ == kI || stype_ == kSI;
    int addr = h_.first_mb * (1 + mbaff_);
    bool prev_skipped = false;
    bool expect_run = !cabac_ && !intra;  // parsing CAVLC: a mb_skip_run comes next
    int run = 0;  // CAVLC: skipped macroblocks left in the run (parsing), not written yet (writing)
    for (;;) {
      start_mb(addr);
      bool skip = false;
      if (!intra) {
        if (W) {
          skip = ch_->skip(0) != 0;
          if (cabac_)
            mb_skip_flag(skip);
          else if (skip)
            run++;
        } else {
          if (cabac_) {
            skip = mb_skip_flag(0);
          } else {
            if (expect_run) {
              uint32_t r = br_->ue();
              if (r > (uint32_t)size_) fail("mb_skip_run too large");
              run = (int)r;
              expect_run = false;
            }
            skip = run > 0;
            run -= skip;
          }
          ch_->skip(skip);
        }
      }
      if (skip) {
        set_skip();
      } else {
        if (W && !cabac_ && !intra) {
          bw_->ue((uint32_t)run);
          run = 0;
        }
        coded_mb(addr, prev_skipped);
      }
      prev_skipped = skip;
      // end of the slice: with CABAC after each macroblock (pair, MBAFF),
      // with CAVLC where the data ends
      if (!cabac_ || !mbaff_ || (addr & 1)) {
        bool certain = next_mb(addr) >= size_;
        bool end;
        if (W) {
          end = ch_->end(0, certain) != 0;
          if (cabac_)
            term(end);
          else if (end && run)
            bw_->ue((uint32_t)run);
        } else {
          if (cabac_) {
            end = term(0);
          } else {
            end = skip && run ? false : !br_->more_rbsp_data();
            if (!end && !skip && !intra) expect_run = true;
          }
          ch_->end(end, certain);
        }
        if (end) break;
      }
      addr = next_mb(addr);
    }
  }

  void coded_mb(int addr, bool prev_skipped) {
    ch_->mb_begin();
    if (mbaff_ && (!(addr & 1) || prev_skipped)) {
      int f = mb_field_decoding_flag(W ? ch_->field(0) : 0);
      if (!W) ch_->field(f);
      set_field(f);
    }
    macroblock();
    ch_->mb_end();
  }

  void start_mb(int addr) {
    if (addr >= size_) fail("too many macroblocks in the slice");
    cur_addr_ = addr;
    if (mbaff_) {
      if (!(addr & 1)) {
        // a new pair: neighbouring pairs (6.4.10), and the inferred field
        // decoding mode until mb_field_decoding_flag is decoded (7.4.4)
        int pair = addr / 2;
        left_ = pair % width_ ? avail(2 * (pair - 1)) : nullptr;
        above_ = pair >= width_ ? avail(2 * (pair - width_)) : nullptr;
        pair_field_ = left_ ? left_->field : above_ ? above_->field : 0;
      }
    } else {
      left_ = addr % width_ ? avail(addr - 1) : nullptr;
      above_ = addr >= width_ ? avail(addr - width_) : nullptr;
    }
    cur_ = &store_.mbs[addr];
    memset(cur_, 0, sizeof *cur_);
    cur_->slice = slice_id_;
    cur_->field = (uint8_t)(mbaff_ && pair_field_);
    update_nb();
  }

  void set_skip() {
    cur_->skip = 1;
    cur_->direct16 = stype_ == kB;
    last_qpd_ = 0;
  }

  // mb_field_decoding_flag decoded for the pair
  void set_field(int f) {
    pair_field_ = f;
    cur_->field = (uint8_t)f;
    if (cur_addr_ & 1) (cur_ - 1)->field = (uint8_t)f;  // skipped top macroblock
    update_nb();
  }

  void update_nb() {
    int xW, yW;
    mb_a_ = nb_loc(-1, 0, 16, 16, xW, yW);
    mb_b_ = nb_loc(0, -1, 16, 16, xW, yW);
  }

  int next_mb(int addr) const {
    if (group_.empty()) return addr + 1;
    int i = addr + 1;
    while (i < size_ && group_[i] != group_[addr]) i++;
    return i;
  }

  // --- macroblock layer (7.3.5)
  //
  // Each syntax element goes through the channel: parsing, after it is
  // read; writing, before it is written.
  int mb_type_v() {
    int v = mb_type(W ? ch_->mb_type(0) : 0);
    if (!W) ch_->mb_type(v);
    return v;
  }
  int t8x8_v() {
    int v = transform_size_8x8_flag(W ? ch_->t8x8(0) : 0);
    if (!W) ch_->t8x8(v);
    return v;
  }
  int ipm_v(int blk) {
    int v = intra_pred_mode(W ? ch_->ipm(blk, 0) : 0);
    if (!W) ch_->ipm(blk, v);
    return v;
  }
  int cpred_v() {
    int v = intra_chroma_pred_mode(W ? ch_->cpred(0) : 0);
    if (!W) ch_->cpred(v);
    return v;
  }
  int sub_v(int i) {
    int v = sub_mb_type(W ? ch_->sub(i, 0) : 0);
    if (!W) ch_->sub(i, v);
    return v;
  }
  int cbp_v(bool intra_nxn) {
    int v = coded_block_pattern(W ? ch_->cbp(0) : 0, intra_nxn);
    if (!W) ch_->cbp(v);
    return v;
  }
  int qpd_v() {
    int v = mb_qp_delta(W ? ch_->qpd(0) : 0);
    if (!W) ch_->qpd(v);
    return v;
  }

  void macroblock() {
    MbState& m = *cur_;
    int t = mb_type_v();
    int itype = -1, ptype = -1, btype = -1;  // intra type in I slice numbering
    bool si = false;
    switch (stype_) {
      case kI:
        itype = t;
        break;
      case kSI:
        if (t == 0)
          si = true;
        else
          itype = t - 1;
        break;
      case kB:
        if (t < 23)
          btype = t;
        else
          itype = t - 23;
        break;
      default:
        if (t < 5)
          ptype = t;
        else
          itype = t - 5;
    }
    m.intra = itype >= 0 || si;
    m.si = si;
    if (itype == 25) {  // I_PCM
      m.pcm = 1;
      m.cbp = 0x2f;
      memset(m.nz, 16, sizeof m.nz);
      memset(m.cbf_dc, 1, sizeof m.cbf_dc);
      pcm_samples();
      last_qpd_ = 0;
      return;
    }
    m.i16 = itype >= 1;
    m.inxn = itype == 0;
    m.direct16 = btype == 0;
    bool no_sub_lt8 = true;
    int t8 = 0;
    if (ptype == 3 || ptype == 4 || btype == 22) {
      sub_mb_pred(ptype, no_sub_lt8);
    } else if (m.intra) {
      if (pps_.transform_8x8_mode_flag && m.inxn) t8 = t8x8_v();
      if (m.inxn || si) {
        for (int i = 0; i < (t8 ? 4 : 16); i++) ipm_v(i);
      }
      if (chroma_ == 1 || chroma_ == 2) m.cpred = (uint8_t)cpred_v();
    } else {
      mb_pred_inter(ptype, btype);
    }
    int cbp;
    if (!m.i16) {
      cbp = cbp_v(m.inxn || si);
      if ((cbp & 15) && pps_.transform_8x8_mode_flag && !m.inxn && no_sub_lt8 &&
          (btype != 0 || sps_.direct_8x8_inference_flag))
        t8 = t8x8_v();
    } else {
      cbp = ((itype - 1) / 12 ? 15 : 0) | ((itype - 1) / 4 % 3) << 4;
    }
    m.cbp = (uint8_t)cbp;
    m.t8x8 = (uint8_t)t8;
    int qpd = 0;
    if (cbp || m.i16) {
      qpd = qpd_v();
      residual(cbp, t8);
    }
    last_qpd_ = qpd;
  }

  void pcm_samples() {
    int nc = chroma_ ? 2 * (16 / (chroma_ == 3 ? 1 : 2)) * (16 / (chroma_ == 1 ? 2 : 1)) : 0;
    int n = 256 + nc;
    // the alignment bits relative to x264's, whose last one is its padding bit
    // (writing without the bins, nb comes from the channel)
    int nb = (8 - (int)((W ? bw_->pos() : br_->pos()) & 7)) & 7;
    int align = 0;
    pcm_.resize(n);
    if (!W) {
      align = (int)br_->u(nb) ^ (cabac_ && nb ? h_.pad : 0);
      for (int i = 0; i < n; i++) pcm_[i] = (int)br_->u(i < 256 ? sps_.bit_depth_luma : sps_.bit_depth_chroma);
      ch_->pcm(align, nb, pcm_.data(), n);
    } else {
      ch_->pcm(align, nb, pcm_.data(), n);
      if (align < 0 || align >= (1 << nb)) fail("bad pcm alignment bits");
      bw_->put((uint32_t)(align ^ (cabac_ && nb ? h_.pad : 0)), nb);
      for (int i = 0; i < n; i++) {
        int depth = i < 256 ? sps_.bit_depth_luma : sps_.bit_depth_chroma;
        if (pcm_[i] < 0 || pcm_[i] >= (1 << depth)) fail("PCM sample out of range");
        bw_->put((uint32_t)pcm_[i], depth);
      }
    }
    // (the walk without its bins writes no CABAC data, nor whole bytes)
    if (cabac_) {
      if (!W)
        dec_init();
      else if (!nobins_)
        enc_.init(bw_);
    }
  }

  void mb_pred_inter(int ptype, int btype) {
    const Part* parts;
    int n, pred[2];
    if (ptype >= 0) {
      n = ptype ? 2 : 1;
      parts = ptype == 0 ? kParts16x16 : ptype == 1 ? kParts16x8 : kParts8x16;
      pred[0] = pred[1] = 1;
    } else if (btype == 0) {
      return;  // B_Direct_16x16
    } else if (btype <= 3) {
      n = 1;
      parts = kParts16x16;
      pred[0] = btype;
    } else {
      n = 2;
      parts = btype & 1 ? kParts8x16 : kParts16x8;
      pred[0] = kBPartPred[(btype - 4) / 2][0];
      pred[1] = kBPartPred[(btype - 4) / 2][1];
    }
    for (int l = 0; l < 2; l++)
      for (int i = 0; i < n; i++)
        if (pred[i] & (1 << l)) code_ref(l, parts[i], num_ref_[l] > 1 || cur_->field);
    for (int l = 0; l < 2; l++)
      for (int i = 0; i < n; i++)
        if (pred[i] & (1 << l)) code_mvd(l, parts[i]);
  }

  void sub_mb_pred(int ptype, bool& no_sub_lt8) {
    int sub[4];
    for (int i = 0; i < 4; i++) sub[i] = sub_v(i);
    bool b = stype_ == kB;
    auto pred = [&](int i) { return b ? kBSubPred[sub[i]] : 1; };
    auto shape = [&](int i) { return b ? kBSubShape[sub[i]] : sub[i]; };
    for (int l = 0; l < 2; l++)
      for (int i = 0; i < 4; i++)
        if (pred(i) & (1 << l)) {
          Part p = {(i & 1) * 2, (i >> 1) * 2, 2, 2};
          code_ref(l, p, (num_ref_[l] > 1 || cur_->field) && ptype != 4);
        }
    for (int l = 0; l < 2; l++)
      for (int i = 0; i < 4; i++)
        if (pred(i) & (1 << l)) {
          int s = shape(i);
          for (int j = 0; j < kNumSubParts[s]; j++) {
            Part p = kSubParts[s][j];
            p.x += (i & 1) * 2;
            p.y += (i >> 1) * 2;
            code_mvd(l, p);
          }
        }
    for (int i = 0; i < 4; i++) {
      if (b && sub[i] == 0) {
        if (!sps_.direct_8x8_inference_flag) no_sub_lt8 = false;
      } else if (kNumSubParts[shape(i)] > 1) {
        no_sub_lt8 = false;
      }
    }
  }

  void code_ref(int list, const Part& p, bool coded) {
    int v = 0;
    if (coded) {
      v = ref_idx(list, p.x, p.y, W ? ch_->ref(list, p.x, p.y, p.w, p.h, 0) : 0);
      if (!W) ch_->ref(list, p.x, p.y, p.w, p.h, v);
    }
    for (int y = p.y; y < p.y + p.h; y += 2)
      for (int x = p.x; x < p.x + p.w; x += 2) cur_->ref[list][(y >> 1) * 2 + (x >> 1)] = (int8_t)v;
  }

  void code_mvd(int list, const Part& p) {
    int mx = mvd(list, 0, p.x, p.y, W ? ch_->mvd(list, 0, p.x, p.y, p.w, p.h, 0) : 0);
    if (!W) ch_->mvd(list, 0, p.x, p.y, p.w, p.h, mx);
    int my = mvd(list, 1, p.x, p.y, W ? ch_->mvd(list, 1, p.x, p.y, p.w, p.h, 0) : 0);
    if (!W) ch_->mvd(list, 1, p.x, p.y, p.w, p.h, my);
    uint8_t ax = (uint8_t)std::min(abs(mx), 127), ay = (uint8_t)std::min(abs(my), 127);
    for (int y = p.y; y < p.y + p.h; y++)
      for (int x = p.x; x < p.x + p.w; x++) {
        cur_->mvd[list][y * 4 + x][0] = ax;
        cur_->mvd[list][y * 4 + x][1] = ay;
      }
  }

  // residual (7.3.5.3)
  void residual(int cbp, int t8) {
    MbState& m = *cur_;
    residual_luma(0, cbp, t8);
    if (chroma_ == 3) {  // Cb and Cr coded like luma
      residual_luma(1, cbp, t8);
      residual_luma(2, cbp, t8);
    } else if (chroma_) {
      int c[16];
      if (cbp >> 4) {
        for (int comp = 0; comp < 2; comp++) {
          block(3, comp, c, ch_blocks_, cabac_ ? cbf_inc_dc(1 + comp) : chroma_ == 1 ? -1 : -2);
          m.cbf_dc[1 + comp] = count_nonzero(c, ch_blocks_) != 0;
        }
      }
      if ((cbp >> 4) == 2) {
        for (int comp = 0; comp < 2; comp++)
          for (int b = 0; b < ch_blocks_; b++) {
            int x = b & 1, y = b >> 1;
            int ctx = cabac_ ? cbf_inc_chroma_ac(comp, x, y) : nc_chroma(comp, x, y);
            m.nz[1 + comp][y * 2 + x] = (uint8_t)block(4, comp * ch_blocks_ + b, c, 15, ctx);
          }
      }
    }
  }

  // residual_luma(): the blocks of Y, or of Cb or Cr with 4:4:4
  void residual_luma(int plane, int cbp, int t8) {
    MbState& m = *cur_;
    uint8_t* nz = m.nz[plane];
    int c[64];
    if (m.i16) {
      block(kCatDc[plane], 0, c, 16, cabac_ ? cbf_inc_dc(plane) : nc_luma(plane, 0, 0));
      m.cbf_dc[plane] = count_nonzero(c, 16) != 0;
    }
    for (int b8 = 0; b8 < 4; b8++) {
      if (!(cbp & (1 << b8))) continue;
      int x0 = (b8 & 1) * 2, y0 = (b8 >> 1) * 2;
      if (t8 && cabac_) {
        // coded_block_flag only with 4:4:4
        block(kCat8x8[plane], b8, c, 64, chroma_ == 3 ? cbf_inc_8x8(plane, b8) : -1);
        nz[y0 * 4 + x0] = nz[y0 * 4 + x0 + 1] = nz[y0 * 4 + x0 + 4] = nz[y0 * 4 + x0 + 5] =
            (uint8_t)count_nonzero(c, 64);
      } else if (t8) {
        // CAVLC: four 4x4 blocks with interleaved coefficients
        if (W) ch_->block(kCat8x8[plane], b8, c, 64, false);
        for (int b4 = 0; b4 < 4; b4++) {
          int c4[16];
          for (int i = 0; i < 16; i++) c4[i] = c[4 * i + b4];
          int x = x0 + (b4 & 1), y = y0 + (b4 >> 1);
          int nc = nc_luma(plane, x, y);
          int tc = W ? cavlc_write_block(*bw_, nc, 16, c4) : cavlc_read_block(*br_, nc, 16, c4);
          if (!W)
            for (int i = 0; i < 16; i++) c[4 * i + b4] = c4[i];
          nz[y * 4 + x] = (uint8_t)tc;
        }
        if (!W) ch_->block(kCat8x8[plane], b8, c, 64, false);
      } else {
        for (int b4 = 0; b4 < 4; b4++) {
          int blk = b8 * 4 + b4, x = blk_x(blk), y = blk_y(blk);
          int ctx = cabac_ ? cbf_inc_luma(plane, x, y) : nc_luma(plane, x, y);
          nz[y * 4 + x] = (uint8_t)(m.i16 ? block(kCatAc[plane], blk, c, 15, ctx)
                                          : block(kCat4x4[plane], blk, c, 16, ctx));
        }
      }
    }
  }

  // a block of coefficients; ctx is the coded_block_flag ctxIdxInc with
  // CABAC (-1: no coded_block_flag), nC with CAVLC
  int block(int cat, int idx, int* c, int n, int ctx) {
    bool coded = cabac_ && ctx < 0;
    if (W) ch_->block(cat, idx, c, n, coded);
    int k;
    if (cabac_)
      k = cabac_residual(cat, c, n, ctx);
    else if (W)
      k = cavlc_write_block(*bw_, ctx, n, c);
    else
      k = cavlc_read_block(*br_, ctx, n, c);
    if (!W) ch_->block(cat, idx, c, n, coded);
    return k;
  }

  // --- syntax elements
  int mb_skip_flag(int v) {
    int inc = (mb_a_ && !mb_a_->skip) + (mb_b_ && !mb_b_->skip);
    return bin((stype_ == kB ? 24 : 11) + inc, v);
  }

  int mb_field_decoding_flag(int v) {
    if (W && (v < 0 || v > 1)) fail("missing or bad mb_field_decoding_flag (fld row)");
    if (!cabac_) return u(1, v);
    return bin(70 + (left_ && left_->field) + (above_ && above_->field), v);
  }

  int mb_type(int v) {
    if (!cabac_) return ue(v, stype_ == kI ? 25 : stype_ == kSI ? 26 : stype_ == kB ? 48 : 30);
    switch (stype_) {
      case kI:
        return mb_type_i(v, 3, true);
      case kSI: {
        if (W && (v < 0 || v > 26)) fail("mb_type out of range");
        int inc = (mb_a_ && !mb_a_->si) + (mb_b_ && !mb_b_->si);
        if (!bin(inc, v != 0)) return 0;
        return 1 + mb_type_i(v - 1, 3, true);
      }
      case kB: {
        if (W && (v < 0 || v > 48)) fail("mb_type out of range");
        int inc = (mb_a_ && !mb_a_->direct16) + (mb_b_ && !mb_b_->direct16);
        if (!bin(27 + inc, v != 0)) return 0;
        if (!bin(30, v >= 3)) return 1 + bin(32, v == 2);
        int bits = v <= 10 ? v - 3 : v == 11 ? 14 : v == 22 ? 15 : v >= 23 ? 13 : (v + 4) >> 1;
        int x = bin(31, bits >> 3 & 1) << 3;
        x |= bin(32, bits >> 2 & 1) << 2;
        x |= bin(32, bits >> 1 & 1) << 1;
        x |= bin(32, bits & 1);
        if (x < 8) return x + 3;
        if (x == 13) return 23 + mb_type_i(v - 23, 32, false);
        if (x == 14) return 11;
        if (x == 15) return 22;
        return ((x << 1) | bin(32, (v + 4) & 1)) - 4;
      }
      default: {
        if (W && (v < 0 || v > 30)) fail("mb_type out of range");
        if (W && v == 4) fail("P_8x8ref0 cannot be coded with CABAC");
        if (bin(14, v >= 5)) return 5 + mb_type_i(v - 5, 17, false);
        int b1 = bin(15, v == 1 || v == 2);
        int b2 = bin(b1 ? 17 : 16, v == 1 || v == 3);
        return b1 ? (b2 ? 1 : 2) : (b2 ? 3 : 0);
      }
    }
  }

  // mb_type of an intra macroblock, I slice numbering: I slices (ctxIdxOffset
  // 3) or the suffix in P (17) and B (32) slices
  int mb_type_i(int v, int off, bool islice) {
    if (W && (v < 0 || v > 25)) fail("mb_type out of range");
    int ctx0 = islice ? off + (mb_a_ && !mb_a_->inxn) + (mb_b_ && !mb_b_->inxn) : off;
    if (!bin(ctx0, v != 0)) return 0;
    if (term(v == 25)) return 25;
    int x = v - 1;
    int luma = bin(off + (islice ? 3 : 1), x / 12);
    int c = x / 4 % 3;
    int chroma = bin(off + (islice ? 4 : 2), c != 0);
    if (chroma) chroma += bin(off + (islice ? 5 : 2), c == 2);
    int p = bin(off + (islice ? 6 : 3), x % 4 >> 1) << 1;
    p |= bin(off + (islice ? 7 : 3), x & 1);
    return 1 + p + 4 * chroma + 12 * luma;
  }

  int sub_mb_type(int v) {
    bool b = stype_ == kB;
    if (!cabac_) return ue(v, b ? 12 : 3);
    if (W && (v < 0 || v > (b ? 12 : 3))) fail("sub_mb_type out of range");
    if (!b) {
      if (bin(21, v == 0)) return 0;
      if (!bin(22, v >= 2)) return 1;
      return bin(23, v == 2) ? 2 : 3;
    }
    if (!bin(36, v != 0)) return 0;
    if (!bin(37, v >= 3)) return 1 + bin(39, v == 2);
    if (bin(38, v >= 7)) {
      if (bin(39, v >= 11)) return 11 + bin(39, v == 12);
      int x = bin(39, (v - 7) >> 1 & 1) << 1;
      return 7 + (x | bin(39, (v - 7) & 1));
    }
    int x = bin(39, (v - 3) >> 1 & 1) << 1;
    return 3 + (x | bin(39, (v - 3) & 1));
  }

  int ref_idx(int list, int x, int y, int v) {
    int range = num_ref_[list] * (1 + cur_->field) - 1;  // field macroblocks of MBAFF frames: twice
    if (!cabac_) return te(v, range);
    if (W && (v < 0 || v > range)) fail("ref_idx out of range");
    int inc = ref_inc(list, x, y);
    int n = 0;
    while (bin(54 + inc, n < v)) {
      inc = n ? 5 : 4;
      if (++n > range) fail("ref_idx out of range");
    }
    return n;
  }

  // ctxIdxInc of the first bin of ref_idx_lX of the partition at the 4x4 block (x, y)
  int ref_inc(int list, int x, int y) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      int i;
      const MbState* n = k ? nb4(x, y - 1, i) : nb4(x - 1, y, i);
      // refIdxZeroFlagN: a field neighbour of a frame macroblock refers to fields
      if (n && n->ref[list][(i >> 3) * 2 + ((i & 3) >> 1)] > (!cur_->field && n->field)) inc += 1 + k;
    }
    return inc;
  }

  // ctxIdxInc of the first bin of mvd_lX[comp] of the partition at the 4x4 block (x, y)
  int mvd_inc(int list, int comp, int x, int y) const {
    int ia, ib;
    const MbState* a = nb4(x - 1, y, ia);
    const MbState* b = nb4(x, y - 1, ib);
    int sum = 0;
    for (int k = 0; k < 2; k++) {
      const MbState* n = k ? b : a;
      if (!n) continue;
      int m = n->mvd[list][k ? ib : ia][comp];
      // vertical components of frame and field macroblocks of MBAFF frames
      if (comp && !cur_->field && n->field) m *= 2;
      if (comp && cur_->field && !n->field) m /= 2;
      sum += m;
    }
    return sum < 3 ? 0 : sum > 32 ? 2 : 1;
  }

  int mvd(int list, int comp, int x, int y, int v) {
    const int lim = 1 << 24;
    if (!cabac_) return se(v, lim);
    if (W && (v < -lim || v > lim)) fail("mvd out of range");
    int base = comp ? 47 : 40;
    int av = abs(v);
    if (!bin(base + mvd_inc(list, comp, x, y), av > 0)) return 0;
    int n = 1, ctx = base + 3;
    while (n < 9 && bin(ctx, av > n)) {
      if (n < 4) ctx++;
      n++;
    }
    if (n >= 9) n += exp_golomb_bypass(3, av - 9);
    if (n > lim) fail("mvd out of range");
    return bypass(v < 0) ? -n : n;
  }

  int intra_pred_mode(int v) {
    if (W && (v < -1 || v > 7)) fail("intra prediction mode out of range");
    if (!cabac_) {
      if (u(1, v < 0)) return -1;
      return u(3, v);
    }
    if (bin(68, v < 0)) return -1;
    int r = bin(69, v & 1);
    r |= bin(69, v >> 1 & 1) << 1;
    r |= bin(69, v >> 2 & 1) << 2;
    return r;
  }

  int intra_chroma_pred_mode(int v) {
    if (!cabac_) return ue(v, 3);
    if (W && (v < 0 || v > 3)) fail("intra_chroma_pred_mode out of range");
    auto cond = [](const MbState* n) { return n && n->intra && !n->pcm && n->cpred != 0; };
    if (!bin(64 + cond(mb_a_) + cond(mb_b_), v > 0)) return 0;
    if (!bin(67, v > 1)) return 1;
    return bin(67, v > 2) ? 3 : 2;
  }

  int coded_block_pattern(int v, bool intra_nxn) {
    bool has_chroma = chroma_ == 1 || chroma_ == 2;
    if (W && (v < 0 || (v & 15) > 15 || (v >> 4) > (has_chroma ? 2 : 0))) fail("coded_block_pattern out of range");
    if (!cabac_) {
      const uint8_t* tab =
          has_chroma ? (intra_nxn ? kCbpIntra : kCbpInter) : (intra_nxn ? kCbpIntraMono : kCbpInterMono);
      int n = has_chroma ? 48 : 16;
      int code = 0;
      if (W)
        while (tab[code] != v) code++;
      return tab[ue(code, n - 1)];
    }
    int luma = 0;
    for (int b8 = 0; b8 < 4; b8++) luma |= bin(73 + cbp_luma_inc(b8, luma), v >> b8 & 1) << b8;
    if (!has_chroma) return luma;
    int c = 0;
    if (bin(77 + cbp_chroma_inc(0), (v >> 4) != 0)) c = 1 + bin(81 + cbp_chroma_inc(1), (v >> 4) == 2);
    return luma | c << 4;
  }

  // ctxIdxInc of the luma bit of the 8x8 block b8 of coded_block_pattern,
  // luma having the bits of the blocks before it (9.3.3.1.1.4)
  int cbp_luma_inc(int b8, int luma) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      int x = (b8 & 1) * 8 - !k, y = (b8 >> 1) * 8 - k, xW, yW;
      const MbState* n = nb_loc(x, y, 16, 16, xW, yW);
      int b8n = (yW >> 3) * 2 + (xW >> 3);
      int cbp = n == cur_ ? luma : n ? n->cbp : 0;
      if (n && !(n != cur_ && n->pcm) && !(cbp >> b8n & 1)) inc += 1 + k;
    }
    return inc;
  }

  // ctxIdxInc of the first (bin 0: chroma not 0) or second chroma bin (1: chroma 2)
  int cbp_chroma_inc(int bin) const {
    return (mb_a_ && (mb_a_->cbp >> 4) > bin) + 2 * (mb_b_ && (mb_b_->cbp >> 4) > bin);
  }

  int transform_size_8x8_flag(int v) {
    if (!cabac_) return u(1, v);
    if (W && (v < 0 || v > 1)) fail("transform_size_8x8_flag out of range");
    return bin(399 + (mb_a_ && mb_a_->t8x8) + (mb_b_ && mb_b_->t8x8), v);
  }

  int mb_qp_delta(int v) {
    const int lim = 128;
    if (!cabac_) return se(v, lim);
    if (W && (v < -lim || v > lim)) fail("mb_qp_delta out of range");
    int k = v > 0 ? 2 * v - 1 : -2 * v;
    int n = 0, ctx = 60 + (last_qpd_ != 0);
    while (bin(ctx, n < k)) {
      ctx = n ? 63 : 62;
      if (++n > 2 * lim) fail("mb_qp_delta out of range");
    }
    return n & 1 ? (n + 1) / 2 : -(n / 2);
  }

  // residual_block_cabac (7.3.5.3.3); cbf_inc < 0: no coded_block_flag
  int cabac_residual(int cat, int* c, int n, int cbf_inc) {
    if (!W)
      for (int i = 0; i < n; i++) c[i] = 0;
    int nonzero = W ? count_nonzero(c, n) : 0;
    if (cbf_inc >= 0) {
      if (!bin(kCbfBase[cat] + cbf_inc, nonzero != 0)) {
        for (int i = 0; i < n; i++) c[i] = 0;
        return 0;
      }
    } else if (W && !nonzero) {
      fail("8x8 block without coefficients cannot be coded with CABAC");
    }
    int last = n - 1;
    if (W)
      while (!c[last]) last--;
    int sig_base = kSigBase[field_mb()][cat], last_base = kLastBase[field_mb()][cat], abs_base = kAbsBase[cat];
    bool b8x8 = cat == 5 || cat == 9 || cat == 13;
    int numc8x8 = chroma_ == 2 ? 2 : 1;
    bool sig[64];
    int i;
    for (i = 0; i < n - 1; i++) {
      int inc_sig, inc_last;
      if (cat == 3) {
        inc_sig = inc_last = std::min(i / numc8x8, 2);
      } else if (b8x8) {
        inc_sig = field_mb() ? kSig8x8Field[i] : kSig8x8Frame[i];
        inc_last = kLast8x8[i];
      } else {
        inc_sig = inc_last = i;
      }
      sig[i] = bin(sig_base + inc_sig, W ? c[i] != 0 : 0);
      if (sig[i] && bin(last_base + inc_last, W ? i == last : 0)) break;
    }
    if (i == n - 1) sig[i] = true;  // no last flag: the last coefficient is significant
    int eq1 = 0, gt1 = 0, count = 0;
    for (; i >= 0; i--) {
      if (!sig[i]) {
        c[i] = 0;
        continue;
      }
      int a = W ? abs(c[i]) - 1 : 0;
      int v = bin(abs_base + (gt1 ? 0 : std::min(4, 1 + eq1)), a > 0);
      if (v) {
        int ctx = abs_base + 5 + std::min(4 - (cat == 3), gt1);
        while (v < 14 && bin(ctx, a > v)) v++;
        if (v == 14) v += exp_golomb_bypass(0, a - 14);
      }
      int s = bypass(W ? c[i] < 0 : 0);
      c[i] = s ? -(v + 1) : v + 1;
      if (v == 0)
        eq1++;
      else
        gt1++;
      count++;
    }
    return count;
  }

  // CAVLC nC (9.2.1)
  int nc_luma(int plane, int x, int y) const {
    int ia, ib;
    const MbState* a = nb4(x - 1, y, ia);
    const MbState* b = nb4(x, y - 1, ib);
    if (a && b) return (a->nz[plane][ia] + b->nz[plane][ib] + 1) >> 1;
    if (a) return a->nz[plane][ia];
    if (b) return b->nz[plane][ib];
    return 0;
  }

  int nc_chroma(int comp, int x, int y) const {
    int ia, ib;
    const MbState* a = nbc(x - 1, y, ia);
    const MbState* b = nbc(x, y - 1, ib);
    if (a && b) return (a->nz[1 + comp][ia] + b->nz[1 + comp][ib] + 1) >> 1;
    if (a) return a->nz[1 + comp][ia];
    if (b) return b->nz[1 + comp][ib];
    return 0;
  }

  // coded_block_flag ctxIdxInc (9.3.3.1.1.9)
  int cbf_inc_luma(int plane, int x, int y) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      int i;
      const MbState* n = k ? nb4(x, y - 1, i) : nb4(x - 1, y, i);
      int cond;
      if (!n)
        cond = cur_->intra;
      else if (n->pcm)
        cond = 1;
      else if (!((n->cbp >> ((i >> 3) * 2 + ((i & 3) >> 1))) & 1))
        cond = 0;
      else  // of the 4x4 block, or of the 8x8 block when transform_size_8x8_flag
        cond = n->nz[plane][i] != 0;
      inc += cond << k;
    }
    return inc;
  }

  // coded_block_flag of an 8x8 block (4:4:4 only)
  int cbf_inc_8x8(int plane, int b8) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      int xW, yW;
      const MbState* n = nb_loc((b8 & 1) * 8 - !k, (b8 >> 1) * 8 - k, 16, 16, xW, yW);
      int cond;
      if (!n)
        cond = cur_->intra;
      else if (n->pcm)
        cond = 1;
      else if (!((n->cbp >> ((yW >> 3) * 2 + (xW >> 3))) & 1) || !n->t8x8)
        cond = 0;
      else
        cond = n->nz[plane][(yW >> 2) * 4 + (xW >> 2)] != 0;
      inc += cond << k;
    }
    return inc;
  }

  int cbf_inc_dc(int comp) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      const MbState* n = k ? mb_b_ : mb_a_;
      int cond;
      if (!n)
        cond = cur_->intra;
      else if (n->pcm)
        cond = 1;
      else if (comp == 0 || chroma_ == 3)
        cond = n->i16 && n->cbf_dc[comp];
      else
        cond = (n->cbp >> 4) && n->cbf_dc[comp];
      inc += cond << k;
    }
    return inc;
  }

  int cbf_inc_chroma_ac(int comp, int x, int y) const {
    int inc = 0;
    for (int k = 0; k < 2; k++) {
      int i;
      const MbState* n = k ? nbc(x, y - 1, i) : nbc(x - 1, y, i);
      int cond;
      if (!n)
        cond = cur_->intra;
      else if (n->pcm)
        cond = 1;
      else
        cond = (n->cbp >> 4) == 2 && n->nz[1 + comp][i] != 0;
      inc += cond << k;
    }
    return inc;
  }

  const bool W;
  const bool nobins_;  // W, but the bins only walked, not coded (Chan::nobins)
  const SliceHeader& h_;
  const Sps& sps_;
  const Pps& pps_;
  MbStore& store_;
  BitReader* br_;
  BitWriter* bw_;
  Chan* ch_;
  BinRec* rec_;                    // parsing: the bins recorded (Chan::rec)
  const uint8_t* chk_ = nullptr;   // writing without the bins: the next one recorded

  bool cabac_ = false, field_ = false, mbaff_ = false;
  int stype_ = 0, chroma_ = 1, ch_h_ = 2, ch_blocks_ = 4;
  int width_ = 0, size_ = 0;
  int num_ref_[2] = {0, 0};
  uint32_t slice_id_ = 0;
  std::vector<uint8_t> group_;  // mbToSliceGroupMap, empty without FMO
  int cur_addr_ = 0;
  MbState* cur_ = nullptr;
  const MbState* mb_a_ = nullptr;  // macroblocks A and B (6.4.10.1)
  const MbState* mb_b_ = nullptr;
  const MbState* left_ = nullptr;  // left and above macroblock, or top
  const MbState* above_ = nullptr; // macroblock of the left and above pair
  int pair_field_ = 0;             // MBAFF: field decoding mode of the pair
  std::vector<int> pcm_;
  int last_qpd_ = 0;
  CabacCtx ctx_[kNumCtx];
  CabacDecoder dec_;
  CabacEncoder enc_;
};

void read_slice_data(const SliceHeader& h, MbStore& store, BitReader& br, Chan& ch) {
  SliceCoder(h, store, &br, nullptr, ch).run();
}

void write_slice_data(const SliceHeader& h, MbStore& store, Chan& ch, BitWriter& bw) {
  SliceCoder(h, store, nullptr, &bw, ch).run();
}

}  // namespace h264p
