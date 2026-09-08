# tsvcomp: a balrogg record stream, compressed

    tsvcomp c [options] input.tsv output.tc
    tsvcomp d [options] input.tc  restored.tsv

`tsvtrans` rearranges a record stream and leaves the coding to somebody else.
This one does the coding. It walks the same structure -- the walk is
tsvtrans's, and has to be, since what a record means is decided by records
already read -- and instead of writing a value out as text hands it to a model
and a range coder.

The model is mp3c's, described in `MP3C-ALGORITHM.md`. Every value becomes a
cascade of binary decisions and every decision goes through the same four
parts: two counters on different contexts, an APM correcting the first, a
logistic mixer weighing them, and a carryless binary range coder underneath.
The encoder and the decoder are the same code -- `tc_fam::code` either codes
the value it is given or decodes one, and both then run the identical update
-- so a model change cannot desynchronise the two halves.

What is different from mp3c is which variables the contexts are built from.
Those are declared rather than written, in mp3c's own IDX notation; §"The
contexts, declared" is the whole of what this program knows about how a
variable becomes a table row.

Options:

| | |
|---|---|
| `-v` | say where the bits went, by stage |

`TSVCOMP_IDX` names a file to take the context declaration from in place of
the one compiled in. Every measurement below was made that way.

## What it costs

Against `xz -9e` on the same TSVs, and against the `.ogg` the TSV came from:

| | .ogg | .tsv | `xz -9e` | tsvcomp | of .ogg | of xz |
|---|---:|---:|---:|---:|---:|---:|
| 17 libvorbis files | 12,734,656 | 108,163,951 | 16,860,676 | **12,117,967** | 95.2% | 71.9% |
| 22 ffmpeg files | 515,872 | 4,386,774 | 381,424 | **285,955** | 55.4% | 75.0% |
| **total** | **13,250,528** | **112,550,725** | **17,242,100** | **12,403,922** | **93.6%** | **71.9%** |

The `of .ogg` column is the one that matters: it is the whole point of balrogg
that a `.tsv` plus a coder should come out smaller than the Ogg Vorbis file
that produced it, and every file in both corpora does.

The ffmpeg files gain far more because they are short -- 10 to 40 KB -- and a
large part of each is the setup packet, whose codebooks the record-level
header model reads far better than any byte-oriented compressor can. The
libvorbis files are minutes long, so the residue digits are nearly all of them
and the margin is what the digit model earns.

Every file in both corpora round-trips byte for byte:

    make test-comp                          the bundled 00.ogg
    make test-comp TESTFILES=path/to/dir    every .ogg in a directory

`-v` says where the bits went. It is accounting only -- the same trick
MP3C-ALGORITHM.md's per-stage table came from -- and changes no output:

    $ tsvcomp c -v 00000000.tsv 00000000.tc
    tsvcomp: 171491 bytes of model, 40 MB of tables
      headers          3507 bytes   2.04%
      pages             836 bytes   0.49%
      packets            75 bytes   0.04%
      floor            6031 bytes   3.52%  18083 values, 2.668 bits each
      class            2265 bytes   1.32%  10424 values, 1.738 bits each
      digits         158777 bytes  92.59%  434720 values, 2.922 bits each

## How a number becomes bits

There is one value coder and every field uses it. A magnitude is:

* `== 0?`, `== 1?`, `== 2?` -- three direct steps, which is where nearly every
  residue digit stops;
* otherwise the bit length of what is left, in unary, one decision per
  length;
* then its mantissa, most significant bit first;
* then, if the value can be negative, a sign.

Nothing has to know a field's range in advance, which is what lets a page
granule and a residue digit share a coder. Where in that cascade a bit sits
is context in its own right -- a family's tables are `TC_NODE` rows deep per
context -- so the ladder step and the length bit do not have to be declared as
variables.

**The mantissa is a plane of its own**, on its own index and its own tables,
as mp3c's `Smant` is a submodel of its own. Folded into the head it would be
six times the depth, multiplying the whole of a rich context by the part of
the cascade that carries the least and is reached the least: the head of a
digit is worth asking 230,400 ways and its fourteenth mantissa bit is not.

## The contexts, declared

