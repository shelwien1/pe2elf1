// call.inc -- dtsdpack.cpp's Call: a call's files (the stream, the WAV, the output), its progress
// line, its run; d and c on it. (ac3pack's, with the WAV, and the alignment c finds.)

constexpr size_t PRE_READ = 4 * MIB;  // c: the stream's start, read first to find the alignment
constexpr size_t FAR_READ = 64 * MIB;  // (and where nothing lines up in it, up to this much more: find_alignment())
constexpr int PROBES = 64;            // the blocks the alignment is found from
static_assert(PROBES <= PROBES_MAX, "find_bind()'s");
constexpr int LFE_PROBES = 1024;      // and the LFE samples its channel is
constexpr int MIX_FRAMES = 256;       // and the frames each channel's mix is (find_mixes())
constexpr int MIX_SEEN = 8192;        // (of the frames read first, at most)
constexpr int MIX_KEEP = 1024;        // (their first blocks' analyses kept for the second pass)

// The sums of a weighted least squares fit of each core channel's decoded subband samples (its
// plain subbands', each by 1 / its uncertainty squared: its quantization's, step^2 / 12, and the
// WAV's own, sg^2, the finer the surer) against each of the WAV's channels' analysis
// (Call::find_mixes()'s first pass)
struct MixSums {
  int nw;     // the WAV's channels
  double sg;  // what the WAV's analysis can't tell apart (its rounding and the bands' leakage: decoder units)
  double G[NCH][WAV_CH][WAV_CH], C[NCH][WAV_CH], Y[NCH];  // (G's lower half)
  double Cu[NCH][WAV_CH], Gu[NCH][WAV_CH];  // and unweighted (loud samples decide: Call::find_mixes())
  U64 N[NCH], NZ[NCH];  // (the samples, and those not 0)
  int nchx;             // (the channels, XCH's too)
  void operator()(const CoreRec& r, const CoreSub& s, int j, S64 (*xw)[32]) {
    double xs[32][WAV_CH];  // (the WAV's channels' values, in decoder units: each once; 0s to a multiple of 4)
    int nb = 0;
    if (r.npchs + r.nxch > nchx) nchx = r.npchs + r.nxch;
    const int nw4 = (nw + 3) & ~3;
    for (int ch = 0; ch < r.npchs + r.nxch; ch++) nb = r.nvqsub[ch] > nb ? r.nvqsub[ch] : nb;
    for (int b = 0; b < nb && b < 32; b++) {
      for (int i = 0; i < nw; i++) xs[b][i] = (double)xw[i][b] / 65536;
      for (int i = nw; i < nw4; i++) xs[b][i] = 0;
    }
    for (int ch = 0; ch < r.npchs + r.nxch; ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        if (!s.abits[ch][b] || s.pmode[ch][b]) continue;
        const S64 ss = step_scale(r, s, ch, b, j >> 3);
        if (ss <= 0) continue;
        const double y = dequantize(s.q[ch][b][j], ss), st = (double)ss / 4194304, wt = 1 / (st * st / 12 + sg * sg);
        const double* const x = xs[b];
#ifdef DTSD_AVX2  // (the same products and sums, 4 at a time: G's upper half's too, never read; a channel
                  // past the WAV's, 0s)
        const __m256d vy = _mm256_set1_pd(y), vw = _mm256_set1_pd(wt);
        for (int i = 0; i < nw4; i += 4) {
          const __m256d xi = _mm256_loadu_pd(x + i), wx = _mm256_mul_pd(vw, xi);
          _mm256_storeu_pd(Cu[ch] + i, _mm256_add_pd(_mm256_loadu_pd(Cu[ch] + i), _mm256_mul_pd(xi, vy)));
          _mm256_storeu_pd(Gu[ch] + i, _mm256_add_pd(_mm256_loadu_pd(Gu[ch] + i), _mm256_mul_pd(xi, xi)));
          _mm256_storeu_pd(C[ch] + i, _mm256_add_pd(_mm256_loadu_pd(C[ch] + i), _mm256_mul_pd(wx, vy)));
        }
        for (int i = 0; i < nw; i++) {
          const __m256d wx = _mm256_set1_pd(wt * x[i]);
          double* const g = G[ch][i];
          for (int k = 0; k <= i; k += 4)
            _mm256_storeu_pd(g + k, _mm256_add_pd(_mm256_loadu_pd(g + k), _mm256_mul_pd(wx, _mm256_loadu_pd(x + k))));
        }
#else
        for (int i = 0; i < nw; i++) {
          const double xi = x[i], wx = wt * xi;
          Cu[ch][i] += xi * y, Gu[ch][i] += xi * xi;
          C[ch][i] += wx * y;
          for (int k = 0; k <= i; k++) G[ch][i][k] += wx * x[k];
        }
#endif
        Y[ch] += wt * y * y;
        N[ch]++, NZ[ch] += y != 0;
      }
  }
};
// A binding's candidates for each core channel, and how many of the core's indexes each makes
// exactly (find_mixes()'s second pass)
struct MixCand {
  int w1, w2;
  S32 g1, g2;
  S32 p1, p2;  // (the gains before the mix, Bind's)
  S64 e1() const { return pre_gain(g1, p1); }  // (the analyses' gains)
  S64 e2() const { return pre_gain(g2, p2); }
};
constexpr int MIX_CANDS = 12;
struct MixHits {
  MixCand c[NCH][MIX_CANDS];
  int nc[NCH];
  U64 hits[NCH][MIX_CANDS], n[NCH];
  void operator()(const CoreRec& r, const CoreSub& s, int j, S64 (*xw)[32]) {
    for (int ch = 0; ch < r.npchs + r.nxch; ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        if (!s.abits[ch][b] || s.pmode[ch][b]) continue;
        const S64 ss = step_scale(r, s, ch, b, j >> 3);
        if (ss <= 0) continue;
        S64 lo, hi, xlo, xhi;
        quant_range(r, ch, s.abits[ch][b], lo, hi);
        index_cell(s.q[ch][b][j], ss, lo, hi, &xlo, &xhi);  // (the values requant() takes to its index)
        for (int k = 0; k < nc[ch]; k++) {
          const MixCand& m = c[ch][k];
          const S128 v = (S128)xw[m.w1][b] * m.e1() + (m.w2 >= 0 ? (S128)xw[m.w2][b] * m.e2() : 0);
          const S64 x = (S64)((v + (1 << 22)) >> 23);
          hits[ch][k] += x >= xlo && x <= xhi;
        }
        n[ch]++;
      }
  }
};

