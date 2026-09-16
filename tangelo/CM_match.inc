struct MatchModel {

  int h, ptr, len, result, posnl;

  static const uint t_size = MEM;
  int t[t_size];
  SmallStationaryContextMap<0x20000> scm1;
  SmallStationaryContextMap<0x20000> scm2;

  typedef Buf<MEM*8> t_buf;
  t_buf buf;

  void Init( void ) {
    h = 0, ptr = 0, len = 0, result = 0, posnl = 0;
    scm1.Init();  
    scm2.Init();
    buf.Init();
  }

  // Match submodel. INLINE for the reason in tangelo/CM_main.inc: the
  // instrumented translation unit must define no weak symbol.
  template< class Mixer, class t_c0 >
  INLINE
  int matchModel( Mixer& m, t_c0& c0 ) {
    const int MAXLEN = 0xfffe;
    int& pos = buf.pos;

    if (!bpos) {
      W(h) = (h * 997 * 8 + buf(1) + 1) & (t_size - 1);
      if (len) {
        ++W(len), ++W(ptr);
      } else {
        W(ptr) = t[h];
        if (ptr && pos - ptr < buf.size()) {
          while (buf(len + 1) == buf[ptr - len - 1] && len < MAXLEN) ++W(len);
        }
      }
      W(t[h]) = pos;
      W(result) = len;
      scm1.set(pos);
      if (buf(1) == 0xff || buf(1) == '\r' || buf(1) == '\n') W(posnl) = pos;
      scm2.set(min(pos - posnl, 255));
    }
    if (len) {
      if (buf(1) == buf[ptr - 1] && c0 == (buf[ptr] + 256) >> (8 - bpos)) {
        if (len > MAXLEN) W(len) = MAXLEN;
        if (buf[ptr] >> (7 - bpos) & 1) {
          m.add(ilog(len) << 2);
          m.add(min(len, 32) << 6);
        } else {
          m.add(-(ilog(len) << 2));
          m.add(-(min(len, 32) << 6));
        }
      } else {
        W(len)=0;
        m.add(0);
        m.add(0);
      }
    } else {
      m.add(0);
      m.add(0);
    }
    scm1.mix(c0,m);
    scm2.mix(c0,m);
    return result;
  }

};
//#include "CM_DMC.inc"
