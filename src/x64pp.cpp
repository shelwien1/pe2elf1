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
  S_J8,     // rel8 branch targets: rank among nearby instruction starts
  S_JMP,    // rel32 jmp/jcc targets: instruction index + 1, 0 = escape
  S_CALL,   // rel32 call targets: absolute address
  S_RIP,    // RIP-relative disp32: absolute address
  S_DISP,   // disp32
  S_IMM32,  // imm32
  S_IMM64,  // imm64, moffs64
  S_ESC,    // rel32 jmp/jcc targets that are not instruction starts
  NSTREAM
};

// order of streams in the output file
static const uint8_t kOrder[NSTREAM] = {S_DATA, S_ESC, S_IMM64, S_IMM32, S_DISP, S_RIP, S_CALL, S_JMP, S_OP, S_J8};

enum { kAlign = 16, kVersion = 1 };
enum { FL_JMPIDX = 1, FL_J8RANK = 2 };
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
// Instruction starts and branch target coding

struct Bounds {
  std::vector<uint64_t> va;  // sorted, unique
  size_t lower(uint64_t t) const { return std::lower_bound(va.begin(), va.end(), t) - va.begin(); }
  void finish() {
    std::sort(va.begin(), va.end());
    va.erase(std::unique(va.begin(), va.end()), va.end());
  }
};

static inline bool inlineField(unsigned c) { return c == x64::F_D8 || c == x64::F_I8 || c == x64::F_I16; }

static inline uint32_t zigzag(int64_t d) { return (uint32_t)((uint64_t)d << 1 ^ (uint64_t)(d >> 63)); }

// rel8 targets: the 256 addresses next-128..next+127 are ranked, instruction
// starts first (ordered by zigzag of the instruction distance from `next`),
// then the other addresses in increasing order.  The code is the rank.
struct J8Win {
  size_t b0, b1, rn;  // starts in the window are [b0,b1); rn = first start >= next
  uint64_t lo;
  bool ok;
  J8Win(const Bounds& B, uint64_t next) {
    ok = next >= 128 && next <= UINT64_MAX - 128;
    b0 = b1 = rn = 0;
    lo = next - 128;
    if (!ok) return;
    b0 = B.lower(lo);
    b1 = B.lower(next + 128);
    rn = B.lower(next);
  }
};

static uint8_t j8Encode(const Bounds& B, uint64_t next, uint8_t rel) {
  J8Win w(B, next);
  if (!w.ok) return rel;
  uint64_t t = next + (uint64_t)(int64_t)(int8_t)rel;
  size_t r = B.lower(t);
  size_t nb = w.b1 - w.b0;
  if (r < w.b1 && B.va[r] == t) {
    // count window starts with a smaller key
    int64_t d = (int64_t)r - (int64_t)w.rn;
    int64_t dmin = (int64_t)w.b0 - (int64_t)w.rn, dmax = (int64_t)w.b1 - 1 - (int64_t)w.rn;
    uint32_t z = zigzag(d);
    if (z == 0) return 0;
    int64_t hi = (int64_t)((z - 1) / 2), lo = -(int64_t)(z / 2);
    int64_t a = std::max(dmin, lo), b = std::min(dmax, hi);
    return (uint8_t)(b >= a ? b - a + 1 : 0);
  }
  return (uint8_t)(nb + (t - w.lo) - (r - w.b0));
}

static uint8_t j8Decode(const Bounds& B, uint64_t next, uint8_t code) {
  J8Win w(B, next);
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
    t = B.va[(size_t)((int64_t)w.rn + d)];
  } else {
    t = w.lo + (code - nb);
    for (size_t k = w.b0; k < w.b1 && B.va[k] <= t; k++) t++;
  }
  return (uint8_t)(t - next);
}

//----------------------------------------------------------------------------
// Code regions

static void collectBounds(const uint8_t* p, size_t n, uint64_t va, Bounds& B) {
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(p + i, n - i, I);
    B.va.push_back(va + i);
    i += I.len;
  }
}

struct Encoder {
  Buf S[NSTREAM];
  const Bounds* B;
  unsigned flags;

