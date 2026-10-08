// analyze.h - find x86-64 code and known tables in PE32+ / ELF64 images.
//
// The result is a list of non-overlapping regions sorted by file offset.
// Regions are only hints for the encoder: the container stores them, so the
// decoder never parses executable headers and any input round-trips, even a
// malformed or truncated one.
#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "tables.h"

enum RegionType : uint8_t {
  R_CODE = 0,  // x86-64 instructions
  R_PDATA,     // PE exception directory
  R_EHHDR,     // .eh_frame_hdr search table
  R_RELA,      // Elf64_Rela array
  R_RELOC,     // PE base relocation blocks
  R_EHFRAME,   // .eh_frame
  R_GNUHASH,   // .gnu.hash
  R_NTYPES
};

struct Region {
  uint64_t off, size;
  uint64_t va;   // virtual address of the first byte (code, .eh_frame, .eh_frame_hdr)
  uint32_t img;  // image number (analysis only)
  uint8_t type;
  // type specific parameters, stored in the container:
  //   .eh_frame_hdr  index of the image's .eh_frame region
  //   Elf64_Rela     (va, file offset, size) of each PT_LOAD segment
  //   .gnu.hash      file offset and size of .dynsym and .dynstr
  //   .eh_frame, PE exception directory: virtual address bias of the
  //                  image's code regions
  std::vector<uint64_t> par;
};

