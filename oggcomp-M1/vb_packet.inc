constexpr u32 VB_MAXBLK = 8192;
constexpr u32 VB_MAXDIM = 0xFFFF;
constexpr sz VB_YSMAX = (sz)VB_MAXCH * VB_MAXPOST;
constexpr sz VB_CSMAX = (sz)VB_MAXCH * (VB_MAXBLK / 2 + VB_MAXDIM);
static u32 vb_ys[VB_YSMAX];
static u32 vb_cs[VB_CSMAX];
static u32 lk1(u32 entries, u32 dim) {
  u32 v = 0, i, p;
  for(;;) {
    p = 1;
    Fi(
        dim, if(p > entries / (v + 1)) {
          p = entries + 1;
          break;
        } p *= v + 1);
    if(p > entries)
      return v;
    v++;
  }
}
constexpr u32 QUANT[4] = {256, 128, 86, 64};
//  The sink is a type parameter so that the same parser runs against the
//  model (io) and against null_sink, which is how the encoder finds out
//  whether a page parses before it has spent a bit on it.
template <class S> struct io_t {
  int enc;
  S &t;
  vb_ctx &v;
  u8 *b;
  sz len, pos;
  io_t(vb_ctx &vv, S &tt, int e, u8 *pkt, sz n) : enc(e), t(tt), v(vv), b(pkt), len(n), pos(0) {
    if(!e)
      memset(pkt, 0, n);
  }
  //  The bits after the last field.  A packet is whole bytes, so there are
  //  up to seven of them always, and libvorbis under a minimum bitrate pads
  //  with many more; they are zero from every encoder there is, and the
  //  decoder gets zero for free -- the packet is written into a buffer of
  //  its length, cleared.  So a zero tail costs one symbol.  A nonzero one
  //  is coded byte by byte, from the byte the last field ended in: the
  //  whole byte, parsed bits included, so the decoder can just store it.
  void tail() {
    sz i, k = pos >> 3;
    u32 nz = 0;
    if(enc) {
      if(pos & 7)
        nz |= b[k] >> (pos & 7);
      for(i = k + ((pos & 7) ? 1 : 0); i < len && !nz; i++)
        nz |= b[i];
    }
    if(t.padnz(nz != 0))
      for(i = k; i < len; i++)
        b[i] = (u8)t.raw(enc ? b[i] : 0);
    pos = len * 8;
  }
  u32 bget(int n) {
    u32 r = 0;
    int i;
    VB_IF(pos + (sz)n > len * 8)("vorbis: packet ends inside a field");
    for(i = 0; i < n; i++, pos++)
      r |= (u32)((b[pos >> 3] >> (pos & 7)) & 1) << i;
    return r;
  }
  void bput(int n, u32 x) {
    int i;
    VB_IF(pos + (sz)n > len * 8)("vorbis: packet ends inside a field");
    for(i = 0; i < n; i++, pos++)
      b[pos >> 3] |= (u8)(((x >> i) & 1) << (pos & 7));
  }
  INLINE u32 bget1() {
    u32 r;
    VB_IF(pos >= len * 8)("vorbis: packet ends inside a codeword");
    r = (b[pos >> 3] >> (pos & 7)) & 1;
    pos++;
    return r;
  }
  INLINE void bput1(u32 bit) {
    VB_IF(pos >= len * 8)("vorbis: packet ends inside a codeword");
    b[pos >> 3] |= (u8)(bit << (pos & 7));
    pos++;
  }
  INLINE u32 val(const char *tag, u32 x, u64 hi) {
    if(!enc)
      return t.hdr(tag, 0, hi);
    VB_IF((u64)x >= hi)("vorbis: %s is out of range", tag);
    return t.hdr(tag, x, hi);
  }
  INLINE i32 digit(i32 x, i32 lim) {
    i64 r;
    if(enc) {
      VB_IF(x < -lim || x > lim)("vorbis: res.digit is out of range");
      return (i32)t.digit(x);
    }
    r = t.digit(0);
    VB_UNLESS(r >= -lim && r <= lim, "vorbis: res.digit is out of range");
    return (i32)r;
  }
  u32 fld(const char *tag, int n, int w) {
    u32 x;
    u64 hi = w >= 32 ? (u64)0x100000000 : (u64)1 << w;
    if(enc) {
      x = bget(n);
      VB_UNLESS(w >= 32 || !(x >> w), "vorbis: %s value %" PRIu64 " exceeds %d bits", tag, (u64)x, w);
      return t.hdr(tag, x, hi);
    }
    x = t.hdr(tag, 0, hi);
    bput(n, x);
    return x;
  }
  void cst(int n, u32 k) {
    if(enc) {
      u32 x = bget(n);
      VB_UNLESS(x == k, "vorbis: fixed field %" PRIu64 ", expected %" PRIu64, (u64)x, (u64)k);
    } else
      bput(n, k);
  }
  u32 flt(const char *ts, const char *te, const char *tm) {
    u32 w = enc ? bget(32) : 0, s, e, m;
    s = val(ts, enc ? w >> 31 : 0, 2);
    e = val(te, enc ? (w >> 21) & 0x3FF : 0, 0x400);
    m = val(tm, enc ? w & 0x1FFFFF : 0, 0x200000);
    w = (s << 31) | (e << 21) | m;
    if(!enc)
      bput(32, w);
    return w;
  }
  u32 bk_get(vb_book *bk) {
    u32 idx = 0;
    i32 nx;
    for(;;) {
      nx = bk->nd[2 * idx + bget1()];
      VB_IF(nx == 0)("vorbis: the packet holds no such codeword");
      if(nx < 0)
        return (u32)(-nx - 1);
      idx = (u32)nx;
    }
  }
  void bk_put(vb_book *bk, u32 e) {
    u32 k;
    VB_UNLESS(e < bk->ent && bk->len[e], "vorbis: codebook has no entry %" PRIu64, (u64)e);
    for(k = bk->len[e]; k > 0; k--)
      bput1((bk->code[e] >> (k - 1)) & 1);
  }
  void lengths(vb_book *k) {
    u32 i, got, cur, n, ent = k->ent;
    int used;
    if(fld("cb.ordered", 1, 1)) {
      got = 0;
      cur = fld("cb.ordlen", 5, 5) + 1;
      while(got < ent) {
        n = fld("cb.ordrun", (int)ogc_ilog(ent - got), (int)ogc_ilog(ent - got));
        VB_UNLESS(n <= ent - got, "vorbis: ordered run overruns its book");
        VB_UNLESS(cur <= 32, "vorbis: ordered lengths run past 32");
        for(i = got; i < got + n; i++)
          k->len[i] = (u8)cur;
        got += n;
        cur++;
      }
      return;
    }
    if(!fld("cb.sparse", 1, 1)) {
      Fi(ent, k->len[i] = (u8)(fld("cb.len", 5, 5) + 1));
      return;
    }
    used = (int)fld("cb.used", 1, 1);
    Fi(ent, if(i) used = (int)fld("cb.used", 1, 1); if(used) k->len[i] = (u8)(fld("cb.len", 5, 5) + 1));
  }
  void codebooks() {
    vb_setup *s = v.cur;
    u32 nb, j, lk, vb, i;
    vb_book *k;
    nb = fld("cb.count", 8, 8) + 1;
    VB_UNLESS(nb <= VB_MAXBOOK, "vorbis: %" PRIu64 " codebooks", (u64)nb);
    s->nbk = nb;
    Fj(nb, k = s->bk + j; cst(24, 0x564342); k->dim = fld("cb.dim", 16, 16); k->ent = fld("cb.entries", 24, 24); VB_UNLESS(k->dim > 0, "vorbis: codebook %" PRIu64 " has dimension 0", (u64)j); VB_UNLESS(k->ent <= ((u32)1 << 20), "vorbis: codebook %" PRIu64 " has %" PRIu64 " entries", (u64)j, (u64)k->ent); VB_UNLESS(ogc_ilog(k->dim) + ogc_ilog(k->ent) <= 24, "vorbis: codebook %" PRIu64 " size %" PRIu64 " x %" PRIu64 " is unsupported", (u64)j, (u64)k->dim, (u64)k->ent); k->len = vb_alloc<u8>(k->ent); memset(k->len, 0, k->ent); k->code = vb_alloc<u32>(k->ent); lengths(k); k->words(); k->tree(); lk = fld("cb.lookup", 4, 2); k->look = (u8)lk; if(!lk) continue; VB_UNLESS(lk == 1, "vorbis: unsupported codebook lookup type %" PRIu64, (u64)lk); flt("cb.min.sign", "cb.min.exp", "cb.min.mant"); flt("cb.dlt.sign", "cb.dlt.exp", "cb.dlt.mant"); vb = fld("cb.valbits", 4, 4) + 1; fld("cb.sequence", 1, 1); k->nv = lk1(k->ent, k->dim); k->mult = vb_alloc<u32>(k->nv ? k->nv : 1);
       Fi(k->nv, k->mult[i] = fld("cb.mult", (int)vb, (int)vb)); VB_UNLESS(k->nv > 0, "vorbis: codebook %" PRIu64 " has an empty lookup table", (u64)j); k->grid());
  }
  void floors() {
    vb_setup *s = v.cur;
    u32 nt, nf, k, i, j, nc, sub, sb;
    vb_floor *q;
    nt = fld("flr.ntime", 6, 6) + 1;
    Fi(nt, cst(16, 0));
    nf = fld("flr.count", 6, 6) + 1;
    VB_UNLESS(nf <= VB_MAXFLOOR, "vorbis: %" PRIu64 " floors", (u64)nf);
    s->nfl = nf;
    Fk(nf, q = s->fl + k; VB_UNLESS(fld("flr.type", 16, 1) == 1, "vorbis: floor type 0 is unsupported"); q->parts = fld("flr.parts", 5, 5); VB_UNLESS(q->parts <= VB_MAXPART, "vorbis: floor has %" PRIu64 " partitions", (u64)q->parts); nc = 0; Fi(q->parts, q->pcls[i] = (u8)fld("flr.pclass", 4, 4); VB_UNLESS(q->pcls[i] < VB_MAXCLASS, "vorbis: floor class %" PRIu64, (u64)q->pcls[i]); if((u32)q->pcls[i] + 1 > nc) nc = q->pcls[i] + 1); Fi(nc, q->cdim[i] = (u8)(fld("flr.cdim", 3, 3) + 1); sub = fld("flr.csub", 2, 2); q->csub[i] = (u8)sub; q->cbook[i] = -1; if(sub) q->cbook[i] = (i32)fld("flr.cbook", 8, 8); Fj((u32)1 << sub, sb = fld("flr.sbook", 8, 8); q->csb[i][j] = (i32)sb - 1)); q->mult = fld("flr.mult", 2, 2) + 1; q->quant = QUANT[q->mult - 1]; q->rng = fld("flr.rangebits", 4, 4); q->x[0] = 0; q->x[1] = (u32)1 << q->rng; q->posts = 2;
       Fi(q->parts, Fj(q->cdim[q->pcls[i]], VB_UNLESS(q->posts < VB_MAXPOST, "vorbis: floor has over %" PRIu64 " posts", (u64)VB_MAXPOST); q->x[q->posts++] = fld("flr.x", (int)q->rng, (int)q->rng))); q->sort());
  }
  void residues() {
    vb_setup *s = v.cur;
    u32 nr, k, i, j, bno, c;
    vb_res *q;
    nr = fld("res.count", 6, 6) + 1;
    VB_UNLESS(nr <= VB_MAXRES, "vorbis: %" PRIu64 " residues", (u64)nr);
    s->nrs = nr;
    Fk(nr, q = s->rs + k; q->type = fld("res.type", 16, 2); VB_UNLESS(q->type <= 2, "vorbis: residue type above 2"); q->beg = fld("res.begin", 24, 24); q->end = fld("res.end", 24, 24); q->psz = fld("res.partsize", 24, 24) + 1; q->ncl = fld("res.nclass", 6, 6) + 1; VB_UNLESS(q->ncl <= VB_MAXRCL, "vorbis: residue has %" PRIu64 " classifications", (u64)q->ncl); q->cbook = fld("res.classbook", 8, 8); Fi(
           q->ncl, c = 0; if(enc) {
             c = bget(3);
             if(bget(1))
               c |= bget(5) << 3;
           } c = val("res.cascade", c, 256);
           q->casc[i] = (u8)c; if(!enc) {
             bput(3, c & 7);
             bput(1, c >> 3 ? 1 : 0);
             if(c >> 3)
               bput(5, c >> 3);
           });
       Fi(q->ncl, Fj(8, q->book[i][j] = -1; if(!(q->casc[i] & ((u32)1 << j))) continue; bno = fld("res.book", 8, 8); q->book[i][j] = (i32)bno)));
  }
  void mappings(u32 ch) {
    vb_setup *s = v.cur;
    u32 nm, j, i, sub = 1, steps = 0, nmode, fl;
    vb_map *q;
    nm = fld("map.count", 6, 6) + 1;
    VB_UNLESS(nm <= VB_MAXMAP, "vorbis: %" PRIu64 " mappings", (u64)nm);
    s->nmp = nm;
    Fj(
        nm, q = s->mp + j; cst(16, 0); if(enc) {
          fl = bget(1);
          sub = fl ? bget(4) + 1 : 1;
          VB_UNLESS(!fl || sub > 1, "vorbis: invalid submap flag");
        } sub = val("map.submaps", enc ? sub - 1 : 0, 16) + 1;
        VB_UNLESS(sub >= 1 && sub <= VB_MAXSUB, "vorbis: mapping has %" PRIu64 " submaps", (u64)sub); if(!enc) {
          bput(1, sub > 1);
          if(sub > 1)
            bput(4, sub - 1);
        } if(enc) steps = bget(1) ? bget(8) + 1 : 0;
        steps = val("map.steps", steps, 257); if(!enc) {
          bput(1, steps != 0);
          if(steps)
            bput(8, steps - 1);
        } q->sub = sub;
        q->nstep = steps; VB_UNLESS(steps <= VB_MAXCH, "vorbis: %" PRIu64 " coupling steps", (u64)steps); Fi(steps, q->mag[i] = (u8)fld("map.mag", (int)ogc_ilog(ch - 1), (int)ogc_ilog(ch - 1)); q->ang[i] = (u8)fld("map.ang", (int)ogc_ilog(ch - 1), (int)ogc_ilog(ch - 1)); VB_UNLESS(q->mag[i] < ch && q->ang[i] < ch && q->mag[i] != q->ang[i], "vorbis: invalid coupling channels %d/%d of %" PRIu64, q->mag[i], q->ang[i], (u64)ch)); cst(2, 0); if(sub > 1) Fi(ch, q->mux[i] = (u8)fld("map.mux", 4, 4); VB_UNLESS(q->mux[i] < sub, "vorbis: channel %" PRIu64 " maps to submap %d of %" PRIu64, (u64)i, q->mux[i], (u64)sub)); Fi(sub, fld("map.smtime", 8, 8); q->fl[i] = (u8)fld("map.smfloor", 8, 8); q->rs[i] = (u8)fld("map.smres", 8, 8)));
    nmode = fld("mode.count", 6, 6) + 1;
    VB_UNLESS(nmode <= VB_MAXMODE, "vorbis: %" PRIu64 " modes", (u64)nmode);
    s->nmd = nmode;
    Fi(nmode, s->blockflag[i] = (u8)fld("mode.blockflag", 1, 1); cst(16, 0); cst(16, 0); s->mdmap[i] = (u8)fld("mode.mapping", 8, 8));
    cst(1, 1);
  }
  void ident() {
    vb_info *n = &v.i;
    u32 bs;
    VB_UNLESS(!enc || len < 8 || memcmp(b, "OpusHead", 8), "Ogg Opus is not supported");
    cst(8, 1);
    cst(8, 'v');
    cst(8, 'o');
    cst(8, 'r');
    cst(8, 'b');
    cst(8, 'i');
    cst(8, 's');
    cst(32, 0);
    n->ch = fld("id.channels", 8, 8);
    n->rate = fld("id.rate", 32, 32);
    fld("id.brmax", 32, 32);
    fld("id.brnom", 32, 32);
    fld("id.brmin", 32, 32);
    bs = fld("id.blocksize", 8, 8);
    n->bs0 = (u32)1 << (bs & 15);
    n->bs1 = (u32)1 << (bs >> 4);
    fld("id.framing", 8, 8);
    VB_UNLESS(n->ch >= 1, "vorbis: %" PRIu64 " channels", (u64)n->ch);
    VB_UNLESS(n->rate >= 1, "vorbis: sample rate 0");
    VB_UNLESS(n->bs0 >= 64 && n->bs1 >= n->bs0 && n->bs1 <= 8192, "vorbis: block sizes %" PRIu64 " and %" PRIu64, (u64)n->bs0, (u64)n->bs1);
  }
  void comment() {
    sz i;
    Fi(len, b[i] = (u8)t.hdr("cmt.byte", b[i], 256));
    pos = len * 8;
  }
  void setup() {
    vb_setup *s;
    cst(8, 5);
    cst(8, 'v');
    cst(8, 'o');
    cst(8, 'r');
    cst(8, 'b');
    cst(8, 'i');
    cst(8, 's');
    VB_UNLESS(!v.cur, "vorbis: a link has two setup headers");
    s = &vb_su;
    memset(s, 0, sizeof *s);
    v.cur = s;
    codebooks();
    floors();
    residues();
    mappings(v.i.ch);
    s->check();
  }
  void hdr(int which) {
    switch(which) {
    case 0:
      ident();
      break;
    case 1:
      comment();
      break;
    default:
      setup();
      t.setup_done(v);
      break;
    }
    tail();
  }
  int fl_get(vb_floor *f, u32 *y) {
    vb_setup *s = v.cur;
    u32 i, k, j = 2, p, cd, cs, cv;
    if(!bget(1))
      return 0;
    p = ogc_ilog(f->quant - 1);
    y[0] = bget(p);
    y[1] = bget(p);
    Fi(f->parts, u32 c = f->pcls[i], cv0; cd = f->cdim[c]; cs = f->csub[c]; cv = 0; if(cs) cv = bk_get(s->bk + f->cbook[c]); cv0 = cv; Fk(cd, i32 sb = f->csb[c][cv & (((u32)1 << cs) - 1)]; cv >>= cs; y[j + k] = sb >= 0 ? bk_get(s->bk + sb) : 0); VB_UNLESS(!(cv0 >> (cs * cd)), "vorbis: floor class codeword exceeds its dimension"); Fk(
           cd, u32 l, d = (cv0 >> (k * cs)) & (((u32)1 << cs) - 1); for(l = 0; l < d; l++) {
             u32 mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
             VB_UNLESS(y[j + k] >= mx, "vorbis: floor value uses a later subclass book than "
                                          "the first that fits");
           });
       j += cd);
    return 1;
  }
  void fl_put(vb_floor *f, const u32 *y, int used) {
    vb_setup *s = v.cur;
    u32 i, k, l, j = 2, p, cd, cs, cv, mx;
    bput(1, (u32)used);
    if(!used)
      return;
    p = ogc_ilog(f->quant - 1);
    VB_UNLESS(y[0] < ((u32)1 << p) && y[1] < ((u32)1 << p), "vorbis: floor post does not fit %" PRIu64 " bits", (u64)p);
    bput(p, y[0]);
    bput(p, y[1]);
    Fi(
        f->parts, u32 c = f->pcls[i]; cd = f->cdim[c]; cs = f->csub[c]; cv = 0; if(cs) {
          Fk(
              cd,
              for(l = 0; l < ((u32)1 << cs); l++) {
                mx = f->csb[c][l] < 0 ? 1 : s->bk[f->csb[c][l]].ent;
                if(y[j + k] < mx)
                  break;
              } VB_UNLESS(l < ((u32)1 << cs), "vorbis: no class %" PRIu64 " book fits value %" PRIu64, (u64)c, (u64)y[j + k]);
              cv |= l << (k * cs));
          bk_put(s->bk + f->cbook[c], cv);
        } Fk(cd, i32 sb = f->csb[c][cv & (((u32)1 << cs) - 1)]; cv >>= cs; if(sb >= 0) bk_put(s->bk + sb, y[j + k]));
        j += cd);
  }
  INLINE void rs_sym(vb_book *bk) {
    u32 k, e = 0, np = 1, sym = 0;
    i32 d;
    if(enc) {
      e = bk_get(bk);
      sym = e;
    }
    t.vec_begin();
    Fk(
        bk->dim,
        if(enc) {
          d = (i32)bk->mult[sym % bk->nv] - (i32)bk->off;
          sym /= bk->nv;
          digit(d, RS_DIGIT_MAX);
        } else {
          u32 p;
          d = digit(0, RS_DIGIT_MAX);
          VB_IF(!(d + (i32)bk->off >= 0 && d + (i32)bk->off < (i32)bk->base))
          ("vorbis: residue digit %" PRId64 " outside codebook grid", (i64)d);
          p = bk->inv[(u32)(d + (i32)bk->off)];
          VB_IF(p == (u32)-1)
          ("vorbis: residue digit %" PRId64 " is not a codebook multiplicand", (i64)d);
          e += p * np;
          np *= bk->nv;
        });
    if(!enc) {
      VB_IF(e >= bk->ent)("vorbis: residue has no codebook entry");
      bk_put(bk, e);
    }
  }
  HOT FLATTEN void rs_part(vb_res *r, vb_book *bk) {
    u32 i, st;
    if(r->type == 0) {
      st = r->psz / bk->dim;
      VB_IF(st * bk->dim != r->psz)
      ("vorbis: residue 0 partition %" PRIu64 " not divisible by %" PRIu64, (u64)r->psz, (u64)bk->dim);
      Fi(st, rs_sym(bk));
    } else
      for(i = 0; i < r->psz; i += bk->dim)
        rs_sym(bk);
  }
  void residue(u32 rno, const u8 *nz, u32 nch, u32 n) {
    vb_setup *s = v.cur;
    vb_res *r = s->rs + rno;
    vb_book *cb = s->bk + r->cbook;
    u32 vch, end, np, pv, pass, pc, i, j, k, cw, w;
    u32 *cl;
    u32 nc = 0;
    Fi(nch, if(nz[i]) nc++);
    if(!nc)
      return;
    if(r->type == 2) {
      vch = 1;
      end = ogc_min(r->end, n * nch);
    } else {
      vch = nc;
      end = ogc_min(r->end, n);
    }
    if(end <= r->beg)
      return;
    np = (end - r->beg) / r->psz;
    if(!np)
      return;
    pv = cb->dim;
    w = np + pv;
    VB_UNLESS((sz)vch * w <= VB_CSMAX, "vorbis: residue needs %" PRIu64 " classification slots, limit %" PRIu64, (u64)((sz)vch * w), (u64)VB_CSMAX);
    cl = vb_cs;
    t.residue_begin(rno, r, nz, nch, n);
    for(pass = 0; pass < 8; pass++) {
      pc = 0;
      while(pc < np) {
        if(!pass)
          Fj(
              vch,
              if(enc) {
                cw = bk_get(cb);
                for(k = pv; k > 0; k--) {
                  cl[j * w + pc + k - 1] = cw % r->ncl;
                  cw /= r->ncl;
                }
                VB_UNLESS(!cw, "vorbis: residue classword exceeds its partitions");
              } Fk(pv, cl[j * w + pc + k] = t.cls(rno, j, pc, k, enc ? cl[j * w + pc + k] : 0));
              if(!enc) {
                cw = 0;
                Fk(pv, cw = cw * r->ncl + cl[j * w + pc + k]);
                bk_put(cb, cw);
              });
        Fk(pv, if(pc >= np) break; Fj(
               vch, i32 bn; VB_IF(cl[j * w + pc] >= r->ncl)("vorbis: residue class %" PRIu64 ", limit %" PRIu64, (u64)cl[j * w + pc], (u64)r->ncl); bn = r->book[cl[j * w + pc]][pass]; if(bn >= 0) {
                 t.part_begin(rno, pass, j, pc, r, s->bk + bn, (u32)bn, cl[j * w + pc]);
                 rs_part(r, s->bk + bn);
               });
           pc++);
      }
    }
  }
  void payload(u32 mode) {
    vb_setup *s = v.cur;
    vb_map *mp = s->mp + s->mdmap[mode];
    u32 n = (s->blockflag[mode] ? v.i.bs1 : v.i.bs0) / 2;
    u32 ch = v.i.ch, k, i, j, m;
    u8 nz[VB_MAXCH], sub[VB_MAXCH];
    u32 *y = vb_ys;
    memset(nz, 0, sizeof nz);
    Fk(ch, u32 fno = mp->fl[mp->mux[k]]; vb_floor *f = s->fl + fno; u32 *yc = y + k * VB_MAXPOST; int u = 0; if(enc) u = fl_get(f, yc); t.floor(k, fno, f, yc, u); nz[k] = (u8)u; if(!enc) fl_put(f, yc, u));
    Fi(mp->nstep, if(nz[mp->mag[i]] || nz[mp->ang[i]]) nz[mp->mag[i]] = nz[mp->ang[i]] = 1);
    Fi(mp->sub, m = 0; Fj(ch, if(mp->mux[j] == i) sub[m++] = nz[j]); if(m) residue(mp->rs[i], sub, m, n));
  }
  void audio() {
    u32 md;
    int d, w, i;
    VB_UNLESS(v.cur && v.cur->nmd > 0, "vorbis: audio before setup");
    d = (int)ogc_ilog(v.cur->nmd - 1);
    if(enc)
      VB_UNLESS(bget(1) == 0, "vorbis: header on audio path");
    else
      bput(1, 0);
    md = enc ? bget(d) : 0;
    md = t.mode(md, v.cur->nmd);
    if(!enc)
      bput(d, md);
    if(v.cur->blockflag[md])
      Fi(2, w = (int)(i ? t.wnext(enc ? bget(1) : 0) : t.wprev(enc ? bget(1) : 0)); if(!enc) bput(1, (u32)w));
    payload(md);
    tail();
  }
};
typedef io_t<oc_model> io;
