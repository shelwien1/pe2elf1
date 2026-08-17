// dff2dsf - Direct Stream Transfer (DST) encoder.
//
// Per frame, for all channels together:
//
//   1. autocorrelate the +-1 mapped DSD bits (as popcounts of shifted XORs),
//   2. Levinson-Durbin for a 128 tap predictor, quantised to 9 bit coefficients,
//   3. refine that predictor against the objective DST actually scores it by -
//      how often it gets the sign of the next bit wrong, not mean square error,
//   4. run the prediction to collect, per |prediction| bin, how often the
//      predicted bit was wrong: that measured error rate is the probability
//      table the decoder will use,
//   5. arithmetic code the "prediction was wrong" flags with those probabilities.
//
// Steps 3 to 5 must predict bit for bit as the decoder does, so all of them use
// the filter lookup table from dst.cpp.

#ifndef DFF2DSF_DSTENC_HPP
#define DFF2DSF_DSTENC_HPP

#include "dstenc.h"
#include "bitwrite.h"

#include <math.h>

// The vector kernels are selected by the target the compiler was pointed at:
// build with -mavx2 (or any -march that implies it) and they are used, build
// without and the scalar versions are.  Both produce identical output, so
// compiling it both ways and comparing is how the vector code is tested.
#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace dff2dsf {

namespace {

// 128 bits of history precede every frame, and the decoder starts them at 0xAA.
constexpr unsigned kHistoryBits = 128;
constexpr unsigned kHistoryWords = kHistoryBits / 64;

// Refinement schedule.  Each pass costs one run of the prediction over the
// frame, and the cost keeps falling out to about a dozen; stopping as soon as a
// pass fails to improve is not safe, because the cost dips and recovers.
constexpr int kRefineIterations = 12;
constexpr double kRefineStep = 0.35;
constexpr double kRefineDecay = 0.7;

// Bitplanes the per-sample gradient weight is quantised to.  Four levels of
// resolution is enough to match an exact weighting, at a quarter of the cost.
constexpr unsigned kGradientPlanes = 4;

// The vectorised prediction reads a byte of history per four taps, the furthest
// reaching back this far before the sample.
constexpr unsigned kWindowLead = 128;

// ------------------------------------------------------------ arithmetic coder
//
// The inverse of the decoder's ac_get(): the decoder keeps a 12 bit offset into
// the current interval and reads bits as the interval is renormalised, so the
// encoder keeps the matching 12 bit window of `low` and writes the bits that
// leave it.  Sub-interval [0, a-q) codes e = 1, [a-q, a) codes e = 0.
class ArithEncoder {
public:
    void init(BitWriter* bw) {
        bw_ = bw;
        a_ = 4095;
        low_ = 0;
    }

    void encode(unsigned p, unsigned e) {
        unsigned k = (a_ >> 8) | ((a_ >> 7) & 1);
        unsigned q = k * p;
        unsigned a_q = a_ - q;

        if (e) {
            a_ = a_q;
        } else {
            low_ += a_q;
            if (low_ >= 4096) { low_ -= 4096; bw_->carry(); }
            a_ = q;
        }

        while (a_ < 2048) {
            bw_->put_bit((low_ >> 11) & 1);
            low_ = (low_ << 1) & 4095;
            a_ <<= 1;
        }
    }

