/* xad_codec.inc -- part of xadpcm.cpp; not standalone.
 *
 * Codec<MD> -- the model.  Assembled from the four parts included in its
 * body: including them here rather than declaring member functions out of line
 * keeps every definition in exactly one place, and keeps the whole model in one
 * translation unit.  Same idiom as d5c_codec.inc. */
/* Codec is templated on the DIRECTION only, deliberately not on the format.
   The obvious extra parameter would be FMT, and it is wrong here: -ss
   (SOLID_ALL) carries counters, mixer weights, APM tables and the match history
   across a wav boundary whatever the codec on either side, and two
   Codec<MD,FMT> objects cannot share one set of tables -- a stream holding an
   MS wav and an IMA wav would silently lose the carry-over that mode exists
   for.  So FMT parameterises the payload WALK below (code_data_ima<BPS,XST>,
   code_data_ms<XST>, code_symbol<FMT,BPS>), where everything it decides still
   folds, while the tables stay in one object.

   `rc` is a reference to the frontend, which IS the range coder (Xad<MD>
   derives from RC<MD>), so every rc.bit() call site reads as it always did. */
template<int MD>
struct Codec {
  enum { enc = (MD==0) };
  RC<MD> &rc;
  explicit Codec(RC<MD> &r) : rc(r) {}
  Params pm;
  /* Two of them, because "the last k codes occurred before" means two different
     things at two scales.  A short window is really a high-order context model:
     it fires constantly, is right rather more often than chance, and gives the
     mixer a view further back than any of the fixed contexts reach.  A long
     window fires rarely and is nearly certain when it does -- that is the one
     that catches a looped sample or a repeated asset.  One model cannot be both:
     the sweep below the short one bottoms out around five codes and the long one
     needs twelve or more, and each setting is clearly worse at the other's job. */
  MatchModel<G0_MATCH_ORDER, G0_MATCH_HB, G0_MATCH_TB> mmS;
  MatchModel<G0_MATCH_ORDER2, G0_MATCH_HB2, G0_MATCH_TB2> mmL;
  int nsym = 0, nbit = 0; // set by init()/begin_seg()
  const int hj_bits = pclamp(G0_HJ_BITS, 8, 24);
  const int sgn_maxnb = pclamp(G0_SGN_MAXNB, 8, 62);
  int nib_rows = 89; // IMA step index buckets, or the MS iDelta ones
  int sgn_ncls = 20; // magnitude classes that get their own mantissa rows

  // --- code models (row width = alphabet, node index 1..alphabet-1)
  CtrTab tQ;       // (q^, conf)
  CtrSTab tQs;     // slow twin of the above
  CtrTab tI;       // step index bucket
  CtrTab tP;       // previous code, same channel
  CtrTab tP2;      // (prev, prev2) hashed
  CtrTab tX;       // other channel at the same instant
  CtrTab tQP;      // (q^, prev) hashed
  CtrTab tQI;      // (q^, index bucket)
  CtrTab tQX;      // (q^, other channel)
  CtrTab tIP;      // (index bucket, prev, conf)
  CtrTab tQ2;      // short-horizon predicted code (stages 1+2 only)
  CtrTab tQ3;      // shortest view (stage 1 only)
  CtrTab tMS, tML; // match: (length bucket, expected bit), row 0 = no prediction

  /* Joint contexts: own code history crossed with the other channel's.  Every
     context above this point is either purely own-channel or a bare look at the
     other one, and that is the structural difference from a byte-oriented
     general-purpose CM -- such a coder sees an (L,R) nibble pair as one symbol,
     so its order-N byte context is automatically a joint stereo history several
     instants deep.  Crossing the two here is what closes that gap; measured
     together they are worth 2.3% on real music. */
  CtrTab tPX;   // (own previous, other channel now) -- the joint pair
  CtrTab tXX;   // (other channel now, other channel before) -- its trajectory
  CtrTab tPPX;  // (own previous two, other channel now)
  CtrTab tPX2;  // (own previous, other channel now and before)
  CtrTab tP3;   // own previous three codes
  CtrTab tPPXX; // (own previous two, other channel now and before)
  CtrTab tP4;   // own previous four codes
  CtrTab tQPX;  // (q^, own previous, other channel now)

