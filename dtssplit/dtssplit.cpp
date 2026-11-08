// dtssplit - lossless split of a DTS stream into its lossy core, a WAV and a metainfo file, and the
// join back
//
// Usage: dtssplit c [options] input.dts prefix
//        dtssplit d [options] prefix restored.dts
//   c    split the stream into prefix.dtsc, prefix.wav and prefix.meta
//   d    join them back into the stream, byte for byte
//   -v   (c) join the files written and compare the result with the input
//   -q   no statistics
// The input of c and the output of d can be - for stdin and stdout (not with -v).
//
//   prefix.dtsc  the core frames: a DTS stream, the lossy part as any DTS decoder plays it
//   prefix.wav   the samples of the lossless extension (XLL); a further layout of channels has a
//                WAV of its own: prefix.2.wav, prefix.3.wav, ...
//   prefix.meta  all else: the extension substream (EXSS) and XLL headers, each segment's coding
//                parameters, every byte that is neither a core frame nor split, and where it all goes
//
// A DTS-HD Master Audio stream is, frame by frame, a DTS core frame (subband ADPCM) and an EXSS frame
// that carries the XLL frame: per channel the difference between the original PCM and the core's
// decode (or the PCM itself), decorrelated in channel pairs, linear-predicted and coded with Rice
// and binary codes, in one frequency band (to 96 kHz) or two (to 192 kHz). The split decodes each
// XLL frame down to the samples it codes, its prediction, decorrelation and MSB/LSB split undone and
// its frequency bands put together, and writes them to the WAV; the join predicts and codes them
// again with the meta file's parameters. The split joins every frame back before it writes it so
// and compares it with the original; a frame that doesn't come back the same, or that it doesn't
// split (four frequency bands, XLL frames that peak bit-rate smoothing spreads over EXSS frames), is
// kept in the meta file as bytes, and so is all that isn't a frame. Any input joins back to itself.
//
// The WAV holds a sample a channel for each sample of each XLL frame, its channels in WAV order (the
// stream's speaker positions), 24-bit (16-bit for 16-bit streams; 32-bit where the XLL embeds a
// downmix, whose lower channel sets hold the downmix unscaled). In a residual channel (the XLL's
// usual one) a sample is the original minus the core's decode, at the XLL's precision: what the
// lossy core leaves out of the audio. In a channel coded without the core it is the audio. The meta
// file keeps what the WAV can't: a value too large for its samples (written clipped), the LSBs'
// bits that overlap the MSBs (bit-width adjustment), and, with two bands, the last 7 lifted samples
// of band 0 in each frame, which the band filter delays into the next.
//
// The meta file: "DTSSPLIT", a version byte, then records, each a tag byte and its fields (numbers
// as LEB128 varints, signed ones zigzagged):
//   R n bytes          bytes that aren't a frame
//   C n                a core frame: the next n bytes of prefix.dtsc
//   E n bytes          an extension substream frame kept as bytes
//   W w ...            WAV w's layout (before its first X): its channels, width and rate, the
//                      speaker mask, the XLL channel of each WAV channel, the channel sets, the bands
//   X w ...            an extension substream frame with its XLL frame split, its samples in WAV w:
//                      the EXSS bytes before the XLL frame, the XLL header bytes, the coding
//                      parameters of each segment of each channel set in each band, the XLL bytes
//                      after the band data, the EXSS bytes after the XLL frame, band 0's delayed
//                      samples, the overlapping LSB bits, the WAV values written clipped
//   Z n crc k s...     the end: the input's size and CRC-32, the WAVs and their samples a channel
//
// Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
// Profiles), clauses 5, 7 and 8. The XLL band filter of two frequency bands, which the specification
// leaves out, is FFmpeg's (libavcodec/dcadsp.c, dcadata.c), so this file is licensed under the GNU
// Lesser General Public License, version 2.1 or later.
//
// Build: g++ -O2 -o dtssplit dtssplit.cpp
//        x86_64-w64-mingw32-g++ -O2 -s -static -o dtssplit.exe dtssplit.cpp

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

typedef uint8_t U8;
typedef int8_t S8;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int32_t S32;
typedef int64_t S64;

#define countof(a) (int)(sizeof(a) / sizeof((a)[0]))

// exit codes
enum { EX_OK, EX_USAGE, EX_READ, EX_WRITE, EX_DATA, EX_FAIL };

