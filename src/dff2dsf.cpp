// dff2dsf - convert between DSDIFF (.dff) and DSF (.dsf).
//
//   dff2dsf d input.dff output.dsf     decode, undoing DST compression
//   dff2dsf c input.dsf output.dff     compress with DST
//
// plus --enable-/--disable- flags naming the optional chunks of the .dff.

// The whole program is one translation unit, and this is the only file that
// includes anything: the headers below assume everything before them, so they
// are listed in dependency order rather than including each other.  Building is
//
//     c++ -O2 -std=c++20 -mavx2 -pthread src/dff2dsf.cpp -o dff2dsf

#define _FILE_OFFSET_BITS 64      // must precede the C library headers

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <sys/types.h>            // off_t, for the 64-bit fseeko
#endif

// The vector kernels in dstenc.hpp are selected by whether the target has AVX2.
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <condition_variable>
#include <mutex>
#include <new>
#include <thread>

// One header per module, each depending on the ones above it: every class
// carries its own implementation, so there is nothing else to include.
#include "common.h"
#include "bits.h"
#include "bitwrite.h"
#include "crc.h"
#include "dst.h"
#include "dsdiff.h"
#include "dsf.h"
#include "dsfread.h"
#include "dstenc.h"
#include "dffwrite.h"
#include "encpool.h"


