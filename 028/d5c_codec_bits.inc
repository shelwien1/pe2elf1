/* d5c_codec_bits.inc -- included INSIDE struct Codec; not standalone.
 *
 * The coding primitives: mbit (one mixed bit) and tree (an MSB-first
 * binary tree of them).  Both run per coded bit; mbit is force-inlined. */

  // one mixed bit: counters cs[0..ncs), mixer ctx, optional APM
  /* NCS is the counter count, and it is the trip count of FOUR loops that all
   * run per coded bit: the mixer add loop below, Mixer::mix's
   * multiply-accumulate, Mixer::upd's weight update, and the counter update
   * loop.  mbit is ~42% of all retired instructions, so making it a constant
   * matters -- but not for the reason it looks like.  Removing the compare is
   * worth almost nothing (the branch was perfectly predicted and off the
   * critical path); what a constant trip count buys is that
   * Mixer::mix/upd BECOME VECTORIZABLE.  With a runtime nx they are not:
   * -fopt-info-vec reports no vectorization there before this change and SSE
   * afterwards.  The lanes only pay off with -march= something that has them,
   * hence the ARCH hook in mk.sh. */
  template<int NCS>
  D5C_FORCE_INLINE int mbit(Mixer& mx, int mctx, CtrPP cs, APM* ap, int actx, int b,
                  APM* ap2 = nullptr, int actx2 = 0, int abr = 3, int abr2 = 3) {
    constexpr int ncs = NCS;
    const int ab = PBLEND(abr), ab2 = PBLEND(abr2);
    mx.nx = 0; mx.add(G0_BIAS);
    for(int i = 0; i < ncs; i++) mx.add(STT[cs[i]->pr()]);
    CtrS* sl = slowin; slowin = nullptr;
    if(sl) mx.add(STT[sl->pr()]);
    int p = mx.mix(mctx);
    int pf = p;
    if(ap) { int pa = ap->pp(p, actx); pf = (ab * pa + (4 - ab) * p) >> 2; }
    if(ap2) {
      if(pf < P_MIN) pf = P_MIN; else if(pf > P_MAX) pf = P_MAX;
      int pa2 = ap2->pp(pf, actx2); pf = (ab2 * pa2 + (4 - ab2) * pf) >> 2;
    }
    if(pf < P_MIN) pf = P_MIN; else if(pf > P_MAX) pf = P_MAX;
    b = rc.bit(pf, b);
    stat[scat] += CLT[pf][b];
    for(int i = 0; i < ncs; i++) cs[i]->upd(b);
    if(sl) sl->upd(b);
    mx.upd(b);
    if(ap) ap->upd(b);
    if(ap2) ap2->upd(b);
    return b;
  }

  /* MSB-first tree over B bits, one mixed bit per node.  The row pointers are
   * whatever the call site has: it passes them contiguously from r0 and leaves
   * the rest defaulted, so the COUNT is the whole shape.  That count was
   * recomputed with nine null tests per coded bit; templating on it turns the
   * array build into straight-line stores and hands mbit a constant.
   * The dispatcher below keeps the original signature so no call site moved.
   *
   * CONTIGUITY REQUIREMENT: the rows must be non-null from r0 with no gaps.
   * tree_n<NR> indexes cs[0..NR-1] unconditionally, so a nullptr row followed
   * by a non-null one dereferences the nullptr.  Any conditional row (the
   * `hasP ? ... : nullptr` parent rows) must therefore be LAST.  Adding a row
   * after one costs a segfault, which is how this comment came to exist. */
  inline int tree(Mixer& mx, CtrP r0, CtrP r1, CtrP r2, CtrP r3, int B, int v, CtrP r4 = nullptr, CtrP r5 = nullptr, CtrP r6 = nullptr, CtrP r7 = nullptr, CtrP r8 = nullptr, CtrP r9 = nullptr) {
    const int nr = (r0?1:0)+(r1?1:0)+(r2?1:0)+(r3?1:0)+(r4?1:0)+(r5?1:0)+(r6?1:0)+(r7?1:0)+(r8?1:0)+(r9?1:0);
    switch(nr) {
      case 10: return tree_n<10>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 9: return tree_n<9>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 8: return tree_n<8>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 7: return tree_n<7>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 6: return tree_n<6>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 5: return tree_n<5>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 4: return tree_n<4>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 3: return tree_n<3>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      case 2: return tree_n<2>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
      default: return tree_n<1>(mx,r0,r1,r2,r3,B,v,r4,r5,r6,r7,r8,r9);
    }
  }

  /* tree() plus a match prediction.  `pb` is the byte the match model expects
   * (-1 = none) and `lb` a confidence bucket from the verified match length.
   *
   * The prediction is only meaningful while the bits decided so far still agree
   * with pb -- once they diverge, pb says nothing about the rest of the byte, so
   * `ok` latches false and the context collapses to "no prediction".  That
   * distinction is the whole value: a bare "expected bit" with no on-track flag
   * would feed noise for the remaining nodes of every byte the match got wrong. */
  int tree_rm(Mixer& mx, CtrP r0, CtrP r1, CtrP r2, CtrP r3, int B, int v,
              CtrP r4, CtrP r5, int pb, u32 lb, u32 act, u32 neq) {
    int nd = 1; bool ok = (pb >= 0);
    for(int k = B - 1; k >= 0; k--) {
      const int eb = ok ? ((pb >> k) & 1) : -1;
      const u32 st = eb < 0 ? 0u : u32(eb) + 1u;        // 0 none, 1 expect 0, 2 expect 1
      Ctr* cs[10];
      cs[0] = r0 + nd; cs[1] = r1 + nd; cs[2] = r2 + nd; cs[3] = r3 + nd;
      cs[4] = r4 + nd; cs[5] = r5 + nd;
      cs[6] = rmt  + (size_t(st) * 8 + lb) * 256 + nd;
      cs[7] = rmt2 + (size_t(ok ? 1 : 0) * 256 + size_t(pb < 0 ? 0 : pb)) * 256 + nd;
      cs[8] = rwa  + size_t(act) * 256 + nd;           // local activity
      cs[9] = rwq  + size_t(neq) * 256 + nd;           // (NE, NN)
      const int tb = v < 0 ? -1 : (v >> k) & 1;
      /* Two SSE stages, as every block path has.  Stage 1 keys on the match
       * state and activity, stage 2 on activity alone -- the two things that say
       * how much to trust the mix here. */
      const int a1 = int((size_t(st) * 8 + lb) * 256) + nd;
      const int a2 = int(size_t(act) * 256) + nd;
      const int b = mbit<10>(mx, nd, cs, &apRW, a1, tb, &apRW2, a2, G0_AB_AI, G0_AB_AI2);
      if(ok && b != eb) ok = false;
      nd = nd + nd + b;
    }
    return nd - (1 << B);
  }

  template<int NR>
  D5C_FORCE_INLINE int tree_n(Mixer& mx, CtrP r0, CtrP r1, CtrP r2, CtrP r3, int B, int v, CtrP r4, CtrP r5, CtrP r6, CtrP r7, CtrP r8, CtrP r9) {
#if D5C_RESTRICT_CHECK
    /* The rows are declared restrict, i.e. promised not to alias.  Every call
     * site draws them from different CtrTab slabs; this asserts it rather than
     * trusting it, because a violation would not crash -- it would silently
     * change the coded stream. */
    { const Ctr* rr[9] = {r0,r1,r2,r3,r4,r5,r6,r7,r8};
      for(int i=0;i<9;i++) for(int j=i+1;j<9;j++)
        if(rr[i] && rr[i]==rr[j]) { fprintf(stderr,"tree(): aliasing rows %d,%d\n",i,j); abort(); } }
#endif
    int nd = 1;
    for(int k = B - 1; k >= 0; k--) {
      Ctr* cs[10]; constexpr int n = NR;
      if(NR > 0) cs[0] = r0 + nd;
      if(NR > 1) cs[1] = r1 + nd;
      if(NR > 2) cs[2] = r2 + nd;
      if(NR > 3) cs[3] = r3 + nd;
      if(NR > 4) cs[4] = r4 + nd;
      if(NR > 5) cs[5] = r5 + nd;
      if(NR > 6) cs[6] = r6 + nd;
      if(NR > 7) cs[7] = r7 + nd;
      if(NR > 8) cs[8] = r8 + nd;
      if(NR > 9) cs[9] = r9 + nd;
      const int tb = v < 0 ? -1 : (v >> k) & 1;
      int b = mbit<NR>(mx, nd, cs, nullptr, 0, tb);
      nd = nd + nd + b;
    }
    return nd - (1 << B);
  }

  static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