static void die(int code, const char* fmt, ...) __attribute__((noreturn, format(printf, 2, 3)));
static void die(int code, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("dtssplit: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(code);
}

// ---------------------------------------------------------------------------------------------
// CRCs

static U16 crc16_tab[256];
static U32 crc32_tab[256];

static void crc_init(void) {
  for (U32 i = 0; i < 256; i++) {
    U32 c = i << 8;
    for (int k = 0; k < 8; k++) c = c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1;
    crc16_tab[i] = (U16)c;
    c = i;
    for (int k = 0; k < 8; k++) c = c & 1 ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    crc32_tab[i] = c;
  }
}

// DTS's CRC-16 (CCITT, from 0xFFFF): 0 over bytes that end with their own CRC
static U16 crc16(const U8* p, size_t n) {
  U32 c = 0xFFFF;
  while (n--) c = ((c << 8) ^ crc16_tab[(c >> 8) ^ *p++]) & 0xFFFF;
  return (U16)c;
}

static U32 crc32_upd(U32 c, const U8* p, size_t n) {
  c = ~c;
  while (n--) c = (c >> 8) ^ crc32_tab[(c ^ *p++) & 0xFF];
  return ~c;
}

// ---------------------------------------------------------------------------------------------
// bits

static inline U64 be64(const U8* p) {
  U64 v;
  memcpy(&v, p, 8);
  return __builtin_bswap64(v);
}
static inline U32 be32(const U8* p) { return (U32)p[0] << 24 | (U32)p[1] << 16 | (U32)p[2] << 8 | p[3]; }

enum { PAD = 16 };  // the bytes a buffer read with BitReader has after its end

// MSB first, over the bits [pos, end) of d (with PAD bytes after end, so a read never leaves the
// buffer); a read past end sets over and returns zeros
struct BitReader {
  const U8* d;
  U64 pos, end;
  bool over;
  void init(const U8* data, U64 start, U64 stop) {
    d = data;
    pos = start;
    end = stop;
    over = false;
  }
  U32 get(int n) {  // n 0-32
    if (n <= 0) return 0;
    if (pos + n > end) {
      over = true;
      pos = end;
      return 0;
    }
    const U64 w = be64(d + (pos >> 3)) << (pos & 7);
    pos += n;
    return (U32)(w >> (64 - n));
  }
  U32 peek32() const { return (U32)((be64(d + (pos >> 3)) << (pos & 7)) >> 32); }  // the next 32 bits
  void skip(U64 n) {
    if (pos + n > end) {
      over = true;
      pos = end;
    } else {
      pos += n;
    }
  }
  // the 0s before the next 1 (which is read too); -1 where no 1 comes before end
  S64 unary() {
    S64 n = 0;
    while (pos < end) {
      U64 w = be64(d + (pos >> 3)) << (pos & 7);
      const U64 lim = end - pos < 57 ? end - pos : 57;
      w &= ~(~0ULL >> lim);
      if (w) {
        const int z = __builtin_clzll(w);
        pos += z + 1;
        return n + z;
      }
      n += lim;
      pos += lim;
    }
    over = true;
    return -1;
  }
};

struct BitWriter {
  std::vector<U8> v;
  U64 acc;
  int nacc;
  void reset() {
    v.clear();
    acc = 0;
    nacc = 0;
  }
  U64 bits() const { return (U64)v.size() * 8 + nacc; }
  void put(U32 x, int n) {  // n 0-32
    if (n <= 0) return;
    if (n < 32) x &= (1u << n) - 1;
    acc = acc << n | x;
    nacc += n;
    while (nacc >= 8) {
      nacc -= 8;
      v.push_back((U8)(acc >> nacc));
    }
    acc &= (1ULL << nacc) - 1;
  }
  void zeros(U64 n) {
    while (n > 32) {
      put(0, 32);
      n -= 32;
    }
    put(0, (int)n);
  }
  void bytes(const U8* p, size_t n) {  // at a byte boundary
    v.insert(v.end(), p, p + n);
  }
  void copy(const U8* src, U64 pos, U64 n) {  // n bits of src from bit pos (PAD bytes after them)
    while (n) {
      const int k = n > 32 ? 32 : (int)n;
      const U64 w = be64(src + (pos >> 3)) << (pos & 7);
      put((U32)(w >> (64 - k)), k);
      pos += k;
      n -= k;
    }
  }
  void align() { put(0, (8 - nacc) & 7); }
};

// the signed value of a code's unsigned word, and back
static inline S32 unzig(U32 u) { return u & 1 ? -(S32)(u >> 1) - 1 : (S32)(u >> 1); }
static inline U32 zig(S32 v) { return v < 0 ? (((U32)(-(v + 1))) << 1) | 1 : (U32)v << 1; }

#include "dtscore.inc"

// ---------------------------------------------------------------------------------------------
// records: built in memory, read from memory or from a file

struct Rec {
  std::vector<U8> b;
  void clear() { b.clear(); }
  void u8(int x) { b.push_back((U8)x); }
  void uv(U64 x) {
    while (x >= 0x80) {
      b.push_back((U8)(x | 0x80));
      x >>= 7;
    }
    b.push_back((U8)x);
  }
  void sv(S64 x) { uv(x < 0 ? ((U64)(-(x + 1)) << 1) | 1 : (U64)x << 1); }
  void bytes(const U8* p, size_t n) { b.insert(b.end(), p, p + n); }
  void blob(const U8* p, size_t n) {
    uv(n);
    bytes(p, n);
  }
};

// a reader of records: from memory (for the split's check) or a file (the join)
struct RecIn {
  FILE* f;
  const char* what;
  const U8* mem;
  size_t msize, mpos;
  std::vector<U8> buf;
  size_t lo, hi;
  bool bad;   // read past the end, or a number that doesn't fit
  U64 nread;  // bytes read
  void init_file(FILE* fp, const char* name) {
    f = fp;
    what = name;
    mem = 0;
    buf.resize(1 << 16);
    lo = hi = 0;
    bad = false;
    nread = 0;
  }
  void init_mem(const U8* p, size_t n) {
    f = 0;
    what = "memory";
    mem = p;
    msize = n;
    mpos = 0;
    bad = false;
    nread = 0;
  }
  bool refill() {
    lo = 0;
    hi = fread(&buf[0], 1, buf.size(), f);
    if (!hi && ferror(f)) die(EX_READ, "can't read %s", what);
    return hi != 0;
  }
  int u8() {
    if (!f) {
      if (mpos >= msize) {
        bad = true;
        return 0;
      }
      nread++;
      return mem[mpos++];
    }
    if (lo == hi && !refill()) {
      bad = true;
      return 0;
    }
    nread++;
    return buf[lo++];
  }
  bool at_end() {  // the end of the input
    if (!f) return mpos >= msize;
    return lo == hi && !refill();
  }
  U64 uv() {
    U64 x = 0;
    for (int s = 0; s < 64; s += 7) {
      const int c = u8();
      x |= (U64)(c & 0x7F) << s;
      if (!(c & 0x80)) return x;
    }
    bad = true;
    return 0;
  }
  S64 sv() {
    const U64 u = uv();
    return u & 1 ? -(S64)(u >> 1) - 1 : (S64)(u >> 1);
  }
  void bytes(U8* p, size_t n) {
    if (!f) {
      if (n > msize - mpos) {
        bad = true;
        memset(p, 0, n);
        mpos = msize;
        return;
      }
      memcpy(p, mem + mpos, n);
      mpos += n;
      nread += n;
      return;
    }
    while (n) {
      if (lo == hi && !refill()) {
        bad = true;
        memset(p, 0, n);
        return;
      }
      const size_t k = hi - lo < n ? hi - lo : n;
      memcpy(p, &buf[lo], k);
      lo += k;
      p += k;
      n -= k;
      nread += k;
    }
  }
  // a length-prefixed byte string, at most max bytes (PAD zero bytes after it, not counted)
  bool blob(std::vector<U8>& out, size_t max) {
    const U64 n = uv();
    if (bad || n > max) {
      bad = true;
      return false;
    }
    out.resize(n + PAD);
    bytes(&out[0], n);
    memset(&out[n], 0, PAD);
    out.resize(n);
    return !bad;
  }
};

// ---------------------------------------------------------------------------------------------
// WAV

enum { WAV_HEAD = 104 };  // the header the split writes: RIFF, JUNK (ds64 in RF64), fmt, data

static void put16(U8* p, U32 x) {
  p[0] = (U8)x;
  p[1] = (U8)(x >> 8);
}
static void put32(U8* p, U32 x) {
  put16(p, x);
  put16(p + 2, x >> 16);
}
static void put64(U8* p, U64 x) {
  put32(p, (U32)x);
  put32(p + 4, (U32)(x >> 32));
}
static U32 get16(const U8* p) { return p[0] | (U32)p[1] << 8; }
static U32 get32(const U8* p) { return get16(p) | get16(p + 2) << 16; }
static U64 get64(const U8* p) { return get32(p) | (U64)get32(p + 4) << 32; }

static const U8 PCM_GUID[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};

struct WavOut {
  FILE* f;
  std::string name;
  int nch, bits, rate;
  U32 mask;
  U64 frames;  // samples a channel written
  U64 nbytes;  // data bytes written
  std::vector<U8> buf;
  void open(const std::string& path) {
    name = path;
    f = fopen(path.c_str(), "wb");
    if (!f) die(EX_WRITE, "can't create %s", path.c_str());
    nch = 1;
    bits = 16;
    rate = 48000;
    mask = 0;
    frames = nbytes = 0;
    U8 z[WAV_HEAD] = {0};
    if (fwrite(z, 1, WAV_HEAD, f) != WAV_HEAD) die(EX_WRITE, "can't write %s", name.c_str());
  }
  void write(const S32* v, size_t count) {  // count values, interleaved
    const int bps = bits / 8;
    buf.resize(count * bps);
    U8* q = &buf[0];
    for (size_t i = 0; i < count; i++) {
      const U32 x = (U32)v[i];
      q[0] = (U8)x;
      q[1] = (U8)(x >> 8);
      if (bps >= 3) q[2] = (U8)(x >> 16);
      if (bps == 4) q[3] = (U8)(x >> 24);
      q += bps;
    }
    if (count && fwrite(&buf[0], 1, buf.size(), f) != buf.size()) die(EX_WRITE, "can't write %s", name.c_str());
    nbytes += buf.size();
    frames += count / nch;
  }
  U64 size() const { return WAV_HEAD + nbytes + (nbytes & 1); }
  void finish() {
    U8 h[WAV_HEAD];
    memset(h, 0, sizeof h);
    const int pad = (int)(nbytes & 1);
    if (pad && fputc(0, f) == EOF) die(EX_WRITE, "can't write %s", name.c_str());
    const U64 riff = WAV_HEAD - 8 + nbytes + pad;
    const bool rf64 = riff > 0xFFFFFFFFull;
    memcpy(h, rf64 ? "RF64" : "RIFF", 4);
    put32(h + 4, rf64 ? 0xFFFFFFFFu : (U32)riff);
    memcpy(h + 8, "WAVE", 4);
    memcpy(h + 12, rf64 ? "ds64" : "JUNK", 4);
    put32(h + 16, 28);
    if (rf64) {
      put64(h + 20, riff);
      put64(h + 28, nbytes);
      put64(h + 36, frames);
    }
    memcpy(h + 48, "fmt ", 4);
    put32(h + 52, 40);
    put16(h + 56, 0xFFFE);
    put16(h + 58, nch);
    put32(h + 60, rate);
    put32(h + 64, (U32)rate * nch * (bits / 8));
    put16(h + 68, nch * (bits / 8));
    put16(h + 70, bits);
    put16(h + 72, 22);
    put16(h + 74, bits);
    put32(h + 76, mask);
    memcpy(h + 80, PCM_GUID, 16);
    memcpy(h + 96, "data", 4);
    put32(h + 100, rf64 ? 0xFFFFFFFFu : (U32)nbytes);
    if (fflush(f) || fseek(f, 0, SEEK_SET) || fwrite(h, 1, WAV_HEAD, f) != WAV_HEAD || fclose(f))
      die(EX_WRITE, "can't write %s's header", name.c_str());
    f = 0;
  }
};

// any PCM WAV (RIFF or RF64, WAVE_FORMAT_PCM or EXTENSIBLE): what the join reads samples from
struct WavIn {
  FILE* f;
  int nch, bits, rate;
  U64 left;  // data bytes not read yet (~0: to the end of the file)
  std::vector<U8> buf;
  const char* open(FILE* fp) {
    f = fp;
    U8 h[12];
    if (fread(h, 1, 12, f) != 12 || (memcmp(h, "RIFF", 4) && memcmp(h, "RF64", 4)) || memcmp(h + 8, "WAVE", 4))
      return "not a WAV file";
    const bool rf64 = !memcmp(h, "RF64", 4);
    U64 ds64 = ~0ull;
    bool fmt = false;
    for (;;) {
      U8 c[8];
      if (fread(c, 1, 8, f) != 8) return "a WAV without data";
      const U64 n = get32(c + 4);
      if (!memcmp(c, "data", 4)) {
        if (!fmt) return "a WAV without its format before its data";
        left = n == 0xFFFFFFFFu ? (rf64 ? ds64 : ~0ull) : n;
        return 0;
      }
      std::vector<U8> d(n + (n & 1));
      if (n > (1u << 20) || (d.size() && fread(&d[0], 1, d.size(), f) != d.size())) return "a WAV chunk that can't be read";
      if (!memcmp(c, "ds64", 4) && n >= 16) ds64 = get64(&d[8]);
      if (!memcmp(c, "fmt ", 4)) {
        if (n < 16) return "a WAV format chunk too short";
        const U32 tag = get16(&d[0]);
        nch = get16(&d[2]);
        rate = get32(&d[4]);
        bits = get16(&d[14]);
        if (tag == 0xFFFE && (n < 40 || memcmp(&d[24], PCM_GUID, 16))) return "a WAV that isn't PCM";
        if (tag != 1 && tag != 0xFFFE) return "a WAV that isn't PCM";
        if ((bits != 16 && bits != 24 && bits != 32) || nch < 1) return "a WAV that isn't 16, 24 or 32-bit";
        fmt = true;
      }
    }
  }
  bool read(S32* v, size_t count) {  // count interleaved values; false where the data ends first
    const int bps = bits / 8;
    const size_t n = count * bps;
    if (left != ~0ull && n > left) return false;
    buf.resize(n);
    if (n && fread(&buf[0], 1, n, f) != n) {
      if (ferror(f)) die(EX_READ, "can't read a WAV");
      return false;
    }
    if (left != ~0ull) left -= n;
    const U8* q = &buf[0];
    for (size_t i = 0; i < count; i++) {
      v[i] = bps == 4   ? (S32)get32(q)
             : bps == 3 ? (S32)((U32)q[0] << 8 | (U32)q[1] << 16 | (U32)q[2] << 24) >> 8
                        : (S32)(int16_t)get16(q);
      q += bps;
    }
    return true;
  }
};

// ---------------------------------------------------------------------------------------------
// the stream: core frames and extension substream frames

enum { SYNC_CORE = 0x7FFE8001, SYNC_EXSS = 0x64582025, SYNC_XLL = 0x41A29547 };

// the size of the core frame at p (av bytes there), where its header says one is there: the sync
// word and a frame size and block count in range; 0 where it isn't
static int core_frame(const U8* p, size_t av) {
  if (av < 16 || be32(p) != SYNC_CORE) return 0;
  BitReader r;
  r.init(p, 32, 16 * 8);
  r.get(1);  // FTYPE
  r.get(5);  // SHORT
  r.get(1);  // CPF
  const int nblks = r.get(7);
  const int fsize = r.get(14) + 1;
  if (nblks < 5 || fsize < 96 || (size_t)fsize > av) return 0;
  return fsize;
}

// the size of the extension substream frame at p, where it is one: the sync word and a header
// whose CRC holds; 0 where it isn't
static int exss_frame(const U8* p, size_t av) {
  if (av < 16 || be32(p) != SYNC_EXSS) return 0;
  BitReader r;
  r.init(p, 32, 16 * 8);
  r.get(8);  // UserDefinedBits
  r.get(2);  // nExtSSIndex
  const int wide = r.get(1);
  const int hs = r.get(wide ? 12 : 8) + 1, fs = r.get(wide ? 20 : 16) + 1;
  if (hs < 8 || hs > fs || (size_t)fs > av) return 0;
  if (crc16(p + 5, hs - 5)) return 0;
  return fs;
}

// what the extension substream headers have said in their static fields (which need not be in
// every header), for the headers without them
struct ExssState {
  bool stat;  // static fields seen
  int mix, nmixcfg, nmixch[4];
  bool known[8];
  int one2one[8], es[8], e6[8], totch[8];
};

static int numspk(U32 mask) { return __builtin_popcount(mask) + __builtin_popcount(mask & 0xAE66); }

// The XLL data of the extension substream frame p (n bytes): its offset and size, and whether its
// channels are speaker feeds (one2one: -1 where the header doesn't say); false where it has none, or
// the header can't be followed (why says why)
static bool exss_xll(ExssState& st, const U8* p, int n, int& off, int& size, int& one2one, const char*& why) {
  BitReader r;
  r.init(p, 32, (U64)n * 8);
  r.get(8);
  const int idx = r.get(2), wide = r.get(1);
  const int hs = r.get(wide ? 12 : 8) + 1, nb = wide ? 20 : 16;
  r.get(nb);
  const int stat = r.get(1);
  int npres = 1, nassets = 1;
  if (stat) {
    r.get(2);  // nuRefClockCode
    r.get(3);  // nuExSSFrameDurationCode
    if (r.get(1)) r.skip(36);
    npres = r.get(3) + 1;
    nassets = r.get(3) + 1;
    int amask[8];
    for (int i = 0; i < npres; i++) amask[i] = r.get(idx + 1);
    for (int i = 0; i < npres; i++)
      for (int s = 0; s <= idx; s++)
        if (amask[i] >> s & 1) r.get(8);
    st.mix = r.get(1);
    if (st.mix) {
      r.get(2);
      const int mb = (r.get(2) + 1) << 2;
      st.nmixcfg = r.get(2) + 1;
      for (int i = 0; i < st.nmixcfg; i++) st.nmixch[i] = numspk(r.get(mb));
    }
    st.stat = true;
  }
  if (!st.stat) {
    // no static fields yet: the XLL frame by its sync word (DWORD-aligned, its header's CRC holding)
    for (int o = (hs + 3) & ~3; o + 16 <= n; o += 4)
      if (be32(p + o) == SYNC_XLL) {
        const int h = ((p[o + 4] & 15) << 4 | p[o + 5] >> 4) + 1;
        if (h >= 12 && o + h <= n && !crc16(p + o + 4, h - 4)) {
          off = o;
          size = n - o;
          one2one = -1;
          return true;
        }
      }
    why = "no extension substream static fields yet, and no XLL sync word";
    return false;
  }
  int asize[8], aoff[8], o = hs;
  for (int a = 0; a < nassets; a++) {
    asize[a] = r.get(nb) + 1;
    aoff[a] = o;
    o += asize[a];
  }
  if (r.over || o > n) {
    why = "extension substream assets past the frame";
    return false;
  }
  for (int a = 0; a < nassets; a++) {
    const U64 dpos = r.pos;
    const int dsize = r.get(9) + 1;
    const int ai = r.get(3);
    if (stat) {
      if (r.get(1)) r.get(4);
      if (r.get(1)) r.get(24);
      if (r.get(1)) r.skip((U64)(r.get(10) + 1) * 8);
      r.get(5);  // nuBitResolution
      r.get(4);  // nuMaxSampleRate
      const int tot = r.get(8) + 1;
      const int oo = r.get(1);
      int es = 0, e6 = 0;
      if (oo) {
        es = tot > 2 ? r.get(1) : 0;
        e6 = tot > 6 ? r.get(1) : 0;
        int mb = 0;
        if (r.get(1)) {
          mb = (r.get(2) + 1) << 2;
          r.get(mb);
        }
        const int nrs = r.get(3);
        if (nrs && !mb) {
          why = "speaker remapping without a speaker mask";
          return false;
        }
        U32 lay[8];
        for (int s = 0; s < nrs; s++) lay[s] = r.get(mb);
        for (int s = 0; s < nrs; s++) {
          const int ns = numspk(lay[s]), nd = r.get(5) + 1;
          for (int c = 0; c < ns; c++) r.skip((U64)__builtin_popcount(r.get(nd)) * 5);
        }
      } else {
        r.get(3);  // nuRepresentationType
      }
      st.known[ai] = true;
      st.one2one[ai] = oo;
      st.es[ai] = es;
      st.e6[ai] = e6;
      st.totch[ai] = tot;
    }
    if (!st.known[ai]) {
      r.pos = dpos + (U64)dsize * 8;
      continue;
    }
    const int drc = r.get(1);
    if (drc) r.get(8);
    if (r.get(1)) r.get(5);
    if (drc && st.es[ai]) r.get(8);
    if (st.mix && r.get(1)) {
      r.get(1);
      r.get(6);
      if (r.get(2) < 3)
        r.get(3);
      else
        r.get(8);
      const int perch = r.get(1);
      for (int s = 0; s < st.nmixcfg; s++) r.skip(perch ? 6 * st.nmixch[s] : 6);
      int nd[3], ne = 0;
      nd[ne++] = st.totch[ai];
      if (st.e6[ai]) nd[ne++] = 6;
      if (st.es[ai]) nd[ne++] = 2;
      for (int s = 0; s < st.nmixcfg; s++)
        for (int e = 0; e < ne; e++)
          for (int c = 0; c < nd[e]; c++) r.skip((U64)__builtin_popcount(r.get(st.nmixch[s])) * 6);
    }
    int mask = 0, csz[6] = {0, 0, 0, 0, 0, 0};  // core, XBR, XXCH, X96, LBR, XLL
    switch (r.get(2)) {
      case 0:
        mask = r.get(12);
        if (mask & 0x010) {
          csz[0] = r.get(14) + 1;
          if (r.get(1)) r.get(2);
        }
        if (mask & 0x020) csz[1] = r.get(14) + 1;
        if (mask & 0x040) csz[2] = r.get(14) + 1;
        if (mask & 0x080) csz[3] = r.get(12) + 1;
        if (mask & 0x100) {
          csz[4] = r.get(14) + 1;
          if (r.get(1)) r.get(2);
        }
        if (mask & 0x200) {
          csz[5] = r.get(nb) + 1;
          if (r.get(1)) {
            r.get(4);
            r.get(r.get(5) + 1);
            r.get(nb);
          }
        }
        if (mask & 0x400) r.get(16);
        if (mask & 0x800) r.get(16);
        break;
      case 1:
        mask = 0x200;
        csz[5] = r.get(nb) + 1;
        if (r.get(1)) {
          r.get(4);
          r.get(r.get(5) + 1);
          r.get(nb);
        }
        break;
      case 2:
        mask = 0x100;
        csz[4] = r.get(14) + 1;
        if (r.get(1)) r.get(2);
        break;
      default:
        r.get(14);
        r.get(8);
        if (r.get(1)) r.get(3);
        break;
    }
    if (r.over) {
      why = "an asset descriptor past the header";
      return false;
    }
    r.pos = dpos + (U64)dsize * 8;
    if (mask & 0x200) {
      int x = aoff[a];
      for (int i = 0; i < 5; i++)
        if (mask & (0x010 << i)) x += csz[i];
      if (x + csz[5] > aoff[a] + asize[a]) {
        why = "XLL data past its asset";
        return false;
      }
      off = x;
      size = csz[5];
      one2one = st.one2one[ai];
      return true;
    }
  }
  why = "no XLL";
  return false;
}

// ---------------------------------------------------------------------------------------------
// XLL

enum { XCH = 16, XCS = 16, XORD = 16, XSEG = 512, XFRAME = 65536, XWAV = 64, XBAND = 4, XNWAV = 16, XDH = 8 };

static const int FREQ[16] = {8000,  16000, 32000, 64000,  128000, 22050,  44100,  88200,
                             176400, 352800, 12000, 24000, 48000, 96000, 192000, 384000};
static const int DMIX_NCH[8] = {1, 2, 2, 3, 3, 4, 4, 0};

// the quantized reflection coefficients (TS 102 114 8.5.4)
static const U16 RC_TAB[128] = {
    0,     3070,  5110,  7140,  9156,  11154, 13132, 15085, 17010, 18904, 20764, 22588, 24373, 26117, 27818, 29474,
    31085, 32648, 34164, 35631, 37049, 38418, 39738, 41008, 42230, 43404, 44530, 45609, 46642, 47630, 48575, 49477,
    50337, 51157, 51937, 52681, 53387, 54059, 54697, 55302, 55876, 56421, 56937, 57426, 57888, 58326, 58741, 59132,
    59502, 59852, 60182, 60494, 60789, 61066, 61328, 61576, 61809, 62029, 62236, 62431, 62615, 62788, 62951, 63105,
    63250, 63386, 63514, 63635, 63749, 63855, 63956, 64051, 64140, 64224, 64302, 64376, 64446, 64512, 64573, 64631,
    64686, 64737, 64785, 64830, 64873, 64913, 64950, 64986, 65019, 65050, 65079, 65107, 65133, 65157, 65180, 65202,
    65222, 65241, 65259, 65275, 65291, 65306, 65320, 65333, 65345, 65357, 65368, 65378, 65387, 65396, 65405, 65413,
    65420, 65427, 65434, 65440, 65446, 65451, 65456, 65461, 65466, 65470, 65474, 65478, 65481, 65485, 65488, 65491};

// the lifting steps' coefficients of the two-band filter (FFmpeg's ff_dca_xll_band_coeff)
static const S32 BAND_COEFF[20] = {868669,   -5931642, -1228483, 4194304,  -20577,  122631,   -393647,
                                   904476,   -1696305, 2825313,  -4430736, 6791313, 41153,    -245210,
                                   785564,   -1788164, 3259333,  -5074941, 6928550, -8204883};

struct Band {  // a channel set's coding in a frequency band (its part of the sub-header)
  int decor, order[XCH], dcoeff[XCH / 2];
  int aorder[XCH], forder[XCH], highest;
  int rc[XCH][XORD];
  int dmix_embedded;
  int lsb_size, nlsb[XCH], adj[XCH];  // lsb_size: the bytes at the end of a segment's data the LSB part takes
};

struct Chset {  // a channel set: its sub-header, and the coding parameters as they go from segment to segment
  int nch, resenc, pcm_res, storage_res, freq, nbands;
  int primary, dmix_coeffs, dmix_embedded, dmix_type, hier, chmask, hier_ofs, nabits;
  Band band[XBAND];
  int seg_common, rice[XCH], aux[XCH], abit_a[XCH], abit_b[XCH], na[XCH];
};

struct Xll {
  int hsize, frame_size, nchsets, nsegs, nseg_log2, nsegsamples, nframesamples, seg_nbits, band_crc, scalable,
      chmask_nbits, fixed_lsb, nbands;
  int nch;  // all channel sets'
  int chofs[XCS + 1];
  Chset cs[XCS];
  std::vector<int> navi;  // band data sizes in bytes: [band][segment][channel set]
  int data_start;         // where the band data starts
};

// a segment of a channel set's band data: what isn't samples
struct Chunk {
  U8 reuse, seg_common;
  U8 rice[XCH], hyb[XCH], aux_code[XCH], a_code[XCH], b_code[XCH];
  std::vector<U16> iso[XCH];  // per channel, where hybrid Rice codes put their isolated samples
  U8 g1, g2;                  // the bits after the MSB part and after the LSB part: 0 zeros, 1 as kept
  U32 g1n, g2n;
  std::vector<U8> g1b, g2b;
  int dh_code;     // band 1, segment 0: the decimator history's width (less 1)
  S32 dh[XCH][XDH];  // and its samples, [1-7] of each channel
};

static inline S32 sat23(S32 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : a; }
static inline S32 mul16(S32 a, S32 b) { return (S32)(((S64)a * b + (1 << 15)) >> 16); }
static inline S32 mulq(S32 a, S32 b, int q) { return (S32)(((S64)a * b + (1LL << (q - 1))) >> q); }

// the common header, the channel set sub-headers and the NAVI table of the XLL frame p (n bytes,
// PAD more): 0, or why they can't be followed
static const char* xll_headers(Xll& x, const U8* p, int n, int one2one) {
  BitReader r;
  r.init(p, 0, (U64)n * 8);
  if (n < 16 || r.get(32) != SYNC_XLL) return "no XLL sync word";
  if (r.get(4) + 1 > 1) return "an XLL version above 1";
  x.hsize = r.get(8) + 1;
  if (x.hsize < 12 || x.hsize > n || crc16(p + 4, x.hsize - 4)) return "an XLL header whose CRC fails";
  const int fsnb = r.get(5) + 1;
  x.frame_size = (int)((U64)r.get(fsnb) + 1);
  if (x.frame_size > n || x.frame_size < x.hsize) return "an XLL frame larger than its asset";
  x.nchsets = r.get(4) + 1;
  const int segl = r.get(4);
  x.nsegs = 1 << segl;
  x.nseg_log2 = r.get(4);
  x.nsegsamples = 1 << x.nseg_log2;
  if (x.nsegs > 1024 || !x.nseg_log2 || x.nsegsamples > XSEG || (x.nsegsamples << segl) > XFRAME)
    return "XLL segments out of range";
  x.nframesamples = x.nsegsamples << segl;
  x.seg_nbits = r.get(5) + 1;
  x.band_crc = r.get(2);
  x.scalable = r.get(1);
  x.chmask_nbits = r.get(5) + 1;
  x.fixed_lsb = x.scalable ? r.get(4) : 0;
  if (r.pos > (U64)(x.hsize - 2) * 8) return "an XLL header too short";
  int off = x.hsize, hier = 0;
  x.nch = 0;
  x.nbands = 1;
  for (int c = 0; c < x.nchsets; c++) {
    Chset& s = x.cs[c];
    memset(&s, 0, sizeof s);
    r.init(p, (U64)off * 8, (U64)n * 8);
    const int hs = r.get(10) + 1;
    if (off + hs > n || hs < 3 || crc16(p + off, hs)) return "a channel set header whose CRC fails";
    s.nch = r.get(4) + 1;
    s.resenc = r.get(s.nch);
    s.pcm_res = r.get(5) + 1;
    s.storage_res = r.get(5) + 1;
    s.freq = FREQ[r.get(4)];
    r.get(2);  // nFsInterpolate
    if (r.get(2)) r.get(1);
    s.hier_ofs = hier;
    if (one2one) {
      s.primary = r.get(1);
      s.dmix_coeffs = r.get(1);
      s.dmix_embedded = s.dmix_coeffs ? r.get(1) : 0;
      s.dmix_type = s.dmix_coeffs && s.primary ? r.get(3) : 0;
      s.hier = r.get(1);
      if (s.dmix_coeffs) {
        if (s.primary && !DMIX_NCH[s.dmix_type]) return "a downmix type out of range";
        const int m = s.primary ? DMIX_NCH[s.dmix_type] : s.hier_ofs;
        r.skip((U64)m * (s.nch + !s.primary) * 9);
      }
      if (r.get(1)) {
        s.chmask = (int)r.get(x.chmask_nbits);
      } else {
        s.chmask = 0;
        r.skip((U64)s.nch * 25);
      }
    } else {
      s.primary = 1;
      s.hier = 1;
      if (r.get(1)) {
        const int cb = 6 + 2 * r.get(3), ncfg = r.get(2) + 1;
        for (int k = 0; k < ncfg; k++) {
          const U32 am = r.get(s.nch);
          const int nspk = r.get(6) + 1, sm = r.get(1);
          if (sm) r.get(x.chmask_nbits);
          for (int t = 0; t < nspk; t++) {
            if (!sm) r.skip(25);
            r.skip((U64)__builtin_popcount(am) * cb);
          }
        }
      }
    }
    // (decoders take bXtraFreqBands 0 for two bands, though the text of 8.3.2 has one)
    s.nbands = s.freq > 96000 ? (r.get(1) ? 4 : 2) : 1;
    if (s.nbands > x.nbands) x.nbands = s.nbands;
    int chbits = 0;
    while ((1 << chbits) < s.nch) chbits++;
    for (int b = 0; b < s.nbands; b++) {
      Band& d = s.band[b];
      d.decor = r.get(1);
      if (d.decor && s.nch > 1) {
        for (int ch = 0; ch < s.nch; ch++) d.order[ch] = r.get(chbits);
        for (int i = 0; i < s.nch / 2; i++) d.dcoeff[i] = r.get(1) ? unzig(r.get(7)) : 0;
      } else {
        for (int ch = 0; ch < s.nch; ch++) d.order[ch] = ch;
        for (int i = 0; i < s.nch / 2; i++) d.dcoeff[i] = 0;
      }
      int seen = 0;
      for (int ch = 0; ch < s.nch; ch++) {
        if (d.order[ch] >= s.nch || (seen >> d.order[ch] & 1)) return "a channel order that isn't a permutation";
        seen |= 1 << d.order[ch];
      }
      d.highest = 0;
      for (int ch = 0; ch < s.nch; ch++) {
        d.aorder[ch] = r.get(4);
        if (d.aorder[ch] > d.highest) d.highest = d.aorder[ch];
      }
      if (d.highest > x.nsegsamples) return "a prediction order above the segment's samples";
      for (int ch = 0; ch < s.nch; ch++) d.forder[ch] = d.aorder[ch] ? 0 : r.get(2);
      for (int ch = 0; ch < s.nch; ch++)
        for (int j = 0; j < d.aorder[ch]; j++) {
          const int q = unzig(r.get(8));
          if (q == -128) return "a reflection coefficient out of range";
          d.rc[ch][j] = q >= 0 ? RC_TAB[q] : -(int)RC_TAB[-q];
        }
      d.dmix_embedded = s.dmix_embedded && (b == 0 || r.get(1));
      if ((b == 0 && x.scalable) || (b && r.get(1))) {
        d.lsb_size = (int)r.get(x.seg_nbits);
        if (d.lsb_size && (x.band_crc > 2 || (b == 0 && x.band_crc > 1))) d.lsb_size += 2;
        for (int ch = 0; ch < s.nch; ch++) {
          d.nlsb[ch] = r.get(4);
          if (d.nlsb[ch] && !d.lsb_size) return "LSBs without an LSB part";
        }
      }
      if ((b == 0 && x.scalable) || (b && r.get(1)))
        for (int ch = 0; ch < s.nch; ch++) d.adj[ch] = r.get(4);
    }
    if (r.over || r.pos > (U64)(off + hs - 2) * 8) return "a channel set header too short";
    s.nabits = s.storage_res > 16 ? 5 : s.storage_res > 8 ? 4 : 3;
    if ((x.nchsets > 1 || s.nbands > 1) && s.nabits < 5) s.nabits++;
    if (s.hier) hier += s.nch;
    x.chofs[c] = x.nch;
    x.nch += s.nch;
    off += hs;
  }
  x.chofs[x.nchsets] = x.nch;
  // NAVI
  r.init(p, (U64)off * 8, (U64)n * 8);
  x.navi.assign((size_t)x.nbands * x.nsegs * x.nchsets, 0);
  int* nv = &x.navi[0];
  for (int b = 0; b < x.nbands; b++)
    for (int s = 0; s < x.nsegs; s++)
      for (int c = 0; c < x.nchsets; c++) *nv++ = x.cs[c].nbands > b ? (int)((U64)r.get(x.seg_nbits) + 1) : 0;
  r.skip((8 - (r.pos & 7)) & 7);
  r.get(16);
  if (r.over) return "a NAVI table past the frame";
  x.data_start = (int)(r.pos >> 3);
  if (crc16(p + off, x.data_start - off)) return "a NAVI table whose CRC fails";
  return 0;
}

// the width of the LSB part a band's channel ch's MSBs are shifted by (8.6.1), and how many of its
// LSB bits overlap the MSBs (bit-width adjustment)
static int lsb_shift(const Xll& x, const Band& d, int ch) {
  if (x.fixed_lsb) return x.fixed_lsb;
  return d.nlsb[ch] && d.adj[ch] ? d.nlsb[ch] - 1 + d.adj[ch] : d.nlsb[ch] + d.adj[ch];
}
static int lsb_overlap(const Xll& x, const Band& d, int ch) {
  const int k = d.nlsb[ch] + d.adj[ch] - lsb_shift(x, d, ch);
  return d.nlsb[ch] && k > 0 ? k : 0;
}

// what the split takes: one or two bands, channel sets alike; 0 or why not
static const char* xll_supported(const Xll& x) {
  if (x.nbands > 2) return "XLL with four frequency bands";
  for (int c = 0; c < x.nchsets; c++) {
    const Chset& s = x.cs[c];
    if (s.freq != x.cs[0].freq || s.storage_res != x.cs[0].storage_res || s.nbands != x.nbands)
      return "XLL channel sets unlike each other";
    if (s.storage_res != 16 && s.storage_res != 20 && s.storage_res != 24) return "an XLL sample width not 16, 20 or 24";
    for (int b = 0; b < s.nbands; b++)
      for (int ch = 0; ch < s.nch; ch++) {
        const Band& d = s.band[b];
        const int sh = lsb_shift(x, d, ch);
        if (sh > 24 || d.adj[ch] > sh || lsb_overlap(x, d, ch) > d.nlsb[ch]) return "XLL LSBs out of range";
      }
  }
  if (x.nch > XWAV) return "too many XLL channels";
  return 0;
}

static bool rd_lin(BitReader& r, S32* v, int n, int bits) {
  if (!bits) {
    memset(v, 0, n * sizeof *v);
    return true;
  }
  for (int i = 0; i < n; i++) v[i] = unzig(r.get(bits));
  return !r.over;
}

static inline bool rd_rice1(BitReader& r, int k, S32& v) {
  const S64 q = r.unary();
  if (q < 0 || (U64)q >> (32 - k)) return false;
  v = unzig((U32)((U64)q << k) | r.get(k));
  return true;
}

static bool rd_rice(BitReader& r, S32* v, int n, int k) {
  for (int i = 0; i < n; i++)
    if (!rd_rice1(r, k, v[i])) return false;
  return !r.over;
}

// bits [a, b) of p as a gap: 0 where all are 0, else 1 and kept
static void gap_of(const U8* p, U64 a, U64 b, U8& mode, U32& nbits, std::vector<U8>& keep) {
  BitReader r;
  r.init(p, a, b);
  bool zero = true;
  for (U64 i = a; i < b && zero; i += 32) zero = !r.get(b - i < 32 ? (int)(b - i) : 32);
  mode = zero ? 0 : 1;
  nbits = (U32)(b - a);
  keep.clear();
  if (!zero) {
    BitWriter w;
    w.reset();
    w.copy(p, a, b - a);
    w.align();
    keep = w.v;
  }
}

// a frame's samples, by band: [band][channel][sample], channel sets one after the other, a set's
// channels as the band data has them
struct BandData {
  int N, nch;
  std::vector<S32> v[2];
  void init(int n, int ch) {
    N = n;
    nch = ch;
    for (int b = 0; b < 2; b++) v[b].assign((size_t)n * ch, 0);
  }
  S32* at(int b, int ch) { return &v[b][(size_t)ch * N]; }
  const S32* at(int b, int ch) const { return &v[b][(size_t)ch * N]; }
};

// the band data of XLL frame p: residuals and LSBs into res and lsb, the segments' coding into ck
// ([band][segment][channel set]), the end of the band data into data_end; 0 or why it can't be
// followed
static const char* xll_data(Xll& x, const U8* p, std::vector<Chunk>& ck, BandData& res, BandData& lsb, int& data_end) {
  const int S = x.nsegsamples;
  U64 at = (U64)x.data_start * 8;
  ck.assign((size_t)x.nbands * x.nsegs * x.nchsets, Chunk());
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++) {
        Chset& s = x.cs[c];
        const size_t ci = ((size_t)b * x.nsegs + seg) * x.nchsets + c;
        if (s.nbands <= b) continue;
        const Band& d = s.band[b];
        Chunk& k = ck[ci];
        const U64 cend = at + (U64)x.navi[ci] * 8;
        if (cend > (U64)x.frame_size * 8) return "band data past the frame";
        BitReader r;
        r.init(p, at, cend);
        k.reuse = seg ? r.get(1) : 0;
        if (!k.reuse) {
          s.seg_common = k.seg_common = r.get(1);
          const int np = s.seg_common ? 1 : s.nch;
          for (int i = 0; i < np; i++) {
            k.rice[i] = r.get(1);
            k.hyb[i] = !s.seg_common && k.rice[i] ? r.get(1) : 0;
            k.aux_code[i] = k.hyb[i] ? r.get(s.nabits) : 0;
            s.rice[i] = k.rice[i];
            s.aux[i] = k.hyb[i] ? k.aux_code[i] + 1 : 0;
          }
          for (int i = 0; i < np; i++) {
            if (!seg) {
              k.a_code[i] = r.get(s.nabits);
              s.abit_a[i] = !s.rice[i] && k.a_code[i] ? k.a_code[i] + 1 : k.a_code[i];
              s.na[i] = s.seg_common ? d.highest : d.aorder[i];
            } else {
              s.abit_a[i] = 0;
              s.na[i] = 0;
            }
            k.b_code[i] = r.get(s.nabits);
            s.abit_b[i] = !s.rice[i] && k.b_code[i] ? k.b_code[i] + 1 : k.b_code[i];
          }
        }
        k.seg_common = s.seg_common;
        for (int ch = 0; ch < s.nch; ch++) {
          const int i = s.seg_common ? 0 : ch;
          S32* pa = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
          const int na = s.na[i], nb = S - na;
          S32* pb = pa + na;
          if (!s.rice[i]) {
            if (!rd_lin(r, pa, na, s.abit_a[i]) || !rd_lin(r, pb, nb, s.abit_b[i])) return "band data cut short";
          } else {
            if (!rd_rice(r, pa, na, s.abit_a[i])) return "band data cut short";
            if (s.aux[i]) {
              const int niso = r.get(x.nseg_log2);
              U8 flag[XSEG];
              memset(flag, 0, sizeof flag);
              k.iso[ch].resize(niso);
              for (int j = 0; j < niso; j++) {
                const int loc = r.get(x.nseg_log2);
                if (loc >= nb) return "an isolated sample past its segment";
                k.iso[ch][j] = (U16)loc;
                flag[loc] = 1;
              }
              for (int j = 0; j < nb; j++) {
                if (flag[j])
                  pb[j] = unzig(r.get(s.aux[i]));
                else if (!rd_rice1(r, s.abit_b[i], pb[j]))
                  return "band data cut short";
              }
              if (r.over) return "band data cut short";
            } else if (!rd_rice(r, pb, nb, s.abit_b[i])) {
              return "band data cut short";
            }
          }
        }
        if (seg == 0 && b == 1) {  // the decimator history
          k.dh_code = r.get(5);
          for (int ch = 0; ch < s.nch; ch++) {
            k.dh[ch][0] = 0;
            for (int j = 1; j < XDH; j++) {
              const int nb = k.dh_code + 1;
              k.dh[ch][j] = nb == 32 ? (S32)r.get(32) : (S32)(r.get(nb) << (32 - nb)) >> (32 - nb);
            }
          }
          if (r.over) return "band data cut short";
        }
        const U64 lstart = cend - (U64)d.lsb_size * 8;
        if (d.lsb_size && lstart < at) return "an LSB part larger than its segment";
        if (r.pos > lstart) return "an MSB part that runs into the LSB part";
        gap_of(p, r.pos, lstart, k.g1, k.g1n, k.g1b);
        k.g2 = 0;
        k.g2n = 0;
        if (d.lsb_size) {
          BitReader q;
          q.init(p, lstart, cend);
          for (int ch = 0; ch < s.nch; ch++)
            if (d.nlsb[ch]) {
              S32* pl = lsb.at(b, x.chofs[c] + ch) + (size_t)seg * S;
              for (int j = 0; j < S; j++) pl[j] = (S32)q.get(d.nlsb[ch]);
            }
          if (q.over) return "an LSB part cut short";
          gap_of(p, q.pos, cend, k.g2, k.g2n, k.g2b);
        }
        at = cend;
      }
  data_end = (int)(at >> 3);
  return 0;
}

