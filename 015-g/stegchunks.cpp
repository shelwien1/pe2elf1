
// stegchunks.cpp — a port of stegdict.cpp for the sorted_chunks case.
//
// sorted_chunks partitions a file into contiguous monotone chunks (each chunk
// is a lexicographically ascending "+N" or descending "-N" run of lines).
// The lines within a chunk are therefore already sorted, so their order is a
// free channel — exactly the situation stegdict exploits for a sorted
// dictionary. stegchunks treats every chunk as one such dictionary and hides
// the payload in the per-chunk line permutations, all sharing one payload
// stream. Descending chunks use the Model's f_desc (reverse) ordering.
//
// Usage: stegchunks c|d <input> <output> <list> <payload>
//   c : embed <payload> into the line order of <input> -> <output>
//   d : recover <payload> from <input>, restoring canonical order -> <output>
// <list> is the sorted_chunks output (signed chunk lengths).

#define INC_FLEN
#define INC_LOG2I
#include "common.inc"
#include "coro3b.inc"

#include "sh_v2f.inc"

#include "idx_tree.inc"
#include "sh_qsort.inc"

#include "model.inc"

#include "coro_fp2.inc"

#include "usage_chunks.inc" // msg_logo / msg_usage / msg_err_* strings, from usage_chunks.txt

static union {
ALIGN(4096) CoroFileProc< Model<0> > C;
ALIGN(4096) CoroFileProc< Model<1> > D;
};

uint f_DEC;

// Process one chunk: ptr/len is its byte range inside the whole input buffer,
// desc selects descending order, h is the shared payload stream, g the output
// file. f_cont threads the range-coder stream across chunks (see model.inc).
NOINLINE
void ProcessChunk( byte* ptr, uint len, uint desc, FILE* h, FILE* g, uint f_cont ) {
  if( f_DEC==0 ) {
    C.Init( ptr, len, 0, desc, 1 ); // rec_size=0 (newline records), quiet
    if( f_cont ) C.f_cont = f_cont;
    C.processfile( h, g );          // read payload from h, write permuted lines to g
    C.Free();
  } else {
    D.Init( ptr, len, 0, desc, 1 );
    D.g = g;                        // restored (canonical) lines go to g
    if( f_cont ) D.f_cont = f_cont;
    D.processfile( 0, h );          // write recovered payload to h
    D.Free();
  }
}

static byte* load( const char* nam, uint& len ) {
  FILE* f = fopen( nam, "rb" ); if( f==0 ) return 0;
  len = flen(f);
  byte* p = new byte[len+1]; if( p ) { len = fread( p, 1,len, f ); p[len]=0; }
  fclose(f);
  return p;
}

int main( int argc, char** argv ) {

  printf( msg_logo );

  if( argc<6 ) { printf( msg_usage ); return 1; }

  f_DEC = (argv[1][0]=='d');
  char* inam = argv[2]; // input file (cover)
  char* onam = argv[3]; // output file
  char* lnam = argv[4]; // chunk list (sorted_chunks output)
  char* pnam = argv[5]; // payload

  // input file -> buffer
  uint f_len;
  byte* buf = load( inam, f_len );
  if( buf==0 ) { printf( msg_err_input, inam ); return 2; }

  // chunk list -> signed integers
  uint l_len;
  byte* lbuf = load( lnam, l_len );
  if( lbuf==0 ) { printf( msg_err_list, lnam ); return 6; }

  int* chunks = new int[l_len+1]; if( chunks==0 ) { printf( msg_err_mem ); return 5; }
  uint nch=0;
  {
    byte* p = lbuf; byte* end = lbuf + l_len;
    while( p<end ) {
      while( (p<end) && ((*p<'0')||(*p>'9')) && (*p!='-') ) p++; // skip to next number
      if( p>=end ) break;
      int sign=1; if( *p=='-' ) { sign=-1; p++; }
      uint got=0; long v=0;
      while( (p<end) && (*p>='0') && (*p<='9') ) { v=v*10+(*p-'0'); p++; got=1; }
      if( got ) chunks[nch++] = (int)(sign*v);
    }
  }

  // line offsets of the input buffer (each entry = start byte of a line)
  uint nlin=0;
  for( uint i=0; i<f_len; i++ ) if( buf[i]=='\n' ) nlin++;
  if( (f_len>0) && (buf[f_len-1]!='\n') ) nlin++; // last line without trailing '\n'

  uint* line_ofs = new uint[nlin+1]; if( line_ofs==0 ) { printf( msg_err_mem ); return 5; }
  { uint j=0; line_ofs[j++]=0;
    for( uint i=0; i<f_len; i++ ) if( (buf[i]=='\n') && (j<=nlin) ) line_ofs[j++]=i+1;
    line_ofs[nlin]=f_len;
  }

  // the chunk magnitudes must tile the input's lines exactly
  { unsigned long long s=0;
    for( uint k=0; k<nch; k++ ) s += (chunks[k]<0) ? (unsigned)(-chunks[k]) : (unsigned)chunks[k];
    if( s != nlin ) {
      printf( msg_err_count, lnam );
      printf( "  (list totals %llu lines, input has %u)\n", s, nlin );
      return 7;
    }
  }

  FILE* g = fopen( onam, "wb" );          if( g==0 ) { printf( msg_err_output, onam ); return 3; }
  FILE* h = fopen( pnam, f_DEC?"wb":"rb" ); if( h==0 ) { printf( msg_err_payload, pnam ); return 4; }

  // one shared payload stream across all chunks (f_cont: bit0=not-first, bit1=not-last)
  uint line=0;
  for( uint k=0; k<nch; k++ ) {
    uint n = (chunks[k]<0) ? (uint)(-chunks[k]) : (uint)chunks[k];
    uint desc = (chunks[k]<0);
    byte* ptr = &buf[ line_ofs[line] ];
    uint  len = line_ofs[line+n] - line_ofs[line];
    uint  f_cont = (k!=0) | (2*(k!=nch-1));
    ProcessChunk( ptr, len, desc, h, g, f_cont );
    line += n;
  }

  fclose(h);
  fclose(g);

  return 0;
}
