// bmgstat -- what is in an RGB/RGBA bitmap that a compressor could exploit.
//
//   bmgstat [options] file.bmp [file.bmp ...]
//
// Every number it prints is meant to answer one question: is there structure
// here that the coder is not already using, and how many bits is it worth?  So
// the report is in bits wherever a saving can be named, and where a property
// holds only *almost* everywhere it says how many exceptions there are and what
// it would cost to send them separately -- because "almost constant" is the
// interesting case and an exact test would throw it away.
//
// Accepts 24- and 32-bit BI_RGB (and 32-bit BI_BITFIELDS, whose masks change
// how a pixel is read but not how its bytes are laid out), and 8-bit paletted
// images -- BI_RGB or BI_RLE8 -- which it expands through the palette so the
// same questions can be asked of them.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long long qword;

// ---------------------------------------------------------------- options

static double opt_tol   = 100.0/1000000.0;  // exceptions allowed, as a fraction
static int    opt_top   = 8;                // colours listed in the palette section
static int    opt_quiet = 0;

// ---------------------------------------------------------------- helpers



static double log2_(double x) { return log(x) * 1.4426950408889634; }

// Order-0 entropy of a histogram, in bits per symbol.
static double hist_entropy( const qword* h, int n, qword tot ) {
  if( !tot ) return 0;
  double s = 0;
  for( int i=0; i<n; i++ ) if( h[i] ) { double p = (double)h[i]/tot; s -= p*log2_(p); }
  return s;
}

// log2 of C(n,k) -- what it costs to say *which* k of n samples are the
// exceptions, which is the whole cost of a property that holds almost
// everywhere.
static double log2_choose( qword n, qword k ) {
  if( k == 0 || k >= n ) return 0;
  return (lgamma((double)n+1) - lgamma((double)k+1) - lgamma((double)(n-k)+1))
         * 1.4426950408889634;
}

static uint gcd_( uint a, uint b ) { while( b ) { uint t = a % b; a = b; b = t; } return a; }

static void hr( const char* title ) { printf( "\n%s\n", title ); }

// ---------------------------------------------------------------- hash map
//
// One open-addressed table serves every grouping question below.  Keys are at
// most 32 bits (a packed pixel, or a packed subset of one), so ~0ull is free to
// mean empty.

struct HMap {
  qword* key;
  qword* val;
  qword  mask, used;

  void init( qword bits ) {
    if( bits < 8 ) bits = 8;
    if( bits > 27 ) bits = 27;
    qword n = 1ull << bits;
    key = new qword[n];
    val = new qword[n];
    for( qword i=0; i<n; i++ ) key[i] = ~0ull;
    memset( val, 0, n*sizeof(qword) );
    mask = n-1; used = 0;
  }
  void free_( void ) { delete[] key; delete[] val; key = 0; val = 0; }

  static qword mix( qword k ) {
    k *= 0x9E3779B97F4A7C15ull;
    k ^= k >> 29;
    k *= 0xBF58476D1CE4E5B9ull;
    k ^= k >> 32;
    return k;
  }
  // Never grows: the caller sizes it from the pixel count, and the load factor
  // is bounded by that.  If it ever did fill, probing would not terminate, so
  // the size is one thing worth getting right rather than checking for.
  qword* at( qword k ) {
    qword i = mix(k) & mask;
    for(;;) {
      if( key[i] == k ) return &val[i];
      if( key[i] == ~0ull ) { key[i] = k; used++; return &val[i]; }
      i = (i+1) & mask;
    }
  }
};

static qword bits_for( qword n ) {          // smallest table above 2x n entries
  qword b = 8;
  while( (1ull<<b) < 2*n + 16 ) b++;
  return b;
}

// ---------------------------------------------------------------- BMP input

struct Image {
  int W, H, NC;              // NC = 3 (B,G,R) or 4 (B,G,R,A)
  byte* px;                  // W*H*NC, top-down, file component order
  const char* note;          // how it was read, for the header line
  uint fsize, bpp, comp;
  int topdown;
  int expanded;              // came through a palette
  int npal;
};

static uint rd16( const byte* p ) { return p[0] | (p[1]<<8); }
static uint rd32( const byte* p ) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint)p[3]<<24); }

static int rle8_decode( const byte* s, uint len, byte* img, int W, int H ) {
  memset( img, 0, (qword)W*H );
  uint i = 0;  int x = 0, row = 0;
  for(;;) {
    if( i+2 > len ) return 1;
    uint n = s[i], v = s[i+1];
    if( n ) {
      if( row >= H || x + (int)n > W ) return 0;
      memset( img + (qword)(H-1-row)*W + x, v, n );
      x += n; i += 2;
    } else if( v == 0 ) { x = 0; row++; i += 2; }
    else if( v == 1 ) return 1;
    else if( v == 2 ) {
      if( i+4 > len ) return 0;
      x += s[i+2]; row += s[i+3]; i += 4;
      if( x > W || row > H ) return 0;
    } else {
      uint pad = (v+1) & ~1u;
      if( i+2+pad > len ) return 0;
      if( row >= H || x + (int)v > W ) return 0;
      memcpy( img + (qword)(H-1-row)*W + x, s+i+2, v );
      x += v; i += 2 + pad;
    }
  }
}

static int load_bmp( const char* fn, Image* im ) {
  FILE* f = fopen( fn, "rb" );
  if( !f ) { fprintf( stderr, "%s: cannot open\n", fn ); return 0; }
  fseek( f, 0, SEEK_END );  long L = ftell(f);  fseek( f, 0, SEEK_SET );
  if( L < 54 ) { fclose(f); fprintf(stderr,"%s: too short\n",fn); return 0; }
  byte* d = new byte[L];
  if( (long)fread( d, 1, L, f ) != L ) { fclose(f); delete[] d; return 0; }
  fclose(f);

  memset( im, 0, sizeof(*im) );
  im->fsize = (uint)L;
  if( d[0]!='B' || d[1]!='M' ) { fprintf(stderr,"%s: not a BMP\n",fn); delete[] d; return 0; }
  uint off = rd32(d+10), dib = rd32(d+14);
  if( dib < 40 || 14+dib > (uint)L ) { fprintf(stderr,"%s: unsupported DIB header\n",fn); delete[] d; return 0; }
  int w = (int)rd32(d+18), sh = (int)rd32(d+22);
  im->bpp = rd16(d+28); im->comp = rd32(d+30);
  uint clr = rd32(d+46);
  im->topdown = (sh < 0);
  int h = im->topdown ? -sh : sh;
  if( w<=0 || h<=0 || rd16(d+26)!=1 ) { fprintf(stderr,"%s: unsupported geometry\n",fn); delete[] d; return 0; }
  im->W = w; im->H = h;

  uint bpp = im->bpp, cp = im->comp;
  if( bpp==24 || bpp==32 ) {
    if( cp!=0 && !(cp==3 && bpp==32) ) { fprintf(stderr,"%s: compression %u unsupported\n",fn,cp); delete[] d; return 0; }
    int nc = bpp>>3;
    uint stride = ((w*bpp + 31)>>5)<<2;
    if( (qword)off + (qword)stride*h > (uint)L ) { fprintf(stderr,"%s: truncated\n",fn); delete[] d; return 0; }
    im->NC = nc;
    im->px = new byte[(qword)w*h*nc];
    for( int r=0; r<h; r++ ) {
      int y = im->topdown ? r : h-1-r;
      memcpy( im->px + (qword)y*w*nc, d + off + (qword)r*stride, (qword)w*nc );
    }
    im->note = (cp==3) ? "BI_BITFIELDS" : "BI_RGB";
  } else if( bpp==8 ) {
    uint ncol = clr ? clr : 256;
    if( ncol > 256 ) ncol = 256;
    uint palo = 14+dib;
    if( palo + 4*ncol > (uint)L ) { fprintf(stderr,"%s: palette missing\n",fn); delete[] d; return 0; }
    byte* idx = new byte[(qword)w*h];
    if( cp==1 ) {
      if( !rle8_decode( d+off, (uint)L-off, idx, w, h ) ) { fprintf(stderr,"%s: bad RLE8\n",fn); delete[] idx; delete[] d; return 0; }
      im->note = "BI_RLE8, expanded through the palette";
    } else if( cp==0 ) {
      uint stride = ((w*8 + 31)>>5)<<2;
      if( (qword)off + (qword)stride*h > (uint)L ) { fprintf(stderr,"%s: truncated\n",fn); delete[] idx; delete[] d; return 0; }
      for( int r=0; r<h; r++ ) {
        int y = im->topdown ? r : h-1-r;
        memcpy( idx + (qword)y*w, d + off + (qword)r*stride, w );
      }
      im->note = "8-bit, expanded through the palette";
    } else { fprintf(stderr,"%s: compression %u unsupported\n",fn,cp); delete[] idx; delete[] d; return 0; }
    im->NC = 3; im->expanded = 1; im->npal = (int)ncol;
    im->px = new byte[(qword)w*h*3];
    for( qword i=0; i<(qword)w*h; i++ ) {
      uint c = idx[i]; if( c >= ncol ) c = ncol-1;
      im->px[3*i+0] = d[palo+4*c+0];
      im->px[3*i+1] = d[palo+4*c+1];
      im->px[3*i+2] = d[palo+4*c+2];
    }
    delete[] idx;
  } else { fprintf(stderr,"%s: %u bpp is not RGB or RGBA\n",fn,bpp); delete[] d; return 0; }

  delete[] d;
  return 1;
}

