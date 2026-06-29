# Encoding a full instruction from `x86insn_t`

The decoder is `bytes → cap[] → x86insn_t`: the prefix run sets the vars, `op1`/
`op2` classify the opcode (mnemonic, `FORM`, `IMK`, operand files, embedded reg,
condition code), the dedicated group stage resolves a `/digit`, and the
`modrm`/`sib` stages fill the addressing captures. This is the inverse:
`x86insn_t → bytes`. It reuses the addressing inverse from `ENCODE_ADDR.md` and
adds the one instruction-specific piece — **mnemonic → opcode bytes, with the
corpus-named bitfields filled from the operand values**.

The split is deliberate and matches the note in `FULL_DECODE.md` §1: the
addressing decode is operand-independent, so one hand-written routine covers
every instruction. Generating a ModR/M assembler per opcode would be busywork.
What is *not* operand-independent — which opcode a mnemonic maps to, where the
register/`cc`/`/digit` fields sit in that opcode — is small, per-rule, and the
only thing the generator needs to emit for the encode direction.

This is the bijection the disassembler-as-compression-preprocessor needs in the
other direction. The bijection endpoints are **the byte stream and `x86insn_t`**;
`cap[]` is a per-direction working buffer, and the two directions need not use
identical cap conventions (decode's `CAP_CC = cc+1` render trick, §8, is one that
they do not). `decode ∘ encode = id` on the byte stream lets a recompressor
round-trip code through the structured form. As in `ENCODE_ADDR.md`, the semantic
operand carries the modellable signal and a tiny *witness* covers the points
where decode is many-to-one — and that witness rides as **fields of `x86insn_t`**
(§11), not a side-channel, so the struct alone is the bijection: a pass that reads
and rewrites `x86insn_t` preserves the bytes by construction.

---

## 1. Scope: hardcoded vs generated

**Hardcoded in C** (hand-written, operand-independent, one routine for all):

* **Prefixes.** Derived from the semantic fields — `opsize`→`66`, `addr`→`67`,
  `lock`→`F0`, `reptype`→`F2/F3`, and a segment override *computed* from `mem_seg`
  vs the base register's default segment. Placed from the captured run
  (`pfx_off`/`pfx[]`) where it still applies, else a fixed canonical order (§9).
  Inverse of `parse_prefixes`.
* **ModR/M + SIB + disp.** Inverse of `parse_addr`, fully specified in
  `ENCODE_ADDR.md`: from `CAP_MODE/RM/BASE/INDEX/SCALE/DISP` (+ a `/digit` or the
  reg operand for the ModR/M reg field) → the addressing byte run, plus the
  witness δ where decode collapses distinct bytes.
* **Immediate.** Width from `IMK` (+ `opsiz` for `IMMZ`/`RELZ`), little-endian;
  `REL*` is `target − end`; `PTR` is `off:sel`. Inverse of `append_imm`.

**Generated from `corpus.p`** (the only instruction-specific encode data):

* the map `(mnemonic, operand signature) → opcode bytes + embedded-field fills`,
* the per-candidate metadata (`FORM`, `IMK`, operand order, `/digit`) the three
  hardcoded emitters above need to know what to do.

In: an `x86insn_t` (mnemonic index + operand list + addressing fields + encoding
witness). Out: the
byte run. Step 0 of the pipeline (§7) lowers the operand list into `cap[]` in the
decoder's units; that is the only point the two directions share a representation.

---

## 2. One source, two directions — and the asymmetry

The byte pattern of a rule's LHS is bidirectional, exactly as `ENCODE_ADDR.md`
§10 argues for addressing. A `BytePattern` is a literal mask/value plus fields
`{pos,width}`; decode does `cap = (byte >> lo) & mask`, encode does
`byte = lit | (cap & mask) << lo`. So the opcode byte and its embedded
register/`cc` field invert with no new information — the field the decoder
*reads* is the field the encoder *writes*. `gen.py` emits both from one rule set;
they cannot drift.

The asymmetry is **selection**, and it is the whole difficulty of the encode
direction:

* Decode: an opcode byte indexes **one** state. The map is a function.
* Encode: a mnemonic names **many** encodings, disambiguated by operands. `add`
  alone is `00/01/03/04/05` and the group forms `81 /0` and `83 /0`.

