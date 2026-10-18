// Symmetric coding of syntax elements: the same code parses a bitstream,
// handing the values to a channel (chan.inc), and writes a bitstream from
// the values a channel gives.

namespace h264p {

// Header syntax: each syntax element, or group of elements, is one named
// group of values (a TSV row).
struct Syn {
  Syn(BitReader* br, Chan* ch) : W(false), br(br), bw(nullptr), ch_(ch) {}
  Syn(Chan* ch, BitWriter* bw) : W(true), br(nullptr), bw(bw), ch_(ch) {}

  const bool W;  // writing a bitstream
  BitReader* const br;
  BitWriter* const bw;

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

  int64_t vu(int n) {
    ch_->hbits(n);
    if (!W) {
      int64_t v = br->u(n);
      ch_->hval(v);
      return v;
    }
    int64_t v = check(ch_->hval(0), 0, ((int64_t)1 << n) - 1);
    bw->put((uint64_t)v, n);
    return v;
  }

  int64_t vue(int64_t lo, int64_t hi) {
    if (!W) {
      int64_t v = check(br->ue(), lo, hi);
      ch_->hval(v);
      return v;
    }
    int64_t v = check(ch_->hval(0), lo, hi);
    bw->ue((uint64_t)v);
    return v;
  }

  int64_t vse(int64_t lo, int64_t hi) {
    if (!W) {
      int64_t v = check(br->se(), lo, hi);
      ch_->hval(v);
      return v;
    }
    int64_t v = check(ch_->hval(0), lo, hi);
    bw->se(v);
    return v;
  }

  // SEI payloadType/payloadSize coding
  int64_t vff() {
    if (!W) {
      int64_t v = 0;
      uint32_t b;
      while ((b = br->u(8)) == 255)
        if ((v += 255) > (1 << 24)) fail("invalid SEI message header");
      v += b;
      ch_->hval(v);
      return v;
    }
    int64_t v = check(ch_->hval(0), 0, 1 << 24);
    for (int64_t x = v;; x -= 255) {
      bw->put(x < 255 ? x : 255, 8);
      if (x < 255) break;
    }
    return v;
  }

  // bytes
  void vbytes(size_t nbytes) {
    std::vector<uint8_t> b(nbytes);
    if (!W) {
      for (auto& x : b) x = (uint8_t)br->u(8);
      ch_->hbytes(b);
      return;
    }
    ch_->hbytes(b);
    if (b.size() != nbytes) fail("wrong data length");
    for (uint8_t x : b) bw->put(x, 8);
  }

  void end() { ch_->hend(); }

  // a value that is not coded as such: parsing, v goes to the channel;
  // writing, the value of the channel is returned
  int64_t vval(int64_t v, int64_t lo, int64_t hi) {
    if (!W) {
      check(v, lo, hi);
      ch_->hval(v);
      return v;
    }
    return check(ch_->hval(0), lo, hi);
  }

  // more_rbsp_data() when parsing; when writing, what the channel says (for
  // TSV rows, whether the next row is named `next`)
  bool more_data(const char* next) {
    if (!W) return ch_->hmore(next, br->more_rbsp_data());
    return ch_->hmore(next, false);
  }

  [[noreturn]] void fail(const std::string& msg) {
    if (!W) throw ParseError(msg);
    ch_->fail(msg);
    throw TsvError(msg);  // not reached
  }

  int64_t check(int64_t v, int64_t lo, int64_t hi) {
    if (v < lo || v > hi) fail("value " + std::to_string(v) + " out of range");
    return v;
  }

  Chan* ch_;
};

struct Sps {
  int id = 0;
  int profile_idc = 0;
  int chroma_format_idc = 1, separate_colour_plane_flag = 0;
  int bit_depth_luma = 8, bit_depth_chroma = 8;
  int log2_max_frame_num = 4;
  int poc_type = 0, log2_max_poc_lsb = 4, delta_pic_order_always_zero_flag = 0;
  int width_mbs = 0, height_map_units = 0;
  int frame_mbs_only_flag = 1, mb_adaptive_frame_field_flag = 0, direct_8x8_inference_flag = 0;
  // for decoding the pictures (model.inc)
  int max_num_ref_frames = 0, transform_bypass = 0, scaling_matrix = 0;
  int64_t offset_for_non_ref_pic = 0, offset_for_top_to_bottom_field = 0;
  std::vector<int64_t> offset_for_ref_frame;

