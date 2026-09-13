/*  oggart.inc -- base64 cover art in an Ogg Vorbis or Ogg Opus comment
    header: strip it out of a header region, and put it back exactly.

    Included by oggdet.cpp (the -a path) and by oggart.cpp (the standalone
    frontend).  Self-contained apart from three things the frontend must
    already provide:

        void   die(const char * fmt, ...);
        void * xmalloc(size_t);
        void * xrealloc(void *, size_t);

    Everything here works on a *header region*: the whole pages holding the
    stream's header packets, ending exactly where the last one ends.  The
    audio pages never come near this code; the caller shifts their sequence
    numbers by artrec.seqdelta and recomputes their CRCs.  */

#ifndef OGGART_INC
#define OGGART_INC

/*  ---------------------------------------------------------------- buffers  */

typedef struct { u8 * b; size_t n, cap; } bbuf;

static void bb_init(bbuf * o) {
  o->cap = 1 << 12;  o->n = 0;  o->b = (u8 *) xmalloc(o->cap);
}
static void bb_free(bbuf * o) { free(o->b);  o->b = NULL;  o->n = o->cap = 0; }
static void bb_put(bbuf * o, const void * p, size_t k) {
  if (!k) return;
  if (o->n + k > o->cap) {
    while (o->n + k > o->cap) o->cap *= 2;
    o->b = (u8 *) xrealloc(o->b, o->cap);
  }
  memcpy(o->b + o->n, p, k);
  o->n += k;
}
static void bb_u8(bbuf * o, u8 v) { bb_put(o, &v, 1); }
static void bb_var(bbuf * o, u64 v) {
  do { u8 c = (u8) (v & 0x7F);  v >>= 7;  if (v) c |= 0x80;  bb_u8(o, c); } while (v);
}

typedef struct { const u8 * b; size_t n, p; } rdr;

static u8 rd_u8(rdr * r) {
  if (r->p >= r->n) die("truncated metainfo");
  return r->b[r->p++];
}
static u64 rd_var(rdr * r) {
  u64 v = 0;
  int sh = 0;
  for (;;) {
    u8 c = rd_u8(r);
    if (sh > 63) die("corrupt varint in the metainfo");
    v |= (u64) (c & 0x7F) << sh;
    if (!(c & 0x80)) break;
    sh += 7;
  }
  return v;
}
static const u8 * rd_bytes(rdr * r, size_t k) {
  const u8 * p;
  if (r->p + k > r->n) die("truncated metainfo");
  p = r->b + r->p;  r->p += k;
  return p;
}

/*  ---------------------------------------------------------- page plumbing  */

#define AP_HDRMIN 27

static u32 AP_CRC[256];
static void ap_crc_init(void) {
  u32 i, j, c;
  if (AP_CRC[1]) return;
  for (i = 0; i < 256; i++) {
    c = i << 24;
    for (j = 0; j < 8; j++) c = (c & 0x80000000u) ? (c << 1) ^ 0x04C11DB7u : c << 1;
    AP_CRC[i] = c;
  }
}

/*  Ogg page CRC, computed with the stored CRC field treated as zero.  */
static u32 ap_pagecrc(const u8 * d, size_t n) {
  static const u8 z4[4] = { 0, 0, 0, 0 };
  const u8 * part[3];
  size_t plen[3], i;
  u32 c = 0;
  int k;
  ap_crc_init();
  part[0] = d;       plen[0] = 22;
  part[1] = z4;      plen[1] = 4;
  part[2] = d + 26;  plen[2] = n - 26;
  for (k = 0; k < 3; k++)
    for (i = 0; i < plen[k]; i++)
      c = (c << 8) ^ AP_CRC[((c >> 24) ^ part[k][i]) & 0xFF];
  return c;
}

static u32 ap_rd32(const u8 * b) {
  return (u32) b[0] | ((u32) b[1] << 8) | ((u32) b[2] << 16) | ((u32) b[3] << 24);
}
static void ap_wr32(u8 * b, u32 v) {
  b[0] = (u8) v;  b[1] = (u8) (v >> 8);
  b[2] = (u8) (v >> 16);  b[3] = (u8) (v >> 24);
}
static u32 ap_rd32be(const u8 * b) {
  return ((u32) b[0] << 24) | ((u32) b[1] << 16) | ((u32) b[2] << 8) | (u32) b[3];
}

