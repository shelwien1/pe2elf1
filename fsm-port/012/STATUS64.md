# x86-64 corpus-driven (dis)assembler — project status

*Branch `claude/brave-hamilton-d0m7rt`, 2026-07-02. Reference disassembler: GNU objdump 2.42
(`-D -b binary -m i386:x86-64 -M intel`; this build decodes APX map 4). Companion docs:
`plan.md` (the original x86-32→64 port plan), `corpus-p-syntax.md`, `FULL_DECODE.md`,
`FULL_ENCODE.md`.*

## 1. Goal

`asm32.cpp` as a file-to-file tool with a **lossless byte-exact round-trip**
(`c(d(bytes)) == bytes`, canonical *and* non-canonical encodings) over the **complete
x86-64 instruction set up to and including APX**, where:

* the instruction-set description lives in **`corpus64.p`** (one rule per encoding form:
  bit layout ⇒ mnemonic + operand template),
* **`gen.py`** compiles it into the decode FSM (`x86_tables.h`) and **`gasm.py`** into the
  encode tables (`x86_tables_enc.h`),
* the C++ is a thin generic layer — the FSM driver, the ModR/M/SIB/displacement handler,
  and capture→operand folding — with **no per-instruction opcode knowledge** hardcoded
  (the ModR/M handler is the sanctioned exception).

## 2. Architecture

```
corpus64.p ──gen.py──▶  x86_tables.h      (decode FSM: 61,440 states, 960 KiB compiled)
corpus64.p ──gasm.py─▶  x86_tables_enc.h  (encode: 1,055 candidates / 1,395 mnemonics)
                              │
   x86insn.hpp (the record) ──┼── x86dec.hpp (run_fsm + generic finalizers)
                              └── x86enc.hpp (encoder: candidates + byte-replay)
   asm32.cpp  (tool: `d` bytes→records, `c` records→bytes)      fuzz.cpp (bijection fuzzer)
```

* **Decode**: prefix run (FSM) → opcode dispatch (FSM) → for legacy: ModR/M stage (FSM,
  generated per address size) → `fill_insn`/`finalize_insn` fold captures into `x86insn_t`.
  For prefixed encodings the FSM has capture stages — `vexp1/vexp2` (C4), `vexp1c5` (C5),
  `evexp0/p1/p2` (62), `xopp1/p2` (8F), `apxp1/p2` (62 map 4) — that record R/X/B/vvvv/L'L/
  z/b/aaa and route to a per-(map,pp,W) **opcode-cell block** whose cell bakes
  MNEM/FORM/register-files/imm-kind; then the ordinary ModR/M stage runs and
  `vex_finalize`/`apx_finalize` lower the captures into operands.
* **Encode**: legacy instructions re-encode **semantically** from generated per-mnemonic
  candidate tables (an `enc` rank preserves which of several encodings was decoded, so
  non-canonical forms survive). VEX/EVEX/XOP/APX re-encode by **replaying the captured
  prefix+opcode bytes verbatim** (`vex1/vex2/vex3/vex_op`, raw ModR/M for reg-direct,
  address-witness rebuild for memory) — decode-side display choices can never break the
  byte round-trip.
* The same sources build both modes: `ARCH_MODE` gates the C++; the **committed**
  `x86_tables*.h` are the 32-bit default, and `make asm64 / fuzz64` regenerate the 64-bit
  tables, build, then restore the 32-bit ones.

### Size inventory

| component | lines | notes |
|---|---:|---|
| `corpus64.p` | 4,254 | **3,764 rules**: legacy+helpers 1,304 · vex 847 + vex2 185 · evex 1,016 · xop 101 · apx 311 |
| `gen.py` | 2,265 | FSM compiler (decode) |
| `gasm.py` | 317 | encode-table compiler |
| `x86dec.hpp` | 950 | FSM driver + generic finalizers |
| `x86enc.hpp` | 416 | encoder |
| `x86insn.hpp` / `asm32.cpp` / `fuzz.cpp` | 115 / 137 / 112 | record / tool / fuzzer |
| 64-bit tables | 6.1 MB + 109 KB src | FSM 61,440 DStates (93.8 % of `uint16_t`), 52 groups, 960 KiB compiled |
| 32-bit tables (committed) | 5.6 MB + 88 KB src | 54,784 DStates (83.6 %), 39 groups, 856 KiB |

## 3. Coverage status

Methodology: exhaustive **differential sweeps** against objdump over every
(map, pp, W, L, opcode) — and for groups every ModR/M `/digit`, reg-direct + memory. Each
test case is padded to 16 bytes with `0x90` and the sweep asserts **zero alignment drift**
(an earlier `0x00` padding let objdump resynchronize across case boundaries and silently
masked real gaps — the XOP GPR hole below was found only after fixing this; every number
here is from a drift-0 sweep).

