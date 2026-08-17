// dff2dsf - DSF (.dsf) container reader.
//
// The inverse of dsf.cpp: undoes the planar 4096-byte blocking and the LSB-first
// bit order, handing out planar MSB-first channels the DST encoder can use.
#ifndef DFF2DSF_DSFREAD_H
#define DFF2DSF_DSFREAD_H

namespace dff2dsf {

class DsfReader {
public:
    bool open(const char* path);

    int32_t channels() const { return channels_; }
    uint32_t dsd_rate() const { return dsd_rate_; }
    uint32_t channel_type() const { return channel_type_; }
    uint64_t samples_per_channel() const { return samples_; }

    // Fills `bytes_per_channel` bytes for each channel, planar, MSB-first.  The
    // final frame is padded with DSD silence.  Returns 1, 0 at end, -1 on error.
    int32_t read_planar(ByteP dst, size_t bytes_per_channel);

private:
    bool refill();

    File f_;
    int32_t channels_ = 0;
    uint32_t dsd_rate_ = 0;
    uint32_t channel_type_ = 0;
    uint32_t block_size_ = 0;     // per channel
    bool lsb_first_ = true;
    bool truncated_ = false;
    uint64_t samples_ = 0;        // DSD bits per channel
    uint64_t remaining_ = 0;      // audio bytes per channel still unread
    int64_t data_end_ = 0;

    // Planar staging, stride capacity_: one frame plus one block per channel, so
    // a frame can always be served whole.  Sized here for the largest frame and
    // the format's block size, which makes it a fixed array; capacity_ is the
    // stride actually in use.
    uint8_t buf_[(size_t(kMaxFrameBytesPerChannel) + kDsfBlockSize) * kDstMaxChannels];
    uint8_t block_[kDsfBlockSize];   // one channel's block, as read from the file
    size_t capacity_ = 0;
    size_t avail_ = 0;
    size_t head_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DSFREAD_H
