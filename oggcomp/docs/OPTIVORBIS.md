# OptiVorbis, and what of it is useful here

A look at [OptiVorbis](https://github.com/OptiVorbis/OptiVorbis) -- what it
does, what it gets on our corpus, and which of its ideas carry over to a
byte-exact recompressor.  Checked at version 0.3.2 (commit `0ac0aec`,
2026-09-12; Rust; `AGPL-3.0-or-later OR BSD-3-Clause`), built with
`cargo build --release -p optivorbis_cli` and run over `testfiles/` with
oggcomp at commit `9ced78a`.

The short of it: OptiVorbis is a different tool solving a different
problem, and its one big trick -- rewriting the Huffman codes -- is
something an adaptive model already has for free.  What it does show,
by beating oggcomp on the smallest files, is that the setup header is
where oggcomp still pays for information it does not use: the codebooks
in every libvorbis file are libvorbis's own static tables, and a
dictionary of them would make the header nearly free.  That is the one
recommendation worth acting on; a fuzz harness and tolerance of
end-of-packet truncation are the two smaller ones.

## 1. What OptiVorbis is

An optimizer, not a compressor: it reads an Ogg Vorbis file and writes
another Ogg Vorbis file that decodes to the same samples and is smaller.
Lossless in the sample sense, not the byte sense -- the output has new
stream serials (randomised by default), new page boundaries, new CRCs,
recomputed granule positions, and by default the vendor string with
" OptiVorbis" appended (19 bytes on our files).  Comment fields are copied.

It works in two passes over each Vorbis stream.  The first parses the
setup header and decodes every audio packet far enough to count, per
codebook, how often each entry is used.  The second rewrites:

1. **Codeword lengths** are recomputed from the counts -- a Huffman code
   for the actual symbol frequencies (Moffat's in-place algorithm,
   `huffman_codeword_lengths.rs`).  Entries the audio never uses get
   length 0 (the sparse format), a trailing run of unused entries is cut
   off the book (`codebook_truncation.rs`, constrained by the residue
   classbook rule `classifications^dim <= entries` and by keeping the
   lookup-1 value count), and the smaller of the ordered and the
   unordered length format is chosen.
2. **Audio packets** are re-emitted with the new codewords, padding bits
   dropped.
3. **The container** is rebuilt: pages filled to the maximum, zero-length
   packets and non-Vorbis streams (Skeleton) dropped, granule positions
   derived from block sizes (`granulator.rs`), serials randomised.

The parser is deliberately tolerant, and fuzzing (there is an AFL target,
`scripts/afl-fuzz.sh`) shaped it: floor type 1 only; both lookup types
parsed, the VQ data merely copied; non-zero time-transform placeholders
ignored; `residue_begin > residue_end` treated as a zero-length residue
(upstream xiph/vorbis#87); a codebook count that overflows its byte
interpreted as 256; page CRC verification switchable off for repair;
and an audio packet that ends inside floor or residue decode is treated
as the specification says, as normal termination (`eval_on_eop` in
`audio_packet_common.rs`).  Its test corpus is ten small files: sine
waves from libvorbis and from aoTuV, uncoupled stereo, 6 channels,
chained streams, a Skeleton stream, a CRC mismatch, an Opus file, a
zero-byte last packet.

## 2. Measured

Fourteen corpus files, each run through OptiVorbis with default options
and through `oggcomp c` before and after.  Every OptiVorbis output was
decoded with `oggdec` and compared to the original's decode: samples
identical on all fourteen.

| file | original | OptiVorbis | % | oggcomp on original | % | oggcomp on OptiVorbis | % |
|---|---|---|---|---|---|---|---|
| music-stereo-q5 | 167,685 | 159,030 | 94.8 | 144,389 | 86.1 | 143,863 | 85.8 |
| music-managed-b96 | 101,549 | 94,227 | 92.8 | 84,604 | 83.3 | 84,080 | 82.8 |
| chirp-stereo-q10 | 130,545 | 120,886 | 92.6 | 116,294 | 89.1 | 115,243 | 88.3 |
| noise-stereo-q3 | 32,036 | 28,607 | 89.3 | 25,745 | 80.4 | 25,051 | 78.2 |
| bigcomment-8k | 136,732 | 135,047 | 98.8 | 113,705 | 83.2 | 112,777 | 82.5 |
| sine-stereo-q5 | 14,541 | 9,122 | 62.7 | 5,684 | 39.1 | 4,532 | 31.2 |
| sweep-mono-qm1 | 6,606 | 3,722 | 56.3 | 3,310 | 50.1 | 2,403 | 36.4 |
| uncoupled-stereo-q4 | 7,070 | 3,949 | 55.9 | 3,099 | 43.8 | 1,955 | 27.7 |
| mono-22k-q4 | 6,499 | 3,571 | 54.9 | 2,669 | 41.1 | 1,574 | 24.2 |
| multi6-48k-q4 | 13,855 | 7,359 | 53.1 | 6,803 | 49.1 | 3,789 | 27.4 |
| silence-8k-long-qm1 | 8,299 | 6,771 | 81.6 | 1,247 | 15.0 | 304 | 3.7 |
| tags-many-8k | 3,023 | 1,338 | 44.3 | 1,554 | 51.4 | 620 | 20.5 |
| tiny-8k-q0 | 2,763 | 1,078 | 39.0 | 1,346 | 48.7 | 415 | 15.0 |
| minbitrate-pad | 17,687 | 2,247 | 12.7 | 1,592 | 9.0 | 417 | 2.4 |
| total | 648,890 | 576,954 | 88.9 | 512,041 | 78.9 | 497,023 | 76.6 |

Three more files encoded with bitrate management (`-b 48 -M 48`, and
`-b 64 -M 64 -m 64`, the mode most likely to truncate packets), to see
whether managed streams change the picture:

| file | original | OptiVorbis | % | oggcomp | % | oggcomp on OptiVorbis | % |
|---|---|---|---|---|---|---|---|
| plain q5 | 86,022 | 81,010 | 94.2 | 73,346 | 85.3 | 72,786 | 84.6 |
| managed 48k max | 28,216 | 25,137 | 89.1 | 21,584 | 76.5 | 21,076 | 74.7 |
| managed 64k hard | 36,201 | 32,623 | 90.1 | 28,052 | 77.5 | 27,561 | 76.1 |

They do not.  Neither file contains a truncated packet (`-v` shows 2
bytes of raw), so libvorbis's bitrate manager did not need its
truncation on this material.

Where the difference between "oggcomp on original" and "oggcomp on
OptiVorbis" goes, from `oggcomp c -v`:

| stage | music, original | music, optimized | tiny, original | tiny, optimized | mono, original | mono, optimized |
|---|---|---|---|---|---|---|
| headers | 1,564 | 1,166 | 1,244 | 325 | 1,632 | 653 |
| pages | 440 | 403 | 25 | 29 | 65 | 66 |
| floor | 3,462 | 3,487 | 25 | 18 | 214 | 185 |
| class | 3,360 | 3,355 | 5 | 3 | 35 | 23 |
| digits | 135,522 | 135,411 | 38 | 30 | 709 | 632 |

Two things to read off this.

**The Huffman rewrite is worth nothing to oggcomp.**  On music the digits
stage moves by 111 bytes in 135 KB (0.08%) when every codeword in the
file has been re-optimised.  The model codes the symbol, not the
codeword, with adaptive probabilities that already track the frequencies
a static Huffman code can only approximate.  The 526 bytes music gains
are 398 bytes of header -- a smaller setup packet -- and 37 bytes of
pages.  (On the small files the digits do move: the books got smaller,
and a model over a smaller alphabet learns faster.  That is a property
of the input, not something the model can exploit on the original.)

**OptiVorbis beats oggcomp on the smallest files, and the reason is the
setup header.**  `tiny-8k-q0`: OptiVorbis 1,078 bytes, oggcomp 1,346;
`tags-many-8k`: 1,338 against 1,554.  The setup packet of these files is
2,476 bytes and oggcomp codes the three headers to 1,244 -- 93% of its
output.  OptiVorbis cuts the same packet to 816 bytes because, of the
3,169 codebook entries with a codeword, the audio uses 54; it marks the
rest unused and their lengths disappear.  A byte-exact coder cannot drop
them.  But it can know them, which is section 3.1.

## 3. What transfers

### 3.1 The codebooks are libvorbis's, and there are few of them

*Done since: tsvcomp, this program's ancestor, had already generated the
table (`vbooks_gen.inc`, 1,346 books in 350 rows from 35 encoders), and
`vb_dict.inc` now matches a setup's books against it bit for bit.  Every
book of every corpus file is in it.  The corpus went from 76.8% to 71.3%,
`tiny-8k-q0` from 1,346 bytes to 200 against OptiVorbis's 1,078, music
from 144,389 to 143,012.  The estimate below was written before that.*

libvorbis does not train codebooks per file.  Its encoder carries a fixed
set of static books (`lib/books/`) and a fixed setup for each of its
modes -- a sample-rate band, coupled or uncoupled or 5.1, a quality
level -- and every file it writes has a setup header assembled from
those.  The corpus shows it.  Over the 29 files in `testfiles/`: 663
codebooks, 248 distinct, 131 of them in more than one file, and 38.8 KB
of the 70.6 KB of book data is a repeat of a book seen earlier.  Over 75
parseable files (the corpus plus the generated channel and chaining
sets): 2,238 books, 284 distinct, 44 KB of distinct book bits in 234 KB;
22 distinct setup packets in 246 KB.

The header cache (`oc_hcache`) already exploits this *within* a run: the
second file with the same setup packet costs a flag.  The dictionary is
the same idea with the content built into the model: the static books,
keyed by a hash of their setup bits, and for each book in a setup packet
one symbol -- `cb.dict`: this book is dictionary entry *k*, or it is not
in the dictionary and its fields follow as today.  The decoder copies the
entry's bits.  Byte-exactness is untouched because the match is on the
bits themselves, and a book that differs in one bit is simply a miss.
Keying per book rather than per packet is what makes it robust: a
setup that mixes libvorbis books with a modified floor (aoTuV does
this; so would a tool that edits a header) still hits on every book it
shares.

What it is worth: the three headers cost 1.1 to 1.6 KB on every corpus
file.  With the books nearly free that becomes the identification and
comment packets plus the setup's floor, residue, mapping and mode
records -- a few hundred bytes at most, and less if the whole setup
packet is also in the dictionary (22 distinct in the corpus; a
`setup.dict` symbol in front of `cb.dict`).  On music that is 0.8% of
the output; on `tiny-8k-q0` it is the difference between 1,346 bytes
and about 200, three times better than OptiVorbis on the file OptiVorbis
wins today; on `oggdet` over a collection of short clips it is most of
the output.  The cost is the dictionary in the executable, on the order
of 100 KB for the complete libvorbis set (the corpus's 284 books are 44
KB uncompressed), gathered by running `oggenc` over its modes the way
`testfiles/gen.sh` already does rather than by copying libvorbis's
source.  Versions matter only as far as the books changed, which across
libvorbis 1.0 to 1.3.7 was rarely; aoTuV's books are its own and would
be a second set.  It is a stream format change (`OC_VER` 5) and the
dictionary becomes part of what `./mk.sh check` must hold constant.

### 3.2 Codeword lengths given the lattice

If a book is not in the dictionary its lengths are coded as they are
now: `cb.len` is the difference from the previous value under the tag,
in the context of the previous one or two values, the position, and the
previous value's low bits (`IDX/tsvcomp-hdr.idx`, index `a`) -- an
order-2 model over the length sequence that does not know what the
entry *is*.  For a lookup-1 book it could: the entry's lattice vector
follows from its index and the VQ fields, and libvorbis's trained
lengths follow the value's probability, short near zero and long at the
edges.  The VQ fields come after the lengths in the packet, so the coded
order would have to differ from the bit order -- code `cb.lookup`, the
min, delta, value bits and multiplicands first, then the lengths, and
have the decoder assemble the book before emitting its bits in
specification order.  The encoder has the whole packet and needs no
change of structure.

A first look, with a crude adaptive count estimate per book (not the
model, which is 20% better than the plain order-0 estimate here):

| file | `cb.len` values | order-0 | order-1 | squared-norm context | norm and order-1 |
|---|---|---|---|---|---|
| tiny-8k-q0 | 3,216 | 1,252 | 1,266 | 1,059 | 1,339 |
| mono-22k-q4 | 4,289 | 1,686 | 1,693 | 1,493 | 1,783 |
| music-stereo-q5 | 11,813 | 1,840 | 1,644 | 1,849 | 1,921 |
| multi6-48k-q4 | 13,711 | 4,444 | 3,981 | 4,428 | 4,597 |

The norm alone is worth 11 to 15% on the small files' books and nothing
on the large ones, where the previous length is the better predictor;
the joint context dilutes at these counts, which is what the model's
mixing is for.  Ten to fifteen percent of `cb.len` is 100 to 150 bytes
on a file where the header is half the output, and nothing where it is
not.  Worth doing only as the miss path of 3.1, and after it.

### 3.3 Audio packets that end early

The specification allows an audio packet to end inside the floor or
residue decode: the decoder treats the end of the packet as the end of
the data and the missing values as zero (Vorbis I, 4.3.1 and 8.6.2).
OptiVorbis honours it.  oggcomp's packet reader refuses it -- `bget` in
`vb_packet.inc` says "packet ends inside a field", and a refusal in the
dry run means the whole page is coded as bytes.  So a stream with such
packets loses a page's worth of modelling for each one.

libvorbis's bitrate manager truncates a packet that would break a hard
maximum (`oggpack_writetrunc` in `lib/bitrate.c`), but on the two
managed files above it did not have to, and none of the corpus has one
either; other encoders and edited streams are where they would come
from.  Supporting it is small:
the decoder knows the packet's byte length from the lacing values
(coded in the pages stage before the packet), so it knows when the bits
run out at exactly the point the encoder did.  The reader stops there
instead of refusing, the values that were never in the packet are not
coded, and the tail that is a prefix of a codeword -- at most 31 bits
-- is coded raw, with one flag per packet saying whether it happened.
The gain on the corpus is zero; the gain is in the promise "what the
Vorbis parser can place is modelled" holding for every stream a
conforming decoder plays.

### 3.4 The other tolerances

OptiVorbis parses several things oggcomp refuses (and so codes as bytes,
a page at a time): lookup type 2 books, non-zero time-transform
placeholders, zero-dimension books, `residue_begin > residue_end`.  No
encoder in use writes any of them -- libvorbis reads type 2 but never
writes it -- and OptiVorbis has them because fuzzing found them.  Each
is a few lines and each is worth nothing on real files.  Not
recommended except as by-products of 3.5.

### 3.5 Fuzzing

OptiVorbis's fuzz target found real defects: the memory blow-up on a
4096-entry 16-dimension book, the codebook count overflow, the residue
range inversion.  oggcomp has never been fuzzed.  `oc_api.h` is the
harness it needs: an in-memory entry point with no files, one instance,
`oggcomp_Loop` until done.  Two targets -- `oggcomp c` on arbitrary
bytes, which must never crash or hang (it does not refuse input), and
`oggcomp d` on a valid `.oc` with bytes flipped, which must refuse
cleanly (the over-read cap `RC_OVER_MAX` and the end CRC are what stand
between a corrupt stream and a wild decoder).  Their `scripts/afl-fuzz.sh`
is a usable template; libFuzzer over the API is the smaller job.
Cheap, and the kind of thing that finds a bug the corpus never will.

### 3.6 Granule positions

OptiVorbis derives every granule position from the block sizes: the
first audio page's is fixed by the samples decoded so far, each later
one adds the overlap of consecutive blocks, the last may be short by
the encoder's trailing truncation (`granulator.rs`).  The pages stage
is 0.3% of oggcomp's output on music (440 bytes, of which the granule
is part) and 2 to 4% on the small files, so however much of it a
block-size prediction could save is bounded by that.  Low priority.

### 3.7 Their corpus

Two of their ten test files cover cases ours does not: a sine encoded by
aoTuV, whose books are not libvorbis's (the miss path of 3.1 wants
exactly this), and a stream whose last packet is zero bytes.  Both are
trivially regenerated -- aoTuV's `oggenc` is a build away, and a
zero-byte last packet is a small edit -- which is better than copying
files whose licence is the repository's dual licence rather than the
corpus's own.

