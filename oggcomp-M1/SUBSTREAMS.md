# Interleaved range coding: several coders, several substreams

An idea for making oggcomp faster, looked into and measured on this tree
(commit 1ab694b, `g++ -O2 -fwrapv`, one Xeon core at a measured 3.2 GHz).
The short of it: the coder is not where the time is.  Interleaving would
buy the encoder at most a few percent and the decoder less, for a format
change; section 5 says what would buy more.

## 1. The idea

A binary range coder is a chain.  Coding one bit reads `range`, scales it
by the probability, picks a side, writes `range` back, and renormalises
when it has shrunk -- and the next bit cannot start until that is done.
On an out-of-order core the chain is some fifteen cycles long, and the
rest of the core sits idle for most of them.

Run N coders instead of one, each with its own `low`/`range`/`code` and
its own byte stream, and hand consecutive symbols to them in turn.  The N
chains are independent, so the core overlaps them: the throughput of the
coding step goes up by nearly N until the issue width is reached.  It is
the trick behind interleaved rANS in the fast entropy coders and behind
the two-coder LZ decoders; it works because in those the coder *is* most
of the work per symbol.

The output is then N streams.  Either they are written whole and
prefixed with their lengths, which costs a buffer the size of the output,
or they are framed in blocks -- every so many symbols a block header with
the N lengths -- or, for N = 2, one coder writes a block forward and the
other backward from its end, which needs the block in memory anyway.  All
of it is bytes: the model does not know, the decoder mirrors the
assignment, the `.oc` version goes up.

## 2. What the coder costs here

Everything below is `music-stereo-q5.ogg` (167,685 bytes, the nearest
thing in the corpus to music) unless it says `big.ogg`, seven corpus
files concatenated (1,106,690 bytes).

**Binary decisions.**  An instrumented build counts calls to
`rc_Process`, which is where every bit of the model meets the coder:

| file | input bytes | decisions | per input byte | output bytes | output bits per decision |
|---|---|---|---|---|---|
| tiny-8k-q0.ogg | 2,763 | 13,974 | 5.1 | 1,346 | 0.77 |
| music-stereo-q5.ogg | 167,685 | 1,851,056 | 11.0 | 144,389 | 0.62 |
| big.ogg | 1,106,690 | 12,639,021 | 11.4 | 960,306 | 0.61 |

**Time per decision.**  Best of three, wall clock:

| file | encode | decode | ns per decision (enc) | cycles at 3.2 GHz |
|---|---|---|---|---|
| music-stereo-q5.ogg | 0.294 s | 0.303 s | 159 | 510 |
| big.ogg | 1.49 s | 1.45 s | 118 | 380 |

**Instructions per decision.**  callgrind on the music encode counts
2.34 G instructions, 1,260 per decision, so the core is retiring about
2.5 instructions per cycle: it is busy, not waiting.  Where they go, by
source file (self cost, encode; decode is within a point of it):

| where | share | what it is |
|---|---|---|
| `cm.inc` | 27% | four counters, two APM stages, the 7-input mixer and the mantissa mixer, their updates |
| `MOD/tsvcomp-dig_p.inc` + `sgn_p.inc` | 24% | `tc_make_dig` / `tc_make_sgn`: the context hashes from the IDX declarations, about 610 instructions per residue value |
| `tc_fam.inc` | 10% | `code()`: the unary/length/mantissa walk, row selection, prefetches |
| `oc_residue.inc` | 10% | `digit()`: gathering the ~30 context inputs (`q1`, `q2`, `t1`, `t2`, neighbours, averages) |
| `tc_base.inc` | 7% | `tc_qlog`, `tc_sq` and the like |
| `rc.inc` | 4% | the coder: `rc_Renorm`, `rc_Process` |
| `vb_packet.inc` | 3% | the bit-level Vorbis parse |

**Memory.**  cachegrind on the same run: 776 M data references, 3.7 M
D1 misses (two per decision), 112 K last-level misses in all -- six
hundredths per decision.  The tables reserve 366 MB but a file this size
touches a working set that stays in cache.  So the time is instruction
count, not latency, at this size.

**The coder taken out.**  A build whose encoder-side `rc_Process`
returns at once (the model runs unchanged, the output is garbage) encodes
`big.ogg` in 1.34 s against 1.49 s: the coder, its renormalisation and
its byte output together are 10% of the encoder's wall time, about 38
cycles per decision.  That is the most any change confined to the coder
can save on encoding; interleaving saves some fraction of it.

**The coder alone.**  A benchmark that includes `rc.inc` over a memory
buffer and codes 16 M bits, one coder against two and four interleaved
bit by bit, on this machine:

| model | coders | encode ns/bit | decode ns/bit |
|---|---|---|---|
| probabilities precomputed (pure chain) | 1 | 5.7 | 5.4 |
| | 2 | 4.1 | 4.2 |
| | 4 | 3.3 | 3.2 |
| an adaptive counter per coder, context its own last bit | 1 | 7.0 | 8.4 |
| | 2 | 8.6 | 10.0 |
| | 4 | 8.5 | 9.4 |
| one counter set, context the previous bit whoever coded it | 1 | 7.4 | 9.7 |
| | 2 | 8.7 | 10.9 |
| | 4 | 8.4 | 10.2 |

(clang gives the same shape at 5.0 / 3.9 / 3.3 for the pure chain and
5.0 / 4.6 / 5.2 with the counter.)  Bare, the coder is a 17-cycle chain
and four of them interleaved reach 10 cycles per bit.  Add one adaptive
counter -- forty-odd instructions per bit in all -- and the core is
issue-bound already: interleaving gains nothing, dependent or not.
oggcomp's model is thirty times that counter.

