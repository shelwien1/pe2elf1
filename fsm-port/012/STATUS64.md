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
| `corpus64.p` | 5,212 | **4,684 rules** across legacy/vex/vex2/evex/xop/apx submatches (incl. the 96 AVX512-FP16 map5/6 + the 2 VEX-map7 USER_MSR rules) |
| `gen.py` | 2,324 | FSM compiler (decode) |
| `gasm.py` | 317 | encode-table compiler |
| `x86dec.hpp` | 953 | FSM driver + generic finalizers |
| `x86enc.hpp` | 401 | encoder |
| `x86insn.hpp` / `asm32.cpp` / `fuzz.cpp` | 115 / 137 / 112 | record / tool / fuzzer |
| 64-bit tables | FSM 61,440 DStates, 52 groups, 1,200 KiB compiled | `DState.next` is `uint32_t` — no state ceiling (was 65,535 under `uint16_t`) |
| 32-bit tables (committed) | 54,784 DStates, 39 groups | 32-bit corpus unchanged this iteration |

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
| EVEX maps 1–3 (AVX-512 core) | **complete** | §6 batches all landed (transcendental/range/fpclass, insert/shuffle, fp move/scalar/unpack, mask conv, ph/bf16 converts, vpcmp, 4-op 4FMAPS/4VNNIW); the only sweep residue is 7 objdump imm-aliases (vcmpeqph, vpcmpequb, vpclmullqlqdq) that this decoder emits generically |
| EVEX maps 5–6 (AVX512-FP16) | **complete — 96 / 96, 0 gaps** | 3-bit `mmm` routing added; full MAP5/MAP6 differential vs objdump: 96 distinct mnemonics, 270 match cases, 0 gaps, 0 length/mnemonic mismatches |
| APX: EVEX-promoted legacy (map 4, incl. ND/NF, push2/pop2, groups) + REX2 + USER_MSR | **complete — 0 missing** | 16,384-case map-4 sweep (drift 0); USER_MSR reg-forms (`urdmsr`/`uwrmsr` 0F38 F8) via per-mod `ppdesc` direction, and the VEX-map7 `urdmsr`/`uwrmsr` r64,imm32 forms (the only decodable opcodes in a VEX maps 4–31 sweep) |
| APX: REX2 (D5) over the full 0F map — SSE/MMX, mov cr/dr, cpuid/bt/push-pop-seg, pinsrw/pextrw/punpckldq | **complete — semantic, 0 gaps** | now decoded as real 0F-map ops with r0-31 (was structural-only); 16,384-case REX2 sweep vs Zydis: 0 length mismatches (§7b). M0=1 routes to `FSM_OP2`; M0=0 + `C4/C5/62` (LES/LDS/BOUND) correctly `#UD` |

Everything the sweeps flag as *not* covered decodes through the **structural fallback**:
the prefix/opcode/ModR/M skeleton is captured raw, displayed as a `vex` placeholder, and
still round-trips byte-exactly — so completeness gaps are display-semantic gaps, never
round-trip gaps.

## 4. Round-trip / bijection status — all green

| check | result |
|---|---|
| `make fuzz64` — 5,000,000 random buffers, 64-bit | 4,072,177 accepted (81.4 %), **0 legacy failures, 0 vex/evex/xop failures** |
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
2. APX map-4 `F8`: `uwrmsr/urdmsr` (reg) ↔ `enqcmds/enqcmd` (mem) swap. (The *legacy*
   0F38 F8 path is now fully descriptor-driven — `ppdesc` `mnem_reg`/`mnem_mem` +
   per-mod `dir_reg` — so only the APX map-4 twin still rides this C++ swap.)
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

## 6. EVEX maps 1–3 — complete (history)

**All families below have landed** (EVEX maps 1–3 now decode 0-gap vs objdump; the only
sweep residue is 7 objdump imm-aliases this decoder emits generically). The
generator-with-discovery/self-verification workflow — probe candidate encodings, confirm
objdump's mnemonic *before* emitting each rule — drove every batch; the sweeps stayed
drift-0 and each batch committed only after fuzz64 + roundtrip64 passed. Landed across
iterations: FMA (50), BW-integer arith/pack/unpack (31), shifts (12), crypto/VAES/GFNI (8),
VPMISC variable-rotate/funnel/lzcnt/popcnt/conflict (21), dot-product/bf16 vpdp* (11),
integer-compare→mask fixed-cc + testm/nm (10), broadcasts (14), scalar↔GPR converts (6),
packed converts unsigned/qq lattice (17), permute RVM (16), then the families once tracked
here as open (transcendental/range/fpclass, insert/shuffle, fp move/scalar/unpack, mask
conversions, ph/bf16 converts, vpcmp→kreg, compress/expand/align, 4-op 4FMAPS/4VNNIW).

Families completed (originally tracked as open):

| family | count | examples / notes |
|---|---:|---|
| transcendental / range / fpclass | 32 | `vgetexp{sd,ss}`, `vgetmant*`, `vreduce*`, `vrndscale{ph,sh}`, `vrange*`, `vrcp28*`, `vrsqrt28*`, `vfpclass*` (kreg dest, imm8, sae) — RM/RMI + RVM; some ph/sh are the FP16 encodings |
| insert / shuffle / mask-mov | 32 | `vinsert{f,i}*x*` (lane insert, imm8), `vshuf{f,i}*x*`/`vshuf{ps,pd}` (imm8), `vpshuf{b,d,hw,lw}`, `vpmov{b,d,q,w}2m` / `vpmovm2*` (kreg⇄vector), `vp2intersect*` (k-pair dest), `vmov{ddup,shdup,sldup}` |
| fp move / compare / 4-op | 27 | `vmovs{s,d}` (MR stores), `vmovnt*`, `vcomis*/vucomis*` (sae), `vmax/vmin s{s,d}`, `vunpck{h,l}p{s,d}`, `vexp2p{s,d}`, `v4f{,n}madd*` (mem-only reg-block), `vp4dpwssd(s)`, `vdbpsadbw` |
| compress / expand / align | 14 | `v(p)compress*` (mem-dest MR store), `v(p)expand*`, `valign[dq]` (imm8) |
| misc / imm groups | 14 | `vfixupimm*` (RVMI), `vprord/vprorq` + `vpshld/vpshrd{d,q,w}` (imm8 rotate/funnel groups on 0F 72/73 & 0F3A), `vcmps{s,d}` (generic-cc scalar) |
| ph/bf16 converts | 4 | `vcvtph2ps` (widen), `vcvtps2ph` (MRI store, imm8), `vcvtne2ps2bf16` (RVM), `vcvtneps2bf16` (narrow) |

