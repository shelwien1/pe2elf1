# Redesign plan: `SSE_Ctr` and `Mix2` as plain components

`Counter<CP>` is the model of what a component should be: one cell of state,
`PredictF()` returns everything the update needs in a `Pred` struct, the caller
hands it back to `C_Update()`, and the cell knows nothing about the table it
lives in -- `o1[256][256]` is just an array of them.  `SSE_Ctr` and `Mix2` are
not built that way.  This document is the plan to make them so.

## 1. What is wrong now

### 1.1 They are containers, not components

| | `SSE_Ctr<CP,NB>` (sh_SSE.inc) | `Mix2<CP>` (sh_mix2.inc) |
|---|---|---|
| owns | `Cell* t` into `S0_tbl`, `ncx` rows, `vol` | `Cell* t` into `M0_tbl`, `n`, `vol` |
| indexes | `Row(cx)`: identity if `vol <= ncx`, else a multiplicative hash | identity if `vol <= n`, else a multiplicative hash |
| allocation | none itself, but the tuning build wraps it in `SSE_Dyn<CP>`: a virtual interface and `new Impl<NB>` chosen at `Init()` | none |
| init | `Init(table, volume)` walks every row | `Init(table, contexts)` walks every cell |

The element the table actually stores is `Counter<CP_S0>` (one bucket) and
`Mix2::Cell` (one mixer context), and the thing that knows how to *use* an
element is a separate object with pointers into the table.  So `coder0.cpp`
needs `sse_table_cells()`, `mix_table_ctx()`, the `TBL_CONSTEXPR` dance, a
`#if USE_NEW` choice between two front ends, and two `Init(table, ...)` calls
to wire it all together.

The hash is load-bearing only on the SSE side.  Today:

```
S0  contexts = Cx_Volume * Cx3_Volume = 522240 * 256 = 133,693,440
    rows     = min(2^HBITS=2^22, 2^SSE_MAXCELLS_LOG / NB) = 4,194,304
    cells    = rows * NB(4) = 16,777,216  *  96 B  =  1.61 GB
    -> 32 contexts share every row on average
M0  contexts = 522240 < 2^MIX_MAXCTX_LOG, so vol <= n: the hash never runs
    cells    = 522240 * 28 B = 14.6 MB
```

### 1.2 Constants live in the objects

`SSE_Ctr` carries `lim`, `zmax`, `qscale`, `upmin`, `upd`; `Mix2` carries `zm`
and computes the initial `W0` logit inside `Init()`.  All of them are pure
functions of the `CP_S0` / `CP_M0` knobs, fixed at load time, and belong next to
`MWX0`, `KYLO`, `iStepU`, ... in the bundles that `config.hpp` /
`config_mix2.hpp` build -- where the tuning build already does its dynamic init
in dependency order and where the F5 build-identity rule (init-time
transcendentals through `rt_logf`/`rt_expf`) is already applied.

### 1.3 Per-query state lives in the objects

`SSE_Ctr::{c, wt, pr0, pr1, dzdz, sh0, sh1}` and `Mix2::{c, s1, s2, wv, p0,
d1, d2}` are written by `Predict()`/`Mix()` and read by `Update()`.  `Counter`
solved exactly this with `Pred`: the caller keeps it on the stack between the
two calls, and the cell holds nothing that is not model state.  The A2 chain
accessors `dz2_dz1()`, `dzf_dz1()`, `dzf_dz2()` exist only to read that hidden
state back out.

### 1.4 The precedent

The original coder already had the split this plan asks for
(`freq_SSE1.inc` / `freq_SSE1_dbg.inc`).  `SSEii<SSEQuant, SSEInter, Width>`
is one row with a compile-time bucket count: `SSE_Pred()` fills an
`SSEii_updstr` that the caller keeps and hands back to `SSE_Update()`, and the
row holds nothing but model state.  Its resizable debug version,
`SSEj<int& Q, int& N>`, is the *table*: templated on references to the two
runtime knobs (the trick IDX-FORMAT.md sec.8 later named `IDXP`), its `Init()`
runs `switch(Q) { case 2: p = new SSEii<2>[N]; ... }` and every call switches
on Q again to reach `((SSEii<Q>*)p)[i]`.  `SSEii_updstr` is this plan's
`Pred`; `SSEj` is its `SSE_Tbl` (sec.2.4).

## 2. Target shape

### 2.1 `Mix2<CP>` -- one mixer context

