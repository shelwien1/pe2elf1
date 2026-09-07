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

/*  Dump one libvorbis release's codebook sets as text, for bookmerge.py.

    A stream carries its codebooks in the setup header, and tsv2wav would
    rather name the set than store it.  Naming it only works if the set is
    one we can rebuild, so every set any released libvorbis can produce is
    worth having: the compiled-in copy is one encoder, and files in the wild
    were written by twenty years of others.  This program is built once per
    release against that release's own tree and prints what it can build;
    psyfit/bookmerge.py folds the dumps together into ../vbooks_gen.inc.
    See psyfit/bookdump.sh, which does both.

    Two axes are swept, because they do not reach the same sets.  Quality
    picks a setup template through its quality_mapping and leaves bitrate
    management off; a nominal bitrate picks through rate_mapping instead, and
    turns management on, which switches the residues to the books_base_managed
    family -- a set no quality ever produces.  Within one template the books
    are indexed by the truncated base_setting, so a coarse sweep of either
    axis reaches everything that axis can reach.

    Two things vary across the releases and are set from the command line:

        -DBG_LIB='"1.0rc2"'   the name this dump is labelled with
        -DBG_LL_LONG          static_codebook::lengthlist is long*, not char*
        -DBG_NO_VBR           vorbis_encode_init_vbr is not declared at all
                              (beta4 and rc1); rc2 declares it but returns
                              OV_EIMPL, which needs no flag -- the sweep just
                              comes back empty

    The set a stream gets depends on the channel count, the sample rate and
    the encoder setting, and libvorbis chooses the template by a sorted list
    of rate restrictions -- so for a fixed (channels, setting) the choice is
    a step function of the rate.  The scan below walks a coarse grid and then
    bisects each step it finds, which puts the exact boundary rate in the
    dump instead of the nearest grid point.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"
#include "codec_internal.h"
#include "backends.h"

#ifndef BG_LIB
#define BG_LIB "unknown"
#endif
#ifdef BG_LL_LONG
typedef long bg_ll;
#else
typedef char bg_ll;
#endif

/*  _book_maptype1_quantvals, spelled out rather than linked: it is static in
    some of these releases and exported from others.  */
static long qv1(long entries, long dim) {
  int vals = (int) floor(pow((float) entries, 1.f / dim));
  for (;;) {
    long acc = 1, acc1 = 1;  int i;
    for (i = 0; i < dim; i++) {
      if (entries / vals < acc) break;
      acc *= vals;
      if (LONG_MAX / (vals + 1) < acc1) acc1 = LONG_MAX;  else acc1 *= vals + 1;
    }
    if (i >= dim && acc <= entries && acc1 > entries) return vals;
    if (i < dim || acc > entries) vals--;  else vals++;
  }
}
static long qvals(const static_codebook * b) {
  if (!b->maptype) return 0;
  return (b->maptype == 1) ? qv1(b->entries, b->dim) : b->entries * b->dim;
}

/*  A book is kept once per dump; the merger interns again across dumps.
    The content is copied out rather than pointed at, so that nothing here
    depends on how long a given release keeps its template tables alive.  */
#define MAXB 8192
struct bg_book {
  long dim, entries, q_min, q_delta;
  int maptype, q_quant, q_sequencep, nq;
  long * ll, * ql;
};
static struct bg_book uniq[MAXB];
static int nuniq = 0;

static void * xmalloc(size_t n) {
  void * p = malloc(n ? n : 1);
  if (!p) { fprintf(stderr, "out of memory\n");  exit(1); }
  return p;
}

static int same(const struct bg_book * a, const struct bg_book * b) {
  long i;
  if (a->dim != b->dim || a->entries != b->entries ||
      a->maptype != b->maptype) return 0;
  if (!a->ll != !b->ll) return 0;
  if (a->ll) for (i = 0; i < a->entries; i++) if (a->ll[i] != b->ll[i]) return 0;
  if (a->maptype) {
    if (a->q_min != b->q_min || a->q_delta != b->q_delta ||
        a->q_quant != b->q_quant || a->q_sequencep != b->q_sequencep) return 0;
    for (i = 0; i < a->nq; i++) if (a->ql[i] != b->ql[i]) return 0;
  }
  return 1;
}
static int intern(const static_codebook * s) {
  struct bg_book b;  long i;  int k;
  memset(&b, 0, sizeof b);
  b.dim = s->dim;  b.entries = s->entries;  b.maptype = s->maptype;
  b.q_min = s->q_min;  b.q_delta = s->q_delta;
  b.q_quant = s->q_quant;  b.q_sequencep = s->q_sequencep;
  if (s->lengthlist) {
    b.ll = (long *) xmalloc(sizeof(long) * (size_t) (s->entries ? s->entries : 1));
    for (i = 0; i < s->entries; i++)
      b.ll[i] = (long) ((bg_ll *) s->lengthlist)[i];
  }
  if (s->maptype) {
    b.nq = (int) qvals(s);
    b.ql = (long *) xmalloc(sizeof(long) * (size_t) (b.nq ? b.nq : 1));
    for (i = 0; i < b.nq; i++) b.ql[i] = s->quantlist[i];
  }
  for (k = 0; k < nuniq; k++)
    if (same(uniq + k, &b)) { free(b.ll);  free(b.ql);  return k; }
  if (nuniq >= MAXB) { fprintf(stderr, "too many books\n");  exit(1); }
  uniq[nuniq] = b;  return nuniq++;
}

/*  The book list for one (ch, setting, rate), as interned ids.  n < 0 means
    the release refuses to encode at all there.  */
