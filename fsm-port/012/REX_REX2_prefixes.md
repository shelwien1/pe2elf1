# x86-64 REX and REX2 prefix behavior

How the **REX** (single-byte legacy) and **REX2** (two-byte APX) prefixes behave with
respect to *repetition*, *combination with each other*, and *coexistence with other
prefixes* — including where each is legal, ignored, or `#UD`.

Everything below is **measured, not quoted from a manual**:

| prefix | how it was exercised |
|---|---|
| **REX** (`0x40`–`0x4F`) | executed as raw bytes on real **Intel Xeon** silicon (a JIT harness loads registers, runs the bytes, and reports which register changed and to what width) |
| **REX2** (`0xD5` + payload) | no shipping CPU has APX yet, so **Intel SDE 10.8.0** (`sde64 -dmr`, the Diamond Rapids model) executes the bytes, and **Intel XED** (Intel's reference decoder, bundled with SDE) is the decode oracle |

Byte sequences use the trailing opcode `01 c0` = `ADD r/m, r` on `rax`/`eax`/`ax` so the
effective **operand size** is visible from the result, and `b8 <imm>` = `MOV eAX, imm` /
`90+r` = `XCHG` where a specific register or length matters.

---

## 1. The two prefixes

```
REX    0 1 0 0 W R X B          one byte, 0x40-0x4F
                └───────── W = 64-bit operand size
                  R X B         high (4th) bit of reg / index / base  -> r8-r15

REX2   0xD5 , M0 R4 X4 B4 W R X B     two bytes (APX)
              │  └──────────────────── R4/X4/B4 = 5th bit of reg/index/base -> r16-r31
              │           W R X B       same meaning as REX
              └── M0 = opcode map: 0 = one-byte map, 1 = 0F map (the 0F is *implied*,
                                    not written; REX2 reaches only these two maps)
```

REX2 is APX's replacement for REX that also encodes the 4th register bit (so it can reach
`r16`–`r31`) and folds the `0F` escape into a map bit. Both are "REX-type" prefixes, and
**they are mutually exclusive** (see §5–6).

---

## 2. The one rule that governs everything

> **A REX / REX2 is effective only as the *last* prefix — the byte(s) immediately before the
> opcode (or `0F` escape, or a VEX/EVEX/XOP introducer). Anything placed between it and the
> opcode changes the outcome completely.**

Every result below is a corollary of this rule plus "REX2 must be immediately followed by
its opcode."

---

## 3. REX — repetition (real hardware)

Multiple REX bytes are **allowed**; only the **last** one is effective and every earlier REX
is **completely discarded** (not merged — discarded).

| bytes | effective | why |
|---|---|---|
| `48 01 c0` | 64-bit `add rax,rax` | one REX.W |
| `48 48 01 c0` | 64-bit | W then W → still W |
| `40 48 01 c0` | 64-bit | REX(0) then REX.W → **last wins** |
| `48 40 01 c0` | **32-bit** `add eax,eax` | REX.W then REX(0) → **W discarded** |
| `4f 40 01 c0` | **32-bit** | REX.WRXB then REX(0) → **all bits discarded** |
| `40 4f 01 c0` | `add r8,r8` | REX(0) then REX.WRXB → last REX's W+R+B all apply |
| `40 40 48 01 c0` | 64-bit | three REX, last is W |
| `48 48 40 01 c0` | 32-bit | last is REX(0) |

The sharpest demonstration that a non-terminal REX is *discarded*, not just overridden:

| bytes | result | length |
|---|---|---|
| `4f 40 b8 44 33 22 11` | `mov eax, 0x11223344` | **7 bytes** |

The leading `REX.W` (`4f`) is dropped, so the immediate is a 4-byte `imm32` (not an 8-byte
`movabs imm64`) and the destination is `eax` (not `rax`). The whole prefix's length and
operand-size effects vanish.

---

## 4. REX — coexistence with other (non-REX) prefixes (real hardware)

| position | effect |
|---|---|
| legacy prefix **before** REX | fine — the REX is still the last prefix, so it is effective |
| any prefix **after** REX | the REX is **completely ignored** (not even its W bit or byte-length effect survives) |

| bytes | effective | note |
|---|---|---|
| `66 48 01 c0` | 64-bit `add rax,rax` | REX.W is last → wins over `66` |
| `48 66 01 c0` | **16-bit** `add ax,ax` | `66` follows REX.W → **REX vanishes**, `66` wins |
| `f2 48 01 c0` | 64-bit | REX.W last → effective |
| `48 f2 01 c0` | **32-bit** | `f2` follows REX.W → REX vanishes |
| `2e 3e 48 01 c0` | 64-bit | two segment prefixes then REX.W → REX last, effective |
| `67 48 01 c0` | 64-bit | REX last → effective (address size 32) |
| `48 67 01 c0` | 32-bit | `67` follows REX → REX vanishes |

