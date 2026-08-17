// dff2dsf - DSDIFF (.dff) container reader.
//
// DSDIFF is Philips' IFF variant for DSD: an FRM8 form with 64-bit big-endian
// chunk sizes.  Sound data is either raw DSD ("DSD " chunk) or DST coded
// ("DST " chunk holding FRTE plus one DSTF chunk per 1/75 s frame).
#ifndef DFF2DSF_DSDIFF_H
#define DFF2DSF_DSDIFF_H

namespace dff2dsf {

// Raw DSD is handed to the writer in chunks of roughly this size.
constexpr size_t kDsdBlockBytes = 1u << 20;

// The read buffer holds either a DST frame with the bit reader's padding, or a
// block of raw DSD, whichever is larger - so it is a fixed array rather than
// something grown to fit as chunks arrive.
constexpr size_t kDffReadBuffer =
    kDsdBlockBytes > kMaxDstFrameSize + kBitReaderPadding
        ? kDsdBlockBytes : kMaxDstFrameSize + kBitReaderPadding;

namespace {

// Sanity bound on the header chunks, whose sizes are only ever read to skip
// past them: anything larger is a corrupt file rather than something to parse.
constexpr uint64_t kMaxChunkPayload = 64u << 20;

} // namespace

class DffReader {
public:
    // Parses everything up to the sound data, leaving the stream positioned at
    // its first chunk.  The file arrives a byte at a time, so chunks that carry
    // nothing needed are skipped rather than seeked over, and the one place that
    // has to look ahead - is there a DSTC after this frame? - pushes the chunk
    // header it read back for the next call.
    bool begin(coro3_pin* inp) {
        f_.attach(inp);

        uint8_t hdr[16];
        if (!f_.read(hdr, sizeof(hdr))) return false;
        if (!tag_is(hdr, "FRM8") || !tag_is(hdr + 12, "DSD "))
            return ERR("not a DSDIFF file (no FRM8/DSD signature)");

        const int64_t form_end = 12 + int64_t(rb64(hdr + 4));
        if (form_end <= 16) return ERR("malformed FRM8 chunk");

        while (f_.tell() + 12 <= form_end) {
            uint8_t id[4];
            uint64_t size;
            if (!read_chunk_header(id, &size)) break;
            const int64_t body = f_.tell();

            if (tag_is(id, "PROP")) {
                const int64_t end = body + int64_t(size);
                if (size < 4 || end > form_end) return ERR("malformed PROP chunk");
                if (!parse_prop(end)) return false;
                continue;
            }
            if (tag_is(id, "DST ") || tag_is(id, "DSD ")) {
                body_pos_ = body;
                body_end_ = body + int64_t(size);
                if (body_end_ < body_pos_) return ERR("malformed sound data chunk");
                // The sound data chunk id is authoritative; CMPR is only a hint.
                const bool dst_chunk = tag_is(id, "DST ");
                if (saw_cmpr_ && dst_chunk != is_dst_)
                    fprintf(stderr, "dff2dsf: warning: CMPR disagrees with the sound data chunk\n");
                is_dst_ = dst_chunk;
                break;
            }
            if (size > kMaxChunkPayload) return ERR("implausible chunk size");
            if (!f_.skip(int64_t(size) + int64_t(size & 1))) break;
        }

        if (!saw_prop_) return ERR("no PROP/SND chunk found");
        if (!body_pos_) return ERR("no sound data chunk found");
        if (!dsd_rate_) return ERR("no FS chunk found");
        if (!channels_) return ERR("no CHNL chunk found");

        // FRTE has to be the first chunk inside the DST data, and carries the
        // frame count the caller wants before decoding starts.  Reading it here
        // costs the look-ahead of one chunk header, which is pushed back if the
        // file skipped FRTE and went straight to the frames.
        if (is_dst_) {
            uint8_t id[4];
            uint64_t size;
            if (read_chunk_header(id, &size)) {
                if (tag_is(id, "FRTE")) {
                    if (!parse_frte(size)) return false;
                } else {
                    push_back(id, size);
                }
            }
            if (frame_rate_ && frame_rate_ != 75)
                fprintf(stderr, "dff2dsf: warning: unusual DST frame rate %u\n", frame_rate_);
        }

        return true;
    }

