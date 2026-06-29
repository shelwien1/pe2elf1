# Lossless inverse of `parse_addr` — assembling ModR/M + SIB + disp from `x86insn_t`

The decoder turns `ModR/M [SIB] [disp]` into the operand fields of `x86insn_t`
(`mem_base`, `mem_index`, `mem_scale`, `mem_disp`, `mem_seg`, `mode`, plus the
ModR/M reg field). This is the inverse: operand fields → the **exact** bytes the
decoder consumed. The catch is that x86 addressing decode is **many-to-one** —
several byte sequences decode to the same operand — so the semantic operand alone
does not determine the bytes. A byte-exact inverse needs a small *encoding
witness* alongside the operand. With it, `encode ∘ decode = id` and
`decode ∘ encode = id` over the whole addressing domain.

This is the property the disassembler-as-compression-preprocessor needs: the
structured form must be a bijection with the byte stream, or the transform is not
reversible. The payoff is that the *semantic* operand (base/index/scale/disp)
carries the modellable signal, while the witness is a tiny residual that is
all-default for canonically-assembled code and entropy-codes to ~nothing.

---

## 1. Scope

In: `mode` (mem vs reg-direct), `mem_base`/`mem_index` (register number 0–7 or
`NONE`), `mem_scale` (1/2/4/8), `mem_disp` (int32), `addr` (0 = 32-bit, 1 =
16-bit), the ModR/M reg field (other operand or `/digit`), and the witness δ
(§3). Out: the `ModR/M [SIB] [disp]` byte run.

Out of scope — and deliberately so:

* **The segment is not encoded here.** The decoder derives it as
  `seg[sbo[base] + segidx]` (32-bit) / `seg[sbo16[rm] + segidx]` (16-bit): the
  *default* segment is a pure function of the base register, and any override
  rides in a **prefix** byte (`26/2E/36/3E/64/65`), which the prefix encoder
  emits. So `mem_seg` is reconstructed, not assembled, and contributes zero bytes
  to the ModR/M+SIB+disp run. The inverse only needs to know whether an override
  prefix existed if it must reproduce it — that is the prefix encoder's witness,
  not this one.
* **No opcode, no immediate.** Only the addressing bytes.

---

## 2. Decode is many-to-one: the redundancies to witness

Each row is a distinct byte sequence that decodes to the operand on the left.
Storing only the operand loses the choice.

| operand | encodings that all decode to it |
|---------|----------------------------------|
| `[eax]` | `mod=00 rm=000` (no SIB) **or** `mod=00 rm=100, SIB=(ss,idx=100,base=000)` (redundant SIB, any `ss`) |
| `[eax+0]` | `[eax]` as above (disp omitted) **or** `mod=01 disp8=0` **or** `mod=10 disp32=0` |
| `[eax+8]` | `mod=01 disp8=08` **or** `mod=10 disp32=00000008` |
| `[ebp]` | **not** `mod=00 rm=101` (that is `[disp32]`); must be `mod=01 rm=101 disp8=0`, **or** via SIB `mod=01 rm=100 SIB.base=101 disp8=0` |
| `[disp32]` | `mod=00 rm=101 disp32` (no SIB) **or** `mod=00 rm=100, SIB=(ss,idx=100,base=101) disp32` (redundant SIB, any `ss`) |
| `[eax*4+disp32]` | `mod=00 rm=100, SIB=(ss=10,idx=000,base=101) disp32` — disp is **forced** D32, no shorter form |
| `[bp]` (16-bit) | **not** `mod=00 rm=110` (that is `[disp16]`); must be `mod=01 rm=110 disp8=0` |

The free bits, distilled:

1. **disp width.** A value that fits in 8 bits (including 0) may be encoded D8 or
   D32 (16-bit: D8 or D16). A 0 displacement with a non-`ebp`/`bp` base may also be
   *omitted* (mod=00). The chosen width is not implied by the value.
