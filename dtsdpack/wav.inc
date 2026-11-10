// wav.inc -- WavIn: the WAV the core is coded against, read as the core goes, a window of its
// samples at a time.

constexpr int WAV_CH = 32;           // its channels, at most
constexpr int WAV_PROBE = 64 * KIB;  // the bytes of its start whose CRC the header keeps
constexpr S64 WAV_WIN = 1 << 20;     // the samples of the window (a channel), a power of 2

// A WAV at 4 times the core's rate, decimated by 2 as it's read (WavIn's set_dec(), as a 192 kHz
// stream's encoder makes the 96 kHz signal whose 64-band analysis's lower half is its core): sample
// n the sum of the taps times samples 2n + WAV_DEC_C + j, j from -WAV_DEC_J to WAV_DEC_J, rounded,
// 0s outside the WAV. The taps (Q30, summing to 2^30): a lowpass at 0.42 of the input's Nyquist
// frequency (a sinc, Kaiser-windowed at beta 8), its centre 0.58 samples before sample 2n +
// WAV_DEC_C; WAV_DEC_C 14 puts the core at the decoders' usual delay (1024 samples). (The delay
// as the core of FFmpeg's two 192 kHz DTS-HD MA samples lines up best, the fit's: its content ends
// below 15 kHz, where any flat lowpass does as well, so the cutoff is a guess. Those samples' cores
// are 44% smaller than coded without the WAV.)
constexpr int WAV_DEC_J = 24, WAV_DEC_C = 14, WAV_RAW = 4096;  // (WAV_RAW: the input kept, a power of 2)
static const S32 WAV_DEC2[2 * WAV_DEC_J + 1] = {
    -69707, -262655, 5317, 735963, 547330, -1257202, -2037770, 1078211,
    4560859, 1072724, -7248559, -6471260, 7847649, 15404865, -2858822, -26073743,
    -11764909, 33798619, 40293840, -29969149, -91304467, -7022835, 226997374, 427820933,
    407432363, 185588119, -33049818, -88388618, -15632339, 44727326, 26993055, -17301925,
    -23986646, 1495429, 15784536, 5256702, -7690119, -6129825, 2203840, 4320840,
    365726, -2149501, -930468, 702182, 634162, -85940, -246766, -37581,
    44484,
};

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
  S32* ext;          // past its end: each channel's continuation, ext_n samples of it (anal.inc's
  int ext_n, ext_on;  // wav_extend()), where ext_on (else 0s)
  int dec;             // its samples to those read (1; 2: decimated, set_dec())
  S32* raw[WAV_CH];    // (decimated: the input's last WAV_RAW samples of each channel, rgot of them
  U64 rgot;            // read, all where rend)
  int rend;

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
    dec = 1;
    ok = 1;
  }
  // its samples decimated by 2 (WAV_DEC2): its rate and samples halved; before any is read
  void set_dec(int d) {
    if (!ok || d != 2 || dec == 2 || got) return;
    dec = 2;
    rate /= 2;
    if (nsamp != ~0ULL) nsamp /= 2;
    for (int c = 0; c < nch; c++) raw[c] = (S32*)xmalloc(sizeof(S32) * WAV_RAW);
  }
  void quit() {
    for (int c = 0; c < WAV_CH; c++) free(ring[c]), free(raw[c]), ring[c] = raw[c] = nullptr;
    free(rb);
    free(probe);
    free(ext);
    rb = probe = nullptr;
    ext = nullptr;
  }

  // sample s of channel c: 0 outside the WAV (past its end, its continuation where made); s in the
  // window need() made
  INLINE S32 at(int c, S64 s) const {
    if (s < 0 || c >= nch) return 0;
    if ((U64)s >= got) {
      const U64 k = (U64)s - nsamp;
      return ext_on && got == nsamp && k < (U64)ext_n ? ext[(size_t)c * ext_n + (size_t)k] : 0;
    }
    return ring[c][s & (WAV_WIN - 1)];
  }
  // samples s..s + n - 1 of channel c (as at()'s): in the ring where they are there in a row, else
  // copied into buf
  INLINE const S32* span(int c, S64 s, int n, S32* buf) const {
    if (s >= 0 && (U64)(s + n) <= got && c < nch && (s & (WAV_WIN - 1)) + n <= WAV_WIN) return ring[c] + (s & (WAV_WIN - 1));
    // (else: 0s before the WAV, those read from the ring, then at()'s past them)
    const int i0 = s < 0 ? (int)(-s < n ? -s : n) : 0;
    int i1 = i0;
    if (c < nch && s + i0 < (S64)got) {
      i1 = (int)((S64)got - s < n ? (S64)got - s : n);
      if (((s + i0) & (WAV_WIN - 1)) + (i1 - i0) > WAV_WIN) i1 = i0;  // (where the ring wraps: at()'s)
    }
    for (int i = 0; i < i0; i++) buf[i] = 0;
    if (i1 > i0) memcpy(buf + i0, ring[c] + ((s + i0) & (WAV_WIN - 1)), sizeof(S32) * (size_t)(i1 - i0));
    if (c >= nch || s + i1 >= (S64)got) {  // (past those read: the continuation where made, else 0s)
      int i2 = i1;
      if (c < nch && ext_on && got == nsamp && s + i1 - (S64)nsamp < ext_n) {
        const S64 k = s + i1 - (S64)nsamp;
        i2 = i1 + (int)(ext_n - k < n - i1 ? ext_n - k : n - i1);
        memcpy(buf + i1, ext + (size_t)c * ext_n + k, sizeof(S32) * (size_t)(i2 - i1));
      }
      for (int i = i2; i < n; i++) buf[i] = 0;
    } else {
      for (int i = i1; i < n; i++) buf[i] = at(c, s + i);  // (where the ring wraps)
    }
    return buf;
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
  // k more samples of each channel into the rings (decimated: from the input's, read as needed)
  bool read_samples(U64 k) {
    if (dec == 1) {
      const U64 n = read_frames(ring, WAV_WIN - 1, got, k);
      got += n;
      return n == k;
    }
    U64 done = 0;
    while (done < k) {
      const U64 n = got + done;
      const S64 hi = 2 * (S64)n + WAV_DEC_C + WAV_DEC_J;  // (the last input sample it takes)
      if (!rend && (S64)rgot <= hi) {  // (WAV_RAW / 2 at a time: the oldest it takes kept)
        const U64 r = read_frames(raw, WAV_RAW - 1, rgot, WAV_RAW / 2);
        rgot += r;
        rend = r < (U64)WAV_RAW / 2;
        continue;
      }
      if (rend && n >= rgot / 2) break;  // (its end: the input's half)
      const S64 i0 = hi - 2 * WAV_DEC_J;  // (its first input sample: in the ring in a row, or not)
      const bool row = i0 >= 0 && hi < (S64)rgot && (i0 & (WAV_RAW - 1)) + 2 * WAV_DEC_J < WAV_RAW;
      for (int c = 0; c < nch; c++) {
        S64 acc = 0;
        if (row) {
          const S32* const x = raw[c] + (i0 & (WAV_RAW - 1));
          for (int j = 0; j <= 2 * WAV_DEC_J; j++) acc += (S64)WAV_DEC2[j] * x[j];
        } else {
          for (int j = 0; j <= 2 * WAV_DEC_J; j++) {
            const S64 i = i0 + j;
            acc += (S64)WAV_DEC2[j] * (i >= 0 && (U64)i < rgot ? raw[c][i & (WAV_RAW - 1)] : 0);
          }
        }
        const S64 v = (acc + (1 << 29)) >> 30;  // (|acc| < 2^23 2^31)
        ring[c][n & (WAV_WIN - 1)] = (S32)(v < -(1 << 23) ? -(1 << 23) : v > (1 << 23) - 1 ? (1 << 23) - 1 : v);
      }
      done++;
    }
    got += done;
    return done == k;
  }
  // k more sample frames (at most IO_BUF) into dst's rings (by mask) from sample at on: the
  // probe's first, then the file's; how many came
  U64 read_frames(S32** dst, U64 mask, U64 at, U64 k) {
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
        dst[c][(at + t) & mask] = v;
      }
    }
    return n;
  }
};
