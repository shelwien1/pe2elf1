// tables.h - in-place, bijective transforms of structured tables found in
// x86-64 executables (unwind tables, relocations).  Every transform maps a
// byte range to a byte range of the same size.  The inverse never consults
// file headers: it only needs the region type, size and virtual address that
// the container stores, and it parses the region in the same order as the
// forward transform, so every decision is reproduced exactly.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <utility>
#include <algorithm>
#include "x64dec.h"

namespace tables {

static inline uint32_t g32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static inline void s32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static inline uint64_t g64(const uint8_t* p) { return g32(p) | (uint64_t)g32(p + 4) << 32; }
static inline void s64(uint8_t* p, uint64_t v) { s32(p, (uint32_t)v); s32(p + 4, (uint32_t)(v >> 32)); }
static inline uint16_t g16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
static inline void s16(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

//----------------------------------------------------------------------------
// .eh_frame_hdr search table: pairs { initial_location, fde_address } (both
// relative to the start of .eh_frame_hdr), sorted by location.  Linkers build
// it from .eh_frame, so it is predicted from the FDE list (`pred`, same
// order) and only the differences are stored.  Without a prediction of the
// right length both columns are delta coded instead.
typedef std::vector<std::pair<uint32_t, uint32_t> > HdrPred;

static void ehhdr(uint8_t* p, size_t n, bool fwd, const HdrPred& pred) {
  bool usePred = pred.size() == n / 8;
  uint32_t pl = 0, pf = 0;
  for (size_t i = 0, k = 0; i + 8 <= n; i += 8, k++) {
    uint32_t l = g32(p + i), f = g32(p + i + 4);
    if (usePred) { pl = pred[k].first; pf = pred[k].second; }
    if (fwd) { s32(p + i, l - pl); s32(p + i + 4, f - pf); }
    else { l += pl; f += pf; s32(p + i, l); s32(p + i + 4, f); }
    if (!usePred) { pl = l; pf = f; }
  }
}

//----------------------------------------------------------------------------
// ELF Elf64_Rela { r_offset, r_info, r_addend }: offset delta coded,
// addend stored big-endian
static void rela(uint8_t* p, size_t n, bool fwd) {
  uint64_t po = 0;
  for (size_t i = 0; i + 24 <= n; i += 24) {
    uint64_t o = g64(p + i), a = g64(p + i + 16);
    if (fwd) { s64(p + i, o - po); po = o; }
    else { o += po; s64(p + i, o); po = o; }
    // byte swap of the addend is its own inverse
    for (int k = 0; k < 8; k++) p[i + 16 + k] = (uint8_t)(a >> (8 * (7 - k)));
  }
}

//----------------------------------------------------------------------------
// PE base relocations: blocks { page_rva, block_size, uint16 entries[] }
// page_rva delta coded against the previous block, entries delta coded
// within a block.  block_size stays as is (it drives the parse).
static void reloc(uint8_t* p, size_t n, bool fwd) {
  size_t i = 0;
  uint32_t ppage = 0;
  while (i + 8 <= n) {
    uint32_t page = g32(p + i), bs = g32(p + i + 4);
    if (bs < 8 || bs > n - i) break;
    if (fwd) { s32(p + i, page - ppage); ppage = page; }
    else { page += ppage; s32(p + i, page); ppage = page; }
    uint16_t pv = 0;
    for (size_t j = i + 8; j + 2 <= i + bs; j += 2) {
      uint16_t v = g16(p + j);
      if (fwd) { s16(p + j, (uint16_t)(v - pv)); pv = v; }
      else { v = (uint16_t)(v + pv); s16(p + j, v); pv = v; }
    }
    i += bs;
  }
}

//----------------------------------------------------------------------------
// .eh_frame: CIE/FDE records
//  FDE CIE pointer  -> offset of the CIE (+1), bijective on nonzero values
//  FDE pc_begin     -> start minus end of the previous FDE (absolute)
//  FDE LSDA pointer -> absolute, delta to the previous LSDA
// Only 4-byte pointer encodings (udata4/sdata4, absolute or pcrel) are
// transformed; everything else is left as is.

struct Cie { uint32_t off; uint8_t renc, lenc, hasz, caf1, daf8; };

static inline bool uleb(const uint8_t* p, size_t n, size_t& i, uint64_t& v) {
  v = 0;
  for (int s = 0; s < 64; s += 7) {
    if (i >= n) return false;
    uint8_t c = p[i++];
    v |= (uint64_t)(c & 0x7F) << s;
    if (!(c & 0x80)) return true;
  }
  return false;
}

static inline int encSize(uint8_t enc) {
  switch (enc & 0x0F) {
    case 0x00: return 8;  // absptr
    case 0x02: case 0x0A: return 2;
    case 0x03: case 0x0B: return 4;
    case 0x04: case 0x0C: return 8;
    default: return -1;
  }
}
static inline bool enc4(uint8_t enc) {
  return ((enc & 0x0F) == 0x03 || (enc & 0x0F) == 0x0B) && ((enc & 0x70) == 0x00 || (enc & 0x70) == 0x10);
}

static bool parseCie(const uint8_t* p, size_t end, size_t i, Cie& c) {
  // i points to the version byte
  if (i >= end) return false;
  uint8_t ver = p[i++];
  size_t a = i;
  while (i < end && p[i]) i++;
  if (i >= end) return false;
  const uint8_t* aug = p + a;
  size_t augn = i - a;
  i++;
  if (ver >= 4) i += 2;
  uint64_t v;
  if (!uleb(p, end, i, v)) return false;
  c.caf1 = v == 1;  // code alignment factor 1: advances are in bytes
  if (!uleb(p, end, i, v)) return false;
  c.daf8 = v == 0x78;  // data alignment factor -8 (sleb): offsets in 8-byte units
  if (ver == 1) i++;
  else if (!uleb(p, end, i, v)) return false;
  c.renc = 0; c.lenc = 0xFF; c.hasz = 0;
  if (augn && aug[0] == 'z') {
    c.hasz = 1;
    if (!uleb(p, end, i, v)) return false;
    for (size_t k = 1; k < augn; k++) {
      if (i > end) return false;
      uint8_t ch = aug[k];
      if (ch == 'R') { if (i >= end) return false; c.renc = p[i++]; }
      else if (ch == 'L') { if (i >= end) return false; c.lenc = p[i++]; }
      else if (ch == 'P') {
        if (i >= end) return false;
        int sz = encSize(p[i++]);
        if (sz < 0) return false;
        i += (size_t)sz;
      } else if (ch == 'S' || ch == 'B') {
      } else break;
    }
  }
  return true;
}


//----------------------------------------------------------------------------
// Code access for unwind tables: the decoded image and its code regions

struct CodeSpan { uint64_t va, off, size; };

struct CodeMap {
  const uint8_t* img = nullptr;
  std::vector<CodeSpan> spans;  // sorted by va
  // Work limit for the code scans of one table, so crafted tables can't make
  // them quadratic.  Reset at the start of every table transform (in the
  // same places on both sides); when it runs out, predictions stop.
  uint64_t codeBytes = 0;
  mutable uint64_t budget = 0;
  void reset() const { budget = 64 * codeBytes + (16u << 20); }
  // decode the instruction at c (avail bytes); false when out of budget
  bool step(const uint8_t* c, uint64_t avail, x64::Insn& I) const {
    if (!budget) return false;
    x64::decode(c, (size_t)std::min<uint64_t>(avail, 64), I);
    budget = budget > I.len ? budget - I.len : 0;
    return true;
  }
  // pointer to the code at va and the number of code bytes from there
  const uint8_t* at(uint64_t va, uint64_t& avail) const {
    size_t lo = 0, hi = spans.size();
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (spans[mid].va + spans[mid].size <= va) lo = mid + 1; else hi = mid;
    }
    if (lo == spans.size() || va < spans[lo].va) return nullptr;
    avail = spans[lo].size - (va - spans[lo].va);
    return img + spans[lo].off + (va - spans[lo].va);
  }
};

// instructions that change the CFA rule: they write rsp, set up rbp from
// rsp, or return
static inline bool stackEvent(const uint8_t* p, const x64::Insn& I) {
  if (I.trunc || I.enc != x64::ENC_LEGACY) return false;
  unsigned op = I.op;
  if (op >= 0x50 && op <= 0x5F) return true;
  switch (op) {
    case 0x68: case 0x6A: case 0x8F: case 0x9C: case 0x9D: case 0xC2: case 0xC3: case 0xC9: case 0xCA: case 0xCB:
      return true;
    default: break;
  }
  if (!I.hasmodrm) return false;
  unsigned m = I.modrm, mod = m >> 6, reg = (m >> 3) & 7, rm = m & 7;
  unsigned rex = I.prex != x64::NOPOS ? p[I.prex] : 0;
  if (op == 0xFF && reg == 6) return true;  // push r/m
  if (mod == 3 && rm == 4 && !(rex & 1)) {  // rsp as r/m destination
    if (op == 0x81 || op == 0x83) return reg != 7;  // not cmp
    if (op == 0x01 || op == 0x09 || op == 0x21 || op == 0x29 || op == 0x31 || op == 0x89 || op == 0xC7) return true;
  }
  if (reg == 4 && !(rex & 4) && (op == 0x03 || op == 0x0B || op == 0x23 || op == 0x2B || op == 0x33 || op == 0x8B || op == 0x8D))
    return true;  // rsp as reg destination
  if (op == 0x89 && m == 0xE5 && !(rex & 5)) return true;  // mov rbp, rsp
  if (op == 0x8B && m == 0xEC && !(rex & 5)) return true;  // mov rbp, rsp
  if (op == 0x8D && reg == 5 && !(rex & 4)) return true;   // lea rbp, [...]
  return false;
}

// instruction boundaries of a function, with "stack event ends here" flags
struct FuncCode {
  std::vector<uint64_t> b;   // boundary offsets from the function start, ascending
  std::vector<uint32_t> ev;  // ev[k] = number of event boundaries among b[0..k-1]
  void build(const CodeMap& cm, const uint8_t* code, uint64_t avail, uint64_t len) {
    b.clear(); ev.clear();
    if (!code) return;
    if (len > avail) len = avail;
    uint64_t i = 0;
    uint32_t ne = 0;
    b.push_back(0); ev.push_back(0);
    while (i < len) {
      x64::Insn I;
      if (!cm.step(code + i, avail - i, I)) break;
      bool e = stackEvent(code + i, I);
      i += I.len;
      ev.push_back(ne);
      if (e) ne++;
      b.push_back(i);
    }
    ev.push_back(ne);  // ev has b.size() + 1 entries
  }
  bool isEvent(size_t k) const { return ev[k + 1] > ev[k]; }

};

// Location advance from loc by d (0..maxd): the targets are ranked event
// ends first, then other instruction boundaries, then other addresses.
static uint64_t advRank(const FuncCode& F, uint64_t loc, uint64_t maxd, uint64_t d) {
  uint64_t hi = loc + maxd, t = loc + d;
  size_t k0 = std::lower_bound(F.b.begin(), F.b.end(), loc) - F.b.begin();
  size_t k1 = std::upper_bound(F.b.begin(), F.b.end(), hi) - F.b.begin();
  size_t kt = std::lower_bound(F.b.begin(), F.b.end(), t) - F.b.begin();
  uint64_t nE = F.ev[k1] - F.ev[k0], nB = k1 - k0;
  if (kt < k1 && F.b[kt] == t) {
    uint64_t eBelow = F.ev[kt] - F.ev[k0];
    if (F.isEvent(kt)) return eBelow;
    return nE + (kt - k0 - eBelow);
  }
  return nB + (t - loc) - (kt - k0);
}

static uint64_t advUnrank(const FuncCode& F, uint64_t loc, uint64_t maxd, uint64_t c) {
  uint64_t hi = loc + maxd;
  size_t k0 = std::lower_bound(F.b.begin(), F.b.end(), loc) - F.b.begin();
  size_t k1 = std::upper_bound(F.b.begin(), F.b.end(), hi) - F.b.begin();
  uint64_t nE = F.ev[k1] - F.ev[k0], nB = k1 - k0;
  if (c < nB) {
    bool wantE = c < nE;
    uint64_t want = wantE ? c : c - nE;  // index among events / non-events in the window
    // smallest k in [k0,k1) of the wanted kind with `want` of that kind before it
    size_t lo = k0, h = k1;
    while (lo < h) {
      size_t mid = (lo + h) / 2;
      uint64_t before = wantE ? F.ev[mid + 1] - F.ev[k0] : (mid + 1 - k0) - (F.ev[mid + 1] - F.ev[k0]);
      if (before <= want) lo = mid + 1; else h = mid;
    }
    return F.b[lo] - loc;
  }
  // the (c - nB)-th address in the window that is not a boundary: skip the m
  // boundaries with at most that many non-boundaries below them
  uint64_t want = c - nB;
  size_t lo = k0, h = k1;
  while (lo < h) {
    size_t mid = (lo + h) / 2;
    if ((F.b[mid] - loc) - (mid - k0) <= want) lo = mid + 1; else h = mid;
  }
  return want + (lo - k0);
}

// CFA program prediction.  A predictor walks the function's code alongside
// the CFA program and proposes each next CFA instruction: an advance to the
// end of the next instruction that moves the stack or the frame pointer,
// then the rule changes and register saves GCC (style 1) or LLVM (style 2)
// emit there.  Each actual instruction, its advance operand ranked
// (advRank), is XORed with the proposal, ranked the same way; the predictor
// then follows the actual instruction, so a wrong guess only costs locally.
// Style 0 proposes nothing: advances are just ranked.
enum { CFA_RANK = 0, CFA_GCC = 1, CFA_LLVM = 2 };

static const uint8_t kDwarfGpr[16] = {0, 2, 1, 3, 7, 6, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15};

struct CfaInsn {
  uint8_t b[16];
  uint8_t n = 0;
  void put(uint8_t x) { if (n < sizeof b) b[n++] = x; }
  void uleb(uint64_t v) {
    do { uint8_t x = v & 0x7F; v >>= 7; put(v ? x | 0x80 : x); } while (v);
  }
  bool operator==(const CfaInsn& o) const { return n == o.n && !memcmp(b, o.b, n); }
};

static CfaInsn cfaOp(uint8_t op) { CfaInsn c; c.put(op); return c; }
static CfaInsn cfaOp(uint8_t op, uint64_t a) { CfaInsn c; c.put(op); c.uleb(a); return c; }
static CfaInsn cfaOp(uint8_t op, uint64_t a, uint64_t b) { CfaInsn c; c.put(op); c.uleb(a); c.uleb(b); return c; }
static CfaInsn cfaOffset(unsigned reg, uint64_t k) {
  if (reg < 64) { CfaInsn c; c.put((uint8_t)(0x80 | reg)); c.uleb(k); return c; }
  return cfaOp(0x05, reg, k);
}
static CfaInsn cfaAdvance(uint64_t d) {
  CfaInsn c;
  if (d < 64) c.put((uint8_t)(0x40 | d));
  else if (d < 0x100) { c.put(0x02); c.put((uint8_t)d); }
  else if (d < 0x10000) { c.put(0x03); c.put((uint8_t)d); c.put((uint8_t)(d >> 8)); }
  else { c.put(0x04); for (int k = 0; k < 4; k++) c.put((uint8_t)(d >> (8 * k))); }
  return c;
}

// length of the CFA instruction at p (n bytes there); 0 if it does not fit.
// Unknown opcodes count as one byte.
static size_t cfaInsnLen(const uint8_t* p, size_t n) {
  if (!n) return 0;
  uint8_t op = p[0];
  size_t i = 1;
  uint64_t v;
  switch (op >> 6) {
    case 1: case 3: return 1;
    case 2: return uleb(p, n, i, v) ? i : 0;
    default: break;
  }
  switch (op) {
    case 0x02: return n >= 2 ? 2 : 0;
    case 0x03: return n >= 3 ? 3 : 0;
    case 0x04: return n >= 5 ? 5 : 0;
    case 0x05: case 0x09: case 0x0C: case 0x11: case 0x12: case 0x14: case 0x15: case 0x2F:
      return uleb(p, n, i, v) && uleb(p, n, i, v) ? i : 0;
    case 0x06: case 0x07: case 0x08: case 0x0D: case 0x0E: case 0x13: case 0x2E:
      return uleb(p, n, i, v) ? i : 0;
    case 0x0F:
      if (!uleb(p, n, i, v) || v > n - i) return 0;
      return i + (size_t)v;
    case 0x10: case 0x16:
      if (!uleb(p, n, i, v) || !uleb(p, n, i, v) || v > n - i) return 0;
      return i + (size_t)v;
    default: return 1;
  }
}

// advance operand of a complete CFA instruction: size and limit
static inline bool cfaAdvanceField(const uint8_t* p, size_t len, size_t& at, size_t& sz, uint64_t& maxd) {
  uint8_t op = p[0];
  if (op >> 6 == 1) { at = 0; sz = 0; maxd = 63; return len == 1; }
  if (op == 0x02) { at = 1; sz = 1; maxd = 0xFF; return len == 2; }
  if (op == 0x03) { at = 1; sz = 2; maxd = 0xFFFF; return len == 3; }
  if (op == 0x04) { at = 1; sz = 4; maxd = 0xFFFFFFFF; return len == 5; }
  return false;
}
static inline uint64_t cfaGetAdv(const uint8_t* p, size_t at, size_t sz) {
  if (!sz) return p[0] & 0x3F;
  uint64_t d = 0;
  for (size_t k = 0; k < sz; k++) d |= (uint64_t)p[at + k] << (8 * k);
  return d;
}
static inline void cfaSetAdv(uint8_t* p, size_t at, size_t sz, uint64_t d) {
  if (!sz) { p[0] = (uint8_t)(0x40 | d); return; }
  for (size_t k = 0; k < sz; k++) p[at + k] = (uint8_t)(d >> (8 * k));
}

// frame state after a prologue: inherited by a split-off cold part
struct CfaFrame {
  bool valid = false;
  unsigned cfaReg = 7;
  int64_t sp = 8, bp = -1;
  std::vector<std::pair<unsigned, int64_t> > saved;
};

struct CfaPredictor {
  const uint8_t* code = nullptr;
  const FuncCode* F = nullptr;
  int style = CFA_RANK;
  size_t k = 0;  // next instruction (index into F->b) to simulate
  struct Op { uint64_t pos; CfaInsn c; };
  std::vector<Op> ops;  // proposals not yet seen, by position
  size_t opi = 0;
  // machine state after the simulated instructions
  int64_t sp = 8, bp = -1;  // CFA - rsp (< 0: unknown); CFA - rbp while rbp is the frame pointer
  unsigned cfaReg = 7;
  int phase = 0;            // 0 prologue, 1 body, 2 epilogue
  bool epiChanged = false, epiMore = false;
  int64_t bodySp = 8, bodyBp = -1;
  unsigned bodyCfaReg = 7;
  std::vector<std::pair<unsigned, int64_t> > queued;  // saves waiting for the end of the prologue
  std::vector<std::pair<unsigned, int64_t> > saved;   // all saves, in push order
  bool shrink = false;  // prologue not at the entry (shrink-wrapped): epilogues restore registers
  uint64_t lastProEnd = 0;  // end of the last prologue instruction
  uint64_t proStart = 0;    // start of the (shrink-wrapped) prologue
  bool restoredAtLeave = false;
  bool dead = false;
  static bool calleeSaved(unsigned dr) { return dr == 3 || dr == 6 || dr >= 12; }

