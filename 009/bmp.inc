// bmp.inc -- the BMP file, as this program holds it in memory.

// The two decision bytes.  `depth` is a width in its low six bits and two
// statements about the palette in its top two; `flags` is separate, and its
// 0x40 means something else entirely -- which is why these are named rather
// than left as an 0x40 in each field meaning packed rows in one and a grey
// ramp in the other.
static constexpr uint8_t depth_bits = 0x3F;      // bits per pixel
static constexpr uint8_t depth_grey = 0x40;      // the palette is a grey ramp
static constexpr uint8_t depth_palette = 0x80;   // a palette follows the pixels
static constexpr uint8_t flags_transposed = 0x02;// width and height are swapped
static constexpr uint8_t flags_packed = 0x40;    // rows are packed sub-byte

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
// stride, and flips bit 1 of `flags`, which is the record that it happened.
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
  img->flags ^= 2u;
  img->stride = (uint16_t)(img->width*planes);
}

// The allocation is one block: a sixteen-byte `BmfImage`, the pixels, and the
// palette after them if there is one.
//
// It used to build the header's `(pad, pad, depth, flags)` word by arithmetic
// -- `bpp<<16` for `depth`, `0x40000000` for `flags & 0x40` (rows are sub-byte
// packed), `0x00800000` for `depth & 0x80` (a palette follows) -- and store it
// through `result[2]`, because the original wrote it as one store.  The two
// decision bytes are written as themselves now, which is also what let the
// return type stop being `int32_t*`.
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
  img->depth = (uint8_t)(bpp|(has_palette ? 0x80 : 0));
  img->flags = sub_byte_rows ? 0x40 : 0;
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
