// x64pp - x86-64 executable preprocessor for LZMA/xz.
//
//   x64pp c input output    forward transform
//   x64pp d input output    inverse transform
//
// Code sections of PE32+ and ELF64 images are parsed into instructions.
// Opcode bytes (prefixes, opcode, ModRM, SIB) plus the small operands go to
// one stream; wide operands are split into separate streams by kind, with
// branch/call/RIP-relative targets converted to absolute form.  Jump targets
// are coded as instruction indices, which the decoder recovers by parsing
// the opcode stream first.  Unwind and relocation tables get delta coding.
// Everything else is copied.  See README.md for the format and the numbers.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include "x64dec.h"
#include "tables.h"
#include "analyze.h"

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
  NSTREAM
};

// order of streams in the output file
static const uint8_t kOrder[NSTREAM] = {S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_JCC, S_J8, S_OP, S_DATA};

enum { kAlign = 16, kVersion = 1 };
enum { FL_LABELS = 1 };
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

static inline bool inlineField(unsigned c) { return c == x64::F_D8 || c == x64::F_I8 || c == x64::F_I16; }

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
static void collectStarts(const uint8_t* p, size_t n, uint64_t va, Labels& B) {
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(p + i, n - i, I);
    if (p[i] != kMark) B.va.push_back(va + i);
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

struct Encoder {
  Buf S[NSTREAM];
  const Labels* L;
  unsigned flags;

  void code(const uint8_t* p, size_t n, uint64_t va) {
    size_t i = 0;
    Buf& op = S[S_OP];
    ImplicitLabels imp;
    bool lab = flags & FL_LABELS;
    while (i < n) {
      x64::Insn I;
      x64::decode(p + i, n - i, I);
      size_t r;
      if (lab && !imp.step(I) && L->find(va + i, r)) op.push_back(kMark);
      if (lab && p[i] == kMark) op.push_back(kMark);
      op.insert(op.end(), p + i, p + i + I.nstruct);
      const uint8_t* q = p + i + I.nstruct;
      uint64_t next = va + i + I.len;
      for (unsigned k = 0; k < I.nfield; k++) {
        unsigned c = I.fclass[k], sz = I.fsize[k];
        uint64_t v = fieldValue(q, sz), t;
        q += sz;
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16:
            putLE(op, v, sz);
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
          case x64::F_RIP: putBE32(S[S_RIP], (uint32_t)(v + next)); break;
          case x64::F_D32: case x64::F_DABS: putBE32(S[S_DISP], (uint32_t)v); break;
          case x64::F_I32: putBE32(S[S_IMM32], (uint32_t)v); break;
          default: putLE(S[S_IMM64], v, sz); break;  // F_I64
        }
      }
      i += I.len;
    }
  }
};

// decoder: consume an optional label marker / escape at an instruction start
static inline bool readMark(InStream& op) {
  if (op.p < op.e && op.p[0] == kMark) {
    bool escape = op.p + 1 < op.e && op.p[1] == kMark;
    op.p++;
    return !escape;
  }
  return false;
}

// decoding pass 1: labels from the opcode stream alone
static bool collectLabelsOp(InStream& op, size_t n, uint64_t va, Labels& L) {
  ImplicitLabels imp;
  size_t i = 0;
  while (i < n) {
    bool marked = readMark(op);
    x64::Insn I;
    x64::decode(op.p, n - i, I);
    size_t adv = I.nstruct;
    for (unsigned k = 0; k < I.nfield; k++) if (inlineField(I.fclass[k])) adv += I.fsize[k];
    if (!op.take(adv)) return false;
    if (imp.step(I) || marked) L.va.push_back(va + i);
    i += I.len;
  }
  return true;
}

