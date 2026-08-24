/* rawadpcm.cpp -- find and extract raw MS/IMA ADPCM streams inside arbitrary data.
 *
 *   rawadpcm c input output prefix     split input into non-ADPCM remainder + .wav files
 *   rawadpcm d output restore prefix   put it back together, byte for byte
 *
 * The point is to reach ADPCM that has no RIFF header in front of it -- audio
 * banks, game archives, resource forks, a wav whose header lives somewhere else
 * entirely.  So nothing here reads a header: the block geometry is recovered
 * from the data, and the .wav files this writes are synthesised afterwards, for
 * the benefit of whatever consumes them (xadpcm, say).  A stream may be MS or
 * IMA, mono or stereo, 2- to 5-bit, and one input may hold any mixture of
 * those: every hypothesis is tried at every position and the reading that pays
 * most wins the bytes.
 *
 * What counts as a hit is a region an ADPCM coder would compress better than a
 * general-purpose one.  Both sides of that are estimated from models this file
 * already runs -- see accept_region, which is where the decision is -- and
 * getting there takes two steps, structure first and entropy second, which is
 * the shape lpsort.cpp uses.
 *
 *   1. ADPCM blocks carry a header every `block_align` bytes, and those headers
 *      have fields that cannot hold arbitrary bytes: IMA has a reserved byte
 *      that is zero and a step index that is at most 88; MS has a coefficient
 *      index below 7 and an iDelta of at least 16 that has to read as a
 *      positive int16.  A position where that holds is a *signature*, and a
 *      chain of them at a fixed stride says "blocks of this size start here" --
 *      so the stride is read off the data rather than guessed, and the stride
 *      confirmed by the most headers wins.  How long a chain has to be before
 *      it means anything is computed per file from the signature's measured
 *      density, because that density is nothing like what random bytes would
 *      give: inside real ADPCM the IMA signature holds at 15% of all positions,
 *      since a zero byte is a common thing for coded audio to contain.
 *
 *   2. Structure still says nothing about whether the bytes BETWEEN the headers
 *      are audio.  For that the candidate is decoded and its codes measured,
 *      the way lpsort measures its three interleavings: the reconstructed
 *      signal drives an NLMS predictor, the prediction is pushed back through
 *      the encoder's own quantizer to give a predicted code q^, and the real
 *      code is coded against q^ by a bit tree over the nzcc FSM counters, with
 *      log2LUT turning probabilities into code lengths.  Real ADPCM comes out
 *      well under its raw width -- 4-bit codes land near 3 bits.  Anything else
 *      lands at 4, because a prediction made from noise predicts nothing.
 *
 * The per-byte code length also places the ends of a region: the running total
 * of (raw width - modelled length) is a gain curve, and the region is the
 * prefix that maximises it.  Blocks of audio push it up, the first bytes past
 * the end push it down, so the argmax sits on the boundary -- including a
 * partial final block, which is where a data chunk usually stops.  On wavs3
 * four of the seven regions come out byte-exact and the rest within 45 bytes.
 *
 * build:  sh mkraw.sh    (or: g++ -std=gnu++17 -O2 rawadpcm.cpp -o rawadpcm)
 * test:   sh traw.sh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <vector>
#include <string>
#include <algorithm>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef unsigned int uint;
typedef unsigned long long qword;

#define DIM(x) (sizeof(x)/sizeof((x)[0]))

template<class T> static inline T tmin( T a, T b ) { return a<b ? a : b; }
template<class T> static inline T tmax( T a, T b ) { return a>b ? a : b; }

/*---------------------------------------------------------------- log2LUT --*/
/* lpsort's table, unchanged: LOG2(a)-LOG2(b) is log2(b/a) in 1/65536 bits, so
   a probability p/SCALE costs LOG2(SCALE)-LOG2(p). */

struct log2LUT {

  enum {
    LUTsize = 32768+1,
    Precision = 16
  };

  uint LUT[LUTsize];

  uint operator() ( uint i ) const {
    return LUT[i];
  }

  constexpr uint Calc( uint i ) const {
    uint k=0;
    qword w = i;
    for( uint j=0; j<Precision; j++ ) {
      w = w * w;
      k = k + k;
      while( w>=(1ULL<<32) ) w=(w+1)>>1, k++;
    }
    return k;
  }

  void Init( void ) {
    for( uint i=0; i<LUTsize; i++ ) LUT[i] = Calc(i);
  }

};

static log2LUT LOG2;

/*------------------------------------------------------------ counter FSM --*/

enum { SCALElog=15, SCALE=1<<SCALElog };

/* The counter FSM: 256 states, each {next state on a 0 bit, next state on a 1
   bit, P(bit==0) scaled to 1<<15}.  This is the nonstationary counter table
   lpsort.cpp reads out of nzcc.txt, kept here as a table so the tool is one
   file with nothing to find at run time.  Every next-state in it is below 256,
   so the machine never leaves what is written. */
