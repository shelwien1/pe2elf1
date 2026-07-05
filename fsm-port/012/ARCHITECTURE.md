# Architecture analysis: the corpus-driven x86 (dis)assembler

An analysis of how the opcode parser/generator is built today, where its costs
sit, and where it can get smaller, more data-driven, and cleaner — including
concrete `.p`-syntax changes. Numbers below are from the **x86-64** corpus
(`corpus64.p`) unless noted; the 32-bit numbers are similar in shape.

---

## 1. Orientation

### 1.1 The pipeline

```
corpus64.p ──gen.py──►  x86_tables.h       (decode: a flat byte-indexed FSM)
           └─gasm.py─►  x86_tables_enc.h    (encode: per-mnemonic candidate lists)

bytes ──run_fsm──► cap[]  ──finalize──► x86insn_t ──encode_insn──► bytes
        (x86dec.hpp)                                  (x86enc.hpp)
```

One declarative description (`corpus64.p`) is compiled **two ways** by two
Python programs that share a parser, so decode and encode can never disagree
about what a rule *means*. The C++ side is generic: it walks generated tables
and never hardcodes an opcode map.

### 1.2 File map (what actually lives where)

The request refers to "the FSM / decode / encode in `asm32.cpp`". Worth stating
the real layout, because the algorithms are **not** in `asm32.cpp`:

| File | Lines | Role |
|---|---:|---|
| `asm32.cpp` | 137 | thin file→file driver only (`d`ecode / `c` encode modes, listing) |
| `x86dec.hpp` | 1260 | **the decoder**: `run_fsm` + prefix/opcode/modrm driver + `finalize`/`vex_finalize`/`apx_finalize` |
| `x86enc.hpp` | 431 | **the encoder**: candidate selection + ModR/M/SIB/disp/imm emit |
| `x86insn.hpp` | 121 | the `x86insn_t` bijection record |
| `gen.py` | 2457 | corpus → decode FSM (`x86_tables.h`) |
| `gasm.py` | 323 | corpus → encode candidates (`x86_tables_enc.h`) |
| `corpus64.p` | 5589 | the instruction description (5027 rules) |

`parser.cpp` is the renderer/demo; `asm32.cpp` is the bijection tool. Both
`#include` the same `x86dec.hpp`/`x86enc.hpp`, so there is one decoder and one
encoder shared by every front-end.

---

## 2. The `.p` DSL

`corpus.p` is a **bit-pattern → string rewriter** (full reference in
`corpus-p-syntax.md`). The engine knows nothing about x86; every fact is in the
description. Four declaration kinds:

- **`arch`** — constants (`$mode`, `$endian`, `$bitorder`, `$maxlen`).
- **`vars`** — per-instruction mutable state, reset each instruction
  (`$opsiz`, `$adrsiz`, `$reptype`, `$rexw`…). Written by action blocks.
- **`table`** — positional arrays; the workhorse (register files, mnemonic
  variants, condition codes, size suffixes).
- **`submatch`** — named bit-pattern matchers; rules `pattern => template ;`
  tried top-to-bottom, first match wins.

The two pillars of the style:

1. **Size-paired register files are one table**, the wider half in the upper
   indices: `greg[32*$rexw + 16*$opsiz + 8*hi + lo]`. Emitting from the upper
   half is *itself* the signal that recovers the `66`/REX prefix on the way back
   out (§6.4 of the syntax ref). The register index is a clean bit-pack so the
   assembler's reverse solver can invert it.

2. **Every ModR/M operand is two rules** — a `mod=11` register rule with an
   inline table, and an `@addr` memory rule that always carries the size suffix.
   `@addr` is memory-only; the register case is its paired rule. This removes any
   "is it memory?" flag — but see §7.2, it is also the single biggest source of
   rule duplication.

VEX/EVEX/XOP/APX rules spell the **whole prefix bit-layout** inline, e.g.

