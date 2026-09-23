// gradtest_mix.cpp -- end-to-end gradient check of p = mix2(C0, C1); built
// and run by gtm.sh, which swaps schrau.inc for exact libm math first (the
// piecewise-linear Schraudolph functions would blur the comparison).
//
// Warms the three tables on a prefix of book1 through the coder's own
// per-bit code, and at every 97th bit compares, against central finite
// differences of the final loss L = -ln P(coded bit):
//   - the chain factors main() hands the counters, e_f*d1 and e_f*d2
//     (dL/dz of each counter's logit at the mixer's input);
//   - the gradient each ParamUpdater actually receives (the GRAD_TEST hook
//     records gd = -dL/dtheta as the updater takes it): the mixer's W and b,
//     and mw and K of both counter cells.  Those are purely end-to-end when
//     the bundle's E2E is 1 (the delivered C0/C1 value); u/v are not
//     checked -- their gradient runs through the RTRL traces, a derivative
//     over the cell's history that a one-step difference cannot reproduce.
// Samples where a counter's logit sits at the mixer's input clip are
// skipped (the derivative is discontinuous there).  Relative errors of
// ~1e-4 median and <1% p99 are float/FD noise; the largest ones come from
// saturated cells (q0 = 1, gradients ~1e-4) where the loss difference is
// below float resolution.
#include <vector>
#include <algorithm>
static std::vector<float> hook;
void grad_test_hook( float gd, float, float ) { hook.push_back(gd); }
#define main coder_main
#include "coder0.cpp"
#undef main

template<class C> static double lossof( const C0_Cell& a, const C1_Cell& b, const Mix2_Cell& m, int bit ) {
  auto p0 = a.PredictF(); auto p1 = b.PredictF(); auto pm = m.Mix( p0.z, p1.z );
  double p = pm.p; return -log( bit ? 1.0-p : p );
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
  Stat sz0{"dL/dz_C0 = e_f*d1"}, sz1{"dL/dz_C1 = e_f*d2"}, sW{"mixer W"}, sB{"mixer b"},
       s0m{"C0 mw (E2E=1)"}, s0k{"C0 K  (E2E=1)"}, s1m{"C1 mw (E2E=1)"}, s1k{"C1 K  (E2E=1)"};
  int last_c=0, c2=0; long nbit=0, nsamp=0;
  const double h = 1e-2;
  for( size_t pos=0; pos<len; pos++ ) {
    uint c = buf[pos];
    for( uint cxt=1; cxt<256; ) {
      int bit = (c>>7)&1;
      uint ox = C0_MakeCx(c2,last_c,cxt), oy = C1_MakeCx(c2,last_c,cxt), mx = M0_MakeCx(c2,last_c,cxt);
      C0_Cell& a = C0.C0_tbl[ox]; C1_Cell& b = C1.C1_tbl[oy]; Mix2_Cell& m = M0.M0_tbl[mx];
      auto pr0 = a.PredictF(); auto pr1 = b.PredictF(); auto pm = m.Mix( pr0.z, pr1.z );
      float e_f = pm.p - float(1-bit);
      bool samp = (++nbit % 97)==0;
      bool k0 = fabsf(pr0.stP) >= CP_C0::stP_min, k1 = fabsf(pr1.stP) >= CP_C1::stP_min;
      if( samp ) {
        nsamp++;
        // chain factors: perturb the counters' logits at the mixer input
        auto Lz = [&](float z0, float z1){ auto q = m.Mix(z0,z1); double p=q.p; return -log(bit?1.0-p:p); };
        float zm = CP_M0::ZM;
        if( fabsf(pr0.z) < zm-0.05f ) add( sz0, e_f*pm.d1, (Lz(pr0.z+h,pr1.z)-Lz(pr0.z-h,pr1.z))/(2*h) );
        if( fabsf(pr1.z) < zm-0.05f ) add( sz1, e_f*pm.d2, (Lz(pr0.z,pr1.z+h)-Lz(pr0.z,pr1.z-h))/(2*h) );
        // perturbed copies of the cells for the parameter FDs
        auto fd = [&](auto mutate) {
          C0_Cell A=a; C1_Cell B=b; Mix2_Cell M=m; mutate(A,B,M,+h); double lp = lossof<int>(A,B,M,bit);
          A=a; B=b; M=m;                        mutate(A,B,M,-h); double lm = lossof<int>(A,B,M,bit);
          return (lp-lm)/(2*h); };
        double nW  = fd([](C0_Cell&,C1_Cell&,Mix2_Cell& M,double d){ M.w.val += d; });
        double nB  = fd([](C0_Cell&,C1_Cell&,Mix2_Cell& M,double d){ M.b.val += d; });
        double n0m = fd([](C0_Cell& A,C1_Cell&,Mix2_Cell&,double d){ A.mw_state.val += d; });
        double n0k = fd([](C0_Cell& A,C1_Cell&,Mix2_Cell&,double d){ A.k_state.val  += d; });
        double n1m = fd([](C0_Cell&,C1_Cell& B,Mix2_Cell&,double d){ B.mw_state.val += d; });
        double n1k = fd([](C0_Cell&,C1_Cell& B,Mix2_Cell&,double d){ B.k_state.val  += d; });
        // the real updates, on copies, capturing each updater's gd
        C0_Cell A=a; C1_Cell B=b; Mix2_Cell M=m;
        hook.clear(); A.C_Update( bit, pr0, 1.0f, e_f*pm.d1 ); std::vector<float> h0 = hook;
        hook.clear(); B.C_Update( bit, pr1, 1.0f, e_f*pm.d2 ); std::vector<float> h1 = hook;
        hook.clear(); M.Update( bit, pm );                      std::vector<float> hm = hook;
        // order: u, v, mw, [K if gated]; mixer: W, b.  gd = -dL/dtheta.
        bool in0 = fabsf(pr0.z) < zm-0.05f, in1 = fabsf(pr1.z) < zm-0.05f;
        if( in0 ) { add( s0m, -h0[2], n0m ); if( k0 ) add( s0k, -h0[3], n0k ); }
        if( in1 ) { add( s1m, -h1[2], n1m ); if( k1 ) add( s1k, -h1[3], n1k ); }
        if( in0 && in1 ) { add( sW, -hm[0], nW ); add( sB, -hm[1], nB ); }
      }
      a.C_Update( bit, pr0, 1.0f, e_f*pm.d1 );
      b.C_Update( bit, pr1, 1.0f, e_f*pm.d2 );
      m.Update( bit, pm );
      c<<=1; cxt += cxt+bit;
    }
    c2 = last_c; last_c = buf[pos];
  }
  printf("E2E: C0 %.3f  C1 %.3f   (1.0 = the updater's gradient is purely end-to-end)\n", CP_C0::E2E, CP_C1::E2E);
  printf("%ld bits, %ld sampled; relative error |analytic - FD| / max(|FD|, 1e-3):\n", nbit, nsamp);
  printf("  %-22s %6s %10s %10s %10s %8s\n","quantity","n","median","p99","max","sign-");
  for( Stat* s : { &sz0,&sz1,&sW,&sB,&s0m,&s0k,&s1m,&s1k } ) {
    auto r = s->rel; std::sort(r.begin(), r.end());
    if( r.empty() ) { printf("  %-22s none\n", s->name); continue; }
    printf("  %-22s %6d %10.2e %10.2e %10.2e %8d\n", s->name, s->n, r[r.size()/2], r[size_t(r.size()*0.99)], r.back(), s->sign_bad);
  }
}