So a REX behaves as if it *terminates* the prefix run: put anything after it and it is as
though the REX were never there.

---

## 5. REX2 — repetition ("double `D5`")

There is **no** "repeat REX2" concept. Because REX2 must be *immediately* followed by its
opcode, the byte after the first `D5 <payload>` is decoded as an **opcode**, so a second
`D5` is just an opcode byte:

| bytes | result | why |
|---|---|---|
| `d5 08 d5 08 01 c0` | `#UD` | after `D5 08` the opcode is `0xD5`; in the one-byte map (`M0=0`) `0xD5` = `AAD`, which is illegal in 64-bit mode |
| `d5 88 d5 c1` | `pmullw …` | with `M0=1` the opcode `0xD5` means `0F D5` = `PMULLW` — a *real* instruction, not `#UD` |

So "two REX2 in a row" is `#UD` in the common (`M0=0`) case, but for the architectural reason
above, not a dedicated anti-repetition check.

---

## 6. REX2 — coexistence and combination (Intel SDE + XED)

| combination | legal? | evidence |
|---|---|---|
| legacy prefix (`66`/`F2`/`F3`/seg/`67`) **before** REX2 | **yes** | `66 d5 08 01 c0` → `add rax,rax` (64-bit) |
| any prefix **after** REX2 (between payload and opcode) | **`#UD`** | `d5 08 66 01 c0` → `#UD` |
| **REX immediately before REX2** | **`#UD`** | `40 d5 08 01 c0` → `#UD` (XED: `BAD_REX_PREFIX`) |
| REX before REX2, but neutralized by a legacy prefix in between | **yes** | `40 66 d5 08 01 c0` → legal (the `66` discards the REX per §4; `66 d5` is fine) |
| REX2 then a REX byte (`0x40`–`0x4F`) as the "opcode" | **`#UD`** | `d5 08 48 01 c0` → `#UD` (`0x48` is not a valid opcode there) |
| REX2 with `M0=1` reaching a further escape (`0F 38`/`0F 3A`/`0F 0F`) | **`#UD`** | `d5 88 38 c0` → `#UD` (REX2 reaches only the 1-byte and `0F` maps) |

REX2's `W` bit follows the same "last prefix wins" logic against `66`:

| bytes | effective | note |
|---|---|---|
| `66 d5 08 01 c0` | 64-bit | REX2.W = 1 overrides the `66` |
| `66 d5 00 01 c0` | **16-bit** `add ax,ax` | REX2.W = 0 → the `66` takes effect |

Ordering subtlety, mirroring §4: a REX is illegal only when it is **directly** adjacent to the
`D5`. `66 40 d5 …` is `#UD` (REX directly before `D5`), whereas `40 66 d5 …` is legal (the
`66` sits between the REX and the `D5`, so the REX is discarded first).

**Bottom line:** REX and REX2 **cannot be combined** — a REX adjacent to a REX2 is always
`#UD` in both orders. Only ordinary legacy prefixes may precede a REX2, and nothing may sit
between the REX2 payload and the opcode.

---

## 6a. REX2 — opcode eligibility (not every opcode accepts it)

Unlike a plain REX (which is *silently ignored* on an opcode that has nothing to extend),
**REX2 is `#UD` on opcodes that carry no eGPR-extendable operand** — i.e. anything with no
ModR/M `r/m`/`reg`, no opcode-embedded GPR, and no SIB for `R4`/`X4`/`B4` to reach. Measured
against Intel XED / SDE, the REX2-illegal opcodes are:

| map | REX2-illegal opcodes | why |
|---|---|---|
| **M0=0** (1-byte) | `70-7F` | `Jcc rel8` — no register operand |
| | `A0-A3` | `mov` accumulator ↔ `[moffs]` — implicit `AL/rAX` |
| | `A4-A7`, `AA-AF` | `movs`/`cmps`/`stos`/`lods`/`scas` — implicit `rSI`/`rDI` |
| | `A8-A9` | `test AL/eAX, imm` — implicit accumulator |
| | `E0-E3` | `loopne`/`loope`/`loop`/`jrcxz` — rel8 |
| | `E4-E7`, `EC-EF` | `in`/`out` (imm8 and `DX`) — port I/O |
| | `E8-E9`, `EB` | `call`/`jmp` rel |
| **M0=1** (0F) | `30-37` | `wrmsr`/`rdtsc`/`rdmsr`/`rdpmc`/`sysenter`/`sysexit`/`getsec` — system |
| | `80-8F` | `Jcc rel32` — no register operand |

