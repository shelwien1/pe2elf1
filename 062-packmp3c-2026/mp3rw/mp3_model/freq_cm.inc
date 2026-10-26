// The mixing stage of mp3c's magnitude decisions (IDX/sh_model.inc:
// RangeDelta::XC): aacpack's counters and mixer (aacpack_src/cm.inc), cut
// to what the stage needs, on 12-bit probabilities of a 0 bit.  The
// stretch and squash are cm_tab.inc's constants, as are the factors of the
// rescaled reference, so that every build codes the same whatever its
// libm; the counters' rates are integer reciprocals made as the program
// starts.

#include "cm_tab.inc"

// the trailing ones of x (0..31), at most 5: a zero site's recm (the zero
// flags at this place that were set, in a row, the latest first)
static const byte cm_trail1[32] = { 0,1,0,2,0,1,0,3, 0,1,0,2,0,1,0,4, 0,1,0,2,0,1,0,3, 0,1,0,2,0,1,0,5 };

// MP3's pretab: what preflag adds to a long-block band's scalefactor
static const byte cm_pretab[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};

// the bit length of x, 0 for 0
static inline int cm_blen( uint x ) { return x ? int(log2i(x))+1 : 0; }

// 65536/(n+2): a counter's rate after n updates
struct cm_rec_t {
  word r[1026];
  cm_rec_t() { for( int i=0; i<1026; i++ ) r[i] = word( 65536/(i+2) ); }
};
static const cm_rec_t cm_rec;

// the stage's nodes (decisions, IDX/sh_model.inc: XC), and its mixer's
// inputs
enum { XND = 7, XNIN = 11 };

// A counter (aacpack's cm_cnt2): the probability of a 0, 16 bits, at an
// adaptive rate 1/(t+2) for t up to a limit, and at a fixed fast one
// (1/2^sh); both are inputs of the stage's mixer, as 12 bits.  It starts
// at 1/2 (Init), and as each update moves a probability only part of the
// way to its target, they stay in [mw, 65535-mw].
struct cm_cnt2 {
  word p, t, q;
  void Init( void ) { p = q = 32768; t = 0; }
  int P( void ) const { return p >> 4; }
  int Q( void ) const { return q >> 4; }
  void upd( int bit, int lim, int mw, int sh ) {
    int target = bit ? mw : 65535 - mw;
    t = word( t + (t < lim) );
    p = word( p + (((target - int(p)) * int(cm_rec.r[t])) >> 16) );
    q = word( q + ((target - int(q)) >> sh) );
  }
};

// The mixing core of the stages (IDX/sh_model.inc: XC): codes bit with
// mp3c's two predictions pd and ps (of a 0, 15 bits) mixed with the four
// counters c0..c3 by the weight set w (XNIN weights, 16.16), whose updates
// so far wn counts; then trains it all on the bit.  The inputs are
// stretched probabilities (+-2047, 256 a unit), the mixer's probability 12
// bits.  lim, mw and sh are the counters' (cm_cnt2::upd), lr the mixer's
// rate and mb its boost while a weight set is new (both clamped: the rate
// is at most 255, so that |x*err| < 2^31; a weight wraps, as unsigned,
// rather than saturate, which it never comes near).
template< class RangeCoder >
static INLINE void cm_code( RangeCoder& rc, int& bit, int pd, int ps,
                            cm_cnt2& c0, cm_cnt2& c1, cm_cnt2& c2, cm_cnt2& c3, int* w, byte& wn,
                            int lim, int mw, int sh, int lr, int mb ) {
  const int x0 = cm_st[pd>>3], x10 = cm_st[ps>>3];
  const int x1 = cm_st[c0.P()], x2 = cm_st[c0.Q()], x3 = cm_st[c1.P()], x4 = cm_st[c1.Q()];
  const int x5 = cm_st[c2.P()], x6 = cm_st[c2.Q()], x7 = cm_st[c3.P()], x8 = cm_st[c3.Q()];
  typedef long long ll;
  ll s = ll(w[0])*x0 + ll(w[1])*x1 + ll(w[2])*x2 + ll(w[3])*x3 + ll(w[4])*x4
       + ll(w[5])*x5 + ll(w[6])*x6 + ll(w[7])*x7 + ll(w[8])*x8 + ll(w[9])*256 + ll(w[10])*x10;
  int sm = int(s>>16); sm = sm<-2047 ? -2047 : sm>2047 ? 2047 : sm;
  const int pm = cm_sq[sm+2047];

  rc.BProcess( pm<<3, bit );

  lim = lim<1 ? 1 : lim>1023 ? 1023 : lim;
  mw = mw<0 ? 0 : mw>16384 ? 16384 : mw;
  sh = sh<1 ? 1 : sh>12 ? 12 : sh;
  c0.upd( bit, lim, mw, sh ); c1.upd( bit, lim, mw, sh );
  c2.upd( bit, lim, mw, sh ); c3.upd( bit, lim, mw, sh );
  const int k = wn; wn = byte( k + (k<255) );
  lr = lr<1 ? 1 : lr>255 ? 255 : lr;
  lr += (lr*(mb<0 ? 0 : mb>256 ? 256 : mb)*int(cm_rec.r[k])) >> 16;
  lr = lr>255 ? 255 : lr;
  const int err = ((bit ? 0 : 4096) - pm) * lr;
  #define XW(i,x) w[i] = int( unsigned(w[i]) + unsigned(((x)*err) >> 16) );
  XW(0,x0) XW(1,x1) XW(2,x2) XW(3,x3) XW(4,x4) XW(5,x5) XW(6,x6) XW(7,x7) XW(8,x8) XW(9,256) XW(10,x10)
  #undef XW
}
