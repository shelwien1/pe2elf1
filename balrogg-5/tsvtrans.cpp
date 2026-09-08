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

/*  tsvtrans -- a balrogg record stream, rearranged for an entropy coder.

      tsvtrans c [options] input.tsv output.tsv
      tsvtrans d [options] output.tsv restored.tsv

    Mode c reads a record stream and writes another one; mode d turns it back,
    byte for byte.  Nothing is compressed here.  What the transform does is
    hand the coder that comes next a stream with less in it and a shape it can
    address:

      * values the stream already implies are dropped, and mode d works them
        out again -- the page header's type, sequence and serial, the granule,
        and the window flag that names the block before this one;

      * the floor posts and the residue digits go out in rows of a fixed
        width -- one floor curve a row, one residue partition a row -- so that
        column n of a row is always the same thing, which a coder with any
        positional context can use and a ragged row denies it.

    Rows are ragged in the input because balrogg merges consecutive records
    with the same tag onto one line and the run length is whatever the packet
    happened to need: this file's residue digits arrive 32, 64, 128, 144, 160,
    992, 1024 and 1056 to a row.  Under a fixed width they arrive 32 to a row,
    always, and a partition never straddles two of them.

    The transform is structural, not statistical.  It removes what is implied
    and regularises what is not; it does not try to predict a floor post or a
    residue digit, because the coder downstream is better placed to do that
    and this tool would only be guessing in front of it.  */

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define DEV_NULL "nul"
#else
  #define DEV_NULL "/dev/null"
#endif

#include "common.inc"      /*  types, diagnostics, loop macros  */
#include "tsv.inc"         /*  the record stream  */
#include "lift.inc"        /*  fixed point; fx() for the setup's floats  */
#include "floor_db.inc"    /*  the dB table, for imdct.inc's sake  */
#include "imdct.inc"       /*  not used here; MD_MAXBLK bounds the setup  */
#include "vd_setup.inc"    /*  codebooks, floors, residues, mappings, modes  */
#include "bmp.inc"        /*  the optional picture of what came out  */

/*  vd_setup calls this to read the codebook section.  tsv2wav substitutes a
    whole set from an index here; there is nothing to substitute in a stream
    that stays a stream, so this is the plain reading.  */
static void su_books(tsv & t, u32 nbk, vd_book * bk) {
  u32 i;
  for (i = 0; i < nbk; i++) bk[i].read(t);
}

constexpr int OGG_MAXSEG = 255;
constexpr u32 LK_MAX = 4096;

/*  What the transform does, and what it leaves alone.  Each can be switched
    off on its own, which is how each was measured.  */
static int tr_page = 1;                   /*  drop the implied page header  */
static int tr_win  = 1;                   /*  drop the derivable window flag  */
static int tr_rows = 1;                   /*  fixed-width floor and digits  */

static int tr_enc;                        /*  1 = c, 0 = d  */
static int tr_trace;

/*  The pictures, when the command line names them.  One row of records
    becomes one row of pixels, which is only a picture worth looking at
    because the rows are a fixed width: a ragged stream would shear.  */
static const char * tr_ybmp;
static const char * tr_gbmp;
static bmp yimg, gimg;
static int tr_img;                        /*  the images are open  */
constexpr u32 TR_BMPW = 4096;             /*  widest row a picture will hold  */
static u8 tr_px[TR_BMPW * 3];
static u32 tr_bmprows = 1u << 16;

static tsv * tr_in;
static tsv * tr_out;
static vd_setup su;

/*  Both images are opened at the first setup, where the widest floor and the
    widest partition are known: a BMP states its width in the header and rows
    are streamed after it, so the width cannot be discovered later.  A second
    link with a wider setup would have its rows clipped, and the count of
    those is reported rather than passed over.  */
static void tr_img_open(void) {
  u32 i, yw = 0, gw = 0;
  if (!tr_ybmp || tr_img) return;
  for (i = 0; i < su.nfl; i++) if (su.fl[i].posts > yw) yw = su.fl[i].posts;
  for (i = 0; i < su.nrs; i++) if (su.rs[i].psz > gw) gw = su.rs[i].psz;
  if (yw > TR_BMPW) yw = TR_BMPW;
  if (gw > TR_BMPW) gw = TR_BMPW;
  yimg.create(tr_ybmp, yw ? yw : 1, tr_bmprows);
  gimg.create(tr_gbmp, gw ? gw : 1, tr_bmprows);
  tr_img = 1;
}

