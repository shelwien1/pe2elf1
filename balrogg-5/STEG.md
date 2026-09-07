# tsvsteg: a payload in the quantization gaps

    tsvsteg c input.tsv output.wav output.meta payload
    tsvsteg d output.wav restored.tsv output.meta payload

`tsvsteg` is `tsv2wav` with one substitution. Where `tsv2wav` decides which of
two integers a sample lands on by a rounding rule, `tsvsteg` decides it by
decoding a payload. Everything else -- the synthesis, the residue walk run
backwards, the floor refit, the class and digit models, the granule prediction
-- is `tsv2wav`'s, called from `tsv2wav.cpp`; `tsvsteg.cpp` is a driver and
`steg.inc` is the substitution.

The pair stays lossless. Mode d rebuilds the record stream byte for byte and
`balrogg d` rebuilds the original Ogg from it, exactly as before. What the
payload changes is not whether the stream comes back but how many bytes of
meta it takes.

## Why there is a gap at all

META.md's account of the WAV is that the tool owns it:

> Where a sample lands between two integers is the one genuinely lossy step in
> the whole chain, and the tool owns it: the WAV is its own output, and mode d
> reads back whatever was written.

That is the whole opening. Nothing downstream cares which of the two integers
is present, because the meta is computed against whichever one is -- so the
choice is free to carry something, and `tsv2wav` spends it on a rounding rule
instead. META.md lists the rules it measured (floor, nearest, ceiling, quarter
steps) and the error feedback it sweeps. `tsvsteg` spends the same freedom on
a message.

## The model

The upper integer is chosen with probability equal to the fractional part.

That is the same first moment round-to-nearest has -- E[v] = x exactly -- so
the samples stay unbiased, and it makes the choice a Bernoulli draw whose
distribution both sides can compute from x alone. Nothing about the model is
carried.

The draw is made by **decoding** the payload under that model, with the range
coder supplied as `sh_v2f.inc`. Because the model *is* the coding
distribution, the payload is consumed at the gap's own entropy; because the
coder is exact, **encoding the choices back returns the payload**. So the tool
never stores a payload anywhere. It stores choices, and the choices are the
payload seen through the model.

Three consequences worth stating plainly:

* **Capacity is the entropy of the fractional parts.** For fractional parts
  spread evenly, that is ∫₀¹ H₂(p) dp = 1/(2 ln 2) = **0.721 bits a sample**.
  Measured over 39 files it is 0.72 on every one of them.
* **The stego WAV differs from the plain one in a quarter of its samples**,
  always by exactly one LSB. The two rules disagree when the draw takes the
  farther integer, which happens with probability min(f, 1−f), and
  ∫₀¹ min(f, 1−f) df = 1/4. Measured on `00.ogg`: 25.0%, max |Δ| = 1.
* **The WAV's quantization noise doubles.** Round-to-nearest leaves an error
  uniform on [−½, ½], variance 1/12. The draw leaves an error of variance
  f(1−f), which averages 1/6. That is +3.01 dB, and it is the real price --
  see the meta cost below, which is that price arriving.

A sample carries nothing when it has no gap to fill: when the fractional part
rounds to 0 or 1 out of 32768 (digital silence, mostly), or when one of the
two integers is outside 16-bit range. Both sides decide that from x, so they
agree without a flag.

## Why extraction needs the meta

Recovery needs x, the sample before it was quantized -- knowing the WAV is not
enough, because the choice is only meaningful relative to where the sample
would have landed. x comes from the synthesis and the synthesis comes from the
record stream, which mode d has just rebuilt. So mode d is two passes: rebuild
the TSV as `tsv2wav d` does, then synthesise it again, reading the WAV instead
of writing it, which turns each sample back into the choice that was made and
each choice back into payload.

**The payload therefore cannot be read out of the WAV alone. It takes the
meta too.** That is a property of the construction, not a security claim; see
the limits at the end.

## Container

The payload goes in behind its own 8-byte little-endian length, so extraction
knows where the real bytes stop and the coder's flush begins. Mode c runs an
encoder alongside the decoder over the same choices and checks that what comes
back is what went in, so a payload too large for the stream fails during the
synthesis rather than silently truncating.

## What it costs

The doubled quantization noise arrives as digit corrections: a sample moved
by one LSB sometimes moves a recovered residue digit into the next bin, and
that costs a record. Measured against the `tsv2wav -DBLR_VORBIS` metas for the
same files, with a 4 KiB payload:

| | tsv2wav | tsvsteg | |
|---|---:|---:|---:|
| 17-file corpus | 13,252,200 | 15,078,038 | **+13.78%** |
| 22 ffmpeg files | 1,007,208 | 1,104,599 | **+9.67%** |

Both round trip exactly: 17 of 17 and 22 of 22 give back the record stream
byte for byte *and* the payload byte for byte.

**The cost is wildly uneven, and it tracks the encoder's quality setting.**
On the ffmpeg files, which differ only in rate and quality:

| | q1 | q5 | q10 |
|---|---:|---:|---:|
| 11025 Hz | +0.05% | +0.31% | +26.26% |
| 16000 Hz | +0.04% | +4.51% | +57.06% |
| 22050 Hz | +0.07% | +0.55% | +19.39% |
| 32000 Hz | −0.03% | +4.82% | +20.64% |
| 44100 Hz | +0.01% | +4.67% | +7.87% |
| 48000 Hz | +0.02% | +4.45% | +12.39% |

