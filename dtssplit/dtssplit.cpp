// dtssplit - lossless split of a DTS stream into its lossy core, a WAV and a metainfo file, and the
// join back
//
// Usage: dtssplit c [options] input.dts prefix
//        dtssplit d [options] prefix restored.dts
//   c    split the stream into prefix.dtsc, prefix.wav and prefix.meta (and prefix.x.wav)
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
//   prefix.x.wav a DTS:X stream's object audio: the samples of the channel set its XLL frames carry
//                after their band data, as the WAV has the XLL's (written where there is any)
//   prefix.dtsx  a DTS:X stream's object data that isn't such a channel set, as it is (written where
//                there is any)
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
// DTS:X streams' XLL frames go on past the band data: their object data, from a sync word of its
// own at a DWORD boundary (0x02000850; 0xF14000D0 for IMAX's), which no published specification
// describes (FFmpeg 6 looks for the sync word only to name the profile; its master decodes what
// follows as the 7.1.4's heights, parse_x_heights()). In Nigel_Stanford_Cymatics_DTS-X.dts (96 MB
// of its 321) it is a header of 21 bytes with its CRC, the same in every frame, a byte, then one
// more XLL channel set, by the frame's common header: its sub-header (4 channels, 24-bit, without
// speaker positions: Lh, Rh, Lhr, Rhr), NAVI table and band data, then 0s to a DWORD boundary
// (obj_split()). The split decodes it as the XLL's own sets, its samples to prefix.x.wav (4
// channels, 24-bit) and the rest to the meta file; object data that isn't such a set goes to
// prefix.dtsx as it is, frame after frame. The join takes prefix.x.wav and prefix.dtsx only where
// the meta file says there is object data. The 7.1 bed has the heights in it, each at -3 dB in the
// channel under it (L, R, Lsr, Rsr: FFmpeg's master takes them out to make the 7.1.4); from version
// 22 the WAV has them out of those of the four coded without the core (xf_choose()), whose samples
// are the audio, where it is 32-bit, and the join puts them back as it reads it (XfSamples).
//
// The meta file: "DTSSPLIT", a version byte (VERSION's; the join reads the earlier ones too, back
// to 2, as VERSION's note says), then records, each a tag byte and its fields (numbers as LEB128
// varints, signed ones zigzagged). Its EXSS and XLL header bytes are the stream's but for what the
// join computes again: their CRC-16s are 0s, and the EXSS frame size, the XLL frame size, the
// channel set header sizes and the NAVI tables' sizes are XORed with those the join will compute
// (the frames' sizes, the bytes the headers' fields take and those each segment's coding takes),
// the prediction orders of a set of one band kept as their rank among the orders by ao_pred()'s
// cost (from version 17; before, from version 13, as their difference from its order, mod 16,
// zigzagged) and the reflection coefficients' codes as their table indexes' difference from
// rc_predict()'s (both from the WAV's samples, which the join reads before the header), so that
// they are mostly 0s too; so are the segments' Rice and binary code widths, kept as their
// difference from those their residuals give (ck_xor(); mod the field's size, zigzagged, from
// version 13), and the EXSS header's fields that hold the XLL frame's size or the DTS:X object
// data's offset or size, less those (an F record says where they are). The join computes the XLL
// frame's size and the object data's from their parts: where the bytes after the band data are 0s
// to the DWORD boundary, and those after the object data's band data 0s to the boundary past 2 more
// bytes (opad_std()), the meta file keeps neither:
//   R n bytes          bytes that aren't a frame
//   C n                a core frame: the next n bytes of prefix.dtsc
//   E n bytes          an extension substream frame kept as bytes
//   W w ...            WAV w's layout (before its first X): its channels, width and rate, the
//                      speaker mask, the XLL channel of each WAV channel, the channel sets, the bands
//   O 0 ... m          prefix.x.wav's layout, as W's (before the first X with object samples), and
//                      (from version 22) the bed channels with the heights folded out (bit i: L,
//                      R, Lsr, Rsr's)
//   F k (at q)...      the EXSS header's fields that hold the XLL frame's size (q 0), the DTS:X
//                      object data's offset in it (1) or its size (2): 16 bits from bit at each;
//                      the X records after it keep them less that (xf_apply())
//   X w f ...          an extension substream frame with its XLL frame split, its samples in WAV w:
//                      (from version 22, after an O record's m other than 0) whether the WAV has the
//                      heights folded out of the frame's bed, the EXSS bytes before the XLL frame,
//                      the speaker-mapping flag, the XLL
//                      header bytes (not their count: the join's parse of them gives it, as it does
//                      that of the object data's), the coding parameters of each segment of each
//                      channel set in each band, the XLL bytes after the band data up to the DTS:X
//                      object data (1 where the object data follows, plus 2 where they are 0s to
//                      the DWORD boundary, else plus their count times 4, and the bytes), then
//                      where there is object data whether it is split: if so its bytes before its
//                      band data, its segments' coding parameters, its bytes after its band data (0
//                      where they are opad_std()'s, else their count plus 1 and the bytes), its
//                      band 0's delayed samples, overlapping LSB bits and values clipped in
//                      prefix.x.wav (as the frame's below), its samples the next of prefix.x.wav;
//                      else its size, its bytes the next of prefix.dtsx; then the EXSS bytes after
//                      the XLL frame, band 0's delayed samples, the overlapping LSB bits, the WAV
//                      values written clipped
//   Z n crc k s... x o the end: the input's size and CRC-32, the WAVs and their samples a channel,
//                      the size of prefix.dtsx, prefix.x.wav's samples a channel
//
// From version 16 the records' parts go to the eight sections of a block (MS_REC...): the XLL
// header bytes, the object data's, the segments' coding parameters' flags and code widths, the
// frame's and the object data's apart, and their isolated samples and the rest, apart from the
// records; each block the sizes of its sections, then their bytes.
//
// Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
// Profiles), clauses 5, 7 and 8. The XLL band filter of two frequency bands, which the specification
// leaves out, is FFmpeg's (libavcodec/dcadsp.c, dcadata.c), so this file is licensed under the GNU
// Lesser General Public License, version 2.1 or later.
//
// Build: g++ -O2 -o dtssplit dtssplit.cpp
//        x86_64-w64-mingw32-g++ -O2 -s -static -o dtssplit.exe dtssplit.cpp

#include <math.h>
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

// the signed value of a code's unsigned word, and back (without branches: to a branch predictor the
// residuals' signs are noise)
static inline S32 unzig(U32 u) { return (S32)((u >> 1) ^ (U32)-(S32)(u & 1)); }
static inline U32 zig(S32 v) { return ((U32)v << 1) ^ (U32)-(S32)(v < 0); }
// a bits-bit field v against its prediction p, as the meta file keeps it (from version 13): their
// difference mod 2^bits, from -2^(bits-1), zigzagged; and v from that (dif_back())
static inline U32 dif_of(U32 v, U32 p, int bits) {
  const U32 m = (U32)1 << bits;
  const S32 d = (S32)((v - p + m / 2) & (m - 1)) - (S32)(m / 2);
  return zig(d) & (m - 1);
}
static inline U32 dif_back(U32 c, U32 p, int bits) { return (p + (U32)unzig(c)) & (((U32)1 << bits) - 1); }

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

// From version 16 the meta file after its version byte is blocks: each the sizes (LEB128) of its
// sections, then their bytes, a record's parts all in one block: the records (MS_REC), and apart from
// them the XLL headers' bytes (MS_HDR), the DTS:X object data's (MS_OHDR), the segments' coding
// parameters' flags (MS_PFLG), the object data's (MS_OPFLG), their code widths (MS_PWID, MS_OPWID),
// and their isolated samples and the rest (MS_PISO, from version 17 the object data's apart in
// MS_OPISO: version 16 has the eight before), which compress better so than mixed in together.
// META_BLOCK: the bytes held after which the split ends a block
enum { MS_REC, MS_HDR, MS_OHDR, MS_PFLG, MS_OPFLG, MS_PWID, MS_OPWID, MS_PISO, MS_OPISO, MS_AC, MS_HAC, MS_N,
       MS_N16 = MS_OPISO, MS_N17 = MS_AC, MS_N18 = MS_HAC, META_BLOCK = 1 << 24 };
