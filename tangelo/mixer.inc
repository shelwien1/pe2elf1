// Mixer - combines models using neural networkss
template< int n, int m, int s = 1, int w = 0 >
struct Mixer: Mixer1<n,m,s,w> {
  typedef Mixer1<n,m,s,w> Base;
  using Base::N; using Base::nx; using Base::tx; using Base::wx;
  using Base::cxt; using Base::pr; using Base::ncxt;

  Mixer1<s,1,1> mp;

  void Init() {
    Mixer1<n,m,s,w>::Init();
    mp.Init();
  }

  INLINE   // no weak symbols in the instrumented build, see CM_main.inc
  int p() {
    while (nx & 7) tx[nx++] = 0;
    mp.update();
    for (int i = 0; i < ncxt; ++i) {
      pr[i] = squash(dot_product(&tx[0], &wx[cxt[i] * N], nx) >> 5);
      mp.add(stretch(pr[i]));
    }
    mp.set(0, 1);
    return mp.p();
  }

};
