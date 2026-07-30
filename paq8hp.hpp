#ifndef NDEBUG
#define NDEBUG
#endif

#ifndef DEFAULT_OPTION
#define DEFAULT_OPTION 8
#endif

#ifndef NOASM
#define NOASM
#endif

namespace paq8hp {

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

#ifndef min
inline int min(int a, int b) {
  return a<b ? a : b;
}

inline int max(int a, int b) {
  return a<b ? b : a;
}

#endif

typedef enum { DEFAULT, JPEG, EXE, BINTEXT, TEXT } Filetype;

#define preprocFlag 1220

#define OPTION_UTF8 1
#define OPTION_USE_NGRAMS 2
#define OPTION_CAPITAL_CONVERSION 4
#define OPTION_WORD_SURROROUNDING_MODELING 8
#define OPTION_SPACE_AFTER_EOL 16
#define OPTION_EOL_CODING 32
#define OPTION_NORMAL_TEXT_FILTER 64
#define OPTION_USE_DICTIONARY 128
#define OPTION_RECORD_INTERLEAVING 256
#define OPTION_DNA_QUARTER_BYTE 512
#define OPTION_TRY_SHORTER_WORD 1024
#define OPTION_TO_LOWER_AFTER_PUNCTUATION 2048
#define OPTION_SPACELESS_WORDS 4096
#define OPTION_ADD_SYMBOLS_0_5 8192
#define OPTION_ADD_SYMBOLS_14_31 16384
#define OPTION_ADD_SYMBOLS_A_Z 32768
#define OPTION_ADD_SYMBOLS_MISC 65536
#define OPTION_SPACE_AFTER_CC_FLAG 131072
#define IF_OPTION(option) ((preprocFlag&option)!=0)

struct Random {
U32 table[64];
int i;

Random() {
  table[0] = 123456789;
  table[1] = 987654321;
  for( int j = 0; j<62; j++ )
    table[j+2] = table[j+1]*11+table[j]*23/16;
  i = 0;
}

U32 operator()() {
  return ++i, table[i&63] = table[(i-24)&63]^table[(i-55)&63];
}
} rnd;

int pos;

// SZ must be a power of two.  Statically sized now, so setsize() is gone; the
// size is the 1<<30 the PAQ8HP wrapper used to pass, not a function of level.
template <U32 SZ> struct Buf {
U8 b[SZ];

U8 &operator[](U32 i) {
  return b[i&(SZ-1)];
}

int operator()(U32 i) const {
  return b[(pos-i)&(SZ-1)];
}

U32 size() const {
  return SZ;
}
};

// was MEM() over the global int level.  The baseline computed 0x10000<<level in
// int, which is UB for level>=15; done in U64 here, same result for every level
// this build accepts.  Levels above 11 are rejected by the dispatcher because
// (U32)MEM()*16 - the size the two big ContextMaps are built from - wraps to 0
// at level 12 and the baseline would have indexed a null table.
constexpr unsigned long long MEM(int L) {
  return (0x10000ULL<<L)>0xFFFFFFFF ? 0xFFFFFFFF : (0x10000ULL<<L);
}

int y = 0;

int c0 = 1;
U32 b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0, b8 = 0, tt = 0, c4 = 0, x4 = 0, x5 = 0, w4 = 0, w5 = 0, f4 = 0;
int order, bpos = 0, cxtfl = 3, sm_shft = 7, sm_add = 65535+127, sm_add_y = 0;

// level-independent, and read by every model, so it stays at file scope with
// the rest of the shared scalar state rather than moving into Predictor<L>
Buf<1u<<30> buf;

struct Ilog {
U8 t[65536];

int operator()(U16 x) const {
  return t[x];
}

Ilog() {
  U32 x = 14155776;
  for( int i = 2; i<65536; ++i ) {
    x += 774541002/(i*2-1);
    t[i] = x>>24;
  }
}
} ilog;

inline int llog(U32 x) {
  if( x>=0x1000000 )
    return 256+ilog(x>>16);
  else if( x>=0x10000 )
    return 128+ilog(x>>8);
  else
    return ilog(x);
}

static const U8 State_table[256][4] = {{1, 2, 0, 0}, {3, 5, 1, 0}, {4, 6, 0, 1}, {7, 10, 2, 0}, {8, 12, 1, 1}, {9, 13, 1, 1}, {11, 14, 0, 2}, {15, 19, 3, 0}, {16, 23, 2, 1}, {17, 24, 2, 1}, {18, 25, 2, 1}, {20, 27, 1, 2}, {21, 28, 1, 2}, {22, 29, 1, 2}, {26, 30, 0, 3}, {31, 33, 4, 0}, {32, 35, 3, 1}, {32, 35, 3, 1}, {32, 35, 3, 1}, {32, 35, 3, 1}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {36, 39, 1, 3}, {36, 39, 1, 3}, {36, 39, 1, 3}, {36, 39, 1, 3}, {38, 40, 0, 4}, {41, 43, 5, 0}, {42, 45, 4, 1}, {42, 45, 4, 1}, {44, 47, 3, 2}, {44, 47, 3, 2}, {46, 49, 2, 3}, {46, 49, 2, 3}, {48, 51, 1, 4}, {48, 51, 1, 4}, {50, 52, 0, 5}, {53, 43, 6, 0}, {54, 57, 5, 1},
  {54, 57, 5, 1}, {56, 59, 4, 2}, {56, 59, 4, 2}, {58, 61, 3, 3}, {58, 61, 3, 3}, {60, 63, 2, 4}, {60, 63, 2, 4}, {62, 65, 1, 5}, {62, 65, 1, 5}, {50, 66, 0, 6}, {67, 55, 7, 0}, {68, 57, 6, 1}, {68, 57, 6, 1}, {70, 73, 5, 2}, {70, 73, 5, 2}, {72, 75, 4, 3}, {72, 75, 4, 3}, {74, 77, 3, 4}, {74, 77, 3, 4}, {76, 79, 2, 5}, {76, 79, 2, 5}, {62, 81, 1, 6}, {62, 81, 1, 6}, {64, 82, 0, 7}, {83, 69, 8, 0}, {84, 71, 7, 1}, {84, 71, 7, 1}, {86, 73, 6, 2}, {86, 73, 6, 2}, {44, 59, 5, 3}, {44, 59, 5, 3}, {58, 61, 4, 4}, {58, 61, 4, 4}, {60, 49, 3, 5}, {60, 49, 3, 5}, {76, 89, 2, 6}, {76, 89, 2, 6}, {78, 91, 1, 7}, {78, 91, 1, 7}, {80, 92, 0, 8}, {93, 69, 9, 0}, {94, 87, 8, 1}, {94, 87, 8, 1},
  {96, 45, 7, 2}, {96, 45, 7, 2}, {48, 99, 2, 7}, {48, 99, 2, 7}, {88, 101, 1, 8}, {88, 101, 1, 8}, {80, 102, 0, 9}, {103, 69, 10, 0}, {104, 87, 9, 1}, {104, 87, 9, 1}, {106, 57, 8, 2}, {106, 57, 8, 2}, {62, 109, 2, 8}, {62, 109, 2, 8}, {88, 111, 1, 9}, {88, 111, 1, 9}, {80, 112, 0, 10}, {113, 85, 11, 0}, {114, 87, 10, 1}, {114, 87, 10, 1}, {116, 57, 9, 2}, {116, 57, 9, 2}, {62, 119, 2, 9}, {62, 119, 2, 9}, {88, 121, 1, 10}, {88, 121, 1, 10}, {90, 122, 0, 11}, {123, 85, 12, 0}, {124, 97, 11, 1}, {124, 97, 11, 1}, {126, 57, 10, 2}, {126, 57, 10, 2}, {62, 129, 2, 10}, {62, 129, 2, 10}, {98, 131, 1, 11}, {98, 131, 1, 11}, {90, 132, 0, 12}, {133, 85, 13, 0}, {134, 97, 12, 1}, {134, 97, 12, 1}, {136, 57, 11, 2}, {136, 57, 11, 2}, {62, 139, 2, 11},
  {62, 139, 2, 11}, {98, 141, 1, 12}, {98, 141, 1, 12}, {90, 142, 0, 13}, {143, 95, 14, 0}, {144, 97, 13, 1}, {144, 97, 13, 1}, {68, 57, 12, 2}, {68, 57, 12, 2}, {62, 81, 2, 12}, {62, 81, 2, 12}, {98, 147, 1, 13}, {98, 147, 1, 13}, {100, 148, 0, 14}, {149, 95, 15, 0}, {150, 107, 14, 1}, {150, 107, 14, 1}, {108, 151, 1, 14}, {108, 151, 1, 14}, {100, 152, 0, 15}, {153, 95, 16, 0}, {154, 107, 15, 1}, {108, 155, 1, 15}, {100, 156, 0, 16}, {157, 95, 17, 0}, {158, 107, 16, 1}, {108, 159, 1, 16}, {100, 160, 0, 17}, {161, 105, 18, 0}, {162, 107, 17, 1}, {108, 163, 1, 17}, {110, 164, 0, 18}, {165, 105, 19, 0}, {166, 117, 18, 1}, {118, 167, 1, 18}, {110, 168, 0, 19}, {169, 105, 20, 0}, {170, 117, 19, 1}, {118, 171, 1, 19}, {110, 172, 0, 20}, {173, 105, 21, 0}, {174, 117, 20, 1}, {118, 175, 1, 20},
  {110, 176, 0, 21}, {177, 105, 22, 0}, {178, 117, 21, 1}, {118, 179, 1, 21}, {110, 180, 0, 22}, {181, 115, 23, 0}, {182, 117, 22, 1}, {118, 183, 1, 22}, {120, 184, 0, 23}, {185, 115, 24, 0}, {186, 127, 23, 1}, {128, 187, 1, 23}, {120, 188, 0, 24}, {189, 115, 25, 0}, {190, 127, 24, 1}, {128, 191, 1, 24}, {120, 192, 0, 25}, {193, 115, 26, 0}, {194, 127, 25, 1}, {128, 195, 1, 25}, {120, 196, 0, 26}, {197, 115, 27, 0}, {198, 127, 26, 1}, {128, 199, 1, 26}, {120, 200, 0, 27}, {201, 115, 28, 0}, {202, 127, 27, 1}, {128, 203, 1, 27}, {120, 204, 0, 28}, {205, 115, 29, 0}, {206, 127, 28, 1}, {128, 207, 1, 28}, {120, 208, 0, 29}, {209, 125, 30, 0}, {210, 127, 29, 1}, {128, 211, 1, 29}, {130, 212, 0, 30}, {213, 125, 31, 0}, {214, 137, 30, 1}, {138, 215, 1, 30}, {130, 216, 0, 31}, {217, 125, 32, 0}, {218, 137, 31, 1},
  {138, 219, 1, 31}, {130, 220, 0, 32}, {221, 125, 33, 0}, {222, 137, 32, 1}, {138, 223, 1, 32}, {130, 224, 0, 33}, {225, 125, 34, 0}, {226, 137, 33, 1}, {138, 227, 1, 33}, {130, 228, 0, 34}, {229, 125, 35, 0}, {230, 137, 34, 1}, {138, 231, 1, 34}, {130, 232, 0, 35}, {233, 125, 36, 0}, {234, 137, 35, 1}, {138, 235, 1, 35}, {130, 236, 0, 36}, {237, 125, 37, 0}, {238, 137, 36, 1}, {138, 239, 1, 36}, {130, 240, 0, 37}, {241, 125, 38, 0}, {242, 137, 37, 1}, {138, 243, 1, 37}, {130, 244, 0, 38}, {245, 135, 39, 0}, {246, 137, 38, 1}, {138, 247, 1, 38}, {140, 248, 0, 39}, {249, 135, 40, 0}, {250, 69, 39, 1}, {80, 251, 1, 39}, {140, 252, 0, 40}, {249, 135, 41, 0}, {250, 69, 40, 1}, {80, 251, 1, 40}, {140, 252, 0, 41}};

#define nex(state, sel) State_table[state][sel]

int squash(int d) {
  static const int t[33] = {1, 2, 3, 6, 10, 16, 27, 45, 73, 120, 194, 310, 488, 747, 1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785, 3901, 3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094};
  if( d>2047 )
    return 4095;
  if( d<-2047 )
    return 0;
  int w = d&127;
  d = (d>>7)+16;
  return (t[d]*(128-w)+t[(d+1)]*w+64)>>7;
}

struct Stretch {
short t[4096];

Stretch() {
  int pi = 0;
  for( int x = -2047; x<=2047; ++x ) {
    int i = squash(x);
    for( int j = pi; j<=i; ++j )
      t[j] = x;
    pi = i+1;
  }
  t[4095] = 2047;
}

int operator()(int p) const {
  return t[p];
}
} stretch;

#if !defined(__GNUC__)

#if (2==_M_IX86_FP)
#define __SSE2__
#elif (1==_M_IX86_FP)
#define __SSE__
#endif

#endif

static int dot_product(const short*const t, const short*const w, int n);

static void train(const short*const t, short*const w, int n, const int e);

#if defined(__SSE2__)
#include <emmintrin.h>
#define OPTIMIZE "SSE2-"

static int dot_product(const short*const t, const short*const w, int n) {
  __m128i sum = _mm_setzero_si128();
  while( (n -= 8)>=0 ) {
    __m128i tmp = _mm_madd_epi16(*(__m128i*)&t[n], *(__m128i*)&w[n]);
    tmp = _mm_srai_epi32(tmp, 8);
    sum = _mm_add_epi32(sum, tmp);
  }
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
  return _mm_cvtsi128_si32(sum);
}

static void train(const short*const t, short*const w, int n, const int e) {
  if( e ) {
    const __m128i one = _mm_set1_epi16(1);
    const __m128i err = _mm_set1_epi16(short(e));
    while( (n -= 8)>=0 ) {
      __m128i tmp = _mm_adds_epi16(*(__m128i*)&t[n], *(__m128i*)&t[n]);
      tmp = _mm_mulhi_epi16(tmp, err);
      tmp = _mm_adds_epi16(tmp, one);
      tmp = _mm_srai_epi16(tmp, 1);
      tmp = _mm_adds_epi16(tmp, *(__m128i*)&w[n]);
      *(__m128i*)&w[n] = tmp;
    }
  }
}

#elif defined(__SSE__)
#include <xmmintrin.h>
#define OPTIMIZE "SSE-"

static int dot_product(const short*const t, const short*const w, int n) {
  __m64 sum = _mm_setzero_si64();
  while( (n -= 8)>=0 ) {
    __m64 tmp = _mm_madd_pi16(*(__m64*)&t[n], *(__m64*)&w[n]);
    tmp = _mm_srai_pi32(tmp, 8);
    sum = _mm_add_pi32(sum, tmp);

    tmp = _mm_madd_pi16(*(__m64*)&t[n+4], *(__m64*)&w[n+4]);
    tmp = _mm_srai_pi32(tmp, 8);
    sum = _mm_add_pi32(sum, tmp);
  }
  sum = _mm_add_pi32(sum, _mm_srli_si64(sum, 32));
  const int retval = _mm_cvtsi64_si32(sum);
  _mm_empty();
  return retval;
}

static void train(const short*const t, short*const w, int n, const int e) {
  if( e ) {
    const __m64 one = _mm_set1_pi16(1);
    const __m64 err = _mm_set1_pi16(short(e));
    while( (n -= 8)>=0 ) {
      __m64 tmp = _mm_adds_pi16(*(__m64*)&t[n], *(__m64*)&t[n]);
      tmp = _mm_mulhi_pi16(tmp, err);
      tmp = _mm_adds_pi16(tmp, one);
      tmp = _mm_srai_pi16(tmp, 1);
      tmp = _mm_adds_pi16(tmp, *(__m64*)&w[n]);
      *(__m64*)&w[n] = tmp;

      tmp = _mm_adds_pi16(*(__m64*)&t[n+4], *(__m64*)&t[n+4]);
      tmp = _mm_mulhi_pi16(tmp, err);
      tmp = _mm_adds_pi16(tmp, one);
      tmp = _mm_srai_pi16(tmp, 1);
      tmp = _mm_adds_pi16(tmp, *(__m64*)&w[n+4]);
      *(__m64*)&w[n+4] = tmp;
    }
    _mm_empty();
  }
}

#else
int dot_product(short* t, short* w, int n) {
  int sum = 0;
  n = (n+15)& -16;
  for( int i = 0; i<n; i += 2 )
    sum += (t[i]*w[i]+t[i+1]*w[i+1])>>8;
  return sum;
}

void train(short* t, short* w, int n, int err) {
  n = (n+15)& -16;
  for( int i = 0; i<n; ++i ) {
    int wt = w[i]+(((t[i]*err*2>>16)+1)>>1);
    if( wt<-32768 )
      wt = -32768;
    if( wt>32767 )
      wt = 32767;
    w[i] = wt;
  }
}

#endif

// The nested mixer the baseline built with `new Mixer(S,1,1,0x7fff)` when S>1.
// It has to be held by value now, but a Mixer with S==1 must not contain one or
// the instantiation would never terminate, so the holder is specialised empty
// for S==1 and forwards the three calls the outer p() makes.
template <int n, int m, int s = 1, int w = 0> struct Mixer;

template <int S> struct SubMixer {
Mixer<S, 1, 1, 0x7fff> mp;

void update2() {
  mp.update2();
}

void add(int x) {
  mp.add(x);
}

int p() {
  return mp.p();
}
};

template <> struct SubMixer<1> {
void update2() {
}

void add(int) {
}

int p() {
  return 0;
}
};

template <int n, int m, int s, int w> struct Mixer {
enum { N = (n+7)& -8, M = m, S = s };

// 16-byte alignment is load-bearing: dot_product/train do aligned __m128i
// accesses, and Array<short,16> used to supply it.  N is a multiple of 8, so
// every wx row start is 16-byte aligned too.
alignas(16) short wx[N*M];
int cxt[S];
int ncxt;
int base;
int pr[S];
SubMixer<S> sub;

alignas(16) short tx[N];
int nx;

Mixer() : ncxt(0), base(0), nx(0) {
  int i;
  for( i = 0; i<S; ++i )
    pr[i] = 2048;
  for( i = 0; i<N*M; ++i )
    wx[i] = w;
  for( i = 0; i<S; ++i )
    cxt[i] = 0;
  for( i = 0; i<N; ++i )
    tx[i] = 0;
}

void update() {
  for( int i = 0; i<ncxt; ++i ) {
    int err = ((y<<12)-pr[i])*7;
    train(&tx[0], &wx[cxt[i]*N], nx, err);
  }
  nx = base = ncxt = 0;
}

void update2() {
  train(&tx[0], &wx[0], nx, ((y<<12)-base)*3/2);
  nx = 0;
}

void add(int x) {
  tx[nx++] = x;
}

void mul(int x) {
  int z = tx[nx];
  z = z*x/4;
  tx[nx++] = z;
}

void set(int cx, int range) {
  cxt[ncxt++] = base+cx;
  base += range;
}

int p() {
  while( nx&7 )
    tx[nx++] = 0;
  if( S>1 ) {
    sub.update2();
    for( int i = 0; i<ncxt; ++i ) {
      int dp = dot_product(&tx[0], &wx[cxt[i]*N], nx);
      dp = (dp*9)>>9;
      pr[i] = squash(dp);
      sub.add(dp);
    }
    return sub.p();
  } else {
    int z = dot_product(&tx[0], &wx[0], nx);
    base = squash((z*15)>>13);
    return squash(z>>9);
  }
}
};

// the one mixer the models are handed
typedef Mixer<456, 128*(16+14+14+12+14+16), 6, 512> MainMixer;

template <int n> struct APM {
int index;

U16 t[n*33];

APM() : index(0) {
  for( int j = 0; j<33; ++j )
    t[j] = squash((j-16)*128)*16;
  for( int i = 33; i<n*33; ++i )
    t[i] = t[i-33];
}

int p(int pr = 2048, int cxt = 0, int rate = 8) {
  pr = stretch(pr);
  int g = (y<<16)+(y<<rate)-y*2;
  t[index] += (g-t[index])>>rate;
  t[index+1] += (g-t[index+1])>>rate;
  const int w = pr&127;
  index = ((pr+2048)>>7)+cxt*33;
  return (t[index]*(128-w)+t[index+1]*w)>>11;
}
};

struct StateMap {
int cxt;
U16 t[256];

StateMap() : cxt(0) {
  for( int i = 0; i<256; ++i ) {
    int n0 = nex(i, 2);
    int n1 = nex(i, 3);
    if( n0==0 )
      n1 *= 128;
    if( n1==0 )
      n0 *= 128;
    t[i] = 65536*(n1+1)/(n0+n1+2);
  }
}

int p(int cx) {
  int q = t[cxt];
  t[cxt] = q+((sm_add_y-q)>>sm_shft);
  return t[cxt = cx]>>4;
}
};

inline U32 hash(U32 a, U32 b, U32 c = 0xffffffff) {
  U32 h = a*110002499u+b*30005491u+c*50004239u;
  return h^h>>9^a>>3^b>>3^c>>4;
}

template <int B, int I> struct BH {
enum { M = 7 };

// PAD is not slack for its own sake.  operator[] indexes buckets [0,I-1] but
// then walks up to M-1 further slots from &t[i*B], so the last bucket reads
// p[2] and memmoves up to 23 bytes PAST t[I*B].  The baseline's
// Array<U8,64>(I*B) had that land in untouched heap behind the block; with the
// table as a member it would land on the next member instead, so the same
// amount of never-read zero space has to be here explicitly.  Measured: 31
// such accesses on book1[0:131072] at level 4.
enum { PAD = 64 };
alignas(64) U8 t[I*B+PAD];
static const U32 n = I-1;

BH() {
  memset(t, 0, sizeof(t));
}

U8*operator[](U32 i) {
  int chk = (i>>16^i)&0xffff;
  i = i*M&n;
  U8* p = &t[i*B]-B;
  int j;
  for( j = 0; j<M; ++j ) {
    p += B;
    if( p[2]==0 ) {
      *(U16*)p = chk;
      break;
    }
    if( *(U16*)p==chk )
      break;
  }
  if( j==0 )
    return p;
  if( j==M ) {
    --j;
    if( p[2]>p[-2] )
      --j;
  } else
    chk = *(int*)p;
  p = &t[i*4];
  memmove(p+4, p, j*4);
  *(int*)p = chk;
  return p;
}
};

inline int mix2(MainMixer &m, int s, StateMap &sm) {
  int p1 = sm.p(s);
  int n0 = -!nex(s, 2);
  int n1 = -!nex(s, 3);
  int st = stretch(p1);
  if( cxtfl ) {
    m.add(st/4);
    int p0 = 4095-p1;
    m.add((p1-p0)*3/64);
    m.add(st*(n1-n0)*3/16);
    m.add(((p1&n0)-(p0&n1))/16);
    m.add(((p0&n0)-(p1&n1))*7/64);
    return s>0;
  }
  m.add(st*9/32);
  m.add(st*(n1-n0)*3/16);
  int p0 = 4095-p1;
  m.add(((p1&n0)-(p0&n1))/16);
  m.add(((p0&n0)-(p1&n1))*7/64);
  return s>0;
}

template <int MSZ, int mulc> struct RunContextMap {
BH<4, MSZ/4> t;
U8* cp;

RunContextMap() {
  cp = t[0]+2;
}

void set(U32 cx) {
  if( cp[0]==0||cp[1]!=b1 )
    cp[0] = 1, cp[1] = b1;
  else if( cp[0]<255 )
    ++cp[0];
  cp = t[cx]+2;
}

int p() {
  if( (cp[1]+256)>>(8-bpos)==c0 )
    return (((cp[1]>>(7-bpos))&1)*2-1)*ilog(cp[0]+1)*mulc;
  else
    return 0;
}

int mix(MainMixer &m) {
  m.add(p());
  return cp[0]!=0;
}
};

template <int MSZ, int mulc> struct SmallStationaryContextMap {
U16 t[MSZ/2];
int cxt;
U16* cp;

SmallStationaryContextMap() : cxt(0) {
  for( U32 i = 0; i<U32(MSZ/2); ++i )
    t[i] = 32768;
  cp = &t[0];
}

void set(U32 cx) {
  cxt = (cx*256)&(U32(MSZ/2)-256);
}

void mix(MainMixer &m) {
  if( pos<4000000 )
    *cp += ((y<<16)-(*cp)+(1<<8))>>9;
  else
    *cp += ((y<<16)-(*cp)+(1<<9))>>10;
  cp = &t[cxt+c0];
  m.add(stretch(*cp>>4)*mulc/32);
}
};

template <U32 MSZ, int NC> struct ContextMap {
enum { C = NC };
static const int Sz = int((MSZ>>6)-1);

struct E {
U16 chk[7];
U8 last;

U8 bh[7][7];

U8*get(U16 ch, int j) {
  ch += j;
  if( chk[last&15]==ch )
    return &bh[last&15][0];
  int b = 0xffff, bi = 0;
  for( int i = 0; i<7; ++i ) {
    if( chk[i]==ch )
      return last = last<<4|i, &bh[i][0];
    int pri = bh[i][0];
    if( (last&15)!=i&&last>>4!=i&&pri<b )
      b = pri, bi = i;
  }
  return last = 0xf0|bi, chk[bi] = ch, (U8*)memset(&bh[bi][0], 0, 7);
}
};

alignas(64) E t[MSZ>>6];
U8* cp[NC];
U8* cp0[NC];
U32 cxt[NC];
U8* runp[NC];
StateMap sm[NC];
int cn;

int mix1(MainMixer &m, int cc, int c1, int y1) {

  int result = 0;
  for( int i = 0; i<cn; ++i ) {
    U8* cpi = cp[i];
    if( cpi ) {
      int ns = nex(*cpi, y1);
      if( ns>=204&&(rnd()<<((452-ns)>>3)) )
        ns -= 4;
      *cpi = ns;
    }

    if( bpos>1&&runp[i][0]==0 )
      cpi = 0;
    else if( bpos==1||bpos==3||bpos==6 )
      cpi = cp0[i]+1+(cc&1);
    else if( bpos==4||bpos==7 )
      cpi = cp0[i]+3+(cc&3);
    else {
      cp0[i] = cpi = t[(cxt[i]+cc)&Sz].get(cxt[i]>>16, i);

      if( bpos==0 ) {
        if( cpi[3]==2 ) {
          const int c = cpi[4]+256;
          U8* p = t[(cxt[i]+(c>>6))&Sz].get(cxt[i]>>16, i);
          p[0] = 1+((c>>5)&1);
          p[p[0]] = 1+((c>>4)&1);
          p[3+((c>>4)&3)] = 1+((c>>3)&1);
          p = t[(cxt[i]+(c>>3))&Sz].get(cxt[i]>>16, i);
          p[0] = 1+((c>>2)&1);
          p[p[0]] = 1+((c>>1)&1);
          p[3+((c>>1)&3)] = 1+(c&1);
          cpi[6] = 0;
        }

        U8 c0 = runp[i][0];

        if( c0==0 )
          c0 = 2, runp[i][1] = c1;
        else if( runp[i][1]!=c1 )
          c0 = 1, runp[i][1] = c1;
        else if( c0<254 )
          c0 += 2;
        runp[i][0] = c0;
        runp[i] = cpi+3;
      }
    }

    int rc = runp[i][0];
    if( (runp[i][1]+256)>>(8-bpos)==cc ) {
      int b = ((runp[i][1]>>(7-bpos))&1)*2-1;
      int c = ilog(rc+1);
      if( rc&1 )
        c = (c*15)/4;
      else
        c *= 13;
      m.add(b*c);
    } else
      m.add(0);

    result += mix2(m, cpi ? *cpi : 0, sm[i]);
    cp[i] = cpi;
  }
  if( bpos==7 )
    cn = 0;
  return result;
}

ContextMap() : cn(0) {
  memset(t, 0, sizeof(t));
  for( int i = 0; i<C; ++i ) {
    cxt[i] = 0;
    cp0[i] = cp[i] = &t[0].bh[0][0];
    runp[i] = cp[i]+3;
  }
}

void set(U32 cx) {
  int i = cn++;
  cx = cx*123456791+i;
  cx = cx<<16|cx>>16;
  cxt[i] = cx*987654323+i;
}

int mix(MainMixer &m) {
  return mix1(m, c0, b1, y);
}
};

static U32 col, frstchar = 0, spafdo = 0, spaces = 0, spacecount = 0, words = 0, wordcount = 0, fails = 0, failz = 0, failcount = 0;

template <int L> struct WordModel {
U32 word0, word1, word2, word3, word4;
ContextMap<((U32)MEM(L)*16), 46> cm;
int nl1, nl;
U32 t1[256];
U16 t2[0x10000];

WordModel() : word0(0), word1(0), word2(0), word3(0), word4(0), nl1(-3), nl(-2) {
  memset(t1, 0, sizeof(t1));
  memset(t2, 0, sizeof(t2));
}

void mix(MainMixer &m) {  if( bpos==0 ) {
    U32 c = b1, f = 0;

    if( spaces&0x80000000 )
      --spacecount;
    if( words&0x80000000 )
      --wordcount;
    spaces = spaces*2;
    words = words*2;

    if( (c-'a')<=('z'-'a')||c==8||c==6||(c>127&&b2!=12) ) {
      ++words, ++wordcount;
      word0 = word0*263*8+c;
    } else {
      if( c==32||c==10 ) {
        ++spaces, ++spacecount;
        if( c==10 )
          nl1 = nl, nl = pos-1;
      }
      if( word0 ) {
        word4 = word3*43;
        word3 = word2*47;
        word2 = word1*53;
        word1 = word0*83;
        word0 = 0;
        if( c=='.'||c=='O'||c==('}'-'{'+'P') )
          f = 1, spafdo = 0;
        else {
          ++spafdo;
          spafdo = min(63, spafdo);
        }
      }
    }

    U32 h = word0*271+c;
    cm.set(word0);
    cm.set(h+word1);
    cm.set(word0*91+word1*89);
    cm.set(h+word1*79+word2*71);

    cm.set(h+word2);
    cm.set(h+word3);
    cm.set(h+word4);
    cm.set(h+word1*73+word3*61);
    cm.set(h+word2*67+word3*59);

    if( f ) {
      word4 = word3*31;
      word3 = word2*37;
      word2 = word1*41;
      word1 = '.';
    }

    cm.set(b3|b4<<8);
    cm.set(spafdo*8*((w4&3)==1));

    col = min(31, pos-nl);
    if( col<=2 ) {
      if( col==2 )
        frstchar = min(c, 96);
      else
        frstchar = 0;
    }
    if( frstchar=='['&&c==32 ) {
      if( b3==']'||b4==']' )
        frstchar = 96;
    }
    cm.set(frstchar<<11|c);

    int above = buf[nl1+col];

    cm.set(col<<16|c<<8|above);
    cm.set(col<<8|c);
    cm.set(col*(c==32));

    h = wordcount*64+spacecount;
    cm.set(spaces&0x7fff);
    cm.set(frstchar<<7);
    cm.set(spaces&0xff);
    cm.set(c*64+spacecount/2);
    cm.set((c<<13)+h);
    cm.set(h);

    U32 d = c4&0xffff;
    h = w4<<6;
    cm.set(c+(h&0xffffff00));
    cm.set(c+(h&0x00ffff00));
    cm.set(c+(h&0x0000ff00));
    h <<= 6;
    cm.set(d+(h&0xffff0000));
    cm.set(d+(h&0x00ff0000));
    h <<= 6, f = c4&0xffffff;
    cm.set(f+(h&0xff000000));

    U16 &r2 = t2[f>>8];
    r2 = r2<<8|c;
    U32 &r1 = t1[d>>8];
    r1 = r1<<8|c;
    U32 t = c|t1[c]<<8;
    cm.set(t&0xffff);
    cm.set(t&0xffffff);
    cm.set(t);
    cm.set(t&0xff00);
    t = d|t2[d]<<16;
    cm.set(t&0xffffff);
    cm.set(t);

    cm.set(x4&0x00ff00ff);
    cm.set(x4&0xff0000ff);
    cm.set(x4&0x00ffff00);
    cm.set(c4&0xff00ff00);
    cm.set(c+b5*256+(1<<17));
    cm.set(c+b6*256+(2<<17));
    cm.set(b4+b8*256+(4<<17));

    cm.set(d);
    cm.set(w4&15);
    cm.set(f4);
    cm.set((w4&63)*128+(5<<17));
    cm.set(d<<9|frstchar);
    cm.set((f4&0xffff)<<11|frstchar);
  }
  cm.mix(m);
}
};

// every size here is a literal, so this one needs no level parameter
struct RecordModel {
int cpos1[256];
int wpos1[0x10000];
ContextMap<32768/4, 2> cm;
ContextMap<32768/2, 5> cn;
ContextMap<32768, 4> co;
ContextMap<32768*2, 3> cp;
ContextMap<32768*4, 3> cq;

RecordModel() {
  memset(cpos1, 0, sizeof(cpos1));
  memset(wpos1, 0, sizeof(wpos1));
}

void mix(MainMixer &m) {
  if( !bpos ) {
    int c = b1, w = (b2<<8)+c, d = w&0xf0ff, e = c4&0xffffff;
    cm.set(c<<8|(min(255, pos-cpos1[c])/4));
    cm.set(w<<9|llog(pos-wpos1[w])>>2);
    cn.set(w);
    cn.set(d<<8);
    cn.set(c<<16);
    cn.set((f4&0xffff)<<3);
    int col = pos&3;
    cn.set(col|2<<12);

    co.set(c);
    co.set(w<<8);
    co.set(w5&0x3ffff);
    co.set(e<<3);

    cp.set(d);
    cp.set(c<<8);
    cp.set(w<<16);

    cq.set(w<<3);
    cq.set(c<<19);
    cq.set(e);

    cpos1[c] = pos;
    wpos1[w] = pos;
  }
  co.mix(m);
  cp.mix(m);
  cxtfl = 0;
  cm.mix(m);
  cn.mix(m);
  cq.mix(m);
  cxtfl = 3;
}
};

template <int L> struct SparseModel {
ContextMap<((U32)(MEM(L)*2)), 5> cn;
SmallStationaryContextMap<0x20000, 17> scm1;
SmallStationaryContextMap<0x20000, 12> scm2;
SmallStationaryContextMap<0x20000, 12> scm3;
SmallStationaryContextMap<0x20000, 13> scm4;
SmallStationaryContextMap<0x10000, 12> scm5;
SmallStationaryContextMap<0x20000, 12> scm6;
SmallStationaryContextMap<0x2000, 12> scm7;
SmallStationaryContextMap<0x8000, 13> scm8;
SmallStationaryContextMap<0x1000, 12> scm9;
SmallStationaryContextMap<0x10000, 16> scma;

void mix(MainMixer &m) {
  if( bpos==0 ) {
    cn.set(words&0x1ffff);
    cn.set((f4&0x000fffff)*7);
    cn.set((x4&0xf8f8f8f8)+3);
    cn.set((tt&0x00000fff)*9);
    cn.set((x4&0x80f0f0ff)+6);
    scm1.set(b1);
    scm2.set(b2);
    scm3.set(b3);
    scm4.set(b4);
    scm5.set(words&127);
    scm6.set((words&12)*16+(w4&12)*4+(b1>>4));
    scm7.set(w4&15);
    scm8.set(spafdo*((w4&3)==1));
    scm9.set(col*(b1==32));
    scma.set(frstchar);
  }
  cn.mix(m);
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scm7.mix(m);
  scm8.mix(m);
  scm9.mix(m);
  scma.mix(m);
}
};

int primes[] = {0, 257, 251, 241, 239, 233, 229, 227, 223, 211, 199, 197, 193, 191};
static U32 WRT_mpw[16] = {3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, tri[4] = {0, 4, 3, 7}, trj[4] = {0, 6, 6, 12};
static U32 WRT_mtt[16] = {0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7};

template <int L> struct ContextModel {
ContextMap<((U32)MEM(L)*16), 7> cm;
RunContextMap<(int)(MEM(L)/4), 14> rcm7;
RunContextMap<(int)(MEM(L)/4), 18> rcm9;
RunContextMap<(int)(MEM(L)/2), 20> rcm10;
MainMixer m;
U32 cxt[16];
int size;

WordModel<L> wordModel;
SparseModel<L> sparseModel;
RecordModel recordModel;

ContextModel() : size(0) {
  memset(cxt, 0, sizeof(cxt));
}

int p() {  if( bpos==0 ) {
    --size;
    if( size==-5 ) {
      size = c4;
    }
  }

  m.update();
  m.add(64);

  if( bpos==0 ) {
    int i = 0, f2 = buf(2);

    if( f2=='.'||f2=='O'||f2=='M'||f2=='!'||f2==')'||f2==('}'-'{'+'P') ) {
      if( b1!=(unsigned int)f2&&buf(3)!=f2 )
        i = 13, x4 = x4*256+f2;
    }

    for(; i>0; --i )
      cxt[i] = cxt[i-1]*primes[i];

    for( i = 13; i>0; --i )
      cxt[i] = cxt[i-1]*primes[i]+b1;

    cm.set(cxt[3]);
    cm.set(cxt[4]);
    cm.set(cxt[5]);
    cm.set(cxt[6]);
    cm.set(cxt[8]);
    cm.set(cxt[13]);
    cm.set(0);

    rcm7.set(cxt[7]);
    rcm9.set(cxt[9]);
    rcm10.set(cxt[11]);

    x4 = x4*256+b1;
  }
  rcm7.mix(m);
  rcm9.mix(m);
  rcm10.mix(m);
  int qq = m.nx;
  order = cm.mix(m)-1;
  if( order<0 )
    order = 0;
  int zz = (m.nx-qq)/7;

  m.nx = qq+zz*3;
  for( qq = zz*2; qq!=0; --qq )
    m.mul(5);
  for( qq = zz; qq!=0; --qq )
    m.mul(6);
  for( qq = zz; qq!=0; --qq )
    m.mul(9);

  if( L>=4 ) {
    wordModel.mix(m);
    sparseModel.mix(m);
    recordModel.mix(m);
  }

  U32 c1 = b1, c2 = b2, c;
  if( c1==9||c1==10||c1==32 )
    c1 = 16;
  if( c2==9||c2==10||c2==32 )
    c2 = 16;

  m.set(256*order+(w4&240)+(c2>>4), 256*7);

  c = (words>>1)&63;
  m.set((w4&3)*64+c+order*256, 256*7);

  c = (w4&255)+256*bpos;
  m.set(c, 256*8);

  if( bpos ) {
    c = c0<<(8-bpos);
    if( bpos==1 )
      c += b3/2;
    c = (min(bpos, 5))*256+(tt&63)+(c&192);
  } else
    c = (words&12)*16+(tt&63);
  m.set(c, 1536);

  c = bpos;
  c2 = (c0<<(8-bpos))|(c1>>bpos);
  m.set(order*256+c+(c2&248), 256*7);

  c = c*256+((c0<<(8-bpos))&255);
  c1 = (words<<bpos)&255;
  m.set(c+(c1>>bpos), 2048);

  return m.p();
}
};

template <int L> struct Predictor {
int pr;

ContextModel<L> contextModel;

APM<256> a1;
APM<0x8000> a2;
APM<0x8000> a3;
APM<0x20000> a4;
APM<0x10000> a5;
APM<0x10000> a6;

Predictor() : pr(2048) {
}

int p() const {
  return pr;
}

// was PAQ8HP::Perceive
void Perceive(int bit) {
  y = bit;
  sm_add_y = bit ? sm_add : 0;
  update();
}

void update() {
  c0 += c0+y;
  if( c0>=256 ) {
    buf[pos++] = c0;
    c0 -= 256;
    if( pos<=1024*1024 ) {
      if( pos==1024*1024 )
        sm_shft = 9, sm_add = 65535+511;
      if( pos==512*1024 )
        sm_shft = 8, sm_add = 65535+255;
      sm_add_y = sm_add&(-y);
    }
    int i = WRT_mpw[c0>>4];
    w4 = w4*4+i;
    if( b1==12 )
      i = 2;
    w5 = w5*4+i;
    b8 = b7, b7 = b6, b6 = b5, b5 = b4, b4 = b3;
    b3 = b2;
    b2 = b1;
    b1 = c0;
    if( c0=='.'||c0=='O'||c0=='M'||c0=='!'||c0==')'||c0==('}'-'{'+'P') ) {
      w5 = (w5<<8)|0x3ff, x5 = (x5<<8)+c0, f4 = (f4&0xfffffff0)+2;
      if( c0!='!'&&c0!='O' )
        w4 |= 12;
      if( c0!='!' )
        b2 = '.', tt = (tt&0xfffffff8)+1;
    }
    c4 = (c4<<8)+c0;
    x5 = (x5<<8)+c0;
    if( c0==32 )
      --c0;
    f4 = f4*16+(c0>>4);
    tt = tt*8+WRT_mtt[c0>>4];
    c0 = 1;
  }
  bpos = (bpos+1)&7;

  if( fails&0x00000080 )
    --failcount;
  fails = fails*2;
  failz = failz*2;
  if( y )
    pr ^= 4095;
  if( pr>=1820 )
    ++fails, ++failcount;
  if( pr>=848 )
    ++failz;

  pr = contextModel.p();

  int rate = 6+(pos>14*256*1024)+(pos>28*512*1024);
  int pt, pu = (a1.p(pr, c0, 3)+7*pr+4)>>3, pv, pz = failcount+1;
  pz += tri[(fails>>5)&3];
  pz += trj[(fails>>3)&3];
  pz += trj[(fails>>1)&3];
  if( fails&1 )
    pz += 8;
  pz = pz/2;

  pu = a4.p(pu, (c0*2)^(hash(b1, (x5>>8)&255, (x5>>16)&0x80ff)&0x1ffff), rate);
  pv = a2.p(pr, (c0*8)^(hash(29, failz&2047)&0x7fff), rate+1);
  pv = a5.p(pv, hash(c0, w5&0xfffff)&0xffff, rate);
  pt = a3.p(pr, (c0*32)^(hash(19, x5&0x80ffff)&0x7fff), rate);
  pz = a6.p(pu, (c0*4)^(hash(min(9, pz), x5&0x80ff)&0xffff), rate);

  if( fails&255 )
    pr = (pt*6+pu+pv*11+pz*14+16)>>5;
  else
    pr = (pt*4+pu*5+pv*12+pz*11+16)>>5;
}
};
} // namespace paq8hp
