#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// --- Packed BMP header structures ---
#pragma pack(push, 1)
struct BMP_FILE_HEADER {
  byte  signature[2];  // "BM"
  uint  fileSize;
  word  reserved1;
  word  reserved2;
  uint  dataOffset;
};

struct BMP_DIB_CORE {   // BITMAPCOREHEADER (OS/2 1.x) - 12 bytes
  uint  headerSize;
  word  width;
  word  height;
  word  planes;
  word  bitsPerPixel;
};

struct BMP_DIB_INFO {   // BITMAPINFOHEADER (Windows) - 40 bytes
  uint  headerSize;
  uint  width;
  int   height;         // can be negative for top-down
  word  planes;
  word  bitsPerPixel;
  uint  compression;
  uint  imageSize;
  uint  xPixelsPerMeter;
  uint  yPixelsPerMeter;
  uint  colorsUsed;
  uint  colorsImportant;
};

struct BMP_PALETTE_ENTRY {
  byte blue;
  byte green;
  byte red;
  byte reserved;
};

struct BMP_HEADER {  // Combined header for 1bpp BMP (62 bytes total)
  BMP_FILE_HEADER file;
  BMP_DIB_INFO    dib;
  BMP_PALETTE_ENTRY palette[2];

  void create( uint w, uint h, uint imgSize ) {
    memset( this, 0, sizeof(*this) );
    file.signature[0] = 'B';
    file.signature[1] = 'M';
    file.fileSize = sizeof(BMP_HEADER) + imgSize;
    file.dataOffset = sizeof(BMP_HEADER);
    dib.headerSize = sizeof(BMP_DIB_INFO);
    dib.width = w;
    dib.height = h;
    dib.planes = 1;
    dib.bitsPerPixel = 1;
    dib.imageSize = imgSize;
    dib.colorsUsed = 2;
    dib.colorsImportant = 2;
    palette[0].blue = palette[0].green = palette[0].red = 0;     // black
    palette[1].blue = palette[1].green = palette[1].red = 255;   // white
  }

  bool parse( const byte* data, uint dataLen ) {
    const uint fileHdrSize = sizeof(BMP_FILE_HEADER);
    const uint coreHdrSize = sizeof(BMP_DIB_CORE);
    const uint infoHdrSize = sizeof(BMP_DIB_INFO);

    if( dataLen < fileHdrSize + coreHdrSize ) return false;

    const BMP_FILE_HEADER* srcFile = (const BMP_FILE_HEADER*)data;
    if( srcFile->signature[0]!='B' || srcFile->signature[1]!='M' ) return false;

    file.dataOffset = srcFile->dataOffset;

    uint dibHeaderSize = *(const uint*)(data + fileHdrSize);

    if( dibHeaderSize == coreHdrSize ) {
      const BMP_DIB_CORE* srcDib = (const BMP_DIB_CORE*)(data + fileHdrSize);
      dib.width = srcDib->width;
      dib.height = srcDib->height;
      dib.bitsPerPixel = srcDib->bitsPerPixel;
    } else if( dibHeaderSize >= infoHdrSize ) {
      if( dataLen < fileHdrSize + infoHdrSize ) return false;
      const BMP_DIB_INFO* srcDib = (const BMP_DIB_INFO*)(data + fileHdrSize);
      dib.width = srcDib->width;
      dib.height = srcDib->height;
      dib.bitsPerPixel = srcDib->bitsPerPixel;
    } else {
      return false;
    }

    if( dib.bitsPerPixel!=1 ) return false;
    return true;
  }
};
#pragma pack(pop)

