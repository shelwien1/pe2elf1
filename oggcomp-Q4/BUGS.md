# Four bugs in oggcomp

Found while porting the coder to PSRC, but none of them is about that: all
four are in oggcomp as it shipped, and this document is written so that they
can be fixed in a tree that has never heard of PSRC.

Everything here was reproduced against a build straight from the
`oggcomp-Q4` tarball -- `OC_VER = 5`, `oggdet 1.6`, `CXXFLAGS=-O2 ./mk.sh
release`, g++ 13.3, x86-64 Linux.  Line numbers are that tree's.  The three
fixes were applied together and checked: all 34 bundled testfiles code to
**byte-identical** `.oc` files before and after, `./t.sh` passes, and UBSan
goes from 68 reports to 0.

They are all still present in this repository's copy as well -- the PSRC
change did not touch any of the four -- and **none of them is fixed here**.
Applying them is a separate piece of work.

| | what | severity |
|---|---|---|
| 1 | `OC_F_SOLID` on a decode after an encode: null dereference | crash, library callers |
| 2 | `Loop` with no output buffer: null write | crash, library callers |
| 3 | `tc_qlog` shifts by -1 on every input | undefined behaviour, always |
| 4 | a valid Ogg with a big granule jump is refused | contract violation |

---

## 1. `OC_F_SOLID` on the first decode of an instance that has encoded

**Symptom.** SIGSEGV inside the model, on a library call sequence `oc_api.h`
describes as supported.

**Reproduction.** Against `oggcomp1.so` from `./mk.sh dll 1`:

    p = oggcomp_Alloc();
    oggcomp_Init(p, OC_ENCODE, OC_F_SOLID, "x");   /*  ... encode a stream ...  */
    oggcomp_Init(p, OC_DECODE, OC_F_SOLID, "x");   /*  ... decode it back ...   */

    #0  tc_fam<1>::code(tcx const&, long)
    #1  oc_model<1>::auxc(int, int, long, int)
    #2  og_unpacker<oc_model<1> >::unpack(char const*)
    #3  oc_coro<1>::do_process()
    ...
    #6  oggcomp_Loop

With `flags = 0` the same sequence round-trips.  The reverse order --
decode first, then encode with `OC_F_SOLID` -- does not crash, for the
reason below.

**Mechanism.** `oc_api.inc:71`

    keep = (flags & OC_F_SOLID) != 0 && p->used;

`used` is one flag for the whole instance (`oc_api.inc:9`, set at `:91`),
but what `keep` decides to skip is *per direction*: `oc_fresh` →
`oc_model<f_DEC>::reset()` → `wire_all()`, which is what gives the
direction's six `tc_fam` objects their table pointers.  So the second Init
on an instance skips the wiring no matter which direction it is for, and the
direction that has not been Init'd yet has never been wired at all: `A`,
`B`, `C`, `D` are null, and `tc_fam::bit` reads `A[ba + node]`.

The encoder survives it only by accident -- `oggcomp_Init` calls
`p->enc.t.init()` once at `oc_api.inc:75`, and `init()` also calls
`wire_all()`.  There is no equivalent for `p->dec`.

**Reachability.** Not from `oggcomp` (one Init per run) and not from
`oggdet` (carve and restore are separate runs, and within either one the
direction never changes).  It is reachable by anything else that drives
`oc_api.h`, which is what the library exists for -- a caller that verifies
each stream after writing it is the obvious case.

**Fix.** Make the flag say what it is being asked: per direction.

    -  int busy, dir, flags, err, in_coro, used;
    +  int busy, dir, flags, err, in_coro, used[2];
    ...
    -  oc_the.used = 0;                                  /* oc_api.inc:48  */
    +  oc_the.used[0] = oc_the.used[1] = 0;
    ...
    -  keep = (flags & OC_F_SOLID) != 0 && p->used;      /* oc_api.inc:71  */
    +  keep = (flags & OC_F_SOLID) != 0 && p->used[p->dir];
    ...
    -    p->used = 1;                                    /* oc_api.inc:91  */
    +    p->used[p->dir] = 1;

The first Init of each direction is then always fresh, which is what a solid
run's first segment gets anyway, and a solid run continues as before.
Checked: the crashing sequence above now round-trips byte for byte.

**What the fix does not do.** Interleaving directions under `OC_F_SOLID`
still cannot do anything useful, because the model state the two share is
one set of tables and whichever direction resets clobbers the other's chain.
The fix makes that safe rather than fatal.  If the intent is that it should
never be attempted, the stronger version is to remember the last direction
and refuse `OC_F_SOLID` across a change.

---

## 2. `oggcomp_Loop` with no output buffer attached

**Symptom.** SIGSEGV on the first byte the coder emits.

