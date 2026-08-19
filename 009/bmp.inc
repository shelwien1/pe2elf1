// bmp.inc -- the BMP file, as this program holds it in memory.

// The two decision bytes, one enum each.  They are adjacent in the header and
// they both use 0x40, for two unrelated things -- packed rows in one, a grey
// ramp in the other -- so an 0x40 left to stand on its own says which field it
// came from and nothing else.  Grouping them also puts each byte on the page
// as a byte: what bits exist, and which are holes.
//
// `depth` is a width in its low six bits and two statements about the palette
// in its top two.
enum : uint8_t {
  depth_bits    = 0x3F,   // bits per pixel
  depth_grey    = 0x40,   // the palette is a grey ramp
  depth_palette = 0x80,   // a palette follows the pixels

  // One eight-bit greyscale plane, which is what a header describing a single
  // plane of a colour image says.  Both sides of the planar path copy the real
  // header, overwrite `depth` with this, and hand the copy to the model; both
  // wrote it as `72`.
  depth_one_plane = depth_grey|8,
};

// `flags` is the same byte in memory and on disk: `compress_image` builds its
// header as a `BmfImage` and writes the sixteen bytes out, `expand_image` reads
// them back into one.  Two bits describe the image, four describe the stream
// that follows it, and one says whether anything sits between the two.
//
// Bit 0 is unused.  `flags_packed` is written by `alloc_image` and read by
// nothing -- in this program or in the file -- which is a fact about BMF 2.01
// rather than about the decompilation, and it is kept and named because a bit
// the encoder sets is part of the format whether or not the decoder consults
// it.
enum : uint8_t {
  flags_transposed  = 0x02, // width and height are swapped
  flags_slow        = 0x04, // coded in -S mode; this build decodes no other
  flags_planar      = 0x08, // each plane coded alone, interleaved afterwards
  flags_descriptors = 0x10, // a plane descriptor per plane heads the stream
  flags_coded       = 0x20, // the data is a coded stream, not raw pixels
  flags_packed      = 0x40, // rows are packed sub-byte
  flags_tail        = 0x80, // a `CodedTail` sits between header and data
};

// A BMP palette entry is four bytes -- blue, green, red, and a zero reserved
// byte -- so little-endian makes the word `0x00RRGGBB`.  The image carries its
// own palette as three-byte BGR triples in that same order, so writing one out
// is a widening by one zero byte, and the shift chains at the call sites showed
// that rather than saying it.
//
// Taking the three channels by name is the point: `(pal[2]<<16)|*(uint16_t*)&pal[0]`
// reads as a two-byte load with a byte on top, and which byte is which is left
// to the reader.
static inline uint32_t bmp_palette_entry(uint8_t b, uint8_t g, uint8_t r) {
  return (uint32_t)r<<16|(uint32_t)g<<8|b;
}

// One 32-bit value into a byte buffer at an arbitrary offset.  A BMP's palette
// starts at byte 54, so no entry in it is four-aligned and none of the six
// sites that write one could be a `uint32_t*` store -- they were
// `*(uint32_t*)&out_buf[4*slot+54]`, which is the store the binary makes and a
// misaligned access as C++.  `memcpy` is the same store with the alignment
// told truthfully; every compiler this builds under folds it to one `mov`.
static inline void put_u32(uint8_t* at, uint32_t v) {
  memcpy(at, &v, sizeof v);
}

// One level of a grey ramp: the same value in all three channels.
static inline uint32_t bmp_grey_entry(uint8_t level) {
  return bmp_palette_entry(level, level, level);
}

// The sixteen-byte member header, on disk and in memory: `write_member_head`
// puts this record out with `fwrite(head, 1, 0x10, fp)` and `expand_image`
// reads it back the same way, so the field offsets below *are* the format.
struct BmfImage {
  uint16_t width;
  uint16_t height;
  // Sixteen bits, and the two bytes it gave up are pad.
  //
  // It was `uint32_t` and every read masked it to sixteen -- seven sites -- and
  // every write truncated first.  Seven unexplained masks are a format limit
  // nobody wrote down, so this writes it down: a row is at most 65535 bytes.
  // `read_bmp` already refuses a wider one (`width * ((bpp+7)>>3) > 0xFFFF`);
  // `expand_image` did not, and 11.3 is that gap -- `width` is `uint16_t`, so a
  // crafted header at 16384 by 32 bits asks for a 262140-byte row, and the
  // truncation made it 0 rather than an error.  The check is there now.
  //
  // The two bytes move into `_pad8` rather than shrinking the record, because
  // the record is the format.  On every stream this program has ever written
  // they were the high half of a stride below 65536, which is to say zero.
  uint16_t stride;
  uint8_t _pad8[4];
  uint8_t depth;
  uint8_t flags;
  uint32_t data_size;
  uint8_t pixels[0];

