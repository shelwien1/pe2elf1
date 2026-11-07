# MTwrap parts mode: recent improvements, and how to port them

This note collects the changes made to h264p's multithreaded mode (`-t N`) between
October and November 2026, for porting to other codecs built the same way: Lib3's
stack-copying coroutines (`coro3b`) for the codec, and MTwrap_v3 in parts mode for the
threads. m4vpack is the other one in this repository.

There are two kinds of change, and they are ported differently:

- **MTwrap's own files** (`h264p_src/MTwrap_v3/*.inc`, and the statistics build
  `h264p_src/MTwrap_v3_dbg/`) are shared code: copy them, or apply the diffs.
- **The worker** (MTwrap's `Enc_class`: h264p's `H264pPart` in `h264p_src/mt.inc`,
  m4vpack's `M4vPart` in `m4vpack/m4vpart.inc`) belongs to each codec. Here a pattern has
  to be reimplemented, and some of MTwrap's changes rely on the worker behaving a certain
  way.

Section 6 lists exactly what m4vpack still needs.

## 1. The scheme, briefly

Compression (`c -t N`):

1. A splitter on the main thread cuts the input into parts and frames each as
   `[u64 L][L bytes]` (h264p: `h264split.inc`; the frame carries a small header of its
   own before the part's bytes).
2. MTwrap's coder (`loop_enc.inc`) deals each frame **whole** to the next worker in turn,
   through that worker's input ring.
3. Each worker codes its parts one after another, each with a model of its own (a
   coroutine of the worker's), and writes a stream of records and compressed parts to
   its output ring.
4. MTwrap writes the workers' output buffers into one container as they come
   (first come, first served), each as a block of up to 64 KB tagged with the worker's id.

Decompression (`d`):

1. MTwrap's decoder (`loop_dec.inc`) reads the container **in order** and gives each
   block to its worker's input ring.
2. Each worker decodes its parts.
3. MTwrap writes the workers' output **in turn** (part 0, part 1, ...). It switches to the
   next worker when the current one ends a round: the worker yields 4, and `thread_pipe`
   flags that output buffer with `flags |= 2`.

The numbers that matter, with h264p's `kMtRing` = 1 MB (`MTwrap::Init()`):

| | size |
|---|---|
| block in the container | at most 64 KB |
| a worker's input ring | 33 buffers of 64 KB, about 2.1 MB (`buf1_N1`: at least 2 MB) |
| a worker's output ring | 17 buffers of 64 KB, about 1.1 MB (`buf2_N`) |

Block headers (`loop_enc.inc`): `00iiiiii` is a block of 64 KB, `01iiiiii` one of
64 KB - 1, `10iiiiii UU VV` a tail block of length `UUVV` (its first two data bytes moved
to its end), and `11iiiiii` an empty block of one byte.

**Dynbuff.** When the decoder reads a block for a worker whose input ring is full, it can
put the block aside in `dyn_buff` instead of waiting. When to do that is the most
important policy in the decoder (section 2.4).

## 2. Changes in MTwrap's files

All of these are in h264p's `MTwrap_v3` and `MTwrap_v3_dbg`. As of 84d8d2e, the only
files of `MTwrap_v3` that differ from m4vpack's copy are `MTwrap.inc` and `loop_dec.inc`,
both by 84d8d2e (section 2.4). Everything else below is already in m4vpack's copy.

### 2.1 The coder writes no empty block after a worker's first (c86afa6)

`loop_enc.inc`: a worker's output buffer of length ≤ 1 (its id byte alone) is dropped
once that worker has a block in the container (`f_seen`). Before this, an empty buffer
was written as a one-byte block, which the decoder takes as the worker's input ending.
A stream cut right after such a byte then decoded as if it were whole.

The first block of each worker is still written, empty or not: the decoder wants a block
from every worker.

### 2.2 Dynbuff packs small blocks; running out of memory has a hook (f349114)

`dyn_buff.inc`: `Save()` appends a block to its worker's last node when it fits, instead
of giving every block a 64 KB node of its own. Before, a crafted container of 2-byte
blocks made `d` hold 1,650 times what it read (250 KB of them took 412 MB). Now 1 MB of
them takes 11 MB, so what a container can make `d` set aside is no more than the bytes
it has read.

`alloc0()` calls `MTWRAP_NOMEM()` if the codec defines it, instead of `__debugbreak()`.
h264p defines it as `err_fatal("out of memory")`; m4vpack as `no_memory()`.

### 2.3 Set-aside blocks go back first; workers know their index (5cba52a)

`loop_dec.inc`: at the top of every iteration, blocks set aside go back to any worker
whose input ring has room, before anything more is read. Before, a worker's blocks went
back only when the next block read was also that worker's. On random bytes
(`-t 4 -c 1`), `d`'s memory grew with the stream: 103, 199 and 371 MB at 64, 256 and
512 MB in. Now it stays at about 150 MB.

`MTwrap.inc`: `C1[i].Init(MArgs1, i)`. Each worker learns its index, so its records can
say where each part belongs (section 3.1).

### 2.4 The decoder reads on while another worker waits for input (84d8d2e)

This is the change m4vpack's copy does not have yet.

**The problem.** At a block whose worker's input ring was full, the decoder set the
block aside only while the worker whose output was next had nothing to do (its input
and output rings both empty). Otherwise it waited. So a run of one worker's blocks longer
than that worker's input ring (2.1 MB) held up all reading until that worker had decoded
most of the run, and every worker whose blocks came after the run sat idle. This is how
`d -t 32` on 16 cores ran on about one thread (section 4.1).

**The rule now.** At a block whose worker `w` has a full ring, the decoder sets the block
aside if any other worker is waiting for input, and reads on toward that worker's blocks.
`MTwrap::waiting(w)` checks for such a worker `k`:

```cpp
// decode: whether a worker other than w waits for input that the parse
// could reach: its input ring empty, no blocks of its put aside, its output
// ring not full (one waiting to write its output, in its turn, is not
// waiting for input), not ended
uint waiting( uint w ) {
  uint k;
  for( k=0; k<th_N; k++ )
    if( (k!=w) && !C1[k].f_done && (que1[k].inp()==0) && (que2[k].out()!=0) && !B.Load(k) ) return 1;
  return 0;
}
```

The decoder waits only when no worker could use what it would read next. Each condition
is there for a reason:

- **The output ring is not full.** A worker that has finished its part and waits for its
  turn to write is blocked on output, not on input. Without this condition the decoder
  would read the container into memory on its behalf, gaining nothing. An earlier rule,
  "set aside whenever the output worker's queue is empty for a moment", had exactly that
  effect: a valid archive ended up mostly in RAM.
- **Not ended.** A worker that has finished its stream never needs input again.
- **No blocks set aside.** Such a worker gets them back first (section 2.3).

**The worker contract this needs.** A worker that has read its stream's last record but
still reads on to confirm nothing follows is waiting for input until the container ends.
Under the new rule, the decoder would set aside everything left in the container for
it. So:

- **The worker must end at its own end-of-stream marker** (section 3.2) and leave the
  check for trailing data to MTwrap.
- **MTwrap reads the container to its end**, even after every worker has ended:
  `while( (f_eof != f_eof_all) || !(f_quit && InpBuf_l==0) )`. The driver sets
  `f_quit` at end of input. MTwrap takes anything after the workers' ends as broken,
  through the checks it already had:
  - a block for a worker that has ended (`C1[i].f_done`);
  - data left in a worker's input (`inp_left`);
  - blocks still queued (`que1[i].pending()`) or set aside (`B.Load(i)`).

**Also in 84d8d2e: an empty block after a worker's first is broken.** A one-byte `0xC0|id`
block after a worker's first block was taken as that worker's input ending. One or more
appended to a stream went unnoticed, in builds before this change too. The coder never
writes one (section 2.1), so the decoder now rejects it:

```cpp
if( ((i&0xC0)==0xC0) && ((f_seen>>(i&0x3F))&1) ) { f_err=1; f_quit=1; InpBuf_l=0; continue; }
```

**Measured.** On a container rewritten into runs of 4 MB of one worker's blocks (each
worker's block order kept; section 7), `d -t 4` on 4 cores:

| | before | after |
|---|---|---|
| time | 50–52 s | 43–45 s (the unmodified container: 40–44 s) |
| workers decoding, on average | 3.17 of 4 | 3.66 of 4 |
| worker time waiting for input | 11% | 0% |

On containers that h264p's `c` writes, nothing is set aside, as before. On 512 MB of
random bytes (`-t 4 -c 1`), at most 9.8 MB was set aside at once (2.6 MB before). Peak
memory still stays flat as the stream grows: 108, 135, 160 and 162 MB at 128 MB, 512 MB,
1 GB and 2 GB (before: 106, 126, 154 and 149).

### 2.5 MTwrap_v3_dbg and mtdbg.py (from m4vpack, 60cb7d1)

`MTwrap_v3_dbg` is MTwrap with an event log for the main loop and each worker, written to
`dbg.txt` when a run on threads ends. `mtdbg.py` summarizes it:

- each worker's coding time and its waits (for input, for room for output, ended);
- what the decoder was doing while workers waited (the "W<k> ring full" columns);
- how many workers were coding at once;
- the container's runs of one worker's blocks;
- dynbuff's peak.

This is the tool that found section 4.1. h264p builds on it with
`make CXXFLAGS="-O3 -DNDEBUG -DMTWRAP_DBG"` (`h264p.inc` picks the include). Then:

```
python3 h264p_src/MTwrap_v3_dbg/mtdbg.py dbg.txt
```

Keep it in step with MTwrap_v3: 84d8d2e changed both, and mtdbg's wording for the rule.

## 3. The worker's side: what each codec has to do

### 3.1 Records (formats 10–11: 5cba52a, 60cb7d1)

A worker's stream is a record before each part, then the part compressed. At the end
comes a record of its own. These records are m4vpack's:

| field | bytes | |
|---|---|---|
| `plen` | 8 | the part's size as compressed (with what the splitter inserted); 0 marks the worker's end |
| `ins` | 8 | where the inserted bytes start in the part (`d` removes `[ins, ins + xlen)`) |
| `xlen` | 8 | how many bytes were inserted |
| `part` | 8 | the part's place in the stream: worker w's k-th of N is `w + kN` |
| CRC-32 | 4 | of the 32 bytes above |
| zero | 4 | |

The end record has `plen`, `ins` and `xlen` all 0, and its `part` is the place the next
part would have had. `d` checks each record's place, CRC and zero word. After the run,
it checks that every worker had as many parts as were dealt to it in turn (`mt_dealt()`).
Together these catch a stream cut at a part boundary, and a part lost, doubled or moved,
none of which a part's own CRC can see. In h264p's test, every cut at a block boundary
and every block left out or doubled fails (65 of 65).

There is **no compressed size** in the record (format 11). That needs part streams that
**end where their decoder stops reading**: the coder writes exactly what its decoder
reads. h264p's range coder does that, and so does m4vpack's.

### 3.2 `d`'s worker

- **Read a part's compressed stream in place.**
  - `lend()` hands the part's decoder the rest of the worker's input buffer as if it had
    been read.
  - `take_back()` takes back what the decoder left: whatever lies past the part's
    stream, which is the next record.

  Nothing is copied, and a part is never held whole.
- **Bound a part's output by its record as it comes.** One byte more than `plen` is
  broken (h264p: in the `put()` that takes the part's output).
- **End at the end record.** Once the worker has read its end record and checked
  `ins == xlen == 0`, it returns. **Do not read on to see that nothing follows**: MTwrap
  does that (section 2.4). This is required with 84d8d2e's MTwrap. Without it, results
  are still correct, but at the stream's tail the decoder sets aside the rest of the
  container for the worker that waits for its input to end. That costs memory up to
  about one round of parts.
- **Optional (1e56b43): write the output as it comes, in turn.** See section 3.3.

### 3.3 `d`'s output, as it comes but in turn (1e56b43; optional)

MTwrap's decoder writes a worker's output only during that worker's turn. A worker that
writes before its turn blocks once its output ring (1.1 MB) is full. If every worker
simply wrote as it decoded, the others could get no more than 1.1 MB ahead of the one in
turn, and `d -t` would run at about one thread's speed. Holding each part's output until
the part is done (m4vpack's `d`, and h264p's before 1e56b43) avoids that, but holds every
worker's part, including the one whose turn it is.

h264p's middle way uses one counter the workers share (`H264pArgs::turn`, a
`uint64_t*`): the place of the part whose output goes next.

```cpp
// a worker's part output (l bytes at p) for the part at place
void give(const uint8_t* p, size_t l, uint64_t place) {
  if (!l) return;
  if (__atomic_load_n(args.turn, __ATOMIC_ACQUIRE) != place) {  // not its turn yet: hold it
    out_.append(p, l);
    return;
  }
  if (out_.size()) write(out_.data(), out_.size()), out_.clear();  // its turn: what it held, then as it comes
  write(p, l);
}
// at the part's end: what it still holds (blocking until its turn), then the next part's turn
if (out_.size()) write(out_.data(), out_.size()), out_.clear();
__atomic_store_n(args.turn, place + 1, __ATOMIC_RELEASE);
yield(this, 4);  // the round's end
```

The counter only decides when a worker stops holding output. MTwrap's turn order is what
keeps the output in order, so a worker that starts writing slightly early just waits on
its own ring until MTwrap reaches it. Each worker still decodes up to one part ahead. The
one in turn holds nothing, and output starts with the first part.

Measured: `d -t 4` of 31 MB in parts of 16 MB peaked at 63 MB instead of 77, at the same
speed. Most of `d`'s memory is the models, so this is a small gain.

### 3.4 `c`'s worker

- **Read the part whole before coding it** (39911da; m4vpack's `c` does this). MTwrap's
  coder deals a frame *whole* to one worker before dealing to the next. A worker that
  read its part only as fast as its model consumed it stalled the dealing as soon as the
  part was bigger than its input ring (2.1 MB), and the other workers went idle:
  `c -t 16` of 124 MB in 4 MB parts used 1.96 of 4 cores and took 88 s. Read whole: 3.94
  cores, 43 s. Small parts hide this: no part of 1 MB or less shows it.
- **Write the output as it comes** (d9e1512; m4vpack's `c` does this). This is what
  matters most for `d`, because MTwrap's container stores blocks in the order the coder
  hands them over:
  - Written as it comes, a part's blocks are spread across the time it took to code,
    among the other workers' blocks. In 1,615 blocks there were 1,587 runs.
  - Written all at once at the part's end, a part becomes one run of a single worker's
    blocks, a part long, and `d` stalls on each run (section 4.1).

  The precondition is a codec that never takes output back. h264p used to restart a
  part when its model pass found a slice that would not round-trip, so it had to hold
  the part's output. Format 12 removed the restart: such a slice's bytes follow its
  coded syntax, behind a flag.
- **End a round after each part** (c86afa6): after a part's output, `yield 4`, as `d`'s
  worker does. `thread_pipe` hands over the last buffer as far as it is filled, so each
  part's end reaches the container when the part is done. Before, a worker with small
  records kept them until 64 KB of them had built up, thousands of parts. `d` then had to
  read past, and set aside, all the other workers' blocks to find the next part. On
  parts alternating between incompressible bytes and zero bytes, `d`'s peak was 170 MB at
  256 MB in and 1.67 GB at 3 GB; after the fix, 47 MB.
- **Write the end record at the end.** It is the worker's last bytes, so the
  container's last block for each worker ends in data, and a cut is caught.

## 4. What went wrong, and why each change exists

### 4.1 `d -t 32` on 16 cores ran on about one thread

`c` from before format 12 held each part's output and wrote it at the part's end. The
container therefore held runs of one worker's blocks, each a part long: up to 3.4 MB in
4 MB parts, and 6.7 MB where two came together. Each run was longer than an input ring,
so `d`'s decoder waited on that one worker (section 2.4's old rule) while the others
emptied their rings and went idle.

MTwrap_v3_dbg on `d -t 32` of 124 MB (the corpus 32 times) in 4 MB parts, on 4 cores:

| | `c` held its output | `c` writes as it comes |
|---|---|---|
| runs of one worker's blocks | 156, up to 3.4 MB | 1,587 in 1,615 blocks |
| workers decoding, on average | 14 of 32 (44%) | 28.4 of 32 (89%) |
| their time waiting for input | 31%, 98% of it behind a full ring | about 2% |

On 4 cores the elapsed time barely changes, because 14 busy workers already fill 4
cores. On 16 cores the other workers drain their rings fast and then wait on the one
decoding the run, which is the single thread reported. In 2 MB parts (runs of about
1.5 MB, which fit a ring) it did not show, nor in the 1 MB and smaller parts of the tests.

Each fix addresses one cause:

- **The container's order:** `c` writes as it comes (3.4).
- **The coder's dealing:** `c` reads its part whole (3.4).
- **The decoder's rule:** reading on while another worker waits (2.4), so even a bursty
  container from anywhere decodes in parallel.

### 4.2 The decoder's rule for setting blocks aside

There are two ways to get it wrong:

- **Too loose: set aside whenever some queue is empty for a moment.** A valid archive
  ends up mostly in memory (MTwrap's history).
- **Too tight: set aside only when the output worker is idle.** The decoder stalls behind
  any run longer than a ring (4.1).

`waiting()` sets blocks aside only when doing so lets a worker that can use input get
it. A worker blocked on output does not qualify, nor does one that has ended. With
crafted input, the bound stays at "no more than the bytes read, packed" (2.2).

### 4.3 Things that look harmless and are not

- **A worker that reads one byte after its end record** "to be sure". With 2.4's rule it
  waits for input until the container ends (3.2).
- **Streaming `c`'s input to save memory.** The coder deals frames whole, so the dealing
  stalls (3.4).
- **Streaming `d`'s output naively.** Workers out of turn block after 1.1 MB (3.3).
- **Testing with small parts only.** Every problem here needs parts larger than a ring
  (about 2 MB compressed). h264p's default part size is 4 MB.

## 5. Results for h264p, in short

On 4 cores, for 124 MB of the corpus in 4 MB parts:

| | before format 12 | now |
|---|---|---|
| `c -t 16` | 3.92 cores, 49 s | 3.94 cores, 43 s |
| `d -t 16` | 3.14 cores, 51 s | 3.92 cores, 40 s |
| `d -t 8`, workers decoding | 5.6 of 8 | 7.2 of 8 |

## 6. Porting to m4vpack

What m4vpack already has (as of `claude/cool-goldberg-eyzxmq`, 8790c6e):

- the records of 3.1, `lend()` and `take_back()`;
- `c` reading its part whole and writing its output as it comes;
- `yield` of `Y_PART` after each part, on both sides;
- `MTWRAP_NOMEM`;
- MTwrap_v3_dbg.

What it needs:

1. **MTwrap_v3:** apply 84d8d2e's diff to `m4vpack/MTwrap_v3/MTwrap.inc` (`waiting()`)
   and `m4vpack/MTwrap_v3/loop_dec.inc` (the rule, reading to the container's end, and
   the empty-block check). These are the only two files that differ; the rest of
   `MTwrap_v3` is byte-identical to h264p's.
2. **MTwrap_v3_dbg:** the same two diffs. Optionally also take mtdbg's new wording for
   the rule (84d8d2e), and the `char th[12]` that silences GCC's `-Wformat-truncation`
   in `mtdbg_dump()` (60cb7d1). Keep m4vpack's own docstrings in `mtdbg.inc` and
   `mtdbg.py`; h264p's differ only in naming h264p.
3. **`m4vpart.inc`, `run_d()`:** at the worker's end record, return without
   `read(&x, 1)`:

   ```cpp
   if (!plen) {  // the worker's end
     if (ins || xlen) return fail(E_DATA, "damaged data: a part's record broken");
     return 0;   // (what follows is MTwrap's to find: it reads the container to its end)
   }
   ```

   Trailing data then fails through MTwrap instead (`f_err`: the run ends in yield 3,
   not `Y_END`), which `call.inc` already reports as `E_DATA`, "damaged data".
4. **Optional:** stream `d`'s output in turn (3.3). It needs a shared counter in
   `MtArgs`, `give()` in place of `out_len += ...` in `run_part()`, and the
   inserted-bytes removal done as the bytes pass. The gain is small (3.3).

## 7. How to verify a port

- **Round trips** at `-t` 2, 3 and 4, with parts both smaller and larger than 2 MB
  compressed (h264p: `-c 16k` and the default 4 MB).
- **A broken container must fail.** h264p's `test/h264p.sh` does each of these:
  - a cut at every block boundary;
  - every block left out, or doubled;
  - a record's `xlen` changed;
  - 1–3 bytes appended, `\300` among them (an empty block after a worker's data);
  - 65,536 blocks of 2 bytes past a waiting part, and a part stream of 128 MB: both
    fail as broken, within a fixed address-space limit.
- **Bursty containers decode in parallel.** Rewrite a valid container into long runs of
  one worker's blocks. Each worker's block order is kept, so the result stays valid. Then
  compare time and MTwrap_v3_dbg's figures (workers decoding, waits for input) against
  the original container. `regroup.py` below does this for an h264p container; the
  block format is MTwrap's, so only the 11-byte header length is codec-specific.

  ```python
  # regroup.py IN OUT R: a -t container with each worker's blocks in runs of R (order kept)
  import sys
  src, dst, R = sys.argv[1], sys.argv[2], int(sys.argv[3])
  HEAD = 11  # h264p's header (magic, version, threads, part size); the codec's own
  d = open(src, "rb").read()
  pos, q, order = HEAD, {}, []
  while pos < len(d):
      b = d[pos]; t = b & 0x3F; k = b & 0xC0
      if k == 0x80: n = max(((d[pos + 1] << 8) | d[pos + 2]) + 2, 5)  # tail block
      elif k == 0x00: n = 65536
      elif k == 0x40: n = 65535
      else: n = 1                                                    # empty block
      if t not in q: q[t] = []; order.append(t)
      q[t].append(d[pos:pos + n]); pos += n
  assert pos == len(d)
  out = [d[:HEAD]]
  while any(q.values()):
      for t in order:
          if q[t]: out.extend(q[t][:R]); del q[t][:R]
  open(dst, "wb").write(b"".join(out))
  ```

  With `R = 64` (4 MB runs), the old rule shows the stall and the new one does not
  (2.4).
- **Memory stays flat.** Decode streams of random bytes (`-t 4 -c 1`) of 128 MB up to
  1–2 GB and compare peak resident memory: it should level off, not grow with the
  stream.
- **No hangs on corrupted input.** Flip bits in `-t` containers: `d` must fail or give
  back the original, never hang. h264p ran 300 such containers after 84d8d2e.
