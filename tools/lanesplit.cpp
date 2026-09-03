// lanesplit -- apply a context function to a file's bits and write each lane
// out as its own file, packed MSB-first.  Then `cdm c` each lane and sum, to
// compare sum(CDM(lane_i)) against CDM(whole file).
//
//   lanesplit FILE CTXID OUTPREFIX
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
typedef unsigned char byte;

static std::vector<byte> buf;
static inline int bitat( size_t i ) { return (buf[i>>3] >> (7-(i&7))) & 1; }

static int c_flat    ( size_t i ){ (void)i; return 0; }
static int c_bitpos8 ( size_t i ){ return i & 7; }
static int c_bitpos32( size_t i ){ return i & 31; }
static int c_prev1bit( size_t i ){ return i? bitat(i-1) : 0; }
static int c_prev3bit( size_t i ){ int v=0; for(int k=3;k>=1;k--) v=v*2+((i>=(size_t)k)?bitat(i-k):0); return v; }
static int c_prev6bit( size_t i ){ int v=0; for(int k=6;k>=1;k--) v=v*2+((i>=(size_t)k)?bitat(i-k):0); return v; }
static int c_prev8bit( size_t i ){ int v=0; for(int k=8;k>=1;k--) v=v*2+((i>=(size_t)k)?bitat(i-k):0); return v; }
static int c_prevbyte( size_t i ){ size_t b=i>>3; return b? buf[b-1] : 0; }
static int c_prevpop ( size_t i ){ size_t b=i>>3; if(!b) return 0; int c=0,v=buf[b-1]; while(v){c+=v&1;v>>=1;} return c; }

struct Ctx { const char* name; int nlanes; int (*lane)( size_t i ); };
static Ctx CTXS[] = {
  { "flat",        1,   c_flat },
  { "bitpos8",     8,   c_bitpos8 },
  { "bitpos32",   32,   c_bitpos32 },
  { "prev1bit",    2,   c_prev1bit },
  { "prev3bit",    8,   c_prev3bit },
  { "prev6bit",   64,   c_prev6bit },
  { "prev8bit",  256,   c_prev8bit },
  { "prevbyte",  256,   c_prevbyte },
  { "prevpop",     9,   c_prevpop },
};

int main( int argc, char** argv ) {
  if( argc<4 ) {
    fprintf(stderr,"usage: lanesplit FILE CTXID OUTPREFIX\nctx ids:\n");
    for( size_t c=0;c<sizeof(CTXS)/sizeof(CTXS[0]);c++ ) fprintf(stderr,"  %zu %s (%d lanes)\n", c, CTXS[c].name, CTXS[c].nlanes);
    return 1;
  }
  FILE* f=fopen(argv[1],"rb"); if(!f) return 2;
  fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
  buf.resize(n); if( fread(buf.data(),1,n,f)!=(size_t)n ) return 3; fclose(f);
  size_t M=(size_t)n*8;

  int id = atoi(argv[2]);
  if( id<0 || id>=(int)(sizeof(CTXS)/sizeof(CTXS[0])) ) return 4;
  Ctx& C = CTXS[id];

  std::vector< std::vector<byte> > lanes(C.nlanes);
  std::vector<size_t> nbits(C.nlanes,0);
  for( size_t i=0; i<M; i++ ) {
    int l=C.lane(i); if(l<0||l>=C.nlanes) continue;
    size_t p=nbits[l]++;
    if( (p>>3) >= lanes[l].size() ) lanes[l].push_back(0);
    if( bitat(i) ) lanes[l][p>>3] |= 1<<(7-(p&7));
  }

  int used=0; size_t tot=0;
  for( int l=0; l<C.nlanes; l++ ) {
    if( lanes[l].empty() ) continue;
    char path[512]; snprintf(path,sizeof(path),"%s.%s.%03d", argv[3], C.name, l);
    FILE* g=fopen(path,"wb"); if(!g) return 5;
    fwrite(lanes[l].data(),1,lanes[l].size(),g); fclose(g);
    used++; tot += lanes[l].size();
  }
  printf("%s %d %zu\n", C.name, used, tot);   // name, lanes written, total bytes
  return 0;
}
