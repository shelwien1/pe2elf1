// dff2dsf - DSDIFF (.dff) container writer for DST coded frames.
#ifndef DFF2DSF_DFFWRITE_H
#define DFF2DSF_DFFWRITE_H

#include "common.h"

namespace dff2dsf {

class DffWriter {
public:
    bool open(const char* path, int channels, unsigned dsd_rate);

    // Appends one DST frame as a DSTF chunk.
    bool write_frame(const uint8_t* data, size_t size);

    // Patches the chunk sizes and the frame count, then closes.
    bool finish();

    uint64_t frames() const { return frames_; }

private:
    File f_;
    int64_t frm8_size_pos_ = 0;
    int64_t dst_size_pos_ = 0;
    int64_t dst_body_pos_ = 0;
    int64_t frte_count_pos_ = 0;
    uint64_t frames_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DFFWRITE_H
