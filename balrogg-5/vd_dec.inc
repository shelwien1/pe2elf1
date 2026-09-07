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

/*  One audio packet: floor curve, residue vectors, inverse coupling, the
    envelope, the inverse transform, and the lapped overlap-add that turns a
    run of blocks into PCM.

    The lap follows libvorbis's own buffer discipline.  The accumulator holds
    two half-blocks whose centres alternate between 0 and n1; a block adds its
    left half onto the tail the previous one left there and drops its right
    half at the other centre.  The overlap always runs for the shorter of the
    two half-blocks, centred inside the longer one, which is what makes a
    long/short transition line up.  */

static float vd_pcm[VD_MAXCH][MD_MAXBLK];   /*  the block being decoded  */
static float vd_acc[VD_MAXCH][MD_MAXBLK];   /*  the two-centre accumulator  */
static float vd_pnd[VD_MAXCH][MD_MAXM];     /*  output held back one block  */
static float vd_frm[VD_MAXCH];
static i32   vd_y[VD_MAXCH][VD_MAXPOST];
static u8    vd_fg[VD_MAXCH][VD_MAXPOST];
static u8    vd_nz[VD_MAXCH];
static u8    vd_used[VD_MAXCH];   /*  floors actually coded, before coupling  */
static u32   vd_cl[VD_MAXCH * (MD_MAXM + 0x10000)];
static float vd_part[MD_MAXBLK];
static float vd_flc[VD_MAXCH][MD_MAXM];   /*  the floor curve alone  */
static i64   vd_xfx[MD_MAXM];               /*  the spectrum, fixed point  */
static i64   vd_yfx[MD_MAXBLK];

struct vd_dec {
  vd_setup s;
  imdct md;
  lifted_dct4 lift;
  wav * out;
  u32 centerW;                /*  0 or n1  */
  int lW, W;                  /*  previous and current block flag  */
  int started;                /*  a block has been lapped  */
  int wp, wn;                 /*  this packet's window flags  */
  u32 pndn;                   /*  frames held back  */
  unsigned long long done;    /*  frames written for the current link  */

  /*  Per-link reset; the setup that follows fills s again.  A chained file
      restarts the lap and the granule count at every link.  */
  void link() { s.have = 0;  reset(); }
  void reset() {
    centerW = 0;  lW = W = 0;  started = 0;  pndn = 0;  done = 0;
    memset(vd_acc, 0, sizeof vd_acc);
  }
  void start(wav * w) { out = w;  reset(); }

  /*  The integer line of the Vorbis specification, multiplied in.  */
  static void render(float * d, int n, int x0, int x1, int y0, int y1) {
    int dy = y1 - y0, adx = x1 - x0, ady = dy < 0 ? -dy : dy;
    int base = dy / adx, sy = dy < 0 ? base - 1 : base + 1;
    int x = x0, y = y0, err = 0, ab = base < 0 ? -base : base;
    ady -= ab * adx;
    if (n > x1) n = x1;
    if (x < n) d[x] *= FLOOR_DB[y];
    while (++x < n) {
      err += ady;
      if (err >= adx) { err -= adx;  y += sy; } else y += base;
      d[x] *= FLOOR_DB[y];
    }
  }

  /*  Unwrap the coded posts against their neighbour prediction, then draw
      the curve onto the residue vector.  */
  void floor_curve(u32 c, vd_floor * f, u32 n) {
    draw(vd_y[c], vd_fg[c], vd_pcm[c], f, n);
  }

  /*  The same curve drawn on its own, so the analysis can divide it out.  */
  /*  The floor curve alone.  `unw`, when given, receives the unwrapped post
      values -- what the encoder's fit produced, before the neighbour
      prediction turned them into the residuals the stream carries.  */
  void floor_only(u32 c, float * o, vd_floor * f, u32 n, i32 * unw = nullptr) {
    i32 y2[VD_MAXPOST];  u8 fg2[VD_MAXPOST];  u32 q;
    memcpy(y2, vd_y[c], sizeof y2);
    for (q = 0; q < n; q++) o[q] = 1.0f;
    draw(y2, fg2, o, f, n);
    if (unw) memcpy(unw, y2, f->posts * sizeof *unw);
  }

