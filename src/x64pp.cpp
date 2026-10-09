// x64pp - x86-64 executable preprocessor for LZMA/xz.
//
//   x64pp c input output    forward transform
//   x64pp d input output    inverse transform
//
// Code sections of PE32+ and ELF64 images are parsed into instructions.
// Opcode bytes (prefixes, opcode, ModRM, SIB) plus the small operands go to
// one stream; wide operands are split into separate streams by kind, with
// RIP-relative targets converted to absolute form.  Branch targets are coded
// through a set of labels that the decoder recovers by parsing the opcode
// stream first.  Unwind, exception, relocation and hash tables are delta
// coded or predicted from the code and from each other.  Everything else is
// copied.  Some codings are options chosen per file (-o, or -a: compress the
// candidates with the LZMA encoder of lzma.hpp).  See README.md for the format
// and the numbers.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <thread>
#include "lzma.hpp"
#include "x64dec.h"
#include "tables.h"
#include "analyze.h"
#include "tdelta.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

typedef std::vector<uint8_t> Buf;

enum StreamId {
  S_DATA,   // everything outside code regions
  S_OP,     // prefixes, opcode, ModRM, SIB, imm8, disp8, imm16
  S_J8,     // rel8 branch targets: rank among nearby labels
  S_JMP,    // rel32 jmp targets: label distance (1 byte), 0 / 255 = escape
  S_JCC,    // rel32 jcc targets: same coding
  S_CALL,   // rel32 call targets: label number, 0 = escape
  S_RIP,    // RIP-relative disp32: absolute address
  S_DISP,   // disp32
  S_IMM32,  // imm32
  S_IMM64,  // imm64, moffs64
  S_ESC,    // escaped rel32 targets: absolute address or label number
  S_INL,    // imm8, disp8, imm16 with FL_INLSEP (else they stay in S_OP)
  NSTREAM
};

// order of streams in the output file
static const uint8_t kOrder[4][NSTREAM] = {
    {S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_JCC, S_J8, S_INL, S_OP, S_DATA},
    {S_DATA, S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_JCC, S_J8, S_INL, S_OP},
    {S_OP, S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_JCC, S_J8, S_INL, S_DATA},
    {S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_JCC, S_J8, S_INL, S_DATA, S_OP}};

enum { kAlign = 16, kVersion = 3 };
// header flags; FL_LABELS is decided by the encoder, the others are coding
// options chosen per file (see main)
enum {
  FL_LABELS = 1,  // branch targets coded through labels
  FL_DDELTA = 2,  // disp32 as delta from the previous disp32 with the same base
  FL_DINL = 4,    // dictionary entries include the leading imm8/disp8/imm16
  FL_D8DELTA = 8, // disp8 too, mod 256 (disp8 and disp32 share the history)
  FL_RIPLAB = 16, // RIP-relative targets as moves in a sorted table of targets
  FL_VEXP = 32,   // VEX payload bits reordered: pp, L ahead of vvvv
  FL_INLSEP = 64, // imm8/disp8/imm16 in their own stream
  FL_ORD1 = 128,  // stream order: kOrder[flags >> 7 & 3]
  FL_ORD2 = 256,
  FL_TDELTA = 512, // tables of fixed-size records in the data stream delta coded
  FL_OPTIONS = FL_DDELTA | FL_DINL | FL_D8DELTA | FL_RIPLAB | FL_VEXP | FL_INLSEP | FL_ORD1 | FL_ORD2 | FL_TDELTA
};
// encoder-only choices, not stored: dictionary escape weight kDictWeight[opts >> 16 & 3]
enum { OPT_W1 = 1 << 16, OPT_W2 = 1 << 17 };
static const unsigned kDictWeight[4] = {4, 2, 8, 16};
// without -o or -a: the set that did best on average on the test files
enum { kDefaultOptions = FL_DINL | FL_VEXP | FL_TDELTA };
static const uint8_t kMagic[4] = {'x', '6', '4', 'p'};

//----------------------------------------------------------------------------
// I/O helpers

static bool readAll(FILE* f, Buf& b) {
  b.clear();
  uint8_t tmp[1 << 16];
  size_t r;
  while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) b.insert(b.end(), tmp, tmp + r);
  return !ferror(f);
}

static bool readFile(const char* name, Buf& b) {
  if (!strcmp(name, "-")) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    return readAll(stdin, b);
  }
  FILE* f = fopen(name, "rb");
  if (!f) return false;
  bool ok = readAll(f, b);
  fclose(f);
  return ok;
}

static bool writeFile(const char* name, const Buf& b) {
  bool sout = !strcmp(name, "-");
#ifdef _WIN32
  if (sout) _setmode(_fileno(stdout), _O_BINARY);
#endif
  FILE* f = sout ? stdout : fopen(name, "wb");
  if (!f) return false;
  bool ok = b.empty() || fwrite(b.data(), 1, b.size(), f) == b.size();
  ok &= fflush(f) == 0;
  if (!sout) ok &= fclose(f) == 0;
  return ok;
}

static void putVar(Buf& b, uint64_t v) {
  while (v >= 0x80) { b.push_back((uint8_t)(v | 0x80)); v >>= 7; }
  b.push_back((uint8_t)v);
}

