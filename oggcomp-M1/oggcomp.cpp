#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
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
//  A run the model refused or could not do: its message, the output
//  dropped, the exit status the error names.
static NORETURN void run_fail(oc_api &api, void *p, int r) {
  fprintf(stderr, "%s: %s\n", ogc_prog, api.Error(p));
  die(r >= 5 && r <= 8 ? r - 4 : OGC_EXIT_REFUSED);
}
//  One run through whichever model the table holds -- the program's own or
//  a library's, the same code either way: the header, then the files
//  through oc_api.h's loop, then the report.
static int run(oc_api &api, int dir, int flags, const char *in, const char *out) {
  static u8 ibuf[1 << 16], obuf[1 << 16];
  filehandle f, g;
  void *p;
  int r;
  if(!f.open(in))
    FATAL_CODE(OGC_EXIT_IO, "cannot open %s", in);
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
    char m[sizeof OC_MAGIC - 1];
    u8 h[2];
    FATAL_UNLESS(f.read(m, sizeof m) == sizeof m && !memcmp(m, OC_MAGIC, sizeof m), "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
    FATAL_UNLESS(f.read(h, 2) == 2, "%s: the stream ends in its header", in);
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
  api.addout(p, obuf, sizeof obuf);
  for(;;) {
    r = api.Loop(p);
    if(r == OC_NEED_INPUT) {
      uint n = f.sread(ibuf, sizeof ibuf);
      api.addinp(p, ibuf, (int)n);
      continue;
    }
    if(r == OC_NEED_OUTPUT || r == OC_DONE) {
      int n = api.getoutlen(p);
      if(n > 0 && g.writ(obuf, (uint)n) != (uint)n)
        FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
      if(r == OC_DONE)
        break;
      api.addout(p, obuf, sizeof obuf);
      continue;
    }
    run_fail(api, p, r);
  }
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
  if(flags & OC_F_VERBOSE)
    api.Report(p);
  api.Free(p);
  return OGC_EXIT_OK;
}
int main(int argc, char **argv) {
  int a = 1, flags = 0, model = 0, have_model = 0, dir;
  const char *mode;
  oc_api api;
  if(argc < 4)
    goto usage;
  mode = argv[a++];
  if((mode[0] != 'c' && mode[0] != 'd') || mode[1])
    goto usage;
  for(; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char *o = argv[a] + 1;
    for(; *o; o++)
      switch(*o) {
      case 'v':
        flags |= OC_F_VERBOSE;
        break;
      case 'H':
        flags |= OC_F_HUGE;
        break;
      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        model = *o - '0';
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
  if(dir == OC_DECODE) {
    //  The stream names its model in its header; -N given must agree.
    filehandle f;
    u8 h[sizeof OC_MAGIC + 1];
    if(f.open(argv[a])) {
      if(f.read(h, sizeof h) == sizeof h && !memcmp(h, OC_MAGIC, sizeof OC_MAGIC - 1)) {
        if(have_model)
          FATAL_UNLESS(h[sizeof OC_MAGIC] == model, "%s was coded with model %d, and -%d asks for another", argv[a], h[sizeof OC_MAGIC], model);
        model = h[sizeof OC_MAGIC];
      }
      f.close();
    }
  }
  oc_api_load(&api, model);
  return run(api, dir, flags, argv[a], argv[a + 1]);
usage:
  fprintf(stderr, "Lossless compressor of Ogg Vorbis files.\n"
                  "usage: oggcomp c [options] input.ogg output.oc\n"
                  "       oggcomp d [options] input.oc  restored.ogg\n"
                  "\n"
                  "  -v  say where the bits went, by stage\n"
                  "  -H  model tables on huge pages (Linux; faster on big files where the\n"
                  "      kernel has 2 MB pages ready, slower where it has to make them)\n"
                  "  -N  model N: 0 the program's own, 1..9 oggcompN.dll (.so) beside it,\n"
                  "      built from an IDX model of its own by ./mk.sh dll N.  c writes N\n"
                  "      into the stream; d reads it back, and -N there must agree\n"
                  "\n");
  return OGC_EXIT_USAGE;
}
