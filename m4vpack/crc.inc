// crc.inc -- the CRC-32 (zlib's) of the stream and of the records of parts.

constexpr U32 CRC_POLY = 0xEDB88320u;  // (reflected)
// its table, made as the program starts (as cm.inc's are)
struct CrcTab {
  U32 t[BYTE_VALUES];
};
static CrcTab crc_make_tab(void) {
  CrcTab c;
  U32 i, k, v;
  for (i = 0; i < BYTE_VALUES; i++) {
    for (v = i, k = 0; k < BYTE_BITS; k++) v = (v >> 1) ^ (CRC_POLY & (0u - (v & 1)));
    c.t[i] = v;
  }
  return c;
}
static const CrcTab crc_tab_ = crc_make_tab();
static const U32 (&crc_tab)[BYTE_VALUES] = crc_tab_.t;
static U32 crc32_upd(U32 crc, const U8* p, size_t n) {
  crc = ~crc;
  while (n--) crc = crc_tab[(crc ^ *p++) & BYTE_MASK] ^ (crc >> BYTE_BITS);
  return ~crc;
}