// the direct-form coefficients of the reflection coefficients (8.5.4; the sums wrap, as 32-bit
// ones do, where a damaged stream makes them overflow)
static void rc_to_lpc(const int* rc, int order, int* co) {
  for (int j = 0; j < order; j++) {
    const int r = rc[j];
    for (int k = 0; k < (j + 1) / 2; k++) {
      const int t1 = co[k], t2 = co[j - k - 1];
      co[k] = (int)((U32)t1 + (U32)mul16(r, t2));
      co[j - k - 1] = (int)((U32)t2 + (U32)mul16(r, t1));
    }
    co[j] = r;
  }
}

// a channel's prediction for sample n, from the o samples before it (8.5.4)
static inline S32 lpc_pred(const U32* b, int n, int o, const int* co) {
  U64 e = 0;  // (S64's sum, wrapping where a damaged stream makes it overflow)
  for (int k = 0; k < o; k++) e += (U64)((S64)(S32)b[n - o + k] * co[o - k - 1]);
  return sat23((S32)(((S64)e + (1 << 15)) >> 16));
}

static inline S32 decor_term(U32 base, int coeff) { return (S32)(((S64)(S32)base * coeff + 4) >> 3); }

// the two-band filter's lifting steps on band 0 (b0[-8..N-1], its decimator history first) and band
// 1 (b1[0..N-1]), forward (as the decoder does them) or undone (inv)
static void band_lift(U32* b0, U32* b1, int N, bool inv) {
  const S32* c = BAND_COEFF;
  if (!inv) {
    for (int s = 0; s < 4; s++) {
      U32* dst = s & 1 ? b1 : b0;
      const U32* src = s & 1 ? b0 : b1;
      for (int i = 0; i < N; i++) dst[i] -= (U32)mulq((S32)src[i], c[s], 22);
    }
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < N; j++) b0[j - i] -= (U32)mulq((S32)b1[j], c[i + 4], 23);
      for (int j = 0; j < N; j++) b1[j] -= (U32)mulq((S32)b0[j - i], c[i + 12], 23);
      for (int j = 0; j < N; j++) b0[j - i] -= (U32)mulq((S32)b1[j], c[i + 4], 23);
    }
  } else {
    for (int i = 7; i >= 0; i--) {
      for (int j = 0; j < N; j++) b0[j - i] += (U32)mulq((S32)b1[j], c[i + 4], 23);
      for (int j = 0; j < N; j++) b1[j] += (U32)mulq((S32)b0[j - i], c[i + 12], 23);
      for (int j = 0; j < N; j++) b0[j - i] += (U32)mulq((S32)b1[j], c[i + 4], 23);
    }
    for (int s = 3; s >= 0; s--) {
      U32* dst = s & 1 ? b1 : b0;
      const U32* src = s & 1 ? b0 : b1;
      for (int i = 0; i < N; i++) dst[i] += (U32)mulq((S32)src[i], c[s], 22);
    }
  }
}