2. **`ebp`/`bp` forcing.** `mod=00 rm=101` (32-bit) and `mod=00 rm=110` (16-bit)
   are the absolute-displacement slots, so an actual `[ebp]`/`[bp]` base **cannot**
   use mod=00 — it is forced to at least D8=0. (Same when the SIB base field is
   `101`.) This is determined by the base, so it is *not* free — but the inverse
   must respect it.
3. **redundant SIB.** A single non-`esp` base with no index can be written with or
   without a SIB byte; `[disp32]` likewise. One bit.
4. **dead scale.** When the SIB index field is `100` (= no index), the 2 `ss`
   bits are ignored by the decoder but still occupy the byte. Two bits.

`esp` as a base **forces** a SIB byte, and `esp` (`100`) can never appear as a SIB
index — those are constraints, not freedoms.

---

## 3. The encoding witness δ

Define the **canonical** encoding (shortest, no redundant SIB, `ss=0` when dead),
and let δ record only the deviation. δ is per-memory-operand and tiny:

```
struct AddrWitness {        // sib = sscale = 0 and disp_w = canonical width  <=>  canonical
  uint8_t disp_w : 2;       // 0=NONE, 1=D8, 2=D16/D32  (actual width used)
  uint8_t sib    : 1;       // SIB present though not required (redundant SIB)
  uint8_t sscale : 2;       // raw ss bits when index == NONE (dead-scale value)
};
```

* `disp_w` is the *actual* width the decoder saw (from `mod`, and from the
  `base==101 @ mod00` no-base rule). The canonical (shortest) width is recomputable
  from the operand (`canon_disp_w`, §8); `disp_w` exceeds it only when a
  longer-than-needed disp was used, so a model that predicts the canonical width
  pays bits only for that excess.
* `sib` distinguishes the redundant-SIB forms of `[base]` and `[disp32]`. It is `0`
  for the *required* SIB of an `esp` base or an indexed form — only a needless SIB
  sets it.
* `sscale` preserves the dead `ss` bits so a redundant/`esp`-base SIB round-trips
  bit-exactly. Live scale (`index != NONE`) comes from `mem_scale`, not δ.

**The decoder must emit δ.** `parse_addr` already knows `mod`, whether a SIB byte
was present, and the raw `ss` bits; it currently discards them. Capturing them
into δ (three more captures: `CAP_DISP_W`, `CAP_SIB_PRESENT`, `CAP_SSCALE`) is the
only decoder-side change, and it is what makes the pair a bijection. δ is not a
separate record — these three are **fields of `x86insn_t`** (`disp_w`/`sib`/
`sscale`), so the struct alone round-trips the bytes (see `FULL_ENCODE.md` §11).

For compression: δ lives in the struct, but the serializer may **demux it into a
side stream** separate from the operand fields. On canonical output `sib` and
`sscale` are literally zero, and `disp_w` coded against the predicted canonical
width (`canon_disp_w`, §8) carries no excess — so the stream costs almost nothing
on compiler code, while hand-written or obfuscated code spends bits there exactly
in proportion to its non-canonical choices.

---

## 4. Encoder inputs

| field | source in `x86insn_t` | meaning |
|-------|-----------------------|---------|
| `mode` | `op[k].type` (T_MEM vs T_GPR) | memory vs reg-direct |
| `base` | `mem_base` (0–7 or `NONE`) | base register number |
| `index` | `mem_index` (0–7 or `NONE`) | index register number (`esp`/4 impossible) |
| `scale` | `mem_scale` (1/2/4/8) | live only when `index != NONE` |
| `disp` | `mem_disp` (int32) | displacement value |
| `addr16` | `addr` | address size |
| `reg` | the ModR/M reg field (other op / `/digit`) | ModR/M[5:3] |
| `rm_reg` | the r/m operand register (reg-direct) | ModR/M[2:0] when `mode==REG` (`CAP_RM`) |
| δ | new `AddrWitness` (§3) | encoding residual |

