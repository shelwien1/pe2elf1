/*  oggdet -- detection and extraction of Ogg bitstreams from arbitrary
    containers, with byte-exact restoration.

      oggdet c [options] <input> <prefix>     carve
      oggdet d [options] <prefix> <output>    restore

    Carving writes every detected physical Ogg bitstream to <prefix>%08X.ogg
    -- or, with -c, coded by oggcomp to <prefix>%08X.oc -- and all remaining
    bytes, plus the layout, to <prefix>.meta.  Restoring reads the metainfo
    and the numbered stream files and reproduces <input> exactly.

    Detection is CRC-driven: a candidate is accepted only when its page CRC
    verifies, which is 2^-32 per position, so the payload of a page can never
    re-detect as a page start.  Public domain / do what you want.

    This is oggdet on oggcomp's I/O: the container and the metainfo go
    through the coroutine's input and output pins, as oggcomp's files do,
    and the numbered stream and image files through Lib3's file API -- so
    there is no stdio in the data path and the WinAPI build is the same
    program.  The compressor is oggcomp's own coroutine, oc_coro, driven
    from inside this one, one fresh model per stream, so that every .oc it
    writes is what `oggcomp c` would have written and `oggcomp d` reads it.  */

#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
#include "oc_rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "sh_mapping.inc"
#include "tc_base.inc"
#include "tc_ptab.inc"
#include "tc_tables.inc"
#include "tc_fam.inc"
#include "oc_hist.inc"
#include "oc_model.inc"
#include "oc_frame.inc"
#include "oc_header.inc"
#include "oc_raw.inc"
#include "oc_floor.inc"
#include "oc_residue.inc"
#include "vb_packet.inc"
#include "ogg_stream.inc"
#include "oc_coro.inc"

#define OGGDET_VERSION "oggdet 1.5"

/*  ------------------------------------------------------------------ misc  */

/*  What oggart.inc and the code below call on a bad input: the message, and
    exit 1, through the same path as oggcomp's refusals -- which also removes
    the output the run was writing.  */
static NORETURN void die(const char * fmt, ...) OGC_PRINTF(1, 2);
static NORETURN void die(const char * fmt, ...) {
  char msg[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof msg, fmt, ap);
  va_end(ap);
  FATAL("%s", msg);
}

static void * xmalloc(size_t n) {
  void * p = malloc(n ? n : 1);
  if (!p) die("out of memory (%llu bytes)", (unsigned long long) n);
  return p;
}

static void * xcalloc_(size_t n, size_t sz) {
  void * p = calloc(n ? n : 1, sz);
  if (!p) die("out of memory");
  return p;
}

static void * xrealloc(void * p, size_t n) {
  void * q = realloc(p, n ? n : 1);
  if (!q) die("out of memory (%llu bytes)", (unsigned long long) n);
  return q;
}

/*  Cover art in the comment header, shared with any other frontend.  */
#include "oggart.inc"

/*  Ogg page CRC: ogg_page.inc's crcinit/crcrun, the same polynomial.  */

/*  Ordinary reflected CRC-32, used only to verify a restore.  */
static u32 RCRC[256];
static void rcrc_init(void) {
  u32 i, j, c;
  if (RCRC[1]) return;
  for (i = 0; i < 256; i++) {
    c = i;
    for (j = 0; j < 8; j++) c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    RCRC[i] = c;
  }
}
static u32 rcrc(u32 c, const u8 * d, size_t n) {
  size_t i;
  c = ~c;
  for (i = 0; i < n; i++) c = RCRC[(c ^ d[i]) & 0xFF] ^ (c >> 8);
  return ~c;
}

/*  ------------------------------------------------------------- page parse  */

#define OGG_MAXPAGE ((size_t) PAGE_MAX)              /*  65307  */

typedef struct {
  size_t len;                 /*  total page size in bytes  */
  u8 type;                    /*  header_type: 1 cont, 2 bos, 4 eos  */
  u32 serial, seq;
  int nseg;
  size_t body;                /*  offset of the body within the page  */
  size_t blen;
  int first_pkt;              /*  length of the first lacing value  */
} page;

/*  Reasons a candidate is rejected, for -w.  */
static const char * PGERR[] = {
  "ok",
  "short buffer for a page header",
  "no OggS signature",
  "stream_structure_version is not 0",
  "short buffer for the segment table",
  "short buffer for the page body",
  "page CRC mismatch"
};

/*  Parse the page at b[0..n).  Returns 0 on success, else a PGERR index.
    `n` is what is currently available; a truncated page is distinguished
    from a malformed one so the caller can stop cleanly at EOF.  */
static int pg_parse(page * p, const u8 * b, size_t n) {
  size_t hl, i;
  u32 want, got;
  if (n < OGG_HDRMIN) return 1;
  if (memcmp(b, "OggS", 4)) return 2;
  if (b[4]) return 3;
  p->nseg = b[26];
  hl = OGG_HDRMIN + (size_t) p->nseg;
  if (n < hl) return 4;
  p->blen = 0;
  for (i = 0; i < (size_t) p->nseg; i++) p->blen += b[OGG_HDRMIN + i];
  p->len = hl + p->blen;
  if (n < p->len) return 5;
  p->type = b[5];
  p->serial = rd32(b + 14);
  p->seq = rd32(b + 18);
  p->body = hl;
  p->first_pkt = p->nseg ? b[OGG_HDRMIN] : 0;
  want = rd32(b + 22);
  {
    static const u8 z4[4] = { 0, 0, 0, 0 };
    got = crcrun(0, b, 22);
    got = crcrun(got, z4, 4);
    got = crcrun(got, b + 26, p->len - 26);
  }
  if (got != want) return 6;
  return 0;
}

/*  --------------------------------------------------------- codec sniffing  */

typedef struct { const char * sig; int n; const char * name; } codec_sig;

static const codec_sig CODECS[] = {
  { "\x01vorbis",     7, "vorbis"   },
  { "OpusHead",       8, "opus"     },
  { "\x80theora",     7, "theora"   },
  { "\x7f" "FLAC",    5, "flac"     },
  { "fLaC",           4, "flac"     },
  { "Speex   ",       8, "speex"    },
  { "fishead\0",      8, "skeleton" },
  { "\x80kate\0\0\0", 8, "kate"     },
  { "\x80" "daala",   6, "daala"    },
  { "BBCD",           4, "dirac"    },
  { "PCM     ",       8, "oggpcm"   },
  { "CMML\0\0\0\0",   8, "cmml"     },
  { "YUV4MPEG",       8, "uvs"      },
  { "\x4f\x67\x67\x4d\x49\x44\x49", 7, "oggmidi" }
};
#define NCODECS ((int) (sizeof CODECS / sizeof CODECS[0]))

static const char * sniff(const u8 * body, size_t blen) {
  int i;
  for (i = 0; i < NCODECS; i++)
    if (blen >= (size_t) CODECS[i].n && !memcmp(body, CODECS[i].sig, CODECS[i].n))
      return CODECS[i].name;
  return "unknown";
}

/*  ------------------------------------------------------------- CRLF view  */

/*  A binary file moved in text mode comes back with every LF turned into
    CRLF: page bodies grow by about one byte in 256, so lengths and CRCs stop
    matching but the stream is otherwise intact.  Parsing such a run through a
    view that collapses CRLF back to LF recovers it exactly, and re-expanding
    on restore is exact as long as the raw bytes hold no bare LF -- which is
    checked per page, not assumed.  */

