# tsvtrans: a record stream rearranged for an entropy coder

    tsvtrans c [options] input.tsv output.tsv
    tsvtrans d [options] output.tsv restored.tsv

Nothing is compressed here. `tsvtrans` reads a balrogg record stream and
writes another one, and mode d turns it back byte for byte. What it does is
hand whatever coder comes next a stream with less in it and a shape that coder
can address:

* **values the stream already implies are dropped**, and mode d works them out
  again -- the page header's type, sequence and serial, the granule, and the
  window flag that names the block before this one;
* **the floor posts and the residue digits go out in rows of a fixed width**
  -- one floor curve to a row, one residue partition to a row -- under a tag
  that names which floor, and which residue and pass, they belong to.

Options, which mode d must be given too:

| | |
|---|---|
| `-P` | keep the page header as it stands |
| `-W` | keep `aud.wprev` rather than deriving it |
| `-R` | keep the floor and digit rows as they came, ragged |
| `-n` | all of the above: copy the stream through unchanged |

## Why the rows are ragged to begin with

balrogg merges consecutive records carrying the same tag onto one line, and
the run length is whatever the packet happened to need. On `00000000` the
residue digits arrive 16, 32, 64, 112, 128, 144, 160, 992, 1024 and 1056 to a
row, because a run ends wherever a `res.class` record interrupts it. Its two
residues have partitions of 16 and 32, so most of those rows are several
partitions run together -- and which residue, which pass and where one
partition ends are all invisible.

Under `tsvtrans c` a row is one partition and the tag names the rest: `g0_0`,
`g0_1`, `g0_2` are residue 0's three passes at 16 values a row, `g1_0` to
`g1_2` are residue 1's at 32. Floor curves arrive `posts` to a row under `y0`
or `y1`, one tag per floor, so a row's width no longer depends on which floor
wrote it.

## What that is worth, and to whom

**It depends entirely on the coder, and the two answers point opposite ways.**

To `xz`, which has no notion of a column, the transform is a loss. It pays for
the extra tags -- a 1,056-value row becomes 66 rows and 66 tags -- and gets
nothing back:

| | tsv | tsvtrans | |
|---|---:|---:|---:|
| 17-file corpus | 16,860,676 | 17,974,192 | **+6.60%** |
| 22 ffmpeg files | 381,424 | 400,592 | **+5.03%** |

To a coder that conditions on the tag and the column, the transform is a
gain. Running the *same* adaptive model over both files -- context is the
tag, the column, and the previous value, and the row framing is charged for
too, so the transformed stream pays for its extra tags -- gives:

| | tsv | tsvtrans | |
|---|---:|---:|---:|
| `00000000` | 178,630 | 176,346 | −1.28% |
| `00000003` | 241,161 | 238,255 | −1.21% |
| `00000005` | 113,681 | 112,277 | −1.24% |
| `ff_44100_q5` | 12,468 | 11,877 | −4.74% |
| `ff_8000_q5` | 7,133 | 5,911 | −17.13% |
| `ff_48000_q10` | 21,490 | 20,255 | −5.75% |
| **total** | **574,563** | **564,921** | **−1.68%** |

The ffmpeg files gain most because they carry more residues and more passes
per stream, so there is more for the tag to separate; the libvorbis files
have two of each and gain about 1.2%.

Splitting that between the two halves of the transform, on the same file:

| | coded size | |
|---|---:|---:|
| unchanged (`-n`) | 178,630 | |
| implied values dropped (`-WR`) | 178,604 | −0.01% |
| fixed rows only (`-PW`) | 176,388 | −1.25% |
| both | 176,346 | **−1.28%** |

So the rows are the win and the removals are close to free -- they take 2.4 KB
off the raw stream and cost nothing, but there is not much of the page header
to remove in the first place.

## Which coordinate actually predicts a digit

The rows were made fixed-width so that a coder could use the column. Measured
over the 434,720 residue digits of `00000000`, the column is the one
coordinate that does **not** help. Each row is an adaptive model conditioned on
what the heading names, against an order-0 model of the same digits:

| context | coded size | |
|---|---:|---:|
| order 0 | 178,694 | — |
| column within the partition | 179,176 | **+0.27%** |
| partition (frequency band) | 175,478 | −1.80% |
| pass | 174,412 | −2.40% |
| previous digit | 173,834 | −2.72% |
| **partition + pass** | **170,191** | **−4.76%** |
| partition + pass + previous | 175,587 | −1.74% |
| partition + pass + previous + column | 220,798 | +23.56% |

A partition is a contiguous band of one spectrum, and there is no reason the
fourth coefficient in it should behave differently from the eleventh -- so the
column carries nothing, and adding it to a context that was working only
divides the statistics. What does carry is *which* band and *which* refinement
pass, and that is why the digit rows are tagged `g<residue>_<pass>` rather
than by residue alone: the pass is in the tag, and consecutive rows within a
pass are consecutive bands, which a coder can count.

The floor is the other way round. There the column is the whole story:

| context | `y0` (19 posts) | `y1` (29 posts) |
|---|---:|---:|
| order 0 | — | — |
| previous post | +0.28% | +7.26% |
| **column** | **−22.20%** | **−8.86%** |

which is what a floor1 curve looks like: post 0 and post 1 are coded raw and
land at the two ends of the row, and everything between them is a folded
residual against a prediction, at a frequency the column names. The two floors
of this stream are 19 and 29 posts wide, and separating them into `y0` and
`y1` is most of what makes that column meaningful.

## Verification

* 39 files -- the 17-file corpus and the 22 ffmpeg files -- through `c` then
  `d`: byte-exact on all 39, and byte-exact under `-n`, `-P`, `-W` and `-R`
  singly on `00000000`.
* `make test-trans` runs `balrogg c`, `tsvtrans c`, `tsvtrans d` and compares.
* The first version was exact on seven of ten corpus files and failed on the
  three that have packets spanning pages. `page.spill` is read while the tee
  that copies the rest of the stream is attached, so it was written twice --
  once by the tee and once by the reader that needed its value. A stream whose
  packets each fit in one page never reaches that line.

## Build

    make tsvtrans
    make test-trans
    make test-trans TESTFILES=dir/

Two environment variables exist for measuring rather than for the format:
`TSVTRANS_DUMP=path` writes every digit's coordinates (residue, pass, channel,
partition, column, value) to a side file, which is what the tables above were
measured from, and `TSVTRANS_TRACE=1` prints the page and packet the walk is
on, which is how the `page.spill` fault above was found -- the two directions
trace identically until they disagree.
