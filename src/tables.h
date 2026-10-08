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

struct Cie { uint32_t off; uint8_t renc, lenc, hasz, caf1; };

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
  void build(const uint8_t* code, uint64_t avail, uint64_t len) {
    b.clear(); ev.clear();
    if (!code) return;
    if (len > avail) len = avail;
    uint64_t i = 0;
    uint32_t ne = 0;
    b.push_back(0); ev.push_back(0);
    while (i < len) {
      x64::Insn I;
      x64::decode(code + i, (size_t)std::min<uint64_t>(avail - i, 64), I);
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
  uint64_t t = loc + (c - nB);
  for (size_t k = k0; k < k1 && F.b[k] <= t; k++) t++;
  return t - loc;
}

// Walk a CFA program; advance operands are replaced by their rank.
static void cfaProgram(uint8_t* p, size_t i, size_t end, const FuncCode& F, bool fwd) {
  uint64_t loc = 0, v;
  while (i < end) {
    uint8_t op = p[i++];
    uint64_t d, maxd;
    size_t at = i, sz;
    switch (op >> 6) {
      case 1: {  // advance_loc: 6-bit delta in the opcode
        d = op & 0x3F;
        uint64_t x = fwd ? advRank(F, loc, 63, d) : advUnrank(F, loc, 63, d);
        if (x > 63) return;
        p[i - 1] = (uint8_t)(0x40 | x);
        loc += fwd ? d : x;
        continue;
      }
      case 2: if (!uleb(p, end, i, v)) return; continue;  // offset
      case 3: continue;                                      // restore
      default: break;
    }
    switch (op) {
      case 0x00: case 0x0A: case 0x0B: case 0x2D: continue;  // nop, remember/restore_state, window_save
      case 0x02: sz = 1; maxd = 0xFF; break;                 // advance_loc1/2/4
      case 0x03: sz = 2; maxd = 0xFFFF; break;
      case 0x04: sz = 4; maxd = 0xFFFFFFFF; break;
      case 0x05: case 0x09: case 0x0C: case 0x14: case 0x2F:  // two uleb
        if (!uleb(p, end, i, v) || !uleb(p, end, i, v)) return;
        continue;
      case 0x06: case 0x07: case 0x08: case 0x0D: case 0x0E: case 0x13: case 0x2E:  // one uleb/sleb
        if (!uleb(p, end, i, v)) return;
        continue;
      case 0x11: case 0x12: case 0x15:  // uleb, sleb
        if (!uleb(p, end, i, v) || !uleb(p, end, i, v)) return;
        continue;
      case 0x0F:  // def_cfa_expression: block
        if (!uleb(p, end, i, v) || v > end - i) return;
        i += (size_t)v;
        continue;
      case 0x10: case 0x16:  // expression: uleb reg, block
        if (!uleb(p, end, i, v) || !uleb(p, end, i, v) || v > end - i) return;
        i += (size_t)v;
        continue;
      default:
        return;  // set_loc, unknown: stop
    }
    if (at + sz > end) return;
    d = 0;
    for (size_t k = 0; k < sz; k++) d |= (uint64_t)p[at + k] << (8 * k);
    uint64_t x = fwd ? advRank(F, loc, maxd, d) : advUnrank(F, loc, maxd, d);
    if (x > maxd) return;
    for (size_t k = 0; k < sz; k++) p[at + k] = (uint8_t)(x >> (8 * k));
    loc += fwd ? d : x;
    i = at + sz;
  }
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
    x64::decode(c + n, (size_t)std::min<uint64_t>(avail - n, 64), I);
    if (!isPad(I)) break;
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
  uint64_t lim = std::min<uint64_t>(avail, 1 << 20), i = 0;
  bool endish = false;
  while (i < lim) {
    if (endish && cet && isEndbr64(c + i, avail - i)) return (uint32_t)i;
    x64::Insn I;
    x64::decode(c + i, (size_t)std::min<uint64_t>(avail - i, 64), I);
    if (endish && isPad(I)) {
      if (!cet) return (uint32_t)i;
      uint64_t j = i;
      while (j < lim) {  // skip the padding run
        x64::Insn J;
        x64::decode(c + j, (size_t)std::min<uint64_t>(avail - j, 64), J);
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

// cm: code of the image (may be null), vbias: added to pc values to find it
static void ehframe(uint8_t* p, size_t n, uint64_t va, bool fwd, const CodeMap* cm, uint64_t vbias) {
  FuncCode F;
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
      if (progOk && cm && c->caf1) {
        uint64_t avail = 0;
        const uint8_t* code = cm->at(vbias + pcva, avail);
        if (code) {
          F.build(code, avail, range);
          cfaProgram(p, prog, end, F, fwd);
        }
      }
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

}  // namespace tables