uint flen( FILE* f ){
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

uint BytesLoaded;

void* fload( FILE* temp ){
  unsigned int len = flen(temp);
  BytesLoaded = 0;
  char* buf = new char[len]; if( buf==0 ) return 0;
  BytesLoaded = fread( buf, 1,len, temp );
  return buf;
}

void put16( uint c, FILE* f ) {
  byte buf[2];
  buf[0] = c&0xFF;
  buf[1] = (c>>8)&0xFF;
  fwrite( buf, 1, 2, f );
}

uint get16( FILE* f ) {
  byte buf[2] = {0,0};
  fread( buf, 1, 2, f );
  return buf[0]|(buf[1]<<8);
}

// Probability scale constants
static const uint SCALElog = 15;
static const uint SCALE = 1 << SCALElog;
static const uint hSCALE = SCALE >> 1;
static const uint mSCALE = SCALE - 1;
static const uint eSCALE = 16*SCALE;

#if 0
// Tunable parameters for adaptive counters
static const int WR_EMPTY  = SCALE / 32;   // Learning rate for empty flag
static const int MW_EMPTY  = SCALE / 8;    // Min weight for empty
static const int WR_FULL   = SCALE / 32;   // Learning rate for full flag
static const int MW_FULL   = SCALE / 8;    // Min weight for full
static const int WR_SINGLE = SCALE / 32;   // Learning rate for single flag
static const int MW_SINGLE = SCALE / 8;    // Min weight for single
static const int WR_BITS   = SCALE / 24;   // Learning rate for bit encoding
static const int MW_BITS   = SCALE / 16;   // Min weight for bits
#endif

#include "sh_mapping.inc"
#include "MOD/sh_model-C0_h.inc"

// Adaptive probability counter (from freq_proc)
struct Counter {
  word P;

  void init( uint p0 = hSCALE ) {
    P = p0;
  }

  // EMA update with bounded probability
  void Update( int c, int wr, int mw ) {
    int dp = P + mw - SCALE + ((SCALE - 2*mw) & (-c));
    dp = (dp * wr) >> SCALElog;
    int q = P - dp + c;
    P = q & mSCALE;
  }
};

// Bit-by-bit number encoder with context (from freq_proc's numbits_proc)
template<int rlen>
struct NumberBits {
  Counter P[1 << rlen];

  void init() {
    for( uint i = 0; i < (1u << rlen); i++ ) {
      P[i].init(hSCALE);
    }
  }
};

// Range coder with binary processing
struct Rangecoder {
  int f_DEC;   // 0=encode, 1=decode
  FILE* arch;
  int get() { int c = getc(arch); return byte(c); }
  void put(int c) { putc(c, arch); }

  static const uint NUM   = 4;
  static const uint sTOP  = 0x01000000U;
  static const uint Thres = 0xFF000000U;

  union {
    struct {
      uint low;
      uint Carry;
    };
    qword lowc;
  };
  uint code;
  uint FFNum;
  uint Cache;
  uint range;

  void rc_Renorm( void ) {
    while( range < sTOP ) ShiftStuff();
  }

#if 1
  uint rc_GetFreq( uint totFreq ) {
    return code / (range / totFreq);
  }

  // Process with cumulative frequency
  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint scale = range / totFreq;
    uint tmp = scale * cumFreq;
    uint rnew = scale * (cumFreq + freq);
    if( cumFreq + freq == totFreq ) rnew = range;
    if( f_DEC ) code -= tmp; else lowc += tmp;
    range = rnew - tmp;
    rc_Renorm();
  }
#else

  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint tmp  = range-mulRdiv( totFreq-cumFreq, totFreq );
    uint rnew = range-mulRdiv( totFreq-(cumFreq+freq), totFreq );
    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range = rnew - tmp;
    rc_Renorm();
  }
