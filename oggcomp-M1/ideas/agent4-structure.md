# Structure, order and cross-stream information

Angle: what is in the *arrangement* of an Ogg Vorbis file (and of a container
of them) rather than in its symbols, and what could be coded in a different
order or grouping so that the decoder knows more at the moment it needs it.

Everything below is grounded in this tree at the current commit.  Where I say
"measured" I ran the shipping `./oggcomp` / `./oggdet` in the tree; the scratch
corpus and scripts are beside this file (`clips/`, `pg.py`, `c*.bin`, `gap.bin`,
`t*.bin`).  A summary of those runs:

| # | what | result |
|---|---|---|
| M1 | 40 clips (20 tonal, 20 noise) q3, `-S`: grouped by kind vs alternating | 83,619 vs 84,168 B (**0.65%**) |
| M2 | 120 clips (40 sources x q0/q3/q6), `-S`: grouped by quality / same-source adjacent / shuffled | 274,138 / 274,458 / 276,384 B (**0.82%** for grouping; **0.00%** for source adjacency) |
| M3 | 48 clips over 3 rates x 2 channel counts, `-S`: grouped vs shuffled | 27,818 vs 28,235 B (**1.48%**) |
| M4 | marginal cost of a 2nd stream in a 2-stream `-S` archive, as % of its cost alone | identical 65%, trimmed 70%, **unrelated 75%**, half-gain 77%, faded 82% |
| M5 | 8 streams, 3,000-byte shared comment field, TITLE differing vs identical | 20,636 vs 6,927 B; `xz -9e` 5,776 B |
| M6 | container 85% non-Ogg text | `oggdet -S` 150,332 B vs `xz -9e` 88,684 B; gap bytes at 4.40 bits/byte |
| M7 | 120-stream `-S` archive: framing + metainfo | 12 B/stream + 1,002 B of coded metainfo for 401 B of content = **~1.0%** of output |
| M8 | same container, `-c` vs `-S` (cold start) | 309,568 vs 274,138 B (**11.4%**) |
| M9 | memoryless entropy of page fields, `music-stereo-q5` | plen 316 B, granule delta 14 B, npkt 12 B, against a 440-byte `pages` stage |

Two facts from those runs shape the whole list.  **The model has no
cross-stream *content* channel at all** (M4: a gain, fade or trim variant of a
stream costs the same as an unrelated stream of the same family; only a
byte-identical repeat is cheaper, and then only through context memorisation).
**What the model does carry across streams is statistics**, so grouping streams
by the parameters that set those statistics is worth 0.6-1.5% (M1, M2, M3) and
costs a permutation.  And the header cache is all-or-nothing on whole packets,
so one differing character in a comment header re-codes every shared byte in it
(M5).

---

### 1. Field-level and blob-level header sharing, and a parsed comment header

- **Idea:** `oc_hcache` (oc_model.inc) matches a *whole* comment or setup
  packet; a comment header that differs in one byte is a miss and every byte of
  it goes through `t.hdr("cmt.byte", ...)` again (vb_packet.inc `comment()`).
  Parse the comment header instead -- vendor length, vendor string, count, then
  `length, "KEY=value"` -- and cache at the *field* level: a vendor string, a
  key, a whole `KEY=value` string seen before costs an index.  Detect the
  base64 `METADATA_BLOCK_PICTURE` payload, decode it to bytes before coding
  (4/3 off before anything else models it) and cache the blob whole.  Code the
  remaining text through `oc_raw`'s byte model rather than through `fam_hdr`,
  which is a value model keyed by tag id and is not a text model.
- **Why it might work:** M5: eight streams whose comment headers differ only in
  `TITLE=track0..7` cost 20,636 bytes solid against 6,927 when the headers are
  identical -- 1,958 bytes, 5.2 bits per byte, per re-coding of a 3,036-byte
  header the model had already seen seven times.  `xz -9e` gets the same
  container to 5,776, i.e. **3.6x smaller than oggcomp**, purely because it
  finds the repeat.  A real album is exactly this shape: identical ALBUM,
  ARTIST, DATE, and 50-500 kB of identical cover art, with TITLE and
  TRACKNUMBER differing.  `oggdet`'s `oggart.inc` carves *recognised* pictures
  out and dedups them byte-identically, which covers one case and not the rest;
  plain `oggcomp c` on a chained album or a tagged single file covers none
  (`art-8k.ogg`: 2,286 of 2,380 output bytes are `headers`).
- **Expected gain:** nothing on the untagged corpus; **40-70% on a chained or
  carved album with art and per-track tags**; ~0 on music-only files.  High
  confidence in the direction, because it is measured; the size depends
  entirely on how much tag payload a file carries.
