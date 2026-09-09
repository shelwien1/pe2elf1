/*  The codeword-length prior.

    A Vorbis residue digit is one base-`nv` place of a codebook entry, and the
    entry was written by the encoder as a Huffman codeword.  A complete
    Huffman code is a probability model: an entry of length L stands for
    2^-L.  balrogg reads those lengths -- it has to, to find where the next
    field starts -- and until now tsvcomp read them and threw them away.
    They are the one thing the Vorbis file knows about its own residue that
    no amount of coded history can supply, and at low bitrates, where a
    context has few visits to learn from, they are most of what there is.

    What this builds, per book, is the conditional form of that model.  The
    digits of a vector are the places of one entry, so once the first place is
    known the entry is confined to a subtree and the second place's
    distribution is that subtree's.  `pw` holds the weight of every prefix:
    level k has nsym^k nodes, node `idx` is the set of entries whose first k
    places spell `idx`, and its nsym children are what the next place may be.

    The cascade does not code a place, it codes a magnitude and then a sign,
    so what a node is asked for is not P(place) but P(the decision in front of
    it) -- the weight of the alphabet on each side of `is it zero`, `is it
    one`, `is its bit length k`.  Those sums are per prefix node and not per
    digit, so they are precomputed here and the coder does no arithmetic over
    the alphabet at all, except for the sign, which needs the magnitude the
    cascade has just settled and so cannot be known in advance.

    The prior does not decide anything on its own: it is one more input to the
    mixer, which learns how far to trust it, and one more axis the contexts
    may use.  Where a book is too large to tabulate, or its lengths did not
    survive reading, `ok` is 0 and every consumer falls back to what it did
    before.  */

constexpr int TCP_NBMAX  = 20;            /*  bit lengths the length cascade holds  */
constexpr int TCP_SYMMAX = 256;           /*  the widest alphabet a prior covers  */
constexpr u32 TCP_NODEMAX = 1u << 16;     /*  prefix nodes one book may take  */
constexpr u32 TCP_SCALE  = 1u << 20;      /*  what a node's weights sum to  */
constexpr u32 TCP_MEMMAX = 64u << 20;     /*  all priors together  */
/*  The mantissa prior is the one that cannot be tabulated, so it walks the
    alphabet at coding time.  That is cheap for a residue book, whose grid
    holds at most a few dozen values, and not for a floor post, whose
    alphabet is its whole range -- where it also returns nothing, since what
    is left of a floor residual after its bit length is close to uniform.  */
constexpr u32 TCP_MANMAX = 64;

static sz tcp_mem;

/*  b^e, stopped well short of where it would wrap.  Used only to bound how
    many entries a given number of places can spell, so a saturated answer
    means "more than any book has" and bounds nothing.  */
static INLINE u64 tcp_pow(u32 b, u32 e) {
  u64 v = 1;
  u32 i;
  for (i = 0; i < e; i++) {
    if (v > ((u64) 1 << 40) / (b ? b : 1)) return (u64) 1 << 40;
    v *= b;
  }
  return v;
}

/*  `nlev` places of an `nsym`-ary alphabet.  A residue book spells its entry
    least significant place first (io.inc's rs_sym divides by nv as it goes);
    a classbook spells it most significant first (the classword loop
    multiplies by ncl), which is `be`.  */
struct tc_ptab {
  int ok, be;
  /*  `nlev` is how many places are tabulated and `ndim` how many the vector
      has.  They part company when the trie would not fit: a dense trie is
      nbr^k nodes at level k, so a book of any width runs out of room after a
      few places, and refusing the whole prior there loses the first places
      too -- which are the ones every entry passes through and the ones a
      vector always reaches.  Tabulating what fits and stopping the cursor
      where the table stops costs nothing and keeps the rest.  */
  u32 nsym, nlev, ndim, nnode;
  /*  A floor post's alphabet is not its trie's branching: the value says
      which subclass book coded it, and it is the subclass that the classword
      spells.  So the symbol alphabet (`nsym` values) and the branching
      (`nbr`) are separate, and `brm` maps one to the other.  For a residue
      digit and a class the two coincide and `brm` is the identity.  */
  u32 nbr;
  u32 of[17], sz_[17];                    /*  node offset and count per level  */
  u32 * pw;                               /*  [nnode][nsym] child weight  */
  u32 * tot;                              /*  [nnode] and their sum  */
  u32 * s012;                             /*  [nnode][3] weight of |v| = 0, 1, 2  */
  u32 * slen;                             /*  [nnode][TCP_NBMAX] by bit length  */
  i32 * val;                              /*  what each symbol stands for  */
  u32 * brm;                              /*  and which branch it takes  */

