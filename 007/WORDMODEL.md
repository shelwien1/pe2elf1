# WordModel — complete description

A specification of `WordModel` as it stands in `paq8hp.hpp`, precise enough to
recreate the class from this document alone. Everything here is read from the
source; where behaviour depends on evaluation *order*, the order is stated,
because several of them are load-bearing for bit-exactness.

WordModel is the largest submodel: 46 contexts into one hashed `ContextMap`,
contributing 276 of the mixer's 466 inputs (~60% of the model's memory traffic).
It models words, word n-grams, line/column geometry, whitespace statistics,
byte-successor ("indirect") contexts, and sparse byte views — everything about
text that is not plain byte context.

---

## 1. Declaration and interface

```c
template <int L> struct WordModel {
  U32 word0, word1, word2, word3, word4;      // the word hash stack
  ContextMap<((U32)MEM(L)*16), W_CM_CTX> cm;  // W_CM_CTX = 46; MEM(L) = 2^(16+L)
  int nl1, nl;                                 // positions of the last two '\n'
  U32 t1[256];                                 // successor table, byte    -> last 4 bytes after it
  U16 t2[0x10000];                             // successor table, bigram  -> last 2 bytes after it

  WordModel();                 // words 0, nl1 = -3, nl = -2, t1/t2 zeroed
  void mix(MainMixer &m);      // called once per BIT
};
```

* `L` is the memory level; the only thing it sizes is `cm`
  (16·2^(16+L) bytes — 512 MiB at L=5, 2 GiB at L=11).
* `mix()` is called once per bit by `ContextModel::p()`, after the core
  ContextMap and before SparseModel. All context selection happens inside the
  `if (bpos==0)` block, i.e. once per **byte**; on the other seven bits only
  `cm.mix(m, MIXB_W + W_cm)` runs, serving the tree walk (§7).
* There is no separate update entry point: `cm.mix()` both trains on the bit
  that was just coded and emits the next bit's inputs (the codec-wide
  predict/update fusion).

## 2. Inputs

### 2.1 Global feature bank (read)

| variable | meaning |
|---|---|
| `bpos` | bit position in the current byte, 0..7 |
| `b1..b8` | last 1..8 bytes (b1 most recent). **`b2` may have been rewritten to `WL('.')`** by `Predictor::update` after a sentence-end byte other than `!` |
| `c4` | last 4 bytes packed big-to-little: `(b4<<24)\|(b3<<16)\|(b2<<8)\|b1` |
| `x4` | like `c4` but maintained in `ContextModel::p` with phantom punctuation injection (an isolated sentence-end byte is pushed twice) |
| `w4` | 2 bits per byte: word-class of each byte's high nibble via `WRT_mpw[]`; sentence-enders OR in 12 |
| `f4` | 4 bits per byte: high nibble per byte (space remapped 32→31 first) |
| `pos` | bytes coded so far; `buf[]` is the history ring (`buf[i]` wraps by power-of-two mask, so the negative `nl1+col` of the first bytes reads harmlessly from the end of the ring) |
| `y` | the previous coded bit (consumed inside `cm.mix`) |

### 2.2 Global feature bank (read *and written* — WordModel is the owner)

`words`, `spaces` (32-bit shift registers, 1 bit/byte), `wordcount`,
`spacecount` (popcounts of those windows), `col`, `frstchar`, `spafdo`,
`nl`, `nl1`. SparseModel and mixer selectors 2/4/6 read these **after**
WordModel::mix has run in the same bit — call order is part of the contract.

### 2.3 Charset macros

All character tests go through the `WL()` layer (`PAQ_CHARSET_WRT`):

* `WL(ch)` — the byte that means `ch`: identity in ASCII mode, `wrt_remap(ch)`
  in WRT mode.
* `PAQ_WORDFOLD(c)` — identity (WRT) or A–Z→a–z fold (ASCII).
* `PAQ_WORDEXTRA(c,prev)` — extra word-characters: `c==8||c==6||(c>127&&prev!=12)`
  (WRT) or `c>127` (ASCII).
