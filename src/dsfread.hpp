// dff2dsf - DSF (.dsf) container reader.

#ifndef DFF2DSF_DSFREAD_HPP
#define DFF2DSF_DSFREAD_HPP

#include "dsfread.h"
#include "dsf.h"

namespace dff2dsf {

DsfReader::~DsfReader() {
    free(buf_);
}

bool DsfReader::open(const char* path) {
    if (!f_.open_read(path)) return false;

    uint8_t hdr[kDsfHeaderSize];
    if (!f_.read(hdr, sizeof(hdr))) return false;

    auto rl32 = [](const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
                                              (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); };
    auto rl64 = [&](const uint8_t* p) { return uint64_t(rl32(p)) | (uint64_t(rl32(p + 4)) << 32); };

    if (memcmp(hdr, "DSD ", 4) || rl64(hdr + 4) != 28)
        return ERR("not a DSF file (no DSD chunk of 28 bytes)");

    if (memcmp(hdr + 28, "fmt ", 4) || rl64(hdr + 32) != 52)
        return ERR("malformed DSF: no fmt chunk of 52 bytes");
    if (rl32(hdr + 40) != 1) return ERR("unsupported DSF format version %u", rl32(hdr + 40));
    if (rl32(hdr + 44) != 0) return ERR("unsupported DSF format id %u", rl32(hdr + 44));

    channel_type_ = rl32(hdr + 48);
    channels_ = int(rl32(hdr + 52));
    dsd_rate_ = rl32(hdr + 56);

    switch (rl32(hdr + 60)) {
    case 1: lsb_first_ = true; break;
    case 8: lsb_first_ = false; break;
    default: return ERR("unsupported DSF bits per sample %u", rl32(hdr + 60));
    }

    samples_ = rl64(hdr + 64);
    block_size_ = rl32(hdr + 72);

    if (channels_ < 1 || channels_ > kDstMaxChannels)
        return ERR("unsupported channel count %d", channels_);
    if (!block_size_ || block_size_ > (1u << 20))
        return ERR("implausible DSF block size %u", block_size_);
    if (!dsd_rate_) return ERR("missing DSF sample rate");

    if (memcmp(hdr + 80, "data", 4))
        return ERR("malformed DSF: no data chunk");
    uint64_t data_size = rl64(hdr + 84);
    if (data_size < 12) return ERR("malformed DSF: short data chunk");

    int64_t file_size = f_.size();
    data_end_ = int64_t(kDsfHeaderSize) + int64_t(data_size - 12);
    if (data_end_ > file_size || data_end_ < int64_t(kDsfHeaderSize))
        data_end_ = file_size;

    remaining_ = samples_ / 8;
    // Fall back to the data chunk if the header's sample count is missing.
    uint64_t stored = uint64_t(data_end_ - int64_t(kDsfHeaderSize)) / unsigned(channels_);
    if (!remaining_ || remaining_ > stored) remaining_ = stored;

    return true;
}

// Reads one block per channel into the planar staging buffer.
bool DsfReader::refill() {
    if (!remaining_) return false;
    if (f_.tell() >= data_end_) return false;

    // Compact anything already consumed to the front.
    if (head_) {
        for (int ch = 0; ch < channels_; ch++)
            memmove(buf_ + size_t(ch) * capacity_, buf_ + size_t(ch) * capacity_ + head_, avail_);
        head_ = 0;
    }

    if (avail_ + block_size_ > capacity_) return false;

    uint8_t* block = static_cast<uint8_t*>(xalloc(block_size_));
    if (!block) return false;

    // A truncated file still yields whatever whole samples it holds, so read
    // what is there and take the shortest channel as the usable length.
    size_t shortest = block_size_;
    for (int ch = 0; ch < channels_; ch++) {
        size_t n = f_.read_some(block, block_size_);
        if (n < shortest) shortest = n;
        uint8_t* d = buf_ + size_t(ch) * capacity_ + avail_;
        if (lsb_first_)
            for (size_t i = 0; i < n; i++) d[i] = kReverse.v[block[i]];
        else
            memcpy(d, block, n);
        if (n < block_size_) memset(d + n, kDsdSilence, block_size_ - n);
    }
    free(block);

    if (!shortest) return false;
    if (shortest < block_size_ && !truncated_) {
        truncated_ = true;
        fprintf(stderr, "dff2dsf: warning: DSF data ends early, encoding what is there\n");
    }

    // The last block is padded out; only the announced samples are audio.
    size_t got = shortest;
    if (remaining_ < got) got = size_t(remaining_);
    remaining_ -= got;
    avail_ += got;
    return true;
}

int DsfReader::read_planar(uint8_t* dst, size_t bytes_per_channel) {
    if (!buf_) {
        capacity_ = bytes_per_channel + block_size_;
        buf_ = static_cast<uint8_t*>(xalloc(capacity_ * size_t(channels_)));
        if (!buf_) return -1;
    } else if (bytes_per_channel + block_size_ > capacity_) {
        ERR("frame size changed mid-stream");
        return -1;
    }

    while (avail_ < bytes_per_channel && refill())
        ;

    if (!avail_) return 0;

    size_t n = avail_ < bytes_per_channel ? avail_ : bytes_per_channel;
    for (int ch = 0; ch < channels_; ch++) {
        uint8_t* d = dst + size_t(ch) * bytes_per_channel;
        memcpy(d, buf_ + size_t(ch) * capacity_ + head_, n);
        if (n < bytes_per_channel)
            memset(d + n, kDsdSilence, bytes_per_channel - n);
    }

    head_ += n;
    avail_ -= n;
    return 1;
}

} // namespace dff2dsf

#endif // DFF2DSF_DSFREAD_HPP
