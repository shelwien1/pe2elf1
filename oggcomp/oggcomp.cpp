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
//  Whether the paths a and b are one file that exists: one device and
//  inode, or on Windows one volume and file index.  ogc_paths_distinct
//  compares names, and `oggcomp c a.ogg ./a.ogg` -- or a full path, a link,
//  another case on Windows -- got past it: the output's creation emptied
//  the input before a byte of it was read, and the run exited 0, a.ogg now
//  the .oc of nothing.
static int ogc_same_file(const char *a, const char *b) {
  if(ogc_is_stdio(a) || ogc_is_stdio(b))
    return 0;
#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION x, y;
  int same = 0;
  HANDLE ha = CreateFileA(a, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  HANDLE hb = CreateFileA(b, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if(ha != INVALID_HANDLE_VALUE && hb != INVALID_HANDLE_VALUE && GetFileInformationByHandle(ha, &x) && GetFileInformationByHandle(hb, &y))
    same = x.dwVolumeSerialNumber == y.dwVolumeSerialNumber && x.nFileIndexHigh == y.nFileIndexHigh && x.nFileIndexLow == y.nFileIndexLow;
  if(ha != INVALID_HANDLE_VALUE)
    CloseHandle(ha);
  if(hb != INVALID_HANDLE_VALUE)
    CloseHandle(hb);
  return same;
#else
  struct stat x, y;
  return !stat(a, &x) && !stat(b, &y) && x.st_dev == y.st_dev && x.st_ino == y.st_ino;
#endif
}
//  The head of what d is given, as main() reads it to learn the level and
//  whether the stream was coded on threads: the magic, the version, the
//  level -- the whole header of a stream of one model, the first 7 bytes of
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
    h[1] = (u8)api.level;
    if(g.writ((void *)OC_MAGIC, sizeof OC_MAGIC - 1) != sizeof OC_MAGIC - 1 || g.writ(h, 2) != 2)
      FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  } else {
    const u8 *h = hd.b + sizeof OC_MAGIC - 1;  // the version, the level
    FATAL_UNLESS(hd.n >= sizeof OC_MAGIC - 1 && !memcmp(hd.b, OC_MAGIC, sizeof OC_MAGIC - 1), "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
    FATAL_UNLESS(hd.n == OC_HEAD, "%s: the stream ends in its header", in);
    FATAL_UNLESS(h[0] == api.StreamVersion(), "%s: made by oggcomp's stream version %u, and this one reads version %d", in, h[0], api.StreamVersion());
    if(!g.make(out))
      FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
    ogc_partial.set(out, &g);
  }
  p = api.Alloc();
  FATAL_UNLESS(p != nullptr, "the thread's instance of the model is in use");
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
      //  As it happens: a read error reads as the end, and a decoder given
      //  an end too soon refuses the stream as cut short -- exit 1 for what
      //  the filesystem did, where the promise is 3.
      if(f.error())
        FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
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
template <class M> static void mt_report(M &m, u64 parts) {
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
    fprintf(stderr, "%s: no worker counted where its bits went\n", ogc_prog);
    return;
  }
  for(i = 0; i < n; i++)
    tot += bits[i];
  //  (the tables' size as a worker had it: on the main thread, which runs
  //  no model, oggcomp_TableBytes() is 0)
  fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " part%s on %u threads, %" PRIu64 " MB of tables each\n", ogc_prog, tot / 8, parts, parts == 1 ? "" : "s", m.th_N,
          (u64)(m.C1[0].tables >> 20));
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
  h[MT_AT_LEVEL] = (u8)api.level;
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
  //  -v reads the workers, so before Quit() frees them -- after it, as it
  //  was, it read freed memory and `c -v -t N` died there.  The workers
  //  have all ended by now: MTwrap's loop ends with the last of them.
  if(o.flags & OC_F_VERBOSE)
    mt_report(e->M2, parts);
  e->M2.Quit();  // (the threads joined, the workers freed)
  e->M1.Quit();
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
  FATAL_UNLESS(h[MT_AT_VERSION] == api.StreamVersion(), "%s: made by oggcomp's stream version %u, and this one reads version %d", in, h[MT_AT_VERSION], api.StreamVersion());
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
//  The level a header's level byte names: the level itself -- 9, 7, 3, 1 --
//  or, from a stream made before there were levels, the number of the model
//  that coded it, which the levels are: 0, the program's own, is -9; 2, the
//  oggcomp2 library, is -7; 1 and 3 kept their numbers.  -1 for a byte that
//  names none.
static int oc_level_of(int b) {
  switch(b) {
  case 0:
  case 9:
    return 9;
  case 2:
  case 7:
    return 7;
  case 3:
    return 3;
  case 1:
    return 1;
  }
  return -1;
}
//  " -9 -7 -3 -1", the levels this build has, for a message.
static const char *oc_levels_list(void) {
  static char b[32];
  int k = 0;
  for(int l : oc_levels)
    k += snprintf(b + k, sizeof b - (sz)k, " -%d", l);
  return b;
}
int main(int argc, char **argv) {
  int a = 1, level = 0, have_level = 0, dir, mt = 0;
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
        level = *s - '0';
        have_level = 1;
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
  if(ogc_same_file(in, out))
    FATAL_CODE(OGC_EXIT_USAGE, "%s and %s are one file: the output would be written over the input", in, out);
  if(!f.open(in))
    FATAL_CODE(OGC_EXIT_IO, "cannot open %s", in);
  hd.n = 0;
  if(dir == OC_ENCODE) {
    //  c at -N, or at the strongest level the build has.
    if(!have_level)
      level = OC_LEVEL_DEFAULT;
    if(!oc_has_level(level))
      FATAL_CODE(OGC_EXIT_USAGE, "this build has no level -%d (it has%s)", level, oc_levels_list());
  } else {
    //  The stream names its level in its header, and d decodes at that
    //  level; -N given must agree.  Its magic says whether it was coded on
    //  threads.
    hd.n = f.sread(hd.b, OC_HEAD);
    if(f.error())
      FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
    if(hd.n == OC_HEAD) {
      mt = !memcmp(hd.b, OC_MT_MAGIC, sizeof OC_MT_MAGIC - 1);
      if(mt || !memcmp(hd.b, OC_MAGIC, sizeof OC_MAGIC - 1)) {
        const int hl = oc_level_of(hd.b[OC_HEAD - 1]);
        //  A byte that names no level was not written by `oggcomp c`:
        //  damage, exit 1.
        FATAL_UNLESS(hl > 0, "%s: damaged data: its header names no level (%d)", in, hd.b[OC_HEAD - 1]);
        if(have_level)
          FATAL_UNLESS(hl == level, "%s was coded at level -%d, and -%d asks for another", in, hl, level);
        FATAL_UNLESS(oc_has_level(hl), "%s was coded at level -%d, which this build has not (it has%s)", in, hl, oc_levels_list());
        level = hl;
      }
    }
    if(!level)
      level = OC_LEVEL_DEFAULT;  // (not a stream: run() says so)
  }
  o.flags |= OC_F_LEVEL(level);
  oc_api_load(&api, level);
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
                  "  -9 -7 -3 -1  (c) the level: -9, the default, codes best; -7 is nearly\n"
                  "        as strong and the fastest.  d takes the level from the stream,\n"
                  "        and -N given there must agree\n"
                  "  -t N  (c) code on N threads, 1 to %d: the input in parts, cut where\n"
                  "        oggsplit cuts, each thread coding its parts with a model of its\n"
                  "        own; d takes N from the stream\n"
                  "  -c N  (c with -t) the size of a part, at least: N MB (1 to 1024), or N KB\n"
                  "        with a k (default: four parts for each thread, of 64 KB to 4 MB;\n"
                  "        1 MB where the input's size is not known)\n"
                  "  -p    the progress line even where stderr is not a terminal\n"
                  "  -q    no progress line\n"
                  "\n",
          OC_MT_MAX);
  return OGC_EXIT_USAGE;
}
