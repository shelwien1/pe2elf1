// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as m4vpack's and
// ac3pack's): a run, on the buffers of its input and output, at a level.

// The stream: "dtsdpack", the format's version, the model's level; then what the WAV was and how
// the core lines up with it (bind_put()), in bytes of their own lengths; then the arithmetic coded
// data: the chunks (kind, then the records of a frame, or a length and the bytes of bytes kept as
// bytes), the end, and the CRC-32 of the stream (32 bits at 1/2 each).
enum { DP_MAGIC = 8, DP_AT_VERSION = DP_MAGIC, DP_AT_LEVEL, DP_AT_BIND };  // the header's fixed part
enum { DP_BIND_MAX = 3 + 10 + 4 + 5 + 3 + 12 * NCH };                     // (and the binding's bytes at most)
static const U8 magic[DP_MAGIC] = {'d', 't', 's', 'd', 'p', 'a', 'c', 'k'};
constexpr int DP_VERSION = 93;  // the format's (2: a core channel's binding a mix of two WAV channels; 3: the XCH
                               // extension as records; 4: a WAV at twice the core's rate, X96 and XXCH; 5: the
                               // scale factor search; 6: the bit allocation search; 7: the bits after the
                               // subframes as the bytes they end in, the energy's quarter of its bucket, the
                               // HF VQ groups by their nearest entries where one is near; 8: the transient
                               // and the ADPCM predictor searches; 9: a VQ subband's scale factor search to
                               // 12 above, by the entry most nearly a multiple; 10: the bits after the
                               // subframes by their bits to a 32-bit boundary, abt tuned again; 11: the
                               // LFE's fit with its last 7 values; 12: the bit allocations' staircases and
                               // the rate control's level, abt tuned again; 13: the ADPCM's history the
                               // decoder's, as the encoder's closed loop has it; 14: the transients' rule,
                               // quiet subbands' PMODE, the ranks' margins from 1/256; 15: the HF VQ
                               // groups' centroids from their entries at unit energy; 16: the level's
                               // shift dearer from the rate control's centre, scale factors by channel
                               // class and the band below's energy; 17: a lossless WAV's index misses
                               // against the channel's level, auxiliary data's bytes from its sync word;
                               // 18: the staircases' energies in 128ths of a doubling, their margins
                               // finer; 19: the level's centre from the frame's bits, an ADPCM
                               // subband's staircase the subband's, the staircases at the subframe's
                               // last level, the VQ subbands' scale factors with noise; 20: a
                               // lossless WAV's top subband less 2^-20 of its bottom one, the
                               // noise gone; 21: and the next subband, 30, with subband 1's; the
                               // LFE's fit by the values' steps, forgetting faster; 22: the tails
                               // that repeat one of the last 32, a lossless WAV's mixes in the PCM;
                               // 23: the HF VQ groups by learned scores, the subband 30-31 leaks
                               // at the core's rate only; 24: the bits before the extension's sync
                               // word as an earlier frame's bytes, vqr tuned again; 25: a lossless
                               // WAV's 64-band analysis at the encoders' gain, the bit allocation
                               // search 4 either side, scl and abt tuned again; 26: the top
                               // subband's RMS with the leak's noise, the HF VQ scores not learning
                               // where it swamps the values; 27: hit and sid tuned again; 28: raw
                               // and hdr tuned again; 29: the ADPCM predictor as its block's rank
                               // by the encoders' codewords, then its rank in the block; vqr
                               // tuned again; 30: the ABITS against what the subframe's bits
                               // leave, abt, sid, scl and hit tuned again; 31: the ABITS by the
                               // lossless WAVs' encoders' allocation table, a transient's by its
                               // values from the transient on, TMODE against what the ABITS
                               // said, the WAV's continuation past its end; abt, sid, scl, hit
                               // and vqr tuned again; 32: a WAV at 4 times the core's rate,
                               // decimated by 2; 33: an index that misses the audio's by a few
                               // as its distance from it; 34: the LFE's step exact, a decoded
                               // WAV's rounded as its decoder has it; 35: X96's joint scales
                               // without the audio's against their neighbours, the ABITS
                               // search's against what the staircase or table says, an index
                               // whose window lies far past the WAV's end without the audio; 36:
                               // an index against how many of its channel's last 32 missed; 37:
                               // smp, abt, sid and hit tuned again; 38: where X96 codes the core's
                               // subbands too, the core's index as X96's lattice leaves it; X96's
                               // joint scales by their running mean; 39: a decoded WAV's bottom
                               // subband through a fit of its filter; 40: where X96 codes the core's
                               // subbands too, their scale factors by its lattice; 41: there, more
                               // scale factors tried, around the last one where the gain is low;
                               // X96's subbands without bits by FFmpeg's noise generator; 42: X96's
                               // bit allocation search, its VQ vectors' searches as the core's; 43:
                               // the bit allocation search's equals the coarser lattice, and a
                               // transient's two scale factors; 44: of the ADPCM predictors that fit,
                               // the nearest; 45: PMODE against a transient's fit too; a decoded
                               // WAV's bottom subband's fit by the decode's next values too; X96's
                               // predicting VQ subbands' vectors less the prediction; 46: the
                               // core's ADPCM predictors where X96 codes its subbands too, by its
                               // values, the decode less X96's lattice's; X96's by the ones that
                               // leave the decode a vector or on a lattice exactly; 47: abt, scl,
                               // hit, sid and smp retuned, a part of a pass; the bits after the
                               // subframes by how the last ones ended; 48: X96's code books after
                               // its subframes, its joint scale factors' after them, against the
                               // cheapest for their values; 49: the core's PMODE and scale factor
                               // by its values where X96 codes its subbands too; X96's PMODE where
                               // the decode fits as it is; vqr, hdr and raw retuned; 50: those
                               // values where X96 codes by its vectors, the decode less a nearby
                               // vector; more predictors tried where few fit; 51: elsewhere there,
                               // a fit of them on the decode's, for their predictor, scale factor
                               // search, PMODE and indexes; 52: a tail as a cached one but for a run
                               // of 0s; X96's predictors that fit, to 32 first, and the vectors at
                               // scale factor 0; 53: an index's nearness to its rounding boundary
                               // against the level times its subband's error's scale, learned; vqr,
                               // scl, sid and hit retuned; 54: a lossless WAV's scale factors' too,
                               // against the level and the RMS, and its HF VQ vectors' margin against
                               // the level; 55: those as contexts of their own, the VQ subbands' and
                               // groups' too; scl and vqr retuned; 56: the groups' margin by their
                               // scores' scale, the vectors' near ties; scl, vqr, sid, abt, smp and
                               // hit retuned; 57: hdr, raw, abt, vqr and scl retuned; 58: where the
                               // scale factor's run of equal entries ends, the level nearness to its
                               // value; an HF VQ miss's rank buckets by the candidates' gaps; scl and
                               // vqr retuned; 59: the HF VQ values scaled into their scale factor's
                               // region, a missed group's vectors by the values moved onto the scores'
                               // boundary, the groups' near ties, the top subband's pl as its rule's;
                               // vqr and scl retuned; 60: the HF VQ misses by their boundaries'
                               // distance, the top subband's scale factor nearness with its leak's
                               // noise, PMODE and TMODE by the rise against the transients'
                               // threshold; vqr, scl and sid retuned; 61: the HF VQ margins in the
                               // boundary order's terms, the top subband's unit with its leak's
                               // noise; vqr retuned; 62: the transients' threshold by the subband,
                               // 8 (19 + b) / 19, for PMODE's and TMODE's rise and the transients'
                               // ABITS; a quiet subband by its RMS under 100; no prediction in the
                               // HF VQ subbands; 63: the transients' ABITS and TMODE's class by
                               // that threshold alone, no second guess from the other mean square;
                               // every family retuned; 64: the LFE by the DTS-HD MA encoders' filter,
                               // fixed, and a third fit forgetting slowly, by the decimations by 32;
                               // sid and abt retuned; 65: the bits after the subframes and before
                               // the extension as the encoders' ring of 8 frame buffers leaves them;
                               // XGAP by where the subframes end; 66: FFmpeg's decodes' LFE filter,
                               // fixed, a fifth predictor; every family retuned; 67: a 192 kHz
                               // WAV's lower band as DTS-HD MA's XLL splits it, at the binding's
                               // resolution, in place of a lowpass; 68: a sixth LFE predictor,
                               // the slow fit at each place of a frame's values apart; 69: the
                               // index misses' error scales from -9, the HF VQ groups' scores
                               // not learned from a vector past the code book's reach; 70: the
                               // least squares fits' LDL' by dot products rounded once, their
                               // samples by the step's reciprocal; the WAV's continuation fitted
                               // to its samples at 25 bits; 71: a sample's hit flag by the cells
                               // either side of its nearnesses, their rates' geometric mean; 72:
                               // each cell's rate at two speeds, the error scales' steps surer
                               // after 16; 73: the faster at 16 flags, surer after 8 steps; 74:
                               // the hit cells' classes of the recent misses, the HF VQ near ties
                               // and region, the scale factors' nearness by 2^-25 of the level;
                               // 75: scl and hit retuned; 76: smp and vqr retuned; 77: the
                               // allocation table's offsets one for all the bands, by its shape;
                               // 78: its guesses first from the start, the level's shift wider,
                               // abt's sx (its guess against the subframe's levels in quarters);
                               // abt, hdr and raw retuned; 79: sx in six classes, abt retuned; the
                               // index misses' error scales from the lossless WAVs' at the core's rate;
                               // 80: an index's hit cells by the earlier channels' misses at its sample;
                               // 81: scl and smp retuned; 82: a lossless WAV's indexes with a bias
                               // of the bottom subband's value, their nearness and the HF VQ
                               // margins against the level along the subband's FFT path; 83: a
                               // lossless WAV's top subband's HF VQ groups and vectors by each
                               // value weighed against the FFT's noise there, by the bottom one's;
                               // 84: their boundary orders' distances, and the missed groups' push,
                               // by that noise, its power in the RMS rule 3/4 of it, its scale
                               // factor's nearness by its power weighed by the values'; the HF VQ
                               // boundary orders' keys the largest against the nearer ones; 85:
                               // that noise by the binade of the bottom subband's value, a missed
                               // group's push to the group that binds, the nearness's noise with
                               // its own square; 86: vqr and scl retuned; 87: the FFT path's
                               // level with bin 0's binade in the even bins' stages, subband 28's
                               // HF VQ boundary orders by that noise, the scale factors' nearness
                               // by the path's level weighed by the values' power; the top
                               // subband's HF VQ values by the noise's mean given the scale
                               // factor's region, a group's push by its mean given the group, the
                               // boundary orders by its variance left; scl and vqr retuned; 88:
                               // subbands 28-30's HF VQ values by that mean too, behind the
                               // switch, a second group bound for the top subband's push; 89: a
                               // twin's (a channel bound as an earlier one, or in a subframe
                               // analysed as one) side fields and indexes by the earlier one's;
                               // 90: the header's binding in bytes of their own lengths; 91: a
                               // lossless WAV's scale factors the RMS rule isn't sure of by the
                               // FFT's noise's posterior over the table's cells, sf_post()); 92:
                               // that noise in the ADPCM subbands by their index error scales;
                               // 93: a lossless WAV's HF VQ groups and entries by a Monte Carlo
                               // posterior of the encoder's noisy values too, hf_mc(); scl retuned

