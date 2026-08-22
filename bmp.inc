// bmp.inc -- the image container, the byte streams, and BMP reading/writing
// Included from bmf.cpp; not a standalone translation unit.

enum : uint8_t {
  depth_bits = 0x3F,
  depth_grey = 0x40,
  depth_palette = 0x80,
  depth_one_plane = depth_grey|8,
};

enum : uint8_t {
  flags_pal_sorted = 0x01,
  flags_transposed = 0x02,
  flags_slow = 0x04,
  flags_planar = 0x08,
  flags_descriptors = 0x10,
  flags_coded = 0x20,
  flags_packed = 0x40,
  flags_tail = 0x80,
};

// Bits of the member header's spare byte.  The flags byte is full, and these
// describe the pixels' representation rather than the coding, so they sit here.
enum : uint8_t {
  pad_ycocg = 0x01,
  pad_flip_x = 0x02,
  pad_flip_y = 0x04,
};

uint32_t bmp_palette_entry(uint8_t b, uint8_t g, uint8_t r) {
  return (uint32_t)r<<16|(uint32_t)g<<8|b;
}

void put_u32(uint8_t* at, uint32_t v) {
  memcpy(at, &v, sizeof v);
}

uint32_t bmp_grey_entry(uint8_t level) {
  return bmp_palette_entry(level, level, level);
}

struct BmfImage {
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  uint8_t _pad8[4];
  uint8_t depth;
  uint8_t flags;
  uint32_t data_size;
  uint8_t pixels[0];
  uint8_t*palette() {
    return pixels+data_size;
  }

  const uint8_t*palette() const {
    return pixels+data_size;
  }

  bool holds(const uint8_t* at, int32_t n) const {
    return at>=pixels&&at+n<=pixels+data_size;
  }

  uint8_t*row(int32_t y) {
    return pixels+y*stride;
  }

  void header_from(const BmfImage* src) {
    width = src[0].width;
    height = src[0].height;
    stride = src[0].stride;
    for( int32_t k = 0; k<4; ++k )
      _pad8[k] = src[0]._pad8[k];
    depth = src[0].depth;
    data_size = src[0].data_size;
  }
};

void transpose_image_in_place(BmfImage* img, int32_t planes) {
  uint16_t w = img[0].width;
  img[0].width = img[0].height;
  img[0].height = w;
  img[0].flags ^= flags_transposed;
  img[0].stride = img[0].width*planes;
}

// Row stride in bytes of a sub-byte-packed image of the given width.
uint32_t packed_stride(int32_t w, int32_t bits) {
  if( bits==1 )
    return (uint32_t)(w+7)>>3;
  if( bits==2 )
    return (uint32_t)(w+3)>>2;
  return (uint32_t)(w+1)>>1;
}

// A packed image may only be transposed in place when both orientations occupy
// the same number of bytes: the buffer is sized for one of them, the palette
// sits immediately after the pixels, and the decoder has to transpose back in
// place as well.  The encoder only offers the transposed orientation when this
// holds, so the decoder can rely on it.
bool packed_transpose_ok(const BmfImage* img) {
  if( !(img[0].flags&flags_packed) )
    return false;
  const int32_t bits = img[0].depth&depth_bits;
  if( bits!=1&&bits!=2&&bits!=4 )
    return false;
  return packed_stride(img[0].width, bits)*(uint32_t)img[0].height
      ==packed_stride(img[0].height, bits)*(uint32_t)img[0].width;
}

void transpose_image_packed(BmfImage* img) {
  if( !packed_transpose_ok(img) )
    return;
  const int32_t bits = img[0].depth&depth_bits;
  const int32_t w = img[0].width, h = img[0].height;
  const uint32_t src_stride = img[0].stride;
  const uint32_t dst_stride = packed_stride(h, bits);
  const int32_t per = 8/bits;
  const uint32_t mask = (1u<<bits)-1;
  uint8_t* copy = (uint8_t*)bmf_new(img[0].data_size);
  memcpy(copy, img[0].pixels, img[0].data_size);
  memset(img[0].pixels, 0, dst_stride*(uint32_t)w);
  for( int32_t y = 0; y<h; ++y ) {
    const uint8_t*const row = copy+(uint32_t)y*src_stride;
    uint8_t*const col = img[0].pixels+(uint32_t)(y/per);
    const int32_t shift = 8-bits*(y%per+1);
    for( int32_t x = 0; x<w; ++x ) {
      const uint32_t v = (row[x/per]>>(8-bits*(x%per+1)))&mask;
      col[(uint32_t)x*dst_stride] |= (uint8_t)(v<<shift);
    }
  }
  img[0].stride = (uint16_t)dst_stride;
  img[0].data_size = dst_stride*(uint32_t)w;
  transpose_image_in_place(img, 1);
  img[0].stride = (uint16_t)dst_stride;
  free(copy);
}