Everything with a ModR/M or opcode-embedded GPR *is* eligible — including the no-ModR/M but
operand-size-sensitive `98`/`99` (`cwde`↔`cdqe`, `cdq`↔`cqo`) and the embedded-register
`50-5F`/`58-5F` (`push`/`pop`), `B8-BF` (`mov r,imm`), `90-97` (`xchg`), `0F C8-CF` (`bswap`).
Note the asymmetry with plain REX: `48 EB 00` decodes as `jmp` (the REX is a no-op), but
`D5 08 EB 00` is `#UD`.

*Not to be confused with:* opcodes that are `#UD` **with or without** REX2 — legacy encodings
removed in 64-bit (`06/07/0E/16/17/…`, `60-62`, `82`, `9A`, `C4/C5`, `D4-D6/CE`, far `EA`), the
`0F 30-3F`/etc. system holes, or SSE opcodes valid only under a mandatory `66`/`F2`/`F3`
prefix. Those are opcode-map facts, not a REX2 rule.

---

## 7. REX / REX2 with VEX / EVEX / XOP

VEX (`C4`/`C5`), EVEX (`62`) and XOP (`8F`) encode their own `W`/`R`/`X`/`B` (and more), so a
REX or REX2 in front of them is both redundant and illegal. The prefixes that *are* tolerated
in front of a VEX/EVEX introducer are narrower than for a legacy opcode:

| prefix before VEX/EVEX | legal? | evidence (`c5 f8 77` = `vzeroupper`) |
|---|---|---|
| segment (`26/2e/36/3e/64/65`) | **yes** | `2e c5 f8 77`, `65 c5 f8 77` → `vzeroupper` |
| address-size `67` | **yes** | `67 c5 f8 77` → `addr32 vzeroupper` |
| `66`, `F2`, `F3`, `LOCK`(`F0`) | **`#UD` (anywhere in the run)** | `66 c5…`, `2e 66 c5…`, `66 2e c5…` all `#UD` |
| **REX (`40`–`4F`) immediately before** | **`#UD`** | `40 c5 f8 77`, `2e 40 c5 f8 77` → `#UD` |
| REX before VEX but neutralized by a following legacy prefix | **yes** | `40 2e c5 f8 77` → `vzeroupper` (the `2e` discards the REX) |
| **REX2 (`D5`) before VEX/EVEX** | **`#UD`** | `d5 08 c5 f8 77`, `d5 08 62 …` → `#UD` |

Two different rules are at work here, both consistent with §2 and §4:

* `66`/`F2`/`F3`/`LOCK` are the *mandatory-prefix* space that VEX/EVEX encode internally
  (`VEX.pp`), so their presence **anywhere** in the prefix run conflicts → `#UD`.
* A **REX** follows the same "last prefix" logic as everywhere else: it is illegal only when
  it is the byte immediately before the introducer; separate it from the introducer with a
  segment/address-size prefix and the REX is simply discarded, leaving a legal instruction.

---

## 8. Cheat-sheet

```
REX  (0x40-4F)
  repeat        : allowed; LAST one wins, earlier ones discarded
  before it     : any prefix OK; REX still effective if it stays last
  after it      : REX is fully discarded (W, R/X/B, and length effect all gone)
  + REX2        : #UD if adjacent (either order)
  + VEX/EVEX    : #UD if immediately before the introducer

REX2 (0xD5 + payload)              [APX; must be immediately followed by the opcode]
  repeat        : no such thing; 2nd D5 is an opcode -> #UD (M0=0, =AAD) / real op (M0=1)
  before it     : legacy prefixes (66/F2/F3/seg/67) OK; 66 vs REX2.W -> last wins
  after it      : nothing allowed between payload and opcode -> #UD
  eligible on   : only opcodes with an eGPR operand (ModRM/embedded-GPR/SIB) or opsize-
                  sensitive (98/99). #UD on Jcc/call/jmp/loop/string/in-out/acc/system:
                  M0=0 {70-7F, A0-AF, E0-EF} ; M0=1 {30-37, 80-8F}
  + REX         : #UD if adjacent (either order)
  + VEX/EVEX    : #UD

VEX / EVEX / XOP introducer
  allowed before: segment + address-size (67) only
  #UD before    : 66, F2, F3, LOCK (anywhere) ; REX/REX2 (immediately before)
```

---

## 9. Conformance of this repository's decoder

The FSM disassembler in this directory was checked against every case above
(`apxb` vs Intel XED vs SDE execution):