  void drop() {
    free(pw);  free(tot);  free(s012);  free(slen);  free(val);  free(brm);
    pw = nullptr;  tot = nullptr;  s012 = nullptr;  slen = nullptr;
    val = nullptr;  brm = nullptr;  ok = 0;
  }

  /*  Where the cascade puts a magnitude of 3 or more: `code` sends
      u = |v| - 2 and then u's bit length in unary.  */
  static INLINE int nbits(i32 a) {
    u32 u = (u32) a - 2;
    int n = 0;
    while (u) { n++;  u >>= 1; }
    return n;
  }

  /*  Level offsets and the tables, for a trie of `nlev` places branching
      `nbr` ways with an alphabet of `nsym`.  Both counts are bounded before
      they are believed: nothing in a stream may ask for a table the machine
      does not have.  */
  u64 * layout(u32 nsym_, u32 nbr_, u32 nlev_, int be_) {
    u64 * acc;
    u64 lim = 1;
    u32 k;
    drop();
    if (!nsym_ || !nbr_ || !nlev_ || nsym_ > TCP_SYMMAX || nlev_ > 16) return nullptr;
    nsym = nsym_;  nbr = nbr_;  ndim = nlev_;  be = be_;
    nnode = 0;
    /*  As many levels as fit, rather than all of them or none.  */
    for (k = 0; k < ndim; k++) {
      if (lim > TCP_NODEMAX || nnode + lim > TCP_NODEMAX) break;
      if (tcp_mem + (sz) (nnode + lim) * (nsym * 2 + 4 + TCP_NBMAX) * 4 > TCP_MEMMAX) break;
      sz_[k] = (u32) lim;
      of[k] = nnode;
      nnode += (u32) lim;
      lim *= nbr;
      if (lim > (u64) 1 << 32) { k++;  break; }
    }
    nlev = k;
    if (!nlev) return nullptr;
    of[nlev] = nnode;
    acc  = (u64 *) calloc((sz) nnode * nsym, sizeof(u64));
    pw   = (u32 *) calloc((sz) nnode * nsym, sizeof(u32));
    tot  = (u32 *) calloc(nnode, sizeof(u32));
    s012 = (u32 *) calloc((sz) nnode * 3, sizeof(u32));
    slen = (u32 *) calloc((sz) nnode * TCP_NBMAX, sizeof(u32));
    val  = (i32 *) calloc(nsym, sizeof(i32));
    brm  = (u32 *) calloc(nsym, sizeof(u32));
    if (!acc || !pw || !tot || !s012 || !slen || !val || !brm)
      { free(acc);  drop();  return nullptr; }
    tcp_mem += (sz) nnode * (nsym * 2 + 4 + TCP_NBMAX) * 4;
    return acc;
  }

