// ctxscan -- does any candidate context produce lanes whose popcount bias
// exceeds chance by enough to pay for a chunk header?
//
// For a chunk of n bits with k zeros, d = n/2 - k, the enumerative code saves
//     n - log2 C(n,k)  ~=  0.5*log2(pi*n/2) + 2*d^2/(n ln2)
// The first term is paid back by transmitting the popcount, so the term that
// can actually win is the second.  Under the null (iid unbiased bits)
// E[d^2] = n/4, giving 1/(2 ln 2) = 0.721 bits per chunk of pure fluctuation --
// which is what plain CDM already harvests.  So the figure of merit for a
// context is the EXCESS over that null:
//     excess = sum over chunks of 2*(d^2 - n/4)/(n ln2)   bits
// whose own noise floor is about sqrt(N) bits for N chunks.  A context is only
// interesting if excess >> sqrt(N) AND excess/N exceeds the header cost.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
typedef unsigned char byte;

static std::vector<byte> buf;
static int NBITS = 2048;                       // chunk size, in lane bits

// bit i of the file, MSB-first within each byte
static inline int bitat( size_t i ) { return (buf[i>>3] >> (7-(i&7))) & 1; }

struct Ctx { const char* name; int nlanes; int (*lane)( size_t i ); };

// --- context functions: lane id for file bit i (causal: only bits < i) ------
static int c_flat    ( size_t i ){ (void)i; return 0; }                       // plain CDM's view
static int c_bitpos8 ( size_t i ){ return i & 7; }                            // bit-plane transpose
static int c_bitpos16( size_t i ){ return i & 15; }
static int c_bitpos32( size_t i ){ return i & 31; }
static int c_bitpos64( size_t i ){ return i & 63; }
static int c_prevbyte( size_t i ){ size_t b=i>>3; return b? buf[b-1] : 0; }   // o1
static int c_prevpop ( size_t i ){ size_t b=i>>3; if(!b) return 0; int c=0,v=buf[b-1]; while(v){c+=v&1;v>>=1;} return c; }
static int c_prevnib ( size_t i ){ size_t b=i>>3; return b? (buf[b-1]>>4) : 0; }
static int c_prev1bit( size_t i ){ return i? bitat(i-1) : 0; }
static int c_prev3bit( size_t i ){ int v=0; for(int k=3;k>=1;k--) v=v*2+((i>=(size_t)k)?bitat(i-k):0); return v; }
static int c_prev6bit( size_t i ){ int v=0; for(int k=6;k>=1;k--) v=v*2+((i>=(size_t)k)?bitat(i-k):0); return v; }
static int c_bitpos_prev1  ( size_t i ){ return (i&7)*2 + (i? bitat(i-1):0); }
static int c_bitpos_prevpop( size_t i ){ return (i&7)*9 + c_prevpop(i); }
// distance since the last 0xFF / 0x00 byte, bucketed -- carryless-rangecoder artifacts
static int dist_bucket( size_t b, byte target ){
  size_t d=0; while( d<8 && b>d && buf[b-1-d]!=target ) d++;
  return (b>d && buf[b-1-d]==target) ? int(d) : 8;
}
static int c_distFF( size_t i ){ return dist_bucket(i>>3,0xFF)*8 + (i&7); }
static int c_dist00( size_t i ){ return dist_bucket(i>>3,0x00)*8 + (i&7); }
static int c_prevbyte_hi3( size_t i ){ size_t b=i>>3; return (b? (buf[b-1]>>5):0)*8 + (i&7); }

static Ctx CTXS[] = {
  { "flat (plain CDM)",        1,   c_flat },
  { "bitpos mod 8",            8,   c_bitpos8 },
  { "bitpos mod 16",          16,   c_bitpos16 },
  { "bitpos mod 32",          32,   c_bitpos32 },
  { "bitpos mod 64",          64,   c_bitpos64 },
  { "prev byte (o1)",        256,   c_prevbyte },
  { "prev byte popcount",      9,   c_prevpop },
  { "prev byte hi nibble",    16,   c_prevnib },
  { "prev 1 bit",              2,   c_prev1bit },
  { "prev 3 bits",             8,   c_prev3bit },
  { "prev 6 bits",            64,   c_prev6bit },
  { "bitpos x prev bit",      16,   c_bitpos_prev1 },
  { "bitpos x prev popcount", 72,   c_bitpos_prevpop },
  { "bitpos x dist-to-FF",    72,   c_distFF },
  { "bitpos x dist-to-00",    72,   c_dist00 },
  { "bitpos x prev hi3",      64,   c_prevbyte_hi3 },
};

int main( int argc, char** argv ) {
  if( argc<2 ) { fprintf(stderr,"usage: ctxscan FILE [chunkbits]\n"); return 1; }
  if( argc>2 ) NBITS = atoi(argv[2]);
  FILE* f=fopen(argv[1],"rb"); if(!f) return 2;
  fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
  buf.resize(n); if( fread(buf.data(),1,n,f)!=(size_t)n ) return 3; fclose(f);
  size_t M = (size_t)n*8;

  printf("\n%s  (%ld bytes, %zu bits, chunk = %d lane bits)\n", argv[1], n, M, NBITS);
  printf("%-24s %6s %9s %12s %9s %10s\n", "context", "lanes", "chunks", "excess bits", "+-noise", "bits/chunk");

  for( size_t c=0; c<sizeof(CTXS)/sizeof(CTXS[0]); c++ ) {
    Ctx& C = CTXS[c];
    std::vector<int> cnt(C.nlanes,0), zeros(C.nlanes,0);
    double excess=0; long chunks=0;
    for( size_t i=0; i<M; i++ ) {
      int l = C.lane(i); if( l<0 || l>=C.nlanes ) continue;
      zeros[l] += (bitat(i)==0);
      if( ++cnt[l] == NBITS ) {                    // a full chunk of this lane
        double d = NBITS/2.0 - zeros[l];
        excess += 2.0*(d*d - NBITS/4.0)/(NBITS*M_LN2);
        chunks++; cnt[l]=0; zeros[l]=0;
      }
    }
    double noise = sqrt((double)chunks);
    printf("%-24s %6d %9ld %12.1f %9.1f %10.3f%s\n", C.name, C.nlanes, chunks, excess, noise,
           chunks? excess/chunks : 0.0, (chunks && excess > 3*noise) ? "   <-- signal" : "");
  }
  return 0;
}
