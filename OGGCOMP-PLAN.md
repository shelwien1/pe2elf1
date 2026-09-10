# oggcomp: one walk over the Ogg, one tool -- the plan

`balrogg c` takes an Ogg Vorbis file apart into a record stream and `tsvcomp
c` codes that stream; `tsvcomp d` and `balrogg d` put it back.  This plans
the tool that does both in one pass, `oggcomp`:

```
oggcomp c input.ogg output.oc
oggcomp d input.oc  restored.ogg
```

with the record stream gone from between them, and with nothing held that
is not held today.  It is a plan, not a design sketch: every value that
crosses from one tool to the other is named below, with where it will cross
instead, and the acceptance test is bit-exactness against the pair of tools
it replaces.

---

## 1. Where the two tools stand

### What each does

`balrogg` (`main.cpp` and the `vb_*`, `io`, `source`, `link_walk`, `codec`
includes) is a bit-level Vorbis parser that never decodes audio.  `io.inc`
reads a packet's bits and writes each value it finds as a `(tag, value)`
record -- `t.put("res.digit", d)` -- or, in the other direction, asks for the
record and writes the bits.  `codec.inc` walks pages, `link_walk.inc` walks
the packets of a page and gathers a packet that runs across pages,
`source.inc` is a forward-only 32 MB window over the input.  Both directions
stream: nothing is held past one page, one gathered packet and the link's
setup.

`tsvcomp` (`tsvcomp.cpp` and its includes) reads that record stream and
models it.  To know what a record *means* it has to know where in the Vorbis
structure it sits, and the only way to know that from a stream of values is
to walk the structure again.  So it carries a second copy of the walk --
`tc_walk` over links and pages, `tc_page` for the page fields, `tc_header`
for the three header packets, `tc_audio`, `tc_payload`, `tc_residue`,
`tc_part` down to the digit -- and a second parser of the setup,
`vd_setup.inc`, which rebuilds the codebooks, floors, residues and mappings
from their records.  It also streams, page by page.

### What passes between them

77 distinct tags, from about 75 value sites in `io.inc` plus the page fields in
`ogg_page.inc` and `link.more`/`page.spill` in `codec.inc`/`link_walk.inc`.
As text, at 8 to 14 times the size of the Ogg:

| file | .ogg | .tsv | .oc | balrogg c | tsvcomp c | tsvcomp d | balrogg d |
|---|---|---|---|---|---|---|---|
| 00000007 | 919,643 | 13,119,882 | 775,998 | 0.37 s | 1.85 s | 1.52 s | 0.13 s |
| 00000008 | 5,701,903 | 47,647,174 | 5,127,183 | 1.52 s | 8.77 s | 8.10 s | 0.56 s |

The model is the cost.  What the merge removes is the text -- formatting it,
writing it, reading it, tokenising it, checking every tag -- and the second
walk that exists only to recover from the text what the first walk knew
when it wrote it.  That is the balrogg column plus some part of the tsvcomp
column: roughly 15-25% of the wall time, and one intermediate file that is
larger than the input.

### The duplication, function by function

| balrogg | tsvcomp | what it is |
|---|---|---|
| `vb_pack` / `vb_unpack` (codec.inc) | `tc_walk` | links, pages, continued packets |
| `ogg_page::put` / `get` | `tc_page` | the page header fields |
| `link_walk::page` | inside `tc_walk` | packets of a page, `page.spill` |
| `io::ident`, `comment`, `setup` | `tc_header` + `vd_setup.inc` | the three header packets |
| `io::codebooks`, `floors`, `residues`, `mappings` | `vd_book::read`, `vd_floor::read`, `vd_res::read`, `vd_map::read` | the setup, parsed twice |
| `vb_book` (len, mult, inv, off, base) | `vd_book` (clen, mval, minv, off, base) | the same tables under other names |
| `io::audio` | `tc_audio` | mode, window flags |
| `io::payload` | `tc_payload` | floors per channel, coupling, residues |
| `io::residue` | `tc_residue` | passes, classwords, partitions |
| `io::rs_part`, `rs_sym` | `tc_part` | the digits of a partition |
| `vb_floor::sort` | `vd_floor::read` (srt, rnk, lo, hi, pcl, ppos) | post order and neighbours |

Every row is the same loop written twice, one over bits and one over
records, kept in step by hand.  `tsvcomp.cpp`'s preamble says so: "the walk
is tsvtrans's, and has to be, since what a record means is decided by
records already read".

