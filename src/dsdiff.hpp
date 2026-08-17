// dff2dsf - DSDIFF (.dff) container reader.

#ifndef DFF2DSF_DSDIFF_HPP
#define DFF2DSF_DSDIFF_HPP

#include "dsdiff.h"
#include "bits.h"

namespace dff2dsf {

namespace {

// A single DSTF frame is at most one uncompressed frame plus its header; this
// bound keeps a corrupt size field from triggering a huge allocation.
constexpr uint64_t kMaxChunkPayload = 64u << 20;

// Raw DSD is handed to the writer in chunks of roughly this size.
constexpr size_t kDsdBlockBytes = 1u << 20;

} // namespace

DffReader::~DffReader() {
    free(buf_);
}

bool DffReader::ensure_capacity(size_t n) {
    if (n <= buf_capacity_) return true;
    size_t want = buf_capacity_ ? buf_capacity_ : 65536;
    while (want < n) want *= 2;
    uint8_t* p = static_cast<uint8_t*>(realloc(buf_, want));
    if (!p) return ERR("out of memory (%zu bytes)", want);
    buf_ = p;
    buf_capacity_ = want;
    return true;
}

bool DffReader::read_chunk_header(uint8_t id[4], uint64_t* size) {
    uint8_t hdr[12];
    if (!f_.read(hdr, sizeof(hdr), /*eof_ok=*/true)) return false;
    memcpy(id, hdr, 4);
    *size = rb64(hdr + 4);
    return true;
}

// PROP/SND holds the stream properties: sample rate, channels and which
// compression the sound data chunk uses.
bool DffReader::parse_prop(int64_t end) {
    uint8_t type[4];
    if (!f_.read(type, 4)) return false;
    if (!tag_is(type, "SND ")) {
        // Property lists other than sound carry nothing we need.
        return f_.seek(end);
    }

    while (f_.tell() + 12 <= end) {
        uint8_t id[4];
        uint64_t size;
        if (!read_chunk_header(id, &size)) return false;
        int64_t body = f_.tell();
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
            unsigned n = rb16(v);
            if (n < 1 || n > unsigned(kDstMaxChannels))
                return ERR("unsupported channel count %u", n);
            if (size < 2 + 4 * uint64_t(n)) return ERR("short CHNL chunk");
            for (unsigned i = 0; i < n; i++) {
                uint8_t idb[4];
                if (!f_.read(idb, 4)) return false;
                channel_ids_[i] = rb32(idb);
            }
            channels_ = int(n);
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

        if (!f_.seek(body + int64_t(size) + int64_t(size & 1))) return false;
    }

    saw_prop_ = true;
    return f_.seek(end);
}

bool DffReader::parse_frte(uint64_t size) {
    if (size < 6) return ERR("short FRTE chunk");
    uint8_t v[6];
    if (!f_.read(v, 6)) return false;
    frame_count_ = rb32(v);
    frame_rate_ = rb16(v + 4);
    return true;
}

bool DffReader::open(const char* path) {
    if (!f_.open_read(path)) return false;
    file_size_ = f_.size();
    if (file_size_ < 16) return ERR("file is too small to be a DSDIFF");

    uint8_t hdr[16];
    if (!f_.read(hdr, sizeof(hdr))) return false;
    if (!tag_is(hdr, "FRM8") || !tag_is(hdr + 12, "DSD "))
        return ERR("not a DSDIFF file (no FRM8/DSD signature)");

    int64_t form_end = 12 + int64_t(rb64(hdr + 4));
    if (form_end <= 16 || form_end > file_size_) form_end = file_size_;

    while (f_.tell() + 12 <= form_end) {
        uint8_t id[4];
        uint64_t size;
        if (!read_chunk_header(id, &size)) break;
        int64_t body = f_.tell();
        int64_t next = body + int64_t(size) + int64_t(size & 1);

        if (tag_is(id, "PROP")) {
            int64_t end = body + int64_t(size);
            if (size < 4 || end > form_end) return ERR("malformed PROP chunk");
            if (!parse_prop(end)) return false;
        } else if (tag_is(id, "DST ") || tag_is(id, "DSD ")) {
            body_pos_ = body;
            body_end_ = body + int64_t(size);
            if (body_end_ > file_size_ || body_end_ < body_pos_)
                body_end_ = file_size_;
            cur_ = body_pos_;
            // The sound data chunk id is authoritative; CMPR is only a hint.
            bool dst_chunk = tag_is(id, "DST ");
            if (saw_cmpr_ && dst_chunk != is_dst_)
                fprintf(stderr, "dff2dsf: warning: CMPR disagrees with the sound data chunk\n");
            is_dst_ = dst_chunk;
            break;
        }

        if (next <= body || !f_.seek(next)) break;
    }

    if (!saw_prop_) return ERR("no PROP/SND chunk found");
    if (!body_pos_) return ERR("no sound data chunk found");
    if (!dsd_rate_) return ERR("no FS chunk found");
    if (!channels_) return ERR("no CHNL chunk found");

    // FRTE precedes the frames inside the DST chunk; read it up front so the
    // total length is known before decoding starts.
    if (is_dst_) {
        int64_t save = cur_;
        while (cur_ + 12 <= body_end_) {
            if (!f_.seek(cur_)) return false;
            uint8_t id[4];
            uint64_t size;
            if (!read_chunk_header(id, &size)) break;
            if (size > kMaxChunkPayload) return ERR("implausible chunk size in DST data");
            if (tag_is(id, "FRTE")) {
                if (!parse_frte(size)) return false;
                break;
            }
            if (tag_is(id, "DSTF")) break;  // no FRTE, fall back to counting
            cur_ += 12 + int64_t(size) + int64_t(size & 1);
        }
        cur_ = save;
        if (!f_.seek(cur_)) return false;
        if (frame_rate_ && frame_rate_ != 75)
            fprintf(stderr, "dff2dsf: warning: unusual DST frame rate %u\n", frame_rate_);
    }

    return true;
}

int DffReader::next_dst_frame(const uint8_t** data, size_t* size, size_t* capacity) {
    while (cur_ + 12 <= body_end_) {
        if (!f_.seek(cur_)) return -1;

        uint8_t id[4];
        uint64_t chunk_size;
        if (!read_chunk_header(id, &chunk_size)) return 0;
        int64_t body = cur_ + 12;
        cur_ = body + int64_t(chunk_size) + int64_t(chunk_size & 1);

        if (tag_is(id, "DSTF")) {
            if (chunk_size < 1 || chunk_size > kMaxChunkPayload) {
                ERR("implausible DSTF size %llu", (unsigned long long)chunk_size);
                return -1;
            }
            if (body + int64_t(chunk_size) > body_end_) {
                ERR("truncated DSTF frame");
                return -1;
            }
            size_t n = size_t(chunk_size);
            if (!ensure_capacity(n + kBitReaderPadding)) return -1;
            if (!f_.read(buf_, n)) return -1;
            memset(buf_ + n, 0, kBitReaderPadding);
            *data = buf_;
            *size = n;
            *capacity = n + kBitReaderPadding;
            dst_payload_ += n;
            return 1;
        }
        // DSTC (frame CRC) and anything else in here is not needed.
    }
    return 0;
}

int DffReader::next_dsd_block(const uint8_t** data, size_t* size) {
    if (cur_ >= body_end_) return 0;

    size_t want = kDsdBlockBytes;
    // Keep blocks a whole number of frames' worth of samples per channel.
    want -= want % size_t(channels_);
    if (int64_t(want) > body_end_ - cur_) want = size_t(body_end_ - cur_);

    if (!ensure_capacity(want)) return -1;
    if (!f_.seek(cur_)) return -1;
    if (!f_.read(buf_, want)) return -1;
    cur_ += int64_t(want);

    *data = buf_;
    *size = want;
    return 1;
}

} // namespace dff2dsf

#endif // DFF2DSF_DSDIFF_HPP