// ---------------------------------------------------------------- prediction

static int med_( int Wv, int Nv, int NWv ) {
  int mx = Wv>Nv ? Wv : Nv, mn = Wv>Nv ? Nv : Wv;
  if( NWv >= mx ) return mn;
  if( NWv <= mn ) return mx;
  return Wv + Nv - NWv;
}

// MED residuals of one plane, optionally through a value map first.  The map is
// how the "alphabet reduction" question below is asked: a monotone relabelling
// of the used values is lossless and free to describe, and on an image whose
// values sit on a sparse or unevenly spaced grid it makes every residual
// smaller.  Section 8.1 of ALGORITHM_v2.md is BMF doing exactly this.
static void med_hist( const byte* pl, int W, int H, const byte* map, qword* hist ) {
  memset( hist, 0, 256*sizeof(qword) );
  for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
    qword i = (qword)y*W + x;
    int v  = map ? map[pl[i]] : pl[i];
    int Wv = x ? (map?map[pl[i-1]]:pl[i-1])
               : (y ? (map?map[pl[i-W]]:pl[i-W]) : 128);
    int Nv = y ? (map?map[pl[i-W]]:pl[i-W]) : Wv;
    int NW = (x&&y) ? (map?map[pl[i-W-1]]:pl[i-W-1]) : Nv;
    hist[ (v - med_(Wv,Nv,NW)) & 255 ]++;
  }
}

// ---------------------------------------------------------------- per component

struct CompStat {
  qword hist[256];
  int   vmin, vmax, distinct;
  double mean, median, sdev, h0;
  uint  step;                 // all used values are vmin + k*step
  uint  and_all, or_all;      // exact always-1 and ever-1 masks
};

static void comp_stats( const byte* px, int NC, int c, qword N, CompStat* s ) {
  memset( s->hist, 0, sizeof(s->hist) );
  uint aa = 0xFF, oo = 0;
  for( qword i=0; i<N; i++ ) { uint v = px[i*NC+c]; s->hist[v]++; aa &= v; oo |= v; }
  s->and_all = aa; s->or_all = oo;
  s->vmin = -1; s->vmax = -1; s->distinct = 0;
  double sum = 0, sum2 = 0;
  for( int v=0; v<256; v++ ) if( s->hist[v] ) {
    if( s->vmin < 0 ) s->vmin = v;
    s->vmax = v; s->distinct++;
    sum  += (double)v * s->hist[v];
    sum2 += (double)v * v * s->hist[v];
  }
  if( s->vmin < 0 ) { s->vmin = s->vmax = 0; }
  s->mean = N ? sum/N : 0;
  s->sdev = N ? sqrt( sum2/N - s->mean*s->mean > 0 ? sum2/N - s->mean*s->mean : 0 ) : 0;
  qword acc = 0, half = N/2;
  s->median = s->vmin;
  for( int v=0; v<256; v++ ) { acc += s->hist[v]; if( acc > half ) { s->median = v; break; } }
  s->h0 = hist_entropy( s->hist, 256, N );
  uint g = 0;
  for( int v=0; v<256; v++ ) if( s->hist[v] && v != s->vmin ) g = gcd_( g, (uint)(v - s->vmin) );
  s->step = g ? g : 1;
}

// The tone curve implied by the *spacing* of the used values.  If a component
// came from a uniform quantiser through a gamma curve, its i-th smallest used
// value sits at vmin + range*(i/(n-1))^gamma; fitting that exponent says which
// way the values are bunched, and the residual says whether the story holds.
// gamma > 1: bunched toward the low end, the signature of linear-light data.
static double fit_gamma( const CompStat* s, double* rms ) {
  int n = s->distinct;
  *rms = 0;
  if( n < 4 || s->vmax == s->vmin ) return 1.0;
  int* v = new int[n];
  int j = 0;
  for( int t=0; t<256; t++ ) if( s->hist[t] ) v[j++] = t;
  double sxy = 0, sxx = 0, range = s->vmax - s->vmin;
  for( int i=1; i<n-1; i++ ) {
    double x = log( (double)i/(n-1) );
    double y = (v[i]-s->vmin)/range;
    if( y <= 0 ) continue;
    sxy += x*log(y); sxx += x*x;
  }
  double gam = sxx > 0 ? sxy/sxx : 1.0;
  double e = 0;
  for( int i=0; i<n; i++ ) {
    double m = s->vmin + range*pow( (double)i/(n-1), gam );
    e += (m - v[i])*(m - v[i]);
  }
  *rms = sqrt(e/n);
  delete[] v;
  return gam;
}

// ---------------------------------------------------------------- report

static const char* CN[4] = { "B", "G", "R", "A" };

// ---------------------------------------------------------------- findings
//
// The sections below each measure one property and print it in full.  This is
// where they leave whatever they found worth acting on, so the last thing the
// report prints is a list of things to do rather than a wall of numbers.
//
// Every byte figure is measured against the same baseline -- an order-0 coder on
// the component values -- and every one is an upper bound, because a predictive
// model already captures part of most of them.  They rank opportunities; they do
// not predict output size.  Each component contributes at most one
// cross-component entry, the best of the table, the curve and the affine fit, so
// three readings of the same redundancy are not counted three times.

struct Findings {
  double bit_bytes;         int    bit_planes;
  double remap_bytes[4];    int    remap_to[4];
  double palette_bytes;     qword  palette_n;
  double cross_bytes[4];    char   cross_how[4][100];
  double ctrans_bytes;      char   ctrans_how[100];
  double rowrep_bytes;      qword  rowrep_n;
  double alpha_bytes;       char   alpha_how[100];
  char   note[16][120];     int    nnote;
};
static Findings F;

static void note_( const char* fmt, ... ) {
  if( F.nnote >= 16 ) return;
  va_list ap; va_start( ap, fmt );
  vsnprintf( F.note[F.nnote], sizeof F.note[0], fmt, ap );
  va_end( ap );
  F.nnote++;
}

// Claim the cross-component slot for component c if this reading beats whatever
// is already in it.
static void cross_( int c, double bytes, const char* fmt, ... ) {
  if( bytes <= F.cross_bytes[c] ) return;
  F.cross_bytes[c] = bytes;
  va_list ap; va_start( ap, fmt );
  vsnprintf( F.cross_how[c], sizeof F.cross_how[0], fmt, ap );
  va_end( ap );
}

