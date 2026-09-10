
/*  Ogg page framing.  */

constexpr int OGG_HDRMIN = 27;
constexpr int OGG_MAXSEG = 255;

/*  Whole-page CRC helpers.  These work on the serialized bytes, not on a
    parsed page, so they stay free functions.  */
u32 ogg_crc(const u8 * d, sz n);
/*  Return the CRC a complete page should contain.  */
u32 ogg_crc_page(const u8 * p, sz n);
int ogg_crc_ok(const u8 * p, sz n);
/*  Update a complete page's CRC field.  */
void ogg_crc_set(u8 * p, sz n);

/*  The payload and the whole image of one page.  */
constexpr sz MAXPAY   = (sz) OGG_MAXSEG * OGG_MAXSEG;
constexpr sz PAGE_MAX = OGG_HDRMIN + OGG_MAXSEG + MAXPAY;

static u32 CRC[256];

static void crcinit(void) {
  u32 i, j, c;
  if (CRC[1]) return;
  Fi(256,
    c = i << 24;
    Fj(8, c = (c & (u32) 0x80000000) ? (c << 1) ^ (u32) 0x04C11DB7 : c << 1);
    CRC[i] = c);
}

static u32 crcrun(u32 c, const u8 * d, sz n) {
  sz i;
  Fi(n, c = (c << 8) ^ CRC[((c >> 24) ^ d[i]) & 0xFF]);
  return c;
}

static void wr32(u8 * b, u32 v) {
  b[0] = (u8) v;  b[1] = (u8) (v >> 8);  b[2] = (u8) (v >> 16);  b[3] = (u8) (v >> 24);
}

/*  Compute a page CRC while treating its stored CRC as zero.  */

static u32 rd32(const u8 * b) {
  return (u32) b[0] | ((u32) b[1] << 8) | ((u32) b[2] << 16) | ((u32) b[3] << 24);
}

struct ogg_page {
  u8 type;
  u32 glo, ghi;               /*  granule position, low and high half  */
  u32 serial, seq;
  int nseg;  u8 lace[OGG_MAXSEG];
  int np;  u32 plen[OGG_MAXSEG];
  int tail;                   /*  the last packet runs onto the next page  */
  sz blen;                    /*  payload bytes = sum of the lacing values  */

  /*  Parse the page starting at b; returns its total size, or 0 if b does not
      begin a complete, well-formed page.  Does not check the CRC.  */

  sz parse(const u8 * b, sz n) {
    sz i, hl;
    if (n < (sz) OGG_HDRMIN || memcmp(b, "OggS", 4) || b[4]) return 0;
    nseg = b[26];  hl = OGG_HDRMIN + (sz) nseg;
    if (n < hl) return 0;
    type = b[5];  glo = rd32(b + 6);  ghi = rd32(b + 10);
    serial = rd32(b + 14);  seq = rd32(b + 18);
    blen = 0;
    Fi((sz) nseg, lace[i] = b[OGG_HDRMIN + i];  blen += lace[i]);
    if (n < hl + blen) return 0;
    unpack();
    return hl + blen;
  }

  /*  Serialize a header and payload with a computed CRC.  */

  sz emit(u8 * out, const u8 * body) const {
    sz i, hl = OGG_HDRMIN + (sz) nseg;
    memcpy(out, "OggS", 4);  out[4] = 0;  out[5] = type;
    wr32(out + 6, glo);  wr32(out + 10, ghi);
    wr32(out + 14, serial);  wr32(out + 18, seq);
    out[26] = (u8) nseg;
    Fi((sz) nseg, out[OGG_HDRMIN + i] = lace[i]);
    memcpy(out + hl, body, blen);
    crc_set(out, hl + blen);
    return hl + blen;
  }

  /*  Convert between lacing values and packets.  */

  void unpack() {
    sz i;
    u32 run = 0;
    np = 0;  tail = 0;
    Fi((sz) nseg,
      run += lace[i];
      if (lace[i] != OGG_MAXSEG) { plen[np++] = run;  run = 0; });
    /*  A final 255 continues the packet on the next page.  */
    if (nseg && lace[nseg - 1] == OGG_MAXSEG) { plen[np++] = run;  tail = 1; }
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
        lace[n++] = OGG_MAXSEG;  v -= OGG_MAXSEG;
      }
      if (!(tail && i == np - 1)) {
        FATAL_UNLESS(n < (sz) OGG_MAXSEG, "page needs more than %d segments",
                     OGG_MAXSEG);
        lace[n++] = (u8) v;
      } else FATAL_UNLESS(!v, "continued packet has a partial segment"));
    nseg = (int) n;
  }

  /*  Compute a page CRC while treating its stored CRC as zero.  */
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
