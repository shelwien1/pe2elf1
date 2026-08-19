// bmppal -- the palette transform of PALETTE.md, as a tool.
//
//   bmppal c input.bmp out.bmp out.pal [out.frq]
//   bmppal d out.bmp out.pal [out.frq] restored.bmp
//
// It splits a bitmap into the three things PALETTE.md section 1 says it is made
// of, and hands each to whatever should code it:
//
//   out.bmp   the index image -- every pixel replaced by its palette index,
//             written as a plain BMP so an image coder can have it.  8bpp with a
//             grey ramp when the palette fits in a byte, which is what makes
//             `bmg` take its index path rather than expanding to colour planes.
//   out.pal   the palette, the ordering, and everything else needed to rebuild
//             the original file byte for byte -- range coded.
//   out.frq   the occurrence counts, uncompressed, little-endian uint32 per
//             palette entry, when a name is given for them.  Without a name
//             they go into out.pal instead, coded.
//
// `bmppal d` reproduces the input **byte for byte**, not merely pixel for pixel:
// header, palette table, row padding, run-length structure and trailing bytes
// included.  That is the whole reason the container record in out.pal exists.
//
// What it does not do is compress the index image; that is the coder's job, and
// the point of the split.  The measured effect of feeding `bmg` an index plane
// instead of colour planes is PALETTE.md section 6.4 -- much the same size, five
// times faster.

// Windows: MSVC's deprecation warnings, MinGW's C99 printf, 32-bit large files.
#define _CRT_SECURE_NO_WARNINGS 1
#define __USE_MINGW_ANSI_STDIO  1
#define _FILE_OFFSET_BITS       64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
#endif

static long long fsize_( FILE* f ) {
#if defined(_MSC_VER)
  if( _fseeki64( f, 0, SEEK_END ) ) return -1;
  long long L = _ftelli64( f );  _fseeki64( f, 0, SEEK_SET );
#elif defined(__MINGW32__)
  if( fseeko64( f, 0, SEEK_END ) ) return -1;
  long long L = ftello64( f );   fseeko64( f, 0, SEEK_SET );
#else
  if( fseeko( f, 0, SEEK_END ) ) return -1;
  long long L = ftello( f );     fseeko( f, 0, SEEK_SET );
#endif
  return L;
}

static byte* slurp( const char* fn, qword* len ) {
  FILE* f = fopen( fn, "rb" );
  if( !f ) { fprintf( stderr, "%s: cannot open\n", fn ); return 0; }
  long long L = fsize_( f );
  if( L < 0 || L > 0x7FFFFFFFll ) { fclose(f); fprintf(stderr,"%s: bad size\n",fn); return 0; }
  byte* d = new byte[ L ? (size_t)L : 1 ];
  if( fread( d, 1, (size_t)L, f ) != (size_t)L ) {
    fclose(f); delete[] d; fprintf(stderr,"%s: short read\n",fn); return 0;
  }
  fclose(f); *len = (qword)L; return d;
}

static int spew( const char* fn, const byte* d, qword n ) {
  FILE* f = fopen( fn, "wb" );
  if( !f ) { fprintf( stderr, "%s: cannot create\n", fn ); return 0; }
  int ok = fwrite( d, 1, (size_t)n, f ) == (size_t)n;
  fclose(f);
  if( !ok ) fprintf( stderr, "%s: short write\n", fn );
  return ok;
}

#include "bmg_rc.inc"

static Rangecoder rc;

// ---------------------------------------------------------------- models
//
// The counter is bcdr5's, the same one `bmg_ctr.inc` uses; it is copied rather
// than included because everything else in that file is the mixing layer, and a
// palette does not need one.  The rates are fixed here instead of being IDX
// knobs: this codes a few kilobytes, not a few megabytes, and there is nothing
// to tune against.

static const int WR = eSCALE/(160+16);
static const int MW = 24;

struct Counter {
  word P;
  void init( uint p0 = hSCALE ) { P = (word)p0; }
  void Update( int c ) {
    int dp = P + MW - SCALE + ((SCALE - 2*MW) & (-c));
    dp = (dp * WR) >> SCALElog;
    P = (word)((P - dp + c) & mSCALE);
  }
};

template<int DEC> static INLINE uint bit_code( Counter& c, uint bit ) {
  bit = rc.rc_BProcess( c.P, bit );
  c.Update( bit );
  return bit;
}

// A byte through a binary tree of 256 counters, optionally conditioned on the
// previous byte.
struct ByteModel {
  Counter t[256][256];
  void init() { for( int i=0; i<256; i++ ) for( int j=0; j<256; j++ ) t[i][j].init(); }
};

template<int DEC> static uint byte_code( ByteModel& m, uint prev, uint v ) {
  Counter* t = m.t[prev & 255];
  uint ctx = 1;
  for( int i=7; i>=0; i-- ) ctx += ctx + bit_code<DEC>( t[ctx], (v>>i)&1 );
  return ctx & 255;
}

// An unsigned integer as an adaptive Elias-gamma code: the bit length of v+1 in
// unary, then its mantissa.  Every stream that carries numbers -- palette
// deltas, counts, lengths -- goes through one of these with its own counters, so
// the distributions never collide.
struct NumModel {
  Counter len[34][34];       // [length of the previous number][unary position]
  Counter man[34][34];
  int prev;
  void init() {
    prev = 0;
    for( int i=0; i<34; i++ ) { for( int j=0; j<34; j++ ) len[i][j].init(), man[i][j].init(); }
  }
};

// Consecutive numbers in these streams are strongly correlated in magnitude --
// palette deltas especially -- so the unary length is coded in the context of
// the previous one.
template<int DEC> static qword num_code( NumModel& m, qword v ) {
  qword w = v + 1;
  int n = 0;
  if( !DEC ) { while( (w >> (n+1)) != 0 ) n++; }          // n = floor(log2 w)
  Counter* len = m.len[m.prev];
  int i = 0;
  for( ; i < 33; i++ ) {
    uint more = DEC ? 0u : (uint)(i < n);
    more = bit_code<DEC>( len[i], more );
    if( !more ) break;
  }
  if( DEC ) n = i;
  m.prev = n;
  qword r = 1;
  for( int j = n-1; j >= 0; j-- ) {
    uint b = DEC ? 0u : (uint)((w >> j) & 1);
    b = bit_code<DEC>( m.man[n][j], b );
    r = (r << 1) | b;
  }
  return DEC ? r - 1 : v;
}

// ---------------------------------------------------------------- the counts
//
// The occurrence table is a composition: K positive numbers with a known sum.
// Coding them one at a time throws that away.  This is the other way, taken from
// Shelwien's frqcomp (`model_freq.inc`, `freq_proc`): recurse over the table,
// splitting the range in half and sending how the parent's total divides between
// the halves, with the right half then determined by subtraction and never sent
// at all.  Two flags catch the degenerate splits -- all of it on one side, none
// of it anywhere -- and the split itself goes through a bounded binary code that
// spends nothing on values the total has already ruled out.
//
// Every count here is at least one, so what is coded is the table of n_i - 1
// against a total of N - K.  That turns "every entry occurs once", the commonest
// shape a palette has, into a table of zeros.
//
// It does not win everywhere, so it does not replace the per-number coder -- see
// PALETTE.md 11.5.  Both are coded and the shorter is kept.

#ifndef FRQ_CTX
#define FRQ_CTX 32                        // adaptive tree nodes; deeper ones go raw
#endif

struct FreqModel {
  Counter node[FRQ_CTX], cmax, cmin;
  void init() { for( int i=0; i<FRQ_CTX; i++ ) node[i].init(); cmax.init(); cmin.init(); }
};

static int log2i_( qword x ) { int n = -1; while( x ) { n++; x >>= 1; } return n; }