static void report_components( const Image* im, const CompStat* cs, qword N ) {
  hr( "component statistics" );
  printf( "  %-4s %4s %4s %9s %7s %8s %9s %9s %6s\n",
          "comp","min","max","mean","median","stddev","distinct","H0 bits","step" );
  for( int c=0; c<im->NC; c++ ) {
    const CompStat* s = &cs[c];
    printf( "  %-4s %4d %4d %9.3f %7.0f %8.3f %9d %9.4f %6u%s\n",
            CN[c], s->vmin, s->vmax, s->mean, s->median, s->sdev,
            s->distinct, s->h0, s->step,
            s->distinct==1 ? "   constant" : "" );
  }
  double tot = 0;
  for( int c=0; c<im->NC; c++ ) tot += cs[c].h0;
  printf( "  order-0 total %.4f bits/pixel = %.0f bytes (against %d stored)\n",
          tot, tot*N/8, im->NC*8 );
}

// Per-bit balance.  A bit that is constant is 1 bit per pixel free; a bit that
// is *almost* constant is nearly that, less what it costs to name the pixels
// where it is not -- which is log2(N choose k) and nothing else, because the
// value of an exception to a binary property is implied.
static void report_bits( const Image* im, const CompStat* cs, qword N ) {
  qword budget = (qword)(opt_tol*N + 0.5);
  hr( "bit planes" );
  printf( "  exception budget %llu of %llu pixels (%.0f ppm)\n",
          (unsigned long long)budget, (unsigned long long)N, opt_tol*1e6 );
  if( !opt_quiet )
    printf( "  %-4s %3s %12s %9s  %s\n", "comp","bit","ones","fraction","verdict" );
  int nfree = 0, nconst = 0, nnear = 0;
  double saved = 0;
  for( int c=0; c<im->NC; c++ ) {
    for( int b=7; b>=0; b-- ) {
      qword ones = 0;
      for( int v=0; v<256; v++ ) if( (v>>b)&1 ) ones += cs[c].hist[v];
      qword zeros = N - ones;
      qword minority = ones < zeros ? ones : zeros;
      const char* what = ones < zeros ? "always 0" : "always 1";
      char verdict[96];
      if( minority == 0 ) { snprintf(verdict,sizeof verdict,"%s (exact)",what); nconst++; saved += N; }
      else if( minority <= budget ) {
        double cost = log2_choose( N, minority );
        snprintf(verdict,sizeof verdict,"%s with %llu exceptions, %.0f bits to send them",
                 what, (unsigned long long)minority, cost );
        nnear++; saved += N - cost;
      } else { snprintf(verdict,sizeof verdict,"free"); nfree++; }
      if( !opt_quiet )
        printf( "  %-4s %3d %12llu %9.5f  %s\n",
                CN[c], b, (unsigned long long)ones, (double)ones/N, verdict );
    }
  }
  printf( "  masks:" );
  for( int c=0; c<im->NC; c++ )
    printf( " %s always1=%02X ever1=%02X always0=%02X ",
            CN[c], cs[c].and_all, cs[c].or_all, (~cs[c].or_all)&0xFF );
  printf( "\n  %d exact, %d within budget, %d free -- %.0f bits (%.0f bytes) of the %llu-bit planes are not information\n",
          nconst, nnear, nfree, saved, saved/8, (unsigned long long)(N*8*im->NC) );
  F.bit_bytes = saved/8; F.bit_planes = nconst + nnear;
}

// What the value *set* looks like, and whether relabelling it would pay.  The
// last two columns are the actionable ones: if the remapped MED residual is
// meaningfully smaller, the image is sitting on a sparse or unevenly spaced
// grid and a coder should be reducing its alphabet before predicting.
static void report_toneset( const Image* im, const CompStat* cs, const byte* const* plane,
                            qword N ) {
  hr( "value set and tone curve" );
  printf( "  %-4s %9s %8s %8s %8s %8s %11s %11s %s\n",
          "comp","distinct","min gap","max gap","gamma","rms","MED H0","remapped","gain" );
  qword h[256];
  for( int c=0; c<im->NC; c++ ) {
    const CompStat* s = &cs[c];
    int prev = -1, gmin = 256, gmax = 0;
    for( int v=0; v<256; v++ ) if( s->hist[v] ) {
      if( prev >= 0 ) { int g = v-prev; if( g<gmin ) gmin=g; if( g>gmax ) gmax=g; }
      prev = v;
    }
    if( gmin > 255 ) { gmin = 0; gmax = 0; }
    double rms, gam = fit_gamma( s, &rms );

    byte map[256];
    int r = 0;
    for( int v=0; v<256; v++ ) { map[v] = (byte)r; if( s->hist[v] ) r++; }
    med_hist( plane[c], im->W, im->H, 0, h );
    double e0 = hist_entropy( h, 256, N );
    med_hist( plane[c], im->W, im->H, map, h );
    double e1 = hist_entropy( h, 256, N );
    printf( "  %-4s %9d %8d %8d %8.3f %8.2f %11.4f %11.4f %+.4f\n",
            CN[c], s->distinct, gmin, gmax, gam, rms, e0, e1, e1-e0 );
    if( e0 - e1 > 0.005 ) { F.remap_bytes[c] = (e0-e1)*N/8; F.remap_to[c] = s->distinct; }
  }
  printf( "  gamma is fitted to the spacing of the used values, not to the image:\n"
          "  1.0 is even spacing, above 1 bunched at the low end, below 1 at the high end.\n" );
}

// ---------------------------------------------------------------- unique pixels

struct Uniq {
  qword* key;   // packed pixel
  qword* cnt;
  qword  n;
};

static void build_uniq( const Image* im, qword N, Uniq* u ) {
  HMap m; m.init( bits_for( N < (1ull<<24) ? N : (1ull<<24) ) );
  const byte* p = im->px;
  int nc = im->NC;
  for( qword i=0; i<N; i++ ) {
    qword k = p[i*nc] | ((qword)p[i*nc+1]<<8) | ((qword)p[i*nc+2]<<16);
    if( nc==4 ) k |= (qword)p[i*nc+3]<<24;
    (*m.at(k))++;
  }
  u->n = m.used;
  u->key = new qword[u->n ? u->n : 1];
  u->cnt = new qword[u->n ? u->n : 1];
  qword j = 0;
  for( qword i=0; i<=m.mask; i++ ) if( m.key[i] != ~0ull ) { u->key[j] = m.key[i]; u->cnt[j] = m.val[i]; j++; }
  m.free_();
}

