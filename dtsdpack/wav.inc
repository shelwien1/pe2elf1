// wav.inc -- WavIn: the WAV the core is coded against, read as the core goes, a window of its
// samples at a time.

constexpr int WAV_CH = 32;           // its channels, at most
constexpr int WAV_PROBE = 64 * KIB;  // the bytes of its start whose CRC the header keeps
constexpr S64 WAV_WIN = 1 << 20;     // the samples of the window (a channel), a power of 2

// A WAV at 4 times the core's rate, taken to twice it as it's read (WavIn's set_dec()): DTS-HD MA
// codes a 192 kHz stream's audio as two frequency bands at 96 kHz, which its decoder puts back
// together by lifting steps (FFmpeg's assemble_freq_bands(), by ff_dca_xll_band_coeff: XLL_BAND),
// and the encoder's core is its lower band's 64-band analysis's lower half. So the WAV goes back into
// those bands, exactly: its even samples the upper band, its odd ones from the 15th the lower band
// from 7 before its first, the steps undone in reverse (from the stream's start only, as the
// decoder's run), at the stream's own resolution (the samples' common trailing 0 bits: dsh, the
// binding's; a 16-bit stream decoded to 24 bits, 8), and the lower band, shifted back, is the WAV
// at 96 kHz. A block of XLL_B of its samples at a time, from the input's XLL_M either side, as many
// as the steps carry a sample's part across; the lower band saturated at 24 bits, as the encoders'
// is. (FFmpeg's two 192 kHz DTS-HD MA samples' cores 78% and 80% smaller than by the lowpass format
// 32 had taken it for, a sinc at 40 kHz: their indexes the audio's 99.5% of the time, from 83%.)
constexpr int XLL_B = 512, XLL_M = 128, WAV_RAW = 4096;  // (WAV_RAW: the input kept, a power of 2)
static const S32 XLL_BAND[20] = {868669,   -5931642, -1228483, 4194304,  -20577,   122631,  -393647,
                                 904476,   -1696305, 2825313,  -4430736, 6791313,  41153,   -245210,
                                 785564,   -1788164, 3259333,  -5074941, 6928550,  -8204883};
// (FFmpeg's mul22(), mul23(): a times c in 2^-bits, rounded)
static INLINE S64 xll_mul(S64 a, S32 c, int bits) { return (a * c + ((S64)1 << (bits - 1))) >> bits; }

