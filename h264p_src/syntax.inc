// Symmetric coding of syntax elements: the same code parses a bitstream,
// handing the values to a channel (chan.inc), and writes a bitstream from
// the values a channel gives.

namespace h264p {

// What the walks of the headers (Syn) and of the slice data (SliceCoderT,
// slice.inc) share, D the walk: its failure, and the fixed-length and
// Exp-Golomb codes in its direction -- writing to D::bw_ (D::W), else
// parsing D::br_ -- their values range-checked either way. (D::W is a
// member of Syn, a constant of the slice walk's mode.)
template <class D>
struct WalkCodes {
  // an inconsistency the walk found (err_raise(), err.inc): parsing, the
  // stream cannot be parsed; writing, the channel says what it means
  [[noreturn]] void fail(const char* fmt, ...) {
    char why[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(why, sizeof why, fmt, ap);
    va_end(ap);
    if (d().W) d().ch_->fail(why);
    err_raise_s(why);
  }
  int64_t check(int64_t v, int64_t lo, int64_t hi) {
    if (v < lo || v > hi) fail("value %" PRId64 " out of range", v);
    return v;
  }
  // u(n), ue(v) and se(v): parsing, the value read (v unused); writing, v
  // written; within [lo, hi] (u(n): its n bits)
  int64_t code_u(int n, int64_t v) {
    if (!d().W) return d().br_->u(n);
    check(v, 0, ((int64_t)1 << n) - 1);
    d().bw_->put((uint64_t)v, n);
    return v;
  }
  int64_t code_ue(int64_t v, int64_t lo, int64_t hi) {
    if (!d().W) return check(d().br_->ue(), lo, hi);
    check(v, lo, hi);
    d().bw_->ue((uint64_t)v);
    return v;
  }
  int64_t code_se(int64_t v, int64_t lo, int64_t hi) {
    if (!d().W) return check(d().br_->se(), lo, hi);
    check(v, lo, hi);
    d().bw_->se(v);
    return v;
  }

 private:
  D& d() { return *static_cast<D*>(this); }
};

// Header syntax: each syntax element, or group of elements, is one named
// group of values.
struct Syn : WalkCodes<Syn> {
  // parsing br, or (none) writing bw, with ch
  void Init(BitReader* br, BitWriter* bw, Chan* ch) {
    W = !br;
    br_ = br;
    bw_ = br ? nullptr : bw;
    ch_ = ch;
    same_way();
  }

  bool W;  // writing a bitstream
  BitReader* br_;
  BitWriter* bw_;
  Chan* ch_;

  // a group with one value
  int64_t u(const char* name, int n) {
    begin(name);
    int64_t v = vu(n);
    end();
    return v;
  }
  int64_t ue(const char* name, int64_t lo, int64_t hi) {
    begin(name);
    int64_t v = vue(lo, hi);
    end();
    return v;
  }
  int64_t se(const char* name, int64_t lo, int64_t hi) {
    begin(name);
    int64_t v = vse(lo, hi);
    end();
    return v;
  }

  // a group with several values
  void begin(const char* name) { ch_->hbegin(name); }

  // A value through the channel: parsing, handed to it after code() read
  // it; writing, the channel's, coded by code()
  template <class F>
  int64_t xval(F code) {
    int64_t v = code(W ? ch_->hval(0) : 0);
    if (!W) ch_->hval(v);
    return v;
  }
  int64_t vu(int n) {
    ch_->hbits(n);
    return xval([&](int64_t v) { return code_u(n, v); });
  }
  int64_t vue(int64_t lo, int64_t hi) {
    return xval([&](int64_t v) { return code_ue(v, lo, hi); });
  }
  int64_t vse(int64_t lo, int64_t hi) {
    return xval([&](int64_t v) { return code_se(v, lo, hi); });
  }

  // SEI payloadType/payloadSize coding
  int64_t vff() {
    if (!W) {
      int64_t v = 0;
      uint32_t b;
      while ((b = br_->u(8)) == 255)
        if ((v += 255) > (1 << 24)) fail("invalid SEI message header");
      if ((v += b) > (1 << 24)) fail("invalid SEI message header");
      ch_->hval(v);
      return v;
    }
    int64_t v = check(ch_->hval(0), 0, 1 << 24);
    for (int64_t x = v;; x -= 255) {
      bw_->put(x < 255 ? x : 255, 8);
      if (x < 255) break;
    }
    return v;
  }