// what a frame decodes to besides its WAV values, for the meta file
struct Extra {
  std::vector<S32> tail;  // two bands: band 0's last 7 lifted samples, [channel][7]
  BitWriter ovl;          // the LSB bits that overlap the MSBs, [band][channel][sample]
};

// The samples of the XLL frame (val: [channel][sample], a set's channels in output order) from its
// residuals and LSBs: in each band the prediction and the pairwise decorrelation undone and the MSBs
// and LSBs put together, then the bands put together
static void xll_decode(const Xll& x, const std::vector<Chunk>& ck, const BandData& res, const BandData& lsb,
                       S32* val, Extra& ex, std::vector<U32>& tmp) {
  const int N = x.nframesamples, NB = x.nbands;
  BandData out;
  out.init(N, x.nch);
  tmp.resize((size_t)XCH * N);
  ex.ovl.reset();
  for (int b = 0; b < NB; b++)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      const Band& d = s.band[b];
      for (int ch = 0; ch < s.nch; ch++) {
        U32* bb = &tmp[(size_t)ch * N];
        const S32* e = res.at(b, x.chofs[c] + ch);
        const int o = d.aorder[ch];
        if (o) {
          int co[XORD];
          rc_to_lpc(d.rc[ch], o, co);
          for (int n = 0; n < o && n < N; n++) bb[n] = (U32)e[n];
          for (int n = o; n < N; n++) bb[n] = (U32)e[n] - (U32)lpc_pred(bb, n, o, co);
        } else {
          for (int n = 0; n < N; n++) bb[n] = (U32)e[n];
          for (int j = 0; j < d.forder[ch]; j++)
            for (int n = 1; n < N; n++) bb[n] += bb[n - 1];
        }
      }
      if (d.decor)
        for (int i = 0; i < s.nch / 2; i++)
          if (d.dcoeff[i]) {
            const U32* b0 = &tmp[(size_t)(2 * i) * N];
            U32* b1 = &tmp[(size_t)(2 * i + 1) * N];
            for (int n = 0; n < N; n++) b1[n] += (U32)decor_term(b0[n], d.dcoeff[i]);
          }
      for (int i = 0; i < s.nch; i++) {
        const int oc = d.order[i];
        const U32* bb = &tmp[(size_t)i * N];
        S32* v = out.at(b, x.chofs[c] + oc);
        const int sh = lsb_shift(x, d, oc), a = d.adj[oc], k = lsb_overlap(x, d, oc);
        const S32* l = lsb.at(b, x.chofs[c] + oc);
        if (!sh)
          for (int n = 0; n < N; n++) v[n] = (S32)bb[n];
        else if (d.nlsb[oc])
          for (int n = 0; n < N; n++) {
            v[n] = (S32)((bb[n] << sh) + ((U32)l[n] << a));
            if (k) ex.ovl.put((U32)l[n] >> (d.nlsb[oc] - k), k);
          }
        else
          for (int n = 0; n < N; n++) v[n] = (S32)(bb[n] << sh);
      }
    }
  ex.ovl.align();
  if (NB == 1) {
    memcpy(val, &out.v[0][0], (size_t)N * x.nch * sizeof *val);
    ex.tail.clear();
    return;
  }
  ex.tail.assign((size_t)x.nch * 7, 0);
  std::vector<U32> w0(N + XDH), w1(N);
  for (int c = 0; c < x.nchsets; c++) {
    const Chunk& k = ck[((size_t)1 * x.nsegs + 0) * x.nchsets + c];
    for (int ch = 0; ch < x.cs[c].nch; ch++) {
      const int g = x.chofs[c] + ch;
      for (int j = 0; j < XDH; j++) w0[j] = (U32)k.dh[ch][j];
      for (int n = 0; n < N; n++) w0[XDH + n] = (U32)out.at(0, g)[n], w1[n] = (U32)out.at(1, g)[n];
      band_lift(&w0[XDH], &w1[0], N, false);
      S32* v = val + (size_t)g * 2 * N;
      for (int n = 0; n < N; n++) {
        v[2 * n] = (S32)w1[n];
        v[2 * n + 1] = (S32)w0[XDH + n - 7];
      }
      for (int j = 0; j < 7; j++) ex.tail[(size_t)g * 7 + j] = (S32)w0[XDH + N - 7 + j];
    }
  }
}