// A PCM WAV (16, 24 or 32-bit; RIFF or RF64), its samples as 24-bit values (16-bit ones shifted
// up, 32-bit ones down: FFmpeg's 32-bit output of a 24-bit decode), a window of them at a time: the
// samples [lo, got) of each channel in rings of WAV_WIN, read as need() asks for later ones; a file
// or a pipe alike; a DTS:X stream's heights folded into it as it's read, where given (heights()) or
// its own (own_heights()).
// Plain data: init() opens it, quit() frees what it holds.
struct WavIn {
  FILE* f;
  int ok;        // a WAV it reads (else the core is coded without one)
  int nch, bps;  // its channels, its bytes a sample
  U32 rate;      // its samples a second
  U64 nsamp;     // its samples a channel (as its header says, or to its end)
  U64 size;      // the file's bytes (0: not known, a pipe)
  U32 crc;       // the CRC-32 of its first WAV_PROBE bytes (or of all, where fewer)
  U32 chmask;    // its channel mask (WAVE_FORMAT_EXTENSIBLE's; 0: none)
  WavIn* hw;     // the heights folded into it as it's read (heights()), else none
  int hc[4];     // (their channels, the WAV's each goes into: -1 none)
  int hself;     // its own heights folded into it instead (own_heights())
  int hs[4];     // (their channels)
  U64 got;       // the samples read so far
  S64 lo;        // the samples before lo are given up
  S32* ring[WAV_CH];
  U8* rb;            // a read's bytes
  U8* probe;         // the first bytes, the samples after the header in them not taken yet
  size_t pn, pat;    // their number, and where the next sample's bytes start
  size_t pat0;       // (where the first sample's start)
  long data_at;      // (the file's place after them, where it can be read again from there: a file's; else -1)
  char why[128];     // why it isn't a WAV it reads
  S32* ext;          // past its end: each channel's continuation, ext_n samples of it (anal.inc's
  int ext_n, ext_on;  // wav_extend()), where ext_on (else 0s)
  // (c: the binding's first pass's analyses of its first blocks, ana_n of them, at the blocks
  // ana_t (in order: 32 up samples a block from ana_off), every channel's 32 bands by ana_fir, to
  // the samples read then, ana_got: analyze() takes them where asked the same, as they are the same)
  S64* ana;
  S64* ana_t;
  int ana_n, ana_up;
  S64 ana_off, ana_got;
  const S64* ana_fir;
  mutable int ext_done[WAV_CH];  // (of it made so far: ext_more() makes it as it is read; -1 not fitted yet)
  mutable S64 ext_q[WAV_CH][65], ext_qa[WAV_CH];  // (the fit: its predictor, Q30, and the sum of |q|; 0s where none)
  void ext_more(int c, S64 k) const;  // (channel c's continuation made to its sample k at least)
  int dec;             // its samples to those read (1; 2: its lower band, set_dec())
  int dsh;             // (the stream's resolution, as the bands go: its samples' 0 bits at the bottom)
  S32* raw[WAV_CH];    // (dec 2: the input's last WAV_RAW samples of each channel, rgot of them
  U64 rgot;            // read, all where rend)
  int rend;
  S32* xb[WAV_CH];     // (and its lower band's block of XLL_B samples from xb0)
  U64 xb0;
  int xbn;             // (how many of them there are: fewer at its end)