#endif

  // Binary process with probability p (out of SCALE)
  uint rc_BProcess( uint p, uint bit = 0 ) {
    uint bound = (qword(range)*(p<<(32-SCALElog)))>>32;
    //uint bound = (qword(range) * p) >> SCALElog;
    if( bound == 0 ) bound = 1;
    if( bound >= range ) bound = range - 1;

    if( f_DEC ) {
      bit = (code >= bound);
    }

    if( bit ) {
      if( f_DEC ) code -= bound;
      else lowc += bound;
      range -= bound;
    } else {
      range = bound;
    }

    rc_Renorm();
    return bit;
  }

  void ShiftStuff( void ) {
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    range = (range<<8);
    code  = (code<<8);
    uint c = get();
    FFNum += (c==-1);
    code += byte(c);
  }

  void ShiftLow( void ) {
    range = (range<<8);
    if( (low<Thres)||Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for(; FFNum!=0; FFNum-- ) put( Carry-1 );
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

  void rcInit( void ) {
    range = 0xFFFFFFFF;
    low   = 0;
    FFNum = 0;
    Carry = 0;
    Cache = -1;
  }

  void rc_Init( void ) {
    rcInit();
    if( f_DEC==1 ) {
      for( int _=0; _<NUM; _++ ) ShiftCode();
      range = 0xFFFFFFFF;
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;
      qword llow=low;
      qword high=llow+range;
      if( (llow|0xFF)<high ) low|=      0xFF, n--;
      if( (llow|0xFFFF)<high ) low|=    0xFFFF, n--;
      if( (llow|0xFFFFFF)<high ) low|=  0xFFFFFF, n--;
      if( (llow|0xFFFFFFFF)<high ) low|=0xFFFFFFFF, n--;
      if( (Cache!=-1)&&((n!=0)||(Cache+Carry!=0xFF)) ) put( Cache+Carry );
      if( (n==0)&&(Carry==0) ) FFNum=0;
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

  void init( FILE* fp, int isDec ) {
    arch = fp; f_DEC = isDec; rc_Init();
  }

  void flush( void ) {
    if( f_DEC==0 ) rc_Quit();
  }

  // Encode/decode a value in range [0, maxVal] with uniform distribution
  void encodeUniform( uint val, uint maxVal ) {
    if( maxVal == 0 ) return;
    uint total = maxVal + 1;
    rc_Process( val, 1, total );
  }

  uint decodeUniform( uint maxVal ) {
    if( maxVal == 0 ) return 0;
    uint total = maxVal + 1;
    uint freq = rc_GetFreq( total );
    uint val = (freq < total) ? freq : maxVal;
    rc_Process( val, 1, total );
    return val;
  }
};

// Integer log2
inline uint log2i( uint x ) {
  uint r = 0;
  while( x > 1 ) { x >>= 1; r++; }
  return r;
}

// Hierarchical image coder with adaptive modeling
struct HierarchicalCoder {
  byte* pixels;
  uint width, height;
  Rangecoder rc;
  int f_DEC;

  // Adaptive counters for special cases (by depth, capped at 15)
  static const int MAX_DEPTH = 16;
  Counter probEmpty[MAX_DEPTH];   // P(count == 0)
  Counter probFull[MAX_DEPTH];    // P(count == size)
  Counter probSingle[MAX_DEPTH];  // P(count == 1)

  // Bit-by-bit count encoder with 6-bit context
  // Context: 3 bits from bit position, 3 bits from depth
  NumberBits<6> countBits;

  void initModels() {
    for( int i = 0; i < MAX_DEPTH; i++ ) {
      probEmpty[i].init(hSCALE);
      probFull[i].init(hSCALE);
      probSingle[i].init(hSCALE);
    }
    countBits.init();
  }

  // Count set pixels in a rectangular region
  uint countPixels( uint x, uint y, uint w, uint h ) {
    uint count = 0;
    for( uint j = y; j < y + h; j++ ) {
      for( uint i = x; i < x + w; i++ ) {
        count += pixels[j * width + i];
      }
    }
    return count;
  }

  void setPixel( uint x, uint y, byte val ) {
    pixels[y * width + x] = val;
  }

  byte getPixel( uint x, uint y ) {
    return pixels[y * width + x];
  }

  // Encode position of single pixel in region (still uniform - could be improved)
  void encodePosition( uint x, uint y, uint w, uint h, uint px, uint py ) {
    uint relX = px - x;
    uint relY = py - y;
    uint idx = relY * w + relX;
    rc.encodeUniform( idx, w * h - 1 );
  }

  void decodePosition( uint x, uint y, uint w, uint h, uint& px, uint& py ) {
    uint idx = rc.decodeUniform( w * h - 1 );
    px = x + (idx % w);
    py = y + (idx / w);
  }

  void findSinglePixel( uint x, uint y, uint w, uint h, uint& px, uint& py ) {
    for( uint j = y; j < y + h; j++ ) {
      for( uint i = x; i < x + w; i++ ) {
        if( pixels[j * width + i] ) {
          px = i;
          py = j;
          return;
        }
      }
    }
  }

  // Bit-by-bit count encoding with context (like numbits_proc but for range [lo, hi])
  // Encodes value relative to lo, returns absolute value
  uint encodeCountBits( uint n, uint lo, uint hi, int depth ) {
    if( hi <= lo ) return lo;

    uint k;

#if 0
    k = (n==lo);
    k = rc.rc_BProcess( probEmpty[depth].P, k );
    probEmpty[depth].Update( k, C0_EMPTY_wr, C0_EMPTY_mw );
    if( k==1 ) return lo;
    ++lo;
    if( hi <= lo ) return lo;
#endif

#if 0
    k = (n==lo);
    k = rc.rc_BProcess( probSingle[depth].P, k );
    probSingle[depth].Update( k, C0_SINGLE_wr, C0_SINGLE_mw );
    if( k==1 ) return lo;
    ++lo;
    if( hi <= lo ) return lo;
#endif

#if 0
    k = (n==hi);
    k = rc.rc_BProcess( probFull[depth].P, k );
    probFull[depth].Update( k, C0_FULL_wr, C0_FULL_mw );
    if( k==1 ) return hi;
    --hi;
    if( hi <= lo ) return hi;
#endif

    uint range = hi - lo;  // Size of valid range
    uint nrel = n - lo;    // Value relative to lo (for encoder; decoder ignores this)

    // Number of bits needed to represent values in [0, range]
    uint clen = log2i(range) + 1;
    uint z = 0;  // Accumulates relative value
    int d = (depth < 7) ? depth : 7;  // 3 bits for depth context

    for( int i = clen - 1; i >= 0; i-- ) {
      uint j = ((1u << clen) + z) >> (i + 1);  // Context from prefix bits
      uint ctx = ((j & 7) << 3) | d;  // 6-bit context: 3 bits prefix, 3 bits depth

      k = (nrel >> i) & 1;  // Current bit to encode (from relative value)

      // Constraint-aware skipping for relative encoding in [0, range]
      // z + (1<<i) - 1 is max value achievable with bit=0
      // z + (1<<i) is min value achievable with bit=1
      if( z + (1u << i) > range ) {
        // Setting bit=1 would exceed range, must set bit=0
        k = 0;
      } else {
        // Normal case: encode/decode with adaptive model
        k = rc.rc_BProcess( countBits.P[ctx].P, k );
        //countBits.P[ctx].Update( k, C0_BITS_wr, C0_BITS_mw );
        countBits.P[ctx].Update( k, d<7?C0_BITS_wr:C0_BITS7_wr, d<7?C0_BITS_mw:C0_BITS7_mw );
      }

      z |= (k << i);
    }

    return lo + z;  // Convert back to absolute value
  }

  // Process a region recursively with adaptive count encoding
  void processRegion( uint x, uint y, uint w, uint h, uint count, int depth ) {
    uint size = w * h;

    // Base case: no set pixels (count known from parent, no encoding needed)
    if( count == 0 ) {
      return;
    }

    // Base case: region is 1x1
    if( w == 1 && h == 1 ) {
      if( f_DEC ) {
        setPixel( x, y, 1 );
      }
      return;
    }

    // Base case: count == 1, encode position directly
    if( count == 1 ) {
      uint px, py;
      if( f_DEC ) {
        decodePosition( x, y, w, h, px, py );
        setPixel( px, py, 1 );
      } else {
        findSinglePixel( x, y, w, h, px, py );
        encodePosition( x, y, w, h, px, py );
      }
      return;
    }

    // Base case: all pixels are set (count known from parent, no encoding needed)
    if( count == size ) {
      if( f_DEC ) {
        for( uint j = y; j < y + h; j++ ) {
          for( uint i = x; i < x + w; i++ ) {
            setPixel( i, j, 1 );
          }
        }
      }
      return;
    }

    // Recursive case: split region
    uint w1, h1, w2, h2;
    uint x1, y1, x2, y2;

    if( (depth & 1) == 0 ) {
      // Split horizontally (along X axis)
      if( w == 1 ) {
        h1 = h / 2;
        h2 = h - h1;
        w1 = w2 = w;
        x1 = x2 = x;
        y1 = y;
        y2 = y + h1;
      } else {
        w1 = w / 2;
        w2 = w - w1;
        h1 = h2 = h;
        x1 = x;
        x2 = x + w1;
        y1 = y2 = y;
      }
    } else {
      // Split vertically (along Y axis)
      if( h == 1 ) {
        w1 = w / 2;
        w2 = w - w1;
        h1 = h2 = h;
        x1 = x;
        x2 = x + w1;
        y1 = y2 = y;
      } else {
        h1 = h / 2;
        h2 = h - h1;
        w1 = w2 = w;
        x1 = x2 = x;
        y1 = y;
        y2 = y + h1;
      }
    }

    uint size1 = w1 * h1;
    uint size2 = w2 * h2;

    // Calculate valid range for count1
    uint minCount1 = (count > size2) ? (count - size2) : 0;
    uint maxCount1 = (count < size1) ? count : size1;

    uint count1, count2;

    if( minCount1 == maxCount1 ) {
      // Only one valid value
      count1 = minCount1;
    } else {
      // Encode count1 using bit-by-bit adaptive model
      uint val = f_DEC ? minCount1 : countPixels( x1, y1, w1, h1 );
      count1 = encodeCountBits( val, minCount1, maxCount1, depth );
    }
    count2 = count - count1;

    // Recurse on both halves
    processRegion( x1, y1, w1, h1, count1, depth + 1 );
    processRegion( x2, y2, w2, h2, count2, depth + 1 );
  }

  void process( byte* Buf, FILE* fp, uint w, uint h, int isDec ) {
    width = w;
    height = h;
    f_DEC = isDec;

    pixels = new byte[width * height];
    memset( pixels, 0, width * height );

    initModels();
    rc.init( fp, isDec );

    uint totalPixels = width * height;
    uint totalCount;

    uint rowBytes = (width + 7) / 8;
    uint rowPadded = (rowBytes + 3) & ~3;

    if( isDec ) {
      // Decode total count (still uniform for now)
      totalCount = rc.decodeUniform( totalPixels );
      fprintf(stderr, "Decoded total: %u out of %u\n", totalCount, totalPixels);
    } else {
      // Convert packed BMP to pixel array
      for( uint y = 0; y < height; y++ ) {
        for( uint x = 0; x < width; x++ ) {
          uint byteIdx = y * rowPadded + (x / 8);
          uint bitIdx = 7 - (x & 7);
          byte val = (Buf[byteIdx] >> bitIdx) & 1;
          pixels[y * width + x] = val;
        }
      }

      totalCount = countPixels( 0, 0, width, height );
      fprintf(stderr, "Total pixels: %u, Set pixels: %u\n", totalPixels, totalCount);
      rc.encodeUniform( totalCount, totalPixels );
    }

    // Process entire image
    processRegion( 0, 0, width, height, totalCount, 0 );

    if( isDec ) {
      memset( Buf, 0, rowPadded * height );

      uint verifyCount = 0;
      for( uint y = 0; y < height; y++ ) {
        for( uint x = 0; x < width; x++ ) {
          if( pixels[y * width + x] ) {
            verifyCount++;
            uint byteIdx = y * rowPadded + (x / 8);
            uint bitIdx = 7 - (x & 7);
            Buf[byteIdx] |= (1 << bitIdx);
          }
        }
      }
      fprintf(stderr, "Verify: decoded %u pixels\n", verifyCount);
    }

    rc.flush();
    delete[] pixels;
  }
};

void usage() {
  printf(
  "Usage: bcdr5 c|d input output\n"
  "  c - compress BMP to BCDR5\n"
  "  d - decompress BCDR5 to BMP\n"
  "\n"
  "Hierarchical decomposition compressor with adaptive modeling.\n"
  "Uses adaptive probability flags and bit-by-bit count encoding.\n" );
  exit(1);
}

HierarchicalCoder hc;

int main( int argc, char** argv ) {

  if( argc!=4 ) usage();

  FILE* f = fopen( argv[2], "rb" ); if( f==0 ) return 1;
  FILE* g = fopen( argv[3], "wb" ); if( g==0 ) return 2;

  char f_DEC = (argv[1][0]=='d');
  byte* pic = 0;

  if( f_DEC==0 ) {
    pic = (byte*)fload(f); if( pic==0 ) return 3;

    BMP_HEADER bmp;
    if( !bmp.parse( pic, BytesLoaded ) ) {
      printf("Error: Invalid or unsupported BMP file\n");
      return 1;
    }

    put16( bmp.dib.width, g );
    put16( bmp.dib.height, g );

    hc.process( pic + bmp.file.dataOffset, g, bmp.dib.width, bmp.dib.height, 0 );

  } else {
    uint width = get16(f);
    uint height = get16(f);

    uint rowBytes = (width + 7) / 8;
    uint rowPadded = (rowBytes + 3) & ~3;
    uint imageSize = rowPadded * height;

    const uint headerSize = sizeof(BMP_HEADER);
    pic = new byte[headerSize + imageSize];
    ((BMP_HEADER*)pic)->create( width, height, imageSize );
    memset( pic + headerSize, 0, imageSize );

    hc.process( pic + headerSize, f, width, height, 1 );

    fwrite( pic, 1, headerSize + imageSize, g );
  }

  delete[] pic;
  fclose(g);
  fclose(f);

  return 0;
}