/*  Collapse into `out`, stopping at `cap` clean bytes.  */
static size_t declean(const u8 * raw, size_t n, u8 * out, size_t cap) {
  size_t i = 0, m = 0;
  while (i < n && m < cap) {
    if (raw[i] == '\r' && i + 1 < n && raw[i + 1] == '\n') i++;
    out[m++] = raw[i++];
  }
  return m;
}

/*  Raw bytes behind `want` clean ones, 0 if the buffer is too short.
    `bare` reports an LF that no CR precedes, which would not survive the
    round trip.  Must follow declean's rule exactly.  */
static size_t raw_span(const u8 * raw, size_t n, size_t want, int * bare) {
  size_t i = 0, m = 0;
  *bare = 0;
  while (m < want && i < n) {
    if (raw[i] == '\r' && i + 1 < n && raw[i + 1] == '\n') i++;
    else if (raw[i] == '\n') *bare = 1;
    i++;  m++;
  }
  return m == want ? i : 0;
}

/*  One page seen through either view.  */
typedef struct {
  page pg;
  const u8 * data;            /*  bytes to write out  */
  size_t cleanlen, rawlen;
  int bare;
} pview;

/*  Enough raw bytes for a maximum page even if every byte of it was doubled.  */
#define LOOKAHEAD (2 * OGG_MAXPAGE)

static u8 cleanbuf[OGG_MAXPAGE];

static int page_view(const u8 * raw, size_t n, int text, pview * v) {
  int r;
  if (!text) {
    r = pg_parse(&v->pg, raw, n);
    if (r) return r;
    v->data = raw;  v->cleanlen = v->rawlen = v->pg.len;  v->bare = 0;
    return 0;
  }
  {
    size_t cn = declean(raw, n, cleanbuf, OGG_MAXPAGE);
    r = pg_parse(&v->pg, cleanbuf, cn);
    if (r) return r;
    v->rawlen = raw_span(raw, n, v->pg.len, &v->bare);
    if (!v->rawlen) return 5;
    v->data = cleanbuf;  v->cleanlen = v->pg.len;
    return 0;
  }
}

/*  ----------------------------------------------------------- input buffer  */

/*  A sliding window over the input pin.  The window is always large enough
    to hold the qualification lookahead (see WIN), so every decision the
    scanner makes is local.  When it runs low it pulls from the pin, which is
    where this coroutine yields for the frontend to read the file.  */

#define WIN (4u << 20)

static u8 od_win[WIN];

typedef struct {
  coro3_pin * in;
  u8 * b;
  size_t cap, n, p;           /*  b[p..n) is unread, b[0..n) is valid  */
  u64 base;                   /*  absolute file offset of b[0]  */
  int eof;
  u32 crc;                    /*  reflected CRC-32 over everything consumed  */
  u64 total;
} src;

static void src_open(src * s, coro3_pin * pin) {
  s->in = pin;
  s->cap = WIN;
  s->b = od_win;
  s->n = s->p = 0;  s->base = 0;  s->eof = 0;
  s->crc = 0;  s->total = 0;
}

/*  Up to n bytes from the pin into dst; zero once the file is done.  */
static size_t src_fill(src * s, u8 * dst, size_t n) {
  coro3_pin & p = *s->in;
  for (;;) {
    size_t got = (size_t) (p.end - p.ptr);
    if (got) {
      if (got > n) got = n;
      memcpy(dst, p.ptr, got);
      p.ptr += got;
      return got;
    }
    if (p.f_quit()) return 0;
    p.yield_r();
  }
}

/*  Make at least `want` bytes available at the cursor when possible.
    Returns how many actually are.  */
static size_t src_need(src * s, size_t want) {
  size_t have = s->n - s->p, got;
  if (have >= want || s->eof) return have;
  if (s->p) {                                   /*  compact  */
    memmove(s->b, s->b + s->p, have);
    s->base += s->p;  s->n = have;  s->p = 0;
  }
  while (s->n < want && s->n < s->cap && !s->eof) {
    got = src_fill(s, s->b + s->n, s->cap - s->n);
    if (!got) { s->eof = 1;  break; }
    s->n += got;
  }
  return s->n - s->p;
}

static const u8 * src_at(src * s) { return s->b + s->p; }
static u64 src_off(const src * s) { return s->base + s->p; }

static void src_skip(src * s, size_t k) {
  s->crc = rcrc(s->crc, s->b + s->p, k);
  s->total += k;
  s->p += k;
}

/*  The metainfo read back as a stream: one byte, or exactly n.  */
static int src_byte(src * s) {
  u8 c;
  if (src_need(s, 1) < 1) return -1;
  c = *src_at(s);
  src_skip(s, 1);
  return c;
}
static int src_take(src * s, u8 * dst, size_t n) {
  while (n) {
    size_t k = src_need(s, n < WIN / 2 ? n : WIN / 2);
    if (!k) return 0;
    if (k > n) k = n;
    memcpy(dst, src_at(s), k);
    src_skip(s, k);
    dst += k;  n -= k;
  }
  return 1;
}

/*  ---------------------------------------------------------- output pin  */

/*  Where the metainfo goes when carving and the restored file when
    restoring: the coroutine's output pin, filled and yielded on.  */
typedef struct {
  coro3_pin * out;
} od_out;

static void out_put(od_out * o, const void * data, size_t n) {
  coro3_pin & p = *o->out;
  const u8 * b = (const u8 *) data;
  while (n) {
    size_t room = (size_t) (p.end - p.ptr);
    if (!room) { p.yield_r();  continue; }
    if (room > n) room = n;
    memcpy(p.ptr, b, room);
    p.ptr += room;
    b += room;
    n -= room;
  }
}

/*  ---------------------------------------------------------- metainfo file  */

/*  Layout:

      "OGGDETv5"                       8 bytes
      record*                          varint (len << 1) | kind
                                         kind 0 = gap, len raw bytes follow
                                         kind 1 = stream: flags byte, then
                                                  varint duplicate-of when
                                                  flag 2, then the art record
                                                  when flag 4
      varint 0                         terminator (a zero-length gap is never
                                                   emitted, so 0 is free)
      u32 le                           CRC-32 of the whole original input
      u64 le                           length of the whole original input

    Stream flags: 1 stored with CRLF collapsed, re-expand LF on restore;
    2 a duplicate of an earlier stream, whose number follows; 4 the cover
    art was taken out, the record follows; 8 the file is an oggcomp .oc.

    Streams are numbered in order of appearance, so a stream record carries
    only its length, which the restorer checks against the file on disk.
    A v4 metainfo is the same without flag 8, and is read as well.  */

#define META_MAGIC "OGGDETv5"
#define META_MAGIC_V4 "OGGDETv4"

/*  One encoder for both record kinds, so a change to the tag width cannot be
    applied to one of them and forgotten on the other.  */
#define REC_GAP     0
#define REC_STREAM  1
#define REC_TAG(len, kind)  ((((u64) (len)) << 1) | (u64) (kind))

static void put_varint(od_out * o, u64 v) {
  do {
    u8 c = (u8) (v & 0x7F);
    v >>= 7;
    if (v) c |= 0x80;
    out_put(o, &c, 1);
  } while (v);
}

