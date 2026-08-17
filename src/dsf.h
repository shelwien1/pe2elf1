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

// Alternating bits are DSD silence: what short blocks and frames are padded with.
inline constexpr uint8_t kDsdSilence = 0x69;

constexpr size_t kDsfBlockSize = 4096;   // per channel, fixed by the format
constexpr size_t kDsfHeaderSize = 28 + 52 + 12;

class DsfWriter {
public:
    bool open(const char* path, int32_t channels, uint32_t dsd_rate, CU32P channel_ids);

    // Appends `bytes_per_channel` bytes per channel of MSB-first DSD that is
    // byte-interleaved by channel, i.e. exactly what DSDIFF and the DST decoder
    // produce.  `src` holds bytes_per_channel * channels bytes.
    bool write(CByteP src, size_t bytes_per_channel);

    // Flushes the partial block and patches the header with the final sizes.
    bool finish();

private:
    bool flush_block();

    File f_;
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
