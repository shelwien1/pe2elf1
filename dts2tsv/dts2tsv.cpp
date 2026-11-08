// dts2tsv - lossless converter of DTS streams to TSV text and back
//
// Usage: dts2tsv c|d [options] input output
//   c    DTS stream -> TSV
//   d    TSV -> the stream, byte for byte
//   -v   (c) convert the TSV written back and compare it with the input
//   -q   no statistics
// input and output can be - for stdin and stdout (not with -v).
//
// The stream is cut into frames (DTS core frames, extension substream frames) and what is between
// them. Each frame is parsed by one walk of its syntax, which reads the bits, writes them, or reads
// and writes the record of the frame's elements as text: a core frame down to the quantization
// indexes of its subband samples, an extension substream frame down to its asset descriptors and
// its XLL (lossless) frames down to their residuals. The record is written as text, read back,
// written as bits and compared with the frame: a frame that comes back the same is kept as records,
// one that doesn't (or doesn't parse) as hex, and so is everything between frames. Any input
// therefore converts back to itself, frames or not.
//
// The text, one record a line, tab-separated, '-' for an element the frame has not, '#' comments:
//   H hex          bytes that are not a frame (32 KiB at most a line; longer runs go on)
//   R hex          a frame kept as bytes (after a comment saying why)
//   C ...          a core frame's header; A its audio coding header
//   S sf ...       a subframe's side information; B ch ... a channel's subband side information in
//                  it; Q ch q the quantization indexes of the channel's subband samples in it
//   T tail         the bits of a core frame after its subframes, in hex
//   X ...          an extension substream frame's header; D asset ... an asset's descriptor
//   U asset what   an asset's data kept as bytes (what: core xbr xxch x96 lbr xll aux all rest)
//   L asset ...    an XLL frame's common header; K cs ... a channel set's sub-header, G band ... its
//                  coding of a frequency band; N the NAVI table; P band seg cs ... a segment's coding
//                  parameters; Y ch ... a channel's residuals in the segment
// The columns of each are listed at the top of the text (#C, #A, ...). An element sent more than
// once in a record is a comma-separated list, in the order the frame sends it. A header CRC is 1
// where it holds (and is then computed again), else 0 and the word as it is.
//
// Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
// Profiles), clauses 5, 7 and 8; where FFmpeg's decoder reads the XLL syntax otherwise than the text
// of the specification (the bits of the coding parameters with two frequency bands, bXtraFreqBands),
// as FFmpeg does. The Huffman code books are dtssplit's (../dtssplit/dtscore_huff.inc).
//
// Build: g++ -O2 -o dts2tsv dts2tsv.cpp
//        x86_64-w64-mingw32-g++ -O2 -s -static -o dts2tsv.exe dts2tsv.cpp

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
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
typedef int32_t S32;
typedef uint64_t U64;
typedef int64_t S64;
#define countof(a) (int)(sizeof(a) / sizeof((a)[0]))

enum { EX_OK, EX_USAGE, EX_READ, EX_WRITE, EX_DATA, EX_FAIL };
enum { PAD = 16 };  // the zero bytes a buffer of bits has after its end
enum { SYNC_CORE = 0x7FFE8001, SYNC_EXSS = 0x64582025, SYNC_XLL = 0x41A29547 };

static inline U32 be32(const U8* p) { return (U32)p[0] << 24 | (U32)p[1] << 16 | (U32)p[2] << 8 | p[3]; }
static inline U64 be64(const U8* p) { return (U64)be32(p) << 32 | be32(p + 4); }
// the signed value of a code's unsigned word (XLL), and back
static inline S32 unzig(U32 u) { return u & 1 ? -(S32)(u >> 1) - 1 : (S32)(u >> 1); }
static inline U32 zig(S32 v) { return v < 0 ? (((U32)(-(v + 1))) << 1) | 1 : (U32)v << 1; }
static inline int popcount(U64 x) { return __builtin_popcountll(x); }
// the loudspeakers of a speaker mask (some of its bits are pairs)
static inline int numspk(U64 mask) { return popcount(mask) + popcount(mask & 0xAE66); }

// DTS's CRC-16 (CCITT, from 0xFFFF): 0 over bytes that end with their own CRC
static U16 crc_tab[256];
static void crc_init(void) {
  for (U32 i = 0; i < 256; i++) {
    U32 c = i << 8;
    for (int k = 0; k < 8; k++) c = c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1;
    crc_tab[i] = (U16)c;
  }
}
static U16 crc16(const U8* p, size_t n) {
  U32 c = 0xFFFF;
  while (n--) c = ((c << 8) ^ crc_tab[(c >> 8) ^ *p++]) & 0xFFFF;
  return (U16)c;
}

// ===================================================================================================
// Huffman code books (TS 102 114 Annex D.5)
// ===================================================================================================

struct HuffEntry {
  S16 level;
  U8 len;
  U32 code;
};
#include "../dtssplit/dtscore_huff.inc"

// a code book: decoded by its left-aligned codes (prefix-free and complete, so their ranges tile
// the 32-bit space), coded by level
struct HuffBook {
  int n, minlev, nlev;
  U32 lcode[129];
  S16 lev[129];
  U8 len[129];
  U32 ecode[129];  // by level - minlev
  U8 elen[129];
  void init(const HuffEntry* e, int cnt) {
    n = cnt;
    minlev = 1 << 30;
    int maxlev = -minlev;
    for (int i = 0; i < n; i++) {
      if (e[i].level < minlev) minlev = e[i].level;
      if (e[i].level > maxlev) maxlev = e[i].level;
    }
    nlev = maxlev - minlev + 1;
    memset(elen, 0, sizeof elen);
    for (int i = 0; i < n; i++) {  // (sorted by left-aligned code: an insertion sort of <= 129)
      const U32 lc = e[i].code << (32 - e[i].len);
      int j = i;
      while (j > 0 && lcode[j - 1] > lc) {
        lcode[j] = lcode[j - 1];
        lev[j] = lev[j - 1];
        len[j] = len[j - 1];
        j--;
      }
      lcode[j] = lc;
      lev[j] = e[i].level;
      len[j] = e[i].len;
      ecode[e[i].level - minlev] = e[i].code;
      elen[e[i].level - minlev] = e[i].len;
    }
  }
  // the level whose code starts the 32 bits w, and its length
  int decode(U32 w, int& l) const {
    int lo = 0, hi = n - 1;  // the last entry whose code is <= w
    while (lo < hi) {
      const int m = (lo + hi + 1) >> 1;
      if (lcode[m] <= w)
        lo = m;
      else
        hi = m - 1;
    }
    l = len[lo];
    return lev[lo];
  }
  bool has(S64 v) const { return v >= minlev && v < minlev + nlev && elen[v - minlev]; }
  int bits(S64 v) const { return elen[v - minlev]; }
};

#define HB(x) HB_##x, countof(HB_##x)
static HuffBook QB[10][7];  // quantization indexes: by ABITS-1, then SEL
static HuffBook BA[5];      // ABITS (BHUFF 0-4)
static HuffBook SB[5];      // scale factors (SHUFF 0-4)
static HuffBook TB[4];      // TMODE (THUFF)
static const int QB_GROUP[10] = {1, 3, 3, 3, 3, 7, 7, 7, 7, 7};  // Huffman books per ABITS
static const int SEL_BITS[10] = {1, 2, 2, 2, 2, 3, 3, 3, 3, 3};
static const int BLOCK_LEVELS[7] = {3, 5, 7, 9, 13, 17, 25};  // block codes, ABITS 1-7
static const int BLOCK_BITS[7] = {7, 10, 12, 13, 15, 17, 19};  // a code of 4 samples

static void huff_init(void) {
  QB[0][0].init(HB(A3));
  static const HuffEntry* const q5[] = {HB_A5, HB_B5, HB_C5}, *const q7[] = {HB_A7, HB_B7, HB_C7},
                                   *const q9[] = {HB_A9, HB_B9, HB_C9}, *const q13[] = {HB_A13, HB_B13, HB_C13};
  static const HuffEntry* const q17[] = {HB_A17, HB_B17, HB_C17, HB_D17, HB_E17, HB_F17, HB_G17};
  static const HuffEntry* const q25[] = {HB_A25, HB_B25, HB_C25, HB_D25, HB_E25, HB_F25, HB_G25};
  static const HuffEntry* const q33[] = {HB_A33, HB_B33, HB_C33, HB_D33, HB_E33, HB_F33, HB_G33};
  static const HuffEntry* const q65[] = {HB_A65, HB_B65, HB_C65, HB_D65, HB_E65, HB_F65, HB_G65};
  static const HuffEntry* const q129[] = {HB_A129, HB_B129, HB_C129, HB_D129, HB_E129, HB_F129, HB_G129};
  static const HuffEntry* const* const q[10] = {0, q5, q7, q9, q13, q17, q25, q33, q65, q129};
  static const int lv[10] = {3, 5, 7, 9, 13, 17, 25, 33, 65, 129};
  for (int a = 1; a < 10; a++)
    for (int s = 0; s < QB_GROUP[a]; s++) QB[a][s].init(q[a][s], lv[a]);
  BA[0].init(HB(A12));
  BA[1].init(HB(B12));
  BA[2].init(HB(C12));
  BA[3].init(HB(D12));
  BA[4].init(HB(E12));
  SB[0].init(HB(SA129));
  SB[1].init(HB(SB129));
  SB[2].init(HB(SC129));
  SB[3].init(HB(SD129));
  SB[4].init(HB(SE129));
  TB[0].init(HB(A4));
  TB[1].init(HB(B4));
  TB[2].init(HB(C4));
  TB[3].init(HB(D4));
}

// ===================================================================================================
// Bits: BitIn reads a frame's elements from its bits, BitOut writes them
// ===================================================================================================

static const char CUT[] = "a frame cut short";

// The walk's IO (BitIn, BitOut, TsvIn, TsvOut): each element through one of
//   u(fid, n, v)      n bits, unsigned (n 0-32)          s(fid, n, v)    n bits, two's complement
//   zz(fid, n, v)     n bits of XLL's signed code (none and 0 where n is 0)
//   rice(fid, k, v)   XLL's Rice code: the 0s of the code's word >> k, a 1, its k low bits
//   huff(fid, b, v)   a Huffman code of book b           raw(fid, n, b)  n bits as they are
//   val(fid, lo, hi, v)  a value of the record that isn't an element of the bits as such
//   crc(fo, fw, from, ok, w)  a CRC-16 word, of the bytes from byte from (ok: it holds)
//   fixed(w, n, why)  a word the syntax fixes (a sync word): BitIn checks it
// pos counts the bits the elements take (at either level); open(line, slot, keys) starts a line
// of the text and sel(line, slot) picks the slot (channel) a line's elements go to; peek32() is the
// next 32 bits (BitIn), peek_line() the next line's letter (TsvIn). BITS: the IO has the bits;
// READ: it fills the record.
struct BitIn {
  enum { BITS = 1, READ = 1 };
  const U8* d;
  U64 pos, end;
  const char* err;
  void init(const U8* p, U64 nbits) {
    d = p;
    pos = 0;
    end = nbits;
    err = 0;
  }
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  bool get(int n, U32& v) {
    if (pos + n > end) return fail(CUT);
    const U64 w = be64(d + (pos >> 3)) << (pos & 7);
    v = n ? (U32)(w >> (64 - n)) : 0;
    pos += n;
    return true;
  }
  U32 peek32() const { return pos < end ? (U32)((be64(d + (pos >> 3)) << (pos & 7)) >> 32) : 0; }
  bool u(int, int n, S64& v) {
    U32 x;
    if (!get(n, x)) return false;
    v = x;
    return true;
  }
  bool s(int, int n, S64& v) {
    U32 x;
    if (!get(n, x)) return false;
    v = n ? (S64)((S32)(x << (32 - n)) >> (32 - n)) : 0;
    return true;
  }
  bool zz(int, int n, S64& v) {
    U32 x;
    if (!get(n, x)) return false;
    v = n ? unzig(x) : 0;
    return true;
  }
  bool rice(int, int k, S64& v) {
    U64 q = 0;
    for (;;) {  // the 0s before a 1
      if (pos >= end) return fail(CUT);
      U64 w = be64(d + (pos >> 3)) << (pos & 7);
      const U64 lim = end - pos < 57 ? end - pos : 57;
      w &= ~(~0ULL >> lim);
      if (w) {
        const int z = __builtin_clzll(w);
        pos += z + 1;
        q += z;
        break;
      }
      q += lim;
      pos += lim;
    }
    if (q >> (32 - k)) return fail("a Rice code out of range");
    U32 lo;
    if (!get(k, lo)) return false;
    v = unzig((U32)(q << k) | lo);
    return true;
  }
  bool huff(int, const HuffBook& b, S64& v) {
    if (pos >= end) return fail(CUT);
    int l;
    const int x = b.decode((U32)((be64(d + (pos >> 3)) << (pos & 7)) >> 32), l);
    if (pos + l > end) return fail(CUT);
    pos += l;
    v = x;
    return true;
  }
  bool raw(int, U64 n, std::vector<U8>& b) {
    if (pos + n > end) return fail(CUT);
    b.resize((n + 7) / 8);
    U64 i = 0;
    if (!(pos & 7)) {  // (whole bytes at a byte)
      i = n & ~7ULL;
      if (i) memcpy(&b[0], d + (pos >> 3), i >> 3);
      pos += i;
    }
    for (; i < n; i += 8) {
      const int k = n - i < 8 ? (int)(n - i) : 8;
      U32 x = 0;
      get(k, x);
      b[i >> 3] = (U8)(x << (8 - k));
    }
    return true;
  }
  bool val(int, S64, S64, S64&) { return true; }
  bool crc(int, int, U64 from, S64& ok, S64& w) {
    U32 x;
    if (!get(16, x)) return false;
    w = x;
    ok = !(pos & 7) && !crc16(d + from, (pos >> 3) - from);
    return true;
  }
  bool fixed(U32 w, int n, const char* why) {
    U32 x;
    if (!get(n, x)) return false;
    return x == w || fail(why);
  }
  void open(int, int, int = 0, int = 0, int = 0) {}
  void sel(int, int) {}
  int peek_line() { return 0; }
};

