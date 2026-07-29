# Plan 5 — full streaming

Two questions, answered first, because one of them has a much smaller answer than
it looks and the other has a much bigger one.

## Do we need the large slabs?

**No. They are 8× oversized, and that is a sizing mistake in Plan 4, not a
requirement.** `cbuf` and `code` are per-channel *slices of one block* — the
channels divide a block between them, they do not each get one:

```
hdr = nch*4;  body = len - hdr;  groups = body/hdr;  cbytes = groups*4
    =>  nch * cbytes  =  hdr * groups  <=  body  <=  ba
    =>  nch * ncode   =  nch*cbytes*8/bps        <=  ba*8/bps
```

So the sum across channels is bounded by `ba` bytes and `ba*8/bps` codes, and
`[MAX_CHANS][BA_MAX+8]` / `[MAX_CHANS][BA_MAX*8/2+8]` pays for `MAX_CHANS` of
them. One flat slab with per-channel offsets:

| | now | flat |
|---|---|---|
| IMA nibbles | 524,344 | 65,599 |
| IMA codes | 8,388,736 | 1,048,816 |
| MS nibbles | 131,078 | 131,078 (genuinely `ba*2`, unpacked; no channel split) |
| **total** | **9.04 MB** | **1.25 MB** |

About twenty lines, no format change, no effect on output. **Do this
independently of everything below** — §3.

## Can the algorithm stream?

**The algorithm already does. The container format does not.** Nothing in the
model reaches outside the block in front of it:

* `code_bytes` is a sequential order-1 walk whose context is the byte it just
  coded — no back-reference into the image.
* `code_data_ima` / `code_data_ms` address `blk = data+pos` and nothing else.
* `Pred`/`Hist` are bounded by tap count; `MatchModel` owns its arena and is
  sized independently of the input; `RC` is already a coroutine over pins.
* `crc32b` is a rolling computation that was simply being handed a whole image.
* **Measured, and this is the one that mattered:** the row indices use the
  *segment's* `nsym`/`nib_rows`, set in `begin_seg`, while `amaxsym`/`amaxnib`
  only size the arena. Forcing the arena to the format maximum — what a streaming
  decoder must do, since it cannot know the archive-wide maxima in advance —
  produces **all 24 md5s identical**. Not merely the same sizes; the same bytes.
  The thing that looked like streaming's headline cost is free.

So this plan is about the container, not the codec.

## And what it is worth, stated up front

**Not footprint.** `src` is 2.5 MB of a 557 MB RSS on `wavs2` — the 512 MB match
arena dominates and this plan does not touch it. Streaming buys **capability**:
input from a pipe, inputs larger than RAM, and output that starts before the
input ends. If the goal were a smaller resident set, the match arena is the file
to open, not this one.

---

## 1. The four things that are not streaming, and exactly why

| | blocker | consequence |
|---|---|---|
| **B1** | the header carries `nseg`, `lead` and every segment's `Params` **before** the first coded byte | the encoder must scan the whole input before it can emit anything |
| **B2** | `pm.tail_size = extent - (head + data)`, and `extent` is the distance to the **next** wav (`xad_wav.inc:158`) | a segment's descriptor cannot be written until the next wav has been found — unbounded lookahead |
| **B3** | `crc32` sits in the header, and `do_decode` verifies it **before** `putn` | the decoder materialises the entire output image |
| **B4** | the file table's `sizes` are accumulated by `feed_files` as the pump runs | they are only final at EOF, yet they are written in the header |
| B5 | `-t` compares the decoded image against `Menc.src` | inherent to `-t`; stays buffered, and that is correct |

B2 is the interesting one. It is not a lookahead the *codec* wants — it is an
artifact of deciding that the bytes between two wavs belong to the earlier one's
record. Change that and the lookahead disappears.

---

## 2. Staging — three steps, each shippable alone

1. **§3 slab fix.** No format change, no output change. Independent of the rest.
2. **§4 streaming decoder, existing v4 format.** Possible *today*: v4's header
   already describes every segment before the rc stream, so the decoder knows
   everything it needs. The only property lost is crc-before-write.