  // The palette, when `depth & 0x80` says there is one: it sits immediately
  // after the pixels.  Four sites spelled this `(uint8_t*)img + data_size+16`,
  // where the 16 is `sizeof(BmfImage)` -- the header this same block starts
  // with, so `pixels` already is it.
  uint8_t* palette() { return pixels+data_size; }
  const uint8_t* palette() const { return pixels+data_size; }

  // `n` bytes from `at` are inside this image's pixels.  Both run-length
  // readers spelled the two bounds into a pair of locals at the top and
  // compared against them four times between them, which is the same sentence
  // written four ways.
  bool holds(const uint8_t* at, int32_t n) const {
    return at>=pixels&&at+n<=pixels+data_size;
  }

  // Every header field of `src` but `flags`, which the two callers set
  // differently: `compress_image` writes the member header twice, once at the
  // top and once after `search_filter`, which may have transposed the image
  // under it.
  //
  // `_pad8[2]`, `depth` and `flags` are four consecutive bytes at offset 8, and
  // both copies moved them as one `uint32_t` -- then the second recovered
  // `flags` from the top of the saved word twenty lines later.  Member copies
  // say it without the layout.  Each copy also spelled the stride into a local
  // of its own, `row_bytes` and `img_stride`, read once on the next line but
  // one.
  // The first byte of row `y`, counting from the bottom -- which is where a
  // BMP's rows start.  Both run-length arms of `read_bmp` compute it when an
  // end-of-line opcode sends them up a row, and both took the stride through a
  // `uint16_t` to match the stores that made it.  The field is sixteen bits
  // now, so the cast is gone from here and from the six other read sites.
  uint8_t* row(int32_t y) { return pixels+y*stride; }

  // All four pad bytes, not two.  Two of them used to be the high half of a
  // 32-bit stride and were carried by the `stride` copy; with a sixteen-bit
  // field nothing else moves them, and they are part of a header that goes to
  // disk byte for byte.
  void header_from(const BmfImage* src) {
    width = src->width;
    height = src->height;
    stride = src->stride;
    for( int32_t k = 0; k<4; ++k )
      _pad8[k] = src->_pad8[k];
    depth = src->depth;
    data_size = src->data_size;
  }
};

// The format, asserted where the fields are declared.  `write_member_head`
// writes 0x10 bytes of this and `expand_image` reads 0x10 back, so a change to
// any offset below is a change to the container.
static_assert(sizeof(BmfImage) == 16, "the member header is sixteen bytes");
static_assert(offsetof(BmfImage, width) == 0 && offsetof(BmfImage, height) == 2
           && offsetof(BmfImage, stride) == 4 && offsetof(BmfImage, _pad8) == 6
           && offsetof(BmfImage, depth) == 10 && offsetof(BmfImage, flags) == 11
           && offsetof(BmfImage, data_size) == 12,
              "member header field offsets are the on-disk layout");

// Transpose the header.  The loop at each call site has already moved the
// pixels; this swaps the two dimensions that describe them, recomputes the
// stride, and flips `flags_transposed`, which is the record that it happened.
//
// Open-coded at five sites, in two spellings that are not obviously the same
// store: three wrote `*(uint16_t*)&img->stride`, which leaves the top half of
// the 32-bit field alone, and two assigned the whole word.  What makes them
// one function is that the *value* is sixteen bits wide at all five -- it goes
// through a `uint16_t` or `int16_t` temporary before the store every time, and
// the reads on these paths are `(uint16_t)img->stride` to match.  A stride
// that did not fit in sixteen bits would already be wrong here in the
// original; given that it fits, zeroing the top half and leaving it alone are
// the same thing, and the whole-word store is the one that says so.
static inline void transpose_image_in_place(BmfImage* img, int32_t planes) {
  uint16_t w = img->width;
  img->width = img->height;
  img->height = w;
  img->flags ^= flags_transposed;
  img->stride = img->width*planes;
}

// And the loop that has to run before it: the pixels themselves, column-major.
// A copy goes aside and comes back so that pixel (x, y) lands at (y, x), all
// `planes` bytes of it moving together.
//
// Open-coded at four sites -- three in `search_filter` and one in
// `compress_image` -- each identical down to the shape of its three nested
// `do`/`while`s, and each needing one value the loop cannot recompute: the
// `planes*(height-1)` the destination advances by per column.  All three in
// `search_filter` kept it in a frame slot, two of them in the same slot, which
// is most of what that frame was for.
//
// The inner `do`/`while`s ran at least once whatever the count, so a zero
// width or a zero plane count would have wrapped; `for` is the shape that says
// what the bounds are.  Neither is reachable -- `search_filter` returns early
// below four columns and `plane_count` is 1..4 -- so this is the same walk on
// every input either can see.
static void transpose_image(BmfImage* img, int32_t planes) {
  uint8_t* copy = (uint8_t*)bmf_new(img->data_size);
  memcpy(copy, img->pixels, img->data_size);
  const int32_t step = planes*(img->height-1);
  const uint8_t* src = copy;
  for( int32_t y = 0; y<img->height; ++y ) {
    uint8_t* dst = img->pixels+planes*y;
    for( int32_t x = 0; x<img->width; ++x ) {
      for( int32_t k = 0; k<planes; ++k )
        *dst++ = *src++;
      dst += step;
    }
  }
  transpose_image_in_place(img, planes);
  free(copy);
}

