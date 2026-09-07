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

/*  The residue walk, run backwards.

    Reading the record stream and reading the PCM meet here.  The walk has the
    same shape as the decoder's residue loop, but where the decoder adds a
    digit's worth of signal this subtracts it: at each pass the coarsest book
    goes first, its digit is whatever the remaining value rounds to, and what
    that digit stands for comes off before the next pass looks.

    Two roles share the walk.  ROLE_META reads the true classes and digits
    alongside, and records the ones the walk got wrong.  ROLE_REST has only
    the corrections, applies them at the same points, and writes the records
    out.  Both subtract the corrected value, so the two stay in step.  */

enum { ROLE_SYNTH = 0, ROLE_META = 1, ROLE_REST = 2, ROLE_FIT = 3 };

/*  How libvorbis chooses a class.

    The encoder does not search; it measures the partition's quantized residue
    and compares against a fixed table.  For residue 0 and 1 (`_01class`) the
    measures are the peak absolute value and the absolute sum scaled by
    100/psz; for residue 2 (`_2class`) they are the peak of the magnitude
    channel and the peak across the angle channels.  The class is the first
    index whose two thresholds both admit the measures.

    Those thresholds -- classmetric1 and classmetric2 -- live in the encoder's
    setup templates and are not in the bitstream, so they are fitted here from
    the stream itself and carried in the meta: a few dozen integers per
    residue against tens of thousands of class records.

    Two model shapes are fitted and the better one kept.  The threshold rule
    reproduces residue 0 and 1 exactly.  Residue 2 streams whose classes are
    not nested in that order do better with a direct table over the measure
    pairs, which stays small because the measures are small integers.  */
constexpr u32 CF_TAB = 2048;              /*  distinct measure pairs kept  */

struct clsfit {
  i32 cm1[VD_MAXRCL], cm2[VD_MAXRCL];     /*  threshold rule  */
  i32 tf1[CF_TAB], tf2[CF_TAB];           /*  table: the measure pairs ...  */
  u8  tcl[CF_TAB];                        /*  ... and the class they imply  */
  u32 tcnt[CF_TAB][VD_MAXCLASS];
  u32 tn, over;                           /*  entries, and whether it spilled  */
  u32 ncl;
  int mode;                               /*  0 threshold rule, 1 table  */

  void reset(u32 n) {
    u32 i, k;
    ncl = n;  tn = 0;  over = 0;  mode = 0;
    for (i = 0; i < VD_MAXRCL; i++) { cm1[i] = -1;  cm2[i] = -1; }
    for (i = 0; i < CF_TAB; i++) { tf1[i] = tf2[i] = -1;  tcl[i] = 0;
                                   for (k = 0; k < VD_MAXCLASS; k++) tcnt[i][k] = 0; }
  }

  void observe(i32 f1, i32 f2, u32 c) {
    u32 i;
    if (c < VD_MAXRCL) {
      if (f1 > cm1[c]) cm1[c] = f1;
      if (f2 > cm2[c]) cm2[c] = f2;
    }
    for (i = 0; i < tn; i++)
      if (tf1[i] == f1 && tf2[i] == f2) {
        if (c < VD_MAXCLASS) tcnt[i][c]++;
        return;
      }
    if (tn == CF_TAB) { over = 1;  return; }
    tf1[tn] = f1;  tf2[tn] = f2;
    if (c < VD_MAXCLASS) tcnt[tn][c]++;
    tn++;
  }

  u32 by_rule(i32 f1, i32 f2) const {
    u32 k;
    for (k = 0; k + 1 < ncl; k++)
      if (f1 <= cm1[k] && f2 <= cm2[k]) return k;
    return ncl ? ncl - 1 : 0;
  }

  u32 by_table(i32 f1, i32 f2) const {
    u32 i;
    for (i = 0; i < tn; i++) if (tf1[i] == f1 && tf2[i] == f2) return tcl[i];
    return by_rule(f1, f2);
  }

