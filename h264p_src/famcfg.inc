// famcfg.inc -- the configuration class of one family at one level: every
// constant tc_fam<f_DEC, C> reads, wrapped from the family's generated
// headers (h264p_levels.md section 7).
//
// Included at the end of each family's MOD/l<L>/<family>_p.inc, from the
// level's template of its families (IDX/l<L>/family.inc), which states the
// level's shape once for all of them -- after the family's _h.inc, with
//
//   #define FC_NAME  C9_skp   // the class to define
//   #define FC_PFX   H9_skp_  // the prefix of the generated names
//   #define FC_FAM   skp      // the family: its variables (H_skp_v), node counts
//   #define FC_NC    10       // counters: the first FC_NC of A-E, H-L
//   #define FC_NMIX  3        // mixers: 1 (by index m), or 3 (m, w, s) and the final one
//   #define FC_NAPM  2        // APMs: 0, or 2 (by s, refining A's; by f, the last step)
//   #include "../../famcfg.inc"
//
// and the template declaring the tables of that shape. Every clamp is the
// consumer's (IDX-FORMAT.md sec.5). In the shipping build (MOD/ in the
// Const form) the members are constant expressions, and tc_fam's code has
// them folded in; in the tuning build they are reads of the mapping
// objects opt.pl patches, initialized when the program starts in the order
// of the definitions below, each after what it reads.

#define FC_CAT2(a, b) a##b
#define FC_CAT(a, b) FC_CAT2(a, b)
#define FCX(n) FC_CAT(FC_PFX, n)              // H9_skp_rA
#define FCF(n) FC_CAT(FC_CAT(H_, FC_FAM), n)  // H_skp_v, H_skp_ND
#define FC_USE(f) | 1ull << offsetof(V, f) / sizeof(int)
#define FC_STR2(a) #a
#define FC_STR(a) FC_STR2(a)  // "skp"

// the counters: their tables' letters and their indices', in order
#if FC_NC == 10
#define FC_CNT(X) X(A, a) X(B, b) X(C, c) X(D, d) X(E, e) X(H, h) X(I, i) X(J, j) X(K, k) X(L, l)
#define FC_LASTC L
#elif FC_NC == 8
#define FC_CNT(X) X(A, a) X(B, b) X(C, c) X(D, d) X(E, e) X(H, h) X(I, i) X(J, j)
#define FC_LASTC J
#elif FC_NC == 6
#define FC_CNT(X) X(A, a) X(B, b) X(C, c) X(D, d) X(E, e) X(H, h)
#define FC_LASTC H
#elif FC_NC == 4
#define FC_CNT(X) X(A, a) X(B, b) X(C, c) X(D, d)
#define FC_LASTC D
#elif FC_NC == 2
#define FC_CNT(X) X(A, a) X(B, b)
#define FC_LASTC B
#else
#error "famcfg.inc: FC_NC is 2, 4, 6, 8 or 10"
#endif
#if FC_NMIX != 1 && FC_NMIX != 3
#error "famcfg.inc: FC_NMIX is 1 or 3"
#endif
#if FC_NAPM != 0 && FC_NAPM != 2
#error "famcfg.inc: FC_NAPM is 0 or 2"
#endif

struct FC_NAME {
  typedef FCF(_v) V;  // the variables, as the model fills them
  typedef FCX(T) T;   // the level's tables
  static const char* name() { return FC_STR(FC_FAM); }  // (its dump's, fam.inc)
  static INLINE void make(const V& D, tcx& X) { FCX(make)(D, X); }
  // the counters' type (the level's template of the family declares their
  // tables): cm_cnt2, or cm_cnt2s with their probabilities kept stretched
  typedef typename NoRef<decltype(((T*)nullptr)->FCX(A)[0])>::type Cnt;
  static void init(T* t) { t->FCX(Init)(); }
  static void quit(T* t) { t->FCX(Quit)(); }  // (the tuning build: its tables unmapped)
  static u64 bytes(const T* t) { return t->FCX(Size); }
  static const int ND = FCF(_ND), MND = FCF(_MND), NG = FCF(_NG);
  static const int NC = FC_NC, NMIX = FC_NMIX, NAPM = FC_NAPM;
  // the mixers' inputs: two per counter, the APM's refinement of A's (A's
  // own without APMs), the outside prediction, the bias and the hint's
  // counter, in lanes of 8
  static const int NIN = (2 * NC + 4 + 7) / 8 * 8;
  static const u64 Uses = 0 FCX(USES)(FC_USE);  // the variables the indices read
  static const int lim[NC], tg[NC];             // the counters' limits and targets
  static const int hc[NC];                      // a counter's index: 0 direct, else hashed into 2^h rows
  static const sz kc[NC];                       // its rows
  static const int rT, mwT, mt, rG, mwG;        // the mantissa's counters and tree, the signs'
  static const int lr, mb, lrm, mbm;            // the mixers' rates
  static const int hM, hT, hN;                  // mixer 1's, the mantissa's, its mixer's index
  static const sz kM, kT, kN, kG;               // their rows, and the signs'
  // (0 where the shape has not the part, which tc_fam then never reads)
  static const int hS;  // mixer 3's and APM 1's index
  static const sz kS;
  static const sz kW;   // mixer 2's rows (its index is direct)
  static const int hF;  // APM 2's index
  static const sz kF;
  static const int rS1, rS2, qs, qf, bw;  // the APMs' rates and widths, and the last one's share
  // the tables, in the order tc_fam takes them
  static void tables(T* t, tc_tabs<Cnt>& b) {
#define FC_TAB(U, l)                                                                                 \
  static_assert(SameType<decltype(+t->FCX(U)), Cnt*>::value, "a family's counters of one type"); \
  b.cnt[i++] = t->FCX(U);
    int i = 0;
    FC_CNT(FC_TAB)
#undef FC_TAB
    b.T = t->FCX(MT), b.G = t->FCX(G);
    b.W = t->FCX(W), b.WC = t->FCX(WC), b.WM = t->FCX(WM), b.WMC = t->FCX(WMC);
#if FC_NMIX > 1
    b.W2 = t->FCX(W2), b.W2C = t->FCX(W2C), b.W3 = t->FCX(W3), b.W3C = t->FCX(W3C);
    b.WF = t->FCX(WF), b.WFC = t->FCX(WFC);
#endif
#if FC_NAPM
    b.S = t->FCX(S), b.SC = t->FCX(SC), b.F = t->FCX(F), b.FC = t->FCX(FC);
#endif
  }
};