static const u16 NZCC[][3] = {
  {  1,  2,16696}, {  3,  5,26434}, {  4,  6, 6521}, {  7, 10,28428},
  {  8, 12,19548}, {  9, 13,14501}, { 11, 14, 4157}, { 15, 19,29533},
  { 16, 23,22495}, { 17, 24,21699}, { 18, 25,16509}, { 20, 27,17524},
  { 21, 28,11928}, { 22, 29,11084}, { 26, 30, 2922}, { 31, 33,30243},
  { 32, 35,24302}, { 32, 35,23935}, { 32, 35,23139}, { 32, 35,17871},
  { 34, 37,20828}, { 34, 37,19306}, { 34, 37,19735}, { 34, 37,13916},
  { 34, 37,15104}, { 34, 37,12983}, { 36, 39,15572}, { 36, 39,10154},
  { 36, 39, 9455}, { 36, 39, 8875}, { 38, 40, 2249}, { 41, 43,30718},
  { 42, 45,25164}, { 42, 45,18777}, { 44, 47,21762}, { 44, 47,15357},
  { 46, 49,18531}, { 46, 49,11896}, { 48, 51,14878}, { 48, 51, 8019},
  { 50, 52, 1720}, { 53, 43,31014}, { 54, 57,26199}, { 54, 57,19885},
  { 56, 59,23190}, { 56, 59,18756}, { 58, 61,20268}, { 58, 61,13477},
  { 60, 63,14985}, { 60, 63, 9962}, { 62, 65,13324}, { 62, 65, 6918},
  { 50, 66, 1407}, { 67, 55,31307}, { 68, 57,26816}, { 68, 57,21260},
  { 70, 73,24247}, { 70, 73,19792}, { 72, 75,21558}, { 72, 75,16815},
  { 74, 77,17100}, { 74, 77,12006}, { 76, 79,13769}, { 76, 79, 8706},
  { 62, 81,12299}, { 62, 81, 6078}, { 64, 82, 1212}, { 83, 69,31493},
  { 84, 71,28501}, { 84, 71,23655}, { 86, 73,24994}, { 86, 73,20318},
  { 44, 89,21515}, { 44, 89,17546}, { 88, 91,19554}, { 88, 91,14116},
  { 90, 49,16274}, { 90, 49,11599}, { 76, 93,12756}, { 76, 93, 7784},
  { 78, 95, 7441}, { 78, 95, 3746}, { 80, 96, 1032}, { 97, 69,31624},
  { 98, 87,28872}, { 98, 87,22147}, {100, 45,25870}, {100, 45,21418},
  { 72, 75,20678}, { 72, 75,15890}, { 74, 77,18083}, { 74, 77,12700},
  { 48,103,11554}, { 48,103, 6778}, { 92,105,10145}, { 92,105, 3312},
  { 80,106,  908}, {107, 69,31761}, {108, 87,29168}, {108, 87,23395},
  {110, 57,26449}, {110, 57,21814}, { 62,113,11022}, { 62,113, 6067},
  { 92,115, 9499}, { 92,115, 2999}, { 80,116,  805}, {117, 85,31855},
  {118, 87,29404}, {118, 87,23741}, {120, 57,26976}, {120, 57,22118},
  { 62,123,10463}, { 62,123, 5462}, { 92,125, 8826}, { 92,125, 2776},
  { 94,126,  701}, {127, 85,31930}, {128,101,29601}, {128,101,24027},
  {130, 57,27421}, {130, 57,22445}, { 62,133, 9904}, { 62,133, 4942},
  {102,135, 7943}, {102,135, 2596}, { 94,136,  688}, {137, 85,32020},
  {138,101,29770}, {138,101,24709}, {140, 57,27816}, {140, 57,22756},
  { 62,143, 9302}, { 62,143, 4495}, {102,145, 7935}, {102,145, 2436},
  { 94,146,  602}, {147, 99,32095}, {148,101,29928}, {148,101,25922},
  { 68, 57,28181}, { 68, 57,23045}, { 62, 81, 8726}, { 62, 81, 4092},
  {102,149, 6382}, {102,149, 2301}, {104,150,  554}, {151, 99,32128},
  {152,111,30071}, {112,153, 2174}, {104,154,  500}, {155, 99,32191},
  {156,111,30198}, {112,157, 2063}, {104,158,  483}, {159, 99,32224},
  {160,111,30319}, {112,161, 1970}, {104,162,  455}, {163,109,32258},
  {164,111,30426}, {112,165, 1883}, {114,166,  419}, {167,109,32291},
  {168,121,30535}, {122,169, 1800}, {114,170,  398}, {171,109,32298},
  {172,121,30622}, {122,173, 1733}, {114,174,  372}, {175,109,32296},
  {176,121,30714}, {122,177, 1663}, {114,178,  340}, {179,109,32350},
  {180,121,30785}, {122,181, 1604}, {114,182,  329}, {183,119,32379},
  {184,121,30864}, {122,185, 1548}, {124,186,  314}, {187,119,32419},
  {188,131,30937}, {132,189, 1495}, {124,190,  292}, {191,119,32423},
  {192,131,31008}, {132,193, 1443}, {124,194,  298}, {195,119,32422},
  {196,131,31064}, {132,197, 1401}, {124,198,  274}, {199,119,32453},
  {200,131,31134}, {132,201, 1355}, {124,202,  283}, {203,119,32478},
  {204,131,31188}, {132,205, 1320}, {124,206,  264}, {207,119,32482},
  {208,131,31236}, {132,209, 1273}, {124,210,  221}, {211,129,32499},
  {212,131,31286}, {132,213, 1246}, {134,214,  246}, {215,129,32510},
  {216,141,31329}, {142,217, 1217}, {134,218,  221}, {219,129,32523},
  {220,141,31378}, {142,221, 1181}, {134,222,  230}, {223,129,32537},
  {224,141,31417}, {142,225, 1150}, {134,226,  213}, {227,129,32547},
  {228,141,31459}, {142,229, 1127}, {134,230,  194}, {231,129,32527},
  {232,141,31493}, {142,233, 1099}, {134,234,  194}, {235,129,32532},
  {236,141,31518}, {142,237, 1073}, {134,238,  190}, {239,129,32561},
  {240,141,31564}, {142,241, 1043}, {134,242,  187}, {243,129,32571},
  {244,141,31592}, {142,245, 1024}, {134,246,  195}, {247,139,32572},
  {248,141,31620}, {142,249, 1001}, {144,250,  157}, {251,139,32572},
  {252, 69,31647}, { 80,253,  984}, {144,254,  159}, {251,139,32766},
  {255, 69,31678}, { 80,253,  272}, {144,254,    2}, {255, 69,32515},
};

enum { N_STATES = sizeof(NZCC)/sizeof(NZCC[0]) };

struct fsm {
  u16 s[2];      // next state after bits 0,1
  uint cc[2];    // what bits 0,1 cost there, in 1/65536 bits
};

static fsm FSM[N_STATES];

static void init_fsm() {
  LOG2.Init();
  for( uint i=0; i<N_STATES; i++ ) {
    uint P = NZCC[i][2];
    P = P<1 ? 1 : P>SCALE-1 ? SCALE-1 : P;
    FSM[i].s[0] = NZCC[i][0]<N_STATES ? NZCC[i][0] : 0;
    FSM[i].s[1] = NZCC[i][1]<N_STATES ? NZCC[i][1] : 0;
    FSM[i].cc[0] = LOG2(SCALE)-LOG2(P);
    FSM[i].cc[1] = LOG2(SCALE)-LOG2(SCALE-P);
  }
}

/*-------------------------------------------------------- entropy meters ---*/
/* Both are lpsort's log2ent with the counter state widened and a context added:
   they run a bit tree over FSM counters and return what the symbol cost. */

struct Order1 {           // generic byte model, order 1 -- the "what a normal
  u16 C[256][256];        // compressor would do with these bytes" reference
  int prev;
  void Init() { memset(C,0,sizeof(C)); prev = 0; }
  uint Update( uint c ) {
    uint acc=0, z=1;
    u16* R = C[prev];
    for( int k=7; k>=0; k-- ) {
      uint bit = (c>>k)&1;
      const fsm& F = FSM[R[z]];
      acc += F.cc[bit];
      R[z] = F.s[bit];
      z += z + bit;
    }
    prev = int(c);
    return acc;
  }
};

enum { SYM_XB = 8, SYM_CTX = 32*2*8*SYM_XB, SYM_TREE = 32 };

/* Coarse view of a neighbouring code: sign, and how far from zero.  This is
   what carries the stereo correlation -- in MS stereo a byte holds one nibble
   from each channel at the SAME instant, which is exactly why an order-1 byte
   model does so well on it (4.6 bits/byte, better than this model manages
   without the context).  Reading the other channel's last code puts that same
   information in front of the code model.  In mono the slot holds the channel's
   own previous code, which is worth having for its own reasons. */
static inline uint xbucket( uint sym, int bps ) {
  uint sign = sym>>(bps-1), mag = sym&((1u<<(bps-1))-1);
  uint m = mag==0 ? 0 : mag==1 ? 1 : mag<4 ? 2 : 3;
  return sign*4+m;
}

struct SymModel {         // ADPCM code model: bit tree over (q^, conf, scale, neighbour)
  u16 C[SYM_CTX][SYM_TREE];
  void Init() { memset(C,0,sizeof(C)); }
  uint Update( uint ctx, uint sym, int nbits ) {
    uint acc=0, z=1;
    u16* R = C[ctx];
    for( int k=nbits-1; k>=0; k-- ) {
      uint bit = (sym>>k)&1;
      const fsm& F = FSM[R[z]];
      acc += F.cc[bit];
      R[z] = F.s[bit];
      z += z + bit;
    }
    return acc;
  }
};

static Order1   O1;
static SymModel SYM;

/*------------------------------------------------------------- IMA ADPCM ---*/

static const u16 step_table[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};

static const signed char ima_t2[2]  = {-1,2};
static const signed char ima_t3[4]  = {-1,-1,1,2};
static const signed char ima_t4[8]  = {-1,-1,-1,-1,2,4,6,8};
static const signed char ima_t5[16] = {-1,-1,-1,-1,-1,-1,-1,-1,1,2,4,6,8,10,13,16};

static inline int index_update( int index, int code, int bps ) {
  switch( bps ) {
    case 2:  index += ima_t2[code&1]; break;
    case 3:  index += ima_t3[code&3]; break;
    case 5:  index += ima_t5[code&15]; break;
    default: index += ima_t4[code&7]; break;
  }
  return index<0 ? 0 : index>88 ? 88 : index;
}

static inline int code_magnitude( int code, int step, int bps ) {
  const int nb = bps-1;
  int mag = step>>nb;
  for( int i=0; i<nb; i++ ) if( code&(1<<(nb-1-i)) ) mag += step>>i;
  return mag;
}

static inline i32 ima_apply( i32 pcm, int code, int step, int bps ) {
  int mag = code_magnitude(code,step,bps);
  pcm += (code&(1<<(bps-1))) ? -mag : mag;
  return pcm<-32768 ? -32768 : pcm>32767 ? 32767 : pcm;
}