struct Decoder {
  InStream S[NSTREAM];
  const Labels* L;
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
    bool lab = flags & FL_LABELS;
    while (i < n) {
      if (lab) readMark(op);
      x64::Insn I;
      x64::decode(op.p, n - i, I);
      const uint8_t* s = op.p;
      if (!op.take(I.nstruct)) return false;
      memcpy(out + i, s, I.nstruct);
      uint8_t* q = out + i + I.nstruct;
      uint64_t next = va + i + I.len;
      for (unsigned k = 0; k < I.nfield; k++) {
        unsigned c = I.fclass[k], sz = I.fsize[k];
        uint64_t v = 0;
        InStream* st;
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16:
            s = op.p;
            if (!op.take(sz)) return false;
            v = fieldValue(s, sz);
            break;
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
            s = S[S_RIP].p;
            if (!S[S_RIP].take(4)) return false;
            v = (uint32_t)(getBE32(s) - next);
            break;
          case x64::F_D32: case x64::F_DABS: case x64::F_I32:
            st = &S[c == x64::F_I32 ? S_IMM32 : S_DISP];
            s = st->p;
            if (!st->take(4)) return false;
            v = getBE32(s);
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
    case R_EHFRAME:
      tables::ehframe(p, (size_t)r.size, r.va, fwd, r.par.size() == 1 ? &cm : nullptr, r.par.size() == 1 ? r.par[0] : 0);
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

static inline bool hasVA(uint8_t type) { return type == R_CODE || type == R_EHFRAME || type == R_EHHDR; }

//----------------------------------------------------------------------------
// Container

struct Stats {
  uint64_t ssize[NSTREAM];
  size_t nregion[R_NTYPES];
};

static void encode(const Buf& src, Buf& out, Stats* st, std::vector<Buf>* dump) {
  std::vector<Region> R = analyze::run(src.data(), src.size(), true);
  Buf in = src;
  for (auto& r : R) if (r.type == R_RELA) relaSlots(in.data(), in.size(), r, R, true);
  tables::CodeMap cm = codeMap(R, src.data());
  for (auto& r : R) if (r.type == R_PDATA) unwindInfos(in.data(), in.size(), &src[(size_t)r.off], r, R, cm, true);
  for (auto& r : R) if (r.type != R_CODE) tableTransform(&in[(size_t)r.off], r, true, R, src.data(), src.size(), cm);

  Labels B, L;
  for (auto& r : R) if (r.type == R_CODE) collectStarts(&in[(size_t)r.off], (size_t)r.size, r.va, B);
  B.finish();
  for (auto& r : R) if (r.type == R_CODE) collectLabels(&in[(size_t)r.off], (size_t)r.size, r.va, B, L);
  L.finish();
  B.va.clear();
  B.va.shrink_to_fit();
  unsigned flags = L.va.size() < 0x7FFFFFFF ? FL_LABELS : 0;

  Encoder E;
  E.L = &L;
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

  out.clear();
  out.insert(out.end(), kMagic, kMagic + 4);
  out.push_back(kVersion);
  out.push_back((uint8_t)flags);
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
  for (int k = 0; k < NSTREAM; k++) putVar(out, E.S[k].size());
  for (int j = 0; j < NSTREAM; j++) {
    const Buf& s = E.S[kOrder[j]];
    while (out.size() % kAlign) out.push_back(0);
    out.insert(out.end(), s.begin(), s.end());
  }
  if (st) {
    memset(st, 0, sizeof *st);
    for (int k = 0; k < NSTREAM; k++) st->ssize[k] = E.S[k].size();
    for (auto& r : R) st->nregion[r.type]++;
  }
  if (dump) for (int k = 0; k < NSTREAM; k++) dump->push_back(E.S[k]);
}

static bool decode(const Buf& in, Buf& out) {
  Reader r{in.data(), in.data() + in.size(), true};
  for (int k = 0; k < 4; k++) if (r.byte() != kMagic[k]) return false;
  if (r.byte() != kVersion) return false;
  unsigned flags = r.byte();
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
  uint64_t sz[NSTREAM];
  for (int k = 0; k < NSTREAM; k++) sz[k] = r.var();
  if (!r.ok) return false;
  Buf pad[NSTREAM];
  Decoder Dc;
  for (int j = 0; j < NSTREAM; j++) {
    int k = kOrder[j];
    while ((r.p - in.data()) % kAlign) r.byte();
    if (!r.ok || sz[k] > (uint64_t)(r.e - r.p)) return false;
    pad[k].assign((size_t)sz[k] + 64, 0);
    if (sz[k]) memcpy(pad[k].data(), r.p, (size_t)sz[k]);
    Dc.S[k].p = pad[k].data();
    Dc.S[k].e = pad[k].data() + sz[k];
    r.p += sz[k];
  }
  if (r.p != r.e) return false;

  // pass 1: labels
  Labels L;
  if (flags & FL_LABELS) {
    InStream op = Dc.S[S_OP];
    for (auto& x : R)
      if (x.type == R_CODE && !collectLabelsOp(op, (size_t)x.size, x.va, L)) return false;
    L.finish();
  }
  Dc.L = &L;
  Dc.flags = flags;

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
  // .eh_frame_hdr (needs .eh_frame), RELATIVE slots, .gnu.hash (needs
  // .dynsym/.dynstr in plain data)
  tables::CodeMap cm = codeMap(R, out.data());
  for (auto& x : R)
    if (x.type != R_CODE && x.type != R_EHHDR && x.type != R_GNUHASH)
      tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  for (auto& x : R)
    if (x.type == R_EHHDR) tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  for (size_t k = R.size(); k-- > 0;)
    if (R[k].type == R_PDATA) unwindInfos(out.data(), out.size(), &out[(size_t)R[k].off], R[k], R, cm, false);
  for (size_t k = R.size(); k-- > 0;)
    if (R[k].type == R_RELA) relaSlots(out.data(), out.size(), R[k], R, false);
  for (auto& x : R)
    if (x.type == R_GNUHASH) tableTransform(&out[(size_t)x.off], x, false, R, out.data(), n, cm);
  return true;
}

//----------------------------------------------------------------------------

static const char* kStreamName[NSTREAM] = {"data", "op", "j8", "jmp", "jcc", "call", "rip", "disp32", "imm32", "imm64", "esc"};
static const char* kRegionName[R_NTYPES] = {"code", "pdata", "eh_frame_hdr", "rela", "reloc", "eh_frame", "gnu.hash"};

static int usage() {
  fprintf(stderr,
          "x64pp - x86-64 executable preprocessor for xz/LZMA\n"
          "usage: x64pp c [-v] [-n] input output   forward transform\n"
          "       x64pp d input output             inverse transform\n"
          "       x64pp s input prefix             write each stream to prefix.<name>\n"
          "  -v  print statistics\n"
          "  -n  don't verify the forward transform by decoding it\n"
          "  input/output may be - for stdin/stdout\n");
  return 1;
}

int main(int argc, char** argv) {
  if (argc < 4 || argv[1][1]) return usage();
  char mode = argv[1][0];
  bool verbose = false, verify = true;
  int a = 2;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-v")) verbose = true;
    else if (!strcmp(argv[a], "-n")) verify = false;
    else return usage();
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
  encode(in, out, &st, mode == 's' ? &dump : nullptr);
  if (verify) {
    Buf chk;
    if (!decode(out, chk) || chk != in) { fprintf(stderr, "x64pp: internal error: verification failed\n"); return 4; }
  }
  if (verbose) {
    fprintf(stderr, "%s: %zu -> %zu\n", iname, in.size(), out.size());
    for (int k = 0; k < R_NTYPES; k++)
      if (st.nregion[k]) fprintf(stderr, "  region %-12s x%zu\n", kRegionName[k], st.nregion[k]);
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
