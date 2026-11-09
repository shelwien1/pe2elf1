// syntax.inc -- the DTS core frame (ETSI TS 102 114 V1.6.1, clause 5) as a record, and one walk of
// its syntax that reads it from the bits (BitIn), writes it as bits (BitOut), or at record level
// hands each element to an IO that codes it (the model, model.inc): dts2tsv's walk of the core,
// with what its IO is told of where it is. The Huffman code books are dtssplit's (huff.inc).

enum { SYNC_CORE = 0x7FFE8001, SYNC_XCH = 0x5A5A5A5A };
constexpr int MAX_FRAME = 16384;  // a core frame's bytes, at most (FSIZE: 14 bits)
constexpr int PAD = 16;           // zero bytes after a buffer the bit reader may read
enum { NCH = 8, NSUB = 32, NSF = 16, NSSF = 4, NSMP = NSSF * 8 };

static INLINE U32 be32(pcU8 p) { return (U32)p[0] << 24 | (U32)p[1] << 16 | (U32)p[2] << 8 | p[3]; }
static INLINE U64 be64(pcU8 p) { return (U64)be32(p) << 32 | be32(p + 4); }

// --- the Huffman code books (Annex D.5) ----------------------------------------------------------

struct HuffEntry {
  S16 level;
  U8 len;
  U32 code;
};
#include "huff.inc"

// a code book: decoded by its left-aligned codes (prefix-free and complete, so their ranges tile
// the 32-bit space), coded by level
struct HuffBook {
  int n, minlev, nlev;
  U32 lcode[129];
  S16 lev[129];
  U8 len[129];
  U32 ecode[129];  // by level - minlev
  U8 elen[129];
  void init(const HuffEntry* e, int cnt) {
    n = cnt;
    minlev = 1 << 30;
    int maxlev = -minlev;
    for (int i = 0; i < n; i++) {
      if (e[i].level < minlev) minlev = e[i].level;
      if (e[i].level > maxlev) maxlev = e[i].level;
    }
    nlev = maxlev - minlev + 1;
    memset(elen, 0, sizeof elen);
    for (int i = 0; i < n; i++) {  // (sorted by left-aligned code: an insertion sort of <= 129)
      const U32 lc = e[i].code << (32 - e[i].len);
      int j = i;
      while (j > 0 && lcode[j - 1] > lc) {
        lcode[j] = lcode[j - 1];
        lev[j] = lev[j - 1];
        len[j] = len[j - 1];
        j--;
      }
      lcode[j] = lc;
      lev[j] = e[i].level;
      len[j] = e[i].len;
      ecode[e[i].level - minlev] = e[i].code;
      elen[e[i].level - minlev] = e[i].len;
    }
  }
  // the level whose code starts the 32 bits w, and its length
  int decode(U32 w, int& l) const {
    int lo = 0, hi = n - 1;  // the last entry whose code is <= w
    while (lo < hi) {
      const int m = (lo + hi + 1) >> 1;
      if (lcode[m] <= w)
        lo = m;
      else
        hi = m - 1;
    }
    l = len[lo];
    return lev[lo];
  }
  bool has(S64 v) const { return v >= minlev && v < minlev + nlev && elen[v - minlev]; }
  int bits(S64 v) const { return elen[v - minlev]; }
};

#define HB(x) HB_##x, (int)(sizeof HB_##x / sizeof HB_##x[0])
static HuffBook QB[10][7];  // quantization indexes: by ABITS-1, then SEL
static HuffBook BA[5];      // ABITS (BHUFF 0-4)
static HuffBook SB[5];      // scale factors (SHUFF 0-4)
static HuffBook TB[4];      // TMODE (THUFF)
static const int QB_GROUP[10] = {1, 3, 3, 3, 3, 7, 7, 7, 7, 7};  // Huffman books per ABITS
static const int SEL_BITS[10] = {1, 2, 2, 2, 2, 3, 3, 3, 3, 3};
static const int BLOCK_LEVELS[7] = {3, 5, 7, 9, 13, 17, 25};  // block codes, ABITS 1-7
static const int BLOCK_BITS[7] = {7, 10, 12, 13, 15, 17, 19};  // a code of 4 samples

