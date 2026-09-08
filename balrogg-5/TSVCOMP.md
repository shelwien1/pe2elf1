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
Those are declared rather than written, in mp3c's own IDX notation and through
its own toolchain: `IDX/tsvcomp.idx` is the declaration, `IDX/idx2inc.pl`
generates `MOD/`, and `IDX/opt.pl` tunes the result by patching the binary.
Nothing in `tsvcomp.cpp` decides a context.

    ./mk.sh              regenerate MOD/ and build for tuning
    ./mk.sh release      the same, with every parameter folded to a literal
    ./mk.sh check        prove the two builds code identically

Options:

| | |
|---|---|
| `-v` | say where the bits went, by stage |

## What it costs

Against `xz -9e` on the same TSVs, and against the `.ogg` the TSV came from:

| | .ogg | .tsv | `xz -9e` | tsvcomp | of .ogg | of xz |
|---|---:|---:|---:|---:|---:|---:|
| 17 libvorbis files | 12,734,656 | 108,163,951 | 16,860,676 | **12,075,279** | 94.8% | 71.6% |
| 22 ffmpeg files | 515,872 | 4,386,774 | 381,424 | **275,056** | 53.3% | 72.1% |
| **total** | **13,250,528** | **112,550,725** | **17,242,100** | **12,350,335** | **93.2%** | **71.6%** |

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
    tsvcomp: 168568 bytes of model, 13 MB of tables
      headers          3122 bytes   1.85%
      pages             809 bytes   0.48%
      packets            78 bytes   0.05%
      floor            5854 bytes   3.47%  18083 values, 2.590 bits each
      class            2139 bytes   1.27%  10424 values, 1.642 bits each
      digits         156565 bytes  92.88%  434720 values, 2.881 bits each

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
digit is worth asking 62,208 ways and its fourteenth mantissa bit is not.

## The contexts, declared

`IDX/tsvcomp.idx` is the declaration. A factor line is a variable, a base and
a pattern, and each `1` in the pattern places a threshold:

     dig_a_q1:   q1,    1!11111

is `(q1>0) + (q1>1) + ... + (q1>4)`, six buckets, one factor of a mixed-radix
index. `ADD 32: fld` is a dense factor for a variable that is already a small
integer, and `&11111111` keeps the named bits of one. `Number dig_rA, 1,
0!11111111` is a tunable integer -- the pattern read as binary, plus the base,
times the multiplier. Each family declares five indices: `_a` for the rich
counter, `_b` for the coarse one, `_s` for the APM's own grouping, `_m` for
the mixer's, `_t` for the mantissa plane. mp3c computes four separate indices
over the same variables for exactly this reason -- each stage wants the
grouping that suits it, and sharing one is a compromise none of them asked
for.

**A variable that did not pay stays visible.** A pattern of zeroes contributes
one bucket and costs nothing, so the declaration records what was tried, and
gives the optimizer somewhere to go:

     dig_a_col:  col,   1!0000000       # +0.27%, and worse in combination
     dig_a_bkq:  bkq,   1!0000          # 1.01% worse than the class it replaces

`IDX/idx2inc.pl`, `IDX/opt.pl`, `IDX/import.pl`, `IDX/IDX-FORMAT.md` and
`sh_mapping.inc` are Shelwien's, taken unchanged from the `psrc` tree apart
from `opt.pl`'s defaults and header, which name this program's corpus and
files instead of `bmf`'s.

`IDX/tsvcomp.inc` is the template. `MakeIndex dig_a,dig_b,dig_s,dig_m,dig_t`
in it becomes the statements that build all five, and `Table( cm_cnt,
%M%dig_A, %M%dig_a_Volume * TC_NODE )` becomes a member of the generated
`TC_T` struct sized by the product of that index's factors. `IDX/idx2inc.pl`
turns the two into `MOD/tsvcomp_h.inc` and `MOD/tsvcomp_p.inc`.