static inline void putBE32(Buf& b, uint32_t v) {
  uint8_t t[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
  b.insert(b.end(), t, t + 4);
}

static inline void putLE(Buf& b, uint64_t v, unsigned n) {
  for (unsigned k = 0; k < n; k++) b.push_back((uint8_t)(v >> (8 * k)));
}

static inline uint32_t getBE32(const uint8_t* p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

struct Reader {
  const uint8_t* p;
  const uint8_t* e;
  bool ok;
  uint64_t var() {
    uint64_t v = 0;
    for (int s = 0; s < 64; s += 7) {
      if (p >= e) break;
      uint8_t c = *p++;
      v |= (uint64_t)(c & 0x7F) << s;
      if (!(c & 0x80)) return v;
    }
    ok = false;
    return 0;
  }
  uint8_t byte() {
    if (p >= e) { ok = false; return 0; }
    return *p++;
  }
};

// input stream with bounds checking; buffers carry 64 bytes of zero padding
// so the instruction decoder may look ahead without checks
struct InStream {
  const uint8_t* p;
  const uint8_t* e;
  bool take(size_t n) {
    if ((size_t)(e - p) < n) return false;
    p += n;
    return true;
  }
};

//----------------------------------------------------------------------------
// Labels
//
// A label is an instruction start that branches may target.  Explicit labels
// are announced in the opcode stream by the byte kMark (0xD6, an invalid
// opcode in 64-bit mode); a genuine 0xD6 instruction is written as
// kMark kMark and never gets an explicit label.  Implicit labels cost
// nothing: the first instruction of a code region and the first non-padding
// instruction after an unconditional control transfer.
// Branch targets are coded through the sorted label set:
//   jmp/jcc rel32  z = zigzag(label distance from the next instruction) + 1
//                  in one byte if z < 255; else 255, and the label number + 1
//                  in S_ESC (far jumps are often tail calls to functions)
//   call rel32     label number + 1, big-endian 32-bit
//   rel8           rank among the 256 possible targets, labels first
// A code of 0 means the target is not a label; its address is in S_ESC.

enum { kMark = 0xD6 };

//----------------------------------------------------------------------------
// Opcode dictionary: the most frequent instruction skeletons (structural
// bytes) of the file get one-byte codes.  The code bytes are the rarest
// first bytes of instructions in the file.  An instruction that starts with
// a code byte or with kMark, and has no code itself, is escaped: kMark kMark,
// then the instruction as is (so a genuine 0xD6 becomes D6 D6 D6).  Escaped
// instructions never get a label marker, so kMark kMark is unambiguous.

enum { kMaxSkel = 15 };

// a skeleton as a fixed-size key: the bytes, zero filled, length in byte 15
struct Skel {
  uint64_t a, b;  // b != 0 for every skeleton (length >= 2)
  bool operator==(const Skel& o) const { return a == o.a && b == o.b; }
  bool operator<(const Skel& o) const { return str() < o.str(); }
  size_t len() const { return (size_t)(b >> 56); }
  std::string str() const {
    uint8_t t[16];
    memcpy(t, &a, 8);
    memcpy(t + 8, &b, 8);
    return std::string((const char*)t, len());
  }
  static Skel of(const uint8_t* p, size_t n) {
    uint8_t t[16] = {};
    memcpy(t, p, n);
    t[15] = (uint8_t)n;
    Skel s;
    memcpy(&s.a, t, 8);
    memcpy(&s.b, t + 8, 8);
    return s;
  }
};

// open addressing map Skel -> uint64_t
struct SkelMap {
  std::vector<Skel> key;
  std::vector<uint64_t> val;
  size_t used = 0;
  SkelMap() : key(256, Skel{0, 0}), val(256, 0) {}
  size_t home(const Skel& s) const {
    uint64_t h = (s.a ^ (s.b * 0x9E3779B97F4A7C15ull)) * 0xBF58476D1CE4E5B9ull;
    return (size_t)(h ^ h >> 29) & (key.size() - 1);
  }
  const uint64_t* find(const Skel& s) const {
    for (size_t i = home(s);; i = (i + 1) & (key.size() - 1)) {
      if (key[i] == s) return &val[i];
      if (!key[i].b) return nullptr;
    }
  }
  uint64_t& operator[](const Skel& s) {
    if (2 * (used + 1) > key.size()) grow();
    size_t i = home(s);
    for (; key[i].b && !(key[i] == s); i = (i + 1) & (key.size() - 1)) {}
    if (!key[i].b) { key[i] = s; val[i] = 0; used++; }
    return val[i];
  }
  void grow() {
    std::vector<Skel> k(key.size() * 2, Skel{0, 0});
    std::vector<uint64_t> v(k.size(), 0);
    k.swap(key);
    v.swap(val);
    used = 0;
    for (size_t i = 0; i < k.size(); i++) if (k[i].b) (*this)[k[i]] = v[i];
  }
};

static inline bool inlineField(unsigned c) { return c == x64::F_D8 || c == x64::F_I8 || c == x64::F_I16; }

// bytes of the inline operands that directly follow the structural bytes
static inline size_t leadInline(const x64::Insn& I) {
  size_t n = 0;
  for (unsigned k = 0; k < I.nfield && inlineField(I.fclass[k]); k++) n += I.fsize[k];
  return n;
}

struct OpDict {
  bool dinl = false;        // FL_DINL: entries are skeleton + leading inline operands
  bool isCode[256] = {};
  std::string entry[256];   // code -> skeleton
  std::string padded[256];  // skeleton + zeros for the decoder
  SkelMap code;             // skeleton -> code, encoder only
  size_t n = 0;
  size_t size() const { return n; }
  void add(uint8_t c, const std::string& sk) {
    isCode[c] = true;
    entry[c] = sk;
    padded[c] = sk + std::string(64, '\0');
    code[Skel::of((const uint8_t*)sk.data(), sk.size())] = c;
    n++;
  }
  // length of the dictionary key of an instruction, 0 if it can't have one
  size_t keyLen(const x64::Insn& I) const {
    size_t k = I.nstruct + (dinl ? leadInline(I) : 0);
    return I.trunc || I.nstruct < 2 || k > kMaxSkel ? 0 : k;
  }
  // encoder: code for the instruction at p, or -1
  int find(const uint8_t* p, const x64::Insn& I) const {
    size_t k = n ? keyLen(I) : 0;
    if (!k) return -1;
    const uint64_t* v = code.find(Skel::of(p, k));
    return v ? (int)*v : -1;
  }
  bool escaped(const uint8_t* p, const x64::Insn& I) const {
    return (p[0] == kMark || isCode[p[0]]) && find(p, I) < 0;
  }
};

// Pairs the k-th most frequent skeleton with the k-th rarest first byte for
// as long as that saves more bytes than the escapes and the entry cost.
static void buildDict(const std::vector<Region>& R, const uint8_t* img, unsigned weight, OpDict& D) {
  SkelMap cnt;
  uint64_t first[256] = {};
  for (auto& r : R) {
    if (r.type != R_CODE) continue;
    const uint8_t* p = img + r.off;
    size_t n = (size_t)r.size, i = 0;
    while (i < n) {
      x64::Insn I;
      x64::decode(p + i, n - i, I);
      first[p[i]]++;
      if (size_t k = D.keyLen(I)) cnt[Skel::of(p + i, k)]++;
      i += I.len;
    }
  }
  std::vector<std::pair<uint64_t, Skel> > byCount;
  for (size_t i = 0; i < cnt.key.size(); i++)
    if (cnt.key[i].b) byCount.push_back({cnt.val[i], cnt.key[i]});
  std::sort(byCount.begin(), byCount.end(), [](const std::pair<uint64_t, Skel>& a, const std::pair<uint64_t, Skel>& b) {
    return a.first != b.first ? a.first > b.first : a.second < b.second;
  });
  std::vector<std::pair<uint64_t, int> > rare;
  for (int b = 0; b < 256; b++) if (b != kMark) rare.push_back({first[b], b});
  std::sort(rare.begin(), rare.end());
  size_t k = 0;
  for (; k < byCount.size() && k < rare.size(); k++) {
    uint64_t len = byCount[k].second.len(), saved = byCount[k].first * (len - 1);
    if (saved <= weight * rare[k].first + len + 2) break;
  }
  // which code goes with which skeleton hardly matters; a canonical pairing
  // (both sorted) lets the container store a code bitmap and a sorted list
  std::vector<std::string> sk;
  std::vector<uint8_t> codes;
  for (size_t j = 0; j < k; j++) {
    sk.push_back(byCount[j].second.str());
    codes.push_back((uint8_t)rare[j].second);
  }
  std::sort(sk.begin(), sk.end());
  std::sort(codes.begin(), codes.end());
  for (size_t j = 0; j < k; j++) D.add(codes[j], sk[j]);
}

struct Labels {
  std::vector<uint64_t> va;  // sorted, unique
  size_t lower(uint64_t t) const { return std::lower_bound(va.begin(), va.end(), t) - va.begin(); }
  bool find(uint64_t t, size_t& r) const {
    r = lower(t);
    return r < va.size() && va[r] == t;
  }
  void finish() {
    std::sort(va.begin(), va.end());
    va.erase(std::unique(va.begin(), va.end()), va.end());
  }
};

static inline bool isPadding(const x64::Insn& I) {
  return !I.trunc && (I.op == 0x90 || I.op == 0xCC || I.op == 0x11F);  // nop, int3, nop r/m
}

static inline bool isBarrier(const x64::Insn& I) {
  if (I.trunc) return false;
  switch (I.op) {
    case 0xC2: case 0xC3: case 0xE9: case 0xEB: case 0xF4: case 0xCC: case 0x10B:  // ret jmp hlt int3 ud2
      return true;
    case 0xFF: {
      unsigned reg = (I.modrm >> 3) & 7;  // jmp r/m, jmp far
      return reg == 4 || reg == 5;
    }
    default:
      return false;
  }
}

struct ImplicitLabels {
  bool after = true;  // region start counts as "after a transfer"
  bool step(const x64::Insn& I) {
    bool lab = after && !isPadding(I);
    if (lab) after = false;
    if (isBarrier(I)) after = true;
    return lab;
  }
};

static inline uint32_t zigzag(int64_t d) { return (uint32_t)((uint64_t)d << 1 ^ (uint64_t)(d >> 63)); }
static inline int64_t unzigzag(uint32_t z) { return (int64_t)(z >> 1) ^ -(int64_t)(z & 1); }

// rel8 targets: the 256 addresses next-128..next+127 are ranked, labels
// first (by zigzag of the label distance from `next`), then the other
// addresses in increasing order.  The code is the rank.
struct J8Win {
  size_t b0, b1, rn;  // labels in the window are [b0,b1); rn = first label >= next
  uint64_t lo;
  bool ok;
  J8Win(const Labels& L, uint64_t next) {
    ok = next >= 128 && next <= UINT64_MAX - 128;
    b0 = b1 = rn = 0;
    lo = next - 128;
    if (!ok) return;
    b0 = L.lower(lo);
    b1 = L.lower(next + 128);
    rn = L.lower(next);
  }
};

static uint8_t j8Encode(const Labels& L, uint64_t next, uint8_t rel) {
  J8Win w(L, next);
  if (!w.ok) return rel;
  uint64_t t = next + (uint64_t)(int64_t)(int8_t)rel;
  size_t r;
  if (L.find(t, r)) {
    // count labels in the window with a smaller key
    int64_t d = (int64_t)r - (int64_t)w.rn;
    int64_t dmin = (int64_t)w.b0 - (int64_t)w.rn, dmax = (int64_t)w.b1 - 1 - (int64_t)w.rn;
    uint32_t z = zigzag(d);
    if (z == 0) return 0;
    int64_t hi = (int64_t)((z - 1) / 2), lo = -(int64_t)(z / 2);
    int64_t a = std::max(dmin, lo), b = std::min(dmax, hi);
    return (uint8_t)(b >= a ? b - a + 1 : 0);
  }
  return (uint8_t)((w.b1 - w.b0) + (t - w.lo) - (r - w.b0));
}

static uint8_t j8Decode(const Labels& L, uint64_t next, uint8_t code) {
  J8Win w(L, next);
  if (!w.ok) return code;
  size_t nb = w.b1 - w.b0;
  uint64_t t;
  if (code < nb) {
    int64_t dmin = (int64_t)w.b0 - (int64_t)w.rn, dmax = (int64_t)w.b1 - 1 - (int64_t)w.rn;
    unsigned cnt = 0;
    int64_t d = 0;
    for (uint32_t k = 0;; k++) {
      d = (k & 1) ? -(int64_t)((k + 1) / 2) : (int64_t)(k / 2);
      if (d < dmin || d > dmax) continue;
      if (cnt++ == code) break;
    }
    t = L.va[(size_t)((int64_t)w.rn + d)];
  } else {
    t = w.lo + (code - nb);
    for (size_t k = w.b0; k < w.b1 && L.va[k] <= t; k++) t++;
  }
  return (uint8_t)(t - next);
}

//----------------------------------------------------------------------------
// Code regions

static inline uint64_t fieldValue(const uint8_t* q, unsigned sz) {
  uint64_t v = 0;
  for (unsigned j = 0; j < sz; j++) v |= (uint64_t)q[j] << (8 * j);
  return v;
}

static inline bool branchTarget(unsigned c, uint64_t v, uint64_t next, uint64_t& t) {
  if (c == x64::F_J8) t = next + (uint64_t)(int64_t)(int8_t)v;
  else if (c == x64::F_JMP || c == x64::F_JCC || c == x64::F_CALL) t = next + (uint64_t)(int64_t)(int32_t)v;
  else return false;
  return true;
}

// encoder: instruction starts that may become explicit labels
static void collectStarts(const uint8_t* p, size_t n, uint64_t va, const OpDict& D, Labels& B) {
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(p + i, n - i, I);
    if (!D.escaped(p + i, I)) B.va.push_back(va + i);
    i += I.len;
  }
}

// encoder: label set = branch targets that are instruction starts + implicit labels
static void collectLabels(const uint8_t* p, size_t n, uint64_t va, const Labels& B, Labels& L) {
  ImplicitLabels imp;
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(p + i, n - i, I);
    if (imp.step(I)) L.va.push_back(va + i);
    const uint8_t* q = p + i + I.nstruct;
    uint64_t next = va + i + I.len, t;
    size_t r;
    for (unsigned k = 0; k < I.nfield; k++) {
      unsigned c = I.fclass[k], sz = I.fsize[k];
      if (branchTarget(c, fieldValue(q, sz), next, t) && B.find(t, r)) L.va.push_back(t);
      q += sz;
    }
    i += I.len;
  }
}