static void huff_init(void) {
  QB[0][0].init(HB(A3));
  static const HuffEntry* const q5[] = {HB_A5, HB_B5, HB_C5}, *const q7[] = {HB_A7, HB_B7, HB_C7},
                                   *const q9[] = {HB_A9, HB_B9, HB_C9}, *const q13[] = {HB_A13, HB_B13, HB_C13};
  static const HuffEntry* const q17[] = {HB_A17, HB_B17, HB_C17, HB_D17, HB_E17, HB_F17, HB_G17};
  static const HuffEntry* const q25[] = {HB_A25, HB_B25, HB_C25, HB_D25, HB_E25, HB_F25, HB_G25};
  static const HuffEntry* const q33[] = {HB_A33, HB_B33, HB_C33, HB_D33, HB_E33, HB_F33, HB_G33};
  static const HuffEntry* const q65[] = {HB_A65, HB_B65, HB_C65, HB_D65, HB_E65, HB_F65, HB_G65};
  static const HuffEntry* const q129[] = {HB_A129, HB_B129, HB_C129, HB_D129, HB_E129, HB_F129, HB_G129};
  static const HuffEntry* const* const q[10] = {0, q5, q7, q9, q13, q17, q25, q33, q65, q129};
  static const int lv[10] = {3, 5, 7, 9, 13, 17, 25, 33, 65, 129};
  for (int a = 1; a < 10; a++)
    for (int s = 0; s < QB_GROUP[a]; s++) QB[a][s].init(q[a][s], lv[a]);
  BA[0].init(HB(A12));
  BA[1].init(HB(B12));
  BA[2].init(HB(C12));
  BA[3].init(HB(D12));
  BA[4].init(HB(E12));
  SB[0].init(HB(SA129));
  SB[1].init(HB(SB129));
  SB[2].init(HB(SC129));
  SB[3].init(HB(SD129));
  SB[4].init(HB(SE129));
  TB[0].init(HB(A4));
  TB[1].init(HB(B4));
  TB[2].init(HB(C4));
  TB[3].init(HB(D4));
}

// --- the elements -----------------------------------------------------------------------------------

// The core frame's elements by their names in TS 102 114, each with its kind: H a field of the
// frame's or a subframe's header (the hdr family), B one of a channel's subbands (the side
// information, ABITS, the scale factors), Q a subband sample's quantization index, T the bits
// after the subframes (tail) or before the XCH extension (xgap). The XCH extension's fields are
// X...: its being there (XCH) and the whole 4-byte words between the subframes' next 4-byte
// boundary and its sync word (XGAP) only the record's (FFmpeg finds it by the sync word); its
// frame size, AMODE, 3 bits after it, the coding header's CRC and a subframe's side information's
// (XSICRC); its channel's elements are the core channels'.
#define CORE_FIELDS(X)                                                                                         \
  X(FTYPE, H) X(SHORT, H) X(CPF, H) X(NBLKS, H) X(FSIZE, H) X(AMODE, H) X(SFREQ, H) X(RATE, H) X(FixedBit, H) \
  X(DYNF, H) X(TIMEF, H) X(AUXF, H) X(HDCD, H) X(EXT_AUDIO_ID, H) X(EXT_AUDIO, H) X(ASPF, H) X(LFF, H)        \
  X(HFLAG, H) X(HCRC, H) X(FILTS, H) X(VERNUM, H) X(CHIST, H) X(PCMR, H) X(SUMF, H) X(SUMS, H) X(DIALNORM, H) \
  X(SUBFS, H) X(PCHS, H) X(SUBS, H) X(VQSUB, H) X(JOINX, H) X(THUFF, H) X(SHUFF, H) X(BHUFF, H) X(SEL, H)    \
  X(ADJ, H) X(AHCRC, H) X(SSC, H) X(PSC, H) X(RANGE, H) X(SICRC, H) X(LFEscaleIndex, H) X(XCH, H)           \
  X(XGAP, H) X(XFSIZE, H) X(XAMODE, H) X(XRES, H) X(XAHCRC, H) X(XSICRC, H)                                    \
  X(PMODE, B) X(PVQ, B) X(ABITS, B) X(TMODE, B) X(SCALES, B) X(JOIN_SHUFF, B) X(JOIN_SCALES, B) X(HFVQ, B)    \
  X(LFE, B) X(q, Q) X(tail, T) X(xgap, T)