Plus 7 cosmetic sweep flags (objdump's imm-variant aliases `vcmpeq*`/`vpcmpeq*u`/`vpclmul*`
that the generic mnemonic + imm8 already round-trips). No generator/C++ changes expected
except possibly the `v4fmadd` mem-only multi-register shape and the `vp*compress` mem-dest
store form.

## 7. Remaining work — structural items

1. ~~**AVX512-FP16 (EVEX maps 5/6): 88 mnemonics, unrouted.**~~ — **done (2026-07-02).**
   `VEX_LAYOUT['evex']` now carries a 3-bit `mmm` map field; `evexp0`/`_vex_parse_rule` route
   maps 5/6 to opcode blocks (`evexp1/2/op` grown to `[5]`), a `bcst16` table was added, and
   all 96 map5/6 mnemonics are in the corpus (arithmetic/sqrt/comis, FMA ph/sh, complex FMA,
   scalef/getexp/rcp/rsqrt, width-changing converts, GPR converts, `vmovsh`/`vmovw`). Full
   MAP5/MAP6 differential vs objdump: 0 gaps. The `uint16_t→uint32_t` state widening (no
   65,535-state ceiling; sizeof(Fsm) now 1,200 KiB) made the block growth free.
2. ~~**`urdmsr` F2 0F38 F8 reg-form** (deferred).~~ — **done (2026-07-02).** The `ppdesc`
   slot gained a per-mod direction: `dir_mem` drives the pre-ModR/M `CAP_DIR` (the common
   path) and `dir_reg` overrides once reg-vs-mem is known — so `urdmsr` (reg-direct `r/m,reg`)
   coexists with `enqcmd` (mem `reg,mem`) at 0F38 F8 pp3. The VEX-map7 `urdmsr`/`uwrmsr`
   `r64,imm32` forms were added too (new `VEX_MI`/`VEX_IM` forms + map7 FSM routing); a VEX
   maps 4–31 sweep confirms those are the only decodable opcodes in the extended-map space.
3. ~~**`vex_structure` generation**~~ — **done** (§5): the encoder tail table is generated
   from the corpus.
4. Optional, out of scope for bijection: a strict-objdump display mode (PTR keywords,
   `movabs`, disp8*N shown pre-scaled, cc/imm alias expansion (`vcmpeqps`,
   `vpclmullqlqdq`), implicit `<xmm0>` rendering).

**As of 2026-07-02 the decoder covers the complete x86-64 instruction set through APX +
AVX512-FP16 with no known decodable-opcode gaps** (every differential sweep — legacy, VEX
1–3 + 7, XOP 8–10, EVEX 1–3 + 5–6, APX map 4 — is drift-0 with 0 gaps, under the byte-exact
bijection invariant).

### 7a. Second differential oracle — iced-x86 1.21 (finds non-standard forms)

objdump 2.42 is only a partial oracle; a cross-check against **iced-x86 1.21** (a
comprehensive Intel-authored decoder that also knows AVX10.2/APX/undocumented forms)
surfaced — and this iteration fixed — several forms objdump had flagged only partially:

* **Dead-REX opsize (correctness bug, fixed).** A legacy REX followed by another legacy
  prefix is ignored by the CPU; the FSM had latched `opsize=2` from a now-dead REX.W, so
  `4A F2 B8 id` mis-decoded as `mov rax,imm64` (len 11) instead of `mov eax,imm32` (len 7).
  `VAR_OPSIZ` is now recomputed from the *effective* REX after prefix parsing. This removed
  every length mismatch from a 100k random-buffer iced differential (0 length bugs).
* **Documented holes filled** (objdump + iced both decode; the earlier legacy sweep missed
  them): 0F C7 group `/3–/7` (xrstors/xsavec/xsaves, vmptrld/vmclear/vmxon/vmptrst,
  rdrand/rdseed/rdpid/senduipi), far `callf`/`jmpf` (FF /3,/5), legacy GFNI (gf2p8mulb,
  gf2p8affineqb, gf2p8affineinvqb), legacy AES aeskeygenassist, VMX invept/invvpid/invpcid,
  SSE4.1 pextrw (0F3A 15), FRED lkgs, TSXLDTRK xsusldtrk/xresldtrk, shift-group `/6`
  (shl alias) and test-group `/1` alias, rdrand/rdseed 16-bit (66).
* **Undocumented x87 reg-direct aliases added** (real on silicon; objdump `(bad)`): fcom
  (DC/2), fcomp (DC/3, DE/2), fxch (DD/1, DF/1), fstp (DF/2, DF/3), fstpnce (D9/3).

Remaining iced-only differences, deliberately left (all bijection-safe):

* **66 on a near branch** (`66 E9`/`E8`/`0F 8x`): this decoder emits rel16 (`jmpw`),
  matching **objdump** and the legacy/AMD reading; iced emits rel32 (66 ignored per the
  Intel-64 near-branch note). A deterministic decoder must pick one; both round-trip.
* **Non-canonical / vendor forms not added**: fence `rm≠0` (lfence/mfence/sfence E9–FF —
  broadening the ModR/M would print a phantom GPR operand on the canonical rm=0 form), and
  VIA PadLock `xsha1`/`xstore`/`xcrypt` (discontinued vendor; iced itself decodes only 2 of
  them). *(The `0F 0D` reg-direct long-nop and the `66`/`F2`-ignored `wbinvd` — both once
  listed here — are now decoded; see §7b.)*