static inline int ima_quantize( i32 diff, int step, int bps, int& leftover ) {
  const int nb = bps-1;
  int code = 0;
  i32 a = diff<0 ? -diff : diff;
  for( int i=0; i<nb; i++ ) {
    i32 w = step>>i;
    if( a>=w ) { code |= 1<<(nb-1-i); a -= w; }
  }
  leftover = int(a>0x7FFF ? 0x7FFF : a);
  return code|(diff<0 ? 1<<nb : 0);
}

/*-------------------------------------------------------------- MS ADPCM ---*/

static const short ms_adapt[16] = {230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230};
static const short ms_std_coef[7][2] = {{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};
static const i32 MS_DELTA_MAX = 2796202;

static inline i32 clip16( i64 v ) { return i32(v<-32768 ? -32768 : v>32767 ? 32767 : v); }

static inline i32 ms_predict( i32 s1, i32 s2, i32 c1, i32 c2 ) {
  i64 x = i64(s1)*i64(c1)+i64(s2)*i64(c2);
  return i32((x+((x>>63)&255))>>8);
}

static inline i32 ms_apply( i32 pred, int nib, i32 delta ) {
  return clip16(i64(pred)+i64((nib&8) ? nib-16 : nib)*i64(delta));
}

static inline i32 ms_delta_update( i32 delta, int nib ) {
  i32 d = i32((i64(ms_adapt[nib])*i64(delta))>>8);
  return d<16 ? 16 : d>MS_DELTA_MAX ? MS_DELTA_MAX : d;
}

// round half away from zero into a 4-bit signed cell; err is the signed leftover
static inline int ms_quantize( i64 diff, i32 delta, i64& err ) {
  i64 d = delta<1 ? 1 : delta, q;
  if( diff>=0 ) q =  (2*diff+d)/(2*d);
  else          q = -((-2*diff+d)/(2*d));
  if( q<-8 ) q = -8;
  if( q> 7 ) q =  7;
  err = diff-q*i64(d);
  return int(q)&15;
}

static inline int ms_dlog( i32 delta ) {
  u32 d = delta<16 ? 16u : u32(delta);
  int e = 31-__builtin_clz(d);
  return e<4 ? 0 : e>21 ? 17 : e-4;
}

// sign-magnitude view of the two's-complement nibble: what the bit tree splits on
static u8 MS_M[16];
static void init_ms_map() {
  for( int i=0; i<16; i++ ) {
    int v = i<8 ? i : i-16;
    MS_M[i] = u8(v>=0 ? v : 8+(-v-1));
  }
}

/*------------------------------------------------------------------ NLMS ---*/
/* One per channel, over the reconstructed signal.  Integer NLMS, initialised to
   linear extrapolation so it predicts something useful from the first sample. */

struct NLMS {
  enum { N = 24, WSH = 16, MUSH = 3 };
  i32 w[N], h[N];
  i64 energy;

  void Init() {
    memset(w,0,sizeof(w));
    memset(h,0,sizeof(h));
    w[0] = 2<<WSH;
    w[1] = -(1<<WSH);
    energy = 1<<16;
  }

  i32 predict() const {
    i64 s = 0;
    for( int i=0; i<N; i++ ) s += i64(w[i])*i64(h[i]);
    s >>= WSH;
    return clip16(s);
  }

  void update( i32 x, i32 p ) {
    i64 e = i64(x)-i64(p);
    i64 t = (e<<(WSH+16))/(energy+1);
    for( int i=0; i<N; i++ ) {
      i64 dw = (t*i64(h[i]))>>(16+MUSH);
      if( dw> (1<<14) ) dw =  (1<<14);
      if( dw< -(1<<14) ) dw = -(1<<14);
      i64 nw = i64(w[i])+dw;
      if( nw> (1<<24) ) nw =  (1<<24);
      if( nw< -(1<<24) ) nw = -(1<<24);
      w[i] = i32(nw);
    }
    energy += i64(x)*i64(x)-i64(h[N-1])*i64(h[N-1]);
    if( energy<(1<<16) ) energy = 1<<16;
    for( int i=N-1; i>0; i-- ) h[i] = h[i-1];
    h[0] = x;
  }
};

/*----------------------------------------------------------- block layout --*/

enum { FMT_IMA=0, FMT_MS=1 };
enum { MAXCH = 8 };

struct Hyp {
  int fmt, nch, bps, ba;
  int hdr() const { return fmt==FMT_MS ? 7*nch : 4*nch; }
};

static inline int rd_i16( const u8* p ) { return i16(u16(p[0])|(u16(p[1])<<8)); }

static inline u32 get_bits( const u8* b, size_t bitpos, int nbits ) {
  u32 v = u32(b[bitpos>>3])>>(bitpos&7);
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) { v |= u32(b[i++])<<have; have += 8; }
  return v&((1u<<nbits)-1);
}

/* The signature: fields of a block header that cannot hold arbitrary bytes. */
static inline bool sig_at( const u8* d, size_t n, size_t p, int fmt, int nch ) {
  if( fmt==FMT_IMA ) {
    if( p+size_t(4*nch)>n ) return false;
    for( int k=0; k<nch; k++ ) {
      if( d[p+4*k+3]!=0 ) return false;     // reserved
      if( d[p+4*k+2]>88 ) return false;     // step index
    }
    return true;
  }
  if( p+size_t(7*nch)>n ) return false;
  for( int k=0; k<nch; k++ ) if( d[p+k]>=7 ) return false;             // coefficient set
  for( int k=0; k<nch; k++ ) {
    int dl = rd_i16(d+p+nch+2*k);
    if( dl<16 ) return false;                                          // iDelta
  }
  return true;
}

/* One bit per position, built once per hypothesis.  The stride search below
   walks every plausible block_align at every candidate start, which is only
   affordable because a signature test is a bit test here. */
struct SigMap {
  std::vector<u8> b;
  size_t n, pop;
  int hdr;
  void build( const u8* d, size_t sz, int fmt, int nch ) {
    n = sz;
    pop = 0;
    hdr = fmt==FMT_MS ? 7*nch : 4*nch;
    b.assign(sz/8+2, 0);
    for( size_t i=0; i+size_t(hdr)<=sz; i++ )
      if( sig_at(d,sz,i,fmt,nch) ) { b[i>>3] |= u8(1u<<(i&7)); pop++; }
  }
  double density() const { return n ? double(pop)/double(n) : 0.0; }
  inline bool operator()( size_t i ) const {
    return i+size_t(hdr)<=n && ((b[i>>3]>>(i&7))&1);
  }
};

/* Capped: L only ranks the strides against each other, and anything that
   reaches the cap is already past every explanation but "this is the grid".
   run_region walks the real extent itself, stopping at the first header that
   does not line up, so nothing is lost by not counting further. */
enum { CHAIN_CAP = 1024 };

static size_t chain_len( const SigMap& S, size_t ba, size_t start, size_t stop ) {
  size_t L = 0, p = start;
  while( p<stop && S(p) ) { L++; p += ba; if( L>=CHAIN_CAP ) break; }
  return L;
}

/*------------------------------------------------------- region scoring ----*/

struct Ch {
  i32 pcm; int index;                 // IMA
  i32 s1, s2, c1, c2, delta;          // MS
  NLMS pd;
};

struct RegionResult {
  size_t start, end;
  i64 gain;          // sum over bytes of (raw width - modelled length), 1/65536 bits
  i64 model_bits;    // what the ADPCM model spent, 1/65536 bits
  size_t nsym;
};

// per-byte hysteresis, so a stretch that models at exactly its raw width does
// not drift the argmax past the real end of the audio
static i64 EPS_PER_BYTE = 3277;   // 0.05 bit

static std::vector<i64> g_bgain;
static std::vector<u8>  g_cbuf[MAXCH];

