// model_core.inc -- what the model's elements share (model.inc): the substreams' histories, the
// fields' numbers in the hdr and sid families, the partners of channels, the mantissas' steps and
// the estimates made of them; the state of the frame being coded.

// The histories, one per kind of substream: 0 the frames' first fields, before the substream is
// known (bsid, strmtyp, substreamid: of the frame before, whatever its substream), 1 AC-3, then
// E-AC-3 by strmtyp (0-2) and substreamid (0-7)
enum { H_GLOBAL, H_AC3, H_EAC3, NHIST = H_EAC3 + 3 * 8 };
constexpr int HOCC = 32;   // hdr: the places of a field in a frame that have a last value (more: the last of them)
constexpr int SOCC = 128;  // sid: in a block
constexpr int kCap = 15;
constexpr int kVal = 62;   // a field's value as a variable (lv, pq): 1 + it, to 1 + kVal
constexpr int kEst = 21;   // an estimate as a variable: 2 + its quarter levels on est_var()'s scale, to 2 + kEst
static INLINE int cap(int v, int c) { return v < c ? v : c; }
constexpr int SKIP_MAX = 512, ADDBSI_MAX = 64, MIXDATA_MAX = 40, BLKSTRT_MAX = 16;
// what raw bytes are (raw's code)
enum { RC_CHUNK, RC_BAD, RC_ADDBSI, RC_MIXDATA, RC_BLKSTRT, RC_SKIP, RC_TAIL, RC_N };
// a chunk's kind (code_kind()): the end, a frame as records, bytes between frames, a frame kept as
// bytes
enum { CK_END, CK_FRAME, CK_RAW, CK_BAD, CK_N };
constexpr i64 kRawLenMax = (i64)1 << 40;  // a chunk's length as decoded (more: damaged data)

// What a kind of substream had last, as its frames are coded: the values of its fields (1 + the
// value, 0 none: zeroed pages are none), its last block's exponents, bit allocation and mantissas,
// its skip fields, and its raw bytes
struct Hist {
  S32 h1[NHF][HOCC], h2[NHF][HOCC];  // hdr's fields: the last two frames' values at each place
  S32 s1[NSF][SOCC];                 // sid's: the last block's
  S8 exps[NCH][NBIN];                // the last block's channels (has: 1 exponents, 2 and mantissas)
  U8 bap[NCH][NBIN];
  S16 q[NCH][NBIN];
  U8 has[NCH];
  S16 strt[NCH], end[NCH];
  U8 skip[NBLK][SKIP_MAX];           // the last skip field of each block (skipn its bytes)
  S16 skipn[NBLK];
  U8 addbsi[ADDBSI_MAX], mixdata[MIXDATA_MAX], blkstrt[BLKSTRT_MAX];
  S16 addbsin, mixdatan, blkstrtn;
  U8 tail[MAX_FRAME];                // the last tail's bytes that aren't 0, at its end (tailn of them)
  S32 tailn;
};

// a field's number in the hdr and sid families (-1: neither's), made as the program starts
struct FieldNums {
  S16 hdr[F_N], sid[F_N];
};
static FieldNums field_nums(void) {
  FieldNums t;
  int h = 0, s = 0;
  for (int fid = 0; fid < F_N; fid++) {
    const int k = field_kind[fid];
    t.hdr[fid] = t.sid[fid] = -1;
    if (k == K_H || k == K_A || k == K_T) t.hdr[fid] = (S16)h++;
    else if (k == K_B || fid == F_absexp || fid == F_gainrng || fid == F_gaqmod) t.sid[fid] = (S16)s++;
  }
  if (h + HX_N > NHF || s > NSF) fatal("internal error: %d header and %d side fields", h + HX_N, s);
  return t;
}
static const FieldNums fnums = field_nums();
static int hdr_field_count(void) {
  int n = 0;
  for (int fid = 0; fid < F_N; fid++) n += fnums.hdr[fid] >= 0;
  return n;
}
static const int NHFRAME = hdr_field_count();  // the container's fields come after the frame's (HX_*)

// The channel a channel is most like, whose exponents and mantissas in the block come before its
// own (-1 none): the first full bandwidth channel for the others, but a right surround's, which is
// its left's (acmod 6, 7); none for the first, the coupling channel and the LFE
static INLINE int partner(int acmod, int nfch, int ch) {
  if (ch < 2 || ch > nfch) return -1;
  if (acmod >= 6 && ch == nfch) return ch - 1;
  return 1;
}