// a hashed index's bits: 0 where the Volume fits the 2^h rows (direct)
#define FC_HASH(v, h) ((v) > (1LL << tc_hbits(h)) ? tc_hbits(h) : 0)
#define set const int FC_NAME::
#define FC_LIM(U, l) tc_clamp(FCX(r##U), 1, CM_TMAX),
set lim[FC_NC] = {FC_CNT(FC_LIM)};
#undef FC_LIM
#define FC_TG(U, l) tc_clamp(FCX(mw##U) * 64, 0, CM_CONE / 4),
set tg[FC_NC] = {FC_CNT(FC_TG)};
#undef FC_TG
#define FC_HC(U, l) FC_HASH(FCX(l##_Volume), FCX(h##U)),
set hc[FC_NC] = {FC_CNT(FC_HC)};
#undef FC_HC
set rT = tc_clamp(FCX(rT), 1, CM_TMAX);
set mwT = tc_clamp(FCX(mwT) * 64, 0, CM_CONE / 4);
set mt = tc_clamp(FCX(mt), 0, TC_MTREE);
set rG = tc_clamp(FCX(rG), 1, CM_TMAX);
set mwG = tc_clamp(FCX(mwG) * 64, 0, CM_CONE / 4);
set lr = tc_clamp(FCX(lr), 1, 64);
set mb = tc_clamp(FCX(mb), 0, 255);
set lrm = tc_clamp(FCX(lrm), 1, 64);
set mbm = tc_clamp(FCX(mbm), 0, 255);
set hM = FC_HASH(FCX(m_Volume), FCX(hM));
set hT = FC_HASH(FCX(t_Volume), FCX(hT));
set hN = FC_HASH(FCX(n_Volume), FCX(hN));
#if FC_NMIX > 1 || FC_NAPM
set hS = FC_HASH(FCX(s_Volume), FCX(hS));
#else
set hS = 0;
#endif
#if FC_NAPM
set hF = FC_HASH(FCX(f_Volume), FCX(hF));
set rS1 = cm_apm_limit(tc_clamp(FCX(rS1), 1, 15));
set rS2 = cm_apm_limit(tc_clamp(FCX(rS2), 1, 15));
set qs = cm_apm_q(FCX(qs));
set qf = cm_apm_q(FCX(qf));
set bw = tc_clamp(FCX(bw), 0, 16);
#else
set hF = 0;
set rS1 = 0;
set rS2 = 0;
set qs = 0;
set qf = 0;
set bw = 0;
#endif
#undef set
#define set const sz FC_NAME::
#define FC_KC(U, l) tc_rows(FCX(l##_Volume), FCX(h##U)),
set kc[FC_NC] = {FC_CNT(FC_KC)};
#undef FC_KC
set kM = tc_rows(FCX(m_Volume), FCX(hM));
set kT = tc_rows(FCX(t_Volume), FCX(hT));
set kN = tc_rows(FCX(n_Volume), FCX(hN));
set kG = (sz)FCX(g_Volume);
#if FC_NMIX > 1 || FC_NAPM
set kS = tc_rows(FCX(s_Volume), FCX(hS));
#else
set kS = 0;
#endif
#if FC_NMIX > 1
set kW = (sz)FCX(w_Volume);
#else
set kW = 0;
#endif
#if FC_NAPM
set kF = tc_rows(FCX(f_Volume), FCX(hF));
#else
set kF = 0;
#endif
#undef set
#undef FC_HASH
// (the shipping build: the mixers' rows as wide as their inputs)
static_assert(Extent<decltype(FC_NAME::T::FCX(W))>::value == 0 ||
                  Extent<decltype(FC_NAME::T::FCX(W))>::value == FC_NAME::kM * FC_NAME::ND * FC_NAME::NIN,
              "famcfg.inc: the template's mixer rows are not as wide as the family's inputs");

// (the counters' vector path, fam.inc, loads and stores 8 bytes at a
// 6-byte counter, 2 past a table's last one: in the shipping build the next
// member's, which no decision writes meanwhile -- the counter tables come
// first and another member after them -- and in the tuning build the 8
// bytes each table is mapped longer, idx2inc.pl)
static_assert(Extent<decltype(FC_NAME::T::FCX(A))>::value == 0 ||
                  offsetof(FC_NAME::T, FCX(FC_LASTC)) + sizeof(FC_NAME::T::FCX(FC_LASTC)) + 2 <= sizeof(FC_NAME::T),
              "famcfg.inc: the last counter table is the last member of the family's tables");

#undef FC_LASTC
#undef FC_CNT
#undef FC_USE
#undef FCF
#undef FCX
#undef FC_CAT
#undef FC_CAT2
#undef FC_STR
#undef FC_STR2
#undef FC_NAME
#undef FC_PFX
#undef FC_FAM
#undef FC_NC
#undef FC_NMIX
#undef FC_NAPM