struct BitOut {
  enum { BITS = 1, READ = 0 };
  std::vector<U8> v;
  U64 acc, pos;
  int nacc;
  const char* err;
  void init() {
    v.clear();
    acc = 0;
    nacc = 0;
    pos = 0;
    err = 0;
  }
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  void put(U32 x, int n) {  // n 0-32
    if (!n) return;
    if (n < 32) x &= (1u << n) - 1;
    acc = acc << n | x;
    nacc += n;
    pos += n;
    while (nacc >= 8) {
      nacc -= 8;
      v.push_back((U8)(acc >> nacc));
    }
    acc &= (1ULL << nacc) - 1;
  }
  U32 peek32() const { return 0; }
  bool u(int, int n, S64& x) {
    if (x < 0 || (U64)x >> n) return fail("a value out of range");
    put((U32)x, n);
    return true;
  }
  bool s(int, int n, S64& x) {
    if (x < -(1LL << (n - 1)) || x >= 1LL << (n - 1)) return fail("a value out of range");
    put((U32)x, n);
    return true;
  }
  bool zz(int, int n, S64& x) {
    if (!n) return !x || fail("a value out of range");
    if (x < -(1LL << 31) || x >= 1LL << 31 || (U64)zig((S32)x) >> n) return fail("a value out of range");
    put(zig((S32)x), n);
    return true;
  }
  bool rice(int, int k, S64& x) {
    if (x < -(1LL << 31) || x >= 1LL << 31) return fail("a value out of range");
    const U32 z = zig((S32)x);
    U32 q = z >> k;
    if (q > (1u << 24)) return fail("a Rice code too long");
    for (; q >= 32; q -= 32) put(0, 32);
    put(1, q + 1);
    if (k) put(z & (k < 32 ? (1u << k) - 1 : ~0u), k);
    return true;
  }
  bool huff(int, const HuffBook& b, S64& x) {
    if (!b.has(x)) return fail("a value out of range");
    put(b.ecode[x - b.minlev], b.elen[x - b.minlev]);
    return true;
  }
  bool raw(int, U64 n, std::vector<U8>& b) {
    if (b.size() != (n + 7) / 8) return fail("bits of another length");
    for (U64 i = 0; i < n; i += 8) {
      const int k = n - i < 8 ? (int)(n - i) : 8;
      put(b[i >> 3] >> (8 - k), k);
    }
    return true;
  }
  bool val(int, S64, S64, S64&) { return true; }
  bool crc(int, int, U64 from, S64& ok, S64& w) {
    if (nacc || from > v.size()) return fail("a CRC off a byte");
    put(ok ? crc16(&v[from], v.size() - from) : (U32)w, 16);
    return true;
  }
  bool fixed(U32 w, int n, const char*) {
    put(w, n);
    return true;
  }
  void open(int, int, int = 0, int = 0, int = 0) {}
  void sel(int, int) {}
  int peek_line() { return 0; }
};

// ===================================================================================================
// The syntax: the elements, by their names in TS 102 114, each with the line of the text it goes to
// ===================================================================================================

enum { L_C, L_A, L_S, L_B, L_Q, L_T, L_X, L_D, L_U, L_L, L_K, L_G, L_N, L_P, L_Y, L_NUM };
static const char LETTER[L_NUM + 1] = "CASBQTXDULKGNPY";
// the key columns of each line (before its elements), and its slots (lines of the kind that take
// elements in turn: one a channel)
static const char* const KEYS[L_NUM] = {"", "", "sf", "ch", "ch", "", "", "asset", "asset what", "asset", "cs", "band", "", "band seg cs", "ch"};
static int NKEYS[L_NUM];
enum { NSLOT = 16 };

#define DTS_FIELDS(FX)                                                                                            \
  /* the core frame header */                                                                                    \
  FX(FTYPE, C, "FTYPE") FX(SHORT, C, "SHORT") FX(CPF, C, "CPF") FX(NBLKS, C, "NBLKS") FX(FSIZE, C, "FSIZE")          \
  FX(AMODE, C, "AMODE") FX(SFREQ, C, "SFREQ") FX(RATE, C, "RATE") FX(FixedBit, C, "FixedBit") FX(DYNF, C, "DYNF")    \
  FX(TIMEF, C, "TIMEF") FX(AUXF, C, "AUXF") FX(HDCD, C, "HDCD") FX(EXT_AUDIO_ID, C, "EXT_AUDIO_ID")                 \
  FX(EXT_AUDIO, C, "EXT_AUDIO") FX(ASPF, C, "ASPF") FX(LFF, C, "LFF") FX(HFLAG, C, "HFLAG") FX(HCRC, C, "HCRC")      \
  FX(FILTS, C, "FILTS") FX(VERNUM, C, "VERNUM") FX(CHIST, C, "CHIST") FX(PCMR, C, "PCMR") FX(SUMF, C, "SUMF")        \
  FX(SUMS, C, "SUMS") FX(DIALNORM, C, "DIALNORM")                                                                  \
  /* the primary audio coding header */                                                                          \
  FX(SUBFS, A, "SUBFS") FX(PCHS, A, "PCHS") FX(SUBS, A, "SUBS") FX(VQSUB, A, "VQSUB") FX(JOINX, A, "JOINX")          \
  FX(THUFF, A, "THUFF") FX(SHUFF, A, "SHUFF") FX(BHUFF, A, "BHUFF") FX(SEL, A, "SEL") FX(ADJ, A, "ADJ")              \
  FX(AHCRC, A, "AHCRC")                                                                                           \
  /* a subframe, a channel's subband side information in it, its samples' quantization indexes */               \
  FX(SSC, S, "SSC") FX(PSC, S, "PSC") FX(RANGE, S, "RANGE") FX(SICRC, S, "SICRC") FX(LFE, S, "LFE")                  \
  FX(LFEscaleIndex, S, "LFEscaleIndex")                                                                           \
  FX(PMODE, B, "PMODE") FX(PVQ, B, "PVQ") FX(ABITS, B, "ABITS") FX(TMODE, B, "TMODE") FX(SCALES, B, "SCALES")        \
  FX(JOIN_SHUFF, B, "JOIN_SHUFF") FX(JOIN_SCALES, B, "JOIN_SCALES") FX(HFVQ, B, "HFVQ")                            \
  FX(q, Q, "q") FX(tail, T, "tail")                                                                                \
  /* the extension substream header */                                                                          \
  FX(UserDefinedBits, X, "UserDefinedBits") FX(nExtSSIndex, X, "nExtSSIndex")                                     \
  FX(bHeaderSizeType, X, "bHeaderSizeType") FX(nuExtSSHeaderSize, X, "nuExtSSHeaderSize")                         \
  FX(nuExtSSFsize, X, "nuExtSSFsize") FX(bStaticFieldsPresent, X, "bStaticFieldsPresent")                         \
  FX(nuRefClockCode, X, "nuRefClockCode") FX(nuExSSFrameDurationCode, X, "nuExSSFrameDurationCode")               \
  FX(bTimeStampFlag, X, "bTimeStampFlag") FX(nuTimeStamp, X, "nuTimeStamp") FX(nLSB, X, "nLSB")                    \
  FX(nuNumAudioPresnt, X, "nuNumAudioPresnt") FX(nuNumAssets, X, "nuNumAssets")                                   \
  FX(nuActiveExSSMask, X, "nuActiveExSSMask") FX(nuActiveAssetMask, X, "nuActiveAssetMask")                       \
  FX(bMixMetadataEnbl, X, "bMixMetadataEnbl") FX(nuMixMetadataAdjLevel, X, "nuMixMetadataAdjLevel")               \
  FX(nuBits4MixOutMask, X, "nuBits4MixOutMask") FX(nuNumMixOutConfigs, X, "nuNumMixOutConfigs")                   \
  FX(nuMixOutChMask, X, "nuMixOutChMask") FX(nuAssetFsize, X, "nuAssetFsize")                                     \
  FX(bBcCorePresent, X, "bBcCorePresent") FX(nuBcCoreExtSSIndex, X, "nuBcCoreExtSSIndex")                         \
  FX(nuBcCoreAssetIndex, X, "nuBcCoreAssetIndex") FX(X_Reserved, X, "Reserved") FX(X_crcok, X, "crcok")            \
  FX(X_crc, X, "crc") FX(X_rest, X, "rest")                                                                        \
  /* an audio asset descriptor */                                                                               \
  FX(nuAssetDescriptFsize, D, "nuAssetDescriptFsize") FX(nuAssetIndex, D, "nuAssetIndex")                         \
  FX(bAssetTypeDescrPresent, D, "bAssetTypeDescrPresent") FX(nuAssetTypeDescriptor, D, "nuAssetTypeDescriptor")   \
  FX(bLanguageDescrPresent, D, "bLanguageDescrPresent") FX(LanguageDescriptor, D, "LanguageDescriptor")           \
  FX(bInfoTextPresent, D, "bInfoTextPresent") FX(nuInfoTextByteSize, D, "nuInfoTextByteSize")                     \
  FX(InfoTextString, D, "InfoTextString") FX(nuBitResolution, D, "nuBitResolution")                               \
  FX(nuMaxSampleRate, D, "nuMaxSampleRate") FX(nuTotalNumChs, D, "nuTotalNumChs")                                 \
  FX(bOne2OneMapChannels2Speakers, D, "bOne2OneMapChannels2Speakers")                                            \
  FX(bEmbeddedStereoFlag, D, "bEmbeddedStereoFlag") FX(bEmbeddedSixChFlag, D, "bEmbeddedSixChFlag")               \
  FX(D_bSpkrMaskEnabled, D, "bSpkrMaskEnabled") FX(nuNumBits4SAMask, D, "nuNumBits4SAMask")                       \
  FX(nuSpkrActivityMask, D, "nuSpkrActivityMask") FX(nuNumSpkrRemapSets, D, "nuNumSpkrRemapSets")                 \
  FX(nuStndrSpkrLayoutMask, D, "nuStndrSpkrLayoutMask") FX(nuNumDecCh4Remap, D, "nuNumDecCh4Remap")               \
  FX(nuRemapDecChMask, D, "nuRemapDecChMask") FX(nuSpkrRemapCodes, D, "nuSpkrRemapCodes")                         \
  FX(nuRepresentationType, D, "nuRepresentationType") FX(bDRCCoefPresent, D, "bDRCCoefPresent")                   \
  FX(nuDRCCode, D, "nuDRCCode") FX(bDialNormPresent, D, "bDialNormPresent") FX(nuDialNormCode, D, "nuDialNormCode") \
  FX(nuDRC2ChDmixCode, D, "nuDRC2ChDmixCode") FX(bMixMetadataPresent, D, "bMixMetadataPresent")                   \
  FX(bExternalMixFlag, D, "bExternalMixFlag") FX(nuPostMixGainAdjCode, D, "nuPostMixGainAdjCode")                 \
  FX(nuControlMixerDRC, D, "nuControlMixerDRC") FX(nuLimit4EmbeddedDRC, D, "nuLimit4EmbeddedDRC")                 \
  FX(nuCustomDRCCode, D, "nuCustomDRCCode") FX(bEnblPerChMainAudioScale, D, "bEnblPerChMainAudioScale")           \
  FX(nuMainAudioScaleCode, D, "nuMainAudioScaleCode") FX(nuMixMapMask, D, "nuMixMapMask")                         \
  FX(nuMixCoeffs, D, "nuMixCoeffs") FX(nuCodingMode, D, "nuCodingMode") FX(nuCoreExtensionMask, D, "nuCoreExtensionMask") \
  FX(nuExSSCoreFsize, D, "nuExSSCoreFsize") FX(bExSSCoreSyncPresent, D, "bExSSCoreSyncPresent")                   \
  FX(nuExSSCoreSyncDistInFrames, D, "nuExSSCoreSyncDistInFrames") FX(nuExSSXBRFsize, D, "nuExSSXBRFsize")         \
  FX(nuExSSXXCHFsize, D, "nuExSSXXCHFsize") FX(nuExSSX96Fsize, D, "nuExSSX96Fsize")                               \
  FX(nuExSSLBRFsize, D, "nuExSSLBRFsize") FX(bExSSLBRSyncPresent, D, "bExSSLBRSyncPresent")                       \
  FX(nuExSSLBRSyncDistInFrames, D, "nuExSSLBRSyncDistInFrames") FX(nuExSSXLLFsize, D, "nuExSSXLLFsize")           \
  FX(bExSSXLLSyncPresent, D, "bExSSXLLSyncPresent") FX(nuPeakBRCntrlBuffSzkB, D, "nuPeakBRCntrlBuffSzkB")         \
  FX(nuBitsInitDecDly, D, "nuBitsInitDecDly") FX(nuInitLLDecDlyFrames, D, "nuInitLLDecDlyFrames")                 \
  FX(nuExSSXLLSyncOffset, D, "nuExSSXLLSyncOffset") FX(Reserved1, D, "Reserved1") FX(Reserved2, D, "Reserved2")    \
  FX(nuExSSAuxFsize, D, "nuExSSAuxFsize") FX(nuAuxCodecID, D, "nuAuxCodecID")                                     \
  FX(bExSSAuxSyncPresent, D, "bExSSAuxSyncPresent") FX(nuExSSAuxSyncDistInFrames, D, "nuExSSAuxSyncDistInFrames") \
  FX(nuDTSHDStreamID, D, "nuDTSHDStreamID") FX(D_Reserved, D, "Reserved")                                          \
  /* an asset's data kept as bytes */                                                                            \
  FX(hex, U, "hex")                                                                                               \
  /* the XLL common header */                                                                                    \
  FX(nVersion, L, "nVersion") FX(nHeaderSize, L, "nHeaderSize") FX(nBits4FrameFsize, L, "nBits4FrameFsize")        \
  FX(nLLFrameSize, L, "nLLFrameSize") FX(nNumChSetsInFrame, L, "nNumChSetsInFrame")                               \
  FX(nSegmentsInFrame, L, "nSegmentsInFrame") FX(nSmplInSeg, L, "nSmplInSeg") FX(nBits4SSize, L, "nBits4SSize")    \
  FX(nBandDataCRCEn, L, "nBandDataCRCEn") FX(bScalableLSBs, L, "bScalableLSBs") FX(nBits4ChMask, L, "nBits4ChMask") \
  FX(nuFixedLSBWidth, L, "nuFixedLSBWidth") FX(L_Reserved, L, "Reserved") FX(L_crcok, L, "crcok") FX(L_crc, L, "crc") \
  FX(L_tail, L, "tail") FX(L_rest, L, "rest")                                                                      \
  /* a channel set sub-header */                                                                                 \
  FX(nChSetHeaderSize, K, "nChSetHeaderSize") FX(nChSetLLChannel, K, "nChSetLLChannel")                           \
  FX(nResidualChEncode, K, "nResidualChEncode") FX(nBitResolution, K, "nBitResolution") FX(nBitWidth, K, "nBitWidth") \
  FX(sFreqIndex, K, "sFreqIndex") FX(nFsInterpolate, K, "nFsInterpolate") FX(nReplacementSet, K, "nReplacementSet") \
  FX(bActiveReplaceSet, K, "bActiveReplaceSet") FX(bPrimaryChSet, K, "bPrimaryChSet")                             \
  FX(bDownmixCoeffCodeEmbedded, K, "bDownmixCoeffCodeEmbedded") FX(bDownmixEmbedded, K, "bDownmixEmbedded")       \
  FX(nLLDownmixType, K, "nLLDownmixType") FX(bHierChSet, K, "bHierChSet") FX(DownmixCoeffs, K, "DownmixCoeffs")    \
  FX(bChMaskEnabled, K, "bChMaskEnabled") FX(nChMask, K, "nChMask") FX(bMappingCoeffsPresent, K, "bMappingCoeffsPresent") \
  FX(nBitsCh2SpkrCoef, K, "nBitsCh2SpkrCoef") FX(nNumSpeakerConfigs, K, "nNumSpeakerConfigs")                     \
  FX(nActiveChannelMask, K, "nActiveChannelMask") FX(nNumSpeakers, K, "nNumSpeakers")                             \
  FX(K_bSpkrMaskEnabled, K, "bSpkrMaskEnabled") FX(nSpkrMask, K, "nSpkrMask") FX(RadiusDelta, K, "RadiusDelta")    \
  FX(Theta, K, "Theta") FX(Phi, K, "Phi") FX(Ch2SpkrMapCoeff, K, "Ch2SpkrMapCoeff") FX(bXtraFreqBands, K, "bXtraFreqBands") \
  FX(K_Reserved, K, "Reserved") FX(K_crcok, K, "crcok") FX(K_crc, K, "crc")                                        \
  /* its coding of a frequency band */                                                                          \
  FX(bPWChDecorEnabled, G, "bPWChDecorEnabled") FX(nOrigChanOrder, G, "nOrigChanOrder") FX(bChPFlag, G, "bChPFlag") \
  FX(nPWChPairsCoeffs, G, "nPWChPairsCoeffs") FX(nAdaptPredOrder, G, "nAdaptPredOrder")                           \
  FX(nFixedPredOrder, G, "nFixedPredOrder") FX(nLPCReflCoeffsQInd, G, "nLPCReflCoeffsQInd")                       \
  FX(bEmbDMixInExtBand, G, "bEmbDMixInExtBand") FX(bMSBLSBSplitEnInExtBands, G, "bMSBLSBSplitEnInExtBands")       \
  FX(nLSBFsize, G, "nLSBFsize") FX(nScalableLSBs, G, "nScalableLSBs") FX(bFlagScalableResExtBand, G, "bFlagScalableResExtBand") \
  FX(nBitWidthAdjPerCh, G, "nBitWidthAdjPerCh")                                                                   \
  /* the NAVI table */                                                                                           \
  FX(BandChSetSize, N, "BandChSetSize") FX(N_align, N, "align") FX(N_crcok, N, "crcok") FX(N_crc, N, "crc")         \
  /* a segment of a channel set's band data: its coding parameters, a channel's residuals */                    \
  FX(bUseSegStateCodeParm, P, "bUseSegStateCodeParm") FX(ncSegType, P, "ncSegType") FX(bRiceCodeFlag, P, "bRiceCodeFlag") \
  FX(bHybridRiceFlag, P, "bHybridRiceFlag") FX(ncAuxABIT, P, "ncAuxABIT") FX(ncABIT0, P, "ncABIT0") FX(ncABIT, P, "ncABIT") \
  FX(nNumBitsForHistSampl, P, "nNumBitsForHistSampl") FX(msbrest, P, "msbrest") FX(lsbrest, P, "lsbrest")          \
  FX(nCountIsMax, Y, "nCountIsMax") FX(IsMaxLoc, Y, "IsMaxLoc") FX(res, Y, "res") FX(DeciHistory, Y, "DeciHistory") \
  FX(lsb, Y, "lsb")