// The chosen binding's variants (find_mixes()'s third pass): as MixHits, and for each sample which
// of them make its index (hist, by the mask of those that do), to tell two of them apart by the
// samples one makes and the other doesn't
constexpr int MIX_REF = 10;
struct MixRef {
  MixCand c[NCH][MIX_REF];
  int nc[NCH];
  U64 hits[NCH][MIX_REF], n[NCH];
  U32 hist[NCH][1 << MIX_REF];
  void operator()(const CoreRec& r, const CoreSub& s, int j, S64 (*xw)[32]) {
    for (int ch = 0; ch < r.npchs + r.nxch; ch++) {
      if (!nc[ch]) continue;
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        if (!s.abits[ch][b] || s.pmode[ch][b]) continue;
        const S64 ss = step_scale(r, s, ch, b, j >> 3);
        if (ss <= 0) continue;
        S64 lo, hi, xlo, xhi;
        quant_range(r, ch, s.abits[ch][b], lo, hi);
        index_cell(s.q[ch][b][j], ss, lo, hi, &xlo, &xhi);
        U32 mask = 0;
        for (int k = 0; k < nc[ch]; k++) {
          const MixCand& m = c[ch][k];
          const S128 v = (S128)xw[m.w1][b] * m.e1() + (m.w2 >= 0 ? (S128)xw[m.w2][b] * m.e2() : 0);
          const S64 x = (S64)((v + (1 << 22)) >> 23);
          const int h = x >= xlo && x <= xhi;
          hits[ch][k] += h, mask |= (U32)h << k;
        }
        hist[ch][mask]++;
        n[ch]++;
      }
    }
  }
  // of channel ch's samples, those variant i makes and k doesn't
  U64 only(int ch, int i, int k) const {
    U64 t = 0;
    for (U32 m = 0; m < 1u << nc[ch]; m++)
      if ((m >> i & 1) && !(m >> k & 1)) t += hist[ch][m];
    return t;
  }
};

struct Call {
  FILE* in;
  const char* in_name;
  FILE* wf;
  const char* wav_name;
  WavIn wav;
  Sink s;
  const char* out_name;
  Progress pr;
  RunCounts n;
  Bind bind;    // c: how the core lines up with the WAV
  int level;    // c: the level it runs (pick_level()'s, or the one asked for)
  double corr;  // and how well its channel 0 correlates there
  bool lfe_few;  // (c: whether the LFE had too few samples that aren't 0 there: find_alignment())
  U8 chfew[NCH];  // (and which channels did: find_mixes())
  U32 core_hz;  // the core's samples a second (as its first frame says; 0: not known)

  // from the file from to to (compare: s compares with to instead of writing it), the WAV wavf, its
  // progress line tagged tag: shown with -p, or where stderr is a terminal, but not with -q
  void init(FILE* from, const char* from_name, FILE* wavf, const char* wname, FILE* to, const char* to_name,
            int compare, const Opts& o, const char* tag) {
    in = from;
    in_name = from_name;
    wf = wavf;
    wav_name = wname;
    wav.init(wf);
    memset(&s, 0, sizeof s);
    s.f = to;
    s.compare = compare;
    out_name = to_name;
    memset(&n, 0, sizeof n);
    bind.none();
    corr = 0;
    lfe_few = false;
    memset(chfew, 0, sizeof chfew);
    core_hz = 0;
    pr.Init();
    if (o.progress || (!o.quiet && stderr_tty())) pr.start(tag, file_size(in));
  }
  // c's or d's call from the file named from, against the WAV named wname, to the one named to: 0,
  // or the exit status of an error (said)
  int open(const char* from, const char* wname, const char* to, const Opts& o, const char* tag) {
    FILE *f, *w, *t;
    if (!(f = open_file(from, 0))) {
      fprintf(stderr, "dtsdpack: can't open %s\n", from);
      return E_READ;
    }
    if (!(w = open_file(wname, 0))) {
      fprintf(stderr, "dtsdpack: can't open %s\n", wname);
      if (f != stdin) fclose(f);
      return E_READ;
    }
    if (!(t = open_file(to, 1))) {
      fprintf(stderr, "dtsdpack: can't create %s\n", to);
      if (f != stdin) fclose(f);
      if (w != stdin) fclose(w);
      return E_WRITE;
    }
    init(f, from, w, wname, t, to, 0, o, tag);
    return 0;
  }
  // the end of the call's progress line, its run's status r; the WAV closed
  void end(int r) {
    pr.end(r, n.raw, n.packed, n.frames);
    pr.Quit();
    wav.quit();
    if (wf && wf != stdin) fclose(wf);
    wf = nullptr;
  }
  // the end of c's or d's call, its run's status r: the output flushed and closed (the input stays
  // open: -v reads it again), the progress line ended; r, or the exit status of an error writing
  int close(int r) {
    int flushed = fflush(s.f) == 0;
#ifndef _WIN32
    struct stat st;  // (open_file()'s output: cut to what was written)
    if (flushed && s.f != stdout && !s.compare && !fstat(fileno(s.f), &st) && S_ISREG(st.st_mode) && (U64)st.st_size > s.pos)
      flushed = ftruncate(fileno(s.f), (off_t)s.pos) == 0;
#endif
    const int closed = s.f == stdout || fclose(s.f) == 0;
    if (!(flushed && closed) && !r) r = (pr.say("dtsdpack: %s: write error\n", out_name), E_WRITE);
    end(r);
    return r;
  }