// The residuals and LSBs that decode to val (xll_decode's inverse); false where val doesn't, with
// the frame's header and what the meta file kept
static bool xll_encode_samples(const Xll& x, const std::vector<Chunk>& ck, const S32* val, const Extra& ex,
                               BandData& res, BandData& lsb, std::vector<U32>& tmp) {
  const int N = x.nframesamples, NB = x.nbands;
  BandData in;
  in.init(N, x.nch);
  if (NB == 1) {
    memcpy(&in.v[0][0], val, (size_t)N * x.nch * sizeof *val);
  } else {
    if (ex.tail.size() != (size_t)x.nch * 7) return false;
    std::vector<U32> w0(N + XDH), w1(N);
    for (int c = 0; c < x.nchsets; c++) {
      const Chunk& k = ck[((size_t)1 * x.nsegs + 0) * x.nchsets + c];
      for (int ch = 0; ch < x.cs[c].nch; ch++) {
        const int g = x.chofs[c] + ch;
        const S32* v = val + (size_t)g * 2 * N;
        w0[0] = (U32)k.dh[ch][0];
        for (int n = 0; n < N; n++) {
          w1[n] = (U32)v[2 * n];
          w0[XDH + n - 7] = (U32)v[2 * n + 1];
        }
        for (int j = 0; j < 7; j++) w0[XDH + N - 7 + j] = (U32)ex.tail[(size_t)g * 7 + j];
        band_lift(&w0[XDH], &w1[0], N, true);
        for (int j = 1; j < XDH; j++)
          if (w0[j] != (U32)k.dh[ch][j]) return false;
        for (int n = 0; n < N; n++) in.at(0, g)[n] = (S32)w0[XDH + n], in.at(1, g)[n] = (S32)w1[n];
      }
    }
  }
  tmp.resize((size_t)2 * XCH * N);
  std::vector<U8> ovb(ex.ovl.v);  // the overlapping LSB bits, as xll_decode wrote them
  ovb.resize(ovb.size() + PAD);
  BitReader ov;
  ov.init(&ovb[0], 0, (U64)ex.ovl.v.size() * 8);
  for (int b = 0; b < NB; b++)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      const Band& d = s.band[b];
      for (int i = 0; i < s.nch; i++) {
        const int oc = d.order[i];
        U32* bb = &tmp[(size_t)i * N];
        const S32* v = in.at(b, x.chofs[c] + oc);
        const int sh = lsb_shift(x, d, oc), a = d.adj[oc], nl = d.nlsb[oc], k = lsb_overlap(x, d, oc);
        S32* l = lsb.at(b, x.chofs[c] + oc);
        if (!sh) {
          for (int n = 0; n < N; n++) bb[n] = (U32)v[n];
        } else {
          const U32 m = (1u << sh) - 1, am = (1u << a) - 1;
          for (int n = 0; n < N; n++) {
            const U32 u = (U32)v[n], rem = u & m;
            const U32 hi = k ? ov.get(k) : 0;
            bb[n] = (U32)((S32)u >> sh) - hi;
            if (nl) {
              const U32 lo = rem >> a;
              if (rem & am || (lo >> (nl - k))) return false;
              l[n] = (S32)(lo + (hi << (nl - k)));
            } else if (rem) {
              return false;
            }
          }
        }
      }
      if (d.decor)
        for (int i = 0; i < s.nch / 2; i++)
          if (d.dcoeff[i]) {
            const U32* b0 = &tmp[(size_t)(2 * i) * N];
            U32* b1 = &tmp[(size_t)(2 * i + 1) * N];
            for (int n = 0; n < N; n++) b1[n] -= (U32)decor_term(b0[n], d.dcoeff[i]);
          }
      for (int ch = 0; ch < s.nch; ch++) {
        const U32* bb = &tmp[(size_t)ch * N];
        S32* e = res.at(b, x.chofs[c] + ch);
        const int o = d.aorder[ch];
        if (o) {
          int co[XORD];
          rc_to_lpc(d.rc[ch], o, co);
          for (int n = 0; n < o && n < N; n++) e[n] = (S32)bb[n];
          for (int n = o; n < N; n++) e[n] = (S32)(bb[n] + (U32)lpc_pred(bb, n, o, co));
        } else {
          U32* dd = &tmp[(size_t)(XCH + ch) * N];
          memcpy(dd, bb, (size_t)N * sizeof *dd);
          for (int j = 0; j < d.forder[ch]; j++)
            for (int n = N - 1; n > 0; n--) dd[n] -= dd[n - 1];
          for (int n = 0; n < N; n++) e[n] = (S32)dd[n];
        }
      }
    }
  return !ov.over && ov.end - ov.pos < 8;  // (all the overlapping bits taken, but a byte's padding)
}

static inline bool wr_lin(BitWriter& w, const S32* v, int n, int bits) {
  if (!bits) {
    for (int i = 0; i < n; i++)
      if (v[i]) return false;
    return true;
  }
  for (int i = 0; i < n; i++) {
    const U32 u = zig(v[i]);
    if (bits < 32 && u >> bits) return false;
    w.put(u, bits);
  }
  return true;
}

// a Rice code, where it ends by lim (the bits its segment has: a damaged WAV can make a residual
// whose unary part would run to gigabytes)
static inline bool wr_rice1(BitWriter& w, S32 v, int k, U64 lim) {
  const U32 u = zig(v);
  const U64 q = u >> k;
  if (w.bits() + q + 1 + k > lim) return false;
  w.zeros(q);
  w.put(1, 1);
  w.put(u, k);
  return true;
}

// x's band data from its residuals and LSBs, coded as ck says; 0 or why it can't be
static const char* xll_write_data(Xll& x, const std::vector<Chunk>& ck, const BandData& res, const BandData& lsb,
                                  BitWriter& w) {
  const int S = x.nsegsamples;
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++) {
        Chset& s = x.cs[c];
        const size_t ci = ((size_t)b * x.nsegs + seg) * x.nchsets + c;
        if (s.nbands <= b) continue;
        const Band& d = s.band[b];
        const Chunk& k = ck[ci];
        const U64 start = w.bits(), cend = start + (U64)x.navi[ci] * 8;
        const U64 lstart = cend - (U64)d.lsb_size * 8;
        if (seg) w.put(k.reuse, 1);
        if (!k.reuse) {
          s.seg_common = k.seg_common;
          w.put(s.seg_common, 1);
          const int np = s.seg_common ? 1 : s.nch;
          for (int i = 0; i < np; i++) {
            w.put(k.rice[i], 1);
            if (!s.seg_common && k.rice[i]) {
              w.put(k.hyb[i], 1);
              if (k.hyb[i]) w.put(k.aux_code[i], s.nabits);
            }
            s.rice[i] = k.rice[i];
            s.aux[i] = !s.seg_common && k.rice[i] && k.hyb[i] ? k.aux_code[i] + 1 : 0;
          }
          for (int i = 0; i < np; i++) {
            if (!seg) {
              w.put(k.a_code[i], s.nabits);
              s.abit_a[i] = !s.rice[i] && k.a_code[i] ? k.a_code[i] + 1 : k.a_code[i];
              s.na[i] = s.seg_common ? d.highest : d.aorder[i];
            } else {
              s.abit_a[i] = 0;
              s.na[i] = 0;
            }
            w.put(k.b_code[i], s.nabits);
            s.abit_b[i] = !s.rice[i] && k.b_code[i] ? k.b_code[i] + 1 : k.b_code[i];
          }
        }
        for (int ch = 0; ch < s.nch; ch++) {
          const int i = s.seg_common ? 0 : ch;
          const S32* pa = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
          const int na = s.na[i], nb = S - na;
          const S32* pb = pa + na;
          if (!s.rice[i]) {
            if (!wr_lin(w, pa, na, s.abit_a[i]) || !wr_lin(w, pb, nb, s.abit_b[i])) return "a residual its code can't hold";
          } else {
            for (int j = 0; j < na; j++)
              if (!wr_rice1(w, pa[j], s.abit_a[i], lstart)) return "a residual longer than its segment";
            if (s.aux[i]) {
              const std::vector<U16>& iso = k.iso[ch];
              if (iso.size() >> x.nseg_log2) return "too many isolated samples";
              U8 flag[XSEG];
              memset(flag, 0, sizeof flag);
              w.put((U32)iso.size(), x.nseg_log2);
              for (size_t j = 0; j < iso.size(); j++) {
                if (iso[j] >= nb) return "an isolated sample past its segment";
                w.put(iso[j], x.nseg_log2);
                flag[iso[j]] = 1;
              }
              for (int j = 0; j < nb; j++) {
                if (flag[j]) {
                  const U32 u = zig(pb[j]);
                  if (s.aux[i] < 32 && u >> s.aux[i]) return "an isolated sample its code can't hold";
                  w.put(u, s.aux[i]);
                } else if (!wr_rice1(w, pb[j], s.abit_b[i], lstart)) {
                  return "a residual longer than its segment";
                }
              }
            } else {
              for (int j = 0; j < nb; j++)
                if (!wr_rice1(w, pb[j], s.abit_b[i], lstart)) return "a residual longer than its segment";
            }
          }
        }
        if (seg == 0 && b == 1) {
          w.put(k.dh_code, 5);
          const int nbits = k.dh_code + 1;
          for (int ch = 0; ch < s.nch; ch++)
            for (int j = 1; j < XDH; j++) {
              const S32 v = k.dh[ch][j];
              if (nbits < 32 && (v < -(1 << (nbits - 1)) || v >= 1 << (nbits - 1))) return "a decimator sample its width can't hold";
              w.put((U32)v, nbits);
            }
        }
        if (w.bits() > lstart) return "an MSB part longer than its place";
        if (k.g1) {
          if (k.g1n != lstart - w.bits()) return "an MSB part of another length";
          w.copy(&k.g1b[0], 0, k.g1n);
        } else {
          w.zeros(lstart - w.bits());
        }
        if (d.lsb_size) {
          for (int ch = 0; ch < s.nch; ch++)
            if (d.nlsb[ch]) {
              const S32* pl = lsb.at(b, x.chofs[c] + ch) + (size_t)seg * S;
              for (int j = 0; j < S; j++) {
                if ((U32)pl[j] >> d.nlsb[ch]) return "an LSB its width can't hold";
                w.put((U32)pl[j], d.nlsb[ch]);
              }
            }
          if (w.bits() > cend) return "an LSB part longer than its place";
          if (k.g2) {
            if (k.g2n != cend - w.bits()) return "an LSB part of another length";
            w.copy(&k.g2b[0], 0, k.g2n);
          } else {
            w.zeros(cend - w.bits());
          }
        }
      }
  return 0;
}