```cpp
template<class CP> struct Mix2 {
  ParamUpdater<typename CP::Config_W> w;   // logit of the weight of z1
  ParamUpdater<typename CP::Config_B> b;   // stretch-domain bias
  uint age;                                // A1 young-cell schedule

  struct Pred {
    float p;         // mixed P(bit==0)
    float s1, s2;    // the two inputs after the +-ZM clip
    float wv;        // sq(w.val)
    float d1, d2;    // dz_f/dz1, dz_f/dz2: wv and 1-wv, or 0 where the input clip binds
  };

  void Init() { w.Init(CP::WX0); b.Init(0.0f); age = 0; }
  Pred Mix( float z1, float z2 ) const;    // the current Mix() body, writing Pred
  void Update( int bit, const Pred& pr );  // the current Update() body, reading Pred
};
```

The nested `Cell` disappears: `Mix2` *is* the cell, exactly as `Counter` is.

### 2.2 `SSE<CP,NB>` -- one row: NB buckets over the quantized input logit

"Mapping one prediction to another" needs the two bracketing buckets, so the
row, not the bucket, is the unit.  `Counter<CP> c[NB]` needs a constant, so NB
is the row's template parameter -- exactly as `SSEii<SSEQuant>` was -- and the
`Pred` struct, which does not depend on NB, is defined once beside it so that
every instantiation and the tuning build's dispatcher return the same type.

```cpp
template<class CP> struct SSE_Pred {
  typename Counter<CP>::Pred pr0, pr1;   // the two bracketing cells' predictions
  int   j;                               // left bucket
  float wt;                              // interpolation weight of c[j+1]
  float z;                               // the output logit z0 + wt*(z1-z0)
  float dzdz;                            // dz/dz_in, 0 where the input clip binds
  float sh0, sh1;                        // dz/dz_cell: 1-wt, wt, or 0 where a cell's clip binds
};

template<class CP, int NB> struct SSE {
  static_assert( NB>=2 && NB<=SSE_NB_MAX, "SSE bucket count out of range" );
  Counter<CP> c[NB];

  void Init() { for( int j=0; j<NB; j++ ) c[j].InitN( CP::SEEDS.a[j], CP::SEEDS.b[j] ); }
  SSE_Pred<CP> Predict( float z_in ) const;      // the current Predict() body minus Row()
  void Update( int bit, const SSE_Pred<CP>& pr, float ef );
};
```

`sizeof(SSE<CP_S0,4>)` = 4 x 96 = 384 B with no padding; a row-major
`Counter[rows*NB]` and `SSE[rows]` have the same bytes in the same order, which
is what makes phase 1 below verifiable bit for bit.

**NB stays a patchable knob.**  In the shipping build `CP_S0::NB` is a folded
literal and `SSE<CP_S0, CP_S0::NB>` is simply the table's element type, as
`SSE_Ctr<CP_S0, NB>` is today.  In the tuning build it is a load-time value,
so the row cannot be the `Table()` element there; instead the *table* is a
dispatcher that instantiates the row for every NB in [2, `SSE_NB_MAX`] and
switches on the value -- sec.2.4.  `SEEDS` is sized `SSE_NB_MAX` in the
bundle and filled for the NB in force, so one array serves every
instantiation.

### 2.3 The bundles pick up the constants

`config.hpp`, `#ifdef CP_SSE` block -- replacing the current `NB, HBITS, UPD`
/ `LIM, T0, UPMIN`:

```cpp
static const int   NB;               // the knob, as now; sse_nb_clamp()ed where it is used
static const int   UPD;              // iclamp(knob, 1, 2)
static const float LIM;              // clamp(knob/256, 0.25, 16): the |stretch| clip, already clamped
static const float ZMAX;             // rt_logf(65535): the cells' |logit| bound
static const float QSCALE;           // (NB-1) / (2*LIM)
static const float UPMIN;            // clamp(knob/256, 0, 1)
static const float T0;               // clamp(knob/256, 1/256, 4096)
static const SSE_Seeds SEEDS;        // per-bucket InitN counts: {a[SSE_NB_MAX], b[SSE_NB_MAX]}, filled for NB
```

`SEEDS` is the `a[j]`/`b[j]` loop of today's `SSE_Ctr::Init()` -- the identity
init inverted through the cell's output map at the seed K and mw, mass T0 --
moved into one small struct returned by one function and evaluated once at
load, through `rt_expf` as now.  It is the last definition of the bundle, after
`K`, `M`, `mwP0`, `LIM`, `T0` and `NB`, so the dependency order the tuning
build relies on is kept.

