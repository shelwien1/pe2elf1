// dff2dsf - shared helpers: fixed-width IO, byte order, buffered file access.
//
// No STL streams, no RTTI, no exceptions: plain stdio and explicit error codes.
#ifndef DFF2DSF_COMMON_H
#define DFF2DSF_COMMON_H

#define _FILE_OFFSET_BITS 64

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace dff2dsf {

// ---------------------------------------------------------------- platform
//
// What differs between systems here is the C runtime, not the operating system:
// MinGW and MSVC target the same Windows CRT but are otherwise unalike, so these
// key on the runtime.

// Seeking has to be 64-bit, since these files run past 2 GB.  POSIX spells that
// fseeko, with _FILE_OFFSET_BITS above making off_t wide enough; both Windows
// runtimes spell it _fseeki64 and neither has fseeko at all.
#ifdef _WIN32

inline int file_seek(FILE* f, int64_t offset, int whence) {
    return _fseeki64(f, offset, whence);
}

inline int64_t file_tell(FILE* f) {
    return _ftelli64(f);
}

#else

static_assert(sizeof(off_t) == 8, "64-bit file offsets are required");

inline int file_seek(FILE* f, int64_t offset, int whence) {
    return fseeko(f, off_t(offset), whence);
}

inline int64_t file_tell(FILE* f) {
    return int64_t(ftello(f));
}

#endif

// gmtime_r is POSIX; the Windows CRT spells it gmtime_s, with the arguments the
// other way round.
inline bool utc_now(struct tm* out) {
    const time_t now = time(nullptr);
#ifdef _WIN32
    return gmtime_s(out, &now) == 0;
#else
    return gmtime_r(&now, out) != nullptr;
#endif
}

// ---------------------------------------------------------------- diagnostics

#define ERR(...) (fprintf(stderr, "dff2dsf: " __VA_ARGS__), fputc('\n', stderr), false)

// ---------------------------------------------------------------- byte order

inline uint32_t rb32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}

inline uint64_t rb64(const uint8_t* p) {
    return (uint64_t(rb32(p)) << 32) | rb32(p + 4);
}

inline uint16_t rb16(const uint8_t* p) {
    return uint16_t((uint32_t(p[0]) << 8) | p[1]);
}

inline void wl32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v); p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24);
}

inline void wl64(uint8_t* p, uint64_t v) {
    wl32(p, uint32_t(v)); wl32(p + 4, uint32_t(v >> 32));
}

inline int ilog2(unsigned v) {
    return v ? 31 - __builtin_clz(v) : 0;
}

// Four-character chunk id comparison. Ids are raw ASCII, never byte-swapped.
inline bool tag_is(const uint8_t* p, const char (&s)[5]) {
    return memcmp(p, s, 4) == 0;
}

// ---------------------------------------------------------------- bit order

// Bit-reversal of a byte, built at compile time.  DSF stores its samples least
// significant bit first while DSDIFF is the other way round, and the DST format
// uses the same table for one of its probabilities.
struct ReverseTable {
    uint8_t v[256];
    constexpr ReverseTable() : v() {
        for (int i = 0; i < 256; i++) {
            unsigned r = 0;
            for (int b = 0; b < 8; b++)
                r |= ((unsigned(i) >> b) & 1) << (7 - b);
            v[i] = uint8_t(r);
        }
    }
};
inline constexpr ReverseTable kReverse{};

// ---------------------------------------------------------------- allocation

// Allocation failure is fatal and reported; with -fno-exceptions there is no
// bad_alloc to catch, so every caller checks for null instead.
inline void* xalloc(size_t n) {
    void* p = malloc(n ? n : 1);
    if (!p) fprintf(stderr, "dff2dsf: out of memory (%zu bytes)\n", n);
    return p;
}

// ---------------------------------------------------------------- file access

// Stdio buffer size.  Passing our own buffer to setvbuf rather than a null
// pointer keeps the C library from allocating one, which is the only allocation
// this program would otherwise make per file.
constexpr size_t kFileBuffer = 1 << 20;

class File {
public:
    File() : f_(nullptr) {}
    ~File() { close(); }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool open_read(const char* path) {
        f_ = fopen(path, "rb");
        if (!f_) return ERR("cannot open '%s' for reading", path);
        setvbuf(f_, reinterpret_cast<char*>(buf_), _IOFBF, sizeof(buf_));
        return true;
    }

    bool open_write(const char* path) {
        f_ = fopen(path, "wb+");
        if (!f_) return ERR("cannot open '%s' for writing", path);
        setvbuf(f_, reinterpret_cast<char*>(buf_), _IOFBF, sizeof(buf_));
        return true;
    }

    void close() {
        if (f_) { fclose(f_); f_ = nullptr; }
    }

    // Reads exactly n bytes; a short read is an error unless eof_ok.
    bool read(void* buf, size_t n, bool eof_ok = false) {
        size_t got = fread(buf, 1, n, f_);
        if (got != n) {
            if (eof_ok && got == 0) return false;
            return ERR("unexpected end of file (wanted %zu, got %zu)", n, got);
        }
        return true;
    }

    // Reads up to n bytes, returning how many were read.  Unlike read(), a
    // short read is not an error: callers use this where a truncated file is
    // something to cope with rather than reject.
    size_t read_some(void* buf, size_t n) {
        return fread(buf, 1, n, f_);
    }

    bool write(const void* buf, size_t n) {
        if (fwrite(buf, 1, n, f_) != n) return ERR("write failed");
        return true;
    }

    bool seek(int64_t pos) {
        if (file_seek(f_, pos, SEEK_SET) != 0) return ERR("seek failed");
        return true;
    }

    bool skip(int64_t n) {
        if (file_seek(f_, n, SEEK_CUR) != 0) return ERR("seek failed");
        return true;
    }

    int64_t tell() const { return file_tell(f_); }

    int64_t size() {
        int64_t cur = tell();
        if (file_seek(f_, 0, SEEK_END) != 0) return -1;
        int64_t n = tell();
        file_seek(f_, cur, SEEK_SET);
        return n;
    }

    bool ok() const { return f_ != nullptr; }

private:
    FILE* f_;
    uint8_t buf_[kFileBuffer];   // the stream's buffer, ours rather than libc's
};

} // namespace dff2dsf

#endif // DFF2DSF_COMMON_H
