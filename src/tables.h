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

namespace tables {

static inline uint32_t g32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static inline void s32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static inline uint64_t g64(const uint8_t* p) { return g32(p) | (uint64_t)g32(p + 4) << 32; }
static inline void s64(uint8_t* p, uint64_t v) { s32(p, (uint32_t)v); s32(p + 4, (uint32_t)(v >> 32)); }
static inline uint16_t g16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
static inline void s16(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

//----------------------------------------------------------------------------
// PE exception directory: RUNTIME_FUNCTION { begin, end, unwind } (RVAs)
// begin -> begin - previous end, end -> end - begin, unwind -> delta
static void pdata(uint8_t* p, size_t n, bool fwd) {
  uint32_t pe = 0, pu = 0;
  for (size_t i = 0; i + 12 <= n; i += 12) {
    uint32_t a = g32(p + i), b = g32(p + i + 4), c = g32(p + i + 8);
    if (fwd) {
      s32(p + i, a - pe); s32(p + i + 4, b - a); s32(p + i + 8, c - pu);
      pe = b; pu = c;
    } else {
      a += pe; b += a; c += pu;
      s32(p + i, a); s32(p + i + 4, b); s32(p + i + 8, c);
      pe = b; pu = c;
    }
  }
}

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

struct Cie { uint32_t off; uint8_t renc, lenc, hasz; };

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
  if (!uleb(p, end, i, v) || !uleb(p, end, i, v)) return false;
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

static void ehframe(uint8_t* p, size_t n, uint64_t va, bool fwd) {
  std::vector<Cie> cies;
  size_t i = 0;
  uint32_t prevEnd = 0, prevLsda = 0;
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
      uint32_t v = g32(p + j), range = g32(p + j + 4), absb;
      if (fwd) { absb = v + pcrel; s32(p + j, absb - prevEnd); }
      else { absb = v + prevEnd; s32(p + j, absb - pcrel); }
      prevEnd = absb + range;
      j += 8;
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
