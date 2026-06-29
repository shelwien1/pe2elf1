# corpus64.p — x86-64 (long mode) bit-pattern description

`corpus64.p` is the x86-64 companion to `corpus.p`. It is a declarative
bit-pattern ⇄ string description in the same language (`corpus-p-syntax.md`)
and is driven by the same two engines:

* `parsergen.py corpus64.p IMAGE.bin` — **disassemble** bytes → text
* `asm.py corpus64.p INPUT.asm`        — **assemble** text → bytes

The two are exact inverses: for any covered instruction,
`asm(disasm(bytes)) == bytes` and `disasm(asm(text)) == text`.

```
./tools/roundtrip.sh        # verify every round-trip is byte-exact
```

## Coverage

`corpus64.p` (~2000 lines, ~1700 rules, 92 tables) covers the
classic x86-64 integer/FP ISA plus AVX/AVX2 and a broad slice of AVX-512:

| group | examples |
|-------|----------|
| legacy 1-byte + REX | mov/lea/alu/shift/test/imul/inc/dec/push/pop/xchg, movsxd, movabs, string ops (movs/stos/.../q), in/out, cbw/cwde/cdqe/cqo |
| branches | jmp/call (rel8/rel32/indirect/far), Jcc (short + near), loop, ret/retf, int, enter/leave |
| 0F two-byte GP | jcc-near, setcc, cmovcc, movzx/movsx, bt/bts/btr/btc, bsf/bsr/tzcnt/lzcnt/popcnt, shld/shrd, cmpxchg/cmpxchg8b/16b, xadd, bswap, movnti |
| system | 0F 00/01 (lgdt/sgdt/lldt/...), swapgs, rdtscp, cpuid, rd/wrmsr, syscall, mov cr/dr, lar/lsl, clflush/fences/fxsave, endbr64/32 |
| SSE/SSE2/SSE3 | mov-family, packed/scalar FP arith+cmp+cvt, MMX/SSE2 integer, movd/movq, pshuf, shifts-by-imm, pinsrw/pextrw, pmovmskb, lddqu/haddps/... |
| SSSE3/SSE4/AES/SHA | pshufb/phadd/pmovsx/zx/pmuldq/pcmpeqq/pcmpgtq/pmin/max, ptest, round/blend/palignr/dpps/pclmulqdq/pcmpestri/aeskeygen, aes*, sha*, crc32, movbe, adcx/adox, pextr/pinsr (b/w/d/q) |
| x87 | full D8–DF (arith, fld/fst, fcmov, fucomi, transcendentals, control) |
| AVX/AVX2 (VEX) | C4 & C5 forms: packed/scalar FP arith + vsqrt, vmovaps/ups/pd/dqa/dqu + scalar vmovss/sd (+stores), vmovd, vmovq (xmm↔xmm/m64 load 7E & store D6, incl. the extended-register dual-encoding), integer (vpadd/psub/pand/por/pxor/pcmpeq/gt/punpck/...), vcmp*/vshuf*/vpshufd (imm8), vpshufb, shift-by-imm (vpsll/srl/sra), scalar converts (vcvtsi2sd/ss, vcvt[t]sd2si/ss2si) + same-width packed (vcvtdq2ps/ps2dq/ttps2dq), vbroadcastss/sd, vzeroupper/all |
| AVX-512 (EVEX) | zmm0–31 with `{k1-7}` masking, `{z}` zeroing, `{1toN}` broadcast and `{er}`/`{sae}` embedded rounding: FP arith ps/pd/ss/sd, min/max, logical, vmovaps/upd/dqa32/64/dqu8/16/32/64 (+stores), integer vpadd/sub/and/or/xor/mull (d/q/b/w), FMA (vfmadd/sub/nmadd 132/213/231 ps/pd) |
| AVX-512 more | same-width converts (vcvtdq2ps/ps2dq/ttps2dq + `{er}`/`{sae}`), scalar converts (vcvtss2sd/sd2ss, vcvtsi2sd/ss, vcvt[t]sd2si/ss2si with `{er}`/`{sae}`), broadcasts (vbroadcastss/sd, vpbroadcastd/q from xmm/mem **and GPR**), integer min/max (vpmin/max s/u b/w/d/q), abs (vpabs b/w/d/q), variable shifts (vpsll/srl/srav d/q), shift-by-imm (vpsll/srl/sra w/d/q), permutes (vpermd/ps, vpermq/pd imm8) |
| AVX-512 yet more | `vpternlogd/q` (3-input LUT, imm8), `vsqrtps/pd/ss/sd` (`{er}`), integer multiply (vpmuldq/muludq/mullq), round-to-scale (vrndscaleps/pd/ss/sd, imm8 + `{sae}`), `vscalefps/pd/ss/sd` (`{er}`), reciprocal/rsqrt approximations (vrcp14/vrsqrt14 ps/pd/ss/sd), `vgetexpps/pd` + `vgetmantps/pd` (imm8 + `{sae}`), mask-merge blends (vpblendmd/q, vblendmps/pd), and `vptestnmd/q` → k |
| AVX-512 masks (k) | `kmovw/b/d/q`, k-logic (`kand/kandn/kor/kxor/kxnor/knot/kortest/ktest/kadd/kunpck`), `kshiftl/rw`, and mask-producing compares `vpcmp{eq,gt}d`/`vcmpps/pd`/`vpcmp[u]d/q`/`vptestmd/q` → k (with optional `{k}` mask) |
| XOP (8F, AMD) | `vpcmov`/`vpperm`/`vpmacsdd` (4-operand is4), `vprot{b,w,d,q}` (imm8 + variable), `vpsh{l,a}{b,w,d,q}`, `vphadd*`/`vphsub*` |