### 7b. Third differential oracle — Zydis (finds the REX2 0F-map gap)

A cross-check against **Zydis** (zyantific/zydis, built from source) revealed that the
APX **REX2 (D5) prefix** was only *structurally* handled for most of the 0F map: the
bytes round-tripped, but everything except a handful of ops decoded to a placeholder or
`(bad)`. REX2 with `M0=1` is just "the 0F map with the 4th register bit", so `D5 <pl> op
modrm` must decode exactly like `0F op modrm` with r0-31 registers. Three root causes,
all fixed this iteration (objdump `{rex2 …}` and Zydis agree on every case):

* **Vector ops mis-routed to VEX/EVEX (Category A).** The C++ VEX/EVEX/XOP detector keyed
  on the *opcode byte* being `C4`/`C5`/`62` — but under REX2.M0=1 those are ordinary 0F
  opcodes (`62` punpckldq, `C4` pinsrw, `C5` pextrw), already decoded by `FSM_OP2`. It
  wrongly re-entered `vex_decode` (a length-10 phantom EVEX op). Fixed by excluding the
  REX2 path from that detector; `D5 <pl> 62/C4/C5` now fall through to the legacy fill.
* **1-byte-map special blocks eating 0F opcodes (Category B).** The `mov cr/dr` block keyed
  on a literal `0F` (never present under REX2), and the `moffs` (A0–A3) block grabbed the
  REX2 0F-map opcodes `A0/A1/A2/A3` (push/pop fs, cpuid, bt) and demanded an 8-byte address
  → truncated → `(bad)`. Fixed: the `moffs` block is now guarded by `!rex2_0f`, and `mov
  cr/dr` is a single unified handler placed after REX2 consumption that covers both the
  literal-`0F` form (`vex=5`) and the REX2.M0 form (`vex=6`, the 0F implied by M0).
* **`mov cr/dr` rendering (pre-existing, fixed).** The GPR is now r64 in long mode (was
  wrongly r32: `mov rcx,cr0`), and the cr/dr number extends to 0–15 via REX.R / REX2.R3
  while the GPR extends to r0–31 via REX.B / REX2.B3.B4 (`44 0F 20 c1` → `mov rcx,cr8`;
  `41 0F 20 c1` → `mov r9,cr0`; `D5 84 20 d1` → `mov rcx,cr10`).

REX2.M0=0 + `C4`/`C5`/`62` (1-byte-map LES/LDS/BOUND, all `#UD` in 64-bit) is rejected — a
VEX/EVEX prefix cannot follow REX2, and `FSM_OP1`'s capture transitions must not apply.
Validated by a 16,384-case REX2 sweep (all `M0=1` payload/reg-bit combos × reg/mem × all
256 opcodes): **0 length mismatches vs Zydis**, and the 5 M-buffer `fuzz64` stays at 0
round-trip failures (bijection is preserved — the register extension only changes the
*rendered* number; the encoder re-emits ModR/M from the low 3 bits with the high bits in
the replayed REX2 payload).

A follow-up comprehensive legacy+REX+REX2 sweep vs Zydis (14,848 cases: every prefix ×
map × opcode × reg/mem) then closed the last two coverage gaps it found:

* **`0F 0D` reg-direct long-nop** (Intel reserved multi-byte NOP; objdump `(bad)`, Zydis
  `nop`). Added as `nop~0d` reg-forms — the `~0d` disambiguator gives it a distinct enum
  from the `0F 1F` / `0F 18-1E` nop-space opcodes, so the encoder round-trips it byte-exact
  (mnem 224 vs 48/220…) with no candidate ambiguity; it renders as `nop`.
* **`66`/`F2` on `wbinvd`** (`0F 09`). The `[$pp==0]` guard (added to split off `wbnoinvd`
  at F3) rejected the meaningless-prefix forms that real silicon ignores → wbinvd; added
  `[$pp==1]`/`[$pp==3]` → `wbinvd` (F3 still → wbnoinvd, which is *more* correct than this
  Zydis build, which mis-decodes F3 as wbinvd). This also removed an `invd`-vs-`wbinvd`
  inconsistency (invd was already unguarded and accepted 66).

After both, the legacy+REX+REX2 sweep is **GAP=0 vs Zydis** — the only residual differences
are the 18 `66`-near-branch length picks (deliberate rel16, §7a) and intentional
over-decoding of non-canonical forms Zydis rejects.

A high-volume random sweep (300k buffers, VEX/EVEX/XOP-weighted) then found the last two:

* **xabort (`C6 F8 ib`) / xbegin (`C7 F8 rel16/32`)** — TSX/RTM. These are the `C6`/`C7`
  group `/7` with a *fully-fixed* ModR/M byte (`F8`) followed by an immediate/rel — a shape
  the fixed-ModR/M path did not support (it assumed no operand and dropped the tail). The
  machinery now carries a `(mnem, form, imk)` triple through `_fixmodrm` → `group_state` →
  the encoder's `fixmodrm` candidate, so a fixed-ModR/M op can carry a trailing imm8/rel
  (the encoder emits opcode + ModR/M + `enc_imm`). xbegin honours `66`→rel16 like the other
  near branches. Existing no-operand fixed-ModR/M ops (endbr64/monitor/xgetbv/lfence/…) are
  untouched (form NONE, imk NONE).

With these, **both** the systematic legacy sweep and the 300k random VEX/EVEX/XOP sweep are
**GAP=0 vs Zydis** (only the deliberate `66`-near-branch length picks and the discontinued
KNC `jkzd`/`jknzd` remain, both documented).

### 7c. AMX (Advanced Matrix Extensions) — tile ops now decoded semantically

A *mnemonic*-comparing differential (not just length) found that the length-only sweeps had
hidden a whole family: AMX (Sapphire Rapids+) decoded to the `vex` structural placeholder
with a *matching length*, so `agree_len` counted it as agreement. All 13 AMX ops now decode
semantically (verified against Zydis + objdump), via a new `T_TMM` tile-register file:

* **Infra**: `T_TMM` operand type (tmm0-7); VEX file code 7 (`vex_mkop`); `tmmreg` table +
  `VEXFILE['TMM']=7` and the `tmmreg`→`TMM` operand-file mapping in gen.py. `vvvv` is typed
  by the reg file, so `rf=TMM` covers dst+src1+src2 for the 3-operand forms.
* **Ops** (VEX.128.0F38.W0): LDTILECFG (NP 49 mem), STTILECFG (66 49 mem), TILERELEASE
  (NP 49 C0 — reg-form swap in vex_finalize, like vmovlps→vmovhlps), TILEZERO (F2 49 reg),
  TILELOADD/TILELOADDT1/TILESTORED (F2/66/F3 4B, sibmem), and the dot-products TDPB[uu/us/
  su/ss]D (5E, RMV) + TDPBF16PS/TDPFP16PS (5C). `AMX-COMPLEX` (tcmm* at 6C) is post-dating
  this Zydis build; not yet added.

Bijection preserved (VEX byte-replay is mnemonic-independent): fuzz64 5 M = 0 failures,
roundtrip64 byte-identical, plus a targeted AMX decode/re-encode check.

### 7d. Mnemonic-comparing differential — the WIG / VEX-completion campaign

The length-only Zydis sweeps (§7a/§7b) have a blind spot: an uncovered VEX/EVEX opcode
decodes to the `vex`/`evex` *structural placeholder* which, for most opcodes, consumes the
*same length* as the real instruction — so `agree_len` counts it as agreement. A sweep that
compares the **mnemonic** (400 k random VEX/EVEX/XOP buffers) exposed 378 such hidden
placeholder cases (28 distinct mnemonics). Fixing them took placeholders 378 → 13:

* **WIG W-fallback (biggest lever).** Most VEX 0F/0F38/0F3A and EVEX byte/word ops are
  W-ignored (WIG), so `VEX.W1 vaddps`, `vpminub`, EVEX `vpmullw`, ... are valid and decode
  like W0 — but the rules bind one W. `build_vex` now fills an empty W-sibling cell from its
  partner (VEX + EVEX; never overwrites, so W-significant ops with both W present, and
  XOP/APX, are untouched). Removed ~320 placeholders; 0 wrong fills (verified).
* **FORM_VEX_MVR.** vmovss/vmovsd `0F 11` store reg-form uses operand order r/m,vvvv,reg,
  which had no VEX form → the cell silently never compiled (latent bug). Added the form.
* **Genuinely-missing AVX/AVX2/newer VEX ops** (were absent, not just non-canonical):
  vperm2i128, vmovntdqa, vmaskmovps/pd, vpmaskmovd/q, vcvtps2ph store, vpmadd52luq/huq
  (AVX-IFMA), vgf2p8affineqb/invqb (GFNI VEX), vprold/vprolq (EVEX 0F72 /1), vpdpwusd/s
  (AVX-VNNI-INT16), blsr/blsmsk/blsi (BMI1) + rorx (BMI2). A build_vex group-typing fix
  (prefer a concrete r/m file over the MEM marker) was needed for blsr's GPR source.

### 7e. APX CCMP/CTEST — the conditional compare/test family (done, 2026-07-03)

Every map-4 CMP (38-3B, group 80/81/83 /7) and TEST (84/85, group F6/F7 /0,/1) is really
the APX *conditional* form. `apx_finalize` now rewrites the mnemonic by the source condition
code SCC = `P2[3:0]` — `o no b nb z nz be nbe s ns t f l nl le nle` — to `ccmp`cc / `ctest`cc,
and records the default-flags value DFV = the raw (non-inverted) `vvvv` = {OF,SF,ZF,CF},
rendered `{dfv=…}`. The 32 mnemonics (16 SCC × ccmp/ctest) are appended by gen.py
(`MNEM_CCMP_BASE` / `MNEM_CTEST_BASE`); the NDD prepend is suppressed for the family (they
only read flags). CCMP rides the existing reg cells (which serve the memory r/m forms too)
plus the uniform-imm 80/81/83 groups; CTEST's F6/F7 is a *mixed-imm* group (only /0,/1 carry
an immediate, /2,/3 = not/neg do not), which the per-opcode `vextail` can't express — so the
imm width is set by the ModR/M /digit on both sides: a decode-time `CAP_IMK` override (before
`append_imm`) and an `in->apx`-gated `vex_encode` override, keeping the round-trip exact.
Validated GAP=0 vs Zydis over a 3200-case reg/mem/imm × 16-SCC × DFV sweep (only Zydis
disassembles this family; objdump 2.42 rejects it). Bijection preserved: fuzz64 5 M = 0,
roundtrip64 byte-identical.

### 7f. Post-CCMP mnemonic-differential sweep — remaining VEX/EVEX gaps (done, 2026-07-03)

A fresh 300 k-buffer mnemonic sweep vs Zydis (after CCMP/CTEST) surfaced ~15 non-KNC
placeholder mnemonics; all now decode:

* **AVX2 VEX forms** whose EVEX/legacy siblings were present but the VEX form absent:
  `vpermq`/`vpermpd` (0F3A 00/01 W1), `vbroadcasti128`/`vbroadcastf128` (0F38 5A/1A),
  `vldmxcsr`/`vstmxcsr` (0F AE /2,/3 — the first VEX `@addr(/digit)` group → VEX_MG).
* **SM3 / AVX-NE-CONVERT 66-prefix siblings** (the NP/F3/F2 were present): `vsm3msg2`,
  `vcvtneeph2ps`, `vbcstnesh2ps`, plus the VEX form of `vcvtneps2bf16` (F3 72).
* **AMD FMA4 `vpermil2ps/pd`** (VEX.66.0F3A.W0/W1 48/49): 4-operand is4 with the W0/W1
  r/m↔is4 swap, mirroring the FMA4 vfmaddsub is4 family. The is4 byte's low-nibble
  selector is preserved in the replayed immediate; shown 4-op (the control nibble is a
  §8 display simplification).