- **Cost:** moderate.  A comment-header parser (~60 lines; the format is
  trivial and length-prefixed), a second cache keyed by string, a base64
  decoder, a new tag or two.  No per-symbol CPU on audio.  Stream format
  change: yes, `OC_VER`.  No loss of random access.
- **Risks:** a malformed comment header must fall back to the present
  byte-at-a-time path, which means the parse has to be as forgiving as
  `page_fits` is -- dry-run it and take the byte path on refusal.  Base64 that
  is not canonical must round-trip (pad, whitespace, case) or the decode must
  be refused.  The field cache grows the 4 MB `OC_HCMEM` budget; cover art
  alone can exceed it, so the blob cache wants its own accounting.
- **Measure first:** take a real tagged album (or M5's `tdiff.bin`), run
  `oggdet -S -c`, and compare with `xz -9e` on the same bytes.  The gap is the
  upper bound and it is already 3.6x on the synthetic.  Cheaper still: count
  the bytes of comment header per stream that are byte-identical with some
  earlier stream's comment header -- a ten-line script -- and multiply by the
  5.2 bits/byte measured above.

### 2. Hoist every residue class ahead of every digit, and give the digit model forward class context

- **Idea:** In `io_t::residue()` (vb_packet.inc) classes are *not* all coded
  before the digits: the loop codes `pv = classbook->dim` classes per vector,
  then the digits of those `pv` partitions, then the next `pv`.  In libvorbis's
  stereo residue books the classbook is dimension 2 with `nclasses^2` entries,
  so classes arrive two partitions at a time while `np` is 30-64.  Code all
  `vch * np` classes of a residue first (the encoder buffers the classwords it
  already reads with `bk_get`, the decoder emits them into the packet at the
  end), then all the digits.  Then add to `tc_dv` the variables that only
  exist once this is done: the class of the next partition and the one after
  (`cn1`, `cn2`), the forward run length (how many partitions until the class
  changes), the number of remaining partitions in this vector whose book is not
  -1, and the mean class over the rest of the vector.
- **Why it might work:** the class *is* the encoder's own statement about the
  partition's energy, and `cls`, `crun`, `cm` already show it is one of the
  strongest digit contexts backwards.  Forwards it says where the spectrum is
  going: a digit near the end of a run of a loud class, with a quiet class
  next, is on a falling edge and is smaller than the run's average.  The floor
  model already exploits exactly this asymmetry -- `vd_render(lo, hi, ...)` in
  oc_floor.inc predicts a post from neighbours on *both* sides -- and the digit
  model has nothing forward within the packet at all (`n1` is the *previous*
  packet at `slot + str`).  The reorder itself costs nothing: the classes are
  coded in the same order relative to each other, only earlier relative to the
  digits, and `classify()`'s own contexts do not read digits.
- **Expected gain:** digits are 94.8% of output on music, and the class axis is
  one of the ten indices `tc_make_dig` builds.  I would put a forward class
  pair at **0.3 to 1.5% of the digit stage** on music, more on material with
  sharp spectral edges (the chirp and sweep files), near zero where the class
  never changes (silence, `uncoupled-stereo-q4` codes classes at 0.10 bits
  each).  Medium confidence.  Note this is the only idea in the list that
  attacks the 95%.
- **Cost:** the encoder must read a vector's classwords ahead -- the same
  refactor IMPROVEMENTS 7.4 priced for the run-length experiment, and cheap
  because `cl[]` already holds them.  Two to four more inputs to
  `tc_make_dig`, so a few percent on the 24% that `tsvcomp-dig_p.inc` costs.
  Memory: none new (`vb_cs` already sizes for it).  Format change: yes.  No
  loss of random access.
- **Risks:** the class may already be saturated as a context -- 7.4's finding
  that `zrun` and `vpos` were noise because "the codebook prior is already
  doing what those axes would" is the cautionary tale, and the codebook prior
  does know the class (it *is* the book).  What the prior does not know is the
  *next* partition's book, so this is not the same axis, but the gain could
  still be small.  If `pv` happens to equal `np` for a file, the reorder half
  is a no-op there and only the new variables matter.
- **Measure first:** no code change needed for the estimate.  Instrument
  `digit()` behind `tc_verbose` to log `(cls, crun, cm, band, pass, q1, t1,
  zero?)` plus the *next* partition's class, dump it for
  `music-stereo-q5.ogg`, and compute the conditional entropy of the zero
  decision with and without the forward class -- exactly the table in
  IMPROVEMENTS 7.4.  A gap under 0.5% there means drop it.

### 3. Order the streams of a container by similarity before coding solid

- **Idea:** `oggdet -S` codes streams in the order they appear in the
  container.  Have the carver compute a cheap per-stream key -- sample rate,
  channel count, block sizes, the identification header's bitrate fields, the
  setup-header hash, and a coarse content fingerprint such as the mean class
  per band over the first few packets -- sort or greedily chain the streams by
  it, code them in that order, and record the permutation in the metainfo
  (the record already exists per stream; one varint each, ~1.5 bytes).
- **Why it might work:** solid mode's whole benefit is that the tables are warm
  (M8: 11.4% here, 4% on the real 1,257-clip set), and a warm table is warm for
  the *last* streams' statistics.  Measured directly: grouping 40 clips by
  tonal/noise beats alternating them by 0.65% (M1); grouping 120 clips by
  encoder quality beats a shuffle by 0.82% (M2); grouping 48 clips by sample
  rate and channel count beats a shuffle by 1.48% (M3).  Those are 2, 3 and 6
  classes; a real container (a game's asset pack, a music library, a disk
  image) has many more, and the gain from ordering in solid archivers grows
  with the number of classes.
- **Expected gain:** **0.5 to 2% of a container's output**, from the three
  measurements above; more on a heterogeneous container, zero on a container
  of one kind of stream.  Nothing on a single file.  High confidence -- this is
  the best-measured idea here and needs no model change at all.
- **Cost:** low.  A sort in the carver, a varint per stream in the metainfo,
  and a restorer that writes segments out in the recorded order.  No per-symbol
  CPU.  Archive-format change: yes (metainfo v7).  Random access is unaffected
  for `-c`; for `-S` it is already front-to-back.  The carver must hold the
  stream extents before coding, which it already does per run -- but it must
  now buffer or re-read them, so `-c` to a pipe loses this (as it already loses
  dedup).
- **Risks:** the fingerprint could sort on the wrong axis and lose; M2's
  third arm is the warning -- sorting the same 120 clips so that the three
  quality encodes of one source are adjacent was *worse* than sorting by
  quality.  So sort by encoder parameters first and content second, and keep
  file order as a fallback the encoder picks when it estimates the reorder does
  not pay.
- **Measure first:** already done (M1-M3).  The next step is one real
  container: carve it, sort the segment files by the key, `cat` them in both
  orders, and run `oggdet -S -c` on each.  Ten minutes, no code.

### 4. Code a page's payload before its header: derive the lacing, the granule and the page break

- **Idea:** `oc_frame.inc::page()` codes type, granule, serial, sequence, the
  packet count and every packet length *before* the payload.  Turn it inside
  out: code the packets of a link as one continuous sequence, and after each
  packet code one binary "does a page end here" symbol plus, when it does, the
  page's residue fields.  A packet's length then follows from the decode
  position (`io_t::pos` after the last field) plus the padding, which `padnz`
  already half-codes; the granule follows from the modes of the packets that
  end in the page (`blockflag` gives the block sizes, the overlap rule gives
  the increment) with an explicit value only on a link's first and last page;
  the sequence number is the counter; and the page break itself is predictable
  from libvorbis's own flush rule (255 segments, or ~4 kB of body), so the
  symbol is a "did it flush where the model expected" bit.
- **Why it might work:** M9 -- the memoryless entropy of `music-stereo-q5`'s
  409 packet lengths is 316 bytes against a 440-byte `pages` stage, and the
  granule deltas are 14 bytes and the packet counts 12.  IMPROVEMENTS 7.3
  measured the same on the clip set: 5.83 bits per lacing value, 36 kB of the
  41.9 kB pages stage.  The length is fully redundant with the payload once the
  payload is decoded, and the plan already names this; what I would add is that
  the *same reorder* makes the granule and the sequence free, and that the page
  break is better modelled as an encoder-emulation bit than as a packet count
  plus lengths.
- **Expected gain:** **0.24% on music** (350 of 440 bytes), **0.6-0.7% on a
  container of short clips**, and disproportionately more where packets are
  many and tiny -- `silence-8k-long-qm1.ogg` has 2,817 packets in 14 pages and
  spends 53 of its 151 output bytes on `pages` (**35%**).  High confidence in
  the lengths, medium in the granule (the last page of a stream is short by the
  encoder's truncation and must be coded).
- **Cost:** the largest structural change in this list.  `og_packer::code_page`
  and `og_unpacker::unpack` both walk pages; the decoder must decode packets
  without knowing their length up front, which means `io_t` stops bounding the
  parse by `len` and instead runs to the end of the fields -- and that removes
  the `VB_IF(pos + n > len*8)` guard that currently refuses a truncated packet,
  so an explicit "this packet was cut short" escape is needed (which is also
  OPTIVORBIS 3.3's end-of-packet item, so the two go together).  Pages whose
  payload is coded raw keep the present path.  Format change: yes.  No loss of
  random access.  Per-symbol CPU: slightly less.
- **Risks:** the continuation machinery (`spill`, `g.cont`, `join_pkt`) is the
  hard part -- a packet that straddles a page boundary is currently handled by
  coding the spill; with derived lengths the split point has to be coded
  instead, and the corpus's `minbitrate-pad.ogg` and `zerolen-8k-q0.ogg` are
  the cases that will break first.  The libvorbis flush emulation is
  encoder-specific; a stream from another muxer will mispredict every page, and
  then the symbol costs about what it costs now.
- **Measure first:** `pg.py` beside this file already prints the memoryless
  entropy of the lacing, the granule deltas and the packet counts per file; run
  it over `testfiles/` and over a real container.  For the flush rule, count
  how often a page's body length is within a few bytes of 4,096 or the segment
  count is 255: if that is 95% of pages, the bit is nearly free.

### 5. Frame the solid archive once, and derive the metainfo's redundant fields

- **Idea:** In `-S`, every stream segment carries the 7-byte `OGGDETc`-style
  oggcomp header, the 4-byte whole-file CRC and the coder's flush byte (12
  bytes, the same 12 an empty input codes to), each metainfo chunk is a
  separate `os_open`/`os_close` with its own CRC and flush, and each segment
  gets a raw varint length.  In solid mode none of that is needed per segment:
  one header and one CRC for the archive, one continuous model stream, segment
  boundaries as a symbol in the model rather than a raw varint.  Separately,
  the stream record's length field is *derivable*: `oggdet.cpp` writes
  `REC_TAG(run_len, REC_STREAM)` and the restorer only "checks it against what
  it reads back", so in `-c` mode the decoded segment already says it (keep an
  explicit length only behind the CRLF flag, where the stored and restored
  lengths differ).
- **Why it might work:** M7 -- on a 120-stream archive the in-file metainfo
  costs 1,002 bytes for 401 bytes of content (2.5x expansion, because each
  chunk pays a CRC and a flush for a few bytes of record), the per-stream 12
  bytes are 1,440, and the total framing is about 1.0% of the output.
  IMPROVEMENTS 7.3 measured 27 kB, 0.5%, on the real clip set.  This is pure
  overhead: no information is being coded.
- **Expected gain:** **0.4 to 1.0% of a many-stream archive**; more the smaller
  the streams.  Nothing on a single file.  High confidence -- the bytes are
  countable and none of them carry information.
- **Cost:** low to moderate.  Archive-format change: yes (`OGGDETc3`).  The
  cost is the property the README advertises, that "cut out, a stream's segment
  is `oggcomp d`'s to read" -- which `-S` already gives up ("cut out, oggcomp d
  reads the first segment and refuses the rest"), so in solid mode nothing real
  is lost.  Keep the present framing for `-c`, where per-segment addressing is
  the point.
- **Risks:** damage detection gets coarser (one CRC for the archive instead of
  one per segment), so a truncated archive is detected later; `t.sh`'s check
  that a `-c` segment is byte-identical to `oggcomp c`'s output must be scoped
  to `-c`.
- **Measure first:** already done -- compare `oggdet -S -c` with and without
  `-m` (M7 gives 1,002 bytes for 401 of content), and multiply 12 bytes by the
  stream count.  No code needed.

### 6. Column-major digit order within a partition: a right-hand neighbour that keeps the codebook prior

- **Idea:** `rs_part` walks a partition codeword by codeword and `rs_sym` codes
  the `dim` digits of a codeword before reading the next, so the digit at slot
  `s` sees slots `< s` and nothing above.  Code the partition column-major
  instead: all the *first* digits of all `psz/dim` codewords, then all the
  second, and so on.  The codebook prior survives exactly -- `tc_pcur` needs
  digits 0..k-1 of *its own* codeword to step the trie, and in column-major
  order every earlier column is done -- at the price of `psz/dim` concurrent
  cursors, which is four words each.  Then add the new neighbours to `tc_dv`:
  for a digit at phase `k > 0`, slot `s-1` is known *and so are slots
  `s+dim-1`, `s+dim-2`, ...* above it.
- **Why it might work:** it makes the digit model partly non-causal for the
  same reason the floor model is: `vd_render` predicts a post from `lo` and
  `hi`, and the floor codes at 1.25 bits a post while a digit costs 1.56.  An
  MDCT residue around a tonal peak is a short cluster; knowing the bin *above*
  is worth as much as the bin below.  For `dim = 2` this is exactly the
  checkerboard scan of lossless image coding: half the slots trade a
  neighbour at distance 1 for one at distance 2, and half gain a neighbour on
  the other side.
- **Expected gain:** **0.5 to 2% of the digit stage** on tonal material,
  0 on `dim = 1` books (where the order does not change), possibly negative on
  noise.  Low-to-medium confidence: this is the most speculative of the top
  ideas, and the sign is not obvious.
- **Cost:** moderate.  `rs_part`/`rs_sym` restructured so the encoder reads a
  partition's codewords into an array first (the same read-ahead item 2 needs,
  so do them together) and the decoder assembles codewords at the end.
  `psz/dim` prior cursors, ~64 * 16 bytes.  Two more context variables.
  Format change: yes.  Per-symbol CPU: a little more bookkeeping, no more
  arithmetic.
