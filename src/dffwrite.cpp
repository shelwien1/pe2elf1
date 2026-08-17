// dff2dsf - DSDIFF (.dff) container writer for DST coded frames.

#include "dffwrite.h"
#include "dst.h"

namespace dff2dsf {

namespace {

// DSDIFF channel identifiers, by channel count.
const char* const kChannelIds[kDstMaxChannels + 1][kDstMaxChannels] = {
    {},
    { "C   " },
    { "SLFT", "SRGT" },
    { "MLFT", "MRGT", "C   " },
    { "MLFT", "MRGT", "LS  ", "RS  " },
    { "MLFT", "MRGT", "C   ", "LS  ", "RS  " },
    { "MLFT", "MRGT", "C   ", "LFE ", "LS  ", "RS  " },
};

// Loudspeaker configuration codes matching those channel layouts.
unsigned lsco_for(int channels) {
    switch (channels) {
    case 5:  return 3;   // 5 channel
    case 6:  return 4;   // 5 channel + LFE
    default: return 0;   // 2 channel stereo, and anything else undefined
    }
}

void put_tag(uint8_t*& p, const char* tag) {
    memcpy(p, tag, 4);
    p += 4;
}

void put_be64(uint8_t*& p, uint64_t v) {
    for (int i = 7; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be32(uint8_t*& p, uint32_t v) {
    for (int i = 3; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be16(uint8_t*& p, uint16_t v) {
    *p++ = uint8_t(v >> 8);
    *p++ = uint8_t(v);
}

} // namespace

bool DffWriter::open(const char* path, int channels, unsigned dsd_rate) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d", channels);
    if (!f_.open_write(path)) return false;

    uint8_t hdr[256];
    uint8_t* p = hdr;

    put_tag(p, "FRM8");
    frm8_size_pos_ = p - hdr;
    put_be64(p, 0);                       // patched in finish()
    put_tag(p, "DSD ");

    put_tag(p, "FVER");
    put_be64(p, 4);
    put_be32(p, 0x01050000);              // DSDIFF 1.5.0.0

    // PROP/SND: sample rate, channels, compression, start time, speaker config.
    const unsigned chnl_size = 2 + 4 * unsigned(channels);
    const unsigned prop_size = 4 +                 // "SND "
                               12 + 4 +            // FS
                               12 + chnl_size + (chnl_size & 1) +
                               12 + 20 +           // CMPR
                               12 + 8 +            // ABSS
                               12 + 2;             // LSCO
    put_tag(p, "PROP");
    put_be64(p, prop_size);
    put_tag(p, "SND ");

    put_tag(p, "FS  ");
    put_be64(p, 4);
    put_be32(p, dsd_rate);

    put_tag(p, "CHNL");
    put_be64(p, chnl_size);
    put_be16(p, uint16_t(channels));
    for (int i = 0; i < channels; i++)
        put_tag(p, kChannelIds[channels][i]);
    if (chnl_size & 1) *p++ = 0;

    put_tag(p, "CMPR");
    put_be64(p, 20);
    put_tag(p, "DST ");
    *p++ = 14;
    memcpy(p, "DST compressed", 14);
    p += 14;
    *p++ = 0;                             // pad to the even size above

    put_tag(p, "ABSS");
    put_be64(p, 8);
    put_be16(p, 0);                       // hours
    *p++ = 0;                             // minutes
    *p++ = 0;                             // seconds
    put_be32(p, 0);                       // samples

    put_tag(p, "LSCO");
    put_be64(p, 2);
    put_be16(p, uint16_t(lsco_for(channels)));

    put_tag(p, "DST ");
    dst_size_pos_ = p - hdr;
    put_be64(p, 0);                       // patched in finish()

    put_tag(p, "FRTE");
    put_be64(p, 6);
    frte_count_pos_ = p - hdr;
    put_be32(p, 0);                       // patched in finish()
    put_be16(p, 75);                      // DST frames are always 1/75 s

    if (!f_.write(hdr, size_t(p - hdr))) return false;
    dst_body_pos_ = dst_size_pos_ + 8;   // first byte of the DST chunk body
    return true;
}

bool DffWriter::write_frame(const uint8_t* data, size_t size) {
    uint8_t hdr[12];
    uint8_t* p = hdr;
    put_tag(p, "DSTF");
    put_be64(p, size);

    if (!f_.write(hdr, sizeof(hdr))) return false;
    if (!f_.write(data, size)) return false;
    if (size & 1) {
        const uint8_t pad = 0;
        if (!f_.write(&pad, 1)) return false;
    }
    frames_++;
    return true;
}

bool DffWriter::finish() {
    const int64_t end = f_.tell();

    uint8_t v[8];
    uint8_t* p;

    p = v; put_be64(p, uint64_t(end - 12));
    if (!f_.seek(frm8_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be64(p, uint64_t(end - dst_body_pos_));
    if (!f_.seek(dst_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be32(p, uint32_t(frames_));
    if (!f_.seek(frte_count_pos_) || !f_.write(v, 4)) return false;

    f_.close();
    return true;
}

} // namespace dff2dsf
