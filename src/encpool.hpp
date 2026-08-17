// dff2dsf - encoding several DST frames at once.

#ifndef DFF2DSF_ENCPOOL_HPP
#define DFF2DSF_ENCPOOL_HPP

#include "encpool.h"

namespace dff2dsf {

unsigned default_thread_count() {
    const unsigned n = std::thread::hardware_concurrency();
    return n ? (n > kMaxThreads ? kMaxThreads : n) : 1;
}

EncodePool::~EncodePool() {
    for (unsigned i = 0; i < threads_; i++) encoders_[i].~DstEncoder();
    free(encoders_);
    free(buffers_);
}

// One block for every slot's buffer pair, carved up here.
bool EncodePool::alloc(unsigned slots) {
    buffers_ = static_cast<uint8_t*>(xalloc(kSlotBytes * slots));
    if (!buffers_) return false;

    for (unsigned i = 0; i < slots; i++) {
        slots_[i].src = buffers_ + kSlotBytes * i;
        slots_[i].out = slots_[i].src + kMaxFrameBytes;
    }
    slot_count_ = slots;
    return true;
}

// Reads frames into every slot the writer has finished with.  The slot itself is
// filled without the lock held: until its state says otherwise no worker will
// look at it.
void EncodePool::fill(DsfReader& reader) {
    for (;;) {
        SlotP s;
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (failed_ || total_ != kUnknown) return;
            if (next_fill_ - next_write_ >= slot_count_) return;
            s = &slots_[next_fill_ % slot_count_];
        }

        const int r = reader.read_planar(s->src, bytes_per_channel_);

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
bool EncodePool::write_next(DffWriter& writer) {
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

void EncodePool::worker(unsigned index) {
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

bool EncodePool::run(DsfReader& reader, DffWriter& writer, unsigned threads,
                     int channels, unsigned dsd_rate,
                     void (*progress)(void*, uint64_t, uint64_t), void* ctx) {
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
    // ceiling can just live on the stack rather than being allocated.
    std::thread workers[kMaxThreads];
    for (unsigned i = 0; i < threads; i++)
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

    for (unsigned i = 0; i < threads; i++) workers[i].join();

    return !failed_;
}

} // namespace dff2dsf

#endif // DFF2DSF_ENCPOOL_HPP