  // bytes, a piece at a time
  void vbytes(size_t nbytes) {
    uint8_t b[256];
    for (size_t i = 0; i < nbytes; i += sizeof b) {
      size_t n = nbytes - i < sizeof b ? nbytes - i : sizeof b;
      if (!W) {
        for (size_t k = 0; k < n; k++) b[k] = (uint8_t)br_->u(8);
        ch_->hbytes(b, n);
        continue;
      }
      ch_->hbytes(b, n);
      for (size_t k = 0; k < n; k++) bw_->put(b[k], 8);
    }
  }

  void end() { ch_->hend(); }

  // a value that is not coded as such: parsing, v goes to the channel;
  // writing, the value of the channel is returned
  int64_t vval(int64_t v, int64_t lo, int64_t hi) {
    return xval([&](int64_t x) { return check(W ? x : v, lo, hi); });
  }

  // more_rbsp_data() when parsing; when writing, what the channel says (the
  // model codes it: `next` names what would follow)
  bool more_data(const char* next) {
    if (!W) return ch_->hmore(next, br_->more_rbsp_data());
    return ch_->hmore(next, false);
  }

  // (the walk and the channel go the same way: a channel's direction is
  // the walk's, of the headers as of the slice data)
  void same_way() const {
    if (ch_->W != W) err_raise("internal: the walk and its channel go different ways");
  }
};

struct Sps {
  uint64_t gen;  // the definition it is (ParamSets): 0, none
  int id;
  int profile_idc;
  int chroma_format_idc, separate_colour_plane_flag;
  int bit_depth_luma, bit_depth_chroma;
  int log2_max_frame_num;
  int poc_type, log2_max_poc_lsb, delta_pic_order_always_zero_flag;
  int width_mbs, height_map_units;
  int frame_mbs_only_flag, mb_adaptive_frame_field_flag, direct_8x8_inference_flag;
  // for decoding the pictures (model.inc)
  int max_num_ref_frames, transform_bypass, scaling_matrix;
  int64_t offset_for_non_ref_pic, offset_for_top_to_bottom_field;
  int num_ref_frames_in_poc_cycle;
  int64_t offset_for_ref_frame[255];

  // none, with the values of what an SPS leaves out
  void Init() {
    gen = 0;
    id = profile_idc = 0;
    chroma_format_idc = 1, separate_colour_plane_flag = 0;
    bit_depth_luma = bit_depth_chroma = 8;
    log2_max_frame_num = 4;
    poc_type = 0, log2_max_poc_lsb = 4, delta_pic_order_always_zero_flag = 0;
    width_mbs = height_map_units = 0;
    frame_mbs_only_flag = 1, mb_adaptive_frame_field_flag = direct_8x8_inference_flag = 0;
    max_num_ref_frames = transform_bypass = scaling_matrix = 0;
    offset_for_non_ref_pic = offset_for_top_to_bottom_field = 0;
    num_ref_frames_in_poc_cycle = 0;
  }
  int chroma_array_type() const { return separate_colour_plane_flag ? 0 : chroma_format_idc; }
  int frame_height_mbs() const { return (2 - frame_mbs_only_flag) * height_map_units; }
};

struct Pps {
  uint64_t gen;  // the definition it is (ParamSets): 0, none
  int id, sps_id;
  int entropy_coding_mode_flag;
  int bottom_field_pic_order_in_frame_present_flag;
  int num_slice_groups, slice_group_map_type;
  int run_length_minus1[8], top_left[8], bottom_right[8];  // of the slice groups
  Buf<uint8_t> slice_group_id;                             // of the map units
  int slice_group_change_direction_flag, slice_group_change_rate;
  int num_ref_idx_default[2];
  int weighted_pred_flag, weighted_bipred_idc;
  int pic_init_qp;
  int deblocking_filter_control_present_flag, constrained_intra_pred_flag;
  int redundant_pic_cnt_present_flag;
  int transform_8x8_mode_flag;
  int chroma_qp_index_offset, second_chroma_qp_index_offset, scaling_matrix;