// n in (lo,hi), most significant bit first.  A bit whose whole subtree falls
// outside the interval is not coded: it is implied, and implied bits are free.
template<int DEC> static qword bounded_num( FreqModel& F, qword n, qword lo, qword hi ) {
  if( hi <= lo + 1 ) return lo;                    // nothing left to say
  int clen = log2i_( hi - 1 ) + 1;
  qword z = 0;
  for( int i = clen-1; i >= 0; i-- ) {
    qword step = 1ull << i;
    uint k = DEC ? 0u : (uint)((n >> i) & 1);
    if( z + step - 1 <= lo ) k = 1;
    else if( z + step >= hi ) k = 0;
    else {
      qword j = ((1ull << clen) + z) >> (i+1);
      if( j < FRQ_CTX ) k = bit_code<DEC>( F.node[j], k );
      else k = rc.rc_BProcess( hSCALE, k );        // too deep to be worth a counter
    }
    z |= (qword)k << i;
  }
  return z;
}

// The sum of f[A..B) given the total it has to fit inside.  total < sum on entry
// means the caller already knows the sum -- it is the right half of a split that
// has just been coded -- and then nothing is coded here, only inside.
template<int DEC> static qword freq_proc( FreqModel& F, qword* f, qword A, qword B,
                                          qword total, qword sum ) {
  if( B == A ) return 0;
  if( !DEC ) { sum = 0; for( qword i=A; i<B; i++ ) sum += f[i]; }
  if( total >= sum ) {
    if( bit_code<DEC>( F.cmax, DEC ? 0u : (uint)(sum == total) ) ) sum = total;
    else if( bit_code<DEC>( F.cmin, DEC ? 0u : (uint)(sum == 0) ) ) sum = 0;
    else sum = bounded_num<DEC>( F, sum, 0, total );
  }
  if( B - A <= 1 ) f[A] = sum;
  else if( sum == 0 ) { if( DEC ) for( qword i=A; i<B; i++ ) f[i] = 0; }
  else {
    qword h = (B - A) >> 1;
    qword s0 = freq_proc<DEC>( F, f, A, A+h, sum, 0 );
    if( s0 > sum ) return sum;                     // corrupt stream; caller checks
    freq_proc<DEC>( F, f, A+h, B, 0, sum - s0 );
  }
  return sum;
}

// ---------------------------------------------------------------- hash map
//
// (a,b) -> count over neighbouring index pairs.  Open addressed, power of two,
// grown by rehash; ~0ull marks an empty slot, so that key is reserved and never
// occurs because a and b are both bounded by K.

struct PairMap {
  qword* key; qword* val; qword mask, used;
  PairMap() { key=0; val=0; mask=0; used=0; }
  void alloc( qword bits ) {
    mask = (1ull<<bits) - 1;
    key = new qword[mask+1]; val = new qword[mask+1];
    for( qword i=0; i<=mask; i++ ) { key[i] = ~0ull; val[i] = 0; }
    used = 0;
  }
  void free_() { delete[] key; delete[] val; key=0; val=0; }
  static qword mix( qword x ) {
    x ^= x >> 33; x *= 0xFF51AFD7ED558CCDull;
    x ^= x >> 33; x *= 0xC4CEB9FE1A85EC53ull;
    return x ^ (x >> 33);
  }
  void grow();
  void add( qword k ) {
    if( (used+1)*4 > (mask+1)*3 ) grow();
    qword i = mix(k) & mask;
    while( key[i] != ~0ull && key[i] != k ) i = (i+1) & mask;
    if( key[i] == ~0ull ) { key[i] = k; used++; }
    val[i]++;
  }
};

void PairMap::grow() {
  qword obits = 0; while( (1ull<<obits) <= mask ) obits++;
  qword* ok = key; qword* ov = val; qword om = mask;
  alloc( obits + 1 );
  for( qword i=0; i<=om; i++ ) if( ok[i] != ~0ull ) {
    qword j = mix(ok[i]) & mask;
    while( key[j] != ~0ull ) j = (j+1) & mask;
    key[j] = ok[i]; val[j] = ov[i]; used++;
  }
  delete[] ok; delete[] ov;
}

// ---------------------------------------------------------------- the bitmap

static INLINE uint rd16( const byte* p ) { return p[0] | (p[1]<<8); }
static INLINE uint rd32( const byte* p ) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint)p[3]<<24); }
static INLINE void wr16( byte* p, uint v ) { p[0]=byte(v); p[1]=byte(v>>8); }
static INLINE void wr32( byte* p, uint v ) { p[0]=byte(v); p[1]=byte(v>>8); p[2]=byte(v>>16); p[3]=byte(v>>24); }

struct Bmp {
  byte* d; qword n;
  int W, H, bpp, comp, topdown;
  uint off, dib, ncol, palOff, stride, npx;
  qword pixbytes;          // uncompressed payload length, or the RLE stream length
  int ok;
  const char* why;
};

// Walk an RLE8 stream, calling back on every byte position that holds a palette
// index.  Returns the length of the stream, or 0 if it does not decode.  The
// same walk is used to collect the alphabet and to rewrite it, so the two can
// never disagree about which bytes are indices.
template<class F> static qword rle8_walk( const byte* s, qword avail, int W, int H, F& f ) {
  qword i = 0; int x = 0, y = 0;
  while( i + 1 < avail ) {
    uint a = s[i], b = s[i+1];
    if( a ) {
      if( x + (int)a > W || y >= H ) return 0;
      f( i+1, a );                        // one byte, a pixels
      x += a; i += 2;
    } else if( b == 0 ) { x = 0; y++; i += 2; }
    else if( b == 1 ) { i += 2; return i; }
    else if( b == 2 ) {
      if( i + 3 >= avail ) return 0;
      x += s[i+2]; y += s[i+3]; i += 4;
    } else {
      qword pad = b & 1;
      if( i + 2 + b + pad > avail ) return 0;
      if( x + (int)b > W || y >= H ) return 0;
      for( uint k=0; k<b; k++ ) f( i+2+k, 1 );
      x += b; i += 2 + b + pad;
    }
  }
  return i;
}

struct NopVisit { void operator()( qword, uint ) {} };

static void parse( Bmp* m ) {
  m->ok = 0; m->why = "not a BMP";
  const byte* d = m->d;
  if( m->n < 54 || d[0]!='B' || d[1]!='M' ) return;
  m->off = rd32(d+10); m->dib = rd32(d+14);
  if( m->dib < 40 || 14 + (qword)m->dib > m->n ) { m->why = "unsupported DIB header"; return; }
  int w = (int)rd32(d+18), sh = (int)rd32(d+22);
  m->topdown = sh < 0; int h = m->topdown ? -sh : sh;
  m->bpp = rd16(d+28); m->comp = rd32(d+30);
  if( w <= 0 || h <= 0 || rd16(d+26) != 1 ) { m->why = "unsupported geometry"; return; }
  m->W = w; m->H = h;
  if( !(m->bpp==8 || m->bpp==24 || m->bpp==32) ) { m->why = "only 8, 24 and 32 bpp"; return; }
  if( m->bpp==8  && !(m->comp==0 || m->comp==1) ) { m->why = "8bpp: only BI_RGB and BI_RLE8"; return; }
  if( m->bpp==24 && m->comp!=0 ) { m->why = "24bpp: only BI_RGB"; return; }
  if( m->bpp==32 && !(m->comp==0 || m->comp==3) ) { m->why = "32bpp: only BI_RGB and BI_BITFIELDS"; return; }
  if( m->comp==1 && m->topdown ) { m->why = "RLE8 cannot be top-down"; return; }

  uint clr = rd32(d+46);
  m->ncol = (m->bpp<=8) ? (clr ? clr : (1u<<m->bpp)) : 0;
  m->palOff = 14 + m->dib + ((m->bpp==32 && m->comp==3) ? 12 : 0);
  if( m->ncol ) {
    if( m->ncol > 256 ) { m->why = "more than 256 palette entries"; return; }
    if( (qword)m->palOff + 4ull*m->ncol > m->n ) { m->why = "palette runs past the file"; return; }
    if( m->off < m->palOff + 4u*m->ncol ) { m->why = "pixels overlap the palette"; return; }
  }
  if( (qword)m->off > m->n ) { m->why = "pixel offset past the file"; return; }

  m->npx = m->bpp >> 3;
  m->stride = (((qword)m->W*m->bpp + 31)/32)*4;
  if( m->comp == 1 ) {
    NopVisit nop;
    qword len = rle8_walk( d + m->off, m->n - m->off, m->W, m->H, nop );
    if( !len ) { m->why = "RLE8 stream does not decode"; return; }
    m->pixbytes = len;
  } else {
    m->pixbytes = (qword)m->stride * m->H;
    if( (qword)m->off + m->pixbytes > m->n ) { m->why = "pixel data runs past the file"; return; }
  }
  m->ok = 1; m->why = 0;
}