// FL_VEXP: in the VEX byte holding vvvv, L and pp (the only payload byte of
// VEX2, the second of VEX3) pp and L move to the top, where LZMA's literal
// context (the top 3 bits of the previous byte) then predicts the opcode.
// Instruction lengths don't depend on that byte.
static inline int vexPermPos(const x64::Insn& I) {
  if (I.trunc || I.pvex == x64::NOPOS) return -1;
  if (I.enc == x64::ENC_VEX2) return I.pvex;
  if (I.enc == x64::ENC_VEX3) return I.pvex + 1;
  return -1;
}
static inline uint8_t vexPerm(uint8_t b) {
  return (uint8_t)((b & 3) << 6 | (b >> 2 & 1) << 5 | (b >> 3 & 0x10) | (b >> 3 & 15));
}
static inline uint8_t vexUnperm(uint8_t c) {
  return (uint8_t)((c & 0x10) << 3 | (c & 15) << 3 | (c >> 5 & 1) << 2 | c >> 6);
}

// FL_RIPLAB: the distinct RIP-relative targets of the file, sorted, are a
// table in the header; each reference is coded as the move from the previous
// reference's table index: zigzag in one byte, or 255 and the index in S_ESC.

// FL_DDELTA: a disp32 with base register b is coded as the difference from
// the previous disp32 with base b, if that one is at most kDWin instructions
// back in the same straight-line segment.  Unrolled loops then repeat
// exactly; elsewhere the absolute offsets usually repeat better.
enum { kDWin = 64 };

static unsigned baseReg(const uint8_t* sk, const x64::Insn& I) {
  unsigned b = I.modrm & 7, ext = 0;
  if (b == 4 && I.psib != x64::NOPOS) b = sk[I.psib] & 7;
  if (I.prex != x64::NOPOS) ext = sk[I.prex] & 1;
  else if ((I.enc == x64::ENC_VEX3 || I.enc == x64::ENC_EVEX) && I.pvex != x64::NOPOS) ext = !(sk[I.pvex] & 0x20);
  return b | ext << 3;
}

struct DispHist {
  uint32_t last[16];
  uint64_t at[16];
  uint64_t idx;
  void reset() {
    memset(last, 0, sizeof last);
    memset(at, 0, sizeof at);
    idx = 1;
  }
  // reference for the next disp32 with base b; false = code it as is
  bool ref(unsigned b, uint32_t& v) const {
    v = last[b];
    return at[b] && idx - at[b] <= kDWin;
  }
  void put(unsigned b, uint32_t v) { last[b] = v; at[b] = idx; }
  void step(const x64::Insn& I) {
    idx++;
    if (isBarrier(I)) memset(at, 0, sizeof at);
  }
};

// encoder: RIP-relative targets (FL_RIPLAB)
static void collectRip(const uint8_t* p, size_t n, uint64_t va, std::vector<uint32_t>& T) {
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(p + i, n - i, I);
    const uint8_t* q = p + i + I.nstruct;
    for (unsigned k = 0; k < I.nfield; k++) {
      if (I.fclass[k] == x64::F_RIP) T.push_back((uint32_t)(fieldValue(q, 4) + va + i + I.len));
      q += I.fsize[k];
    }
    i += I.len;
  }
}

struct Encoder {
  Buf S[NSTREAM];
  const Labels* L;
  const OpDict* D;
  const std::vector<uint32_t>* ripTab;  // FL_RIPLAB
  int64_t ripPrev = 0;
  unsigned flags;
  uint64_t ninsn = 0, ncoded = 0;