- **Risks:** `q1`/`q2` change meaning -- they become the previous digit *in
  coding order*, which for phase 0 is a stride-`dim` neighbour, worse than
  today.  The optimizer will have to re-find every threshold on the `q` axes,
  so the gain is only readable after a full `opt.pl` pass, which makes this an
  expensive experiment to judge.  It also collides with item 2's read-ahead
  refactor, which is an argument for doing them as one version.
- **Measure first:** offline and with no model change.  Log the decoded digit
  sequence per partition (a dump behind `tc_verbose`), then fit two crude
  adaptive estimators over the log -- one with `(q1, q2, t1)` and one with
  `(s-1, s+1, t1)` -- and compare bits per digit, exactly as IMPROVEMENTS 7.4
  compared run lengths with per-digit contexts.  If the two-sided estimator is
  not at least 3% better on the log, the real thing will not be either.

### 7. A container prelude: warm state written once, so `-c` gets `-S`'s ratio and keeps random access

- **Idea:** `-S` wins over `-c` because the model is warm; the price is that
  the archive decodes only front to back.  Have the carver make one cheap pass
  over the container, build a compact summary -- the set of distinct setup and
  comment headers, the distribution of modes and block sizes, a coarse
  histogram of class and digit magnitude per band, the counters of the small
  families (`aux`, `hdr`) after a training run over a sample of streams -- and
  write it once as a prelude segment.  Both sides initialise from it; each
  stream is then coded from the *same* warm state, so any stream can be decoded
  on its own.  The natural companion is **grouped solid**: partition the
  container into groups of N streams, each group solid from the prelude, so
  random access is at group granularity and N groups decode on N cores.
