
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.inc"      /*  types, diagnostics, loop macros  */

#include "tsv.inc"         /*  the record stream  */
#include "ogg_page.inc"    /*  page framing and its TSV form  */

#include "vb_info.inc"     /*  the Vorbis identification header  */
#include "vb_book.inc"     /*  codebooks, and the arena their bodies live in  */
#include "vb_floor.inc"
#include "vb_res.inc"
#include "vb_map.inc"
#include "vb_setup.inc"    /*  the four lists a link's audio runs on  */
#include "vb_ctx.inc"      /*  what survives from one packet to the next  */
#include "oc_sink.inc"     /*  what the walk hands each value to  */
#include "oc_tsv.inc"      /*  ... here, the record stream  */
typedef oc_tsv sink_t;
#include "io.inc"          /*  one packet, taken apart or rebuilt  */

#include "source.inc"      /*  the forward window over the input  */
#include "link_walk.inc"   /*  packets across the pages of one link  */
#include "codec.inc"       /*  whole-file entry points  */

int main(int argc, char ** argv) {
  if (argc != 4 || argv[1][0] == 0 || argv[1][1] != 0 ||
      (argv[1][0] != 'c' && argv[1][0] != 'd')) {
    fprintf(stderr, "usage: balrogg c|d input output\n"
                    "  c  decompose Ogg Vorbis into a TSV record stream\n"
                    "  d  rebuild the Ogg Vorbis file from that stream\n");
    return BLR_EXIT_USAGE;
  }
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + 2, 2);
  { oc_tsv t;
    if (argv[1][0] == 'c') {
      t.create(argv[3]);
      blr_output(argv[3]);
      vb_pack(t, argv[2]);
      t.close();
    } else {
      FILE * o;
      t.open(argv[2]);
      o = fopen(argv[3], "wb");
      if (!o) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", argv[3]);
      blr_output(argv[3]);
      vb_unpack(t, argv[2], o, argv[3]);
      t.close();
      if (fclose(o)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", argv[3]);
    }
    blr_output_kept(); }
  return BLR_EXIT_OK;
}
