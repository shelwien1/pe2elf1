# Full instruction parsing on the corpus-driven FSM

How to grow the current prefix + addressing decoder (`corpus.p` → `gen.py` →
`x86_tables.h` → `parser.cpp`) into a complete x86-32 instruction decoder —
opcode → mnemonic + operands — **without adding a new mechanism**. An opcode
byte is the same kind of byte-indexed transition as a prefix or ModR/M byte, and
the operand decoders we already have (ModR/M / SIB / displacement) are exactly
the sub-machines a full decode needs. Everything still originates in `corpus.p`.

The invariant does not change: `run_fsm()` knows no x86; it consumes bytes, runs
`Action`s into the capture array, follows `next`, and halts. All architecture is
data in the `FSM` tables. A separate render step turns captures into the
`x86insn_t` record and into text.

---

## 1. What already exists

The substrate is complete and reusable as-is:

* **`DState`** — `{ uint16_t next; Action act[FSM_MAX_ACT]; }`. `next` is a base
  index into the flat `FSM` (or `FSM_HALT`); the action list ends at `ACT_NONE`.
* **`Action` (16 bits)** — `op:3, dst:5, arg0:5, arg1:3`, with
  `FIELD` (extract bits arg0:arg1 of the consumed byte),
  `CONST` (set a capture), `APPEND` (consume N bytes, endian-aware, sign-extended),
  `MARK` (record the byte offset).
* **Captures** — one int array spanning the prefix vars (`VAR_*`), the addressing
  captures (`CAP_REG/RM/BASE/INDEX/SCALE/DISP/SEG/MODE`) and the per-group prefix
  offsets (`CAP_POFF + group`).
* **Tables** — `prefix[256]`, `modrm[2][256]` (by adrsiz), `sib[3][256]` (by mod),
  contiguous in one `struct Fsm`, indexed flat via `FSM_AT`.
* **`run_fsm(base, s, len, ip, cap)`** — the whole driver.
* **Render** — `fill_insn()` maps captures → `x86insn_t`; `fmt_mem()` /
  `reg_name()` produce text (size chosen by `opsize`/`addr`).

The addressing decoder is already a finished operand sub-machine: feed it a
ModR/M byte and it captures base/index/scale/disp/seg/mode and consumes SIB and
displacement bytes. Full decoding *reaches* this sub-machine from opcode states
the same way ModR/M reaches the SIB table today.

---

## 2. The one structural change: prefix → opcode handoff

Today the prefix table's terminal (a non-prefix byte) is a dead state
(`act[0].op == ACT_NONE && next == FSM_HALT`), and the demo `main()` papers over
the missing opcode stage with `i += 1`. That hack disappears.

Replace the prefix terminal with a real transition into the one-byte opcode map:

```
prefix[b] for a non-prefix b:   next = FSM_OP1     (was FSM_HALT, no actions)
```

Now `run_fsm(FSM_PREFIX, …)` flows prefixes → opcode → operands in one walk, and
the consumed `ip` at `FSM_HALT` is the exact instruction length — once trailing
immediates are consumed via the addressing sub-FSM's return path (§5.1).
`parse_prefixes` / `parse_addr` collapse into a single `decode()` entry point.

---

## 3. Opcode maps are just more DState tables

Add opcode tables to `struct Fsm` exactly like `modrm`/`sib`:

```c
struct Fsm {
    struct DState prefix[256];
    struct DState op1[256];        // one-byte opcodes
    struct DState op0f[256];       // 0F  map
    struct DState op0f38[256];     // 0F 38 map
    struct DState op0f3a[256];     // 0F 3A map
    struct DState modrm[2][256];
    struct DState sib[3][256];
    // (later) vex[…], evex[…]
};
```

Base indices stay `offsetof`-derived (`FSM_OP1 = FSM_INDEX(op1)`, …); nothing is
hand-numbered. Escapes are ordinary transitions:

```
op1[0x0F]    next = FSM_OP0F
op0f[0x38]   next = FSM_OP0F38
op0f[0x3A]   next = FSM_OP0F3A
```

A two-byte opcode `0F 2E` is therefore: `op1[0x0F] → op0f[0x2E]`, and the
`op0f[0x2E]` state does the mnemonic/operand work. Three-byte maps chain once
more. No special casing in the driver — it already follows `next`.