  /*  Normalise each node to TCP_SCALE and take the sums the cascade asks
      for.  A child that any entry can reach keeps at least one unit: a
      symbol the book does admit must not come back as impossible, however
      rare it is.  */
  void finish(u64 * acc) {
    u32 i;
    for (i = 0; i < nnode; i++) {
      u64 * a = acc + (sz) i * nsym;
      u32 * p = pw + (sz) i * nsym;
      u64 T = 0;
      u32 j, sum = 0;
      for (j = 0; j < nsym; j++) T += a[j];
      if (!T) continue;
      for (j = 0; j < nsym; j++) {
        u32 v = (u32) ((a[j] * TCP_SCALE + T / 2) / T);
        if (!v && a[j]) v = 1;
        p[j] = v;  sum += v;
      }
      tot[i] = sum;
      for (j = 0; j < nsym; j++) {
        i32 a2 = val[j] < 0 ? -val[j] : val[j];
        if (a2 < 3) s012[(sz) i * 3 + a2] += p[j];
        else {
          int nb = nbits(a2);
          if (nb > 0 && nb < TCP_NBMAX) slen[(sz) i * TCP_NBMAX + nb] += p[j];
        }
      }
    }
    free(acc);
    ok = 1;
  }

  /*  A residue vector or a classword: the symbols are the places of one
      entry and the branching is the alphabet.  */
  int build(const u8 * len, u32 ent, u32 nsym_, u32 nlev_, int be_,
            const i32 * value) {
    u64 * acc;
    u32 k, i, e, cap;
    u64 lim;
    if (!len) return 0;
    acc = layout(nsym_, nsym_, nlev_, be_);
    if (!acc) return 0;
    for (i = 0; i < nsym; i++) { val[i] = value[i];  brm[i] = i; }
    /*  What the vector's places can spell -- all of them, not just the ones
        that fit in the table, since an entry past the tabulated depth still
        passes through the prefixes that are tabulated.  */
    lim = tcp_pow(nsym, ndim);
    cap = (u32) (lim < ent ? lim : ent);
    /*  Every used entry drops its weight into the node of each of its
        prefixes.  2^-len is scaled by 2^32 so it stays an integer; the
        lengths Vorbis admits are 1..32.  */
    for (e = 0; e < cap; e++) {
      u32 L = len[e], nd, s;
      u64 w;
      if (!L || L > 32) continue;
      w = (u64) 1 << (32 - L);
      for (k = 0; k < nlev; k++) {
        /*  Big-endian spells the most significant place first, so the prefix
            of length k is what is left after dividing out the places below
            it -- of which there are ndim - k, whether or not they are all
            tabulated.  */
        if (be) { u64 d = tcp_pow(nbr, ndim - 1 - k);
                  nd = (u32) (e / d / nsym);  s = (u32) ((e / d) % nsym); }
        else    { nd = e % sz_[k];  s = (e / sz_[k]) % nsym; }
        acc[(sz) (of[k] + nd) * nsym + s] += w;
      }
    }
    finish(acc);
    return 1;
  }

