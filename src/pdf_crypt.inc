// pdf_crypt.inc -- standard security handler: MD5, SHA-2, RC4, AES, key recovery.
//
// Key recovery runs in c only. d gets the recovered file key from the dump header, derives
// per-object keys (V < 5) and re-encrypts; it never runs a password algorithm.

// ---- MD5 -------------------------------------------------------------------------------------
struct Md5 {
  uint32_t h[4] = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};
  uint8_t buf[64];
  uint64_t len = 0;
  static uint32_t rol(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }
  void block(const uint8_t* p) {
    static const uint32_t K[64] = {
        0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
        0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
        0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau, 0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
        0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu, 0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
        0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
        0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u, 0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
        0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
        0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u, 0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u};
    static const int R[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                              5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                              4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                              6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
    uint32_t m[16];
    for (int i = 0; i < 16; i++) m[i] = p[4 * i] | (p[4 * i + 1] << 8) | (p[4 * i + 2] << 16) | ((uint32_t)p[4 * i + 3] << 24);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    for (int i = 0; i < 64; i++) {
      uint32_t f;
      int g;
      if (i < 16) { f = (b & c) | (~b & d); g = i; }
      else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
      else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) & 15; }
      else { f = c ^ (b | ~d); g = (7 * i) & 15; }
      uint32_t t = d;
      d = c;
      c = b;
      b = b + rol(a + f + K[i] + m[g], R[i]);
      a = t;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
  }
  void update(const void* data, size_t n) {
    if (n == 0) return;  // data may be null for an empty vector
    const uint8_t* p = (const uint8_t*)data;
    size_t fill = len & 63;
    len += n;
    if (fill) {
      size_t k = std::min(n, 64 - fill);
      memcpy(buf + fill, p, k);
      p += k; n -= k; fill += k;
      if (fill < 64) return;
      block(buf);
    }
    for (; n >= 64; p += 64, n -= 64) block(p);
    memcpy(buf, p, n);
  }
  void final(uint8_t out[16]) {
    uint64_t bits = len * 8;
    uint8_t pad = 0x80;
    update(&pad, 1);
    uint8_t z = 0;
    while ((len & 63) != 56) update(&z, 1);
    uint8_t lb[8];
    for (int i = 0; i < 8; i++) lb[i] = (uint8_t)(bits >> (8 * i));
    update(lb, 8);
    for (int i = 0; i < 4; i++)
      for (int k = 0; k < 4; k++) out[4 * i + k] = (uint8_t)(h[i] >> (8 * k));
  }
};

static void md5(const uint8_t* p, size_t n, uint8_t out[16]) {
  Md5 m;
  m.update(p, n);
  m.final(out);
}

