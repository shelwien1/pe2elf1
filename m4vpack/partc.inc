// partc.inc -- mt.inc's PartC: a worker's c of a part, at a level.

// A part's run at the level LV, by a worker W (M4vPart): c of the part in its in_ (plen bytes),
// its statistics added to the worker's
template <class LV>
struct PartC {
  template <class W>
  static int run(W& w, U64 plen) {
    M4vCoro<0, LV>* c = coro_new<M4vCoro<0, LV> >();
    U64 pos = 0;
    int r;
    if (!c) return w.fail(E_INTERNAL, "out of memory");
    c->Init(w.args.stats);
    r = w.run_part(c, [&](const U8** p) {  // in_, FEED_MAX at most at a time (addinp() takes 32 bits)
      constexpr U32 FEED_MAX = 1024 * MIB;
      const size_t l = plen - pos < FEED_MAX ? (size_t)(plen - pos) : FEED_MAX;
      *p = w.in_ + pos;
      pos += l;
      return l;
    }, ~0ull);
    if (!r) w.add_stats(c->st, c->co.cost, c->co.orig);
    coro_free(c);
    return r;
  }
};