enum { FK_H, FK_B, FK_Q, FK_T };
#define CF_ID(n, k) F_##n,
#define CF_NAME(n, k) #n,
#define CF_KIND(n, k) FK_##k,
enum { CORE_FIELDS(CF_ID) F_N };
static const char* const field_name[F_N] = {CORE_FIELDS(CF_NAME)};
static const U8 field_kind[F_N] = {CORE_FIELDS(CF_KIND)};

// --- the record -------------------------------------------------------------------------------------

struct CoreSub {  // a subframe
  int ssc, psc, nssc;
  int pmode[NCH][NSUB], pvq[NCH][NSUB], abits[NCH][NSUB], tmode[NCH][NSUB], scale[NCH][NSUB][2];
  int jsel[NCH], jscale[NCH][NSUB];
  int range, sicrc, xsicrc;
  int hfvq[NCH][NSUB];
  int lfe[16], lfescale;
  S32 q[NCH][NSUB][NSMP];  // the quantization indexes, by subsubframe and sample
};

// A core frame: its elements as sent, and what follows from them. Plain data.
struct CoreRec {
  int ftype, deficit, cpf, nblks, fsize, amode, sfreq, rate, fixedbit, dynf, timef, auxf, hdcd, ext_id, ext, aspf, lff,
      hflag, hcrc, filts, vernum, chist, pcmr, sumf, sums, dialnorm;
  int subfs, pchs, subs[NCH], vqsub[NCH], joinx[NCH], thuff[NCH], shuff[NCH], bhuff[NCH], sel[NCH][10], adj[NCH][10],
      ahcrc;
  int nsubfs, npchs, nsubs[NCH], nvqsub[NCH];
  int nblk;  // the subband samples of its subframes
  CoreSub sub[NSF];
  // the XCH extension (6.1's back centre, channel npchs; nxch 1 where there): whether there, the
  // whole words before its sync word after the subframes' 4-byte boundary (XGAP), the bits from
  // the subframes' end to it (and their bytes), its header, its coding header's CRC; the walk's
  // pass (0 the core's channels, 1 XCH's)
  int xch, nxch, xgapw, xgap, xfsize, xamode, xres, xahcrc, xpass;
  U8 xgapb[MAX_FRAME];
  // the channels of the walk's pass: ch0() to ch1() - 1
  int ch0() const { return xpass ? npchs : 0; }
  int ch1() const { return xpass ? npchs + nxch : npchs; }
  int tailbits;  // the bits after the subframes (after XCH), and their bytes (the last one's unused bits 0)
  U8 tail[MAX_FRAME];
};

// the samples of a subsubframe of a subband: how they are coded (0 none, 1 Huffman, 2 plain, 3
// block codes)
static INLINE int sample_coding(const CoreRec& c, int ch, int a) {
  if (!a) return 0;
  if (a <= 10) {
    if (c.sel[ch][a - 1] < QB_GROUP[a - 1]) return 1;
    if (a <= 7) return 3;
  }
  return 2;
}

// --- the bits ---------------------------------------------------------------------------------------

static const char CUT[] = "a frame cut short";

