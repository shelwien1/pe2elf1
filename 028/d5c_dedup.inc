/* d5c_dedup.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * Dedup: a fixed-size table of recently seen textures, keyed by the SHA-256 of
 * their bytes.  A game container repeats whole textures -- dds_det_fail.tar has
 * four -- and a shared model still pays the index planes for every copy, so a
 * repeat costs nearly as much as the original.  A reference costs a few bytes.
 *
 * WHY THE TABLE RETAINS THE BYTES.  The encoder needs them to verify a hit with
 * a memcmp, so a hash collision loses a dedup opportunity instead of corrupting
 * the output.  The decoder needs them because a mode-3 entry IS its reference:
 * the model has moved on, so the earlier texture cannot be re-derived, only
 * remembered.  That makes the retained set the thing both sides must agree on
 * exactly -- see the note on determinism below.
 *
 * DETERMINISM.  The table evolves purely from data both sides have: the hash,
 * the length, and which entries got referenced.  Every entry that is coded is
 * inserted (the decoder hashes what it just decoded); every entry that hits is
 * touched, on both sides, because a hit on the encoder is exactly a mode-3
 * record on the decoder.  Lookups that MISS, and hits the memcmp rejects, must
 * not move anything -- the decoder never sees those.  Geometry (table size and
 * memory budget) rides in the stream rather than being a build constant, so an
 * archive stays readable by a build tuned differently.
 *
 * The table is direct-mapped: one slot per bucket, and an insert evicts
 * whatever was there.  A collision costs a missed dedup, nothing more, and the
 * alternative -- probing -- would have to replay identically on both sides for
 * no measurable gain at these table sizes. */

enum { DD_NIL = 0xFFFFFFFFu };

struct Dedup {
  struct Slot {
    Hash128 h;
    ByteBuf data;             // the retained texture; empty iff !live
    u64  pos = 0;             // where it was found in the input (report only)
    u32  ref = 0;             // "c" mode: the sidecar index the copy points at
    u32  prev = DD_NIL, next = DD_NIL;
    bool live = false;
  };

  Slot*  tab = nullptr;
  u32    mask = 0, nslot = 0;
  u32    head = DD_NIL, tail = DD_NIL;   // FIFO/LRU order over live slots
  size_t held = 0;                       // bytes retained
  size_t budget = 0;
  u32    hb = 0, mb = 0;                 // log2 of the two, as they ride the stream
  bool   on = false;
  u64    hits = 0, saved = 0;            // report: duplicates found, bytes not coded

  ~Dedup() { delete[] tab; }
  Dedup() {}
  Dedup(const Dedup&) = delete;
  Dedup& operator=(const Dedup&) = delete;

  /* hb_: log2 of the slot count, mb_: log2 of the retained-byte budget. */
  void init(u32 hb_, u32 mb_) {
    delete[] tab; tab = nullptr;
    hb = pclamp(int(hb_), 8, 26); mb = pclamp(int(mb_), 20, 46);
    nslot = 1u << hb; mask = nslot - 1;
    budget = size_t(1) << mb;
    tab = anew<Slot>(nslot, "dedup table");
    head = tail = DD_NIL; held = 0; hits = 0; saved = 0;
    on = true;
  }

  u32 bucket(const Hash128& h) const { return u32(h.a) & mask; }

  void unlink(u32 s) {
    Slot& t = tab[s];
    if(t.prev != DD_NIL) tab[t.prev].next = t.next; else head = t.next;
    if(t.next != DD_NIL) tab[t.next].prev = t.prev; else tail = t.prev;
    t.prev = t.next = DD_NIL;
  }
  void push_tail(u32 s) {
    Slot& t = tab[s];
    t.prev = tail; t.next = DD_NIL;
    if(tail != DD_NIL) tab[tail].next = s; else head = s;
    tail = s;
  }
  void drop(u32 s) {
    Slot& t = tab[s];
    if(!t.live) return;
    held -= t.data.size();
    t.data.free_all();
    t.live = false;
    unlink(s);
  }

  /* A hit: the entry is the freshest thing in the table again.  Called by the
     encoder when it emits a reference and by the decoder when it resolves one,
     so the two orderings stay identical. */
  void touch(u32 s) { if(tab[s].live) { unlink(s); push_tail(s); } }

  /* Slot holding exactly these bytes, or DD_NIL.  Verified, not trusted. */
  u32 find(const Hash128& h, const u8* d, size_t n) const {
    if(!on) return DD_NIL;
    const u32 s = bucket(h);
    const Slot& t = tab[s];
    if(!t.live || t.h != h || t.data.size() != n) return DD_NIL;
    return memcmp(t.data.data(), d, n) == 0 ? s : DD_NIL;
  }

  /* Remember these bytes.  A texture larger than the whole budget is skipped:
     retaining it would evict everything else and it could never be referenced
     without exceeding the budget anyway. */
  void insert(const Hash128& h, const u8* d, size_t n, u64 pos, u32 ref) {
    if(!on || !n || n > budget) return;         // an empty entry would match anything
    const u32 s = bucket(h);
    drop(s);                                    // whatever shared this bucket
    while(held + n > budget && head != DD_NIL) drop(head);
    Slot& t = tab[s];
    t.h = h; t.pos = pos; t.ref = ref;
    t.data.assign_copy(d, n);
    t.live = true; held += n;
    push_tail(s);
  }

  bool valid(u32 s, size_t n) const { return on && s < nslot && tab[s].live && tab[s].data.size() == n; }
};
