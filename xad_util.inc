/* xad_util.inc -- part of xadpcm.cpp; not standalone.
 *
 * crc32b, byte order, varint zigzag, bit packing, and the one global flag.
 * Nothing here knows anything about the model. */

/* No load_file / save_file any more.  Both directions read and write through
   coroutine pins (Xad::slurp, Xad::putn and the RC's own pin), and the only
   stdio left in the program is in XadDrive, which is the one place allowed to
   know what a FILE* is. */

static u32 crc32b(const u8* d, size_t n) {
  static u32 T[256];
  static bool init = false;
  if( !init ) {
    for( u32 i = 0; i<256; i++ ) {
      u32 c = i;
      for( int k = 0; k<8; k++ )
        c = c&1 ? 0xEDB88320u^(c>>1) : c>>1;
      T[i] = c;
    }
    init = true;
  }
  u32 c = 0xFFFFFFFFu;
  for( size_t i = 0; i<n; i++ )
    c = T[(c^d[i])&0xFF]^(c>>8);
  return c^0xFFFFFFFFu;
}

static INLINE u32 rd16(const u8* p) {
  return u32(p[0])|(u32(p[1])<<8);
}

static INLINE u32 rd32(const u8* p) {
  return rd16(p)|(rd16(p+2)<<16);
}

static INLINE void wr16(u8* p, u32 v) {
  p[0] = u8(v);
  p[1] = u8(v>>8);
}

static INLINE void wr32(u8* p, u32 v) {
  wr16(p, v&0xFFFF);
  wr16(p+2, v>>16);
}

static INLINE u64 zig(i64 v) {
  return (u64(v)<<1)^u64(v>>63);
}

static INLINE i64 unzig(u64 u) {
  return i64(u>>1)^-i64(u&1);
}

/* Strip any directory part from a stored name.  Archives name their members,
   and a member called ../../etc/x must not be able to say where it lands. */
static string safe_name(const string &s) {
  size_t k = s.find_last_of("/\\");
  string b = k==string::npos ? s : s.substr(k+1);
  if( b.empty()||b==string(".")||b==string("..") )
    b = "member";
  return b;
}


static int verbose = 0;
static INLINE int16_t i16_of(const u8* p) {
  return int16_t(u16(p[0])|u16(u16(p[1])<<8));
}

static INLINE u32 get_bits(const u8* b, size_t bitpos, int nbits) {
  u32 v = u32(b[bitpos>>3])>>(bitpos&7);
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    v |= u32(b[i++])<<have;
    have += 8;
  }
  return v&((1u<<nbits)-1);
}

static INLINE void put_bits(u8* b, size_t bitpos, int nbits, u32 v) {
  b[bitpos>>3] |= u8(v<<(bitpos&7));
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    b[i++] |= u8(v>>have);
    have += 8;
  }
}
