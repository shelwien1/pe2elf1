/*  oggcomp -- an Ogg Vorbis file, compressed.

      oggcomp c [options] input.ogg output.oc
      oggcomp d [options] input.oc  restored.ogg

    balrogg takes an Ogg Vorbis stream apart into a record stream and
    tsvcomp models that stream; this is the two in one pass, with nothing
    materialised between them.  The walk is balrogg's -- io.inc over the
    bits of each packet, link_walk.inc and codec.inc over the pages -- and
    at every point where a value crosses it is handed to the model, with
    the context the walk has there, through the sink in oc_sink.inc.  The
    model is tsvcomp's, in oc_model.inc, and what it codes is byte for byte
    what tsvcomp coded from balrogg's records.  OGGCOMP-PLAN.md is the
    design; TSVCOMP-MODEL.md is the model.

    Both directions stream: a page is parsed where it lies in a forward
    window over the input, each packet is coded as it is taken apart, and
    the coder's bytes go out as they are made.  Nothing is held past one
    page, one packet gathered across pages, the link's setup and the model's
    own histories.

    The streaming is Lib3's: the walk runs as a coroutine (Lib3/coro3b.inc)
    with the input on one pin and the output on the other, and
    CoroFileProc (Lib3/coro_fhp2.inc) drives it over the two files, 64 KB
    at a time.  The coroutine itself is below.
    OGGCOMP-PLAN.md section 11 is that port and what it measured.  */

#include <math.h>
#include <new>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define DEV_NULL "nul"
  #include <windows.h>
#else
  #define DEV_NULL "/dev/null"
  #include <sys/mman.h>
#endif

#include "common.inc"      /*  types, diagnostics, loop macros  */
#include "ogg_page.inc"    /*  page framing  */

#include "vb_info.inc"     /*  the Vorbis identification header  */
#include "vb_book.inc"     /*  codebooks, and the arena their bodies live in  */
#include "vb_floor.inc"
#include "vb_res.inc"
#include "vb_map.inc"
#include "vb_setup.inc"    /*  the four lists a link's audio runs on  */
#include "vb_ctx.inc"      /*  what survives from one packet to the next  */

/*  Lib3, the framework this streams through: coro3b.inc's Coroutine, a
    stackful coroutine on setjmp/longjmp with an input pin and an output
    pin, and coro_fhp2.inc's CoroFileProc, which drives one over a pair of
    files -- 64 KB read into the input pin whenever the coroutine has
    emptied it, 64 KB written out whenever it has filled the output pin.
    The files are as supplied, but for coro3b.inc, where the coroutine's
    stack size can be set from outside, yield refuses a call chain deeper
    than it can save and measures the stack from its own frame pointer,
    and the clang setjmp clobbers the register it takes its buffer in.
    The names Lib3 spells its own way are in common.inc.  */
#define FILE_API_STD 1     /*  the stdio file API on every platform: main looks
                               at ferror() through it, and the raw Win32 one
                               Lib3 would otherwise pick has nothing to look at  */
#include "Lib3/file_api.inc"
#include "Lib3/coro3b.inc"
/*  CoroFileProc counts the input bytes it has handed over here, for a
    progress meter psrc has and this program does not.  */
static qword g_prog_in;
#include "Lib3/coro_fhp2.inc"

/*  The range coder's bytes -- rc.inc's RC_IO_BASE -- on the coroutine's
    pins: got from the input pin, put on the output pin.  Lib3's get()
    returns uint(-1) once the driver has said the input is done; as a byte
    that is 0xFF, forever, which is what the coder's flush counts on.  */
template <class RC>
struct rc_pin_io {
  Coroutine * co;
  byte get() { return (byte) co->pin[0].get(); }
  void put(byte c) { co->pin[1].put(c); }
};
#define RC_IO_BASE rc_pin_io
#include "rc.inc"          /*  the range coder  */
#include "cm.inc"          /*  counter, APM, mixer  */
#include "sh_mapping.inc"  /*  IDX runtime: mapping, masking, pdesc  */

