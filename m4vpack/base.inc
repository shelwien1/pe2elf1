// base.inc -- what every part of m4vpack (and IDX/famtune.cpp) needs first: the integer types, the
// units, the exit statuses, the sizes of the buffers, and the start codes the stream is cut at.

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef short S16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef long long S64;

constexpr int NA = -1000;  // a field not present

// units
constexpr int BYTE_SHIFT = 3, BYTE_BITS = 1 << BYTE_SHIFT;
// the bits from bit pos to the next byte boundary
static inline int to_byte(int pos) { return -pos & (BYTE_BITS - 1); }
constexpr int BYTE_VALUES = 1 << BYTE_BITS;
constexpr U32 BYTE_MASK = BYTE_VALUES - 1;
constexpr int U32_BYTES = sizeof(U32), U64_BYTES = sizeof(U64);
constexpr int U32_BITS = U32_BYTES * BYTE_BITS, U64_BITS = U64_BYTES * BYTE_BITS;
constexpr U32 KIB = 1u << 10, MIB = 1u << 20;
constexpr double PERCENT = 100.0;  // (a share in percent)

// the exit statuses, and those of a run's errors (CoIo::fail()); E_INTERNAL: or out of memory
enum { E_USAGE = 1, E_READ = 2, E_WRITE = 3, E_DATA = 4, E_INTERNAL = 5 };

// The input is cut into chunks at start codes (io.inc's Reader): the longest handled at once
// (longer ones come in pieces, kept as bytes), and a read. A run takes its input and gives its
// output in buffers of IO_BUF.
#ifndef M4V_SMALL_BUFFERS
constexpr U32 IBUF_SIZE = 64 * MIB;
constexpr U32 IREAD = 1 * MIB;
#else  // (tests: chunks in pieces, reads of a few bytes)
constexpr U32 IBUF_SIZE = 64 * KIB;
constexpr U32 IREAD = 7;
#endif
constexpr U32 IO_BUF = 64 * KIB;
constexpr int GB_PAD = 64;  // the zero bytes after a buffer the bit reader reads (it reads ahead)

// The picture's geometry: 8x8 blocks, 16x16 macroblocks of four luma blocks (0 1 / 2 3: bit 0 the
// column, bit 1 the row) and two chroma ones, Cb and Cr
constexpr int BLK_SHIFT = 3, BLK_W = 1 << BLK_SHIFT;  // a block's side
constexpr int BLK_COEFS = BLK_W * BLK_W, BLK_LAST = BLK_COEFS - 1;
constexpr int MB_SHIFT = BLK_SHIFT + 1, MB_W = 1 << MB_SHIFT;  // a macroblock's (luma) side
constexpr int MB_LUMA = 4, MB_BLOCKS = 6;
enum { BLK_CB = MB_LUMA, BLK_CR };
// f_code: a motion vector's range, and the bits of its residual (f_code - 1)
constexpr int FCODE_MIN = 1, FCODE_MAX = 7;

// start codes: 00 00 01 (SC_PREFIX bytes; as a number, SC_START), then the code
constexpr int SC_PREFIX = 3;
constexpr int SC_LEN = SC_PREFIX + 1;
constexpr U32 SC_START = 1;
enum : int {
  SC_VO = 0x00,  // video_object_start_code: 00-1F
  SC_VOL = 0x20, // video_object_layer_start_code: 20-2F
  SC_VOL_END = 0x30,
  SC_VOS = 0xB0,  // visual_object_sequence_start_code
  SC_VOS_END = 0xB1,
  SC_USER_DATA = 0xB2,
  SC_GOV = 0xB3,
  SC_VISOBJ = 0xB5,  // visual_object_start_code
  SC_VOP = 0xB6,
  SC_RESERVED = 0xB7  // reserved (B7-B9), then the system start codes
};
static inline int sc_is_vol(int c) { return c >= SC_VOL && c < SC_VOL_END; }
