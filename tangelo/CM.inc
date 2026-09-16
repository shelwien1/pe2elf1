// Note: main.inc later declares the global "Coder E", which hides this class
// name at namespace scope from that point on. Nothing needs it by name after
// ContextMap below, so the two coexist; keep it that way.
struct E {
  word chk[7];
  byte last;
  byte bh[7][7];

  INLINE   // no weak symbols in the instrumented build, see CM_main.inc
  byte* get(word ch) {
    if (chk[last & 15] == ch) return &bh[last & 15][0];
    int b = 0xffff, bi = 0;
    for (int i = 0; i < 7; ++i) {
      if (chk[i] == ch) return W(last) = last << 4 | i, (byte*) &bh[i][0];
      int pri = bh[i][0];
      if (pri < b && (last & 15) != i && last >> 4 != i) b = pri, bi = i;
    }
    // Was memset(&bh[bi][0],0,7): a library call's stores are invisible to
    // the journal (they are not in the instrumented translation unit), so the
    // eviction would not be rolled back. Spelled out, it compiles to plain
    // stores that track.pl can journal.
    W(last) = 0xf0 | bi;
    W(chk[bi]) = ch;
    byte* q = &bh[bi][0];
    for( int k = 0; k < 7; ++k ) W(q[k]) = 0;
    return q;
  }

};

template< int M, int C >
struct ContextMap {

  static const uint t_size = M>>6;

  ALIGN(64) E t[t_size];
  byte* cp[C];
  byte* cp0[C];
  byte* runp[C];
  uint cxt[C];
  StateMap<256> sm[C];
  int cn;

  void Init( void ) {
    cn = 0;
    for (int i = 0; i < C; ++i) {
      cp0[i] = cp[i] = &t[0].bh[0][0];
      runp[i] = cp[i] + 3;
      sm[i].Init();
    }
  }

  void set(uint cx, int next=-1) {
    int i = W(cn)++;
    i &= next;
    cx = cx * 987654323 + i;
    cx = cx << 16 | cx >> 16;
    W(cxt[i]) = cx * 123456791 + i;
  }

  // INLINE for the same reason as Model::predictNext(): no COMDAT symbol may
  // survive in the instrumented translation unit.
  template< class Mixer, class t_buf, class t_c0 >
  INLINE
  int mix( Mixer &m, t_buf& buf, t_c0& c0 ) {  
    int result = 0;
    for (int i = 0; i < cn; ++i) {
      if (cp[i]) {
        int ns = State_table[*cp[i]][y];
        if (ns >= 204 && rnd() << ((452 - ns) >> 3)) ns -= 4;
        W(*cp[i]) = ns;
      }
      if (bpos > 1 && runp[i][0] == 0) {
        W(cp[i]) = 0;
      } else {
        switch(bpos) {
          case 1: case 3: case 6: W(cp[i]) = cp0[i] + 1 + (c0 & 1); break;
          case 4: case 7: W(cp[i]) = cp0[i] + 3 + (c0 & 3); break;
          case 2: case 5: W(cp0[i]) = W(cp[i]) = t[(cxt[i] + c0) & (t_size - 1)].get(cxt[i] >> 16); break;
          default: {
            W(cp0[i]) = W(cp[i]) = t[(cxt[i] + c0) & (t_size - 1)].get(cxt[i] >> 16);
            if (cp0[i][3] == 2) {
              const int c = cp0[i][4] + 256;
              byte *p = t[(cxt[i] + (c >> 6)) & (t_size - 1)].get(cxt[i] >> 16);
              W(p[0]) = 1 + ((c >> 5) & 1);
              W(p[1 + ((c >> 5) & 1)]) = 1 + ((c >> 4) & 1);
              W(p[3 + ((c >> 4) & 3)]) = 1 + ((c >> 3) & 1);
              p = t[(cxt[i] + (c >> 3)) & (t_size - 1)].get(cxt[i] >> 16);
              W(p[0]) = 1 + ((c >> 2) & 1);
              W(p[1 + ((c >> 2) & 1)]) = 1 + ((c >> 1) & 1);
              W(p[3 + ((c >> 1) & 3)]) = 1 + (c & 1);
              W(cp0[i][6]) = 0;
            }
            int c1 = buf(1);
            if (runp[i][0] == 0) {
              W(runp[i][0]) = 2, W(runp[i][1]) = c1;
            } else if (runp[i][1] != c1) {
              W(runp[i][0]) = 1, W(runp[i][1]) = c1;
            } else if (runp[i][0] < 254) {
              W(runp[i][0]) += 2;
            }
            W(runp[i]) = cp0[i] + 3;
          } break;
        }
      }
      if ((runp[i][1] + 256) >> (8 - bpos) == c0) {
        int rc = runp[i][0];
        int b = (runp[i][1] >> (7 - bpos) & 1) * 2 - 1;
        int c = ilog(rc+1) << (2 + (~rc & 1));
        m.add(b * c);
      } else {
        m.add(0);
      }
      int p;
      if (cp[i]) {
        result += (*cp[i] > 0);
        p = sm[i].p(*cp[i]);
      } else {
        p = sm[i].p(0);
      }
      m.add(stretch(p));
    }
    if (bpos == 7) W(cn)=0;
    return result;
  }

};