  /* Deeper than direct indexing can reach: own last three codes crossed with
     the other channel's last two would want 1.2M rows.  At that depth most
     cells are never visited, so it is hashed into a fixed table instead and a
     collision costs less than the context is worth. */
  CtrTab tHJ;

  /* The second cascade's own view, plus the agreement between the two.  When
     both shapes land on the same code the next symbol is far more predictable
     than either alone implies, and that is exactly what the single-cascade
     model could not express. */
  CtrTab tQB;  // (q^ from cascade B, its confidence)
  CtrTab tQAB; // (q^ from A, q^ from B)
  Mixer<G0_MLR, G0_W_CLAMPL> mxC;
  APM<G0_AP_N_A, G0_AP_R_A> apA;
  APM<G0_AP_N_B, G0_AP_R_B> apB;

  // --- block headers / literals
  CtrTab tHnb, tHmn, tRsv, tLit;

  struct Ch {
    Pred<0> pd;
    Pred<1> pdB;
    i32 pcm = 0;
    int index = 0, prev = 0, prev2 = 0, prev3 = 0, prev4 = 0, cur = 0;
    // MS: the decoder state plus what the previous block header held and what
    // the previous block's last nibble left iDelta at
    i32 samp1 = 0, samp2 = 0, delta = 16, hdelta = 16, rdelta = 16, c1 = 256, c2 = 0;
    /* delta clamped to >=1, and its reciprocal.  Kept next to delta and
       refreshed by set_delta() wherever delta moves, so the four quantizer
       calls per nibble cost no division at all -- one per nibble, where it used
       to be six per sample-channel. */
    i32 dl = 16;
    u32 rcp = 0;
    u64 errR = 0, errH = 0;
    int bprev = 0;

    INLINE void set_delta(i32 v) {
      delta = v;
      dl = v<1 ? 1 : v;
      rcp = ms_rcp(dl);
    }
  } ch[MAX_CHANS];

  /* v4 §1.13 -- IMA reconstruction as a table.  The magnitude a code adds and
     the index it moves to depend only on (index, code) for a fixed bps, so both
     collapse to a lookup: the code_magnitude loop and the index_update switch
     leave the apply path entirely.  The sign is folded into the stored value.
     clip16 cannot be folded in -- pcm+mag genuinely leaves i16 range and the
     saturation is part of the reconstruction.
     There is no MS equivalent: ms_apply and ms_delta_update are functions of
     iDelta, a runtime value over [1, 2796202]. */
  i32 ima_mag[89][32];
  u8 ima_nxt[89][32];

  template<int BPS>
  void build_ima_tables() {
    for( int i = 0; i<89; i++ )
      for( int c = 0; c<(1<<BPS); c++ ) {
        int m = code_magnitude<BPS>(c, step_table[i]);
        ima_mag[i][c] = (c&(1<<(BPS-1))) ? -m : m;
        ima_nxt[i][c] = u8(index_update<BPS>(i, c));
      }
  }

  u64 stat[ST_NCAT] = {};
  int scat = 0;