static u64 STAT_runs, STAT_runbytes;
static bool run_region( const u8* d, size_t n, const Hyp& hy, size_t start, size_t limit, RegionResult& out ) {
  STAT_runs++; STAT_runbytes += (limit>start? limit-start:0);
  const int nch = hy.nch, bps = hy.bps, fmt = hy.fmt;
  const size_t ba = size_t(hy.ba);
  const int hdr = hy.hdr();
  if( start+size_t(hdr)>limit ) return false;
  if( limit>n ) limit = n;

  SYM.Init();
  u8 xsym[MAXCH];
  memset(xsym,0,sizeof(xsym));
  Ch ch[MAXCH];
  for( int k=0; k<nch; k++ ) {
    memset(&ch[k],0,sizeof(Ch));
    ch[k].pd.Init();
    ch[k].delta = 16;
    ch[k].c1 = ms_std_coef[0][0];
    ch[k].c2 = ms_std_coef[0][1];
  }

  if( g_bgain.size()<ba ) g_bgain.resize(ba);
  for( int k=0; k<nch; k++ ) if( g_cbuf[k].size()<ba+8 ) g_cbuf[k].resize(ba+8);

  i64 gain = 0, best = 0, mbits = 0, bestm = 0;
  size_t bestend = start, nsym = 0, bestsym = 0;
  size_t pos = start;
  const i64 BAILOUT = (i64(8)<<16)*8192;   // give up once 8 KB of gain is thrown away

  while( pos+size_t(hdr)<=limit ) {
    size_t len = tmin<size_t>(ba, limit-pos);
    if( !sig_at(d,n,pos,fmt,nch) ) break;
    i64* bg = &g_bgain[0];
    memset(bg,0,len*sizeof(i64));
    const u8* blk = d+pos;

    if( fmt==FMT_IMA ) {
      for( int k=0; k<nch; k++ ) {
        Ch& c = ch[k];
        i32 v = i32(rd_i16(blk+k*4));
        int ix = blk[k*4+2];
        i32 p = c.pd.predict();
        c.pcm = v;
        c.index = ix>88 ? 88 : ix;
        c.pd.update(v,p);
      }
      size_t body = len-size_t(hdr), groups = body/size_t(hdr);
      size_t cbytes = groups*4, ncode = cbytes*8/size_t(bps);
      for( int k=0; k<nch; k++ ) {
        u8* cb = &g_cbuf[k][0];
        for( size_t j=0; j<cbytes; j++ )
          cb[j] = blk[size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3)];
      }
      for( size_t i=0; i<ncode; i++ ) {
        size_t bitpos = i*size_t(bps);
        size_t j = bitpos>>3;
        for( int k=0; k<nch; k++ ) {
          Ch& c = ch[k];
          int code = int(get_bits(&g_cbuf[k][0], bitpos, bps));
          int step = step_table[c.index];
          i32 rhat = c.pd.predict();
          int leftover;
          int qhat = ima_quantize(rhat-c.pcm, step, bps, leftover);
          int base = step>>(bps-1);
          int conf = (2*leftover>=2*base+1);
          int bucket = tmin(7, c.index>>4);
          uint ctx = uint(((qhat*2+conf)*8+bucket)*SYM_XB+int(xbucket(uint(xsym[nch>1 ? (k?0:nch-1) : 0]),bps)));
          uint cost = SYM.Update(ctx, uint(code), bps);
          xsym[k] = u8(code);
          size_t off = size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3);
          if( off<len ) bg[off] += (i64(bps)<<16)-i64(cost);
          mbits += cost; nsym++;
          c.pcm = ima_apply(c.pcm, code, step, bps);
          c.index = index_update(c.index, code, bps);
          c.pd.update(c.pcm, rhat);
        }
      }
    } else {
      for( int k=0; k<nch; k++ ) {
        Ch& c = ch[k];
        int ci = blk[k];
        if( ci>=7 ) ci = 0;
        c.c1 = ms_std_coef[ci][0];
        c.c2 = ms_std_coef[ci][1];
      }
      for( int k=0; k<nch; k++ ) ch[k].delta = i32(rd_i16(blk+nch+2*k));
      for( int k=0; k<nch; k++ ) {   // iSamp2 -- first in time
        Ch& c = ch[k];
        i32 v = i32(rd_i16(blk+5*nch+2*k));
        i32 p = c.pd.predict();
        c.s2 = v;
        c.pd.update(v,p);
      }
      for( int k=0; k<nch; k++ ) {   // iSamp1 -- second
        Ch& c = ch[k];
        i32 v = i32(rd_i16(blk+3*nch+2*k));
        i32 p = c.pd.predict();
        c.s1 = v;
        c.pd.update(v,p);
      }
      size_t body = len-size_t(hdr), nn = body*2, ngrp = nn/size_t(nch);
      for( size_t i=0; i<ngrp; i++ )
        for( int k=0; k<nch; k++ ) {
          Ch& c = ch[k];
          size_t at = i*size_t(nch)+size_t(k);
          size_t off = size_t(hdr)+(at>>1);
          int nib = (at&1) ? (blk[off]&15) : (blk[off]>>4);
          i32 P = ms_predict(c.s1,c.s2,c.c1,c.c2);
          i32 rhat = c.pd.predict();
          i64 err;
          int qhat = ms_quantize(i64(rhat)-i64(P), c.delta, err);
          int conf = (err>=0);
          int bucket = tmin(7, ms_dlog(c.delta)>>1);
          uint ctx = uint(((MS_M[qhat]*2+conf)*8+bucket)*SYM_XB+int(xbucket(uint(xsym[nch>1 ? (k?0:nch-1) : 0]),4)));
          uint cost = SYM.Update(ctx, MS_M[nib], 4);
          xsym[k] = MS_M[nib];
          if( off<len ) bg[off] += (i64(4)<<16)-i64(cost);
          mbits += cost; nsym++;
          i32 v = ms_apply(P, nib, c.delta);
          c.s2 = c.s1; c.s1 = v;
          c.delta = ms_delta_update(c.delta, nib);
          c.pd.update(v, rhat);
        }
    }

    for( size_t j=0; j<len; j++ ) {
      gain += bg[j]-EPS_PER_BYTE;
      if( gain>best ) { best = gain; bestend = pos+j+1; bestm = mbits; bestsym = nsym; }
    }

    pos += len;
    if( gain<best-BAILOUT ) break;
  }

  out.start = start;
  out.end = bestend;
  out.gain = best;
  out.model_bits = bestm;
  out.nsym = bestsym;
  return bestend>start;
}

/*----------------------------------------------------------------- crc32 ---*/

static u32 crc_tab[256];
static void crc_init() {
  for( u32 i=0; i<256; i++ ) {
    u32 c = i;
    for( int k=0; k<8; k++ ) c = (c&1) ? 0xEDB88320u^(c>>1) : (c>>1);
    crc_tab[i] = c;
  }
}
static u32 crc32( const u8* p, size_t n, u32 c=0 ) {
  c = ~c;
  for( size_t i=0; i<n; i++ ) c = crc_tab[(c^p[i])&0xFF]^(c>>8);
  return ~c;
}

/*------------------------------------------------------------- container ---*/

struct Seg {
  size_t start, end;
  int fmt, nch, bps, ba;
  i64 gain;
  i64 model_bits;
  u64 generic_bits;
};

static void put_varint( std::vector<u8>& v, u64 x ) {
  while( x>=0x80 ) { v.push_back(u8(x)|0x80); x >>= 7; }
  v.push_back(u8(x));
}

static bool get_varint( const u8* d, size_t n, size_t& p, u64& x ) {
  x = 0; int sh = 0;
  for( ;; ) {
    if( p>=n || sh>63 ) return false;
    u8 c = d[p++];
    x |= u64(c&0x7F)<<sh;
    if( !(c&0x80) ) return true;
    sh += 7;
  }
}

static const char RAW_MAGIC[4] = {'R','W','A','1'};

/*------------------------------------------------------------ .wav output --*/

static void wr16( u8* p, u32 v ) { p[0]=u8(v); p[1]=u8(v>>8); }
static void wr32( u8* p, u32 v ) { p[0]=u8(v); p[1]=u8(v>>8); p[2]=u8(v>>16); p[3]=u8(v>>24); }

static int spb_of( const Hyp& hy ) {
  if( hy.fmt==FMT_MS ) return (hy.ba-7*hy.nch)*2/hy.nch+2;
  return (hy.ba-hy.nch*4)/hy.nch*8/hy.bps+1;
}

