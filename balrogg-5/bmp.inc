/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/*  A 24-bit BMP sink, one row of the record stream to one row of pixels.

    The height is not known until the stream ends, so the header is written
    twice: a placeholder first and the real one on close, which is what
    wav.inc does with the frame count for the same reason.  The height goes
    out negative -- a top-down BMP -- so that the first row read is the first
    row shown and rows can be written as they arrive rather than held and
    reversed.  */

constexpr sz BMP_HDR = 54;

struct bmp {
  FILE * f;
  const char * path;
  u32 w, rows, cap;
  u8 * line;                              /*  one padded row of pixels  */
  sz stride;
  u32 clipped;                            /*  rows wider than the image  */

  static void w32(u8 * p, u32 v) {
    p[0] = (u8) v;  p[1] = (u8) (v >> 8);
    p[2] = (u8) (v >> 16);  p[3] = (u8) (v >> 24);
  }
  static void w16(u8 * p, u32 v) { p[0] = (u8) v;  p[1] = (u8) (v >> 8); }

  void header(void) {
    u8 h[BMP_HDR];
    u32 img = (u32) (stride * rows);
    memset(h, 0, sizeof h);
    h[0] = 'B';  h[1] = 'M';
    w32(h + 2, (u32) BMP_HDR + img);      w32(h + 10, (u32) BMP_HDR);
    w32(h + 14, 40);
    w32(h + 18, w);
    w32(h + 22, (u32) (0u - rows));       /*  negative: rows run downward  */
    w16(h + 26, 1);  w16(h + 28, 24);
    w32(h + 34, img);
    w32(h + 38, 2835);  w32(h + 42, 2835);   /*  72 dpi  */
    if (fwrite(h, 1, sizeof h, f) != sizeof h)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
  }

  void create(const char * pth, u32 width, u32 maxrows) {
    FATAL_UNLESS(width > 0, "%s: a zero-width image", pth);
    f = fopen(pth, "wb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", pth);
    path = pth;  w = width;  rows = 0;  cap = maxrows;  clipped = 0;
    stride = ((sz) w * 3 + 3) & ~(sz) 3;
    line = (u8 *) malloc(stride);
    if (!line) FATAL_CODE(BLR_EXIT_IO, "out of memory for %s", pth);
    header();
  }

  int full(void) const { return rows >= cap; }

  /*  `n` colours, already in BGR order, and the rest of the row padded.  */
  void row(const u8 * bgr, u32 n) {
    if (rows >= cap) return;
    if (n > w) { n = w;  clipped++; }
    memset(line, 0, stride);              /*  black is the pad  */
    memcpy(line, bgr, (sz) n * 3);
    if (fwrite(line, 1, stride, f) != stride)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
    rows++;
  }

  void close(void) {
    int bad;
    if (!f) return;
    /*  An image with no rows would claim a zero height, which some readers
        reject outright; one blank row is a truer answer than a broken file.  */
    if (!rows) { memset(line, 0, stride);  fwrite(line, 1, stride, f);  rows = 1; }
    bad = fseek(f, 0, SEEK_SET) != 0;
    if (!bad) { header();  bad = ferror(f) != 0; }
    if (fclose(f)) bad = 1;
    free(line);  line = nullptr;  f = nullptr;
    if (bad) FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
  }
};

/*  The two ramps, as BGR triples.

    Both follow the house rule for their job: the floor's posts are a
    magnitude, so they take one hue light to dark; the residue digits are
    signed, so they take two hues with a neutral grey where the value is zero
    -- never a hue at the midpoint, and never a rainbow, because a rainbow
    reads as unordered and the reader would have to consult a key to tell a
    large value from a small one.

    Eight steps an arm, on a log ladder: 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63,
    64 and over.  A linear ramp would spend its whole range on the handful of
    large values and leave the small ones, which are nearly all of them,
    indistinguishable from zero.  */
constexpr u32 BMP_STEPS = 8;

/*  blue, light to dark -- the sequential ramp  */
static const u8 BMP_SEQ[BMP_STEPS][3] = {
  { 0xec, 0xef, 0xf0 }, { 0xfb, 0xe2, 0xcd }, { 0xf4, 0xc5, 0x9e },
  { 0xec, 0xa7, 0x6d }, { 0xe5, 0x87, 0x39 }, { 0xbf, 0x6a, 0x25 },
  { 0x95, 0x4f, 0x18 }, { 0x6b, 0x36, 0x0d }
};
/*  the warm arm; the cool one is the sequential ramp above, so a negative
    digit and a large floor post are the same blue and mean the same thing --
    "far from zero, on the cool side"  */
static const u8 BMP_POS[BMP_STEPS][3] = {
  { 0xec, 0xef, 0xf0 }, { 0xd6, 0xd7, 0xfb }, { 0xae, 0xb0, 0xf7 },
  { 0x82, 0x85, 0xf0 }, { 0x48, 0x49, 0xe3 }, { 0x2e, 0x2f, 0xc0 },
  { 0x1f, 0x1f, 0x95 }, { 0x12, 0x12, 0x6b }
};

/*  Which rung of the ladder a magnitude sits on.  */
static u32 bmp_step(i64 v) {
  u64 a = v < 0 ? (u64) 0 - (u64) v : (u64) v;
  u32 s = 0;
  while (a && s + 1 < BMP_STEPS) { a >>= 1;  s++; }
  return s;
}

static void bmp_seq(i64 v, u8 * out) { memcpy(out, BMP_SEQ[bmp_step(v)], 3); }
static void bmp_div(i64 v, u8 * out) {
  memcpy(out, v < 0 ? BMP_SEQ[bmp_step(v)] : BMP_POS[bmp_step(v)], 3);
}