- **Why it might work:** M8 -- on a 120-stream container `-c` is 309,568 and
  `-S` is 274,138, an 11.4% cold-start tax; on the real 1,257-clip set
  IMPROVEMENTS 7.3 measures the same tax at 4% (240 kB) plus 197 kB of setup
  headers re-coded per stream, which the prelude also carries.  That 4% is
  currently only recoverable by giving up random access entirely.
- **Expected gain:** recovers **2 to 4% of a container of small streams** for
  `-c`, i.e. most of the `-c`/`-S` gap, while keeping per-stream addressing;
  on `-S` itself it helps only the first group, which is small.  It is a
  different thing from the static trained prior in the plan (section 6.5):
  that one is baked into the binary and is the same for every input, this one
  adapts to the container, and the two compose.  Medium confidence.
- **Cost:** high.  A serialisable form of the model's warm state is the hard
  part -- the tables are 366 MB, so the prelude has to be a *summary* that the
  model expands (initial counter values for a chosen set of contexts), not a
  dump.  Two passes over the input, so `-c` from a pipe cannot do it.
  Archive-format change: yes.  Memory: one more table of initial values.
- **Risks:** the summary may not transfer -- a prior fitted on the first pass
  and applied to the same data is optimistic, and the honest measurement is
  cross-validated (fit on half the streams, measure on the other half).  It
  also duplicates work with the static prior: measure the static one first,
  since it is cheaper, and only then ask what the per-container one adds.