  int chroma_array_type() const { return separate_colour_plane_flag ? 0 : chroma_format_idc; }
  int frame_height_mbs() const { return (2 - frame_mbs_only_flag) * height_map_units; }
};

struct Pps {
  int id = 0, sps_id = 0;
  int entropy_coding_mode_flag = 0;
  int bottom_field_pic_order_in_frame_present_flag = 0;
  int num_slice_groups = 1, slice_group_map_type = 0;
  std::vector<int> run_length_minus1, top_left, bottom_right, slice_group_id;
  int slice_group_change_direction_flag = 0, slice_group_change_rate = 1;
  int num_ref_idx_default[2] = {1, 1};
  int weighted_pred_flag = 0, weighted_bipred_idc = 0;
  int pic_init_qp = 26;
  int deblocking_filter_control_present_flag = 0, constrained_intra_pred_flag = 0;
  int redundant_pic_cnt_present_flag = 0;
  int transform_8x8_mode_flag = 0;
  int chroma_qp_index_offset = 0, second_chroma_qp_index_offset = 0, scaling_matrix = 0;
};

struct ParamSets {
  std::shared_ptr<const Sps> sps[32];
  std::shared_ptr<const Pps> pps[256];
};

enum SliceType { kP = 0, kB = 1, kI = 2, kSP = 3, kSI = 4 };

struct SliceHeader {
  int nal_unit_type = 0, nal_ref_idc = 0;
  int first_mb = 0, slice_type = 0, pps_id = 0, colour_plane_id = 0;
  int frame_num = 0, field_pic_flag = 0, bottom_field_flag = 0;
  int direct_spatial_mv_pred_flag = 0;
  int num_ref_idx_active[2] = {0, 0};
  int cabac_init_idc = 0;
  int slice_qp_delta = 0;
  int slice_group_change_cycle = 0;
  // for decoding the pictures (model.inc)
  int poc_lsb = 0, long_term_reference_flag = 0, mmco5 = 0;
  int64_t delta_poc_bottom = 0, delta_poc[2] = {0, 0};
  std::vector<std::pair<int, int64_t>> rplm[2];  // modification_of_pic_nums_idc and its argument
  std::vector<std::vector<int64_t>> mmco;        // memory_management_control_operation and its arguments
  int luma_log2_wd = 0, chroma_log2_wd = 0;
  std::vector<int> wt[2];  // per reference: luma weight, offset, Cb weight, offset, Cr weight, offset; empty: none
  int disable_deblocking = 0, alpha_offset = 0, beta_offset = 0;
  std::shared_ptr<const Sps> sps;
  std::shared_ptr<const Pps> pps;
  int pad = 0;  // x264's padding bit after CABAC flushes (x264_padding row), not coded

  int type() const { return slice_type % 5; }
};

void code_sps(Syn& s, Sps& sps);
void code_pps(Syn& s, Pps& pps, const ParamSets& ps);
void code_sei(Syn& s);
void code_aud(Syn& s);
void code_filler(Syn& s);
void code_slice_header(Syn& s, SliceHeader& h, const ParamSets& ps);

static int64_t tc_clamp64(int64_t v, int64_t lo, int64_t hi) { return v < lo ? lo : v > hi ? hi : v; }

const int64_t kU32 = 0xFFFFFFFE;  // largest ue(v) value
const int64_t kS32 = 0x7FFFFFFF;  // largest se(v) magnitude
const int kMaxMbs = 1 << 20;      // largest picture size in macroblocks accepted

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
    int n = (int)s.ue("num_ref_frames_in_pic_order_cnt_cycle", 0, 255);
    s.begin("offset_for_ref_frame");
    for (int i = 0; i < n; i++) p.offset_for_ref_frame.push_back(s.vse(-kS32, kS32));
    s.end();
  }
  p.max_num_ref_frames = (int)std::min<int64_t>(s.ue("max_num_ref_frames", 0, kU32), 1 << 16);
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
      for (int i = 0; i < p.num_slice_groups; i++) p.run_length_minus1.push_back((int)s.vue(0, kMaxMbs));
      s.end();
    } else if (p.slice_group_map_type == 2) {
      for (int i = 0; i < p.num_slice_groups - 1; i++) {
        s.begin("slice_group_rect");  // top_left, bottom_right
        p.top_left.push_back((int)s.vue(0, kMaxMbs));
        p.bottom_right.push_back((int)s.vue(0, kMaxMbs));
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
      for (int i = 0; i < n; i++) p.slice_group_id.push_back((int)s.vu(bits));
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
      const Sps* sps = ps.sps[p.sps_id].get();
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
    if (!s.W && (uint64_t)size * 8 > s.br->size() - s.br->pos()) s.fail("SEI payload past the end");
    s.vbytes((size_t)size);
    s.end();
  } while (s.more_data("sei_message"));
}

void code_aud(Syn& s) { s.u("primary_pic_type", 3); }

void code_filler(Syn& s) {
  s.begin("ff_bytes");  // the number of ff_byte
  int64_t n = 0;
  if (!s.W)
    while (s.br->size() - s.br->pos() >= 8 && s.br->peek(8) == 0xFF) {
      s.br->skip(8);
      n++;
    }
  n = s.vval(n, 0, 1 << 30);
  if (s.W)
    for (int64_t i = 0; i < n; i++) s.bw->put(0xFF, 8);
  s.end();
}