  // The run c (a coroutine) on the call's files, the bytes at head (got of them) read already first:
  // its progress line's counts f(ctx) as it goes, show(code) after each yield; what run_file() returns
  template <class C, class Show>
  uint run(C& c, ProgressCounts f, void* ctx, Show show, pcU8 head = 0, size_t got = 0) {
    uint y;
    pr.counts(f, ctx);
    y = run_file(c, in, s, show, head, got);
    pr.counts(nullptr, nullptr);
    return y;
  }
  // the end of a run on the call's files: its status r, unless reading or writing failed; said
  int run_end(int r, const char* note) {
    if (ferror(in) || (wf && ferror(wf))) {
      pr.say("dtsdpack: %s: read error\n", ferror(in) ? in_name : wav_name);
      return E_READ;
    }
    if (s.err) {
      pr.say("dtsdpack: %s: write error\n", out_name);
      return E_WRITE;
    }
    if (r) pr.say("dtsdpack: %s: %s\n", in_name, note);
    return r;
  }
  // a run (C: DtsCoro) on the call's files: its counts into n, its status (said)
  template <class C>
  int run_one(C& c, pcU8 head, size_t got) {
    run(c, coro_counts<C>, &c, [&](uint code) { run_show(pr, c, code); }, head, got);
    n.raw = c.raw_done();
    n.packed = c.packed_done();
    n.frames = c.nframe;
    return run_end(c.status, c.note);
  }

  // --- d

  // d of a stream on the call k, at its level LV: the bytes of the input read already (head, got
  // of them) first
  template <class LV>
  struct DecodeOne {
    static int run(Call& k, pcU8 head, size_t got) {
      typedef DtsCoro<1, LV> C;
      C* p = coro_new<C>();
      int r;
      if (!p) no_memory();
      C& c = *p;
      Bind none;
      none.none();
      c.Init(0, &k.wav, none);
      r = k.run_one(c, head, got);
      coro_free(p);
      return r;
    }
  };
  // d of the call's input into its output: 0, or the exit status of an error (said)
  int decode_to() {
    U8 head[DP_AT_BIND];
    const size_t got = fread(head, 1, sizeof head, in);
    int r;
    memset(&n, 0, sizeof n);
    if (got < DP_AT_BIND || memcmp(head, magic, sizeof magic)) {
      if (ferror(in)) return run_end(0, "");
      pr.say("dtsdpack: %s: not a dtsdpack file\n", in_name);
      return E_DATA;
    }
    if (head[DP_AT_VERSION] != DP_VERSION) {
      pr.say("dtsdpack: %s: format version %d, which this dtsdpack (version %d) doesn't read\n", in_name,
             head[DP_AT_VERSION], DP_VERSION);
      return E_DATA;
    }
    r = with_level<DecodeOne>(head[DP_AT_LEVEL], *this, (const U8*)head, got);
    if (r != kNoLevel) return r;
    pr.say("dtsdpack: %s: compressed at level -%d, which this build has not\n", in_name, head[DP_AT_LEVEL]);
    return E_DATA;
  }

  // --- c