  CfaFrame body0;  // state at the end of the prologue
  void init(const uint8_t* c, const FuncCode& f, int st) { code = c; F = &f; style = st; }
  // start inside the frame of the previous function (GCC's .cold parts):
  // its CFA rule and saves, the saves in register order
  void inherit(const CfaFrame& fr) {
    phase = 1;
    cfaReg = fr.cfaReg; sp = fr.sp; bp = fr.bp; saved = fr.saved;
    if (cfaReg == 6) emit(0, cfaOp(0x0C, 6, (uint64_t)bp));
    else if (sp != 8) emit(0, cfaOp(0x0E, (uint64_t)sp));
    std::vector<std::pair<unsigned, int64_t> > regs = saved;
    std::sort(regs.begin(), regs.end());
    for (auto& r : regs) emit(0, cfaOffset(r.first, (uint64_t)r.second / 8));
  }
  // first instruction is part of a prologue (or there is none)
  bool startsWithPrologue() const {
    if (F->b.size() < 2) return true;
    int64_t x;
    Eff e = effect(0, x);
    return e == E_PUSH || e == E_ENDBR || e == E_FPSET || (e == E_SPADD && x > 0);
  }
  uint64_t end() const { return F->b.empty() ? 0 : F->b.back(); }

  enum Eff { E_NONE, E_PAD, E_FLOW, E_PUSH, E_POP, E_SPADD, E_FPSET, E_SPFROMBP, E_LEAVE, E_RET, E_JMPOUT, E_ENDBR,
             E_SPALIGN, E_RBPWRITE, E_JMPIND, E_UNKNOWN };
  // effect of instruction idx, cached; x: register (push/pop), sp change
  // (E_SPADD) or rbp displacement (E_SPFROMBP, E_FPSET)
  mutable std::vector<std::pair<int8_t, int64_t> > effCache;
  Eff effect(size_t idx, int64_t& x) const {
    if (effCache.empty()) effCache.assign(F->b.size(), std::make_pair((int8_t)-1, (int64_t)0));
    if (effCache[idx].first < 0) {
      Eff e = decodeEffect(idx, effCache[idx].second);
      effCache[idx].first = (int8_t)e;
    }
    x = effCache[idx].second;
    return (Eff)effCache[idx].first;
  }
  Eff decodeEffect(size_t idx, int64_t& x) const {
    uint64_t at = F->b[idx], nx = F->b[idx + 1];
    const uint8_t* q = code + at;
    x64::Insn I;
    x64::decode(q, (size_t)(nx - at), I);
    x = 0;
    if (I.trunc) return E_UNKNOWN;
    if (I.enc != x64::ENC_LEGACY) return E_NONE;
    unsigned op = I.op, rex = I.prex != x64::NOPOS ? q[I.prex] : 0;
    unsigned m = I.modrm, mod = m >> 6, reg = (m >> 3) & 7, rm = m & 7;
    auto imm = [&]() -> int64_t {
      const uint8_t* f = q + I.nstruct;
      int64_t v = 0;
      for (unsigned j = 0; j < I.nfield; j++) {
        unsigned sz = I.fsize[j];
        if (I.fclass[j] == x64::F_I8) v = (int8_t)f[0];
        else if (I.fclass[j] == x64::F_I32) { int32_t t; memcpy(&t, f, 4); v = t; }
        else if (I.fclass[j] == x64::F_I16) { int16_t t; memcpy(&t, f, 2); v = t; }
        f += sz;
      }
      return v;
    };
    auto disp = [&]() -> int64_t {
      const uint8_t* f = q + I.nstruct;
      for (unsigned j = 0; j < I.nfield; j++) {
        if (I.fclass[j] == x64::F_D8) return (int8_t)f[0];
        if (I.fclass[j] == x64::F_D32) { int32_t t; memcpy(&t, f, 4); return t; }
        f += I.fsize[j];
      }
      return 0;
    };
    if (op == 0x90 || op == 0x11F || op == 0xCC) return E_PAD;
    if (op >= 0x50 && op <= 0x57) { x = (op & 7) | (rex & 1) << 3; return E_PUSH; }
    if (op >= 0x58 && op <= 0x5F) { x = (op & 7) | (rex & 1) << 3; return E_POP; }
    switch (op) {
      case 0x68: case 0x6A: case 0x9C: x = 8; return E_SPADD;
      case 0x9D: x = -8; return E_SPADD;
      case 0x8F: if (I.hasmodrm && reg == 0) { x = -8; return E_SPADD; } break;
      case 0xC9: return E_LEAVE;
      case 0xC2: case 0xC3: return E_RET;
      case 0xE9: case 0xEB: {
        int64_t t = (int64_t)nx + imm();
        const uint8_t* f = q + I.nstruct;
        int64_t rel = op == 0xEB ? (int8_t)f[0] : 0;
        if (op == 0xE9) { int32_t r; memcpy(&r, f, 4); rel = r; }
        t = (int64_t)nx + rel;
        return t < 0 || (uint64_t)t >= end() ? E_JMPOUT : E_FLOW;
      }
      default: break;
    }
    if (op == 0x11E && I.nstruct == 4 && q[0] == 0xF3 && m == 0xFA) return E_ENDBR;
    for (unsigned j = 0; j < I.nfield; j++) {
      unsigned c = I.fclass[j];
      if (c == x64::F_J8 || c == x64::F_JCC || c == x64::F_CALL || c == x64::F_JMP) return E_FLOW;
    }
    if (!I.hasmodrm) return E_NONE;
    if (op == 0xFF && reg == 6) { x = 8; return E_SPADD; }
    if (op == 0xFF && reg == 4) return E_JMPIND;  // switch, or a tail call at the end of an epilogue
    if (op == 0xFF && (reg == 2 || reg == 3 || reg == 5)) return E_FLOW;
    bool w = rex & 8;
    if (mod == 3 && rm == 4 && !(rex & 1)) {  // rsp as r/m destination
      if ((op == 0x81 || op == 0x83) && w && reg == 5) { x = imm(); return E_SPADD; }   // sub rsp, imm
      if ((op == 0x81 || op == 0x83) && w && reg == 0) { x = -imm(); return E_SPADD; }  // add rsp, imm
      if ((op == 0x81 || op == 0x83) && reg == 7) return E_NONE;                        // cmp
      if ((op == 0x81 || op == 0x83) && w && reg == 4) return E_SPALIGN;                // and rsp, imm
      if (op == 0x89 && reg == 5 && !(rex & 4) && w) return E_SPFROMBP;                 // mov rsp, rbp
    }
    if (op == 0x89 && m == 0xE5 && !(rex & 5) && w) return E_FPSET;  // mov rbp, rsp
    if (op == 0x8B && m == 0xEC && !(rex & 5) && w) return E_FPSET;
    if (op == 0x8B && m == 0xE5 && !(rex & 5) && w) return E_SPFROMBP;  // mov rsp, rbp
    if (op == 0x8D && reg == 4 && !(rex & 4) && mod != 3) {  // lea rsp, [...]
      if (rm == 4 && I.psib != x64::NOPOS && q[I.psib] == 0x24 && !(rex & 3)) { x = -disp(); return E_SPADD; }
      if (rm == 5 && mod != 0 && !(rex & 1)) { x = disp(); return E_SPFROMBP; }
      return E_UNKNOWN;
    }
    if (op == 0x8D && reg == 5 && !(rex & 4)) {  // lea rbp: frame pointer at rsp + d, or rbp as a plain register
      if (mod != 3 && rm == 4 && I.psib != x64::NOPOS && q[I.psib] == 0x24 && !(rex & 3) && w) { x = disp(); return E_FPSET; }
      return E_RBPWRITE;
    }
    if (stackEvent(q, I)) return E_UNKNOWN;  // writes rsp some other way
    return E_NONE;
  }

