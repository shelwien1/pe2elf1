// model.inc -- what codes and decodes the records of a stream, at a level (levels.inc): each
// decision goes to one of the level's families (fam.inc), whose contexts its IDX/l<L>/*.idx
// builds from the variables the model gives it (famvars.inc). The same functions code and decode
// every record: compressing (ENC) they take the values, decompressing they return them, so the
// two directions can't drift apart.

// the kinds of motion vectors (mvd's and mvg's kind)
enum { MVK_P, MVK_PFIELD, MVK_BFWD, MVK_BBWD, MVK_BDIR, MVK_P4, NMVK };
// the classes of coefficient blocks (sig's, lvl's and sgn's cls): inter, B inter, intra, intra with
// AC prediction, each + 1 for chroma
enum { CL_INTER = 0, CL_B = 2, CL_INTRA = 4, CL_INTRA_AC = 6, NCLS = 8 };

typedef struct Last {  // previous values the models predict from
  int kind;
  int rawprev, rawprev2;  // the bytes kept as bytes before (of any chunk)
  int rawat, rawcode;     // the place in the chunk kept as bytes, its start code (raw family's code)
  U32 rawhd;              // its first bytes
  int type, mtb, tib, tinc, dinc[PT_N], rnd, dcthr, tff, alt, quant[PT_N], fcode[PT_N], bcode;
  S64 time_base, last_time_base, last_non_b;  // display times as FFmpeg computes them
  int pp_time, pb_time;                       // anchor to anchor, anchor to B-VOP
} Last;

// Coefficient magnitudes of the blocks coded last in each column of macroblocks: the left and
// above neighbours of a block (a macroblock's blocks replace those of the one above it as they are
// coded).
// mag: |level| (at most 255), sgn: 1 for negative, suf: coefficients at i..63, last: position of the
// last one (-1 none), dcs: size of the intra DC difference (-1 none), dcsg: 1 for a negative one
typedef struct BlkCtx { U8 mag[BLK_COEFS], sgn[BLK_COEFS], suf[BLK_COEFS + 1]; S8 last, dcs, dcsg; U8 n; } BlkCtx;

// macroblock state the contexts of later macroblocks use
enum { C_NONE, C_SKIP, C_INTER, C_4MV, C_INTRA, C_GMC, C_BDIR0, C_BDIR, C_BFWD, C_BBWD, C_BBI };
// mva: magnitudes of the first motion vector difference in VLC code units, mvs: its signs (0, 1 +, 2 -);
// bsk: B-VOPs skip the macroblock at this position
typedef struct MBCtx { U8 cls, cbp, nz[MB_BLOCKS], mva[2], mvs[2], ac, dt, fp, bsk; } MBCtx;
typedef S16 MV4[MB_LUMA][2];  // a macroblock's motion vectors, per 8x8 block

typedef struct VState {  // the coding side of the current VOP
  Vop v;
  int mb_w, prev_dq, run;    // run: macroblocks of the same kind (skip, direct) before this one
  int dc_prev[2], dc_sgn[2];  // the last intra DC difference's size, and its sign (0 none, 1 +, 2 -)
  int vp_start;               // first macroblock of the video packet
  int lmv[2][2][2];           // B-VOP vector predictors: [forward, backward][field][x, y]
  int trb, trd;               // B-VOP: time from the past anchor, between the anchors
  const U8* scan;             // scan order (as far as known: intra blocks with AC prediction use other ones)
} VState;

// The families get the variables as they are: where to split a variable's values is the .idx files'
// (each index's thresholds), which tuning moves. The model only caps a variable where the values
// above the cap are few, and puts one of a wide range on tc_qlog's half octaves.
constexpr int kCap = 15;            // counts, runs, small sums and magnitudes: 0..kCap
constexpr int kNone = kCap + 1;     // ... and "none" (no such neighbour, no vector)
constexpr int kTinyCap = 7;         // the smaller ones (magnitudes coded so far, neighbours' sums): 0..kTinyCap
constexpr int kCountCap = 31;       // a block's count of coefficients, for its levels
constexpr int kHdrCap = 63;         // a header field's last value
constexpr int kDcSizeNone = countof(DCtab_lum);  // an intra DC difference's size (0-12 bits), none
constexpr int kMvcNone = kNone;     // the vector size of coefficients (0..kCap): no vector;
constexpr int kMvcIntra = kNone + 1;  // an intra block's: kMvcIntra + the size of its DC difference
constexpr int kVoteMax = 9;         // the frequency neighbours' signs, weighted (-kVoteMax..kVoteMax)
constexpr int kQ = RC_Q;            // levels as the decoder's contexts estimate them: in quarter levels
constexpr int kEstCap = 4 * kQ;     // an estimated level (quarter levels): 0..kEstCap
constexpr int kDcEstCap = 6 * kQ;   // an estimated DC difference (quarter levels)
constexpr int kDcMax = (1 << (kDcSizeNone - 1)) - 1;  // an intra DC difference's magnitude
constexpr int kLevelMax = ESC3_MAX - 1;  // a level less one (escape 3's 12 bits)
constexpr int kQlogCap = 40;        // tc_qlog of a wide range: 0..kQlogCap
constexpr int kTri = 3;             // the states of a three-state value (0, 1, kTriNone: a bit or none)
constexpr int kTriNone = kTri - 1;
constexpr int kPosCap = 31;         // hdr's pos (a flag's place in its field)
constexpr int kFsnCap = 3;          // mbt's fsn (which field select)
constexpr int kFracNone = 4;        // cbp's frac: none (else the vector's sub-pel components, a bit each)
constexpr int kPatMask = 31;        // cbp's pat: the bits of the blocks before, the last 5
constexpr int kHist = 8, kHistLong = 16;  // sig's h8 and h16: the significance flags before
constexpr int kRbCap = 3;           // lvl's rb: the magnitudes right and below, each to kRbCap
constexpr int kMagMax = UINT8_MAX;  // BlkCtx's magnitudes
constexpr int kSignMag = 5;         // sgn's lam: a block's sign and magnitude (sign_mag())
constexpr int kGh = kTri * kTri * kTri;  // sgn's gh: the last three signs, in base 3
// sgn's vote: the left and above frequency neighbours' signs (+-kVoteNb, times kVoteBig for a
// magnitude above 1), the above left one's against them (+-kVoteDiag)
constexpr int kVoteNb = 2, kVoteBig = 2, kVoteDiag = 1;
static_assert(kVoteMax == 2 * kVoteNb * kVoteBig + kVoteDiag, "sgn's vote: its range");
// the hdr family's ty: a VOP's type less PT_I, or none
constexpr int TY_NONE = PT_N - PT_I;
constexpr int kQuantStart = 4;  // the quantiser the header model predicts before the first VOP
constexpr int kTimeMax = 1 << 30;  // the display times' differences, clamped
constexpr i64 kRawLenMax = (i64)1 << 40;  // a raw chunk's length as decoded (more: damaged data)
constexpr int kTailCtx = 4;  // the tail's bytes: in the context of the byte before's top 4 bits

constexpr int kMvCands = 3;     // the candidates of a vector's prediction
constexpr int kHpelMask = 1, kQpelMask = 3;  // a vector component's sub-sample bits

// a three-state pair (3x3) as one variable
static INLINE int tri2(int a, int b) { return a * kTri + b; }
static int time_clamp(S64 t) { return (int)(t < -kTimeMax ? -kTimeMax : t > kTimeMax ? kTimeMax : t); }
// sig's fp: the bits of the frequency neighbours left, above, above left, 2 left, 2 above
enum { FP_L, FP_A, FP_AL, FP_L2, FP_A2 };
// mbt's rt: the co-located macroblock another kind, of the kind the decision asks of, none, of the
// kind next to it (skipped for intra, inter for 4MV)
enum { RT_OTHER, RT_SAME, RT_NONE, RT_NEXT };
// skp's st: the VOP (P, S with GMC, B); skp's rr: the co-located macroblock not coded (or none),
// coded, intra
enum { SKS_P, SKS_GMC, SKS_B };
enum { RR_NONE, RR_CODED, RR_INTRA };
// cbp's and qpd's c3: the macroblock intra, P, B
enum { C3_INTRA, C3_P, C3_B };
// mvg's hm and thm: the magnitude below, at, above the repeat's (the template's), or none
enum { HM_NONE, HM_BELOW, HM_EQUAL, HM_ABOVE };
// block b's bit of a coded block pattern (block 0 in the top bit)
static INLINE int cbp_bit(int cbp, int b) { return (cbp >> (MB_BLOCKS - 1 - b)) & 1; }
// a block's coefficient at p as sgn's lam has it: 0 none, else 1 + its sign (x2) + its magnitude above 1
static INLINE int sign_mag(const BlkCtx* b, int p) { return b && b->mag[p] ? 1 + b->sgn[p] * 2 + (b->mag[p] > 1) : 0; }

static INLINE int cap(int v, int c) { return v < c ? v : c; }
// quarter levels to levels, rounded
static INLINE int q4_round(int a) { return (a + kQ / 2) / kQ; }
// a nonnegative value of a wide range on tc_qlog's half octaves, 1 + (0 for none)
static INLINE int qlog1(i64 v) { return 1 + cap(tc_qlog(v), kQlogCap); }
// a magnitude in VLC units (or a sum of them), capped; or none
static INLINE int mvq(int q) { return cap(q, kCap); }
// the spread of the vector prediction's candidates (s < 0: none) in VLC units of f_code
static int mv_spread(int s, int fcode) { return s < 0 ? kNone : mvq(s ? ((s - 1) >> (fcode - 1)) + 1 : 0); }
static int mv_wrap(int v, int fcode) { return sign_extend(v, MV_WRAP_BITS + fcode); }  // as the decoder adds them

// a chunk's start code, as the raw family's code
enum ChunkClass {
  SCC_UNKNOWN,  // (its first bytes not coded yet)
  SCC_VOS, SCC_VOS_END, SCC_USER_DATA, SCC_GOV, SCC_VISOBJ, SCC_VOP, SCC_VO, SCC_VOL,
  SCC_SYSTEM,  // reserved and system start codes
  SCC_OTHER,   // other reserved ones
  SCC_NONE     // no start code
};
static int sc_class(int b) {
  return b == SC_VOS ? SCC_VOS : b == SC_VOS_END ? SCC_VOS_END : b == SC_USER_DATA ? SCC_USER_DATA : b == SC_GOV ? SCC_GOV
       : b == SC_VISOBJ ? SCC_VISOBJ : b == SC_VOP ? SCC_VOP : b < SC_VOL ? SCC_VO : b < SC_VOL_END ? SCC_VOL
       : b >= SC_RESERVED ? SCC_SYSTEM : SCC_OTHER;
}