static void report_uniq( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  hr( "unique pixels" );
  // distinct colours ignoring alpha, if there is one
  qword nrgb = u->n;
  if( im->NC == 4 ) {
    HMap m; m.init( bits_for( u->n ) );
    for( qword i=0; i<u->n; i++ ) (*m.at( u->key[i] & 0xFFFFFFull )) += u->cnt[i];
    nrgb = m.used; m.free_();
  }
  printf( "  distinct pixels      %llu  (%.4f per pixel, %.2f bits if named directly)\n",
          (unsigned long long)u->n, (double)u->n/N, log2_((double)(u->n?u->n:1)) );
  if( im->NC == 4 )
    printf( "  distinct RGB         %llu  (alpha adds %llu distinct pixels)\n",
            (unsigned long long)nrgb, (unsigned long long)(u->n - nrgb) );
  printf( "  palettizable         %s\n",
          u->n <= 256 ? "yes, 8 bits per pixel plus the palette"
                      : (nrgb <= 256 ? "RGB yes, RGBA no" : "no") );
  { double base = 0;
    for( int c=0; c<im->NC; c++ ) base += cs[c].h0;
    double idx = log2_( (double)(u->n?u->n:1) );
    double save = (base - idx)*N/8 - (double)u->n*im->NC;
    if( u->n <= 4096 && save > 0 ) { F.palette_bytes = save; F.palette_n = u->n; }
    if( im->NC == 4 && u->n > 256 && nrgb <= 256 )
      note_( "RGB is palettizable (%llu colours) even though RGBA is not -- "
             "index and alpha are two planes, not four", (unsigned long long)nrgb );
  }
  // the top few, by coverage -- one pass keeping an ordered shortlist
  int top = opt_top;
  if( (qword)top > u->n ) top = (int)u->n;
  qword shown = 0;
  qword* bidx = new qword[top ? top : 1];
  qword* bcnt = new qword[top ? top : 1];
  for( int t=0; t<top; t++ ) { bidx[t]=0; bcnt[t]=0; }
  for( qword i=0; i<u->n; i++ ) {
    qword c = u->cnt[i];
    if( top && c > bcnt[top-1] ) {
      int t = top-1;
      while( t>0 && bcnt[t-1] < c ) { bcnt[t]=bcnt[t-1]; bidx[t]=bidx[t-1]; t--; }
      bcnt[t]=c; bidx[t]=i;
    }
  }
  printf( "  most frequent:" );
  for( int t=0; t<top; t++ ) {
    if( !bcnt[t] ) break;
    qword k = u->key[bidx[t]];
    printf( "%s %02X%02X%02X", t?",":"", (uint)((k>>16)&255), (uint)((k>>8)&255), (uint)(k&255) );
    if( im->NC==4 ) printf( "%02X", (uint)((k>>24)&255) );
    printf( "*%.3f%%", 100.0*bcnt[t]/N );
    shown += bcnt[t];
  }
  printf( "\n  top %d cover %.3f%% of the image\n", top, 100.0*shown/N );
  delete[] bidx; delete[] bcnt;
}

// ---------------------------------------------------------------- derivability
//
// Is component T a function of some of the others?  Build the counts table the
// question asks for -- for every combination of the predictors, how often each
// T value occurs -- and read two numbers off it: how many combinations there
// are (the size of the lookup table you would have to send) and how many pixels
// disagree with the most common T for their combination (the exceptions).
//
// Both matter.  A component that is perfectly derivable from a key with half a
// million distinct values has not been compressed, it has been moved.  So the
// verdict is in bits: what the plane costs on its own against what the table
// plus the exceptions would cost.

struct DerivResult {
  qword keys, exceptions;
  double table_bits, exc_bits, plane_bits, cond_bits;
};

static void derive_test( const Uniq* u, int nc, int target, int predmask,
                         qword N, double plane_h0, int t_distinct, DerivResult* r ) {
  HMap h1; h1.init( bits_for( u->n ) );
  for( qword i=0; i<u->n; i++ ) {
    qword k = u->key[i];
    qword pk = 0;
    for( int c=0; c<nc; c++ ) if( predmask & (1<<c) ) pk = (pk<<8) | ((k>>(8*c))&255);
    qword tv = (k>>(8*target))&255;
    *h1.at( (pk<<8) | tv ) += u->cnt[i];
  }
  HMap h2; h2.init( bits_for( h1.used ) );
  for( qword i=0; i<=h1.mask; i++ ) if( h1.key[i] != ~0ull ) {
    qword pk = h1.key[i] >> 8, c = h1.val[i];
    qword* v = h2.at( pk );
    qword mx = (*v)>>32, tot = (*v)&0xFFFFFFFFull;
    if( c > mx ) mx = c;
    tot += c;
    *v = (mx<<32) | tot;
  }
  qword exc = 0;
  double hs = 0;
  for( qword i=0; i<=h2.mask; i++ ) if( h2.key[i] != ~0ull ) {
    qword mx = h2.val[i]>>32, tot = h2.val[i]&0xFFFFFFFFull;
    exc += tot - mx;
    double p = (double)tot/N; hs -= p*log2_(p);
  }
  // H(T|S) = H(T,S) - H(S): how many bits T still costs once the predictors are
  // known, which is the floor for *any* way of reading T out of them -- a table,
  // a curve, a formula.  The exception count answers a yes/no question; this
  // answers the same question by degree, and it is the one to look at when the
  // answer is no.
  double hts = 0;
  for( qword i=0; i<=h1.mask; i++ ) if( h1.key[i] != ~0ull ) {
    double p = (double)h1.val[i]/N; hts -= p*log2_(p);
  }
  r->cond_bits = hts - hs;
  if( r->cond_bits < 0 ) r->cond_bits = 0;
  r->keys = h2.used;
  r->exceptions = exc;
  r->table_bits = (double)r->keys * 8.0;
  r->exc_bits   = log2_choose( N, exc ) + (double)exc * log2_( t_distinct>1 ? t_distinct : 2 );
  r->plane_bits = plane_h0 * N;
  h1.free_(); h2.free_();
}

static void report_derive( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  qword budget = (qword)(opt_tol*N + 0.5);
  hr( "cross-component derivability" );
  printf( "  can each component be read out of a table indexed by the others?\n" );
  printf( "  %-6s %-8s %9s %11s %9s %11s %10s  %s\n",
          "target","from","keys","exceptions","H(T|S)","table+exc","plane H0","verdict" );
  int nc = im->NC;
  for( int t=0; t<nc; t++ ) {
    for( int m=1; m<(1<<nc); m++ ) {
      if( m & (1<<t) ) continue;
      char from[16]; from[0] = 0;
      for( int c=0; c<nc; c++ ) if( m & (1<<c) ) { if(from[0]) strcat(from,","); strcat(from,CN[c]); }
      DerivResult r;
      derive_test( u, nc, t, m, N, cs[t].h0, cs[t].distinct, &r );
      double cost = r.table_bits + r.exc_bits;
      const char* verdict;
      char buf[64];
      if( r.exceptions == 0 ) verdict = "exact";
      else if( r.exceptions <= budget ) { snprintf(buf,sizeof buf,"within budget"); verdict = buf; }
      else verdict = "no";
      printf( "  %-6s %-8s %9llu %11llu %9.4f %11.0f %10.0f  %s%s\n",
              CN[t], from, (unsigned long long)r.keys, (unsigned long long)r.exceptions,
              r.cond_bits, cost/8, r.plane_bits/8, verdict,
              cost < r.plane_bits ? ", cheaper than the plane" : "" );
      if( cost < r.plane_bits )
        cross_( t, (r.plane_bits - cost)/8,
                "read it from a %llu-row table indexed by %s (%llu exceptions)",
                (unsigned long long)r.keys, from, (unsigned long long)r.exceptions );
    }
  }
  printf( "  keys is how many rows the lookup table would have; H(T|S) is bits per pixel\n"
          "  and is the floor for any way of reading T out of S; table+exc and plane H0\n"
          "  are bytes, so those two are the trade the verdict is about.\n" );
}

// ---------------------------------------------------------------- relations

