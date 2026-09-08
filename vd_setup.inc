
/*  The Vorbis setup, read straight out of the record stream.

    Synthesis needs far less of a codebook than reconstruction does.  A
    residue digit is already the centred multiplicand `quantlist[index] - off`,
    so the scalar it stands for is just (digit + off) * delta + minval, with
    the running sum when the book is sequenced.  Codeword lengths, the decode
    tree and the multiplicand table itself are never consulted -- their
    records are read and dropped.

    The record order here mirrors balrogg's traversal.  It has to: the two
    walk the same stream.  tsv::get checks every tag, so any drift shows up
    as a named mismatch at the record where it happens rather than as noise
    in the output.  */

constexpr u32 VD_MAXBOOK  = 256;
constexpr u32 VD_MAXFLOOR = 64;
constexpr u32 VD_MAXRES   = 64;
constexpr u32 VD_MAXMAP   = 64;
constexpr u32 VD_MAXMODE  = 64;
constexpr u32 VD_MAXCLASS = 16;
constexpr u32 VD_MAXPART  = 32;
constexpr u32 VD_MAXPOST  = 256;
constexpr u32 VD_MAXRCL   = 64;
constexpr u32 VD_MAXSUB   = 16;
constexpr u32 VD_MAXCH    = 256;

/*  Largest v where (v + 1)^dim <= entries.  */
static u32 vd_lk1(u32 entries, u32 dim) {
  u32 v = 0, i, p;
  for (;;) {
    p = 1;
    for (i = 0; i < dim; i++) {
      if (p > entries / (v + 1)) { p = entries + 1;  break; }
      p *= v + 1;
    }
    if (p > entries) return v;
    v++;
  }
}

/*  A sparse codebook states a use flag per entry and a length per used entry,
    and the record stream alternates the two.  Alternating tags never group, so
    in the meta they go out as two runs instead; these hold one book's worth
    while it is turned around.  */
static u8 cb_use[1u << 20];
static u8 cb_lenv[1u << 20];

/*  Vorbis float32: sign, 10-bit exponent biased by 788, 21-bit mantissa.  */
static float vd_f32(u32 s, u32 e, u32 m) {
  double v = (double) m * pow(2.0, (double) e - 788.0);
  return (float) (s ? -v : v);
}

/*  Set by the driver so the setup readers can echo what they read.  */
static tsv * su_dst = nullptr;
static int su_role = 0;                   /*  0 synth, 1 meta, 2 restore  */
/*  The codebook section is handled by the driver: depending on the role it is
    read, captured for comparison, or generated from an index.  */
static void su_books(tsv & t, u32 nbk, struct vd_book * bk);

/*  A book whose entry count is past this keeps no length list, so the
    codeword prior below simply does not apply to it.  balrogg refuses a
    larger book outright (io.inc), so in a stream that round-trips this is
    never reached; it bounds what an arbitrary .tsv can ask us to allocate.  */
constexpr u32 VD_LENMAX = 1u << 20;

struct vd_book {
  u32 dim, ent, nv, off;
  int seq, lookup;
  float minval, delta;
  i64 min_fx, delta_fx;       /*  the same two, ready for the recovery  */

  /*  What Vorbis already knows about this book's entries and tsvcomp used to
      drop on the floor.  `clen` is the codeword length per entry (0 = the
      entry is unused), which is the encoder's own probability model: a
      complete Huffman code assigns 2^-len to an entry, and that is the one
      thing about a residue digit that no amount of history can supply.
      `mval` is the multiplicand list and `minv` its inverse, so a coded digit
      can be turned back into the grid index it came from.  */
  u8  * clen;
  u32 * mval;
  i32 * minv;
  u32   base;

  /*  A link's setup replaces the one before it, so what the previous link's
      books held goes back first.  Every vd_setup is a static object, so on
      the first pass these are the null pointers of static initialisation.  */
  void drop() {
    free(clen);  clen = nullptr;
    free(mval);  mval = nullptr;
    free(minv);  minv = nullptr;
    base = 0;
  }

