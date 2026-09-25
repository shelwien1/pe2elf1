
// -------------------------------------------------------------
// sh_SSE.inc -- SSE / APM stage made of Counter<CP> cells.
//
//   row     : NB consecutive cells of the S0 Table(), one row per row
//             context (IDX Index Cx of sh_model-S0.idx): the row for context
//             cx starts at cell cx*NB
//   columns : NB buckets, uniform in stretch(p) over [-LIM, +LIM]
//   cell    : Counter<CP> -- the same n0/n1 + ParamUpdater(wr, mw, K)
//             machinery as the counter tables, with its own bundle CP_S0;
//             identity-initialized with mass T0 (CP::SEEDS)
//
//   Predict(z):   s = clip(z, +-LIM);  x = (s+LIM)(NB-1)/(2 LIM)      (z = the input logit)
//                 j = floor(x);  wt = x-j
//                 z_sse = z_j + wt*(z_j+1 - z_j)          (z_j = the cells' logits)
//                 (the caller squashes; p_sse = sq(z_sse))
//   Update(bit):  UPD 1: c[j].C_Update(bit); c[j+1].C_Update(bit)
//                 UPD 2: the same with weights 1-wt and wt (floored at
//                        UPMIN) -- the interpolated update, each cell
//                        runs its own Newton step on a fractional event
//
// The state is the cells, and the cells are the Table() elements, so the
// table is allocated like every other one (a fixed array in the shipping
// build, new[] in S0_Init() in the tuning build).  SSE<CP> is the mapping
// over a row: it holds nothing, takes the row's cells, and Predict()
// returns what Update() needs in an SSE_Pred the caller keeps.  NB is a
// knob, clamped in the bundle to [2, SSE_NB_MAX]: a folded constant in the
// shipping build (it sizes the table, and the loops over a row unroll), a
// load-time value in the tuning build.  Every other knob is clamped in the
// bundle too (IDX-FORMAT.md sec.5), so opt.pl may sweep any bit pattern
// without crashing the coder.  Design notes and measurements: SSE-DESIGN.md.
// -------------------------------------------------------------


// What Predict() computes and Update() needs back.
template<class CP> struct SSE_Pred {
  typename Counter<CP>::Pred pr0, pr1;   // the two bracketing cells' predictions and intermediates
  int   j;                               // left bucket
  float wt;                              // interpolation weight of c[j+1]
  float z;                               // the output logit z0 + wt*(z1-z0)
  float dzdz;                            // d z_sse / d z_in: (NB-1)/(2 LIM) * (z1-z0), 0 where the input clip binds
  float sh0, sh1;                        // d z_sse / d z_cell: 1-wt and wt, or 0 where the cell's logit clip binds
};

template<class CP> struct SSE {
  typedef Counter<CP> Cell;

  // every bucket at its own probability: the seeds are in the bundle
  static void Init( Cell* c ) { for( int j=0; j<CP::NB; j++ ) c[j].InitN( CP::SEEDS.a[j], CP::SEEDS.b[j] ); }

  // c = the row's NB cells; z = the input logit of P(bit==0)
  static SSE_Pred<CP> Predict( const Cell* c, float z ) {
    SSE_Pred<CP> r;
    float s0 = clamp( z, -CP::ZMAX, CP::ZMAX );
    float s  = clamp( s0, -CP::LIM, CP::LIM );
    float x  = (s + CP::LIM) * CP::QSCALE;
    int   j  = int(x); if( j > CP::NB-2 ) j = CP::NB-2;
    r.j   = j;
    r.wt  = x - float(j);
    r.pr0 = c[j].PredictF();
    r.pr1 = c[j+1].PredictF();
    float z0 = clamp( r.pr0.z, -CP::ZMAX, CP::ZMAX ), z1 = clamp( r.pr1.z, -CP::ZMAX, CP::ZMAX );
    r.dzdz = ( s0 > -CP::LIM && s0 < CP::LIM ) ? CP::QSCALE * (z1 - z0) : 0.0f;
    r.sh0  = ( r.pr0.z > -CP::ZMAX && r.pr0.z < CP::ZMAX ) ? 1.0f - r.wt : 0.0f;
    r.sh1  = ( r.pr1.z > -CP::ZMAX && r.pr1.z < CP::ZMAX ) ? r.wt : 0.0f;
    r.z    = z0 + r.wt*(z1-z0);
    return r;
  }

  // ef = A2 end-to-end error at the SSE output, dL/dz_sse; each cell gets
  // it times the derivative of the output w.r.t. its logit (its
  // interpolation share, 0 where its clip binds).
  static void Update( Cell* c, int bit, const SSE_Pred<CP>& pr, float ef ) {
    Cell& c0 = c[pr.j];
    Cell& c1 = c[pr.j+1];
    if( CP::UPD==1 ) { c0.C_Update(bit, pr.pr0, 1.0f, ef*pr.sh0); c1.C_Update(bit, pr.pr1, 1.0f, ef*pr.sh1); }
    else {
      float g0 = 1.0f-pr.wt, g1 = pr.wt;
      c0.C_Update( bit, pr.pr0, g0<CP::UPMIN ? CP::UPMIN : g0, ef*pr.sh0 );
      c1.C_Update( bit, pr.pr1, g1<CP::UPMIN ? CP::UPMIN : g1, ef*pr.sh1 );
    }
  }
};