// ---------------------------------------------------------------------------------------------
// a WAV's layout: which XLL channel each WAV channel is

// a DTS speaker position's WAV speaker bit (0: none)
static const U32 SPK_WAV[32] = {
    0x4,     0x1,    0x2,    0x200, 0x400, 0x8,     0x100,  0x10,    // C L R Ls Rs LFE1 Cs Lsr
    0x20,    0x200,  0x400,  0x40,  0x80,  0x1000,  0x2000, 0x4000,  // Rsr Lss Rss Lc Rc Lh Ch Rh
    0,       0,      0,      0x800, 0,     0,       0x10000, 0x8000,  // LFE2 Lw Rw Oh Lhs Rhs Chr Lhr
    0x20000, 0,      0,      0,     0,     0,       0,      0};      // Rhr Cl Ll Rl

struct Layout {
  int nch, bits, rate, nbands;
  U32 mask;
  int perm[XWAV];  // the XLL channel (chofs[c] + output channel) of each WAV channel
  int nchsets, cs_nch[XCS], cs_mask[XCS];
};

static void layout_of(const Xll& x, Layout& L) {
  L.nch = x.nch;
  L.bits = x.cs[0].storage_res > 16 ? 24 : 16;
  for (int c = 0; c < x.nchsets; c++)
    if (x.cs[c].dmix_embedded) L.bits = 32;
  L.rate = x.cs[0].freq;
  L.nbands = x.nbands;
  L.nchsets = x.nchsets;
  int wbit[XWAV];
  U32 used = 0;
  for (int c = 0; c < x.nchsets; c++) {
    const Chset& s = x.cs[c];
    L.cs_nch[c] = s.nch;
    L.cs_mask[c] = s.chmask;
    const U32 cm = (U32)s.chmask;
    const bool m = __builtin_popcount(cm) == s.nch;
    for (int ch = 0, bit = 0; ch < s.nch; ch++) {
      U32 wb = 0;
      if (m) {
        while (!(cm >> bit & 1)) bit++;
        wb = SPK_WAV[bit++];
      }
      if (wb & used) wb = 0;
      used |= wb;
      wbit[x.chofs[c] + ch] = wb ? __builtin_ctz(wb) : 32 + x.chofs[c] + ch;
    }
  }
  L.mask = used;
  for (int i = 0; i < x.nch; i++) L.perm[i] = i;
  for (int i = 1; i < x.nch; i++)  // by WAV bit; channels without one last, in stream order
    for (int j = i; j > 0 && wbit[L.perm[j - 1]] > wbit[L.perm[j]]; j--) {
      const int t = L.perm[j];
      L.perm[j] = L.perm[j - 1];
      L.perm[j - 1] = t;
    }
}

static bool layout_same(const Layout& a, const Layout& b) {
  if (a.nch != b.nch || a.bits != b.bits || a.rate != b.rate || a.nbands != b.nbands || a.nchsets != b.nchsets)
    return false;
  for (int c = 0; c < a.nchsets; c++)
    if (a.cs_nch[c] != b.cs_nch[c] || a.cs_mask[c] != b.cs_mask[c]) return false;
  return true;
}

static void put_layout(Rec& o, int w, const Layout& L) {
  o.u8('W');
  o.uv(w);
  o.uv(L.nch);
  o.uv(L.bits);
  o.uv(L.rate);
  o.uv(L.nbands);
  o.uv(L.mask);
  for (int i = 0; i < L.nch; i++) o.uv(L.perm[i]);
  o.uv(L.nchsets);
  for (int c = 0; c < L.nchsets; c++) {
    o.uv(L.cs_nch[c]);
    o.uv((U32)L.cs_mask[c]);
  }
}

static inline bool get_layout(RecIn& in, Layout& L) {
  L.nch = (int)in.uv();
  L.bits = (int)in.uv();
  L.rate = (int)in.uv();
  L.nbands = (int)in.uv();
  L.mask = (U32)in.uv();
  if (L.nch < 1 || L.nch > XWAV || (L.bits != 16 && L.bits != 24 && L.bits != 32) || L.nbands < 1 || L.nbands > 2)
    return false;
  int seen[XWAV] = {0};
  for (int i = 0; i < L.nch; i++) {
    L.perm[i] = (int)in.uv();
    if (L.perm[i] < 0 || L.perm[i] >= L.nch || seen[L.perm[i]]++) return false;
  }
  L.nchsets = (int)in.uv();
  if (L.nchsets < 1 || L.nchsets > XCS) return false;
  int n = 0;
  for (int c = 0; c < L.nchsets; c++) {
    L.cs_nch[c] = (int)in.uv();
    L.cs_mask[c] = (int)in.uv();
    n += L.cs_nch[c];
  }
  return !in.bad && n == L.nch;
}

// ---------------------------------------------------------------------------------------------
// an X record: an extension substream frame with its XLL frame split

static void put_chunk(Rec& o, Xll& x, int b, int seg, int c, const Chunk& k) {
  Chset& s = x.cs[c];
  o.u8(k.reuse | (k.reuse ? 0 : k.seg_common << 1) | k.g1 << 2 | k.g2 << 3);
  if (!k.reuse) {
    s.seg_common = k.seg_common;
    const int np = s.seg_common ? 1 : s.nch;
    for (int i = 0; i < np; i++) {
      o.u8(k.rice[i] | k.hyb[i] << 1);
      if (k.hyb[i]) o.u8(k.aux_code[i]);
      if (!seg) o.u8(k.a_code[i]);
      o.u8(k.b_code[i]);
      s.rice[i] = k.rice[i];
      s.aux[i] = k.hyb[i];
    }
  }
  for (int ch = 0; ch < s.nch; ch++) {
    const int i = s.seg_common ? 0 : ch;
    if (s.rice[i] && s.aux[i]) {
      o.uv(k.iso[ch].size());
      for (size_t j = 0; j < k.iso[ch].size(); j++) o.uv(k.iso[ch][j]);
    }
  }
  if (seg == 0 && b == 1) {
    o.u8(k.dh_code);
    for (int ch = 0; ch < s.nch; ch++)
      for (int j = 1; j < XDH; j++) o.sv(k.dh[ch][j]);
  }
  if (k.g1) o.blob(&k.g1b[0], k.g1b.size()), o.uv(k.g1n);
  if (k.g2) o.blob(&k.g2b[0], k.g2b.size()), o.uv(k.g2n);
}

static bool get_chunk(RecIn& in, Xll& x, int b, int seg, int c, Chunk& k) {
  Chset& s = x.cs[c];
  const int f = in.u8();
  if (f >> 4 || (seg == 0 && (f & 1))) return false;
  k.reuse = f & 1;
  k.g1 = f >> 2 & 1;
  k.g2 = f >> 3 & 1;
  if (!k.reuse) {
    s.seg_common = k.seg_common = f >> 1 & 1;
    const int np = s.seg_common ? 1 : s.nch;
    for (int i = 0; i < np; i++) {
      const int v = in.u8();
      k.rice[i] = v & 1;
      k.hyb[i] = v >> 1 & 1;
      if (v >> 2 || (k.hyb[i] && (s.seg_common || !k.rice[i]))) return false;
      k.aux_code[i] = k.hyb[i] ? (U8)in.u8() : 0;
      k.a_code[i] = !seg ? (U8)in.u8() : 0;
      k.b_code[i] = (U8)in.u8();
      if (k.aux_code[i] >> s.nabits || k.a_code[i] >> s.nabits || k.b_code[i] >> s.nabits) return false;
      s.rice[i] = k.rice[i];
      s.aux[i] = k.hyb[i];
    }
  } else {
    k.seg_common = s.seg_common;
  }
  for (int ch = 0; ch < s.nch; ch++) {
    const int i = s.seg_common ? 0 : ch;
    k.iso[ch].clear();
    if (s.rice[i] && s.aux[i]) {
      const U64 n = in.uv();
      if (n >> x.nseg_log2) return false;
      k.iso[ch].resize(n);
      for (U64 j = 0; j < n; j++) k.iso[ch][j] = (U16)in.uv();
    }
  }
  if (seg == 0 && b == 1) {
    k.dh_code = in.u8();
    if (k.dh_code > 31) return false;
    for (int ch = 0; ch < s.nch; ch++) {
      k.dh[ch][0] = 0;
      for (int j = 1; j < XDH; j++) {
        const S64 v = in.sv();
        if (v != (S32)v) return false;
        k.dh[ch][j] = (S32)v;
      }
    }
  }
  if (k.g1) {
    if (!in.blob(k.g1b, XFRAME)) return false;
    k.g1n = (U32)in.uv();
    k.g1b.resize(k.g1b.size() + PAD);
    if ((U64)k.g1n > (U64)(k.g1b.size() - PAD) * 8) return false;
  }
  if (k.g2) {
    if (!in.blob(k.g2b, XFRAME)) return false;
    k.g2n = (U32)in.uv();
    k.g2b.resize(k.g2b.size() + PAD);
    if ((U64)k.g2n > (U64)(k.g2b.size() - PAD) * 8) return false;
  }
  return !in.bad;
}

// the parts of an extension substream frame that the X record keeps, and what its XLL frame decodes to
struct Split {
  Xll x;
  std::vector<Chunk> ck;
  BandData res, lsb;
  std::vector<S32> val;
  Extra ex;
  std::vector<U32> tmp;
  int data_end;
};

// the clipped values of the frame's samples, in WAV order (count: the frame's samples * nch)
static void wav_values(int NS, const Layout& L, const S32* val, S32* out, std::vector<std::pair<U64, S32> >& exc) {
  const int W = L.nch;
  const S32 lo = (S32)(-(1ll << (L.bits - 1))), hi = (S32)((1ll << (L.bits - 1)) - 1);
  exc.clear();
  for (int n = 0; n < NS; n++)
    for (int w = 0; w < W; w++) {
      const S32 v = val[(size_t)L.perm[w] * NS + n];
      const S32 c = v < lo ? lo : v > hi ? hi : v;
      if (c != v) exc.push_back(std::make_pair((U64)n * W + w, v));
      out[(size_t)n * W + w] = c;
    }
}

