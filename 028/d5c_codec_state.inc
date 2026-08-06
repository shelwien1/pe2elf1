/* d5c_codec_state.inc -- included INSIDE struct Codec; not standalone.
 *
 * Members: geometry planes, block grids, model-table views, the
 * epx staging, and init() -- which binds every generated Table() and refills it. */

  /* Same-name constant as the old threaded `bool enc` parameter, so every
   * branch below folds.  The parameter itself had to GO: a parameter stays
   * runtime even when every call site passes a literal. */
  enum { enc = (MD == 0) };
  Codec(RC<MD>& r) : rc(r) { G0_Init(); M0_Init(); A0_Init(); B0_Init(); C0_Init(); E0_Init(); X0_Init(); P0_Init(); }
  ~Codec() {
    P0_Quit(); X0_Quit(); E0_Quit(); C0_Quit(); B0_Quit(); A0_Quit(); M0_Quit(); G0_Quit();
#if D5C_DYN_GEOM
    delete[] sPlaneMem; delete[] sGridMem; delete[] sRowMem;
#endif
    delete[] rmh; delete[] rmt; delete[] rmt2;   // match model, not geometry
    delete[] rwa; delete[] rwq; delete[] apRWt; delete[] apRW2t;
  }
  Codec(const Codec&) = delete;
  Codec& operator=(const Codec&) = delete;

  RC<MD>& rc;    // the frontend's RC base subobject, bound at construction
  dxt_fmt fmt = FMT_DXT5;

  // per-level geometry & causal state
  u32 bw = 0, bh = 0, iw = 0, ih = 0;
  /* Truncated entry: stop after this many blocks of the level (0 = code it
   * all).  Whole BLOCKS rather than whole rows, so the last partial row is
   * still modelled and only the partial block itself falls to the tail.  The
   * level keeps its true bw/bh, so every context still sees the real geometry
   * -- this only ends the scan early. */
  u32 blk_lim = 0;

  /* Match model for the RASTER path.  The block paths already carry four match
   * candidates (W, N, and two content hashes); code_raster had none, so it
   * could only ever see W/N/NW/planar/MED -- purely local.  A 2048x2048 RGB24
   * terrain texture measured 19.3% of its bytes covered by matches of >=24 B at
   * a MEDIAN DISTANCE OF 3.9 MB, which no local context can reach: xz with a
   * 16 MiB window beat this codec by 12% on it, and xz with a 64 KiB window
   * lost by 13%.  The whole difference was long-range matching.
   *
   * lpaq-style: hash the last few coded bytes to a position, predict the byte
   * that followed it there, and feed the expected bit plus a confidence bucket
   * as contexts. */
  enum { RM_BITS = 22, RM_ORDER = 6 };
  u32* rmh = nullptr;                 // hash -> position within the level
  const u8* rm_buf = nullptr;         // level base; only bytes before rm_cur are coded
  size_t rm_match = 0, rm_cur = 0;    // predicted-from position, current position
  bool rm_have = false;               // rm_match is usable
  u32 rm_len = 0;                     // consecutive correct predictions
  u64 rm_hist = 0;                    // last 8 coded bytes, newest in the low byte
  Ctr* rmt = nullptr;                 // (bit state x len bucket) -> counter rows
  Ctr* rmt2 = nullptr;                // (predicted byte x on-track) -> counter rows
  Ctr* rwa = nullptr;                 // activity bucket x tree node
  Ctr* rwq = nullptr;                 // (NE, NN) quantized x tree node
  u16* apRWt = nullptr;               // SSE tables, allocated with the rest
  u16* apRW2t = nullptr;

  /* Storage for everything that used to be sized per level.  The slabs are
   * fixed at MAX_DIM and the members below are POINTERS into them, because
   * every rotation in this codec is a swap: level_init rotates the finished
   * level into the parent slot, achan_swap exchanges the whole causal set for
   * BC5's second channel, and code_level_chan swaps the block rows each row.
   * Those all stay O(1) pointer exchanges -- copying the buffers instead would
   * be a different program.  Only the live bw*bh / iw*ih region of a slab is
   * ever cleared or read, so the untouched remainder stays demand-zero. */
  static const size_t PLANE_N = size_t(MAX_IW) * MAX_IW;
  static const size_t GRID_N  = size_t(MAX_B) * MAX_B;
  enum { N_PLANE = 18, N_GRID = 4, N_ROW = 4 };