  void init(FILE* fp, bool rings = true) {  // (rings: its samples kept in the window; the heights' aren't)
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
    data_at = size ? ftell(f) : -1;
    if (!parse()) return;
    for (int c = 0; rings && c < nch; c++) ring[c] = (S32*)xmalloc(sizeof(S32) * WAV_WIN);
    rb = (U8*)xmalloc((size_t)IO_BUF * nch * bps);
    dec = 1;
    ok = 1;
  }
  // A DTS:X stream's heights, the WAV h's 4 channels (dtssplit's prefix.x.wav: Lh, Rh, Lhr and Rhr,
  // 7.1.4's heights as FFmpeg's newest decoder has them), folded into this WAV's L, R, Lsr and Rsr
  // (FL, FR, BL and BR by its channel mask; without one, an 8-channel WAV's in FFmpeg's 7.1 order)
  // as they're read, each times 23170/32768, rounded at its own resolution (FFmpeg's mul15()), the
  // sum saturated at 24 bits: as the stream's 7.1 bed has them, so a bed with them out (7.1.4's)
  // lines up as the stream's 7.1 decode does (dtssplit's join adds them back the same way); before
  // any sample is read. 0 where they are, else why not
  const char* heights(WavIn* h) {
    static const U32 BIT[4] = {0x1, 0x2, 0x10, 0x20};  // (FL, FR, BL, BR)
    if (!ok || got) return "the WAV isn't used";
    if (!h->ok) return h->why;
    if (h->nch != 4) return "not 4 channels";
    if (h->rate != rate) return "not at the WAV's rate";
    const U32 m = chmask ? chmask : nch == 8 ? 0x63F : 0;
    int any = 0;
    for (int i = 0; i < 4; i++) {
      int w = -1;
      if (m & BIT[i]) {
        w = 0;
        for (U32 b = 1; b < BIT[i]; b <<= 1) w += (m & b) != 0;
        if (w >= nch) w = -1;
      }
      hc[i] = w, any += w >= 0;
    }
    if (!any) return "the WAV has none of L, R, Lsr and Rsr";
    hw = h;
    return nullptr;
  }
  // Where it has no heights WAV (heights()) and its channels are FFmpeg's 7.1.4 (12, by its channel
  // mask: FL, FR, FC, LFE, BL, BR, SL, SR, TFL, TFR, TBL, TBR), its own heights, the last four,
  // folded into its FL, FR, BL and BR the same way: FFmpeg's master branch decodes a DTS:X stream so,
  // its bed less the heights times 23170/32768 (its parse_x_heights(), DCA_XLL_X_HEIGHT_FOLD), at the
  // output's resolution, clipped at 24 bits (where that clipped, the sum is not the bed); before any
  // sample is read. Whether it is
  bool own_heights() {
    if (!ok || got || hw || nch != 12 || chmask != 0x2D63F) return false;
    static const int BED[4] = {0, 1, 4, 5}, TOP[4] = {8, 9, 10, 11};  // (FL, FR, BL, BR; TFL, TFR, TBL, TBR)
    for (int i = 0; i < 4; i++) hc[i] = BED[i], hs[i] = TOP[i];
    hself = 1;
    return true;
  }
  // its samples its lower band's (XLL_BAND) at the resolution sh: its rate and samples halved;
  // before any is read
  void set_dec(int d, int sh) {
    if (!ok || d != 2 || dec == 2 || got) return;
    dec = 2;
    dsh = sh < 0 ? 0 : sh > 15 ? 15 : sh;
    rate /= 2;
    if (nsamp != ~0ULL) nsamp /= 2;
    for (int c = 0; c < nch; c++) raw[c] = (S32*)xmalloc(sizeof(S32) * WAV_RAW), xb[c] = (S32*)xmalloc(sizeof(S32) * XLL_B);
    xbn = 0;
  }
  // the stream's resolution as the WAV's samples have it (compressing, for the binding): their
  // common trailing 0 bits (to 8; 0 where all are 0), of those in the first 64 KB and, where the
  // file can be read on and back, the next 4 MB, and 4 MB more at a time while fewer than 2^16 of
  // them aren't 0, to 256 MB (a quiet start's 0s say nothing: a 16-bit stream's resolution, 8, was
  // 0 where its first 4 MB were silent, its lower band split at another's)
  int low_zeros() {
    if (!ok) return 0;
    const size_t fb = (size_t)nch * bps, more = (size_t)4 << 20;
    U8* buf = (U8*)xmalloc(fb + pn - pat + more);
    size_t n = pn - pat;
    memcpy(buf, probe + pat, n);
    const long at = ftell(f);
    U32 any = 0;
    U64 nz = 0;
    for (int k = 0; k < 64; k++) {
      size_t r = 0;
      if (at >= 0) r = fread(buf + n, 1, more, f), n += r;
      const size_t whole = n - n % fb;  // (the whole sample frames, the rest kept for the next)
      for (size_t i = 0; i + bps <= whole; i += bps) {
        const U8* p = buf + i;
        const U32 v = bps == 2 ? (U32)((U32)p[0] | (U32)p[1] << 8) << 8
                    : bps == 3 ? (U32)p[0] | (U32)p[1] << 8 | (U32)p[2] << 16
                               : ((U32)p[0] | (U32)p[1] << 8 | (U32)p[2] << 16 | (U32)p[3] << 24) >> 8;
        any |= v, nz += (v & 0xFFFFFF) != 0;
      }
      memmove(buf, buf + whole, n - whole);
      n -= whole;
      if (at < 0 || r < more || nz >= 1u << 16) break;
    }
    if (at >= 0 && fseek(f, at, SEEK_SET)) fatal("the WAV can't be read again from where it was");
    free(buf);
    int z = 0;
    while (any && z < 8 && !(any >> z & 1)) z++;
    return any ? z : 0;
  }
  // (c, after the binding's search far into it: call.inc's find_alignment()) back to its first
  // sample, as init() left it, where it can be read again from there
  bool can_rewind() const { return ok && data_at >= 0 && (!hw || hw->data_at >= 0); }
  void rewind() {
    if (fseek(f, data_at, SEEK_SET)) fatal("the WAV can't be read again from its start");
    pat = pat0, got = 0, lo = 0, rgot = 0, rend = 0, xb0 = 0, xbn = 0;
    if (hw && fseek(hw->f, hw->data_at, SEEK_SET)) fatal("the heights WAV can't be read again from its start");
    if (hw) hw->pat = hw->pat0;
  }
  void quit() {
    for (int c = 0; c < WAV_CH; c++)
      free(ring[c]), free(raw[c]), free(xb[c]), ring[c] = raw[c] = xb[c] = nullptr;
    free(rb);
    free(probe);
    free(ext);
    free(ana);
    free(ana_t);
    rb = probe = nullptr;
    ext = nullptr;
    ana = ana_t = nullptr;
    ana_n = 0;
  }

