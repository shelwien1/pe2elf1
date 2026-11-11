// syntax.inc -- the DTS core frame (ETSI TS 102 114 V1.6.1, clause 5) as a record, and one walk of
// its syntax that reads it from the bits (BitIn), writes it as bits (BitOut), or at record level
// hands each element to an IO that codes it (the model, model.inc): dts2tsv's walk of the core,
// with what its IO is told of where it is. The Huffman code books are dtssplit's (huff.inc).

enum { SYNC_CORE = 0x7FFE8001, SYNC_XCH = 0x5A5A5A5A, SYNC_X96 = 0x1D95F262, SYNC_XXCH = 0x47004A03 };
enum { EXT_XCH = 0, EXT_X96 = 2, EXT_XXCH = 6 };  // EXT_AUDIO_ID's extensions in the core substream
constexpr int MAX_FRAME = 16384;  // a core frame's bytes, at most (FSIZE: 14 bits)
constexpr int PAD = 16;           // zero bytes after a buffer the bit reader may read
enum { NCH = 8, NSUB = 32, NSF = 16, NSSF = 4, NSMP = NSSF * 8, NSUB96 = 64 };

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
  int bits(S64 v) const { return has(v) ? elen[v - minlev] : 0; }
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
// after the subframes (tail) or before the extension (xgap). The extension EXT_AUDIO_ID names
// (XCH, X96, XXCH) has the fields X...: its being there (XCH) and the whole 4-byte words between the
// subframes' next 4-byte boundary and its sync word (XGAP) only the record's (FFmpeg finds it by
// the sync word); its frame size (XCH's and X96's), XCH's AMODE and the 3 bits after it, the CRCs
// of its coding header and a subframe's side information (XSICRC); X96's revision, its high
// resolution flag and first subband (X96...); XXCH's headers (XX...: TS 102 114 7.5's, FFmpeg's
// names), and their other bits (xhrest, xcrest: reserved, the CRCs) and those after its subframes
// (xpad). Its channels' elements are the core channels' (XCH's, XXCH's), or have the core's names
// (X96's subbands).
#define CORE_FIELDS(X)                                                                                         \
  X(FTYPE, H) X(SHORT, H) X(CPF, H) X(NBLKS, H) X(FSIZE, H) X(AMODE, H) X(SFREQ, H) X(RATE, H) X(FixedBit, H) \
  X(DYNF, H) X(TIMEF, H) X(AUXF, H) X(HDCD, H) X(EXT_AUDIO_ID, H) X(EXT_AUDIO, H) X(ASPF, H) X(LFF, H)        \
  X(HFLAG, H) X(HCRC, H) X(FILTS, H) X(VERNUM, H) X(CHIST, H) X(PCMR, H) X(SUMF, H) X(SUMS, H) X(DIALNORM, H) \
  X(SUBFS, H) X(PCHS, H) X(SUBS, H) X(VQSUB, H) X(JOINX, H) X(THUFF, H) X(SHUFF, H) X(BHUFF, H) X(SEL, H)    \
  X(ADJ, H) X(AHCRC, H) X(SSC, H) X(PSC, H) X(RANGE, H) X(SICRC, H) X(LFEscaleIndex, H) X(XCH, H)           \
  X(XGAP, H) X(XFSIZE, H) X(XAMODE, H) X(XRES, H) X(XAHCRC, H) X(XSICRC, H) X(X96REV, H) X(X96HR, H)           \
  X(X96START, H) X(XXHSIZE, H) X(XXCRC, H) X(XXMASKB, H) X(XXNSETS, H) X(XXFSIZE, H) X(XXCMASK, H)             \
  X(XXCHSIZE, H) X(XXNCH, H) X(XXSMASK, H) X(XXDMIX, H) X(XXDEMB, H) X(XXDSCALE, H) X(XXDMASK, H)             \
  X(XXDCOEF, H)                                                                                                \
  X(PMODE, B) X(PVQ, B) X(ABITS, B) X(TMODE, B) X(SCALES, B) X(JOIN_SHUFF, B) X(JOIN_SCALES, B) X(HFVQ, B)    \
  X(LFE, B) X(q, Q) X(tail, T) X(xgap, T) X(xhrest, T) X(xcrest, T) X(xpad, T)
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
struct X96Sub {  // an X96 subframe: by channel and subband (to 64), as the core's are; a VQ'd
                 // subband's vectors, one each pair of subsubframes
  int pmode[NCH][NSUB96], pvq[NCH][NSUB96], abits[NCH][NSUB96], scale[NCH][NSUB96];
  int jsel[NCH], jscale[NCH][NSUB96], vq[NCH][NSUB96][2];
  int sicrc;
  S32 q[NCH][NSUB96][NSMP];
};
struct CoreRec {
  int ftype, deficit, cpf, nblks, fsize, amode, sfreq, rate, fixedbit, dynf, timef, auxf, hdcd, ext_id, ext, aspf, lff,
      hflag, hcrc, filts, vernum, chist, pcmr, sumf, sums, dialnorm;
  int subfs, pchs, subs[NCH], vqsub[NCH], joinx[NCH], thuff[NCH], shuff[NCH], bhuff[NCH], sel[NCH][10], adj[NCH][10],
      ahcrc;
  int nsubfs, npchs, nsubs[NCH], nvqsub[NCH];
  int nblk;  // the subband samples of its subframes
  CoreSub sub[NSF];
  // The extension EXT_AUDIO_ID names, where it's in the record (xext): the whole words before its
  // sync word after the subframes' 4-byte boundary (XGAP), the bits from the subframes' end to it
  // (and their bytes); its channels after the core's (nxch: XCH's back centre, 6.1's, 1; XXCH's); XCH's
  // frame size (X96's too), AMODE, the 3 bits after it, its coding header's CRC (X96's too). The
  // walk's pass: 0 the core's channels, 1 the extension's, 2 X96's subbands.
  int xext, nxch, xgapw, xgap, xfsize, xamode, xres, xahcrc, xpass;
  U8 xgapb[MAX_FRAME];
  // X96 (96 kHz: subbands 32-63, and from x96start the core's residuals): its revision, high
  // resolution flag, first subband; each channel's subbands (SUBS: x96nsubs - 1), joint intensity
  // channel, scale factor and bit allocation code books, quantization index code books; by subframe,
  // its side information and samples
  int x96rev, x96hr, x96start, x96subs[NCH], x96nsubs[NCH], x96joinx[NCH], x96shuff[NCH], x96bhuff[NCH];
  int x96sel[NCH][10];
  X96Sub xs[NSF];
  // XXCH (its channels after the core's, nxch of them: one channel set): its frame header's size
  // (in bytes, less 1), its channel set header's CRC flag, the speaker masks' bits (less 1), the
  // channel sets (less 1), the first's data size (bytes less 1), the core's speaker mask, then the
  // header's other bits to its size (and their bytes); the channel set header's size, channels
  // (less 1), their speaker mask (above the core's 6), whether downmix coefficients, whether
  // embedded, their scale, each channel's mask of the core's channels and its coefficients (7 bits
  // each), the header's other bits; the bits after its subframes to its size
  int xxhsize, xxcrc, xxmaskb, xxnsets, xxfsize, xxhrest;
  S64 xxcmask, xxsmask, xxdmask[2];
  U8 xxhrestb[64];
  int xxchsize, xxnch, xxdmix, xxdemb, xxdscale, xxdcoef[2][32], xxcrest, xxpad;
  U8 xxcrestb[128];
  U8 xxpadb[MAX_FRAME];
  // the channels of the walk's pass: ch0() to ch1() - 1
  int ch0() const { return xpass ? npchs : 0; }
  int ch1() const { return xpass ? npchs + nxch : npchs; }
  int tailbits;  // the bits after the subframes (after XCH), and their bytes (the last one's unused bits 0)
  U8 tail[MAX_FRAME];
};

