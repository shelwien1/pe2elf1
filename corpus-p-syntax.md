# corpus.p — Language Reference

`corpus.p` is a declarative **bit-pattern → string rewriter**. A description file maps a byte
stream to rendered text — and, by the same rules read backwards, text to bytes — for lossless,
byte-exact (dis)assembly. The engine that interprets a description knows nothing about any
instruction set: every architectural fact lives in the description. The current language and the
companion file cover the **disassembly direction** for x86-32.

---

## 1. Lexical structure

- **Comments** begin with `#` and run to end of line.
- **Whitespace** separates tokens and is otherwise insignificant — including inside bit patterns,
  where it serves only to group bits visually.
- **Identifiers** name tables and submatches: `greg`, `addr`, `pfx`.
- **Variables** are written `$name`: `$opsiz`, `$g`, `$disp32`.
- **Numbers** are decimal (`15`) or hexadecimal (`0x8b`).
- **String literals** are double-quoted: `"mov "`, `","`, `""`. Inside a table body a bare token
  with no special characters is shorthand for the string of its characters (`eax` ≡ `"eax"`);
  quotes are required when the value contains spaces or separators, or is empty.

A description is a sequence of four kinds of declaration: `arch`, `vars`, `table`, and `submatch`.

---

## 2. `arch` — architecture constants

```
arch  $mode=32 $endian=le $bitorder=msb $maxlen=15
```

Declares read-only constants, accessible anywhere as `$name`:

| field        | meaning                                       | values            |
|--------------|-----------------------------------------------|-------------------|
| `$mode`      | default operand/address width the size prefixes toggle from | `16`, `32`, `64` |
| `$endian`    | byte order of multi-byte numeric fields       | `le`, `be`        |
| `$bitorder`  | bit order within a byte                       | `msb`, `lsb`      |
| `$maxlen`    | maximum instruction length, in bytes          | integer           |

`$bitorder` and `$endian` are independent axes — see §6.1.

---

## 3. `vars` — mutable state

```
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0
```

Declares per-instruction state variables with their initial (reset) values. The engine resets them
at the start of every instruction. They are written by **action blocks** during matching (§5.2) and
read in templates and expressions. In the x86-32 description they hold decoded prefix state.

---

## 4. `table` — positional arrays

```
table greg { eax,ecx,edx,ebx,esp,ebp,esi,edi, ax,cx,dx,bx,sp,bp,si,di }
table sbo  { 0,0,0,0,7,7,0,0 }
table sfx  { "",".b",".w","",".d","","","",".q","",".t" }
```

A table is a fixed array of values (strings or numbers), indexed from `0`. Look it up with
`table[expr]`, where `expr` is any integer expression (§5.5):

```
greg[$opsiz*8 + $g]       # operand-size GP register
sfx[4]                    # ".d"
seg[sbo[$r] + $segidx]    # segment-override prefix string
```

Tables are the workhorse of the language — register files, mnemonic variants, condition codes,
segment displays, and size suffixes are all tables. Folding a computed string *into* table entries
(e.g. the leading `.` of a size suffix) is preferred over introducing a helper.

---

## 5. `submatch` — matchers

A submatch is a named bit-pattern matcher.

```
submatch name(params) {
  pattern => template ;
  pattern => template ;
  ...
}
```

Each rule is `pattern => template ;`. On entry the rules are tried **top to bottom; the first whose
pattern matches fires** (§6.2). Invoking a submatch consumes the bytes its winning pattern matched
and yields a result string bound to `$name` (§5.3).

`main` is the **entry point** — the engine decodes one instruction per `main` call, with the prefix
vars reset and `$S`/`$E` bound to the instruction's byte span:

```
submatch main { @pfx(0) => $pfx }
```

### 5.1 Parameters

A submatch may take parameters, written `$x`:

```
submatch pfx($d)            { ... }   # ordinary value parameter
submatch immz1($opsiz)      { ... }   # prefix-dispatch parameter (§5.6)
submatch addr1($adrsiz,$g)  { ... }   # prefix-dispatch + bidirectional field (§6.3)
```

Arguments are passed at the call: `@pfx($d+1)`, `@addr1($adrsiz, $g)`.

### 5.2 Pattern elements

A pattern is a sequence of the following, read left to right.

**Bit literals** — `0` and `1` each match one stream bit.

**Hex bytes** — `0x8b` matches eight stream bits with that value (equivalent to spelling the bits).

**Named bit-fields** — a run of one letter matches that many bits and captures them into `$letter`.
The field's value is the integer formed by its bits (§6.1).

```
11 ggg rrr            # one byte: 2 literal bits, 3-bit $g, 3-bit $r (ModR/M)
10111 bbb             # one byte: 5 literal bits, 3-bit $b (B8+r register encoding)
dddddddd              # one byte captured into $d
iiiiiiii iiiiiiii     # two bytes captured into $i (endian-decoded, §6.1)
ss iii bbb            # SIB byte: 2-bit $s (scale), 3-bit $i (index), 3-bit $b (base)
```

