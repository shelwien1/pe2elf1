#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psy.c"
#include "floor1.c"
#include "floor0.c"
#include "mdct.c"
#include "smallft.c"
#include "lookup.c"
#include "lpc.c"
#include "lsp.c"
#include "sharedbook.c"
#include "codebook.c"
#include "registry.c"
#include "info.c"
#include "block.c"
#include "envelope.c"
#include "bitrate.c"
#include "analysis.c"
#include "synthesis.c"
#include "window.c"
#include "res0.c"
#include "mapping0.c"
#include "vorbisenc.c"
#include "bitwise.c"
#include "framing.c"
/*  Dump every codebook exactly as balrogg records it, so the two can be
    compared field for field.  */
int main(int argc, char ** argv) {
  int ch = atoi(argv[1]), rate = atoi(argv[2]);
  double q = atof(argv[3]);
  vorbis_info vi;  vorbis_info_init(&vi);
  if (vorbis_encode_init_vbr(&vi, ch, rate, (float) q)) { puts("no"); return 1; }
  codec_setup_info * ci = (codec_setup_info *) vi.codec_setup;
  printf("books %d\n", ci->books);
  for (int i = 0; i < ci->books; i++) {
    static_codebook * b = ci->book_param[i];
    printf("B %ld %ld %d", b->dim, b->entries, b->lengthlist ? 1 : 0);
    long sum = 0;
    if (b->lengthlist) for (long j = 0; j < b->entries; j++) sum = sum * 131 + b->lengthlist[j];
    printf(" L%ld", sum);
    printf(" %d", b->maptype);
    if (b->maptype) {
      printf(" %ld %ld %d %d %ld", b->q_min, b->q_delta, b->q_quant, b->q_sequencep,
             (long) b->dim);
      long qs = 0, n = (b->maptype == 1) ? _book_maptype1_quantvals(b) : b->entries * b->dim;
      for (long j = 0; j < n; j++) qs = qs * 131 + b->quantlist[j];
      printf(" Q%ld", qs);
    }
    printf("\n");
  }
  vorbis_info_clear(&vi);
  return 0;
}
