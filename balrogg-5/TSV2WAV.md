# tsv2wav: algorithm analysis and source review

`tsv2wav` turns a balrogg record stream into 16-bit PCM and back:

```
tsv2wav c input.tsv output.wav output.meta
tsv2wav d output.wav restored.tsv output.meta
```

Mode `c` is a Vorbis decoder that matches libvorbis to within 1 LSB, plus a
meta stream carrying what the PCM cannot give back. Mode `d` recovers the
record stream exactly. On the 26-file corpus every round trip is bit-exact and
each restored TSV rebuilds its original Ogg through `balrogg d`.

The design follows `mp3synth`: run the synthesis backwards as far as it will
invert, snap the result to the integers the stream must have contained, and
carry the residue of that process — the cases where snapping is wrong or
impossible — verbatim in a keep-list. Reversibility is guaranteed by the
keep-list; everything else in the design is about keeping it small.

This document describes the algorithm as implemented, then reviews the code.
Numbers come from the corpus in `testfiles/` and `gen/`.

---

## 1. Structure

Single translation unit, one class per `.inc`, no allocation.

| file | lines | what |
|---|---:|---|
| `tsv2wav.cpp` | 304 | the page walk, one traversal with three roles |
| `lift.inc` | 207 | fixed point, lifting rotations, bit-exact DCT-IV |
| `vd_setup.inc` | 269 | codebooks, floors, residues, mappings, modes |
| `vd_dec.inc` | 345 | synthesis: floor, residue, coupling, transform, lap |
| `vd_ana.inc` | 169 | analysis: window, MDCT, un-floor, un-couple |
| `vd_rec.inc` | 256 | the residue walk backwards; digits, classes, keeps |
| `imdct.inc` | 124 | the Vorbis window (transform now lives in `lift.inc`) |
| `wav.inc` | 100 | RIFF sink, channel reorder |
| `floor_db.inc` | 67 | `floor1_inverse_dB_table` |

Static footprint 90 MiB; peak RSS 11 MiB on a 3-minute file.

### 1.1 One traversal, three roles

The record stream has one order, fixed by balrogg's `io` traversal. Rather than
three readers, `walk()` runs one traversal under a role:

- `ROLE_SYNTH` — read `input.tsv`, synthesize, write the WAV.
- `ROLE_META` — read `input.tsv` *and* the WAV, run the recovery beside the
  truth, write `output.meta` with corrections.
- `ROLE_REST` — read `output.meta` and the WAV, run the recovery, write
  `restored.tsv`.

Mode `c` is `SYNTH` then `META` (two passes over the input). Mode `d` is
`REST`.

Structural records — page framing, headers, the whole setup — pass through a
*tee* on the source `tsv`: every record `get()` reads is also `put()` to the
destination. Only the audio packet needs role-specific handling, and it
disables the tee while it runs.

### 1.2 The meta is a TSV

Same reader, same tags, greppable. Per link: `link.more 1`, then
`link.frames` (output frames, needed for the coverage test in §4.4), then the
page and packet records. Per audio packet:

```
aud.mode
aud.wnext            only for long blocks
flr.used  ×ch
flr.y     ×posts     only where used
kc.n  {kc.i kc.v}    class corrections: index within the packet, true value
kd.n  {kd.i kd.v}    digit keeps
```