```
h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa  0x58  11 ggg rrr  => wit("evex") "vaddps" …
```

The leading `h k b e 00 01 0 vvvv 1 00 z ll <b> u aaa` is the 3-byte EVEX prefix,
and it is repeated on **every** EVEX rule (see §7.2 for why this matters).

---

## 3. Decode algorithm

### 3.1 gen.py: simulate the corpus engine into a flat FSM

`gen.py` does not emit an interpreter for the DSL. It **runs** the corpus engine
symbolically over *every possible byte at every stage* and records the state the
engine would reach, materialising a flat table of `DState`s:

```c
struct DState { uint32_t next; struct Action act[7]; };   // 20 bytes
struct Action { op:3; dst:5; arg0:5; arg1:3; };           // 16 bits (packed union)
```

The `Fsm` is a struct of byte-indexed stages — `prefix[256]`, `op1[256]`,
`op2[256]`, `op3_38/3a[256]`, `modrm[2][256]`, `sib[3][256]`,
`groups[65][2][256]`, and the VEX/EVEX/XOP/APX product arrays
(`evexop[5][4][2][256]`, …). Each stage is a `DState[256]`: index by the next
stream byte, run that state's actions into a capture array, follow `.next`.

Actions are tiny: `FIELD` (extract bits → `cap[dst]`), `CONST`, `APPEND`
(pull N immediate bytes), `MARK` (prefix offset bookkeeping), `MNEM` (13-bit
mnemonic const).

### 3.2 run_fsm: the entire generic decoder (43 lines)

```c
while (base != FSM_HALT && ip < len) {
  DState* st = &FSM_AT(base + s[ip]);
  if (st->act[0].op==ACT_NONE && st->next==FSM_HALT) break;   // dead: not ours
  ip++;
  for each action: apply to cap[];
  base = st->next;
}
```

That is the whole architecture-free core. Everything else in `x86dec.hpp` is
(a) the **driver** that picks start tables in sequence
(prefixes → opcode → 0F/38/3A escape → ModR/M → immediate), and
(b) the **finalizers** that lower `cap[]` into `x86insn_t`.

### 3.3 The capture array

`cap[NCAPS]` with **NCAPS = 32** `uint64` slots. Vars and captures share one
index space (`VAR_OPSIZ=0 … CAP_REG, CAP_RM, CAP_MNEM, CAP_FORM, …`). The
VEX/EVEX path *aliases* onto slots the legacy path leaves unused (`vvvv→REL`,
`L→CC`, `R'X'B'→TBL3`), because no slot is live on both paths. That aliasing is
clever but it is also load-bearing tribal knowledge (see §7.4).

### 3.4 Lowering: three finalizers

- `finalize_insn` — legacy: places operands in semantic order per `CAP_FORM`,
  folds REX high bits into register numbers, applies default-64 sizing, reads the
  addressing witness, stamps `enc`.
- `vex_finalize` — VEX/EVEX vector: de-inverts R/X/B/vvvv/V', applies the
  writemask/zeroing/broadcast/rounding decorations, places operands per a
  `FORM_VEX_*` switch (~30 cases).
- `apx_finalize` — APX map-4 EVEX-promoted-legacy: folds r0-31, applies NDD/NF,
  handles CCMP/CTEST/CFCMOV/SETcc/IMULZU.

A handful of things the descriptor key can't carry are then fixed up by **8
mnemonic-morph reclassify blocks** and **19 hardcoded opcode-byte tests** in C++
(`movd→movq` under W, `pextrd→pextrq`, `wrssd→wrssq`, `vmovlps→vmovhlps` on the
reg form, `ldtilecfg→tilerelease`, the APX ND-selected trio, …). These are
correct but they are the manual residue (see §7.3).

---

## 4. Encode algorithm

### 4.1 gasm.py: invert per mnemonic