  void draw(i32 * y, u8 * fg, float * o, vd_floor * f, u32 n) {
    u32 i;
    int lx = 0, hx = 0, ly, hy;
    fg[0] = fg[1] = 1;
    for (i = 2; i < f->posts; i++) {
      u32 l = f->lo[i - 2], h = f->hi[i - 2];
      i32 dy = y[h] - y[l], adx = (i32) f->x[h] - (i32) f->x[l];
      i32 ady = dy < 0 ? -dy : dy;
      i32 pred = ady * ((i32) f->x[i] - (i32) f->x[l]) / adx;
      i32 hiroom, loroom, room, val;
      pred = dy < 0 ? y[l] - pred : y[l] + pred;
      hiroom = (i32) f->quant - pred;  loroom = pred;
      room = (hiroom < loroom ? hiroom : loroom) << 1;
      val = y[i];
      if (val) {
        if (val >= room) val = hiroom > loroom ? val - loroom
                                               : -1 - (val - hiroom);
        else val = (val & 1) ? -((val + 1) >> 1) : (val >> 1);
        y[i] = (val + pred) & 0x7FFF;
        fg[i] = 1;  fg[l] = 1;  fg[h] = 1;
      } else { y[i] = pred;  fg[i] = 0; }
    }
    ly = y[0] * (i32) f->mult;
    ly = ly < 0 ? 0 : ly > 255 ? 255 : ly;
    for (i = 1; i < f->posts; i++) {
      u32 cur = f->srt[i];
      if (!fg[cur]) continue;
      hy = y[cur] * (i32) f->mult;
      hy = hy < 0 ? 0 : hy > 255 ? 255 : hy;
      hx = (int) f->x[cur];
      render(o, (int) n, lx, hx, ly, hy);
      lx = hx;  ly = hy;
    }
    for (i = (u32) hx; i < n; i++) o[i] *= FLOOR_DB[ly];
  }

  /*  A residue digit is the centred multiplicand, so the scalar it stands
      for needs no codebook lookup.  */
  void symbol(tsv & t, vd_book * b, float * v) {
    u32 k;
    double last = 0;
    for (k = 0; k < b->dim; k++) {
      long long d = t.get("res.digit");
      double q = (double) (d + (long long) b->off) * (double) b->delta
               + (double) b->minval + last;
      if (b->seq) last = q;
      v[k] = (float) q;
    }
  }

  /*  Placed the way the matching libvorbis decodepart would place it.  */
  void partition(tsv & t, vd_res * r, vd_book * b, float ** ch, u32 nch,
                 u32 pos) {
    u32 i, j, dim = b->dim, psz = r->psz;
    FATAL_UNLESS(dim > 0 && b->lookup == 1, "tsv2wav: residue book has no grid");
    if (r->type == 0) {
      u32 step = psz / dim, o;
      FATAL_UNLESS(step * dim == psz, "tsv2wav: residue 0 partition size");
      for (i = 0; i < step; i++) symbol(t, b, vd_part + i * dim);
      for (i = 0, o = 0; i < dim; i++, o += step)
        for (j = 0; j < step; j++) ch[0][pos + o + j] += vd_part[j * dim + i];
    } else if (r->type == 1) {
      for (i = 0; i < psz; i += dim) {
        symbol(t, b, vd_part);
        for (j = 0; j < dim && i + j < psz; j++) ch[0][pos + i + j] += vd_part[j];
      }
    } else {
      u32 cp = pos % nch, ix = pos / nch;
      for (i = 0; i < psz; i += dim) {
        symbol(t, b, vd_part);
        for (j = 0; j < dim; j++) {
          ch[cp][ix] += vd_part[j];
          if (++cp == nch) { cp = 0;  ix++; }
        }
      }
    }
  }