  /* Allocation and per-segment reset are separate because a solid stream holds
     several wavs: the tables are sized ONCE from the widest geometry any
     segment needs and then keep their statistics across segment boundaries,
     while the signal-domain state -- ADPCM decoder, NLMS filters, channel
     history -- is reset at each boundary because the new wav's samples have
     nothing to do with the old one's.  That split is the whole feature.

     Sizing from the maximum rather than per-segment matters: contexts are
     indexed with the SEGMENT's nsym and nib_rows, which are <= the allocated
     ones, so a 4-bit wav after a 5-bit one still lands on the rows it would
     have hit alone.  When every segment shares one geometry the maxima equal
     it and the layout is exactly what a single-file run produces. */
  /* Below this many payload bytes, leave the match arena demand-zero; above it,
     fault it in with one sequential pass.
     The tables are ~512 MB and probed at random, so which is cheaper depends
     entirely on how much of them the run will touch.  Measured both ways: on a
     20 KB input lazy wins by 39% (20 runs: 16.55 s -> 10.15 s), because most of
     the half gigabyte is never looked at; on a 2.5 MB input eager wins by 8%
     (8.66 s vs 9.34 s), because every page gets touched either way and one
     streaming fill beats ~131k random demand faults.  The crossover is where
     the symbol count approaches the table's page count (2^16 per model), which
     is a few hundred KB of payload. */
  enum { PREFAULT_MIN = 1<<18 };

  void init(const Params &p, int maxsym, int maxnib, u64 workload) {
    amaxsym = maxsym;
    amaxnib = maxnib;
    reset_stats();     // binds both arenas, including the match tables
    if( workload>=u64(PREFAULT_MIN) )
      mta.prefault();
    mmS.init();        // ...then the state, which needs those pointers valid
    mmL.init();
    memset(stat, 0, sizeof(stat));
    begin_seg(p, true);
  }

  /* Back to the state the model had before it saw anything.  Everything except
     the match model's storage is refilled -- a couple of megabytes, cheap enough
     to do per wav -- and the match model uses its barrier instead. */
  void reset_stats() {
    /* First time through, run the whole binding pass DRY -- take() hands out
       nothing and only accumulates -- so the arena can be sized from the same
       expressions that lay it out, with no second copy of them to drift.  The
       sizes depend only on amaxsym / amaxnib / sgn_ncls, which are fixed for the
       life of the object, so every later reset re-binds the same layout. */
    if( !cta.base ) {
      cta.dry = mta.dry = 1;
      bind_tables();
      size_t need = cta.used, needm = mta.used;
      cta.dry = mta.dry = 0;
      mta.huge = 0;
      cta.reserve(need);
      mta.reserve(needm);
    }
    bind_tables();
    cta.check();
    mta.check();
  }

  Arena cta;   // counter tables: dense, refilled per segment, huge pages
  Arena mta;   // match history + hash: 512 MB, sparse, demand-zero, NO huge pages