  u32 predict(i32 f1, i32 f2) const {
    return mode ? by_table(f1, f2) : by_rule(f1, f2);
  }

  /*  Settle the table's majority class per pair, score both shapes on what
      was observed, and keep the better.  */
  void settle() {
    u32 i, k, tot = 0, hit_t = 0, hit_r = 0;
    for (i = 0; i < tn; i++) {
      u32 best = 0, bc = 0, sum = 0;
      for (k = 0; k < VD_MAXCLASS; k++) {
        sum += tcnt[i][k];
        if (tcnt[i][k] > bc) { bc = tcnt[i][k];  best = k; }
      }
      tcl[i] = (u8) best;  hit_t += bc;  tot += sum;
      { u32 p = by_rule(tf1[i], tf2[i]);
        if (p < VD_MAXCLASS) hit_r += tcnt[i][p]; }
    }
    mode = (!over && hit_t > hit_r);
    (void) tot;
  }
};

static clsfit cfit[VD_MAXRES];

/*  A model over the walk's own mistakes.

    The digits themselves are not predictable from each other -- the floor has
    already taken the spectral envelope out, and what is left has an
    autocorrelation of 0.009 at lag one, so a linear predictor over them finds
    nothing.  The walk's *error* is a different matter: it runs -0.42 at lag
    one and +0.32 at lag two, and where the walk has just been wrong it is
    wrong again 74% of the time against 11% overall.  Errors arrive in bursts.

    What will not work on that is LPC, and it was tried: the error is zero at
    nine digits in ten, so a predictor that rounds a continuous estimate
    breaks far more right answers than it mends -- corrections rose 40%.  What
    does work is the commonest error seen before in the same context, with the
    context being the two previous errors clipped to +-3, and with zero given
    a margin so that it wins anything close.

    Nothing about it is carried.  Both roles see the same errors in the same
    order -- META has the truth and REST has the corrections, and either way
    the digit that comes out is the same one -- so both update the same
    counters and reach the same prediction.  All that travels is one flag per
    stream saying whether the model earns its place, which the fit pass
    decides by running it in shadow.  */
constexpr int DG_K = 3;                  /*  errors are clipped to this  */
constexpr int DG_V = 2 * DG_K + 1;       /*  ... so this many of them  */
constexpr int DG_SYM = DG_V + 1;         /*  ... plus "nothing yet"  */
constexpr int DG_CTX = DG_SYM * DG_SYM;  /*  the two previous errors  */

struct dgmodel {
  u16 cnt[DG_CTX][DG_V];
  u8 h0, h1;                             /*  the two previous, as symbols  */

  void clear() { memset(cnt, 0, sizeof cnt);  restart(); }
  void restart() { h0 = h1 = (u8) DG_V; }   /*  a packet starts blind  */
  int ctx() const { return (int) h0 * DG_SYM + (int) h1; }

  /*  The commonest error in this context, or none.  Zero carries half its
      own count again, so a rival has to be clearly ahead to displace it --
      predicting a correction that is not there costs a digit that was
      already right.  */
  int predict() const {
    const u16 * r = cnt[ctx()];
    int v, best = 0;
    long bc = (long) r[DG_K] + (r[DG_K] >> 1);
    for (v = 0; v < DG_V; v++)
      if (v != DG_K && (long) r[v] > bc) { bc = r[v];  best = v - DG_K; }
    return best;
  }
  void observe(i32 e) {
    int s = e < -DG_K ? -DG_K : (e > DG_K ? DG_K : (int) e);
    u16 * r = cnt[ctx()];
    if (++r[s + DG_K] >= 1024) {         /*  halve, so the model can move  */
      int v;
      for (v = 0; v < DG_V; v++) r[v] = (u16) (r[v] >> 1);
    }
    h1 = h0;  h0 = (u8) (s + DG_K);
  }
};

static dgmodel dgm;
static int dg_on;                        /*  does it earn its place?  */
static long long dg_hit0, dg_hit1;       /*  the shadow run, in the fit pass  */

