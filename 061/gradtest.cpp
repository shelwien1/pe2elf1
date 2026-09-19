// gradtest.cpp -- numerical check of the gradient derivation (gt.sh).
//
// Everything starts from the codelength of the coded bit,
//   CL(theta) = -log2 p_bit(theta)   [bits],   p_bit = pK or 1-pK,
// which is what the coder pays.  ParamUpdater works in nats, L = CL*ln2,
// so what Accum receives (descent gradient gd, own gradient gr, curvature
// term hc) must satisfy, against double-precision finite differences of
// CL for the same cell / mixer / pipeline:
//   gr        = -ln2 * dCL/dtheta              (the cell's own codelength)
//   gr^2 - hc =  ln2 * d2CL/dtheta2            (its exact Hessian)
//   gd        = -ln2 * ((1-E2E) dCL/dtheta + E2E dCL_final/dtheta),
//               dCL_final/dtheta = (ef/ln2) dz/dtheta   (A2 blend)
// For the counter the RTRL traces are taken as the derivatives they claim
// to be (n0, n1 modelled as quadratics in wr0, wr1 with the trace
// coefficients), which is what C_Update's chain rule assumes.  The u/v
// curvature uses the shared approximation h00 + h11 (no cross term), so it
// is checked against that formula and the gap to the exact one is printed.
// The end-to-end chain of main() is checked by perturbing the order-1
// logit and the SSE cells through the real SSE_Ctr and Mix2 objects.
#define main coder0_main
#define GRAD_TEST 1
#include "coder0.cpp"
#undef main
#include <vector>
#include <cmath>

struct Rec { float gd, gr, hc; };
static std::vector<Rec> hooks;
void grad_test_hook( float gd, float gr, float hc ) { hooks.push_back( {gd, gr, hc} ); }

static int nfail = 0, ncheck = 0;
static double relerr( double a, double b ) { double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b); return m < 1e-9 ? fabs(a-b) : fabs(a-b)/m; }
static void check( const char* what, double code, double ref, double tol ) {
  double e = relerr(code, ref); ncheck++;
  bool bad = e > tol;
  if( bad ) nfail++;
  printf( "  %-34s code %+.6e  ref %+.6e  rel %.2e %s\n", what, code, ref, e, bad ? "FAIL" : "ok" );
}

// ---------------------------------------------------------------- counter
// Double model of a Counter<CP_C0> cell's logit z as a function of the
// parameter offsets (du, dv, dx, dy) around its current state.
typedef Counter<CP_C0> C;
struct Model {
  const C& c;
  double wr0c, wr1c;   // the pre-step wr pair the chain uses
  Model( const C& cc ) : c(cc) {
    wr0c = c.wrc.wr0; wr1c = c.wrc.wr1;
  }
  double z( double du, double dv, double dx, double dy ) const {
    double u = c.wr0_state.val + du, v = c.wr1_state.val + dv;
    double wr0 = exp( fmin(fmax(u+v, (double)CP_C0::UVLO), (double)CP_C0::UVHI) );
    double wr1 = exp( fmin(fmax(u-v, (double)CP_C0::UVLO), (double)CP_C0::UVHI) );
    double d0 = wr0 - wr0c, d1 = wr1 - wr1c;
    double n0 = c.n0 + c.rt.n0_w0*d0 + c.rt.n0_w1*d1 + 0.5*c.rt.n0_ww0*d0*d0 + 0.5*c.rt.n0_ww1*d1*d1;
    double n1 = c.n1 + c.rt.n1_w0*d0 + c.rt.n1_w1*d1 + 0.5*c.rt.n1_ww0*d0*d0 + 0.5*c.rt.n1_ww1*d1*d1;
    double q0 = n0 / (n0 + n1 + 1e-8);
    double s  = 1.0/(1.0+exp(-(c.mw_state.val + dx)));
    double mw = CP_C0::Config_MW::minVal + CP_C0::MWspan * s;
    double pm = q0*(1.0-mw) + CP_C0::mwP0*mw;
    double P  = pm*0.999998 + 0.000001;
    double st = log( P/(1.0-P) );
    double K  = exp( c.k_state.val + dy );
    return K*st;
  }
  // z along the un-rotated axes u0 = u+v, u1 = u-v (for h00 + h11)
  double z01( double du0, double du1 ) const { return z( 0.5*(du0+du1), 0.5*(du0-du1), 0, 0 ); }
};
// the codelength in bits of the bit under the logit z
static double codelen( double z, int bit ) { double p = 1.0/(1.0+exp(-z)); return -log2( bit ? 1.0-p : p ); }
static const double LN2 = 0.6931471805599453;