  void code(const uint8_t* p, size_t n, uint64_t va) {
    size_t i = 0;
    Buf& op = S[S_OP];
    while (i < n) {
      x64::Insn I;
      x64::decode(p + i, n - i, I);
      op.insert(op.end(), p + i, p + i + I.nstruct);
      const uint8_t* q = p + i + I.nstruct;
      uint64_t next = va + i + I.len;
      for (unsigned k = 0; k < I.nfield; k++) {
        unsigned c = I.fclass[k], sz = I.fsize[k];
        uint64_t v = 0;
        for (unsigned j = 0; j < sz; j++) v |= (uint64_t)q[j] << (8 * j);
        q += sz;
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16:
            putLE(op, v, sz);
            break;
          case x64::F_J8:
            S[S_J8].push_back((flags & FL_J8RANK) ? j8Encode(*B, next, (uint8_t)v) : (uint8_t)v);
            break;
          case x64::F_JMP: case x64::F_JCC:
            if (flags & FL_JMPIDX) {
              uint64_t t = next + (uint64_t)(int64_t)(int32_t)v;
              size_t r = B->lower(t);
              if (r < B->va.size() && B->va[r] == t) putBE32(S[S_JMP], (uint32_t)(r + 1));
              else { putBE32(S[S_JMP], 0); putBE32(S[S_ESC], (uint32_t)t); }
            } else putBE32(S[S_JMP], (uint32_t)(v + next));
            break;
          case x64::F_CALL: putBE32(S[S_CALL], (uint32_t)(v + next)); break;
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

// decoding pass 1: instruction starts from the opcode stream alone
static bool collectBoundsOp(InStream& op, size_t n, uint64_t va, Bounds& B) {
  size_t i = 0;
  while (i < n) {
    x64::Insn I;
    x64::decode(op.p, n - i, I);
    size_t adv = I.nstruct;
    for (unsigned k = 0; k < I.nfield; k++) if (inlineField(I.fclass[k])) adv += I.fsize[k];
    if (!op.take(adv)) return false;
    B.va.push_back(va + i);
    i += I.len;
  }
  return true;
}

struct Decoder {
  InStream S[NSTREAM];
  const Bounds* B;
  unsigned flags;

  bool code(uint8_t* out, size_t n, uint64_t va) {
    size_t i = 0;
    InStream& op = S[S_OP];
    while (i < n) {
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
        switch (c) {
          case x64::F_D8: case x64::F_I8: case x64::F_I16:
            s = op.p;
            if (!op.take(sz)) return false;
            for (unsigned j = 0; j < sz; j++) v |= (uint64_t)s[j] << (8 * j);
            break;
          case x64::F_J8:
            s = S[S_J8].p;
            if (!S[S_J8].take(1)) return false;
            v = (flags & FL_J8RANK) ? j8Decode(*B, next, s[0]) : s[0];
            break;
          case x64::F_JMP: case x64::F_JCC: {
            s = S[S_JMP].p;
            if (!S[S_JMP].take(4)) return false;
            uint32_t y = getBE32(s);
            if (!(flags & FL_JMPIDX)) v = (uint32_t)(y - next);
            else if (y == 0) {
              s = S[S_ESC].p;
              if (!S[S_ESC].take(4)) return false;
              v = (uint32_t)(getBE32(s) - next);
            } else {
              if (y - 1 >= B->va.size()) return false;
              v = (uint32_t)(B->va[y - 1] - next);
            }
            break;
          }
          case x64::F_CALL: case x64::F_RIP: {
            InStream& st = S[c == x64::F_CALL ? S_CALL : S_RIP];
            s = st.p;
            if (!st.take(4)) return false;
            v = (uint32_t)(getBE32(s) - next);
            break;
          }
          case x64::F_D32: case x64::F_DABS: case x64::F_I32: {
            InStream& st = S[c == x64::F_I32 ? S_IMM32 : S_DISP];
            s = st.p;
            if (!st.take(4)) return false;
            v = getBE32(s);
            break;
          }
          default:  // F_I64
            s = S[S_IMM64].p;
            if (!S[S_IMM64].take(sz)) return false;
            for (unsigned j = 0; j < sz; j++) v |= (uint64_t)s[j] << (8 * j);
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

static void tableTransform(uint8_t* p, const Region& r, bool fwd) {
  switch (r.type) {
    case R_PDATA: tables::pdata(p, (size_t)r.size, fwd); break;
    case R_EHHDR: tables::ehhdr(p, (size_t)r.size, fwd); break;
    case R_RELA: tables::rela(p, (size_t)r.size, fwd); break;
    case R_RELOC: tables::reloc(p, (size_t)r.size, fwd); break;
    case R_EHFRAME: tables::ehframe(p, (size_t)r.size, r.va, fwd); break;
    default: break;
  }
}

static inline bool hasVA(uint8_t type) { return type == R_CODE || type == R_EHFRAME; }

//----------------------------------------------------------------------------
// Container

struct Stats {
  uint64_t ssize[NSTREAM];
  size_t nregion[R_NTYPES];
};

static void encode(const Buf& src, Buf& out, Stats* st, std::vector<Buf>* dump) {
  std::vector<Region> R = analyze::run(src.data(), src.size(), true);
  Buf in = src;
  for (auto& r : R) if (r.type != R_CODE) tableTransform(&in[(size_t)r.off], r, true);

  Bounds B;
  for (auto& r : R) if (r.type == R_CODE) collectBounds(&in[(size_t)r.off], (size_t)r.size, r.va, B);
  B.finish();
  unsigned flags = FL_J8RANK;
  if (B.va.size() < 0xFFFFFFFFu) flags |= FL_JMPIDX;

  Encoder E;
  E.B = &B;
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

  // pass 1: instruction starts
  Bounds B;
  InStream op = Dc.S[S_OP];
  for (auto& x : R)
    if (x.type == R_CODE && !collectBoundsOp(op, (size_t)x.size, x.va, B)) return false;
  B.finish();
  Dc.B = &B;
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
  for (auto& x : R) if (x.type != R_CODE) tableTransform(&out[(size_t)x.off], x, false);
  return true;
}

//----------------------------------------------------------------------------

static const char* kStreamName[NSTREAM] = {"data", "op", "j8", "jmp", "call", "rip", "disp32", "imm32", "imm64", "esc"};
static const char* kRegionName[R_NTYPES] = {"code", "pdata", "eh_frame_hdr", "rela", "reloc", "eh_frame"};

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
