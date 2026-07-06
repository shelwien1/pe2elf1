// sorted_chunks.cpp
// Partition the lines of a file into contiguous chunks, each chunk being
// lexicographically non-decreasing ("N\n") or non-increasing ("-N\n"),
// singletons emitted as "1\n". Objective: minimize the number of chunks
// (fewer, longer chunks) -- solved exactly, not greedily.
//
// Model: valid chunk lengths starting at line i form a contiguous range
// [1, R[i]], R[i] = max(asc_run[i], desc_run[i]), because any prefix of a
// monotone run is monotone. Min-chunk parsing is then "minimum jumps with
// contiguous intervals" => exact O(n) layered-BFS shortest path over
// prefix positions 0..n. Greedy "always take the longest run" is NOT
// optimal in general (reach profile {2,3,1,1,4}: greedy 4 chunks, opt 3).
//
// Order: raw byte-lexicographic (memcmp semantics). Lines = bytes between
// '\n', no CR stripping, last line may lack the trailing '\n'.
// Equal adjacent lines count as both ascending and descending; an
// all-equal chunk is reported positive.
//
// Usage: ./sorted_chunks input_file output_file

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>

static int sgncmp( const char* a, size_t la, const char* b, size_t lb ) {
  size_t m = la<lb ? la : lb;
  int c = m ? memcmp(a,b,m) : 0;
  if( c ) return c<0 ? -1 : 1;
  return la<lb ? -1 : la>lb ? 1 : 0;
}

int main( int argc, char** argv ) {
  if( argc!=3 ) { fprintf(stderr,"Usage: %s input_file output_file\n",argv[0]); return 1; }

  FILE* f = fopen(argv[1],"rb");
  if( !f ) { perror(argv[1]); return 1; }
  std::vector<char> buf;
  { char t[1<<16]; size_t k; while( (k=fread(t,1,sizeof t,f))>0 ) buf.insert(buf.end(),t,t+k); }
  fclose(f);

  // pass 1: split lines, keep only sign of adjacent comparisons
  std::vector<int8_t> cmp;               // cmp[i] = sgn(line[i] <=> line[i+1])
  size_t n = 0;
  {
    const char* p = buf.data();
    const char* end = p + buf.size();
    const char* pl = 0; size_t pll = 0;
    while( p<end ) {
      const char* nl = (const char*)memchr( p, '\n', end-p );
      const char* q = nl ? nl : end;
      if( n ) cmp.push_back( (int8_t)sgncmp(pl,pll,p,q-p) );
      pl=p; pll=q-p; n++;
      p = nl ? nl+1 : end;
    }
  }
  if( n>=0xFFFFFFFFull ) { fprintf(stderr,"too many lines\n"); return 1; }

  FILE* g = fopen(argv[2],"wb");
  if( !g ) { perror(argv[2]); return 1; }
  if( n==0 ) { fclose(g); return 0; }

  // pass 2 (backward): maximal monotone run lengths starting at each line
  std::vector<uint32_t> A(n), R(n);      // A=non-decreasing run, R=max(A,D)
  {
    uint32_t a=1, d=1;
    A[n-1]=1; R[n-1]=1;
    for( size_t i=n-1; i-->0; ) {
      a = (cmp[i]<=0) ? a+1 : 1;
      d = (cmp[i]>=0) ? d+1 : 1;
      A[i]=a; R[i] = a>d ? a : d;
    }
  }

  // pass 3: layered BFS over prefix positions 0..n; edge i -> i+1..i+R[i].
  // par[j] = predecessor on some shortest path; assignments are O(n) total.
  std::vector<uint32_t> par(n+1);
  {
    size_t lo=0, hi=0, done=0;
    while( done<n ) {
      size_t nhi = done;
      for( size_t i=lo; i<=hi && i<n; i++ ) {
        size_t r = i + R[i]; if( r>n ) r=n;
        if( r>nhi ) { for( size_t j=nhi+1; j<=r; j++ ) par[j]=(uint32_t)i; nhi=r; }
      }
      lo=hi+1; hi=nhi; done=nhi;         // R[i]>=1 => progress every layer
    }
  }

  // reconstruct chunks backward, emit forward
  std::vector<int64_t> out;
  for( size_t j=n; j>0; j=par[j] ) {
    size_t i = par[j], len = j-i;
    out.push_back( len==1 ? 1 : A[i]>=len ? (int64_t)len : -(int64_t)len );
  }
  for( size_t k=out.size(); k-->0; ) fprintf( g, "%lld\n", (long long)out[k] );
  fclose(g);
  return 0;
}