- **Measure first:** an upper bound with no new format.  Code the container
  `-S`, then code it `-S` *again* on the same model instance (a second `Init`
  with `OC_F_SOLID`), and read the second pass's size: that is what a perfect
  prelude would give.  If the second pass is not several percent under the
  first, the prelude has nothing to carry.

### 8. Give the byte path cross-stream memory, and one model per logical stream

- **Idea:** `oc_raw.inc` is order-0 plus order-1, deliberately, "because
  everything it codes is something oggcomp would rather not be coding".  In a
  container that is exactly wrong: the bytes between streams are most of the
  file, they repeat across the container, and they are coded with a model that
  cannot see a repeat.  Add a match model / LZP over everything the byte path
  has emitted in this run (one hash of the last N bytes, a predicted next byte,
  one more mixer input), and split `raw_o1` per *logical stream serial* so a
  Skeleton stream's bytes and a Theora stream's bytes do not share an order-1
  table with a JPEG in a comment header.
- **Why it might work:** M6 -- a container that is 85% non-Ogg text codes to
  150,332 bytes under `oggdet -S` against 88,684 under `xz -9e`; the non-Ogg
  bytes alone go to 142,186 of 258,858, **4.40 bits a byte**, where a match
  model would find every repeated word.  Even the tiny gaps in my clip
  containers *expand*: "141 other bytes coded to 308 (218%)", because a short
  run pays `rawlen` framing and a cold table.  `oggdet` is advertised for "an
  archive, a disk image, a download that stopped" -- containers where the
  non-Ogg part dominates.  `id3-apic-8k.ogg` shows the single-file version:
  904 bytes at 7.95 bits a byte for an ID3 APIC frame.
