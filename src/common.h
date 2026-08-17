// dff2dsf - shared helpers: fixed-width IO, byte order, buffered file access.
//
// No STL streams, no RTTI, no exceptions: plain stdio and explicit error codes.
#ifndef DFF2DSF_COMMON_H
#define DFF2DSF_COMMON_H


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

inline int32_t file_seek(FILE* f, int64_t offset, int32_t whence) {
    return _fseeki64(f, offset, whence);
}

inline int64_t file_tell(FILE* f) {
    return _ftelli64(f);
}

#else

static_assert(sizeof(off_t) == 8, "64-bit file offsets are required");

inline int32_t file_seek(FILE* f, int64_t offset, int32_t whence) {
    return fseeko(f, off_t(offset), whence);
}

inline int64_t file_tell(FILE* f) {
    return int64_t(ftello(f));
}

#endif

// gmtime_r is POSIX; the Windows CRT spells it gmtime_s, with the arguments the
// other way round.
inline bool utc_now(struct tm* __restrict out) {
    const time_t now = time(nullptr);
#ifdef _WIN32
    return gmtime_s(out, &now) == 0;
#else
    return gmtime_r(&now, out) != nullptr;
#endif
}

// ---------------------------------------------------------------- pointers
//
// Every buffer in this program is a distinct object: frames in, frames out,
// history, per-sample codes, bitplanes, lookup tables.  Nothing is ever aliased,
// and saying so lets the compiler keep values in registers across stores instead
// of reloading them - which matters in the kernels, where a store to the output
// would otherwise be assumed to invalidate the input.
//
// These types spell that promise once so it does not have to be repeated at
// every declaration.  __restrict is spelled the same by GCC, Clang and MSVC.
// They are only used where the promise actually holds; where two pointers could
// name the same bytes, the plain type is used instead.
using ByteP    = uint8_t* __restrict;
using CByteP   = const uint8_t* __restrict;
using WordP    = uint64_t* __restrict;
using CWordP   = const uint64_t* __restrict;
using IntP     = int32_t* __restrict;
using CIntP    = const int32_t* __restrict;
using DoubleP  = double* __restrict;
using UIntP    = uint32_t* __restrict;
using SizeP    = size_t* __restrict;
using CU32P    = const uint32_t* __restrict;
using CBytePP  = CByteP* __restrict;           // out parameter naming a buffer
using VoidP    = void* __restrict;
using CVoidP   = const void* __restrict;

// The prediction lookup: 16 rows of 256, indexed by a byte of history.
using LutRow = int16_t[256];
using LutP   = LutRow* __restrict;
using CLutP  = const LutRow* __restrict;

// The format's fixed table predictors: three orders of up to three taps.
using PredRow = const int8_t[3];
using CPredP  = PredRow* __restrict;

// ---------------------------------------------------------------- diagnostics

#define ERR(...) (fprintf(stderr, "dff2dsf: " __VA_ARGS__), fputc('\n', stderr), false)

// ---------------------------------------------------------------- byte order

inline uint32_t rb32(CByteP p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}

inline uint64_t rb64(CByteP p) {
    return (uint64_t(rb32(p)) << 32) | rb32(p + 4);
}

inline uint16_t rb16(CByteP p) {
    return uint16_t((uint32_t(p[0]) << 8) | p[1]);
}

inline void wl32(ByteP p, uint32_t v) {
    p[0] = uint8_t(v); p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24);
}

inline void wl64(ByteP p, uint64_t v) {
    wl32(p, uint32_t(v)); wl32(p + 4, uint32_t(v >> 32));
}

inline int32_t ilog2(uint32_t v) {
    return v ? 31 - __builtin_clz(v) : 0;
}

// Four-character chunk id comparison. Ids are raw ASCII, never byte-swapped.
inline bool tag_is(CByteP p, const char (&s)[5]) {
    return memcmp(p, s, 4) == 0;
}

// ---------------------------------------------------------------- bit order

// Bit-reversal of a byte, built at compile time.  DSF stores its samples least
// significant bit first while DSDIFF is the other way round, and the DST format
// uses the same table for one of its probabilities.
struct ReverseTable {
    uint8_t v[256];
    constexpr ReverseTable() : v() {
        for (int32_t i = 0; i < 256; i++) {
            uint32_t r = 0;
            for (int32_t b = 0; b < 8; b++)
                r |= ((uint32_t(i) >> b) & 1) << (7 - b);
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

// ---------------------------------------------------------------- streams
//
// All the byte traffic goes through Lib3's coroutine: the driver in
// Lib3/coro_fp.inc reads the input file into a 64 KB buffer and writes the
// output one out, and the conversion runs as a coroutine that pulls and pushes
// bytes through the two pins.  When a pin runs dry or fills up it yields, which
// suspends the whole call stack - parser, codec and all - and returns to the
// driver for a refill or a flush.
//
// The traffic is sequential in both directions, which is what a stream can do.
// The two places that must go back and patch a header once its contents are
// known - a DSF header, and a DSDIFF form and its DST chunk - do that with
// stdio after the coroutine has finished, in finalise().
class Stream {
public:
    void attach(coro3_pin* pin) { pin_ = pin; pos_ = 0; }

    // Bytes read or written so far, which is the position in the file.
    int64_t tell() const { return pos_; }

    // Reads exactly n bytes; a short read is an error unless eof_ok.
    bool read(VoidP buf, size_t n, bool eof_ok = false) {
        const size_t got = read_some(buf, n);
        if (got != n) {
            if (eof_ok && got == 0) return false;
            return ERR("unexpected end of file (wanted %zu, got %zu)", n, got);
        }
        return true;
    }

    // Reads up to n bytes, returning how many were read.  Unlike read(), a
    // short read is not an error: callers use this where a truncated file is
    // something to cope with rather than reject.
    size_t read_some(VoidP buf, size_t n) {
        ByteP d = static_cast<ByteP>(buf);
        size_t left = n;
        while (left) {
            if (!fill()) break;
            const size_t take = window() < left ? window() : left;
            memcpy(d, pin_->ptr, take);
            pin_->ptr += take;
            d += take;
            left -= take;
            pos_ += int64_t(take);
        }
        return n - left;
    }

    bool write(CVoidP buf, size_t n) {
        CByteP s = static_cast<CByteP>(buf);
        while (n) {
            pin_->chkout();                  // yields when the window is full
            const size_t room = pin_->getoutleft();
            const size_t put = room < n ? room : n;
            memcpy(pin_->ptr, s, put);
            pin_->ptr += put;
            s += put;
            n -= put;
            pos_ += int64_t(put);
        }
        return true;
    }

    // Forward only, and free: the bytes are dropped out of the window rather
    // than copied anywhere.
    bool skip(int64_t n) {
        while (n > 0) {
            if (!fill()) return ERR("unexpected end of file");
            const size_t take = int64_t(window()) < n ? window() : size_t(n);
            pin_->ptr += take;
            n -= int64_t(take);
            pos_ += int64_t(take);
        }
        return true;
    }

private:
    size_t window() const { return pin_->getinplen(); }

    // Waits for input, yielding to the driver; false once the file has run out.
    bool fill() {
        while (pin_->ptr >= pin_->end) {
            if (pin_->f_quit()) return false;
            pin_->yield_r();
        }
        return true;
    }

    coro3_pin* pin_ = nullptr;
    int64_t pos_ = 0;
};

} // namespace dff2dsf

#endif // DFF2DSF_COMMON_H