static int get_varint(src * s, u64 * out) {
  u64 v = 0;
  int sh = 0, c;
  for (;;) {
    c = src_byte(s);
    if (c < 0) return 0;
    if (sh > 63) return 0;
    v |= (u64) (c & 0x7F) << sh;
    if (!(c & 0x80)) break;
    sh += 7;
  }
  *out = v;
  return 1;
}

/*  ---------------------------------------------------------------- options  */

typedef struct {
  int verbose;                /*  -v  */
  int why;                    /*  -w  */
  int join;                   /*  -j: do not split a run at chain boundaries  */
  int partial;                /*  -p: accept a run that does not start at BOS  */
  int minpg;                  /*  -n N  */
  const char * meta;          /*  -m FILE  */
  const char * only;          /*  -o LIST  */
  u64 minsize;                /*  -s N: smallest run worth extracting  */
  int notext;                 /*  -x: no CRLF detection or repair  */
  int art;                    /*  cover art comes out by default; -A leaves it  */
  int nodedup;                /*  -D: write duplicate files anyway  */
  int dry;                    /*  -t: detect and report, write nothing  */
  int comp;                   /*  -c: streams coded by oggcomp  */
} opts;

static void usage(FILE * f) {
  fprintf(f,
    "usage: oggdet c [options] <input> <prefix>    carve Ogg streams out\n"
    "       oggdet d [options] <prefix> <output>   put them back\n"
    "\n"
    "  carve writes <prefix>%%08X.ogg per detected stream, <prefix>%%08X.jpg\n"
    "  per cover image found in one, and <prefix>.meta holding every remaining\n"
    "  byte plus the layout.  Byte-identical files are written once.\n"
    "\n"
    "options\n"
    "  -c        compress: each stream coded by oggcomp, <prefix>%%08X.oc\n"
    "            in place of .ogg; the same file `oggcomp c` would write\n"
    "  -v        report each detected stream on stderr\n"
    "  -w        report why each rejected \"OggS\" candidate was rejected\n"
    "  -j        join: one file per contiguous run, do not split chains\n"
    "  -p        accept a run whose first page is not a beginning-of-stream\n"
    "  -n N      require N consecutive valid pages, 1..8 (default 1)\n"
    "  -s N      smallest file worth writing, stream or image, 0 disables\n"
    "            (default 1024)\n"
    "  -x        do not detect or repair CRLF-mangled streams\n"
    "  -A        leave cover art in the stream instead of extracting it\n"
    "  -D        do not deduplicate: write byte-identical files more than once\n"
    "  -o LIST   extract only runs whose every logical stream is in LIST,\n"
    "            e.g. -o vorbis,opus; others are left in the metainfo\n"
    "  -m FILE   metainfo path (default <prefix>.meta)\n"
    "  -t        detect and report only, write no files\n"
    "  -h        this text\n");
}

/*  Own case-insensitive compare, so no platform header is needed.  */
static int ci_eq(const char * a, const char * b, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    int x = a[i], y = b[i];
    if (x >= 'A' && x <= 'Z') x += 32;
    if (y >= 'A' && y <= 'Z') y += 32;
    if (x != y) return 0;
  }
  return 1;
}

static int only_match(const opts * o, const char * name) {
  const char * p = o->only;
  size_t n = strlen(name);
  if (!p) return 1;
  while (*p) {
    const char * q = strchr(p, ',');
    size_t l = q ? (size_t) (q - p) : strlen(p);
    if (l == n && ci_eq(p, name, l)) return 1;
    p = q ? q + 1 : p + l;
  }
  return 0;
}

/*  --------------------------------------------------------------- qualify  */

/*  Look at the candidate under the cursor without consuming it.  Fills
    `codecs` with the codec of every beginning-of-stream page found at the
    head of the run, which is what -o and -v report.  */

typedef struct {
  int ok;
  int page_ok;                /*  the first page parsed; only policy said no  */
  int rej;                    /*  PGERR index when !ok  */
  int text;                   /*  the run parses through the CRLF view  */
  int nbos;
  const char * codec[16];
  u32 serial[16];
  int truncated;              /*  the window ran out during the lookahead  */
} cand;

/*  How many consecutive pages parse from the cursor in one view.  */
#define MODEPROBE 4

static int probe_pages(src * s, int text, int limit) {
  size_t avail = s->n - s->p, at = 0;
  int npg = 0;
  pview v;
  while (npg < limit) {
    if (at + LOOKAHEAD > avail && !s->eof) break;
    if (avail - at < OGG_HDRMIN) break;
    if (page_view(src_at(s) + at, avail - at, text, &v)) break;
    if (text && v.bare) break;
    at += v.rawlen;  npg++;
  }
  return npg;
}

static void qualify(src * s, const opts * o, cand * c) {
  size_t avail, at = 0;
  int npg = 0, nb, nt;
  pview v;

  memset(c, 0, sizeof *c);
  avail = src_need(s, LOOKAHEAD);

  /*  Pick the view that carries the run further.  A page holding no CRLF at
      all parses identically either way, so ties go to the raw view.  */
  nb = probe_pages(s, 0, MODEPROBE);
  nt = o->notext ? 0 : probe_pages(s, 1, MODEPROBE);
  c->text = nt > nb;

  if (page_view(src_at(s), avail, c->text, &v)) {
    c->rej = pg_parse(&v.pg, src_at(s), avail);
    if (!c->rej) c->rej = 6;
    return;
  }
  c->page_ok = 1;
  if (!(v.pg.type & 2) && !o->partial) { c->rej = 7;  return; }

  /*  Walk the head of the run: collect every beginning-of-stream page (per
      the Ogg spec they all precede any data page) and, if -n asked for it,
      keep going until the page count is met.  */
  {
    int bos_run = 1;
    for (;;) {
      if (at + LOOKAHEAD > avail && !s->eof) { c->truncated = 1;  break; }
      if (avail - at < OGG_HDRMIN) break;
      if (page_view(src_at(s) + at, avail - at, c->text, &v)) break;
      if (c->text && v.bare) break;
      npg++;
      if (v.pg.type & 2) {
        if (bos_run && c->nbos < 16) {
          c->codec[c->nbos] = sniff(v.data + v.pg.body, v.pg.blen);
          c->serial[c->nbos] = v.pg.serial;
          c->nbos++;
        }
      } else bos_run = 0;
      at += v.rawlen;
      if (!bos_run && npg >= o->minpg) break;
      if (npg >= 64) break;
    }
  }

  if (npg < o->minpg && !c->truncated) { c->rej = 8;  return; }

  if (o->only) {
    int i;
    for (i = 0; i < c->nbos; i++)
      if (!only_match(o, c->codec[i])) { c->rej = 9;  return; }
    if (!c->nbos && !o->partial) { c->rej = 9;  return; }
  }
  c->ok = 1;
}

static const char * rej_text(int r) {
  static const char * extra[] = {
    "the run does not begin at a beginning-of-stream page",
    "the run is shorter than the minimum page count",
    "a logical stream is outside the -o list"
  };
  if (r <= 6) return PGERR[r];
  return extra[r - 7];
}

/*  ------------------------------------------------------------ side files  */