/*  The same idea over the classes, with the context iczelia's balrogg uses
    for them (src/vorbis.h, AR_PCLS): not the class of the partition before
    this one, but the class this same partition had in the *previous packet*.
    A spectral envelope holds still from one packet to the next, so partition
    p tends to want the ladder it wanted last time -- which is exactly what
    the metric fit cannot see, since it looks at one partition's residue and
    nothing else.

    Both of those and the fit's own guess together are worth far more than
    either alone: over three files the walk-order predecessor alone takes 4
    to 16 percent of the class corrections, the previous packet alone 6 to 26,
    and the two together 20 to 30.

    Adaptive, and nothing travels but a flag.  */
constexpr int CG_P = VD_MAXCLASS + 1;    /*  a class, or "nothing yet"  */
constexpr int CG_CTX = VD_MAXCLASS * CG_P * CG_P;
constexpr u32 CG_NCH = 4;                /*  channels past this share a slot  */
constexpr u32 CG_NPART = 1024;           /*  ... as do partitions past this  */

struct cgmodel {
  u16 (* cnt)[VD_MAXCLASS];
  u8 * last;                             /*  per residue, channel, partition  */
  u8 pw;                                 /*  the walk-order predecessor  */

  void alloc() {
    if (!cnt) cnt = (u16 (*)[VD_MAXCLASS]) calloc(CG_CTX, sizeof *cnt);
    if (!last) last = (u8 *) malloc(VD_MAXRES * CG_NCH * CG_NPART);
  }
  void clear() {
    alloc();
    if (cnt) memset(cnt, 0, CG_CTX * sizeof *cnt);
    if (last) memset(last, (int) VD_MAXCLASS, VD_MAXRES * CG_NCH * CG_NPART);
    restart();
  }
  void restart() { pw = (u8) VD_MAXCLASS; }
  u32 slot(u32 rno, u32 ch, u32 part) const {
    if (rno >= VD_MAXRES) rno = VD_MAXRES - 1;
    if (ch >= CG_NCH) ch = CG_NCH - 1;
    if (part >= CG_NPART) part = CG_NPART - 1;
    return (rno * CG_NCH + ch) * CG_NPART + part;
  }
  u32 ctx(u32 got, u32 was) const { return (got * CG_P + was) * CG_P + pw; }

  /*  The commonest class seen in this context, or the fit's guess.  The
      guess carries half its own count again, so a rival must be clearly
      ahead before a right answer is thrown away.  */
  u32 predict(u32 got, u32 rno, u32 ch, u32 part) const {
    const u16 * r;  u32 v, best = got;  long bc;
    if (!cnt || !last || got >= VD_MAXCLASS) return got;
    r = cnt[ctx(got, last[slot(rno, ch, part)])];
    bc = (long) r[got] + (r[got] >> 1);
    for (v = 0; v < VD_MAXCLASS; v++)
      if (v != got && (long) r[v] > bc) { bc = r[v];  best = v; }
    return best;
  }
  void observe(u32 got, u32 want, u32 rno, u32 ch, u32 part) {
    if (cnt && last && got < VD_MAXCLASS) {
      u32 sl = slot(rno, ch, part);
      u16 * r = cnt[ctx(got, last[sl])];
      if (want < VD_MAXCLASS) {
        if (++r[want] >= 1024) {
          u32 v;
          for (v = 0; v < VD_MAXCLASS; v++) r[v] = (u16) (r[v] >> 1);
        }
        last[sl] = (u8) want;
      }
    }
    pw = (u8) (want < VD_MAXCLASS ? want : VD_MAXCLASS);
  }
};

static cgmodel cgm;
static int cg_on, cg_score;
static long long cg_hit0, cg_hit1;

constexpr u32 KEEP_MAX = 1u << 20;        /*  keeps in one packet  */

static u32 kc_i[KEEP_MAX];  static i32 kc_v[KEEP_MAX];
static u32 kd_i[KEEP_MAX];  static i32 kd_v[KEEP_MAX];
/*  Every true value of the packet, in order.  The writer only learns which
    form is cheaper once the walk has finished, so it records both the sparse
    mismatch list and the full run and picks at emission time.  */