So the encode key is `(mnemonic, operand signature)`, not the mnemonic alone.
The generator buckets the rules by mnemonic and, within a bucket, by the operand
signature it reads off the rule's RHS template — the same templates the decoder
already mines for `FORM`, files, and direction.

---

## 3. The operand signature

The signature is the minimal classification of `x86insn_t`'s operands that
distinguishes the candidates in a mnemonic's bucket. Per operand:

* **class** — `ACC` (implicit `al`/`ax`/`eax`), `REG` (a register that lands in
  the ModR/M reg field), `RM` (register-or-memory, the ModR/M r/m field), `MEM`
  (memory only), `IMM` (immediate, with a width-fit subnote), `REL`, `CC`, `PTR`;
* **file/size** — `OPF_*` (`RGB`/`GREG`/`XMM`/`MM`/`SREG`) and 8/16/32, which is
  what splits a byte opcode from its word/dword sibling (§9);
* **order** — which operand is the reg and which is the r/m (the decoder's
  `CAP_DIR`), since that picks `01` (`r/m, r`) vs `03` (`r, r/m`).

The signature is derived, not stored: for `add eax, ecx` it is `[REG32,
RM32(reg), dir=rm←reg]`; for `add eax, [m]` it is `[REG32, MEM, dir=reg←rm]`; for
`add eax, 0x10` it is `[RM32, IMM(fits8)]` (with the accumulator subnote). The
generator computes each rule's signature the same way the decoder built `FORM`/
`RFILE`/`MFILE`/`DIR` — from `greg[$g]`/`rgb`/`ssereg`/`$addr`/`hex($imm)`/`cc`.

---

## 4. Selection policy

`select()` has two stages: a **static** match on the signature (class/file/size/
order — a table lookup), then a **dynamic** refinement over the surviving
candidates that depends on actual *values*, not just types. Only the second stage
needs runtime logic; the first reuses the same per-rule classification the decode
dispatch produced (`FORM`/`RFILE`/`MFILE`/`DIR`), read as a key instead of an
output. Because a mnemonic maps to many opcodes (§2), this is a lookup with
fan-out, not a clean function inverse.

The dynamic tie-breaks:

* **immediate fit** — `add r/m, imm` with an `imm` that sign-extends from 8 bits
  prefers `83 /0` (imm8) over `81 /0` (immz). Depends on the imm value, so it
  cannot be a pure type match.
* **accumulator short forms** — `add eAX, imm` *can* use `05`, but `83 C0 ib`
  (3 bytes) beats `05 id` (5 bytes) for an imm8, so length-first picks `83`;
  `05` wins only when the imm needs the full width.
* **direction** — `reg, reg` can encode as either `01` or `03`; pick one
  canonical form (say the store-form `01`). When one operand is memory the
  direction is forced and the static stage already resolves it.

Two structural notes that make the buckets less tidy than "one mnemonic, several
imm widths":

* **Operands match several signatures.** `eax` is both the accumulator (`05`) and
  a general register (`83`/`81`), so an `eax, imm` insn lands in two candidates at
  once; selection is over the union, not a single signature lookup. Accumulator
  forms are *specializations* — always optional, chosen only when cheaper.
* **A mnemonic can span opcode *and* form.** `inc` is `40+r` (embedded reg, 1
  byte) **and** `FF /0` (group, r/m). `inc eax` → `40` (1 byte) beats `FF C0` (2);
  `inc [m]` → only `FF /0` can address memory. The static stage routes by operand
  class (reg→`40`, mem→`FF /0`); both candidates sit in the `inc` bucket because
  the `/digit` member of a group is itself a distinct mnemonic.

For plain *assembly* this is enough: the chosen form is a valid encoding of the
mnemonic. For **byte-exact recompression** the original may have used a
non-canonical choice; that residual is an *opcode witness*, the §11 analogue of
δ.

---

## 5. The reverse-action structure

The emit step mirrors `struct Action` (the 16-bit decode union), data flow
reversed. Decode's `FIELD` writes `cap` from a byte; encode's `EMIT_FIELD` writes
a byte from `cap`, over the *same* `(field, hi, lo)`:

    union EmitAction {
      struct { uint16_t op:3, src:5, hi:5, lo:3; };   // EMIT_FIELD: a cap + bitrange
      struct { uint16_t opx:3, argx:13; };            // EMIT_LIT / EMIT_DIGIT: a literal
      uint16_t x;
    };

    EMIT_LIT(byte)        out[len++] = byte;                       // opcode byte (the LHS literal)
    EMIT_FIELD(src,hi,lo) out[last] |= (cap[src] & M) << lo;       // M = (1<<(hi-lo+1))-1
    EMIT_DIGIT(n)         pend_digit = n;                          // ModR/M reg = /digit (groups)