---

## 2. The shape of the merged tool

**One walk -- balrogg's -- with the model called at the value sites, with
the context the walk has in hand.**

Three shapes were considered.

**(a) Connect the two as they are, in memory.**  `tsv` already has hooks
(`create_hook`, `open_hook`) that route `put` and `get` to a function
instead of a file.  But balrogg *pushes* records and tsvcomp *pulls* them,
and joining a push to a pull in one thread needs a coroutine or an inversion
of one side.  It would also still format and parse text -- the hooks carry
tag strings and the model would still re-derive its context by walking the
records.  Two walks, no file.  It is the fallback if (c) stalls (section 8).

**(b) Make the model a push-driven `tsv`.**  Give the model a `put(tag,
value)` and let balrogg drive it.  The model then has to know, from the
sequence of records alone, that this `res.digit` is pass 3 of partition 17
of residue 1 in channel 0 -- which is what `tc_residue` and `tc_part` know
because they *are* loops.  Turning each into a state machine that is
advanced one record at a time is a rewrite of every loop in tsvcomp's walk
into explicit state, and at the end there are still two copies of the
structure.

**(c) One walk, typed calls into the model.**  `io.inc`'s `residue()` has
`rno`, the pass, the vector, the partition, the class and the book at the
point where it reads a digit; that is what tsvcomp keeps a second copy of
the loop to know, and hands to `tc_part` in twelve arguments.  So the
digit is handed to the model *there*, with those arguments, and the
model's own walk is deleted.  The record stream
survives only as balrogg's inspection format, produced by a sink that writes
records instead of coding them.

(c) is the plan.  Its one structural cost is that `io.inc` can no longer
name `tsv` as the type of its sink, and its one structural benefit is that
the model gets its context from the parser instead of reconstructing it.

### The sink

`io` holds a `tsv &` today, `link_walk::page` takes one as an argument, and
`vb_pack` / `vb_unpack` each own one.  All three will name `sink_t` instead,
a typedef made before `io.inc` is included:

```c
typedef oc_tsv   sink_t;    /*  main.cpp: balrogg, writes records  */
typedef oc_model sink_t;    /*  oggcomp.cpp: codes them  */
```

A typedef rather than a template, because the two sinks never meet in one
binary and `io.inc` stays readable.  The sink is a set of **typed methods**
-- fifteen, of which ten carry a value -- each taking what the walk knows at
that site.  Every
method has the same shape as `io::val` has today: in mode c it is given the
value and codes it; in mode d it decodes and returns it.

`oc_tsv` implements each method as the `t.put` / `t.get_u` that `io.inc`
does now, with the same tags, so `balrogg c` writes the same bytes it always
has.  `oc_model` implements each with the corresponding piece of tsvcomp.

### The sink's contract, in full

| method | called from | absorbs from tsvcomp | context it takes |
|---|---|---|---|
| `link(more)` | `vb_pack`/`vb_unpack` | `tc_aux(F_MORE)` | -- |
| `link_begin()` | after `v.link()` | the per-link resets at the top of `tc_walk`'s loop | -- |
| `page(ogg_page &, bos, cont)` | `link_walk::page` (c), `vb_unpack` (d) | `tc_page`: type against `bos<<1|cont`, granule/serial/sequence/lengths as residuals | `bos`, `cont` from `codec`; the page counter is the sink's own |
| `spill(n)` | `link_walk::page` / `vb_unpack` | `tc_aux(F_SPILL)` | -- |
| `hdr(tag, x, hi)` | `io::fld`, `val`, `flt` on the header path; `io::comment` | `tc_hdrval` via `tc_tagid`: the difference from the last value under the tag | the tag string, interned by content with the last pointer cached, as `tsv::shorten` does |
| `setup_done(vb_ctx &)` | `io::hdr(2)` after `setup()` | `tc_setup_done`, `tcp_build`, the floor neighbour tables from `vd_floor::read` | the whole `vb_setup` |
| `mode(md, nmd)`, `wprev(w)`, `wnext(w)` | `io::audio` | `tc_audio` | previous mode / window from the sink's own state |
| `floor(k, fno, f, y[], used)` | `io::payload`, once per channel | the per-channel block of `tc_payload`: `flr.used`, then every post, coded in list order | channel, floor number, the floor, the whole post array |
| `residue_begin(rno, r, nz[], nch, n)` | `io::residue` on entry | `tc_residue`'s entry: `cidx` from `nz`, the per-residue history clears | the channels and which are non-zero |
| `cls(rno, ch, pc, k)` | `io::residue`, the `res.class` site | `tc_classify` plus the run/match bookkeeping and the classbook prior cursor | which channel, which partition, which place in the classword -- the cursor restarts at place 0 |
| `part_begin(rno, pass, ch, pc, r, bk, bn, cls)` | `io::residue`, just before each `rs_part` -- `rs_part` itself has only the residue and the book | `tc_part`'s prologue: history rows, the neighbour stride, the book's prior | everything `tc_part` takes today, from the loop that has it |
| `vec_begin()` | `io::rs_sym` on entry | the prior cursor restart at `vpos == 0` | -- |
| `digit(d)` | `io::rs_sym`, the `res.digit` site | `tc_part`'s per-digit body: build `tc_dv`, code magnitude and sign, step the prior, update the histories | the sink counts the slot and `vpos` itself |

