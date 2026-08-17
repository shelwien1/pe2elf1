// dff2dsf - DSF (.dsf) container reader.
//
// The inverse of dsf.cpp: undoes the planar 4096-byte blocking and the LSB-first
// bit order, handing out planar MSB-first channels the DST encoder can use.
#ifndef DFF2DSF_DSFREAD_H
#define DFF2DSF_DSFREAD_H

#include "common.h"
#include "dst.h"

namespace dff2dsf {

class DsfReader {
public:
    ~DsfReader();

    bool open(const char* path);

    int channels() const { return channels_; }
    unsigned dsd_rate() const { return dsd_rate_; }
    unsigned channel_type() const { return channel_type_; }
    uint64_t samples_per_channel() const { return samples_; }

    // Fills `bytes_per_channel` bytes for each channel, planar, MSB-first.  The
    // final frame is padded with DSD silence.  Returns 1, 0 at end, -1 on error.
    int read_planar(uint8_t* dst, size_t bytes_per_channel);

private:
    bool refill();

    File f_;
    int channels_ = 0;
    unsigned dsd_rate_ = 0;
    unsigned channel_type_ = 0;
    unsigned block_size_ = 0;     // per channel
    bool lsb_first_ = true;
    bool truncated_ = false;
    uint64_t samples_ = 0;        // DSD bits per channel
    uint64_t remaining_ = 0;      // audio bytes per channel still unread
    int64_t data_end_ = 0;

    uint8_t* buf_ = nullptr;      // planar, stride capacity_
    size_t capacity_ = 0;
    size_t avail_ = 0;
    size_t head_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DSFREAD_H