/*  Rewrite one page's sequence number and CRC in place.  This is the whole
    transform applied to the audio half of a stream.  */
static void ap_reseq(u8 * page, size_t len, long delta) {
  ap_wr32(page + 18, (u32) ((long) ap_rd32(page + 18) + delta));
  ap_wr32(page + 22, ap_pagecrc(page, len));
}

/*  Size of the page at `b`, or 0 if it is not a whole well-formed page.  */
static size_t ap_pagelen(const u8 * b, size_t n) {
  size_t hl, bl = 0, i;
  int ns;
  if (n < AP_HDRMIN || memcmp(b, "OggS", 4) || b[4]) return 0;
  ns = b[26];  hl = AP_HDRMIN + (size_t) ns;
  if (n < hl) return 0;
  for (i = 0; i < (size_t) ns; i++) bl += b[AP_HDRMIN + i];
  return n < hl + bl ? 0 : hl + bl;
}

/*  Packets completed by this page.  `clean` reports whether the last segment
    of the page also ends a packet, which is what lets a header region stop
    on a page boundary.  */
static int ap_page_packets(const u8 * b, int * clean) {
  int ns = b[26], i, k = 0;
  for (i = 0; i < ns; i++) if (b[AP_HDRMIN + i] < 255) k++;
  *clean = ns > 0 && b[AP_HDRMIN + ns - 1] < 255;
  return k;
}

/*  0 vorbis, 1 opus, -1 anything else.  `body` is the first page's payload.  */
static int art_codec(const u8 * body, size_t blen) {
  if (blen >= 7 && !memcmp(body, "\x01vorbis", 7)) return 0;
  if (blen >= 8 && !memcmp(body, "OpusHead", 8)) return 1;
  return -1;
}
static int art_nhp(int codec) { return codec ? 2 : 3; }

/*  Assemble the header packets out of a buffered header region.  */
#define ART_MAXHP 3

typedef struct {
  int codec, nhp;
  bbuf pk[ART_MAXHP];
  u32 serial;
  size_t npg;
  u8 * ptype;  u64 * pgran;  u32 * pseq;  u64 * pbrk;
} apregion;

static void apr_free(apregion * a) {
  int i;
  for (i = 0; i < ART_MAXHP; i++) bb_free(&a->pk[i]);
  free(a->ptype);  free(a->pgran);  free(a->pseq);  free(a->pbrk);
  memset(a, 0, sizeof *a);
}

/*  Returns 0 when the buffer is not exactly a header region.  */
static int apr_parse(const u8 * b, size_t n, apregion * a) {
  size_t o = 0, got = 0, cap = 8, at = 0;
  int i;
  memset(a, 0, sizeof *a);
  if (!ap_pagelen(b, n)) return 0;
  a->codec = art_codec(b + AP_HDRMIN + b[26], n - AP_HDRMIN - b[26]);
  if (a->codec < 0) return 0;
  a->nhp = art_nhp(a->codec);
  a->serial = ap_rd32(b + 14);
  for (i = 0; i < ART_MAXHP; i++) bb_init(&a->pk[i]);
  a->ptype = (u8 *) xmalloc(cap);
  a->pgran = (u64 *) xmalloc(cap * sizeof *a->pgran);
  a->pseq  = (u32 *) xmalloc(cap * sizeof *a->pseq);
  a->pbrk  = (u64 *) xmalloc(cap * sizeof *a->pbrk);
  while (o < n) {
    size_t len = ap_pagelen(b + o, n - o), hl;
    const u8 * seg;
    const u8 * body;
    size_t k = 0;
    int ns;
    if (!len) { apr_free(a);  return 0; }
    if (ap_rd32(b + o + 14) != a->serial) { apr_free(a);  return 0; }
    ns = b[o + 26];  hl = AP_HDRMIN + (size_t) ns;
    seg = b + o + AP_HDRMIN;  body = b + o + hl;
    if (a->npg == cap) {
      cap *= 2;
      a->ptype = (u8 *) xrealloc(a->ptype, cap);
      a->pgran = (u64 *) xrealloc(a->pgran, cap * sizeof *a->pgran);
      a->pseq  = (u32 *) xrealloc(a->pseq, cap * sizeof *a->pseq);
      a->pbrk  = (u64 *) xrealloc(a->pbrk, cap * sizeof *a->pbrk);
    }
    a->ptype[a->npg] = b[o + 5];
    a->pgran[a->npg] = 0;
    for (i = 0; i < 8; i++) a->pgran[a->npg] |= (u64) b[o + 6 + i] << (8 * i);
    a->pseq[a->npg] = ap_rd32(b + o + 18);
    for (i = 0; i < ns; i++) {
      if (got >= (size_t) a->nhp) { apr_free(a);  return 0; }
      bb_put(&a->pk[got], body + k, seg[i]);
      k += seg[i];
      if (seg[i] < 255) got++;
    }
    at += k;
    a->pbrk[a->npg] = at;
    a->npg++;
    o += len;
  }
  if (o != n || got != (size_t) a->nhp) { apr_free(a);  return 0; }
  return 1;
}