// The cheap exact questions, asked over the unique table so the cost is in the
// number of distinct pixels rather than the number of pixels.
static void report_relations( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  qword budget = (qword)(opt_tol*N + 0.5);
  int nc = im->NC;
  hr( "exact relations" );
  int printed = 0;
  for( int a=0; a<nc; a++ ) {
    if( cs[a].distinct == 1 ) {
      printf( "  %s is constant %d -- %llu bits free\n", CN[a], cs[a].vmin,
              (unsigned long long)(N*8) );
      note_( "%s is constant %d: drop the plane, send one byte", CN[a], cs[a].vmin );
      printed++;
    }
    for( int b=a+1; b<nc; b++ ) {
      qword ne = 0, nc2 = 0;
      for( qword i=0; i<u->n; i++ ) {
        int va = (int)((u->key[i]>>(8*a))&255), vb = (int)((u->key[i]>>(8*b))&255);
        if( va != vb ) ne += u->cnt[i];
        if( va != 255-vb ) nc2 += u->cnt[i];
      }
      if( ne <= budget ) {
        printf( "  %s == %s%s  (%llu exceptions, %.0f bits to send them)\n",
                CN[a], CN[b], ne?"":" exactly", (unsigned long long)ne, log2_choose(N,ne) );
        cross_( b, (cs[b].h0*N - log2_choose(N,ne) - (double)ne*8)/8,
                "it equals %s (%llu exceptions)", CN[a], (unsigned long long)ne );
        printed++;
      }
      if( nc2 <= budget ) {
        printf( "  %s == 255-%s%s  (%llu exceptions)\n",
                CN[a], CN[b], nc2?"":" exactly", (unsigned long long)nc2 );
        cross_( b, (cs[b].h0*N - log2_choose(N,nc2) - (double)nc2*8)/8,
                "it equals 255-%s (%llu exceptions)", CN[a], (unsigned long long)nc2 );
        printed++;
      }
    }
  }
  if( !printed ) printf( "  none within budget\n" );
  { qword grey = 0;
    for( qword i=0; i<u->n; i++ ) {
      uint b = (uint)(u->key[i]&255), g = (uint)((u->key[i]>>8)&255), r = (uint)((u->key[i]>>16)&255);
      if( b!=g || g!=r ) grey += u->cnt[i];
    }
    if( grey <= budget )
      note_( "the colour is greyscale%s -- two of the three planes are a copy",
             grey ? " everywhere but a few pixels" : "" );
  }
}

// Weighted least squares over the unique table: the best affine reading of one
// component from the others, and what its residual actually costs.  This is the
// continuous cousin of the table above -- where that asks whether T is a
// *function* of the others, this asks whether it is a *linear* function, which
// is the form a colour transform can use without carrying a table at all.
static void report_affine( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  int nc = im->NC;
  hr( "affine fits (weighted least squares over distinct pixels)" );
  printf( "  %-6s %-34s %11s %8s %10s\n", "target","fit","residual H0","max |r|","nonzero" );
  for( int t=0; t<nc; t++ ) {
    double bestH = 1e300; char bestf[64] = "";
    qword bestmax = 0, bestnz = 0;
    for( int m=1; m<(1<<nc); m++ ) {
      if( m & (1<<t) ) continue;
      int idx[4], k = 0;
      for( int c=0; c<nc; c++ ) if( m & (1<<c) ) idx[k++] = c;
      // normal equations, k+1 unknowns
      double A[4][5]; memset( A, 0, sizeof(A) );
      for( qword i=0; i<u->n; i++ ) {
        double w = (double)u->cnt[i];
        double x[4];
        for( int j=0; j<k; j++ ) x[j] = (double)((u->key[i]>>(8*idx[j]))&255);
        x[k] = 1.0;
        double y = (double)((u->key[i]>>(8*t))&255);
        for( int r2=0; r2<=k; r2++ ) {
          for( int c2=0; c2<=k; c2++ ) A[r2][c2] += w*x[r2]*x[c2];
          A[r2][k+1] += w*x[r2]*y;
        }
      }
      int n2 = k+1;
      double sol[4] = {0,0,0,0};
      { // Gaussian elimination with partial pivoting
        int sing = 0;
        for( int i=0; i<n2; i++ ) {
          int piv = i;
          for( int r2=i+1; r2<n2; r2++ ) if( fabs(A[r2][i]) > fabs(A[piv][i]) ) piv = r2;
          if( fabs(A[piv][i]) < 1e-9 ) { sing = 1; break; }
          if( piv != i ) for( int c2=0; c2<=n2; c2++ ) { double q=A[i][c2]; A[i][c2]=A[piv][c2]; A[piv][c2]=q; }
          for( int r2=i+1; r2<n2; r2++ ) {
            double f = A[r2][i]/A[i][i];
            for( int c2=i; c2<=n2; c2++ ) A[r2][c2] -= f*A[i][c2];
          }
        }
        if( !sing ) for( int i=n2-1; i>=0; i-- ) {
          double s = A[i][n2];
          for( int c2=i+1; c2<n2; c2++ ) s -= A[i][c2]*sol[c2];
          sol[i] = s/A[i][i];
        }
      }
      qword h[256]; memset( h, 0, sizeof(h) );
      qword mx = 0, nz = 0;
      for( qword i=0; i<u->n; i++ ) {
        double p = sol[k];
        for( int j=0; j<k; j++ ) p += sol[j]*(double)((u->key[i]>>(8*idx[j]))&255);
        int y = (int)((u->key[i]>>(8*t))&255);
        int r2 = y - (int)floor(p+0.5);
        int f = ((r2+128)&255)-128;
        h[(f)&255] += u->cnt[i];
        qword a2 = (qword)(f<0?-f:f);
        if( a2 > mx ) mx = a2;
        if( f ) nz += u->cnt[i];
      }
      double H = hist_entropy( h, 256, N );
      if( H < bestH ) {
        bestH = H; bestmax = mx; bestnz = nz;
        if( k==1 ) snprintf( bestf, sizeof bestf, "%.4f*%s %+.2f", sol[0], CN[idx[0]], sol[1] );
        else if( k==2 ) snprintf( bestf, sizeof bestf, "%.4f*%s %+.4f*%s %+.2f",
                                  sol[0], CN[idx[0]], sol[1], CN[idx[1]], sol[2] );
        else snprintf( bestf, sizeof bestf, "%.4f*%s %+.4f*%s %+.4f*%s %+.2f",
                       sol[0], CN[idx[0]], sol[1], CN[idx[1]], sol[2], CN[idx[2]], sol[3] );
      }
    }
    printf( "  %-6s %-34s %11.4f %8llu %10llu\n",
            CN[t], bestf, bestH, (unsigned long long)bestmax, (unsigned long long)bestnz );
    printf( "        against %.4f bits standing alone -- %+.4f bits/pixel\n",
            cs[t].h0, bestH - cs[t].h0 );
    if( bestH < cs[t].h0 )
      cross_( t, (cs[t].h0 - bestH)*N/8, "subtract the affine fit %s", bestf );
  }
}

// ---------------------------------------------------------------- tone maps

