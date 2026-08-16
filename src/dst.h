// dff2dsf - Direct Stream Transfer (DST) decoder.
//
// DST is the lossless coder Philips uses inside DSDIFF (.dff) files, specified
// in ISO/IEC 14496-3 Part 3 Subpart 10.  This decoder is a port of FFmpeg's
// libavcodec/dstdec.c (Copyright (c) 2014 Peter Ross), reduced to the DSD
// bitstream output: the DSD-to-PCM conversion FFmpeg performs afterwards is not
// needed here, because a .dsf file stores the very same DSD bits.
// Licensed LGPL-2.1-or-later; see LICENSE.ffmpeg.
#ifndef DFF2DSF_DST_H
#define DFF2DSF_DST_H

#include "common.h"

namespace dff2dsf {

constexpr int kDstMaxChannels = 6;
constexpr int kDstMaxElements = 2 * kDstMaxChannels;

// State of the binary arithmetic decoder (10.11); an implementation detail of
// DstDecoder, at namespace scope only so the decoding helpers can take it.
struct DstArithCoder {
    unsigned a;
    unsigned c;
};

class DstDecoder {
public:
    // `dsd_rate` is the DSD bit rate per channel (e.g. 2822400 for DSD64).
    bool init(int channels, unsigned dsd_rate);

    // DSD bits per channel in one DST frame (37632 for DSD64, i.e. 1/75 s).
    unsigned frame_bits() const { return frame_bits_; }

    // Size of the interleaved output buffer decode() expects.
    size_t frame_bytes() const { return size_t(frame_bits_ / 8) * unsigned(channels_); }

    // Decodes one DSTF frame into `out`, which must hold frame_bytes() bytes.
    // Output is MSB-first DSD, byte-interleaved by channel, as stored in DSDIFF.
    // `data` must be followed by kBitReaderPadding zero bytes; `capacity` is the
    // size of that whole padded allocation.
    bool decode(const uint8_t* data, size_t size, size_t capacity, uint8_t* out);

    // Number of frames that were stored uncompressed rather than DST coded.
    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    struct Table {
        unsigned elements;
        unsigned length[kDstMaxElements];
        int coeff[kDstMaxElements][128];
    };

    bool read_map(class BitReader& br, Table& t, unsigned map[kDstMaxChannels]);
    bool read_table(class BitReader& br, Table& t, const int8_t pred[3][3],
                    int length_bits, int coeff_bits, bool is_signed, int offset);
    bool build_filter();

    int channels_ = 0;
    unsigned frame_bits_ = 0;
    uint64_t uncoded_frames_ = 0;

    Table fsets_ {};
    Table probs_ {};
    DstArithCoder ac_ {};
    // 128-bit per-channel shift register of decoded bits, low word first, so
    // that byte index x of the register selects filter table row x.
    uint64_t status_[kDstMaxChannels][2] {};
    alignas(16) int16_t filter_[kDstMaxElements][16][256] {};
};

} // namespace dff2dsf

#endif // DFF2DSF_DST_H
