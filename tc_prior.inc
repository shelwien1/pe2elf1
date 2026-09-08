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
constexpr int TCP_SYMMAX = 64;            /*  the widest alphabet a prior covers  */
constexpr u32 TCP_NODEMAX = 1u << 16;     /*  prefix nodes one book may take  */
constexpr u32 TCP_SCALE  = 1u << 20;      /*  what a node's weights sum to  */
constexpr u32 TCP_MEMMAX = 64u << 20;     /*  all priors together  */

static sz tcp_mem;

/*  `nlev` places of an `nsym`-ary alphabet.  A residue book spells its entry
    least significant place first (io.inc's rs_sym divides by nv as it goes);
    a classbook spells it most significant first (the classword loop
    multiplies by ncl), which is `be`.  */
struct tc_ptab {
  int ok, be;
  u32 nsym, nlev, nnode;
  u32 of[17], sz_[17];                    /*  node offset and count per level  */
  u32 * pw;                               /*  [nnode][nsym] child weight  */
  u32 * tot;                              /*  [nnode] and their sum  */
  u32 * s012;                             /*  [nnode][3] weight of |v| = 0, 1, 2  */
  u32 * slen;                             /*  [nnode][TCP_NBMAX] by bit length  */
  i32 val[TCP_SYMMAX];                    /*  what each place stands for  */

  void drop() {
    free(pw);  free(tot);  free(s012);  free(slen);
    pw = nullptr;  tot = nullptr;  s012 = nullptr;  slen = nullptr;  ok = 0;
  }

  /*  Where the cascade puts a magnitude of 3 or more: `code` sends
      u = |v| - 2 and then u's bit length in unary.  */
  static INLINE int nbits(i32 a) {
    u32 u = (u32) a - 2;
    int n = 0;
    while (u) { n++;  u >>= 1; }
    return n;
  }

  int build(const u8 * len, u32 ent, u32 nsym_, u32 nlev_, int be_,
            const i32 * value) {
    u64 * acc;
    u64 lim = 1;
    u32 k, i, e, cap;
    drop();
    if (!len || !nsym_ || !nlev_ || nsym_ > TCP_SYMMAX || nlev_ > 16) return 0;
    nsym = nsym_;  nlev = nlev_;  be = be_;
    /*  nsym^nlev, and the node count under it, both bounded before they are
        believed: a pattern in the stream must not be able to ask for a table
        the machine does not have.  */
    nnode = 0;
    for (k = 0; k < nlev; k++) {
      sz_[k] = (u32) lim;
      of[k] = nnode;
      if (lim > TCP_NODEMAX || nnode + lim > TCP_NODEMAX) return 0;
      nnode += (u32) lim;
      lim *= nsym;
      if (lim > (u64) 1 << 32) return 0;
    }
    of[nlev] = nnode;
    cap = (u32) (lim < ent ? lim : ent);   /*  entries the places can spell  */
    if ((sz) nnode * nsym * 4 > TCP_MEMMAX ||
        tcp_mem + (sz) nnode * (nsym + 4 + TCP_NBMAX) * 4 > TCP_MEMMAX) return 0;
    for (i = 0; i < nsym; i++) val[i] = value[i];

    acc = (u64 *) calloc((sz) nnode * nsym, sizeof(u64));
    pw   = (u32 *) calloc((sz) nnode * nsym, sizeof(u32));
    tot  = (u32 *) calloc(nnode, sizeof(u32));
    s012 = (u32 *) calloc((sz) nnode * 3, sizeof(u32));
    slen = (u32 *) calloc((sz) nnode * TCP_NBMAX, sizeof(u32));
    if (!acc || !pw || !tot || !s012 || !slen) { free(acc);  drop();  return 0; }
    tcp_mem += (sz) nnode * (nsym + 4 + TCP_NBMAX) * 4;

    /*  Every used entry drops its weight into the node of each of its
        prefixes.  2^-len is scaled by 2^32 so it stays an integer; the
        lengths Vorbis admits are 1..32.  */
    for (e = 0; e < cap; e++) {
      u32 L = len[e], nd, s, q;
      u64 w;
      if (!L || L > 32) continue;
      w = (u64) 1 << (32 - L);
      for (k = 0; k < nlev; k++) {
        if (be) { nd = e / sz_[nlev - 1 - k] / nsym;  s = (e / sz_[nlev - 1 - k]) % nsym; }
        else    { nd = e % sz_[k];  q = e / sz_[k];  s = q % nsym; }
        acc[(sz) (of[k] + nd) * nsym + s] += w;
      }
    }

    /*  Normalise each node to TCP_SCALE.  A child that any entry can reach
        keeps at least one unit: an alphabet symbol the book does admit must
        not come back as impossible, however rare it is.  */
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
    if (!on || !t) return;
    if (s < 0 || (u32) s >= t->nsym || lev + 1 > t->nlev) { on = 0;  return; }
    if (t->be) idx = idx * t->nsym + (u32) s;
    else       idx = idx + (u32) s * t->sz_[lev];
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
/*  A summary of the prior for the digit as a whole, for use as a context
    axis: the log-odds that it is zero, in 32 steps.  */
static INLINE int tcp_axis(const tc_pcur & c) {
  int p = tcp_head(c, 0);
  if (p < 0) return 0;
  return tc_clamp((cm_stretch(p) + CM_STMAX) >> 7, 0, 31) + 1;
}