## 3. What could be split, and what could not

Interleaving is free only where the model does not chain the decisions.
In the encoder every bit is known in advance, so the model's work for the
next decision overlaps the coder's chain for this one already, and the
out-of-order core does the interleaving by itself; the 38 cycles above are
what is left after that.  In the decoder the model for decision k+1 needs
the value of decision k, so what matters is whether the *model* chains --
and here it does, everywhere it counts:

- **Residue digits** are 94% of the output and most of the decisions.
  Each digit's contexts (`oc_residue.inc`, `digit()`) include the previous
  two values in the vector (`q1`, `q2`), the zero run (`zr`), the class
  run, and the same position in the previous two frames and the other
  channel of an interleaved residue (`t1`, `t2`, `x1`).  The next digit
  cannot begin before this one's value is known, signed: the sign is
  coded after the magnitude, its context depends on the magnitude, and
  the signed value is the next digit's `q1`.
- **Within a packet** the parse is sequential by construction: the
  decoder writes bits in packet order, and which field comes next depends
  on values already decoded (a floor's "used" flag, a codeword's length).
  Floor, class and residue of one packet cannot go to separate coders
  running ahead of each other.
- **Across packets** nothing in the parse chains, but the model does:
  `t1` and `t2` are the previous frames, `cl_hist` the previous classes,
  and that is a good part of the compression.  Two packets in two coders
  would need two models, which is a different compressor with a worse
  ratio, not an interleaving of this one.
- What does not chain -- page header fields, comment bytes, the
  end-of-stream CRC -- is under 2% of the decisions on any audio file.

So there is no assignment of decisions to substreams that lets the
decoder run two model chains at once.  Interleaving in the decoder can
only take the coder's own latency off the one chain: about 17 cycles of
the 380 to 510 that a decision takes, or 3 to 4%.

## 4. What it would take to do it anyway

For the record, the shape of it:

1. `oc_model<f_DEC>::rc` becomes `rc[N]`, and `tc_bit` picks one by a
   rule both sides can follow with nothing coded: alternate per decision,
   or one per family (`dig`, `sgn`, `flr`, `cls`, `aux`, `hdr`).  Per
   family is simpler to reason about but skews the load onto the `dig`
   coder, which is most of the decisions; alternating balances it but
   makes every decision's coder a state to carry.
2. Each coder has its own `rc_pin_io`, its own over-read count against
   `RC_OVER_MAX`, and `rc_Quit` runs N times.
3. Framing: the pin is one byte stream, so either the N streams are held
   in memory and written with lengths at the end -- the encoder is a
   coroutine over a pin precisely so that it need not hold the output --
   or the stream is cut into blocks of B decisions, each block a small
   header with N lengths and the N pieces.  Blocks cost N flushes each
   (`rc_Quit` leaves 4 to 8 bytes per coder) and a `rc_Init`, so B has to
   be large -- 64 K decisions, say, for a tenth of a percent -- which is
   fine for a file and awkward for a stream cut short.
4. `oc_check` and `OC_VER` move: the CRC at the end goes into the last
   block; the version becomes 5 and every `.oc` before it is refused,
   as now.  oggdet's segments are unaffected: a segment is one framed
   stream whatever is inside it.
5. `./mk.sh check` and `./t.sh` as they are; the coder benchmark above
   becomes the regression test for the coding step.

Expected: encode 3 to 5% faster, decode 2 to 4%, output a few hundred
bytes per megabyte larger, one more moving part in every path that
touches the coder.

## 5. Where the time actually is

The same measurements say what would help, in the order it would help:

- **The coder's state lives in memory.**  `rc` is a static member and
  `rc_Process` is inlined into `tc_fam::bit`, but between decisions
  `range`, `low` and `rpre` are stored and reloaded, and the standalone
  chain is 17 cycles against 38 measured in place.  Keeping the coder's
  state in registers across a value's decisions -- a local copy taken in
  `code()` and written back once -- is a change inside `tc_fam.inc` with
  no format change, worth trying before anything structural, and the
  benchmark's 17 cycles is the floor it would approach: up to 5% of the
  encoder.
- **Instructions per decision.**  1,260 is a lot.  The context hashes
  (`tc_make_dig`, 610 instructions per value from thirty inputs, each
  quantised through `tc_qlog`/`tc_sq` first) and the mixers (7 and 3
  inputs, 64-bit multiplies in `mix` and again in `upd`) are half of it.
  Both are the IDX-generated and hand-tuned parts of the model, so any
  cut is a ratio question as much as a speed one; but 32-bit weights, a
  vectorised dot product, and hashing only the inputs that changed since
  the previous digit are speed without a ratio cost, and each is worth
  more than interleaving.
- **Parallelism where the model already restarts.**  `oggdet -c` resets
  the model per stream by design, so its streams are independent work:
  N streams on N cores, N models -- 366 MB of tables each, so two or
  four, not sixteen -- and the segments come out identical to today's.
  Linear on cores for a container of many streams, no format change,
  nothing shared but the output file.  Not `-S`, whose point is the
  shared model; and not a single file, which would have to be cut into
  independently modelled pieces and pay for it in ratio.
- **Memory** is not the limit at these sizes: two L1 misses and no
  last-level misses to speak of per decision.  It may become one on a
  file whose touched rows outgrow the cache, which is a measurement to
  make on something long before believing either way.

## 6. Recommendation

Do not interleave the coder.  The honest ceiling is a few percent on a
step that is 4% of the instructions and 10% of the time, paid for with a
format change and framing.  If speed is wanted, take the coder's state
out of memory first (section 5, no format change, up to the same few
percent), then look at the instruction count of the model, and use cores
where the model already restarts.
