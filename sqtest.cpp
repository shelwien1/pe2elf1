/* sqtest.cpp -- accuracy test for the squash/stretch/code-length tables.
 *
 * xad_logistic.inc builds SQT/STT/CLT with pure integer arithmetic, so they are
 * reproducible -- but reproducible is not the same as CORRECT, and the md5
 * matrix only proves that two builds agree, not that either one maps
 * probabilities the way the model assumes.  This checks the mapping itself,
 * against long double references (~19 significant digits, against a table whose
 * tightest rounding margin is 1.6e-7, so the reference is not the limit).
 *
 * It includes the REAL xad_logistic.inc rather than a copy, so it cannot drift.
 * Not part of the xadpcm build: build and run it with ./sqtest.sh.
 *
 * What it asserts, and why each bound is what it is:
 *   range       SQT in [P_MIN,P_MAX], STT in [ST_MIN,ST_MAX] -- these index and
 *               are indexed by the model, so out of range is a memory bug.
 *   monotone    both tables non-decreasing.  The mixer assumes more evidence
 *               means higher probability; a dip would make the model
 *               non-monotone in its own input, which no amount of tuning fixes.
 *   accuracy    every entry within 1 of the correctly-rounded value.  1, not 0:
 *               integer LOG2[] carries ~1e-5 of log2 error and SQT is built by
 *               inverting the QUANTIZED stretch rather than by rounding the
 *               exact logistic, so exact agreement is not what this
 *               construction promises -- boundedness is.
 *   STC         STC[p] == STT[max(p,P_MIN)], the folded pr() clamp.  Cheap, and
 *               it is the one table that has already been wrong once.
 *   inverse     STT[SQT[d]] == d wherever d is in STT's range.  This is the
 *               property the integer build buys and the float pair only had
 *               approximately: SQT is constructed as the inverse of the STT the
 *               mixer actually sees, not as a separately rounded transcendental.
 *   CLT         within 1 of ST_SCALE*(P_BITS - log2 p).  Accounting only, so a
 *               failure here misreports -v and does not corrupt an archive.
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int32_t i32; typedef int64_t i64;
#ifdef __GNUC__
 #define INLINE __attribute__((always_inline)) inline
#else
 #define INLINE __forceinline
#endif

/* The tables are the only thing under test, but xad_logistic.inc also builds the
   four counter rate schedules, so the G0_ names they need have to exist.  Any
   legal value does: fill_rates() clamps them and nothing here reads RS_*. */
#ifndef SQ_ST_SCALE
#define SQ_ST_SCALE 256          // sqtest.sh sweeps this
#endif
#define G0_ST_SCALE SQ_ST_SCALE
#define RW(p) p##0=1000, p##1=1000, p##2=1000, p##3=1000, p##4=1000, p##5=1000, \
              p##6=1000, p##7=1000, p##8=1000, p##9=1000, p##10=1000, p##11=1000, \
              p##12=1000, p##13=1000, p##14=1000, p##15=1000, p##_NMAX=8
enum { RW(G0_CW), RW(G0_HW), RW(G0_LW), RW(G0_MW),
       G0_CP_INIT=32768, G0_HP_INIT=32768, G0_LP_INIT=32768, G0_MP_INIT=32768 };
#undef RW

#define XAD_STATS 1              // CLT only exists in the stats build
#include "xad_prelude.inc"
#include "xad_logistic.inc"

static int fails = 0;
static void bad(const char* what, long long a, long long b, long long c) {
  if( ++fails<=8 ) fprintf(stderr, "  FAIL %s: %lld %lld %lld\n", what, a, b, c);
}

// exact references.  ST_SCALE is a runtime value in the Debug build, hence not constexpr.
static long double ex_stretch(int p) { return (long double)ST_SCALE*logl((long double)p/(long double)(P_ONE-p)); }
static long double ex_squash(int d)  { return (long double)P_ONE/(1.0L+expl(-(long double)d/(long double)ST_SCALE)); }
static int rnd_clamp(long double x, int lo, int hi) {
  long double r = floorl(x+0.5L);
  return r<lo ? lo : r>hi ? hi : int(r);
}

