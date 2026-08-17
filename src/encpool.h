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

namespace dff2dsf {

constexpr uint32_t kMaxThreads = 256;

// How many threads to use for `--threads auto`, and the ceiling on the request.
inline uint32_t default_thread_count() {
    const uint32_t n = std::thread::hardware_concurrency();
    return n ? (n > kMaxThreads ? kMaxThreads : n) : 1;
}

class EncodePool {
public:
    ~EncodePool() {
        for (uint32_t i = 0; i < threads_; i++) encoders_[i].~DstEncoder();
        free(encoders_);
        free(buffers_);
    }

    // Encodes every frame `reader` still holds into `writer`.  `progress` is
    // called from the calling thread after each frame is written, and may be
    // null.  Returns false if reading, encoding or writing failed.
    bool run(DsfReader& reader, DffWriter& writer, uint32_t threads,
             int32_t channels, uint32_t dsd_rate,
             void (*progress)(void* ctx, uint64_t frames, uint64_t coded_bytes),
             void* ctx) {
        if (threads < 1) threads = 1;
        if (threads > kMaxThreads) threads = kMaxThreads;

        channels_ = channels;
        dsd_rate_ = dsd_rate;
        bytes_per_channel_ = frame_bits_for(dsd_rate) / 8;
        if (!bytes_per_channel_) return ERR("unsupported sample rate %u", dsd_rate);

        // Two spare slots: one being read into and one being written out while every
        // worker has a frame in hand.
        if (!alloc(threads + 2)) return false;

        // One encoder per worker.  They are three megabytes each of fixed size
        // buffers, so they are built here in one block rather than on the workers'
        // stacks, which a Windows thread starts with one megabyte of.
        encoders_ = static_cast<DstEncoder*>(xalloc(sizeof(DstEncoder) * threads));
        if (!encoders_) return false;
        for (; threads_ < threads; threads_++)
            new (&encoders_[threads_]) DstEncoder();

        // A default constructed std::thread is empty and costs a pointer, so the
        // ceiling can just be a member: an array of them on the stack would be
        // copied on every coroutine yield, since run() is called from inside one.
        for (uint32_t i = 0; i < threads; i++)
            workers[i] = std::thread(&EncodePool::worker, this, i);

        for (;;) {
            fill(reader);
            if (!write_next(writer)) break;
            if (progress) progress(ctx, frames_, coded_bytes_);
        }

        // Whatever happened, the workers have to be released before returning: a
        // failure elsewhere leaves them waiting on the queue.
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (total_ == kUnknown) total_ = next_fill_;
        }
        cv_.notify_all();

        for (uint32_t i = 0; i < threads; i++) workers[i].join();

        return !failed_;
    }

    uint64_t frames() const { return frames_; }
    uint64_t coded_bytes() const { return coded_bytes_; }
    uint64_t uncoded_frames() const { return uncoded_frames_; }

private:
    enum State : int32_t { kEmpty, kFilled, kCoding, kCoded };

    struct Slot {
        ByteP src = nullptr;       // planar DSD for one frame
        ByteP out = nullptr;       // the coded frame
        size_t out_size = 0;
        int32_t state = kEmpty;
    };

    // The two allocations this program makes, and the only ones: an encoder per
    // worker and a buffer pair per slot.  Both scale with --threads, which is a
    // runtime choice by definition, so there is no maximum worth reserving - at
    // the ceiling of 256 threads these would come to a gigabyte.  Everything
    // within them is fixed size.
    static constexpr size_t kSlotBytes = kMaxFrameBytes + kMaxDstFrameSize;

    DstEncoder* __restrict encoders_ = nullptr;
    ByteP buffers_ = nullptr;

    // One block for every slot's buffer pair, carved up here.
    bool alloc(uint32_t slots) {
        buffers_ = static_cast<uint8_t*>(xalloc(kSlotBytes * slots));
        if (!buffers_) return false;

        for (uint32_t i = 0; i < slots; i++) {
            slots_[i].src = buffers_ + kSlotBytes * i;
            slots_[i].out = slots_[i].src + kMaxFrameBytes;
        }
        slot_count_ = slots;
        return true;
    }

    void worker(uint32_t index) {
        DstEncoder& enc = encoders_[index];
        if (!enc.init(channels_, dsd_rate_)) {
            {
                std::lock_guard<std::mutex> lock(mu_);
                failed_ = true;
            }
            cv_.notify_all();
            return;
        }

        for (;;) {
            SlotP s = nullptr;
            {
                std::unique_lock<std::mutex> lock(mu_);
                for (;;) {
                    if (failed_) break;
                    // Frames are claimed in order, so the slot for the next one is
                    // either already filled or about to be.
                    if (next_encode_ >= total_) break;
                    Slot& candidate = slots_[next_encode_ % slot_count_];
                    if (candidate.state == kFilled) {
                        candidate.state = kCoding;
                        next_encode_++;
                        s = &candidate;
                        break;
                    }
                    cv_.wait(lock);
                }
            }
            if (!s) break;

            const bool ok = enc.encode(s->src, s->out, kMaxDstFrameSize, &s->out_size);

            {
                std::lock_guard<std::mutex> lock(mu_);
                if (ok) s->state = kCoded; else failed_ = true;
            }
            cv_.notify_all();
            if (!ok) break;
        }

        std::lock_guard<std::mutex> lock(mu_);
        uncoded_frames_ += enc.uncoded_frames();
    }

    // Reads frames into every slot the writer has finished with.  The slot itself is
    // filled without the lock held: until its state says otherwise no worker will
    // look at it.
    void fill(DsfReader& reader) {
        for (;;) {
            SlotP s;
            {
                std::lock_guard<std::mutex> lock(mu_);
                if (failed_ || total_ != kUnknown) return;
                if (next_fill_ - next_write_ >= slot_count_) return;
                s = &slots_[next_fill_ % slot_count_];
            }

            const int32_t r = reader.read_planar(s->src, bytes_per_channel_);

            {
                std::lock_guard<std::mutex> lock(mu_);
                if (r < 0) {
                    failed_ = true;
                } else if (r == 0) {
                    total_ = next_fill_;      // the input ran out here
                } else {
                    s->state = kFilled;
                    next_fill_++;
                }
            }
            cv_.notify_all();
            if (r <= 0) return;
        }
    }

    // Waits for the oldest outstanding frame and writes it, which is what keeps the
    // output in order however the workers finish.
    bool write_next(DffWriter& writer) {
        SlotP s;
        uint64_t index;
        {
            std::unique_lock<std::mutex> lock(mu_);
            for (;;) {
                if (failed_) return false;
                if (total_ != kUnknown && next_write_ >= total_) return false;
                index = next_write_;
                s = &slots_[index % slot_count_];
                if (s->state == kCoded) break;
                cv_.wait(lock);
            }
        }

        const bool ok = writer.write_frame(s->out, s->out_size, s->src, bytes_per_channel_);

        {
            std::lock_guard<std::mutex> lock(mu_);
            if (!ok) {
                failed_ = true;
            } else {
                s->state = kEmpty;
                next_write_ = index + 1;
                frames_++;
                coded_bytes_ += s->out_size;
            }
        }
        cv_.notify_all();
        return ok;
    }

    using SlotP = Slot* __restrict;

    Slot slots_[kMaxThreads + 2];
    std::thread workers[kMaxThreads];
    uint32_t slot_count_ = 0;
    uint32_t threads_ = 0;
    size_t bytes_per_channel_ = 0;
    int32_t channels_ = 0;
    uint32_t dsd_rate_ = 0;

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