static i32 kc_all[KEEP_MAX];
static i32 kd_all[KEEP_MAX];
static i64 rc_tmp[MD_MAXBLK];             /*  a partition, for the class trial  */

struct vd_rec {
  vd_setup * s;
  tsv * src, * dst;
  int role;
  int covered;                            /*  the PCM spans this block  */
  int kc_raw, kd_raw;                     /*  corrections came as raw runs  */
  u32 fit_rno;  i32 fit_f1, fit_f2;  int fit_ok;
  u32 kc_n, kd_n, kc_p, kd_p;             /*  keep counts and cursors  */
  u32 ci, di;                             /*  class and digit index in packet  */
  u32 nclamp;                             /*  digits that hit a book's edge  */

  void packet_begin() { kc_n = kd_n = kc_p = kd_p = 0;  ci = di = 0;
                        nclamp = 0;  kc_raw = kd_raw = 0;
                        dgm.restart();  cgm.restart(); }

  /*  One digit: what is left rounds to it, and what it stands for comes off.

      A sequenced book adds the previous scalar of the same symbol to each
      one, so `last` carries that sum; for the ordinary unsequenced book it
      stays zero and drops out.  */
  /*  A deadzone was measured here and does not pay.  Leaning the digit
      threshold toward zero looks right -- on a six-channel stream 97% of the
      corrections had a true value of zero -- but that is a selection effect:
      among the predictions that are wrong the truth is usually zero, while
      most predictions are right.  Thresholds from 0.50 to 0.75 grow the meta
      monotonically, 218,700 to 262,085 bytes.  Nearest is optimal.  */
  i32 take(i64 & v, vd_book * b, u32 * clamped, i64 & last) {
    i64 base = b->min_fx + last, con;
    /*  Multiply, not shift: v - base is routinely negative here, and shifting
        a negative value left is undefined before C++20.  The two are the same
        value on any two's-complement target, and this is the only left shift
        in the tree whose operand can be negative.  */
    i64 t = (i64) (((__int128) (v - base) * ((__int128) 1 << F)) / b->delta_fx);
    i64 q = _rnd(t);
    i32 d = (i32) (q - (i64) b->off);
    i32 lo = -(i32) b->off, hi = (i32) b->nv - 1 - (i32) b->off;
    if (d < lo) { d = lo;  if (clamped) (*clamped)++; }
    if (d > hi) { d = hi;  if (clamped) (*clamped)++; }
    con = (i64) (d + (i32) b->off) * b->delta_fx + base;
    v -= con;
    if (b->seq) last = con;
    return d;
  }

  /*  A digit, reconciled against the truth (META) or the corrections (REST),
      then emitted.  Returns the value both roles agree on.

      A block the PCM does not span cannot be analysed at all, so every digit
      in it is a correction.  Carrying those as index/value pairs costs two
      records where one would do, so such a packet switches to a raw run: the
      values in order, no indices.  Both roles derive `covered` from the same
      block geometry, so neither needs to be told which form is in use.  */
  /*  What the walk guessed, moved by what the model makes of the mistakes
      before it, and held inside the book either way.  */
  i32 guess(i32 got, vd_book * b) const {
    i32 lo = -(i32) b->off, hi = (i32) b->nv - 1 - (i32) b->off, p;
    if (!dg_on) return got;
    p = got + (i32) dgm.predict();
    return p < lo ? lo : (p > hi ? hi : p);
  }

