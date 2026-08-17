// dff2dsf - DSF (.dsf) container writer.
//
// DSF stores the same DSD bits as DSDIFF but differently arranged: data is
// planar in blocks of 4096 bytes per channel (channel 0's block, then channel
// 1's, and so on), and bytes are LSB-first, whereas DSDIFF interleaves per byte
// and is MSB-first.  Both rearrangements happen here.
#ifndef DFF2DSF_DSF_H
#define DFF2DSF_DSF_H

namespace dff2dsf {

constexpr size_t kDsfBlockSize = 4096;   // per channel, fixed by the format
constexpr size_t kDsfHeaderSize = 28 + 52 + 12;

namespace {

// DSF channel type codes: 1 mono, 2 stereo, 3 three channel, 4 quad,
// 5 four channel, 6 five channel, 7 five channel + LFE.
uint32_t channel_type_for(int32_t channels, CU32P ids) {
    switch (channels) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4: {
        // Quad is LF/RF/LS/RS; the "four channel" layout has a centre instead.
        for (int32_t i = 0; i < 4; i++)
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

class DsfWriter {
public:
    // Writes the header, with the three fields that are only known at the end
    // left at zero; patch() fills them in once the coroutine has run.
    bool begin(coro3_pin* out, int32_t channels, uint32_t dsd_rate,
               CU32P channel_ids) {
        if (channels < 1 || channels > kDstMaxChannels)
            return ERR("unsupported channel count %d", channels);

        // channel_ids arrive as big-endian packed four character codes; compare them
        // in that same byte order.
        uint32_t ids_be[kDstMaxChannels] = {};
        for (int32_t i = 0; i < channels; i++) {
            const ByteP p = reinterpret_cast<uint8_t*>(&ids_be[i]);
            p[0] = uint8_t(channel_ids[i] >> 24); p[1] = uint8_t(channel_ids[i] >> 16);
            p[2] = uint8_t(channel_ids[i] >> 8);  p[3] = uint8_t(channel_ids[i]);
        }

        channels_ = channels;
        dsd_rate_ = dsd_rate;
        channel_type_ = channel_type_for(channels, ids_be);
        if (!channel_type_) return ERR("no DSF channel type for %d channels", channels);

        f_.attach(out);

        // Placeholder header; the sizes are patched at the end.
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
        wl32(hdr + 52, uint32_t(channels));
        wl32(hdr + 56, dsd_rate);
        wl32(hdr + 60, 1);                    // bits per sample: 1 = LSB first
        wl64(hdr + 64, 0);                    // sample count per channel
        wl32(hdr + 72, uint32_t(kDsfBlockSize));
        wl32(hdr + 76, 0);                    // reserved

        memcpy(hdr + 80, "data", 4);
        wl64(hdr + 84, 12);                   // data chunk size

        return f_.write(hdr, sizeof(hdr));
    }

    // Appends `bytes_per_channel` bytes per channel of MSB-first DSD that is
    // byte-interleaved by channel, i.e. exactly what DSDIFF and the DST decoder
    // produce.  `src` holds bytes_per_channel * channels bytes.
    //
    // The channel count is fixed for the file, so it is settled once here: the
    // deinterleave below is instantiated per count, which turns the inner loop's
    // stride and trip count into constants and drops the two-channel special
    // case that used to be written out by hand.
    bool write(CByteP src, size_t bytes_per_channel) {
        switch (channels_) {
        case 1:  return write_n<1>(src, bytes_per_channel);
        case 2:  return write_n<2>(src, bytes_per_channel);
        case 3:  return write_n<3>(src, bytes_per_channel);
        case 4:  return write_n<4>(src, bytes_per_channel);
        case 5:  return write_n<5>(src, bytes_per_channel);
        default: return write_n<6>(src, bytes_per_channel);
        }
    }

    // Flushes the partial block.  Still on the stream, so still in the
    // coroutine: this is the last thing it writes.
    bool finish() { return flush_block(); }

    // The sizes and the sample count, written back into the header that went
    // out before any of them were known.  The coroutine has finished by now, so
    // this is plain stdio on the file the driver wrote.
    bool patch(FILE* g) const {
        uint8_t v[8];
        wl64(v, kDsfHeaderSize + data_bytes_);      // total file size
        if (!patch_at(g, 12, v)) return false;
        wl64(v, samples_);                          // sample count per channel
        if (!patch_at(g, 64, v)) return false;
        wl64(v, 12 + data_bytes_);                  // data chunk size
        return patch_at(g, 84, v);
    }

private:
    template <int32_t Channels>
    bool write_n(CByteP src, size_t bytes_per_channel) {
        size_t done = 0;

        while (done < bytes_per_channel) {
            size_t n = kDsfBlockSize - fill_;
            if (n > bytes_per_channel - done) n = bytes_per_channel - done;

            // Deinterleave into the planar blocks, reversing each byte on the way.
            for (int32_t ch = 0; ch < Channels; ch++) {
                const ByteP d = blocks_ + size_t(ch) * kDsfBlockSize + fill_;
                const CByteP s = src + done * size_t(Channels) + size_t(ch);
                for (size_t i = 0; i < n; i++)
                    d[i] = kReverse.v[s[i * size_t(Channels)]];
            }

            fill_ += n;
            done += n;
            samples_ += uint64_t(n) * 8;

            if (fill_ == kDsfBlockSize && !flush_block()) return false;
        }
        return true;
    }

    static bool patch_at(FILE* g, int64_t pos, const uint8_t (&v)[8]) {
        if (file_seek(g, pos, SEEK_SET) != 0) return ERR("seek failed");
        if (fwrite(v, 1, 8, g) != 8) return ERR("write failed");
        return true;
    }

    bool flush_block() {
        if (!fill_) return true;
        // Short final block: pad every channel out to the fixed block size.
        if (fill_ < kDsfBlockSize)
            for (int32_t ch = 0; ch < channels_; ch++)
                memset(blocks_ + size_t(ch) * kDsfBlockSize + fill_, kDsdSilence,
                       kDsfBlockSize - fill_);

        if (!f_.write(blocks_, size_t(channels_) * kDsfBlockSize)) return false;
        data_bytes_ += uint64_t(channels_) * kDsfBlockSize;
        fill_ = 0;
        return true;
    }

    Stream f_;
    int32_t channels_ = 0;
    uint32_t dsd_rate_ = 0;
    uint32_t channel_type_ = 0;
    // The block being filled, planar: channel c occupies c * kDsfBlockSize.  The
    // block size is fixed by the format, so this is too.
    uint8_t blocks_[kDstMaxChannels * kDsfBlockSize];
    size_t fill_ = 0;              // bytes used per channel in the current block
    uint64_t samples_ = 0;         // DSD bits written per channel
    uint64_t data_bytes_ = 0;      // bytes written to the data chunk, padding included
};

} // namespace dff2dsf

#endif // DFF2DSF_DSF_H
