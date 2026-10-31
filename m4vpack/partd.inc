// partd.inc -- mt.inc's PartD: a worker's d of a part.

// A part's d at the level LV, by a worker W (M4vPart), as PartC its c: its stream's header (M4V_HEAD
// bytes at head) read already, the rest read where it is in the worker's input (lent to the run:
// the stream ends where its decoder stops reading, and what the run leaves is the next record's);
// plen its size decompressed
template <class LV>
struct PartD {
  template <class W>
  static int run(W& w, const U8* head, U64 plen) {
    typedef M4vCoro<1, LV> C;
    C* q = coro_new<C>();
    int first = 1, r;
    if (!q) return w.fail(E_INTERNAL, "out of memory");
    C& c = *q;
    c.Init(0);
    c.is_part = 1;
    r = w.run_part(c, [&](const U8** p) -> size_t {
      if (first) {
        first = 0;
        *p = head;
        return M4V_HEAD;
      }
      return w.lend(p);
    }, plen);
    w.take_back(c);
    coro_free(q);
    return r;
  }
};