static void spew_(const char * path, const void * b, size_t n) {
  filehandle f;
  if (!f.make(path)) die("cannot create %s", path);
  if (n && f.writ((void *) b, (uint) n) != n) die("write error on %s", path);
  if (f.error() || f.close()) die("write error on %s", path);
}

/*  Read a whole file into a buffer.  */
static void slurp_(const char * path, bbuf * o) {
  filehandle f;
  static u8 tmp[1 << 16];
  if (!f.open(path)) die("cannot open %s", path);
  bb_init(o);
  for (;;) {
    uint k = f.read(tmp, sizeof tmp);
    if (!k) break;
    bb_put(o, tmp, k);
  }
  if (f.error()) die("read error on %s", path);
  f.close();
}

static int files_equal(const char * a, const char * b) {
  filehandle fa, fb;
  static u8 ba[1 << 16], bb2[1 << 16];
  int same = 1;
  if (!fa.open(a)) return 0;
  if (!fb.open(b)) { fa.close();  return 0; }
  for (;;) {
    uint ka = fa.read(ba, sizeof ba), kb = fb.read(bb2, sizeof bb2);
    if (ka != kb || memcmp(ba, bb2, ka)) { same = 0;  break; }
    if (!ka) break;
  }
  fa.close();  fb.close();
  return same;
}

/*  -------------------------------------------------------- the compressor  */

/*  oggcomp, inside this program: its encoder and decoder coroutines, each
    with its stack pad and its model, driven from here.  The tables are
    mapped once, on the first stream that needs them, and reset before every
    stream, so that each .oc is what a fresh `oggcomp c` writes and a fresh
    `oggcomp d` reads.  This coroutine drives them from a bounded depth of
    its own stack, which the pad between frontend and coroutine allows.  */

static oc_coro<0> od_enc;
static oc_coro<1> od_dec;
static int od_model_ready;

static void od_model(void) {
  if (od_model_ready) return;
  od_enc.t.init();
  od_model_ready = 1;
}

/*  Where an accepted run's bytes go: the .ogg file as they are, or through
    the encoder into the .oc file.  */
typedef struct {
  int on, comp;
  filehandle f;
  const char * path;
  u64 written;
  u8 obuf[1 << 16];
} osink;

static void os_flush(osink * s) {
  uint n = od_enc.getoutsize();
  if (n && s->f.writ(od_enc.outbeg, n) != n) die("write error on %s", s->path);
  s->written += n;
  od_enc.addout(s->obuf, sizeof s->obuf);
}

static void os_open(osink * s, const char * path, int comp) {
  s->path = path;  s->comp = comp;  s->written = 0;
  if (!s->f.make(path)) die("cannot create %s", path);
  s->on = 1;
  if (comp) {
    u8 h[8];
    memcpy(h, OC_MAGIC, sizeof OC_MAGIC - 1);
    h[sizeof OC_MAGIC - 1] = (u8) OC_VER;
    h[sizeof OC_MAGIC] = 0;
    if (s->f.writ(h, sizeof OC_MAGIC + 1) != sizeof OC_MAGIC + 1) die("write error on %s", path);
    s->written = sizeof OC_MAGIC + 1;
    od_model();
    oc_fresh(od_enc, path);
    od_enc.addout(s->obuf, sizeof s->obuf);
  }
}

static void os_put(osink * s, const u8 * p, size_t k) {
  if (!s->on) return;
  if (!s->comp) {
    if (k && s->f.writ((void *) p, (uint) k) != k) die("write error on %s", s->path);
    s->written += k;
    return;
  }
  od_enc.addinp((byte *) p, (uint) k);
  for (;;) {
    uint r = od_enc.coro_call(&od_enc);
    if (r == 2) { os_flush(s);  continue; }
    if (r == 1) break;
    die("internal: the encoder stopped inside %s", s->path);
  }
}

/*  End the file: for the encoder, the end of its input, then everything it
    still has to say.  */
static void os_close(osink * s) {
  if (!s->on) return;
  if (s->comp) {
    od_enc.f_quit = 1;
    od_enc.addinp(s->obuf, 0);
    for (;;) {
      uint r = od_enc.coro_call(&od_enc);
      if (r == 2) { os_flush(s);  continue; }
      if (r == 0) { os_flush(s);  break; }
      die("internal: the encoder asked for more of %s", s->path);
    }
  }
  if (s->f.error() || s->f.close()) die("write error on %s", s->path);
  s->on = 0;
}

/*  A .oc back to the bytes it holds.  */
static void oc_decode(const u8 * oc, size_t n, const char * path, bbuf * out) {
  static u8 obuf[1 << 16];
  if (n < sizeof OC_MAGIC + 1 || memcmp(oc, OC_MAGIC, sizeof OC_MAGIC - 1))
    die("%s is not an oggcomp stream", path);
  if (oc[sizeof OC_MAGIC - 1] != OC_VER)
    die("%s: made by oggcomp version %u, this is version %u", path,
        oc[sizeof OC_MAGIC - 1], OC_VER);
  od_model();
  oc_fresh(od_dec, path);
  od_dec.addinp((byte *) oc + sizeof OC_MAGIC + 1, (uint) (n - sizeof OC_MAGIC - 1));
  od_dec.addout(obuf, sizeof obuf);
  for (;;) {
    uint r = od_dec.coro_call(&od_dec);
    if (r == 2) { bb_put(out, obuf, od_dec.getoutsize());  od_dec.addout(obuf, sizeof obuf);  continue; }
    if (r == 1) { od_dec.f_quit = 1;  od_dec.addinp(obuf, 0);  continue; }
    if (r == 0) { bb_put(out, obuf, od_dec.getoutsize());  break; }
    die("internal: the decoder stopped inside %s", path);
  }
}

/*  ------------------------------------------------------------------ carve  */

/*  Deduplication.  A file is hashed as it is written; on a hash hit the two
    files are compared byte for byte before one is called a duplicate, so a
    collision costs a comparison and never costs correctness.  */

typedef struct { u64 hash, len; u32 idx; int used; } dent;
typedef struct { dent * e; size_t cap, n; const char * prefix; } dtab;

static void dt_init(dtab * t, const char * prefix) {
  t->cap = 256;  t->n = 0;  t->prefix = prefix;
  t->e = (dent *) xcalloc_(t->cap, sizeof *t->e);
}
static void dt_free(dtab * t) { free(t->e);  memset(t, 0, sizeof *t); }

static u64 fnv(u64 h, const u8 * p, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) { h ^= p[i];  h *= 1099511628211ull; }
  return h;
}
#define FNV0 1469598103934665603ull

static void dt_grow(dtab * t) {
  dent * old = t->e;
  size_t oc = t->cap, i;
  t->cap *= 2;
  t->e = (dent *) xcalloc_(t->cap, sizeof *t->e);
  for (i = 0; i < oc; i++) {
    if (!old[i].used) continue;
    { size_t j = (size_t) (old[i].hash & (t->cap - 1));
      while (t->e[j].used) j = (j + 1) & (t->cap - 1);
      t->e[j] = old[i]; }
  }
  free(old);
}

/*  `path` is the file just written.  Returns the index of an identical file
    already on disk, or -1 after recording this one under `idx`.  */