`NONE` is the existing absent-register sentinel (distinct from `edi`/`di`=7;
see the size-independent `store_reg` numbering). `reg` for a `/digit` group is
just the digit.

---

## 5. 32-bit assembly

```
emit_modrm32(out, m):                 // m = inputs from §4, m.addr16 == 0
  // --- reg-direct: trivial, no SIB/disp ---
  if m.mode == REG:
      out << byte(0b11<<6 | m.reg<<3 | m.rm_reg)        // rm_reg = the register
      return

  need_sib = (m.index != NONE) || (m.base == ESP) || m.delta.sib

  // --- no base ---
  if m.base == NONE:
      if need_sib:                                       // [index*scale + disp32] or forced [disp32]
          mod = 0; rm = 0b100
          out << byte(mod<<6 | m.reg<<3 | rm)
          idx = (m.index==NONE) ? 0b100 : m.index
          ss  = (m.index==NONE) ? m.delta.sscale : log2(m.scale)
          out << byte(ss<<6 | idx<<3 | 0b101)            // base=101 + mod0 => no base
          emit_disp(out, D32, m.disp)                    // forced
      else:                                              // canonical [disp32]
          out << byte(0<<6 | m.reg<<3 | 0b101)
          emit_disp(out, D32, m.disp)
      return

  // --- base present ---
  dw  = m.delta.disp_w                                   // NONE / D8 / D32
  if m.base == EBP && dw == NONE: dw = D8                // ebp forbids mod=00
  mod = (dw==NONE)?0 : (dw==D8)?1 : 2
  rm  = need_sib ? 0b100 : m.base
  out << byte(mod<<6 | m.reg<<3 | rm)

  if need_sib:
      idx = (m.index==NONE) ? 0b100 : m.index
      ss  = (m.index==NONE) ? m.delta.sscale : log2(m.scale)
      out << byte(ss<<6 | idx<<3 | m.base)               // base != 101 here unless base==EBP w/ mod>=1
  emit_disp(out, dw, m.disp)                             // NONE emits nothing
```

Notes. `ESP`=4 always takes the SIB path (`rm=100`, `SIB.base=100`, `index=100` if
none). `EBP`=5 as a base forces `dw ≥ D8` whether or not a SIB is used (the SIB
`base=101 @ mod=00` slot means *no base*). `emit_disp` writes 0/1/4 little-endian
bytes; the value is truncated to the chosen width (the decoder sign-extends D8, so
a witnessed D8 is only legal when `disp` fits signed-8 — guaranteed by a
consistent δ).

---

## 6. 16-bit assembly

No SIB. The eight `rm16` forms invert directly; `esp`/scale do not exist.

```
rm16_of(base, index):           // inverse of table rm16 = {bx+si,bx+di,bp+si,bp+di,si,di,bp,bx}
  (BX,SI)->0  (BX,DI)->1  (BP,SI)->2  (BP,DI)->3
  (SI,NONE)->4  (DI,NONE)->5  (BP,NONE)->6  (BX,NONE)->7

emit_modrm16(out, m):
  if m.mode == REG: out << byte(0b11<<6 | m.reg<<3 | m.rm_reg); return

  if m.base == NONE && m.index == NONE:                  // canonical [disp16]
      out << byte(0<<6 | m.reg<<3 | 0b110)
      emit_disp(out, D16, m.disp); return

  rm = rm16_of(m.base, m.index)
  dw = m.delta.disp_w                                    // NONE / D8 / D16
  if rm == 6 && dw == NONE: dw = D8                      // [bp] forbids mod=00 (that slot = [disp16])
  mod = (dw==NONE)?0 : (dw==D8)?1 : 2
  out << byte(mod<<6 | m.reg<<3 | rm)
  emit_disp(out, dw, m.disp)
```

The `rm==6` guard is the 16-bit twin of the `ebp` rule: `mod=00 rm=110` is the
`[disp16]` slot, so a real `[bp]` base is forced to `disp8=0` minimum.

