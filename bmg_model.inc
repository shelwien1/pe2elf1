// bmg_model.inc -- the plane model.
//
// The shape is ALGORITHM_v2.md's three models taken apart and put back
// together as one:
//
//   * model B's MED / LOCO-I prediction (9.1) and its activity measure over
//     neighbour magnitudes, quantised by two deliberately different ladders
//     (9.2), with nine ternary features combined mixed-radix base 3;
//   * model A's reading of the neighbourhood as *equalities* rather than as
//     quantities -- the six pairwise equalities among N, W, NW, NE, whose
//     transitive closure is the fifteen partitions of a four-set (8.3) -- and
//     a match model, which is what its four-way ladder and run mode are for
//     (8.5) in a form that suits a residual coder;
//   * model B's residual coding: zigzag-folded, a short literal ladder, then
//     an escape into a level-and-path tree (9.3, 5.4);
//   * section 9.4's neighbouring-activity updates.
//
// What is deliberately absent is section 10's learned predictor.  An integer
// NLMS filter over the neighbourhood errors was built, measured and removed:
// once the section 7.1 colour transform is doing the cross-plane work in the
// value domain, the filter had nothing left to find and cost 0.8% on the
// corpus at every step size tried.  So did a JPEG-LS bias corrector.  Both
// findings are in BMG-FORMAT.md rather than in the code.
//
// Where BMF trains neighbouring strips by writing to them because nothing
// blends predictions at read time (5.5), bmg mixes seven context models in the
// logistic domain and keeps the neighbour writes as well.
//
// Planes are coded interleaved -- plane 0,1,2,3 of a pixel before moving on --
// which is what lets a plane's context see what the other planes just did at
// the same pixel (section 9's interleaved driver).

#ifndef BMG_NLIT
 #define BMG_NLIT 12
#endif
static const int NLIT  = BMG_NLIT;   // literal codes 0..NLIT-1, then escape
static const int NMIX  = 13;     // mixer inputs
static const int MARG  = 8;      // row margin, in samples

static const int M1SZ  = 2187*8*3;          // t1..t7 x alvl x pclass
#ifndef M2LOG
 #define M2LOG 19
#endif
static const int M2SZ  = 1<<M2LOG;          // hashed, everything
static const int M3SZ  = 9*9*8*8*3;         // qeW x qeN x qpred x alvl x pclass
static const int M4SZ  = 8*3*8*9;           // alvl x pclass x qrun x qeW
static const int M5SZ  = 9*9*8*3;           // cross-plane residuals
static const int M6SZ  = 15*3*8*3;          // partition x MED branch x alvl x pclass
static const int M7SZ  = 3*8*3;             // match state x match length x pclass
static const int M8SZ  = 256*8*3;           // low nibbles of W and N x alvl x pclass
#ifndef M9LOG
 #define M9LOG 18
#endif
#ifndef MALOG
 #define MALOG 19
#endif
static const int M9SZ  = 1<<M9LOG;          // the exact neighbourhood, hashed
static const int MASZ  = 1<<MALOG;          // the wider neighbourhood, hashed
static const int NSTRIP = 3*8*9*8;          // escape strips
static const int STRIPW = 8*128;            // one path tree per level
static const int E2SZ   = 15*3*8*3;         // partition x MED branch x alvl x pclass
static const int E3LOG  = 16;
static const int E3SZ   = 1<<E3LOG;
static const int EPLOG   = 18;
static const int EPSZ    = 1<<EPLOG;        // second escape-path model
static const int EMXSETS = 7*8*3*8;
static const int PMXSETS = 8*8*3*8;
static const int EAPSETS = 7*8*3*9;
static const int MXSETS  = NLIT*8*3*8*3*3;      // first layer, gradient state
static const int MXSETSB = NLIT*15*9*3;         // first layer, a different cut
static const int MX2SETS = NLIT*3*3*8;          // second layer
static const int APSETS = NLIT*8*3*9;
static const int AP2SETS = NLIT*3*8*3*3;
static const int MHLOG  = 19;               // match table, per plane
static const int NCAND  = 20;               // candidate predictors mixed by LMS
static const int NTX    = 12;               // texture buckets selecting a weight set
static const int LWSETS = 3*NTX;

static Rangecoder rc;

#ifdef BMG_STATS
static double st_bits[8];
static qword  st_hist[4][256];
static qword  st_n[4], st_mhit[2];
static qword  st_hmed[256], st_hlms[256], st_ncorr, st_acorr;
static INLINE void st_add( int k, uint p15, uint bit ) {
  double p = bit ? (1.0 - p15/32768.0) : (p15/32768.0);
  st_bits[k] -= log(p)/log(2.0);
}
#else
 #define st_add(a,b,c)
#endif

template<int f_DEC> static INLINE uint bit_code( Counter& c, uint bit, int wr, int mw ) {
  bit = rc.rc_BProcess( c.P, bit );
  c.Update( bit, wr, mw );
  return bit;
}

// A plain 8-bit binary tree over 256 counters; used by everything that codes a
// byte without a real model behind it.
template<int f_DEC> static uint byte_code( Counter* t, uint v, int wr, int mw ) {
  uint ctx = 1;
  for( int i=7; i>=0; i-- ) {
    uint b = (v>>i)&1;
    b = bit_code<f_DEC>( t[ctx], b, wr, mw );
    ctx += ctx + b;
  }
  return ctx & 255;
}