// ---- SHA-256 / SHA-384 / SHA-512 ------------------------------------------------------------
static Bytes sha256(const uint8_t* p, size_t n) {
  static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
  uint32_t h[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
  auto ror = [](uint32_t x, int c) { return (x >> c) | (x << (32 - c)); };
  Bytes msg(p, p + n);
  uint64_t bits = (uint64_t)n * 8;
  msg.push_back(0x80);
  while (msg.size() % 64 != 56) msg.push_back(0);
  for (int i = 7; i >= 0; i--) msg.push_back((uint8_t)(bits >> (8 * i)));
  for (size_t off = 0; off < msg.size(); off += 64) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
      w[i] = ((uint32_t)msg[off + 4 * i] << 24) | (msg[off + 4 * i + 1] << 16) | (msg[off + 4 * i + 2] << 8) | msg[off + 4 * i + 3];
    for (int i = 16; i < 64; i++) {
      uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
      uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 64; i++) {
      uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
      uint32_t ch = (e & f) ^ (~e & g);
      uint32_t t1 = hh + S1 + ch + K[i] + w[i];
      uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
      uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
      uint32_t t2 = S0 + mj;
      hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
  }
  Bytes out(32);
  for (int i = 0; i < 8; i++)
    for (int k = 0; k < 4; k++) out[4 * i + k] = (uint8_t)(h[i] >> (24 - 8 * k));
  return out;
}

static Bytes sha512x(const uint8_t* p, size_t n, bool is384) {
  static const uint64_t K[80] = {
    0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full, 0xe9b5dba58189dbbcull,
    0x3956c25bf348b538ull, 0x59f111f1b605d019ull, 0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull,
    0xd807aa98a3030242ull, 0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
    0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull, 0xc19bf174cf692694ull,
    0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull, 0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull,
    0x2de92c6f592b0275ull, 0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
    0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full, 0xbf597fc7beef0ee4ull,
    0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull, 0x06ca6351e003826full, 0x142929670a0e6e70ull,
    0x27b70a8546d22ffcull, 0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
    0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull, 0x92722c851482353bull,
    0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull, 0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull,
    0xd192e819d6ef5218ull, 0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
    0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull, 0x34b0bcb5e19b48a8ull,
    0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull, 0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull,
    0x748f82ee5defb2fcull, 0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
    0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull, 0xc67178f2e372532bull,
    0xca273eceea26619cull, 0xd186b8c721c0c207ull, 0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull,
    0x06f067aa72176fbaull, 0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
    0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull, 0x431d67c49c100d4cull,
    0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull, 0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull};
  static const uint64_t H512[8] = {0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull, 0xa54ff53a5f1d36f1ull, 0x510e527fade682d1ull, 0x9b05688c2b3e6c1full, 0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull};
  static const uint64_t H384[8] = {0xcbbb9d5dc1059ed8ull, 0x629a292a367cd507ull, 0x9159015a3070dd17ull, 0x152fecd8f70e5939ull, 0x67332667ffc00b31ull, 0x8eb44a8768581511ull, 0xdb0c2e0d64f98fa7ull, 0x47b5481dbefa4fa4ull};
  uint64_t h[8];
  memcpy(h, is384 ? H384 : H512, sizeof(h));
  auto ror = [](uint64_t x, int c) { return (x >> c) | (x << (64 - c)); };
  Bytes msg(p, p + n);
  uint64_t bits = (uint64_t)n * 8;
  msg.push_back(0x80);
  while (msg.size() % 128 != 112) msg.push_back(0);
  for (int i = 0; i < 8; i++) msg.push_back(0);
  for (int i = 7; i >= 0; i--) msg.push_back((uint8_t)(bits >> (8 * i)));
  for (size_t off = 0; off < msg.size(); off += 128) {
    uint64_t w[80];
    for (int i = 0; i < 16; i++) {
      uint64_t v = 0;
      for (int k = 0; k < 8; k++) v = (v << 8) | msg[off + 8 * i + k];
      w[i] = v;
    }
    for (int i = 16; i < 80; i++) {
      uint64_t s0 = ror(w[i - 15], 1) ^ ror(w[i - 15], 8) ^ (w[i - 15] >> 7);
      uint64_t s1 = ror(w[i - 2], 19) ^ ror(w[i - 2], 61) ^ (w[i - 2] >> 6);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint64_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 80; i++) {
      uint64_t S1 = ror(e, 14) ^ ror(e, 18) ^ ror(e, 41);
      uint64_t ch = (e & f) ^ (~e & g);
      uint64_t t1 = hh + S1 + ch + K[i] + w[i];
      uint64_t S0 = ror(a, 28) ^ ror(a, 34) ^ ror(a, 39);
      uint64_t mj = (a & b) ^ (a & c) ^ (b & c);
      uint64_t t2 = S0 + mj;
      hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
  }
  Bytes out(is384 ? 48 : 64);
  for (size_t i = 0; i < out.size(); i++) out[i] = (uint8_t)(h[i / 8] >> (56 - 8 * (i % 8)));
  return out;
}

// ---- RC4 -------------------------------------------------------------------------------------
static void rc4(const uint8_t* key, size_t klen, const uint8_t* in, size_t n, uint8_t* out) {
  uint8_t S[256];
  for (int i = 0; i < 256; i++) S[i] = (uint8_t)i;
  for (int i = 0, j = 0; i < 256; i++) {
    j = (j + S[i] + key[i % klen]) & 255;
    std::swap(S[i], S[j]);
  }
  int i = 0, j = 0;
  for (size_t k = 0; k < n; k++) {
    i = (i + 1) & 255;
    j = (j + S[i]) & 255;
    std::swap(S[i], S[j]);
    out[k] = in[k] ^ S[(S[i] + S[j]) & 255];
  }
}

// ---- AES -------------------------------------------------------------------------------------
struct Aes {
  static uint8_t sbox[256], inv[256];
  static void init() {
    static const bool once = build();  // thread-safe one-time initialization
    (void)once;
  }
  static bool build() {
    uint8_t p = 1, q = 1;
    do {
      p = (uint8_t)(p ^ (p << 1) ^ ((p & 0x80) ? 0x1B : 0));
      q ^= (uint8_t)(q << 1);
      q ^= (uint8_t)(q << 2);
      q ^= (uint8_t)(q << 4);
      if (q & 0x80) q ^= 0x09;
      auto rl = [](uint8_t x, int s) { return (uint8_t)((x << s) | (x >> (8 - s))); };
      uint8_t x = (uint8_t)(q ^ rl(q, 1) ^ rl(q, 2) ^ rl(q, 3) ^ rl(q, 4));
      sbox[p] = (uint8_t)(x ^ 0x63);
    } while (p != 1);
    sbox[0] = 0x63;
    for (int i = 0; i < 256; i++) inv[sbox[i]] = (uint8_t)i;
    return true;
  }
  static uint8_t xt(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0)); }
  static uint8_t mul(uint8_t a, uint8_t b) {
    uint8_t r = 0;
    while (b) {
      if (b & 1) r ^= a;
      a = xt(a);
      b >>= 1;
    }
    return r;
  }
  uint8_t rk[240];
  int nr = 0;
  Aes(const uint8_t* key, size_t klen) {
    init();
    int nk = (int)klen / 4;
    nr = nk + 6;
    memcpy(rk, key, klen);
    uint8_t rcon = 1;
    for (int i = nk; i < 4 * (nr + 1); i++) {
      uint8_t t[4];
      memcpy(t, rk + 4 * (i - 1), 4);
      if (i % nk == 0) {
        uint8_t t0 = t[0];
        t[0] = (uint8_t)(sbox[t[1]] ^ rcon);
        t[1] = sbox[t[2]];
        t[2] = sbox[t[3]];
        t[3] = sbox[t0];
        rcon = xt(rcon);
      } else if (nk > 6 && i % nk == 4) {
        for (int k = 0; k < 4; k++) t[k] = sbox[t[k]];
      }
      for (int k = 0; k < 4; k++) rk[4 * i + k] = (uint8_t)(rk[4 * (i - nk) + k] ^ t[k]);
    }
  }
  void encBlock(const uint8_t* in, uint8_t* out) const {
    uint8_t s[16], t[16];
    for (int i = 0; i < 16; i++) s[i] = (uint8_t)(in[i] ^ rk[i]);
    for (int r = 1; r <= nr; r++) {
      for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) t[row + 4 * c] = sbox[s[row + 4 * ((c + row) & 3)]];
      if (r != nr) {
        for (int c = 0; c < 4; c++) {
          uint8_t a0 = t[4 * c], a1 = t[4 * c + 1], a2 = t[4 * c + 2], a3 = t[4 * c + 3];
          s[4 * c] = (uint8_t)(xt(a0) ^ xt(a1) ^ a1 ^ a2 ^ a3);
          s[4 * c + 1] = (uint8_t)(a0 ^ xt(a1) ^ xt(a2) ^ a2 ^ a3);
          s[4 * c + 2] = (uint8_t)(a0 ^ a1 ^ xt(a2) ^ xt(a3) ^ a3);
          s[4 * c + 3] = (uint8_t)(xt(a0) ^ a0 ^ a1 ^ a2 ^ xt(a3));
        }
      } else {
        memcpy(s, t, 16);
      }
      for (int i = 0; i < 16; i++) s[i] ^= rk[16 * r + i];
    }
    memcpy(out, s, 16);
  }
  void decBlock(const uint8_t* in, uint8_t* out) const {
    uint8_t s[16], t[16];
    for (int i = 0; i < 16; i++) s[i] = (uint8_t)(in[i] ^ rk[16 * nr + i]);
    for (int r = nr - 1; r >= 0; r--) {
      for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) t[row + 4 * ((c + row) & 3)] = inv[s[row + 4 * c]];
      for (int i = 0; i < 16; i++) t[i] ^= rk[16 * r + i];
      if (r) {
        for (int c = 0; c < 4; c++) {
          uint8_t a0 = t[4 * c], a1 = t[4 * c + 1], a2 = t[4 * c + 2], a3 = t[4 * c + 3];
          s[4 * c] = (uint8_t)(mul(a0, 14) ^ mul(a1, 11) ^ mul(a2, 13) ^ mul(a3, 9));
          s[4 * c + 1] = (uint8_t)(mul(a0, 9) ^ mul(a1, 14) ^ mul(a2, 11) ^ mul(a3, 13));
          s[4 * c + 2] = (uint8_t)(mul(a0, 13) ^ mul(a1, 9) ^ mul(a2, 14) ^ mul(a3, 11));
          s[4 * c + 3] = (uint8_t)(mul(a0, 11) ^ mul(a1, 13) ^ mul(a2, 9) ^ mul(a3, 14));
        }
      } else {
        memcpy(s, t, 16);
      }
    }
    memcpy(out, s, 16);
  }
};
uint8_t Aes::sbox[256];
uint8_t Aes::inv[256];

