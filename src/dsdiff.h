// dff2dsf - DSDIFF (.dff) container reader.
//
// DSDIFF is Philips' IFF variant for DSD: an FRM8 form with 64-bit big-endian
// chunk sizes.  Sound data is either raw DSD ("DSD " chunk) or DST coded
// ("DST " chunk holding FRTE plus one DSTF chunk per 1/75 s frame).
#ifndef DFF2DSF_DSDIFF_H
#define DFF2DSF_DSDIFF_H

#include "common.h"
#include "dst.h"

namespace dff2dsf {

class DffReader {
public:
    ~DffReader();

    bool open(const char* path);

    unsigned dsd_rate() const { return dsd_rate_; }
    int channels() const { return channels_; }
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
    int next_dst_frame(const uint8_t** data, size_t* size, size_t* capacity);

    // The DSTC chunk that followed the frame just returned, if there was one:
    // a CRC over the DSD that frame decodes to.
    bool has_frame_crc() const { return has_frame_crc_; }
    uint32_t frame_crc() const { return frame_crc_; }

    // Next block of raw interleaved DSD.  Returns 1, 0 or -1 as above.
    int next_dsd_block(const uint8_t** data, size_t* size);

private:
    bool parse_prop(int64_t end);
    bool parse_frte(uint64_t size);
    void read_frame_crc();
    bool read_chunk_header(uint8_t id[4], uint64_t* size);
    bool ensure_capacity(size_t n);

    File f_;
    int64_t file_size_ = 0;
    int64_t body_pos_ = 0;   // first byte of sound data
    int64_t body_end_ = 0;   // one past the last byte of sound data
    int64_t cur_ = 0;

    unsigned dsd_rate_ = 0;
    int channels_ = 0;
    bool is_dst_ = false;
    bool saw_prop_ = false;
    bool saw_cmpr_ = false;
    uint32_t frame_count_ = 0;
    uint64_t dst_payload_ = 0;
    bool has_frame_crc_ = false;
    uint32_t frame_crc_ = 0;
    uint16_t frame_rate_ = 0;
    uint32_t channel_ids_[kDstMaxChannels] = {};

    uint8_t* buf_ = nullptr;
    size_t buf_capacity_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DSDIFF_H
