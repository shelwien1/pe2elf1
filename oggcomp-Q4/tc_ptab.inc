constexpr int TCP_NBMAX = 20;
constexpr int TCP_SYMMAX = 256;
constexpr u32 TCP_NODEMAX = 1u << 16;
constexpr u32 TCP_SCALE = 1u << 20;
constexpr u32 TCP_MEMMAX = 64u << 20;
constexpr u32 TCP_MANMAX = 64;
//  Where the tables come from.  layout() budgets them: it charges `charged`
//  by a per-node formula and stops adding levels when the charge would pass
//  TCP_MEMMAX, and that decision shapes the tables and so the stream, so the
//  formula is untouched and `charged` is still what it counts.  The bytes
//  themselves are a bump pointer over a static block: the formula charges
//  at least as much as a table takes for every table with two nodes or
//  more, a one-node table can run a kilobyte past it, and there are at most
//  1344 tables, which is what the 4 MB over the budget is for.  The three
//  scratch buffers are the builders' temporaries -- acc is bounded by the
//  same budget, since the formula charges 8 bytes per node and symbol and
//  acc holds one u64 per node and symbol; bw and cwt by the floor's eight
//  subclasses and TCP_NODEMAX.  All of it is given back whole at reset().
constexpr sz TCP_ARENA = (sz)TCP_MEMMAX + ((sz)4 << 20);
struct tcp_arena {
  sz charged;
  sz used;
  alignas(8) u8 mem[TCP_ARENA];
  u64 acc[TCP_MEMMAX / 8];
  u64 bw[8 * TCP_SYMMAX];
  u64 cwt[(sz)TCP_NODEMAX * 8];
  void *take(sz n) {
    sz want = (n + 7) & ~(sz)7;
    void *p = mem + used;
    FATAL_UNLESS(want <= TCP_ARENA - used, "internal: the codebook tables outgrew their budget");
    used += want;
    memset(p, 0, want);
    return p;
  }
  void reset() {
    charged = 0;
    used = 0;
  }
};
static INLINE u64 tcp_pow(u32 b, u32 e) {
  u64 v = 1;
  u32 i;
  for(i = 0; i < e; i++) {
    if(v > ((u64)1 << 40) / (b ? b : 1))
      return (u64)1 << 40;
    v *= b;
  }
  return v;
}
static INLINE int tcp_p(u32 w0, u32 w1) {
  u64 s = (u64)w0 + w1;
  int p;
  if(!s)
    return -1;
  p = (int)(((u64)w0 * CM_PONE + s / 2) / s);
  return p < 1 ? 1 : p > CM_PONE - 1 ? CM_PONE - 1 : p;
}
struct tc_ptab {
  int ok, be;
  u32 nsym, nlev, ndim, nnode;
  u32 nbr;
  u32 of[17], sz_[17];
  u32 *pw;
  u32 *tot;
  u32 *s012;
  u32 *slen;
  i32 *val;
  u32 *brm;
  i16 *ph;
  i16 *pl;
  i16 *psg;
  u32 nmag;
  void drop() {
    pw = nullptr;
    tot = nullptr;
    s012 = nullptr;
    slen = nullptr;
    val = nullptr;
    brm = nullptr;
    ph = nullptr;
    pl = nullptr;
    psg = nullptr;
    nmag = 0;
    ok = 0;
  }
  static INLINE int nbits(i32 a) {
    u32 u = (u32)a - 2;
    int n = 0;
    while(u) {
      n++;
      u >>= 1;
    }
    return n;
  }
  u64 *layout(tcp_arena &ar, u32 nsym_, u32 nbr_, u32 nlev_, int be_) {
    u64 *acc;
    u64 lim = 1;
    u32 k;
    drop();
    if(!nsym_ || !nbr_ || !nlev_ || nsym_ > TCP_SYMMAX || nlev_ > 16)
      return nullptr;
    nsym = nsym_;
    nbr = nbr_;
    ndim = nlev_;
    be = be_;
    nnode = 0;
    for(k = 0; k < ndim; k++) {
      if(lim > TCP_NODEMAX || nnode + lim > TCP_NODEMAX)
        break;
      if(ar.charged + (sz)(nnode + lim) * ((nsym * 2 + 4 + TCP_NBMAX) * 4 + (3 + TCP_NBMAX) * 2) > TCP_MEMMAX)
        break;
      sz_[k] = (u32)lim;
      of[k] = nnode;
      nnode += (u32)lim;
      lim *= nbr;
      if(lim > (u64)1 << 32) {
        k++;
        break;
      }
    }
    nlev = k;
    if(!nlev)
      return nullptr;
    of[nlev] = nnode;
    acc = ar.acc;
    memset(acc, 0, (sz)nnode * nsym * sizeof(u64));
    pw = (u32 *)ar.take((sz)nnode * nsym * sizeof(u32));
    tot = (u32 *)ar.take(nnode * sizeof(u32));
    s012 = (u32 *)ar.take((sz)nnode * 3 * sizeof(u32));
    slen = (u32 *)ar.take((sz)nnode * TCP_NBMAX * sizeof(u32));
    val = (i32 *)ar.take(nsym * sizeof(i32));
    brm = (u32 *)ar.take(nsym * sizeof(u32));
    ph = (i16 *)ar.take((sz)nnode * 3 * sizeof(i16));
    pl = (i16 *)ar.take((sz)nnode * TCP_NBMAX * sizeof(i16));
    ar.charged += (sz)nnode * ((nsym * 2 + 4 + TCP_NBMAX) * 4 + (3 + TCP_NBMAX) * 2);
    return acc;
  }
  void finish(tcp_arena &ar, u64 *acc, int want_sign) {
    u32 i;
    for(i = 0; i < nnode; i++) {
      u64 *a = acc + (sz)i * nsym;
      u32 *p = pw + (sz)i * nsym;
      u64 T = 0;
      u32 j, sum = 0;
      for(j = 0; j < nsym; j++)
        T += a[j];
      if(!T)
        continue;
      for(j = 0; j < nsym; j++) {
        u32 v = (u32)((a[j] * TCP_SCALE + T / 2) / T);
        if(!v && a[j])
          v = 1;
        p[j] = v;
        sum += v;
      }
      tot[i] = sum;
      for(j = 0; j < nsym; j++) {
        i32 a2 = val[j] < 0 ? -val[j] : val[j];
        if(a2 < 3)
          s012[(sz)i * 3 + a2] += p[j];
        else {
          int nb = nbits(a2);
          if(nb > 0 && nb < TCP_NBMAX)
            slen[(sz)i * TCP_NBMAX + nb] += p[j];
        }
      }
    }
    for(i = 0; i < nnode; i++) {
      const u32 *s = s012 + (sz)i * 3;
      const u32 *sl = slen + (sz)i * TCP_NBMAX;
      u32 w0 = tot[i], j;
      int m, k;
      for(m = 0; m < 3; m++) {
        w0 -= s[m];
        ph[(sz)i * 3 + m] = (i16)tcp_p(w0, s[m]);
      }
      for(k = 0; k + 1 < TCP_NBMAX; k++) {
        u32 w = 0;
        for(j = (u32)k + 2; j < TCP_NBMAX; j++)
          w += sl[j];
        pl[(sz)i * TCP_NBMAX + k] = (i16)tcp_p(w, sl[k + 1]);
      }
      pl[(sz)i * TCP_NBMAX + TCP_NBMAX - 1] = -1;
    }
    nmag = 0;
    if(want_sign && nsym <= TCP_MANMAX) {
      i32 mx = 0, m;
      u32 j;
      for(j = 0; j < nsym; j++) {
        i32 a = val[j] < 0 ? -val[j] : val[j];
        if(a > mx)
          mx = a;
      }
      if(mx < 256 && ar.charged + (sz)nnode * ((sz)mx + 1) * 2 <= TCP_MEMMAX)
        psg = (i16 *)ar.take((sz)nnode * ((sz)mx + 1) * sizeof(i16));
      if(psg) {
        nmag = (u32)mx + 1;
        ar.charged += (sz)nnode * nmag * 2;
        for(i = 0; i < nnode; i++) {
          const u32 *p = pw + (sz)i * nsym;
          psg[(sz)i * nmag] = -1;
          for(m = 1; m <= mx; m++) {
            u32 wp = 0, wn = 0;
            for(j = 0; j < nsym; j++) {
              if(val[j] == m)
                wp += p[j];
              else if(val[j] == -m)
                wn += p[j];
            }
            psg[(sz)i * nmag + (u32)m] = (i16)tcp_p(wp, wn);
          }
        }
      }
    }
    ok = 1;
  }
  int build(tcp_arena &ar, const u8 *len, u32 ent, u32 nsym_, u32 nlev_, int be_, const i32 *value, int want_sign) {
    u64 *acc;
    u32 k, i, e, cap;
    u64 lim;
    if(!len)
      return 0;
    acc = layout(ar, nsym_, nsym_, nlev_, be_);
    if(!acc)
      return 0;
    for(i = 0; i < nsym; i++) {
      val[i] = value[i];
      brm[i] = i;
    }
    lim = tcp_pow(nsym, ndim);
    cap = (u32)(lim < ent ? lim : ent);
    for(e = 0; e < cap; e++) {
      u32 L = len[e], nd, s;
      u64 w;
      if(!L || L > 32)
        continue;
      w = (u64)1 << (32 - L);
      for(k = 0; k < nlev; k++) {
        if(be) {
          u64 d = tcp_pow(nbr, ndim - 1 - k);
          nd = (u32)(e / d / nsym);
          s = (u32)((e / d) % nsym);
        } else {
          nd = e % sz_[k];
          s = (e / sz_[k]) % nsym;
        }
        acc[(sz)(of[k] + nd) * nsym + s] += w;
      }
    }
    finish(ar, acc, want_sign);
    return 1;
  }
  int build_floor(tcp_arena &ar, const u8 *clen, u32 cent, u32 cs, u32 cd, const u8 *const *blen, const u32 *bent, u32 maxy) {
    u64 *acc;
    u32 nbr_ = 1u << cs, k, i, e, y, cap;
    u64 lim;
    u64 *bw;
    if(!maxy || maxy > TCP_SYMMAX || cs > 3)
      return 0;
    acc = layout(ar, maxy, nbr_, cd, 0);
    if(!acc)
      return 0;
    bw = ar.bw;
    memset(bw, 0, (sz)nbr_ * maxy * sizeof(u64));
    for(y = 0; y < maxy; y++) {
      u32 sel = nbr_;
      for(i = 0; i < nbr_; i++) {
        u32 mx = blen[i] ? bent[i] : 1;
        if(y < mx) {
          sel = i;
          break;
        }
      }
      val[y] = (i32)y;
      brm[y] = sel < nbr_ ? sel : 0;
      if(sel >= nbr_)
        continue;
      if(!blen[sel]) {
        if(!y)
          bw[(sz)sel * maxy] = TCP_SCALE;
      } else if(blen[sel][y] && blen[sel][y] <= 32)
        bw[(sz)sel * maxy + y] = (u64)1 << (32 - blen[sel][y]);
    }
    for(i = 0; i < nbr_; i++) {
      u64 T = 0;
      for(y = 0; y < maxy; y++)
        T += bw[(sz)i * maxy + y];
      if(!T)
        continue;
      for(y = 0; y < maxy; y++) {
        u64 *q = bw + (sz)i * maxy + y;
        u64 v = (*q * TCP_SCALE + T / 2) / T;
        if(!v && *q)
          v = 1;
        *q = v;
      }
    }
    {
      u64 *cwt = ar.cwt;
      memset(cwt, 0, (sz)nnode * nbr_ * sizeof(u64));
      if(!clen || !cs) {
        for(i = 0; i < nnode; i++)
          cwt[(sz)i * nbr_] = TCP_SCALE;
      } else {
        lim = tcp_pow(nbr_, cd);
        cap = (u32)(lim < cent ? lim : cent);
        for(e = 0; e < cap; e++) {
          u32 L = clen[e], nd, sb;
          u64 w;
          if(!L || L > 32)
            continue;
          w = (u64)1 << (32 - L);
          for(k = 0; k < nlev; k++) {
            nd = e % sz_[k];
            sb = (e / sz_[k]) % nbr_;
            cwt[(sz)(of[k] + nd) * nbr_ + sb] += w;
          }
        }
        for(i = 0; i < nnode; i++) {
          u64 *a = cwt + (sz)i * nbr_, T = 0;
          for(k = 0; k < nbr_; k++)
            T += a[k];
          if(!T)
            continue;
          for(k = 0; k < nbr_; k++) {
            u64 v = (a[k] * TCP_SCALE + T / 2) / T;
            if(!v && a[k])
              v = 1;
            a[k] = v;
          }
        }
      }
      for(i = 0; i < nnode; i++)
        for(y = 0; y < maxy; y++)
          acc[(sz)i * maxy + y] = cwt[(sz)i * nbr_ + brm[y]] * bw[(sz)brm[y] * maxy + y];
    }
    finish(ar, acc, 0);
    return 1;
  }
};
static INLINE int tcp_bucket(int p) {
  if(p < 0)
    return 0;
  return tc_clamp((cm_stretch(p) + CM_STMAX) >> 7, 0, 31) + 1;
}
struct tc_pcur {
  const tc_ptab *t;
  u32 idx;
  u32 lev;
  int on;
  INLINE void off() {
    t = nullptr;
    on = 0;
  }
  INLINE void start(const tc_ptab *tab) {
    t = tab && tab->ok ? tab : nullptr;
    idx = 0;
    lev = 0;
    on = t != nullptr;
  }
  INLINE u32 node() const { return t->of[lev] + idx; }
  INLINE int live() const { return on; }
  INLINE int usable() const { return live() && t->tot[node()]; }
  INLINE void step(int s) {
    u32 b, big, little;
    if(!on || !t)
      return;
    if(s < 0 || (u32)s >= t->nsym || lev + 1 > t->nlev) {
      on = 0;
      return;
    }
    b = t->brm[s];
    big = idx * t->nbr + b;
    little = idx + b * t->sz_[lev];
    idx = t->be ? big : little;
    lev++;
    on = lev < t->nlev;
  }