  i32 digit(i64 & v, vd_book * b, i64 & last) {
    i64 before = last;
    i32 got = take(v, b, &nclamp, last), fin, pred;
    if (role == ROLE_FIT) {
      /*  the model runs here only to be scored: the walk's guess against the
          same guess corrected, over every digit in the stream  */
      i32 want = (i32) src->get("res.digit");
      dg_hit0 += (want == got);
      dg_hit1 += (want == got + (i32) dgm.predict());
      dgm.observe(want - got);
      if (want != got) v -= (i64) (want - got) * b->delta_fx;
      di++;
      return want;
    }
    pred = guess(got, b);
    if (role == ROLE_META) {
      i32 want = (i32) src->get("res.digit");
      FATAL_UNLESS(di < KEEP_MAX && kd_n < KEEP_MAX,
                   "too many digits in a packet");
      kd_all[di] = want;
      if (want != pred) { kd_i[kd_n] = di;  kd_v[kd_n] = want;  kd_n++; }
      fin = want;
    } else {
      if (kd_raw) fin = kd_v[kd_p++];           /*  raw run  */
      else {
        fin = pred;
        if (kd_p < kd_n && kd_i[kd_p] == di) fin = kd_v[kd_p++];
      }
      dst->put("res.digit", fin);
    }
    dgm.observe(fin - got);
    /*  put back what the walk took and remove what the stream really had  */
    if (fin != got) {
      v -= (i64) (fin - got) * b->delta_fx;
      if (b->seq) last = before + (i64) (fin + (i32) b->off) * b->delta_fx
                       + b->min_fx;
    }
    di++;
    return fin;
  }

  /*  Every scalar of one partition, in the order the decoder placed them.  */
  void split(vd_res * r, vd_book * b, i64 ** bundle, u32 nch, u32 pos) {
    u32 i, j, dim = b->dim, psz = r->psz;
    if (r->type == 0) {
      u32 step = psz / dim;
      FATAL_UNLESS(step * dim == psz, "residue 0 partition size");
      for (j = 0; j < step; j++) {
        i64 last = 0;
        for (i = 0; i < dim; i++) digit(bundle[0][pos + i * step + j], b, last);
      }
    } else if (r->type == 1) {
      for (i = 0; i < psz; i += dim) {
        i64 last = 0;
        for (j = 0; j < dim; j++) {
          if (i + j < psz) digit(bundle[0][pos + i + j], b, last);
          else { i64 dead = 0;  digit(dead, b, last); }  /*  placed past the end  */
        }
      }
    } else {
      u32 cp = pos % nch, ix = pos / nch;
      for (i = 0; i < psz; i += dim) {
        i64 last = 0;
        for (j = 0; j < dim; j++) {
          digit(bundle[cp][ix], b, last);
          if (++cp == nch) { cp = 0;  ix++; }
        }
      }
    }
  }

  /*  Score a candidate class: run its ladder over a copy of the partition and
      see what it costs and what it leaves behind.  The true class leaves
      nothing, so the cheapest clean fit is the prediction.  */
  double trial(vd_res * r, u32 cls, i64 ** bundle, u32 nch, u32 pos) {
    u32 pass, i, j, dim, psz = r->psz;
    double cost = 0;
    u32 clamped = 0;
    for (i = 0; i < psz; i++) {
      if (r->type == 2) rc_tmp[i] = bundle[(pos + i) % nch][(pos + i) / nch];
      else rc_tmp[i] = bundle[0][pos + i];
    }
    for (pass = 0; pass < 8; pass++) {
      i32 bn = r->book[cls][pass];
      vd_book * b;
      if (bn < 0) continue;
      b = s->bk + bn;  dim = b->dim;
      if (r->type == 0) {
        u32 step = psz / dim;
        if (step * dim != psz) return 1e30;
        for (j = 0; j < step; j++) {
          i64 last = 0;
          for (i = 0; i < dim; i++) {
            i32 d = take(rc_tmp[i * step + j], b, &clamped, last);
            cost += d < 0 ? -d : d;
          }
        }
      } else {
        for (i = 0; i < psz; i += dim) {
          i64 last = 0;
          for (j = 0; j < dim && i + j < psz; j++) {
            i32 d = take(rc_tmp[i + j], b, &clamped, last);
            cost += d < 0 ? -d : d;
          }
        }
      }
    }
    for (i = 0; i < psz; i++) {
      double left = fabs((double) rc_tmp[i] / (double) ((i64) 1 << F));
      cost += 8.0 * left;                 /*  what the ladder could not explain  */
    }
    return cost + 1e6 * clamped;
  }