Constants checked by `io::cst` are not records today and are not coded
tomorrow.  Nothing else crosses.

The header path needs one note.  `io::fld`, `val`, `sval` and `flt` are
funnels used by both the header packets and the audio path, and the sink
cannot tell from a tag string which is which.  The header sites keep the
funnels, and the funnels call `t.hdr(tag, x, hi)`; the seven audio-path
sites (`flr.used`, `flr.y`, `res.class`, `res.digit`, `aud.mode`,
`aud.wprev`, `aud.wnext`) are rewritten to the typed calls above, which is
where they need context anyway.

---

## 3. What moves, what goes, what stays

### tsvcomp.cpp

| piece | fate |
|---|---|
| `tc_walk`, `main`, the `.tc` container | gone; `codec.inc`'s walk and `oggcomp.cpp`'s main replace them |
| `tc_page`, `tc_pg` | becomes `oc_model::page`; the residual logic unchanged, the `tsv` argument gone |
| `tc_header`, `tc_hdr_enc`, `tc_hdr_dec`, the `tee` / hook arrangement | gone; `oc_model::hdr` is called at the `put` sites directly, which is what the hooks were simulating |
| `tc_tagid`, `tc_tlast`, `tc_runpos` | kept as is: the `tag` axis of the header model is the id in order of first sighting, and bit-exactness needs that order |
| `tc_audio` | becomes `mode` / `wprev` / `wnext` |
| `tc_payload` | the floor block becomes `oc_model::floor`; the coupling and the submap loop are `io::payload`'s already |
| `tc_residue` | the bookkeeping becomes `residue_begin` and `cls`; the loops are `io::residue`'s already |
| `tc_part` | becomes `part_begin` + `vec_begin` + `digit`; the body is unchanged |
| `tc_classify` | inside `cls` |
| `tc_setup_done`, `tc_hist_free`, `tcp_build` | `setup_done`, reading `vb_setup` instead of `vd_setup` |
| `tc_fam`, `tc_dv`, `tcx`, the histories, `tc_models`, `tc_map`, `tc_tables`, `TC_MEMCOST`, `-v` accounting, `TC_WIRE` | unchanged |
| `vd_setup.inc` | gone |
| `lift.inc`, `floor_db.inc`, `imdct.inc` | gone from this build; they were included for `MD_MAXBLK` and `fx()`, which `vd_setup` used and the model does not |
| `tsv.inc` | not included by oggcomp; `TSV_TAGMAX`, which sizes `tc_tagbuf`, moves to the sink |

`cm.inc`, `sh_v2f.inc`, `sh_mapping.inc`, `tc_prior.inc`, `MOD/`, `IDX/`,
`idx2inc.pl`, `opt.pl`, `import.pl` are untouched.  They are the model and
its declarations, and nothing about the model changes.

### The setup, once

The model reads `vb_setup` through `vb_ctx::cur`.  The fields it uses map
one to one, with three things to watch:

| vd_setup | vb_setup | note |
|---|---|---|
| `vd_book::clen` | `vb_book::len` | same: codeword length per entry, 0 = unused |
| `vd_book::mval`, `nv` | `vb_book::mult`, `nv` | same |
| `vd_book::minv` (`i32`, -1 absent) | `vb_book::inv` (`u32`, `(u32) -1` absent) | one comparison changes sign |
| `vd_book::off`, `base`, `dim`, `ent`, `lookup` | `off`, `base`, `dim`, `ent`, `look` | same |
| `vd_floor::srt`, `x`, `quant`, `posts`, `cdim`, `csub`, `csb`, `cbk` | `vb_floor::srt`, `x`, `quant`, `posts`, `cdim`, `csub`, `csb`, `cbook` | same |
| `vd_floor::rnk`, `lo`, `hi`, `pcl`, `ppos` | -- | derived; computed in `setup_done` into a per-floor `oc_floor` beside `vb_floor`, by the loop at the end of `vd_floor::read` |
| `vd_res`, `vd_map` | `vb_res`, `vb_map` | identical layouts |
| `su.ch`, `bs0`, `bs1` | `vb_info` | same |
| `vd_book::min_fx`, `delta_fx`, `minval`, `delta` | -- | never read by the model; dropped with `lift.inc` |

`vb_setup` validates more than `vd_setup` does (`vb_setup::check`,
`vb_book::grid`); the model's own checks in `tc_setup_done` and `tcp_build`
stay, since they bound what the model allocates rather than what Vorbis
allows.

### balrogg

`main.cpp` keeps building, as the record-stream tool: `io.inc`'s sites now
call sink methods and `oc_tsv` turns them back into the same `put`s, so the
TSV it writes is byte for byte what it writes today.  That is checked, not
assumed (section 6, step 2).

### The files

```
oc_sink.inc     the contract above, as comments, and oc_tsv
oc_model.inc    the model sink: tsvcomp.cpp minus its walk, reading vb_setup
oggcomp.cpp     includes, the .oc container, main; instantiates the walk
io.inc          sink_t & t; the typed calls at the sites
link_walk.inc, codec.inc   sink_t & t
main.cpp        typedef oc_tsv sink_t before io.inc
```

`vb_pack` and `vb_unpack` in `codec.inc` are the walk.  They take the sink
by reference and no longer open a `tsv`; opening the output (a TSV or the
range coder) is the caller's.  The page loop, the continued-packet handling
and the EOS/BOS checks are theirs and run unchanged in both tools.

---

## 4. The container and what streams

```
"oggc\x1a"   magic
u8 version   1
u8 spare     0; the per-file profile byte, if one is ever wanted
...          the range coder's bytes
```

Encode: `source` reads the input through its window; each page is parsed
where it lies, each packet coded as it is taken apart, and `rc_buf` writes
the coder's bytes in 64 KB pieces.  Decode: `rc_buf` refills in 64 KB
pieces; a page's fields are decoded, its packets rebuilt into `body`, the
page emitted, and the next begun.  What is held at any moment:

| what | bound | today |
|---|---|---|
| the input window | 32 MB (`IN_WIN`) | balrogg, unchanged |
| one page, one page image | 65 KB each (`PAGE_MAX`) | unchanged |
| one packet gathered across pages | 16.6 MB (`JB_MAX`) | unchanged |
| the link's setup and codebook arena | 96 MB (`VB_ARENA`) | unchanged |
| the model's histories | per residue span, `TC_HISTMAX` | tsvcomp, unchanged |
| the model's tables | address space, mapped; touched pages only | unchanged |
| the coder buffer | 64 KB | unchanged |
| **the record stream** | **the whole file, 8-14x the input** | **gone** |

The last row is the only thing about today's arrangement that is not
sequential, and it goes.

Where the walk is still not strictly one value at a time, and why that
stays:

- **A continued packet is gathered before it is coded.**  `link_walk`
  looks ahead through the window to find where the packet ends
  (`source::join_len`) and copies it whole into `jb`.  Coding could in
  principle start on the first fragment, but `io` reads bits, a codeword
  may straddle the fragment boundary, and the bound is 16.6 MB; the
  lookahead is bounded by the window and already exists.
- **A channel's floor posts are coded after the whole channel is parsed.**
  The model codes posts in list order, not the ascending-X order the
  packet holds them in, because that is the order the prediction is
  available in.  256 values at most.
- **The setup completes before any audio.**  The codebook prior is built
  from the whole setup in `setup_done`.  That is the format's own order.
- **A page's fields are coded before its packets**, on both sides, which is
  the order the page holds them in.  No lookahead.

---

## 5. Decode