#include "oc_sink.inc"     /*  what the walk hands each value to  */
#include "oc_model.inc"    /*  ... here, the model  */
typedef oc_model sink_t;

#include "io.inc"          /*  one packet, taken apart or rebuilt  */
#include "source.inc"      /*  the forward window over the input  */
#include "link_walk.inc"   /*  packets across the pages of one link  */
#include "codec.inc"       /*  the link and page walk  */

/*  The container: a magic, a version, a spare byte, and the coder's bytes.
    The parameters compiled in through MOD/ are part of the format -- a
    stream decodes under a build with the same MOD/, and no other.  Version
    2 is rc.inc's coder; version 1 was sh_v2f.inc's, over the same model.  */
constexpr char OC_MAGIC[] = "oggc\x1a";
constexpr int  OC_VER = 2;


/*  The coroutine.  Its body is the walk -- vb_pack or vb_unpack, whichever
    direction -- with the coder and the input window reading and writing the
    coroutine's pins, and yielding to the driver whenever a pin runs dry or
    fills.  CoroFileProc is the driver: 64 KB read, the coroutine run until
    it wants more or has 64 KB to write, 64 KB written, round again.
    Encoding, the walk pulls the .ogg through the input pin into source's
    window and the coder puts its bytes on the output pin; decoding, the
    coder gets its bytes from the input pin and each rebuilt page goes out
    on the output pin.  Nothing else changes hands: the container header is
    the driver's caller's, before and after.  */
struct oc_coro : Coroutine {
  oc_model t;
  const char * in;           /*  the input's name, for messages  */

  /*  source's pull: what the input pin holds, or wait for the driver to
      refill it; 0 once the driver has said the input is done.  */
  static sz pull(void * ctx, u8 * dst, sz n) {
    coro3_pin & p = ((oc_coro *) ctx)->pin[0];
    for (;;) {
      sz have = (sz) (p.end - p.ptr);
      if (have) {
        if (have > n) have = n;
        memcpy(dst, p.ptr, have);  p.ptr += have;
        return have;
      }
      if (p.f_quit()) return 0;
      p.yield_r();
    }
  }

  /*  vb_unpack's page writer: onto the output pin, the driver draining it
      as often as it fills.  */
  static void push(void * ctx, const u8 * s, sz n) {
    coro3_pin & p = ((oc_coro *) ctx)->pin[1];
    while (n) {
      sz room = (sz) (p.end - p.ptr);
      if (!room) { p.yield_r();  continue; }
      if (room > n) room = n;
      memcpy(p.ptr, s, room);  p.ptr += room;  s += room;  n -= room;
    }
  }

  void do_process() {
    if (tc_enc) {
      source src;
      src.open(in, pull, this);
      rce.co = this;  rce.rc_Init();
      vb_pack(t, src);
      rce.rc_Quit();
      if (rce.carry_lost())
        FATAL_CODE(BLR_EXIT_INTERNAL, "%s: the carryless coder lost a carry; "
                   "this is a measuring build, ship one with OC_CARRYLESS=0", in);
      src.close();
    } else {
      rcd.co = this;  rcd.rc_Init();
      vb_unpack(t, in, push, this);
    }
    yield(this, 0);          /*  done: the driver writes what is left and stops  */
  }
};

/*  Static rather than main's: the driver's two 64 KB buffers and the
    coroutine's stack copy, under the 256 KB pad the coroutine puts below
    its caller's frame.  */
static CoroFileProc<oc_coro> oc_run;