- **Expected gain:** **nothing on a pure Vorbis file**; on a container where
  the gaps are a third of the bytes and compressible, **20-45% of the
  archive** (M6 says oggcomp gives up 62 kB of 150 kB there).  High confidence
  that a match model closes most of the gap to `xz`, since that is all `xz` is
  doing.
- **Cost:** moderate.  One hash table (a few MB), one predicted byte, one more
  mixer input in `raw()`, plus the per-serial table split.  No effect on the
  audio path's CPU.  Format change: yes.  The order-1 split is nearly free.
- **Risks:** it is a general-purpose compressor bolted on, and the counter-
  argument in `oc_raw.inc`'s comment ("the day that changes, this is where a
  real model goes") is the author's own -- the risk is scope, not correctness.
  A 4 MB hash makes the memory story worse on top of 366 MB of tables.
- **Measure first:** `gap.bin` beside this file, or any real container: compare
  `oggdet -S -c` with `xz -9e` and with `xz -9e` on the gaps alone (carve
  without `-c` and compress `prefix.meta`).  The difference is the ceiling.

### 9. Hoist a page's floors ahead of its residues

- **Idea:** `payload()` already codes every channel's floor before any residue
  of that packet.  Go one level up: buffer a page's packets and code *all* the
  floors of the page, then all the residues.  Then IMPROVEMENTS 4.2's
  floor-normalised history gets a forward half -- the digit model can see the
  floor at this partition's frequency in the *next* packet as well as the last,
  so a decaying tone and a rising one are told apart at the digit rather than
  after it.
- **Why it might work:** a residue is the spectrum divided by the floor, so the
  floor's motion is exactly the part of `t1`'s error that 4.2 is about; the
  backward difference says where the envelope has been and the forward one says
  where it is going, and for a decaying note those are not the same.  The floor
  model itself loses nothing by being hoisted -- `oc_floor::floor` reads only
  `fl_hist`/`fl_yhist`, never a digit.
- **Expected gain:** **0.2 to 1% of the digit stage** on tonal material, on top
  of whatever 4.2 gets; nothing on noise.  Low-to-medium confidence, and it
  should only be attempted after 4.2 has shown the backward half works.
- **Cost:** moderate.  The decoder must hold a page's worth of packet buffers
  and floor vectors before it can write the packets back (a page is at most
  65,025 payload bytes, and `vb_ys` would become per-packet), and `io_t`'s
  bit-writing becomes two-phase.  Format change: yes.  Interacts with item 4,
  which restructures the same walk -- do them together or not at all.
- **Risks:** the buffering is the cost and the gain is the smaller half of an
  idea that has not yet been shown to work at all.  A page with one packet (the
  common case for short blocks in some encoders) gets nothing.
- **Measure first:** implement 4.2's backward `fd`/`t1n` first and read the
  corpus total.  Then, offline, log `(floor at the partition's frequency,
  previous, next, digit)` and compare the conditional entropy of the digit
  given the backward difference alone against given both.

### 10. Per-logical-stream page state, for multiplexed and chained files

- **Idea:** `oc_frame.inc::page()` keeps one `oc_seq`, one `pg_prev` (granule)
  and one `pg_prevser` for the whole *group*, and `link_begin()` resets them.
  In a multiplexed file the pages of two or three logical streams interleave,
  so the sequence number is predicted from a counter that counts *all* the
  group's pages, the granule delta is taken between two different streams'
  granules, and the serial delta oscillates.  Keep the three per serial number
  in a small table (the group already tracks `vserial`).
- **Why it might work:** `skeleton-8k.ogg`'s six pages are serials
  `412,411,412,411,412,411` with per-serial sequence numbers `0,1,2` each: the
  present model predicts `seq` as 0,1,2,3,4,5 and pays for deltas of
  0,-1,-1,-2,-2,-3, and predicts each granule from the other stream's.  With
  per-serial state every one of those is a zero delta.  A Theora video beside
  the Vorbis makes it worse, since a Theora granule is a packed frame pair and
  bears no relation to an audio granule.
- **Expected gain:** a few bytes per page on multiplexed files -- `skeleton-8k`
  spends 35 of 268 output bytes (13%) on `pages` for six pages.  On a
  Vorbis+Theora file with thousands of interleaved pages this is the whole
  `pages` stage, so **1-3% of a multiplexed file**; **nothing** on the
  single-stream files that are 90% of the corpus.  High confidence, low
  absolute value except on the files it is for.