`config_mix2.hpp`:

```cpp
static const float ZM;    // rt_logf((1-Pmin)/Pmin): the input |logit| bound
static const float WX0;   // clip(rt_logf(w0/(1-w0)), Wclip), w0 = clamp(W0, 1/65536, 1-1/65536)
```

`WX0` is to `W0` what `MWX0` is to `mw` in `config.hpp`.

The clamps that today sit in `Init()` (`lim`, `upmin`, `upd`, `T0`) move into
these definitions, so IDX-FORMAT.md sec.5 ("consumers must clamp") still
holds: the bundle is the consumer of the raw knob, the component only ever
sees a sane value.

### 2.4 The tables are the IDX `Table()`s, indexed directly

Rule: `Table()` is the only place anything is allocated.  It is a fixed array
in the shipping build and a `new[]` in `%M%_Init()` in the tuning build, and
that is the whole of the coder's dynamic memory -- the components hold no
pointers, `main()` allocates nothing, and the per-query `Pred` structs live on
the stack.  (The tuning build's other allocations are the IDX runtime's own:
the `mapping`/`masking` descriptor tables that `sh_mapping.inc` builds for the
patchable knobs.  They are not the coder's and are untouched by this plan.)

The mixer table is a plain `Table()`, in `IDX/sh_model-M0.inc`:

```
Table( Mix2_Cell, %M%tbl, %M%Cx_Volume );     // was mix_table_ctx( Cx_Volume )
```

with `typedef Mix2<CP_M0> Mix2_Cell;` in `coder0.cpp`.  `Table()` already does
the right thing per build (a fixed array sized by the constant-expression
`_Volume` in the shipping build, a pointer allocated by `M0_Init()` from the
runtime `_Volume` in the tuning build, IDX-FORMAT.md sec.9); the only thing the
coder adds is the element type.  `tbl_n()` stays, since the generated code
calls it.

**The SSE table** cannot go through `Table( SSE_Row, ... )` in the tuning
build: its element type depends on NB, a runtime value there.  This is the
`SSEj` case of sec.1.4, and it gets the same answer, with the knob read from
the bundle instead of bound as a reference parameter:

```cpp
// tuning build only: the S0 table, resizable in NB
template<class CP> struct SSE_Tbl {
  void* p; uint n; int nb;
  void Init( uint rows );        // nb = sse_nb_clamp(CP::NB); switch( nb ) { case 2: p = new SSE<CP,2>[rows]; ... }
  void Quit();                   // the matching delete[]
  struct Ref {                   // what tbl[i] yields; the table keeps no per-query state
    SSE_Tbl& t; uint i;
    SSE_Pred<CP> Predict( float z ) const;   // switch( t.nb ) { case 2: return ((SSE<CP,2>*)t.p)[i].Predict(z); ... }
    void Update( int bit, const SSE_Pred<CP>& pr, float ef );
  };
  Ref operator[]( uint i ) { return { *this, i }; }
};
```

The S0 template declares the table through `def_Data` / `def_Init`, which
`idx2inc.pl` copies verbatim into `S0_T` *after* its own `#define USE_NEW`, so
the two forms select themselves:

```
def_Data
#if USE_NEW
  SSE_Tbl<CP_S0> %M%tbl;
#else
  SSE_Row<CP_S0::NB> %M%tbl[ %M%Cx_Volume ];      // template<int NB> using SSE_Row = SSE<CP_S0,NB>;
#endif
end_Data
def_Init
#if USE_NEW
  %M%tbl.Init( tbl_n(%M%Cx_Volume) );
  %M%_Size += %M%tbl.Bytes();
#endif
end_Init
def_Quit
#if USE_NEW
  %M%tbl.Quit();
#endif
end_Quit
```

The allocation is therefore still the generated `S0_Init()`'s -- the IDX
table's, the one place the rule of this section allows -- and `main()` spells
`S0.S0_tbl[cx].Predict(pr.z)` identically in both builds: a plain
`SSE<CP_S0,4>&` in one, a `Ref` in the other.  `def_Quit` / `end_Quit` is new
in `idx2inc.pl` (the exact mirror of `def_Init`, copied into `%M%_Quit()`
after the `Table()` `delete[]`s), so the table is freed where the `Table()`
members are; `%M%_Size` is a member the `def_Init` code can add to, so the
tuning build's byte count stays complete (the shipping build's is a
`constexpr` over the `Table()` lines only, and nothing in the coder reads
either).  Should a second table ever need the pattern it earns a `Table()`
form of its own.