    // Writing the window verbatim leaves the decoder's offset at zero, which is
    // inside the final interval; anything the decoder reads beyond the frame is
    // zero padding, so nothing more is needed.
    void flush() { bw_->put(low_, 12); }

private:
    BitWriter* bw_ = nullptr;
    unsigned a_ = 0;
    unsigned low_ = 0;
};

// ------------------------------------------------------------- table encoding

// Cost in bits of the JPEG-LS Golomb code the decoder reads, sign included.
inline size_t golomb_bits(int v, int k) {
    unsigned m = unsigned(v < 0 ? -v : v);
    return size_t(m >> k) + 1 + size_t(k) + (m ? 1 : 0);
}

inline void put_golomb(BitWriter& bw, int v, int k) {
    unsigned m = unsigned(v < 0 ? -v : v);
    for (unsigned i = m >> k; i; i--) bw.put_bit(0);
    bw.put_bit(1);
    bw.put(m, k);
    if (m) bw.put_bit(v < 0);
}

// Residual the decoder's table predictor has to be fed to reproduce values[j].
inline int table_residual(const int* values, unsigned j, unsigned method,
                          const int8_t pred[3][3]) {
    int x = 0;
    for (unsigned k = 0; k <= method; k++)
        x += pred[method][k] * values[j - k - 1];
    return x >= 0 ? values[j] + (x + 4) / 8 : values[j] - (-x + 3) / 8;
}

// Picks the cheapest of the four codings the format allows (verbatim, or
// prediction of order 1..3 with Golomb coded residuals) and, if `bw` is given,
// writes it.  Returns the cost in bits, length field included.  Verbatim values
// are written as coeff_bits of two's complement, which is what the decoder reads
// back with either get_sbits() or get_bits() plus the offset.
size_t code_table(BitWriter* bw, const int* values, unsigned n,
                  const int8_t pred[3][3], int length_bits, int coeff_bits,
                  int offset) {
    const size_t raw_bits = size_t(length_bits) + 1 + size_t(n) * size_t(coeff_bits);

    size_t best = raw_bits;
    int best_method = -1, best_k = 0;

    for (unsigned method = 0; method < 3 && method + 1 < n; method++) {
        for (int k = 0; k < 8; k++) {
            size_t bits = size_t(length_bits) + 1 + 2 +
                          size_t(method + 1) * size_t(coeff_bits) + 3;
            bool usable = true;
            for (unsigned j = method + 1; j < n; j++) {
                int r = table_residual(values, j, method, pred);
                // A run of zeroes longer than the frame would not decode back.
                if ((unsigned(r < 0 ? -r : r) >> k) > 4095) { usable = false; break; }
                bits += golomb_bits(r, k);
                if (bits >= best) { usable = false; break; }
            }
            if (usable && bits < best) { best = bits; best_method = int(method); best_k = k; }
        }
    }

    if (!bw) return best;

    bw->put(n - 1, length_bits);
    if (best_method < 0) {
        bw->put_bit(0);
        for (unsigned j = 0; j < n; j++)
            bw->put(unsigned(values[j] - offset), coeff_bits);
    } else {
        unsigned method = unsigned(best_method);
        bw->put_bit(1);
        bw->put(method, 2);
        for (unsigned j = 0; j <= method; j++)
            bw->put(unsigned(values[j] - offset), coeff_bits);
        bw->put(unsigned(best_k), 3);
        for (unsigned j = method + 1; j < n; j++)
            put_golomb(*bw, table_residual(values, j, method, pred), best_k);
    }
    return best;
}

// Cost in bits of coding `n` flags of which `errors` were prediction errors,
// with the probability the encoder will actually use for that bin.
inline double bin_cost(uint64_t n, uint64_t errors, unsigned p) {
    if (!n) return 0;
    const double pe = double(p) / 256.0;
    return double(errors) * -log2(pe) + double(n - errors) * -log2(1.0 - pe);
}

// The coder's interval arithmetic makes the effective probability slightly less
// than p/256, so the scale that minimises the coded size is a shade above 256.
// Sweeping it here put the optimum between 259 and 262; the reference encoder
// uses 256.5, which is the same correction and is what this follows.
inline unsigned quantise_prob(uint64_t n, uint64_t errors) {
    if (!n) return 128;
    int p = int(lround(256.5 * double(errors) / double(n)));
    if (p < 1) p = 1;
    if (p > 128) p = 128;
    return unsigned(p);
}


// --------------------------------------------------------- bitplane packing
//
// Lays the per-sample gradient weights out as bitplanes for the kernel above.
// Done a word at a time with the plane words held in registers: the obvious
// per-sample "set this bit if that weight bit is set" costs four unpredictable
// branches and four read-modify-writes per sample, which measured far more than
// the correlation it feeds.  Returns the sum of the weights.
#ifndef __AVX2__

uint64_t pack_planes(const uint8_t* code, const uint8_t* weight_of,
                     uint64_t* planes, unsigned nwords) {
    uint64_t total = 0;

    for (unsigned j = 0; j < nwords; j++) {
        const uint8_t* c = code + size_t(j) * 64;
        uint64_t plane[kGradientPlanes] = {};

        for (unsigned b = 0; b < 64; b++) {
            const uint64_t q = weight_of[c[b]];
            const unsigned shift = 63 - b;
            total += q;
            for (unsigned p = 0; p < kGradientPlanes; p++)
                plane[p] |= ((q >> p) & 1) << shift;
        }

        for (unsigned p = 0; p < kGradientPlanes; p++)
            planes[p * nwords + j] = plane[p];
    }
    return total;
}

#else

// Same packing with AVX2.  A byte comparison turns "weight bit p is set" into a
// per-byte mask, and movemask collects 32 of those into a word in one go; the
// bytes are reversed first because the bit numbering here runs the other way.
uint64_t pack_planes(const uint8_t* code, const uint8_t* weight_of,
                     uint64_t* planes, unsigned nwords) {
    const __m256i reverse_lane = _mm256_setr_epi8(
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    const __m256i zero = _mm256_setzero_si256();
    __m256i weight_sum = zero;

    for (unsigned j = 0; j < nwords; j++) {
        uint64_t plane[kGradientPlanes] = {};

        for (unsigned half = 0; half < 2; half++) {
            const uint8_t* c = code + size_t(j) * 64 + half * 32;
            alignas(32) uint8_t q[32];
            for (unsigned b = 0; b < 32; b++)
                q[b] = weight_of[c[b]];

            const __m256i v = _mm256_load_si256((const __m256i*)q);
            weight_sum = _mm256_add_epi64(weight_sum, _mm256_sad_epu8(v, zero));

            const __m256i lanes = _mm256_shuffle_epi8(v, reverse_lane);
            const __m256i reversed = _mm256_permute2x128_si256(lanes, lanes, 0x01);

            for (unsigned p = 0; p < kGradientPlanes; p++) {
                const __m256i sel = _mm256_set1_epi8(char(1 << p));
                const __m256i hit = _mm256_cmpeq_epi8(_mm256_and_si256(reversed, sel), sel);
                const uint32_t bits = uint32_t(_mm256_movemask_epi8(hit));
                plane[p] |= uint64_t(bits) << (half ? 0 : 32);
            }
        }

        for (unsigned p = 0; p < kGradientPlanes; p++)
            planes[p * nwords + j] = plane[p];
    }

    alignas(32) uint64_t lanes[4];
    _mm256_store_si256((__m256i*)lanes, weight_sum);
    return lanes[0] + lanes[1] + lanes[2] + lanes[3];
}

#endif // __AVX2__

// ------------------------------------------------------- correlation kernel
//
// The encoder's innermost loop, by a wide margin: for one lag, the weighted
// count of samples whose bit differs from the bit that many places earlier,
//
//     sum over planes p of  2^p * popcount(plane_p & (bits ^ bits >> k))
//
// Bit parallel already, so 64 samples move per operation; AVX2 quadruples that
// and replaces the popcount with the usual nibble table shuffle.

// Shifted view of the bit sequence: the word k bits earlier than word j.
inline uint64_t lagged_word(const uint64_t* w, unsigned j, unsigned wshift, unsigned bshift) {
    const uint64_t* d = w + j - wshift;
    return bshift ? ((d[-1] << (64 - bshift)) | (d[0] >> bshift)) : d[0];
}

#ifndef __AVX2__

uint64_t disagreement(const uint64_t* w, const uint64_t* planes,
                      unsigned nwords, unsigned k) {
    const unsigned wshift = k >> 6;
    const unsigned bshift = k & 63;
    uint64_t sum[kGradientPlanes] = {};

    for (unsigned j = 0; j < nwords; j++) {
        const uint64_t differs = w[j] ^ lagged_word(w, j, wshift, bshift);
        for (unsigned p = 0; p < kGradientPlanes; p++)
            sum[p] += unsigned(__builtin_popcountll(planes[p * nwords + j] & differs));
    }

    uint64_t total = 0;
    for (unsigned p = 0; p < kGradientPlanes; p++)
        total += sum[p] << p;
    return total;
}

#else

uint64_t disagreement(const uint64_t* w, const uint64_t* planes,
                      unsigned nwords, unsigned k) {
    const unsigned wshift = k >> 6;
    const unsigned bshift = k & 63;

    // Popcount of each nibble, for the byte-wise table lookup below.
    const __m256i nibble_count = _mm256_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i low_nibble = _mm256_set1_epi8(0x0f);
    const __m256i zero = _mm256_setzero_si256();

    __m256i acc[kGradientPlanes];
    for (unsigned p = 0; p < kGradientPlanes; p++)
        acc[p] = zero;

    unsigned j = 0;
    for (; j + 4 <= nwords; j += 4) {
        const __m256i x = _mm256_loadu_si256((const __m256i*)(w + j));
        const __m256i d0 = _mm256_loadu_si256((const __m256i*)(w + j - wshift));
        __m256i y = d0;
        if (bshift) {
            // Every 64-bit lane is shifted the same way, so the lane-wise shifts
            // reproduce the scalar version exactly.
            const __m256i dm1 = _mm256_loadu_si256((const __m256i*)(w + j - wshift - 1));
            y = _mm256_or_si256(_mm256_slli_epi64(dm1, int(64 - bshift)),
                                _mm256_srli_epi64(d0, int(bshift)));
        }
        const __m256i differs = _mm256_xor_si256(x, y);

        for (unsigned p = 0; p < kGradientPlanes; p++) {
            const __m256i v = _mm256_and_si256(
                _mm256_loadu_si256((const __m256i*)(planes + p * nwords + j)), differs);
            const __m256i lo = _mm256_and_si256(v, low_nibble);
            const __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_nibble);
            const __m256i counts = _mm256_add_epi8(_mm256_shuffle_epi8(nibble_count, lo),
                                                   _mm256_shuffle_epi8(nibble_count, hi));
            // Sums each 8 byte group, so counts stay well inside 64 bit lanes.
            acc[p] = _mm256_add_epi64(acc[p], _mm256_sad_epu8(counts, zero));
        }
    }

    uint64_t total = 0;
    for (unsigned p = 0; p < kGradientPlanes; p++) {
        alignas(32) uint64_t lanes[4];
        _mm256_store_si256((__m256i*)lanes, acc[p]);
        total += (lanes[0] + lanes[1] + lanes[2] + lanes[3]) << p;
    }

    // Tail, for frame sizes that are not a multiple of four words.
    for (; j < nwords; j++) {
        const uint64_t differs = w[j] ^ lagged_word(w, j, wshift, bshift);
        for (unsigned p = 0; p < kGradientPlanes; p++)
            total += uint64_t(unsigned(__builtin_popcountll(planes[p * nwords + j] & differs))) << p;
    }
    return total;
}

#endif // __AVX2__


// Plain popcount of the same shifted XOR, for the autocorrelation.
#ifndef __AVX2__

uint64_t differing_bits(const uint64_t* w, unsigned nwords, unsigned k) {
    const unsigned wshift = k >> 6;
    const unsigned bshift = k & 63;
    uint64_t diff = 0;
    for (unsigned j = 0; j < nwords; j++)
        diff += unsigned(__builtin_popcountll(w[j] ^ lagged_word(w, j, wshift, bshift)));
    return diff;
}

#else

uint64_t differing_bits(const uint64_t* w, unsigned nwords, unsigned k) {
    const unsigned wshift = k >> 6;
    const unsigned bshift = k & 63;
    const __m256i nibble_count = _mm256_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i low_nibble = _mm256_set1_epi8(0x0f);
    const __m256i zero = _mm256_setzero_si256();
    __m256i acc = zero;

    unsigned j = 0;
    for (; j + 4 <= nwords; j += 4) {
        const __m256i x = _mm256_loadu_si256((const __m256i*)(w + j));
        const __m256i d0 = _mm256_loadu_si256((const __m256i*)(w + j - wshift));
        __m256i y = d0;
        if (bshift) {
            const __m256i dm1 = _mm256_loadu_si256((const __m256i*)(w + j - wshift - 1));
            y = _mm256_or_si256(_mm256_slli_epi64(dm1, int(64 - bshift)),
                                _mm256_srli_epi64(d0, int(bshift)));
        }
        const __m256i v = _mm256_xor_si256(x, y);
        const __m256i lo = _mm256_and_si256(v, low_nibble);
        const __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_nibble);
        const __m256i counts = _mm256_add_epi8(_mm256_shuffle_epi8(nibble_count, lo),
                                               _mm256_shuffle_epi8(nibble_count, hi));
        acc = _mm256_add_epi64(acc, _mm256_sad_epu8(counts, zero));
    }