static void put_le(U8* p, U64 v, int n) {
  for (int i = 0; i < n; i++) p[i] = (U8)(v >> (8 * i));
}
// a number in 7-bit groups, the lowest first, each but the last with its top bit set (put_uv(): its
// bytes); a signed one as twice its magnitude, less 1 where negative (zz())
static int put_uv(U8* p, U64 v) {
  int n = 0;
  for (; v > 127; v >>= 7) p[n++] = (U8)(v | 128);
  p[n++] = (U8)v;
  return n;
}
static U64 zz(S64 v) { return v < 0 ? (U64)(-(v + 1)) << 1 | 1 : (U64)v << 1; }
static S64 unzz(U64 u) { return u & 1 ? -(S64)(u >> 1) - 1 : (S64)(u >> 1); }
// a binding's gain (Q23) as a number: 0 for 0; the downmix table's entries, from 1 down, each then
// its negative, 1-482; any other, 483 on (zz())
static U64 gain_code(S32 g) {
  const U64 a = g < 0 ? (U64)(-(S64)g) : (U64)g;
  if (!g) return 0;
  if (!(a & 255))
    for (int k = 241; k >= 1; k--)
      if (DMIX[k] == a >> 8) return 1 + 2 * (U64)(241 - k) + (g < 0);
  return 483 + zz(g);
}
static bool gain_uncode(U64 u, S32& g) {
  if (u < 483) {
    const S32 m = u ? (S32)DMIX[241 - (int)((u - 1) >> 1)] << 8 : 0;
    g = (u - 1) & 1 ? -m : m;
    return true;
  }
  const S64 x = unzz(u - 483);
  if (x < INT32_MIN || x > INT32_MAX) return false;
  g = (S32)x;
  return true;
}
// What the WAV was (whether it was one, its channels, its bytes a sample, a byte each; its size; the
// CRC-32 of its first 64 KB, 4 bytes) and the binding (the offset, signed; the WAV's samples to the
// core's, 1, 2 or 4: its lower band, then 2, and in the upper 4 bits the lower band's resolution, a
// byte; the LFE's channel, plus 1: 0 none; how many core channels follow, a byte, to the last
// bound one; each one's WAV channel, plus 1, plus 128 where it is that channel as it is, else where
// bound, the second of its mix, plus 1, and the gains, gain_code()'s, the second's where it has
// one), into p: its bytes (DP_BIND_MAX at most)
static int bind_put(U8* p, const Bind& b, const WavIn& w) {
  int n = 0, nb = 0;
  p[n++] = (U8)w.ok;
  p[n++] = (U8)w.nch;
  p[n++] = (U8)w.bps;
  n += put_uv(p + n, w.size);
  put_le(p + n, w.crc, 4);
  n += 4;
  n += put_uv(p + n, zz(b.off));
  p[n++] = (U8)(b.up * b.dec + 16 * b.dsh);
  p[n++] = (U8)(b.lfe + 1);
  for (int ch = 0; ch < NCH; ch++)
    if (b.map[ch] >= 0) nb = ch + 1;
  p[n++] = (U8)nb;
  for (int ch = 0; ch < nb; ch++) {
    const int plain = b.map[ch] >= 0 && b.map[ch] < 127 && b.map2[ch] < 0 && b.g1[ch] == BIND_ONE;
    p[n++] = (U8)(b.map[ch] + 1 + 128 * plain);
    if (b.map[ch] < 0 || plain) continue;
    p[n++] = (U8)(b.map2[ch] + 1);
    n += put_uv(p + n, gain_code(b.g1[ch]));
    if (b.map2[ch] >= 0) n += put_uv(p + n, gain_code(b.g2[ch]));
  }
  return n;
}
// what the WAV was (BindWav) and the binding, back from the bytes get() gives (-1: none left): whether
// they are whole and in range (the channels not there unbound)
struct BindWav {
  U8 ok, nch, bps;
  U64 size;
  U32 crc;
};
template <class G>
static bool bind_read(G get, Bind& b, BindWav& w) {
  U64 u = 0;
  int c[3];
  auto uv = [&](U64& v) {  // (at most 10 bytes, the last with the 64th bit at most)
    v = 0;
    for (int s = 0; s < 70; s += 7) {
      const int x = get();
      if (x < 0 || (s == 63 && x > 1)) return false;
      v |= (U64)(x & 127) << s;
      if (!(x & 128)) return true;
    }
    return false;
  };
  auto s32 = [&](S32& v) {
    if (!uv(u)) return false;
    const S64 x = unzz(u);
    if (x < INT32_MIN || x > INT32_MAX) return false;
    v = (S32)x;
    return true;
  };
  b.none();
  for (int i = 0; i < 3; i++)
    if ((c[i] = get()) < 0) return false;
  w.ok = (U8)c[0], w.nch = (U8)c[1], w.bps = (U8)c[2];
  if (!uv(w.size)) return false;
  w.crc = 0;
  for (int i = 0; i < 4; i++) {
    const int x = get();
    if (x < 0) return false;
    w.crc |= (U32)x << (8 * i);
  }
  if (!s32(b.off)) return false;
  const int up = get(), lfe = get(), nb = get();
  if (up < 0 || lfe < 0 || lfe > 128 || nb < 0 || nb > NCH) return false;
  b.up = (up & 15) == 2 || (up & 15) == 4 ? 2 : 1;
  b.dec = (up & 15) == 4 ? 2 : 1;
  b.dsh = (S8)(up >> 4);
  b.lfe = (S8)(lfe - 1);
  for (int ch = 0; ch < nb; ch++) {
    const int m = get();
    if (m < 0) return false;
    if (!m) continue;
    if (m > 128) {  // (one WAV channel as it is)
      b.map[ch] = (S8)(m - 129);
      continue;
    }
    const int m2 = get();
    if (m2 < 0 || m2 > 128) return false;
    b.map[ch] = (S8)(m - 1), b.map2[ch] = (S8)(m2 - 1);
    if (!uv(u) || !gain_uncode(u, b.g1[ch])) return false;
    if (m2 && (!uv(u) || !gain_uncode(u, b.g2[ch]))) return false;
  }
  return true;
}
// the WAV as it was (got) and as it is (w): the same?
static const char* bind_differs(const BindWav& got, const WavIn& w) {
  if (got.ok != (U8)w.ok || got.nch != (U8)w.nch || got.bps != (U8)w.bps) return "its format";
  if (got.crc != w.crc) return "its first 64 KB";
  if (got.size && w.size && got.size != w.size) return "its size";
  return 0;
}

