// decodeone.inc -- call.inc's Call::DecodeOne: d of a stream of one model, at its level (included
// in Call's body).

  // d of one model's stream on the call k, at its level LV: the bytes of the input read already
  // (head, got of them) first
  template <class LV>
  struct DecodeOne {
    static int run(Call& k, const U8* head, size_t got) {
      typedef M4vCoro<1, LV> C;
      C* p = coro_new<C>();
      int r;
      if (!p) no_memory();
      C& c = *p;
      c.Init(0);
      r = k.run_one(c, head, got);
      coro_free(p);
      return r;
    }
  };