  void emit(uint64_t pos, const CfaInsn& c) { ops.push_back({pos, c}); }
  void flushQueued(uint64_t pos) {
    if (style == CFA_GCC) for (auto& s : queued) emit(pos, cfaOffset(s.first, (uint64_t)s.second / 8));
    else for (size_t j = queued.size(); j-- > 0;) emit(pos, cfaOffset(queued[j].first, (uint64_t)queued[j].second / 8));
    queued.clear();
  }
  bool isPrologue(Eff e) const { return e == E_PUSH || e == E_FPSET || e == E_ENDBR || (e == E_SPADD); }
  // more prologue instructions (push, sub rsp, frame setup) follow at idx
  // before any branch?  Compilers interleave argument moves with them.
  bool prologueAhead(size_t idx) const {
    for (size_t j = idx; j + 1 < F->b.size() && j < idx + 8; j++) {
      int64_t x;
      Eff e = effect(j, x);
      if (e == E_PUSH || e == E_FPSET || (e == E_SPADD && x > 0)) return true;
      if (e != E_NONE && e != E_RBPWRITE) return false;
    }
    return false;
  }
  // does an epilogue start at instruction idx: only stack restoring
  // instructions up to a return or tail jump?  more = code follows it
  bool epilogueAt(size_t idx, bool& more) {
    for (size_t j = idx; j + 1 < F->b.size() && j < idx + 24; j++) {
      int64_t x;
      Eff e = effect(j, x);
      if (e == E_RET || e == E_JMPOUT || e == E_JMPIND) {
        uint64_t nb = padEnd(j);
        more = nb < end() && !entryBlock(nb);
        return true;
      }
      if (e == E_POP || e == E_LEAVE || e == E_SPFROMBP || (e == E_SPADD && x < 0) || e == E_NONE || e == E_RBPWRITE) continue;
      return false;
    }
    return false;
  }
  void cfaChange(uint64_t pos, const CfaInsn& c) {
    if (phase == 2 && !epiChanged) {
      epiChanged = true;
      if (style == CFA_GCC && epiMore) emit(pos, cfaOp(0x0A));  // remember_state
    }
    emit(pos, c);
  }
  // jumps inside the function: (target, source), sorted
  std::vector<std::pair<uint64_t, uint64_t> > jumps;
  bool jumpsBuilt = false;
  void buildJumps() {
    jumpsBuilt = true;
    for (size_t j = 0; j + 1 < F->b.size(); j++) {
      uint64_t at = F->b[j], nx = F->b[j + 1];
      x64::Insn I;
      x64::decode(code + at, (size_t)(nx - at), I);
      const uint8_t* f = code + at + I.nstruct;
      for (unsigned t = 0; t < I.nfield; t++) {
        unsigned c = I.fclass[t];
        int64_t rel = 0;
        bool br = true;
        if (c == x64::F_J8) rel = (int8_t)f[0];
        else if (c == x64::F_JMP || c == x64::F_JCC) { int32_t r; memcpy(&r, f, 4); rel = r; }
        else br = false;
        if (br) {
          int64_t tg = (int64_t)nx + rel;
          if (tg >= 0 && (uint64_t)tg < end()) jumps.push_back({(uint64_t)tg, at});
        }
        f += I.fsize[t];
      }
    }
    std::sort(jumps.begin(), jumps.end());
  }
  // is the block at pos reached only from before the (shrink-wrapped) prologue?
  bool entryBlock(uint64_t pos) {
    if (!shrink) return false;
    if (!jumpsBuilt) buildJumps();
    auto it = std::lower_bound(jumps.begin(), jumps.end(), std::make_pair(pos, (uint64_t)0));
    bool any = false;
    for (; it != jumps.end() && it->first == pos; ++it) {
      if (it->second >= proStart) return false;
      any = true;
    }
    return any;
  }
  uint64_t padEnd(size_t idx) const {  // end of the padding after instruction idx
    uint64_t nx = F->b[idx + 1];
    for (size_t j = idx + 1; j + 1 < F->b.size(); j++) {
      int64_t y;
      if (effect(j, y) != E_PAD) break;
      nx = F->b[j + 1];
    }
    return nx;
  }
  // simulate one instruction; false at the end
  bool step() {
    if (dead || style == CFA_RANK || k + 1 >= F->b.size()) return false;
    size_t idx = k++;
    uint64_t at = F->b[idx], nx = F->b[idx + 1];
    int64_t x;
    Eff e = effect(idx, x);
    if (e == E_UNKNOWN) { dead = true; return false; }
    if (phase == 1 && e == E_PUSH && calleeSaved(kDwarfGpr[x & 15])) {  // shrink-wrapped prologue
      phase = 0;
      shrink = true;
      proStart = at;
    }
    if (phase == 0 && !isPrologue(e)) {
      // GCC writes queued saves before any other instruction, LLVM after
      // the whole prologue
      if (style == CFA_GCC) flushQueued(at);
      if (!(e == E_NONE && prologueAhead(idx + 1))) {
        if (style != CFA_GCC) flushQueued(lastProEnd);
        phase = 1;
        if (!body0.valid && sp >= 0 && (cfaReg == 6 || sp != 8)) {
          body0.valid = true;
          body0.cfaReg = cfaReg; body0.sp = sp; body0.bp = bp; body0.saved = saved;
        }
      }
    }
    if (phase == 0 && isPrologue(e)) lastProEnd = nx;
    if (phase == 1) {
      bool more;
      if ((e == E_POP || e == E_LEAVE || e == E_SPFROMBP || (e == E_SPADD && x < 0)) && epilogueAt(idx, more)) {
        phase = 2;
        epiChanged = false;
        epiMore = more;
        restoredAtLeave = false;
        bodySp = sp; bodyBp = bp; bodyCfaReg = cfaReg;
      }
    }
    switch (e) {
      case E_PUSH: {
        if (sp < 0) break;
        sp += 8;
        unsigned dr = kDwarfGpr[x & 15];
        if (cfaReg == 7) cfaChange(nx, cfaOp(0x0E, (uint64_t)sp));
        bool calleeSaved = dr == 3 || dr == 6 || dr >= 12;  // else a stack adjustment (push rax)
        if (phase == 0 && calleeSaved) {
          saved.push_back({dr, sp});
          if (style == CFA_GCC && cfaReg == 7) emit(nx, cfaOffset(dr, (uint64_t)sp / 8));
          else if (style == CFA_LLVM && dr == 6 && idx + 2 < F->b.size() && [&] { int64_t y; return effect(idx + 1, y) == E_FPSET; }())
            emit(nx, cfaOffset(6, (uint64_t)sp / 8));
          else queued.push_back({dr, sp});
        }
        break;
      }
      case E_SPADD:
        if (sp < 0) break;
        sp += x;
        if (cfaReg == 7) cfaChange(nx, cfaOp(0x0E, (uint64_t)sp));
        break;
      case E_SPALIGN:
        if (cfaReg == 7) { dead = true; return false; }
        sp = -1;
        break;
      case E_RBPWRITE:
        if (bp >= 0 && cfaReg == 6) { dead = true; return false; }
        bp = -1;
        break;
      case E_POP:
        if (sp < 0) { dead = true; return false; }
        sp -= 8;
        if ((x & 15) == 5 && cfaReg == 6) {  // pop rbp: CFA back on rsp
          bp = -1;
          cfaReg = 7;
          cfaChange(nx, cfaOp(0x0C, 7, (uint64_t)sp));
        } else {
          if ((x & 15) == 5) bp = -1;
          if (cfaReg == 7) cfaChange(nx, cfaOp(0x0E, (uint64_t)sp));
        }
        break;
      case E_FPSET:
        if (sp < 0) { dead = true; return false; }
        bp = sp - x;
        cfaReg = 6;
        if (x) emit(nx, cfaOp(0x0C, 6, (uint64_t)bp));
        else emit(nx, cfaOp(0x0D, 6));
        break;
      case E_SPFROMBP:
        if (bp < 0) { dead = true; return false; }
        sp = bp - x;
        if (cfaReg == 7) cfaChange(nx, cfaOp(0x0E, (uint64_t)sp));
        break;
      case E_LEAVE:
        if (bp < 0) { dead = true; return false; }
        sp = bp - 8;
        bp = -1;
        cfaReg = 7;
        if (style == CFA_GCC && phase == 2 && shrink && !epiMore && !restoredAtLeave) {
          for (auto& sv : saved) emit(nx, cfaOp((uint8_t)(0xC0 | sv.first)));
          restoredAtLeave = true;
        }
        cfaChange(nx, cfaOp(0x0C, 7, (uint64_t)sp));
        break;
      case E_JMPIND:
        if (phase != 2) break;
        // fall through
      case E_RET: case E_JMPOUT:
        if (phase == 2) {
          // the next block starts after the alignment padding
          nx = padEnd(idx);
          if (style == CFA_GCC && nx < end() && entryBlock(nx) && !restoredAtLeave) {
            // reached only from before the prologue: back to the entry rule
            std::vector<unsigned> regs;
            for (auto& sv : saved) regs.push_back(sv.first);
            std::sort(regs.begin(), regs.end());
            for (unsigned r : regs) emit(nx, cfaOp((uint8_t)(0xC0 | r)));
            sp = 8; bp = -1; cfaReg = 7;
            phase = 1;
            break;
          }
          if (epiChanged && nx < end()) {
            if (style == CFA_GCC) emit(nx, cfaOp(0x0B));  // restore_state
            else if (bodyCfaReg == 7) emit(nx, cfaOp(0x0E, (uint64_t)bodySp));
            else emit(nx, cfaOp(0x0C, 6, (uint64_t)bodyBp));
          }
          sp = bodySp; bp = bodyBp; cfaReg = bodyCfaReg;
          phase = 1;
        }
        break;
      default: break;
    }
    return true;
  }
  // the next instruction expected at location loc
  CfaInsn predict(uint64_t loc) {
    for (;;) {
      while (opi < ops.size() && ops[opi].pos < loc) opi++;
      if (opi < ops.size()) break;
      ops.clear();
      opi = 0;
      if (!step()) return cfaOp(0x00);
    }
    if (ops[opi].pos > loc) return cfaAdvance(ops[opi].pos - loc);
    return ops[opi].c;
  }
  // follow the actual (complete) instruction a at loc; loc moves on advances
  void observe(const uint8_t* a, size_t len, uint64_t& loc) {
    size_t at, sz;
    uint64_t maxd;
    if (cfaAdvanceField(a, len, at, sz, maxd)) { loc += cfaGetAdv(a, at, sz); return; }
    if (a[0] == 0x00 || style == CFA_RANK) return;
    CfaInsn c;
    for (size_t j = 0; j < len && j < sizeof c.b; j++) c.put(a[j]);
    for (size_t j = opi; j < ops.size() && ops[j].pos == loc; j++)
      if (ops[j].c == c) { ops.erase(ops.begin() + (long)j); return; }
    // unexpected: follow its rule change
    size_t i = 1;
    uint64_t r, o;
    if (a[0] == 0x0E && uleb(a, len, i, o)) { if (cfaReg == 7) sp = (int64_t)o; }
    else if (a[0] == 0x0C && uleb(a, len, i, r) && uleb(a, len, i, o)) {
      cfaReg = (unsigned)r;
      if (r == 7) sp = (int64_t)o;
      else if (r == 6) bp = (int64_t)o;
    } else if (a[0] == 0x0D && uleb(a, len, i, r)) {
      cfaReg = (unsigned)r;
      if (r == 6) bp = sp;
    }
  }
};

// ranked form of an instruction (advance operand replaced by its rank)
static void cfaRanked(const FuncCode& F, uint64_t loc, uint8_t* b, size_t len) {
  size_t at, sz;
  uint64_t maxd;
  if (!cfaAdvanceField(b, len, at, sz, maxd)) return;
  cfaSetAdv(b, at, sz, advRank(F, loc, maxd, cfaGetAdv(b, at, sz)));
}

// Transform a CFA program in place: each instruction becomes its ranked form
// XOR the ranked proposal (zero padded).
// frame: in, the previous function's frame (for an unaligned start without a
// prologue); out, this function's
static void cfaProgram(uint8_t* p, size_t i, size_t end, const FuncCode& F, const uint8_t* code, int style, bool fwd,
                       CfaFrame& frame, bool unaligned) {
  CfaPredictor pr;
  pr.init(code, F, style);
  if (style == CFA_GCC && unaligned && frame.valid && !pr.startsWithPrologue()) pr.inherit(frame);
  uint64_t loc = 0;
  uint8_t a[64];
  while (i < end) {
    CfaInsn pc = pr.predict(loc);
    size_t pl = cfaInsnLen(pc.b, pc.n);
    cfaRanked(F, loc, pc.b, pl);
    size_t len;
    if (fwd) {
      len = cfaInsnLen(p + i, end - i);
      if (!len) len = end - i;
      size_t take = std::min(len, sizeof a);
      memcpy(a, p + i, take);
      if (len <= sizeof a) cfaRanked(F, loc, p + i, len);
      for (size_t t = 0; t < len && t < pc.n; t++) p[i + t] ^= pc.b[t];
      if (len <= sizeof a) pr.observe(a, len, loc);
    } else {
      len = 0;
      size_t t = 0;
      while (!len && i + t < end) {
        if (t < pc.n) p[i + t] ^= pc.b[t];
        t++;
        len = cfaInsnLen(p + i, t);
      }
      if (!len) len = end - i;
      if (len <= sizeof a) {
        size_t at, sz;
        uint64_t maxd;
        if (cfaAdvanceField(p + i, len, at, sz, maxd)) cfaSetAdv(p + i, at, sz, advUnrank(F, loc, maxd, cfaGetAdv(p + i, at, sz)));
        memcpy(a, p + i, len);
        pr.observe(a, len, loc);
      }
    }
    i += len;
  }
  frame = pr.body0;
}

static inline bool isPad(const x64::Insn& I) {
  return !I.trunc && (I.op == 0x90 || I.op == 0xCC || I.op == 0x11F);
}

// bytes of padding (nop / int3) at va
static uint32_t padAt(const CodeMap* cm, uint64_t va) {
  uint64_t avail = 0;
  const uint8_t* c = cm ? cm->at(va, avail) : nullptr;
  uint32_t n = 0;
  while (c && n < avail && n < 4096) {
    x64::Insn I;
    if (!cm->step(c + n, avail - n, I) || !isPad(I)) break;
    n += I.len;
  }
  return n;
}

static inline bool isEndbr64(const uint8_t* c, uint64_t avail) {
  return avail >= 4 && c[0] == 0xF3 && c[1] == 0x0F && c[2] == 0x1E && c[3] == 0xFA;
}

// predicted function length: up to the first ret / jmp / call / ud2 / hlt
// followed by padding or by the end of the code region.  If the function
// starts with endbr64, the next one must too: padding inside the function
// (aligned jump targets) is skipped.
static uint32_t funcLenAt(const CodeMap* cm, uint64_t va) {
  uint64_t avail = 0;
  const uint8_t* c = cm ? cm->at(va, avail) : nullptr;
  if (!c) return 0;
  bool cet = isEndbr64(c, avail);
  uint64_t lim = std::min<uint64_t>(avail, 1 << 16), i = 0;
  bool endish = false;
  while (i < lim) {
    if (endish && cet && isEndbr64(c + i, avail - i)) return (uint32_t)i;
    x64::Insn I;
    if (!cm->step(c + i, avail - i, I)) return 0;
    if (endish && isPad(I)) {
      if (!cet) return (uint32_t)i;
      uint64_t j = i;
      while (j < lim) {  // skip the padding run
        x64::Insn J;
        if (!cm->step(c + j, avail - j, J)) return 0;
        if (!isPad(J)) break;
        j += J.len;
      }
      if (isEndbr64(c + j, avail - j)) return (uint32_t)i;
      i = j;
      endish = false;
      continue;
    }
    endish = false;
    if (!I.trunc) {
      unsigned op = I.op, reg = (I.modrm >> 3) & 7;
      endish = op == 0xC3 || op == 0xC2 || op == 0xE9 || op == 0xEB || op == 0xE8 || op == 0xF4 || op == 0x10B ||
               (op == 0xFF && I.hasmodrm && reg >= 2 && reg <= 5);
    }
    i += I.len;
  }
  return endish && i == avail ? (uint32_t)i : 0;
}

//----------------------------------------------------------------------------
// PE exception directory: RUNTIME_FUNCTION { begin, end, unwind } (RVAs)
//   begin  -> begin - previous end - padding found there
//   end    -> end - begin - predicted function length
//   unwind -> delta to the previous unwind RVA
// (cm, vbias: the image's code, RVA + vbias is its virtual address)
static void pdata(uint8_t* p, size_t n, bool fwd, const CodeMap* cm, uint64_t vbias) {
  uint32_t pe = 0, pu = 0;
  if (cm) cm->reset();
  for (size_t i = 0; i + 12 <= n; i += 12) {
    uint32_t a = g32(p + i), b = g32(p + i + 4), c = g32(p + i + 8);
    uint32_t gap = cm && pe ? padAt(cm, vbias + pe) : 0;
    if (fwd) {
      uint32_t len = cm ? funcLenAt(cm, vbias + a) : 0;
      s32(p + i, a - pe - gap); s32(p + i + 4, b - a - len); s32(p + i + 8, c - pu);
    } else {
      a += pe + gap;
      uint32_t len = cm ? funcLenAt(cm, vbias + a) : 0;
      b += a + len; c += pu;
      s32(p + i, a); s32(p + i + 4, b); s32(p + i + 8, c);
    }
    pe = b; pu = c;
  }
}

// PE UNWIND_INFO: the unwind codes are generated from the prolog the way
// the MSVC toolchain does it, and XORed with the actual codes.
//  - push r            UWOP_PUSH_NONVOL
//  - sub rsp, n        UWOP_ALLOC_SMALL / UWOP_ALLOC_LARGE (also mov eax, n;
//                      call __chkstk; sub rsp, rax)
//  - mov [rsp+d], r    UWOP_SAVE_NONVOL for nonvolatile r, offset relative to
//                      the final rsp; saves made before the last allocation
//                      are listed first, at the prolog end, latest first
//  - movaps [rsp+d], x UWOP_SAVE_XMM128 likewise
//  - lea r, [rsp+d]    UWOP_SET_FPREG, frame register/offset in the header
// Codes are listed from the last instruction back.

struct PEvent { uint8_t end, kind, reg; uint32_t val; uint64_t sp; };  // kind 1 push 2 alloc 3 save 4 xmm 5 frame

static inline bool nonvolatile(unsigned r) { return r == 3 || r == 5 || r == 6 || r == 7 || r >= 12; }

static void prologEvents(const CodeMap& cm, const uint8_t* code, uint64_t avail, std::vector<PEvent>& ev) {
  ev.clear();
  if (!code) return;
  uint64_t lim = std::min<uint64_t>(avail, 255), i = 0, sp = 0;
  uint64_t eaxImm = 0;
  while (i < lim) {
    x64::Insn I;
    if (!cm.step(code + i, avail - i, I)) break;
    const uint8_t* q = code + i;
    i += I.len;
    if (I.trunc || I.enc != x64::ENC_LEGACY || i > lim) break;
    unsigned op = I.op, rex = I.prex != x64::NOPOS ? q[I.prex] : 0;
    if (op == 0xC3 || op == 0xC2 || op == 0xE9 || op == 0xEB || (op >= 0x70 && op <= 0x7F) || (op >= 0x180 && op <= 0x18F)) break;
    PEvent e{(uint8_t)i, 0, 0, 0, 0};
    if (op >= 0x50 && op <= 0x57) { e.kind = 1; e.reg = (uint8_t)((op & 7) | (rex & 1) << 3); sp += 8; }
    else if (op == 0xB8 && !(rex & 1)) eaxImm = g32(q + I.nstruct);
    else if (I.hasmodrm) {
      unsigned m = I.modrm, mod = m >> 6, reg = ((m >> 3) & 7) | (rex & 4) << 1, rm = m & 7;
      bool rspMem = mod != 3 && rm == 4 && I.psib != x64::NOPOS && (q[I.psib] & 7) == 4 && !(rex & 1);
      int64_t disp = 0;
      if (rspMem && I.nfield && (I.fclass[0] == x64::F_D8 || I.fclass[0] == x64::F_D32))
        disp = I.fclass[0] == x64::F_D8 ? (int8_t)q[I.nstruct] : (int32_t)g32(q + I.nstruct);
      if ((op == 0x81 || op == 0x83) && mod == 3 && rm == 4 && ((m >> 3) & 7) == 5 && !(rex & 1)) {
        e.kind = 2;
        e.val = op == 0x83 ? (uint32_t)(int32_t)(int8_t)q[I.len - 1] : g32(q + I.len - 4);
        sp += e.val;
      } else if (((op == 0x2B && m == 0xE0) || (op == 0x29 && m == 0xC4)) && (rex & 8) && !(rex & 5)) {
        e.kind = 2; e.val = (uint32_t)eaxImm; sp += e.val;  // sub rsp, rax after __chkstk
      } else if (op == 0x89 && rspMem && (rex & 8) && nonvolatile(reg) && disp >= 0) {
        e.kind = 3; e.reg = (uint8_t)reg; e.val = (uint32_t)disp;
      } else if ((op == 0x129 || op == 0x17F) && rspMem && reg >= 6 && disp >= 0) {
        e.kind = 4; e.reg = (uint8_t)reg; e.val = (uint32_t)disp;
      } else if (op == 0x8D && rspMem && (rex & 8) && disp >= 0) {
        e.kind = 5; e.reg = (uint8_t)reg; e.val = (uint32_t)disp;
      }
    }
    if (e.kind) { e.sp = sp; ev.push_back(e); }
  }
}

static bool unwindInfo(uint8_t* u, size_t avail, const std::vector<PEvent>& ev, bool fwd) {
  if (avail < 4 || ((u[0] & 7) != 1 && (u[0] & 7) != 2)) return false;
  unsigned cnt = u[2];
  if (4 + 2 * (size_t)((cnt + 1) & ~1u) > avail) return false;
  uint8_t psize = ev.empty() ? 0 : ev.back().end;
  u[1] ^= psize;
  uint8_t size = fwd ? (uint8_t)(u[1] ^ psize) : u[1];
  // the prolog as far as SizeOfProlog; saves made before the last
  // allocation are recorded at the end of that allocation
  size_t ne = 0;
  uint64_t spf = 0;
  uint8_t lastAlloc = size;
  while (ne < ev.size() && ev[ne].end <= size) {
    if (ev[ne].kind <= 2) lastAlloc = ev[ne].end;
    spf = ev[ne++].sp;
  }
  std::vector<uint16_t> pred;
  uint8_t frame = 0;
  auto save = [&](const PEvent& e, uint8_t off) {
    uint64_t rel = e.val + (spf - e.sp), sc = e.kind == 4 ? 16 : 8;
    uint8_t op = e.kind == 4 ? 8 : 4;
    if (rel / sc <= 0xFFFF) {
      pred.push_back((uint16_t)(off | (unsigned)(op | e.reg << 4) << 8));
      pred.push_back((uint16_t)(rel / sc));
    } else {
      pred.push_back((uint16_t)(off | (unsigned)((op + 1) | e.reg << 4) << 8));
      pred.push_back((uint16_t)rel);
      pred.push_back((uint16_t)(rel >> 16));
    }
  };
  for (size_t k = ne; k-- > 0;)
    if ((ev[k].kind == 3 || ev[k].kind == 4) && ev[k].sp != spf) save(ev[k], lastAlloc);
  for (size_t k = ne; k-- > 0;) {
    const PEvent& e = ev[k];
    switch (e.kind) {
      case 1: pred.push_back((uint16_t)(e.end | (unsigned)(e.reg << 4) << 8)); break;
      case 2:
        if (e.val >= 8 && e.val <= 128) pred.push_back((uint16_t)(e.end | (unsigned)(2 | ((e.val - 8) / 8) << 4) << 8));
        else if (e.val <= 512 * 1024 - 8) { pred.push_back((uint16_t)(e.end | 1u << 8)); pred.push_back((uint16_t)(e.val / 8)); }
        else { pred.push_back((uint16_t)(e.end | 0x11u << 8)); pred.push_back((uint16_t)e.val); pred.push_back((uint16_t)(e.val >> 16)); }
        break;
      case 3: case 4: if (e.sp == spf) save(e, e.end); break;
      case 5:
        pred.push_back((uint16_t)(e.end | 3u << 8));
        frame = (uint8_t)(e.reg | (e.val / 16) << 4);
        break;
    }
  }
  u[3] ^= frame;
  for (unsigned k = 0; k < cnt && k < pred.size(); k++) s16(u + 4 + 2 * k, (uint16_t)(g16(u + 4 + 2 * k) ^ pred[k]));
  return true;
}

// cm: code of the image (may be null), vbias: added to pc values to find it
static void ehframe(uint8_t* p, size_t n, uint64_t va, bool fwd, const CodeMap* cm, uint64_t vbias, int style) {
  FuncCode F;
  CfaFrame frame;  // of the previous FDE
  if (cm) cm->reset();
  std::vector<Cie> cies;
  size_t i = 0;
  uint32_t prevEnd = 0, prevLsda = 0;
  uint64_t prevEnd64 = 0;
  while (i + 8 <= n) {
    uint32_t len = g32(p + i);
    if (len == 0 || len == 0xFFFFFFFFu || len > n - i - 4) break;
    size_t idf = i + 4, end = idf + len;
    uint32_t id = g32(p + idf);
    if (id == 0) {
      Cie c;
      c.off = (uint32_t)i;
      if (parseCie(p, end, idf + 4, c)) cies.push_back(c);
      i = end;
      continue;
    }
    // FDE: CIE pointer
    uint32_t f1 = (uint32_t)idf + 1, ptr;
    if (fwd) {
      ptr = id;
      uint32_t t = f1 - ptr;
      if (t == 0) t = f1;
      s32(p + idf, t);
    } else {
      ptr = (id == f1) ? f1 : f1 - id;
      s32(p + idf, ptr);
    }
    uint32_t cieOff = (uint32_t)idf - ptr;
    const Cie* c = nullptr;
    // CIEs are seen in increasing offset order: binary search
    size_t lo = 0, hi = cies.size();
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (cies[mid].off < cieOff) lo = mid + 1; else hi = mid;
    }
    if (lo < cies.size() && cies[lo].off == cieOff) c = &cies[lo];
    size_t j = idf + 4;
    if (c && enc4(c->renc) && j + 8 <= end) {
      uint32_t fva = (uint32_t)(va + j);
      uint32_t pcrel = (c->renc & 0x70) == 0x10 ? fva : 0;
      // pc_begin: gap after the previous FDE, minus the padding found there
      uint32_t gap = cm && prevEnd64 ? padAt(cm, vbias + prevEnd64) : 0;
      uint32_t v = g32(p + j), range = g32(p + j + 4), absb;
      if (fwd) { absb = v + pcrel; s32(p + j, absb - prevEnd - gap); }
      else { absb = v + prevEnd + gap; s32(p + j, absb - pcrel); }
      uint64_t pcva = (c->renc & 0x0F) == 0x0B && (c->renc & 0x70) == 0x10
                          ? va + j + (uint64_t)(int64_t)(int32_t)(absb - pcrel) : (uint64_t)absb;
      // pc_range: minus the predicted function length
      uint32_t plen = cm ? funcLenAt(cm, vbias + pcva) : 0;
      if (fwd) s32(p + j + 4, range - plen);
      else { range += plen; s32(p + j + 4, range); }
      prevEnd = absb + range;
      prevEnd64 = pcva + range;
      j += 8;
      // CFA program: advances ranked against the function's instructions
      size_t prog = j;
      bool progOk = true;
      if (c->hasz) {
        uint64_t al;
        if (!uleb(p, end, prog, al) || al > end - prog) progOk = false;
        else prog += (size_t)al;
      }
      bool ran = false;
      if (progOk && cm && c->caf1) {
        uint64_t avail = 0;
        const uint8_t* code = cm->at(vbias + pcva, avail);
        if (code) {
          F.build(*cm, code, avail, range);
          cfaProgram(p, prog, end, F, code, c->daf8 && style <= CFA_LLVM ? style : CFA_RANK, fwd, frame, pcva & 15);
          ran = true;
        }
      }
      if (!ran) frame.valid = false;
      if (c->hasz && c->lenc != 0xFF && enc4(c->lenc)) {
        uint64_t al;
        size_t k = j;
        if (uleb(p, end, k, al) && al == 4 && k + 4 <= end) {
          uint32_t lva = (uint32_t)(va + k);
          uint32_t lrel = (c->lenc & 0x70) == 0x10 ? lva : 0;
          // 0 means "no LSDA": it stays 0; the code it would collide with
          // takes the slot that 0 leaves free
          uint32_t w = g32(p + k);
          if (w != 0) {
            if (fwd) {
              uint32_t absl = w + lrel, y = absl - prevLsda;
              if (y == 0) y = lrel - prevLsda;
              s32(p + k, y);
              prevLsda = absl;
            } else {
              uint32_t x = (w == lrel - prevLsda) ? prevLsda - lrel : w + prevLsda - lrel;
              s32(p + k, x);
              prevLsda = x + lrel;
            }
          }
        }
      }
    }
    i = end;
  }
}

