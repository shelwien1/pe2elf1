// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as m4vpack's and
// ac3pack's): a run, on the buffers of its input and output, at a level.

// The stream: "dtsdpack", the format's version, the model's level; the WAV's binding (bind_put()):
// how the core lines up with it (the offset, 4 bytes; each channel's WAV channel, 8 bytes, the
// second WAV channel of its mix, 8, and their gains, 8 and 8 of 4 bytes; the LFE's channel, 1; the
// WAV's samples to the core's, 1),
// what the WAV was (whether it was one, its channels, its bytes a sample, its size, 8 bytes, and
// the CRC-32 of its first 64 KB, 4); then the arithmetic coded data: the chunks (kind, then the
// records of a frame, or a length and the bytes of bytes kept as bytes), the end, and the CRC-32
// of the stream (32 bits at 1/2 each).
enum { BO_OFF = 0, BO_MAP = 4, BO_MAP2 = BO_MAP + NCH, BO_G1 = BO_MAP2 + NCH, BO_G2 = BO_G1 + 4 * NCH,
       BO_LFE = BO_G2 + 4 * NCH, BO_UP, BO_OK, BO_NCH, BO_BPS, BO_SIZE, BO_CRC = BO_SIZE + 8,
       DP_BIND = BO_CRC + 4 };  // (the binding's bytes)
enum { DP_MAGIC = 8, DP_AT_VERSION = DP_MAGIC, DP_AT_LEVEL, DP_AT_BIND, DP_HEAD = DP_AT_BIND + DP_BIND };  // the header's
static const U8 magic[DP_MAGIC] = {'d', 't', 's', 'd', 'p', 'a', 'c', 'k'};
constexpr int DP_VERSION = 13;  // the format's (2: a core channel's binding a mix of two WAV channels; 3: the XCH
                               // extension as records; 4: a WAV at twice the core's rate, X96 and XXCH; 5: the
                               // scale factor search; 6: the bit allocation search; 7: the bits after the
                               // subframes as the bytes they end in, the energy's quarter of its bucket, the
                               // HF VQ groups by their nearest entries where one is near; 8: the transient
                               // and the ADPCM predictor searches; 9: a VQ subband's scale factor search to
                               // 12 above, by the entry most nearly a multiple; 10: the bits after the
                               // subframes by their bits to a 32-bit boundary, abt tuned again; 11: the
                               // LFE's fit with its last 7 values; 12: the bit allocations' staircases and
                               // the rate control's level, abt tuned again; 13: the ADPCM's history the
                               // decoder's, as the encoder's closed loop has it)

static void put_le(U8* p, U64 v, int n) {
  for (int i = 0; i < n; i++) p[i] = (U8)(v >> (8 * i));
}
static U64 get_le(const U8* p, int n) {
  U64 v = 0;
  for (int i = 0; i < n; i++) v |= (U64)p[i] << (8 * i);
  return v;
}
// the binding into p (DP_BIND bytes), and back: the WAV as it was (got) and as it is (w), the same?
static void bind_put(U8* p, const Bind& b, const WavIn& w) {
  put_le(p + BO_OFF, (U32)b.off, 4);
  for (int ch = 0; ch < NCH; ch++) {
    p[BO_MAP + ch] = (U8)b.map[ch];
    p[BO_MAP2 + ch] = (U8)b.map2[ch];
    put_le(p + BO_G1 + 4 * ch, (U32)b.g1[ch], 4);
    put_le(p + BO_G2 + 4 * ch, (U32)b.g2[ch], 4);
  }
  p[BO_LFE] = (U8)b.lfe;
  p[BO_UP] = (U8)b.up;
  p[BO_OK] = (U8)w.ok;
  p[BO_NCH] = (U8)w.nch;
  p[BO_BPS] = (U8)w.bps;
  put_le(p + BO_SIZE, w.size, 8);
  put_le(p + BO_CRC, w.crc, 4);
}
static Bind bind_get(const U8* p) {
  Bind b;
  b.off = (S32)(U32)get_le(p + BO_OFF, 4);
  for (int ch = 0; ch < NCH; ch++) {
    b.map[ch] = (S8)p[BO_MAP + ch];
    b.map2[ch] = (S8)p[BO_MAP2 + ch];
    b.g1[ch] = (S32)(U32)get_le(p + BO_G1 + 4 * ch, 4);
    b.g2[ch] = (S32)(U32)get_le(p + BO_G2 + 4 * ch, 4);
  }
  b.lfe = (S8)p[BO_LFE];
  b.up = p[BO_UP] == 2 ? 2 : 1;
  return b;
}
static const char* bind_differs(const U8* p, const WavIn& w) {
  if (p[BO_OK] != (U8)w.ok || p[BO_NCH] != (U8)w.nch || p[BO_BPS] != (U8)w.bps) return "its format";
  if (get_le(p + BO_CRC, 4) != w.crc) return "its first 64 KB";
  const U64 sz = get_le(p + BO_SIZE, 8);
  if (sz && w.size && sz != w.size) return "its size";
  return 0;
}

// a compression's statistics (its summary line)
struct Stats {
  U64 frames, packed, raw_parse, raw_verify;
  double in_bytes, raw_bytes;
  U64 hits, smp, sf_hits, sf;  // (-s: the samples and scale factors the audio predicted)
  U64 vq[VK_N], vq_hits[VK_N];  // (and the code books' indexes, those its searches found)
  U64 hits9, smp9, sf_hits9, sf9;  // (and X96's samples and scale factors)
  U64 ab, ab_srch, ab_hits;        // (and the ABITS, those the bit allocation search said, those it said right)
  U64 ab_st, ab_sthits;            // (those a staircase said, those it said right)

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
      if (smp9 || sf9)
        fprintf(stderr, "  X96: the audio's indexes: %.2f%% of the samples; the audio's scale factors: %.2f%%\n",
                smp9 ? PERCENT * hits9 / smp9 : 0.0, sf9 ? PERCENT * sf_hits9 / sf9 : 0.0);
      if (vq[VK_PVQ] || vq[VK_HFG])
        fprintf(stderr, "  the encoder's searches on the audio: %.2f%% of the ADPCM predictors; HF VQ: %.2f%% of the "
                        "groups, %.2f%% of the vectors in them\n",
                vq[VK_PVQ] ? PERCENT * vq_hits[VK_PVQ] / vq[VK_PVQ] : 0.0,
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