  void Init() {
    slice_group_id.Init();
    reset();
  }
  void Quit() { slice_group_id.Quit(); }
  // none, with the values of what a PPS leaves out (the room for
  // slice_group_id stays)
  void reset() {
    gen = 0;
    id = sps_id = 0;
    entropy_coding_mode_flag = 0;
    bottom_field_pic_order_in_frame_present_flag = 0;
    num_slice_groups = 1, slice_group_map_type = 0;
    for (int g = 0; g < 8; g++) run_length_minus1[g] = top_left[g] = bottom_right[g] = 0;
    slice_group_id.clear();
    slice_group_change_direction_flag = 0, slice_group_change_rate = 1;
    num_ref_idx_default[0] = num_ref_idx_default[1] = 1;
    weighted_pred_flag = weighted_bipred_idc = 0;
    pic_init_qp = 26;
    deblocking_filter_control_present_flag = constrained_intra_pred_flag = 0;
    redundant_pic_cnt_present_flag = 0;
    transform_8x8_mode_flag = 0;
    chroma_qp_index_offset = second_chroma_qp_index_offset = scaling_matrix = 0;
  }
};

// The parameter sets of a stream: the last definition of each id (gen,
// its number in the stream: a slice header's parameter sets are those
// they were while gen stays), and room to parse the next definition in
struct ParamSets {
  Sps sps[32];
  Pps pps[256];
  Sps next_sps;
  Pps next_pps;
  uint64_t gen;  // the definitions so far

  void Init() {
    for (Sps& p : sps) p.Init();
    for (Pps& p : pps) p.Init();
    next_sps.Init();
    next_pps.Init();
    gen = 0;
  }
  void Quit() {
    for (Pps& p : pps) p.Quit();
    next_pps.Quit();
  }
  const Sps* get_sps(int id) const { return sps[id].gen ? &sps[id] : nullptr; }
  const Pps* get_pps(int id) const { return pps[id].gen ? &pps[id] : nullptr; }
  // next_sps (next_pps), parsed, the definition of its id
  void define_sps() {
    next_sps.gen = ++gen;
    sps[next_sps.id] = next_sps;
  }
  void define_pps() {
    next_pps.gen = ++gen;
    Pps& p = pps[next_pps.id];
    char t[sizeof(Pps)];  // (swapped whole: the slot's room for slice_group_id goes to next_pps)
    memcpy(t, (void*)&p, sizeof t);
    memcpy((void*)&p, (void*)&next_pps, sizeof t);
    memcpy((void*)&next_pps, t, sizeof t);
  }
};

enum SliceType { kP = 0, kB = 1, kI = 2, kSP = 3, kSI = 4 };

// a ref_pic_list_modification(): modification_of_pic_nums_idc and its argument
struct Rplm {
  int idc;
  int64_t arg;
};
// a memory_management_control_operation and its n arguments
struct Mmco {
  int64_t op, arg[2];
  int n;
};

struct SliceHeader {
  int nal_unit_type, nal_ref_idc;
  int first_mb, slice_type, pps_id;
  int frame_num, field_pic_flag, bottom_field_flag;
  int direct_spatial_mv_pred_flag;
  int num_ref_idx_active[2];
  int cabac_init_idc;
  int slice_qp_delta;
  int slice_group_change_cycle;
  // for decoding the pictures (model.inc)
  int poc_lsb, long_term_reference_flag, mmco5;
  int64_t idr_pic_id, redundant_pic_cnt;  // (a redundant coded picture's slice: redundant_pic_cnt > 0)
  int64_t delta_poc_bottom, delta_poc[2];
  int nrplm[2];
  Rplm rplm[2][66];  // (65 at most, and one that fails)
  int nmmco;
  Mmco mmco[68];  // (67 at most, and one that fails)
  int luma_log2_wd, chroma_log2_wd;
  int nwt[2];
  int wt[2][32 * 6];  // per reference: luma weight, offset, Cb weight, offset, Cr weight, offset; none: no table
  int disable_deblocking, alpha_offset, beta_offset;
  const Sps* sps;  // its parameter sets (ParamSets')
  const Pps* pps;
  int pad;  // x264's padding bit after CABAC flushes (x264_padding row), not coded