    alignas(32) uint64_t lanes[4];
    _mm256_store_si256((__m256i*)lanes, acc);
    uint64_t diff = lanes[0] + lanes[1] + lanes[2] + lanes[3];

    for (; j < nwords; j++)
        diff += unsigned(__builtin_popcountll(w[j] ^ lagged_word(w, j, wshift, bshift)));
    return diff;
}

#endif // __AVX2__

} // namespace

DstEncoder::~DstEncoder() {
    free(bits_);
    free(code_);
    free(filter_);
    free(mask_);
    free(window_);
}

bool DstEncoder::init(int channels, unsigned dsd_rate) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d (DST allows up to %d)", channels, kDstMaxChannels);
    if (dsd_rate > 512u * 44100u || dsd_rate % 44100u)
        return ERR("unsupported sample rate %u", dsd_rate);

    frame_bits_ = 588u * (dsd_rate / 44100u);
    if (frame_bits_ & 63)
        return ERR("frame size of %u bits is not a whole number of 64 bit words", frame_bits_);

    channels_ = channels;
    words_per_channel_ = kHistoryWords + frame_bits_ / 64;
    return alloc();
}

bool DstEncoder::alloc() {
    bits_ = static_cast<uint64_t*>(xalloc(sizeof(uint64_t) * words_per_channel_ * unsigned(channels_)));
    code_ = static_cast<uint8_t*>(xalloc(size_t(frame_bits_) * size_t(channels_)));
    filter_ = static_cast<int16_t(*)[256]>(xalloc(sizeof(int16_t) * 16 * 256));
    window_ = static_cast<uint8_t*>(xalloc(kWindowLead + frame_bits_ + 1));
    mask_ = static_cast<uint64_t*>(xalloc(sizeof(uint64_t) * kGradientPlanes *
                                          (frame_bits_ / 64) * unsigned(channels_)));
    return bits_ && code_ && filter_ && mask_ && window_;
}