// The allocation is one block: a sixteen-byte `BmfImage`, the pixels, and the
// palette after them if there is one.
//
// It used to build the header's `(pad, pad, depth, flags)` word by arithmetic
// -- `bpp<<16` for `depth`, `0x40000000` for `flags_packed`, `0x00800000` for
// `depth_palette` -- and store it through `result[2]`, because the original
// wrote it as one store.  The two decision bytes are written as themselves
// now, which is also what let the return type stop being `int32_t*`.
BmfImage*alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed) {
  int32_t row_pack, row_bytes, pal_bytes;
  bool sub_byte_rows = false;
  uint16_t row16 = img_w;
  int32_t bits = bpp;
  if( bpp>=5&&bpp<=7 ) {
    row_bytes = row16;
  } else {
    if( bpp==3 )
      bits = 4;
    else if( !packed )
      bits = max32(bpp, 4);
    bool byte_rows = bpp!=3&&bits>=8;
    if( !packed ) {
      if( !byte_rows&&bits==4 )
        row16 = ((img_w+7)>>1)&0xFFFC;
      else
        row16 = (uint16_t)(((bits+7)>>3)*img_w);
      row_bytes = row16;
    } else if( byte_rows ) {
      row16 = (uint16_t)(((bits+7)>>3)*img_w);
      row_bytes = row16;
    } else {
      if( bits==1 ) {
        row_pack = (int32_t)(((uint32_t)((img_w+7)>>2)>>29)+img_w+7)>>3;
      } else if( bits==2 ) {
        row_pack = (int32_t)(((uint32_t)((img_w+3)>>1)>>30)+img_w+3)>>2;
      } else {
        row_pack = (int32_t)(img_w+((uint32_t)(img_w+1)>>31)+1)>>1;
      }
      sub_byte_rows = true;
      row_bytes = (uint16_t)row_pack;
    }
  }
  int32_t data_bytes = row_bytes*img_h;
  bool has_palette = false;
  if( palette&&bpp<=8 ) {
    pal_bytes = 3<<(bpp&31);
    has_palette = true;
  } else {
    pal_bytes = 0;
  }
  // 19, not 16: three bytes of slop past the header, pixels and palette, which
  // the original allocated and this keeps.
  BmfImage* img = (BmfImage*)bmf_new(data_bytes+pal_bytes+19);
  if( !img )
    return nullptr;
  img->width = img_w;
  img->height = img_h;
  img->stride = row_bytes;
  for( int32_t k = 0; k<4; ++k )
    img->_pad8[k] = 0;
  img->depth = (uint8_t)(bpp|(has_palette ? depth_palette : 0));
  img->flags = sub_byte_rows ? flags_packed : 0;
  img->data_size = data_bytes;
  // The clear used to be guarded by `depth & 0x80`, with a `nullptr` for the
  // other arm -- and reaching it needed `pal_bytes` non-zero, which is exactly
  // when that bit was just set.  The guard could not be false and the `nullptr`
  // could not be passed to `memset`.
  if( pal_bytes )
    memset(img->palette(), 0, pal_bytes);
  return img;
}

// The BMP header's own constants, which are Windows' rather than BMF's.
// `bfType` is the two letters `BM` little-endian; `biSize` is the size of the
// second half of the header, which is what distinguishes the only version
// either side here handles from OS/2's twelve-byte one and Windows' later
// longer ones; and `biCompression` is a three-value enum both `read_bmp` and
// `write_bmp` were spelling 0, 1 and 2.
enum : uint32_t {
  bmp_magic   = 'B'|'M'<<8,   // 0x4D42
  bmp_hdr_v3  = 40,           // BITMAPINFOHEADER, and no other
  bmp_rgb     = 0,            // rows as they are
  bmp_rle8    = 1,            // run-length, one byte per pixel
  bmp_rle4    = 2,            // run-length, one nibble per pixel
};

#pragma pack(push, 1)
struct BmpHeader {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
};
#pragma pack(pop)

static inline int32_t bmf_pixels(const BmfImage* img) {
  return img->width*img->height;
}