/*  Tags for the fixed-width rows, one per floor and per residue, built once
    so that the pointers handed to tsv::put outlive the row they open: put()
    remembers the tag pointer to decide whether the next record joins the row,
    and a buffer that has since been rewritten would make it join the wrong
    one.  */
static char tag_y[VD_MAXFLOOR][12];
static char tag_c[VD_MAXRES][12];
static char tag_d[VD_MAXRES][8][12];

static void tr_tags(void) {
  u32 i;
  for (i = 0; i < VD_MAXFLOOR; i++) snprintf(tag_y[i], sizeof *tag_y, "y%u", i);
  for (i = 0; i < VD_MAXRES; i++) {
    u32 k;
    snprintf(tag_c[i], sizeof *tag_c, "q%u", i);
    /*  The pass rides in the tag rather than being left for the coder to
        infer: measured over 434,720 digits, conditioning on the partition and
        the pass is worth -4.76% where the column inside the partition is
        worth +0.27%, so the pass is the coordinate that has to be visible.  */
    for (k = 0; k < 8; k++) snprintf(tag_d[i][k], sizeof **tag_d, "g%u_%u", i, k);
  }
}

/*  A record that crosses unchanged, but whose value the traversal also needs.

    The tee is held off for the read.  Most of the stream reaches the output
    through the tee, and a record read with the tee still attached would be
    written by it and again by the put below -- which is invisible on a stream
    whose packets each fit in one page, and doubles page.spill on one whose
    packets do not.  */
static i64 pass(const char * tag) {
  tsv * save = tr_in->tee;
  i64 v;
  tr_in->tee = nullptr;
  v = tr_in->get(tag);
  tr_in->tee = save;
  tr_out->put(tag, v);
  return v;
}
static u32 pass_u(const char * tag, u64 hi) {
  tsv * save = tr_in->tee;
  u32 v;
  tr_in->tee = nullptr;
  v = tr_in->get_u(tag, hi);
  tr_in->tee = save;
  tr_out->put(tag, (i64) v);
  return v;
}

/*  A value written under one tag and read back under another: `a` is the tag
    the original stream uses, `b` the one the transformed stream uses.  */
static i64 xf(const char * a, const char * b, u64 hi) {
  tsv * save = tr_in->tee;
  i64 v;
  tr_in->tee = nullptr;
  v = (i64) tr_in->get_u(tr_enc ? a : b, hi);
  tr_in->tee = save;
  tr_out->put(tr_enc ? b : a, v);
  return v;
}

/*  Classification slots, laid out as io.inc lays them: vch rows of np + pv,
    because the class walk reads pv of them at a time and the last read runs
    past the partition count.  */
constexpr sz TR_CSMAX = 1u << 20;
static u8 tr_cl[TR_CSMAX];

/*  One residue partition: psz digits, and under -R they are a row of their
    own.  balrogg emits them one record at a time and lets consecutive records
    merge, so a row there is however many partitions ran together between two
    class records.  */
/*  A side channel for measuring, not for the format: mode c can write out
    what each digit's coordinates are, so that the question of which of them a
    coder should condition on is settled by counting rather than by taste.  */
static FILE * tr_dump;
static u32 tr_dpart, tr_dpass, tr_dch;

