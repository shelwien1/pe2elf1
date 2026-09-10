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
    own histories.  */

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

#include "sh_v2f.inc"      /*  the range coder  */
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
    stream decodes under a build with the same MOD/, and no other.  */
constexpr char OC_MAGIC[] = "oggc\x1a";
constexpr int  OC_VER = 1;

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
        default: goto usage;
      }
  }
  if (argc - a != 2) goto usage;
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + a, 2);
  tc_enc = mode[0] == 'c';
  std::set_new_handler(tc_nomem);
  tc_models();
  { oc_model t;
    const char * in = argv[a], * out = argv[a + 1];
    FILE * bf;
    if (tc_enc) {
      u8 h[2];
      bf = fopen(out, "wb");
      if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
      blr_output(out);
      h[0] = (u8) OC_VER;  h[1] = 0;
      if (fwrite(OC_MAGIC, 1, sizeof OC_MAGIC - 1, bf) != sizeof OC_MAGIC - 1 ||
          fwrite(h, 1, 2, bf) != 2)
        FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
      rcb.attach(bf, 1 << 16);
      rc.StartEncode(&rcb);
      vb_pack(t, in);
      rc.FinishEncode();
      rcb.flush();
#ifdef TC_MEMCOST
      /*  The model's rent, paid where the optimizer is looking.  */
      { u64 n = tc_tables() / ((u64) 1 << 30) * TC_MEMCOST
              + tc_tables() % ((u64) 1 << 30) * TC_MEMCOST / ((u64) 1 << 30);
        u64 i;
        for (i = 0; i < n; i++) fputc(0xFF, bf);
        if (tc_verbose)
          fprintf(stderr, "%s: %" PRIu64 " MB of tables, %" PRIu64 " bytes of rent\n",
                  blr_prog, tc_tables() >> 20, n); }
#endif
      { int bad = ferror(bf);
        if (fclose(bf)) bad = 1;
        if (bad) FATAL_CODE(BLR_EXIT_IO, "write error on %s", out); }
    } else {
      char m[sizeof OC_MAGIC - 1];
      u8 h[2];
      FILE * o;
      bf = fopen(in, "rb");
      if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", in);
      FATAL_UNLESS(fread(m, 1, sizeof m, bf) == sizeof m &&
                   !memcmp(m, OC_MAGIC, sizeof m),
                   "%s: not an oggcomp stream -- `oggcomp c` makes one", in);
      FATAL_UNLESS(fread(h, 1, 2, bf) == 2,
                   "%s: the stream ends in its header", in);
      FATAL_UNLESS(h[0] == OC_VER,
                   "%s: made by oggcomp version %u, this is version %u",
                   in, h[0], OC_VER);
      rcb.attach(bf, 1 << 16);
      rc.StartDecode(&rcb);
      o = fopen(out, "wb");
      if (!o) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
      blr_output(out);
      vb_unpack(t, in, o, out);
      if (fclose(o)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
      fclose(bf);
    }
    blr_output_kept();
    rcb.free_(); }
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
    "\n"
    "The contexts and the rates are part of the format -- a stream is\n"
    "decodable by a build with the same MOD/, and by no other.  ./mk.sh\n"
    "regenerates MOD/ and IDX/opt.pl tunes it.\n");
  return BLR_EXIT_USAGE;
}