    uint32_t dsd_rate() const { return dsd_rate_; }
    int32_t channels() const { return channels_; }
    bool is_dst() const { return is_dst_; }
    uint32_t frame_count() const { return frame_count_; }
    uint16_t frame_rate() const { return frame_rate_; }
    const uint32_t* channel_ids() const { return channel_ids_; }
    int64_t data_bytes() const { return body_end_ - body_pos_; }

    // Total size of the DST frame payloads seen so far, which is the compressed
    // audio proper: container extras such as a frame index chunk are not
    // included, so this compares like for like across encoders.
    uint64_t dst_payload() const { return dst_payload_; }

    // Next DST frame.  Returns 1 on success, 0 at end of data, -1 on error.
    // The payload is zero padded to allow the bit reader's windowed reads.
    int32_t next_dst_frame(CBytePP data, SizeP size, SizeP capacity) {
        while (chunk_pos() + 12 <= body_end_) {
            uint8_t id[4];
            uint64_t chunk_size;
            if (!read_chunk_header(id, &chunk_size)) return 0;
            const int64_t body = f_.tell();

            if (tag_is(id, "DSTF")) {
                // The frame has to fit the buffer, which is sized for the largest
                // frame the format can produce; anything larger is corrupt.
                if (chunk_size < 1 || chunk_size + kBitReaderPadding > sizeof(buf_)) {
                    ERR("implausible DSTF size %" PRIu64, chunk_size);
                    return -1;
                }
                if (body + int64_t(chunk_size) > body_end_) {
                    ERR("truncated DSTF frame");
                    return -1;
                }
                const size_t n = size_t(chunk_size);
                if (!f_.read(buf_, n)) return -1;
                memset(buf_ + n, 0, kBitReaderPadding);
                if ((chunk_size & 1) && !f_.skip(1)) return -1;
                *data = buf_;
                *size = n;
                *capacity = n + kBitReaderPadding;
                dst_payload_ += n;
                read_frame_crc();
                return 1;
            }

            // Anything else in here is not needed.
            if (chunk_size > kMaxChunkPayload) {
                ERR("implausible chunk size %" PRIu64 " in DST data", chunk_size);
                return -1;
            }
            if (!f_.skip(int64_t(chunk_size) + int64_t(chunk_size & 1))) return 0;
        }
        return 0;
    }

    // The DSTC chunk that followed the frame just returned, if there was one:
    // a CRC over the DSD that frame decodes to.
    bool has_frame_crc() const { return has_frame_crc_; }
    uint32_t frame_crc() const { return frame_crc_; }

    // Next block of raw interleaved DSD.  Returns 1, 0 or -1 as above.
    int32_t next_dsd_block(CBytePP data, SizeP size) {
        const int64_t left = body_end_ - f_.tell();
        if (left <= 0) return 0;

        size_t want = kDsdBlockBytes;
        // Keep blocks a whole number of frames' worth of samples per channel.
        want -= want % size_t(channels_);
        if (int64_t(want) > left) want = size_t(left);

        const size_t got = f_.read_some(buf_, want);
        if (!got) return 0;

        *data = buf_;
        *size = got - got % size_t(channels_);
        return *size ? 1 : 0;
    }

private:
    // PROP/SND holds the stream properties: sample rate, channels and which
    // compression the sound data chunk uses.
    bool parse_prop(int64_t end) {
        uint8_t type[4];
        if (!f_.read(type, 4)) return false;
        if (!tag_is(type, "SND ")) {
            // Property lists other than sound carry nothing we need.
            return f_.skip(end - f_.tell());
        }

        while (f_.tell() + 12 <= end) {
            uint8_t id[4];
            uint64_t size;
            if (!read_chunk_header(id, &size)) return false;
            const int64_t body = f_.tell();
            if (size > kMaxChunkPayload || body + int64_t(size) > end)
                return ERR("malformed PROP chunk");

            if (tag_is(id, "FS  ")) {
                if (size < 4) return ERR("short FS chunk");
                uint8_t v[4];
                if (!f_.read(v, 4)) return false;
                dsd_rate_ = rb32(v);
            } else if (tag_is(id, "CHNL")) {
                if (size < 2) return ERR("short CHNL chunk");
                uint8_t v[2];
                if (!f_.read(v, 2)) return false;
                const uint32_t n = rb16(v);
                if (n < 1 || n > uint32_t(kDstMaxChannels))
                    return ERR("unsupported channel count %u", n);
                if (size < 2 + 4 * uint64_t(n)) return ERR("short CHNL chunk");
                for (uint32_t i = 0; i < n; i++) {
                    uint8_t idb[4];
                    if (!f_.read(idb, 4)) return false;
                    channel_ids_[i] = rb32(idb);
                }
                channels_ = int32_t(n);
            } else if (tag_is(id, "CMPR")) {
                if (size < 4) return ERR("short CMPR chunk");
                uint8_t v[4];
                if (!f_.read(v, 4)) return false;
                if (tag_is(v, "DST ")) {
                    is_dst_ = true;
                } else if (tag_is(v, "DSD ")) {
                    is_dst_ = false;
                } else {
                    return ERR("unsupported compression '%.4s'", reinterpret_cast<char*>(v));
                }
                saw_cmpr_ = true;
            }

            if (!f_.skip(body + int64_t(size) + int64_t(size & 1) - f_.tell())) return false;
        }

        saw_prop_ = true;
        return f_.skip(end - f_.tell());
    }

