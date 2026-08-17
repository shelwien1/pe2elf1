// dff2dsf - decoding several DST frames at once.
//
// The same arrangement as the encoder's pool, and for the same reason: a DST
// frame starts the decoder's history from a fixed pattern, so nothing carries
// over from the frame before and a frame can be decoded wherever there is a core
// free.  Reading and writing stay in the calling thread and stay in order; only
// the decoding is spread out.
//
// That the calling thread does all the I/O is not a preference here but a
// requirement: it is the coroutine's thread, and reading or writing suspends it
// by copying its stack.  Workers only ever touch buffers.
//
// Frames move through a small ring of slots, one buffer pair each.  The caller's
// thread fills every free slot from the .dff, then blocks until the oldest frame
// has been decoded and writes it, which frees a slot again.  Workers take the
// next undecoded frame in turn.  The ring is a couple of slots longer than the
// number of workers, so reading and writing overlap decoding rather than
// stalling it.
#ifndef DFF2DSF_DECPOOL_H
#define DFF2DSF_DECPOOL_H

namespace dff2dsf {

class DecodePool {
public:
    ~DecodePool() {
        for (uint32_t i = 0; i < threads_; i++) decoders_[i].~DstDecoder();
        free(decoders_);
        free(buffers_);
    }

    // Decodes every frame `reader` still holds into `writer`.  `progress` is
    // called from the calling thread after each frame is written, and may be
    // null.  Returns false if reading, decoding or writing failed.
    bool run(DffReader& reader, DsfWriter& writer, uint32_t threads,
             int32_t channels, uint32_t dsd_rate,
             void (*progress)(void* ctx, uint64_t frames), void* ctx) {
        if (threads < 1) threads = 1;
        if (threads > kMaxThreads) threads = kMaxThreads;

        channels_ = channels;
        dsd_rate_ = dsd_rate;
        const uint32_t frame_bits = frame_bits_for(dsd_rate);
        if (!frame_bits || (frame_bits & 7))
            return ERR("unsupported sample rate %u", dsd_rate);
        bytes_per_channel_ = frame_bits / 8;
        frame_bytes_ = bytes_per_channel_ * size_t(channels);

        // Two spare slots: one being read into and one being written out while
        // every worker has a frame in hand.
        if (!alloc(threads + 2)) return false;

        // One decoder per worker.  They are a couple of hundred kilobytes each
        // of fixed size tables, so they are built here in one block rather than
        // on the workers' stacks, which a Windows thread starts with one
        // megabyte of.
        decoders_ = static_cast<DstDecoder*>(xalloc(sizeof(DstDecoder) * threads));
        if (!decoders_) return false;
        for (; threads_ < threads; threads_++)
            new (&decoders_[threads_]) DstDecoder();

        for (uint32_t i = 0; i < threads; i++)
            workers[i] = std::thread(&DecodePool::worker, this, i);

        for (;;) {
            fill(reader);
            if (!write_next(writer)) break;
            if (progress) progress(ctx, frames_);
        }

        // Whatever happened, the workers have to be released before returning: a
        // failure elsewhere leaves them waiting on the queue.
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (total_ == kUnknown) total_ = next_fill_;
        }
        cv_.notify_all();

        for (uint32_t i = 0; i < threads; i++) workers[i].join();

        // Reported here rather than from the worker that hit it, so that the
        // message names the first bad frame however the workers finished.
        if (bad_frame_ != kUnknown)
            return ERR("failed decoding frame %" PRIu64, bad_frame_);

        return !failed_ && !read_failed_;
    }

    uint64_t frames() const { return frames_; }
    uint64_t crcs() const { return crcs_; }
    uint64_t crc_errors() const { return crc_errors_; }

    uint64_t uncoded_frames() const {
        uint64_t n = 0;
        for (uint32_t i = 0; i < threads_; i++) n += decoders_[i].uncoded_frames();
        return n;
    }

private:
    enum State : int32_t { kEmpty, kFilled, kDecoding, kDecoded };

    struct Slot {
        ByteP src = nullptr;       // the coded frame, followed by the reader's padding
        ByteP out = nullptr;       // interleaved DSD for one frame
        size_t size = 0;           // coded bytes, padding excluded
        size_t capacity = 0;       // coded bytes plus the padding
        uint32_t crc = 0;
        bool has_crc = false;
        bool crc_ok = false;
        int32_t state = kEmpty;
    };

    // The two allocations this direction makes: a decoder per worker and a
    // buffer pair per slot.  Both scale with --threads, which is a runtime
    // choice by definition, so there is no maximum worth reserving.  Everything
    // within them is fixed size.
    static constexpr size_t kSlotBytes =
        kMaxDstFrameSize + kBitReaderPadding + kMaxFrameBytes;

    DstDecoder* __restrict decoders_ = nullptr;
    ByteP buffers_ = nullptr;

    // One block for every slot's buffer pair, carved up here.
    bool alloc(uint32_t slots) {
        buffers_ = static_cast<uint8_t*>(xalloc(kSlotBytes * slots));
        if (!buffers_) return false;

        for (uint32_t i = 0; i < slots; i++) {
            slots_[i].src = buffers_ + kSlotBytes * i;
            slots_[i].out = slots_[i].src + kMaxDstFrameSize + kBitReaderPadding;
        }
        slot_count_ = slots;
        return true;
    }

