/* d5c_counter_upd.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * Ctr::upd / CtrS::upd bodies, plus crc32b.
 *
 * ORDERING: must follow the MOD/*_h.inc includes (see d5c_counter.inc). */



static u32 crc32b(const u8* d, size_t n) {
  static u32 T[256]; static bool init = false;
  if(!init) { for(u32 i = 0; i < 256; i++) { u32 c = i; for(int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; T[i] = c; } init = true; }
  u32 c = 0xFFFFFFFFu;
  for(size_t i = 0; i < n; i++) c = T[(c ^ d[i]) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}
