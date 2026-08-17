// dff2dsf - convert between DSDIFF (.dff) and DSF (.dsf).
//
//   dff2dsf d input.dff output.dsf     decode, undoing DST compression
//   dff2dsf c input.dsf output.dff     compress with DST

// The whole program is one translation unit: each .hpp is an implementation
// file that includes its own declarations, so building is just
//
//     c++ -O2 -std=c++20 src/dff2dsf.cpp -o dff2dsf
#include "dst.hpp"
#include "dsdiff.hpp"
#include "dsf.hpp"
#include "dsfread.hpp"
#include "dstenc.hpp"
#include "dffwrite.hpp"

namespace dff2dsf {

namespace {

int usage() {
    fprintf(stderr,
            "dff2dsf - DSDIFF (DST) and DSF converter\n"
            "\n"
            "usage: dff2dsf d input.dff output.dsf\n"
            "       dff2dsf c input.dsf output.dff\n"
            "\n"
            "  d   decode: DST coded or raw DSD in a .dff, written as a .dsf\n"
            "  c   compress: DSD in a .dsf, written as a DST coded .dff\n");
    return 1;
}

void print_progress(uint64_t done, uint64_t total) {
    if (total)
        fprintf(stderr, "\rdecoding: %llu/%llu frames (%.1f%%)",
                (unsigned long long)done, (unsigned long long)total,
                100.0 * double(done) / double(total));
    else
        fprintf(stderr, "\rdecoding: %llu frames", (unsigned long long)done);
    fflush(stderr);
}

bool decode_dst(DffReader& reader, DsfWriter& writer, uint64_t* frames_out) {
    DstDecoder dec;
    if (!dec.init(reader.channels(), reader.dsd_rate())) return false;

    uint8_t* frame = static_cast<uint8_t*>(xalloc(dec.frame_bytes()));
    if (!frame) return false;

    const size_t bytes_per_channel = dec.frame_bits() / 8;
    uint64_t frames = 0;
    bool ok = true;

    for (;;) {
        const uint8_t* data;
        size_t size, capacity;
        int r = reader.next_dst_frame(&data, &size, &capacity);
        if (r < 0) { ok = false; break; }
        if (r == 0) break;

        if (!dec.decode(data, size, capacity, frame)) {
            ERR("failed decoding frame %llu", (unsigned long long)frames);
            ok = false;
            break;
        }
        if (!writer.write(frame, bytes_per_channel)) { ok = false; break; }

        if ((++frames & 255) == 0) print_progress(frames, reader.frame_count());
    }

    print_progress(frames, reader.frame_count());
    fputc('\n', stderr);

    if (ok && reader.frame_count() && frames != reader.frame_count())
        fprintf(stderr, "dff2dsf: warning: FRTE announced %u frames, found %llu\n",
                reader.frame_count(), (unsigned long long)frames);
    if (dec.uncoded_frames())
        fprintf(stderr, "dff2dsf: %llu frame(s) were stored uncompressed\n",
                (unsigned long long)dec.uncoded_frames());

    free(frame);
    *frames_out = frames;
    return ok;
}

bool decode_raw_dsd(DffReader& reader, DsfWriter& writer) {
    uint64_t bytes = 0;
    for (;;) {
        const uint8_t* data;
        size_t size;
        int r = reader.next_dsd_block(&data, &size);
        if (r < 0) return false;
        if (r == 0) break;

        if (!writer.write(data, size / size_t(reader.channels()))) return false;
        bytes += size;
        fprintf(stderr, "\rcopying: %llu MiB", (unsigned long long)(bytes >> 20));
    }
    fputc('\n', stderr);
    return true;
}

bool decode_file(const char* in_path, const char* out_path) {
    DffReader reader;
    if (!reader.open(in_path)) return false;

    const unsigned rate = reader.dsd_rate();
    const int channels = reader.channels();

    fprintf(stderr, "input : %s\n", in_path);
    fprintf(stderr, "format: %s, %u Hz DSD (DSD%u), %d channel(s)\n",
            reader.is_dst() ? "DST coded" : "raw DSD", rate, rate / 44100, channels);
    if (reader.is_dst() && reader.frame_count())
        fprintf(stderr, "length: %u frames, %.2f s\n", reader.frame_count(),
                double(reader.frame_count()) / (reader.frame_rate() ? reader.frame_rate() : 75));

    DsfWriter writer;
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
        fprintf(stderr, "dst payload: %llu bytes in %llu frames\n",
                (unsigned long long)reader.dst_payload(), (unsigned long long)frames);
    fprintf(stderr, "output: %s\n", out_path);
    return true;
}

bool encode_file(const char* in_path, const char* out_path) {
    DsfReader reader;
    if (!reader.open(in_path)) return false;

    const int channels = reader.channels();
    const unsigned rate = reader.dsd_rate();

    DstEncoder enc;
    if (!enc.init(channels, rate)) return false;

    const size_t bytes_per_channel = enc.frame_bytes_per_channel();
    const uint64_t total_frames =
        (reader.samples_per_channel() + enc.frame_bits() - 1) / enc.frame_bits();

    fprintf(stderr, "input : %s\n", in_path);
    fprintf(stderr, "format: %u Hz DSD (DSD%u), %d channel(s)\n", rate, rate / 44100, channels);
    fprintf(stderr, "length: %llu frames, %.2f s\n", (unsigned long long)total_frames,
            double(reader.samples_per_channel()) / double(rate));

    DffWriter writer;
    if (!writer.open(out_path, channels, rate)) return false;

    uint8_t* src = static_cast<uint8_t*>(xalloc(bytes_per_channel * size_t(channels)));
    uint8_t* frame = static_cast<uint8_t*>(xalloc(enc.max_frame_size()));
    if (!src || !frame) { free(src); free(frame); return false; }

    uint64_t frames = 0, coded_bytes = 0;
    bool ok = true;

    for (;;) {
        int r = reader.read_planar(src, bytes_per_channel);
        if (r < 0) { ok = false; break; }
        if (r == 0) break;

        size_t size = 0;
        if (!enc.encode(src, frame, enc.max_frame_size(), &size)) { ok = false; break; }
        if (!writer.write_frame(frame, size)) { ok = false; break; }

        frames++;
        coded_bytes += size;
        if ((frames & 63) == 0) {
            fprintf(stderr, "\rencoding: %llu/%llu frames (%.1f%%), ratio %.3f",
                    (unsigned long long)frames, (unsigned long long)total_frames,
                    total_frames ? 100.0 * double(frames) / double(total_frames) : 0.0,
                    double(frames * bytes_per_channel * size_t(channels)) / double(coded_bytes));
            fflush(stderr);
        }
    }

    free(src);
    free(frame);
    if (!ok) return false;

    if (!writer.finish()) return false;

    const double raw = double(frames * bytes_per_channel * size_t(channels));
    fprintf(stderr, "\rencoding: %llu frames, ratio %.3f%*s\n",
            (unsigned long long)frames, coded_bytes ? raw / double(coded_bytes) : 0.0, 20, "");
    if (enc.uncoded_frames())
        fprintf(stderr, "dff2dsf: %llu frame(s) did not compress and were stored raw\n",
                (unsigned long long)enc.uncoded_frames());
    fprintf(stderr, "output: %s\n", out_path);
    return true;
}

} // namespace
} // namespace dff2dsf

int main(int argc, char** argv) {
    using namespace dff2dsf;

    if (argc != 4) return usage();
    if (argv[1][1] != '\0' || (argv[1][0] != 'd' && argv[1][0] != 'c')) {
        fprintf(stderr, "dff2dsf: unknown command '%s'\n\n", argv[1]);
        return usage();
    }

    const bool ok = argv[1][0] == 'd' ? decode_file(argv[2], argv[3])
                                      : encode_file(argv[2], argv[3]);
    return ok ? 0 : 1;
}