3. **§5–§6 streaming encoder, format v5.** The one that needs a format change.

If only part of this is wanted, 1 and 2 are worth having on their own and 2 is
what makes `xadpcm d big.xac - | player` possible.

---

## 3. Slab fix (no format change)

Replace the two `[MAX_CHANS][...]` slabs with flat ones plus per-channel offsets:

```
static u8  scr_nib [BA_MAX + MAX_CHANS*8];
static int scr_code[BA_MAX*8/2 + MAX_CHANS*8];
```

In `code_data_ima`, `cbytes` and `ncode` are already computed per block; give
channel `k` the slices at `k*cbytes` and `k*ncode`. The `+ MAX_CHANS*8` is
alignment slack so each slice can start on an 8-byte boundary — `get_bits`/
`put_bits` read past the end of the live prefix by up to a word, which is what
the existing `+8` was for.

Cheap, and it must not move a byte: `md5s.sh` unchanged is the whole test.

---

## 4. Streaming decoder against the existing format

`Xad<1>::src` becomes a **window**, not an image.

* Size it `WIN = max(BA_MAX + head_max, 1<<20)`. A block is decoded into the
  window and pushed to the sink; the window is then reused.
* `write_canon` writes into the window rather than at an absolute offset.
* `at`/`orig_size` bookkeeping becomes a running counter for the split sink,
  which already consumes a byte stream (`sink_split`) and does not care where it
  came from.
* `crc32b` becomes rolling: fold each window into a running crc before pushing.

**The one property that changes: crc-before-write.** Options, and the
recommendation:

* **Default (recommended): write as you go, verify at the trailer, and remove
  the output on mismatch** — `unlink` the file, and every split member already
  closed. That keeps "no bad output survives a run" for file output, which is
  what the property was actually for.
* **Pipe output cannot have it.** Bytes already down the pipe cannot be recalled.
  Report on stderr, exit non-zero, and say so in the usage text.
* **Keep `-b`** for today's exact behaviour: buffer the whole image, verify, then
  write. One flag, and the old code path is the fallback rather than a rewrite.

No format change. `verify.sh` and `md5s.sh` must be unchanged by this step —
decode output is compared byte-for-byte either way.

---

## 5. Format v5

Version 4 puts a table of contents in front of one rc stream. Version 5 makes
the stream self-describing, so nothing needs to be known in advance:

```
XAC1 | ver=5 | solid | <rc stream>

rc stream, a tag loop:
  LITERAL  varint n, n bytes through the LIT_GAP/LIT_TAIL model
  SEGMENT  Params (canonical, format, chans, bps, block_align, data_size,
                   head_size or rate+factdiff, MS coefficients)
           then head (literal, or regenerated when canonical)
           then data, via code_data
  END      varint nfile, the file table, crc32 of the whole reconstructed stream
```

What each blocker becomes:

* **B1 dissolves.** A segment's descriptor is emitted when the segment is
  recognised, which is as soon as its RIFF header has been parsed.
* **B2 dissolves.** Inter-wav bytes are a `LITERAL` run, not part of the previous
  segment's record. The encoder never needs to know where the next wav starts.
  **This is the change that makes streaming possible at all.**
* **B3, B4 dissolve.** The crc and the file table move into `END`, where both are
  known.

**Cost.** One tag per record, plus the varint lengths that today are implicit.
Estimate a handful of bytes per segment against v4; the `Params` bytes are the
same bytes, only moved. The acceptance criterion must reflect that (§8).

**Compatibility.** The tree has no back-compat machinery and v4 has not shipped
anywhere. Recommendation: **v5 replaces v4**; `VERSION` becomes 5 and the reader
rejects 4 with a clear message rather than carrying two parsers. If a v4 reader
must survive, say so before this step starts — it is a fork in `do_decode`, not
a detail.

---

## 6. Streaming encoder

`slurp()` — today, read the whole input — becomes a sliding window over `pin[0]`.