static void tr_part(u32 rno, u32 pss, u32 psz) {
  u32 i;
  if (tr_dump && tr_enc) {
    tsv * in = tr_in;
    for (i = 0; i < psz; i++) {
      i64 d = in->get("res.digit");
      fprintf(tr_dump, "%u %u %u %u %u %" PRId64 "\n", rno, tr_dpass, tr_dch,
              tr_dpart, i, d);
      tr_out->put(tag_d[rno][pss], d);
    }
    tr_out->flush_row();
    return;
  }
  if (tr_img && !gimg.full()) {
    /*  the row is wanted as pixels as well as records, and the records are
        consumed once, so it is built here rather than read back  */
    u32 k = psz > TR_BMPW ? TR_BMPW : psz;
    for (i = 0; i < psz; i++) {
      i64 d = tr_in->get(tr_enc || !tr_rows ? "res.digit" : tag_d[rno][pss]);
      if (i < k) bmp_div(d, tr_px + i * 3);
      tr_out->put(tr_enc && tr_rows ? tag_d[rno][pss] : "res.digit", d);
    }
    if (tr_enc && tr_rows) tr_out->flush_row();
    gimg.row(tr_px, psz);
    return;
  }
  if (!tr_rows) { for (i = 0; i < psz; i++) pass("res.digit");  return; }
  if (tr_enc) {
    for (i = 0; i < psz; i++) tr_out->put(tag_d[rno][pss], tr_in->get("res.digit"));
    tr_out->flush_row();
  } else
    for (i = 0; i < psz; i++) tr_out->put("res.digit", tr_in->get(tag_d[rno][pss]));
}

/*  The residue walk of io.inc, over records rather than bits.  */
static void tr_residue(u32 rno, const u8 * nz, u32 nch, u32 n) {
  vd_res * r = su.rs + rno;
  vd_book * cb = su.bk + r->cbook;
  u32 vch, end, np, pv, pass_, pc, i, j, k, w, nc = 0;
  for (i = 0; i < nch; i++) if (nz[i]) nc++;
  if (!nc) return;
  if (r->type == 2) { vch = 1;  end = blr_min(r->end, n * nch); }
  else { vch = nc;  end = blr_min(r->end, n); }
  if (end <= r->beg) return;
  np = (end - r->beg) / r->psz;
  if (!np) return;
  pv = cb->dim;
  w = np + pv;
  FATAL_UNLESS((sz) vch * w <= TR_CSMAX,
               "residue needs %" PRIu64 " classification slots, limit %" PRIu64,
               (u64) ((sz) vch * w), (u64) TR_CSMAX);
  for (pass_ = 0; pass_ < 8; pass_++) {
    pc = 0;
    while (pc < np) {
      if (!pass_)
        for (j = 0; j < vch; j++) {
          for (k = 0; k < pv; k++)
            tr_cl[j * w + pc + k] =
              (u8) (tr_rows ? xf("res.class", tag_c[rno], VD_MAXRCL)
                            : (i64) pass_u("res.class", VD_MAXRCL));
          if (tr_rows && tr_enc) tr_out->flush_row();
        }
      for (k = 0; k < pv; k++) {
        if (pc >= np) break;
        for (j = 0; j < vch; j++) {
          i32 bn;
          FATAL_UNLESS(tr_cl[j * w + pc] < r->ncl, "residue class out of range");
          bn = r->book[tr_cl[j * w + pc]][pass_];
          tr_dpart = pc;  tr_dpass = pass_;  tr_dch = j;
          if (bn >= 0) tr_part(rno, pass_, r->psz);
        }
        pc++;
      }
    }
  }
}

/*  Floor curves, then the residues, for one audio packet.  */
static void tr_payload(u32 mode) {
  vd_map * mp = su.mp + su.mdmap[mode];
  u32 n = (su.blockflag[mode] ? su.bs1 : su.bs0) / 2;
  u32 ch = su.ch, k, i, j, m;
  u8 nz[VD_MAXCH], sub[VD_MAXCH];
  memset(nz, 0, sizeof nz);
  for (k = 0; k < ch; k++) {
    u32 fno = mp->fl[mp->mux[k]];
    vd_floor * f = su.fl + fno;
    u32 u = pass_u("flr.used", 2);
    nz[k] = (u8) u;
    if (!u) continue;
    if (tr_img && !yimg.full()) {
      u32 kk = f->posts > TR_BMPW ? TR_BMPW : f->posts;
      for (i = 0; i < f->posts; i++) {
        i64 y = tr_in->get(tr_enc || !tr_rows ? "flr.y" : tag_y[fno]);
        if (i < kk) bmp_seq(y, tr_px + i * 3);
        tr_out->put(tr_enc && tr_rows ? tag_y[fno] : "flr.y", y);
      }
      if (tr_enc && tr_rows) tr_out->flush_row();
      yimg.row(tr_px, f->posts);
      continue;
    }
    if (!tr_rows) { for (i = 0; i < f->posts; i++) pass("flr.y"); }
    else if (tr_enc) {
      for (i = 0; i < f->posts; i++)
        tr_out->put(tag_y[fno], tr_in->get("flr.y"));
      tr_out->flush_row();
    } else
      for (i = 0; i < f->posts; i++)
        tr_out->put("flr.y", tr_in->get(tag_y[fno]));
  }
  for (i = 0; i < mp->nstep; i++)
    if (nz[mp->mag[i]] || nz[mp->ang[i]])
      nz[mp->mag[i]] = nz[mp->ang[i]] = 1;
  for (i = 0; i < mp->sub; i++) {
    m = 0;
    for (j = 0; j < ch; j++) if (mp->mux[j] == i) sub[m++] = nz[j];
    if (m) tr_residue(mp->rs[i], sub, m, n);
  }
}

