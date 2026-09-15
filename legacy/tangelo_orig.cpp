//--- #include "common.inc"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a < b ? b : a; }

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

#define MEM (0x10000 << 7)
int y = 0;
//int c0 = 1;
int bpos = 0;
//uint c4 = 0;
//int pos;
//#include "array.inc"
//--- #include "window.inc"

// Buffer - array of n last bytes
template< int N >
struct Buf {

  int pos;
  byte b[N];

  void Init() {
    pos=0;
  }

  byte& operator[](int i) {
    return b[i & (N-1)];
  }

  int operator()(int i) const {
    return b[(pos - i) & (N-1)];
  }

  int size() const {
    return N;
  }

};

//--- #include "random.inc"

// Random generator

struct Random {

//  Array<uint> table;
  uint table[64];
  int i;

  Random() {
    table[0] = 123456789;
    table[1] = 987654321;
    for (int j = 0; j < 62; j++) {
      table[j + 2] = table[j + 1] * 11 + table[j] * 23 / 16;
    }
    i = 0;
  }

  uint operator()() {
    return ++i, table[i & 63] = table[(i - 24) & 63] ^ table[(i - 55) & 63];
  }

} rnd;
//--- #include "ilog.inc"

// Logarithm

struct Ilog {

//  Array<byte> t;
  byte t[65536];

  int operator()(word x) const { return t[x]; }

  Ilog() /*:t(65536)*/ {
    uint x = 14155776;
    for (int i = 2; i < 65536; ++i) {
      x += 774541002 / (i * 2 - 1);
      t[i] = x >> 24;
    }
  }

} ilog;

//--- #include "idiv.inc"

// Precomputed table for pt(i) = 16384 / (i + i + 3)
struct Ptable {

//  Array<int> t;
  int t[1024];

  int operator()(word x) const { return t[x]; }

  Ptable() /*:t(1024)*/ {
    for (int i = 0; i < 1024; ++i) {
      t[i] = 16384 / (i + i + 3);
    }
  }

} pt;
//--- #include "hash.inc"

// Hash
uint hash(uint a, uint b, uint c = 0xffffffff) {
  uint h = a * 200002979u + b * 30005491u + c * 50004239u + 4114959990u;
  return h ^ h >> 9 ^ a >> 2 ^ b >> 3 ^ c >> 4 ^ 0x4000000;
}
//--- #include "table_fsm.inc"