  INLINE int head(int m) const {
    if(!live() || m < 0 || m > 2)
      return -1;
    return t->ph[(sz)node() * 3 + m];
  }
  INLINE int len(int k) const {
    if(!live() || k + 1 >= TCP_NBMAX)
      return -1;
    return t->pl[(sz)node() * TCP_NBMAX + k];
  }
  INLINE int sign(i64 mag) const {
    const u32 *p;
    u32 nd, wn = 0, wp = 0;
    u32 j;
    if(!live() || mag <= 0)
      return -1;
    nd = node();
    if(t->psg)
      return mag < (i64)t->nmag ? t->psg[(sz)nd * t->nmag + (sz)mag] : -1;
    p = t->pw + (sz)nd * t->nsym;
    for(j = 0; j < t->nsym; j++) {
      i32 v = t->val[j];
      if(v == (i32)mag)
        wp += p[j];
      else if(v == -(i32)mag)
        wn += p[j];
    }
    return tcp_p(wp, wn);
  }
  INLINE int man(int nb, int p, u64 pref) const {
    const u32 *w;
    u32 nd, w0 = 0, w1 = 0, j;
    if(!usable() || nb < 2 || p > nb - 2 || t->nsym > TCP_MANMAX)
      return -1;
    nd = node();
    w = t->pw + (sz)nd * t->nsym;
    for(j = 0; j < t->nsym; j++) {
      i32 v = t->val[j];
      i32 a = v < 0 ? -v : v;
      u32 u;
      if(a < 3)
        continue;
      if(tc_ptab::nbits(a) != nb)
        continue;
      u = (u32)a - 2;
      if((u >> (nb - 1 - p)) != pref)
        continue;
      if((u >> (nb - 2 - p)) & 1)
        w1 += w[j];
      else
        w0 += w[j];
    }
    return tcp_p(w0, w1);
  }
  INLINE int axis() const { return tcp_bucket(head(0)); }
};
//  Every codebook-derived table of the current setup: one per residue book,
//  one per residue's classification book, one per floor class.
struct tc_ptabs {
  tc_ptab book[VB_MAXBOOK];
  tc_ptab cls[VB_MAXRES];
  tc_ptab flr[VB_MAXFLOOR][VB_MAXCLASS];
  tcp_arena ar;
  void reset() {
    u32 i, j;
    for(i = 0; i < VB_MAXBOOK; i++)
      book[i].drop();
    for(i = 0; i < VB_MAXRES; i++)
      cls[i].drop();
    for(i = 0; i < VB_MAXFLOOR; i++)
      for(j = 0; j < VB_MAXCLASS; j++)
        flr[i][j].drop();
    ar.reset();
  }
  void build(const vb_setup &su);
};
static tc_ptabs tcp;
void tc_ptabs::build(const vb_setup &su) {
  i32 v[TCP_SYMMAX];
  u32 i, j;
  const vb_setup *s = &su;
  reset();
  for(i = 0; i < s->nbk; i++) {
    const vb_book *b = s->bk + i;
    if(b->look != 1 || !b->ent || !b->len || !b->mult || !b->nv)
      continue;
    if(b->nv > TCP_SYMMAX)
      continue;
    for(j = 0; j < b->nv; j++)
      v[j] = (i32)b->mult[j] - (i32)b->off;
    book[i].build(ar, b->len, b->ent, b->nv, b->dim, 0, v, 1);
  }
  for(i = 0; i < s->nrs; i++) {
    const vb_res *r = s->rs + i;
    const vb_book *cb = s->bk + r->cbook;
    if(!cb->ent || !cb->len || r->ncl > TCP_SYMMAX)
      continue;
    for(j = 0; j < r->ncl; j++)
      v[j] = (i32)j;
    cls[i].build(ar, cb->len, cb->ent, r->ncl, cb->dim, 1, v, 0);
  }
  for(i = 0; i < s->nfl; i++) {
    const vb_floor *f = s->fl + i;
    u32 c;
    for(c = 0; c < VB_MAXCLASS; c++) {
      const u8 *bl[8];
      u32 be[8], nb = 1u << f->csub[c], maxy = 1, k;
      const u8 *cl = nullptr;
      u32 cent = 0;
      if(!f->cdim[c] || f->csub[c] > 3)
        continue;
      for(k = 0; k < nb; k++) {
        i32 sb = f->csb[c][k];
        if(sb >= 0 && (u32)sb < s->nbk && s->bk[sb].ent && s->bk[sb].len) {
          bl[k] = s->bk[sb].len;
          be[k] = s->bk[sb].ent;
          if(be[k] > maxy)
            maxy = be[k];
        } else if(sb >= 0) {
          bl[k] = nullptr;
          be[k] = 0;
          maxy = 0;
          break;
        } else {
          bl[k] = nullptr;
          be[k] = 1;
        }
      }
      if(!maxy || maxy > TCP_SYMMAX)
        continue;
      if(f->csub[c] && f->cbook[c] >= 0 && (u32)f->cbook[c] < s->nbk && s->bk[f->cbook[c]].ent) {
        cl = s->bk[f->cbook[c]].len;
        cent = s->bk[f->cbook[c]].ent;
      }
      flr[i][c].build_floor(ar, cl, cent, f->csub[c], f->cdim[c], bl, be, maxy);
    }
  }
}
