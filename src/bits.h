// dff2dsf - MSB-first bit reader and the JPEG-LS Golomb code used by DST.
//
// Behaviour is modelled on FFmpeg's libavcodec/get_bits.h and golomb.h so that
// decoding stays bit-exact, including how reads past the end of a frame behave
// (they return zeroes, which FFmpeg gets from its zero-padded packet buffers).
// Derived from FFmpeg, licensed LGPL-2.1-or-later; see LICENSE.ffmpeg.
#ifndef DFF2DSF_BITS_H
#define DFF2DSF_BITS_H

#include "common.h"

namespace dff2dsf {

// Bytes of zero padding a caller must provide past the payload so that the
// 64-bit windowed reads below never run off the allocation.
constexpr size_t kBitReaderPadding = 16;

class BitReader {
public:
    // `size` is the payload length; `capacity` is the allocation length, which
    // must be at least size + kBitReaderPadding and zero-filled past `size`.
    void init(const uint8_t* buf, size_t size, size_t capacity) {
        buf_ = buf;
        nbits_ = int64_t(size) * 8;
        capacity_ = capacity;
        pos_ = 0;
    }

    int64_t left() const { return nbits_ - pos_; }
    int64_t pos() const { return pos_; }

    unsigned get1() {
        size_t byte = size_t(pos_ >> 3);
        unsigned bit = unsigned(pos_ & 7);
        pos_++;
        if (byte >= capacity_) return 0;
        return (buf_[byte] >> (7 - bit)) & 1;
    }

    // n in [0,32]; n == 0 yields 0, matching FFmpeg's get_bits_long().
    unsigned get(int n) {
        if (n <= 0) return 0;
        unsigned v = unsigned((window() << unsigned(pos_ & 7)) >> (64 - n));
        pos_ += n;
        return v;
    }

    int get_signed(int n) {
        if (n <= 0) return 0;
        unsigned v = get(n);
        unsigned sign = 1u << (n - 1);
        return int((v ^ sign) - sign);
    }

    void skip(int n) { pos_ += n; }

private:
    uint64_t window() const {
        size_t byte = size_t(pos_ >> 3);
        if (byte + 8 > capacity_) return 0;
        return rb64(buf_ + byte);
    }

    const uint8_t* buf_ = nullptr;
    int64_t nbits_ = 0;
    int64_t pos_ = 0;
    size_t capacity_ = 0;
};

// Unsigned JPEG-LS Golomb code (FFmpeg get_ur_golomb_jpegls with esc_len 0).
// The loop mirrors FFmpeg's operand order: the bit is consumed before the
// remaining-bits check, which matters at the tail of a frame.
inline int get_ur_golomb_jpegls(BitReader& br, int k, int64_t limit) {
    int64_t i;
    for (i = 0; i < limit && br.get1() == 0 && br.left() > 0; i++)
        ;
    if (i < limit - 1)
        return int(br.get(k)) + int(i << k);
    return -1;
}

// Signed variant used by the DST coefficient tables: magnitude followed by a
// sign bit, which is only present for non-zero values.
inline int get_sr_golomb_dst(BitReader& br, unsigned k) {
    int v = get_ur_golomb_jpegls(br, int(k), br.left());
    if (v && br.get1())
        v = -v;
    return v;
}

} // namespace dff2dsf

#endif // DFF2DSF_BITS_H
