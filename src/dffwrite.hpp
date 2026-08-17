// dff2dsf - DSDIFF (.dff) container writer for DST coded frames.

#ifndef DFF2DSF_DFFWRITE_HPP
#define DFF2DSF_DFFWRITE_HPP

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

void put_tag(ByteP& p, const char* tag) {
    memcpy(p, tag, 4);
    p += 4;
}

void put_be64(ByteP& p, uint64_t v) {
    for (int i = 7; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be32(ByteP& p, uint32_t v) {
    for (int i = 3; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be16(ByteP& p, uint16_t v) {
    *p++ = uint8_t(v >> 8);
    *p++ = uint8_t(v);
}

// What this encoder writes into the file's comment chunk, the way the reference
// encoder records the tool that produced the file.
const char kCreatingMachine[] = "dff2dsf DST encoder";

} // namespace

bool DffWriter::open(const char* path, int channels, unsigned dsd_rate,
                     const DffWriteOptions& options) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d", channels);
    opt_ = options;
    channels_ = channels;
    if (!f_.open_write(path)) return false;

    uint8_t hdr[256];
    ByteP p = hdr;

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
                               (opt_.abss ? 12 + 8 : 0) +
                               (opt_.lsco ? 12 + 2 : 0);
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

    if (opt_.abss) {
        put_tag(p, "ABSS");
        put_be64(p, 8);
        put_be16(p, 0);                   // hours
        *p++ = 0;                         // minutes
        *p++ = 0;                         // seconds
        put_be32(p, 0);                   // samples
    }

    if (opt_.lsco) {
        put_tag(p, "LSCO");
        put_be64(p, 2);
        put_be16(p, uint16_t(lsco_for(channels)));
    }

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

bool DffWriter::write_frame(CByteP data, size_t size,
                            CByteP dsd, size_t dsd_bytes_per_channel) {
    uint8_t hdr[12];
    ByteP p = hdr;
    put_tag(p, "DSTF");
    put_be64(p, size);

    if (!f_.write(hdr, sizeof(hdr))) return false;
    if (!f_.write(data, size)) return false;
    if (size & 1) {
        const uint8_t pad = 0;
        if (!f_.write(&pad, 1)) return false;
    }

    // DSTC belongs immediately after the frame it covers, and covers the DSD
    // that went in rather than the DST that came out.
    if (opt_.dstc) {
        if (!dsd) return ERR("DSTC is enabled but the frame's DSD was not given");
        uint8_t crc[16];
        p = crc;
        put_tag(p, "DSTC");
        put_be64(p, 4);
        put_be32(p, dsd_crc_planar(dsd, dsd_bytes_per_channel, channels_));
        if (!f_.write(crc, sizeof(crc))) return false;
    }

    frames_++;
    return true;
}

// DSTI: one entry per frame - where its data starts and how long it is - so a
// player can seek without walking every chunk.
//
// The entries are recovered by walking the DST chunk just written rather than
// remembered as the frames go by.  A remembered index is the one thing here
// whose size the format does not bound: 12 bytes a frame is 229 KB for the test
// file but grows without limit with duration, and it would be the only
// allocation left in a single threaded encode.  Walking costs one 12 byte read
// per frame, sequential and against pages that were written moments ago.
bool DffWriter::write_index(int64_t dst_end) {
    if (!opt_.dsti || !frames_) return true;

    uint8_t hdr[12];
    ByteP p = hdr;
    put_tag(p, "DSTI");
    put_be64(p, frames_ * 12);
    if (!f_.seek(dst_end) || !f_.write(hdr, sizeof(hdr))) return false;

    constexpr unsigned kBatch = 256;         // entries buffered between writes
    uint8_t buf[kBatch * 12];
    unsigned n = 0;
    uint64_t seen = 0;
    int64_t pos = dst_body_pos_;             // the walk, over FRTE, DSTF, DSTC
    int64_t out = dst_end + 12;              // where the next entries go

    while (pos + 12 <= dst_end) {
        uint8_t ck[12];
        if (f_.tell() != pos && !f_.seek(pos)) return false;
        if (!f_.read(ck, sizeof(ck))) return false;
        const uint64_t size = rb64(ck + 4);

        if (tag_is(ck, "DSTF")) {
            ByteP q = buf + size_t(n) * 12;
            put_be64(q, uint64_t(pos) + 12);
            put_be32(q, uint32_t(size));
            seen++;
            if (++n == kBatch) {
                if (!f_.seek(out) || !f_.write(buf, sizeof(buf))) return false;
                out += int64_t(sizeof(buf));
                n = 0;
            }
        }
        pos += 12 + int64_t(size) + int64_t(size & 1);
    }

    if (n) {
        if (!f_.seek(out) || !f_.write(buf, size_t(n) * 12)) return false;
        out += int64_t(n) * 12;
    }

    if (seen != frames_)
        return ERR("frame index found %llu frames, expected %llu",
                   (unsigned long long)seen, (unsigned long long)frames_);

    // The comment chunk is written next, and follows the index.
    return f_.seek(out);
}

// COMT: a single file-history comment naming the encoder, timestamped now.
bool DffWriter::write_comment() {
    if (!opt_.comt) return true;

    const uint32_t text_len = uint32_t(sizeof(kCreatingMachine) - 1);
    const uint32_t padded = text_len + (text_len & 1);
    const uint64_t size = 2 + 14 + padded;

    struct tm utc;
    if (!utc_now(&utc)) return ERR("cannot read the current time");

    uint8_t buf[128];
    ByteP p = buf;
    put_tag(p, "COMT");
    put_be64(p, size);
    put_be16(p, 1);                                   // one comment
    put_be16(p, uint16_t(utc.tm_year + 1900));
    *p++ = uint8_t(utc.tm_mon + 1);
    *p++ = uint8_t(utc.tm_mday);
    *p++ = uint8_t(utc.tm_hour);
    *p++ = uint8_t(utc.tm_min);
    put_be16(p, 3);                                   // comment type: file history
    put_be16(p, 2);                                   // reference: creating machine
    put_be32(p, text_len);
    memcpy(p, kCreatingMachine, text_len);
    p += text_len;
    if (text_len & 1) *p++ = 0;

    return f_.write(buf, size_t(p - buf));
}

bool DffWriter::finish() {
    // The sound data ends here; the index and comment follow it, outside the
    // DST chunk but inside the form.
    const int64_t dst_end = f_.tell();

    if (!write_index(dst_end)) return false;
    if (!write_comment()) return false;

    const int64_t end = f_.tell();

    uint8_t v[8];
    ByteP p;

    p = v; put_be64(p, uint64_t(end - 12));
    if (!f_.seek(frm8_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be64(p, uint64_t(dst_end - dst_body_pos_));
    if (!f_.seek(dst_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be32(p, uint32_t(frames_));
    if (!f_.seek(frte_count_pos_) || !f_.write(v, 4)) return false;

    f_.close();
    return true;
}

} // namespace dff2dsf

#endif // DFF2DSF_DFFWRITE_HPP
