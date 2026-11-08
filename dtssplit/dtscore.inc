// dtscore.inc -- the DTS core frame (ETSI TS 102 114 V1.6.1, clause 5): its syntax down to the
// quantization indexes of its subband samples, parsed and written back bit for bit. What follows
// the subframes (the optional information, the extensions in the core frame: XCh, X96, XXCH) is
// kept as bits. The includer defines U8..S64, BitReader and BitWriter.

struct HuffEntry {
  S16 level;
  U8 len;
  U32 code;
};
#include "dtscore_huff.inc"

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
  bool get(BitReader& r, int& v) const {
    const U32 w = r.peek32();
    int lo = 0, hi = n - 1;  // the last entry whose code is <= w
    while (lo < hi) {
      const int m = (lo + hi + 1) >> 1;
      if (lcode[m] <= w)
        lo = m;
      else
        hi = m - 1;
    }
    if (r.pos + len[lo] > r.end) return false;
    r.pos += len[lo];
    v = lev[lo];
    return true;
  }
  bool put(BitWriter& w, int v) const {
    if (v < minlev || v >= minlev + nlev) return false;
    w.put(ecode[v - minlev], elen[v - minlev]);
    return true;
  }
};

#define HB(x) HB_##x, countof(HB_##x)
static HuffBook QB[10][7];  // quantization indexes: by ABITS-1, then SEL
static HuffBook BA[5];      // ABITS (BHUFF 0-4)
static HuffBook SB[5];      // scale factors (SHUFF 0-4)
static HuffBook TB[4];      // TMODE (THUFF)
static const int QB_GROUP[10] = {1, 3, 3, 3, 3, 7, 7, 7, 7, 7};    // Huffman books per ABITS
static const int SEL_BITS[10] = {1, 2, 2, 2, 2, 3, 3, 3, 3, 3};
static const int BLOCK_LEVELS[7] = {3, 5, 7, 9, 13, 17, 25};       // block codes, ABITS 1-7
static const int BLOCK_BITS[7] = {7, 10, 12, 13, 15, 17, 19};      // a code of 4 samples