static long dt_lookup(dtab * t, const char * path, const char * ext,
                      u64 hash, u64 len, u32 idx) {
  size_t j;
  char other[4096];
  if (t->n * 2 >= t->cap) dt_grow(t);
  j = (size_t) (hash & (t->cap - 1));
  while (t->e[j].used) {
    if (t->e[j].hash == hash && t->e[j].len == len) {
      snprintf(other, sizeof other, "%s%08X.%s", t->prefix, t->e[j].idx, ext);
      if (files_equal(path, other)) return (long) t->e[j].idx;
    }
    j = (j + 1) & (t->cap - 1);
  }
  t->e[j].used = 1;  t->e[j].hash = hash;  t->e[j].len = len;  t->e[j].idx = idx;
  t->n++;
  return -1;
}

/*  The cover-art stage.  Pages of a run pass through it: the header region is
    held back until its packets are complete, then handed to art_strip, and
    every page after it gets its sequence number shifted.  Memory is bounded
    by the header region, which is where the art lives, not by the stream.  */

#define ART_HDRCAP (16u << 20)

typedef struct {
  int on, done, have;
  bbuf hdr;
  int gotpk, nhp;
  long seqdelta;
  artrec rec;
  u8 * scratch;  size_t scap;
} astage;

static void as_init(astage * a) {
  memset(a, 0, sizeof *a);
  bb_init(&a->hdr);
}
static void as_reset(astage * a) {
  if (a->have) art_free(&a->rec);
  a->on = a->done = a->have = a->gotpk = 0;
  a->seqdelta = 0;  a->hdr.n = 0;
}
static void as_free(astage * a) {
  if (a->have) art_free(&a->rec);
  bb_free(&a->hdr);  free(a->scratch);
  memset(a, 0, sizeof *a);
}

/*  Bytes that are not part of an accepted stream.  Buffered only until the
    next stream starts, so peak memory is one gap, not the file.  */
typedef struct {
  u8 * b;
  size_t n, cap;
  od_out * mf;
  u64 total;
} gapb;

static void gap_init(gapb * g, od_out * mf) {
  g->cap = 1u << 16;  g->n = 0;  g->total = 0;  g->mf = mf;
  g->b = (u8 *) xmalloc(g->cap);
}

#define GAP_CHUNK (1u << 20)

static void gap_flush(gapb * g);

static void gap_put(gapb * g, const u8 * p, size_t k) {
  if (!k) return;
  if (g->n + k > g->cap) {
    while (g->n + k > g->cap) g->cap *= 2;
    g->b = (u8 *) xrealloc(g->b, g->cap);
  }
  memcpy(g->b + g->n, p, k);
  g->n += k;
  /*  Adjacent gap records concatenate, so a long stretch of non-Ogg data
      can be emitted as it is seen instead of held whole.  */
  if (g->n >= GAP_CHUNK) gap_flush(g);
}

static void gap_flush(gapb * g) {
  if (!g->n) return;
  if (g->mf) {
    put_varint(g->mf, REC_TAG(g->n, REC_GAP));
    out_put(g->mf, g->b, g->n);
  }
  g->total += g->n;
  g->n = 0;
}

/*  Consume one run of consecutive valid pages, stopping at a chain boundary
    unless -j.

    With `prefix` NULL the run goes straight to the gap: that is the path for
    a run policy already turned down, and it is what keeps the scanner from
    resuming inside a page body.

    Otherwise the run is a candidate.  Its length is not known until it ends,
    so the first bytes are held back until they clear -s; only then is the gap
    flushed and the output file opened.  A run that ends short is appended to
    the gap instead, and `*accepted` comes back 0.  The hold buffer is bounded
    by the threshold, so the default costs about 1 KiB.  */
static u64 take_run(src * s, const opts * o, gapb * g,
                    const char * prefix, u32 idx, int text, astage * as,
                    u64 * rawtotal, u64 * hashout, u64 * fileout,
                    int * npages, int * saw_eos, int * accepted) {
  u64 len = 0, rawlen = 0, hash = FNV0;
  int npg = 0, seen_data = 0, eos = 0, acc = 0;
  static osink of;
  u8 * hold = NULL;
  size_t held = 0, holdcap = 0;
  /*  The raw bytes behind `hold`.  A run that turns out to be too small goes
      back into the gap, and the gap must get what was actually in the
      container -- not a CRLF repair, and not an art strip.  */
  u8 * rawhold = NULL;
  size_t rawheld = 0, rawcap = 0;
  static char path[4096];

  of.on = 0;  of.written = 0;

  #define PROMOTE() do {                                                      \
      gap_flush(g);                                                           \
      if (!o->dry) {                                                          \
        snprintf(path, sizeof path, "%s%08X.%s", prefix, idx, o->comp ? "oc" : "ogg"); \
        os_open(&of, path, o->comp);                                          \
      }                                                                       \
      if (held) os_put(&of, hold, held);                                      \
      held = 0;  rawheld = 0;  acc = 1;                                       \
    } while (0)

  /*  Everything an accepted run emits goes through here: the -s hold buffer,
      the output sink, and the running hash the deduplicator uses.  */
  #define EMIT(p_, k_) do {                                                   \
      const u8 * ep_ = (const u8 *) (p_);                                     \
      size_t ek_ = (k_);                                                      \
      hash = fnv(hash, ep_, ek_);                                             \
      if (acc) os_put(&of, ep_, ek_);                                         \
      else {                                                                  \
        if (held + ek_ > holdcap) {                                           \
          if (!holdcap) holdcap = (size_t) o->minsize + OGG_MAXPAGE;          \
          while (held + ek_ > holdcap) holdcap *= 2;                          \
          hold = (u8 *) xrealloc(hold, holdcap);                              \
        }                                                                     \
        memcpy(hold + held, ep_, ek_);  held += ek_;                          \
      }                                                                       \
      len += ek_;                                                             \
      if (!acc && len >= o->minsize) PROMOTE();                               \
    } while (0)

  if (prefix && !o->minsize) PROMOTE();

  for (;;) {
    pview v;
    size_t a = src_need(s, LOOKAHEAD);
    if (a < OGG_HDRMIN) break;
    if (page_view(src_at(s), a, text, &v)) break;
    /*  A bare LF could not be put back, so the run ends before that page.  */
    if (text && v.bare) break;
    /*  Every beginning-of-stream page of a physical bitstream precedes its
        data pages, so a BOS after a data page opens the next chain link.  */
    if (npg && (v.pg.type & 2) && seen_data && !o->join) break;

    if (prefix && !acc) {
      if (rawheld + v.rawlen > rawcap) {
        if (!rawcap) rawcap = (size_t) o->minsize + OGG_MAXPAGE;
        while (rawheld + v.rawlen > rawcap) rawcap *= 2;
        rawhold = (u8 *) xrealloc(rawhold, rawcap);
      }
      memcpy(rawhold + rawheld, src_at(s), v.rawlen);
      rawheld += v.rawlen;
    }

    if (!prefix) {
      /*  A run left behind goes back verbatim.  */
      gap_put(g, src_at(s), v.rawlen);
    } else if (as && as->on && !as->done) {
      int clean = 0;
      bb_put(&as->hdr, v.data, v.cleanlen);
      if (!as->nhp) {
        int c = art_codec(v.data + AP_HDRMIN + v.data[26],
                          v.cleanlen - AP_HDRMIN - v.data[26]);
        as->nhp = c < 0 ? -1 : art_nhp(c);
      }
      as->gotpk += ap_page_packets(v.data, &clean);
      if (as->nhp > 0 && as->gotpk >= as->nhp && clean) {
        bbuf nh;
        if (art_strip(as->hdr.b, as->hdr.n, o->minsize,
                      &as->rec, &nh, o->verbose)) {
          as->have = 1;  as->seqdelta = as->rec.seqdelta;
          EMIT(nh.b, nh.n);
          bb_free(&nh);
        } else EMIT(as->hdr.b, as->hdr.n);
        as->done = 1;  as->hdr.n = 0;
      } else if (as->nhp < 0 || as->hdr.n > ART_HDRCAP) {
        EMIT(as->hdr.b, as->hdr.n);
        as->done = 1;  as->hdr.n = 0;
      }
    } else if (as && as->seqdelta) {
      /*  Fewer header pages than the original, so the rest shifts down.  */
      if (as->scap < v.cleanlen) {
        as->scap = v.cleanlen;
        as->scratch = (u8 *) xrealloc(as->scratch, as->scap);
      }
      memcpy(as->scratch, v.data, v.cleanlen);
      ap_reseq(as->scratch, v.cleanlen, -as->seqdelta);
      EMIT(as->scratch, v.cleanlen);
    } else EMIT(v.data, v.cleanlen);

    rawlen += v.rawlen;
    if (!(v.pg.type & 2)) seen_data = 1;
    if (v.pg.type & 4) eos = 1;
    npg++;
    src_skip(s, v.rawlen);
  }

  /*  A run that ended inside its own headers keeps them verbatim.  */
  if (prefix && as && as->on && !as->done && as->hdr.n) {
    EMIT(as->hdr.b, as->hdr.n);
    as->done = 1;  as->hdr.n = 0;
  }

  if (prefix && !acc) gap_put(g, rawhold, rawheld);  /*  too small: leave it  */
  free(hold);  free(rawhold);
  os_close(&of);
  *npages = npg;  *saw_eos = eos;  *accepted = acc;
  if (rawtotal) *rawtotal = rawlen;
  if (hashout) *hashout = hash;
  if (fileout) *fileout = of.written;
  return len;
  #undef EMIT
  #undef PROMOTE
}