static const int MS_PAR[3] = {MS_PFLG, MS_PWID, MS_PISO}, MS_OPAR[3] = {MS_OPFLG, MS_OPWID, MS_OPISO},
                 MS_OPAR16[3] = {MS_OPFLG, MS_OPWID, MS_PISO};

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
  // the next n bytes into p without reading them: how many there are (fewer only at the input's end)
  size_t peek(U8* p, size_t n) {
    if (!f) {
      const size_t k = msize - mpos < n ? msize - mpos : n;
      memcpy(p, mem + mpos, k);
      return k;
    }
    if (hi - lo < n) {
      if (buf.size() < n) buf.resize(n);
      memmove(&buf[0], &buf[lo], hi - lo);
      hi -= lo;
      lo = 0;
      while (hi < n) {
        const size_t g = fread(&buf[hi], 1, buf.size() - hi, f);
        if (!g) {
          if (ferror(f)) die(EX_READ, "can't read %s", what);
          break;
        }
        hi += g;
      }
    }
    const size_t k = hi - lo < n ? hi - lo : n;
    memcpy(p, &buf[lo], k);
    return k;
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
static const U32 SYNC_XLL_X = 0x02000850u, SYNC_XLL_X_IMAX = 0xF14000D0u;  // (DTS:X's object data)

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

// n bits of p from bit pos, and an XOR of them
static U32 bits_at(const U8* p, int pos, int n) {
  U32 v = 0;
  for (int i = 0; i < n; i++) v = v << 1 | (p[(pos + i) >> 3] >> (7 - ((pos + i) & 7)) & 1);
  return v;
}
static void bits_xor(U8* p, int pos, int n, U32 v) {
  for (int i = 0; i < n; i++)
    if (v >> (n - 1 - i) & 1) p[(pos + i) >> 3] ^= (U8)(0x80 >> ((pos + i) & 7));
}

// The EXSS frame header in e (len bytes of it, the frame n bytes): its frame size XORed with the
// frame's (less 1) and its CRC-16 zeroed, as the meta file keeps them (put: 1); or back, as the
// join puts them (put: 0)
static void exss_fields(U8* e, size_t len, int n, int put) {
  if (len < 8) return;
  const int wide = bits_at(e, 42, 1), hw = wide ? 12 : 8, fw = wide ? 20 : 16;
  const int hs = (int)bits_at(e, 43, hw) + 1;
  if (hs < 8 || (size_t)hs > len) return;
  bits_xor(e, 43 + hw, fw, (U32)(n - 1) & ((1u << fw) - 1));
  if (put) {
    e[hs - 2] = e[hs - 1] = 0;
  } else {
    const U16 c = crc16(e + 5, hs - 7);
    e[hs - 2] = (U8)(c >> 8);
    e[hs - 1] = (U8)c;
  }
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
// the largest XLL frame the join of a version 7 meta file takes (which computes their sizes; the
// split keeps larger ones as bytes, its check failing)
enum { XLL_MAX = 1 << 16 };

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
  int rcat[XCH];  // where a channel's reflection coefficients' codes start in the header (bits)
  int aoat;       // where the prediction orders start in the header (bits)
  int ordat, dcat[XCH / 2];  // where the channel order starts, each pair's coefficient (bits; -1: none)
  int dfat[XCH / 2];         // where each pair's flag for a coefficient is (bits)
  int dmix_embedded;
  int lsb_size, nlsb[XCH], adj[XCH];  // lsb_size: the bytes at the end of a segment's data the LSB part takes
};

struct Chset {  // a channel set: its sub-header, and the coding parameters as they go from segment to segment
  int nch, resenc, pcm_res, storage_res, freq, nbands;
  int primary, dmix_coeffs, dmix_embedded, dmix_type, hier, chmask, hier_ofs, nabits;
  Band band[XBAND];
  int seg_common, rice[XCH], aux[XCH], abit_a[XCH], abit_b[XCH], na[XCH];
};

// a field of an XLL header as xll_headers() reads it: where, its bits, its key (HdrReader::label())
struct HField {
  int pos, w;
  U32 key;
};
struct Xll {
  int csoff[XCS], csnat[XCS];  // each channel set header's offset, and the size its fields take (with the CRC)
  int hsize, frame_size, nchsets, nsegs, nseg_log2, nsegsamples, nframesamples, seg_nbits, band_crc, scalable,
      chmask_nbits, fixed_lsb, nbands;
  int nch;  // all channel sets'
  int chofs[XCS + 1];
  Chset cs[XCS];
  std::vector<int> navi;  // band data sizes in bytes: [band][segment][channel set]
  int data_start;         // where the band data starts
  int ncrc, crcat[XCS + 2];  // where the headers' CRC-16s are (their 2 bytes; the meta file has 0s there)
  int navi_at;               // where the NAVI table starts (its sizes against navi_pred()'s in the meta file)
  std::vector<HField> flog;  // the header's fields, as its parse read them (from version 19 put_hdr()'s)
};

// a segment of a channel set's band data: what isn't samples
struct Chunk {
  U8 reuse, seg_common;
  U8 rice[XCH], hyb[XCH], aux_code[XCH], a_code[XCH], b_code[XCH];
  std::vector<U16> iso[XCH];  // per channel, where hybrid Rice codes put their isolated samples
  U8 isopred[XCH] = {};       // (from version 11) the isolated samples are those ck_xor()'s rule says
  U8 g1, g2;                  // the bits after the MSB part and after the LSB part: 0 zeros, 1 as kept
  U32 g1n, g2n;
  U32 mbits;                  // the MSB part's bits (the split's)
  std::vector<U8> g1b, g2b;
  int dh_code;     // band 1, segment 0: the decimator history's width (less 1)
  S32 dh[XCH][XDH];  // and its samples, [1-7] of each channel
};

static inline S32 sat23(S32 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : a; }
static inline S32 mul16(S32 a, S32 b) { return (S32)(((S64)a * b + (1 << 15)) >> 16); }
static inline S32 mulq(S32 a, S32 b, int q) { return (S32)(((S64)a * b + (1LL << (q - 1))) >> q); }

// the CRC-16 of the bytes of p from a to b, its last 2: whether it holds (fix: written there instead,
// where the join puts back the meta file's 0s); where it is, into x
static bool crc_here(Xll& x, const U8* p, U8* fix, int a, int b) {
  if (x.ncrc < XCS + 2) x.crcat[x.ncrc++] = b - 2;
  if (!fix) return !crc16(p + a, b - a);
  const U16 c = crc16(p + a, b - a - 2);
  fix[b - 2] = (U8)(c >> 8);
  fix[b - 1] = (U8)c;
  return true;
}

// the common header, the channel set sub-headers and the NAVI table of the XLL frame p (n bytes,
// PAD more): 0, or why they can't be followed; with base, p is DTS:X object data (obj_split()), its
// channel set's sub-header at off0 and the frame's common header base's; with fix (p itself), the
// CRC-16s are written rather than checked (the meta file keeps them as 0s); lax: a reflection
// coefficient's code out of range taken as 0 (the meta file keeps the codes against rc_predict()'s)
// The prediction orders' rule (ao_pred()): its state, the counts of the candidates' hits ([channel
// set, the DTS:X object data's last][channel]; from version 14 (v14) all of them halved where one
// passes AO_LIM), and what a frame's prediction needs
enum { AO_NL = 12, AO_NF = 5, AO_NR = AO_NL * AO_NF, AO_K = 10, AO_DEF = 2 * AO_NL + 3, AO_LIM = 64 };
struct AoState {
  U32 n[XCS + 1][XCH][AO_NR];
  int v14, v15, v17;  // (v17: the orders kept as their rank under the rule's cost, from version 17)
  int v18;            // (v18: the ranks and the pairs' flags in MS_AC, from version 18)
  int v23;            // (v23: the pairs' flags by the decorrelation's gain after the prediction, dc_gain(), the
                      // ranks by the two cheapest orders, ac_rx())
  int v24;            // (v24: the orders' rule on past where Levinson's recursion breaks down, ao_pred())
  int v25;            // (v25: the pairs' flags by the bits decorrelating would save, dc_bits())
  int v27;            // (v27: the orders' rule with the frame's first samples at the signal's variance, ao_pred())
  U8 dcp[XCS + 1][XCH / 2];  // (from version 18: each pair's flag in its set's last frame, ac_dcf()'s)
};

// The arithmetic-coded section (MS_AC, from version 18): binary decisions, each by an adaptive
// probability of its context's, through a range coder (LZMA's, with 12-bit probabilities). The
// probabilities are a block's from its start, and move 1/32 of the way to a decision once the
// record it is in is through (AcModel::commit()): the split codes a record's decisions once the
// check has joined it back, and the join can parse a header again (AcIo::mark(), rewind()).
// Contexts: a prediction order's rank, 0 or not, then 1 or not, by its set, the bucket of the
// rule's cost between its two cheapest orders and from version 23 those two orders (ac_rx())
// (ac_rank_put()); ranks from 2 as 4 bits by the set; a pair's flag for a coefficient by its set,
// the pair, the flags of the 3 pairs before it in the frame, its own in the set's last frame, the
// size of the coefficient dc_code() gives it and from version 23 what decorrelating by it would
// save after the prediction (dc_gain()) (ac_dcf()). From version 23 a rank's first two decisions'
// contexts start from their parent's estimate, the context but for ac_rx(), which each of their
// decisions moves too (ac_par(), HacModel::commit()).
enum { AC_RX = 36, AC_RK0 = 0, AC_RK1 = AC_RK0 + (XCS + 1) * 16 * AC_RX, AC_RKT = AC_RK1 + (XCS + 1) * 16 * AC_RX,
       AC_DCF = AC_RKT + (XCS + 1) * 16, AC_NG = 36, AC_RP0 = AC_DCF + (XCS + 1) * 8 * 8 * 2 * 4 * AC_NG,
       AC_RP1 = AC_RP0 + (XCS + 1) * 16, AC_NCTX = AC_RP1 + (XCS + 1) * 16 };
static U32 ac_par(U32 c) {
  if (c >= AC_RKT) return c;
  return c < AC_RK1 ? AC_RP0 + (c - AC_RK0) / AC_RX : AC_RP1 + (c - AC_RK1) / AC_RX;
}
struct AcModel {
  U16 p[AC_NCTX];         // P(0) in 4096ths
  std::vector<U32> pend;  // the record's decisions (context << 1 | bit), for commit()
  void reset() {
    for (int i = 0; i < AC_NCTX; i++) p[i] = 2048;
    pend.clear();
  }
  void commit() {
    for (size_t i = 0; i < pend.size(); i++) {
      U16& q = p[pend[i] >> 1];
      if (pend[i] & 1) q = (U16)(q - (q >> 5));
      else q = (U16)(q + ((4096 - q) >> 5));
    }
    pend.clear();
  }
};
struct AcEnc {
  U64 low, csize;
  U32 range;
  U8 cache;
  std::vector<U8> out;
  void reset() {
    low = 0, csize = 1, range = 0xFFFFFFFFu, cache = 0;
    out.clear();
  }
  void shift() {
    if ((U32)low < 0xFF000000u || (low >> 32)) {
      U8 c = cache;
      do {
        out.push_back((U8)(c + (U8)(low >> 32)));
        c = 0xFF;
      } while (--csize);
      cache = (U8)(low >> 24);
    }
    csize++;
    low = (low & 0x00FFFFFFu) << 8;
  }
  void bit(U32 p0, int b) {
    const U32 bound = (range >> 12) * p0;
    if (b) low += bound, range -= bound;
    else range = bound;
    while (range < (1u << 24)) range <<= 8, shift();
  }
  void bit16(U32 p0, int b) {
    const U32 bound = (range >> 16) * p0;
    if (b) low += bound, range -= bound;
    else range = bound;
    while (range < (1u << 24)) range <<= 8, shift();
  }
  void finish() {
    for (int i = 0; i < 5; i++) shift();
  }
};
struct AcDec {
  const U8* d;
  size_t n, pos;
  U32 range, code;
  U32 next() { return pos < n ? d[pos++] : 0; }  // (0s past the end: a damaged section says nothing)
  void init(const U8* p, size_t len) {
    d = p, n = len, pos = 0, range = 0xFFFFFFFFu, code = 0;
    for (int i = 0; i < 5; i++) code = code << 8 | next();
  }
  int bit(U32 p0) {
    const U32 bound = (range >> 12) * p0;
    int b;
    if (code < bound) range = bound, b = 0;
    else code -= bound, range -= bound, b = 1;
    while (range < (1u << 24)) range <<= 8, code = code << 8 | next();
    return b;
  }
  int bit16(U32 p0) {  // (from version 19 MS_HAC's: P(0) in 65536ths)
    const U32 bound = (range >> 16) * p0;
    int b;
    if (code < bound) range = bound, b = 0;
    else code -= bound, range -= bound, b = 1;
    while (range < (1u << 24)) range <<= 8, code = code << 8 | next();
    return b;
  }
};
// From version 19 the range coders' model (each section's its own): each context's probability by
// the decisions it has had, as many as lim of them (Krichevsky-Trofimov's estimate, then a running
// one), a new one's its parent's (1/2: no decision trains a parent). Version 19's lim is HC_LIM;
// from version 20 MS_AC's is HC_LIMA, MS_HAC's HC_LIMH and the coding parameters' HC_LIMC: they
// all came out smaller the shorter their memory, to those (MS_AC's 22239 bytes of the Cymatics
// stream at 4096, 22092 at 30; MS_HAC's 16228 at 4096, 13146 at 14 (the rules of version 19); the
// coding parameters' 21536 at 4096, 19767 at 30)
enum { HC_N = 1 << 18, HC_NA = 1 << 19, HC_LIM = 4096, HC_LIMA = 30, HC_LIMC = 30, HC_LIMH = 14 };
struct HacModel {
  int lim;    // (the decisions a context's estimate counts)
  U32 size;  // (its contexts: HC_N, MS_AC's HC_NA, its contexts not hashed)
  HacModel() : lim(HC_LIM), size(HC_N) {}
  std::vector<U32> p;  // P(0) in 2^-24ths
  std::vector<U16> n;
  std::vector<U64> pend;  // the record's decisions (context | parent << 24 | bit << 48 | the parent's too << 49)
  void reset() {
    p.assign(size, 1u << 23);
    n.assign(size, 0);
    pend.clear();
  }
  U32 p16(U32 c, U32 par) const {
    const U32 q = (n[c] ? p[c] : p[par]) >> 8;
    return q < 1 ? 1 : q > 65535 ? 65535 : q;
  }
  void commit() {
    for (size_t i = 0; i < pend.size(); i++) {
      const U32 c = (U32)(pend[i] & 0xFFFFFF), par = (U32)(pend[i] >> 24 & 0xFFFFFF);
      if (!n[c]) p[c] = p[par];
      move(c, (int)(pend[i] >> 48 & 1));
      if (pend[i] >> 49 & 1) move(par, (int)(pend[i] >> 48 & 1));
    }
    pend.clear();
  }
  void move(U32 c, int b) {
    const S64 d = (b ? 0 : (S64)1 << 24) - (S64)p[c];
    p[c] = (U32)((S64)p[c] + d / (n[c] + 2));
    if (n[c] < lim) n[c]++;
  }
};
// the decisions of a record's headers: the split's (put(): to its tape), the check's (get(): back
// from that tape, each in the context it was put in) and the join's (get(): from the section)
struct AcIo {
  std::vector<U32>* put_to;
  const std::vector<U32>* tape;
  AcDec* dec;
  AcModel* model;
  HacModel* kmodel;  // (from version 19 the join's, in model's place)
  bool par;          // (from version 23 the contexts' parents, ac_par())
  AcDec mdec;
  size_t pos, mpos, mpend;
  bool bad, mbad;
  void init(std::vector<U32>* w, const std::vector<U32>* t, AcDec* d, AcModel* m) {
    put_to = w, tape = t, dec = d, model = m, kmodel = 0, par = false, pos = mpos = mpend = 0, bad = mbad = false;
  }
  void put(U32 ctx, int b) {
    if (put_to) put_to->push_back(ctx << 1 | (U32)b);
  }
  int get(U32 ctx) {
    if (dec && kmodel) {
      const U32 pa = par ? ac_par(ctx) : ctx;
      const int b = dec->bit16(kmodel->p16(ctx, pa));
      kmodel->pend.push_back((U64)ctx | (U64)pa << 24 | (U64)b << 48 | (U64)(pa != ctx) << 49);
      return b;
    }
    if (dec) {
      const int b = dec->bit(model->p[ctx]);
      model->pend.push_back(ctx << 1 | (U32)b);
      return b;
    }
    if (!tape || pos >= tape->size() || ((*tape)[pos] >> 1) != ctx) {
      bad = true;
      return 0;
    }
    return (int)((*tape)[pos++] & 1);
  }
  void mark() {
    mpos = pos, mbad = bad;
    if (dec) mdec = *dec, mpend = kmodel ? kmodel->pend.size() : model->pend.size();
  }
  void rewind() {
    pos = mpos, bad = mbad;
    if (dec) {
      *dec = mdec;
      if (kmodel) kmodel->pend.resize(mpend);
      else model->pend.resize(mpend);
    }
  }
};
// (the cheapest order in 6 buckets (0, 1, 2, 3-4, 5-7, 8 up), how far the next cheapest is from it
// (1, 2, more) and on which side)
static int ac_rx(const U8* rank) {
  const int o = rank[0], d = rank[1] - rank[0], a = d < 0 ? -d : d;
  const int ob = o < 3 ? o : o < 5 ? 3 : o < 8 ? 4 : 5;
  return (ob * 3 + (a < 2 ? 0 : a < 3 ? 1 : 2)) * 2 + (d > 0);
}
static void ac_rank_put(AcIo& io, int set, int mb, int rx, int r) {
  const int k = (set * 16 + mb) * AC_RX + rx;
  io.put(AC_RK0 + k, r != 0);
  if (!r) return;
  io.put(AC_RK1 + k, r != 1);
  if (r == 1) return;
  for (int i = 3, node = 1; i >= 0; i--) {
    const int b = (r - 2) >> i & 1;
    io.put(AC_RKT + set * 16 + node, b);
    node = node * 2 + b;
  }
}
static int ac_rank_get(AcIo& io, int set, int mb, int rx) {
  const int k = (set * 16 + mb) * AC_RX + rx;
  if (!io.get(AC_RK0 + k)) return 0;
  if (!io.get(AC_RK1 + k)) return 1;
  int node = 1;
  for (int i = 0; i < 4; i++) node = node * 2 + io.get(AC_RKT + set * 16 + node);
  return (node - 16 + 2) & 15;  // (14 and 15 none are put as; a damaged section's, 0 and 1)
}
static U32 ac_dcf(int set, int i, int hist, int last, int pc, int gb) {
  const int a = pc < 0 ? -pc : pc;
  return AC_DCF + (U32)(((((set * 8 + (i < 7 ? i : 7)) * 8 + (hist & 7)) * 2 + (last & 1)) * 4 + (a < 3 ? a : 3)) * AC_NG + gb);
}

// From version 19 the meta file keeps the XLL headers as their fields, range-coded (MS_HAC) as
// xll_headers() reads them (HdrReader), after the XORs put_hdr() makes; then the bits no field takes
// (hdr_gaps()). A field's key is what it is (HdrReader::label()): its part of the header, channel set,
// sub-part (a band, a channel) and place there (from 0, the 255th and on alike), and the frame's or
// the object data's. Each is coded as whether it is the value the same key had last (the last frame
// that had it: HdrState, a frame's values the next frame's to go by), in the context of the key and
// of how many of the header's fields so far weren't (0, 1, more); if not, whether it is 0 (where the
// last wasn't); if not, its bits: by a binary tree for up to 8 of them, else MSB first by place and
// whether a 1 came before, in the contexts of the part (the key less its place) and the width. The
// contexts are hashed into HC_N of them, a field's first decision by its part's.
enum { HL_FRM = 1, HL_SET, HL_BAND, HL_DCP, HL_AO, HL_FO, HL_RC, HL_LSB, HL_NAVI, HL_GAP, HL_CHK, HL_WID };
static inline U32 hmix(U32 a, U32 b) {
  U32 h = a * 0x9E3779B1u ^ (b * 0x85EBCA77u + 0x632BE5ABu);
  h ^= h >> 16;
  h *= 0x7FEB352Du;
  h ^= h >> 15;
  h *= 0x846CA68Bu;
  h ^= h >> 16;
  return h;
}
static inline U32 hctx(U32 a, U32 b) { return hmix(a, b) & (HC_N - 1); }
// MS_HAC's decisions: the split's (to its tape), the check's (back from it, each in the context it
// was put in) and the join's (from the section)
struct HacIo {
  std::vector<U64>* put_to;
  const std::vector<U64>* tape;
  AcDec* dec;
  HacModel* model;
  size_t pos;
  bool bad;
  void init(std::vector<U64>* w, const std::vector<U64>* t, AcDec* d, HacModel* m) {
    put_to = w, tape = t, dec = d, model = m, pos = 0, bad = false;
  }
  void put(U32 c, U32 par, int b, bool sh = false) {  // (sh: from version 26 the decision trains par too)
    if (put_to) put_to->push_back((U64)c | (U64)par << 24 | (U64)b << 48 | (U64)(sh && par != c) << 49);
  }
  int get(U32 c, U32 par, bool sh = false) {
    const U64 e = (U64)c | (U64)par << 24;
    if (dec) {
      const int b = dec->bit16(model->p16(c, par));
      model->pend.push_back(e | (U64)b << 48 | (U64)(sh && par != c) << 49);
      return b;
    }
    if (!tape || pos >= tape->size() || ((*tape)[pos] & 0xFFFFFFFFFFFFull) != e) {
      bad = true;
      return 0;
    }
    return (int)((*tape)[pos++] >> 48 & 1);
  }
};
enum { HS_N = 1 << 16 };
struct HdrState {
  std::vector<U32> last;                    // the keys' last values (hashed into HS_N)
  std::vector<std::pair<U32, U32> > pend;  // a frame's, once it is through (hdr_commit())
  int v26;       // (v26: the common header's size and segment fields by HdrCoder::fcode())
  U32 lfs, pfs;  // (v26: the last XLL frame's size, and that of the one being joined, join_x())
  U32 nfr;       // (v26: the frames through)
  HdrState() : v26(0), lfs(0), pfs(0), nfr(0) {}
  void reset() {
    last.assign(HS_N, 0);
    pend.clear();
    lfs = pfs = nfr = 0;
  }
};
static void hdr_commit(HdrState& s) {
  for (size_t i = 0; i < s.pend.size(); i++) s.last[s.pend[i].first] = s.pend[i].second;
  s.pend.clear();
  s.lfs = s.pfs;
  s.nfr++;
}
struct HdrCoder {
  HacIo* io;
  HdrState* st;
  U32 obj;  // (1: the DTS:X object data's header)
  int chg;  // the header's fields so far unlike their last
  U32 f3, f6;  // (v26: the frame's frame_size_nbits-1 and nframesegs_log2, once coded)
  void start(HacIo* i, HdrState* s, bool o) { io = i, st = s, obj = o, chg = 0, f3 = f6 = 0; }
  int bit(U32 c, U32 par, int b, bool put, bool sh = false) {
    if (put) {
      io->put(c, par, b, sh);
      return b;
    }
    return io->get(c, par, sh);
  }
  // the value of the field key (n bits, 1-32): put, v (the split's), or got
  U32 code(U32 key, int n, U32 v, bool put) {
    const U32 k = key | obj << 20, part = k & ~0xFFu, slot = hmix(k, 0) & (HS_N - 1);
    const U32 last = n < 32 ? st->last[slot] & ((1u << n) - 1) : st->last[slot];
    if (st->v26 && part == (U32)HL_FRM << 16 && n <= 5 && ((k & 0xFF) == 3 || ((k & 0xFF) >= 6 && (k & 0xFF) <= 8))) {
      v = fcode(k, n, v, put, last);
      if (v != last) chg++;
      if (!put) st->pend.push_back(std::make_pair(slot, v));
      return v;
    }
    const U32 cc = 1 + (U32)(chg < 2 ? chg : 2);
    if (bit(hctx(k, cc), hctx(part, cc), v == last, put)) {
      v = last;
    } else {
      chg++;
      if (last && bit(hctx(k, 4), hctx(part, 4), v == 0, put)) {
        v = 0;
      } else {
        const U32 g = part | (U32)n << 24;
        U32 x = 0;
        if (n <= 8) {
          U32 node = 1;
          for (int i = n - 1; i >= 0; i--) {
            const U32 c = hctx(g, 0x100 + node);
            node = node << 1 | (U32)bit(c, c, (int)(v >> i & 1), put);
          }
          x = node - (1u << n);
        } else {
          for (int i = n - 1; i >= 0; i--) {
            const U32 c = hctx(g, 0x1000 + (U32)i * 2 + (x != 0));
            x = x << 1 | (U32)bit(c, c, (int)(v >> i & 1), put);
          }
        }
        v = x;
      }
    }
    if (!put) st->pend.push_back(std::make_pair(slot, v));
    return v;
  }
  U32 tree(U32 k, U32 base, int n, U32 v, bool put) {  // (v, n bits, by a binary tree in the contexts k, base)
    U32 node = 1;
    for (int i = n - 1; i >= 0; i--) {
      const U32 c = hctx(k, base + node);
      node = node << 1 | (U32)bit(c, c, (int)(v >> i & 1), put);
    }
    return node - (1u << n);
  }
  // (from version 26, past a stream's first 1024 frames: before, code()'s) a parameter set's flags
  // against fp_set()'s (key kf, v: f1), the Rice one in the context of the part, of the bucket of how
  // many bits the rule's choice of it was by (br, fp_bucket()) and of the key's last value, the hybrid
  // one of the hybrid code's (bh) and of the Rice one's; each parent the context less the last value,
  // trained by the decisions it is the parent of. (On a short stream, code()'s contexts, the key's
  // own, make the range coder's bytes runs of 0xFF that xz takes in a few bytes)
  U32 f1code(U32 kf, U32 v, bool put, U32 br, U32 bh) {
    const U32 k = kf | obj << 20, part = k & ~0xFFu, slot = hmix(k, 0) & (HS_N - 1), last = st->last[slot] & 3;
    const int b0 = bit(hctx(part, 0x70000 + (br * 4 + last) * 2), hctx(part, 0x70200 + br), (int)(v & 1), put, true);
    const U32 c1 = (bh * 2 + (U32)b0) * 4 + last;
    const int b1 = bit(hctx(part, 0x70400 + c1), hctx(part, 0x70600 + bh * 2 + (U32)b0), (int)(v >> 1 & 1), put, true);
    v = (U32)b0 | (U32)b1 << 1;
    if (v != last) chg++;
    if (!put) st->pend.push_back(std::make_pair(slot, v));
    return v;
  }
  U32 flast(U32 f, int n) const { return st->last[hmix((U32)HL_FRM << 16 | f, 0) & (HS_N - 1)] & ((1u << n) - 1); }
  // (from version 26) the common header's field k (n bits; last: its last value): frame_size_nbits (3),
  // nframesegs_log2 (6), nsegsamples_log2 (7) or seg_size_nbits (8), each less 1 but 6 and 7. Where
  // the last frame's size was in its binade (pos: the 4 bits below its top one, HdrState::lfs) tells
  // how near the next is: 3 is whether it is the last's (in the context of pos), else the one past it
  // toward the nearer end, else its bits by a binary tree. 6 is whether it is the last's, in the
  // context of the last one and of whether 3 went up, down or not, else the other of 1 and 2, else a
  // binary tree (the encoders' segment count follows no rule of the residuals' bits that we found:
  // a third of the Cymatics stream's frames take the fewest bits so). 7 by whether the frame's samples
  // are the last frame's (the last 6 and 7 less this 6), else a binary tree. 8 by whether it is 3's
  // less 1, else 3's (the encoders' width of a segment's size is near that of the frame's), in the
  // context of pos, of 3's change and of the last frame's 8 less its 3; else a binary tree. Each first
  // and second decision's parent is its context less pos (or 3's change), trained by it.
  U32 fcode(U32 k, int n, U32 v, bool put, U32 last) {
    const U32 f = k & 0xFF, m = (1u << n) - 1;
    const U32 lv = st->lfs ? st->lfs - 1 : 0;
    int kb = 0;
    while (kb < 32 && (lv >> kb)) kb++;
    const U32 pos = kb > 4 ? lv >> (kb - 5) & 15 : 0;
    const U32 l3 = flast(3, 5), ch = f3 > l3 ? 2 : f3 < l3 ? 0 : 1;
    if (f == 3) {
      const U32 a = (pos >= 8 ? last + 1 : last - 1) & m, c = 0x50000 + pos * 4;
      if (bit(hctx(k, c), hctx(k, 0x50100), v == last, put, true)) {
        v = last;
      } else if (bit(hctx(k, c + 1), hctx(k, 0x50101), v == a, put, true)) {
        v = a;
      } else {
        v = tree(k, 0x50200, n, v, put);
      }
      f3 = v;
    } else if (f == 6) {
      const U32 a = last == 1 ? 2 : 1, c = 0x51000 + ((last & 15) * 3 + ch) * 2, pc = 0x51100 + (last & 15) * 2;
      if (bit(hctx(k, c), hctx(k, pc), v == last, put, true)) {
        v = last;
      } else if (bit(hctx(k, c + 1), hctx(k, pc + 1), v == a, put, true)) {
        v = a;
      } else {
        v = tree(k, 0x51200 + (last & 15) * 32, n, v, put);
      }
      f6 = v;
    } else if (f == 7) {
      const U32 p = (last + flast(6, 4) - f6) & m;
      if (bit(hctx(k, 0x52000), hctx(k, 0x52000), v == p, put)) {
        v = p;
      } else {
        v = tree(k, 0x52100, n, v, put);
      }
    } else {
      const int lm = (int)last - (int)l3;
      const U32 lb = lm < -2 ? 0 : lm > 2 ? 4 : (U32)(lm + 2), c = 0x53000 + ((pos * 3 + ch) * 5 + lb) * 4;
      const U32 p = (f3 - 1) & m, a = f3 & m, pc = 0x53600 + (ch * 5 + lb) * 4;
      if (bit(hctx(k, c), hctx(k, pc), v == p, put, true)) {
        v = p;
      } else if (bit(hctx(k, c + 1), hctx(k, pc + 1), v == a, put, true)) {
        v = a;
      } else {
        v = tree(k, 0x53800, n, v, put);
      }
    }
    return v;
  }
  // (from version 20) a code width's difference from its rule's (ck_xor(); key, n bits), in the
  // context m of the part, the code and how near the rule's width is to the next ones by their bits
  // (w_margin()): whether it is 0, then its bits by a binary tree
  U32 wcode(U32 key, int n, U32 v, bool put, U32 m) {
    const U32 k = key | obj << 20, lab = (k & 0xF0000u) | obj << 20;
    if (bit(hctx(k, 0x20000 + m), hctx(lab, 0x20000 + m), v == 0, put)) return 0;
    const U32 g = lab | (U32)n << 24;
    U32 node = 1;
    for (int i = n - 1; i >= 0; i--) {
      const U32 c = hctx(g, 0x40000 + (m << 8) + node);
      node = node << 1 | (U32)bit(c, c, (int)(v >> i & 1), put);
    }
    return node - (1u << n);
  }
};

// xll_headers()'s reader: each field it reads into log, by its key; from version 19 the join's
// takes them from MS_HAC (src) and writes them into wr as it reads them
struct HdrReader : BitReader {
  std::vector<HField>* log;
  HdrCoder* src;
  U8* wr;
  U32 lab;
  int ord;
  void label(int l, int c, int sub = 0, int ord0 = 0) {
    lab = (U32)l << 16 | (U32)(c & 15) << 12 | (U32)(sub & 15) << 8;
    ord = ord0;
  }
  U32 get(int n) {  // n 0-32
    if (n <= 0) return 0;
    if (pos + n > end) {
      over = true;
      pos = end;
      return 0;
    }
    const U32 key = lab | (U32)(ord < 255 ? ord : 255);
    ord++;
    if (log) {
      const HField f = {(int)pos, n, key};
      log->push_back(f);
    }
    if (!src) return BitReader::get(n);
    const U32 v = src->code(key, n, 0, false);
    bits_xor(wr, (int)pos, n, v);  // (0s there)
    pos += n;
    return v;
  }
  void skip(U64 n) {  // (as fields of up to 8 bits)
    if (pos + n > end) {
      over = true;
      pos = end;
      return;
    }
    for (U64 i = 0; i < n; i += 8) get((int)(n - i < 8 ? n - i : 8));
  }
};

// the bits of the header x (its first nb bytes) that neither a field takes nor a CRC-16 is in: each
// run of them as fields of up to 8 bits (keyed HL_GAP's: the run, the byte in it)
static void hdr_gaps(const Xll& x, int nb, std::vector<HField>& g) {
  g.clear();
  if (nb <= 0) return;
  std::vector<U8> cov((size_t)nb * 8, 0);
  for (size_t i = 0; i < x.flog.size(); i++)
    for (int k = 0; k < x.flog[i].w; k++)
      if (x.flog[i].pos >= 0 && (size_t)(x.flog[i].pos + k) < cov.size()) cov[(size_t)(x.flog[i].pos + k)] = 1;
  for (int i = 0; i < x.ncrc; i++)
    for (int k = 0; k < 16; k++)
      if (x.crcat[i] >= 0 && (size_t)x.crcat[i] * 8 + k < cov.size()) cov[(size_t)x.crcat[i] * 8 + k] = 1;
  int run = 0;
  for (size_t i = 0; i < cov.size();) {
    if (cov[i]) {
      i++;
      continue;
    }
    size_t j = i;
    while (j < cov.size() && !cov[j]) j++;
    const U32 r = (U32)(run < 255 ? run : 255);
    for (size_t k = i; k < j; k += 8) {
      const U32 b = (U32)((k - i) / 8 < 255 ? (k - i) / 8 : 255);
      const HField f = {(int)k, (int)(j - k < 8 ? j - k : 8), (U32)HL_GAP << 16 | (r >> 4) << 12 | (r & 15) << 8 | b};
      g.push_back(f);
    }
    run++;
    i = j;
  }
}
struct AoCtx {
  const S32* val;  // the frame's samples as the WAV has them ([XLL channel][sample]): 0, none
  int nch, N;      // its channels, samples a channel
  AoState* st;
  __int128 S[XCH][XCH];  // (dc_order()'s: the products of a channel set's channels)
  int np;  // the join's candidates for the counts once the frame is through (ao_count())
  struct {
    U8 set, oc, pos, act, out[AO_NR];
  } pend[XCS * XCH];
  AcIo* io;  // (from version 18: the headers' decisions in MS_AC)
  HacIo* hio;    // (from version 19: the headers' fields in MS_HAC)
  HdrState* hst;
  const char* (*pre)(void*, const Xll&);  // (from version 19 the join's: the samples, once the common header has their count)
  void* pre_arg;
  int ndp;   // (and the pairs' flags for AoState::dcp once the frame is through)
  struct {
    U8 set, i, f;
  } dpend[XCS * XCH / 2];
};
static void ao_pred(AoCtx& ac, int set, int g0, const Chset& s, const Band& d, int* pred, bool keep, U8 (*rank)[16] = 0,
                    U8* mbk = 0);
static int dc_gain(const AoCtx& ac, int g0, int a, int b, int pc);
static void dc_order(AoCtx& ac, int g0, int nch, int* order);
static int dc_code(const AoCtx& ac, int nch, int a, int b);

static const char* xll_headers(Xll& x, const U8* p, int n, int one2one, const Xll* base = 0, int off0 = 0,
                               U8* fix = 0, bool lax = false, bool nofs = false, bool hsx = false, AoCtx* ao = 0,
                               bool dif = false) {
  HdrReader r;
  HdrCoder hc;
  r.log = &x.flog;
  x.flog.clear();
  r.src = 0;
  r.wr = fix;
  if (ao && ao->hio && fix) {  // (from version 19 the join's: the fields from MS_HAC as they are read)
    hc.start(ao->hio, ao->hst, base != 0);
    r.src = &hc;
  }
  r.label(HL_FRM, 0);
  r.init(p, 0, (U64)n * 8);
  x.ncrc = 0;
  if (base) {  // (DTS:X object data: one channel set from off0, by base's common header)
    if (off0 + 3 > n) return "DTS:X object data too short";
    if (r.src) x.crcat[x.ncrc++] = off0 - 3;  // (its first bytes' CRC-16: join_obj() puts it there)
    x.hsize = off0;
    x.frame_size = n;
    x.nchsets = 1;
    x.nsegs = base->nsegs;
    x.nseg_log2 = base->nseg_log2;
    x.nsegsamples = base->nsegsamples;
    x.nframesamples = base->nframesamples;
    x.seg_nbits = base->seg_nbits;
    x.band_crc = base->band_crc;
    x.scalable = base->scalable;
    x.chmask_nbits = base->chmask_nbits;
    x.fixed_lsb = base->fixed_lsb;
  } else {
    if (n < 16 || r.get(32) != SYNC_XLL) return "no XLL sync word";
    if (r.get(4) + 1 > 1) return "an XLL version above 1";
    x.hsize = r.get(8) + 1;
    if (x.hsize < 12 || x.hsize > n || !crc_here(x, p, fix, 4, x.hsize)) return "an XLL header whose CRC fails";
    const int fsnb = r.get(5) + 1;
    x.frame_size = (int)((U64)r.get(fsnb) + 1);
    if (nofs)  // (the join's, before it knows the frame's size: n, which bounds it)
      x.frame_size = n;
    else if (x.frame_size > n || x.frame_size < x.hsize)
      return "an XLL frame larger than its asset";
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
  }
  if (ao && ao->pre) {  // (from version 19 the join's: now that the samples' count is known)
    const char* why = ao->pre(ao->pre_arg, x);
    if (why) return why;
  }
  int off = x.hsize, hier = 0;
  x.nch = 0;
  x.nbands = 1;
  for (int c = 0; c < x.nchsets; c++) {
    Chset& s = x.cs[c];
    memset(&s, 0, sizeof s);
    r.init(p, (U64)off * 8, (U64)n * 8);
    r.label(HL_SET, c);
    int hs = r.get(10) + 1;  // (hsx: the join's, XORed with the size its fields take, put back below)
    if (!hsx && (off + hs > n || hs < 3 || !crc_here(x, p, fix, off, off + hs)))
      return "a channel set header whose CRC fails";
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
      r.label(HL_BAND, c, b);
      d.decor = r.get(1);
      // (the join's from version 9, ao: the channel order and the coefficients against dc_order()'s and
      // dc_code()'s)
      const bool dp = ao && ao->val && fix && b == 0 && s.nbands == 1;
      if (d.decor && s.nch > 1) {
        d.ordat = (int)r.pos;
        for (int ch = 0; ch < s.nch; ch++) d.order[ch] = r.get(chbits);
        if (dp) {
          int op[XCH];
          dc_order(*ao, x.nch, s.nch, op);
          for (int ch = 0; ch < s.nch; ch++) {
            bits_xor(fix, d.ordat + chbits * ch, chbits, (U32)op[ch]);
            d.order[ch] ^= op[ch];
          }
        }
        const bool acf = dp && ao->st->v18 && ao->io;  // (from version 18 the flags in MS_AC: ac_dcf())
        const int set = base ? XCS : c;
        int hist = 0;
        for (int i = 0; i < s.nch / 2; i++) {
          d.dcat[i] = -1;
          d.dcoeff[i] = 0;
          r.label(HL_DCP, c, b, 2 * i);
          d.dfat[i] = (int)r.pos;
          int fl = (int)r.get(1);
          const int pc = dp ? dc_code(*ao, s.nch, d.order[2 * i], d.order[2 * i + 1]) : 0;
          if (acf) {
            const int gb = ao->st->v23 ? dc_gain(*ao, x.nch, d.order[2 * i], d.order[2 * i + 1], pc) : 0;
            const int f = fl ^ ao->io->get(ac_dcf(set, i, hist, ao->st->dcp[set][i], unzig((U32)pc), gb));
            bits_xor(fix, d.dfat[i], 1, (U32)(fl ^ f));
            fl = f;
            if (ao->ndp < XCS * XCH / 2) {
              ao->dpend[ao->ndp].set = (U8)set;
              ao->dpend[ao->ndp].i = (U8)i;
              ao->dpend[ao->ndp++].f = (U8)f;
            }
          }
          hist = hist << 1 | fl;
          if (!fl) continue;
          d.dcat[i] = (int)r.pos;
          U32 code = r.get(7);
          if (dp) {
            bits_xor(fix, d.dcat[i], 7, (U32)pc);
            code ^= (U32)pc;
          }
          d.dcoeff[i] = unzig(code);
        }
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
      d.aoat = (int)r.pos;
      r.label(HL_AO, c, b);
      for (int ch = 0; ch < s.nch; ch++) d.aorder[ch] = r.get(4);
      if (ao && ao->val && fix && b == 0 && s.nbands == 1) {  // (the join's: the orders against ao_pred()'s)
        int pred[XCH];
        U8 rank[XCH][16], mbk[XCH];
        const int p0 = ao->np;
        ao_pred(*ao, base ? XCS : c, x.nch, s, d, pred, true, rank, mbk);
        const bool acr = ao->st->v18 && ao->io;  // (from version 18 the ranks in MS_AC: ac_rank_get())
        for (int ch = 0; ch < s.nch; ch++) {  // (from version 17 their rank; dif: from 13 their difference; before, XORed)
          const int sym =
              acr ? d.aorder[ch] ^ ac_rank_get(*ao->io, base ? XCS : c, mbk[ch], ao->st->v23 ? ac_rx(rank[ch]) : 0)
                  : d.aorder[ch];
          const int act = ao->st->v17 ? rank[ch][sym]
                          : dif       ? (int)dif_back((U32)d.aorder[ch], (U32)pred[ch], 4)
                                      : d.aorder[ch] ^ pred[ch];
          bits_xor(fix, d.aoat + 4 * ch, 4, (U32)(d.aorder[ch] ^ act));
          d.aorder[ch] = act;
        }
        for (int i = p0; i < ao->np; i++) ao->pend[i].act = (U8)d.aorder[ao->pend[i].pos];
      }
      for (int ch = 0; ch < s.nch; ch++)
        if (d.aorder[ch] > d.highest) d.highest = d.aorder[ch];
      if (d.highest > x.nsegsamples) return "a prediction order above the segment's samples";
      for (int ch = 0; ch < s.nch; ch++) {
        r.label(HL_FO, c, b, ch);
        d.forder[ch] = d.aorder[ch] ? 0 : r.get(2);
      }
      for (int ch = 0; ch < s.nch; ch++) {
        r.label(HL_RC, c, b, ch * 16);
        d.rcat[ch] = (int)r.pos;
        for (int j = 0; j < d.aorder[ch]; j++) {
          const int q = unzig(r.get(8));
          if (q == -128 && !lax) return "a reflection coefficient out of range";
          d.rc[ch][j] = q >= 0 ? RC_TAB[q] : q > -128 ? -(int)RC_TAB[-q] : 0;
        }
      }
      r.label(HL_LSB, c, b);
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
    x.csoff[c] = off;
    x.csnat[c] = (int)((r.pos - (U64)off * 8 + 7) >> 3) + 2;
    if (hsx) {
      if (!fix || r.over || x.csnat[c] > 1024) return "a damaged channel set header";
      hs = ((hs - 1) ^ (x.csnat[c] - 1)) + 1;
      bits_xor(fix, off * 8, 10, (U32)(x.csnat[c] - 1));
      if (off + hs > n || hs < 3 || !crc_here(x, p, fix, off, off + hs)) return "a channel set header whose CRC fails";
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
  x.navi_at = off;
  r.init(p, (U64)off * 8, (U64)n * 8);
  r.label(HL_NAVI, 0);
  x.navi.assign((size_t)x.nbands * x.nsegs * x.nchsets, 0);
  int* nv = &x.navi[0];
  for (int b = 0; b < x.nbands; b++)
    for (int s = 0; s < x.nsegs; s++)
      for (int c = 0; c < x.nchsets; c++) *nv++ = x.cs[c].nbands > b ? (int)((U64)r.get(x.seg_nbits) + 1) : 0;
  r.skip((8 - (r.pos & 7)) & 7);
  r.get(16);
  if (r.over) return "a NAVI table past the frame";
  x.data_start = (int)(r.pos >> 3);
  if (!crc_here(x, p, fix, off, x.data_start)) return "a NAVI table whose CRC fails";
  if (r.src) {  // (from version 19 the bits no field takes, after the fields)
    std::vector<HField> g;
    hdr_gaps(x, x.data_start, g);
    for (size_t i = 0; i < g.size(); i++) bits_xor(fix, g[i].pos, g[i].w, hc.code(g[i].key, g[i].w, 0, false));
  }
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
        k.mbits = (U32)(r.pos - at);
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

// The decorrelated MSB signals of the frame's samples val (xll_decode()'s inverse, as far as its
// prediction: the LPC's input) into msb ([band][channel, a set's as its band data has them]), the
// LSBs into lsb; false where val doesn't decode so, with the frame's header and what the meta file kept
static bool xll_msb(const Xll& x, const std::vector<Chunk>& ck, const S32* val, const Extra& ex, BandData& msb,
                    BandData& lsb) {
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
  msb.init(N, x.nch);
  lsb.init(N, x.nch);
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
        U32* bb = (U32*)msb.at(b, x.chofs[c] + i);
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
            const U32* b0 = (const U32*)msb.at(b, x.chofs[c] + 2 * i);
            U32* b1 = (U32*)msb.at(b, x.chofs[c] + 2 * i + 1);
            for (int n = 0; n < N; n++) b1[n] -= (U32)decor_term(b0[n], d.dcoeff[i]);
          }
    }
  return !ov.over && ov.end - ov.pos < 8;  // (all the overlapping bits taken, but a byte's padding)
}

// the residuals of the decorrelated MSB signals msb (xll_msb()'s) by the frame's prediction
static void xll_residuals(const Xll& x, const BandData& msb, BandData& res, std::vector<U32>& tmp) {
  const int N = x.nframesamples;
  res.init(N, x.nch);
  tmp.resize((size_t)N);
  for (int b = 0; b < x.nbands; b++)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      const Band& d = s.band[b];
      for (int ch = 0; ch < s.nch; ch++) {
        const U32* bb = (const U32*)msb.at(b, x.chofs[c] + ch);
        S32* e = res.at(b, x.chofs[c] + ch);
        const int o = d.aorder[ch];
        if (o) {
          int co[XORD];
          rc_to_lpc(d.rc[ch], o, co);
          for (int n = 0; n < o && n < N; n++) e[n] = (S32)bb[n];
          for (int n = o; n < N; n++) e[n] = (S32)(bb[n] + (U32)lpc_pred(bb, n, o, co));
        } else {
          U32* dd = &tmp[0];
          memcpy(dd, bb, (size_t)N * sizeof *dd);
          for (int j = 0; j < d.forder[ch]; j++)
            for (int n = N - 1; n > 0; n--) dd[n] -= dd[n - 1];
          for (int n = 0; n < N; n++) e[n] = (S32)dd[n];
        }
      }
    }
}

// --- the reflection coefficients as the encoders' analysis makes them (the meta file keeps the
// stream's codes XORed with these)

// sin^2(pi n / (N - 1)), n < N (Hann's window), in Q30: in integers, the same everywhere
static const std::vector<S32>& hann_q30(int N) {
  static std::vector<S32> w;
  static int wn = -1;
  if (wn == N) return w;
  w.assign((size_t)N, 0);
  wn = N;
  typedef unsigned __int128 U128;
  const U128 PI61 = 7244019458077122842ull;  // (pi in Q61)
  for (int n = 1; n < N - 1; n++) {
    const int m = n <= (N - 1) / 2 ? n : N - 1 - n;  // (the angle to pi/2 at most)
    const U128 x = PI61 * (U64)m / (U64)(N - 1), x2 = (x * x) >> 61;
    U128 t = x, sn = x;  // (sin x by its series: the terms alternate and fall)
    for (int k = 1; k <= 12; k++) {
      t = ((t * x2) >> 61) / (U64)((2 * k) * (2 * k + 1));
      if (k & 1) sn -= t;
      else sn += t;
    }
    w[n] = (S32)((sn * sn + ((U128)1 << 91)) >> 92);
  }
  return w;
}

// single precision, each operation rounded where it is (no fused or wider arithmetic)
static inline float f32mul(float a, float b) {
  volatile float r = a * b;
  return r;
}
static inline float f32add(float a, float b) {
  volatile float r = a + b;
  return r;
}
static inline float f32div(float a, float b) {
  volatile float r = a / b;
  return r;
}

// a 128-bit integer (|v| < 2^127) as the nearest double, ties to even: the same everywhere
static double i128_double(__int128 v) {
  typedef unsigned __int128 U128;
  const bool neg = v < 0;
  U128 u = neg ? (U128)0 - (U128)v : (U128)v;
  int sh = 0;
  while (u >> (53 + sh)) sh++;
  if (sh) {
    U128 q = u >> sh;
    const U128 rem = u - (q << sh), half = (U128)1 << (sh - 1);
    if (rem > half || (rem == half && (q & 1))) q++;
    u = q;
  }
  const double d = ldexp((double)(U64)u, sh);
  return neg ? -d : d;
}

// tanh(i/32), the bounds of the encoders' quantization of a reflection coefficient's size (RC_TAB's
// entries are tanh((i + 1/2)/32) but the first, 0)
static const double RC_THR[128] = {
    0.0, 0.031239831446031256, 0.062418746747512514, 0.09347630396922774, 0.12435300177159621,
    0.1549907303716235, 0.1853331999081395, 0.21532633966578324, 0.24491866240370913,
    0.2740615889607664, 0.3027097293321085, 0.330821117493628, 0.35835739835078595,
    0.3852839662694723, 0.41157005567402244, 0.43718878514171233, 0.46211715726000974,
    0.48633601722496234, 0.5098299737352566, 0.5325872861719194, 0.5545997223493823,
    0.5758623912857893, 0.5963735554792423, 0.6161344271055263, 0.6351489523872873,
    0.6534235881413468, 0.6709670742068736, 0.6877902051090852, 0.703905603936621,
    0.7193275010198336, 0.7340715196043415, 0.7481544703297354, 0.7615941559557649,
    0.7744091874342136, 0.7866188121086977, 0.7982427545398869, 0.809301070201781,
    0.8198140120760964, 0.8298019099859595, 0.8392850623555859, 0.8482836399575129,
    0.8568176011108951, 0.8649066177207418, 0.8725700114960693, 0.8798266996519848,
    0.8866951493840524, 0.8931933404003515, 0.8993387348050462, 0.9051482536448664,
    0.9106382594546996, 0.9158245441687624, 0.9207223217980599, 0.925346225311741,
    0.9297103071984135, 0.9338280432225917, 0.937712338930443, 0.9413755384972874,
    0.9448294355464201, 0.9480852856044063, 0.9511538198917856, 0.9540452601799487,
    0.9567693344747047, 0.9593352933146825, 0.9617519264982222, 0.9640275800758169,
    0.966170173466547, 0.9681872165763705, 0.9700858268137, 0.971872745913509, 0.9735543564953897,
    0.9751366982936284, 0.9766254840086112, 0.9780261147388136, 0.9793436949613896,
    0.9805830470370519, 0.9817487252216389, 0.982845029172576, 0.9838760169434567,
    0.9848455174642784, 0.9857571425085297, 0.9866142981514303, 0.9874201957262151,
    0.9881778622875124, 0.9888901505926178, 0.9895597486128832, 0.990189188588556,
    0.9907808556412516, 0.991336995958872, 0.9918597245682077, 0.9923510327107256,
    0.9928127948371598, 0.993246775236521, 0.9936546343150297, 0.9940379345402902,
    0.994398146065758, 0.9947366520502309, 0.9950547536867305, 0.9953536749547375,
    0.9956345671093096, 0.9958985129201653, 0.996146530673345, 0.9963795779475947,
    0.9965985551771295, 0.9968043090119653, 0.996997635486526, 0.997179283006764,
    0.9973499551655737, 0.9975103133958233, 0.997660979469889, 0.9978025378541507,
    0.9979355379264904, 0.9980604960644348, 0.9981778976111987, 0.9982881987265095,
    0.9983918281287415, 0.998489188734541, 0.9985806592017988, 0.9986665953815128,
    0.9987473316837812, 0.9988231823628861, 0.9988944427261528, 0.9989613902710102,
    0.9990242857544355, 0.99908337419873, 0.9991388858373508, 0.9991910370043159,
    0.9992400309704963, 0.9992860587299247
};

// the code of reflection coefficient k, zigzagged: the encoders', it seems (fl, from version 10),
// floor(32 atanh(|k|)), that is, the last i with tanh(i/32) at most |k|; before, the nearest of
// RC_TAB; cap at most
static int rc_code(float k, int cap, bool fl) {
  const double a = fabs((double)k);
  int best = 0;
  if (fl) {
    while (best < 127 && RC_THR[best + 1] <= a) best++;
  } else {
    double bd = fabs(a * 65536.0 - RC_TAB[0]);
    for (int i = 1; i < 128; i++) {
      const double d = fabs(a * 65536.0 - RC_TAB[i]);
      if (d < bd) bd = d, best = i;
    }
  }
  if (best > cap) best = cap;
  return k < 0 && best ? 2 * best - 1 : 2 * best;
}

static void rc_autocorr(const S32* bb, int N, int o, float* R);
// the codes of the o reflection coefficients Levinson's recursion in single precision gives the
// autocorrelation R (as rc_predict()'s); from version 20 (f20) as the encoders take it on where it
// breaks down (the error no more than 0, a coefficient past 1), a code from the second on at most
// 121 where it is positive and 127 where it is negative (all 320 channels of the Cymatics stream's
// first 60 MB where format 19's rule missed: there it stopped, 0s, and capped the negative at 121)
static void rc_levinson(const float* R, int o, int* code, bool fl, bool f20) {
  float a[XORD + 1], an[XORD + 1];
  a[0] = 1;
  float E = R[0];
  for (int j = 1; j <= o; j++) {
    if (!f20 && !(E > 0)) return;
    float acc = R[j];
    for (int i = 1; i < j; i++) acc = f32add(acc, f32mul(a[i], R[j - i]));
    const float k = -f32div(acc, E);
    code[j - 1] = rc_code(k, j > 1 && !(f20 && k < 0) ? 121 : 127, fl);
    for (int i = 1; i < j; i++) an[i] = f32add(a[i], f32mul(k, a[j - i]));
    for (int i = 1; i < j; i++) a[i] = an[i];
    a[j] = k;
    E = f32mul(E, f32add(1.0f, -f32mul(k, k)));
  }
}
// The codes of the o reflection coefficients the encoders' analysis gives the decorrelated MSB
// signal bb (N samples): its autocorrelation under Hann's window, Levinson's recursion in single
// precision, each coefficient's code rc_code()'s (fl), from the second on 121 at most (the DTS:X
// encoder's: 95% of Nigel_Stanford_Cymatics_DTS-X.dts's codes so); 0s where the recursion breaks
// down. The autocorrelation (f15, from version 15) as the encoders compute it, rc_autocorr()'s,
// else in integers, each sample to 31 bits
static void rc_predict(const S32* bb, int N, int o, int* code, bool fl, bool f15, bool f20 = false) {
  for (int j = 0; j < o; j++) code[j] = 0;
  float R[XORD + 1];
  if (f15) {
    rc_autocorr(bb, N, o, R);
    rc_levinson(R, o, code, fl, f20);
    return;
  }
  const std::vector<S32>& w = hann_q30(N);
  static std::vector<S64> xs;
  xs.resize((size_t)N);
  U64 mx = 0;
  for (int n = 0; n < N; n++) {
    xs[n] = (S64)bb[n] * w[n];
    mx |= (U64)(xs[n] < 0 ? -xs[n] : xs[n]);
  }
  int sh = 0;
  while (mx >> (31 + sh)) sh++;
  if (sh)
    for (int n = 0; n < N; n++) xs[n] = (xs[n] + ((S64)1 << (sh - 1))) >> sh;
  for (int l = 0; l <= o; l++) {
    __int128 acc = 0;
    for (int n = 0; n + l < N; n++) acc += xs[n] * xs[n + l];
    R[l] = (float)i128_double(acc);
  }
  rc_levinson(R, o, code, fl, false);
}

static inline double f64mul(double a, double b) {
  volatile double r = a * b;
  return r;
}
static inline double f64div(double a, double b) {
  volatile double r = a / b;
  return r;
}
static inline double f64sub(double a, double b) {
  volatile double r = a - b;
  return r;
}

// cos(x) of a double x in [0, 2 pi], rounded to the nearest double, in 124-bit fixed point (a
// platform's own cos needn't round so: glibc's misses 54 of the 32764 angles of hann_cos()'s
// windows of 4, 8, ... 16384 samples, by one unit; this one's results, checked in 60-digit decimals
// where they differ, are the nearest)
namespace fxcos {
typedef unsigned __int128 U128;
const int FB = 124;
const U128 PI = ((U128)0x3243F6A8885A308DULL << 64) | (U128)0x313198A2E0370734ULL;  // (pi in Q124)
// a b >> 124, a and b under 2^127
inline U128 mul(U128 a, U128 b) {
  const U64 a1 = (U64)(a >> 64), a0 = (U64)a, b1 = (U64)(b >> 64), b0 = (U64)b;
  const U128 p00 = (U128)a0 * b0, p01 = (U128)a0 * b1, p10 = (U128)a1 * b0, p11 = (U128)a1 * b1;
  U128 lo = p00, hi = p11;
  const U128 mid = p01 + p10, midc = mid < p01 ? (U128)1 << 64 : 0, ml = mid << 64;
  lo += ml;
  hi += (mid >> 64) + (lo < ml) + midc;
  return (hi << (128 - FB)) | (lo >> FB);
}
// the series of cos y and sin y, y in [0, pi/4]
inline U128 cos_s(U128 y) {
  const U128 y2 = mul(y, y);
  U128 t = (U128)1 << FB, s = t;
  for (int k = 1; k <= 30 && t; k++) {
    t = mul(t, y2) / (U64)((2 * k - 1) * (2 * k));
    s = k & 1 ? s - t : s + t;
  }
  return s;
}
inline U128 sin_s(U128 y) {
  const U128 y2 = mul(y, y);
  U128 t = y, s = y;
  for (int k = 1; k <= 30 && t; k++) {
    t = mul(t, y2) / (U64)((2 * k) * (2 * k + 1));
    s = k & 1 ? s - t : s + t;
  }
  return s;
}
inline double cos_rn(double x) {
  int e;
  const double m = frexp(x, &e);  // (x = m 2^e exactly, m in [1/2, 1) or 0)
  U128 q = 0;
  if (m != 0) {
    const U64 mi = (U64)ldexp(m, 53);
    const int sh = e - 53 + FB;
    q = sh >= 0 ? (U128)mi << sh : (U128)mi >> -sh;
  }
  bool neg = false;
  if (q > PI) q = (PI << 1) > q ? (PI << 1) - q : q - (PI << 1);  // (cos(2 pi - x))
  if (q > (PI >> 1)) q = PI - q, neg = true;                        // (-cos(pi - x))
  const U128 v = q > (PI >> 2) ? sin_s((PI >> 1) - q) : cos_s(q);
  if (!v) return 0;
  int top = 127;
  while (!((v >> top) & 1)) top--;
  const int drop = top - 52;  // (53 bits kept, the rest rounded to nearest, ties to even)
  U128 r = drop > 0 ? v >> drop : v << -drop;
  if (drop > 0) {
    const U128 rem = v - (r << drop), half = (U128)1 << (drop - 1);
    if (rem > half || (rem == half && (r & 1))) r++;
  }
  const double d = ldexp((double)(U64)r, drop - FB);
  return neg ? -d : d;
}
}  // namespace fxcos

// the encoders' Hann window of N samples as their analysis has it, in double precision: 0.5 - 0.5
// cos(2 pi n / (N - 1)), the angle and the rest rounded at each step (from version 15)
static const std::vector<double>& hann_cos(int N) {
  static std::vector<double> w;
  static int wn = -1;
  if (wn == N) return w;
  w.assign((size_t)N, 0);
  wn = N;
  const double twopi = 6.283185307179586476925286766559;  // (2 pi as a double, as 2 * M_PI)
  for (int n = 1; n < N - 1; n++) w[n] = f64sub(0.5, f64mul(0.5, fxcos::cos_rn(f64div(f64mul(twopi, n), N - 1))));
  return w;
}
// The autocorrelation R[0..o] of bb (N samples) under Hann's window as the encoders compute it, it
// seems: each windowed sample rounded to single precision (the product in double precision), each
// lag's products in single precision, summed in double precision, the sum rounded to single
// precision. With it (from version 15) the reflection coefficients' codes come out the same in all
// of the Cymatics stream's first channel set and all but 44 of the 885113 of s.2.dts (99.9% and
// 99.3% with the integers' before), in 99.3% of the Cymatics stream's second set's (92.5%) and
// 99.8% of its object audio's (93.5%)
// (the last ones are kept with their samples: the pairs' gains, the orders' rule and the reflection
// coefficients take those of the same samples, and the split's check takes them all again; to
// AO_K lags at least, for those that ask for more; and their dc_bits())
struct AcfCache {
  enum { NE = 48 };
  struct E {
    std::vector<S32> x;
    U64 h;
    int o;  // (the last lag; -1: none)
    float R[XORD + 1];
    int nb;  // (dc_bits() kept: 1)
    S64 bits[2];
  } e[NE];
  int next;
  AcfCache() : next(0) {
    for (int i = 0; i < NE; i++) e[i].h = 0, e[i].o = -1, e[i].nb = 0;
  }
};
static AcfCache acf;
// the entry of bb's (N > 0 samples) autocorrelation to lag o at least
static AcfCache::E* acf_get(const S32* bb, int N, int o) {
  AcfCache& c = acf;
  U64 h[4] = {(U64)N, 1, 2, 3};
  int n = 0;
  for (; n + 4 <= N; n += 4)
    for (int j = 0; j < 4; j++) h[j] = (h[j] + (U32)bb[n + j]) * 0x9E3779B97F4A7C15ull;
  for (; n < N; n++) h[0] = (h[0] + (U32)bb[n]) * 0x9E3779B97F4A7C15ull;
  const U64 hk = h[0] ^ (h[1] >> 7 | h[1] << 57) ^ (h[2] >> 23 | h[2] << 41) ^ (h[3] >> 41 | h[3] << 23);
  for (int i = 0; i < AcfCache::NE; i++) {
    const AcfCache::E& e = c.e[i];
    if (e.h == hk && e.o >= o && e.x.size() == (size_t)N && !memcmp(&e.x[0], bb, (size_t)N * sizeof *bb))
      return &c.e[i];
  }
  const int oc = o > AO_K ? o : AO_K;
  const std::vector<double>& w = hann_cos(N);
  static std::vector<float> xf;
  xf.resize((size_t)N);
  for (n = 0; n < N; n++) xf[n] = (float)f64mul((double)bb[n], w[n]);
  AcfCache::E& e = c.e[c.next];
  c.next = (c.next + 1) % AcfCache::NE;
  for (int l = 0; l <= oc; l++) {
    double acc = 0;
    for (n = 0; n + l < N; n++) acc += (double)f32mul(xf[n], xf[n + l]);
    e.R[l] = (float)acc;
  }
  e.x.assign(bb, bb + N);
  e.h = hk;
  e.o = oc;
  e.nb = 0;
  return &e;
}
static void rc_autocorr(const S32* bb, int N, int o, float* R) {
  if (N <= 0) {
    for (int l = 0; l <= o; l++) R[l] = 0;
    return;
  }
  memcpy(R, acf_get(bb, N, o)->R, (size_t)(o + 1) * sizeof *R);
}

// The order of the nch channels of the set whose first is g0 of ac's samples, as the encoders' pairwise
// decorrelation has it, it seems (every frame of nearly every stream tried): pair after pair, the two
// channels left whose correlation is the largest in size (the first of equals, as the pairs come
// first channel, then second), the less loud first (equals: the later), the one left over last;
// their products into ac.S, for dc_code()
static void dc_order(AoCtx& ac, int g0, int nch, int* order) {
  const int N = ac.N;
  for (int j = 0; j < nch; j++)
    for (int k = j; k < nch; k++) {
      __int128 acc = 0;
      if (g0 + k < ac.nch) {
        const S32* a = ac.val + (size_t)(g0 + j) * N;
        const S32* b = ac.val + (size_t)(g0 + k) * N;
        for (int n = 0; n < N; n++) acc += (__int128)((S64)a[n] * b[n]);
      }
      ac.S[j][k] = ac.S[k][j] = acc;
    }
  bool used[XCH] = {false};
  int n = 0;
  for (int t = 0; t < nch / 2; t++) {
    int bj = -1, bk = -1;
    double br = -1;
    for (int j = 0; j < nch; j++)
      for (int k = j + 1; k < nch; k++) {
        if (used[j] || used[k]) continue;
        double r2 = 0;  // (the correlation squared, in correctly rounded steps: the same everywhere)
        if (ac.S[j][j] > 0 && ac.S[k][k] > 0) {
          const double c = i128_double(ac.S[j][k]);
          r2 = f64div(f64mul(c, c), f64mul(i128_double(ac.S[j][j]), i128_double(ac.S[k][k])));
        }
        if (r2 > br) br = r2, bj = j, bk = k;
      }
    if (ac.S[bj][bj] < ac.S[bk][bk]) {
      order[n++] = bj;
      order[n++] = bk;
    } else {
      order[n++] = bk;
      order[n++] = bj;
    }
    used[bj] = used[bk] = true;
  }
  for (int j = 0; j < nch; j++)
    if (!used[j]) order[n++] = j;
}

// The code of the pairwise decorrelation coefficient the encoders give the pair a (the base) and b
// of the channels dc_order() has had: their least squares fit in 8ths, rounded (every coefficient
// that isn't 0, in nearly every stream tried), zigzagged in 7 bits
static int dc_code(const AoCtx& ac, int nch, int a, int b) {
  if (a < 0 || b < 0 || a >= nch || b >= nch || !(ac.S[a][a] > 0)) return 0;
  const __int128 num = 16 * ac.S[a][b] + ac.S[a][a], den = 2 * ac.S[a][a];
  __int128 q = num / den;
  if (num % den && (num < 0)) q--;  // (floor)
  const int v = q < -64 ? -64 : q > 63 ? 63 : (int)q;
  return (int)(zig(v) & 127);
}

// log2 of the finite float f > 0 in Q20, by squaring its mantissa: in integers, the same everywhere
static S64 log2_q20(float f) {
  U32 u;
  memcpy(&u, &f, 4);
  int e = (int)(u >> 23) & 255;
  U64 m = u & 0x7FFFFF;
  if (e) {
    m |= 0x800000;
  } else {
    while (!(m & 0x800000)) m <<= 1, e--;
    e++;
  }
  S64 r = (S64)(e - 127) * (1 << 20);
  U64 v = m << 7;  // (1 to 2 in Q30)
  for (int i = 19; i >= 0; i--) {
    v = (v * v + (1ull << 29)) >> 30;
    if (v >> 31) v >>= 1, r += (S64)1 << i;
  }
  return r;
}

// The bits a channel's prediction takes by the autocorrelation R of its N samples (rc_autocorr()'s), as
// the order rule weighs them (ao_pred()'s): (N/2) log2 of the variance Levinson's recursion leaves,
// in single precision, at the cheapest order to 10, 8 bits an order (Q20; 0 where R[0] isn't > 0)
static S64 dc_cost(const float* R, int N) {
  if (!(R[0] > 0) || !(R[0] < 3.0e38f)) return 0;
  float a[11], an[11], E = R[0];
  a[0] = 1;
  S64 best = log2_q20(E) * (N / 2);
  for (int j = 1; j <= 10; j++) {
    float acc = R[j];
    for (int t = 1; t < j; t++) acc = f32add(acc, f32mul(a[t], R[j - t]));
    const float k = -f32div(acc, E);
    for (int t = 1; t < j; t++) an[t] = f32add(a[t], f32mul(k, a[j - t]));
    for (int t = 1; t < j; t++) a[t] = an[t];
    a[j] = k;
    E = f32mul(E, f32add(1.0f, -f32mul(k, k)));
    if (!(E > 0) || !(E < 3.0e38f)) break;
    const S64 c = log2_q20(E) * (N / 2) + (S64)8 * j * (1 << 20);
    if (c < best) best = c;
  }
  return best;
}
// From version 23 a pair's flag goes by what decorrelating its channel b by the base a at the
// coefficient pc (dc_code()'s) would save b's prediction (dc_cost() less dc_cost()), in 5 buckets:
// under -8 bits, -8 to 0, 0 to 8, 8 to 32, more. The encoders decorrelate where it saves bits after
// the prediction, it seems: the Cymatics stream's pairs at 64 bits or more 97% decorrelated, under -8
// 9% (by the samples' own correlation alone, as decorrelated past 128 bits as not). From version 24
// in 7: under -48, -48 to -8, -8 to 0, 0 to 8, 8 to 32, 32 to 64, more (the share decorrelated goes
// on changing past -8 and 32: of the Cymatics stream's pairs with a coefficient, 0.14 under -64,
// 0.24 at -32, 0.74 at 32, 0.93 past 64)
// From version 25 the bits the encoders would code a channel's samples x in, it seems: the order the
// rule picks (8.5 bits an order, no floor, on past where the recursion breaks down; from version 27,
// v27, the encoders' cost, ao_cost()'s at L 8 and F 2), its
// coefficients' codes (rc_levinson()'s) at 8 bits each, the residual by the decoder's prediction, each
// 32 samples' Rice code at its best width (the bits a width takes are convex in it); each 256's into
// b256
static S64 dc_bits(const S32* x, int N, S64* b256, bool v27) {
  *b256 = 0;
  if (N <= 0) return 0;
  AcfCache::E* ce = acf_get(x, N, 10);  // (the samples' entry: none other is taken till the bits are kept)
  if (ce->nb) {
    *b256 = ce->bits[1];
    return ce->bits[0];
  }
  ce->nb = 1;
  ce->bits[0] = ce->bits[1] = 0;
  float R[11];
  memcpy(R, ce->R, sizeof R);
  if (!(R[0] > 0) || !(R[0] < 3.0e38f)) return 0;
  float a[11], an[11], E = R[0];
  a[0] = 1;
  // (from version 27 the order by the encoders' cost, ao_cost()'s at L 8 and F 2)
  const S64 off = -log2_q20((float)(3 * N) / 8.0f), F = (S64)2 << 20, l0 = log2_q20(E) + off, L0 = l0 > F ? l0 : F;
  S64 best = v27 ? L0 * N : log2_q20(E) * (N / 2);
  int bk = 0;
  for (int j = 1; j <= 10; j++) {
    float acc = R[j];
    for (int t = 1; t < j; t++) acc = f32add(acc, f32mul(a[t], R[j - t]));
    const float k = -f32div(acc, E);
    for (int t = 1; t < j; t++) an[t] = f32add(a[t], f32mul(k, a[j - t]));
    for (int t = 1; t < j; t++) a[t] = an[t];
    a[j] = k;
    E = f32mul(E, f32add(1.0f, -f32mul(k, k)));
    if (!(E > 0) || !(E < 3.0e38f)) continue;
    S64 c;
    if (v27) {
      const S64 lk = log2_q20(E) + off, Lk = lk > F ? lk : F;
      c = Lk * (N - j) + L0 * j + (S64)16 * j * (1 << 20);
    } else {
      c = log2_q20(E) * (N / 2) + (S64)17 * j * (1 << 19);
    }
    if (c < best) best = c, bk = j;
  }
  int code[10] = {0}, rq[10], co[XORD];
  rc_levinson(R, bk, code, true, true);
  for (int j = 0; j < bk; j++) {
    const int q = unzig((U32)code[j]);
    rq[j] = q >= 0 ? RC_TAB[q] : q > -128 ? -(int)RC_TAB[-q] : 0;
  }
  if (bk) rc_to_lpc(rq, bk, co);
  static std::vector<U32> z;
  z.resize((size_t)N);
  for (int n = 0; n < N; n++) z[n] = zig(n < bk ? x[n] : (S32)((U32)x[n] + (U32)lpc_pred((const U32*)x, n, bk, co)));
  S64 bits[2] = {8 * bk, 8 * bk};
  for (int bl = 0; bl < 2; bl++)
    for (int s0 = 0, B = bl ? 256 : 32; s0 < N; s0 += B) {
    const int s1 = s0 + B < N ? s0 + B : N, len = s1 - s0;
    U64 sum = 0;
    for (int n = s0; n < s1; n++) sum += z[n];
    int p = 0;
    while (p < 23 && (sum / (U64)len) >> (p + 1)) p++;
    auto cost = [&](int w) {
      U64 c = (U64)w + (U64)len * (U64)(1 + w);
      for (int n = s0; n < s1; n++) c += z[n] >> w;
      return c;
    };
    // (the widths p - 1, p and p + 1 at once, nearly always the least among them: then on past it,
    // the smallest of the widths at the least, as one at a time from p would end at)
    const int q = p > 0 ? p - 1 : 0;
    U64 c3[3] = {0, 0, 0};
    for (int n = s0; n < s1; n++) c3[0] += z[n] >> q, c3[1] += z[n] >> (q + 1), c3[2] += z[n] >> (q + 2);
    for (int t = 0; t < 3; t++) c3[t] += (U64)(q + t) + (U64)len * (U64)(1 + q + t);
    U64 cp;
    if (p > 0 && c3[0] <= c3[1]) {
      p--, cp = c3[0];
      while (p > 0) {
        const U64 cl = cost(p - 1);
        if (cl > cp) break;
        cp = cl, p--;
      }
    } else {
      cp = c3[p - q];
      if (p < 23) {
        U64 ch = c3[p - q + 1];
        while (ch < cp) {
          cp = ch, p++;
          if (p >= 23) break;
          ch = cost(p + 1);
        }
      }
    }
    bits[bl] += (S64)cp;
  }
  ce->bits[0] = bits[0];
  ce->bits[1] = *b256 = bits[1];
  return bits[0];
}
// (from version 25 the buckets of the bits decorrelating would save, less the coefficient's 7, by
// dc_bits(), at Rice codes of 32 samples and of 256, each under -32, -8, 0, 8, 32, more: the flags of
// the Cymatics stream's pairs 31% fewer bytes, s.2.dts's 38%; the 32's the better there, the 256's,
// which are s.2.dts's segments, here)
static int dc_gain(const AoCtx& ac, int g0, int a, int b, int pc) {
  const int N = ac.N;
  const bool v24 = ac.st->v24, v25 = ac.st->v25;
  if (!ac.val || N <= 0 || a < 0 || b < 0 || g0 + a >= ac.nch || g0 + b >= ac.nch) return v25 ? 14 : v24 ? 3 : 2;
  const S32* va = ac.val + (size_t)(g0 + a) * N;
  const S32* vb = ac.val + (size_t)(g0 + b) * N;
  static std::vector<S32> yd;
  yd.resize((size_t)N);
  const int cf = unzig((U32)pc);
  for (int n = 0; n < N; n++) yd[n] = (S32)((U32)vb[n] - (U32)decor_term((U32)va[n], cf));
  float R1[11], R2[11];
  rc_autocorr(vb, N, 10, R1);
  rc_autocorr(&yd[0], N, 10, R2);
  const S64 g = dc_cost(R1, N) - dc_cost(R2, N), B = (S64)1 << 20;
  if (v25) {
    if (!cf) return 14;  // (as decorrelating by 0, both the same)
    S64 l1, l2;
    const S64 s1 = dc_bits(vb, N, &l1, ac.st->v27 != 0), s2 = dc_bits(&yd[0], N, &l2, ac.st->v27 != 0);
    const S64 d[2] = {s1 - s2 - 7, l1 - l2 - 7};
    static const int E[5] = {-32, -8, 0, 8, 32};
    int bi[2] = {0, 0};
    for (int t = 0; t < 2; t++)
      while (bi[t] < 5 && d[t] >= E[bi[t]]) bi[t]++;
    return bi[0] * 6 + bi[1];
  }
  if (v24) return g < -48 * B ? 0 : g < -8 * B ? 1 : g < 0 ? 2 : g < 8 * B ? 3 : g < 32 * B ? 4 : g < 64 * B ? 5 : 6;
  return g < -8 * B ? 0 : g < 0 ? 1 : g < 8 * B ? 2 : g < 32 * B ? 3 : 4;
}

// The prediction orders of the channels of channel set s (set: its index in the counts) in band 0,
// its first channel g0 of ac's samples, decorrelated as the band d's header says (no LSBs taken off),
// into pred: the encoders' choice, it seems, of the order k (0 to 10) least in (N/2) max(log2 E_k,
// F) + L k, E_k the variance left after Levinson's recursion to order k (as rc_predict()'s, of the
// samples under Hann's window), F a floor and L a coefficient's cost in bits. L and F go by the
// channel (set and index in it, whatever place the decorrelation's order gives it): those of 12 by 5
// that would have predicted its recent orders best (from version 14; 6 by 3 and all its past orders
// before: L 8.5 and F 2 for most of the DTS:X encoder's first channel set, its LFE too, L 15 for its
// second set and its object audio). From version 24 the recursion goes on past where it breaks down
// (E_k no more than 0), as the encoders' does (rc_levinson()'s), an order where it has no E_k is
// none, as their cost's NaN would be, and one past it can be the cheapest: the orders of the
// Cymatics stream's frames where it breaks down, 85% (the second set) and 80% (the object audio) as
// the rule had them before, 98% and 97%. From version 27 the cost is the encoders' own, it seems:
// the first k samples, which the prediction can't take, at the samples' variance and the rest at
// E_k's, (N - k)/2 max(log2 E_k, F) + (k/2) max(log2 E_0, F) + L k, with L 8 bits (each
// coefficient's code) and F 2: 99.97% of s.2.dts's orders, 99.78% of the Cymatics stream's (the
// rule before took 97.4% and 95.7%), 99.1-100% of the other 48 kHz streams'; the 96 kHz ones', at
// 1024 samples a frame, go past 10 and are neither rule's. L in 12 values about 8 and F in 5, the
// candidates, go by the channel as before. keep: the candidates' orders to ac's pending ones, for
// ao_count()
// (the candidates: L in halves of a bit, F; before version 14, L 12, 17, 22, 26, 30 or 36 and F
// none, 2 or 4 alone, AO_OLD)
static const int AO_LAM[AO_NL] = {10, 12, 14, 17, 20, 22, 24, 26, 28, 30, 33, 36};
// (from version 27: L in halves of a bit, around the encoders' 8; AO_DEF27 L 8 and F 2)
static const int AO_LAM27[AO_NL] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 24, 28};
enum { AO_DEF27 = 2 * AO_NL + 4 };
static const int AO_FLO[AO_NF] = {-1000, 1, 2, 3, 4};
static bool ao_old(int j) {
  const int f = j / AO_NL, l = j % AO_NL;
  return (f == 0 || f == 2 || f == 4) && (l & 1);
}
// candidate f, l's cost of order k (L: the variance's log2 at each order, Q20): before version 27
// (N/2) max(L_k, F) + L k, from version 27 (v27) twice ((N - k)/2 max(L_k, F) + (k/2) max(L_0, F) + L k)
static S64 ao_cost(const S64* L, int k, int N, int f, int l, bool v27) {
  const S64 F = (S64)AO_FLO[f] * (1 << 20), lv = L[k] > F ? L[k] : F;
  if (!v27) return lv * (N / 2) + (S64)AO_LAM[l] * k * (1 << 19);
  const S64 l0 = L[0] > F ? L[0] : F;
  return lv * (N - k) + l0 * k + (S64)AO_LAM27[l] * k * (1 << 20);
}
static void ao_pred(AoCtx& ac, int set, int g0, const Chset& s, const Band& d, int* pred, bool keep, U8 (*rank)[16],
                    U8* mbk) {
  const int N = ac.N;
  const bool v27 = ac.st->v27 != 0;
  static std::vector<S32> sig[2];
  static std::vector<S64> xs;
  sig[0].resize((size_t)N);
  sig[1].resize((size_t)N);
  xs.resize((size_t)N);
  int nb = 0;
  while ((1 << nb) < N) nb++;
  const S64 lim = (S64)1 << ((62 - nb) / 2);  // (the autocorrelation in 64 bits: |xs| under it)
  for (int i = 0; i < s.nch; i++) {
    pred[i] = 0;
    if (rank)  // (as they are where there are no samples to weigh them by)
      for (int k = 0; k < 16; k++) rank[i][k] = (U8)k;
    if (mbk) mbk[i] = 15;
    const int oc = d.order[i], g = g0 + oc;
    if (g >= ac.nch) continue;
    S32* bb = &sig[i & 1][0];
    const S32* v = ac.val + (size_t)g * N;
    if ((i & 1) && d.decor && d.dcoeff[i / 2]) {
      const S32* b0 = &sig[0][0];
      for (int n = 0; n < N; n++) bb[n] = (S32)((U32)v[n] - (U32)decor_term((U32)b0[n], d.dcoeff[i / 2]));
    } else {
      memcpy(bb, v, (size_t)N * sizeof *bb);
    }
    float R[AO_K + 1];
    int sh = 30;
    if (ac.st->v15) {
      rc_autocorr(bb, N, AO_K, R);  // (from version 15 as the encoders compute it, in the samples' units)
    } else {
      const std::vector<S32>& w = hann_q30(N);
      S64 mx = 0;
      for (int n = 0; n < N; n++) {
        xs[n] = (S64)bb[n] * w[n];
        const S64 a = xs[n] < 0 ? -xs[n] : xs[n];
        if (a > mx) mx = a;
      }
      sh = 0;
      while ((mx >> sh) >= lim) sh++;
      if (sh)
        for (int n = 0; n < N; n++) xs[n] = (xs[n] + ((S64)1 << (sh - 1))) >> sh;
      for (int l = 0; l <= AO_K; l++) {
        S64 acc = 0;
        for (int n = 0; n + l < N; n++) acc += xs[n] * xs[n + l];
        R[l] = (float)acc;  // (exactly so in any float environment: an integer, rounded once)
      }
    }
    U8 out[AO_NR];
    memset(out, 0, sizeof out);
    S64 Lk[AO_K + 1];  // (rank's: the variance's log2 at each order, K the last order there is one for)
    bool okk[AO_K + 1];  // (the orders there is one for: from version 24 past where the recursion breaks down)
    int Kv = -1;
    if (R[0] > 0) {
      // log2 of the samples' variance: E's under the window (Q30, shifted by sh) less log2(3N/8)
      const S64 off = (S64)(2 * sh - 60) * (1 << 20) - log2_q20((float)(3 * N) / 8.0f);
      S64 L[AO_K + 1];
      float a[AO_K + 1], an[AO_K + 1], E = R[0];
      a[0] = 1;
      int K = 0;
      L[0] = log2_q20(E) + off;
      okk[0] = true;
      for (int j = 1; j <= AO_K; j++) okk[j] = false;
      for (int j = 1; j <= AO_K; j++) {
        float acc = R[j];
        for (int t = 1; t < j; t++) acc = f32add(acc, f32mul(a[t], R[j - t]));
        const float k = -f32div(acc, E);
        for (int t = 1; t < j; t++) an[t] = f32add(a[t], f32mul(k, a[j - t]));
        for (int t = 1; t < j; t++) a[t] = an[t];
        a[j] = k;
        E = f32mul(E, f32add(1.0f, -f32mul(k, k)));
        if (!(E > 0) || !(E < 3.0e38f)) {  // (no order there: from version 24 on past it, as the encoders go)
          L[j] = 0;
          if (ac.st->v24) continue;
          break;
        }
        L[j] = log2_q20(E) + off;
        okk[j] = true;
        K = j;
      }
      Kv = K;
      for (int k = 0; k <= K; k++) Lk[k] = L[k];
      for (int f = 0; f < AO_NF; f++)
        for (int l = 0; l < AO_NL; l++) {
          int bk = 0;
          S64 bj = 0;
          for (int k = 0; k <= K; k++) {
            if (!okk[k]) continue;
            const S64 j = ao_cost(L, k, N, f, l, v27);
            if (k == 0 || j < bj) bj = j, bk = k;
          }
          out[f * AO_NL + l] = (U8)bk;
        }
    }
    const U32* cnt = ac.st->n[set][oc];
    int bi = v27 ? (int)AO_DEF27 : (int)AO_DEF;
    for (int j = 0; j < AO_NR; j++)
      if (cnt[j] > cnt[bi] && (ac.st->v14 || ao_old(j))) bi = j;
    pred[i] = out[bi];
    if (rank) {  // (the orders by the cost under candidate bi, the smaller first of equals, then those past K)
      S64 J[16];
      const int f = bi / AO_NL, l = bi % AO_NL;
      for (int k = 0; k <= Kv; k++) J[k] = ao_cost(Lk, k, N, f, l, v27);
      int nr = 0, nv = 0;
      bool used[16] = {false};
      for (;;) {
        int bk = -1;
        for (int k = 0; k <= Kv; k++)
          if (okk[k] && !used[k] && (bk < 0 || J[k] < J[bk])) bk = k;
        if (bk < 0) break;
        used[bk] = true;
        rank[i][nr++] = (U8)bk;
        nv++;
      }
      for (int k = 0; k < 16; k++)
        if (!used[k]) rank[i][nr++] = (U8)k;
      if (mbk && nv >= 2) {  // (the cost between the two cheapest, in bits: the bucket of its log2, to 14)
        const U64 t = (U64)(J[rank[i][1]] - J[rank[i][0]]) >> (v27 ? 21 : 20);
        int b = 0;
        while (b < 14 && (t + 1) >> (b + 1)) b++;
        mbk[i] = (U8)b;
      }
    }
    if (keep && ac.np < XCS * XCH) {
      ac.pend[ac.np].set = (U8)set;
      ac.pend[ac.np].oc = (U8)oc;
      ac.pend[ac.np].pos = (U8)i;
      memcpy(ac.pend[ac.np].out, out, AO_NR);
      ac.np++;
    }
  }
}

// the counts of the orders the frame had (ac's pending ones)
static void ao_count(AoCtx& ac) {
  for (int i = 0; i < ac.np; i++) {
    U32* n = ac.st->n[ac.pend[i].set][ac.pend[i].oc];
    U32 mx = 0;
    for (int j = 0; j < AO_NR; j++) {
      if (ac.pend[i].out[j] == ac.pend[i].act) n[j]++;
      if (n[j] > mx) mx = n[j];
    }
    if (ac.st->v14 && mx > AO_LIM)
      for (int j = 0; j < AO_NR; j++) n[j] = (n[j] + 1) >> 1;
  }
  ac.np = 0;
  for (int i = 0; i < ac.ndp; i++) ac.st->dcp[ac.dpend[i].set][ac.dpend[i].i] = ac.dpend[i].f;
  ac.ndp = 0;
}

// The reflection coefficients' codes in the header h (x's) against rc_predict()'s from the
// decorrelated MSB signals msb (xll_msb()'s), as the meta file keeps them (dir 1) and back (-1): the
// difference of their table indexes, zigzagged (as 8 bits: from version 8), or (dir 0) XORed with
// them (versions 5 to 7)
static void rc_xor(U8* h, const Xll& x, const BandData& msb, int dir, bool fl, bool f15, bool f20) {
  int code[XORD];
  for (int b = 0; b < x.nbands; b++)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      if (s.nbands <= b) continue;
      const Band& d = s.band[b];
      for (int ch = 0; ch < s.nch; ch++) {
        if (!d.aorder[ch]) continue;
        rc_predict(msb.at(b, x.chofs[c] + ch), x.nframesamples, d.aorder[ch], code, fl, f15, f20);
        for (int j = 0; j < d.aorder[ch]; j++) {
          const int at = d.rcat[ch] + 8 * j;
          const U32 v = bits_at(h, at, 8);
          U32 nv = v ^ (U32)code[j];
          if (dir) {
            const int q = unzig(v) + (dir > 0 ? -unzig((U32)code[j]) : unzig((U32)code[j]));
            nv = zig((S32)(S8)(U8)q);  // (within -128..127, as 8 bits wrap)
          }
          bits_xor(h, at, 8, v ^ nv);
        }
      }
    }
}

