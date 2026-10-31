// partc.inc -- mt.inc's PartC: a worker's c of a part, at a level.

// A part's run at the level LV, by a worker W (Ac3Part): c of the part in its in_ (plen bytes),
// its statistics added to the worker's
template <class LV>
struct PartC {
  template <class W>
  static int run(W& w, U64 plen) {
    typedef Ac3Coro<0, LV> C;
    C* q = coro_new<C>();
    U64 pos = 0;
    int r;
    if (!q) return w.fail(E_INTERNAL, "out of memory");
    C& c = *q;
    c.Init(w.args.stats);
    r = w.run_part(c, [&](rp<const U8*> p) {  // in_, FEED_MAX at most at a time (addinp() takes 32 bits)
      constexpr U32 FEED_MAX = 1024 * MIB;
      const size_t l = plen - pos < FEED_MAX ? (size_t)(plen - pos) : FEED_MAX;
      *p = w.in_ + pos;
      pos += l;
      return l;
    }, ~0ull);
    if (!r) w.add_stats(c.st, c.cost, c.orig);
    coro_free(q);
    return r;
  }
};