/*  The granule goes out as a difference from the page before.

    It is fully derivable in principle -- a page's granule is the decodable
    position at the end of the last packet that completes on it, and a packet
    with window n and predecessor M puts out (M + n) / 2 samples, all of which
    is in the stream.  But the header is written before the page's packets are
    walked, so the exact figure is not available where it is needed, and
    getting it there means holding a page's records back until its packets
    have been read.  The difference costs no lookahead and gives up little:
    it is a small number from a small set -- tsv2wav counted 120 distinct
    values over 3,187 pages, a third of them exactly 4096 -- which is a
    handful of bits to the coder downstream, against zero for the exact form.
    META.md has the same accounting from the other side.  */
static i64 pg_prev;

/*  One audio packet, in io.inc's order.  */
static u32 tr_prevW;                      /*  the block before this one  */
static void tr_audio(void) {
  u32 md = pass_u("aud.mode", su.nmd);
  FATAL_UNLESS(md < su.nmd, "audio names mode %" PRIu64, (u64) md);
  if (su.blockflag[md]) {
    /*  wprev names the block before this one, which both sides have already
        walked.  A stream that says otherwise is not wrong -- the decoder uses
        what it is told -- so the disagreement is carried instead.  */
    if (!tr_win) { pass("aud.wprev");  pass("aud.wnext"); }
    else if (tr_enc) {
      u32 wp = tr_in->get_u("aud.wprev", 2);
      if (wp != tr_prevW) tr_out->put("aud.wpfix", (i64) wp);
      tr_out->put("aud.wnext", (i64) tr_in->get_u("aud.wnext", 2));
    } else {
      u32 wp = tr_prevW;
      if (!strcmp(tr_in->peek(), "aud.wpfix")) wp = (u32) tr_in->get("aud.wpfix");
      tr_out->put("aud.wprev", (i64) wp);
      tr_out->put("aud.wnext", (i64) tr_in->get_u("aud.wnext", 2));
    }
  }
  tr_prevW = su.blockflag[md];
  tr_payload(md);
}

/*  Just enough of a page header to find the packet boundaries.  Three of its
    fields are not information: the type follows from position (BOS on a
    link's first page, EOS on its last, continued when the page before ended
    mid-packet), the sequence counts from zero within a link, and the serial
    is one number per link rather than one per page.  */
struct tr_pg {
  u32 type, glo, ghi, serial, seq, np;
  u32 plen[OGG_MAXSEG];
  int tail;

