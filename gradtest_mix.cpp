// gradtest_mix.cpp -- end-to-end gradient check of p = SSE( mix2(C0, C1) );
// built and run by gtm.sh, which swaps schrau.inc for exact libm math first
// (the piecewise-linear Schraudolph functions would blur the comparison).
//
// Warms the tables on a prefix of book1 through the coder's own per-bit
// code, and at every 97th bit compares, against central finite differences
// of the final loss L = -ln P(coded bit), P = sq(z_sse):
//   - the chain factors main() hands back through the stages: e_f*dzdz at
//     the mixer's output logit, e_f*dzdz*d1 and e_f*dzdz*d2 at the two
//     counters' logits;
//   - the SSE cells' end-to-end chain, e_f*sh0 and e_f*sh1 times dz/dy for
//     y = ln K of the two bracketing cells;
//   - the gradient each ParamUpdater actually receives (the GRAD_TEST hook
//     records gd = -dL/dtheta as the updater takes it): the mixer's W and b,
//     and mw and K of both counter cells -- purely end-to-end when their
//     E2E is 1, the delivered C0/C1/M0 value.  u/v are not checked: their
//     gradient runs through the RTRL traces, a derivative over the cell's
//     history that a one-step difference cannot reproduce.
// Upstream checks use only bits whose mixed logit is inside the SSE's input
// range, away from its bucket knots (beyond +-LIM the final prediction does
// not depend on anything upstream and every chained gradient is 0, which
// the check would pass trivially), and whose counter logits are inside the
// mixer's input clip.  Relative errors of ~1e-4 median and <1% p99 are
// float/FD noise; the largest come from saturated cells (q0 = 1, gradients
// ~1e-4) where the loss difference is below float resolution.
#include <vector>
#include <algorithm>
static std::vector<float> hook;
void grad_test_hook( float gd, float, float ) { hook.push_back(gd); }
#define main coder_main
#include "coder0.cpp"
#undef main