int main() {
  init_tables();   // aborts by itself if the shipped checksum does not match

  /* 1. range, monotonicity, and the folded-clamp table */
  for( int p = 0; p<P_ONE; p++ ) {
    if( STT[p]<ST_MIN || STT[p]>ST_MAX ) bad("STT range", p, STT[p], 0);
    if( SQT[p]<P_MIN || SQT[p]>P_MAX )   bad("SQT range", p, SQT[p], 0);
    if( STC[p]!=STT[p<P_MIN ? P_MIN : p] ) bad("STC", p, STC[p], STT[p<P_MIN ? P_MIN : p]);
    if( p && STT[p]<STT[p-1] ) bad("STT monotone", p, STT[p-1], STT[p]);
    if( p && SQT[p]<SQT[p-1] ) bad("SQT monotone", p, SQT[p-1], SQT[p]);
  }

  /* 2. accuracy against the exact logistic, over the domains the codec reaches.
        STT is read at p in [P_MIN,P_MAX] (Ctr::pr() clamps); SQT is read at
        d in [ST_MIN,ST_MAX] (squash() clamps).  Entries outside those are
        unreachable and are not held to anything. */
  int nst = 0, nsq = 0, ncl = 0; long double est = 0, esq = 0;
  int wst = 0, wsq = 0, nest = 0, nesq = 0;
  for( int p = P_MIN; p<=P_MAX; p++ ) {
    int want = rnd_clamp(ex_stretch(p), ST_MIN, ST_MAX);
    int e = STT[p]-want; if( e ) { nst++; if( abs(e)>wst ) wst = abs(e); }
    if( abs(e)>1 ) bad("STT accuracy", p, STT[p], want);
    /* |err| against the unclamped real value is only meaningful where the table
       is not saturated -- past |stretch| = ST_MAX it is a clamp, not an error.
       At ST_SCALE 2048 that clamp covers most of the domain, so averaging over
       it would report the clamp and call it accuracy. */
    long double x = ex_stretch(p);
    if( x>(long double)ST_MIN && x<(long double)ST_MAX ) { est += fabsl((long double)STT[p]-x); nest++; }

    int cw = rnd_clamp((long double)ST_SCALE*((long double)P_BITS - log2l((long double)p)), 0, 65535);
    if( abs(int(CLT[p][1])-cw)>1 ) { ncl++; bad("CLT", p, CLT[p][1], cw); }
  }
  for( int d = ST_MIN; d<=ST_MAX; d++ ) {
    int want = rnd_clamp(ex_squash(d), P_MIN, P_MAX);
    int e = SQT[d+P_MID]-want; if( e ) { nsq++; if( abs(e)>wsq ) wsq = abs(e); }
    if( abs(e)>1 ) bad("SQT accuracy", d, SQT[d+P_MID], want);
    long double y = ex_squash(d);   // same exclusion, for the same reason
    if( y>(long double)P_MIN && y<(long double)P_MAX ) { esq += fabsl((long double)SQT[d+P_MID]-y); nesq++; }
  }

  /* 3. inverse consistency.  SQT is built as the inverse of STT, so for every d
        that some p actually stretches to, squashing d must land on a p that
        stretches back to d.  Where d is NOT in STT's range there is no p to
        find and the requirement is vacuous.
        SATURATED entries are excluded, and this is the interesting part: at
        ST_SCALE 512 the real stretch reaches 4258 while STT clamps at 2047, so
        d = +-2047 IS in STT's range -- reached by the clamp, by 74 different p.
        SQT does not clamp; its binary search inverts the unsaturated curve and
        lands on p = 4022, whose stretch is 2046.  Nothing is wrong: STT[p] = d
        and SQT[d] != p simply cannot both hold once STT stops being injective,
        and asserting otherwise (which the first version of this test did) makes
        the test fail at exactly the ST_SCALE values where the clamp bites. */
  static bool hit[P_ONE]; memset(hit, 0, sizeof hit);
  for( int p = P_MIN; p<=P_MAX; p++ ) {
    long double x = ex_stretch(p);
    if( x>(long double)ST_MIN && x<(long double)ST_MAX ) hit[STT[p]+P_MID] = true;
  }
  int ninv = 0, nchk = 0;
  for( int d = ST_MIN; d<=ST_MAX; d++ ) {
    if( !hit[d+P_MID] ) continue;
    nchk++;
    if( STT[SQT[d+P_MID]]!=d ) { ninv++; bad("inverse", d, SQT[d+P_MID], STT[SQT[d+P_MID]]); }
  }

  /* 4. the composition the codec actually performs, reported not asserted.  A
        counter's p goes stretch -> mix -> squash, and stretch is lossy by
        construction: near p = P_MID its slope is 2*ST_SCALE/P_ONE = 1/8, so
        eight neighbouring p share one d and no inverse can separate them.  The
        number below is that quantization, not an error budget. */
  int rt0 = 0, rtw = 0;
  for( int p = P_MIN; p<=P_MAX; p++ ) {
    int e = abs(squash(STT[p])-p);
    if( !e ) rt0++;
    if( e>rtw ) rtw = e;
  }

  u32 h = 2166136261u;
  for( int i = 0; i<P_ONE; i++ ) { h = (h^u32(SQT[i]))*16777619u; h = (h^u32(u16(STT[i])))*16777619u; }

  printf("ST_SCALE %4d  tables %08X\n", ST_SCALE, h);
  printf("  STT  %4d/4095 not correctly rounded (worst %d), mean |err| %.4f over %d unsaturated\n",
         nst, wst, nest ? double(est)/nest : 0.0, nest);
  printf("  SQT  %4d/4095 not correctly rounded (worst %d), mean |err| %.4f over %d unsaturated\n",
         nsq, wsq, nesq ? double(esq)/nesq : 0.0, nesq);
  printf("  CLT  %4d/4095 off by more than 1\n", ncl);
  printf("  inverse STT[SQT[d]]==d: %d/%d reachable d\n", nchk-ninv, nchk);
  printf("  squash(stretch(p))==p: %d/4095 exact, worst %d (stretch slope quantizes %d p per d at P_MID)\n",
         rt0, rtw, (P_ONE+2*ST_SCALE-1)/(2*ST_SCALE));
  printf("%s\n", fails ? "  FAILED" : "  ok");
  return fails!=0;
}
