# corpus-driven x86-32 (dis)assembler

A table-driven x86-32 prefix / opcode / ModR/M / SIB / displacement / immediate
decoder whose every architectural fact lives in one declarative source,
`corpus.p`. Nothing about x86 is hand-coded in the C++ decoder; it is a generic
byte-driven state machine that interprets tables generated from the corpus.

## Pipeline

    corpus.p  --(gen.py)-->  x86_tables.h  --(#include)-->  parser.cpp / test.cpp

* `corpus.p`            single source of truth: arch, vars, tables, submatches
* `corpus-p-syntax.md`  the corpus DSL reference
* `gen.py`              interprets corpus.p, emits the FSM tables; `--check` self-tests
* `x86_tables.h`        generated: one flat FSM + the string/number tables (do not edit)
* `x86insn.hpp`         the decoded-instruction record
* `parser.cpp`          the generic FSM driver + a small demo `main`
* `test.cpp`            validation harness (drives the real decoder over all bytes)

## Build

    make            # build parser + test
    make check      # gen.py self-check (decode tables + FSM simulation)
    make run        # run the demo
    make runtest    # run the validation harness

## Design

**One uniform state record.** Prefix, ModR/M and SIB tables are all `DState`,
stored in a single flat `FSM.s[]` array. `next` is a 16-bit *index* into that
array (not a pointer), so the whole machine is position-independent. Each state
carries a short list of 16-bit `Action`s:

* `FIELD`  -- capture bits hi:lo of the consumed byte
* `CONST`  -- set a capture to a literal
* `APPEND` -- consume N more bytes (endian-aware, sub-word sign-extended) into a capture
* `MARK`   -- record the consumed byte's offset (prefix-group bookkeeping)
* `MNEM`   -- set the mnemonic index (wide 13-bit const; for the `op1` map)

**The instruction map (`op1`).** The prefix run hands off to a one-byte opcode
table `op1[256]`. Each `op1` state classifies one opcode: it sets the mnemonic
(`MNEM` -> `mnem_tab[]`), any opcode-embedded register (`B8+r`, `40+r`, `50+r`),
an operand `FORM` (for the renderer) and an immediate `KIND`. `decode_insn()`
then routes exactly as `parse_addr` does -- a ModR/M form runs
`run_fsm(FSM_MODRM + adrsiz*256)`; otherwise the one trailing immediate is
appended (the `immz = 2/4` opsize rule is the entry's only added fact). Covered
so far: the single-byte (non-`0x0F`) opcodes without a group `/digit` extension
-- mov/add r/m forms, `lea`, embedded-reg `inc`/`push`/`mov r,imm`, the
accumulator-immediate and `jcc`/`jmp`/`call`/`int`/`ret` forms, and the no-operand
opcodes. Opcode-extension **groups** (`FF`/`81`/`83`/`C1`/`C6`/`C7`/`F7`) decode
through a *dedicated reg-fixed ModR/M stage* per group (`groups[gid][adrsiz][256]`):
`@addr(N)` fixes `ggg=N` at generator time, so each byte of the stage bakes its
own mnemonic from its reg field (undefined `/digit` extensions are dead states),
memory forms fold in the operand-size suffix (`inc.d`, `mov.b`), and the trailing
immediate is appended by the entry as usual (`83`'s imm8 sign-extends).

**Two-byte `0F` map.** `op1[0x0f]` is an escape that hands off to a second opcode
table `op2[256]` (read with the byte after `0F`); the same post-opcode routing
then applies. Covered: `movzx`/`movsx`, `setcc`/`cmov` (per-cond, baked per byte
like `jcc`), `cpuid`, and the MMX/SSE moves `movq`/`movaps`/`paddb` plus
`ucomiss`/`ucomisd` and `andps`/`andpd`. Each operand carries a **register file**
(`OPF_GREG`/`RGB`/`XMM`/`MM`/`SREG`, plus `SSE_OS` = mm-or-xmm by opsize), captured
from the corpus template, so the renderer names registers from the right table
(`movzx eax, cl` mixes greg and rgb; `66 0F FC`→`paddb xmm0`). Conditional
instructions keep the condition out of the mnemonic: `jcc`/`setcc`/`cmovcc` are
single mnemonics and the 4-bit code is a trailing immediate operand
(`setcc cl, 0`, `jcc 0xFFFFFFFC, 4`, `cmovcc eax, ecx, 4`), so the 48 baked
condition mnemonics collapse to three. The reptype/`elt` mnemonic families
(`0F 10` movups, `0F 58` addps) and the x87 groups remain deferred; so does the
`0F 0F`-style 3DNow suffix space.

**The driver knows no x86.** `run_fsm()` indexes `FSM.s[base + byte]`, runs the
actions into a capture array, sets `base = next`, and halts at `FSM_HALT`. The
only architectural constants it uses (`ARCH_ENDIAN`, `ARCH_MAXLEN`, `VAR_ADRSIZ`)
are themselves emitted from corpus.p. The captures are mapped to `x86insn_t`
afterwards by `fill_insn()` -- the single place that knows what a capture means.

**Register numbering.** `mem_base` / `mem_index` hold a size-independent register
*number* (0..7), or `GREG_NONE`. The 16-bit alias (+8 into `greg[]`) is applied at
render time using the address size, so `di` (number 7) never collides with
`GREG_NONE` (15) -- 16-bit `[bx+di]` decodes correctly.

**Prefixes.** Each prefix byte sets its group's var and `MARK`s its offset, so
`x86insn_t` records, per group, the offset of the last (effective) prefix
(`0xFF` = none) plus the raw prefix bytes and their count. The demo marks the
effective prefix in each group with `[..]`.

The demo decodes a spread of single-byte instructions end to end and checks each
against its expected disassembly:

    89 D8           mov eax, ebx
    8B 4C C3 10     mov ecx, [ebx+eax*8+0x10]
    2E 8B 44 9D 08  mov eax, cs:[ebp+ebx*4+0x8]
    B8 78 56 34 12  mov eax, 0x12345678
    66 B9 34 12     mov cx, 0x1234           # 66 -> 16-bit operand
    74 FA           je 0xFFFFFFFC            # rel8 target
    67 8B 07        mov eax, [bx]            # 67 -> 16-bit addressing
    FF E3           jmp ebx                  # group /4
    83 C3 F0        add ebx, 0xFFFFFFF0      # group /0, imm8 sign-extended
    C6 42 04 7B     mov.b [edx+0x4], 0x7B    # group /0 mem, size suffix
    0F B6 C1        movzx eax, cl            # 0F map: greg dest, rgb src
    0F 44 C1        cmovcc eax, ecx, 4       # condition code as an immediate operand
    66 0F 2E C1     ucomisd xmm0, xmm1       # 0F map: opsize-selected mnemonic

`test.cpp` separately drives the addressing decoder over all 512 ModR/M bytes and
the prefix bookkeeping as a regression harness.
