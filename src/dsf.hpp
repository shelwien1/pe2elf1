// dff2dsf - DSF (.dsf) container writer.

#ifndef DFF2DSF_DSF_HPP
#define DFF2DSF_DSF_HPP

#include "dsf.h"

namespace dff2dsf {

namespace {

// DSF channel type codes: 1 mono, 2 stereo, 3 three channel, 4 quad,
// 5 four channel, 6 five channel, 7 five channel + LFE.
unsigned channel_type_for(int channels, const uint32_t* ids) {
    switch (channels) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4: {
        // Quad is LF/RF/LS/RS; the "four channel" layout has a centre instead.
        for (int i = 0; i < 4; i++)
            if (memcmp(&ids[i], "C   ", 4) == 0 || memcmp(&ids[i], "MC  ", 4) == 0)
                return 5;
        return 4;
    }
    case 5: return 6;
    case 6: return 7;
    default: return 0;
    }
}

} // namespace

bool DsfWriter::open(const char* path, int channels, unsigned dsd_rate,
                     const uint32_t* channel_ids) {
    if (channels < 1 || channels > kDstMaxChannels)
        return ERR("unsupported channel count %d", channels);

    // channel_ids arrive as big-endian packed four character codes; compare them
    // in that same byte order.
    uint32_t ids_be[kDstMaxChannels] = {};
    for (int i = 0; i < channels; i++) {
        uint8_t* p = reinterpret_cast<uint8_t*>(&ids_be[i]);
        p[0] = uint8_t(channel_ids[i] >> 24); p[1] = uint8_t(channel_ids[i] >> 16);
        p[2] = uint8_t(channel_ids[i] >> 8);  p[3] = uint8_t(channel_ids[i]);
    }

    channels_ = channels;
    dsd_rate_ = dsd_rate;
    channel_type_ = channel_type_for(channels, ids_be);
    if (!channel_type_) return ERR("no DSF channel type for %d channels", channels);

    if (!f_.open_write(path)) return false;

    // Placeholder header; the sizes are patched in finish().
    uint8_t hdr[kDsfHeaderSize] = {};
    memcpy(hdr, "DSD ", 4);
    wl64(hdr + 4, 28);
    wl64(hdr + 12, 0);                    // total file size
    wl64(hdr + 20, 0);                    // metadata pointer, none

    memcpy(hdr + 28, "fmt ", 4);
    wl64(hdr + 32, 52);
    wl32(hdr + 40, 1);                    // format version
    wl32(hdr + 44, 0);                    // format id: DSD raw
    wl32(hdr + 48, channel_type_);
    wl32(hdr + 52, unsigned(channels));
    wl32(hdr + 56, dsd_rate);
    wl32(hdr + 60, 1);                    // bits per sample: 1 = LSB first
    wl64(hdr + 64, 0);                    // sample count per channel
    wl32(hdr + 72, unsigned(kDsfBlockSize));
    wl32(hdr + 76, 0);                    // reserved

    memcpy(hdr + 80, "data", 4);
    wl64(hdr + 84, 12);                   // data chunk size

    return f_.write(hdr, sizeof(hdr));
}

bool DsfWriter::flush_block() {
    if (!fill_) return true;
    // Short final block: pad every channel out to the fixed block size.
    if (fill_ < kDsfBlockSize)
        for (int ch = 0; ch < channels_; ch++)
            memset(blocks_ + size_t(ch) * kDsfBlockSize + fill_, kDsdSilence,
                   kDsfBlockSize - fill_);

    if (!f_.write(blocks_, size_t(channels_) * kDsfBlockSize)) return false;
    data_bytes_ += uint64_t(channels_) * kDsfBlockSize;
    fill_ = 0;
    return true;
}

bool DsfWriter::write(const uint8_t* src, size_t bytes_per_channel) {
    const int channels = channels_;
    size_t done = 0;

    while (done < bytes_per_channel) {
        size_t n = kDsfBlockSize - fill_;
        if (n > bytes_per_channel - done) n = bytes_per_channel - done;

        // Deinterleave into the planar blocks, reversing each byte on the way.
        if (channels == 2) {
            uint8_t* d0 = blocks_ + fill_;
            uint8_t* d1 = blocks_ + kDsfBlockSize + fill_;
            const uint8_t* s = src + done * 2;
            for (size_t i = 0; i < n; i++) {
                d0[i] = kReverse.v[s[2 * i]];
                d1[i] = kReverse.v[s[2 * i + 1]];
            }
        } else {
            for (int ch = 0; ch < channels; ch++) {
                uint8_t* d = blocks_ + size_t(ch) * kDsfBlockSize + fill_;
                const uint8_t* s = src + done * size_t(channels) + size_t(ch);
                for (size_t i = 0; i < n; i++)
                    d[i] = kReverse.v[s[i * size_t(channels)]];
            }
        }

        fill_ += n;
        done += n;
        samples_ += uint64_t(n) * 8;

        if (fill_ == kDsfBlockSize && !flush_block()) return false;
    }
    return true;
}

bool DsfWriter::finish() {
    if (!flush_block()) return false;

    uint64_t file_size = kDsfHeaderSize + data_bytes_;

    uint8_t v[8];
    wl64(v, file_size);
    if (!f_.seek(12) || !f_.write(v, 8)) return false;

    wl64(v, samples_);
    if (!f_.seek(64) || !f_.write(v, 8)) return false;

    wl64(v, 12 + data_bytes_);
    if (!f_.seek(84) || !f_.write(v, 8)) return false;

    f_.close();
    return true;
}

} // namespace dff2dsf

#endif // DFF2DSF_DSF_HPP