static void put_x(Rec& o, int wi, const U8* e, int n, int xoff, const U8* xf, int one2one, Split& sp,
                  const std::vector<std::pair<U64, S32> >& exc) {
  Xll& x = sp.x;
  o.u8('X');
  o.uv(wi);
  o.blob(e, xoff);
  o.u8(one2one);
  o.uv(x.frame_size);
  o.blob(xf, x.data_start);
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++)
        if (x.cs[c].nbands > b) put_chunk(o, x, b, seg, c, sp.ck[((size_t)b * x.nsegs + seg) * x.nchsets + c]);
  o.blob(xf + sp.data_end, x.frame_size - sp.data_end);
  o.blob(e + xoff + x.frame_size, n - xoff - x.frame_size);
  for (size_t i = 0; i < sp.ex.tail.size(); i++) o.sv(sp.ex.tail[i]);
  o.blob(sp.ex.ovl.v.empty() ? (const U8*)"" : &sp.ex.ovl.v[0], sp.ex.ovl.v.size());
  o.uv(exc.size());
  U64 last = 0;
  for (size_t i = 0; i < exc.size(); i++) {
    o.uv(exc[i].first - last);
    o.sv(exc[i].second);
    last = exc[i].first;
  }
}

// where the join takes an X record's samples from
struct Samples {
  virtual bool get(S32* v, size_t count) = 0;
  virtual ~Samples() {}
};

// An X record (its tag and WAV index read) joined into out: 0, or why it can't be. L: the WAV's layout.
static const char* join_x(RecIn& in, const Layout* L, Samples& smp, std::vector<U8>& out, Split& sp,
                          std::vector<S32>& wv) {
  std::vector<U8> pre, hdr, tail, post;
  if (!in.blob(pre, 1 << 20)) return "a damaged record";
  const int one2one = in.u8();
  const U64 fsize = in.uv();
  if (in.bad || one2one > 1 || fsize > (1 << 20) || !in.blob(hdr, fsize)) return "a damaged record";
  Xll& x = sp.x;
  std::vector<U8> fb(fsize + PAD, 0);  // the header, as the frame's start: what xll_headers reads it from
  memcpy(&fb[0], &hdr[0], hdr.size());
  const char* why = xll_headers(x, &fb[0], (int)fsize, one2one);
  if (why) return why;
  if ((size_t)x.data_start != hdr.size() || (U64)x.frame_size != fsize) return "a damaged record";
  if ((why = xll_supported(x))) return why;
  Layout fl;
  layout_of(x, fl);
  if (!L || !layout_same(*L, fl)) return "an XLL frame unlike its WAV";
  sp.ck.assign((size_t)x.nbands * x.nsegs * x.nchsets, Chunk());
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++)
        if (x.cs[c].nbands > b && !get_chunk(in, x, b, seg, c, sp.ck[((size_t)b * x.nsegs + seg) * x.nchsets + c]))
          return "a damaged record";
  if (!in.blob(tail, fsize) || !in.blob(post, 1 << 20)) return "a damaged record";
  sp.ex.tail.assign(x.nbands > 1 ? (size_t)x.nch * 7 : 0, 0);
  for (size_t i = 0; i < sp.ex.tail.size(); i++) {
    const S64 v = in.sv();
    if (v != (S32)v) return "a damaged record";
    sp.ex.tail[i] = (S32)v;
  }
  std::vector<U8> ov;
  if (!in.blob(ov, 1 << 26)) return "a damaged record";
  sp.ex.ovl.reset();
  sp.ex.ovl.v = ov;
  const int NS = x.nframesamples * x.nbands, W = L->nch;
  wv.resize((size_t)NS * W);
  if (!smp.get(&wv[0], wv.size())) return "the WAV ends early";
  const U64 nexc = in.uv();
  if (in.bad || nexc > wv.size()) return "a damaged record";
  U64 at = 0;
  for (U64 i = 0; i < nexc; i++) {
    at += in.uv();
    const S64 v = in.sv();
    if (in.bad || at >= wv.size() || v != (S32)v) return "a damaged record";
    wv[at] = (S32)v;
  }
  sp.val.resize((size_t)NS * W);
  for (int n = 0; n < NS; n++)
    for (int w = 0; w < W; w++) sp.val[(size_t)L->perm[w] * NS + n] = wv[(size_t)n * W + w];
  sp.res.init(x.nframesamples, x.nch);
  sp.lsb.init(x.nframesamples, x.nch);
  if (!xll_encode_samples(x, sp.ck, &sp.val[0], sp.ex, sp.res, sp.lsb, sp.tmp)) return "WAV samples the XLL frame can't code";
  BitWriter w;
  w.reset();
  w.bytes(&hdr[0], hdr.size());
  if ((why = xll_write_data(x, sp.ck, sp.res, sp.lsb, w))) return why;
  w.bytes(&tail[0], tail.size());
  if (w.v.size() != fsize || w.nacc) return "an XLL frame of another size";
  out.clear();
  out.insert(out.end(), pre.begin(), pre.end());
  out.insert(out.end(), w.v.begin(), w.v.end());
  out.insert(out.end(), post.begin(), post.end());
  return 0;
}

struct MemSamples : Samples {
  const S32* v;
  size_t n;
  bool get(S32* out, size_t count) {
    if (count > n) return false;
    memcpy(out, v, count * sizeof *out);
    v += count;
    n -= count;
    return true;
  }
};

struct WavSamples : Samples {
  WavIn* w;
  bool get(S32* out, size_t count) { return w->read(out, count); }
};

// ---------------------------------------------------------------------------------------------
// the split

struct Stats {
  U64 bytes, raw, core, exss, xsplit, exceptions, ovl_bits;
  U64 m_exss, m_xhdr, m_param, m_kept, m_raw;  // the meta file: EXSS headers, XLL headers, the rest of
                                               // X records, frames kept as bytes, bytes between frames
  std::vector<std::string> why;
  std::vector<U64> why_n;
  void kept(const char* w) {
    for (size_t i = 0; i < why.size(); i++)
      if (why[i] == w) {
        why_n[i]++;
        return;
      }
    why.push_back(w);
    why_n.push_back(1);
  }
};

struct FileOut {
  FILE* f;
  std::string name;
  U64 size;
  void open(const std::string& path) {
    name = path;
    size = 0;
    f = fopen(path.c_str(), "wb");
    if (!f) die(EX_WRITE, "can't create %s", path.c_str());
  }
  void write(const U8* p, size_t n) {
    if (n && fwrite(p, 1, n, f) != n) die(EX_WRITE, "can't write %s", name.c_str());
    size += n;
  }
  void write(const Rec& r) { write(r.b.empty() ? 0 : &r.b[0], r.b.size()); }
  void close() {
    if (fclose(f)) die(EX_WRITE, "can't write %s", name.c_str());
  }
};

static const U8 MAGIC[8] = {'D', 'T', 'S', 'S', 'P', 'L', 'I', 'T'};
enum { VERSION = 2 };

static std::string wav_name(const std::string& prefix, int w) {
  if (!w) return prefix + ".wav";
  char b[32];
  snprintf(b, sizeof b, ".%d.wav", w + 1);
  return prefix + b;
}

// the input, read through a window that holds the largest frame
struct Input {
  FILE* f;
  std::vector<U8> buf;
  size_t lo, hi;
  bool eof;
  void init(FILE* fp) {
    f = fp;
    buf.resize((4 << 20) + PAD);
    lo = hi = 0;
    eof = false;
  }
  size_t fill(size_t need) {
    if (hi - lo < need && !eof) {
      if (lo) {
        memmove(&buf[0], &buf[lo], hi - lo);
        hi -= lo;
        lo = 0;
      }
      while (hi < buf.size() - PAD && !eof) {
        const size_t k = fread(&buf[hi], 1, buf.size() - PAD - hi, f);
        if (!k) {
          if (ferror(f)) die(EX_READ, "can't read the input");
          eof = true;
        }
        hi += k;
      }
      memset(&buf[hi], 0, PAD);
    }
    return hi - lo;
  }
};

enum { LOOK = (1 << 20) + 64, RAW_MAX = 1 << 20 };

struct Splitter {
  std::string prefix;
  FileOut dts, meta;
  WavOut wav[XNWAV];
  Layout lay[XNWAV];
  int nwav;
  ExssState st;
  Stats stt;
  Split sp, chk;
  std::vector<S32> wv, wv2;
  static CoreFrame cf;

  // the WAV of an XLL frame's layout (a new one where it is new): -1 where there are too many
  int wav_of(const Layout& fl) {
    for (int w = 0; w < nwav; w++)
      if (layout_same(lay[w], fl)) return w;
    return nwav < XNWAV ? nwav : -1;
  }

  // the XLL frame of the EXSS frame e (n bytes) at off split, its record into o: 0, or why not
  const char* try_split(const U8* e, int n, int off, int size, int one2one, Rec& o, int& wi) {
    std::vector<U8> xf(size + PAD, 0);
    memcpy(&xf[0], e + off, size);
    Xll& x = sp.x;
    const char* why = xll_headers(x, &xf[0], size, one2one);
    if (why || (why = xll_supported(x))) return why;
    Layout fl;
    layout_of(x, fl);
    if ((wi = wav_of(fl)) < 0) return "more channel layouts than WAVs";
    sp.res.init(x.nframesamples, x.nch);
    sp.lsb.init(x.nframesamples, x.nch);
    const int NS = x.nframesamples * x.nbands;
    sp.val.assign((size_t)NS * x.nch, 0);
    Xll hx = x;  // (the band data parse changes the coding state)
    if ((why = xll_data(x, &xf[0], sp.ck, sp.res, sp.lsb, sp.data_end))) return why;
    x = hx;
    xll_decode(x, sp.ck, sp.res, sp.lsb, &sp.val[0], sp.ex, sp.tmp);
    std::vector<std::pair<U64, S32> > exc;
    wv.resize((size_t)NS * x.nch);
    wav_values(NS, fl, &sp.val[0], &wv[0], exc);
    o.clear();
    put_x(o, wi, e, n, off, &xf[0], one2one, sp, exc);
    // the check: the record joined back as the join will
    RecIn in;
    in.init_mem(&o.b[0], o.b.size());
    in.u8();
    in.uv();
    MemSamples ms;
    ms.v = &wv[0];
    ms.n = wv.size();
    std::vector<U8> back;
    if ((why = join_x(in, &fl, ms, back, chk, wv2))) return why;
    if (back.size() != (size_t)n || memcmp(&back[0], e, n) || in.mpos != in.msize)
      return "an XLL frame that doesn't join back the same";
    if (wi == nwav) {
      lay[nwav] = fl;
      wav[nwav].open(wav_name(prefix, nwav));
      wav[nwav].nch = fl.nch;
      wav[nwav].bits = fl.bits;
      wav[nwav].rate = fl.rate;
      wav[nwav].mask = fl.mask;
      Rec lr;
      put_layout(lr, nwav, fl);
      meta.write(lr);
      nwav++;
    }
    wav[wi].write(&wv[0], wv.size());
    stt.exceptions += exc.size();
    stt.ovl_bits += sp.ex.ovl.v.size() * 8;
    stt.m_exss += n - x.frame_size;
    stt.m_xhdr += x.data_start + (x.frame_size - sp.data_end);
    stt.m_param += o.b.size() - (n - x.frame_size) - x.data_start - (x.frame_size - sp.data_end);
    return 0;
  }

  void exss(const U8* e, int n) {
    int off = 0, size = 0, one2one = 0, wi = 0;
    const char* why = 0;
    stt.exss++;
    Rec o;
    if (exss_xll(st, e, n, off, size, one2one, why)) {
      why = 0;
      for (int t = one2one < 0 ? 1 : one2one; t >= (one2one < 0 ? 0 : one2one); t--)  // (unknown: 1, then 0)
        if (!(why = try_split(e, n, off, size, t, o, wi))) break;
      if (!why) {
        meta.write(o);
        stt.xsplit++;
        return;
      }
    }
    stt.kept(why ? why : "?");
    o.clear();
    o.u8('E');
    o.blob(e, n);
    meta.write(o);
    stt.m_kept += o.b.size();
  }

  void bytes(const U8* p, size_t n) {
    Rec o;
    o.u8('R');
    o.blob(p, n);
    meta.write(o);
    stt.m_raw += o.b.size();
  }

  // a core frame at p (n bytes, av there): one where a frame or the end follows, or it parses
  bool core_ok(const U8* p, int n, size_t av, bool eof) {
    if ((size_t)n == av && eof) return true;
    if ((size_t)n + 4 <= av && (be32(p + n) == SYNC_CORE || be32(p + n) == SYNC_EXSS)) return true;
    return !core_parse(p, n, cf);
  }