    void worker(uint32_t index) {
        DstDecoder& dec = decoders_[index];
        if (!dec.init(channels_, dsd_rate_)) {
            {
                std::lock_guard<std::mutex> lock(mu_);
                failed_ = true;
            }
            cv_.notify_all();
            return;
        }

        for (;;) {
            SlotP s = nullptr;
            uint64_t index_of = 0;
            {
                std::unique_lock<std::mutex> lock(mu_);
                for (;;) {
                    if (failed_) break;
                    // Frames are claimed in order, so the slot for the next one
                    // is either already filled or about to be.
                    if (next_decode_ >= total_) break;
                    Slot& candidate = slots_[next_decode_ % slot_count_];
                    if (candidate.state == kFilled) {
                        candidate.state = kDecoding;
                        index_of = next_decode_++;
                        s = &candidate;
                        break;
                    }
                    cv_.wait(lock);
                }
            }
            if (!s) break;

            const bool ok = dec.decode(s->src, s->size, s->capacity, s->out);
            // A DSTC chunk covers the DSD the frame decodes to, so checking it
            // here checks this thread's decoding; the verdict is reported in
            // frame order by whoever writes the frame out.
            if (ok && s->has_crc)
                s->crc_ok = dsd_crc(0, s->out, frame_bytes_) == s->crc;

            {
                std::lock_guard<std::mutex> lock(mu_);
                if (ok) {
                    s->state = kDecoded;
                } else {
                    failed_ = true;
                    if (index_of < bad_frame_) bad_frame_ = index_of;
                }
            }
            cv_.notify_all();
            if (!ok) break;
        }
    }

    // Reads frames into every slot the writer has finished with.  The slot itself
    // is filled without the lock held: until its state says otherwise no worker
    // will look at it.
    void fill(DffReader& reader) {
        for (;;) {
            SlotP s;
            {
                std::lock_guard<std::mutex> lock(mu_);
                if (failed_ || total_ != kUnknown) return;
                if (next_fill_ - next_write_ >= slot_count_) return;
                s = &slots_[next_fill_ % slot_count_];
            }

            CByteP data;
            size_t size, capacity;
            const int32_t r = reader.next_dst_frame(&data, &size, &capacity);
            if (r > 0) {
                // The reader hands back a pointer into its own buffer, which the
                // next call overwrites, so the frame is copied here - padding
                // included, since the bit reader reads into it.
                if (capacity > kMaxDstFrameSize + kBitReaderPadding) {
                    ERR("DST frame of %zu bytes is larger than this build handles", size);
                    {
                        std::lock_guard<std::mutex> lock(mu_);
                        read_failed_ = true;
                        total_ = next_fill_;
                    }
                    cv_.notify_all();
                    return;
                }
                memcpy(s->src, data, capacity);
                s->size = size;
                s->capacity = capacity;
                s->has_crc = reader.has_frame_crc();
                s->crc = reader.frame_crc();
                s->crc_ok = true;
            }

            {
                std::lock_guard<std::mutex> lock(mu_);
                if (r < 0) {
                    // A read that fails ends the input as far as the ring is
                    // concerned rather than abandoning it: the frames already in
                    // hand were read successfully and are still written, which is
                    // what one thread would have done with them.  The failure is
                    // remembered separately so the conversion still reports it.
                    read_failed_ = true;
                    total_ = next_fill_;
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

    // Waits for the oldest outstanding frame and writes it, which is what keeps
    // the output in order however the workers finish.
    bool write_next(DsfWriter& writer) {
        SlotP s;
        uint64_t index;
        {
            std::unique_lock<std::mutex> lock(mu_);
            for (;;) {
                if (failed_) return false;
                if (total_ != kUnknown && next_write_ >= total_) return false;
                index = next_write_;
                s = &slots_[index % slot_count_];
                if (s->state == kDecoded) break;
                cv_.wait(lock);
            }
        }

        // In frame order, so the count and which frames are named do not depend
        // on the thread count.
        if (s->has_crc) {
            crcs_++;
            if (!s->crc_ok) {
                if (crc_errors_ < 8)
                    fprintf(stderr, "\rdff2dsf: warning: frame %" PRIu64
                                    " fails its DSTC CRC\n", index);
                crc_errors_++;
            }
        }

        const bool ok = writer.write(s->out, bytes_per_channel_);

        {
            std::lock_guard<std::mutex> lock(mu_);
            if (!ok) {
                failed_ = true;
            } else {
                s->state = kEmpty;
                next_write_ = index + 1;
                frames_++;
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
    size_t frame_bytes_ = 0;
    int32_t channels_ = 0;
    uint32_t dsd_rate_ = 0;

    std::mutex mu_;
    std::condition_variable cv_;

    // All of these are guarded by mu_.  The unknown total is what tells the
    // workers that more frames may still arrive.
    static constexpr uint64_t kUnknown = ~uint64_t(0);
    uint64_t next_fill_ = 0;      // frame to read next
    uint64_t next_decode_ = 0;    // frame to hand to the next free worker
    uint64_t next_write_ = 0;     // frame to write next
    uint64_t total_ = kUnknown;   // frame count, once the input has run out
    uint64_t bad_frame_ = kUnknown;
    bool failed_ = false;        // decoding or writing gave up; nothing more is written
    bool read_failed_ = false;   // the input gave up; what was read is still written

    // Touched only by the calling thread, in frame order.
    uint64_t frames_ = 0;
    uint64_t crcs_ = 0;
    uint64_t crc_errors_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_DECPOOL_H