// r[k] = sum over channels of sum_i x[i] * x[i-k], with x = +-1 from the bits.
// Equal bits contribute +1 and differing bits -1, so the sum is the sample count
// minus twice the number of differing bits, which is a popcount of an XOR.
void DstEncoder::autocorrelation(double* r) const {
    const unsigned nwords = frame_bits_ / 64;
    const int64_t total = int64_t(frame_bits_) * channels_;

    r[0] = double(total);

    for (unsigned k = 1; k <= kDstFilterLength; k++) {
        int64_t diff = 0;
        for (int ch = 0; ch < channels_; ch++)
            diff += int64_t(differing_bits(bits_ + size_t(ch) * words_per_channel_ + kHistoryWords,
                                           nwords, k));
        r[k] = double(total - 2 * diff);
    }
}

// Levinson-Durbin.  No white noise correction is applied: the recursion is
// bounded by the reflection coefficient check below, and slackening the
// correlation measurably costs prediction accuracy on DSD.
void DstEncoder::design_filter(double* r) {
    double b[kDstFilterLength + 1] = {};
    double tmp[kDstFilterLength + 1];

    double err = r[0];
    unsigned order = 0;

    if (err > 0) {
        for (unsigned m = 1; m <= kDstFilterLength; m++) {
            double acc = r[m];
            for (unsigned i = 1; i < m; i++)
                acc -= b[i] * r[m - i];
            double refl = acc / err;
            if (!(refl > -1.0 && refl < 1.0)) break;

            for (unsigned i = 1; i < m; i++)
                tmp[i] = b[i] - refl * b[m - i];
            for (unsigned i = 1; i < m; i++)
                b[i] = tmp[i];
            b[m] = refl;
            order = m;

            err *= 1.0 - refl * refl;
            if (err <= 0) break;
        }
    }

    for (unsigned i = 0; i < kDstFilterLength; i++)
        weight_[i] = i < order ? b[i + 1] : 0.0;

    // Keep the solution as solved, then smooth a copy of it.  Least squares
    // fits every wrinkle of the measured correlation, including the part that
    // is estimation noise rather than signal; a [1 2 1] pass takes that off and
    // usually predicts slightly better, as well as coding a little cheaper
    // because neighbouring coefficients then differ by less.  Usually, not
    // always - which is why both are kept and refine_filter() tries each.
    memcpy(raw_weight_, weight_, sizeof(weight_));
    double previous = weight_[0];
    for (unsigned i = 1; i + 1 < kDstFilterLength; i++) {
        const double current = weight_[i];
        weight_[i] = 0.25 * previous + 0.5 * current + 0.25 * weight_[i + 1];
        previous = current;
    }

    quantise_filter();
}