// CBC without padding; n must be a multiple of 16
static void aesCbcEncrypt(const uint8_t* key, size_t klen, const uint8_t* iv, const uint8_t* in, size_t n, uint8_t* out) {
  Aes a(key, klen);
  uint8_t chain[16], blk[16];
  memcpy(chain, iv, 16);
  for (size_t off = 0; off < n; off += 16) {
    for (int i = 0; i < 16; i++) blk[i] = (uint8_t)(in[off + i] ^ chain[i]);
    a.encBlock(blk, out + off);
    memcpy(chain, out + off, 16);
  }
}
static void aesCbcDecrypt(const uint8_t* key, size_t klen, const uint8_t* iv, const uint8_t* in, size_t n, uint8_t* out) {
  Aes a(key, klen);
  uint8_t chain[16], blk[16];
  memcpy(chain, iv, 16);
  for (size_t off = 0; off < n; off += 16) {
    uint8_t save[16];
    memcpy(save, in + off, 16);
    a.decBlock(save, blk);
    for (int i = 0; i < 16; i++) out[off + i] = (uint8_t)(blk[i] ^ chain[i]);
    memcpy(chain, save, 16);
  }
}

// ---- standard security handler ---------------------------------------------------------------
enum Cipher : uint8_t { CI_NONE, CI_RC4, CI_AES128, CI_AES256 };
static const char* const kCipherName[] = {"none", "RC4", "AES-128", "AES-256"};

