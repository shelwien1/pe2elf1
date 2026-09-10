# oggcomp: where the time goes, and what would take it back

OGGCOMP-PLAN.md sections 11 and 12 took the two cheap steps: the walk
became a coroutine (nothing gained, the I/O was never the cost) and the
range coder became rc.inc's (6 to 11%, the coder's whole share).  What is
left is the model, per bit and per value, and this document is a profile of
it and the list of what could be done about it -- fewer branches, `restrict`,
prefetch, SIMD, and the rest -- each with where it applies, what it is worth
and what it risks.  Anything that changes the stream is marked as such;
most of what follows does not, and "identical output on the corpus" is the
test for all of those.

The measurements are one machine: a 4-core Xeon at 2.8 GHz (Skylake-class,
AVX-512), gcc 13.3, `-O2 -fwrapv` unless stated, the corpus of
`/home/user/testfiles`.  Wall times are minimums of three alternating runs;
the noise floor is about 2%.

---

## 1. The profile

callgrind, with cache and branch simulation, on `oggcomp c` of 00000005.ogg
(117 kB in, 105,828 out).  Decode has the same shape and is not shown.

| | count |
|---|---|
| instructions | 1,410 M |
| coded bits (calls of the coder's step) | 876,725 |
| residue digits (`oc_digit`), each a value through the cascade | 306,544 |
| floor and class values, likewise | 7,504 and 6,504 |
| user time, native | 0.18 s |
| L1 data misses | 2.82 M |
| last-level misses | 41,510 |
| conditional branches | 92 M |
| mispredicted | 4.01 M |

So about 1,600 instructions per coded bit, 3.2 L1 misses per bit, 4.3
mispredicts per hundred branches.  By where the instructions are spent
(self cost, inlined code attributed to the file it was written in):

| component | instructions | L1 misses | mispredicts |
|---|---|---|---|
| cm.inc: counters, APMs, mixer (in `tc_fam::code`) | 34.9% | 63.2% | 29.6% |
| MOD/tsvcomp-dig_p.inc: the digit index builder | 13.7% | 0.1% | -- |
| oc_model.inc: `oc_digit`, the context gathering | 12.1% | 2.6% | 17.1% |
| cm.inc: the sign bit's whole step (the sgn family), inlined in `oc_digit` | 9.7% | 20.1% | 4.5% |
| MOD/tsvcomp-sgn_p.inc: the sign index builder | 7.4% | 0.1% | -- |
| oc_model.inc: `tc_fam::code` itself, the cascade | 6.0% | 0.2% | 7.8% |
| tc_prior.inc: the codebook prior, per bit and per digit | 7.2% | 4.8% | 28.6% |
| rc.inc: the coder | 4.2% | 0 | 3.0% |
| io.inc: the walk (`rs_part`, the codebooks) | 1.7% | 2.8% | 4.6% |

Four things stand out.

**The tables are the misses.**  Four in five L1 misses are in seven lines
of cm.inc.  Five of them first touch a value's rows: the counter's `t ? p
>> 4 : half` (729 K), the APM's count byte `c[i]` (380 K), the APM's two
table entries (473 K), the mixer's weights (142 K).  The other two are the
8 kB curve tables, stretch (332 K) and squash (175 K), which are small
enough to live in L1 and do not, because ten random lines a value keep
pushing them out.  On this
file the working set fits the simulated last level (41 K misses in 2.8 M),
so the misses cost L2 and L3 latency; on 00000008 the tables reach further
and the same misses go to memory, which is why section 5's huge-page result
is a decode gain on the big file and nothing on the small one.

**The index builders are a fifth of the instructions.**  `tc_make_dig` is
631 instructions per digit and `tc_make_sgn` 455 per sign (228,224 of the
digits have one), all straight-line compares and adds -- ninety thresholds
each applied as `(x > k)` and summed -- plus `oc_digit`'s own 560 per digit
gathering the variables: twenty `tc_qlog` calls that loop, four integer
divisions (`%`) for positions that step by one, an eight-term weighted mean
in 64 bits.  Together they cost three quarters of what the counters, APMs
and mixers do.

**A third of the mispredicts are avoidable.**  The lines:

| line | mispredicts | of all |
|---|---|---|
| `if (t < lim) t++;` -- counter saturation, four per bit | 491,754 | 12.3% |
| `if (k < lim) c[i] = k + 1;` -- APM count, four per bit | 352,483 | 8.8% |
| `if (x < 4) return x;` and `while (x >= 4)` -- `tc_qlog` | 568,453 | 14.2% |
| the prior: `tcp_sign`'s loop over the book, `tcp_len`'s over the lengths, `step()`'s tests, `usable()` | 1,147,119 | 28.6% |
| `if (bit(1, ...)) return 1;` etc. -- the cascade on the value | 313,069 | 7.8% |
| the codebook tree walk, `if (nx < 0) return` | 155,396 | 3.9% |
| the mixer's clamp of the dot product | 98,180 | 2.4% |
| the coder's renorm loop, `for (; n; n--)` | 108,413 | 2.7% |

The cascade's branches are the data -- which value it is -- and cannot go.
The saturations, the quantiser, the clamps and the renorm loop are branches
on nothing and can (40% of the mispredicts), and the prior's are branches
on the codebook, which a table settles (another 29%).  At 15 to 20 cycles a
mispredict and 4 M of them in half a billion cycles, mispredicts are an
eighth of the run, and two thirds of that can go -- which is what section
6 then measures for the first group.

**And the instructions are most of the cycles.**  The run's user time is
0.18 s -- half a billion cycles at 2.8 GHz -- for 1.4 G instructions:
nearly three instructions a cycle, on a core that retires four.  So on
this file the model is not waiting; it is executing, and the misses (3 a
bit, to L2 and L3) and the mispredicts are the quarter or so that it is
not.  On the large files the misses reach memory and the balance shifts
some -- section 5.1's huge-page row and section 6's prefetch row are that
shift, 5% and 3% -- but the instruction count is the first thing to cut.

---

## 2. Fewer branches

### 2.1  What is a branch on nothing

Each of these compiles to a compare and a jump today, is taken on the
data's whim, and has a form the compiler turns into a `cmov` or an add:

- **Counter saturation**, `cm_cnt::upd`: `if (t < lim) t++;` becomes
  `t += (t < lim);`.  Four per bit.
- **APM and mixer counts**, `cm_apm::upd1` and `cm_mix::upd`: `if (k <
  lim) c[i] = k + 1;` becomes `c[i] = k + (k < lim);` -- an unconditional
  store of the same byte, which the line already holds dirty.
- **Squash and the mixer's clamp**: `if (x <= -MAX) return sq[0]; if (x >=
  MAX) ...` becomes a clamp into the index.  The two clamps on `pr` after
  it are already ternaries.
- **`tc_qlog`**: `if (x < 4) return x; while (x >= 4) { x >>= 1; n++; }` is
  a loop whose trip count is the magnitude's bit length.  It is
  `bitlength(x) - 2` shifts, so `n = 62 - clzll(x | 1)`, the result
  `2 + 2n + ((x >> n) & 1)`, and `x < 4 ? x : that` selects.  Twenty calls
  per digit, six of them through `tc_sq`.
- **The coder's renorm**: `for (; n; n--)` with n = 0 nine times in ten.
  `if (UNLIKELY(range < sTOP))` around the shift, as the psrc decoder does,
  makes the common case fall through.

Section 6 measures the first four; they are the experiment `e1` and `e2`
there.  None of them changes a value, so the stream is identical.

### 2.2  What is a branch on the data, and what to do instead

- **The cascade** (`if (bit(0, ...)) return 0;` ...) branches on which
  value it is.  A decoder cannot avoid that: the next node is the bit.  An
  encoder could code the three head nodes without branching -- it knows the
  value -- but the branch is one of three per value and predicts as well as
  the data allows.  Leave it.
- **The prior's loops** are branches on the codebook: `tcp_sign` walks
  every symbol of the book for one digit's sign, `tcp_len` sums the
  lengths above k for every length bit, `tcp_man` walks the book for every
  mantissa bit.  These are not branch problems but precomputation
  problems, and section 5.2 has them.
- **The codebook tree walk** in `io.inc` (`bget1` per bit, `if (nx < 0)`)
  is the Vorbis Huffman decode, one bit and one branch per code bit.  A
  lookahead table (8 or 10 bits of the stream indexing a table of (symbol,
  length), the tree only for longer codes) is the standard cure and would
  cut both the branches and the `bk->nd` misses (82 K).  Encode side only
  -- the decoder emits codes from a table already -- and 1.7% of the run,
  so it is last on the list.

### 2.3  `if_e0` and its relatives

`LIKELY`/`UNLIKELY` (common.inc) are `__builtin_expect`; Lib3's `if_e0`
and `if_e1` are the same thing.  They do not remove a branch, they lay the
code out so the expected path falls through and the other is out of line,
which is worth something for the instruction cache and for the front end,
and nothing for a branch that mispredicts.  Places where the hint is right
and absent:

- `rc_Renorm`'s shift (n != 0 one time in ten);
- `tc_bit`'s `tc_verbose && tc_enc`, false in every run that is not `-v`;
- `prp >= 0` in `bit()`, which is per family: always true for digits under
  a book, always false for headers;
- the `FATAL_UNLESS` checks in `code()` and the hot walk, though a call to a
  `noreturn` function is already treated as cold.

Profile-guided optimisation does all of this from a real run and more (it
lays out the whole binary by measured frequency), and section 5.1 measures
it at 3 to 4%.  The hints are for a build without a profile.

---

## 3. `restrict`

### 3.1  Where aliasing costs

The step is a sequence of reads and writes to seven rows through eight
pointers, and after every store the compiler has to ask whether the next
load might read what was just written.  Two things settle that question
today:

- **Type-based alias analysis** separates the `u16` counters, the `i16`
  APM entries and the `i32` weights from each other: a store through one
  type is known not to change a load through another.
- **`u8`** is `unsigned char`, and a store through a `char` type may alias
  anything.  The APM's and the mixer's count arrays are `u8 *`.  So after
  `c[i] = ...` in `cm_apm::upd1` and `c[ctxn] = ...` in `cm_mix::upd`, the
  compiler must assume the counters, the weights, the APM entries, the
  family's rates and bounds, the mixer's own `x[]` and `nx` -- all of them
  -- may have changed, and reload them.  Every one is an L1 hit, so it is
  instructions rather than stalls -- which, on a run that is instruction
  bound, is the right thing to be saving -- and it is the one place
  `restrict` has a real job here.

### 3.2  The types, and where they go

`restrict` is a promise about a pointer for the scope it is declared in,
and gcc honours it on function parameters and on local variables; a
restrict-qualified struct member is legal and is not used.  So the useful
form is a local:

```
typedef cm_cnt * __restrict  cnt_p;     /*  a counter row  */
typedef i16     * __restrict  apm_p;     /*  an APM's entries  */
typedef u8      * __restrict  cnt8_p;    /*  a count array  */
typedef i32     * __restrict  mixw_p;    /*  a mixer's weights  */
```

and in each hot method, the row taken into one of these before the work:

- `cm_apm::upd1`: `cnt8_p cc = c;  apm_p tt = t;` and use those.
- `cm_mix::mix` and `upd`: `mixw_p ww = w + row;` and `cnt8_p cc = c;`.
- `tc_fam::bit`: the four counter references as `cm_cnt & __restrict`.

Section 6's `e3` is exactly that.  The alternative that needs no `restrict`
at all: give the count arrays a type that is not `char`.  A `u16` count
has the alias set of `u16` and nothing else, at the price of doubling two
small arrays (the tables' shape is generated, so it is a generator change,
and the stream does not move).  A `struct cnt8 { u8 v; }` keeps the size,
and whether gcc's access-path analysis lets a `char` member inside a struct
out of the `char` rule is a thing to check on the generated code before
relying on it.  Either way the fix is in the type, once, rather than in a
local in every method.

What `restrict` cannot do is remove the loads that are really needed: a
counter row is read for `P()` and read again for `upd()` because the coder
ran in between and the row could not stay in a register across a call.
Keeping `p` and `t` in locals through the step -- read once, write once --
is a small rewrite of `bit()` that saves eight loads a bit whatever the
aliasing.

---

## 4. Prefetch

### 4.1  What is known when

A value's ten rows -- four counters, two APM rows with their counts, the
mixer's row with its count -- are addressed by `select(v)`, before the
first bit, and nothing about them changes until the value is done.  That
is the prefetch point: ten `__builtin_prefetch` there, and the loads that
follow find the lines on their way.  What the prefetch overlaps is whatever
runs between `select()` and the first touch: `tcp_head`, the prior for
node 0 (a 64-bit division, forty cycles), and the head of `bit()`.  That is
not a memory latency (a hundred nanoseconds, 280 cycles) but it is most of
an L2 or L3 one, which on the small files is what the misses are.  Section
6's `e5` is this, alone; it is worth measuring together with what follows.

### 4.2  Earlier: the encoder knows the future

The rows of digit n+1 depend on digit n (through `q1`, `q2`, `zrun`, the
histories), so the decoder cannot address them before it has decoded
digit n.  The encoder can: it has every digit of the partition in hand.
So the encoder can run the context gathering and the index builder one
digit ahead -- compute `tcx` for digit n+1, prefetch its ten rows, then
code digit n -- and every row is in cache by the time it is wanted.  The
same for the sign: its rows depend on the magnitude, which the encoder
has.  This is software pipelining of the model over the walk, and it
overlaps a whole digit's worth of context work (some 1,500 instructions:
the two builders and the gathering) with the fetch, which covers a memory
miss.  It costs a second copy of the index
state and a restructured `oc_digit`, and applies to the encoder only.

### 4.3  The decoder: guess

The decoder can prefetch for the digit it expects.  Zero is the most
common digit by far in a residue, and the rows for "digit n was 0" can be
addressed before digit n is decoded -- everything else in the index is
known.  A wrong guess costs ten idle prefetches, a right one hides the
miss.  Two guesses (0 and the previous digit, say) cover more.  This is
speculative and the gain depends on how often the guess is right, which is
the zero rate of the residue: high on quiet music, low on noise.

### 4.4  What not to prefetch

The curve tables (stretch, squash, `cm_rec`) and the mixer's `x[]` are
small and hot; their misses are evictions, and the cure for those is fewer
lines per value (section 5.3), not prefetch.  The walk's histories
(`hist`, `avg`, `pp`, `ps`, `pn`) are sequential per slot and the hardware
prefetcher has them.

---

## 5. Other ideas, larger

### 5.1  The compiler and the pages (measured)

Nothing in this section changes the source.

Wall time, minimum of three alternating runs, the same source built
eight ways; every build's output is byte-identical to the base's.

| build | 00000007 `c` | `d` | 00000008 `c` | `d` |
|---|---|---|---|---|
| `-O2` (shipping) | 1.90 s | 1.85 s | 9.36 s | 9.28 s |
| `-O3` | 1.83 (-3.7%) | 1.80 (-2.8%) | 9.64 (+3.1%) | 9.26 (-0.2%) |
| `-O2 -march=native` | 1.97 (+3.7%) | 1.80 (-2.6%) | 9.36 (0.0%) | 9.32 (+0.5%) |
| `-O3 -march=native` | 1.85 (-2.7%) | 1.84 (-0.5%) | 9.48 (+1.3%) | 9.32 (+0.5%) |
| clang 18 `-O2` | 1.89 (-0.8%) | 1.84 (-0.2%) | 9.93 (+6.1%) | 9.61 (+3.6%) |
| clang 18 `-O3 -march=native` | 1.84 (-3.4%) | 1.84 (-0.4%) | 9.31 (-0.5%) | 9.15 (-1.3%) |
| `-O2` + PGO (profiled on 00000003) | 1.87 (-1.6%) | 1.80 (-2.4%) | 9.07 (-3.0%) | 9.15 (-1.3%) |
| `-O2`, tables on transparent huge pages | 4.05 (+113%) | 1.84 (-0.2%) | 12.07 (+29%) | 8.84 (-4.7%) |

So: `-O3` gains 3% on one file and loses 3% on the other, `-march=native`
and clang are noise, and PGO is the one that is consistently a little
better -- 1.5 to 3%, in both directions on both files, which is what a
layout by measured frequency buys on a program whose branches are mostly
data.  None of it is the 7 to 11% the source changes below get.

**Huge pages** are the interesting row.  The tables are 1020 MB of
`mmap(MAP_NORESERVE)`, touched sparsely, and a 4 kB page walk is a TLB miss
on nearly every row a big file reaches.  `madvise(MADV_HUGEPAGE)` on the
mappings (the kernel here has transparent huge pages in `madvise` mode)
takes 4 to 6% off the user time of every run on 00000008 -- but the first
process to ask pays the kernel for compacting memory into 2 MB pages:
2.2 s of system time on the small file, 3 s on the large one, and the
decode that runs right after shows the gain without the cost, presumably
because the pages the encoder freed are still whole.  On a machine with the pages preallocated (`vm.nr_hugepages`, or
`hugetlbfs`) the gain would come without the cost; as transparent huge
pages on this kernel it is a loss on wall time and stays off.  The 5% is
also the measure of how much of the run is address translation.  A
software prefetch walks the page tables as a load does, so it hides the
walk only if it is issued early enough for both, which is the case for
section 4.2's digit-ahead form and not for section 4.1's; and fewer lines
per value (5.3) is fewer walks as well as fewer misses.

### 5.2  The prior, tabulated

`tcp_p` is `(w0 * 4096 + s / 2) / s` -- a 64-bit division -- once per bit
that has a prior, and `tcp_head`, `tcp_len` and `tcp_sign` each walk part
of the book to find `w0` and `w1` first.  Every one of these depends only
on the codebook node and the step, which are fixed for the link: `tcp_head`
is three numbers per node, `tcp_len` is `TCP_NBMAX` per node, and both can
be computed once in `tcp_build` with the identical integer formula and read
back as a load.  `tcp_sign` is one number per (node, magnitude) and the
magnitudes a node's entries take are few, so a small per-node table
indexed by magnitude, with a fallback to the loop past its end, replaces
the walk.  `tcp_man`, the mantissa's, is the one that has to be walked --
the prefix is not a prefix of the entry -- and it runs only for values of
3 and up.  Stream-identical if the stored quotient is the same quotient;
about 7% of instructions and over a quarter (28.6%) of the mispredicts.

### 5.3  Fewer lines per value

Ten lines a value is the cost of keeping each component's count in an
array of its own.  Interleaving the count with the row it counts -- `struct
{ i16 t[q]; u8 c[q]; }` per APM context, `struct { i32 w[N]; u8 c; }` per
mixer context -- makes the count the same line as the row and takes three
lines off every value.  The values stored are the same values, so the
stream does not change, but the tables' shape is generated (MOD/, from
idx2inc.pl) and the generator has to learn the shape.  The counter rows
are 29 nodes of 4 bytes, 116 bytes, so a row is not line-aligned and the
three head nodes' 12 bytes straddle a line boundary in one row of six;
padding a row to two lines would end that, at 10% more table.

### 5.4  The index builders

A fifth of the instructions, and a shape the generator chooses.  Three
directions:

- **Table lookups for the threshold chains.**  `(x > 1) + (x > 4) + (x > 5)
  + (x > 6)` is a dozen instructions, a compare and a set per threshold and
  the adds; `T[x]` with `x` clamped is three, and the
  generator already emits that form for some variables (the `TC_dig_b_band`
  style).  Emitting it for every chain of three or more compares is a
  generator change with no format change, and the tables are a few hundred
  bytes.
- **Sums instead of products.**  An index is a mixed-radix number, `((v1 *
  8 + v2) * 16 + v3) * 5 ...`, which is `v1 * s1 + v2 * s2 + ...` with the
  strides known at generation.  A table per (index, variable) holding
  `bucket(x) * stride` makes the whole builder a sum of ninety loads, no
  multiplies and no compares.  Larger tables (a few kB), and they compete
  for L1 with everything else, so it wants measuring against the first
  form.
- **Hoisting what is constant for the partition.**  `pass`, `band`, `cls`,
  `blk`, `bkq`, `rno` do not change between one digit and the next of a
  partition; a builder split into a partition part (computed once in
  `part_begin`) and a digit part would do the ninety compares once per
  partition instead of once per digit for a third of them.

### 5.5  `oc_digit`'s own arithmetic

Beside `tc_qlog`: the four `%` by `dim` and `ilv` are positions that
advance by one and wrap, and two counters in `P` replace them (section 6,
`e4`).  The weighted mean `ex` is eight 64-bit multiplies for one
quantised number; with the weights bounded as the .idx bounds them it fits
32 bits, but the comment there says why it is 64 and the reason (the
optimizer's corners) stands, so that one stays.

### 5.6  SIMD

The one loop worth a vector is the mixer: seven products `(w + W0) * x` in
64 bits, summed, then seven updates `w += (x * err) >> 16`.  Both are a
fixed seven wide (`nx` is always `N` when `mix()` runs -- every input is
added every time -- so the loop bound can be the constant, which lets the
compiler unroll it and is worth doing first).  With AVX2, `vpmuldq` makes
four 64-bit products of the even 32-bit lanes; two of them and a shuffle
cover eight lanes, then a horizontal add -- about ten instructions for the
dot product against thirty scalar, and the same for the update.  The
inputs are already in an `int x[N]`; the weights are a contiguous row.
This is a format-neutral rewrite of two loops, maybe 5% of the run.

What SIMD cannot do here is code two bits at once: each bit's contexts
depend on the last bit.  The psrc coder got its width by running sixteen
independent streams in lanes; oggcomp has one stream and one history.

The index builders' compares are the other candidate: thirty variables
against ninety thresholds is one AVX-512 compare per sixteen thresholds
with a popcount of the mask, if the variables are laid out for it.  The
table forms of section 5.4 are simpler and probably as fast.

A last, different SIMD: the mixer's 64-bit products exist because the
weights are 32-bit.  A mixer with 16-bit weights and `vpmaddwd` is what
the paq family does, at a quarter of the instructions -- but that changes
the arithmetic, hence the stream, hence the tuned parameters.  It is a
model change to be evaluated as one, not an optimisation.

### 5.7  What is not worth doing

- **The direction as a compile-time constant** (one binary per direction):
  measured in section 11 of the plan, no gain.
- **The coroutine**: nothing to gain, as measured; nothing to lose either.
- **Threads**: the model is one dependency chain through one history.  The
  walk could run ahead of the model in the encoder, but it is 2% of the
  run.
- **`-march=native`, clang**: measured below, within noise.

---

## 6. Five source experiments, measured

Five changes from sections 2 to 5, applied one on top of the other in a
scratch copy of the tree, each built `-O2`, checked for byte-identical
output on two corpus files and a round trip, and timed against the
unchanged source.  Minimum of three runs on 00000007 and of two on
00000008.

| cumulative change | 00000007 `c` | `d` | 00000008 `c` | `d` |
|---|---|---|---|---|
| base, the tree as committed | 1.83 s | 1.78 s | 9.37 s | 9.18 s |
| e1: branchless saturations and clamps (2.1) | 1.75 (-4.7%) | 1.70 (-4.4%) | | |
| e2: + `tc_qlog` by bit length (2.1) | 1.70 (-7.4%) | 1.68 (-5.3%) | | |
| e3: + `restrict` locals on the count arrays and rows (3.2) | 1.70 (-7.5%) | 1.64 (-7.6%) | | |
| e4: + the four `%` per digit as two counters (5.5) | 1.67 (-8.7%) | 1.64 (-7.6%) | 8.61 (-8.2%) | 8.53 (-7.1%) |
| e5: + ten prefetches at `select()` (4.1) | 1.67 (-8.9%) | 1.66 (-6.5%) | 8.34 (-11.1%) | 8.40 (-8.5%) |

Reading down the column: the two branch changes are 5 to 7%, which is
what section 1 prices the mispredicts they remove at; `restrict` is
within noise on the small file's encode and 2% on its decode; the counters
for the divisions are about 1%; and the prefetch is nothing on the small
file, whose rows are in L2 anyway, and 2 to 3% on the large one, where
they are not.
Eleven percent on 00000008's encode, for some forty lines, all of it
stream-identical.  What is not in the table is the encoder one digit ahead
(4.2), which is where the prefetch would stop being 3%.

---

## 7. The order to do them in

By expected gain per unit of work, all stream-identical unless marked:

1. Section 6's five changes, into the tree: 7 to 11% measured, forty
   lines, stream-identical.  The scratch copies are the patch.
2. PGO in the build (5.1): 1.5 to 3% on top, for a profile run in `mk.sh`.
3. The prior tabulated (5.2): a build-time table per node, 7% of the
   instructions and a quarter of the mispredicts.
4. The count arrays typed (3.2) and the rows read once (3.2): small, in
   `cm.inc`.
5. The encoder one digit ahead (4.2): the prefetch that covers a memory
   miss rather than an L2 one; encoder-only.
6. The index builders as tables (5.4): a generator change, a fifth of the
   instructions.
7. The mixer in AVX2 (5.6): two loops.
8. Rows with their counts (5.3): generator and tables, three lines a value
   and fewer TLB walks.
9. Huge pages where the kernel gives them without the fault cost (5.1):
   5% on the large files, on a box set up for it.

The stream-changing ones -- a 16-bit mixer, a smaller stretch table -- are
model work, tuned and measured for size first, and are not on this list.