`io` in rebuild mode calls the same sink methods with `enc == 0`; each
returns what it decoded, `io` writes the bits, `ogg_page::pack` lays out
the page, `emit` computes the CRC.  This is `vb_unpack` today with the
`tsv` reads replaced, and the order of sink calls is the order of the
encoder's by construction, because the same `io` code makes them in both
directions.  That is the contract `io.inc` already has with `tsv` and the
one `tsvcomp` already has with itself; the merge does not add a new one.

What the record stream did that the coder does not is name the tag of
every value, so that a desync fails at the record rather than as noise.
oggcomp keeps that as a debug aid, not a format: `-DOC_TRACE` writes the
tag and value of every sink call to a side file, and a trace from `c`
diffed against one from `d` finds the first divergence.  A trace from
`balrogg c` is the same sequence, which is what section 6 leans on.

---

## 6. The steps, each with its check

The whole migration is a refactor with a bit-exact oracle: `tsvcomp c` on
`balrogg c`'s output is a function of the values and their order, and
oggcomp is meant to see the same values in the same order with the same
context.  So the coded bytes must match exactly, and until they do the
work is not finished.  Each step below is small enough to land alone and
has a check that decides it.

**Step 0 -- baseline.**  Record the 17 corpus sizes (`log.txt` has them)
and the `.tsv` of every corpus file from the current `balrogg c`.  Those
are the oracles for steps 2 and 4.

**Step 1 -- `sink_t`.**  `io.inc`, `link_walk.inc` and `codec.inc` take
`sink_t & t`; `main.cpp` typedefs `tsv` to it.  No site changes.
*Check:* balrogg's TSV output is byte-identical on all 17 files.

**Step 2 -- typed sites.**  `oc_sink.inc` defines `oc_tsv` with the
methods of section 2 implemented as today's `put`/`get_u` with today's
tags; the seven audio-path sites, the page fields, `link.more` and
`page.spill` are rewritten to call them; the header funnels call `hdr`.
*Check:* balrogg's TSV is still byte-identical.  This step proves the
sites are correctly placed before any model is attached to them.

**Step 3 -- `oc_model.inc`.**  Move the model out of `tsvcomp.cpp`:
everything from `cm.inc`'s inclusion to `tc_models`, minus the walk
functions in section 3's table.  Re-point the setup fields at `vb_*`; add
`oc_floor` and the derived-table loop to `setup_done`.  Implement the sink
methods as thin shells over the moved bodies.  `tsvcomp.cpp` keeps
building against `vd_setup` until step 5, so the oracle stays available.
*Check:* it compiles; `./mk.sh` reports the same count of tunable bits
and patterns for oggcomp that it reports for tsvcomp (5947 bits in 651
patterns as this is written).

**Step 4 -- `oggcomp.cpp`.**  The container, `main`, and the walk
instantiated over `oc_model`.  `vb_pack` / `vb_unpack` lose their `tsv`
argument and gain the sink.
*Check, the one that matters:* for every corpus file, `oggcomp c file.ogg`
minus its 7-byte container header is byte-identical to `tsvcomp c` of
`balrogg c file.ogg` minus its 7-byte header.  Any difference is found by
diffing the `OC_TRACE` of `oggcomp c` against `balrogg c`'s TSV (they are
the same sequence with the same tags) for the first value that differs,
or, if the values agree, the first context that does -- which the trace
can also carry.  Expect the first run to disagree at one of: the page
counter, `cidx` for a residue coded per channel, the digit count of a
partition whose size is not a multiple of the book dimension (section
7), or the tag-id order in the header model.  Each is a one-line fix and
the trace names it.

**Step 5 -- round trip.**  `oggcomp d` reproduces every corpus `.ogg`
byte for byte.  `./mk.sh check` builds tuning and release forms of
oggcomp and proves they code identically, as it does for tsvcomp today.
`t-guard.sh` runs against oggcomp.

**Step 6 -- retire.**  `mk.sh` builds `oggcomp.cpp`; `opt.lst` names
`.ogg` files and `opt.pl` runs `oggcomp c`; the `Makefile` gains the
target; `tsvcomp.cpp` and `vd_setup.inc` are deleted; `main.cpp` stays.
`TSVCOMP-MODEL.md` section 1 is corrected to say the walk is balrogg's.
*Check:* the corpus number in `log.txt` is unchanged; the same bits are
visible to the optimizer.