enum { VERSION = 27 };  // (the join reads 26 too, the orders' rule with the frame's first samples at the
                        // residuals' variance, 25, the common header's size and segment fields as the
                        // others, 24, the pairs' flags without dc_bits(), 23, the orders'
                        // rule stopped where the recursion breaks down, 22, the pairs' flags without dc_gain() and the ranks without ac_rx(), 21, without
                        // the DTS:X heights folded out of the bed,
                        // 20, with the common flags as they are, format 17's
                        // Rice and hybrid rules and the Rice widths by one rule, 19, with the
                        // coding parameters as bytes, 18, with
                        // the headers as their bytes, 17, with the ranks
                        // and the pairs' flags in the headers, 16, with the orders as their
                        // difference, 15, with its
                        // sections one stream, 14, with the autocorrelation in integers, 13, with
                        // ao_pred()'s first rule, 12, with the orders and widths XORed, 11, with the
                        // headers' sizes, 10, with the isolated samples kept, 9, with rc_code()'s
                        // nearest entries, 8, with the prediction orders as they are, 7, with the
                        // channel set header sizes, 6, with the frame sizes, 5, with the code widths
                        // as they are, 4, with the headers as they are, 3, without prefix.x.wav, and
                        // 2, without prefix.dtsx)

// The segments' code widths (a_code, b_code) against those their residuals res say, as the meta file
// keeps them (join: back), from version 13 (ver) as their difference (dif_of()), before XORed with
// them: a Rice code's, floor(log2) of the mean of the zigzagged residuals it
// codes (a hybrid Rice code's but for its isolated samples); a binary code's the least that holds
// them. The encoders' rule, it seems: 99.2% of the Rice widths of s.2.dts and of the Cymatics stream
// past a segment's first residuals (those before the prediction's order), 98-99% of the first
// ones' (96% in master_audio_7.1_24bit.dts), and all but a few binary widths. From version 11
// (ver) a hybrid Rice code's isolated samples are those whose Rice code, (z >> k) + 1 + k bits,
// would be longer than theirs, aux bits and the position's nseg_log2 (all of those of the Cymatics
// stream and master_audio_7.1_24bit.dts): where they are so (isopred), the meta file keeps none of
// them, and the join puts them back here, after the widths. Their width aux is against that of the
// largest residual (99.2%), and the Rice width against that of the residuals but those a first guess
// would isolate (k the floor of the log2 of the mean of them all): it needs no isolated samples.
// From version 21 the width of a Rice code of the residuals before the order (the samples
// themselves) is by the mean of 2|r| - 1, 0s as 0 (their sum less the count of the positive ones):
// the Cymatics stream's misses there 3066 -> 987, s2_mid.dts's 8 -> 0; past them a second rule, by
// their sum less half the count of the positive and 0 ones (a hybrid code's: less the positive
// ones'), is the context of the widths where it differs (the misses it would make 3751 of the
// first rule's 5257 there in the Cymatics stream; the misses are where the mean is just past a
// power of 2, more of them the smaller it is, as if the encoders' sum were a little smaller).
static int iso_rule(const S32* e, int n, int k, int aux, int lg, std::vector<U16>& iso) {
  iso.clear();
  for (int j = 0; j < n; j++)
    if ((U64)(zig(e[j]) >> k) + 1 + (U64)k > (U64)aux + (U64)lg) iso.push_back((U16)j);
  return (int)iso.size();
}
// (from version 20) the bits a Rice code of the residuals uu takes at the widths pred-1 and pred+1
// against pred's, each as a bucket of 16ths of a bit a residual: 0 fewer, 1-8 more (8: 7/16 or more),
// 9 where there is no such width
static U32 w_bucket(S64 d, U64 n) {
  if (d < 0) return 0;
  const U64 q = (U64)d * 16 / n;
  return 1 + (U32)(q < 7 ? q : 7);
}
static U32 w_margin(const std::vector<U32>& uu, int pred, int top) {
  const U64 n = uu.size();
  if (!n) return 9 | 9 << 4;
  U64 c0 = 0, cm = 0, cp = 0;
  for (size_t j = 0; j < n; j++) {
    const U32 u = uu[j];
    c0 += u >> pred;
    if (pred) cm += u >> (pred - 1);
    if (pred < top && pred < 31) cp += u >> (pred + 1);
  }
  const S64 b0 = (S64)c0 + (S64)n * (pred + 1);
  const U32 bm = pred ? w_bucket((S64)cm + (S64)n * pred - b0, n) : 9;
  const U32 bp = pred < top && pred < 31 ? w_bucket((S64)cp + (S64)n * (pred + 2) - b0, n) : 9;
  return bm | bp << 4;
}
static void ck_xor(const Xll& x, std::vector<Chunk>& ck, const BandData& res, int ver, bool join, HdrCoder* hc = 0) {
  const int S = x.nsegsamples;
  const bool dif = ver >= 13;
  std::vector<U16> rule;
  std::vector<U32> uu;  // (from version 20: the residuals a width is by, for its margins)
  for (int b = 0; b < x.nbands; b++)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      if (s.nbands <= b) continue;
      const Band& d = s.band[b];
      int rice[XCH] = {0}, kb[XCH] = {0}, aux[XCH] = {0};  // (the last chunk's not reused: the widths)
      for (int seg = 0; seg < x.nsegs; seg++) {
        Chunk& k = ck[((size_t)b * x.nsegs + seg) * x.nchsets + c];
        if (!k.reuse) {
          const int np = k.seg_common ? 1 : s.nch, top = (1 << s.nabits) - 1;
          for (int i = 0; i < np; i++) {
            const bool hy = k.rice[i] && k.hyb[i] && !k.seg_common, h11 = hy && ver >= 11;
            int ax = 0;
            if (h11) {  // (the isolated samples' width: the largest residual's)
              const int na = seg ? 0 : d.aorder[i];
              const S32* e = res.at(b, x.chofs[c] + i) + (size_t)seg * S;
              U32 mx = 0;
              for (int j = na; j < S; j++)
                if (zig(e[j]) > mx) mx = zig(e[j]);
              int w = 0;
              while (w < 32 && (mx >> w)) w++;
              int pa = w ? w - 1 : 0;
              if (pa > top) pa = top;
              const U32 akey = (U32)HL_WID << 16 | (U32)(c & 15) << 12 | (U32)((b & 3) << 2 | 3) << 8 | (U32)i;
              if (hc && join) k.aux_code[i] = (U8)hc->wcode(akey, s.nabits, 0, false, 0x3FF);  // (from version 20)
              const int act = !join ? k.aux_code[i] : dif ? (int)dif_back(k.aux_code[i], (U32)pa, s.nabits) : k.aux_code[i] ^ pa;
              ax = act + 1;
              k.aux_code[i] = (U8)(join ? act : dif ? (int)dif_of((U32)act, (U32)pa, s.nabits) : act ^ pa);
              if (hc && !join) hc->wcode(akey, s.nabits, k.aux_code[i], true, 0x3FF);
            }
            for (int part = seg ? 1 : 0; part < 2; part++) {  // (0 the residuals before the order, 1 the rest)
              U64 sum = 0, n = 0, npos = 0, nzero = 0;  // (npos, nzero: from version 21, the second rule's)
              U32 mx = 0;
              uu.clear();
              for (int ch = 0; ch < s.nch; ch++) {
                if (!k.seg_common && ch != i) continue;
                const int na = seg ? 0 : k.seg_common ? d.highest : d.aorder[ch];
                const S32* e = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
                U8 iso[XSEG];
                memset(iso, 0, sizeof iso);
                const int j0 = part ? na : 0, j1 = part ? S : na < S ? na : S;
                if (part && h11) {  // (the first guess's isolated samples)
                  U64 s0 = 0;
                  for (int j = j0; j < j1; j++) s0 += zig(e[j]);
                  const U64 m0 = j1 > j0 ? s0 / (U64)(j1 - j0) : 0;
                  int k0 = 0;
                  while (m0 >> (k0 + 1)) k0++;
                  iso_rule(e + j0, j1 - j0, k0, ax, x.nseg_log2, rule);
                  for (size_t j = 0; j < rule.size(); j++) iso[rule[j]] = 1;
                } else if (part && hy) {
                  for (size_t j = 0; j < k.iso[ch].size(); j++)
                    if (k.iso[ch][j] < XSEG) iso[k.iso[ch][j]] = 1;
                }
                for (int j = j0; j < j1; j++) {
                  if (part && iso[j - na]) continue;
                  const U32 u = zig(e[j]);
                  sum += u, n++;
                  npos += u && !(u & 1), nzero += !u;
                  if (hc) uu.push_back(u);
                  if (u > mx) mx = u;
                }
              }
              int pred, alt = -1;  // (alt: from version 21, a second rule's, for the context)
              if (k.rice[i]) {
                // (from version 21 the residuals before the order as 2|r| - 1, but 0s as 0: their sum
                // less the positive ones' count; past them a second rule, their sum less half the
                // positive and 0 ones' count, a hybrid code's less the positive ones', for the context)
                const bool fw = ver >= 21;
                const U64 sp = fw && !part ? sum - npos : sum, mean = n ? sp / n : 0;
                pred = 0;
                while (mean >> (pred + 1)) pred++;
                if (fw && part) {
                  const U64 ds = hy ? npos : (npos + nzero) / 2, sa = sum > ds ? sum - ds : 0, ma = n ? sa / n : 0;
                  alt = 0;
                  while (ma >> (alt + 1)) alt++;
                  if (alt > top) alt = top;
                }
              } else {
                int w = 0;
                while (w < 32 && (mx >> w)) w++;
                pred = w >= 2 ? w - 1 : w;
              }
              if (pred > top) pred = top;
              U8& f = part ? k.b_code[i] : k.a_code[i];  // (dif: as their difference, from version 13; before, XORed)
              U32 wm = 0;
              const U32 wkey = (U32)HL_WID << 16 | (U32)(c & 15) << 12 | (U32)((b & 3) << 2 | part) << 8 | (U32)i;
              if (hc) {  // (from version 20 range-coded, by the rule's margins)
                wm = (U32)part | (U32)(k.rice[i] ? 1 : 0) << 1 | (k.rice[i] ? w_margin(uu, pred, top) : 10u | 10u << 4) << 2 |
                     (U32)(alt >= 0 && alt != pred) << 10;
                // (from version 26 a common set's first residuals' width also by how far the largest one's is past
                // the rule's: the encoders' is the rule's and 1 in 66% of s.2.dts's, and 2 in 21%, where theirs
                // is no rule of the residuals' sums, sums of squares or bits that we found)
                if (ver >= 26 && k.seg_common && !part && k.rice[i]) {
                  int lw = 0;
                  while (lw < 32 && (mx >> (lw + 1))) lw++;
                  const int dw = lw - pred;
                  wm |= 1u << 11 | (U32)(dw < 0 ? 0 : dw > 4 ? 4 : dw) << 12;
                }
                if (join) f = (U8)hc->wcode(wkey, s.nabits, 0, false, wm);
              }
              const int act = !join ? f : dif ? (int)dif_back(f, (U32)pred, s.nabits) : f ^ pred;
              if (part) kb[i] = act;
              f = (U8)(join ? act : dif ? (int)dif_of((U32)act, (U32)pred, s.nabits) : act ^ pred);
              if (hc && !join) hc->wcode(wkey, s.nabits, f, true, wm);
            }
            rice[i] = k.rice[i];
            aux[i] = h11 ? ax : 0;
          }
        }
        if (ver >= 11)  // (where the isolated samples are those the rule says)
          for (int ch = 0; ch < s.nch; ch++) {
            if (k.seg_common || !rice[ch] || !aux[ch]) continue;
            const int na = seg ? 0 : d.aorder[ch];
            iso_rule(res.at(b, x.chofs[c] + ch) + (size_t)seg * S + na, S - na, kb[ch], aux[ch], x.nseg_log2, rule);
            if (join) {
              if (k.isopred[ch]) k.iso[ch] = rule;
            } else {
              k.isopred[ch] = k.iso[ch] == rule;
            }
          }
      }
    }
}

