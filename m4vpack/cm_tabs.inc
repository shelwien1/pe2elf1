// cm_tabs.inc -- cm.inc's cm_tabs: the tables the counters, APMs and mixers read; cm_make_tabs()
// makes them.

//  The tables the counters, APMs and mixers read: the reciprocals of their
//  counts and squash and stretch -- constants, computed as the program
//  starts and only read after, by every model at once
struct cm_tabs {
  u16 rec[CM_TMAX + 2];
  u16 arec[CM_AMAX + 2];
  u16 mrec[CM_AMAX + 2];
  i16 st[CM_PONE + 1];  // (and one more, read in pairs)
  u16 sq[2 * CM_STMAX + 1];
  void Init() {
    int i;
    for(i = 0; i <= CM_TMAX + 1; i++)
      rec[i] = (u16)(CM_CONE / (i + 2));
    for(i = 0; i <= CM_AMAX + 1; i++)
      arec[i] = (u16)(CM_CONE / (i + 2));
    for(i = 0; i <= CM_AMAX + 1; i++)
      mrec[i] = (u16)((CM_CONE - 1) / (i + 1));
    for(i = -CM_STMAX; i <= CM_STMAX; i++) {
      double v = CM_PONE / (1.0 + exp(-i / (double)CM_SUNIT));
      int p = (int)(v + 0.5);
      if(p < 1)
        p = 1;
      if(p > CM_PONE - 1)
        p = CM_PONE - 1;
      sq[i + CM_STMAX] = (u16)p;
    }
    int j = 0;
    for(i = 0; i < CM_PONE; i++) {
      while(j < 2 * CM_STMAX && sq[j] < i)
        j++;
      st[i] = (i16)(j - CM_STMAX);
    }
    st[CM_PONE] = 0;
  }
};
static cm_tabs cm_make_tabs() {
  cm_tabs t;
  t.Init();
  return t;
}