// True when a packed image's sub-byte layout follows from its depth, which is
// what the bit-level geometry below assumes.  Depth 3 packs into 4-bit fields
// and so does not qualify.
bool packed_bits_ok(const BmfImage* img) {
  const int32_t bits = img[0].depth&depth_bits;
  return !(img[0].flags&flags_packed)||bits==1||bits==2||bits==4;
}

// Mirror about the horizontal axis: whole rows swap, so the packed and
// byte-interleaved cases are the same code and row padding rides along.
void flip_image_y(BmfImage* img, int32_t planes) {
  (void)planes;
  const uint32_t stride = img[0].stride;
  uint8_t*const px = img[0].pixels;
  uint8_t* tmp = (uint8_t*)bmf_new(stride);
  for( int32_t y = 0, z = img[0].height-1; y<z; ++y, --z ) {
    memcpy(tmp, px+(uint32_t)y*stride, stride);
    memcpy(px+(uint32_t)y*stride, px+(uint32_t)z*stride, stride);
    memcpy(px+(uint32_t)z*stride, tmp, stride);
  }
  free(tmp);
  img[0]._pad8[0] ^= pad_flip_y;
}

// Mirror about the vertical axis.  Only the pixels are reversed: the slots past
// the last pixel of a packed row are row padding the BMP writer has to
// reproduce byte for byte, so they stay where they are.
void flip_image_x(BmfImage* img, int32_t planes) {
  const uint32_t stride = img[0].stride;
  const int32_t w = img[0].width, h = img[0].height;
  uint8_t*const px = img[0].pixels;
  if( img[0].flags&flags_packed ) {
    const int32_t bits = img[0].depth&depth_bits;
    const int32_t per = 8/bits;
    const uint32_t mask = (1u<<bits)-1;
    uint8_t* row_copy = (uint8_t*)bmf_new(stride);
    for( int32_t y = 0; y<h; ++y ) {
      uint8_t*const row = px+(uint32_t)y*stride;
      memcpy(row_copy, row, stride);
      for( int32_t x = 0; x<w; ++x ) {
        const int32_t src = w-1-x;
        const uint32_t v = (row_copy[src/per]>>(8-bits*(src%per+1)))&mask;
        const int32_t sh = 8-bits*(x%per+1);
        row[x/per] = (uint8_t)((row[x/per]&~(mask<<sh))|(v<<sh));
      }
    }
    free(row_copy);
  } else {
    uint8_t tmp[4];
    for( int32_t y = 0; y<h; ++y ) {
      uint8_t*const row = px+(uint32_t)y*stride;
      for( int32_t x = 0, z = w-1; x<z; ++x, --z ) {
        memcpy(tmp, row+(uint32_t)x*planes, (size_t)planes);
        memcpy(row+(uint32_t)x*planes, row+(uint32_t)z*planes, (size_t)planes);
        memcpy(row+(uint32_t)z*planes, tmp, (size_t)planes);
      }
    }
  }
  img[0]._pad8[0] ^= pad_flip_x;
}