// ---------------------------------------------------------------- orderings

static const int ORD_PACKED = 0, ORD_LUMA = 1, ORD_WEIGHT = 2, ORD_PERM = 3;

struct Order {
  int mode;
  int w[4];        // ORD_WEIGHT, on B,G,R,A
  int* perm;       // ORD_PERM: perm[j] = index given to canonical entry j
};

// The colour a canonical palette entry stands for.  In a paletted file the
// symbol is the *stored index byte* -- two slots holding the same colour stay
// two symbols, because the file's bytes have to come back -- so the colour has
// to be looked up.  In a true-colour file the symbol is the colour.
struct Colours { int nc; byte c[4]; };

static void entry_colour( int mode_pal, const byte* pal, qword sym, byte* out, int nc ) {
  if( mode_pal ) { out[0]=pal[4*sym]; out[1]=pal[4*sym+1]; out[2]=pal[4*sym+2]; out[3]=0; }
  else { for( int i=0; i<nc; i++ ) out[i] = byte(sym >> (8*i)); }
}

struct SortKey { qword key; qword canon; };

static int cmp_key( const void* a, const void* b ) {
  const SortKey* x = (const SortKey*)a; const SortKey* y = (const SortKey*)b;
  if( x->key != y->key ) return x->key < y->key ? -1 : 1;
  return x->canon < y->canon ? -1 : (x->canon > y->canon);   // total, and decoder-reproducible
}

// pos[j] = index assigned to canonical entry j.  Every branch here has to run
// identically in the decoder, which is why the key is integer throughout
// (PALETTE.md 5.4): a floating-point key would order ties differently on a
// different compiler and the palette would come back permuted.
static void apply_order( const Order& o, int K, int mode_pal, const byte* pal,
                         const qword* sym, int nc, int* pos ) {
  if( o.mode == ORD_PACKED ) { for( int j=0; j<K; j++ ) pos[j] = j; return; }
  if( o.mode == ORD_PERM )   { for( int j=0; j<K; j++ ) pos[j] = o.perm[j]; return; }
  const int* w = o.w;
  int lum[4] = { 29, 150, 77, 0 };
  if( o.mode == ORD_LUMA ) w = lum;
  SortKey* k = new SortKey[K];
  for( int j=0; j<K; j++ ) {
    byte c[4]; entry_colour( mode_pal, pal, sym[j], c, nc );
    long long s = 0;
    for( int i=0; i<4; i++ ) s += (long long)w[i] * c[i];
    k[j].key = (qword)(s + (1ll<<40));            // bias: weights may be negative
    k[j].canon = (qword)j;
  }
  qsort( k, K, sizeof(SortKey), cmp_key );
  for( int j=0; j<K; j++ ) pos[ k[j].canon ] = j;
  delete[] k;
}

// The objective of PALETTE.md 5.3: the adjacency-weighted sum of rank
// differences.  Measured there against the real coder, it beats the residual
// entropies wherever the candidates are close together -- which is the only case
// where the choice matters.
static double order_cost( const PairMap& pm, const int* pos ) {
  double s = 0;
  for( qword i=0; i<=pm.mask; i++ ) if( pm.key[i] != ~0ull ) {
    int a = (int)(pm.key[i] >> 32), b = (int)(pm.key[i] & 0xFFFFFFFFu);
    int d = pos[a] - pos[b]; if( d < 0 ) d = -d;
    s += (double)pm.val[i] * d;
  }
  return s;
}

static uint gcd_( uint a, uint b ) { while( b ) { uint t = a % b; a = b; b = t; } return a; }

// ---------------------------------------------------------------- exclusion
//
// The occurrence table is only worth carrying if something reads it.  PALETTE.md
// 3.2 works out what reading it buys: given the counts, the exact information
// content of the index sequence is the multinomial log2(N! / prod n_i!), and a
// coder reaches it by striking each symbol out of the alphabet as its count runs
// down to zero.  Everything after that point that would have named the dead
// symbol is redundancy, and the last stretch of the image -- where one symbol is
// all that is left -- is free.
//
// So the index a pixel is given is not a fixed number per colour.  It is the
// colour's **rank among the symbols still alive**, which a Fenwick tree over the
// live flags answers, and updates, in log K.  The transform is a bijection given
// the counts, so the file still comes back byte for byte; and it is monotone in
// the static order, so a rank only ever drifts downwards and the spatial
// structure the ordering search worked for survives.

struct Fenwick {
  int n; int* t;
  Fenwick() { n = 0; t = 0; }
  void init( int K ) {                       // every symbol alive
    n = K; t = new int[K+1]; t[0] = 0;
    for( int i=1; i<=K; i++ ) t[i] = i & (-i);
  }
  void free_() { delete[] t; t = 0; }
  void kill( int i ) { for( int j=i+1; j<=n; j += j & (-j) ) t[j]--; }
  int  rank( int i ) {                       // how many alive strictly below i
    int s = 0; for( int j=i; j>0; j -= j & (-j) ) s += t[j];
    return s;
  }
  int  nth( int k ) {                        // the k-th alive, k counted from 0
    int i = 0, lg = 1;
    while( (lg<<1) <= n ) lg <<= 1;
    for( int b=lg; b; b >>= 1 ) if( i + b <= n && t[i+b] <= k ) { i += b; k -= t[i]; }
    return i;                                // i is 0-based
  }
};

// One step of the countdown, shared by both directions so they cannot disagree.
struct Countdown {
  Fenwick fw; qword* remain; int K, alive;
  double alivesum; qword steps;              // how much alphabet the coder saw
  Countdown() { remain = 0; K = alive = 0; alivesum = 0; steps = 0; }
  void init( int K_, const qword* cnt ) {
    K = alive = K_; fw.init( K_ );
    remain = new qword[K_];
    for( int j=0; j<K_; j++ ) { remain[j] = cnt[j]; if( !cnt[j] ) { fw.kill(j); alive--; } }
  }
  void free_() { fw.free_(); delete[] remain; remain = 0; }
  int  to_rank( int j ) { return fw.rank( j ); }
  int  from_rank( int r ) { return fw.nth( r ); }   // caller checks r < alive
  int  take( int j, qword k ) {              // 0 if the counts do not cover it
    alivesum += alive; steps++;
    if( j < 0 || j >= K || remain[j] < k ) return 0;
    remain[j] -= k;
    if( !remain[j] ) { fw.kill( j ); alive--; }
    return 1;
  }
  int  spent() { for( int j=0; j<K; j++ ) if( remain[j] ) return 0; return 1; }
};

// The two visitors that carry the countdown through an RLE8 stream.  A run is
// one index byte standing for n pixels, and the rank cannot move inside a run --
// only the run's own symbol is being spent, and it cannot die before its last
// pixel -- so one substitution per byte is right in both directions.
struct RleToRank {
  byte* s; const int* map; Countdown* cd; int ok;
  void operator()( qword p, uint n ) {
    if( !ok ) return;
    int j = map[ s[p] ];
    s[p] = byte( cd->to_rank( j ) );
    if( !cd->take( j, n ) ) ok = 0;
  }
};
struct RleFromRank {
  byte* s; const int* map; Countdown* cd; int ok;   // map[static index] = original byte
  void operator()( qword p, uint n ) {
    if( !ok ) return;
    int r = s[p];
    if( r >= cd->alive ) { ok = 0; return; }
    int j = cd->from_rank( r );
    s[p] = byte( map[j] );
    if( !cd->take( j, n ) ) ok = 0;
  }
};

// ---------------------------------------------------------------- .pal stream
//
// One function, run forwards to write and backwards to read, so the two can
// never drift.  Everything in it is range coded; the file has a four byte magic
// in front and nothing else in the clear.