  void read(tsv & t) {
    u32 i, ent2, lk, vb, mx = 0, mult;
    drop();
    dim = t.get_u("cb.dim", 0x10000);
    /*  vd_lk1 never terminates on dim == 0, and the recovery divides by the
        book's step.  balrogg rejects the same book at io.inc:208.  */
    FATAL_UNLESS(dim > 0, "codebook has dimension 0");
    ent = t.get_u("cb.entries", 0x1000000);
    /*  Lengths: kept now, not dropped -- see `clen`.  A record still has to
        be consumed whether or not there is room to store it, so the guard is
        on the write and not on the read.  */
    if (ent && ent <= VD_LENMAX) clen = (u8 *) calloc(ent, 1);
    if (t.get_u("cb.ordered", 2)) {
      u32 got = 0, cur = (u32) t.get_u("cb.ordlen", 32) + 1;
      while (got < ent) {
        u32 n = t.get_u("cb.ordrun", (u64) 0x100000000);
        FATAL_UNLESS(n <= ent - got, "ordered run overruns its book");
        if (clen && cur <= 32) for (i = got; i < got + n; i++) clen[i] = (u8) cur;
        got += n;  cur++;
      }
    } else if (!t.get_u("cb.sparse", 2)) {
      for (i = 0; i < ent; i++) {
        u32 L = (u32) t.get_u("cb.len", 32) + 1;
        if (clen) clen[i] = (u8) L;
      }
    } else if (su_role == 0) {
      int used = (int) t.get_u("cb.used", 2);
      for (i = 0; i < ent; i++) {
        if (i) used = (int) t.get_u("cb.used", 2);
        if (used) { u32 L = (u32) t.get_u("cb.len", 32) + 1;
                    if (clen) clen[i] = (u8) L; }
      }
    } else {
      /*  Turn the alternation into two runs, or back again.  */
      u32 nu = 0;
      FATAL_UNLESS(ent <= sizeof cb_use, "codebook is too large");
      t.tee = nullptr;
      if (su_role == 1) {                 /*  interleaved in, two runs out  */
        int used = (int) t.get_u("cb.used", 2);
        for (i = 0; i < ent; i++) {
          if (i) used = (int) t.get_u("cb.used", 2);
          cb_use[i] = (u8) used;
          if (used) cb_lenv[nu++] = (u8) t.get_u("cb.len", 256);
        }
        for (i = 0; i < ent; i++) su_dst->put("cb.used", cb_use[i]);
        for (i = 0; i < nu; i++) su_dst->put("cb.len", cb_lenv[i]);
      } else {                            /*  two runs in, interleaved out  */
        for (i = 0; i < ent; i++) {
          cb_use[i] = (u8) t.get_u("cb.used", 2);
          if (cb_use[i]) nu++;
        }
        for (i = 0; i < nu; i++) cb_lenv[i] = (u8) t.get_u("cb.len", 256);
        nu = 0;
        for (i = 0; i < ent; i++) {
          su_dst->put("cb.used", cb_use[i]);
          if (cb_use[i]) su_dst->put("cb.len", cb_lenv[nu++]);
        }
      }
      t.tee = su_dst;
    }
    lk = t.get_u("cb.lookup", 4);
    lookup = (int) lk;  nv = 0;  off = 0;  seq = 0;
    minval = delta = 0;  min_fx = delta_fx = 0;
    if (!lk) return;
    FATAL_UNLESS(lk == 1, "unsupported codebook lookup type %" PRIu64,
                 (u64) lk);
    {
      u32 s0 = t.get_u("cb.min.sign", 2), e0 = t.get_u("cb.min.exp", 0x400),
          m0 = t.get_u("cb.min.mant", 0x200000);
      u32 s1 = t.get_u("cb.dlt.sign", 2), e1 = t.get_u("cb.dlt.exp", 0x400),
          m1 = t.get_u("cb.dlt.mant", 0x200000);
      minval = vd_f32(s0, e0, m0);  delta = vd_f32(s1, e1, m1);
    }
    vb = t.get_u("cb.valbits", 16) + 1;
    seq = (int) t.get_u("cb.sequence", 2);
    nv = vd_lk1(ent, dim);
    ent2 = nv ? nv : 1;
    mval = (u32 *) calloc(ent2, sizeof(u32));
    for (i = 0; i < ent2; i++) {
      mult = t.get_u("cb.mult", (u64) 1 << vb);
      mval[i] = mult;
      if (mult > mx) mx = mult;
    }
    off = (mx + 1) >> 1;
    /*  balrogg's vb_book::grid, for the same reason: a digit is coded as
        `mval[index] - off` and the index is what the entry decomposes into,
        so the model needs the way back.  A duplicate multiplicand would make
        that ambiguous; balrogg refuses such a book, and here the map simply
        keeps the first, which only costs the prior its accuracy.  */
    base = mx + 1;
    if (base && base <= (1u << 24)) {
      minv = (i32 *) malloc((sz) base * sizeof(i32));
      if (minv) {
        for (i = 0; i < base; i++) minv[i] = -1;
        for (i = 0; i < ent2; i++)
          if (mval[i] < base && minv[mval[i]] < 0) minv[mval[i]] = (i32) i;
      }
    }
    min_fx = fx((double) minval);  delta_fx = fx((double) delta);
    FATAL_UNLESS(delta_fx != 0, "codebook has a zero delta");
  }
};