`gasm.py` reuses gen.py's rule parser (`parse_insn_lhs` / `insn_mnem` /
`operand_file`) and rebuilds each rule as an **`EncCand`** — opcode byte(s),
embedded-field program, operand-file signature, immediate kind — then buckets
candidates by mnemonic (contiguous, stable order). 1133 candidates across the
mnemonic space.

A hard invariant: **enc is 3 bits**, so at most 8 same-shape encodings may share
one mnemonic. gasm.py enforces this at build time and fails loudly; it has
already forced distinct-mnemonic hacks (`MOVQ_GPR`, `PEXTRQ`) to stay under the
cap (see §7.5).

### 4.2 enc_select + the witness

```c
c = matching_candidates(mnem, operand_shape)[in->enc];   // enc = which twin
emit prefix run (replayed verbatim from pfx[]);
emit opcode(+embedded reg/cc/digit), ModR/M-SIB-disp, immediate;
```

`enc` records **which** of several equal-cost encodings the original bytes used
(inc via `40+r` vs `FF /0`; `add eax,imm` via `05` vs `81 /0` vs `83 /0`), so a
decoded insn re-encodes to the identical bytes. The ModR/M/SIB/disp emitter is
otherwise canonical (shortest disp, SIB only when forced); non-canonical
addressing is reproduced from the `disp_w`/`sib`/`sscale` witness.

VEX/EVEX/XOP/APX encode is **byte-replay**: the prefix bytes and raw ModR/M were
snapshotted at decode (`vex1/2/3/op/modrm`), so `vex_encode` just replays them
and rebuilds only the SIB/disp tail from the addressing witness. The mnemonic is
irrelevant to the vector encode path — which is why decode-side operand
reordering can't perturb the bytes.

---

## 5. The bijection record (`x86insn_t`)

The endpoint that makes `c(d(bytes)) == bytes` hold for canonical **and**
non-canonical encodings. It carries semantic fields (mnem, operands, sizes) plus
an **encoding witness**:

- `enc:3` — opcode-twin choice (canonical = 0).
- `disp_w:2`, `sib:1`, `sscale:2` — addressing residue (wide-zero disp,
  gratuitous SIB, dead scale bits).
- `reg_w:4` — dead ModR/M.reg for single-r/m forms (non-canonical setcc bits).
- raw `pfx[]` + `vex1/2/3/op/modrm` — replayed verbatim.

All-zero witness ⇒ canonical, so a synthesized/edited insn just re-encodes
canonically. This is the right design; the only wrinkle is `enc` being a *rank*
rather than an opcode delta (§7.5).

---

## 6. Measured cost

### 6.1 Table sizes

| Artifact | Size |
|---|---:|
| `x86_tables.h` (64-bit) | 79 104 `DState` × 20 B = **1.51 MB**, 46% dead |
| `x86_tables.h` (32-bit) | 65 792 `DState` × 20 B = 1.25 MB, 50% dead |
| `x86_tables_enc.h` | 1133 `EncCand` ≈ 30 KB |

### 6.2 Where the dead states are (64-bit)

```
  groups      33280 states   33.0% dead   ← 44% of the whole table
  evexop      10240 states   89.6% dead
  evexp2      10240 states    0.0% dead
  vexop        8192 states   88.9% dead
  xopop        6144 states   98.7% dead
  apxop        2048 states   81.1% dead
  op3_3a        256 states   89.1% dead
  modrm         512 states    0.0% dead   ← shared, dense: the good case
  sib           768 states    0.0% dead
```

Two structural facts jump out:

1. **`groups` alone is 44% of the table** (65 groups × 2 adrsiz × 256 = 33 280
   states, ≈ 666 KB). A group only branches on the 3-bit reg `/digit` (× reg-vs-
   mem), yet each group re-materialises the *entire* ModR/M addressing decode —
   which is byte-identical to the shared `modrm[]` table. This is pure
   replication.