static const uint8_t kPwPad[32] = {0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41, 0x64, 0x00, 0x4E,
                                   0x56, 0xFF, 0xFA, 0x01, 0x08, 0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68,
                                   0x3E, 0x80, 0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A};

// Per-object key for V < 5 (Algorithm 1); V5 uses the file key directly.
static Bytes objectKey(const Bytes& fileKey, uint32_t num, uint32_t gen, bool aes) {
  Md5 m;
  m.update(fileKey.data(), fileKey.size());
  uint8_t b[5] = {(uint8_t)num, (uint8_t)(num >> 8), (uint8_t)(num >> 16), (uint8_t)gen, (uint8_t)(gen >> 8)};
  m.update(b, 5);
  if (aes) m.update("sAlT", 4);
  uint8_t h[16];
  m.final(h);
  return Bytes(h, h + std::min<size_t>(fileKey.size() + 5, 16));
}

// Decrypts one stream/string. Returns false when the unit cannot round-trip (AES: bad length or
// non-canonical padding); then the caller keeps the ciphertext.
static bool cipherDecrypt(uint8_t ci, const Bytes& fileKey, uint32_t num, uint32_t gen, const uint8_t* in, size_t n,
                          Bytes& out, uint8_t iv[16]) {
  if (ci == CI_RC4) {
    Bytes k = objectKey(fileKey, num, gen, false);
    out.resize(n);
    if (n) rc4(k.data(), k.size(), in, n, out.data());
    return true;
  }
  if (ci == CI_AES128 || ci == CI_AES256) {
    if (n < 32 || n % 16) return false;
    Bytes k = ci == CI_AES128 ? objectKey(fileKey, num, gen, true) : fileKey;
    if (k.size() != (ci == CI_AES128 ? 16u : 32u)) return false;
    memcpy(iv, in, 16);
    out.resize(n - 16);
    aesCbcDecrypt(k.data(), k.size(), iv, in + 16, n - 16, out.data());
    uint8_t p = out.back();
    if (p < 1 || p > 16) return false;
    for (size_t i = 0; i < p; i++)
      if (out[out.size() - 1 - i] != p) return false;
    out.resize(out.size() - p);
    return true;
  }
  return false;
}

