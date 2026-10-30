// modelbase.inc -- what the model (model.inc) is made of: the kinds and classes its variables
// take, the state it keeps between records (Last: the values before; BlkCtx and MBCtx: the blocks
// and macroblocks around; VState: the VOP's), the caps on its variables, and its small functions.

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