// Prediction of the .eh_frame_hdr table at virtual address hdrva from the
// (untransformed) .eh_frame at va: FDEs sorted by initial location.
static void ehhdrPredict(const uint8_t* p, size_t n, uint64_t va, uint64_t hdrva, HdrPred& out) {
  struct F { uint64_t pc, fva; };
  std::vector<F> fd;
  std::vector<Cie> cies;
  size_t i = 0;
  while (i + 8 <= n) {
    uint32_t len = g32(p + i);
    if (len == 0 || len == 0xFFFFFFFFu || len > n - i - 4) break;
    size_t idf = i + 4, end = idf + len;
    uint32_t id = g32(p + idf);
    if (id == 0) {
      Cie c;
      c.off = (uint32_t)i;
      if (parseCie(p, end, idf + 4, c)) cies.push_back(c);
    } else {
      uint32_t cieOff = (uint32_t)idf - id;
      size_t lo = 0, hi = cies.size();
      while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (cies[mid].off < cieOff) lo = mid + 1; else hi = mid;
      }
      size_t j = idf + 4;
      if (lo < cies.size() && cies[lo].off == cieOff && enc4(cies[lo].renc) && j + 4 <= end) {
        uint8_t enc = cies[lo].renc;
        uint64_t v = (enc & 0x0F) == 0x0B ? (uint64_t)(int64_t)(int32_t)g32(p + j) : g32(p + j);
        uint64_t pc = ((enc & 0x70) == 0x10 ? va + j : 0) + v;
        fd.push_back({pc, va + i});
      }
    }
    i = end;
  }
  std::stable_sort(fd.begin(), fd.end(), [](const F& a, const F& b) { return a.pc < b.pc; });
  out.clear();
  for (auto& f : fd) out.push_back({(uint32_t)(f.pc - hdrva), (uint32_t)(f.fva - hdrva)});
}