void transpose_image(BmfImage* img, int32_t planes) {
  if( img[0].flags&flags_packed ) {
    transpose_image_packed(img);
    return;
  }
  uint8_t* copy = (uint8_t*)bmf_new(img[0].data_size);
  memcpy(copy, img[0].pixels, img[0].data_size);
  const int32_t step = planes*(img[0].height-1);
  const uint8_t* src = copy;
  for( int32_t y = 0; y<img[0].height; ++y ) {
    uint8_t* dst = img[0].pixels+planes*y;
    for( int32_t x = 0; x<img[0].width; ++x ) {
      for( int32_t k = 0; k<planes; ++k )
        *dst++ = *src++;
      dst += step;
    }
  }
  transpose_image_in_place(img, planes);
  free(copy);
}

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
  BmfImage* img = (BmfImage*)bmf_new(data_bytes+pal_bytes+19);
  if( !img )
    return nullptr;
  img[0].width = img_w;
  img[0].height = img_h;
  img[0].stride = row_bytes;
  for( int32_t k = 0; k<4; ++k )
    img[0]._pad8[k] = 0;
  img[0].depth = (uint8_t)(bpp|(has_palette ? depth_palette : 0));
  img[0].flags = sub_byte_rows ? flags_packed : 0;
  img[0].data_size = data_bytes;
  if( pal_bytes )
    memset(img[0].palette(), 0, pal_bytes);
  return img;
}

enum : uint32_t {
  bmp_magic = 'B'|'M'<<8,
  bmp_hdr_v3 = 40,
  bmp_rgb = 0,
  bmp_rle8 = 1,
  bmp_rle4 = 2,
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

int32_t bmf_pixels(const BmfImage* img) {
  return img[0].width*img[0].height;
}

enum : uint16_t {
  bmf_sig_image = 0x8A81,
  bmf_sig_other = 0x9081,
};
constexpr int32_t bmf_version_20 = 2<<8;
int32_t bmf_tag_version(uint32_t tag) {
  return (((uint8_t)(tag>>16)-'0')<<8)|((uint8_t)(tag>>24)-'0');
}

uint32_t bmf_tag(uint16_t sig, char major, char minor) {
  return sig|((uint32_t)(uint8_t)major<<16)|((uint32_t)(uint8_t)minor<<24);
}

struct BmfStream {
  FILE* fp;
  uint8_t* buf;
  size_t len;
  size_t cap;
  size_t pos;
  bool ran_off;
  void Init() {
    fp = nullptr;
    buf = nullptr;
    len = 0;
    cap = 0;
    pos = 0;
    ran_off = false;
  }
  static BmfStream over_file(FILE* f) {
    BmfStream s;
    s.Init();
    s.fp = f;
    return s;
  }

  static BmfStream over_memory(const void* data, size_t n) {
    BmfStream s;
    s.Init();
    s.buf = (uint8_t*)data;
    s.len = n;
    return s;
  }

  static BmfStream in_memory() {
    BmfStream s;
    s.Init();
    s.cap = 1;
    s.buf = (uint8_t*)bmf_new(1);
    return s;
  }

  size_t read(void* dst, size_t n) {
    if( fp ) return fread(dst, 1u, n, fp);
    const size_t got = pos>=len ? 0 : (n<len-pos ? n : len-pos);
    memcpy(dst, buf+pos, got);
    pos += got;
    if( got<n ) ran_off = true;
    return got;
  }

  size_t write(const void* src, size_t n) {
    if( fp ) return fwrite(src, 1u, n, fp);
    if( !cap ) return 0;
    if( pos+n>cap ) {
      size_t want = cap;
      while( want<pos+n ) want *= 2;
      uint8_t* wider = (uint8_t*)bmf_new(want);
      memcpy(wider, buf, len);
      free(buf);
      buf = wider;
      cap = want;
    }
    memcpy(buf+pos, src, n);
    pos += n;
    if( pos>len ) len = pos;
    return n;
  }

  bool read_exact(void* dst, size_t n) {
    return n==0||read(dst, n)==n;
  }

  bool write_exact(const void* src, size_t n) {
    return n==0||write(src, n)==n;
  }

  void write_all(const void* src, size_t n) {
    if( !write_exact(src, n) ) bmf_fatal(bmf_write_error);
  }

  bool seek(long off, int32_t whence) {
    if( fp ) return fseek(fp, off, whence)==0;
    long at = whence==SEEK_SET ? 0 : whence==SEEK_END ? (long)len : (long)pos;
    if( at+off<0||(size_t)(at+off)>len ) return false;
    pos = (size_t)(at+off);
    return true;
  }

  bool eof() const {
    return fp ? feof(fp)!=0 : ran_off;
  }

  void flush() {
    if( fp ) fflush(fp);
  }

