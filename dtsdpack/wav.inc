// wav.inc -- WavIn: the WAV the core is coded against, read as the core goes, a window of its
// samples at a time.

constexpr int WAV_CH = 32;           // its channels, at most
constexpr int WAV_PROBE = 64 * KIB;  // the bytes of its start whose CRC the header keeps
constexpr S64 WAV_WIN = 1 << 20;     // the samples of the window (a channel), a power of 2

// A PCM WAV (16, 24 or 32-bit; RIFF or RF64), its samples as 24-bit values (16-bit ones shifted
// up, 32-bit ones down: FFmpeg's 32-bit output of a 24-bit decode), a window of them at a time: the
// samples [lo, got) of each channel in rings of WAV_WIN, read as need() asks for later ones; a file
// or a pipe alike. Plain data: init() opens it, quit() frees what it holds.
struct WavIn {
  FILE* f;
  int ok;        // a WAV it reads (else the core is coded without one)
  int nch, bps;  // its channels, its bytes a sample
  U32 rate;      // its samples a second
  U64 nsamp;     // its samples a channel (as its header says, or to its end)
  U64 size;      // the file's bytes (0: not known, a pipe)
  U32 crc;       // the CRC-32 of its first WAV_PROBE bytes (or of all, where fewer)
  U64 got;       // the samples read so far
  S64 lo;        // the samples before lo are given up
  S32* ring[WAV_CH];
  U8* rb;            // a read's bytes
  U8* probe;         // the first bytes, the samples after the header in them not taken yet
  size_t pn, pat;    // their number, and where the next sample's bytes start
  char why[128];     // why it isn't a WAV it reads

  void init(FILE* fp) {
    memset(this, 0, sizeof *this);
    f = fp;
    probe = (U8*)xmalloc(WAV_PROBE);
    while (pn < (size_t)WAV_PROBE) {
      const size_t r = fread(probe + pn, 1, WAV_PROBE - pn, f);
      if (!r) break;
      pn += r;
    }
    crc = crc32_upd(0, probe, pn);
    struct stat st;
    if (!fstat(fileno(f), &st) && S_ISREG(st.st_mode)) size = (U64)st.st_size;
    if (!parse()) return;
    for (int c = 0; c < nch; c++) ring[c] = (S32*)xmalloc(sizeof(S32) * WAV_WIN);
    rb = (U8*)xmalloc((size_t)IO_BUF * nch * bps);
    ok = 1;
  }
  void quit() {
    for (int c = 0; c < WAV_CH; c++) free(ring[c]), ring[c] = nullptr;
    free(rb);
    free(probe);
    rb = probe = nullptr;
  }

  // sample s of channel c: 0 outside the WAV; s in the window need() made
  INLINE S32 at(int c, S64 s) const {
    if (s < 0 || (U64)s >= got || c >= nch) return 0;
    return ring[c][s & (WAV_WIN - 1)];
  }
  // the samples up to s (not included) read, those before from given up (s - from <= WAV_WIN)
  void need(S64 from, S64 s) {
    if (!ok) return;
    if (from > lo) lo = from;
    while ((S64)got < s && got < nsamp) {
      const S64 room = lo + WAV_WIN - (S64)got;  // (the ring's free slots)
      if (room <= 0) break;
      U64 k = (U64)(s - (S64)got);
      if (k > (U64)room) k = (U64)room;
      if (k > IO_BUF) k = IO_BUF;
      if (k > nsamp - got) k = nsamp - got;
      if (!read_samples(k)) {
        nsamp = got;  // (cut short: what came is all)
        break;
      }
    }
  }

 private:
  // the header in the probe: the format, where the samples start and how many there are
  bool parse() {
    const U8* h = probe;
    const size_t n = pn;
    auto le16 = [&](size_t i) { return (U32)h[i] | (U32)h[i + 1] << 8; };
    auto le32 = [&](size_t i) { return le16(i) | le16(i + 2) << 16; };
    auto le64 = [&](size_t i) { return (U64)le32(i) | (U64)le32(i + 4) << 32; };
    if (n < 12 || (memcmp(h, "RIFF", 4) && memcmp(h, "RF64", 4)) || memcmp(h + 8, "WAVE", 4))
      return snprintf(why, sizeof why, "not a WAV"), false;
    U64 ds64 = 0;
    int fmt = 0, bits = 0;
    size_t i = 12;
    while (i + 8 <= n) {
      const U32 sz = le32(i + 4);
      if (!memcmp(h + i, "ds64", 4) && i + 24 <= n) ds64 = le64(i + 16);
      if (!memcmp(h + i, "fmt ", 4) && i + 24 <= n) {
        fmt = (int)le16(i + 8);
        nch = (int)le16(i + 10);
        rate = le32(i + 12);
        bits = (int)le16(i + 22);
        if (fmt == 0xFFFE && sz >= 40 && i + 8 + 26 <= n) fmt = (int)le16(i + 8 + 24);  // (the subformat's)
      }
      if (!memcmp(h + i, "data", 4)) {
        if (fmt != 1 || nch < 1 || nch > WAV_CH || (bits != 16 && bits != 24 && bits != 32))
          return snprintf(why, sizeof why, "not a WAV of 16, 24 or 32-bit PCM, 1 to %d channels", WAV_CH), false;
        bps = bits / 8;
        pat = i + 8;
        U64 dn = sz == 0xFFFFFFFFu ? ds64 : sz;
        if (size && (!dn || pat + dn > size)) dn = size - pat;  // (as much as there is)
        if (!dn && !size) dn = ~0ULL;                          // (a pipe: to its end)
        nsamp = dn / ((U64)nch * bps);
        return true;
      }
      i += 8 + sz + (sz & 1);
    }
    return snprintf(why, sizeof why, "a WAV whose samples don't start in its first %u KB", WAV_PROBE / KIB), false;
  }
  // k more samples of each channel into the rings: the probe's first, then the file's
  bool read_samples(U64 k) {
    const size_t want = (size_t)k * nch * bps;
    size_t have = 0;
    if (pat < pn) {
      have = pn - pat < want ? pn - pat : want;
      memcpy(rb, probe + pat, have);
      pat += have;
    }
    while (have < want) {
      const size_t r = fread(rb + have, 1, want - have, f);
      if (!r) break;
      have += r;
    }
    const U64 n = have / ((size_t)nch * bps);
    for (U64 t = 0; t < n; t++) {
      const U8* p = rb + (size_t)t * nch * bps;
      for (int c = 0; c < nch; c++, p += bps) {
        S32 v;
        if (bps == 2) v = (S32)(S16)((U32)p[0] | (U32)p[1] << 8) * 256;
        else if (bps == 3) v = (S32)((U32)p[0] << 8 | (U32)p[1] << 16 | (U32)p[2] << 24) >> 8;
        else v = (S32)((U32)p[0] | (U32)p[1] << 8 | (U32)p[2] << 16 | (U32)p[3] << 24) >> 8;
        ring[c][(got + t) & (WAV_WIN - 1)] = v;
      }
    }
    got += n;
    return n == k;
  }
};
