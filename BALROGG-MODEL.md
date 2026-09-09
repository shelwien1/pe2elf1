# What balrogg's model has that tsvcomp's does not

Compared against **balrogg v1.3**, upstream `iczelia/balrogg` at `2adfb13`
(2026-09-07).  File references below are to that tree's `src/`.

## What is being compared

The `balrogg` in *this* repository is only half of the program upstream ships.
`main.cpp` here contains no coder at all — it takes an Ogg Vorbis stream apart
into a record stream and puts it back together, and `grep -c 'rc_enc\|cm_'
main.cpp` returns 0.  `tsvcomp` is the other half: it models that record
stream.  Upstream balrogg does both in one pass, so its model sees the same
Vorbis content through the same decomposition, and the two are directly
comparable on modelling grounds.

They agree on the shape of the problem.  Both spend ~95% of their output on
residue digits; both code a digit as **zero flag → sign → is-it-one → bit
length → mantissa bits**; both drive an adaptive binary probability with a
count-derived rate that saturates at a bound; both key that probability on
declared context axes.  What follows is what balrogg does *on top of* that,
that tsvcomp does not do at all.

| # | mechanism | balrogg | tsvcomp |
|---|---|---|---|
| 1 | indirect model: bit-history states + state map | 253 states, per stage | — |
| 2 | hashed contexts | 2^18 per stage | direct-indexed only |
| 3 | match model over the digit stream | 2^18 ring, 2^16 table | — |
| 4 | per-file parameter search, transmitted | 3 bytes, up to 12 trials | compiled in |
| 5 | effort ladder | 9 levels, stages switchable | one build |
| 6 | model banks keyed by codebook identity | 32 pooled slots | context axis only |
| 7 | explicit predictor (delta) on header fields | order 0/1/2 per field | context only |
| 8 | separate coder streams by content | 3 | 1 |
| 9 | floor classword derived, only corrections coded | yes | codes it outright |
| 10 | SIMD mixer kernels | SSE2 + AVX2 | scalar |
| 11 | mixer weight-set selection by neighbourhood | 200 rows | context-indexed rows |
| 12 | tail/​padding byte model | banked by previous byte | field-coded |

---

## 1. Indirect modelling — bit-history states and a state map

`cm.c:cm_init` builds a **253-state nonstationary bit-history machine** at
startup: each state stands for an (n0, n1) pair of observed zero and one
counts, with the pair rounded down as counts grow (`ilogt`, the `while
(!t[p][q][1])` walk), so recent bits weigh more than old ones.  `cm_nex[state]
[0..1]` are the transitions; `cm_nex[state][2..3]` are the counts; `cm_nexd`
is `(n1==0) - (n0==0)`, a determinism flag.

Each context slot holds **one byte — the state — not a probability**.  The
probability comes from a per-stage **state map** `sm[256]`, itself adaptive:

```c
u8 * sp = s->hist + (h & c->hmask);   /*  state of this context  */
int state = *sp;
uint64_t sm = *smp;                   /*  what that state currently predicts  */
...
*sp = cm_nex[state][bit];             /*  advance the state  */
*smp = cm_sm(nv, count + ..., state); /*  and what the state predicts  */
```

This is the classic two-level indirection: the context tells you *which
history*, the state map tells you *what a history like that usually means*.
It generalises across contexts — a context seen three times inherits what
every other context in the same state has learned — which is exactly the
regime a wide, sparse context lives in.

**tsvcomp has no equivalent.**  Its `cm_cnt` stores a probability and a visit
count directly, so a rarely-visited context learns only from its own visits.
Its two APMs refine an *already-formed* probability against a quantised
context; they are not an indirection over history.

This is the single biggest structural difference, and it is the one that most
directly addresses tsvcomp's known weakness: its measured wins come from very
wide contexts (the tuned set that codes `07` best asks for 178 M contexts on
`dig_a` alone), and wide contexts are sparse contexts.

## 2. Hashed contexts

`cm_new` allocates `1 << bits` bytes of history per stage (`CM_BITS 18`) and
indexes them by a hash of the exact context:

```c
u32 cm_hpre(a,b,c,d)  /*  fold slot, channel, index, memory bits  */
u32 cm_hst(base, xr, stage)
u8 * sp = s->hist + (h & c->hmask);
```

So the *addressable* context is unbounded — slot × channel × exact residue
index × memory bits — while the table is a fixed 256 KB per stage.  Collisions
are accepted; there is no check byte on the history table.