struct PalCtx {
  int   mode;                 // 0 = paletted file, 1 = true colour
  qword K;
  int   ordmode, w[4];
  int*  perm;
  int   has_counts, excl, frqmode;
  qword ncol;      byte*  paltab;     // mode 0
  qword hdrlen;    byte*  hdr;        // mode 1
  qword padlen;    byte*  pad;
  qword taillen;   byte*  tail;
  qword W, H, npx;                    // mode 1 geometry
  qword* sym;                         // K canonical symbols, ascending
  qword* cnt;                         // K counts, in index order
  int owns;                           // the decoder allocates all of it; the
  PalCtx() { memset( this, 0, sizeof(*this) ); }   // encoder mostly points into
  void free_() {                                   // the file it read
    if( owns ) { delete[] paltab; delete[] hdr; delete[] pad; delete[] tail; }
    delete[] perm; delete[] sym; delete[] cnt;
    memset( this, 0, sizeof(*this) );
  }
};

struct PalModels {
  ByteModel bm, bmd;
  NumModel  nmisc, nlen, ndelta, ncnt, nperm;
  FreqModel fq;
  void init() { bm.init(); bmd.init(); nmisc.init(); nlen.init(); ndelta.init();
                ncnt.init(); nperm.init(); fq.init(); }
};

// A run of bytes, order-1.
template<int DEC> static void blob( PalModels& M, byte* p, qword n ) {
  uint prev = 0;
  for( qword i=0; i<n; i++ ) prev = byte_code<DEC>( M.bm, prev, DEC ? 0 : p[i] ), p[i] = byte(prev);
}

// A run of bytes with a period: code the difference against the byte one period
// back, in a context of the phase.  A BMP palette table is four bytes per entry
// and its columns are ramps -- a grey ramp differences to a run of ones, the
// 3-3-2 cube to a short repeating pattern -- so this is the difference between
// spending a kilobyte on the table and spending tens of bytes.
template<int DEC> static void blob_period( PalModels& M, byte* p, qword n, qword per ) {
  for( qword i=0; i<n; i++ ) {
    uint pred = (i >= per) ? p[i-per] : 0;
    uint v = byte_code<DEC>( M.bmd, (uint)(i % per), DEC ? 0 : (uint)((p[i] - pred) & 255) );
    p[i] = byte( (v + pred) & 255 );
  }
}

template<int DEC> static int pal_stream( PalCtx& c, PalModels& M ) {
  // Every bound below is checked because a .pal that did not come from here --
  // truncated, corrupted, or simply the wrong file -- must be refused rather
  // than believed.  A decoder that trusts its own stream is a decoder that
  // indexes a palette with whatever fell out of the arithmetic coder.
  c.mode       = (int)num_code<DEC>( M.nmisc, c.mode );
  c.K          =      num_code<DEC>( M.nmisc, c.K );
  c.ordmode    = (int)num_code<DEC>( M.nmisc, c.ordmode );
  if( c.mode > 1 || c.K == 0 || c.K > (1u<<24) || c.ordmode > ORD_PERM ) return 0;
  if( c.ordmode == ORD_WEIGHT )
    for( int i=0; i<4; i++ ) {
      qword v = num_code<DEC>( M.nmisc, (qword)(c.w[i] + 256) );
      if( v > 512 ) return 0;
      c.w[i] = (int)v - 256;
    }
  if( c.ordmode == ORD_PERM ) {
    if( DEC ) { c.perm = new int[c.K]; for( qword j=0; j<c.K; j++ ) c.perm[j] = 0; }
    for( qword j=0; j<c.K; j++ ) {
      qword v = num_code<DEC>( M.nperm, (qword)c.perm[j] );
      if( v >= c.K ) return 0;
      c.perm[j] = (int)v;
    }
  }
  c.has_counts = (int)num_code<DEC>( M.nmisc, c.has_counts );
  c.excl       = (int)num_code<DEC>( M.nmisc, c.excl );
  if( c.excl > 1 ) return 0;
  if( c.has_counts ) {
    c.frqmode = (int)num_code<DEC>( M.nmisc, c.frqmode );
    if( c.frqmode > 1 ) return 0;
  }

  if( c.mode == 0 ) {
    c.ncol = num_code<DEC>( M.nmisc, c.ncol );
    if( c.ncol == 0 || c.ncol > 256 || c.K > c.ncol ) return 0;
    if( DEC ) c.paltab = new byte[c.ncol*4];
    blob_period<DEC>( M, c.paltab, c.ncol*4, 4 );
  } else {
    c.W = num_code<DEC>( M.nmisc, c.W );
    c.H = num_code<DEC>( M.nmisc, c.H );
    c.npx = num_code<DEC>( M.nmisc, c.npx );
    if( c.W == 0 || c.H == 0 || c.W > (1u<<20) || c.H > (1u<<20) ) return 0;
    if( c.npx < 3 || c.npx > 4 || c.W*c.H > (1ull<<32) ) return 0;
    c.hdrlen  = num_code<DEC>( M.nlen, c.hdrlen );
    if( c.hdrlen > (1u<<26) ) return 0;
    if( DEC ) c.hdr = new byte[c.hdrlen ? c.hdrlen : 1];
    blob<DEC>( M, c.hdr, c.hdrlen );
    c.padlen  = num_code<DEC>( M.nlen, c.padlen );
    if( c.padlen > 4ull*c.H || c.padlen % c.H ) return 0;
    if( DEC ) c.pad = new byte[c.padlen ? c.padlen : 1];
    blob<DEC>( M, c.pad, c.padlen );
    c.taillen = num_code<DEC>( M.nlen, c.taillen );
    if( c.taillen > (1u<<26) ) return 0;
    if( DEC ) c.tail = new byte[c.taillen ? c.taillen : 1];
    blob<DEC>( M, c.tail, c.taillen );
  }

  // The palette as a set, ascending: PALETTE.md 2.1 -- the order carries no
  // information, so it is sent canonically and the ordering is rebuilt from the
  // parameters above.  2.5 says the differences of the packed integer are the
  // cheapest thing to send, cheaper than any colour transform or curve order.
  if( DEC ) { c.sym = new qword[c.K]; for( qword j=0; j<c.K; j++ ) c.sym[j] = 0; }
  qword limit = (c.mode == 0) ? c.ncol : (c.npx == 4 ? (1ull<<32) : (1ull<<24));
  qword prev = 0;
  for( qword j=0; j<c.K; j++ ) {
    qword d = DEC ? 0 : c.sym[j] - prev - (j != 0);
    d = num_code<DEC>( M.ndelta, d );
    if( DEC ) {
      if( d > limit ) return 0;
      c.sym[j] = prev + d + (j != 0);
    }                                          // an index into a palette that is
    if( c.sym[j] >= limit ) return 0;          // not there is the whole risk --
    prev = c.sym[j];                           // and the encoder checks it too,
                                               // so the two cannot disagree
  }

  if( c.has_counts ) {
    if( DEC ) c.cnt = new qword[c.K];
    if( c.frqmode == 0 ) {
      // one number at a time, length contexted on the previous length.  This one
      // needs no total, so it is not sent -- the mode is coded first precisely so
      // that the rest of the block can differ.
      for( qword j=0; j<c.K; j++ ) {
        qword v = num_code<DEC>( M.ncnt, DEC ? 0 : c.cnt[j] - 1 );
        if( v > (1ull<<40) ) return 0;
        if( DEC ) c.cnt[j] = v + 1;
      }
    } else {
      qword tot = 0;
      if( !DEC ) for( qword j=0; j<c.K; j++ ) tot += c.cnt[j];
      tot = num_code<DEC>( M.ncnt, tot );
      if( tot < c.K || tot > (1ull<<40) ) return 0;    // K counts, each at least one
      // the whole table at once, against its known total
      qword* m = new qword[c.K];
      if( !DEC ) for( qword j=0; j<c.K; j++ ) m[j] = c.cnt[j] - 1;
      qword got = freq_proc<DEC>( M.fq, m, 0, c.K, tot - c.K, 0 );
      qword chk = 0;
      for( qword j=0; j<c.K; j++ ) { c.cnt[j] = m[j] + 1; chk += m[j]; }
      delete[] m;
      if( got != tot - c.K || chk != tot - c.K ) return 0;
    }
  }
  return 1;
}