* **APX EVEX-promoted BMI** — the *first EVEX GPR ops*: `andn` `bextr` `bzhi` `shlx`
  `sarx` `shrx` `pext` `pdep` `mulx` (map 2 F2-F7) + `rorx` (map 3 F0), W0=r32/W1=r64.
  `vex_finalize` already lowers them; reg/vvvv/r-m reach r0-31 via the EVEX R'/V'/X
  bits and the memory-base high bit rides in the replayed prefix. `rorx` (no vvvv) fixes
  V'=1. All match Zydis; fuzz64 5 M = 0, roundtrip64 byte-identical.

### 7g. VSIB gather, EVEX vpermil/vpinsr, the k-mask matrix, CMPccXADD (done, 2026-07-03)

Closing the last non-KNC gaps. Two larger sweeps (300 k, then 1.5 M) drove the placeholder
set down to KNC only:

* **VSIB — vector-typed memory index** (`x86insn_t.mem_ix_t`): `vex_finalize` now recognizes
  the gather (0F38 90-93) / scatter (A0-A3) opcodes and, for the memory form, re-derives the
  index from the raw SIB (bypassing the GPR "no-index" rule that dropped index==4), tags its
  vector width, and sizes the data reg + VEX mask by the index-vs-element rule. This adds the
  **AVX2 VEX gathers** (`vpgatherd/q d/q`, `vgatherd/q ps/pd`) *and* fixes the already-covered
  **EVEX gathers**, which had been rendering their VSIB index as a GPR and dropping index==4.
  Display-only (the encoder replays the SIB), so byte-exact throughout.
* **EVEX vpermilps/pd by imm8** (0F3A 04/05) and **EVEX vpinsrb/d/q** (0F3A 20/22) — only the
  VEX / EVEX-variable siblings had been present.
* **AVX-512 k-mask matrix completed.** The set only had the W0 b/w forms; W is significant
  (b/w vs d/q) yet the generic WIG W-fill silently decoded every W1 as its W0 mnemonic. Added
  explicit W1 rules for `kand/kandn/kor/kxnor/kxor/kadd` (d/q), `kunpckwd/dq`,
  `knot/kortest/ktest` (b/d/q) and `kshift l/r` (b/d/q), and fixed `kmov` d/q (kmovq kreg,kreg
  was at the wrong prefix).
* **CMPccXADD** (EVEX.66.0F38 E0-EF) — the 2022 atomic compare-and-add family: `[mem],reg,vvvv`,
  cc in the opcode nibble, W0=r32/W1=r64. Added `FORM_VEX_MRV` (r/m,reg,vvvv) + 32 rules.

All match Zydis; fuzz64 5 M = 0, roundtrip64 byte-identical, 32-bit 858/858.

The only placeholders left are **KNC / Xeon Phi (discontinued)** — `jkzd`/`jknzd`,
`kconcatl/h`, `kmerge2l1*`, `kextract`, `vprefetch0/1/2/nta/enta/e2`, `clevict0/1`, and the
*VEX-encoded* `tzcnt`/`lzcnt`/`popcnt` (the legacy F3.0F forms are covered). A dead ISA branch
whose encodings collide with modern semantics; deliberately out of scope (same policy as §7a),
and bijection-safe via the structural fallback regardless.

### 7h. REX2 execution differential vs Intel SDE — two decoder bugs found + fixed (2026-07-03)