* **Window size.** Must hold (a) the largest RIFF header `analyze` will parse,
  and (b) one block. `WIN = 1<<20` covers both with margin; the invariant to
  assert is `WIN >= BA_MAX + max_header_extent`.
* **Incremental scan.** `memchr` for `'R'` inside the window; on a candidate too
  close to the end to parse, refill and retry. Bytes that are not a wav are
  flushed as `LITERAL` as the window advances, so junk of any size costs nothing.
* **crc32** rolls over the window as it advances.
* **The file table** is emitted in `END`, by which time `feed_files` has finished
  and `in_sizes` is final.

### The detail that needs the most care

`analyze` clamps a declared `data_size` against what is actually available
(`xad_wav.inc:36`, `w.data_size = avail`) — that is how a truncated final wav is
handled. A streaming encoder does not know `avail` until EOF.

Two ways out, and the second is preferred:

1. Defer the `SEGMENT` descriptor until the data has been consumed. Costs a
   buffer the size of the data — i.e. it defeats the purpose.
2. **Emit the declared `data_size`, and let a short read terminate the segment
   early with an explicit `SEGMENT_SHORT` marker carrying the count actually
   coded.** Constant space, and it makes truncation an explicit thing in the
   format rather than something inferred from arithmetic. The synthetic corpus
   already has the case (`gen/ima_b3_c2_odd.wav`, `gen/ms_c2_odd.wav`), so it is
   testable from day one.

---

## 7. What is still buffered afterwards, and why that is right

| | size | why |
|---|---|---|
| match arena | 512 MB | model state; unrelated to input size |
| counter arena | 27 MB | same |
| per-block scratch | 1.25 MB after §3 | one block, at the format's own maximum |
| encode window | 1 MB | RIFF header parse + one block |
| `-t` | input + archive | the point of `-t` is to compare the two |

Peak RSS goes from `540 MB + input` to `540 MB + ~2 MB`, flat in input size.
On today's test files that is a 0.45% saving; on a 4 GB wav it is the difference
between working and not.

---

## 8. Risks, and the check for each

| risk | check |
|---|---|
| **v5 is not byte-identical to v4 — by construction.** Tails move from the segment record into `LITERAL` runs, so the same input segments differently | the acceptance criterion changes to **lossless round-trip, and total size ≤ v4 + a few bytes per segment**. Say the expected delta before measuring it, not after |
| a truncated final wav | §6, `SEGMENT_SHORT`; `gen/*_odd.wav` and a deliberately half-cut wav |
| window too small for some RIFF header | assert `WIN >= BA_MAX + header extent` at parse time and fail loudly, never silently truncate |
| decode writes bytes that fail the crc | §4: unlink on mismatch for files; documented and non-zero exit for pipes; `-b` restores the old order |
| split output leaves half-written members after a crc failure | unlink every member opened this run, not just the current one |
| the file table check `named > total` happens before any output today | in v5 `total` is not known until `END`; move the check there and treat it as a decode failure with the same unlink path |
| solid-mode carry-over across segments | unaffected — `begin_seg`/`reset_stats` already key off the *previous* segment only, which a streaming encoder still has |
| model geometry unknown in advance | **already measured: free.** §0 |

---

## 9. Verification

The standing suite (`md5s.sh` in both build modes, `verify.sh`, `ktest.sh`,
`sqtest.sh`, the compiler matrix), plus, for this plan specifically:

* **§3 and §4 must not change a byte** — `md5s.sh` unchanged is the gate.
* **§5–§6 change the archive**, so the gate becomes: every file in the corpus
  round-trips, and v5 total ≤ v4 total + (bytes/segment × segments), stated in
  advance.
* **A pipe test**: `cat wavs2 | xadpcm c - - | xadpcm d - - | cmp - wavs2`.
  This is the whole point of the plan and nothing else proves it.
* **A larger-than-window input**: a synthetic wav well past `WIN`, and a
  concatenation with megabytes of junk between two wavs, which is precisely the
  case B2 could not stream.