  /*  A floor partition.  The classword spells which subclass book codes each
      of the partition's `cd` posts, and the post's own value then comes from
      that book -- so P(post) is P(subclass) times P(value in its book), and
      which subclass a value implies is fixed: libvorbis picks the first book
      whose entry count covers it, which is what `sel` holds.  Both factors
      are static, so the whole value distribution is tabulated per prefix and
      the coder does no work at all.  */
  int build_floor(const u8 * clen, u32 cent, u32 cs, u32 cd,
                  const u8 * const * blen, const u32 * bent, u32 maxy) {
    u64 * acc;
    u32 nbr_ = 1u << cs, k, i, e, y, cap;
    u64 lim;
    u64 * bw;                               /*  per subclass, per value  */
    if (!maxy || maxy > TCP_SYMMAX || cs > 3) return 0;
    acc = layout(maxy, nbr_, cd, 0);
    if (!acc) return 0;
    bw = (u64 *) calloc((sz) nbr_ * maxy, sizeof(u64));
    if (!bw) { free(acc);  drop();  return 0; }
    for (y = 0; y < maxy; y++) {
      u32 sel = nbr_;
      for (i = 0; i < nbr_; i++) {
        u32 mx = blen[i] ? bent[i] : 1;
        if (y < mx) { sel = i;  break; }
      }
      val[y] = (i32) y;
      brm[y] = sel < nbr_ ? sel : 0;
      if (sel >= nbr_) continue;            /*  no book can hold it  */
      if (!blen[sel]) { if (!y) bw[(sz) sel * maxy] = TCP_SCALE; }
      else if (blen[sel][y] && blen[sel][y] <= 32)
        bw[(sz) sel * maxy + y] = (u64) 1 << (32 - blen[sel][y]);
    }
    /*  Scale each book to TCP_SCALE so the product with a class weight,
        itself under 2^32, stays inside 64 bits.  */
    for (i = 0; i < nbr_; i++) {
      u64 T = 0;
      for (y = 0; y < maxy; y++) T += bw[(sz) i * maxy + y];
      if (!T) continue;
      for (y = 0; y < maxy; y++) {
        u64 * q = bw + (sz) i * maxy + y;
        u64 v = (*q * TCP_SCALE + T / 2) / T;
        if (!v && *q) v = 1;
        *q = v;
      }
    }
    /*  The classword's own trie, or a single branch when there is none.  */
    { u64 * cwt = (u64 *) calloc((sz) nnode * nbr_, sizeof(u64));
      if (!cwt) { free(bw);  free(acc);  drop();  return 0; }
      if (!clen || !cs) {
        for (i = 0; i < nnode; i++) cwt[(sz) i * nbr_] = TCP_SCALE;
      } else {
        lim = tcp_pow(nbr_, cd);
        cap = (u32) (lim < cent ? lim : cent);
        for (e = 0; e < cap; e++) {
          u32 L = clen[e], nd, sb;
          u64 w;
          if (!L || L > 32) continue;
          w = (u64) 1 << (32 - L);
          for (k = 0; k < nlev; k++) {
            nd = e % sz_[k];  sb = (e / sz_[k]) % nbr_;
            cwt[(sz) (of[k] + nd) * nbr_ + sb] += w;
          }
        }
        for (i = 0; i < nnode; i++) {       /*  down to TCP_SCALE, as above  */
          u64 * a = cwt + (sz) i * nbr_, T = 0;
          for (k = 0; k < nbr_; k++) T += a[k];
          if (!T) continue;
          for (k = 0; k < nbr_; k++) {
            u64 v = (a[k] * TCP_SCALE + T / 2) / T;
            if (!v && a[k]) v = 1;
            a[k] = v;
          }
        }
      }
      for (i = 0; i < nnode; i++)
        for (y = 0; y < maxy; y++)
          acc[(sz) i * maxy + y] = cwt[(sz) i * nbr_ + brm[y]] * bw[(sz) brm[y] * maxy + y];
      free(cwt);
    }
    free(bw);
    finish(acc);
    return 1;
  }
};

/*  One family's prior while it codes one value: which table, which prefix
    node, and how far into the vector it is.  `on` goes to 0 the moment
    anything does not line up -- an unusable book, a digit the grid does not
    hold, a node no entry reaches -- and the coder then behaves exactly as it
    did before there was a prior.  */
struct tc_pcur {
  const tc_ptab * t;
  u32 idx;                                /*  node within its level  */
  u32 lev;
  int on;

  INLINE void off() { t = nullptr;  on = 0; }
  INLINE void start(const tc_ptab * tab) {
    t = tab && tab->ok ? tab : nullptr;  idx = 0;  lev = 0;  on = t != nullptr;
  }
  /*  Ready for the value at place `lev`; `on` says whether there is one.  */
  INLINE u32 node() const { return t->of[lev] + idx; }
  INLINE int usable() const { return on && t && lev < t->nlev && t->tot[node()]; }

  /*  Advance past a place whose symbol was `s`.  */
  INLINE void step(int s) {
    u32 b;
    if (!on || !t) return;
    if (s < 0 || (u32) s >= t->nsym || lev + 1 > t->nlev) { on = 0;  return; }
    b = t->brm[s];
    if (t->be) idx = idx * t->nbr + b;
    else       idx = idx + b * t->sz_[lev];
    lev++;
    if (lev >= t->nlev) on = 0;             /*  the vector ends here  */
  }
};