/* The canonical layouts xadpcm regenerates instead of storing, so an extracted
   wav costs a flag there rather than 60-90 literal bytes. */
static size_t build_head( u8* h, const Hyp& hy, u32 rate, size_t dsize ) {
  int spb = spb_of(hy);
  u32 nblk = u32((dsize+size_t(hy.ba)-1)/size_t(hy.ba));
  u32 nsamples = nblk*u32(spb>0?spb:0);
  if( hy.fmt==FMT_MS ) {
    const int ncoef = 7;
    u32 fmtsz = 22+4*u32(ncoef);
    int at = 40;
    memcpy(h,"RIFF",4);
    wr32(h+4, u32(62+4*ncoef-8)+u32(dsize));
    memcpy(h+8,"WAVE",4);
    memcpy(h+12,"fmt ",4);
    wr32(h+16, fmtsz);
    wr16(h+20, 0x02);
    wr16(h+22, u32(hy.nch));
    wr32(h+24, rate);
    wr32(h+28, spb>0 ? rate*u32(hy.ba)/u32(spb) : 0);
    wr16(h+32, u32(hy.ba));
    wr16(h+34, 4);
    wr16(h+36, fmtsz-18);
    wr16(h+38, u32(spb));
    wr16(h+at, u32(ncoef)); at += 2;
    for( int i=0; i<ncoef; i++, at += 4 ) {
      wr16(h+at,   u32(ms_std_coef[i][0])&0xFFFF);
      wr16(h+at+2, u32(ms_std_coef[i][1])&0xFFFF);
    }
    memcpy(h+at,"fact",4);
    wr32(h+at+4, 4);
    wr32(h+at+8, nsamples);
    memcpy(h+at+12,"data",4);
    wr32(h+at+16, u32(dsize));
    return size_t(at+20);
  }
  memcpy(h,"RIFF",4);
  wr32(h+4, 52+u32(dsize));
  memcpy(h+8,"WAVE",4);
  memcpy(h+12,"fmt ",4);
  wr32(h+16, 20);
  wr16(h+20, 0x11);
  wr16(h+22, u32(hy.nch));
  wr32(h+24, rate);
  wr32(h+28, spb>0 ? rate*u32(hy.ba)/u32(spb) : 0);
  wr16(h+32, u32(hy.ba));
  wr16(h+34, u32(hy.bps));
  wr16(h+36, 2);
  wr16(h+38, u32(spb));
  memcpy(h+40,"fact",4);
  wr32(h+44, 4);
  wr32(h+48, nsamples);
  memcpy(h+52,"data",4);
  wr32(h+56, u32(dsize));
  return 60;
}

static void seg_name( std::string& s, const char* prefix, size_t i ) {
  char buf[32];
  snprintf(buf,sizeof(buf),"%03u.wav",unsigned(i));
  s = std::string(prefix)+buf;
}

/*----------------------------------------------------------------- files ---*/

static u8* load_file( const char* nam, size_t& n ) {
  FILE* f = fopen(nam,"rb"); if( !f ) return 0;
  fseek(f,0,SEEK_END); long L = ftell(f); fseek(f,0,SEEK_SET);
  if( L<0 ) { fclose(f); return 0; }
  n = size_t(L);
  u8* p = (u8*)malloc(n?n:1);
  if( !p ) { fclose(f); return 0; }
  if( n && fread(p,1,n,f)!=n ) { fclose(f); free(p); return 0; }
  fclose(f);
  return p;
}

/*--------------------------------------------------------------- encoder ---*/

struct Options {
  u32 rate = 44100;
  int verbose = 0;
  double min_gain = 0.75;   // bits/byte the code model has to beat the raw width by
  double adpcm_scale = 0.8; // what this model reads high by, against the real thing
  double min_dense = 3.0;   // bits/byte order-1 must still be spending on it
  size_t min_len = 4096;    // bytes -- below this the structural evidence is thin either way
  size_t min_blocks = 4;
  int all_widths = 1;       // try IMA code widths 2,3,5 as well as 4
  size_t max_ba = 4096;
  size_t lookahead = 64;   // bytes to keep comparing past the first anchor that works
  double probe_bits = 5.0;   // a 4 KB window this dense is what makes an anchor worth decoding
};

static Options OPT;

static u64 STAT_anchors=0, STAT_cand=0, STAT_chain=0;

// what an order-1 byte model spends on [a,b), in 1/65536 bits
static inline u64 generic_bits( const u64* gpfx, size_t a, size_t b ) { return gpfx[b]-gpfx[a]; }

static bool probe_ok( const u64* gpfx, size_t n, size_t i ) {
  const size_t CH = 4096, SPAN = 16;
  for( size_t j=0; j<SPAN; j++ ) {
    size_t a = i+j*CH;
    if( a>=n ) break;
    size_t b = tmin<size_t>(n, a+CH);
    if( b-a<256 ) break;
    if( double(generic_bits(gpfx,a,b))/65536.0/double(b-a)>=OPT.probe_bits ) return true;
  }
  return false;
}

/* Two tests, and they are asking different questions.

   The first is "is this ADPCM": the codes have to model below their own raw
   width, which a prediction of the decoded signal can only buy if the decoded
   signal is real.  Random bytes decode to a random walk that predicts nothing
   and score zero here, however well they might do on the second test.

   The second is "is it worth taking out", and that is the real question: a
   region belongs in a wav only if an ADPCM coder would beat a general-purpose
   one on it.  Both sides are estimated from models already running:

     xz         <- the order-1 byte model.  Measured against xz -9e on the seven
                   regions of wavs3 it lands within 0.15 bits/byte every time
                   (4.56 vs 4.51, 5.45 vs 5.32, 5.17 vs 5.38, ...), so it is
                   used as-is.
     xadpcm     <- this file's code model, scaled.  It is the same idea as
                   xadpcm's -- predict the signal, quantize the prediction,
                   code the real code against it -- but with one context where
                   xadpcm mixes eleven, and one NLMS where it cascades four, so
                   it reads systematically high: 4.21 where xadpcm gets 3.76,
                   5.09 where xadpcm gets 3.84.  The scale factor is what that
                   difference is worth, and it is a factor rather than an offset
                   on purpose -- an offset would make a stretch of digital
                   silence come out NEGATIVE and get extracted, and silence is
                   the one thing that must not be, since a run of equal bytes
                   compresses to nothing whether or not anyone knows it is
                   audio.  A factor leaves cheap material cheap on both sides.

   The scale factor is also why the second test carries a floor on what order-1
   is spending.  The factor stands for machinery that only pays off on real
   audio, so crediting it to material that is not audio over-values ADPCM there
   -- and an executable's padding is exactly that trap: it decodes to a nearly
   flat line, so it beats its raw code width by 5.4 bits/byte, more than any
   real region in wavs3 manages.  What gives it away is that order-1 already
   has it at 2.1 bits/byte.  Nothing a plain byte model already holds that
   cheaply needs an ADPCM coder.

   Between them the tests place every case in the test material: real ADPCM
   passes; digital silence beats its raw width and fails the comparison; an
   executable's padding beats its raw width and fails the floor; random bytes
   fail the width test itself. */
static bool accept_region( const u64* gpfx, const RegionResult& r, const Hyp& hy, Seg& out ) {
  size_t len = r.end-r.start;
  if( len<OPT.min_len ) return false;
  double gpb = double(r.gain)/65536.0/double(len);
  if( gpb<OPT.min_gain ) return false;
  u64 gen = generic_bits(gpfx, r.start, r.end);
  double dens = double(gen)/65536.0/double(len);
  if( dens<OPT.min_dense ) return false;
  if( double(r.model_bits)*OPT.adpcm_scale >= double(gen) ) return false;
  out.start = r.start; out.end = r.end;
  out.fmt = hy.fmt; out.nch = hy.nch; out.bps = hy.bps; out.ba = hy.ba;
  out.gain = r.gain; out.model_bits = r.model_bits; out.generic_bits = gen;
  return true;
}

