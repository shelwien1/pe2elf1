// wordmodel.inc -- WordModel, rebuilt from WORDMODEL.md.
//
// This file replaces the WordModel that sits under `#if 0` in paq8hp.hpp; it
// was written from that document's spec rather than copied, which is the test
// the document exists for.  The contract is byte-identical output -- verify.sh
// holds it to the same archive -- so every ordering the spec calls load-bearing
// is preserved:
//
//   * contexts 1..9 are set from the PRE-restack word stack; the sentence-end
//     restack runs after them,
//   * col/frstchar are updated before contexts 12..15,
//   * the successor tables are written before they are read,
//   * the 46 cm.set() calls run unconditionally, in spec order -- the ordinal
//     both salts the hash and owns the mixer slots.
//
// Every tunable lives in IDX/paq8-W0.idx (prefix W0_); this file holds no
// numeric choice of its own.  The one renaming against the old code: the local
// `f` had two lives (sentence-end flag, then c4&0xffffff), and here they are
// two variables, `eos` and `c3`.  Same values, same stream, one less trap.
//
// Included from paq8hp.hpp after the charset macros and the shared feature
// bank (col, frstchar, spafdo, words, spaces, counts, nl, nl1) -- WordModel is
// the OWNER of those globals; SparseModel and mixer selectors 2/4/6 read them
// later in the same bit.

// The wide masks, reassembled from the byte lanes the .idx declares them as.
// HA..HF window h = w4<<6 (the 2-bit-per-byte class register at byte
// alignment, shifted twice more); TA..TD window the successor chains; XA..XD
// are the sparse views of x4/c4.  A lane is the register's own field, and it
// is as wide a pattern as IDX can spell (mapping::value is a signed int).
const U32 W0_HA_MASK = LANES4(W0_HA_M3, W0_HA_M2, W0_HA_M1, W0_HA_M0);
const U32 W0_HB_MASK = LANES4(W0_HB_M3, W0_HB_M2, W0_HB_M1, W0_HB_M0);
const U32 W0_HC_MASK = LANES4(W0_HC_M3, W0_HC_M2, W0_HC_M1, W0_HC_M0);
const U32 W0_HD_MASK = LANES4(W0_HD_M3, W0_HD_M2, W0_HD_M1, W0_HD_M0);
const U32 W0_HE_MASK = LANES4(W0_HE_M3, W0_HE_M2, W0_HE_M1, W0_HE_M0);
const U32 W0_HF_MASK = LANES4(W0_HF_M3, W0_HF_M2, W0_HF_M1, W0_HF_M0);
const U32 W0_F_MASK  = LANES4(W0_F_M3,  W0_F_M2,  W0_F_M1,  W0_F_M0);
const U32 W0_TA_MASK = LANES4(W0_TA_M3, W0_TA_M2, W0_TA_M1, W0_TA_M0);
const U32 W0_TB_MASK = LANES4(W0_TB_M3, W0_TB_M2, W0_TB_M1, W0_TB_M0);
const U32 W0_TC_MASK = LANES4(W0_TC_M3, W0_TC_M2, W0_TC_M1, W0_TC_M0);
const U32 W0_TD_MASK = LANES4(W0_TD_M3, W0_TD_M2, W0_TD_M1, W0_TD_M0);
const U32 W0_XA_MASK = LANES4(W0_XA_M3, W0_XA_M2, W0_XA_M1, W0_XA_M0);
const U32 W0_XB_MASK = LANES4(W0_XB_M3, W0_XB_M2, W0_XB_M1, W0_XB_M0);
const U32 W0_XC_MASK = LANES4(W0_XC_M3, W0_XC_M2, W0_XC_M1, W0_XC_M0);
const U32 W0_XD_MASK = LANES4(W0_XD_M3, W0_XD_M2, W0_XD_M1, W0_XD_M0);
const U32 W0_D_MASK  = U32(LANES2(W0_D_M1, W0_D_M0));
const U32 W0_F4_MASK = U32(LANES2(W0_F4_M1, W0_F4_M0));

