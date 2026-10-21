// CABAC arithmetic decoding and encoding engines (H.264 9.3.1.2, 9.3.3.2,
// 9.3.4.2), table driven: a renormalization is one shift, the decoder reads
// bytes ahead of the bits it has used, the encoder writes whole bytes and
// adds a carry back into those written. Both keep the exact position in the
// bitstream of the standard's bit-serial engines, which the I_PCM samples
// and the end of a slice need.

namespace h264p {

// a context variable: pStateIdx * 2 + valMPS
struct CabacCtx {
  uint8_t s;
  int state() const { return s >> 1; }
  int mps() const { return s & 1; }
};

const int kNumCtx = 1024;


// tables 9-44 and 9-45 by a context's s
struct CabacTables {
  uint8_t lps[4][128];   // codIRangeLPS, by (codIRange >> 6) & 3
  uint8_t next[2][128];  // s after an MPS, after an LPS (valMPS flipped at pStateIdx 0)
};
CabacTables cabac_tables() {
  CabacTables t;
  for (int s = 0; s < 128; s++) {
    int st = s >> 1, m = s & 1;
    for (int q = 0; q < 4; q++) t.lps[q][s] = kRangeLps[st][q];
    t.next[0][s] = (uint8_t)(kTransIdx[st][1] * 2 + m);
    t.next[1][s] = (uint8_t)(kTransIdx[st][0] * 2 + (st == 0 ? 1 - m : m));
  }
  return t;
}
const CabacTables kCabac = cabac_tables();

// the state a context goes to after a bin, whatever the interval
inline void cabac_next(CabacCtx& c, int bin) { c.s = kCabac.next[bin != (c.s & 1)][c.s]; }

// the left shift that brings a range of 2..511 to 256..511
inline int cabac_shift(uint32_t range) { return __builtin_clz(range) - 23; }

struct CabacDecoder {
  // codIOffset is val_ >> kB; below it the next nb_ bits of the input, then
  // zeros (nb_ < 0: the last -nb_ bits of codIOffset are still to come)
  static const int kB = 40;

  void init(BitReader* br) {
    br_ = br;
    data_ = br->data_;
    end_ = data_ + br->nbits_ / 8;
    range_ = 510;
    val_ = 0;
    nb_ = -9;
    while (br->pos_ & 7) val_ |= (uint64_t)br->bit() << (kB - ++nb_);
    p_ = data_ + br->pos_ / 8;
    while (nb_ < 0) refill();
    if (val_ >> kB >= 510) err_raise("invalid CABAC data");
  }
  // (without branches on the bin: an LPS selects its values)
  int decision(CabacCtx& c) {
    unsigned s = c.s;
    uint32_t lps = kCabac.lps[(range_ >> 6) & 3][s];
    uint32_t r = range_ - lps;
    uint64_t sr = (uint64_t)r << kB;
    int l = val_ >= sr;
    val_ -= l ? sr : 0;
    r = l ? lps : r;
    c.s = kCabac.next[l][s];
    int n = cabac_shift(r);
    range_ = r << n;
    val_ <<= n;
    nb_ -= n;
    if (nb_ < 0) refill();
    return (int)(s & 1) ^ l;
  }
  int bypass() {
    val_ <<= 1;
    if (--nb_ < 0) refill();
    uint64_t sr = (uint64_t)range_ << kB;
    if (val_ >= sr) {
      val_ -= sr;
      return 1;
    }
    return 0;
  }
  int terminate() {
    range_ -= 2;
    if (val_ >= (uint64_t)range_ << kB) {
      // no renormalization: the last bit read was the final one
      br_->pos_ = (size_t)(p_ - data_) * 8 - nb_;
      return 1;
    }
    if (range_ < 256) {
      range_ <<= 1;
      val_ <<= 1;
      if (--nb_ < 0) refill();
    }
    return 0;
  }