/*  The coroutine: the frontend hands it the container (carving) or the
    metainfo (restoring) on pin 0 and takes the metainfo or the restored
    file from pin 1.  */
struct od_coro : Coroutine {
  const opts * o;
  const char * in;            /*  what is on pin 0, for messages  */
  const char * prefix;
  const char * outname;       /*  what is on pin 1  */
  int restoring;
  int rc;
  void do_process();
};

static int carve(od_coro * co) {
  const opts * o = co->o;
  const char * prefix = co->prefix;
  src s;
  od_out meta;
  od_out * mf = NULL;
  gapb g;
  astage as;
  dtab dstream, dimage;
  static char path[4096], other[4096];
  const char * ext = o->comp ? "oc" : "ogg";
  u32 idx = 0, iidx = 0;
  u64 nstream = 0, nimg = 0, dupstream = 0, dupimage = 0, ocbytes = 0, ogbytes = 0;
  int rc = 0;

  src_open(&s, &co->pin[0]);
  as_init(&as);
  dt_init(&dstream, prefix);
  dt_init(&dimage, prefix);

  if (!o->dry) {
    meta.out = &co->pin[1];
    mf = &meta;
    out_put(mf, META_MAGIC, 8);
  }
  gap_init(&g, mf);

  for (;;) {
    size_t avail = src_need(&s, LOOKAHEAD);
    cand c;
    u64 run_off, run_len, run_raw = 0, hash = 0, run_file = 0;
    int npg = 0, saw_eos = 0, accepted = 0;
    long dup = -1;

    if (!avail) break;

    /*  Skip to the next possible signature in one step.  */
    if (avail < 4 || memcmp(src_at(&s), "OggS", 4)) {
      const u8 * b = src_at(&s);
      const u8 * q = (const u8 *) memchr(b + 1, 'O', avail - 1);
      size_t k = q ? (size_t) (q - b) : avail;
      gap_put(&g, b, k);
      src_skip(&s, k);
      continue;
    }

    qualify(&s, o, &c);

    if (!c.ok) {
      if (c.page_ok) {
        /*  A real page run that policy turned down: keep it whole, so the
            scanner never resumes inside a page body.  */
        run_off = src_off(&s);
        run_len = take_run(&s, o, &g, NULL, 0, c.text, NULL, &run_raw, NULL, NULL,
                           &npg, &saw_eos, &accepted);
        if (o->verbose || o->why)
          fprintf(stderr, "[ skip ] off=%llu len=%llu pages=%d: %s\n",
                  (unsigned long long) run_off, (unsigned long long) run_len,
                  npg, rej_text(c.rej));
      } else {
        if (o->why)
          fprintf(stderr, "[skip] off=%llu: %s\n",
                  (unsigned long long) src_off(&s), rej_text(c.rej));
        gap_put(&g, src_at(&s), 1);
        src_skip(&s, 1);
      }
      continue;
    }

    run_off = src_off(&s);
    as_reset(&as);
    /*  A joined run can span several chain links with independent page
        numbering, which the sequence shift would disturb.  */
    as.on = o->art && !o->join;
    run_len = take_run(&s, o, &g, prefix, idx, c.text, &as, &run_raw, &hash, &run_file,
                       &npg, &saw_eos, &accepted);
    if (!accepted) {
      if (o->verbose || o->why)
        fprintf(stderr, "[ skip ] off=%llu len=%llu pages=%d: "
                        "the run is smaller than the %llu byte minimum\n",
                (unsigned long long) run_off, (unsigned long long) run_len,
                npg, (unsigned long long) o->minsize);
      continue;
    }

    /*  Write the images, deduplicated, before the record that names them.  */
    if (as.have) {
      size_t k;
      for (k = 0; k < as.rec.nimg; k++) {
        artimg * q = as.rec.img + k;
        long d;
        snprintf(path, sizeof path, "%s%08X.%s", prefix, iidx, q->ext);
        if (!o->dry) spew_(path, q->img.b, q->img.n);
        d = o->nodedup || o->dry ? -1
          : dt_lookup(&dimage, path, q->ext,
                      fnv(FNV0, q->img.b, q->img.n), q->img.n, iidx);
        nimg++;
        if (d >= 0) {
          remove(path);
          q->file = (u32) d;
          dupimage++;
          if (o->verbose)
            fprintf(stderr, "         image %llu bytes, same as %08X\n",
                    (unsigned long long) q->img.n, (unsigned) d);
        } else {
          q->file = iidx++;
          if (o->verbose)
            fprintf(stderr, "         %s  %.*s, %llu bytes\n", path,
                    (int) q->name.n, (const char *) q->name.b,
                    (unsigned long long) q->img.n);
        }
      }
    }

    /*  Then the stream file itself.  */
    snprintf(path, sizeof path, "%s%08X.%s", prefix, idx, ext);
    if (!o->dry && !o->nodedup)
      dup = dt_lookup(&dstream, path, ext, hash, run_len, idx);
    if (dup >= 0) {
      remove(path);
      dupstream++;
      snprintf(other, sizeof other, "%s%08X.%s", prefix, (unsigned) dup, ext);
    } else {
      ogbytes += run_len;
      ocbytes += run_file;
    }

    if (mf) {
      u8 flags = (u8) ((c.text ? 1 : 0) | (dup >= 0 ? 2 : 0) | (as.have ? 4 : 0) | (o->comp ? 8 : 0));
      bbuf rec;
      put_varint(mf, REC_TAG(run_len, REC_STREAM));
      out_put(mf, &flags, 1);
      if (dup >= 0) put_varint(mf, (u64) dup);
      if (as.have) {
        bb_init(&rec);
        art_put(&rec, &as.rec);
        out_put(mf, rec.b, rec.n);
        bb_free(&rec);
      }
    }

    if (o->verbose) {
      int i;
      fprintf(stderr, "[%08X] off=%llu len=%llu pages=%d%s",
              idx, (unsigned long long) run_off, (unsigned long long) run_len,
              npg, saw_eos ? "" : " (no EOS)");
      if (o->comp && dup < 0 && !o->dry)
        fprintf(stderr, " oc=%llu", (unsigned long long) run_file);
      if (c.text)
        fprintf(stderr, " CRLF-repaired(raw %llu)", (unsigned long long) run_raw);
      if (as.have)
        fprintf(stderr, " art(%llu)", (unsigned long long) as.rec.nimg);
      if (dup >= 0) fprintf(stderr, " = %08X", (unsigned) dup);
      for (i = 0; i < c.nbos; i++)
        fprintf(stderr, " %s/%08X", c.codec[i], c.serial[i]);
      if (c.truncated) fprintf(stderr, " (lookahead clipped)");
      fputc('\n', stderr);
    }
    if (dup < 0) idx++;
    nstream++;
  }

  gap_flush(&g);

  if (mf) {
    u8 t[12];
    int i;
    put_varint(mf, 0);
    t[0] = (u8) s.crc;  t[1] = (u8) (s.crc >> 8);
    t[2] = (u8) (s.crc >> 16);  t[3] = (u8) (s.crc >> 24);
    for (i = 0; i < 8; i++) t[4 + i] = (u8) (s.total >> (8 * i));
    out_put(mf, t, 12);
  }

  fprintf(stderr, "%s: %llu stream%s", co->in, (unsigned long long) nstream,
          nstream == 1 ? "" : "s");
  if (dupstream)
    fprintf(stderr, " in %u file%s", idx, idx == 1 ? "" : "s");
  if (nimg) {
    fprintf(stderr, ", %llu image%s", (unsigned long long) nimg,
            nimg == 1 ? "" : "s");
    if (dupimage) fprintf(stderr, " in %u file%s", iidx, iidx == 1 ? "" : "s");
  }
  fprintf(stderr, ", %llu of %llu bytes carved",
          (unsigned long long) (s.total - g.total), (unsigned long long) s.total);
  if (o->comp && !o->dry && ogbytes)
    fprintf(stderr, ", %llu bytes of streams coded to %llu (%.2f%%)",
            (unsigned long long) ogbytes, (unsigned long long) ocbytes,
            100.0 * (double) ocbytes / (double) ogbytes);
  fputc('\n', stderr);

  as_free(&as);  dt_free(&dstream);  dt_free(&dimage);
  free(g.b);
  return rc;
}