    bool parse_frte(uint64_t size) {
        if (size < 6) return ERR("short FRTE chunk");
        uint8_t v[6];
        if (!f_.read(v, 6)) return false;
        frame_count_ = rb32(v);
        frame_rate_ = rb16(v + 4);
        return f_.skip(int64_t(size) - 6 + int64_t(size & 1));
    }

    // A frame's CRC, if the file carries them, is in the DSTC chunk immediately
    // after it.  Looking for it here keeps it paired with the frame it covers;
    // when the next chunk is another frame instead, its header goes back.
    void read_frame_crc() {
        has_frame_crc_ = false;
        if (chunk_pos() + 12 > body_end_) return;

        uint8_t id[4];
        uint64_t size;
        if (!read_chunk_header(id, &size)) return;
        if (!tag_is(id, "DSTC") || size < 4 || size > kMaxChunkPayload) {
            push_back(id, size);
            return;
        }

        uint8_t v[4];
        if (!f_.read(v, 4)) return;
        frame_crc_ = rb32(v);
        has_frame_crc_ = true;
        f_.skip(int64_t(size) - 4 + int64_t(size & 1));
    }

    // One chunk header of push-back, which is all the look-ahead here needs.
    void push_back(CByteP id, uint64_t size) {
        memcpy(pending_, id, 4);
        pending_size_ = size;
        has_pending_ = true;
        pending_pos_ = f_.tell() - 12;
    }

    bool read_chunk_header(ByteP id, WordP size) {
        if (has_pending_) {
            has_pending_ = false;
            memcpy(id, pending_, 4);
            *size = pending_size_;
            return true;
        }
        uint8_t hdr[12];
        if (!f_.read(hdr, sizeof(hdr), /*eof_ok=*/true)) return false;
        memcpy(id, hdr, 4);
        *size = rb64(hdr + 4);
        return true;
    }

    // Where the stream is as far as the chunk walk is concerned: one chunk
    // header behind the stream itself while a header is pushed back.
    int64_t chunk_pos() const { return has_pending_ ? pending_pos_ : f_.tell(); }

    Stream f_;
    int64_t body_pos_ = 0;   // first byte of sound data
    int64_t body_end_ = 0;   // one past the last byte of sound data

    uint8_t pending_[4] = {};
    uint64_t pending_size_ = 0;
    int64_t pending_pos_ = 0;
    bool has_pending_ = false;

    uint32_t dsd_rate_ = 0;
    int32_t channels_ = 0;
    bool is_dst_ = false;
    bool saw_prop_ = false;
    bool saw_cmpr_ = false;
    uint32_t frame_count_ = 0;
    uint64_t dst_payload_ = 0;
    bool has_frame_crc_ = false;
    uint32_t frame_crc_ = 0;
    uint16_t frame_rate_ = 0;
    uint32_t channel_ids_[kDstMaxChannels] = {};

    uint8_t buf_[kDffReadBuffer];
};

} // namespace dff2dsf

#endif // DFF2DSF_DSDIFF_H