// The fifteen signatures a set of pairwise equalities on four neighbours can
// have.  A set of pairwise equalities is an equivalence relation, so only the
// patterns closed under transitivity occur -- the partitions of a 4-set, and
// there are Bell(4) = 15 of them.  Section 8.3 derives the same list and finds
// it is exactly BMF's ctx_group_flags, in that order.
static byte part_of[64];
static void init_partitions( void ) {
  static const int flags[15] = { 0,1,2,4,8,10,13,16,17,22,32,35,36,56,63 };
  for( int i=0; i<64; i++ ) part_of[i] = 0;
  for( int i=0; i<15; i++ ) part_of[ flags[i] ] = byte(i);
}

struct ImgModel {
  int W, H, NP, stride, rows;
  int order[4], refA[4], refB[4], pclass[4];
  int tw0[4], tw1[4], tdc[4];   // section 7.1 colour transform, per plane

  byte*  vb[4];  byte*  v[4];      // transformed planes (what the model sees)
  byte*  ab[4];  byte*  a[4];      // actual sample values, for reconstruction
  short* eb[4];  short* e[4];      // signed prediction errors, padded
  byte*  mb[4];  byte*  m[4];      // |error|, clipped to 255

  Counter *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *t9, *ta, *tesc;
  // A second, slow-rate counter on the two strongest models.  One rate has to
  // be a compromise between tracking and averaging; two let the mixer pick,
  // per context state, which of the two answers to believe.
  Counter *t9s, *tas;
  byte *n1_, *n2_, *n3_, *n4_, *n5_, *n6_, *n8_, *n9_, *na_;   // per-row use counts
  byte *ne1, *ne2, *ne3, *nep;
  Counter *el1, *el2, *el3;        // the escape level, mixed like the ladder
  Counter *ep2;                    // second escape-path model
  // Two layers.  One gated mixer answers with the weights of whatever
  // selector it was given; two of them, gated on different things, disagree
  // where the selectors disagree, and a second layer learns which to believe.
  Mixer<NMIX> mx, mxb;
  Mixer<3>    mx2;
  Mixer<4>    emx;
  Mixer<3>    pmx;
  APM  apm, apm2, eapm;

  uint* mtab[4];                   // match model: context hash -> last position
  int   mlen[4];
  int   runlen[4];
  int   usefilt[4];     // does this plane code against the filter or against MED?

  // Section 10.1's filter, in integers.  Model C does not correct a fixed
  // predictor -- it mixes candidate *predictions* with NLMS weights, picks the
  // weight set by a quantised description of the texture, and normalises each
  // tap by its own running power.  That last part is what makes it work: the
  // candidates have wildly different variances, so one shared normalisation
  // lets the loud taps drown the quiet ones.
  int  lw[LWSETS][NCAND];
  int  lpw[LWSETS][NCAND];

  byte level_of[512], group_of[512];
  int  dzof[8];
  qword tbl_bytes;

  // ------------------------------------------------------------- set-up

  static void ladder( byte* out, const int* edges ) {
    int e[7];
    for( int i=0; i<7; i++ ) e[i] = edges[i];
    for( int i=0; i<7; i++ ) { if( e[i] < 1 ) e[i] = 1; if( e[i] > 511 ) e[i] = 511; }
    for( int i=1; i<7; i++ ) if( e[i] <= e[i-1] ) e[i] = e[i-1]+1;
    for( int i=0; i<7; i++ ) if( e[i] > 511 ) e[i] = 511;
    int lvl = 0;
    for( int q=0; q<512; q++ ) {
      while( lvl<7 && q >= e[lvl] ) lvl++;
      out[q] = byte(lvl);
    }
  }

