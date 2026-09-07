# Reproducing the floor with libvorbis's own psychoacoustic model

`fit.c` links libvorbis's encoder internals and runs its psychoacoustic
analysis and `floor1_fit` over blocks whose sizes *we* impose, rather than
letting the encoder choose them. That removes the block-alignment problem: a
plain re-encode picked 415 blocks where the stream had 877, so no two floors
could be compared. Driving the internals directly, every block corresponds by
construction.

    ./fit <quality*10> <rate> <channels> <dump> [blocktype]

Input is the dump `tsv2wav` writes under `TSV2WAV_DUMP`: per block the block
size, the floor's post list, the **unwrapped** posts, and the windowed PCM the
block spans.

## Results

`mono_q2.ogg`, whose encoder and setting are known and reproducible:

| quality | exact | within +-1 | mean \|dY\| | H(dY) |
|---:|---:|---:|---:|---:|
| 0.1 | 0.9% | 5.5% | 3.36 | 2.354 |
| **0.2** | **48.4%** | **82.2%** | **1.17** | **2.471** |
| 0.3 | 38.2% | 76.3% | 1.30 | 2.667 |

The sweep has a sharp maximum at 0.2, which is the quality the file was
actually encoded at, so the encoder setting can be *identified* from the stream
rather than guessed, and carried in the meta as a single index.

`00000000.ogg`, whose encoder is not this libvorbis (no setting reproduces its
codebooks): best at quality 1.0, blocktype 0 -- 41.7% exact, 62.9% within one
step, H(dY) = 2.973 bits against a 3.37-bit cost basis for the coded `flr.y`
as stored. Even with the encoder mismatched, the correction is cheaper than the
value.

Blocktype makes little difference (48.4% vs 48.9% on `mono_q2`, and 41.7% vs
28.6% on `00000000`), so blocktype 0 is a reasonable fixed choice.

## Build

    curl -O https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
    curl -O https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz
    # configure --disable-shared both, install to $PREFIX
    gcc -O2 -o fit fit.c -I$VORBIS_SRC/lib -I$PREFIX/include \
        $PREFIX/lib/libvorbisenc.a $PREFIX/lib/libvorbis.a $PREFIX/lib/libogg.a -lm

Every internal needed resolves from the static library: `_vp_psy_init`,
`_vp_noisemask`, `_vp_tonemask`, `_vp_offset_and_mix`, `floor1_fit`,
`mdct_forward`, `drft_forward`.

## Two bugs that made earlier attempts look hopeless

Both were the same mistake in different clothes, and both are worth
remembering because each one made a working idea look dead.

**Comparing coded values against absolute posts.** `flr.y` in the stream is a
residual against the neighbour interpolation and is zero at more than half the
posts; `floor1_fit` returns absolute post values. Comparing the two gave a
uniform offset of about -68 -- exactly the typical absolute post value -- and a
mean \|dY\| near 70. `floor_only` now returns the unwrapped posts for the dump.

**`global_ampmax` passed as the block's own peak.** libvorbis carries it across
the stream, attenuated by `ampmax_att_per_sec` each block, and `_vp_tonemask`
takes both it and the local peak. The harness now tracks it the same way.

## Improving on the raw fit

`analyse.py` scores predictors on the `fit` dump, learning on the first half of
the blocks and reporting on the second, so none of these are in-sample numbers.

| predictor | mono_q2 | 00000000 |
|---|---:|---:|
| cost basis: coded `flr.y` as stored | 3.22 | 3.37 |
| psy fit alone | 2.475 | 2.899 |
| + per-post-index bias | **2.024** | 2.950 |
| + neighbour-residual prediction | 3.031 | **2.363** |
| + both | 2.562 | 2.363 |
| saved | **37%** | **30%** |

Two things are worth noting.

**The neighbour-residual predictor is free.** Post *i*'s low and high
neighbours are always decoded before it, so the decoder can form
`0.5*(r[lo] + r[hi])` from residuals it already has. No side information. On
`00000000` the residual correlates +0.754 with that quantity, which is the
signature of a slowly varying level error -- exactly what an encoder mismatch
produces. Removing it recovers most of the gap.

**The two files want different predictors**, and each one *hurts* on the other
file. So neither can be applied unconditionally: mode `c` has to score both and
store which it chose, the same adaptive pattern the class-metric fit already
uses. The per-post bias is roughly 19-29 integers per floor; the neighbour
predictor is a single flag.

One tweak that does *not* help: rounding the fit to post units instead of
truncating. libvorbis truncates (`val >>= 3` for mult 2), and matching that is
worth 10 points of exact match on `mono_q2` (48.4% against 38.8%). The
truncation is not a rounding error to be corrected, it is part of the encoder.

## What integration would require

`tsv2wav` would link libvorbis and run this analysis in both `c` and `d`, mode
`c` sweeping quality and storing the winning index plus the `dY` corrections.
The two sides must agree bit for bit, so both would have to link the same
libvorbis build -- a real constraint the current self-contained tool does not
have.

## Codebooks are not generated, they are chosen

libvorbis does not build codebooks at encode time. `lib/books/` holds several
hundred `static_codebook` tables, marked "autogenerated by huff/huffbuld" --
built offline, once, and shipped. `vorbis_encode_init_vbr` picks a setup
template by quality and sample rate, and that template just points at them:

    ci->book_param[ci->books++] = (static_codebook *) books[x[is]][i];

