// dff2dsf - encoding several DST frames at once.
//
// DST frames are independent: each one starts the decoder's history from the
// same fixed pattern, so nothing carries over from the frame before and a frame
// can be encoded wherever there is a core free.  Reading and writing stay in the
// calling thread and stay in order; only the encoding is spread out.
//
// Frames move through a small ring of slots, one buffer pair each.  The caller's
// thread fills every free slot from the .dsf, then blocks until the oldest frame
// has been coded and writes it, which frees a slot again.  Workers take the next
// uncoded frame in turn.  The ring is a couple of slots longer than the number
// of workers, so reading and writing overlap encoding rather than stalling it.
#ifndef DFF2DSF_ENCPOOL_H
#define DFF2DSF_ENCPOOL_H

#include "common.h"
#include "dsfread.h"
#include "dstenc.h"
#include "dffwrite.h"

#include <condition_variable>
#include <mutex>
#include <new>
#include <thread>

namespace dff2dsf {

// How many threads to use for `--threads auto`, and the ceiling on the request.
unsigned default_thread_count();
constexpr unsigned kMaxThreads = 256;

class EncodePool {
public:
    ~EncodePool();

    // Encodes every frame `reader` still holds into `writer`.  `progress` is
    // called from the calling thread after each frame is written, and may be
    // null.  Returns false if reading, encoding or writing failed.
    bool run(DsfReader& reader, DffWriter& writer, unsigned threads,
             int channels, unsigned dsd_rate,
             void (*progress)(void* ctx, uint64_t frames, uint64_t coded_bytes),
             void* ctx);

    uint64_t frames() const { return frames_; }
    uint64_t coded_bytes() const { return coded_bytes_; }
    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    enum State { kEmpty, kFilled, kCoding, kCoded };

    struct Slot {
        uint8_t* src = nullptr;    // planar DSD for one frame
        uint8_t* out = nullptr;    // the coded frame
        size_t out_size = 0;
        int state = kEmpty;
    };

    // The two allocations this program makes, and the only ones: an encoder per
    // worker and a buffer pair per slot.  Both scale with --threads, which is a
    // runtime choice by definition, so there is no maximum worth reserving - at
    // the ceiling of 256 threads these would come to a gigabyte.  Everything
    // within them is fixed size.
    static constexpr size_t kSlotBytes = kMaxFrameBytes + kMaxDstFrameSize;

    DstEncoder* encoders_ = nullptr;
    uint8_t* buffers_ = nullptr;

    bool alloc(unsigned slots);
    void worker(unsigned index);
    void fill(DsfReader& reader);
    bool write_next(DffWriter& writer);   // false once finished or failed

    Slot slots_[kMaxThreads + 2];
    unsigned slot_count_ = 0;
    unsigned threads_ = 0;
    size_t bytes_per_channel_ = 0;
    int channels_ = 0;
    unsigned dsd_rate_ = 0;

    std::mutex mu_;
    std::condition_variable cv_;

    // All of these are guarded by mu_.  The unknown total is what tells the
    // workers that more frames may still arrive.
    static constexpr uint64_t kUnknown = ~uint64_t(0);
    uint64_t next_fill_ = 0;      // frame to read next
    uint64_t next_encode_ = 0;    // frame to hand to the next free worker
    uint64_t next_write_ = 0;     // frame to write next
    uint64_t total_ = kUnknown;   // frame count, once the input has run out
    bool failed_ = false;

    uint64_t frames_ = 0;
    uint64_t coded_bytes_ = 0;
    uint64_t uncoded_frames_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_ENCPOOL_H
