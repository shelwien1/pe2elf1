#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef int8_t i8;
typedef uint16_t u16; typedef int16_t i16;
typedef uint32_t u32; typedef int32_t i32;
typedef size_t sz;
#if defined(__GNUC__) && !defined(BLR_NO_ATTRS)
#define INLINE inline __attribute__((__always_inline__))
#define HOT __attribute__((__hot__))
#define FLATTEN __attribute__((__flatten__))
#define NORETURN __attribute__((__noreturn__))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define BLR_PRINTF(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define INLINE inline
#define HOT
#define FLATTEN
#define NORETURN
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define BLR_PRINTF(f, a)
#endif
static INLINE u32 blr_ilog(u32 v) {
#if defined(__GNUC__) && !defined(BLR_NO_ATTRS)
  return v ? 32 - (u32) __builtin_clz(v) : 0;
#else
  u32 n = 0;
  while (v) { n++; v >>= 1; }
  return n;
#endif
}
constexpr int BLR_EXIT_OK = 0;
constexpr int BLR_EXIT_REFUSED = 1;
constexpr int BLR_EXIT_USAGE = 2;
constexpr int BLR_EXIT_IO = 3;
constexpr int BLR_EXIT_INTERNAL = 4;
#define FATAL blr_fatal
#define FATAL_CODE blr_fatal_code
#define FATAL_UNLESS blr_fatal_unless
NORETURN void blr_fatal(const char * fmt, ...) BLR_PRINTF(1, 2);
NORETURN void blr_fatal_code(int code, const char * fmt, ...) BLR_PRINTF(2, 3);
void blr_fatal_unless(int cond, const char * fmt, ...) BLR_PRINTF(2, 3);
#define FATAL_IF_HOT(cond) if (cond) FATAL
#define Fi(n, ...) for (i = 0; i < (n); i++) { __VA_ARGS__; }
#define Fj(n, ...) for (j = 0; j < (n); j++) { __VA_ARGS__; }
#define Fk(n, ...) for (k = 0; k < (n); k++) { __VA_ARGS__; }
template <class T> constexpr const T & blr_min(const T & a, const T & b) {
  return a < b ? a : b;
}
template <class T> constexpr const T & blr_max(const T & a, const T & b) {
  return a > b ? a : b;
}
static void message(const char * fmt, va_list ap) BLR_PRINTF(1, 0);
static void message(const char * fmt, va_list ap) {
  fputs("ogg2tsv: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}
static NORETURN void die(int code) { exit(code); }
NORETURN void blr_fatal(const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); message(fmt, ap); va_end(ap);
  die(BLR_EXIT_REFUSED);
}
NORETURN void blr_fatal_code(int code, const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); message(fmt, ap); va_end(ap);
  die(code);
}
void blr_fatal_unless(int cond, const char * fmt, ...) {
  va_list ap;
  if (cond) return;
  va_start(ap, fmt); message(fmt, ap); va_end(ap);
  die(BLR_EXIT_REFUSED);
}
constexpr char TSV_MAGIC[] = "tag\tvalues";
constexpr sz TSV_OBUF = 1 << 20;
constexpr sz TSV_IBUF = 1 << 16;
constexpr sz TSV_TAGMAX = 31;
constexpr sz TSV_ROWMAX = 8192;
static char tsv_ib[TSV_IBUF];
static char tsv_ob[TSV_OBUF];
struct tsv {
  FILE * f;
  const char * path;
  int enc;
  unsigned long row;
  sz on;
  sz rl;
  const char * wtag;
  tsv * tee;
  sz ip, in;
  char rtag[TSV_TAGMAX + 1];
  int left;
  void create(const char * pth) {
    f = fopen(pth, "wb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", pth);
    path = pth; enc = 1; row = 0; on = 0; rl = 0; wtag = nullptr;
    emit(TSV_MAGIC, sizeof TSV_MAGIC - 1); emit("\n", 1);
  }
  void open(const char * pth) {
    const char * m = TSV_MAGIC;
    int c;
    f = fopen(pth, "rb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", pth);
    path = pth; enc = 0; row = 0; ip = in = 0; left = 0; rtag[0] = 0;
    tee = nullptr;
    while (*m) {
      FATAL_UNLESS(rd() == (u8) *m, "%s: not a ogg2tsv TSV stream", path);
      m++;
    }
    c = rd();
    if (c == '\r') c = rd();
    FATAL_UNLESS(c == '\n', "%s: not a ogg2tsv TSV stream", path);
  }
  void close() {
    if (enc) {
      if (wtag) emit("\n", 1);
      flush();
    }
    if (f) {
      int bad = ferror(f);
      if (fclose(f)) bad = 1;
      if (bad) FATAL_CODE(BLR_EXIT_IO, "%s on %s",
                          enc ? "write error" : "read error", path);
    }
    f = nullptr;
  }
  void put(const char * tag, long long v) {
    char n[24];
    int k = (int) sizeof n;
    unsigned long long a = v < 0 ? 0ULL - (unsigned long long) v
                                 : (unsigned long long) v;
    if (wtag && rl < TSV_ROWMAX && (tag == wtag || !strcmp(tag, wtag)))
      emit(" ", 1);
    else {
      sz tn = strlen(tag);
      FATAL_UNLESS(tn <= TSV_TAGMAX, "tag '%s' is too long", tag);
      if (wtag) emit("\n", 1);
      emit(tag, tn); emit("\t", 1);
      wtag = tag; rl = 0;
    }
    do { n[--k] = (char) ('0' + (a % 10)); a /= 10; } while (a);
    if (v < 0) n[--k] = '-';
    emit(n + k, sizeof n - (sz) k);
    rl += sizeof n - (sz) k + 1;
    row++;
  }
  long long get(const char * tag) {
    unsigned long long a = 0;
    int c, neg = 0, nd = 0;
    row++;
    if (!left) head(tag);
    FATAL_UNLESS(!strcmp(rtag, tag), "%s: record %lu is '%s', expected '%s'",
                 path, row, rtag, tag);
    c = rd();
    if (c == '-') { neg = 1; c = rd(); }
    while (c >= '0' && c <= '9') {
      FATAL_UNLESS(nd < 19, "%s: record %lu has a malformed value", path, row);
      a = a * 10 + (unsigned) (c - '0');
      nd++; c = rd();
    }
    FATAL_UNLESS(nd && a <= 9223372036854775807ULL,
                 "%s: record %lu has a malformed value", path, row);
    if (c == ' ') left = 1;
    else {
      if (c == '\r') c = rd();
      FATAL_UNLESS(c == '\n' || c < 0, "%s: record %lu has a malformed value",
                   path, row);
      left = 0;
    }
    {
      long long r = neg ? -(long long) a : (long long) a;
      if (tee) tee->put(tag, r);
      return r;
    }
  }
  u32 get_u(const char * tag, unsigned long long hi) {
    long long v = get(tag);
    FATAL_UNLESS(v >= 0 && (unsigned long long) v < hi,
                 "%s: record %lu: %s = %lld is out of range", path, row,
                 tag, v);
    return (u32) v;
  }
private:
  void flush() {
    if (!on) return;
    if (fwrite(tsv_ob, 1, on, f) != on)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
    on = 0;
  }
  void emit(const char * s, sz n) {
    if (on + n > TSV_OBUF) flush();
    memcpy(tsv_ob + on, s, n); on += n;
  }
  INLINE int rd() {
    if (ip == in) {
      in = fread(tsv_ib, 1, TSV_IBUF, f);
      ip = 0;
      if (!in) {
        if (ferror(f)) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
        return -1;
      }
    }
    return (u8) tsv_ib[ip++];
  }
  void head(const char * tag) {
    int c = rd();
    sz n = 0;
    FATAL_UNLESS(c >= 0, "%s: stream ends before record %lu (%s)", path, row, tag);
    while (c >= 0 && c != '\t' && c != '\n') {
      FATAL_UNLESS(n < TSV_TAGMAX, "%s: record %lu has an overlong tag", path, row);
      rtag[n++] = (char) c;
      c = rd();
    }
    FATAL_UNLESS(c == '\t', "%s: record %lu has no value column", path, row);
    rtag[n] = 0;
    left = 1;
  }
};
constexpr int OGG_HDRMIN = 27;
constexpr int OGG_MAXSEG = 255;
u32 ogg_crc(const u8 * d, sz n);
u32 ogg_crc_page(const u8 * p, sz n);
int ogg_crc_ok(const u8 * p, sz n);
void ogg_crc_set(u8 * p, sz n);
constexpr sz MAXPAY = (sz) OGG_MAXSEG * OGG_MAXSEG;
constexpr sz PAGE_MAX = OGG_HDRMIN + OGG_MAXSEG + MAXPAY;
static u32 CRC[256];
static void crcinit(void) {
  u32 i, j, c;
  if (CRC[1]) return;
  Fi(256,
    c = i << 24;
    Fj(8, c = (c & 0x80000000UL) ? (c << 1) ^ 0x04C11DB7UL : c << 1);
    CRC[i] = c);
}
static u32 crcrun(u32 c, const u8 * d, sz n) {
  sz i;
  Fi(n, c = (c << 8) ^ CRC[((c >> 24) ^ d[i]) & 0xFF]);
  return c;
}
static void wr32(u8 * b, u32 v) {
  b[0] = (u8) v; b[1] = (u8) (v >> 8); b[2] = (u8) (v >> 16); b[3] = (u8) (v >> 24);
}
static u32 rd32(const u8 * b) {
  return (u32) b[0] | ((u32) b[1] << 8) | ((u32) b[2] << 16) | ((u32) b[3] << 24);
}
struct ogg_page {
  u8 type;
  u32 glo, ghi;
  u32 serial, seq;
  int nseg; u8 lace[OGG_MAXSEG];
  int np; u32 plen[OGG_MAXSEG];
  int tail;
  sz blen;
  sz parse(const u8 * b, sz n) {
    sz i, hl;
    if (n < (sz) OGG_HDRMIN || memcmp(b, "OggS", 4) || b[4]) return 0;
    nseg = b[26]; hl = OGG_HDRMIN + (sz) nseg;
    if (n < hl) return 0;
    type = b[5]; glo = rd32(b + 6); ghi = rd32(b + 10);
    serial = rd32(b + 14); seq = rd32(b + 18);
    blen = 0;
    Fi((sz) nseg, lace[i] = b[OGG_HDRMIN + i]; blen += lace[i]);
    if (n < hl + blen) return 0;
    unpack();
    return hl + blen;
  }
  sz emit(u8 * out, const u8 * body) const {
    sz i, hl = OGG_HDRMIN + (sz) nseg;
    memcpy(out, "OggS", 4); out[4] = 0; out[5] = type;
    wr32(out + 6, glo); wr32(out + 10, ghi);
    wr32(out + 14, serial); wr32(out + 18, seq);
    out[26] = (u8) nseg;
    Fi((sz) nseg, out[OGG_HDRMIN + i] = lace[i]);
    memcpy(out + hl, body, blen);
    crc_set(out, hl + blen);
    return hl + blen;
  }
  void unpack() {
    sz i;
    u32 run = 0;
    np = 0; tail = 0;
    Fi((sz) nseg,
      run += lace[i];
      if (lace[i] != OGG_MAXSEG) { plen[np++] = run; run = 0; });
    if (nseg && lace[nseg - 1] == OGG_MAXSEG) { plen[np++] = run; tail = 1; }
  }
  void pack() {
    int i;
    sz n = 0;
    blen = 0;
    Fi(np,
      u32 v = plen[i];
      blen += v;
      while (v >= (u32) OGG_MAXSEG) {
        FATAL_UNLESS(n < (sz) OGG_MAXSEG, "page needs more than %d segments",
                     OGG_MAXSEG);
        lace[n++] = OGG_MAXSEG; v -= OGG_MAXSEG;
      }
      if (!(tail && i == np - 1)) {
        FATAL_UNLESS(n < (sz) OGG_MAXSEG, "page needs more than %d segments",
                     OGG_MAXSEG);
        lace[n++] = (u8) v;
      } else FATAL_UNLESS(!v, "continued packet has a partial segment"));
    nseg = (int) n;
  }
  void put(tsv & t) const {
    int i;
    t.put("page.type", type);
    t.put("page.granlo", glo);
    t.put("page.granhi", ghi);
    t.put("page.serial", serial);
    t.put("page.seq", seq);
    t.put("page.npkt", np);
    Fi(np, t.put("page.plen", plen[i]));
    if (np && !(plen[np - 1] % OGG_MAXSEG)) t.put("page.tail", tail);
  }
  void get(tsv & t) {
    int i;
    type = (u8) t.get_u("page.type", 0x100);
    glo = t.get_u("page.granlo", 0x100000000ULL);
    ghi = t.get_u("page.granhi", 0x100000000ULL);
    serial = t.get_u("page.serial", 0x100000000ULL);
    seq = t.get_u("page.seq", 0x100000000ULL);
    np = (int) t.get_u("page.npkt", OGG_MAXSEG + 1);
    Fi(np, plen[i] = t.get_u("page.plen",
                             (unsigned long long) OGG_MAXSEG * OGG_MAXSEG + 1));
    tail = 0;
    if (np && !(plen[np - 1] % OGG_MAXSEG)) tail = (int) t.get_u("page.tail", 2);
    pack();
  }
  static u32 crc_page(const u8 * p, sz n) {
    static const u8 z4[4] = { 0, 0, 0, 0 };
    u32 c;
    if (n < 26) return 0;
    crcinit();
    c = crcrun(0, p, 22);
    c = crcrun(c, z4, 4);
    return crcrun(c, p + 26, n - 26);
  }
  static int crc_ok(const u8 * p, sz n) {
    return n >= (sz) OGG_HDRMIN && rd32(p + 22) == crc_page(p, n);
  }
  static void crc_set(u8 * p, sz n) {
    FATAL_UNLESS(n >= (sz) OGG_HDRMIN, "cannot checksum a short Ogg page");
    wr32(p + 22, crc_page(p, n));
  }
};
struct vb_info {
  u32 ch, rate;
  u32 bs0, bs1;
};
constexpr u32 VB_MAXBOOK = 256;
constexpr sz VB_ARENA = 96UL << 20;
static u8 vb_arena[VB_ARENA];
static sz vb_used;
template <class T> static T * vb_alloc(sz n) {
  sz want = (n * sizeof(T) + 7) & ~(sz) 7;
  u8 * p = vb_arena + vb_used;
  FATAL_UNLESS(want <= VB_ARENA - vb_used,
               "vorbis: the setup needs more than the %lu MiB codebook arena",
               (unsigned long) (VB_ARENA >> 20));
  vb_used += want;
  return (T *) p;
}
struct vb_book {
  u32 dim, ent;
  u8 look;
  u8 * len;
  u32 * code;
  i32 * nd;
  u32 nv;
  u32 * mult;
  u32 base, off;
  u32 * inv;
  void words() {
    u32 mk[33], i, j, e, l;
    memset(mk, 0, sizeof mk);
    Fi(ent,
      l = len[i];
      if (!l) continue;
      e = mk[l];
      FATAL_UNLESS(l >= 32 || !(e >> l), "vorbis: over-populated codebook");
      code[i] = e;
      for (j = l; j > 0; j--) {
        if (mk[j] & 1) { mk[j] = j == 1 ? mk[1] + 1 : mk[j - 1] << 1; break; }
        mk[j]++;
      }
      for (j = l + 1; j < 33; j++) {
        if ((mk[j] >> 1) != e) break;
        e = mk[j]; mk[j] = mk[j - 1] << 1;
      });
  }
  void tree() {
    u32 i, k, n = 1, u = 0, idx, cap;
    i32 t;
    Fi(ent, if (len[i]) u++);
    cap = u + 2;
    nd = vb_alloc<i32>(2 * cap);
    memset(nd, 0, 2 * cap * sizeof *nd);
    Fi(ent,
      if (!len[i]) continue;
      idx = 0;
      for (k = len[i]; k > 1; k--) {
        u32 c = 2 * idx + ((code[i] >> (k - 1)) & 1);
        t = nd[c];
        if (!t) { FATAL_UNLESS(n < cap, "vorbis: codebook tree overflows");
                  t = (i32) n++; nd[c] = t; }
        FATAL_UNLESS(t > 0, "vorbis: codebook is not prefix free");
        idx = (u32) t;
      }
      idx = 2 * idx + (code[i] & 1);
      FATAL_UNLESS(!nd[idx], "vorbis: codebook is not prefix free");
      nd[idx] = -(i32) (i + 1));
  }
  void grid() {
    u32 i, mx = 0, np;
    Fi(nv, if (mult[i] > mx) mx = mult[i]);
    base = mx + 1; off = base >> 1;
    inv = vb_alloc<u32>(base);
    Fi(base, inv[i] = (u32) -1);
    Fi(nv,
      FATAL_UNLESS(inv[mult[i]] == (u32) -1,
                   "vorbis: duplicate codebook multiplicand %lu",
                   (unsigned long) mult[i]);
      inv[mult[i]] = i);
    np = 1;
    if (nv > 1) Fi(dim, np *= nv);
    Fi(ent, if (len[i] && i >= np)
                 FATAL("vorbis: codebook entry %lu exceeds lookup grid %lu",
                       (unsigned long) i,
                       (unsigned long) np));
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
    Fi(posts,
      srt[i] = (u8) i;
      for (j = i; j > 0 && x[srt[j - 1]] > x[srt[j]]; j--) {
        t = srt[j]; srt[j] = srt[j - 1]; srt[j - 1] = (u8) t;
      });
    Fi(posts, if (i) FATAL_UNLESS(x[srt[i]] != x[srt[i - 1]],
                                     "vorbis: floor X list repeats a value"));
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
  vb_book bk[VB_MAXBOOK]; u32 nbk;
  vb_floor fl[VB_MAXFLOOR]; u32 nfl;
  vb_res rs[VB_MAXRES]; u32 nrs;
  vb_map mp[VB_MAXMAP]; u32 nmp;
  u32 nmd;
  u8 blockflag[VB_MAXMODE], mdmap[VB_MAXMODE];
  void check() const {
    u32 i, j;
    Fi(nfl, Fj(VB_MAXCLASS,
      u32 k;
      FATAL_UNLESS(fl[i].cbook[j] < (i32) nbk, "vorbis: floor master book %ld",
                   (long) fl[i].cbook[j]);
      Fk(8,
        FATAL_UNLESS(fl[i].csb[j][k] < (i32) nbk, "vorbis: floor subclass "
                     "book %ld", (long) fl[i].csb[j][k]))));
    Fi(nrs,
      u32 c, p;
      FATAL_UNLESS(rs[i].cbook < nbk, "vorbis: residue class book %lu",
                   (unsigned long) rs[i].cbook);
      FATAL_UNLESS(rs[i].ncl <= 16,
                   "vorbis: %lu residue classifications, limit 16",
                   (unsigned long) rs[i].ncl);
      for (c = 0; c < rs[i].ncl; c++)
        for (p = 0; p < 8; p++) {
          i32 b = rs[i].book[c][p];
          FATAL_UNLESS(b < (i32) nbk, "vorbis: residue book %ld", (long) b);
          FATAL_UNLESS(b < 0 || bk[b].look == 1, "vorbis: residue book %ld has "
                       "no lookup table", (long) b);
        });
    Fi(nmd, FATAL_UNLESS(mdmap[i] < nmp, "vorbis: mode %lu names mapping "
                            "%d", (unsigned long) i, mdmap[i]));
    Fi(nmp, Fj(mp[i].sub,
      FATAL_UNLESS(mp[i].fl[j] < nfl, "vorbis: submap floor %d", mp[i].fl[j]);
      FATAL_UNLESS(mp[i].rs[j] < nrs, "vorbis: submap residue %d", mp[i].rs[j])));
  }
};
static vb_setup vb_su;
struct vb_ctx {
  vb_info i;
  vb_setup * cur;
  void init() {
    memset(&i, 0, sizeof i);
    cur = nullptr;
    vb_used = 0;
  }
  void link() { cur = nullptr; vb_used = 0; }
};
constexpr u32 VB_MAXBLK = 8192;
constexpr u32 VB_MAXDIM = 0xFFFF;
constexpr i32 RS_DIGIT_MAX = 0x7FFFFF;
constexpr sz VB_YSMAX = (sz) VB_MAXCH * VB_MAXPOST;
constexpr sz VB_CSMAX = (sz) VB_MAXCH * (VB_MAXBLK / 2 + VB_MAXDIM);
static u32 vb_ys[VB_YSMAX];
static u32 vb_cs[VB_CSMAX];
static u32 lk1(u32 entries, u32 dim) {
  u32 v = 0, i, p;
  for (;;) {
    p = 1;
    Fi(dim,
      if (p > entries / (v + 1)) { p = entries + 1; break; }
      p *= v + 1);
    if (p > entries) return v;
    v++;
  }
}
constexpr u32 QUANT[4] = { 256, 128, 86, 64 };
struct io {
  int enc;
  tsv & t;
  vb_ctx & v;
  u8 * b;
  sz len, pos;
  io(vb_ctx & vv, tsv & tt, int e, u8 * pkt, sz n)
    : enc(e), t(tt), v(vv), b(pkt), len(n), pos(0) {
    if (!e) memset(pkt, 0, n);
  }
  u32 bget(int n) {
    u32 r = 0;
    int i;
    FATAL_IF_HOT(pos + (sz) n > len * 8)("vorbis: packet ends inside a field");
    for (i = 0; i < n; i++, pos++)
      r |= (u32) ((b[pos >> 3] >> (pos & 7)) & 1) << i;
    return r;
  }
  void bput(int n, u32 x) {
    int i;
    FATAL_IF_HOT(pos + (sz) n > len * 8)("vorbis: packet ends inside a field");
    for (i = 0; i < n; i++, pos++)
      b[pos >> 3] |= (u8) (((x >> i) & 1) << (pos & 7));
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
    b[pos >> 3] |= (u8) (bit << (pos & 7));
    pos++;
  }
  INLINE u32 val(const char * tag, u32 x, unsigned long long hi) {
    if (!enc) return t.get_u(tag, hi);
    FATAL_IF_HOT((unsigned long long) x >= hi)("vorbis: %s is out of range", tag);
    t.put(tag, x);
    return x;
  }
  INLINE i32 sval(const char * tag, i32 x, i32 lim) {
    long long r;
    if (enc) {
      FATAL_IF_HOT(x < -lim || x > lim)("vorbis: %s is out of range", tag);
      t.put(tag, x);
      return x;
    }
    r = t.get(tag);
    FATAL_UNLESS(r >= -lim && r <= lim, "vorbis: %s is out of range", tag);
    return (i32) r;
  }
  u32 fld(const char * tag, int n, int w) {
    u32 x;
    if (enc) {
      x = bget(n);
      FATAL_UNLESS(w >= 32 || !(x >> w), "vorbis: %s value %lu exceeds %d bits",
                   tag, (unsigned long) x, w);
      t.put(tag, x);
      return x;
    }
    x = t.get_u(tag, w >= 32 ? 0x100000000ULL : 1ULL << w);
    bput(n, x);
    return x;
  }
  void cst(int n, u32 k) {
    if (enc) {
      u32 x = bget(n);
      FATAL_UNLESS(x == k, "vorbis: fixed field %lu, expected %lu",
                   (unsigned long) x, (unsigned long) k);
    } else bput(n, k);
  }
  u32 flt(const char * ts, const char * te, const char * tm) {
    u32 w = enc ? bget(32) : 0, s, e, m;
    s = val(ts, enc ? w >> 31 : 0, 2);
    e = val(te, enc ? (w >> 21) & 0x3FF : 0, 0x400);
    m = val(tm, enc ? w & 0x1FFFFF : 0, 0x200000);
    w = (s << 31) | (e << 21) | m;
    if (!enc) bput(32, w);
    return w;
  }
  u32 bk_get(vb_book * bk) {
    u32 idx = 0;
    i32 nx;
    for (;;) {
      nx = bk->nd[2 * idx + bget1()];
      FATAL_IF_HOT(nx == 0)("vorbis: the packet holds no such codeword");
      if (nx < 0) return (u32) (-nx - 1);
      idx = (u32) nx;
    }
  }
  void bk_put(vb_book * bk, u32 e) {
    u32 k;
    FATAL_UNLESS(e < bk->ent && bk->len[e], "vorbis: codebook has no entry %lu",
                 (unsigned long) e);
    for (k = bk->len[e]; k > 0; k--) bput1((bk->code[e] >> (k - 1)) & 1);
  }
  void lengths(vb_book * k) {
    u32 i, got, cur, n, ent = k->ent;
    int used;
    if (fld("cb.ordered", 1, 1)) {
      got = 0; cur = fld("cb.ordlen", 5, 5) + 1;
      while (got < ent) {
        n = fld("cb.ordrun", (int) blr_ilog(ent - got), (int) blr_ilog(ent - got));
        FATAL_UNLESS(n <= ent - got, "vorbis: ordered run overruns its book");
        FATAL_UNLESS(cur <= 32, "vorbis: ordered lengths run past 32");
        for (i = got; i < got + n; i++) k->len[i] = (u8) cur;
        got += n; cur++;
      }
      return;
    }
    if (!fld("cb.sparse", 1, 1)) {
      Fi(ent, k->len[i] = (u8) (fld("cb.len", 5, 5) + 1));
      return;
    }
    used = (int) fld("cb.used", 1, 1);
    Fi(ent,
      if (i) used = (int) fld("cb.used", 1, 1);
      if (used) k->len[i] = (u8) (fld("cb.len", 5, 5) + 1));
  }
  void codebooks() {
    vb_setup * s = v.cur;
    u32 nb, j, lk, vb, i;
    vb_book * k;
    nb = fld("cb.count", 8, 8) + 1;
    FATAL_UNLESS(nb <= VB_MAXBOOK, "vorbis: %lu codebooks", (unsigned long) nb);
    s->nbk = nb;
    Fj(nb,
      k = s->bk + j;
      cst(24, 0x564342);
      k->dim = fld("cb.dim", 16, 16);
      k->ent = fld("cb.entries", 24, 24);
      FATAL_UNLESS(k->dim > 0, "vorbis: codebook %lu has dimension 0",
                   (unsigned long) j);
      FATAL_UNLESS(k->ent <= (1UL << 20), "vorbis: codebook %lu has %lu entries",
                   (unsigned long) j, (unsigned long) k->ent);
      FATAL_UNLESS(blr_ilog(k->dim) + blr_ilog(k->ent) <= 24,
                   "vorbis: codebook %lu size %lu x %lu is unsupported",
                   (unsigned long) j, (unsigned long) k->dim,
                   (unsigned long) k->ent);
      k->len = vb_alloc<u8>(k->ent); memset(k->len, 0, k->ent);
      k->code = vb_alloc<u32>(k->ent);
      lengths(k);
      k->words(); k->tree();
      lk = fld("cb.lookup", 4, 2);
      k->look = (u8) lk;
      if (!lk) continue;
      FATAL_UNLESS(lk == 1, "vorbis: unsupported codebook lookup type %lu",
                   (unsigned long) lk);
      flt("cb.min.sign", "cb.min.exp", "cb.min.mant");
      flt("cb.dlt.sign", "cb.dlt.exp", "cb.dlt.mant");
      vb = fld("cb.valbits", 4, 4) + 1;
      fld("cb.sequence", 1, 1);
      k->nv = lk1(k->ent, k->dim);
      k->mult = vb_alloc<u32>(k->nv ? k->nv : 1);
      Fi(k->nv, k->mult[i] = fld("cb.mult", (int) vb, (int) vb));
      FATAL_UNLESS(k->nv > 0, "vorbis: codebook %lu has an empty lookup table",
                   (unsigned long) j);
      k->grid());
  }
  void floors() {
    vb_setup * s = v.cur;
    u32 nt, nf, k, i, j, nc, sub, sb;
    vb_floor * q;
    nt = fld("flr.ntime", 6, 6) + 1;
    Fi(nt, cst(16, 0));
    nf = fld("flr.count", 6, 6) + 1;
    FATAL_UNLESS(nf <= VB_MAXFLOOR, "vorbis: %lu floors", (unsigned long) nf);
    s->nfl = nf;
    Fk(nf,
      q = s->fl + k;
      FATAL_UNLESS(fld("flr.type", 16, 1) == 1,
                   "vorbis: floor type 0 is unsupported");
      q->parts = fld("flr.parts", 5, 5);
      FATAL_UNLESS(q->parts <= VB_MAXPART, "vorbis: floor has %lu partitions",
                   (unsigned long) q->parts);
      nc = 0;
      Fi(q->parts,
        q->pcls[i] = (u8) fld("flr.pclass", 4, 4);
        FATAL_UNLESS(q->pcls[i] < VB_MAXCLASS, "vorbis: floor class %lu",
                     (unsigned long) q->pcls[i]);
        if ((u32) q->pcls[i] + 1 > nc) nc = q->pcls[i] + 1);
      Fi(nc,
        q->cdim[i] = (u8) (fld("flr.cdim", 3, 3) + 1);
        sub = fld("flr.csub", 2, 2);
        q->csub[i] = (u8) sub; q->cbook[i] = -1;
        if (sub) q->cbook[i] = (i32) fld("flr.cbook", 8, 8);
        Fj(1UL << sub,
          sb = fld("flr.sbook", 8, 8);
          q->csb[i][j] = (i32) sb - 1));
      q->mult = fld("flr.mult", 2, 2) + 1;
      q->quant = QUANT[q->mult - 1];
      q->rng = fld("flr.rangebits", 4, 4);
      q->x[0] = 0; q->x[1] = 1UL << q->rng; q->posts = 2;
      Fi(q->parts, Fj(q->cdim[q->pcls[i]],
        FATAL_UNLESS(q->posts < VB_MAXPOST, "vorbis: floor has over %lu posts",
                     (unsigned long) VB_MAXPOST);
        q->x[q->posts++] = fld("flr.x", (int) q->rng, (int) q->rng)));
      q->sort());
  }
  void residues() {
    vb_setup * s = v.cur;
    u32 nr, k, i, j, bno, c;
    vb_res * q;
    nr = fld("res.count", 6, 6) + 1;
    FATAL_UNLESS(nr <= VB_MAXRES, "vorbis: %lu residues", (unsigned long) nr);
    s->nrs = nr;
    Fk(nr,
      q = s->rs + k;
      q->type = fld("res.type", 16, 2);
      FATAL_UNLESS(q->type <= 2, "vorbis: residue type above 2");
      q->beg = fld("res.begin", 24, 24); q->end = fld("res.end", 24, 24);
      q->psz = fld("res.partsize", 24, 24) + 1;
      q->ncl = fld("res.nclass", 6, 6) + 1;
      FATAL_UNLESS(q->ncl <= VB_MAXRCL, "vorbis: residue has %lu classifications",
                   (unsigned long) q->ncl);
      q->cbook = fld("res.classbook", 8, 8);
      Fi(q->ncl,
        c = 0;
        if (enc) { c = bget(3); if (bget(1)) c |= bget(5) << 3; }
        c = val("res.cascade", c, 256);
        q->casc[i] = (u8) c;
        if (!enc) {
          bput(3, c & 7); bput(1, c >> 3 ? 1 : 0);
          if (c >> 3) bput(5, c >> 3);
        });
      Fi(q->ncl, Fj(8,
        q->book[i][j] = -1;
        if (!(q->casc[i] & (1UL << j))) continue;
        bno = fld("res.book", 8, 8);
        q->book[i][j] = (i32) bno)));
  }
  void mappings(u32 ch) {
    vb_setup * s = v.cur;
    u32 nm, j, i, sub = 1, steps = 0, nmode, fl;
    vb_map * q;
    nm = fld("map.count", 6, 6) + 1;
    FATAL_UNLESS(nm <= VB_MAXMAP, "vorbis: %lu mappings", (unsigned long) nm);
    s->nmp = nm;
    Fj(nm,
      q = s->mp + j;
      cst(16, 0);
      if (enc) {
        fl = bget(1); sub = fl ? bget(4) + 1 : 1;
        FATAL_UNLESS(!fl || sub > 1, "vorbis: invalid submap flag");
      }
      sub = val("map.submaps", enc ? sub - 1 : 0, 16) + 1;
      FATAL_UNLESS(sub >= 1 && sub <= VB_MAXSUB, "vorbis: mapping has %lu submaps",
                   (unsigned long) sub);
      if (!enc) { bput(1, sub > 1); if (sub > 1) bput(4, sub - 1); }
      if (enc) steps = bget(1) ? bget(8) + 1 : 0;
      steps = val("map.steps", steps, 257);
      if (!enc) { bput(1, steps != 0); if (steps) bput(8, steps - 1); }
      q->sub = sub; q->nstep = steps;
      FATAL_UNLESS(steps <= VB_MAXCH, "vorbis: %lu coupling steps",
                   (unsigned long) steps);
      Fi(steps,
        q->mag[i] = (u8) fld("map.mag", (int) blr_ilog(ch - 1), (int) blr_ilog(ch - 1));
        q->ang[i] = (u8) fld("map.ang", (int) blr_ilog(ch - 1), (int) blr_ilog(ch - 1));
        FATAL_UNLESS(q->mag[i] < ch && q->ang[i] < ch && q->mag[i] != q->ang[i],
                     "vorbis: invalid coupling channels %d/%d of %lu",
                     q->mag[i], q->ang[i], (unsigned long) ch));
      cst(2, 0);
      if (sub > 1) Fi(ch,
        q->mux[i] = (u8) fld("map.mux", 4, 4);
        FATAL_UNLESS(q->mux[i] < sub, "vorbis: channel %lu maps to submap %d of %lu",
                     (unsigned long) i, q->mux[i],
                     (unsigned long) sub));
      Fi(sub,
        fld("map.smtime", 8, 8);
        q->fl[i] = (u8) fld("map.smfloor", 8, 8);
        q->rs[i] = (u8) fld("map.smres", 8, 8)));
    nmode = fld("mode.count", 6, 6) + 1;
    FATAL_UNLESS(nmode <= VB_MAXMODE, "vorbis: %lu modes", (unsigned long) nmode);
    s->nmd = nmode;
    Fi(nmode,
      s->blockflag[i] = (u8) fld("mode.blockflag", 1, 1);
      cst(16, 0); cst(16, 0);
      s->mdmap[i] = (u8) fld("mode.mapping", 8, 8));
    cst(1, 1);
  }
  void ident() {
    vb_info * n = &v.i;
    u32 bs;
    FATAL_UNLESS(!enc || len < 8 || memcmp(b, "OpusHead", 8),
                 "Ogg Opus is not supported");
    cst(8, 1); cst(8, 'v'); cst(8, 'o'); cst(8, 'r');
    cst(8, 'b'); cst(8, 'i'); cst(8, 's'); cst(32, 0);
    n->ch = fld("id.channels", 8, 8);
    n->rate = fld("id.rate", 32, 32);
    fld("id.brmax", 32, 32);
    fld("id.brnom", 32, 32);
    fld("id.brmin", 32, 32);
    bs = fld("id.blocksize", 8, 8);
    n->bs0 = 1UL << (bs & 15); n->bs1 = 1UL << (bs >> 4);
    fld("id.framing", 8, 8);
    FATAL_UNLESS(n->ch >= 1, "vorbis: %lu channels", (unsigned long) n->ch);
    FATAL_UNLESS(n->rate >= 1, "vorbis: sample rate 0");
    FATAL_UNLESS(n->bs0 >= 64 && n->bs1 >= n->bs0 && n->bs1 <= 8192,
                 "vorbis: block sizes %lu and %lu",
                 (unsigned long) n->bs0, (unsigned long) n->bs1);
  }
  void comment() {
    sz i;
    Fi(len,
      if (enc) t.put("cmt.byte", b[i]);
      else b[i] = (u8) t.get_u("cmt.byte", 256));
    pos = len * 8;
  }
  void setup() {
    vb_setup * s;
    cst(8, 5); cst(8, 'v'); cst(8, 'o'); cst(8, 'r');
    cst(8, 'b'); cst(8, 'i'); cst(8, 's');
    FATAL_UNLESS(!v.cur, "vorbis: a link has two setup headers");
    s = &vb_su; memset(s, 0, sizeof *s);
    v.cur = s;
    codebooks(); floors(); residues(); mappings(v.i.ch);
    s->check();
  }
  void hdr(int which) {
    switch (which) {
      case 0: ident(); break;
      case 1: comment(); break;
      default: setup(); break;
    }
    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %lu unparsed packet bits",
                 (unsigned long) (len * 8 - pos));
    if (enc)
      FATAL_UNLESS(bget((int) (len * 8 - pos)) == 0,
                   "vorbis: nonzero packet padding");
  }
  int fl_get(vb_floor * f, u32 * y) {
    vb_setup * s = v.cur;
    u32 i, k, j = 2, p, cd, cs, cv;
    if (!bget(1)) return 0;
    p = blr_ilog(f->quant - 1);
    y[0] = bget(p); y[1] = bget(p);
    Fi(f->parts,
      u32 c = f->pcls[i], cv0;
      cd = f->cdim[c]; cs = f->csub[c]; cv = 0;
      if (cs) cv = bk_get(s->bk + f->cbook[c]);
      cv0 = cv;
      Fk(cd,
        i32 sb = f->csb[c][cv & ((1UL << cs) - 1)];
        cv >>= cs;
        y[j + k] = sb >= 0 ? bk_get(s->bk + sb) : 0);
      FATAL_UNLESS(!(cv0 >> (cs * cd)),
                   "vorbis: floor class codeword exceeds its dimension");
      Fk(cd,
        u32 l, d = (cv0 >> (k * cs)) & ((1UL << cs) - 1);
        for (l = 0; l < d; l++) {
          u32 mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
          FATAL_UNLESS(y[j + k] >= mx,
                       "vorbis: floor value uses a later subclass book than "
                       "the first that fits");
        });
      j += cd);
    return 1;
  }
  void fl_put(vb_floor * f, const u32 * y, int used) {
    vb_setup * s = v.cur;
    u32 i, k, l, j = 2, p, cd, cs, cv, mx;
    bput(1, (u32) used);
    if (!used) return;
    p = blr_ilog(f->quant - 1);
    FATAL_UNLESS(y[0] < (1UL << p) && y[1] < (1UL << p),
                 "vorbis: floor post does not fit %lu bits", (unsigned long) p);
    bput(p, y[0]); bput(p, y[1]);
    Fi(f->parts,
      u32 c = f->pcls[i];
      cd = f->cdim[c]; cs = f->csub[c]; cv = 0;
      if (cs) {
        Fk(cd,
          for (l = 0; l < (1UL << cs); l++) {
            mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
            if (y[j + k] < mx) break;
          }
          FATAL_UNLESS(l < (1UL << cs), "vorbis: no class %lu book fits value %lu",
                       (unsigned long) c, (unsigned long) y[j + k]);
          cv |= l << (k * cs));
        bk_put(s->bk + f->cbook[c], cv);
      }
      Fk(cd,
        i32 sb = f->csb[c][cv & ((1UL << cs) - 1)];
        cv >>= cs;
        if (sb >= 0) bk_put(s->bk + sb, y[j + k]));
      j += cd);
  }
  INLINE void rs_sym(vb_book * bk) {
    u32 k, e = 0, np = 1, sym = 0;
    i32 d;
    if (enc) { e = bk_get(bk); sym = e; }
    Fk(bk->dim,
      if (enc) {
        d = (i32) bk->mult[sym % bk->nv] - (i32) bk->off; sym /= bk->nv;
        sval("res.digit", d, RS_DIGIT_MAX);
      } else {
        u32 p;
        d = sval("res.digit", 0, RS_DIGIT_MAX);
        FATAL_IF_HOT(!(d + (i32) bk->off >= 0 && d + (i32) bk->off < (i32) bk->base))
          ("vorbis: residue digit %ld outside codebook grid", (long) d);
        p = bk->inv[(u32) (d + (i32) bk->off)];
        FATAL_IF_HOT(p == (u32) -1)
          ("vorbis: residue digit %ld is not a codebook multiplicand", (long) d);
        e += p * np; np *= bk->nv;
      });
    if (!enc) {
      FATAL_IF_HOT(e >= bk->ent)("vorbis: residue has no codebook entry");
      bk_put(bk, e);
    }
  }
  HOT FLATTEN void rs_part(vb_res * r, vb_book * bk) {
    u32 i, st;
    if (r->type == 0) {
      st = r->psz / bk->dim;
      FATAL_IF_HOT(st * bk->dim != r->psz)
        ("vorbis: residue 0 partition %lu not divisible by %lu",
         (unsigned long) r->psz, (unsigned long) bk->dim);
      Fi(st, rs_sym(bk));
    } else
      for (i = 0; i < r->psz; i += bk->dim)
        rs_sym(bk);
  }
  void residue(u32 rno, const u8 * nz, u32 nch, u32 n) {
    vb_setup * s = v.cur;
    vb_res * r = s->rs + rno;
    vb_book * cb = s->bk + r->cbook;
    u32 vch, end, np, pv, pass, pc, i, j, k, cw, w;
    u32 * cl;
    u32 nc = 0;
    Fi(nch, if (nz[i]) nc++);
    if (!nc) return;
    if (r->type == 2) { vch = 1; end = blr_min(r->end, n * nch); }
    else { vch = nc; end = blr_min(r->end, n); }
    if (end <= r->beg) return;
    np = (end - r->beg) / r->psz;
    if (!np) return;
    pv = cb->dim;
    w = np + pv;
    FATAL_UNLESS((sz) vch * w <= VB_CSMAX,
                 "vorbis: residue needs %lu classification slots, limit %lu",
                 (unsigned long) ((sz) vch * w), (unsigned long) VB_CSMAX);
    cl = vb_cs;
    for (pass = 0; pass < 8; pass++) {
      pc = 0;
      while (pc < np) {
        if (!pass)
          Fj(vch,
            if (enc) {
              cw = bk_get(cb);
              for (k = pv; k > 0; k--) { cl[j * w + pc + k - 1] = cw % r->ncl;
                                         cw /= r->ncl; }
              FATAL_UNLESS(!cw, "vorbis: residue classword exceeds its partitions");
            }
            Fk(pv, cl[j * w + pc + k] = val("res.class",
                                            enc ? cl[j * w + pc + k] : 0, 16));
            if (!enc) {
              cw = 0;
              Fk(pv, cw = cw * r->ncl + cl[j * w + pc + k]);
              bk_put(cb, cw);
            });
        Fk(pv,
          if (pc >= np) break;
          Fj(vch,
            i32 bn;
            FATAL_IF_HOT(cl[j * w + pc] >= r->ncl)
              ("vorbis: residue class %lu, limit %lu",
               (unsigned long) cl[j * w + pc], (unsigned long) r->ncl);
            bn = r->book[cl[j * w + pc]][pass];
            if (bn >= 0) rs_part(r, s->bk + bn));
          pc++);
      }
    }
  }
  void payload(u32 mode) {
    vb_setup * s = v.cur;
    vb_map * mp = s->mp + s->mdmap[mode];
    u32 n = (s->blockflag[mode] ? v.i.bs1 : v.i.bs0) / 2;
    u32 ch = v.i.ch, k, i, j, m;
    u8 nz[VB_MAXCH], sub[VB_MAXCH];
    u32 * y = vb_ys;
    memset(nz, 0, sizeof nz);
    Fk(ch,
      u32 fno = mp->fl[mp->mux[k]];
      vb_floor * f = s->fl + fno;
      u32 * yc = y + k * VB_MAXPOST;
      int u = 0;
      if (enc) u = fl_get(f, yc);
      u = (int) val("flr.used", (u32) u, 2);
      nz[k] = (u8) u;
      if (u)
        Fi(f->posts, u32 p = f->srt[i];
                     yc[p] = val("flr.y", enc ? yc[p] : 0, 0x100000000ULL));
      if (!enc) fl_put(f, yc, u));
    Fi(mp->nstep, if (nz[mp->mag[i]] || nz[mp->ang[i]])
                    nz[mp->mag[i]] = nz[mp->ang[i]] = 1);
    Fi(mp->sub,
      m = 0;
      Fj(ch, if (mp->mux[j] == i) sub[m++] = nz[j]);
      if (m) residue(mp->rs[i], sub, m, n));
  }
  void audio() {
    u32 md;
    int d, w, i;
    FATAL_UNLESS(v.cur && v.cur->nmd > 0, "vorbis: audio before setup");
    d = (int) blr_ilog(v.cur->nmd - 1);
    if (enc) FATAL_UNLESS(bget(1) == 0, "vorbis: header on audio path");
    else bput(1, 0);
    md = enc ? bget(d) : 0;
    md = val("aud.mode", md, v.cur->nmd);
    if (!enc) bput(d, md);
    if (v.cur->blockflag[md])
      Fi(2,
        w = (int) val(i ? "aud.wnext" : "aud.wprev",
                      enc ? bget(1) : 0, 2);
        if (!enc) bput(1, (u32) w));
    payload(md);
    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %lu unparsed audio bits",
                 (unsigned long) (len * 8 - pos));
    if (enc)
      FATAL_UNLESS(bget((int) (len * 8 - pos)) == 0,
                   "vorbis: nonzero audio padding");
  }
};
constexpr sz IN_LOW = 20UL << 20;
constexpr sz IN_WIN = 32UL << 20;
static u8 in_win[IN_WIN];
struct source {
  FILE * f;
  const char * path;
  sz at, have;
  sz origin;
  int eof;
  void open(const char * pth) {
    f = fopen(pth, "rb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", pth);
    path = pth; at = have = origin = 0; eof = 0;
    want();
  }
  void close() {
    int bad = ferror(f);
    if (fclose(f)) bad = 1;
    if (bad) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
    f = nullptr;
  }
  void want() {
    sz got;
    if (avail() >= IN_LOW || eof) return;
    if (at) {
      memmove(in_win, in_win + at, have - at);
      origin += at; have -= at; at = 0;
    }
    while (have < IN_WIN) {
      got = fread(in_win + have, 1, IN_WIN - have, f);
      if (!got) { eof = 1; break; }
      have += got;
    }
    if (ferror(f)) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
  }
  void skip(sz n) { at += n; }
  const u8 * cur() const { return in_win + at; }
  sz avail() const { return have - at; }
  sz offset() const { return origin + at; }
  sz join_len(sz got, const ogg_page & p, sz pl) const {
    const ogg_page * prev = &p;
    sz total = pl, k = at + got, n;
    ogg_page q;
    for (;;) {
      FATAL_UNLESS(!(prev->type & 4), "the packet at %lu exceeds its link",
                   (unsigned long) offset());
      n = q.parse(in_win + k, have - k);
      FATAL_UNLESS(n != 0, "the packet at %lu has no continuation page",
                   (unsigned long) offset());
      FATAL_UNLESS(q.np > 0 && (q.type & 1),
                   "the continuation of the packet at %lu is missing",
                   (unsigned long) offset());
      total += q.plen[0];
      FATAL_UNLESS(total - pl <= MAXPAY * (sz) OGG_MAXSEG,
                   "the packet at %lu continues too far",
                   (unsigned long) offset());
      if (!(q.np == 1 && q.tail)) return total;
      k += n; prev = &q;
    }
  }
  void join_pkt(sz got, const u8 * head, sz pl, sz total, u8 * dst) const {
    sz done = pl, k = at + got, n, take;
    ogg_page q;
    memcpy(dst, head, pl);
    while (done < total) {
      n = q.parse(in_win + k, have - k);
      FATAL_UNLESS(n != 0, "the packet at %lu has no continuation page",
                   (unsigned long) offset());
      take = q.plen[0];
      if (take > total - done) take = total - done;
      memcpy(dst + done, in_win + k + OGG_HDRMIN + q.nseg, take);
      done += take; k += n;
    }
  }
};
constexpr sz JB_MAX = MAXPAY * (OGG_MAXSEG + 1);
static u8 jb[JB_MAX];
struct link_walk {
  int w;
  int cont;
  sz spill;
  void start() { w = 0; cont = 0; spill = 0; }
  void page(vb_ctx & v, tsv & t, source & s, const ogg_page & p,
                       sz got) {
    const u8 * pbody = s.cur() + OGG_HDRMIN + p.nseg;
    unsigned long where = (unsigned long) s.offset();
    sz at = 0;
    int j;
    p.put(t);
    Fj(p.np,
      const u8 * pk = pbody + at;
      sz pl = p.plen[j];
      sz off = at;
      at += pl;
      if (j == 0 && (p.type & 1) && cont) {
        FATAL_UNLESS(pl <= spill, "the page at %lu takes %lu of %lu remaining "
                     "bytes", where, (unsigned long) pl, (unsigned long) spill);
        spill -= pl;
        if (!(p.np == 1 && p.tail)) {
          FATAL_UNLESS(!spill, "the page at %lu closes a packet %lu bytes early",
                       where, (unsigned long) spill);
          cont = 0;
        }
        continue;
      }
      FATAL_UNLESS(pl > 0, "the page at %lu carries an empty packet", where);
      if (j == p.np - 1 && p.tail) {
        sz tot = s.join_len(got, p, pl);
        s.join_pkt(got, pbody + off, pl, tot, jb);
        t.put("page.spill", (long long) (tot - pl));
        spill = tot - pl; cont = 1; pk = jb; pl = tot;
      }
      io z(v, t, 1, (u8 *) pk, pl);
      if (pk[0] & 1) {
        FATAL_UNLESS(w < 3, "a link has over three header packets");
        z.hdr(w); w++;
      } else z.audio());
  }
};
static u8 pgbuf[PAGE_MAX];
static u8 body[MAXPAY];
void vb_pack(const char * in, const char * out) {
  source src;
  vb_ctx v;
  tsv t;
  link_walk lw;
  ogg_page p;
  sz got, npage = 0;
  int eos = 1;
  src.open(in);
  t.create(out);
  v.init();
  lw.start();
  for (;;) {
    src.want();
    if (!src.avail()) break;
    got = p.parse(src.cur(), src.avail());
    FATAL_UNLESS(got != 0, "%s: no Ogg page at %lu", in,
                 (unsigned long) src.offset());
    FATAL_UNLESS(ogg_page::crc_ok(src.cur(), got), "%s: the page at %lu has a bad CRC",
                 in, (unsigned long) src.offset());
    if ((p.type & 2) && eos) {
      t.put("link.more", 1); v.link(); lw.start(); eos = 0;
    }
    FATAL_UNLESS(!eos, "%s: page %lu %s", in, (unsigned long) npage,
                 npage ? "follows end of stream" : "precedes any bitstream");
    lw.page(v, t, src, p, got);
    if (p.type & 4) {
      FATAL_UNLESS(!lw.cont, "%s: a continued packet is %lu bytes short", in,
                   (unsigned long) lw.spill);
      eos = 1;
    }
    src.skip(got); npage++;
  }
  FATAL_UNLESS(npage > 0, "%s: not an Ogg bitstream", in);
  FATAL_UNLESS(eos, "%s: final bitstream has no end-of-stream page", in);
  t.put("link.more", 0);
  t.close(); src.close();
}
void vb_unpack(const char * in, const char * out) {
  vb_ctx v;
  tsv t;
  FILE * o;
  t.open(in);
  v.init();
  o = fopen(out, "wb");
  if (!o) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
  while (t.get_u("link.more", 2)) {
    ogg_page q;
    sz jn = 0, spill = 0;
    int w = 0, done = 0, cont = 0;
    v.link();
    while (!done) {
      sz at = 0, pgl;
      int j;
      q.get(t);
      FATAL_UNLESS(q.blen <= MAXPAY, "%s: a page claims %lu payload bytes", in,
                   (unsigned long) q.blen);
      Fj(q.np,
        sz pl = q.plen[j];
        if (j == 0 && (q.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill, "%s: page takes %lu of %lu remaining bytes",
                       in, (unsigned long) pl, (unsigned long) spill);
          if (pl) memcpy(body + at, jb + jn - spill, pl);
          spill -= pl;
          if (!(q.np == 1 && q.tail)) {
            FATAL_UNLESS(!spill, "%s: a page closes a packet %lu bytes early",
                         in, (unsigned long) spill);
            cont = 0;
          }
          at += pl; continue;
        }
        FATAL_UNLESS(pl > 0, "%s: a page claims an empty packet", in);
        if (j == q.np - 1 && q.tail) {
          sz ex = (sz) t.get_u("page.spill",
                               (unsigned long long) MAXPAY * OGG_MAXSEG + 1);
          sz tot = pl + ex;
          io z(v, t, 0, jb, tot);
          jn = tot;
          if (w < 3) { z.hdr(w); w++; } else z.audio();
          memcpy(body + at, jb, pl);
          spill = ex; cont = 1; at += pl; continue;
        }
        io z(v, t, 0, body + at, pl);
        if (w < 3) { z.hdr(w); w++; } else z.audio();
        at += pl);
      pgl = q.emit(pgbuf, body);
      if (fwrite(pgbuf, 1, pgl, o) != pgl)
        FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
      if (q.type & 4) done = 1;
    }
    FATAL_UNLESS(!cont, "%s: a continued packet is %lu bytes short", in,
                 (unsigned long) spill);
  }
  t.close();
  if (fclose(o)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
}
int main(int argc, char ** argv) {
  if (argc != 4 || argv[1][0] == 0 || argv[1][1] != 0 ||
      (argv[1][0] != 'c' && argv[1][0] != 'd')) {
    fprintf(stderr, "usage: ogg2tsv c|d input output\n"
                    "  c  decompose Ogg Vorbis into a TSV record stream\n"
                    "  d  rebuild the Ogg Vorbis file from that stream\n");
    return BLR_EXIT_USAGE;
  }
  if (argv[1][0] == 'c') vb_pack(argv[2], argv[3]);
  else vb_unpack(argv[2], argv[3]);
  return BLR_EXIT_OK;
}
