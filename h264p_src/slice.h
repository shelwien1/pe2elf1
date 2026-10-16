// Slice data: the macroblock layer, CAVLC or CABAC coded (H.264 7.3.4, 7.3.5).
#ifndef H264P_SLICE_H
#define H264P_SLICE_H

#include <cstdint>
#include <vector>

#include "syntax.h"

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

// Parses the slice data following the slice header into rows, or writes it
// from rows. The slice data ends with the rbsp_stop_one_bit (written by the
// CABAC flush, or explicitly for CAVLC).
void read_slice_data(const SliceHeader& h, MbStore& store, BitReader& br, RowWriter& out);
void write_slice_data(const SliceHeader& h, MbStore& store, RowReader& in, BitWriter& bw);

}  // namespace h264p

#endif
