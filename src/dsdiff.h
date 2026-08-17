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

class DffReader {
public:
    bool open(const char* path);

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
    int32_t next_dst_frame(CBytePP data, SizeP size, SizeP capacity);

    // The DSTC chunk that followed the frame just returned, if there was one:
    // a CRC over the DSD that frame decodes to.
    bool has_frame_crc() const { return has_frame_crc_; }
    uint32_t frame_crc() const { return frame_crc_; }

    // Next block of raw interleaved DSD.  Returns 1, 0 or -1 as above.
    int32_t next_dsd_block(CBytePP data, SizeP size);

private:
    bool parse_prop(int64_t end);
    bool parse_frte(uint64_t size);
    void read_frame_crc();
    bool read_chunk_header(ByteP id, WordP size);

    File f_;
    int64_t file_size_ = 0;
    int64_t body_pos_ = 0;   // first byte of sound data
    int64_t body_end_ = 0;   // one past the last byte of sound data
    int64_t cur_ = 0;

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
