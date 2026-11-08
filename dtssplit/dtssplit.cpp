// dtssplit - lossless split of a DTS stream into its lossy part and a WAV, and the join back
//
// Usage: dtssplit c [options] input.dts output.ac3 output.wav
//        dtssplit d [options] output.ac3 output.wav restored.dts
//   c    split the stream: the side file (output.ac3, any name) and the WAV
//   d    join the side file and the WAV back into the stream, byte for byte
//   -v   (c) join the two files written and compare the result with the input
//   -q   no statistics
// The input of c and the output of d can be - for stdin and stdout (not with -v).
//
// A DTS-HD Master Audio stream is, frame by frame, a DTS core frame (the lossy part, which any DTS
// decoder plays) and an extension substream (EXSS) frame that carries the lossless extension
// (XLL): per channel the difference between the original PCM and the core's decode (or the PCM
// itself), decorrelated in channel pairs, linear-predicted and coded with Rice and binary codes.
// The split decodes each XLL frame down to the samples it codes, its prediction and decorrelation
// undone, and writes them to the WAV; all else goes to the side file: the core frames and the EXSS
// headers as they are, the XLL headers, and each segment's coding parameters (the code choices, the
// Rice parameters, the places of the isolated samples of hybrid Rice codes, the bits after the
// codes). The join predicts and codes the WAV's samples again with those parameters. The split
// joins every frame back before it writes it so and compares it with the original; a frame that
// doesn't come back the same, or that the split doesn't handle, is kept as bytes, and so is all
// that isn't a frame. Any input therefore joins back to itself.
//
// The WAV holds a sample a channel for each sample of each XLL frame, its channels in WAV order
// (the stream's speaker positions), 24-bit (16-bit for 16-bit streams; 32-bit where the XLL embeds
// a downmix, whose lower channel sets hold the downmix unscaled). In a residual channel (the XLL's
// usual one) a sample is the original minus the core's decode, at the XLL's precision: what the
// lossy core leaves out of the audio. In a channel coded without the core it is the audio. A value
// the WAV's samples can't hold is written clipped and kept in the side file.
//
// The side file: "DTSSPLIT", a version byte, then records, each a tag byte and its fields (numbers
// as LEB128 varints, signed ones zigzagged):
//   R n bytes          bytes that aren't a frame
//   C n bytes          a core frame
//   E n bytes          an extension substream frame kept as bytes
//   W ...              the WAV's layout (before the first X): its channels, their speaker mask,
//                      which XLL channel each is, and the channel sets it takes
//   X ...              an extension substream frame with its XLL frame split: the EXSS bytes
//                      before the XLL frame, the XLL header bytes, the coding parameters of each
//                      segment of each channel set, the XLL bytes after the band data, the EXSS
//                      bytes after the XLL frame, the WAV values written clipped
//   Z n crc samples    the end: the input's size and CRC-32, the WAV's samples a channel
//
// Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
// Profiles), clause 7 (the extension substream) and clause 8 (XLL).
//
// Build: g++ -O2 -o dtssplit dtssplit.cpp
//        x86_64-w64-mingw32-g++ -O2 -s -static -o dtssplit.exe dtssplit.cpp

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

typedef uint8_t U8;
typedef uint16_t U16;
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
};

// the signed value of a code's unsigned word, and back
static inline S32 unzig(U32 u) { return u & 1 ? -(S32)(u >> 1) - 1 : (S32)(u >> 1); }
static inline U32 zig(S32 v) { return v < 0 ? (((U32)(-(v + 1))) << 1) | 1 : (U32)v << 1; }

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
  const U8* mem;
  size_t msize, mpos;
  std::vector<U8> buf;
  size_t lo, hi;
  bool bad;   // read past the end, or a number that doesn't fit
  U64 nread;  // bytes read
  void init_file(FILE* fp) {
    f = fp;
    mem = 0;
    buf.resize(1 << 16);
    lo = hi = 0;
    bad = false;
    nread = 0;
  }
  void init_mem(const U8* p, size_t n) {
    f = 0;
    mem = p;
    msize = n;
    mpos = 0;
    bad = false;
    nread = 0;
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
    if (lo == hi) {
      lo = 0;
      hi = fread(&buf[0], 1, buf.size(), f);
      if (!hi) {
        if (ferror(f)) die(EX_READ, "can't read the side file");
        bad = true;
        return 0;
      }
    }
    nread++;
    return buf[lo++];
  }
  int peek_end() {  // 1 at the end of the input
    if (!f) return mpos >= msize;
    if (lo == hi) {
      lo = 0;
      hi = fread(&buf[0], 1, buf.size(), f);
      if (!hi && ferror(f)) die(EX_READ, "can't read the side file");
    }
    return lo == hi;
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
      if (lo == hi) {
        lo = 0;
        hi = fread(&buf[0], 1, buf.size(), f);
        if (!hi) {
          if (ferror(f)) die(EX_READ, "can't read the side file");
          bad = true;
          memset(p, 0, n);
          return;
        }
      }
      size_t k = hi - lo < n ? hi - lo : n;
      memcpy(p, &buf[lo], k);
      lo += k;
      p += k;
      n -= k;
      nread += k;
    }
  }
  // a length-prefixed byte string, at most max bytes
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
  int nch, bits, rate;
  U32 mask;
  U64 frames;   // samples a channel written
  U64 nbytes;   // data bytes written
  std::vector<U8> buf;
  void open(FILE* fp) {
    f = fp;
    nch = 1;
    bits = 16;
    rate = 48000;
    mask = 0;
    frames = nbytes = 0;
    U8 z[WAV_HEAD] = {0};
    if (fwrite(z, 1, WAV_HEAD, f) != WAV_HEAD) die(EX_WRITE, "can't write the WAV");
  }
  // count values, interleaved
  void write(const S32* v, size_t count) {
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
    if (count && fwrite(&buf[0], 1, buf.size(), f) != buf.size()) die(EX_WRITE, "can't write the WAV");
    nbytes += buf.size();
    frames += count / nch;
  }
  void finish() {
    U8 h[WAV_HEAD];
    memset(h, 0, sizeof h);
    const int pad = (int)(nbytes & 1);
    if (pad && fputc(0, f) == EOF) die(EX_WRITE, "can't write the WAV");
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
    if (fflush(f) || fseek(f, 0, SEEK_SET) || fwrite(h, 1, WAV_HEAD, f) != WAV_HEAD || fflush(f))
      die(EX_WRITE, "can't write the WAV's header (the WAV needs to be a file)");
  }
};

