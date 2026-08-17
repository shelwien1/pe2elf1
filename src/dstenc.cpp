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

#include "dstenc.h"
#include "bitwrite.h"

#include <math.h>

namespace dff2dsf {

namespace {

// Same fixed predictors the decoder applies when reading back the tables.
constexpr int8_t kFsetsPredCoeff[3][3] = {
    {  -8,  0, 0 },
    { -16,  8, 0 },
    {  -9, -5, 6 },
};

constexpr int8_t kProbsPredCoeff[3][3] = {
    {  -8,  0,  0 },
    { -16,  8,  0 },
    { -24, 24, -8 },
};

struct ReverseTable {
    uint8_t v[256];
    constexpr ReverseTable() : v() {
        for (int i = 0; i < 256; i++) {
            unsigned r = 0;
            for (int b = 0; b < 8; b++)
                r |= ((unsigned(i) >> b) & 1) << (7 - b);
            v[i] = uint8_t(r);
        }
    }
};
constexpr ReverseTable kReverse{};

inline unsigned prob_dst_x_bit(int c) {
    return unsigned((kReverse.v[c & 127] >> 1) + 1);
}

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

inline unsigned quantise_prob(uint64_t n, uint64_t errors) {
    if (!n) return 128;
    int p = int(lround(256.0 * double(errors) / double(n)));
    if (p < 1) p = 1;
    if (p > 128) p = 128;
    return unsigned(p);
}

} // namespace

DstEncoder::~DstEncoder() {
    free(bits_);
    free(code_);
    free(filter_);
    free(mask_);
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
    mask_ = static_cast<uint64_t*>(xalloc(sizeof(uint64_t) * kGradientPlanes *
                                          (frame_bits_ / 64) * unsigned(channels_)));
    return bits_ && code_ && filter_ && mask_;
}

// r[k] = sum over channels of sum_i x[i] * x[i-k], with x = +-1 from the bits.
// Equal bits contribute +1 and differing bits -1, so the sum is the sample count
// minus twice the number of differing bits, which is a popcount of an XOR.
void DstEncoder::autocorrelation(double* r) const {
    const unsigned nwords = frame_bits_ / 64;
    const int64_t total = int64_t(frame_bits_) * channels_;

    r[0] = double(total);

    for (unsigned k = 1; k <= kDstFilterLength; k++) {
        const unsigned wshift = k >> 6;
        const unsigned bshift = k & 63;
        int64_t diff = 0;

        for (int ch = 0; ch < channels_; ch++) {
            const uint64_t* w = bits_ + size_t(ch) * words_per_channel_;
            for (unsigned j = 0; j < nwords; j++) {
                const uint64_t x = w[kHistoryWords + j];
                const unsigned base = kHistoryWords + j - wshift;
                const uint64_t y = bshift ? ((w[base - 1] << (64 - bshift)) | (w[base] >> bshift))
                                          : w[base];
                diff += __builtin_popcountll(x ^ y);
            }
        }
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

// Runs the decoder's prediction over one channel, recording for every sample
// which probability bin it falls in and whether the prediction was wrong.
void DstEncoder::analyse_channel(int ch) {
    const uint64_t* w = bits_ + size_t(ch) * words_per_channel_;
    uint8_t* code = code_ + size_t(ch) * frame_bits_;
    const int16_t (*filter)[256] = filter_;

    uint64_t lo = 0xAAAAAAAAAAAAAAAAull, hi = 0xAAAAAAAAAAAAAAAAull;

    for (unsigned i = 0; i < frame_bits_; i++) {
#define F(x) filter[(x)][((x) < 8 ? (lo >> (8 * (x))) : (hi >> (8 * ((x) - 8)))) & 0xFF]
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

// Runs the current filter over every channel and gathers the per-bin statistics
// the probability table is built from.  The samples whose history is still the
// fixed 0xAA pattern rather than signal are coded at even odds, so they are left
// out of the statistics.
bool DstEncoder::analyse_frame() {
    if (!build_filter_lut(coeff_, kDstFilterLength, filter_))
        return false;

    memset(bin_count_, 0, sizeof(bin_count_));
    memset(bin_errors_, 0, sizeof(bin_errors_));

    for (int ch = 0; ch < channels_; ch++) {
        analyse_channel(ch);
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

    memset(mask_, 0, sizeof(uint64_t) * kGradientPlanes * size_t(channels_) * nwords);
    double total = 0;

    for (int ch = 0; ch < channels_; ch++) {
        const uint8_t* code = code_ + size_t(ch) * frame_bits_;
        uint64_t* planes = mask_ + size_t(ch) * kGradientPlanes * nwords;
        for (unsigned i = 0; i < frame_bits_; i++) {
            const unsigned q = weight_of[code[i]];
            if (!q) continue;
            total += double(q);
            const uint64_t bit = 1ull << (63 - (i & 63));
            for (unsigned p = 0; p < kGradientPlanes; p++)
                if ((q >> p) & 1) planes[p * nwords + (i >> 6)] |= bit;
        }
    }
    if (total <= 0) return;

    double step = kRefineStep;
    for (int i = 0; i < iteration; i++)
        step *= kRefineDecay;

    for (unsigned t = 0; t < kDstFilterLength; t++) {
        const unsigned k = t + 1;
        const unsigned wshift = k >> 6;
        const unsigned bshift = k & 63;
        double disagree = 0;

        for (int ch = 0; ch < channels_; ch++) {
            const uint64_t* w = bits_ + size_t(ch) * words_per_channel_;
            const uint64_t* planes = mask_ + size_t(ch) * kGradientPlanes * nwords;
            uint64_t sum[kGradientPlanes] = {};

            for (unsigned j = 0; j < nwords; j++) {
                const uint64_t x = w[kHistoryWords + j];
                const unsigned base = kHistoryWords + j - wshift;
                const uint64_t y = bshift ? ((w[base - 1] << (64 - bshift)) | (w[base] >> bshift))
                                          : w[base];
                const uint64_t disagreement = x ^ y;
                for (unsigned p = 0; p < kGradientPlanes; p++)
                    sum[p] += unsigned(__builtin_popcountll(planes[p * nwords + j] & disagreement));
            }
            for (unsigned p = 0; p < kGradientPlanes; p++)
                disagree += double(sum[p] << p);
        }
        // Weight agreeing with the target counts positive, disagreeing negative.
        weight_[t] += step * (total - 2.0 * disagree) / total;
    }

    quantise_filter();
}

// Iterates the refinement, keeping whichever filter codes cheapest.  Leaves the
// analysis in code_ and the bin counters matching the filter it settles on.
bool DstEncoder::refine_filter() {
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