- **Cost:** very low.  A small hash or linear table of at most a handful of
  serials, three fields each.  Format change: yes, but trivial.  No CPU cost.
- **Risks:** a file with many serials (a badly muxed capture) needs the table
  bounded, with a fall-back to today's behaviour past the bound.
- **Measure first:** `pg.py` prints serial/seq/granule per page; compute the
  entropy of the sequence deltas under the two predictors on a real
  Vorbis+Theora file.  Ten lines.

### 11. Carry the per-stream content history across links and solid segments

- **Idea:** `setup_done()` calls `hist.reset()`, so every link of a chained
  file and every segment of a solid archive begins with `t1 = t2 = 0`, an empty
  `dg_avg`, an empty `cl_hist` and `fl_who = -1`.  The counters carry over;
  the *content* history does not.  When the new stream's setup is the same
  object as the previous one's (which the header cache already tells us), keep
  `dg_avg` (the decayed magnitude per slot) and the floor history, and reset
  only the exact per-slot values that would be wrong.
- **Why it might work:** M4 -- a second stream in a solid archive costs 75% of
  what it costs alone, and the first packet of every stream is the worst-coded
  one because it has no `t1`.  On the 1,257-clip set a stream is 11 to 118
  audio packets, so the cold packets are a real share.  `dg_avg` is a
  statistic, not a value, and it transfers between streams of the same kind in
  a way the exact `t1` does not.
- **Expected gain:** **0.2 to 0.8% of a container of short streams**; near zero
  on a long single file.  Compounds with item 3 (ordering), since a carried
  average is only useful if the next stream is similar.  Medium confidence.
- **Cost:** low.  A flag through `hist.reset()` and a version bump.  No CPU, no
  memory.  Format change: yes.  Does not affect random access in `-c` because
  in `-c` the model is reset anyway; this is a `-S`-only and chained-file-only
  change.
- **Risks:** carrying the wrong history is worse than carrying none, and the
  model has no way to be told which it has.  Mitigate by carrying only the
  decayed averages and by scaling them down at a segment boundary.
- **Measure first:** the cheapest proxy exists already -- M4's table.  Build a
  two-stream archive from two streams of the same kind and one from two of
  different kinds, and compare the marginal cost of the second; if carrying
  `dg_avg` is worth anything, it is worth it in the first case.

### 12. Parse the ID3 and APE frames around a stream

- **Idea:** an ID3v2 tag before the first page and an ID3v1 or APE tag after
  the last go through `consume_junk` to `oc_raw` at 8 bits a byte
  (`id3-apic-8k.ogg`: 904 bytes at 7.95 bits each; `id3-prefix-8k.ogg`,
  `id3v1-trailer-8k.ogg`, `chained-id3.ogg` are the other witnesses).  Detect
  the tag, code its frame headers as fields (frame id from a fixed list the way
  `oc_tagorder` does it, size as a value, flags as a symbol), and route the
  frame *payloads* through the same field/blob cache item 1 builds -- an APIC
  frame is the same cover art as the comment header's picture block, and in a
  container it is the same across every track.
- **Why it might work:** the structure is entirely known and entirely
  unmodelled; the synchsafe size fields alone are four bytes each of which
  three are usually zero, and the padding at the end of an ID3v2 tag is
  typically hundreds of zero bytes coded one at a time.
- **Expected gain:** **10-40% of a tagged small file's output**, zero on
  untagged files.  Together with item 1 it makes a container of tagged tracks
  pay for its art once.  Medium-high confidence on the direction.
- **Cost:** low to moderate, and it rides on item 1's cache.  Format change:
  yes.  Falls back to the byte path on anything it does not recognise, which is
  the existing behaviour.
- **Risks:** ID3 has unsynchronisation, several versions, and extended headers;
  a byte-exact parser for all of it is more work than it looks, so scope it to
  the plain v2.3/v2.4 case and let the rest fall through.
- **Measure first:** `id3-apic-8k.ogg` and `id3-prefix-8k.ogg`: `oggcomp c -v`
  prints the `raw` stage, and the ceiling is that number minus what `xz` gets
  on the same tag bytes cut out with `dd`.

### 13. Sub-stream deduplication: a stream that is another with a piece changed

- **Idea:** `dt_lookup` dedups whole streams byte-identically.  Extend it one
  level: hash each stream's *pages* (or each page's payload), and when a new
  stream's page hashes mostly match an earlier stream's, record it as "stream
  k, with these pages replaced" and code only the differing pages.  The obvious
  cases are a clip that is another clip trimmed, a re-tag (same audio, new
  comment header, new CRCs and sequence numbers -- so *not* byte-identical),
  and a partial download beside the whole file in the same container.