#if D5C_DYN_GEOM
  /* Sized to the largest texture seen, not to MAX_DIM.  reserve() grows the
   * slabs at a TEXTURE boundary -- the only place it is safe, because
   * texture_init declares the cross-level state dead there, so re-pointing the
   * rotating members costs nothing.  It never shrinks: a solid archive mixes
   * sizes and re-allocating per texture would thrash for no gain.
   *
   * MAX_DIM stops being an allocation size and becomes purely the sanity bound
   * on a hostile header, which is what it was always meant to be. */
  size_t plane_n = 0, grid_n = 0, row_n = 0;
  u8*  sPlaneMem = nullptr;
  Blk* sGridMem  = nullptr;
  Blk* sRowMem   = nullptr;
  u8*  P(int i) { return sPlaneMem + size_t(i) * plane_n; }
  Blk* G(int i) { return sGridMem  + size_t(i) * grid_n; }
  Blk* R(int i) { return sRowMem   + size_t(i) * row_n;  }
  /* Grow the slabs so one level of the next texture fits.  Sizes are AREAS,
     not the side of a square: level_init(bw,bh) clears iw*ih = 16*bw*bh plane
     bytes, bw*bh grid entries and bw row entries, and a 64x512 texture
     (bw=16, bh=128) needs 32768 plane bytes where the old square rule --
     (4*bw)^2 = 4096 -- reserved an eighth of that.

     Each family only ever GROWS.  Shrinking one while another grows is not a
     saving, it is a buffer overflow: the caller has already sized the coding
     state from the texture the caller is about to walk, so a slab that came
     back smaller is written past its end on the first level_init.  The old
     code did exactly that -- when only the grid needed growing it set the
     plane side to 0 as a "keep the larger" shorthand, which made plane_n 0 and
     turned the next memset into an unbounded heap write.  Keeping the maximum
     per family, independently, is what that line meant to say. */
  void reserve(size_t need_plane, size_t need_grid, size_t need_row) {
    if(need_plane <= plane_n && need_grid <= grid_n && need_row <= row_n) return;
    // round up so a corpus of gently growing textures does not realloc each time
    size_t np = plane_n ? plane_n : size_t(64) * 64; while(np < need_plane) np *= 2;
    size_t ng = grid_n  ? grid_n  : size_t(16) * 16; while(ng < need_grid ) ng *= 2;
    size_t nr = row_n   ? row_n   : 16;              while(nr < need_row  ) nr *= 2;
    delete[] sPlaneMem; delete[] sGridMem; delete[] sRowMem;
    sPlaneMem = nullptr; sGridMem = nullptr; sRowMem = nullptr;  // anew() may exit(3)
    plane_n = np; grid_n = ng; row_n = nr;
    sPlaneMem = anew<u8 >(plane_n * N_PLANE, "geometry planes");
    sGridMem  = anew<Blk>(grid_n  * N_GRID , "block grids");
    sRowMem   = anew<Blk>(row_n   * N_ROW  , "block rows");
    rebind_geom();
  }
#else
  u8  sPlane[N_PLANE][PLANE_N];  // 7 live + 7 parent + 4 for BC5's spare channel
  Blk sGrid [N_GRID ][GRID_N];   // cBlk, qBlk + the BC5 spares
  Blk sRow  [N_ROW  ][MAX_B];    // rowP, rowC + the BC5 spares
  u8*  P(int i) { return sPlane[i]; }
  Blk* G(int i) { return sGrid[i];  }
  Blk* R(int i) { return sRow[i];   }
  void reserve(size_t, size_t, size_t) {}