`idx.inc` reads the part of mp3c's IDX notation that a single-file program
needs. A factor line is a variable, a base and a pattern, and each `1` in the
pattern places a threshold:

     a_q1:   q1,    1!11111

is `(q1>0) + (q1>1) + ... + (q1>4)`, six buckets, one factor of a mixed-radix
index. `ADD 32: fld` is a dense factor for a variable that is already a small
integer, and `&11111111` keeps the named bits of one. Each family declares
five indices: `_a` for the rich counter, `_b` for the coarse one, `_s` for the
APM's own grouping, `_m` for the mixer's, `_t` for the mantissa plane. mp3c
computes four separate indices over the same variables for exactly this
reason -- each stage wants the grouping that suits it, and sharing one is a
compromise none of them asked for.

**What IDX is for is that a variable which did not pay stays visible.** A
pattern of zeroes contributes one bucket and is dropped, so the declaration
records what was tried:

     a_col:  col,   1!0000000       # +0.27%, and worse in combination
     a_bkq:  bkq,   1!0000          # 0.9% worse than the class it replaces

The difference from mp3c is that nothing is generated: this is IDX's *tuning*
shape -- thresholds live, patterns editable -- kept, with the declaration
parsed at startup rather than compiled in by `idx2inc.pl`. A stream costs
milliseconds to walk and the parse is a few hundred microseconds once, so the
shipping shape would buy nothing that matters and would cost a perl dependency
and a generated file that can go stale against its source.

The declaration *is* the format: move one threshold and the same bytes mean
something else. So a stream carries a checksum of the declaration that coded
it, and mode d refuses one it does not have --

    tsvcomp: 00.tc: coded against a different context declaration (e542, this
    one is 74aa) -- set TSVCOMP_IDX to the file it was made with

-- rather than decoding to the end and producing nonsense.

## What each context is worth

Measured on `00000000`, `00000007` and `ff_44100_q5` -- 1,070,740 bytes coded
between them -- by turning one thing off in the declaration and leaving
everything else alone:

| off | coded | |
|---|---:|---:|
| nothing -- the shipped declaration | 1,070,740 | — |
| the residue class, in the digit model | 1,121,836 | +4.77% |
| the neighbouring digits (`q2`, `zrun`) | 1,100,986 | +2.83% |
| the same slot one packet ago (`t1`, `p1`) | 1,095,716 | +2.33% |
| the column, in the floor model | 1,075,515 | +0.45% |

Two of those are worth reading twice.

**The class is the largest single factor, and that was not the expectation.**
TSVTRANS.md measured the partition and the pass as the coordinates that
predict a digit, and they do -- but the class is *the choice of ladder*, and
the ladder decides what a digit can be:

| `cls` in `dig_a` | coded | |
|---|---:|---:|
| 16 buckets -- shipped | 1,070,740 | — |
| 8 buckets | 1,074,377 | +0.34% |
| 4 buckets | 1,120,557 | +4.65% |

Once it is in, the frequency band it was competing with all but disappears:
one bucket costs 0.05% and thirteen *cost* 0.10%, so `band` is at three in the
shipped declaration. The partition had been standing in for the class.

The obvious refinement does not work. `bkq` -- the codebook's `off`, which is
the half-width of its multiplicand range, in six buckets rather than sixteen
-- is the same information in a form that would mean the same thing in the
next file, where a class number means nothing at all. Substituted for `cls` it
measures **+1.01%**. Two classes with the same range are still not the same
choice, and the class number carries which one the encoder made.

**The column ablation looks small and is not.** The floor is 3.5% of a coded
stream on `00000000` and 5.3% on `00000007`, so 0.45% overall is most of it:
floor posts alone go from 53,011 bytes to 57,529 across those two, **+8.52%**.
That is TSVTRANS.md's −22.20% seen from the other side, against a model that
already has the previous post and the same post one packet ago to work with.

## The counter's precision, and a bug worth naming

`cm_cnt` is mp3c's `Node2ii`: a probability and a visit count, updated by the
running mean, `p += (target - p) / (t + 2)`, with `t` capped at a tunable
limit so a well-visited context settles without freezing.