  /*  The two measures libvorbis takes of a partition, from the quantized
      residue.  For residue 2 the first is the peak of the bundle's magnitude
      channel and the second the peak across its angle channels; otherwise the
      peak and the absolute sum scaled by 100/psz, both as libvorbis rounds
      them.  */
  void measures(vd_res * r, i64 ** bundle, u32 nch, u32 pos, i32 & f1, i32 & f2) {
    i64 half = (i64) 1 << (F - 1);
    u32 i, k;
    f1 = f2 = 0;
    if (r->type == 2) {
      for (i = 0; i < r->psz; i += nch)
        for (k = 0; k < nch; k++) {
          i64 v = bundle[k][(pos + i) / nch];
          i32 a = (i32) (((v < 0 ? -v : v) + half) >> F);
          if (k == 0) { if (a > f1) f1 = a; } else if (a > f2) f2 = a;
        }
    } else {
      i64 sum = 0;
      for (i = 0; i < r->psz; i++) {
        i64 v = bundle[0][pos + i];
        i32 a = (i32) (((v < 0 ? -v : v) + half) >> F);
        if (a > f1) f1 = a;
        sum += a;
      }
      f2 = (i32) ((double) sum * (100.0 / (double) r->psz));
    }
  }

  /*  How far a class's ladder can reach: the sum over its passes of the
      largest digit each book can hold, in signal units.  Fixed by the setup,
      so it is computed once per residue rather than per partition.  */
  double rch[VD_MAXRES][VD_MAXRCL];

  void build_reach() {
    u32 i, c, pass;
    for (i = 0; i < s->nrs; i++)
      for (c = 0; c < s->rs[i].ncl; c++) {
        double t = 0;
        for (pass = 0; pass < 8; pass++) {
          i32 bn = s->rs[i].book[c][pass];
          if (bn < 0) continue;
          t += (double) ((i32) s->bk[bn].nv - 1 - (i32) s->bk[bn].off)
             * fabs((double) s->bk[bn].delta);
        }
        rch[i][c] = t;
      }
  }

  /*  The class is the encoder's choice of ladder, and libvorbis chooses it
      from the partition's magnitude: the cheapest ladder that still covers
      the values.  So take the smallest reach that does not clamp, rather
      than whichever ladder happens to yield the smallest digits.  */
  u32 predict(vd_res * r, u32 rno, i64 ** bundle, u32 nch, u32 pos) {
    u32 c, best = 0, i;
    double bs = 1e300, peak = 0, br = 1e300;
    if (cfit[rno].ncl) {                  /*  a model was fitted for this one  */
      i32 f1, f2;
      measures(r, bundle, nch, pos, f1, f2);
      c = cfit[rno].predict(f1, f2);
      return c < r->ncl ? c : 0;
    }
    for (i = 0; i < r->psz; i++) {
      i64 v = r->type == 2 ? bundle[(pos + i) % nch][(pos + i) / nch]
                           : bundle[0][pos + i];
      double a = fabs((double) v / (double) ((i64) 1 << F));
      if (a > peak) peak = a;
    }
    for (c = 0; c < r->ncl; c++) {
      double rc = rch[rno][c];
      if (rc + 0.5 < peak) continue;            /*  cannot hold the peak  */
      if (rc < br - 1e-12) { br = rc;  best = c; }
    }
    if (br < 1e299) return best;
    /*  nothing reaches: fall back on the full trial  */
    for (c = 0; c < r->ncl; c++) {
      double sc = trial(r, c, bundle, nch, pos);
      if (sc < bs - 1e-9) { bs = sc;  best = c; }
    }
    return best;
  }