// the samples of a subsubframe of a subband: how they are coded (0 none, 1 Huffman, 2 plain, 3
// block codes)
// how X96's channel ch's samples of ABITS a (its bit allocation less 1) are coded, as sample_coding()
// says the core's are (X96's code books: a <= 6, 10 with its high resolution)
static INLINE int x96_coding(const CoreRec& c, int ch, int a) {
  if (a < 1) return 0;
  if (a <= 10) {
    if (c.x96sel[ch][a - 1] < QB_GROUP[a - 1]) return 1;
    if (a <= 7) return 3;
  }
  return 2;
}
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
// whether the extension c's header names may be in the record: XCH or XXCH with a channel for it, X96
static INLINE int ext_room(const CoreRec& c) {
  return c.ext && (((c.ext_id == EXT_XCH || c.ext_id == EXT_XXCH) && c.npchs < NCH) || c.ext_id == EXT_X96);
}
// the CRC-16 (CCITT, from 0xFFFF) of n bytes at p: 0 where they end in their CRC
static U32 crc16(pcU8 p, int n) {
  U32 c = 0xFFFF;
  for (int i = 0; i < n; i++) {
    c ^= (U32)p[i] << 8;
    for (int k = 0; k < 8; k++) c = c & 0x8000 ? ((c << 1) ^ 0x1021) & 0xFFFF : (c << 1) & 0xFFFF;
  }
  return c;
}

