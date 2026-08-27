
#ifndef EXTERN
#define EXTERN
#define INC_FLEN
#include "common.inc"
#endif

enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, hSCALE=SCALE/2, eSCALE=16*SCALE };
enum { N_STATES=32768 };

EXTERN word pp[N_STATES];

struct fsm {
  word s[2]; // next state after bits 0,1
//  word pp;

  uint get_number( char*& p, char* q ) {
    int c; uint r=0;
    while( p<q ) { c = *p++; if( (c>='0')&&(c<='9') ) {--p; break; } }
    while( p<q ) { c = *p++; if( (c>='0')&&(c<='9') ) (r*=10)+=c-'0'; else break; }
    return r;
  }

  uint Load( FILE* f ) {
    uint i, f_len = flen(f);
    char* p = new char[f_len]; if( p==0 ) return 1;
    f_len = fread( p, 1,f_len, f );
    char* q = p + f_len, *p0=p;
    for( i=0; i<N_STATES; i++ ) {
      this[i].s[0] = Max(0,Min(N_STATES-1,get_number(p,q)));
      this[i].s[1] = Max(0,Min(N_STATES-1,get_number(p,q)));
//      this[i].pp   = Max(1,Min(SCALE-1,get_number(p,q)));
      pp[i]   = Max(1,Min(SCALE-1,get_number(p,q)));
    }
    delete[] p0;
    return 0;
  }

};

EXTERN fsm FSM[N_STATES];