To test the APX **REX2** prefix (`D5` + payload `M0 R4 X4 B4 W R X B`) the same way the REX
run was tested (run raw bytes, observe register effects), and since no shipping silicon has
APX, the executing oracle is **Intel SDE 10.8.0** (`sde64 -dmr` = Diamond Rapids, the first
APX chip). `/tmp/rex2exec.c` is the REX-harness extended to load/save r16-r19 via REX2 movabs
/ stores, so EGPR effects are observable; `/tmp/rex2drive.py` compares **apxb** (this decoder)
against **XED** (Intel's reference decoder, bundled with SDE) and **SDE execution** across
REX2.W, EGPR access, the M0 map bit, and prefix-ordering edge cases. (QEMU is not an option —
even 11.0.2 models only the APX CPUID/XSAVE state for KVM pass-through; its TCG decoder has no
REX2/APX at all.) The differential exposed two real decode bugs, both now fixed:

* **`0x90-0x97` (XCHG eAX,r) ignored the register extension.** The old per-byte rules
  (`0x91 => "xchg eax," greg[1]` …) used a *literal* table index, which never materializes as
  an operand — so `0x91-0x97` rendered an empty second operand, and none of the range saw
  REX.B / REX2.B4. `41 90` (xchg r8d,eax) and `d5 18 90` (xchg r16,rax) both mis-decoded as
  `nop`. Fixed with the embedded-reg idiom `10010 bbb => "xchg eax," greg[$b]` (extends to
  r0-31 like push/pop/bswap), keeping `0x90 => nop` as the specific rule for the nop encode
  candidate; C++ `decode_insn` maps the byte back to `nop` only when REX.B/REX2.B4 leave the
  register at rAX. SDE confirms the semantics: `d5 18 90` with rax≠r16 **swaps** them (a real
  xchg, not a nop). One operand + prefix-replay, so byte-exact.
* **A legacy REX (`0x40-0x4F`) immediately before REX2 is `#UD`.** XED reports `BAD_REX_PREFIX`
  and SDE faults `#UD`, but the decoder had dropped the REX and decoded the REX2 instruction.
  Fixed in the REX2 branch: if `s[ip-1]` is a REX byte, reject (`mnem = 0xFFFF`). A *legacy*
  prefix before REX2 stays legal (`66 d5…`), and a REX that a following legacy prefix already
  neutralized stays legal (`40 66 d5…`) — only a REX byte adjacent to `D5` is illegal, matching
  XED/SDE exactly.

Everything else agreed on the first pass: REX2.W operand size, EGPR r16-r31 (reg/rm/base/index
via B4/R4/X4), the M0=1 0F-map (`imul`/`bsf` with no `0F` byte), and the whole edge-case set
(`D5` then a prefix/REX/`D5`, or M0=1 reaching the unreachable 0F38/0F0F maps → all `#UD`).
After the fixes: apxb == XED == SDE across the battery, fuzz64 5 M = 0, roundtrip64 byte-
identical, 32-bit 858/858 + noncanon + fuzz 5 M = 0. Residual cosmetic-only differences (not
decode errors): xchg operand order (accumulator-first vs XED's reg-first — a symmetric op) and
the accumulator rendering `eax` under REX.W, both shared with the pre-existing `add eax,`/`cmp
eax,` accumulator-string convention.

**Follow-up — illegal prefixes before VEX/EVEX/XOP now `#UD`.** The REX2 differential's sibling
question ("can a REX / mandatory prefix precede a VEX-class introducer?") turned up a third,
broader deviation: the decoder had accepted *any* prefix before a `C4`/`C5`/`62`/`8F`(map≥8)
introducer (e.g. `66 c5 f8 77`, `40 c5 f8 77` decoded instead of faulting). Measured on XED/SDE,
the rule is: `66`/`F2`/`F3`/`LOCK` anywhere in the run → `#UD`; a REX (`40-4F`) *immediately*
before the introducer → `#UD` (a REX a following segment/`67` prefix neutralizes stays legal,
mirroring the plain-REX rule); segment and address-size (`67`) prefixes are allowed. `decode_insn`
now enforces this in the 64-bit path (one guard, ahead of the VEX/XOP/REX2 dispatch). Verified
0 mismatches over an exhaustive single/double-prefix × {VEX C5, VEX C4, EVEX, XOP} sweep vs XED;
fuzz64 5 M = 0 (accept rate 83.9 %→76.7 % as the illegal combinations are now rejected),
roundtrip64 byte-identical, 32-bit 858/858 + fuzz 5 M = 0. Rejecting illegal encodings cannot
break the bijection (there is nothing to re-encode). The full REX/REX2/VEX prefix behavior — and
this decoder's conformance — is written up in `REX_REX2_prefixes.md`. (One unrelated gap the sweep
noted and left: `LOCK` on a register-destination op, e.g. `f0 01 c0`, is `#UD` on hardware but
still accepted — a LOCK/ModR/M-validity issue, not a REX/REX2 one, and bijection-safe. **Fixed in
§7i.**)

**Follow-up — REX2 opcode eligibility now `#UD`.** A 256-opcode × {map0, map1} sweep vs XED found
the decoder was applying REX2 to opcodes that have no eGPR-extendable operand, which is `#UD` on
silicon: `M0=0` {`70-7F` Jcc8, `A0-AF` moffs/string/test-acc, `E0-EF` loop/jrcxz/in-out/call/jmp};
`M0=1` {`30-37` system, `80-8F` Jcc32}. `decode_insn` now rejects these in the REX2 block (opcodes
with a ModR/M, an embedded GPR, or opsize-sensitivity like `98/99` stay eligible — note the
asymmetry with plain REX, which is a silent no-op on `48 EB 00` where `D5 08 EB 00` is `#UD`).
Sweep now 0 REX2-eligibility mismatches vs XED; fuzz64 5 M = 0 (accept 76.7 %→75.1 %), roundtrip64
byte-identical, 32-bit 858/858. Documented in `REX_REX2_prefixes.md` §6a.

**Follow-up — NP-form mandatory-prefix SSE opcodes now `#UD`.** The REX2 sweep's map-1
residue turned out to be a *separate, non-REX2* bug: the decoder accepted the NP (no-mandatory-
prefix) form of `66`/`F2`/`F3`-only SSE opcodes (e.g. `0F 6C` bare decoded as `punpcklqdq mm`
where hardware `#UD`s — punpcklqdq is 66-only). Seven opcodes: `0F 6C/6D` (punpcklqdq/punpckhqdq,
66-only), `0F 7C/7D` (haddp/hsubp: 66=pd, F2=ps), `0F D0` (addsubp: 66=pd, F2=ps), `0F E6`
(cvt: 66=cvttpd2dq, F3=cvtdq2pd, F2=cvtpd2dq), `0F B8` (popcnt, F3-only). Fixed in the corpus by
marking the `#UD` prefix slots of the per-prefix mnemonic tables with a new `-` sentinel that
`gen.py`/`gasm.py` compile to mnem `0xFFFF` (→ `#UD`); `0F 6C/6D` and `0F B8` were converted from
their unguarded/opsize rules onto the same pp-select tables. Valid-form encodings are unchanged
(only `#UD` slots removed), so bijection is intact: fuzz64 5 M = 0, roundtrip64 byte-identical,
32-bit 858/858; the NP-0F over-acceptance sweep vs XED is now 0. The single remaining REX2 map-1
sweep entry (`0F A6` montmul, VIA PadLock) is a *don't-care* corner — REX2 + VIA PadLock executes
on no CPU (no chip has both APX and PadLock) and XED is itself inconsistent (it `#UD`s REX2+montmul
but accepts REX2+xstore at `0F A7`), so the VIA ops are left decoding under REX2, bijection-safe.

**Follow-up — NP-form 66-only SSE4 opcodes in 0F38/0F3A (~53), + the pp/opsize root fix.** The same
NP-over-acceptance extended across the SSE4.1/4.2 0F38/0F3A ops (`pmovsx*`, `ptest`, `pmulld`,
`round*`, `blend*`, `pinsr*`, `pextr*`, `dpps`, `pcmp*str*`, …): their NP (and F3/F2) forms decoded
as a phantom MMX/xmm op where hardware `#UD`s. These use the `ssereg[$opsiz*8+…]` / `[$pp==1]`
mechanism, not the pp-vtab, and the obvious `[$pp==1]` guard turned out to *conflate pp with
operand size*: `[$pp==1]` compiles to "opsize==1" (66 **and no** REX.W), so `66`+`REX.W` (opsize=2)
mis-read as NP — it would reject the valid `66 48 0F38 20` (pmovsxbw) and, indeed, already rejected
`66 48 0F38 DC` (aesenc, a pre-existing bug). **Root fix:** `pp` is now derived from *"is 0x66 in
the prefix run"* (`insn_has_66`, scanning `pfx[]`) instead of `$opsiz==1`, at all five C++ pp sites
— REX.W-independent. That alone fixed the aesenc-family `66`+`REX.W` over-rejection (and `movapd`
under REX.W, etc.). Then the ~53 ops got `[$pp==1]` + an `ssereg[8+…]` (xmm) operand, so NP/F3/F2
→ `#UD` and `66`(±REX.W) decode correctly. NP-0F38/0F3A over-acceptance sweep vs XED now 0; fuzz64
5 M = 0, roundtrip64 byte-identical, 32-bit 858/858 + fuzz 5 M = 0 (32-bit is unaffected — with no
REX.W, `has_66` ≡ the old `opsize==1`). The corpus32 baseline still carries the un-guarded NP forms
(out of scope for the x64 port), bijection-safe there.

### 7i. VEX/EVEX/XOP map-range validation + LOCK-prefix validity (2026-07-04)

Two `#UD`-tightening fixes, both bijection-safe (rejecting an illegal byte stream only removes it
from the accepted set — there is nothing to re-encode), verified against XED across the opcode space.

**VEX/EVEX/XOP map-range.** `vex_decode` now rejects a structurally-impossible map field before the
opcode lookup: `C4` (3-byte VEX) map ∈ {1,2,3}; `C5` (2-byte VEX) is always map 1; `8F` (XOP) map ∈
{8,9,10}; `62` (EVEX) map ∈ {1,2,3,4,5,6} (4 = APX, 5/6 = the AVX512-FP16 MAP5/MAP6). Anything else →
`#UD`. Confirmed vs XED: every out-of-range map faults on both sides (`C4` map 0/4-31, `8F` map 0-7/11-31,
`62` map 0/7), every in-range map with a valid opcode decodes on both.

**LOCK (`F0`) validity.** `F0` is now legal only on an RMW mnemonic whose *destination* is memory,
matching silicon. The lockable set is a generated `mnem_lockable[]` bitmap (`gen.py`: add/adc/and/or/
xor/sub/sbb/inc/dec/neg/not/xchg/xadd/btc/btr/bts/cmpxchg/cmpxchg8b/cmpxchg16b), keyed by a
`_lockbase()` that strips the `~`-tag and the `.b/.w/.d/.q` size suffix so group memory forms
(`inc.b`, `bts.d`, …) match. The C++ `lock_illegal()` helper checks `mnem_lockable[mnem] && op[0]`
is `T_MEM` (with the xchg symmetry: its memory operand may sit in `op[1]`); it runs both on the main
legacy exit **and** on the moffs (`A0-A3`) early-return, so `lock mov [abs],al` is rejected the same
as `lock mov r,r`. This closes the register-destination LOCK gap that §7h had explicitly noted and
left (`f0 01 c0` and friends now `#UD`). Exhaustively differenced vs XED — all lockable opcodes ×
all 256 ModR/M bytes, plus the full 1-byte and 0F opcode space × a ModR/M sample — with **0 false
rejections** (no legal LOCK is wrongly faulted). The only permissive divergence from XED is
`LOCK MOV CR/DR` (`0F 20-23`), a documented **AMD** CR8/DR8-access extension: some x86 CPU decodes it,
so per the "if any CPU decodes it, we decode it too" rule it keeps its own early return without the
LOCK check (and round-trips losslessly — `F0` rides in `pfx[]`).

Gate after both fixes: fuzz64 5 M = 0, roundtrip64 byte-identical, 32-bit 858/858 + noncanon + x8632all
110072/110072 + fuzz 5 M = 0, prog64/b/c/d/e/f all byte-identical, and a 62-instruction lock-heavy
binary (every lockable form + REX/REX2 widening + disp/SIB/RIP addressing + redundant-prefix runs +
AMD lock-mov-cr) round-trips byte-identical.

**Follow-on (now closed in §7j).** Undefined opcodes *inside* a valid VEX/EVEX/XOP map used to
decode to the structural placeholder mnemonic `vex` instead of `#UD` (e.g. `8F` map9 opcode `A2`,
`C4` map1 opcode `27`). That is now fixed with an XED-derived per-`(kind,map,pp,W,opcode)` validity
oracle — see §7j.

### 7j. VEX/EVEX/XOP opcode-validity oracle — undefined opcodes in valid maps now #UD (2026-07-04)

§7i's map-range check rejects impossible *maps*; this closes the finer gap it noted — a genuinely
undefined *opcode within* a valid map (e.g. `C4` VEX map1 `27`, `8F` XOP map9 `A2`, EVEX map1 `06`)
used to fall through to the structural `vex` placeholder rather than `#UD`, so the decoder accepted a
large swath of encodings no CPU decodes (~91% of the 24 576-key VEX space; the placeholder existed
purely as a bijection safety-net that round-trips any structurally-valid VEX byte stream).

`tools/vexvalid_oracle.py` builds a validity bitmap from **Intel XED** (SDE's reference decoder, fast
`-F` filter mode): for every `(kind,map,pp,W,opcode)` key it probes XED across all 8 ModR/M.reg
values × {reg-direct, `[rax]`, VSIB (rm=100+SIB, essential for gather/scatter), rm=101 disp32} ×
{L=0,1,2} × (EVEX) mask {k0,k1}, and marks the key VALID iff XED decodes *any* probe. Keys XED
rejects in **every** form are the undefined ones. The 12 KB bitmap + a `vexvalid()` accessor is
emitted to the generated `x86_vexvalid.h`; `vex_decode` consults it on the placeholder path only, so
a key XED never decodes becomes `#UD` while covered opcodes (named by the FSM-native path, which
never reaches the placeholder) and valid-but-uncovered keys are untouched.

This "reject only what XED rejects in every form" rule cannot `#UD` anything XED accepts. Validated:
a 4.42 M-probe apxb-vs-XED differential shows **0 regressions** (no XED-accepted encoding is faulted);
the change removed ~95 % of the VEX-space over-acceptance. Full gate stays green — fuzz64 5 M = 0,
roundtrip64 byte-identical, 32-bit 858/858 + noncanon + x8632all 110072/110072 + fuzz 5 M = 0, and
prog64/b/c/d/e/f (incl. real AVX-512 EVEX with masks/broadcasts in prog64e) byte-identical. Two
addressing-form subtleties the validation surfaced and fixed in the oracle: gather/scatter are `#UD`
except with a vector-index SIB (so the VSIB probe is required — a missing one first showed up as an
x8632all `vgatherdpd` regression), and EVEX gather/scatter are `#UD` without a mask (so the k1
probe is required). Regenerate with `python3 tools/vexvalid_oracle.py <xed64> x86_vexvalid.h`.

Residual (accepted, conservative): validity is keyed at `(kind,map,pp,W,opcode)`, so a *defined*
opcode's undefined sub-forms (a particular /digit, addressing form, or vector length XED rejects)
still take the placeholder rather than `#UD` (~229 K such sub-key probes). Tightening those needs
finer per-/digit/L keying with more regression risk; left as-is. The KNC/MVEX `62`-prefix variant is
not modelled (this decoder reads `62` as EVEX only); it is a non-issue since the placeholder never
decoded MVEX semantics anyway.

### 7k. Zydis 40-bit-mining roundtrip corpus (25 397 encodings, 2026-07-04)

A canonical corpus mined by **Zydis** across the 40-bit opcode space (one representative per
canonical form; `tools/rtbatch.cpp` decodes+re-encodes each and compares bytes): **0 byte-level
roundtrip mismatches** over all 25 397 — the bijection holds on every form Zydis and this decoder
both accept. 100 encodings decoded to `#UD` here that Zydis accepts; a differential vs XED split
them into real gaps (fixed) and Knights Corner (left as `#UD`), and a *mnemonic* differential
(names, beyond the byte roundtrip) found the rest.

Fixed (committed):
* **8 real-Intel coverage gaps** the byte roundtrip flagged as `#UD`: `0F 01` mandatory-prefix
  reg-forms `tdcall`/`seamret`/`seamops` (66), `saveprevssp`/`uiret`/`testui` (F3); `hreset`
  (`F3 0F3A F0 /0`); `jmpabs` (APX `D5 <W0> A1 io64`, a C++ REX2 one-off reusing the moffs shape).
* **11 `0F 01` prefix-form mnemonics** the *mnemonic* differential caught (byte-exact but named as
  the NP form): `seamcall`, `wrmsrlist`/`rdmsrlist`, `eretu`/`erets`, `mcommit`, `rmpadjust`/
  `rmpupdate`, `psmash`/`pvalidate`, plus `setssbsy`/`clui`/`stui`.
* **6 W/L/prefix mnemonic bugs**: VEX.W1 `vpsrlvq`/`vpsllvq` and EVEX.W1 `vpermq`/`vpermpd` (the W1
  twins were missing, so W1 took the W0 descriptor); `vzeroall` (L not in the VEX key -> vex_finalize
  promotes on L=1); `pause` (`F3 90`, C++ reclassify + encode one-off).

Deliberate divergences (left as-is, matching XED):
* **92 Knights Corner (KNC / Xeon Phi)** encodings Zydis decodes but XED rejects — `clevict0/1`,
  `vprefetch*`, `delay`, `spflt`, `kconcat*`/`kextract`/`kandnr`/`kmerge2l1*`, VEX `lzcnt`/`popcnt`/
  `tzcnt`/`tzcnti`, `jkzd`/`jknzd`. A discontinued CPU whose VEX opcodes sit in otherwise-`#UD`
  slots; per the user's call these stay `#UD` (see §8). `prefetchit0/1` (`0F 18 /6,/7`) likewise stays
  `nop` — XED itself renders them `nop` (forward-compatible hint), so it is not a divergence.
* **`pcommit`** (`66 0F AE F8`): removed from the ISA; XED (and this decoder) render it `sfence`.

Known remaining (byte-exact today via placeholder / base name — not yet semantically named):
* **APX new instructions**: `cfcmov<cc>` (map-4 `40-4F`, currently shown as the promoted `cmov<cc>`),
  `setzu<cc>`, `imulzu` (map-4, shown as the `vex` placeholder), EVEX `vmovd`/`vmovq` mem (`66.0F.W0/
  W1 7E @addr`, placeholder), VEX `vsha512msg1/2` (`F2.0F38.W0 CC/CD`, placeholder). These need
  per-instruction EVEX/APX/VEX rules with exact operand widths, each XED-verified.
* **Legacy REX.W size mnemonics**: `movq` (`0F 6E/7E`), `cmpxchg16b` (`0F C7 /1`), `wrssq` (`0F38 F6`)
  render the W0 name. The opsize-mnemonic table (MNSEL mode 1, `sret`/`incssp`-style) needs extending
  to `/digit`-group and pp-guarded `@addr` rules in `gen.py` (it currently only fires on simple and
  fixed-ModR/M-reg rules). All are byte-exact; only the displayed size suffix differs.

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

## 10. Status of the completion plan

1. ~~Finish EVEX maps 1–3 by family (§6).~~ **Done** — 0-gap vs objdump.
2. ~~FP16 maps 5/6 rules + `evexp0` routing.~~ **Done** — 96/96, 0 gaps (§7.1).
3. ~~`urdmsr` reg-form descriptor-direction fix.~~ **Done** — per-mod `ppdesc` direction,
   plus the VEX-map7 `urdmsr`/`uwrmsr` imm forms (§7.2).
4. Remaining optional polish: a strict-objdump display mode (§7.4) and a longer fuzz soak
   (≥ 50 M iters) — neither changes decodable-opcode coverage, which is now complete under
   the byte-exact bijection invariant (every differential sweep drift-0, 0 gaps).

**Done since first draft:** the encoder's `vex_structure` tail table is now generated from
the corpus (§5), and the FSM state ceiling is removed via a `uint32_t next` widening (§7.1).