// Scales the working filter so its largest tap fills the 9 bit coefficient
// field, which is the finest quantisation the format allows.  A silent or
// pathological frame gives no usable predictor; a zero filter still codes
// correctly, just without prediction.
void DstEncoder::quantise_filter() {
    double peak = 0;
    for (unsigned i = 0; i < kDstFilterLength; i++) {
        double v = weight_[i] < 0 ? -weight_[i] : weight_[i];
        if (v > peak) peak = v;
    }
    const double scale = peak > 0 ? 255.0 / peak : 0.0;

    for (unsigned i = 0; i < kDstFilterLength; i++) {
        int c = int(lround(weight_[i] * scale));
        if (c < -256) c = -256;
        if (c > 255) c = 255;
        coeff_[i] = c;
    }
}



#ifdef __AVX2__

// The prediction, vectorised.
//
// Two things make this possible.  First, byte j of the decoder's shift register
// at sample i is the byte it held at sample i - 8j, so one sliding array of "the
// eight bits before this sample" serves every tap.  Second, and unlike the
// decoder - which cannot look ahead because each bit depends on the one just
// decoded - the encoder knows every bit in advance, so the samples are
// independent of one another and vectorise.
//
// Four taps at a time index a 16 entry table, which is exactly a byte shuffle.
// Their partial sums have to fit in a byte for that, so each coefficient is
// split into its high and low nibble and the two halves are accumulated
// separately: four taps of a nibble reach at most +-64, and recombining them as
// 16 * high + low reproduces the sum exactly.
void DstEncoder::analyse_channel_avx2(int ch) {
    const uint64_t* w = bits_ + size_t(ch) * words_per_channel_ + kHistoryWords;
    uint8_t* code = code_ + size_t(ch) * frame_bits_;
    uint8_t* window = window_ + kWindowLead;

    // Before the frame the register holds the fixed 0xAA history, which is just
    // alternating bits; after that each byte is the previous one shifted up with
    // the newest bit added.
    for (int i = -int(kWindowLead); i <= 0; i++)
        window[i] = (i & 1) ? 0x55 : 0xAA;
    for (unsigned i = 0; i < frame_bits_; i++) {
        const unsigned bit = unsigned(w[i >> 6] >> (63 - (i & 63))) & 1;
        window[i + 1] = uint8_t((window[i] << 1) | bit);
    }

    constexpr unsigned kGroups = kDstFilterLength / 4;
    __m256i high_table[kGroups], low_table[kGroups];
    for (unsigned g = 0; g < kGroups; g++) {
        alignas(16) int8_t high[16], low[16];
        for (unsigned n = 0; n < 16; n++) {
            int sh = 0, sl = 0;
            for (unsigned b = 0; b < 4; b++) {
                const int sign = ((n >> b) & 1) ? 1 : -1;
                sh += sign * (coeff_[4 * g + b] >> 4);
                sl += sign * (coeff_[4 * g + b] & 15);
            }
            high[n] = int8_t(sh);
            low[n] = int8_t(sl);
        }
        high_table[g] = _mm256_broadcastsi128_si256(_mm_load_si128((const __m128i*)high));
        low_table[g] = _mm256_broadcastsi128_si256(_mm_load_si128((const __m128i*)low));
    }

    const __m256i nibble = _mm256_set1_epi8(0x0f);
    const __m256i one = _mm256_set1_epi16(1);
    const __m256i bin_limit = _mm256_set1_epi16(int16_t(kDstMaxProbLength - 1));

    unsigned i = 0;
    for (; i + 32 <= frame_bits_; i += 32) {
        __m256i hi_lo = _mm256_setzero_si256(), hi_hi = _mm256_setzero_si256();
        __m256i lo_lo = _mm256_setzero_si256(), lo_hi = _mm256_setzero_si256();

        for (unsigned g = 0; g < kGroups; g++) {
            const __m256i index = _mm256_and_si256(
                _mm256_loadu_si256((const __m256i*)(window + i - 4 * g)), nibble);
            const __m256i ph = _mm256_shuffle_epi8(high_table[g], index);
            const __m256i pl = _mm256_shuffle_epi8(low_table[g], index);
            hi_lo = _mm256_add_epi16(hi_lo, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(ph)));
            hi_hi = _mm256_add_epi16(hi_hi, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(ph, 1)));
            lo_lo = _mm256_add_epi16(lo_lo, _mm256_cvtepi8_epi16(_mm256_castsi256_si128(pl)));
            lo_hi = _mm256_add_epi16(lo_hi, _mm256_cvtepi8_epi16(_mm256_extracti128_si256(pl, 1)));
        }

        // 16 * high + low, wrapping exactly as the decoder's 16 bit sum does.
        const __m256i predict[2] = {
            _mm256_add_epi16(_mm256_slli_epi16(hi_lo, 4), lo_lo),
            _mm256_add_epi16(_mm256_slli_epi16(hi_hi, 4), lo_hi),
        };

        for (unsigned half = 0; half < 2; half++) {
            const __m256i p = predict[half];
            const __m256i bin = _mm256_min_epi16(
                _mm256_srli_epi16(_mm256_abs_epi16(p), 3), bin_limit);
            const __m256i negative = _mm256_srli_epi16(p, 15);
            const __m256i actual = _mm256_and_si256(
                _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)(window + i + 1 + half * 16))),
                one);
            const __m256i correct = _mm256_xor_si256(actual, negative);
            const __m256i packed = _mm256_or_si256(bin, _mm256_slli_epi16(correct, 7));

            const __m256i bytes = _mm256_packus_epi16(packed, packed);
            _mm_storel_epi64((__m128i*)(code + i + half * 16), _mm256_castsi256_si128(bytes));
            _mm_storel_epi64((__m128i*)(code + i + half * 16 + 8),
                             _mm256_extracti128_si256(bytes, 1));
        }
    }

    for (; i < frame_bits_; i++) {
        int sum = 0;
        for (unsigned t = 0; t < kDstFilterLength; t++)
            sum += ((window[i - 4 * (t / 4)] >> (t % 4)) & 1) ? coeff_[t] : -coeff_[t];
        const int16_t predict = int16_t(sum);
        unsigned bin = unsigned(predict < 0 ? -predict : predict) >> 3;
        if (bin >= kDstMaxProbLength) bin = kDstMaxProbLength - 1;
        const unsigned correct = (unsigned(window[i + 1]) ^ unsigned(predict >> 15)) & 1;
        code[i] = uint8_t(bin | (correct << 7));
    }
}