/*  ------------------------------------------------------------ pagination  */

/*  A page's lacing is fully determined by the packet lengths and the byte
    offsets where pages break, so a layout is recorded as break offsets.  */

typedef struct { u8 * seg; size_t nseg, cap; u64 * cum; u8 * starts; } seglist;

static void seg_build(const size_t * pl, int npk, seglist * s) {
  int i;
  u64 at = 0;
  memset(s, 0, sizeof *s);
  s->cap = 64;
  s->seg = (u8 *) xmalloc(s->cap);
  s->cum = (u64 *) xmalloc((s->cap + 1) * sizeof *s->cum);
  s->starts = (u8 *) xmalloc(s->cap);
  for (i = 0; i < npk; i++) {
    size_t left = pl[i];
    int first = 1;
    for (;;) {
      u8 v = left >= 255 ? 255 : (u8) left;
      if (s->nseg == s->cap) {
        s->cap *= 2;
        s->seg = (u8 *) xrealloc(s->seg, s->cap);
        s->cum = (u64 *) xrealloc(s->cum, (s->cap + 1) * sizeof *s->cum);
        s->starts = (u8 *) xrealloc(s->starts, s->cap);
      }
      s->seg[s->nseg] = v;  s->cum[s->nseg] = at;  s->starts[s->nseg] = (u8) first;
      s->nseg++;  at += v;  first = 0;
      if (v < 255) break;
      left -= 255;
    }
  }
  s->cum[s->nseg] = at;
}
static void seg_free(seglist * s) { free(s->seg);  free(s->cum);  free(s->starts); }

static void ap_emit(bbuf * o, u8 type, u64 gran, u32 serial, u32 seq,
                    const u8 * segs, int nseg, const u8 * body, size_t blen) {
  u8 h[AP_HDRMIN];
  size_t at = o->n;
  int i;
  memcpy(h, "OggS", 4);  h[4] = 0;  h[5] = type;
  for (i = 0; i < 8; i++) h[6 + i] = (u8) (gran >> (8 * i));
  ap_wr32(h + 14, serial);  ap_wr32(h + 18, seq);
  memset(h + 22, 0, 4);  h[26] = (u8) nseg;
  bb_put(o, h, AP_HDRMIN);
  bb_put(o, segs, (size_t) nseg);
  bb_put(o, body, blen);
  ap_wr32(o->b + at + 22, ap_pagecrc(o->b + at, o->n - at));
}

/*  Lay packets out.  `brk` (npg entries, cumulative byte offsets) reproduces
    a recorded layout; NULL packs canonically -- the first packet alone, then
    the rest at 255 segments a page, which is what libvorbis and libopus do.
    Returns the number of pages written.  */