  /*  Reconcile a predicted class against the truth or the corrections.  */
  u32 klass(u32 got, u32 rno, u32 ch, u32 part) {
    u32 fin;
    if (role == ROLE_FIT) {               /*  collect, emit nothing  */
      u32 want = src->get_u("res.class", 16);
      if (fit_ok) cfit[fit_rno].observe(fit_f1, fit_f2, want);
      ci++;
      return want;
    }
    /*  The fit's guess is only real once the fit has settled, which is after
        the fit pass, so the model is scored in a meta pass instead.  */
    { u32 pred = cg_on ? cgm.predict(got, rno, ch, part) : got;
    if (role == ROLE_META) {
      u32 want = src->get_u("res.class", 16);
      FATAL_UNLESS(ci < KEEP_MAX && kc_n < KEEP_MAX,
                   "too many classes in a packet");
      /*  scored here, where the fit's guess is real and the truth is at hand  */
      if (cg_score) { cg_hit0 += (want == got);
                      cg_hit1 += (want == cgm.predict(got, rno, ch, part)); }
      kc_all[ci] = (i32) want;
      if (want != pred) { kc_i[kc_n] = ci;  kc_v[kc_n] = (i32) want;  kc_n++; }
      fin = want;
    } else {
      if (kc_raw) fin = (u32) kc_v[kc_p++];     /*  raw run  */
      else {
        fin = pred;
        if (kc_p < kc_n && kc_i[kc_p] == ci) fin = (u32) kc_v[kc_p++];
      }
      dst->put("res.class", fin);
    }
    cgm.observe(got, fin, rno, ch, part); }
    ci++;
    return fin;
  }

  /*  The decoder's residue loop, with the two steps above in place of the
      decode.  */
  void residue(u32 rno, i64 ** bundle, const u8 * nz, u32 nch, u32 n) {
    vd_res * r = s->rs + rno;
    vd_book * cb = s->bk + r->cbook;
    i64 * act[VD_MAXCH];
    u32 vch, end, np, pv, w, pass, pc, i, j, k, nc = 0;
    for (i = 0; i < nch; i++) if (nz[i]) act[nc++] = bundle[i];
    if (!nc) return;
    if (r->type == 2) { vch = 1;  end = r->end < n * nch ? r->end : n * nch; }
    else { vch = nc;  end = r->end < n ? r->end : n; }
    if (end <= r->beg) return;
    np = (end - r->beg) / r->psz;
    if (!np) return;
    pv = cb->dim;  w = np + pv;
    FATAL_UNLESS((sz) vch * w <= sizeof vd_cl / sizeof *vd_cl,
                 "residue classification table");
    for (pass = 0; pass < 8; pass++) {
      pc = 0;
      while (pc < np) {
        if (!pass)
          for (j = 0; j < vch; j++)
            for (k = 0; k < pv; k++) {
              /*  the decoder codes a whole classword even where the last few
                  partitions do not exist; those classes are never used  */
              u32 at = pc + k, guess = 0;
              fit_ok = 0;
              if (covered && at < np) {
                i64 ** bp = r->type == 2 ? bundle : act + j;
                u32 bpos = r->beg + at * r->psz;
                if (role == ROLE_FIT) {
                  measures(r, bp, nch, bpos, fit_f1, fit_f2);
                  fit_rno = rno;  fit_ok = 1;
                } else guess = predict(r, rno, bp, nch, bpos);
              }
              vd_cl[j * w + at] = klass(guess, rno, j, at);
            }
        for (k = 0; k < pv; k++) {
          if (pc >= np) break;
          for (j = 0; j < vch; j++) {
            u32 c = vd_cl[j * w + pc];
            i32 bn;
            FATAL_UNLESS(c < r->ncl, "residue class %lu out of range",
                         (unsigned long) c);
            bn = r->book[c][pass];
            if (bn < 0) continue;
            if (r->type == 2)
              split(r, s->bk + bn, bundle, nch, r->beg + pc * r->psz);
            else
              split(r, s->bk + bn, act + j, 1, r->beg + pc * r->psz);
          }
          pc++;
        }
      }
    }
  }
};