* `PAQ_EOS_WORD(c)` — sentence-end set used *here*: `{. ? }}` in ASCII spelling
  (three members — deliberately smaller than the six-member `PAQ_EOS_BYTE` used
  elsewhere).

### 2.4 Tunable constants (IDX/paq8-W0.idx, prefix `W0_`)

Seeded values (= shipped behaviour):

| group | name = value |
|---|---|
| word chain | `WORD0_MUL`=2104 (263·8), `WORD1_MUL`=83, `WORD2_MUL`=53, `WORD3_MUL`=47, `WORD4_MUL`=43 |
| sentence-end chain | `FWORD2_MUL`=41, `FWORD3_MUL`=37, `FWORD4_MUL`=31 |
| pair hashes | `H_MUL`=271; `P0_W0`=91, `P0_W1`=89; `P1_W1`=79, `P1_W2`=71; `P2_W1`=73, `P2_W3`=61; `P3_W2`=67, `P3_W3`=59 |
| line geometry | `COL_CAP`=31, `COL_TH`=2, `FRST_CAP`=96, `COLSP_EQ`=32 (' ') |
| spafdo | `SPAFDO_CAP`=63, `SPAFDO_MUL`=8, `SPAFDO_W4M`=3, `SPAFDO_EQ`=1 |
| whitespace | `SPACES_M1`=0x7fff, `SPACES_M0`=0xff, `SC_SH`=1, `C_SH13`=13 |
| shifts | `FRST_SH`=11, `FRST_SH2`=7, `H_SH`=6, `T1_SH`=8, `T2_SH`=16, `D_SH9`=9, `F4_SH11`=11, `TAG_SH`=17 |
| w4-window masks (LANES4 of byte lanes) | `HA`=0xffffff00, `HB`=0x00ffff00, `HC`=0x0000ff00, `HD`=0xffff0000, `HE`=0x00ff0000, `HF`=0xff000000 |
| byte masks | `D_MASK`=0xffff, `F_MASK`=0xffffff, `F4_MASK`=0xffff |
| successor masks | `TA`=0xffff, `TB`=0xffffff, `TC`=0xff00, `TD`=0xffffff |
| sparse views | `XA`=0x00ff00ff, `XB`=0xff0000ff, `XC`=0x00ffff00, `XD`=0xff00ff00 |
| tags | `TAG_A`=1, `TAG_B`=2, `TAG_C`=4, `TAG_D`=5 |
| tail | `W4_M15`=15, `W4_M63`=63, `W4_MUL`=128 |

Tight packings are IDX Indexes with generated builders:
`MakeBb34(b4,b3)` = `b4<<8|b3`; `MakeCca(col,c,above)` = `(col&63)<<16|c<<8|above`;
`MakeCc(col,c)` = `(col&63)<<8|c`; `MakeCsp(c,sc)` = `c*64+sc`;
`MakeWsp(wc,sc)` = `wc*64+sc`; `MakeBp5(b5,c)` = `b5<<8|c`;
`MakeBp6(b6,c)` = `b6<<8|c`; `MakeBp8(b8,b4)` = `b8<<8|b4`.
Every shift constant passes through `PSH()` = clamp to [0,31] at the point of
use.

## 3. Outputs

1. **276 mixer inputs**, written to slots `MIXB_W+W_cm .. +275` (mix_layout.inc);
   6 per context: the run input, then `mix2`'s five terms (`st/4`,
   `(p1−p0)·3/64`, `st(n1−n0)·3/16`, and two one-sided deterministic-history
   terms). Produced by `cm.mix`, not by WordModel itself.