static size_t ap_region(bbuf * o, const u8 * const * pk, const size_t * pl,
                        int npk, u32 serial,
                        const u8 * type, const u64 * gran, const u32 * seq,
                        const u64 * brk, size_t npg) {
  seglist s;
  bbuf flat;
  size_t i = 0, page = 0;
  seg_build(pl, npk, &s);
  bb_init(&flat);
  for (i = 0; i < (size_t) npk; i++) bb_put(&flat, pk[i], pl[i]);
  i = 0;
  while (i < s.nseg) {
    size_t a = i, cnt = 0;
    u8 t;
    if (brk) {
      u64 want;
      if (page >= npg) die("the recorded page layout is short");
      want = brk[page];
      while (i < s.nseg && s.cum[i + 1] <= want) { i++;  cnt++; }
      if (s.cum[i] != want) die("the recorded page layout does not fit the packets");
    } else {
      while (i < s.nseg && cnt < 255) {
        i++;  cnt++;
        if (page == 0 && i < s.nseg && s.starts[i]) break;
      }
    }
    t = type ? type[page] : (u8) ((page == 0 ? 2 : 0) | (s.starts[a] ? 0 : 1));
    ap_emit(o, t, gran ? gran[page] : 0, serial, seq ? seq[page] : (u32) page,
            s.seg + a, (int) cnt, flat.b + s.cum[a],
            (size_t) (s.cum[i] - s.cum[a]));
    page++;
  }
  bb_free(&flat);
  seg_free(&s);
  return page;
}

/*  --------------------------------------------------------------- comments  */

typedef struct {
  const u8 * name;  size_t nlen;
  const u8 * val;   size_t vlen;
  const u8 * raw;   size_t rlen;
} vtag;

typedef struct {
  const u8 * pre;  size_t prelen;     /*  "\x03vorbis" or "OpusTags"  */
  const u8 * vendor;  size_t vlen;
  vtag * t;  size_t nt;
  const u8 * tail;  size_t tlen;      /*  framing byte, or Opus padding  */
} comment;

/*  Returns 0 on anything malformed rather than dying: a comment packet we do
    not understand simply keeps its art.  */
static int cm_parse(const u8 * p, size_t n, int codec, comment * c) {
  size_t at = codec ? 8 : 7, i;
  u32 nt;
  memset(c, 0, sizeof *c);
  if (n < at + 4) return 0;
  if (memcmp(p, codec ? (const void *) "OpusTags" : (const void *) "\x03vorbis", at))
    return 0;
  c->pre = p;  c->prelen = at;
  c->vlen = ap_rd32(p + at);  at += 4;
  if (c->vlen > n - at) return 0;
  c->vendor = p + at;  at += c->vlen;
  if (at + 4 > n) return 0;
  nt = ap_rd32(p + at);  at += 4;
  if (nt > (n - at) / 4) return 0;
  c->nt = nt;
  c->t = (vtag *) xmalloc((nt ? nt : 1) * sizeof *c->t);
  for (i = 0; i < nt; i++) {
    u32 l;
    const u8 * eq;
    if (at + 4 > n) { free(c->t);  c->t = NULL;  return 0; }
    l = ap_rd32(p + at);  at += 4;
    if (l > n - at) { free(c->t);  c->t = NULL;  return 0; }
    c->t[i].raw = p + at;  c->t[i].rlen = l;
    eq = (const u8 *) memchr(p + at, '=', l);
    if (eq) {
      c->t[i].name = p + at;  c->t[i].nlen = (size_t) (eq - (p + at));
      c->t[i].val = eq + 1;   c->t[i].vlen = l - c->t[i].nlen - 1;
    } else {
      c->t[i].name = p + at;  c->t[i].nlen = l;
      c->t[i].val = p + at + l;  c->t[i].vlen = 0;
    }
    at += l;
  }
  c->tail = p + at;  c->tlen = n - at;
  return 1;
}

static void cm_build(bbuf * o, const comment * c, const u8 * const * raw,
                     const size_t * rlen, size_t nt) {
  u8 w[4];
  size_t i;
  bb_put(o, c->pre, c->prelen);
  ap_wr32(w, (u32) c->vlen);  bb_put(o, w, 4);
  bb_put(o, c->vendor, c->vlen);
  ap_wr32(w, (u32) nt);  bb_put(o, w, 4);
  for (i = 0; i < nt; i++) {
    ap_wr32(w, (u32) rlen[i]);  bb_put(o, w, 4);
    bb_put(o, raw[i], rlen[i]);
  }
  bb_put(o, c->tail, c->tlen);
}