constexpr U32 TAIL_MAX = 1 * MIB;  // the most bytes after a VOP's last macroblock it codes

enum { W_VERIFY, W_ENCODE, W_DECODE };  // what vop_walk() does
enum { W_FAIL = -1, W_NOMEM = -2 };     // and how it fails (the data, or out of memory)
enum { CK_END, CK_VOP, CK_RAW };        // a chunk's kind (code_kind()): the end, a VOP as records, bytes


// can the models code everything in this macroblock (escape mode 3 allows a level of 0)
static int mb_codable(const MB* m) {
  int i, j;
  for (i = 0; i < MB_BLOCKS; i++)
    for (j = 0; j < m->b[i].n; j++)
      if (!m->b[i].level[j]) return 0;
  return 1;
}

static void vop_state(Vop* v, const Ctx* cx, const VopHdr* h) {
  v->type = h->type;
  v->gmc = h->type == PT_S && cx->sprite_usage == GMC_SPRITE;
  v->fcode = h->fcode == NA ? 1 : h->fcode;
  v->bcode = h->bcode == NA ? 1 : h->bcode;
  v->qscale = clipi(h->quant, QS_MIN, QS_MAX);
  v->dcthr = dc_threshold[h->dcthr];
  v->tib = h->tib;
}

// the tables of a level's family, mapped as a run starts (zeroed pages)
template <class T>
static T* tc_table() {
  return (T*)tc_map(sizeof(T));
}

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.

