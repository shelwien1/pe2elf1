// dff2dsf - Direct Stream Transfer (DST) encoder.
//
// Produces frames the DST decoder in dst.cpp (and FFmpeg's) reads back exactly.
// The approach follows what the Philips encoder emits, as observed in its
// output: one filter shared by all channels, 128 taps quantised to 9 bits, and
// a short probability table measured from the frame itself.
#ifndef DFF2DSF_DSTENC_H
#define DFF2DSF_DSTENC_H

#include "common.h"
#include "dst.h"

namespace dff2dsf {

constexpr uint32_t kDstFilterLength = 128;   // maximum, and what encoders use
constexpr uint32_t kDstMaxProbLength = 64;   // 6 bit length field

// 128 bits of history precede every frame, and the decoder starts them at 0xAA.
constexpr uint32_t kHistoryBits = 128;
constexpr uint32_t kHistoryWords = kHistoryBits / 64;

// Bitplanes the per-sample gradient weight is quantised to.  Four levels of
// resolution is enough to match an exact weighting, at a quarter of the cost.
constexpr uint32_t kGradientPlanes = 4;

// The vectorised prediction reads a byte of history per four taps, the furthest
// reaching back this far before the sample.
constexpr uint32_t kWindowLead = 128;

// Working set for the largest stream this build handles.  Sizing the buffers
// from kMaxFrameBits rather than from the rate in hand costs nothing at DSD64
// beyond untouched memory, and buys an encoder that allocates nothing at all:
// the whole object is about 3 MB, and the threaded encoder allocates one of
// these per worker in a single block.
constexpr uint32_t kMaxWordsPerChannel = kHistoryWords + kMaxFrameBits / 64;

class DstEncoder {
public:
    bool init(int32_t channels, uint32_t dsd_rate);

    uint32_t frame_bits() const { return frame_bits_; }
    size_t frame_bytes_per_channel() const { return frame_bits_ / 8; }

    // Largest frame the encoder can emit: the uncompressed fallback.
    size_t max_frame_size() const {
        return 1 + frame_bytes_per_channel() * size_t(channels_);
    }

    // Encodes one frame.  `src` holds planar MSB-first DSD, one channel after
    // another, frame_bytes_per_channel() bytes each.  `out` must have room for
    // max_frame_size() bytes.
    bool encode(CByteP src, ByteP out, size_t out_capacity, SizeP out_size);

    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    void autocorrelation(DoubleP r) const;
    void design_filter(DoubleP r);
    void quantise_filter();
    bool analyse_frame();
#ifdef __AVX2__
    void analyse_channel_avx2(int32_t ch);
#else
    void analyse_channel(int32_t ch);
#endif
    double estimate_cost() const;
    uint32_t trimmed_length() const;
    bool refine_filter();
    bool refine_from_start();
    void gradient_step(int32_t iteration);
    void build_prob_table(UIntP length, IntP table) const;

    int32_t channels_ = 0;
    uint32_t frame_bits_ = 0;
    uint32_t words_per_channel_ = 0;   // 64-bit words of history + frame bits
    uint64_t uncoded_frames_ = 0;

    // Per channel: 0xAA history then the frame's bits, stride words_per_channel_.
    uint64_t bits_[size_t(kMaxWordsPerChannel) * kDstMaxChannels];
    // Per channel per sample: bin index | correct<<7, stride frame_bits_.
    uint8_t code_[size_t(kMaxFrameBits) * kDstMaxChannels];
    // Per channel: the gradient weights as bitplanes.
    uint64_t mask_[size_t(kGradientPlanes) * (kMaxFrameBits / 64) * kDstMaxChannels];
    uint8_t window_[kWindowLead + kMaxFrameBits + 1];   // the 8 bits before each sample
    int16_t filter_[16][256] = {};        // prediction lookup

    int32_t coeff_[kDstFilterLength] = {};
    double weight_[kDstFilterLength] = {};   // filter before quantisation
    double raw_weight_[kDstFilterLength] = {};   // the same before smoothing

    // Per-bin sample and prediction-error counts, gathered over all channels.
    uint64_t bin_count_[kDstMaxProbLength] = {};
    uint64_t bin_errors_[kDstMaxProbLength] = {};
};

} // namespace dff2dsf

#endif // DFF2DSF_DSTENC_H
