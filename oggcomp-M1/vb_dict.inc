//  Codebooks from a table instead of from the stream.
//
//  libvorbis does not build codebooks when it encodes.  lib/books holds
//  several hundred static tables, generated offline once, and the setup
//  template that a stream's channel count, rate and setting select simply
//  points at them; so the books in a stream it wrote are one of a few
//  hundred fixed sets, and the setup header's largest section -- 78 to 92%
//  of its bits -- can be named instead of coded.  Which sets exist is a
//  property of the release: the tables were revised for 1.1 and again for
//  1.3, aoTuV replaced the templates that choose them, ffmpeg's own encoder
//  writes one set into everything it makes.  vbooks_gen.inc carries the
//  sets of every released libvorbis, the aoTuV betas and ffmpeg: 1,346
//  distinct books in 350 rows, generated once by psyfit/bookdump.sh in
//  tsvcomp, this program's ancestor.
//
//  A row is matched by its bits.  vg_pack() writes a table book the way
//  vorbis_staticbook_pack does -- whether the lengths go out ordered,
//  sparse or plain follows from the lengths, as it does there -- and the
//  packet's bits at the book are compared with that; a book that packs to
//  anything else, however it would parse, is a miss and is coded field by
//  field as before.  Nothing is parsed to decide.  The encoder asks which
//  row the packet's first books are the bits of and codes the row; then,
//  for each book, whether it is the row's next book; and on a miss the
//  fields.  The decoder writes the packed bits into the packet, and both
//  sides then parse them with nothing listening, so a book from the table
//  is the vb_book the same bits from the stream would have been.
//
//  A row is keyed by nothing but its bits.  The channel count and rate
//  libvorbis chooses it by are in the table and unused here: a header that
//  has been edited, or a setup one encoder borrowed from another, still
//  holds the books it holds.  Rows that list the same books -- one per
//  release that shipped them -- are one candidate, so a set is named by
//  its place among the distinct ones.
struct vg_book {
  i32 dim, entries;
  const char *lengthlist;
  i32 maptype;
  u32 q_min, q_delta; //  float32 bit patterns, as the stream carries them
  i32 q_quant, q_sequencep;
  const i32 *quantlist;
};
//  One set, over the sample rates libvorbis chooses it for; `first`
//  indexes vg_idx.
struct vg_set {
  i32 ch, rate_lo, rate_hi, n, first;
};
#include "vbooks_gen.inc"
constexpr u32 VG_NBOOK = (u32)(sizeof vg_books / sizeof *vg_books);
constexpr u32 VG_NSET = (u32)(sizeof vg_sets / sizeof *vg_sets);
//  The widest book in the table packs to 33,511 bits; vg_pack checks
//  rather than trusts.
constexpr sz VG_PACKMAX = (sz)1 << 16;
static u8 vg_buf[VG_PACKMAX];
//  The spec's lookup1_values: the largest v with v^dim <= entries.
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
//  Up to 32 bits at a bit position, least significant first, as Vorbis
//  packs them.  Reads only the bytes the bits are in: the caller has
//  checked that pos + n is inside the buffer.
static INLINE u32 vg_get(const u8 *b, sz pos, int n) {
  sz p = pos >> 3;
  int s = (int)(pos & 7), have = 0;
  u64 v = 0;
  while(have < s + n) {
    v |= (u64)b[p++] << have;
    have += 8;
  }
  return (u32)((v >> s) & (n >= 32 ? 0xFFFFFFFFu : (((u32)1 << n) - 1)));
}
struct vg_writer {
  u8 *b;
  sz pos;
  void put(u32 x, int n) {
    int i;
    FATAL_UNLESS(pos + (sz)n <= VG_PACKMAX * 8, "codebook table: a book packs past %" PRIu64 " bytes", (u64)VG_PACKMAX);
    for(i = 0; i < n; i++, pos++) {
      if(!(pos & 7))
        b[pos >> 3] = 0;
      b[pos >> 3] |= (u8)(((x >> i) & 1) << (pos & 7));
    }
  }
};
//  Book c as vorbis_staticbook_pack writes it, into dst; returns the bit
//  count.  The three length forms are chosen by the tests it makes, in
//  its order: ordered if no length is zero and none is less than the one
//  before it, else plain if none is zero, else sparse.
static sz vg_pack(const vg_book *c, u8 *dst) {
  vg_writer w = {dst, 0};
  const char *L = c->lengthlist;
  i32 i, j, count, ent = c->entries;
  w.put(0x564342, 24);
  w.put((u32)c->dim, 16);
  w.put((u32)ent, 24);
  for(i = 1; i < ent; i++)
    if(L[i - 1] == 0 || L[i] < L[i - 1])
      break;
  if(i == ent) {
    w.put(1, 1);
    w.put((u32)(L[0] - 1), 5);
    count = 0;
    for(i = 1; i < ent; i++) {
      i32 now = L[i], last = L[i - 1];
      if(now > last)
        for(j = last; j < now; j++) {
          w.put((u32)(i - count), (int)ogc_ilog((u32)(ent - count)));
          count = i;
        }
    }
    w.put((u32)(i - count), (int)ogc_ilog((u32)(ent - count)));
  } else {
    w.put(0, 1);
    for(i = 0; i < ent; i++)
      if(L[i] == 0)
        break;
    if(i == ent) {
      w.put(0, 1);
      for(i = 0; i < ent; i++)
        w.put((u32)(L[i] - 1), 5);
    } else {
      w.put(1, 1);
      for(i = 0; i < ent; i++) {
        if(L[i] == 0)
          w.put(0, 1);
        else {
          w.put(1, 1);
          w.put((u32)(L[i] - 1), 5);
        }
      }
    }
  }
  w.put((u32)c->maptype, 4);
  if(c->maptype) {
    i32 nv = c->maptype == 1 ? (i32)lk1((u32)ent, (u32)c->dim) : ent * c->dim;
    w.put(c->q_min, 32);
    w.put(c->q_delta, 32);
    w.put((u32)(c->q_quant - 1), 4);
    w.put((u32)c->q_sequencep, 1);
    for(i = 0; i < nv; i++) {
      i32 q = c->quantlist[i];
      w.put((u32)(q < 0 ? -q : q), c->q_quant);
    }
  }
  return w.pos;
}
//  Are the n bits at pos in the packet the n bits packed at pk?
static int vg_match(const u8 *pkt, sz len, sz pos, const u8 *pk, sz n) {
  sz i;
  int w;
  if(pos + n > len * 8)
    return 0;
  for(i = 0; i < n; i += (sz)w) {
    w = (int)(n - i < 32 ? n - i : 32);
    if(vg_get(pkt, pos + i, w) != vg_get(pk, i, w))
      return 0;
  }
  return 1;
}
//  The distinct rows, in table order.  Built on first use, from the table
//  alone, so the encoder and the decoder agree on what row k is.
struct vg_dict {
  u16 row[VG_NSET];
  u32 n;
  int ready;
  void build() {
    u32 i, j, k;
    n = 0;
    for(i = 0; i < VG_NSET; i++) {
      const vg_set *a = vg_sets + i;
      int dup = 0;
      for(j = 0; j < n && !dup; j++) {
        const vg_set *o = vg_sets + row[j];
        if(o->n != a->n)
          continue;
        for(k = 0; k < (u32)a->n; k++)
          if(vg_idx[a->first + k] != vg_idx[o->first + k])
            break;
        dup = k == (u32)a->n;
      }
      if(!dup)
        row[n++] = (u16)i;
    }
    ready = 1;
  }
  u32 count() {
    if(!ready)
      build();
    return n;
  }
  const vg_set *get(u32 i) {
    if(!ready)
      build();
    return vg_sets + row[i];
  }
  const vg_book *book(const vg_set *g, u32 k) const { return vg_books + vg_idx[g->first + (i32)k]; }
};
static vg_dict vg_d;
//  One more than the distinct row whose books the packet's books, from
//  pos, are the bits of for the longest run; 0 when no row's first book
//  is.  The packet holds nb books; a row may list more or fewer.
static u32 vg_find(const u8 *pkt, sz len, sz pos, u32 nb) {
  u32 i, j, best = 0, bestn = 0, n = vg_d.count();
  for(i = 0; i < n; i++) {
    const vg_set *g = vg_d.get(i);
    u32 lim = (u32)g->n < nb ? (u32)g->n : nb;
    sz p = pos;
    for(j = 0; j < lim; j++) {
      sz k = vg_pack(vg_d.book(g, j), vg_buf);
      if(!vg_match(pkt, len, p, vg_buf, k))
        break;
      p += k;
    }
    if(j > bestn) {
      bestn = j;
      best = i + 1;
    }
  }
  return best;
}