// The pairwise, continuous form of the derivability question.  The table above
// asks whether T is a *function* of the other components; this asks how much of
// T a single other component P already tells us, and whether the answer has the
// shape of a curve.
//
//   H(T|P)   what T still costs once P is known -- the floor for reading T out
//            of P by any means whatsoever, table or curve or formula
//   I(T;P)   how much of T's standalone entropy that removes
//   curve    the best deterministic map P->T (the per-column mode): the order-0
//            cost of its residual, how many pixels miss it, and what the map
//            itself costs to ship as a table
//   gamma    the exponent of the power law fitted to the same relation, on each
//            component's own range: T ~ tmin + trange*((P-pmin)/prange)^gamma.
//            This is what a gamma correction between two channels looks like --
//            two numbers instead of a 256-entry table, and worth taking whenever
//            its residual costs about what the table's does.
//   mono     whether the mode curve rises or falls monotonically.  A real tone
//            map does; a scrambled palette index does not, and its low H(T|P)
//            means only that the two components share a small alphabet.
static void report_tonemap( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  int nc = im->NC;
  hr( "pairwise tone maps (T read from one other component P)" );
  printf( "  %-3s %-3s %7s %7s %7s %8s %10s %7s %7s %7s %8s %5s\n",
          "T","P","H(T)","H(T|P)","I(T;P)","curveH0","misses","tab.B","gamma","rms","gammaH0","mono" );

  static qword jh[256][256];    // jh[p][t], summed over the whole image
  double bestc[4]; int bestp[4];
  for( int i=0; i<4; i++ ) { bestc[i] = 1e300; bestp[i] = -1; }

  for( int t=0; t<nc; t++ ) for( int p=0; p<nc; p++ ) {
    if( t == p ) continue;
    memset( jh, 0, sizeof(jh) );
    for( qword i=0; i<u->n; i++ ) {
      int tv = (int)((u->key[i]>>(8*t))&255);
      int pv = (int)((u->key[i]>>(8*p))&255);
      jh[pv][tv] += u->cnt[i];
    }

    // H(T,P) over the joint table, then H(T|P) = H(T,P) - H(P)
    double htp = 0;
    for( int pv=0; pv<256; pv++ ) for( int tv=0; tv<256; tv++ ) if( jh[pv][tv] ) {
      double q = (double)jh[pv][tv]/N; htp -= q*log2_(q);
    }
    double hcond = htp - cs[p].h0;
    if( hcond < 0 ) hcond = 0;                 // rounding only
    double mi = cs[t].h0 - hcond;
    if( hcond < bestc[t] ) { bestc[t] = hcond; bestp[t] = p; }

    // the mode curve, its residual, and whether it is monotone
    int   curve[256]; qword hit = 0, used = 0;
    qword res[256]; memset( res, 0, sizeof(res) );
    for( int pv=0; pv<256; pv++ ) {
      qword best = 0; int arg = -1;
      for( int tv=0; tv<256; tv++ ) if( jh[pv][tv] > best ) { best = jh[pv][tv]; arg = tv; }
      curve[pv] = arg;
      if( arg >= 0 ) { hit += best; used++; }
    }
    for( int pv=0; pv<256; pv++ ) if( curve[pv]>=0 )
      for( int tv=0; tv<256; tv++ ) if( jh[pv][tv] )
        res[(tv - curve[pv]) & 255] += jh[pv][tv];
    double curveH = hist_entropy( res, 256, N );
    int up = 1, dn = 1, prev = -1;
    for( int pv=0; pv<256; pv++ ) if( curve[pv]>=0 ) {
      if( prev>=0 ) { if( curve[pv] < prev ) up = 0; if( curve[pv] > prev ) dn = 0; }
      prev = curve[pv];
    }
    const char* mono = up ? "up" : (dn ? "down" : "no");

    // the power law, fitted in logs on each component's own normalised range
    double prange = cs[p].vmax - cs[p].vmin, trange = cs[t].vmax - cs[t].vmin;
    double gamma = 0, rms = 0, gH = 0;
    int havegamma = ( prange > 0 && trange > 0 );
    if( havegamma ) {
      double sxy = 0, sxx = 0;
      for( int pv=cs[p].vmin; pv<=cs[p].vmax; pv++ ) for( int tv=cs[t].vmin; tv<=cs[t].vmax; tv++ ) {
        qword c = jh[pv][tv]; if( !c ) continue;
        double px = (pv - cs[p].vmin)/prange, ty = (tv - cs[t].vmin)/trange;
        if( px <= 0 || ty <= 0 ) continue;
        double lx = log(px), ly = log(ty);
        sxy += (double)c*lx*ly; sxx += (double)c*lx*lx;
      }
      if( sxx > 0 ) gamma = sxy/sxx; else havegamma = 0;
    }
    if( havegamma ) {
      qword gres[256]; memset( gres, 0, sizeof(gres) );
      double s2 = 0;
      for( int pv=0; pv<256; pv++ ) {
        double px = (pv - cs[p].vmin)/prange;
        double pred = cs[t].vmin;
        if( px > 0 ) pred += trange*pow( px, gamma );
        if( pred < 0 ) pred = 0; else if( pred > 255 ) pred = 255;
        int pi = (int)floor( pred + 0.5 );
        for( int tv=0; tv<256; tv++ ) {
          qword c = jh[pv][tv]; if( !c ) continue;
          double d = tv - pred; s2 += (double)c*d*d;
          gres[(tv - pi) & 255] += c;
        }
      }
      rms = sqrt( s2/N );
      gH  = hist_entropy( gres, 256, N );
    }

    printf( "  %-3s %-3s %7.4f %7.4f %7.4f %8.4f %10llu %7llu",
            CN[t], CN[p], cs[t].h0, hcond, mi, curveH,
            (unsigned long long)(N - hit), (unsigned long long)used );
    if( havegamma ) printf( " %7.4f %7.3f %8.4f", gamma, rms, gH );
    else            printf( " %7s %7s %8s", "-", "-", "-" );
    printf( " %5s\n", mono );
  }

  for( int t=0; t<nc; t++ ) if( bestp[t] >= 0 ) {
    double save = (cs[t].h0 - bestc[t])*N/8;
    printf( "  %s is best explained by %s: %.4f -> %.4f bits/pixel, up to %.0f bytes\n",
            CN[t], CN[bestp[t]], cs[t].h0, bestc[t], save );
    if( save > 0 )
      cross_( t, save, "condition it on %s -- H(%s|%s) = %.4f against %.4f alone",
              CN[bestp[t]], CN[t], CN[bestp[t]], bestc[t], cs[t].h0 );
  }
}

static void report_corr( const Image* im, const Uniq* u, qword N ) {
  int nc = im->NC;
  double s[4] = {0,0,0,0}, ss[4][4];
  memset( ss, 0, sizeof(ss) );
  for( qword i=0; i<u->n; i++ ) {
    double w = (double)u->cnt[i], v[4];
    for( int c=0; c<nc; c++ ) v[c] = (double)((u->key[i]>>(8*c))&255);
    for( int a=0; a<nc; a++ ) { s[a] += w*v[a];
      for( int b=0; b<nc; b++ ) ss[a][b] += w*v[a]*v[b]; }
  }
  hr( "correlation" );
  printf( "       " );
  for( int c=0; c<nc; c++ ) printf( "%8s", CN[c] );
  printf( "\n" );
  for( int a=0; a<nc; a++ ) {
    printf( "  %-4s ", CN[a] );
    for( int b=0; b<nc; b++ ) {
      double ca = ss[a][b]/N - (s[a]/N)*(s[b]/N);
      double va = ss[a][a]/N - (s[a]/N)*(s[a]/N);
      double vb = ss[b][b]/N - (s[b]/N)*(s[b]/N);
      double r = (va>0 && vb>0) ? ca/sqrt(va*vb) : 0;
      printf( "%8.3f", r );
    }
    printf( "\n" );
  }
}

// ---------------------------------------------------------------- spatial