  void bind_tables() {
    cta.rewind();
    mta.rewind();
    mmS.bind(mta);
    mmL.bind(mta);
    nsym = amaxsym;
    nib_rows = amaxnib;
    sgn_ncls = pclamp(G0_SGN_NCLS, 2, sgn_maxnb+1);
    tQ.init(cta, nsym*NCONF, nsym, RS_CODE);
    tQs.init(cta, nsym*NCONF, nsym);
    tI.init(cta, nib_rows, nsym, RS_CODE);
    tP.init(cta, nsym, nsym, RS_CODE);
    tP2.init(cta, nsym*nsym, nsym, RS_CODE);
    tX.init(cta, nsym+1, nsym, RS_CODE);
    tQP.init(cta, nsym*nsym, nsym, RS_CODE);
    tQI.init(cta, nsym*nib_rows, nsym, RS_CODE);
    tQX.init(cta, nsym*(nsym+1), nsym, RS_CODE);
    tIP.init(cta, nib_rows*nsym*NCONF, nsym, RS_CODE);
    tQ2.init(cta, nsym*nsym*NCONF, nsym, RS_CODE);
    tQ3.init(cta, nsym*nsym*nsym, nsym, RS_CODE);
    tMS.init(cta, 1+2*C0_Ml_Volume, nsym, RS_MATCH);
    tML.init(cta, 1+2*C0_Ml_Volume, nsym, RS_MATCH);
    tPX.init(cta, nsym*(nsym+1), nsym, RS_CODE);
    tXX.init(cta, (nsym+1)*(nsym+1), nsym, RS_CODE);
    tPPX.init(cta, nsym*nsym*(nsym+1), nsym, RS_CODE);
    tPX2.init(cta, nsym*(nsym+1)*(nsym+1), nsym, RS_CODE);
    tP3.init(cta, nsym*nsym*nsym, nsym, RS_CODE);
    tPPXX.init(cta, nsym*nsym*(nsym+1)*(nsym+1), nsym, RS_CODE);
    tP4.init(cta, nsym*nsym*nsym*nsym, nsym, RS_CODE);
    tQPX.init(cta, nsym*nsym*(nsym+1), nsym, RS_CODE);
    tHJ.init(cta, 1<<hj_bits, nsym, RS_CODE);
    tQB.init(cta, nsym*NCONF, nsym, RS_CODE);
    tQAB.init(cta, nsym*nsym, nsym, RS_CODE);
    mmS.barrier();
    mmL.barrier();
    mxC.init(cta, nsym*NCONF*(G0_MATCH_MIXSEL ? 2 : 1));
    apA.init(cta, nsym*nsym);
    apB.init(cta, nib_rows*nsym);
    tHnb.init(cta, SGN_NMODEL, SGN_NODES, RS_HDR);
    tHmn.init(cta, SGN_NMODEL*sgn_ncls, SGN_NODES, RS_HDR);
    tRsv.init(cta, 1, 2, RS_LIT); // one node, two branches
    tLit.init(cta, LIT_NSTREAM*LIT_W, LIT_W, RS_LIT);
    /* The predictors' weights and histories share the arena (v4 §2.1): 64B
       alignment for the kernels, and they land next to the tables instead of
       inside this object.  Bound in the same pass, so the dry run sizes them
       too; reset() -- which is what actually touches the memory -- runs later,
       from begin_seg. */
    for( int k = 0; k<MAX_CHANS; k++ ) {
      ch[k].pd.alloc(cta);
      ch[k].pdB.alloc(cta);
    }
  }

  /* Enter a new wav.  Everything the counters, mixer and APM have learned
     stays; everything that describes where the previous signal had got to
     goes.  G0_SEG_KEEPW keeps the NLMS weights across the boundary as a warm
     start -- the histories are cleared either way, since predicting the first
     samples of one file from the tail of another is worse than predicting
     from silence. */
  void begin_seg(const Params &p, bool fresh) {
    mmS.newseg();
    mmL.newseg();
    pm = p;
    nbit = p.bps;
    int segsym = 1<<p.bps;
    int segnib = p.format==FMT_MS ? M0_Md_Volume : I0_Ib_Volume;
    nsym = segsym;
    nib_rows = segnib;
    for( int k = 0; k<MAX_CHANS; k++ ) {
      ch[k].pd.reset(!fresh&&G0_SEG_KEEPW);
      ch[k].pdB.reset(!fresh&&G0_SEG_KEEPW);
      ch[k].pcm = 0;
      ch[k].index = ch[k].prev = ch[k].prev2 = ch[k].prev3 = ch[k].prev4 = ch[k].cur = 0;
      ch[k].samp1 = ch[k].samp2 = 0;
      ch[k].delta = ch[k].hdelta = ch[k].rdelta = 16;
      ch[k].set_delta(16);
      ch[k].errR = ch[k].errH = 0;
      ch[k].bprev = 0;
      ch[k].c1 = pm.ncoef ? pm.coef[0][0] : 256;
      ch[k].c2 = pm.ncoef ? pm.coef[0][1] : 0;
    }
  }

  int amaxsym = 0, amaxnib = 0; // set by init()

  #include "xad_codec_bits.inc"
  #include "xad_codec_ima.inc"
  #include "xad_codec_ms.inc"
};
