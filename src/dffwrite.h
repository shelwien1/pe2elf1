// dff2dsf - DSDIFF (.dff) container writer for DST coded frames.
//
// Lays the file out the way the Philips encoder does: FVER, PROP/SND, the DST
// sound data, then the DSTI frame index and a COMT comment.
#ifndef DFF2DSF_DFFWRITE_H
#define DFF2DSF_DFFWRITE_H

namespace dff2dsf {

// Which optional parts of the layout the writer emits.  The defaults are what
// the reference encoder's files carry; each one can be turned off from the
// command line, since some players and tools are happier without them.  The
// chunks not listed here - FRM8, FVER, PROP/SND with FS, CHNL and CMPR, and the
// DST sound data itself - carry what a decoder needs and are not optional.
struct DffWriteOptions {
    bool dsti = true;    // DSTI frame index, for seeking
    bool comt = true;    // COMT comment naming the encoder
    bool abss = true;    // ABSS absolute start time, in PROP/SND
    bool lsco = true;    // LSCO loudspeaker configuration, in PROP/SND
    bool dstc = false;   // DSTC per-frame CRC after each frame
};

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
uint32_t lsco_for(int32_t channels) {
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
    for (int32_t i = 7; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be32(ByteP& p, uint32_t v) {
    for (int32_t i = 3; i >= 0; i--) *p++ = uint8_t(v >> (8 * i));
}

void put_be16(ByteP& p, uint16_t v) {
    *p++ = uint8_t(v >> 8);
    *p++ = uint8_t(v);
}

// What this encoder writes into the file's comment chunk, the way the reference
// encoder records the tool that produced the file.
const char kCreatingMachine[] = "dff2dsf DST encoder";

} // namespace

class DffWriter {
public:
    bool begin(coro3_pin* out, int32_t channels, uint32_t dsd_rate,
               const DffWriteOptions& options = DffWriteOptions()) {
        if (channels < 1 || channels > kDstMaxChannels)
            return ERR("unsupported channel count %d", channels);
        opt_ = options;
        channels_ = channels;
        f_.attach(out);

        uint8_t hdr[256];
        ByteP p = hdr;

        put_tag(p, "FRM8");
        frm8_size_pos_ = p - hdr;
        put_be64(p, 0);                       // patched in patch()
        put_tag(p, "DSD ");

        put_tag(p, "FVER");
        put_be64(p, 4);
        put_be32(p, 0x01050000);              // DSDIFF 1.5.0.0

        // PROP/SND: sample rate, channels, compression, start time, speaker config.
        const uint32_t chnl_size = 2 + 4 * uint32_t(channels);
        const uint32_t prop_size = 4 +                 // "SND "
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
        for (int32_t i = 0; i < channels; i++)
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
        put_be64(p, 0);                       // patched in patch()

        put_tag(p, "FRTE");
        put_be64(p, 6);
        frte_count_pos_ = p - hdr;
        put_be32(p, 0);                       // patched in patch()
        put_be16(p, 75);                      // DST frames are always 1/75 s

        if (!f_.write(hdr, size_t(p - hdr))) return false;
        dst_body_pos_ = dst_size_pos_ + 8;   // first byte of the DST chunk body
        return true;
    }

    // Appends one DST frame as a DSTF chunk.  `dsd` is the uncompressed planar
    // DSD the frame codes, which the DSTC chunk's CRC is taken over; it is only
    // read when that chunk is enabled.
    bool write_frame(CByteP data, size_t size,
                     CByteP dsd = nullptr, size_t dsd_bytes_per_channel = 0) {
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

    // The last frame has gone out, so the stream's work is over: the index and
    // the comment come after the sound data, and the index has to read the
    // frames back, neither of which a stream can do.
    bool finish() {
        dst_end_ = f_.tell();
        return true;
    }

    // The trailing chunks and the three fields that were written before their
    // values were known, on the file the driver has just finished writing.
    bool patch(FILE* g) {
        if (!write_index(g)) return false;
        if (!write_comment(g)) return false;

        const int64_t end = file_tell(g);

        uint8_t v[8];
        ByteP p;

        p = v; put_be64(p, uint64_t(end - 12));
        if (!patch_at(g, frm8_size_pos_, v, 8)) return false;

        p = v; put_be64(p, uint64_t(dst_end_ - dst_body_pos_));
        if (!patch_at(g, dst_size_pos_, v, 8)) return false;

        p = v; put_be32(p, uint32_t(frames_));
        return patch_at(g, frte_count_pos_, v, 4);
    }

    uint64_t frames() const { return frames_; }

private:
    static bool patch_at(FILE* g, int64_t pos, const uint8_t* v, size_t n) {
        if (file_seek(g, pos, SEEK_SET) != 0) return ERR("seek failed");
        if (fwrite(v, 1, n, g) != n) return ERR("write failed");
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
    bool write_index(FILE* g) {
        if (!opt_.dsti || !frames_) return true;
        const int64_t dst_end = dst_end_;

        uint8_t hdr[12];
        ByteP p = hdr;
        put_tag(p, "DSTI");
        put_be64(p, frames_ * 12);
        if (!seek_to(g, dst_end) || !write_at(g, hdr, sizeof(hdr))) return false;

        constexpr uint32_t kBatch = 256;         // entries buffered between writes
        uint8_t buf[kBatch * 12];
        uint32_t n = 0;
        uint64_t seen = 0;
        int64_t pos = dst_body_pos_;             // the walk, over FRTE, DSTF, DSTC
        int64_t out = dst_end + 12;              // where the next entries go

        while (pos + 12 <= dst_end) {
            uint8_t ck[12];
            if (file_tell(g) != pos && !seek_to(g, pos)) return false;
            if (fread(ck, 1, sizeof(ck), g) != sizeof(ck)) return ERR("short read");
            const uint64_t size = rb64(ck + 4);

            if (tag_is(ck, "DSTF")) {
                ByteP q = buf + size_t(n) * 12;
                put_be64(q, uint64_t(pos) + 12);
                put_be32(q, uint32_t(size));
                seen++;
                if (++n == kBatch) {
                    if (!seek_to(g, out) || !write_at(g, buf, sizeof(buf))) return false;
                    out += int64_t(sizeof(buf));
                    n = 0;
                }
            }
            pos += 12 + int64_t(size) + int64_t(size & 1);
        }

        if (n) {
            if (!seek_to(g, out) || !write_at(g, buf, size_t(n) * 12)) return false;
            out += int64_t(n) * 12;
        }

        if (seen != frames_)
            return ERR("frame index found %" PRIu64 " frames, expected %" PRIu64,
                       seen, frames_);

        // The comment chunk is written next, and follows the index.
        return seek_to(g, out);
    }

    // COMT: a single file-history comment naming the encoder, timestamped now.
    bool write_comment(FILE* g) {
        if (!opt_.comt) return true;
        if (!opt_.dsti && !seek_to(g, dst_end_)) return false;

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

        return write_at(g, buf, size_t(p - buf));
    }

    static bool seek_to(FILE* g, int64_t pos) {
        if (file_seek(g, pos, SEEK_SET) != 0) return ERR("seek failed");
        return true;
    }

    static bool write_at(FILE* g, const uint8_t* v, size_t n) {
        if (fwrite(v, 1, n, g) != n) return ERR("write failed");
        return true;
    }

    DffWriteOptions opt_;
    Stream f_;
    int32_t channels_ = 0;
    int64_t dst_end_ = 0;      // where the sound data stopped
    int64_t frm8_size_pos_ = 0;
    int64_t dst_size_pos_ = 0;
    int64_t dst_body_pos_ = 0;
    int64_t frte_count_pos_ = 0;
    uint64_t frames_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DFFWRITE_H