// false (out empty) if the file key has the wrong length for the cipher
static bool cipherEncrypt(uint8_t ci, const Bytes& fileKey, uint32_t num, uint32_t gen, const uint8_t iv[16],
                          const Bytes& in, Bytes& out) {
  if (ci == CI_RC4) {
    Bytes k = objectKey(fileKey, num, gen, false);
    out.resize(in.size());
    if (!in.empty()) rc4(k.data(), k.size(), in.data(), in.size(), out.data());
    return true;
  }
  Bytes k = ci == CI_AES128 ? objectKey(fileKey, num, gen, true) : fileKey;
  if (k.size() != (ci == CI_AES128 ? 16u : 32u)) {
    out.clear();
    return false;
  }
  size_t p = 16 - in.size() % 16;
  Bytes padded(in);
  padded.insert(padded.end(), p, (uint8_t)p);
  out.resize(16 + padded.size());
  memcpy(out.data(), iv, 16);
  aesCbcEncrypt(k.data(), k.size(), iv, padded.data(), padded.size(), out.data() + 16);
  return true;
}

struct Security {
  bool present = false;   // /Encrypt in a trailer
  bool standard = false;  // /Filter /Standard
  uint32_t encNum = 0;    // object number of the /Encrypt dictionary (0: direct)
  int V = 0, R = 0, lengthBits = 0;
  Bytes O, U, OE, UE, ID0;
  int32_t P = 0;
  bool encryptMetadata = true;
  std::string stmF = "Identity", strF = "Identity", eff;
  std::map<std::string, uint8_t> cfm;
  Bytes key;
  std::string keySource;  // "empty user password", "user password", "owner password"

  static Bytes strBytes(const Obj* o) {
    if (!o || o->t != Obj::STR) return Bytes();
    return Bytes(o->s.begin(), o->s.end());
  }