  // How the core lines up with the WAV (bind, corr), from the stream's first n bytes at pre (eof:
  // all of it): the decoder's plain subband samples of bands 0-7 of the frames there, each block at
  // its place in the stream as the run counts them (every frame's NBLKS + 1), the PROBES with the
  // most energy of those the WAV's window holds, searched for in the WAV (anal.inc's find_bind()):
  // by the 64-band analysis where the WAV is at twice the core's rate (as the first frame has it).
  // Where nothing lines up there and the WAV can be read again from its start, the same from the
  // stream's first block within 30 dB of its loudest (loud_start()), in those bytes and, where the
  // input can be read on and back, up to FAR_READ more: a stream that starts quiet for longer than
  // the WAV's window (2^20 samples: 21.8 s at 48 kHz) has nothing there to line up. And where the
  // channels lined up but the window had too few of the LFE's samples that aren't 0, the LFE's by
  // them further on (lfe_far()). The WAV is read again from its start after.
  void find_alignment(pcU8 pre, size_t n, int eof) {
    align_at(pre, n, eof, 0);
    const bool far = bind.map[0] < 0;
    if ((!far && !lfe_few && !any_few()) || !wav.can_rewind()) return;
    U8* more = nullptr;
    pcU8 buf = pre;
    size_t bn = n;
    int beof = eof;
    const long at = eof ? -1 : ftell(in);
    if (at >= 0) {
      more = (U8*)xmalloc(n + FAR_READ + PAD);
      memcpy(more, pre, n);
      while (bn < n + FAR_READ) {
        const size_t r = fread(more + bn, 1, n + FAR_READ - bn, in);
        if (!r) break;
        bn += r;
      }
      beof = bn < n + FAR_READ;
      memset(more + bn, 0, PAD);
      if (fseek(in, at, SEEK_SET)) fatal("the input can't be read again from where it was");
      buf = more;
    }
    bool moved = false;
    if (far) {
      const S64 t0 = loud_start(buf, bn, beof);
      if (t0 > 0) align_at(buf, bn, beof, t0), moved = true;
    }
    if (bind.map[0] >= 0 && any_few() && chans_far(buf, bn, beof)) moved = true;
    if (bind.map[0] >= 0 && lfe_few && lfe_far(buf, bn, beof)) moved = true;
    if (moved) wav.rewind();
    free(more);
  }
  // the LFE's binding (where the window the channels lined up in had too few of its samples that
  // aren't 0: align_at()'s) by those in the n bytes at buf (eof: all of them) from the first that
  // has 16 more within the WAV's window, as many as align_at() takes; whether it moved the WAV's
  // window there
  bool lfe_far(pcU8 buf, size_t n, int eof) {
    static CoreRec r;  // (large)
    static S64 ln[LFE_PROBES], ly[LFE_PROBES];
    const int up = bind.up;
    const S64 span = (WAV_WIN - 4096 * up) / (64 * up);  // (the LFE's samples a window holds)
    int nl = 0, nch = 0, filts = -1;
    S64 t = 0;
    for (size_t i = 0; i < n && nl < LFE_PROBES;) {
      const int m = frame_at(buf + i, n - i, eof, &r);
      if (!m) {
        i++;
        continue;
      }
      const int nb = core_blocks(buf + i, (size_t)m);
      if (!core_parse(buf + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
        if (filts < 0) filts = r.filts, nch = r.npchs;
        S64 tb = t;
        for (int sf = 0; sf < r.nsubfs && nl < LFE_PROBES; sf++) {
          const CoreSub& s = r.sub[sf];
          for (int k = 0; r.lff == 2 && k < 4 * s.nssc && nl < LFE_PROBES; k++) {
            if (!s.lfe[k]) continue;
            const S64 p = tb / 2 + k;
            if (nl && p - ln[0] >= span) {  // (past the window from the first: enough, or the first ones dropped)
              if (nl >= 16) goto done;
              int d = 0;
              while (d < nl && p - ln[d] >= span) d++;
              for (int j = d; j < nl; j++) ln[j - d] = ln[j], ly[j - d] = ly[j];
              nl -= d;
            }
            ln[nl] = p, ly[nl++] = s.lfe[k] * lfe_scale(s.lfescale);
          }
          tb += s.nssc * 8;
        }
      }
      t += nb;
      i += (size_t)m;
    }
  done:
    if (nl < 16) return false;
    wav.need(imax64(0, 64 * up * ln[0] + bind.off - 1024 * up), 64 * up * ln[nl - 1] + bind.off + 512 * up);
    bind_lfe(ln, ly, nl, up);
    return true;
  }
  static S64 imax64(S64 a, S64 b) { return a > b ? a : b; }
  bool any_few() const {
    for (int ch = 0; ch < NCH; ch++)
      if (chfew[ch]) return true;
    return false;
  }
  // the channels the window bound none of for too few of their samples that aren't 0 (chfew, as
  // find_mixes() had them) by the frames in the n bytes at buf (eof: all of them) from the first
  // block where one of them is within 30 dB of its loudest, that a window holds, as find_mixes()
  // binds them; whether it moved the WAV's window there
  bool chans_far(pcU8 buf, size_t n, int eof) {
    static CoreRec r;  // (large)
    static U32 fpos[MIX_SEEN], flen[MIX_SEEN];
    static S64 ft[MIX_SEEN];
    double emax[NCH];
    for (int ch = 0; ch < NCH; ch++) emax[ch] = 0;
    S64 t1 = -1;
    for (int pass = 0; pass < 2 && t1 < 0; pass++) {
      int nch = 0, filts = -1;
      S64 t = 0;
      for (size_t i = 0; i < n && t1 < 0;) {
        const int m = frame_at(buf + i, n - i, eof, &r);
        if (!m) {
          i++;
          continue;
        }
        const int nb = core_blocks(buf + i, (size_t)m);
        if (!core_parse(buf + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
          if (filts < 0) filts = r.filts, nch = r.npchs;
          S64 tb = t;
          for (int sf = 0; sf < r.nsubfs && t1 < 0; sf++) {
            const CoreSub& s = r.sub[sf];
            for (int j = 0; j < s.nssc * 8 && t1 < 0; j++, tb++)
              for (int ch = 0; ch < r.npchs + r.nxch && ch < NCH; ch++) {
                if (!chfew[ch]) continue;
                double e = 0;
                for (int b = 0; b < 8; b++)
                  if (b < r.nvqsub[ch] && s.abits[ch][b] && !s.pmode[ch][b]) {
                    const double y = (double)dequantize(s.q[ch][b][j], step_scale(r, s, ch, b, j >> 3));
                    e += y * y;
                  }
                if (!pass) emax[ch] = e > emax[ch] ? e : emax[ch];
                else if (e > 0 && e >= emax[ch] / 1024) {
                  t1 = tb;
                  break;
                }
              }
          }
        }
        t += nb;
        i += (size_t)m;
      }
    }
    if (t1 <= 0) return false;
    const int up = bind.up;
    const S64 tmax = t1 + (WAV_WIN - 4096 * up) / (32 * up);
    int nf = 0, nch = 0, filts = -1;
    S64 t = 0;
    for (size_t i = 0; i < n && t < tmax;) {
      const int m = frame_at(buf + i, n - i, eof, &r);
      if (!m) {
        i++;
        continue;
      }
      const int nb = core_blocks(buf + i, (size_t)m);
      if (!core_parse(buf + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
        if (filts < 0) filts = r.filts, nch = r.npchs;
        if (nf < MIX_SEEN && t >= t1 && t + nb <= tmax) fpos[nf] = (U32)i, flen[nf] = (U32)m, ft[nf++] = t;
      }
      t += nb;
      i += (size_t)m;
    }
    if (!nf) return false;
    wav.need(t1 > 64 ? 32 * up * (t1 - 64) : 0, 32 * up * (ft[nf - 1] + 128) + bind.off + 512 * up);
    find_mixes(buf, fpos, flen, ft, nf, nch, filts, chfew);
    return true;
  }
  // the LFE: the WAV channel no core channel has whose decimation correlates best with its nl
  // samples ln, ly (by those whose decimation's window ends in the WAV, where it ends in the
  // window: as find_bind()'s probes), by 0.9 or more; where none does, the best of them by more
  // than 0.5 if its decimation corrected as the model's fixed predictors have it does (LFE_W2, or
  // LFE_W2D, a decoded WAV's LFE interpolation undone: the WAVs FFmpeg decoded from its encoder's
  // streams, whose LFE the decimation makes 0.89-0.91 of, LFE_W2D 0.985-0.987; any other channel
  // 0.70 at most, 0.27)
  void bind_lfe(S64* ln, S64* ly, int nl, int up) {
    if (wav.got == wav.nsamp) {
      int m = 0;
      for (int k = 0; k < nl; k++)
        if (64 * up * ln[k] + bind.off <= (S64)wav.nsamp) ln[m] = ln[k], ly[m++] = ly[k];
      nl = m;
    }
    if (nl < 16) return;
    double bc = 0.9, b2 = 0.5;
    int found = 0, w2 = -1;
    for (int wc = 0; wc < wav.nch; wc++) {
      int used = 0;
      for (int ch = 0; ch < NCH; ch++) used |= bind.map[ch] == wc || bind.map2[ch] == wc;
      if (used) continue;
      double sxy = 0, sxx = 0, syy = 0;
      for (int k = 0; k < nl; k++) {
        const double x = (double)lfe_decim(wav, wc, 64 * up * ln[k] + bind.off, up), y = (double)ly[k];
        sxy += x * y, sxx += x * x, syy += y * y;
      }
      const double c = sxx > 0 && syy > 0 ? sxy / sqrt(sxx * syy) : 0;
      if (c > bc) bc = c, bind.lfe = (S8)wc, found = 1;
      if (c > b2) b2 = c, w2 = wc;
    }
    if (found || w2 < 0) return;
    // (the decimations by 32 around each sample, as the model's lfe_d2(): each once)
    static S64 dv[128], dm[128];
    for (int i = 0; i < 128; i++) dm[i] = INT64_MIN;
    auto d2 = [&](S64 m) {
      const int i = (int)(m & 127);
      if (dm[i] != m) dm[i] = m, dv[i] = lfe_decim(wav, w2, 32 * up * m + bind.off, up);
      return dv[i];
    };
    for (int dd = 0; dd < 2 && !found; dd++) {
      const S32* const w = dd ? LFE_W2D : LFE_W2;
      double sxy = 0, sxx = 0, syy = 0;
      for (int k = 0; k < nl; k++) {
        S128 p = 0;
        for (int t = 0; t < LFE_N2; t++) p += (S128)w[t] * d2(2 * ln[k] + t - LFE_K2);
        const double x = (double)(S64)(p >> (dd ? 24 : 30)), y = (double)ly[k];
        sxy += x * y, sxx += x * x, syy += y * y;
      }
      if (sxx > 0 && syy > 0 && sxy / sqrt(sxx * syy) >= 0.9) bind.lfe = (S8)w2, found = 1;
    }
  }
  // the first block of the frames in the n bytes at buf (eof: all of them) whose plain subband
  // samples (bands 0-7, as align_at()'s probes have them) have at least a 1024th of the energy of
  // the loudest's (0: none)
  S64 loud_start(pcU8 buf, size_t n, int eof) {
    static CoreRec r;  // (large)
    double emax = 0;
    for (int pass = 0; pass < 2; pass++) {
      int nch = 0, filts = -1;
      S64 t = 0;
      for (size_t i = 0; i < n;) {
        const int m = frame_at(buf + i, n - i, eof, &r);
        if (!m) {
          i++;
          continue;
        }
        const int nb = core_blocks(buf + i, (size_t)m);
        if (!core_parse(buf + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
          if (filts < 0) filts = r.filts, nch = r.npchs;
          S64 tb = t;
          for (int sf = 0; sf < r.nsubfs; sf++) {
            const CoreSub& s = r.sub[sf];
            for (int j = 0; j < s.nssc * 8; j++, tb++) {
              double e = 0;
              for (int ch = 0; ch < r.npchs; ch++)
                for (int b = 0; b < 8; b++)
                  if (b < r.nvqsub[ch] && s.abits[ch][b] && !s.pmode[ch][b]) {
                    const double y = (double)dequantize(s.q[ch][b][j], step_scale(r, s, ch, b, j >> 3));
                    e += y * y;
                  }
              if (!pass) emax = e > emax ? e : emax;
              else if (e > 0 && e >= emax / 1024) return tb;
            }
          }
        }
        t += nb;
        i += (size_t)m;
      }
      if (emax <= 0) return 0;
    }
    return 0;
  }
  // the binding by the blocks from t0 on that the WAV's window holds (find_alignment()'s)
  void align_at(pcU8 pre, size_t n, int eof, S64 t0) {
    static CoreRec r;                 // (large)
    static Probe pr_[4 * PROBES];     // the most energetic so far (a heap would do; few are kept)
    static double en[4 * PROBES];
    static S64 ln[LFE_PROBES], ly[LFE_PROBES];  // the LFE's samples (64x): their places, their values times their steps
    static U32 fpos[MIX_SEEN], flen[MIX_SEEN];   // the frames that parse, for the mixes: where, how long, their first block
    static S64 ft[MIX_SEEN];
    int np = 0, nch = 0, filts = -1, nl = 0, nf = 0, up = 1;
    bool has_lfe = false;
    S64 t = 0;
    lfe_few = false;
    memset(chfew, 0, sizeof chfew);
    if (!wav.ok) return;
    for (size_t i = 0; i < n; i++) {
      const int m = frame_at(pre + i, n - i, eof, &r);
      if (!m) continue;
      if (!core_parse(pre + i, m, r) && wav.rate) {
        core_hz = (U32)core_rate(r.sfreq);
        if (wav.rate == 4 * core_hz) wav.set_dec(2, wav.low_zeros());  // (its lower band: twice the core's rate)
        if (wav.rate == 2 * core_hz) up = 2;
        else if (wav.rate != core_hz) return;  // (nothing the analysis makes of it lines up: not searched)
      }
      break;
    }
    const S64 tmax = t0 + (WAV_WIN - 4096 * up) / (32 * up);  // (the probes' audio within the window from t0)
    for (size_t i = 0; i < n && t < tmax;) {
      const int m = frame_at(pre + i, n - i, eof, &r);
      if (!m) {
        i++;
        continue;
      }
      const int nb = core_blocks(pre + i, (size_t)m);
      if (!core_parse(pre + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
        if (filts < 0) filts = r.filts, nch = r.npchs;
        has_lfe |= r.lff == 2;
        if (nf < MIX_SEEN && t >= t0 && t + nb <= tmax) fpos[nf] = (U32)i, flen[nf] = (U32)m, ft[nf++] = t;
        S64 tb = t;
        for (int sf = 0; sf < r.nsubfs; sf++) {
          const CoreSub& s = r.sub[sf];
          for (int k = 0; r.lff == 2 && k < 4 * s.nssc && nl < LFE_PROBES; k++)  // (those not 0: a silent start says nothing)
            if (s.lfe[k] && tb >= t0) ln[nl] = tb / 2 + k, ly[nl++] = s.lfe[k] * lfe_scale(s.lfescale);
          for (int j = 0; j < s.nssc * 8; j++, tb++) {
            if (tb < t0) continue;
            Probe q;
            double e = 0;
            memset(&q, 0, sizeof q);
            q.t = tb;
            for (int ch = 0; ch < r.npchs; ch++)
              for (int b = 0; b < 8; b++)
                if (b < r.nvqsub[ch] && s.abits[ch][b] && !s.pmode[ch][b]) {
                  q.ok[ch][b] = 1;
                  q.y[ch][b] = dequantize(s.q[ch][b][j], step_scale(r, s, ch, b, j >> 3));
                  e += (double)q.y[ch][b] * q.y[ch][b];
                }
            // kept among the 4 PROBES most energetic (the weakest replaced)
            if (np < 4 * PROBES) pr_[np] = q, en[np++] = e;
            else {
              int w = 0;
              for (int k = 1; k < np; k++)
                if (en[k] < en[w]) w = k;
              if (e > en[w]) pr_[w] = q, en[w] = e;
            }
          }
        }
      }
      t += nb;
      i += (size_t)m;
    }
    if (!np) return;
    // the PROBES most energetic, in the order of the stream
    static Probe best[PROBES];
    int nb = 0;
    for (; nb < PROBES && nb < np; nb++) {
      int w = 0;
      for (int k = 1; k < np; k++)
        if (en[k] > en[w]) w = k;
      if (en[w] <= 0) break;
      best[nb] = pr_[w];
      en[w] = -1;
    }
    S64 last = 0;
    for (int k = 0; k < nb; k++) last = best[k].t > last ? best[k].t : last;
    for (int k = 0; k < nl; k++) last = imax(last, 2 * ln[k]);
    wav.need(t0 > 64 ? 32 * up * (t0 - 64) : 0, 32 * up * (last + 1) + (1536 + 512) * up);  // (from 2048 samples (times up) before t0's)
    bind = find_bind(wav, best, nb, nch, filts, up, &corr);
    bind.dec = (S8)wav.dec;
    bind.dsh = (S8)wav.dsh;
    if (bind.map[0] >= 0) find_mixes(pre, fpos, flen, ft, nf, nch, filts);
    // the LFE (too few of its samples aren't 0, where the core has one at 64 times: lfe_few, for
    // find_alignment())
    if (bind.map[0] < 0) return;
    lfe_few = has_lfe && nl < 16;
    bind_lfe(ln, ly, nl, up);
  }

  // a pass over (up to MIX_FRAMES of) the nf frames read first, at pre (where, how long, their
  // first blocks), evenly spaced: f(r, s, j, xw) for each block j of each of their subframes s,
  // the WAV's channels' analysis there in xw (those of the first keep_n blocks: in keep, where
  // filled; else kept there, nw channels a block)
  template <class F>
  void mix_pass(pcU8 pre, const U32* pos, const U32* len, const S64* ft, int nf, int nch, int filts, F& f,
                S64 (*keep)[32], S64* keep_t, int keep_n, bool filled) {
    static CoreRec r;
    static S64 xw[WAV_CH][32];
    const S64* fir = filts ? FIR_PERFECT : FIR_NONPERFECT;
    const int ns = imin(nf, MIX_FRAMES), nw = wav.nch;
    int nb = 0;
    for (int k = 0; k < ns; k++) {
      const int i = nf <= MIX_FRAMES ? k : (int)((S64)k * nf / MIX_FRAMES);
      if (core_parse(pre + pos[i], (int)len[i], r) || r.npchs != nch || r.filts != filts) continue;
      S64 tb = ft[i];
      for (int sf = 0; sf < r.nsubfs; sf++) {
        const CoreSub& s = r.sub[sf];
        for (int j = 0; j < s.nssc * 8; j++, tb++, nb++) {
          S64(*const kp)[32] = nb < keep_n ? keep + (size_t)nb * nw : nullptr;
          if (kp && filled) memcpy(xw, kp, sizeof(S64) * 32 * nw);
          else {
            for (int wc = 0; wc < nw; wc++) analyze(wav, wc, 32 * bind.up * tb + bind.off, bind.up, fir, xw[wc]);
            if (kp) memcpy(kp, xw, sizeof(S64) * 32 * nw), keep_t[nb] = tb;
          }
          f(r, s, j, xw);
        }
      }
    }
  }
  // How each core channel lines up with the WAV's, the offset found (bind's map, map2, g1, g2): its
  // best single WAV channel and its best pair, by a weighted least squares fit over the frames read
  // first (MixSums), each with its gains as fitted and as the downmix table's entries either side
  // (gain_near()), the best single channel as it is too, and the channel the alignment found, as
  // it is and by the table's gains of an unweighted fit; of those, the one that makes the most of
  // the channel's indexes exactly
  // (MixHits; by 0.5% of them at least over the ones before it; not one whose gains are all under
  // 1/64), where that is a fifth of them at least (else the channel isn't bound)
  // (only: the channels it binds, those of it not 0; else all, and those it had too few samples of
  // that aren't 0 to bind, where the alignment didn't, in chfew)
  void find_mixes(pcU8 pre, const U32* pos, const U32* len, const S64* ft, int nf, int nch, int filts,
                  const U8* only = nullptr) {
    static MixSums ms;
    static MixHits mh;
    S64 last = 0;
    for (int k = 0; k < nf; k++) last = imax(last, ft[k]);
    wav.need(0, 32 * bind.up * (last + 128) + bind.off + 512 * bind.up);
    memset((void*)&ms, 0, sizeof ms);
    memset((void*)&mh, 0, sizeof mh);
    ms.nw = wav.nch;
    // (a silent subband the encoder gave a fine step and indexes of 0, as an open rate's may, is
    // the WAV's rounding noise there: without the WAV's uncertainty such samples outweigh the rest)
    ms.sg = 0.5 * (wav.bps == 2 ? 256 : 1);
    // (the first pass's analyses, of its first MIX_KEEP blocks, for the second)
    const int keep_n = MIX_KEEP;
    S64(*const keep)[32] = (S64(*)[32])xmalloc(sizeof(S64) * 32 * wav.nch * (size_t)keep_n);
    S64* const keep_t = (S64*)xmalloc(sizeof(S64) * (size_t)keep_n);
    for (int k = 0; k < keep_n; k++) keep_t[k] = -1;
    mix_pass(pre, pos, len, ft, nf, nch, filts, ms, keep, keep_t, keep_n, false);
    const int nw = wav.nch;
    auto g = [&](int ch, int i, int k) { return i >= k ? ms.G[ch][i][k] : ms.G[ch][k][i]; };
    if (!only) memset(chfew, 0, sizeof chfew);
    for (int ch = 0; ch < NCH; ch++) {  // (XCH's channel too: after the core's)
      if (only && !only[ch]) continue;
      // (too few to tell: as find_bind() had it; where it had none, too few not 0 too: a silent
      // channel's fit, all 0s, took a gain of 0, as FFmpeg's encoder's silent subbands have bits)
      if (ms.N[ch] < 64 || (bind.map[ch] < 0 && ms.NZ[ch] < 64)) {
        if (!only && bind.map[ch] < 0 && ch < ms.nchx) chfew[ch] = 1;
        continue;
      }
      double rs = 1e300, rp = 1e300, gs = 0, pa = 0, pb = 0;
      int si = -1, pi = -1, pk = -1;
      for (int i = 0; i < nw; i++) {
        const double gi = g(ch, i, i);
        if (gi <= 0) continue;
        const double a = ms.C[ch][i] / gi, res = ms.Y[ch] - a * ms.C[ch][i];
        if (res < rs) rs = res, si = i, gs = a;
        for (int k = i + 1; k < nw; k++) {
          const double gk = g(ch, k, k), gik = g(ch, i, k), d = gi * gk - gik * gik;
          if (gk <= 0 || d <= 1e-9 * gi * gk) continue;
          const double x = (ms.C[ch][i] * gk - ms.C[ch][k] * gik) / d, z = (ms.C[ch][k] * gi - ms.C[ch][i] * gik) / d;
          const double res2 = ms.Y[ch] - x * ms.C[ch][i] - z * ms.C[ch][k];
          if (res2 < rp) rp = res2, pi = i, pk = k, pa = x, pb = z;
        }
      }
      // the candidates, in order of preference: the channel the alignment found, as it is and by the
      // table's gains of the unweighted fit (the weighted one can miss them: a decoded WAV's analysis
      // isn't the core's where the filterbank's reconstruction errs most, beside a sudden loud
      // onset, and those samples' fine steps outweigh the rest); one channel by the table's gains
      // (the nearer first), by the fit's, as it is (where the alignment didn't find it, as it
      // doesn't XCH's: a clipped decode's fit can be far off, FATE's xbr_xch_61_24_48_3840's XCH
      // channel's 0.065); two channels by the table's, by the fit's
      int& n = mh.nc[ch];
      S32 ga[2], gb[2];
      if (bind.map[ch] >= 0) {
        const int w = bind.map[ch];
        mh.c[ch][n++] = MixCand{w, -1, BIND_ONE, 0, BIND_Q15, BIND_Q15};
        if (ms.Gu[ch][w] > 0) {
          const int na = gain_near(ms.Cu[ch][w] / ms.Gu[ch][w], ga);
          for (int a = 0; a < na; a++)
            if (ga[a] != BIND_ONE) mh.c[ch][n++] = MixCand{w, -1, ga[a], 0, BIND_Q15, BIND_Q15};
        }
      }
      if (si >= 0) {
        const int na = gain_near(gs, ga);
        for (int a = 0; a < na; a++) mh.c[ch][n++] = MixCand{si, -1, ga[a], 0, BIND_Q15, BIND_Q15};
        mh.c[ch][n++] = MixCand{si, -1, (S32)llround(gs * BIND_ONE), 0, BIND_Q15, BIND_Q15};
        if (si != bind.map[ch]) mh.c[ch][n++] = MixCand{si, -1, BIND_ONE, 0, BIND_Q15, BIND_Q15};
      }
      if (pi >= 0) {
        const int na = gain_near(pa, ga), nb2 = gain_near(pb, gb);
        for (int a = 0; a < na; a++)
          for (int z = 0; z < nb2; z++) mh.c[ch][n++] = MixCand{pi, pk, ga[a], gb[z], BIND_Q15, BIND_Q15};
        mh.c[ch][n++] = MixCand{pi, pk, (S32)llround(pa * BIND_ONE), (S32)llround(pb * BIND_ONE), BIND_Q15, BIND_Q15};
      }
    }
    mix_pass(pre, pos, len, ft, nf, nch, filts, mh, keep, keep_t, keep_n, true);
    auto big = [](S32 g) { return (g < 0 ? -(S64)g : g) >= BIND_ONE / 64; };
    int bks[NCH];
    for (int ch = 0; ch < NCH; ch++) {
      bks[ch] = -1;
      if (!mh.nc[ch] || !mh.n[ch] || (only && !only[ch])) continue;
      int bk = 0;  // (the candidates in order of preference: one channel before two, the table's gains before the fit's;
      for (int k = 1; k < mh.nc[ch]; k++) {  // not one whose gains are all under 1/64, as a clipped WAV's fit makes:
        const MixCand& m = mh.c[ch][k];        // its distortion swamps the quiet subbands with fine steps)
        if ((big(m.g1) || (m.w2 >= 0 && big(m.g2))) && mh.hits[ch][k] > mh.hits[ch][bk] + mh.n[ch] / 200) bk = k;
      }
      if (mh.hits[ch][bk] * 5 >= mh.n[ch]) bks[ch] = bk;
    }
    // (then, at the core's rate, the choice's variants: each gain as the table's entry nearest it,
    // each WAV channel's samples times 1, 1 - 2^-15 or 1 + 2^-15 before the mix, a pair's either or
    // both, and the choice as it is; a third pass counts the indexes each makes. A DTS:X encoder's
    // sides went into the core's surrounds times 32767/32768, a Q15 unity's, rounded, then at the
    // table's gain, which the fits miss by about their own noise: and two surrounds bound by fits
    // differ where they are the same, so that the second isn't the first's twin. At twice the rate
    // a lossless WAV's 64-band analysis is 1 - 1.4e-4 times the one the passes make, so that
    // 1 - 2^-15 looked better there and wasn't)
    static MixRef mr;
    memset((void*)&mr, 0, sizeof mr);
    int rfk[NCH] = {0}, nord[NCH] = {0};
    bool more = false;
    for (int ch = 0; ch < NCH; ch++) {
      if (bks[ch] < 0 || bind.up != 1) continue;
      const MixCand& m = mh.c[ch][bks[ch]];
      auto tab = [&](S32 g) {
        S32 t[2];
        return big(g) && gain_near((double)g / BIND_ONE, t) ? t[0] : g;
      };
      const S32 t1 = tab(m.g1), t2 = m.w2 >= 0 ? tab(m.g2) : 0;
      int& n = mr.nc[ch];
      static const int ord[9][2] = {{0, 0}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
      for (int k = 0; k < 9; k++) {
        const int d1 = ord[k][0], d2 = ord[k][1];
        if ((d1 && !big(t1)) || (d2 && (m.w2 < 0 || !big(t2)))) continue;
        MixCand v = m;
        v.g1 = t1, v.p1 = BIND_Q15 + d1;
        if (m.w2 >= 0) v.g2 = t2, v.p2 = BIND_Q15 + d2;
        mr.c[ch][n++] = v;
      }
      nord[ch] = n;
      rfk[ch] = -1;
      for (int k = 0; k < n && rfk[ch] < 0; k++)
        if (mr.c[ch][k].g1 == m.g1 && mr.c[ch][k].p1 == m.p1 && (m.w2 < 0 || (mr.c[ch][k].g2 == m.g2 && mr.c[ch][k].p2 == m.p2)))
          rfk[ch] = k;
      if (rfk[ch] < 0) rfk[ch] = n, mr.c[ch][n++] = m;
      more |= n > 1;
    }
    if (more) mix_pass(pre, pos, len, ft, nf, nch, filts, mr, keep, keep_t, keep_n, true);
    // (the first pass's analyses, kept for the run: the model's, of the same blocks and channels,
    // the same)
    int kn = 0;
    while (kn < keep_n && keep_t[kn] >= 0) kn++;
    if (kn && !wav.ana_n) {  // (the first window's, where it's there)
      wav.ana = (S64*)keep, wav.ana_t = keep_t, wav.ana_n = kn, wav.ana_up = bind.up, wav.ana_off = bind.off;
      wav.ana_got = (S64)wav.got, wav.ana_fir = filts ? FIR_PERFECT : FIR_NONPERFECT;
    } else
      free(keep), free(keep_t);
    for (int ch = 0; ch < NCH; ch++) {
      if (!mh.nc[ch] || !mh.n[ch] || (only && !only[ch])) continue;
      bind.alt[ch] = 0, bind.p1[ch] = bind.p2[ch] = bind.pa1[ch] = bind.pa2[ch] = BIND_Q15;
      if (bks[ch] < 0) {
        bind.map[ch] = bind.map2[ch] = -1, bind.g1[ch] = BIND_ONE, bind.g2[ch] = 0;
        continue;
      }
      // (of the variants that make no fewer of the indexes than the choice as it is, the first in
      // that order that the one making most doesn't clearly beat: by the samples one of the two
      // makes and the other doesn't, by more than 8 and 3 sigmas of a fair coin's. Not by the
      // misses' count: elsewhere in the first window misses of other kinds dilute those of the gain)
      const int nv = mr.nc[ch], rf = rfk[ch];
      int bb = 0, bk = rf;
      for (int k = 1; k < nv; k++)
        if (mr.hits[ch][k] > mr.hits[ch][bb]) bb = k;
      for (int k = 0; k < nv; k++) {
        if (k != rf && mr.hits[ch][k] < mr.hits[ch][rf]) continue;
        const U64 a = mr.only(ch, bb, k), c = mr.only(ch, k, bb);
        const double d = (double)a - (double)c;
        if (d <= 8 || d * d <= 9.0 * (double)(a + c)) {
          bk = k;
          break;
        }
      }
      const MixCand& m = mr.nc[ch] ? mr.c[ch][bk] : mh.c[ch][bks[ch]];
      bind.map[ch] = (S8)m.w1, bind.map2[ch] = (S8)m.w2, bind.g1[ch] = m.g1, bind.g2[ch] = m.w2 >= 0 ? m.g2 : 0;
      bind.p1[ch] = m.p1, bind.p2[ch] = m.w2 >= 0 ? m.p2 : BIND_Q15;
      // (and a second pair of gains before the mix, model_smp.inc's gain_pick() taking either for a
      // subframe by the indexes each has made lately: where the choice's are 1 -+ 2^-15, one or
      // both, 1; where they are 1, the variant's that makes most of the indexes where that makes 8
      // more than the choice. A DTS:X encoder put its sides into the core's surrounds times
      // 32767/32768, rounded, which leaves a sample under 2^14 as it is: where they are quiet in
      // the frames the passes take, the two make the same indexes, 1 is the choice, and the stream
      // may be louder further on)
      int ak = -1;
      if (bk > 0 && bk < nord[ch]) ak = 0;
      if (!bk && nord[ch] > 1) {
        int vb = 1;
        for (int k = 2; k < nord[ch]; k++)
          if (mr.hits[ch][k] > mr.hits[ch][vb]) vb = k;
        if (mr.hits[ch][vb] >= mr.hits[ch][0] + 8) ak = vb;
      }
      if (ak >= 0) bind.alt[ch] = 1, bind.pa1[ch] = mr.c[ch][ak].p1, bind.pa2[ch] = m.w2 >= 0 ? mr.c[ch][ak].p2 : BIND_Q15;
    }
  }

  // c on the call k at the level LV: its statistics into st, cost and orig
  template <class LV>
  struct CompressOne {
    static int run(Call& k, const Opts& o, Stats& st, rp<double> cost, rp<double> orig, pcU8 pre, size_t got) {
      typedef DtsCoro<0, LV> C;
      C* p = coro_new<C>();
      int r;
      if (!p) no_memory();
      C& c = *p;
      c.Init(o.stats, &k.wav, k.bind);
      r = k.run_one(c, pre, got);
      st = c.st;
      memcpy(cost, c.cost, sizeof c.cost);
      memcpy(orig, c.orig, sizeof c.orig);
      coro_free(p);
      return r;
    }
  };
  // c without a level: -8, the DTS:X encoders' parameters, where the binding mixes two WAV channels
  // with a gain before the mix (their downmix's surrounds, a back surround times a Q15 unity's
  // 32767/32768, above) and the build has -8; else the first level, dflt. (A channel scaled alone
  // is not their sign: an FFmpeg encoder's stream whose surrounds are bound so, 0.01% bigger at -8.)
  int pick_level(int dflt) const {
    bool pre = false;
    for (int ch = 0; ch < NCH; ch++)
      if (bind.map[ch] >= 0 && bind.map2[ch] >= 0 &&
          (bind.p1[ch] != BIND_Q15 || bind.p2[ch] != BIND_Q15 || bind.alt[ch]))
        pre = true;
    if (!pre) return dflt;
    for (size_t i = 0; i < sizeof kLevels / sizeof kLevels[0]; i++)
      if (kLevels[i] == 8) return 8;
    return dflt;
  }
  int compress_one(const Opts& o, Stats& st, rp<double> cost, rp<double> orig) {
    U8* pre = (U8*)xmalloc(PRE_READ + PAD);
    size_t got = 0;
    while (got < PRE_READ) {
      const size_t r = fread(pre + got, 1, PRE_READ - got, in);
      if (!r) break;
      got += r;
    }
    memset(pre + got, 0, PAD);
    find_alignment(pre, got, got < PRE_READ);
    level = o.level_set ? o.level : pick_level(o.level);
    const int r = with_level<CompressOne>(level, *this, o, st, cost, orig, (pcU8)pre, got);
    free(pre);
    return r;
  }
};