//----------------------------------------------------------------------------
// .gnu.hash: bloom filter, buckets and chains follow from the names of the
// hashed .dynsym entries, so the table is XORed with its recomputation
// (XOR: forward and inverse are the same).  The 16-byte header stays.
static void gnuhash(uint8_t* p, size_t n, const uint8_t* sym, uint64_t symsize, const uint8_t* str, uint64_t strsize) {
  if (n < 16) return;
  uint32_t nb = g32(p), symoff = g32(p + 4), bsz = g32(p + 8), shift = g32(p + 12);
  uint64_t nsym = symsize / 24;
  if (nb == 0 || bsz == 0 || symoff > nsym) return;
  uint64_t bloomEnd = 16 + 8ull * bsz, buckEnd = bloomEnd + 4ull * nb, chainEnd = buckEnd + 4ull * (nsym - symoff);
  if (chainEnd > n) return;
  std::vector<uint32_t> h((size_t)(nsym - symoff));
  for (uint64_t i = symoff; i < nsym; i++) {
    uint32_t name = g32(sym + 24 * i), x = 5381;
    for (uint64_t k = name; k < strsize && str[k]; k++) x = x * 33 + str[k];
    h[(size_t)(i - symoff)] = x;
  }
  std::vector<uint64_t> bloom(bsz, 0);
  std::vector<uint32_t> buck(nb, 0);
  for (size_t j = 0; j < h.size(); j++) {
    uint32_t x = h[j];
    bloom[(x / 64) % bsz] |= (uint64_t)1 << (x % 64) | (uint64_t)1 << ((x >> (shift & 31)) % 64);
    uint32_t b = x % nb;
    if (!buck[b]) buck[b] = (uint32_t)(symoff + j);
  }
  for (uint32_t k = 0; k < bsz; k++) s64(p + 16 + 8 * k, g64(p + 16 + 8 * k) ^ bloom[k]);
  for (uint32_t k = 0; k < nb; k++) s32(p + bloomEnd + 4 * k, g32(p + bloomEnd + 4 * k) ^ buck[k]);
  for (size_t j = 0; j < h.size(); j++) {
    bool last = j + 1 == h.size() || h[j + 1] % nb != h[j] % nb;
    uint32_t c = (h[j] & ~1u) | (last ? 1u : 0u);
    s32(p + buckEnd + 4 * j, g32(p + buckEnd + 4 * j) ^ c);
  }
}

