// dff2dsf - DSDIFF (.dff) container writer for DST coded frames.
//
// Lays the file out the way the Philips encoder does: FVER, PROP/SND, the DST
// sound data, then the DSTI frame index and a COMT comment.
#ifndef DFF2DSF_DFFWRITE_H
#define DFF2DSF_DFFWRITE_H

#include "common.h"

namespace dff2dsf {

// Which optional parts of the layout the writer emits.  The defaults are what
// the reference encoder's files carry; each one can be turned off from the
// command line, since some players and tools are happier without them.  The
// chunks not listed here - FRM8, FVER, PROP/SND with FS, CHNL and CMPR, and the
// DST sound data itself - carry what a decoder needs and are not optional.
struct DffWriteOptions {
    bool dsti = true;   // DSTI frame index, for seeking
    bool comt = true;   // COMT comment naming the encoder
    bool abss = true;   // ABSS absolute start time, in PROP/SND
    bool lsco = true;   // LSCO loudspeaker configuration, in PROP/SND
};

class DffWriter {
public:
    ~DffWriter();

    bool open(const char* path, int channels, unsigned dsd_rate,
              const DffWriteOptions& options = DffWriteOptions());

    // Appends one DST frame as a DSTF chunk.
    bool write_frame(const uint8_t* data, size_t size);

    // Writes the trailing chunks, patches the sizes and the frame count, closes.
    bool finish();

    uint64_t frames() const { return frames_; }

private:
    bool record_frame(int64_t payload_pos, size_t size);
    bool write_index();
    bool write_comment();

    DffWriteOptions opt_;
    File f_;
    int64_t frm8_size_pos_ = 0;
    int64_t dst_size_pos_ = 0;
    int64_t dst_body_pos_ = 0;
    int64_t frte_count_pos_ = 0;
    uint64_t frames_ = 0;

    // One DSTI entry per frame: where its data starts and how long it is.
    struct IndexEntry {
        uint64_t offset;
        uint32_t size;
    };
    IndexEntry* index_ = nullptr;
    uint64_t index_capacity_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DFFWRITE_H