**Step 7 -- measure.**  Wall time and resident memory on 07 and 08
against the table in section 1.  Expected: encode within a few percent of
`tsvcomp c` alone, decode likewise; resident memory no more than balrogg's
static buffers (213 MB declared: a 32 MB input window, a 96 MB codebook
arena, 68 MB of classification slots, a 16.6 MB gathered-packet buffer, of
which a stream touches what it needs) plus the model's touched pages.  Then the
first thing worth doing with the merged tool that the split one could
not: nothing in this plan, and that is deliberate (section 9).

Effort, in lines: about 150 in `io.inc`, `link_walk.inc`, `codec.inc`;
about 120 for `oc_sink.inc`; `oc_model.inc` is `tsvcomp.cpp` with 600
lines removed and 150 added; `oggcomp.cpp` is under 150.  The
`OC_TRACE` facility is 30.  Most of the time goes to step 4's diff.

---

## 7. Where the two walks disagree today, and which side wins

The two walks were kept in step by hand and a few places have drifted.
Each has to be settled before step 4 can pass, and the answer is always
"balrogg's walk, and the model adapts", because balrogg's is the one that
reads the bits.

- **Validation.**  `tsvcomp` re-checks ranges `io.inc` has already
  checked (residue class < 16, floor post < 32768, page lengths).  On the
  decode side those checks are what protect `io` from a corrupt `.oc`, so
  they stay in the sink's decode paths; on the encode side they are
  redundant and go.
- **The header tag ids.**  `tc_tagid` assigns ids in order of first
  sighting across the whole file and never resets.  `io.inc`'s tag
  literals arrive in the same order, so the ids match; but the sink must
  intern by string, not by literal address, because the same tag is a
  different literal at different sites (`cb.len` in `lengths()` twice).
- **`cst` fields.**  Not records, not coded, checked by `io`.  Same as
  today.
- **The comment header's length.**  `tc_header` passed the packet length
  to `su.comment`; `io::comment` loops over `len` itself.  Same records.
- **`flr.y` order.**  `io::payload` writes posts in ascending-X order
  (`srt`); the model codes in list order.  The `floor` method takes the
  whole array, so the order inside is the model's business and the
  records `oc_tsv` writes keep balrogg's order.
- **Digits per partition.**  For residue types 1 and 2, `io::rs_part`
  codes whole vectors -- `ceil(psz / dim) * dim` digits -- where
  `tc_part` codes exactly `psz`.  libvorbis reads whole vectors too, so
  balrogg is right, and a stream whose partition size is not a multiple
  of a book's dimension desynchronises tsvcomp today, one record after
  the partition ends.  No corpus file has such a book.  The sink counts
  digits per vector, as `io` does.
- **The page counter and `seq`.**  `tc_walk` counts pages per link for
  the expected sequence number; `vb_pack` does not.  The sink counts.
- **Multi-link streams.**  `vb_ctx::link` drops the setup; the sink's
  `link_begin` must reset exactly what `tc_walk`'s loop resets (`pg_prev`,
  `tc_prevW`, `tc_prevmode`, `pg_prevtype`, `pg_prevser`, `tc_used`) and
  nothing the file-wide state keeps (`tc_tlast`, the tag ids).  The corpus
  has no multi-link file, so this is checked by concatenating two.

---

## 8. Risks

- **Step 4 does not converge.**  The trace makes each disagreement
  specific, but a disagreement in the *context* rather than the value --
  a history keyed one packet off -- shows up as different coded bytes with
  identical traced values.  The trace therefore carries, for `digit`, the
  `tc_dv` it built.  If that is still not enough, the fallback is (a) from
  section 2: an in-memory `tsv` between balrogg's push and a tsvcomp walk
  run on a second thread through a bounded queue, which streams, removes
  the file and the text, and keeps the two walks.  It is worth less and
  costs less.
- **A behaviour the record stream had that oggcomp needs.**  `tsv::peek`
  and the meta roles in `vd_setup` (`su_role`, `cb_use`) are tsvtrans's
  and unused by tsvcomp; they go with `vd_setup.inc`.  Nothing else in
  `tsv.inc` is reached from the model.
- **Static buffers.**  balrogg declares 213 MB of static arrays and the
  model maps a gigabyte of address space.  Both fit as they do today;
  nothing is duplicated.
- **Windows.**  `tc_map` has a `VirtualAlloc` path and balrogg is static
  arrays; the merge adds no platform code.
- **The IDX machinery.**  `opt.pl` patches `!MAP!` strings in whatever
  binary it is given and measures whatever command it runs; pointing it
  at `oggcomp c` with `.ogg` paths is the whole change.  The tuning build
  is the same source with `Const 0`.