  void code(const uint8_t* p, size_t n, uint64_t va) {
    size_t i = 0;
    Buf& op = S[S_OP];
    ImplicitLabels imp;
    DispHist dh;
    dh.reset();
    bool lab = flags & FL_LABELS, ddelta = flags & FL_DDELTA, d8delta = flags & FL_D8DELTA;
    Buf& inl = flags & FL_INLSEP ? S[S_INL] : op;
    while (i < n) {
      x64::Insn I;
      x64::decode(p + i, n - i, I);
      int dc = D->find(p + i, I);
      size_t iskip = 0;  // inline operand bytes that are part of the dictionary entry
      bool esc = dc < 0 && (p[i] == kMark || D->isCode[p[i]]);
      // an escaped instruction is never a target (collectStarts), but its
      // address may still be a label from an overlapping region
      size_t r;
      if (lab && !imp.step(I) && !esc && L->find(va + i, r)) op.push_back(kMark);
      ninsn++;
      if (dc >= 0) {
        op.push_back((uint8_t)dc);
        ncoded++;
        if (D->dinl) iskip = leadInline(I);
      } else {
        if (esc) { op.push_back(kMark); op.push_back(kMark); }
        size_t o0 = op.size();
        op.insert(op.end(), p + i, p + i + I.nstruct);
        int vp = flags & FL_VEXP ? vexPermPos(I) : -1;
        if (vp >= 0) op[o0 + vp] = vexPerm(op[o0 + vp]);
      }
      const uint8_t* q = p + i + I.nstruct;
      uint64_t next = va + i + I.len;
      for (unsigned k = 0; k < I.nfield; k++) {
        unsigned c = I.fclass[k], sz = I.fsize[k];
        uint64_t v = fieldValue(q, sz), t;
        q += sz;
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16:
            if (c == x64::F_D8 && d8delta) {
              unsigned b = baseReg(p + i, I);
              uint32_t ref = 0;
              if (!dh.ref(b, ref)) ref = 0;
              dh.put(b, (uint32_t)(int32_t)(int8_t)v);
              v = (uint8_t)(v - ref);
            }
            if (iskip) iskip -= sz;
            else putLE(inl, v, sz);
            break;
          case x64::F_J8:
            S[S_J8].push_back(lab ? j8Encode(*L, next, (uint8_t)v) : (uint8_t)v);
            break;
          case x64::F_CALL:
            branchTarget(c, v, next, t);
            if (!lab) putBE32(S[S_CALL], (uint32_t)t);
            else if (!L->find(t, r)) { putBE32(S[S_CALL], 0); putBE32(S[S_ESC], (uint32_t)t); }
            else putBE32(S[S_CALL], (uint32_t)(r + 1));
            break;
          case x64::F_JMP: case x64::F_JCC: {
            Buf& st = S[c == x64::F_JMP ? S_JMP : S_JCC];
            branchTarget(c, v, next, t);
            if (!lab) putBE32(st, (uint32_t)t);
            else if (!L->find(t, r)) { st.push_back(0); putBE32(S[S_ESC], (uint32_t)t); }
            else {
              uint32_t z = zigzag((int64_t)r - (int64_t)L->lower(next)) + 1;
              if (z < 255) st.push_back((uint8_t)z);
              else { st.push_back(255); putBE32(S[S_ESC], (uint32_t)(r + 1)); }
            }
            break;
          }
          case x64::F_RIP: {
            uint32_t a = (uint32_t)(v + next);
            if (!(flags & FL_RIPLAB)) { putBE32(S[S_RIP], a); break; }
            int64_t x = std::lower_bound(ripTab->begin(), ripTab->end(), a) - ripTab->begin();
            uint32_t z = zigzag(x - ripPrev);
            if (z < 255) S[S_RIP].push_back((uint8_t)z);
            else { S[S_RIP].push_back(255); putBE32(S[S_ESC], (uint32_t)x); }
            ripPrev = x;
            break;
          }
          case x64::F_D32: {
            uint32_t ref = 0;
            if (ddelta || d8delta) {
              unsigned b = baseReg(p + i, I);
              if (!dh.ref(b, ref) || !ddelta) ref = 0;
              dh.put(b, (uint32_t)v);
            }
            putBE32(S[S_DISP], (uint32_t)v - ref);
            break;
          }
          case x64::F_DABS: putBE32(S[S_DISP], (uint32_t)v); break;
          case x64::F_I32: putBE32(S[S_IMM32], (uint32_t)v); break;
          default: putLE(S[S_IMM64], v, sz); break;  // F_I64
        }
      }
      dh.step(I);
      i += I.len;
    }
  }
};

// decoder: where the structural bytes of the next instruction are
struct InsnSrc {
  const uint8_t* sp;  // in the opcode stream or in a dictionary entry
  bool marked;        // label marker seen
  bool dict;          // from the dictionary
  size_t held;        // inline operand bytes that follow sp in the entry (FL_DINL)
};

static bool readInsn(InStream& op, const OpDict& D, size_t avail, x64::Insn& I, InsnSrc& src) {
  src.marked = src.dict = false;
  src.held = 0;
  bool literal = false;
  if (op.p < op.e && op.p[0] == kMark) {
    if (op.p + 1 < op.e && op.p[1] == kMark) { op.p += 2; literal = true; }
    else { op.p++; src.marked = true; }
  }
  if (!literal && op.p < op.e && D.isCode[op.p[0]]) {
    uint8_t c = *op.p++;
    src.sp = (const uint8_t*)D.padded[c].data();
    src.dict = true;
    x64::decode(src.sp, avail, I);
    if (D.keyLen(I) != D.entry[c].size()) return false;
    src.held = D.entry[c].size() - I.nstruct;
    return true;
  }
  src.sp = op.p;
  x64::decode(src.sp, avail, I);
  return op.take(I.nstruct);
}

// decoding pass 1: labels from the opcode stream alone
static bool collectLabelsOp(InStream& op, const OpDict& D, bool inlsep, size_t n, uint64_t va, Labels& L) {
  ImplicitLabels imp;
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    InsnSrc src;
    if (!readInsn(op, D, n - i, I, src)) return false;
    size_t adv = 0;
    for (unsigned k = 0; k < I.nfield; k++) if (inlineField(I.fclass[k])) adv += I.fsize[k];
    if (!inlsep && !op.take(adv - src.held)) return false;
    if (imp.step(I) || src.marked) L.va.push_back(va + i);
    i += I.len;
  }
  return true;
}

struct Decoder {
  InStream S[NSTREAM];
  const Labels* L;
  const OpDict* D;
  const std::vector<uint32_t>* ripTab;  // FL_RIPLAB
  int64_t ripPrev = 0;
  unsigned flags;

  bool escaped(uint32_t& x) {
    const uint8_t* s = S[S_ESC].p;
    if (!S[S_ESC].take(4)) return false;
    x = getBE32(s);
    return true;
  }

  // y: call label number + 1, or jmp/jcc z code; 0 = absolute target in S_ESC
  bool target(uint32_t y, bool call, uint64_t next, uint64_t& v) {
    if (y == 0) {
      uint32_t a;
      if (!escaped(a)) return false;
      v = (uint32_t)(a - next);
      return true;
    }
    int64_t r = call ? (int64_t)y - 1 : (int64_t)L->lower(next) + unzigzag(y - 1);
    if (r < 0 || r >= (int64_t)L->va.size()) return false;
    v = (uint32_t)(L->va[(size_t)r] - next);
    return true;
  }

  bool code(uint8_t* out, size_t n, uint64_t va) {
    size_t i = 0;
    InStream& op = S[S_OP];
    DispHist dh;
    dh.reset();
    bool lab = flags & FL_LABELS, ddelta = flags & FL_DDELTA, d8delta = flags & FL_D8DELTA;
    InStream& inl = flags & FL_INLSEP ? S[S_INL] : op;
    while (i < n) {
      x64::Insn I;
      InsnSrc src;
      if (!readInsn(op, *D, n - i, I, src)) return false;
      const uint8_t* s = src.sp;
      memcpy(out + i, s, I.nstruct);
      int vp = flags & FL_VEXP && !src.dict ? vexPermPos(I) : -1;
      if (vp >= 0) out[i + vp] = vexUnperm(out[i + vp]);
      size_t held = src.held;
      const uint8_t* hp = s + I.nstruct;  // inline operands held by the dictionary entry
      uint8_t* q = out + i + I.nstruct;
      uint64_t next = va + i + I.len;
      for (unsigned k = 0; k < I.nfield; k++) {
        unsigned c = I.fclass[k], sz = I.fsize[k];
        uint64_t v = 0;
        InStream* st;
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16: {
            bool inEntry = held != 0;  // leading fields, exactly `held` bytes (readInsn)
            if (inEntry) {
              v = fieldValue(hp, sz);
              hp += sz;
              held -= sz;
            } else {
              s = inl.p;
              if (!inl.take(sz)) return false;
              v = fieldValue(s, sz);
            }
            if (c == x64::F_D8 && d8delta) {
              unsigned b = baseReg(out + i, I);
              uint32_t ref;
              if (dh.ref(b, ref) && !inEntry) v = (uint8_t)(v + ref);
              dh.put(b, (uint32_t)(int32_t)(int8_t)v);
            }
            break;
          }
          case x64::F_J8:
            s = S[S_J8].p;
            if (!S[S_J8].take(1)) return false;
            v = lab ? j8Decode(*L, next, s[0]) : s[0];
            break;
          case x64::F_CALL:
            s = S[S_CALL].p;
            if (!S[S_CALL].take(4)) return false;
            if (!lab) v = (uint32_t)(getBE32(s) - next);
            else if (!target(getBE32(s), true, next, v)) return false;
            break;
          case x64::F_JMP: case x64::F_JCC: {
            st = &S[c == x64::F_JMP ? S_JMP : S_JCC];
            s = st->p;
            if (!lab) {
              if (!st->take(4)) return false;
              v = (uint32_t)(getBE32(s) - next);
              break;
            }
            if (!st->take(1)) return false;
            uint32_t z = s[0];
            if (z == 255) {  // far: label number + 1
              if (!escaped(z) || z == 0 || !target(z, true, next, v)) return false;
            } else if (!target(z, false, next, v)) return false;
            break;
          }
          case x64::F_RIP:
            if (flags & FL_RIPLAB) {
              s = S[S_RIP].p;
              if (!S[S_RIP].take(1)) return false;
              int64_t x;
              uint32_t e;
              if (s[0] == 255) {
                if (!escaped(e)) return false;
                x = e;
              } else x = ripPrev + unzigzag(s[0]);
              if (x < 0 || x >= (int64_t)ripTab->size()) return false;
              ripPrev = x;
              v = (uint32_t)((*ripTab)[(size_t)x] - next);
              break;
            }
            s = S[S_RIP].p;
            if (!S[S_RIP].take(4)) return false;
            v = (uint32_t)(getBE32(s) - next);
            break;
          case x64::F_D32: case x64::F_DABS: case x64::F_I32:
            st = &S[c == x64::F_I32 ? S_IMM32 : S_DISP];
            s = st->p;
            if (!st->take(4)) return false;
            v = getBE32(s);
            if (c == x64::F_D32 && (ddelta || d8delta)) {
              unsigned b = baseReg(out + i, I);
              uint32_t ref;
              if (dh.ref(b, ref) && ddelta) v = (uint32_t)(v + ref);
              dh.put(b, (uint32_t)v);
            }
            break;
          default:  // F_I64
            s = S[S_IMM64].p;
            if (!S[S_IMM64].take(sz)) return false;
            v = fieldValue(s, sz);
            break;
        }
        for (unsigned j = 0; j < sz; j++) q[j] = (uint8_t)(v >> (8 * j));
        q += sz;
      }
      dh.step(I);
      i += I.len;
    }
    return true;
  }
};