namespace dff2dsf {

namespace {

// The optional chunks of a written .dff, each named after the chunk itself and
// each switchable with --enable-<name> or --disable-<name>.  The defaults live
// in DffWriteOptions, not here, so the two cannot drift apart.
struct Feature {
    const char* name;
    bool DffWriteOptions::* flag;
    const char* what;
};

constexpr Feature kFeatures[] = {
    { "DSTI", &DffWriteOptions::dsti, "frame index, 12 bytes per frame, for seeking" },
    { "COMT", &DffWriteOptions::comt, "comment naming the encoder, with a timestamp" },
    { "ABSS", &DffWriteOptions::abss, "absolute start time, in PROP/SND" },
    { "LSCO", &DffWriteOptions::lsco, "loudspeaker configuration, in PROP/SND" },
    { "DSTC", &DffWriteOptions::dstc, "per-frame CRC over the DSD, 16 bytes per frame" },
};

int32_t usage() {
    fprintf(stderr,
            "dff2dsf - DSDIFF (DST) and DSF converter\n"
            "\n"
            "usage: dff2dsf d input.dff output.dsf\n"
            "       dff2dsf c input.dsf output.dff [options]\n"
            "\n"
            "  d   decode: DST coded or raw DSD in a .dff, written as a .dsf\n"
            "  c   compress: DSD in a .dsf, written as a DST coded .dff\n"
            "\n"
            "  --threads N   encode N frames at a time (default 1, 'auto' for one\n"
            "                per core); the output does not depend on the count\n"
            "\n"
            "options, which select the optional chunks of a written .dff:\n");

    // Each is listed the way round that changes something: what is written by
    // default is offered for removal, and the rest for adding.
    const DffWriteOptions defaults;
    for (const Feature& f : kFeatures) {
        char flag[32];
        snprintf(flag, sizeof(flag), "--%s-%s",
                 defaults.*f.flag ? "disable" : "enable", f.name);
        fprintf(stderr, "  %-16s %s\n", flag, f.what);
    }

    fprintf(stderr,
            "\n"
            "Each has both forms, so a default can also be restated explicitly.\n"
            "FVER, PROP/SND with FS, CHNL and CMPR, and the DST sound data itself\n"
            "are what a decoder needs, and are always written.\n");
    return 1;
}

// Chunk ids are conventionally upper case, but neither shift key is required.
bool name_is(const char* a, const char* b) {
    for (;; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x = char(x - 'a' + 'A');
        if (y >= 'a' && y <= 'z') y = char(y - 'a' + 'A');
        if (x != y) return false;
        if (!x) return true;
    }
}

bool parse_threads(const char* s, UIntP out) {
    if (name_is(s, "auto")) {
        *out = default_thread_count();
        return true;
    }
    char* end = nullptr;
    const uint64_t v = strtoul(s, &end, 10);
    if (end == s || !end || *end || v < 1 || v > kMaxThreads) return false;
    *out = uint32_t(v);
    return true;
}

bool parse_option(const char* arg, DffWriteOptions* __restrict opt) {
    bool enable;
    const char* name;
    if (strncmp(arg, "--enable-", 9) == 0) {
        enable = true;
        name = arg + 9;
    } else if (strncmp(arg, "--disable-", 10) == 0) {
        enable = false;
        name = arg + 10;
    } else {
        return false;
    }

    for (const Feature& f : kFeatures) {
        if (name_is(name, f.name)) {
            opt->*f.flag = enable;
            return true;
        }
    }
    return false;
}

void print_progress(uint64_t done, uint64_t total) {
    if (total)
        fprintf(stderr, "\rdecoding: %" PRIu64 "/%" PRIu64 " frames (%.1f%%)",
                done, total,
                100.0 * double(done) / double(total));
    else
        fprintf(stderr, "\rdecoding: %" PRIu64 " frames", done);
    fflush(stderr);
}

bool decode_dst(DffReader& reader, DsfWriter& writer, WordP frames_out) {
    // One decoder and one frame buffer, both sized for the largest stream this
    // build handles rather than for the one in hand, and both static: nothing
    // here has to be allocated, and neither is small enough to want on a stack.
    static DstDecoder dec;
    static uint8_t frame[kMaxFrameBytes];
    if (!dec.init(reader.channels(), reader.dsd_rate())) return false;

    const size_t bytes_per_channel = dec.frame_bits() / 8;
    uint64_t frames = 0, crcs = 0, crc_errors = 0;
    bool ok = true;

    for (;;) {
        CByteP data;
        size_t size, capacity;
        int32_t r = reader.next_dst_frame(&data, &size, &capacity);
        if (r < 0) { ok = false; break; }
        if (r == 0) break;

        if (!dec.decode(data, size, capacity, frame)) {
            ERR("failed decoding frame %" PRIu64, frames);
            ok = false;
            break;
        }
        // A DSTC chunk covers the DSD the frame decodes to, so it checks the
        // decoding as well as the file.
        if (reader.has_frame_crc()) {
            crcs++;
            if (dsd_crc(0, frame, dec.frame_bytes()) != reader.frame_crc()) {
                if (crc_errors < 8)
                    fprintf(stderr, "\rdff2dsf: warning: frame %" PRIu64 " fails its DSTC CRC\n",
                            frames);
                crc_errors++;
            }
        }

        if (!writer.write(frame, bytes_per_channel)) { ok = false; break; }

        if ((++frames & 255) == 0) print_progress(frames, reader.frame_count());
    }

    print_progress(frames, reader.frame_count());
    fputc('\n', stderr);

    if (ok && reader.frame_count() && frames != reader.frame_count())
        fprintf(stderr, "dff2dsf: warning: FRTE announced %u frames, found %" PRIu64 "\n",
                reader.frame_count(), frames);
    if (dec.uncoded_frames())
        fprintf(stderr, "dff2dsf: %" PRIu64 " frame(s) were stored uncompressed\n",
                dec.uncoded_frames());
    if (crcs)
        fprintf(stderr, "crc: %" PRIu64 " frame(s) carried a DSTC CRC, %" PRIu64 " mismatched\n",
                crcs, crc_errors);

    *frames_out = frames;
    return ok;
}

bool decode_raw_dsd(DffReader& reader, DsfWriter& writer) {
    uint64_t bytes = 0;
    for (;;) {
        CByteP data;
        size_t size;
        int32_t r = reader.next_dsd_block(&data, &size);
        if (r < 0) return false;
        if (r == 0) break;

        if (!writer.write(data, size / size_t(reader.channels()))) return false;
        bytes += size;
        fprintf(stderr, "\rcopying: %" PRIu64 " MiB", (bytes >> 20));
    }
    fputc('\n', stderr);
    return true;
}

bool decode_file(const char* in_path, const char* out_path) {
    // Static for the same reason as the decoder above: the reader's chunk buffer
    // and the writer's block are fixed size, and one of each exists.
    static DffReader reader;
    if (!reader.open(in_path)) return false;

    const uint32_t rate = reader.dsd_rate();
    const int32_t channels = reader.channels();

    fprintf(stderr, "input : %s\n", in_path);
    fprintf(stderr, "format: %s, %u Hz DSD (DSD%u), %d channel(s)\n",
            reader.is_dst() ? "DST coded" : "raw DSD", rate, rate / 44100, channels);
    if (reader.is_dst() && reader.frame_count())
        fprintf(stderr, "length: %u frames, %.2f s\n", reader.frame_count(),
                double(reader.frame_count()) / (reader.frame_rate() ? reader.frame_rate() : 75));

    static DsfWriter writer;
    if (!writer.open(out_path, channels, rate, reader.channel_ids())) return false;

    bool ok;
    uint64_t frames = 0;
    if (reader.is_dst()) {
        ok = decode_dst(reader, writer, &frames);
    } else {
        ok = decode_raw_dsd(reader, writer);
    }

    if (!ok) return false;
    if (!writer.finish()) return false;

    if (reader.is_dst())
        fprintf(stderr, "dst payload: %" PRIu64 " bytes in %" PRIu64 " frames\n",
                reader.dst_payload(), frames);
    fprintf(stderr, "output: %s\n", out_path);
    return true;
}

struct EncodeProgress {
    uint64_t total_frames;
    size_t raw_per_frame;
};

// Called after every frame is written, by either encode path.
void print_encode_progress(void* ctx, uint64_t frames, uint64_t coded_bytes) {
    if (frames & 63) return;
    const EncodeProgress* p = static_cast<const EncodeProgress*>(ctx);
    fprintf(stderr, "\rencoding: %" PRIu64 "/%" PRIu64 " frames (%.1f%%), ratio %.3f",
            frames, p->total_frames,
            p->total_frames ? 100.0 * double(frames) / double(p->total_frames) : 0.0,
            double(frames * p->raw_per_frame) / double(coded_bytes));
    fflush(stderr);
}

// One frame at a time, in this thread: the default, and what the pool below is
// measured against.
bool encode_serial(DsfReader& reader, DffWriter& writer, int32_t channels, uint32_t rate,
                   const EncodeProgress& prog, WordP frames_out,
                   WordP coded_out, WordP uncoded_out) {
    // The encoder is around 3 MB of working buffers, so it is static rather than
    // automatic - a Windows thread gets a 1 MB stack by default - and the two
    // frame buffers are sized for the largest stream this build handles.
    static DstEncoder enc;
    static uint8_t src[kMaxFrameBytes];
    static uint8_t frame[kMaxDstFrameSize];
    if (!enc.init(channels, rate)) return false;

    const size_t bytes_per_channel = enc.frame_bytes_per_channel();
    uint64_t frames = 0, coded_bytes = 0;
    bool ok = true;

    for (;;) {
        int32_t r = reader.read_planar(src, bytes_per_channel);
        if (r < 0) { ok = false; break; }
        if (r == 0) break;

        size_t size = 0;
        if (!enc.encode(src, frame, enc.max_frame_size(), &size)) { ok = false; break; }
        if (!writer.write_frame(frame, size, src, bytes_per_channel)) { ok = false; break; }

        frames++;
        coded_bytes += size;
        print_encode_progress(const_cast<EncodeProgress*>(&prog), frames, coded_bytes);
    }

    *frames_out = frames;
    *coded_out = coded_bytes;
    *uncoded_out = enc.uncoded_frames();
    return ok;
}

bool encode_file(const char* in_path, const char* out_path,
                 const DffWriteOptions& options, uint32_t threads) {
    static DsfReader reader;
    if (!reader.open(in_path)) return false;

    const int32_t channels = reader.channels();
    const uint32_t rate = reader.dsd_rate();

    // The frame geometry follows from the sample rate alone, so both encode
    // paths can have it without building an encoder to ask.
    const uint32_t frame_bits = frame_bits_for(rate);
    if (!frame_bits) return ERR("unsupported sample rate %u", rate);
    const size_t bytes_per_channel = frame_bits / 8;
    const uint64_t total_frames =
        (reader.samples_per_channel() + frame_bits - 1) / frame_bits;

    fprintf(stderr, "input : %s\n", in_path);
    fprintf(stderr, "format: %u Hz DSD (DSD%u), %d channel(s)\n", rate, rate / 44100, channels);
    fprintf(stderr, "length: %" PRIu64 " frames, %.2f s\n", total_frames,
            double(reader.samples_per_channel()) / double(rate));
    if (threads > 1)
        fprintf(stderr, "threads: %u\n", threads);

    // Static like the rest: a File carries its own megabyte of stdio buffer, and
    // a Windows thread starts with a megabyte of stack in total.
    static DffWriter writer;
    if (!writer.open(out_path, channels, rate, options)) return false;

    const EncodeProgress prog = { total_frames, bytes_per_channel * size_t(channels) };
    uint64_t frames = 0, coded_bytes = 0, uncoded = 0;
    bool ok;

    if (threads > 1) {
        EncodePool pool;
        ok = pool.run(reader, writer, threads, channels, rate,
                      print_encode_progress, const_cast<EncodeProgress*>(&prog));
        frames = pool.frames();
        coded_bytes = pool.coded_bytes();
        uncoded = pool.uncoded_frames();
    } else {
        ok = encode_serial(reader, writer, channels, rate, prog,
                           &frames, &coded_bytes, &uncoded);
    }

    if (!ok) return false;
    if (!writer.finish()) return false;

    const double raw = double(frames * prog.raw_per_frame);
    fprintf(stderr, "\rencoding: %" PRIu64 " frames, ratio %.3f%*s\n",
            frames, coded_bytes ? raw / double(coded_bytes) : 0.0, 20, "");
    if (uncoded)
        fprintf(stderr, "dff2dsf: %" PRIu64 " frame(s) did not compress and were stored raw\n",
                uncoded);
    fprintf(stderr, "output: %s\n", out_path);
    return true;
}

} // namespace
} // namespace dff2dsf