---

## 4. The `insn` submatch is the source

`corpus.p` already describes every instruction declaratively. `gen.py` interprets
it the same way it interprets `addr1`/`sib0`/`pfx` today. Each rule is

```
<opcode bytes/bitfields>  <operand encoding>  =>  <mnemonic>  <operand template>
```

Examples (verbatim from `corpus.p`):

```
0x8b 11 ggg rrr => "mov " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
0x8b @addr      => "mov " greg[$opsiz*8+$g] "," $addr ;
0x0f 0x10 @addr => m10[$reptype] sfx[m10sz[$reptype]] " " ssereg[8+$g] "," $addr ;
0xc7 11 000 rrr @immz => "mov " greg[$opsiz*8+$r] "," hex($immz) ;
0x81 @addr(0)   @immz => "add" sfx[4] " " $addr "," hex($immz) ;
0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;
0x05 @immz      => "add " greg[$opsiz*8+0] "," hex($immz) ;
10111 bbb @immz => "mov " greg[$opsiz*8+$b] "," hex($immz) ;
0xeb @rel8      => "jmp " hex($rel8) ;
```

The LHS (opcode + operand encoding) determines the **FSM state and transitions**;
the RHS (mnemonic + operand text) determines the **render descriptor**. `gen.py`
builds an opcode state per opcode byte by interpreting the LHS, and emits a
per-instruction descriptor by interpreting the RHS — just as `modrm_state` today
turns an `addr1` rule into actions + `next`.

---

## 5. Opcode LHS → state, transitions, captures

Each `insn` rule's LHS is a byte pattern, optionally followed by an operand
encoding. The opcode byte's state captures whatever bitfields are embedded in the
opcode and routes to the operand sub-machine:

| LHS form                         | meaning                          | state action / transition |
|----------------------------------|----------------------------------|---------------------------|
| `0x8b 11 ggg rrr`                | reg-direct r/m                   | reached via ModR/M; `CAP_MODE=RM_REG`, `CAP_REG=g`, `CAP_RM=r` |
| `0x8b @addr`                     | ModR/M memory operand           | opcode state `next = FSM_MODRM`; the addr sub-FSM runs |
| `0x81 @addr(0)`                  | group `/digit`: `@addr(N)` fixes reg = N | `next` → the opcode's reg-fixed ModR/M stage; mnemonic baked per reg (§9) |
| `10111 bbb` (`0xB8+rd`)          | register in opcode              | opcode state `FIELD CAP_REG = bits 2:0` |
| `0111 cccc` (`0x70+cc`)          | condition in opcode             | `FIELD CAP_COND = bits 3:0` |
| `0x0f 1001 cccc`                 | 0F map, condition in low nibble | `op0f` state, `FIELD CAP_COND` |
| `@imm8 / @immz`                  | trailing immediate              | `APPEND CAP_IMM` — at the opcode state for no-ModR/M ops, else via the §5.1 return path |
| `@rel8 / @relz`                  | branch displacement             | `APPEND CAP_REL` |
| `0xea @imm32 @imm16`             | far pointer ptr16:32            | two `APPEND`s (offset, selector) |

Two new captures cover the operand payloads: **`CAP_IMM`** and **`CAP_REL`** (and
`CAP_COND` for condition codes, `CAP_OP2IMM`/far-selector if you keep `enter`/far
pointers exact). `APPEND` already does the byte consumption and sign-extension;
`immz`/`relz` width = `opsize ? 2 : 4`, which is a per-opcode constant the state
knows (it can emit two variants, one per opsize, reached through a 66-routed
opcode subtable — see §8 — or pick the width at render from `VAR_OPSIZ`).

The crucial reuse: **r/m operands need no new code.** `@addr` → `next = FSM_MODRM`
and the existing machine captures everything. Reg-direct (`11 ggg rrr`) is the
ModR/M `mod == 11` path we already decode (`CAP_MODE = RM_REG`).

### 5.1 Returning from the addressing sub-FSM (trailing immediates)

Where the immediate lands depends on whether the opcode has a ModR/M byte:

* **No-ModR/M opcodes** (`B8+rd`, `05`, `EB`, `E9`, `EA`, `C2`, `CD`, …): the
  immediate/relative bytes follow the opcode directly, so the **opcode state
  appends them itself** (`APPEND CAP_IMM`/`CAP_REL` in its own action list).
* **r/m + immediate opcodes** (`C7`, `81`, `83`, `C0/C1`, `69`, …): the immediate
  comes **after** ModR/M+SIB+disp, which the *shared* `modrm`/`sib` sub-FSM
  consumes and then terminates. The opcode state runs *before* that sub-FSM, so it
  cannot append the immediate, and the shared addressing states can't carry an
  opcode-specific `APPEND`. The decode has to *return* from addressing to an
  opcode-chosen continuation.

Give the addressing sub-FSM a return path — one capture, one sentinel, two lines
in the driver:

```
CAP_CONT                      // continuation index, default FSM_HALT
FSM_RET                       // next-sentinel (e.g. 0xFFFE)

modrm/sib terminals:  next = FSM_RET           (was FSM_HALT)
run_fsm:  base = (st->next == FSM_RET) ? cap[CAP_CONT] : st->next;
```

An opcode that needs a trailing immediate sets `CONST CAP_CONT = <append-imm
state>` before `next = FSM_MODRM`; that state does `APPEND CAP_IMM` and halts.
Opcodes with nothing after addressing leave `CAP_CONT = FSM_HALT` (its default),
so the addressing decode halts exactly as today — the standalone `parse_addr`
path is unchanged, because it never sets `CAP_CONT` and `FSM_RET → FSM_HALT`.
With this in place the immediate is consumed inside the FSM walk, so `ip` at the
final `FSM_HALT` is still the full instruction length (§13).

---

## 6. Operand RHS → an operand template

The RHS names, per operand, a register file and a field. These map to a small
fixed vocabulary of **operand specs** that the render step turns into `op[]`:

| RHS fragment                | operand spec                                   |
|-----------------------------|------------------------------------------------|
| `greg[$opsiz*8+$g]`         | GPR, operand-size, from `CAP_REG`              |
| `greg[$opsiz*8+$r]`         | GPR, operand-size, from `CAP_RM` (reg-direct)  |
| `rgb[$r]` / `rgb[$g]`       | GPR8                                           |
| `ssereg[8+$g]`              | XMM, from `CAP_REG`                            |
| `ssereg[$g]`                | MMX, from `CAP_REG`                            |
| `sreg[$g]`                  | SREG                                           |
| `$addr`                     | the one memory/RM operand (addr sub-FSM)       |
| `hex($immz)` / `hex($imm8)` | immediate, from `CAP_IMM`                      |
| `hex($rel8)` / `hex($relz)` | rel displacement (target = end+disp at render), `CAP_REL` |
| `greg[$opsiz*8+0]`          | implicit eAX                                   |
| `"al"`, `"dx,al"`, `1`, `CL`| implicit operands (CONST in the spec)          |