static void dtscore_init(void) {
  static bool done = false;
  if (done) return;
  done = true;
  QB[0][0].init(HB(A3));
  const HuffEntry* const* q;
  static const HuffEntry* const q5[] = {HB_A5, HB_B5, HB_C5}, *const q7[] = {HB_A7, HB_B7, HB_C7},
                                   *const q9[] = {HB_A9, HB_B9, HB_C9}, *const q13[] = {HB_A13, HB_B13, HB_C13};
  static const HuffEntry* const q17[] = {HB_A17, HB_B17, HB_C17, HB_D17, HB_E17, HB_F17, HB_G17};
  static const HuffEntry* const q25[] = {HB_A25, HB_B25, HB_C25, HB_D25, HB_E25, HB_F25, HB_G25};
  static const HuffEntry* const q33[] = {HB_A33, HB_B33, HB_C33, HB_D33, HB_E33, HB_F33, HB_G33};
  static const HuffEntry* const q65[] = {HB_A65, HB_B65, HB_C65, HB_D65, HB_E65, HB_F65, HB_G65};
  static const HuffEntry* const q129[] = {HB_A129, HB_B129, HB_C129, HB_D129, HB_E129, HB_F129, HB_G129};
  static const int lv[10] = {3, 5, 7, 9, 13, 17, 25, 33, 65, 129};
  for (int a = 1; a < 10; a++) {
    q = a == 1 ? q5 : a == 2 ? q7 : a == 3 ? q9 : a == 4 ? q13 : a == 5 ? q17 : a == 6 ? q25 : a == 7 ? q33 : a == 8 ? q65 : q129;
    for (int s = 0; s < QB_GROUP[a]; s++) QB[a][s].init(q[s], lv[a]);
  }
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

enum { DC_CH = 8, DC_SUB = 32, DC_SF = 16, DC_SSF = 4 };

// what each part of a core frame takes, in bits
enum { DC_HDR, DC_PMODE, DC_PVQ, DC_ABITS, DC_TMODE, DC_SCALE, DC_JOINT, DC_SIDE, DC_HFVQ, DC_LFE,
       DC_HUFF, DC_BLOCK, DC_NFE, DC_DSYNC, DC_TAIL, DC_NCAT };

struct CoreSub {  // a subframe
  int nssc, psc;
  U8 pmode[DC_CH][DC_SUB];
  U16 pvq[DC_CH][DC_SUB];
  U8 abits[DC_CH][DC_SUB];
  U8 tmode[DC_CH][DC_SUB];
  U8 scale[DC_CH][DC_SUB][2];  // the scale factors' quantization indexes
  U8 jsel[DC_CH];
  S16 jscale[DC_CH][DC_SUB];   // the joint scale factors' indexes as coded (64 less than the table's)
  int drc, sicrc;
  U16 hfvq[DC_CH][DC_SUB];
  int nlfe, lfe_scale;
  S8 lfe[16];
  S32 q[DC_CH][DC_SUB][DC_SSF * 8];  // the quantization indexes, by subsubframe and sample
  U16 dsync[DC_SSF];
};

struct CoreFrame {
  // the bit stream header
  int ftype, deficit, cpf, nblks, fsize, amode, sfreq, rate, fixedbit, dynf, timef, auxf, hdcd, ext_id, ext, aspf, lff,
      hflag, hcrc, filts, vernum, chist, pcmr, sumf, sums, dialnorm;
  // the primary audio coding header
  int nsubfs, npchs, nsubs[DC_CH], vqsub[DC_CH], joinx[DC_CH], thuff[DC_CH], shuff[DC_CH], bhuff[DC_CH];
  int sel[DC_CH][10], adj[DC_CH][10], ahcrc;
  CoreSub sub[DC_SF];
  U64 data_end;  // where the subframes end
  U64 bits[DC_NCAT];
};

// the block code of the 4 samples v (ABITS a)
static inline bool block_code(const S32* v, int a, U32& code) {
  const int L = BLOCK_LEVELS[a - 1], off = (L - 1) / 2;
  code = 0;
  for (int i = 3; i >= 0; i--) {
    if (v[i] < -off || v[i] > off) return false;
    code = code * L + (U32)(v[i] + off);
  }
  return true;
}

// the samples of a subsubframe of a subband: how they are coded (0 none, 1 Huffman, 2 none further,
// 3 block codes)
static inline int sample_coding(const CoreFrame& f, int ch, int a) {
  if (!a) return 0;
  if (a <= 10) {
    const int s = f.sel[ch][a - 1];
    if (s < QB_GROUP[a - 1]) return 1;
    if (a <= 7) return 3;
  }
  return 2;
}

// The core frame p (n bytes, PAD more) parsed into f: 0, or why it can't be
static const char* core_parse(const U8* p, int n, CoreFrame& f) {
  BitReader r;
  r.init(p, 0, (U64)n * 8);
  memset(f.bits, 0, sizeof f.bits);
  if (r.get(32) != 0x7FFE8001) return "no core sync word";
  f.ftype = r.get(1);
  f.deficit = r.get(5);
  f.cpf = r.get(1);
  f.nblks = r.get(7);
  f.fsize = r.get(14);
  f.amode = r.get(6);
  f.sfreq = r.get(4);
  f.rate = r.get(5);
  f.fixedbit = r.get(1);
  f.dynf = r.get(1);
  f.timef = r.get(1);
  f.auxf = r.get(1);
  f.hdcd = r.get(1);
  f.ext_id = r.get(3);
  f.ext = r.get(1);
  f.aspf = r.get(1);
  f.lff = r.get(2);
  f.hflag = r.get(1);
  f.hcrc = f.cpf ? (int)r.get(16) : 0;
  f.filts = r.get(1);
  f.vernum = r.get(4);
  f.chist = r.get(2);
  f.pcmr = r.get(3);
  f.sumf = r.get(1);
  f.sums = r.get(1);
  f.dialnorm = r.get(4);
  if (f.fsize + 1 > n || f.fsize < 95) return "a core frame size out of range";
  if (f.lff == 3) return "an LFE flag out of range";
  const U64 end = (U64)(f.fsize + 1) * 8;
  r.end = end;
  // the primary audio coding header
  f.nsubfs = r.get(4) + 1;
  f.npchs = r.get(3) + 1;
  for (int ch = 0; ch < f.npchs; ch++)
    if ((f.nsubs[ch] = r.get(5) + 2) > DC_SUB) return "a subband count out of range";
  for (int ch = 0; ch < f.npchs; ch++) f.vqsub[ch] = r.get(5) + 1;
  for (int ch = 0; ch < f.npchs; ch++)
    if ((f.joinx[ch] = r.get(3)) > f.npchs) return "a joint intensity channel out of range";
  for (int ch = 0; ch < f.npchs; ch++) f.thuff[ch] = r.get(2);
  for (int ch = 0; ch < f.npchs; ch++)
    if ((f.shuff[ch] = r.get(3)) == 7) return "a scale factor code book out of range";
  for (int ch = 0; ch < f.npchs; ch++)
    if ((f.bhuff[ch] = r.get(3)) == 7) return "a bit allocation code book out of range";
  for (int a = 0; a < 10; a++)
    for (int ch = 0; ch < f.npchs; ch++) f.sel[ch][a] = r.get(SEL_BITS[a]);
  for (int a = 0; a < 10; a++)
    for (int ch = 0; ch < f.npchs; ch++) f.adj[ch][a] = f.sel[ch][a] < QB_GROUP[a] ? (int)r.get(2) : 0;
  f.ahcrc = f.cpf ? (int)r.get(16) : 0;
  f.bits[DC_HDR] = r.pos;
  int nblk = 0;  // subband samples so far
  for (int sf = 0; sf < f.nsubfs; sf++) {
    CoreSub& s = f.sub[sf];
    U64 at = r.pos;
    s.nssc = r.get(2) + 1;
    s.psc = r.get(3);
    if (s.psc) return "a partial subsubframe";
    nblk += s.nssc * 8;
    if (nblk > f.nblks + 1) return "more subband samples than blocks";
    f.bits[DC_SIDE] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.nsubs[ch]; b++) s.pmode[ch][b] = r.get(1);
    f.bits[DC_PMODE] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.nsubs[ch]; b++) s.pvq[ch][b] = s.pmode[ch][b] ? r.get(12) : 0;
    f.bits[DC_PVQ] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.vqsub[ch]; b++) {
        int a;
        if (f.bhuff[ch] < 5) {
          if (!BA[f.bhuff[ch]].get(r, a)) return "a frame cut short";
        } else {
          a = r.get(f.bhuff[ch] - 1);
        }
        if (a > 26) return "a bit allocation out of range";
        s.abits[ch][b] = (U8)a;
      }
    f.bits[DC_ABITS] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.vqsub[ch]; b++) {
        int t = 0;
        if (s.nssc > 1 && s.abits[ch][b] && !TB[f.thuff[ch]].get(r, t)) return "a frame cut short";
        s.tmode[ch][b] = (U8)t;
      }
    f.bits[DC_TMODE] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++) {
      const int sel = f.shuff[ch], max = sel == 6 ? 128 : 64;
      int si = 0;
      for (int b = 0; b < f.nsubs[ch] || b < f.vqsub[ch]; b++) {
        s.scale[ch][b][0] = s.scale[ch][b][1] = 0;
        const int k = b < f.vqsub[ch] ? (s.abits[ch][b] ? 1 + (s.tmode[ch][b] > 0) : 0) : 1;
        for (int j = 0; j < k; j++) {
          if (sel < 5) {
            int d;
            if (!SB[sel].get(r, d)) return "a frame cut short";
            si += d;
          } else {
            si = r.get(sel + 1);
          }
          if (si < 0 || si >= max) return "a scale factor out of range";
          s.scale[ch][b][j] = (U8)si;
        }
      }
    }
    f.bits[DC_SCALE] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      if (f.joinx[ch] && (s.jsel[ch] = r.get(3)) == 7) return "a joint scale code book out of range";
    for (int ch = 0; ch < f.npchs; ch++)
      if (f.joinx[ch]) {
        const int src = f.joinx[ch] - 1;
        for (int b = f.nsubs[ch]; b < f.nsubs[src]; b++) {
          int v;
          if (s.jsel[ch] < 5) {
            if (!SB[s.jsel[ch]].get(r, v)) return "a frame cut short";
          } else {
            v = r.get(s.jsel[ch] + 1);
          }
          s.jscale[ch][b] = (S16)v;
        }
      }
    f.bits[DC_JOINT] += r.pos - at;
    at = r.pos;
    s.drc = f.dynf ? (int)r.get(8) : 0;
    s.sicrc = f.cpf ? (int)r.get(16) : 0;
    f.bits[DC_SIDE] += r.pos - at;
    at = r.pos;
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = f.vqsub[ch]; b < f.nsubs[ch]; b++) s.hfvq[ch][b] = r.get(10);
    f.bits[DC_HFVQ] += r.pos - at;
    at = r.pos;
    s.nlfe = f.lff ? 2 * f.lff * s.nssc : 0;
    for (int i = 0; i < s.nlfe; i++) s.lfe[i] = (S8)r.get(8);
    s.lfe_scale = f.lff ? (int)r.get(8) : 0;
    f.bits[DC_LFE] += r.pos - at;
    for (int ssf = 0; ssf < s.nssc; ssf++) {
      for (int ch = 0; ch < f.npchs; ch++)
        for (int b = 0; b < f.vqsub[ch]; b++) {
          const int a = s.abits[ch][b], how = sample_coding(f, ch, a);
          S32* v = &s.q[ch][b][ssf * 8];
          at = r.pos;
          if (how == 0) {
            memset(v, 0, 8 * sizeof *v);
          } else if (how == 1) {
            const HuffBook& hb = QB[a - 1][f.sel[ch][a - 1]];
            for (int m = 0; m < 8; m++) {
              int x;
              if (!hb.get(r, x)) return "a frame cut short";
              v[m] = x;
            }
            f.bits[DC_HUFF] += r.pos - at;
          } else if (how == 3) {
            const int L = BLOCK_LEVELS[a - 1], off = (L - 1) / 2;
            for (int h = 0; h < 2; h++) {
              U32 c = r.get(BLOCK_BITS[a - 1]);
              for (int m = 0; m < 4; m++) {
                v[h * 4 + m] = (S32)(c % L) - off;
                c /= L;
              }
              if (c) return "a block code out of range";
            }
            f.bits[DC_BLOCK] += r.pos - at;
          } else {
            const int nb = a - 3;
            for (int m = 0; m < 8; m++) v[m] = (S32)(r.get(nb) << (32 - nb)) >> (32 - nb);
            f.bits[DC_NFE] += r.pos - at;
          }
        }
      at = r.pos;
      s.dsync[ssf] = ssf == s.nssc - 1 || f.aspf ? (U16)r.get(16) : 0xFFFF;
      if (s.dsync[ssf] != 0xFFFF) return "no DSYNC where it should be";
      f.bits[DC_DSYNC] += r.pos - at;
    }
  }
  if (r.over) return "a frame cut short";
  f.data_end = r.pos;
  f.bits[DC_TAIL] = end - r.pos;
  return 0;
}