  void read_plain(tsv & t) {
    u32 i;
    type = t.get_u("page.type", 0x100);
    glo = t.get_u("page.granlo", (u64) 0x100000000);
    ghi = t.get_u("page.granhi", (u64) 0x100000000);
    serial = t.get_u("page.serial", (u64) 0x100000000);
    seq = t.get_u("page.seq", (u64) 0x100000000);
    np = t.get_u("page.npkt", OGG_MAXSEG + 1);
    for (i = 0; i < np; i++)
      plen[i] = t.get_u("page.plen", (u64) OGG_MAXSEG * OGG_MAXSEG + 1);
    tail = 0;
    if (np && !(plen[np - 1] % OGG_MAXSEG)) tail = (int) t.get_u("page.tail", 2);
  }
  void write_plain(tsv & t) const {
    u32 i;
    t.put("page.type", type);
    t.put("page.granlo", glo);   t.put("page.granhi", ghi);
    t.put("page.serial", serial);  t.put("page.seq", seq);
    t.put("page.npkt", np);
    for (i = 0; i < np; i++) t.put("page.plen", plen[i]);
    if (np && !(plen[np - 1] % OGG_MAXSEG)) t.put("page.tail", tail);
  }
  /*  The shape goes out first, because the granule is predicted from it.  */
  void write_thin(tsv & t, i64 pred) const {
    u32 i;
    t.put("page.npkt", np);
    for (i = 0; i < np; i++) t.put("page.plen", plen[i]);
    if (np && !(plen[np - 1] % OGG_MAXSEG)) t.put("page.tail", tail);
    if (type & 4) t.put("page.eos", 1);
    t.put("page.gran", (i64) (((u64) ghi << 32) | glo) - pred);
  }
  void read_thin(tsv & t, u32 ser, u32 sq, int bos, int cont) {
    u32 i;
    type = (u32) ((bos ? 2 : 0) | (cont ? 1 : 0));
    serial = ser;  seq = sq;
    np = t.get_u("page.npkt", OGG_MAXSEG + 1);
    for (i = 0; i < np; i++)
      plen[i] = t.get_u("page.plen", (u64) OGG_MAXSEG * OGG_MAXSEG + 1);
    tail = 0;
    if (np && !(plen[np - 1] % OGG_MAXSEG)) tail = (int) t.get_u("page.tail", 2);
    if (!strcmp(t.peek(), "page.eos")) { t.get("page.eos");  type |= 4; }
  }
};


/*  One pass over the stream.  The traversal is the same in both directions --
    it has to be, since the shape of what follows is decided by records
    already read -- so the two modes differ only in which tag each value
    arrives under and which of them are written at all.  */
