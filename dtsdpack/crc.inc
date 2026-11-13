// crc.inc -- the CRC-32 (zlib's) of the stream and of the records of parts.

constexpr U32 CRC_POLY = 0xEDB88320u;  // (reflected)
// its tables, made as the program starts (as cm.inc's are): a byte's (t[0]), and that of a byte
// followed by k 0 bytes (t[k]), for 8 bytes at a time
struct CrcTab {
  U32 t[8][BYTE_VALUES];
};
static CrcTab crc_make_tab(void) {
  CrcTab c;
  U32 i, k, v;
  for (i = 0; i < BYTE_VALUES; i++) {
    for (v = i, k = 0; k < BYTE_BITS; k++) v = (v >> 1) ^ (CRC_POLY & (0u - (v & 1)));
    c.t[0][i] = v;
  }
  for (i = 0; i < BYTE_VALUES; i++)
    for (k = 1; k < 8; k++) c.t[k][i] = (c.t[k - 1][i] >> BYTE_BITS) ^ c.t[0][c.t[k - 1][i] & BYTE_MASK];
  return c;
}
static const CrcTab crc32_tab_ = crc_make_tab();
static U32 crc32_upd(U32 crc, pcU8 p, size_t n) {
  const U32(*const t)[BYTE_VALUES] = crc32_tab_.t;
  crc = ~crc;
  for (; n >= 8; n -= 8, p += 8) {  // (8 bytes at a time: the same, by the tables of the bytes after)
    const U32 a = crc ^ ((U32)p[0] | (U32)p[1] << 8 | (U32)p[2] << 16 | (U32)p[3] << 24);
    crc = t[7][a & BYTE_MASK] ^ t[6][(a >> 8) & BYTE_MASK] ^ t[5][(a >> 16) & BYTE_MASK] ^ t[4][a >> 24] ^ t[3][p[4]] ^
          t[2][p[5]] ^ t[1][p[6]] ^ t[0][p[7]];
  }
  while (n--) crc = t[0][(crc ^ *p++) & BYTE_MASK] ^ (crc >> BYTE_BITS);
  return ~crc;
}