// table transforms; .eh_frame_hdr needs the untransformed .eh_frame in `img`
static bool plainRange(const std::vector<Region>& R, uint64_t off, uint64_t len, uint64_t n);
static tables::CodeMap codeMap(const std::vector<Region>& R, const uint8_t* img) {
  tables::CodeMap cm;
  cm.img = img;
  for (auto& r : R)
    if (r.type == R_CODE) {
      cm.spans.push_back({r.va, r.off, r.size});
      cm.codeBytes += r.size;
    }
  std::stable_sort(cm.spans.begin(), cm.spans.end(), [](const tables::CodeSpan& a, const tables::CodeSpan& b) { return a.va < b.va; });
  return cm;
}

static void tableTransform(uint8_t* p, const Region& r, bool fwd, const std::vector<Region>& R, const uint8_t* img, uint64_t n,
                           const tables::CodeMap& cm) {
  switch (r.type) {
    case R_PDATA:
      tables::pdata(p, (size_t)r.size, fwd, r.par.empty() ? nullptr : &cm, r.par.empty() ? 0 : r.par[0]);
      break;
    case R_EHHDR: {
      tables::HdrPred pred;
      if (r.par.size() == 1 && r.par[0] < R.size() && R[(size_t)r.par[0]].type == R_EHFRAME) {
        const Region& e = R[(size_t)r.par[0]];
        tables::ehhdrPredict(img + e.off, (size_t)e.size, e.va, r.va - 12, pred);
      }
      tables::ehhdr(p, (size_t)r.size, fwd, pred);
      break;
    }
    case R_GNUHASH:
      // .dynsym and .dynstr must be plain data, so both sides see the original bytes
      if (r.par.size() == 4 && plainRange(R, r.par[0], r.par[1], n) && plainRange(R, r.par[2], r.par[3], n))
        tables::gnuhash(p, (size_t)r.size, img + r.par[0], r.par[1], img + r.par[2], r.par[3]);
      break;
    case R_RELA: tables::rela(p, (size_t)r.size, fwd); break;
    case R_RELOC: tables::reloc(p, (size_t)r.size, fwd); break;
    case R_EXCEPT:  // par: the .eh_frame region, LSDAs transformed
      if (r.par.size() == 2 && r.par[1] && r.par[0] < R.size() && R[(size_t)r.par[0]].type == R_EHFRAME &&
          !R[(size_t)r.par[0]].par.empty()) {
        const Region& e = R[(size_t)r.par[0]];
        std::vector<tables::LsdaRef> refs;
        tables::lsdaRefs(img + e.off, (size_t)e.size, e.va, r.va, r.size, refs);
        tables::lsdaTable(p, (size_t)r.size, refs, cm, e.par[0], fwd);
      }
      break;
    case R_EHFRAME:  // par: image VA bias, CFA prediction style
      tables::ehframe(p, (size_t)r.size, r.va, fwd, r.par.size() == 2 ? &cm : nullptr, r.par.size() == 2 ? r.par[0] : 0,
                      r.par.size() == 2 && r.par[1] <= tables::CFA_LLVM ? (int)r.par[1] : tables::CFA_RANK);
      break;
    default: break;
  }
}

// R_X86_64_RELATIVE / IRELATIVE: the linker stores the addend in the
// relocated slot too.  The slot gets slot - addend (mostly zero), if it lies
// in plain data (outside every region).  Undone in reverse order, so even
// overlapping slots are restored exactly.
static bool plainData(const std::vector<Region>& R, uint64_t off, uint64_t len) {
  size_t lo = 0, hi = R.size();
  while (lo < hi) {  // first region ending after off
    size_t mid = (lo + hi) / 2;
    if (R[mid].off + R[mid].size <= off) lo = mid + 1; else hi = mid;
  }
  return lo == R.size() || R[lo].off >= off + len;
}

static bool plainRange(const std::vector<Region>& R, uint64_t off, uint64_t len, uint64_t n) {
  return off <= n && len <= n - off && plainData(R, off, len);
}

static void relaSlots(uint8_t* img, uint64_t n, const Region& r, const std::vector<Region>& R, bool fwd) {
  const uint8_t* t = img + r.off;
  size_t cnt = (size_t)(r.size / 24);
  for (size_t k = 0; k < cnt; k++) {
    size_t e = fwd ? k : cnt - 1 - k;
    uint64_t va = tables::g64(t + 24 * e), info = tables::g64(t + 24 * e + 8), add = tables::g64(t + 24 * e + 16);
    uint32_t type = (uint32_t)info;
    if (type != 8 && type != 37) continue;
    for (size_t m = 0; m + 3 <= r.par.size(); m += 3) {
      uint64_t sva = r.par[m], soff = r.par[m + 1], ssize = r.par[m + 2];
      if (va < sva || va - sva >= ssize || ssize - (va - sva) < 8) continue;
      uint64_t off = soff + (va - sva);
      if (n < 8 || off > n - 8 || !plainData(R, off, 8)) break;
      uint64_t v = tables::g64(img + off);
      tables::s64(img + off, fwd ? v - add : v + add);
      break;
    }
  }
}

// PE unwind data: each UNWIND_INFO referenced by the (untransformed)
// exception directory `pd`, if it lies in plain data, is transformed once,
// with the prolog of the first function using it.  Undone in reverse order.
static void unwindInfos(uint8_t* img, uint64_t n, const uint8_t* pd, const Region& r, const std::vector<Region>& R,
                        const tables::CodeMap& cm, bool fwd) {
  if (r.par.empty()) return;
  uint64_t vbias = r.par[0];
  struct U { uint64_t off; uint32_t begin; };
  std::vector<U> list;
  for (size_t i = 0; i + 12 <= r.size; i += 12) {
    uint32_t begin = tables::g32(pd + i), uw = tables::g32(pd + i + 8);
    for (size_t m = 1; m + 3 <= r.par.size(); m += 3) {
      uint64_t sva = r.par[m], soff = r.par[m + 1], ssize = r.par[m + 2];
      if (uw < sva || uw - sva >= ssize) continue;
      uint64_t off = soff + (uw - sva);
      if (ssize - (uw - sva) >= 4 && off + 4 <= n) list.push_back({off, begin});
      break;
    }
  }
  // unique structures, first use wins
  std::vector<size_t> idx(list.size());
  for (size_t k = 0; k < idx.size(); k++) idx[k] = k;
  std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return list[a].off < list[b].off; });
  std::vector<char> keep(list.size(), 0);
  for (size_t k = 0; k < idx.size(); k++)
    if (k == 0 || list[idx[k]].off != list[idx[k - 1]].off) keep[idx[k]] = 1;
  std::vector<tables::PEvent> ev;
  for (size_t j = 0; j < list.size(); j++) {
    size_t k = fwd ? j : list.size() - 1 - j;
    if (!keep[k]) continue;
    uint8_t* u = img + list[k].off;
    size_t len = 4 + 2 * (size_t)((u[2] + 1) & ~1u);
    if (list[k].off + len > n || !plainData(R, list[k].off, len)) continue;
    uint64_t avail = 0;
    const uint8_t* code = cm.at(vbias + list[k].begin, avail);
    cm.reset();  // per structure (at most 255 bytes of prolog): independent of the order
    tables::prologEvents(cm, code, code ? avail : 0, ev);
    tables::unwindInfo(u, len, ev, fwd);
  }
}

