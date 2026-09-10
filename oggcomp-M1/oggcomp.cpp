
static const char *ogc_prog = "oggcomp";

#include <inttypes.h>
#include <math.h>
#include <new>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef size_t sz;
typedef u8 byte;
typedef u16 word;
typedef u32 uint;
typedef u64 qword;

static qword g_prog_in;

#undef __min
#undef __max
#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ALIGN
#if defined(__GNUC__) || defined(__clang__)
#define ALIGN(n) __attribute__((aligned(n)))
#else
#define ALIGN(n) __attribute__((align(n)))
#endif
#endif
#define if_e0(x) if(__builtin_expect((x), 0))
#define if_e1(x) if(__builtin_expect((x), 1))
#ifndef _MSC_VER
#define __assume(x)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
  do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
    if(!(x))                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
      __builtin_unreachable();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
  } while(0)
#endif
template <class T, int N> constexpr uint DIM(T (&)[N]) { return N; }
#if defined(__x86_64) || defined(_M_X64)
#define X64
#endif
#if (defined(__GNUC__) || defined(__clang__)) && !defined(OGC_NO_ATTRS)
#define INLINE inline __attribute__((__always_inline__))
#define NOINLINE __attribute__((__noinline__))
#define HOT __attribute__((__hot__))
#define FLATTEN __attribute__((__flatten__))
#define NORETURN __attribute__((__noreturn__))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define OGC_PRINTF(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define INLINE inline
#define NOINLINE
#define HOT
#define FLATTEN
#define NORETURN
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define OGC_PRINTF(f, a)
#endif
static INLINE u32 ogc_ilog(u32 v) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(OGC_NO_ATTRS)
  return v ? 32 - (u32)__builtin_clz(v) : 0;
#else
  u32 n = 0;
  while(v) {
    n++;
    v >>= 1;
  }
  return n;
#endif
}

#ifdef _WIN32
#define DEV_NULL "nul"
#include <windows.h>
//#define ferror(x) GetLastError()
#define FILE_API_STD 1
#else
#define FILE_API_STD 1
#define DEV_NULL "/dev/null"
#include <sys/mman.h>
#endif

#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"

constexpr int OGC_EXIT_OK = 0;
constexpr int OGC_EXIT_REFUSED = 1;
constexpr int OGC_EXIT_USAGE = 2;
constexpr int OGC_EXIT_IO = 3;
constexpr int OGC_EXIT_INTERNAL = 4;
#define FATAL ogc_fatal
#define FATAL_CODE ogc_fatal_code
#define FATAL_UNLESS ogc_fatal_unless
NORETURN void ogc_fatal(const char *fmt, ...) OGC_PRINTF(1, 2);
NORETURN void ogc_fatal_code(int code, const char *fmt, ...) OGC_PRINTF(2, 3);
void ogc_fatal_unless(int cond, const char *fmt, ...) OGC_PRINTF(2, 3);
#define FATAL_IF_HOT(cond)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
  if(cond)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
  FATAL