// a compression's statistics (its summary line)
struct Stats {
  U64 frames, packed, raw_parse, raw_verify;
  double in_bytes, raw_bytes;
  U64 hits, smp, sf_hits, sf;  // (-s: the samples and scale factors the audio predicted)
  U64 vq[VK_N], vq_hits[VK_N];  // (and the code books' indexes, those its searches found)
  U64 pv, pv_hits;              // (the ADPCM predictors, those found: their blocks' and entries' ranks 0)
  U64 hits9, smp9, sf_hits9, sf9;  // (and X96's samples and scale factors)
  U64 ab9, ab_srch9, ab_hits9;     // (and its ABITS, those its bit allocation search said, those it said right)
  U64 ab, ab_srch, ab_hits;        // (and the ABITS, those the bit allocation search said, those it said right)
  U64 ab_st, ab_sthits;            // (those a staircase said, those it said right)
  U64 ab_tb, ab_tbhits;            // (those the allocation table said, those it said right)

  // c's summary line (out_bytes: the compressed size, at the level), and with -s the bytes by kind
  // of syntax element, the coder's cost and orig (CAT_*)
  void report(const char* name, rp<const double> cost, rp<const double> orig, double out_bytes, int level, int stats,
              const char* bound) const {
    static const char* const cat_name[NCAT] = {"bytes kept as bytes", "headers, container", "side information",
                                               "bit allocations", "scale factors", "subband samples",
                                               "bits after the subframes", "X96's subbands"};
    int i;
    fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%, -%d); %llu frames, %llu packed", name, in_bytes, out_bytes,
            in_bytes ? PERCENT * out_bytes / in_bytes : 0.0, level, (unsigned long long)frames, (unsigned long long)packed);
    if (frames > packed)
      fprintf(stderr, ", %llu kept as bytes (%llu unparsable, %llu not reproduced)", (unsigned long long)(frames - packed),
              (unsigned long long)raw_parse, (unsigned long long)raw_verify);
    fprintf(stderr, "; %.0f bytes between frames; %s\n", raw_bytes, bound);
    if (stats) {
      double o = 0, p = 0;
      fprintf(stderr, "  %-28s %10s %10s\n", "bytes", "input", "packed");
      for (i = 0; i < NCAT; i++) {
        if (i == CAT_X96 && !orig[i]) continue;  // (only where there is some)
        fprintf(stderr, "  %-28s %10.0f %10.0f %7.2f%%\n", cat_name[i], orig[i] / BYTE_BITS, cost[i] / BYTE_BITS,
                orig[i] ? PERCENT * cost[i] / orig[i] : 0.0);
        o += orig[i];
        p += cost[i];
      }
      fprintf(stderr, "  %-28s %10.0f %10.0f %7.2f%%\n", "total", o / BYTE_BITS, p / BYTE_BITS, o ? PERCENT * p / o : 0.0);
      if (smp) fprintf(stderr, "  the audio's indexes: %.2f%% of the samples; the audio's scale factors: %.2f%%\n",
                       PERCENT * hits / smp, sf ? PERCENT * sf_hits / sf : 0.0);
      if (ab_srch)
        fprintf(stderr, "  the bit allocation search: %.2f%% of the ABITS, right in %.2f%% of them\n",
                PERCENT * ab_srch / ab, PERCENT * ab_hits / ab_srch);
      if (ab_st)
        fprintf(stderr, "  the bit allocations' staircases: %.2f%% of the ABITS, right in %.2f%% of them\n",
                PERCENT * ab_st / ab, PERCENT * ab_sthits / ab_st);
      if (ab_tb)
        fprintf(stderr, "  the encoders' allocation table: %.2f%% of the ABITS, right in %.2f%% of them\n",
                PERCENT * ab_tb / ab, PERCENT * ab_tbhits / ab_tb);
      if (smp9 || sf9)
        fprintf(stderr, "  X96: the audio's indexes: %.2f%% of the samples; the audio's scale factors: %.2f%%\n",
                smp9 ? PERCENT * hits9 / smp9 : 0.0, sf9 ? PERCENT * sf_hits9 / sf9 : 0.0);
      if (ab_srch9)
        fprintf(stderr, "  X96's bit allocation search: %.2f%% of its ABITS, right in %.2f%% of them\n",
                PERCENT * ab_srch9 / ab9, PERCENT * ab_hits9 / ab_srch9);
      if (pv || vq[VK_HFG])
        fprintf(stderr, "  the encoder's searches on the audio: %.2f%% of the ADPCM predictors; HF VQ: %.2f%% of the "
                        "groups, %.2f%% of the vectors in them\n",
                pv ? PERCENT * pv_hits / pv : 0.0,
                vq[VK_HFG] ? PERCENT * vq_hits[VK_HFG] / vq[VK_HFG] : 0.0,
                vq[VK_HFV] ? PERCENT * vq_hits[VK_HFV] / vq[VK_HFV] : 0.0);
    }
  }
};

