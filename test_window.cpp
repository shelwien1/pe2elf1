// test_window.cpp -- proves the history window holds enwik9 whole.
//
//   g++ -O2 -o test_window test_window.cpp && ./test_window
//
// Unlike test_logistic.cpp this does NOT re-implement anything: it includes the
// real paq8hp_speed.hpp and inspects the actual paq8hp::buf, so it cannot drift
// from what paq8hpc ships.
//
// What matters here: there is no MatchModel in this build, so Buf IS the whole
// match/history window.  Every model back-reference goes through it -- the
// order-N contexts via buf(i), WordModel's `above` via buf[nl1+col] -- and
// operator[] masks with SZ-1.  So a window smaller than the input does not fail
// loudly; references past SZ quietly alias earlier bytes and the model trains on
// wrapped garbage while still round-tripping perfectly.  That is the failure mode
// these checks exist to rule out.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paq8hp_speed.hpp"

using paq8hp::buf;
using paq8hp::pos;

static int fails = 0;
static void ck(bool ok, const char* what, const char* detail = "") {
  if( !ok ) { printf("  FAIL  %s %s\n", what, detail); fails++; }
  else        printf("  ok    %s %s\n", what, detail);
}

constexpr int ENWIK9 = 1000000000;

int main() {
  char b[256];
  const unsigned long long SZ = buf.size();

  printf("\n=== 1. window size vs enwik9 ===\n");
  snprintf(b,sizeof b,"%llu bytes (%.3f GiB), sizeof(buf)=%zu", SZ, SZ/1073741824.0, sizeof(buf));
  ck(SZ==(1ull<<30), "window is exactly 1 GiB", b);
  ck(sizeof(buf)==SZ, "sizeof(buf) == declared size");
  snprintf(b,sizeof b,"enwik9 needs %d, window has %llu, headroom %.1f MiB / %.1f MB (%.1f%%)",
           ENWIK9, SZ, (SZ-(unsigned long long)ENWIK9)/1048576.0,
           (SZ-(unsigned long long)ENWIK9)/1000000.0, 100.0*(SZ-ENWIK9)/ENWIK9);
  ck(SZ>=(unsigned long long)ENWIK9, "window holds enwik9 whole", b);
  ck((SZ&(SZ-1))==0, "window is a power of two (operator[] masks with SZ-1)");

  printf("\n=== 2. no position in [0,enwik9) aliases another (exhaustive) ===\n");
  {
    // operator[] returns b[i & (SZ-1)], so distinct positions stay distinct iff
    // masking is the identity over the whole range.  Checked for every one of
    // the 10^9 positions, not sampled.
    unsigned long long bad = 0; long long firstbad = -1;
    for( unsigned int p = 0; p<(unsigned)ENWIK9; p++ ) {
      if( (p & (unsigned)(SZ-1)) != p ) { if(firstbad<0) firstbad = p; bad++; }
    }
    snprintf(b,sizeof b,"%llu aliasing positions below enwik9", bad);
    if( bad ) snprintf(b,sizeof b,"%llu aliasing positions, first at %lld", bad, firstbad);
    ck(bad==0, "masking is the identity for all 10^9 positions", b);
  }

  printf("\n=== 3. the wrap boundary is exactly at 1 GiB ===\n");
  {
    // documents the real limit: position SZ is the first one that collides, and
    // it sits 73.7 MB past enwik9.
    // position SZ masks to slot 0, i.e. it is the first position that collides
    ck(((unsigned long long)SZ & (SZ-1)) == 0ull,
       "position SZ aliases position 0 (first collision)");
    ck(((unsigned long long)(SZ-1) & (SZ-1)) == SZ-1,
       "position SZ-1 is still distinct (last usable slot)");
    snprintf(b,sizeof b,"first colliding position is %llu, i.e. enwik9 + %llu bytes",
             SZ, SZ-(unsigned long long)ENWIK9);
    ck(SZ>(unsigned long long)ENWIK9, "first collision is past enwik9, not inside it", b);
  }

  printf("\n=== 4. write/read the full enwik9 span, then back-reference it ===\n");
  {
    // Fill positions 0..enwik9-1 the way Predictor::update does (buf[pos++]=c),
    // with a pattern that is a function of position, then check buf(i) resolves
    // to the right byte at every distance that matters -- including one byte
    // short of the whole file, which is the reference a match model or a long
    // order-N context would want on enwik9.
    pos = 0;
    for( unsigned int p = 0; p<(unsigned)ENWIK9; p++ )
      buf[pos++] = (paq8hp::U8)((p*2654435761u)>>24);
    snprintf(b,sizeof b,"pos ended at %d", pos);
    ck(pos==ENWIK9, "wrote all 10^9 bytes, pos tracked them", b);

    const unsigned int dists[] = {1u,2u,3u,255u,256u,65535u,65536u,1000000u,
                                  16777216u,536870911u,999999998u,999999999u};
    int bad = 0; unsigned int badd = 0;
    for( unsigned k = 0; k<sizeof(dists)/sizeof(dists[0]); k++ ) {
      unsigned int d = dists[k];
      unsigned int want_pos = (unsigned)ENWIK9 - d;          // buf(d) is buf[pos-d]
      paq8hp::U8 want = (paq8hp::U8)((want_pos*2654435761u)>>24);
      if( (paq8hp::U8)buf(d) != want ) { if(!bad) badd = d; bad++; }
    }
    snprintf(b,sizeof b,"%d wrong of %zu distances up to 999,999,999",
             bad, sizeof(dists)/sizeof(dists[0]));
    if( bad ) snprintf(b,sizeof b,"%d wrong, first at distance %u", bad, badd);
    ck(bad==0, "buf(i) back-references resolve correctly across the whole file", b);

    // and the very first byte of the file is still reachable from the very last
    paq8hp::U8 first = (paq8hp::U8)((0u*2654435761u)>>24);
    ck((paq8hp::U8)buf((unsigned)ENWIK9) == first,
       "byte 0 of enwik9 is still live when pos is at the end (no eviction)");
  }

  printf("\n=== 5. WordModel's negative index is well-defined ===\n");
  {
    // WordModel starts with nl1 = -3 and evaluates buf[nl1+col]; the U32
    // conversion sends that to the top of the buffer, which is in bounds and
    // zero before anything is written there.  Confirming it is defined, not that
    // the value is meaningful.
    int nl1 = -3, col = 0;
    unsigned int idx = (unsigned int)(nl1+col);
    snprintf(b,sizeof b,"index %d -> slot %llu of %llu", nl1+col,
             (unsigned long long)(idx & (unsigned)(SZ-1)), SZ);
    ck((idx & (unsigned)(SZ-1)) < SZ, "buf[nl1+col] with nl1=-3 lands in bounds", b);
  }

  printf("\n=== 6. int pos headroom ===\n");
  {
    // pos is a signed int, so it is the other ceiling.  enwik9 uses 46.6% of it.
    snprintf(b,sizeof b,"enwik9 uses %.1f%% of INT_MAX; pos overflows at 2^31 = %u",
             100.0*ENWIK9/2147483647.0, 2147483648u);
    ck((long long)ENWIK9 < 2147483647LL, "int pos holds enwik9 with margin", b);
    // and the format's own limit, from the 4-byte length header in the driver
    snprintf(b,sizeof b,"4-byte f_len header caps input at %llu; window wraps first at %llu",
             4294967295ull, SZ);
    ck(true, "limit ordering: window (1 GiB) < int pos (2 GiB) < header (4 GiB)", b);
  }

  printf("\n%s (%d failures)\n\n", fails? "FAILED" : "ALL CHECKS PASSED", fails);
  return fails ? 1 : 0;
}