2. **The file-scope variables of §2.2**, consumed downstream in the same bit.
3. `cm.mix` returns the count of contexts with nonzero state; WordModel's
   caller ignores it (only the core map's return becomes `order`).

## 4. Per-byte algorithm (bpos==0), in exact order

Let `c = b1` (the byte just completed), `f = 0` (a flag, **reused later as a
hash** — keep one variable if you want identical code shape).

**Step 1 — window maintenance.** If bit 31 of `spaces` set, `--spacecount`;
same for `words`/`wordcount`. Then `spaces <<= 1`, `words <<= 1` (a 32-byte
sliding window with popcount).

**Step 2 — word/non-word.** `wc = PAQ_WORDFOLD(c)`. If
`wc` ∈ [`WL('a')`,`WL('z')`] or `PAQ_WORDEXTRA(c,b2)`:

```
++words; ++wordcount;  word0 = word0*WORD0_MUL + wc;
```

Otherwise (non-word byte):
* if `c` is `WL(' ')` or `WL('\n')`: `++spaces; ++spacecount`; on newline
  `nl1 = nl, nl = pos-1`.
* if `word0 != 0` (a word just ended) the stack shifts —
  `word4=word3*WORD4_MUL; word3=word2*WORD3_MUL; word2=word1*WORD2_MUL;
  word1=word0*WORD1_MUL; word0=0` — and then: if `PAQ_EOS_WORD(c)`, set
  `f=1, spafdo=0`; else `spafdo = min(SPAFDO_CAP, spafdo+1)`.

**Step 3 — word contexts** (contexts 1–9). `h = word0*H_MUL + c`:

| # | context |
|---|---|
| 1 | `word0` |
| 2 | `h + word1` |
| 3 | `word0*P0_W0 + word1*P0_W1` |
| 4 | `h + word1*P1_W1 + word2*P1_W2` |
| 5 | `h + word2` |
| 6 | `h + word3` |
| 7 | `h + word4` |
| 8 | `h + word1*P2_W1 + word3*P2_W3` |
| 9 | `h + word2*P3_W2 + word3*P3_W3` |

**Step 4 — sentence-end restack.** If `f` (set in step 2): the stack shifts
*again* with the F-multipliers — `word4=word3*FWORD4_MUL;
word3=word2*FWORD3_MUL; word2=word1*FWORD2_MUL; word1='.'` (the literal 0x2E
sentinel; fixed under both charsets since `wrt_remap('.')=='.'`). Note this
happens **after** contexts 1–9 were set from the pre-restack values.

**Step 5 — contexts 10–11.**
10: `MakeBb34(b4,b3)`.
11: `spafdo * SPAFDO_MUL * ((w4&SPAFDO_W4M)==SPAFDO_EQ)`.

**Step 6 — line geometry, then contexts 12–15.**
`col = min(COL_CAP, pos-nl)`. If `col<=COL_TH`: `frstchar = (col==COL_TH) ?
min(c,FRST_CAP) : 0`. Wiki-link special: if `frstchar==WL('[')` and
`c==WL(' ')` and (`b3==WL(']')` or `b4==WL(']')`), `frstchar = FRST_CAP`.
`above = buf[nl1+col]` (byte at the same column of the previous line).

12: `frstchar<<FRST_SH | c` 13: `MakeCca(col,c,above)`
14: `MakeCc(col,c)` 15: `col * (c==COLSP_EQ)`

**Step 7 — whitespace statistics, contexts 16–21.**
`h = MakeWsp(wordcount, spacecount)` (reusing `h`):

16: `spaces & SPACES_M1` 17: `frstchar<<FRST_SH2`
18: `spaces & SPACES_M0` 19: `MakeCsp(c, spacecount>>SC_SH)`
20: `(c<<C_SH13) + h` 21: `h`

**Step 8 — the w4 window, contexts 22–27.**
`d = c4 & D_MASK` (last 2 bytes); `h = w4<<H_SH` (reusing `h` again — the
2-bit-per-byte class register brought up to byte alignment):

22: `c + (h & HA_MASK)` 23: `c + (h & HB_MASK)` 24: `c + (h & HC_MASK)`
then `h <<= H_SH`:
25: `d + (h & HD_MASK)` 26: `d + (h & HE_MASK)`
then `h <<= H_SH` and `f = c4 & F_MASK` (— `f` is now the last-3-bytes hash):
27: `f + (h & HF_MASK)`

**Step 9 — successor tables, contexts 28–33.** Update **before** read:

```
t2[f>>8] = t2[f>>8]<<8 | c;    // f>>8 = (b3,b2): bytes after that bigram
t1[d>>8] = t1[d>>8]<<8 | c;    // d>>8 = b2:      bytes after that byte
```

(If `b1==b2`, the subsequent `t1[c]` read sees the just-pushed byte; keep the
write-then-read order.) Then `t = c | t1[c]<<T1_SH`:

28: `t & TA_MASK` 29: `t & TB_MASK` 30: `t` 31: `t & TC_MASK`
then `t = d | t2[d]<<T2_SH`:
32: `t & TD_MASK` 33: `t`

**Step 10 — sparse views and gapped bigrams, contexts 34–40.**

34: `x4 & XA_MASK` 35: `x4 & XB_MASK` 36: `x4 & XC_MASK` 37: `c4 & XD_MASK`
38: `MakeBp5(b5,c) + (TAG_A<<TAG_SH)` 39: `MakeBp6(b6,c) + (TAG_B<<TAG_SH)`
40: `MakeBp8(b8,b4) + (TAG_C<<TAG_SH)`

**Step 11 — tail, contexts 41–46.**

41: `d` 42: `w4 & W4_M15` 43: `f4`
44: `(w4 & W4_M63)*W4_MUL + (TAG_D<<TAG_SH)`
45: `d<<D_SH9 | frstchar` 46: `(f4 & F4_MASK)<<F4_SH11 | frstchar`

Context order **is** the layout: context *i* owns mixer slots
`MIXB_W + (i−1)*6 .. +5`, and `ContextMap::set` also salts the hash with the
context's ordinal, so reordering changes the bitstream twice over.

## 5. What `cm` does with each context (summary of the consumer contract)

`cm.set(cx)` scrambles: `cx = cx*123456791 + i` (i = ordinal), rotate 16, `*987654323 + i`,
stores it. Per byte each context probes up to 3 hash lines (at bpos 0, 2, 5:
bucket `(cxt+c0) & Sz`) of 64 B: 7 ways × (16-bit checksum + 7-byte bit-history
tree). Per bit it emits the 6 inputs of §3 and advances the depth-3 tree walk;
the in-line run model (slots [3],[4] of the bpos-0 line) supplies the run input
with the parity trick (odd count = "ever contradicted", gain 15/4 vs 13).
During the **first byte** of the stream `cm` has been handed no contexts
(`cn==0`) and emits nothing — WordModel's 276 slots stay zero, which is inert
by construction.

## 6. Recreation checklist (the parts that are easy to get wrong)

1. **Order**: contexts 1–9 before the sentence-end restack; `col`/`frstchar`
   updated before contexts 12–15; `t1`/`t2` written before read; window
   maintenance before the word test.
2. **`f` is one variable** with two lives: flag (steps 2–4), then `c4&0xffffff`
   (steps 8–9). The `t2` index `f>>8` depends on the second life.
3. `word1 = '.'` after a sentence end — a sentinel, not a hash product.
4. `b2` may already be `'.'`-rewritten by the byte-state update *before*
   WordModel runs; the word test's `PAQ_WORDEXTRA(c,b2)` sees the rewritten
   value.
5. `nl1` starts at −3: `buf[nl1+col]` relies on `buf[]`'s power-of-two wrap.
6. `spafdo` increments only when a word *ends* on a non-sentence-end byte —
   it counts word ends since the last sentence end, not bytes.
7. All 46 `set()` calls run unconditionally every byte; the count must equal
   `W_CM_CTX` (checked by `mix_ctxcheck`).
8. WordModel uses the three-member sentence-end set; the byte-state update
   uses the six-member one. They are different on purpose (§10.7 of the model
   analysis) — do not unify them silently.
