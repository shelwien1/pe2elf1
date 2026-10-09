// del_eh - zero out or delete unwind/exception tables and relocations in
// executables that never throw, so that they compress better.
//
//   del_eh [options] input output
//
// ELF64 (little endian):
//   -e  .eh_frame, .eh_frame_hdr, .gcc_except_table, .debug_frame and their
//       relocation sections; PT_GNU_EH_FRAME becomes PT_NULL.  Without
//       section headers the tables are found through PT_GNU_EH_FRAME.
//   -r  relocation sections that are not loaded (.rela.text etc., left by
//       --emit-relocs); .rela.dyn and .rela.plt are needed and kept.
// PE32 / PE32+:
//   -e  exception directory (.pdata) and .xdata.
//   -r  base relocations (.reloc) of an executable: the image is marked
//       "relocations stripped" and loses its dynamic base (ASLR) flags, so
//       it always loads at its preferred base.  DLLs need -f.
//
// Without -d the bytes are zeroed and the layout is kept.  With -d they are
// taken out of the file where the layout allows it: ELF sections at the end
// of a loaded segment (the segment is shortened, later file offsets move by
// whole pages so that every segment keeps its offset/address congruence)
// and sections outside segments; PE section data (the section becomes
// uninitialized, or is dropped when it is the last one).  Whatever cannot
// be deleted is zeroed.
//
// A program treated this way still runs as long as nothing is thrown or
// unwound: a C++ exception then ends in std::terminate.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

typedef std::vector<uint8_t> Buf;

static uint16_t g16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t g32(const uint8_t* p) { return (uint32_t)g16(p) | (uint32_t)g16(p + 2) << 16; }
static uint64_t g64(const uint8_t* p) { return (uint64_t)g32(p) | (uint64_t)g32(p + 4) << 32; }
static void s16(uint8_t* p, uint64_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void s32(uint8_t* p, uint64_t v) { s16(p, v); s16(p + 2, v >> 16); }
static void s64(uint8_t* p, uint64_t v) { s32(p, v); s32(p + 4, v >> 32); }

struct Opts {
  bool eh = false, rel = false, del = false, force = false, verbose = false;
  std::vector<std::string> names;
};

static bool verbose = false;
static void note(const char* fmt, const char* a, uint64_t n) {
  if (verbose) { fprintf(stderr, fmt, a, (unsigned long long)n); fputc('\n', stderr); }
}

static bool readFile(const char* name, Buf& b) {
  FILE* f = strcmp(name, "-") ? fopen(name, "rb") : stdin;
  if (!f) return false;
#ifdef _WIN32
  if (f == stdin) _setmode(_fileno(stdin), _O_BINARY);
#endif
  uint8_t tmp[1 << 16];
  size_t r;
  b.clear();
  while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) b.insert(b.end(), tmp, tmp + r);
  bool ok = !ferror(f);
  if (f != stdin) fclose(f);
  return ok;
}

static bool writeFile(const char* name, const Buf& b) {
  FILE* f = strcmp(name, "-") ? fopen(name, "wb") : stdout;
  if (!f) return false;
#ifdef _WIN32
  if (f == stdout) _setmode(_fileno(stdout), _O_BINARY);
#endif
  bool ok = fwrite(b.data(), 1, b.size(), f) == b.size();
  if (f != stdout) ok = fclose(f) == 0 && ok;
  else ok = fflush(f) == 0 && ok;
  return ok;
}

// removes n bytes at off; returns false if out of range
static bool cut(Buf& b, uint64_t off, uint64_t n) {
  if (off > b.size() || n > b.size() - off) return false;
  b.erase(b.begin() + (size_t)off, b.begin() + (size_t)(off + n));
  return true;
}

//----------------------------------------------------------------------------
// ELF64

namespace elf {

enum { PT_NULL = 0, PT_LOAD = 1, PT_GNU_EH_FRAME = 0x6474E550 };
enum { SHT_NULL = 0, SHT_RELA = 4, SHT_NOBITS = 8, SHT_REL = 9 };
enum { SHF_ALLOC = 2 };

struct Sec {
  size_t hdr;  // file offset of the header
  std::string name;
  uint32_t type;
  uint64_t flags, addr, off, size;
  bool target;
};
struct Seg {
  size_t hdr;
  uint32_t type, flags;
  uint64_t off, vaddr, filesz, memsz;
};

struct File {
  Buf& b;
  std::vector<Sec> S;
  std::vector<Seg> P;
  explicit File(Buf& f) : b(f) {}