static void ref_pic_list_modification(Syn& s, const char* name, std::vector<std::pair<int, int64_t>>& ops) {
  s.begin(name);  // ref_pic_list_modification_flag, then the operations
  if (s.vu(1)) {
    for (int n = 0;; n++) {
      int idc = (int)s.vue(0, 3);
      if (idc == 3) break;
      ops.push_back(std::make_pair(idc, s.vue(0, kU32)));  // abs_diff_pic_num_minus1 or long_term_pic_num
      if (n > 64) s.fail("too many reference list modifications");
    }
  }
  s.end();
}

void code_slice_header(Syn& s, SliceHeader& h, const ParamSets& ps) {
  h.first_mb = (int)s.ue("first_mb_in_slice", 0, kMaxMbs);
  h.slice_type = (int)s.ue("slice_type", 0, 9);
  h.pps_id = (int)s.ue("pic_parameter_set_id", 0, 255);
  h.pps = ps.pps[h.pps_id];
  if (!h.pps) s.fail("slice refers to a missing PPS");
  h.sps = ps.sps[h.pps->sps_id];
  if (!h.sps) s.fail("slice refers to a missing SPS");
  const Sps& sps = *h.sps;
  const Pps& pps = *h.pps;
  int t = h.type();
  if (sps.separate_colour_plane_flag) h.colour_plane_id = (int)s.u("colour_plane_id", 2);
  h.frame_num = (int)s.u("frame_num", sps.log2_max_frame_num);
  if (!sps.frame_mbs_only_flag) {
    h.field_pic_flag = (int)s.u("field_pic_flag", 1);
    if (h.field_pic_flag) h.bottom_field_flag = (int)s.u("bottom_field_flag", 1);
  }
  if (h.nal_unit_type == 5) s.ue("idr_pic_id", 0, kU32);
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
  if (pps.redundant_pic_cnt_present_flag) s.ue("redundant_pic_cnt", 0, kU32);
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
  if (t != kI && t != kSI) ref_pic_list_modification(s, "ref_pic_list_modification_l0", h.rplm[0]);
  if (t == kB) ref_pic_list_modification(s, "ref_pic_list_modification_l1", h.rplm[1]);
  if ((pps.weighted_pred_flag && (t == kP || t == kSP)) || (pps.weighted_bipred_idc == 1 && t == kB)) {
    h.luma_log2_wd = (int)s.ue("luma_log2_weight_denom", 0, 7);
    int chroma = sps.chroma_array_type() != 0;
    if (chroma) h.chroma_log2_wd = (int)s.ue("chroma_log2_weight_denom", 0, 7);
    for (int l = 0; l < (t == kB ? 2 : 1); l++)
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
        h.wt[l].insert(h.wt[l].end(), w, w + 6);
      }
  }
  if (h.nal_ref_idc) {
    if (h.nal_unit_type == 5) {
      s.u("no_output_of_prior_pics_flag", 1);
      h.long_term_reference_flag = (int)s.u("long_term_reference_flag", 1);
    } else if (s.u("adaptive_ref_pic_marking_mode_flag", 1)) {
      for (int n = 0;; n++) {
        std::vector<int64_t> op(1);
        s.begin("mmco");  // memory_management_control_operation and its arguments
        op[0] = s.vue(0, 6);
        if (op[0] == 1 || op[0] == 3) op.push_back(s.vue(0, kU32));  // difference_of_pic_nums_minus1
        if (op[0] == 2) op.push_back(s.vue(0, kU32));                // long_term_pic_num
        if (op[0] == 3 || op[0] == 6) op.push_back(s.vue(0, kU32));  // long_term_frame_idx
        if (op[0] == 4) op.push_back(s.vue(0, kU32));                // max_long_term_frame_idx_plus1
        s.end();
        if (op[0] == 0) break;
        h.mmco5 |= op[0] == 5;
        h.mmco.push_back(op);
        if (n > 66) s.fail("too many memory management operations");
      }
    }
  }
  if (pps.entropy_coding_mode_flag && t != kI && t != kSI) h.cabac_init_idc = (int)s.ue("cabac_init_idc", 0, 2);
  h.slice_qp_delta = (int)s.se("slice_qp_delta", -128, 127);
  if (t == kSP || t == kSI) {
    if (t == kSP) s.u("sp_for_switch_flag", 1);
    s.se("slice_qs_delta", -kS32, kS32);
  }
  if (pps.deblocking_filter_control_present_flag) {
    h.disable_deblocking = (int)std::min<int64_t>(s.ue("disable_deblocking_filter_idc", 0, kU32), 3);
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