/* One hypothesis, prepared once: its signature map and the chain length that
   makes a stride worth decoding on THIS file. */
struct HypDesc {
  int fmt, nch;
  SigMap sm;
  size_t conf, minba, step;
  const char* name() const { return fmt==FMT_MS ? "MS " : "IMA"; }
};

static void prepare( HypDesc& H, const u8* d, size_t n, int fmt, int nch ) {
  H.fmt = fmt; H.nch = nch;
  H.step = (fmt==FMT_IMA) ? size_t(4*nch) : 2;   // IMA blocks are whole groups; MS block_align is even
  size_t mb = fmt==FMT_MS ? size_t(7*nch+4) : size_t(8*nch);
  H.minba = ((mb+H.step-1)/H.step)*H.step;
  H.sm.build(d,n,fmt,nch);

  /* How many block headers have to line up before a stride is worth decoding.
     A signature is not rare -- inside real ADPCM the IMA one holds at 15% of
     all positions, because a zero byte is a common thing for coded audio to
     contain -- so the bar has to come from the density that is actually there
     rather than from what random bytes would give.  Ask for a chain whose
     expected count over every (start, stride) pair in this file is under
     1/100, which is where "these headers line up by accident" stops being an
     explanation. */
  H.conf = OPT.min_blocks;
  double p = H.sm.density();
  size_t nba = (OPT.max_ba>H.minba) ? (OPT.max_ba-H.minba)/H.step+1 : 1;
  if( p>1e-9 && p<0.999 ) {
    double pairs = double(H.sm.pop?H.sm.pop:1)*double(nba);
    double need = (log(0.01)-log(pairs))/log(p);
    if( need>0 ) H.conf = tmax<size_t>(H.conf, size_t(need)+2);
  }
  H.conf = tmin<size_t>(H.conf, 64);
}

/* Try one hypothesis at one anchor.  Returns the best region it can defend
   there, or false. */
static bool try_anchor( const u8* d, size_t n, const u64* gpfx, const HypDesc& H,
                        size_t i, size_t stop, Seg& best ) {
  const int fmt = H.fmt, nch = H.nch;
  const int hdr = fmt==FMT_MS ? 7*nch : 4*nch;
  const size_t step = H.step, minba = H.minba, conf = H.conf;
  const SigMap& SM = H.sm;

  /* Every stride that puts a signature under the next `conf` block headers.
     Real audio also matches 2*ba, 3*ba and every other multiple, and all of
     those reach just as far into the file as the true stride does -- so the
     ranking is by how many headers actually lined up, not by how far the chain
     reached.  ba=36 confirms 2222 of them where ba=3816 confirms 21, and the
     shortest stride breaks the ties. */
  struct Cand { size_t ba, L; };
  static std::vector<Cand> cands;
  cands.clear();
  for( size_t ba=minba; ba<=OPT.max_ba && i+ba*(conf-1)+size_t(hdr)<=stop; ba+=step ) {
    bool ok = true;
    for( size_t r=1; r<conf; r++ ) if( !SM(i+r*ba) ) { ok = false; break; }
    if( !ok ) continue;
    size_t L = chain_len(SM,ba,i,stop);
    STAT_chain += L; STAT_cand++;
    if( L<conf ) continue;
    if( tmin<size_t>(stop-i, L*ba)<OPT.min_len ) continue;
    Cand cc; cc.ba = ba; cc.L = L;
    cands.push_back(cc);
  }
  if( cands.empty() ) return false;
  std::sort(cands.begin(),cands.end(),[]( const Cand& a, const Cand& b ){
    if( a.L!=b.L ) return a.L>b.L;
    return a.ba<b.ba;
  });

  bool have = false; i64 bestgain = 0;
  size_t ntry = tmin<size_t>(cands.size(), 4);
  if( OPT.verbose>1 ) {
    fprintf(stderr,"    %s%d anchor %llu: %llu cands, top:",H.name(),nch,(unsigned long long)i,(unsigned long long)cands.size());
    for( size_t t=0; t<ntry; t++ ) fprintf(stderr," ba=%llu L=%llu",(unsigned long long)cands[t].ba,(unsigned long long)cands[t].L);
    fprintf(stderr,"\n");
  }
  for( size_t t=0; t<ntry; t++ ) {
    size_t ba = cands[t].ba;
    size_t limit = (cands[t].L>=size_t(CHAIN_CAP)) ? stop : tmin<size_t>(stop, i+cands[t].L*ba);

    /* Is there anything here worth taking out?  Free, from the prefix sum, and
       it has to be asked over the whole candidate span rather than over the
       screening window: a wav may open with several blocks of digital silence
       -- the second MS file in wavs3 opens with eleven -- and a window that
       lands entirely inside them says "nothing to gain" about a region that is
       almost all music. */
    {
      double dens = double(generic_bits(gpfx,i,limit))/65536.0/double(limit-i);
      if( dens<OPT.min_dense*0.5 ) continue;
    }

    /* Screen on a short prefix before paying for the whole run.  The screen
       applies the same two tests the region has to pass, at half strength: it
       is there to throw out a stride that is merely a coarse multiple of a real
       one, and to throw out silence, both of which would otherwise cost a full
       decode. */
    size_t screen = tmin<size_t>(limit, i+tmax<size_t>(4*ba, 4096));
    int widths[4] = {4,2,3,5};
    int nw = (fmt==FMT_MS) ? 1 : (OPT.all_widths ? 4 : 1);
    /* The screen bar is deliberately a low fixed number rather than a fraction
       of the real one: it sees only the first 4 KB, and a wav is allowed to
       open quietly.  Its job is to throw out geometry that is plainly wrong,
       not to make the accept decision -- that is what the full run is for. */
    const double bar = tmin(OPT.min_gain*0.5, 0.15);

    /* Code width.  4 bits is what nearly everything is, so it is screened
       first and the others only if it does not already look right.  What the
       screen may NOT do is pick the width outright: on a wav that opens quietly
       the first 4 KB rank the widths by noise, and one bad ranking costs the
       whole region (this is how a 251 KB IMA file came out three blocks short).
       So an alternative width has to look clearly better on the screen just to
       earn a full run, and the full runs decide between themselves.
       There is no fast path that skips the alternatives when 4 bits already
       looks good, and that is deliberate: 2-bit ADPCM read as 4-bit codes
       screens perfectly respectably -- packed codes keep some of their
       structure through the wrong window -- so any bar low enough to be worth
       having would take that reading and never ask. */
    double gs = 0;
    int runw[2]; int nr = 0;
    {
      Hyp hy{fmt,nch,4,int(ba)};
      RegionResult r;
      if( run_region(d,n,hy,i,screen,r) ) {
        gs = double(r.gain)/65536.0/double(r.end-r.start);
        if( gs>=bar ) runw[nr++] = 4;
      }
      {
        int alt = 0; double ag = gs+0.25;
        for( int wi=1; wi<nw; wi++ ) {
          Hyp h2{fmt,nch,widths[wi],int(ba)};
          RegionResult r2;
          if( !run_region(d,n,h2,i,screen,r2) ) continue;
          double g = double(r2.gain)/65536.0/double(r2.end-r2.start);
          if( g>=bar && g>ag ) { ag = g; alt = widths[wi]; }
        }
        if( alt ) runw[nr++] = alt;
      }
    }
    if( !nr ) continue;

    for( int wi=0; wi<nr; wi++ ) {
      Hyp hy{fmt,nch,runw[wi],int(ba)};
      RegionResult r;
      if( !run_region(d,n,hy,i,limit,r) ) continue;
      Seg s;
      bool ok = accept_region(gpfx,r,hy,s);
      if( OPT.verbose>1 ) {
        double L2 = double(r.end>r.start ? r.end-r.start : 1);
        fprintf(stderr,"      %s%d ba=%llu bps=%d -> [%llu,%llu) len=%.0f gain %.3f model %.3f gen %.3f  %s\n",
          H.name(),nch,(unsigned long long)ba,runw[wi],(unsigned long long)r.start,(unsigned long long)r.end,L2,
          double(r.gain)/65536.0/L2, double(r.model_bits)/65536.0/L2,
          double(generic_bits(gpfx,r.start,r.end))/65536.0/L2, ok?"accept":"reject");
      }
      if( !ok ) continue;
      if( !have || s.gain>bestgain ) { best = s; bestgain = s.gain; have = true; }
    }
  }
  return have;
}

