// Symmetric coding of syntax elements: the same code parses a bitstream into
// TSV rows (reading) and writes a bitstream from TSV rows (writing).
#ifndef H264P_SYNTAX_H
#define H264P_SYNTAX_H

#include <memory>
#include <string>
#include <vector>

#include "bits.h"
#include "rows.h"

namespace h264p {

// Header syntax: each syntax element, or group of elements, is one named row.
class Syn {
 public:
  Syn(BitReader* br, RowWriter* out) : W(false), br(br), bw(nullptr), out_(out), in_(nullptr) {}
  Syn(RowReader* in, BitWriter* bw) : W(true), br(nullptr), bw(bw), out_(nullptr), in_(in) {}

  const bool W;  // writing a bitstream
  BitReader* const br;
  BitWriter* const bw;

  // one row with one value
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

  // a row with several values
  void begin(const char* name);
  int64_t vu(int n);
  int64_t vue(int64_t lo, int64_t hi);
  int64_t vse(int64_t lo, int64_t hi);
  int64_t vff();                    // SEI payloadType/payloadSize coding
  std::string vhex(size_t nbytes);  // bytes in hex
  void end();
  // a value that is not coded as such: reading, v goes to the row; writing,
  // the value of the row is returned
  int64_t vval(int64_t v, int64_t lo, int64_t hi);

  // more_rbsp_data() when reading; when writing, whether the next row is
  // named `next`
  bool more_data(const char* next);
  [[noreturn]] void fail(const std::string& msg);

 private:
  int64_t check(int64_t v, int64_t lo, int64_t hi);
  RowWriter* out_;
  RowReader* in_;
  const Row* row_ = nullptr;
  size_t field_ = 0;
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

}  // namespace h264p

#endif