struct vd_floor {
  u32 parts, mult, quant, posts, rng;
  u8 pcls[VD_MAXPART];
  u8 cdim[VD_MAXCLASS], csub[VD_MAXCLASS];
  /*  Which books a class codes with: `cbk` picks the subclass codeword and
      `csb` the book each of its digits names, -1 for "this post is zero".
      balrogg reads both to find the bits; the model wants them because the
      lengths in those books are what a floor post was coded against.  */
  i32 cbk[VD_MAXCLASS], csb[VD_MAXCLASS][8];
  u8 pcl[VD_MAXPOST], ppos[VD_MAXPOST];  /*  post >= 2: its class and place  */
  u32 x[VD_MAXPOST];
  u8 srt[VD_MAXPOST];           /*  post numbers in ascending X order  */
  u8 rnk[VD_MAXPOST];           /*  and the inverse: a post's place in X  */
  u8 lo[VD_MAXPOST], hi[VD_MAXPOST];  /*  decode neighbours, for post >= 2  */

  void read(tsv & t) {
    u32 i, j, k, nc = 0, sub;
    static const u32 Q[4] = { 256, 128, 86, 64 };
    /*  A class number the floor does not use keeps whatever the previous
        link's floor left there, and tcp_build walks every class.  */
    memset(cdim, 0, sizeof cdim);  memset(csub, 0, sizeof csub);
    memset(pcl, 0, sizeof pcl);    memset(ppos, 0, sizeof ppos);
    for (i = 0; i < VD_MAXCLASS; i++) {
      cbk[i] = -1;
      for (j = 0; j < 8; j++) csb[i][j] = -1;
    }
    FATAL_UNLESS(t.get_u("flr.type", 2) == 1, "floor type 0");
    parts = t.get_u("flr.parts", 32);
    FATAL_UNLESS(parts <= VD_MAXPART, "floor partitions");
    for (i = 0; i < parts; i++) {
      pcls[i] = (u8) t.get_u("flr.pclass", VD_MAXCLASS);
      if ((u32) pcls[i] + 1 > nc) nc = pcls[i] + 1;
    }
    for (i = 0; i < nc; i++) {
      cdim[i] = (u8) (t.get_u("flr.cdim", 8) + 1);
      sub = t.get_u("flr.csub", 4);
      csub[i] = (u8) sub;
      cbk[i] = -1;
      if (sub) cbk[i] = (i32) t.get_u("flr.cbook", 256);
      for (j = 0; j < ((u32) 1 << sub); j++)
        csb[i][j] = (i32) t.get_u("flr.sbook", 257) - 1;
    }
    mult = t.get_u("flr.mult", 4) + 1;
    quant = Q[mult - 1];
    rng = t.get_u("flr.rangebits", 16);
    x[0] = 0;  x[1] = (u32) 1 << rng;  posts = 2;
    for (i = 0; i < parts; i++)
      for (j = 0; j < cdim[pcls[i]]; j++) {
        FATAL_UNLESS(posts < VD_MAXPOST, "too many floor posts");
        pcl[posts] = pcls[i];  ppos[posts] = (u8) j;
        x[posts++] = t.get_u("flr.x", (u64) 1 << rng);
      }
    /*  sort posts by X, then find each post's bracketing neighbours among
        the posts that precede it in list order  */
    for (i = 0; i < posts; i++) {
      srt[i] = (u8) i;
      for (j = i; j > 0 && x[srt[j - 1]] > x[srt[j]]; j--) {
        u8 tmp = srt[j];  srt[j] = srt[j - 1];  srt[j - 1] = tmp;
      }
    }
    /*  Two posts at the same X make x1 - x0 zero in render(), which divides
        by it.  balrogg rejects the same list at vb_floor.inc:37.  */
    for (i = 1; i < posts; i++)
      FATAL_UNLESS(x[srt[i]] != x[srt[i - 1]],
                   "floor X list repeats a value");
    for (i = 0; i < posts; i++) rnk[srt[i]] = (u8) i;
    for (i = 0; i + 2 < posts; i++) {
      u32 l = 0, h = 1, lx = 0, hx = x[1], cx = x[i + 2];
      for (k = 0; k < i + 2; k++) {
        if (x[k] > lx && x[k] < cx) { l = k;  lx = x[k]; }
        if (x[k] < hx && x[k] > cx) { h = k;  hx = x[k]; }
      }
      lo[i] = (u8) l;  hi[i] = (u8) h;
    }
  }
};