* **REX** repetition and ordering — matches real hardware. `decode_insn` recomputes the
  effective operand size from the *last* prefix byte (and treats a REX followed by any other
  prefix as dead), so `48 40`, `4f 40 b8…`, `48 66`, etc. all decode with the true CPU width
  and length.
* **REX2** combination/adjacency/repetition — matches XED/SDE, including `40 d5…` → `#UD`,
  `66 40 d5…` → `#UD` vs `40 66 d5…` legal, `d5 08 66…`/`d5 08 48…`/double-`D5` → `#UD`, and
  the `66`-vs-`REX2.W` last-wins behavior. (The REX-immediately-before-REX2 `#UD` check was
  added while writing this document's REX2 test — see `STATUS64.md` §7h.)
* **REX2 opcode eligibility (§6a)** — enforced. `decode_insn` rejects REX2 on the
  no-eGPR-operand opcodes (`M0=0` {`70-7F`,`A0-AF`,`E0-EF`}; `M0=1` {`30-37`,`80-8F`}).
  Verified 0 mismatches vs XED over a full 256-opcode × {map0, map1} eligibility sweep — the
  sole remaining entry, `0F A6` (VIA PadLock `montmul`) under REX2, is a don't-care combo (no
  CPU has both APX and PadLock; XED itself accepts REX2+`xstore` at `0F A7` yet rejects
  REX2+`montmul`), so the VIA ops are left decoding, bijection-safe.
* **NP-form mandatory-prefix SSE opcodes** — the general (non-REX2) bug the sweep surfaced is
  fixed across the 0F, 0F38 and 0F3A maps: `0F 6C/6D/7C/7D/B8/D0/E6` **and** the ~53 66-only
  SSE4.1/4.2 0F38/0F3A ops (`pmovsx*`, `ptest`, `pmulld`, `round*`, `blend*`, `pinsr*`, `dpps`,
  `pcmp*str*`, …) no longer decode their `#UD` no-prefix / wrong-prefix forms (bare `0F 6C` is
  `#UD`, not `punpcklqdq`). Fixing this exposed and fixed a pp/opsize conflation: `pp` is now
  derived from *"66 present in the prefix run"* (REX.W-independent), so `66`+`REX.W` SSE ops
  (`66 48 0F38 DC` aesenc, `66 48 0F28` movapd, …) decode correctly instead of `#UD`. 0 NP-0F/38/3A
  over-acceptances vs XED; fuzz64 5 M = 0, roundtrip64 byte-identical, 32-bit 858/858 (see
  `STATUS64.md` §7h).
* **VEX / EVEX / XOP preceding-prefix rule (§7)** — matches XED. `decode_insn` rejects a
  `66`/`F2`/`F3`/`LOCK` anywhere in the run before a `C4`/`C5`/`62`/`8F`(map≥8) introducer, and
  a REX immediately before it, while still allowing segment + `67` (and a REX that a following
  segment/`67` neutralizes). Verified 0 mismatches over an exhaustive single/double-prefix ×
  {VEX C5, VEX C4, EVEX, XOP} differential vs XED.

A separate, non-REX gap that this investigation surfaced (left as-is): a `LOCK` (`F0`) prefix on
a *register*-destination instruction (e.g. `f0 01 c0`) is `#UD` on hardware but the decoder
still accepts it — a LOCK/ModR/M-validity issue unrelated to REX/REX2, and bijection-safe.

---

## Appendix — reproducing the measurements

```
# REX on real silicon: rexexec.c JITs  [load regs][test bytes][save regs]  and reports
# which register changed. add rax,rax reveals operand width by the result.
/tmp/rexexec 0001000180008000 8888888888888888  48 66 01 c0      # -> 16-bit (REX discarded)

# REX2 under Intel SDE (Diamond Rapids) + XED reference decoder:
SDE=/tmp/sde-external-10.8.0-2026-03-15-lin
$SDE/xed64 -64 -d 40 d5 08 01 c0                                 # -> BAD_REX_PREFIX (#UD)
$SDE/sde64 -dmr -- /tmp/rex2exec d5 18 90                        # -> swaps rax<->r16 (a real xchg)

# this repo's decoder (build the batch harness against the 64-bit tables first):
python3 gen.py corpus64.p x86_tables.h && python3 gasm.py corpus64.p x86_tables_enc.h
g++ -std=c++17 -O2 -I. -o /tmp/apxb tools/apxbatch.cpp
echo "66 d5 00 01 c0" | /tmp/apxb                                # -> add ax,ax
python3 gen.py corpus.p x86_tables.h && python3 gasm.py corpus.p x86_tables_enc.h  # restore 32-bit
```