`EMIT_FIELD` is `ACT_FIELD` (`cap[dst] = (b>>lo) & M`) run backwards over the
*same* `(field, hi, lo)` and the same width mask `M`, so the generator emits it by
copying the decode action and flipping the op. It reads the **raw** operand value
the encoder placed in `cap[src]` (§7 step 0), e.g. a register number 0–7 or a
condition code 0–15. `EMIT_LIT` is the literal bits the decoder matched as a table
index. `EMIT_DIGIT` carries the `/digit` the decoder baked per-byte into the
dedicated group stage; here it rides into the ModR/M reg field the hardcoded
addressing emitter writes. The 0F escape is just `EMIT_LIT(0x0f)` then
`EMIT_LIT(op2byte)` — no new op.

A candidate's emit program is 1–3 actions: optional `0F`, the opcode byte, and at
most one `EMIT_FIELD`/`EMIT_DIGIT`. Everything wider (ModR/M, disp, imm) is the
hardcoded tail, driven by the metadata below.

---

## 6. Candidate record and generated tables

    struct EncCand {
      uint16_t   sig;        // operand signature key (§3)
      uint8_t    form;       // FORM_*  -- does the addressing emitter run, and how
      uint8_t    imk;        // IMK_*   -- immediate width for the imm emitter
      uint8_t    dir;        // operand order: which operand is ModR/M.reg
      uint8_t    n_emit;
      EmitAction emit[3];    // 0F? + opcode + (field | digit)
    };

    static const EncCand   enc_tab[];                 // every candidate, grouped by mnemonic
    static const uint16_t  enc_index[MNEM_COUNT];     // mnem -> first candidate
    static const uint8_t   enc_count[MNEM_COUNT];     // contiguous run length

`enc_index`/`enc_count` slice `enc_tab` per `mnem_tab` index; `select()` scans the
slice for a `sig` match and applies §4 policy. The metadata (`form`, `imk`, `dir`)
is the *same* per-opcode data the decoder set into `CAP_FORM`/`CAP_IMK`/`CAP_DIR`
— reused, not recomputed.

The recent `cc`-as-immediate change shows the symmetry mostly paying off, and one
seam where it does not. Decode folded the 16 conditions of `jcc`/`setcc`/`cmovcc`
into three mnemonics with the code as an operand; the encode table inherits
exactly three buckets, each one candidate, the condition supplied by a single
`EMIT_FIELD(CAP_CC, 3, 0)` into the opcode's low nibble. Had the 48 baked
condition mnemonics survived, `enc_tab` would carry 48 trivially-different
candidates; the encode table now tracks `mnem_tab` one-for-one.

The seam: decode does **not** field the code out, it bakes `CONST CAP_CC = cc+1`
per byte (the `+1` so `0` can mean *no cc operand* in the render). So the encode
`EMIT_FIELD` cannot read `cap[CAP_CC]` as-is — it reads the **raw** code (0–15)
the encoder places from the operand (§7 step 0), not decode's biased value. The
clean fix is to make decode field `CAP_CC` from the opcode's low nibble and carry
presence in a separate bit (or model `cc` as an ordinary immediate operand): then
`FIELD`/`EMIT_FIELD` are byte-identical inverses and `cap` needs no per-direction
special case for the condition. This is the one place the current decoder trades
a clean field for a render shortcut.

---

## 7. The encode pipeline

Given `x86insn_t in` with its operand list:

    0. fill_cap(in)                          // inverse of fill_insn: operands -> CAP_REG/RM/BASE/
                                             //   INDEX/SCALE/DISP/IMM/REL and the RAW cc into CAP_CC
    1. sig  = signature(in)                 // §3: classify the operands
    2. cand = select(in->mnem, sig)         // §4: enc_index[mnem].. + policy; fail => not encodable
    3. emit_prefixes(in)                     // §9: derive set from fields (mem_seg -> override);
                                             //   place via pfx_off/pfx[], else canonical order
    4. for a in cand.emit:                   // the generated program
         EMIT_LIT   -> append opcode byte
         EMIT_FIELD -> OR an operand-derived field into the last byte (§8)
         EMIT_DIGIT -> stash the /digit for step 5
    5. if cand.form in {MODRM, RM, GROUP}:
         emit_modrm_sib_disp(in, regfield)   // ENCODE_ADDR.md; regfield = reg operand | digit | 0
    6. emit_imm(in, cand.imk, opsiz)         // hardcoded width; REL = target-end; PTR = off:sel
    7. return len