// fn == 0 measures instead of writing, which is how the report below can say
// what the occurrence table costs without coding the file twice by hand.
static qword write_pal( const char* fn, PalCtx& c ) {
  qword cap = (1u<<16) + 16*( c.ncol*4 + c.K*10 + c.hdrlen + c.padlen + c.taillen );
  byte* best = 0; qword bn = 0; int bestmode = 0;
  // Neither way of coding the occurrence table wins everywhere -- one number at
  // a time reads a long tail of ones cheaply, the whole table at once reads a
  // small dense one much better -- so both are coded and the shorter is kept.
  // It costs two passes over a few kilobytes and one bit to say which.
  for( int mode = 0; mode <= (c.has_counts ? 1 : 0); mode++ ) {
    byte* buf = new byte[cap];
    buf[0]='B'; buf[1]='P'; buf[2]='A'; buf[3]='L'; buf[4]=3;
    c.frqmode = mode;
    rc.init( buf+5, cap-5, 0 );
    PalModels* M = new PalModels; M->init();
    pal_stream<0>( c, *M );
    rc.flush();
    delete M;
    if( rc.mpos >= cap-5 ) { fprintf(stderr,"palette stream overflowed\n"); delete[] buf; delete[] best; return 0; }
    qword n = 5 + rc.mpos;
    if( !best || n < bn ) { delete[] best; best = buf; bn = n; bestmode = mode; }
    else delete[] buf;
  }
  c.frqmode = bestmode;
  if( fn && !spew( fn, best, bn ) ) bn = 0;
  delete[] best;
  return bn;
}

static int read_pal( const char* fn, PalCtx& c ) {
  qword n; byte* d = slurp( fn, &n );
  if( !d ) return 0;
  if( n < 6 || d[0]!='B' || d[1]!='P' || d[2]!='A' || d[3]!='L' || d[4]!=3 ) {
    fprintf( stderr, "%s: not a bmppal palette\n", fn ); delete[] d; return 0;
  }
  rc.init( d+5, n-5, 1 );
  PalModels* M = new PalModels; M->init();
  c.owns = 1;
  int ok = pal_stream<1>( c, *M );
  delete M; delete[] d;
  if( !ok ) { fprintf( stderr, "%s: palette stream is corrupt\n", fn ); c.free_(); }
  return ok;
}

// ---------------------------------------------------------------- the search
//
// Candidates in increasing order of what they cost to describe: the canonical
// order (free), luma (free), an integer weight vector (four bytes), an explicit
// permutation (K numbers, affordable only for a tiny palette -- PALETTE.md 4).
// All of them are scored with the adjacency objective of 5.3.

static int next_perm( int* a, int n ) {
  int i = n-2; while( i >= 0 && a[i] >= a[i+1] ) i--;
  if( i < 0 ) return 0;
  int j = n-1; while( a[j] <= a[i] ) j--;
  int t = a[i]; a[i] = a[j]; a[j] = t;
  for( int l=i+1, r=n-1; l<r; l++, r-- ) { t=a[l]; a[l]=a[r]; a[r]=t; }
  return 1;
}

static Order choose_order( int mode_pal, const byte* pal, const qword* sym, int K,
                           int nc, const PairMap& pm, int quiet ) {
  Order best; best.mode = ORD_PACKED; best.perm = 0;
  memset( best.w, 0, sizeof(best.w) );
  int* pos = new int[K];
  apply_order( best, K, mode_pal, pal, sym, nc, pos );
  double bc = order_cost( pm, pos );
  if( !quiet ) fprintf( stderr, "  ordering: packed %.0f", bc );

  Order o; o.perm = 0; memset( o.w, 0, sizeof(o.w) );
  o.mode = ORD_LUMA;
  apply_order( o, K, mode_pal, pal, sym, nc, pos );
  double c = order_cost( pm, pos );
  if( !quiet ) fprintf( stderr, "  luma %.0f", c );
  if( c < bc ) { bc = c; best = o; }

  int R = (K <= 512) ? 8 : (K <= 4096 ? 4 : 0);
  if( nc == 4 && R > 4 ) R = 4;
  if( R ) {
    o.mode = ORD_WEIGHT;
    int w[4] = {0,0,0,0}, tried = 0;
    for( w[3] = (nc==4 ? -R : 0); w[3] <= (nc==4 ? R : 0); w[3]++ )
    for( w[2] = -R; w[2] <= R; w[2]++ )
    for( w[1] = -R; w[1] <= R; w[1]++ )
    for( w[0] = -R; w[0] <= R; w[0]++ ) {
      int lead = 0;
      for( int i=nc-1; i>=0; i-- ) if( w[i] ) { lead = w[i]; break; }
      if( lead <= 0 ) continue;                       // w and -w are one candidate
      uint g = 0;
      for( int i=0; i<nc; i++ ) g = gcd_( g, (uint)(w[i]<0 ? -w[i] : w[i]) );
      if( g != 1 ) continue;                          // and so are w and 2w
      memcpy( o.w, w, sizeof(o.w) );
      apply_order( o, K, mode_pal, pal, sym, nc, pos );
      c = order_cost( pm, pos ); tried++;
      if( c < bc ) { bc = c; best = o; }
    }
    if( !quiet ) fprintf( stderr, "  weights(%d tried) %.0f", tried, bc );
  }

  if( K <= 8 ) {                                      // PALETTE.md 6.2
    int* p = new int[K];
    for( int i=0; i<K; i++ ) p[i] = i;
    int* bp = new int[K]; double pc = 1e300;
    do {
      c = order_cost( pm, p );
      if( c < pc ) { pc = c; memcpy( bp, p, K*sizeof(int) ); }
    } while( next_perm( p, K ) );
    if( pc < bc ) { bc = pc; best.mode = ORD_PERM; best.perm = bp; }
    else delete[] bp;
    delete[] p;
    if( !quiet ) fprintf( stderr, "  perm %.0f", pc );
  }
  if( !quiet ) {
    static const char* nm[4] = { "packed", "luma", "weights", "permutation" };
    fprintf( stderr, "  ->  %s", nm[best.mode] );
    if( best.mode == ORD_WEIGHT )
      fprintf( stderr, " (%d,%d,%d,%d)", best.w[0], best.w[1], best.w[2], best.w[3] );
    fprintf( stderr, "\n" );
  }
  delete[] pos;
  return best;
}

// ---------------------------------------------------------------- index image

static int index_bits( qword K ) { return K <= 256 ? 8 : (K <= (1u<<24) ? 24 : 32); }

static byte* build_index_bmp( const int* idx, int W, int H, int topdown, qword K, qword* outn ) {
  int bits = index_bits( K ), npx = bits >> 3;
  qword stride = (((qword)W*bits + 31)/32)*4;
  // Only as many palette entries as there are indices, spread over the grey
  // ramp: the file stays viewable, the header stays small, and the ramp stays
  // monotone, which is what makes `bmg` code the indices instead of expanding
  // them into colour planes.
  qword palb = (bits == 8) ? 4*K : 0;
  qword off = 54 + palb, n = off + stride*H;
  byte* d = new byte[n];
  memset( d, 0, (size_t)n );
  d[0]='B'; d[1]='M';
  wr32( d+2, (uint)n ); wr32( d+10, (uint)off );
  wr32( d+14, 40 ); wr32( d+18, (uint)W ); wr32( d+22, (uint)(topdown ? -H : H) );
  wr16( d+26, 1 ); wr16( d+28, (uint)bits ); wr32( d+30, 0 );
  wr32( d+34, (uint)(stride*H) ); wr32( d+38, 2835 ); wr32( d+42, 2835 );
  wr32( d+46, bits==8 ? (uint)K : 0 );
  for( qword i=0; i<palb/4; i++ ) {
    byte v = byte( K > 1 ? (i*255 + (K-1)/2)/(K-1) : 0 );
    d[54+4*i] = d[55+4*i] = d[56+4*i] = v;
  }
  for( int y=0; y<H; y++ ) {
    byte* r = d + off + (qword)y*stride;
    const int* s = idx + (qword)y*W;
    for( int x=0; x<W; x++ ) for( int p=0; p<npx; p++ ) r[(qword)x*npx+p] = byte(s[x] >> (8*p));
  }
  *outn = n;
  return d;
}