typedef SSE_Row<sse_nb_clamp(CP_S0::NB)> Row;
static double lossof( const C0_Cell& a, const C1_Cell& b, const Mix2_Cell& m, const Row& s, int bit ) {
  auto p0 = a.PredictF(); auto p1 = b.PredictF(); auto pm = m.Mix( p0.z, p1.z ); auto ps = s.Predict( pm.z );
  double p = sq( ps.z ); return -log( bit ? 1.0-p : p );
}
struct Stat { const char* name; std::vector<double> rel; int sign_bad = 0, n = 0; };
static void add( Stat& s, double ana, double num ) {
  double d = fabs(ana-num), sc = fmax(fabs(num), 1e-3);
  s.rel.push_back( d/sc ); s.n++;
  if( fabs(num) > 1e-3 && ana*num < 0 ) s.sign_bad++;
}
int main() {
  FILE* f = fopen("book1","rb"); if(!f) return 1;
  std::vector<unsigned char> buf(768771); size_t len = fread(buf.data(),1,buf.size(),f); fclose(f);
  len = 200000;   // warm the tables on a prefix; sample every 97th bit
  C0.C0_Init(); for( uint i=0; i<uint(C0_Cx_Volume); i++ ) C0.C0_tbl[i].Init();
  C1.C1_Init(); for( uint i=0; i<uint(C1_Cx_Volume); i++ ) C1.C1_tbl[i].Init();
  M0.M0_Init(); for( uint i=0; i<uint(M0_Cx_Volume); i++ ) M0.M0_tbl[i].Init();
  S0.S0_Init(); for( uint r=0; r<S0_ROWS; r++ ) S0.S0_tbl[r].Init();
  Stat szm{"dL/dz_mix = e_f*dzdz"}, sz0{"dL/dz_C0 = e_m*d1"}, sz1{"dL/dz_C1 = e_m*d2"},
       ss0{"SSE cell j   K chain"}, ss1{"SSE cell j+1 K chain"}, sW{"mixer W (E2E=1)"}, sB{"mixer b (E2E=1)"},
       s0m{"C0 mw (E2E=1)"}, s0k{"C0 K  (E2E=1)"}, s1m{"C1 mw (E2E=1)"}, s1k{"C1 K  (E2E=1)"};
  int last_c=0, c2=0, c3=0; long nbit=0, nsamp=0, nin=0;
  const double h = 5e-3;
  const float LIM = CP_S0::LIM, knot = 2.0f*LIM/float(sse_nb_clamp(CP_S0::NB)-1), zm = CP_M0::ZM, zx = CP_S0::ZMAX;
  for( size_t pos=0; pos<len; pos++ ) {
    uint c = buf[pos];
    for( uint cxt=1; cxt<256; ) {
      int bit = (c>>7)&1;
      uint ox = C0_MakeCx(c2,last_c,cxt), oy = C1_MakeCx(c2,last_c,cxt), mx = M0_MakeCx(c2,last_c,cxt);
      uint sx = S0_Row( qword(S0_MakeCx(c2,last_c,cxt))*S0_Cx3_Volume + S0_MakeCx3(c3) );
      C0_Cell& a = C0.C0_tbl[ox]; C1_Cell& b = C1.C1_tbl[oy]; Mix2_Cell& m = M0.M0_tbl[mx]; Row& s = S0.S0_tbl[sx];
      auto pr0 = a.PredictF(); auto pr1 = b.PredictF(); auto pm = m.Mix( pr0.z, pr1.z ); auto ps = s.Predict( pm.z );
      float pf = sq( ps.z ), e_f = pf - float(1-bit), e_m = e_f * ps.dzdz;
      if( (++nbit % 97)==0 ) {
        nsamp++;
        // is the mixed logit inside the SSE's range, away from its knots?
        float xk = (pm.z + LIM) / knot, dk = fabsf( xk - roundf(xk) ) * knot;
        bool upstream = fabsf(pm.z) < LIM - 0.1f && dk > 0.1f;
        bool in0 = fabsf(pr0.z) < zm-0.05f, in1 = fabsf(pr1.z) < zm-0.05f;
        auto fd = [&](auto mutate) {
          C0_Cell A=a; C1_Cell B=b; Mix2_Cell M=m; Row R=s; mutate(A,B,M,R,+h); double lp = lossof(A,B,M,R,bit);
          A=a; B=b; M=m; R=s;                               mutate(A,B,M,R,-h); double lm = lossof(A,B,M,R,bit);
          return (lp-lm)/(2*h); };
        // the SSE cells: y = ln K of the two bracketing cells; dz_sse/dz_cell = sh, dz_cell/dy = z_cell
        if( dk > 0.02f && fabsf(ps.pr0.z) < zx-0.1f )
          add( ss0, e_f*ps.sh0*ps.pr0.z, fd([&](C0_Cell&,C1_Cell&,Mix2_Cell&,Row& R,double d){ R.c[ps.j].k_state.val += d; }) );
        if( dk > 0.02f && fabsf(ps.pr1.z) < zx-0.1f )
          add( ss1, e_f*ps.sh1*ps.pr1.z, fd([&](C0_Cell&,C1_Cell&,Mix2_Cell&,Row& R,double d){ R.c[ps.j+1].k_state.val += d; }) );
        if( upstream ) {
          nin++;
          auto Lzm = [&](float z){ auto q = s.Predict(z); double p = sq(q.z); return -log(bit?1.0-p:p); };
          add( szm, e_m, (Lzm(pm.z+h)-Lzm(pm.z-h))/(2*h) );
          auto Lz = [&](float z0, float z1){ auto q = s.Predict( m.Mix(z0,z1).z ); double p = sq(q.z); return -log(bit?1.0-p:p); };
          if( in0 ) add( sz0, e_m*pm.d1, (Lz(pr0.z+h,pr1.z)-Lz(pr0.z-h,pr1.z))/(2*h) );
          if( in1 ) add( sz1, e_m*pm.d2, (Lz(pr0.z,pr1.z+h)-Lz(pr0.z,pr1.z-h))/(2*h) );
          double nW  = fd([](C0_Cell&,C1_Cell&,Mix2_Cell& M,Row&,double d){ M.w.val += d; });
          double nB  = fd([](C0_Cell&,C1_Cell&,Mix2_Cell& M,Row&,double d){ M.b.val += d; });
          double n0m = fd([](C0_Cell& A,C1_Cell&,Mix2_Cell&,Row&,double d){ A.mw_state.val += d; });
          double n0k = fd([](C0_Cell& A,C1_Cell&,Mix2_Cell&,Row&,double d){ A.k_state.val  += d; });
          double n1m = fd([](C0_Cell&,C1_Cell& B,Mix2_Cell&,Row&,double d){ B.mw_state.val += d; });
          double n1k = fd([](C0_Cell&,C1_Cell& B,Mix2_Cell&,Row&,double d){ B.k_state.val  += d; });
          // the real updates, on copies, capturing each updater's gd; order:
          // counter u, v, mw, [K if gated]; mixer W, b.  gd = -dL/dtheta.
          bool k0 = fabsf(pr0.stP) >= CP_C0::stP_min, k1 = fabsf(pr1.stP) >= CP_C1::stP_min;
          C0_Cell A=a; C1_Cell B=b; Mix2_Cell M=m;
          hook.clear(); A.C_Update( bit, pr0, 1.0f, e_m*pm.d1 ); std::vector<float> h0 = hook;
          hook.clear(); B.C_Update( bit, pr1, 1.0f, e_m*pm.d2 ); std::vector<float> h1 = hook;
          hook.clear(); M.Update( bit, pm, e_m );                 std::vector<float> hm = hook;
          if( in0 ) { add( s0m, -h0[2], n0m ); if( k0 ) add( s0k, -h0[3], n0k ); }
          if( in1 ) { add( s1m, -h1[2], n1m ); if( k1 ) add( s1k, -h1[3], n1k ); }
          if( in0 && in1 ) { add( sW, -hm[0], nW ); add( sB, -hm[1], nB ); }
        }
      }
      a.C_Update( bit, pr0, 1.0f, e_m*pm.d1 );
      b.C_Update( bit, pr1, 1.0f, e_m*pm.d2 );
      m.Update( bit, pm, e_m );
      s.Update( bit, ps, e_f );
      c<<=1; cxt += cxt+bit;
    }
    c3 = c2; c2 = last_c; last_c = buf[pos];
  }
  printf("E2E: C0 %.3f  C1 %.3f  M0 %.3f   (1.0 = the updater's gradient is purely end-to-end)\n", CP_C0::E2E, CP_C1::E2E, CP_M0::E2E);
  printf("%ld bits, %ld sampled, %ld with the mixed logit inside the SSE's range (upstream checks);\n", nbit, nsamp, nin);
  printf("relative error |analytic - FD| / max(|FD|, 1e-3):\n");
  printf("  %-24s %6s %10s %10s %10s %8s\n","quantity","n","median","p99","max","sign-");
  for( Stat* st : { &szm,&sz0,&sz1,&ss0,&ss1,&sW,&sB,&s0m,&s0k,&s1m,&s1k } ) {
    auto r = st->rel; std::sort(r.begin(), r.end());
    if( r.empty() ) { printf("  %-24s none\n", st->name); continue; }
    printf("  %-24s %6d %10.2e %10.2e %10.2e %8d\n", st->name, st->n, r[r.size()/2], r[size_t(r.size()*0.99)], r.back(), st->sign_bad);
  }
}