template <int f_DEC, class LV>
struct Model {
  static constexpr int ENC = f_DEC == 0;
  Coder* c;  // the run's coder
  // each family's coder (f_hdr ...) and tables (t_hdr ...)
#define M_FAM(F)                       \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;
  M4V_FAMS(M_FAM)
#undef M_FAM
  // (a level reads no variable its parts cannot make: famvars.inc)
#define M_PARTS(F)                                                         \
  static_assert(LV::recon || !(LV::F::Uses & M_##F##_needs<NEED_REC>()), \
                "a level without the decoder reads a variable of " #F " that needs it");
  M4V_FAMS(M_PARTS)
#undef M_PARTS
  Last last;
  BlkCtx rowb[MAX_MB_W][MB_BLOCKS];
  // per macroblock, mb_cap of them: the current VOP's and the last anchor VOP's
  MBCtx *mbcur, *mbref;
  MV4 *mvcur, *mvref;
  int mb_cap;
  int ref_mbs;  // size of the VOP in mbref, 0 if none
  U8 tailbuf[TAIL_MAX];
  MB mrec;      // the macroblock vop_walk() has
  Recon rec;    // the decoder in the loop (LV::recon)
  RcEst est;    // its intra estimate for the macroblock (est.ok: there is one)
  int tmk[2], tmx[2], tmy[2];  // its vectors by template matching, forward and backward (tmk: found)
  int est_err, est_nb;         // how far the macroblock's blocks so far were from the estimate (quarter levels)

  void Init(void) {
#define M_INIT(F) f_##F.Init(), t_##F = nullptr;
    M4V_FAMS(M_INIT)
#undef M_INIT
    mbcur = mbref = 0;
    mvcur = mvref = 0;
    mb_cap = 0;
    if (LV::recon) rc_init(&rec);
  }
  void Quit(void) {
#define M_QUIT(F)                                                                                    \
  if (t_##F) LV::F::quit(t_##F), tc_unmap(t_##F, sizeof(typename LV::F::T)), t_##F = nullptr; \
  f_##F.Quit();
    M4V_FAMS(M_QUIT)
#undef M_QUIT
    free(mbcur);
    free(mbref);
    free(mvcur);
    free(mvref);
    mbcur = mbref = 0;
    mvcur = mvref = 0;
    mb_cap = 0;
    if (LV::recon) rc_quit(&rec);
  }
  // the run starts: the families' tables mapped and wired to the coder co
  void start(Coder* co) {
    int i;
    c = co;
#define M_MAP(F) t_##F = tc_table<typename LV::F::T>(), LV::F::init(t_##F), f_##F.wire(t_##F, c);
    M4V_FAMS(M_MAP)
#undef M_MAP
    memset(&last, 0, sizeof(last));
    last.tib = TIB_MIN;
    for (i = 0; i < PT_N; i++) { last.quant[i] = kQuantStart; last.fcode[i] = FCODE_MIN; }
    last.bcode = FCODE_MIN;
    ref_mbs = 0;
  }
  // room for n macroblocks (0: no memory)
  int mb_room(int n) {
    void* q;
    if (n <= mb_cap) return 1;
    if (!(q = realloc(mbcur, n * sizeof(MBCtx)))) return 0;
    mbcur = (MBCtx*)q;
    if (!(q = realloc(mbref, n * sizeof(MBCtx)))) return 0;
    mbref = (MBCtx*)q;
    if (!(q = realloc(mvcur, n * sizeof(MV4)))) return 0;
    mvcur = (MV4*)q;
    if (!(q = realloc(mvref, n * sizeof(MV4)))) return 0;
    mvref = (MV4*)q;
    memset(mbref + mb_cap, 0, (n - mb_cap) * sizeof(MBCtx));
    memset(mvref + mb_cap, 0, (n - mb_cap) * sizeof(MV4));
    mb_cap = n;
    return 1;
  }
  void rowb_clear(int x) {
    int i;
    memset(rowb[x], 0, sizeof(rowb[x]));
    for (i = 0; i < MB_BLOCKS; i++) rowb[x][i].last = rowb[x][i].dcs = -1;
  }

  // family f's contexts from its variables D, selected: what its bit()s code with
  template <class F, class V>
  static INLINE void sel(F& f, const V& D) {
    tcx X;
    f.make(D, X);
    f.select(X);
  }
  // a value decompressed outside what it can be: the data is damaged (the walk fails the chunk)
  INLINE i64 bound(i64 x, i64 lo, i64 hi) {
    if (UNLIKELY(x < lo || x > hi)) {
      c->err = 1;
      return lo;
    }
    return x;
  }

  // --- the headers: the hdr family, a field (fld) of the VOP of type ty (0 I, 1 P, 2 B, 3 S,
  // TY_NONE none) at a time, whose last value is p1
  INLINE void hdr_vars(M_hdr_v& D, int fld, int ty, i64 p1, int pos) {
    D.fld = fld;
    D.ty = ty;
    D.p1 = (int)(p1 < 0 ? 0 : p1 > kHdrCap ? kHdrCap : p1);
    D.pos = pos;
  }
  i64 hdr_val(int fld, int ty, i64 p1, int pos, i64 x) {  // x >= 0
    M_hdr_v D;
    tcx X;
    hdr_vars(D, fld, ty, p1, pos);
    f_hdr.make(D, X);
    return f_hdr.code(X, ENC ? x : 0);
  }
  i64 hdr_sval(int fld, int ty, i64 p1, int pos, i64 x) {  // signed
    M_hdr_v D;
    tcx X;
    hdr_vars(D, fld, ty, p1, pos);
    f_hdr.make(D, X);
    return f_hdr.codes(X, ENC ? x : 0, (u32)fld);
  }
  int hdr_flag(int fld, int ty, i64 p1, int pos, int b) {
    M_hdr_v D;
    hdr_vars(D, fld, ty, p1, pos);
    sel(f_hdr, D);
    return f_hdr.bit(0, ENC && b);
  }

  int code_kind(int k) {  // CK_*
    c->cat = CAT_RAW;
    k = (int)bound(hdr_val(F_KIND, TY_NONE, last.kind, 0, k), CK_END, CK_RAW);
    return last.kind = k;
  }

  // a chunk kept as bytes: its length, then the bytes (the raw family: the bytes before, the
  // chunk's start code, the place in it), which may come a piece at a time
  size_t code_raw_len(size_t n) {
    c->cat = CAT_RAW;
    last.rawat = last.rawcode = 0;
    last.rawhd = 0;
    return (size_t)bound(hdr_val(F_RAWLEN, TY_NONE, 0, 0, (i64)n), 0, kRawLenMax);
  }
  void code_raw_bytes(U8* p, size_t n) {
    M_raw_v D;
    size_t i;
    c->cat = CAT_RAW;
    for (i = 0; i < n; i++) {
      int x = 1, k;
      D.c1 = last.rawprev;
      D.c2 = last.rawprev2;
      D.code = last.rawcode;
      D.at = tc_qlog(last.rawat);
      sel(f_raw, D);
      for (k = BYTE_BITS - 1; k >= 0; k--) x = x * 2 + f_raw.bit(x, ENC && ((p[i] >> k) & 1));
      p[i] = (U8)(x - BYTE_VALUES);
      last.rawprev2 = last.rawprev;
      last.rawprev = p[i];
      if (last.rawat < SC_PREFIX) last.rawhd = last.rawhd << BYTE_BITS | p[i];
      else if (last.rawat == SC_PREFIX) last.rawcode = last.rawhd == SC_START ? sc_class(p[i]) : SCC_NONE;
      last.rawat++;
    }
  }

  void code_vop_hdr(const Ctx* cx, VopHdr* h) {
    const int ty0 = last.type ? last.type - PT_I : TY_NONE;
    int i, d, mask, ty;
    c->cat = CAT_HDR;
    h->type = PT_I + (int)bound(hdr_val(F_VTYPE, ty0, ty0, 0, h->type - PT_I), 0, PT_S - PT_I);
    ty = h->type - PT_I;
    h->mtb = (int)bound(hdr_val(F_MTB, ty, last.mtb, 0, h->mtb), 0, RUN_MAX);
    h->tib = (int)bound(hdr_val(F_TIB, ty, last.tib, 0, h->tib), TIB_MIN, TIB_MAX);
    mask = (int)((1u << h->tib) - 1);
    d = (h->tinc - last.tinc) & mask;
    if (hdr_flag(F_TSAME, ty, 0, 0, d == (last.dinc[h->type] & mask))) d = last.dinc[h->type] & mask;
    else d = (int)bound(hdr_val(F_TINC, ty, 0, 0, d), 0, mask);
    h->tinc = (last.tinc + d) & mask;
    last.type = h->type;
    last.mtb = h->mtb;
    last.tib = h->tib;
    last.tinc = h->tinc;
    last.dinc[h->type] = d;
    {  // display times (FFmpeg's decode_vop_header) for the B-VOP vector contexts
      const S64 res = cx->time_res > 0 ? cx->time_res : 1;
      S64 t;
      if (h->type != PT_B) {
        last.last_time_base = last.time_base;
        last.time_base += h->mtb;
        t = last.time_base * res + h->tinc;
        t -= last.last_non_b;
        last.pp_time = time_clamp(t);
        last.last_non_b = last.time_base * res + h->tinc;
      } else {
        t = (last.last_time_base + h->mtb) * res + h->tinc;
        t = last.pp_time - (last.last_non_b - t);
        last.pb_time = time_clamp(t);
      }
    }
    h->coded = hdr_flag(F_CODED, ty, 0, 0, h->coded);
    if (!ENC) {
      h->rnd = h->dcthr = h->tff = h->alt = h->quant = h->fcode = h->bcode = NA;
      h->ncplx = 0;
    }
    if (!h->coded) return;
    if (h->type == PT_P || (h->type == PT_S && cx->sprite_usage == GMC_SPRITE))
      last.rnd = h->rnd = hdr_flag(F_RND, ty, last.rnd, 0, h->rnd);
    h->ncplx = cx->cplx_i + (h->type != PT_I ? cx->cplx_p : 0) + (h->type == PT_B ? cx->cplx_b : 0);
    for (i = 0; i < h->ncplx; i++)
      h->cplx[i] = (char)('0' + hdr_flag(F_CPLX, ty, 0, cap(i, kPosCap), h->cplx[i] == '1'));
    last.dcthr = h->dcthr = (int)bound(hdr_val(F_DCTHR, ty, last.dcthr, 0, h->dcthr), 0, DCTHR_MAX);
    if (cx->interlaced) {
      last.tff = h->tff = hdr_flag(F_TFF, ty, last.tff, 0, h->tff);
      last.alt = h->alt = hdr_flag(F_ALT, ty, last.alt, 0, h->alt);
    }
    if (h->type == PT_S && cx->sprite_usage == GMC_SPRITE)
      for (i = 0; i < 2 * cx->warp_points; i++)
        h->warp[i] = (int)bound(hdr_sval(F_WARP, ty, 0, i & 1, h->warp[i]), -WARP_MAX, WARP_MAX);
    h->quant = last.quant[h->type] + (int)hdr_sval(F_QUANT, ty, 0, 0, h->quant - last.quant[h->type]);
    h->quant = (int)bound(h->quant, QS_MIN, QUANT_MAX);
    last.quant[h->type] = h->quant;
    if (h->type != PT_I) {
      h->fcode = (int)bound(hdr_val(F_FCODE, ty, last.fcode[h->type], 0, h->fcode), FCODE_MIN, FCODE_MAX);
      last.fcode[h->type] = h->fcode;
    }
    if (h->type == PT_B) {
      h->bcode = (int)bound(hdr_val(F_BCODE, ty, last.bcode, 0, h->bcode), FCODE_MIN, FCODE_MAX);
      last.bcode = h->bcode;
    }
  }

  void code_vp(const Ctx* cx, VState* s, VpHdr* k, int expect) {
    Vop* v = &s->v;
    const int ty = v->type - PT_I;
    int i;
    c->cat = CAT_HDR;
    k->st = (int)bound(hdr_val(F_VPST, ty, 0, 0, k->st), 0, RUN_MAX);
    if (hdr_flag(F_VPMBSAME, ty, 0, 0, k->mbnum == expect)) k->mbnum = expect;
    else k->mbnum = (int)bound(hdr_val(F_VPMB, ty, 0, 0, k->mbnum), 0, MAX_MBS);
    k->quant = (int)bound(v->qscale + hdr_sval(F_VPQ, ty, 0, 0, k->quant - v->qscale), 0, QUANT_MAX);
    k->hec = hdr_flag(F_HEC, ty, 0, 0, k->hec);
    if (k->quant) v->qscale = k->quant;
    v->qscale = clipi(v->qscale, QS_MIN, QS_MAX);
    if (!ENC) k->mtb = k->tinc = k->type = k->dcthr = k->fcode = k->bcode = NA;
    if (!k->hec) return;
    k->mtb = (int)bound(hdr_val(F_HMTB, ty, 0, 0, k->mtb), 0, RUN_MAX);
    k->tinc = (int)bound(hdr_val(F_HTINC, ty, 0, 0, k->tinc), 0, (1 << v->tib) - 1);
    k->type = (int)bound(hdr_val(F_HTYPE, ty, 0, 0, k->type), 0, PT_S - PT_I);
    k->dcthr = (int)bound(hdr_val(F_HDCTHR, ty, 0, 0, k->dcthr), 0, DCTHR_MAX);
    if (v->gmc)
      for (i = 0; i < 2 * cx->warp_points; i++)
        k->warp[i] = (int)bound(hdr_sval(F_HWARP, ty, 0, i & 1, k->warp[i]), -WARP_MAX, WARP_MAX);
    if (v->type != PT_I) k->fcode = (int)bound(hdr_val(F_HFCODE, ty, 0, 0, k->fcode), FCODE_MIN, FCODE_MAX);
    if (v->type == PT_B) k->bcode = (int)bound(hdr_val(F_HBCODE, ty, 0, 0, k->bcode), FCODE_MIN, FCODE_MAX);
  }


  // what follows the last macroblock: nb bits up to the byte boundary, then *n bytes
  int code_tail(int nb, int* bits, int* n, U8* bytes, int max) {
    int i, canon = 0;
    c->cat = CAT_HDR;
    if (ENC) canon = nb ? *bits == stuffing_bits(nb) && !*n : *n == 1 && bytes[0] == stuffing_bits(BYTE_BITS);
    if (hdr_flag(F_TAILC, TY_NONE, 0, nb, canon)) {  // the usual stuffing, to the byte boundary
      if (nb) { *bits = stuffing_bits(nb); *n = 0; }
      else { *bits = 0; *n = 1; bytes[0] = (U8)stuffing_bits(BYTE_BITS); }
      return 0;
    }
    *bits = (int)bound(hdr_val(F_TAILB, TY_NONE, 0, nb, *bits), 0, (1 << nb) - 1);
    *n = (int)bound(hdr_val(F_TAILN, TY_NONE, 0, 0, *n), 0, max + 1);
    if (*n > max) return -1;
    for (i = 0; i < *n; i++)
      bytes[i] = (U8)bound(hdr_val(F_TAILBYTE, TY_NONE, i ? bytes[i - 1] >> kTailCtx : 0, 0, bytes[i]), 0, BYTE_MASK);
    return 0;
  }

  // intra DC difference (the dcs family): its magnitude, then its sign. Contexts: the previous
  // block's size, the left and above blocks' (and their coefficient counts), whether the block has
  // AC coefficients, the quantiser; for the sign, the left and above blocks' and the previous one's
  // (e4: the intra estimate's difference, quarter levels, if eok; tq how far to trust it)
  int code_dc(VState* s, int lc, int v, const BlkCtx* Lb, const BlkCtx* Ab, int coded, int qs, int eok, int e4,
              int tq) {
    M_dcs_v D;
    tcx X;
    i64 a;
    int neg;
    const int ea = e4 < 0 ? -e4 : e4;
    D.lc = lc;
    D.prev = s->dc_prev[lc];
    D.ls = Lb && Lb->dcs >= 0 ? Lb->dcs : kDcSizeNone;
    D.as = Ab && Ab->dcs >= 0 ? Ab->dcs : kDcSizeNone;
    D.coded = coded;
    D.qs = qs;
    D.lbn = Lb ? cap(Lb->n, kCap) : kNone;
    D.abn = Ab ? cap(Ab->n, kCap) : kNone;
    D.ls2 = Lb && Lb->dcs > 0 ? 1 + Lb->dcsg : 0;
    D.as2 = Ab && Ab->dcs > 0 ? 1 + Ab->dcsg : 0;
    D.psg = s->dc_sgn[lc];
    D.sz = 0;
    D.ie = eok ? 1 + cap(ea, kDcEstCap) : 0;
    D.es = eok && ea >= kQ / 2 ? 1 + (e4 < 0) : 0;
    D.tq = tq;
    f_dcs.make(D, X);
    f_dcs.hint = eok ? cap(q4_round(ea), kDcMax) : -1;
    a = bound(f_dcs.code(X, ENC ? (v < 0 ? -v : v) : 0), 0, kDcMax);
    f_dcs.hint = -1;
    s->dc_prev[lc] = bitlen((int)a);
    if (!a) return 0;
    D.sz = bitlen((int)a);
    sel(f_dcs, D);
    neg = f_dcs.bit(TC_NODE, ENC && v < 0);
    s->dc_sgn[lc] = 1 + neg;
    return neg ? -(int)a : (int)a;
  }

  // (level -9) an estimated level (quarter levels, signed) as a variable: 1 + its magnitude, capped
  static int est_level(int e4) { return 1 + cap(e4 < 0 ? -e4 : e4, kEstCap); }
  // how far to trust the estimate (0 no estimate): before the macroblock's first block by its
  // template's miss (kTqNoTemplate none, else kTqMiss + its mean difference, quarter samples, on
  // tc_qlog's scale), then by how far the blocks so far were from it (kTqErr + their mean
  // difference per block, levels, on tc_qlog's scale)
  static constexpr int kTqNoTemplate = 1, kTqMiss = 2, kTqErr = 24;
  int est_trust() const {
    if (!est.ok) return 0;
    if (est_nb) return kTqErr + cap(tc_qlog(est_err / (kQ * est_nb)), kQlogCap - 1);
    return est.miss < 0 ? kTqNoTemplate : kTqMiss + cap(tc_qlog(est.miss), kTqErr - kTqMiss - 1);
  }

  // (level -9) skipping's variables, zt and ze (0 none)
  void skip_vars(int* zt, int* ze) {
    int a, b;
    rc_skip_feat(&rec, &a, &b);
    *zt = a < 0 ? 0 : qlog1(a);
    *ze = b < 0 ? 0 : qlog1(b);
  }

  // how far an intra block's levels as coded (cl, by raster position) were from the estimate (E4)
  void est_account(const int* E4, const int* cl) {
    int i, e = 0;
    for (i = 0; i < BLK_COEFS; i++) e += abs(kQ * cl[i] - E4[i]);
    est_err += e;
    est_nb++;
  }

  // Coefficients of a block, coded like H.264's CABAC residual: significance map in scan order
  // (the sig family), then the levels in reverse order (lvl), signs (sgn), and the escape modes
  // where they aren't the default (esc). The contexts: the neighbouring blocks' coefficient counts
  // (CAVLC's nC), the coefficients found so far, and the left and above blocks' coefficients at the
  // same position; the significance flags before (the last 8 by position, 16 hashed), the
  // frequency neighbours left and above in the block; for the levels, the magnitudes coded before
  // them and the neighbours right and below; for the signs, the signs coded before them and those
  // of the frequency neighbours (a block's coefficients come in patterns, as an edge leaves them).
  // cls: block class, nctx: neighbours' counts, qs: quantiser, mvc: motion vector size, rnz: the
  // co-located block's count; Lb, Ab: left and above blocks or 0; out (which may be Ab) gets this one;
  // Xb: for a Cr block, the Cb block of its macroblock (0 else)
  void code_block(VState* s, Blk* b, int n, int intra, int dcvlc, int coded, int cls, int nctx, int qs,
                  int mvc, int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx* out, const BlkCtx* Xb) {
    const RL* rl = intra ? &rl_intra : &rl_inter;
    const U8* sc = LV::recon && rec.on ? rc_scan(&rec, n, intra) : s->scan;  // (-9: an AC predicted block's own)
    BlkCtx o;
    U8 rm[BLK_COEFS];   // coefficients coded so far by raster position: 1 in the significance map, then magnitudes
    U8 nzr[BLK_COEFS];  // the significance map by raster position
    int pos[BLK_COEFS], lev[BLK_COEFS], k = 0, i, j, start, prev;
    U32 sh = 0;  // the significance flags so far, the last in bit 0
    // (level -9) E4: an intra block's estimate, the levels coded (the prediction taken off) by raster
    // position, quarter levels (eon), its trust tq; or an inter block's levels its edges ask for
    // (pon), its prediction's texture ptx and edge miss pem
    int E4[BLK_COEFS], eon = 0, tq = 0, ptx = 0, pem = 0, pon = 0, PF[BLK_COEFS], ptd = RC_TD_NONE;
    memset(&o, 0, sizeof(o));
    memset(rm, 0, sizeof(rm));
    o.last = o.dcs = -1;
    if (LV::recon && intra && est.ok) {
      int P[BLK_COEFS], dir;
      rc_intra_pred(&rec, n, &dir, P);
      for (i = 0; i < BLK_COEFS; i++) E4[i] = rc_est_q4(&rec, &est, n, i) - kQ * P[i];
      eon = 1;
      tq = est_trust();
    }
    if (LV::recon && rec.on && !intra && coded) {
      rc_pred_feat(&rec, n, 1, n < MB_LUMA && rec.interlaced_dct, &ptx, &pem);
      ptx = qlog1(ptx);
      pem = pem < 0 ? 0 : qlog1(pem);
      pon = rc_edge_proj(&rec, n, E4);
      rc_pred_spec(&rec, n, n < MB_LUMA && rec.interlaced_dct, PF, &ptd);
    }
    if (dcvlc) {
      c->cat = CAT_DC;
      b->dc = code_dc(s, n >= MB_LUMA, b->dc, Lb, Ab, coded, qs, eon, eon ? E4[0] : 0, tq);
      o.dcs = (S8)bitlen(b->dc);
      o.dcsg = b->dc < 0;
    } else if (!ENC)
      b->dc = 0;
    if (!coded) {
      b->n = 0;
      *out = o;
      if (eon) {
        int cl[BLK_COEFS];
        memset(cl, 0, sizeof cl);
        cl[0] = dcvlc ? b->dc : 0;
        est_account(E4, cl);
      }
      return;
    }
    start = dcvlc ? 1 : 0;
    if (dcvlc) mvc = kMvcIntra + o.dcs;  // (an intra block has no vector: the DC difference's size)
    if (ENC)
      for (j = 0, prev = start - 1; j < b->n; j++) { pos[j] = prev += b->run[j] + 1; lev[j] = b->level[j]; }
    c->cat = CAT_SIG;
    {
      M_sig_v S;
      S.cls = cls;
      S.nc = nctx;
      S.qs = qs;
      S.mvc = mvc;
      S.rnz = rnz;
      for (i = start, prev = start - 1; i < BLK_LAST; i++) {
        const int r = sc[i], u = r & (BLK_W - 1), v = r >> BLK_SHIFT, run = i - prev - 1;
        S.pos = i;
        S.kb = cap(k, kCap);
        S.nz = tri2(Lb ? Lb->mag[i] != 0 : kTriNone, Ab ? Ab->mag[i] != 0 : kTriNone);
        S.past = tri2(Lb ? i > Lb->last : kTriNone, Ab ? i > Ab->last : kTriNone);  // beyond their last coefficient
        S.run = cap(run, kCap);
        S.fp = (u ? rm[r - 1] : 0) << FP_L | (v ? rm[r - BLK_W] : 0) << FP_A | (u && v ? rm[r - BLK_W - 1] : 0) << FP_AL |
               (u > 1 ? rm[r - 2] : 0) << FP_L2 | (v > 1 ? rm[r - 2 * BLK_W] : 0) << FP_A2;
        S.xm = Xb ? cap(Xb->mag[i], kTinyCap) : kTinyCap + 1;
        S.h8 = (int)(sh & ((1u << kHist) - 1));
        S.h16 = (int)(sh & ((1u << kHistLong) - 1));
        S.sl = Lb ? cap(Lb->suf[i + 1], kCap) : kNone;
        S.sa = Ab ? cap(Ab->suf[i + 1], kCap) : kNone;
        S.ie = eon ? est_level(E4[sc[i]]) : 0;
        S.tq = tq;
        S.tx = ptx;
        S.em = pem;
        S.pe = pon ? est_level(E4[sc[i]]) : 0;
        S.td = ptd;
        S.pf = pon ? qlog1((PF[sc[i]] < 0 ? -PF[sc[i]] : PF[sc[i]]) / rec.qscale) : 0;
        sel(f_sig, S);
        if (!f_sig.bit(SIG_SIG, ENC && k < b->n && pos[k] == i)) {
          sh <<= 1;
          continue;
        }
        sh = sh << 1 | 1;
        pos[k] = prev = i;
        rm[r] = 1;
        if (f_sig.bit(SIG_LAST, ENC && k == b->n - 1)) {
          k++;
          break;
        }
        k++;
      }
    }
    if (i == BLK_LAST) pos[k++] = BLK_LAST;  // reached the end: the last coefficient is there
    rm[sc[pos[k - 1]]] = 1;
    memcpy(nzr, rm, sizeof(rm));
    memset(rm, 0, sizeof(rm));
    c->cat = CAT_LEV;
    {
      // lh: the magnitudes coded before, the last three, each at most 3 in 2 bits; ssum: their sum
      constexpr int kLhBase = 4, kLhMask = 63;
      int ngt1 = 0, neq1 = 0, pm = 0, lh = 0, ssum = 0;
      M_lvl_v L;
      L.cls = cls;
      L.qs = qs;
      L.kc = cap(k, kCountCap);
      L.mvc = mvc;
      for (j = k - 1; j >= 0; j--) {
        const int a = ENC ? (lev[j] < 0 ? -lev[j] : lev[j]) - 1 : 0, p = pos[j], r = sc[p], u = r & (BLK_W - 1),
                  v = r >> BLK_SHIFT;
        const int nm = (Lb ? Lb->mag[p] : 0) + (Ab ? Ab->mag[p] : 0);
        // the frequency neighbours right, below and below right, coded already
        const int ur = u < BLK_W - 1, vb = v < BLK_W - 1;  // (there are some)
        const int sum = (ur ? rm[r + 1] : 0) + (vb ? rm[r + BLK_W] : 0) + (ur && vb ? rm[r + BLK_W + 1] : 0);
        const int ra = ur ? cap(rm[r + 1], kRbCap) : 0, ba = vb ? cap(rm[r + BLK_W], kRbCap) : 0;
        tcx X;
        i64 t;
        L.pos = p;
        L.gs = ngt1 ? 0 : 1 + cap(neq1, kTinyCap - 1);
        L.nmb = cap(nm, kTinyCap);
        L.pm = pm;
        L.nb2 = cap(sum, kTinyCap);
        L.den = (ur ? nzr[r + 1] : 0) + (vb ? nzr[r + BLK_W] : 0) + (u ? nzr[r - 1] : 0) + (v ? nzr[r - BLK_W] : 0);
        L.lh = lh;
        L.rb = ra * (kRbCap + 1) + ba;
        L.ssb = ssum ? qlog1(ssum) : 0;
        L.ngt = cap(ngt1, kTinyCap);
        L.ie = eon ? est_level(E4[sc[p]]) : 0;
        L.tq = tq;
        L.tx = ptx;
        L.em = pem;
        L.pe = pon ? est_level(E4[sc[p]]) : 0;
        L.pf = pon ? qlog1((PF[sc[p]] < 0 ? -PF[sc[p]] : PF[sc[p]]) / rec.qscale) : 0;
        f_lvl.make(L, X);
        if (eon) {
          const int ea = E4[sc[p]] < 0 ? -E4[sc[p]] : E4[sc[p]];
          f_lvl.hint = q4_round(ea) > 1 ? q4_round(ea) - 1 : 0;
        }
        t = bound(f_lvl.code(X, a), 0, kLevelMax);
        f_lvl.hint = -1;
        if (t) ngt1++;
        else neq1++;
        lev[j] = (int)t + 1;
        pm = cap((int)t + 1, kTinyCap);
        lh = (lh * kLhBase + cap(pm, kLhBase - 1)) & kLhMask;
        ssum += lev[j];
        rm[sc[p]] = o.mag[p] = (U8)cap(lev[j], kMagMax);
      }
    }
    o.last = (S8)pos[k - 1];
    o.n = (U8)k;
    for (i = BLK_LAST; i >= 0; i--) o.suf[i] = (U8)(o.suf[i + 1] + (o.mag[i] != 0));
    c->cat = CAT_SIGN;
    {
      U8 srs[BLK_COEFS];   // the signs coded so far by raster position: 0 none, 1 +, 2 -
      int sgh = 0;  // the last 3 signs (1 +, 2 -) in base 3
      M_sgn_v G;
      RcSign sp;    // (level -9) the first K signs predicted, coded after the others
      const int K = LV::recon && rec.on && rc_sign_can(&rec, n) ? (k < RC_SPK ? k : RC_SPK) : 0;
      int t;
      memset(srs, 0, sizeof(srs));
      G.cls = cls;
      for (t = 0; t < k; t++) {
        int p, r, u, v;
        j = t < k - K ? K + t : t - (k - K);
        if (K && t == k - K) rc_sign_init(&rec, &sp, n, intra, dcvlc, b->dc, k, pos, lev, K);
        p = pos[j], r = sc[p], u = r & (BLK_W - 1), v = r >> BLK_SHIFT;
        const int ls = Lb && Lb->mag[p] ? Lb->sgn[p] : kTriNone, as = Ab && Ab->mag[p] ? Ab->sgn[p] : kTriNone;
        // the frequency neighbours left, above (and 2 away), above left: coded before, in zigzag order
        const int fl = u ? srs[r - 1] : 0, fu = v ? srs[r - BLK_W] : 0, fd = u && v ? srs[r - BLK_W - 1] : 0;
        const int fl2 = u > 1 ? srs[r - 2] : 0, fu2 = v > 1 ? srs[r - 2 * BLK_W] : 0;
        int vote = 0;  // their signs, weighted by their magnitudes
        if (fl) vote += (fl == 1 ? kVoteNb : -kVoteNb) * (rm[r - 1] > 1 ? kVoteBig : 1);
        if (fu) vote += (fu == 1 ? kVoteNb : -kVoteNb) * (rm[r - BLK_W] > 1 ? kVoteBig : 1);
        if (fd) vote += fd == 1 ? -kVoteDiag : kVoteDiag;
        G.pos = p;
        G.first = j == 0;
        G.lsa = tri2(ls, as);
        G.gh = sgh;
        G.ps = j && lev[j - 1] < 0;
        G.flu = tri2(fl, fu);
        G.flu2 = tri2(fl2, fu2);
        G.vote = vote + kVoteMax;
        G.fdlu = tri2(tri2(fd, fl), fu);
        G.xs = Xb && Xb->mag[p] ? 1 + Xb->sgn[p] : 0;
        G.lam = sign_mag(Lb, p) * kSignMag + sign_mag(Ab, p);
        G.pp = G.mg = G.pp2 = G.mg2 = 0;
        G.rk = RC_SPK;
        G.ie = eon ? est_level(E4[sc[p]]) : 0;
        G.es = eon && (E4[sc[p]] >= kQ / 2 || E4[sc[p]] <= -kQ / 2) ? 1 + (E4[sc[p]] < 0) : 0;  // (half a level)
        G.pj = pon && (E4[sc[p]] >= kQ / 2 || E4[sc[p]] <= -kQ / 2) ? 1 + (E4[sc[p]] < 0) : 0;
        G.pe = pon ? est_level(E4[sc[p]]) : 0;
        if (j < K && sp.nc) {
          int mg;
          G.pp = 1 + rc_sign_predict(&sp, 0, j, &mg);
          G.mg = mg ? qlog1(mg) : 0;
          G.pp2 = 1 + rc_sign_predict(&sp, 1, j, &mg);
          G.mg2 = mg ? qlog1(mg) : 0;
          G.rk = j;
        }
        sel(f_sgn, G);
        if (f_sgn.bit(0, ENC && b->level[j] < 0)) lev[j] = -lev[j];
        if (j < K && sp.nc) rc_sign_decide(&sp, j, lev[j] < 0);
        o.sgn[p] = lev[j] < 0;
        srs[r] = (U8)(1 + (lev[j] < 0));
        sgh = sgh * kTri % kGh + 1 + (lev[j] < 0);
      }
    }
    *out = o;  // (after the signs: out may be Ab)
    if (eon) {
      int cl[BLK_COEFS];
      memset(cl, 0, sizeof cl);
      if (dcvlc) cl[0] = b->dc;
      for (j = 0; j < k; j++) cl[sc[pos[j]]] = lev[j];
      est_account(E4, cl);
    }
    c->cat = CAT_ESC;
    for (j = 0, prev = start - 1; j < k; j++) {
      const int run = pos[j] - prev - 1, al = lev[j] < 0 ? -lev[j] : lev[j];
      const int d = esc_default(rl, j == k - 1, run, al);
      int e = ENC ? b->esc[j] : d;
      M_esc_v E;
      E.dm = d;
      sel(f_esc, E);
      if (!f_esc.bit(ESCN_USUAL, e == d)) {  // which, in a tree of two bits
        const int b1 = f_esc.bit(ESCN_HI, ENC && (e >> 1) & 1);
        e = b1 * 2 + f_esc.bit(ESCN_LO + b1, ENC && e & 1);
      }
      if (!ENC) { b->run[j] = (U8)run; b->level[j] = (S16)lev[j]; b->esc[j] = (U8)e; }
      prev = pos[j];
    }
    if (!ENC) b->n = k;
  }

  // coded block pattern, block 0 in bit 5 (the cbp family, the block its node). Contexts: the left
  // and above 8x8 blocks (in this macroblock or a neighbour), the size of the motion vector
  // differences (mvc, kNone for none), which components of the vector are sub-pel (frac, 4 for none),
  // the co-located macroblock of the last anchor VOP, the blocks coded so far, the quantiser (qs:
  // before this macroblock's dquant, which comes after the pattern), and lq: whether the left and
  // above macroblocks agree on having coded blocks (0), else 1 + whether the co-located one has some.
  // (level -9: cie, the intra estimate's AC coefficients per block; tq its trust; ctx, cem an inter
  // block's prediction's texture and its edge miss)
  int code_cbp(int cls3, const MBCtx* L, const MBCtx* A, const MBCtx* R, int mvc, int frac, int qs, int lq, int cbp,
               const int* cie, int tq, const int* ctx, const int* cem) {
    M_cbp_v D;
    int b, v = 0, n = 0;
    D.tq = tq;
    D.c3 = cls3;
    D.mvc = mvc;
    D.frac = frac;
    D.qs = qs;
    D.lq = lq;
    for (b = 0; b < MB_BLOCKS; b++) {
      int l, u, bit;
      if (b < MB_LUMA) {  // (b & 1: the right column, b & 2: the lower row)
        l = (b & 1) ? cbp_bit(v, b - 1) : L ? cbp_bit(L->cbp, b + 1) : kTriNone;
        u = (b & 2) ? cbp_bit(v, b - 2) : A ? cbp_bit(A->cbp, b + 2) : kTriNone;
      } else {
        l = L ? cbp_bit(L->cbp, b) : kTriNone;
        u = A ? cbp_bit(A->cbp, b) : kTriNone;
      }
      D.lu = tri2(l, u);
      D.ie = cie[b];
      D.tx = ctx[b];
      D.em = cem[b];
      D.rbit = R ? cbp_bit(R->cbp, b) : kTriNone;
      D.cnt = n;
      D.pat = v >> (MB_BLOCKS - b) & kPatMask;
      sel(f_cbp, D);
      bit = f_cbp.bit(b, ENC && cbp_bit(cbp, b));
      v |= bit << (MB_BLOCKS - 1 - b);
      n += bit;
    }
    return v;
  }

  // neighbouring blocks' coefficient counts (CAVLC's nC)
  static int nz_ctx(const MBCtx* me, const MBCtx* L, const MBCtx* A, int b) {
    int na, nb, m;
    if (b < MB_LUMA) {
      na = (b & 1) ? me->nz[b - 1] : L ? L->nz[b + 1] : -1;
      nb = (b & 2) ? me->nz[b - 2] : A ? A->nz[b + 2] : -1;
    } else {
      na = L ? L->nz[b] : -1;
      nb = A ? A->nz[b] : -1;
    }
    if (na < 0 && nb < 0) return 0;
    m = na < 0 ? nb : nb < 0 ? na : (na + nb + 1) >> 1;
    return 1 + cap(m, kCap);  // (0 none)
  }

  // A motion vector difference as coded, like CABAC's mvd: its magnitude in units of the VLC code
  // (so f_code doesn't change its statistics; the mvd family), its sign (mvg), the f_code residual
  // bits (mvd). Contexts: the neighbours' magnitudes (their sum, and each), the other component
  // (oth: of this vector for y, of the previous one for x, kNone for none), the co-located macroblock
  // of the last anchor VOP, and d: the difference that would give this vector the co-located one's
  // value (dok 0: none), whose magnitude is the magnitude's hint; with the decoder, td the difference
  // that would give it the vector the template matches best (tok 0: none), the hint then.
  int code_mvd(int kind, int comp, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, int oth, int dok, int d,
               int sp, int v, int* q, int tok, int td) {
    const int shift = fcode - 1, a = v < 0 ? -v : v;
    const int ad = (d < 0 ? -d : d) - 1, dq = d ? (ad >> shift) + 1 : 0;
    const int ds = !dok || !d ? 0 : d > 0 ? 1 : 2;
    const int tq = td ? (((td < 0 ? -td : td) - 1) >> shift) + 1 : 0, ts = !tok || !td ? 0 : td > 0 ? 1 : 2;
    M_mvd_v D;
    tcx X;
    int neg, r, i;
    i64 x;
    D.kind = kind;
    D.comp = comp;
    D.ns = mvq((L ? L->mva[comp] : 0) + (A ? A->mva[comp] : 0));
    D.oth = oth;
    D.rs = R ? mvq(R->mva[comp]) : kNone;
    D.dz = dok ? mvq(dq) : kNone;
    D.sp = sp;
    D.lq = L ? mvq(L->mva[comp]) : kNone;
    D.aq = A ? mvq(A->mva[comp]) : kNone;
    D.ds = ds;
    D.fc = shift;
    D.rbi = D.rsame = 0;
    D.tz = tok ? mvq(tq) : kNone;
    D.ts = ts;
    f_mvd.make(D, X);
    f_mvd.hint = ds ? dq : tok ? tq : -1;  // (the magnitude that repeats the co-located vector, or the template's)
    x = bound(f_mvd.code(X, ENC ? (a ? ((a - 1) >> shift) + 1 : 0) : 0), 0, MV_VLC_MAX);
    f_mvd.hint = -1;
    *q = (int)x;
    if (!x) return 0;
    {
      M_mvg_v G;
      G.kind = kind;
      G.comp = comp;
      G.ls = L ? L->mvs[comp] : 0;
      G.as = A ? A->mvs[comp] : 0;
      G.ds = ds;
      G.dz = D.dz;
      G.mag = mvq((int)x);
      G.hm = !ds ? HM_NONE : x < dq ? HM_BELOW : x == dq ? HM_EQUAL : HM_ABOVE;
      G.ts = ts;
      G.thm = !ts ? HM_NONE : x < tq ? HM_BELOW : x == tq ? HM_EQUAL : HM_ABOVE;
      sel(f_mvg, G);
      neg = f_mvg.bit(0, ENC && v < 0);
    }
    for (i = shift - 1, r = 0; i >= 0; i--) {  // residual bits, predicted by d's while they agree so far
      const int same = ds == 1 + neg && x == dq && ad >> (i + 1) == ((int)(x - 1) << (shift - i - 1) | r);
      D.rbi = i;
      D.rsame = same ? 1 + ((ad >> i) & 1) : 0;
      sel(f_mvd, D);
      r |= f_mvd.bit(TC_NODE + i, ENC && ((a - 1) >> i) & 1) << i;
    }
    x = ((x - 1) << shift | r) + 1;
    return neg ? -(int)x : (int)x;
  }

  // The decoder's prediction of the vector of block b (FFmpeg's h263_pred_motion): the median of the
  // left, above and above-right blocks. A candidate outside the picture or before the start of the
  // video packet is invalid: it counts as 0, and if only one is valid, that one is the prediction.
  void pred_mv(const VState* s, int idx, int b, int* px, int* py, int* sx, int* sy) {
    enum { NB_HERE = 0, NB_LEFT = -1, NB_ABOVE = -2, NB_ABOVE_RIGHT = -3 };
    static const S8 nb[MB_LUMA][kMvCands][2] = {  // [block][left, above, above-right]: macroblock, block
      { { NB_LEFT, 1 }, { NB_ABOVE, 2 }, { NB_ABOVE_RIGHT, 2 } },
      { { NB_HERE, 0 }, { NB_ABOVE, 3 }, { NB_ABOVE_RIGHT, 2 } },
      { { NB_LEFT, 3 }, { NB_HERE, 0 }, { NB_HERE, 1 } },
      { { NB_HERE, 2 }, { NB_HERE, 1 }, { NB_HERE, 0 } } };
    const int w = s->mb_w, x = idx % w, y = idx / w;
    int cx[kMvCands], cy[kMvCands], nv = 0, i;
    for (i = 0; i < kMvCands; i++) {
      const int k = nb[b][i][0];
      int j = idx, ok = 1;
      if (k == NB_LEFT) { j = idx - 1; ok = x > 0; }
      else if (k == NB_ABOVE) { j = idx - w; ok = y > 0; }
      else if (k == NB_ABOVE_RIGHT) { j = idx - w + 1; ok = y > 0 && x + 1 < w; }
      if (ok && k && j < s->vp_start) ok = 0;
      cx[i] = ok ? mvcur[j][nb[b][i][1]][0] : 0;
      cy[i] = ok ? mvcur[j][nb[b][i][1]][1] : 0;
      nv += ok;
    }
    if (nv == 1) {
      *px = cx[0] + cx[1] + cx[2];
      *py = cy[0] + cy[1] + cy[2];
      *sx = *sy = -1;
    } else {
      *sx = (cx[0] > cx[1] ? (cx[0] > cx[2] ? cx[0] : cx[2]) : (cx[1] > cx[2] ? cx[1] : cx[2])) -
            (cx[0] < cx[1] ? (cx[0] < cx[2] ? cx[0] : cx[2]) : (cx[1] < cx[2] ? cx[1] : cx[2]));
      *sy = (cy[0] > cy[1] ? (cy[0] > cy[2] ? cy[0] : cy[2]) : (cy[1] > cy[2] ? cy[1] : cy[2])) -
            (cy[0] < cy[1] ? (cy[0] < cy[2] ? cy[0] : cy[2]) : (cy[1] < cy[2] ? cy[1] : cy[2]));
      *px = cx[0] > cx[1] ? (cx[1] > cx[2] ? cx[1] : cx[0] > cx[2] ? cx[2] : cx[0])
                          : (cx[0] > cx[2] ? cx[0] : cx[1] > cx[2] ? cx[2] : cx[1]);
      *py = cy[0] > cy[1] ? (cy[1] > cy[2] ? cy[1] : cy[0] > cy[2] ? cy[2] : cy[0])
                          : (cy[0] > cy[2] ? cy[0] : cy[1] > cy[2] ? cy[2] : cy[1]);
    }
  }

  // one vector (x, y) of kind kind, predicted by (px, py) from candidates with spreads (sx, sy) (-1:
  // none), with the co-located vector (tx, ty) if tok; tm: the template's vector (tmk[], tmx[], tmy[])
  // of that direction, or -1
  void code_mv(MB* m, int* mvi, int kind, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, MBCtx* me, int* sumq,
               int px, int py, int sx, int sy, int tok, int tx, int ty, int* mx, int* my, int tm = -1) {
    const int first = *mvi == 0, mk = tm >= 0 && tmk[tm];
    int v, q;
    v = code_mvd(kind, 0, fcode, L, A, R, first ? kNone : mvq(me->mva[0]), tok, mv_wrap(tx - px, fcode),
                 mv_spread(sx, fcode), m->mv[*mvi], &q, mk, mk ? mv_wrap(tmx[tm] - px, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *mx = mv_wrap(px + v, fcode);
    if (first) { me->mva[0] = (U8)q; me->mvs[0] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
    v = code_mvd(kind, 1, fcode, L, A, R, mvq(q), tok, mv_wrap(ty - py, fcode), mv_spread(sy, fcode),
                 m->mv[*mvi], &q, mk, mk ? mv_wrap(tmy[tm] - py, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *my = mv_wrap(py + v, fcode);
    if (first) { me->mva[1] = (U8)q; me->mvs[1] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
  }

  // the macroblock's kind and flags: a decision of the mbt family, its node, with the variables of
  // the neighbours of the kind it asks of
  int mbt_bit(int node, M_mbt_v& D, int b) {
    sel(f_mbt, D);
    return f_mbt.bit(node, ENC && b);
  }
  // how many of the macroblocks are of the kind k
  static int count_of(int k, const MBCtx* a, const MBCtx* b) { return (a && a->cls == k) + (b && b->cls == k); }

  // one macroblock record; in decoding mode m is filled in
  void code_mb(const Ctx* cx, VState* s, MB* m, int idx) {
    Vop* v = &s->v;
    const int mb_x = idx % s->mb_w;
    const MBCtx* L = mb_x > 0 && mbcur[idx - 1].cls ? &mbcur[idx - 1] : 0;
    const MBCtx* A = idx >= s->mb_w && mbcur[idx - s->mb_w].cls ? &mbcur[idx - s->mb_w] : 0;
    const MBCtx* R = ref_mbs ? &mbref[idx] : 0;
    const MBCtx* AL = mb_x > 0 && idx > s->mb_w && mbcur[idx - s->mb_w - 1].cls ? &mbcur[idx - s->mb_w - 1] : 0;
    const MBCtx* AR =
        mb_x + 1 < s->mb_w && idx >= s->mb_w && mbcur[idx - s->mb_w + 1].cls ? &mbcur[idx - s->mb_w + 1] : 0;
    const int rb = cap(s->run, kCap);
    const int ty = v->type - PT_I;
    MBCtx* me = &mbcur[idx];
    const int dcvlc = v->qscale < v->dcthr;
    int intra = 0, cbp, cls3, i, k, mvi = 0, sumq = 0, px = 0, py = 0, sx = -1, sy = -1, frac = kFracNone;
    M_mbt_v T;
    memset(&T, 0, sizeof(T));
    T.st = v->type - PT_I;
    T.rc = R ? R->cls : 0;
    if (!ENC) {
      m->st = m->sk = 0;
      m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
      m->nfs = m->fs = m->nmv = 0;
    }
    memset(me, 0, sizeof(*me));
    est.ok = 0;
    tmk[0] = tmk[1] = 0;
    if (LV::recon && rec.on) rc_mb_begin(&rec, idx);
    c->cat = CAT_MB;
    if (v->type == PT_B && mb_x == 0) memset(s->lmv, 0, sizeof(s->lmv));
    if (v->type != PT_I && v->type != PT_B) pred_mv(s, idx, 0, &px, &py, &sx, &sy);
    if (v->type == PT_I) {
      m->st = (int)bound(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
      m->t = MB_INTRA;
      intra = 1;
      cls3 = C3_INTRA;
    } else if (v->type != PT_B) {
      M_skp_v K;
      m->st = (int)bound(hdr_val(F_MBST, ty, 0, 0, m->st), 0, RUN_MAX);
      K.st = v->gmc ? SKS_GMC : SKS_P;
      K.cl = count_of(C_SKIP, L, A);
      K.c4 = K.cl + count_of(C_SKIP, AL, AR);
      K.rt = R ? R->cls == C_SKIP : kTriNone;
      K.rc = R ? R->cls : 0;
      K.run = rb;
      K.pz = px || py;
      K.rz = !R ? kTriNone : mvref[idx][0][0] || mvref[idx][0][1];
      K.bsk = K.rr = 0;
      K.zt = K.ze = 0;
      if (LV::recon && rec.on) skip_vars(&K.zt, &K.ze);
      T.zt = K.zt;
      T.ze = K.ze;
      sel(f_skp, K);
      if ((m->sk = f_skp.bit(SKP_P, ENC && m->sk))) {
        me->cls = C_SKIP;
        me->bsk = !v->gmc;
        rowb_clear(mb_x);
        s->run++;
        if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
        return;
      }
      s->run = 0;
      T.cl = count_of(C_INTRA, L, A);
      T.c4 = T.cl + count_of(C_INTRA, AL, AR);
      T.rt = !R ? RT_NONE : R->cls == C_INTRA ? RT_SAME : R->cls == C_SKIP ? RT_NEXT : RT_OTHER;
      if (mbt_bit(MBN_INTRA, T, m->t == MB_INTRA)) {
        m->t = MB_INTRA;
        intra = 1;
        cls3 = C3_INTRA;
      } else {
        T.cl = count_of(C_4MV, L, A);
        T.c4 = T.cl + count_of(C_4MV, AL, AR);
        T.rt = !R ? RT_NONE : R->cls == C_4MV ? RT_SAME : R->cls == C_INTER ? RT_NEXT : RT_OTHER;
        m->t = mbt_bit(MBN_4MV, T, m->t == MB_4MV) ? MB_4MV : MB_INTER;
        if (v->gmc && m->t == MB_INTER) {
          T.cl = count_of(C_GMC, L, A);
          T.c4 = T.cl + count_of(C_GMC, AL, AR);
          T.rt = !R ? RT_NONE : R->cls == C_GMC ? RT_SAME : RT_OTHER;
          m->mc = mbt_bit(MBN_GMC, T, m->mc);
        }
        cls3 = C3_P;
      }
    } else {
      M_skp_v K;
      memset(&K, 0, sizeof(K));
      K.st = SKS_B;
      K.rc = R ? R->cls : 0;
      K.bsk = R ? R->bsk : kTriNone;
      sel(f_skp, K);
      if ((m->sk = f_skp.bit(SKP_B, ENC && m->sk))) {
        me->cls = C_SKIP;
        rowb_clear(mb_x);
        if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
        return;
      }
      K.cl = count_of(C_BDIR0, L, A);
      K.c4 = K.cl + count_of(C_BDIR0, AL, AR);
      K.rt = R ? R->cls == C_BDIR0 : kTriNone;
      K.run = rb;
      K.rr = !R || R->cls == C_SKIP ? RR_NONE : R->cls == C_INTRA ? RR_INTRA : RR_CODED;
      if (LV::recon && rec.on) skip_vars(&K.zt, &K.ze);
      T.zt = K.zt;
      T.ze = K.ze;
      sel(f_skp, K);
      if (f_skp.bit(SKP_BDIR, ENC && m->mb == MODB_NONE)) {
        m->mb = MODB_NONE;
        m->t = MB_DIRECT;
        for (i = 0; i < MB_BLOCKS; i++) m->b[i].n = 0;
        me->cls = C_BDIR0;
        rowb_clear(mb_x);
        s->run++;
        if (LV::recon && rec.on) rc_mb_inter(&rec, m), rc_mb_end(&rec);
        return;
      }
      s->run = 0;
      T.cl = (L && L->cbp) + (A && A->cbp);
      T.c4 = T.cl;
      T.rt = R ? R->cbp != 0 : kTriNone;
      m->mb = mbt_bit(MBN_BCBP, T, m->mb == MODB_CBP) ? MODB_CBP : MODB_NOCBP;
      T.cd = count_of(C_BDIR, L, A);
      T.cl = count_of(C_BFWD, L, A);
      T.c4 = T.cl + count_of(C_BFWD, AL, AR);
      T.rt = R ? R->cls == C_BFWD : kTriNone;
      if (mbt_bit(MBN_BDIRECT, T, m->t == MB_DIRECT)) m->t = MB_DIRECT;
      else if (mbt_bit(MBN_BFWD, T, m->t == MB_FWD)) m->t = MB_FWD;
      else {
        T.cl = count_of(C_BBWD, L, A);
        T.c4 = T.cl + count_of(C_BBWD, AL, AR);
        T.rt = R ? R->cls == C_BBWD : kTriNone;
        m->t = mbt_bit(MBN_BBWD, T, m->t == MB_BWD) ? MB_BWD : MB_BI;
      }
      cls3 = C3_B;
    }
    T.cd = 0;
    T.rt = 0;
    if (intra) {
      T.cl = (L && L->ac) + (A && A->ac);
      T.c4 = T.cl;
      T.intra = 1;
      m->ac = mbt_bit(MBN_AC, T, m->ac);
      if (LV::recon && rec.on) rc_mb_intra(&rec, m->ac), rc_intra_est(&rec, &est), est_err = est_nb = 0;
    }
    if (!intra && cx->interlaced && (v->type == PT_B ? m->t != MB_DIRECT : m->t == MB_INTER && m->mc != 1)) {
      T.cl = (L && L->fp) + (A && A->fp);
      T.c4 = T.cl;
      m->fp = mbt_bit(MBN_FIELD, T, m->fp);
      if (m->fp == 1) {
        m->nfs = v->type == PT_B ? 2 * (fwd_dir(m->t) + bwd_dir(m->t)) : 2;  // (a select per field)
        for (i = m->nfs - 1, k = 0; i >= 0; i--) {
          T.fsn = cap(i, kFsnCap);
          k |= mbt_bit(MBN_FSEL, T, (m->fs >> i) & 1) << i;
        }
        m->fs = k;
        T.fsn = 0;
      }
    }
    c->cat = CAT_MV;
    if (LV::recon && rec.on && !intra) {  // the vectors the template matches best
      if (v->type != PT_B) {
        if (m->t == MB_4MV || (m->mc != 1 && m->fp != 1))
          tmk[0] = rc_tm_search(&rec, rec.last, px, py, mvref[idx][0][0], mvref[idx][0][1], &tmx[0], &tmy[0]);
      } else if (m->t != MB_DIRECT && m->fp != 1) {
        const int cx0 = R ? mvref[idx][0][0] : 0, cy0 = R ? mvref[idx][0][1] : 0;
        const int tfx = (int)((S64)cx0 * s->trb / s->trd), tfy = (int)((S64)cy0 * s->trb / s->trd);
        if (fwd_dir(m->t))
          tmk[0] = rc_tm_search(&rec, rec.last, s->lmv[0][0][0], s->lmv[0][0][1], tfx, tfy, &tmx[0], &tmy[0]);
        if (bwd_dir(m->t))
          tmk[1] = rc_tm_search(&rec, rec.next, s->lmv[1][0][0], s->lmv[1][0][1], tfx - cx0, tfy - cy0, &tmx[1], &tmy[1]);
      }
    }
    if (!intra) {
      const int tok = R != 0, fm = cx->qpel ? kQpelMask : kHpelMask;
      int mx = 0, my = 0, mx2, my2;
      if (v->type != PT_B) {  // the vectors as the decoder gets them; their prediction
        if (m->t == MB_4MV)
          for (i = 0; i < MB_LUMA; i++) {
            if (i) pred_mv(s, idx, i, &px, &py, &sx, &sy);
            code_mv(m, &mvi, MVK_P4, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][i][0],
                    mvref[idx][i][1], &mx, &my, 0);
            mvcur[idx][i][0] = (S16)mx;
            mvcur[idx][i][1] = (S16)my;
          }
        else {
          if (m->mc == 1) mx = px, my = py;  // GMC: the prediction stands in for the warped vector
          else if (m->fp == 1) {
            code_mv(m, &mvi, MVK_PFIELD, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                    mvref[idx][0][1] / 2, &mx, &my);
            code_mv(m, &mvi, MVK_PFIELD, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                    mvref[idx][0][1] / 2, &mx2, &my2);
            mx = ((mx + mx2) >> 1) | ((mx + mx2) & 1);
            my = my + my2;
          } else
            code_mv(m, &mvi, MVK_P, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][0][0],
                    mvref[idx][0][1], &mx, &my, 0);
          for (i = 0; i < MB_LUMA; i++) { mvcur[idx][i][0] = (S16)mx; mvcur[idx][i][1] = (S16)my; }
        }
      } else if (m->t != MB_DIRECT) {  // B-VOP: predicted by the last vector in the row; the co-located one scaled as in
                          // direct mode
        const int n = m->fp == 1 ? 2 : 1, cx0 = R ? mvref[idx][0][0] : 0, cy0 = R ? mvref[idx][0][1] : 0;
        const int tfx = (int)((S64)cx0 * s->trb / s->trd), tfy = (int)((S64)cy0 * s->trb / s->trd);
        int dir;
        for (dir = 0; dir < 2; dir++) {
          const int code = dir ? v->bcode : v->fcode, tx = dir ? tfx - cx0 : tfx, tyv = dir ? tfy - cy0 : tfy;
          if (!(dir ? bwd_dir(m->t) : fwd_dir(m->t))) continue;
          for (i = 0; i < n; i++) {
            const int ppx = s->lmv[dir][i][0], ppy = n == 2 ? s->lmv[dir][i][1] / 2 : s->lmv[dir][i][1];
            code_mv(m, &mvi, MVK_BFWD + dir, code, L, A, R, me, &sumq, ppx, ppy, -1, -1, tok, tx, n == 2 ? tyv / 2 : tyv,
                    &mx, &my, n == 1 ? dir : -1);
            if (n == 1) s->lmv[dir][0][0] = s->lmv[dir][1][0] = mx, s->lmv[dir][0][1] = s->lmv[dir][1][1] = my;
            else s->lmv[dir][i][0] = mx, s->lmv[dir][i][1] = my * 2;
          }
        }
      } else
        code_mv(m, &mvi, MVK_BDIR, FCODE_MIN, L, A, R, me, &sumq, 0, 0, -1, -1, 0, 0, 0, &mx, &my);
      if (v->type != PT_B && m->mc != 1) mx = mvcur[idx][0][0], my = mvcur[idx][0][1];
      if ((v->type != PT_B && m->mc != 1) || (v->type == PT_B && m->t != MB_DIRECT))
        frac = ((mx & fm) != 0) + 2 * ((my & fm) != 0);
      if (LV::recon && rec.on) rc_mb_inter(&rec, m);  // (its prediction in the picture)
    }
    c->cat = CAT_CBP;
    int cie[MB_BLOCKS] = {0}, ctx[MB_BLOCKS] = {0}, cem[MB_BLOCKS] = {0};
    if (LV::recon && est.ok)
      for (i = 0; i < MB_BLOCKS; i++) {  // the estimate's AC coefficients of half a level or more
        int r, nn = 0;
        for (r = 1; r < BLK_COEFS; r++) nn += abs(rc_est_q4(&rec, &est, i, r)) >= kQ / 2;
        cie[i] = 1 + cap(nn, kCap);
      }
    if (LV::recon && rec.on && !intra && !(v->type == PT_B && m->mb == MODB_NOCBP))
      for (i = 0; i < MB_BLOCKS; i++) {  // the prediction's texture and edge miss (its edges on other macroblocks)
        int tx, em;
        rc_pred_feat(&rec, i, 0, 0, &tx, &em);
        ctx[i] = qlog1(tx);
        cem[i] = em < 0 ? 0 : qlog1(em);
      }
    cbp = v->type == PT_B && m->mb == MODB_NOCBP
              ? 0
              : code_cbp(cls3, L, A, R, mvi ? mvq(sumq) : kNone, frac, v->qscale,
                         (L ? L->cbp != 0 : kTriNone) == (A ? A->cbp != 0 : kTriNone) ? 0 : 1 + (R && R->cbp), ENC ? cbp_of(m) : 0,
                         cie, est_trust(), ctx, cem);
    c->cat = CAT_MB;
    if (v->type != PT_B || (m->t != MB_DIRECT && cbp)) {
      M_qpd_v Q;
      const int node = v->type == PT_B ? QPD_B : QPD_P;
      Q.c3 = cls3;
      Q.pdq = s->prev_dq;
      Q.qs = v->qscale;
      sel(f_qpd, Q);
      k = f_qpd.bit(node, ENC && m->dq != 0);
      if (!k) m->dq = 0;
      else if (v->type != PT_B) {  // which of -1, -2, 1, 2 (quant_tab), in a tree of two bits
        const int x = ENC ? dq_index(m->dq) : 0, b1 = f_qpd.bit(QPD_IDX, x >> 1);
        m->dq = quant_tab[b1 * 2 + f_qpd.bit(QPD_IDX_LO + b1, x & 1)];
      } else
        m->dq = f_qpd.bit(QPD_BSIGN, ENC && m->dq > 0) ? DQ_B : -DQ_B;
      s->prev_dq = k;
      v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
      if (LV::recon && rec.on) rc_set_qscale(&rec, v->qscale);
    }
    if (cx->interlaced && (intra || cbp)) {
      T.cl = (L && L->dt) + (A && A->dt);
      T.c4 = T.cl;
      T.intra = intra;
      m->dt = mbt_bit(MBN_DCT, T, m->dt);
      if (LV::recon) rec.interlaced_dct = m->dt;
      if (LV::recon && est.ok && m->dt == 1) rc_est_luma(&rec, &est, 1);  // (the estimate's luma in fields)
    }
    m->nmv = mvi;
    {
      for (i = 0; i < MB_BLOCKS; i++) {
        const int luma = i < MB_LUMA;  // (a luma block: i & 1 the right column, i & 2 the lower row)
        const int cls = (intra ? (m->ac == 1 ? CL_INTRA_AC : CL_INTRA) : v->type == PT_B ? CL_B : CL_INTER) + !luma;
        const BlkCtx* Lb = luma && (i & 1) ? &rowb[mb_x][i - 1] : !L ? 0 : &rowb[mb_x - 1][luma ? i + 1 : i];
        const BlkCtx* Ab = luma && (i & 2) ? &rowb[mb_x][i - 2] : !A ? 0 : &rowb[mb_x][luma ? i + 2 : i];
        code_block(s, &m->b[i], i, intra, intra && dcvlc, cbp_bit(cbp, i), cls, nz_ctx(me, L, A, i), v->qscale,
                   mvi ? mvq(sumq) : kMvcNone, R ? cap(R->nz[i], kCap) : kNone, Lb, Ab, &rowb[mb_x][i],
                   i == BLK_CR ? &rowb[mb_x][BLK_CB] : 0);
        me->nz[i] = (U8)cap(m->b[i].n, BLK_LAST);
        if (LV::recon && rec.on) rc_block(&rec, i, &m->b[i], intra, intra && dcvlc);
      }
    }
    if (LV::recon && rec.on) rc_mb_end(&rec);
    me->cbp = (U8)cbp;
    me->ac = intra && m->ac == 1;
    me->dt = m->dt == 1;
    me->fp = m->fp == 1;
    if (intra) me->cls = C_INTRA;
    else if (v->type != PT_B) me->cls = m->t == MB_4MV ? C_4MV : m->mc == 1 ? C_GMC : C_INTER;
    else me->cls = m->t == MB_DIRECT ? C_BDIR : m->t == MB_FWD ? C_BFWD : m->t == MB_BWD ? C_BBWD : C_BBI;
  }

  // One VOP (after its start code). W_VERIFY: parse it and write it again (into pb); W_ENCODE:
  // parse it and code the records; W_DECODE: decode the records and write the VOP (with its start
  // code). W_FAIL: it can't be (parsed, written: unsupported or damaged data), W_NOMEM: no memory.
  int vop_walk(Ctx* cx, int mode, GB* gb, PB* pb) {
    MB& m = mrec;
    VopHdr h;
    Vop vp, vw;
    VState s;
    int mb_x = 0, mb_y = 0, anchor, ret = 0, nb, tbits = 0, tn = 0, i;
    double mb_bits = 0;
    memset(&h, 0, sizeof(h));
    memset(&s, 0, sizeof(s));
    if (mode != W_DECODE && parse_vop_hdr(cx, gb, &h) < 0) return W_FAIL;
    if (mode != W_VERIFY) code_vop_hdr(cx, &h);
    if (mode == W_DECODE && c->err) return W_FAIL;
    if (LV::recon && mode != W_VERIFY && rc_vop_begin(&rec, cx, &h) < 0) return W_NOMEM;
    if (mode != W_ENCODE && put_vop_hdr(cx, pb, &h) < 0) return W_FAIL;
    if (h.coded) {
      if (!cx->have_vol || cx->unsupported || !cx->mb_num || cx->mb_num > MAX_MBS) return W_FAIL;
      if (!ctx_skip_room(cx, cx->mb_num) || !mb_room(cx->mb_num)) return W_NOMEM;
      vop_state(&vp, cx, &h);
      vw = s.v = vp;
      s.mb_w = cx->mb_w;
      s.scan = h.alt == 1 ? alternate_vertical_scan : zigzag_direct;
      anchor = h.type != PT_B;
      if (mode != W_DECODE && anchor) memset(cx->skip_cur, 0, cx->mb_num + 1);
      if (mode != W_VERIFY) {
        memset(mbcur, 0, cx->mb_num * sizeof(*mbcur));
        memset(mvcur, 0, cx->mb_num * sizeof(*mvcur));
        if (ref_mbs != cx->mb_num) ref_mbs = 0;
        s.trb = last.pb_time;
        s.trd = last.pp_time;
        if (s.trb <= 0 || s.trb >= s.trd) s.trb = 1, s.trd = 2;  // (inconsistent times: halfway)
      }
      for (;;) {
        const int idx = mb_y * cx->mb_w + mb_x;
        m.x = mb_x;
        m.y = mb_y;
        if (mode != W_DECODE) {
          const double b0 = pbits[PB_MV], b1 = pbits[PB_DC], b2 = pbits[PB_COEF];
          const int p0 = gb->pos;
          if (parse_mb(cx, gb, &vp, &m, idx) < 0 || (mode == W_VERIFY && !mb_codable(&m))) { ret = W_FAIL; break; }
          if (mode == W_ENCODE && c->stats) {
            c->orig[CAT_MV] += pbits[PB_MV] - b0;
            c->orig[CAT_DC] += pbits[PB_DC] - b1;
            c->orig[CAT_SIG] += pbits[PB_COEF] - b2;
            c->orig[CAT_MB] += gb->pos - p0 - (pbits[PB_MV] - b0) - (pbits[PB_DC] - b1) - (pbits[PB_COEF] - b2);
            mb_bits += gb->pos - p0;
          }
          if (anchor) cx->skip_cur[idx] = (U8)(m.sk == 1 && !vp.gmc);
        }
        if (mode != W_VERIFY) code_mb(cx, &s, &m, idx);
        if (mode == W_DECODE && (c->err || c->io->over > OVER_MAX)) { ret = W_FAIL; break; }  // damaged data
        if (mode != W_ENCODE && put_mb(cx, pb, &vw, &m) < 0) { ret = W_FAIL; break; }
        if (mode == W_DECODE && pb->err) { ret = W_FAIL; break; }
        if (++mb_x == cx->mb_w) { mb_x = 0; mb_y++; }
        if (mb_y >= cx->mb_h) break;
        {
          VpHdr k = VpHdr();  // (decompressing, what code_vp() would read of it for the compressor)
          int vpf = 0, nstuff = 0, next;
          if (mode != W_DECODE) {
            next = resync_ahead(cx, gb, &vp, &nstuff);
            vpf = next && !(idx + 1 < next && vp.type == PT_B && cx->skip_ref[idx + 1]);
          }
          if (mode != W_VERIFY) {
            c->cat = CAT_HDR;
            vpf = hdr_flag(F_VPFLAG, s.v.type - PT_I, 0, 0, vpf);
          }
          if (!vpf) continue;
          if (mode != W_DECODE) {
            if (nstuff && vp.type == PT_S) { ret = W_FAIL; break; }  // not MCBPC stuffing there
            k.st = nstuff;
            gb_skip(gb, nstuff * stuffing_len(vp.type));
            gb_skip(gb, 1);
            gb_align(gb);
            if (parse_vp(cx, gb, &vp, &k) < 0) { ret = W_FAIL; break; }
            vp.qscale = clipi(vp.qscale, QS_MIN, QS_MAX);
          }
          if (mode != W_VERIFY) {
            code_vp(cx, &s, &k, idx + 1);
            s.vp_start = k.mbnum;
            memset(s.lmv, 0, sizeof(s.lmv));
            if (LV::recon && rec.on && k.mbnum > 0 && k.mbnum < cx->mb_num) rc_vp_begin(&rec, &k);
          }
          if ((mode != W_ENCODE && put_vp(cx, pb, &vw, &k) < 0) || k.mbnum < 1 || k.mbnum >= cx->mb_num) {
            ret = W_FAIL;
            break;
          }
          mb_x = k.mbnum % cx->mb_w;
          mb_y = k.mbnum / cx->mb_w;
        }
      }
      if (mode != W_DECODE && anchor) memcpy(cx->skip_ref, cx->skip_cur, cx->mb_num + 1);
      if (LV::recon && mode != W_VERIFY) rc_vop_end(&rec);
      if (ret < 0) return W_FAIL;
      if (mode != W_VERIFY && anchor) {
        memcpy(mbref, mbcur, cx->mb_num * sizeof(*mbcur));
        memcpy(mvref, mvcur, cx->mb_num * sizeof(*mvcur));
        ref_mbs = cx->mb_num;
      }
    }
    // the rest: stuffing, normally
    if (mode != W_DECODE) {
      if (gb->pos > gb->size) return W_FAIL;
      nb = to_byte(gb->pos);
      tbits = (int)gb_get(gb, nb);
      tn = (gb->size - gb->pos) >> BYTE_SHIFT;
      if (tn > (int)TAIL_MAX) return W_FAIL;
      memcpy(tailbuf, gb->buf + (gb->pos >> BYTE_SHIFT), (size_t)tn);
    } else
      nb = to_byte(pb->n);
    if (mode != W_VERIFY && code_tail(nb, &tbits, &tn, tailbuf, TAIL_MAX) < 0) return W_FAIL;
    if (mode == W_DECODE && c->err) return W_FAIL;
    if (mode == W_ENCODE && c->stats) c->orig[CAT_HDR] += gb->size + SC_LEN * BYTE_BITS - mb_bits;
    if (mode != W_ENCODE) {
      pb_put(pb, (U32)tbits, nb);
      for (i = 0; i < tn; i++) pb_put(pb, tailbuf[i], BYTE_BITS);
    }
    return 0;
  }
};