#include "dtsdcoro.inc"  // DtsCoro: a run of c or d

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them. coro_new() makes it (plain data: then Init()), coro_free() frees it after its Quit().
template <class T>
T* coro_new(void) {
  return (T*)map_pages(sizeof(T));
}
template <class T>
void coro_free(T* p) {
  unmap_pages(p, sizeof(T));
}

// The output of a call: a file, or (c -v) the input to compare with
struct Sink {
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs, err;

  void write(pcU8 p, size_t n) {
    static U8 cmp[IO_BUF];  // (the main thread's)
    if (!compare) {
      if (n && fwrite(p, 1, n, f) != n) err = 1;
      pos += n;
      return;
    }
    while (n && !differs) {
      const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
      const size_t got = fread(cmp, 1, k, f);
      size_t i;
      for (i = 0; i < got && cmp[i] == p[i]; i++) {}
      if (i < k) {
        differs = 1;
        diff_at = pos + i;
        return;
      }
      pos += k;
      p += k;
      n -= k;
    }
  }
};

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns);
// show(code) after each yield (what it tells, Y_AUDIO; the progress line)
template <class C, class In, class Out, class Show>
uint drive(C& c, In in, Out out, Show show) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) in();
    else if (r != Y_AUDIO) out(r == Y_OUT);
    show(r);
    if (r != Y_IN && r != Y_OUT && r != Y_AUDIO) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s, and
// show(code) after each yield
static U8 io_bufs[2][IO_BUF];  // (the main thread's: input, output)
template <class C, class Show>
uint run_file(C& c, FILE* f, Sink& s, Show show, pcU8 pre = 0, size_t n = 0) {
  const uint kIo = sizeof io_bufs[0];
  U8 *ib = io_bufs[0], *ob = io_bufs[1];
  c.addout(ob, kIo);
  if (n) c.addinp((byte*)pre, (uint)n);
  return drive(
      c,
      [&]() {
        const uint l = (uint)fread(ib, 1, kIo, f);
        if (!l) c.f_quit = 1;
        c.addinp(ib, l);
      },
      [&](bool more) {
        s.write(c.outbeg, c.getoutsize());
        if (more) c.addout(ob, kIo);
      },
      show);
}