The same letter repeated across byte boundaries forms one wider field; whitespace only groups.

**`<...>` — prefix-dispatch match** (§5.6) — matches the bits of the submatch's prefix-dispatch
*argument*, MSB-first; `?` is a don't-care bit. Consumes no stream.

```
<0>       # the passed value's leading bit is 0
<1>
<? 1>     # leading bit any, next bit 1
```

**`@sub` / `@sub(args)` — submatch invocation** — runs another submatch at the current position,
consuming what it matches; its result is bound to `$sub`.

```
@disp32                   # -> $disp32
@addr                     # -> $addr ; reg field free  (captured)
@addr(0)                  # -> $addr ; reg field bound to 0
@pfx($d+1)                # -> $pfx
```

**`[expr]` — guard** — a boolean expression that must hold for the rule to match; consumes no stream.

```
[$d >= $maxlen] => "" ;
```

**`{ $v = expr ; ... }` — action** — assigns state variables when the rule matches; consumes no
stream. Used for decoded state and for handing values back out of a submatch.

```
0x66 @pfx($d+1) {$opsiz=1} => "opsiz " $pfx ;
ss 100 bbb {$sbase=$b}     => "[" greg[$b] "]" ;
```

### 5.3 Results

Invoking `@foo` (or `@foo(args)`) binds the winning rule's template output to `$foo`. The result
name is the submatch's **base** name regardless of arguments: `@addr1(...)` yields `$addr1`.

### 5.4 Templates

The right-hand side of `=>` is a template: terms concatenated by juxtaposition.

- **String literals**: `"mov "`, `","`.
- **Variable interpolation**: `$g`, `$disp32`, `$addr`.
- **Table lookups**: `greg[$opsiz*8+$g]`, `sfx[1]`.
- **Builtins** (§7): `hex($immz)`, `dec($b)`, `sgn($disp8)`.
- **Conditionals**: `c ? a : b`.
- Arbitrary integer **expressions** inside indices and builtins.

```
"mov " greg[$opsiz*8+$g] "," $addr
m10[$reptype] sfx[m10sz[$reptype]] " " ssereg[8+$g] "," $addr
"j" cond[$c] " " hex($rel8)
```

### 5.5 Expressions

Integer expressions appear in table indices, builtin arguments, guards, and actions. Operators:
`+ - * /`, shifts `<< >>`, comparisons `== != < <= > >=`, and the ternary `?:`. Operands are
variables, numbers, table lookups, and `$S`/`$E`. The sentinel `none` denotes "no value" (e.g. a
missing base register; `sbo[none]` is `0`).

### 5.6 Prefix-dispatch parameters and `<...>`

`<...>` matches a value the **caller passes in**, not any global. The matched value is the
submatch's prefix-dispatch parameter — the parameter that has **no corresponding stream field**.
In `addr1($adrsiz, $g)`, `$g` is pinned by the `ggg` field, so `<...>` matches `$adrsiz`; in
`immz1($opsiz)`, `<...>` matches `$opsiz`.

This keeps each matcher a pure function of its arguments. Global state enters only through thin
wrappers that read the current variables and forward them:

```
submatch immz  { @immz1($opsiz) => $immz1 }              # inject current opsiz
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }      # inject current adrsiz, forward $g
```

Because the dispatched values here are single bits, the matches are just `<0>`/`<1>`.

---

## 6. Semantics

### 6.1 Bit order vs. byte order

Two independent axes govern how a field's integer value is formed:

- **`$bitorder`** orders bits *within* a byte. With `msb`, the byte `11 ggg rrr` yields `mod` from
  the top two bits, `reg` from the next three, `rm` from the low three.
- **`$endian`** orders bytes *across* a multi-byte field. With `le`, `dddddddd dddddddd` is
  assembled low-byte-first, so the two bytes `34 12` give `0x1234`.

A single-byte (or sub-byte) field is unaffected by `$endian`; a multi-byte field uses both axes.

### 6.2 Match order

Within a submatch, rules are attempted in source order and the first match wins, so patterns are
written specific-before-general where they could overlap. In the x86-32 description each ModR/M
opcode is split into two rules — a `mod=11` register rule (`0xff 11 000 rrr`) placed before the
memory rule (`0xff @addr(0)`) — and the memory submatch matches only `mod≠11`, so the two are
disjoint by construction.

### 6.3 Bidirectional field parameters

A submatch parameter that unifies with a stream field is **bidirectional**:

- **free** (no argument) → the field is **captured** into the parameter and flows back to the caller;
- **bound** (an argument passed) → the field is **constrained** to equal the argument.

`addr`'s reg field `$g` is the example. `@addr` leaves `$g` free, so it captures the ModR/M reg
field as an operand; `@addr(0)` binds it, so the field must be `000` — an opcode-extension group,
`/0`. The same match-or-capture duality is what lets the rules run backwards for assembly: a field
is an output when disassembling an unknown encoding and an input when assembling a known one.