// State table
const byte State_table[256][2] = {
  {1,2},{3,5},{4,6},{7,10},{8,12},{9,13},{11,14},{15,19},{16,23},{17,24},
  {18,25},{20,27},{21,28},{22,29},{26,30},{31,33},{32,35},{32,35},{32,35},
  {32,35},{34,37},{34,37},{34,37},{34,37},{34,37},{34,37},{36,39},{36,39},
  {36,39},{36,39},{38,40},{41,43},{42,45},{42,45},{44,47},{44,47},{46,49},
  {46,49},{48,51},{48,51},{50,52},{53,43},{54,57},{54,57},{56,59},{56,59},
  {58,61},{58,61},{60,63},{60,63},{62,65},{62,65},{50,66},{67,55},{68,57},
  {68,57},{70,73},{70,73},{72,75},{72,75},{74,77},{74,77},{76,79},{76,79},
  {62,81},{62,81},{64,82},{83,69},{84,71},{84,71},{86,73},{86,73},{44,59},
  {44,59},{58,61},{58,61},{60,49},{60,49},{76,89},{76,89},{78,91},{78,91},
  {80,92},{93,69},{94,87},{94,87},{96,45},{96,45},{48,99},{48,99},{88,101},
  {88,101},{80,102},{103,69},{104,87},{104,87},{106,57},{106,57},{62,109},
  {62,109},{88,111},{88,111},{80,112},{113,85},{114,87},{114,87},{116,57},
  {116,57},{62,119},{62,119},{88,121},{88,121},{90,122},{123,85},{124,97},
  {124,97},{126,57},{126,57},{62,129},{62,129},{98,131},{98,131},{90,132},
  {133,85},{134,97},{134,97},{136,57},{136,57},{62,139},{62,139},{98,141},
  {98,141},{90,142},{143,95},{144,97},{144,97},{68,57},{68,57},{62,81},{62,81},
  {98,147},{98,147},{100,148},{149,95},{150,107},{150,107},{108,151},{108,151},
  {100,152},{153,95},{154,107},{108,155},{100,156},{157,95},{158,107},{108,159},
  {100,160},{161,105},{162,107},{108,163},{110,164},{165,105},{166,117},
  {118,167},{110,168},{169,105},{170,117},{118,171},{110,172},{173,105},
  {174,117},{118,175},{110,176},{177,105},{178,117},{118,179},{110,180},
  {181,115},{182,117},{118,183},{120,184},{185,115},{186,127},{128,187},
  {120,188},{189,115},{190,127},{128,191},{120,192},{193,115},{194,127},
  {128,195},{120,196},{197,115},{198,127},{128,199},{120,200},{201,115},
  {202,127},{128,203},{120,204},{205,115},{206,127},{128,207},{120,208},
  {209,125},{210,127},{128,211},{130,212},{213,125},{214,137},{138,215},
  {130,216},{217,125},{218,137},{138,219},{130,220},{221,125},{222,137},
  {138,223},{130,224},{225,125},{226,137},{138,227},{130,228},{229,125},
  {230,137},{138,231},{130,232},{233,125},{234,137},{138,235},{130,236},
  {237,125},{238,137},{138,239},{130,240},{241,125},{242,137},{138,243},
  {130,244},{245,135},{246,137},{138,247},{140,248},{249,135},{250,69},{80,251},
  {140,252},{249,135},{250,69},{80,251},{140,252}
};
//--- #include "stsq.inc"

int squash( int d ) {
  static const int t[33] = {
    1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,2047,2549,2994,3348,
    3607,3785,3901,3975,4022,4050,4068,4079,4085,4089,4092,4093,4094};
  if (d > 2047) return 4095;
  if (d < -2047) return 0;
  int w = d & 127;
  d = (d >> 7) + 16;
  return (t[d] * (128 - w) + t[(d + 1)] * w + 64) >> 7;
}

struct Stretch {

//  Array<short> t;
  short t[4096];

  int operator()(int x) const { return t[x]; }

  Stretch() /*:t(4096)*/ {
    int j = 0;
    for (int x = -2047; x <= 2047; ++x) {
      int i = squash(x);
      while (j <= i) t[j++] = x;
    }
    t[4095] = 2047;
  }

} stretch;
//--- #include "mixer_train.inc"

#if !defined(__GNUC__)
#if (2 == _M_IX86_FP)
# define __SSE2__
#endif
#endif
#if defined(__SSE2__)
#include <emmintrin.h>

static int dot_product (const short* const t, const short* const w, int n) {
  __m128i sum = _mm_setzero_si128 ();
  while ((n -= 8) >= 0) {
    __m128i tmp = _mm_madd_epi16 (*(__m128i *) &t[n], *(__m128i *) &w[n]);
    tmp = _mm_srai_epi32 (tmp, 8);
    sum = _mm_add_epi32 (sum, tmp);
  }
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 8));
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 4));
  return _mm_cvtsi128_si32 (sum);
}

static void train (const short* const t, short* const w, int n, const int e) {
  if (e) {
    const __m128i one = _mm_set1_epi16 (1);
    const __m128i err = _mm_set1_epi16 (short(e));
    while ((n -= 8) >= 0) {
      __m128i tmp = _mm_adds_epi16 (*(__m128i *) &t[n], *(__m128i *) &t[n]);
      tmp = _mm_mulhi_epi16 (tmp, err);
      tmp = _mm_adds_epi16 (tmp, one);
      tmp = _mm_srai_epi16 (tmp, 1);
      tmp = _mm_adds_epi16 (tmp, *(__m128i *) &w[n]);
      *(__m128i *) &w[n] = tmp;
    }
  }
}
#else