**MOD/ is a build input, not a build artefact.** It ships generated, in the
shipping form, so `make tsvcomp` needs no perl -- and a stale MOD/ compiles
fine and codes differently, so run `./mk.sh` after editing anything in `IDX/`.

### Why it is generated rather than parsed

The declaration could be read at startup instead, and that would be simpler.
It would also be useless, because of how the optimizer works.

`Debug` and `Const` are orthogonal flags, not two modes. With `Const 0` each
threshold is a live `mapping` object built from a pattern string; with `Debug
1` that string starts with `!MAP!`, and it is embedded in the executable:

    ./mk.sh
    mk.sh: tuning build -- 654 tunable bits in 112 patterns, visible to IDX/opt.pl

`IDX/opt.pl` does not parse or rebuild anything. It scans the binary for those
markers, flips their bits **in the binary**, re-runs the corpus and keeps what
shrinks it -- so one build serves an entire hill-climb, and a run is bounded
by how fast the corpus codes rather than by how fast it compiles. That is what
the format is *for*, and it is only available if the patterns are objects in a
compiled binary. The whole loop:

    ./mk.sh                                     # tuning build
    perl IDX/opt.pl opt.lst ./tsvcomp           # hill-climb; writes export.!!!
    cd IDX && perl import.pl tsvcomp.idx ../export.!!! > t && mv t tsvcomp.idx
    ./mk.sh check                               # then ship what it found

`import.pl` folds the winners back into the `.idx` by name, keeping the
comments -- so the annotations above stay attached to the patterns they
describe.

`./mk.sh release` derives the shipping build from the same source with one
substitution, `Const 0` becoming `Const 1` in a copy, so the two cannot drift.
There every threshold is folded:

    flr_a = flr_a*49 + TC_flr_a_col[__min(48,__max(0,col-(1-1)))];

**Both builds must produce identical streams**, and `./mk.sh check` is that
test -- the only one that catches a parameter which folds to something
different from what it evaluated to. It codes the same files under both and
compares byte for byte.

One consequence to be clear about: **the parameters are part of the format.**
Move a threshold and the same bytes mean something else, so a stream is
decodable by a build with the same `MOD/` and by no other. There is no
checksum in the stream to catch that, because the two builds have nothing in
common at runtime to compute one from -- the shipping build has no objects
left. Anything a mid-hill-climb binary writes should be thrown away.

## How the declaration was arrived at

In two stages, and they found different kinds of thing.

### By hand: which variables to offer at all

Sweeping one pattern at a time over three files, against a declaration whose
shape was chosen by hand. The lasting result of that stage is the variable
list -- what a family is allowed to condition on -- and one finding:

**The residue class is the largest single factor, and that was not the
expectation.** TSVTRANS.md measured the partition and the pass as the
coordinates that predict a digit, and they do -- but the class is *the choice
of ladder*, and the ladder decides what a digit can be. On that declaration,
sixteen buckets of it against four was worth 4.65%, and once it was in, the
frequency band it had been competing with all but disappeared: one bucket cost
0.05% and thirteen *cost* 0.10%. The partition had been standing in for the
class.

Two things measured useless in that stage, and both are worth naming because
the optimizer later contradicted both:

* `col`, the position inside a partition -- +0.27% alone in TSVTRANS.md, worse
  in combination, left at zero;
* `bkq`, the codebook's `off`, which is the half-width of its value range --
  the class in a form that would mean the same thing in the next file, where a
  class number means nothing. Substituted for `cls` it measured **+1.01%**.

### By opt.pl: how to arrange them

`IDX/opt.pl` on `00000000` and `ff_44100_q5`, 654 bits, to convergence -- 25
minutes, ending at 183,552 from 187,210, **−1.95%** on what it could see. On
the two corpora, of which it saw two files:

| | hand-set | after opt.pl | |
|---|---:|---:|---:|
| 17 libvorbis files | 12,117,933 | 12,075,279 | −0.35% |
| 22 ffmpeg files | 285,911 | 275,056 | **−3.80%** |
| tables | 40 MB | 13 MB | |
| `00000000`, coding | 0.36 s | 0.10 s | |