// What a predictive coder would actually see.  The neighbour-equality figures
// say whether a match model has anything to match; the MED residual entropies
// are the floor an order-0 coder would reach on the residuals; and the last
// table is the colour transform question -- for each plane, which other plane
// is worth subtracting, in the value domain, before predicting.
static void report_spatial( const Image* im, const CompStat* cs,
                            const byte* const* plane, qword N ) {
  int W = im->W, H = im->H, nc = im->NC;
  hr( "spatial structure" );

  qword eqW = 0, eqN = 0, eqNW = 0, eqNE = 0, runs = 0, maxrun = 0, run = 0;
  qword prevrow_same = 0;
  for( int y=0; y<H; y++ ) {
    for( int x=0; x<W; x++ ) {
      qword i = (qword)y*W + x;
      int sameW = x>0, sameN = y>0, sameNW = (x>0&&y>0), sameNE = (y>0&&x+1<W);
      for( int c=0; c<nc && (sameW||sameN||sameNW||sameNE); c++ ) {
        byte v = plane[c][i];
        if( sameW  && plane[c][i-1]   != v ) sameW = 0;
        if( sameN  && plane[c][i-W]   != v ) sameN = 0;
        if( sameNW && plane[c][i-W-1] != v ) sameNW = 0;
        if( sameNE && plane[c][i-W+1] != v ) sameNE = 0;
      }
      eqW += sameW; eqN += sameN; eqNW += sameNW; eqNE += sameNE;
      if( x>0 && sameW ) run++; else { if( run+1 > maxrun ) maxrun = run+1; runs++; run = 0; }
    }
    if( y>0 ) {
      int same = 1;
      for( int c=0; c<nc && same; c++ )
        if( memcmp( plane[c] + (qword)y*W, plane[c] + (qword)(y-1)*W, W ) ) same = 0;
      prevrow_same += same;
    }
  }
  printf( "  pixel equals its west neighbour   %8.5f\n", (double)eqW/N );
  printf( "  pixel equals its north neighbour  %8.5f\n", (double)eqN/N );
  printf( "  pixel equals north-west           %8.5f\n", (double)eqNW/N );
  printf( "  pixel equals north-east           %8.5f\n", (double)eqNE/N );
  printf( "  runs of identical pixels          %llu, mean %.2f, longest %llu\n",
          (unsigned long long)runs, (double)N/(runs?runs:1), (unsigned long long)maxrun );
  printf( "  rows identical to the one above   %llu of %d\n",
          (unsigned long long)prevrow_same, H-1 );

  // horizontal period: the lag whose whole-pixel match rate stands out
  int bestlag = 0; double bestrate = 0;
  for( int lag=1; lag<=64 && lag<W; lag++ ) {
    qword m = 0, tot = 0;
    for( int y=0; y<H; y+=(H>512?4:1) ) for( int x=lag; x<W; x++ ) {
      qword i = (qword)y*W + x;
      int same = 1;
      for( int c=0; c<nc && same; c++ ) if( plane[c][i] != plane[c][i-lag] ) same = 0;
      m += same; tot++;
    }
    double r = tot ? (double)m/tot : 0;
    if( lag>1 && r > bestrate ) { bestrate = r; bestlag = lag; }
  }
  if( bestlag ) printf( "  strongest horizontal period       lag %d at %.5f\n", bestlag, bestrate );
  else          printf( "  strongest horizontal period       none\n" );

  qword h[256];
  double tot0 = 0;
  printf( "  MED residual entropy, bits/sample:" );
  for( int c=0; c<nc; c++ ) {
    med_hist( plane[c], W, H, 0, h );
    double e = hist_entropy( h, 256, N );
    tot0 += e;
    printf( "  %s %.4f", CN[c], e );
  }
  printf( "   sum %.4f bits/pixel = %.0f bytes\n", tot0, tot0*N/8 );
  if( prevrow_same ) {
    F.rowrep_bytes = (double)prevrow_same*W*tot0/8;
    F.rowrep_n = prevrow_same;
  }
  if( bestlag && bestrate > 0.25 && bestrate*N > (double)eqW )
    note_( "pixels repeat at lag %d in %.1f%% of positions, against %.1f%% at lag 1 -- "
           "a periodic predictor would see it", bestlag, 100*bestrate, 100.0*eqW/N );

  hr( "value-domain colour transform" );
  printf( "  MED residual entropy of (target - k*reference) & 255, best k in 1/128ths\n" );
  printf( "  %-6s %-9s %6s %11s %11s %s\n", "target","reference","k","H0","alone","gain" );
  byte* diff = new byte[(qword)W*H];
  double alone[4], cost[4][4];
  int bk[4][4];
  memset( bk, 0, sizeof(bk) );
  for( int t=0; t<nc; t++ ) {
    med_hist( plane[t], W, H, 0, h );
    alone[t] = cost[t][t] = hist_entropy( h, 256, N );
    for( int r=0; r<nc; r++ ) {
      if( r == t ) continue;
      cost[t][r] = 1e300;
      for( int k=32; k<=160; k+=32 ) {
        for( qword i=0; i<(qword)W*H; i++ )
          diff[i] = (byte)( (plane[t][i] - ((k*plane[r][i] + 64)>>7)) & 255 );
        med_hist( diff, W, H, 0, h );
        double e = hist_entropy( h, 256, N );
        if( e < cost[t][r] ) { cost[t][r] = e; bk[t][r] = k; }
      }
    }
  }
  delete[] diff;
  for( int t=0; t<nc; t++ ) {
    int bestr = -1;
    for( int r=0; r<nc; r++ ) if( r!=t && cost[t][r] < alone[t] )
      if( bestr < 0 || cost[t][r] < cost[t][bestr] ) bestr = r;
    if( bestr < 0 ) printf( "  %-6s %-9s %6s %11.4f %11.4f %s\n", CN[t], "none", "-", alone[t], alone[t], "-" );
    else printf( "  %-6s %-9s %6d %11.4f %11.4f %+.4f\n",
                 CN[t], CN[bestr], bk[t][bestr], cost[t][bestr], alone[t], cost[t][bestr]-alone[t] );
  }

  // Those rows are chosen one at a time, so together they can be circular --
  // B against G and G against B cannot both be coded.  A transform that a coder
  // could actually run keeps one plane and codes the rest against it, so this
  // is the number to believe.
  int bestbase = 0; double bestsum = 1e300;
  for( int b=0; b<nc; b++ ) {
    double sum = alone[b];
    for( int t=0; t<nc; t++ ) if( t != b )
      sum += cost[t][b] < alone[t] ? cost[t][b] : alone[t];
    if( sum < bestsum ) { bestsum = sum; bestbase = b; }
  }
  char how[100]; how[0] = 0;
  for( int t=0; t<nc; t++ ) if( t != bestbase && cost[t][bestbase] < alone[t] ) {
    char one[32];
    snprintf( one, sizeof one, "%s%s-%d/128*%s", how[0]?", ":"",
              CN[t], bk[t][bestbase], CN[bestbase] );
    strncat( how, one, sizeof(how)-strlen(how)-1 );
  }
  printf( "  realizable: keep %s, code the rest against it -- %.4f bits/pixel = %.0f bytes,\n"
          "              against %.0f untransformed%s%s\n",
          CN[bestbase], bestsum, bestsum*N/8, tot0*N/8,
          how[0] ? "; " : "", how[0] ? how : "no plane gains" );
  if( tot0 - bestsum > 0.005 && how[0] ) {
    F.ctrans_bytes = (tot0 - bestsum)*N/8;
    snprintf( F.ctrans_how, sizeof F.ctrans_how, "%s", how );
  }
}

// ---------------------------------------------------------------- alpha

static void report_alpha( const Image* im, const CompStat* cs, const Uniq* u, qword N ) {
  if( im->NC != 4 ) return;
  hr( "alpha" );
  const CompStat* a = &cs[3];
  printf( "  distinct values      %d  (min %d, max %d)\n", a->distinct, a->vmin, a->vmax );
  printf( "  fully opaque (255)   %.5f\n", (double)a->hist[255]/N );
  printf( "  fully clear (0)      %.5f\n", (double)a->hist[0]/N );
  int binary = (a->distinct <= 2 && (a->hist[0] + a->hist[255]) == N);
  printf( "  shape                %s\n",
          a->distinct==1 ? "constant -- the fourth byte carries nothing"
          : binary ? "binary, a mask rather than a gradient" : "a real gradient" );
  if( binary && a->distinct == 2 )
    note_( "alpha is a two-valued mask -- code it as one bit per pixel, not as a byte" );
  // colour under transparency, and premultiplication
  qword clear_n = 0, clear_black = 0, clear_uni = 0, over = 0;
  qword first = ~0ull;
  for( qword i=0; i<u->n; i++ ) {
    qword k = u->key[i];
    uint av = (uint)((k>>24)&255);
    uint b = (uint)(k&255), g = (uint)((k>>8)&255), r = (uint)((k>>16)&255);
    if( av == 0 ) {
      clear_n += u->cnt[i];
      if( !b && !g && !r ) clear_black += u->cnt[i];
      if( first == ~0ull ) first = k & 0xFFFFFFull;
      if( (k & 0xFFFFFFull) == first ) clear_uni += u->cnt[i];
    }
    if( b > av || g > av || r > av ) over += u->cnt[i];
  }
  if( clear_n ) {
    printf( "  where alpha is 0     %llu pixels; %.5f are black, %.5f share one colour\n",
            (unsigned long long)clear_n, (double)clear_black/clear_n, (double)clear_uni/clear_n );
    if( clear_uni == clear_n ) {
      double save = 0;
      for( int c=0; c<3; c++ ) save += cs[c].h0;
      save = (double)clear_n*save/8;
      printf( "                       colour under full transparency is constant -- %.0f bytes of it are free\n",
              save );
      F.alpha_bytes = save;
      snprintf( F.alpha_how, sizeof F.alpha_how,
                "the %llu fully transparent pixels all carry colour %06llX -- do not code it",
                (unsigned long long)clear_n, (unsigned long long)first );
    }
  }
  printf( "  premultiplied        %s (%llu pixels have a component above alpha)\n",
          over ? "no" : "consistent with it", (unsigned long long)over );
}