| ISA area | status | evidence |
|---|---|---|
| Legacy 1-byte + 0F + 0F38 + 0F3A maps (integer, system, string, seg, ctrl) | **complete** | full-map differential sweeps, mnemonic+operand clean |
| x87 (D8–DF, all reg-direct specials) | **complete** | full-map sweep |
| SSE–SSE4.2, SSE4a, 3DNow!, MPX, SHA-NI, AES-NI, AES-KL, GFNI, CRC32/MOVBE/ADX, CET, RAO-INT, enqcmd/movdiri/movdir64b, VMX/SMX oddities | **complete** | full-map sweeps; per-prefix descriptors (`ppdesc`), pp-variant groups, fixed-ModR/M cells |
| VEX maps 1–3 (AVX/AVX2, FMA3/FMA4, BMI1/2, VNNI/-INT8, NE-CONVERT, SM3/SM4, AMX, vector AES/GFNI) | **complete — 0 gaps** | 12,288-case sweep; last hole (vmovhlps/vmovlhps reg-form) closed via a decode-time mnemonic swap |
| XOP maps 8–10 (vector SIMD **and** GPR: TBM, LWP, XOP-bextr) | **complete — 0 gaps** | 12,288-case sweep + 160-case all-`/digit` group sweep; GPR families added this iteration via two new group forms (`VEX_MG`, `VEX_VMG0`) + `IMK_IMM32` |
| EVEX maps 1–3 (AVX-512 core) | **317 / 521 objdump mnemonics (~61 %)** | sweep both W, reg+mem; 222 genuinely missing (§6), ~1 display alias (vpclmul imm-variants) |
| EVEX maps 5–6 (AVX512-FP16) | **not routed — 0 / 88** | measured today; `evexp0` sends maps 5/6 to the structural path (§7 state-budget note) |
| APX: EVEX-promoted legacy (map 4, incl. ND/NF, push2/pop2, groups) + REX2 | **complete — 0 missing** | 16,384-case sweep today (pp×W×ND×opcode×reg/mem): drift 0, missing 0 |

Everything the sweeps flag as *not* covered decodes through the **structural fallback**:
the prefix/opcode/ModR/M skeleton is captured raw, displayed as a `vex` placeholder, and
still round-trips byte-exactly — so completeness gaps are display-semantic gaps, never
round-trip gaps.

## 4. Round-trip / bijection status — all green

| check | result |
|---|---|
| `make fuzz64` — 5,000,000 random buffers, 64-bit | 4,072,160 accepted (81.4 %), **0 legacy failures, 0 vex/evex/xop failures** |
| `make fuzz` — 5,000,000, 32-bit regression | 4,436,652 accepted (88.7 %), **0 + 0 failures** |
| `make roundtrip64` — corpus64.bin | byte-identical (279 insns / 1,115 B) |
| `make roundtrip` — 32-bit corpus.bin | 858/858 byte-identical |
| `make prog64 … prog64f` — real `gcc -O2/-O3` output (int, BMI/CET, SSE2 scalar+packed, AVX-512, x87) | byte-identical |
| `make noncanon` — curated non-canonical encodings | byte-identical |
| `make x8632all` — 32-bit all-instruction corpus | 110,072 B / 18,836 insns byte-identical |

The fuzzer property is the strong one: **every accepted decode — semantic or structural
placeholder — must re-encode to the identical bytes.** All ISA-completion work happens
under this invariant (each batch: regenerate → fuzz64 + roundtrip64 → commit).

## 5. FSM-native vs C++ — audit against the stated ideal

### Corpus/FSM-driven (where the knowledge lives today)

* All opcode dispatch, prefix handling, ModR/M/SIB/disp parsing (generated per address
  size), operand register files, immediate kinds/widths, mnemonic text.
* All per-instruction VEX/EVEX/XOP/APX facts: the opcode-cell tables (mnemonic, operand
  FORM, reg/mem register files, imm kind) are compiled by `gen.py` **from corpus rules** —
  adding an AVX-512 instruction is corpus-only work, no C++ or generator change (the four
  EVEX batches landed this iteration — FMA 50, BW-integer 31, shifts 12, crypto 8 — touched
  only `corpus64.p`).
* Group mechanisms (`/digit` mnemonic tables `vexgrp`, pp-variant groups `ppgroup`,
  per-prefix full descriptors `ppdesc`, fixed-ModR/M display cells, display-alias `~tag`,
  3DNow suffix opcodes, opsize-selected mnemonics).

### Generic C++ (by-design, instruction-agnostic)