static void test_counter( C cell, int bit, float ef, const char* tag ) {
  printf( "counter %s bit %d ef %+.3f: n0 %.3f n1 %.3f u %.3f v %.4f x %.3f y %.3f age %u\n", tag, bit, ef,
          cell.n0, cell.n1, cell.wr0_state.val, cell.wr1_state.val, cell.mw_state.val, cell.k_state.val, cell.age );
  Model m( cell );
  C::Pred pr = cell.PredictF();
  hooks.clear();
  C work = cell;
  work.C_Update( bit, pr, 1.0f, ef );
  const int nh = (int)hooks.size();
  if( nh < 3 ) { printf( "  only %d Accum calls (K gate?)\n", nh ); }
  const double h = 1e-4;
  const double beta = CP_C0::E2E;
  double z0 = m.z(0,0,0,0);
  check( "PredictF z", pr.z, z0, 1e-5 );
  check( "PredictF pK", pr.pK, 1.0/(1.0+exp(-z0)), 1e-5 );
  // per axis: gradient and exact curvature of L = -ln p(bit) w.r.t. theta
  struct Ax { const char* name; int i; } ax[4] = { {"u",0}, {"v",1}, {"x (mw)",2}, {"y (ln K)",3} };
  for( int a = 0; a < 4 && a < nh; a++ ) {
    double d[4] = {0,0,0,0};
    auto CL = [&]( double t ) { d[ax[a].i] = t; double r = codelen( m.z(d[0],d[1],d[2],d[3]), bit ); d[ax[a].i] = 0; return r; };
    auto Z = [&]( double t ) { d[ax[a].i] = t; double r = m.z(d[0],d[1],d[2],d[3]); d[ax[a].i] = 0; return r; };
    double dL  = LN2 * (CL(h) - CL(-h)) / (2*h);            // dL/dt in nats
    double d2L = LN2 * (CL(h) - 2*CL(0) + CL(-h)) / (h*h);
    double dz  = (Z(h) - Z(-h)) / (2*h);
    Rec r = hooks[a];
    char buf[64];
    snprintf( buf, 64, "gr(%s) = -ln2 dCL/dt", ax[a].name );   check( buf, r.gr, -dL, 2e-3 );
    snprintf( buf, 64, "gd(%s) = A2 blend", ax[a].name ); check( buf, r.gd, -(1-beta)*dL - beta*ef*dz, 2e-3 );
    if( a >= 2 ) {   // mw, K: the exact Hessian
      snprintf( buf, 64, "gr^2-hc(%s) = ln2 d2CL/dt2", ax[a].name ); check( buf, (double)r.gr*r.gr - r.hc, d2L, 2e-2 );
    } else {         // u, v: the shared approximation h00 + h11 in the cell's terms
      double pK = 1.0/(1.0+exp(-z0)), sign = bit ? -1 : 1, p_o = bit ? pK : 1-pK, e_o = sign*p_o, c2 = 1-2*pK;
      double z0p = (m.z01(h,0) - m.z01(-h,0))/(2*h), z1p = (m.z01(0,h) - m.z01(0,-h))/(2*h);
      double z0pp = (m.z01(h,0) - 2*z0 + m.z01(-h,0))/(h*h), z1pp = (m.z01(0,h) - 2*z0 + m.z01(0,-h))/(h*h);
      double hc_ref = e_o*( c2*(z0p*z0p + z1p*z1p) + z0pp + z1pp );
      snprintf( buf, 64, "hc(%s) = e_o(h00+h11)", ax[a].name ); check( buf, r.hc, hc_ref, 2e-2 );
      double exact = d2L, approx = (double)r.gr*r.gr - r.hc;
      printf( "    (u/v shared-curvature approximation: code d2L %+.4e, exact %+.4e)\n", approx, exact );
    }
  }
}