2. **The VEX/EVEX/XOP/APX product arrays are 85–99% dead** (~37 000 states).
   They are indexed by the full `(map, pp, W, opcode)` cross-product, but only a
   sparse set of coordinates is live.

### 6.3 Where the manual code is

| Manual surface | Count |
|---|---:|
| corpus rules total | 5027 |
| — legacy `insn`-body rules | 1337 |
| — VEX/EVEX/XOP/APX rules | ~3731 |
| — **EVEX submatch alone** | **2481** |
| reg-direct (`11 ggg rrr`) rules | 1593 |
| `@addr` sibling rules | 2130 |
| C++ mnemonic-morph reclassify blocks | 8 |
| C++ hardcoded `opcode == 0x..` tests | 19 |

---

## 7. Improvement opportunities

Grouped as the request framed them: **smaller structures**, **less
hand-written code**, **cleaner algorithms**, and **`.p` syntax changes**. Each
item notes rough payoff and risk.

### 7.1 Smaller structures

**(A) Collapse `groups` into shared-modrm + a `/digit` mnemonic table.**
*Payoff: ~44% of the table (~666 KB → a few KB). Risk: medium.*
Groups don't need their own addressing decode — it is identical to `modrm[]`.
Route a group opcode through the **shared** `modrm[adrsiz]` stage (as ordinary
ModR/M ops already do), and add a small `groupmnem[gid][8]` table the driver
indexes by the captured reg field. The `groups[65][2][256]` array disappears.
This is the single highest-value size change and it also *simplifies* the driver
(one addressing path instead of two).

**(B) Sparsify the VEX/EVEX/XOP/APX opcode arrays.**
*Payoff: ~37 000 → ~4 000 live states (~650 KB). Risk: medium.*
Replace the dense `[map][pp][W][256]` product with a compact keyed table: pack
`(map,pp,W,opcode)` into a `uint32` key and use a sorted array + binary search,
or a generated perfect hash. Vector decode is not hot relative to correctness, so
an O(log n) lookup is fine. `run_fsm` stays for the byte-sequential stages;
only the vector opcode dispatch changes.

