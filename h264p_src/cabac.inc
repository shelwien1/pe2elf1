// CABAC arithmetic decoding and encoding engines (H.264 9.3.1.2, 9.3.3.2,
// 9.3.4), working bit by bit so that the position in the bitstream is
// always exact.

namespace h264p {

struct CabacCtx {
  uint8_t state, mps;
};

const int kNumCtx = 1024;

// model: 0 for I and SI slices, else cabac_init_idc + 1
void cabac_init_contexts(CabacCtx* ctx, int slice_qp, int model);

class CabacDecoder {
 public:
  void init(BitReader* br) {
    br_ = br;
    range_ = 510;
    offset_ = br->u(9);
    if (offset_ >= 510) throw ParseError("invalid CABAC data");
  }
  int decision(CabacCtx& c);
  int bypass() {
    offset_ = (offset_ << 1) | br_->bit();
    if (offset_ >= range_) {
      offset_ -= range_;
      return 1;
    }
    return 0;
  }
  int terminate() {
    range_ -= 2;
    if (offset_ >= range_) return 1;  // no renormalization: the last bit read was the final one
    renorm();
    return 0;
  }

 private:
  void renorm() {
    while (range_ < 256) {
      range_ <<= 1;
      offset_ = (offset_ << 1) | br_->bit();
    }
  }
  BitReader* br_ = nullptr;
  uint32_t range_ = 0, offset_ = 0;
};

class CabacEncoder {
 public:
  void init(BitWriter* bw) {
    bw_ = bw;
    low_ = 0;
    range_ = 510;
    outstanding_ = 0;
    first_ = true;
  }
  void decision(CabacCtx& c, int bin);
  void bypass(int bin) {
    low_ <<= 1;
    if (bin) low_ += range_;
    if (low_ >= 1024) {
      put_bit(1);
      low_ -= 1024;
    } else if (low_ < 512) {
      put_bit(0);
    } else {
      low_ -= 512;
      outstanding_++;
    }
  }
  // a terminating bin of 1 also flushes the engine; its last bit written is
  // the rbsp_stop_one_bit at the end of a slice
  void terminate(int bin) {
    range_ -= 2;
    if (bin) {
      low_ += range_;
      range_ = 2;
      renorm();
      put_bit((low_ >> 9) & 1);
      bw_->put(((low_ >> 7) & 3) | 1, 2);
    } else {
      renorm();
    }
  }

 private:
  void renorm() {
    while (range_ < 256) {
      if (low_ < 256) {
        put_bit(0);
      } else if (low_ >= 512) {
        low_ -= 512;
        put_bit(1);
      } else {
        low_ -= 256;
        outstanding_++;
      }
      range_ <<= 1;
      low_ <<= 1;
    }
  }
  void put_bit(int b) {
    if (first_)
      first_ = false;
    else
      bw_->bit(b);
    for (; outstanding_ > 0; outstanding_--) bw_->bit(1 - b);
  }
  BitWriter* bw_ = nullptr;
  uint32_t low_ = 0, range_ = 0;
  int outstanding_ = 0;
  bool first_ = true;
};

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