  // all 0: none yet
  void Init() { memset((void*)this, 0, sizeof *this); }
  int type() const { return slice_type % 5; }
};

// A slice's picture: its kind, by which the model keeps its records of the
// last picture of each (History, model.inc): slice type, reference or not,
// frame or top or bottom field, 30 kinds; and its size in macroblocks
inline int pic_kind(const SliceHeader& h) {
  return (h.type() * 2 + (h.nal_ref_idc != 0)) * 3 + (h.field_pic_flag ? 1 + h.bottom_field_flag : 0);
}
inline int pic_mbs(const SliceHeader& h) {
  return h.sps->width_mbs * (h.sps->frame_height_mbs() / (1 + h.field_pic_flag));
}


static int64_t tc_clamp64(int64_t v, int64_t lo, int64_t hi) { return v < lo ? lo : v > hi ? hi : v; }

const int64_t kU32 = 0xFFFFFFFE;  // largest ue(v) value
const int64_t kS32 = 0x7FFFFFFF;  // largest se(v) magnitude
const int kMaxMbs = 139264;       // largest picture size in macroblocks accepted (MaxFS of level 6.2)

// ---------------------------------------------------------------------------

static void scaling_list(Syn& s, const char* name, int size) {
  s.begin(name);  // seq/pic_scaling_list_present_flag, then the delta_scale values
  if (s.vu(1)) {
    int last = 8, next = 8;
    for (int j = 0; j < size; j++) {
      if (next != 0) next = (last + (int)s.vse(-128, 127) + 256) % 256;
      if (next != 0) last = next;
    }
  }
  s.end();
}

static void hrd_parameters(Syn& s) {
  int cnt = (int)s.ue("cpb_cnt_minus1", 0, 31);
  s.u("bit_rate_scale", 4);
  s.u("cpb_size_scale", 4);
  for (int i = 0; i <= cnt; i++) {
    s.begin("cpb");  // bit_rate_value_minus1, cpb_size_value_minus1, cbr_flag
    s.vue(0, kU32);
    s.vue(0, kU32);
    s.vu(1);
    s.end();
  }
  s.u("initial_cpb_removal_delay_length_minus1", 5);
  s.u("cpb_removal_delay_length_minus1", 5);
  s.u("dpb_output_delay_length_minus1", 5);
  s.u("time_offset_length", 5);
}

static void vui_parameters(Syn& s) {
  if (s.u("aspect_ratio_info_present_flag", 1))
    if (s.u("aspect_ratio_idc", 8) == 255) {
      s.u("sar_width", 16);
      s.u("sar_height", 16);
    }
  if (s.u("overscan_info_present_flag", 1)) s.u("overscan_appropriate_flag", 1);
  if (s.u("video_signal_type_present_flag", 1)) {
    s.u("video_format", 3);
    s.u("video_full_range_flag", 1);
    if (s.u("colour_description_present_flag", 1)) {
      s.u("colour_primaries", 8);
      s.u("transfer_characteristics", 8);
      s.u("matrix_coefficients", 8);
    }
  }
  if (s.u("chroma_loc_info_present_flag", 1)) {
    s.ue("chroma_sample_loc_type_top_field", 0, kU32);
    s.ue("chroma_sample_loc_type_bottom_field", 0, kU32);
  }
  if (s.u("timing_info_present_flag", 1)) {
    s.u("num_units_in_tick", 32);
    s.u("time_scale", 32);
    s.u("fixed_frame_rate_flag", 1);
  }
  int nal = (int)s.u("nal_hrd_parameters_present_flag", 1);
  if (nal) hrd_parameters(s);
  int vcl = (int)s.u("vcl_hrd_parameters_present_flag", 1);
  if (vcl) hrd_parameters(s);
  if (nal || vcl) s.u("low_delay_hrd_flag", 1);
  s.u("pic_struct_present_flag", 1);
  if (s.u("bitstream_restriction_flag", 1)) {
    s.u("motion_vectors_over_pic_boundaries_flag", 1);
    s.ue("max_bytes_per_pic_denom", 0, kU32);
    s.ue("max_bits_per_mb_denom", 0, kU32);
    s.ue("log2_max_mv_length_horizontal", 0, kU32);
    s.ue("log2_max_mv_length_vertical", 0, kU32);
    s.ue("max_num_reorder_frames", 0, kU32);
    s.ue("max_dec_frame_buffering", 0, kU32);
  }
}

static bool high_profile(int p) {
  return p == 100 || p == 110 || p == 122 || p == 244 || p == 44 || p == 83 || p == 86 || p == 118 ||
         p == 128 || p == 138 || p == 139 || p == 134 || p == 135;
}

void code_sps(Syn& s, Sps& p) {
  p.profile_idc = (int)s.u("profile_idc", 8);
  s.u("constraint_set_flags", 8);  // constraint_set0..5_flag, reserved_zero_2bits
  s.u("level_idc", 8);
  p.id = (int)s.ue("seq_parameter_set_id", 0, 31);
  if (high_profile(p.profile_idc)) {
    p.chroma_format_idc = (int)s.ue("chroma_format_idc", 0, 3);
    if (p.chroma_format_idc == 3) p.separate_colour_plane_flag = (int)s.u("separate_colour_plane_flag", 1);
    p.bit_depth_luma = 8 + (int)s.ue("bit_depth_luma_minus8", 0, 6);
    p.bit_depth_chroma = 8 + (int)s.ue("bit_depth_chroma_minus8", 0, 6);
    p.transform_bypass = (int)s.u("qpprime_y_zero_transform_bypass_flag", 1);
    if ((p.scaling_matrix = (int)s.u("seq_scaling_matrix_present_flag", 1)))
      for (int i = 0; i < (p.chroma_format_idc != 3 ? 8 : 12); i++)
        scaling_list(s, "seq_scaling_list", i < 6 ? 16 : 64);
  }
  p.log2_max_frame_num = 4 + (int)s.ue("log2_max_frame_num_minus4", 0, 12);
  p.poc_type = (int)s.ue("pic_order_cnt_type", 0, 2);
  if (p.poc_type == 0) {
    p.log2_max_poc_lsb = 4 + (int)s.ue("log2_max_pic_order_cnt_lsb_minus4", 0, 12);
  } else if (p.poc_type == 1) {
    p.delta_pic_order_always_zero_flag = (int)s.u("delta_pic_order_always_zero_flag", 1);
    p.offset_for_non_ref_pic = s.se("offset_for_non_ref_pic", -kS32, kS32);
    p.offset_for_top_to_bottom_field = s.se("offset_for_top_to_bottom_field", -kS32, kS32);
    p.num_ref_frames_in_poc_cycle = (int)s.ue("num_ref_frames_in_pic_order_cnt_cycle", 0, 255);
    s.begin("offset_for_ref_frame");
    for (int i = 0; i < p.num_ref_frames_in_poc_cycle; i++) p.offset_for_ref_frame[i] = s.vse(-kS32, kS32);
    s.end();
  }
  p.max_num_ref_frames = (int)Min<int64_t>(s.ue("max_num_ref_frames", 0, kU32), 1 << 16);
  s.u("gaps_in_frame_num_value_allowed_flag", 1);
  p.width_mbs = 1 + (int)s.ue("pic_width_in_mbs_minus1", 0, 4095);
  p.height_map_units = 1 + (int)s.ue("pic_height_in_map_units_minus1", 0, 4095);
  p.frame_mbs_only_flag = (int)s.u("frame_mbs_only_flag", 1);
  if (!p.frame_mbs_only_flag) p.mb_adaptive_frame_field_flag = (int)s.u("mb_adaptive_frame_field_flag", 1);
  p.direct_8x8_inference_flag = (int)s.u("direct_8x8_inference_flag", 1);
  if (s.u("frame_cropping_flag", 1)) {
    s.ue("frame_crop_left_offset", 0, kU32);
    s.ue("frame_crop_right_offset", 0, kU32);
    s.ue("frame_crop_top_offset", 0, kU32);
    s.ue("frame_crop_bottom_offset", 0, kU32);
  }
  if (s.u("vui_parameters_present_flag", 1)) vui_parameters(s);
  if ((int64_t)p.width_mbs * p.frame_height_mbs() > kMaxMbs) s.fail("picture too large");
}

void code_pps(Syn& s, Pps& p, const ParamSets& ps) {
  p.id = (int)s.ue("pic_parameter_set_id", 0, 255);
  p.sps_id = (int)s.ue("seq_parameter_set_id", 0, 31);
  p.entropy_coding_mode_flag = (int)s.u("entropy_coding_mode_flag", 1);
  p.bottom_field_pic_order_in_frame_present_flag = (int)s.u("bottom_field_pic_order_in_frame_present_flag", 1);
  p.num_slice_groups = 1 + (int)s.ue("num_slice_groups_minus1", 0, 7);
  if (p.num_slice_groups > 1) {
    p.slice_group_map_type = (int)s.ue("slice_group_map_type", 0, 6);
    if (p.slice_group_map_type == 0) {
      s.begin("run_length_minus1");
      for (int i = 0; i < p.num_slice_groups; i++) p.run_length_minus1[i] = (int)s.vue(0, kMaxMbs);
      s.end();
    } else if (p.slice_group_map_type == 2) {
      for (int i = 0; i < p.num_slice_groups - 1; i++) {
        s.begin("slice_group_rect");  // top_left, bottom_right
        p.top_left[i] = (int)s.vue(0, kMaxMbs);
        p.bottom_right[i] = (int)s.vue(0, kMaxMbs);
        s.end();
      }
    } else if (p.slice_group_map_type >= 3 && p.slice_group_map_type <= 5) {
      p.slice_group_change_direction_flag = (int)s.u("slice_group_change_direction_flag", 1);
      p.slice_group_change_rate = 1 + (int)s.ue("slice_group_change_rate_minus1", 0, kMaxMbs);
    } else if (p.slice_group_map_type == 6) {
      int n = 1 + (int)s.ue("pic_size_in_map_units_minus1", 0, kMaxMbs);
      int bits = 0;
      while ((1 << bits) < p.num_slice_groups) bits++;
      s.begin("slice_group_id");
      for (int i = 0; i < n; i++) p.slice_group_id.push_back((uint8_t)s.vu(bits));
      s.end();
    }
  }
  p.num_ref_idx_default[0] = 1 + (int)s.ue("num_ref_idx_l0_default_active_minus1", 0, 31);
  p.num_ref_idx_default[1] = 1 + (int)s.ue("num_ref_idx_l1_default_active_minus1", 0, 31);
  p.weighted_pred_flag = (int)s.u("weighted_pred_flag", 1);
  p.weighted_bipred_idc = (int)s.u("weighted_bipred_idc", 2);
  p.pic_init_qp = 26 + (int)s.se("pic_init_qp_minus26", -128, 127);
  s.se("pic_init_qs_minus26", -kS32, kS32);
  p.chroma_qp_index_offset = p.second_chroma_qp_index_offset = (int)tc_clamp64(s.se("chroma_qp_index_offset", -kS32, kS32), -12, 12);
  p.deblocking_filter_control_present_flag = (int)s.u("deblocking_filter_control_present_flag", 1);
  p.constrained_intra_pred_flag = (int)s.u("constrained_intra_pred_flag", 1);
  p.redundant_pic_cnt_present_flag = (int)s.u("redundant_pic_cnt_present_flag", 1);
  if (s.more_data("transform_8x8_mode_flag")) {
    p.transform_8x8_mode_flag = (int)s.u("transform_8x8_mode_flag", 1);
    if ((p.scaling_matrix = (int)s.u("pic_scaling_matrix_present_flag", 1))) {
      const Sps* sps = ps.get_sps(p.sps_id);
      if (!sps) s.fail("PPS refers to a missing SPS");
      int n = 6 + (sps->chroma_format_idc != 3 ? 2 : 6) * p.transform_8x8_mode_flag;
      for (int i = 0; i < n; i++) scaling_list(s, "pic_scaling_list", i < 6 ? 16 : 64);
    }
    p.second_chroma_qp_index_offset = (int)tc_clamp64(s.se("second_chroma_qp_index_offset", -kS32, kS32), -12, 12);
  }
}

void code_sei(Syn& s) {
  do {
    s.begin("sei_message");  // payloadType, payloadSize, payload in hex
    s.vff();
    int64_t size = s.vff();
    if (!s.W && (uint64_t)size * 8 > s.br_->size() - s.br_->pos()) s.fail("SEI payload past the end");
    s.vbytes((size_t)size);
    s.end();
  } while (s.more_data("sei_message"));
}

void code_aud(Syn& s) { s.u("primary_pic_type", 3); }

void code_filler(Syn& s) {
  s.begin("ff_bytes");  // the number of ff_byte
  int64_t n = 0;
  if (!s.W)
    while (s.br_->size() - s.br_->pos() >= 8 && s.br_->peek(8) == 0xFF) {
      s.br_->skip(8);
      n++;
    }
  n = s.vval(n, 0, 1 << 24);
  if (s.W)
    for (int64_t i = 0; i < n; i++) s.bw_->put(0xFF, 8);
  s.end();
}

static void ref_pic_list_modification(Syn& s, const char* name, Rplm* ops, int& nops) {
  s.begin(name);  // ref_pic_list_modification_flag, then the operations
  if (s.vu(1)) {
    for (int n = 0;; n++) {
      int idc = (int)s.vue(0, 3);
      if (idc == 3) break;
      ops[n].idc = idc;
      ops[n].arg = s.vue(0, kU32);  // abs_diff_pic_num_minus1 or long_term_pic_num
      nops = n + 1;
      if (n > 64) s.fail("too many reference list modifications");
    }
  }
  s.end();
}

// pred_weight_table() (7.3.3.2) of lists lists, the pictures with chroma
// or not, into h.wt
static void pred_weight_table(Syn& s, SliceHeader& h, bool chroma, int lists) {
  h.luma_log2_wd = (int)s.ue("luma_log2_weight_denom", 0, 7);
  if (chroma) h.chroma_log2_wd = (int)s.ue("chroma_log2_weight_denom", 0, 7);
  for (int l = 0; l < lists; l++)
    for (int i = 0; i < h.num_ref_idx_active[l]; i++) {
      // luma_weight_lX_flag [luma_weight luma_offset]
      // chroma_weight_lX_flag [cb_weight cb_offset cr_weight cr_offset]
      int w[6] = {1 << h.luma_log2_wd, 0, 1 << h.chroma_log2_wd, 0, 1 << h.chroma_log2_wd, 0};
      s.begin(l ? "pred_weight_l1" : "pred_weight_l0");
      if (s.vu(1)) {
        w[0] = (int)s.vse(-128, 127);
        w[1] = (int)s.vse(-128, 127);
      }
      if (chroma && s.vu(1))
        for (int j = 0; j < 4; j++) w[2 + j] = (int)s.vse(-128, 127);
      s.end();
      for (int k = 0; k < 6; k++) h.wt[l][h.nwt[l]++] = w[k];
    }
}

// dec_ref_pic_marking() (7.3.3.3), into h's long_term_reference_flag,
// mmco and mmco5
static void dec_ref_pic_marking(Syn& s, SliceHeader& h) {
  if (h.nal_unit_type == 5) {
    s.u("no_output_of_prior_pics_flag", 1);
    h.long_term_reference_flag = (int)s.u("long_term_reference_flag", 1);
    return;
  }
  if (!s.u("adaptive_ref_pic_marking_mode_flag", 1)) return;
  for (int n = 0;; n++) {
    Mmco& op = h.mmco[n];
    op.n = 0;
    s.begin("mmco");  // memory_management_control_operation and its arguments
    op.op = s.vue(0, 6);
    if (op.op == 1 || op.op == 3) op.arg[op.n++] = s.vue(0, kU32);  // difference_of_pic_nums_minus1
    if (op.op == 2) op.arg[op.n++] = s.vue(0, kU32);                // long_term_pic_num
    if (op.op == 3 || op.op == 6) op.arg[op.n++] = s.vue(0, kU32);  // long_term_frame_idx
    if (op.op == 4) op.arg[op.n++] = s.vue(0, kU32);                // max_long_term_frame_idx_plus1
    s.end();
    if (op.op == 0) break;
    h.mmco5 |= op.op == 5;
    h.nmmco = n + 1;
    if (n > 66) s.fail("too many memory management operations");
  }
}

void code_slice_header(Syn& s, SliceHeader& h, const ParamSets& ps) {
  h.first_mb = (int)s.ue("first_mb_in_slice", 0, kMaxMbs);
  h.slice_type = (int)s.ue("slice_type", 0, 9);
  h.pps_id = (int)s.ue("pic_parameter_set_id", 0, 255);
  h.pps = ps.get_pps(h.pps_id);
  if (!h.pps) s.fail("slice refers to a missing PPS");
  h.sps = ps.get_sps(h.pps->sps_id);
  if (!h.sps) s.fail("slice refers to a missing SPS");
  const Sps& sps = *h.sps;
  const Pps& pps = *h.pps;
  int t = h.type();
  if (sps.separate_colour_plane_flag) s.u("colour_plane_id", 2);
  h.frame_num = (int)s.u("frame_num", sps.log2_max_frame_num);
  if (!sps.frame_mbs_only_flag) {
    h.field_pic_flag = (int)s.u("field_pic_flag", 1);
    if (h.field_pic_flag) h.bottom_field_flag = (int)s.u("bottom_field_flag", 1);
  }
  if (h.nal_unit_type == 5) h.idr_pic_id = s.ue("idr_pic_id", 0, kU32);
  if (sps.poc_type == 0) {
    h.poc_lsb = (int)s.u("pic_order_cnt_lsb", sps.log2_max_poc_lsb);
    if (pps.bottom_field_pic_order_in_frame_present_flag && !h.field_pic_flag)
      h.delta_poc_bottom = s.se("delta_pic_order_cnt_bottom", -kS32, kS32);
  }
  if (sps.poc_type == 1 && !sps.delta_pic_order_always_zero_flag) {
    h.delta_poc[0] = s.se("delta_pic_order_cnt_0", -kS32, kS32);
    if (pps.bottom_field_pic_order_in_frame_present_flag && !h.field_pic_flag)
      h.delta_poc[1] = s.se("delta_pic_order_cnt_1", -kS32, kS32);
  }
  if (pps.redundant_pic_cnt_present_flag) h.redundant_pic_cnt = s.ue("redundant_pic_cnt", 0, kU32);
  if (t == kB) h.direct_spatial_mv_pred_flag = (int)s.u("direct_spatial_mv_pred_flag", 1);
  h.num_ref_idx_active[0] = h.num_ref_idx_active[1] = 0;
  if (t == kP || t == kSP || t == kB) {
    h.num_ref_idx_active[0] = pps.num_ref_idx_default[0];
    if (t == kB) h.num_ref_idx_active[1] = pps.num_ref_idx_default[1];
    if (s.u("num_ref_idx_active_override_flag", 1)) {
      h.num_ref_idx_active[0] = 1 + (int)s.ue("num_ref_idx_l0_active_minus1", 0, 31);
      if (t == kB) h.num_ref_idx_active[1] = 1 + (int)s.ue("num_ref_idx_l1_active_minus1", 0, 31);
    }
  }
  if (t != kI && t != kSI) ref_pic_list_modification(s, "ref_pic_list_modification_l0", h.rplm[0], h.nrplm[0]);
  if (t == kB) ref_pic_list_modification(s, "ref_pic_list_modification_l1", h.rplm[1], h.nrplm[1]);
  if ((pps.weighted_pred_flag && (t == kP || t == kSP)) || (pps.weighted_bipred_idc == 1 && t == kB))
    pred_weight_table(s, h, sps.chroma_array_type() != 0, t == kB ? 2 : 1);
  if (h.nal_ref_idc) dec_ref_pic_marking(s, h);
  if (pps.entropy_coding_mode_flag && t != kI && t != kSI) h.cabac_init_idc = (int)s.ue("cabac_init_idc", 0, 2);
  h.slice_qp_delta = (int)s.se("slice_qp_delta", -128, 127);
  // SliceQPY in -QpBdOffsetY..51 (7.4.3), which the model and the reconstruction rely on
  int qp = pps.pic_init_qp + h.slice_qp_delta;
  if (qp < -6 * (sps.bit_depth_luma - 8) || qp > 51) s.fail("slice QP out of range");
  if (t == kSP || t == kSI) {
    if (t == kSP) s.u("sp_for_switch_flag", 1);
    s.se("slice_qs_delta", -kS32, kS32);
  }
  if (pps.deblocking_filter_control_present_flag) {
    h.disable_deblocking = (int)Min<int64_t>(s.ue("disable_deblocking_filter_idc", 0, kU32), 3);
    if (h.disable_deblocking != 1) {
      h.alpha_offset = 2 * (int)tc_clamp64(s.se("slice_alpha_c0_offset_div2", -kS32, kS32), -6, 6);
      h.beta_offset = 2 * (int)tc_clamp64(s.se("slice_beta_offset_div2", -kS32, kS32), -6, 6);
    }
  }
  if (pps.num_slice_groups > 1 && pps.slice_group_map_type >= 3 && pps.slice_group_map_type <= 5) {
    // Ceil(Log2(PicSizeInMapUnits / SliceGroupChangeRate + 1))
    int64_t units = (int64_t)sps.width_mbs * sps.height_map_units;
    int bits = 0;
    while ((((int64_t)1 << bits) - 1) * pps.slice_group_change_rate < units) bits++;
    h.slice_group_change_cycle = (int)s.u("slice_group_change_cycle", bits);
  }
}

}  // namespace h264p
