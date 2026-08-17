// dff2dsf - MSB-first bit writer with carry propagation.
//
// The arithmetic coder in DST renormalises by shifting bits out of a 12-bit
// window, which means an addition to the window can carry into bits that were
// already written.  The whole frame is buffered, so the carry simply propagates
// backwards through the buffer.
#ifndef DFF2DSF_BITWRITE_H
#define DFF2DSF_BITWRITE_H

#include "common.h"

namespace dff2dsf {

class BitWriter {
public:
    // `buf` must be zero filled: bits are OR-ed in.
    void init(ByteP buf, size_t capacity) {
        buf_ = buf;
        capacity_bits_ = capacity * 8;
        pos_ = 0;
        overflow_ = false;
    }

    void put_bit(unsigned v) {
        if (pos_ >= capacity_bits_) { overflow_ = true; return; }
        if (v) buf_[pos_ >> 3] |= uint8_t(0x80u >> (pos_ & 7));
        pos_++;
    }

    // Writes the low n bits of v, most significant first.
    void put(unsigned v, int n) {
        for (int i = n - 1; i >= 0; i--)
            put_bit((v >> i) & 1);
    }

    // Adds one at the position just past the last written bit's weight, i.e.
    // increments the number formed by the bits written so far.
    void carry() {
        if (!pos_) { overflow_ = true; return; }
        size_t i = (pos_ - 1) >> 3;
        unsigned add = 0x80u >> ((pos_ - 1) & 7);
        for (;;) {
            unsigned v = buf_[i] + add;
            buf_[i] = uint8_t(v);
            if (v < 256) return;
            if (i == 0) { overflow_ = true; return; }  // carry out of the buffer
            i--;
            add = 1;
        }
    }

    size_t bit_pos() const { return pos_; }
    size_t byte_size() const { return (pos_ + 7) >> 3; }
    bool overflow() const { return overflow_; }

private:
    ByteP buf_ = nullptr;
    size_t capacity_bits_ = 0;
    size_t pos_ = 0;
    bool overflow_ = false;
};

using BitWriterP = BitWriter* __restrict;

} // namespace dff2dsf

#endif // DFF2DSF_BITWRITE_H