  void residue(tsv & t, u32 rno, float ** bundle, const u8 * nz, u32 nch,
               u32 n) {
    vd_res * r = s.rs + rno;
    vd_book * cb = s.bk + r->cbook;
    float * act[VD_MAXCH];
    u32 vch, end, np, pv, w, pass, pc, i, j, k, nc = 0;
    for (i = 0; i < nch; i++) if (nz[i]) act[nc++] = bundle[i];
    if (!nc) return;
    if (r->type == 2) { vch = 1;  end = r->end < n * nch ? r->end : n * nch; }
    else { vch = nc;  end = r->end < n ? r->end : n; }
    if (end <= r->beg) return;
    np = (end - r->beg) / r->psz;
    if (!np) return;
    pv = cb->dim;
    w = np + pv;
    FATAL_UNLESS((sz) vch * w <= sizeof vd_cl / sizeof *vd_cl,
                 "tsv2wav: residue needs %lu classification slots",
                 (unsigned long) ((sz) vch * w));
    for (pass = 0; pass < 8; pass++) {
      pc = 0;
      while (pc < np) {
        if (!pass)
          for (j = 0; j < vch; j++)
            for (k = 0; k < pv; k++)
              vd_cl[j * w + pc + k] = t.get_u("res.class", 16);
        for (k = 0; k < pv; k++) {
          if (pc >= np) break;
          for (j = 0; j < vch; j++) {
            u32 c = vd_cl[j * w + pc];
            i32 bn;
            FATAL_UNLESS(c < r->ncl, "tsv2wav: residue class %lu, limit %lu",
                         (unsigned long) c, (unsigned long) r->ncl);
            bn = r->book[c][pass];
            if (bn < 0) continue;
            if (r->type == 2)
              partition(t, r, s.bk + bn, bundle, nch, r->beg + pc * r->psz);
            else
              partition(t, r, s.bk + bn, act + j, 1, r->beg + pc * r->psz);
          }
          pc++;
        }
      }
    }
  }

  void audio(tsv & t) {
    u32 mo, n, i, j, k, ch = s.ch;
    vd_map * mp;
    int Wn;
    FATAL_UNLESS(s.have, "tsv2wav: audio before setup");
    mo = t.get_u("aud.mode", s.nmd);
    Wn = s.blockflag[mo];
    wp = wn = 0;
    if (Wn) { wp = (int) t.get_u("aud.wprev", 2);  wn = (int) t.get_u("aud.wnext", 2); }
    mp = s.mp + s.mdmap[mo];
    n = (Wn ? s.bs1 : s.bs0) / 2;

    for (k = 0; k < ch; k++) {
      vd_floor * f = s.fl + mp->fl[mp->mux[k]];
      int used = (int) t.get_u("flr.used", 2);
      vd_nz[k] = (u8) used;
      memset(vd_pcm[k], 0, n * sizeof *vd_pcm[k]);
      if (!used) continue;
      for (i = 0; i < f->posts; i++)
        vd_y[k][f->srt[i]] = (i32) t.get_u("flr.y", 0x8000);
    }
    /*  Coupling dirties the non-silent list for the residue decode only; the
        envelope still goes only where a floor was actually coded.  */
    memcpy(vd_used, vd_nz, ch);
    for (i = 0; i < mp->nstep; i++)
      if (vd_nz[mp->mag[i]] || vd_nz[mp->ang[i]])
        vd_nz[mp->mag[i]] = vd_nz[mp->ang[i]] = 1;

    for (i = 0; i < mp->sub; i++) {
      float * bundle[VD_MAXCH];
      u8 nzb[VD_MAXCH];
      u32 m = 0;
      for (j = 0; j < ch; j++)
        if (mp->mux[j] == i) { nzb[m] = vd_nz[j];  bundle[m++] = vd_pcm[j]; }
      if (m) residue(t, mp->rs[i], bundle, nzb, m, n);
    }

    for (i = mp->nstep; i-- > 0; ) {
      float * pm = vd_pcm[mp->mag[i]], * pa = vd_pcm[mp->ang[i]];
      for (j = 0; j < n; j++) {
        float mg = pm[j], an = pa[j];
        if (mg > 0) { if (an > 0) { pm[j] = mg;  pa[j] = mg - an; }
                      else        { pa[j] = mg;  pm[j] = mg + an; } }
        else        { if (an > 0) { pm[j] = mg;  pa[j] = mg + an; }
                      else        { pa[j] = mg;  pm[j] = mg - an; } }
      }
    }
    for (k = 0; k < ch; k++) {
      if (vd_used[k]) floor_curve(k, s.fl + mp->fl[mp->mux[k]], n);
      else memset(vd_pcm[k], 0, n * sizeof *vd_pcm[k]);
    }
    for (k = 0; k < ch; k++) {
      /*  The transform runs in fixed point through the lifting DCT-IV, so
          the same spectrum always yields the same samples and the step is
          exactly invertible.  */
      for (i = 0; i < n; i++) vd_xfx[i] = fx((double) vd_pcm[k][i]);
      lift.backward(vd_xfx, vd_yfx, n);
      for (i = 0; i < 2 * n; i++)
        vd_pcm[k][i] = (float) ((double) vd_yfx[i] / (double) ((i64) 1 << F));
    }
    lap(Wn, n);
  }