// ---------------------------------------------------------------- summary

// Whatever the sections above left in the ledger, biggest first.  Nothing here
// is new measurement -- it is the same numbers, sorted by how much they are
// worth, so the report ends with a decision instead of a table.
static void report_opps( const Image* im, qword N ) {
  struct Row { double bytes; const char* base; char text[160]; } row[24];
  int n = 0;
  #define ROW(b,bs,...) do { if( (b) > 0.5 && n < 24 ) { row[n].bytes = (b); row[n].base = (bs); \
      snprintf( row[n].text, sizeof row[n].text, __VA_ARGS__ ); n++; } } while(0)

  if( F.bit_planes )
    ROW( F.bit_bytes, "raw", "%d of the %d bit planes are constant or nearly so",
         F.bit_planes, im->NC*8 );
  if( F.palette_bytes )
    ROW( F.palette_bytes, "raw", "only %llu distinct pixels -- index them and send a palette",
         (unsigned long long)F.palette_n );
  for( int c=0; c<im->NC; c++ ) if( F.cross_bytes[c] > 0 )
    ROW( F.cross_bytes[c], "H0", "%s: %s", CN[c], F.cross_how[c] );
  for( int c=0; c<im->NC; c++ ) if( F.remap_bytes[c] > 0 )
    ROW( F.remap_bytes[c], "MED", "%s: only %d values are used -- renumber them 0..%d before predicting",
         CN[c], F.remap_to[c], F.remap_to[c]-1 );
  if( F.ctrans_bytes > 0 )
    ROW( F.ctrans_bytes, "MED", "subtract a scaled plane before predicting: %s", F.ctrans_how );
  if( F.rowrep_bytes > 0 )
    ROW( F.rowrep_bytes, "MED", "%llu rows repeat the row above them exactly",
         (unsigned long long)F.rowrep_n );
  if( F.alpha_bytes > 0 )
    ROW( F.alpha_bytes, "H0", "%s", F.alpha_how );
  #undef ROW

  hr( "what to do about it" );
  if( !n && !F.nnote ) {
    printf( "  nothing beyond ordinary prediction -- the image is what it looks like\n" );
    return;
  }
  for( int i=0; i<n; i++ ) for( int j=i+1; j<n; j++ )
    if( row[j].bytes > row[i].bytes ) { Row t = row[i]; row[i] = row[j]; row[j] = t; }
  double raw = (double)N*im->NC;
  for( int i=0; i<n; i++ )
    printf( "  %11.0f bytes  %5.1f%% of raw  vs %-3s  %s\n",
            row[i].bytes, 100.0*row[i].bytes/raw, row[i].base, row[i].text );
  for( int i=0; i<F.nnote; i++ )
    printf( "  %11s                          %s\n", "-", F.note[i] );
  printf( "\n  Each figure is an upper bound against the baseline named in its row, of the\n"
          "  %.0f raw bytes:\n"
          "    raw  the bytes as stored, 8 bits per component\n"
          "    H0   an order-0 coder on that component's values\n"
          "    MED  an order-0 coder on that component's MED residuals\n"
          "  So they do not add up: a constant bit plane is already free under H0, and a\n"
          "  predictive model already captures part of most of the rest.  Entries overlap\n"
          "  wherever two of them describe the same redundancy.\n", raw );
}

// ---------------------------------------------------------------- main

static void usage( void ) {
  printf(
  "bmgstat -- what is in an RGB/RGBA bitmap that a compressor could exploit\n"
  "\n"
  "  bmgstat [options] file.bmp [file.bmp ...]\n"
  "\n"
  "  -t PPM   exceptions allowed before a property stops counting as one,\n"
  "           in parts per million of the pixel count (default 100)\n"
  "  -n N     colours listed in the palette section (default 8)\n"
  "  -q       skip the per-bit table (its summary line stays)\n"
  "\n"
  "Reads 24- and 32-bit BI_RGB and 32-bit BI_BITFIELDS, and 8-bit paletted\n"
  "images (BI_RGB or BI_RLE8), which it expands through the palette.\n" );
  exit(1);
}

static int analyse( const char* fn ) {
  Image im;
  if( !load_bmp( fn, &im ) ) return 0;
  qword N = (qword)im.W * im.H;
  memset( &F, 0, sizeof(F) );

  printf( "\n=== %s\n", fn );
  printf( "  %dx%d  %ubpp  %s  %s  file %u bytes, %llu pixels, %d components (%s)\n",
          im.W, im.H, im.bpp, im.note, im.topdown ? "top-down" : "bottom-up",
          im.fsize, (unsigned long long)N, im.NC,
          im.NC==4 ? "B,G,R,A" : "B,G,R" );
  if( im.expanded )
    printf( "  expanded from a %d-entry palette; the statistics below are of the colours,\n"
            "  not of the indices the file stores\n", im.npal );

  // planes, so every pass below is contiguous
  byte* plane[4] = { 0, 0, 0, 0 };
  for( int c=0; c<im.NC; c++ ) {
    plane[c] = new byte[N];
    for( qword i=0; i<N; i++ ) plane[c][i] = im.px[i*im.NC+c];
  }
  const byte* cplane[4] = { plane[0], plane[1], plane[2], im.NC>3 ? plane[3] : 0 };

  CompStat cs[4];
  for( int c=0; c<im.NC; c++ ) comp_stats( im.px, im.NC, c, N, &cs[c] );

  Uniq u;
  build_uniq( &im, N, &u );

  report_components( &im, cs, N );
  report_bits( &im, cs, N );
  report_toneset( &im, cs, cplane, N );
  report_uniq( &im, cs, &u, N );
  report_derive( &im, cs, &u, N );
  report_relations( &im, cs, &u, N );
  report_affine( &im, cs, &u, N );
  report_tonemap( &im, cs, &u, N );
  report_corr( &im, &u, N );
  report_spatial( &im, cs, cplane, N );
  report_alpha( &im, cs, &u, N );
  report_opps( &im, N );

  delete[] u.key; delete[] u.cnt;
  for( int c=0; c<im.NC; c++ ) delete[] plane[c];
  delete[] im.px;
  return 1;
}

int main( int argc, char** argv ) {
  int nf = 0;
  for( int i=1; i<argc; i++ ) {
    if( argv[i][0]=='-' && argv[i][1] ) {
      char o = argv[i][1];
      if( o=='t' && i+1<argc ) opt_tol = atof(argv[++i])/1e6;
      else if( o=='n' && i+1<argc ) opt_top = atoi(argv[++i]);
      else if( o=='q' ) opt_quiet = 1;
      else usage();
    } else nf++;
  }
  if( !nf ) usage();
  int bad = 0;
  for( int i=1; i<argc; i++ ) {
    if( argv[i][0]=='-' && argv[i][1] ) { char o=argv[i][1]; if( (o=='t'||o=='n') ) i++; continue; }
    if( !analyse( argv[i] ) ) bad++;
  }
  return bad ? 2 : 0;
}