static inline bool hasVA(uint8_t type) { return type == R_CODE || type == R_EHFRAME || type == R_EHHDR || type == R_EXCEPT; }

//----------------------------------------------------------------------------
// Container

struct Stats {
  uint64_t ssize[NSTREAM];
  size_t nregion[R_NTYPES];
  size_t ndict, ntab;
  uint64_t ninsn, ncoded, tabbytes;
};

static void encode(const Buf& src, Buf& out, unsigned opts, Stats* st, std::vector<Buf>* dump) {
  std::vector<Region> R = analyze::run(src.data(), src.size(), true);
  Buf in = src;
  for (auto& r : R) if (r.type == R_RELA) relaSlots(in.data(), in.size(), r, R, true);
  tables::CodeMap cm = codeMap(R, src.data());
  for (auto& r : R) if (r.type == R_PDATA) unwindInfos(in.data(), in.size(), &src[(size_t)r.off], r, R, cm, true);
  // .eh_frame: the CFA prediction style that leaves the fewest nonzero bytes
  for (auto& r : R) {
    if (r.type != R_EHFRAME || r.par.size() != 1) continue;
    uint64_t best = UINT64_MAX;
    unsigned bs = 0;
    r.par.push_back(0);
    Region sample = r;  // the first 64 KB
    sample.size = std::min<uint64_t>(r.size, 1 << 16);
    for (unsigned style = tables::CFA_RANK; style <= tables::CFA_LLVM; style++) {
      Buf t(in.begin() + sample.off, in.begin() + sample.off + sample.size);
      sample.par[1] = style;
      tableTransform(t.data(), sample, true, R, src.data(), src.size(), cm);
      uint64_t nz = 0;
      for (uint8_t b : t) nz += b != 0;
      if (nz < best) { best = nz; bs = style; }
    }
    r.par[1] = bs;
  }
  // .gcc_except_table: only if every LSDA allows it
  for (auto& r : R) {
    if (r.type != R_EXCEPT || r.par.size() != 1) continue;
    bool ok = r.par[0] < R.size() && R[(size_t)r.par[0]].type == R_EHFRAME;
    if (ok) {
      const Region& e = R[(size_t)r.par[0]];
      std::vector<tables::LsdaRef> refs;
      tables::lsdaRefs(&src[(size_t)e.off], (size_t)e.size, e.va, r.va, r.size, refs);
      ok = !refs.empty() && tables::lsdaUsable(&src[(size_t)r.off], (size_t)r.size, refs);
    }
    r.par.push_back(ok);
  }
  for (auto& r : R) if (r.type != R_CODE) tableTransform(&in[(size_t)r.off], r, true, R, src.data(), src.size(), cm);

  OpDict dict;
  dict.dinl = opts & FL_DINL;
  buildDict(R, in.data(), kDictWeight[opts >> 16 & 3], dict);
  Labels B, L;
  for (auto& r : R) if (r.type == R_CODE) collectStarts(&in[(size_t)r.off], (size_t)r.size, r.va, dict, B);
  B.finish();
  for (auto& r : R) if (r.type == R_CODE) collectLabels(&in[(size_t)r.off], (size_t)r.size, r.va, B, L);
  L.finish();
  B.va.clear();
  B.va.shrink_to_fit();
  unsigned flags = (L.va.size() < 0x7FFFFFFF ? FL_LABELS : 0) | (opts & FL_OPTIONS);
  std::vector<uint32_t> ripTab;
  if (flags & FL_RIPLAB) {
    for (auto& r : R) if (r.type == R_CODE) collectRip(&in[(size_t)r.off], (size_t)r.size, r.va, ripTab);
    std::sort(ripTab.begin(), ripTab.end());
    ripTab.erase(std::unique(ripTab.begin(), ripTab.end()), ripTab.end());
  }

  Encoder E;
  E.L = &L;
  E.D = &dict;
  E.ripTab = &ripTab;
  E.flags = flags;
  uint64_t pos = 0;
  Buf& D = E.S[S_DATA];
  for (auto& r : R) {
    if (r.type != R_CODE) continue;
    D.insert(D.end(), in.begin() + pos, in.begin() + r.off);
    E.code(&in[(size_t)r.off], (size_t)r.size, r.va);
    pos = r.off + r.size;
  }
  D.insert(D.end(), in.begin() + pos, in.end());
  std::vector<tdelta::Table> dtab;
  if (flags & FL_TDELTA) tdelta::encode(D, dtab);

  out.clear();
  out.insert(out.end(), kMagic, kMagic + 4);
  out.push_back(kVersion);
  putVar(out, flags);
  putVar(out, in.size());
  putVar(out, R.size());
  uint64_t prev = 0, pva = 0;
  for (auto& r : R) {
    out.push_back(r.type);
    putVar(out, r.off - prev);
    putVar(out, r.size);
    if (hasVA(r.type)) { putVar(out, r.va - pva); pva = r.va; }
    putVar(out, r.par.size());
    for (uint64_t x : r.par) putVar(out, x);
    prev = r.off + r.size;
  }
  // opcode dictionary: code bitmap, then the skeletons in code order (sorted)
  putVar(out, dict.size());
  if (dict.size()) {
    for (int c = 0; c < 256; c += 8) {
      uint8_t m = 0;
      for (int j = 0; j < 8; j++) m |= (uint8_t)(dict.isCode[c + j] << j);
      out.push_back(m);
    }
    for (int c = 0; c < 256; c++)
      if (dict.isCode[c]) {
        out.push_back((uint8_t)dict.entry[c].size());
        out.insert(out.end(), dict.entry[c].begin(), dict.entry[c].end());
      }
  }
  if (flags & FL_RIPLAB) {  // RIP target table, delta coded
    putVar(out, ripTab.size());
    uint32_t prev = 0;
    for (uint32_t t : ripTab) { putVar(out, t - prev); prev = t; }
  }
  if (flags & FL_TDELTA) {  // data stream tables: gap to the previous one, type, rows
    putVar(out, dtab.size());
    uint64_t end = 0;
    for (auto& t : dtab) {
      putVar(out, t.off - end);
      putVar(out, t.type);
      putVar(out, t.rows);
      end = t.off + (uint64_t)tdelta::typeN(t.type) * t.rows;
    }
  }
  for (int k = 0; k < NSTREAM; k++) putVar(out, E.S[k].size());
  const uint8_t* order = kOrder[flags >> 7 & 3];
  for (int j = 0; j < NSTREAM; j++) {
    const Buf& s = E.S[order[j]];
    while (out.size() % kAlign) out.push_back(0);
    out.insert(out.end(), s.begin(), s.end());
  }
  if (st) {
    memset(st, 0, sizeof *st);
    for (int k = 0; k < NSTREAM; k++) st->ssize[k] = E.S[k].size();
    for (auto& r : R) st->nregion[r.type]++;
    st->ndict = dict.size();
    st->ninsn = E.ninsn;
    st->ncoded = E.ncoded;
    st->ntab = dtab.size();
    for (auto& t : dtab) st->tabbytes += (uint64_t)tdelta::typeN(t.type) * t.rows;
  }
  if (dump) for (int k = 0; k < NSTREAM; k++) dump->push_back(E.S[k]);
}