  void lap(int Wn, u32 n) {
    u32 ch = s.ch, c, i;
    u32 n0 = s.bs0 / 2, n1 = s.bs1 / 2, thisC, prevC, avail;
    lW = W;  W = Wn;
    if (centerW) { thisC = n1;  prevC = 0; } else { thisC = 0;  prevC = n1; }
    for (c = 0; c < ch; c++) {
      float * a = vd_acc[c] + prevC, * p = vd_pcm[c];
      if (lW) {
        if (W) {
          const float * w = md.win[1];
          for (i = 0; i < n1; i++) a[i] = a[i] * w[n1 - i - 1] + p[i] * w[i];
        } else {
          const float * w = md.win[0];
          float * q = a + n1 / 2 - n0 / 2;
          for (i = 0; i < n0; i++) q[i] = q[i] * w[n0 - i - 1] + p[i] * w[i];
        }
      } else {
        const float * w = md.win[0];
        if (W) {
          const float * r = p + n1 / 2 - n0 / 2;
          for (i = 0; i < n0; i++) a[i] = a[i] * w[n0 - i - 1] + r[i] * w[i];
          for (; i < n1 / 2 + n0 / 2; i++) a[i] = r[i];
        } else
          for (i = 0; i < n0; i++) a[i] = a[i] * w[n0 - i - 1] + p[i] * w[i];
      }
      memcpy(vd_acc[c] + thisC, p + n, n * sizeof *p);
    }
    centerW = centerW ? 0 : n1;
    if (!started) { started = 1;  return; }
    avail = (lW ? n1 : n0) / 2 + (W ? n1 : n0) / 2;
    hold(prevC, avail);
  }

  /*  Output runs one block behind so the last one can be cut to the granule
      position the stream ends on.  */
  void flush(u32 keep) {
    u32 c, i;
    for (i = 0; i < keep; i++) {
      for (c = 0; c < s.ch; c++) vd_frm[c] = vd_pnd[c][i];
      out->frame(vd_frm);
    }
    done += keep;  pndn = 0;
  }

  void hold(u32 off, u32 count) {
    u32 c;
    flush(pndn);
    for (c = 0; c < s.ch; c++)
      memcpy(vd_pnd[c], vd_acc[c] + off, count * sizeof(float));
    pndn = count;
  }

  void finish(unsigned long long granule) {
    u32 keep = pndn;
    if (granule >= done && granule - done < (unsigned long long) pndn)
      keep = (u32) (granule - done);
    flush(keep);
  }
};
