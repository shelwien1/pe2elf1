// partd.inc -- mt.inc's PartD: a worker's d of a part.

// A part's d at the level LV, by a worker W (M4vPart), as PartC its c: the part's record, its
// stream's header (nh bytes at head) read already, the rest (left bytes) from the worker's input,
// IO_BUF at a time; plen its size decompressed
template <class LV>
struct PartD {
  template <class W>
  static int run(W& w, const U8* head, size_t nh, U64 plen, U64& left) {
    M4vCoro<1, LV>* c = coro_new<M4vCoro<1, LV> >();
    int first = 1, r;
    if (!c) return w.fail(E_INTERNAL, "out of memory");
    c->Init(0);
    r = w.run_part(c, [&](const U8** p) {
      size_t got;
      if (first) {
        first = 0;
        *p = head;
        return nh;
      }
      got = w.read(w.in_, left < IO_BUF ? (size_t)left : IO_BUF);
      left -= got;
      *p = w.in_;
      return got;
    }, plen);
    coro_free(c);
    return r;
  }
};
