// inc/crc32.inc -- CRC32 (slicing by 8)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- CRC32 (slicing by 8)

static uint crc_t[8][256];

static void CrcInit() {
  for( uint i=0; i<256; i++ ) {
    uint c = i;
    for( int k=0; k<8; k++ ) c = (c>>1) ^ (0xEDB88320u & (0u-(c&1)));
    crc_t[0][i] = c;
  }
  for( uint i=0; i<256; i++ ) for( int t=1; t<8; t++ ) crc_t[t][i] = (crc_t[t-1][i]>>8) ^ crc_t[0][crc_t[t-1][i]&255];
}

static uint Crc( uint crc, const byte* p, size_t n ) {
  crc = ~crc;
  for(; n && (size_t(p)&7); n-- ) crc = (crc>>8) ^ crc_t[0][(crc^*p++)&255];
  for(; n>=8; n-=8, p+=8 ) {
    uint a = crc ^ (p[0] | (p[1]<<8) | (p[2]<<16) | (uint(p[3])<<24));
    uint b = p[4] | (p[5]<<8) | (p[6]<<16) | (uint(p[7])<<24);
    crc = crc_t[7][a&255] ^ crc_t[6][(a>>8)&255] ^ crc_t[5][(a>>16)&255] ^ crc_t[4][a>>24]
        ^ crc_t[3][b&255] ^ crc_t[2][(b>>8)&255] ^ crc_t[1][(b>>16)&255] ^ crc_t[0][b>>24];
  }
  for(; n; n-- ) crc = (crc>>8) ^ crc_t[0][(crc^*p++)&255];
  return ~crc;
}

