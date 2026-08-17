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

constexpr unsigned kDstFilterLength = 128;   // maximum, and what encoders use
constexpr unsigned kDstMaxProbLength = 64;   // 6 bit length field

class DstEncoder {
public:
    ~DstEncoder();

    bool init(int channels, unsigned dsd_rate);

    unsigned frame_bits() const { return frame_bits_; }
    size_t frame_bytes_per_channel() const { return frame_bits_ / 8; }

    // Largest frame the encoder can emit: the uncompressed fallback.
    size_t max_frame_size() const {
        return 1 + frame_bytes_per_channel() * size_t(channels_);
    }

    // Encodes one frame.  `src` holds planar MSB-first DSD, one channel after
    // another, frame_bytes_per_channel() bytes each.  `out` must have room for
    // max_frame_size() bytes.
    bool encode(const uint8_t* src, uint8_t* out, size_t out_capacity, size_t* out_size);

    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    bool alloc();
    void autocorrelation(double* r) const;
    void design_filter(double* r);
    void quantise_filter();
    bool analyse_frame();
#ifdef __AVX2__
    void analyse_channel_avx2(int ch);
#else
    void analyse_channel(int ch);
#endif
    double estimate_cost() const;
    unsigned trimmed_length() const;
    bool refine_filter();
    void gradient_step(int iteration);
    void build_prob_table(unsigned* length, int* table) const;

    int channels_ = 0;
    unsigned frame_bits_ = 0;
    unsigned words_per_channel_ = 0;   // 64-bit words of history + frame bits
    uint64_t uncoded_frames_ = 0;

    uint64_t* bits_ = nullptr;      // per channel: 0xAA history then frame bits
    uint8_t* code_ = nullptr;       // per channel per sample: bin index | correct<<7
    uint64_t* mask_ = nullptr;      // per channel: gradient weight as bitplanes
    int coeff_[kDstFilterLength] = {};
    double weight_[kDstFilterLength] = {};   // filter before quantisation
    int16_t (*filter_)[256] = nullptr;   // [16][256] prediction lookup
    uint8_t* window_ = nullptr;          // the eight bits preceding each sample

    // Per-bin sample and prediction-error counts, gathered over all channels.
    uint64_t bin_count_[kDstMaxProbLength] = {};
    uint64_t bin_errors_[kDstMaxProbLength] = {};
};

} // namespace dff2dsf

#endif // DFF2DSF_DSTENC_H