//----------------------------------------------------------------------------
// .gcc_except_table: the call-site records of each LSDA (found through the
// FDEs of the untransformed .eh_frame) are ranked against the function's
// code: the start among (end of the previous record, call starts after it),
// the end among call ends, the landing pad among (0, instruction starts
// after the previous landing pad).  GCC writes one record per call, clang
// one per range between calls; both become mostly 0 and 1.  Each rank is
// written as a ULEB128 of the original field's length, so the table stays
// in place; that needs minimal ULEB128 fields and LSDAs that don't overlap,
// which lsdaUsable checks.

struct LsdaRef { uint64_t pc, range, off; };  // function, LSDA offset in the table

// LSDAs of the functions in .eh_frame (p, n at va) that lie in the table at
// tva (tn bytes), in FDE order, each once
static void lsdaRefs(const uint8_t* p, size_t n, uint64_t va, uint64_t tva, uint64_t tn, std::vector<LsdaRef>& out) {
  out.clear();
  std::vector<Cie> cies;
  std::vector<uint64_t> seen;
  size_t i = 0;
  while (i + 8 <= n) {
    uint32_t len = g32(p + i);
    if (len == 0 || len == 0xFFFFFFFFu || len > n - i - 4) break;
    size_t idf = i + 4, end = idf + len;
    uint32_t id = g32(p + idf);
    if (id == 0) {
      Cie c;
      c.off = (uint32_t)i;
      if (parseCie(p, end, idf + 4, c)) cies.push_back(c);
      i = end;
      continue;
    }
    uint32_t cieOff = (uint32_t)idf - id;
    size_t lo = 0, hi = cies.size();
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (cies[mid].off < cieOff) lo = mid + 1; else hi = mid;
    }
    size_t j = idf + 4;
    if (lo < cies.size() && cies[lo].off == cieOff) {
      const Cie& c = cies[lo];
      if (c.hasz && enc4(c.renc) && c.lenc != 0xFF && enc4(c.lenc) && j + 8 <= end) {
        uint64_t v = (c.renc & 0x0F) == 0x0B ? (uint64_t)(int64_t)(int32_t)g32(p + j) : g32(p + j);
        uint64_t pc = ((c.renc & 0x70) == 0x10 ? va + j : 0) + v, range = g32(p + j + 4);
        size_t k = j + 8;
        uint64_t al;
        if (uleb(p, end, k, al) && al == 4 && k + 4 <= end) {
          uint64_t w = (c.lenc & 0x0F) == 0x0B ? (uint64_t)(int64_t)(int32_t)g32(p + k) : g32(p + k);
          uint64_t l = ((c.lenc & 0x70) == 0x10 ? va + k : 0) + w;
          if (w && l >= tva && l - tva < tn && std::find(seen.begin(), seen.end(), l) == seen.end()) {
            seen.push_back(l);
            out.push_back({pc, range, l - tva});
          }
        }
      }
    }
    i = end;
  }
}