static int dot_product (const short* const t, const short* const w, int n) {
  int sum = 0;
  while ((n -= 2) >= 0) {
    sum += (t[n] * w[n] + t[n + 1] * w[n + 1]) >> 8;
  }
  return sum;
}

static void train (const short* const t, short* const w, int n, const int err) {
  if (err) {
    while ((n -= 1) >= 0) {
      int wt = w[n] + ((((t[n] * err * 2) >> 16) + 1) >> 1);
      if (wt < -32768) {
        w[n] = -32768;
      } else if (wt > 32767) {
        w[n] = 32767;
      } else {
        w[n] = wt;
      }
    }
  }
}
#endif

//--- #include "mixer1.inc"

// Mixer - combines models using neural networkss
template< int n, int m, int s = 1, int w = 0 >
struct Mixer1 {
  static const int N = (n + 7) & -8;
  static const int M = m;
  static const int S = s;

  int ncxt, base, nx;

  ALIGN(16) short tx[N];
  ALIGN(16) short wx[N*M];
  int cxt[S];
  int pr[S];

  void Init( void ) {
    ncxt=0; base=0; nx=0;
    for( int i=0; i<S; i++ ) pr[i] = 2048;
    for( int j=0; j<N*M; j++ ) wx[j] = w;
  }

  void update() {
    for( int i=0; i<ncxt; i++ ) {
      int err = ((y << 12) - pr[i]) * 7;
      train(&tx[0], &wx[cxt[i] * N], nx, err);
    }
    nx = base = ncxt = 0;
  }

  void add( int x ) { tx[nx++] = x; }

  void set( int cx, int range ) {
    cxt[ncxt++] = base + cx;
    base += range;
  }

  int p() {
    while (nx & 7) tx[nx++] = 0;
    return pr[0] = squash(dot_product(&tx[0], &wx[0], nx) >> 8);
  }

};
//--- #include "mixer.inc"

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

//--- #include "APM.inc"

// APM - stationary map combining a context and an input probability.
template< int N >
struct APM {
  int index;
  word t[N*33];

  void Init( void ) {
    index=0;
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < 33; ++j) {
        t[i * 33 + j] = i == 0 ? squash((j - 16) * 128) * 16 : t[j];
      }
    }
  }

  int p(int pr = 2048, int cxt = 0, int rate = 7) {
    pr = stretch(pr);
    int g = (y << 16) + (y << rate) - y - y;
    t[index] += (g - t[index]) >> rate;
    t[index + 1] += (g - t[index + 1]) >> rate;
    const int w = pr & 127;
    index = ((pr + 2048) >> 7) + cxt * 33;
    return(t[index] * (128 - w) + t[index + 1] * w) >> 11;
  }

};
//--- #include "counter.inc"

//  StateMap - maps a context to a probability               
template< int N >
struct StateMap {
//  const int N;
  int cxt;
//  Array<uint> t;  
  uint t[N];

//  StateMap(int n = 256):N(n), cxt(0), t(n) {

  void Init( void ) {
    cxt = 0;
    for (int i = 0; i < N; ++i) {
      t[i] = 1 << 31;
    }
  }

  int p(int cx) {
    uint *p = &t[cxt], p0 = p[0];
    int n = p0 & 1023, pr = p0 >> 10;
    if (n < 1023) ++p0;
    else p0 = (p0 & 0xfffffc00) | 1023;
    p0 += (((y << 22) - pr) >> 3) * pt(n) & 0xfffffc00;
    p[0] = p0;
    return t[cxt = cx] >> 20;
  }

};
//--- #include "CM_small.inc"

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

  void set(uint cx) { cxt = cx * 256 & (t_size - 256); }

  template< class Mixer, class t_c0 >
  void mix( t_c0& c0, Mixer& m, int rate = 7) {
    *cp += ((y << 16) - (*cp) + (1 << (rate - 1))) >> rate;
    cp = &t[cxt + c0];
    m.add(stretch((*cp) >> 4));
  }

};
//--- #include "CM.inc"

struct E {
  word chk[7];
  byte last;
  byte bh[7][7];