int main(int argc, char ** argv) {
  int a = 1;
  const char * mode;
  if (argc < 4) goto usage;
  mode = argv[a++];
  if ((mode[0] != 'c' && mode[0] != 'd') || mode[1]) goto usage;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char * o = argv[a] + 1;
    for (; *o; o++)
      switch (*o) {
        case 'v': tc_verbose = 1;  break;
        case 'H': tc_huge = 1;  break;
        default: goto usage;
      }
  }
  if (argc - a != 2) goto usage;
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + a, 2);
  tc_enc = mode[0] == 'c';
  std::set_new_handler(tc_nomem);
  tc_models();
  { const char * in = argv[a], * out = argv[a + 1];
    filehandle f, g;
    if (!f.open(in)) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", in);
    if (tc_enc) {
      u8 h[2];
      if (!g.make(out)) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
      blr_output(out);
      h[0] = (u8) OC_VER;  h[1] = 0;
      if (g.writ((void *) OC_MAGIC, sizeof OC_MAGIC - 1) != sizeof OC_MAGIC - 1 ||
          g.writ(h, 2) != 2)
        FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
    } else {
      char m[sizeof OC_MAGIC - 1];
      u8 h[2];
      FATAL_UNLESS(f.read(m, sizeof m) == sizeof m &&
                   !memcmp(m, OC_MAGIC, sizeof m),
                   "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
      FATAL_UNLESS(f.read(h, 2) == 2,
                   "%s: the stream ends in its header", in);
      FATAL_UNLESS(h[0] == OC_VER,
                   "%s: made by oggcomp version %u, this is version %u",
                   in, h[0], OC_VER);
      if (!g.make(out)) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
      blr_output(out);
    }
    /*  The walk, from the header on, and everything it produces.  */
    oc_run.in = in;
    oc_run.processfile(f, g);
    /*  The driver takes a short read as the end of the input; whether it
        was is known here.  */
    if (ferror(f.f)) FATAL_CODE(BLR_EXIT_IO, "read error on %s", in);
    f.close();
#ifdef TC_MEMCOST
    /*  The model's rent, paid where the optimizer is looking.  */
    if (tc_enc) {
      u64 n = tc_tables() / ((u64) 1 << 30) * TC_MEMCOST
            + tc_tables() % ((u64) 1 << 30) * TC_MEMCOST / ((u64) 1 << 30);
      u64 i;
      for (i = 0; i < n; i++) fputc(0xFF, g.f);
      if (tc_verbose)
        fprintf(stderr, "%s: %" PRIu64 " MB of tables, %" PRIu64 " bytes of rent\n",
                blr_prog, tc_tables() >> 20, n);
    }
#endif
    { int bad = ferror(g.f);
      if (g.close()) bad = 1;
      if (bad) FATAL_CODE(BLR_EXIT_IO, "write error on %s", out); }
    blr_output_kept(); }
  if (tc_verbose) {
    int i;
    double tot = 0;
    for (i = 0; i < STG_N; i++) tot += tc_bits[i];
    fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " MB of tables\n",
            blr_prog, tot / 8, (u64) (tc_tables() >> 20));
    if (tc_enc)
      for (i = 0; i < STG_N; i++) {
        fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i],
                tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
        if (tc_syms[i])
          fprintf(stderr, "  %" PRIu64 " values, %.3f bits each",
                  tc_syms[i], tc_bits[i] / (double) tc_syms[i]);
        fputc('\n', stderr);
      }
  }
  tc_hist_free();
  tcp_free();
  return BLR_EXIT_OK;
usage:
  fprintf(stderr,
    "usage: oggcomp c [options] input.ogg output.oc\n"
    "       oggcomp d [options] input.oc  restored.ogg\n"
    "\n"
    "Lossless compression of an Ogg Vorbis file.  The walk is balrogg's;\n"
    "the model is tsvcomp's, with its contexts declared in IDX/tsvcomp-*.idx\n"
    "and compiled in through MOD/.\n"
    "\n"
    "  -v  say where the bits went, by stage\n"
    "  -H  model tables on huge pages (Linux; faster on big files where the\n"
    "      kernel has 2 MB pages ready, slower where it has to make them)\n"
    "\n"
    "The contexts and the rates are part of the format -- a stream is\n"
    "decodable by a build with the same MOD/, and by no other.  ./mk.sh\n"
    "regenerates MOD/ and IDX/opt.pl tunes it.\n");
  return BLR_EXIT_USAGE;
}