struct BitIn {
  enum { BITS = 1, READ = 1 };
  const U8* d;
  U64 pos, end;
  const char* err;
  int no_ext;  // the extension not parsed (the bits after the subframes all a tail)
  void init(const U8* p, U64 nbits) {
    d = p;
    pos = 0;
    end = nbits;
    err = 0;
    no_ext = 0;
  }
  // The extension of the core frame c (bits 0 to fend, its subframes' to pos) its header names,
  // where it may be there (ext_room()): its sync word searched for from the frame's end at 4-byte
  // places, at or after pos, as FFmpeg searches: XCH's frame size the distance to the frame's end,
  // or one more (96 bytes at least; AMODE 1, then 3 bits of 0); X96's the distance (96 bytes at
  // least); XXCH's frame header's CRC right (its size 11 bytes to the distance). c.xext, and
  // c.xgapw the whole words before it after pos's 4-byte boundary.
  void find_ext(CoreRec& c, U64 fend) {
    c.xext = c.xgapw = 0;
    if (no_ext || !ext_room(c)) return;
    const S64 fs = (S64)(fend >> 3);
    const U32 sync = c.ext_id == EXT_XCH ? SYNC_XCH : c.ext_id == EXT_X96 ? SYNC_X96 : SYNC_XXCH;
    for (S64 k = fs / 4 - 2; k >= 0 && (U64)k * 32 >= pos; k--) {
      if (be32(d + 4 * k) != sync) continue;
      const U32 w2 = be32(d + 4 * k + 4);
      const S64 dist = fs - 4 * k;
      int ok = 0;
      if (c.ext_id == EXT_XCH) {
        const S64 size = (S64)(w2 >> 22) + 1;
        ok = size >= 96 && (size == dist || size - 1 == dist) && ((w2 >> 15) & 0x7f) == 0x08;
      } else if (c.ext_id == EXT_X96) {
        const S64 size = (S64)(w2 >> 20) + 1;
        ok = size >= 96 && size == dist;
      } else {
        const S64 size = (S64)(w2 >> 26) + 1;
        ok = size >= 11 && size <= dist && !crc16(d + 4 * k + 4, (int)size - 4);
      }
      if (ok) {
        c.xext = 1;
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
  bool x96_defer(CoreRec&) { return false; }
  bool x96_books(CoreRec&) { return true; }
  bool x96_jdefer() { return false; }
  bool x96_jsel(const CoreRec&, X96Sub&) { return true; }
  void pos_to(U64 p) { pos = p; }
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
  bool x96_defer(CoreRec&) { return false; }
  bool x96_books(CoreRec&) { return true; }
  bool x96_jdefer() { return false; }
  bool x96_jsel(const CoreRec&, X96Sub&) { return true; }
  void pos_to(U64 p) { pos = p; }
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
    if (IO::READ) c.xext = c.nxch = c.xgapw = c.xgap = 0;
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
    // the extension the header names (XCH, X96, XXCH): found in the bits where it may be there, as
    // FFmpeg finds it, else as the record has it
    if (IO::BITS && IO::READ) find_ext(io, c, end);
    {
      S64 v = c.xext;
      if (!io.val(F_XCH, 0, ext_room(c), v)) return false;
      c.xext = (int)v;
    }
    if (c.xext) {
      // (its sync word at a 4-byte boundary of the frame: the subframes' next, or XGAP words after)
      const int pad = (int)((32 - ((io.pos - p0) & 31)) & 31);
      const S64 room = (S64)(end - io.pos) - pad;
      S64 v = c.xgapw;
      if (!io.val(F_XGAP, 0, room > 0 ? room / 32 : 0, v)) return false;
      c.xgapw = (int)v;
      c.xgap = pad + 32 * c.xgapw;
      if (!io.raw(F_xgap, c.xgap, c.xgapb)) return false;
      if (!(c.ext_id == EXT_XCH ? xch(c) : c.ext_id == EXT_X96 ? x96(c) : xxch(c, end))) return false;
      c.xpass = 0;
      if (io.pos > end) return fail("the extension past the end of the frame");
    }
    c.tailbits = (int)(end - io.pos);
    if (!io.raw(F_tail, c.tailbits, c.tail)) return false;
    return true;
  }
  // (BitIn's search, nothing for the other IOs)
  static void find_ext(BitIn& i, CoreRec& c, U64 end) { i.find_ext(c, end); }
  template <class I>
  static void find_ext(I&, CoreRec&, U64) {}

  // XCH (6.1's back centre): its header, its channel's coding header and subframes as the core's
  bool xch(CoreRec& c) {
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
    return true;
  }

  // XXCH (FFmpeg's parse_xxch_frame(), one channel set): its frame header, its channel set's header
  // (the coding header of its channels after the core's, a joint channel numbered from them), its
  // subframes as the core's; each header's bits to its size, the channel set's to its
  bool xxch(CoreRec& c, U64 end) {
    const U64 h0 = io.pos;
    if (!io.fixed(SYNC_XXCH, 32, "no XXCH sync word")) return false;
    WU(XXHSIZE, 6, c.xxhsize);
    WU(XXCRC, 1, c.xxcrc);
    WU(XXMASKB, 5, c.xxmaskb);
    const int mb = c.xxmaskb + 1;
    if (mb <= 6) return fail("an XXCH speaker mask's bits out of range");
    WU(XXNSETS, 2, c.xxnsets);
    if (c.xxnsets) return fail("XXCH channel sets past the first");
    WU(XXFSIZE, 14, c.xxfsize);
    if (!io.u(F_XXCMASK, mb, c.xxcmask)) return false;
    const S64 hr = (S64)(h0 + (U64)(c.xxhsize + 1) * 8) - (S64)io.pos;
    if (hr < 0) return fail("an XXCH frame header past its size");
    c.xxhrest = (int)hr;
    if (!io.raw(F_xhrest, c.xxhrest, c.xxhrestb)) return false;
    const U64 c0 = io.pos;
    WU(XXCHSIZE, 7, c.xxchsize);
    WU(XXNCH, 3, c.xxnch);
    const int nx = c.xxnch + 1;
    if (nx > 2 || c.npchs + nx > NCH) return fail("XXCH channels out of range");
    if (!io.u(F_XXSMASK, mb - 6, c.xxsmask)) return false;
    WU(XXDMIX, 1, c.xxdmix);
    if (c.xxdmix) {
      WU(XXDEMB, 1, c.xxdemb);
      WU(XXDSCALE, 6, c.xxdscale);
      for (int ch = 0; ch < nx; ch++)
        if (!io.u(F_XXDMASK, mb, c.xxdmask[ch])) return false;
      for (int ch = 0; ch < nx; ch++)
        for (int n = 0; n < mb; n++)
          if ((c.xxdmask[ch] >> n) & 1) WU(XXDCOEF, 7, c.xxdcoef[ch][n]);
    }
    c.nxch = nx;
    if (!coding_header(c, c.npchs, c.npchs + nx, c.npchs - 1)) return false;
    const S64 cr = (S64)(c0 + (U64)(c.xxchsize + 1) * 8) - (S64)io.pos;
    if (cr < 0) return fail("an XXCH channel set header past its size");
    c.xxcrest = (int)cr;
    if (!io.raw(F_xcrest, c.xxcrest, c.xxcrestb)) return false;
    c.xpass = 1;
    c.nblk = 0;
    for (int sf = 0; sf < c.nsubfs; sf++)
      if (!subframe(c, sf)) return false;
    const U64 e = h0 + (U64)(c.xxhsize + 1 + c.xxfsize + 1) * 8;
    if (e < io.pos || e > end) return fail("an XXCH channel set past its size");
    c.xxpad = (int)(e - io.pos);
    return io.raw(F_xpad, c.xxpad, c.xxpadb);
  }

  // X96 (FFmpeg's parse_x96_frame(), the core substream's): its header, its coding header (the
  // core's channels' subbands from x96start to theirs: their code books), its subframes
  bool x96(CoreRec& c) {
    if (!io.fixed(SYNC_X96, 32, "no X96 sync word")) return false;
    WU(XFSIZE, 12, c.xfsize);
    WU(X96REV, 4, c.x96rev);
    if (c.x96rev < 1 || c.x96rev > 8) return fail("an X96 revision out of range");
    WU(X96HR, 1, c.x96hr);
    if (c.x96rev < 8) {
      WU(X96START, 5, c.x96start);
      if (c.x96start > 27) return fail("an X96 first subband out of range");
    } else {
      c.x96start = NSUB;
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(SUBS, 6, c.x96subs[ch]);
      if ((c.x96nsubs[ch] = c.x96subs[ch] + 1) < NSUB) return fail("an X96 subband count out of range");
    }
    for (int ch = 0; ch < c.npchs; ch++) {
      WU(JOINX, 3, c.x96joinx[ch]);
      if (c.x96joinx[ch] > c.npchs) return fail("an X96 joint intensity channel out of range");
    }
    // (the model may code the code books after the subframes, from the values: x96_defer(), the
    // subframes then walked with each the first book, whose values are those of all the books, then
    // x96_books(), and the subframes' bits counted with the books)
    const bool dfr = io.x96_defer(c);
    if (dfr) {
      io.pos += (U64)c.npchs * 6;
      for (int a = 0; a < 6 + 4 * c.x96hr; a++) io.pos += (U64)c.npchs * SEL_BITS[a];
    } else {
      for (int ch = 0; ch < c.npchs; ch++) {
        WU(SHUFF, 3, c.x96shuff[ch]);
        if (c.x96shuff[ch] >= 6) return fail("an X96 scale factor code book out of range");
      }
      for (int ch = 0; ch < c.npchs; ch++) WU(BHUFF, 3, c.x96bhuff[ch]);
      for (int a = 0; a < 6 + 4 * c.x96hr; a++)
        for (int ch = 0; ch < c.npchs; ch++) WU(SEL, SEL_BITS[a], c.x96sel[ch][a]);
    }
    if (c.cpf) WU(XAHCRC, 16, c.xahcrc);
    c.xpass = 2;
    c.nblk = 0;
    const U64 p0 = io.pos;
    for (int sf = 0; sf < c.nsubfs; sf++)
      if (!x96_subframe(c, sf)) return false;
    if (dfr) {
      if (!io.x96_books(c)) return fail("an X96 code book out of range");
      BitOut bo;  // (counts, writes nothing)
      bo.init(nullptr, 0);
      Walk<BitOut> wb(bo);
      const int nb = c.nblk;
      c.nblk = 0;
      for (int sf = 0; sf < c.nsubfs; sf++)
        if (!wb.x96_subframe(c, sf)) return fail(wb.why());
      c.nblk = nb;
      io.pos_to(p0 + bo.pos);
    }
    return true;
  }
  // a value v of a field coded as its difference from the one before (prev) by the book hb (the
  // bits'; the record has the value, in lo..hi)
  bool diff(int fid, const HuffBook& hb, int prev, S64 lo, S64 hi, S64& v) {
    if (IO::BITS) {
      S64 d = v - prev;
      if (!io.huff(fid, hb, d)) return false;
      v = prev + d;
    } else {
      if (!io.val(fid, lo, hi, v)) return false;
      if (!hb.has(v - prev)) return fail("a difference its code book hasn't");
      io.pos += hb.bits(v - prev);
    }
    return v >= lo && v <= hi;
  }
  // an X96 subframe (FFmpeg's parse_x96_subframe_header() and _audio()): the side information of
  // each channel's subbands from x96start, the VQ'd subbands' vectors, the samples
  bool x96_subframe(CoreRec& c, int sf) {
    X96Sub& s = c.xs[sf];
    const int nssc = c.sub[sf].nssc, b0 = c.x96start, hr = c.x96hr;
    if (c.nblk + nssc * 8 > c.nblks + 1) return fail("more subband samples than blocks");
    io.subframe(c, sf);
    for (int ch = 0; ch < c.npchs; ch++)
      for (int b = b0; b < c.x96nsubs[ch]; b++) {
        io.at(ch, b, 0);
        WU(PMODE, 1, s.pmode[ch][b]);
      }
    for (int ch = 0; ch < c.npchs; ch++)
      for (int b = b0; b < c.x96nsubs[ch]; b++)
        if (s.pmode[ch][b]) {
          io.at(ch, b, 0);
          WU(PVQ, 12, s.pvq[ch][b]);
        }
    for (int ch = 0; ch < c.npchs; ch++) {  // (ABITS: 0 none, 1 VQ, else the quantizer's ABITS + 1)
      const int sel = c.x96bhuff[ch], max = 7 + 8 * hr;
      int ab = 0;
      for (int b = b0; b < c.x96nsubs[ch]; b++) {
        S64 v = s.abits[ch][b];
        io.at(ch, b, 0);
        if (sel == 7) {
          if (!io.u(F_ABITS, 3 + hr, v)) return false;
        } else if (!diff(F_ABITS, QB[5 + 2 * hr][sel], ab, 0, max, v)) {
          return fail("an X96 bit allocation out of range");
        }
        if (v > max) return fail("an X96 bit allocation out of range");
        s.abits[ch][b] = ab = (int)v;
      }
    }
    for (int ch = 0; ch < c.npchs; ch++) {  // (one each subband, transmitted where no bits are allocated too)
      const int sh = c.x96shuff[ch];
      int si = 0;
      for (int b = b0; b < c.x96nsubs[ch]; b++) {
        S64 v = s.scale[ch][b];
        io.at(ch, b, 0);
        if (sh == 5) {
          if (!io.u(F_SCALES, 6, v)) return false;
        } else if (!diff(F_SCALES, SB[sh], si, 0, 63, v)) {
          return fail("a scale factor out of range");
        }
        s.scale[ch][b] = si = (int)v;
      }
    }
    // (the model may code the joint scale factors' code books after them, x96_jdefer(): the scale
    // factors walked as any book's values, then the books, x96_jsel(), and their bits counted)
    const bool jd = io.x96_jdefer();
    for (int ch = 0; ch < c.npchs; ch++)
      if (c.x96joinx[ch]) {
        io.at(ch, 0, 0);
        if (jd) {
          io.pos += 3;
          continue;
        }
        WU(JOIN_SHUFF, 3, s.jsel[ch]);
        if (s.jsel[ch] == 7) return fail("a joint scale code book out of range");
      }
    for (int ch = 0; ch < c.npchs; ch++) {
      if (!c.x96joinx[ch]) continue;
      const int src = c.x96joinx[ch] - 1;
      for (int b = c.x96nsubs[ch]; b < c.x96nsubs[src]; b++) {
        io.at(ch, b, 0);
        if (jd) {
          S64 v = s.jscale[ch][b];
          if (!io.val(F_JOIN_SCALES, -64, 127, v)) return false;
          s.jscale[ch][b] = (int)v;
        } else if (s.jsel[ch] < 5) {
          WH(JOIN_SCALES, SB[s.jsel[ch]], s.jscale[ch][b]);
        } else {
          WU(JOIN_SCALES, s.jsel[ch] + 1, s.jscale[ch][b]);
        }
      }
    }
    if (jd) {
      if (!io.x96_jsel(c, s)) return fail("a joint scale code book out of range");
      for (int ch = 0; ch < c.npchs; ch++) {
        if (!c.x96joinx[ch]) continue;
        const int src = c.x96joinx[ch] - 1, k = s.jsel[ch];
        for (int b = c.x96nsubs[ch]; b < c.x96nsubs[src]; b++) {
          const int v = s.jscale[ch][b];
          if (k < 5 ? !SB[k].has(v) : v < 0 || v >= 1 << (k + 1)) return fail("a joint scale factor its code book hasn't");
          io.pos += k < 5 ? SB[k].bits(v) : k + 1;
        }
      }
    }
    if (c.cpf) WU(XSICRC, 16, s.sicrc);
    for (int ch = 0; ch < c.npchs; ch++)  // (a VQ'd subband's vectors: 16 samples each)
      for (int b = b0; b < c.x96nsubs[ch]; b++)
        if (s.abits[ch][b] == 1)
          for (int p = 0; p < (nssc + 1) / 2; p++) {
            io.at(ch, b, p);
            WU(HFVQ, 10, s.vq[ch][b][p]);
          }
    for (int ssf = 0; ssf < nssc; ssf++) {
      for (int ch = 0; ch < c.npchs; ch++)
        for (int b = b0; b < c.x96nsubs[ch]; b++) {
          const int a = s.abits[ch][b] - 1, how = x96_coding(c, ch, a);
          S32* v = &s.q[ch][b][ssf * 8];
          if (how == 1) {
            const HuffBook& hb = QB[a - 1][c.x96sel[ch][a - 1]];
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
      if ((ssf == nssc - 1 || c.aspf) && !io.fixed(0xFFFF, 16, "no X96 DSYNC where it should be")) return false;
    }
    io.subframe_end(c, sf);
    c.nblk += nssc * 8;
    return true;
  }

  // the coding header's fields of channels ch0 to ch1 - 1 (the core's; XCH's; XXCH's, whose joint
  // channel's number, 1 up, counts from jbase + 1: from its own first channel)
  bool coding_header(CoreRec& c, int ch0, int ch1, int jbase = 0) {
    for (int ch = ch0; ch < ch1; ch++) {
      WU(SUBS, 5, c.subs[ch]);
      if ((c.nsubs[ch] = c.subs[ch] + 2) > NSUB) return fail("a subband count out of range");
    }
    for (int ch = ch0; ch < ch1; ch++) {
      WU(VQSUB, 5, c.vqsub[ch]);
      c.nvqsub[ch] = c.vqsub[ch] + 1;
    }
    for (int ch = ch0; ch < ch1; ch++) {
      S64 v = c.joinx[ch] ? c.joinx[ch] - jbase : 0;
      if (!io.u(F_JOINX, 3, v)) return false;
      c.joinx[ch] = v ? (int)v + jbase : 0;
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

// the core's samples a second by its SFREQ (0: none)
static INLINE int core_rate(int sfreq) {
  static const int R[16] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};
  return R[sfreq & 15];
}

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
// (where its extension doesn't parse, without it: its bytes a tail)
static const char* core_parse(pcU8 p, int n, CoreRec& c) {
  const char* why = 0;
  for (int no = 0; no < 2; no++) {
    BitIn bi;
    bi.init(p, (U64)n * 8);
    bi.no_ext = no;
    Walk<BitIn> w(bi);
    if (!w.core(c)) why = w.why();
    else if (bi.pos != (U64)n * 8) why = "a frame size otherwise than its header's";
    else return 0;
    if (!c.xext) break;
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
