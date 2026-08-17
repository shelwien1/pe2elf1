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

// What this encoder writes into the file's comment chunk, the way the reference
// encoder records the tool that produced the file.
const char kCreatingMachine[] = "dff2dsf DST encoder";

} // namespace

bool DffWriter::open(const char* path, int channels, unsigned dsd_rate,
                     const DffWriteOptions& options) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d", channels);
    opt_ = options;
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

bool DffWriter::record_frame(int64_t payload_pos, size_t size) {
    if (frames_ == index_capacity_) {
        uint64_t want = index_capacity_ ? index_capacity_ * 2 : 4096;
        IndexEntry* p = static_cast<IndexEntry*>(
            realloc(index_, sizeof(IndexEntry) * size_t(want)));
        if (!p) return ERR("out of memory growing the frame index");
        index_ = p;
        index_capacity_ = want;
    }
    index_[frames_].offset = uint64_t(payload_pos);
    index_[frames_].size = uint32_t(size);
    return true;
}

bool DffWriter::write_frame(const uint8_t* data, size_t size) {
    uint8_t hdr[12];
    uint8_t* p = hdr;
    put_tag(p, "DSTF");
    put_be64(p, size);

    const int64_t chunk_pos = f_.tell();
    if (!f_.write(hdr, sizeof(hdr))) return false;
    if (!f_.write(data, size)) return false;
    if (size & 1) {
        const uint8_t pad = 0;
        if (!f_.write(&pad, 1)) return false;
    }

    // The index points at the frame data, not at its chunk header.  With no
    // index to write there is nothing to remember, so nothing is kept.
    if (opt_.dsti && !record_frame(chunk_pos + 12, size)) return false;
    frames_++;
    return true;
}

// DSTI: one entry per frame, so a player can seek without walking every chunk.
bool DffWriter::write_index() {
    if (!opt_.dsti || !frames_) return true;

    uint8_t hdr[12];
    uint8_t* p = hdr;
    put_tag(p, "DSTI");
    put_be64(p, frames_ * 12);
    if (!f_.write(hdr, sizeof(hdr))) return false;

    // Buffered rather than one write per entry, but not all at once either.
    constexpr uint64_t kBatch = 4096;
    uint8_t* buf = static_cast<uint8_t*>(xalloc(size_t(kBatch) * 12));
    if (!buf) return false;

    bool ok = true;
    for (uint64_t i = 0; i < frames_ && ok; i += kBatch) {
        const uint64_t n = (frames_ - i < kBatch) ? frames_ - i : kBatch;
        uint8_t* q = buf;
        for (uint64_t j = 0; j < n; j++) {
            put_be64(q, index_[i + j].offset);
            put_be32(q, index_[i + j].size);
        }
        ok = f_.write(buf, size_t(n) * 12);
    }
    free(buf);
    return ok;
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
    uint8_t* p = buf;
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

    if (!write_index()) return false;
    if (!write_comment()) return false;

    const int64_t end = f_.tell();

    uint8_t v[8];
    uint8_t* p;

    p = v; put_be64(p, uint64_t(end - 12));
    if (!f_.seek(frm8_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be64(p, uint64_t(dst_end - dst_body_pos_));
    if (!f_.seek(dst_size_pos_) || !f_.write(v, 8)) return false;

    p = v; put_be32(p, uint32_t(frames_));
    if (!f_.seek(frte_count_pos_) || !f_.write(v, 4)) return false;

    f_.close();
    return true;
}

DffWriter::~DffWriter() {
    free(index_);
}

} // namespace dff2dsf

#endif // DFF2DSF_DFFWRITE_HPP