It improves 38 of the 39 files, including the 37 it never saw; the exception
is `00000007`, at +1.46%, which is the densest stream in either corpus and the
kind of file the tuning list should have had one of.

**What it actually did was swap the two counters' roles.** `dig_a` had been
the rich one -- the class at sixteen buckets, the neighbouring digits, the
packet-ago slot, 230,400 rows -- and `dig_b` the coarse fallback. It is now
the *structural* counter, on the pass, the band and the class, at 6,144 rows,
while `dig_b` is the *local* one, on the neighbouring digits, the column and
the zero run, at 62,208. One counter says where in the stream this digit is,
the other what has just been happening around it, and the mixer weighs them.

That is a decomposition, and a sweep cannot find one: it moves a variable at a
time inside the arrangement it is handed. It is also why the tables shrank by
two thirds and the program got three and a half times faster -- the split
model fits in cache where the single rich one did not.

Both of the hand-stage's dead ends come back under it. `col` is five
thresholds in `dig_b`, where it is not competing with the band. `bkq` is on in
four indices *alongside* the class rather than instead of it.

### What each variable is worth now

Same method, against the shipped declaration, on the same three files:

| off | coded | |
|---|---:|---:|
| nothing -- the shipped declaration | 1,079,958 | — |
| the neighbouring digits (`q1`, `q2`, `zrun`) | 1,161,225 | **+7.52%** |
| the same slot one packet ago (`t1`, `p1`) | 1,103,806 | +2.21% |
| the residue class | 1,091,073 | +1.03% |
| the column, in the floor model | 1,085,913 | +0.55% |

Compare that with the same table before tuning, where the class was +4.77% and
the neighbours +2.83%. Nothing about the stream changed; what changed is which
counter carries which, and the marginal value of a variable is a property of
the arrangement it sits in rather than of the variable.

**The floor column looks small and is not.** The floor is 3.5% of a coded
stream, so 0.55% overall is most of what the floor costs at all -- which is
TSVTRANS.md's −22.20% seen from the other side, against a model that already
has the previous post and the same post one packet ago to work with.

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
the shape of the table is the whole story -- same three files, only the digit
counters' limit and their width changing (measured on the hand-set
declaration, before `opt.pl` rearranged it; the cliff is a property of the
counter, not of the parameters):

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

13 MB of tables, 16 MB peak, and 0.10 s to code and 0.08 s to decode
`00000000`'s 1.27 MB TSV -- about 13 MB of TSV per second each way, against
1.6 MB/s for `xz -9e` on the same file, for 30% less output. Decoding costs
the same as coding, which is what a context-mixing coder is; `xz` wins on
decompression alone.

The tables are whatever the declaration asks for, and the arrangement the
optimizer found is small: 62,208 rows in the largest index where the hand-set
one had 230,400. That was not something it was asked to do -- `opt.pl` scores
coded bytes and nothing else -- but a model that fits in cache is faster, and
one whose rows are visited more often each is also better predicted. Both
followed from the same rearrangement.

## What it does not do

* **The tuning corpus was two files.** They were chosen to span the two
  encoders and both length regimes, and the result generalised -- 37 of the 39
  files it never saw improved. `00000007` did not, by 1.46%, and it is the
  densest stream in either corpus: a list with one of those in it would
  probably take that back and cost a little elsewhere. The run took 25
  minutes; a real one should be longer and wider.
* **There is no match model.** mp3c does not need one; a residue stream with
  long exact repeats -- silence, loops -- would reward one here.
* **The mixer has two inputs.** mp3c's has two as well, but its counters have
  a third and fourth context that this does not: `ctx2`, `ctx3` and three
  granules of history rather than one.
* **The codebook substitution that META.md describes is not applied.** A
  stream whose codebooks are one of the sets in `vbooks.inc` still pays for
  them record by record. That is a large win available on libvorbis-produced
  files and it belongs here rather than in the header model.