Step 0 is where the bijection's representation contract lives: `fill_cap` must
lay operands into `cap` in the *same* units the decoder uses (register **number**
0–7, `mem_disp` as int32, raw `cc`), so `EMIT_FIELD` and `emit_modrm_sib_disp` are
true inverses of `FIELD`/`fill_insn`. The driver owns steps 0/3/5/6 and the
dispatch; the generated part is only step 4 plus the `cand` metadata steering
steps 5–6. `FORM_NONE`/`REG`/`REG_IMM`/`IMM`/`REL`/`PTR` skip step 5;
`MODRM`/`RM`/`GROUP` run it.

---

## 8. Where the operands land — filling the embedded fields

The operand values live in `cap[]` (the encoder populates them from `x86insn_t` in
step 0, the inverse of `fill_insn`). The fields the decoder pulled *out* of the
opcode are pushed back *in*:

* **`B8+r` / `40+r` / `50+r`** — `EMIT_FIELD(CAP_REG, 2, 0)`: `opcode |= reg`.
  Inverse of the decode `FIELD CAP_REG` on the opcode byte; `CAP_REG` is the same
  register number 0–7 in both directions, so this pair is exactly symmetric.
* **`jcc` / `setcc` / `cmovcc`** — `EMIT_FIELD(CAP_CC, 3, 0)`: `opcode |= cc`,
  reading the **raw** code 0–15. Note the asymmetry from §6: decode stores
  `cc+1`, so step 0 must place the raw code here rather than copy decode's value.
* **group `/digit`** — `EMIT_DIGIT(n)`: the ModR/M reg field is the extension `n`,
  not a reg operand. Inverse of `@addr(N)` fixing `ggg = N` in the dedicated
  group stage.
* **plain ModR/M reg operand** — not an `EmitAction`; the addressing emitter
  writes `CAP_REG` into the reg field directly (step 5).

So `EMIT_FIELD` handles the opcode-resident fields and `EMIT_DIGIT` the
ModR/M-resident one; the reg/r/m *operands* are the addressing emitter's job.

---

## 9. Prefixes — derived from the fields, placed from the run

`emit_prefixes` is hardcoded and operand-independent, but it is not a blind
canonical dump: it derives *which* prefixes from the semantic fields and decides
*where* from the captured prefix run.

**Which prefixes (the set).** Each comes from an `x86insn_t` field, the inverse of
what `parse_prefixes`/`fill_insn` recorded:

* **`66`** from `opsize`. This is also the operand-size split that picks the
  opcode: 8-bit is a *different opcode* (`rgb` → `88/00/C6`, `greg` → `89/01/C7`),
  while 16-bit vs 32-bit is the *same opcode plus `66`*. `SSE_OS` (`paddb`,
  `ssereg[$opsiz*8+$g]`) is likewise one opcode whose file flips `mm`↔`xmm` with
  `66`. So `select()` (§4) resolves the 8-bit opcode statically and leaves the
  16/32 choice to this prefix.
* **`67`** from `addr`; **`F0`** from `lock`; **`F2`/`F3`** from `reptype`.
* **segment override** from `mem_seg` — the one that is *computed*, not a flag.
  The default segment is a function of the base register, `seg[sbo[base]]`
  (16-bit: `seg[sbo16[rm]]`); an override (`26/2E/36/3E/64/65`) is emitted only
  when `mem_seg` differs from that default. Exact inverse of the decoder's
  `mem_seg = seg[sbo[base] + segidx]`, so an edited `mem_seg` re-derives the right
  override — or drops it — with no stored prefix involved.

**Where (placement).** The decoder hands over a prefix witness: `n_pfx`, `pfx[]`
(raw bytes, in order), and `pfx_off[VAR_COUNT]` (the offset of each group's
effective prefix, `0xFF` = that group had none). Placement is:

* **replay** when the witness is present (`has_pfx`) and still covers the derived
  set: emit each derived prefix at its `pfx_off[group]` offset — sorting by offset
  reproduces the original order — and any redundant or ineffective bytes straight
  from `pfx[]`. Byte-exact.
