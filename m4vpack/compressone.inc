// compressone.inc -- m4vpack.cpp's CompressOne: c on one model, at a level.

// c on one model on the call k: its statistics into st, cost and orig
template <class LV>
struct CompressOne {
  static int run(Call* k, const Opts* o, Stats* st, double* cost, double* orig) {
    typedef M4vCoro<0, LV> C;
    C* c = coro_new<C>();
    int r;
    if (!c) no_memory();
    c->Init(o->stats);
    r = run_one(k, c, nullptr, 0);
    *st = c->st;
    memcpy(cost, c->co.cost, sizeof c->co.cost);
    memcpy(orig, c->co.orig, sizeof c->co.orig);
    coro_free(c);
    return r;
  }
};