  // the next bits of the input below the nb_ < 32 there are: 32 of them,
  // or the last bytes; the input ends where the bit-serial engine would
  // have read past it
  void refill() {
    if (end_ - p_ >= 4) {
      uint32_t w = (uint32_t)p_[0] << 24 | (uint32_t)p_[1] << 16 | (uint32_t)p_[2] << 8 | p_[3];
      val_ |= (uint64_t)w << (kB - 32 - nb_);
      nb_ += 32;
      p_ += 4;
      return;
    }
    if (nb_ < 0 && p_ == end_) err_raise("unexpected end of NAL unit");
    for (; p_ < end_ && nb_ < 24; nb_ += 8) val_ |= (uint64_t)*p_++ << (kB - 8 - nb_);
  }
  BitReader* br_;
  const uint8_t *data_, *end_, *p_;
  uint64_t val_;
  uint32_t range_;
  int nb_;
};

struct CabacEncoder {
  // codILow is low_'s 10 low bits, the bits above them up to bit cut_ - 1
  // not yet written, and bit cut_ a carry into the bytes written. The bits
  // start at a byte boundary, at bit 8 of the first codILow: the first bit
  // the standard's PutBit() leaves out, bit 9, is the first cut_, always 0
  // (the code value stays below 510 / 512).
  void init(BitWriter* bw) {
    if (bw->n_ & 7) err_raise("internal: CABAC data not byte aligned");
    bw_ = bw;
    start_ = bw->buf_.size();
    low_ = 0;
    range_ = 510;
    cut_ = 9;
  }
  void decision(CabacCtx& c, int bin) {
    unsigned s = c.s;
    uint32_t lps = kCabac.lps[(range_ >> 6) & 3][s];
    uint32_t r = range_ - lps;
    int l = bin != (int)(s & 1);
    low_ += l ? r : 0;
    r = l ? lps : r;
    c.s = kCabac.next[l][s];
    int n = cabac_shift(r);
    range_ = r << n;
    low_ <<= n;
    cut_ += n;
    if (cut_ >= 42) put();
  }
  void bypass(int bin) {
    low_ = (low_ << 1) + (-(uint64_t)(bin != 0) & range_);
    if (++cut_ >= 42) put();
  }
  // a terminating bin of 1 also flushes the engine; its last bit written is
  // the rbsp_stop_one_bit at the end of a slice
  void terminate(int bin) {
    range_ -= 2;
    if (bin) {
      low_ += range_;
      // range 2 renormalized (7 bits), then PutBit(codILow >> 9 & 1) and
      // codILow >> 7 & 3 | 1 in 2 bits: all bits down to bit 7, the last 1
      low_ <<= 7;
      cut_ += 7;
      if (low_ >> cut_) carry();
      int n = cut_ - 7;
      bw_->put((low_ & ((1ull << cut_) - 1)) >> 7 | 1, n);
      cut_ = 9;
      low_ = 0;
    } else if (range_ < 256) {
      range_ <<= 1;
      low_ <<= 1;
      if (++cut_ >= 42) put();
    }
  }

  // the 32 bits below cut_ written, with the carry above them
  void put() {
    int k = cut_ - 32;
    uint64_t out = low_ >> k;
    low_ &= (1ull << k) - 1;
    cut_ = k;
    if (out >> 32) carry();
    Buf<uint8_t>& b = bw_->buf_;
    size_t i = b.size();
    b.resize(i + 4);
    b[i] = (uint8_t)(out >> 24);
    b[i + 1] = (uint8_t)(out >> 16);
    b[i + 2] = (uint8_t)(out >> 8);
    b[i + 3] = (uint8_t)out;
    bw_->n_ += 32;
  }
  // 1 added to the bytes written (past 0xff bytes); never before the first
  void carry() {
    Buf<uint8_t>& b = bw_->buf_;
    size_t i = b.size();
    while (i > start_ && ++b[--i] == 0) {
    }
  }
  BitWriter* bw_;
  size_t start_;
  uint64_t low_;
  uint32_t range_;
  int cut_;
};

// the contexts' initial states (9.3.1.1); model: 0 for I and SI slices,
// else cabac_init_idc + 1
void cabac_init_contexts(CabacCtx* ctx, int slice_qp, int model) {
  int qp = slice_qp < 0 ? 0 : slice_qp > 51 ? 51 : slice_qp;
  for (int i = 0; i < kNumCtx; i++) {
    const int8_t* mn = i < 460 ? kCabacInit[i][model] : kCabacInit444[i - 460][model];
    int m = mn[0], n = mn[1];
    int pre = ((m * qp) >> 4) + n;
    pre = pre < 1 ? 1 : pre > 126 ? 126 : pre;
    ctx[i].s = (uint8_t)(pre <= 63 ? (63 - pre) * 2 : (pre - 64) * 2 + 1);
  }
}

}  // namespace h264p