**Reproduction.** Alloc, Init, then drive `Loop`/`addinp` to completion and
never call `oggcomp_addout`:

    #0  (a store through pin[1].ptr, which is null)
    #1  tc_fam<0>::code(tcx const&, long)
    #2  tc_fam<0>::codes(tcx const&, long, unsigned int)
    #3  oc_model<0>::hdrval(int, long)
    #4  io_t<oc_model<0> >::ident()
    ...
    #11 oggcomp_Loop

The first few `Loop` calls return `OC_NEED_INPUT` quite happily; the fault
comes when the model has something to write.

**Mechanism.** `Lib3/coro3_pin.inc:43`

    void put( uint c ) { *ptr++ = c; chkout(); }

writes and *then* asks whether the buffer has run out.  That is right for a
buffer with room in it -- it is what makes the yield land at the end rather
than one byte early -- and it is a null dereference for `ptr == end == 0`,
which is what `coro_init` leaves behind (`coro3_pin.inc:8`) and what an
instance that was never given an output buffer still has.

`oggcomp_Loop` already guards the other ways a caller can get this wrong
(`oc_api.inc:103` returns 8 for a foreign pointer or a missing coroutine);
this one it does not.

**Reachability.** Only by a caller that skips a step `oc_api.h`'s example
performs.  It is in this list because the failure is a wild store rather
than the error return the same function gives for every other misuse, and
because the library is loaded by name at runtime and driven by code its
author does not control.

**Fix.** One guard, beside the ones already there, rather than changing the
pin protocol that `Lib3` shares with everything else:

     if(p != &oc_the || !p->co)
       return 8;
    +  //  Nowhere for the output to go.  The coroutine's pin writes a byte
    +  //  and then asks whether it has run out, which is right for a buffer
    +  //  and a null dereference for no buffer, so the check has to be here.
    +  if(!p->co->pin[1].beg)
    +    return 8;
     if(p->err)
       return p->err;

Checked: `Loop` returns 8 instead of faulting, and nothing else changes.

---

## 3. `tc_qlog` shifts by -1, on every input

**Symptom.** Undefined behaviour on every file, both directions.  Under
`CXXFLAGS='-O1 -g -fsanitize=undefined' ./mk.sh release`, encoding and
decoding the 34 bundled testfiles produces 68 reports and they are all the
same one:

    tc_base.inc:14:32: runtime error: shift exponent -1 is negative

**Mechanism.** `tc_base.inc:11`

    static INLINE i32 tc_qlog(i64 x) {
      u64 u = (u64)(x < 0 ? -x : x);
      int n = 62 - __builtin_clzll(u | 1);
      i32 q = 2 + n * 2 + (i32)((u >> n) & 1);
      return u < 4 ? (i32)u : q;
    }

For `u` of 0 or 1, `u | 1` is 1, `clzll` is 63, and `n` is **-1**.  `q` is
then computed with `u >> -1` and thrown away by the `u < 4` return -- but it
is computed, and a negative shift count is undefined.  It happens in every
run -- the 68 reports are one per process, 34 encodes and 34 decodes --
because `tc_qlog` is the context function for every family and a remembered
value of 0 or 1 is the commonest thing there is.

On x86-64 the generated `shr` takes its count modulo 64, so the discarded
value comes out as `u >> 63` and nothing observable happens.  That is the
only reason this has never been seen.  It costs nothing to make it defined,
and until it is, the program cannot be built with UBSan to look for anything
else.

**A second, latent one on the same line.** `-x` for `x` the most negative
`i64` overflows, which is also undefined.  It is unreachable today only
because bug 4's cap refuses any value that large before it can reach here --
so a fix for bug 4 that raises the cap must fix this too.

**Fix.** Leave before `n` exists, and take the magnitude without negating a
signed value:

    static INLINE i32 tc_qlog(i64 x) {
      u64 u = x < 0 ? ~(u64)x + 1 : (u64)x;
      int n;
      if(u < 4)
        return (i32)u;
      n = 62 - __builtin_clzll(u);
      return 2 + n * 2 + (i32)((u >> n) & 1);
    }

Same answer for every `u`: the early return covers 0..3 exactly as the old
`u < 4` did, and for `u >= 4` `clzll(u) <= 61` so `n >= 1` and the `| 1` is
not needed.  Checked: all 34 testfiles code byte-identically, `./t.sh`
passes, UBSan reports 0.

---

## 4. A valid Ogg file with a large granule jump is refused

**Symptom.**

    oggcomp: this stream holds: a value of 2^62 or more

on a structurally valid Ogg file -- correct capture pattern, correct segment
table, correct page CRC.  `oggcomp c` exits 1 and writes nothing.

**Reproduction.** Take any file the compressor handles, set one page's
granule position, fix that page's CRC, and nothing else:

    page 3 of 5, granulepos <- 0xFFFFFFFFFFFFFFFF   round-trips
    page 3 of 5, granulepos <- 0x4000000000000000   round-trips
    page 3 of 5, granulepos <- 0x7FFFFFFFFFFFFFFF   REFUSED
    page 3 of 5, granulepos <- 0x8000000000000000   REFUSED