* **canonical order** for any derived prefix the witness does not place:
  `has_pfx = 0` (a synthesized `x86insn_t`, never decoded), or `pfx_off[group] =
  0xFF` for a group the *modified* fields now require — a transform set `lock`, or
  changed `mem_seg` so an override is newly needed, or flipped `opsize`. Those go
  in a fixed order (segment, `66`, `67`, `F0`, `F2/F3`); the witness, where it
  has a slot, overrides it.

So the field set is authoritative for *which* prefixes exist — a transform that
edits `x86insn_t` always gets a correct, sufficient prefix set — while `pfx_off`/
`pfx[]` is a placement witness that reproduces order and redundancy for unmodified
code and is silently bypassed where the edit made it stale.

---

## 10. Worked examples (byte-exact)

    add eax, ecx        sig [REG32, RM32(reg), dir=rm<-reg]
                        select add/01  -> EMIT_LIT(0x01); FORM_MODRM, dir=rm,reg
                        modrm: reg=ecx(001) rm=eax(000) mod=11 = 0xC8
                        => 01 C8                         (canonical; 03 C1 is the equal-length twin)

    add eax, 0x10       sig [RM32, IMM(fits8)]
                        select add/83 /0 (imm8 < imm32)  -> EMIT_LIT(0x83); EMIT_DIGIT(0); FORM_GROUP, IMK_IMM8SX
                        modrm: reg=/0(000) rm=eax(000) mod=11 = 0xC0 ; imm8 = 10
                        => 83 C0 10                      (05 0A 00 00 00 is the longer accumulator form)

    inc eax             sig [REG32]
                        select inc/40+r (1 byte) over inc/FF /0 (2 bytes)
                        EMIT_LIT(0x40); EMIT_FIELD(CAP_REG,2,0) -> 0x40|0 ; FORM_REG
                        => 40                            (FF C0 is the longer r/m form; required only for inc [m])

    jcc 0xFFFFFFFC, 4   mnem jcc, cc=4, rel
                        EMIT_LIT(0x70); EMIT_FIELD(CAP_CC,3,0) -> 0x70|4 = 0x74 ; FORM_REL, IMK_REL8
                        rel8 = target - end = -4 - 2 = -6 = 0xFA
                        => 74 FA

    setcc cl, 0         mnem setcc, cc=0
                        EMIT_LIT(0x0f); EMIT_LIT(0x90); EMIT_FIELD(CAP_CC,3,0) -> 0x90|0 ; FORM_RM
                        modrm: reg=0 rm=cl(001) mod=11 = 0xC1
                        => 0F 90 C1

    movzx eax, cl       sig [REG32<-greg, RM8<-rgb]
                        EMIT_LIT(0x0f); EMIT_LIT(0xb6) ; FORM_MODRM, dir=reg,rm
                        modrm: reg=eax(000) rm=cl(001) mod=11 = 0xC1
                        => 0F B6 C1

Each output is what the decoder of the previous steps consumes; the demo's
expected strings are the fixed points of `decode ∘ encode`.

**Verification gate.** Mirror the decoder's empirical harnesses (the 512-ModR/M
sweep, the self-checking demo). For every `enc_tab` candidate, synthesize a
canonical `x86insn_t` over a spread of operands, `encode` it, `decode` the bytes,
and assert the structured form round-trips — `decode(encode(in)) ≡ in` on the
canonical domain, and byte-equality where the input was itself canonical. Cross
this with the addressing sweep (encode every ModR/M form through
`emit_modrm_sib_disp` and re-decode) so the two hand-written inverses are checked
independently of the generated opcode layer. This catches the asymmetries §6/§8
warn about — the `cc+1` seam fails this gate immediately if step 0 forgets to
normalize.

---

## 11. Byte-exact round-trip: the witness lives in `x86insn_t`

