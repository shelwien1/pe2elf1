// call.inc -- dtsdpack.cpp's Call: a call's files (the stream, the WAV, the output), its progress
// line, its run; d and c on it. (ac3pack's, with the WAV, and the alignment c finds.)

constexpr size_t PRE_READ = 4 * MIB;  // c: the stream's start, read first to find the alignment
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
  U64 N[NCH];
  void operator()(const CoreRec& r, const CoreSub& s, int j, S64 (*xw)[32]) {
    for (int ch = 0; ch < r.npchs + r.nxch; ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        if (!s.abits[ch][b] || s.pmode[ch][b]) continue;
        const S64 ss = step_scale(r, s, ch, b, j >> 3);
        if (ss <= 0) continue;
        const double y = dequantize(s.q[ch][b][j], ss), st = (double)ss / 4194304, wt = 1 / (st * st / 12 + sg * sg);
        for (int i = 0; i < nw; i++) {
          const double xi = (double)xw[i][b] / 65536;
          Cu[ch][i] += xi * y, Gu[ch][i] += xi * xi;
          C[ch][i] += wt * xi * y;
          for (int k = 0; k <= i; k++) G[ch][i][k] += wt * xi * ((double)xw[k][b] / 65536);
        }
        Y[ch] += wt * y * y;
        N[ch]++;
      }
  }
};
// A binding's candidates for each core channel, and how many of the core's indexes each makes
// exactly (find_mixes()'s second pass)
struct MixCand {
  int w1, w2;
  S32 g1, g2;
};
constexpr int MIX_CANDS = 11;
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
        S64 lo, hi, alt;
        S128 dist;
        quant_range(r, ch, s.abits[ch][b], lo, hi);
        for (int k = 0; k < nc[ch]; k++) {
          const MixCand& m = c[ch][k];
          const S128 v = (S128)xw[m.w1][b] * m.g1 + (m.w2 >= 0 ? (S128)xw[m.w2][b] * m.g2 : 0);
          hits[ch][k] += requant((S64)((v + (1 << 22)) >> 23), ss, lo, hi, &dist, &alt) == s.q[ch][b][j];
        }
        n[ch]++;
      }
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
  double corr;  // and how well its channel 0 correlates there
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
    const int flushed = fflush(s.f) == 0, closed = s.f == stdout || fclose(s.f) == 0;
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
    U8 head[DP_HEAD];
    const size_t got = fread(head, 1, sizeof head, in);
    int r;
    memset(&n, 0, sizeof n);
    if (got < DP_HEAD || memcmp(head, magic, sizeof magic)) {
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
  // by the 64-band analysis where the WAV is at twice the core's rate (as the first frame has it)
  void find_alignment(pcU8 pre, size_t n, int eof) {
    static CoreRec r;                 // (large)
    static Probe pr_[4 * PROBES];     // the most energetic so far (a heap would do; few are kept)
    static double en[4 * PROBES];
    static S64 ln[LFE_PROBES], ly[LFE_PROBES];  // the LFE's samples (64x): their places, their values times their steps
    static U32 fpos[MIX_SEEN], flen[MIX_SEEN];   // the frames that parse, for the mixes: where, how long, their first block
    static S64 ft[MIX_SEEN];
    int np = 0, nch = 0, filts = -1, nl = 0, nf = 0, up = 1;
    S64 t = 0;
    if (!wav.ok) return;
    for (size_t i = 0; i < n; i++) {
      const int m = frame_at(pre + i, n - i, eof, &r);
      if (!m) continue;
      if (!core_parse(pre + i, m, r) && wav.rate) {
        core_hz = (U32)core_rate(r.sfreq);
        if (wav.rate == 2 * core_hz) up = 2;
        else if (wav.rate != core_hz) return;  // (nothing the analysis makes of it lines up: not searched)
      }
      break;
    }
    const S64 tmax = (WAV_WIN - 4096 * up) / (32 * up);  // (the probes' audio within the window from 0)
    for (size_t i = 0; i < n && t < tmax;) {
      const int m = frame_at(pre + i, n - i, eof, &r);
      if (!m) {
        i++;
        continue;
      }
      const int nb = core_blocks(pre + i, (size_t)m);
      if (!core_parse(pre + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
        if (filts < 0) filts = r.filts, nch = r.npchs;
        if (nf < MIX_SEEN && t + nb <= tmax) fpos[nf] = (U32)i, flen[nf] = (U32)m, ft[nf++] = t;
        S64 tb = t;
        for (int sf = 0; sf < r.nsubfs; sf++) {
          const CoreSub& s = r.sub[sf];
          for (int k = 0; r.lff == 2 && k < 4 * s.nssc && nl < LFE_PROBES; k++)  // (those not 0: a silent start says nothing)
            if (s.lfe[k]) ln[nl] = tb / 2 + k, ly[nl++] = s.lfe[k] * lfe_scale(s.lfescale);
          for (int j = 0; j < s.nssc * 8; j++, tb++) {
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
    wav.need(0, 32 * up * (last + 1) + (1536 + 512) * up);
    bind = find_bind(wav, best, nb, nch, filts, up, &corr);
    if (bind.map[0] >= 0) find_mixes(pre, fpos, flen, ft, nf, nch, filts);
    // the LFE: the WAV channel no core channel has whose decimation correlates best with it
    if (bind.map[0] < 0 || nl < 16) return;
    double bc = 0.9;
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
      if (c > bc) bc = c, bind.lfe = (S8)wc;
    }
  }

  // a pass over (up to MIX_FRAMES of) the nf frames read first, at pre (where, how long, their
  // first blocks), evenly spaced: f(r, s, j, xw) for each block j of each of their subframes s,
  // the WAV's channels' analysis there in xw (those of the first keep_n blocks: in keep, where
  // filled; else kept there, nw channels a block)
  template <class F>
  void mix_pass(pcU8 pre, const U32* pos, const U32* len, const S64* ft, int nf, int nch, int filts, F& f,
                S64 (*keep)[32], int keep_n, bool filled) {
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
            if (kp) memcpy(kp, xw, sizeof(S64) * 32 * nw);
          }
          f(r, s, j, xw);
        }
      }
    }
  }
  // How each core channel lines up with the WAV's, the offset found (bind's map, map2, g1, g2): its
  // best single WAV channel and its best pair, by a weighted least squares fit over the frames read
  // first (MixSums), each with its gains as fitted and as the downmix table's entries either side
  // (gain_near()), and the channel the alignment found, as it is and by the table's gains of an
  // unweighted fit; of those, the one that makes the most of the channel's indexes exactly
  // (MixHits; by 0.5% of them at least over the ones before it), where that is a fifth of them at
  // least (else the channel isn't bound)
  void find_mixes(pcU8 pre, const U32* pos, const U32* len, const S64* ft, int nf, int nch, int filts) {
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
    mix_pass(pre, pos, len, ft, nf, nch, filts, ms, keep, keep_n, false);
    const int nw = wav.nch;
    auto g = [&](int ch, int i, int k) { return i >= k ? ms.G[ch][i][k] : ms.G[ch][k][i]; };
    for (int ch = 0; ch < NCH; ch++) {  // (XCH's channel too: after the core's)
      if (ms.N[ch] < 64) continue;  // (too few to tell: as find_bind() had it)
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
      // (the nearer first), by the fit's; two channels by the table's, by the fit's
      int& n = mh.nc[ch];
      S32 ga[2], gb[2];
      if (bind.map[ch] >= 0) {
        const int w = bind.map[ch];
        mh.c[ch][n++] = MixCand{w, -1, BIND_ONE, 0};
        if (ms.Gu[ch][w] > 0) {
          const int na = gain_near(ms.Cu[ch][w] / ms.Gu[ch][w], ga);
          for (int a = 0; a < na; a++)
            if (ga[a] != BIND_ONE) mh.c[ch][n++] = MixCand{w, -1, ga[a], 0};
        }
      }
      if (si >= 0) {
        const int na = gain_near(gs, ga);
        for (int a = 0; a < na; a++) mh.c[ch][n++] = MixCand{si, -1, ga[a], 0};
        mh.c[ch][n++] = MixCand{si, -1, (S32)llround(gs * BIND_ONE), 0};
      }
      if (pi >= 0) {
        const int na = gain_near(pa, ga), nb2 = gain_near(pb, gb);
        for (int a = 0; a < na; a++)
          for (int z = 0; z < nb2; z++) mh.c[ch][n++] = MixCand{pi, pk, ga[a], gb[z]};
        mh.c[ch][n++] = MixCand{pi, pk, (S32)llround(pa * BIND_ONE), (S32)llround(pb * BIND_ONE)};
      }
    }
    mix_pass(pre, pos, len, ft, nf, nch, filts, mh, keep, keep_n, true);
    free(keep);
    for (int ch = 0; ch < NCH; ch++) {
      if (!mh.nc[ch] || !mh.n[ch]) continue;
      int bk = 0;  // (the candidates in order of preference: one channel before two, the table's gains before the fit's)
      for (int k = 1; k < mh.nc[ch]; k++)
        if (mh.hits[ch][k] > mh.hits[ch][bk] + mh.n[ch] / 200) bk = k;
      const MixCand& m = mh.c[ch][bk];
      if (mh.hits[ch][bk] * 5 < mh.n[ch]) {
        bind.map[ch] = bind.map2[ch] = -1, bind.g1[ch] = BIND_ONE, bind.g2[ch] = 0;
        continue;
      }
      bind.map[ch] = (S8)m.w1, bind.map2[ch] = (S8)m.w2, bind.g1[ch] = m.g1, bind.g2[ch] = m.w2 >= 0 ? m.g2 : 0;
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
    const int r = with_level<CompressOne>(o.level, *this, o, st, cost, orig, (pcU8)pre, got);
    free(pre);
    return r;
  }
};