**(C) Dedup `DState` action-lists into a pool; shrink `DState` to 8 bytes.**
*Payoff: ~2× on whatever survives (A)/(B). Risk: medium-high (touches run_fsm).*
Most states carry 1–2 actions, but every `DState` reserves 7 slots (14 of its 20
bytes). Thousands of states share identical action-lists (all the "capture
reg/rm/mode" modrm cells, all the "append imm8" cells). Intern action-lists into
a shared pool and make `DState = { uint32 next; uint16 act_off; uint16 act_len; }`
(8 bytes) indexing the pool. Combined with (A)+(B) this could bring the whole
table well under 500 KB.

**(D) Share the fully-dead row.**
*Payoff: small, but trivial. Risk: none.*
`prefix` is 89% dead, `op3_3a` 89%, `xopop` 99% — all pointing at the same
all-`NOACT`/`FSM_HALT` row. If any dense arrays remain after (A)/(B), emit one
canonical dead row and point dead cells at it (needs an indirection or a
generator that coalesces).

### 7.2 Less hand-written corpus

**(E) Auto-pair reg/mem: one rule instead of two.** ⭐
*Payoff: up to ~1593 rules removed. Risk: medium; the biggest ergonomics win.*
Today every ModR/M op is written twice — a `11 ggg rrr` register rule and an
`@addr` memory rule — differing only in whether the r/m renders from a register
table or from `$addr`. The engine already knows reg-vs-mem (`CAP_MODE`). Introduce
an `@rm(table)` operand that matches **both** mod=11 and mod≠11 and renders the
register from `table` in the reg case, `$addr` otherwise:

```
# today (two rules):
0x89 11 ggg rrr => "mov " greg[$r] "," greg[$g] ;
0x89 @addr      => "mov " $addr "," greg[$g] ;
# proposed (one rule):
0x89 @rm(greg[$r]) => "mov " $rm "," greg[$g] ;
```

The size-suffix-on-memory-only behaviour folds in (suffix emitted only in the
`@addr` branch). This halves the legacy ModR/M rule count and — applied to the
vector rules, which are almost all paired — is worth far more.

**(F) Factor the VEX/EVEX/XOP prefix preamble.** ⭐
*Payoff: the readability/2481-rule problem. Risk: low-medium.*
Every EVEX rule respells `h k b e 00 01 0 vvvv 1 00 z ll <b> u aaa`. gen.py's
`_vex_parse_rule` already *extracts* `(map, pp, W, opcode)` from those bits — the
raw layout is redundant with what the compiler recovers. Add a compact rule form
that declares the coordinates as fields instead of spelled bits:

```
# proposed EVEX shorthand — generator expands the prefix layout:
evex map=0F pp=NP  W=0 0x58 rvm(ereg,evvv,ereg) => "vaddps" … ;
evex map=0F pp=66  W=1 0x58 rvm(ereg,evvv,ereg) => "vaddpd" … ;
```

This does not shrink the *generated* table, but it removes the dominant source of
hand-written, error-prone boilerplate (2481 EVEX rules) and makes the corpus
diffable.

**(G) Bring element-suffix tables back to the vector side.**
*Payoff: collapses the ps/pd/ss/sd and d/q W-variants. Risk: low.*
The legacy side already selects the element suffix by table
(`elt[$reptype*2+$opsiz]`); the vector side abandoned this and writes `vaddps`,
`vaddpd`, `vaddss`, `vaddsd` as four separate rules. A `velt[pp,W]` table + one
rule per opcode restores the legacy economy. `build_vex` already auto-fills WIG
W-siblings — extend the same idea to *element-significant* W/pp instead of
enumerating them by hand.

Combining (E)+(F)+(G) plausibly takes the vector corpus from ~3700 rules to
several hundred, with the generated table unchanged (or smaller via §7.1).

### 7.3 Move C++ special-cases into corpus data

**(H) A "mnemonic selected by W / mod / ND" descriptor, like the pp selectors.**
*Payoff: removes ~8 reclassify blocks + several of the 19 opcode tests. Risk: medium.*
There are already three MNSEL mechanisms — mode 1 (opsize-add), mode 2 (ppvtab,
mandatory-prefix select), mode 3 (ppdesc, full per-prefix descriptor). The
remaining morphs (`movd→movq`/`pextrd→pextrq`/`wrssd→wrssq` on **W**;
`ldtilecfg→tilerelease`/`vmovlps→vmovhlps` on **mod**; the APX
`cfcmov`/`setcc`/`imulzu` trio on **EVEX.ND**) are the same "one slot, mnemonic
picked by a bit" pattern with a different selector. Generalise the descriptor to
carry a *selector axis* (pp | W | mod | ND) and a small value→mnemonic table, and
these stop being hand-written `if (mnem==X && cond) mnem=Y` in the decoder.

**(I) Generate the bank-selection logic once.**
*Payoff: removes a decode/encode drift risk. Risk: low.*
`file_to_T` (decode) and `enc_file_class` (encode) must agree bit-for-bit on the
SSE mm/xmm bank rule (`(has66||reptype)?XMM:MM`), and today they are two
hand-kept copies with warning comments. Emit the operand-file → class mapping
from one source (the interp already owns `OPF`), so the two sides can't drift.

### 7.4 Cleaner algorithms

**(J) Unify the three finalizers behind one form-driven operand placer.**
*Payoff: shrinks `x86dec.hpp` materially; one place for operand order. Risk: medium.*
`finalize_insn`, `vex_finalize`, and `apx_finalize` each contain a large
`FORM_* → op[]` switch with overlapping logic. The form → (operand role
sequence) mapping is conceptually a table (`RVM = [reg, vvvv, rm]`, `MR = [rm,
reg]`, …). Drive all three from one role-sequence table plus per-family operand
builders (`vex_mkop`, `apx_gpr`, legacy `SETREG/SETRM`). The ~30-case VEX switch
and the APX switch collapse into data + a short loop.

**(K) Make `enc` an opcode delta, not a bucket rank.**
*Payoff: removes the 8-encoding cap and the distinct-mnemonic hacks. Risk: medium.*
`enc:3` counts "which matching candidate," which couples the witness to how
candidates bucket by mnemonic and caps same-shape encodings at 8. Storing a small
**opcode/twin descriptor** (or widening to `enc:4/5` and keying on the actual
opcode choice) lets `MOVQ_GPR`/`PEXTRQ` go back to being `movq`/`pextrq` selected
by W (see (H)), instead of distinct indices invented to dodge the cap.

**(L) Single rule-IR shared by both generators.**
*Payoff: kills the parallel decode/encode derivation. Risk: medium-high.*
`EncBuilder.add_rule` is a near-mirror of `build_insn`'s per-rule attribute
derivation; the invariant "both interpret a rule identically" is currently held
by careful mirroring, not by construction. Factor one pass that compiles a rule
into a neutral IR `(opcode, form, files, imk, emb, dir, group/digit, …)`, and have
the decode-FSM emitter and the encode-candidate emitter both consume it. This is
the structural fix that makes drift impossible rather than merely tested-against.

**(M) Fold prefix-effectiveness into the FSM prefix stage.**
*Payoff: removes delicate imperative recompute in `decode_insn`. Risk: medium.*
The "last REX wins / recompute `VAR_OPSIZ` from the effective REX" logic and the
VEX-can't-follow-66/F2/F3/REX checks are hand-written scans over `pfx[]`. A
"last-writer-wins" prefix stage (the FSM already visits every prefix byte) could
carry the effective-prefix state directly, shrinking the C++ prologue.

### 7.5 Concrete `.p` syntax proposals (summary)

| Proposal | Replaces | Removes |
|---|---|---|
| `@rm(regtable)` — reg+mem in one rule (E) | paired `11 ggg rrr` / `@addr` rules | ~1593 legacy + most vector pairs |
| `evex map= pp= W= op rvm(...)` shorthand (F) | inline 3-byte prefix bit spelling | the 2481-rule EVEX boilerplate |
| `velt[pp,W]` element-suffix tables (G) | 4× ps/pd/ss/sd rules per op | ~3× vector rule fan-out |
| descriptor `select=W\|mod\|ND` axis (H) | C++ reclassify blocks | 8 morphs + several opcode tests |

None of these change the *generated* tables' semantics; they change how the
description is written and (for §7.1) how it is stored.

---

## 8. Suggested order of work

Sequenced by payoff-to-risk, each independently shippable behind the existing
gate (`fuzz64` 5M, `roundtrip64` byte-exact, Zydis differential, 32-bit
858/858 + x8632all):

1. **(A) collapse `groups`** — biggest size win (~44%), also simplifies the
   driver. Self-contained: generator + one driver path.
2. **(E) `@rm` auto-pairing** — biggest corpus-ergonomics win; halves the
   hand-written ModR/M rules. Generator + a template feature.
3. **(F)+(G) vector rule shorthand + element tables** — attacks the 2481-rule
   EVEX mass; table-shape unchanged, so low validation risk.
4. **(B) sparsify vector opcode arrays** — the second big size win.
5. **(H)+(K) selector-axis descriptor + opcode-delta `enc`** — retires the C++
   morphs and the distinct-mnemonic hacks together.
6. **(L) single rule-IR** — the durable fix for decode/encode drift.
7. **(C)+(J)+(D)+(I)+(M)** — the remaining size/cleanliness polish.

Everything above is refactoring: the semantics are pinned by the round-trip
gate, so each step is verifiable byte-for-byte before the next.