A 1200-case mutation fuzz over the bundled corpus found this once by
accident; the four lines above are the minimal form of it.

**Mechanism.** `oc_frame.inc:41`

    g = ENC ? (i64)(((u64)p.ghi << 32) | p.glo) : 0;
    g = auxc(F_GRAN, tc_qlog(tc_last2[F_GRAN]), ENC ? g - pg_prev : 0, 1) + pg_prev;

The granule position is an arbitrary 64-bit field -- the Ogg framing spec
leaves its meaning to the codec -- and what gets coded is the signed
difference from the previous page's.  Two things follow.  The difference can
need 64 bits, and the value coder refuses anything needing 62 or more
(`tc_fam.inc:219`, `TC_NBMAX` at `tc_base.inc:8`).  And `g - pg_prev` is a
signed `i64` subtraction that can overflow, which is undefined -- for
instance `g = -2^63` after any positive `pg_prev`.

`0xFFFFFFFFFFFFFFFF` is fine because as an `i64` it is -1, so the delta is
small.  That is the value real encoders write for "no packet ends on this
page", which is why the corpus never trips it.

**Why it counts as a bug rather than a limit.** The contract the tree states
for itself -- `t.sh`'s own header -- is that `oggcomp c` "takes any file --
an Ogg with a tag in front, a stream cut off halfway, a file that is not Ogg
at all -- and gives it back byte for byte".  There is a whole-page raw
fallback for pages that do not parse as Vorbis (`page_fits`, `pgraw`), but
it does not help here: `page()` codes the page header fields, granule
included, for raw and parsed pages alike.  So there is no path by which this
file gets compressed at all.

**Fix.** Not attempted, and not a one-liner, so what follows is the shape
rather than a patch.

The narrow fix is to make the value coder able to carry a full 64-bit
magnitude: raise `TC_NBMAX` to 64, and change `tc_fam::code`/`codes` to take
and return the magnitude as `u64` rather than `i64` -- `codes` currently
negates with `x < 0 ? -x : x` (`tc_fam.inc:237`), which overflows on the
most negative `i64` for the same reason as bug 3, and `code` rejects a
negative argument at `:189`, which is what a magnitude of 2^63 looks like
after the cast.  The node tables already reach: `tc_node_len(63)` is 28
against `TC_NODE = 29`, and `tc_node_man` tops out at 168 against
`TC_MNODE = 169`.

That is backwards compatible in the direction that matters -- values of 2^62
and over could not be coded before, so no existing `.oc` contains one and
every existing stream decodes bit for bit -- but it is a format extension,
so it wants the stream version bumped if old binaries must refuse the new
streams rather than mis-read them.

The alternative, if the value coder is not to be touched, is to code the
granule position as two 32-bit halves.  That changes the format for every
file rather than only for the ones that cannot be coded now, and would want
measuring.

---

## What was checked and found clean

So that the next person knows where not to look.  All against the same
pristine build.

- **The encoder's contract, 1200 cases.** Ogg files from the bundled corpus
  mutated -- bit flips, byte bursts, truncations, spliced `OggS` capture
  patterns, duplicated chunks, up to three at a time -- then encoded and
  decoded and compared.  1199 round-tripped byte for byte; the one that did
  not is bug 4.  No crash, no hang, no unbounded output.
- **The decoder against damage, 600 cases.** Valid `.oc` files mutated the
  same way and decoded under a 4 GB address-space limit and a 30-second
  timeout.  479 clean refusals, 120 decoded to nonsense as a lossless
  archive without a checksum must, 1 I/O error.  No crash, no hang.
- **`oggdet`'s archive decoder, 400 cases.** Same treatment of an
  `oggdet c -c` archive.  No crash, no hang.
- **The dry-run flag cannot leak.** `vb_dry` (`oc_fatal.inc:135`) is set
  around `page_fits` (`ogg_stream.inc:261-268`) and would throw
  `vb_dry_refusal` with no handler if it were ever set at a `Loop` return.
  It cannot be: the dry walk runs against `null_sink`, and the only things
  in it that touch the source -- `join_len`, `join_pkt` -- work inside the
  window and refuse rather than ask for more input, so the dry run never
  yields.
- **The codebook arena cannot leak across solid segments.** `vb_ar`
  (`vb_setup.inc:23`) is a bump allocator that `oc_next` does not reset, but
  `vb_ctx::init`/`link` (`vb_setup.inc:161-169`) give it back at every link,
  which is per stream.
- **`Loop` after `Free`** returns normally rather than faulting: `Free`
  clears only `busy`, and `Alloc` clears `co`.  Untidy, not a defect.
