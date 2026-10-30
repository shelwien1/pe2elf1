// compressone.inc -- m4vpack.cpp's CompressOne: c on one model, at a level.

// c on one model: its statistics into st, cost and orig, its counts into n
template <class LV>
struct CompressOne {
  static int run(FILE* in, const char* in_name, Sink* s, const char* out_name, const Opts* o, Stats* st, double* cost,
                 double* orig, Progress& pr, RunCounts* n) {
    typedef M4vCoro<0, LV> C;
    C* c;
    int r;
    if (!(c = coro_new<C>())) no_memory();
    c->Init(o->stats);
    pr.counts(coro_counts<C>, c);
    run_file(*c, in, s, [&](uint code) { run_show(pr, c, code); });
    pr.counts(nullptr, nullptr);
    *st = c->st;
    memcpy(cost, c->co.cost, sizeof c->co.cost);
    memcpy(orig, c->co.orig, sizeof c->co.orig);
    n->raw = c->raw_done();
    n->packed = c->packed_done();
    n->vops = c->nvop;
    r = run_end(c->status, c->note, in, in_name, s, out_name, &pr);
    coro_free(c);
    return r;
  }
};