// Read the index image back, in the same storage row order it was written in.
static int* read_index_bmp( const Bmp& m, qword K ) {
  int bits = index_bits( K );
  if( m.bpp != bits || m.comp != 0 ) return 0;
  int npx = bits >> 3;
  int* idx = new int[(qword)m.W*m.H];
  for( int y=0; y<m.H; y++ ) {
    const byte* r = m.d + m.off + (qword)y*m.stride;
    int* s = idx + (qword)y*m.W;
    for( int x=0; x<m.W; x++ ) {
      uint v = 0;
      for( int p=0; p<npx; p++ ) v |= (uint)r[(qword)x*npx+p] << (8*p);
      if( v >= K ) { delete[] idx; return 0; }
      s[x] = (int)v;
    }
  }
  return idx;
}

// ---------------------------------------------------------------- compress

static int cmp_qw( const void* a, const void* b ) {
  qword x = *(const qword*)a, y = *(const qword*)b;
  return x < y ? -1 : (x > y);
}

static qword find_sym( const qword* sym, qword K, qword v ) {
  qword lo = 0, hi = K;
  while( lo < hi ) { qword m = (lo+hi)>>1; if( sym[m] < v ) lo = m+1; else hi = m; }
  return lo;
}

static void rle8_image( const byte* s, qword avail, int W, int H, qword* raw ) {
  for( qword i=0; i<(qword)W*H; i++ ) raw[i] = ~0ull;
  qword i = 0; int x = 0, y = 0;
  while( i + 1 < avail ) {
    uint a = s[i], b = s[i+1];
    if( a ) {
      if( y >= H || x + (int)a > W ) break;
      for( uint k=0; k<a; k++ ) raw[(qword)y*W+x+k] = b;
      x += a; i += 2;
    } else if( b == 0 ) { x = 0; y++; i += 2; }
    else if( b == 1 ) { i += 2; break; }
    else if( b == 2 ) { if( i+3 >= avail ) break; x += s[i+2]; y += s[i+3]; i += 4; }
    else {
      if( y >= H || x + (int)b > W || i+2+b > avail ) break;
      for( uint k=0; k<b; k++ ) raw[(qword)y*W+x+k] = s[i+2+k];
      x += b; i += 2 + b + (b&1);
    }
  }
}

// Rewrite every index byte of an RLE8 stream in place through map[].
struct RleRemap {
  byte* s; const int* map;
  void operator()( qword p, uint ) { s[p] = byte(map[ s[p] ]); }
};

static int do_compress( const char* fin, const char* fbmp, const char* fpal,
                        const char* ffrq, int quiet, int excl ) {
  Bmp m; m.d = slurp( fin, &m.n );
  if( !m.d ) return 0;
  parse( &m );
  if( !m.ok ) { fprintf( stderr, "%s: %s\n", fin, m.why ); delete[] m.d; return 0; }

  int mode_pal = (m.bpp == 8);
  qword N = (qword)m.W * m.H;
  qword* raw = new qword[N];

  if( mode_pal ) {
    if( m.comp == 1 ) rle8_image( m.d + m.off, m.n - m.off, m.W, m.H, raw );
    else for( int y=0; y<m.H; y++ ) {
      const byte* r = m.d + m.off + (qword)y*m.stride;
      for( int x=0; x<m.W; x++ ) raw[(qword)y*m.W+x] = r[x];
    }
  } else {
    for( int y=0; y<m.H; y++ ) {
      const byte* r = m.d + m.off + (qword)y*m.stride;
      for( int x=0; x<m.W; x++ ) {
        const byte* s = r + (qword)x*m.npx;
        qword k = s[0] | ((qword)s[1]<<8) | ((qword)s[2]<<16);
        if( m.npx == 4 ) k |= (qword)s[3]<<24;
        raw[(qword)y*m.W+x] = k;
      }
    }
  }

  // the alphabet, and how often each member occurs
  PairMap hm; hm.alloc( 12 );
  for( qword i=0; i<N; i++ ) if( raw[i] != ~0ull ) hm.add( raw[i] );
  qword K = hm.used;
  if( K == 0 ) { fprintf(stderr,"%s: no pixels\n",fin); delete[] raw; hm.free_(); delete[] m.d; return 0; }
  if( mode_pal ) {
    // An index byte with no palette entry behind it has no colour to sort by and
    // nothing to restore from.  It is a malformed file, not a hard case.
    qword hi = 0;
    for( qword i=0; i<=hm.mask; i++ ) if( hm.key[i] != ~0ull && hm.key[i] > hi ) hi = hm.key[i];
    if( hi >= m.ncol ) {
      fprintf( stderr, "%s: pixel index %llu is outside the %u-entry palette\n",
               fin, (unsigned long long)hi, m.ncol );
      delete[] raw; hm.free_(); delete[] m.d; return 0;
    }
  }
  qword* sym = new qword[K];
  { qword j = 0;
    for( qword i=0; i<=hm.mask; i++ ) if( hm.key[i] != ~0ull ) sym[j++] = hm.key[i]; }
  qsort( sym, (size_t)K, sizeof(qword), cmp_qw );
  qword* scnt = new qword[K];
  for( qword j=0; j<K; j++ ) {
    qword i = PairMap::mix(sym[j]) & hm.mask;
    while( hm.key[i] != sym[j] ) i = (i+1) & hm.mask;
    scnt[j] = hm.val[i];
  }
  hm.free_();

  int* idx0 = new int[N];
  for( qword i=0; i<N; i++ )
    idx0[i] = (raw[i] == ~0ull) ? -1 : (int)find_sym( sym, K, raw[i] );
  delete[] raw;

  // adjacency counts -- equal neighbours contribute nothing to the objective and
  // are the majority, so they never enter the table
  PairMap pm; pm.alloc( 12 );
  for( int y=0; y<m.H; y++ ) for( int x=0; x<m.W; x++ ) {
    int a = idx0[(qword)y*m.W+x];
    if( a < 0 ) continue;
    if( x ) { int b = idx0[(qword)y*m.W+x-1]; if( b >= 0 && b != a ) pm.add( ((qword)a<<32)|(qword)b ); }
    if( y ) { int b = idx0[(qword)(y-1)*m.W+x]; if( b >= 0 && b != a ) pm.add( ((qword)a<<32)|(qword)b ); }
  }

  Order o = choose_order( mode_pal, mode_pal ? m.d + m.palOff : 0, sym, (int)K,
                          mode_pal ? 3 : (int)m.npx, pm, quiet );
  pm.free_();
  int* pos = new int[K];
  apply_order( o, (int)K, mode_pal, mode_pal ? m.d + m.palOff : 0, sym,
               mode_pal ? 3 : (int)m.npx, pos );

  qword* cnt = new qword[K];
  for( qword j=0; j<K; j++ ) cnt[ pos[j] ] = scnt[j];
  delete[] scnt;

  Countdown cd;
  if( excl ) cd.init( (int)K, cnt );

  // ---- out.bmp
  qword bn = 0; byte* bd = 0;
  if( mode_pal ) {
    // the file itself, with the palette replaced by a grey ramp and every index
    // byte remapped; the run structure, the row padding and everything outside
    // the pixels survive untouched, which is what makes this byte exact
    bn = m.n; bd = new byte[bn];
    memcpy( bd, m.d, (size_t)bn );
    for( uint i=0; i<m.ncol; i++ ) {
      byte* p = bd + m.palOff + 4*i;
      p[0] = p[1] = p[2] = byte(i < 256 ? i : 255); p[3] = 0;
    }
    int map[256]; for( int i=0; i<256; i++ ) map[i] = 0;
    for( qword j=0; j<K; j++ ) map[ (int)sym[j] ] = pos[j];
    if( m.comp == 1 ) {
      if( excl ) {
        RleToRank rr; rr.s = bd + m.off; rr.map = map; rr.cd = &cd; rr.ok = 1;
        rle8_walk( bd + m.off, bn - m.off, m.W, m.H, rr );
        if( !rr.ok ) { fprintf(stderr,"%s: internal: the countdown ran dry\n",fin); return 0; }
      } else {
        RleRemap rr; rr.s = bd + m.off; rr.map = map;
        rle8_walk( bd + m.off, bn - m.off, m.W, m.H, rr );
      }
    } else for( int y=0; y<m.H; y++ ) {
      byte* r = bd + m.off + (qword)y*m.stride;
      for( int x=0; x<m.W; x++ ) {
        int j = map[r[x]];
        r[x] = byte( excl ? cd.to_rank(j) : j );
        if( excl ) cd.take( j, 1 );
      }
    }
  } else {
    int* idx = new int[N];
    for( qword i=0; i<N; i++ ) {
      int j = pos[ idx0[i] ];
      idx[i] = excl ? cd.to_rank(j) : j;
      if( excl ) cd.take( j, 1 );
    }
    bd = build_index_bmp( idx, m.W, m.H, m.topdown, K, &bn );
    delete[] idx;
  }
  if( excl && !cd.spent() ) { fprintf(stderr,"%s: internal: the countdown did not finish\n",fin); return 0; }
  double meanalive = (excl && cd.steps) ? cd.alivesum/cd.steps : (double)K;
  cd.free_();
  if( !spew( fbmp, bd, bn ) ) { delete[] bd; return 0; }

  // ---- out.pal
  PalCtx c;
  c.mode = mode_pal ? 0 : 1;
  c.K = K; c.ordmode = o.mode; c.perm = o.perm;
  memcpy( c.w, o.w, sizeof(c.w) );
  c.has_counts = ffrq ? 0 : 1;
  c.excl = excl;
  c.sym = sym; c.cnt = cnt;
  qword padrow = 0;
  if( mode_pal ) {
    c.ncol = m.ncol;
    c.paltab = m.d + m.palOff;
  } else {
    c.W = m.W; c.H = m.H; c.npx = m.npx;
    c.hdrlen = m.off;  c.hdr = m.d;
    padrow = m.stride - (qword)m.W*m.npx;
    c.padlen = padrow * m.H;
    c.pad = new byte[c.padlen ? c.padlen : 1];
    for( int y=0; y<m.H; y++ )
      memcpy( c.pad + (qword)y*padrow, m.d + m.off + (qword)y*m.stride + (qword)m.W*m.npx, (size_t)padrow );
    c.taillen = m.n - m.off - m.pixbytes;
    c.tail = m.d + m.off + m.pixbytes;
  }
  qword pn_bare = 0;
  if( c.has_counts ) { c.has_counts = 0; pn_bare = write_pal( 0, c ); c.has_counts = 1; }
  qword pn = write_pal( fpal, c );
  if( !pn ) return 0;

  // ---- out.frq
  qword fn_ = 0;
  if( ffrq ) {
    byte* fb = new byte[K*4];
    for( qword j=0; j<K; j++ ) {
      qword v = cnt[j] > 0xFFFFFFFFull ? 0xFFFFFFFFull : cnt[j];
      wr32( fb + 4*j, (uint)v );
    }
    if( !spew( ffrq, fb, K*4 ) ) { delete[] fb; return 0; }
    fn_ = K*4; delete[] fb;
  }

  if( !quiet ) {
    fprintf( stderr, "%s: %dx%d %dbpp, %llu distinct pixels (%d bits/index)\n"
                     "  %llu -> %llu bmp + %llu pal%s%llu frq = %llu\n",
             fin, m.W, m.H, m.bpp, (unsigned long long)K, index_bits(K),
             (unsigned long long)m.n, (unsigned long long)bn, (unsigned long long)pn,
             ffrq ? " + " : " = ", (unsigned long long)fn_,
             (unsigned long long)(bn+pn+fn_) );
    if( pn_bare )
      fprintf( stderr, "  of the palette, %llu bytes are the occurrence table (%s)%s\n",
               (unsigned long long)(pn - pn_bare),
               c.frqmode ? "split against its total" : "one number at a time",
               excl ? ", which the index image needs" : " -- nothing reads it" );
    if( excl )
      fprintf( stderr, "  countdown: the alphabet averaged %.1f of %llu symbols"
                       " (%.1f%%) -- that is all the exclusion can be worth\n",
               meanalive, (unsigned long long)K, 100.0*meanalive/(double)K );
  }

  delete[] bd; delete[] pos; delete[] idx0; delete[] sym; delete[] cnt;
  if( o.perm ) delete[] o.perm;
  if( !mode_pal ) delete[] c.pad;
  delete[] m.d;
  return 1;
}

