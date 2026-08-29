
#ifndef EXTERN
#define EXTERN
#define INC_FLEN
#include "common.inc"
#endif

#include "rc_config.inc"   // SCALElog and friends

enum { N_STATES=32768 };

EXTERN word pp[N_STATES];

// RC_FUSE_PP: the counter step's table, one dword per (state,bit):
//   low 16  = pp[next], the probability the NEXT visit to this context wants
//   high 16 = next, the state itself, for the visit after that
// Same address shape as ((word*)FSM)[s*2+bit] -- one scaled index, scale 4
// instead of 2 -- which is the whole point; a {word s[2]; word p;} struct
// would put the address back to base + s*6 + bit*2 (see counter.inc).
EXTERN uint FSM2[N_STATES*2];

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
    for( i=0; i<N_STATES; i++ ) {
      uint n0 = this[i].s[0], n1 = this[i].s[1];
      FSM2[i*2+0] = uint(pp[n0]) + (n0<<16);
      FSM2[i*2+1] = uint(pp[n1]) + (n1<<16);
    }
    delete[] p0;
    return 0;
  }

};

EXTERN fsm FSM[N_STATES];