/*  ----------------------------------------------------------------- base64  */

static const char ART_B64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*  Strict: no whitespace, no stray bytes, canonical padding, and zero
    leftover bits -- anything else would not survive a re-encode.  */
static int b64_decode(const u8 * s, size_t n, bbuf * o) {
  size_t i;
  u32 acc = 0;
  int have = 0, pad = 0;
  if (!n || n % 4) return 0;
  for (i = 0; i < n; i++) {
    const char * q;
    int v;
    if (s[i] == '=') {
      if (i + 2 < n) return 0;
      pad++;  continue;
    }
    if (pad) return 0;
    q = (const char *) memchr(ART_B64, s[i], 64);
    if (!q) return 0;
    v = (int) (q - ART_B64);
    acc = (acc << 6) | (u32) v;  have += 6;
    if (have >= 8) { have -= 8;  bb_u8(o, (u8) (acc >> have)); }
  }
  if (pad > 2) return 0;
  if (have && (acc & ((1u << have) - 1))) return 0;
  return 1;
}

static void b64_encode(const u8 * d, size_t n, bbuf * o) {
  size_t i = 0;
  while (i + 3 <= n) {
    u32 v = ((u32) d[i] << 16) | ((u32) d[i + 1] << 8) | d[i + 2];
    bb_u8(o, (u8) ART_B64[v >> 18]);        bb_u8(o, (u8) ART_B64[(v >> 12) & 63]);
    bb_u8(o, (u8) ART_B64[(v >> 6) & 63]);  bb_u8(o, (u8) ART_B64[v & 63]);
    i += 3;
  }
  if (n - i == 1) {
    u32 v = (u32) d[i] << 16;
    bb_u8(o, (u8) ART_B64[v >> 18]);  bb_u8(o, (u8) ART_B64[(v >> 12) & 63]);
    bb_u8(o, '=');  bb_u8(o, '=');
  } else if (n - i == 2) {
    u32 v = ((u32) d[i] << 16) | ((u32) d[i + 1] << 8);
    bb_u8(o, (u8) ART_B64[v >> 18]);        bb_u8(o, (u8) ART_B64[(v >> 12) & 63]);
    bb_u8(o, (u8) ART_B64[(v >> 6) & 63]);  bb_u8(o, '=');
  }
}

/*  -------------------------------------------------------------- art tags  */

/*  Split a decoded blob into the bytes wrapping the image and the image.
    METADATA_BLOCK_PICTURE carries a FLAC PICTURE header; a legacy COVERART
    is the image on its own.  */
static int art_split(const u8 * name, size_t nlen, const u8 * d, size_t n,
                     size_t * envlen) {
  size_t at, l;
  if (nlen == 8 && !memcmp(name, "COVERART", 8)) { *envlen = 0;  return 1; }
  if (!(nlen == 22 && !memcmp(name, "METADATA_BLOCK_PICTURE", 22))) return 0;
  if (n < 32) return 0;
  at = 4;
  l = ap_rd32be(d + at);  at += 4;            /*  mime  */
  if (l > n - at) return 0;
  at += l;
  if (at + 4 > n) return 0;
  l = ap_rd32be(d + at);  at += 4;            /*  description  */
  if (l > n - at) return 0;
  at += l;
  if (at + 20 > n) return 0;
  at += 16;                                   /*  w, h, depth, colours  */
  l = ap_rd32be(d + at);  at += 4;            /*  data length  */
  if (l != n - at) return 0;
  *envlen = at;
  return 1;
}