---

## 9. What this plan does not do

- **Change the model or the format**, beyond the container's magic.  The
  acceptance test is bit-exactness against `tsvcomp`, and that test is
  only worth having if nothing else moves at the same time.  The things
  the merged tool makes possible -- coding a digit knowing the length of
  the codeword it came from, which `io` has and the record stream never
  carried -- come after step 7, one at a time, measured.
- **Thread.**  The walk is sequential and the coder is sequential; there
  is nothing to overlap that the operating system's read-ahead does not
  already.
- **Remove balrogg.**  The record stream is the inspection format, and
  `oc_tsv` keeps it at the cost of one small adapter.  `tsvcomp` goes,
  because everything it did that balrogg does not is now `oc_model`.
- **Inline the model into `io.inc`.**  The sink boundary is what lets the
  same walk write records or code them, and what lets the model be tested
  against the records.  It stays a boundary.

---

## Appendix A -- every tag, and where it crosses tomorrow

| tags | sink method |
|---|---|
| `link.more` | `link` |
| `page.type`, `page.granlo`, `page.granhi`, `page.serial`, `page.seq`, `page.npkt`, `page.plen`, `page.tail` | `page` |
| `page.spill` | `spill` |
| `id.channels`, `id.rate`, `id.brmax`, `id.brnom`, `id.brmin`, `id.blocksize`, `id.framing` | `hdr` |
| `cmt.byte` | `hdr` |
| `cb.count`, `cb.dim`, `cb.entries`, `cb.ordered`, `cb.ordlen`, `cb.ordrun`, `cb.sparse`, `cb.used`, `cb.len`, `cb.lookup`, `cb.min.*`, `cb.dlt.*`, `cb.valbits`, `cb.sequence`, `cb.mult` | `hdr` |
| `flr.ntime`, `flr.count`, `flr.type`, `flr.parts`, `flr.pclass`, `flr.cdim`, `flr.csub`, `flr.cbook`, `flr.sbook`, `flr.mult`, `flr.rangebits`, `flr.x` | `hdr` |
| `res.count`, `res.type`, `res.begin`, `res.end`, `res.partsize`, `res.nclass`, `res.classbook`, `res.cascade`, `res.book` | `hdr` |
| `map.count`, `map.submaps`, `map.steps`, `map.mag`, `map.ang`, `map.mux`, `map.smtime`, `map.smfloor`, `map.smres`, `mode.count`, `mode.blockflag`, `mode.mapping` | `hdr` |
| `aud.mode`, `aud.wprev`, `aud.wnext` | `mode`, `wprev`, `wnext` |
| `flr.used`, `flr.y` | `floor` |
| `res.class` | `cls` |
| `res.digit` | `digit` |

77 tags, 10 methods.  The 60 header tags go through one method because
the header model's context is the tag itself; the audio tags get a method
each because their context is the walk's position, which is what the
merge exists to hand over.

## Appendix B -- the state the model keeps, and who resets it

| state | scope | reset by |
|---|---|---|
| model tables (`tcm_*`) | process | never; mapped zero at start |
| `tc_tlast`, `tc_tlast2`, tag ids, `tc_lasttag`, `tc_runpos` | file | never |
| `tc_last`, `tc_last2` (aux fields) | file | never |
| `pg_prev`, `pg_prevtype`, `pg_prevser`, `tc_prevW`, `tc_prevmode`, `tc_used`, the page counter | link | `link_begin` |
| priors (`tcp_book`, `tcp_cls`, `tcp_flr`), histories (`dg_*`, `cl_*`, `fl_*`), `oc_floor` | link | `setup_done` |
| `dg_pp`, `dg_ps`, `dg_pn`, `dg_q1`, `dg_q2`, `dg_zr` | residue call | `residue_begin` |
| `cl_last`, `cl_last2` | link | `setup_done` |
| `cl_run` | file | never -- kept so for bit-exactness; a candidate for `link_begin` after step 7 |
| `cl_same`, `cl_run_buf` | residue call | `cls` |
| the prior cursors | vector / classword / floor partition | `vec_begin`, `cls`, `floor` |
| `tc_stage`, `tc_bits`, `tc_syms` | file | never; `-v` only |

Whatever `tc_walk` resets today at the top of its link loop is what
`link_begin` resets; the list above is read off it and is checked by the
two-link concatenation in section 7.