Absent, because derived: `res.digit` (recovered), `res.class` (predicted),
`aud.wprev` (the previous block's flag). The corrections come *after* the
floor records so that `META` can stream — it reads the true classes and digits
past the recovery, accumulates disagreements, and writes the keep blocks at
the end. `REST` emits the floor records as it reads them, reads the keep
blocks, then computes and emits the residue records. Neither role buffers a
packet.

---

## 2. Synthesis (`ROLE_SYNTH`)

Standard Vorbis synthesis; see `SYNTHESIS.md` for the math. Two things are
specific to this implementation.

**The transform is integer lifting.** The spectrum is converted to i64 fixed
point (F = 32) and passed through `lifted_dct4::backward`: a DCT-IV from one
M/2-point complex FFT, every twiddle a three-shear Givens rotation, butterflies
exact. The DCT-IV inverts bit-exactly (0 ulp at every M from 64 to 4096) and
its float accuracy is 3.5e-8 at M = 4096, about 0.001 PCM LSB.

The rotations are *quadrant-reduced*. `Ct = tan(θ/2)` reaches 652 for the
outer FFT roots at N = 2048, and each shear multiplies its own rounding by
that. Reducing every angle into [−π/4, π/4] by whole quarter turns (exact
signed swaps) bounds `Ct` at 0.414 and cut the error 5300×. Round-trip
exactness never depended on this; fidelity to libvorbis did.

**Only the transform is lifting.** Floor rendering, residue accumulation and
coupling run in float, as in libvorbis. This departs from `mp3synth`, which
carries the whole spectral chain in fixed point. It does not affect
reversibility — the keep-list closes any gap — and the float path is what
lets the output match libvorbis to 1 LSB. See §6.1.

### 2.1 Where the sample lands, which the tool chooses

The last step, `round(y × 32767)`, is the one genuinely lossy step in the
chain, and it is the tool's own: the WAV is an output, not an input, and mode
`d` reads back whatever was written. So nothing about how a sample is placed
between two integers has to be carried, and the placement is free to be
chosen to suit the recovery rather than the ear.

Three ways of choosing were measured over 26 to 37 files.

| | effect on the meta |
|---|---|
| the lifting's rounding rule (nearest, floor, ceiling, truncate) | under 0.03%; a non-lever |
| the split point (floor, nearest, ceiling, quarter steps) | nearest is best; floor or ceiling ≈ 1%, quarter steps 0.4–0.7% |
| first-order error feedback | **−2.1% chosen per file, up to −11.5% on one** |

The third is the one that pays. Quantizing sample *n* with a fraction *h* of
sample *n−1*'s error subtracted shapes the error spectrum by `1 − h/z`,
draining it away from the low frequencies where most of the digits are. Its
value is strongly file-dependent and not one-signed: `h = 0.6` everywhere is
6.3% *worse* on the 17-file corpus even though it is 11.5% better on one of
its files, so `wav::frame`'s coefficient is swept per file in mode `c` and
the winner kept. Second-order shaping adds nothing measurable.

The candidates are scored with the floor refit switched off, which makes a
scoring pass twenty-five times cheaper — the refit is 97% of mode `c`'s
runtime — and was checked over 37 files to pick the same coefficient as
scoring the whole meta. Four candidates cost about 25% more runtime.

The product is formed in double, not float: it is around 32767, so a float
product would carry only nine bits below the point, and the bits below the
point are the whole of the decision being made. This is correctness rather
than a gain — measured against the float product it is worth 0.00% once the
sweep is running, though it was worth 3.3% on one file before the sweep
existed.

The ceiling on all of this is known. Instrumenting the digit walk on
`ff_48000_q10`: 89.0% of digits are recovered exactly, 1.7% are wrong but
within one step of right, and 9.3% are wrong by a whole step or more — and
37% of the wrong ones are in blocks the PCM does not span at all. Only that
1.7% is close enough to the boundary for a better sample to flip, which is
why the numbers above are single-digit percentages and not more.

---

## 3. What is recoverable, measured

Before building the recovery I measured what the PCM could give back. This
decided the meta contents.

| stream | of records | of bytes | disposition |
|---|---:|---:|---|
| `res.digit` | 91.8 % | 86.8 % | recovered from PCM |
| `flr.y` | 4.5 % | 4.3 % | **stays**: recovery divides by it |
| `res.class` | 3.1 % | 6.6 % | predicted, corrections stored |
| `aud.wprev/wnext` | 0.14 % | 0.67 % | `wprev` derived; `wnext` stays |
| `flr.used`, `aud.mode` | 0.21 % | 0.92 % | stay |
| framing, headers, setup | 0.30 % | 0.78 % | stay |

**Digits through 16-bit PCM.** Matched analysis of the s16 output against the
true residue: median error 1.2e-2, p99 0.19, and 0.29 % of bins beyond the
0.5 snap threshold on the pessimistic Python probe; the real code, which
skips partially covered blocks, measures 0.000–0.080 % across the corpus.

**Window flags.** Every long block in six files has `aud.wprev` equal to the
previous block's blockflag — zero exceptions. `aud.wnext` equals the next
block's flag with exactly one exception per file, the final block. So `wprev`
is derived and the tool asserts the assumption; `wnext` is carried, since
deriving it needs one-packet lookahead for a 0.07 % saving.

**Re-encoding.** Asked whether libvorbis would just re-fit the same floor
from the decoded PCM, I tested it. With the *same* encoder and setting the
structure reproduces remarkably: setup bit-identical, mode sequence 100 %,
`flr.used` and window flags 100 %, `res.class` 95.7 %. But `flr.y` — the one
stream it would have been worth eliminating — is the worst predicted: only
34 % of nonzero posts agree, median |diff| 9. And none of the real corpus can
be re-encoded at all; those files came from a different encoder with a
different codebook set. Rejected as a load-bearing mechanism.

---

## 4. Recovery (`ROLE_META`, `ROLE_REST`)

The synthesis chain for one channel is

```
digits → residue → couple → × floor → IMDCT → window → overlap-add → round to s16
```

Recovery inverts it right to left. Steps that are exactly invertible are
inverted; the two that are not are where the keep-list earns its place.

### 4.1 Spectrum: the matched MDCT

Overlap-add with a Princen-Bradley window is perfectly reconstructing: the
same window followed by the matched MDCT returns the spectrum exactly, with
`X = (2/M) · MDCT(w · segment)`. Verified numerically to 5e-14, and the MDCT
is a DCT-IV read through the TDAC fold (`vd_ana::spectrum`), so it is the
*same* lifting transform as the synthesis.

Block `b`'s span in output time is `[T_b − M_b, T_b + M_b)` with `T_0 = 0`
and `T_b = T_{b−1} + (M_{b−1} + M_b)/2`. The window that the synthesis
effectively applied is rebuilt from `(lW, W, nW)` — `lW` from the tracked
previous flag, `nW` from `aud.wnext`.

### 4.2 Floor: divide

`r[k] = X[k] / floor[k]`. This is one of the two lossy steps: where the floor
is small the s16 rounding noise is amplified. The floor spans seven decades,
so the division is done in double and only the quotient carried on as fixed
point (§6.2).

### 4.3 Coupling: exact inverse, with a subtlety

The decoder's coupling is four sign-conditional cases. The encoder keeps
`|ang| ≤ |mag|`, which puts the four outputs in disjoint quadrants where `M`
and `A` share a sign, so the inverse is exact.

The first implementation branched on `sign(M)` and was wrong 8.7 % of the
time on stereo. Case two has `M = mag + ang`, which sits at zero whenever the
channels nearly cancel — the ordinary situation — so noise flipped the branch
and the angle came back negated. The error histogram gave it away: 37.5 % of
wrong digits off by exactly 2, 21.5 % by 4, odd values suppressed;
`|got − want| = 2|want|` is a sign flip. `M + A` keeps the sign of `mag` in
all four cases and never sits near zero unless both do. Digit keeps on
`00000007`: 8.69 % → 1.68 %.

### 4.4 Residue: the ladder walked backwards

`vd_rec::residue` has the shape of the decoder's residue loop. Where the
decoder adds a digit's worth of signal, `take()` subtracts it:

```
q  = round( (v − minval) / delta )
d  = clamp( q − off, [−off, nv−1−off] )
v -= (d + off)·delta + minval
```

Passes go coarsest first, so each digit is whatever the *remaining* value
rounds to. That reproduces libvorbis, whose `_encodepart` also quantises
nearest-first per pass. The multiplicand tables are permuted
(`(3, 2, 4, 1, 5, 0, 6)` is typical — centre first, then alternating) but
their *set* is always `0..nv−1`, so the digit range is contiguous and only
`off` matters.

Placement mirrors `vd_dec::partition`: type 0 striped
(`symbol j, digit k → pos + k·step + j`), type 1 sequential, type 2
round-robin across the bundle. Type 1 digits placed past `psz` are emitted
by the decoder but never used; they are unrecoverable and always keep-listed.

### 4.5 Reconciliation

Both roles must subtract the same value, or they drift apart. `digit()`:

```
got = take(v, book)                        both roles
META:  want = read truth; keep (i, want) if ≠ got; fin = want
REST:  fin = got, overridden by a keep at this index
if fin ≠ got:  v -= (fin − got)·delta       put back the walk's guess, remove the truth
```

`klass()` does the same with the class prediction. Because `META` reconciles
against the truth at every step and `REST` against the corrections at the
same steps, the two are in lock-step by construction. This is what makes the
whole thing exact rather than approximately right.

**How the corrections are written.** The list is `(index, value)` pairs, and
there are three ways to put it on the page. The packet's `kn` count says
which, in its sign and its low bit, and mode `c` prices all three and takes
the cheapest:

| form | `kn` | what goes out |
|---|---|---|
| sparse | `n` | the gap to each correction on `kd.i`, then the values on `kd.v` |
| raw | `−2t` | every one of the packet's `t` digits on `kd.v`, zeros run-coded |
| marked | `−(2t+1)` | the same `t` positions on `kd.v`: a digit the walk got right is a zero and joins a run, a digit it got wrong carries `zig(v)+1` |

The same three forms carry the class corrections on `kc.i` and `kc.v`, with
one difference: a digit is signed and goes out zigzagged so that its sign
never costs a byte of its own and never looks like a run marker, but a class
is an index into a ladder and cannot be negative, so zigzagging one would
double it for nothing — class five would cost the two digits of ten. The
classes skip it, which is worth 6.2% of `kc` on `00000003`.

The marked form is the sparse list with the indices left out — what separates
two corrections is a run marker rather than a gap, and **two corrections side
by side need nothing between them at all.** Corrections cluster, so that is
where it wins: on `00000007` it takes the correction stream down 8.2%. It is
decoded straight back into the `(index, value)` list, so nothing downstream
of `read_keeps` knows the difference.

Riding the form in the count's low bit rather than in a record of its own is
worth 1.7 of those 8.2 points: doubling a number costs a digit only when it
crosses a power of ten, where a record of its own costs two bytes every time.

Two things that sound right and are not. Storing `want − got` instead of
`want` is **worse** (+4.1%): a correction's true digit is usually zero, and
the difference from a wrong guess is not. And using the marked form alone,
dropping `kd.i` entirely, is only 0.04% behind keeping all three — but it
regresses badly on packets where the walk is useless, so all three stay.

### 4.5.1 A model over the walk's own mistakes

The digits cannot be predicted from each other. The floor has already taken
the spectral envelope out, and what remains has an autocorrelation of **+0.009
at lag one** — a linear predictor over past digits finds nothing, and that is
the floor doing its job rather than a shortcoming.

The walk's *error* is another matter:

| | lag 1 | lag 2 | lag 3 | lag 8 |
|---|---:|---:|---:|---:|
| `want` | +0.009 | +0.011 | −0.022 | +0.058 |
| `want − got` | **−0.418** | +0.320 | −0.243 | +0.196 |

and where the walk has just been wrong it is wrong again **74%** of the time
against 11% overall. Errors arrive in bursts, with an alternating sign.

**LPC on that does not work, and it was tried.** The error is zero at nine
digits in ten, so a predictor that rounds a continuous estimate breaks far
more right answers than it mends: order 1 cost +14.5% more corrections, order
8 cost +40.1%. The autocorrelation is real but it is carried entirely by the
one digit in ten that is wrong.

What does work is a **conditional mode**: the commonest error seen before in
the same context, the context being the two previous errors clipped to ±3,
with zero given half its own count again as a margin so a rival has to be
clearly ahead. Nothing is carried — both roles see the same errors in the
same order, so both update the same counters — except one flag per stream,
which the fit pass sets by running the model in shadow over every digit.

It wins by a lot or not at all:

| | walk right | with the model | corrections |
|---|---:|---:|---:|
| `ff_8000_q5` | 68.7% | 77.8% | **−29.2%** |
| `ff_48000_q10` | 89.2% | 90.9% | **−15.8%** |
| `00000007` | 98.4% | 98.4% | +0.7% |
| `00000003` | 99.9% | 99.9% | +1.4% |

so the flag is what makes it safe. On the 22 ffmpeg files every one turns it
on, for −2.21% of the meta and −18.0% of the corrections on `ff_48000_q10`;
on all 17 corpus files every one turns it off, and pays the eight bytes the
flag's row costs. The byte saving is smaller than the correction saving
because a correction that goes away leaves a zero, and a lone zero does not
merge into a run.

### 4.6 Class prediction

The class is the encoder's choice of cascade ladder, and libvorbis chooses it
from the partition's magnitude. So the predictor computes each class's
*reach* — the sum over its passes of the largest digit each book holds, in
signal units — and takes the smallest reach that covers the partition's
peak. The first version scored candidates by Σ|digits|, which prefers
whatever ladder yields small digits: the opposite of the encoder. Class
corrections on `00000000`: 28.4 % → 0.45 %.

Classes that share a ladder are interchangeable and cannot be predicted;
`00000007` has 43.7 % of its class records in such a pair, and that file's
class corrections stay at 53 %.

### 4.7 Coverage

The first block of a link spans `[−M_0, M_0)` — half of it before the PCM
starts. The last block's span runs past the granule trim. Neither can be
analysed. `recover()` zeroes the residue for such blocks so both roles
compute identical (useless) digits, and every digit is keep-listed. Two
blocks per link.

---

## 5. Where the keeps come from

After the fixes above, keep sources on the corpus:

| source | nature | example |
|---|---|---|
| s16 rounding under a deep floor | inherent; the lossy step | 0.0–0.08 % of digits |
| `|ang| > |mag|` coupled bins | inherent; coupling not injective there | `00000007`: 4.18 % of coupled bins |
| coupled pair with one silent channel | inherent; decoder discards the pair's angle | `ch6`: step (0,1), ch0 never coded |
| interchangeable classes | inherent; same ladder | `00000007`: 43.7 % of classes |
| first/last block per link | inherent; no PCM | 2 blocks per link |
| type-1 digits past `psz` | inherent; decoder never uses them | rare |

The second and third rows were found by review, not by design. Both were
confirmed empirically: I tested the four-case coupling for injectivity
outside `|ang| ≤ |mag|` (42 of 72 pairs collide) and counted violations in
the synthesis (4.18 % of `00000007`'s coupled bins, 0 % everywhere else);
and I checked `ch6`'s mapping (channel 0 is never given a floor yet is coupled
to channel 1, so the decoder computes the pair and then zeroes one side —
two unknowns from one observation).

All six rows are things the decoder throws away. That is exactly the
category the keep-list exists for, and it is the same category `mp3synth`
names ("deep-gain, non-injective dequant, terminal boundary"). There is no
known keep that comes from the recovery being *wrong*.

### 5.1 Meta size

Corpus total: **19.4 % of the TSV**. The real testfiles sit at 8.7–12.6 %.
The outliers are short synthetic files where the setup header is most of the
stream (`tiny` 117 %, `lo8k` 68 %) and the two files above with inherent
coupling losses (`00000007` 27 %, `ch6` 77 %).

Of the ~5 % I projected from the record shares, the shortfall is entirely the
coupling rows. On a file with `|ang| ≤ |mag|` throughout and all channels
coded, the meta is floor posts plus setup plus a few hundred keeps.

---

## 6. Review

Read with the code in front of me. Ordered by consequence.

### 6.1 The synthesis spectral path is float

`vd_dec` renders the floor in float, accumulates residue in float and couples
in float; only the transform is fixed-point lifting. `mp3synth` keeps the
whole chain in i64. This is a real gap against the brief, and I chose it
knowingly: the float path is what reproduces libvorbis to 1 LSB, and since
reversibility is carried by the keep-list, the lifting buys exactness only
where the transform is concerned. Converting the rest to fixed point would
make the synthesis deterministic across platforms (float32 is not, strictly)
without changing the keep-list. Worth doing; not urgent.

All findings below have since been fixed; each subsection records what the
defect was and what replaced it.

### 6.2 Two precision defects

`vd_ana::spectrum` scaled samples by `<< (F−15)` — that is `1/32768` — while
`wav::frame` had written `round(y × 32767)`. A systematic 3e-5 gain error.
Now multiplies by `fx(1/32767)`.

`recover()` converted the floor to fixed point before dividing. At F = 32 a
floor of 1e-7 is 429 units — nine bits. Now `divide()` works in double and
carries only the quotient as fixed point.

Neither changed a single keep on the corpus. Both are correct and both stay.

### 6.3 The WAV reader assumed a bare header — fixed

`pcmwin::fill` seeked to `44 + first·ch·2` and the frame count came from the
file size minus 44. True of what `wav.inc` writes, false of most other WAVs:
a `LIST` chunk would have shifted every block silently.

`pcmwin::open` now walks the RIFF chunks for `data`, taking its offset and
size, and checks `fmt` against the stream — format, channels, rate and bit
depth. A mismatched WAV now fails at open with a named reason instead of ten
thousand records later. Verified by inserting a 28-byte `LIST` chunk ahead of
`data` and recovering exactly, and by feeding mode `d` a 2-channel WAV against
a 6-channel meta (`/tmp/s.wav: 2 channels, the stream has 6`).

### 6.4 Assumptions asserted rather than handled — fixed

**`aud.wprev`** had to equal the previous block's flag or mode `c` would
`FATAL`. Measured on 4,500 long blocks without exception, but a `FATAL` on a
valid stream is the wrong failure mode, and the obvious fix — a per-packet
correction slot — costs a record per packet to say "nothing to correct",
which is the entire saving.

`tsv::peek()` resolves it: it returns the next record's tag without consuming
it, so an *optional* record costs nothing when absent. Mode `c` emits
`aud.wpfix` only where the derivation fails; mode `d` peeks for it. Streams
that satisfy the assumption carry zero `wpfix` records; a stream with one
flag deliberately flipped emits exactly one and still round-trips.

**Sequenced codebooks** (`cb.sequence = 1`) were treated as if each scalar
stood alone. `take()` now carries the running sum through a symbol's scalars,
which is correct because all three placements process a symbol's digits in
order. Tested by forcing `cb.sequence` to 1 across a stream: exact round trip,
meta 30.1% against 18.6% unsequenced.

**`KEEP_MAX = 2^20`** keeps per packet remains a `FATAL`. A 255-channel
stream with an uncovered block could exceed it. That one is a genuine bound
rather than an assumption, and the message says so.

### 6.5 Recomputation — fixed

`take()` recomputed `fx(minval)` and `fx(delta)` per digit; they are now
`min_fx` and `delta_fx`, computed once in `vd_book::read`. `vd_ana::window`
rebuilt the window per sample per block; there are only eight windows — the
three flags — so they are cached and invalidated at each setup. `reach()` ran
per partition; `build_reach()` fills a table per setup.

Timing is unchanged (1.16 s / 0.72 s against 1.10 s / 0.70 s on the
85-second file), which says the cost was never where the review guessed. The
changes stay because they remove three recomputations of setup-fixed values,
not because they were hot.

### 6.6 Code that was odd but correct — fixed

`walk()` obtained the channel map by creating a `wav` on `/dev/null`, copying
its `ord[]` into the file-scope `sink` and closing it, because `WAV_ORDER`
lived in `wav.inc` and the map was filled by `create()`. It is now
`wav_order(ch, ord)`, a free function both the sink and `pcmwin` call, and
`pcmwin` owns its own copy. An earlier version pointed `pcmwin::ord` at a
block-scope `wav` and read stack garbage — the bug that held multichannel
recovery at 12 % until it was found.

`vd_dec` carried a `hook` function pointer and a `curmap` member from the
retired compare mode, along with a second floor render behind it. Both are
dead since `audio_rec` took over the recovery path; removed.

### 6.7 What is well done

The tee. Three roles over one traversal without duplicating the page walk or
the setup reader, and every structural record reaches the output untouched
by code that never names it.

Reconciliation at the point of use (§4.5). It is the reason exactness is a
property of the structure rather than of every stage being right.

`tsv::get` checking every tag. Every one of the dozen bugs found while
building this surfaced as a named tag mismatch at a record number, never as
wrong audio.

---

## 7. Verification

- 26 streams: `c` then `d` bit-exact; restored TSV rebuilds the original Ogg
  through `balrogg d`.
- Synthesis: 1 LSB max vs `oggdec` (or ffmpeg where oggdec cannot decode),
  RMS 0.14–0.59 LSB, on every file.
- `lifted_dct4`: 0 ulp round trip at M = 64..4096; 3.5e-8 vs float.
- Coupling inverse: exhaustively checked on the integer grid within
  `|ang| ≤ |mag|`.
- Clean under `-Wall -Wextra -Wshadow -pedantic`, c++11/17/23.

- ASan+UBSan on the `c`/`d` driver over seven streams including a 3-link
  chain and a sequenced-codebook stream: clean.
- 120 corrupted-meta and 60 corrupted-WAV mutations into mode `d`: no
  unexpected exits.
- A stream with `cb.sequence` forced to 1 throughout: exact.
- A stream with one `aud.wprev` flipped, so the derivation fails: exact, one
  `aud.wpfix` record.
- A WAV with a `LIST` chunk ahead of `data`: exact. A WAV whose channel count
  disagrees with the meta: rejected at open.

The `-Wpedantic` builds report `__int128` as a GNU extension in `pmul`. That
is deliberate: the shear needs a 128-bit intermediate, and the alternative is
splitting every multiply.

---

## 8. Floor prediction from PCM: measured, not implemented

`flr.y` is the largest remaining item in the meta. libvorbis computes the floor
in `floor1_fit` from `logmask`, the psychoacoustic mask of the signal *before*
quantization, which the decoded PCM does not contain. The question is whether
enough of it survives to be worth predicting.

Three facts bound the answer.

**The dB table's step ratio is 1.064986, so a factor of two is 11.01 steps.**
`(floor, r = 2)` and `(floor + 11 steps, r = 1)` give the same spectrum, so a
per-bin factorization is ambiguous by construction.

**Where the residue is zero the PCM says nothing about the floor.** On
`00000000.ogg`, 11.3% of bins are zero, 11.1% have `|r| = 1` (the floor is
exactly `|X|` there), and 77.5% have `|r| >= 2`, visible only up to the
ambiguity above. On a sparse stream it is far worse: `mono_q2.ogg` is 99.3%
zero.

**The coded value is already a strong residual.** `flr.y` codes
`Y - render_point(neighbours)`, and 58.6% of posts code as zero, meaning the
neighbour interpolation was exactly right. Any PCM-derived prediction has to
beat that, not the raw floor.

Measured on 16,268 posts of `00000000.ogg`, with the zero/nonzero structure
kept intact so the run-length coding of the dropped posts survives:

| predictor for the nonzero posts | R² | H |
|---|---:|---:|
| none (as stored today) | — | 4.824 bits |
| one windowed spectrum quantile | 46.1% | 4.406 |
| five spectrum windows | 61.1% | 4.198 |
| previous packet's floor only | 59.7% | 4.312 |
| five windows + previous packet | 70.0% | 4.065 |

The best model explains 70% of the variance, which is real signal, but entropy
falls only as `0.5 log2(1/(1-R²))`. At 41.4% of posts carrying a nonzero value
the saving is 0.31 bits/post against 2.978, so **about 10% of the floor stream
and 3% of the meta** — for five windowed quantiles per post in both roles, a
fitted seven-coefficient model in the meta, previous-packet tracking, and a
rewrite of the `flr.y` representation on a round trip that is currently exact.

Two blind alleys worth recording. Fitting the floor from the spectrum *alone*,
ignoring the neighbour prediction, gives 5.26 bits against 3.35 — worse than
what is stored now. And a plain least-squares fit on `[neighbour, spectrum]`
also loses (4.957 bits), because it shrinks the neighbour coefficient to 0.499
and destroys the predictor that was already working; the spectrum has to be
applied to the neighbour *residual*, not alongside it.

### 8.1 Correction: the hand-rolled estimator was the wrong instrument

The table above measures ad-hoc estimators. libvorbis itself computes the
floor from PCM, so the right experiment is to run *its* fit on the decoded
PCM and see how much of the original floor comes back.

An earlier attempt at this compared the **coded** `flr.y` values and found only
34% of nonzero posts matching. That comparison is invalid: the coded value is a
residual against the neighbour interpolation, so a one-step difference in the
underlying floor shows up as a large difference in the residual. The right
comparison is the unwrapped `Y`.

Re-encoding a decoded WAV with the encoder and setting that produced it,
then unwrapping both floors (`mono_q2.ogg`, 131 blocks, 3,779 posts):

| comparison | identical | within +-1 | H |
|---|---:|---:|---:|
| coded `flr.y` (the earlier, invalid test) | 64.5% | -- | 2.649 bits |
| **unwrapped floor `Y`** | **65.9%** | **84.3%** | **2.100 bits** |
| cost basis: coded `flr.y` as stored | -- | -- | 3.220 bits |

So `dY` against a libvorbis-refit floor is 2.100 bits against 3.220 -- about
**35% of the floor stream and 10% of the meta**, three times what the regression
in the table above could reach. The psychoacoustic fit is a far better
predictor than any windowed statistic, which is unsurprising: it is the
function that produced the value.

Two obstacles remain, both measured rather than assumed.

**Floor layout: not an obstacle.** For `00000000.ogg` no encoder setting
reproduces the codebooks, but the *floor* configuration -- post count, X
positions, multiplier -- matches at q >= 5. Post alignment is all the
prediction needs.

**Block decisions: an obstacle.** The same re-encode yields 415 packets against
the original's 877, so the packets do not correspond in time and their floors
cannot be compared one to one. Driving an external encoder therefore only works
where its block decisions happen to agree.

The way past both is to port `floor1_fit` rather than shell out to an encoder,
driving it with the block sizes and floor configuration already known from the
stream, so alignment is exact by construction. `floor1_fit` takes two inputs:
`logmdct`, which the decoded spectrum supplies directly, and `logmask`, the
psychoacoustic mask -- which comes from `_vp_compute_mask` with encoder-side
parameters that are not in the bitstream. Substituting `logmdct` for `logmask`
is the obvious approximation, and its accuracy is the one number in this
section that has not been measured.

### 8.2 The fit ported, and where the ceiling actually is

`floor1_fit` and its helpers were ported to Python
(`floor1_fit_experiment.py`) to settle the substitution. All of the
encoder-side floor parameters turn out to be constant across every template in
`modes/floor_all.h` -- `maxover 60, maxunder 30, maxerr 500, twofitweight 1.0,
twofitatten 18.0` -- so none of them has to be fitted.

Measured on `00000000.ogg`, against a cost basis of 3.44-3.52 bits per post
for the coded `flr.y` as stored:

| `logmask` fed to the fit | median \|dY\| | H(dY) |
|---|---:|---:|
| the decoded spectrum, raw | 20 | 5.35 |
| spectrum spread over 9 bins | 29 | 4.88 |
| spectrum spread over 25 bins | 32 | 4.93 |
| ... best of those, per-block offset removed | -- | 4.45 |
| **the true floor curve (self test)** | **1** | **2.30** |

The last row is the port validated against itself: fed the actual floor as its
own mask, the greedy fit returns 47.9% of posts exactly and 80.0% within one
step. That confirms the port works, and it also fixes the ceiling. Even a
*perfect* mask yields 2.30 bits against 3.44 -- **33% of the floor stream,
about 10% of the meta**, agreeing with the 2.100 bits the real re-encode
achieved.

So the whole of the available gain sits in the mask, and no cheap proxy for it
works: every spectrum-derived variant lands *worse* than storing the coded
values unchanged. `logmask` is a masking threshold, not a spectrum; it sits
above the signal by an amount that varies per band, and spreading or offsetting
the spectrum does not reproduce that.

### 8.3 What would actually be required

Reaching the 10% means porting `_vp_compute_mask` from `psy.c` -- tone/noise
separation, bark-scale spreading, the absolute threshold of hearing -- and
supplying a `vorbis_info_psy`, which is encoder-side, quality-dependent, and
absent from the bitstream. libvorbis ships a fixed set of psy templates, so
they could be tried in turn with the winning index stored in the meta, exactly
as the classifier thresholds are fitted today. But the corpus files were not
produced by this libvorbis -- no quality setting reproduces their codebooks --
so there is no guarantee their psy settings are in the set at all.

### 8.4 The psy port attempted, and why it stalls

The estimate of "roughly 600 lines" above was wrong, and the reason it was
wrong is worth recording.

`bark_noise_hybridmp`, the bark-window construction and `_vp_noisemask` were
ported (`floor1_fit_experiment.py`). That is the noise half of the mask, and
it is faithful: a weighted least-squares line through a bark-width window,
run twice, exactly as `psy.c` does it. Feeding its output to the ported
`floor1_fit`, with the seventeen per-band offsets fitted by coordinate descent
on the file itself:

| mask | median \|dY\| | H(dY) |
|---|---:|---:|
| noise mask, offsets zero | 14 | 5.65 |
| noise mask, 17 offsets fitted (3 sweeps) | 10 | 5.46 |
| for comparison: raw spectrum | 20 | 5.35 |
| cost basis: coded `flr.y` as stored | -- | **3.37** |

Still far worse than storing the values. The noise mask on its own is not the
mask; `_vp_offset_and_mix` is where noise and tone are combined, the absolute
threshold of hearing applied and peaks limited, and `_vp_tonemask` needs the
FFT of the windowed PCM -- a second transform -- together with the tone curves
`setup_tone_curves` builds from `toneatt[]`.

The deeper problem is not the code but the parameters. `vorbis_info_psy` holds
roughly 121 free values -- `toneatt[17]`, `noiseoff[3][17]`,
`noisecompand[40]`, the ATH attenuations, the tone master attenuations, the
curve limits -- and **none of them is in the bitstream**. They are chosen by
quality and sample rate in the encoder's templates. Fitting seventeen of them
by coordinate descent already moved H by only 0.19 bits; fitting a hundred and
twenty-one is a different kind of problem, and on a stream whose encoder is not
this libvorbis there is no template to fall back on.

So the position is: the ceiling is real and measured at about 10% of the meta,
the fit that would reach it is ported and validated, and the mask that drives
the fit is the part that cannot be had -- not for want of code, but because it
is a hundred-odd tuning constants that the format never transmits. Storing the
floor is, on this evidence, the cheaper answer.

---

## 9. Why the encoder helps the floor and not the residue

libvorbis derives both the floor and the residue from PCM, so it is natural to
ask whether re-encoding could predict the residue as well as it predicts the
floor. Measured on `mono_q2.ogg`, whose encoder and setting are reproducible,
by re-encoding the decoded WAV with `oggenc -q2` and comparing record streams:

| | recovery in tsv2wav | re-encode |
|---|---:|---:|
| classes right | **98.82%** | 95.71% |
| digits right | **97.11%** | see below |
| floor posts right | -- | 64.44% |

The digit streams cannot be compared directly: the classes differ, so the
number of digits differs (16,448 against 12,768) and the streams desynchronise
after the first mismatch. Restricting to the packets whose class streams agree
entirely -- 7 of 131, the most favourable subset available -- the re-encode's
digits are 95.36% identical, still below what the recovery achieves across
*all* packets.

The asymmetry has one cause. The floor is a fit to the psychoacoustic mask of
the signal *before* quantization, and the decoded PCM does not contain it, so
the encoder's psy model is the only route to it. The residue is different: it
is exactly `spectrum / floor`, and the floor is already in the meta. The
recovery therefore holds something the re-encoder does not -- the stream's own
floor -- and the re-encoder, having to guess it, gets 64% of the posts right
and a correspondingly wrong residue.

So the two halves want opposite treatment. Carry the floor and let libvorbis
predict it; derive the residue and do not.