static void tr_walk(const char * inpath, const char * outpath) {
  tsv in, out;
  tr_pg p;
  u32 link = 0;

  in.open(inpath);
  out.create(outpath);
  tr_in = &in;  tr_out = &out;
  in.tee = &out;                          /*  everything not named below  */

  while (in.get_u("link.more", 2)) {
    int done = 0, cont = 0, w = 0;
    u32 serial = 0, seq = 0;
    int prevtail = 0;
    sz spill = 0;
    pg_prev = 0;  tr_prevW = 0;
    su.have = 0;
    while (!done) {
      u32 j;
      in.tee = nullptr;
      if (!tr_page) { p.read_plain(in);  p.write_plain(out); }
      else if (tr_enc) {
        p.read_plain(in);
        FATAL_UNLESS(p.type == (u32) (((seq == 0) ? 2 : 0) | (prevtail ? 1 : 0) |
                                      (p.type & 4)) &&
                     p.seq == seq && (!seq || p.serial == serial),
                     "%s: page %" PRIu64 " does not have a canonical header; "
                     "this stream needs page.type, page.seq or page.serial "
                     "carried", inpath, (u64) seq);
        if (!seq) { serial = p.serial;  out.put("link.serial", serial); }
        p.write_thin(out, pg_prev);
        pg_prev = (i64) (((u64) p.ghi << 32) | p.glo);
      } else {
        if (!seq) serial = in.get_u("link.serial", (u64) 0x100000000);
        p.read_thin(in, serial, seq, seq == 0, prevtail);
        { i64 g = pg_prev + in.get("page.gran");
          pg_prev = g;
          p.glo = (u32) ((u64) g & 0xFFFFFFFFu);
          p.ghi = (u32) ((u64) g >> 32); }
        p.write_plain(out);
      }
      in.tee = &out;
      prevtail = p.tail;
      seq++;
      for (j = 0; j < p.np; j++) {
        sz pl = p.plen[j], len = pl;
        if (tr_trace)
          fprintf(stderr, "seq=%u j=%u np=%u tail=%d cont=%d w=%d type=%u pl=%u\n",
                  seq - 1, j, p.np, p.tail, cont, w, p.type, (u32) pl);
        if (j == 0 && (p.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill, "%s: page overruns a continued packet", inpath);
          spill -= pl;
          if (!(p.np == 1 && p.tail)) cont = 0;
          continue;
        }
        if (j == p.np - 1 && p.tail) {
          sz ex = (sz) pass_u("page.spill", (u64) 0x100000000);
          len = pl + ex;  spill = ex;  cont = 1;
        }
        if (w < 3) {
          if (w == 0) su.ident(in);
          else if (w == 1) su.comment(in, len);
          else { su_role = 0;  su_dst = nullptr;  su.setup(in);  su.have = 1;
                 tr_img_open(); }
          w++;
        } else {
          in.tee = nullptr;
          tr_audio();
          in.tee = &out;
        }
      }
      if (p.type & 4) done = 1;
    }
    link++;
    FATAL_UNLESS(link < LK_MAX, "%s: too many links", inpath);
  }
  in.tee = nullptr;
  out.close();
  in.close();
}

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
        case 'P': tr_page = 0;  break;
        case 'W': tr_win = 0;   break;
        case 'R': tr_rows = 0;  break;
        case 'n': tr_page = tr_win = tr_rows = 0;  break;
        default: goto usage;
      }
  }
  { int nf = argc - a;
    if (nf != 2 && nf != 4) goto usage;
    blr_set_prog(argv[0]);
    blr_paths_distinct(argv + a, nf);
    if (nf == 4) { tr_ybmp = argv[a + 2];  tr_gbmp = argv[a + 3]; } }
  { const char * r = getenv("TSVTRANS_BMPROWS");
    if (r) { long v = strtol(r, nullptr, 10);
             tr_bmprows = v > 0 ? (u32) v : 1; } }
  tr_enc = mode[0] == 'c';
  tr_trace = getenv("TSVTRANS_TRACE") != nullptr;
  { const char * dp = getenv("TSVTRANS_DUMP");
    if (dp && tr_enc) { tr_dump = fopen(dp, "wb");
                        if (!tr_dump) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", dp); } }
  tr_tags();
  tr_walk(argv[a], argv[a + 1]);
  if (tr_dump && fclose(tr_dump)) FATAL_CODE(BLR_EXIT_IO, "write error on the dump");
  if (tr_img) {
    u32 yr = yimg.rows, gr = gimg.rows, yw = yimg.w, gw = gimg.w;
    u32 yc = yimg.clipped, gc = gimg.clipped;
    yimg.close();  gimg.close();
    fprintf(stderr, "%s: %s %" PRIu32 "x%" PRIu32 ", %s %" PRIu32 "x%" PRIu32 "\n",
            blr_prog, tr_ybmp, yw, yr, tr_gbmp, gw, gr);
    if (yr >= tr_bmprows || gr >= tr_bmprows)
      fprintf(stderr, "%s: stopped at %" PRIu32 " rows; TSVTRANS_BMPROWS raises it\n",
              blr_prog, tr_bmprows);
    if (yc || gc)
      fprintf(stderr, "%s: %" PRIu32 " floor and %" PRIu32 " digit rows were wider "
              "than the image and were cut\n", blr_prog, yc, gc);
  }
  return BLR_EXIT_OK;
usage:
  fprintf(stderr,
    "usage: tsvtrans c [options] input.tsv output.tsv [floor.bmp digits.bmp]\n"
    "       tsvtrans d [options] output.tsv restored.tsv [floor.bmp digits.bmp]\n"
    "\n"
    "Name a third and fourth file and the floor curves and the residue digits\n"
    "are also drawn, one curve or one partition to a row of pixels.  The rows\n"
    "are taken from the walk rather than from the text, so the pictures come\n"
    "out square whatever the options above say.  A row narrower than the image\n"
    "-- a 19-post floor beside a 29-post one -- is padded in black.\n"
    "TSVTRANS_BMPROWS caps the height, by default 65536.\n"
    "\n"
    "  -P  keep the page header as it stands (type, sequence, serial, granule)\n"
    "  -W  keep aud.wprev rather than deriving it\n"
    "  -R  keep the floor and digit rows as they came, ragged\n"
    "  -n  all of the above: copy the stream through unchanged\n"
    "\n"
    "Mode d must be given the same options mode c was.\n");
  return BLR_EXIT_USAGE;
}