## 4. What does not transfer

- **The Huffman rewrite.**  0.08% on the digits stage, measured above.  A
  two-pass count-then-code design is what an adaptive coder replaces.
- **Dropping unused entries and truncating books.**  Not permitted
  byte-exact; the dictionary reaches the same bytes by knowing them
  instead of deleting them.
- **Page packing, padding removal, serial randomisation, stripping
  Skeleton, zero-length packets.**  All must be reproduced, and all are
  already cheap: pages are 0.3%, `packets` is 0.02%, and Skeleton goes
  through the raw stage at a few bytes per page of overhead.
- **Vendor string and comment actions.**  Same.
- **The tolerant parser as a whole.**  oggcomp's answer to a stream it
  cannot place is to code it as bytes, which is safer than guessing what
  a decoder would do and needs no per-quirk decision.

## 5. In order

1. **The libvorbis codebook dictionary** (3.1): about 1 KB off every
   file, three times better on the smallest ones, most of the output of
   `oggdet` over short clips.  Moderate work: a generator, a hash table
   keyed by book bits, a `cb.dict` symbol, `OC_VER` 5.
2. **A fuzz harness over `oc_api.h`** (3.5): small, and the only item
   here about correctness rather than size.
3. **End-of-packet termination** (3.3): small; closes a gap between the
   parser and the specification.
4. **Value-conditioned `cb.len`** (3.2): 10 to 15% of a small file's
   book lengths, for books the dictionary misses.  After 1.
5. **Granule prediction** (3.6): bounded by the 0.3% pages stage.
