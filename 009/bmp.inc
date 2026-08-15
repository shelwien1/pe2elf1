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

// One level of a grey ramp: the same value in all three channels.
static inline uint32_t bmp_grey_entry(uint8_t level) {
  return bmp_palette_entry(level, level, level);
}

struct BmfImage {
  uint16_t width;
  uint16_t height;
  uint32_t stride;
  uint8_t _pad8[2];
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
};

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
  img->stride = (uint16_t)(img->width*planes);
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
  int32_t bits, row_pack, row_bytes, pal_bytes;
  uint16_t row16;
  bool sub_byte_rows = false;
  row16 = img_w;
  bits = bpp;
  if( bpp>=5&&bpp<=7 ) {
    row_bytes = row16;
  } else {
    if( bpp==3 )
      bits = 4;
    else if( !packed )
      bits = bpp>4 ? bpp : 4;
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
  img->_pad8[0] = 0;
  img->_pad8[1] = 0;
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