/*  The image's own magic, not the declared MIME, which is often wrong.  */
static const char * art_ext(const u8 * d, size_t n) {
  if (n >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF) return "jpg";
  if (n >= 8 && !memcmp(d, "\x89PNG\r\n\x1a\n", 8)) return "png";
  if (n >= 6 && (!memcmp(d, "GIF87a", 6) || !memcmp(d, "GIF89a", 6))) return "gif";
  if (n >= 2 && !memcmp(d, "BM", 2)) return "bmp";
  if (n >= 12 && !memcmp(d, "RIFF", 4) && !memcmp(d + 8, "WEBP", 4)) return "webp";
  return "bin";
}

/*  ---------------------------------------------------------------- records  */

typedef struct {
  size_t idx;                 /*  position in the original tag list  */
  bbuf name, env, img;
  const char * ext;
  u32 file;                   /*  which image file holds it  */
} artimg;

typedef struct {
  int codec;
  artimg * img;  size_t nimg;
  u8 * ptype;  u64 * pgran;  u32 * pseq;  u64 * pbrk;  size_t npg;
  long seqdelta;              /*  added back to every page after the region  */
} artrec;

static void art_free(artrec * r) {
  size_t i;
  for (i = 0; i < r->nimg; i++) {
    bb_free(&r->img[i].name);  bb_free(&r->img[i].env);  bb_free(&r->img[i].img);
  }
  free(r->img);  free(r->ptype);  free(r->pgran);  free(r->pseq);  free(r->pbrk);
  memset(r, 0, sizeof *r);
}

static void art_put(bbuf * o, const artrec * r) {
  size_t i;
  bb_var(o, r->nimg);
  for (i = 0; i < r->nimg; i++) {
    const artimg * a = r->img + i;
    size_t el = strlen(a->ext);
    bb_var(o, a->idx);
    bb_var(o, a->name.n);  bb_put(o, a->name.b, a->name.n);
    bb_var(o, a->env.n);   bb_put(o, a->env.b, a->env.n);
    bb_var(o, a->file);
    bb_var(o, a->img.n);
    bb_var(o, el);  bb_put(o, a->ext, el);
  }
  bb_var(o, r->npg);
  for (i = 0; i < r->npg; i++) {
    bb_u8(o, r->ptype[i]);
    bb_var(o, r->pgran[i]);
    bb_var(o, r->pseq[i]);
    bb_var(o, r->pbrk[i]);
  }
  { long d = r->seqdelta;
    bb_var(o, (u64) (((u64) d << 1) ^ (u64) (d >> 31))); }
}

/*  `getimg` hands back the bytes of image file `file`; the frontend owns the
    naming and the deduplication.  */
typedef void (*art_imgfn)(void * ctx, u32 file, const char * ext, size_t len,
                          bbuf * out);

static void art_get(rdr * rd, artrec * r, art_imgfn getimg, void * ctx) {
  size_t i;
  memset(r, 0, sizeof *r);
  r->nimg = (size_t) rd_var(rd);
  r->img = (artimg *) xmalloc((r->nimg ? r->nimg : 1) * sizeof *r->img);
  for (i = 0; i < r->nimg; i++) {
    artimg * a = r->img + i;
    size_t k, ilen, el;
    static char ext[8][16];
    memset(a, 0, sizeof *a);
    a->idx = (size_t) rd_var(rd);
    k = (size_t) rd_var(rd);  bb_init(&a->name);  bb_put(&a->name, rd_bytes(rd, k), k);
    k = (size_t) rd_var(rd);  bb_init(&a->env);   bb_put(&a->env, rd_bytes(rd, k), k);
    a->file = (u32) rd_var(rd);
    ilen = (size_t) rd_var(rd);
    el = (size_t) rd_var(rd);
    if (el >= 15) die("corrupt image extension in the metainfo");
    { char * e = ext[i & 7];
      memcpy(e, rd_bytes(rd, el), el);  e[el] = 0;  a->ext = e; }
    bb_init(&a->img);
    getimg(ctx, a->file, a->ext, ilen, &a->img);
    if (a->img.n != ilen)
      die("image file %08X is %llu bytes, the metainfo says %llu",
          a->file, (unsigned long long) a->img.n, (unsigned long long) ilen);
  }
  r->npg = (size_t) rd_var(rd);
  r->ptype = (u8 *) xmalloc(r->npg ? r->npg : 1);
  r->pgran = (u64 *) xmalloc((r->npg ? r->npg : 1) * sizeof *r->pgran);
  r->pseq  = (u32 *) xmalloc((r->npg ? r->npg : 1) * sizeof *r->pseq);
  r->pbrk  = (u64 *) xmalloc((r->npg ? r->npg : 1) * sizeof *r->pbrk);
  for (i = 0; i < r->npg; i++) {
    r->ptype[i] = rd_u8(rd);
    r->pgran[i] = rd_var(rd);
    r->pseq[i] = (u32) rd_var(rd);
    r->pbrk[i] = rd_var(rd);
  }
  { u64 z = rd_var(rd);
    r->seqdelta = (long) ((z >> 1) ^ (u64) (-(long long) (z & 1))); }
}