#endif // __AVX2__

// Runs the decoder's prediction over one channel, recording for every sample
// which probability bin it falls in and whether the prediction was wrong.
#ifndef __AVX2__

void DstEncoder::analyse_channel(int ch) {
    const uint64_t* w = bits_ + size_t(ch) * words_per_channel_;
    uint8_t* code = code_ + size_t(ch) * frame_bits_;
    const int16_t (*filter)[256] = filter_;

    uint64_t lo = 0xAAAAAAAAAAAAAAAAull, hi = 0xAAAAAAAAAAAAAAAAull;

    for (unsigned i = 0; i < frame_bits_; i++) {
#define F(x) filter[(x)][((((x) < 8 ? lo : hi) >> (8 * ((x) & 7))) & 0xFF)]
        const int16_t predict = int16_t(F( 0) + F( 1) + F( 2) + F( 3) +
                                        F( 4) + F( 5) + F( 6) + F( 7) +
                                        F( 8) + F( 9) + F(10) + F(11) +
                                        F(12) + F(13) + F(14) + F(15));
#undef F
        const unsigned v = unsigned(w[kHistoryWords + (i >> 6)] >> (63 - (i & 63))) & 1;

        unsigned bin = unsigned(predict < 0 ? -predict : predict) >> 3;
        if (bin >= kDstMaxProbLength) bin = kDstMaxProbLength - 1;

        // The decoder forms the bit as (predict >> 15) ^ residual, so a residual
        // of 1 means the prediction was right.
        const unsigned correct = (v ^ (unsigned(predict >> 15) & 1)) & 1;
        code[i] = uint8_t(bin | (correct << 7));

        hi = (hi << 1) | (lo >> 63);
        lo = (lo << 1) | v;
    }
}

#endif // !__AVX2__

// Runs the current filter over every channel and gathers the per-bin statistics
// the probability table is built from.  The samples whose history is still the
// fixed 0xAA pattern rather than signal are coded at even odds, so they are left
// out of the statistics.
bool DstEncoder::analyse_frame() {
    // Only the scalar prediction reads this table; the vector path indexes the
    // coefficients four at a time instead.  It is still worth knowing that the
    // decoder can build its own copy, but that cannot fail here: eight taps of
    // at most 256 sum to 2048, well inside the 16 bits it has to fit.
#ifndef __AVX2__
    if (!build_filter_lut(coeff_, kDstFilterLength, filter_))
        return false;
#endif

    memset(bin_count_, 0, sizeof(bin_count_));
    memset(bin_errors_, 0, sizeof(bin_errors_));

    for (int ch = 0; ch < channels_; ch++) {
#ifdef __AVX2__
        analyse_channel_avx2(ch);
#else
        analyse_channel(ch);
#endif
        const uint8_t* code = code_ + size_t(ch) * frame_bits_;
        for (unsigned i = kDstFilterLength; i < frame_bits_; i++) {
            const unsigned bin = code[i] & 0x7F;
            bin_count_[bin]++;
            bin_errors_[bin] += (code[i] >> 7) ^ 1;
        }
    }
    return true;
}

// Taps past the last non-zero coefficient contribute nothing to the prediction,
// so the filter can be sent that much shorter for free.
unsigned DstEncoder::trimmed_length() const {
    unsigned n = kDstFilterLength;
    while (n > 1 && coeff_[n - 1] == 0)
        n--;
    return n;
}

// Bits the frame will cost: the arithmetic coded payload plus the filter itself.
// Refinement makes the coefficients less smooth, which makes them more expensive
// to transmit, so that has to be part of the score or the refinement trades away
// more than it gains.
//
// Sending a filter shorter than its non-zero span was also tried, in case a
// quiet frame would rather spend its bits elsewhere; it never paid for itself,
// so only the free trim above is applied.
double DstEncoder::estimate_cost() const {
    double cost = double(code_table(nullptr, coeff_, trimmed_length(),
                                    kFsetsPredCoeff, 7, 9, 0));
    for (unsigned j = 0; j < kDstMaxProbLength; j++)
        cost += bin_cost(bin_count_[j], bin_errors_[j],
                         quantise_prob(bin_count_[j], bin_errors_[j]));
    return cost;
}

