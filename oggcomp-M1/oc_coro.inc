constexpr char OC_MAGIC[] = "oggc\x1a";
constexpr int OC_VER = 3;
//  The coroutine, one per direction: f_DEC is what Rangecoder calls it, 0
//  to code and 1 to decode.  Each direction's walk is a member function
//  chosen by overload on the direction's tag, and a member function of a
//  class template is compiled only where it is called, so the decoder's
//  binary path holds no dry run and the encoder's no page writer.
template <int N> struct oc_dir {};
template <int f_DEC> struct oc_coro : Coroutine {
  oc_model<f_DEC> t;
  const char *in;
  void go(oc_dir<0>) {
    source src;
    src.open(in, &pin[0]);
    t.rc.co = this;
    t.rc.rc_Init();
    {
      og_packer<oc_model<f_DEC>> pk(t, src);
      pk.pack();
    }
    t.rc.rc_Quit();
    if(t.rc.carry_lost())
      FATAL_CODE(OGC_EXIT_INTERNAL,
                 "%s: the carryless coder lost a carry; "
                 "this is a measuring build, ship one with OC_CARRYLESS=0",
                 in);
    src.close();
  }
  void go(oc_dir<1>) {
    t.rc.co = this;
    t.rc.rc_Init();
    og_unpacker<oc_model<f_DEC>> up(t, &pin[1]);
    up.unpack(in);
  }
  void do_process() {
    go(oc_dir<f_DEC>());
    yield(this, 0);
  }
};
//  The state a new process would start a run with, for a frontend that
//  runs several: the model reset (oc_model::reset), the parser's scratch
//  and setup back to zero, the over-read count cleared, the coroutine
//  initialised and told its input's name.  After it, feeding this
//  coroutine a file gives the .oc that `oggcomp c` would.
template <int f_DEC> static void oc_fresh(oc_coro<f_DEC> &co, const char *in) {
  co.t.reset();
  tc_zero(&vb_su, sizeof vb_su, 0);
  tc_zero(vb_ys, sizeof vb_ys, 0);
  tc_zero(vb_cs, sizeof vb_cs, 0);
  tc_zero(in_win, sizeof in_win, 0);
  tc_zero(jb, sizeof jb, 0);
  tc_zero(pgbuf, sizeof pgbuf, 0);
  tc_zero(body, sizeof body, 0);
  rc_over = 0;
  oc_in = in;
  co.in = in;
  co.coro_init();
}