/*  ---------------------------------------------------------------- restore  */

/*  The restored file on its way out: the pin, and the CRC and count the
    metainfo's trailer is checked against.  */
typedef struct {
  od_out o;
  u32 crc;
  u64 total;
} rout;

static void rout_put(rout * r, const u8 * b, size_t k) {
  if (!k) return;
  out_put(&r->o, b, k);
  r->crc = rcrc(r->crc, b, k);
  r->total += k;
}

/*  Put back the CRLF a text-mode transfer left in the container.  */
static void put_expanded(rout * r, const u8 * b, size_t k) {
  u8 t[8192];
  size_t n = 0, i;
  for (i = 0; i < k; i++) {
    if (n + 2 > sizeof t) { rout_put(r, t, n);  n = 0; }
    if (b[i] == '\n') t[n++] = '\r';
    t[n++] = b[i];
  }
  rout_put(r, t, n);
}

typedef struct { const char * prefix; } imgctx;

static void img_load(void * ctx, u32 file, const char * ext, size_t len,
                     bbuf * outb) {
  imgctx * c = (imgctx *) ctx;
  char path[4096];
  bbuf t;
  (void) len;
  snprintf(path, sizeof path, "%s%08X.%s", c->prefix, file, ext);
  slurp_(path, &t);
  bb_put(outb, t.b, t.n);
  bb_free(&t);
}

/*  Put the art back into a stored stream.  The record says how many pages the
    original header region had; the stored one has that many less seqdelta.  */
static void unstrip(const bbuf * in, const artrec * r, bbuf * out) {
  size_t want = (size_t) ((long) r->npg - r->seqdelta), o = 0, i;
  bbuf hdr;
  if ((long) r->npg - r->seqdelta < 1) die("the art record has an impossible page count");
  for (i = 0; i < want; i++) {
    size_t k = ap_pagelen(in->b + o, in->n - o);
    if (!k) die("the stored stream ends inside its header region");
    o += k;
  }
  bb_init(&hdr);
  bb_put(&hdr, in->b, o);
  art_rebuild(hdr.b, hdr.n, r, out);
  bb_free(&hdr);
  while (o < in->n) {
    size_t k = ap_pagelen(in->b + o, in->n - o), at = out->n;
    if (!k) die("the stored stream has a malformed page");
    bb_put(out, in->b + o, k);
    ap_reseq(out->b + at, k, r->seqdelta);
    o += k;
  }
}

