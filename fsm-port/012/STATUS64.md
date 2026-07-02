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