// ULEB128 field at p[i] (end e): value, length
static inline bool ulebField(const uint8_t* p, size_t e, size_t i, uint64_t& v, size_t& len) {
  size_t k = i;
  if (!uleb(p, e, k, v) || k - i > 5) return false;
  len = k - i;
  return true;
}
static inline size_t ulebLen(uint64_t v) { size_t n = 1; while (v >= 0x80) { v >>= 7; n++; } return n; }
static inline void ulebPut(uint8_t* p, uint64_t v, size_t len) {  // padded to len bytes
  for (size_t k = 0; k < len; k++) { p[k] = (uint8_t)((v & 0x7F) | (k + 1 < len ? 0x80 : 0)); v >>= 7; }
}

// Ranking within the values whose minimal ULEB128 has len bytes: first the
// candidates, a[k] - base for k in [k0, k1) after an optional `first`, all
// ascending; then the other values, ascending.
struct LsdaRank {
  uint64_t lo, hi;      // value range of the field length
  bool f;               // `first` is a candidate
  uint64_t first, base;
  const uint64_t* a;    // candidates a[0..na) (positions; value = a[k] - base)
  size_t na;
  LsdaRank(size_t len, bool hasFirst, uint64_t fv, const std::vector<uint64_t>& v, uint64_t from, uint64_t b) {
    lo = len == 1 ? 0 : (uint64_t)1 << (7 * (len - 1));
    hi = (uint64_t)1 << (7 * len);
    base = b;
    f = hasFirst && fv >= lo && fv < hi;
    first = fv;
    // positions > from (values > from - base) inside [lo, hi), and above first
    uint64_t pmin = std::max<uint64_t>(from + 1, lo + base);
    if (f) pmin = std::max<uint64_t>(pmin, first + base + 1);
    const uint64_t* b0 = std::lower_bound(v.data(), v.data() + v.size(), pmin);
    const uint64_t* b1 = hi + base < hi ? v.data() + v.size() : std::lower_bound(b0, v.data() + v.size(), hi + base);
    a = b0;
    na = (size_t)(b1 - b0);
  }
  size_t ncand() const { return (f ? 1 : 0) + na; }
  uint64_t below(uint64_t x) const {  // candidates < x
    return (f && first < x ? 1 : 0) + (uint64_t)(std::lower_bound(a, a + na, x + base) - a);
  }
  uint64_t rank(uint64_t x) const {
    if (f && x == first) return 0;
    const uint64_t* q = std::lower_bound(a, a + na, x + base);
    if (q < a + na && *q == x + base) return (f ? 1 : 0) + (uint64_t)(q - a);
    return ncand() + (x - lo) - below(x);
  }
  uint64_t unrank(uint64_t r) const {
    if (f && r == 0) return first;
    if (r < ncand()) return a[r - (f ? 1 : 0)] - base;
    uint64_t t = r - ncand(), c = 0;
    for (;;) {  // smallest x with (x - lo) - candidates below x == t, x no candidate
      uint64_t x = lo + t + c;
      uint64_t c2 = below(x + 1);
      if (c2 == c) return x;
      c = c2;
    }
  }
};