// ---------------------------------------------------------------- mixer
static void test_mixer( float W, float b, float s1, float s2, int bit ) {
  Mix2<CP_M0> mx; std::vector<Mix2_Cell> t(1); mx.Init( t.data(), 1 );
  t[0].w.val = W; t[0].b.val = b;
  auto zf = [&]( double dW, double db ) { double w = 1.0/(1.0+exp(-(W+dW))); return w*s1 + (1.0-w)*s2 + b + db; };
  // Mix() takes logits; feed s1, s2 inside the clip
  float pf = mx.Mix( 0, s1, s2 );
  hooks.clear(); mx.Update( bit );
  printf( "mixer W %.3f b %.3f s1 %.3f s2 %.3f bit %d: pf %.5f\n", W, b, s1, s2, bit, pf );
  const double h = 1e-4;
  double LW_p = LN2*codelen( zf(h,0), bit ), LW_m = LN2*codelen( zf(-h,0), bit ), L0 = LN2*codelen( zf(0,0), bit );
  double Lb_p = LN2*codelen( zf(0,h), bit ), Lb_m = LN2*codelen( zf(0,-h), bit );
  check( "gr(W) = -ln2 dCL/dW", hooks[0].gr, -(LW_p-LW_m)/(2*h), 2e-3 );
  check( "gr^2-hc(W) = ln2 d2CL/dW2", (double)hooks[0].gr*hooks[0].gr - hooks[0].hc, (LW_p-2*L0+LW_m)/(h*h), 2e-2 );
  check( "gr(b) = -ln2 dCL/db", hooks[1].gr, -(Lb_p-Lb_m)/(2*h), 2e-3 );
  check( "gr^2-hc(b) = ln2 d2CL/db2", (double)hooks[1].gr*hooks[1].gr - hooks[1].hc, (Lb_p-2*L0+Lb_m)/(h*h), 2e-2 );
}

// ---------------------------------------------------------------- end-to-end chain of main()
typedef SSE_Ctr<CP_S0, sse_nb_clamp(CP_S0::NB)> SSE;
static void test_chain( float z1, int bit, const char* tag ) {
  static std::vector<Counter<CP_S0>> tbl; static SSE sse; static Mix2<CP_M0> mx; static std::vector<Mix2_Cell> mt(1);
  static bool init = false;
  if( !init ) {
    tbl.resize( sse_rows(1, CP_S0::HBITS, sse_nb_clamp(CP_S0::NB)) * sse_nb_clamp(CP_S0::NB) );
    sse.Init( tbl.data(), 1 ); mx.Init( mt.data(), 1 ); init = true;
    // train the SSE row and the mixer a little so the cells are not the identity
    uint r = 12345;
    for( int i = 0; i < 3000; i++ ) {
      r = r*1103515245u + 12345u; float zz = float(int((r>>8)&1023) - 512) / 64.0f;
      float z2 = sse.Predict( 0, zz ); float pf = mx.Mix( 0, zz, z2 );
      r = r*1103515245u + 12345u; int bt = ((r>>16)&1023) < uint(pf*1024*0.9f) ? 0 : 1;
      float e_f = pf - float(1-bt);
      sse.Update( bt, e_f*mx.dzf_dz2() ); mx.Update( bt );
    }
  }
  // the pipeline's final loss as a function of the order-1 logit
  // the final codelength (in nats) as a function of the order-1 logit
  auto Lf = [&]( double dz ) {
    float zz = float(z1 + dz); float z2 = sse.Predict( 0, zz ); float pf = mx.Mix( 0, zz, z2 );
    return -log( bit ? 1.0 - (double)pf : (double)pf );
  };
  float z2 = sse.Predict( 0, z1 ); float pf = mx.Mix( 0, z1, z2 );
  float e_f = pf - float(1-bit); float wv = mx.wv;
  float c_1 = mx.dzf_dz1() + mx.dzf_dz2() * sse.dz2_dz1();
  const double h = 1e-3;
  printf( "chain %s: z1 %.3f z2 %.3f pf %.5f w %.3f wt %.3f dz2/dz1 %.3f\n", tag, z1, z2, pf, wv, sse.wt, sse.dz2_dz1() );
  check( "dLf/dz1 = e_f*c_1", e_f*c_1, (Lf(h)-Lf(-h))/(2*h), 5e-3 );
  // the SSE cells: perturb cell j's K coordinate y (z_j scales by e^dy)
  for( int j = 0; j < 2; j++ ) {
    Counter<CP_S0>& cj = sse.c[j];
    float y0 = cj.k_state.val;
    auto Lc = [&]( double dy ) { cj.k_state.val = float(y0+dy); float zz2 = sse.Predict( 0, z1 ); float pf2 = mx.Mix( 0, z1, zz2 ); cj.k_state.val = y0;
                                 return -log( bit ? 1.0 - (double)pf2 : (double)pf2 ); };
    float zj = ( j==0 ? sse.pr0 : sse.pr1 ).z;
    double dLdy = (Lc(h)-Lc(-h))/(2*h);           // = dLf/dz_j * z_j
    float share = j==0 ? sse.sh0 : sse.sh1;       // what SSE_Ctr::Update hands the cell as ef, / (e_f dzf/dz2)
    char buf[64]; snprintf( buf, 64, "dLf/dz_c%d = e_f dzf2 share", j );
    check( buf, e_f*mx.dzf_dz2()*share*zj, dLdy, 5e-3 );
    sse.Predict( 0, z1 ); mx.Mix( 0, z1, z2 );
  }
}

