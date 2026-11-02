#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
#include "vb_dict.inc"
#include "oc_rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "sh_mapping.inc"
#include "tc_base.inc"
#include "tc_ptab.inc"
#include "tc_tables.inc"
#include "tc_fam.inc"
#include "oc_hist.inc"
#include "oc_model.inc"
#include "oc_frame.inc"
#include "oc_header.inc"
#include "oc_raw.inc"
#include "oc_floor.inc"
#include "oc_residue.inc"
#include "vb_packet.inc"
#include "ogg_stream.inc"
#include "oc_coro.inc"
#include "oc_api.inc"
#include "oc_load.inc"
#include "oc_progress.inc"
#include "oc_mt.inc"
//  What the command line asks for besides the files.
struct oc_opts {
  int flags;     // OC_F_VERBOSE, OC_F_HUGE
  int threads;   // -t: 1 is the stream of one model
  u64 part;      // -c: a part's size, at least; 0: mt_part_default()
  int progress;  // -p: the progress line even where stderr is not a terminal
  int quiet;     // -q: no progress line
};
static int oc_show_progress(const oc_opts &o) { return o.progress || (!o.quiet && oc_stderr_tty()); }
//  The head of what d is given, as main() reads it to learn the model and
//  whether the stream was coded on threads: the magic, the version, the
//  model -- the whole header of a stream of one model, the first 7 bytes of
//  12 of one coded on threads.  The run takes these bytes as read: they are
//  not read again, since from stdin they could not be.
constexpr uint OC_HEAD = sizeof OC_MAGIC + 1;
struct oc_head {
  u8 b[OC_HEAD];
  uint n;  // the bytes there were: fewer where the input ends sooner
};
//  A run the model refused or could not do: its message, the output
//  dropped, the exit status the error names.
static NORETURN void run_fail(oc_api &api, void *p, int r) {
  ogc_fatal_code(r >= 5 && r <= 8 ? r - 4 : OGC_EXIT_REFUSED, "%s", api.Error(p));
}
//  A run of one model's counts, for its progress line: the .ogg side's
//  bytes as the model says they are done -- an encoder reads far ahead of
//  where it codes -- or as they were fed and written, and the .oc side's.
struct oc_run_counts {
  oc_api *api;
  void *p;
  int dir;
  u64 fed, written;  // the bytes given to the model and taken from it, with the header
};
static void oc_one_counts(void *ctx, oc_counts &c) {
  const oc_run_counts &k = *(const oc_run_counts *)ctx;
  c.parts = 0;
  if(k.dir == OC_ENCODE) {
    c.raw = k.api->Done ? (u64)k.api->Done(k.p) : k.fed;
    c.packed = k.written;
  } else {
    c.raw = k.written;
    c.packed = k.fed;
  }
}
//  One run through whichever model the table holds -- the program's own or
//  a library's, the same code either way: the header, then the files
//  through oc_api.h's loop, then the report.  f is the input, open, and
//  for d past the header in hd.
static int run(oc_api &api, int dir, const oc_opts &o, filehandle &f, const oc_head &hd, const char *in, const char *out) {
  static u8 ibuf[1 << 16], obuf[1 << 16];
  const int flags = o.flags;
  filehandle g;
  oc_progress pr;
  oc_run_counts k;
  void *p;
  int r;
  if(dir == OC_ENCODE) {
    u8 h[2];
    if(!g.make(out))
      FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
    ogc_partial.set(out, &g);
    h[0] = (u8)api.StreamVersion();
    h[1] = (u8)api.model;
    if(g.writ((void *)OC_MAGIC, sizeof OC_MAGIC - 1) != sizeof OC_MAGIC - 1 || g.writ(h, 2) != 2)
      FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  } else {
    const u8 *h = hd.b + sizeof OC_MAGIC - 1;  // the version, the model
    FATAL_UNLESS(hd.n >= sizeof OC_MAGIC - 1 && !memcmp(hd.b, OC_MAGIC, sizeof OC_MAGIC - 1), "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
    FATAL_UNLESS(hd.n == OC_HEAD, "%s: the stream ends in its header", in);
    FATAL_UNLESS(h[0] == api.StreamVersion(), "%s: made by oggcomp version %u, model %d writes version %d", in, h[0], api.model, api.StreamVersion());
    if(!g.make(out))
      FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
    ogc_partial.set(out, &g);
  }
  p = api.Alloc();
  FATAL_UNLESS(p != nullptr, "model %d is in use", api.model);
  r = api.Init(p, dir, flags, in);
  if(r)
    run_fail(api, p, r);
  k.api = &api;
  k.p = p;
  k.dir = dir;
  k.fed = dir == OC_DECODE ? OC_HEAD : 0;  // (the header, on the .oc side)
  k.written = dir == OC_ENCODE ? OC_HEAD : 0;
  pr.init();
  if(oc_show_progress(o)) {
    pr.start(dir == OC_ENCODE ? "enc" : "dec", dir == OC_DECODE, 0, oc_file_size(in));
    pr.counts(oc_one_counts, &k);
  }
  api.addout(p, obuf, sizeof obuf);
  for(;;) {
    r = api.Loop(p);
    pr.poll();
    if(r == OC_NEED_INPUT) {
      uint n = f.sread(ibuf, sizeof ibuf);
      api.addinp(p, ibuf, (int)n);
      k.fed += n;
      continue;
    }
    if(r == OC_NEED_OUTPUT || r == OC_DONE) {
      int n = api.getoutlen(p);
      if(n > 0 && g.writ(obuf, (uint)n) != (uint)n)
        FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
      k.written += (u64)(n > 0 ? n : 0);
      if(r == OC_DONE)
        break;
      api.addout(p, obuf, sizeof obuf);
      continue;
    }
    run_fail(api, p, r);
  }
  pr.counts(nullptr, nullptr);
  if(f.error())
    FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
  f.close();
#ifdef TC_MEMCOST
  if(dir == OC_ENCODE) {
    u64 bytes = api.TableBytes();
    u64 rent = bytes / ((u64)1 << 30) * TC_MEMCOST + bytes % ((u64)1 << 30) * TC_MEMCOST / ((u64)1 << 30);
    u64 n = rent;
    u8 pad[4096];
    memset(pad, 0xFF, sizeof pad);
    while(n) {
      u32 k = n < sizeof pad ? (u32)n : (u32)sizeof pad;
      g.writ(pad, k);
      n -= k;
    }
    if(flags & OC_F_VERBOSE)
      fprintf(stderr, "%s: %" PRIu64 " MB of tables, %" PRIu64 " bytes of rent\n", ogc_prog, bytes >> 20, rent);
  }
#endif
  {
    int bad = g.error();
    if(g.close())
      bad = 1;
    if(bad)
      FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  }
  ogc_partial.kept();
  {
    oc_counts c;
    oc_one_counts(&k, c);
    pr.done(c);
    pr.quit();
  }
  if(flags & OC_F_VERBOSE)
    api.Report(p);
  api.Free(p);
  return OGC_EXIT_OK;
}

//  ---------------------------------------------------------------------------
//  Runs on threads (oc_mt.inc).

//  A coroutine on the files: its input from f (after what was read of it),
//  its output to g; its progress line polled as it goes.  The last yield:
//  0 done, 3 failed.
template <class C> static uint mt_drive(C &c, filehandle &f, filehandle &g, const char *in, const char *out, oc_progress &pr, u64 &fed, u64 &written) {
  static u8 ibuf[1 << 16], obuf[1 << 16];
  uint y;
  c.addout(obuf, sizeof obuf);
  for(;;) {
    y = c.coro_call(&c);
    pr.poll();
    if(y == 1) {
      uint n = f.sread(ibuf, sizeof ibuf);
      if(f.error())
        FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
      if(!n)
        c.f_quit = 1;
      c.addinp(ibuf, n);
      fed += n;
      continue;
    }
    {
      uint n = c.getoutsize();
      if(n && g.writ(obuf, n) != n)
        FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
      written += n;
    }
    if(y != 2)
      return y;
    c.addout(obuf, sizeof obuf);
  }
}
//  The end of a run's output file.
static void mt_close(filehandle &g, const char *out) {
  int bad = g.error();
  if(g.close())
    bad = 1;
  if(bad)
    FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  ogc_partial.kept();
}
//  After a failure: the threads joined and freed, if all have ended -- as
//  they have unless the splitter failed (out of memory for a part) and left
//  them waiting for more; those end with the process.
template <class M> static void mt_quit_ended(M &m) {
  uint i;
  for(i = 0; i < m.th_N; i++)
    if(!m.C1[i].f_done)
      return;
  m.Quit();
}
//  -v on threads: where the bits went, the workers' added up -- what
//  oggcomp_Report() says of one model.
template <class M> static void mt_report(M &m, oc_api &api, u64 parts) {
  double bits[16] = {0}, tot = 0;
  u64 values[16] = {0};
  int n = 0, i;
  uint w;
  for(w = 0; w < m.th_N; w++) {
    const auto &c = m.C1[w];
    for(i = 0; i < c.nstage; i++) {
      bits[i] += c.bits[i];
      values[i] += c.values[i];
    }
    if(c.nstage > n)
      n = c.nstage;
  }
  if(!n) {
    fprintf(stderr, "%s: model %d does not say where its bits went on threads\n", ogc_prog, api.model);
    return;
  }
  for(i = 0; i < n; i++)
    tot += bits[i];
  fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " parts on %u threads, %" PRIu64 " MB of tables each\n", ogc_prog, tot / 8, parts, m.th_N,
          (u64)(api.TableBytes() >> 20));
  for(i = 0; i < n; i++) {
    fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", m.C1[0].stage[i], bits[i] / 8, tot > 0 ? 100.0 * bits[i] / tot : 0.0);
    if(values[i])
      fprintf(stderr, "  %" PRIu64 " values, %.3f bits each", values[i], bits[i] / (double)values[i]);
    fputc('\n', stderr);
  }
}
//  c -t N: the input in parts, the parts dealt to N workers, each coding
//  its parts with a model of its own.
static int run_mt_c(oc_api &api, const oc_opts &o, filehandle &f, const char *in, const char *out) {
  filehandle g;
  oc_progress pr;
  oc_mt_args a;
  OcMtEnc *e;
  u8 h[MT_HEAD];
  const u64 size = oc_file_size(in), part = o.part ? o.part : mt_part_default(size, o.threads);
  u64 fed = 0, written = MT_HEAD, parts;
  char note[1100] = "internal error: a thread stopped early";
  uint y, w;
  int r;
  if(!g.make(out))
    FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
  ogc_partial.set(out, &g);
  memcpy(h, OC_MT_MAGIC, sizeof OC_MT_MAGIC - 1);
  h[MT_AT_VERSION] = (u8)api.StreamVersion();
  h[MT_AT_MODEL] = (u8)api.model;
  h[MT_AT_THREADS] = (u8)o.threads;
  mt_put_le(h + MT_AT_PART, part >> 10, 4);
  if(g.writ(h, MT_HEAD) != MT_HEAD)
    FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  a.threads = o.threads;
  a.flags = o.flags;
  a.part_max = mt_part_max(part);
  a.api = &api;
  a.meter = &pr;
  a.name = in;
  pr.init();
  e = new(oc_map_block(sizeof(OcMtEnc))) OcMtEnc;
  e->init();
  e->M1.Init(part);
  w = e->M2.Init((uint)o.threads, MT_RING, &a, 1);
  if(w)
    FATAL_CODE(OGC_EXIT_IO, w == 7 ? "cannot start %d threads" : "no memory for the buffers of %d threads", o.threads);
  if(oc_show_progress(o)) {
    pr.start("enc", 0, o.threads, size);
    pr.counts(mt_counts<OcMtWorkers>, &e->M2);
  }
  y = mt_drive(*e, f, g, in, out, pr, fed, written);
  pr.counts(nullptr, nullptr);
  if((r = mt_failed(e->M2, note, sizeof note)) || y != 0) {
    if(!r)
      r = OGC_EXIT_INTERNAL;
    mt_quit_ended(e->M2);
    if(e->M1.fail)
      ogc_fatal_code(OGC_EXIT_IO, "out of memory for the parts of %s", in);
    ogc_fatal_code(r, "%s: %s", in, note);
  }
  parts = e->M1.parts;
  e->M2.Quit();  // (the threads joined, the workers freed)
  e->M1.Quit();
  f.close();
  mt_close(g, out);
  {
    oc_counts c;
    c.raw = e->M1.total;
    c.packed = written;
    c.parts = parts;
    pr.done(c);
    pr.quit();
  }
  if(o.flags & OC_F_VERBOSE)
    mt_report(e->M2, api, parts);
  oc_unmap_block(e, sizeof(OcMtEnc));
  return OGC_EXIT_OK;
}
//  d of what c -t N wrote: the threads from its header, each worker
//  decoding the parts c's worker of its place coded, in turn.  f is past
//  the header's first bytes, in hd.
static int run_mt_d(oc_api &api, const oc_opts &o, filehandle &f, const oc_head &hd, const char *in, const char *out) {
  filehandle g;
  oc_progress pr;
  oc_mt_args a;
  OcMtDec *d;
  u8 h[MT_HEAD];
  u64 fed = MT_HEAD, written = 0, part, parts = 0;
  char note[1100] = "damaged data";
  int threads, r;
  uint y, w;
  memcpy(h, hd.b, OC_HEAD);
  FATAL_UNLESS(f.sread(h + OC_HEAD, MT_HEAD - OC_HEAD) == MT_HEAD - OC_HEAD, "%s: the stream ends in its header", in);
  FATAL_UNLESS(h[MT_AT_VERSION] == api.StreamVersion(), "%s: made by oggcomp version %u, model %d writes version %d", in, h[MT_AT_VERSION], api.model, api.StreamVersion());
  threads = h[MT_AT_THREADS];
  part = mt_get_le(h + MT_AT_PART, 4) << 10;
  FATAL_UNLESS(threads >= 1 && threads <= OC_MT_MAX && part, "%s: damaged data: its header is broken", in);
  if(!g.make(out))
    FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
  ogc_partial.set(out, &g);
  a.threads = threads;
  a.flags = o.flags & ~OC_F_VERBOSE;
  a.part_max = mt_part_max(part);
  a.api = &api;
  a.meter = &pr;
  a.name = in;
  pr.init();
  d = new(oc_map_block(sizeof(OcMtDec))) OcMtDec;
  w = d->Init((uint)threads, MT_RING, &a, 1);
  if(w)
    FATAL_CODE(OGC_EXIT_IO, w == 7 ? "cannot start %d threads" : "no memory for the buffers of %d threads", threads);
  if(oc_show_progress(o)) {
    pr.start("dec", 1, threads, oc_file_size(in));
    pr.counts(mt_counts<OcMtDec>, d);
  }
  y = mt_drive(*d, f, g, in, out, pr, fed, written);
  pr.counts(nullptr, nullptr);
  if(!(r = mt_failed(*d, note, sizeof note)) && (y != 0 || !mt_dealt(*d)))
    r = OGC_EXIT_REFUSED;
  if(r) {
    mt_quit_ended(*d);
    ogc_fatal_code(r, "%s: %s", in, note);
  }
  for(w = 0; w < d->th_N; w++)
    parts += d->C1[w].parts;
  d->Quit();
  f.close();
  mt_close(g, out);
  {
    oc_counts c;
    c.raw = written;
    c.packed = fed;
    c.parts = parts;
    pr.done(c);
    pr.quit();
  }
  oc_unmap_block(d, sizeof(OcMtDec));
  return OGC_EXIT_OK;
}