// call starts and ends; block starts after an unconditional transfer (where
// landing pads go: they are never fallen into)
struct CallInfo { std::vector<uint64_t> after, cstart, cend; };

static void callInfo(const CodeMap& cm, const uint8_t* code, uint64_t avail, uint64_t len, CallInfo& ci) {
  ci.after.clear(); ci.cstart.clear(); ci.cend.clear();
  if (!code) return;
  if (len > avail) len = avail;
  uint64_t i = 0;
  bool barrier = false;
  while (i < len) {
    x64::Insn I;
    if (!cm.step(code + i, avail - i, I)) break;
    bool call = !I.trunc && I.enc == x64::ENC_LEGACY &&
                (I.op == 0xE8 || (I.op == 0xFF && I.hasmodrm && ((I.modrm >> 3) & 7) == 2));
    if (barrier && !isPad(I)) { ci.after.push_back(i); barrier = false; }
    if (call) ci.cstart.push_back(i);
    i += I.len;
    if (call) ci.cend.push_back(i);
    if (I.trunc) continue;
    unsigned reg = (I.modrm >> 3) & 7;
    if (I.op == 0xC3 || I.op == 0xC2 || I.op == 0xE9 || I.op == 0xEB || I.op == 0x10B || I.op == 0xF4 ||
        (I.op == 0xFF && I.hasmodrm && (reg == 4 || reg == 5)))
      barrier = true;
  }
}

// call-site table of the LSDA at p[o] (n bytes in all): [cs, ce), or false
static bool lsdaCalls(const uint8_t* p, size_t n, size_t o, size_t& cs, size_t& ce) {
  if (o + 3 > n || p[o] != 0xFF) return false;  // LPStart must be the function start
  size_t i = o + 1;
  uint8_t tt = p[i++];
  uint64_t v;
  if (tt != 0xFF && !uleb(p, n, i, v)) return false;
  if (i >= n || p[i++] != 0x01) return false;  // ULEB128 call sites
  if (!uleb(p, n, i, v) || v > n - i) return false;
  cs = i;
  ce = i + (size_t)v;
  return true;
}

// can the file's LSDAs be transformed: every call-site field minimal, no
// two of them overlapping
static bool lsdaUsable(const uint8_t* p, size_t n, const std::vector<LsdaRef>& refs) {
  std::vector<std::pair<size_t, size_t> > ext;
  for (auto& r : refs) {
    size_t i, e;
    if (!lsdaCalls(p, n, (size_t)r.off, i, e)) continue;
    ext.push_back({(size_t)r.off, e});
    while (i < e) {
      for (int f = 0; f < 4 && i < e; f++) {
        uint64_t v;
        size_t len;
        if (!ulebField(p, e, i, v, len)) return true;  // the walk stops here on both sides
        if (f < 3 && len != ulebLen(v)) return false;
        i += len;
      }
    }
  }
  std::sort(ext.begin(), ext.end());
  for (size_t k = 1; k < ext.size(); k++)
    if (ext[k].first < ext[k - 1].second) return false;
  return true;
}

static void lsdaTable(uint8_t* p, size_t n, const std::vector<LsdaRef>& refs, const CodeMap& cm, uint64_t vbias, bool fwd) {
  cm.reset();
  CallInfo ci;
  for (const LsdaRef& r : refs) {
    size_t i, e;
    if (!lsdaCalls(p, n, (size_t)r.off, i, e)) continue;
    uint64_t avail = 0;
    const uint8_t* code = cm.at(vbias + r.pc, avail);
    callInfo(cm, code, code ? avail : 0, r.range, ci);
    uint64_t prevEnd = 0, prevLp = 0;
    while (i < e) {
      uint64_t s = 0, l = 0;
      for (int f = 0; f < 4 && i < e; f++) {
        uint64_t v;
        size_t len;
        if (!ulebField(p, e, i, v, len)) { i = e; break; }
        if (f < 3) {
          LsdaRank rk = f == 0 ? LsdaRank(len, true, prevEnd, ci.cstart, prevEnd, 0)
                      : f == 1 ? LsdaRank(len, false, 0, ci.cend, s, s)
                               : LsdaRank(len, true, 0, ci.after, prevLp, 0);
          uint64_t x = fwd ? rk.rank(v) : rk.unrank(v);
          if (x >= ((uint64_t)1 << (7 * len))) { i = e; break; }  // corrupt: leave the rest
          ulebPut(p + i, x, len);
          uint64_t orig = fwd ? v : x;
          if (f == 0) s = orig;
          else if (f == 1) { l = orig; prevEnd = s + l; }
          else if (orig) prevLp = orig;
        }
        i += len;
      }
    }
  }
}

}  // namespace tables
