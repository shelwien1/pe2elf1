// dff2dsf - the DSDIFF frame CRC, carried in DSTC chunks.
//
// DSDIFF 1.5, section 3.4.3, defines a four byte CRC over the *uncompressed*
// interleaved DSD of each frame:
//
//     CRC(x) = (x^32 . I(x)) mod2long G(x),   G(x) = x^32 + x^31 + x^4 + 1
//
// where I(x) is the frame's DSD bits taken most significant bit of the first
// byte first, and the recipe given alongside it is "append 32 zero bits, divide
// by G modulo 2, the remainder is the CRC".  That is an ordinary MSB-first CRC
// with generator 0x80000011 (the x^32 term implicit), zero initial value, no
// reflection and no final inversion, stored most significant byte first.
#ifndef DFF2DSF_CRC_H
#define DFF2DSF_CRC_H

#include "common.h"

namespace dff2dsf {

constexpr uint32_t kDsdCrcPoly = 0x80000011;   // x^31 + x^4 + 1, x^32 implied

// One byte's worth of the division, tabulated at compile time.
struct DsdCrcTable {
    uint32_t v[256];
    constexpr DsdCrcTable() : v() {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i << 24;
            for (int32_t b = 0; b < 8; b++)
                c = (c & 0x80000000u) ? (c << 1) ^ kDsdCrcPoly : c << 1;
            v[i] = c;
        }
    }
};
inline constexpr DsdCrcTable kDsdCrc{};

inline uint32_t dsd_crc(uint32_t crc, CByteP p, size_t n) {
    for (size_t i = 0; i < n; i++)
        crc = (crc << 8) ^ kDsdCrc.v[((crc >> 24) ^ p[i]) & 0xFF];
    return crc;
}

// The same over planar DSD, which is how the encoder holds a frame: the CRC is
// defined over the interleaved order, so the channels are woven together here
// rather than by writing out an interleaved copy first.
inline uint32_t dsd_crc_planar(CByteP planar, size_t bytes_per_channel,
                               int32_t channels) {
    uint32_t crc = 0;
    for (size_t i = 0; i < bytes_per_channel; i++)
        for (int32_t ch = 0; ch < channels; ch++) {
            const uint8_t b = planar[size_t(ch) * bytes_per_channel + i];
            crc = (crc << 8) ^ kDsdCrc.v[((crc >> 24) ^ b) & 0xFF];
        }
    return crc;
}

} // namespace dff2dsf

#endif // DFF2DSF_CRC_H