// Maximum likelihood (logistic) gradient step.
//
// The decoder turns the prediction into a bit by taking its sign, so the filter
// is a binary classifier, and the gradient of its likelihood is
//
//     sum over samples of  P(model gets this one wrong) * target * history
//
// Least squares, which is what Levinson-Durbin gives, fits the magnitude of the
// prediction instead, and that is measurably not the same filter.  The
// probability bin a sample landed in already measures how often the model is
// wrong there, so it supplies the weight directly: a sample that was predicted
// wrongly is weighted by one minus that, one predicted rightly by the rate
// itself, and both push the filter towards the bit that actually occurred.
//
// The weights are quantised to a few levels and laid out as bitplanes, which
// turns the weighted correlation into one masked popcount per plane - the same
// trick the autocorrelation uses.
void DstEncoder::gradient_step(int iteration) {
    const unsigned nwords = frame_bits_ / 64;
    const unsigned levels = (1u << kGradientPlanes) - 1;

    // Weight for every possible code byte, so the per-sample loop below is a
    // single lookup rather than a probability computation.
    uint8_t weight_of[256];
    memset(weight_of, 0, sizeof(weight_of));
    for (unsigned bin = 0; bin < kDstMaxProbLength; bin++) {
        const double rate = bin_count_[bin]
                                ? double(bin_errors_[bin]) / double(bin_count_[bin])
                                : 0.5;
        for (unsigned right = 0; right < 2; right++) {
            long q = lround((right ? rate : 1.0 - rate) * double(levels));
            if (q < 0) q = 0;
            if (q > long(levels)) q = long(levels);
            weight_of[bin | (right << 7)] = uint8_t(q);
        }
    }

    double total = 0;
    for (int ch = 0; ch < channels_; ch++)
        total += double(pack_planes(code_ + size_t(ch) * frame_bits_, weight_of,
                                    mask_ + size_t(ch) * kGradientPlanes * nwords, nwords));

    if (total <= 0) return;

    double step = kRefineStep;
    for (int i = 0; i < iteration; i++)
        step *= kRefineDecay;

    for (unsigned t = 0; t < kDstFilterLength; t++) {
        uint64_t disagree = 0;
        for (int ch = 0; ch < channels_; ch++)
            disagree += disagreement(bits_ + size_t(ch) * words_per_channel_ + kHistoryWords,
                                     mask_ + size_t(ch) * kGradientPlanes * nwords,
                                     nwords, t + 1);
        // Weight agreeing with the target counts positive, disagreeing negative.
        weight_[t] += step * (total - 2.0 * double(disagree)) / total;
    }

    quantise_filter();
}

// The refinement is a local search, so where it ends up depends on where it
// starts.  The smoothed and unsmoothed designs sit in different basins and
// neither wins reliably - on the test material each is better than the other on
// a third of frames - so both are followed and the cheaper result kept.
bool DstEncoder::refine_filter() {
    int smoothed_coeff[kDstFilterLength];

    if (!refine_from_start()) return false;
    const double smoothed_cost = estimate_cost();
    memcpy(smoothed_coeff, coeff_, sizeof(coeff_));

    memcpy(weight_, raw_weight_, sizeof(weight_));
    quantise_filter();
    if (!refine_from_start()) return false;

    if (estimate_cost() > smoothed_cost) {
        memcpy(coeff_, smoothed_coeff, sizeof(coeff_));
        return analyse_frame();
    }
    return true;
}

// Iterates the refinement from wherever weight_ currently is, keeping whichever
// filter codes cheapest.  Leaves the analysis in code_ and the bin counters
// matching the filter it settles on.
bool DstEncoder::refine_from_start() {
    int best_coeff[kDstFilterLength];
    double best_cost = 0;
    bool analysis_is_best = false;

    for (int it = 0; it <= kRefineIterations; it++) {
        if (!analyse_frame()) return false;

        const double cost = estimate_cost();
        if (it == 0 || cost < best_cost) {
            best_cost = cost;
            memcpy(best_coeff, coeff_, sizeof(coeff_));
            analysis_is_best = true;
        }
        if (it == kRefineIterations) break;

        gradient_step(it);
        analysis_is_best = false;   // coeff_ has moved on from what was analysed
    }

    if (!analysis_is_best) {
        memcpy(coeff_, best_coeff, sizeof(coeff_));
        return analyse_frame();
    }
    return true;
}