  void load(const Obj& enc, const Obj* id) {
    present = true;
    const Obj* f = enc.get("Filter");
    standard = f && f->isName("Standard");
    if (const Obj* v = enc.get("V")) V = v->t == Obj::INT ? (int)v->i : 0;
    if (const Obj* v = enc.get("R")) R = v->t == Obj::INT ? (int)v->i : 0;
    if (const Obj* v = enc.get("Length")) lengthBits = v->t == Obj::INT ? (int)v->i : 0;
    if (const Obj* v = enc.get("P")) P = v->t == Obj::INT ? (int32_t)(uint32_t)v->i : 0;
    if (const Obj* v = enc.get("EncryptMetadata")) encryptMetadata = !(v->t == Obj::BOOL && !v->i);
    O = strBytes(enc.get("O"));
    U = strBytes(enc.get("U"));
    OE = strBytes(enc.get("OE"));
    UE = strBytes(enc.get("UE"));
    if (id && id->t == Obj::ARR && !id->a.empty()) ID0 = strBytes(&id->a[0]);
    if (V >= 4) {
      if (const Obj* cf = enc.get("CF")) {
        if (cf->t == Obj::DICT) {
          for (size_t k = 0; k + 1 < cf->a.size(); k += 2) {
            const Obj& d = cf->a[k + 1];
            uint8_t m = CI_NONE;
            if (const Obj* cm = d.get("CFM")) {
              if (cm->isName("V2")) m = CI_RC4;
              else if (cm->isName("AESV2")) m = CI_AES128;
              else if (cm->isName("AESV3")) m = CI_AES256;
            }
            cfm[cf->a[k].s] = m;
          }
        }
      }
      if (const Obj* s = enc.get("StmF")) if (s->t == Obj::NAME) stmF = s->s;
      if (const Obj* s = enc.get("StrF")) if (s->t == Obj::NAME) strF = s->s;
      if (const Obj* s = enc.get("EFF")) if (s->t == Obj::NAME) eff = s->s;
    }
  }

  uint8_t filterCipher(const std::string& name) const {
    if (V < 4) return CI_RC4;
    if (name == "Identity") return CI_NONE;
    auto it = cfm.find(name);
    return it == cfm.end() ? (uint8_t)CI_NONE : it->second;
  }
  uint8_t streamCipher() const { return filterCipher(stmF); }
  uint8_t embeddedCipher() const { return filterCipher(eff.empty() ? stmF : eff); }

  size_t klen = 5;  // key length in bytes being tried (V < 5)

  // Candidate key lengths: qpdf's rule first (V<=1: 40 bits, V4: 128, V2/3: /Length if valid
  // else 128), then the spec default of 40 bits. The U check decides which one is right.
  std::vector<size_t> keyLens() const {
    if (R == 2 || V <= 1) return {5};
    if (V == 4) return {16};
    size_t n = (lengthBits % 8 == 0 && lengthBits >= 40 && lengthBits <= 128) ? (size_t)lengthBits / 8 : 16;
    if (n == 5) return {5};
    return {n, 5};
  }
  size_t keyLen() const { return klen; }

  static Bytes pad(const std::string& pw) {
    Bytes r(32);
    size_t n = std::min<size_t>(32, pw.size());
    memcpy(r.data(), pw.data(), n);
    memcpy(r.data() + n, kPwPad, 32 - n);
    return r;
  }

  Bytes computeKey(const Bytes& padded) const {
    Md5 m;
    m.update(padded.data(), 32);
    m.update(O.data(), std::min<size_t>(32, O.size()));
    uint8_t p4[4] = {(uint8_t)P, (uint8_t)(P >> 8), (uint8_t)(P >> 16), (uint8_t)(P >> 24)};
    m.update(p4, 4);
    m.update(ID0.data(), ID0.size());
    if (R >= 4 && !encryptMetadata) m.update("\xff\xff\xff\xff", 4);
    uint8_t h[16];
    m.final(h);
    size_t n = keyLen();
    if (R >= 3)
      for (int i = 0; i < 50; i++) md5(h, n, h);
    return Bytes(h, h + n);
  }

  bool checkUserKey(const Bytes& k) const {
    if (R == 2) {
      uint8_t u[32];
      rc4(k.data(), k.size(), kPwPad, 32, u);
      return U.size() >= 32 && memcmp(u, U.data(), 32) == 0;
    }
    Md5 m;
    m.update(kPwPad, 32);
    m.update(ID0.data(), ID0.size());
    uint8_t x[16];
    m.final(x);
    rc4(k.data(), k.size(), x, 16, x);
    Bytes kk(k);
    for (int i = 1; i <= 19; i++) {
      for (size_t j = 0; j < k.size(); j++) kk[j] = (uint8_t)(k[j] ^ i);
      rc4(kk.data(), kk.size(), x, 16, x);
    }
    return U.size() >= 16 && memcmp(x, U.data(), 16) == 0;
  }

