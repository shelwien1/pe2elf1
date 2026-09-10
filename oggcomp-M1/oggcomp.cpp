#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
//  Past the end of the input the pin hands back -1, which as a byte is
//  the 0xFF the encoder's flush left off; a few of those are the tail of
//  every stream.  Many more is a stream cut short, and the decoder would
//  otherwise go on turning them into output for as long as they came --
//  a cut in the middle of a run of bytes has no parser check to stop it.
static const char *oc_in;
static u32 rc_over;
constexpr u32 RC_OVER_MAX = 48;
template <class RC> struct rc_pin_io {
  Coroutine *co;
  byte get() {
    uint c = co->pin[0].get();
    if(UNLIKELY(c == uint(-1))) {
      FATAL_UNLESS(++rc_over <= RC_OVER_MAX, "%s: cut short -- the stream ends before its last symbol", oc_in);
      return 0xFF;
    }
    return (byte)c;
  }
  void put(byte c) { co->pin[1].put(c); }
};
#define RC_IO_BASE rc_pin_io
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
constexpr char OC_MAGIC[] = "oggc\x1a";
constexpr int OC_VER = 3;
//  The coroutine, one per direction: f_DEC is what Rangecoder calls it, 0
//  to code and 1 to decode.  Each direction's walk is a member function
//  chosen by overload on the direction's tag, and a member function of a
//  class template is compiled only where it is called, so the decoder's
//  binary path holds no dry run and the encoder's no page writer.
template <int N> struct oc_dir {};
template <int f_DEC> struct oc_coro : Coroutine {
  oc_model<f_DEC> t;
  const char *in;
  void go(oc_dir<0>) {
    source src;
    src.open(in, &pin[0]);
    t.rc.co = this;
    t.rc.rc_Init();
    {
      og_packer<oc_model<f_DEC>> pk(t, src);
      pk.pack();
    }
    t.rc.rc_Quit();
    if(t.rc.carry_lost())
      FATAL_CODE(OGC_EXIT_INTERNAL,
                 "%s: the carryless coder lost a carry; "
                 "this is a measuring build, ship one with OC_CARRYLESS=0",
                 in);
    src.close();
  }
  void go(oc_dir<1>) {
    t.rc.co = this;
    t.rc.rc_Init();
    og_unpacker<oc_model<f_DEC>> up(t, &pin[1]);
    up.unpack(in);
  }
  void do_process() {
    go(oc_dir<f_DEC>());
    yield(this, 0);
  }
};
//  One run, in the direction f_DEC: the tables and the model, the files, the
//  coroutine over them, the report.
template <int f_DEC> static int run(const char *in, const char *out) {
  static CoroFileProc<oc_coro<f_DEC>> co;
  filehandle f, g;
  co.t.init();
  if(!f.open(in))
    FATAL_CODE(OGC_EXIT_IO, "cannot open %s", in);
  if(!f_DEC) {
    u8 h[2];
    if(!g.make(out))
      FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
    ogc_partial.set(out, &g);
    h[0] = (u8)OC_VER;
    h[1] = 0;
    if(g.writ((void *)OC_MAGIC, sizeof OC_MAGIC - 1) != sizeof OC_MAGIC - 1 || g.writ(h, 2) != 2)
      FATAL_CODE(OGC_EXIT_IO, "write error on %s", out);
  } else {
    char m[sizeof OC_MAGIC - 1];
    u8 h[2];
    FATAL_UNLESS(f.read(m, sizeof m) == sizeof m && !memcmp(m, OC_MAGIC, sizeof m), "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
    FATAL_UNLESS(f.read(h, 2) == 2, "%s: the stream ends in its header", in);
    FATAL_UNLESS(h[0] == OC_VER, "%s: made by oggcomp version %u, this is version %u", in, h[0], OC_VER);
    if(!g.make(out))
      FATAL_CODE(OGC_EXIT_IO, "cannot create %s", out);
    ogc_partial.set(out, &g);
  }
  oc_in = in;
  co.in = in;
  co.processfile(f, g);
  if(f.error())
    FATAL_CODE(OGC_EXIT_IO, "read error on %s", in);
  f.close();
#ifdef TC_MEMCOST
  if(!f_DEC) {
    u64 rent = tabs.bytes() / ((u64)1 << 30) * TC_MEMCOST + tabs.bytes() % ((u64)1 << 30) * TC_MEMCOST / ((u64)1 << 30);
    u64 n = rent;
    u8 pad[4096];
    memset(pad, 0xFF, sizeof pad);
    while(n) {
      u32 k = n < sizeof pad ? (u32)n : (u32)sizeof pad;
      g.writ(pad, k);
      n -= k;
    }
    if(tc_verbose)
      fprintf(stderr, "%s: %" PRIu64 " MB of tables, %" PRIu64 " bytes of rent\n", ogc_prog, tabs.bytes() >> 20, rent);
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
  if(tc_verbose) {
    int i;
    double tot = 0;
    for(i = 0; i < STG_N; i++)
      tot += tc_bits[i];
    fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " MB of tables\n", ogc_prog, tot / 8, (u64)(tabs.bytes() >> 20));
    if(!f_DEC)
      for(i = 0; i < STG_N; i++) {
        fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i], tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
        if(tc_syms[i])
          fprintf(stderr, "  %" PRIu64 " values, %.3f bits each", tc_syms[i], tc_bits[i] / (double)tc_syms[i]);
        fputc('\n', stderr);
      }
  }
  hist.reset();
  tcp.reset();
  return OGC_EXIT_OK;
}
int main(int argc, char **argv) {
  int a = 1;
  const char *mode;
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
        tc_verbose = 1;
        break;
      case 'H':
        tc_huge = 1;
        break;
      default:
        goto usage;
      }
  }
  if(argc - a != 2)
    goto usage;
  ogc_set_prog(argv[0]);
  ogc_paths_distinct(argv + a, 2);
  return mode[0] == 'c' ? run<0>(argv[a], argv[a + 1]) : run<1>(argv[a], argv[a + 1]);
usage:
  fprintf(stderr, "Lossless compressor of Ogg Vorbis files.\n"
                  "usage: oggcomp c [options] input.ogg output.oc\n"
                  "       oggcomp d [options] input.oc  restored.ogg\n"
                  "\n"
                  "  -v  say where the bits went, by stage\n"
                  "  -H  model tables on huge pages (Linux; faster on big files where the\n"
                  "      kernel has 2 MB pages ready, slower where it has to make them)\n"
                  "\n");
  return OGC_EXIT_USAGE;
}
