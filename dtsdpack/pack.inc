// pack.inc -- the ways a DTS stream packs its bits into bytes. The core's own is 16-bit words
// big-endian (what the model codes, PK_BE16); a stream may have them little-endian too (PK_LE16:
// DTS in a WAV at 48 kHz, its bytes swapped), or 14 bits of it in each 16-bit word, the word's top
// 2 bits copies of its bit 13 so that as PCM the stream is no louder than 14 bits (PK_LE14: a DTS
// CD's, DTS in a WAV at 44.1 kHz; PK_BE14 its words big-endian). A frame in one of the others is
// unpacked into the bytes the model codes and packed back as it was; where packing them again
// doesn't make its bytes (a word's top bits not so copied, or the bits after the frame's in its
// last word not 0), it is kept as bytes, its blocks counted as a frame's (reader.inc's *ex).

enum { PK_BE16, PK_LE16, PK_BE14, PK_LE14, PK_N };

// the packing whose core sync word starts at p (av bytes there), or -1
static INLINE int pack_sync(pcU8 p, size_t av) {
  if (av < 4) return -1;
  const U32 w = be32(p);
  return w == 0x7FFE8001u ? PK_BE16 : w == 0xFE7F0180u ? PK_LE16 : w == 0x1FFFE800u ? PK_BE14 : w == 0xFF1F00E8u ? PK_LE14 : -1;
}
// the bytes an n-byte frame takes packed by pk
static INLINE size_t pack_size(int pk, size_t n) {
  return pk == PK_BE16 ? n : pk == PK_LE16 ? (n + 1) & ~(size_t)1 : 2 * ((8 * n + 13) / 14);
}
// the frame bytes the first av bytes at p (packed by pk) hold, at most
static INLINE size_t pack_holds(int pk, size_t av) {
  return pk == PK_BE16 ? av : pk == PK_LE16 ? av & ~(size_t)1 : av / 2 * 14 / 8;
}
// the av bytes at p (packed by pk) unpacked into out, its first n bytes (n <= pack_holds(pk, av))
static void pack_unpack(int pk, pcU8 p, U8* out, size_t n) {
  if (pk == PK_BE16) {
    memcpy(out, p, n);
    return;
  }
  if (pk == PK_LE16) {
    for (size_t i = 0; i < n; i++) out[i] = p[i ^ 1];
    return;
  }
  const bool le = pk == PK_LE14;
  U32 acc = 0;
  int bits = 0;
  size_t m = 0;
  for (size_t i = 0; m < n; i += 2) {
    const U32 w = le ? (U32)p[i] | (U32)p[i + 1] << 8 : (U32)p[i] << 8 | p[i + 1];
    acc = (acc << 14 | (w & 0x3FFF)) & 0x3FFFFF;  // (fewer than 8 bits were left: 22 hold them)
    bits += 14;
    while (bits >= 8 && m < n) out[m++] = (U8)(acc >> (bits - 8)), bits -= 8;
  }
}
// the n bytes at in packed by pk into out (pack_size(pk, n) bytes): where n is odd, a 16-bit word's
// other byte 0; the bits of the last 14 past the frame's, 0, and each word's top 2 bits its bit 13's
static void pack_pack(int pk, pcU8 in, size_t n, U8* out) {
  if (pk == PK_BE16) {
    memcpy(out, in, n);
    return;
  }
  if (pk == PK_LE16) {
    for (size_t i = 0; i < n; i += 2) out[i] = i + 1 < n ? in[i + 1] : 0, out[i + 1] = in[i];
    return;
  }
  const bool le = pk == PK_LE14;
  const size_t nw = (8 * n + 13) / 14;
  U32 acc = 0;
  int bits = 0;
  size_t j = 0;
  for (size_t k = 0; k < nw; k++) {
    while (bits < 14) acc = (acc << 8 | (j < n ? in[j] : 0)) & 0x3FFFFF, j++, bits += 8;
    U32 v = (acc >> (bits - 14)) & 0x3FFF;
    bits -= 14;
    if (v & 0x2000) v |= 0xC000;
    out[2 * k] = (U8)(le ? v : v >> 8), out[2 * k + 1] = (U8)(le ? v >> 8 : v);
  }
}
// whether the n bytes at in packed by pk make the bytes at p
static bool pack_same(int pk, pcU8 in, size_t n, pcU8 p) {
  U8 w[2 * 64];  // (a run of words at a time)
  const size_t total = pack_size(pk, n);
  if (pk == PK_BE16) return !memcmp(in, p, n);
  // (the packed bytes are made in runs of 64 words: in's bytes for them start at 14 k / 8, byte
  // aligned every 4 words for the 14-bit packings, at 2 k for the 16-bit one)
  for (size_t at = 0; at < total; at += sizeof w) {
    const size_t m = total - at < sizeof w ? total - at : sizeof w;
    if (pk == PK_LE16) {
      for (size_t i = 0; i < m; i++) {
        const size_t s = (at + i) ^ 1;
        w[i] = s < n ? in[s] : 0;
      }
    } else {
      const size_t k0 = at / 2, from = k0 * 14 / 8;  // (k0 a multiple of 64: from whole)
      const size_t left = n - from;
      pack_pack(pk, in + from, left < 112 ? left : 112, w);  // (64 words: 112 bytes)
    }
    if (memcmp(w, p + at, m)) return false;
  }
  return true;
}
