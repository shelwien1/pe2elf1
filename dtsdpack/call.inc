// call.inc -- dtsdpack.cpp's Call: a call's files (the stream, the WAV, the output), its progress
// line, its run; d and c on it. (ac3pack's, with the WAV, and the alignment c finds.)

constexpr size_t PRE_READ = 4 * MIB;  // c: the stream's start, read first to find the alignment
constexpr int PROBES = 64;            // the blocks the alignment is found from
constexpr int LFE_PROBES = 1024;      // and the LFE samples its channel is

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
  // most energy of those the WAV's window holds, searched for in the WAV (anal.inc's find_bind())
  void find_alignment(pcU8 pre, size_t n, int eof) {
    static CoreRec r;                 // (large)
    static Probe pr_[4 * PROBES];     // the most energetic so far (a heap would do; few are kept)
    static double en[4 * PROBES];
    static S64 ln[LFE_PROBES], ly[LFE_PROBES];  // the LFE's samples (64x): their places, their values times their steps
    int np = 0, nch = 0, filts = -1, nl = 0;
    S64 t = 0;
    const S64 tmax = (WAV_WIN - 4096) / 32;  // (the probes' audio within the window from 0)
    if (!wav.ok) return;
    for (size_t i = 0; i < n && t < tmax;) {
      const int m = frame_at(pre + i, n - i, eof, &r);
      if (!m) {
        i++;
        continue;
      }
      const int nb = core_blocks(pre + i, (size_t)m);
      if (!core_parse(pre + i, m, r) && (filts < 0 || (r.npchs == nch && r.filts == filts))) {
        if (filts < 0) filts = r.filts, nch = r.npchs;
        S64 tb = t;
        for (int sf = 0; sf < r.nsubfs; sf++) {
          const CoreSub& s = r.sub[sf];
          for (int k = 0; r.lff == 2 && k < 4 * s.nssc && nl < LFE_PROBES; k++, nl++)
            ln[nl] = tb / 2 + k, ly[nl] = s.lfe[k] * lfe_scale(s.lfescale);
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
    wav.need(0, 32 * (last + 1) + 1536 + 512);
    bind = find_bind(wav, best, nb, nch, filts, &corr);
    // the LFE: the WAV channel no core channel has whose decimation correlates best with it
    if (bind.map[0] < 0 || nl < 16) return;
    double bc = 0.9;
    for (int wc = 0; wc < wav.nch; wc++) {
      int used = 0;
      for (int ch = 0; ch < NCH; ch++) used |= bind.map[ch] == wc;
      if (used) continue;
      double sxy = 0, sxx = 0, syy = 0;
      for (int k = 0; k < nl; k++) {
        const double x = (double)lfe_decim(wav, wc, 64 * ln[k] + bind.off), y = (double)ly[k];
        sxy += x * y, sxx += x * x, syy += y * y;
      }
      const double c = sxx > 0 && syy > 0 ? sxy / sqrt(sxx * syy) : 0;
      if (c > bc) bc = c, bind.lfe = (S8)wc;
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