* **An RSS bound**: peak resident must be flat in input size. Assert it, do not
  eyeball it — measure at 20 KB, 2.5 MB and 200 MB and require the three to
  agree within a few MB.
* **The coroutine stack high-water** stays ~1345 bytes. A sliding window whose
  buffer lands on the coroutine stack would be the same 64 KB mistake Plan 4
  documents; the window is a member, like `nmbuf`.

---

## 10. What this plan does not do

* Touch the match model's 512 MB. That is the actual memory story and it is a
  different plan.
* Change the model, the tables, or any tunable.
* Add a v4 reader alongside v5 (§5) unless asked.
* Make `-t` streaming. It cannot be, and should not pretend to be.

---

# Implemented

All three steps. `verify.sh`, `ktest.sh`, `sqtest.sh` green; Debug == Const; six
compiler/flag configurations byte-identical; UBSan clean.

**§3, slab fix — output-identical.** BSS 9.58 MB → 1.78 MB, all 24 md5s
unchanged. It also exposed a hole the vectors had hidden: `get_seg` bounded
`block_align` from below and never from above, so a hostile header memset a
megabyte into a 64 KB slice. Reachable, verified with an 88-byte archive, and now
a regression test. The per-call scratch clear turned out to be dead — poisoning
the slab with 0xAA left every archive byte-identical, which corrects the claim
Plan 4 attached to it.

**§4, streaming decoder — output-identical, no format change.** Peak RSS drops by
exactly the input size. The crc moves from before the first byte to after the
last, so `discard_output()` takes the output back on failure (every member, for
split output); `-b` restores the old order; `-` means stdin/stdout.

**§5–§6, streaming encoder and format v5.** Encoder RSS is now flat in input size
too. Against v4 over the 24 archives: **−2439 bytes total**.

| | v4 | v5 | |
|---|---|---|---|
| total, 24 archives | 8,732,576 | 8,730,137 | **−2439** |
| `wavs2` | 1,248,335 | 1,248,346 | +11 |
| `multi(2) -ss` | 1,606,697 | 1,604,315 | **−2382** |
| counter arena | 27.4 MB | 27.7 MB | |
| peak RSS, `wavs2` | 559.5 MB | 556.5 MB | |

## Three things the plan got wrong

**The file table cannot live in the trailer.** §5 put it in `T_END` with the crc.
It is the one piece of container metadata a reader needs *before* the first output
byte — the split sink cuts the decoded stream on it — so a multi-member archive
decoded into a directory failed outright. It is in the preamble, and the encoder
knows it up front because the driver seeks each input before the coroutine starts.

**Sizing at the format maximum is not free after all.** The measurement in §0 was
right that it does not change the output, and wrong to conclude it was therefore
free: `tP4` is `nsym^5`, so 4-bit → the 5-bit maximum takes the counter arena from
27 MB to **321 MB**. It grows on demand instead (`Codec::grow_geom`), which both
directions do at the same segments because both read the same record. The
counters do not survive the resize — the same thing a non-solid boundary already
does. That turned out to *help*: `multi(2) -ss` gained 2382 bytes, because
resetting when the alphabet widens beats carrying statistics across the change.

**`SEGMENT_SHORT` was not needed.** §6 proposed a marker for a truncated final
wav. The chunk list subsumes it: the encoder commits to nothing it has not read,
so a short file is just a short last chunk.

## Two bugs worth recording

The encoder emitted `T_SEG` *after* `reset_stats`/`begin_seg`; the decoder cannot
reset anything until it has read the record telling it to. It decoded the first
segment perfectly and fell over on the second. Records now go out before the
model moves.

`ew_eof()` was `f_quit && window empty`. At end of input the window still holds
the 11 bytes held back for a straddling RIFF, so it read false, the bytes were
held back again, and the loop exited with them unwritten — 11 bytes short, caught
by round-trip and nothing else.

## What is still buffered

`-t` (it compares the archive against the input, so it holds both); one block; the
1 MB scan window; the 1 MB output window; the model arenas, which are 540 MB and
unrelated to input size. Peak RSS is flat in input size in both directions.