  void alloc( int w, int h, int np ) {
    W = w; H = h; NP = np;
    stride = W + 2*MARG;
    rows   = H + MARG;
    qword n = (qword)stride * rows;
    for( int p=0; p<NP; p++ ) {
      vb[p] = new byte [n];  memset( vb[p], 128, n );
      ab[p] = new byte [n];  memset( ab[p], 128, n );
      eb[p] = new short[n];  memset( eb[p], 0, n*sizeof(short) );
      mb[p] = new byte [n];  memset( mb[p], 0, n );
      v[p] = vb[p] + (qword)MARG*stride + MARG;
      a[p] = ab[p] + (qword)MARG*stride + MARG;
      e[p] = eb[p] + (qword)MARG*stride + MARG;
      m[p] = mb[p] + (qword)MARG*stride + MARG;
      mtab[p] = new uint[(qword)1<<MHLOG];
      memset( mtab[p], 0xFF, sizeof(uint)<<MHLOG );
      mlen[p] = 0; runlen[p] = 0;
    }

    t1   = new Counter[ (qword)M1SZ*NLIT ];
    t2   = new Counter[ (qword)M2SZ*NLIT ];
    t3   = new Counter[ (qword)M3SZ*NLIT ];
    t4   = new Counter[ (qword)M4SZ*NLIT ];
    t5   = new Counter[ (qword)M5SZ*NLIT ];
    t6   = new Counter[ (qword)M6SZ*NLIT ];
    t7   = new Counter[ (qword)M7SZ*NLIT ];
    t8   = new Counter[ (qword)M8SZ*NLIT ];
    t9   = new Counter[ (qword)M9SZ*NLIT ];
    ta   = new Counter[ (qword)MASZ*NLIT ];
    t9s  = new Counter[ (qword)M9SZ*NLIT ];
    tas  = new Counter[ (qword)MASZ*NLIT ];
    n1_ = new byte[M1SZ]; memset( n1_, 0, M1SZ );
    n2_ = new byte[M2SZ]; memset( n2_, 0, M2SZ );
    n3_ = new byte[M3SZ]; memset( n3_, 0, M3SZ );
    n4_ = new byte[M4SZ]; memset( n4_, 0, M4SZ );
    n5_ = new byte[M5SZ]; memset( n5_, 0, M5SZ );
    n6_ = new byte[M6SZ]; memset( n6_, 0, M6SZ );
    n8_ = new byte[M8SZ]; memset( n8_, 0, M8SZ );
    n9_ = new byte[M9SZ]; memset( n9_, 0, M9SZ );
    na_ = new byte[MASZ]; memset( na_, 0, MASZ );
    ne1 = new byte[NSTRIP]; memset( ne1, 0, NSTRIP );
    ne2 = new byte[E2SZ];   memset( ne2, 0, E2SZ );
    ne3 = new byte[E3SZ];   memset( ne3, 0, E3SZ );
    nep = new byte[EPSZ];   memset( nep, 0, EPSZ );
    init_wrtab( P0_CT_wr );
    tesc = new Counter[ (qword)NSTRIP*STRIPW ];
    el1  = new Counter[ (qword)NSTRIP*8 ];
    el2  = new Counter[ (qword)E2SZ*8 ];
    el3  = new Counter[ (qword)E3SZ*8 ];
    ep2  = new Counter[ (qword)EPSZ ];
    tbl_bytes = ( (qword)(M1SZ+M2SZ+M3SZ+M4SZ+M5SZ+M6SZ+M7SZ+M8SZ+2*M9SZ+2*MASZ)*NLIT
                + (qword)NSTRIP*STRIPW + (qword)NSTRIP*8
                + (qword)E2SZ*8 + (qword)E3SZ*8 + (qword)EPSZ ) * sizeof(Counter)
                + (qword)NP*sizeof(uint)*((qword)1<<MHLOG);

    // A fresh literal-ladder counter starts biased toward "yes": the folded
    // residual distribution is monotone, so the first slot really is the most
    // likely one.  BMF seeds its seven-slot node {8,2,2,2,2,3,3} for the same
    // reason (5.3).
    const uint seed = hSCALE + (hSCALE>>2);
    ctr_fill( t1, (qword)M1SZ*NLIT, seed );
    ctr_fill( t2, (qword)M2SZ*NLIT, seed );
    ctr_fill( t3, (qword)M3SZ*NLIT, seed );
    ctr_fill( t4, (qword)M4SZ*NLIT, seed );
    ctr_fill( t5, (qword)M5SZ*NLIT, seed );
    ctr_fill( t6, (qword)M6SZ*NLIT, seed );
    ctr_fill( t7, (qword)M7SZ*NLIT, seed );
    ctr_fill( t8, (qword)M8SZ*NLIT, seed );
    ctr_fill( t9, (qword)M9SZ*NLIT, seed );
    ctr_fill( ta, (qword)MASZ*NLIT, seed );
    ctr_fill( t9s, (qword)M9SZ*NLIT, seed );
    ctr_fill( tas, (qword)MASZ*NLIT, seed );
    ctr_fill( tesc, (qword)NSTRIP*STRIPW, hSCALE );
    ctr_fill( el1, (qword)NSTRIP*8, seed );
    ctr_fill( el2, (qword)E2SZ*8, seed );
    ctr_fill( el3, (qword)E3SZ*8, seed );
    ctr_fill( ep2, (qword)EPSZ, hSCALE );

    memset( lw, 0, sizeof(lw) );
    for( int q=0; q<4; q++ ) usefilt[q] = 0;
    for( int s=0; s<LWSETS; s++ ) for( int j=0; j<NCAND; j++ ) lpw[s][j] = 64;
    mx.alloc( MXSETS, P0_MX_W0 );
    mxb.alloc( MXSETSB, P0_MX_W0 );
    mx2.alloc( MX2SETS, (1<<16)/2 );
    apm.alloc( APSETS );
    apm2.alloc( AP2SETS );
    emx.alloc( EMXSETS, P0_MX_W0*2 );
    pmx.alloc( PMXSETS, P0_MX_W0*3 );
    eapm.alloc( EAPSETS );

    const int ae[7] = { P0_ACT_E0,P0_ACT_E1,P0_ACT_E2,P0_ACT_E3,P0_ACT_E4,P0_ACT_E5,P0_ACT_E6 };
    const int ge[7] = { P0_GRP_E0,P0_GRP_E1,P0_GRP_E2,P0_GRP_E3,P0_GRP_E4,P0_GRP_E5,P0_GRP_E6 };
    ladder( level_of, ae );
    ladder( group_of, ge );
    const int dz[8] = { P0_DZ0,P0_DZ1,P0_DZ2,P0_DZ3,P0_DZ4,P0_DZ5,P0_DZ6,P0_DZ7 };
    for( int i=0; i<8; i++ ) { int d = dz[i]; if( d<0 ) d=0; if( d>64 ) d=64; dzof[i] = d; }

    set_hub( 1 );
  }

