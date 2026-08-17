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

constexpr int32_t kDstMaxChannels = 6;
constexpr int32_t kDstMaxElements = 2 * kDstMaxChannels;

// The largest stream this program handles, and with it the size of every working
// buffer.  ISO/IEC 14496-3 specifies DST for 64, 128 and 256 times 44100; 512 is
// accepted as well, since .dsf files at that rate exist and the frame arithmetic
// is unchanged.  Everything derived from this is a compile-time constant, which
// is what lets the buffers be plain arrays rather than allocations: a frame is
// 1/75 s, so 588 DSD bits per channel per 44.1 kHz of rate.
constexpr uint32_t kMaxDsdRate = 512u * 44100u;
constexpr uint32_t kMaxFrameBits = 588u * (kMaxDsdRate / 44100u);        // 301056
constexpr uint32_t kMaxFrameBytesPerChannel = kMaxFrameBits / 8;         // 37632
constexpr size_t kMaxFrameBytes = size_t(kMaxFrameBytesPerChannel) * kDstMaxChannels;

// DSD bits per channel in one frame at `dsd_rate`.
inline uint32_t frame_bits_for(uint32_t dsd_rate) { return 588u * (dsd_rate / 44100u); }

// The uncompressed fallback frame: a header byte and the interleaved DSD.
constexpr size_t kMaxDstFrameSize = 1 + kMaxFrameBytes;

// State of the binary arithmetic decoder (10.11); an implementation detail of
// DstDecoder, at namespace scope only so the decoding helpers can take it.
struct DstArithCoder {
    uint32_t a;
    uint32_t c;
};

using ArithCoderP = DstArithCoder* __restrict;

// Fixed predictors the coefficient and probability tables are coded against
// (10.12, 10.13).  Shared by the decoder that reads them and the encoder that
// has to produce residuals against the same prediction.
inline constexpr int8_t kFsetsPredCoeff[3][3] = {
    {  -8,  0, 0 },
    { -16,  8, 0 },
    {  -9, -5, 6 },
};

inline constexpr int8_t kProbsPredCoeff[3][3] = {
    {  -8,  0,  0 },
    { -16,  8,  0 },
    { -24, 24, -8 },
};

// Probability of the sign bit of the very first sample (10.11).
inline uint32_t prob_dst_x_bit(int32_t c) {
    return uint32_t((kReverse.v[c & 127] >> 1) + 1);
}

// Expands filter coefficients into the lookup table that evaluates the 128-tap
// sign-based FIR eight taps at a time: lut[j][k] is the contribution of history
// byte j when those eight bits are k.  Shared with the encoder, which has to
// predict exactly as the decoder does.  False if a partial sum leaves int16.
bool build_filter_lut(CIntP coeff, uint32_t length, LutP lut);

class DstDecoder {
public:
    // `dsd_rate` is the DSD bit rate per channel (e.g. 2822400 for DSD64).
    bool init(int32_t channels, uint32_t dsd_rate);

    // DSD bits per channel in one DST frame (37632 for DSD64, i.e. 1/75 s).
    uint32_t frame_bits() const { return frame_bits_; }

    // Size of the interleaved output buffer decode() expects.
    size_t frame_bytes() const { return size_t(frame_bits_ / 8) * uint32_t(channels_); }

    // Decodes one DSTF frame into `out`, which must hold frame_bytes() bytes.
    // Output is MSB-first DSD, byte-interleaved by channel, as stored in DSDIFF.
    // `data` must be followed by kBitReaderPadding zero bytes; `capacity` is the
    // size of that whole padded allocation.
    bool decode(CByteP data, size_t size, size_t capacity, ByteP out);

    // Number of frames that were stored uncompressed rather than DST coded.
    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    struct Table {
        uint32_t elements;
        uint32_t length[kDstMaxElements];
        int32_t coeff[kDstMaxElements][128];
    };

    bool read_map(class BitReader& br, Table& t, UIntP map);
    bool read_table(class BitReader& br, Table& t, CPredP pred,
                    int32_t length_bits, int32_t coeff_bits, bool is_signed, int32_t offset);
    bool build_filter();

    int32_t channels_ = 0;
    uint32_t frame_bits_ = 0;
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