  byte* get(word ch) {
    if (chk[last & 15] == ch) return &bh[last & 15][0];
    int b = 0xffff, bi = 0;
    for (int i = 0; i < 7; ++i) {
      if (chk[i] == ch) return last = last << 4 | i, (byte*) &bh[i][0];
      int pri = bh[i][0];
      if (pri < b && (last & 15) != i && last >> 4 != i) b = pri, bi = i;
    }
    return last = 0xf0 | bi, chk[bi] = ch, (byte*)memset(&bh[bi][0], 0, 7);
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
    int i = cn++;
    i &= next;
    cx = cx * 987654323 + i;
    cx = cx << 16 | cx >> 16;
    cxt[i] = cx * 123456791 + i;
  }

  template< class Mixer, class t_buf, class t_c0 >
  int mix( Mixer &m, t_buf& buf, t_c0& c0 ) {  
    int result = 0;
    for (int i = 0; i < cn; ++i) {
      if (cp[i]) {
        int ns = State_table[*cp[i]][y];
        if (ns >= 204 && rnd() << ((452 - ns) >> 3)) ns -= 4;
        *cp[i] = ns;
      }
      if (bpos > 1 && runp[i][0] == 0) {
        cp[i] = 0;
      } else {
        switch(bpos) {
          case 1: case 3: case 6: cp[i] = cp0[i] + 1 + (c0 & 1); break;
          case 4: case 7: cp[i] = cp0[i] + 3 + (c0 & 3); break;
          case 2: case 5: cp0[i] = cp[i] = t[(cxt[i] + c0) & (t_size - 1)].get(cxt[i] >> 16); break;
          default: {
            cp0[i] = cp[i] = t[(cxt[i] + c0) & (t_size - 1)].get(cxt[i] >> 16);
            if (cp0[i][3] == 2) {
              const int c = cp0[i][4] + 256;
              byte *p = t[(cxt[i] + (c >> 6)) & (t_size - 1)].get(cxt[i] >> 16);
              p[0] = 1 + ((c >> 5) & 1);
              p[1 + ((c >> 5) & 1)] = 1 + ((c >> 4) & 1);
              p[3 + ((c >> 4) & 3)] = 1 + ((c >> 3) & 1);
              p = t[(cxt[i] + (c >> 3)) & (t_size - 1)].get(cxt[i] >> 16);
              p[0] = 1 + ((c >> 2) & 1);
              p[1 + ((c >> 2) & 1)] = 1 + ((c >> 1) & 1);
              p[3 + ((c >> 1) & 3)] = 1 + (c & 1);
              cp0[i][6] = 0;
            }
            int c1 = buf(1);
            if (runp[i][0] == 0) {
              runp[i][0] = 2, runp[i][1] = c1;
            } else if (runp[i][1] != c1) {
              runp[i][0] = 1, runp[i][1] = c1;
            } else if (runp[i][0] < 254) {
              runp[i][0] += 2;
            }
            runp[i] = cp0[i] + 3;
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
    if (bpos == 7) cn=0;
    return result;
  }

};
//--- #include "CM_match.inc"

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

  // Match submodel
  template< class Mixer, class t_c0 >
  int matchModel( Mixer& m, t_c0& c0 ) {
    const int MAXLEN = 0xfffe;
    int& pos = buf.pos;

    if (!bpos) {
      h = (h * 997 * 8 + buf(1) + 1) & (t_size - 1);
      if (len) {
        ++len, ++ptr;
      } else {
        ptr = t[h];
        if (ptr && pos - ptr < buf.size()) {
          while (buf(len + 1) == buf[ptr - len - 1] && len < MAXLEN) ++len;
        }
      }
      t[h] = pos;
      result = len;
      scm1.set(pos);
      if (buf(1) == 0xff || buf(1) == '\r' || buf(1) == '\n') posnl = pos;
      scm2.set(min(pos - posnl, 255));
    }
    if (len) {
      if (buf(1) == buf[ptr - 1] && c0 == (buf[ptr] + 256) >> (8 - bpos)) {
        if (len > MAXLEN) len = MAXLEN;
        if (buf[ptr] >> (7 - bpos) & 1) {
          m.add(ilog(len) << 2);
          m.add(min(len, 32) << 6);
        } else {
          m.add(-(ilog(len) << 2));
          m.add(-(min(len, 32) << 6));
        }
      } else {
        len=0;
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
//--- #include "CM_main.inc"

// Main model - predicts next bit probability from previous data

struct Model {

  uint mask, mask2, word0, word1;
  uint c0, c4;

  uint cxt[15]; 
  uint t1[0x100];
  word t2[0x10000];

  ALIGN(64) ContextMap<MEM * 32, 22> cm;
  APM<0x100> a1;
  APM<0x10000> a2;
  APM<0x10000> a3;
  MatchModel match;
  Mixer<73, 1160, 5> mixer;

  void Init() {
    mask = 0, mask2 = 0, word0 = 0, word1 = 0;
    c0=1; c4=0;
    cm.Init();
    a1.Init(); a2.Init(); a3.Init();
    match.Init();
    mixer.Init();
  }

  int predictNext() {
    MatchModel::t_buf& buf = match.buf;
    int& pos = match.buf.pos;

    c0 += c0 + y;
    if (c0 >= 256) {
      buf[pos++] = c0;
      c4 = (c4 << 8) + c0 - 256;
      c0 = 1;
    }   
    bpos = (bpos + 1) & 7;  
    int c1 = c4 & 0xff, c2 = (c4 & 0xff00) >> 8;

    mixer.update();
    int ismatch = ilog(match.matchModel(mixer,c0));
  //  dmcModel(mixer);

    if (bpos == 0) {
      for (int i = 14; i > 0; --i) {
        cxt[i] = hash(cxt[i - 1], c1);
      }
      cm.set(0);
      cm.set(c1);
      cm.set(c4 & 0x0000ffff);
      cm.set(c4 & 0x00ffffff);
      cm.set(c4);
      cm.set(cxt[5]);
      cm.set(cxt[6]);
      cm.set(cxt[14]);
      cm.set(c4 & 0xf8f8c0ff);
      cm.set(c4 & 0x00e0e0e0);
      cm.set(c4 & 0xffc0ff80);
      cm.set(ismatch | (c4 & 0xffff0000));
      cm.set(ismatch | (c4 & 0x0000ff00));
      cm.set(ismatch | (c4 & 0x00ff0000));
      mask = (mask << 3) | (!c1 ? 0 : isalpha(c1) ? 1 : ispunct(c1) ? 2 :
        isspace(c1) ? 3 : (c1 == 255) ? 4 : (c1 < 16) ? 5 : (c1 < 64) ? 6 : 7);
      cm.set(mask);
      mask2 = (mask2 << 3) | ((mask >> 27) & 7);
      cm.set(hash(mask << 5, mask2 << 2));
      uint& ic1r = t1[c2];
      ic1r = ic1r << 8 | c1;
      word& ic2r = t2[(buf(3) << 8) | c2];
      ic2r = ic2r << 8 | c1;
      const uint ic1 = c1 | t1[c1] << 8;
      const uint ic2 = ((c2 << 8) | c1) | t2[(c2 << 8) | c1] << 16;
      cm.set((ic1 >> 8) & ((1 << 16) - 1));
      cm.set((ic2 >> 16) & ((1 << 8) - 1));
      cm.set(ic1 & ((1 << 16) - 1));
      cm.set(ic2 & ((1 << 24) - 1));  
      //cm.set(hash(4 * (buf(5) == 0x0f) + 8 * (buf(5) == 0x66)
      //       + 12 * (buf(5) == 0x67) + (buf(4) == 0x0f) + 2 * (buf(4) == 0x66)
      //       + 3 * (buf(4) == 0x67), buf(3), buf(2) & 0xc7));   
      int c = (c1 >= 'A' && c1 <= 'Z') ? c1 + 'a' - 'A' : c1;
      if ((c >= 'a' && c <= 'z') || c >= 128) word0 = hash(word0, c);
      else if (word0) word1 = word0, word0 = 0;
      cm.set(word0);
      cm.set(hash(word0, word1));
    }
    
    int o = cm.mix(mixer,buf,c0);
    mixer.set(c1 + 8, 264);
    mixer.set(c0, 256);
    mixer.set(o + ((c1 > 32) << 4) + ((bpos == 0) << 5) + ((c1 == c2) << 6), 128);
    mixer.set(c2, 256);
    mixer.set(ismatch, 256);  
       
    int pr0 = mixer.p();
    return (a1.p(pr0, c0) * 5
           + a2.p(pr0, c0 + (c1 << 8)) * 15
           + a3.p(pr0, hash(bpos, c1, c2) & 0xffff) * 12
           + 16) >> 5; // Probability adjusted with 3 APMs                       
  }

};

//ALIGN(64) Model M;
//ALIGN(64) byte M_memory[sizeof(Model)];


//--- #include "RC.inc"

// Encoder - Arithmetic coding
template< uint mode >
struct Encoder {
//  uint mode;
  uint x, x1, x2;
  int p;  

  byte* rcptr;
  uint get( void ) { return *rcptr++; }
  void put( byte c ) { *rcptr++ = c; }

  int code( int i, Model& M ) {
    p += p < 2048;  
    uint xmid = x1 +((x2 - x1) >> 12) * p + (((x2 - x1) & 0xfff) * p >> 12);
    if (!mode) y = x <= xmid; else y = i;
    y ? (x2 = xmid) : (x1 = xmid+1);
    p = M.predictNext(); // Update models and predict next bit probability
    while (((x1 ^ x2) & 0xff000000) == 0) {
      if (mode) put(x2 >> 24);
      x1 <<= 8;
      x2 = (x2 << 8) + 255;
      if (!mode) x = (x << 8) + (get() & 255);
    }
    return y;
  }

  Encoder(): x(0), x1(0), x2(0xffffffff), p(2048) {}

  void prefetch() {
    for (int i = 0; i < 4; ++i) {
      x = (x << 8) + (get() & 255);
    }
  }

  void flush() {
    put(x1 >> 24);
  }  

  void compress(int c,Model& M) {
    for (int i = 7; i >= 0; --i) {
      code((c >> i) & 1,M);
    }
  }

  int decompress(Model& M) {
    int c = 0;
    for (int i = 0; i < 8; ++i) {
      c += c + code(0,M);
    }
    return c;
  }

};


int main( int argc, char** argv ) {

  if( argc<4 ) return 2;

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen( argv[2], "rb" ); if( f==0 ) return 2;
  FILE* g = fopen( argv[3], "wb" ); if( g==0 ) return 2;

  uint i, f_len = flen(f), g_len, outSize;
  byte* in = new byte[f_len+4096]; if( in==0 ) return 3;
  for( i=0; i<f_len+4096; i++ ) in[i]=0xFF;

  if( f_DEC==0 ) {
    fwrite( &f_len,1,4, g );
    g_len = f_len;
  } else {
    fread( &g_len,1,4, f );
  }

  f_len = fread( in, 1,f_len, f );
  fclose(f);

//printf( "f_len=%i g_len=%i\n", f_len, g_len );

  byte* out = new byte[g_len+4096]; if( out==0 ) return 4;

//  Model& M = *(Model*)M_memory;
//memset( M_memory, 0, sizeof(M_memory) );
  byte* M_ptr = new byte[ sizeof(Model)+4096 ];
  M_ptr += (4096-(M_ptr-((byte*)0))%4096)%4096;
  Model& M = *(Model*)M_ptr;

  M.Init();

  if( f_DEC==0 ) {
#ifndef DISABLE_ENC
    Encoder<1> en;
    en.rcptr = out;
    for( i=0; i<f_len; i++) en.compress(*in++,M);
    en.flush(); outSize=en.rcptr-out;
#endif
  } else {
#ifndef DISABLE_DEC
    Encoder<0> en;
    en.rcptr = in;
    en.prefetch();
    for( i=0,outSize=0; i<g_len; i++ ) out[outSize++]=en.decompress(M);
#endif
  }

  fwrite( out, 1,outSize, g );

  fclose(g);

  return 0;
}