/*  ------------------------------------------------------------- the two ops */

/*  Rebuild the original header region from a stripped one plus its record.  */
static void art_rebuild(const u8 * hdr, size_t n, const artrec * r, bbuf * out) {
  apregion a;
  comment c;
  const u8 ** raw;
  size_t * rlen;
  size_t nt, i, si = 0, ai = 0, npg;
  bbuf cm, * built;
  const u8 * pk[ART_MAXHP];
  size_t pl[ART_MAXHP];

  if (!apr_parse(hdr, n, &a)) die("the stored header region does not parse");
  if (!cm_parse(a.pk[1].b, a.pk[1].n, a.codec, &c))
    die("the stored comment packet does not parse");

  nt = c.nt + r->nimg;
  raw = (const u8 **) xmalloc((nt ? nt : 1) * sizeof *raw);
  rlen = (size_t *) xmalloc((nt ? nt : 1) * sizeof *rlen);
  built = (bbuf *) xmalloc((r->nimg ? r->nimg : 1) * sizeof *built);
  for (i = 0; i < nt; i++) {
    if (ai < r->nimg && r->img[ai].idx == i) {
      bbuf blob;
      bb_init(&blob);
      bb_put(&blob, r->img[ai].env.b, r->img[ai].env.n);
      bb_put(&blob, r->img[ai].img.b, r->img[ai].img.n);
      bb_init(&built[ai]);
      bb_put(&built[ai], r->img[ai].name.b, r->img[ai].name.n);
      bb_u8(&built[ai], '=');
      b64_encode(blob.b, blob.n, &built[ai]);
      bb_free(&blob);
      raw[i] = built[ai].b;  rlen[i] = built[ai].n;
      ai++;
    } else {
      if (si >= c.nt) die("the record does not match the stored comment");
      raw[i] = c.t[si].raw;  rlen[i] = c.t[si].rlen;  si++;
    }
  }
  if (si != c.nt) die("the record does not account for every tag");

  bb_init(&cm);
  cm_build(&cm, &c, raw, rlen, nt);
  for (i = 0; i < (size_t) a.nhp; i++) { pk[i] = a.pk[i].b;  pl[i] = a.pk[i].n; }
  pk[1] = cm.b;  pl[1] = cm.n;
  npg = ap_region(out, pk, pl, a.nhp, a.serial,
                  r->ptype, r->pgran, r->pseq, r->pbrk, r->npg);
  if (npg != r->npg)
    die("rebuilt %llu header pages, the record says %llu",
        (unsigned long long) npg, (unsigned long long) r->npg);
  bb_free(&cm);
  for (i = 0; i < r->nimg; i++) bb_free(&built[i]);
  free(built);  free(raw);  free(rlen);  free(c.t);
  apr_free(&a);
}

/*  Strip art out of a header region.  On success `out` holds the replacement
    region and `r` the record; returns 0 (leaving both untouched) when there
    is nothing to take or when the result would not rebuild exactly.  */