//  -t N, -c N: the number after the option letter at o, or in the next
//  argument; 0 when there is none.
static const char *opt_value(const char *o, int &a, int argc, char **argv) {
  if(o[1])
    return o + 1;
  if(a + 1 < argc)
    return argv[++a];
  return nullptr;
}
int main(int argc, char **argv) {
  int a = 1, model = 0, have_model = 0, dir, mt = 0;
  oc_opts o;
  const char *mode, *in, *out;
  oc_api api;
  filehandle f;
  oc_head hd;
  memset(&o, 0, sizeof o);
  o.threads = 1;
  if(argc < 4)
    goto usage;
  mode = argv[a++];
  if((mode[0] != 'c' && mode[0] != 'd') || mode[1])
    goto usage;
  for(; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char *s = argv[a] + 1, *v;
    char *e;
    unsigned long n;
    for(; *s; s++)
      switch(*s) {
      case 'v':
        o.flags |= OC_F_VERBOSE;
        break;
      case 'H':
        o.flags |= OC_F_HUGE;
        break;
      case 'p':
        o.progress = 1;
        break;
      case 'q':
        o.quiet = 1;
        break;
      case 't':
      case 'c':
        v = opt_value(s, a, argc, argv);
        if(!v)
          goto usage;
        n = strtoul(v, &e, 10);
        if(*s == 't') {
          if(*e || e == v || n < 1 || n > (unsigned long)OC_MT_MAX)
            FATAL_CODE(OGC_EXIT_USAGE, "-t takes 1 to %d threads", OC_MT_MAX);
          o.threads = (int)n;
        } else {
          u64 unit = (u64)1 << 20;
          if(*e == 'k' || *e == 'K')
            unit = 1024, e++;
          if(*e || e == v || n < 1 || n > 1024 * (((u64)1 << 20) / unit))
            FATAL_CODE(OGC_EXIT_USAGE, "-c takes a part size of 1 to 1024 MB, or in KB with a k");
          o.part = (u64)n * unit;
        }
        s = " ";  // (the rest of the argument was the number)
        break;
      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        model = *s - '0';
        have_model = 1;
        break;
      default:
        goto usage;
      }
  }
  if(argc - a != 2)
    goto usage;
  ogc_set_prog(argv[0]);
  ogc_paths_distinct(argv + a, 2);
  dir = mode[0] == 'c' ? OC_ENCODE : OC_DECODE;
  in = argv[a];
  out = argv[a + 1];
  if(!f.open(in))
    FATAL_CODE(OGC_EXIT_IO, "cannot open %s", in);
  hd.n = 0;
  if(dir == OC_DECODE) {
    //  The stream names its model in its header; -N given must agree.  Its
    //  magic says whether it was coded on threads.
    hd.n = f.sread(hd.b, OC_HEAD);
    if(hd.n == OC_HEAD) {
      mt = !memcmp(hd.b, OC_MT_MAGIC, sizeof OC_MT_MAGIC - 1);
      if(mt || !memcmp(hd.b, OC_MAGIC, sizeof OC_MAGIC - 1)) {
        if(have_model)
          FATAL_UNLESS(hd.b[OC_HEAD - 1] == model, "%s was coded with model %d, and -%d asks for another", in, hd.b[OC_HEAD - 1], model);
        model = hd.b[OC_HEAD - 1];
      }
    }
  }
  oc_api_load(&api, model);
  if(dir == OC_DECODE && mt)
    return run_mt_d(api, o, f, hd, in, out);
  if(dir == OC_ENCODE && o.threads > 1)
    return run_mt_c(api, o, f, in, out);
  return run(api, dir, o, f, hd, in, out);
usage:
  fprintf(stderr, "Lossless compressor of Ogg Vorbis files.\n"
                  "usage: oggcomp c [options] input.ogg output.oc\n"
                  "       oggcomp d [options] input.oc  restored.ogg\n"
                  "\n"
                  "  -v    say where the bits went, by stage\n"
                  "  -H    model tables on huge pages (Linux; faster on big files where the\n"
                  "        kernel has 2 MB pages ready, slower where it has to make them)\n"
                  "  -N    model N: 0 the program's own, 1..9 oggcompN.dll (.so) beside it,\n"
                  "        built from an IDX model of its own by ./mk.sh dll N.  c writes N\n"
                  "        into the stream; d reads it back, and -N there must agree\n"
                  "  -t N  (c) code on N threads, 1 to %d: the input in parts, cut where\n"
                  "        oggsplit cuts, each thread coding its parts with a model of its\n"
                  "        own; d takes N from the stream\n"
                  "  -c N  (c with -t) the size of a part, at least: N MB (1 to 1024), or N KB\n"
                  "        with a k (default: a part for each thread, 1 MB at most)\n"
                  "  -p    the progress line even where stderr is not a terminal\n"
                  "  -q    no progress line\n"
                  "\n",
          OC_MT_MAX);
  return OGC_EXIT_USAGE;
}