Against today's `SSE_Dyn`: no virtual interface and no separately allocated
`Impl`; the dispatcher *is* the table rather than a global next to it; and the
row underneath has no `Cell* t`, `ncx`, `vol` or `Row()`.  The per-call
`switch` is on a load-time constant and exists in the tuning build only -- the
shipping build has neither switch nor vtable, as now.

`main()` then reads like the order-1 model already does:

```cpp
for( i=0; i<S0_Cx_Volume; i++ ) S0.S0_tbl[i].Init();
for( i=0; i<M0_Cx_Volume; i++ ) M0.M0_tbl[i].Init();
...
Counter<CP_C0>::Pred pr = o1[last_c][cxt].PredictF();
SSE_Row&   s  = S0.S0_tbl[ S0_MakeCx(c2, last_c, cxt) ];
Mix2_Cell& m  = M0.M0_tbl[ M0_MakeCx(c2, last_c, cxt) ];
SSE_Row::Pred   ps = s.Predict( pr.z );
Mix2_Cell::Pred pm = m.Mix( pr.z, ps.z );
p = uint( clamp( pm.p * float(SCALE) ) );
bit = rc.rc_BProcess( p, bit );

float e_f = pm.p - float(1 - bit);                       // dL/dz_f
o1[last_c][cxt].C_Update( bit, pr, 1.0f, e_f * (pm.d1 + pm.d2 * ps.dzdz) );
s.Update( bit, ps, e_f * pm.d2 );
m.Update( bit, pm );
```

Gone from `coder0.cpp`: `sse_table_cells`, `mix_table_ctx`, `TBL_CONSTEXPR`,
the `#if USE_NEW` choice of front end, the global `sse`/`mix` objects, both
`Init(table, ...)` calls, the `Cx*Cx3_Volume + Cx3` 64-bit combination.  Gone
from the .inc files: `SSE_Dyn` (replaced by `SSE_Tbl`), `sse_rows`, `Row()`,
`mix_rows`, `SSE_MAXCELLS_LOG`, `MIX_MAXCTX_LOG`, and the three chain
accessors (their values are now `Pred` fields).  `SSE_NB_MAX` and
`sse_nb_clamp()` stay: they bound the dispatcher's switch and clamp the knob
into it.

### 2.5 No hash means the S0 index must fit

This is the one part of the plan that changes the model.  Without `Row()` every
context gets its own row, so `S0_Cx_Volume * 384 B` is the table, full stop.
The present index (`c2` 6 bits x `c1` 5 bits x `cxt` 255 x `c3` 8 bits) would
be 51 GB; the present *table* is 1.6 GB with 32 contexts colliding per row.

The .idx author bounds the table by what goes into the index and by the
**pattern lengths**: opt.pl flips bits inside a `&` pattern but never widens
it, so an n-character mask contributes at most 2^n rows whatever the tuner
does, and the worst case is known before the first tuning pass.  For the same
1.6 GB as today:

```
Index Cx
 c2: c2, &011111      # 6-char pattern: at most 64 rows from c2
 c1: c1, &11111111    # at most 256
 cx: cxt, b&00000000  # 255 nodes
                      # max 64*256*255 = 4,177,920 rows * 384 B = 1.60 GB
```

That is the same row count the hash produces today, direct instead of 32:1
shared (at NB = 4; a tuner move on NB scales the table with it, which is one
more reason the pattern lengths, not the tuner, must own the budget).  Whether it compresses better is a measurement, not a given: the S0
rates were tuned on the colliding table.  A smaller start (`c2` and `c1` as
today, no `c3`: 522240 rows, 200 MB) is the cheaper first data point.  The
`Cx3` index and `S0_MakeCx3()` go in either case, and so does `HBITS`, which
only ever capped the hash.

The mixer side needs nothing: 522240 contexts are already direct-indexed, so
deleting its hash is a no-op on the stream.

### 2.6 `ON`, `UPD`, `UPMIN`