/* One left-to-right pass with all four hypotheses competing at every position.
   Scanning them one at a time instead -- however the order is chosen -- lets a
   wrong reading claim a region before the right one is ever asked, and the
   wrong readings are not weak: an MS-mono signature holds all over IMA data on
   multiples of the IMA stride, and reads it at 1.1 bits/byte of gain where IMA
   reads it at 2.8.  Competing at a shared position settles that the only way it
   can be settled, by which reading pays more; and since a region's true start is
   the earliest position anything fires at, the left-to-right order settles the
   rest. */
static void scan_all( const u8* d, size_t n, const u64* gpfx, HypDesc* HD, int nh, std::vector<Seg>& found ) {
  size_t i = 0;
  while( i<n ) {
    bool fires = false;
    for( int h=0; h<nh; h++ ) if( HD[h].sm(i) ) { fires = true; break; }
    if( !fires ) { i++; continue; }
    STAT_anchors++;

    /* Cheap reject: material a plain order-1 model already handles is not what
       this tool is for.  It also keeps runs of zeros -- which satisfy every IMA
       signature there is, and decode to perfectly modelled silence -- from ever
       reaching the decoder.
       Read as the BEST 4 KB in the next 64 KB rather than as one average,
       because a wav is entitled to open with silence: both MS files in wavs3
       do, one of them for eight full blocks, and an averaged probe throws the
       start of the region away and picks it up wherever the music happens to
       begin. */
    if( !probe_ok(gpfx,n,i) ) { i++; continue; }

    Seg best; bool have = false;
    for( int h=0; h<nh; h++ ) {
      if( !HD[h].sm(i) ) continue;
      Seg s;
      if( !try_anchor(d,n,gpfx,HD[h],i,n,s) ) continue;
      if( !have || s.gain>best.gain ) { best = s; have = true; }
    }

    /* One more look before committing.  Anchors are compared left to right, and
       the earliest one is usually the region's true start -- but not always:
       an MS block header puts a zero where IMA's reserved byte goes and a small
       iDelta where its step index goes, so IMA fires one byte BEFORE every MS
       block, systematically.  Read that way a 365 KB MS file scores 0.8
       bits/byte where reading it as MS scores 2.5.  So look a little further
       along before settling, and let the reading that pays more win even if it
       starts a few bytes later. */
    if( have ) {
      size_t jend = tmin<size_t>(n, i+OPT.lookahead);
      for( size_t j=i+1; j<jend; j++ ) {
        bool f = false;
        for( int h=0; h<nh; h++ ) if( HD[h].sm(j) ) { f = true; break; }
        if( !f ) continue;
        if( !probe_ok(gpfx,n,j) ) continue;
        for( int h=0; h<nh; h++ ) {
          if( !HD[h].sm(j) ) continue;
          Seg s;
          if( !try_anchor(d,n,gpfx,HD[h],j,n,s) ) continue;
          if( s.gain>best.gain ) best = s;
        }
      }
    }

    if( have ) {
      if( OPT.verbose )
        fprintf(stderr,"  %s ch=%d bps=%d ba=%-5d [%llu,%llu) %llu bytes  %.3f bits/byte gain, %.2f vs order-1 %.2f\n",
                best.fmt==FMT_MS?"MS ":"IMA", best.nch, best.bps, best.ba,
                (unsigned long long)best.start,(unsigned long long)best.end,
                (unsigned long long)(best.end-best.start),
                double(best.gain)/65536.0/double(best.end-best.start),
                double(best.model_bits)/65536.0/double(best.end-best.start),
                double(best.generic_bits)/65536.0/double(best.end-best.start));
      found.push_back(best);
      i = best.end;
    } else {
      i++;
    }
  }
}

static int encode( const char* inp, const char* outp, const char* prefix ) {
  size_t n = 0;
  u8* d = load_file(inp,n);
  if( !d ) { fprintf(stderr,"error: cannot read '%s'\n",inp); return 2; }

  // the reference model, over the whole file, as a prefix sum
  u64* gpfx = (u64*)malloc(sizeof(u64)*(n+1));
  if( !gpfx ) { fprintf(stderr,"error: out of memory\n"); return 4; }
  O1.Init();
  gpfx[0] = 0;
  for( size_t i=0; i<n; i++ ) gpfx[i+1] = gpfx[i]+O1.Update(d[i]);
  if( OPT.verbose && n )
    fprintf(stderr,"order-1 over the whole input: %.3f bits/byte\n", double(gpfx[n])/65536.0/double(n));

  static const int hyps[4][2] = {{FMT_IMA,1},{FMT_IMA,2},{FMT_MS,1},{FMT_MS,2}};
  HypDesc HD[4];
  for( int h=0; h<4; h++ ) {
    prepare(HD[h],d,n,hyps[h][0],hyps[h][1]);
    if( OPT.verbose )
      fprintf(stderr,"%s %dch: signature density %.4f, %llu hits, asking for %llu aligned block headers\n",
              HD[h].fmt==FMT_MS?"MS ":"IMA", HD[h].nch, HD[h].sm.density(),
              (unsigned long long)HD[h].sm.pop, (unsigned long long)HD[h].conf);
  }

  std::vector<Seg> found;
  scan_all(d,n,gpfx,HD,4,found);
  if( OPT.verbose>1 ) fprintf(stderr,"anchors=%llu cands=%llu chainsteps=%llu runs=%llu runbytes=%llu\n",
    (unsigned long long)STAT_anchors,(unsigned long long)STAT_cand,(unsigned long long)STAT_chain,
    (unsigned long long)STAT_runs,(unsigned long long)STAT_runbytes);

  /* The scan emits regions left to right and jumps past each one it takes, so
     this is already sorted and disjoint.  Named for what the rest of the
     function reads out of it. */
  const std::vector<Seg>& keep = found;

  // ---- write the wavs
  for( size_t s=0; s<keep.size(); s++ ) {
    const Seg& S = keep[s];
    Hyp hy{S.fmt,S.nch,S.bps,S.ba};
    size_t len = S.end-S.start;
    u8 head[256];
    size_t hl = build_head(head,hy,OPT.rate,len);
    std::string nam; seg_name(nam,prefix,s);
    FILE* f = fopen(nam.c_str(),"wb");
    if( !f ) { fprintf(stderr,"error: cannot write '%s'\n",nam.c_str()); return 5; }
    fwrite(head,1,hl,f);
    fwrite(d+S.start,1,len,f);
    fclose(f);
    printf("%s: %s %dch %dbit ba=%d  %llu bytes at %llu\n", nam.c_str(),
           S.fmt==FMT_MS?"MS":"IMA", S.nch, S.bps, S.ba,
           (unsigned long long)len, (unsigned long long)S.start);
  }

  // ---- write the remainder
  std::vector<u8> hdr;
  hdr.insert(hdr.end(),RAW_MAGIC,RAW_MAGIC+4);
  put_varint(hdr,u64(n));
  u32 crc = crc32(d,n);
  put_varint(hdr,u64(crc));
  put_varint(hdr,u64(keep.size()));
  size_t at = 0;
  for( size_t s=0; s<keep.size(); s++ ) {
    const Seg& S = keep[s];
    put_varint(hdr,u64(S.start-at));
    put_varint(hdr,u64(S.end-S.start));
    hdr.push_back(u8(S.fmt));
    hdr.push_back(u8(S.nch));
    hdr.push_back(u8(S.bps));
    put_varint(hdr,u64(S.ba));
    at = S.end;
  }

  FILE* g = fopen(outp,"wb");
  if( !g ) { fprintf(stderr,"error: cannot write '%s'\n",outp); return 3; }
  fwrite(&hdr[0],1,hdr.size(),g);
  at = 0;
  size_t lit = 0;
  for( size_t s=0; s<keep.size(); s++ ) {
    const Seg& S = keep[s];
    if( S.start>at ) { fwrite(d+at,1,S.start-at,g); lit += S.start-at; }
    at = S.end;
  }
  if( n>at ) { fwrite(d+at,1,n-at,g); lit += n-at; }
  fclose(g);

  printf("%llu bytes in, %llu segments, %llu bytes extracted, %llu bytes left (+%llu table)\n",
         (unsigned long long)n, (unsigned long long)keep.size(),
         (unsigned long long)(n-lit), (unsigned long long)lit, (unsigned long long)hdr.size());
  free(gpfx);
  free(d);
  return 0;
}