* `run_fsm` driver; `fill_insn`/`finalize_insn` (legacy capture folding, REX/opsize
  folding); `vex_finalize`/`apx_finalize` — a switch over ~25 operand **shapes**
  (`FORM_VEX_RVM`, `FORM_APX_MR`, …) shared by hundreds of instructions; EVEX decoration
  lowering (mask/zero/broadcast/rounding); 16/32-register extension folding; `enc_mem*`
  ModR/M/SIB emission (the sanctioned exception).

### Deviation inventory — per-instruction knowledge still in C++ (decode)

Nine localized cases, all operand-shape quirks that the descriptor model cannot express
(each is data-adjacent: the *mnemonic indices* come from generated `MNEM_*` defines):

1. `vmovlps→vmovhlps` / `vmovhps→vmovlhps` reg-form swap (VEX 0F 12/16 NP; descriptor is
   per-opcode, reg-vs-mem known only after ModR/M).
2. APX map-4 `F8`: `uwrmsr/urdmsr` (reg) ↔ `enqcmds/enqcmd` (mem) swap.
3. APX `shld/shrd` (A5/AD): implicit `cl` third operand.
4. APX `sha256rnds2` (DB): implicit `<xmm0>`.
5. APX `push2/pop2` (FF `/6`, 8F `/0`): 64-bit register pair, ND-bit-as-marker.
6. APX shift groups D0–D3: implicit `1` / `cl` count.
7. `mov cr/dr` (0F 20–23): raw-ModR/M capture channel (hardware ignores mod).
8. 8F POP-vs-XOP split (map ≥ 8) — inherently outside a byte-driven FSM.
9. 32-bit-mode C4/C5/62 VEX-vs-LES/LDS/BOUND disambiguation (mod == 11).

### Deviation inventory — encode side

* **`vex_structure()`** in `x86enc.hpp` — **resolved (2026-07-02).** The byte-replay tail
  layout (has-ModR/M? + imm length for VEX/EVEX/XOP/APX) is now a lookup into a generated
  `vextail[7][256]` table (gen.py), compiled from the corpus cells: seeded with the per-map
  defaults the structural fallback needs for uncovered opcodes, then overridden with the
  cell-exact `(has_modrm, imm)` for every covered opcode, with a conflict detector. An
  exhaustive new-vs-old check over every (map, opcode, opsize) confirmed 0 differences.
  The encoder now has **no** hardcoded opcode table; adding an immediate-bearing VEX/EVEX
  instruction to the corpus updates the tail automatically.
* Byte-replay itself is a design choice, not a gap: the encoder never *synthesizes* a
  VEX/EVEX prefix, it replays the decoded one. That is exactly what a lossless round-trip
  wants (it preserves non-canonical prefix choices for free). It does mean the tool is a
  re-assembler of decoded records, not a from-mnemonic assembler for prefixed encodings —
  consistent with the project goal.

## 6. Remaining work — EVEX maps 1–3 (222 mnemonics)

Grouped by family, from today's sweep (each family is one or a few corpus-rule batches;
the generator-with-self-verification workflow — build the EVEX bytes, confirm objdump's
mnemonic *before* emitting the rule — is established and caught a real pp-assignment error
in the FMA batch before it landed):

| family | count | examples / notes |
|---|---:|---|
| transcendental / range / fpclass | 32 | `vgetexp*`, `vgetmant*`, `vreduce*`, `vrndscale*`, `vrange*`, `vrcp28/vrsqrt28`, `vfpclass*` (kreg dest, imm8, sae) |
| insert / shuffle / mask-mov | 32 | `vinsert*x*` (eregx/eregh lanes), `vshuf*`, `vpshuf*`, `vpmov{b,d,q,w}2m` / `vpmovm2*` (kreg⇄vector), `vp2intersect*` (k-pair dest) |
| perm / compress / expand | 30 | `vperm{b,w,i2*,t2*,ilpd/ps}`, `v(p)compress*` (mem-dest MR + elem tuple), `v(p)expand*`, `valign[dq]` |
| vpmisc | 29 | `vprol/vpror(v)*`, `vpshld/vpshrd(v)*` (imm8 + variable), `vpmadd52*`, `vpopcnt*`, `vplzcnt*`, `vpconflict*`, `vpmultishiftqb` |
| converts | 27 | the unsigned/qq lattice `vcvt(t)?{ps,pd}2{u,}{dq,qq}`, `vcvt{u,}{dq,qq}2{ps,pd}`, `vcvt(t)?s{s,d}2usi`, `vcvtusi2s{s,d}` (GPR W-sized), `vcvtph2ps/vcvtps2ph`, bf16 (`vcvtne*`) |
| fp move / compare / misc | 20 | `vmovs{s,d}` (3-op reg + 2-op mem, MR stores), `vmov{ddup,shdup,sldup}`, `vmovnt*`, `vcomis*/vucomis*` (sae), `vmax/vmin s{s,d}`, `vunpck*`, `vexp2p{s,d}` |
| integer compare → mask | 18 | `vpcmpeq/gt{b,w,q}`, `vpcmp(u){b,w,d,q}` imm forms, `vptestm/vptestnm{b,w}` — kreg-dest RVM (plumbing exists: the d/q forms are already covered) |
| broadcast | 14 | `vbroadcast{f,i}{32,64}x{2,4,8}` (tuple mem/eregx src), `vpbroadcast{b,w}`, `vpbroadcastm{b2q,w2d}` (kreg src) |
| dot products / bf16 | 11 | `vpdp{b,w}*` 8 variants, `vdpbf16ps` — plain RVM+bcst batches |
| 4-op / misc tail | 9 | `v4fmadd*/v4fnmadd*` (mem-only multi-reg block), `vp4dpwssd(s)`, `vdbpsadbw`, `vcmps{s,d}` (generic-cc scalar) |