  // Which plane leads and what references what.  Section 6.3 picks this by
  // costing three candidate orderings; hub is the plane every other one is
  // predicted from, and it goes first because nothing can predict it.
  void set_hub( int hub ) {
    for( int p=0; p<4; p++ ) { order[p]=p; refA[p]=-1; refB[p]=-1; pclass[p]=0;
                               tw0[p]=0; tw1[p]=0; tdc[p]=0; }
    if( NP < 3 ) return;
    if( hub < 0 || hub > 2 ) hub = 1;
    int k = 0;
    order[k++] = hub;
    pclass[hub] = 0;
    int prev = -1;
    for( int p=0; p<3; p++ ) if( p != hub ) {
      order[k++] = p;
      refA[p] = hub; refB[p] = prev; pclass[p] = 1;
      prev = p;
    }
    if( NP == 4 ) { order[3]=3; refA[3]=hub; refB[3]=prev; pclass[3]=2; }
  }

  // The blend the section 7.1 transform subtracts: a weighted sum of the
  // reference planes' *values* at this same pixel, plus the plane's DC.  Done
  // in the value domain and modulo 256, which is what makes a colour plane
  // cheap -- subtracting the reference's *residual* instead measures three
  // times worse on the corpus, because the residuals are not the correlated
  // part.
  INLINE int blend_of( int p, const int* cur ) {
    int ra = refA[p];
    if( ra < 0 ) return tdc[p];
    int s = tw0[p]*cur[ra];
    int rb = refB[p];
    if( rb >= 0 ) s += tw1[p]*cur[rb];
    return rsh( s, 7 ) + tdc[p];
  }

  void free_( void ) {
    for( int p=0; p<NP; p++ ) {
      delete[] vb[p]; delete[] ab[p]; delete[] eb[p]; delete[] mb[p]; delete[] mtab[p];
    }
    delete[] t1; delete[] t2; delete[] t3; delete[] t4;
    delete[] t5; delete[] t6; delete[] t7; delete[] t8; delete[] t9;
    delete[] ta; delete[] t9s; delete[] tas; delete[] tesc;
    delete[] el1; delete[] el2; delete[] el3; delete[] ep2;
    delete[] n1_; delete[] n2_; delete[] n3_; delete[] n4_;
    delete[] n5_; delete[] n6_; delete[] n8_; delete[] n9_; delete[] na_;
    delete[] ne1; delete[] ne2; delete[] ne3; delete[] nep;
    mx.free_(); mxb.free_(); mx2.free_(); apm.free_(); apm2.free_();
    emx.free_(); eapm.free_(); pmx.free_();
  }

  // ------------------------------------------------------------- helpers

  static INLINE int med( int Wv, int Nv, int NWv ) {
    int mx_ = Wv>Nv ? Wv : Nv, mn = Wv>Nv ? Nv : Wv;
    if( NWv >= mx_ ) return mn;
    if( NWv <= mn  ) return mx_;
    return Wv + Nv - NWv;
  }
  static INLINE int tern( int d, int dz ) { return (d < -dz) ? 0 : ((d <= dz) ? 1 : 2); }
  static INLINE int qerr( int x ) {
    int s = x<0 ? -1 : 1, a = x<0 ? -x : x;
    int b = (a==0) ? 0 : (a<=1 ? 1 : (a<=3 ? 2 : (a<=7 ? 3 : 4)));
    return 4 + s*b;
  }
  static INLINE uint zigzag( int e ) {
    int f = ((e + 128) & 255) - 128;          // e mod 256, folded to [-128,127]
    return (f >= 0) ? uint(2*f) : uint(-2*f - 1);
  }
  static INLINE int unzigzag( uint c ) { return (c & 1) ? -int((c+1)>>1) : int(c>>1); }

  // Left margin of the row about to be coded is the leftmost sample of the row
  // just finished; the right margin is that row's last sample.  Section 10.5:
  // fill the margins and the predictor needs no edge case at all.
  void row_margins( int y ) {
    if( y == 0 ) return;
    for( int p=0; p<NP; p++ ) {
      qword i0 = (qword)y*stride;
      byte  vv = v[p][(qword)(y-1)*stride];
      short ee = e[p][(qword)(y-1)*stride];
      byte  mm = m[p][(qword)(y-1)*stride];
      for( int k=1; k<=MARG; k++ ) { v[p][i0-k]=vv; e[p][i0-k]=ee; m[p][i0-k]=mm; }
    }
  }
  void row_close( int y ) {
    for( int p=0; p<NP; p++ ) {
      qword i0 = (qword)y*stride + W;
      byte  vv = v[p][i0-1];
      short ee = e[p][i0-1];
      byte  mm = m[p][i0-1];
      for( int k=0; k<MARG; k++ ) { v[p][i0+k]=vv; e[p][i0+k]=ee; m[p][i0+k]=mm; }
    }
  }

  // ------------------------------------------------------------- one sample

