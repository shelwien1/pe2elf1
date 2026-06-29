# Plan — porting the corpus-driven (dis)assembler from x86-32 to x86-64

## 0. Goal

Make the table-driven decoder/encoder handle **x86-64 (long mode)** while keeping
the existing property: `c(d(bytes)) == bytes` for every covered instruction
(byte-exact round-trip, canonical *and* non-canonical), with all architectural
facts still living in `corpus.p` and the C++ staying architecture-agnostic.

x86-64 adds, over x86-32:

* the **REX** prefix group (`0x40..0x4F`): `W` (64-bit operand), `R/X/B`
  (a 4th bit for ModR/M.reg, SIB.index, ModR/M.rm or SIB.base), and *presence*
  alone (turns `ah/ch/dh/bh` into `spl/bpl/sil/dil`);
* **16** general/xmm registers (`r8..r15`, `xmm8..xmm15`), 32 for EVEX;
* **64-bit default** operand size for the stack/near-branch group, and a 64-bit
  operand size selectable by `REX.W`; `movsxd`; `mov r64, imm64` (`movabs`);
* **RIP-relative** addressing (`mod=00 rm=101` → `[rip+disp32]`), absolute via the
  SIB no-base form, and base/index registers `r8..r15`; no 16-bit addressing
  (the `67` prefix now selects 32-bit, not 16-bit);
* a set of opcodes that are **removed / repurposed** in 64-bit mode
  (`06/07/0E/16/17/1E/1F` push/pop seg, `27/2F/37/3F` BCD, `60/61` pusha/popa,
  `62` is now EVEX, `82` alias, `9A/EA` far call/jmp imm, `C4/C5` are now VEX,
  `D4/D5` aam/aad, single-byte `inc/dec 40..4F` are now REX).

What does **not** change: the FSM driver (`run_fsm`), the capture model, the
prefix→opcode→ModR/M→immediate routing, and the byte-exact **round-trip of the
VEX/EVEX/XOP path** — those prefix bytes are captured raw (`vex1/vex2/vex3`) and
replayed verbatim by `vex_decode`/`vex_encode` (`x86dec.hpp`/`x86enc.hpp`), so
all of `R/X/B/R'/V'/vvvv` already survive re-encoding regardless of mode, and the
operand *type* is already set from `L` (xmm/ymm/zmm). What that path does **not**
do yet is *lower* the high register bits into the semantic operand: `vex_decode`
calls `fill_insn`, which stores only the low 3 ModR/M bits into `op[].index`
(`x86dec.hpp:100,104`), and `vvvv` is not decoded into an operand at all — so
*printing* `xmm8..31` or the `vvvv` source needs work in this port, but the
**bytes already round-trip** because the raw prefix carries the high bits. The
64-bit work is therefore almost entirely the **legacy + 0F** path plus the REX
prefix (the VEX/EVEX tail layout is already mode-independent).

A second, independent x86-64 description of the same instruction set already
exists in this repo (`/corpus64.p`, driven by the sibling Python toolchain
`parsergen.py`/`asm.py`). It is a **reference for the architectural facts**
(REX layout, register-bank indexing, RIP-relative/absolute-SIB addressing,
default-64 groups, `movsxd`/`movabs`, removed opcodes) — *not* a drop-in, because
the DSL dialect differs (it carries the round-trip residual as `wit()` tokens,
whereas this project carries it in the `x86insn_t` witness fields and recovers
prefixes by §6.4 token-suppression).

---

## 1. Pipeline recap (what touches what)

```
gen_rules.py --(mkcorpus.py)--> corpus.p   (prelude hand-kept; insn body generated)
corpus.p     --(gen.py)-------> x86_tables.h        (decode FSM:  bytes -> x86insn_t)
corpus.p     --(gasm.py)------> x86_tables_enc.h    (encode:      x86insn_t -> bytes)
x86insn.hpp                      the record (bijection endpoint)
x86dec.hpp / x86enc.hpp          generic FSM decode + data-driven encode (C++)
asm32.cpp                        file-to-file tool;  parser.cpp/test.cpp/fuzz.cpp harnesses
```

Where x86-32 is baked into the baseline (line refs are the pre-port commit
`758e147`; the `record` row is already addressed by Phase 1 below):

| place | x86-32 assumption | file:where |
|-------|-------------------|-----------|
| record | reg index 3 bits (0..7); `mem_base/index` 4 bits; `opsize` 1 bit | `x86insn.hpp:38,52-56` |
| decode | `GREG_NONE=15` (4-bit), 8-entry reg tables, `store_reg()` folds 8..15→0..7 | `gen.py` reg handling |
| decode | `adrsiz` 0=32 / 1=16; 16-bit `rm16`; no RIP slot | `gen.py` modrm/sib states |
| decode | no REX var in the prefix run | `gen.py` prefix_table |
| C++ | `& 7` operand masks; `addr==1`→16-bit witness; no REX parse | `x86dec.hpp:178-181,154-158` |
| C++ | `(reg<<3)|(rm&7)`, no REX byte emitted | `x86enc.hpp:275,311` |
| encode | candidate `op` is one byte, no REX derivation | `gasm.py` |

