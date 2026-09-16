template< int SZ >
struct SmallStationaryContextMap {
  static const uint t_size = SZ/2;
  word t[t_size];
  int cxt;
  word* cp;

  void Init() {
    cxt = 0;
    for (int i = 0; i < t_size; ++i) {
      t[i] = 32768;
    }
    cp = &t[0];
  }

  void set(uint cx) { W(cxt) = cx * 256 & (t_size - 256); }

  template< class Mixer, class t_c0 >
  void mix( t_c0& c0, Mixer& m, int rate = 7) {
    W(*cp) += ((y << 16) - (*cp) + (1 << (rate - 1))) >> rate;
    W(cp) = &t[cxt + c0];
    m.add(stretch((*cp) >> 4));
  }

};