#define MAXN 512
struct blist { int n, b[MAXN]; };

/*  mode 0 asks by quality, mode 1 by nominal bitrate.  */
static int build(struct blist * out, int ch, int rate, int mode, double set) {
  vorbis_info vi;  int i, r;
  vorbis_info_init(&vi);
  if (mode) r = vorbis_encode_init(&vi, ch, rate, -1, (long) set, -1);
#ifdef BG_NO_VBR
  else { vorbis_info_clear(&vi);  out->n = -1;  return 0; }
#else
  else r = vorbis_encode_init_vbr(&vi, ch, rate, (float) set);
#endif
  if (r) { vorbis_info_clear(&vi);  out->n = -1;  return 0; }
  { const codec_setup_info * ci = (const codec_setup_info *) vi.codec_setup;
    if (ci->books > MAXN) { fprintf(stderr, "set too wide\n");  exit(1); }
    out->n = ci->books;
    for (i = 0; i < ci->books; i++) out->b[i] = intern(ci->book_param[i]); }
  /*  intern() copies, and vorbis_staticbook_destroy leaves a book that lives
      in static memory alone, so this is safe to hand back  */
  vorbis_info_clear(&vi);
  return 1;
}
static int blist_eq(const struct blist * a, const struct blist * b) {
  int i;
  if (a->n != b->n) return 0;
  for (i = 0; i < a->n; i++) if (a->b[i] != b->b[i]) return 0;
  return 1;
}

/*  Grid for the coarse pass.  Bisection finds a boundary exactly, but only
    one per gap, so the gaps are 1 kHz where the templates actually switch and
    widen only above 60 kHz, where none of them do.  */
static int RATES[512];
static int NRATES;
static void make_rates(void) {
  int r, n = 0;
  RATES[n++] = 1;
  for (r = 1000; r <= 60000; r += 1000) RATES[n++] = r;
  for (r = 64000; r < 200000; r += 4000) RATES[n++] = r;
  RATES[n++] = 200000;                    /*  the highest any template allows  */
  NRATES = n;
}


/*  The largest rate in [lo, hi] that still builds the same list as lo.
    Both ends are known and differ; the step between them is unique because
    the template restrictions are a sorted list, so a plain bisection lands
    on it.  */
static int boundary(int ch, int mode, double setting, int lo, int hi,
                    const struct blist * at_lo) {
  while (lo + 1 < hi) {
    int mid = lo + (hi - lo) / 2;
    struct blist m;
    build(&m, ch, mid, mode, setting);
    if (blist_eq(&m, at_lo)) lo = mid;  else hi = mid;
  }
  return lo;
}

static void sweep(int ch, int mode, double setting, const char * param) {
  struct blist cur, nxt;
  int i, lo, hi;
  build(&cur, ch, RATES[0], mode, setting);
  lo = RATES[0];
  for (i = 1; i < NRATES; i++) {
    build(&nxt, ch, RATES[i], mode, setting);
    if (blist_eq(&nxt, &cur)) continue;
    hi = boundary(ch, mode, setting, RATES[i - 1], RATES[i], &cur);
    if (cur.n >= 0) {
      int k;
      printf("set %d %d %d %s %d", ch, lo, hi, param, cur.n);
      for (k = 0; k < cur.n; k++) printf(" %d", cur.b[k]);
      printf("\n");
    }
    lo = hi + 1;  cur = nxt;
  }
  if (cur.n >= 0) {
    int k;
    printf("set %d %d %d %s %d", ch, lo, RATES[NRATES - 1], param, cur.n);
    for (k = 0; k < cur.n; k++) printf(" %d", cur.b[k]);
    printf("\n");
  }
}

int main(void) {
  int ch, i;
  make_rates();
  printf("lib %s\n", BG_LIB);
  /*  1 and 2 are the common ones; 6 has its own template and the
      rest fall to the uncoupled ones, so all eight are swept.  */
  for (ch = 1; ch <= 8; ch++) {
    for (i = -1; i <= 10; i++) {
      char p[32];  sprintf(p, "q%d", i);
      sweep(ch, 0, i * 0.1, p);
    }
    /*  Per channel, so that the template's rate_mapping -- which is also per
        channel -- is swept end to end.  Only the interval matters, since the
        books within a template are indexed by the truncated setting, so this
        is spaced to land in every interval rather than to be fine.  */
    { static const long BR[] = {
        2000, 3000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 20000,
        24000, 28000, 32000, 36000, 40000, 44000, 48000, 56000, 64000, 72000,
        80000, 96000, 112000, 128000, 144000, 160000, 192000, 224000, 256000,
        320000, 400000, 500000
      };
      for (i = 0; i < (int) (sizeof BR / sizeof *BR); i++) {
        char p[32];  sprintf(p, "b%ld", BR[i]);
        sweep(ch, 1, (double) (BR[i] * ch), p);
      } }
  }
  for (i = 0; i < nuniq; i++) {
    const struct bg_book * b = uniq + i;  long j;
    printf("book %d %ld %ld %d %ld %ld %d %d\n", i, b->dim, b->entries,
           b->maptype, b->q_min, b->q_delta, b->q_quant, b->q_sequencep);
    printf("l");
    if (b->ll) for (j = 0; j < b->entries; j++) printf(" %ld", b->ll[j]);
    printf("\n");
    if (b->maptype) {
      printf("q");
      for (j = 0; j < b->nq; j++) printf(" %ld", b->ql[j]);
      printf("\n");
    }
  }
  fprintf(stderr, "%s: %d books\n", BG_LIB, nuniq);
  return 0;
}