---

## 7. Reg-direct (mod = 11)

`parse_addr` sets `mode = RM_REG` and captures the rm field as a register
(`CAP_RM`). The inverse is one byte: `mod=11, reg = ModR/M reg, rm = the
register number`. No SIB, no disp, no witness. Register *size* (32 vs 16) is an
opsize/`reg_name` concern at render, not an addressing byte — identical low-3-bit
number either way.

---

## 8. Canonical (lossy-but-valid) inverse — a plain assembler

When byte-exactness is not required (normalizing/re-assembling, or emitting fresh
code), drop δ and compute the canonical width:

```
canon_disp_w(base, index, disp, addr16):
  if base == NONE: return addr16 ? D16 : D32            // absolute or no-base+index
  forced = addr16 ? (base == BP && index == NONE)       // 16-bit: only [bp]-alone is the disp16 slot (rm=110)
                  : (base == EBP)                        // 32-bit: any ebp base (SIB base=101 @ mod0 = no base)
  if disp == 0 && !forced: return NONE
  if fits_s8(disp): return D8
  return addr16 ? D16 : D32
```

The forcing asymmetry is the one subtlety: 32-bit `ebp` forces a disp **with or
without** an index (the SIB `base=101 @ mod=00` slot means *no base*), whereas
16-bit forces only `[bp]` *alone* — `[bp+si]`/`[bp+di]` (rm=2/3) keep their own
mod=00 forms and do not collide with the `[disp16]` slot.

Take `sib = required-only` and `sscale = 0`; feeding `δ = {canon_disp_w, 0, 0}`
into §5/§6 yields the shortest standard encoding. This is exactly the δ ≡ 0 point
of the bijection: the canonical encoder is the lossless one with the witness
pinned to its default.

---

## 9. Round-trip verification

The bijection domain is finite and small; test it exhaustively against
`parse_addr`, mirroring the existing decoder self-check:

* **decode → encode (byte identity).** For both address sizes, every ModR/M byte
  `00..FF`, every SIB byte where `rm=100`, and each legal disp pattern: decode to
  `(operand, δ)`, re-encode, assert the emitted bytes equal the input bytes. This
  is the property that matters for the preprocessor — it covers the redundant
  forms because δ distinguishes them.
* **encode → decode (operand identity).** For every reachable `(operand, δ)`,
  encode then decode, assert the operand and δ come back. Enumerate δ over its
  ≤ 32 values per operand; invalid `(operand, δ)` pairs (e.g. `disp_w=NONE` with
  an `ebp` base) are excluded by construction.

Both directions belong in `gen.py --check` next to the decode simulation, so any
table edit that breaks the inverse is caught immediately. The `esp`-base,
`ebp`/`bp`-forcing, no-base-SIB, redundant-SIB, and dead-scale corners are the
ones to assert explicitly — they are where a naive inverse silently diverges.

---

## 10. One source: emit both directions from `corpus.p`

The encoder tables are literally the inverses of the decode tables already in
`corpus.p`: `rm16_of` inverts `rm16`; the base/index/scale field placement
inverts `sib0`/`sib1`; the `mod`↔`disp_w` mapping inverts the `addr1` mod rows.
The DSL's bidirectional fields are meant for exactly this — a field that the
decoder *reads* from a byte is the field the encoder *writes* into it. So `gen.py`
can emit `parse_addr` and `emit_addr` from the same rules, keeping them
drift-free under the single-source-of-truth invariant.

The witness δ is the only thing the forward rules do not already name: it is the
residual the bijection needs where decode collapses distinct bytes. Generating
the encoder is therefore "run the `addr1`/`sib0`/`sib1` rules backwards, and thread
δ through the points where the backward map is one-to-many." Everything else —
register numbering, the `esp`/`ebp` special slots, disp widths — is the same data,
read in the other direction.