static int restore(od_coro * co) {
  const opts * o = co->o;
  const char * prefix = co->prefix;
  const char * mpath = co->in;
  static char path[4096];
  src mf;
  rout out;
  u8 magic[8], tail[12];
  u8 * buf;
  u32 idx = 0, want_crc;
  u64 nstream = 0, want_len, v;
  size_t bufcap = 1u << 20;
  imgctx ic;

  ic.prefix = prefix;
  src_open(&mf, &co->pin[0]);
  if (!src_take(&mf, magic, 8) || (memcmp(magic, META_MAGIC, 8) && memcmp(magic, META_MAGIC_V4, 8)))
    die("%s is not an oggdet metainfo file", mpath);

  out.o.out = &co->pin[1];
  out.crc = 0;  out.total = 0;
  buf = (u8 *) xmalloc(bufcap);

  for (;;) {
    u64 len;
    int kind, flags = 0;
    if (!get_varint(&mf, &v)) die("%s: truncated record table", mpath);
    if (!v) break;
    kind = (int) (v & 1);
    len = v >> 1;

    if (!kind) {                                        /*  gap  */
      while (len) {
        size_t k = len > bufcap ? bufcap : (size_t) len;
        if (!src_take(&mf, buf, k)) die("%s: truncated gap payload", mpath);
        rout_put(&out, buf, k);
        len -= k;
      }
      continue;
    }

    flags = src_byte(&mf);
    if (flags < 0) die("%s: truncated stream record", mpath);
    {
      u32 file = idx;
      bbuf stored, rebuilt;
      artrec r;
      int havert = 0;

      if (flags & 2) {
        if (!get_varint(&mf, &v)) die("%s: truncated duplicate reference", mpath);
        if (v >= idx) die("%s: stream %llu points forward at %llu", mpath,
                          (unsigned long long) nstream, (unsigned long long) v);
        file = (u32) v;
      }
      snprintf(path, sizeof path, "%s%08X.%s", prefix, file, (flags & 8) ? "oc" : "ogg");
      if (flags & 8) {
        bbuf coded;
        slurp_(path, &coded);
        bb_init(&stored);
        oc_decode(coded.b, coded.n, path, &stored);
        bb_free(&coded);
      } else slurp_(path, &stored);
      if (stored.n != len)
        die("%s is %llu bytes, the metainfo says %llu", path,
            (unsigned long long) stored.n, (unsigned long long) len);

      if (flags & 4) {
        /*  The art record is read straight out of the metainfo stream: it
            is a few kilobytes at most -- names, envelopes and a page layout,
            never the images -- so the window holds it whole.  */
        rdr rd;
        size_t have = src_need(&mf, WIN / 2);
        rd.b = src_at(&mf);  rd.n = have;  rd.p = 0;
        art_get(&rd, &r, img_load, &ic);
        havert = 1;
        src_skip(&mf, rd.p);
      }

      bb_init(&rebuilt);
      if (havert) unstrip(&stored, &r, &rebuilt);
      else bb_put(&rebuilt, stored.b, stored.n);

      if (flags & 1) put_expanded(&out, rebuilt.b, rebuilt.n);
      else rout_put(&out, rebuilt.b, rebuilt.n);

      if (o->verbose)
        fprintf(stderr, "[%08X] %llu bytes%s%s%s%s\n", file,
                (unsigned long long) rebuilt.n,
                (flags & 8) ? " (decoded)" : "",
                (flags & 4) ? " (art put back)" : "",
                (flags & 1) ? " (LF re-expanded)" : "",
                (flags & 2) ? " (shared)" : "");
      if (havert) art_free(&r);
      bb_free(&stored);  bb_free(&rebuilt);
      if (!(flags & 2)) idx++;
      nstream++;
    }
  }

  if (!src_take(&mf, tail, 12)) die("truncated metainfo trailer");
  want_crc = rd32(tail);
  { int i;  want_len = 0;  for (i = 7; i >= 0; i--) want_len = (want_len << 8) | tail[4 + i]; }
  free(buf);

  if (out.total != want_len)
    die("restored %llu bytes, the metainfo says %llu",
        (unsigned long long) out.total, (unsigned long long) want_len);
  if (out.crc != want_crc)
    die("restored CRC %08X, the metainfo says %08X", out.crc, want_crc);

  fprintf(stderr, "%s: %llu stream%s from %u file%s, %llu bytes, CRC %08X ok\n",
          co->outname, (unsigned long long) nstream, nstream == 1 ? "" : "s",
          idx, idx == 1 ? "" : "s", (unsigned long long) out.total, out.crc);
  return 0;
}

void od_coro::do_process() {
  rc = restoring ? restore(this) : carve(this);
  yield(this, 0);
}

static CoroFileProc<od_coro> od_run;

/*  ------------------------------------------------------------------- main  */

int main(int argc, char ** argv) {
  opts o;
  const char * pos[2] = { NULL, NULL };
  const char * mode = NULL;
  static char mpath[4096];
  filehandle f, g;
  int i, np = 0, have_out;

  fprintf(stderr, "%s\n", OGGDET_VERSION);
  ogc_set_prog(argv[0]);
  crcinit();  rcrc_init();

  memset(&o, 0, sizeof o);
  o.minpg = 1;  o.minsize = 1024;  o.art = 1;

  for (i = 1; i < argc; i++) {
    const char * a = argv[i];
    if (a[0] == '-' && a[1]) {
      switch (a[1]) {
        case 'v': o.verbose = 1;  break;
        case 'w': o.why = 1;  break;
        case 'j': o.join = 1;  break;
        case 'p': o.partial = 1;  break;
        case 't': o.dry = 1;  break;
        case 'x': o.notext = 1;  break;
        case 'A': o.art = 0;  break;
        case 'D': o.nodedup = 1;  break;
        case 'c': o.comp = 1;  break;
        case 'h': usage(stdout);  return 0;
        case 's':
          if (++i >= argc) { usage(stderr);  return 2; }
          o.minsize = strtoull(argv[i], NULL, 0);
          break;
        case 'n':
          if (++i >= argc) { usage(stderr);  return 2; }
          o.minpg = atoi(argv[i]);
          if (o.minpg < 1 || o.minpg > 8) die("-n takes 1..8");
          break;
        case 'm':
          if (++i >= argc) { usage(stderr);  return 2; }
          o.meta = argv[i];  break;
        case 'o':
          if (++i >= argc) { usage(stderr);  return 2; }
          o.only = argv[i];  break;
        default:
          fprintf(stderr, "oggdet: unknown option %s\n", a);
          usage(stderr);  return 2;
      }
      continue;
    }
    if (!mode) { mode = a;  continue; }
    if (np < 2) pos[np++] = a;
    else { usage(stderr);  return 2; }
  }

  if (!mode || np != 2 || mode[1] || (mode[0] != 'c' && mode[0] != 'd')) { usage(stderr);  return 2; }

  /*  The window must hold the whole qualification lookahead.  */
  if ((size_t) o.minpg * LOOKAHEAD > WIN / 2) die("-n is too large for the window");

  if (o.meta) snprintf(mpath, sizeof mpath, "%s", o.meta);
  else snprintf(mpath, sizeof mpath, "%s.meta", pos[mode[0] == 'c' ? 1 : 0]);

  od_run.o = &o;
  od_run.prefix = pos[mode[0] == 'c' ? 1 : 0];
  od_run.restoring = mode[0] == 'd';
  od_run.rc = 0;
  if (mode[0] == 'c') {
    od_run.in = pos[0];
    od_run.outname = mpath;
    if (!f.open(pos[0])) FATAL_CODE(OGC_EXIT_IO, "cannot open %s", pos[0]);
    have_out = !o.dry;
    if (have_out && !g.make(mpath)) FATAL_CODE(OGC_EXIT_IO, "cannot create %s", mpath);
  } else {
    if (o.dry) die("-t is meaningless when restoring");
    od_run.in = mpath;
    od_run.outname = pos[1];
    if (!f.open(mpath)) FATAL_CODE(OGC_EXIT_IO, "cannot open %s", mpath);
    have_out = 1;
    if (!g.make(pos[1])) FATAL_CODE(OGC_EXIT_IO, "cannot create %s", pos[1]);
  }
  if (have_out) {
    const char * pp[2] = { od_run.in, od_run.outname };
    ogc_paths_distinct(pp, 2);
    ogc_partial.set(od_run.outname, &g);
  }

  od_run.processfile(f, g);

  if (f.error()) FATAL_CODE(OGC_EXIT_IO, "read error on %s", od_run.in);
  f.close();
  if (have_out) {
    int bad = g.error();
    if (g.close()) bad = 1;
    if (bad) FATAL_CODE(OGC_EXIT_IO, "write error on %s", od_run.outname);
  }
  ogc_partial.kept();
  return od_run.rc;
}