  bool tryUserR4(const Bytes& padded) {
    Bytes k = computeKey(padded);
    if (!checkUserKey(k)) return false;
    key = k;
    return true;
  }

  bool tryOwnerR4(const std::string& pw) {
    if (O.size() < 32) return false;
    uint8_t h[16];
    Bytes p = pad(pw);
    md5(p.data(), 32, h);
    if (R >= 3)
      for (int i = 0; i < 50; i++) md5(h, 16, h);
    size_t n = keyLen();
    Bytes x(O.begin(), O.begin() + 32);
    if (R == 2) {
      rc4(h, n, x.data(), 32, x.data());
    } else {
      uint8_t kk[16];
      for (int i = 19; i >= 0; i--) {
        for (size_t j = 0; j < n; j++) kk[j] = (uint8_t)(h[j] ^ i);
        rc4(kk, n, x.data(), 32, x.data());
      }
    }
    return tryUserR4(x);
  }

  static Bytes hash2B(const std::string& pw, const uint8_t* salt, const uint8_t* udata, size_t ulen, bool r6) {
    Bytes in(pw.begin(), pw.end());
    in.insert(in.end(), salt, salt + 8);
    in.insert(in.end(), udata, udata + ulen);
    Bytes k = sha256(in.data(), in.size());
    if (!r6) return k;
    for (int round = 0;; round++) {
      Bytes k1;
      for (int i = 0; i < 64; i++) {
        k1.insert(k1.end(), pw.begin(), pw.end());
        k1.insert(k1.end(), k.begin(), k.end());
        k1.insert(k1.end(), udata, udata + ulen);
      }
      Bytes e(k1.size());
      aesCbcEncrypt(k.data(), 16, k.data() + 16, k1.data(), k1.size(), e.data());
      unsigned sum = 0;
      for (int i = 0; i < 16; i++) sum += e[i];
      switch (sum % 3) {
        case 0: k = sha256(e.data(), e.size()); break;
        case 1: k = sha512x(e.data(), e.size(), true); break;
        default: k = sha512x(e.data(), e.size(), false); break;
      }
      if (round >= 63 && e.back() <= round + 1 - 32) break;
    }
    k.resize(32);
    return k;
  }

  bool tryR6(const std::string& pw0, bool owner) {
    std::string pw = pw0.substr(0, 127);
    bool r6 = R >= 6;
    if (U.size() < 48 || (owner && O.size() < 48)) return false;
    const Bytes& X = owner ? O : U;
    const Bytes& XE = owner ? OE : UE;
    const uint8_t* ud = owner ? U.data() : nullptr;
    size_t ul = owner ? 48 : 0;
    Bytes h = hash2B(pw, X.data() + 32, ud, ul, r6);
    if (memcmp(h.data(), X.data(), 32) != 0) return false;
    if (XE.size() < 32) return false;
    Bytes ik = hash2B(pw, X.data() + 40, ud, ul, r6);
    uint8_t iv[16] = {0};
    key.resize(32);
    aesCbcDecrypt(ik.data(), 32, iv, XE.data(), 32, key.data());
    return true;
  }

  // Recovers the file key: empty user password, then pw as user, then as owner password.
  bool recover(const std::string& pw) {
    if (!standard) return false;
    if (R >= 5) {
      if (tryR6("", false)) { keySource = "empty user password"; return true; }
      if (!pw.empty() && tryR6(pw, false)) { keySource = "user password"; return true; }
      if (!pw.empty() && tryR6(pw, true)) { keySource = "owner password"; return true; }
      return false;
    }
    if (R < 2) return false;
    for (size_t n : keyLens()) {
      klen = n;
      if (tryUserR4(pad(""))) { keySource = "empty user password"; return true; }
      if (!pw.empty() && tryUserR4(pad(pw))) { keySource = "user password"; return true; }
      if (tryOwnerR4(pw)) { keySource = pw.empty() ? "empty owner password" : "owner password"; return true; }
    }
    return false;
  }
};