- **`S0_ON` / `M0_ON`** -- removed (commit "Remove the S0_ON / M0_ON stage
  bypasses", ahead of phase 1).  Both were 1; a bypass is pipeline wiring, not
  a property of a cell, and the log's "both stages bypassed" numbers were a
  one-time reference.  The `if( !CP::ON )` early-outs, the `? : 1.0f` forms of
  the chain accessors and the `ON ? rows : 1` table sizes went with them.
- **`UPD` / `UPMIN`** -- 1 (both bracketing cells get a full event) and 0.
  This is a genuine model variant opt.pl may flip, and the branch in
  `SSE::Update()` folds to nothing in the shipping build, so it stays, as a
  bundle constant.  If UPD=1 is later judged settled, the branch and `UPMIN`
  leave the same way the adaptation switches did.

## 3. Sequence

### Phase 0 -- baseline

`verify.sh` md5s of `book1`/`wcc386`/`book1wcc` on both builds; cell sizes by
the `static_assert` probe (96 / 28 today, 384 for the row after the change).
Both already recorded in this session's history.

### Phase 1 -- mechanical, byte-identical

1. `config.hpp` / `config_mix2.hpp`: add the sec.2.3 constants and
   `SSE_Seeds`; drop `HBITS`.
2. `sh_mix2.inc`: `Mix2` becomes the cell; `Pred`; `Init()`, `Mix()`,
   `Update()` with the bodies unchanged apart from where the state lives.
3. `sh_SSE.inc`: `SSE_Pred<CP>`, the `SSE<CP,NB>` row with `Init()`,
   `Predict()`, `Update()`, and `SSE_Tbl<CP>` in place of `SSE_Dyn`; delete
   the geometry helpers.
4. IDX templates: the M0 `Table()` line and the S0 `def_Data`/`def_Init`
   block of sec.2.4.  `.idx`: drop `HBITS` (`ON` is already gone, `NB`
   stays); **keep `Cx3` for now**.
5. `coder0.cpp`: sec.2.4, but with one temporary:

   ```cpp
   // phase 1 only: today's row hash, so that the stream is provably unchanged
   static uint S0_Row( qword cx ) { ... the current Row() over sse_rows(...) ... }
   SSE_Row& s = S0.S0_tbl[ S0_Row( qword(S0_MakeCx(..))*S0_Cx3_Volume + S0_MakeCx3(c3) ) ];
   ```

   and the S0 table sized by that row count for this phase.
6. Verify: `verify.sh` identical to phase 0 on both builds; `t1.sh`; the
   probe shows 96 / 384 / 28; `-DC0_CACHE_WR=0`, `-DTRACE_P`, `-DGRAD_TEST`
   still compile.

Byte identity here is the whole point of splitting: it proves that moving the
constants, introducing the `Pred` structs, fixing NB and turning the row into a
struct changed no arithmetic.  Once the index changes that proof is no longer
available.

### Phase 2 -- direct index (model change)

1. Delete `S0_Row()`, `sse_rows()`, the `Cx3` index, `S0_MakeCx3()`; the S0
   `Table()` is `%M%Cx_Volume`.
2. Choose the S0 index within the memory budget (sec.2.5), regenerate `MOD/`,
   measure, add the line to `log.txt` with the geometry stated.
3. opt.pl / optv.pl passes: the S0 masks first (which `c2`/`c1` bits), then the
   S0 rates and `T0`, which were tuned for a table where every row was shared.
4. Verify: `t1.sh` round-trips; shipping and tuning builds byte-identical
   (`builds-identical` in `t1.sh` -- this contract is unchanged); sizes logged.

### Optional, afterwards

- `Counter::C_Update` -> `Update` and `PredictF` -> `Predict`, so the three
  components read the same.
- `SSE-DESIGN.md` (not in this tree) needs its sec.3 geometry and the
  `SSE_Dyn` paragraph rewritten.

## 4. What does not change

- `Counter`, `ParamUpdater`, `RTRLState`, `WrCache`, `config.hpp` outside the
  `CP_SSE` block, `sh_mapping.inc`, `idx2inc.pl`, `opt.pl`.
- The A2 chain: `dzdz`, `sh0`/`sh1`, `d1`/`d2` are the same quantities in the
  same places, only returned instead of stored.
- The `UPD` proportional update, the young-cell schedule in the mixer, the
  `ZMAX`/`ZM` clips and the zero-derivative rule where a clip binds.
- Every knob except `HBITS` (and, in phase 2, the `Cx3` index) stays
  patchable, `NB` included; opt.pl's search space loses only that, on top of
  the two `ON` bypasses already removed.
- The shipping/tuning identity contract, and `gc.sh` / `t1.sh` as its test.