#define FID(id, line, name) F_##id,
#define FNAME(id, line, name) name,
#define FLINE(id, line, name) L_##line,
enum { DTS_FIELDS(FID) F_N };
static const char* const field_name[F_N] = {DTS_FIELDS(FNAME)};
static const U8 field_line[F_N] = {DTS_FIELDS(FLINE)};
static int field_col[F_N];  // its column in its line, after the keys
static int line_ncols[L_NUM];
static std::string line_header[L_NUM];

// what an asset's data kept as bytes is (U's second key)
enum { W_CORE, W_XBR, W_XXCH, W_X96, W_LBR, W_XLL, W_AUX, W_ALL, W_REST, W_N };
static const char* const WHAT[W_N] = {"core", "xbr", "xxch", "x96", "lbr", "xll", "aux", "all", "rest"};

static void cols_init(void) {
  for (int l = 0; l < L_NUM; l++) {
    NKEYS[l] = 0;
    line_header[l] = std::string("#") + LETTER[l];
    for (const char* k = KEYS[l]; *k;) {
      const char* e = k;
      while (*e && *e != ' ') e++;
      line_header[l] += '\t';
      line_header[l].append(k, e - k);
      NKEYS[l]++;
      k = *e ? e + 1 : e;
    }
  }
  for (int f = 0; f < F_N; f++) {
    field_col[f] = line_ncols[field_line[f]]++;
    line_header[field_line[f]] += std::string("\t") + field_name[f];
  }
}

// ===================================================================================================
// The record of a frame: its elements as sent, and what follows from them
// ===================================================================================================

enum { NCH = 8, NSUB = 32, NSF = 16, NSSF = 4 };

struct CoreSub {  // a subframe
  int ssc, psc, nssc;
  int pmode[NCH][NSUB], pvq[NCH][NSUB], abits[NCH][NSUB], tmode[NCH][NSUB], scale[NCH][NSUB][2];
  int jsel[NCH], jscale[NCH][NSUB];
  int range, sicrc;
  int hfvq[NCH][NSUB];
  int lfe[16], lfescale;
  S32 q[NCH][NSUB][NSSF * 8];  // the quantization indexes, by subsubframe and sample
};

struct CoreRec {
  int ftype, deficit, cpf, nblks, fsize, amode, sfreq, rate, fixedbit, dynf, timef, auxf, hdcd, ext_id, ext, aspf, lff,
      hflag, hcrc, filts, vernum, chist, pcmr, sumf, sums, dialnorm;
  int subfs, pchs, subs[NCH], vqsub[NCH], joinx[NCH], thuff[NCH], shuff[NCH], bhuff[NCH], sel[NCH][10], adj[NCH][10],
      ahcrc;
  int nsubfs, npchs, nsubs[NCH], nvqsub[NCH];
  CoreSub sub[NSF];
  std::vector<U8> tail;
};

enum { XCH = 16, XCS = 16, XORD = 16, XSEG = 512, XFRAME = 65536 };

struct XllBandRec {  // a channel set's coding of a frequency band
  int decor, order[XCH], cpflag[XCH / 2], coeff[XCH / 2], aorder[XCH], forder[XCH], rc[XCH][XORD], dmixext, split;
  S64 lsbsize;
  int nlsb[XCH], scalres, adj[XCH];
  int highest;
  S64 lsb_bytes;  // what the LSB part of a segment takes, its CRC included
};

struct XllChsetRec {  // a channel set sub-header
  int hsc, nchc, resenc, bitres, width, fidx, interp, repl, active, primary, dce, demb, dtype, hier;
  std::vector<int> dcoef;
  int cmen;
  S64 chmask;
  std::vector<int> rad, theta, phi;  // the positions of the channels, or of a mapping's speakers
  int mapp, mapbits, ncfgc;
  std::vector<int> cfg_am, cfg_nsc, cfg_sm, cfg_mapc;
  std::vector<S64> cfg_mask;
  int xtra;
  XllBandRec band[2];
  std::vector<U8> hres;
  S64 crcok, crc;
  int nch, nbands, nabits, hier_ofs, freq;
  // the coding parameters as they go from segment to segment
  int seg_common, rice[XCH], aux[XCH], abit_a[XCH], abit_b[XCH], na[XCH];
};

struct XllSegRec {  // a segment of a channel set's band data: what isn't samples
  int reuse, common, rice[XCH], hyb[XCH], auxc[XCH], acode[XCH], bcode[XCH], dhbits;
  int niso[XCH];
  std::vector<int> iso[XCH];  // per channel, where hybrid Rice codes put their isolated samples
  S32 dh[XCH][8];             // band 1, segment 0: the decimator history, [1-7] of each channel
  std::vector<U8> g1, g2;     // the bits after the MSB part, and after the LSB part
};

struct XllRec {
  int ver, hsc, fsnbc;
  S64 fsc;
  int ncsc, segl, ssl, ssnbc, bandcrc, scalable, cmnbc, fixedlsb;
  std::vector<U8> hres;
  S64 crcok, crc;
  XllChsetRec cs[XCS];
  std::vector<S64> navi;  // the band data sizes less 1: [band][segment][channel set], -1 for none
  std::vector<U8> nalign;
  S64 ncrcok, ncrc;
  std::vector<XllSegRec> seg;  // [band][segment][channel set]
  std::vector<S32> res, lsb;   // [band][channel][sample], the channel sets' channels one after the other
  std::vector<U8> tail, rest;
  int hsize, nchsets, nsegs, nseg_log2, nsegsamples, nframesamples, seg_nbits, chmask_nbits, nbands, nch;
  S64 frame_size;
  int chofs[XCS + 1];
};

struct ExssAsset {
  int dsize, index;
  int typep, type, langp, lang, infop, infosize;
  std::vector<U8> info;
  int bitres, maxrate, totch, one2one, es, e6, smen, smbits, nremap, layout[8], ndec[8], reptype;
  S64 smask;
  std::vector<int> remap_mask, remap_code;
  int known;  // its static fields seen (in this header or before)
  int drcp, drc, dnp, dn, drc2, mixp, extmix, postmix, ctrl, limdrc, customdrc, perch;
  std::vector<int> mainscale, mixmask, mixcoef;
  int mode, mask, core_size, core_sync, core_dist, xbr_size, xxch_size, x96_size, lbr_size, lbr_sync, lbr_dist;
  S64 xll_size;
  int xll_sync, xll_peak, xll_dlybits;
  S64 xll_dly, xll_syncofs;
  int rsv1, rsv2, aux_size, aux_codec, aux_sync, aux_dist, hdid;
  std::vector<U8> dres;
  int xll_parsed;  // its XLL component as an XLL frame (else as bytes)
  std::vector<U8> comp[W_N];
  XllRec xll;
};

struct ExssRec {
  int user, idx, wide, hsc, fsc, stat, refclk, dur, tsf, tslsb, npresc, nassetsc;
  S64 ts;
  int exmask[8], amask[8][4], mix, mixadj, mixbits, nmixcfgc;
  S64 mixchmask[4], asize[8];
  int bc[8], bcss[8], bcasset[8];
  std::vector<U8> hres;
  S64 crcok, crc;
  ExssAsset a[8];
  std::vector<U8> rest;
  int npres, nassets, hsize, fsize;
};

struct Frame {
  int kind;  // 0 core, 1 extension substream
  CoreRec core;
  ExssRec exss;
};

// What the extension substream headers' static fields have said, for the headers after them (which
// need not have them)
struct Ctx {
  int mix, nmixcfg, nmixch[4];
  int known[8], one2one[8], es[8], e6[8], totch[8];
  void reset() { memset(this, 0, sizeof *this); }
};

// ===================================================================================================
// The walk of a frame through its IO
// ===================================================================================================

static const int XLL_FREQ[16] = {8000,   16000,  32000, 64000, 128000, 22050, 44100,  88200,
                                 176400, 352800, 12000, 24000, 48000,  96000, 192000, 384000};
static const int DMIX_NCH[8] = {1, 2, 2, 3, 3, 4, 4, 0};  // the channels of an XLL downmix type

// the samples of a subsubframe of a subband: how they are coded (0 none, 1 Huffman, 2 plain, 3
// block codes)
static inline int sample_coding(const CoreRec& c, int ch, int a) {
  if (!a) return 0;
  if (a <= 10) {
    if (c.sel[ch][a - 1] < QB_GROUP[a - 1]) return 1;
    if (a <= 7) return 3;
  }
  return 2;
}

