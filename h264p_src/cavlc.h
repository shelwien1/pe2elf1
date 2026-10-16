// CAVLC residual blocks (H.264 7.3.5.3.2, 9.2).
#ifndef H264P_CAVLC_H
#define H264P_CAVLC_H

#include "bits.h"

namespace h264p {

// One residual_block_cavlc(); c[0..max_coeff-1] are the coefficients in
// coding order. nC is -1 for 4:2:0 chroma DC, -2 for 4:2:2 chroma DC.
// Both return TotalCoeff.
int cavlc_read_block(BitReader& br, int nC, int max_coeff, int* c);
int cavlc_write_block(BitWriter& bw, int nC, int max_coeff, const int* c);

}  // namespace h264p

#endif