/*  A weight pair as a 12-bit probability that the bit is zero.  */
static INLINE int tcp_p(u32 w0, u32 w1) {
  u64 s = (u64) w0 + w1;
  int p;
  if (!s) return -1;
  p = (int) (((u64) w0 * CM_PONE + s / 2) / s);
  return p < 1 ? 1 : p > CM_PONE - 1 ? CM_PONE - 1 : p;
}

/*  The four shapes of decision the cascade makes, each as the weight on
    either side of it.  `m` is the step: node 0 asks "is it zero", node 1 "is
    it one", node 2 "is it two".  */
static INLINE int tcp_head(const tc_pcur & c, int m) {
  const u32 * s;
  u32 nd;
  u32 w1, w0;
  int j;
  if (!c.usable() || m < 0 || m > 2) return -1;
  nd = c.node();
  s = c.t->s012 + (sz) nd * 3;
  w1 = s[m];
  w0 = c.t->tot[nd];
  for (j = 0; j <= m; j++) w0 -= s[j];      /*  what is left above m  */
  return tcp_p(w0, w1);
}
/*  The unary length step: is the bit length exactly k + 1, given it is at
    least that.  */
static INLINE int tcp_len(const tc_pcur & c, int k) {
  const u32 * s;
  u32 nd, w1 = 0, w0 = 0;
  int j;
  if (!c.usable() || k + 1 >= TCP_NBMAX) return -1;
  nd = c.node();
  s = c.t->slen + (sz) nd * TCP_NBMAX;
  w1 = s[k + 1];
  for (j = k + 2; j < TCP_NBMAX; j++) w0 += s[j];
  return tcp_p(w0, w1);
}
/*  The sign, once the magnitude is settled.  The alphabet is small and this
    is asked once per nonzero digit, so it is a loop rather than a table.  */
static INLINE int tcp_sign(const tc_pcur & c, i64 mag) {
  const u32 * p;
  u32 nd, wn = 0, wp = 0;
  u32 j;
  if (!c.usable() || mag <= 0) return -1;
  nd = c.node();
  p = c.t->pw + (sz) nd * c.t->nsym;
  for (j = 0; j < c.t->nsym; j++) {
    i32 v = c.t->val[j];
    if (v == (i32) mag) wp += p[j];
    else if (v == -(i32) mag) wn += p[j];
  }
  return tcp_p(wp, wn);
}
/*  A mantissa bit.  The length cascade has settled how many bits the value
    has and `pref` is what has been coded of it, leading one included, so the
    alphabet is down to the few symbols that spell the rest.  This is the one
    prior that has to be walked at coding time -- the prefix is not a prefix
    of the codebook entry, so it cannot be tabulated per node.  */
static INLINE int tcp_man(const tc_pcur & c, int nb, int p, u64 pref) {
  const u32 * w;
  u32 nd, w0 = 0, w1 = 0, j;
  if (!c.usable() || nb < 2 || p > nb - 2 || c.t->nsym > TCP_MANMAX) return -1;
  nd = c.node();
  w = c.t->pw + (sz) nd * c.t->nsym;
  for (j = 0; j < c.t->nsym; j++) {
    i32 v = c.t->val[j];
    i32 a = v < 0 ? -v : v;
    u32 u;
    if (a < 3) continue;
    if (tc_ptab::nbits(a) != nb) continue;
    u = (u32) a - 2;
    if ((u >> (nb - 1 - p)) != pref) continue;
    if ((u >> (nb - 2 - p)) & 1) w1 += w[j];  else w0 += w[j];
  }
  return tcp_p(w0, w1);
}

/*  A probability as a context axis: its log-odds in 32 steps, with 0 kept
    for "there was no prior", which is not the same as an even one.  */
static INLINE int tcp_bucket(int p) {
  if (p < 0) return 0;
  return tc_clamp((cm_stretch(p) + CM_STMAX) >> 7, 0, 31) + 1;
}
/*  The prior for the digit as a whole: the log-odds that it is zero.  */
static INLINE int tcp_axis(const tc_pcur & c) {
  return tcp_bucket(tcp_head(c, 0));
}
