// decodeone.inc -- m4vpack.cpp's DecodeOne: d of a stream of one model, at its level.

// d of one model's stream, at its level LV: the bytes of the input read already (head, got of
// them) first; its counts into n
template <class LV>
struct DecodeOne {
  static int run(FILE* f, const char* in_name, Sink* s, const char* out_name, const U8* head, size_t got,
                 Progress& pr, RunCounts* n) {
    typedef M4vCoro<1, LV> C;
    C* c;
    int r;
    if (!(c = coro_new<C>())) no_memory();
    c->Init(0);
    pr.counts(coro_counts<C>, c);
    run_file(*c, f, s, [&](uint code) { run_show(pr, c, code); }, head, got);
    pr.counts(nullptr, nullptr);
    n->raw = c->raw_done();
    n->packed = c->packed_done();
    n->vops = c->nvop;
    r = run_end(c->status, c->note, f, in_name, s, out_name, &pr);
    coro_free(c);
    return r;
  }
};