---

## 2. The record first (Phase 1 — `x86insn.hpp`)

The record is the bijection endpoint, so it is widened first. Changes:

* `x86op_t.index`: **3 → 5 bits** (0..31 — covers 16 GP/xmm and 32 zmm). The
  struct grows to a `uint16_t` bitfield (`type:5 + index:5`); `sizeof(x86op_t)`
  goes 1 → 2 bytes.
* `mem_base`, `mem_index`: **4 → 5 bits** (0..15 plus `GREG_NONE` ≥ 16). The
  current `uint16_t` packing of the mem/flags group overflows, so it moves to a
  `uint32_t` group.
* `opsize`: **1 → 2 bits** — `0`=default(32), `1`=16 (`66`), `2`=64 (`REX.W`).
  This is what resolves `T_GPR` width and the `immz` width (now 2/4/4, with a
  separate `immq`).
* add `rex : 1` — a REX byte was present (records the bare-`0x40`/`spl..dil`
  case; `W` is carried by `opsize==2`, and `R/X/B` are re-derived from operand
  register numbers ≥ 8, so the REX byte is reconstructable from
  {present, opsize, operands}).
* add `rip : 1` — the memory operand is `[rip+disp32]` (base/index ignored).
* widen `reg_w` (dead ModR/M.reg witness) **3 → 4 bits** to hold the REX.R-extended
  dead reg of single-r/m forms.

This change is **backward-compatible with the current x86-32 tables** (all
x86-32 values fit), so the success test for Phase 1 is: rebuild and confirm the
x86-32 round-trip is still byte-exact (`make roundtrip`, `make x8632all`).
Deferred refinement: a 4-bit `rex_wrxb` witness for genuinely non-canonical REX
(e.g. a dead REX.R on a no-reg form) — only if such encodings must round-trip.

---

## 3. corpus.p prelude (Phase 2 — hand-edited part)

* `arch $mode=64 $endian=le $bitorder=msb $maxlen=15`.
* `vars`: add the REX state — `$rexw=0 $rexr=0 $rexx=0 $rexb=0 $rex=0`.
* **Register tables** widened, with the size bank selected by operand size:
  * `greg` → 64-bit bank (`rax..r15`) + 32-bit bank (`eax..r15d`) + 16-bit bank
    (`ax..r15w`), indexed by `(size-bank, hi, lo)` where the high bit `hi` is
    `REX.R` for a ModR/M.reg operand and `REX.B` for an rm/base operand (the bank
    comes from `$rexw`/`$opsiz`). Mirror the sibling `corpus64.p` 64-entry layout
    `greg[32*$rexw + 16*$opsiz + 8*hi + lo]`.
  * `rgb` → 8-bit registers; **REX-present** selects the `spl/bpl/sil/dil`
    naming for indices 4..7 and exposes `r8b..r15b` (so `rgb[16*$rex + 8*hi + lo]`).
  * `ssereg`/xmm → extend the legacy SSE path to 16 xmm (`xmm0..xmm15`, high bit
    from `REX.R`/`REX.B`); the 32 EVEX registers stay on the C++ VEX path.
* **Addressing** (`addr1`/`sib0`/`sib1`): a 64-bit branch (`$adrsiz==0`):
  * `mod=00 rm=101` → `[rip+disp32]` (NOT `[disp32]`);
  * absolute `[disp32]` is the SIB `base=101,index=none` form;
  * base/index extended by `REX.B`/`REX.X` (`areg[8*$rexb + b]`,
    `areg[8*$rexx + i]`);
  * the `67` prefix selects the **32-bit** address branch (`$adrsiz==1`), reusing
    32-bit-style SIB/disp but with the 8 (or 16 w/ REX) 32-bit address regs;
  * the 16-bit `rm16` forms are dropped from 64-bit mode.
* **Default-64 group**: `push/pop/call/jmp near/ret/leave/...` default to 64-bit
  operand size regardless of `REX.W`; model with a separate register bank/table
  as `corpus64.p`'s `dreg` does.
* **Immediates**: `immz` = 2 (`66`) / 4 (default) / 4 (`REX.W`, sign-extended);
  `mov r64,imm64` (`movabs`) uses the operand-size immediate `immv` =
  2 / 4 / 8 by `66` / default / `REX.W`. *Implemented as `IMK_IMMV`: REX.W is
  not an FSM var, so the width is finalized C++-side (decode passes the
  effective-last-prefix REX.W into `append_imm`; `enc_imm` mirrors it from
  `opsize`); the `imm` field is `int64_t` so the 64-bit value fits.*