A coarsely coded stream has residue digits far wider than an LSB, so moving a
sample by one almost never changes one and the payload is close to free. A
finely coded stream has digits at the same scale as the noise, and the payload
costs half again as much meta. `00000008` and `00000009`, which are 86% of the
corpus by bytes, pay +14.45% and +16.61% and are what the corpus total is.

Five of the seventeen corpus files come out **smaller** (`00000001` and
`0000000B` by 0.34%). Moving a sample is as likely to fix a digit as to break
one where the walk was already marginal, so at the low end this is noise
around zero, not a cost.

**The cost does not depend on how much payload there is.** Every gap is filled
by a draw whether or not there are payload bytes left to draw from -- past the
end the coder reads padding -- so a one-byte payload buys the same noise as a
full one. On `00000000`, against a 48,084-byte baseline:

| payload | meta |
|---|---:|
| 1 byte | 48,069 |
| 4,096 bytes | 48,013 |
| 20,000 bytes | 48,163 |

That is the same ±0.15% either way. If a stream is worth using at all, fill it.

## Capacity

0.72 bits a sample, which is 63.5 kbit/s of payload in a 44.1 kHz stereo
stream, or about 7.9 KB/s.

| | payload capacity |
|---|---:|
| 17-file corpus | 2,625,567 bytes |
| 22 ffmpeg files | 635,446 bytes |

Measured per file the rate is 0.72 bits a sample on every stream that is not
partly silent; `ff_8000_q1` comes to 0.69 because 4.3% of its samples have no
gap to fill at all.

`TSVSTEG_VERBOSE=1` reports the figure. What mode c *quotes* when a payload
does not fit is deliberately a little under the entropy -- 32 bytes and a
further tenth of a percent -- because the entropy is a ceiling a range coder
does not quite reach: on `00.ogg` the ceiling is 21,565 bytes and the largest
payload that actually goes in is 21,544. The quoted figure is one that fits.

## Limits

**There is no authentication and no encryption.** The payload goes in as it
arrives. Anyone holding both the WAV and the meta recovers it by running mode
d; that the WAV alone is not enough is a consequence of needing x, not a
secret. Encrypt before embedding if the content needs it.

**A single altered sample is not detected, because it is the carrier.** Moving
a sample by more than one LSB is caught -- mode d finds a sample that is
neither of the two integers the stream allows and refuses, naming the sample.
Moving one by exactly one LSB is a legal choice, so it passes, and the payload
comes back altered: on `00.ogg` one flipped sample changed one byte of a 4,000
byte payload and left the rest intact. A tampered or truncated WAV is caught;
a subtly dithered one is not.

**The error feedback is off** (`STEG_SHAPE`). `tsv2wav` sweeps it per file and
carries nothing, which it can do because nothing downstream has to agree about
it. Here both sides compute x and x depends on it, so it would have to be
carried in the meta -- one record a file -- before it could be swept. Until
then the shaping that is worth up to 12.9% of a `tsv2wav` meta is not
available here, and some of the cost above is that absence rather than the
payload.

**The obvious next knob is not implemented.** The draw could be sharpened
toward the nearer integer -- P(upper) ∝ f^γ -- which trades capacity for
noise, and at γ→∞ is round-to-nearest with no payload. That would let a
finely coded stream carry less and pay less, instead of paying +57% or
nothing. It needs the exponent carried in the meta so mode d can build the
same model, which is the same one-record change the shaping needs.

**Little-endian only.** `sh_v2f.inc` aliases the coder's low word and carry as
one 64-bit counter; the build stops on a big-endian target rather than
miscoding quietly.

## Verification

* 39 files -- the 17-file corpus and the 22 ffmpeg files -- through
  `c` then `d`: restored TSV byte-exact and payload byte-exact on all 39.
* Five streams end to end -- `balrogg c`, `tsvsteg c`, `tsvsteg d`,
  `balrogg d` -- rebuild the original Ogg byte for byte with the payload
  intact: `00.ogg`, two ffmpeg files and two corpus files. `make test-steg`
  runs this over whatever `TESTFILES` names.
* An empty payload; and 21,531 bytes into `00.ogg` against a quoted 21,512,
  confirming the quote is under the true limit rather than over it.
* A payload above capacity: refused during the synthesis, naming the size that
  would fit.
* A WAV sample moved by 1000: refused, naming the sample.
* A meta from a different stream: refused.
* The stego WAV against the plain one on `00.ogg`: 25.0% of samples differ,
  every one of them by exactly 1 -- which is the ∫₀¹ min(f, 1−f) df = 1/4 the
  model predicts.

## Build

    make tsvsteg           # plain
    make tsvsteg-vorbis    # libvorbis-assisted, as the numbers above
    make test-steg         # round trip 00.ogg with a payload
    make test-steg TESTFILES=dir/

`tsvsteg.cpp` defines `BLR_STEG` and includes `tsv2wav.cpp`, whose `main` is
compiled out under that macro. The hooks in `tsv2wav.cpp` and `wav.inc` are
guarded, so the `tsv2wav` binaries are unaffected -- their metas are
byte-identical to the ones built before `tsvsteg` existed.
