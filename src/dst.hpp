// dff2dsf - Direct Stream Transfer (DST) decoder.
// Ported from FFmpeg libavcodec/dstdec.c, Copyright (c) 2014 Peter Ross.
// Licensed LGPL-2.1-or-later; see LICENSE.ffmpeg.
//
// ISO/IEC 14496-3 Part 3 Subpart 10 section numbers are quoted where FFmpeg
// quotes them, so the two sources can be read side by side.

#ifndef DFF2DSF_DST_HPP
#define DFF2DSF_DST_HPP

#include "dst.h"
#include "bits.h"

namespace dff2dsf {

namespace {

inline void ac_init(DstArithCoder* ac, BitReader& br) {
    ac->a = 4095;
    ac->c = br.get(12);
}

// Binary arithmetic decoder (10.11).  `p` is the probability, scaled to 1..128,
// that the decoded bit equals the prediction.
inline void ac_get(DstArithCoder* ac, BitReader& br, unsigned p, unsigned* e) {
    unsigned k = (ac->a >> 8) | ((ac->a >> 7) & 1);
    unsigned q = k * p;
    unsigned a_q = ac->a - q;

    *e = ac->c < a_q;
    if (*e) {
        ac->a = a_q;
    } else {
        ac->a = q;
        ac->c -= a_q;
    }

    if (ac->a < 2048) {
        int n = 11 - ilog2(ac->a);
        ac->a <<= n;
        ac->c = (ac->c << n) | br.get(n);
    }
}

void read_uncoded_coeff(BitReader& br, int* dst, unsigned elements,
                        int coeff_bits, bool is_signed, int offset) {
    for (unsigned i = 0; i < elements; i++)
        dst[i] = (is_signed ? br.get_signed(coeff_bits) : int(br.get(coeff_bits))) + offset;
}

} // namespace

bool DstDecoder::init(int channels, unsigned dsd_rate) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d (DST allows up to %d)", channels, kDstMaxChannels);

    // ISO/IEC 14496-3 only allows 64, 128 and 256 times 44100.  Be a little more
    // permissive, but keep the bound that makes the frame size sane.
    if (dsd_rate > 512u * 44100u || dsd_rate % 44100u)
        return ERR("unsupported sample rate %u", dsd_rate);

    // 588 DSD bits per channel per 44.1kHz-relative unit, i.e. one frame is 1/75 s.
    frame_bits_ = 588u * (dsd_rate / 44100u);
    if (frame_bits_ & 7)
        return ERR("frame size of %u bits is not a whole number of bytes", frame_bits_);

    channels_ = channels;
    uncoded_frames_ = 0;
    return true;
}

// Channel to filter/probability element mapping (10.7 - 10.9).
bool DstDecoder::read_map(BitReader& br, Table& t, unsigned map[kDstMaxChannels]) {
    t.elements = 1;
    map[0] = 0;
    if (!br.get1()) {
        for (int ch = 1; ch < channels_; ch++) {
            int bits = ilog2(t.elements) + 1;
            map[ch] = br.get(bits);
            if (map[ch] == t.elements) {
                t.elements++;
                if (t.elements >= unsigned(kDstMaxElements))
                    return ERR("too many coding elements");
            } else if (map[ch] > t.elements) {
                return ERR("invalid element map");
            }
        }
    } else {
        memset(map, 0, sizeof(*map) * kDstMaxChannels);
    }
    return true;
}

// Filter coefficient sets (10.12) and probability tables (10.13).  Each table is
// either stored verbatim or predicted from its own previous entries with one of
// three fixed predictors, the residual being Golomb coded.
bool DstDecoder::read_table(BitReader& br, Table& t, const int8_t pred[3][3],
                            int length_bits, int coeff_bits, bool is_signed, int offset) {
    for (unsigned i = 0; i < t.elements; i++) {
        t.length[i] = br.get(length_bits) + 1;
        if (!br.get1()) {
            read_uncoded_coeff(br, t.coeff[i], t.length[i], coeff_bits, is_signed, offset);
        } else {
            unsigned method = br.get(2);
            if (method == 3)
                return ERR("invalid table prediction method");

            read_uncoded_coeff(br, t.coeff[i], method + 1, coeff_bits, is_signed, offset);

            unsigned lsb_size = br.get(3);
            for (unsigned j = method + 1; j < t.length[i]; j++) {
                int x = 0;
                for (unsigned k = 0; k < method + 1; k++)
                    x += pred[method][k] * int(unsigned(t.coeff[i][j - k - 1]));
                int c = get_sr_golomb_dst(br, lsb_size);
                if (x >= 0)
                    c -= (x + 4) / 8;
                else
                    c += (-x + 3) / 8;
                if (!is_signed && (c < offset || c >= offset + (1 << coeff_bits)))
                    return ERR("table coefficient out of range");
                t.coeff[i][j] = c;
            }
        }
    }
    return true;
}