Validated against GNU `as`/`objdump`: every instruction in the test corpus
assembles to the exact bytes `as` produces, and the entire `.text` of `/bin/ls`
(~20 000 instructions) disassembles with **zero** undecodable bytes.

**Not yet covered (future work):** AVX-512 VSIB gather/scatter (vector-index
SIB); the EVEX forms of the *width-changing* converts (vcvtps2pd/pd2ps,
vcvtdq2pd/pd2dq, where the two operands differ in vector width — these are
covered for VEX but not EVEX); and the long tail of rarer EVEX leaves. EVEX
`disp8` is shown as the raw encoded byte rather than the disp8×N effective
displacement (the corpus.p convention — see below). The `vcmp`/`vpcmp`
predicate is kept as an explicit `imm8` operand rather than folded into the
mnemonic (objdump folds it).

## How long mode is expressed

The disassembler engine (`parsergen.py`) is architecture-agnostic, so almost
everything new lives in `corpus64.p`:

* **REX (`0x40..0x4F`)** is a prefix frame that sets `$rexw/$rexr/$rexx/$rexb/
  $rex` and emits **no token** — its bits are recovered straight from the
  register names, exactly as the `66` prefix is recovered from a 16-bit name in
  `corpus.p`. A 64-bit name reveals `W`; an `r8..r15` name reveals the relevant
  `R/X/B` extension; `spl/bpl/sil/dil` reveal REX presence.
* **Register files** are widened to 16 and to 64-bit. The GP index is the clean
  bit-pack `32*$rexw + 16*$opsiz + 8*hi + lo` (`hi` = REX.R/B/X), so the
  assembler's reverse solver inverts it positionally.
* **64-bit addressing**: `mod=00 rm=101` is RIP-relative (`[rip+disp]`); absolute
  `[disp32]` is the SIB `base=101,index=none` form; bases/indexes are `rax..r15`
  (REX.B/X extended). `0x67` switches to 32-bit addressing.
* **Default-64 operand size** for the stack / near-branch group uses a separate
  `dreg` table; REX.W is ignored there.
* **VEX/AVX**: `R/X/B/vvvv` are stored *inverted* in the prefix, so the register
  tables (`vreg`, `vvv`) use an inverted-extension layout that keeps the
  assembler's solver coefficients positive. C5 (2-byte) and C4 (3-byte) forms
  are separate submatches; the assembler picks the shorter that re-disassembles.
* **EVEX/AVX-512**: the 4-byte `0x62` prefix carries inverted `R/X/B/R'/V'`
  extension bits selecting zmm0–31 — handled with the same inverted-layout-table
  trick (`ereg`/`evvv`, 96 entries spanning xmm/ymm/zmm). `{k}`/`{z}` render via
  `kzdec`, `{1toN}` via `bcst32/64`, `{er}` via `rcdec`. As in `corpus.p`, the
  EVEX `disp8` is rendered as the raw encoded byte (disp8×N scaling is not
  applied); this keeps the round-trip byte-exact without per-instruction tuple
  tables. The assembler suppresses any REX byte before a VEX/EVEX/XOP lead and
  bridges memory-operand REX.B/X into the inverted prefix fields (`asm._vexfix`).

## Engine changes (`asm.py`)

The disassembler needed no changes. The assembler's reverse path gained
x86-64-specific support (kept mode-driven so the 32-bit `corpus.p` round-trip is
preserved):

1. a general **positional bit-pack solver** in `solve_to`, to invert
   multi-field register indices in one shot;
2. a **64-bit addressing reverse-matcher** (`_enc64`): `rax..r15` base/index,
   RIP-relative, absolute-via-SIB, REX.B/X into the env;
3. **REX byte re-derivation** in `revealed_bytes` (emitted last, masked to one
   bit per field);
4. a **VEX bridge** (`_vexfix`) mapping memory-operand REX.B/X into the inverted
   VEX byte-1 fields, and suppression of any REX byte before a VEX/XOP/EVEX lead.

## Files

| file | role |
|------|------|
| `corpus64.p` | the x86-64 description (deliverable) |
| `corpus64.asm` / `corpus64.bin` | round-trip test: text ⇄ bytes (deliverable) |
| `evex64.asm` / `evex64.bin` | AVX-512 EVEX round-trip test (built `.s`→GAS→`.text`) |
| `xop64.asm` / `xop64.bin` | XOP round-trip test (built `.s`→GAS→`.text`) |
| `tests/prog.c` / `prog.bin` / `prog.asm` | round-trip of real `gcc -O2` output |
| `x86d64.py` / `x86a64.py` | standalone disassembler / assembler (`--emit`) |
| `tools/rtcheck.py` | per-instruction cross-check against GNU `as`/`objdump` (`--bin` for a raw blob) |
| `tools/gen_sse.py`, `gen_38_3a.py`, `gen_vex.py`, `gen_evex.py` | rule generators for the regular maps |
| `tools/corpus64-src.s`, `tools/evex64-src.s` | Intel-syntax sources the test corpora are built from |
| `tools/roundtrip.sh` | runs every round-trip check |