#endif

  Blk *rowP = nullptr, *rowC = nullptr;
  u8  *pR = nullptr, *pG = nullptr, *pB = nullptr, *pA = nullptr, *pX = nullptr;
  u8  *pCI = nullptr, *pAI = nullptr;       // raw per-texel index planes
  // A1: previous (finer) level kept alive across level_init -- the parent of
  // every texel/block of the current level. qw/qh = parent plane dims; qv = has parent.
  u8  *qR = nullptr, *qG = nullptr, *qB = nullptr, *qA = nullptr;
  u8  *qX = nullptr, *qCI = nullptr, *qAI = nullptr;
  Blk *qBlk = nullptr, *cBlk = nullptr;   // parent block grid, current grid (becomes parent)
  u32 qw = 0, qh = 0, qbw = 0, qbh = 0; bool hasP = false;
  int prevAi = 0, prevCi = 0;
  int prevPlA = 0, prevPlC = 0; // p^L of the previous literal-coded texel (A5 gradient)
  u32 hAM = 0, hCM = 0, hAW = 0, hCW = 0; // match histories
  u32 mtAM = 0, mtCM = 0, mtAW = 0, mtCW = 0; // A6: last-match-type registers

  /* The names below are all VIEWS onto the inherited Table() storage, bound in
   * init().  They exist so the coding routines read the same as before the
   * conversion -- tAM1[c1], tE[c][j].row(...), HA[h] -- rather than carrying
   * the module prefix through every context expression. */

  // ---- alpha endpoints
  Ctr *tAM1, *tAM2, *tAMH;   // W-match, (2nd ctx), hash-match
  Ctr *tAN1, *tAN2;
  CtrTab tA0W, tA0N, tA0P, tA0o, tA0M, tA0H, tA0P2, tA1W, tA1A, tA1P, tA1o, tA1M, tA1H, tA1P2;
  CtrTab tA0E, tA1E, tA0L, tA1L;   // D6: decoded-pixel edge rows
  Mixer mxAMW, mxAMN, mxAMH, mxA0, mxA1;
  u32 *HA; // (a0,a1) pair hash: bit16 = valid

  // ---- color endpoints
  Ctr *tCM1, *tCM2, *tCMH, *tCMP;
  Ctr *tCN1, *tCN2;
  CtrTab tE[6][10]; // per channel r0,g0,b0,r1,g1,b1: context tables
  Mixer mxCMW, mxCMN, mxCMH, mxCMP, mxE[6];
  u64 *HC; // (c0,c1) pair hash: bit32 = valid

  // ---- alpha index words + texels
  Ctr *tAW1, *tAW2, *tAWH, *tAWH2;
  Ctr *tAN1w, *tAN2w;
  CtrTab tAI1, tAI2, tAI3, tAI4, tAI0, tAI5, tAI6, tAI9, tAI8, tAIb, tAIe, tAIf, tAIp, tAIq;
  Mixer mxAWW, mxAWN, mxAWH, mxAWH2, mxAI;
  APM apAI, apAI2;
  APM apRW, apRW2;                    // SSE for the raster path (it had none)
  u64 *HAW; u8 *HAWv;   // ai word keyed by (a0,a1)
  u64 *HAW2; u8 *HAW2v; // ai word keyed by (W.aw,N.aw)

  // ---- color index words + texels
  Ctr *tCW1, *tCW2, *tCWH, *tCWH2;
  Ctr *tCN1w, *tCN2w;
  CtrTab tCI1, tCI2, tCI3, tCI4, tCI0, tCI5, tCI6, tCI7, tCI9, tCI8, tCIb, tCIc, tCId, tCIe, tCIf, tCIg, tCIp, tCIq, tCIr;
  CtrS *sCI1, *sAI1;
  Mixer mxCWW, mxCWN, mxCWH, mxCWH2, mxCI;
  APM apCI, apCI2;
  u64 *HCW; u8 *HCWv;
  u64 *HCW2; u8 *HCW2v;

  // ---- dxt3 alpha nibbles
  CtrTab tX[6];
  Mixer mxX;

  // ---- tail
  CtrTab tTail;

  // clamped hash widths (bits)
  int hbA = PHBITS(M0_HB_A), hbC = PHBITS(M0_HB_C);
  int hbAW = PHBITS(M0_HB_AW), hbAW2 = PHBITS(M0_HB_AW2);
  int hbCW = PHBITS(M0_HB_CW), hbCW2 = PHBITS(M0_HB_CW2);

  // ---- stats (bits*256 per category)
  u64 stat[8] = {}; // 0 aep 1 cep 2 aiw 3 ai 4 ciw 5 ci 6 x4 7 tail
  int scat = 0;

  /* External endpoints (ddsdet --bmp).  When epx is set the endpoint fields
   * are NOT range-coded: the encoder appends them here and the decoder reads
   * them back, so they can live in an image file for an external codec while
   * the index planes stay in the model.  Everything downstream is unaffected
   * -- the palette, p^ and every endpoint-derived context still see the real
   * values, so index coding is exactly as strong as before.
   * 6 bytes per block: a0, a1, c0 lo, c0 hi, c1 lo, c1 hi (a0/a1 = 0 unless DXT5). */
  ByteBuf* epx = nullptr;
  size_t epx_pos = 0;
  bool epx_fail = false;
  inline void epx_put(int a0, int a1, int c0, int c1) {
    epx->push(u8(a0)); epx->push(u8(a1));
    epx->push(u8(c0)); epx->push(u8(c0 >> 8));
    epx->push(u8(c1)); epx->push(u8(c1 >> 8));
  }
  inline bool epx_get(int& a0, int& a1, int& c0, int& c1) {
    if(epx_pos + 6 > epx->size()) return false;
    const u8* p = epx->data() + epx_pos; epx_pos += 6;
    a0 = p[0]; a1 = p[1];
    c0 = p[2] | p[3] << 8; c1 = p[4] | p[5] << 8;
    return true;
  }
  int epa0 = 0, epa1 = 0, epc0 = 0, epc1 = 0; // staged values for this block

  /* Bind every view to its Table() slab and reset it.  This is called once per
   * .dds file in --ns mode, so it must not allocate -- the storage was claimed
   * by the constructor.  The sizes are the same expressions as before the
   * conversion, so the layout, and therefore the coded stream, is unchanged. */
  static void bindC(Ctr*& p, Ctr* store, size_t n) { p = store; fill_ctr(store, n); }
  template<class T> static void bindZ(T*& p, T* store, size_t n) { p = store; memset(store, 0, n * sizeof(T)); }

  void init(dxt_fmt f) {
    fmt = f;
    const size_t nM1 = size_t(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    const size_t nM2 = size_t(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    bindC(tAM1, M0_tAM1, nM1); bindC(tAM2, M0_tAM2, nM2); bindC(tAMH, M0_tAMH, nM1);
    bindC(tAN1, M0_tAN1, nM1); bindC(tAN2, M0_tAN2, nM2);
    bindC(tCM1, M0_tCM1, nM1); bindC(tCM2, M0_tCM2, nM2); bindC(tCMH, M0_tCMH, nM1);
    bindC(tCMP, M0_tCMP, nM1);
    bindC(tCN1, M0_tCN1, nM1); bindC(tCN2, M0_tCN2, nM2);
    bindC(tAW1, M0_tAW1, nM1); bindC(tAW2, M0_tAW2, nM2); bindC(tAWH, M0_tAWH, nM1); bindC(tAWH2, M0_tAWH2, nM1);
    bindC(tAN1w, M0_tAN1w, nM1); bindC(tAN2w, M0_tAN2w, nM2);
    bindC(tCW1, M0_tCW1, nM1); bindC(tCW2, M0_tCW2, nM2); bindC(tCWH, M0_tCWH, nM1); bindC(tCWH2, M0_tCWH2, nM1);
    bindC(tCN1w, M0_tCN1w, nM1); bindC(tCN2w, M0_tCN2w, nM2);
    tA0W.bind(A0_tA0W, A0_AE0W_Volume, 256); tA0N.bind(A0_tA0N, A0_AE0N_Volume, 256);
    tA0P.bind(A0_tA0P, A0_AE0P_Volume, 256); tA0o.bind(A0_tA0o, 1, 256);
    tA1W.bind(A0_tA1W, A0_AE1W_Volume, 256); tA1A.bind(A0_tA1A, A0_AE1A_Volume, 256);
    tA1P.bind(A0_tA1P, A0_AE1P_Volume, 256); tA1o.bind(A0_tA1o, 1, 256);
    tA0M.bind(A0_tA0M, A0_AE0M_Volume, 256); tA1M.bind(A0_tA1M, A0_AE1M_Volume, 256);
    tA0E.bind(A0_tA0E, A0_AE0E_Volume, 256); tA1E.bind(A0_tA1E, A0_AE1E_Volume, 256);
    tA0L.bind(A0_tA0L, A0_AE0L_Volume, 256); tA1L.bind(A0_tA1L, A0_AE1L_Volume, 256);
    tA0H.bind(A0_tA0H, A0_AE0H_Volume, 256); tA1H.bind(A0_tA1H, A0_AE1H_Volume, 256);
    tA0P2.bind(A0_tA0P2, A0_AE0P2_Volume, 256); tA1P2.bind(A0_tA1P2, A0_AE1P2_Volume, 256);
    mxAMW.bind(M0_mxAMWw, 4, M0_MM_Volume, G0_MW_PAIR);
    mxAMN.bind(M0_mxAMNw, 4, M0_MM_Volume, G0_MW_PAIR);
    mxAMH.bind(M0_mxAMHw, 4, M0_MM_Volume, G0_MW_PAIR);
    if(!rmh) {
      rmh  = anew<u32>(size_t(1) << RM_BITS, "raster match hash");
      rmt  = anew<Ctr>(3 * 8 * 256, "raster match ctx");    // 3 bit-states x 8 len buckets, 256-node tree
      rmt2 = anew<Ctr>(2 * 256 * 256, "raster match byte ctx");  // on-track x predicted byte
      rwa  = anew<Ctr>(16 * 256, "raster activity ctx");       // activity bucket
      rwq  = anew<Ctr>(64 * 256, "raster NE/NN ctx");       // (NE,NN) joint
      /* The raster path had NO SSE stage while every block path has two.  These
       * are sized like the block APMs: one interpolation table per context. */
      /* An APM table is nctx * (PAPN(n) + 1) u16 -- PAPN clamps the interval
       * count to [2, 2048], so the cell count is whatever opt.pl tuned, NOT a
       * constant.  These were sized with a hardcoded 33, which is right only
       * while N == 32.  G0_AP_N_AI2 is tuned to 33, so apRW2t was one u16 per
       * context short, and APM::bind wrote past the end of all 4096 of them.
       * The overflow was silent in Debug and intermittently fatal in Const,
       * depending on what the heap put next.  Derive it from the same
       * expression bind() uses, exactly as the block APM tables do. */
      apRWt  = anew<u16>(size_t(8 * 8 * 256) * (PAPN(G0_AP_N_AI ) + 1), "raster SSE 1");
      apRW2t = anew<u16>(size_t(16 * 256)    * (PAPN(G0_AP_N_AI2) + 1), "raster SSE 2");
    }
    apRW.bind(apRWt, 8 * 8 * 256, G0_AP_N_AI, G0_AP_R_AI);
    apRW2.bind(apRW2t, 16 * 256, G0_AP_N_AI2, G0_AP_R_AI2);
    mxA0.bind(A0_mxA0w, 16, 256, G0_MW_AEP); mxA1.bind(A0_mxA1w, 10, 256, G0_MW_AEP);
    bindZ(HA, M0_HA, size_t(1) << hbA);

    static const int chw[6] = {32, 64, 32, 32, 64, 32};
    Ctr* const eS[6][10] = {
      { E0_tE00, E0_tE01, E0_tE02, E0_tE03, E0_tE04, E0_tE05, E0_tE06, E0_tE07, E0_tE08, E0_tE09 },
      { E0_tE10, E0_tE11, E0_tE12, E0_tE13, E0_tE14, E0_tE15, E0_tE16, E0_tE17, E0_tE18, E0_tE19 },
      { E0_tE20, E0_tE21, E0_tE22, E0_tE23, E0_tE24, E0_tE25, E0_tE26, E0_tE27, E0_tE28, E0_tE29 },
      { E0_tE30, E0_tE31, E0_tE32, E0_tE33, E0_tE34, E0_tE35, E0_tE36, E0_tE37, E0_tE38, E0_tE39 },
      { E0_tE40, E0_tE41, E0_tE42, E0_tE43, E0_tE44, E0_tE45, E0_tE46, E0_tE47, E0_tE48, E0_tE49 },
      { E0_tE50, E0_tE51, E0_tE52, E0_tE53, E0_tE54, E0_tE55, E0_tE56, E0_tE57, E0_tE58, E0_tE59 },
    };
    int* const eW[6] = { E0_mxE0w, E0_mxE1w, E0_mxE2w, E0_mxE3w, E0_mxE4w, E0_mxE5w };
    for(int c = 0; c < 6; c++) {
      int wd = chw[c];
      // rows: [0]=W(or e0 for e1), [1]=N(or W), [2]=pred2d, [3]=cross-channel/order0
      tE[c][0].bind(eS[c][0], c < 3 ? E0_E0W_Volume : E0_E1A_Volume, wd);
      tE[c][1].bind(eS[c][1], c < 3 ? E0_E0N_Volume : E0_E1W_Volume, wd);
      tE[c][2].bind(eS[c][2], c < 3 ? E0_E0P_Volume : E0_E1P_Volume, wd);
      tE[c][3].bind(eS[c][3], c % 3 == 0 ? 1 : c % 3 == 1 ? E0_EX1_Volume : E0_EX2_Volume, wd);
      tE[c][4].bind(eS[c][4], c < 3 ? E0_E0Q_Volume : E0_E1Q_Volume, wd);
      tE[c][5].bind(eS[c][5], c < 3 ? E0_E0J_Volume : E0_E1J_Volume, wd);
      tE[c][6].bind(eS[c][6], c < 3 ? E0_E0M_Volume : E0_E1M_Volume, wd);
      tE[c][7].bind(eS[c][7], c < 3 ? E0_E0H_Volume : E0_E1H_Volume, wd);
      tE[c][8].bind(eS[c][8], c < 3 ? E0_E0PP_Volume : E0_E1PP_Volume, wd);
      tE[c][9].bind(eS[c][9], c < 3 ? E0_E0E_Volume : E0_E1E_Volume, wd);
      mxE[c].bind(eW[c], 11, wd, G0_MW_E);   // D6 row added
    }
    mxCMW.bind(M0_mxCMWw, 4, M0_MM_Volume, G0_MW_PAIR);
    mxCMN.bind(M0_mxCMNw, 4, M0_MM_Volume, G0_MW_PAIR);
    mxCMH.bind(M0_mxCMHw, 4, M0_MM_Volume, G0_MW_PAIR);
    mxCMP.bind(M0_mxCMPw, 4, M0_MM_Volume, G0_MW_PAIR);
    bindZ(HC, M0_HC, size_t(1) << hbC);

    tAI1.bind(B0_tAI1, B0_AI1_Volume, 8); tAI2.bind(B0_tAI2, B0_AI2_Volume, 8);
    tAI3.bind(B0_tAI3, B0_AI3_Volume, 8); tAI4.bind(B0_tAI4, B0_AI4_Volume, 8);
    tAI0.bind(B0_tAI0, B0_AI0_Volume, 8); tAI5.bind(B0_tAI5, B0_AI5_Volume, 8);
    tAI6.bind(B0_tAI6, B0_AI6_Volume, 8);
    sAI1 = B0_sAI1; fill_ctrs(sAI1, size_t(B0_SLA_Volume) * 8);
    tAI9.bind(B0_tAI9, B0_AI9_Volume, 8); tAI8.bind(B0_tAI8, B0_AI8_Volume, 8);
    tAIb.bind(B0_tAIb, B0_AIb_Volume, 8);
    tAIe.bind(B0_tAIe, B0_AIe_Volume, 8); tAIf.bind(B0_tAIf, B0_AIf_Volume, 8);
    tAIp.bind(B0_tAIp, B0_AIp_Volume, 8); tAIq.bind(B0_tAIq, B0_AIq_Volume, 8);
    mxAWW.bind(M0_mxAWWw, 4, M0_MM_Volume, G0_MW_WORD);
    mxAWN.bind(M0_mxAWNw, 4, M0_MM_Volume, G0_MW_WORD);
    mxAWH.bind(M0_mxAWHw, 4, M0_MM_Volume, G0_MW_WORD);
    mxAWH2.bind(M0_mxAWH2w, 4, M0_MM_Volume, G0_MW_WORD);
    mxAI.bind(B0_mxAIw, 17, B0_AIM_Volume * 8, G0_MW_AI);
    apAI.bind(B0_apAIt, B0_APA_Volume * 8, G0_AP_N_AI, G0_AP_R_AI);
    apAI2.bind(B0_apAI2t, B0_APA2_Volume * 8, G0_AP_N_AI2, G0_AP_R_AI2);
    bindZ(HAW, M0_HAW, size_t(1) << hbAW); bindZ(HAWv, M0_HAWv, size_t(1) << hbAW);
    bindZ(HAW2, M0_HAW2, size_t(1) << hbAW2); bindZ(HAW2v, M0_HAW2v, size_t(1) << hbAW2);

    tCI1.bind(C0_tCI1, C0_CI1_Volume, 4); tCI2.bind(C0_tCI2, C0_CI2_Volume, 4);
    tCI5.bind(C0_tCI5, C0_CI5_Volume, 4);
    tCI6.bind(C0_tCI6, C0_CI6_Volume, 4); tCI7.bind(C0_tCI7, C0_CI7_Volume, 4);
    sCI1 = C0_sCI1; fill_ctrs(sCI1, size_t(C0_SLC_Volume) * 4);
    tCI9.bind(C0_tCI9, C0_CI9_Volume, 4); tCI8.bind(C0_tCI8, C0_CI8_Volume, 4);
    tCIb.bind(C0_tCIb, C0_CIb_Volume, 4);
    tCIc.bind(C0_tCIc, C0_CIc_Volume, 4); tCId.bind(C0_tCId, C0_CId_Volume, 4);
    tCIe.bind(C0_tCIe, C0_CIe_Volume, 4); tCIf.bind(C0_tCIf, C0_CIf_Volume, 4);
    tCIg.bind(C0_tCIg, C0_CIg_Volume, 4); tCIp.bind(C0_tCIp, C0_CIp_Volume, 4);
    tCIq.bind(C0_tCIq, C0_CIq_Volume, 4); tCIr.bind(C0_tCIr, C0_CIr_Volume, 4);
    tCI3.bind(C0_tCI3, C0_CI3_Volume, 4); tCI4.bind(C0_tCI4, C0_CI4_Volume, 4);
    tCI0.bind(C0_tCI0, C0_CI0_Volume, 4);
    mxCWW.bind(M0_mxCWWw, 4, M0_MM_Volume, G0_MW_WORD);
    mxCWN.bind(M0_mxCWNw, 4, M0_MM_Volume, G0_MW_WORD);
    mxCWH.bind(M0_mxCWHw, 4, M0_MM_Volume, G0_MW_WORD);
    mxCWH2.bind(M0_mxCWH2w, 4, M0_MM_Volume, G0_MW_WORD);
    mxCI.bind(C0_mxCIw, 22, C0_CIM_Volume * 4, G0_MW_CI);
    apCI.bind(C0_apCIt, C0_APC_Volume * 4, G0_AP_N_CI, G0_AP_R_CI);
    apCI2.bind(C0_apCI2t, C0_APC2_Volume * 4, G0_AP_N_CI2, G0_AP_R_CI2);
    bindZ(HCW, M0_HCW, size_t(1) << hbCW); bindZ(HCWv, M0_HCWv, size_t(1) << hbCW);
    bindZ(HCW2, M0_HCW2, size_t(1) << hbCW2); bindZ(HCW2v, M0_HCW2v, size_t(1) << hbCW2);

    tX[0].bind(X0_tX0, X0_X1_Volume, 16); tX[1].bind(X0_tX1, X0_X2_Volume, 16);
    tX[2].bind(X0_tX2, X0_X3_Volume, 16); tX[3].bind(X0_tX3, 1, 16);
    tX[4].bind(X0_tX4, X0_X4_Volume, 16); tX[5].bind(X0_tX5, X0_X5_Volume, 16);
    mxX.bind(X0_mxXw, 7, 16, G0_MW_X); // 4 base rows + X4 + X5 + bias

    tTail.bind(X0_tTail, X0_TL_Volume, 256);
    bindC(tP[0], P0_tP0, size_t(P0_P1_Volume) * 256); bindC(tP[1], P0_tP1, size_t(P0_P2_Volume) * 256);
    bindC(tP[2], P0_tP2, size_t(P0_P3_Volume) * 256); bindC(tP[3], P0_tP3, size_t(P0_P4_Volume) * 256);
    bindC(tP[4], P0_tP4, size_t(P0_P5_Volume) * 256); bindC(tP[5], P0_tP5, size_t(P0_P6_Volume) * 256);
    mxP.bind(P0_mxPw, 6 + 1, 256, G0_MW_P);  // tree() keys the mixer on the node
  }

  /* BC5 is two independent BC4 channels in one block.  The adaptive tables are
   * shared between them on purpose -- the two channels have the same kind of
   * statistics and a shared model warms up twice as fast -- but everything
   * causal (neighbour rows, decoded planes, match histories, the parent level)
   * has to be per channel, or channel Y's contexts would be reading channel
   * X's pixels.  achan_swap() exchanges the live set with the spare. */
  Blk *rowP2 = nullptr, *rowC2 = nullptr, *cBlk2 = nullptr, *qBlk2 = nullptr;
  u8  *pA2 = nullptr, *pAI2 = nullptr, *qA2 = nullptr, *qAI2 = nullptr;

  /* Canonical assignment of the rotating views onto the slabs.  level_init and
   * achan_swap permute these, so this is only correct to call where the
   * cross-level state is dead -- i.e. alongside texture_init. */
  void rebind_geom() {
    rowP = R(0); rowC = R(1); rowP2 = R(2); rowC2 = R(3);
    pR = P(0); pG = P(1); pB = P(2); pA = P(3); pX = P(4); pCI = P(5); pAI = P(6);
    qR = P(7); qG = P(8); qB = P(9); qA = P(10); qX = P(11); qCI = P(12); qAI = P(13);
    pA2 = P(14); pAI2 = P(15); qA2 = P(16); qAI2 = P(17);
    qBlk = G(0); cBlk = G(1); cBlk2 = G(2); qBlk2 = G(3);
  }
  int prevAi2 = 0, prevPlA2 = 0;
  u32 hAM2 = 0, hAW2 = 0, mtAM2 = 0, mtAW2 = 0;
  bool hasP2 = false;
