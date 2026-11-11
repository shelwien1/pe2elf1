// model_core.inc -- what the model's elements share (model.inc): the fields' numbers in the hdr
// family, what the stream had last, the values as variables, a magnitude's code.

constexpr int HOCC = 128;  // hdr: the places of a field in a frame that have a last value (more: the last of them)
constexpr int kVal = 62;   // a value as a variable (lv, pq): 1 + it, to 1 + kVal
// what raw bytes are (raw's code)
enum { RC_CHUNK, RC_BAD, RC_TAIL, RC_GAP };
// a chunk's kind (code_kind()): the end, a frame as records, bytes between frames, a frame kept as
// bytes
enum { CK_END, CK_FRAME, CK_RAW, CK_BAD, CK_N };
constexpr S64 kRawLenMax = (S64)1 << 40;  // a chunk's length as decoded (more: damaged data)

// A field's number in the hdr family (-1: not one of its), made as the program starts: the core
// frame's fields, the container's (NHFRAME + HX_*), then the XCH extension's (so the others keep
// the numbers they had before XCH had fields: the levels' contexts take parts of them)
static const int NHFRAME = 42;  // the core frame's hdr fields
static_assert(F_LFEscaleIndex + 1 == NHFRAME && F_XCH == NHFRAME && F_XXDCOEF + 1 == F_PMODE,
              "the hdr family's fields are the first of CORE_FIELDS, the extensions' after the core's");
struct FieldNums {
  S16 hdr[F_N];
};
static FieldNums field_nums(void) {
  FieldNums t;
  int h = 0;
  for (int fid = 0; fid < F_N; fid++) {
    if (fid == F_XCH) h += HX_N;
    t.hdr[fid] = field_kind[fid] == FK_H ? (S16)h++ : (S16)-1;
  }
  if (h > NHF) fatal("internal error: %d header fields", h);
  return t;
}
static const FieldNums fnums = field_nums();

// What the stream had last, as its frames are coded (zeroed pages: none)
struct Hist {
  S32 h1[NHF][HOCC], h2[NHF][HOCC];  // hdr's fields: the last two frames' values at each place (1 + them)
  // each channel's subbands in the subframe before (1 + the value, 0 none): their side
  // information, ABITS, first scale factor, HF VQ vector; the prediction of their scale factor the
  // audio made, its energy as a scale factor index; whether the scale factor before was the
  // prediction (hm: 0 none, 1 it was, 2 not); the LFE's last sample
  S32 pmode[NCH][NSUB], pvq[NCH][NSUB], tmode[NCH][NSUB], abits[NCH][NSUB], scale[NCH][NSUB], hfvq[NCH][NSUB];
  S32 jscale[NCH][NSUB];
  S32 pscale[NCH][NSUB], energy[NCH][NSUB];
  S32 lfe;
  // the last tail's bytes that aren't 0, at its end (tailn of them); the same of the bits before
  // the extension ([1]) and XXCH's other bits ([2-4]: model_hdr.inc's code_tail())
  U8 tail[5][MAX_FRAME];
  S32 tailn[5];
  S32 tailz;  // (1 + the last bits after the subframes' trailing 0 bits, 0 none)
  S32 tailsync;  // (the last aux tail's sync word, its place among those bytes: code_tail(); -1 none)
  // the last TC_K tails of each kind that weren't all 0 (as code_tail() has them, to TC_B bytes), the
  // last first: their bits (0 none) and bytes
  static constexpr int TC_K = 32, TC_B = 64;
  S32 tcn[5][TC_K];
  U8 tcb[5][TC_K][TC_B];
};

// a value as a variable: 0 none (v 0), else 1 + it, to 1 + c (v as Hist keeps it: 1 + the value)
static INLINE int vvar(S32 v, int c) { return v ? 1 + (v - 1 < c ? v - 1 : c) : 0; }
// a sign as a variable: 0 none or 0, 1 +, 2 -
static INLINE int svar(S64 v) { return v > 0 ? 1 : v < 0 ? 2 : 0; }
// a difference as a variable: -4..4 (clipped) as 0-8
static INLINE int dvar(int d) { return clip(d, -4, 4) + 4; }
// the bits a magnitude takes (0 for 0)
static INLINE int bit_len(U64 m) { return m ? ilog2(m) + 1 : 0; }
// a channel's class: 0 the first, 1 the next two, 2 the others
static INLINE int chan_class(int ch) { return ch == 0 ? 0 : ch < 3 ? 1 : 2; }

// A magnitude x of maxnb bits at most (maxnb 1-24), by the family f whose contexts are selected:
// its length (the bits it takes, 0 for 0) as a tree of decisions (nodes len0 + the bits so far:
// at most five of them), then its bits below the top one: the first LEN_TOP of them decisions too
// (nodes after the tree's, by the length and the bits before them), the rest in the family's
// mantissa plane (bitm(), as code() codes them: the first mt by the bits before them, the rest by
// place). Decompressing, -1 for a length past maxnb (damaged data). (ac3pack's.)
template <class F>
static INLINE S64 code_len(F& f, int len0, int maxnb, int mt, S64 x, int enc) {
  const int depth = U32_BITS - __builtin_clz((unsigned)maxnb);
  int nb = 0, t = 1;
  S64 m = 1;
  if (enc) nb = bit_len((U64)x);
  for (int d = depth - 1; d >= 0; d--) t = 2 * t + f.bit(len0 + t, enc && ((nb >> d) & 1));
  nb = t - (1 << depth);
  if (nb > maxnb) return -1;
  if (!nb) return 0;
  for (int p = 0; p < nb - 1; p++) {
    const int b = enc && ((x >> (nb - 2 - p)) & 1);
    if (p < LEN_TOP) {
      m = 2 * m + f.bit(len0 + LEN_NODES + nb * ((1 << LEN_TOP) - 1) + (int)m - 1, b);
    } else {
#ifdef DTSD_DUMP  // (as code() dumps its mantissa's bits: IDX/famtune.cpp replays them)
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
