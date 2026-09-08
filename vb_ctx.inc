
struct vb_ctx {
  vb_info i;
  vb_setup * cur;             /*  the setup this link's audio runs on  */

  void init() {
    memset(&i, 0, sizeof i);
    cur = nullptr;
    vb_used = 0;
  }

  /*  Each link carries its own three header packets, so the previous link's
      setup is never referenced again.  */
  void link() { cur = nullptr;  vb_used = 0; }
};