// Chooses the probability table length that minimises the coded size: a longer
// table models the low confidence bins better but costs more to transmit.
void DstEncoder::build_prob_table(unsigned* out_length, int* out_table) const {
    double best_cost = 0;
    unsigned best_length = 1;

    for (unsigned length = 1; length <= kDstMaxProbLength; length++) {
        double cost = 7.0 * double(length) + 7.0;   // rough table cost
        for (unsigned j = 0; j + 1 < length; j++)
            cost += bin_cost(bin_count_[j], bin_errors_[j], quantise_prob(bin_count_[j], bin_errors_[j]));

        uint64_t n = 0, e = 0;
        for (unsigned j = length - 1; j < kDstMaxProbLength; j++) {
            n += bin_count_[j];
            e += bin_errors_[j];
        }
        cost += bin_cost(n, e, quantise_prob(n, e));

        if (length == 1 || cost < best_cost) { best_cost = cost; best_length = length; }
    }

    for (unsigned j = 0; j + 1 < best_length; j++) {
        // Empty bins get their neighbour's value: it costs nothing to code and
        // keeps the table smooth for the differential coding.
        out_table[j] = bin_count_[j] ? int(quantise_prob(bin_count_[j], bin_errors_[j]))
                                     : (j ? out_table[j - 1] : 128);
    }
    uint64_t n = 0, e = 0;
    for (unsigned j = best_length - 1; j < kDstMaxProbLength; j++) {
        n += bin_count_[j];
        e += bin_errors_[j];
    }
    out_table[best_length - 1] = n ? int(quantise_prob(n, e))
                                   : (best_length > 1 ? out_table[best_length - 2] : 128);

    *out_length = best_length;
}

bool DstEncoder::encode(const uint8_t* src, uint8_t* out, size_t out_capacity, size_t* out_size) {
    const unsigned nbytes = frame_bits_ / 8;
    if (out_capacity < max_frame_size())
        return ERR("output buffer too small for a DST frame");

    // Load the frame, preceded by the history the decoder starts from.
    for (int ch = 0; ch < channels_; ch++) {
        uint64_t* w = bits_ + size_t(ch) * words_per_channel_;
        for (unsigned i = 0; i < kHistoryWords; i++)
            w[i] = 0xAAAAAAAAAAAAAAAAull;
        const uint8_t* s = src + size_t(ch) * nbytes;
        for (unsigned i = 0; i < frame_bits_ / 64; i++)
            w[kHistoryWords + i] = rb64(s + size_t(i) * 8);
    }

    double r[kDstFilterLength + 1];
    autocorrelation(r);
    design_filter(r);

    if (!refine_filter())
        return false;

    const unsigned filter_length = trimmed_length();

    unsigned prob_length;
    int prob_table[kDstMaxProbLength];
    build_prob_table(&prob_length, prob_table);

    // The samples whose history is still the fixed 0xAA pattern can either be
    // coded at even odds (one bit each) or through the probability table, which
    // is a per channel choice.  Whichever is cheaper wins; the table was built
    // without them, so this is a genuine comparison rather than a formality.
    unsigned half_prob[kDstMaxChannels];
    for (int ch = 0; ch < channels_; ch++) {
        const uint8_t* code = code_ + size_t(ch) * frame_bits_;
        double table_cost = 0;
        for (unsigned i = 0; i < filter_length && i < frame_bits_; i++) {
            const unsigned bin = (code[i] & 0x7F) < prob_length ? (code[i] & 0x7F)
                                                                : prob_length - 1;
            const double pe = double(prob_table[bin]) / 256.0;
            table_cost += (code[i] >> 7) ? -log2(1.0 - pe) : -log2(pe);
        }
        half_prob[ch] = table_cost > double(filter_length) ? 1u : 0u;
    }

    memset(out, 0, out_capacity);
    BitWriter bw;
    bw.init(out, out_capacity);

    bw.put_bit(1);      // DST coded rather than raw
    bw.put_bit(1);      // same segmentation
    bw.put_bit(1);      // same segmentation for all channels
    bw.put_bit(1);      // end of channel segmentation
    bw.put_bit(1);      // same mapping for filters and probability tables
    bw.put_bit(1);      // one element, every channel maps to it

    for (int ch = 0; ch < channels_; ch++)
        bw.put_bit(half_prob[ch]);

    code_table(&bw, coeff_, filter_length, kFsetsPredCoeff, 7, 9, 0);
    code_table(&bw, prob_table, prob_length, kProbsPredCoeff, 6, 7, 1);

    bw.put_bit(0);      // no arithmetic coder reset

    ArithEncoder ac;
    ac.init(&bw);

    // The decoder opens with a bit it discards; code the cheaper symbol.
    ac.encode(prob_dst_x_bit(coeff_[0]), 1);

    for (unsigned i = 0; i < frame_bits_; i++) {
        for (int ch = 0; ch < channels_; ch++) {
            const uint8_t c = code_[size_t(ch) * frame_bits_ + i];
            const unsigned p = (half_prob[ch] && i < filter_length)
                                   ? 128u
                                   : unsigned(prob_table[(c & 0x7F) < prob_length
                                                             ? (c & 0x7F)
                                                             : prob_length - 1]);
            ac.encode(p, c >> 7);
        }
    }
    ac.flush();

    size_t size = bw.byte_size();

    // Anything the decoder reads past the frame is zero, so trailing zero bytes
    // carry no information and can be dropped.
    while (size > 2 && out[size - 1] == 0)
        size--;

    // A frame that did not compress is stored raw: a zero header byte, then the
    // channel-interleaved DSD.
    if (bw.overflow() || size >= max_frame_size()) {
        out[0] = 0;
        for (unsigned i = 0; i < nbytes; i++)
            for (int ch = 0; ch < channels_; ch++)
                out[1 + size_t(i) * size_t(channels_) + size_t(ch)] = src[size_t(ch) * nbytes + i];
        size = max_frame_size();
        uncoded_frames_++;
    }

    *out_size = size;
    return true;
}

} // namespace dff2dsf

#endif // DFF2DSF_DSTENC_HPP