static bool decode(const Buf& in, Buf& out) {
  Reader r{in.data(), in.data() + in.size(), true};
  for (int k = 0; k < 4; k++) if (r.byte() != kMagic[k]) return false;
  if (r.byte() != kVersion) return false;
  uint64_t flags = r.var();
  if (!r.ok || flags & ~(uint64_t)(FL_LABELS | FL_OPTIONS)) return false;
  uint64_t n = r.var();
  uint64_t nr = r.var();
  if (!r.ok || nr > (uint64_t)(r.e - r.p) || n > SIZE_MAX - 64) return false;
  std::vector<Region> R((size_t)nr);
  uint64_t prev = 0, pva = 0;
  for (auto& x : R) {
    x.type = r.byte();
    uint64_t gap = r.var();
    x.size = r.var();
    x.va = 0;
    if (hasVA(x.type)) { x.va = pva + r.var(); pva = x.va; }
    uint64_t np = r.var();
    if (np > (uint64_t)(r.e - r.p)) return false;
    x.par.resize((size_t)np);
    for (uint64_t& v : x.par) v = r.var();
    if (!r.ok || x.type >= R_NTYPES || prev > n || gap > n - prev || x.size > n - prev - gap) return false;
    x.off = prev + gap;
    prev = x.off + x.size;
  }
  OpDict dict;
  dict.dinl = flags & FL_DINL;
  uint64_t nd = r.var();
  if (!r.ok || nd > 255) return false;
  if (nd) {
    uint8_t bm[32];
    for (int j = 0; j < 32; j++) bm[j] = r.byte();
    std::string last;
    for (int c = 0; c < 256; c++) {
      if (!(bm[c >> 3] >> (c & 7) & 1)) continue;
      size_t len = r.byte();
      if (!r.ok || c == kMark || len < 2 || len > kMaxSkel || len > (size_t)(r.e - r.p)) return false;
      std::string sk((const char*)r.p, len);
      r.p += len;
      if (dict.size() && sk <= last) return false;
      dict.add((uint8_t)c, sk);
      last = sk;
    }
    if (dict.size() != nd) return false;
  }
  std::vector<uint32_t> ripTab;
  if (flags & FL_RIPLAB) {
    uint64_t nt = r.var();
    if (!r.ok || nt > (uint64_t)(r.e - r.p)) return false;
    ripTab.resize((size_t)nt);
    uint64_t t = 0;
    for (size_t k = 0; k < ripTab.size(); k++) {
      uint64_t d = r.var();
      if (!r.ok || (k && !d) || d > 0xFFFFFFFFu - t) return false;
      t += d;
      ripTab[k] = (uint32_t)t;
    }
  }
  std::vector<tdelta::Table> dtab;
  if (flags & FL_TDELTA) {
    uint64_t nt = r.var();
    if (!r.ok || nt > (uint64_t)(r.e - r.p)) return false;
    dtab.resize((size_t)nt);
    uint64_t end = 0;
    for (auto& t : dtab) {
      uint64_t gap = r.var(), type = r.var(), rows = r.var();
      if (!r.ok || gap > n || type >> (tdelta::kMaxN + 1) || rows > n || rows > 0xFFFFFFFFu) return false;
      t.off = end + gap;
      t.type = (uint32_t)type;
      t.rows = (uint32_t)rows;
      int w = tdelta::typeN(t.type);
      if (w < 1 || t.off > n) return false;
      end = t.off + (uint64_t)w * rows;
    }
  }
  uint64_t sz[NSTREAM];
  for (int k = 0; k < NSTREAM; k++) sz[k] = r.var();
  if (!r.ok) return false;
  Buf pad[NSTREAM];
  Decoder Dc;
  const uint8_t* order = kOrder[flags >> 7 & 3];
  for (int j = 0; j < NSTREAM; j++) {
    int k = order[j];
    while (r.ok && (r.p - in.data()) % kAlign) r.byte();
    if (!r.ok || sz[k] > (uint64_t)(r.e - r.p)) return false;
    pad[k].assign((size_t)sz[k] + 64, 0);
    if (sz[k]) memcpy(pad[k].data(), r.p, (size_t)sz[k]);
    Dc.S[k].p = pad[k].data();
    Dc.S[k].e = pad[k].data() + sz[k];
    r.p += sz[k];
  }
  if (r.p != r.e) return false;
  if (!tdelta::decode(pad[S_DATA].data(), sz[S_DATA], dtab)) return false;

  // pass 1: labels
  Labels L;
  if (flags & FL_LABELS) {
    InStream op = Dc.S[S_OP];
    for (auto& x : R)
      if (x.type == R_CODE && !collectLabelsOp(op, dict, flags & FL_INLSEP, (size_t)x.size, x.va, L)) return false;
    L.finish();
  }
  Dc.L = &L;
  Dc.D = &dict;
  Dc.ripTab = &ripTab;
  Dc.flags = (unsigned)flags;

  // pass 2: rebuild the image
  out.assign((size_t)n, 0);
  uint64_t pos = 0;
  InStream& D = Dc.S[S_DATA];
  for (auto& x : R) {
    if (x.type != R_CODE) continue;
    size_t len = (size_t)(x.off - pos);
    const uint8_t* s = D.p;
    if (!D.take(len)) return false;
    if (len) memcpy(&out[(size_t)pos], s, len);
    if (!Dc.code(&out[(size_t)x.off], (size_t)x.size, x.va)) return false;
    pos = x.off + x.size;
  }
  size_t len = (size_t)(n - pos);
  const uint8_t* s = D.p;
  if (!D.take(len)) return false;
  if (len) memcpy(&out[(size_t)pos], s, len);
  for (int k = 0; k < NSTREAM; k++) if (Dc.S[k].p != Dc.S[k].e) return false;
  // inverse order of dependencies: tables (.eh_frame needs the code),
  // .eh_frame_hdr and .gcc_except_table (need .eh_frame), RELATIVE slots,
  // .gnu.hash (needs .dynsym/.dynstr in plain data)
  tables::CodeMap cm = codeMap(R, out.data());
  for (auto& x : R)
    if (x.type != R_CODE && x.type != R_EHHDR && x.type != R_GNUHASH && x.type != R_EXCEPT)
      tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  for (auto& x : R)
    if (x.type == R_EHHDR || x.type == R_EXCEPT) tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  for (size_t k = R.size(); k-- > 0;)
    if (R[k].type == R_PDATA) unwindInfos(out.data(), out.size(), &out[(size_t)R[k].off], R[k], R, cm, false);
  for (size_t k = R.size(); k-- > 0;)
    if (R[k].type == R_RELA) relaSlots(out.data(), out.size(), R[k], R, false);
  for (auto& x : R)
    if (x.type == R_GNUHASH) tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  return true;
}

//----------------------------------------------------------------------------
// Option search (-a): every combination of the coding options is encoded and
// compressed like xz -9e; the smallest wins.

// LZMA2 with xz -9e settings, the dictionary shrunk to the data (which saves
// memory and does not change the result)
static void lzmaOptions(size_t n, lzma_options_lzma& o) {
  lzma_lzma_preset(&o, 9 | LZMA_PRESET_EXTREME);
  uint32_t d = 1 << 12;
  while (d < n && d < o.dict_size) d <<= 1;
  o.dict_size = d;
}

// raw LZMA2 size of b
static uint64_t lzmaSize(const Buf& b) {
  lzma_options_lzma o;
  lzmaOptions(b.size(), o);
  lzma_filter f[2] = {{LZMA_FILTER_LZMA2, &o}, {LZMA_VLI_UNKNOWN, nullptr}};
  size_t cap = b.size() + b.size() / 16 + 4096, pos = 0;
  Buf out(cap);
  if (lzma_raw_buffer_encode(f, nullptr, b.data(), b.size(), out.data(), &pos, cap) != LZMA_OK) return UINT64_MAX;
  return pos;
}

static const char kOptLetter[] = "dierumtfpwW";
static const unsigned kOptFlag[] = {FL_DDELTA, FL_DINL, FL_D8DELTA, FL_RIPLAB, FL_VEXP, FL_INLSEP, FL_TDELTA, FL_ORD1, FL_ORD2, OPT_W1, OPT_W2};
enum { NOPT = 11 };

static std::string optString(unsigned opts) {
  std::string s;
  for (int k = 0; k < NOPT; k++) if (opts & kOptFlag[k]) s += kOptLetter[k];
  return s.empty() ? "-" : s;
}

