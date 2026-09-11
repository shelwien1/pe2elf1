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

In a scratch copy of `tc_fam.inc`: a row is the index hashed to the top
`HBITS` of a Fibonacci multiply, the index itself kept beside the table
as the row's tag, and a row whose tag is another context's is zeroed --
which is every table's initial state, since `tc_map` and BSS hand out
zeros and `_Init()` writes nothing -- and taken over.

    static u32 tc_tags[6][8][HROWS];
    static INLINE u32 tc_hrow(u32 idx, int bits, u32 *tags, void *t1, sz r1, void *t2, sz r2) {
      u32 row = (idx * 0x9E3779B1u) >> (32 - bits);
      if(tags[row] != idx + 1) {
        tags[row] = idx + 1;
        memset((u8 *)t1 + (sz)row * r1, 0, r1);
        if(t2)
          memset((u8 *)t2 + (sz)row * r2, 0, r2);
      }
      return row;
    }
    //  in select(), for each index whose Volume exceeds HROWS:
    ba = (sz)(hA ? tc_hrow((u32)x.a, hA, tgA, A, (sz)nd * sizeof(cm_cnt), nullptr, 0) : (u32)x.a) * nd;
    bs = (u32)(hS ? tc_hrow((u32)x.s, hS, tgS, ap.t, (sz)nd * ap.q * sizeof(i16), ap.c, (sz)nd * ap.q) : (u32)x.s) * nd;
    //  ... and B, D, F, M, T, N the same way; C and G are small and stay direct.
    //  in wire(): hA = va > HROWS ? HBITS : 0; and so on.

The tables themselves were left as declared, so the prototype measures
what hashing costs the model and the clock, not what it saves in address
space: the rows it uses are the first `HROWS` of each table.  The tag is
the whole index, so a collision is always seen; a real tag can be 16
bits of it (section 4).  Encoder and decoder run the same code, so the
stream stays decodable, and every input round-trips.

## 3. What it costs

Bytes over the direct-table build, same profile, 34 inputs (the corpus,
1 MB of random bytes, the binary, the 512 KB cut-off stream, and
`big.ogg`):

| rows per hashed table | output, all 34 (3176474 bytes) | `music-stereo-q5` (144365) | `chirp-stereo-q10` (116249) | `big.ogg` (960192) | rows taken over on `big.ogg` |
|---|---|---|---|---|---|
| 2^14 | +1451 (+0.046%) | +159 | +284 | +652 | 402555 of 33.2M selects |
| 2^16 | +461 (+0.015%) | +39 | +82 | +224 | 185118 of 27.5M |
| 2^18 | +128 (+0.004%) | +10 | +31 | +61 | 117837 of 25.8M |

Files that use few rows -- everything under 10 KB, the non-Ogg inputs --
do not move at all.  `chirp-stereo-q10.ogg`, `-q 10` with the widest
books, is the most sensitive.

Time and memory, encode, release build:

| | direct | 2^16 rows | 2^18 rows |
|---|---|---|---|
| `music-stereo-q5.ogg` | 0.34 s | 0.36 s | 0.39 s |
| `chirp-stereo-q10.ogg` | 0.32 s | 0.31 s | 0.36 s |
| `big.ogg` | 1.66 s | 1.69 s | 1.86 s |
| peak RSS, `big.ogg` | 182 MB | 203 MB | 272 MB |

At 2^16 the hashing is free: eight multiplies and eight tag loads per
select against a select that already prefetches ten lines.  At 2^18 the
hashed region of the wide-row tables outgrows what the direct layout
happened to touch and the misses come back.  Resident memory goes up,
not down, because hashing spreads rows over the whole hashed region
where the direct layout clustered them; what goes down is the
reservation, which the prototype does not change and section 4 does.

## 4. The design

**Rows per table, not one cap.**  The row is 4 bytes in `sgn` and 4611
in `dig.f`; a uniform 2^16 rows puts `dig.f` at 288 MB and `sgn.a` at
256 KB.  Size each hashed table to its own budget: the rows `big.ogg`
touched, times four for headroom, rounded up to a power of two --

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

**The tag.**  Sixteen bits of the index that the hash did not use --
`idx >> bits`, folded once if the index is wider than `bits` + 16 -- in
a `u16` array beside each table, 128 KB for 2^16 rows; the prototype's
full-index `u32` costs twice that and detects nothing a 16-bit tag
misses in practice (a false match is a 1-in-65536 chance per takeover).
Zero must mean "never used", so store `tag | 1` or use `idx + 1`.

**Takeover.**  Direct-mapped, always replace, as the prototype does: at
2^16 rows and 25M selects one select in 150 took a row over on
`big.ogg`, and most of those were first touches.  A two-way bucket that
keeps the row with the higher `cm_cnt::t` (the observation count the
counters already carry) would halve the loss again for one more compare;
worth measuring once the first version is in, not before.

**Where.**  `tc_fam<f_DEC>::wire` gets the row count of each table from
its caller and derives `hbits`; `select()` is the prototype's; the tag
arrays are members of `oc_tables` beside the table they belong to, sized
statically, since every count is a compile-time constant.  The hash and
the tag are direction-independent, so the tuning and shipping builds
still agree and `./mk.sh check` still proves it.

**Declaring the tables.**  A table's array is declared by `Table( type,
%M%A, %M%a_Volume * %M%ND )` in `IDX/tsvcomp-*.inc`, and in the shipping
build that expression is the member's size.  For the hashed tables the
size becomes `TC_ROWS_a * %M%ND`, a constant per table in
`tc_tables.inc` -- one edit per hashed table in the six templates, which
is the one thing here that touches `IDX/`.  Without it the prototype's
approach works, and costs nothing on Linux, where `tc_map` reserves and
does not commit; on Windows `VirtualAlloc` with `MEM_COMMIT` charges the
5 GB against the pagefile at start, which is the case that needs the
declaration to shrink.  `tc_home` then puts every family in BSS and
`tc_map` has nothing left to do.

**The tuning framework.**  Once a wide context costs nothing, `opt.pl`
will widen them; that is the point.  Two consequences: the `TC_MEMCOST`
rent (`oggcomp.cpp`, `run<f_DEC>`) charges by `tabs.bytes()`, which is
then a few hundred megabytes whatever the patterns say, so the rent
should go, or charge the row counts; and the optimizer's objective gains
a little noise from takeovers, which the budgets above keep under one
part in five thousand.

**The stream.**  Hashed rows collide where direct rows did not, so the
model is not the current one and a `.oc` written before does not decode
after -- which is also true of the profile that just landed, and of every
profile before it.  The CRC at the end of the stream turns that into
exit 1 rather than wrong output.  A better guard, independent of this
work, is four bytes of a hash over the model's parameters in the `.oc`
header, so `d` can say which build wrote the file.

## 5. What to do

1. `tc_fam.inc`: the prototype's `select()` and `wire()`, with 16-bit
   tags and per-table row counts.  `tc_tables.inc`: the `TC_ROWS_*`
   constants and the tag arrays in `oc_tables`.  Verify as the
   refactoring was: `./t.sh`, `./mk.sh check`, the 34-input size table
   against the direct build, encode time within noise.
2. The six `IDX/tsvcomp-*.inc` templates: the twelve `Table()` sizes,
   regenerate `MOD/`, confirm `tabs.bytes()` says about 320 MB and
   `VmPeak` about 900 MB.
3. Measure `08.ogg`: rows touched per table, and the size cost at the
   budgets above; adjust the budgets to it, since it is what the profile
   was tuned on and the largest stream to hand.
4. Then, separately: the two-way bucket, and `q` re-tuned with the row's
   price in the objective.