int main(int argc, char** argv) {   // the one signature C++ fixes
    using namespace dff2dsf;

    // Command, input and output in that order, with options anywhere among them.
    DffWriteOptions options;
    uint32_t threads = 1;
    const char* arg[3] = {};
    int32_t nargs = 0;
    bool any_option = false;

    for (int32_t i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            // --threads takes a value, either attached or as the next argument.
            if (strncmp(argv[i], "--threads", 9) == 0 &&
                (argv[i][9] == '\0' || argv[i][9] == '=')) {
                const char* value = argv[i][9] == '=' ? argv[i] + 10
                                  : (i + 1 < argc ? argv[++i] : nullptr);
                if (!value || !parse_threads(value, &threads)) {
                    fprintf(stderr, "dff2dsf: --threads wants a count from 1 to %u,"
                                    " or 'auto'\n\n", kMaxThreads);
                    return usage();
                }
                any_option = true;
                continue;
            }
            if (!parse_option(argv[i], &options)) {
                fprintf(stderr, "dff2dsf: unknown option '%s'\n\n", argv[i]);
                return usage();
            }
            any_option = true;
            continue;
        }
        if (nargs == 3) {
            fprintf(stderr, "dff2dsf: unexpected argument '%s'\n\n", argv[i]);
            return usage();
        }
        arg[nargs++] = argv[i];
    }

    if (nargs != 3) return usage();
    if (arg[0][1] != '\0' || (arg[0][0] != 'd' && arg[0][0] != 'c')) {
        fprintf(stderr, "dff2dsf: unknown command '%s'\n\n", arg[0]);
        return usage();
    }

    const bool decoding = arg[0][0] == 'd';
    if (decoding && any_option)
        fprintf(stderr, "dff2dsf: note: those options only apply to 'c'\n");

    const bool ok = decoding ? decode_file(arg[1], arg[2])
                             : encode_file(arg[1], arg[2], options, threads);
    return ok ? 0 : 1;
}