// ---------------------------------------------------------------- restore

static int do_restore( const char* fbmp, const char* fpal, const char* ffrq,
                       const char* fout, int quiet ) {
  PalCtx c;
  int rv = 0, K = 0;
  int *pos = 0, *inv = 0;
  qword* have = 0;
  qword* cnt = 0;
  byte* rd = 0;
  qword rn = 0;
  Countdown cd;
  Bmp m; m.d = 0;

  if( !read_pal( fpal, c ) ) return 0;
  m.d = slurp( fbmp, &m.n );
  if( !m.d ) goto done;
  parse( &m );
  if( !m.ok ) { fprintf( stderr, "%s: %s\n", fbmp, m.why ); goto done; }

  // rebuild the ordering from the parameters, then invert it
  K = (int)c.K;
  pos = new int[K];
  { Order o; o.mode = c.ordmode; o.perm = c.perm; memcpy( o.w, c.w, sizeof(o.w) );
    apply_order( o, K, c.mode==0, c.paltab, c.sym, c.mode==0 ? 3 : (int)c.npx, pos ); }
  inv = new int[K];
  for( int j=0; j<K; j++ ) inv[j] = -1;
  for( int j=0; j<K; j++ ) {
    if( pos[j] < 0 || pos[j] >= K || inv[pos[j]] >= 0 ) {
      fprintf( stderr, "%s: the ordering is not a permutation\n", fpal ); goto done;
    }
    inv[pos[j]] = j;
  }

  // With the countdown on, the occurrence table is not a note about the image --
  // it is half the code.  It has to be here before a single pixel is read.
  if( c.excl ) {
    if( ffrq ) {
      qword fn2; byte* fb = slurp( ffrq, &fn2 );
      if( !fb ) goto done;
      if( fn2 != (qword)K*4 ) {
        fprintf(stderr,"%s: %llu bytes, expected %d entries\n",ffrq,(unsigned long long)fn2,K);
        delete[] fb; goto done;
      }
      cnt = new qword[K];
      for( int j=0; j<K; j++ ) cnt[j] = rd32( fb + 4*j );
      delete[] fb;
      if( c.has_counts ) for( int j=0; j<K; j++ ) if( cnt[j] != c.cnt[j] ) {
        fprintf( stderr, "%s: disagrees with the table coded into %s\n", ffrq, fpal ); goto done;
      }
    } else if( c.has_counts ) {
      cnt = new qword[K];
      for( int j=0; j<K; j++ ) cnt[j] = c.cnt[j];
    } else {
      fprintf( stderr, "%s: the index image was coded against the occurrence table,"
                       " which is not in here -- name the out.frq\n", fpal );
      goto done;
    }
    cd.init( K, cnt );
  }

  if( c.mode == 0 ) {
    if( m.bpp != 8 || m.ncol != c.ncol ) { fprintf(stderr,"%s: does not match the palette\n",fbmp); goto done; }
    rn = m.n; rd = new byte[rn];
    memcpy( rd, m.d, (size_t)rn );
    memcpy( rd + m.palOff, c.paltab, (size_t)(c.ncol*4) );
    int map[256]; for( int i=0; i<256; i++ ) map[i] = 0;
    for( int u=0; u<K; u++ ) map[u] = (int)c.sym[ inv[u] ];
    if( m.comp == 1 ) {
      if( c.excl ) {
        RleFromRank rr; rr.s = rd + m.off; rr.map = map; rr.cd = &cd; rr.ok = 1;
        rle8_walk( rd + m.off, rn - m.off, m.W, m.H, rr );
        if( !rr.ok ) { fprintf(stderr,"%s: does not decode against this occurrence table\n",fbmp); goto done; }
      } else {
        RleRemap rr; rr.s = rd + m.off; rr.map = map;
        rle8_walk( rd + m.off, rn - m.off, m.W, m.H, rr );
      }
    } else for( int y=0; y<m.H; y++ ) {
      byte* r = rd + m.off + (qword)y*m.stride;
      for( int x=0; x<m.W; x++ ) {
        if( c.excl ) {
          int rk = r[x];
          if( rk >= cd.alive ) { fprintf(stderr,"%s: does not decode against this occurrence table\n",fbmp); goto done; }
          int j = cd.from_rank( rk );
          r[x] = byte( c.sym[ inv[j] ] );
          if( !cd.take( j, 1 ) ) { fprintf(stderr,"%s: does not decode against this occurrence table\n",fbmp); goto done; }
        } else r[x] = byte(map[r[x]]);
      }
    }
  } else {
    if( (qword)m.W != c.W || (qword)m.H != c.H ) { fprintf(stderr,"%s: geometry does not match the palette\n",fbmp); goto done; }
    int* idx = read_index_bmp( m, c.K );
    if( !idx ) { fprintf( stderr, "%s: not the index image this palette describes\n", fbmp ); goto done; }
    qword padrow = c.H ? c.padlen / c.H : 0;
    qword rowb = c.W*c.npx + padrow;
    rn = c.hdrlen + rowb*c.H + c.taillen;
    rd = new byte[rn];
    memcpy( rd, c.hdr, (size_t)c.hdrlen );
    for( qword y=0; y<c.H; y++ ) {
      byte* r = rd + c.hdrlen + y*rowb;
      const int* s = idx + y*c.W;
      for( qword x=0; x<c.W; x++ ) {
        int j = s[x];
        if( c.excl ) {
          if( j >= cd.alive ) { delete[] idx; fprintf(stderr,"%s: does not decode against this occurrence table\n",fbmp); goto done; }
          j = cd.from_rank( j );
          if( !cd.take( j, 1 ) ) { delete[] idx; fprintf(stderr,"%s: does not decode against this occurrence table\n",fbmp); goto done; }
        }
        qword v = c.sym[ inv[j] ];
        for( qword p=0; p<c.npx; p++ ) r[x*c.npx+p] = byte(v >> (8*p));
      }
      memcpy( r + c.W*c.npx, c.pad + y*padrow, (size_t)padrow );
    }
    memcpy( rd + c.hdrlen + rowb*c.H, c.tail, (size_t)c.taillen );
    delete[] idx;
  }

  // the counts are not needed to restore anything; when they are present they
  // are checked, because a mismatch means the wrong file was named
  if( c.excl ) {
    if( !cd.spent() ) {
      fprintf( stderr, "%s: the occurrence table was not used up by the index image\n", fpal );
      goto done;
    }
  } else if( c.has_counts || ffrq ) {
    have = new qword[K];
    for( int j=0; j<K; j++ ) have[j] = 0;
    if( c.mode == 0 ) {
      qword* raw = new qword[(qword)m.W*m.H];
      if( m.comp == 1 ) rle8_image( m.d + m.off, m.n - m.off, m.W, m.H, raw );
      else for( int y=0; y<m.H; y++ ) {
        const byte* r = m.d + m.off + (qword)y*m.stride;
        for( int x=0; x<m.W; x++ ) raw[(qword)y*m.W+x] = r[x];
      }
      for( qword i=0; i<(qword)m.W*m.H; i++ ) if( raw[i] != ~0ull && raw[i] < (qword)K ) have[raw[i]]++;
      delete[] raw;
    } else {
      int* idx = read_index_bmp( m, c.K );
      if( idx ) { for( qword i=0; i<c.W*c.H; i++ ) have[idx[i]]++; delete[] idx; }
    }
  }
  if( c.has_counts && !c.excl ) {
    for( int j=0; j<K; j++ ) if( c.cnt[j] != have[j] ) {
      fprintf( stderr, "%s: the coded occurrence table disagrees with the index image\n", fpal );
      goto done;
    }
  }
  if( ffrq && !c.excl ) {
    qword fn2; byte* fb = slurp( ffrq, &fn2 );
    if( !fb ) goto done;
    int bad = 0;
    if( fn2 != (qword)K*4 ) {
      fprintf(stderr,"%s: %llu bytes, expected %d entries\n",ffrq,(unsigned long long)fn2,K); bad = 1;
    } else for( int j=0; j<K; j++ )
      if( rd32(fb+4*j) != (uint)(have[j] > 0xFFFFFFFFull ? 0xFFFFFFFFull : have[j]) ) {
        fprintf( stderr, "%s: entry %d disagrees with the index image\n", ffrq, j ); bad = 1; break;
      }
    delete[] fb;
    if( bad ) goto done;
  }

  if( !spew( fout, rd, rn ) ) goto done;
  if( !quiet )
    fprintf( stderr, "%s: %llu distinct pixels, %llu bytes restored\n",
             fout, (unsigned long long)c.K, (unsigned long long)rn );
  rv = 1;
done:
  cd.free_();
  delete[] rd; delete[] pos; delete[] inv; delete[] have; delete[] cnt; delete[] m.d;
  c.free_();
  return rv;
}