// Option search.  -a: every option is toggled (all in parallel) and the best
// change kept, until no toggle helps; once from the given options and once
// from none, since the interactions leave local optima.  -aa: all
// combinations.  Each combination is encoded and compressed once.
struct OptionSearch {
  const Buf& in;
  bool verbose;
  unsigned threads;
  std::vector<std::pair<unsigned, uint64_t> > seen;

  OptionSearch(const Buf& b, bool v) : in(b), verbose(v) {
    // memory per job: LZMA encoder plus about three copies of the data
    lzma_options_lzma o;
    lzmaOptions(in.size() + in.size() / 4, o);
    lzma_filter f[2] = {{LZMA_FILTER_LZMA2, &o}, {LZMA_VLI_UNKNOWN, nullptr}};
    uint64_t job = lzma_raw_encoder_memusage(f) + 3 * (uint64_t)in.size() + (16 << 20);
    uint64_t budget = (uint64_t)3 << 30;
    threads = std::max(1u, std::min(std::thread::hardware_concurrency(), 8u));
    while (threads > 1 && threads * job > budget) threads--;
  }

  // sizes of the given option sets (memoized)
  void eval(std::vector<unsigned> sets) {
    std::vector<unsigned> todo;
    for (unsigned o : sets) {
      bool known = false;
      for (auto& x : seen) known |= x.first == o;
      for (unsigned t : todo) known |= t == o;
      if (!known) todo.push_back(o);
    }
    std::vector<uint64_t> res(todo.size());
    std::atomic<size_t> next(0);
    auto work = [&]() {
      for (size_t k; (k = next++) < todo.size();) {
        Buf tmp;
        encode(in, tmp, todo[k], nullptr, nullptr);
        res[k] = lzmaSize(tmp);
      }
    };
    std::vector<std::thread> pool;
    for (unsigned j = 1; j < threads && j < todo.size(); j++) pool.emplace_back(work);
    work();
    for (auto& t : pool) t.join();
    for (size_t k = 0; k < todo.size(); k++) {
      seen.push_back({todo[k], res[k]});
      if (verbose) fprintf(stderr, "  options %-10s %10llu\n", optString(todo[k]).c_str(), (unsigned long long)res[k]);
    }
  }
  uint64_t size(unsigned o) const {
    for (auto& x : seen) if (x.first == o) return x.second;
    return UINT64_MAX;
  }

  unsigned run(unsigned start, bool all) {
    if (all) {
      std::vector<unsigned> sets;
      for (unsigned m = 0; m < (1u << NOPT); m++) {
        unsigned o = 0;
        for (int k = 0; k < NOPT; k++) if (m >> k & 1) o |= kOptFlag[k];
        sets.push_back(o);
      }
      eval(sets);
    } else {
      for (unsigned cur : {start, 0u}) {
        eval({cur});
        for (;;) {
          std::vector<unsigned> sets;
          for (int k = 0; k < NOPT; k++) sets.push_back(cur ^ kOptFlag[k]);
          eval(sets);
          unsigned b = cur;
          for (unsigned o : sets) if (size(o) < size(b)) b = o;
          if (b == cur) break;
          cur = b;
        }
      }
    }
    unsigned best = start;
    for (auto& x : seen) if (x.second < size(best)) best = x.first;
    return best;
  }
};

//----------------------------------------------------------------------------

static const char* kStreamName[NSTREAM] = {"data", "op", "j8", "jmp", "jcc", "call", "rip", "disp32", "imm32", "imm64", "esc", "inl"};
static const char* kRegionName[R_NTYPES] = {"code", "pdata", "eh_frame_hdr", "rela", "reloc", "eh_frame", "gnu.hash", "except_table"};

static int usage() {
  fprintf(stderr,
          "x64pp - x86-64 executable preprocessor for xz/LZMA\n"
          "usage: x64pp c [-v] [-n] [-oLIST | -a | -aa] input output   forward transform\n"
          "       x64pp d input output                         inverse transform\n"
          "       x64pp s [-oLIST] input prefix                write each stream to prefix.<name>\n"
          "  -v  print statistics\n"
          "  -n  don't verify the forward transform by decoding it\n"
          "  -o  coding options, any of:\n"
          "        d  disp32 as delta from the previous one with the same base register\n"
          "        e  disp8 likewise (helps unrolled loops)\n"
          "        i  opcode dictionary entries include imm8/disp8/imm16\n"
          "        r  RIP-relative targets through a sorted table of targets\n"
          "        u  VEX prefix bits reordered (AVX code)\n"
          "        m  imm8/disp8/imm16 in their own stream instead of the opcode stream\n"
          "        t  tables of fixed-size records in data delta coded\n"
          "        f, p  stream order: data first (f), opcodes first (p), opcodes last (fp)\n"
          "        w, W  smaller opcode dictionary (w: more entries, W: fewer, wW: fewest)\n"

          "  -a  choose the options that compress best with xz -9e:\n"
          "      toggled one at a time while that helps; -aa tries all combinations\n"
          "  input/output may be - for stdin/stdout\n");
  return 1;
}

int main(int argc, char** argv) {
  if (argc < 4 || argv[1][1]) return usage();
  char mode = argv[1][0];
  bool verbose = false, verify = true;
  int search = 0;
  unsigned opts = kDefaultOptions;
  int a = 2;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-v")) verbose = true;
    else if (!strcmp(argv[a], "-n")) verify = false;
    else if (!strcmp(argv[a], "-a")) search = 1;
    else if (!strcmp(argv[a], "-aa")) search = 2;
    else if (argv[a][1] == 'o') {
      opts = 0;
      for (const char* o = argv[a] + 2; *o; o++) {
        const char* k = strchr(kOptLetter, *o);
        if (!k || !*k) return usage();
        opts |= kOptFlag[k - kOptLetter];
      }
    } else return usage();
  }
  if (argc - a != 2 || (mode != 'c' && mode != 'd' && mode != 's')) return usage();
  const char* iname = argv[a];
  const char* oname = argv[a + 1];
  Buf in, out;
  if (!readFile(iname, in)) { fprintf(stderr, "x64pp: can't read %s\n", iname); return 2; }
  if (mode == 'd') {
    if (!decode(in, out)) { fprintf(stderr, "x64pp: %s: not a valid x64pp stream\n", iname); return 4; }
    if (!writeFile(oname, out)) { fprintf(stderr, "x64pp: can't write %s\n", oname); return 3; }
    return 0;
  }
  Stats st;
  std::vector<Buf> dump;
  if (search && mode == 'c') {
    OptionSearch os(in, verbose);
    opts = os.run(opts, search == 2);
    if (verbose) fprintf(stderr, "  chosen: %s (%zu combinations, %u threads)\n", optString(opts).c_str(), os.seen.size(), os.threads);
  }
  encode(in, out, opts, &st, mode == 's' ? &dump : nullptr);
  if (verify) {
    Buf chk;
    if (!decode(out, chk) || chk != in) { fprintf(stderr, "x64pp: internal error: verification failed\n"); return 4; }
  }
  if (verbose) {
    fprintf(stderr, "%s: %zu -> %zu\n", iname, in.size(), out.size());
    for (int k = 0; k < R_NTYPES; k++)
      if (st.nregion[k]) fprintf(stderr, "  region %-12s x%zu\n", kRegionName[k], st.nregion[k]);
    fprintf(stderr, "  instructions %llu, %zu dictionary codes cover %.1f%%\n", (unsigned long long)st.ninsn, st.ndict,
            st.ninsn ? 100.0 * (double)st.ncoded / (double)st.ninsn : 0.0);
    if (st.ntab) fprintf(stderr, "  data tables %zu, %llu bytes\n", st.ntab, (unsigned long long)st.tabbytes);
    for (int k = 0; k < NSTREAM; k++)
      fprintf(stderr, "  stream %-7s %10llu\n", kStreamName[k], (unsigned long long)st.ssize[k]);
  }
  if (mode == 's') {
    for (int k = 0; k < NSTREAM; k++) {
      std::string nm = std::string(oname) + "." + kStreamName[k];
      if (!writeFile(nm.c_str(), dump[k])) { fprintf(stderr, "x64pp: can't write %s\n", nm.c_str()); return 3; }
    }
    return 0;
  }
  if (!writeFile(oname, out)) { fprintf(stderr, "x64pp: can't write %s\n", oname); return 3; }
  return 0;
}
