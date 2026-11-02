# Hashed rows for the oversized tables

The profile committed in `101ff53` widens the contexts until the model
tables take 5148 MB: the digit family alone is 4795 MB, its `A` counter
table 1927 MB and its second APM `F` 1368 MB.  On a megabyte of input
the coder touches 0.04% of the rows of `A` and 2% of the rows of `F`.
The rest is address space -- 5.6 GB of `VmPeak`, committed on Windows --
and, on Linux, page-table and TLB work: encode time went up by a third
with the profile.  Hashing the rows of every table over a size cap keeps
the model within 0.02% of its output and the tables at a few hundred
megabytes.  This is the measurement, the prototype it was made with, and
the design for doing it properly.

**Done.**  Section 4 as written, with the counts of its table, frozen in
the `.idx` sources; `tc_rows()` and `tc_hbits()` in `tc_tables.inc`,
`tc_hrow()` and the hashed `select()` in `tc_fam.inc`, the counts
through `TC_WIRE`, the twelve `Table()` sizes in the templates,
`idx2inc.pl` untouched.  Measured against the direct tables of the same
profile:

| | direct | hashed |
|---|---|---|
| model tables | 5148 MB | 366 MB |
| address space reserved (`VmPeak`) | 5635 MB | 858 MB |
| BSS of the shipping binary | 833 MB | 546 MB |
| peak resident, `music-stereo-q5.ogg` | 70 MB | 84 MB |
| peak resident, `big.ogg` | 182 MB | 157 MB |
| output, 34 inputs (3176474 bytes) | | +312 (+0.010%) |
| `music-stereo-q5.ogg` (144365) | | +14 |
| `chirp-stereo-q10.ogg` (116249) | | +36 |
| `big.ogg` (960192) | | +208 |

Everything under 10 KB and every non-Ogg input codes to the same size.
Encode and decode time are within the noise of the direct build.
`./t.sh` and `./mk.sh check` pass; clang++ and both Windows backends
write the same bytes.  The digit family is still 300 MB, over
`tc_home`'s 256 MB, so it alone is mapped; the rest is BSS.  What is
left is section 5's step 4: the counts against `08.ogg`, and `q` with
the row's price in it.

## 1. What a table is, and how much of it is used

Every family's table is rows of one context each: the counter tables
`A`, `B`, `C`, `D` hold `TC_NODE` = 29 `cm_cnt` per context (116 bytes),
the APM tables `S` and `F` hold 29 × `q` buckets of an `i16` and a `u8`
(1653 bytes at `q` = 19, 4611 at `q` = 53), the mixer `W` 29 × 7 weights
and a count (841 bytes), the mantissa plane `MT` 169 counters (676
bytes).  A context index is the mixed-radix product the IDX patterns
define, `tc_make_*` in `MOD/`, and `tc_fam::select` (`tc_fam.inc`) turns
it into a row base: `ba = x.a * nd`.  The table's size is the index's
`Volume` times the row.

Distinct rows selected per table, counted with a probe in `select()`:

| table | volume | row bytes | direct size | rows touched, `music-stereo-q5` | `big.ogg` (1.1 MB, seven files) |
|---|---|---|---|---|---|
| `dig.a` | 17418240 | 116 | 1927 MB | 7774 (0.04%) | 26354 (0.15%) |
| `dig.b` | 3981312 | 116 | 440 MB | 3302 | 14526 |
| `dig.d` | 2257920 | 116 | 250 MB | 6455 | 17980 |
| `dig.s` | 331776 | 1653 | 523 MB | 2436 | 5393 |
| `dig.f` | 311040 | 4611 | 1368 MB | 1394 | 6163 |
| `dig.t` | 356400 | 676 | 230 MB | 1002 | 3378 |
| `sgn.a` | 6220800 | 4 | 24 MB | 1838 | 7791 |
| `sgn.b` | 14256000 | 4 | 54 MB | 4621 | 15292 |
| `sgn.f` | 232848 | 195 | 43 MB | 766 | 3243 |
| `flr.b` | 290304 | 116 | 32 MB | 1680 | 3838 |
| `flr.f` | 87480 | 1740 | 145 MB | 849 | 2061 |
| `flr.s` | 18720 | 2001 | 36 MB | 656 | 1481 |

