// prints the table: name flags gccset clangset (feature names)
#include "x86targets.h"
using namespace x86;
int main() {
  for( int t=0; t<N_TARGET; t++ ) {
    const X86Target& x = x86_target[t];
    printf( "%s\t%d", x.name, x.flags );
    for( int k=0; k<2; k++ ) {
      const FSet& s = k ? x.c : x.g;
      printf( "\t" );
      for( int f=0; f<N_FEAT; f++ ) if( Has( s, f ) ) printf( "%s ", x86_feat[f].name );
    }
    printf( "\n" );
  }
}
