// compressone.inc -- call.inc's Call::CompressOne: c on one model, at a level (included in Call's
// body).

  // c on one model on the call k: its statistics into st, cost and orig
  template <class LV>
  struct CompressOne {
    static int run(Call& k, const Opts& o, Stats& st, rp<double> cost, rp<double> orig) {
      typedef M4vCoro<0, LV> C;
      C* p = coro_new<C>();
      int r;
      if (!p) no_memory();
      C& c = *p;
      c.Init(o.stats);
      r = k.run_one(c, nullptr, 0);
      st = c.st;
      memcpy(cost, c.cost, sizeof c.cost);
      memcpy(orig, c.orig, sizeof c.orig);
      coro_free(p);
      return r;
    }
  };