// A segment's size as the NAVI table would have it from its MSB part's bits (mbits), the bits kept
// after them and its LSB part: the bytes they take (the meta file keeps the table's sizes against these)
static int navi_size(U64 mbits, const Chunk& k, const Band& d) {
  return (int)((mbits + (k.g1 ? k.g1n : 0) + 7) >> 3) + d.lsb_size;
}

// the NAVI table's sizes in the header h (x's) XORed with pred's, as the meta file keeps them (and
// back): each less 1, in its seg_nbits
static void navi_xor(U8* h, const Xll& x, const std::vector<int>& pred) {
  int pos = x.navi_at * 8;
  for (size_t i = 0; i < pred.size(); i++) {
    const int c = (int)(i % x.nchsets), b = (int)(i / x.nchsets / x.nsegs);
    if (x.cs[c].nbands <= b) continue;
    bits_xor(h, pos, x.seg_nbits, (U32)(pred[i] - 1) & (U32)(((U64)1 << x.seg_nbits) - 1));
    pos += x.seg_nbits;
  }
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

// x's band data from its residuals and LSBs, coded as ck says; 0 or why it can't be. With pred, each
// segment at the size its coding takes (navi_size()), those sizes into pred, all of them within nmax
// bytes (w empty at first)
static const char* xll_write_data(Xll& x, const std::vector<Chunk>& ck, const BandData& res, const BandData& lsb,
                                  BitWriter& w, std::vector<int>* pred = 0, int nmax = 0) {
  const int S = x.nsegsamples;
  if (pred) pred->assign(x.navi.size(), 0);
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++) {
        Chset& s = x.cs[c];
        const size_t ci = ((size_t)b * x.nsegs + seg) * x.nchsets + c;
        if (s.nbands <= b) continue;
        const Band& d = s.band[b];
        const Chunk& k = ck[ci];
        const U64 start = w.bits();
        U64 cend = pred ? (U64)nmax * 8 : start + (U64)x.navi[ci] * 8;
        if (!pred && (U64)x.navi[ci] < (U64)d.lsb_size) return "a segment smaller than its LSB part";
        U64 lstart = pred ? cend : cend - (U64)d.lsb_size * 8;
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
        if (pred) {  // (navi_pred()'s size)
          (*pred)[ci] = navi_size(w.bits() - start, k, d);
          cend = start + (U64)(*pred)[ci] * 8;
          lstart = cend - (U64)d.lsb_size * 8;
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

// DTS:X's heights folded out of the bed (from version 22). FFmpeg's decoder (its master's
// parse_x_heights()) takes a DTS:X stream's 7.1 for a downmix of 7.1.4, each height in the bed
// channel under it at -3 dB: L, R, Lsr and Rsr less Lh, Rh, Lhr and Rhr times 23170/32768, rounded
// (mul15()), are the 7.1.4's bed. Where a frame's object data went to prefix.x.wav (its channels
// those four heights, in that order), the WAV has each of those bed channels the stream's O record
// names so, the join adding the height back as it reads them: the channels coded without the core,
// whose samples are the audio (those coded against it are residuals, the core's decode the fold),
// at the heights' resolution, where the WAV is 32-bit (a sample less a height fits). The
// Cymatics stream's rear surrounds so through FLAC 23% smaller, 51.2 MB to 39.4 MB, its first two
// minutes 49%: their audio was mostly the heights'.
static const U32 XF_WAV[4] = {0x1, 0x2, 0x10, 0x20};  // (the WAV bits of L, R, Lsr, Rsr)
static inline S32 xf_part(S32 h) { return (S32)(((S64)h * 23170 + (1 << 14)) >> 15); }
// the WAV channel of layout L at WAV bit b (-1: none): those with one come first, by their bit
static int wav_ch_at(const Layout& L, U32 b) { return L.mask & b ? __builtin_popcount(L.mask & (b - 1)) : -1; }
// whether the layouts L (the bed's) and O (the object data's) take the fold
static bool xf_layouts(const Layout& L, const Layout& O) {
  return L.bits == 32 && L.nbands == 1 && O.nch == 4 && !O.mask && O.nbands == 1;
}
// the bed channels to fold the heights out of (bit i: XF_WAV[i]'s), by the frame x of layout L and
// its object data's set ox of layout O
static int xf_choose(const Xll& x, const Layout& L, const Xll& ox, const Layout& O) {
  if (!xf_layouts(L, O)) return 0;
  int m = 0;
  for (int i = 0; i < 4; i++) {
    const int w = wav_ch_at(L, XF_WAV[i]);
    if (w < 0) continue;
    const int k = L.perm[w];  // (its XLL channel)
    int c = 0;
    while (c + 1 < x.nchsets && x.chofs[c + 1] <= k) c++;
    if (x.cs[c].resenc >> (k - x.chofs[c]) & 1 && x.cs[c].pcm_res == ox.cs[0].pcm_res) m |= 1 << i;
  }
  return m;
}
// the heights ov (ns samples of O's channels) folded out of the bed's samples wv (ns of L's) in
// the channels m names (sign -1), or back in (+1)
static void xf_fold(S32* wv, const S32* ov, size_t ns, const Layout& L, const Layout& O, int m, int sign) {
  for (int i = 0; i < 4; i++) {
    const int w = m >> i & 1 ? wav_ch_at(L, XF_WAV[i]) : -1;
    if (w < 0) continue;
    for (size_t t = 0; t < ns; t++)  // (mod 2^32: a damaged WAV's values anything)
      wv[t * L.nch + w] = (S32)((U32)wv[t * L.nch + w] + (U32)(sign * xf_part(ov[t * O.nch + i])));
  }
}

static bool layout_same(const Layout& a, const Layout& b) {
  if (a.nch != b.nch || a.bits != b.bits || a.rate != b.rate || a.nbands != b.nbands || a.nchsets != b.nchsets)
    return false;
  for (int c = 0; c < a.nchsets; c++)
    if (a.cs_nch[c] != b.cs_nch[c] || a.cs_mask[c] != b.cs_mask[c]) return false;
  return true;
}

static void put_layout(Rec& o, int tag, int w, const Layout& L) {
  o.u8(tag);
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

// the meta file's sections as the join reads them (before version 16, all the one stream)
struct MetaIn {
  RecIn* s[MS_N];
  RecIn& r() { return *s[MS_REC]; }
};

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

// From version 17 the Rice and hybrid flags of a set of one band are kept XORed with those its
// segment's residuals give (fp_set()), as the encoders choose them, it seems: a parameter set's
// Rice code where its bits, k the floor of the log2 of the residuals' mean (as ck_xor()'s), are no
// more than a binary code's of the largest's width (99.94% of the Rice flags of the Cymatics
// stream's first 60 MB, 99.1% of master_audio_7.1_24bit.dts's); hybrid, of a set's own, where the
// isolated samples iso_rule() would take at that width save bits, counting their count and width
// (864 of the former's 1152 hybrid codes, and 41 that aren't; 567 of the latter's 586, and none
// that aren't). From version 21 (f21) a binary code's bits are those of its true width (a width of
// 1 takes 2), a Rice code of a channel's own pays its hybrid flag, ties go to the binary code
// (the Rice flags' misses on the Cymatics stream 272 -> 210, master_audio_7.1_24bit.dts's 174 ->
// 12), and a hybrid code's bits are hyb_bits()', at the width of the residuals but those a first
// guess would isolate (the hybrid flags' misses 1630 -> 784 and 19 -> 2). The join takes these
// parameters after the residuals, as the flags say which follow; those of a set of two bands
// before them, as before version 17: the band filter needs band 1's history.
struct FpSet {
  int rice, hyb;
  S64 dr, dh;  // (from version 26 the bits each flag's rule chose by: dr Rice less binary, dh hybrid less Rice; FP_NONE: none)
};
static const S64 FP_NONE = (S64)((U64)1 << 63);
// (from version 26) a margin's bucket: 5 none, 4-0 1-2, 3-8, 9-32, 33-128 or more bits below 0, 6-10 above
static U32 fp_bucket(S64 d) {
  const U64 a = d < 0 ? (U64)-d : (U64)d;
  const U32 b = !a ? 0 : a <= 2 ? 1 : a <= 8 ? 2 : a <= 32 ? 3 : a <= 128 ? 4 : 5;
  return d < 0 ? 5 - b : 5 + b;
}
// (from version 21) a binary code's width for residuals whose largest is w bits (zigzagged): the
// code's field holds the width less 1 (0: no bits), so a width of 1 takes 2; at most top + 1
static int lin_width(int w, int top) {
  int p = w >= 2 ? w - 1 : w;
  if (p > top) p = top;
  return p ? p + 1 : 0;
}
// (from version 21) a hybrid Rice code's bits for a channel's residuals e[0..n), as ck_xor()'s rule
// codes them: its width that of the residuals but those a first guess would isolate, its isolated
// samples iso_rule()'s at that width (each aux + lg bits, aux the largest's width), and their count
static U64 hyb_bits(const S32* e, int n, int lg, int top) {
  if (n <= 0) return (U64)lg;
  U32 mx = 0;
  U64 sum = 0;
  for (int j = 0; j < n; j++) {
    const U32 z = zig(e[j]);
    sum += z;
    if (z > mx) mx = z;
  }
  int w = 0;
  while (w < 32 && (mx >> w)) w++;
  const U64 aux = (U64)(w ? (w - 1 > top ? top + 1 : w) : 1), cap = aux + (U64)lg;
  const U64 m0 = sum / (U64)n;
  int k0 = 0;
  while (m0 >> (k0 + 1)) k0++;
  U64 s2 = 0, n2 = 0;
  for (int j = 0; j < n; j++) {
    const U32 z = zig(e[j]);
    if ((U64)(z >> k0) + 1 + (U64)k0 <= cap) s2 += z, n2++;
  }
  const U64 m = n2 ? s2 / n2 : 0;
  int k = 0;
  while (m >> (k + 1)) k++;
  if (k > top) k = top;
  U64 t = (U64)lg;
  for (int j = 0; j < n; j++) {
    const U64 rb = (U64)(zig(e[j]) >> k) + 1 + (U64)k;
    t += rb < cap ? rb : cap;
  }
  return t;
}
static void fp_set(const Xll& x, const BandData& res, int b, int c, int seg, int i, bool common, FpSet& o,
                   bool f21) {
  const int S = x.nsegsamples;
  const Chset& s = x.cs[c];
  const Band& d = s.band[b];
  const int c0 = common ? 0 : i, c1 = common ? s.nch : i + 1;
  U64 sum = 0, n = 0;
  U32 any = 0;  // (the largest one's width is that of them all ORed)
  for (int ch = c0; ch < c1; ch++) {
    const int na = seg ? 0 : common ? d.highest : d.aorder[ch];
    const S32* e = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
    for (int j = na; j < S; j++) {
      const U32 u = zig(e[j]);
      sum += u;
      any |= u;
    }
    if (na < S) n += (U64)(S - na);
  }
  const U64 mean = n ? sum / n : 0;
  int kr = 0, bw = 0;
  while (mean >> (kr + 1)) kr++;
  while (bw < 32 && (any >> bw)) bw++;
  // (a Rice code's bits: (z >> kr) + 1 + kr each; a hybrid one's: those but at most aux + lg, an
  // isolated sample's, and its count's and width's)
  const U64 aux = bw ? (U64)bw : 1, lg = (U64)x.nseg_log2, one = 1 + (U64)kr;
  const bool own = !common;
  const U64 cap = aux + lg > one ? aux + lg - one : 0;  // (z >> kr at most this; past it, aux + lg)
  U64 t1 = 0, t2 = 0;
  for (int ch = c0; ch < c1; ch++) {
    const int na = seg ? 0 : common ? d.highest : d.aorder[ch];
    const S32* e = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
    if (own)
      for (int j = na; j < S; j++) {
        const U64 t = zig(e[j]) >> kr;
        t1 += t;
        t2 += t < cap ? t : cap;
      }
    else
      for (int j = na; j < S; j++) t1 += zig(e[j]) >> kr;
  }
  const U64 rb = t1 + n * one;
  if (f21) {  // (a binary code's true width; a Rice code of its own pays its hybrid flag; ties binary)
    const int top = (1 << s.nabits) - 1;
    const U64 lb = n * (U64)(bw >= 2 ? bw : bw ? 2 : 0);
    o.rice = rb + (own ? 1 : 0) < lb;
    o.hyb = 0;
    o.dr = (S64)(rb + (own ? 1 : 0)) - (S64)lb, o.dh = FP_NONE;
    if (o.rice && own) {
      const int na = seg ? 0 : d.aorder[i] < S ? d.aorder[i] : S;
      const U64 hb = (U64)s.nabits + hyb_bits(res.at(b, x.chofs[c] + i) + (size_t)seg * S + na, S - na, (int)lg, top);
      o.hyb = hb < rb;
      o.dh = (S64)hb - (S64)rb;
    }
    return;
  }
  o.rice = rb <= n * (U64)bw;
  o.hyb = 0;
  if (o.rice && own) {
    const U64 hb = lg + (U64)s.nabits + (aux + lg > one ? t2 + n * one : n * (aux + lg));
    o.hyb = hb < rb;
  }
}
// (from version 21) the bits of a part of nc channels' residuals, es[ch][j0..j1), under one code:
// binary (rice 0) of the largest's width, or Rice at ck_xor()'s width, floor(log2) of their mean
static U64 part_bits(const S32* const* es, int nc, int j0, int j1, int rice, int top) {
  if (j1 <= j0) return 0;
  U64 sum = 0;
  U32 mx = 0;
  for (int ch = 0; ch < nc; ch++)
    for (int j = j0; j < j1; j++) {
      const U32 z = zig(es[ch][j]);
      sum += z;
      if (z > mx) mx = z;
    }
  const U64 n = (U64)nc * (U64)(j1 - j0);
  if (!rice) {
    int w = 0;
    while (w < 32 && (mx >> w)) w++;
    return n * (U64)lin_width(w, top);
  }
  const U64 m = sum / n;
  int k = 0;
  while (m >> (k + 1)) k++;
  if (k > top) k = top;
  U64 t = n * (U64)(1 + k);
  for (int ch = 0; ch < nc; ch++)
    for (int j = j0; j < j1; j++) t += zig(es[ch][j]) >> k;
  return t;
}
// From version 21 whether a segment's parameters are common to its channels is kept against the
// encoders' choice, it seems: common where the segment takes fewer bits so, its parameters and its
// residuals by fp_set()'s codes at ck_xor()'s widths, than with each channel's own (ties their own;
// 99.8% of the Cymatics stream's segments). cm_rule(): that choice, and (mg) a bucket of how many
// bits it is by: 0 more than 32, 1 more than 8, 2 more than 2, 3 1-2, 4 none.
static int cm_rule(const Xll& x, const BandData& res, int b, int c, int seg, U32* mg) {
  const int S = x.nsegsamples, lg = x.nseg_log2;
  const Chset& s = x.cs[c];
  const Band& d = s.band[b];
  const int top = (1 << s.nabits) - 1, nab = s.nabits;
  const S32* es[XCH];
  U64 own = 0;
  for (int ch = 0; ch < s.nch; ch++) {
    es[ch] = res.at(b, x.chofs[c] + ch) + (size_t)seg * S;
    const int na = seg ? 0 : d.aorder[ch] < S ? d.aorder[ch] : S;
    FpSet fp = {0, 0};
    fp_set(x, res, b, c, seg, ch, false, fp, true);
    own += 1 + (U64)(fp.rice ? 1 : 0) + (U64)(fp.hyb ? nab : 0) + (U64)(seg ? 0 : nab) + (U64)nab;
    own += part_bits(&es[ch], 1, 0, na, fp.rice, top);
    own += fp.hyb ? hyb_bits(es[ch] + na, S - na, lg, top) : part_bits(&es[ch], 1, na, S, fp.rice, top);
  }
  FpSet fc = {0, 0};
  fp_set(x, res, b, c, seg, 0, true, fc, true);
  const int nc = seg ? 0 : d.highest < S ? d.highest : S;
  const U64 com = 1 + (U64)(seg ? 0 : nab) + (U64)nab + part_bits(es, s.nch, 0, nc, fc.rice, top) +
                  part_bits(es, s.nch, nc, S, fc.rice, top);
  const U64 g = com < own ? own - com : com - own;
  *mg = g > 32 ? 0 : g > 8 ? 1 : g > 2 ? 2 : g ? 3 : 4;
  return com < own;
}
// (into o[q[0]], o[q[1]] and o[q[2]] the segment's flags, code widths, and isolated samples and the
// rest: from version 16 sections of their own, MS_PAR's or MS_OPAR's; res: the residuals, the flags
// against fp_set()'s, from version 17)
static void put_chunk(Rec* o, const int* q, Xll& x, int b, int seg, int c, const Chunk& k, const BandData* res = 0,
                      HdrCoder* hc = 0) {
  Chset& s = x.cs[c];
  Rec &fl = o[q[0]], &wd = o[q[1]], &is = o[q[2]];
  const U32 kb = (U32)HL_CHK << 16 | (U32)(c & 15) << 12 | (U32)((b & 3) << 1 | (seg ? 1 : 0)) << 8;  // (hc: from version 20)
  U32 mg = 0;  // (from version 21 the common flag against cm_rule()'s, in the context of its margin)
  const int pc = hc && res ? cm_rule(x, *res, b, c, seg, &mg) : 0;
  const U32 f0 = (U32)(k.reuse | (k.reuse ? 0 : (k.seg_common ^ pc) << 1) | k.g1 << 2 | k.g2 << 3);
  if (hc) hc->code(kb | (mg ? 0x80 | mg : 0), 4, f0, true);
  else fl.u8((int)f0);
  if (!k.reuse) {
    s.seg_common = k.seg_common;
    const int np = s.seg_common ? 1 : s.nch;
    for (int i = 0; i < np; i++) {
      FpSet fp = {0, 0, 0, FP_NONE};
      if (res) fp_set(x, *res, b, c, seg, i, s.seg_common, fp, hc != 0);
      const U32 f1 = (U32)((k.rice[i] ^ fp.rice) | (k.hyb[i] ^ fp.hyb) << 1);
      if (hc) {  // (from version 20 the widths are ck_xor()'s to code)
        if (hc->st->v26 && res && hc->st->nfr >= 1024)
          hc->f1code(kb | (U32)(1 + i), f1, true, fp_bucket(fp.dr), fp.dh == FP_NONE ? 11 : fp_bucket(fp.dh));
        else
          hc->code(kb | (U32)(1 + i), 2, f1, true);
      } else {
        fl.u8((int)f1);
        if (k.hyb[i]) wd.u8(k.aux_code[i]);
        if (!seg) wd.u8(k.a_code[i]);
        wd.u8(k.b_code[i]);
      }
      s.rice[i] = k.rice[i];
      s.aux[i] = k.hyb[i];
    }
  }
  for (int ch = 0; ch < s.nch; ch++) {
    const int i = s.seg_common ? 0 : ch;
    if (s.rice[i] && s.aux[i]) {  // (0 where they are those ck_xor()'s rule says, else their count and 1)
      is.uv(k.isopred[ch] ? 0 : k.iso[ch].size() + 1);
      if (!k.isopred[ch])
        for (size_t j = 0; j < k.iso[ch].size(); j++) is.uv(k.iso[ch][j]);
    }
  }
  if (seg == 0 && b == 1) {
    is.u8(k.dh_code);
    for (int ch = 0; ch < s.nch; ch++)
      for (int j = 1; j < XDH; j++) is.sv(k.dh[ch][j]);
  }
  if (k.g1) is.blob(&k.g1b[0], k.g1b.size()), is.uv(k.g1n);
  if (k.g2) is.blob(&k.g2b[0], k.g2b.size()), is.uv(k.g2n);
}

// (from m's sections q[0], q[1] and q[2], put_chunk()'s; before version 16 all one stream; res: the
// residuals the flags are against, from version 17)
static bool get_chunk(MetaIn& m, const int* q, Xll& x, int b, int seg, int c, Chunk& k, int ver, const BandData* res = 0,
                      HdrCoder* hc = 0) {
  Chset& s = x.cs[c];
  RecIn &fl = *m.s[q[0]], &wd = *m.s[q[1]], &is = *m.s[q[2]];
  const U32 kb = (U32)HL_CHK << 16 | (U32)(c & 15) << 12 | (U32)((b & 3) << 1 | (seg ? 1 : 0)) << 8;  // (hc: from version 20)
  U32 mg = 0;
  const bool f21 = hc && ver >= 21;
  const int pc = f21 && res ? cm_rule(x, *res, b, c, seg, &mg) : 0;
  const int f = hc ? (int)hc->code(kb | (mg ? 0x80 | mg : 0), 4, 0, false) : fl.u8();
  if (f >> 4 || (seg == 0 && (f & 1))) return false;
  k.reuse = f & 1;
  k.g1 = f >> 2 & 1;
  k.g2 = f >> 3 & 1;
  if (!k.reuse) {
    s.seg_common = k.seg_common = (f >> 1 & 1) ^ pc;
    const int np = s.seg_common ? 1 : s.nch;
    for (int i = 0; i < np; i++) {
      FpSet fp = {0, 0, 0, FP_NONE};
      if (res) fp_set(x, *res, b, c, seg, i, s.seg_common, fp, f21);
      const int v = hc && hc->st->v26 && res && f21 && hc->st->nfr >= 1024 ? (int)hc->f1code(kb | (U32)(1 + i), 0, false, fp_bucket(fp.dr), fp.dh == FP_NONE ? 11 : fp_bucket(fp.dh))
                    : hc ? (int)hc->code(kb | (U32)(1 + i), 2, 0, false) : fl.u8();
      k.rice[i] = (v & 1) ^ fp.rice;
      k.hyb[i] = (v >> 1 & 1) ^ fp.hyb;
      if (v >> 2 || (k.hyb[i] && (s.seg_common || !k.rice[i]))) return false;
      if (hc) {  // (from version 20 ck_xor() takes the widths)
        k.aux_code[i] = k.a_code[i] = k.b_code[i] = 0;
      } else {
        k.aux_code[i] = k.hyb[i] ? (U8)wd.u8() : 0;
        k.a_code[i] = !seg ? (U8)wd.u8() : 0;
        k.b_code[i] = (U8)wd.u8();
        if (k.aux_code[i] >> s.nabits || k.a_code[i] >> s.nabits || k.b_code[i] >> s.nabits) return false;
      }
      s.rice[i] = k.rice[i];
      s.aux[i] = k.hyb[i];
    }
  } else {
    k.seg_common = s.seg_common;
  }
  for (int ch = 0; ch < s.nch; ch++) {
    const int i = s.seg_common ? 0 : ch;
    k.iso[ch].clear();
    k.isopred[ch] = 0;
    if (s.rice[i] && s.aux[i]) {
      U64 n = is.uv();
      if (ver >= 11) {
        if (!n) {
          k.isopred[ch] = 1;
          continue;
        }
        n--;
      }
      if (n >> x.nseg_log2) return false;
      k.iso[ch].resize(n);
      for (U64 j = 0; j < n; j++) k.iso[ch][j] = (U16)is.uv();
    }
  }
  if (seg == 0 && b == 1) {
    k.dh_code = is.u8();
    if (k.dh_code > 31) return false;
    for (int ch = 0; ch < s.nch; ch++) {
      k.dh[ch][0] = 0;
      for (int j = 1; j < XDH; j++) {
        const S64 v = is.sv();
        if (v != (S32)v) return false;
        k.dh[ch][j] = (S32)v;
      }
    }
  }
  if (k.g1) {
    if (!is.blob(k.g1b, XFRAME)) return false;
    k.g1n = (U32)is.uv();
    k.g1b.resize(k.g1b.size() + PAD);
    if ((U64)k.g1n > (U64)(k.g1b.size() - PAD) * 8) return false;
  }
  if (k.g2) {
    if (!is.blob(k.g2b, XFRAME)) return false;
    k.g2n = (U32)is.uv();
    k.g2b.resize(k.g2b.size() + PAD);
    if ((U64)k.g2n > (U64)(k.g2b.size() - PAD) * 8) return false;
  }
  return !fl.bad && !wd.bad && !is.bad;
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
  // the DTS:X object data as one more channel set (obj_split()): its frame, coding and samples
  Xll ox;
  std::vector<Chunk> ock;
  BandData ores, olsb;
  std::vector<S32> oval;
  Extra oex;
  int oend;       // where its band data ends
  int obj;        // join_x(): whether the record had it split
  int xfmask = 0;  // (from version 22) the bed channels with the heights folded out (xf_choose()'s; the O record's)
  size_t obytes;  // put_x(): its part of the record, in bytes
  // the EXSS header's fields that hold the XLL frame's size, the object data's offset in it or its
  // size (xfq 0, 1, 2), 16 bits from bit xfat (an F record's: xf_apply())
  std::vector<int> xfat, xfq;
  // the prediction orders' rule (ao_pred()): its counts (aost: those it takes, the split's the
  // check's), the frame's and the object data's samples as the WAVs have them (pv, opv: [XLL
  // channel][sample]) and the join's contexts (the frame's, the object data's)
  AoState ao;
  AoState* aost;
  std::vector<S32> pv, opv;
  AoCtx aoc, aoco;
  AcIo* acput;  // (the split's: where put_hdr() puts the headers' decisions)
  HacIo* hput;   // (and from version 19 the headers' fields)
  HacIo* cput[2];  // (from version 20 the split's: the coding parameters', the frame's and the object data's)
  HacIo* cio[2];   // (the check's and the join's)
  HdrState* hst;
  bool hauto;  // (the join's: the fields' last values once a frame is through; the check's are the split's)
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

// where an XLL frame's DTS:X object data starts (its sync word, at the first DWORD boundary of the
// frame past the band data, where FFmpeg looks for it): its offset in the frame xf, size where none
static int dtsx_at(const U8* xf, int data_end, int size) {
  const int a = (data_end + 3) & ~3;
  if (a + 4 > size) return size;
  const U32 w = be32(xf + a);
  return w == SYNC_XLL_X || (w >> 1) == (SYNC_XLL_X_IMAX >> 1) ? a : size;
}

// DTS:X object data as Nigel_Stanford_Cymatics_DTS-X.dts has it: its sync word and 15 bytes with
// their CRC-16 (21 bytes, the same in every frame there), a byte (2), then one more XLL channel set
// by the frame's common header (its segments, their sizes' width, the band data CRCs, the LSBs): its
// sub-header (as one without one-to-one speaker mapping), NAVI table and band data, then 0s to the
// end (2 bytes and the DWORD boundary). OBJ_HEAD: the bytes before the sub-header
enum { OBJ_HEAD = 22 };

// the object data ob (n bytes, PAD more) of the XLL frame x as that channel set, its samples into
// sp.oval ([channel][sample]): 0, or why it isn't one
static const char* obj_split(const Xll& x, const U8* ob, int n, Split& sp) {
  if (n < OBJ_HEAD + 3 || crc16(ob, OBJ_HEAD - 1)) return "DTS:X object data of another kind";
  Xll& ox = sp.ox;
  const char* why = xll_headers(ox, ob, n, 0, &x, OBJ_HEAD);
  if (why || (why = xll_supported(ox))) return why;
  sp.ores.init(ox.nframesamples, ox.nch);
  sp.olsb.init(ox.nframesamples, ox.nch);
  const Xll hx = ox;  // (the band data parse changes the coding state)
  if ((why = xll_data(ox, ob, sp.ock, sp.ores, sp.olsb, sp.oend))) return why;
  ox = hx;
  sp.oval.assign((size_t)ox.nframesamples * ox.nbands * ox.nch, 0);
  xll_decode(ox, sp.ock, sp.ores, sp.olsb, &sp.oval[0], sp.oex, sp.tmp);
  return 0;
}

// the values clipped in a WAV (where in its samples, and the value), into o; and back into wv
static void put_exc(Rec& o, const std::vector<std::pair<U64, S32> >& exc) {
  o.uv(exc.size());
  U64 last = 0;
  for (size_t i = 0; i < exc.size(); i++) {
    o.uv(exc[i].first - last);
    o.sv(exc[i].second);
    last = exc[i].first;
  }
}
static const char* get_exc(RecIn& in, std::vector<S32>& wv) {
  const U64 nexc = in.uv();
  if (in.bad || nexc > wv.size()) return "a damaged record";
  U64 at = 0;
  for (U64 i = 0; i < nexc; i++) {
    at += in.uv();
    const S64 v = in.sv();
    if (in.bad || at >= wv.size() || v != (S32)v) return "a damaged record";
    wv[at] = (S32)v;
  }
  return 0;
}

// The XLL common header h's frame size field XORed with the frame's size fs (less 1), as the meta
// file keeps it (from version 7) and back: false where fs doesn't fit it
static bool fs_xor(U8* h, int fs) {
  const int fsnb = (int)bits_at(h, 44, 5) + 1;
  if (fs < 1 || (fsnb < 32 && (U32)(fs - 1) >> fsnb)) return false;
  bits_xor(h, 49, fsnb, (U32)(fs - 1));
  return true;
}

// the samples a channel of the XLL frame whose common header h (n bytes) has: 0 where it can't be one
static int xll_nfs(const U8* h, size_t n) {
  if (n < 16 || be32(h) != SYNC_XLL) return 0;
  const int at = 49 + (int)bits_at(h, 44, 5) + 1;
  if ((size_t)(at + 12 + 7) / 8 > n) return 0;
  const int segl = (int)bits_at(h, at + 4, 4), sl = (int)bits_at(h, at + 8, 4);
  if (!sl || segl > 10 || (1 << sl) > XSEG || ((1 << sl) << segl) > XFRAME) return 0;
  return (1 << sl) << segl;
}

// ac (its samples val) for samples v ([sample][WAV channel], n a channel) in the layout L: false
// where L isn't one to take them (but then ac has none)
static bool ao_samples(AoCtx& ac, std::vector<S32>& pv, const S32* v, int n, const Layout& L, AoState* st) {
  ac.val = 0;
  ac.np = 0;
  ac.ndp = 0;
  const int W = L.nch;
  for (int w = 0; w < W; w++)
    if (L.perm[w] < 0 || L.perm[w] >= W) return false;
  pv.resize((size_t)n * W);
  for (int t = 0; t < n; t++)
    for (int w = 0; w < W; w++) pv[(size_t)L.perm[w] * n + t] = v[(size_t)t * W + w];
  ac.val = pv.empty() ? 0 : &pv[0];
  ac.nch = W;
  ac.N = n;
  ac.st = st;
  return true;
}

// The 0s after DTS:X object data whose band data ends at oend, as its encoders put them: to the
// DWORD boundary past 2 more bytes
static int opad_std(int oend) { return 2 + ((-(oend + 2)) & 3); }

// The EXSS header fields at sp.xfat (16 bits each) less what they hold (sign 1), as the meta file
// keeps them, or back (-1): the XLL frame's size fs (xfq 0), the DTS:X object data's offset in it (1)
// or its size nx (2)
static void xf_apply(const Split& sp, std::vector<U8>& pre, int fs, int nx, int sign) {
  const int q[3] = {fs, fs - nx, nx};
  const size_t k = sp.xfat.size();
  for (size_t j = 0; j < k; j++) {
    const size_t i = sign > 0 ? j : k - 1 - j;
    const int at = sp.xfat[i];
    if ((size_t)at + 16 > pre.size() * 8) continue;
    const U32 v = bits_at(&pre[0], at, 16), d = (U32)q[sp.xfq[i]];
    bits_xor(&pre[0], at, 16, v ^ ((sign > 0 ? v - d : v + d) & 0xFFFF));
  }
}

// a header's bytes (p, x.data_start of them) into o, its CRC-16s as 0s (xll_headers() puts them
// back), its NAVI table's sizes against those the segments' coding parameters say (ck's), its
// reflection coefficients' codes against those its samples' decorrelated MSB signals (msb) say; not
// their count (from version 12: the join's parse of them gives it, hdr_ahead())
static void put_hdr(Rec& o, const U8* p, const Xll& x, const std::vector<Chunk>& ck, const BandData* msb, int fs = 0,
                    AoCtx* ao = 0, int set0 = 0) {
  std::vector<U8> h(p, p + x.data_start);
  if (fs) fs_xor(&h[0], fs);
  if (ao && ao->val)  // (the prediction orders XORed with ao_pred()'s, band 0's of a set of one band)
    for (int c = 0; c < x.nchsets; c++) {
      const Chset& s = x.cs[c];
      if (s.nbands != 1) continue;
      const Band& d = s.band[0];
      if (d.decor && s.nch > 1) {  // (and the channel order with dc_order()'s, the coefficients with dc_code()'s)
        int chbits = 0;
        while ((1 << chbits) < s.nch) chbits++;
        int op[XCH];
        dc_order(*ao, x.chofs[c], s.nch, op);
        for (int ch = 0; ch < s.nch; ch++) bits_xor(&h[0], d.ordat + chbits * ch, chbits, (U32)op[ch]);
        int hist = 0;  // (from version 18 the flags to MS_AC, their bits 0s: ac_dcf())
        for (int i = 0; i < s.nch / 2; i++) {
          const U32 pc = (U32)dc_code(*ao, s.nch, d.order[2 * i], d.order[2 * i + 1]);
          const int fl = d.dcat[i] >= 0;
          if (fl) bits_xor(&h[0], d.dcat[i], 7, pc);
          if (ao->st->v18 && ao->io) {
            const int gb = ao->st->v23 ? dc_gain(*ao, x.chofs[c], d.order[2 * i], d.order[2 * i + 1], (int)pc) : 0;
            ao->io->put(ac_dcf(set0 + c, i, hist, ao->st->dcp[set0 + c][i], unzig(pc), gb), fl);
            bits_xor(&h[0], d.dfat[i], 1, (U32)fl);
          }
          hist = hist << 1 | fl;
        }
      }
      int pred[XCH];
      U8 rank[XCH][16], mbk[XCH];
      ao_pred(*ao, set0 + c, x.chofs[c], s, s.band[0], pred, false, rank, mbk);
      for (int ch = 0; ch < s.nch; ch++) {  // (as their rank among the orders by the rule's cost)
        const int at = s.band[0].aoat + 4 * ch;
        const U32 v = bits_at(&h[0], at, 4);
        U32 r = 0;
        while (r < 15 && rank[ch][r] != v) r++;
        if (ao->st->v18 && ao->io) {  // (from version 18 to MS_AC, the field 0s: ac_rank_put())
          ac_rank_put(*ao->io, set0 + c, mbk[ch], ao->st->v23 ? ac_rx(rank[ch]) : 0, (int)r);
          bits_xor(&h[0], at, 4, v);
        } else {
          bits_xor(&h[0], at, 4, v ^ r);
        }
      }
    }
  for (int c = 0; c < x.nchsets; c++) bits_xor(&h[0], x.csoff[c] * 8, 10, (U32)(x.csnat[c] - 1) & 1023);
  if (msb) rc_xor(&h[0], x, *msb, 1, true, true, true);
  for (int i = 0; i < x.ncrc; i++) h[x.crcat[i]] = h[x.crcat[i] + 1] = 0;
  std::vector<int> pred(x.navi.size(), 0);
  for (size_t i = 0; i < pred.size(); i++) {
    const int c = (int)(i % x.nchsets), b = (int)(i / x.nchsets / x.nsegs);
    if (x.cs[c].nbands > b) pred[i] = navi_size(ck[i].mbits, ck[i], x.cs[c].band[b]);
  }
  navi_xor(&h[0], x, pred);
  if (ao && ao->hio) {  // (from version 19 the fields as the join's parse will read them, then the gaps: MS_HAC)
    HdrCoder hc;
    hc.start(ao->hio, ao->hst, set0 == XCS);
    for (size_t i = 0; i < x.flog.size(); i++) {
      const HField& f = x.flog[i];
      hc.code(f.key, f.w, bits_at(&h[0], f.pos, f.w), true);
    }
    std::vector<HField> g;
    hdr_gaps(x, (int)h.size(), g);
    for (size_t i = 0; i < g.size(); i++) hc.code(g[i].key, g[i].w, bits_at(&h[0], g[i].pos, g[i].w), true);
    return;
  }
  o.bytes(&h[0], h.size());
}

// an X record's part for the object data obj_split() split (ob, n bytes): its bytes before its band
// data, its segments' coding parameters, its bytes after its band data, its band 0's delayed
// samples, overlapping LSB bits and values clipped in prefix.x.wav (exc)
static void put_obj(Rec* o, const Split& sp, const U8* ob, int n, const std::vector<std::pair<U64, S32> >& exc) {
  Xll x = sp.ox;
  x.crcat[x.ncrc++] = OBJ_HEAD - 3;  // (and its header's: obj_split() has found it holds)
  BandData msb, lsb;
  const bool m = xll_msb(x, sp.ock, &sp.oval[0], sp.oex, msb, lsb);
  AoCtx ac;
  ac.val = sp.opv.empty() ? 0 : &sp.opv[0];
  ac.nch = x.nch;
  ac.N = x.nframesamples;
  ac.st = sp.aost;
  ac.np = 0;
  ac.io = sp.acput;
  ac.ndp = 0;
  ac.hio = sp.hput;
  ac.hst = sp.hst;
  ac.pre = 0;
  put_hdr(o[MS_OHDR], ob, x, sp.ock, m ? &msb : 0, 0, &ac, XCS);
  std::vector<Chunk> kx = sp.ock;  // (the code widths against the residuals')
  std::vector<U64> wt;  // (from version 20 the widths' decisions, after the flags')
  HacIo wio;
  wio.init(&wt, 0, 0, 0);
  HdrCoder hw, hf;
  hw.start(&wio, sp.hst, true);
  hf.start(sp.cput[1], sp.hst, true);
  ck_xor(x, kx, sp.ores, VERSION, false, sp.cput[1] ? &hw : 0);
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      put_chunk(o, MS_OPAR, x, b, seg, 0, kx[(size_t)b * x.nsegs + seg], x.nbands == 1 ? &sp.ores : 0, sp.cput[1] ? &hf : 0);
  if (sp.cput[1] && sp.cput[1]->put_to) sp.cput[1]->put_to->insert(sp.cput[1]->put_to->end(), wt.begin(), wt.end());
  {  // (the bytes after its band data: 0, where they are opad_std()'s 0s, else their count and 1, and them)
    const int pl = n - sp.oend;
    bool z = true;
    for (int i = 0; i < pl; i++) z = z && !ob[sp.oend + i];
    if (z && pl == opad_std(sp.oend)) {
      o[MS_REC].uv(0);
    } else {
      o[MS_REC].uv((U64)pl + 1);
      o[MS_REC].bytes(ob + sp.oend, pl);
    }
  }
  for (size_t i = 0; i < sp.oex.tail.size(); i++) o[MS_REC].sv(sp.oex.tail[i]);
  o[MS_REC].blob(sp.oex.ovl.v.empty() ? (const U8*)"" : &sp.oex.ovl.v[0], sp.oex.ovl.v.size());
  put_exc(o[MS_REC], exc);
}

// an X record into o, the frame's DTS:X object data into xs
static void put_x(Rec* o, int wi, const U8* e, int n, int xoff, const U8* xf, int one2one, Split& sp,
                  const std::vector<std::pair<U64, S32> >& exc, std::vector<U8>& xs, int obj,
                  const std::vector<std::pair<U64, S32> >& oexc, int fold, int xm) {
  Xll& x = sp.x;
  Rec& r = o[MS_REC];
  r.u8('X');
  r.uv(wi);
  if (xm) r.u8(fold);  // (from version 22, once the stream has bed channels to fold the heights out of,
                       // xm, the O record's: whether the WAV has them so in the frame)
  const int xa = dtsx_at(xf, sp.data_end, x.frame_size), hl = xa - sp.data_end;
  std::vector<U8> pre(e, e + xoff);
  exss_fields(&pre[0], pre.size(), n, 1);
  xf_apply(sp, pre, x.frame_size, x.frame_size - xa, 1);
  r.blob(&pre[0], pre.size());
  r.u8(one2one);
  {
    BandData msb, lsb;
    const bool m = xll_msb(x, sp.ck, &sp.val[0], sp.ex, msb, lsb);
    AoCtx ac;
    ac.val = sp.pv.empty() ? 0 : &sp.pv[0];
    ac.nch = x.nch;
    ac.N = x.nframesamples;
    ac.st = sp.aost;
    ac.np = 0;
    ac.io = sp.acput;
    ac.ndp = 0;
    ac.hio = sp.hput;
    ac.hst = sp.hst;
    ac.pre = 0;
    put_hdr(o[MS_HDR], xf, x, sp.ck, m ? &msb : 0, x.frame_size, &ac, 0);
  }
  std::vector<Chunk> kx = sp.ck;  // (the code widths against the residuals')
  std::vector<U64> wt;  // (from version 20 the widths' decisions, after the flags')
  HacIo wio;
  wio.init(&wt, 0, 0, 0);
  HdrCoder hw, hf;
  hw.start(&wio, sp.hst, false);
  hf.start(sp.cput[0], sp.hst, false);
  ck_xor(x, kx, sp.res, VERSION, false, sp.cput[0] ? &hw : 0);
  for (int b = 0; b < x.nbands; b++)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++)
        if (x.cs[c].nbands > b)
          put_chunk(o, MS_PAR, x, b, seg, c, kx[((size_t)b * x.nsegs + seg) * x.nchsets + c], x.nbands == 1 ? &sp.res : 0,
                    sp.cput[0] ? &hf : 0);
  if (sp.cput[0] && sp.cput[0]->put_to) sp.cput[0]->put_to->insert(sp.cput[0]->put_to->end(), wt.begin(), wt.end());
  // the bytes after the band data up to the DTS:X object data or the frame's end: their count and
  // they, or (sg) none, where they are 0s to the DWORD boundary; whether the object data follows
  bool z = true;
  for (int i = 0; i < hl; i++) z = z && !xf[sp.data_end + i];
  const bool sg = z && hl == ((-sp.data_end) & 3);
  r.uv((U64)(xa < x.frame_size) | (U64)sg << 1 | (sg ? 0 : (U64)hl << 2));
  if (!sg) r.bytes(xf + sp.data_end, hl);
  xs.clear();
  sp.obytes = 0;
  if (xa < x.frame_size) {
    r.u8(obj);  // (whether obj_split() split it: its samples in prefix.x.wav; else its bytes in prefix.dtsx)
    if (obj) {
      size_t o0 = 0;  // (its parts: all but the frame's header bytes)
      for (int k = 0; k < MS_N; k++) o0 += k == MS_HDR ? 0 : o[k].b.size();
      put_obj(o, sp, xf + xa, x.frame_size - xa, oexc);
      sp.obytes = 0;
      for (int k = 0; k < MS_N; k++) sp.obytes += k == MS_HDR ? 0 : o[k].b.size();
      sp.obytes -= o0;
    } else {
      r.uv(x.frame_size - xa);
      xs.assign(xf + xa, xf + x.frame_size);
    }
  }
  r.blob(e + xoff + x.frame_size, n - xoff - x.frame_size);
  for (size_t i = 0; i < sp.ex.tail.size(); i++) r.sv(sp.ex.tail[i]);
  r.blob(sp.ex.ovl.v.empty() ? (const U8*)"" : &sp.ex.ovl.v[0], sp.ex.ovl.v.size());
  put_exc(r, exc);
}

// where the join takes an X record's samples from
struct Samples {
  virtual bool get(S32* v, size_t count) = 0;
  virtual ~Samples() {}
};

static const char* const WHY_DTSX = "the DTS:X object data ends early";

// The join's reflection coefficients' codes: back into the header hdr from the meta file's (against
// rc_predict()'s from the decorrelated MSB signals msb; ver: the meta file's version, as they are
// before 5), x the header's again (n, one2one, base, off0: xll_headers()'s)
static const char* rc_join(Xll& x, const BandData& msb, std::vector<U8>& hdr, int n, int one2one, const Xll* base,
                           int off0, int ver) {
  if (ver >= 5) rc_xor(&hdr[0], x, msb, ver >= 8 ? -1 : 0, ver >= 10, ver >= 15, ver >= 20);
  std::vector<U8> fb((size_t)n + PAD, 0);
  memcpy(&fb[0], &hdr[0], hdr.size());
  const char* why = xll_headers(x, &fb[0], n, one2one, base, off0, &fb[0], false, ver >= 7 && !base);
  if (why) return why;
  if ((size_t)x.data_start != hdr.size()) return "a damaged record";
  memcpy(&hdr[0], &fb[0], hdr.size());
  return 0;
}

// What follows an XLL frame's band data, for version 7's join to compute the frame's size fs: 0s to
// the DWORD boundary (sg), else hl bytes, then the DTS:X object data's nx bytes
struct FrameEnd {
  bool sg;
  int hl, nx, fs;
};

// The join's header and band data into w: the band data at the sizes its coding takes, the NAVI
// table's sizes from the meta file's (against those; as they are before version 5, ver) put back
// into the header hdr, from version 7 the frame size too (from fe, which gets the frame's size and
// its gap), its CRC-16s then (xll_headers()), the band data again where a size isn't the least (n,
// one2one, base, off0: xll_headers()'s)
static const char* navi_join(Xll& x, const std::vector<Chunk>& ck, const BandData& res, const BandData& lsb,
                             std::vector<U8>& hdr, int n, int one2one, const Xll* base, int off0, int ver, BitWriter& w,
                             FrameEnd* fe = 0) {
  BitWriter d;
  d.reset();
  std::vector<int> pred;
  Xll x1 = x;
  const char* why = xll_write_data(x1, ck, res, lsb, d, &pred, n);
  if (why) return why;
  if (ver >= 5) navi_xor(&hdr[0], x, pred);
  std::vector<U8> fb((size_t)n + PAD, 0);
  memcpy(&fb[0], &hdr[0], hdr.size());
  if (fe) {  // (the NAVI table's sizes, the frame's size from them)
    if ((why = xll_headers(x, &fb[0], n, one2one, base, off0, &fb[0], false, true))) return why;
    U64 de = (U64)x.data_start;
    for (size_t i = 0; i < x.navi.size(); i++) de += (U64)x.navi[i];
    if (fe->sg) fe->hl = (int)((0 - de) & 3);
    const U64 fs = de + (U64)fe->hl + (U64)fe->nx;
    if (fs > XLL_MAX || !fs_xor(&hdr[0], (int)fs)) return "an XLL frame too large";
    fe->fs = (int)fs;
    n = (int)fs;
    fb.assign((size_t)n + PAD, 0);
    memcpy(&fb[0], &hdr[0], hdr.size());
  }
  if ((why = xll_headers(x, &fb[0], n, one2one, base, off0, &fb[0]))) return why;
  if ((size_t)x.data_start != hdr.size()) return "a damaged record";
  memcpy(&hdr[0], &fb[0], hdr.size());
  w.reset();
  w.bytes(&hdr[0], hdr.size());
  if (x.navi == pred) {
    w.v.insert(w.v.end(), d.v.begin(), d.v.end());
    w.acc = d.acc;
    w.nacc = d.nacc;
    return 0;
  }
  return xll_write_data(x, ck, res, lsb, w);
}

// an X record's part for split object data (put_obj()'s), joined onto out (n bytes) by the frame
// x's common header, its samples from smp in the layout L (none: no prefix.x.wav): 0, or why it can't be
// (version 12) the meta file's next bytes, those a header may take, into fb (0s, n bytes and PAD):
// a first part, or (all, fb as a parse of the first part has left it) n at most; how many
enum { HDR_PART = 4096 };
static size_t hdr_ahead(RecIn& in, std::vector<U8>& fb, size_t n, bool all) {
  if (all) memset(&fb[0], 0, fb.size());
  return in.peek(&fb[0], all || n < HDR_PART ? n : (size_t)HDR_PART);
}

static const char* join_obj(MetaIn& m, const Xll& x, int n, const Layout* L, Samples* smp, Split& sp,
                            std::vector<U8>& out, int ver) {
  RecIn &in = m.r(), &ih = *m.s[MS_OHDR];  // (from version 16 the sections apart)
  std::vector<U8> hdr, tail;
  std::vector<U8> ob((size_t)n + PAD, 0);
  size_t k;  // (the header's bytes: from version 12 those that may be, ahead in the meta file; from 19 none)
  if (ver >= 19) {
    k = 0;
  } else if (ver >= 12) {
    k = hdr_ahead(ih, ob, (size_t)n, false);
  } else {
    if (!ih.blob(hdr, (size_t)n)) return "a damaged record";
    k = hdr.size();
    memcpy(&ob[0], &hdr[0], k);
  }
  Xll& ox = sp.ox;
  if (ver < 19 && k < OBJ_HEAD) return "a damaged record";
  // (from version 9 the samples first: ao_pred() takes them)
  std::vector<S32> wv;
  bool early = false;
  sp.aoco.val = 0;
  sp.aoco.np = sp.aoco.ndp = 0;
  if (ver >= 9 && L && smp && L->nbands == 1) {
    wv.resize((size_t)x.nframesamples * L->nch);
    if (!smp->get(wv.empty() ? 0 : &wv[0], wv.size())) return "prefix.x.wav ends early";
    early = true;
    if (!ao_samples(sp.aoco, sp.opv, wv.empty() ? 0 : &wv[0], x.nframesamples, *L, sp.aost)) return "a damaged layout";
  }
  const char* why = 0;
  if (sp.aoco.io) sp.aoco.io->mark();
  for (int all = 0; all < 2; all++) {  // (version 12: the first part of what's ahead, then all of it)
    if (all) {
      if (ver < 12 || ver >= 19 || k < HDR_PART) break;
      k = hdr_ahead(ih, ob, (size_t)n, true);
      sp.aoco.np = 0;
      sp.aoco.ndp = 0;
      if (sp.aoco.io) sp.aoco.io->rewind();
    }
    if (ver < 19) {
      const U16 hc = crc16(&ob[0], OBJ_HEAD - 3);
      ob[OBJ_HEAD - 3] = (U8)(hc >> 8);
      ob[OBJ_HEAD - 2] = (U8)hc;
    }
    why = xll_headers(ox, &ob[0], n, 0, &x, OBJ_HEAD, &ob[0], true, false, ver >= 8, ver >= 9 ? &sp.aoco : 0,
                      ver >= 13);
    if (!why && (ver >= 19 || (size_t)ox.data_start <= k)) break;
  }
  if (why) return why;
  if (ver >= 19) {  // (the first bytes' CRC-16, now that they are there)
    const U16 hc = crc16(&ob[0], OBJ_HEAD - 3);
    ob[OBJ_HEAD - 3] = (U8)(hc >> 8);
    ob[OBJ_HEAD - 2] = (U8)hc;
    hdr.assign(&ob[0], &ob[0] + ox.data_start);
  } else {
    if (ver >= 12 ? (size_t)ox.data_start > k : (size_t)ox.data_start != k) return "a damaged record";
    if (ver >= 12) {
      hdr.resize((size_t)ox.data_start + PAD);
      ih.bytes(&hdr[0], (size_t)ox.data_start);
      hdr.resize((size_t)ox.data_start);
    }
    memcpy(&hdr[0], &ob[0], hdr.size());
  }
  if ((why = xll_supported(ox))) return why;
  Layout fl;
  layout_of(ox, fl);
  if (!L || !smp || !layout_same(*L, fl)) return "DTS:X object data unlike prefix.x.wav";
  sp.ock.assign((size_t)ox.nbands * ox.nsegs, Chunk());
  HdrCoder hcc;  // (from version 20 the coding parameters' coder)
  HdrCoder* hcp = 0;
  if (ver >= 20 && sp.cio[1]) {
    hcc.start(sp.cio[1], sp.hst, true);
    hcp = &hcc;
  }
  const bool late = ver >= 17 && ox.nbands == 1;  // (from version 17 after the residuals: fp_set())
  if (!late)
    for (int b = 0; b < ox.nbands; b++)
      for (int seg = 0; seg < ox.nsegs; seg++)
        if (!get_chunk(m, ver >= 17 ? MS_OPAR : MS_OPAR16, ox, b, seg, 0, sp.ock[(size_t)b * ox.nsegs + seg], ver, 0, hcp))
          return "a damaged record";
  bool pstd = false;  // (version 7: the 0s after the band data as opad_std() has them)
  if (ver >= 7) {
    const U64 pc = in.uv();
    if (in.bad || pc > 65) return "a damaged record";
    pstd = !pc;
    if (pc) {
      tail.resize(pc - 1 + PAD);
      in.bytes(&tail[0], pc - 1);
      tail.resize(pc - 1);
    }
  } else if (!in.blob(tail, (size_t)n)) {
    return "a damaged record";
  }
  sp.oex.tail.assign(ox.nbands > 1 ? (size_t)ox.nch * 7 : 0, 0);
  for (size_t i = 0; i < sp.oex.tail.size(); i++) {
    const S64 v = in.sv();
    if (v != (S32)v) return "a damaged record";
    sp.oex.tail[i] = (S32)v;
  }
  std::vector<U8> ov;
  if (!in.blob(ov, 1 << 26)) return "a damaged record";
  sp.oex.ovl.reset();
  sp.oex.ovl.v = ov;
  const int NS = ox.nframesamples * ox.nbands, W = L->nch;
  if (!early) {
    wv.resize((size_t)NS * W);
    if (!smp->get(&wv[0], wv.size())) return "prefix.x.wav ends early";
  } else if (wv.size() != (size_t)NS * W) {
    return "a damaged record";
  }
  if ((why = get_exc(in, wv))) return why;
  sp.oval.resize((size_t)NS * W);
  for (int t = 0; t < NS; t++)
    for (int w = 0; w < W; w++) sp.oval[(size_t)L->perm[w] * NS + t] = wv[(size_t)t * W + w];
  BandData msb;
  if (!xll_msb(ox, sp.ock, &sp.oval[0], sp.oex, msb, sp.olsb)) return "prefix.x.wav samples the DTS:X object data can't code";
  if ((why = rc_join(ox, msb, hdr, n, 0, &x, OBJ_HEAD, ver))) return why;
  xll_residuals(ox, msb, sp.ores, sp.tmp);
  if (late)
    for (int seg = 0; seg < ox.nsegs; seg++)
      if (!get_chunk(m, MS_OPAR, ox, 0, seg, 0, sp.ock[seg], ver, &sp.ores, hcp)) return "a damaged record";
  if (ver >= 6) ck_xor(ox, sp.ock, sp.ores, ver, true, hcp);
  BitWriter w;
  if ((why = navi_join(ox, sp.ock, sp.ores, sp.olsb, hdr, n, 0, &x, OBJ_HEAD, ver, w))) return why;
  if (pstd) tail.assign(opad_std((int)w.v.size()), 0);
  w.bytes(tail.empty() ? (const U8*)"" : &tail[0], tail.size());
  if ((ver >= 7 ? w.v.size() > (size_t)n : w.v.size() != (size_t)n) || w.nacc) return "DTS:X object data of another size";
  out.insert(out.end(), w.v.begin(), w.v.end());
  return 0;
}
// An X record (its tag and WAV index read) joined into out: 0, or why it can't be. L: the WAV's
// layout; xs: the DTS:X object data kept as bytes (null: a version 2 meta file, which keeps it in the
// record); ver: the meta file's version; OL, osmp: prefix.x.wav's layout and samples (null: none)
// (from version 19 xll_headers()'s hook: the frame's samples, once its common header has their count)
struct JxPre {
  const Layout* L;
  Samples* smp;
  std::vector<S32>* wv;
  Split* sp;
  bool* early;
};
static const char* jx_pre(void* a, const Xll& x) {
  JxPre& q = *(JxPre*)a;
  if (!q.L || q.L->nbands != 1) return 0;
  std::vector<S32>& wv = *q.wv;
  wv.resize((size_t)x.nframesamples * q.L->nch);
  if (!q.smp->get(wv.empty() ? 0 : &wv[0], wv.size())) return "the WAV ends early";
  *q.early = true;
  if (!ao_samples(q.sp->aoc, q.sp->pv, wv.empty() ? 0 : &wv[0], x.nframesamples, *q.L, q.sp->aost))
    return "a damaged layout";
  return 0;
}

// (from version 22) a frame's bed samples as the join takes them: the WAV's (bed) with the heights
// folded back in, prefix.x.wav's (obj) read along with them into ov, where the object data's join
// takes them (VecSamples)
struct XfSamples : Samples {
  Samples* bed;
  Samples* obj;
  const Layout* L;
  const Layout* O;
  int m;
  std::vector<S32> ov;
  bool get(S32* out, size_t count) {
    const size_t ns = count / L->nch, o0 = ov.size();
    if (count % L->nch || !bed->get(out, count)) return false;
    ov.resize(o0 + ns * O->nch);
    if (ns && !obj->get(&ov[o0], ns * O->nch)) return false;
    if (ns) xf_fold(out, &ov[o0], ns, *L, *O, m, 1);
    return true;
  }
};
// the samples v from at on
struct VecSamples : Samples {
  const std::vector<S32>* v;
  size_t at;
  bool get(S32* out, size_t count) {
    if (count > v->size() - at) return false;
    if (count) memcpy(out, &(*v)[at], count * sizeof *out);
    at += count;
    return true;
  }
};

static const char* join_x(MetaIn& m, const Layout* L, Samples& wsmp, std::vector<U8>& out, Split& sp,
                          std::vector<S32>& wv, RecIn* xs, int ver, const Layout* OL, Samples* wosmp) {
  RecIn &in = m.r(), &ih = *m.s[MS_HDR];  // (from version 16 the sections apart)
  // (from version 22 whether the WAV has the heights folded out of the frame's bed: then the bed's
  // samples come with them back in, and the object data's from what was read along)
  const int fold = ver >= 22 && sp.xfmask ? in.u8() : 0;
  if (in.bad || fold > 1 || (fold && (!L || !OL || !wosmp || !sp.xfmask || !xf_layouts(*L, *OL))))
    return "a damaged record";
  XfSamples xfs;
  VecSamples xfo;
  if (fold) {
    xfs.bed = &wsmp, xfs.obj = wosmp, xfs.L = L, xfs.O = OL, xfs.m = sp.xfmask;
    xfo.v = &xfs.ov, xfo.at = 0;
  }
  Samples& smp = fold ? (Samples&)xfs : wsmp;
  Samples* const osmp = fold ? (Samples*)&xfo : wosmp;
  std::vector<U8> pre, hdr, tail, post, ob;
  if (!in.blob(pre, 1 << 20)) return "a damaged record";
  const int one2one = in.u8();
  // (the XLL frame's size: from version 7, the join's from its parts, XLL_MAX at most)
  const U64 fsize = ver >= 7 ? (U64)XLL_MAX : in.uv();
  if (in.bad || one2one > 1 || fsize > (1 << 20)) return "a damaged record";
  std::vector<U8> fb(fsize + PAD, 0);  // the header, as the frame's start: what xll_headers reads it from
  size_t k;  // (its bytes: from version 12 those that may be, ahead in the meta file; from 19 none)
  if (ver >= 19) {
    k = 0;
  } else if (ver >= 12) {
    k = hdr_ahead(ih, fb, (size_t)fsize, false);
  } else {
    if (!ih.blob(hdr, fsize)) return "a damaged record";
    k = hdr.size();
    memcpy(&fb[0], &hdr[0], k);
  }
  Xll& x = sp.x;
  // (from version 9 the samples first: ao_pred() takes them)
  bool early = false;
  sp.aoc.val = 0;
  sp.aoc.np = sp.aoc.ndp = 0;
  sp.aoco.np = sp.aoco.ndp = 0;
  JxPre jp = {L, &smp, &wv, &sp, &early};
  sp.aoc.pre = 0;
  if (ver >= 19) {  // (the samples once the parse has their count: jx_pre())
    sp.aoc.pre = jx_pre;
    sp.aoc.pre_arg = &jp;
  } else if (ver >= 9 && L && L->nbands == 1) {
    const int nf = xll_nfs(&fb[0], k);
    if (!nf) return "a damaged record";
    wv.resize((size_t)nf * L->nch);
    if (!smp.get(wv.empty() ? 0 : &wv[0], wv.size())) return "the WAV ends early";
    early = true;
    if (!ao_samples(sp.aoc, sp.pv, wv.empty() ? 0 : &wv[0], nf, *L, sp.aost)) return "a damaged layout";
  }
  const char* why = 0;
  if (sp.aoc.io) sp.aoc.io->mark();
  for (int all = 0; all < 2; all++) {  // (version 12: the first part of what's ahead, then all of it)
    if (all) {
      if (ver < 12 || ver >= 19 || k < HDR_PART) break;
      k = hdr_ahead(ih, fb, (size_t)fsize, true);
      sp.aoc.np = 0;
      sp.aoc.ndp = 0;
      if (sp.aoc.io) sp.aoc.io->rewind();
    }
    why = xll_headers(x, &fb[0], (int)fsize, one2one, 0, 0, &fb[0], true, ver >= 7, ver >= 8, ver >= 9 ? &sp.aoc : 0,
                      ver >= 13);
    if (!why && (ver >= 19 || (size_t)x.data_start <= k)) break;
  }
  sp.aoc.pre = 0;
  if (why) return why;
  if (ver >= 19) {
    if ((U64)x.frame_size != fsize) return "a damaged record";
    hdr.assign(&fb[0], &fb[0] + x.data_start);
  } else {
    if ((ver >= 12 ? (size_t)x.data_start > k : (size_t)x.data_start != k) || (U64)x.frame_size != fsize)
      return "a damaged record";
    if (ver >= 12) {
      hdr.resize((size_t)x.data_start + PAD);
      ih.bytes(&hdr[0], (size_t)x.data_start);
      hdr.resize((size_t)x.data_start);
    }
    memcpy(&hdr[0], &fb[0], hdr.size());
  }
  if ((why = xll_supported(x))) return why;
  Layout fl;
  layout_of(x, fl);
  if (!L || !layout_same(*L, fl)) return "an XLL frame unlike its WAV";
  sp.ck.assign((size_t)x.nbands * x.nsegs * x.nchsets, Chunk());
  HdrCoder hcc;  // (from version 20 the coding parameters' coder)
  HdrCoder* hcp = 0;
  if (ver >= 20 && sp.cio[0]) {
    hcc.start(sp.cio[0], sp.hst, false);
    hcp = &hcc;
  }
  const bool late = ver >= 17 && x.nbands == 1;  // (from version 17 after the residuals: fp_set())
  if (!late)
    for (int b = 0; b < x.nbands; b++)
      for (int seg = 0; seg < x.nsegs; seg++)
        for (int c = 0; c < x.nchsets; c++)
          if (x.cs[c].nbands > b &&
              !get_chunk(m, MS_PAR, x, b, seg, c, sp.ck[((size_t)b * x.nsegs + seg) * x.nchsets + c], ver, 0, hcp))
            return "a damaged record";
  FrameEnd fe = {false, 0, 0, 0};
  if (!xs) {
    if (!in.blob(tail, fsize)) return "a damaged record";
  } else if (ver >= 7) {
    const U64 hx = in.uv(), hl = hx >> 2;
    fe.sg = hx >> 1 & 1;
    if (in.bad || hl > fsize || (fe.sg && hl)) return "a damaged record";
    tail.resize(hl + PAD);
    in.bytes(&tail[0], hl);
    tail.resize(hl);
    fe.hl = (int)hl;
    sp.obj = 0;
    if (hx & 1) {
      sp.obj = in.u8();
      if (in.bad || sp.obj > 1) return "a damaged record";
      if (sp.obj) {
        if ((why = join_obj(m, x, XLL_MAX, OL, osmp, sp, ob, ver))) return why;
      } else {
        const U64 nx = in.uv();
        if (in.bad || nx > XLL_MAX) return "a damaged record";
        ob.resize(nx + PAD);
        if (nx) xs->bytes(&ob[0], nx);
        ob.resize(nx);
        if (xs->bad) return WHY_DTSX;
      }
    }
    fe.nx = (int)ob.size();
    if (fold && (!sp.obj || xfo.at != xfs.ov.size())) return "a damaged record";
  } else {
    const U64 hx = in.uv(), hl = hx >> 1;
    if (in.bad || hl > fsize) return "a damaged record";
    tail.resize(hl + PAD);
    in.bytes(&tail[0], hl);
    tail.resize(hl);
    const U64 nx = hx & 1 ? in.uv() : 0;
    if (in.bad || nx > fsize - hl) return "a damaged record";
    sp.obj = nx && ver >= 4 ? in.u8() : 0;
    if (in.bad || sp.obj > 1) return "a damaged record";
    if (sp.obj) {
      if ((why = join_obj(m, x, (int)nx, OL, osmp, sp, tail, ver))) return why;
    } else {
      tail.resize(hl + nx);
      if (nx) xs->bytes(&tail[hl], nx);
      if (xs->bad) return WHY_DTSX;
    }
  }
  if (!in.blob(post, 1 << 20)) return "a damaged record";
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
  if (!early) {
    wv.resize((size_t)NS * W);
    if (!smp.get(&wv[0], wv.size())) return "the WAV ends early";
  } else if (wv.size() != (size_t)NS * W) {
    return "a damaged record";
  }
  if ((why = get_exc(in, wv))) return why;
  sp.val.resize((size_t)NS * W);
  for (int n = 0; n < NS; n++)
    for (int w = 0; w < W; w++) sp.val[(size_t)L->perm[w] * NS + n] = wv[(size_t)n * W + w];
  BandData msb;
  if (!xll_msb(x, sp.ck, &sp.val[0], sp.ex, msb, sp.lsb)) return "WAV samples the XLL frame can't code";
  if ((why = rc_join(x, msb, hdr, (int)fsize, one2one, 0, 0, ver))) return why;
  xll_residuals(x, msb, sp.res, sp.tmp);
  if (late)
    for (int seg = 0; seg < x.nsegs; seg++)
      for (int c = 0; c < x.nchsets; c++)
        if (!get_chunk(m, MS_PAR, x, 0, seg, c, sp.ck[(size_t)seg * x.nchsets + c], ver, &sp.res, hcp)) return "a damaged record";
  if (ver >= 6) ck_xor(x, sp.ck, sp.res, ver, true, hcp);
  BitWriter w;
  if ((why = navi_join(x, sp.ck, sp.res, sp.lsb, hdr, (int)fsize, one2one, 0, 0, ver, w, ver >= 7 && xs ? &fe : 0)))
    return why;
  if (fe.sg) tail.assign(fe.hl, 0);
  w.bytes(tail.empty() ? (const U8*)"" : &tail[0], tail.size());
  w.bytes(ob.empty() ? (const U8*)"" : &ob[0], ob.size());
  if (w.v.size() != (ver >= 7 ? (size_t)fe.fs : fsize) || w.nacc) return "an XLL frame of another size";
  if (ver >= 7) xf_apply(sp, pre, fe.fs, fe.nx, -1);
  if (ver >= 5 && !pre.empty()) exss_fields(&pre[0], pre.size(), (int)(pre.size() + w.v.size() + post.size()), 0);
  ao_count(sp.aoc);  // (the frame's orders to the counts)
  ao_count(sp.aoco);
  if (sp.hst) sp.hst->pfs = (U32)w.v.size();  // (from version 26 the frame's size, the next's contexts)
  if (sp.hauto && sp.hst) hdr_commit(*sp.hst);  // (and from version 19 its headers' fields to their last values)
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
  U64 x_bytes, x_frames;  // prefix.dtsx: the DTS:X object data kept as bytes, and the frames with it
  U64 o_frames;           // the frames whose object data went to prefix.x.wav
  U64 f_frames;           // (of them, those with the heights folded out of the bed: f_mask's channels)
  int f_mask;
  U64 m_obj;              // the meta file: their object data's headers and coding parameters
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

// the split's meta file: its magic and version byte, then blocks of its sections (MS_REC...), each
// ended after the record that takes them to META_BLOCK bytes or more, and at the end
struct MetaOut {
  FileOut f;
  Rec s[MS_N];
  size_t held;
  AcEnc enc, henc;  // (MS_AC's: a record's decisions, coded once it is through; from version 19 MS_HAC's)
  HacModel amodel, hmodel;  // (from version 19 MS_AC's KT too)
  AcEnc cenc[2];  // (from version 20 the coding parameters', the frames' in MS_PFLG, the object data's in MS_OPFLG)
  HacModel cmodel[2];
  U64 nc[2];
  U64 nac, nhac;
  U64 acbytes;   // (the range-coded sections' bytes, all blocks': the headers')
  U64 cbytes[2];  // (and the coding parameters', the frames' and the object data's)
  void open(const std::string& path) {
    amodel.lim = HC_LIMA;
    amodel.size = HC_NA;
    hmodel.lim = HC_LIMH;
    cmodel[0].lim = cmodel[1].lim = HC_LIMC;
    f.open(path);
    for (int k = 0; k < MS_N; k++) s[k].clear();
    held = 0;
    enc.reset();
    henc.reset();
    amodel.reset();
    hmodel.reset();
    for (int j = 0; j < 2; j++) cenc[j].reset(), cmodel[j].reset(), nc[j] = 0;
    nac = nhac = 0;
    acbytes = cbytes[0] = cbytes[1] = 0;
  }
  // an X record's decisions, before write_x(): the orders' and the pairs' (MS_AC), the headers' fields
  // (MS_HAC)
  void ac(const std::vector<U32>& t, const std::vector<U64>& th, const std::vector<U64>* tc) {
    amodel.pend.clear();
    for (size_t i = 0; i < t.size(); i++) {
      const U32 c = t[i] >> 1, pa = ac_par(c);
      enc.bit16(amodel.p16(c, pa), (int)(t[i] & 1));
      amodel.pend.push_back((U64)c | (U64)pa << 24 | (U64)(t[i] & 1) << 48 | (U64)(pa != c) << 49);
    }
    for (size_t i = 0; i < th.size(); i++)
      henc.bit16(hmodel.p16((U32)(th[i] & 0xFFFFFF), (U32)(th[i] >> 24 & 0xFFFFFF)), (int)(th[i] >> 48 & 1));
    amodel.commit();
    hmodel.pend.assign(th.begin(), th.end());
    hmodel.commit();
    nac += t.size();
    nhac += th.size();
    for (int j = 0; j < 2; j++) {
      const std::vector<U64>& c = tc[j];
      for (size_t i = 0; i < c.size(); i++)
        cenc[j].bit16(cmodel[j].p16((U32)(c[i] & 0xFFFFFF), (U32)(c[i] >> 24 & 0xFFFFFF)), (int)(c[i] >> 48 & 1));
      cmodel[j].pend.assign(c.begin(), c.end());
      cmodel[j].commit();
      nc[j] += c.size();
    }
  }
  void head(const Rec& r) { f.write(r); }
  void write(const Rec& r) {  // a record of the MS_REC section's alone
    s[MS_REC].bytes(r.b.empty() ? 0 : &r.b[0], r.b.size());
    held += r.b.size();
    if (held >= META_BLOCK) flush();
  }
  void write_x(const Rec* p) {  // an X record's parts
    for (int k = 0; k < MS_N; k++) {
      s[k].bytes(p[k].b.empty() ? 0 : &p[k].b[0], p[k].b.size());
      held += p[k].b.size();
    }
    if (held >= META_BLOCK) flush();
  }
  void flush() {
    if (!held) return;
    if (nac) {
      enc.finish();
      s[MS_AC].bytes(&enc.out[0], enc.out.size());
    }
    if (nhac) {
      henc.finish();
      s[MS_HAC].bytes(&henc.out[0], henc.out.size());
    }
    for (int j = 0; j < 2; j++)
      if (nc[j]) {
        cenc[j].finish();
        s[j ? MS_OPFLG : MS_PFLG].bytes(&cenc[j].out[0], cenc[j].out.size());
        cbytes[j] += cenc[j].out.size();
      }
    acbytes += s[MS_AC].b.size() + s[MS_HAC].b.size();
    enc.reset();
    henc.reset();
    amodel.reset();
    hmodel.reset();
    for (int j = 0; j < 2; j++) cenc[j].reset(), cmodel[j].reset(), nc[j] = 0;
    nac = nhac = 0;
    Rec h;
    for (int k = 0; k < MS_N; k++) h.uv(s[k].b.size());
    f.write(h);
    for (int k = 0; k < MS_N; k++) f.write(s[k]), s[k].clear();
    held = 0;
  }
  void close() {
    flush();
    f.close();
  }
};

static const U8 MAGIC[8] = {'D', 'T', 'S', 'S', 'P', 'L', 'I', 'T'};
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
  FileOut dts, dtsx;  // (dtsx opened where the first frame has DTS:X object data)
  MetaOut meta;
  std::vector<U8> xpay;     // (the frame's)
  WavOut wav[XNWAV];
  Layout lay[XNWAV];
  int nwav;
  WavOut xwav;  // prefix.x.wav: the object data's samples (xwav_on: opened, in the layout xlay)
  Layout xlay;
  bool xwav_on;
  std::vector<S32> owv;
  ExssState st;
  Stats stt;
  Split sp, chk;
  std::vector<U32> actape;  // (a record's decisions, put_hdr()'s; acw puts them, acr takes them back)
  AcIo acw, acr;
  std::vector<U64> htape;  // (from version 19 its headers' fields: hw puts them, hrd takes them back)
  HacIo hw, hrd;
  std::vector<U64> ctape[2];  // (from version 20 its coding parameters', the frame's and the object data's)
  HacIo cw[2], crd[2];
  HdrState hst;
  std::vector<S32> wv, wv2, wvf;  // (wvf: the WAV's samples with the heights folded out of the bed)
  static CoreFrame cf;
  // the EXSS header fields that hold the XLL frame's size, the DTS:X object data's offset or its size
  // (xf_apply()), found in the first 8 frames of distinct sizes (xf_done then): their EXSS headers
  // (as the meta file keeps them but for those fields) and sizes
  std::vector<std::vector<U8> > probe;
  std::vector<int> probe_fs, probe_nx;
  bool xf_done;

  // the frame's EXSS header e (off bytes; the frame n), XLL frame size fs and object data size nx as a
  // probe, and where these make 8, the fields that hold those sizes, an F record that says where
  void xf_probe(const U8* e, int n, int off, int fs, int nx) {
    for (size_t i = 0; i < probe_fs.size(); i++)
      if (probe_fs[i] == fs && probe_nx[i] == nx) return;
    std::vector<U8> pre(e, e + off);
    exss_fields(&pre[0], pre.size(), n, 1);
    probe.push_back(pre);
    probe_fs.push_back(fs);
    probe_nx.push_back(nx);
    if (probe.size() < 8) return;
    xf_done = true;
    size_t len = probe[0].size();
    for (size_t i = 1; i < probe.size(); i++)
      if (probe[i].size() < len) len = probe[i].size();
    std::vector<int> at, q;
    for (int qi = 0; qi < 3; qi++) {
      U32 qv[8];
      int distinct = 0;
      for (size_t i = 0; i < 8; i++) {
        qv[i] = (U32)(qi == 0 ? probe_fs[i] : qi == 1 ? probe_fs[i] - probe_nx[i] : probe_nx[i]);
        bool seen = false;
        for (size_t k = 0; k < i; k++) seen = seen || qv[k] == qv[i];
        distinct += !seen;
      }
      if (distinct < 3) continue;  // (a size that doesn't change tells nothing)
      // (16 bits less the size the same in every probe; not the sync word nor the CRC)
      for (int a = 32; a + 32 <= (int)len * 8 && at.size() < 16; a++) {
        const U32 d = (bits_at(&probe[0][0], a, 16) - qv[0]) & 0xFFFF;
        bool same = true;
        for (size_t i = 1; i < 8 && same; i++) same = ((bits_at(&probe[i][0], a, 16) - qv[i]) & 0xFFFF) == d;
        if (!same) continue;
        at.push_back(a);
        q.push_back(qi);
        for (size_t i = 0; i < 8; i++) {
          const U32 v = bits_at(&probe[i][0], a, 16);
          bits_xor(&probe[i][0], a, 16, v ^ ((v - qv[i]) & 0xFFFF));
        }
        a += 15;
      }
    }
    probe.clear();
    if (at.empty()) return;
    sp.xfat = chk.xfat = at;
    sp.xfq = chk.xfq = q;
    Rec fr;
    fr.u8('F');
    fr.uv(at.size());
    for (size_t i = 0; i < at.size(); i++) {
      fr.uv(at[i]);
      fr.u8(q[i]);
    }
    meta.write(fr);
  }

  // the WAV of an XLL frame's layout (a new one where it is new): -1 where there are too many
  int wav_of(const Layout& fl) {
    for (int w = 0; w < nwav; w++)
      if (layout_same(lay[w], fl)) return w;
    return nwav < XNWAV ? nwav : -1;
  }

  // the XLL frame of the EXSS frame e (n bytes) at off split, its record into o: 0, or why not
  const char* try_split(const U8* e, int n, int off, int size, int one2one, Rec* o, int& wi) {
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
    // (the DTS:X object data, as one more channel set where it is one, of prefix.x.wav's layout)
    std::vector<std::pair<U64, S32> > oexc;
    const int xa = dtsx_at(&xf[0], sp.data_end, x.frame_size);
    int obj = 0;
    Layout ol;
    if (xa < x.frame_size && !obj_split(x, &xf[xa], x.frame_size - xa, sp)) {
      layout_of(sp.ox, ol);
      if (!xwav_on || layout_same(xlay, ol)) {
        obj = 1;
        const int ONS = sp.ox.nframesamples * sp.ox.nbands;
        owv.resize((size_t)ONS * ol.nch);
        wav_values(ONS, ol, &sp.oval[0], &owv[0], oexc);
      }
    }
    // (the heights folded out of the bed, in the channels the stream's first object audio chose;
    // xm: those in effect, the O record's, or this frame's choice where it writes that)
    const int xm = xwav_on ? sp.xfmask : obj ? xf_choose(x, fl, sp.ox, ol) : 0;
    const int fold = obj && xm && xf_layouts(fl, ol) && sp.ox.nframesamples * sp.ox.nbands == NS;
    if (fold) {
      wvf = wv;
      xf_fold(&wvf[0], &owv[0], (size_t)NS, fl, ol, xm, -1);
    }
    const std::vector<S32>& ww = fold ? wvf : wv;  // (the WAV's)
    if (!xf_done && off >= 8) xf_probe(e, n, off, x.frame_size, xa < x.frame_size ? x.frame_size - xa : 0);
    // (the samples as the WAVs have them, for the prediction orders' rule, as the join will take them)
    sp.pv.clear();
    sp.opv.clear();
    AoCtx tmp;
    tmp.io = 0;
    tmp.hio = 0;
    tmp.pre = 0;
    if (fl.nbands == 1) ao_samples(tmp, sp.pv, &wv[0], x.nframesamples, fl, &chk.ao);
    if (obj && ol.nbands == 1) ao_samples(tmp, sp.opv, &owv[0], sp.ox.nframesamples, ol, &chk.ao);
    for (int k = 0; k < MS_N; k++) o[k].clear();
    actape.clear();
    htape.clear();
    for (int j = 0; j < 2; j++) ctape[j].clear();
    hst.pend.clear();
    put_x(o, wi, e, n, off, &xf[0], one2one, sp, exc, xpay, obj, oexc, fold, xm);
    acr.pos = 0;
    acr.bad = false;
    hrd.pos = 0;
    hrd.bad = false;
    for (int j = 0; j < 2; j++) crd[j].pos = 0, crd[j].bad = false;
    // the check: the record joined back as the join will
    static const U8 none = 0;
    RecIn sec[MS_N];
    MetaIn mi;
    size_t rsize = 0;  // (the record's bytes, all its parts)
    for (int k = 0; k < MS_N; k++) {
      sec[k].init_mem(o[k].b.empty() ? &none : &o[k].b[0], o[k].b.size());
      mi.s[k] = &sec[k];
      rsize += o[k].b.size();
    }
    RecIn& in = sec[MS_REC];
    in.u8();
    in.uv();
    MemSamples ms;
    ms.v = &ww[0];
    ms.n = ww.size();
    std::vector<U8> back;
    RecIn xin;
    xin.init_mem(xpay.empty() ? &none : &xpay[0], xpay.size());
    MemSamples oms;
    oms.v = obj ? &owv[0] : 0;
    oms.n = obj ? owv.size() : 0;
    static AoState aosave;  // (the rule's counts as they were, where the check fails)
    aosave = chk.ao;
    chk.xfmask = xm;
    if (!(why = join_x(mi, &fl, ms, back, chk, wv2, &xin, VERSION, obj ? &ol : 0, &oms)) &&
        (back.size() != (size_t)n || memcmp(&back[0], e, n) || xin.mpos != xin.msize))
      why = "an XLL frame that doesn't join back the same";
    for (int k = 0; k < MS_N; k++)  // (and every part of the record taken)
      if (!why && sec[k].mpos != sec[k].msize) why = "an XLL frame that doesn't join back the same";
    if (!why && (acr.bad || acr.pos != actape.size())) why = "an XLL frame that doesn't join back the same";
    if (!why && (hrd.bad || hrd.pos != htape.size())) why = "an XLL frame that doesn't join back the same";
    for (int j = 0; j < 2; j++)
      if (!why && (crd[j].bad || crd[j].pos != ctape[j].size())) why = "an XLL frame that doesn't join back the same";
    if (why) {
      chk.ao = aosave;
      hst.pend.clear();
      return why;
    }
    hdr_commit(hst);  // (the headers' fields' last values: the frame is through)
    if (wi == nwav) {
      lay[nwav] = fl;
      wav[nwav].open(wav_name(prefix, nwav));
      wav[nwav].nch = fl.nch;
      wav[nwav].bits = fl.bits;
      wav[nwav].rate = fl.rate;
      wav[nwav].mask = fl.mask;
      Rec lr;
      put_layout(lr, 'W', nwav, fl);
      meta.write(lr);
      nwav++;
    }
    wav[wi].write(&ww[0], ww.size());
    stt.exceptions += exc.size();
    stt.f_frames += fold;
    stt.ovl_bits += sp.ex.ovl.v.size() * 8;
    if (obj) {
      if (!xwav_on) {
        xlay = ol;
        xwav.open(prefix + ".x.wav");
        xwav.nch = ol.nch;
        xwav.bits = ol.bits;
        xwav.rate = ol.rate;
        xwav.mask = ol.mask;
        Rec lr;
        put_layout(lr, 'O', 0, ol);
        lr.u8(xm);  // (from version 22: the bed channels with the heights folded out, xf_choose()'s)
        meta.write(lr);
        sp.xfmask = xm;
        stt.f_mask = xm;
        xwav_on = true;
      }
      xwav.write(&owv[0], owv.size());
      stt.exceptions += oexc.size();
      stt.ovl_bits += sp.oex.ovl.v.size() * 8;
      stt.o_frames++;
    }
    const size_t tl = xa - sp.data_end;  // (the record's XLL bytes between the band data and the object data)
    stt.m_exss += n - x.frame_size;
    stt.m_xhdr += tl;  // (and the range-coded sections, the headers', once the meta file is through)
    stt.m_obj += sp.obytes;
    stt.m_param += rsize - (n - x.frame_size) - tl - sp.obytes;
    return 0;
  }

  void exss(const U8* e, int n) {
    int off = 0, size = 0, one2one = 0, wi = 0;
    const char* why = 0;
    stt.exss++;
    Rec o[MS_N];
    if (exss_xll(st, e, n, off, size, one2one, why)) {
      why = 0;
      for (int t = one2one < 0 ? 1 : one2one; t >= (one2one < 0 ? 0 : one2one); t--)  // (unknown: 1, then 0)
        if (!(why = try_split(e, n, off, size, t, o, wi))) break;
      if (!why) {
        meta.ac(actape, htape, ctape);
        meta.write_x(o);
        stt.xsplit++;
        if (xpay.size()) {
          if (!stt.x_frames) dtsx.open(prefix + ".dtsx");
          dtsx.write(&xpay[0], xpay.size());
          stt.x_bytes += xpay.size();
          stt.x_frames++;
        }
        return;
      }
    }
    stt.kept(why ? why : "?");
    Rec& r = o[MS_REC];
    r.clear();
    r.u8('E');
    r.blob(e, n);
    meta.write(r);
    stt.m_kept += r.b.size();
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
    xwav_on = false;
    xf_done = false;
    memset(&chk.ao, 0, sizeof chk.ao);
    chk.ao.v14 = chk.ao.v15 = chk.ao.v17 = chk.ao.v18 = chk.ao.v23 = chk.ao.v24 = chk.ao.v25 = chk.ao.v27 = 1;
    chk.aost = sp.aost = &chk.ao;
    acw.init(&actape, 0, 0, 0);
    acr.init(0, &actape, 0, 0);
    sp.acput = &acw;
    chk.acput = 0;
    sp.aoc.io = sp.aoco.io = 0;
    chk.aoc.io = chk.aoco.io = &acr;
    sp.aoc.ndp = sp.aoco.ndp = chk.aoc.ndp = chk.aoco.ndp = 0;
    hst.reset();
    hst.v26 = 1;
    hw.init(&htape, 0, 0, 0);
    hrd.init(0, &htape, 0, 0);
    sp.hput = &hw;
    chk.hput = 0;
    sp.hst = chk.hst = &hst;
    sp.hauto = chk.hauto = false;
    sp.aoc.hio = sp.aoco.hio = 0;
    chk.aoc.hio = chk.aoco.hio = &hrd;
    sp.aoc.hst = sp.aoco.hst = chk.aoc.hst = chk.aoco.hst = &hst;
    sp.aoc.pre = sp.aoco.pre = chk.aoc.pre = chk.aoco.pre = 0;
    for (int j = 0; j < 2; j++) {
      cw[j].init(&ctape[j], 0, 0, 0);
      crd[j].init(0, &ctape[j], 0, 0);
      sp.cput[j] = &cw[j];
      chk.cput[j] = 0;
      sp.cio[j] = 0;
      chk.cio[j] = &crd[j];
    }
    Rec hr;
    hr.bytes(MAGIC, 8);
    hr.u8(VERSION);
    meta.head(hr);
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
    zr.uv(stt.x_bytes);
    zr.uv(xwav_on ? xwav.frames : 0);
    meta.write(zr);
    for (int w = 0; w < nwav; w++) wav[w].finish();
    if (xwav_on) xwav.finish();
    dts.close();
    meta.close();
    if (stt.x_frames) dtsx.close();
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
  RecIn fin;
  fin.init_file(fm, mname.c_str());
  U8 h[9];
  fin.bytes(h, 9);
  if (fin.bad || memcmp(h, MAGIC, 8)) die(EX_DATA, "%s: not a dtssplit meta file", mname.c_str());
  if (h[8] < 2 || h[8] > VERSION)
    die(EX_DATA, "%s: format version %d, which this dtssplit (version %d) doesn't read", mname.c_str(), h[8], VERSION);
  // (the DTS:X object data, where there is the file: a stream without has none; a version 2 meta file
  // keeps it in its records)
  const std::string xname = prefix + ".dtsx";
  RecIn xin;
  static const U8 none = 0;
  xin.init_mem(&none, 0);
  FILE* fx = h[8] >= 3 ? fopen(xname.c_str(), "rb") : 0;
  if (fx) xin.init_file(fx, xname.c_str());
  Layout lay[XNWAV];
  bool have[XNWAV] = {false};
  WavIn wi[XNWAV];
  FILE* wf[XNWAV] = {0};
  U64 wsamples[XNWAV] = {0};
  Layout olay;  // prefix.x.wav's (have_o: its O record read)
  bool have_o = false;
  WavIn owi;
  FILE* fo = 0;
  U64 osamples = 0;
  const std::string oname = prefix + ".x.wav";
  static Split sp;
  memset(&sp.ao, 0, sizeof sp.ao);
  sp.ao.v14 = h[8] >= 14;
  sp.ao.v15 = h[8] >= 15;
  sp.ao.v17 = h[8] >= 17;
  sp.ao.v18 = h[8] >= 18;
  sp.ao.v23 = h[8] >= 23;
  sp.ao.v24 = h[8] >= 24;
  sp.ao.v25 = h[8] >= 25;
  sp.ao.v27 = h[8] >= 27;
  sp.aost = &sp.ao;
  static AcDec acdec, hacdec;  // (from version 18, MS_AC's: a block's decisions; from 19 MS_HAC's)
  static AcModel acmodel;
  static HacModel hacmodel, kacmodel;  // (from version 19 MS_HAC's, and MS_AC's in acmodel's place)
  static AcDec cdec[2];  // (from version 20 the coding parameters', MS_PFLG's and MS_OPFLG's)
  static HacModel cmodel[2];
  static HacIo cio[2];
  static AcIo acio;
  static HacIo hacio;
  static HdrState hst;
  acdec.init(&none, 0);
  hacdec.init(&none, 0);
  acmodel.reset();
  hacmodel.reset();
  kacmodel.size = HC_NA;
  kacmodel.reset();
  kacmodel.lim = h[8] >= 20 ? HC_LIMA : HC_LIM;  // (version 19: HC_LIM)
  hacmodel.lim = h[8] >= 20 ? HC_LIMH : HC_LIM;
  acio.init(0, 0, &acdec, &acmodel);
  if (h[8] >= 19) acio.kmodel = &kacmodel;
  acio.par = h[8] >= 23;
  for (int j = 0; j < 2; j++) {
    cdec[j].init(&none, 0);
    cmodel[j].reset();
    cmodel[j].lim = HC_LIMC;
    cio[j].init(0, 0, &cdec[j], &cmodel[j]);
    sp.cput[j] = 0;
    sp.cio[j] = h[8] >= 20 ? &cio[j] : 0;
  }
  hacio.init(0, 0, &hacdec, &hacmodel);
  hst.reset();
  hst.v26 = h[8] >= 26;
  sp.acput = 0;
  sp.hput = 0;
  sp.aoc.io = sp.aoco.io = h[8] >= 18 ? &acio : 0;
  sp.aoc.hio = sp.aoco.hio = h[8] >= 19 ? &hacio : 0;
  sp.aoc.hst = sp.aoco.hst = sp.hst = &hst;
  sp.aoc.pre = sp.aoco.pre = 0;
  sp.hauto = true;
  sp.aoc.ndp = sp.aoco.ndp = 0;
  std::vector<S32> wv;
  std::vector<U8> buf;
  U32 crc = 0;
  U64 total = 0;
  // (from version 16 its sections a block at a time, in memory; before, the records read as they come)
  const bool blocks = h[8] >= 16;
  static std::vector<U8> blk[MS_N];
  RecIn bin[MS_N];
  MetaIn mi;
  for (int k = 0; k < MS_N; k++) {
    bin[k].init_mem(&none, 0);
    mi.s[k] = blocks ? &bin[k] : &fin;
  }
  U64 base = 0;  // (where the block's records start in the meta file)
  for (;;) {
    if (blocks && bin[MS_REC].at_end()) {  // (the next block, its records and their parts)
      const U64 b0 = fin.nread;
      for (int k = 1; k < MS_N; k++)
        if (k != MS_AC && k != MS_HAC && !(h[8] >= 20 && (k == MS_PFLG || k == MS_OPFLG)) && !bin[k].at_end())
          damaged("a damaged block", b0);
      if (fin.at_end()) damaged("the meta file ends early", b0);
      U64 len[MS_N] = {0};
      const int ns = h[8] >= 19 ? MS_N : h[8] >= 18 ? MS_N18 : h[8] >= 17 ? MS_N17 : MS_N16;  // (version 18: ten sections, 17: nine, 16: eight)
      for (int k = 0; k < ns; k++) {
        len[k] = fin.uv();
        if (fin.bad || len[k] > ((U64)1 << 30) || (!k && !len[k])) damaged("a damaged block", b0);
      }
      base = fin.nread;
      for (int k = 0; k < MS_N; k++) {  // (a MB at a time: a damaged size takes no more than the file has)
        blk[k].clear();
        for (U64 left = len[k]; left;) {
          const size_t c = left < (1u << 20) ? (size_t)left : (size_t)1 << 20, o = blk[k].size();
          blk[k].resize(o + c);
          fin.bytes(&blk[k][o], c);
          if (fin.bad) damaged("the meta file ends early", b0);
          left -= c;
        }
        bin[k].init_mem(blk[k].empty() ? &none : &blk[k][0], blk[k].size());
      }
      acdec.init(blk[MS_AC].empty() ? &none : &blk[MS_AC][0], blk[MS_AC].size());  // (from version 18)
      hacdec.init(blk[MS_HAC].empty() ? &none : &blk[MS_HAC][0], blk[MS_HAC].size());  // (from version 19)
      for (int j = 0; j < 2; j++) {  // (from version 20)
        const int q = j ? MS_OPFLG : MS_PFLG;
        cdec[j].init(blk[q].empty() ? &none : &blk[q][0], blk[q].size());
        cmodel[j].reset();
      }
      acmodel.reset();
      hacmodel.reset();
      kacmodel.reset();
    }
    RecIn& in = mi.r();
    const U64 at = (blocks ? base : 0) + in.nread;
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
    } else if (tag == 'O') {
      if (h[8] < 4 || have_o || in.uv() != 0 || in.bad || !get_layout(in, olay)) damaged("a damaged layout record", at);
      sp.xfmask = h[8] >= 22 ? in.u8() : 0;  // (the bed channels with the heights folded out)
      if (in.bad || sp.xfmask > 15) damaged("a damaged layout record", at);
      if (!(fo = fopen(oname.c_str(), "rb"))) die(EX_READ, "can't open %s", oname.c_str());
      const char* why = owi.open(fo);
      if (why) die(EX_DATA, "%s: %s", oname.c_str(), why);
      if (owi.nch != olay.nch || owi.bits != olay.bits || owi.rate != olay.rate)
        die(EX_DATA, "%s: %d channels, %d-bit, %d Hz, where the meta file has %d, %d-bit, %d Hz", oname.c_str(), owi.nch,
            owi.bits, owi.rate, olay.nch, olay.bits, olay.rate);
      have_o = true;
      continue;
    } else if (tag == 'F') {
      const U64 k = in.uv();
      if (in.bad || h[8] < 7 || k > 16) damaged("a damaged record", at);
      sp.xfat.clear();
      sp.xfq.clear();
      for (U64 i = 0; i < k; i++) {
        const U64 a = in.uv();
        const int q = in.u8();
        if (in.bad || a > (8u << 20) || q > 2) damaged("a damaged record", at);
        sp.xfat.push_back((int)a);
        sp.xfq.push_back(q);
      }
      continue;
    } else if (tag == 'X') {
      const U64 w = in.uv();
      if (in.bad || w >= XNWAV || !have[w]) damaged("a damaged record", at);
      WavSamples ws, ows;
      ws.w = &wi[w];
      ows.w = &owi;
      const char* why = join_x(mi, &lay[w], ws, buf, sp, wv, h[8] >= 3 ? &xin : 0, h[8], have_o ? &olay : 0, have_o ? &ows : 0);
      if (why == WHY_DTSX && !fx) die(EX_READ, "can't open %s", xname.c_str());
      if (why) damaged(why, at);
      acmodel.commit();  // (the record's decisions to the probabilities)
      hacmodel.commit();
      kacmodel.commit();
      for (int j = 0; j < 2; j++) cmodel[j].commit();
      wsamples[w] += (U64)sp.x.nframesamples * sp.x.nbands;
      if (sp.obj) osamples += (U64)sp.ox.nframesamples * sp.ox.nbands;
    } else if (tag == 'Z') {
      const U64 n = in.uv(), c = in.uv(), nw = in.uv();
      if (in.bad || nw > XNWAV) damaged("a damaged end record", at);
      for (U64 w = 0; w < nw; w++)
        if (in.uv() != wsamples[w]) damaged("sample counts unlike the WAVs'", at);
      const U64 xb = h[8] >= 3 ? in.uv() : 0;  // (prefix.dtsx's size: 0, and a file there is another split's)
      if (xb != xin.nread) damaged("DTS:X object data unlike the meta file's", at);
      if ((h[8] >= 4 ? in.uv() : 0) != osamples) damaged("sample counts unlike prefix.x.wav's", at);
      if (in.bad || !in.at_end()) damaged("a damaged end record", at);
      if (blocks) {  // (the block's sections all taken, and no block after it)
        for (int k = 1; k < MS_N; k++)  // (but MS_AC and MS_HAC: their decoders read no more than they need)
          if (k != MS_AC && k != MS_HAC && !(h[8] >= 20 && (k == MS_PFLG || k == MS_OPFLG)) && !bin[k].at_end())
            damaged("a damaged end record", at);
        if (!fin.at_end()) damaged("a damaged end record", at);
      }
      if (xb && !xin.at_end()) die(EX_DATA, "%s: more than the meta file takes", xname.c_str());
      if (n != total || c != crc) damaged("the stream joined isn't the one split", at);
      if (fgetc(fd) != EOF) die(EX_DATA, "%s: more than the meta file takes", dname.c_str());
      for (U64 w = 0; w < XNWAV; w++) {
        S32 extra;
        if (have[w] && wi[w].read(&extra, 1)) die(EX_DATA, "%s: more samples than the meta file takes", wav_name(prefix, (int)w).c_str());
        if (wf[w]) fclose(wf[w]);
      }
      if (have_o) {
        S32 extra;
        if (owi.read(&extra, 1)) die(EX_DATA, "%s: more samples than the meta file takes", oname.c_str());
        fclose(fo);
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
  if (fx) fclose(fx);
  nbytes = total;
}

// ---------------------------------------------------------------------------------------------

static void usage(void) {
  fputs(
      "dtssplit - lossless split of DTS (DTS-HD Master Audio) streams and the join back\n"
      "Usage: dtssplit c [options] input.dts prefix\n"
      "       dtssplit d [options] prefix restored.dts\n"
      "  c    split: the core frames into prefix.dtsc, the lossless extension's samples into\n"
      "       prefix.wav (and prefix.2.wav, ... for further channel layouts), a DTS:X stream's\n"
      "       object audio into prefix.x.wav (object data of another form into prefix.dtsx),\n"
      "       all else into prefix.meta\n"
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
    static const char* const ext[] = {".dtsc", ".meta", ".wav", ".dtsx", ".x.wav"};
    for (int i = 0; i < 5; i++)
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
    st.x_bytes = st.x_frames = st.o_frames = st.f_frames = 0;
    st.f_mask = 0;
    st.m_exss = st.m_xhdr = st.m_param = st.m_kept = st.m_raw = st.m_obj = 0;
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
      if (sp.xwav_on) {
        static const char* const XF_NAME[4] = {"L", "R", "Lsr", "Rsr"};
        std::string fn;
        for (int i = 0; i < 4; i++)
          if (st.f_mask >> i & 1) fn += std::string(fn.empty() ? "" : ", ") + XF_NAME[i];
        fprintf(stderr, "%s: %llu bytes: %llu samples a channel, %d channels, %d-bit, %d Hz: the DTS:X object audio of %llu XLL frames",
                sp.xwav.name.c_str(), (unsigned long long)sp.xwav.size(), (unsigned long long)sp.xwav.frames,
                sp.xwav.nch, sp.xwav.bits, sp.xwav.rate, (unsigned long long)st.o_frames);
        if (st.f_frames)
          fprintf(stderr, ", in %llu of them folded out of the bed's %s", (unsigned long long)st.f_frames, fn.c_str());
        fprintf(stderr, "\n");
      }
      if (st.x_frames)
        fprintf(stderr, "%s: %llu bytes: the DTS:X object data of %llu XLL frames, as it is\n", sp.dtsx.name.c_str(),
                (unsigned long long)st.x_bytes, (unsigned long long)st.x_frames);
      fprintf(stderr,
              "%s: %llu bytes: extension substream headers %llu, XLL headers %llu, XLL coding parameters %llu, "
              "DTS:X object headers and coding parameters %llu, frames kept as bytes %llu, bytes between frames "
              "%llu, the rest %llu; %llu values clipped, %llu overlapping LSB bits\n",
              sp.meta.f.name.c_str(), (unsigned long long)sp.meta.f.size, (unsigned long long)st.m_exss,
              (unsigned long long)(st.m_xhdr + sp.meta.acbytes), (unsigned long long)(st.m_param + sp.meta.cbytes[0]),
              (unsigned long long)(st.m_obj + sp.meta.cbytes[1]),
              (unsigned long long)st.m_kept, (unsigned long long)st.m_raw,
              (unsigned long long)(sp.meta.f.size - st.m_exss - st.m_xhdr - sp.meta.acbytes - st.m_param - sp.meta.cbytes[0] -
                                   st.m_obj - sp.meta.cbytes[1] - st.m_kept - st.m_raw),
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