struct vd_res {
  u32 type, beg, end, psz, ncl, cbook;
  u8 casc[VD_MAXRCL];
  i32 book[VD_MAXRCL][8];

  void read(tsv & t) {
    u32 i, j;
    type = t.get_u("res.type", 4);
    FATAL_UNLESS(type <= 2, "residue type above 2");
    beg = t.get_u("res.begin", 0x1000000);
    end = t.get_u("res.end", 0x1000000);
    psz = t.get_u("res.partsize", 0x1000000) + 1;
    ncl = t.get_u("res.nclass", 64) + 1;
    FATAL_UNLESS(ncl <= VD_MAXRCL, "residue classifications");
    cbook = t.get_u("res.classbook", 256);
    for (i = 0; i < ncl; i++) casc[i] = (u8) t.get_u("res.cascade", 256);
    for (i = 0; i < ncl; i++)
      for (j = 0; j < 8; j++)
        book[i][j] = (casc[i] & ((u32) 1 << j))
                   ? (i32) t.get_u("res.book", 256) : -1;
  }
};

struct vd_map {
  u32 sub, nstep;
  u8 mag[VD_MAXCH], ang[VD_MAXCH];
  u8 mux[VD_MAXCH], fl[VD_MAXSUB], rs[VD_MAXSUB];

  void read(tsv & t, u32 ch) {
    u32 i;
    memset(mux, 0, sizeof mux);
    sub = t.get_u("map.submaps", 16) + 1;
    FATAL_UNLESS(sub <= VD_MAXSUB, "mapping submaps");
    nstep = t.get_u("map.steps", 257);
    FATAL_UNLESS(nstep <= VD_MAXCH, "coupling steps");
    for (i = 0; i < nstep; i++) {
      mag[i] = (u8) t.get_u("map.mag", 256);
      ang[i] = (u8) t.get_u("map.ang", 256);
    }
    if (sub > 1) for (i = 0; i < ch; i++) mux[i] = (u8) t.get_u("map.mux", 16);
    for (i = 0; i < sub; i++) {
      t.get("map.smtime");
      fl[i] = (u8) t.get_u("map.smfloor", 256);
      rs[i] = (u8) t.get_u("map.smres", 256);
    }
  }
};