So an instruction is `{ mnemonic-descriptor, operand-spec[0..n] }`. This is a
per-instruction constant emitted from `corpus.p`, indexed by a captured
**`CAP_INSN`** (the matched rule's id). `x86insn_t` already has the storage:
`mnem`, `op[5]` (`type` + `index`), `imm`, `disp`, and the mem fields. `fill_insn`
walks the operand specs and assembles `op[]` from the captures.

Note the register-file choice is *static per opcode* (the opcode picks GPR vs XMM
vs GPR8); only the *width* within a file is dynamic (opsize). So the spec stores
the file + field; the renderer adds the `opsize*8` (or `addr*8` for addresses)
offset using the same `reg_name()` idea already in `parser.cpp`.

---

## 7. Mnemonic selection (resolve at render, like `seg[]`)

The RHS mnemonic comes in three shapes, all visible in `corpus.p`:

1. **literal** — `"mov "`, `"lea "`.
2. **prefix-indexed table** — `m10[$reptype]`, `uc[$opsiz]`, `movs[$opsiz]`,
   `cdqw[$opsiz]`, `"add" elt[$reptype*2+$opsiz]`.
3. **field-indexed concatenation** — `"j" cond[$c]`, `"set" cond[$c]`,
   `"cmov" cond[$c]`.

Resolving these inside the FSM would need an action that indexes a table by a
*capture* (call it `ACT_SELECT: cap[dst] = table[cap[src]]`). That is a clean
option and worth adding if you want the machine to fully resolve the mnemonic id.

The lighter path mirrors the segment string, which we already resolve at render
(`seg[seg_row + segidx]`): give each instruction a **mnemonic descriptor**
`{ base, selector }` where `selector` names which capture drives the sub-index
(`none`, `reptype`, `opsiz`, `cond`, `reptype*2+opsiz`). Render computes
`mnem = mtab[base + sub(selector, caps)]`. Literal mnemonics use a one-entry
group; the `cond`/`reptype`/`opsiz` families fall out of the same table lookup.
This keeps `run_fsm` free of table-indexed-by-capture logic and matches the
existing seg/`opsize`/`addr`-at-render style.

Either way the mnemonic *strings* (`m10`, `uc`, `cond`, `sfx`, `elt`, …) are
emitted as ordinary string tables from `corpus.p`, exactly like `greg`/`seg` now.

---

## 8. Mandatory prefixes and 66/F2/F3 routing

For SSE, `66`/`F2`/`F3` are *mandatory prefixes* that select a different opcode,
not legacy prefixes. `corpus.p` models this with prefix-indexed mnemonic tables
(`m10[$reptype]`, `m54[$opsiz]`): the prefix already set `reptype`/`opsiz`, and
the single `op0f` state renders the right mnemonic from those captures.

Two ways to carry the distinction into the FSM:

* **Render-time (simplest, already the corpus's model):** keep one `op0f` state
  per opcode; the mnemonic descriptor's selector reads `reptype`/`opsiz`. This is
  what the existing `m10[$reptype]` / `uc[$opsiz]` rules do, and it needs nothing
  new — the prefix captures are already there.
* **Table-routed (if you want distinct states/operands per mandatory prefix):**
  give the 0F map a prefix dimension `op0f[mpfx][256]` (like `modrm[adrsiz]`),
  and have the prefix stage select the start index. Use this only where the
  *operand encoding* (not just the mnemonic) differs by mandatory prefix.

Same applies to operand-size-selected opcodes (`movs[$opsiz]`, `cdqw[$opsiz]`,
the `immz`/`relz` 2-vs-4-byte width): a render-time selector over `VAR_OPSIZ`,
or a 66-routed opcode subtable when the byte structure itself changes.

---

## 9. Group opcodes (`/digit`): `@addr(N)` fixes the reg field

Group opcodes (`0x80`, `0x81`, `0x83`, `0xC0/C1`, `0xD0`–`D3`, `0xF6/F7`,
`0xFE/FF`) select the operation from the ModR/M reg field. The corpus writes each
member as `@addr(N)` (memory form) or `11 NNN rrr` (reg-direct):

```
0xff @addr(0) => "inc"  …      // ggg fixed to 000 → inc
0xff @addr(4) => "jmp"  …      // ggg fixed to 100 → jmp
0xff @addr(6) => "push" …      // ggg fixed to 110 → push
```

`@addr(N)` is a **bidirectional-field constraint**: it force-fixes the ModR/M reg
field `ggg` to `N` — `@addr(6)` matches only bytes whose reg is `110` — exactly as
`addr1`'s `$g` parameter binds `ggg`. It is a *generator-time* fact, not a runtime
one: there is no `group_ff[CAP_REG]` render lookup, and the reg field is **not** an
operand here — it is the opcode extension.

The generator resolves the constraint straight into the FSM table. For a group
opcode it emits a **dedicated ModR/M stage** (`op_ff_modrm[256]`, reached by the
opcode's `next`). Every entry carries the same reg-independent addressing decode
(mod/rm → base/index/scale/disp, the SIB/disp transitions — the shared `addr1`
behaviour), plus, **baked from the entry's own reg bits**, the group mnemonic via
`ACT_MNEM` with the index computed at generation time (`000`→`inc`, `110`→`push`,
…). Reading the ModR/M byte then decodes the address and sets the mnemonic in one
lookup; reg values the group leaves undefined are dead-end entries. The `mod == 11`
reg-direct members live in the same table, since reg is bits 5:3 regardless of mod.
The cost is one 256-entry `DState` table per group opcode — the addressing actions
restate the shared decode and only the `ACT_MNEM` (and the continuation below) vary
across reg — which is the price of resolving the extension in the table instead of
at render.

**Trailing immediates fall out the same way.** Because each entry's behaviour is
fixed by its reg at generation time, the immediate continuation (§5.1) is too: an
entry whose rule carries `@immz`/`@imm8` sets `CAP_CONT` to an append state, one
whose rule does not leaves `CAP_CONT = FSM_HALT`. So the `F6/F7` wart — `/0,/1`
(`test`) take an immediate while `not/neg/mul/imul/div/idiv` do not — needs no
special finalize: the generator bakes the immz continuation into the `reg ∈ {0,1}`
entries and not the rest. The uniform groups (`80/81/83/C0/C1` all-immz, `FE/FF`
none) are just the case where every reg sets the same `CAP_CONT`. (The corpus shows
only `F7 /4 mul`, so its visible subset looks uniform; the full group is not.)

---

## 10. New captures and (optional) action

Additions to the capture enum:

```
CAP_INSN     // matched insn-rule id  -> mnemonic descriptor + operand specs
CAP_IMM      // immediate value      (APPEND)
CAP_REL      // rel displacement     (APPEND; target = end+disp at render)
CAP_COND     // condition code       (FIELD from opcode byte)
CAP_CONT     // post-addressing continuation index (§5.1; default FSM_HALT)
```

Plus one `next`-sentinel, `FSM_RET` (§5.1), alongside `FSM_HALT`.

`NCAPS` stays small (≪ 32), so `Action.dst` (5 bits) still fits. The only
candidate new primitive is `ACT_SELECT` (table-indexed-by-capture) for §7's
in-FSM mnemonic resolution; it is optional — render-time resolution needs no new
op. `op` is 3 bits (room for 8 ops; 5 used), so adding `ACT_SELECT` is free.

`x86insn_t` already carries the rest: `mnem`, `n_ops`, `op[5]`, `imm`, `disp`,
`rep`/`lock` (from the prefix vars), and the mem descriptor.

---

## 11. Render: captures → `x86insn_t` → text

`fill_insn` grows from "fill the one r/m operand" to "walk the operand specs":

```
in->mnem = resolve_mnem(CAP_INSN, caps);          // §7
for each operand spec of CAP_INSN:
    GPR/GPR8/XMM/MMX/SREG : op[k] = {file, field-capture + size offset}
    RM                    : op[k] = T_MEM (mem_* already captured) or
                                     T_GPR (reg-direct, CAP_RM)
    IMM                   : in->imm  = CAP_IMM ; op[k] = T_IMM
    REL                   : in->imm  = CAP_REL ; op[k] = T_REL
    implicit              : op[k] = CONST from the spec
```

Text formatting reuses what exists: `reg_name(reg, size16)` for registers (size
from `opsize` for operands, `addr` for address registers), `fmt_mem` for the
memory operand, and `sfx[]` for the operand-size suffix on mem-form ops. The
final string is `mnemonic + sfx + " " + operand,operand,…`.

---

## 12. VEX / XOP / EVEX (sketch)

`C5` (2-byte VEX), `C4` (3-byte VEX), `8F` (XOP), `62` (EVEX) are not legacy
prefixes; each introduces a fixed run of bytes encoding `vvvv`, `L`, `pp`
(implied 66/F2/F3), `mm` (implied 0F / 0F38 / 0F3A), `W`, and (EVEX) `aaa`/`z`/
`b`/`V'`. Model each as its own small sub-FSM reached from `op1[0xC5/0xC4/0x8F]`
and `op1[0x62]`:

* `FIELD` the VEX/EVEX bitfields into captures (`CAP_VVVV`, `CAP_L`, `CAP_PP`,
  `CAP_MM`, `CAP_W`, …) across the 1–3 prefix bytes.
* Then transition into the opcode map selected by `mm` (`FSM_OP0F` / `OP0F38` /
  `OP0F3A`), with `pp` playing the mandatory-prefix role and `vvvv` supplying the
  extra (non-destructive) register operand.

This is the same pattern as the legacy prefix stage (consume structured bytes,
set captures, hand off to the opcode map) — larger, but no new mechanism. (Your
existing `gen_vtext.py` / `vtext.hpp` work is the text side of this; the FSM side
is the byte-consumption + capture side.)

---

## 13. Length and termination

There is no separate length pass. Each instruction's last sub-machine
(`APPEND` of the final imm/disp, or the reg-direct terminal) has `next =
FSM_HALT`. When `run_fsm` reaches it, `ip` is the instruction length. Truncated
input (`ip + n > len` inside `APPEND`) already stops cleanly; a decode that
reaches `FSM_HALT` before consuming a required operand, or that runs off a map
with no matching opcode state, is the natural "bad/incomplete" signal (an opcode
byte whose state is a dead-end, `act[0]==ACT_NONE && next==FSM_HALT`).

---

## 14. `gen.py` work items

The generator changes are additive and parallel the existing addr/sib/prefix
interpreters:

1. **Parse `insn`** like `addr1`/`pfx`: split rules, parse each LHS byte pattern
   (now allowing multiple opcode bytes and embedded fields `bbb`/`cccc`), and the
   operand encoding (`@addr`, `@addr(N)`, `11 ggg rrr`, `@imm*`, `@rel*`).
2. **Build opcode states** `op1`/`op0f`/`op0f38`/`op0f3a`: per opcode byte, emit
   the field captures, the operand transition (`FSM_MODRM` or inline
   `APPEND`s), and `CONST CAP_INSN = <rule id>`. Escapes (`0F`, `38`, `3A`) are
   transitions.
3. **Emit descriptors** from each RHS: the mnemonic descriptor (`{base,selector}`
   or a resolved id) and the operand-spec list, as `static const` tables indexed
   by `CAP_INSN`.
4. **Emit the string tables** already named in `corpus.p` (`rgb`, `ssereg`,
   `sreg`, `cond`, `sfx`, `m10`, `uc`, `elt`, `movs`, `cdqw`, …) verbatim, exactly
   as `greg`/`seg` are emitted now.
5. **Wire prefix → `FSM_OP1`** (replace the prefix terminal's `FSM_HALT`).
6. Optionally add `ACT_SELECT` if resolving mnemonics in the FSM.

The `self_check` extends to run the full machine over a corpus of encodings and
diff the rendered text against an oracle (Zydis, as in the addressing work):
generate each `insn` rule's representative bytes, decode, and compare. The
divergence-worklist process applies unchanged.

---

## 15. Incremental milestones

Each step is independently testable against the oracle; the machine stays valid
throughout.

1. **Handoff** — prefix terminal → `op1`; collapse `parse_prefixes`/`parse_addr`
   into one `decode()`. The demo stops hard-coding the opcode skip.
2. **One-byte r/m ops** — `mov`/`add`/`lea` families (`88..8B`, `01/03`, `8D`):
   opcode state + `FSM_MODRM`, operand specs, GPR render. Reg-direct and memory
   both fall out of the existing ModR/M machine.
3. **Immediates** — `CAP_IMM`, `APPEND`; `B8+rd`, `C6/C7`, `04/05/80/81/83`.
4. **Embedded reg / cond** — `CAP_COND`, `FIELD` from opcode; `40+`, `50+`,
   `70+cc`, `0F 90+cc`, `0F 40+cc`.
5. **Branches** — `CAP_REL`; `EB/E9/E8`, far `EA`, `C2/C3`, `CD`.
6. **0F map** — `op0f` table + the `0F` escape; `movzx`/`movsx`/`cpuid`/`set`/
   `cmov`.
7. **Groups** — `FF`/`F7`/`C1`/`80/81/83`: `@addr(N)` fixes the reg field, the
   generator bakes a per-reg `ACT_MNEM` (and continuation) into a dedicated ModR/M
   stage (§9).
8. **Mandatory-prefix SSE** — `0F 10/2E/54/58/6F/FC/28`, render-time `reptype`/
   `opsiz` selectors (or 66-routed subtables where operands differ).
9. **0F 38 / 0F 3A** — two more maps, two more escapes.
10. **VEX / XOP / EVEX** — the structured-prefix sub-FSMs and `mm`/`pp` routing.

At every milestone the rule holds: the driver is unchanged, the new behaviour is
new **data** in `FSM`, and every architectural fact came from `corpus.p`.