// an element through the IO: its value from the record, back into the record (x evaluated once)
#define WU(f, n, x)                       \
  do {                                    \
    auto& r_ = (x);                       \
    S64 v_ = r_;                          \
    if (!io.u(F_##f, n, v_)) return false; \
    r_ = v_;                              \
  } while (0)
#define WS(f, n, x)                       \
  do {                                    \
    auto& r_ = (x);                       \
    S64 v_ = r_;                          \
    if (!io.s(F_##f, n, v_)) return false; \
    r_ = v_;                              \
  } while (0)
#define WZ(f, n, x)                       \
  do {                                    \
    auto& r_ = (x);                       \
    S64 v_ = r_;                          \
    if (!io.zz(F_##f, n, v_)) return false; \
    r_ = v_;                              \
  } while (0)
#define WR(f, k, x)                       \
  do {                                    \
    auto& r_ = (x);                       \
    S64 v_ = r_;                          \
    if (!io.rice(F_##f, k, v_)) return false; \
    r_ = v_;                              \
  } while (0)
#define WH(f, b, x)                       \
  do {                                    \
    auto& r_ = (x);                       \
    S64 v_ = r_;                          \
    if (!io.huff(F_##f, b, v_)) return false; \
    r_ = v_;                              \
  } while (0)
#define WRAW(f, n, b) \
  if (!io.raw(F_##f, n, b)) return false
#define WCRC(fo, fw, from, ok, w) \
  if (!io.crc(F_##fo, F_##fw, from, ok, w)) return false

// A walk of one frame through its IO: reads its elements from the bits into the record (BitIn), or
// writes them (BitOut), or at record level reads or writes the record's values as they are (TsvIn,
// TsvOut): one walk, so that the directions can't disagree. cx: what the extension substream
// headers before have said (the walk updates it).
template <class IO>
struct Walk {
  IO& io;
  Ctx& cx;
  int xll_raw;  // (BitIn) XLL components as bytes
  const char* err;
  int dummy;
  S64 dummy64;
  Walk(IO& i, Ctx& c, int xr = 0) : io(i), cx(c), xll_raw(xr), err(0) {}
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  // element k of a list of the record (made where the IO reads)
  int& at(std::vector<int>& v, size_t k) {
    if (k >= v.size()) {
      if (!IO::READ) return fail("internal error: a list too short"), dummy;
      v.resize(k + 1);
    }
    return v[k];
  }
  S64& at(std::vector<S64>& v, size_t k) {
    if (k >= v.size()) {
      if (!IO::READ) return fail("internal error: a list too short"), dummy64;
      v.resize(k + 1);
    }
    return v[k];
  }
  bool frame(Frame& f) { return (f.kind ? exss(f.exss) : core(f.core)) && !err; }

  // --- the core frame (TS 102 114 clause 5) ----------------------------------------------------------
  bool core(CoreRec& c) {
    const U64 p0 = io.pos;
    io.open(L_C, 0);
    if (!io.fixed(SYNC_CORE, 32, "no core sync word")) return false;
    WU(FTYPE, 1, c.ftype);
    WU(SHORT, 5, c.deficit);
    WU(CPF, 1, c.cpf);
    WU(NBLKS, 7, c.nblks);
    WU(FSIZE, 14, c.fsize);
    WU(AMODE, 6, c.amode);
    WU(SFREQ, 4, c.sfreq);
    WU(RATE, 5, c.rate);
    WU(FixedBit, 1, c.fixedbit);
    WU(DYNF, 1, c.dynf);
    WU(TIMEF, 1, c.timef);
    WU(AUXF, 1, c.auxf);
    WU(HDCD, 1, c.hdcd);
    WU(EXT_AUDIO_ID, 3, c.ext_id);
    WU(EXT_AUDIO, 1, c.ext);
    WU(ASPF, 1, c.aspf);
    WU(LFF, 2, c.lff);
    WU(HFLAG, 1, c.hflag);
    if (c.cpf) WU(HCRC, 16, c.hcrc);
    WU(FILTS, 1, c.filts);
    WU(VERNUM, 4, c.vernum);
    WU(CHIST, 2, c.chist);
    WU(PCMR, 3, c.pcmr);
    WU(SUMF, 1, c.sumf);
    WU(SUMS, 1, c.sums);
    WU(DIALNORM, 4, c.dialnorm);
    if (c.fsize < 95) return fail("a core frame size out of range");
    if (c.lff == 3) return fail("an LFE flag out of range");
    const U64 end = p0 + (U64)(c.fsize + 1) * 8;
    // the primary audio coding header
    io.open(L_A, 0);
    WU(SUBFS, 4, c.subfs);
    c.nsubfs = c.subfs + 1;
    WU(PCHS, 3, c.pchs);
    c.npchs = c.pchs + 1;
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(SUBS, 5, c.subs[ch]);
      if ((c.nsubs[ch] = c.subs[ch] + 2) > NSUB) return fail("a subband count out of range");
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(VQSUB, 5, c.vqsub[ch]);
      c.nvqsub[ch] = c.vqsub[ch] + 1;
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(JOINX, 3, c.joinx[ch]);
      if (c.joinx[ch] > c.npchs) return fail("a joint intensity channel out of range");
    }
    for (int ch = 0; ch < c.npchs; ch++) WU(THUFF, 2, c.thuff[ch]);
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(SHUFF, 3, c.shuff[ch]);
      if (c.shuff[ch] == 7) return fail("a scale factor code book out of range");
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(BHUFF, 3, c.bhuff[ch]);
      if (c.bhuff[ch] == 7) return fail("a bit allocation code book out of range");
    }
    for (int a = 0; a < 10; a++)
      for (int ch = 0; ch < c.npchs; ch++) WU(SEL, SEL_BITS[a], c.sel[ch][a]);
    for (int a = 0; a < 10; a++)
      for (int ch = 0; ch < c.npchs; ch++) {
        if (c.sel[ch][a] < QB_GROUP[a])
          WU(ADJ, 2, c.adj[ch][a]);
        else
          c.adj[ch][a] = 0;
      }
    if (c.cpf) WU(AHCRC, 16, c.ahcrc);
    int nblk = 0;  // subband samples so far
    for (int sf = 0; sf < c.nsubfs; sf++)
      if (!subframe(c, sf, nblk)) return false;
    if (io.pos > end) return fail("subframes past the end of the frame");
    io.open(L_T, 0);
    WRAW(tail, end - io.pos, c.tail);
    return true;
  }

  bool subframe(CoreRec& c, int sf, int& nblk) {
    CoreSub& s = c.sub[sf];
    io.open(L_S, 0, sf);
    WU(SSC, 2, s.ssc);
    s.nssc = s.ssc + 1;
    WU(PSC, 3, s.psc);
    if (s.psc) return fail("a partial subsubframe");
    nblk += s.nssc * 8;
    if (nblk > c.nblks + 1) return fail("more subband samples than blocks");
    for (int ch = 0; ch < c.npchs; ch++) io.open(L_B, ch, ch);
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      for (int b = 0; b < c.nsubs[ch]; b++) WU(PMODE, 1, s.pmode[ch][b]);
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      for (int b = 0; b < c.nsubs[ch]; b++)
        if (s.pmode[ch][b]) WU(PVQ, 12, s.pvq[ch][b]);
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      for (int b = 0; b < c.nvqsub[ch]; b++) {
        if (c.bhuff[ch] < 5)
          WH(ABITS, BA[c.bhuff[ch]], s.abits[ch][b]);
        else
          WU(ABITS, c.bhuff[ch] - 1, s.abits[ch][b]);
        if (s.abits[ch][b] < 0 || s.abits[ch][b] > 26) return fail("a bit allocation out of range");
      }
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      for (int b = 0; b < c.nvqsub[ch]; b++) {
        if (s.nssc > 1 && s.abits[ch][b])
          WH(TMODE, TB[c.thuff[ch]], s.tmode[ch][b]);
        else
          s.tmode[ch][b] = 0;
      }
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      const int sh = c.shuff[ch], max = sh == 6 ? 128 : 64;
      int si = 0;
      for (int b = 0; b < c.nsubs[ch] || b < c.nvqsub[ch]; b++) {
        const int k = b < c.nvqsub[ch] ? (s.abits[ch][b] ? 1 + (s.tmode[ch][b] > 0) : 0) : 1;
        for (int j = 0; j < k; j++) {
          S64 v = s.scale[ch][b][j];
          if (sh >= 5) {
            if (!io.u(F_SCALES, sh + 1, v)) return false;
          } else if (IO::BITS) {  // (the bits have the difference to the scale factor before)
            S64 d = v - si;
            if (!io.huff(F_SCALES, SB[sh], d)) return false;
            v = si + d;
          } else {
            if (!io.val(F_SCALES, 0, max - 1, v)) return false;
            io.pos += SB[sh].bits(v - si);
          }
          if (v < 0 || v >= max) return fail("a scale factor out of range");
          s.scale[ch][b][j] = (int)v;
          si = (int)v;
        }
      }
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      if (c.joinx[ch]) {
        WU(JOIN_SHUFF, 3, s.jsel[ch]);
        if (s.jsel[ch] == 7) return fail("a joint scale code book out of range");
      }
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      if (!c.joinx[ch]) continue;
      const int src = c.joinx[ch] - 1;
      for (int b = c.nsubs[ch]; b < c.nsubs[src]; b++) {
        if (s.jsel[ch] < 5)
          WH(JOIN_SCALES, SB[s.jsel[ch]], s.jscale[ch][b]);
        else
          WU(JOIN_SCALES, s.jsel[ch] + 1, s.jscale[ch][b]);
      }
    }
    if (c.dynf) WU(RANGE, 8, s.range);
    if (c.cpf) WU(SICRC, 16, s.sicrc);
    for (int ch = 0; ch < c.npchs; ch++) {
      io.sel(L_B, ch);
      for (int b = c.nvqsub[ch]; b < c.nsubs[ch]; b++) WU(HFVQ, 10, s.hfvq[ch][b]);
    }
    const int nlfe = c.lff ? 2 * c.lff * s.nssc : 0;
    for (int i = 0; i < nlfe; i++) WS(LFE, 8, s.lfe[i]);
    if (c.lff) WU(LFEscaleIndex, 8, s.lfescale);
    // the subband samples
    for (int ch = 0; ch < c.npchs; ch++) io.open(L_Q, ch, ch);
    for (int ssf = 0; ssf < s.nssc; ssf++) {
      for (int ch = 0; ch < c.npchs; ch++) {
        io.sel(L_Q, ch);
        for (int b = 0; b < c.nvqsub[ch]; b++) {
          const int a = s.abits[ch][b], how = sample_coding(c, ch, a);
          S32* v = &s.q[ch][b][ssf * 8];
          if (how == 1) {
            const HuffBook& hb = QB[a - 1][c.sel[ch][a - 1]];
            for (int m = 0; m < 8; m++) WH(q, hb, v[m]);
          } else if (how == 3) {
            if (!block(v, a)) return false;
          } else if (how == 2) {
            for (int m = 0; m < 8; m++) WS(q, a - 3, v[m]);
          }
        }
      }
      if ((ssf == s.nssc - 1 || c.aspf) && !io.fixed(0xFFFF, 16, "no DSYNC where it should be")) return false;
    }
    return true;
  }

  // 8 samples of ABITS a in two block codes (each 4 samples as the digits of a number of base the
  // levels)
  bool block(S32* v, int a) {
    const int L = BLOCK_LEVELS[a - 1], off = (L - 1) / 2, nb = BLOCK_BITS[a - 1];
    for (int h = 0; h < 2; h++, v += 4) {
      if (IO::BITS) {
        S64 code = 0;
        if (!IO::READ)
          for (int i = 3; i >= 0; i--) {
            if (v[i] < -off || v[i] > off) return fail("a value out of range");
            code = code * L + v[i] + off;
          }
        if (!io.u(F_q, nb, code)) return false;
        if (IO::READ) {
          for (int i = 0; i < 4; i++) {
            v[i] = (S32)(code % L) - off;
            code /= L;
          }
          if (code) return fail("a block code out of range");
        }
      } else {
        for (int i = 0; i < 4; i++) {
          S64 x = v[i];
          if (!io.val(F_q, -off, off, x)) return false;
          v[i] = (S32)x;
        }
        io.pos += nb;
      }
    }
    return true;
  }

  // --- the extension substream (clause 7) ------------------------------------------------------------
  bool exss(ExssRec& x) {
    const U64 p0 = io.pos;
    io.open(L_X, 0);
    if (!io.fixed(SYNC_EXSS, 32, "no extension substream sync word")) return false;
    WU(UserDefinedBits, 8, x.user);
    WU(nExtSSIndex, 2, x.idx);
    WU(bHeaderSizeType, 1, x.wide);
    const int nbf = x.wide ? 20 : 16;
    WU(nuExtSSHeaderSize, x.wide ? 12 : 8, x.hsc);
    WU(nuExtSSFsize, nbf, x.fsc);
    x.hsize = x.hsc + 1;
    x.fsize = x.fsc + 1;
    if (x.hsize < 8 || x.hsize > x.fsize) return fail("an extension substream header size out of range");
    WU(bStaticFieldsPresent, 1, x.stat);
    x.npres = x.nassets = 1;
    if (x.stat) {
      WU(nuRefClockCode, 2, x.refclk);
      WU(nuExSSFrameDurationCode, 3, x.dur);
      WU(bTimeStampFlag, 1, x.tsf);
      if (x.tsf) {
        WU(nuTimeStamp, 32, x.ts);
        WU(nLSB, 4, x.tslsb);
      }
      WU(nuNumAudioPresnt, 3, x.npresc);
      WU(nuNumAssets, 3, x.nassetsc);
      x.npres = x.npresc + 1;
      x.nassets = x.nassetsc + 1;
      for (int i = 0; i < x.npres; i++) WU(nuActiveExSSMask, x.idx + 1, x.exmask[i]);
      for (int i = 0; i < x.npres; i++)
        for (int ss = 0; ss <= x.idx; ss++)
          if (x.exmask[i] >> ss & 1) WU(nuActiveAssetMask, 8, x.amask[i][ss]);
      WU(bMixMetadataEnbl, 1, x.mix);
      cx.mix = x.mix;
      if (x.mix) {
        WU(nuMixMetadataAdjLevel, 2, x.mixadj);
        WU(nuBits4MixOutMask, 2, x.mixbits);
        WU(nuNumMixOutConfigs, 2, x.nmixcfgc);
        cx.nmixcfg = x.nmixcfgc + 1;
        for (int i = 0; i < cx.nmixcfg; i++) {
          WU(nuMixOutChMask, (x.mixbits + 1) << 2, x.mixchmask[i]);
          cx.nmixch[i] = numspk(x.mixchmask[i]);
        }
      }
    }
    U64 total = x.hsize;
    for (int a = 0; a < x.nassets; a++) {
      WU(nuAssetFsize, nbf, x.asize[a]);
      total += x.asize[a] + 1;
    }
    if (total > (U64)x.fsize) return fail("extension substream assets past the frame");
    for (int a = 0; a < x.nassets; a++)
      if (!descriptor(x, a)) return false;
    for (int i = 0; i < x.npres; i++) WU(bBcCorePresent, 1, x.bc[i]);
    for (int i = 0; i < x.npres; i++)
      if (x.bc[i]) {
        WU(nuBcCoreExtSSIndex, 2, x.bcss[i]);
        WU(nuBcCoreAssetIndex, 3, x.bcasset[i]);
      }
    const U64 hc = p0 + (U64)(x.hsize - 2) * 8;
    if (io.pos > hc) return fail("an extension substream header too short");
    WRAW(X_Reserved, hc - io.pos, x.hres);
    WCRC(X_crcok, X_crc, p0 / 8 + 5, x.crcok, x.crc);
    U64 at = p0 + (U64)x.hsize * 8;
    for (int a = 0; a < x.nassets; a++) {
      const U64 e = at + (U64)(x.asize[a] + 1) * 8;
      if (!asset(x, a, e)) return false;
      at = e;
    }
    WRAW(X_rest, p0 + (U64)x.fsize * 8 - at, x.rest);
    return true;
  }

  // an audio asset descriptor (7.5)
  bool descriptor(ExssRec& x, int a) {
    ExssAsset& s = x.a[a];
    io.open(L_D, 0, a);
    const U64 dpos = io.pos;
    WU(nuAssetDescriptFsize, 9, s.dsize);
    const U64 dend = dpos + (U64)(s.dsize + 1) * 8;
    WU(nuAssetIndex, 3, s.index);
    const int ai = s.index;
    if (x.stat) {
      WU(bAssetTypeDescrPresent, 1, s.typep);
      if (s.typep) WU(nuAssetTypeDescriptor, 4, s.type);
      WU(bLanguageDescrPresent, 1, s.langp);
      if (s.langp) WU(LanguageDescriptor, 24, s.lang);
      WU(bInfoTextPresent, 1, s.infop);
      if (s.infop) {
        WU(nuInfoTextByteSize, 10, s.infosize);
        WRAW(InfoTextString, (U64)(s.infosize + 1) * 8, s.info);
      }
      WU(nuBitResolution, 5, s.bitres);
      WU(nuMaxSampleRate, 4, s.maxrate);
      WU(nuTotalNumChs, 8, s.totch);
      WU(bOne2OneMapChannels2Speakers, 1, s.one2one);
      if (s.one2one) {
        if (s.totch + 1 > 2)
          WU(bEmbeddedStereoFlag, 1, s.es);
        else
          s.es = 0;
        if (s.totch + 1 > 6)
          WU(bEmbeddedSixChFlag, 1, s.e6);
        else
          s.e6 = 0;
        WU(D_bSpkrMaskEnabled, 1, s.smen);
        int mb = 0;
        if (s.smen) {
          WU(nuNumBits4SAMask, 2, s.smbits);
          mb = (s.smbits + 1) << 2;
          WU(nuSpkrActivityMask, mb, s.smask);
        }
        WU(nuNumSpkrRemapSets, 3, s.nremap);
        if (s.nremap && !mb) return fail("speaker remapping without a speaker mask");
        for (int i = 0; i < s.nremap; i++) WU(nuStndrSpkrLayoutMask, mb, s.layout[i]);
        if (IO::READ) s.remap_mask.clear(), s.remap_code.clear();
        size_t km = 0, kc = 0;
        for (int i = 0; i < s.nremap; i++) {
          const int nspk = numspk(s.layout[i]);
          WU(nuNumDecCh4Remap, 5, s.ndec[i]);
          for (int c = 0; c < nspk; c++) {
            const size_t m = km++;
            WU(nuRemapDecChMask, s.ndec[i] + 1, at(s.remap_mask, m));
            for (int k = popcount(at(s.remap_mask, m)); k > 0; k--) {
              const size_t j = kc++;
              WU(nuSpkrRemapCodes, 5, at(s.remap_code, j));
            }
          }
        }
      } else {
        s.es = s.e6 = 0;
        WU(nuRepresentationType, 3, s.reptype);
      }
      cx.known[ai] = 1;
      cx.one2one[ai] = s.one2one;
      cx.es[ai] = s.es;
      cx.e6[ai] = s.e6;
      cx.totch[ai] = s.totch + 1;
    }
    s.known = cx.known[ai];
    if (s.known) {
      // dynamic metadata: DRC, DNC and mixing metadata
      WU(bDRCCoefPresent, 1, s.drcp);
      if (s.drcp) WU(nuDRCCode, 8, s.drc);
      WU(bDialNormPresent, 1, s.dnp);
      if (s.dnp) WU(nuDialNormCode, 5, s.dn);
      if (s.drcp && cx.es[ai]) WU(nuDRC2ChDmixCode, 8, s.drc2);
      if (cx.mix)
        WU(bMixMetadataPresent, 1, s.mixp);
      else
        s.mixp = 0;
      if (s.mixp) {
        WU(bExternalMixFlag, 1, s.extmix);
        WU(nuPostMixGainAdjCode, 6, s.postmix);
        WU(nuControlMixerDRC, 2, s.ctrl);
        if (s.ctrl < 3)
          WU(nuLimit4EmbeddedDRC, 3, s.limdrc);
        else
          WU(nuCustomDRCCode, 8, s.customdrc);
        WU(bEnblPerChMainAudioScale, 1, s.perch);
        if (IO::READ) s.mainscale.clear(), s.mixmask.clear(), s.mixcoef.clear();
        size_t k = 0, km = 0, kc = 0;
        for (int ns = 0; ns < cx.nmixcfg; ns++)
          for (int c = 0; c < (s.perch ? cx.nmixch[ns] : 1); c++) {
            const size_t j = k++;
            WU(nuMainAudioScaleCode, 6, at(s.mainscale, j));
          }
        int nd[3], ne = 0;
        nd[ne++] = cx.totch[ai];
        if (cx.e6[ai]) nd[ne++] = 6;
        if (cx.es[ai]) nd[ne++] = 2;
        for (int ns = 0; ns < cx.nmixcfg; ns++)
          for (int e = 0; e < ne; e++)
            for (int c = 0; c < nd[e]; c++) {
              const size_t m = km++;
              WU(nuMixMapMask, cx.nmixch[ns], at(s.mixmask, m));
              for (int j = popcount(at(s.mixmask, m)); j > 0; j--) {
                const size_t i = kc++;
                WU(nuMixCoeffs, 6, at(s.mixcoef, i));
              }
            }
      }
      // decoder navigation data
      const int nbf = x.wide ? 20 : 16;
      WU(nuCodingMode, 2, s.mode);
      switch (s.mode) {
        case 0:
          WU(nuCoreExtensionMask, 12, s.mask);
          if (s.mask & 0x010) {
            WU(nuExSSCoreFsize, 14, s.core_size);
            WU(bExSSCoreSyncPresent, 1, s.core_sync);
            if (s.core_sync) WU(nuExSSCoreSyncDistInFrames, 2, s.core_dist);
          }
          if (s.mask & 0x020) WU(nuExSSXBRFsize, 14, s.xbr_size);
          if (s.mask & 0x040) WU(nuExSSXXCHFsize, 14, s.xxch_size);
          if (s.mask & 0x080) WU(nuExSSX96Fsize, 12, s.x96_size);
          if (s.mask & 0x100 && !lbr(s)) return false;
          if (s.mask & 0x200 && !xll_params(s, nbf)) return false;
          if (s.mask & 0x400) WU(Reserved1, 16, s.rsv1);
          if (s.mask & 0x800) WU(Reserved2, 16, s.rsv2);
          break;
        case 1:
          s.mask = 0x200;
          if (!xll_params(s, nbf)) return false;
          break;
        case 2:
          s.mask = 0x100;
          if (!lbr(s)) return false;
          break;
        default:
          s.mask = 0;
          WU(nuExSSAuxFsize, 14, s.aux_size);
          WU(nuAuxCodecID, 8, s.aux_codec);
          WU(bExSSAuxSyncPresent, 1, s.aux_sync);
          if (s.aux_sync) WU(nuExSSAuxSyncDistInFrames, 3, s.aux_dist);
          break;
      }
      if (s.mask & 0x200) WU(nuDTSHDStreamID, 3, s.hdid);
    }
    // (what follows the stream ID: the mixing flags, the DRC of revision 2, reserved bits, as they are)
    if (io.pos > dend) return fail("an asset descriptor past its size");
    WRAW(D_Reserved, dend - io.pos, s.dres);
    return true;
  }
  bool lbr(ExssAsset& s) {
    WU(nuExSSLBRFsize, 14, s.lbr_size);
    WU(bExSSLBRSyncPresent, 1, s.lbr_sync);
    if (s.lbr_sync) WU(nuExSSLBRSyncDistInFrames, 2, s.lbr_dist);
    return true;
  }
  bool xll_params(ExssAsset& s, int nbf) {
    WU(nuExSSXLLFsize, nbf, s.xll_size);
    WU(bExSSXLLSyncPresent, 1, s.xll_sync);
    if (s.xll_sync) {
      WU(nuPeakBRCntrlBuffSzkB, 4, s.xll_peak);
      WU(nuBitsInitDecDly, 5, s.xll_dlybits);
      WU(nuInitLLDecDlyFrames, s.xll_dlybits + 1, s.xll_dly);
      WU(nuExSSXLLSyncOffset, nbf, s.xll_syncofs);
    }
    return true;
  }

  // an asset's data, to bit end: its components in the order of its descriptor (the XLL frame of an
  // XLL component that has one at its start, the others as bytes), then what is left as bytes
  bool asset(ExssRec& x, int a, U64 end) {
    ExssAsset& s = x.a[a];
    if (!s.known) return bytes(a, W_ALL, end - io.pos, s.comp[W_ALL]);
    static const int what[6] = {W_CORE, W_XBR, W_XXCH, W_X96, W_LBR, W_XLL};
    const S64 size[6] = {s.core_size + 1LL, s.xbr_size + 1LL, s.xxch_size + 1LL, s.x96_size + 1LL, s.lbr_size + 1LL, s.xll_size + 1};
    for (int i = 0; i < 6; i++) {
      if (!(s.mask & (0x010 << i))) continue;
      const U64 ce = io.pos + (U64)size[i] * 8;
      if (ce > end) return fail("an asset's components past its size");
      if (i == 5) {
        if (IO::BITS && IO::READ) s.xll_parsed = !xll_raw && size[i] >= 16 && io.peek32() == SYNC_XLL;
        if (!IO::BITS && IO::READ) s.xll_parsed = io.peek_line() == 'L';
        if (s.xll_parsed) {
          if (!xll(s.xll, a, ce, cx.one2one[s.index])) return false;
          continue;
        }
      }
      if (!bytes(a, what[i], ce - io.pos, s.comp[what[i]])) return false;
    }
    if (s.mode == 3) {
      const U64 ce = io.pos + (U64)(s.aux_size + 1) * 8;
      if (ce > end) return fail("an asset's components past its size");
      if (!bytes(a, W_AUX, ce - io.pos, s.comp[W_AUX])) return false;
    }
    if (io.pos < end && !bytes(a, W_REST, end - io.pos, s.comp[W_REST])) return false;
    return true;
  }
  bool bytes(int a, int what, U64 n, std::vector<U8>& b) {
    io.open(L_U, 0, a, what);
    WRAW(hex, n, b);
    return true;
  }

  // --- XLL (clause 8): an XLL frame from the bit it starts at to cend, the end of its component ------
  bool xll(XllRec& x, int a, U64 cend, int one2one) {
    const U64 p0 = io.pos;
    io.open(L_L, 0, a);
    if (!io.fixed(SYNC_XLL, 32, "no XLL sync word")) return false;
    WU(nVersion, 4, x.ver);
    if (x.ver) return fail("an XLL version above 1");
    WU(nHeaderSize, 8, x.hsc);
    x.hsize = x.hsc + 1;
    WU(nBits4FrameFsize, 5, x.fsnbc);
    WU(nLLFrameSize, x.fsnbc + 1, x.fsc);
    x.frame_size = x.fsc + 1;
    if (x.hsize < 8 || x.frame_size < x.hsize || p0 + (U64)x.frame_size * 8 > cend)
      return fail("an XLL frame larger than its component");
    const U64 xend = p0 + (U64)x.frame_size * 8;
    WU(nNumChSetsInFrame, 4, x.ncsc);
    x.nchsets = x.ncsc + 1;
    WU(nSegmentsInFrame, 4, x.segl);
    WU(nSmplInSeg, 4, x.ssl);
    x.nsegs = 1 << x.segl;
    x.nseg_log2 = x.ssl;
    x.nsegsamples = 1 << x.ssl;
    if (x.nsegs > 1024 || !x.ssl || x.nsegsamples > XSEG || (x.nsegsamples << x.segl) > XFRAME)
      return fail("XLL segments out of range");
    x.nframesamples = x.nsegsamples << x.segl;
    WU(nBits4SSize, 5, x.ssnbc);
    x.seg_nbits = x.ssnbc + 1;
    WU(nBandDataCRCEn, 2, x.bandcrc);
    WU(bScalableLSBs, 1, x.scalable);
    WU(nBits4ChMask, 5, x.cmnbc);
    x.chmask_nbits = x.cmnbc + 1;
    if (x.scalable)
      WU(nuFixedLSBWidth, 4, x.fixedlsb);
    else
      x.fixedlsb = 0;
    const U64 hc = p0 + (U64)(x.hsize - 2) * 8;
    if (io.pos > hc) return fail("an XLL header too short");
    WRAW(L_Reserved, hc - io.pos, x.hres);
    WCRC(L_crcok, L_crc, p0 / 8 + 4, x.crcok, x.crc);
    // the channel set sub-headers
    int hier = 0;
    x.nch = 0;
    x.nbands = 1;
    for (int c = 0; c < x.nchsets; c++)
      if (!chset(x, c, one2one, hier, xend)) return false;
    x.chofs[x.nchsets] = x.nch;
    if ((S64)x.nbands * x.nch * x.nframesamples > 1 << 24) return fail("an XLL frame too large");
    // the NAVI table
    io.open(L_N, 0);
    const U64 n0 = io.pos;
    const size_t nck = (size_t)x.nbands * x.nsegs * x.nchsets;
    if (IO::READ)
      x.navi.assign(nck, -1);
    else if (x.navi.size() != nck)
      return fail("internal error: a NAVI table of another size");
    for (int b = 0; b < x.nbands; b++)
      for (int seg = 0; seg < x.nsegs; seg++)
        for (int c = 0; c < x.nchsets; c++)
          if (x.cs[c].nbands > b) WU(BandChSetSize, x.seg_nbits, x.navi[((size_t)b * x.nsegs + seg) * x.nchsets + c]);
    WRAW(N_align, (8 - (io.pos & 7)) & 7, x.nalign);
    WCRC(N_crcok, N_crc, n0 / 8, x.ncrcok, x.ncrc);
    // the band data
    const size_t nval = (size_t)x.nbands * x.nch * x.nframesamples;
    if (IO::READ) {
      if (x.seg.size() < nck) x.seg.resize(nck);
      if (x.res.size() < nval) x.res.resize(nval), x.lsb.resize(nval);
    } else if (x.seg.size() < nck || x.res.size() < nval) {
      return fail("internal error: XLL data of another size");
    }
    U64 at = io.pos;
    for (int b = 0; b < x.nbands; b++)
      for (int seg = 0; seg < x.nsegs; seg++)
        for (int c = 0; c < x.nchsets; c++) {
          XllChsetRec& s = x.cs[c];
          if (s.nbands <= b) continue;
          const size_t i = ((size_t)b * x.nsegs + seg) * x.nchsets + c;
          const U64 ce = at + (U64)(x.navi[i] + 1) * 8;
          if (ce > xend) return fail("band data past the XLL frame");
          if (!segment(x, s, s.band[b], x.seg[i], b, seg, c, at, ce)) return false;
          at = ce;
        }
    WRAW(L_tail, xend - at, x.tail);
    WRAW(L_rest, cend - xend, x.rest);
    return true;
  }

  // a channel set sub-header (8.3.2)
  bool chset(XllRec& x, int c, int one2one, int& hier, U64 xend) {
    XllChsetRec& s = x.cs[c];
    const U64 c0 = io.pos;
    io.open(L_K, 0, c);
    WU(nChSetHeaderSize, 10, s.hsc);
    const U64 cend = c0 + (U64)(s.hsc + 1) * 8;
    if (s.hsc + 1 < 3 || cend > xend) return fail("a channel set header past the XLL frame");
    WU(nChSetLLChannel, 4, s.nchc);
    s.nch = s.nchc + 1;
    WU(nResidualChEncode, s.nch, s.resenc);
    WU(nBitResolution, 5, s.bitres);
    WU(nBitWidth, 5, s.width);
    WU(sFreqIndex, 4, s.fidx);
    WU(nFsInterpolate, 2, s.interp);
    WU(nReplacementSet, 2, s.repl);
    if (s.repl) WU(bActiveReplaceSet, 1, s.active);
    s.hier_ofs = hier;
    if (IO::READ) {
      s.dcoef.clear(), s.rad.clear(), s.theta.clear(), s.phi.clear();
      s.cfg_am.clear(), s.cfg_nsc.clear(), s.cfg_sm.clear(), s.cfg_mapc.clear(), s.cfg_mask.clear();
    }
    size_t kp = 0;  // the positions so far
    if (one2one) {
      WU(bPrimaryChSet, 1, s.primary);
      WU(bDownmixCoeffCodeEmbedded, 1, s.dce);
      if (s.dce)
        WU(bDownmixEmbedded, 1, s.demb);
      else
        s.demb = 0;
      if (s.dce && s.primary)
        WU(nLLDownmixType, 3, s.dtype);
      else
        s.dtype = 0;
      WU(bHierChSet, 1, s.hier);
      if (s.dce) {
        if (s.primary && !DMIX_NCH[s.dtype]) return fail("a downmix type out of range");
        const int m = s.primary ? DMIX_NCH[s.dtype] : s.hier_ofs, n = m * (s.nch + !s.primary);
        for (int k = 0; k < n; k++) WU(DownmixCoeffs, 9, at(s.dcoef, k));
      }
      WU(bChMaskEnabled, 1, s.cmen);
      if (s.cmen)
        WU(nChMask, x.chmask_nbits, s.chmask);
      else
        for (int ch = 0; ch < s.nch; ch++)
          if (!position(s, kp++)) return false;
    } else {
      s.primary = 1;
      s.hier = 1;
      s.dce = s.demb = s.dtype = 0;
      WU(bMappingCoeffsPresent, 1, s.mapp);
      if (s.mapp) {
        WU(nBitsCh2SpkrCoef, 3, s.mapbits);
        WU(nNumSpeakerConfigs, 2, s.ncfgc);
        const int cb = 6 + 2 * s.mapbits;
        size_t kc = 0;
        for (int k = 0; k <= s.ncfgc; k++) {
          WU(nActiveChannelMask, s.nch, at(s.cfg_am, k));
          WU(nNumSpeakers, 6, at(s.cfg_nsc, k));
          WU(K_bSpkrMaskEnabled, 1, at(s.cfg_sm, k));
          if (at(s.cfg_sm, k)) WU(nSpkrMask, x.chmask_nbits, at(s.cfg_mask, k));
          for (int t = 0; t <= at(s.cfg_nsc, k); t++) {
            if (!at(s.cfg_sm, k) && !position(s, kp++)) return false;
            for (int ch = 0; ch < s.nch; ch++)
              if (at(s.cfg_am, k) >> ch & 1) {
                const size_t j = kc++;
                WU(Ch2SpkrMapCoeff, cb, at(s.cfg_mapc, j));
              }
          }
        }
      }
    }
    s.freq = XLL_FREQ[s.fidx];
    if (s.freq > 96000) {
      WU(bXtraFreqBands, 1, s.xtra);  // (decoders take 0 for two bands, though the text of 8.3.2 has one)
      s.nbands = s.xtra ? 4 : 2;
    } else {
      s.xtra = 0;
      s.nbands = 1;
    }
    if (s.nbands > 2) return fail("XLL with four frequency bands");
    if (s.nbands > x.nbands) x.nbands = s.nbands;
    s.nabits = s.width + 1 > 16 ? 5 : s.width + 1 > 8 ? 4 : 3;
    if ((x.nchsets > 1 || s.nbands > 1) && s.nabits < 5) s.nabits++;
    int chbits = 0;
    while ((1 << chbits) < s.nch) chbits++;
    for (int b = 0; b < s.nbands; b++)
      if (!band(x, s, b, chbits)) return false;
    if (s.hier) hier += s.nch;
    x.chofs[c] = x.nch;
    x.nch += s.nch;
    if (io.pos > cend - 16) return fail("a channel set header too short");
    WRAW(K_Reserved, cend - 16 - io.pos, s.hres);
    WCRC(K_crcok, K_crc, c0 / 8, s.crcok, s.crc);
    return true;
  }
  bool position(XllChsetRec& s, size_t k) {
    WU(RadiusDelta, 9, at(s.rad, k));
    WU(Theta, 9, at(s.theta, k));
    WU(Phi, 7, at(s.phi, k));
    return true;
  }

  // a channel set's coding of frequency band b
  bool band(XllRec& x, XllChsetRec& s, int b, int chbits) {
    XllBandRec& d = s.band[b];
    io.open(L_G, 0, b);
    WU(bPWChDecorEnabled, 1, d.decor);
    if (d.decor && s.nch > 1) {
      for (int ch = 0; ch < s.nch; ch++) WU(nOrigChanOrder, chbits, d.order[ch]);
      for (int i = 0; i < s.nch / 2; i++) {
        WU(bChPFlag, 1, d.cpflag[i]);
        if (d.cpflag[i]) WU(nPWChPairsCoeffs, 7, d.coeff[i]);
      }
    }
    d.highest = 0;
    for (int ch = 0; ch < s.nch; ch++) {
      WU(nAdaptPredOrder, 4, d.aorder[ch]);
      if (d.aorder[ch] > d.highest) d.highest = d.aorder[ch];
    }
    if (d.highest > x.nsegsamples) return fail("a prediction order above the segment's samples");
    for (int ch = 0; ch < s.nch; ch++)
      if (!d.aorder[ch]) WU(nFixedPredOrder, 2, d.forder[ch]);
    for (int ch = 0; ch < s.nch; ch++)
      for (int j = 0; j < d.aorder[ch]; j++) WU(nLPCReflCoeffsQInd, 8, d.rc[ch][j]);
    if (s.demb && b) WU(bEmbDMixInExtBand, 1, d.dmixext);
    if (b)
      WU(bMSBLSBSplitEnInExtBands, 1, d.split);
    else
      d.split = x.scalable;
    d.lsb_bytes = 0;
    if (d.split) {
      WU(nLSBFsize, x.seg_nbits, d.lsbsize);
      d.lsb_bytes = d.lsbsize;  // (and the CRC after the LSB part)
      if (d.lsbsize && (x.bandcrc > 2 || (!b && x.bandcrc > 1))) d.lsb_bytes += 2;
      for (int ch = 0; ch < s.nch; ch++) {
        WU(nScalableLSBs, 4, d.nlsb[ch]);
        if (d.nlsb[ch] && !d.lsb_bytes) return fail("LSBs without an LSB part");
      }
    } else {
      for (int ch = 0; ch < s.nch; ch++) d.nlsb[ch] = 0;
    }
    if (b)
      WU(bFlagScalableResExtBand, 1, d.scalres);
    else
      d.scalres = x.scalable;
    for (int ch = 0; ch < s.nch; ch++) {
      if (d.scalres)
        WU(nBitWidthAdjPerCh, 4, d.adj[ch]);
      else
        d.adj[ch] = 0;
    }
    return true;
  }

  // a segment of channel set c's band data in band b, from bit at to ce (8.5.2)
  bool segment(XllRec& x, XllChsetRec& s, XllBandRec& d, XllSegRec& k, int b, int seg, int c, U64 at, U64 ce) {
    io.open(L_P, 0, b, seg, c);
    for (int ch = 0; ch < s.nch; ch++) io.open(L_Y, ch, ch);
    if (seg)
      WU(bUseSegStateCodeParm, 1, k.reuse);
    else
      k.reuse = 0;
    if (!k.reuse) {  // (else the coding parameters of the segment before)
      WU(ncSegType, 1, k.common);
      s.seg_common = k.common;
      const int np = k.common ? 1 : s.nch;
      for (int i = 0; i < np; i++) {
        WU(bRiceCodeFlag, 1, k.rice[i]);
        if (!k.common && k.rice[i])
          WU(bHybridRiceFlag, 1, k.hyb[i]);
        else
          k.hyb[i] = 0;
        if (k.hyb[i]) WU(ncAuxABIT, s.nabits, k.auxc[i]);
        s.rice[i] = k.rice[i];
        s.aux[i] = k.hyb[i] ? k.auxc[i] + 1 : 0;
      }
      for (int i = 0; i < np; i++) {
        if (!seg) {
          WU(ncABIT0, s.nabits, k.acode[i]);
          s.abit_a[i] = !s.rice[i] && k.acode[i] ? k.acode[i] + 1 : k.acode[i];
          s.na[i] = k.common ? d.highest : d.aorder[i];
        } else {
          s.abit_a[i] = 0;
          s.na[i] = 0;
        }
        WU(ncABIT, s.nabits, k.bcode[i]);
        s.abit_b[i] = !s.rice[i] && k.bcode[i] ? k.bcode[i] + 1 : k.bcode[i];
      }
    }
    // the residuals: part A (the first segment's first samples, as many as the prediction order),
    // part B; linear or Rice codes, hybrid Rice codes coding their isolated samples linearly
    const int S = x.nsegsamples;
    for (int ch = 0; ch < s.nch; ch++) {
      io.sel(L_Y, ch);
      const int i = s.seg_common ? 0 : ch, na = s.na[i], nb = S - na;
      S32* pa = &x.res[((size_t)b * x.nch + x.chofs[c] + ch) * x.nframesamples + (size_t)seg * S];
      S32* pb = pa + na;
      if (!s.rice[i]) {
        for (int j = 0; j < na; j++) WZ(res, s.abit_a[i], pa[j]);
        for (int j = 0; j < nb; j++) WZ(res, s.abit_b[i], pb[j]);
        continue;
      }
      for (int j = 0; j < na; j++) WR(res, s.abit_a[i], pa[j]);
      if (!s.aux[i]) {
        for (int j = 0; j < nb; j++) WR(res, s.abit_b[i], pb[j]);
        continue;
      }
      WU(nCountIsMax, x.nseg_log2, k.niso[ch]);
      std::vector<int>& iso = k.iso[ch];
      if (IO::READ)
        iso.resize(k.niso[ch]);
      else if ((int)iso.size() != k.niso[ch])
        return fail("internal error: isolated samples");
      U8 flag[XSEG];
      memset(flag, 0, nb);
      for (int j = 0; j < k.niso[ch]; j++) {
        WU(IsMaxLoc, x.nseg_log2, iso[j]);
        if (iso[j] >= nb) return fail("an isolated sample past its segment");
        flag[iso[j]] = 1;
      }
      for (int j = 0; j < nb; j++) {
        if (flag[j])
          WZ(res, s.aux[i], pb[j]);
        else
          WR(res, s.abit_b[i], pb[j]);
      }
    }
    if (!seg && b == 1) {  // the decimator history of the band
      WU(nNumBitsForHistSampl, 5, k.dhbits);
      for (int ch = 0; ch < s.nch; ch++) {
        io.sel(L_Y, ch);
        for (int j = 1; j < 8; j++) WS(DeciHistory, k.dhbits + 1, k.dh[ch][j]);
      }
    }
    // the LSB part, at the end of the segment
    if ((U64)d.lsb_bytes * 8 > ce - at) return fail("an LSB part larger than its segment");
    const U64 ls = ce - (U64)d.lsb_bytes * 8;
    if (io.pos > ls) return fail("an MSB part that runs into the LSB part");
    WRAW(msbrest, ls - io.pos, k.g1);
    if (d.lsb_bytes) {
      for (int ch = 0; ch < s.nch; ch++)
        if (d.nlsb[ch]) {
          io.sel(L_Y, ch);
          S32* pl = &x.lsb[((size_t)b * x.nch + x.chofs[c] + ch) * x.nframesamples + (size_t)seg * S];
          for (int j = 0; j < S; j++) WU(lsb, d.nlsb[ch], pl[j]);
        }
      if (io.pos > ce) return fail("an LSB part cut short");
      WRAW(lsbrest, ce - io.pos, k.g2);
    }
    return true;
  }
};

// ===================================================================================================
// The text
// ===================================================================================================

static const char hexd[] = "0123456789abcdef";
static inline int hexv(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }
static inline void put_int(std::string& s, S64 v) {
  char b[24];
  int n = 0;
  U64 u = v < 0 ? (U64)(-(v + 1)) + 1 : (U64)v;
  do {
    b[n++] = (char)('0' + u % 10);
    u /= 10;
  } while (u);
  if (v < 0) s += '-';
  while (n) s += b[--n];
}

// --- the text written: TsvOut, the walk's IO that writes the record's values as text ----------------
struct TsvOut {
  enum { BITS = 0, READ = 0 };
  U64 pos;
  const char* err;
  struct Line {
    int l, key[3];
    std::vector<std::string> col;
  };
  std::vector<Line> lines;  // (kept from frame to frame, for their strings' storage)
  int nl;
  int slot_of[L_NUM][NSLOT];  // the line of each slot
  int cur[L_NUM];
  void start() {
    nl = 0;
    pos = 0;
    err = 0;
    for (int l = 0; l < L_NUM; l++) cur[l] = -1;
  }
  void open(int l, int slot, int k0 = 0, int k1 = 0, int k2 = 0) {
    if (nl == (int)lines.size()) lines.push_back(Line());
    Line& L = lines[nl];
    L.l = l;
    L.key[0] = k0;
    L.key[1] = k1;
    L.key[2] = k2;
    L.col.resize(line_ncols[l]);
    for (size_t c = 0; c < L.col.size(); c++) L.col[c].clear();
    slot_of[l][slot] = nl++;
    cur[l] = slot;
  }
  void sel(int l, int slot) { cur[l] = slot; }
  std::string& col(int fid) {
    const int l = field_line[fid];
    if (cur[l] < 0) {
      fprintf(stderr, "dts2tsv: internal error: %s before its line\n", field_name[fid]);
      exit(EX_FAIL);
    }
    std::string& c = lines[slot_of[l][cur[l]]].col[field_col[fid]];
    if (!c.empty()) c += ',';
    return c;
  }
  bool num(int fid, S64 v) {
    put_int(col(fid), v);
    return true;
  }
  U32 peek32() const { return 0; }
  int peek_line() { return 0; }
  bool u(int fid, int n, S64& v) { return pos += n, num(fid, v); }
  bool s(int fid, int n, S64& v) { return pos += n, num(fid, v); }
  bool zz(int fid, int n, S64& v) { return pos += n, num(fid, v); }
  bool rice(int fid, int k, S64& v) { return pos += (zig((S32)v) >> k) + 1 + k, num(fid, v); }
  bool huff(int fid, const HuffBook& b, S64& v) { return pos += b.bits(v), num(fid, v); }
  bool val(int fid, S64, S64, S64& v) { return num(fid, v); }
  bool raw(int fid, U64 n, std::vector<U8>& b) {
    pos += n;
    if (!n) return true;
    std::string& c = col(fid);
    for (size_t i = 0; i < (n + 7) / 8; i++) c += hexd[b[i] >> 4], c += hexd[b[i] & 15];
    return true;
  }
  bool crc(int fo, int fw, U64, S64& ok, S64& w) {
    pos += 16;
    num(fo, ok);
    return ok || num(fw, w);
  }
  bool fixed(U32, int n, const char*) { return pos += n, true; }
  // the lines as text
  void render(std::string& out) const {
    for (int i = 0; i < nl; i++) {
      const Line& L = lines[i];
      out += LETTER[L.l];
      for (int k = 0; k < NKEYS[L.l]; k++) {
        out += '\t';
        if (L.l == L_U && k == 1)
          out += WHAT[L.key[1]];
        else
          put_int(out, L.key[k]);
      }
      for (size_t c = 0; c < L.col.size(); c++) {
        out += '\t';
        out += L.col[c].empty() ? "-" : L.col[c];
      }
      out += '\n';
    }
  }
};

// --- the lines of a text, one at a time (comments and empty lines skipped), from memory or a file ---
struct LineSrc {
  FILE* f;
  std::vector<char> buf;
  const char* base;
  size_t p, e;  // [p, e) of base not taken yet
  bool eof, bad, have;
  const char* ls;  // the line peeked
  size_t ln;
  U64 lineno;
  void init_mem(const char* t, size_t n) {
    f = 0;
    base = t;
    p = 0;
    e = n;
    eof = true;
    bad = have = false;
    lineno = 0;
  }
  void init_file(FILE* fp) {
    f = fp;
    buf.resize(1 << 20);
    base = &buf[0];
    p = e = 0;
    eof = bad = have = false;
    lineno = 0;
  }
  void refill() {
    if (p) {
      memmove(&buf[0], &buf[p], e - p);
      e -= p;
      p = 0;
    }
    if (e == buf.size()) buf.resize(buf.size() * 2);
    base = &buf[0];
    const size_t k = fread(&buf[e], 1, buf.size() - e, f);
    if (!k) {
      eof = true;
      bad = ferror(f) != 0;
    }
    e += k;
  }
  // the next line (s, n: valid until the next call), false at the end of the text
  bool next(const char*& s, size_t& n) {
    if (have) {
      have = false;
      s = ls;
      n = ln;
      return true;
    }
    for (;;) {
      const char* b = base + p;
      const char* nlp = (const char*)memchr(b, '\n', e - p);
      if (!nlp && !eof) {
        refill();
        continue;
      }
      if (!nlp && p == e) return false;
      size_t len = nlp ? (size_t)(nlp - b) : e - p;
      p += len + (nlp != 0);
      lineno++;
      if (len && b[len - 1] == '\r') len--;
      if (len && b[0] != '#') {
        s = ls = b;
        n = ln = len;
        return true;
      }
    }
  }
  bool peek(const char*& s, size_t& n) {
    if (!have && !next(s, n)) return false;
    have = true;
    s = ls;
    n = ln;
    return true;
  }
};

// --- the text read: TsvIn, the walk's IO that reads the record's values from text -------------------
enum { MAXCOL = 72 };
struct TsvIn {
  enum { BITS = 0, READ = 1 };
  U64 pos;
  const char* err;
  LineSrc* src;
  struct Line {
    std::string t;
    U32 at[MAXCOL], ce[MAXCOL];  // each column's next value and end
    U64 lineno;
    bool used;
  };
  Line in[L_NUM][NSLOT];
  int cur[L_NUM];
  char msg[128];
  U64 err_line;  // the line of the first error, 0 for none known
  void start(LineSrc* s) {
    src = s;
    pos = 0;
    err = 0;
    err_line = 0;
    for (int l = 0; l < L_NUM; l++) {
      cur[l] = 0;
      for (int k = 0; k < NSLOT; k++) in[l][k].used = false;
    }
  }
  bool fail(const char* why) {
    if (!err) err = why, err_line = src->lineno;
    return false;
  }
  bool bad(int fid, const char* what) {  // (what is wrong with an element's value)
    if (!err) {
      snprintf(msg, sizeof msg, "%s: %s", field_name[fid], what);
      err = msg;
      err_line = in[field_line[fid]][cur[field_line[fid]]].lineno;
    }
    return false;
  }
  static bool taken(const Line& L, int l) {
    for (int c = 0; c < line_ncols[l]; c++)
      if (L.at[c] < L.ce[c]) return false;
    return true;
  }
  bool extra(const Line& L) {  // (a line with a value the frame has no element for)
    if (!err) err = "a value too many", err_line = L.lineno;
    return false;
  }
  // whether every value of every line was taken
  bool done() {
    for (int l = 0; l < L_NUM; l++)
      for (int k = 0; k < NSLOT; k++)
        if (in[l][k].used && !taken(in[l][k], l)) return extra(in[l][k]);
    return !err;
  }
  void open(int l, int slot, int k0 = 0, int k1 = 0, int k2 = 0) {
    if (err) return;
    Line& L = in[l][slot];
    cur[l] = slot;
    if (L.used && !taken(L, l)) {
      extra(L);
      return;
    }
    L.used = false;
    const char* s;
    size_t n;
    if (!src->next(s, n)) {
      fail("the text ends inside a frame");
      return;
    }
    if (s[0] != LETTER[l] || (n > 1 && s[1] != '\t')) {
      fail("a line out of place");
      return;
    }
    L.t.assign(s, n);
    L.lineno = src->lineno;
    const char* t = L.t.data();
    const int nk = NKEYS[l], nc = line_ncols[l], keys[3] = {k0, k1, k2};
    size_t q = 1;
    for (int c = 0; c < nk + nc; c++) {
      if (q >= n || t[q] != '\t') {
        fail("a line with too few columns");
        return;
      }
      const size_t b = ++q;
      while (q < n && t[q] != '\t') q++;
      if (c < nk) {
        std::string want;
        if (l == L_U && c == 1)
          want = WHAT[keys[1]];
        else
          put_int(want, keys[c]);
        if (want.size() != q - b || memcmp(t + b, want.data(), q - b)) {
          fail("a line of another subframe, channel, band, segment or asset");
          return;
        }
      } else {
        L.at[c - nk] = (U32)b;
        L.ce[c - nk] = (U32)q;
        if (q - b == 1 && t[b] == '-') L.at[c - nk] = L.ce[c - nk];  // (none)
      }
    }
    if (q != n) {
      fail("a line with too many columns");
      return;
    }
    L.used = true;
  }
  void sel(int l, int slot) { cur[l] = slot; }
  Line* line_of(int fid) {
    Line& L = in[field_line[fid]][cur[field_line[fid]]];
    if (!L.used) {
      fail("an element before its line");
      return 0;
    }
    return &L;
  }
  // the next value of the field's column
  bool num(int fid, S64& v) {
    Line* L = line_of(fid);
    if (!L) return false;
    U32& a = L->at[field_col[fid]];
    const U32 e = L->ce[field_col[fid]];
    const char* t = L->t.data();
    if (a >= e) return bad(fid, "a value missing");
    const bool neg = t[a] == '-';
    if (neg) a++;
    U64 x = 0;
    int nd = 0;
    while (a < e && t[a] >= '0' && t[a] <= '9') {
      if (++nd > 18) return bad(fid, "not a number");
      x = x * 10 + (t[a++] - '0');
    }
    if (!nd || (a < e && t[a] != ',')) return bad(fid, "not a number");
    if (a < e && ++a == e) return bad(fid, "a comma with no value after it");
    v = neg ? -(S64)x : (S64)x;
    return true;
  }
  U32 peek32() const { return 0; }
  int peek_line() {
    const char* s;
    size_t n;
    return src->peek(s, n) ? s[0] : 0;
  }
  bool u(int fid, int n, S64& v) {
    if (!num(fid, v)) return false;
    if (v < 0 || (U64)v >> n) return bad(fid, "a value out of range");
    return pos += n, true;
  }
  bool s(int fid, int n, S64& v) {
    if (!num(fid, v)) return false;
    if (v < -(1LL << (n - 1)) || v >= 1LL << (n - 1)) return bad(fid, "a value out of range");
    return pos += n, true;
  }
  bool zz(int fid, int n, S64& v) {
    if (!num(fid, v)) return false;
    if (!n) return !v || bad(fid, "a value out of range");
    if (v < -(1LL << 31) || v >= 1LL << 31 || (U64)zig((S32)v) >> n) return bad(fid, "a value out of range");
    return pos += n, true;
  }
  bool rice(int fid, int k, S64& v) {
    if (!num(fid, v)) return false;
    if (v < -(1LL << 31) || v >= 1LL << 31 || zig((S32)v) >> k > 1u << 24) return bad(fid, "a value out of range");
    return pos += (zig((S32)v) >> k) + 1 + k, true;
  }
  bool huff(int fid, const HuffBook& b, S64& v) {
    if (!num(fid, v)) return false;
    if (!b.has(v)) return bad(fid, "a value its code book has no code for");
    return pos += b.bits(v), true;
  }
  bool val(int fid, S64 lo, S64 hi, S64& v) {
    if (!num(fid, v)) return false;
    return (v >= lo && v <= hi) || bad(fid, "a value out of range");
  }
  bool raw(int fid, U64 n, std::vector<U8>& b) {
    pos += n;
    b.resize((n + 7) / 8);
    if (!n) return true;
    Line* L = line_of(fid);
    if (!L) return false;
    U32& a = L->at[field_col[fid]];
    const U32 e = L->ce[field_col[fid]];
    const char* t = L->t.data();
    if ((U64)(e - a) < 2 * b.size()) return bad(fid, "fewer hex digits than its bits take");
    for (size_t i = 0; i < b.size(); i++, a += 2) {
      const int hi = hexv(t[a]), lo = hexv(t[a + 1]);
      if (hi < 0 || lo < 0) return bad(fid, "not hex");
      b[i] = (U8)(hi << 4 | lo);
    }
    if (n & 7 && b.back() & (0xFF >> (n & 7))) return bad(fid, "bits past its end that aren't 0");
    if (a < e && (t[a] != ',' || ++a == e)) return bad(fid, "more hex digits than its bits take");
    return true;
  }
  bool crc(int fo, int fw, U64, S64& ok, S64& w) {
    pos += 16;
    if (!num(fo, ok) || (ok != 0 && ok != 1)) return bad(fo, "not 0 or 1");
    if (!ok && (!num(fw, w) || w < 0 || w > 0xFFFF)) return bad(fw, "a value out of range");
    return true;
  }
  bool fixed(U32, int n, const char*) { return pos += n, true; }
};

// ===================================================================================================
// The conversions
// ===================================================================================================

static Frame g_f1, g_f2, g_probe;  // (large)
static TsvOut g_to;
static TsvIn g_ti;
static BitOut g_bo;

// the size of the core frame at p (av bytes there), where its header says one is there: the sync
// word and a frame size and block count in range; 0 where it isn't
static int core_size(const U8* p, size_t av) {
  if (av < 16 || be32(p) != SYNC_CORE) return 0;
  const U32 w = be32(p + 4);  // FTYPE SHORT CPF NBLKS FSIZE ...
  const int nblks = (w >> 18) & 0x7F, fsize = ((w >> 4) & 0x3FFF) + 1;
  if (nblks < 5 || fsize < 96 || (size_t)fsize > av) return 0;
  return fsize;
}

// the size of the extension substream frame at p, where it is one: the sync word and a header
// whose CRC holds; 0 where it isn't
static int exss_size(const U8* p, size_t av) {
  if (av < 16 || be32(p) != SYNC_EXSS) return 0;
  const U64 w = be64(p + 4);  // UserDefinedBits nExtSSIndex bHeaderSizeType sizes
  int hs, fs;
  if (w >> 53 & 1) {
    hs = (int)(w >> 41 & 0xFFF) + 1;
    fs = (int)(w >> 21 & 0xFFFFF) + 1;
  } else {
    hs = (int)(w >> 45 & 0xFF) + 1;
    fs = (int)(w >> 29 & 0xFFFF) + 1;
  }
  if (hs < 8 || hs > fs || (size_t)fs > av) return 0;
  return crc16(p + 5, hs - 5) ? 0 : fs;
}

// A frame of a text: its record walked from the lines of src (the first a C or an X line) into f,
// and its bytes into bo; 0, or why not. cx: what the extension substream frames before have said.
static const char* text_to_frame(LineSrc& src, Frame& f, Ctx& cx, BitOut& bo) {
  const char* s;
  size_t n;
  if (!src.peek(s, n)) return "the text ends";
  f.kind = s[0] == 'X';
  TsvIn& ti = g_ti;
  ti.start(&src);
  Ctx c1 = cx;
  Walk<TsvIn> w(ti, c1);
  if (!w.frame(f)) return ti.err ? ti.err : w.err ? w.err : "the frame";
  if (!ti.done()) return ti.err;
  bo.init();
  Ctx c2 = cx;
  Walk<BitOut> wb(bo, c2);
  if (!wb.frame(f)) return bo.err ? bo.err : wb.err ? wb.err : "its bits";
  if (bo.nacc || bo.pos != ti.pos) return "its bits";
  cx = c1;
  return 0;
}

// A frame (n bytes at p, PAD bytes readable after them; kind 0 core, 1 extension substream) as text,
// through its record and back: 0, or why it can't be (it doesn't parse, or doesn't come back the
// same). xll_raw: its XLL components as bytes.
static const char* frame_to_text(const U8* p, int n, int kind, Ctx& cx, std::string& text, int xll_raw) {
  Frame& f = g_f1;
  f.kind = kind;
  BitIn bi;
  bi.init(p, (U64)n * 8);
  Ctx c0 = cx;
  Walk<BitIn> w(bi, c0, xll_raw);
  if (!w.frame(f)) return bi.err ? bi.err : w.err ? w.err : "parse";
  if (bi.pos != (U64)n * 8) return "a frame size otherwise than its header's";
  TsvOut& to = g_to;
  to.start();
  Ctx c1 = cx;
  Walk<TsvOut> wt(to, c1);
  if (!wt.frame(f) || to.pos != bi.pos) {
    fprintf(stderr, "dts2tsv: internal error: the record's bits counted otherwise than the frame's\n");
    exit(EX_FAIL);
  }
  text.clear();
  to.render(text);
  // back from the text, as d does
  LineSrc src;
  src.init_mem(text.data(), text.size());
  Ctx c2 = cx;
  const char* why = text_to_frame(src, g_f2, c2, g_bo);
  if (why) return why;
  if (g_bo.v.size() != (size_t)n || memcmp(&g_bo.v[0], p, n)) return "not reproduced";
  cx = c0;
  return 0;
}

// whether the core frame p (n bytes) parses
static bool core_parses(const U8* p, int n) {
  BitIn bi;
  bi.init(p, (U64)n * 8);
  Ctx cx;
  cx.reset();
  Walk<BitIn> w(bi, cx);
  g_probe.kind = 0;
  return w.frame(g_probe) && bi.pos == (U64)n * 8;
}

// --- c ------------------------------------------------------------------------------------------------
struct Stats {
  U64 frames, core, exss, xll, xll_bytes, raw_frames, raw_bytes, text;
  char raw_why[8][128];  // (copies: a reason can be in a buffer used again)
  U64 raw_why_n[8];
  int nwhy;
  void why(const char* w) {
    for (int i = 0; i < nwhy; i++)
      if (!strcmp(raw_why[i], w)) {
        raw_why_n[i]++;
        return;
      }
    if (nwhy < 8) snprintf(raw_why[nwhy], sizeof raw_why[nwhy], "%s", w), raw_why_n[nwhy++] = 1;
  }
};

// the input, read through a window that holds the largest frame
struct Input {
  FILE* f;
  std::vector<U8> buf;
  size_t lo, hi;
  bool eof, bad;
  void init(FILE* fp) {
    f = fp;
    buf.resize((4 << 20) + PAD);
    lo = hi = 0;
    eof = bad = false;
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
          eof = true;
          bad = ferror(f) != 0;
        }
        hi += k;
      }
      memset(&buf[hi], 0, PAD);
    }
    return hi - lo;
  }
};
enum { LOOK = (1 << 20) + 64 };

// the text written, through a buffer
struct TextOut {
  FILE* f;
  std::string b;
  U64 n;
  bool ok;
  void init(FILE* fp) {
    f = fp;
    b.clear();
    n = 0;
    ok = true;
  }
  void flush() {
    if (!b.empty() && fwrite(b.data(), 1, b.size(), f) != b.size()) ok = false;
    n += b.size();
    b.clear();
  }
  void add(const std::string& s) {
    b += s;
    if (b.size() > (1 << 22)) flush();
  }
};

static void put_hex_lines(std::string& out, char letter, const U8* p, size_t n) {
  const size_t LINE = 32 * 1024;
  for (size_t i = 0; i < n; i += LINE) {
    const size_t k = n - i < LINE ? n - i : LINE;
    out += letter;
    out += '\t';
    for (size_t j = 0; j < k; j++) out += hexd[p[i + j] >> 4], out += hexd[p[i + j] & 15];
    out += '\n';
  }
}

static const int SFREQ_HZ[16] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};

// a comment on a frame's coding (written where it changes)
static void describe(const Frame& f, std::string& d) {
  char t[256];
  d.clear();
  if (!f.kind) {
    const CoreRec& c = f.core;
    snprintf(t, sizeof t, "# core: AMODE %d%s, %d Hz, %d primary channels, %d samples, %d bytes a frame\n", c.amode,
             c.lff ? " + LFE" : "", SFREQ_HZ[c.sfreq], c.npchs, (c.nblks + 1) * 32, c.fsize + 1);
    d = t;
    return;
  }
  const ExssRec& x = f.exss;
  snprintf(t, sizeof t, "# extension substream %d: %d asset%s", x.idx, x.nassets, x.nassets > 1 ? "s" : "");
  d = t;
  for (int a = 0; a < x.nassets; a++) {
    const ExssAsset& s = x.a[a];
    d += a ? "; " : ": ";
    if (!s.known) {
      d += "(no static fields yet)";
      continue;
    }
    static const char* const comp[7] = {"core", "XBR", "XXCH", "X96", "LBR", "XLL", "aux"};
    int any = 0;
    for (int i = 0; i < 6; i++)
      if (s.mask & (0x010 << i)) d += any++ ? " " : "", d += comp[i];
    if (s.mode == 3) d += any++ ? " " : "", d += comp[6];
    if (s.mask & 0x200 && s.xll_parsed) {
      const XllRec& l = s.xll;
      snprintf(t, sizeof t, " (%d channel%s in %d set%s, %d Hz, %d bits)", l.nch, l.nch > 1 ? "s" : "", l.nchsets,
               l.nchsets > 1 ? "s" : "", l.cs[0].freq, l.cs[0].bitres + 1);
      d += t;
    }
  }
  d += '\n';
}

// the stream of fi as text into fo
static bool stream_to_text(FILE* fi, FILE* fo, Stats& st, bool& read_ok) {
  Input in;
  in.init(fi);
  TextOut out;
  out.init(fo);
  std::string hdr = "# dts2tsv 1: a DTS stream as text; the columns of each line:\n";
  for (int l = 0; l < L_NUM; l++) hdr += line_header[l] + '\n';
  hdr += "#H\thex\n#R\thex\n";
  out.add(hdr);
  Ctx cx;
  cx.reset();
  std::vector<U8> between;
  std::string text, desc, last_desc[2], s;
  U64 at = 0;
  for (;;) {
    const size_t av = in.fill(LOOK);
    if (!av) break;
    const U8* p = &in.buf[in.lo];
    int n, kind = -1;
    if ((n = core_size(p, av))) {
      const bool follows = ((size_t)n == av && in.eof) ||
                           ((size_t)n + 4 <= av && (be32(p + n) == SYNC_CORE || be32(p + n) == SYNC_EXSS));
      if (follows || core_parses(p, n)) kind = 0;
    } else if ((n = exss_size(p, av))) {
      kind = 1;
    }
    if (kind < 0) {
      between.push_back(*p);
      if (between.size() == 32 * 1024) {
        s.clear();
        put_hex_lines(s, 'H', &between[0], between.size());
        out.add(s);
        st.raw_bytes += between.size();
        between.clear();
      }
      in.lo++;
      at++;
      continue;
    }
    if (!between.empty()) {
      s.clear();
      put_hex_lines(s, 'H', &between[0], between.size());
      out.add(s);
      st.raw_bytes += between.size();
      between.clear();
    }
    st.frames++;
    const char* why = frame_to_text(p, n, kind, cx, text, 0);
    if (why && kind == 1 && !frame_to_text(p, n, kind, cx, text, 1)) {
      why = 0;  // (its XLL kept as bytes)
      st.xll_bytes++;
    }
    if (why) {
      char c[160];
      snprintf(c, sizeof c, "# a frame at %llu kept as bytes: %s\n", (unsigned long long)at, why);
      s = c;
      put_hex_lines(s, 'R', p, n);
      out.add(s);
      st.raw_frames++;
      st.why(why);
    } else {
      describe(g_f1, desc);
      if (desc != last_desc[kind]) {
        out.add(desc);
        last_desc[kind] = desc;
      }
      out.add(text);
      if (kind) {
        st.exss++;
        for (int a = 0; a < g_f1.exss.nassets; a++) st.xll += g_f1.exss.a[a].known && g_f1.exss.a[a].mask & 0x200 && g_f1.exss.a[a].xll_parsed;
      } else {
        st.core++;
      }
    }
    in.lo += n;
    at += n;
  }
  if (!between.empty()) {
    s.clear();
    put_hex_lines(s, 'H', &between[0], between.size());
    out.add(s);
    st.raw_bytes += between.size();
  }
  out.flush();
  st.text = out.n;
  read_ok = !in.bad;
  return out.ok;
}

// --- d ------------------------------------------------------------------------------------------------
struct Sink {
  virtual bool put(const U8* p, size_t n) = 0;
  virtual ~Sink() {}
};
struct FileSink : Sink {
  FILE* f;
  bool put(const U8* p, size_t n) { return fwrite(p, 1, n, f) == n; }
};
// compares what is put with a file
struct CmpSink : Sink {
  FILE* f;
  U64 n, diff;  // bytes compared; the first that differs (~0 none)
  std::vector<U8> b;
  bool put(const U8* p, size_t k) {
    if (b.size() < k) b.resize(k);
    const size_t r = fread(b.data(), 1, k, f);
    for (size_t i = 0; i < k && diff == ~0ULL; i++)
      if (i >= r || b[i] != p[i]) diff = n + i;
    n += k;
    return true;
  }
};

// The stream of a text into out: 0, or why not (*line: where)
static const char* text_to_stream(LineSrc& src, Sink& out, U64* line, bool& write_ok) {
  Ctx cx;
  cx.reset();
  write_ok = true;
  std::vector<U8> hb;
  for (;;) {
    const char* s;
    size_t n;
    if (!src.peek(s, n)) return src.bad ? "can't read it" : 0;
    *line = src.lineno;
    if (*s == 'H' || *s == 'R') {
      src.next(s, n);
      if (n < 2 || s[1] != '\t' || (n & 1)) return "a hex line";
      hb.resize((n - 2) / 2);
      for (size_t i = 0; i < hb.size(); i++) {
        const int hi = hexv(s[2 + 2 * i]), lo = hexv(s[3 + 2 * i]);
        if (hi < 0 || lo < 0) return "a hex line";
        hb[i] = (U8)(hi << 4 | lo);
      }
      if (!hb.empty() && !out.put(&hb[0], hb.size())) write_ok = false;
      continue;
    }
    if (*s != 'C' && *s != 'X') return "a line out of place";
    const char* why = text_to_frame(src, g_f2, cx, g_bo);
    if (why) {
      *line = g_ti.err && g_ti.err_line ? g_ti.err_line : src.lineno;
      return why;
    }
    if (!out.put(&g_bo.v[0], g_bo.v.size())) write_ok = false;
  }
}

// --- the command line ----------------------------------------------------------------------------------
static void usage(void) {
  fprintf(stderr,
          "dts2tsv - lossless converter of DTS streams to TSV text and back\n"
          "Usage: dts2tsv c|d [options] input output\n"
          "  c    DTS stream -> TSV\n"
          "  d    TSV -> the stream, byte for byte\n"
          "  -v   (c) convert the TSV written back and compare it with the input\n"
          "  -q   no statistics\n"
          "input and output can be - for stdin and stdout (not with -v)\n");
}

int main(int argc, char** argv) {
  int verify = 0, quiet = 0, a;
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    usage();
    return EX_USAGE;
  }
  for (a = 2; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-v"))
      verify = 1;
    else if (!strcmp(argv[a], "-q"))
      quiet = 1;
    else {
      usage();
      return EX_USAGE;
    }
  }
  if (a != argc - 2) {
    usage();
    return EX_USAGE;
  }
  const char *in_name = argv[a], *out_name = argv[a + 1];
  if (verify && (!strcmp(in_name, "-") || !strcmp(out_name, "-"))) {
    fprintf(stderr, "dts2tsv: -v reads the input and the output again: they can't be stdin or stdout\n");
    return EX_USAGE;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  crc_init();
  huff_init();
  cols_init();
  FILE* fi = strcmp(in_name, "-") ? fopen(in_name, "rb") : stdin;
  if (!fi) {
    fprintf(stderr, "dts2tsv: can't open %s\n", in_name);
    return EX_READ;
  }
  FILE* fo = strcmp(out_name, "-") ? fopen(out_name, "wb") : stdout;
  if (!fo) {
    fprintf(stderr, "dts2tsv: can't write %s\n", out_name);
    return EX_WRITE;
  }
  if (argv[1][0] == 'd') {
    LineSrc src;
    src.init_file(fi);
    FileSink out;
    out.f = fo;
    U64 line = 0;
    bool write_ok;
    const char* why = text_to_stream(src, out, &line, write_ok);
    if (why) {
      fprintf(stderr, "dts2tsv: %s: line %llu: %s\n", in_name, (unsigned long long)line, why);
      return src.bad ? EX_READ : EX_DATA;
    }
    if (!write_ok || (fo == stdout ? fflush(fo) : fclose(fo))) {
      fprintf(stderr, "dts2tsv: can't write %s\n", out_name);
      return EX_WRITE;
    }
    return EX_OK;
  }
  Stats st;
  memset(&st, 0, sizeof st);
  bool read_ok;
  const bool write_ok = stream_to_text(fi, fo, st, read_ok);
  if (!read_ok) {
    fprintf(stderr, "dts2tsv: can't read %s\n", in_name);
    return EX_READ;
  }
  if (!write_ok || (fo == stdout ? fflush(fo) : fclose(fo))) {
    fprintf(stderr, "dts2tsv: can't write %s\n", out_name);
    return EX_WRITE;
  }
  if (!quiet) {
    fprintf(stderr, "%s: %llu frames (core %llu, extension substream %llu, %llu XLL frames", in_name,
            (unsigned long long)st.frames, (unsigned long long)st.core, (unsigned long long)st.exss,
            (unsigned long long)st.xll);
    if (st.xll_bytes) fprintf(stderr, ", %llu frames with their XLL as bytes", (unsigned long long)st.xll_bytes);
    fprintf(stderr, "), %llu kept as bytes", (unsigned long long)st.raw_frames);
    for (int i = 0; i < st.nwhy; i++) fprintf(stderr, "%s%s %llu", i ? ", " : " (", st.raw_why[i], (unsigned long long)st.raw_why_n[i]);
    fprintf(stderr, "%s; %llu bytes between frames; %llu bytes of text\n", st.nwhy ? ")" : "",
            (unsigned long long)st.raw_bytes, (unsigned long long)st.text);
  }
  if (verify) {
    FILE* ft = fopen(out_name, "rb");
    if (!ft || fseek(fi, 0, SEEK_SET)) {
      fprintf(stderr, "dts2tsv: can't read %s again\n", ft ? in_name : out_name);
      return EX_READ;
    }
    LineSrc src;
    src.init_file(ft);
    CmpSink cmp;
    cmp.f = fi;
    cmp.n = 0;
    cmp.diff = ~0ULL;
    U64 line = 0;
    bool w;
    const char* why = text_to_stream(src, cmp, &line, w);
    if (!why && cmp.diff == ~0ULL) {  // (and nothing of the input left over)
      U8 c;
      if (fread(&c, 1, 1, fi) == 1) cmp.diff = cmp.n;
    }
    if (why || cmp.diff != ~0ULL) {
      fprintf(stderr, "verify: FAILED (%s at byte %llu)\n", why ? why : "difference",
              (unsigned long long)(why ? cmp.n : cmp.diff));
      return EX_FAIL;
    }
    if (!quiet) fprintf(stderr, "verify: OK\n");
  }
  return EX_OK;
}