Everything else is under 13 MB and mostly used.  The touched set grows
sub-linearly with input: seven files, 1.1 MB, touch three to four times
what one 168 KB file does.  A real album-length stream would touch more,
which is what the headroom in section 4 is for, and what `08.ogg` should
be measured with before the budgets are fixed.

## 2. The prototype

In a scratch copy of `tc_fam.inc`: for every index whose Volume exceeds
the row count, the row is the index hashed to the top `HBITS` of a
Fibonacci multiply, and that is all -- no tag, no check, no takeover.
Contexts that land on the same row share it, and the counters adapt to
whichever is coding.

    static INLINE u32 tc_hrow(u32 idx, int bits) { return (idx * 0x9E3779B1u) >> (32 - bits); }
    //  in select(), for each index whose Volume exceeds the row count:
    ba = (sz)(hA ? tc_hrow((u32)x.a, hA) : (u32)x.a) * nd;
    bs = (u32)(hS ? tc_hrow((u32)x.s, hS) : (u32)x.s) * nd;
    //  ... and B, D, F, M, T, N the same way; C and G are small and stay direct.
    //  in wire(): hA = va > HROWS ? HBITS : 0; and so on.

A first version tagged each row with its index and zeroed a row on a
mismatch, so that no context ever saw another's statistics.  It was
measured too, and sharing beats it: the tag costs a load and a compare
per table per select, the takeover throws away statistics that were
usually still useful, and the numbers below say the counters recover
from a shared row faster than from an empty one.

The tables were left as declared, so the prototype measures what
hashing costs the model and the clock, not what it saves in address
space: the rows it uses are the first `HROWS` of each table.  Encoder
and decoder run the same code, so the stream stays decodable, and every
input round-trips.

## 3. What it costs

Bytes over the direct-table build, same profile, 34 inputs (the corpus,
1 MB of random bytes, the binary, the 512 KB cut-off stream, and
`big.ogg`, seven corpus files chained):

| rows per hashed table | shared, all 34 (3176474 bytes) | `music-stereo-q5` (144365) | `chirp-stereo-q10` (116249) | `big.ogg` (960192) | tagged and reset, all 34 |
|---|---|---|---|---|---|
| 2^14 | +1124 (+0.035%) | +82 | +93 | +717 | +1451 |
| 2^16 | +398 (+0.013%) | +18 | +36 | +253 | +461 |
| 2^18 | +129 (+0.004%) | +9 | +16 | +79 | +128 |

Files that use few rows -- everything under 10 KB, the non-Ogg inputs --
do not move at all.  `chirp-stereo-q10.ogg`, `-q 10` with the widest
books, is the most sensitive.  Sharing wins on every single file; the
tagged version is a little better only on `big.ogg`, where seven
streams' contexts turn over in the same rows and a reset now and then
helps -- 29 bytes in a megabyte.

Time, encode, release build, `music-stereo-q5.ogg`: 0.34 s direct,
0.35 s shared at 2^16, 0.39 s at 2^18; `big.ogg` 1.66, 1.70 and 1.85 s.
At 2^16 the hashing is free -- a multiply per table per select against a
select that already prefetches ten lines.  At 2^18 the hashed region of
the wide-row tables outgrows what the direct layout happened to touch
and the misses come back.  Resident memory goes up, not down (182 MB to
203 MB on `big.ogg` at 2^16), because hashing spreads rows over the
whole hashed region where the direct layout clustered them; what goes
down is the reservation, which the prototype does not change and
section 4 does.

## 4. The design

**Rows per table, declared in the `.idx`.**  The row is 4 bytes in `sgn`
and 4611 in `dig.f`; a uniform 2^16 rows puts `dig.f` at 288 MB and
`sgn.a` at 256 KB, so each hashed table gets its own row count, and the
count is a parameter of the family like its rates are: a `Number` per
table in `IDX/tsvcomp-*.idx`, the log2 of the rows --

    Number hA, 1, 0!10001          # dig: 2^17 rows of A
    Number hS, 1, 0!01111          # 2^15 rows of S

-- which the generator already turns into `TC_dig_hA`: a folded constant
in the shipping build and a live `mapping` value in the tuning build,
which is exactly the two forms the table sizes need.  A table's array is
then declared by

    Table( cm_cnt, %M%A, tc_rows(%M%a_Volume, %M%hA) * %M%ND );

