// decodeone.inc -- m4vpack.cpp's DecodeOne: d of a stream of one model, at its level.

// d of one model's stream on the call k, at its level LV: the bytes of the input read already
// (head, got of them) first
template <class LV>
struct DecodeOne {
  static int run(Call* k, const U8* head, size_t got) {
    typedef M4vCoro<1, LV> C;
    C* c = coro_new<C>();
    int r;
    if (!c) no_memory();
    c->Init(0);
    r = run_one(k, c, head, got);
    coro_free(c);
    return r;
  }
};