/*--------------------------------------------------------------- decoder ---*/

// find the data chunk of a wav we wrote (or that came back from a round trip)
static bool wav_data( const u8* f, size_t n, size_t& off, size_t& size ) {
  if( n<12 || memcmp(f,"RIFF",4) || memcmp(f+8,"WAVE",4) ) return false;
  for( size_t pos=12; pos+8<=n; ) {
    u32 sz = u32(f[pos+4])|(u32(f[pos+5])<<8)|(u32(f[pos+6])<<16)|(u32(f[pos+7])<<24);
    size_t body = pos+8;
    if( !memcmp(f+pos,"data",4) ) {
      off = body;
      size = (body+sz<=n) ? sz : n-body;
      return true;
    }
    pos = body+sz+(sz&1);
  }
  return false;
}

static int decode( const char* outp, const char* restore, const char* prefix ) {
  size_t n = 0;
  u8* d = load_file(outp,n);
  if( !d ) { fprintf(stderr,"error: cannot read '%s'\n",outp); return 2; }
  if( n<4 || memcmp(d,RAW_MAGIC,4) ) { fprintf(stderr,"error: '%s' is not a rawadpcm container\n",outp); return 3; }
  size_t p = 4;
  u64 total=0, crc=0, nseg=0;
  if( !get_varint(d,n,p,total) || !get_varint(d,n,p,crc) || !get_varint(d,n,p,nseg) ) {
    fprintf(stderr,"error: truncated container header\n"); return 3;
  }
  std::vector<Seg> segs;
  for( u64 s=0; s<nseg; s++ ) {
    u64 gap=0, len=0, ba=0;
    if( !get_varint(d,n,p,gap) || !get_varint(d,n,p,len) ) { fprintf(stderr,"error: truncated segment table\n"); return 3; }
    if( p+3>n ) { fprintf(stderr,"error: truncated segment table\n"); return 3; }
    Seg S; S.fmt = d[p]; S.nch = d[p+1]; S.bps = d[p+2]; p += 3;
    if( !get_varint(d,n,p,ba) ) { fprintf(stderr,"error: truncated segment table\n"); return 3; }
    S.ba = int(ba);
    S.start = size_t(gap);   // reused: gap before this segment
    S.end = size_t(len);     // reused: segment length
    segs.push_back(S);
  }

  FILE* g = fopen(restore,"wb");
  if( !g ) { fprintf(stderr,"error: cannot write '%s'\n",restore); return 4; }

  size_t lit = p;   // where the literal bytes start
  u32 acc = 0;
  u64 written = 0;
  for( size_t s=0; s<segs.size(); s++ ) {
    size_t gap = segs[s].start, len = segs[s].end;
    if( lit+gap>n ) { fprintf(stderr,"error: container is short of literal data\n"); fclose(g); return 5; }
    if( gap ) { fwrite(d+lit,1,gap,g); acc = crc32(d+lit,gap,acc); written += gap; lit += gap; }
    std::string nam; seg_name(nam,prefix,s);
    size_t wn = 0;
    u8* w = load_file(nam.c_str(),wn);
    if( !w ) { fprintf(stderr,"error: cannot read '%s'\n",nam.c_str()); fclose(g); return 6; }
    size_t doff=0, dsz=0;
    if( !wav_data(w,wn,doff,dsz) ) { fprintf(stderr,"error: '%s' has no data chunk\n",nam.c_str()); fclose(g); free(w); return 6; }
    if( dsz<len ) { fprintf(stderr,"error: '%s' holds %llu bytes, the table wants %llu\n",nam.c_str(),
                            (unsigned long long)dsz,(unsigned long long)len); fclose(g); free(w); return 6; }
    fwrite(w+doff,1,len,g);
    acc = crc32(w+doff,len,acc);
    written += len;
    free(w);
  }
  if( n>lit ) { fwrite(d+lit,1,n-lit,g); acc = crc32(d+lit,n-lit,acc); written += n-lit; }
  fclose(g);

  if( written!=total )
    fprintf(stderr,"warning: restored %llu bytes, the table says %llu\n",(unsigned long long)written,(unsigned long long)total);
  if( acc!=u32(crc) ) {
    fprintf(stderr,"error: crc mismatch (%08X, want %08X)\n",acc,u32(crc));
    return 7;
  }
  printf("%llu bytes restored, crc %08X ok\n",(unsigned long long)written,acc);
  free(d);
  return 0;
}

/*------------------------------------------------------------------ main ---*/

static void usage() {
  fprintf(stderr,
    "rawadpcm -- raw MS/IMA ADPCM detector and extractor\n"
    "\n"
    "  rawadpcm c input output prefix [options]   split off the ADPCM\n"
    "  rawadpcm d output restore prefix           put it back\n"
    "\n"
    "options for c:\n"
    "  -v            report what was found and why\n"
    "  -r RATE       sample rate to stamp on the extracted wavs (default 44100)\n"
    "  -g BITS       gain over the raw code width a region must show (default 0.75)\n"
    "  -e BITS       order-1 cost a region must still carry to be worth extracting (default 3.0)\n"
    "  -m BYTES      shortest region to keep (default 4096)\n"
    "  -b N          shortest run of blocks to consider (default 4)\n"
    "  -4            only look for 4-bit IMA (default tries 2,3,4,5)\n"
    "  -B BYTES      largest block_align to consider (default 4096)\n"
    "  -p BITS       order-1 density a 4 KB window needs before an anchor is decoded (default 5.0)\n"
    "  -x SCALE      what to scale the code model by to estimate xadpcm (default 0.8)\n"
    "  -l BYTES      how far past a working anchor to keep looking for a better one (default 64)\n"
    );
}

int main( int argc, char** argv ) {
  if( argc<5 ) { usage(); return 1; }
  int mode = argv[1][0];
  if( mode!='c' && mode!='d' ) { usage(); return 1; }

  for( int i=5; i<argc; i++ ) {
    const char* a = argv[i];
    if( !strcmp(a,"-v") ) OPT.verbose = 1;
    else if( !strcmp(a,"-vv") ) OPT.verbose = 2;
    else if( !strcmp(a,"-4") ) OPT.all_widths = 0;
    else if( !strcmp(a,"-r") && i+1<argc ) OPT.rate = u32(atoi(argv[++i]));
    else if( !strcmp(a,"-g") && i+1<argc ) OPT.min_gain = atof(argv[++i]);
    else if( !strcmp(a,"-e") && i+1<argc ) OPT.min_dense = atof(argv[++i]);
    else if( !strcmp(a,"-m") && i+1<argc ) OPT.min_len = size_t(atoll(argv[++i]));
    else if( !strcmp(a,"-b") && i+1<argc ) OPT.min_blocks = size_t(atoll(argv[++i]));
    else if( !strcmp(a,"-B") && i+1<argc ) OPT.max_ba = size_t(atoll(argv[++i]));
    else if( !strcmp(a,"-p") && i+1<argc ) OPT.probe_bits = atof(argv[++i]);
    else if( !strcmp(a,"-x") && i+1<argc ) OPT.adpcm_scale = atof(argv[++i]);
    else if( !strcmp(a,"-l") && i+1<argc ) OPT.lookahead = size_t(atoll(argv[++i]));
    else { fprintf(stderr,"error: unknown option '%s'\n",a); return 1; }
  }

  crc_init();
  init_ms_map();
  init_fsm();

  if( mode=='c' ) return encode(argv[2],argv[3],argv[4]);
  return decode(argv[2],argv[3],argv[4]);
}