// any PCM WAV (RIFF or RF64, WAVE_FORMAT_PCM or EXTENSIBLE): what the join reads its samples from
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
      U64 n = get32(c + 4);
      if (!memcmp(c, "data", 4)) {
        if (!fmt) return "a WAV without its format before its data";
        left = rf64 && n == 0xFFFFFFFFu ? ds64 : n;
        if (n == 0xFFFFFFFFu && !rf64) left = ~0ull;
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
  // count interleaved values; false where the data ends first
  bool read(S32* v, size_t count) {
    const int bps = bits / 8;
    const size_t n = count * bps;
    if (left != ~0ull && n > left) return false;
    buf.resize(n);
    if (n && fread(&buf[0], 1, n, f) != n) {
      if (ferror(f)) die(EX_READ, "can't read the WAV");
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

// the size of the core frame at p (av bytes there), where it is one: the sync word and a header
// whose frame size and block count are in range; 0 where it isn't
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

// The XLL data of the extension substream frame p (n bytes): its offset and size; false where it
// has none, or the header can't be followed (why says why)
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
    why = "no extension substream static fields yet";
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

enum { XCH = 16, XCS = 16, XORD = 16, XSEG = 512, XFRAME = 65536, XWAV = 64 };

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

// a channel set: its sub-header (band 0), and the coding parameters as they go from segment to segment
struct Chset {
  int nch, resenc, pcm_res, storage_res, freq, nbands;
  int primary, dmix_coeffs, dmix_embedded, dmix_type, hier, chmask, hier_ofs, nabits;
  int decor, order[XCH], dcoeff[XCH / 2];
  int aorder[XCH], forder[XCH], highest;
  int rc[XCH][XORD];
  int lsb_size, nlsb[XCH], adj[XCH];  // lsb_size: the bytes at the end of a segment's data the LSB part takes
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
};

static inline S32 sat23(S32 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : a; }
static inline S32 mul16(S32 a, S32 b) { return (S32)(((S64)a * b + (1 << 15)) >> 16); }

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
    if (s.freq > 96000) {
      // (two bands where bXtraFreqBands is 0, as decoders read it, though the text of 8.3.2 has 1)
      s.nbands = r.get(1) ? 4 : 2;
    } else {
      s.nbands = 1;
    }
    if (s.nbands > x.nbands) x.nbands = s.nbands;
    s.decor = r.get(1);
    int chbits = 0;
    while ((1 << chbits) < s.nch) chbits++;
    if (s.decor) {
      for (int ch = 0; ch < s.nch; ch++) s.order[ch] = r.get(chbits);
      for (int i = 0; i < s.nch / 2; i++) s.dcoeff[i] = r.get(1) ? unzig(r.get(7)) : 0;
    } else {
      for (int ch = 0; ch < s.nch; ch++) s.order[ch] = ch;
    }
    int seen = 0;
    for (int ch = 0; ch < s.nch; ch++) {
      if (s.order[ch] >= s.nch || (seen >> s.order[ch] & 1)) return "a channel order that isn't a permutation";
      seen |= 1 << s.order[ch];
    }
    s.highest = 0;
    for (int ch = 0; ch < s.nch; ch++) {
      s.aorder[ch] = r.get(4);
      if (s.aorder[ch] > s.highest) s.highest = s.aorder[ch];
    }
    if (s.highest > x.nsegsamples) return "a prediction order above the segment's samples";
    for (int ch = 0; ch < s.nch; ch++) s.forder[ch] = s.aorder[ch] ? 0 : r.get(2);
    for (int ch = 0; ch < s.nch; ch++)
      for (int j = 0; j < s.aorder[ch]; j++) {
        const int q = unzig(r.get(8));
        if (q == -128) return "a reflection coefficient out of range";
        s.rc[ch][j] = q >= 0 ? RC_TAB[q] : -(int)RC_TAB[-q];
      }
    if (x.scalable) {
      s.lsb_size = (int)r.get(x.seg_nbits);
      if (s.lsb_size && x.band_crc > 1) s.lsb_size += 2;
      for (int ch = 0; ch < s.nch; ch++) s.nlsb[ch] = r.get(4);
      for (int ch = 0; ch < s.nch; ch++) s.adj[ch] = r.get(4);
      for (int ch = 0; ch < s.nch; ch++)
        if (s.nlsb[ch] && !s.lsb_size) return "LSBs without an LSB part";
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

// the width of the LSB part channel ch's MSBs are shifted by (8.6.1)
static int lsb_shift(const Xll& x, const Chset& s, int ch) {
  if (x.fixed_lsb) return x.fixed_lsb;
  return s.nlsb[ch] && s.adj[ch] ? s.nlsb[ch] - 1 + s.adj[ch] : s.nlsb[ch] + s.adj[ch];
}

// what the split takes: band 0 alone, channel sets alike, MSB and LSB parts that split again; 0 or why not
static const char* xll_supported(const Xll& x) {
  if (x.nbands > 1) return "XLL frequency bands above band 0";
  for (int c = 0; c < x.nchsets; c++) {
    const Chset& s = x.cs[c];
    if (s.freq != x.cs[0].freq || s.storage_res != x.cs[0].storage_res) return "XLL channel sets unlike each other";
    if (s.storage_res != 16 && s.storage_res != 20 && s.storage_res != 24) return "an XLL sample width not 16, 20 or 24";
    for (int ch = 0; ch < s.nch; ch++) {
      const int sh = lsb_shift(x, s, ch);
      if (s.nlsb[ch] + s.adj[ch] > sh || sh > 24) return "XLL LSBs that overlap the MSBs (bit-width adjustment)";
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
    w.put(0, (8 - (int)(w.bits() & 7)) & 7);
    keep = w.v;
  }
}

// the band data of XLL frame p: residuals and LSBs into res and lsb ([channel][sample], channel
// sets one after the other, a set's channels in coded order), the segments' coding into ck, the
// end of the band data into data_end; 0 or why it can't be followed
static const char* xll_data(Xll& x, const U8* p, std::vector<Chunk>& ck, S32* res, S32* lsb, int& data_end) {
  const int N = x.nframesamples, S = x.nsegsamples;
  U64 at = (U64)x.data_start * 8;
  ck.assign((size_t)x.nsegs * x.nchsets, Chunk());
  for (int seg = 0; seg < x.nsegs; seg++)
    for (int c = 0; c < x.nchsets; c++) {
      Chset& s = x.cs[c];
      Chunk& k = ck[(size_t)seg * x.nchsets + c];
      const U64 cend = at + (U64)x.navi[(size_t)seg * x.nchsets + c] * 8;
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
            s.na[i] = s.seg_common ? s.highest : s.aorder[i];
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
        S32* pa = res + (size_t)(x.chofs[c] + ch) * N + (size_t)seg * S;
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
      const U64 lstart = cend - (U64)s.lsb_size * 8;
      if (s.lsb_size && lstart < at) return "an LSB part larger than its segment";
      if (r.pos > lstart) return "an MSB part that runs into the LSB part";
      gap_of(p, r.pos, lstart, k.g1, k.g1n, k.g1b);
      k.g2 = 0;
      k.g2n = 0;
      if (s.lsb_size) {
        BitReader q;
        q.init(p, lstart, cend);
        for (int ch = 0; ch < s.nch; ch++)
          if (s.nlsb[ch]) {
            S32* pl = lsb + (size_t)(x.chofs[c] + ch) * N + (size_t)seg * S;
            for (int j = 0; j < S; j++) pl[j] = (S32)q.get(s.nlsb[ch]);
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

// the samples of the XLL frame (val: [channel][sample], a set's channels in output order) from its
// residuals and LSBs: the prediction and the pairwise decorrelation undone, MSBs and LSBs put together
static void xll_decode(const Xll& x, const S32* res, const S32* lsb, S32* val, std::vector<U32>& tmp) {
  const int N = x.nframesamples;
  tmp.resize((size_t)XCH * N);
  for (int c = 0; c < x.nchsets; c++) {
    const Chset& s = x.cs[c];
    for (int ch = 0; ch < s.nch; ch++) {
      U32* b = &tmp[(size_t)ch * N];
      const S32* e = res + (size_t)(x.chofs[c] + ch) * N;
      const int o = s.aorder[ch];
      if (o) {
        int co[XORD];
        rc_to_lpc(s.rc[ch], o, co);
        for (int n = 0; n < o && n < N; n++) b[n] = (U32)e[n];
        for (int n = o; n < N; n++) b[n] = (U32)e[n] - (U32)lpc_pred(b, n, o, co);
      } else {
        for (int n = 0; n < N; n++) b[n] = (U32)e[n];
        for (int j = 0; j < s.forder[ch]; j++)
          for (int n = 1; n < N; n++) b[n] += b[n - 1];
      }
    }
    if (s.decor)
      for (int i = 0; i < s.nch / 2; i++)
        if (s.dcoeff[i]) {
          const U32* bb = &tmp[(size_t)(2 * i) * N];
          U32* bd = &tmp[(size_t)(2 * i + 1) * N];
          for (int n = 0; n < N; n++) bd[n] += (U32)decor_term(bb[n], s.dcoeff[i]);
        }
    for (int i = 0; i < s.nch; i++) {
      const int oc = s.order[i];
      const U32* b = &tmp[(size_t)i * N];
      S32* v = val + (size_t)(x.chofs[c] + oc) * N;
      const int sh = lsb_shift(x, s, oc), a = s.adj[oc];
      const S32* l = lsb + (size_t)(x.chofs[c] + oc) * N;
      if (!sh)
        for (int n = 0; n < N; n++) v[n] = (S32)b[n];
      else if (s.nlsb[oc])
        for (int n = 0; n < N; n++) v[n] = (S32)((b[n] << sh) + ((U32)l[n] << a));
      else
        for (int n = 0; n < N; n++) v[n] = (S32)(b[n] << sh);
    }
  }
}

// the residuals and LSBs that decode to val (xll_decode's inverse); false where val doesn't split
// into MSBs and LSBs as the frame's header says
static bool xll_encode_samples(const Xll& x, const S32* val, S32* res, S32* lsb, std::vector<U32>& tmp) {
  const int N = x.nframesamples;
  tmp.resize((size_t)2 * XCH * N);
  for (int c = 0; c < x.nchsets; c++) {
    const Chset& s = x.cs[c];
    for (int i = 0; i < s.nch; i++) {
      const int oc = s.order[i];
      U32* b = &tmp[(size_t)i * N];
      const S32* v = val + (size_t)(x.chofs[c] + oc) * N;
      const int sh = lsb_shift(x, s, oc), a = s.adj[oc], nl = s.nlsb[oc];
      S32* l = lsb + (size_t)(x.chofs[c] + oc) * N;
      if (!sh) {
        for (int n = 0; n < N; n++) b[n] = (U32)v[n];
      } else {
        const U32 m = (1u << sh) - 1, am = (1u << a) - 1;
        for (int n = 0; n < N; n++) {
          const U32 u = (U32)v[n], rem = u & m;
          b[n] = (U32)((S32)u >> sh);
          if (nl) {
            if (rem & am || (rem >> a) >> nl) return false;
            l[n] = (S32)(rem >> a);
          } else if (rem) {
            return false;
          }
        }
      }
    }
    if (s.decor)
      for (int i = 0; i < s.nch / 2; i++)
        if (s.dcoeff[i]) {
          const U32* bb = &tmp[(size_t)(2 * i) * N];
          U32* bd = &tmp[(size_t)(2 * i + 1) * N];
          for (int n = 0; n < N; n++) bd[n] -= (U32)decor_term(bb[n], s.dcoeff[i]);
        }
    for (int ch = 0; ch < s.nch; ch++) {
      const U32* b = &tmp[(size_t)ch * N];
      S32* e = res + (size_t)(x.chofs[c] + ch) * N;
      const int o = s.aorder[ch];
      if (o) {
        int co[XORD];
        rc_to_lpc(s.rc[ch], o, co);
        for (int n = 0; n < o && n < N; n++) e[n] = (S32)b[n];
        for (int n = o; n < N; n++) e[n] = (S32)(b[n] + (U32)lpc_pred(b, n, o, co));
      } else {
        U32* d = &tmp[(size_t)(XCH + ch) * N];
        memcpy(d, b, (size_t)N * sizeof *d);
        for (int j = 0; j < s.forder[ch]; j++)
          for (int n = N - 1; n > 0; n--) d[n] -= d[n - 1];
        for (int n = 0; n < N; n++) e[n] = (S32)d[n];
      }
    }
  }
  return true;
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
static const char* xll_write_data(Xll& x, const std::vector<Chunk>& ck, const S32* res, const S32* lsb, BitWriter& w) {
  const int N = x.nframesamples, S = x.nsegsamples;
  for (int seg = 0; seg < x.nsegs; seg++)
    for (int c = 0; c < x.nchsets; c++) {
      Chset& s = x.cs[c];
      const Chunk& k = ck[(size_t)seg * x.nchsets + c];
      const U64 start = w.bits(), cend = start + (U64)x.navi[(size_t)seg * x.nchsets + c] * 8;
      const U64 lstart = cend - (U64)s.lsb_size * 8;
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
            s.na[i] = s.seg_common ? s.highest : s.aorder[i];
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
        const S32* pa = res + (size_t)(x.chofs[c] + ch) * N + (size_t)seg * S;
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
      if (w.bits() > lstart) return "an MSB part longer than its place";
      if (k.g1) {
        if (k.g1n != lstart - w.bits()) return "an MSB part of another length";
        w.copy(&k.g1b[0], 0, k.g1n);
      } else {
        w.zeros(lstart - w.bits());
      }
      if (s.lsb_size) {
        for (int ch = 0; ch < s.nch; ch++)
          if (s.nlsb[ch]) {
            const S32* pl = lsb + (size_t)(x.chofs[c] + ch) * N + (size_t)seg * S;
            for (int j = 0; j < S; j++) w.put((U32)pl[j], s.nlsb[ch]);
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
// the WAV's layout: which XLL channel each WAV channel is

// a DTS speaker position's WAV speaker bit (0: none)
static const U32 SPK_WAV[32] = {
    0x4,    0x1,    0x2,     0x200,  0x400,  0x8,     0x100, 0x10,  // C L R Ls Rs LFE1 Cs Lsr
    0x20,   0x200,  0x400,   0x40,   0x80,   0x1000,  0x2000, 0x4000,  // Rsr Lss Rss Lc Rc Lh Ch Rh
    0,      0,      0,       0x800,  0,      0,       0x10000, 0x8000,  // LFE2 Lw Rw Oh Lhs Rhs Chr Lhr
    0x20000, 0,     0,       0,      0,      0,       0,     0};       // Rhr Cl Ll Rl

struct Layout {
  int nch, bits, rate;
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
  if (a.nch != b.nch || a.bits != b.bits || a.rate != b.rate || a.nchsets != b.nchsets) return false;
  for (int c = 0; c < a.nchsets; c++)
    if (a.cs_nch[c] != b.cs_nch[c] || a.cs_mask[c] != b.cs_mask[c]) return false;
  return true;
}

static void put_layout(Rec& o, const Layout& L) {
  o.u8('W');
  o.uv(L.nch);
  o.uv(L.bits);
  o.uv(L.rate);
  o.uv(L.mask);
  for (int i = 0; i < L.nch; i++) o.uv(L.perm[i]);
  o.uv(L.nchsets);
  for (int c = 0; c < L.nchsets; c++) {
    o.uv(L.cs_nch[c]);
    o.uv((U32)L.cs_mask[c]);
  }
}

static bool get_layout(RecIn& in, Layout& L) {
  L.nch = (int)in.uv();
  L.bits = (int)in.uv();
  L.rate = (int)in.uv();
  L.mask = (U32)in.uv();
  if (L.nch < 1 || L.nch > XWAV || (L.bits != 16 && L.bits != 24 && L.bits != 32)) return false;
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

static void put_chunk(Rec& o, Xll& x, int seg, int c, const Chunk& k) {
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
  if (k.g1) o.blob(&k.g1b[0], k.g1b.size()), o.uv(k.g1n);
  if (k.g2) o.blob(&k.g2b[0], k.g2b.size()), o.uv(k.g2n);
}

static bool get_chunk(RecIn& in, Xll& x, int seg, int c, Chunk& k) {
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
      const int b = in.u8();
      k.rice[i] = b & 1;
      k.hyb[i] = b >> 1 & 1;
      if (b >> 2 || (k.hyb[i] && (s.seg_common || !k.rice[i]))) return false;
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
  std::vector<S32> res, lsb, val;
  std::vector<U32> tmp;
  int data_end;
};

// the clipped values of the frame's samples, in WAV order (count: nframesamples * nch)
static void wav_values(const Xll& x, const Layout& L, const S32* val, S32* out, std::vector<std::pair<U64, S32> >& exc) {
  const int N = x.nframesamples, W = L.nch;
  const S32 lo = (S32)(-(1ll << (L.bits - 1))), hi = (S32)((1ll << (L.bits - 1)) - 1);
  exc.clear();
  for (int n = 0; n < N; n++)
    for (int w = 0; w < W; w++) {
      const S32 v = val[(size_t)L.perm[w] * N + n];
      const S32 c = v < lo ? lo : v > hi ? hi : v;
      if (c != v) exc.push_back(std::make_pair((U64)n * W + w, v));
      out[(size_t)n * W + w] = c;
    }
}

static void put_x(Rec& o, const U8* e, int n, int xoff, const U8* xf, int one2one, Split& sp,
                  const std::vector<std::pair<U64, S32> >& exc) {
  Xll& x = sp.x;
  o.u8('X');
  o.blob(e, xoff);
  o.u8(one2one);
  o.uv(x.frame_size);
  o.blob(xf, x.data_start);
  for (int seg = 0; seg < x.nsegs; seg++)
    for (int c = 0; c < x.nchsets; c++) put_chunk(o, x, seg, c, sp.ck[(size_t)seg * x.nchsets + c]);
  o.blob(xf + sp.data_end, x.frame_size - sp.data_end);
  o.blob(e + xoff + x.frame_size, n - xoff - x.frame_size);
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

// An X record (its tag read) joined into out: 0, or why it can't be
static const char* join_x(RecIn& in, const Layout* L, Samples& smp, std::vector<U8>& out, Split& sp,
                          std::vector<S32>& wv) {
  std::vector<U8> pre, hdr, tail, post;
  if (!in.blob(pre, 1 << 20)) return "a damaged record";
  const int one2one = in.u8();
  const U64 fsize = in.uv();
  if (in.bad || one2one > 1 || fsize > (1 << 20) || !in.blob(hdr, fsize)) return "a damaged record";
  Xll& x = sp.x;
  // the header, as the frame's start: what xll_headers reads it from
  std::vector<U8> fb(fsize + PAD, 0);
  memcpy(&fb[0], &hdr[0], hdr.size());
  const char* why = xll_headers(x, &fb[0], (int)fsize, one2one);
  if (why) return why;
  if ((size_t)x.data_start != hdr.size() || (U64)x.frame_size != fsize) return "a damaged record";
  if ((why = xll_supported(x))) return why;
  Layout fl;
  layout_of(x, fl);
  if (!L || !layout_same(*L, fl)) return "an XLL frame unlike the WAV";
  sp.ck.assign((size_t)x.nsegs * x.nchsets, Chunk());
  for (int seg = 0; seg < x.nsegs; seg++)
    for (int c = 0; c < x.nchsets; c++)
      if (!get_chunk(in, x, seg, c, sp.ck[(size_t)seg * x.nchsets + c])) return "a damaged record";
  if (!in.blob(tail, fsize) || !in.blob(post, 1 << 20)) return "a damaged record";
  const int N = x.nframesamples, W = L->nch;
  wv.resize((size_t)N * W);
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
  sp.val.resize((size_t)N * W);
  for (int n = 0; n < N; n++)
    for (int w = 0; w < W; w++) sp.val[(size_t)L->perm[w] * N + n] = wv[(size_t)n * W + w];
  sp.res.assign((size_t)N * W, 0);
  sp.lsb.assign((size_t)N * W, 0);
  if (!xll_encode_samples(x, &sp.val[0], &sp.res[0], &sp.lsb[0], sp.tmp)) return "WAV samples the XLL frame can't code";
  BitWriter w;
  w.reset();
  w.bytes(&hdr[0], hdr.size());
  if ((why = xll_write_data(x, sp.ck, &sp.res[0], &sp.lsb[0], w))) return why;
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
  U64 bytes, raw, core, exss, xsplit, wav_samples, exceptions;
  U64 s_core, s_kept, s_exss, s_xhdr, s_param;  // the side file: core frames, frames kept as bytes,
                                                 // EXSS headers, XLL headers, the rest of X records
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

struct SideOut {
  FILE* f;
  U64 size;
  void write(const Rec& r) {
    if (r.b.size() && fwrite(&r.b[0], 1, r.b.size(), f) != r.b.size()) die(EX_WRITE, "can't write the side file");
    size += r.b.size();
  }
};

static const U8 MAGIC[8] = {'D', 'T', 'S', 'S', 'P', 'L', 'I', 'T'};
enum { VERSION = 1 };

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

static void split_exss(const U8* e, int n, ExssState& st, Layout& L, bool& have_layout, SideOut& so, WavOut& wo,
                       Stats& stt, Split& sp, Split& chk, std::vector<S32>& wv, std::vector<S32>& wv2) {
  Rec o;
  int off = 0, size = 0, one2one = 0;
  const char* why = 0;
  stt.exss++;
  if (exss_xll(st, e, n, off, size, one2one, why)) {
    std::vector<U8> xf(size + PAD, 0);
    memcpy(&xf[0], e + off, size);
    Xll& x = sp.x;
    if (!(why = xll_headers(x, &xf[0], size, one2one)) && !(why = xll_supported(x))) {
      Layout fl;
      layout_of(x, fl);
      if (have_layout && !layout_same(L, fl)) why = "an XLL layout unlike the first frame's";
      if (!why) {
        const size_t cnt = (size_t)x.nframesamples * x.nch;
        sp.res.assign(cnt, 0);
        sp.lsb.assign(cnt, 0);
        sp.val.assign(cnt, 0);
        Xll hx = x;  // (the band data parse changes the coding state)
        why = xll_data(x, &xf[0], sp.ck, &sp.res[0], &sp.lsb[0], sp.data_end);
        if (!why) {
          x = hx;
          xll_decode(x, &sp.res[0], &sp.lsb[0], &sp.val[0], sp.tmp);
          std::vector<std::pair<U64, S32> > exc;
          wv.resize(cnt);
          wav_values(x, fl, &sp.val[0], &wv[0], exc);
          put_x(o, e, n, off, &xf[0], one2one, sp, exc);
          // the check: the record joined back as the join will
          RecIn in;
          in.init_mem(&o.b[1], o.b.size() - 1);
          MemSamples ms;
          ms.v = &wv[0];
          ms.n = wv.size();
          std::vector<U8> back;
          why = join_x(in, &fl, ms, back, chk, wv2);
          if (!why && (back.size() != (size_t)n || memcmp(&back[0], e, n) || in.mpos != in.msize))
            why = "an XLL frame that doesn't join back the same";
          if (!why) {
            if (!have_layout) {
              L = fl;
              have_layout = true;
              wo.nch = L.nch;
              wo.bits = L.bits;
              wo.rate = L.rate;
              wo.mask = L.mask;
              Rec lr;
              put_layout(lr, L);
              so.write(lr);
            }
            so.write(o);
            wo.write(&wv[0], wv.size());
            stt.s_exss += n - x.frame_size;
            stt.s_xhdr += x.data_start + (x.frame_size - sp.data_end);
            stt.s_param += o.b.size() - (n - x.frame_size) - x.data_start - (x.frame_size - sp.data_end);
            stt.xsplit++;
            stt.wav_samples += x.nframesamples;
            stt.exceptions += exc.size();
            return;
          }
        }
      }
    }
  }
  stt.kept(why ? why : "?");
  o.clear();
  o.u8('E');
  o.blob(e, n);
  so.write(o);
  stt.s_kept += o.b.size();
}

static void put_bytes(SideOut& so, int tag, const U8* p, size_t n) {
  Rec o;
  o.u8(tag);
  o.blob(p, n);
  so.write(o);
}

static FILE* open_in(const char* path, const char* what) {
  if (!strcmp(path, "-")) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    return stdin;
  }
  FILE* f = fopen(path, "rb");
  if (!f) die(EX_READ, "can't open %s %s", what, path);
  return f;
}

static FILE* open_out(const char* path, const char* what) {
  if (!strcmp(path, "-")) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    return stdout;
  }
  FILE* f = fopen(path, "wb");
  if (!f) die(EX_WRITE, "can't create %s %s", what, path);
  return f;
}

static void split(const char* ipath, const char* spath, const char* wpath, Stats& stt, U32& crc, U64& side, U64& wav) {
  FILE* fi = open_in(ipath, "the input");
  SideOut so;
  so.f = open_out(spath, "the side file");
  so.size = 0;
  if (!strcmp(wpath, "-")) die(EX_USAGE, "the WAV can't be stdout (its header is written last)");
  WavOut wo;
  wo.open(open_out(wpath, "the WAV"));
  Rec hr;
  hr.bytes(MAGIC, 8);
  hr.u8(VERSION);
  so.write(hr);
  Input in;
  in.init(fi);
  ExssState st;
  memset(&st, 0, sizeof st);
  Layout L;
  memset(&L, 0, sizeof L);
  bool have_layout = false;
  Split sp, chk;
  std::vector<S32> wv, wv2;
  std::vector<U8> raw;
  crc = 0;
  for (;;) {
    const size_t av = in.fill(LOOK);
    if (!av) break;
    const U8* p = &in.buf[in.lo];
    int n = core_frame(p, av);
    if (n) {
      if (raw.size()) put_bytes(so, 'R', &raw[0], raw.size()), raw.clear();
      put_bytes(so, 'C', p, n);
      stt.core++;
      stt.s_core += n;
    } else if ((n = exss_frame(p, av))) {
      if (raw.size()) put_bytes(so, 'R', &raw[0], raw.size()), raw.clear();
      split_exss(p, n, st, L, have_layout, so, wo, stt, sp, chk, wv, wv2);
    } else {
      // bytes up to the next byte that could start a sync word
      n = 1;
      const size_t lim = av < 65536 ? av : 65536;
      while ((size_t)n < lim && p[n] != 0x7F && p[n] != 0x64) n++;
      raw.insert(raw.end(), p, p + n);
      stt.raw += n;
      if (raw.size() >= RAW_MAX) put_bytes(so, 'R', &raw[0], raw.size()), raw.clear();
    }
    crc = crc32_upd(crc, p, n);
    stt.bytes += n;
    in.lo += n;
  }
  if (raw.size()) put_bytes(so, 'R', &raw[0], raw.size());
  Rec z;
  z.u8('Z');
  z.uv(stt.bytes);
  z.uv(crc);
  z.uv(wo.frames);
  so.write(z);
  wo.finish();
  side = so.size;
  wav = WAV_HEAD + wo.nbytes + (wo.nbytes & 1);
  if (fi != stdin) fclose(fi);
  if ((so.f != stdout && fclose(so.f)) || fclose(wo.f)) die(EX_WRITE, "can't write the output");
}

// ---------------------------------------------------------------------------------------------
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

static void damaged(const char* why, U64 at) { die(EX_DATA, "damaged input: %s (side file byte %llu)", why, (unsigned long long)at); }

static void join(const char* spath, const char* wpath, Sink& out, U64& nbytes) {
  FILE* fs = open_in(spath, "the side file");
  FILE* fw = fopen(wpath, "rb");
  if (!fw) die(EX_READ, "can't open the WAV %s", wpath);
  WavIn wi;
  const char* why = wi.open(fw);
  if (why) die(EX_DATA, "%s: %s", wpath, why);
  RecIn in;
  in.init_file(fs);
  U8 h[9];
  in.bytes(h, 9);
  if (in.bad || memcmp(h, MAGIC, 8)) die(EX_DATA, "%s: not a dtssplit side file", spath);
  if (h[8] != VERSION) die(EX_DATA, "%s: format version %d, which this dtssplit (version %d) doesn't read", spath, h[8], VERSION);
  Layout L;
  bool have_layout = false;
  Split sp;
  std::vector<S32> wv;
  std::vector<U8> buf;
  WavSamples ws;
  ws.w = &wi;
  U32 crc = 0;
  U64 total = 0, frames = 0;
  for (;;) {
    const U64 at = in.nread;
    const int tag = in.u8();
    if (in.bad) damaged("the side file ends early", at);
    if (tag == 'R' || tag == 'C' || tag == 'E') {
      if (!in.blob(buf, tag == 'R' ? 2 * RAW_MAX : tag == 'C' ? 16384 : 1 << 20)) damaged("a damaged record", at);
    } else if (tag == 'W') {
      if (have_layout || !get_layout(in, L)) damaged("a damaged layout record", at);
      if (wi.nch != L.nch || wi.bits != L.bits || wi.rate != L.rate)
        die(EX_DATA, "%s: %d channels, %d-bit, %d Hz, where the side file has %d, %d-bit, %d Hz", wpath, wi.nch, wi.bits,
            wi.rate, L.nch, L.bits, L.rate);
      have_layout = true;
      continue;
    } else if (tag == 'X') {
      if ((why = join_x(in, have_layout ? &L : 0, ws, buf, sp, wv))) damaged(why, at);
      frames += sp.x.nframesamples;
    } else if (tag == 'Z') {
      const U64 n = in.uv(), c = in.uv(), f = in.uv();
      if (in.bad || !in.peek_end()) damaged("a damaged end record", at);
      if (n != total || c != crc) damaged("the stream joined isn't the one split", at);
      if (f != frames) damaged("a sample count unlike the WAV's", at);
      S32 extra;
      if (have_layout && wi.read(&extra, 1)) die(EX_DATA, "%s: more samples than the side file takes", wpath);
      break;
    } else {
      damaged("an unknown record", at);
    }
    if (buf.size()) out.write(&buf[0], buf.size());
    crc = crc32_upd(crc, buf.size() ? &buf[0] : 0, buf.size());
    total += buf.size();
  }
  if (fs != stdin) fclose(fs);
  fclose(fw);
  nbytes = total;
}

// ---------------------------------------------------------------------------------------------

static void usage(void) {
  fputs(
      "dtssplit - lossless split of DTS (DTS-HD Master Audio) streams and the join back\n"
      "Usage: dtssplit c [options] input.dts output.ac3 output.wav\n"
      "       dtssplit d [options] output.ac3 output.wav restored.dts\n"
      "  c    split: the lossy core and all else but the lossless extension's samples into\n"
      "       the side file (output.ac3), those samples into the WAV\n"
      "  d    join the side file and the WAV back into the stream, byte for byte\n"
      "  -v   (c) join the files written and compare the result with the input\n"
      "  -q   no statistics\n"
      "The input of c and the output of d can be - (stdin, stdout).\n",
      stderr);
  exit(EX_USAGE);
}

int main(int argc, char** argv) {
  crc_init();
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
  if (files.size() != 3) usage();
  if (cmode) {
    if (verify && (!strcmp(files[0], "-") || !strcmp(files[1], "-"))) die(EX_USAGE, "-v needs files, not -");
    Stats st;
    st.bytes = st.raw = st.core = st.exss = st.xsplit = st.wav_samples = st.exceptions = 0;
    st.s_core = st.s_kept = st.s_exss = st.s_xhdr = st.s_param = 0;
    U32 crc;
    U64 side, wav;
    split(files[0], files[1], files[2], st, crc, side, wav);
    if (!quiet) {
      fprintf(stderr, "%s: %llu bytes: %llu core frames, %llu extension substream frames (%llu with XLL split",
              files[0], (unsigned long long)st.bytes, (unsigned long long)st.core, (unsigned long long)st.exss,
              (unsigned long long)st.xsplit);
      for (size_t i = 0; i < st.why.size(); i++)
        fprintf(stderr, "; %llu kept as bytes: %s", (unsigned long long)st.why_n[i], st.why[i].c_str());
      fprintf(stderr, "), %llu bytes between frames\n", (unsigned long long)st.raw);
      fprintf(stderr,
              "%s: %llu bytes: core frames %llu, extension substream headers %llu, XLL headers %llu, XLL coding "
              "parameters %llu, frames kept as bytes %llu, the rest %llu\n",
              files[1], (unsigned long long)side, (unsigned long long)st.s_core, (unsigned long long)st.s_exss,
              (unsigned long long)st.s_xhdr, (unsigned long long)st.s_param, (unsigned long long)st.s_kept,
              (unsigned long long)(side - st.s_core - st.s_exss - st.s_xhdr - st.s_param - st.s_kept));
      fprintf(stderr, "%s: %llu bytes: %llu samples a channel, %llu values clipped\n", files[2], (unsigned long long)wav,
              (unsigned long long)st.wav_samples, (unsigned long long)st.exceptions);
    }
    if (verify) {
      CmpSink cs;
      cs.f = fopen(files[0], "rb");
      if (!cs.f) die(EX_READ, "can't open the input %s", files[0]);
      cs.at = 0;
      cs.diff = false;
      U64 n;
      join(files[1], files[2], cs, n);
      int more = fgetc(cs.f);
      fclose(cs.f);
      if (cs.diff || more != EOF || n != st.bytes) {
        fprintf(stderr, "verify: FAILED at byte %llu\n", (unsigned long long)cs.at);
        return EX_FAIL;
      }
      if (!quiet) fprintf(stderr, "verify: OK\n");
    }
  } else {
    FileSink fsk;
    fsk.f = open_out(files[2], "the output");
    U64 n;
    join(files[0], files[1], fsk, n);
    if (fsk.f != stdout ? fclose(fsk.f) : fflush(fsk.f)) die(EX_WRITE, "can't write the output");
  }
  return EX_OK;
}