// f written back (its tail from p, the frame it was parsed from, or tail: the tail's bits from bit 0)
__attribute__((unused)) static bool core_write(const CoreFrame& f, const U8* tail, U64 tail_pos, BitWriter& w) {
  w.reset();
  w.put(0x7FFE8001, 32);
  w.put(f.ftype, 1);
  w.put(f.deficit, 5);
  w.put(f.cpf, 1);
  w.put(f.nblks, 7);
  w.put(f.fsize, 14);
  w.put(f.amode, 6);
  w.put(f.sfreq, 4);
  w.put(f.rate, 5);
  w.put(f.fixedbit, 1);
  w.put(f.dynf, 1);
  w.put(f.timef, 1);
  w.put(f.auxf, 1);
  w.put(f.hdcd, 1);
  w.put(f.ext_id, 3);
  w.put(f.ext, 1);
  w.put(f.aspf, 1);
  w.put(f.lff, 2);
  w.put(f.hflag, 1);
  if (f.cpf) w.put(f.hcrc, 16);
  w.put(f.filts, 1);
  w.put(f.vernum, 4);
  w.put(f.chist, 2);
  w.put(f.pcmr, 3);
  w.put(f.sumf, 1);
  w.put(f.sums, 1);
  w.put(f.dialnorm, 4);
  w.put(f.nsubfs - 1, 4);
  w.put(f.npchs - 1, 3);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.nsubs[ch] - 2, 5);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.vqsub[ch] - 1, 5);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.joinx[ch], 3);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.thuff[ch], 2);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.shuff[ch], 3);
  for (int ch = 0; ch < f.npchs; ch++) w.put(f.bhuff[ch], 3);
  for (int a = 0; a < 10; a++)
    for (int ch = 0; ch < f.npchs; ch++) w.put(f.sel[ch][a], SEL_BITS[a]);
  for (int a = 0; a < 10; a++)
    for (int ch = 0; ch < f.npchs; ch++)
      if (f.sel[ch][a] < QB_GROUP[a]) w.put(f.adj[ch][a], 2);
  if (f.cpf) w.put(f.ahcrc, 16);
  for (int sf = 0; sf < f.nsubfs; sf++) {
    const CoreSub& s = f.sub[sf];
    w.put(s.nssc - 1, 2);
    w.put(s.psc, 3);
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.nsubs[ch]; b++) w.put(s.pmode[ch][b], 1);
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.nsubs[ch]; b++)
        if (s.pmode[ch][b]) w.put(s.pvq[ch][b], 12);
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.vqsub[ch]; b++) {
        if (f.bhuff[ch] < 5) {
          if (!BA[f.bhuff[ch]].put(w, s.abits[ch][b])) return false;
        } else {
          w.put(s.abits[ch][b], f.bhuff[ch] - 1);
        }
      }
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = 0; b < f.vqsub[ch]; b++)
        if (s.nssc > 1 && s.abits[ch][b] && !TB[f.thuff[ch]].put(w, s.tmode[ch][b])) return false;
    for (int ch = 0; ch < f.npchs; ch++) {
      const int sel = f.shuff[ch];
      int si = 0;
      for (int b = 0; b < f.nsubs[ch] || b < f.vqsub[ch]; b++) {
        const int k = b < f.vqsub[ch] ? (s.abits[ch][b] ? 1 + (s.tmode[ch][b] > 0) : 0) : 1;
        for (int j = 0; j < k; j++) {
          if (sel < 5) {
            if (!SB[sel].put(w, s.scale[ch][b][j] - si)) return false;
            si = s.scale[ch][b][j];
          } else {
            w.put(s.scale[ch][b][j], sel + 1);
          }
        }
      }
    }
    for (int ch = 0; ch < f.npchs; ch++)
      if (f.joinx[ch]) w.put(s.jsel[ch], 3);
    for (int ch = 0; ch < f.npchs; ch++)
      if (f.joinx[ch]) {
        const int src = f.joinx[ch] - 1;
        for (int b = f.nsubs[ch]; b < f.nsubs[src]; b++) {
          if (s.jsel[ch] < 5) {
            if (!SB[s.jsel[ch]].put(w, s.jscale[ch][b])) return false;
          } else {
            w.put((U32)s.jscale[ch][b], s.jsel[ch] + 1);
          }
        }
      }
    if (f.dynf) w.put(s.drc, 8);
    if (f.cpf) w.put(s.sicrc, 16);
    for (int ch = 0; ch < f.npchs; ch++)
      for (int b = f.vqsub[ch]; b < f.nsubs[ch]; b++) w.put(s.hfvq[ch][b], 10);
    for (int i = 0; i < s.nlfe; i++) w.put((U8)s.lfe[i], 8);
    if (f.lff) w.put(s.lfe_scale, 8);
    for (int ssf = 0; ssf < s.nssc; ssf++) {
      for (int ch = 0; ch < f.npchs; ch++)
        for (int b = 0; b < f.vqsub[ch]; b++) {
          const int a = s.abits[ch][b], how = sample_coding(f, ch, a);
          const S32* v = &s.q[ch][b][ssf * 8];
          if (how == 1) {
            const HuffBook& hb = QB[a - 1][f.sel[ch][a - 1]];
            for (int m = 0; m < 8; m++)
              if (!hb.put(w, v[m])) return false;
          } else if (how == 3) {
            for (int h = 0; h < 2; h++) {
              U32 c;
              if (!block_code(v + h * 4, a, c)) return false;
              w.put(c, BLOCK_BITS[a - 1]);
            }
          } else if (how == 2) {
            const int nb = a - 3;
            for (int m = 0; m < 8; m++) {
              if (nb < 32 && (v[m] < -(1 << (nb - 1)) || v[m] >= 1 << (nb - 1))) return false;
              w.put((U32)v[m], nb);
            }
          }
        }
      if (ssf == s.nssc - 1 || f.aspf) w.put(s.dsync[ssf], 16);
    }
  }
  if (w.bits() != f.data_end) return false;
  const U64 tn = (U64)(f.fsize + 1) * 8 - f.data_end;
  w.copy(tail, tail_pos, tn);
  return !w.nacc;
}