So a stream produced by libvorbis carries codebooks that are *exactly* one of
the built-in sets. They can be reproduced from a quality index instead of
being transmitted.

`cbdump.c` prints every codebook of a given (channels, rate, quality) in a form
`cbmatch.py` can compare against a balrogg record stream, field for field:
dimension, entry count, a hash of the whole length list, the lookup type and a
hash of the quantisation list.

Across the 26-file corpus, **21 files reproduce their entire codebook set**:

| files | result |
|---|---|
| 15 testfiles at 48 kHz mono | all 44 books match at q=0.9 |
| big_meta, ch3, ch5, ch8, lo8k, mono_q2, tiny | match at q=0.4, 0.6, 0.3, 0.2, 0.0, 0.2, 0.5 |
| 00000007, cbr45, ch6, st_q0, st_q10 | no VBR setting matches |

The five misses are worth chasing: `cbr45` is constant-bitrate, so it comes
from `vorbis_encode_init` rather than the VBR entry point, and the others may
be managed-bitrate or a different libvorbis release.

Both guesses turned out to be right; see *One library is not enough* below.

This corrects a claim made earlier in this work -- that no encoder setting
reproduces the corpus files' codebooks. That was wrong. It was concluded from
a full-stream re-encode, where a mismatch anywhere looks like a mismatch
everywhere; comparing the codebooks alone shows they match exactly.

Codebooks are 18.6% of the meta. Replacing them with a quality index needs the
records rebuilt in balrogg's exact form. The ordered/sparse choice for the
length list looked at first like something only the bitstream knew, needing two
bits per book alongside the index; it is not. `vorbis_staticbook_pack` decides
it from the length list itself, and `vbooks.inc` makes the same decision, so
the index carries everything.

## One library is not enough

The misses above were not noise. Vendor strings name their encoders exactly:
`00000007` is `Xiphophorus libVorbis I 20010813`, which is 1.0rc2, and the two
`Sony Ogg Vorbis 1.0 Final` files are `Xiph.Org libVorbis I 20020717`, which is
1.0's literal `ENCODE_VENDOR_STRING`. Those releases' tables are not the ones
compiled in: 1.0 differs from 1.3.7 in 319 of 790 shared arrays, with 32 gone
and 446 added.

So the sets of every released libvorbis are carried as data instead. Two axes
have to be swept, not one:

* **quality**, through `vorbis_encode_init_vbr`, which is what a VBR file used;
* **nominal bitrate**, through `vorbis_encode_init`, which selects the setup
  template through `rate_mapping` rather than `quality_mapping` *and* turns
  bitrate management on -- and a managed encode takes its residues from
  `books_base_managed`, a family no quality setting ever reaches.

That second axis is what `cbr45` needed, and it is also the only axis 1.0rc2
has: rc2 declares `vorbis_encode_init_vbr` but its body is `return OV_EIMPL`.

`bookgen.c` sweeps both over channels 1..8 and the whole rate range, bisecting
each template boundary so the table carries the exact rate libvorbis switches
at. `bookdump.sh` runs it once per release -- two libvorbis copies cannot share
an address space -- and `bookmerge.py` interns the books by content and unions
the rate ranges of every release that agrees on a set.

Over 19 libvorbis releases (1.0beta4 through 1.3.7) and 15 aoTuV betas: **1318
distinct codebooks in 349 sets** -- 1346 in 350 once ffmpeg's own encoder is
added below. The aoTuV betas add *nothing* -- not one book,
not one set. aoTuV replaced the setup templates, and its own template choices
are already reachable from the same tables by some (channels, rate, setting)
libvorbis itself can be asked for.

Against the 17-file corpus this names the codebooks of every file, `00000007`
included, and costs 0.40% of the meta's values and 0.47% of its bytes.

## The one encoder that is not libvorbis

`ffmpeg -c:a vorbis` is ffmpeg's own encoder, not `libvorbis`, and it does not
choose its codebooks from anything -- it has one set of 29 in
`libavcodec/vorbis_enc_data.h` and writes it into every file, whatever the
sample rate or the quality. It refuses to encode anything but stereo. So the
whole encoder is a single row, and `ffbookgen.c` builds against an ffmpeg
checkout to produce it, reproducing `create_vorbis_context` and the two derived
fields `put_codebook_header` writes: `q_min`/`q_delta` are its `put_float` of
the `cvectors` floats, `q_quant` its `bits`.

The one thing that had to be checked rather than assumed: **ffmpeg decides a
book is "ordered" on a weaker test than libvorbis.** libvorbis also disqualifies
a zero length; ffmpeg only requires the lengths not to descend. `vbooks.inc`
emits libvorbis's decision, so a book the two rules disagree about would be
found and then fail to match. On these 29 they agree everywhere, so nothing has
to be carried to say which packer wrote the stream.

`fftest.sh` synthesises stereo sources and encodes 22 files across seven sample
rates and four qualities. Before, every one of them fell through to raw
codebooks; after, all 22 name the set and all 22 still round-trip bit-exactly:

| | values | bytes |
|---|---:|---:|
| before | 701,613 | 1,748,707 |
| after | 450,153 | 1,149,911 |
| | **-35.8%** | **-34.2%** |

Every file saves the same 11,430 values -- the fixed set, removed once. The
percentages are large because these are six-second files, where 29 codebooks
are most of the meta; on a long file the same 11,430 would disappear into the
noise. It is a fixed cost, and it is now zero.