namespace analyze {

using tables::g16;
using tables::g32;
using tables::g64;

struct Ctx {
  const uint8_t* b;
  uint64_t n;
  std::vector<Region>& R;
  uint32_t img;
  void add(uint8_t type, uint64_t off, uint64_t size, uint64_t va) {
    if (off >= n || size == 0) return;
    if (size > n - off) size = n - off;
    R.push_back({off, size, va, img, type, {}});
  }
};

// PE32+ (machine AMD64) image at file offset base; returns its size estimate or 0
static uint64_t parsePE(Ctx& C, uint64_t base, uint64_t vbias) {
  uint64_t n = C.n;
  if (base + 0x40 > n) return 0;
  const uint8_t* p = C.b + base;
  if (p[0] != 'M' || p[1] != 'Z') return 0;
  uint32_t lfa = g32(p + 0x3C);
  if (lfa < 0x40 || lfa > 0x10000 || base + lfa + 24 > n) return 0;
  const uint8_t* h = p + lfa;
  if (g32(h) != 0x00004550 || g16(h + 4) != 0x8664) return 0;
  uint32_t nsec = g16(h + 6), optsz = g16(h + 20);
  if (nsec == 0 || base + lfa + 24 + optsz + 40ull * nsec > n) return 0;
  const uint8_t* opt = h + 24;
  const uint8_t* s0 = opt + optsz;
  uint64_t imgEnd = lfa + 24 + optsz + 40ull * nsec;
  auto rva2off = [&](uint32_t rva, uint32_t size, uint64_t& off) -> bool {
    const uint8_t* s = s0;
    for (uint32_t k = 0; k < nsec; k++, s += 40) {
      uint32_t va = g32(s + 12), rsize = g32(s + 16), roff = g32(s + 20);
      if (rva >= va && rva - va < rsize && size <= rsize - (rva - va)) {
        off = base + roff + (rva - va);
        return true;
      }
    }
    return false;
  };
  const uint8_t* s = s0;
  for (uint32_t k = 0; k < nsec; k++, s += 40) {
    uint32_t vsize = g32(s + 8), rva = g32(s + 12), rsize = g32(s + 16), roff = g32(s + 20), ch = g32(s + 36);
    if (rsize && (uint64_t)roff + rsize > imgEnd) imgEnd = (uint64_t)roff + rsize;
    if (!(ch & 0x20000020)) continue;  // CNT_CODE | MEM_EXECUTE
    uint64_t size = rsize;
    if (vsize && vsize < size) size = vsize;
    C.add(R_CODE, base + roff, size, vbias + rva);
  }
  if (g16(opt) == 0x20B && optsz >= 112 + 8 * 6) {
    uint32_t ndir = g32(opt + 108);
    const uint8_t* dd = opt + 112;
    uint64_t off;
    if (ndir > 3) {
      uint32_t rva = g32(dd + 3 * 8), size = g32(dd + 3 * 8 + 4);
      if (size && rva2off(rva, size, off)) {
        C.add(R_PDATA, off, size / 12 * 12, 0);
        if (!C.R.empty() && C.R.back().type == R_PDATA) C.R.back().par.assign(1, vbias);
      }
    }
    if (ndir > 5) {
      uint32_t rva = g32(dd + 5 * 8), size = g32(dd + 5 * 8 + 4);
      if (size && rva2off(rva, size, off)) C.add(R_RELOC, off, size, 0);
    }
  }
  return imgEnd;
}

// ELF64 (x86-64, little endian) image at file offset base
static uint64_t parseELF(Ctx& C, uint64_t base, uint64_t vbias) {
  uint64_t n = C.n;
  if (base + 64 > n) return 0;
  const uint8_t* p = C.b + base;
  if (g32(p) != 0x464C457F || p[4] != 2 || p[5] != 1 || g16(p + 18) != 62) return 0;
  uint64_t avail = n - base;
  uint64_t shoff = g64(p + 0x28);
  uint32_t shentsize = g16(p + 0x3A), shnum = g16(p + 0x3C), shstrndx = g16(p + 0x3E);
  uint64_t phoff = g64(p + 0x20);
  uint32_t phentsize = g16(p + 0x36), phnum = g16(p + 0x38);
  bool hasPh = phoff && phnum && phentsize >= 56 && phoff < avail && (uint64_t)phentsize * phnum <= avail - phoff;
  std::vector<uint64_t> segs;  // PT_LOAD (va, file offset, size) triples
  if (hasPh) {
    for (uint32_t k = 0; k < phnum; k++) {
      const uint8_t* s = p + phoff + (uint64_t)k * phentsize;
      uint64_t off = g64(s + 8), size = g64(s + 32);
      if (g32(s) != 1 || off >= avail || !size) continue;
      segs.push_back(g64(s + 16));
      segs.push_back(base + off);
      segs.push_back(std::min(size, avail - off));
    }
  }
  size_t before = C.R.size();
  uint64_t imgEnd = 64;
  if (shoff && shnum && shentsize >= 64 && shoff < avail && (uint64_t)shentsize * shnum <= avail - shoff) {
    imgEnd = shoff + (uint64_t)shentsize * shnum;
    const uint8_t* strtab = nullptr;
    uint64_t strsize = 0;
    if (shstrndx < shnum) {
      const uint8_t* s = p + shoff + (uint64_t)shstrndx * shentsize;
      uint64_t off = g64(s + 24), size = g64(s + 32);
      if (off < avail && size <= avail - off) { strtab = p + off; strsize = size; }
    }
    for (uint32_t k = 0; k < shnum; k++) {
      const uint8_t* s = p + shoff + (uint64_t)k * shentsize;
      uint32_t name = g32(s), type = g32(s + 4);
      uint64_t flags = g64(s + 8), addr = g64(s + 16), off = g64(s + 24), size = g64(s + 32), entsize = g64(s + 56);
      if (type == 8 || type == 0 || off >= avail) continue;  // NOBITS / NULL
      if (size > avail - off) size = avail - off;
      if (off + size > imgEnd) imgEnd = off + size;
      const char* nm = "";
      if (strtab && name < strsize && memchr(strtab + name, 0, strsize - name)) nm = (const char*)strtab + name;
      if (flags & 4) { C.add(R_CODE, base + off, size, vbias + addr); continue; }  // SHF_EXECINSTR
      if (type == 4 && entsize == 24) {
        C.add(R_RELA, base + off, size / 24 * 24, 0);
        if (!C.R.empty() && C.R.back().type == R_RELA) C.R.back().par = segs;
        continue;
      }
      if (type == 0x6FFFFFF6 && size >= 16) {  // SHT_GNU_HASH -> .dynsym -> .dynstr
        uint32_t link = g32(s + 40);
        if (link < shnum) {
          const uint8_t* ds = p + shoff + (uint64_t)link * shentsize;
          uint32_t slink = g32(ds + 40);
          uint64_t dsoff = g64(ds + 24), dssize = g64(ds + 32);
          if (g32(ds + 4) == 11 && slink < shnum && dsoff < avail && dssize <= avail - dsoff) {
            const uint8_t* ss = p + shoff + (uint64_t)slink * shentsize;
            uint64_t stroff = g64(ss + 24), strsize = g64(ss + 32);
            if (stroff < avail && strsize <= avail - stroff) {
              C.add(R_GNUHASH, base + off, size, 0);
              if (!C.R.empty() && C.R.back().type == R_GNUHASH)
                C.R.back().par = {base + dsoff, dssize, base + stroff, strsize};
            }
          }
        }
        continue;
      }
      if (!strcmp(nm, ".eh_frame")) {
        C.add(R_EHFRAME, base + off, size, addr);
        if (!C.R.empty() && C.R.back().type == R_EHFRAME) C.R.back().par.assign(1, vbias);
        continue;
      }
      if (!strcmp(nm, ".eh_frame_hdr") && size >= 12) {
        const uint8_t* h = p + off;
        if (h[0] == 1 && h[2] == 0x03 && h[3] == 0x3B) {
          uint64_t tsz = (uint64_t)g32(h + 8) * 8;
          if (tsz > size - 12) tsz = (size - 12) / 8 * 8;
          C.add(R_EHHDR, base + off + 12, tsz, addr + 12);
        }
      }
    }
  }
  if (C.R.size() == before) {
    // no usable section headers: executable PT_LOAD segments
    if (hasPh) {
      for (uint32_t k = 0; k < phnum; k++) {
        const uint8_t* s = p + phoff + (uint64_t)k * phentsize;
        uint64_t off = g64(s + 8), size = g64(s + 32);
        if (off < avail && off + size > imgEnd) imgEnd = std::min(off + size, avail);
        if (g32(s) != 1 || !(g32(s + 4) & 1)) continue;  // PT_LOAD, PF_X
        C.add(R_CODE, base + off, size, vbias + g64(s + 16));
      }
    }
  }
  return C.R.size() > before ? imgEnd : 0;
}

// Scan the whole input for embedded images (archives, installers, ...).
// Each image gets its own 4 GiB virtual address window, so instruction
// indices of different images never collide.
static std::vector<Region> run(const uint8_t* b, uint64_t n, bool scan) {
  std::vector<Region> R;
  Ctx C{b, n, R, 0};
  uint64_t pos = 0, nimg = 0;
  while (pos + 64 <= n) {
    uint64_t vbias = nimg << 32, len = 0;
    C.img = (uint32_t)nimg;
    if (b[pos] == 'M' && b[pos + 1] == 'Z') len = parsePE(C, pos, vbias);
    else if (b[pos] == 0x7F && b[pos + 1] == 'E' && b[pos + 2] == 'L' && b[pos + 3] == 'F') len = parseELF(C, pos, vbias);
    if (len) {
      nimg++;
      pos += std::max<uint64_t>(len, 64);
      continue;
    }
    if (!scan) break;
    // next candidate signature
    uint64_t i = pos + 1;
    for (; i + 64 <= n; i++) {
      uint8_t c = b[i];
      if ((c == 'M' && b[i + 1] == 'Z') || (c == 0x7F && b[i + 1] == 'E' && b[i + 2] == 'L' && b[i + 3] == 'F')) break;
    }
    pos = i;
  }
  // sort by offset; drop overlaps (code regions are clipped)
  std::stable_sort(R.begin(), R.end(), [](const Region& a, const Region& c) { return a.off < c.off; });
  std::vector<Region> out;
  uint64_t end = 0;
  for (auto r : R) {
    if (r.off < end) {
      if (r.type != R_CODE || r.off + r.size <= end) continue;
      uint64_t d = end - r.off;
      r.off += d; r.va += d; r.size -= d;
    }
    out.push_back(r);
    end = r.off + r.size;
  }
  // .eh_frame_hdr tables are predicted from the image's .eh_frame
  for (auto& h : out) {
    if (h.type != R_EHHDR) continue;
    for (size_t k = 0; k < out.size(); k++)
      if (out[k].type == R_EHFRAME && out[k].img == h.img) { h.par.assign(1, k); break; }
  }
  return out;
}

}  // namespace analyze