#define Fi(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(i = 0; i < (n); i++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
#define Fj(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(j = 0; j < (n); j++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
#define Fk(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(k = 0; k < (n); k++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
template <class T> constexpr const T &ogc_min(const T &a, const T &b) { return a < b ? a : b; }
static void message(const char *fmt, va_list ap) OGC_PRINTF(1, 0);
static void ogc_set_prog(const char *argv0) {
  const char *s;
  if(!argv0 || !*argv0)
    return;
  for(s = argv0; *s; s++)
    if(*s == '/' || *s == '\\')
      argv0 = s + 1;
  if(*argv0)
    ogc_prog = argv0;
}
static void message(const char *fmt, va_list ap) {
  fputs(ogc_prog, stderr);
  fputs(": ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}
static const char *ogc_partial;
static void ogc_output(const char *path) { ogc_partial = path; }
static void ogc_output_kept(void) { ogc_partial = nullptr; }
static NORETURN void die(int code) {
  if(ogc_partial) {
    const char *p = ogc_partial;
    ogc_partial = nullptr;
    remove(p);
  }
  exit(code);
}
NORETURN void ogc_fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
NORETURN void ogc_fatal_code(int code, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(code);
}
void ogc_fatal_unless(int cond, const char *fmt, ...) {
  va_list ap;
  if(cond)
    return;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
static void ogc_paths_distinct(char **v, int n) {
  int i, j;
  for(i = 0; i < n; i++)
    for(j = i + 1; j < n; j++)
      if(!strcmp(v[i], v[j]))
        FATAL_CODE(OGC_EXIT_USAGE,
                   "%s is named twice: an output would be "
                   "written over an input",
                   v[i]);
}
constexpr int OGG_HDRMIN = 27;
constexpr int OGG_MAXSEG = 255;
u32 ogg_crc(const u8 *d, sz n);
u32 ogg_crc_page(const u8 *p, sz n);
int ogg_crc_ok(const u8 *p, sz n);
void ogg_crc_set(u8 *p, sz n);
constexpr sz MAXPAY = (sz)OGG_MAXSEG * OGG_MAXSEG;
constexpr sz PAGE_MAX = OGG_HDRMIN + OGG_MAXSEG + MAXPAY;
static u32 CRC[256];
static void crcinit(void) {
  u32 i, j, c;
  if(CRC[1])
    return;
  Fi(256, c = i << 24; Fj(8, c = (c & (u32)0x80000000) ? (c << 1) ^ (u32)0x04C11DB7 : c << 1); CRC[i] = c);
}
static u32 crcrun(u32 c, const u8 *d, sz n) {
  sz i;
  Fi(n, c = (c << 8) ^ CRC[((c >> 24) ^ d[i]) & 0xFF]);
  return c;
}
static void wr32(u8 *b, u32 v) {
  b[0] = (u8)v;
  b[1] = (u8)(v >> 8);
  b[2] = (u8)(v >> 16);
  b[3] = (u8)(v >> 24);
}
static u32 rd32(const u8 *b) { return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24); }
struct ogg_page {
  u8 type;
  u32 glo, ghi;
  u32 serial, seq;
  int nseg;
  u8 lace[OGG_MAXSEG];
  int np;
  u32 plen[OGG_MAXSEG];
  int tail;
  sz blen;
  sz parse(const u8 *b, sz n) {
    sz i, hl;
    if(n < (sz)OGG_HDRMIN || memcmp(b, "OggS", 4) || b[4])
      return 0;
    nseg = b[26];
    hl = OGG_HDRMIN + (sz)nseg;
    if(n < hl)
      return 0;
    type = b[5];
    glo = rd32(b + 6);
    ghi = rd32(b + 10);
    serial = rd32(b + 14);
    seq = rd32(b + 18);
    blen = 0;
    Fi((sz)nseg, lace[i] = b[OGG_HDRMIN + i]; blen += lace[i]);
    if(n < hl + blen)
      return 0;
    unpack();
    return hl + blen;
  }
  sz emit(u8 *out, const u8 *body) const {
    sz i, hl = OGG_HDRMIN + (sz)nseg;
    memcpy(out, "OggS", 4);
    out[4] = 0;
    out[5] = type;
    wr32(out + 6, glo);
    wr32(out + 10, ghi);
    wr32(out + 14, serial);
    wr32(out + 18, seq);
    out[26] = (u8)nseg;
    Fi((sz)nseg, out[OGG_HDRMIN + i] = lace[i]);
    memcpy(out + hl, body, blen);
    crc_set(out, hl + blen);
    return hl + blen;
  }
  void unpack() {
    sz i;
    u32 run = 0;
    np = 0;
    tail = 0;
    Fi((sz)nseg, run += lace[i]; if(lace[i] != OGG_MAXSEG) {
      plen[np++] = run;
      run = 0;
    });
    if(nseg && lace[nseg - 1] == OGG_MAXSEG) {
      plen[np++] = run;
      tail = 1;
    }
  }
  void pack() {
    int i;
    sz n = 0;
    blen = 0;
    Fi(
        np, u32 v = plen[i]; blen += v; while(v >= (u32)OGG_MAXSEG) {
          FATAL_UNLESS(n < (sz)OGG_MAXSEG, "page needs more than %d segments", OGG_MAXSEG);
          lace[n++] = OGG_MAXSEG;
          v -= OGG_MAXSEG;
        } if(!(tail && i == np - 1)) {
          FATAL_UNLESS(n < (sz)OGG_MAXSEG, "page needs more than %d segments", OGG_MAXSEG);
          lace[n++] = (u8)v;
        } else FATAL_UNLESS(!v, "continued packet has a partial segment")
    );
    nseg = (int)n;
  }
  static u32 crc_page(const u8 *p, sz n) {
    static const u8 z4[4] = {0, 0, 0, 0};
    u32 c;
    if(n < 26)
      return 0;
    crcinit();
    c = crcrun(0, p, 22);
    c = crcrun(c, z4, 4);
    return crcrun(c, p + 26, n - 26);
  }
  static int crc_ok(const u8 *p, sz n) { return n >= (sz)OGG_HDRMIN && rd32(p + 22) == crc_page(p, n); }
  static void crc_set(u8 *p, sz n) {
    FATAL_UNLESS(n >= (sz)OGG_HDRMIN, "cannot checksum a short Ogg page");
    wr32(p + 22, crc_page(p, n));
  }
};
struct vb_info {
  u32 ch, rate;
  u32 bs0, bs1;
};
constexpr u32 VB_MAXBOOK = 256;
constexpr sz VB_ARENA = (sz)96 << 20;
alignas(8) static u8 vb_arena[VB_ARENA];
static sz vb_used;
template <class T> static T *vb_alloc(sz n) {
  sz want = (n * sizeof(T) + 7) & ~(sz)7;
  u8 *p = vb_arena + vb_used;
  FATAL_UNLESS(want <= VB_ARENA - vb_used, "vorbis: the setup needs more than the %" PRIu64 " MiB codebook arena", (u64)(VB_ARENA >> 20));
  vb_used += want;
  return (T *)p;
}
struct vb_book {
  u32 dim, ent;
  u8 look;
  u8 *len;
  u32 *code;
  i32 *nd;
  u32 nv;
  u32 *mult;
  u32 base, off;
  u32 *inv;
  void words() {
    u32 mk[33], i, j, e, l;
    memset(mk, 0, sizeof mk);
    Fi(
        ent, l = len[i]; if(!l) continue; e = mk[l]; FATAL_UNLESS(l >= 32 || !(e >> l), "vorbis: over-populated codebook"); code[i] = e; for(j = l; j > 0; j--) {
          if(mk[j] & 1) {
            mk[j] = j == 1 ? mk[1] + 1 : mk[j - 1] << 1;
            break;
          }
          mk[j]++;
        } for(j = l + 1; j < 33; j++) {
          if((mk[j] >> 1) != e)
            break;
          e = mk[j];
          mk[j] = mk[j - 1] << 1;
        });
  }
  void tree() {
    u32 i, k, n = 1, u = 0, idx, cap;
    i32 t;
    Fi(ent, if(len[i]) u++);
    cap = u + 2;
    nd = vb_alloc<i32>(2 * cap);
    memset(nd, 0, 2 * cap * sizeof *nd);
    Fi(
        ent, if(!len[i]) continue; idx = 0; for(k = len[i]; k > 1; k--) {
          u32 c = 2 * idx + ((code[i] >> (k - 1)) & 1);
          t = nd[c];
          if(!t) {
            FATAL_UNLESS(n < cap, "vorbis: codebook tree overflows");
            t = (i32)n++;
            nd[c] = t;
          }
          FATAL_UNLESS(t > 0, "vorbis: codebook is not prefix free");
          idx = (u32)t;
        } idx = 2 * idx + (code[i] & 1);
        FATAL_UNLESS(!nd[idx], "vorbis: codebook is not prefix free"); nd[idx] = -(i32)(i + 1));
  }
  void grid() {
    u32 i, mx = 0, np;
    Fi(nv, if(mult[i] > mx) mx = mult[i]);
    base = mx + 1;
    off = base >> 1;
    inv = vb_alloc<u32>(base);
    Fi(base, inv[i] = (u32)-1);
    Fi(nv, FATAL_UNLESS(inv[mult[i]] == (u32)-1, "vorbis: duplicate codebook multiplicand %" PRIu64, (u64)mult[i]); inv[mult[i]] = i);
    np = 1;
    if(nv > 1)
      Fi(dim, np *= nv);
    Fi(ent, if(len[i] && i >= np) FATAL("vorbis: codebook entry %" PRIu64 " exceeds lookup grid %" PRIu64, (u64)i, (u64)np));
  }
};
constexpr u32 VB_MAXFLOOR = 64;
constexpr u32 VB_MAXCLASS = 16;
constexpr u32 VB_MAXPART = 32;
constexpr u32 VB_MAXPOST = 256;
struct vb_floor {
  u32 parts, mult, quant, posts, rng;
  u8 pcls[VB_MAXPART];
  u8 cdim[VB_MAXCLASS], csub[VB_MAXCLASS];
  i32 cbook[VB_MAXCLASS], csb[VB_MAXCLASS][8];
  u32 x[VB_MAXPOST];
  u8 srt[VB_MAXPOST];
  void sort() {
    u32 i, j, t;
    Fi(
        posts, srt[i] = (u8)i; for(j = i; j > 0 && x[srt[j - 1]] > x[srt[j]]; j--) {
          t = srt[j];
          srt[j] = srt[j - 1];
          srt[j - 1] = (u8)t;
        });
    Fi(posts, if(i) FATAL_UNLESS(x[srt[i]] != x[srt[i - 1]], "vorbis: floor X list repeats a value"));
  }
};
constexpr u32 VB_MAXRES = 64;
constexpr u32 VB_MAXRCL = 64;
struct vb_res {
  u32 type, beg, end, psz, ncl, cbook;
  u8 casc[VB_MAXRCL];
  i32 book[VB_MAXRCL][8];
};
constexpr u32 VB_MAXMAP = 64;
constexpr u32 VB_MAXSUB = 16;
constexpr u32 VB_MAXCH = 256;
struct vb_map {
  u32 sub, nstep;
  u8 mag[VB_MAXCH], ang[VB_MAXCH];
  u8 mux[VB_MAXCH], fl[VB_MAXSUB], rs[VB_MAXSUB];
};
constexpr u32 VB_MAXMODE = 64;
struct vb_setup {
  vb_book bk[VB_MAXBOOK];
  u32 nbk;
  vb_floor fl[VB_MAXFLOOR];
  u32 nfl;
  vb_res rs[VB_MAXRES];
  u32 nrs;
  vb_map mp[VB_MAXMAP];
  u32 nmp;
  u32 nmd;
  u8 blockflag[VB_MAXMODE], mdmap[VB_MAXMODE];
  void check() const {
    u32 i, j;
    Fi(nfl, Fj(VB_MAXCLASS, u32 k; FATAL_UNLESS(fl[i].cbook[j] < (i32)nbk, "vorbis: floor master book %" PRId64, (i64)fl[i].cbook[j]); Fk(8, FATAL_UNLESS(fl[i].csb[j][k] < (i32)nbk,
                                                                                                                                                          "vorbis: floor subclass "
                                                                                                                                                          "book %" PRId64,
                                                                                                                                                          (i64)fl[i].csb[j][k]))));
    Fi(
        nrs, u32 c, p; FATAL_UNLESS(rs[i].cbook < nbk, "vorbis: residue class book %" PRIu64, (u64)rs[i].cbook); FATAL_UNLESS(rs[i].ncl <= 16, "vorbis: %" PRIu64 " residue classifications, limit 16", (u64)rs[i].ncl); for(c = 0; c < rs[i].ncl; c++) for(p = 0; p < 8; p++) {
          i32 b = rs[i].book[c][p];
          FATAL_UNLESS(b < (i32)nbk, "vorbis: residue book %" PRId64, (i64)b);
          FATAL_UNLESS(b < 0 || bk[b].look == 1,
                       "vorbis: residue book %" PRId64 " has "
                       "no lookup table",
                       (i64)b);
        });
    Fi(nmd, FATAL_UNLESS(mdmap[i] < nmp,
                         "vorbis: mode %" PRIu64 " names mapping "
                         "%d",
                         (u64)i, mdmap[i]));
    Fi(nmp, Fj(mp[i].sub, FATAL_UNLESS(mp[i].fl[j] < nfl, "vorbis: submap floor %d", mp[i].fl[j]); FATAL_UNLESS(mp[i].rs[j] < nrs, "vorbis: submap residue %d", mp[i].rs[j])));
  }
};
static vb_setup vb_su;
struct vb_ctx {
  vb_info i;
  vb_setup *cur;
  void init() {
    memset(&i, 0, sizeof i);
    cur = nullptr;
    vb_used = 0;
  }
  void link() {
    cur = nullptr;
    vb_used = 0;
  }
};
template <class RC> struct rc_pin_io {
  Coroutine *co;
  byte get() { return (byte)co->pin[0].get(); }
  void put(byte c) { co->pin[1].put(c); }
};
#define RC_IO_BASE rc_pin_io
#ifndef RC_IO_BASE
#error "rc.inc: name the byte I/O as RC_IO_BASE before including it"
#endif
enum { SCALElog = 15, SCALE = 1 << SCALElog };
#ifndef RC_LOWBYTES
#define RC_LOWBYTES 8
#endif
#if RC_LOWBYTES < 4 || RC_LOWBYTES > 8
#error "RC_LOWBYTES is 4..8"
#endif
#ifndef RC_FF_TRIM
#define RC_FF_TRIM 32
#endif
#if defined(__GNUC__) || defined(__clang__)
#define RC_BSR64(x) ((uint)(63 - __builtin_clzll(x)))
#else
static inline uint RC_BSR64(qword x) {
  uint i = 0;
  while(x >>= 1)
    i++;
  return i;
}
#endif
template <int f_DEC, int CARRYLESS = 0> struct Rangecoder : RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS>> {
  typedef RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS>> t_IO;
  using t_IO::get;
  using t_IO::put;
  static const uint CODBYTES = 4;
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS = LOWBYTES * 8;
  static const uint sTOP = 1u << 24;
  static const uint gTOP = 1u << 16;
  static constexpr qword LOW_MASK = qword(-1) >> (64 - LOWBITS);
  static constexpr qword LOW_MAX = LOW_MASK + 1;
  uint lowl, lowh;
  uint rpre;
  uint Carry;
  uint code;
  uint range;
  uint FFNum;
  uint Cache;
  uint skip;
  uint carry_lost() const { return CARRYLESS ? FFNum : 0; }
  uint low_Add() {
    uint c = 0;
    const uint cf = (lowl + rpre < lowl);
    lowl += rpre;
    if(LOWBYTES == 8)
      c = (lowh + cf < lowh);
    lowh += cf;
    if(LOWBYTES < 8)
      c = (lowh >= uint(LOW_MAX >> 32));
    lowh &= uint(LOW_MASK >> 32);
    return c;
  }
  uint low_Top() const {
    enum { TOPSH = LOWBITS > 32 ? LOWBITS - 40 : 0 };
    return LOWBITS > 32 ? ((lowh >> TOPSH) & 0xFF) : ((lowl >> 24) & 0xFF);
  }
  void low_Shift8() {
    lowh = (lowh << 8) | (lowl >> 24);
    lowl <<= 8;
    lowh &= uint(LOW_MASK >> 32);
  }
  qword low_Get() const { return (qword(lowh) << 32) + lowl; }
  void emit(byte c) {
    if(LOWBYTES > CODBYTES) {
      if(UNLIKELY(skip)) {
        skip--;
        return;
      }
    }
    put(c);
  }
  void ShiftLow(uint n) {
    const uint c = low_Add();
    rpre = 0;
    if(CARRYLESS) {
      FFNum += c;
      for(; n; n--) {
        emit((byte)low_Top());
        low_Shift8();
      }
    } else {
      Carry |= c;
      for(; n; n--) {
        const uint cl = low_Top();
        low_Shift8();
        if(LIKELY(cl != 0xFF || Carry)) {
          if(Cache != uint(-1))
            emit((byte)(Cache + Carry));
          for(; FFNum; FFNum--)
            emit((byte)(Carry - 1));
          Cache = cl;
          Carry = 0;
        } else
          FFNum++;
      }
    }
  }
  void ShiftCode(uint n) {
    code -= rpre;
    rpre = 0;
    for(; n; n--)
      code = (code << 8) + get();
  }
  void rc_Renorm() {
    const uint n = (range < sTOP) + (range < gTOP);
    if(f_DEC == 0)
      ShiftLow(n);
    else
      ShiftCode(n);
    range <<= n * 8;
  }
  INLINE void rc_Process(uint freq, uint &bit) {
    uint b = bit;
    rc_Renorm();
    rpre = range >> SCALElog;
    rpre *= freq;
    if(f_DEC)
      b = (code >= rpre);
    range -= rpre;
    range = b ? range : rpre;
    rpre &= -b;
    bit = b;
  }
  void rc_Init() {
    lowl = lowh = 0;
    rpre = 0;
    Carry = 0;
    FFNum = 0;
    Cache = uint(-1);
    range = 0xFFFFFFFFu;
    skip = LOWBYTES - CODBYTES;
    if(f_DEC) {
      code = 0;
      for(uint i = 0; i < CODBYTES; i++)
        code = (code << 8) + get();
    }
  }
  void rc_Quit() {
    if(f_DEC)
      return;
    rc_Renorm();
    uint i, n = LOWBYTES;
    qword llow = low_Get();
    const qword high = llow + range;
    const uint hic = (high < llow);
    const uint k = hic ? LOWBITS : RC_BSR64(llow ^ high);
    llow |= (k >= 64) ? ~qword(0) : ((qword(1) << k) - 1);
    n -= k / 8;
    if(CARRYLESS) {
      for(i = 0; i < n; i++)
        emit((byte)(llow >> (LOWBITS - 8 - i * 8)));
    } else if(n == 0 && Carry == 0) {
      const uint run = FFNum + (Cache != uint(-1) && Cache == 0xFF);
      const uint keep = run > RC_FF_TRIM ? run - RC_FF_TRIM : 0;
      if(Cache != uint(-1) && Cache != 0xFF)
        emit((byte)Cache);
      for(i = 0; i < keep; i++)
        emit(0xFF);
    } else {
      if(Cache != uint(-1))
        emit((byte)(Cache + Carry));
      for(i = 0; i < FFNum; i++)
        emit((byte)(0xFF + Carry));
      for(i = 0; i < n; i++)
        emit((byte)(llow >> (LOWBITS - 8 - i * 8)));
    }
  }
};
#undef RC_BSR64
constexpr int CM_PBITS = 12;
constexpr int CM_PONE = 1 << CM_PBITS;
constexpr int CM_PHALF = CM_PONE / 2;
constexpr int CM_STMAX = 2047;
constexpr int CM_CBITS = 16;
constexpr int CM_CONE = 1 << CM_CBITS;
constexpr int CM_TMAX = 4096;
static u16 cm_rec[CM_TMAX + 2];
constexpr int CM_AMAX = 255;
static u16 cm_arec[CM_AMAX + 2];
static u16 cm_mrec[CM_AMAX + 2];
static i16 cm_st[CM_PONE];
static u16 cm_sq[2 * CM_STMAX + 1];
constexpr int CM_QMAX = 130;
static void cm_tables(void) {
  int i;
  for(i = 0; i <= CM_TMAX + 1; i++)
    cm_rec[i] = (u16)(CM_CONE / (i + 2));
  for(i = 0; i <= CM_AMAX + 1; i++)
    cm_arec[i] = (u16)(65536 / (i + 2));
  for(i = 0; i <= CM_AMAX + 1; i++)
    cm_mrec[i] = (u16)(65535 / (i + 1));
  for(i = -CM_STMAX; i <= CM_STMAX; i++) {
    double v = CM_PONE / (1.0 + exp(-i / 256.0));
    int p = (int)(v + 0.5);
    if(p < 1)
      p = 1;
    if(p > CM_PONE - 1)
      p = CM_PONE - 1;
    cm_sq[i + CM_STMAX] = (u16)p;
  }
  {
    int j = 0;
    for(i = 0; i < CM_PONE; i++) {
      while(j < 2 * CM_STMAX && cm_sq[j] < i)
        j++;
      cm_st[i] = (i16)(j - CM_STMAX);
    }
  }
}
static INLINE int cm_squash(int x) {
  x = x < -CM_STMAX ? -CM_STMAX : x > CM_STMAX ? CM_STMAX : x;
  return cm_sq[x + CM_STMAX];
}
static INLINE int cm_stretch(int p) { return cm_st[p]; }
struct cm_cnt {
  u16 p, t;
  INLINE int P() const {
    int v = t ? p >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE void upd(int bit, int lim, int mw) {
    int target = bit ? mw : CM_CONE - 1 - mw;
    int cur = t ? (int)p : CM_CONE / 2;
    t = (u16)(t + (t < lim));
    p = (u16)(cur + (((target - cur) * cm_rec[t]) >> CM_CBITS));
  }
};
struct cm_apm {
  i16 *__restrict t;
  u8 *__restrict c;
  sz n;
  int idx, bkt, q;
  u16 ident[CM_QMAX];
  void init(i16 *tab, u8 *cnt, sz nctx, int qq) {
    int j;
    t = tab;
    c = cnt;
    n = nctx;
    idx = 0;
    bkt = 0;
    q = qq < 3 ? 3 : qq > CM_QMAX ? CM_QMAX : qq;
    for(j = 0; j < q; j++) {
      int x = j * 4096 / (q - 1) - CM_STMAX - 1;
      ident[j] = (u16)cm_squash(x);
    }
  }
  INLINE int pp(int pr, u32 ctx) {
    int s = cm_stretch(pr) + CM_STMAX;
    int pos = s * (q - 1);
    int w = (pos & 4095) >> 5;
    bkt = pos >> 12;
    idx = (int)(ctx * (u32)q) + bkt;
    return (((int)ident[bkt] + t[idx]) * (128 - w) + ((int)ident[bkt + 1] + t[idx + 1]) * w) >> 7;
  }
  INLINE void upd1(int i, int b, int g, int lim) {
    u8 *__restrict cc = c;
    i16 *__restrict tt = t;
    int k = cc[i];
    int v = (int)ident[b] + tt[i];
    cc[i] = (u8)(k + (k < lim));
    v += ((g - v) * cm_arec[k]) >> 16;
    tt[i] = (i16)(v - (int)ident[b]);
  }
  static int limit(int rate) {
    int lim = rate >= 8 ? CM_AMAX : (1 << rate) - 2;
    return lim < 1 ? 1 : lim;
  }
  INLINE void upd(int bit, int lim) {
    int g = bit ? 0 : CM_PONE;
    upd1(idx, bkt, g, lim);
    upd1(idx + 1, bkt + 1, g, lim);
  }
};

#if defined(__clang__)
#define CM_UNROLL _Pragma("unroll")
#elif defined(__GNUC__)
#define CM_UNROLL _Pragma("GCC unroll 8")
#else
#define CM_UNROLL
#endif

template <int N> struct cm_mix {
  i32 *__restrict w;
  u8 *__restrict c;
  int x[N], pr, row, ctxn;
  enum { W0 = (1 << 16) / N };
  void init(i32 *wt, u8 *cnt, sz nctx) {
    (void)nctx;
    w = wt;
    c = cnt;
    row = 0;
    ctxn = 0;
    pr = CM_PHALF;
  }
  INLINE void set(int k, int st) { x[k] = st; }
  INLINE int mix(u32 ctx) {
    i64 s = 0;
    int i;
    ctxn = (int)ctx;
    row = (int)ctx * N;
    CM_UNROLL
    for(i = 0; i < N; i++)
      s += (i64)(w[row + i] + W0) * x[i];
    pr = cm_squash((int)(s >> 16));
    pr = pr < 1 ? 1 : pr > CM_PONE - 1 ? CM_PONE - 1 : pr;
    return pr;
  }
  INLINE void upd(int bit, int lr, int mb) {
    int err, i, n;
    i32 *__restrict ww = w + row;
    if(mb) {
      u8 *__restrict cc = c;
      n = cc[ctxn];
      cc[ctxn] = (u8)(n + (n < CM_AMAX));
      lr += (int)(((i64)mb * lr * cm_mrec[n]) >> 16);
    }
    err = ((bit ? 0 : CM_PONE) - pr) * lr;
    CM_UNROLL
    for(i = 0; i < N; i++)
      ww[i] += (i32)(((i64)x[i] * err) >> 16);
  }
};
#include "sh_mapping.inc"
constexpr i32 RS_DIGIT_MAX = 0x7FFFFF;
static int tc_enc;
#ifndef OC_CARRYLESS
#define OC_CARRYLESS 0
#endif
static Rangecoder<0, OC_CARRYLESS> rce;
static Rangecoder<1, OC_CARRYLESS> rcd;
static vb_ctx *oc_v;
constexpr int TC_MIXN = 7;
constexpr int TC_NODE = 29;
constexpr int TC_MNODE = 169;
constexpr int TC_NBMAX = 62;
static INLINE int tc_node_len(int k) { return 3 + (k < 25 ? k : 25); }
static INLINE int tc_node_man(int nb, int p) { return (nb > 12 ? 12 : nb) * 13 + (p > 12 ? 12 : p); }
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64)(x < 0 ? -x : x);
  int n = 62 - __builtin_clzll(u | 1);
  i32 q = 2 + n * 2 + (i32)((u >> n) & 1);
  return u < 4 ? (i32)u : q;
}
static INLINE i32 tc_abs(i32 v) { return v < 0 ? -v : v; }
static INLINE int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static sz tc_mem;
static void *tc_alloc(sz n) {
  void *p = calloc(n ? n : 1, 1);
  if(!p)
    FATAL_CODE(OGC_EXIT_IO, "out of memory for %" PRIu64 " bytes", (u64)n);
  tc_mem += n;
  return p;
}
static int tc_huge;
static void *tc_map(sz n) {
  void *p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if(p == MAP_FAILED)
    p = nullptr;
#ifdef MADV_HUGEPAGE
  if(p && tc_huge)
    madvise(p, n, MADV_HUGEPAGE);
#endif
#endif
  if(!p)
    FATAL_CODE(OGC_EXIT_IO,
               "cannot map %" PRIu64 " MB for a model table -- "
               "IDX/tsvcomp-*.idx asks for more than this machine will map",
               (u64)(n >> 20));
  return p;
}
constexpr int TCP_NBMAX = 20;
constexpr int TCP_SYMMAX = 256;
constexpr u32 TCP_NODEMAX = 1u << 16;
constexpr u32 TCP_SCALE = 1u << 20;
constexpr u32 TCP_MEMMAX = 64u << 20;
constexpr u32 TCP_MANMAX = 64;
static sz tcp_mem;
static INLINE u64 tcp_pow(u32 b, u32 e) {
  u64 v = 1;
  u32 i;
  for(i = 0; i < e; i++) {
    if(v > ((u64)1 << 40) / (b ? b : 1))
      return (u64)1 << 40;
    v *= b;
  }
  return v;
}
static INLINE int tcp_p(u32 w0, u32 w1) {
  u64 s = (u64)w0 + w1;
  int p;
  if(!s)
    return -1;
  p = (int)(((u64)w0 * CM_PONE + s / 2) / s);
  return p < 1 ? 1 : p > CM_PONE - 1 ? CM_PONE - 1 : p;
}
struct tc_ptab {
  int ok, be;
  u32 nsym, nlev, ndim, nnode;
  u32 nbr;
  u32 of[17], sz_[17];
  u32 *pw;
  u32 *tot;
  u32 *s012;
  u32 *slen;
  i32 *val;
  u32 *brm;
  i16 *ph;
  i16 *pl;
  i16 *psg;
  u32 nmag;
  void drop() {
    free(pw);
    free(tot);
    free(s012);
    free(slen);
    free(val);
    free(brm);
    free(ph);
    free(pl);
    free(psg);
    pw = nullptr;
    tot = nullptr;
    s012 = nullptr;
    slen = nullptr;
    val = nullptr;
    brm = nullptr;
    ph = nullptr;
    pl = nullptr;
    psg = nullptr;
    nmag = 0;
    ok = 0;
  }
  static INLINE int nbits(i32 a) {
    u32 u = (u32)a - 2;
    int n = 0;
    while(u) {
      n++;
      u >>= 1;
    }
    return n;
  }
  u64 *layout(u32 nsym_, u32 nbr_, u32 nlev_, int be_) {
    u64 *acc;
    u64 lim = 1;
    u32 k;
    drop();
    if(!nsym_ || !nbr_ || !nlev_ || nsym_ > TCP_SYMMAX || nlev_ > 16)
      return nullptr;
    nsym = nsym_;
    nbr = nbr_;
    ndim = nlev_;
    be = be_;
    nnode = 0;
    for(k = 0; k < ndim; k++) {
      if(lim > TCP_NODEMAX || nnode + lim > TCP_NODEMAX)
        break;
      if(tcp_mem + (sz)(nnode + lim) * ((nsym * 2 + 4 + TCP_NBMAX) * 4 + (3 + TCP_NBMAX) * 2) > TCP_MEMMAX)
        break;
      sz_[k] = (u32)lim;
      of[k] = nnode;
      nnode += (u32)lim;
      lim *= nbr;
      if(lim > (u64)1 << 32) {
        k++;
        break;
      }
    }
    nlev = k;
    if(!nlev)
      return nullptr;
    of[nlev] = nnode;
    acc = (u64 *)calloc((sz)nnode * nsym, sizeof(u64));
    pw = (u32 *)calloc((sz)nnode * nsym, sizeof(u32));
    tot = (u32 *)calloc(nnode, sizeof(u32));
    s012 = (u32 *)calloc((sz)nnode * 3, sizeof(u32));
    slen = (u32 *)calloc((sz)nnode * TCP_NBMAX, sizeof(u32));
    val = (i32 *)calloc(nsym, sizeof(i32));
    brm = (u32 *)calloc(nsym, sizeof(u32));
    ph = (i16 *)calloc((sz)nnode * 3, sizeof(i16));
    pl = (i16 *)calloc((sz)nnode * TCP_NBMAX, sizeof(i16));
    if(!acc || !pw || !tot || !s012 || !slen || !val || !brm || !ph || !pl) {
      free(acc);
      drop();
      return nullptr;
    }
    tcp_mem += (sz)nnode * ((nsym * 2 + 4 + TCP_NBMAX) * 4 + (3 + TCP_NBMAX) * 2);
    return acc;
  }
  void finish(u64 *acc, int want_sign) {
    u32 i;
    for(i = 0; i < nnode; i++) {
      u64 *a = acc + (sz)i * nsym;
      u32 *p = pw + (sz)i * nsym;
      u64 T = 0;
      u32 j, sum = 0;
      for(j = 0; j < nsym; j++)
        T += a[j];
      if(!T)
        continue;
      for(j = 0; j < nsym; j++) {
        u32 v = (u32)((a[j] * TCP_SCALE + T / 2) / T);
        if(!v && a[j])
          v = 1;
        p[j] = v;
        sum += v;
      }
      tot[i] = sum;
      for(j = 0; j < nsym; j++) {
        i32 a2 = val[j] < 0 ? -val[j] : val[j];
        if(a2 < 3)
          s012[(sz)i * 3 + a2] += p[j];
        else {
          int nb = nbits(a2);
          if(nb > 0 && nb < TCP_NBMAX)
            slen[(sz)i * TCP_NBMAX + nb] += p[j];
        }
      }
    }
    for(i = 0; i < nnode; i++) {
      const u32 *s = s012 + (sz)i * 3;
      const u32 *sl = slen + (sz)i * TCP_NBMAX;
      u32 w0 = tot[i], j;
      int m, k;
      for(m = 0; m < 3; m++) {
        w0 -= s[m];
        ph[(sz)i * 3 + m] = (i16)tcp_p(w0, s[m]);
      }
      for(k = 0; k + 1 < TCP_NBMAX; k++) {
        u32 w = 0;
        for(j = (u32)k + 2; j < TCP_NBMAX; j++)
          w += sl[j];
        pl[(sz)i * TCP_NBMAX + k] = (i16)tcp_p(w, sl[k + 1]);
      }
      pl[(sz)i * TCP_NBMAX + TCP_NBMAX - 1] = -1;
    }
    nmag = 0;
    if(want_sign && nsym <= TCP_MANMAX) {
      i32 mx = 0, m;
      u32 j;
      for(j = 0; j < nsym; j++) {
        i32 a = val[j] < 0 ? -val[j] : val[j];
        if(a > mx)
          mx = a;
      }
      if(mx < 256 && tcp_mem + (sz)nnode * ((sz)mx + 1) * 2 <= TCP_MEMMAX)
        psg = (i16 *)calloc((sz)nnode * ((sz)mx + 1), sizeof(i16));
      if(psg) {
        nmag = (u32)mx + 1;
        tcp_mem += (sz)nnode * nmag * 2;
        for(i = 0; i < nnode; i++) {
          const u32 *p = pw + (sz)i * nsym;
          psg[(sz)i * nmag] = -1;
          for(m = 1; m <= mx; m++) {
            u32 wp = 0, wn = 0;
            for(j = 0; j < nsym; j++) {
              if(val[j] == m)
                wp += p[j];
              else if(val[j] == -m)
                wn += p[j];
            }
            psg[(sz)i * nmag + (u32)m] = (i16)tcp_p(wp, wn);
          }
        }
      }
    }
    free(acc);
    ok = 1;
  }
  int build(const u8 *len, u32 ent, u32 nsym_, u32 nlev_, int be_, const i32 *value, int want_sign) {
    u64 *acc;
    u32 k, i, e, cap;
    u64 lim;
    if(!len)
      return 0;
    acc = layout(nsym_, nsym_, nlev_, be_);
    if(!acc)
      return 0;
    for(i = 0; i < nsym; i++) {
      val[i] = value[i];
      brm[i] = i;
    }
    lim = tcp_pow(nsym, ndim);
    cap = (u32)(lim < ent ? lim : ent);
    for(e = 0; e < cap; e++) {
      u32 L = len[e], nd, s;
      u64 w;
      if(!L || L > 32)
        continue;
      w = (u64)1 << (32 - L);
      for(k = 0; k < nlev; k++) {
        if(be) {
          u64 d = tcp_pow(nbr, ndim - 1 - k);
          nd = (u32)(e / d / nsym);
          s = (u32)((e / d) % nsym);
        } else {
          nd = e % sz_[k];
          s = (e / sz_[k]) % nsym;
        }
        acc[(sz)(of[k] + nd) * nsym + s] += w;
      }
    }
    finish(acc, want_sign);
    return 1;
  }
  int build_floor(const u8 *clen, u32 cent, u32 cs, u32 cd, const u8 *const *blen, const u32 *bent, u32 maxy) {
    u64 *acc;
    u32 nbr_ = 1u << cs, k, i, e, y, cap;
    u64 lim;
    u64 *bw;
    if(!maxy || maxy > TCP_SYMMAX || cs > 3)
      return 0;
    acc = layout(maxy, nbr_, cd, 0);
    if(!acc)
      return 0;
    bw = (u64 *)calloc((sz)nbr_ * maxy, sizeof(u64));
    if(!bw) {
      free(acc);
      drop();
      return 0;
    }
    for(y = 0; y < maxy; y++) {
      u32 sel = nbr_;
      for(i = 0; i < nbr_; i++) {
        u32 mx = blen[i] ? bent[i] : 1;
        if(y < mx) {
          sel = i;
          break;
        }
      }
      val[y] = (i32)y;
      brm[y] = sel < nbr_ ? sel : 0;
      if(sel >= nbr_)
        continue;
      if(!blen[sel]) {
        if(!y)
          bw[(sz)sel * maxy] = TCP_SCALE;
      } else if(blen[sel][y] && blen[sel][y] <= 32)
        bw[(sz)sel * maxy + y] = (u64)1 << (32 - blen[sel][y]);
    }
    for(i = 0; i < nbr_; i++) {
      u64 T = 0;
      for(y = 0; y < maxy; y++)
        T += bw[(sz)i * maxy + y];
      if(!T)
        continue;
      for(y = 0; y < maxy; y++) {
        u64 *q = bw + (sz)i * maxy + y;
        u64 v = (*q * TCP_SCALE + T / 2) / T;
        if(!v && *q)
          v = 1;
        *q = v;
      }
    }
    {
      u64 *cwt = (u64 *)calloc((sz)nnode * nbr_, sizeof(u64));
      if(!cwt) {
        free(bw);
        free(acc);
        drop();
        return 0;
      }
      if(!clen || !cs) {
        for(i = 0; i < nnode; i++)
          cwt[(sz)i * nbr_] = TCP_SCALE;
      } else {
        lim = tcp_pow(nbr_, cd);
        cap = (u32)(lim < cent ? lim : cent);
        for(e = 0; e < cap; e++) {
          u32 L = clen[e], nd, sb;
          u64 w;
          if(!L || L > 32)
            continue;
          w = (u64)1 << (32 - L);
          for(k = 0; k < nlev; k++) {
            nd = e % sz_[k];
            sb = (e / sz_[k]) % nbr_;
            cwt[(sz)(of[k] + nd) * nbr_ + sb] += w;
          }
        }
        for(i = 0; i < nnode; i++) {
          u64 *a = cwt + (sz)i * nbr_, T = 0;
          for(k = 0; k < nbr_; k++)
            T += a[k];
          if(!T)
            continue;
          for(k = 0; k < nbr_; k++) {
            u64 v = (a[k] * TCP_SCALE + T / 2) / T;
            if(!v && a[k])
              v = 1;
            a[k] = v;
          }
        }
      }
      for(i = 0; i < nnode; i++)
        for(y = 0; y < maxy; y++)
          acc[(sz)i * maxy + y] = cwt[(sz)i * nbr_ + brm[y]] * bw[(sz)brm[y] * maxy + y];
      free(cwt);
    }
    free(bw);
    finish(acc, 0);
    return 1;
  }
};
struct tc_pcur {
  const tc_ptab *t;
  u32 idx;
  u32 lev;
  int on;
  INLINE void off() {
    t = nullptr;
    on = 0;
  }
  INLINE void start(const tc_ptab *tab) {
    t = tab && tab->ok ? tab : nullptr;
    idx = 0;
    lev = 0;
    on = t != nullptr;
  }
  INLINE u32 node() const { return t->of[lev] + idx; }
  INLINE int live() const { return on; }
  INLINE int usable() const { return live() && t->tot[node()]; }
  INLINE void step(int s) {
    u32 b, big, little;
    if(!on || !t)
      return;
    if(s < 0 || (u32)s >= t->nsym || lev + 1 > t->nlev) {
      on = 0;
      return;
    }
    b = t->brm[s];
    big = idx * t->nbr + b;
    little = idx + b * t->sz_[lev];
    idx = t->be ? big : little;
    lev++;
    on = lev < t->nlev;
  }
};
static INLINE int tcp_head(const tc_pcur &c, int m) {
  if(!c.live() || m < 0 || m > 2)
    return -1;
  return c.t->ph[(sz)c.node() * 3 + m];
}
static INLINE int tcp_len(const tc_pcur &c, int k) {
  if(!c.live() || k + 1 >= TCP_NBMAX)
    return -1;
  return c.t->pl[(sz)c.node() * TCP_NBMAX + k];
}
static INLINE int tcp_sign(const tc_pcur &c, i64 mag) {
  const u32 *p;
  u32 nd, wn = 0, wp = 0;
  u32 j;
  if(!c.live() || mag <= 0)
    return -1;
  nd = c.node();
  if(c.t->psg)
    return mag < (i64)c.t->nmag ? c.t->psg[(sz)nd * c.t->nmag + (sz)mag] : -1;
  p = c.t->pw + (sz)nd * c.t->nsym;
  for(j = 0; j < c.t->nsym; j++) {
    i32 v = c.t->val[j];
    if(v == (i32)mag)
      wp += p[j];
    else if(v == -(i32)mag)
      wn += p[j];
  }
  return tcp_p(wp, wn);
}
static INLINE int tcp_man(const tc_pcur &c, int nb, int p, u64 pref) {
  const u32 *w;
  u32 nd, w0 = 0, w1 = 0, j;
  if(!c.usable() || nb < 2 || p > nb - 2 || c.t->nsym > TCP_MANMAX)
    return -1;
  nd = c.node();
  w = c.t->pw + (sz)nd * c.t->nsym;
  for(j = 0; j < c.t->nsym; j++) {
    i32 v = c.t->val[j];
    i32 a = v < 0 ? -v : v;
    u32 u;
    if(a < 3)
      continue;
    if(tc_ptab::nbits(a) != nb)
      continue;
    u = (u32)a - 2;
    if((u >> (nb - 1 - p)) != pref)
      continue;
    if((u >> (nb - 2 - p)) & 1)
      w1 += w[j];
    else
      w0 += w[j];
  }
  return tcp_p(w0, w1);
}
static INLINE int tcp_bucket(int p) {
  if(p < 0)
    return 0;
  return tc_clamp((cm_stretch(p) + CM_STMAX) >> 7, 0, 31) + 1;
}
static INLINE int tcp_axis(const tc_pcur &c) { return tcp_bucket(tcp_head(c, 0)); }
static tc_ptab tcp_book[VB_MAXBOOK];
static tc_ptab tcp_cls[VB_MAXRES];
static tc_ptab tcp_flr[VB_MAXFLOOR][VB_MAXCLASS];
static void tcp_free(void) {
  u32 i, j;
  for(i = 0; i < VB_MAXBOOK; i++)
    tcp_book[i].drop();
  for(i = 0; i < VB_MAXRES; i++)
    tcp_cls[i].drop();
  for(i = 0; i < VB_MAXFLOOR; i++)
    for(j = 0; j < VB_MAXCLASS; j++)
      tcp_flr[i][j].drop();
  tcp_mem = 0;
}
static void tcp_build(void) {
  i32 v[TCP_SYMMAX];
  u32 i, j;
  const vb_setup *s = oc_v->cur;
  tcp_free();
  for(i = 0; i < s->nbk; i++) {
    const vb_book *b = s->bk + i;
    if(b->look != 1 || !b->ent || !b->len || !b->mult || !b->nv)
      continue;
    if(b->nv > TCP_SYMMAX)
      continue;
    for(j = 0; j < b->nv; j++)
      v[j] = (i32)b->mult[j] - (i32)b->off;
    tcp_book[i].build(b->len, b->ent, b->nv, b->dim, 0, v, 1);
  }
  for(i = 0; i < s->nrs; i++) {
    const vb_res *r = s->rs + i;
    const vb_book *cb = s->bk + r->cbook;
    if(!cb->ent || !cb->len || r->ncl > TCP_SYMMAX)
      continue;
    for(j = 0; j < r->ncl; j++)
      v[j] = (i32)j;
    tcp_cls[i].build(cb->len, cb->ent, r->ncl, cb->dim, 1, v, 0);
  }
  for(i = 0; i < s->nfl; i++) {
    const vb_floor *f = s->fl + i;
    u32 c;
    for(c = 0; c < VB_MAXCLASS; c++) {
      const u8 *bl[8];
      u32 be[8], nb = 1u << f->csub[c], maxy = 1, k;
      const u8 *cl = nullptr;
      u32 cent = 0;
      if(!f->cdim[c] || f->csub[c] > 3)
        continue;
      for(k = 0; k < nb; k++) {
        i32 sb = f->csb[c][k];
        if(sb >= 0 && (u32)sb < s->nbk && s->bk[sb].ent && s->bk[sb].len) {
          bl[k] = s->bk[sb].len;
          be[k] = s->bk[sb].ent;
          if(be[k] > maxy)
            maxy = be[k];
        } else if(sb >= 0) {
          bl[k] = nullptr;
          be[k] = 0;
          maxy = 0;
          break;
        } else {
          bl[k] = nullptr;
          be[k] = 1;
        }
      }
      if(!maxy || maxy > TCP_SYMMAX)
        continue;
      if(f->csub[c] && f->cbook[c] >= 0 && (u32)f->cbook[c] < s->nbk && s->bk[f->cbook[c]].ent) {
        cl = s->bk[f->cbook[c]].len;
        cent = s->bk[f->cbook[c]].ent;
      }
      tcp_flr[i][c].build_floor(cl, cent, f->csub[c], f->cdim[c], bl, be, maxy);
    }
  }
}

constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }
static INLINE u64 tbl_n(u64 n) {
  FATAL_UNLESS(n > 0 && n <= ((u64)1 << 40), "IDX/tsvcomp-*.idx asks for a table of %" PRIu64 " entries", n);
  return n;
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  FATAL_UNLESS(vol > 0 && vol < ((sz)1 << 31) && nodes > 0,
               "IDX/tsvcomp-*.idx gives the %s an index of %" PRIu64 " rows -- "
               "idx2inc.pl builds indices in int, so a Volume must stay under "
               "2147483648",
               what, (u64)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  FATAL_UNLESS(vol * mult < ((sz)1 << 31),
               "IDX/tsvcomp-*.idx gives the %s %" PRIu64 " rows of %" PRIu64 " -- it addresses them with an int, so the product must stay "
               "under 2147483648",
               what, (u64)vol, (u64)mult);
}
enum { F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL, F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_N };
constexpr int TC_MAXTAG = 128;
constexpr int TC_SGN_AUX = F_N;
constexpr int TC_SGN_HDR = TC_MAXTAG;
constexpr int TC_SGN_FLR = 1;
constexpr int TC_dig_ND = TC_NODE, TC_dig_MND = TC_MNODE;
constexpr int TC_sgn_ND = 1, TC_sgn_MND = 1;
constexpr int TC_flr_ND = TC_NODE, TC_flr_MND = TC_MNODE;
constexpr int TC_cls_ND = TC_NODE, TC_cls_MND = TC_MNODE;
constexpr int TC_aux_ND = TC_NODE, TC_aux_MND = TC_MNODE;
constexpr int TC_hdr_ND = TC_NODE, TC_hdr_MND = TC_MNODE;
#include "MOD/tsvcomp-aux_h.inc"
#include "MOD/tsvcomp-cls_h.inc"
#include "MOD/tsvcomp-dig_h.inc"
#include "MOD/tsvcomp-flr_h.inc"
#include "MOD/tsvcomp-hdr_h.inc"
#include "MOD/tsvcomp-sgn_h.inc"
static TC_dig_T *tcm_dig;
static TC_sgn_T *tcm_sgn;
static TC_flr_T *tcm_flr;
static TC_cls_T *tcm_cls;
static TC_aux_T *tcm_aux;
static TC_hdr_T *tcm_hdr;
struct tc_dv {
  int rno, pass, band, col, vpos, q1, q2, q1s, q2s, t1, t1s, t2, n1, w1, p0, p0s, pn, cls, zrun, blk, bkq, ax, mg, pq, sq, ps, chn, crun, cm, x1, x1s;
};
struct tcx {
  int a, b, c, d, s, f, m, t, n, g;
};
#include "MOD/tsvcomp-aux_p.inc"
#include "MOD/tsvcomp-cls_p.inc"
#include "MOD/tsvcomp-dig_p.inc"
#include "MOD/tsvcomp-flr_p.inc"
#include "MOD/tsvcomp-hdr_p.inc"
#include "MOD/tsvcomp-sgn_p.inc"
enum { STG_HDR, STG_PAGE, STG_AUD, STG_FLOOR, STG_CLASS, STG_DIGIT, STG_N };
static const char *const TC_STAGE[STG_N] = {"headers", "pages", "packets", "floor", "class", "digits"};
static double tc_bits[STG_N];
static u64 tc_syms[STG_N];
static int tc_stage, tc_verbose;
static INLINE int tc_bit(int p, int bit) {
  uint b = (uint)bit;
  if(tc_verbose && tc_enc)
    tc_bits[tc_stage] -= log2(bit ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
  if(tc_enc)
    rce.rc_Process((uint)(p << 3), b);
  else
    rcd.rc_Process((uint)(p << 3), b);
  return (int)b;
}
struct tc_fam {
  cm_cnt *A, *B, *C, *D, *G, *T;
  i16 *S, *S2;
  i32 *W;
  cm_apm ap, ap2;
  cm_mix<TC_MIXN> mx;
  cm_mix<3> mxm;
  tc_pcur pc;
  int prp;
  sz ba, bb, bc, bd;
  u32 bs, bf, bm, bt, bn, bg;
  int rA, rB, rC, rD, rT, rG;
  int mwA, mwB, mwC, mwD, mwT, mwG;
  int rS1, rS2;
  int lr, mb, lrm, mbm;
  int bw;
  int ng;
  int nd, nm;
  void wire(cm_cnt *a, sz va, cm_cnt *b, sz vb, cm_cnt *c, sz vc, cm_cnt *d, sz vd, cm_cnt *t, sz vt, i16 *s, u8 *sc, sz vs, i16 *s2, u8 *sc2, sz vf, i32 *w, sz vm, cm_cnt *g, sz vg, sz vgi, i32 *wm, sz vn, u8 *wc, u8 *wmc, int ra, int rb, int rc_, int rd, int rt, int rg, int mwa, int mwb, int mwc, int mwd, int mwtt, int mwg, int rs1, int rs2, int lrate, int mbt, int lrm2, int mbm2, int bwt, int qqs, int qqf, int ndp, int nmp) {
    A = a;
    B = b;
    C = c;
    D = d;
    T = t;
    S = s;
    S2 = s2;
    W = w;
    G = g;
    ng = (int)vg;
    nd = ndp;
    nm = nmp;
    tc_vfits("counter A", va, nd);
    tc_vfits("counter B", vb, nd);
    tc_vfits("counter C", vc, nd);
    tc_vfits("counter D", vd, nd);
    tc_vfits("counter T", vt, nm);
    if(G)
      tc_vfits("counter G", vg, vgi);
    tc_ifits("APM 1", vs, (sz)nd * qqs);
    tc_ifits("APM 2", vf, (sz)nd * qqf);
    tc_ifits("mixer", vm, (sz)nd * TC_MIXN);
    tc_ifits("mantissa mixer", vn, (sz)nm * 3);
    (void)sc;
    (void)sc2;
    (void)wc;
    (void)wmc;
    ap.init(S, sc, vs * nd, qqs);
    ap2.init(S2, sc2, vf * nd, qqf);
    mx.init(W, wc, vm * nd);
    mxm.init(wm, wmc, vn * nm);
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rT = tc_clamp(rt, 1, CM_TMAX);
    rG = tc_clamp(rg, 1, CM_TMAX);
    mwA = tc_clamp(mwa * 64, 0, CM_CONE / 4);
    mwB = tc_clamp(mwb * 64, 0, CM_CONE / 4);
    mwC = tc_clamp(mwc * 64, 0, CM_CONE / 4);
    mwD = tc_clamp(mwd * 64, 0, CM_CONE / 4);
    mwT = tc_clamp(mwtt * 64, 0, CM_CONE / 4);
    mwG = tc_clamp(mwg * 64, 0, CM_CONE / 4);
    rS1 = cm_apm::limit(tc_clamp(rs1, 1, 15));
    rS2 = cm_apm::limit(tc_clamp(rs2, 1, 15));
    lr = tc_clamp(lrate, 1, 64);
    mb = tc_clamp(mbt, 0, 255);
    lrm = tc_clamp(lrm2, 1, 64);
    mbm = tc_clamp(mbm2, 0, 255);
    bw = tc_clamp(bwt, 0, 16);
    pc.off();
    prp = -1;
  }
  INLINE void select(const tcx &x) {
    ba = (sz)x.a * nd;
    bb = (sz)x.b * nd;
    bc = (sz)x.c * nd;
    bd = (sz)x.d * nd;
    bs = (u32)x.s * nd;
    bf = (u32)x.f * nd;
    bm = (u32)x.m * nd;
    bt = (u32)x.t * nm;
    bn = (u32)x.n * nm;
    bg = (u32)x.g;
    __builtin_prefetch(A + ba);
    __builtin_prefetch(B + bb);
    __builtin_prefetch(C + bc);
    __builtin_prefetch(D + bd);
    __builtin_prefetch(ap.t + (sz)bs * (u32)ap.q);
    __builtin_prefetch(ap.c + (sz)bs * (u32)ap.q);
    __builtin_prefetch(mx.w + (sz)bm * TC_MIXN);
    __builtin_prefetch(mx.c + bm);
    __builtin_prefetch(ap2.t + (sz)bf * (u32)ap2.q);
    __builtin_prefetch(ap2.c + (sz)bf * (u32)ap2.q);
  }
  INLINE int bit(int node, int b) {
    cm_cnt a = A[ba + node];
    cm_cnt c = B[bb + node];
    cm_cnt e = C[bc + node];
    cm_cnt f = D[bd + node];
    int pa = a.P();
    int p1 = ap.pp(pa, bs + (u32)node);
    if(p1 > CM_PONE - 1)
      p1 = CM_PONE - 1;
    mx.set(0, cm_stretch(p1));
    mx.set(1, cm_stretch(c.P()));
    mx.set(2, cm_stretch(e.P()));
    mx.set(3, cm_stretch(f.P()));
    mx.set(4, cm_stretch(pa));
    mx.set(5, prp >= 0 ? cm_stretch(prp) : 0);
    mx.set(6, 256);
    {
      int pm = mx.mix(bm + (u32)node);
      int pf = ap2.pp(pm, bf + (u32)node);
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b);
    }
    mx.upd(b, lr, mb);
    ap.upd(b, rS1);
    ap2.upd(b, rS2);
    a.upd(b, rA, mwA);
    A[ba + node] = a;
    c.upd(b, rB, mwB);
    B[bb + node] = c;
    e.upd(b, rC, mwC);
    C[bc + node] = e;
    f.upd(b, rD, mwD);
    D[bd + node] = f;
    return b;
  }
  INLINE int bitm(int node, int b) {
    cm_cnt t = T[bt + node];
    if(prp < 0) {
      b = tc_bit(t.P(), b);
      t.upd(b, rT, mwT);
      T[bt + node] = t;
      return b;
    }
    mxm.set(0, cm_stretch(t.P()));
    mxm.set(1, cm_stretch(prp));
    mxm.set(2, 256);
    {
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
    }
    mxm.upd(b, lrm, mbm);
    t.upd(b, rT, mwT);
    T[bt + node] = t;
    return b;
  }
  i64 code(const tcx &v, i64 x) {
    int k, p, nb = 0;
    u64 u;
    select(v);
    if(UNLIKELY(tc_enc && x < 0))
      FATAL("negative magnitude");
    prp = tcp_head(pc, 0);
    if(bit(0, tc_enc && x == 0)) {
      prp = -1;
      return 0;
    }
    prp = tcp_head(pc, 1);
    if(bit(1, tc_enc && x == 1)) {
      prp = -1;
      return 1;
    }
    prp = tcp_head(pc, 2);
    if(bit(2, tc_enc && x == 2)) {
      prp = -1;
      return 2;
    }
    u = tc_enc ? (u64)(x - 3) + 1 : 1;
    if(tc_enc) {
      u64 t = u;
      while(t) {
        nb++;
        t >>= 1;
      }
    }
    for(k = 0;; k++) {
      prp = tcp_len(pc, k);
      if(bit(tc_node_len(k), tc_enc && nb == k + 1)) {
        nb = k + 1;
        break;
      }
      if(UNLIKELY(k + 2 > TC_NBMAX))
        FATAL("%s: a value of 2^%d or more", tc_enc ? "this stream holds" : "coded stream", TC_NBMAX);
    }
    {
      u64 pref = 1;
      for(p = 0; p < nb - 1; p++) {
        int mb;
        prp = tcp_man(pc, nb, p, pref);
        mb = bitm(tc_node_man(nb, p), tc_enc && ((u >> (nb - 2 - p)) & 1) != 0);
        pref = pref * 2 + (u64)mb;
      }
      prp = -1;
      if(!tc_enc)
        u = pref;
    }
    return (i64)(u - 1) + 3;
  }
  i64 codes(const tcx &v, i64 x, u32 sc) {
    i64 m = code(v, tc_enc ? (x < 0 ? -x : x) : 0);
    int s;
    if(!m)
      return 0;
    if(G) {
      cm_cnt g = G[bg * (u32)ng + sc];
      s = tc_bit(g.P(), tc_enc && x < 0);
      g.upd(s, rG, mwG);
      G[bg * (u32)ng + sc] = g;
    } else {
      FATAL_CODE(OGC_EXIT_IO, "a family with no sign counter coded a sign");
    }
    return s ? -m : m;
  }
};
static tc_fam fam_dig, fam_sgn, fam_flr, fam_cls, fam_aux, fam_hdr;
static i64 tc_last[F_N], tc_last2[F_N];
static i64 tc_auxc(int fld, i32 c, i64 x, int sgn) {
  tcx v;
  i64 r;
  tc_make_aux(fld, c, tc_qlog(tc_last[fld]), v);
  r = sgn ? fam_aux.codes(v, x, (u32)fld) : fam_aux.code(v, x);
  tc_last2[fld] = tc_last[fld];
  tc_last[fld] = r;
  return r;
}
static i64 tc_aux(int fld, i64 x) { return tc_auxc(fld, tc_qlog(tc_last2[fld]), x, 0); }
constexpr sz TC_SPANMAX = 1u << 17;
constexpr sz TC_HISTMAX = 1u << 23;
static i16 *dg_hist[VB_MAXRES];
static i16 *dg_hist2[VB_MAXRES];
static u32 *dg_avg[VB_MAXRES];
static i16 *dg_pp[VB_MAXRES];
static i16 *dg_ps[VB_MAXRES];
static u8 *dg_pn[VB_MAXRES];
static u8 *cl_hist[VB_MAXRES];
static sz dg_span[VB_MAXRES], cl_np[VB_MAXRES];
static i32 *dg_q1, *dg_q2, *dg_zr;
static i32 cl_last[VB_MAXRES][VB_MAXCH], cl_last2[VB_MAXRES][VB_MAXCH];
static i16 *fl_hist;
static i16 *fl_yhist;
static i32 *fl_who;
static u32 tc_blk;
static u32 tc_nchan;
static void tc_hist_free(void) {
  u32 i;
  for(i = 0; i < VB_MAXRES; i++) {
    free(dg_hist[i]);
    dg_hist[i] = nullptr;
    free(dg_hist2[i]);
    dg_hist2[i] = nullptr;
    free(dg_avg[i]);
    dg_avg[i] = nullptr;
    free(dg_pp[i]);
    dg_pp[i] = nullptr;
    free(dg_ps[i]);
    dg_ps[i] = nullptr;
    free(dg_pn[i]);
    dg_pn[i] = nullptr;
    free(cl_hist[i]);
    cl_hist[i] = nullptr;
    dg_span[i] = cl_np[i] = 0;
  }
  free(dg_q1);
  free(dg_q2);
  free(dg_zr);
  free(fl_hist);
  free(fl_yhist);
  free(fl_who);
  dg_q1 = dg_q2 = dg_zr = nullptr;
  fl_hist = nullptr;
  fl_yhist = nullptr;
  fl_who = nullptr;
}
struct oc_floor {
  u8 pcl[VB_MAXPOST], ppos[VB_MAXPOST];
  u8 rnk[VB_MAXPOST];
  u8 lo[VB_MAXPOST], hi[VB_MAXPOST];
};
static oc_floor oc_fl[VB_MAXFLOOR];
static void oc_floor_tables(const vb_floor *f, oc_floor *d) {
  u32 i, j, k, posts = 2;
  memset(d, 0, sizeof *d);
  for(i = 0; i < f->parts; i++)
    for(j = 0; j < f->cdim[f->pcls[i]]; j++) {
      if(posts >= VB_MAXPOST)
        break;
      d->pcl[posts] = f->pcls[i];
      d->ppos[posts] = (u8)j;
      posts++;
    }
  for(i = 0; i < f->posts; i++)
    d->rnk[f->srt[i]] = (u8)i;
  for(i = 0; i + 2 < f->posts; i++) {
    u32 l = 0, h = 1, lx = 0, hx = f->x[1], cx = f->x[i + 2];
    for(k = 0; k < i + 2; k++) {
      if(f->x[k] > lx && f->x[k] < cx) {
        l = k;
        lx = f->x[k];
      }
      if(f->x[k] < hx && f->x[k] > cx) {
        h = k;
        hx = f->x[k];
      }
    }
    d->lo[i] = (u8)l;
    d->hi[i] = (u8)h;
  }
}
static void tc_setup_done(void) {
  const vb_setup *s = oc_v->cur;
  u32 i;
  tc_hist_free();
  tcp_build();
  tc_nchan = oc_v->i.ch;
  for(i = 0; i < s->nrs; i++) {
    const vb_res *r = s->rs + i;
    sz span = r->end > r->beg ? r->end - r->beg : 0;
    sz need;
    if(span > TC_SPANMAX)
      span = 0;
    need = (sz)2 * 8 * tc_nchan * span;
    if(need > TC_HISTMAX)
      span = 0;
    dg_span[i] = span;
    if(span) {
      dg_hist[i] = (i16 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(i16));
      dg_hist2[i] = (i16 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(i16));
      dg_avg[i] = (u32 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(u32));
      dg_pp[i] = (i16 *)tc_alloc((sz)tc_nchan * span * sizeof(i16));
      dg_ps[i] = (i16 *)tc_alloc((sz)tc_nchan * span * sizeof(i16));
      dg_pn[i] = (u8 *)tc_alloc((sz)tc_nchan * span);
    }
    cl_np[i] = span ? span / r->psz + 2 : 0;
    if(cl_np[i])
      cl_hist[i] = (u8 *)tc_alloc((sz)2 * tc_nchan * cl_np[i]);
    memset(cl_last[i], 0, sizeof cl_last[i]);
    memset(cl_last2[i], 0, sizeof cl_last2[i]);
  }
  dg_q1 = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_q2 = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_zr = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  fl_hist = (i16 *)tc_alloc((sz)2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_yhist = (i16 *)tc_alloc((sz)2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_who = (i32 *)tc_alloc((sz)2 * tc_nchan * sizeof(i32));
  for(i = 0; i < 2 * tc_nchan; i++)
    fl_who[i] = -1;
  for(i = 0; i < s->nfl; i++)
    oc_floor_tables(s->fl + i, oc_fl + i);
}
static i32 vd_render(i32 x0, i32 x1, i32 y0, i32 y1, i32 X) {
  i32 dy = y1 - y0, adx = x1 - x0;
  i32 ady = dy < 0 ? -dy : dy;
  i32 off;
  if(adx <= 0)
    return y0;
  off = (i32)(((i64)ady * (X - x0)) / adx);
  return dy < 0 ? y0 - off : y0 + off;
}
static i32 vd_fold(i32 y, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 d = y - pred;
  i32 a = d >= 0 ? 2 * d : -2 * d - 1;
  if(!d)
    return 0;
  if(a < room)
    return a;
  return hiroom > lowroom ? d + lowroom : hiroom - 1 - d;
}
static i32 vd_unfold(i32 val, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 y;
  if(!val)
    y = pred;
  else if(val >= room)
    y = hiroom > lowroom ? val - lowroom + pred : pred - val + hiroom - 1;
  else if(val & 1)
    y = pred - ((val + 1) >> 1);
  else
    y = pred + (val >> 1);
  return y < 0 ? 0 : y > quant - 1 ? quant - 1 : y;
}
constexpr sz OC_TAGMAX = 31;
static char tc_tagbuf[TC_MAXTAG * (OC_TAGMAX + 1)];
static const char *tc_tag[TC_MAXTAG];
static u32 tc_ntag;
static int tc_lasttag = -1;
static i64 tc_tlast[TC_MAXTAG], tc_tlast2[TC_MAXTAG];
static i64 tc_runpos;
static int tc_tagid(const char *tag) {
  u32 i;
  int id = -1;
  for(i = 0; i < tc_ntag; i++)
    if(!strcmp(tc_tag[i], tag)) {
      id = (int)i;
      break;
    }
  if(id < 0) {
    char *dst = tc_tagbuf + (sz)tc_ntag * (OC_TAGMAX + 1);
    FATAL_UNLESS(tc_ntag < TC_MAXTAG, "the reader asks for more than %d tags", TC_MAXTAG);
    snprintf(dst, OC_TAGMAX + 1, "%s", tag);
    tc_tag[tc_ntag] = dst;
    id = (int)tc_ntag++;
  }
  tc_runpos = (id == tc_lasttag) ? tc_runpos + 1 : 0;
  tc_lasttag = id;
  return id;
}
static i64 tc_hdrval(int id, i64 x) {
  tcx v;
  i64 pr = tc_tlast[id];
  tc_make_hdr(id, tc_qlog(tc_tlast[id]), tc_qlog(tc_tlast2[id]), tc_qlog(tc_runpos), (int)(tc_tlast[id] & 255), v);
  x = fam_hdr.codes(v, tc_enc ? x - pr : 0, (u32)id) + pr;
  tc_tlast2[id] = tc_tlast[id];
  tc_tlast[id] = x;
  return x;
}
static INLINE i32 tc_sq(i32 v) {
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
static INLINE u32 tc_gcd(u32 a, u32 b) {
  while(b) {
    u32 t = a % b;
    a = b;
    b = t;
  }
  return a;
}
static i32 cl_run[VB_MAXRES];
static u8 cl_same[1u << 20];
static u8 cl_run_buf[1u << 20];
static struct {
  u32 vch, np, pv, w, ilv;
  u8 cidx[VB_MAXCH];
} R;
static struct {
  sz span;
  i16 *hist, *hist2;
  u32 *avg;
  i16 *pp, *ps;
  u8 *pn;
  i32 *q1, *q2, *zr;
  u32 str;
  const vb_book *bk;
  const tc_ptab *pt;
  u32 rno, pss, j, pc, psz, cls, dim, ilv, i;
  u32 vmod, smod;
  i32 bkq, clrun;
  int clmatch;
} P;
static void oc_part_begin(u32 rno, u32 pss, u32 jv, u32 pc, const vb_res *r, const vb_book *bk, u32 bn, u32 cls) {
  u32 j = R.cidx[jv];
  sz sl = (sz)jv * R.w + pc;
  sz span = dg_span[rno];
  sz hb = ((sz)(tc_blk * 8 + pss) * tc_nchan + j) * span;
  sz base = (sz)((rno * 8 + pss) * tc_nchan + j);
  u32 dim = bk->dim, ilv = R.ilv;
  P.rno = rno;
  P.pss = pss;
  P.j = j;
  P.pc = pc;
  P.psz = r->psz;
  P.cls = cls;
  P.dim = dim;
  P.ilv = ilv;
  P.i = 0;
  P.vmod = 0;
  P.smod = ilv ? (u32)(((sz)pc * r->psz) % ilv) : 0;
  P.bkq = tc_qlog(bk->off);
  P.clrun = sl < sizeof cl_run_buf ? cl_run_buf[sl] : 0;
  P.clmatch = sl < sizeof cl_same ? cl_same[sl] : 0;
  P.span = span;
  P.hist = span ? dg_hist[rno] + hb : nullptr;
  P.hist2 = span ? dg_hist2[rno] + hb : nullptr;
  P.avg = span ? dg_avg[rno] + hb : nullptr;
  P.pp = span ? dg_pp[rno] + (sz)j * span : nullptr;
  P.ps = span ? dg_ps[rno] + (sz)j * span : nullptr;
  P.pn = span ? dg_pn[rno] + (sz)j * span : nullptr;
  P.q1 = dg_q1 + base;
  P.q2 = dg_q2 + base;
  P.zr = dg_zr + base;
  P.str = ilv && dim ? dim / tc_gcd(dim, ilv) * ilv : dim;
  P.bk = bk;
  P.pt = tcp_book[bn].ok ? &tcp_book[bn] : nullptr;
  tc_stage = STG_DIGIT;
  fam_dig.pc.off();
}
static i64 oc_digit(i64 dg) {
  u32 i = P.i++;
  sz slot = (sz)P.pc * P.psz + i;
  int ok = P.hist && slot < P.span;
  tcx v;
  tc_dv d;
  i32 t1 = ok ? P.hist[slot] : 0;
  i32 t2 = ok ? P.hist2[slot] : 0;
  i32 n1 = ok && slot + P.str < P.span ? P.hist[slot + P.str] : 0;
  i32 w1 = ok && slot >= P.str ? P.hist[slot - P.str] : 0;
  sz xs = P.ilv > 1 ? slot - P.smod + (P.smod ? P.smod - 1 : P.ilv - 1) : 0;
  i32 x1 = ok && P.ilv > 1 && xs < P.span ? P.hist[xs] : 0;
  i32 av = ok ? P.avg[slot] : 0;
  i32 p0 = ok ? P.pp[slot] : 0;
  i32 psum = ok ? P.ps[slot] : 0;
  i32 pnn = ok ? P.pn[slot] : 0;
  d.rno = (int)P.rno;
  d.pass = (int)P.pss;
  d.band = tc_qlog(P.pc);
  d.col = tc_qlog(i);
  d.vpos = (int)P.vmod;
  d.q1 = tc_qlog(*P.q1);
  d.q2 = tc_qlog(*P.q2);
  d.q1s = tc_sq(*P.q1);
  d.q2s = tc_sq(*P.q2);
  d.t1 = tc_qlog(t1);
  d.t1s = tc_sq(t1);
  d.t2 = tc_qlog(t2);
  d.n1 = tc_qlog(n1);
  d.w1 = tc_qlog(w1);
  d.p0 = tc_qlog(p0);
  d.p0s = tc_sq(p0);
  d.pn = pnn;
  d.x1 = tc_qlog(x1);
  d.x1s = tc_sq(x1);
  d.ps = tc_sq(psum);
  d.chn = (int)(P.ilv ? P.smod : P.j);
  d.crun = tc_qlog(P.clrun);
  d.cm = P.clmatch;
  d.cls = (int)P.cls;
  d.zrun = tc_qlog(*P.zr);
  d.blk = (int)tc_blk;
  d.bkq = P.bkq;
  d.pq = tcp_axis(fam_dig.pc);
  d.sq = 0;
  {
    i64 ex = (i64)(av >> 4) * TC_dig_avA + (i64)tc_abs(t1) * TC_dig_avT1 + (i64)tc_abs(t2) * TC_dig_avT2 + (i64)tc_abs(n1) * TC_dig_avN1 + (i64)tc_abs(w1) * TC_dig_avW1 + (i64)tc_abs(*P.q1) * TC_dig_avQ1 + (i64)tc_abs(*P.q2) * TC_dig_avQ2 + (i64)tc_abs(x1) * TC_dig_avX1;
    d.ax = tc_qlog((ex * TC_dig_avC) >> 12);
  }
  tc_make_dig(d, v);
  if(!tc_enc)
    dg = 0;
  {
    i64 mg = fam_dig.code(v, tc_enc ? (dg < 0 ? -dg : dg) : 0);
    if(mg) {
      tcx w;
      int sn;
      int spr = tcp_sign(fam_dig.pc, mg);
      d.mg = tc_qlog(mg);
      d.sq = tcp_bucket(spr);
      tc_make_sgn(d, w);
      fam_sgn.select(w);
      fam_sgn.prp = spr;
      sn = fam_sgn.bit(0, tc_enc && dg < 0);
      fam_sgn.prp = -1;
      dg = sn ? -mg : mg;
    } else
      dg = 0;
  }
  if(fam_dig.pc.on) {
    i64 gv = dg + (i64)P.bk->off;
    fam_dig.pc.step(P.bk->inv && gv >= 0 && gv < (i64)P.bk->base && P.bk->inv[gv] != (u32)-1 ? (int)P.bk->inv[gv] : -1);
  }
  if(ok) {
    i16 c = (i16)(dg < -32768 ? -32768 : dg > 32767 ? 32767 : dg);
    i32 a = tc_abs(c);
    P.hist2[slot] = P.hist[slot];
    P.hist[slot] = c;
    P.avg[slot] = (((u32)P.avg[slot] * (u32)TC_dig_avD) >> 8) + (u32)(a > 4095 ? 4095 : a) * 16;
    P.pp[slot] = c;
    P.pn[slot] = (u8)(pnn < 7 ? pnn + 1 : 7);
    {
      i32 t = psum + c;
      P.ps[slot] = (i16)(t < -32768 ? -32768 : t > 32767 ? 32767 : t);
    }
  }
  *P.q2 = *P.q1;
  *P.q1 = (i32)dg;
  *P.zr = dg ? 0 : *P.zr + 1;
  if(P.dim && ++P.vmod == P.dim)
    P.vmod = 0;
  if(P.ilv && ++P.smod == P.ilv)
    P.smod = 0;
  if(tc_verbose)
    tc_syms[STG_DIGIT]++;
  return dg;
}
static u32 tc_classify(u32 rno, u32 j, u32 slot, u32 cin) {
  tcx v;
  u8 *hist = cl_np[rno] ? cl_hist[rno] + ((sz)tc_blk * tc_nchan + j) * cl_np[rno] : nullptr;
  i32 t1 = (hist && slot < cl_np[rno]) ? hist[slot] : 0;
  i32 tn = (hist && slot + 1 < cl_np[rno]) ? hist[slot + 1] : 0;
  i64 c;
  tc_stage = STG_CLASS;
  tc_make_cls((int)rno, tc_qlog(slot), cl_last[rno][j], t1, (int)tc_blk, tn, cl_last2[rno][j], tcp_axis(fam_cls.pc), v);
  c = tc_enc ? (i64)cin : 0;
  c = fam_cls.code(v, c);
  FATAL_UNLESS(c >= 0 && c < 16, "coded stream: residue class %" PRId64, c);
  cl_run[rno] = (c == cl_last[rno][j]) ? cl_run[rno] + 1 : 0;
  if(hist && slot < cl_np[rno])
    hist[slot] = (u8)c;
  cl_last2[rno][j] = cl_last[rno][j];
  cl_last[rno][j] = (i32)c;
  if(tc_verbose)
    tc_syms[STG_CLASS]++;
  return (u32)c;
}
static void oc_residue_begin(u32 rno, const vb_res *r, const u8 *nz, u32 nch, u32 n) {
  const vb_book *cb = oc_v->cur->bk + r->cbook;
  u32 end, i, k, nc = 0;
  for(i = 0; i < nch; i++)
    if(nz[i])
      nc++;
  if(r->type == 2) {
    R.vch = 1;
    R.ilv = nch;
    R.cidx[0] = 0;
    end = ogc_min(r->end, n * nch);
  } else {
    R.vch = nc;
    R.ilv = 0;
    end = ogc_min(r->end, n);
    for(i = 0, k = 0; i < nch; i++)
      if(nz[i])
        R.cidx[k++] = (u8)i;
  }
  R.np = end > r->beg ? (end - r->beg) / r->psz : 0;
  R.pv = cb->dim;
  R.w = R.np + R.pv;
  FATAL_UNLESS((sz)R.vch * R.w <= sizeof cl_run_buf, "residue needs %" PRIu64 " classification slots, limit %" PRIu64, (u64)((sz)R.vch * R.w), (u64)sizeof cl_run_buf);
  memset(dg_q1 + (sz)rno * 8 * tc_nchan, 0, (sz)8 * tc_nchan * sizeof(i32));
  memset(dg_q2 + (sz)rno * 8 * tc_nchan, 0, (sz)8 * tc_nchan * sizeof(i32));
  memset(dg_zr + (sz)rno * 8 * tc_nchan, 0, (sz)8 * tc_nchan * sizeof(i32));
  if(dg_span[rno]) {
    memset(dg_pp[rno], 0, (sz)tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_ps[rno], 0, (sz)tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_pn[rno], 0, (sz)tc_nchan * dg_span[rno]);
  }
}
static u32 oc_cls(u32 rno, u32 jv, u32 pc, u32 k, u32 cin) {
  u32 j = R.cidx[jv];
  sz sl = (sz)jv * R.w + pc + k;
  u8 *ch = cl_np[rno] ? cl_hist[rno] + ((sz)tc_blk * tc_nchan + j) * cl_np[rno] : nullptr;
  i32 was = (ch && pc + k < cl_np[rno]) ? ch[pc + k] : -1;
  i32 run = cl_run[rno];
  u32 c;
  if(k == 0)
    fam_cls.pc.start(tcp_cls[rno].ok ? &tcp_cls[rno] : nullptr);
  c = tc_classify(rno, j, pc + k, cin);
  if(sl < sizeof cl_run_buf) {
    cl_run_buf[sl] = (u8)(run > 255 ? 255 : run);
    cl_same[sl] = (u8)(was == (i32)c);
  }
  fam_cls.pc.step((int)c);
  return c;
}
static u8 tc_used[VB_MAXCH];
static u8 oc_fno[VB_MAXCH];
static i16 fl_cur[VB_MAXPOST];
static i16 fl_fy[VB_MAXPOST];
static void oc_flr(u32 k, u32 fno, const vb_floor *f, u32 *y, int &used) {
  const oc_floor *fd = oc_fl + fno;
  sz row = (sz)tc_blk * tc_nchan + k;
  i16 *hp = fl_hist + row * VB_MAXPOST;
  i16 *hy = fl_yhist + row * VB_MAXPOST;
  int mine = fl_who[row] == (i32)fno;
  int hasn = k && tc_used[k - 1] && fl_who[row - 1] == (i32)oc_fno[k - 1];
  u32 u, p;
  i32 quant = (i32)f->quant;
  oc_fno[k] = (u8)fno;
  tc_stage = STG_AUD;
  u = (u32)tc_auxc(F_USED, tc_used[k], tc_enc ? (i64)used : 0, 0);
  FATAL_UNLESS(u < 2, "coded stream: flr.used is %u", u);
  tc_used[k] = (u8)u;
  used = (int)u;
  if(!u)
    return;
  tc_stage = STG_FLOOR;
  fam_flr.pc.off();
  if(tc_enc)
    for(p = 0; p < f->posts; p++)
      fl_cur[p] = (i16)y[p];
  for(p = 0; p < f->posts; p++) {
    tcx v;
    i64 yy;
    i32 pred = 0, room = 0, hl = 0, lov = 0, hiv = 0, ep = 0;
    i32 o1 = hasn ? hp[p - (sz)VB_MAXPOST] : 0;
    i32 od = hasn && mine ? hy[p - (sz)VB_MAXPOST] - hy[p] : 0;
    if(p >= 2) {
      u32 l = fd->lo[p - 2], h = fd->hi[p - 2];
      i32 hiroom, lowroom;
      u32 c = fd->pcl[p];
      if(!fd->ppos[p])
        fam_flr.pc.start(tcp_flr[fno][c].ok ? &tcp_flr[fno][c] : nullptr);
      pred = vd_render((i32)f->x[l], (i32)f->x[h], fl_fy[l], fl_fy[h], (i32)f->x[p]);
      if(pred < 0)
        pred = 0;
      if(pred > quant - 1)
        pred = quant - 1;
      hiroom = quant - pred;
      lowroom = pred;
      room = (hiroom < lowroom ? hiroom : lowroom) << 1;
      hl = hiroom > lowroom ? 1 : hiroom < lowroom ? 2 : 0;
      lov = fl_cur[l];
      hiv = fl_cur[h];
      ep = mine ? vd_fold(hy[p], pred, quant) : 0;
      if(ep < 0)
        ep = 0;
    }
    tc_make_flr((int)fno, (int)(fd->rnk[p] < VB_MAXPOST ? fd->rnk[p] : VB_MAXPOST - 1), tc_qlog(mine ? hp[p] : 0), tc_qlog(mine ? hy[p] : 0), p < 2, (int)tc_blk, tc_qlog(o1), tc_qlog(pred), tc_sq(od), (int)k, tc_qlog(room), hl, tc_qlog(lov), tc_qlog(hiv), tcp_axis(fam_flr.pc), tc_qlog(ep), v);
    yy = tc_enc ? fl_cur[p] : 0;
    yy = fam_flr.codes(v, yy, 0);
    FATAL_UNLESS(yy >= 0 && yy < 32768, "coded stream: floor post %" PRId64, yy);
    fl_cur[p] = (i16)yy;
    fl_fy[p] = (i16)(p < 2 ? (yy > quant - 1 ? quant - 1 : (i32)yy) : vd_unfold((i32)yy, pred, quant));
    if(fam_flr.pc.on)
      fam_flr.pc.step(yy >= 0 && yy < (i64)fam_flr.pc.t->nsym ? (int)yy : -1);
    if(tc_verbose)
      tc_syms[STG_FLOOR]++;
  }
  if(!tc_enc)
    for(p = 0; p < f->posts; p++)
      y[p] = (u32)fl_cur[p];
  for(p = 0; p < f->posts; p++) {
    hp[p] = fl_cur[p];
    hy[p] = fl_fy[p];
  }
  fl_who[row] = (i32)fno;
}
static u32 tc_prevW;
static u32 tc_prevmode;
static u32 oc_nextW, oc_wp;
static u32 oc_mode(u32 md, u32 nmd) {
  tc_stage = STG_AUD;
  md = (u32)tc_auxc(F_MODE, (i32)tc_prevmode, tc_enc ? (i64)md : 0, 0);
  FATAL_UNLESS(md < nmd, "coded stream: audio names mode %" PRIu64, (u64)md);
  tc_prevmode = md;
  tc_blk = oc_v->cur->blockflag[md];
  oc_nextW = tc_blk;
  if(!tc_blk)
    tc_prevW = 0;
  return md;
}
static u32 oc_wprev(u32 w) {
  w = (u32)tc_auxc(F_WPREV, (i32)tc_prevW, tc_enc ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wprev is %u", w);
  oc_wp = w;
  return w;
}
static u32 oc_wnext(u32 w) {
  w = (u32)tc_auxc(F_WNEXT, (i32)oc_wp, tc_enc ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wnext is %u", w);
  tc_prevW = oc_nextW;
  return w;
}
static i64 pg_prev;
static u32 pg_prevtype, pg_prevser;
static u32 oc_seq;
static void oc_page(ogg_page &p, int cont) {
  u32 i;
  i64 g;
  u32 seq = oc_seq++;
  int bos = seq == 0;
  tc_stage = STG_PAGE;
  {
    u32 want = (u32)((bos ? 2 : 0) | (cont ? 1 : 0));
    i64 d = tc_auxc(F_PGTYPE, (i32)(want * 4 + (pg_prevtype & 3)), tc_enc ? (i64)p.type - (i64)want : 0, 1);
    if(!tc_enc) {
      FATAL_UNLESS(d + (i64)want >= 0 && d + (i64)want < 0x100, "coded stream: page type %" PRId64, d + (i64)want);
      p.type = (u8)(d + (i64)want);
    }
    pg_prevtype = p.type;
  }
  g = tc_enc ? (i64)(((u64)p.ghi << 32) | p.glo) : 0;
  g = tc_auxc(F_GRAN, tc_qlog(tc_last2[F_GRAN]), tc_enc ? g - pg_prev : 0, 1) + pg_prev;
  pg_prev = g;
  if(!tc_enc) {
    p.glo = (u32)((u64)g & 0xFFFFFFFFu);
    p.ghi = (u32)((u64)g >> 32);
  }
  {
    i64 d = tc_auxc(F_SERIAL, bos, tc_enc ? (i64)p.serial - (i64)pg_prevser : 0, 1);
    if(!tc_enc)
      p.serial = (u32)((i64)pg_prevser + d);
    pg_prevser = p.serial;
  }
  {
    i64 d = tc_auxc(F_SEQ, 0, tc_enc ? (i64)p.seq - (i64)seq : 0, 1);
    if(!tc_enc)
      p.seq = (u32)((i64)seq + d);
  }
  {
    i64 np = tc_aux(F_NPKT, tc_enc ? (i64)p.np : 0);
    FATAL_UNLESS(np >= 0 && np <= OGG_MAXSEG, "coded stream: page holds %" PRId64 " packets", np);
    p.np = (int)np;
  }
  {
    i64 prev = 0;
    for(i = 0; i < (u32)p.np; i++) {
      i64 v = tc_auxc(F_PLEN, tc_qlog(prev), tc_enc ? (i64)p.plen[i] - prev : 0, 1) + prev;
      FATAL_UNLESS(v >= 0 && v <= (i64)OGG_MAXSEG * OGG_MAXSEG, "coded stream: packet length %" PRId64, v);
      p.plen[i] = (u32)v;
      prev = v;
    }
  }
  if(p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) {
    i64 v = tc_aux(F_TAIL, tc_enc ? (i64)p.tail : 0);
    FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page tail %" PRId64, v);
    p.tail = (int)v;
  } else
    p.tail = 0;
  if(!tc_enc)
    p.pack();
}
static u32 oc_link(u32 more) {
  i64 m;
  tc_stage = STG_PAGE;
  m = tc_aux(F_MORE, tc_enc ? (i64)more : 0);
  FATAL_UNLESS(m >= 0 && m < 2, "coded stream: link.more is %" PRId64, m);
  return (u32)m;
}
static void oc_link_begin(void) {
  pg_prev = 0;
  tc_prevW = 0;
  tc_prevmode = 0;
  pg_prevtype = 0;
  pg_prevser = 0;
  oc_seq = 0;
  memset(tc_used, 0, sizeof tc_used);
}
static u32 oc_spill(u32 n) {
  i64 ex;
  tc_stage = STG_PAGE;
  ex = tc_aux(F_SPILL, tc_enc ? (i64)n : 0);
  FATAL_UNLESS(ex >= 0 && ex <= (i64)MAXPAY * OGG_MAXSEG, "coded stream: page spill %" PRId64, ex);
  return (u32)ex;
}
static u32 oc_hdr(const char *tag, u32 x, u64 hi) {
  i64 v;
  tc_stage = STG_HDR;
  v = tc_hdrval(tc_tagid(tag), tc_enc ? (i64)x : 0);
  if(!tc_enc)
    FATAL_UNLESS(v >= 0 && (u64)v < hi, "coded stream: %s = %" PRId64 " is out of range", tag, v);
  return (u32)v;
}
#ifdef TC_MEMCOST
#if TC_MEMCOST + 0 <= 1
#undef TC_MEMCOST
#define TC_MEMCOST 10000
#endif
#endif
#define TC_WIRE(f, F, g, ng, rt, mwt, rg, mwg, lrmt, mbmt) (f).wire(tcm_##F->TC_##F##_A, TC_##F##_a_Volume, tcm_##F->TC_##F##_B, TC_##F##_b_Volume, tcm_##F->TC_##F##_C, TC_##F##_c_Volume, tcm_##F->TC_##F##_D, TC_##F##_d_Volume, tcm_##F->TC_##F##_MT, TC_##F##_t_Volume, tcm_##F->TC_##F##_S, tcm_##F->TC_##F##_SC, TC_##F##_s_Volume, tcm_##F->TC_##F##_F, tcm_##F->TC_##F##_FC, TC_##F##_f_Volume, tcm_##F->TC_##F##_W, TC_##F##_m_Volume, (g), (ng), TC_##F##_g_Volume, tcm_##F->TC_##F##_WM, TC_##F##_n_Volume, tcm_##F->TC_##F##_WC, tcm_##F->TC_##F##_WMC, TC_##F##_rA, TC_##F##_rB, TC_##F##_rC, TC_##F##_rD, (rt), (rg), TC_##F##_mwA, TC_##F##_mwB, TC_##F##_mwC, TC_##F##_mwD, (mwt), (mwg), TC_##F##_rS1, TC_##F##_rS2, TC_##F##_lr, TC_##F##_mb, (lrmt), (mbmt), TC_##F##_bw, TC_##F##_qs, TC_##F##_qf, TC_##F##_ND, TC_##F##_MND)
#define TC_WIRE_ALL(f, F, g, ng) TC_WIRE(f, F, g, ng, TC_##F##_rT, TC_##F##_mwT, TC_##F##_rG, TC_##F##_mwG, TC_##F##_lrm, TC_##F##_mbm)
#ifndef TC_MEMCAP
#define TC_MEMCAP ((u64)64 << 30)
#endif
static INLINE u64 tc_tables(void) { return (u64)tcm_dig->TC_dig_Size + (u64)tcm_sgn->TC_sgn_Size + (u64)tcm_flr->TC_flr_Size + (u64)tcm_cls->TC_cls_Size + (u64)tcm_aux->TC_aux_Size + (u64)tcm_hdr->TC_hdr_Size; }
#if !USE_NEW
static_assert(TC_dig_T::TC_dig_Size + TC_sgn_T::TC_sgn_Size + TC_flr_T::TC_flr_Size + TC_cls_T::TC_cls_Size + TC_aux_T::TC_aux_Size + TC_hdr_T::TC_hdr_Size <= TC_MEMCAP, "IDX/tsvcomp-*.idx ask for more model tables than TC_MEMCAP allows");
#endif
static void tc_models(void) {
  cm_tables();
#define TC_MAP(F) (tcm_##F = (TC_##F##_T *)tc_map(sizeof(TC_##F##_T)), tcm_##F->TC_##F##_Init())
  TC_MAP(dig);
  TC_MAP(sgn);
  TC_MAP(flr);
  TC_MAP(cls);
  TC_MAP(aux);
  TC_MAP(hdr);
#undef TC_MAP
  FATAL_UNLESS(tc_tables() <= TC_MEMCAP,
               "IDX/tsvcomp-*.idx asks for %" PRIu64 " MB of model tables, past "
               "the %" PRIu64 " MB this build allows -- narrow a context, or "
               "rebuild with a larger TC_MEMCAP",
               (u64)(tc_tables() >> 20), (u64)(TC_MEMCAP >> 20));
  TC_WIRE(fam_dig, dig, (cm_cnt *)nullptr, 0, TC_dig_rT, TC_dig_mwT, 0, 0, TC_dig_lrm, TC_dig_mbm);
  TC_WIRE(fam_sgn, sgn, (cm_cnt *)nullptr, 0, 0, 0, 0, 0, 0, 0);
  TC_WIRE_ALL(fam_flr, flr, tcm_flr->TC_flr_G, TC_SGN_FLR);
  TC_WIRE(fam_cls, cls, (cm_cnt *)nullptr, 0, TC_cls_rT, TC_cls_mwT, 0, 0, TC_cls_lrm, TC_cls_mbm);
  TC_WIRE_ALL(fam_aux, aux, tcm_aux->TC_aux_G, TC_SGN_AUX);
  TC_WIRE_ALL(fam_hdr, hdr, tcm_hdr->TC_hdr_G, TC_SGN_HDR);
}
static void tc_nomem(void) {
  FATAL_CODE(OGC_EXIT_IO, "out of memory -- IDX/tsvcomp-*.idx asks for more "
                          "than this machine has");
}
struct oc_model {
  u32 link(u32 more) { return oc_link(more); }
  void link_begin() { oc_link_begin(); }
  void page(ogg_page &p, int cont) { oc_page(p, cont); }
  u32 spill(u32 n) { return oc_spill(n); }
  u32 hdr(const char *tag, u32 x, u64 hi) { return oc_hdr(tag, x, hi); }
  void setup_done(vb_ctx &v) {
    oc_v = &v;
    tc_setup_done();
  }
  u32 mode(u32 md, u32 nmd) { return oc_mode(md, nmd); }
  u32 wprev(u32 w) { return oc_wprev(w); }
  u32 wnext(u32 w) { return oc_wnext(w); }
  void floor(u32 k, u32 fno, vb_floor *f, u32 *y, int &used) { oc_flr(k, fno, f, y, used); }
  void residue_begin(u32 rno, vb_res *r, const u8 *nz, u32 nch, u32 n) { oc_residue_begin(rno, r, nz, nch, n); }
  u32 cls(u32 rno, u32 j, u32 pc, u32 k, u32 c) { return oc_cls(rno, j, pc, k, c); }
  void part_begin(u32 rno, u32 pass, u32 j, u32 pc, vb_res *r, vb_book *bk, u32 bn, u32 c) { oc_part_begin(rno, pass, j, pc, r, bk, bn, c); }
  void vec_begin() { fam_dig.pc.start(P.pt); }
  i64 digit(i64 d) { return oc_digit(d); }
};
typedef oc_model sink_t;
constexpr u32 VB_MAXBLK = 8192;
constexpr u32 VB_MAXDIM = 0xFFFF;
constexpr sz VB_YSMAX = (sz)VB_MAXCH * VB_MAXPOST;
constexpr sz VB_CSMAX = (sz)VB_MAXCH * (VB_MAXBLK / 2 + VB_MAXDIM);
static u32 vb_ys[VB_YSMAX];
static u32 vb_cs[VB_CSMAX];
static u32 lk1(u32 entries, u32 dim) {
  u32 v = 0, i, p;
  for(;;) {
    p = 1;
    Fi(
        dim, if(p > entries / (v + 1)) {
          p = entries + 1;
          break;
        } p *= v + 1);
    if(p > entries)
      return v;
    v++;
  }
}
constexpr u32 QUANT[4] = {256, 128, 86, 64};
struct io {
  int enc;
  sink_t &t;
  vb_ctx &v;
  u8 *b;
  sz len, pos;
  io(vb_ctx &vv, sink_t &tt, int e, u8 *pkt, sz n) : enc(e), t(tt), v(vv), b(pkt), len(n), pos(0) {
    if(!e)
      memset(pkt, 0, n);
  }
  u32 bget(int n) {
    u32 r = 0;
    int i;
    FATAL_IF_HOT(pos + (sz)n > len * 8)("vorbis: packet ends inside a field");
    for(i = 0; i < n; i++, pos++)
      r |= (u32)((b[pos >> 3] >> (pos & 7)) & 1) << i;
    return r;
  }
  void bput(int n, u32 x) {
    int i;
    FATAL_IF_HOT(pos + (sz)n > len * 8)("vorbis: packet ends inside a field");
    for(i = 0; i < n; i++, pos++)
      b[pos >> 3] |= (u8)(((x >> i) & 1) << (pos & 7));
  }
  INLINE u32 bget1() {
    u32 r;
    FATAL_IF_HOT(pos >= len * 8)("vorbis: packet ends inside a codeword");
    r = (b[pos >> 3] >> (pos & 7)) & 1;
    pos++;
    return r;
  }
  INLINE void bput1(u32 bit) {
    FATAL_IF_HOT(pos >= len * 8)("vorbis: packet ends inside a codeword");
    b[pos >> 3] |= (u8)(bit << (pos & 7));
    pos++;
  }
  INLINE u32 val(const char *tag, u32 x, u64 hi) {
    if(!enc)
      return t.hdr(tag, 0, hi);
    FATAL_IF_HOT((u64)x >= hi)("vorbis: %s is out of range", tag);
    return t.hdr(tag, x, hi);
  }
  INLINE i32 digit(i32 x, i32 lim) {
    i64 r;
    if(enc) {
      FATAL_IF_HOT(x < -lim || x > lim)("vorbis: res.digit is out of range");
      return (i32)t.digit(x);
    }
    r = t.digit(0);
    FATAL_UNLESS(r >= -lim && r <= lim, "vorbis: res.digit is out of range");
    return (i32)r;
  }
  u32 fld(const char *tag, int n, int w) {
    u32 x;
    u64 hi = w >= 32 ? (u64)0x100000000 : (u64)1 << w;
    if(enc) {
      x = bget(n);
      FATAL_UNLESS(w >= 32 || !(x >> w), "vorbis: %s value %" PRIu64 " exceeds %d bits", tag, (u64)x, w);
      return t.hdr(tag, x, hi);
    }
    x = t.hdr(tag, 0, hi);
    bput(n, x);
    return x;
  }
  void cst(int n, u32 k) {
    if(enc) {
      u32 x = bget(n);
      FATAL_UNLESS(x == k, "vorbis: fixed field %" PRIu64 ", expected %" PRIu64, (u64)x, (u64)k);
    } else
      bput(n, k);
  }
  u32 flt(const char *ts, const char *te, const char *tm) {
    u32 w = enc ? bget(32) : 0, s, e, m;
    s = val(ts, enc ? w >> 31 : 0, 2);
    e = val(te, enc ? (w >> 21) & 0x3FF : 0, 0x400);
    m = val(tm, enc ? w & 0x1FFFFF : 0, 0x200000);
    w = (s << 31) | (e << 21) | m;
    if(!enc)
      bput(32, w);
    return w;
  }
  u32 bk_get(vb_book *bk) {
    u32 idx = 0;
    i32 nx;
    for(;;) {
      nx = bk->nd[2 * idx + bget1()];
      FATAL_IF_HOT(nx == 0)("vorbis: the packet holds no such codeword");
      if(nx < 0)
        return (u32)(-nx - 1);
      idx = (u32)nx;
    }
  }
  void bk_put(vb_book *bk, u32 e) {
    u32 k;
    FATAL_UNLESS(e < bk->ent && bk->len[e], "vorbis: codebook has no entry %" PRIu64, (u64)e);
    for(k = bk->len[e]; k > 0; k--)
      bput1((bk->code[e] >> (k - 1)) & 1);
  }
  void lengths(vb_book *k) {
    u32 i, got, cur, n, ent = k->ent;
    int used;
    if(fld("cb.ordered", 1, 1)) {
      got = 0;
      cur = fld("cb.ordlen", 5, 5) + 1;
      while(got < ent) {
        n = fld("cb.ordrun", (int)ogc_ilog(ent - got), (int)ogc_ilog(ent - got));
        FATAL_UNLESS(n <= ent - got, "vorbis: ordered run overruns its book");
        FATAL_UNLESS(cur <= 32, "vorbis: ordered lengths run past 32");
        for(i = got; i < got + n; i++)
          k->len[i] = (u8)cur;
        got += n;
        cur++;
      }
      return;
    }
    if(!fld("cb.sparse", 1, 1)) {
      Fi(ent, k->len[i] = (u8)(fld("cb.len", 5, 5) + 1));
      return;
    }
    used = (int)fld("cb.used", 1, 1);
    Fi(ent, if(i) used = (int)fld("cb.used", 1, 1); if(used) k->len[i] = (u8)(fld("cb.len", 5, 5) + 1));
  }
  void codebooks() {
    vb_setup *s = v.cur;
    u32 nb, j, lk, vb, i;
    vb_book *k;
    nb = fld("cb.count", 8, 8) + 1;
    FATAL_UNLESS(nb <= VB_MAXBOOK, "vorbis: %" PRIu64 " codebooks", (u64)nb);
    s->nbk = nb;
    Fj(nb, k = s->bk + j; cst(24, 0x564342); k->dim = fld("cb.dim", 16, 16); k->ent = fld("cb.entries", 24, 24); FATAL_UNLESS(k->dim > 0, "vorbis: codebook %" PRIu64 " has dimension 0", (u64)j); FATAL_UNLESS(k->ent <= ((u32)1 << 20), "vorbis: codebook %" PRIu64 " has %" PRIu64 " entries", (u64)j, (u64)k->ent); FATAL_UNLESS(ogc_ilog(k->dim) + ogc_ilog(k->ent) <= 24, "vorbis: codebook %" PRIu64 " size %" PRIu64 " x %" PRIu64 " is unsupported", (u64)j, (u64)k->dim, (u64)k->ent); k->len = vb_alloc<u8>(k->ent); memset(k->len, 0, k->ent); k->code = vb_alloc<u32>(k->ent); lengths(k); k->words(); k->tree(); lk = fld("cb.lookup", 4, 2); k->look = (u8)lk; if(!lk) continue; FATAL_UNLESS(lk == 1, "vorbis: unsupported codebook lookup type %" PRIu64, (u64)lk); flt("cb.min.sign", "cb.min.exp", "cb.min.mant"); flt("cb.dlt.sign", "cb.dlt.exp", "cb.dlt.mant"); vb = fld("cb.valbits", 4, 4) + 1; fld("cb.sequence", 1, 1); k->nv = lk1(k->ent, k->dim); k->mult = vb_alloc<u32>(k->nv ? k->nv : 1);
       Fi(k->nv, k->mult[i] = fld("cb.mult", (int)vb, (int)vb)); FATAL_UNLESS(k->nv > 0, "vorbis: codebook %" PRIu64 " has an empty lookup table", (u64)j); k->grid());
  }
  void floors() {
    vb_setup *s = v.cur;
    u32 nt, nf, k, i, j, nc, sub, sb;
    vb_floor *q;
    nt = fld("flr.ntime", 6, 6) + 1;
    Fi(nt, cst(16, 0));
    nf = fld("flr.count", 6, 6) + 1;
    FATAL_UNLESS(nf <= VB_MAXFLOOR, "vorbis: %" PRIu64 " floors", (u64)nf);
    s->nfl = nf;
    Fk(nf, q = s->fl + k; FATAL_UNLESS(fld("flr.type", 16, 1) == 1, "vorbis: floor type 0 is unsupported"); q->parts = fld("flr.parts", 5, 5); FATAL_UNLESS(q->parts <= VB_MAXPART, "vorbis: floor has %" PRIu64 " partitions", (u64)q->parts); nc = 0; Fi(q->parts, q->pcls[i] = (u8)fld("flr.pclass", 4, 4); FATAL_UNLESS(q->pcls[i] < VB_MAXCLASS, "vorbis: floor class %" PRIu64, (u64)q->pcls[i]); if((u32)q->pcls[i] + 1 > nc) nc = q->pcls[i] + 1); Fi(nc, q->cdim[i] = (u8)(fld("flr.cdim", 3, 3) + 1); sub = fld("flr.csub", 2, 2); q->csub[i] = (u8)sub; q->cbook[i] = -1; if(sub) q->cbook[i] = (i32)fld("flr.cbook", 8, 8); Fj((u32)1 << sub, sb = fld("flr.sbook", 8, 8); q->csb[i][j] = (i32)sb - 1)); q->mult = fld("flr.mult", 2, 2) + 1; q->quant = QUANT[q->mult - 1]; q->rng = fld("flr.rangebits", 4, 4); q->x[0] = 0; q->x[1] = (u32)1 << q->rng; q->posts = 2;
       Fi(q->parts, Fj(q->cdim[q->pcls[i]], FATAL_UNLESS(q->posts < VB_MAXPOST, "vorbis: floor has over %" PRIu64 " posts", (u64)VB_MAXPOST); q->x[q->posts++] = fld("flr.x", (int)q->rng, (int)q->rng))); q->sort());
  }
  void residues() {
    vb_setup *s = v.cur;
    u32 nr, k, i, j, bno, c;
    vb_res *q;
    nr = fld("res.count", 6, 6) + 1;
    FATAL_UNLESS(nr <= VB_MAXRES, "vorbis: %" PRIu64 " residues", (u64)nr);
    s->nrs = nr;
    Fk(nr, q = s->rs + k; q->type = fld("res.type", 16, 2); FATAL_UNLESS(q->type <= 2, "vorbis: residue type above 2"); q->beg = fld("res.begin", 24, 24); q->end = fld("res.end", 24, 24); q->psz = fld("res.partsize", 24, 24) + 1; q->ncl = fld("res.nclass", 6, 6) + 1; FATAL_UNLESS(q->ncl <= VB_MAXRCL, "vorbis: residue has %" PRIu64 " classifications", (u64)q->ncl); q->cbook = fld("res.classbook", 8, 8); Fi(
           q->ncl, c = 0; if(enc) {
             c = bget(3);
             if(bget(1))
               c |= bget(5) << 3;
           } c = val("res.cascade", c, 256);
           q->casc[i] = (u8)c; if(!enc) {
             bput(3, c & 7);
             bput(1, c >> 3 ? 1 : 0);
             if(c >> 3)
               bput(5, c >> 3);
           });
       Fi(q->ncl, Fj(8, q->book[i][j] = -1; if(!(q->casc[i] & ((u32)1 << j))) continue; bno = fld("res.book", 8, 8); q->book[i][j] = (i32)bno)));
  }
  void mappings(u32 ch) {
    vb_setup *s = v.cur;
    u32 nm, j, i, sub = 1, steps = 0, nmode, fl;
    vb_map *q;
    nm = fld("map.count", 6, 6) + 1;
    FATAL_UNLESS(nm <= VB_MAXMAP, "vorbis: %" PRIu64 " mappings", (u64)nm);
    s->nmp = nm;
    Fj(
        nm, q = s->mp + j; cst(16, 0); if(enc) {
          fl = bget(1);
          sub = fl ? bget(4) + 1 : 1;
          FATAL_UNLESS(!fl || sub > 1, "vorbis: invalid submap flag");
        } sub = val("map.submaps", enc ? sub - 1 : 0, 16) + 1;
        FATAL_UNLESS(sub >= 1 && sub <= VB_MAXSUB, "vorbis: mapping has %" PRIu64 " submaps", (u64)sub); if(!enc) {
          bput(1, sub > 1);
          if(sub > 1)
            bput(4, sub - 1);
        } if(enc) steps = bget(1) ? bget(8) + 1 : 0;
        steps = val("map.steps", steps, 257); if(!enc) {
          bput(1, steps != 0);
          if(steps)
            bput(8, steps - 1);
        } q->sub = sub;
        q->nstep = steps; FATAL_UNLESS(steps <= VB_MAXCH, "vorbis: %" PRIu64 " coupling steps", (u64)steps); Fi(steps, q->mag[i] = (u8)fld("map.mag", (int)ogc_ilog(ch - 1), (int)ogc_ilog(ch - 1)); q->ang[i] = (u8)fld("map.ang", (int)ogc_ilog(ch - 1), (int)ogc_ilog(ch - 1)); FATAL_UNLESS(q->mag[i] < ch && q->ang[i] < ch && q->mag[i] != q->ang[i], "vorbis: invalid coupling channels %d/%d of %" PRIu64, q->mag[i], q->ang[i], (u64)ch)); cst(2, 0); if(sub > 1) Fi(ch, q->mux[i] = (u8)fld("map.mux", 4, 4); FATAL_UNLESS(q->mux[i] < sub, "vorbis: channel %" PRIu64 " maps to submap %d of %" PRIu64, (u64)i, q->mux[i], (u64)sub)); Fi(sub, fld("map.smtime", 8, 8); q->fl[i] = (u8)fld("map.smfloor", 8, 8); q->rs[i] = (u8)fld("map.smres", 8, 8)));
    nmode = fld("mode.count", 6, 6) + 1;
    FATAL_UNLESS(nmode <= VB_MAXMODE, "vorbis: %" PRIu64 " modes", (u64)nmode);
    s->nmd = nmode;
    Fi(nmode, s->blockflag[i] = (u8)fld("mode.blockflag", 1, 1); cst(16, 0); cst(16, 0); s->mdmap[i] = (u8)fld("mode.mapping", 8, 8));
    cst(1, 1);
  }
  void ident() {
    vb_info *n = &v.i;
    u32 bs;
    FATAL_UNLESS(!enc || len < 8 || memcmp(b, "OpusHead", 8), "Ogg Opus is not supported");
    cst(8, 1);
    cst(8, 'v');
    cst(8, 'o');
    cst(8, 'r');
    cst(8, 'b');
    cst(8, 'i');
    cst(8, 's');
    cst(32, 0);
    n->ch = fld("id.channels", 8, 8);
    n->rate = fld("id.rate", 32, 32);
    fld("id.brmax", 32, 32);
    fld("id.brnom", 32, 32);
    fld("id.brmin", 32, 32);
    bs = fld("id.blocksize", 8, 8);
    n->bs0 = (u32)1 << (bs & 15);
    n->bs1 = (u32)1 << (bs >> 4);
    fld("id.framing", 8, 8);
    FATAL_UNLESS(n->ch >= 1, "vorbis: %" PRIu64 " channels", (u64)n->ch);
    FATAL_UNLESS(n->rate >= 1, "vorbis: sample rate 0");
    FATAL_UNLESS(n->bs0 >= 64 && n->bs1 >= n->bs0 && n->bs1 <= 8192, "vorbis: block sizes %" PRIu64 " and %" PRIu64, (u64)n->bs0, (u64)n->bs1);
  }
  void comment() {
    sz i;
    Fi(len, b[i] = (u8)t.hdr("cmt.byte", b[i], 256));
    pos = len * 8;
  }
  void setup() {
    vb_setup *s;
    cst(8, 5);
    cst(8, 'v');
    cst(8, 'o');
    cst(8, 'r');
    cst(8, 'b');
    cst(8, 'i');
    cst(8, 's');
    FATAL_UNLESS(!v.cur, "vorbis: a link has two setup headers");
    s = &vb_su;
    memset(s, 0, sizeof *s);
    v.cur = s;
    codebooks();
    floors();
    residues();
    mappings(v.i.ch);
    s->check();
  }
  void hdr(int which) {
    switch(which) {
    case 0:
      ident();
      break;
    case 1:
      comment();
      break;
    default:
      setup();
      t.setup_done(v);
      break;
    }
    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %" PRIu64 " unparsed packet bits", (u64)(len * 8 - pos));
    if(enc)
      FATAL_UNLESS(bget((int)(len * 8 - pos)) == 0, "vorbis: nonzero packet padding");
  }
  int fl_get(vb_floor *f, u32 *y) {
    vb_setup *s = v.cur;
    u32 i, k, j = 2, p, cd, cs, cv;
    if(!bget(1))
      return 0;
    p = ogc_ilog(f->quant - 1);
    y[0] = bget(p);
    y[1] = bget(p);
    Fi(f->parts, u32 c = f->pcls[i], cv0; cd = f->cdim[c]; cs = f->csub[c]; cv = 0; if(cs) cv = bk_get(s->bk + f->cbook[c]); cv0 = cv; Fk(cd, i32 sb = f->csb[c][cv & (((u32)1 << cs) - 1)]; cv >>= cs; y[j + k] = sb >= 0 ? bk_get(s->bk + sb) : 0); FATAL_UNLESS(!(cv0 >> (cs * cd)), "vorbis: floor class codeword exceeds its dimension"); Fk(
           cd, u32 l, d = (cv0 >> (k * cs)) & (((u32)1 << cs) - 1); for(l = 0; l < d; l++) {
             u32 mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
             FATAL_UNLESS(y[j + k] >= mx, "vorbis: floor value uses a later subclass book than "
                                          "the first that fits");
           });
       j += cd);
    return 1;
  }
  void fl_put(vb_floor *f, const u32 *y, int used) {
    vb_setup *s = v.cur;
    u32 i, k, l, j = 2, p, cd, cs, cv, mx;
    bput(1, (u32)used);
    if(!used)
      return;
    p = ogc_ilog(f->quant - 1);
    FATAL_UNLESS(y[0] < ((u32)1 << p) && y[1] < ((u32)1 << p), "vorbis: floor post does not fit %" PRIu64 " bits", (u64)p);
    bput(p, y[0]);
    bput(p, y[1]);
    Fi(
        f->parts, u32 c = f->pcls[i]; cd = f->cdim[c]; cs = f->csub[c]; cv = 0; if(cs) {
          Fk(
              cd,
              for(l = 0; l < ((u32)1 << cs); l++) {
                mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
                if(y[j + k] < mx)
                  break;
              } FATAL_UNLESS(l < ((u32)1 << cs), "vorbis: no class %" PRIu64 " book fits value %" PRIu64, (u64)c, (u64)y[j + k]);
              cv |= l << (k * cs));
          bk_put(s->bk + f->cbook[c], cv);
        } Fk(cd, i32 sb = f->csb[c][cv & (((u32)1 << cs) - 1)]; cv >>= cs; if(sb >= 0) bk_put(s->bk + sb, y[j + k]));
        j += cd);
  }
  INLINE void rs_sym(vb_book *bk) {
    u32 k, e = 0, np = 1, sym = 0;
    i32 d;
    if(enc) {
      e = bk_get(bk);
      sym = e;
    }
    t.vec_begin();
    Fk(
        bk->dim,
        if(enc) {
          d = (i32)bk->mult[sym % bk->nv] - (i32)bk->off;
          sym /= bk->nv;
          digit(d, RS_DIGIT_MAX);
        } else {
          u32 p;
          d = digit(0, RS_DIGIT_MAX);
          FATAL_IF_HOT(!(d + (i32)bk->off >= 0 && d + (i32)bk->off < (i32)bk->base))
          ("vorbis: residue digit %" PRId64 " outside codebook grid", (i64)d);
          p = bk->inv[(u32)(d + (i32)bk->off)];
          FATAL_IF_HOT(p == (u32)-1)
          ("vorbis: residue digit %" PRId64 " is not a codebook multiplicand", (i64)d);
          e += p * np;
          np *= bk->nv;
        });
    if(!enc) {
      FATAL_IF_HOT(e >= bk->ent)("vorbis: residue has no codebook entry");
      bk_put(bk, e);
    }
  }
  HOT FLATTEN void rs_part(vb_res *r, vb_book *bk) {
    u32 i, st;
    if(r->type == 0) {
      st = r->psz / bk->dim;
      FATAL_IF_HOT(st * bk->dim != r->psz)
      ("vorbis: residue 0 partition %" PRIu64 " not divisible by %" PRIu64, (u64)r->psz, (u64)bk->dim);
      Fi(st, rs_sym(bk));
    } else
      for(i = 0; i < r->psz; i += bk->dim)
        rs_sym(bk);
  }
  void residue(u32 rno, const u8 *nz, u32 nch, u32 n) {
    vb_setup *s = v.cur;
    vb_res *r = s->rs + rno;
    vb_book *cb = s->bk + r->cbook;
    u32 vch, end, np, pv, pass, pc, i, j, k, cw, w;
    u32 *cl;
    u32 nc = 0;
    Fi(nch, if(nz[i]) nc++);
    if(!nc)
      return;
    if(r->type == 2) {
      vch = 1;
      end = ogc_min(r->end, n * nch);
    } else {
      vch = nc;
      end = ogc_min(r->end, n);
    }
    if(end <= r->beg)
      return;
    np = (end - r->beg) / r->psz;
    if(!np)
      return;
    pv = cb->dim;
    w = np + pv;
    FATAL_UNLESS((sz)vch * w <= VB_CSMAX, "vorbis: residue needs %" PRIu64 " classification slots, limit %" PRIu64, (u64)((sz)vch * w), (u64)VB_CSMAX);
    cl = vb_cs;
    t.residue_begin(rno, r, nz, nch, n);
    for(pass = 0; pass < 8; pass++) {
      pc = 0;
      while(pc < np) {
        if(!pass)
          Fj(
              vch,
              if(enc) {
                cw = bk_get(cb);
                for(k = pv; k > 0; k--) {
                  cl[j * w + pc + k - 1] = cw % r->ncl;
                  cw /= r->ncl;
                }
                FATAL_UNLESS(!cw, "vorbis: residue classword exceeds its partitions");
              } Fk(pv, cl[j * w + pc + k] = t.cls(rno, j, pc, k, enc ? cl[j * w + pc + k] : 0));
              if(!enc) {
                cw = 0;
                Fk(pv, cw = cw * r->ncl + cl[j * w + pc + k]);
                bk_put(cb, cw);
              });
        Fk(pv, if(pc >= np) break; Fj(
               vch, i32 bn; FATAL_IF_HOT(cl[j * w + pc] >= r->ncl)("vorbis: residue class %" PRIu64 ", limit %" PRIu64, (u64)cl[j * w + pc], (u64)r->ncl); bn = r->book[cl[j * w + pc]][pass]; if(bn >= 0) {
                 t.part_begin(rno, pass, j, pc, r, s->bk + bn, (u32)bn, cl[j * w + pc]);
                 rs_part(r, s->bk + bn);
               });
           pc++);
      }
    }
  }
  void payload(u32 mode) {
    vb_setup *s = v.cur;
    vb_map *mp = s->mp + s->mdmap[mode];
    u32 n = (s->blockflag[mode] ? v.i.bs1 : v.i.bs0) / 2;
    u32 ch = v.i.ch, k, i, j, m;
    u8 nz[VB_MAXCH], sub[VB_MAXCH];
    u32 *y = vb_ys;
    memset(nz, 0, sizeof nz);
    Fk(ch, u32 fno = mp->fl[mp->mux[k]]; vb_floor *f = s->fl + fno; u32 *yc = y + k * VB_MAXPOST; int u = 0; if(enc) u = fl_get(f, yc); t.floor(k, fno, f, yc, u); nz[k] = (u8)u; if(!enc) fl_put(f, yc, u));
    Fi(mp->nstep, if(nz[mp->mag[i]] || nz[mp->ang[i]]) nz[mp->mag[i]] = nz[mp->ang[i]] = 1);
    Fi(mp->sub, m = 0; Fj(ch, if(mp->mux[j] == i) sub[m++] = nz[j]); if(m) residue(mp->rs[i], sub, m, n));
  }
  void audio() {
    u32 md;
    int d, w, i;
    FATAL_UNLESS(v.cur && v.cur->nmd > 0, "vorbis: audio before setup");
    d = (int)ogc_ilog(v.cur->nmd - 1);
    if(enc)
      FATAL_UNLESS(bget(1) == 0, "vorbis: header on audio path");
    else
      bput(1, 0);
    md = enc ? bget(d) : 0;
    md = t.mode(md, v.cur->nmd);
    if(!enc)
      bput(d, md);
    if(v.cur->blockflag[md])
      Fi(2, w = (int)(i ? t.wnext(enc ? bget(1) : 0) : t.wprev(enc ? bget(1) : 0)); if(!enc) bput(1, (u32)w));
    payload(md);
    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %" PRIu64 " unparsed audio bits", (u64)(len * 8 - pos));
    if(enc)
      FATAL_UNLESS(bget((int)(len * 8 - pos)) == 0, "vorbis: nonzero audio padding");
  }
};
constexpr sz IN_LOW = (sz)20 << 20;
constexpr sz IN_WIN = (sz)32 << 20;
typedef sz (*source_pull)(void *ctx, u8 *dst, sz n);
static u8 in_win[IN_WIN];
struct source {
  source_pull pull;
  void *pctx;
  const char *path;
  sz at, have;
  sz origin;
  int eof;
  void open(const char *pth, source_pull p, void *ctx) {
    pull = p;
    pctx = ctx;
    path = pth;
    at = have = origin = 0;
    eof = 0;
    want();
  }
  void close() { pull = nullptr; }
  void want() {
    sz got;
    if(avail() >= IN_LOW || eof)
      return;
    if(at) {
      memmove(in_win, in_win + at, have - at);
      origin += at;
      have -= at;
      at = 0;
    }
    while(have < IN_WIN) {
      got = pull(pctx, in_win + have, IN_WIN - have);
      if(!got) {
        eof = 1;
        break;
      }
      have += got;
    }
  }
  void skip(sz n) { at += n; }
  const u8 *cur() const { return in_win + at; }
  sz avail() const { return have - at; }
  sz offset() const { return origin + at; }
  sz join_len(sz got, const ogg_page &p, sz pl) const {
    const ogg_page *prev = &p;
    sz total = pl, k = at + got, n;
    ogg_page q;
    for(;;) {
      FATAL_UNLESS(!(prev->type & 4), "the packet at %" PRIu64 " exceeds its link", (u64)offset());
      n = q.parse(in_win + k, have - k);
      if(!n)
        FATAL("the packet at %" PRIu64 " has no continuation page", (u64)offset());
      FATAL_UNLESS(q.np > 0 && (q.type & 1), "the continuation of the packet at %" PRIu64 " is missing", (u64)offset());
      total += q.plen[0];
      FATAL_UNLESS(total - pl <= MAXPAY * (sz)OGG_MAXSEG, "the packet at %" PRIu64 " continues too far", (u64)offset());
      if(!(q.np == 1 && q.tail))
        return total;
      k += n;
      prev = &q;
    }
  }
  void join_pkt(sz got, const u8 *head, sz pl, sz total, u8 *dst) const {
    sz done = pl, k = at + got, n, take;
    ogg_page q;
    memcpy(dst, head, pl);
    while(done < total) {
      n = q.parse(in_win + k, have - k);
      if(!n)
        FATAL("the packet at %" PRIu64 " has no continuation page", (u64)offset());
      take = q.plen[0];
      if(take > total - done)
        take = total - done;
      memcpy(dst + done, in_win + k + OGG_HDRMIN + q.nseg, take);
      done += take;
      k += n;
    }
  }
};
constexpr sz JB_MAX = MAXPAY * (OGG_MAXSEG + 1);
static u8 jb[JB_MAX];
struct link_walk {
  int w;
  int cont;
  sz spill;
  void start() {
    w = 0;
    cont = 0;
    spill = 0;
  }
  void page(vb_ctx &v, sink_t &t, source &s, ogg_page &p, sz got) {
    const u8 *pbody = s.cur() + OGG_HDRMIN + p.nseg;
    u64 where = (u64)s.offset();
    sz at = 0;
    int j;
    t.page(p, cont);
    Fj(
        p.np, const u8 *pk = pbody + at; sz pl = p.plen[j]; sz off = at; at += pl; if(j == 0 && (p.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill,
                       "the page at %" PRIu64 " takes %" PRIu64 " of %" PRIu64 " remaining "
                       "bytes",
                       where, (u64)pl, (u64)spill);
          spill -= pl;
          if(!(p.np == 1 && p.tail)) {
            FATAL_UNLESS(!spill, "the page at %" PRIu64 " closes a packet %" PRIu64 " bytes early", where, (u64)spill);
            cont = 0;
          }
          continue;
        } FATAL_UNLESS(pl > 0, "the page at %" PRIu64 " carries an empty packet", where);
        if(j == p.np - 1 && p.tail) {
          sz tot = s.join_len(got, p, pl);
          s.join_pkt(got, pbody + off, pl, tot, jb);
          t.spill((u32)(tot - pl));
          spill = tot - pl;
          cont = 1;
          pk = jb;
          pl = tot;
        } io z(v, t, 1, (u8 *)pk, pl);
        if(pk[0] & 1) {
          FATAL_UNLESS(w < 3, "a link has over three header packets");
          z.hdr(w);
          w++;
        } else z.audio());
  }
};
static u8 pgbuf[PAGE_MAX];
static u8 body[MAXPAY];
typedef void (*page_writer)(void *ctx, const u8 *page, sz len);
void vb_pack(sink_t &t, source &src) {
  const char *in = src.path;
  vb_ctx v;
  link_walk lw;
  ogg_page p;
  sz got, npage = 0;
  int eos = 1;
  v.init();
  lw.start();
  for(;;) {
    src.want();
    if(!src.avail())
      break;
    got = p.parse(src.cur(), src.avail());
    FATAL_UNLESS(got != 0, "%s: no Ogg page at %" PRIu64, in, (u64)src.offset());
    FATAL_UNLESS(ogg_page::crc_ok(src.cur(), got), "%s: the page at %" PRIu64 " has a bad CRC", in, (u64)src.offset());
    if((p.type & 2) && eos) {
      t.link(1);
      v.link();
      lw.start();
      t.link_begin();
      eos = 0;
    }
    FATAL_UNLESS(!eos, "%s: page %" PRIu64 " %s", in, (u64)npage, npage ? "follows end of stream" : "precedes any bitstream");
    lw.page(v, t, src, p, got);
    if(p.type & 4) {
      FATAL_UNLESS(!lw.cont, "%s: a continued packet is %" PRIu64 " bytes short", in, (u64)lw.spill);
      eos = 1;
    }
    src.skip(got);
    npage++;
  }
  FATAL_UNLESS(npage > 0, "%s: not an Ogg bitstream", in);
  FATAL_UNLESS(eos, "%s: final bitstream has no end-of-stream page", in);
  t.link(0);
}
void vb_unpack(sink_t &t, const char *in, page_writer emit, void *ectx) {
  vb_ctx v;
  v.init();
  while(t.link(0)) {
    ogg_page q;
    sz jn = 0, spill = 0;
    int w = 0, done = 0, cont = 0;
    v.link();
    t.link_begin();
    while(!done) {
      sz at = 0, pgl;
      int j;
      t.page(q, cont);
      FATAL_UNLESS(q.blen <= MAXPAY, "%s: a page claims %" PRIu64 " payload bytes", in, (u64)q.blen);
      Fj(
          q.np, sz pl = q.plen[j]; if(j == 0 && (q.type & 1) && cont) {
            FATAL_UNLESS(pl <= spill, "%s: page takes %" PRIu64 " of %" PRIu64 " remaining bytes", in, (u64)pl, (u64)spill);
            if(pl)
              memcpy(body + at, jb + jn - spill, pl);
            spill -= pl;
            if(!(q.np == 1 && q.tail)) {
              FATAL_UNLESS(!spill, "%s: a page closes a packet %" PRIu64 " bytes early", in, (u64)spill);
              cont = 0;
            }
            at += pl;
            continue;
          } FATAL_UNLESS(pl > 0, "%s: a page claims an empty packet", in);
          if(j == q.np - 1 && q.tail) {
            sz ex = t.spill(0);
            sz tot = pl + ex;
            io z(v, t, 0, jb, tot);
            jn = tot;
            if(w < 3) {
              z.hdr(w);
              w++;
            } else
              z.audio();
            memcpy(body + at, jb, pl);
            spill = ex;
            cont = 1;
            at += pl;
            continue;
          } io z(v, t, 0, body + at, pl);
          if(w < 3) {
            z.hdr(w);
            w++;
          } else z.audio();
          at += pl);
      pgl = q.emit(pgbuf, body);
      emit(ectx, pgbuf, pgl);
      if(q.type & 4)
        done = 1;
    }
    FATAL_UNLESS(!cont, "%s: a continued packet is %" PRIu64 " bytes short", in, (u64)spill);
  }
}
constexpr char OC_MAGIC[] = "oggc\x1a";
constexpr int OC_VER = 2;
struct oc_coro : Coroutine {
  oc_model t;
  const char *in;
  static sz pull(void *ctx, u8 *dst, sz n) {
    coro3_pin &p = ((oc_coro *)ctx)->pin[0];
    for(;;) {
      sz have = (sz)(p.end - p.ptr);
      if(have) {
        if(have > n)
          have = n;
        memcpy(dst, p.ptr, have);
        p.ptr += have;
        return have;
      }
      if(p.f_quit())
        return 0;
      p.yield_r();
    }
  }
  static void push(void *ctx, const u8 *s, sz n) {
    coro3_pin &p = ((oc_coro *)ctx)->pin[1];
    while(n) {
      sz room = (sz)(p.end - p.ptr);
      if(!room) {
        p.yield_r();
        continue;
      }
      if(room > n)
        room = n;
      memcpy(p.ptr, s, room);
      p.ptr += room;
      s += room;
      n -= room;
    }
  }
  void do_process() {
    if(tc_enc) {
      source src;
      src.open(in, pull, this);
      rce.co = this;
      rce.rc_Init();
      vb_pack(t, src);
      rce.rc_Quit();
      if(rce.carry_lost())
        FATAL_CODE(OGC_EXIT_INTERNAL,
                   "%s: the carryless coder lost a carry; "
                   "this is a measuring build, ship one with OC_CARRYLESS=0",
                   in);
      src.close();
    } else {
      rcd.co = this;
      rcd.rc_Init();
      vb_unpack(t, in, push, this);
    }
    yield(this, 0);
  }
};
static CoroFileProc<oc_coro> oc_run;
int main(int argc, char **argv) {
  int a = 1;
  const char *mode;
  if(argc < 4)
    goto usage;
  mode = argv[a++];
  if((mode[0] != 'c' && mode[0] != 'd') || mode[1])
    goto usage;
  for(; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char *o = argv[a] + 1;
    for(; *o; o++)
      switch(*o) {
      case 'v':
        tc_verbose = 1;
        break;
      case 'H':
        tc_huge = 1;
        break;
      default:
        goto usage;
      }
  }
  if(argc - a != 2)
    goto usage;
  ogc_set_prog(argv[0]);
  ogc_paths_distinct(argv + a, 2);
  tc_enc = mode[0] == 'c';
  std::set_new_handler(tc_nomem);
  tc_models();
  {
    const char *in = argv[a], *out = argv[a + 1];
    filehandle f, g;
    if(!f.open(in))
      FATAL_CODE(OGC_EXIT_IO, "cannot open %s", in);
    if(tc_enc) {
      u8 h[2];
      if(!g.make(out))
        FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
      ogc_output(out);
      h[0] = (u8)OC_VER;
      h[1] = 0;
      if(g.writ((void *)OC_MAGIC, sizeof OC_MAGIC - 1) != sizeof OC_MAGIC - 1 || g.writ(h, 2) != 2)
        FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
    } else {
      char m[sizeof OC_MAGIC - 1];
      u8 h[2];
      FATAL_UNLESS(f.read(m, sizeof m) == sizeof m && !memcmp(m, OC_MAGIC, sizeof m), "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
      FATAL_UNLESS(f.read(h, 2) == 2, "%s: the stream ends in its header", in);
      FATAL_UNLESS(h[0] == OC_VER, "%s: made by oggcomp version %u, this is version %u", in, h[0], OC_VER);
      if(!g.make(out))
        FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
      ogc_output(out);
    }
    oc_run.in = in;
    oc_run.processfile(f, g);
    if(ferror(f.f))
      FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
    f.close();
#ifdef TC_MEMCOST
    if(tc_enc) {
      u64 n = tc_tables() / ((u64)1 << 30) * TC_MEMCOST + tc_tables() % ((u64)1 << 30) * TC_MEMCOST / ((u64)1 << 30);
      u64 i;
      for(i = 0; i < n; i++)
        fputc(0xFF, g.f);
      if(tc_verbose)
        fprintf(stderr, "%s: %" PRIu64 " MB of tables, %" PRIu64 " bytes of rent\n", ogc_prog, tc_tables() >> 20, n);
    }
#endif
    {
      int bad = ferror(g.f);
      if(g.close())
        bad = 1;
      if(bad)
        FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
    }
    ogc_output_kept();
  }
  if(tc_verbose) {
    int i;
    double tot = 0;
    for(i = 0; i < STG_N; i++)
      tot += tc_bits[i];
    fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " MB of tables\n", ogc_prog, tot / 8, (u64)(tc_tables() >> 20));
    if(tc_enc)
      for(i = 0; i < STG_N; i++) {
        fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i], tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
        if(tc_syms[i])
          fprintf(stderr, "  %" PRIu64 " values, %.3f bits each", tc_syms[i], tc_bits[i] / (double)tc_syms[i]);
        fputc('\n', stderr);
      }
  }
  tc_hist_free();
  tcp_free();
  return OGC_EXIT_OK;
usage:
  fprintf(stderr, "Lossless compressor of Ogg Vorbis files.\n"
                  "usage: oggcomp c [options] input.ogg output.oc\n"
                  "       oggcomp d [options] input.oc  restored.ogg\n"
                  "\n"
                  "  -v  say where the bits went, by stage\n"
                  "  -H  model tables on huge pages (Linux; faster on big files where the\n"
                  "      kernel has 2 MB pages ready, slower where it has to make them)\n"
                  "\n");
  return OGC_EXIT_USAGE;
}
