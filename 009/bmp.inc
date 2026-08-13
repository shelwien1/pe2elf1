// bmp.inc -- the BMP file, as this program holds it in memory.

struct BmfImage {
  uint16_t width;
  uint16_t height;
  uint32_t stride;
  uint8_t _pad8[2];
  uint8_t depth;
  uint8_t flags;
  uint32_t data_size;
  uint8_t pixels[0];
};

int32_t*__alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed) {
  uint8_t* buf;
  int32_t* result;
  bool byte_rows;
  int32_t bits, word2, row_pack, row_bytes, data_bytes, pal_bytes;
  uint16_t row16;
  row16 = img_w;
  bits = bpp;
  word2 = (uint8_t)bpp<<16;
  if( bpp>=5&&bpp<=7 ) {
    row_bytes = row16;
  } else {
    if( bpp==3 )
      bits = 4;
    else if( !packed )
      bits = bpp>4 ? bpp : 4;
    byte_rows = bpp!=3&&bits>=8;
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
      word2 = ((uint8_t)bpp<<16)|0x40000000;
      row_bytes = (uint16_t)row_pack;
    }
  }
  data_bytes = row_bytes*img_h;
  if( palette ) {
    pal_bytes = 3<<(bpp&31);
    if( bpp>8 )
      pal_bytes = 0;
    else
      word2 |= 0x00800000;
  } else {
    pal_bytes = 0;
  }
  result = ((int32_t*)bmf_new(data_bytes+pal_bytes+19));
  if( !result )
    return nullptr;
  BmfImage*const img = (BmfImage*)result;
  img->width = img_w;
  img->height = img_h;
  img->stride = row_bytes;
  result[2] = word2;
  img->data_size = data_bytes;
  if( pal_bytes ) {
    if( (((const BmfImage*)result)->depth&0x80)!=0 )
      buf = (uint8_t*)result+result[3]+16;
    else
      buf = nullptr;
    memset(buf, 0, pal_bytes);
    return result;
  }
  return result;
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

static inline int32_t bmf_pixels(const uint8_t* p) {
  const BmfImage* img = (const BmfImage*)p;
  return img->width*img->height;
}