// The walk's IO (BitIn, BitOut, the model): each element through one of
//   u(fid, n, v)      n bits, unsigned (n 0-32)          s(fid, n, v)    n bits, two's complement
//   huff(fid, b, v)   a Huffman code of book b           raw(fid, n, p)  n bits as they are
//   val(fid, lo, hi, v)  a value of the record that isn't an element of the bits as such
//   fixed(w, n, why)  a word the syntax fixes (a sync word): BitIn checks it
// and told where the walk is: frame_start(), subframe(c, sf) (its SSC and PSC known), at(ch, b, j)
// (the channel, the subband and the sample or scale factor of the next element), subframe_end().
// pos counts the bits the elements take, at either level. BITS: the IO has the bits; READ: it
// fills the record.
struct BitIn {
  enum { BITS = 1, READ = 1 };
  const U8* d;
  U64 pos, end;
  const char* err;
  int no_xch;  // the XCH extension not parsed (the bits after the subframes all a tail)
  void init(const U8* p, U64 nbits) {
    d = p;
    pos = 0;
    end = nbits;
    err = 0;
    no_xch = 0;
  }
  // The XCH extension of the core frame c (bits 0 to fend, its subframes' to pos), where c's header
  // says there is one: its sync word searched for from the frame's end at 4-byte places, as
  // FFmpeg searches (the XCH frame size the distance to the frame's end, or one more; 96 bytes at
  // least; AMODE 1, then 3 bits of 0), at or after pos: c.xch, and c.xgapw the whole words before
  // it after pos's 4-byte boundary
  void find_xch(CoreRec& c, U64 fend) {
    c.xch = c.xgapw = 0;
    if (no_xch || !c.ext || c.ext_id != 0 || c.npchs >= NCH) return;
    const S64 fs = (S64)(fend >> 3);
    for (S64 k = fs / 4 - 2; k >= 0 && (U64)k * 32 >= pos; k--) {
      if (be32(d + 4 * k) != SYNC_XCH) continue;
      const U32 w2 = be32(d + 4 * k + 4);
      const S64 size = (S64)(w2 >> 22) + 1, dist = fs - 4 * k;
      if (size >= 96 && (size == dist || size - 1 == dist) && ((w2 >> 15) & 0x7f) == 0x08) {
        c.xch = 1;
        c.xgapw = (int)(k - (S64)((pos + 31) >> 5));
        return;
      }
    }
  }
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  bool get(int n, U32& v) {
    if (pos + n > end) return fail(CUT);
    const U64 w = be64(d + (pos >> 3)) << (pos & 7);
    v = n ? (U32)(w >> (64 - n)) : 0;
    pos += n;
    return true;
  }
  bool u(int, int n, S64& v) {
    U32 x;
    if (!get(n, x)) return false;
    v = x;
    return true;
  }
  bool s(int, int n, S64& v) {
    U32 x;
    if (!get(n, x)) return false;
    v = n ? (S64)((S32)(x << (32 - n)) >> (32 - n)) : 0;
    return true;
  }
  bool huff(int, const HuffBook& b, S64& v) {
    if (pos >= end) return fail(CUT);
    int l;
    const int x = b.decode((U32)((be64(d + (pos >> 3)) << (pos & 7)) >> 32), l);
    if (pos + l > end) return fail(CUT);
    pos += l;
    v = x;
    return true;
  }
  bool raw(int, int n, U8* b) {
    if (pos + n > end) return fail(CUT);
    for (int i = 0; i < n; i += 8) {
      const int k = n - i < 8 ? n - i : 8;
      U32 x = 0;
      get(k, x);
      b[i >> 3] = (U8)(x << (8 - k));
    }
    return true;
  }
  bool val(int, S64, S64, S64&) { return true; }
  bool fixed(U32 w, int n, const char* why) {
    U32 x;
    if (!get(n, x)) return false;
    return x == w || fail(why);
  }
  void frame_start() {}
  void subframe(const CoreRec&, int) {}
  void at(int, int, int) {}
  void subframe_end(const CoreRec&, int) {}
};

struct BitOut {
  enum { BITS = 1, READ = 0 };
  U8* v;
  int cap;  // v's bytes
  U64 acc, pos;
  int nacc, n;  // n: the bytes written
  const char* err;
  void init(U8* p, int bytes) {
    v = p;
    cap = bytes;
    acc = 0;
    nacc = 0;
    n = 0;
    pos = 0;
    err = 0;
  }
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  void put(U32 x, int k) {  // k 0-32
    if (!k) return;
    if (k < 32) x &= (1u << k) - 1;
    acc = acc << k | x;
    nacc += k;
    pos += k;
    while (nacc >= 8) {
      nacc -= 8;
      if (n < cap) v[n] = (U8)(acc >> nacc);
      n++;
    }
    acc &= (1ULL << nacc) - 1;
  }
  bool u(int, int k, S64& x) {
    if (x < 0 || (U64)x >> k) return fail("a value out of range");
    put((U32)x, k);
    return true;
  }
  bool s(int, int k, S64& x) {
    if (x < -(1LL << (k - 1)) || x >= 1LL << (k - 1)) return fail("a value out of range");
    put((U32)x, k);
    return true;
  }
  bool huff(int, const HuffBook& b, S64& x) {
    if (!b.has(x)) return fail("a value out of range");
    put(b.ecode[x - b.minlev], b.elen[x - b.minlev]);
    return true;
  }
  bool raw(int, int k, U8* b) {
    for (int i = 0; i < k; i += 8) {
      const int m = k - i < 8 ? k - i : 8;
      put(b[i >> 3] >> (8 - m), m);
    }
    return true;
  }
  bool val(int, S64, S64, S64&) { return true; }
  bool fixed(U32 w, int k, const char*) {
    put(w, k);
    return true;
  }
  void frame_start() {}
  void subframe(const CoreRec&, int) {}
  void at(int, int, int) {}
  void subframe_end(const CoreRec&, int) {}
};

