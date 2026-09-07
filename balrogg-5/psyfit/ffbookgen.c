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

/*  Dump ffmpeg's own Vorbis encoder's codebooks, for bookmerge.py.

    ffmpeg has two Vorbis encoders.  `-c:a libvorbis` is libvorbis, which
    bookgen.c already covers; `-c:a vorbis` is ffmpeg's own, and it chooses
    from nothing -- it has one set of 29 codebooks in libavcodec/
    vorbis_enc_data.h and uses it for every file it writes.  Neither the
    sample rate nor the quality nor anything else moves it, and the encoder
    refuses to run on anything but stereo, so the whole encoder is one row
    of the table.

    Built against an ffmpeg checkout, which supplies the data:

        git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg
        cc -O1 -w -I ffmpeg -o ffbookgen psyfit/ffbookgen.c -lm
        ./ffbookgen > ffmpeg.dump

    -w is not laziness: the cvectors entries with no lookup leave `min` and
    `delta` off, so -Wextra warns sixteen times about ffmpeg's own header.
    Those fields are zero and are only read when the lookup says they mean
    something.

    The construction below is create_vorbis_context() in vorbisenc.c, and
    the two derived fields are what put_codebook_header() writes: q_min and
    q_delta are its put_float() of the cvectors floats, and q_quant is its
    `bits`, the widest quantlist entry.  It is written out rather than
    transcribed so that a later ffmpeg is a rebuild rather than a rewrite.

    One thing to know before trusting the result: ffmpeg decides a book is
    "ordered" on a weaker test than libvorbis does -- libvorbis also
    disqualifies a zero length, ffmpeg only checks that the lengths do not
    descend -- and vbooks.inc emits libvorbis's decision.  On these 29 books
    the two rules agree everywhere, so nothing has to be carried to say
    which packer wrote the stream.  A future book where they disagree would
    show up as a set that is found but does not match, and would need that.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libavcodec/vorbis_enc_data.h"

#define NBOOKS ((int) (sizeof cvectors / sizeof *cvectors))

/*  ilog(i) is av_log2(2*i) in ffmpeg: the number of bits i needs, and 0
    for 0.  */
static int ilog(unsigned v) { int n = 0;  while (v) { n++;  v >>= 1; }  return n; }

/*  ff_vorbis_nth_root: the largest r with r^n <= x.  */
static int nth_root(int x, int n) {
  int r = (int) (pow(x, 1.0 / n) + 0.5), i;
  for (;;) {
    double acc = 1;
    for (i = 0; i < n; i++) acc *= r;
    if (acc > x) r--;  else break;
  }
  for (;;) {
    double acc = 1;
    for (i = 0; i < n; i++) acc *= r + 1;
    if (acc <= x) r++;  else break;
  }
  return r;
}
static int lookup_vals(int lookup, int dim, int entries) {
  if (lookup == 1) return nth_root(entries, dim);
  if (lookup == 2) return dim * entries;
  return 0;
}

/*  put_float(), which is the Vorbis packed 32-bit float the header carries
    and what libvorbis keeps in static_codebook::q_min.  */
static long pack_float(double f) {
  int exp, mant;
  unsigned res = 0;
  mant = (int) ldexp(frexp(f, &exp), 20);
  exp += 788 - 20;
  if (mant < 0) { res |= 1u << 31;  mant = -mant; }
  res |= (unsigned) mant | ((unsigned) exp << 21);
  return (long) res;
}

int main(void) {
  const unsigned char * clens = codebooks;
  const unsigned char * quant = quant_tables;
  int book, i;

  printf("lib ffmpeg\n");
  /*  vorbis_encode_init() refuses anything but two channels, and the set
      does not depend on the rate, so this is the whole encoder.  */
  printf("set 2 1 200000 fixed %d", NBOOKS);
  for (i = 0; i < NBOOKS; i++) printf(" %d", i);
  printf("\n");

  for (book = 0; book < NBOOKS; book++) {
    int dim = cvectors[book].dim;
    int used = cvectors[book].len;        /*  lengths actually listed  */
    int ent = cvectors[book].real_len;    /*  entries; the rest are zero  */
    int lookup = cvectors[book].lookup;
    int nq = lookup_vals(lookup, dim, ent);
    int bits = 0;

    for (i = 0; i < nq; i++) {
      int b = ilog(quant[i]);
      if (b > bits) bits = b;
    }
    printf("book %d %d %d %d %ld %ld %d %d\n", book, dim, ent, lookup,
           lookup ? pack_float(cvectors[book].min) : 0L,
           lookup ? pack_float(cvectors[book].delta) : 0L,
           bits, 0);                      /*  seq_p is 0 for every one  */
    printf("l");
    for (i = 0; i < used; i++) printf(" %d", clens[i]);
    for (; i < ent; i++) printf(" 0");
    printf("\n");
    if (lookup) {
      printf("q");
      for (i = 0; i < nq; i++) printf(" %d", quant[i]);
      printf("\n");
    }
    clens += used;  quant += nq;
  }
  /*  Both tables are one flat run consumed in order, so landing exactly on
      their ends is a real check that every entry count and every quantlist
      length came out right -- one wrong and the rest are shifted.  */
  if (clens != codebooks + sizeof codebooks ||
      quant != quant_tables + sizeof quant_tables) {
    fprintf(stderr, "ffbookgen: consumed %ld of %ld codebook bytes and %ld of "
            "%ld quant bytes -- this ffmpeg is not shaped as expected\n",
            (long) (clens - codebooks), (long) sizeof codebooks,
            (long) (quant - quant_tables), (long) sizeof quant_tables);
    return 1;
  }
  fprintf(stderr, "ffmpeg: %d books\n", NBOOKS);
  return 0;
}