  template<int f_DEC>
  INLINE void sample( int p, qword i, int* cur ) {
    int blend = blend_of( p, cur );
    int val = f_DEC ? 0 : ((v[p][i] - blend) & 255);   // the transformed sample
    byte*  vp = v[p];
    short* ep = e[p];
    byte*  mp = m[p];
    const qword s1 = stride, s2 = 2*(qword)stride;

    int Wv = vp[i-1], Nv = vp[i-s1], NWv = vp[i-s1-1], NEv = vp[i-s1+1];
    int NNv = vp[i-s2], WWv = vp[i-2], NEEv = vp[i-s1+2], NNEv = vp[i-s2+1];
    int NWWv = vp[i-s1-2], NNWv = vp[i-s2-1];

    int eW = ep[i-1], eN = ep[i-s1];
    int ra = refA[p], rb = refB[p];
    int eRA = (ra>=0) ? e[ra][i] : 0;
    int eRB = (rb>=0) ? e[rb][i] : 0;

    // MED, and which of its three branches answered -- the branch is a
    // statement about the local edge and costs nothing to carry.
    int mxv = Wv>Nv ? Wv : Nv, mnv = Wv>Nv ? Nv : Wv;
    int pmed, mtype;
    if( NWv >= mxv )      { pmed = mnv;           mtype = 0; }
    else if( NWv <= mnv ) { pmed = mxv;           mtype = 1; }
    else                  { pmed = Wv + Nv - NWv; mtype = 2; }
    if( pmed < 0 ) pmed = 0; else if( pmed > 255 ) pmed = 255;
    int pred = pmed;

    // ---- the candidate predictors, and the filter that mixes them
    int cd[NCAND];
    cd[ 0] = Wv;
    cd[ 1] = Nv;
    cd[ 2] = NEv;
    cd[ 3] = NWv;
    cd[ 4] = Wv + Nv - NWv;
    cd[ 5] = (Wv + Nv + 1) >> 1;
    cd[ 6] = Wv + NEv - Nv;
    cd[ 7] = 2*Wv - WWv;
    cd[ 8] = 2*Nv - NNv;
    cd[ 9] = (3*(Wv + Nv) - 2*NWv + 2) >> 2;
    cd[10] = Nv + NEv - NNEv;
    cd[11] = Wv + NWv - NWWv;
    cd[12] = (Wv + NEv + 1) >> 1;
    cd[13] = 2*NEv - NEEv;
    cd[14] = NNv + Wv - NNWv;
    cd[15] = (NEv + NWv + 1) >> 1;
    // Section 10.1's rows 4 to 6 are cross-plane terms.  Here the reference
    // planes have already been through the colour transform, so what they have
    // left to say at this pixel is their own residual.
    cd[16] = pmed + eRA;
    cd[17] = pmed + eRB;
    cd[18] = pmed + rsh( eRA + eRB, 1 );
    cd[19] = (Wv + Nv + NEv + NWv + 2) >> 2;

    int spread = iabs(Wv-Nv) + iabs(Nv-NEv) + iabs(Wv-NWv) + iabs(Nv-NWv);
    int tx = ilog2i( (uint)spread + 1 ); if( tx > NTX-1 ) tx = NTX-1;
    int ws = pclass[p]*NTX + tx;
    int* lwp = lw[ws];
    int* lpp = lpw[ws];

    int dd[NCAND];
    long long dot = 0;
    for( int j=0; j<NCAND; j++ ) {
      int d = cd[j] - pmed;
      dd[j] = d;
      dot += (long long)lwp[j]*d;
    }
    int plms = pmed + rsh64( dot, 16 );
    if( plms < 0 ) plms = 0; else if( plms > 255 ) plms = 255;

    // Coding against the filter moves the prediction out from under the
    // contexts that carry the exact neighbourhood, which is where a
    // screenshot's compression lives -- so when the filter is in use, its
    // correction goes into those contexts and they are deterministic again.
    int lqx = 0;
    if( usefilt[p] ) {
      pred = plms;
      int c = plms - pmed;
      lqx = 1 + ((c < -8) ? 0 : (c > 8) ? 16 : c + 8);
    }

    // activity
    int act = P0_AW_W  * mp[i-1]    + P0_AW_N  * mp[i-s1]
            + P0_AW_NW * mp[i-s1-1] + P0_AW_NE * mp[i-s1+1]
            + P0_AW_NN * mp[i-s2]   + P0_AW_WW * mp[i-2]
            + mp[i-s2+1] + mp[i-s1+2]
            + P0_AW_X  * ( (ra>=0 ? m[ra][i] : 0) + (rb>=0 ? m[rb][i] : 0) );
    int aq = (act + 7) >> 4;
    if( aq > 511 ) aq = 511;
    int alvl = level_of[aq], gA = group_of[aq];
    int dz = dzof[alvl];

    int t0 = (pred < 22) ? 0 : ((pred > 216) ? 2 : 1);
    int t1_ = tern( NWv - Nv, dz );
    int t2_ = tern( NWv - Wv, dz );
    int t3_ = tern( NNv - pred, dz );
    int t4_ = tern( NEv - pred, dz );
    int t5_ = tern( 2*Wv - WWv - pred, dz );
    int t6_ = tern( 2*Nv - NNv - pred, dz );
    int t7_ = tern( NEv + Wv - Nv - pred, dz );
    int t8_ = tern( eRA, dz );

    // the six pairwise equalities among N, W, NW, NE, folded to their fifteen
    // partitions (section 8.3)
    int sig = (Nv==NWv) | ((Wv==NWv)<<1) | ((NEv==NWv)<<2)
            | ((NEv==Nv)<<3) | ((Wv==NEv)<<4) | ((Wv==Nv)<<5);
    int part = part_of[sig];

    int qeW = qerr( eW ), qeN = qerr( eN );
    int qpr = pred >> 5;
    int pc  = pclass[p];
    int qrn = ilog2i( (uint)runlen[p] + 1 ); if( qrn > 7 ) qrn = 7;

    // ---- match model: what followed this exact neighbourhood last time
    uint mh = ( (uint)Wv * 0x9E3779B1u ) ^ ( (uint)Nv * 0x85EBCA6Bu )
            ^ ( (uint)NWv * 0xC2B2AE35u ) ^ ( (uint)NEv * 0x27D4EB2Fu )
            ^ ( (uint)WWv * 0x165667B1u ) ^ ( (uint)NNv * 0x9E3779B9u )
            ^ ( (uint)NEEv * 0x7FEB352Du ) ^ ( (uint)NNEv * 0x846CA68Bu );
    mh ^= mh >> 15;
    uint mfull = mh;                 // the same eight neighbours, as a context
    mh &= (1u<<MHLOG) - 1;
    uint mpos = mtab[p][mh];
    int  mvalid = (mpos != 0xFFFFFFFFu);
    uint mcode = 0xFFFFu;
    if( mvalid ) mcode = zigzag( (int)vp[mpos] - pred );
    int mlq = ilog2i( (uint)mlen[p] + 1 ); if( mlq > 7 ) mlq = 7;

    // ---- context rows
    int c1 = ((((((t1_*3+t2_)*3+t3_)*3+t4_)*3+t5_)*3+t6_)*3+t7_);
    qword r1 = (qword)(c1*8 + alvl)*3 + pc;
    uint  hh = (uint)c1 * 0x9E3779B1u;
    hh ^= (uint)((t0*3 + t8_)*15 + part) * 0x85EBCA6Bu;
    hh ^= (uint)(((alvl*3 + pc)*9 + qeW)*9 + qeN) * 0xC2B2AE35u;
    hh ^= (uint)(((qpr*3 + mtype)*8 + gA)*18 + lqx) * 0x27D4EB2Fu;
    hh ^= hh >> 15;
    qword r2 = hh & (M2SZ-1);
    qword r3 = (qword)(((qeW*9 + qeN)*8 + qpr)*8 + alvl)*3 + pc;
    qword r4 = (qword)((alvl*3 + pc)*8 + qrn)*9 + qeW;
    qword r5 = (qword)((qerr(eRA)*9 + qerr(eRB))*8 + alvl)*3 + pc;
    qword r6 = (qword)((part*3 + mtype)*8 + alvl)*3 + pc;
    // The low nibbles of the two nearest neighbours.  Section 8.5's ctx_id3
    // refines its context with exactly this and for the same reason: on data
    // that came through a colour quantiser or a palette, the low bits of a
    // neighbour say which cell of the quantiser grid this pixel sits in, which
    // no gradient or activity measure can see.
    qword r8 = (qword)(((Wv&15)*16 + (Nv&15))*8 + alvl)*3 + pc;

    Counter* c1p = t1 + r1*NLIT;   int u1 = n1_[r1], w1_ = wrtab[u1];
    Counter* c2p = t2 + r2*NLIT;   int u2 = n2_[r2], w2_ = wrtab[u2];
    Counter* c3p = t3 + r3*NLIT;   int u3 = n3_[r3], w3_ = wrtab[u3];
    Counter* c4p = t4 + r4*NLIT;   int u4 = n4_[r4];
    Counter* c5p = t5 + r5*NLIT;   int u5 = n5_[r5], w5_ = wrtab[u5];
    Counter* c6p = t6 + r6*NLIT;   int u6 = n6_[r6], w6_ = wrtab[u6];
    // The exact neighbourhood, hashed.  Where the match model offers one
    // prediction from one earlier occurrence, this carries the whole
    // distribution of what has followed this neighbourhood -- which is what
    // model A is doing when it asks "is this pixel one of its neighbours"
    // (section 8.5), written as a context rather than as a ladder.
    uint h9 = (uint)Wv * 0x9E3779B1u;
    h9 ^= (uint)Nv * 0x85EBCA6Bu;
    h9 ^= (uint)NWv * 0xC2B2AE35u;
    h9 ^= (uint)NEv * 0x27D4EB2Fu;
    h9 ^= (uint)(pc*18 + lqx) * 0x165667B1u;
    h9 ^= h9 >> 15;
    uint mfx = mfull ^ ((uint)lqx * 0x85EBCA6Bu);
    qword r9 = h9 & (M9SZ-1), rA = ((mfx*2654435761u)>>4) & (MASZ-1);
    Counter* c8p = t8 + r8*NLIT;   int u8 = n8_[r8], w8_ = wrtab[u8];
    Counter* c9p = t9 + r9*NLIT;   int u9 = n9_[r9], w9_ = wrtab[u9];
    Counter* cap = ta + rA*NLIT;   int uA = na_[rA], wA_ = wrtab[uA];
    Counter* c9s = t9s + r9*NLIT;
    Counter* cas = tas + rA*NLIT;

    // neighbouring activity levels, for the section 9.4 side updates
    Counter* n1lo = (alvl>0) ? t1 + ((qword)(c1*8 + alvl-1)*3 + pc)*NLIT : 0;
    Counter* n1hi = (alvl<7) ? t1 + ((qword)(c1*8 + alvl+1)*3 + pc)*NLIT : 0;

    uint code = f_DEC ? 0 : zigzag( val - pred );

    int k;
    for( k=0; k<NLIT; k++ ) {
      int ms = mvalid ? ((mcode == (uint)k) ? 2 : 1) : 0;
      Counter* c7p = t7 + ((qword)((ms*8 + mlq)*3 + pc))*NLIT;
      mx.st[0] = u1 ? stretch15( c1p[k].P ) : 0;
      mx.st[1] = u2 ? stretch15( c2p[k].P ) : 0;
      mx.st[2] = u3 ? stretch15( c3p[k].P ) : 0;
      mx.st[3] = stretch15( c4p[k].P );
      mx.st[4] = u5 ? stretch15( c5p[k].P ) : 0;
      mx.st[5] = u6 ? stretch15( c6p[k].P ) : 0;
      mx.st[6] = stretch15( c7p[k].P );
      mx.st[7] = u8 ? stretch15( c8p[k].P ) : 0;
      mx.st[8] = u9 ? stretch15( c9p[k].P ) : 0;
      mx.st[9] = uA ? stretch15( cap[k].P ) : 0;
      mx.st[10] = u9 ? stretch15( c9s[k].P ) : 0;
      mx.st[11] = uA ? stretch15( cas[k].P ) : 0;
      mx.st[12] = 256;
      for( int q=0; q<NMIX; q++ ) mxb.st[q] = mx.st[q];
      mx.select( ((((k*8 + alvl)*3 + pc)*8 + qrn)*3 + ms)*3 + mtype );
      mxb.select( ((k*15 + part)*9 + qeW)*3 + pc );
      int pra = mx.mix(), prb = mxb.mix();
      mx2.st[0] = stretch_t[pra];
      mx2.st[1] = stretch_t[prb];
      mx2.st[2] = 256;
      mx2.select( ((k*3 + pc)*3 + ms)*8 + mlq );
      int pr = mx2.mix();
      // Two secondary estimators on different questions -- one on the
      // gradient state, one on what the match model is doing -- averaged with
      // the mixer's own answer.
      int pa  = apm.pp ( pr, ((k*8 + alvl)*3 + pc)*9 + qeW );
      int pa2 = apm2.pp( pr, (((k*3 + ms)*8 + mlq)*3 + pc)*3 + mtype );
      int wt = P0_AP_WT; if( wt < 0 ) wt = 0; if( wt > 16 ) wt = 16;
      int wt2 = P0_AP_WT2; if( wt2 < 0 ) wt2 = 0; if( wt+wt2 > 16 ) wt2 = 16-wt;
      int prf = (pr*(16-wt-wt2) + pa*wt + pa2*wt2) >> 4;
      uint p15 = uint(prf) << 3;
      if( p15 < 8 ) p15 = 8;
      if( p15 > SCALE-8 ) p15 = SCALE-8;

      uint bit = f_DEC ? 0 : (code == (uint)k ? 0u : 1u);
      bit = rc.rc_BProcess( p15, bit );
      st_add( 0, p15, bit );

      mx.update( bit, P0_MX_LR );
      mxb.update( bit, P0_MX_LR );
      mx2.update( bit, P0_MX_LR2 );
      apm.update( bit, P0_AP_RATE );
      apm2.update( bit, P0_AP_RATE );
      c1p[k].Update( bit, w1_, P0_CT_mw );
      c2p[k].Update( bit, w2_, P0_CT_mw );
      c3p[k].Update( bit, w3_, P0_CT_mw );
      c4p[k].Update( bit, P0_CTF_wr, P0_CTF_mw );
      c5p[k].Update( bit, w5_, P0_CT_mw );
      c6p[k].Update( bit, w6_, P0_CT_mw );
      c7p[k].Update( bit, P0_CTF_wr, P0_CTF_mw );
      c8p[k].Update( bit, w8_, P0_CT_mw );
      c9p[k].Update( bit, w9_, P0_CTF_mw );
      cap[k].Update( bit, wA_, P0_CTF_mw );
      c9s[k].Update( bit, (w9_ < P0_CTS_wr) ? w9_ : P0_CTS_wr, P0_CT_mw );
      cas[k].Update( bit, (wA_ < P0_CTS_wr) ? wA_ : P0_CTS_wr, P0_CT_mw );

      if( n1lo ) n1lo[k].Update( bit, P0_CTN_wr, P0_CT_mw );
      if( n1hi ) n1hi[k].Update( bit, P0_CTN_wr, P0_CT_mw );

      if( bit == 0 ) break;
    }

    if( k == NLIT ) {
      // escape: the remaining magnitude through a level and a path, section 5.4.
      // gA, not alvl: section 9.2 keeps the escape ladder deliberately
      // different from the level ladder, so the two quantisers disagree and the
      // strip index carries something the counter index does not.
      qword strip = (qword)(((pc*8 + gA)*9 + qeW)*8 + qpr);
      Counter* sp = tesc + strip*STRIPW;
      uint eh = (uint)c1 * 0x9E3779B1u;
      eh ^= (uint)(((alvl*3 + pc)*9 + qeW)*9 + qeN) * 0x85EBCA6Bu;
      eh ^= (uint)((part*3 + mtype)*3 + t0) * 0xC2B2AE35u;
      eh ^= eh >> 13;
      qword re2 = (qword)((part*3 + mtype)*8 + alvl)*3 + pc, re3 = eh & (E3SZ-1);
      Counter* l1 = el1 + strip*8;   int v1 = ne1[strip], y1 = wrtab[v1];
      Counter* l2 = el2 + re2*8;     int v2 = ne2[re2],   y2 = wrtab[v2];
      Counter* l3 = el3 + re3*8;     int v3 = ne3[re3],   y3 = wrtab[v3];
      if( v1<15 ) ne1[strip]++;  if( v2<15 ) ne2[re2]++;  if( v3<15 ) ne3[re3]++;
      uint vv = f_DEC ? 0 : (code - NLIT + 1);      // 1 .. 256-NLIT
      int lvl = f_DEC ? 0 : ilog2i( vv );
      int j;
      for( j=0; j<7; j++ ) {
        emx.st[0] = v1 ? stretch15( l1[j].P ) : 0;
        emx.st[1] = v2 ? stretch15( l2[j].P ) : 0;
        emx.st[2] = v3 ? stretch15( l3[j].P ) : 0;
        emx.st[3] = 256;
        emx.select( ((j*8 + alvl)*3 + pc)*8 + gA );
        int pr = emx.mix();
        int pa = eapm.pp( pr, ((j*8 + alvl)*3 + pc)*9 + qeW );
        int wt = P0_AP_WT; if( wt < 0 ) wt = 0; if( wt > 16 ) wt = 16;
        int prf = (pr*(16-wt) + pa*wt) >> 4;
        uint p15 = uint(prf) << 3;
        if( p15 < 8 ) p15 = 8;
        if( p15 > SCALE-8 ) p15 = SCALE-8;
        uint bit = f_DEC ? 0 : (lvl == j ? 0u : 1u);
        bit = rc.rc_BProcess( p15, bit );
        st_add( 1, p15, bit );
        emx.update( bit, P0_MX_LR );
        eapm.update( bit, P0_AP_RATE );
        l1[j].Update( bit, y1, P0_EL_mw );
        l2[j].Update( bit, y2, P0_EL_mw );
        l3[j].Update( bit, y3, P0_EL_mw );
        if( bit == 0 ) break;
      }
      lvl = j;
      uint ctx = 1;
      for( int b=lvl-1; b>=0; b-- ) {
        Counter& q1 = sp[lvl*128 + (ctx&127)];
        uint ph = (uint)((lvl*128 + (ctx&127))) * 0x9E3779B1u;
        ph ^= (uint)(((part*3 + mtype)*8 + alvl)*3 + pc) * 0x85EBCA6Bu;
        ph ^= (uint)((Wv&15)*16 + (Nv&15)) * 0xC2B2AE35u;
        ph ^= ph >> 13;
        qword rp = ph & (EPSZ-1);
        Counter& q2 = ep2[ rp ];
        int vp2 = nep[rp], yp = wrtab[vp2];
        if( vp2<15 ) nep[rp]++;
        pmx.st[0] = stretch15( q1.P );
        pmx.st[1] = vp2 ? stretch15( q2.P ) : 0;
        pmx.st[2] = 256;
        pmx.select( ((b*8 + alvl)*3 + pc)*8 + gA );
        int pr = pmx.mix();
        uint p15 = uint(pr) << 3;
        if( p15 < 8 ) p15 = 8;
        if( p15 > SCALE-8 ) p15 = SCALE-8;
        uint bit = f_DEC ? 0 : ((vv>>b)&1);
        bit = rc.rc_BProcess( p15, bit );
        st_add( 2, p15, bit );
        pmx.update( bit, P0_MX_LR );
        q1.Update( bit, P0_EP_wr, P0_EP_mw );
        q2.Update( bit, yp, P0_EP_mw );
        ctx += ctx + bit;
      }
      if( f_DEC ) { vv = ctx; code = vv - 1 + NLIT; }
    } else {
      if( f_DEC ) code = (uint)k;
    }

    val = (pred + unzigzag(code)) & 255;
    cur[p] = (val + blend) & 255;

    int err = val - pred;
    vp[i] = byte(val);
    a[p][i] = byte( cur[p] );
    ep[i] = short(err);
    int ae_ = err<0 ? -err : err;
    mp[i] = byte( ae_>255 ? 255 : ae_ );

#ifdef BMG_STATS
    st_hist[pc][ zigzag(err) ]++; st_n[pc]++;
    st_hmed[ zigzag(val-pmed) ]++; st_hlms[ zigzag(val-plms) ]++;
    st_ncorr++; st_acorr += iabs(plms-pmed);
    if( mvalid ) st_mhit[ (int)vp[mpos]==val ]++;
#endif

    if( u1<15 ) n1_[r1]++;  if( u2<15 ) n2_[r2]++;  if( u3<15 ) n3_[r3]++;
    if( u4<15 ) n4_[r4]++;  if( u5<15 ) n5_[r5]++;  if( u6<15 ) n6_[r6]++;
    if( u8<15 ) n8_[r8]++;  if( u9<15 ) n9_[r9]++;  if( uA<15 ) na_[rA]++;

    // ---- the filter's step, normalised per tap.  A shift rather than a
    // divide: 2^sh tracks the tap's own running power, so the step is the NLMS
    // step to within a factor of two and costs no division at all.
    {
      int lerr = val - plms;
      int lim = P0_LM_LIM, kmu = P0_LM_MU;
      for( int j=0; j<NCAND; j++ ) {
        int d = dd[j];
        lpp[j] += rsh( d*d - lpp[j], 5 );
        int sh = ilog2i( (uint)lpp[j] + 4 ) - kmu;
        if( sh < 0 ) sh = 0; else if( sh > 24 ) sh = 24;
        int nw = lwp[j] + rsh64( (long long)lerr * d, sh );
        if( nw >  lim ) nw =  lim;
        if( nw < -lim ) nw = -lim;
        lwp[j] = nw;
      }
    }

    mlen[p]   = (mvalid && (int)vp[mpos] == val) ? mlen[p]+1 : 0;
    mtab[p][mh] = (uint)i;
    runlen[p] = (err==0) ? runlen[p]+1 : 0;
  }

  // ------------------------------------------------------------- the walk

  template<int f_DEC>
  void run( void ) {
    for( int y=0; y<H; y++ ) {
      row_margins( y );
      qword base = (qword)y*stride;
      for( int x=0; x<W; x++ ) {
        qword i = base + x;
        int cur[4] = {0,0,0,0};
        for( int k=0; k<NP; k++ ) sample<f_DEC>( order[k], i, cur );
      }
      row_close( y );
    }
  }
};
