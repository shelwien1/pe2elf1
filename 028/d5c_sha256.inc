/* d5c_sha256.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * SHA-256, used only to key the dedup table (d5c_dedup.inc).
 *
 * Why SHA-256 and not SHA-512: the key is a content fingerprint for a hash
 * table, not a signature.  128 bits of it is already 2^-64 collision odds over
 * 2^32 distinct textures, the table verifies every hit with a memcmp against
 * the retained bytes anyway, and the archive carries a crc32 per entry on top
 * of that -- so the extra 256 bits would buy nothing while costing about twice
 * the time per byte on 32-bit builds (SHA-512 is a 64-bit-word design).
 *
 * Plain reference implementation: this runs once over each carved texture,
 * which is a fraction of what the model then does with the same bytes. */

struct SHA256 {
  u32 h[8]; u64 len; u8 buf[64]; size_t n;

  SHA256() { reset(); }
  void reset() {
    static const u32 iv[8] = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                               0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };
    for(int i = 0; i < 8; i++) h[i] = iv[i];
    len = 0; n = 0;
  }
  static u32 ror(u32 x, int r) { return (x >> r) | (x << (32 - r)); }

  void block(const u8* p) {
    static const u32 K[64] = {
      0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
      0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
      0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
      0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
      0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
      0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
      0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
      0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u };
    u32 w[64];
    for(int i = 0; i < 16; i++)
      w[i] = u32(p[i*4]) << 24 | u32(p[i*4+1]) << 16 | u32(p[i*4+2]) << 8 | u32(p[i*4+3]);
    for(int i = 16; i < 64; i++) {
      const u32 s0 = ror(w[i-15],7) ^ ror(w[i-15],18) ^ (w[i-15] >> 3);
      const u32 s1 = ror(w[i-2],17) ^ ror(w[i-2],19) ^ (w[i-2] >> 10);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    u32 a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for(int i = 0; i < 64; i++) {
      const u32 S1 = ror(e,6) ^ ror(e,11) ^ ror(e,25);
      const u32 ch = (e & f) ^ (~e & g);
      const u32 t1 = hh + S1 + ch + K[i] + w[i];
      const u32 S0 = ror(a,2) ^ ror(a,13) ^ ror(a,22);
      const u32 mj = (a & b) ^ (a & c) ^ (b & c);
      const u32 t2 = S0 + mj;
      hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
  }

  void update(const u8* p, size_t k) {
    len += k;
    if(n) {                                   // top up the partial block first
      size_t t = 64 - n; if(t > k) t = k;
      memcpy(buf + n, p, t); n += t; p += t; k -= t;
      if(n < 64) return;
      block(buf); n = 0;
    }
    while(k >= 64) { block(p); p += 64; k -= 64; }
    if(k) { memcpy(buf, p, k); n = k; }
  }

  void final(u8 out[32]) {
    const u64 bits = len * 8;
    u8 pad = 0x80; update(&pad, 1);
    static const u8 z = 0;
    while(n != 56) update(&z, 1);
    u8 lb[8]; for(int i = 0; i < 8; i++) lb[i] = u8(bits >> (56 - 8*i));
    update(lb, 8);
    for(int i = 0; i < 8; i++) {
      out[i*4]   = u8(h[i] >> 24); out[i*4+1] = u8(h[i] >> 16);
      out[i*4+2] = u8(h[i] >> 8);  out[i*4+3] = u8(h[i]);
    }
  }
};

/* The 128-bit key the dedup table indexes by: the top half of SHA-256, as two
 * big-endian u64s so the value prints and compares the same everywhere. */
struct Hash128 {
  u64 a = 0, b = 0;
  bool operator==(const Hash128& o) const { return a == o.a && b == o.b; }
  bool operator!=(const Hash128& o) const { return !(*this == o); }
};

static Hash128 hash128(const u8* d, size_t n) {
  SHA256 s; s.update(d, n);
  u8 o[32]; s.final(o);
  Hash128 r;
  for(int i = 0; i < 8; i++) r.a = (r.a << 8) | o[i];
  for(int i = 0; i < 8; i++) r.b = (r.b << 8) | o[8 + i];
  return r;
}