Estimated ~600–800 corpus rules total. No generator or C++ changes are expected except
possibly: a `kreg`-destination form check (likely already handled via the existing KREG
file), and the `v4fmadd` mem-only shape.

## 7. Remaining work — structural items

1. **AVX512-FP16 (EVEX maps 5/6): 88 mnemonics, currently unrouted.** Needs `gen.py` to
   route `evexp0` maps 5/6 into opcode blocks + the corpus rules. **State-budget constraint:
   resolved (2026-07-02).** `DState.next` (and `FSM_INDEX`/`FSM_HALT`/`run_fsm`) were widened
   from `uint16_t` to `uint32_t`, removing the 65,535-state ceiling permanently (sizeof(Fsm)
   960 → 1,200 KiB). Adding the FP16 maps' ~34 blocks is now purely a matter of writing the
   routing + rules; block allocation stays dense-arithmetic (sparse allocation remains a
   future memory optimization, not a prerequisite).
2. **`urdmsr` F2 0F38 F8 reg-form** (deferred): its operand order conflicts with the
   single-direction `ppdesc` slot shared with `enqcmd` — needs a per-mod direction bit in
   the descriptor (small mechanism extension).
3. ~~**`vex_structure` generation**~~ — **done** (§5): the encoder tail table is generated
   from the corpus.
4. Optional, out of scope for bijection: a strict-objdump display mode (PTR keywords,
   `movabs`, disp8*N shown pre-scaled, cc/imm alias expansion (`vcmpeqps`,
   `vpclmullqlqdq`), implicit `<xmm0>` rendering).

## 8. Deliberate decode-display policy (not bugs)

* Size suffixes (`.b/.w/.d/.q/.t`) instead of `BYTE/WORD/... PTR`; `[rax+0]`-style
  addresses; `mov` for `movabs`; generic `cc`/imm-alias mnemonics with the raw imm8 shown;
  implicit operands (`<xmm0>` of blendv/sha256rnds2) not rendered; EVEX disp8*N shown raw.
* **Lenient reserved-field decode** — reserved `vvvv ≠ 1111`, undefined L'L, scalar {er}
  rendered at zmm width: encodings objdump calls `(bad)` still decode if the fields are
  ignorable on silicon, per the project rule *"if some x86 CPU decodes it, this
  implementation should decode it too"*. All of it is bijection-safe because the encoder
  replays raw bytes.

## 9. Reproducing the numbers

```
make roundtrip64 && make fuzz64          # bijection (also restores 32-bit tables)
make roundtrip  && make fuzz             # 32-bit regression
make prog64 prog64b prog64c prog64d prog64e prog64f
# differential sweeps (build the batch harness against 64-bit tables first):
python3 gen.py corpus64.p x86_tables.h && python3 gasm.py corpus64.p x86_tables_enc.h
g++ -std=c++17 -O2 -I. -o /tmp/apxbatch tools/apxbatch.cpp
python3 tools/vexsweep.py ; python3 tools/xopsweep.py ; python3 tools/evexsweep2.py
python3 gen.py corpus.p x86_tables.h && python3 gasm.py corpus.p x86_tables_enc.h  # restore
```

## 10. Suggested order of remaining work

1. Finish EVEX maps 1–3 by family (§6) — pure corpus batches under the proven
   generate→self-verify→fuzz→commit loop; largest coverage win per effort.
2. FP16 maps 5/6 rules (88 mnemonics — the same shapes as existing float families); route
   `evexp0` maps 5/6 in `gen.py` (the state ceiling is already lifted, §7.1).
3. `urdmsr` reg-form descriptor-direction fix.
4. Final full-ISA differential run (legacy + VEX + XOP + EVEX 1–6 + APX sweeps, drift-0)
   and a fuzz soak (≥ 50 M iters) as the completion gate.

**Done since first draft:** the encoder's `vex_structure` tail table is now generated from
the corpus (§5), and the FSM state ceiling is removed via a `uint32_t next` widening (§7.1).