static int art_strip(const u8 * hdr, size_t n, u64 minsize, artrec * r,
                     bbuf * out, int verbose) {
  apregion a;
  comment c;
  const u8 ** raw;
  size_t * rlen;
  size_t i, si = 0, npgnew;
  bbuf cm;
  const u8 * pk[ART_MAXHP];
  size_t pl[ART_MAXHP];

  memset(r, 0, sizeof *r);
  if (!apr_parse(hdr, n, &a)) return 0;
  if (!cm_parse(a.pk[1].b, a.pk[1].n, a.codec, &c)) { apr_free(&a);  return 0; }
  r->codec = a.codec;

  r->img = (artimg *) xmalloc((c.nt ? c.nt : 1) * sizeof *r->img);
  raw = (const u8 **) xmalloc((c.nt ? c.nt : 1) * sizeof *raw);
  rlen = (size_t *) xmalloc((c.nt ? c.nt : 1) * sizeof *rlen);
  for (i = 0; i < c.nt; i++) {
    int take = 0;
    if (c.t[i].nlen == 8 || c.t[i].nlen == 22) {
      bbuf dec;
      size_t envlen = 0;
      bb_init(&dec);
      if (b64_decode(c.t[i].val, c.t[i].vlen, &dec) &&
          art_split(c.t[i].name, c.t[i].nlen, dec.b, dec.n, &envlen) &&
          dec.n - envlen >= minsize) {
        bbuf re;
        bb_init(&re);
        b64_encode(dec.b, dec.n, &re);
        take = re.n == c.t[i].vlen && !memcmp(re.b, c.t[i].val, re.n);
        if (!take && verbose)
          fprintf(stderr, "         %.*s: base64 is not canonical, left in place\n",
                  (int) c.t[i].nlen, (const char *) c.t[i].name);
        bb_free(&re);
      }
      if (take) {
        artimg * q = r->img + r->nimg++;
        memset(q, 0, sizeof *q);
        q->idx = i;
        bb_init(&q->name);  bb_put(&q->name, c.t[i].name, c.t[i].nlen);
        bb_init(&q->env);   bb_put(&q->env, dec.b, envlen);
        bb_init(&q->img);   bb_put(&q->img, dec.b + envlen, dec.n - envlen);
        q->ext = art_ext(q->img.b, q->img.n);
      }
      bb_free(&dec);
    }
    if (!take) { raw[si] = c.t[i].raw;  rlen[si] = c.t[i].rlen;  si++; }
  }

  if (!r->nimg) {
    art_free(r);  free(raw);  free(rlen);  free(c.t);  apr_free(&a);
    return 0;
  }

  bb_init(&cm);
  cm_build(&cm, &c, raw, rlen, si);
  for (i = 0; i < (size_t) a.nhp; i++) { pk[i] = a.pk[i].b;  pl[i] = a.pk[i].n; }
  pk[1] = cm.b;  pl[1] = cm.n;
  bb_init(out);
  npgnew = ap_region(out, pk, pl, a.nhp, a.serial, NULL, NULL, NULL, NULL, 0);

  r->npg = a.npg;
  r->ptype = (u8 *) xmalloc(r->npg);
  r->pgran = (u64 *) xmalloc(r->npg * sizeof *r->pgran);
  r->pseq  = (u32 *) xmalloc(r->npg * sizeof *r->pseq);
  r->pbrk  = (u64 *) xmalloc(r->npg * sizeof *r->pbrk);
  memcpy(r->ptype, a.ptype, r->npg);
  memcpy(r->pgran, a.pgran, r->npg * sizeof *r->pgran);
  memcpy(r->pseq, a.pseq, r->npg * sizeof *r->pseq);
  memcpy(r->pbrk, a.pbrk, r->npg * sizeof *r->pbrk);
  r->seqdelta = (long) a.npg - (long) npgnew;

  bb_free(&cm);  free(raw);  free(rlen);  free(c.t);  apr_free(&a);

  /*  Nothing is returned until it has been put back and compared.  */
  {
    bbuf back;
    int ok;
    bb_init(&back);
    art_rebuild(out->b, out->n, r, &back);
    ok = back.n == n && !memcmp(back.b, hdr, n);
    bb_free(&back);
    if (!ok) {
      if (verbose)
        fprintf(stderr, "         the header region would not rebuild exactly, "
                        "left alone\n");
      art_free(r);  bb_free(out);
      return 0;
    }
  }
  return 1;
}

#endif
