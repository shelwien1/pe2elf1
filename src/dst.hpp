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

inline void ac_init(ArithCoderP ac, BitReader& br) {
    ac->a = 4095;
    ac->c = br.get(12);
}

// Binary arithmetic decoder (10.11).  `p` is the probability, scaled to 1..128,
// that the decoded bit equals the prediction.
inline void ac_get(ArithCoderP ac, BitReader& br, uint32_t p, UIntP e) {
    uint32_t k = (ac->a >> 8) | ((ac->a >> 7) & 1);
    uint32_t q = k * p;
    uint32_t a_q = ac->a - q;

    *e = ac->c < a_q;
    if (*e) {
        ac->a = a_q;
    } else {
        ac->a = q;
        ac->c -= a_q;
    }

    if (ac->a < 2048) {
        int32_t n = 11 - ilog2(ac->a);
        ac->a <<= n;
        ac->c = (ac->c << n) | br.get(n);
    }
}

void read_uncoded_coeff(BitReader& br, IntP dst, uint32_t elements,
                        int32_t coeff_bits, bool is_signed, int32_t offset) {
    for (uint32_t i = 0; i < elements; i++)
        dst[i] = (is_signed ? br.get_signed(coeff_bits) : int32_t(br.get(coeff_bits))) + offset;
}

} // namespace

bool DstDecoder::init(int32_t channels, uint32_t dsd_rate) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d (DST allows up to %d)", channels, kDstMaxChannels);

    // ISO/IEC 14496-3 only allows 64, 128 and 256 times 44100.  Be a little more
    // permissive, but keep the bound the working buffers are sized for.
    if (!dsd_rate || dsd_rate > kMaxDsdRate || dsd_rate % 44100u)
        return ERR("unsupported sample rate %u", dsd_rate);

    frame_bits_ = frame_bits_for(dsd_rate);
    if (frame_bits_ & 7)
        return ERR("frame size of %u bits is not a whole number of bytes", frame_bits_);

    channels_ = channels;
    uncoded_frames_ = 0;
    return true;
}

// Channel to filter/probability element mapping (10.7 - 10.9).
bool DstDecoder::read_map(BitReader& br, Table& t, UIntP map) {
    t.elements = 1;
    map[0] = 0;
    if (!br.get1()) {
        for (int32_t ch = 1; ch < channels_; ch++) {
            int32_t bits = ilog2(t.elements) + 1;
            map[ch] = br.get(bits);
            if (map[ch] == t.elements) {
                t.elements++;
                if (t.elements >= uint32_t(kDstMaxElements))
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
bool DstDecoder::read_table(BitReader& br, Table& t, CPredP pred,
                            int32_t length_bits, int32_t coeff_bits, bool is_signed, int32_t offset) {
    for (uint32_t i = 0; i < t.elements; i++) {
        t.length[i] = br.get(length_bits) + 1;
        if (!br.get1()) {
            read_uncoded_coeff(br, t.coeff[i], t.length[i], coeff_bits, is_signed, offset);
        } else {
            uint32_t method = br.get(2);
            if (method == 3)
                return ERR("invalid table prediction method");

            read_uncoded_coeff(br, t.coeff[i], method + 1, coeff_bits, is_signed, offset);

            uint32_t lsb_size = br.get(3);
            for (uint32_t j = method + 1; j < t.length[i]; j++) {
                int32_t x = 0;
                for (uint32_t k = 0; k < method + 1; k++)
                    x += pred[method][k] * int32_t(uint32_t(t.coeff[i][j - k - 1]));
                int32_t c = get_sr_golomb_dst(br, lsb_size);
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
bool build_filter_lut(CIntP coeff, uint32_t length, LutP lut) {
    for (int32_t j = 0; j < 16; j++) {
        int32_t total = int32_t(length) - j * 8;
        if (total < 0) total = 0;
        if (total > 8) total = 8;

        for (int32_t k = 0; k < 256; k++) {
            int64_t v = 0;
            for (int32_t l = 0; l < total; l++)
                v += (((k >> l) & 1) * 2 - 1) * coeff[j * 8 + l];
            if (int16_t(v) != v)
                return ERR("filter coefficient overflow");
            lut[j][k] = int16_t(v);
        }
    }
    return true;
}

bool DstDecoder::build_filter() {
    for (uint32_t i = 0; i < fsets_.elements; i++)
        if (!build_filter_lut(fsets_.coeff[i], fsets_.length[i], filter_[i]))
            return false;
    return true;
}

bool DstDecoder::decode(CByteP data, size_t size, size_t capacity, ByteP out) {
    const int32_t channels = channels_;
    const uint32_t samples = frame_bits_;
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
    uint32_t map_ch_to_felem[kDstMaxChannels];
    uint32_t map_ch_to_pelem[kDstMaxChannels];

    uint32_t same_map = br.get1();

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
    uint32_t half_prob[kDstMaxChannels];
    for (int32_t ch = 0; ch < channels; ch++)
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

    uint32_t dst_x_bit;
    ac_get(&ac_, br, prob_dst_x_bit(fsets_.coeff[0][0]), &dst_x_bit);

    for (uint32_t i = 0; i < samples; i++) {
        const ByteP out_byte = out + size_t(i >> 3) * uint32_t(channels);
        const uint32_t out_shift = 7 - (i & 7);

        for (int32_t ch = 0; ch < channels; ch++) {
            const uint32_t felem = map_ch_to_felem[ch];
            const CLutP filter = filter_[felem];
            const WordP status = status_[ch];

            const uint64_t lo = status[0], hi = status[1];
#define F(x) filter[(x)][((((x) < 8 ? lo : hi) >> (8 * ((x) & 7))) & 0xFF)]
            const int16_t predict = int16_t(F( 0) + F( 1) + F( 2) + F( 3) +
                                            F( 4) + F( 5) + F( 6) + F( 7) +
                                            F( 8) + F( 9) + F(10) + F(11) +
                                            F(12) + F(13) + F(14) + F(15));
#undef F

            uint32_t prob;
            if (!half_prob[ch] || i >= fsets_.length[felem]) {
                uint32_t pelem = map_ch_to_pelem[ch];
                uint32_t index = uint32_t(predict < 0 ? -predict : predict) >> 3;
                uint32_t last = probs_.length[pelem] - 1;
                prob = uint32_t(probs_.coeff[pelem][index < last ? index : last]);
            } else {
                prob = 128;
            }

            uint32_t residual;
            ac_get(&ac_, br, prob, &residual);
            uint32_t v = ((predict >> 15) ^ int32_t(residual)) & 1;
            out_byte[ch] |= uint8_t(v << out_shift);

            status[1] = (hi << 1) | (lo >> 63);
            status[0] = (lo << 1) | v;
        }
    }

    return true;
}

} // namespace dff2dsf

#endif // DFF2DSF_DST_HPP
