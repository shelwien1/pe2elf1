// tc_tabs.inc -- fam.inc's tc_tabs: a family's tables.

//  A family's tables, in the order tc_fam takes them (its class's
//  tables(), famcfg.inc): those its shape has (the counters' of its
//  class's type, C::Cnt)
template <class Cnt>
struct tc_tabs {
  Cnt *cnt[TC_NC_MAX];
  cm_cnt *T, *G;
  i32 *W, *WM, *W2, *W3, *WF;
  u8 *WC, *WMC, *W2C, *W3C, *WFC;
  i16 *S, *F;
  u8 *SC, *FC;
  void Init() { memset((void *)this, 0, sizeof *this); }  // (none)
};