At 12 bits that quotient truncates to zero. A context visited 4,094 times
stops moving at all, whatever it then sees -- and the symptom is not a bad
prediction but a frozen one, so it does not look like a rate problem. It was
found by sweeping the limit: 255 and 1023 behaved as expected and 4095 doubled
the output.

Holding the probability at 16 bits and handing it out at 12 moves the freeze
out past any limit worth setting. What that is worth is not one number, and
the shape of the table is the whole story -- same three files, same
declaration, only the digit counters' limit and their width changing:

| `dig_rA` | 12-bit | 16-bit | |
|---|---:|---:|---:|
| 255 | 1,075,652 | 1,070,740 | −0.46% |
| 1023 | 1,107,545 | 1,071,089 | −3.29% |
| 4095 | 1,233,445 | 1,075,314 | **−12.82%** |

At 16 bits the limit barely matters, which is how a tunable parameter should
behave: it chooses how fast a settled context still tracks, and every setting
in a reasonable range is nearly as good as every other. At 12 bits it is a
cliff, and a sweep that wandered up to it -- which is exactly what an
optimizer does -- would find a factor it could not explain. The shipped limit
is 255, where the two builds are within half a per cent, so this is not
half a per cent of headline: it is a trap taken out of the search space.

## The header packets

The identification, comment and setup packets are not modelled field by
field, and they are not coded as text either. **Their tag sequence is not
information.** Both sides run the same reader -- `vd_setup`'s -- which asks
for the tags it needs in the order it needs them, so only the values have to
cross.

Mode c reads the packet from the input while a memory stream captures it,
then re-reads the capture record by record and codes the values. Mode d
decodes the values into a memory stream and runs the same reader over that,
with a tee to the output, so the records reach the restored file under the
tags and in the order they were read with. A tag costs one bit -- "the same as
the last one?", which balrogg's runs make true nearly always -- plus, once per
stream, its letters.

That is why the ffmpeg corpus does so much better than the libvorbis one.
`ff_8000_q1` is 9,808 bytes of Ogg of which the setup is a large fraction, and
its codebooks reach tsvcomp as `cb.len`, `cb.used` and `cb.mult` records under
a model that knows what each of those is, rather than as decimal digits under
a model that does not.

## The page header

`tsvtrans` drops the page type, sequence and serial and refuses a stream whose
header is not canonical. tsvcomp codes them as **residuals** against the same
expectations -- the type against what the position implies, the sequence
against a counter, the serial against the page before, the granule against the
page before -- so the usual case is a run of zeros costing the same nothing,
and a stream that disagrees costs a few bits instead of being turned away.
Page headers are 0.5% of a coded stream either way.

## Memory and speed

About 40 MB of tables, and 0.36 s to code and 0.38 s to decode `00000000`'s
1.27 MB TSV -- 3.5 MB of TSV per second each way, against 1.6 MB/s for `xz
-9e` on the same file, for 28% less output. Decoding is the same work as
coding, which is what a context-mixing coder costs and where `xz` wins
outright.

The tables are what the declaration asks for. `dig_a`, the digit model's rich
counter, is the large one at 230,400 rows; the sweep that set its shape was
mostly about *not* spending more. Widening `band` from three buckets to
thirteen takes it to 998,400 rows and the program from 40 MB of tables to 111
MB -- and codes 0.10% *worse*, because those rows are then visited a fifth as
often each.

## What it does not do

* **There is no optimizer.** The patterns were set by hand-run sweeps over
  three files; IDX exists so that `opt.pl` can hill-climb them over a corpus,
  and pointing one at this is the obvious next thing. The `Debug`/`Const`
  machinery and the `!MAP!` markers it scans for are not implemented, so that
  means either adding them or writing a sweeper that edits the `.idx` and
  re-runs -- which is all `TSVCOMP_IDX` was added for.
* **There is no match model.** mp3c does not need one; a residue stream with
  long exact repeats -- silence, loops -- would reward one here.
* **The mixer has two inputs.** mp3c's has two as well, but its counters have
  a third and fourth context that this does not: `ctx2`, `ctx3` and three
  granules of history rather than one.
* **The codebook substitution that META.md describes is not applied.** A
  stream whose codebooks are one of the sets in `vbooks.inc` still pays for
  them record by record. That is a large win available on libvorbis-produced
  files and it belongs here rather than in the header model.
