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

/*  A band is one stream's rows, kept at that stream's own width.  The streams
    do not arrive interleaved -- the walk finishes a partition before it starts
    the next -- but they have to leave side by side, so the rows are held and
    the image assembled at the end.  Memory is then the data itself, with no
    padding stored; what bounds it is the row cap.  */
struct bmp_band {
  const char * name;
  u8 * px;
  u32 w, rows, cap;                       /*  cap: rows allocated  */

  void init(const char * nm, u32 width) {
    name = nm;  px = nullptr;  w = width;  rows = cap = 0;
  }
  void row(const u8 * bgr, u32 n) {
    if (rows == cap) {
      u32 want = cap ? cap * 2 : 256;
      u8 * q = (u8 *) realloc(px, (sz) want * w * 3);
      if (!q) FATAL_CODE(BLR_EXIT_IO,
                         "out of memory for %s at %" PRIu32 " rows; "
                         "TSVTRANS_BMPROWS lowers it", name, want);
      px = q;  cap = want;
    }
    { u8 * d = px + (sz) rows * w * 3;
      u32 k = n > w ? w : n;
      memset(d, 0, (sz) w * 3);
      memcpy(d, bgr, (sz) k * 3); }
    rows++;
  }
  void free_() { ::free(px);  px = nullptr;  rows = cap = 0; }
};

/*  Write `n` bands side by side: the image is as wide as they are together
    and as tall as the tallest, and a band that runs out is black below.

    The height is known here, so unlike wav.inc's sink this writes its header
    once.  It goes out negative -- a top-down BMP -- so the first row read is
    the first row shown.  */
static void bmp_save(const char * path, const bmp_band * b, u32 n) {
  FILE * f;
  u8 h[BMP_HDR], * line;
  u32 i, r, W = 0, H = 0;
  sz stride, img;
  for (i = 0; i < n; i++) { W += b[i].w;  if (b[i].rows > H) H = b[i].rows; }
  if (!W) W = 1;
  if (!H) H = 1;                          /*  a zero height is not a BMP  */
  stride = ((sz) W * 3 + 3) & ~(sz) 3;
  img = stride * H;
  f = fopen(path, "wb");
  if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", path);
  memset(h, 0, sizeof h);
  h[0] = 'B';  h[1] = 'M';
  { u8 * p = h;
    #define BMP_W32(o, v) do { u32 v_ = (u32) (v);  p[o] = (u8) v_; \
                               p[(o) + 1] = (u8) (v_ >> 8); \
                               p[(o) + 2] = (u8) (v_ >> 16); \
                               p[(o) + 3] = (u8) (v_ >> 24); } while (0)
    #define BMP_W16(o, v) do { u32 v_ = (u32) (v);  p[o] = (u8) v_; \
                               p[(o) + 1] = (u8) (v_ >> 8); } while (0)
    BMP_W32(2, (u32) BMP_HDR + img);  BMP_W32(10, (u32) BMP_HDR);
    BMP_W32(14, 40);  BMP_W32(18, W);  BMP_W32(22, 0u - H);
    BMP_W16(26, 1);   BMP_W16(28, 24);
    BMP_W32(34, img); BMP_W32(38, 2835);  BMP_W32(42, 2835);
    #undef BMP_W32
    #undef BMP_W16
  }
  line = (u8 *) malloc(stride);
  if (!line) { fclose(f);  FATAL_CODE(BLR_EXIT_IO, "out of memory for %s", path); }
  if (fwrite(h, 1, sizeof h, f) != sizeof h)
    { fclose(f);  free(line);  FATAL_CODE(BLR_EXIT_IO, "write error on %s", path); }
  for (r = 0; r < H; r++) {
    u8 * d = line;
    memset(line, 0, stride);
    for (i = 0; i < n; i++) {
      if (r < b[i].rows) memcpy(d, b[i].px + (sz) r * b[i].w * 3, (sz) b[i].w * 3);
      d += (sz) b[i].w * 3;
    }
    if (fwrite(line, 1, stride, f) != stride)
      { fclose(f);  free(line);  FATAL_CODE(BLR_EXIT_IO, "write error on %s", path); }
  }
  free(line);
  if (fclose(f)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
}

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