  bool parse() {
    if (b.size() < 64 || memcmp(b.data(), "\x7f" "ELF", 4) || b[4] != 2 || b[5] != 1) return false;
    uint64_t phoff = g64(&b[0x20]), shoff = g64(&b[0x28]);
    unsigned phes = g16(&b[0x36]), phn = g16(&b[0x38]), shes = g16(&b[0x3a]), shn = g16(&b[0x3c]),
             shstr = g16(&b[0x3e]);
    if (phn && (phes != 56 || phoff > b.size() || (uint64_t)phn * 56 > b.size() - phoff)) return false;
    for (unsigned k = 0; k < phn; k++) {
      const uint8_t* p = &b[(size_t)(phoff + 56 * k)];
      P.push_back({(size_t)(phoff + 56 * k), g32(p), g32(p + 4), g64(p + 8), g64(p + 16), g64(p + 32), g64(p + 40)});
    }
    if (!shoff) return true;  // no section headers
    if (shes != 64 || shoff > b.size() || b.size() - shoff < 64) return false;
    uint64_t n = shn ? shn : g64(&b[(size_t)shoff + 32]);  // extended numbering
    if (shstr == 0xFFFF) shstr = (unsigned)g32(&b[(size_t)shoff + 40]);
    if (n > (b.size() - shoff) / 64) return false;
    for (uint64_t k = 0; k < n; k++) {
      const uint8_t* p = &b[(size_t)(shoff + 64 * k)];
      S.push_back({(size_t)(shoff + 64 * k), "", g32(p + 4), g64(p + 8), g64(p + 16), g64(p + 24), g64(p + 32), false});
    }
    if (shstr < S.size() && S[shstr].type != SHT_NOBITS && S[shstr].off <= b.size() &&
        S[shstr].size <= b.size() - S[shstr].off) {
      const Sec& st = S[shstr];
      for (size_t k = 0; k < S.size(); k++) {
        uint32_t ni = g32(&b[S[k].hdr]);
        if (ni >= st.size) continue;
        const char* s = (const char*)&b[(size_t)(st.off + ni)];
        size_t len = strnlen(s, (size_t)(st.size - ni));
        S[k].name.assign(s, len);
      }
    }
    return true;
  }
  bool hasData(const Sec& s) const {
    return s.type != SHT_NOBITS && s.type != SHT_NULL && s.size && s.off <= b.size() && s.size <= b.size() - s.off;
  }
  void zero(uint64_t off, uint64_t n) {
    if (off <= b.size() && n <= b.size() - off) memset(&b[(size_t)off], 0, (size_t)n);
  }
  // file offset of a virtual address in a loaded segment, and the bytes left in it
  bool vaToOff(uint64_t va, uint64_t& off, uint64_t& left) const {
    for (auto& p : P)
      if (p.type == PT_LOAD && va >= p.vaddr && va - p.vaddr < p.filesz) {
        off = p.off + (va - p.vaddr);
        left = p.filesz - (va - p.vaddr);
        return off <= b.size() && left <= b.size() - off;
      }
    return false;
  }
  void writeSec(const Sec& s) {
    uint8_t* p = &b[s.hdr];
    s64(p + 24, s.off);
    s64(p + 32, s.size);
  }
  void writeSeg(const Seg& g) {
    uint8_t* p = &b[g.hdr];
    s32(p, g.type);
    s64(p + 8, g.off);
    s64(p + 32, g.filesz);
    s64(p + 40, g.memsz);
  }
  // takes n bytes out at off and moves every later file offset back
  void remove(uint64_t off, uint64_t n) {
    if (!n || !cut(b, off, n)) return;
    uint64_t phoff = g64(&b[0x20]), shoff = g64(&b[0x28]);
    if (phoff >= off + n) s64(&b[0x20], phoff - n);
    if (shoff >= off + n) s64(&b[0x28], shoff - n);
    for (auto& p : P) {
      if (p.hdr >= off + n) p.hdr -= (size_t)n;
      if (p.off >= off + n) p.off -= n;
    }
    for (auto& s : S) {
      if (s.hdr >= off + n) s.hdr -= (size_t)n;
      if (s.off >= off + n) s.off -= n;
      else if (s.off >= off) s.off = off;
    }
    for (auto& p : P) writeSeg(p);
    for (auto& s : S) writeSec(s);
  }
};

// .eh_frame records from off: up to the zero terminator, within left bytes
static uint64_t ehFrameLen(const Buf& b, uint64_t off, uint64_t left) {
  uint64_t i = 0;
  while (left - i >= 4) {
    uint64_t len = g32(&b[(size_t)(off + i)]);
    if (len == 0) return i + 4;
    uint64_t hl = 4;
    if (len == 0xFFFFFFFFu) {
      if (left - i < 12) break;
      len = g64(&b[(size_t)(off + i + 4)]);
      hl = 12;
    }
    if (len > left - i - hl) break;
    i += hl + len;
  }
  return i;
}

static int run(Buf& b, const Opts& o) {
  File F(b);
  if (!F.parse()) return -1;
  const char* ehNames[] = {".eh_frame", ".eh_frame_hdr", ".gcc_except_table", ".debug_frame",
                           ".rela.eh_frame", ".rel.eh_frame", ".rela.gcc_except_table", ".rela.debug_frame"};
  for (auto& s : F.S) {
    bool t = false;
    if (o.eh) for (const char* n : ehNames) t |= s.name == n;
    if (o.rel && (s.type == SHT_RELA || s.type == SHT_REL)) {
      if (s.flags & SHF_ALLOC) note("kept %s: needed by the dynamic loader", s.name.c_str(), 0);
      else t = true;
    }
    for (auto& n : o.names) t |= s.name == n;
    s.target = t && F.hasData(s);
  }
  // PT_GNU_EH_FRAME: without section headers it is the way to the tables
  if (o.eh) {
    bool named = false;
    for (auto& s : F.S) named |= s.target && (s.name == ".eh_frame" || s.name == ".eh_frame_hdr");
    for (auto& p : F.P) {
      if (p.type != PT_GNU_EH_FRAME) continue;
      if (!named && p.filesz >= 4 && p.off <= b.size() && p.filesz <= b.size() - p.off && b[(size_t)p.off] == 1 &&
          b[(size_t)p.off + 1] == 0x1B) {  // version 1, eh_frame_ptr pcrel|sdata4
        uint64_t va = p.vaddr + 4 + (uint64_t)(int64_t)(int32_t)g32(&b[(size_t)p.off + 4]), off, left;
        if (F.vaToOff(va, off, left)) {
          uint64_t n = ehFrameLen(b, off, left);
          F.zero(off, n);
          note("zeroed .eh_frame (found through PT_GNU_EH_FRAME), %s%llu bytes", "", n);
        }
        F.zero(p.off, p.filesz);
        note("zeroed .eh_frame_hdr (PT_GNU_EH_FRAME)%s, %llu bytes", "", p.filesz);
      }
      p.type = PT_NULL;
      F.writeSeg(p);
      note("PT_GNU_EH_FRAME%s -> PT_NULL", "", 0);
    }
  }
  if (!o.del) {
    for (auto& s : F.S)
      if (s.target) {
        F.zero(s.off, s.size);
        note("zeroed %s, %llu bytes", s.name.c_str(), s.size);
      }
    return 0;
  }
  // deletion: first the tails of loaded segments, then sections outside segments
  std::vector<std::pair<uint64_t, uint64_t> > spans;  // [start, end) to take out, with page granularity
  for (auto& g : F.P) {
    if (g.type != PT_LOAD || !g.filesz) continue;
    uint64_t end = g.off + g.filesz;
    // the lowest start a such that [a, end) holds only target sections and gaps
    uint64_t a = end;
    for (;;) {
      bool moved = false;
      for (auto& s : F.S)
        if (s.target && (s.flags & SHF_ALLOC) && s.off < a && s.off >= g.off && s.off + s.size + 15 >= a &&
            s.off + s.size <= end) {
          a = s.off;
          moved = true;
        }
      if (!moved) break;
    }
    if (a == end) continue;
    bool clean = g.memsz == g.filesz;  // no zero-filled tail to keep in place
    for (auto& s : F.S)
      if (!s.target && F.hasData(s) && s.off < end && s.off + s.size > a) clean = false;
    for (auto& q : F.P)
      if (&q != &g && q.type != PT_NULL && q.filesz && q.off < end && q.off + q.filesz > a) clean = false;
    if (!clean) continue;
    g.filesz = a - g.off;
    g.memsz = g.filesz;
    F.writeSeg(g);
    for (auto& s : F.S)
      if (s.target && s.off >= a && s.off + s.size <= end) {
        note("deleted %s, %llu bytes", s.name.c_str(), s.size);
        s.size = 0;
        s.off = a;
        s.target = false;
        F.writeSec(s);
      }
    spans.push_back({a, end});
  }
  // sections that no segment covers can go entirely
  for (auto& s : F.S) {
    if (!s.target) continue;
    bool inSeg = false;
    for (auto& g : F.P) inSeg |= g.type != PT_NULL && g.filesz && s.off < g.off + g.filesz && s.off + s.size > g.off;
    if (inSeg || (s.flags & SHF_ALLOC)) continue;
    note("deleted %s, %llu bytes", s.name.c_str(), s.size);
    spans.push_back({s.off, s.off + s.size});
    s.size = 0;
    s.target = false;
    F.writeSec(s);
  }
  for (auto& s : F.S)
    if (s.target) {
      F.zero(s.off, s.size);
      note("zeroed %s (not at the end of a segment), %llu bytes", s.name.c_str(), s.size);
    }
  // take the spans out, last first; with loaded data after a span only whole pages go
  std::sort(spans.begin(), spans.end());
  for (size_t k = spans.size(); k-- > 0;) {
    uint64_t a = spans[k].first, e = spans[k].second, next = b.size();
    // the gap after the span up to the next thing in the file is padding
    auto later = [&](uint64_t x) { if (x >= e && x < next) next = x; };
    for (auto& g : F.P) if (g.type != PT_NULL && (g.filesz || g.type == PT_LOAD)) later(g.off);
    for (auto& s : F.S) if (s.type != SHT_NULL && (s.size || s.type == SHT_NOBITS)) later(s.off);
    later(g64(&b[0x20]));
    later(g64(&b[0x28]));
    bool loadedAfter = false;
    for (auto& g : F.P) loadedAfter |= g.type == PT_LOAD && g.off >= e;
    uint64_t n = next - a;
    if (loadedAfter) n &= ~(uint64_t)0xFFF;
    F.zero(a, next - a);
    if (n) F.remove(a, n);
  }
  return 0;
}

}  // namespace elf

