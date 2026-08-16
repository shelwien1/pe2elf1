// dff2dsf - DSF (.dsf) container writer.
//
// DSF stores the same DSD bits as DSDIFF but differently arranged: data is
// planar in blocks of 4096 bytes per channel (channel 0's block, then channel
// 1's, and so on), and bytes are LSB-first, whereas DSDIFF interleaves per byte
// and is MSB-first.  Both rearrangements happen here.
#ifndef DFF2DSF_DSF_H
#define DFF2DSF_DSF_H

#include "common.h"
#include "dst.h"

namespace dff2dsf {

constexpr size_t kDsfBlockSize = 4096;   // per channel, fixed by the format
constexpr size_t kDsfHeaderSize = 28 + 52 + 12;

class DsfWriter {
public:
    ~DsfWriter();

    bool open(const char* path, int channels, unsigned dsd_rate, const uint32_t* channel_ids);

    // Appends `bytes_per_channel` bytes per channel of MSB-first DSD that is
    // byte-interleaved by channel, i.e. exactly what DSDIFF and the DST decoder
    // produce.  `src` holds bytes_per_channel * channels bytes.
    bool write(const uint8_t* src, size_t bytes_per_channel);

    // Flushes the partial block and patches the header with the final sizes.
    bool finish();

private:
    bool flush_block();

    File f_;
    int channels_ = 0;
    unsigned dsd_rate_ = 0;
    unsigned channel_type_ = 0;
    uint8_t* blocks_ = nullptr;    // channels_ * kDsfBlockSize, planar
    size_t fill_ = 0;              // bytes used per channel in the current block
    uint64_t samples_ = 0;         // DSD bits written per channel
    uint64_t data_bytes_ = 0;      // bytes written to the data chunk, padding included
};

} // namespace dff2dsf

#endif // DFF2DSF_DSF_H
