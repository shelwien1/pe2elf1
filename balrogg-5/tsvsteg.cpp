/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/*  tsvsteg -- tsv2wav, with the quantization gaps carrying a payload.

      tsvsteg c input.tsv output.wav output.meta payload
      tsvsteg d output.wav restored.tsv output.meta payload

    Mode c does what tsv2wav's does -- PCM out of a balrogg record stream, and
    a meta holding what the PCM cannot give back -- except that where a sample
    falls between two integers, the integer is chosen by decoding the payload
    rather than by a rounding rule.  Mode d rebuilds the record stream exactly
    as tsv2wav does, then synthesises it once more to recover the payload.

    The pair is still lossless: nothing about the payload changes what the
    meta has to carry, only how much of it there is.

    Three differences from tsv2wav's mode c, all of them consequences of mode d
    having to recompute the samples rather than merely read them:

      * the placement sweep is gone and the error feedback is fixed at
        STEG_SHAPE.  tsv2wav can sweep it because nothing downstream has to
        agree about it; here both sides compute x and x depends on it.
      * the synthesis runs once, so the payload is decoded once.
      * mode d costs a second synthesis pass over the stream it just restored.

    Everything else -- the floor refit sweep, the class and digit models, the
    granule prediction -- is tsv2wav's, called from there.  */

#define BLR_STEG 1
#include "tsv2wav.cpp"

int main(int argc, char ** argv) {
  if (argc != 6 || (argv[1][0] != 'c' && argv[1][0] != 'd') || argv[1][1]) {
    fprintf(stderr,
      "usage: tsvsteg c input.tsv output.wav output.meta payload\n"
      "       tsvsteg d output.wav restored.tsv output.meta payload\n");
    return BLR_EXIT_USAGE;
  }
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + 2, 4);
#ifdef BLR_VORBIS
  vf_dbg = getenv("TSV2WAV_SWEEP") != nullptr;
#endif
  wav_shape = STEG_SHAPE;
  if (argv[1][0] == 'c') {
    /*  The payload goes in during the synthesis, and the check that it came
        back out runs alongside it, so a payload too big for the stream fails
        here rather than after the whole meta has been built.  */
    steg_embed_begin(argv[5]);
    synth_and_fit(argv[2], argv[3]);
    steg_embed_end(argv[5]);
    steg_report(argv[3]);
    /*  The analysis passes below read the WAV; none of them synthesises, so
        none of them can disturb what was just written.  */
    steg_mode = STEG_OFF;
    vf_settings(argv);
    { cg_on = 0;  cg_score = 1;  cg_hit0 = cg_hit1 = 0;
      pg_pred = 0;  pg_score = 1;  pg_cd = pg_cp = pg_nlink = 0;
      walk(ROLE_META, argv[2], DEV_NULL, argv[3]);
      cg_score = 0;  cg_on = cg_hit1 > cg_hit0;
      pg_score = 0;  pg_pred = pg_cp + PG_FLAG * pg_nlink < pg_cd; }
    walk(ROLE_META, argv[2], argv[4], argv[3]);
  } else {
    /*  The record stream first, because the samples cannot be recomputed
        without it; then the same synthesis mode c ran, reading the WAV back
        instead of writing it, which turns each sample into the choice that
        was made and each choice back into payload.  */
    walk(ROLE_REST, argv[4], argv[3], argv[2]);
    steg_extract_begin();
    steg_refpath = argv[2];
    walk(ROLE_SYNTH, argv[3], nullptr, DEV_NULL);
    steg_extract_end(argv[5]);
    steg_report(argv[2]);
  }
  return BLR_EXIT_OK;
}