**tsvcomp is direct-indexed throughout.**  An IDX index is a product of factor
sizes and the table is that product times the node count; nothing hashes.  That
is why widening a context costs address space (now mapped rather than
allocated, so the cost is small, but the 2^31 limit on a `_Volume` is real and
the index arithmetic is `int`).  Hashing would let a context be exact —
`(slot, channel, exact index)` rather than `(band bucket, channel, column
bucket)` — without the product being materialised.

## 3. A match model over the digit stream

`cm.h`/`cm.c` keep a ring buffer of the last 2^18 coded digits (`mbuf`,
clamped to ±127 and biased to a byte), a rolling hash of the last
`CM_MMIN = 10` of them, and a 2^16-entry table from that hash to the position
that followed it.  On a hit the match is verified backwards and its length
recorded, capped at `CM_MMAX = 63`.  While a match holds, `cm_match` yields
the digit that followed last time, and every stage of the cascade is asked to
predict *that digit's bit*:

```c
mex = mok ? pv != 0 : -1;                        /*  zero flag  */
t = mix(&n->cm, 0, psel, PHASH(0), o, mex, v != 0);
if (t != (pv != 0)) mok = 0;                     /*  match broken  */
```

The prediction does not enter as a hard decision.  Two mixer inputs carry it:
`sign × stretch(P(the match is right))`, learned per `(stage, length bucket)`
in `c->mp[]` with 16 length buckets, and `sign × min(len,32) × 64`, the raw
confidence.  When the match breaks mid-digit every later stage falls back.

**tsvcomp has nothing of the kind.**  Its long-range structure comes from
`t1`/`t2` (the same slot one and two packets ago) and `ps` (earlier passes at
the slot), which are *fixed offsets*.  A match model finds a repeat wherever it
is.  Vorbis residue from a stationary passage repeats at whatever period the
music has, not at one packet.

## 4. Per-file parameters, searched and transmitted

`vb_tune` is three bytes — `alim` (the counter's count cap), `lr` (the mixer
learning rate), `flags` (two model switches) — searched at encode time and
written into the container:

```c
/*  codec.c:vb_pack  */
trial(&s, &t);                          /*  the default, always  */
t = s.bt;  t.flags |= VB_TF_CLS;    trial(&s, &t);
t = s.bt;  t.flags |= VB_TF_MATCH;  trial(&s, &t);
Fk(NALIM, t = s.bt;  t.alim = TRY_ALIM[k];  if (!trial(&s, &t)) break);
Fk(NLR,   t = s.bt;  t.lr   = TRY_LR[k];    if (!trial(&s, &t)) break);
```

It is a coordinate search over five axes from the current best, each trial a
full encode, budgeted by the effort level (`EFFORT[].search`, 0 at `-1`
through 12 at `-9`) and stopping when the budget runs out.  The winner's bytes
are kept — `trial` moves the candidate down in the same output file rather
than re-encoding it.

**tsvcomp's parameters are compiled in.**  `IDX/opt.pl` tunes them offline
against a corpus and the result ships in `MOD/`, identical for every file.
This is the difference the user of this tree already identified: the tsvcomp1
and tsvcomp2 columns in `log.txt` differ only by which file the parameters
were tuned on, and the tuned-on-07 set measured here beats the shipped one by
3.0 kB on `07` and 2.7 kB on `08` while losing 7.7 kB over the corpus.  Three
transmitted bytes would recover most of that spread, and the channel count and
stream length are already known to both sides for free.

## 5. An effort ladder that switches models off

```c
const u8 CM_LEVMASK[CM_NLEV] = { 0x00, 0x05, 0x1D, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
```

Level 0 disables the mixer entirely (arena probabilities code directly);
level 1 mixes only the zero flag and the is-one flag; level 2 adds the length
and mantissa stages; level 3 and up run all five.  `residue.h` is included
once per `RS_CM` mask value with the stage tests resolved at compile time, so
a disabled stage costs nothing at run time — the dispatch happens outside the
partition loop.

**tsvcomp has one build and one speed.**  Its decode is 1.09×–1.71× Rev F's
against a 2× budget, with no way to trade back.  The same template trick would
apply directly: `tc_fam::bit` is already one function per family.

## 6. Model banks keyed by codebook identity

`pool_slot` (`vorbis.c`) maps each codebook to one of `VB_NSLOT = 32` model
slots by the **similarity of its codeword-length vector**:

```c
d = pool_dist(v->sl + i, len, b->ent);      /*  sum |len_i - len_j|  */
nrm = (bd << 8) / nu;                       /*  per used entry  */
if (nrm <= 0x100) { ...reuse this slot... }
```

Books must agree on entry count, `nv`, and lookup geometry to be candidates;
a normalised distance of one length unit per used entry is close enough to
share.  If nothing is close a new slot is taken; if none is free the
least-used slot is evicted and **re-seeded** (`v->ai[k] = 0`), so a stale
slot's statistics do not leak into the book that replaces it.  Each slot owns
its own copy of all five residue tables (`arena`: `aglob + ns * astep`).

The point is that a codebook *is* a distribution.  Two books with the same
length vector describe the same shape of data even in different files, and
chained links reuse the pool.

**tsvcomp uses codebook identity as a context axis** (`cls` is the class, so
the choice of book; `bkq` is the book's `off`, log-quantised) but the tables
are shared: every book's statistics land in the same rows, separated only by
those axes and only as far as the axes' widths allow.  There is no pooling
across setups and nothing is ever re-seeded.

## 7. An explicit predictor on header fields

`model.h`'s `mdl_cfg` has an `order` field, and `model.c` acts on it:

```c
static u32 fwd(model * m, u32 v) {
  if (!m->c.order) return v;
  r = v - m->m0;  m->m0 = v;                    /*  first difference  */
  if (m->c.order > 1) { t = r - m->m1;  m->m1 = r;  r = t; }
  return r;                                     /*  second difference  */
}
```

Most of the 44 field models in `CFG[]` are order-1; codeword lengths, ordered
run lengths and multiplicands are order-0.  On top of that, every setup field
goes through `fld(z, k, n, p)` → `mv(...)`, which codes `v - p` for a
caller-supplied prediction `p`.  The model sees a *residual*, so its zero flag,
length tree and mantissa all work on a value that is usually small.

**tsvcomp uses the same information as context, not as a prediction.**
`hdr_a_p1` and `hdr_a_p2` are the previous two values under the same tag,
log-quantised, feeding the index; the value coded is the value itself.  That
costs resolution twice — the quantiser throws away the low bits of the
prediction, and the cascade then codes a full-width value rather than a small
difference.  For a monotonically increasing field (a granule position, a serial
number) the difference is large.

## 8. Separate coder streams

```c
#define S_BULK  0   /*  setup headers, and the audio payload  */
#define S_MODE  1   /*  page headers, and the packet modes  */
#define S_TYPE  2   /*  one packet-type value per packet  */
```

Three range coders run concurrently over one file, so the arithmetic coder's
carry and normalisation state is never shared between a dense, stationary
stream (page headers, one per page) and a sparse one (residue digits, millions
per file).

**tsvcomp codes everything into one `rc`.**  This costs nothing in modelling
terms — the models are separate either way — but it does mean the coder state
for the header stage is interleaved with digits.  Upstream's split also lets
the decoder read modes ahead of the payload, which is what makes its
packet-type probe possible.

## 9. The floor classword is derived, not coded

Vorbis floor 1 writes a classword that selects a subclass book per post.  The
classword is *usually* recoverable: given the post values, the first subclass
book whose entry count covers each value is the canonical choice.  balrogg
reconstructs it and codes only the difference when the encoder chose otherwise:

```c
/*  fl_put  */
Fk(cd, Fj(1UL << cs, ... if (y[post + k] < mx) break);  cv |= j << k * cs);
if (z->choices) cv = mv(z, S_BULK, M_FCLASS, classes[i], cv);
```

`z->choices` is set during the probe pass (`fl_get`) only when some post could
have been coded by more than one book, so a floor whose classword is forced
costs **zero bits**.

**tsvcomp codes the classword.**  `fam_cls` models residue classifications and
`fam_flr` the posts; the floor classword travels as records and is modelled
like any other value.  Deriving it and coding only corrections is a
decomposition change rather than a model change — it would have to happen in
`main.cpp`/`vb_floor.inc` — but it is the kind of thing that removes a field
rather than predicting it better.

## 10–11. The mixer

Six live inputs, eight lanes, one `_mm_madd_epi16`:

| lane | input |
|---|---|
| 0 | `stretch(arena P)` — the direct-context counter, the caller's prediction |
| 1 | `stretch(state map P)` — the indirect model |
| 2 | 256 — bias |
| 3 | `stretch(state map P) × nexd[state]` — signed by the state's determinism |
| 4 | `sign × stretch(P(match correct))` |
| 5 | `sign × min(matchlen, 32) × 64` |

Weights are `short`, updated with `_mm_adds_epi16` (saturating), and the
weight *set* is chosen by

```c
psel = ((c1 * 5 + cx) * 2 + (ch & 1)) * 4 + (m & 3);   /*  0..199  */
```

— the class of the previous digit in this channel's run (5), the class of the
immediately preceding digit if it came from another channel (5), the channel
parity (2), and two bits of cross-pass memory (4).  `cm_squash` interpolates a
33-entry table exactly, with one multiply.

**tsvcomp's mixer is seven counters plus a scalar dot product**, its weight
row chosen by a full IDX index (`dig_m`), and no SIMD.  The inputs are all of
the same *kind* — six adaptive counters and an APM-refined one — where
balrogg's six are of four different kinds.  Mixing predictors that fail
differently is what a mixer is for; mixing six counters that differ only in
context is a weaker use of it.

The **cross-channel selector** (`cx`) deserves separate mention: it is the
class of the immediately preceding digit *when that digit belonged to another
channel*.  tsvcomp's `q1`/`q2` cover the preceding digits in coding order, and
for an interleaved type-2 residue those are cross-channel — but they enter as
index factors, quantised, not as a weight-set selector, and tsvcomp has no
*aligned* cross-channel value (the other channel at the same bin) at all.

## 12. Tail and padding bytes

A Vorbis packet can end with bits that the format's syntax does not account
for.  balrogg probes each packet (`audio_type`), and when there is a tail it
codes those bytes through a byte tree **banked by the high nibble of the
previous byte** (`VB_TBANK = 8` banks of 256 nodes) — an order-½ byte model
for what is usually padding or an encoder quirk.

**tsvcomp codes the tail as a field** (`F_TAIL` in its header enum) through
the `aux` family, with the field id as the context.  The bytes get no
byte-context of their own.

---

## What tsvcomp has that balrogg does not

For orientation, since the picture is not one-sided:

* **The codebook prior.**  `tc_prior.inc` turns each codebook's Huffman
  codeword lengths into a conditional distribution over the digits of an
  entry — P(entry) = 2^-len, conditioned on the prefix the earlier places have
  spelled — and feeds it to the mixer as `pq`/`sq` and as a context axis.  It
  is the one thing a Vorbis file knows about its own residue that no coded
  history supplies.  Measured against it: coding digits at the codebook's own
  P(entry) = 2^-len costs 3.128, 1.285 and 2.294 bits on `01`, `07` and `09`,
  where the model spends 2.902, 1.150 and 2.097 — so the model beats Vorbis's
  own Huffman by 7–10% on the digits, using the prior as one of its inputs.
  balrogg reads the same lengths and uses them only to parse.
* **Two APM/SSE stages per family**, one refining counter A before the mixer
  and one refining the mixer's output, each with its own curve width.
* **A declarative context language.**  Every axis in every index is a pattern
  an optimizer can widen or narrow in the binary without a rebuild, which is
  how tsvcomp's contexts were arrived at.  balrogg's axes are C expressions.
* **Far richer per-digit context.**  `ax` (a weighted mean of the
  neighbourhood), `n1`/`w1` (slot ± dim), `t1`/`t2` (the same slot one and two
  packets back), `zrun`, `crun`, `p0`/`ps`/`pn` (three views of what earlier
  passes left).  balrogg's arena axes are comparatively plain; its power comes
  from the indirect model and the mixer, not from the axes.

## Where the leverage is

Ordered by expected return against effort, for this tree:

1. **Bit histories and a state map** (§1) — addresses sparse wide contexts,
   which is where tsvcomp's measured gains already come from.
2. **Per-file transmitted parameters** (§4) — the spread between profiles is
   measured and large, and the selector is free.
3. **A match model** (§3) — orthogonal to everything tsvcomp has, and the
   only mechanism here that finds structure at an unknown period.
4. **Hashed contexts** (§2) — mostly a way to make 1 and 3 affordable, and to
   lift the `int` ceiling on a Volume.
5. **An explicit predictor on header fields** (§7) — small in absolute terms
   (headers are ~2% of output) but cheap to do.
6. **The effort ladder** (§5) — buys back decode time rather than bytes.