// a mantissa's quantiser step, by bap, in 2^-15 of the mantissa's range (1.0 = 32768): bap 1-5
// the symmetric quantisers of 3, 5, 7, 11, 15 levels, 6-15 two's complement of 5-16 bits
struct MantSteps {
  U32 s[16];
};
static MantSteps mant_steps(void) {
  static const int levels[6] = {1, 3, 5, 7, 11, 15};
  MantSteps t;
  t.s[0] = 1u << 15;
  for (int b = 1; b < 16; b++) t.s[b] = b < 6 ? (U32)((2u << 15) / levels[b]) : (1u << 16) >> quant_bits[b];
  return t;
}
static const MantSteps msteps = mant_steps();
// a coefficient's magnitude as its mantissa (bap b, level q) gives it, at its exponent: in 2^-15
// of the mantissa's range (the level's, or a quarter step for 0)
static INLINE U32 mant_amp(int b, int q) { return q ? (U32)(q < 0 ? -q : q) * msteps.s[b] : msteps.s[b] / 4; }
// quarter levels x as an estimate: 2 + x for 0-7, then two buckets a doubling (by the bit below
// the top one), to 2 + kEst
static INLINE int est_var(U64 x) {
  if (x < 8) return 2 + (int)x;
  const int t = ilog2(x);
  return 2 + cap(2 * t + (int)((x >> (t - 1)) & 1) + 2, kEst);
}
// that magnitude (at exponent e) in quarter levels of the quantiser of bap b at exponent ec, as an
// estimate
static INLINE int mant_est(U32 amp, int e, int b, int ec) {
  U64 num = (U64)amp * 4, den = msteps.s[b];
  const int d = ec - e;
  if (d >= 0) num <<= d;
  else den <<= -d;
  return est_var(num / den);
}

// a value as a variable: 0 none (v 0), else 1 + it, to 1 + c (v as Hist keeps it: 1 + the value)
static INLINE int vvar(S32 v, int c) { return v ? 1 + (v - 1 < c ? v - 1 : c) : 0; }
// a sign as a variable: 0 none or 0, 1 +, 2 -
static INLINE int svar(int v) { return v > 0 ? 1 : v < 0 ? 2 : 0; }

// A magnitude x of maxnb bits at most (maxnb 1-16), by the family f whose contexts are selected:
// its length (the bits it takes, 0 for 0) as a tree of decisions (nodes len0 + the bits so far:
// at most five of them), then its bits below the top one: the first LEN_TOP of them decisions too
// (nodes after the tree's, by the length and the bits before them), the rest in the family's
// mantissa plane (bitm(), as code() codes them: the first mt by the bits before them, the rest by
// place). Decompressing, -1 for a length past maxnb (damaged data).
template <class F>
static INLINE int code_len(F& f, int len0, int maxnb, int mt, int x, int enc) {
  const int depth = U32_BITS - __builtin_clz((unsigned)maxnb);
  int nb = 0, t = 1, m = 1;
  if (enc)
    for (unsigned u = (unsigned)x; u; u >>= 1) nb++;
  for (int d = depth - 1; d >= 0; d--) t = 2 * t + f.bit(len0 + t, enc && ((nb >> d) & 1));
  nb = t - (1 << depth);
  if (nb > maxnb) return -1;
  if (!nb) return 0;
  for (int p = 0; p < nb - 1; p++) {
    const int b = enc && ((x >> (nb - 2 - p)) & 1);
    if (p < LEN_TOP) {
      m = 2 * m + f.bit(len0 + LEN_NODES + nb * ((1 << LEN_TOP) - 1) + m - 1, b);
    } else {
#ifdef AC3PACK_DUMP  // (as code() dumps its mantissa's bits: IDX/famtune.cpp replays them)
      if (enc) f.dump_.rec('m', nb, p, b, f.hs, f.prp, f.prpa != nullptr, (i64)m);
#endif
      m = 2 * m + f.bitm(tc_node_man(nb, p, (u64)m, mt), b);
    }
  }
  return m;
}

// the tables of a level's family, mapped as a run starts (zeroed pages)
template <class T>
static T* tc_table() {
  return (T*)tc_map(sizeof(T));
}
// a family F of the level, in an element of the model: its coder (f_F) and its tables (t_F)
#define M_FAM(F)                       \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;