The **REX prefix run**: add REX frames to `pfx` that set `$rexw/r/x/b/$rex` and
emit **no token** (its bits are recovered from register names, exactly as `66`
is recovered from a 16-bit register — §6.4). Because REX must be the *last*
prefix before the opcode, the cleanest model is a dedicated REX stage entered
after the legacy prefix run (see Phase 5 for the FSM shape).

---

## 4. gen_rules.py (Phase 3 — generated insn body)

The instruction body is regenerated by `gen_rules.py`. Port it to emit x86-64:

* register operands index the widened banks (`greg[$rexw...]`, REX.R/B on
  reg/rm), 8-bit operands via the REX-aware `rgb`;
* default-64 stack/branch ops use the default-64 bank;
* `movsxd r64, r/m32` (`0x63`, replaces `arpl`); `mov r64, imm64` (`movabs`,
  `B8+r` with `REX.W` → `immv`/`IMK_IMMV`); `xchg`-form `nop`/`pause`;
* the string ops, `cbw/cwde/cdqe`, `cwd/cdq/cqo` gain the `REX.W` (`q`) variant;
* **omit the opcodes that are illegal in 64-bit mode** (BCD, pusha/popa, push/pop
  seg via 06/07/…, far imm call/jmp, aam/aad) — they must *not* appear, since
  `40..4F`, `62`, `C4`, `C5`, `82` are now REX/EVEX/VEX/alias.

Keep the two-rule ModR/M convention (one `mod=11` register rule, one `@addr`
memory rule) and the prefix-indexed mnemonic tables.

---

## 5. gen.py (Phase 4 — decode FSM compiler)

* read `$mode`; when 64, change `GREG_NONE` to **16** (5-bit sentinel) and emit
  the 64-bit register/addressing logic.
* **REX prefix**: emit a REX stage. Two viable shapes —
  1. a 16-entry continuation table after the legacy prefix run, where each
     `0x40+n` state sets `$rexw/r/x/b` via `FIELD`/`CONST` actions and hands off
     to `FSM_OP1`; or
  2. keep REX inside the prefix run but mark it terminal-for-the-run (must be
     last). Shape (1) matches the architecture (REX immediately precedes the
     opcode) and is simplest.
* **register extension**: the ModR/M/SIB states must add the REX bit as the 4th
  (and EVEX 5th) register bit. Today `store_reg()` folds 8..15→0..7; instead
  capture the low 3 bits as now and let the **renderer/`fill_insn`** combine them
  with `$rexr/$rexx/$rexb` (`reg = 8*REX.R + modrm.reg`). The captures already
  exist (`CAP_REG/RM/BASE/INDEX`); add `CAP_REXR/REXX/REXB` (or fold the REX bit
  into the existing capture during the addressing states).
* **64-bit addressing**: a third `modrm`/`sib` mode for 64-bit (or reuse the
  32-bit state shape with the RIP slot for `mod=00 rm=101` and the
  REX.B/X-extended base/index). Emit `CAP_RIP` for the RIP-relative form.
* emit the new `#define`s (`GREG_NONE=16`, any `VAR_REX*`, `CAP_REX*`,
  `IMK_IMMQ`, `OPF_*` if a REX-aware 8-bit file is added) the C++ consumes.
* `--check` self-test extended with 64-bit ModR/M+SIB+REX coverage.

---

## 6. gasm.py (Phase 5 — encode candidate compiler)

* derive each candidate's **REX requirement** from its register files and
  operand sizes: emit `REX.W` when the candidate is the 64-bit-operand variant;
  `REX.R/X/B` are computed at encode time from the chosen operands' numbers, so
  the candidate table needs only a *"REX-capable"* marker plus the `op` byte.
* default-64 candidates record that they need no `REX.W` for 64-bit width.
* the `ENC_CAP` (≤ 8 candidates per shape) may need re-checking once the 64-bit
  twins (e.g. `movsxd` vs `movzx`, `B8+r imm32` vs `imm64`) are added.

---

## 7. x86dec.hpp / x86enc.hpp (Phase 6 — C++ driver)

Decode:
* `parse_prefixes` already runs the prefix FSM; after it, consume the REX byte
  (or let the REX stage do it) and set `insn.rex`, `insn.opsize=2` on `REX.W`.
* `fill_insn`: `reg = 8*REXR + CAP_REG`, `rm/base/index` likewise; set
  `insn.rip` for the RIP form; drop the `& 7` truncation (use the full 5-bit
  number).