  uint32_t bytes_left() {
    if( !fp ) return (uint32_t)(len-pos);
    const long here = ftell(fp);
    if( here<0||fseek(fp, 0, SEEK_END)!=0 ) return 0;
    const long end = ftell(fp);
    if( end<0||fseek(fp, here, SEEK_SET)!=0 ) return 0;
    return (uint32_t)(end-here);
  }

  uint8_t*take(size_t* out_len) {
    uint8_t* out = buf;
    out_len[0] = len;
    buf = nullptr;
    cap = len = pos = 0;
    return out;
  }

  void close() {
    if( fp ) {
      fclose(fp);
      fp = nullptr;
    } else if( cap ) {
      free(buf);
      buf = nullptr;
      cap = len = pos = 0;
    }
  }
};

struct BmfFile {
  BmfStream io;
  void Init() {
    io.Init();
  }

  BmfImage*fail() {
    close();
    return nullptr;
  }

  void close() {
    io.close();
  }
};

void bmf_close_file(BmfFile* arc) {
  arc[0].close();
  free(arc);
}

bool read_exact(void* dst, size_t n, FILE* fp) {
  return n==0||fread(dst, n, 1u, fp)==1;
}

BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only) {
  FILE* fp = fopen(path, read_only ? "rb" : "wb");
  if( !fp ) bmf_fatal(bmf_no_open, path);
  out[0].io = BmfStream::over_file(fp);
  return out;
}
bool read_rle_op(FILE* fp, int32_t &n, int32_t &v) {
  if( ferror(fp) ) return false;
  n = fgetc(fp);
  v = fgetc(fp);
  return n>=0&&v>=0;
}

uint8_t*write_nibbles(uint8_t* at, int32_t &hi, const uint8_t* src, uint32_t n) {
  while( 1 ) {
    const uint8_t pix = src[0];
    const uint32_t left = n-1;
    if( hi ) {
      if( !left ) {
        at[0] = src[0]&0xF0;
        hi = 0;
        return at;
      }
      *at++ = pix;
    } else {
      *at++ |= (uint8_t)src[0]>>4;
      if( !left ) {
        hi = 1;
        return at;
      }
      at[0] = 16*(pix&0xF);
    }
    ++src;
    n = left-1;
    if( !n )
      return at;
  }
}

bool read_rle8(FILE* fp, BmfImage* img, uint8_t* at, void* pal_buf) {
  int32_t y = img[0].height-1;
  int32_t n, v;
  while( read_rle_op(fp, n, v) ) {
    if( n ) {
      if( !img[0].holds(at, n) )
        return false;
      memset(at, v, n);
      at += n;
    } else if( !v ) {
      if( --y<0 )
        return true;
      at = img[0].row(y);
    } else if( v==1 ) {
      return true;
    } else if( v==2 ) {
      int32_t dx, dy;
      if( !read_rle_op(fp, dx, dy) )
        return false;
      at += dx-dy*img[0].stride;
    } else {
      if( !img[0].holds(at, v) )
        return false;
      if( !read_exact(pal_buf, (v+1)&0xFFFFFFFE, fp) )
        return false;
      memcpy(at, (uint8_t*)pal_buf, v);
      at += v;
    }
  }
  return false;
}