// --- the walk ---------------------------------------------------------------------------------------

// an element through the IO: its value from the record, back into the record (x evaluated once)
#define WU(f, n, x)                        \
  do {                                     \
    auto& r_ = (x);                        \
    S64 v_ = r_;                           \
    if (!io.u(F_##f, n, v_)) return false; \
    r_ = (int)v_;                          \
  } while (0)
#define WS(f, n, x)                        \
  do {                                     \
    auto& r_ = (x);                        \
    S64 v_ = r_;                           \
    if (!io.s(F_##f, n, v_)) return false; \
    r_ = (int)v_;                          \
  } while (0)
#define WH(f, b, x)                           \
  do {                                        \
    auto& r_ = (x);                           \
    S64 v_ = r_;                              \
    if (!io.huff(F_##f, b, v_)) return false; \
    r_ = (int)v_;                             \
  } while (0)

// A walk of one core frame through its IO: reads its elements from the bits into the record
// (BitIn), or writes them (BitOut), or at record level codes or decodes the record's values (the
// model): one walk, so that the directions can't disagree.
template <class IO>
struct Walk {
  IO& io;
  const char* err;
  explicit Walk(IO& i) : io(i), err(0) {}
  bool fail(const char* why) {
    if (!err) err = why;
    return false;
  }
  const char* why() const { return io.err ? io.err : err ? err : "the frame"; }

  bool core(CoreRec& c) {
    const U64 p0 = io.pos;
    c.xpass = 0;
    if (IO::READ) c.xch = c.nxch = c.xgapw = c.xgap = 0;
    io.frame_start();
    if (!io.fixed(SYNC_CORE, 32, "no core sync word")) return false;
    WU(FTYPE, 1, c.ftype);
    WU(SHORT, 5, c.deficit);
    WU(CPF, 1, c.cpf);
    WU(NBLKS, 7, c.nblks);
    WU(FSIZE, 14, c.fsize);
    WU(AMODE, 6, c.amode);
    WU(SFREQ, 4, c.sfreq);
    WU(RATE, 5, c.rate);
    WU(FixedBit, 1, c.fixedbit);
    WU(DYNF, 1, c.dynf);
    WU(TIMEF, 1, c.timef);
    WU(AUXF, 1, c.auxf);
    WU(HDCD, 1, c.hdcd);
    WU(EXT_AUDIO_ID, 3, c.ext_id);
    WU(EXT_AUDIO, 1, c.ext);
    WU(ASPF, 1, c.aspf);
    WU(LFF, 2, c.lff);
    WU(HFLAG, 1, c.hflag);
    if (c.cpf) WU(HCRC, 16, c.hcrc);
    WU(FILTS, 1, c.filts);
    WU(VERNUM, 4, c.vernum);
    WU(CHIST, 2, c.chist);
    WU(PCMR, 3, c.pcmr);
    WU(SUMF, 1, c.sumf);
    WU(SUMS, 1, c.sums);
    WU(DIALNORM, 4, c.dialnorm);
    if (c.fsize < 95) return fail("a core frame size out of range");
    if (c.lff == 3) return fail("an LFE flag out of range");
    const U64 end = p0 + (U64)(c.fsize + 1) * 8;
    // the primary audio coding header
    WU(SUBFS, 4, c.subfs);
    c.nsubfs = c.subfs + 1;
    WU(PCHS, 3, c.pchs);
    c.npchs = c.pchs + 1;
    if (!coding_header(c, 0, c.npchs)) return false;
    if (c.cpf) WU(AHCRC, 16, c.ahcrc);
    c.nblk = 0;  // subband samples so far
    for (int sf = 0; sf < c.nsubfs; sf++)
      if (!subframe(c, sf)) return false;
    if (io.pos > end) return fail("subframes past the end of the frame");
    // the XCH extension (6.1's back centre): found in the bits where the header says there may be
    // one, else as the record has it; its channel's coding header, its subframes, as the core's
    if (IO::BITS && IO::READ) find_xch(io, c, end);
    {
      S64 v = c.xch;
      if (!io.val(F_XCH, 0, c.ext && c.ext_id == 0 && c.npchs < NCH, v)) return false;
      c.xch = (int)v;
    }
    if (c.xch) {
      // (its sync word at a 4-byte boundary of the frame: the subframes' next, or XGAP words after)
      const int pad = (int)((32 - ((io.pos - p0) & 31)) & 31);
      const S64 room = (S64)(end - io.pos) - pad;
      S64 v = c.xgapw;
      if (!io.val(F_XGAP, 0, room > 0 ? room / 32 : 0, v)) return false;
      c.xgapw = (int)v;
      c.xgap = pad + 32 * c.xgapw;
      if (!io.raw(F_xgap, c.xgap, c.xgapb)) return false;
      if (!io.fixed(SYNC_XCH, 32, "no XCH sync word")) return false;
      WU(XFSIZE, 10, c.xfsize);
      WU(XAMODE, 4, c.xamode);
      WU(XRES, 3, c.xres);
      c.nxch = 1;
      if (!coding_header(c, c.npchs, c.npchs + 1)) return false;
      if (c.cpf) WU(XAHCRC, 16, c.xahcrc);
      c.xpass = 1;
      c.nblk = 0;
      for (int sf = 0; sf < c.nsubfs; sf++)
        if (!subframe(c, sf)) return false;
      c.xpass = 0;
      if (io.pos > end) return fail("XCH past the end of the frame");
    }
    c.tailbits = (int)(end - io.pos);
    if (!io.raw(F_tail, c.tailbits, c.tail)) return false;
    return true;
  }
  // (BitIn's search, nothing for the other IOs)
  static void find_xch(BitIn& i, CoreRec& c, U64 end) { i.find_xch(c, end); }
  template <class I>
  static void find_xch(I&, CoreRec&, U64) {}

  // the coding header's fields of channels ch0 to ch1 - 1 (the core's; XCH's)
  bool coding_header(CoreRec& c, int ch0, int ch1) {
    for (int ch = ch0; ch < ch1; ch++) {
      WU(SUBS, 5, c.subs[ch]);
      if ((c.nsubs[ch] = c.subs[ch] + 2) > NSUB) return fail("a subband count out of range");
    }
    for (int ch = ch0; ch < ch1; ch++) {
      WU(VQSUB, 5, c.vqsub[ch]);
      c.nvqsub[ch] = c.vqsub[ch] + 1;
    }
    for (int ch = ch0; ch < ch1; ch++) {
      WU(JOINX, 3, c.joinx[ch]);
      if (c.joinx[ch] > ch1) return fail("a joint intensity channel out of range");
    }
    for (int ch = ch0; ch < ch1; ch++) WU(THUFF, 2, c.thuff[ch]);
    for (int ch = ch0; ch < ch1; ch++) {
      WU(SHUFF, 3, c.shuff[ch]);
      if (c.shuff[ch] == 7) return fail("a scale factor code book out of range");
    }
    for (int ch = ch0; ch < ch1; ch++) {
      WU(BHUFF, 3, c.bhuff[ch]);
      if (c.bhuff[ch] == 7) return fail("a bit allocation code book out of range");
    }
    for (int a = 0; a < 10; a++)
      for (int ch = ch0; ch < ch1; ch++) WU(SEL, SEL_BITS[a], c.sel[ch][a]);
    for (int a = 0; a < 10; a++)
      for (int ch = ch0; ch < ch1; ch++) {
        if (c.sel[ch][a] < QB_GROUP[a])
          WU(ADJ, 2, c.adj[ch][a]);
        else
          c.adj[ch][a] = 0;
      }
    return true;
  }

  // a subframe of the pass's channels (c.xpass: the core's, with their header and the LFE; XCH's,
  // the core's subsubframes)
  bool subframe(CoreRec& c, int sf) {
    CoreSub& s = c.sub[sf];
    const int ch0 = c.ch0(), ch1 = c.ch1();
    if (!c.xpass) {
      WU(SSC, 2, s.ssc);
      s.nssc = s.ssc + 1;
      WU(PSC, 3, s.psc);
      if (s.psc) return fail("a partial subsubframe");
    }
    if (c.nblk + s.nssc * 8 > c.nblks + 1) return fail("more subband samples than blocks");
    io.subframe(c, sf);
    for (int ch = ch0; ch < ch1; ch++)
      for (int b = 0; b < c.nsubs[ch]; b++) {
        io.at(ch, b, 0);
        WU(PMODE, 1, s.pmode[ch][b]);
      }
    for (int ch = ch0; ch < ch1; ch++)
      for (int b = 0; b < c.nsubs[ch]; b++)
        if (s.pmode[ch][b]) {
          io.at(ch, b, 0);
          WU(PVQ, 12, s.pvq[ch][b]);
        }
    for (int ch = ch0; ch < ch1; ch++)
      for (int b = 0; b < c.nvqsub[ch]; b++) {
        io.at(ch, b, 0);
        if (c.bhuff[ch] < 5)
          WH(ABITS, BA[c.bhuff[ch]], s.abits[ch][b]);
        else
          WU(ABITS, c.bhuff[ch] - 1, s.abits[ch][b]);
        if (s.abits[ch][b] < 0 || s.abits[ch][b] > 26) return fail("a bit allocation out of range");
      }
    for (int ch = ch0; ch < ch1; ch++)
      for (int b = 0; b < c.nvqsub[ch]; b++) {
        if (s.nssc > 1 && s.abits[ch][b]) {
          io.at(ch, b, 0);
          WH(TMODE, TB[c.thuff[ch]], s.tmode[ch][b]);
          if (s.tmode[ch][b] >= s.nssc) return fail("a transient past the subframe");
        } else {
          s.tmode[ch][b] = 0;
        }
      }
    for (int ch = ch0; ch < ch1; ch++) {
      const int sh = c.shuff[ch], max = sh == 6 ? 128 : 64;
      int si = 0;
      for (int b = 0; b < c.nsubs[ch] || b < c.nvqsub[ch]; b++) {
        const int k = b < c.nvqsub[ch] ? (s.abits[ch][b] ? 1 + (s.tmode[ch][b] > 0) : 0) : 1;
        for (int j = 0; j < k; j++) {
          S64 v = s.scale[ch][b][j];
          io.at(ch, b, j);
          if (sh >= 5) {
            if (!io.u(F_SCALES, sh + 1, v)) return false;
          } else if (IO::BITS) {  // (the bits have the difference to the scale factor before)
            S64 d = v - si;
            if (!io.huff(F_SCALES, SB[sh], d)) return false;
            v = si + d;
          } else {
            if (!io.val(F_SCALES, 0, max - 1, v)) return false;
            io.pos += SB[sh].bits(v - si);
          }
          if (v < 0 || v >= max) return fail("a scale factor out of range");
          s.scale[ch][b][j] = (int)v;
          si = (int)v;
        }
      }
    }
    for (int ch = ch0; ch < ch1; ch++)
      if (c.joinx[ch]) {
        io.at(ch, 0, 0);
        WU(JOIN_SHUFF, 3, s.jsel[ch]);
        if (s.jsel[ch] == 7) return fail("a joint scale code book out of range");
      }
    for (int ch = ch0; ch < ch1; ch++) {
      if (!c.joinx[ch]) continue;
      const int src = c.joinx[ch] - 1;
      for (int b = c.nsubs[ch]; b < c.nsubs[src]; b++) {
        io.at(ch, b, 0);
        if (s.jsel[ch] < 5)
          WH(JOIN_SCALES, SB[s.jsel[ch]], s.jscale[ch][b]);
        else
          WU(JOIN_SCALES, s.jsel[ch] + 1, s.jscale[ch][b]);
      }
    }
    if (c.dynf && !c.xpass) WU(RANGE, 8, s.range);
    if (c.cpf) {
      if (c.xpass) WU(XSICRC, 16, s.xsicrc);
      else WU(SICRC, 16, s.sicrc);
    }
    for (int ch = ch0; ch < ch1; ch++)
      for (int b = c.nvqsub[ch]; b < c.nsubs[ch]; b++) {
        io.at(ch, b, 0);
        WU(HFVQ, 10, s.hfvq[ch][b]);
      }
    const int nlfe = c.lff && !c.xpass ? 2 * c.lff * s.nssc : 0;
    for (int i = 0; i < nlfe; i++) {
      io.at(0, 0, i);
      WS(LFE, 8, s.lfe[i]);
    }
    if (c.lff && !c.xpass) {
      WU(LFEscaleIndex, 8, s.lfescale);
      if (s.lfescale >= 128) return fail("an LFE scale factor out of range");
    }
    // the subband samples
    for (int ssf = 0; ssf < s.nssc; ssf++) {
      for (int ch = ch0; ch < ch1; ch++)
        for (int b = 0; b < c.nvqsub[ch]; b++) {
          const int a = s.abits[ch][b], how = sample_coding(c, ch, a);
          S32* v = &s.q[ch][b][ssf * 8];
          if (how == 1) {
            const HuffBook& hb = QB[a - 1][c.sel[ch][a - 1]];
            for (int m = 0; m < 8; m++) {
              io.at(ch, b, ssf * 8 + m);
              WH(q, hb, v[m]);
            }
          } else if (how == 3) {
            if (!block(v, a, ch, b, ssf * 8)) return false;
          } else if (how == 2) {
            for (int m = 0; m < 8; m++) {
              io.at(ch, b, ssf * 8 + m);
              WS(q, a - 3, v[m]);
            }
          }
        }
      if ((ssf == s.nssc - 1 || c.aspf) && !io.fixed(0xFFFF, 16, "no DSYNC where it should be")) return false;
    }
    io.subframe_end(c, sf);
    c.nblk += s.nssc * 8;
    return true;
  }

  // 8 samples of ABITS a in two block codes (each 4 samples as the digits of a number of base the
  // levels)
  bool block(S32* v, int a, int ch, int b, int j0) {
    const int L = BLOCK_LEVELS[a - 1], off = (L - 1) / 2, nb = BLOCK_BITS[a - 1];
    for (int h = 0; h < 2; h++, v += 4) {
      if (IO::BITS) {
        S64 code = 0;
        if (!IO::READ)
          for (int i = 3; i >= 0; i--) {
            if (v[i] < -off || v[i] > off) return fail("a value out of range");
            code = code * L + v[i] + off;
          }
        if (!io.u(F_q, nb, code)) return false;
        if (IO::READ) {
          for (int i = 0; i < 4; i++) {
            v[i] = (S32)(code % L) - off;
            code /= L;
          }
          if (code) return fail("a block code out of range");
        }
      } else {
        for (int i = 0; i < 4; i++) {
          S64 x = v[i];
          io.at(ch, b, j0 + 4 * h + i);
          if (!io.val(F_q, -off, off, x)) return false;
          v[i] = (S32)x;
        }
        io.pos += nb;
      }
    }
    return true;
  }
};

// --- frames -----------------------------------------------------------------------------------------

// the size of the core frame at p (av bytes there), where its header says one is there: the sync
// word and a frame size and block count in range; 0 where it isn't
static INLINE int core_size(pcU8 p, size_t av) {
  if (av < 16 || be32(p) != SYNC_CORE) return 0;
  const U32 w = be32(p + 4);  // FTYPE SHORT CPF NBLKS FSIZE ...
  const int nblks = (w >> 18) & 0x7F, fsize = ((w >> 4) & 0x3FFF) + 1;
  if (nblks < 5 || fsize < 96 || (size_t)fsize > av) return 0;
  return fsize;
}
// the subband samples of the core frame at p as its header says (its NBLKS + 1), 0 where p isn't one
static INLINE int core_blocks(pcU8 p, size_t n) {
  if (n < 16 || be32(p) != SYNC_CORE) return 0;
  return (int)((be32(p + 4) >> 18) & 0x7F) + 1;
}

// the core frame at p (n bytes, PAD readable after them) parsed into c: 0, or why it can't be
// (where its XCH extension doesn't parse, without it: its bytes a tail)
static const char* core_parse(pcU8 p, int n, CoreRec& c) {
  const char* why = 0;
  for (int no = 0; no < 2; no++) {
    BitIn bi;
    bi.init(p, (U64)n * 8);
    bi.no_xch = no;
    Walk<BitIn> w(bi);
    if (!w.core(c)) why = w.why();
    else if (bi.pos != (U64)n * 8) why = "a frame size otherwise than its header's";
    else return 0;
    if (!c.xch) break;
  }
  return why;
}
// c written back into out (room for cap bytes): its bytes, 0 where it doesn't make whole bytes
static int core_write(CoreRec& c, U8* out, int cap) {
  BitOut bo;
  bo.init(out, cap);
  Walk<BitOut> w(bo);
  if (!w.core(c) || bo.nacc || bo.n > cap) return 0;
  return bo.n;
}