template <int L> struct WordModel {

// The word hash stack: word0 accumulates the current word a byte at a time,
// word1..word4 hold the previous four words, each scaled by its own multiplier
// on the way down.  Those multipliers decide what two word histories must
// share before they collide in cm.
U32 word0, word1, word2, word3, word4;

// 46 contexts into one hashed map; L sizes nothing else here.
ContextMap<((U32)MEM(L)*16), W_CM_CTX> cm;

// positions of the last two newlines, for column geometry.  -2/-3 rather than
// 0 so the first line has no phantom newline at position 0; buf[] wraps by
// power-of-two mask, so the negative index of the first bytes is legal.
int nl1, nl;

// Successor tables (the indirect model): t1[b] holds the last four bytes seen
// after byte b, t2[bigram] the last two seen after that bigram.
U32 t1[256];
U16 t2[0x10000];

WordModel() : word0(0), word1(0), word2(0), word3(0), word4(0), nl1(-3), nl(-2) {
  memset(t1, 0, sizeof(t1));
  memset(t2, 0, sizeof(t2));
}

void mix(MainMixer &m) {
  if( bpos==0 ) {
    U32 c = b1, eos = 0;

    // -- window maintenance: 32-byte sliding windows with popcounts ---------
    if( spaces&0x80000000 )
      --spacecount;
    if( words&0x80000000 )
      --wordcount;
    spaces = spaces*2;
    words = words*2;

    // -- word/non-word ------------------------------------------------------
    // The class test and the fold both follow the charset: in WRT mode an
    // uppercase byte is a transform artefact and correctly ends a word, in
    // ASCII mode it folds into the word.  word0 accumulates the FOLDED byte.
    U32 wc = PAQ_WORDFOLD(c);
    if( (wc-U32(WL('a')))<=U32(WL('z')-WL('a'))||PAQ_WORDEXTRA(c, b2) ) {
      ++words, ++wordcount;
      word0 = word0*W0_WORD0_MUL+wc;
    } else {
      if( c==WL(' ')||c==WL('\n') ) {
        ++spaces, ++spacecount;
        if( c==WL('\n') )
          nl1 = nl, nl = pos-1;
      }
      if( word0 ) {
        // a word just ended: shift the stack, top-down
        word4 = word3*W0_WORD4_MUL;
        word3 = word2*W0_WORD3_MUL;
        word2 = word1*W0_WORD2_MUL;
        word1 = word0*W0_WORD1_MUL;
        word0 = 0;
        // spafdo counts word ENDS since the last sentence end, not bytes
        if( PAQ_EOS_WORD(c) )
          eos = 1, spafdo = 0;
        else {
          ++spafdo;
          spafdo = min(U32(W0_SPAFDO_CAP), spafdo);
        }
      }
    }

    // -- word contexts 1..9, from the pre-restack stack ---------------------
    U32 h = word0*W0_H_MUL+c;
    cm.set(word0);
    cm.set(h+word1);
    cm.set(word0*W0_P0_W0+word1*W0_P0_W1);
    cm.set(h+word1*W0_P1_W1+word2*W0_P1_W2);

    cm.set(h+word2);
    cm.set(h+word3);
    cm.set(h+word4);
    cm.set(h+word1*W0_P2_W1+word3*W0_P2_W3);
    cm.set(h+word2*W0_P3_W2+word3*W0_P3_W3);

    // -- sentence-end restack, AFTER contexts 1..9 --------------------------
    // The stack shifts a second time with its own multipliers and word1
    // becomes the '.' sentinel (a fixed point of the charset permutation, so
    // it spells the same in both modes).
    if( eos ) {
      word4 = word3*W0_FWORD4_MUL;
      word3 = word2*W0_FWORD3_MUL;
      word2 = word1*W0_FWORD2_MUL;
      word1 = '.';
    }

    // -- contexts 10..11 ----------------------------------------------------
    cm.set(W0_MakeBb34(b4, b3));
    cm.set(spafdo*W0_SPAFDO_MUL*((w4&W0_SPAFDO_W4M)==U32(W0_SPAFDO_EQ)));

    // -- line geometry, then contexts 12..15 --------------------------------
    col = min(W0_COL_CAP, pos-nl);
    if( col<=U32(W0_COL_TH) ) {
      if( col==U32(W0_COL_TH) )
        frstchar = min(c, U32(W0_FRST_CAP));
      else
        frstchar = 0;
    }
    // the wiki-link special: a line opening "[ " with "]" nearby folds to the
    // same bucket as the cap
    if( frstchar==U32(WL('['))&&c==WL(' ') ) {
      if( b3==U32(WL(']'))||b4==U32(WL(']')) )
        frstchar = W0_FRST_CAP;
    }
    cm.set(frstchar<<PSH(W0_FRST_SH)|c);

    // the byte at the same column of the previous line
    int above = buf[nl1+col];

    cm.set(W0_MakeCca(col, c, above));
    cm.set(W0_MakeCc(col, c));
    cm.set(col*(c==U32(W0_COLSP_EQ)));

    // -- whitespace statistics, contexts 16..21 -----------------------------
    h = W0_MakeWsp(wordcount, spacecount);
    cm.set(spaces&W0_SPACES_M1);
    cm.set(frstchar<<PSH(W0_FRST_SH2));
    cm.set(spaces&W0_SPACES_M0);
    cm.set(W0_MakeCsp(c, spacecount>>PSH(W0_SC_SH)));
    cm.set((c<<PSH(W0_C_SH13))+h);
    cm.set(h);

    // -- the w4 window, contexts 22..27 -------------------------------------
    // h is reused as the class register at byte alignment; each mask keeps a
    // different span of it, against the last 1, 2 or 3 bytes.
    U32 d = c4&W0_D_MASK;
    h = w4<<PSH(W0_H_SH);
    cm.set(c+(h&W0_HA_MASK));
    cm.set(c+(h&W0_HB_MASK));
    cm.set(c+(h&W0_HC_MASK));
    h <<= PSH(W0_H_SH);
    cm.set(d+(h&W0_HD_MASK));
    cm.set(d+(h&W0_HE_MASK));
    h <<= PSH(W0_H_SH);
    U32 c3 = c4&W0_F_MASK;
    cm.set(c3+(h&W0_HF_MASK));

    // -- successor tables, contexts 28..33 ----------------------------------
    // Write before read, and observably so: with b1==b2 the t1[c] read sees
    // the just-pushed byte, with b1==b2==b3 the t2[d] read does too.
    U16 &r2 = t2[c3>>8];        // c3>>8 == (b3,b2): the bigram before b1
    r2 = r2<<8|c;
    U32 &r1 = t1[d>>8];         // d>>8  == b2
    r1 = r1<<8|c;
    U32 t = c|t1[c]<<PSH(W0_T1_SH);
    cm.set(t&W0_TA_MASK);
    cm.set(t&W0_TB_MASK);
    cm.set(t);
    cm.set(t&W0_TC_MASK);
    t = d|t2[d]<<PSH(W0_T2_SH);
    cm.set(t&W0_TD_MASK);
    cm.set(t);

    // -- sparse views and tagged gapped bigrams, contexts 34..40 ------------
    cm.set(x4&W0_XA_MASK);
    cm.set(x4&W0_XB_MASK);
    cm.set(x4&W0_XC_MASK);
    cm.set(c4&W0_XD_MASK);
    cm.set(W0_MakeBp5(b5, c)+(W0_TAG_A<<PSH(W0_TAG_SH)));
    cm.set(W0_MakeBp6(b6, c)+(W0_TAG_B<<PSH(W0_TAG_SH)));
    cm.set(W0_MakeBp8(b8, b4)+(W0_TAG_C<<PSH(W0_TAG_SH)));

    // -- tail, contexts 41..46 ----------------------------------------------
    cm.set(d);
    cm.set(w4&W0_W4_M15);
    cm.set(f4);
    cm.set((w4&W0_W4_M63)*W0_W4_MUL+(W0_TAG_D<<PSH(W0_TAG_SH)));
    cm.set(d<<PSH(W0_D_SH9)|frstchar);
    cm.set((f4&W0_F4_MASK)<<PSH(W0_F4_SH11)|frstchar);
  }

  // Every bit: train on the bit just coded, advance the tree walk, and write
  // this map's 276 inputs into its slice of the layout.
  cm.mix(m, MIXB_W + W_cm);
}
};