  // sample s of channel c: 0 outside the WAV (past its end, its continuation where made); s in the
  // window need() made
  INLINE S32 at(int c, S64 s) const {
    if (s < 0 || c >= nch) return 0;
    if ((U64)s >= got) {
      const U64 k = (U64)s - nsamp;
      if (!(ext_on && got == nsamp && k < (U64)ext_n)) return 0;
      if ((S64)k >= ext_done[c]) ext_more(c, (S64)k);
      return ext[(size_t)c * ext_n + (size_t)k];
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
        if (k + (i2 - i1) > ext_done[c]) ext_more(c, k + (i2 - i1) - 1);
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
        if (fmt == 0xFFFE && sz >= 40 && i + 8 + 26 <= n)  // (the subformat's, and the channel mask)
          fmt = (int)le16(i + 8 + 24), chmask = le32(i + 8 + 20);
      }
      if (!memcmp(h + i, "data", 4)) {
        if (fmt != 1 || nch < 1 || nch > WAV_CH || (bits != 16 && bits != 24 && bits != 32))
          return snprintf(why, sizeof why, "not a WAV of 16, 24 or 32-bit PCM, 1 to %d channels", WAV_CH), false;
        bps = bits / 8;
        pat = pat0 = i + 8;
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
      if (n >= xb0 + (U64)xbn || n < xb0) {  // (the next block: the input it takes read first)
        const U64 n0 = n - n % XLL_B;
        const S64 hi = 2 * ((S64)n0 + XLL_B + XLL_M) + 16;  // (past the last input sample it takes)
        if (!rend && (S64)rgot < hi) {  // (WAV_RAW / 2 at a time: the oldest it takes kept)
          const U64 r = read_frames(raw, WAV_RAW - 1, rgot, WAV_RAW / 2);
          rgot += r;
          rend = r < (U64)WAV_RAW / 2;
          continue;
        }
        if (rend && n >= rgot / 2) break;  // (its end: the input's half)
        xll_block(n0);
      }
      for (int c = 0; c < nch; c++) ring[c][n & (WAV_WIN - 1)] = xb[c][n - xb0];
      done++;
    }
    got += done;
    return done == k;
  }
  // the lower band's samples n0 to n0 + XLL_B into xb: the input's samples from 2 (n0 - XLL_M) on,
  // a[g] (the lower band) its sample 2 g + 15, b[g] (the upper) its sample 2 g, at the resolution;
  // the decoder's steps undone, last first, for g from 0 (the stream's first sample) in the window
  void xll_block(U64 n0) {
    constexpr int W = XLL_B + 2 * XLL_M;
    static S64 a[W + 8], b[W];  // (a from g0 - 8, b from g0)
    const S64 g0 = (S64)n0 - XLL_M;
    const int k0 = g0 < 0 ? (int)-g0 : 0;  // (the steps from g 0 on)
    const S32* const C = XLL_BAND;
    for (int c = 0; c < nch; c++) {
      auto in = [&](S64 i) -> S64 { return i >= 0 && (U64)i < rgot ? (S64)(raw[c][i & (WAV_RAW - 1)] >> dsh) : 0; };
      for (int k = 0; k < W + 8; k++) a[k] = in(2 * (g0 - 8 + k) + 15);
      for (int k = 0; k < W; k++) b[k] = in(2 * (g0 + k));
      S64* const A = a + 8;  // (A[k]: a at g0 + k)
      for (int i = 7; i >= 0; i--) {
        for (int k = k0; k < W; k++) A[k - i] += xll_mul(b[k], C[4 + i], 23);
        for (int k = k0; k < W; k++) b[k] += xll_mul(A[k - i], C[12 + i], 23);
        for (int k = k0; k < W; k++) A[k - i] += xll_mul(b[k], C[4 + i], 23);
      }
      for (int k = k0; k < W; k++) {
        b[k] += xll_mul(A[k], C[3], 22), A[k] += xll_mul(b[k], C[2], 22);
        b[k] += xll_mul(A[k], C[1], 22), A[k] += xll_mul(b[k], C[0], 22);
      }
      for (int k = 0; k < XLL_B; k++) {
        const S64 v = A[XLL_M + k] * ((S64)1 << dsh);  // (in 24 bits, as the encoders' is: theirs saturates there)
        xb[c][k] = (S32)(v < -(1 << 23) ? -(1 << 23) : v > (1 << 23) - 1 ? (1 << 23) - 1 : v);
      }
    }
    xb0 = n0;
    xbn = XLL_B;
  }
  // k more sample frames' bytes (at most IO_BUF) into rb: the probe's first, then the file's; how
  // many came whole
  U64 fill(U64 k) {
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
    return have / ((size_t)nch * bps);
  }
  // a sample's bytes at p as a 24-bit value
  INLINE S32 value(const U8* p) const {
    if (bps == 2) return (S32)(S16)((U32)p[0] | (U32)p[1] << 8) * 256;
    if (bps == 3) return (S32)((U32)p[0] << 8 | (U32)p[1] << 16 | (U32)p[2] << 24) >> 8;
    return (S32)((U32)p[0] | (U32)p[1] << 8 | (U32)p[2] << 16 | (U32)p[3] << 24) >> 8;
  }
  // k more sample frames (at most IO_BUF) into dst's rings (by mask) from sample at on, the heights
  // folded in (heights()); how many came
  U64 read_frames(S32** dst, U64 mask, U64 at, U64 k) {
    const U64 n = fill(k);
    for (U64 t = 0; t < n; t++) {
      const U8* p = rb + (size_t)t * nch * bps;
      for (int c = 0; c < nch; c++, p += bps) dst[c][(at + t) & mask] = value(p);
    }
    if (hw) {  // (the heights of those frames, as many as there are)
      const U64 hn = hw->fill(n);
      const int hb = hw->bps;
      for (U64 t = 0; t < hn; t++) {
        const U8* p = hw->rb + (size_t)t * 4 * hb;
        for (int i = 0; i < 4; i++, p += hb) {
          if (hc[i] < 0) continue;
          const S32 h = hw->value(p);
          dst[hc[i]][(at + t) & mask] = fold(dst[hc[i]][(at + t) & mask], h, hb);
        }
      }
    }
    if (hself)  // (its own heights)
      for (U64 t = 0; t < n; t++)
        for (int i = 0; i < 4; i++)
          dst[hc[i]][(at + t) & mask] = fold(dst[hc[i]][(at + t) & mask], dst[hs[i]][(at + t) & mask], bps);
    return n;
  }
  // the bed's value v with the height h of a WAV of b bytes a sample in it: h times 23170/32768,
  // rounded at that WAV's resolution, the sum saturated at 24 bits
  static INLINE S32 fold(S32 v, S32 h, int b) {
    const S64 x = v + (b == 2 ? xll_mul(h >> 8, 23170, 15) * 256 : xll_mul(h, 23170, 15));
    return (S32)(x < -(1 << 23) ? -(1 << 23) : x > (1 << 23) - 1 ? (1 << 23) - 1 : x);
  }
};