### 6.4 Prefix-token recovery

Prefixes are emitted as leading tokens by `pfx`, one frame per byte. The engine then **suppresses**
a prefix's token automatically when the rendered instruction emits a token whose *identity that
prefix determined* — i.e. the prefix is already recoverable from the text, so a separate token would
double-count it. The determining emits are:

| prefix                       | recovered when the render emits                          |
|------------------------------|----------------------------------------------------------|
| `opsiz` (66)                 | a `greg` upper-half (16-bit) register, or a 16-bit `rm16` form |
| `adrsiz` (67)                | a 16-bit `rm16` addressing form                          |
| `segidx` (26/2e/36/3e/64/65) | a non-empty `seg[]` entry — a `xx:` override             |
| `reptype` (f3/f2)            | a mnemonic chosen from a prefix-indexed table            |

A plain number is **identity-stable** — it reads the same whatever the prefix — so it recovers
nothing. Hence `66 68` disassembles to `push 0x1234` and keeps its `opsiz` token, and a `67` with an
absolute `[disp]` keeps its `adrsiz` token. With several overrides the **last** determining emit
wins, so `36 2e 2e …` renders `segss segcs mov eax,cs:[eax]`: the effective `cs:` recovers the final
`2e`, leaving the earlier override bytes as tokens.

This recovery is the disassembly-side stand-in for the assembler re-deriving prefixes from text. The
genuinely redundant encodings — an over-wide displacement, an aliased opcode such as `0x82`, a
redundant override — are left to the assembly direction.

---

## 7. Builtins

| builtin                       | result                                                        |
|-------------------------------|---------------------------------------------------------------|
| `hex(n)`                      | `n` as `0x…` hexadecimal                                      |
| `dec(n)`                      | `n` as decimal                                                |
| `sgn(n)`                      | signed displacement, `+0x…` / `-0x…`, for joining inside `[ ]`|
| `sx8(n)` `sx16(n)` `sx32(n)`  | sign-extend an 8/16/32-bit value to the working width        |

`sgn` is the only string-shaping builtin; the rest are numeric render/convert.

---

## 8. Reserved variables

| variable                          | meaning                                                          |
|-----------------------------------|------------------------------------------------------------------|
| `$S`                              | address of the first byte of the current instruction             |
| `$E`                              | address one past the last byte (PC-relative targets: `$E + sx8($d)`) |
| `$letter`                         | a captured bit-field                                             |
| `$name`                           | the result of submatch `name`                                    |
| `$maxlen` and other `arch` fields | architecture constants                                           |
| `none`                            | the "no value" sentinel                                          |

---

## 9. Conventions in the x86-32 description

- Size-paired register files are **one table** with the 16-bit half in the upper indices
  (`greg[$opsiz*8+i]`); emitting from the upper half is exactly the `opsiz`-recovering signal (§6.4).
- Each ModR/M operand is **two rules**: a `mod=11` register rule with an inline table and no size
  suffix, and an `@addr` memory rule that always carries the suffix — this removes any
  "is it memory?" flag.
- Mandatory-prefix and operand-size mnemonic choices are **prefix-indexed tables**
  (`m10[$reptype]`, `movs[$opsiz]`); indexing one both picks the mnemonic and recovers the prefix.
- `addr` is **memory-only**; the register form is handled by its paired `mod=11` rule.
- Segment display is the `seg` / `sbo` / `sbo16` tables, indexed inline; the "ds on a ds-default
  base is omitted, but ds overriding an ss-default base is shown" rule is encoded *in the table*
  rather than as a conditional.

---

## 10. Reading a decode (worked trace)

Bytes `8B 4C 18 10` against the description:

1. `main` → `@pfx(0)`. The first byte `8B` is not a prefix, so `pfx` falls to `@insn`.
2. In `insn`, `0x8b` matches, then `@addr` runs on `4C…`. `$g` is free.
3. `addr` → `@addr1($adrsiz, $g)`. `$adrsiz` is `0`, so the `<0>` rules apply. `4C` = `01 001 100`:
   `mod=01`, `reg=001` (captured: `$g=1`), `rm=100` → the SIB rule `<0> 01 ggg 100 @sib1 @disp8`.
4. `@sib1` on `18` = `00 011 000`: `scale=0`, `index=011` (`ecx`), `base=000` (`eax`), `$sbase=0`.
   `sib1` returns the core `eax+ecx*1` (`dec(1<<0)` = `1`).
5. `@disp8` on `10` → `$disp8 = 0x10`; `sgn` renders `+0x10`.
6. `addr1` emits `seg[sbo[0]+0] "[" "eax+ecx*1" "+0x10" "]"` = `[eax+ecx*1+0x10]`
   (`sbo[0]=0`, `seg[0]=""`).
7. Back in `insn`: `"mov " greg[$opsiz*8+1] "," $addr` → **`mov ecx,[eax+ecx*1+0x10]`**.

`$E` now points one past `10`; the engine resets the vars and calls `main` again for the next
instruction.