bool read_rle4(FILE* fp, BmfImage* img, uint8_t* row, void* pal_buf) {
  int32_t hi_nibble = 1;
  int32_t y = img[0].height-1;
  int32_t run4, byte_in;
  while( read_rle_op(fp, run4, byte_in) ) {
    const uint32_t byte = (uint32_t)byte_in;
    if( run4 ) {
      if( !img[0].holds(row, (run4+1+!hi_nibble)/2) )
        return false;
      uint8_t lo = byte&0xF;
      if( hi_nibble ) {
        int32_t left4b = run4;
        uint8_t* row3 = row;
        while( left4b>1 ) {
          *row3++ = byte;
          left4b -= 2;
        }
        row = row3;
        hi_nibble = left4b==0;
        if( left4b )
          *row3 = byte&0xF0;
      } else {
        int32_t left4 = run4;
        uint8_t* row4 = row;
        uint8_t cur = row[0];
        uint32_t hi = byte>>4;
        uint8_t lo16 = 16*lo;
        do {
          *row4++ = hi|cur;
          cur = lo16;
          left4 -= 2;
        } while( left4>0 );
        row = row4;
        hi_nibble = left4<0;
        if( !hi_nibble )
          *row4 = lo16;
      }
    } else if( !byte ) {
      if( --y<0 )
        return true;
      row = img[0].row(y);
      hi_nibble = 1;
    } else if( byte==1 ) {
      return true;
    } else if( byte==2 ) {
      int32_t dxy, dy4;
      if( !read_rle_op(fp, dxy, dy4) )
        return false;
      int32_t step = (dxy>>1)-dy4*img[0].stride;
      if( (dxy&1)==1 ) {
        if( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      row += step;
    } else {
      if( !img[0].holds(row, ((int32_t)byte+1+!hi_nibble)/2) )
        return false;
      if( !read_exact(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, fp) )
        return false;
      row = write_nibbles(row, hi_nibble, (const uint8_t*)pal_buf, byte);
    }
  }
  return false;
}

bool read_rows(FILE* fp, BmfImage* img, uint8_t* row, uint32_t stride_pad) {
  const uint32_t stride = img[0].stride;
  const uint32_t row_pad = stride_pad-stride;
  for( int32_t y = img[0].height-1; y>=0; --y ) {
    if( fread(row, 1u, stride, fp)!=stride )
      return false;
    if( row_pad )
      fseek(fp, row_pad, 1);
    row -= stride;
  }
  return true;
}

BmfImage*read_bmp(char* path) {
  uint8_t bmp_bgra[4];
  int32_t pal_n, i;
  BmpHeader hdr;
  FILE* fp = fopen(path, "rb");
  if( !fp )
    bmf_fatal(bmf_no_open, path);
  if( fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=bmp_magic||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=bmp_hdr_v3||hdr.biPlanes!=1 ) {
    fclose(fp);
    return nullptr;
  }
  {
    const int32_t bmp_w = hdr.biWidth;
    const int32_t bmp_h = hdr.biHeight;
    const int32_t bmp_bpp = hdr.biBitCount;
    if( bmp_w<=0||bmp_w>0xFFFF||bmp_h<=0||bmp_h>0xFFFF||(bmp_bpp>=8&&(uint32_t)bmp_w*(uint32_t)((bmp_bpp+7)>>3)>0xFFFFu)||(bmp_bpp!=1&&bmp_bpp!=4&&bmp_bpp!=8&&bmp_bpp!=24&&bmp_bpp!=32)||(int32_t)hdr.biClrUsed<0||(bmp_bpp<=8&&(int32_t)hdr.biClrUsed>(1<<bmp_bpp))||hdr.biCompression>bmp_rle4||(hdr.biCompression==bmp_rle8&&bmp_bpp!=8)||(hdr.biCompression==bmp_rle4&&bmp_bpp!=4) ) {
      fclose(fp);
      return nullptr;
    }
  }
  BmfImage* img = alloc_image(hdr.biWidth, hdr.biHeight, hdr.biBitCount, hdr.biBitCount<=8u, 1);
  if( !img ) {
    fclose(fp);
    return nullptr;
  }
  uint32_t stride_pad = (img[0].stride+3)&0xFFFFFFFC;
  if( hdr.biBitCount<=8u ) {
    pal_n = 1<<(hdr.biBitCount&31);
    if( (int32_t)hdr.biClrUsed )
      pal_n = (int32_t)hdr.biClrUsed;
    if( pal_n>0 ) {
      uint8_t* pal = img[0].depth&depth_palette ? img[0].pixels+img[0].data_size : nullptr;
      for( i = 0; i<pal_n; ++i ) {
        if( !read_exact(bmp_bgra, 4u, fp) ) {
          fclose(fp);
          return nullptr;
        }
        pal[3*i+2] = bmp_bgra[2];
        pal[3*i+1] = bmp_bgra[1];
        pal[3*i] = bmp_bgra[0];
      }
    }
  }
  static uint8_t pal_store[65536];
  void* pal_buf = pal_store;

  uint8_t* row = img[0].pixels+img[0].data_size-img[0].stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  bool ok;
  if( hdr.biCompression==bmp_rgb ) {
    ok = read_rows(fp, img, row, stride_pad);
  } else {
    memset(img[0].pixels, 0, img[0].data_size);
    ok = hdr.biCompression==bmp_rle8 ? read_rle8(fp, img, row, pal_buf) : hdr.biCompression==bmp_rle4 ? read_rle4(fp, img, row, pal_buf) : false;
  }
  if( !ok ) {
    fclose(fp);
    free(img);
    return nullptr;
  }
  fclose(fp);
  return img;
}

int32_t write_bmp_palette(const BmfImage* img, uint8_t* out_buf, uint8_t depth, int32_t bits) {
  if( bits>8 )
    return 0;
  const int32_t ncol = 1<<(bits&31);
  if( depth&depth_grey ) {
    uint32_t levels = 0x100u>>(bits&31);
    uint32_t k = 0;
    int32_t grey = 0;
    for(; k<(uint32_t)(ncol/2); ++k, grey += 2*levels ) {
      put_u32(&out_buf[8*k+54], bmp_grey_entry((uint8_t)grey));
      put_u32(&out_buf[8*k+58], bmp_grey_entry((uint8_t)(grey+levels)));
    }
    const uint32_t at = 2*k+1;
    if( (at-1)<(uint32_t)ncol )
      put_u32(&out_buf[4*at+50], bmp_grey_entry((uint8_t)(levels*(at-1))));
    return 4*ncol;
  } else if( depth&depth_palette ) {
    uint32_t pairs = ncol/2;
    uint32_t j = 0;
    const uint8_t* pal = img[0].palette();
    for(; j<pairs; ++j ) {
      const uint32_t slot = 2*j;
      const uint8_t* e0 = &pal[6*j], * e1 = &pal[6*j+3];
      put_u32(&out_buf[4*slot+54], bmp_palette_entry(e0[0], e0[1], e0[2]));
      put_u32(&out_buf[4*slot+58], bmp_palette_entry(e1[0], e1[1], e1[2]));
    }
    const uint32_t at2 = 2*j+1;
    if( (at2-1)<(uint32_t)ncol ) {
      const uint8_t* e = img[0].palette()+3*at2-3;
      put_u32(&out_buf[4*at2+50], bmp_palette_entry(e[0], e[1], e[2]));
    }
    return 4*ncol;
  } else {
    memset(out_buf+54, 0, 4*ncol);
    return 4*ncol;
  }
}

uint8_t*bmp_rle_encode(BmfImage* img, int32_t bits, uint8_t* buf, int32_t rows, uint32_t stride, int32_t* kind) {
  if( bits!=4&&bits!=8 )
    return nullptr;
  const int32_t nib = bits==4;
  const int32_t run_max = (0x100u>>(nib&31))-1;
  kind[0] = nib ? bmp_rle4 : bmp_rle8;
  uint8_t* out_at = buf, * q = nullptr, * end = nullptr;
  uint8_t* p = &img[0].pixels[img[0].data_size-stride];
  uint8_t byte = 0;
  int32_t lit_len = 0, run = 0;
  auto flush_literal = [&](const uint8_t* run_end) {
                         if( nib ? lit_len!=1 : lit_len>=3 ) {
                           out_at[0] = 0;
                           out_at[1] = lit_len<<(nib&31);
                           memcpy(out_at+2, run_end-lit_len, lit_len);
                           if( lit_len&1 )
                             out_at[lit_len+2] = 0;
                           out_at += lit_len+2+(lit_len&1);
                         } else if( nib ) {
                           out_at[0] = 2;
                           out_at[1] = run_end[-1];
                           out_at += 2;
                         } else {
                           if( lit_len==2 ) {
                             out_at[0] = 1;
                             out_at[1] = run_end[-2];
                             out_at += 2;
                           }
                           out_at[0] = 1;
                           out_at[1] = run_end[-1];
                           out_at += 2;
                         }
                       };
  auto run_length = [](const uint8_t* from, const uint8_t* stop) {
                      int32_t n = 1;
                      while( stop>from+n&&from[n]==from[0] )
                        ++n;
                      return n;
                    };
  auto emit_runs = [&]() {
                     while( 1 ) {
                       q = p+1;
                       if( p+1>=end )
                         return false;
                       byte = (uint8_t)p[0];
                       run = run_length(p, end);
                       if( run<=2&&(run!=2||lit_len) )
                         return false;
                       run = min32(run, run_max);
                       if( lit_len ) {
                         flush_literal(p);
                         lit_len = 0;
                         byte = p[0];
                       }
                       out_at[1] = byte;
                       p += run;
                       out_at[0] = run<<(nib&31);
                       out_at += 2;
                       if( p>=end )
                         return true;
                     }
                   };
  auto emit_row = [&]() {
                    end = &p[stride];
                    if( p>=end )
                      return;
                    lit_len = 0;
                    do {
                      while( 1 ) {
                        if( emit_runs() )
                          return;
                        ++p;
                        if( ++lit_len!=run_max )
                          break;
                        flush_literal(q);
                        if( q>=end )
                          return;
                        lit_len = 0;
                      }
                    } while( q<end );
                    if( lit_len )
                      flush_literal(q);
                  };
  if( rows>0 ) {
    for( int32_t row_i = 0; row_i<rows; ++row_i ) {
      emit_row();
      out_at[0] = 0;
      out_at[1] = 0;
      out_at += 2;
      p -= 2*stride;
    }
  }
  out_at[0] = 0;
  out_at[1] = 1;
  return out_at+2;
}

int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  uint8_t* out_at;
  int32_t bits, coded_bytes;
  FILE* fp = fopen(path, "wb");
  if( !fp )
    return 0;
  uint8_t* out_buf = (uint8_t*)bmf_new(img[0].data_size+8*img[0].height+(img[0].data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  bmp[0].biSize = bmp_hdr_v3;
  bmp[0].bfType = bmp_magic;
  int32_t rows = img[0].height;
  bmp[0].bfReserved2 = 0;
  bmp[0].bfReserved1 = 0;
  bmp[0].biWidth = img[0].width;
  bmp[0].biHeight = rows;
  uint8_t depth = img[0].depth;
  bmp[0].biPlanes = 1;
  bits = depth&depth_bits;
  bmp[0].biBitCount = bits;
  bmp[0].biClrImportant = 0;
  bmp[0].biClrUsed = 0;
  bmp[0].biYPelsPerMeter = 0;
  bmp[0].biXPelsPerMeter = 0;
  const int32_t pal_bytes = write_bmp_palette(img, out_buf, depth, bits);
  uint8_t*const buf = &out_buf[pal_bytes+54];
  uint32_t stride = img[0].stride;
  auto finish = [&](int32_t coded) {
                  bmp[0].biSizeImage = (uint32_t)coded;
                  uint32_t n_bytes = (uint32_t)(out_at-out_buf);
                  bmp[0].bfSize = n_bytes;
                  if( fwrite(out_buf, 1u, n_bytes, fp)!=bmp[0].bfSize )
                    return 0;
                  free(out_buf);
                  fclose(fp);
                  return 1;
                };
  uint32_t off_bits = (uint32_t)(buf-out_buf);
  const uint32_t data_len = img[0].data_size;
  bmp[0].bfOffBits = off_bits;
  out_at = buf;
  uint8_t* p = &img[0].pixels[data_len-stride];
  if( want_rle ) {
    int32_t rle_kind = 0;
    uint8_t*const rle_end = bmp_rle_encode(img, bits, buf, rows, stride, &rle_kind);
    if( rle_end&&data_len>(uint32_t)(rle_end-buf) ) {
      bmp[0].biCompression = rle_kind;
      out_at = rle_end;
      return finish(out_at-buf);
    }
  }
  bmp[0].biCompression = bmp_rgb;
  if( rows<=0 ) {
    coded_bytes = 0;
  } else {
    const uint32_t pad = ((stride+3)&0xFFFFFFFC)-stride;
    for( int32_t y = 0; y<img[0].height; ++y ) {
      memcpy(out_at, p, stride);
      out_at += stride;
      p -= stride;
      if( pad ) {
        memset(out_at, 0, pad);
        out_at += pad;
      }
    }
    coded_bytes = out_at-buf;
  }
  return finish(coded_bytes);
}
uint8_t write_member_head(const void* head, const CodedTail* extra, BmfStream* io) {
  uint8_t ok = io[0].write_exact(head, 0x10u);
  if( extra ) {
    const uint32_t coded_len = extra[0].len;
    ok &= io[0].write_exact(extra, coded_len+sizeof(CodedTail));
  }
  return ok;
}
// ---------------------------------------------------------------------------
