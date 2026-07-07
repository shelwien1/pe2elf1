# Lossless compression of executables via disassembly preprocessing

How disassembly-based preprocessing improves lossless compression of machine
code — the mechanism behind 7-Zip's BCJ/BCJ2 filters, what they leave on the
table, and how full byte-exact disassembly (as in Google's Courgette/Zucchini)
goes further. It closes with why the byte-exact bijective (dis)assembler in this
repo is the enabling primitive such a codec needs.

The theme throughout: **a disassembly transform does not compress anything —
it exposes structure the backend compressor (LZMA, a context model, bsdiff)
could not otherwise see.** The transform must be perfectly reversible; the
compression comes from the ordinary backend applied to a friendlier input.

---

## 1. Why executables resist general-purpose compression

A general compressor (LZ77 + entropy coding, or a context-mixing model) finds
redundancy as **repeated byte sequences** and **predictable next bytes**.
Machine code frustrates both, for two structural reasons.

### 1.1 Position-dependent operands (the pointer problem)

Most references in code are encoded as **displacements relative to the
instruction**, or as **absolute addresses**. Either way the *same logical
target* produces *different bytes* depending on where the referring instruction
sits.

Worked example — three calls to the same function `foo`:

```
  file off 0x1000:  E8 <rel32 = foo - 0x1005>
  file off 0x1A40:  E8 <rel32 = foo - 0x1A45>
  file off 0x2C80:  E8 <rel32 = foo - 0x2C85>
```

Semantically these are identical ("call foo"), but the three `rel32` fields are
three different 4-byte values. The LZ match-finder sees three unrelated
5-byte strings and cannot fold them. A binary with thousands of intra-module
calls carries thousands of these pseudo-random 4-byte holes punched through
otherwise-repetitive code.

Absolute addresses (x86-32 `mov eax,[0x00403010]`, jump tables, vtables,
pointer arrays in `.data`/`.rdata`) have the same defect, and worse: they don't
even share a common form the way `E8 rel32` does.

### 1.2 Column interleaving (opcodes mixed with operands)

An instruction stream interleaves fields of wildly different entropy:

```
  opcode   modrm   sib   displacement      immediate
  [low H]  [low H] [low] [medium/high H]   [high H]
```

Opcodes and ModR/M bytes are a tiny, highly repetitive alphabet — the "grammar"
of the program. Displacements and immediates are near-random. Laid out
sequentially they are *shuffled together*, so:

- LZMA's literal coder predicts each byte from the **previous byte(s)**. Before
  an opcode byte sits the last immediate byte of the previous instruction —
  noise. The strong predictor "opcode → next opcode" (instruction n-grams) is
  never in context.
- Matches straddle field boundaries and keep breaking on the noisy fields.

Separating the columns so each is contiguous restores both: the opcode stream
becomes a low-entropy language the model predicts well, and the noisy fields are
quarantined where they can't pollute anyone's context.

---

## 2. The current practice: BCJ and BCJ2

### 2.1 BCJ — branch/call/jump conversion

BCJ (used in xz/liblzma and 7-Zip) is a **stateless byte-scanning filter**, one
per architecture. The x86 variant scans for `E8` (call) and `E9` (jmp) bytes
and rewrites the following `rel32` from **relative to absolute**:
`abs = rel + position`. Now all three calls to `foo` above become the *same*
4-byte absolute value, and the match-finder folds them.

It is deliberately not a disassembler. To limit damage from `E8`/`E9` bytes that
are not really opcodes, it uses a cheap heuristic (a rolling mask of recent
candidate positions plus a check that the displacement's top byte is `0x00` or
`0xFF`, i.e. the "absolute" target is near). Conversion is **always reversible**
regardless of false positives — the decoder applies the inverse at the same
detected positions — so false positives cost only *ratio*, not correctness.

Per-architecture BCJ filters exist for x86, ARM, ARM-Thumb, ARM64, PowerPC,
SPARC, IA-64, and (recently) RISC-V. Each is a few dozen lines.

**Ceiling.** BCJ handles only the one or two opcodes it scans for. It does
nothing for: conditional jumps (`0F 8x`), absolute memory operands, RIP-relative
operands, jump/switch tables, pointer arrays in data, or the column-interleaving
problem. It converts rel→abs but does **not** separate streams.

### 2.2 BCJ2 — stream separation + learned decisions

BCJ2 (7-Zip) is the meaningful step up. It splits output into **four streams**:

- **main** — the code with the branch-target fields *removed*;
- **call** — the converted `E8` targets, collected together;
- **jump** — the converted `E9`/`jcc` targets, collected together;
- **rc** — a range-coded control stream recording, per candidate, whether it was
  a real branch to convert.

Two wins over BCJ. First, **stream separation**: the branch targets are now a
homogeneous block of 4-byte absolute addresses that compress far better on
their own, and the main stream is no longer riddled with those holes. Second,
the **range-coded decision** replaces BCJ's top-byte heuristic with a learned
"convert or not" bit, cutting the ratio cost of false positives.

BCJ2 is, in miniature, the whole idea of this document: *rel→abs* + *demux the
noisy column out*. It just does it for branch targets only, using pattern
matching rather than real decode.

---

## 3. The leap: full-disassembly preprocessing

Replace the byte-scan with a real decoder and the picture generalizes. The
pipeline:

```
  bytes ──decode──► instruction IR ──transform──► streams ──backend──► archive
  archive ──backend⁻¹──► streams ──assemble──► instruction IR ──encode──► bytes
```

A true disassembler knows **instruction boundaries** and **every operand's role**,
which unlocks three transforms BCJ/BCJ2 can only approximate:

### 3.1 Universal reference resolution (no false positives)

Because the decoder knows exactly which bytes are opcodes and which are
operands, *every* control-transfer and *every* address operand can be
rel→abs-normalized (or symbolized, §3.3) with **zero false positives**. Not
just `E8`/`E9`: conditional jumps, indirect-through-memory operands, LEA
addresses, RIP-relative data references, and pointer arrays in data all become
tractable. The false-positive problem that forces BCJ's heuristics simply does
not exist once you have a decode.

### 3.2 Stream demux (structure-of-arrays)

Transpose the instruction stream into parallel homogeneous streams — one per
field:

```
  opcodes/mnemonics │ prefixes │ modrm/reg roles │ displacements │
  immediates │ branch targets │ reloc-referenced addresses │ witness residue
```

Each stream is now internally homogeneous and gets its own model. The opcode
stream (a tiny alphabet with strong n-gram structure) collapses toward its true
entropy; immediates and displacements sit in their own streams where their
noise harms nothing. This is the column-interleaving fix of §1.2 done exactly.

### 3.3 Pointer/label abstraction (Courgette's core idea)

Go beyond rel→abs: replace every reference with a **symbolic label index** into
a separate label table.

- Collect all distinct targets (branch targets, absolute operands, reloc
  entries, jump-table entries) into a table.
- In the instruction stream, "call foo" / "jmp .L3" / "mov eax,[obj]" all become
  **reference-to-label-N**. The same target ⇒ the same small integer ⇒ the
  match-finder and the model see clean repetition.
- The label table is a list of addresses; sorted or in-reference-order it
  **delta-codes** to small values (nearby targets differ by little).

This is strictly stronger than rel→abs: rel→abs makes identical targets look
identical *as 4-byte values*; label abstraction makes them a *repeated small
symbol* and pulls the actual address values out into a separately-modeled,
delta-friendly table.

### 3.4 The non-negotiable constraint: byte-exact reversibility

All of the above is only legal if `assemble(disassemble(x)) == x` **byte for
byte** — including the encodings a normal disassembler discards (see §7). This
single requirement is why disasm-preprocessing is rare in production: an
ordinary disassembler (objdump, Capstone) normalizes, cannot re-emit the exact
input bytes, and desyncs on data-in-code. You need a *bijective* (dis)assembler.

---

## 4. The reference designs: Courgette and Zucchini

### 4.1 Courgette (Google, Chrome updates, ~2009)

Courgette targets **delta updates**, where the pointer problem is at its most
brutal: inserting one function shifts every following address, so a tiny source
change makes almost every pointer in the binary differ — and a raw binary diff
(bsdiff) balloons. Courgette's pipeline:

1. **Disassemble** old and new binaries into an "assembly program" IR with all
   internal pointers replaced by **symbolic labels**.
2. **Adjust**: align the two label sets so an unchanged target keeps the same
   label index across versions.
3. **Diff** the two IRs (bsdiff over the assembly programs). Because the
   symbolic stream barely moved, this diff is tiny; only the **label table**
   (the concrete addresses) really changed.
4. Ship (diff + new label table); the client **re-assembles** to exact bytes.

The reported effect was roughly an **order of magnitude** smaller updates than
bsdiff-on-raw-bytes (which was already far better than shipping a compressed
full binary): Google's example update dropped from **704,512 bytes (bsdiff) to
78,848 bytes (Courgette)**, ~9×. The lever is entirely §3.3: the diff no longer
drowns in shifted pointers.

### 4.2 Zucchini (Google, Courgette's successor)

Zucchini generalizes the same reference-abstraction idea to be more robust and
multi-architecture (x86, x64, ARM, AArch64, Android DEX, …). Rather than a full
decode-to-reassembly round, it locates **references** and their targets and
abstracts *those*, which is cheaper to maintain across ISAs and tolerant of
partial understanding of the binary. It is the current production executable-diff
engine in the Chromium update path. The conceptual core — pointers become
stable symbols, address values move to a separate table — is unchanged from
Courgette; the engineering is more general.

### 4.3 The connection to single-file compression

Courgette/Zucchini are *diff* engines, but the transform they apply is the same
one that helps a *single-file* compressor: symbolize references, separate the
address values, expose the repetitive symbolic stream. Diffing merely makes the
payoff spectacular because pointer instability dominates that problem. For
single-file compression the same transform helps — just less dramatically
(§5).

---

## 5. Single-file compression vs delta updates: where the gains live

Be honest about magnitudes; the transforms pay off very differently by use case.

| Transform | Single-file `.text` | Delta update |
|---|---|---|
| rel→abs (BCJ) | small, reliable (folds repeat calls) | moderate |
| stream demux (§3.2) | moderate–large on code | moderate |
| label abstraction (§3.3) | moderate | **dramatic** (order of magnitude) |

Two sobering facts for the single-file case:

1. **Code is often a minority of the file.** Data, resources, debug info, and
   already-compressed blobs dilute whatever you win on `.text`. Whole-file gains
   are smaller than code-section gains.
2. **The backend still does the work.** The transform's job is to feed LZMA/CM a
   better input; if the code section is small, the headline ratio moves little
   even when the code section itself compresses much better.

The realistic framing: disasm preprocessing yields a **solid, mechanism-justified
improvement on the code section** for single-file compression (materially more
than BCJ, because it fixes interleaving and covers all reference types), and a
**decisive** improvement for **delta updates** (because label stability attacks
the dominant cost directly). One concrete single-file data point: kkrunchy's
disassemble-and-split filter (§9), the most complete deployed instance of this
transform, reports **~20%** reduction on x86 code over its context-mixing
backend alone. If the goal is update delivery, this is the highest-leverage
technique available; if the goal is shrinking one static binary, it is a
worthwhile-but-bounded win concentrated where the code is.

---

## 6. A concrete codec design

Putting §3 together into a buildable shape.

### 6.1 Streams

Emit, per code region, a set of parallel streams (each compressed with a model
suited to it — order-N CM for opcodes, delta+entropy for the label table, raw
LZMA for immediates):

- **opcode/mnemonic** — the low-entropy backbone.
- **operand structure** — register numbers, addressing-mode shape, operand
  count. Highly repetitive.
- **displacements** — memory `disp` fields, delta-coded where sequential.
- **immediates** — the high-entropy residue; isolated so it pollutes nothing.
- **references** — label indices (from §3.3), the repetitive symbolic stream.
- **label table** — the concrete target addresses, sorted/delta-coded.
- **encoding witness** — the residue that makes reassembly byte-exact (§7).
  Near-empty for compiler output (canonical encodings), so it costs almost
  nothing yet guarantees losslessness.

### 6.2 Relocations are the pointer oracle

PE (`.reloc`) and ELF (`.rela`) relocation tables *tell you where the pointers
are* and what they point at — you need not guess. Use them to:

- find and symbolize absolute pointers precisely (no scanning);
- delta-code the relocation table itself (it is a sorted list of RVAs — very
  compressible);
- resolve jump/switch tables (arrays of code addresses) into label arrays.

Courgette and Zucchini both lean on relocation info for exactly this. A codec
that ignores it is leaving the easiest, most reliable pointers untouched.

### 6.3 Reassembly

Decompress all streams, then walk the instruction structure: for each record,
resolve label indices back to addresses (rebuild displacements/absolutes from
the label table + instruction position), and **re-encode to bytes using the
witness**. Fast — table lookups plus byte emission — which matters because the
update use case decompresses on millions of machines (asymmetric cost is fine:
compress once, decompress everywhere).

### 6.4 Robustness: code/data discrimination and fallback

Variable-length x86 means a single misidentified byte **desyncs the entire
downstream decode**. Defenses:

- Use section/symbol/reloc metadata to **bound code regions**; don't disassemble
  blindly across the whole file.
- On an unknown or malformed opcode, **stop and fall back to a raw (unfiltered)
  span** rather than guessing — a mis-decode that silently "succeeds" is far
  worse than a raw passthrough. The reassembler must reproduce raw spans
  verbatim.
- Handle data-in-code (jump tables, constant pools, alignment padding) as
  explicit raw or typed spans, not as instructions.

---

## 7. The hard part is reversibility — and this repo already solves it

Everything above rests on a bijective (dis)assembler:
`assemble(disassemble(x)) == x`, byte-exact, for **every** input including the
encodings normal tools normalize away. That is exactly what the (dis)assembler
in this repository is built to do, which makes it the missing primitive for a
disasm-preprocessing codec.

### 7.1 The bijection is the property BCJ can't offer and objdump can't provide

`c(d(bytes)) == bytes` holds here for **canonical and non-canonical** encodings.
A stock disassembler fails this three ways that all break a compressor:

- it **normalizes** redundant encodings (picks one `add eax,imm` form), so it
  cannot reproduce the input;
- it **discards** prefixes/padding/over-long displacements it deems irrelevant;
- it **desyncs** on data-in-code.

The decoder here instead records an **encoding witness** — the residue that
distinguishes equivalent encodings — so re-encode is exact.

### 7.2 `x86insn_t` is already the structure-of-arrays decomposition

The record this decoder produces is, field for field, the demux of §3.2 waiting
to happen:

```
  mnem                     → opcode stream
  op[].type / op[].index   → operand-structure stream
  disp                     → displacement stream
  imm                      → immediate stream
  mem_base/index/scale/seg → addressing stream
  (a branch's disp/imm)    → reference stream (symbolize via §3.3)
```

`asm32 d` already writes these as fixed-size records and `asm32 c` inverts them
exactly (the file is a whole number of `x86insn_t`, count = size/`sizeof`). To
build the codec's front half you **transpose those records into per-field
streams** instead of writing them interleaved — the semantic decomposition is
done.

### 7.3 The witness fields are what make it lossless — and they are cheap

`x86insn_t` carries precisely the residue §6.1 calls the "encoding witness":

- `enc:3` — which equal-cost opcode twin was used (`05` vs `81 /0` vs `83 /0`);
- `disp_w:2`, `sib:1`, `sscale:2` — non-canonical addressing (wide-zero disp,
  gratuitous SIB, dead scale bits);
- `reg_w:4` — dead ModR/M.reg bits on single-r/m forms;
- raw `pfx[]` + the VEX/EVEX bytes — replayed verbatim.

For ordinary compiler output these are almost all zero (canonical encodings), so
the witness stream compresses to nearly nothing — you pay a negligible price for
a *guaranteed* byte-exact round-trip. This is the exact trade a compressor wants:
lossless by construction, near-free on the common case.

### 7.4 The stop-at-unknown behavior is the §6.4 fallback

The decoder halts at an opcode it does not model (`mnem == 0xFFFF`) rather than
guessing — "a stream can't be resynchronised past an instruction of unknown
length." That is precisely the robustness contract §6.4 demands: the codec
treats such a point as the end of a filtered span and emits the remainder raw,
so an incomplete instruction model degrades ratio, never correctness.

### 7.5 What is still missing to turn it into a codec

The decoder/encoder is the hard 80%. To reach a working preprocessor you would
add, on top of this repo:

1. a **container/driver** that bounds code regions from ELF/PE section+reloc
   metadata and routes non-code as raw spans (§6.2, §6.4);
2. **reference symbolization** — turn branch/absolute targets into a label table
   + indices (§3.3), the one transform not yet present (the decoder resolves
   targets to values but does not yet pool them into labels);
3. **stream transposition** — write per-field streams rather than interleaved
   records (§7.2);
4. a **backend** per stream (reuse LZMA, or a small CM for the opcode stream).

None of these needs new instruction knowledge; they are plumbing around the
bijection the repo already guarantees.

---

## 8. Costs and trade-offs

- **Speed.** Full decode is slower than a byte scan. Acceptable because the cost
  is asymmetric: reassembly (the client side) is fast table-lookup + emit, and
  compression is one-time. For interactive whole-file compression the decode
  cost is the main tax.
- **Per-architecture investment.** A bijective decoder is a large per-ISA effort
  (this repo is one such, for x86-64) versus BCJ's per-arch handful of lines.
  This is the real reason BCJ won in general-purpose tools and full disasm lives
  in specialized pipelines (browser updaters).
- **Format/model complexity.** Many streams, each wanting a model; a richer
  container. More moving parts than "filter + LZMA."
- **Modern PIE/PIC.** x86-64 position-independent code already uses RIP-relative
  addressing, so intra-module data references are position-relative and somewhat
  more stable than x86-32 absolutes — the pointer problem is *shifted* (into GOT
  entries, PLT stubs, and RIP-relative disps) rather than removed. Label
  abstraction still helps but the baseline is less pathological than 2009-era
  x86-32. Delta updates still benefit strongly (any insertion still perturbs
  RIP-relative disps and GOT layout).

---

## 9. Landscape

| Tool / technique | Goal | Reference handling | Streams | Disasm depth |
|---|---|---|---|---|
| **BCJ** (xz/7-Zip) | single-file filter | E8/E9 rel→abs | in-place | byte scan |
| **BCJ2** (7-Zip) | single-file filter | E8/E9/jcc rel→abs + range-coded | 4 (main/call/jump/rc) | byte scan |
| **Courgette** (Google) | delta update | symbolic labels + label table | assembly IR | full, reassembly-grade |
| **Zucchini** (Google) | delta update | generalized reference abstraction | reference/target tables | reference-level, multi-arch |
| **bsdiff** (Percival) | delta update | none (raw byte diff) | — | none |
| **Crinkler** (demoscene) | tiny PE packer | E8E9 filter | reordered sections | PAQ-style context models |
| **kkrunchy** (demoscene) | 64k PE packer | rel→abs via function-address cache + jump-table detect | **per-field split streams** | **disassembles x86** (escape for unknowns) |
| **PAQ/cmix `exe` model** | max-ratio archiver | E8E9 transform + model | model contexts | filter + CM |
| **UPX** | self-extracting packer | BCJ-like E8/E9 call/jump filter (id 0x49) | — | filter-level |

Two distinctions worth keeping straight:

- **Packers (UPX, Crinkler, kkrunchy)** produce a *self-decompressing
  executable* — a different goal from improving a general archiver's ratio,
  though they use the same class of preprocessing internally (kkrunchy the most
  thoroughly; see §10).
- **Diff engines (Courgette, Zucchini, bsdiff)** compress *the change between two
  binaries*; disasm preprocessing helps them most because pointer instability is
  the dominant cost there.

---

## 10. Where to push beyond the state of the art

The production tools each stop short in a different place:

- BCJ/BCJ2 have the right ideas (rel→abs, stream split) but only for a couple of
  opcodes, via pattern matching, with no operand-column demux.
- Courgette/Zucchini symbolize references thoroughly but are aimed at *diffing*,
  not single-file ratio, and Zucchini deliberately avoids reassembly-grade
  disassembly for maintainability.
- **kkrunchy is the closest prior art** and deserves the credit: it already
  disassembles x86, demuxes per-field streams (§3.2), does rel→abs, and detects
  jump tables — the §3/§6 design, shipping since the mid-2000s. What it does
  *not* do is decode the **complete** ISA byte-exactly: it targets
  compiler-generated 64k-intro code and **escapes** instructions it doesn't
  model rather than round-tripping them, and it has no relocation-driven pointer
  discovery (32-bit intros are position-dependent).

So the ideas are not new; the gap is **completeness and generality**. A codec
that combines **(a)** reassembly-grade bijective decode over the *whole* x86-64
ISA including non-canonical encodings (this repo — kkrunchy's escape hatch
becomes an exact decode), **(b)** Courgette-depth label abstraction across all
reference types, **(c)** kkrunchy-style per-field demux with per-stream models,
and **(d)** relocation-driven pointer discovery — on arbitrary production
binaries, for both single-file and delta scenarios — is what no single shipping
tool offers. The bijective (dis)assembler here supplies exactly the piece the
others approximate (kkrunchy's escapes, Zucchini's partial understanding); the
remainder (§7.5) is plumbing.

The honest expectation to set: on the **code section** this beats BCJ/BCJ2
clearly (it fixes interleaving and covers every reference type); on **whole
static files** the win is real but bounded by the code fraction; on **delta
updates** it is transformative, for the same reason Courgette was.