  void run(FILE* fi, U32& crc) {
    dts.open(prefix + ".dtsc");
    meta.open(prefix + ".meta");
    nwav = 0;
    Rec hr;
    hr.bytes(MAGIC, 8);
    hr.u8(VERSION);
    meta.write(hr);
    Input in;
    in.init(fi);
    memset(&st, 0, sizeof st);
    std::vector<U8> raw;
    crc = 0;
    for (;;) {
      const size_t av = in.fill(LOOK);
      if (!av) break;
      const U8* p = &in.buf[in.lo];
      int n = core_frame(p, av);
      if (n && core_ok(p, n, av, in.eof)) {
        if (raw.size()) bytes(&raw[0], raw.size()), raw.clear();
        Rec o;
        o.u8('C');
        o.uv(n);
        meta.write(o);
        dts.write(p, n);
        stt.core++;
      } else if ((n = exss_frame(p, av))) {
        if (raw.size()) bytes(&raw[0], raw.size()), raw.clear();
        exss(p, n);
      } else {
        // bytes up to the next byte that could start a sync word
        n = 1;
        const size_t lim = av < 65536 ? av : 65536;
        while ((size_t)n < lim && p[n] != 0x7F && p[n] != 0x64) n++;
        raw.insert(raw.end(), p, p + n);
        stt.raw += n;
        if (raw.size() >= RAW_MAX) bytes(&raw[0], raw.size()), raw.clear();
      }
      crc = crc32_upd(crc, p, n);
      stt.bytes += n;
      in.lo += n;
    }
    if (raw.size()) bytes(&raw[0], raw.size());
    if (!nwav) {  // (a stream without XLL: an empty WAV all the same)
      wav[0].open(wav_name(prefix, 0));
      nwav = 1;
      Layout z;
      memset(&z, 0, sizeof z);
      lay[0] = z;
    }
    Rec zr;
    zr.u8('Z');
    zr.uv(stt.bytes);
    zr.uv(crc);
    zr.uv(nwav);
    for (int w = 0; w < nwav; w++) zr.uv(wav[w].frames);
    meta.write(zr);
    for (int w = 0; w < nwav; w++) wav[w].finish();
    dts.close();
    meta.close();
  }
};
CoreFrame Splitter::cf;

// ---------------------------------------------------------------------------------------------
#ifndef DTSSPLIT_LIB  // (dtscore.cpp takes what comes before)
// the join

struct Sink {
  virtual void write(const U8* p, size_t n) = 0;
  virtual ~Sink() {}
};

struct FileSink : Sink {
  FILE* f;
  void write(const U8* p, size_t n) {
    if (n && fwrite(p, 1, n, f) != n) die(EX_WRITE, "can't write the output");
  }
};

// the join's output compared with a file (-v)
struct CmpSink : Sink {
  FILE* f;
  U64 at;
  bool diff;
  std::vector<U8> buf;
  void write(const U8* p, size_t n) {
    if (diff) return;
    buf.resize(n);
    const size_t k = n ? fread(&buf[0], 1, n, f) : 0;
    if (k != n || memcmp(&buf[0], p, n)) {
      for (size_t i = 0; i < k; i++)
        if (buf[i] != p[i]) {
          at += i;
          break;
        }
      diff = true;
      return;
    }
    at += n;
  }
};

static void damaged(const char* why, U64 at) { die(EX_DATA, "damaged input: %s (meta file byte %llu)", why, (unsigned long long)at); }

static void join(const std::string& prefix, Sink& out, U64& nbytes) {
  const std::string mname = prefix + ".meta", dname = prefix + ".dtsc";
  FILE* fm = fopen(mname.c_str(), "rb");
  if (!fm) die(EX_READ, "can't open %s", mname.c_str());
  FILE* fd = fopen(dname.c_str(), "rb");
  if (!fd) die(EX_READ, "can't open %s", dname.c_str());
  RecIn in;
  in.init_file(fm, mname.c_str());
  U8 h[9];
  in.bytes(h, 9);
  if (in.bad || memcmp(h, MAGIC, 8)) die(EX_DATA, "%s: not a dtssplit meta file", mname.c_str());
  if (h[8] != VERSION) die(EX_DATA, "%s: format version %d, which this dtssplit (version %d) doesn't read", mname.c_str(), h[8], VERSION);
  Layout lay[XNWAV];
  bool have[XNWAV] = {false};
  WavIn wi[XNWAV];
  FILE* wf[XNWAV] = {0};
  U64 wsamples[XNWAV] = {0};
  Split sp;
  std::vector<S32> wv;
  std::vector<U8> buf;
  U32 crc = 0;
  U64 total = 0;
  for (;;) {
    const U64 at = in.nread;
    const int tag = in.u8();
    if (in.bad) damaged("the meta file ends early", at);
    buf.clear();
    if (tag == 'R' || tag == 'E') {
      if (!in.blob(buf, tag == 'R' ? 2 * RAW_MAX : 1 << 20)) damaged("a damaged record", at);
    } else if (tag == 'C') {
      const U64 n = in.uv();
      if (in.bad || n < 96 || n > 16384) damaged("a damaged record", at);
      buf.resize(n);
      if (fread(&buf[0], 1, n, fd) != n) die(EX_DATA, "%s ends early", dname.c_str());
    } else if (tag == 'W') {
      const U64 w = in.uv();
      if (in.bad || w >= XNWAV || have[w] || !get_layout(in, lay[w])) damaged("a damaged layout record", at);
      const std::string wn = wav_name(prefix, (int)w);
      if (!(wf[w] = fopen(wn.c_str(), "rb"))) die(EX_READ, "can't open %s", wn.c_str());
      const char* why = wi[w].open(wf[w]);
      if (why) die(EX_DATA, "%s: %s", wn.c_str(), why);
      if (wi[w].nch != lay[w].nch || wi[w].bits != lay[w].bits || wi[w].rate != lay[w].rate)
        die(EX_DATA, "%s: %d channels, %d-bit, %d Hz, where the meta file has %d, %d-bit, %d Hz", wn.c_str(), wi[w].nch,
            wi[w].bits, wi[w].rate, lay[w].nch, lay[w].bits, lay[w].rate);
      have[w] = true;
      continue;
    } else if (tag == 'X') {
      const U64 w = in.uv();
      if (in.bad || w >= XNWAV || !have[w]) damaged("a damaged record", at);
      WavSamples ws;
      ws.w = &wi[w];
      const char* why = join_x(in, &lay[w], ws, buf, sp, wv);
      if (why) damaged(why, at);
      wsamples[w] += (U64)sp.x.nframesamples * sp.x.nbands;
    } else if (tag == 'Z') {
      const U64 n = in.uv(), c = in.uv(), nw = in.uv();
      if (in.bad || nw > XNWAV) damaged("a damaged end record", at);
      for (U64 w = 0; w < nw; w++)
        if (in.uv() != wsamples[w]) damaged("sample counts unlike the WAVs'", at);
      if (in.bad || !in.at_end()) damaged("a damaged end record", at);
      if (n != total || c != crc) damaged("the stream joined isn't the one split", at);
      if (fgetc(fd) != EOF) die(EX_DATA, "%s: more than the meta file takes", dname.c_str());
      for (U64 w = 0; w < XNWAV; w++) {
        S32 extra;
        if (have[w] && wi[w].read(&extra, 1)) die(EX_DATA, "%s: more samples than the meta file takes", wav_name(prefix, (int)w).c_str());
        if (wf[w]) fclose(wf[w]);
      }
      break;
    } else {
      damaged("an unknown record", at);
    }
    if (buf.size()) out.write(&buf[0], buf.size());
    crc = crc32_upd(crc, buf.size() ? &buf[0] : 0, buf.size());
    total += buf.size();
  }
  fclose(fm);
  fclose(fd);
  nbytes = total;
}

// ---------------------------------------------------------------------------------------------

static void usage(void) {
  fputs(
      "dtssplit - lossless split of DTS (DTS-HD Master Audio) streams and the join back\n"
      "Usage: dtssplit c [options] input.dts prefix\n"
      "       dtssplit d [options] prefix restored.dts\n"
      "  c    split: the core frames into prefix.dtsc, the lossless extension's samples into\n"
      "       prefix.wav (and prefix.2.wav, ... for further channel layouts), all else into\n"
      "       prefix.meta\n"
      "  d    join them back into the stream, byte for byte\n"
      "  -v   (c) join the files written and compare the result with the input\n"
      "  -q   no statistics\n"
      "The input of c and the output of d can be - (stdin, stdout).\n",
      stderr);
  exit(EX_USAGE);
}

static bool same_file(const char* a, const char* b) {
  struct stat sa, sb;
  if (stat(a, &sa) || stat(b, &sb)) return false;
#ifdef _WIN32
  return false;
#else
  return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
#endif
}

int main(int argc, char** argv) {
  crc_init();
  dtscore_init();
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) usage();
  const bool cmode = argv[1][0] == 'c';
  bool verify = false, quiet = false;
  std::vector<const char*> files;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1]) {
      for (const char* o = argv[i] + 1; *o; o++) {
        if (*o == 'v' && cmode)
          verify = true;
        else if (*o == 'q')
          quiet = true;
        else
          usage();
      }
    } else {
      files.push_back(argv[i]);
    }
  }
  if (files.size() != 2) usage();
  if (cmode) {
    const std::string prefix = files[1];
    if (prefix == "-") die(EX_USAGE, "the prefix names files: not -");
    if (verify && !strcmp(files[0], "-")) die(EX_USAGE, "-v needs the input as a file, not -");
    static const char* const ext[] = {".dtsc", ".meta", ".wav"};
    for (int i = 0; i < 3; i++)
      if (strcmp(files[0], "-") && (same_file(files[0], (prefix + ext[i]).c_str()) || prefix + ext[i] == files[0]))
        die(EX_USAGE, "%s would overwrite the input", (prefix + ext[i]).c_str());
    FILE* fi = stdin;
    if (strcmp(files[0], "-")) {
      if (!(fi = fopen(files[0], "rb"))) die(EX_READ, "can't open the input %s", files[0]);
    } else {
#ifdef _WIN32
      _setmode(_fileno(stdin), _O_BINARY);
#endif
    }
    static Splitter sp;
    sp.prefix = prefix;
    Stats& st = sp.stt;
    st.bytes = st.raw = st.core = st.exss = st.xsplit = st.exceptions = st.ovl_bits = 0;
    st.m_exss = st.m_xhdr = st.m_param = st.m_kept = st.m_raw = 0;
    U32 crc;
    sp.run(fi, crc);
    if (fi != stdin) fclose(fi);
    if (!quiet) {
      fprintf(stderr, "%s: %llu bytes: %llu core frames, %llu extension substream frames (%llu with XLL split", files[0],
              (unsigned long long)st.bytes, (unsigned long long)st.core, (unsigned long long)st.exss,
              (unsigned long long)st.xsplit);
      for (size_t i = 0; i < st.why.size(); i++)
        fprintf(stderr, "; %llu kept as bytes: %s", (unsigned long long)st.why_n[i], st.why[i].c_str());
      fprintf(stderr, "), %llu bytes between frames\n", (unsigned long long)st.raw);
      fprintf(stderr, "%s: %llu bytes, the core frames\n", sp.dts.name.c_str(), (unsigned long long)sp.dts.size);
      for (int w = 0; w < sp.nwav; w++)
        fprintf(stderr, "%s: %llu bytes: %llu samples a channel, %d channels, %d-bit, %d Hz\n", sp.wav[w].name.c_str(),
                (unsigned long long)sp.wav[w].size(), (unsigned long long)sp.wav[w].frames, sp.wav[w].nch,
                sp.wav[w].bits, sp.wav[w].rate);
      fprintf(stderr,
              "%s: %llu bytes: extension substream headers %llu, XLL headers %llu, XLL coding parameters %llu, "
              "frames kept as bytes %llu, bytes between frames %llu, the rest %llu; %llu values clipped, %llu "
              "overlapping LSB bits\n",
              sp.meta.name.c_str(), (unsigned long long)sp.meta.size, (unsigned long long)st.m_exss,
              (unsigned long long)st.m_xhdr, (unsigned long long)st.m_param, (unsigned long long)st.m_kept,
              (unsigned long long)st.m_raw,
              (unsigned long long)(sp.meta.size - st.m_exss - st.m_xhdr - st.m_param - st.m_kept - st.m_raw),
              (unsigned long long)st.exceptions, (unsigned long long)st.ovl_bits);
    }
    if (verify) {
      CmpSink cs;
      cs.f = fopen(files[0], "rb");
      if (!cs.f) die(EX_READ, "can't open the input %s", files[0]);
      cs.at = 0;
      cs.diff = false;
      U64 n;
      join(prefix, cs, n);
      const int more = fgetc(cs.f);
      fclose(cs.f);
      if (cs.diff || more != EOF || n != st.bytes) {
        fprintf(stderr, "verify: FAILED at byte %llu\n", (unsigned long long)cs.at);
        return EX_FAIL;
      }
      if (!quiet) fprintf(stderr, "verify: OK\n");
    }
  } else {
    FileSink fsk;
    if (strcmp(files[1], "-")) {
      if (!(fsk.f = fopen(files[1], "wb"))) die(EX_WRITE, "can't create %s", files[1]);
    } else {
#ifdef _WIN32
      _setmode(_fileno(stdout), _O_BINARY);
#endif
      fsk.f = stdout;
    }
    U64 n;
    join(files[0], fsk, n);
    if (fsk.f != stdout ? fclose(fsk.f) : fflush(fsk.f)) die(EX_WRITE, "can't write the output");
  }
  return EX_OK;
}
#endif