//----------------------------------------------------------------------------
// PE32 / PE32+

namespace pe {

enum { DIR_EXCEPTION = 3, DIR_SECURITY = 4, DIR_BASERELOC = 5, DIR_DEBUG = 6 };
enum { FILE_RELOCS_STRIPPED = 1, FILE_DLL = 0x2000, DYNAMIC_BASE = 0x40, HIGH_ENTROPY_VA = 0x20 };

struct Sec {
  size_t hdr;
  std::string name;
  uint32_t vsize, va, rawsize, rawptr, ch;
  bool target;
};

struct File {
  Buf& b;
  size_t fh = 0, oh = 0, dd = 0;  // file header, optional header, data directory
  unsigned ndir = 0;
  bool plus = false;
  std::vector<Sec> S;
  explicit File(Buf& f) : b(f) {}

  bool parse() {
    if (b.size() < 0x40 || b[0] != 'M' || b[1] != 'Z') return false;
    uint32_t lf = g32(&b[0x3c]);
    if (lf > b.size() || b.size() - lf < 24 || memcmp(&b[lf], "PE\0\0", 4)) return false;
    fh = lf + 4;
    unsigned nsec = g16(&b[fh + 2]), ohsize = g16(&b[fh + 16]);
    oh = fh + 20;
    if (b.size() - oh < ohsize || ohsize < 2) return false;
    unsigned magic = g16(&b[oh]);
    if (magic != 0x10b && magic != 0x20b) return false;
    plus = magic == 0x20b;
    size_t ndirOff = plus ? 108 : 92;
    dd = oh + (plus ? 112 : 96);
    if (ohsize < ndirOff + 4) return false;
    ndir = g32(&b[oh + ndirOff]);
    if (ndir > (ohsize - (dd - oh)) / 8) ndir = (unsigned)((ohsize - (dd - oh)) / 8);
    size_t sh = oh + ohsize;
    if ((uint64_t)nsec * 40 > b.size() - sh) return false;
    for (unsigned k = 0; k < nsec; k++) {
      const uint8_t* p = &b[sh + 40 * k];
      Sec s;
      s.hdr = sh + 40 * k;
      s.name.assign((const char*)p, strnlen((const char*)p, 8));
      s.vsize = g32(p + 8);
      s.va = g32(p + 12);
      s.rawsize = g32(p + 16);
      s.rawptr = g32(p + 20);
      s.ch = g32(p + 36);
      s.target = false;
      S.push_back(s);
    }
    return true;
  }
  uint32_t dirVa(int k) const { return (unsigned)k < ndir ? g32(&b[dd + 8 * k]) : 0; }
  uint32_t dirSize(int k) const { return (unsigned)k < ndir ? g32(&b[dd + 8 * k + 4]) : 0; }
  void clearDir(int k) { if ((unsigned)k < ndir) { s32(&b[dd + 8 * k], 0); s32(&b[dd + 8 * k + 4], 0); } }
  // file range of an RVA range inside one section's raw data
  bool rvaToOff(uint32_t va, uint32_t n, uint64_t& off) const {
    for (auto& s : S)
      if (va >= s.va && va - s.va < s.rawsize && n <= s.rawsize - (va - s.va)) {
        off = (uint64_t)s.rawptr + (va - s.va);
        return off <= b.size() && n <= b.size() - off;
      }
    return false;
  }
  void zero(uint64_t off, uint64_t n) {
    if (off <= b.size() && n <= b.size() - off) memset(&b[(size_t)off], 0, (size_t)n);
  }
  void writeSec(const Sec& s) {
    uint8_t* p = &b[s.hdr];
    s32(p + 8, s.vsize);
    s32(p + 16, s.rawsize);
    s32(p + 20, s.rawptr);
    s32(p + 36, s.ch);
  }
  bool rawOk(const Sec& s) const { return s.rawsize && s.rawptr <= b.size() && s.rawsize <= b.size() - s.rawptr; }
  // takes n bytes of section data out at off, moving later file offsets back
  void remove(uint64_t off, uint64_t n) {
    if (!n || !cut(b, off, n)) return;
    for (auto& s : S)
      if (s.rawptr >= off + n) { s.rawptr -= (uint32_t)n; writeSec(s); }
    uint32_t sym = g32(&b[fh + 8]);
    if (sym >= off + n) s32(&b[fh + 8], sym - n);
    // the certificate table is addressed by file offset (the signature breaks anyway)
    uint32_t cert = dirVa(DIR_SECURITY);
    if (cert >= off + n) s32(&b[dd + 8 * DIR_SECURITY], cert - n);
    // debug directory entries point at their data by file offset too
    uint64_t doff;
    uint32_t dva = dirVa(DIR_DEBUG), dsz = dirSize(DIR_DEBUG);
    if (dva && rvaToOff(dva, dsz, doff))
      for (uint64_t e = doff; e + 28 <= doff + dsz; e += 28) {
        uint32_t ptr = g32(&b[(size_t)e + 24]);
        if (ptr >= off + n) s32(&b[(size_t)e + 24], ptr - n);
      }
  }
  void checksum() {
    size_t ck = oh + 64;
    if (!g32(&b[ck])) return;  // not used
    uint64_t sum = 0;
    for (size_t i = 0; i < b.size(); i += 2) {
      if (i == ck || i == ck + 2) continue;
      uint32_t w = b[i] | (i + 1 < b.size() ? b[i + 1] << 8 : 0);
      sum += w;
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
    sum = (sum & 0xFFFF) + (sum >> 16);
    s32(&b[ck], (uint32_t)(sum + b.size()));
  }
};

static int run(Buf& b, const Opts& o) {
  File F(b);
  if (!F.parse()) return -1;
  uint16_t chr = g16(&b[F.fh + 18]);
  bool rel = o.rel;
  if (rel && (chr & FILE_DLL) && !o.force) {
    fprintf(stderr, "del_eh: a DLL needs its base relocations (use -f to remove them anyway)\n");
    rel = false;
  }
  // directory ranges first, while the directories are intact
  struct Dir { int k; const char* what; } dirs[] = {{DIR_EXCEPTION, "exception directory"}, {DIR_BASERELOC, "base relocations"}};
  for (auto& d : dirs) {
    if (!((d.k == DIR_EXCEPTION && o.eh) || (d.k == DIR_BASERELOC && rel))) continue;
    uint32_t va = F.dirVa(d.k), n = F.dirSize(d.k);
    uint64_t off;
    if (va && n && F.rvaToOff(va, n, off)) {
      F.zero(off, n);
      note("zeroed the %s, %llu bytes", d.what, n);
    }
    if (va || n) F.clearDir(d.k);
  }
  if (rel) {
    s16(&b[F.fh + 18], chr | FILE_RELOCS_STRIPPED);
    size_t dc = F.oh + 70;
    s16(&b[dc], g16(&b[dc]) & ~(DYNAMIC_BASE | HIGH_ENTROPY_VA));
    note("image marked relocations stripped, dynamic base off%s (%llu)", "", 0);
  }
  for (auto& s : F.S) {
    bool t = (o.eh && (s.name == ".pdata" || s.name == ".xdata")) || (rel && s.name == ".reloc");
    for (auto& n : o.names) t |= s.name == n;
    s.target = t && F.rawOk(s);
  }
  for (size_t k = F.S.size(); k-- > 0;) {
    Sec& s = F.S[k];
    if (!s.target) continue;
    if (!o.del) {
      F.zero(s.rawptr, s.rawsize);
      note("zeroed %s, %llu bytes", s.name.c_str(), s.rawsize);
      continue;
    }
    uint64_t at = s.rawptr, n = s.rawsize;
    bool last = k + 1 == F.S.size();
    for (auto& q : F.S) last &= &q == &s || q.va < s.va;
    if (last) {
      // drop the section header too; the image ends where the section began
      memset(&b[s.hdr], 0, 40);
      s16(&b[F.fh + 2], F.S.size() - 1);
      s32(&b[F.oh + 56], s.va);
      note("deleted %s and its header, %llu bytes", s.name.c_str(), n);
      F.S.erase(F.S.begin() + k);
    } else {
      if (!s.vsize) s.vsize = s.rawsize;
      s.rawsize = 0;
      s.rawptr = 0;
      s.ch = (s.ch & ~0x40u) | 0x80u;  // uninitialized data, like .bss
      F.writeSec(s);
      note("deleted the data of %s, %llu bytes (now uninitialized)", s.name.c_str(), n);
    }
    F.remove(at, n);
  }
  F.checksum();
  return 0;
}

}  // namespace pe

//----------------------------------------------------------------------------

static int usage() {
  fprintf(stderr,
          "del_eh - zero out or delete exception/unwind tables and relocations\n"
          "usage: del_eh [options] input output\n"
          "  -e       exception and unwind tables (the default):\n"
          "           ELF .eh_frame, .eh_frame_hdr, .gcc_except_table, .debug_frame,\n"
          "           PT_GNU_EH_FRAME; PE exception directory (.pdata), .xdata\n"
          "  -r       relocations: ELF relocation sections that are not loaded\n"
          "           (.rela.dyn/.rela.plt are kept); PE base relocations (.reloc),\n"
          "           the image then always loads at its preferred base\n"
          "  -s NAME  also the section NAME (no checks)\n"
          "  -d       delete the bytes where the layout allows it, zero the rest\n"
          "           (default: zero everything, keeping the layout)\n"
          "  -f       remove base relocations from a DLL too\n"
          "  -v       list what is done\n"
          "  input/output may be - for stdin/stdout\n"
          "A program treated this way still runs, but a thrown C++ exception then\n"
          "ends in std::terminate.\n");
  return 1;
}

int main(int argc, char** argv) {
  Opts o;
  int a = 1;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char* p = argv[a] + 1;
    if (!strcmp(p, "s")) {
      if (++a >= argc) return usage();
      o.names.push_back(argv[a]);
      continue;
    }
    for (; *p; p++) {
      switch (*p) {
        case 'e': o.eh = true; break;
        case 'r': o.rel = true; break;
        case 'd': o.del = true; break;
        case 'f': o.force = true; break;
        case 'v': o.verbose = true; break;
        default: return usage();
      }
    }
  }
  if (argc - a != 2) return usage();
  if (!o.eh && !o.rel && o.names.empty()) o.eh = true;
  verbose = o.verbose;
  Buf b;
  if (!readFile(argv[a], b)) { fprintf(stderr, "del_eh: can't read %s\n", argv[a]); return 2; }
  size_t before = b.size();
  int r = elf::run(b, o);
  if (r < 0) r = pe::run(b, o);
  if (r < 0) { fprintf(stderr, "del_eh: %s: not an ELF64 or PE file\n", argv[a]); return 4; }
  if (!writeFile(argv[a + 1], b)) { fprintf(stderr, "del_eh: can't write %s\n", argv[a + 1]); return 3; }
#ifndef _WIN32
  // keep the permissions of the input (an executable stays executable)
  struct stat st;
  if (strcmp(argv[a], "-") && strcmp(argv[a + 1], "-") && stat(argv[a], &st) == 0 && S_ISREG(st.st_mode))
    chmod(argv[a + 1], st.st_mode & 07777);
#endif
  if (verbose) fprintf(stderr, "%s: %zu -> %zu bytes\n", argv[a], before, b.size());
  return 0;
}