// Precomputes the FIR prediction as 16 lookups of 8 taps each: entry [j][k] is
// the contribution of the 8 history bits in byte j when those bits are `k`.
bool build_filter_lut(const int* coeff, unsigned length, int16_t lut[16][256]) {
    for (int j = 0; j < 16; j++) {
        int total = int(length) - j * 8;
        if (total < 0) total = 0;
        if (total > 8) total = 8;

        for (int k = 0; k < 256; k++) {
            int64_t v = 0;
            for (int l = 0; l < total; l++)
                v += (((k >> l) & 1) * 2 - 1) * coeff[j * 8 + l];
            if (int16_t(v) != v)
                return ERR("filter coefficient overflow");
            lut[j][k] = int16_t(v);
        }
    }
    return true;
}

bool DstDecoder::build_filter() {
    for (unsigned i = 0; i < fsets_.elements; i++)
        if (!build_filter_lut(fsets_.coeff[i], fsets_.length[i], filter_[i]))
            return false;
    return true;
}

bool DstDecoder::decode(const uint8_t* data, size_t size, size_t capacity, uint8_t* out) {
    const int channels = channels_;
    const unsigned samples = frame_bits_;
    const size_t out_size = frame_bytes();

    if (size <= 1)
        return ERR("DST frame too short (%zu bytes)", size);

    BitReader br;
    br.init(data, size, capacity);

    memset(out, 0, out_size);

    // An uncompressed frame carries raw DSD after a one byte header (10.3).
    if (!br.get1()) {
        br.skip(1);
        if (br.get(6))
            return ERR("invalid uncompressed frame header");
        size_t n = size - 1;
        if (n > out_size) n = out_size;
        memcpy(out, data + 1, n);
        uncoded_frames_++;
        return true;
    }

    // Segmentation (10.4 - 10.6).  Real encoders only ever emit the default,
    // and the reference decoder this is ported from rejects anything else.
    if (!br.get1())
        return ERR("unsupported frame: not same segmentation");
    if (!br.get1())
        return ERR("unsupported frame: not same segmentation for all channels");
    if (!br.get1())
        return ERR("unsupported frame: not end of channel segmentation");

    // Mapping (10.7 - 10.9).
    unsigned map_ch_to_felem[kDstMaxChannels];
    unsigned map_ch_to_pelem[kDstMaxChannels];

    unsigned same_map = br.get1();

    if (!read_map(br, fsets_, map_ch_to_felem))
        return false;

    if (same_map) {
        probs_.elements = fsets_.elements;
        memcpy(map_ch_to_pelem, map_ch_to_felem, sizeof(map_ch_to_felem));
    } else {
        if (!read_map(br, probs_, map_ch_to_pelem))
            return false;
    }

    // Half probability (10.10).
    unsigned half_prob[kDstMaxChannels];
    for (int ch = 0; ch < channels; ch++)
        half_prob[ch] = br.get1();

    if (!read_table(br, fsets_, kFsetsPredCoeff, 7, 9, true, 0))
        return false;
    if (!read_table(br, probs_, kProbsPredCoeff, 6, 7, false, 1))
        return false;

    // Arithmetic coded data (10.11).
    if (br.get1())
        return ERR("invalid arithmetic coder header");
    ac_init(&ac_, br);

    if (!build_filter())
        return false;

    memset(status_, 0xAA, sizeof(status_));

    unsigned dst_x_bit;
    ac_get(&ac_, br, prob_dst_x_bit(fsets_.coeff[0][0]), &dst_x_bit);

    for (unsigned i = 0; i < samples; i++) {
        uint8_t* out_byte = out + size_t(i >> 3) * unsigned(channels);
        const unsigned out_shift = 7 - (i & 7);

        for (int ch = 0; ch < channels; ch++) {
            const unsigned felem = map_ch_to_felem[ch];
            const int16_t (*filter)[256] = filter_[felem];
            uint64_t* status = status_[ch];

            const uint64_t lo = status[0], hi = status[1];
#define F(x) filter[(x)][((x) < 8 ? (lo >> (8 * (x))) : (hi >> (8 * ((x) - 8)))) & 0xFF]
            const int16_t predict = int16_t(F( 0) + F( 1) + F( 2) + F( 3) +
                                            F( 4) + F( 5) + F( 6) + F( 7) +
                                            F( 8) + F( 9) + F(10) + F(11) +
                                            F(12) + F(13) + F(14) + F(15));
#undef F

            unsigned prob;
            if (!half_prob[ch] || i >= fsets_.length[felem]) {
                unsigned pelem = map_ch_to_pelem[ch];
                unsigned index = unsigned(predict < 0 ? -predict : predict) >> 3;
                unsigned last = probs_.length[pelem] - 1;
                prob = unsigned(probs_.coeff[pelem][index < last ? index : last]);
            } else {
                prob = 128;
            }

            unsigned residual;
            ac_get(&ac_, br, prob, &residual);
            unsigned v = ((predict >> 15) ^ int(residual)) & 1;
            out_byte[ch] |= uint8_t(v << out_shift);

            status[1] = (hi << 1) | (lo >> 63);
            status[0] = (lo << 1) | v;
        }
    }

    return true;
}

} // namespace dff2dsf

#endif // DFF2DSF_DST_HPP