// an SSE cell driven as far as its state allows toward the SSE clip: its
// share must be 0 once |z| >= ln 65535.  (At the tuned constants mwMin
// bounds p_mix below 1 - mwMin(1-mwP0), so |z| <= K*st(that) stays under
// the clip and the case is an interior check; other constants reach it.)
static void test_chain_satcell() {
  std::vector<Counter<CP_S0>> tbl( sse_rows(1, CP_S0::HBITS, sse_nb_clamp(CP_S0::NB)) * sse_nb_clamp(CP_S0::NB) );
  SSE sse; sse.Init( tbl.data(), 1 );
  Mix2<CP_M0> mx; std::vector<Mix2_Cell> mt(1); mx.Init( mt.data(), 1 );
  float z1 = 0.4f; sse.Predict( 0, z1 );
  Counter<CP_S0>& c1 = sse.c[1];
  c1.n0 = 1e6f; c1.n1 = 1e-3f; c1.k_state.val = 0.5f;   // p_mix -> 1, K = 1.65: z well past +-ln 65535
  int bit = 1;
  float z2 = sse.Predict( 0, z1 ); float pf = mx.Mix( 0, z1, z2 ); float e_f = pf - float(1-bit);
  printf( "chain cell at its extreme: z_c1 %.2f (clip %.2f) z2 %.3f share1 %.3f%s\n", sse.pr1.z, sse.zmax, z2, sse.sh1,
          fabsf(sse.pr1.z) < sse.zmax ? "  (clip not reachable at these constants)" : "" );
  float y0 = c1.k_state.val; const double h = 1e-3;
  auto Lc = [&]( double dy ) { c1.k_state.val = float(y0+dy); float zz2 = sse.Predict( 0, z1 ); float pf2 = mx.Mix( 0, z1, zz2 ); c1.k_state.val = y0;
                               return -log( bit ? 1.0 - (double)pf2 : (double)pf2 ); };
  double dLdy = (Lc(h)-Lc(-h))/(2*h);
  sse.Predict( 0, z1 ); mx.Mix( 0, z1, z2 );
  check( "dLf/dz_c1 (cell at extreme)", e_f*mx.dzf_dz2()*sse.sh1*sse.pr1.z, dLdy, 5e-3 );
}

int main() {
  printf( "C0 E2E %.4f  S0 E2E %.4f  mixer zm(Pmin) %.3f  SSE lim %.3f\n", CP_C0::E2E, CP_S0::E2E, log((1.0-CP_M0::Pmin)/CP_M0::Pmin), CP_S0::LIM );
  // --- counters at three ages on a biased random source, both bits, with and without an E2E error
  C cell; cell.Init();
  uint r = 777;
  int marks[3] = { 40, 600, 8000 };
  for( int mi = 0, n = 0; mi < 3; mi++ ) {
    for( ; n < marks[mi]; n++ ) {
      r = r*1103515245u + 12345u;
      int bt = ((r>>16) & 1023) < ((n/400)%2 ? 700u : 200u) ? 0 : 1;   // drifting bias
      C::Pred pr = cell.PredictF(); cell.C_Update( bt, pr, 1.0f, 0.3f*(pr.pK - float(1-bt)) );
    }
    char tag[16]; snprintf( tag, 16, "age%d", marks[mi] );
    test_counter( cell, 0, 0.0f, tag );
    test_counter( cell, 1, 0.0f, tag );
    test_counter( cell, 0, 0.37f, tag );
    test_counter( cell, 1, -0.52f, tag );
  }
  // --- mixer
  test_mixer( 0.3f, 0.1f, 2.0f, -1.0f, 0 );
  test_mixer( -1.2f, -0.4f, -3.0f, 0.5f, 1 );
  test_mixer( 2.5f, 1.9f, 4.0f, 3.0f, 1 );
  // --- end-to-end chain, interior and near the clips
  test_chain( 0.7f, 0, "interior" );
  test_chain( -2.1f, 1, "interior" );
  test_chain( 2.9f, 0, "near SSE lim" );
  test_chain( 12.5f, 1, "beyond mixer clip" );
  test_chain( -12.5f, 0, "beyond mixer clip" );
  test_chain_satcell();
  printf( "%d checks, %d failed\n", ncheck, nfail );
  return nfail != 0;
}
