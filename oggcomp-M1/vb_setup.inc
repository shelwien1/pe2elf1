struct vb_info {
  u32 ch, rate;
  u32 bs0, bs1;
};
constexpr u32 VB_MAXBOOK = 256;
constexpr sz VB_ARENA = (sz)96 << 20;
alignas(8) static u8 vb_arena[VB_ARENA];
static sz vb_used;
template <class T> static T *vb_alloc(sz n) {
  sz want = (n * sizeof(T) + 7) & ~(sz)7;
  u8 *p = vb_arena + vb_used;
  VB_UNLESS(want <= VB_ARENA - vb_used, "vorbis: the setup needs more than the %" PRIu64 " MiB codebook arena", (u64)(VB_ARENA >> 20));
  vb_used += want;
  return (T *)p;
}
struct vb_book {
  u32 dim, ent;
  u8 look;
  u8 *len;
  u32 *code;
  i32 *nd;
  u32 nv;
  u32 *mult;
  u32 base, off;
  u32 *inv;
  void words() {
    u32 mk[33], i, j, e, l;
    memset(mk, 0, sizeof mk);
    Fi(
        ent, l = len[i]; if(!l) continue; e = mk[l]; VB_UNLESS(l >= 32 || !(e >> l), "vorbis: over-populated codebook"); code[i] = e; for(j = l; j > 0; j--) {
          if(mk[j] & 1) {
            mk[j] = j == 1 ? mk[1] + 1 : mk[j - 1] << 1;
            break;
          }
          mk[j]++;
        } for(j = l + 1; j < 33; j++) {
          if((mk[j] >> 1) != e)
            break;
          e = mk[j];
          mk[j] = mk[j - 1] << 1;
        });
  }
  void tree() {
    u32 i, k, n = 1, u = 0, idx, cap;
    i32 t;
    Fi(ent, if(len[i]) u++);
    cap = u + 2;
    nd = vb_alloc<i32>(2 * cap);
    memset(nd, 0, 2 * cap * sizeof *nd);
    Fi(
        ent, if(!len[i]) continue; idx = 0; for(k = len[i]; k > 1; k--) {
          u32 c = 2 * idx + ((code[i] >> (k - 1)) & 1);
          t = nd[c];
          if(!t) {
            VB_UNLESS(n < cap, "vorbis: codebook tree overflows");
            t = (i32)n++;
            nd[c] = t;
          }
          VB_UNLESS(t > 0, "vorbis: codebook is not prefix free");
          idx = (u32)t;
        } idx = 2 * idx + (code[i] & 1);
        VB_UNLESS(!nd[idx], "vorbis: codebook is not prefix free"); nd[idx] = -(i32)(i + 1));
  }
  void grid() {
    u32 i, mx = 0, np;
    Fi(nv, if(mult[i] > mx) mx = mult[i]);
    base = mx + 1;
    off = base >> 1;
    inv = vb_alloc<u32>(base);
    Fi(base, inv[i] = (u32)-1);
    Fi(nv, VB_UNLESS(inv[mult[i]] == (u32)-1, "vorbis: duplicate codebook multiplicand %" PRIu64, (u64)mult[i]); inv[mult[i]] = i);
    np = 1;
    if(nv > 1)
      Fi(dim, np *= nv);
    Fi(ent, if(len[i] && i >= np) VB_REFUSE("vorbis: codebook entry %" PRIu64 " exceeds lookup grid %" PRIu64, (u64)i, (u64)np));
  }
};
constexpr u32 VB_MAXFLOOR = 64;
constexpr u32 VB_MAXCLASS = 16;
constexpr u32 VB_MAXPART = 32;
constexpr u32 VB_MAXPOST = 256;
struct vb_floor {
  u32 parts, mult, quant, posts, rng;
  u8 pcls[VB_MAXPART];
  u8 cdim[VB_MAXCLASS], csub[VB_MAXCLASS];
  i32 cbook[VB_MAXCLASS], csb[VB_MAXCLASS][8];
  u32 x[VB_MAXPOST];
  u8 srt[VB_MAXPOST];
  void sort() {
    u32 i, j, t;
    Fi(
        posts, srt[i] = (u8)i; for(j = i; j > 0 && x[srt[j - 1]] > x[srt[j]]; j--) {
          t = srt[j];
          srt[j] = srt[j - 1];
          srt[j - 1] = (u8)t;
        });
    Fi(posts, if(i) VB_UNLESS(x[srt[i]] != x[srt[i - 1]], "vorbis: floor X list repeats a value"));
  }
};
constexpr u32 VB_MAXRES = 64;
constexpr u32 VB_MAXRCL = 64;
struct vb_res {
  u32 type, beg, end, psz, ncl, cbook;
  u8 casc[VB_MAXRCL];
  i32 book[VB_MAXRCL][8];
};
constexpr u32 VB_MAXMAP = 64;
constexpr u32 VB_MAXSUB = 16;
constexpr u32 VB_MAXCH = 256;
struct vb_map {
  u32 sub, nstep;
  u8 mag[VB_MAXCH], ang[VB_MAXCH];
  u8 mux[VB_MAXCH], fl[VB_MAXSUB], rs[VB_MAXSUB];
};
constexpr u32 VB_MAXMODE = 64;
struct vb_setup {
  vb_book bk[VB_MAXBOOK];
  u32 nbk;
  vb_floor fl[VB_MAXFLOOR];
  u32 nfl;
  vb_res rs[VB_MAXRES];
  u32 nrs;
  vb_map mp[VB_MAXMAP];
  u32 nmp;
  u32 nmd;
  u8 blockflag[VB_MAXMODE], mdmap[VB_MAXMODE];
  void check() const {
    u32 i, j;
    Fi(nfl, Fj(VB_MAXCLASS, u32 k; VB_UNLESS(fl[i].cbook[j] < (i32)nbk, "vorbis: floor master book %" PRId64, (i64)fl[i].cbook[j]); Fk(8, VB_UNLESS(fl[i].csb[j][k] < (i32)nbk,
                                                                                                                                                          "vorbis: floor subclass "
                                                                                                                                                          "book %" PRId64,
                                                                                                                                                          (i64)fl[i].csb[j][k]))));
    Fi(
        nrs, u32 c, p; VB_UNLESS(rs[i].cbook < nbk, "vorbis: residue class book %" PRIu64, (u64)rs[i].cbook); VB_UNLESS(rs[i].ncl <= 16, "vorbis: %" PRIu64 " residue classifications, limit 16", (u64)rs[i].ncl); for(c = 0; c < rs[i].ncl; c++) for(p = 0; p < 8; p++) {
          i32 b = rs[i].book[c][p];
          VB_UNLESS(b < (i32)nbk, "vorbis: residue book %" PRId64, (i64)b);
          VB_UNLESS(b < 0 || bk[b].look == 1,
                       "vorbis: residue book %" PRId64 " has "
                       "no lookup table",
                       (i64)b);
        });
    Fi(nmd, VB_UNLESS(mdmap[i] < nmp,
                         "vorbis: mode %" PRIu64 " names mapping "
                         "%d",
                         (u64)i, mdmap[i]));
    Fi(nmp, Fj(mp[i].sub, VB_UNLESS(mp[i].fl[j] < nfl, "vorbis: submap floor %d", mp[i].fl[j]); VB_UNLESS(mp[i].rs[j] < nrs, "vorbis: submap residue %d", mp[i].rs[j])));
  }
};
static vb_setup vb_su;
struct vb_ctx {
  vb_info i;
  vb_setup *cur;
  void init() {
    memset(&i, 0, sizeof i);
    cur = nullptr;
    vb_used = 0;
  }
  void link() {
    cur = nullptr;
    vb_used = 0;
  }
};
