# The Compressed Data Model (CDM): Analysis, Verified Against Source

*Primary source:* Shelwien, *encode.su* thread #2742, "Compressed data model"
(April 2017, releases `cdm_test_1` through `cdm_test_5`). Distribution:
`http://nishi.dreamhosters.com/u/cdm_test_*.rar`.

This document consolidates seven independent reviews of CDM into a single account,
verified line-by-line against the original forum thread. Every quantitative claim
below has been checked against the posted benchmark tables and Shelwien's own
descriptions; where the reviews disagreed or overstated, the discrepancy is noted
inline. It then proposes (1) targeted attacks that beat CDM on identifiable upstream
codecs, and (2) a new section on residual redundancies CDM currently leaves on the
table that a CDM-like post-coder could be extended to capture.

---

## 1. What CDM Is

CDM is a **post-coder**. It consumes a stream presumed to be near-incompressible —
typically the output of another compressor — and extracts residual redundancy that
survives because most coders pad, byte-align, use slightly mismatched models, or
quantise probabilities. It is *not* an LZ, *not* a context mixer, and *not* a
model-aware coder for any specific format. Shelwien explicitly recommends
LZ4-precompressing redundant inputs and running CDM on the result, "because CDM
doesn't include LZ compression" (Post #4).

In Post #5, Jarek correctly identified the underlying idea as classical **enumerative
(combinatorial) coding** — the binomial recurrence `C(n,k) = C(n-1,k) + C(n-1,k-1)`
used to enumerate the `C(n,k)` arrangements of `k` ones in `n` positions, the same
construction at the heart of Fischer's Pyramid Vector Quantizer and Daala's PVQ.
Shelwien's reply (Post #6) is the key clarification: CDM does *not* use an explicit
combinatorial enumerator. He tried one "a very long time ago" (precomputing binomials
via Pascal's triangle, multiplication-free) and found no compression gain over a
rangecoder and no speed advantage, with much more complexity. CDM therefore implements
enumerative coding with an ordinary — but precise — rangecoder.

The single observation underlying the whole design:

> A "random-looking" bit block of length `n` with population count `k ≠ n/2` is **not**
> uniformly distributed over `2ⁿ` strings — it is uniformly distributed over `C(n, k)`
> strings. The deficit `n − log₂ C(n, k)` is real, recoverable redundancy.

Shelwien's worked example (Post #1), reproduced exactly:

```
1024 − log₂ C(1024, 412) = 33.6576 bits
```

(Here `412 = 1024/2 − 100`, so `k = 412` zeros, `612` ones.) If the side-information
needed to communicate `k` — or, more precisely, the model parameter that lets the
decoder reach `k` — costs less than 33.66 bits, the block compresses losslessly. CDM
does this across many small blocks.

---

## 2. Algorithmic Mechanics

### 2.1 Block coder: stored vs. enumerative

For each candidate block CDM chooses between two options:

1. **Stored** — emit the raw bits verbatim, plus a flag.
2. **Coded** — communicate `freq0` first, then encode each bit with a *known,
   decrementing* per-symbol probability.

The coded path initialises the rangecoder with `c0 = freq0`, `c1 = blklen − freq0`,
and decrements the relevant counter after every emitted bit (Post #6: "instead of
starting with `freq0=1, freq1=1` and incrementing them, I'm starting with
`freq1 = blklen − freq0` and decrementing"). Because the decoder knows `freq0` up
front, at every step `P(bit = 0) = remaining_zeros / remaining_length` is exact, and
the resulting codelength satisfies

```
codelen ≈ log₂( (freq0 + freq1)! / (freq0! · freq1!) )  =  log₂ C(n, freq0).
```

Shelwien contrasts this with the standard *adaptive* (Krichevsky–Trofimov-style)
variant, which codes an unknown count and produces

```
codelen ≈ log₂( (freq0 + freq1 + 1)! / (freq0! · freq1!) )
```

— exactly `log₂(n + 1)` bits more. He notes this is "the same as encoding `freq0`
value with probability `1/(blklen+1)`" — i.e. a uniform prior over the `n+1` possible
counts. CDM pays the count cost *explicitly* precisely so it can spend **less** than
`log₂(n+1)` bits on `freq0`, using a non-uniform prior. As Shelwien puts it, "with
compressed data we would have a pretty good idea about `freq0` value, rather than
assuming it to be purely random."

**That non-uniform prior on `freq0` is the entire mathematical novelty of the scheme.**
Everything else is engineering around it.

### 2.2 Parsing optimiser

The block model is useless without a rule for chopping the input. CDM runs a parsing
optimiser — a forward dynamic program over candidate block boundaries — that minimises
total encoded length, deciding per block between stored and coded. To make the search
affordable, codelengths are *estimated* during parsing, not computed exactly, via
(Post #6):

- a fixed-point `log₂(n)` lookup table,
- a `log₂(n!)` table built from it,
- so that `log₂ C(c0+c1, c0) = LUT[c0+c1] − LUT[c0] − LUT[c1]` is three lookups and
  two adds.

`maxblk` is 256 or 1024 *bits* in the released tunings. (Do not confuse this with the
BWTS preprocessing block, which is 5 MB *bytes* — see §2.4.) Encoder cost is dominated
by this optimiser; this is the source of the algorithm's deliberate asymmetry. Larger
`maxblk` improves ratio but makes the parse markedly slower, which is why the byte-level
frequency-table model Shelwien originally wanted was never shipped — it "would require
supporting much longer blocks, and parsing optimization is pretty slow already even with
maxblk=256" (Post #6).

### 2.3 Auxiliary submodels

- **Gray-code submodel** (v3). The optimiser may encode `bit[i] ⊕ bit[i−1]` instead of
  raw bits when it pays — a one-step decorrelator that helps when the input is a
  slowly-varying waveform whose LSBs flip predictably (raw audio after BWT).
- **Adaptive uncompressed-block flag** (v4), folded into the parser's cost evaluation.
  Earlier versions used static probabilities for flags; making them adaptive was one of
  Shelwien's stated future-work items (Post #6).
- **Three `freq0` ranges** (v2; up from two), giving the count prior some
  skew-dependent expressiveness (low / mid / high bias).
- A **permutation submodel** — each byte value appears at most once per block — was
  tried and dropped (Post #13). On a 256-symbol block it can save ~45.5 bytes, but
  CDM's optimiser only ever used such blocks as a replacement for *uncompressed* ones,
  and even then only 4–5 times per file. It also slowed encoding, so it was removed.

### 2.4 Why the BWT step matters

This is the most important practical fact about CDM, and it is easy to miss if you only
read the changelog. **Applied directly to bytes, CDM almost never beats `7z -mx9`.** In
every benchmark table the winning row (marked `*`) is overwhelmingly one of
`bit7-0 bitwise bwts` or `bit0-7 bitwise bwts`. In practice CDM is a
**bitwise-BWT post-coder**, not a standalone coder.

The mechanism is physical: bitwise BWTS clusters bits that share a high-order context,
producing long bit-runs with locally biased `freq0`. CDM's per-block count model then
has something real to encode. Without BWT, per-block counts sit at `n/2` everywhere, the
model has nothing to say, and the parser dumps almost everything into stored blocks.

Shelwien confirms this interpretation directly when discussing the surprising WAV
results (Post #13): "It's actually BWT compression. You can probably get better results
with normal BWT postcoders, rather than CDM."

The bit order determines which bits become "high-order context" inside the BWTS, and
this yields a genuinely useful side-effect: **format detection**. JPEG emits Huffman
codes MSB-first and compresses best under `bit7-0`; DEFLATE is byte-aligned LSB-first
and compresses best under `bit0-7` (Post #1). Whichever bit ordering produces the
smallest output reveals the source codec's bit traversal — "CDM can be used for file
analysis/format detection."

The bytewise/bitwise BWTS utilities (`bwts`, `bwtsh`, `bwtsl`) were shipped natively in
v5; the default BWTS block size is 5 MB.

### 2.5 A three-layer mental model

CDM is usefully viewed as three loosely coupled layers:

1. **Representation layer** — which view exposes regularity: raw bytes, `bit7-0`,
   `bit0-7`, gray-code, bytewise BWTS, or a hypothetical `bit[i] ⊕ bit[i−k]` transform.
2. **Block-statistics layer** — for each candidate block, the LUT estimate of whether
   the exact-count coder beats raw storage after metadata.
3. **Parsing/selection layer** — the forward DP that picks a cost-minimising
   segmentation.

Most of CDM's *intellectual content* is in layer 2; most of its *CPU cost* is in layer
3; most of its *empirical gain* is decided in layer 1. (The exact internal form of the
optimiser is inferred from described behaviour — Shelwien states a slow parsing
optimiser exists but does not fully specify it.)

### 2.6 Implementation evolution, v1 → v5

| Version | Verified changes |
|---|---|
| **v1** | Proof of concept. Static `maxblk`, ~1 GB price cache. Required separate tunings (`cdm`, `cdm1`, `cdm2`) for different input families because `p_maxblk` was static; Shelwien later made `p_maxblk` adaptive, which "doesn't matter that much anymore" (Post #13). |
| **v2** | Price cache removed → encoder memory ~1 GB → ~10 MB. EOF coding fix saves 1–2 bytes/file. `blklen ≠ maxblk` supported for uncompressed blocks. Three `freq0` ranges (up from two). Speed optimisations. LZ4 `-12` recommended as a precompressor for compressible inputs. |
| **v3** | Decoder ~40% faster (~8 MB/s). Gray-code submodel added to the optimiser (better but slower compression). |
| **v4** | Rangecoder bugfix for files < 4 bytes. Encoder ~3× faster than v3 (~0.4 MB/s), decoder ~2× faster (~14 MB/s). Adaptive coding of the uncompressed-block flag, integrated into the parser. |
| **v5** | Further encoder speedups (e.g. 19.2 s → 1.7 s on `zik.ogg.bwts7-0`, 2.65 MB). Re-tuned for compressible data (slightly worse on incompressible). `bwts/bwtsh/bwtsl` utilities shipped. |

The trajectory is clear: early work chased ratio, later work chased speed, memory, and
adaptivity, with the mathematical core unchanged throughout.

*Correction to two reviews:* the decoder figure is best stated as v3 ≈ 8 MB/s → v4/v5 ≈
14 MB/s. The "4 MB/s baseline" cited in some drafts is not supported by the posted
timings (v2 decoded the 2.65 MB file in 0.234 s ≈ 11 MB/s already; the relevant
verified speedups are the v3 +40% and v4 ×2 steps).

---

## 3. Empirical Behaviour

Across the v4/v5 tables, CDM's gain over `min(original, 7z -mx9)` falls into clear
regimes. All percentages are verified from the posted tables (Shelwien Posts #10–#14)
and Darek's testbed reports (Posts #7, #9, #12, #15).

| Input class | CDM gain (typical) | Comment |
|---|---|---|
| JPEG, AAC, MPEG-1, low-quality JPG | 3 – 8 % | `bit7-0` BWT path; Huffman/AC bitstreams with significant residual structure. `1.aac` reached 7.5 %, `skiing.avi` (MPEG-1) 5.1 %, low-quality JPG ~12 %. |
| OGG Vorbis, WMA, PCX, text PDF | 1.5 – 12 % | Mixed. Per-file outliers: WMA ~9 % (ratio 21 % vs 7z's 13 %), PCX ~5 %, PBM ~11.5 %, Reymont.pdf (Silesia) ~12 %. |
| H.264-in-AVI, ZIP/CAB/RAR | 0.01 – 0.1 % | Tight LZ + entropy stages leave little. `001.avi` (H.264): 0.01–0.04 %. |
| 7z (LZMA2), zpaq, RAR `-mct` (PPMd), CHM | 0.00 – 0.07 % | Well-modelled CM/LZMA output, essentially incompressible. |
| PAQ, CMIX, CMV, FLIF | ≤ 0 % | Shelwien: "wasn't able to compress any paq archives"; Darek: no gains after CMIX12/CMV; FLIF "completely incompressible" (≈ −1.4 %). |

Darek's testbed also produced useful comparisons absent from the gain table:

- Pure CDM **v2** was ~1.1 % better than `lz4 -12` over the whole testbed; running CDM
  on `lz4 -12` output added ~7 %; combining best-of-both was ~9.9 % better than lz4.
- Pure CDM **v3** was ~6.0 % better than `lz4 -12`; here the lz4-preprocessing path
  added only ~2.6 % (the stronger v3 coder made preprocessing matter less); combined
  ~10.7 % better than lz4.
- On raw audio, pure CDM was ~25 % better than lz4 on a WAV file, but still ~6–8 %
  *worse* than `7z -mx9` (this is the BWT side-effect, not a real win).

Average format gains over `7z -mx9` from Darek's larger survey (Post #15): AAC ~1.9 %,
MP3 ~1.7 %, JPG ~3.2 %, PNG ~4.3 %, TIFF (LZW) ~2.5 %, WAV/AIFF ~5–6 % *worse* than 7z.

Two patterns are worth pinning down:

1. **CDM gain is largely a function of how lazy the original encoder was about its
   entropy stage.** JPEG's static Huffman tables, AAC's fixed Huffman codebooks, and
   MPEG-1's pre-tabled VLCs all leave bit-level biases that surface after BWT. Modern
   context mixers leave none.
2. **The dramatic outliers are not entropy-stage redundancy.** WMA 9 %, PCX 5 %, PBM
   11.5 % are CDM's *stored blocks* finding near-uncompressed regions — BWT-postcoder
   territory, not CDM-specific cleverness.

A small but telling data point on modern LZ codecs (Post #13): after Oodle's Kraken,
CDM managed only 0.12 %; after LZNA, 0.00 %; after BitKnit, 0.02 %.

---

## 4. Strengths and Weaknesses

**Strengths.**

- **Always non-expansive in expectation.** The stored-block fallback plus the v2 EOF fix
  bound the worst case to a small constant overhead per file.
- **Decoder is genuinely fast** (~14 MB/s in v4/v5) and trivially block-parallelisable.
- **Format detection as a free side-effect** (the three-BWT-order trick).
- **Composes cleanly with LZ4 / LZMA2.** As long as the upstream compressor leaves
  *stored* blocks for incompressible runs, CDM operates on those runs. Shelwien notes
  (Post #16) that `lzma`/`plzma` do *not* work for this — only `lzma2` produces stored
  blocks for incompressible data — and that `srep` or `lz4` are alternatives, with lz4
  sometimes better because lzma2 blocks are large (~64 KB).

**Weaknesses.**

- **Encoder is slow** (sub-MB/s) and was memory-heavy before v2.
- **Model is iid-per-block.** It cannot exploit cross-block bit correlation — exactly
  the structure most upstream coders leave behind. The gray-code submodel is the only
  nod to bit dependence.
- **Useless on well-modelled streams.** PAQ, CMIX, modern CM output, FLIF — the bit
  stream is genuinely iid Bernoulli(½) at CDM's resolution.
- **Heavily dependent on a BWT pre-pass** in practice, despite the standalone framing.
- **No notion of byte boundaries or symbol structure.** Recovering a per-symbol model —
  Huffman tree, tANS table, AC parameters — is impossible for CDM by construction.

The last two points motivate §5 (specialised attacks) and §6 (generic missed
redundancies).

---

## 5. Better-than-CDM Solutions for Specific Source Codecs

CDM's refusal to assume anything about the upstream coder is its whole point — and the
reason it is dominated, often by large margins, whenever the upstream coder is
identifiable. Each case below gives a concrete attack and a realistic gain estimate over
CDM. Where the source reviews gave inflated figures, the estimate has been tightened and
the disagreement flagged.

### 5.1 Static Huffman with unknown code table

**Setup.** A concatenation of variable-length prefix codewords from an unknown canonical
Huffman code over an unknown alphabet — JPEG entropy segments, DEFLATE Huffman blocks
with the table region stripped, AAC scalefactor data.

**Why CDM under-performs.** A Huffman bitstream has weak per-block bias (Kraft-tight
codes are nearly entropy-balanced, so `freq0 ≈ n/2`) but very strong **bit-level
conditional structure**: the prefix property forces bit `i+1` to depend on the partial
codeword consumed so far. CDM models bits as iid given the block count and cannot see
this; worse, the BWT pre-pass it relies on actively *scrambles* the prefix structure.
CDM gain on such streams is typically ≤ 1–3 %.

**Attack — codebook recovery and transcoding.**

1. Estimate the code-length histogram. For each candidate `(l_1, …, l_k)` satisfying
   Kraft equality `Σ 2^{−l_i} = 1`, attempt to parse the stream. Most candidates fail to
   consume all bits or yield inconsistent counts. Heavy pruning: short codewords are
   frequent, so after a few KB the feasible set collapses to one or two trees.
2. Exploit canonical structure — codewords pack by length in lexicographic order, so the
   search is over depth histograms, not arbitrary trees.
3. Score by the order-1 entropy of the recovered symbols; the true tree minimises it.
4. Transcode to symbols, re-encode with an adaptive order-1 (or higher) arithmetic /
   rANS coder. If many files share a codebook, cache it and send only deltas.

**Expected gain over CDM:** ~5–25 % on Huffman-coded payloads, decomposing as Huffman
inefficiency vs. order-0 entropy (~0.05–0.5 bits/symbol, worse for skewed alphabets) plus
order-0-vs-order-`k` redundancy (~1–3 bits/symbol on text, 0.1–0.5 on audio
coefficients). CDM's ~3 % on JPEG is mostly the Huffman-table inefficiency it half-sees
through BWT; direct recovery roughly doubles that, and adding context modelling more than
triples it. *(One review quoted 15–40 %; that is optimistic for the table-recovery step
alone and is not used here.)*

**Cheaper fallback — bit-context mixing.** A PAQ-style mixer with order-4 to order-12 bit
contexts learns the prefix structure implicitly and typically beats CDM by 3–8 % at
modest cost, with no explicit tree recovery.

### 5.2 tANS / FSE output streams

**Setup.** A tANS bitstream: fractional-bit emissions tied to state transitions over a
finite-state table of size `L` (commonly 4096), usually with a header carrying normalised
frequencies.

**Why CDM under-performs.** Well-tuned tANS is within `~1/(L·ln2)` bits/symbol of Shannon
entropy — about 0.0004 bits/symbol at `L = 4096`. The output is, to high precision, iid
Bernoulli(½), and CDM finds essentially nothing.

But redundancy still sits in three places:

1. **The frequency table itself.** FSE encoders ship tables in tightly-but-not-optimally
   coded headers (zstd's FSE-coded FSE table is a small example) — a few hundred bytes per
   frame, often weakly compressed.
2. **Probability quantisation.** Frequencies are integers summing to `L`; the KL
   divergence between true `p` and quantised `p̂` is `O(1/L)` per symbol — detectable for
   skewed sources.
3. **Model mismatch.** A static table on a non-stationary or higher-order source pays the
   order-0-vs-order-`k` gap — usually the largest of the three.

**Attack — header-aware decoding plus recompression.** Detect the format (zstd, lzfse,
raw FSE have characteristic headers); recover the table from the header, or by
maximum-likelihood estimation over `L ∈ {2^k}` fitting frequencies that maximise the
probability of the observed bitstream under the FSE state machine; decode to symbols;
recompress with a higher-precision rANS or adaptive context model.

**Expected gain over CDM:** on well-tuned tANS over a stationary source, both CDM and any
attack save < 1 % — neither wins. On a static table over a higher-order source, 5–15 % is
realistic. The cheapest path is precomp-style: if the format is recognisable, decompress
and recompress with a stronger codec — exactly what Shelwien advocates in Post #16.

*Flag — unrealistic claim in one review.* One draft proposed using a deep LSTM / cmix-style
network to "learn the tANS state-transition table" and beat the combinatorial model. For
a *correctly tuned* static tANS table this is not achievable: the emitted stream is
information-theoretically near-iid, and no predictor — neural or otherwise — recovers
meaningful structure from it. The only realistic gains come from (1) table/header
inefficiency, (2) quantisation, or (3) genuine model mismatch, all of which are far more
cheaply attacked by table recovery than by black-box learning. The neural approach is only
plausible against a *mismatched or deliberately handicapped* tANS encoder, where the same
gains are available far more cheaply.

### 5.3 Static arithmetic coding with unknown model

**Setup.** Output of a rangecoder/AC using a fixed (possibly multi-order) probability
table; the table is unknown.

**Why CDM under-performs.** AC output, like tANS, is near-uniform iid. CDM's count model
finds at best a few bits per block from probability quantisation and renormalisation
overhead. Typical CDM gain: 0.05–0.5 %.

**Where redundancy actually sits.**

1. **Renormalisation byte-alignment.** 32-bit rangecoders output bytes; the remaining
   state at EOF is padded.
2. **E3 underflow handling** in classical Witten–Neal–Cleary AC injects bit-stuffing
   patterns visible at fixed bit positions.
3. **Static order-0 model on an order-`k` source** — the encoder pays `H_0 − H_k` per
   symbol.
4. **Plain model mismatch** — `D_KL(p ‖ q)` per symbol when static model `q` differs from
   empirical `p`.

**Attack — model recovery.** Estimate the static probability vector `q̂` by fitting:
choose `q̂` so that running the AC decoder under it produces a byte stream with sensible
statistics (flat order-0 entropy on recovered text, or a known prior). Search the space
with simulated annealing or a few hundred candidate distributions — each test is cheap
because decoding under a hypothesis is just rangecoder arithmetic. Select on minimum
`gzip(decoded)` size or minimum order-1 entropy. Recompress with an adaptive
context-mixing coder. A variant (one review) runs a black-box CMA-ES loop that drives the
*output length* toward the original file size and stores any decode errors as a small
correction stream — viable when the model is parametric but not exactly recoverable.

**Expected gain over CDM:** a textbook order-0 AC on natural-language text leaves a
30–50 % gap a PAQ-level recompressor will close; an order-2 AC on the same data leaves
5–10 %. CDM captures essentially zero of this in either case.

**Cheaper fallback — bit-context mixing.** Without recovering the model, a PAQ-style bit
mixer with order-4 to order-16 contexts picks up most of the order-mismatch redundancy and
beats CDM by 2–10 % on typical static-AC output.

### 5.4 Repeated-key XOR ("encryption")

**Setup.** Plaintext `P` of length `N` XORed with a key `K` of period `L`:
`C[i] = P[i] ⊕ K[i mod L]`. We want to compress `C`.

**Why this is a bad case for CDM specifically.** CDM is byte- and bit-agnostic across
positions. Repeated-XOR is a transformation *exactly* destroyed by ignoring the
position-mod-`L` index. A coder that knows `L` reverts to plaintext compression; CDM does
not, and does not try. Per-block counts can be slightly biased so CDM extracts *something*,
but the unseen structure is overwhelmingly larger. (Shelwien's unimplemented
`bit[i] ⊕ bit[i−k]` idea from Post #6 gestures at this, but it is not in any release.)

**Attack — full key recovery, then any compressor** (textbook Vigenère-on-bytes):

1. **Determine the period `L`.** Autocorrelation `A(d) = #{ i : C[i] = C[i+d] }` peaks at
   multiples of `L`; equivalently, the index of coincidence on every-`d`-th byte matches
   the plaintext IC at `d = L` and uniform IC ≈ `1/256` otherwise; Kasiski on repeated
   trigrams is a complementary, text-robust signal.
2. **Recover each key byte independently.** For residue class `j`, the substream
   `S_j = C[j], C[j+L], …` is plaintext XORed with the constant `K[j]`. Try all 256
   candidates; score by chi-squared distance of `S_j ⊕ k` to a known plaintext
   distribution, or by its compressibility under a small order-0 model.
3. **Decrypt and compress** with whatever standard compressor matches the plaintext.

**Cost.** `O(N·L)` for autocorrelation, `O(256·N)` for key recovery; fast and
deterministic for `N ≫ L`.

**Edge cases.** `L = 1` is trivial (256 trials). `L` comparable to `N` gives insufficient
statistics and degrades gracefully. If the *plaintext is itself already compressed*, its IC
is near-uniform and byte-level period detection fails — use bit-level autocorrelation on
each bit-plane, or trial-decrypt and look for a known compressed format's magic bytes.
Binary-executable plaintext still scores well on byte frequency (dominated by `0x00`,
`0xFF`, `0x90`, opcode prefixes).

**Expected gain over CDM:** effectively unbounded; 50–95 % on compressible text/code,
versus CDM's sub-1 %. *(One review wrote "infinite"; the honest statement is "bounded only
by the plaintext's own compressibility.")* This is the most extreme illustration of CDM's
limit: a transformation that looks like noise to a model-free post-coder is trivially
invertible to a model-aware one.

### 5.5 Summary table

| Source | CDM gain (typical) | Targeted-attack gain | What CDM misses |
|---|---|---|---|
| Static Huffman, unknown table | 0.5 – 3 % | 5 – 25 % | Prefix-property bit dependence; codebook structure |
| tANS/FSE, well-tuned, stationary | < 0.1 % | < 1 % | Both small; neither wins |
| tANS/FSE, mismatched / higher-order | 0.1 – 0.5 % | 5 – 15 % | Table inefficiency; order-`k` structure |
| Static AC, mismatched model | 0.05 – 0.5 % | 5 – 50 % | Model parameters; source order |
| Repeated-key XOR over plaintext | < 1 % | 50 – 95 % | Period structure; positional dependence |

The pattern is consistent: CDM extracts the redundancy visible as block-count bias after a
generic pre-transform. Anything left as **structured cross-position dependence** — Huffman
prefix structure, tANS state machine, AC model mismatch, XOR period — is invisible to its
iid-per-block model and recoverable by a model-aware attack at modest cost.

---

## 6. Improvements: Missed Redundancies to Look For

The §5 attacks all assume an *identifiable upstream codec*. This section is different: it
asks what residual redundancy a generic CDM-*like* post-coder leaves unclaimed even when
the upstream coder is unknown — i.e. how CDM could be strengthened without abandoning its
model-free, always-non-expansive philosophy. Each item names the redundancy, explains why
the current design misses it, gives a concrete extension, and estimates payoff and cost.

### 6.1 Redundancy inside CDM's own side information

**6.1.1 The `freq0` sequence is itself compressible.** CDM codes `freq0` per block with a
static 3-range prior. But after BWTS, adjacent blocks share context, so the *sequence*
`freq0[0], freq0[1], …` is strongly autocorrelated. Coding `freq0[i]` conditioned on
`freq0[i−1]` (a delta or a small adaptive order-1 model, or even a second range coder over
the residual `freq0[i] − freq0[i−1]`) recovers redundancy CDM currently pays in full on
every block. *Payoff:* on long BWTS runs this is a recurring few-bits-per-block saving that
compounds; plausibly 0.1–0.5 % on the formats where CDM already wins. *Cost:* one extra
adaptive counter; negligible decode cost. This is the single cheapest win available and
fits CDM's existing structure exactly.

**6.1.2 The block partition carries information.** Where the parser places boundaries is
itself a signal — if boundaries correlate with upstream record/frame sizes (common in
container formats), the partition is predictable. Encoding boundary positions with a model
conditioned on recent gaps (rather than a flat block-length code) reclaims metadata. *Cost:*
small; *payoff:* modest but real on structured containers.

**6.1.3 Block-flag run structure.** The stored/coded flag stream is bursty (coded blocks
cluster inside compressible regions, stored blocks inside incompressible ones). v4 made the
flag adaptive, but an order-1 model on the flag (or RLE over flag runs) captures the
burstiness the order-0 adaptive counter does not.

### 6.2 Within-block structure beyond population count

CDM uses only the 0/1 count `k`. At fixed `k` it treats all `C(n,k)` arrangements as
equiprobable — but they often are not.

**6.2.1 Transition counts (order-1 within block).** Replace the single count `k` with the
four di-bit counts `n00, n01, n10, n11` (equivalently, `k` plus the number of `01`/`10`
transitions). The conditional enumerative code over fixed transition counts is a
Whittle/Goulden-Jackson-style construction and is exact. This captures within-block run
structure — clustering or alternation of bits — that survives BWTS as locally correlated
runs. *Payoff:* this is the natural next moment after population count and is where most of
the unclaimed within-block redundancy lives on BWTS output; potentially the largest single
gain in this section. *Cost:* side information grows from one count to roughly two
(`k` + transition count), and the optimiser's per-block estimate needs a 2-D LUT, but the
DP structure is unchanged.

**6.2.2 Sparse-block gap structure.** When `freq0` is far from `n/2` (very sparse blocks),
the rare-bit positions often cluster rather than spreading uniformly. A secondary model on
inter-event gap lengths (a geometric/Golomb model on gaps) beats the flat enumerative code
in exactly this regime. *Cost:* fires only on sparse blocks, so cheap on average.

**6.2.3 Revisit the permutation model with a faster optimiser.** The dropped
each-byte-once submodel saved ~45.5 bytes on a qualifying 256-byte block but fired only
4–5×/file because the slow optimiser couldn't afford to look for it. With a cheap
pre-screen (a quick distinct-byte-count check before invoking the full price computation),
it could be re-enabled at near-zero amortised cost. More generally, a *low-cardinality
block* model (block uses only `m ≪ 256` distinct byte values) generalises this and is
common in structured binary data.

### 6.3 Cross-block and positional structure

**6.3.1 Carry the model across block boundaries.** The iid-per-block reset is the central
blind spot. BWTS runs routinely cross boundaries, and the first bits of a block are
predictable from the tail of the previous one. Letting the coded path *prime* its initial
probability from the previous block's trailing bits (or allowing the optimiser to choose
boundaries that respect run edges) captures continuity the current reset throws away.
*Cost:* a small running context; *payoff:* directly attacks the weakness §4 identifies as
most damaging.

**6.3.2 Position-mod-`p` bias scan.** CDM is entirely position-agnostic, so it misses every
periodic structure: XOR keys (§5.4), fixed-size records, interleaved audio channels,
pixel-stride patterns in raw images. A cheap pre-pass — autocorrelation or per-stride
entropy over small candidate periods `p` — can detect periodicity and apply a stride
de-interleave before the existing pipeline, with the chosen `p` (or "none") coded in a few
bits. This folds the most valuable part of the §5.4 attack into the generic coder without
requiring the upstream codec to be identified. *Cost:* one `O(N · p_max)` scan; *payoff:*
turns several near-zero cases into large wins.

**6.3.3 Bit-position-mod-8/32 model for alignment artifacts.** Byte-aligned upstream coders
leave renormalisation/padding patterns at fixed bit offsets (the §5.3 E3 and EOF artifacts,
generically). A submodel keyed on `bitindex mod 8` (and optionally `mod 32`) captures these
without any codec knowledge — it is the position-agnostic blind spot applied to small fixed
periods. Cheap and always safe to try.

### 6.4 Representation-layer gains

**6.4.1 Generalise gray-code to `bit[i] ⊕ bit[i−k]` with a searched `k`.** The shipped
gray-code submodel is the `k = 1` special case. Shelwien listed the general transform as
future work (Post #6) but never shipped it. A quick autocorrelation pass to pick the best
`k` (or a small set) targets periodic LSB structure in waveform and image data that `k = 1`
misses. *Cost:* the search; *payoff:* concentrated on raw-media-after-BWT, where CDM
already does its BWT-postcoder trick.

**6.4.2 Mix representations instead of hard-selecting one.** CDM currently picks the single
best of {direct, byte-BWTS, bit7-0, bit0-7}. These views carry partially independent
predictive information; a logistic mixer over their per-bit predictions (PAQ-style) would
beat hard selection. This crosses from "model-free post-coder" toward "context mixer," so it
is the most expensive item here and the least in keeping with CDM's philosophy — but it is
also where the ceiling is highest. *Decode cost:* substantial (loses the fast-decoder
property), so this is a different design point rather than a drop-in.

**6.4.3 The byte-value bitmask / frequency-table model Shelwien wanted.** His original goal
was a byte model that compresses a block's byte-value frequency table (Post #6), shelved
because it needs much longer blocks and the optimiser is already slow. The fix is not a
faster full optimiser but **conditional block expansion**: keep `maxblk` small by default
and merge into a long byte-model block only when a cheap estimate says the per-extra-metadata
-bit gain is positive. This makes the long-block model affordable exactly where it pays.

### 6.5 Where *not* to look

For completeness, two regimes offer essentially nothing and should be skipped to save
encoder time: (a) output of modern context mixers (PAQ/CMIX/zpaq) and FLIF — verified
incompressible by CDM and by every attack in §5; and (b) well-tuned tANS/rANS over a
stationary source (§5.2) — near-iid by construction. A practical encoder should detect these
(near-`n/2` counts everywhere across all four representations, no autocorrelation peaks) and
fall straight to stored-block passthrough rather than running the full parse.

---

## 7. Practical Recommendations

For a stream of unknown provenance where beating CDM is the goal:

1. **Identify the upstream coder first.** Magic numbers, header probes, and short-trial
   decompression with common codec libraries handle most real-world cases. CDM's
   three-BWT-order trick is a useful zeroth pass when no header is present. Shelwien himself
   points at `precomp` and dedicated deflate/JPEG recompressors for embedded streams
   (Post #16) — these beat CDM decisively and should run first.
2. **Huffman-bearing streams:** attempt canonical-codebook recovery; fall back to a
   bit-context mixer if recovery is too slow.
3. **tANS/FSE/AC streams:** if the header is recognisable, decode and recompress with a
   stronger model (precomp-style); if unknown, fit the model with a small simulated-annealing
   search.
4. **Anything with uniform-ish bytes but suspicious autocorrelation peaks:** run period
   detection; if `L` is found, recover the key and compress the plaintext.
5. **Keep CDM as the terminal stage** for residual incompressible-looking parts — its
   always-non-expansive guarantee makes it a reasonable last coder when nothing else applies.

The natural architecture is therefore:

```
detect  →  invert/decode the upstream coder  →  recompress with a stronger model  →  CDM on the residual
```

CDM does what it does best — squeezing the last few bits — while the specialists upstream
take the larger gains. And within CDM itself, §6 shows the model-free philosophy still has
unclaimed redundancy to chase: the `freq0` sequence, within-block transition counts, and a
cheap positional/periodicity scan are the three highest-value, lowest-risk extensions, none
of which sacrifices the fast-decoder property that makes CDM worth using in the first place.

---

## Appendix: Verification Notes

The following claims were checked against the primary thread and corrected where reviews
diverged:

- **`33.6576`-bit example** (`n=1024, k=412`): exact, Post #1.
- **`freq1 = blklen − freq0`, decrementing; adaptive variant costs `log₂(n+1)` more**:
  exact, Post #6.
- **LUT identity `log₂C(c0+c1,c0) = LUT[c0+c1] − LUT[c0] − LUT[c1]`**: exact, Post #6.
- **Speeds**: encoder ~0.4 MB/s and decoder ~14 MB/s in v4 (Post #11); v3 decoder ~8 MB/s
  (Post #8); v5 encode of the 2.65 MB file in 1.685 s (Post #14). The "4 MB/s decoder
  baseline" in two drafts is unsupported and was dropped.
- **Memory 1 GB → 10 MB at v2**: exact, Post #4.
- **`maxblk` (256/1024 bits) is distinct from the 5 MB BWTS block**: one review conflated
  them; corrected.
- **Permutation model: ~45.5 bytes on a 256-block, fired 4–5×/file, dropped**: exact,
  Post #13.
- **`p_maxblk` made adaptive, obviating `cdm`/`cdm1`/`cdm2` tunings**: Post #13.
- **PAQ/CMIX/CMV/FLIF incompressible by CDM**: Posts #1, #12, #15.
- **Per-format gains (AAC 1.9 %, MP3 1.7 %, JPG 3.2 %, PNG 4.3 %, WMA 9 %, PCX 5 %, PBM
  11.5 %, Reymont.pdf 12 %)**: Post #15. **lz4 comparisons**: Posts #7, #9.
- **Overstatements corrected:** Huffman "15–40 %" → 5–25 %; XOR "infinite" → 50–95 %
  (bounded by plaintext compressibility); the "LSTM learns a well-tuned tANS table" claim
  flagged as not information-theoretically achievable.