struct vd_setup {
  u32 ch, rate, bs0, bs1;
  vd_book bk[VD_MAXBOOK];   u32 nbk;
  vd_floor fl[VD_MAXFLOOR]; u32 nfl;
  vd_res rs[VD_MAXRES];     u32 nrs;
  vd_map mp[VD_MAXMAP];     u32 nmp;
  u32 nmd;
  u8 blockflag[VD_MAXMODE], mdmap[VD_MAXMODE];
  int have;

  void ident(tsv & t) {
    u32 b;
    ch = t.get_u("id.channels", 256);
    rate = t.get_u("id.rate", (u64) 0x100000000);
    t.get("id.brmax");  t.get("id.brnom");  t.get("id.brmin");
    b = t.get_u("id.blocksize", 256);
    bs0 = (u32) 1 << (b & 15);  bs1 = (u32) 1 << (b >> 4);
    t.get("id.framing");
    FATAL_UNLESS(ch >= 1 && ch < VD_MAXCH, "%" PRIu64 " channels",
                 (u64) ch);
    FATAL_UNLESS(bs0 >= 64 && bs1 >= bs0 && bs1 <= MD_MAXBLK,
                 "block sizes %" PRIu64 " and %" PRIu64,
                 (u64) bs0, (u64) bs1);
  }

  /*  The comment header is opaque; its length comes from the page.  */
  void comment(tsv & t, sz len) {
    sz i;
    for (i = 0; i < len; i++) t.get("cmt.byte");
  }

  void setup(tsv & t) {
    u32 i, nt;
    nbk = t.get_u("cb.count", 256) + 1;
    FATAL_UNLESS(nbk <= VD_MAXBOOK, "codebooks");
    su_books(t, nbk, bk);
    nt = t.get_u("flr.ntime", 64) + 1;  (void) nt;
    nfl = t.get_u("flr.count", 64) + 1;
    FATAL_UNLESS(nfl <= VD_MAXFLOOR, "floors");
    for (i = 0; i < nfl; i++) fl[i].read(t);
    nrs = t.get_u("res.count", 64) + 1;
    FATAL_UNLESS(nrs <= VD_MAXRES, "residues");
    for (i = 0; i < nrs; i++) rs[i].read(t);
    /*  Every book a residue can reach must exist and carry a lookup grid: the
        decode path asks for that at vd_dec.inc:148, but the recovery divides
        by the book's step without asking (vd_rec::split, class_cost), so a
        stream naming a gridless or never-read book reached mode d as a
        division by zero.  The books and the residues are both known here, and
        this runs whichever role is walking the stream.  */
    for (i = 0; i < nrs; i++) {
      u32 c, p;
      FATAL_UNLESS(rs[i].cbook < nbk, "residue %" PRIu64 " names codebook %" PRIu64,
                   (u64) i, (u64) rs[i].cbook);
      FATAL_UNLESS(bk[rs[i].cbook].dim > 0,
                   "residue %" PRIu64 " class book has no grid",
                   (u64) i);
      for (c = 0; c < rs[i].ncl; c++)
        for (p = 0; p < 8; p++) {
          i32 bn = rs[i].book[c][p];
          if (bn < 0) continue;
          FATAL_UNLESS((u32) bn < nbk, "residue %" PRIu64 " names codebook %" PRId64,
                       (u64) i, (i64) bn);
          FATAL_UNLESS(bk[bn].dim > 0 && bk[bn].lookup == 1,
                       "residue book %" PRId64 " has no grid", (i64) bn);
        }
    }
    nmp = t.get_u("map.count", 64) + 1;
    FATAL_UNLESS(nmp <= VD_MAXMAP, "mappings");
    for (i = 0; i < nmp; i++) mp[i].read(t, ch);
    nmd = t.get_u("mode.count", 64) + 1;
    FATAL_UNLESS(nmd <= VD_MAXMODE, "modes");
    for (i = 0; i < nmd; i++) {
      blockflag[i] = (u8) t.get_u("mode.blockflag", 2);
      mdmap[i] = (u8) t.get_u("mode.mapping", 256);
      FATAL_UNLESS(mdmap[i] < nmp, "mode names mapping %d", mdmap[i]);
    }
    have = 1;
  }
};