* `capture_addr_witness`: replace the `addr==1`→16-bit branch with the 64-bit
  rules (RIP `mod=00 rm=101`; SIB no-base → disp32); `addr==1` is now 32-bit.
* `finalize_insn`/`SETREG`/`SETRM`: mask to 5 bits, not 3.
* `vex_decode`: round-trips correctly today (raw replay), but for x86-64
  *disassembly* it must lower the VEX/EVEX `R/X/B/R'/V'` bits into `op[].index`
  (and decode `vvvv` into an operand) so `xmm8..31` and the NDS/NDD source print
  correctly. Encode is unaffected — the raw `vex1/vex2/vex3` bytes are replayed.

Encode:
* `encode_insn`: after selecting the candidate, **derive and emit the REX byte**
  when needed — `W` from `opsize==2`, `R` from `op[reg].index>>3`, `B`/`X` from
  the rm/base/index numbers, plus the recorded `insn.rex` for a bare REX; place
  it immediately before the opcode (after legacy prefixes).
* `enc_mem32` → a 64-bit emitter: RIP-relative (`mod=00 rm=101 + disp32`),
  absolute via SIB, base/index low-3 bits in ModR/M/SIB with the high bits
  having gone into REX. `enc_mem16` (16-bit addressing) is **unreachable in
  64-bit mode** and is retired; the `67`/32-bit-address case reuses the **32-bit**
  emitter (`enc_mem32` with 32-bit register names), not `enc_mem16`.
* `(reg<<3)|(rm&7)` → use low 3 bits; high bits are already in REX.

---

## 8. Testing (Phase 7)

* **x86-32 must not regress**: keep `corpus.bin`/`x8632all.bin` green at every
  step (the record change in Phase 1 is validated this way before anything else).
  Consider gating x86-64 behind the `arch` mode so a 32-bit build still exists,
  or branch the corpus.
* **x86-64 corpus**: assemble a `.s` with GNU `as`, extract `.text`
  (`objcopy -O binary --only-section=.text`) → `corpus64.bin`, exactly as the
  sibling `corpus64.*` tests were built; then `asm32 d corpus64.bin recs.dat`
  and `asm32 c recs.dat out.bin`, assert `out.bin == corpus64.bin`.
* **real-world**: decode the `.text` of a 64-bit binary (e.g. `/bin/ls`),
  require 0 undecodable, and re-encode byte-exact (the sibling toolchain already
  passes this and can serve as a differential oracle / disassembly cross-check).
* **fuzz**: extend `fuzz.cpp` to prepend random REX bytes; every accepted decode
  must re-encode identically.

---

## 9. Suggested order of work (each step independently testable)

1. **`x86insn.hpp`** widen registers/REX/RIP; rebuild; x86-32 round-trip still
   858/858 and 110072/110072. *(this commit)*
2. `gen.py`: accept `arch $mode=64`, emit `GREG_NONE=16` and a 64-bit register
   render path while still producing correct x86-32 output for `$mode=32`.
3. corpus.p prelude + `gen_rules.py`: REX prefix run, 16-register tables, 64-bit
   addressing, default-64 group — decode-only first (`make check`).
4. C++ decode (`x86dec.hpp`): REX parse + register/addr wiring; decode a 64-bit
   `.text` with 0 undecodable.
5. `gasm.py` + `x86enc.hpp`: REX derivation/emission, 64-bit ModR/M emitter;
   round-trip the x86-64 corpus byte-exact.
6. Non-canonical REX + addressing witness; fuzz; real-world `/bin/ls` round-trip.
7. (Optional) fold the VEX/EVEX coverage already proven in the sibling
   `corpus64.p` into this toolchain’s test corpus.

## 10. Risks / open questions

* **REX ordering**: REX must be the byte immediately before the opcode; a REX
  not in that position is just an ignored prefix. The FSM must only treat
  `0x40..4F` as REX when it is the last prefix — shape (1) in Phase 5 handles
  this by entering the REX stage only on the prefix→opcode boundary.
* **`67` semantics flip**: address-size override now means 32-bit; 16-bit
  addressing is gone. The `addr` witness bit keeps working (0=64, 1=32).
* **Non-canonical REX** (redundant `0x40`, dead REX.R) needs the `rex`/`rex_wrxb`
  witness to round-trip; canonical code needs only `rex` presence.
* **`ENC_CAP`**: the 64-bit opcode twins may exceed 8 candidates/shape; re-check.
* **Keeping x86-32 alive**: decide whether `$mode` switches the same generator/
  C++ at build time (preferred) or the 32-bit corpus is forked. The C++ is
  arch-agnostic, so mode-gating in `gen.py`/`gasm.py` plus the `arch` line is the
  low-risk route.
