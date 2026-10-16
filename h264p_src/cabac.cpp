#include "cabac.h"

#include "tables.h"

namespace h264p {

void cabac_init_contexts(CabacCtx* ctx, int slice_qp, int model) {
  int qp = slice_qp < 0 ? 0 : slice_qp > 51 ? 51 : slice_qp;
  for (int i = 0; i < kNumCtx; i++) {
    const int8_t* mn = i < 460 ? kCabacInit[i][model] : kCabacInit444[i - 460][model];
    int m = mn[0], n = mn[1];
    int pre = ((m * qp) >> 4) + n;
    pre = pre < 1 ? 1 : pre > 126 ? 126 : pre;
    if (pre <= 63) {
      ctx[i].state = (uint8_t)(63 - pre);
      ctx[i].mps = 0;
    } else {
      ctx[i].state = (uint8_t)(pre - 64);
      ctx[i].mps = 1;
    }
  }
}

int CabacDecoder::decision(CabacCtx& c) {
  uint32_t lps = kRangeLps[c.state][(range_ >> 6) & 3];
  range_ -= lps;
  int bin;
  if (offset_ >= range_) {
    bin = !c.mps;
    offset_ -= range_;
    range_ = lps;
    if (c.state == 0) c.mps = (uint8_t)(1 - c.mps);
    c.state = kTransIdx[c.state][0];
  } else {
    bin = c.mps;
    c.state = kTransIdx[c.state][1];
  }
  renorm();
  return bin;
}

void CabacEncoder::decision(CabacCtx& c, int bin) {
  uint32_t lps = kRangeLps[c.state][(range_ >> 6) & 3];
  range_ -= lps;
  if (bin != c.mps) {
    low_ += range_;
    range_ = lps;
    if (c.state == 0) c.mps = (uint8_t)(1 - c.mps);
    c.state = kTransIdx[c.state][0];
  } else {
    c.state = kTransIdx[c.state][1];
  }
  renorm();
}

}  // namespace h264p
