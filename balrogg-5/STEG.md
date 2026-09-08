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

## Container, and what happens when the payload does not fit

The payload is read as an endless stream: the file, then `0xFF` for ever. So
the two awkward cases stop being cases at all.

**A payload smaller than the stream** runs into the padding. Nothing marks the
end of the file, because nothing needs to.

**A payload larger than the stream is not an error.** The gaps hold what they
hold; the prefix that fits goes in and the rest does not. Mode c says which
happened, and that line is the one thing it always prints:

    $ tsvsteg c 00.tsv 00steg.wav 00steg.meta 00.ogg
    tsvsteg: stored 21528 of 183832 payload bytes; the remaining 162304 did not fit

    $ tsvsteg c 00.tsv 00steg.wav 00steg.meta small.bin
    tsvsteg: stored 4000 payload bytes

The count is measured, not predicted. An encoder runs beside the decoder over
the same choices, and because mode d encodes the same symbols through the same
models, what that encoder gives back is exactly what mode d will give back --
so the payload bytes that survive are the leading run on which the two agree.
Past that the stream ran out of gaps and the coder's flush took over.

Mode d writes exactly those bytes and says so:

    $ tsvsteg d 00steg.wav 00.tsr 00steg.meta out.bin
    tsvsteg: recovered 21528 payload bytes

For that it needs the count, and the count is not knowable until the gaps have
been walked -- so it cannot ride at the front of the stream it would itself
displace. It goes in the meta, as `steg.n`. **That is the one place a tsvsteg
meta differs from a tsv2wav one**, and it costs both ways: a tsv2wav build can
no longer restore the record stream from a tsvsteg pair, and the meta says a
payload is there. The alternative was handing back the payload with the
coder's flush stuck on the end for the caller to trim, which is worse.

`TSVSTEG_VERBOSE=1` adds the gaps behind the count:

    tsvsteg: 239348 of 239360 samples had a gap to fill, 172586 bits =
    21573 bytes at the ceiling; 0.72 bits a sample

That ceiling is the entropy, and it is a ceiling rather than a promise: a range
coder spends a little over it and the flush costs a few bytes, so on `00.ogg`
21,573 bytes of ceiling took 21,528 bytes of payload. Size a payload by the
ceiling; believe the stored count.

## What it costs

The doubled quantization noise arrives as digit corrections: a sample moved
by one LSB sometimes moves a recovered residue digit into the next bin, and
that costs a record. Measured against the `tsv2wav -DBLR_VORBIS` metas for the
same files, with a 4 KiB payload:

| | tsv2wav | tsvsteg | |
|---|---:|---:|---:|
| 17-file corpus | 13,252,200 | 15,073,118 | **+13.74%** |
| 22 ffmpeg files | 1,007,208 | 1,103,549 | **+9.57%** |

Both round trip exactly: 17 of 17 and 22 of 22 give back the record stream
byte for byte *and* the payload byte for byte.

**The cost is wildly uneven, and it tracks the encoder's quality setting.**
On the ffmpeg files, which differ only in rate and quality:

| | q1 | q5 | q10 |
|---|---:|---:|---:|
| 8000 Hz | +0.40% | +9.73% | +8.36% |
| 11025 Hz | +0.04% | +0.41% | +25.76% |
| 16000 Hz | +0.03% | +3.80% | +56.57% |
| 22050 Hz | +0.03% | +0.82% | +18.76% |
| 32000 Hz | +0.08% | +4.77% | +20.75% |
| 44100 Hz | +0.03% | +4.73% | +7.65% |
| 48000 Hz | +0.03% | +4.48% | +11.86% |

A coarsely coded stream has residue digits far wider than an LSB, so moving a
sample by one almost never changes one and the payload is close to free. A
finely coded stream has digits at the same scale as the noise, and the payload
costs half again as much meta. `00000008` and `00000009`, which are 86% of the
corpus by bytes, pay +14.52% and +16.38% and are what the corpus total is.

Four of the seventeen corpus files come out **smaller** (`00000001` and
`0000000B` by 0.34%, `00000002` and `00000010` by 0.30%). Moving a sample is as
likely to fix a digit as to break one where the walk was already marginal, so
at the low end this is noise around zero, not a cost.

**The cost does not depend on how much payload there is.** Every gap is filled
by a draw whether or not there are payload bytes left to draw from -- past the
end the coder reads `0xFF` padding -- so a one-byte payload buys the same noise
as a full one, and an oversized one buys no more. On `00000000`, against a
48,084-byte baseline:

| payload offered | stored | meta |
|---|---:|---:|
| 1 byte | 1 | 48,131 |
| 4,096 bytes | 4,096 | 48,137 |
| 20,000 bytes | 20,000 | 48,125 |
| 30,000 bytes | 21,572 | 48,126 |

Twelve bytes of spread across a range of 30,000, and the row that overran costs
no more than the row that did not. If a stream is worth using at all, fill it.

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

These are entropy ceilings, which is what `TSVSTEG_VERBOSE=1` reports. Nothing
now has to be sized against them: a payload that overruns is stored as far as
it goes and mode c prints how far, so the ceiling is for planning and the
stored count is for trusting. On `00.ogg` the ceiling is 21,573 bytes and
21,528 go in.

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
miscoding quietly. Its types are this tree's fixed-width ones -- see TSV2WAV.md
7.1 -- so `uint`, `qword` and `byte` are `u32`, `u64` and `u8`, and nothing in
it changes width between Linux and Windows.

## Verification

* 39 files -- the 17-file corpus and the 22 ffmpeg files -- through
  `c` then `d`: restored TSV byte-exact and payload byte-exact on all 39.
* Five streams end to end -- `balrogg c`, `tsvsteg c`, `tsvsteg d`,
  `balrogg d` -- rebuild the original Ogg byte for byte with the payload
  intact: `00.ogg`, two ffmpeg files and two corpus files. `make test-steg`
  runs this over whatever `TESTFILES` names.
* An empty payload: stored 0, recovered 0.
* A payload nine times the stream's capacity -- `00.ogg` carrying its own
  183,832-byte Ogg -- stores 21,528 and exits 0, and what mode d gives back is
  the first 21,528 bytes of that Ogg, byte for byte.
* A payload of exactly the stored count, 21,528 bytes: all of it, exact.
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