in `IDX/tsvcomp-*.inc`, with `constexpr sz tc_rows(long long vol, int h)
{ return vol <= (1LL << h) ? vol : (sz)1 << h; }` in `tc_tables.inc`: a
table whose Volume fits is direct, as now, and one that does not is
hashed into 2^h rows.  In the shipping build the expression is a
constant and the member array is that size; in the tuning build
`_Init()` maps that many bytes.  Nothing in `idx2inc.pl` changes.  Whether
`opt.pl` may move the counts is a `Debug` decision per line: left live,
it will grow them, since a bigger table is never worse by the objective
alone, so either freeze them (`! Number ...`) or price them (below).

Starting counts, from the rows `big.ogg` touched, times four for
headroom, rounded up to a power of two --

| table | rows | size | table | rows | size |
|---|---|---|---|---|---|
| `dig.a` | 2^17 | 14.5 MB | `dig.s` | 2^15 | 51.7 MB |
| `dig.b` | 2^16 | 7.2 MB | `dig.f` | 2^15 | 144.1 MB |
| `dig.d` | 2^17 | 14.5 MB | `dig.t` | 2^14 | 10.6 MB |
| `sgn.a` | 2^15 | 0.1 MB | `sgn.f` | 2^14 | 3.0 MB |
| `sgn.b` | 2^16 | 0.2 MB | `flr.f` | 2^14 | 27.2 MB |
| `flr.b` | 2^14 | 1.8 MB | `flr.s` | 2^13 | 15.6 MB |

-- 303 MB for what is 5133 MB direct, with the unhashed remainder
another 15 MB.  The APM rows are the cost: `dig.f` at `q` = 53 is 4611
bytes a row, and the optimizer set `q` (`TC_dig_qf`) with the row's
price at zero.  With hashed tables the row width is what the budget
should be charged for, and 2^15 rows of `dig.f` is a number to tune, not
to take from this table.

**Where.**  `tc_fam<f_DEC>::wire` takes each table's `h` with its
Volume and keeps `hA = va > (1u << h) ? h : 0`; `select()` is the
prototype's.  With `h` a constant in the shipping build the test folds
and a direct table costs what it does today.  The hash is
direction-independent, so the tuning and shipping builds still agree and
`./mk.sh check` still proves it.  `tc_home` then puts every family in
BSS and `tc_map` has nothing left to do; on Windows, where `VirtualAlloc`
with `MEM_COMMIT` charges the whole reservation at start, the 5 GB
commit goes with it.

**The tuning framework.**  Once a wide context costs nothing, `opt.pl`
will widen them; that is the point.  Two consequences: the `TC_MEMCOST`
rent (`oggcomp.cpp`, `run<f_DEC>`) charges by `tabs.bytes()`, which is
then whatever the row counts say whatever the patterns say, so it either
goes or becomes the price of the row counts if those are left live; and
the optimizer's objective gains a little noise from shared rows, under
one part in five thousand at the counts above.

**The stream.**  Hashed rows are shared where direct rows were not, so
the model is not the current one and a `.oc` written before does not
decode after -- which is also true of the profile that just landed, and
of every profile before it.  The CRC at the end of the stream turns that
into exit 1 rather than wrong output.  A better guard, independent of
this work, is four bytes of a hash over the model's parameters in the
`.oc` header, so `d` can say which build wrote the file.

## 5. What to do

1. `tc_tables.inc`: `tc_rows()`.  `tc_fam.inc`: `wire()` takes the
   counts, `select()` hashes.  `oc_model::init()`: pass `TC_x_h*` through
   `TC_WIRE`.
2. The six `.idx` files: a `Number h*` per hashed table at the counts
   above, frozen; the six `.inc` templates: the twelve `Table()` sizes.
   Regenerate `MOD/`, confirm `tabs.bytes()` says about 320 MB and
   `VmPeak` about 900 MB.
3. Verify as the refactoring was: `./t.sh`, `./mk.sh check`, the 34-input
   size table against the direct build (expect about +400 bytes on
   3.2 MB), encode time within noise.
4. Measure `08.ogg`: rows touched per table, and the size cost at the
   counts above; adjust them to it, since it is what the profile was
   tuned on and the largest stream to hand.  Then `q` re-tuned with the
   row's price in the objective.