// ---------------------------------------------------------------- main

static void usage( void ) {
  printf(
  "bmppal -- split a bitmap into an index image, a palette and its counts\n"
  "\n"
  "  bmppal [-qx] c input.bmp out.bmp out.pal [out.frq]\n"
  "  bmppal [-q]  d out.bmp out.pal [out.frq] restored.bmp\n"
  "\n"
  "  -q        no report on stderr\n"
  "  -x        no countdown: index by a fixed number per colour, and leave the\n"
  "            occurrence table as something nothing reads\n"
  "  out.bmp   the index image: every pixel replaced by its palette index,\n"
  "            8bpp with a grey ramp while the palette fits in a byte, 24 or\n"
  "            32bpp packed little-endian above that.  Hand it to an image\n"
  "            coder -- bmppal does not compress it.\n"
  "  out.pal   the palette, the ordering, and the container record that makes\n"
  "            the restore byte exact.  Range coded.  Carries the occurrence\n"
  "            counts as well unless out.frq is named.\n"
  "  out.frq   the occurrence counts, uncompressed: one little-endian uint32\n"
  "            per palette entry, in index order.\n"
  "\n"
  "By default a pixel's index is its colour's rank among the colours that have\n"
  "not yet been used up, so a colour leaves the alphabet when its count reaches\n"
  "zero and the tail of the image codes against a smaller and smaller alphabet.\n"
  "That makes the occurrence table part of the code rather than a note about it,\n"
  "and the decoder needs it.  -x turns it off.\n"
  "\n"
  "Reads 8bpp BI_RGB and BI_RLE8, 24bpp BI_RGB, and 32bpp BI_RGB and\n"
  "BI_BITFIELDS.  `d` reproduces the input byte for byte.\n"
  "\n"
  "The palette order is searched, because it is worth up to 44%% of the coded\n"
  "size of the index image; see PALETTE.md.\n" );
  exit(1);
}

int main( int argc, char** argv ) {
  int quiet = 0, excl = 1, i = 1;
  for( ; i < argc && argv[i][0]=='-' && argv[i][1] && !argv[i][2]; i++ ) {
    if( argv[i][1] == 'q' ) quiet = 1;
    else if( argv[i][1] == 'x' ) excl = 0;
    else usage();
  }
  int n = argc - i;
  if( n != 4 && n != 5 ) usage();
  char m = argv[i][0];
  if( argv[i][1] || (m != 'c' && m != 'd') ) usage();
  char** a = argv + i + 1;
  int ok;
  if( m == 'c' ) ok = do_compress( a[0], a[1], a[2], n==5 ? a[3] : 0, quiet, excl );
  else           ok = do_restore ( a[0], a[1], n==5 ? a[2] : 0, n==5 ? a[3] : a[2], quiet );
  return ok ? 0 : 1;
}