For plain assembly, §4's canonical pick is fine. For lossless `code → x86insn_t →
code` — the recompression case — `x86insn_t` must *be* the bijection: every
redundant choice the canonical encoder would override has to be a **field of
`x86insn_t`**, not a side-channel, or a pass that reads and rewrites the struct
cannot preserve the bytes. Beyond the semantic operand, that residual is:

* **addressing δ** — `disp_w` (actual disp width vs the canonical shortest), `sib`
  (a redundant SIB byte was present), `sscale` (dead scale bits when there is no
  index). The three fields `ENCODE_ADDR.md` §3 specified as `AddrWitness`; they now
  live *in* `x86insn_t`, populated by `parse_addr` — which already sees `mod`, the
  SIB byte, and the raw `ss` bits and currently throws them away.
* **opcode-choice `enc`** — which equal-cost encoding a mnemonic+operands used: the
  direction twin (`01` vs `03`, which decode otherwise hides in `op[]`/`CAP_DIR`),
  the immediate-width twin (`83` sx-`imm8` vs `81` `imm32` when it fits), the
  accumulator short form (`05` vs `81 /0`), `moffs` (`A1/A3`) vs a `disp32` ModR/M.
  A small deviation code, `0` = canonical, so a synthesized or operand-edited insn
  re-encodes canonically with no stale witness to honor.
* **prefix run** — already carried by `n_pfx` / `pfx[]` / `pfx_off[]`: order plus
  any redundant or ineffective prefix bytes (§9). The semantic fields say *which*
  prefixes; this part of the witness says *where* and *which extras*.

All of these are all-zero / canonical for compiler-emitted code, so the witness
costs ~nothing there and spends bits only on hand-written or obfuscated code, in
proportion to its non-canonical choices — the property `ENCODE_ADDR.md` argues for
δ. The struct being the single source of truth does not stop the compression
layer from **demuxing these fields into their own stream** at serialization time
(so the model sees witness apart from operands); that is a stream-layout choice
over one struct, not a second source of truth.

Encoder use: `select()` (§4) computes the canonical candidate; a nonzero `enc`
overrides it to the recorded twin, and δ (`disp_w`/`sib`/`sscale`) steers
`emit_modrm_sib_disp`. With the whole witness all-zero the encoder emits canonical
bytes — "plain assembler" is exactly *ignore the witness fields*.

> Implementation note: the decoder does **not** yet populate `disp_w`/`sib`/
> `sscale`/`enc` (they default to 0 = canonical), so today's round-trip is exact
> only for instructions that happened to be encoded canonically. Wiring
> `parse_addr` and `select()`/`op1` to record them is the change that makes the
> bijection hold for *all* input.

---

## 12. `gen.py` work items

Run the same rule set the decoder consumes, backwards:

* **emit programs.** For each non-deferred rule, build the `EmitAction[]`:
  `EMIT_LIT(0x0f)` if two-byte, `EMIT_LIT(opcode literal bits)`, then
  `EMIT_FIELD` for each embedded field (`b`/`cc`) — copied from the decode
  `FIELD` action with the op flipped — and `EMIT_DIGIT(N)` for a group `/digit`.
* **signature + metadata.** Reuse the per-rule `FORM`/`IMK`/`DIR`/files/`cc`
  already computed for the decode descriptor; pack the operand signature.
* **buckets.** Group candidates by `mnem_tab` index into `enc_index`/`enc_count`;
  within a bucket order by §4 policy (shortest / most-specific first) so
  `select()` takes the first candidate whose signature matches *and* whose
  value predicate holds (e.g. the imm actually fits imm8 for `83`).
* **groups.** One candidate per defined `/digit` (the digit → `EMIT_DIGIT`),
  inheriting the dedicated-stage data inverted.
* **deferred, as in decode** — the reptype/`elt` SSE families, x87 groups, 3DNow.

The addressing inverse (`emit_modrm_sib_disp`) and the prefix/immediate emitters
are hand-written and shared; `gen.py` emits only `enc_tab`/`enc_index`/
`enc_count`, keeping the encode path under the same single-source invariant as
the decode tables.

---

## 13. Open questions / policy knobs

* **Canonical opcode** (`01` vs `03`, `83` vs `81` vs `05`): length-first is a
  default, but a recompressor may prefer "match the witness"; make it a knob.
* **Signature granularity** — classify operands just finely enough to separate a
  mnemonic's candidates; over-fine signatures bloat the table for no gain.
* **Witness vs canonical** — assembler builds (canonical, drop witness) and
  recompressor builds (replay witness) share the same `enc_tab`; only the driver
  policy differs.
* **`emit_prefixes` placement** — the field set fixes *which* prefixes; the
  `pfx_off`/`pfx[]` witness fixes *where*. Open: how much of the witness to keep
  through a transform — replay offsets for untouched groups, canonical-place the
  rest — and whether to preserve decoded redundant/ineffective prefixes at all.