- **Why it might work:** re-tagging changes the comment header's page and
  nothing else, yet today the whole stream is a dedup miss and is coded again;
  M4 shows a trimmed variant costs 70% of its standalone size, so most of it is
  being paid for twice.  Page hashing is cheap and the carver already hashes
  streams.
- **Expected gain:** **0 on most containers; up to the size of the duplicated
  part** on containers that hold variants -- a media library with both a tagged
  and an untagged copy, a backup with two generations of the same file.  Low
  confidence that it fires often, high confidence that it is large when it
  does.
- **Cost:** moderate.  A page-hash index in the carver, a new metainfo record
  kind, and a restorer that can splice.  Archive-format change: yes.  Random
  access: a stream that references another needs the other, so it is a
  back-reference like the existing duplicate-of.
- **Risks:** page-level matching is defeated by anything that shifts a byte
  (page boundaries move, sequence numbers change, CRCs change), which is most
  re-encodes; it only catches *copies*, not variants.  M4 says nothing here
  about variants, which is the honest scope.
- **Measure first:** on a real container, hash every page payload and report
  how many bytes are in payloads that occur more than once but in streams that
  are not byte-identical.  A twenty-line script over `oggdet -t` output.

---

## Wild cards

**Two-pass significance then magnitude, with a non-causal magnitude pass.**
Code, for every slot of a residue vector, only "is this digit zero", then come
back and code the magnitudes and signs of the nonzero ones with the whole
zero-map -- both sides -- as context.  The zero decision is 37-59% of the digit
bytes (7.4) and the magnitude pass would see local clustering the way a
bitplane image coder does.  The reason I would not bet on it: it breaks the
codebook prior, which needs the *exact values* of the earlier digits of a
codeword to step the trie, and the prior is one of the two things that make
this a Vorbis model rather than a generic one.  The prior could be kept for the
significance pass and dropped for the magnitudes, but then the magnitudes lose
`prp` and `pq`, and 7.4's experience is that whatever you take from the prior
you do not get back elsewhere.  Item 6 is the version of this idea that keeps
the prior, which is why it is in the main list and this is not.

**Interleave the streams of a container packet by packet.**  For a container
that is many near-aligned variants of the same source -- a game's footstep
sounds at four intensities, a stem set, a multi-quality asset pack -- code
packet 1 of every stream, then packet 2 of every stream, so that `t1`, the same
slot one packet ago, becomes the same slot in the *previous stream*.  That is
the cross-stream reference channel of item 10 realised for free, with no new
context variables at all.  Against it: M4 says the current model gets nothing
from adjacency for gain, fade and trim variants, and interleaving would destroy
`t1`'s normal meaning for every stream that is not a variant, so it would have
to be a mode the carver selects on evidence.  Worth an afternoon on a real
variant set before dismissing.

**Code the canonical form, then the diff from it.**  Byte-exactness forces
reproducing page packing, granules, serials, sequence numbers, padding bits and
CRCs; everything else about the stream is what libvorbis would have produced.
So build the canonical stream the way OptiVorbis's `granulator.rs` and page
filler do -- fill pages to the flush threshold, derive granules, number pages
from zero -- and code only where the real file departs from it.  For a file
straight out of `oggenc` the diff is empty and the whole container layer costs
a handful of symbols; for a file that has been through a tagger, a streaming
server or a text-mode transfer the diff is exactly the interesting part.  It is
item 4 taken to its conclusion, and the reason it is a wild card is that it
makes the compressor carry a model of *one* encoder's muxing policy, which ages
badly and has to be versioned.

**Sort the partitions of a packet by class before coding their digits.**  Once
every class of a residue is coded (item 2), the permutation that sorts
partitions by class is free -- both sides can compute it.  Code the digits in
that order, so that all the partitions with the loudest book come first and the
counters see homogeneous statistics in long runs instead of alternating.  The
obvious objection is that it destroys spectral adjacency, and `q1`/`q2` are
strong contexts; but the class already conditions nearly everything the digit
model does, and it is not obvious which of adjacency and homogeneity wins.  A
cheap variant that risks nothing: keep frequency order but *add* the
class-sorted rank of the partition as a context variable.

**A per-container codebook-table row learned on the fly.**  `vbooks_gen.inc`
is 350 fixed rows from 35 encoders and a setup that is not one of them is coded
field by field.  In a container, the *first* stream with an unknown setup could
become row 351 for the rest of the container -- which the header cache almost
does, except that it matches whole packets and a setup that differs in one
codebook is a total miss.  A per-book cache (this book is book 7 of the setup
two streams ago) would catch the aoTuV/OptiVorbis/re-encoder cases the table
misses, at the cost of a symbol per book.  IMPROVEMENTS 7.1 says the table took
the corpus from 76.8% to 71.3%; the streams it does not cover are the ones this
would.
