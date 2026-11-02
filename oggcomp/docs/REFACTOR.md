# Splitting oggcomp.cpp

`oggcomp.cpp` is 3181 lines in one translation unit: the Ogg container, the
Vorbis setup and packet grammar, a range coder, the context-mixing
primitives, the codebook-derived probability tables, the per-family bit
coder, six models, the model assembly, the stream walker and `main()`.  The
plan is to move each of those into a `.inc` of its own, included from
`oggcomp.cpp` in the order the single translation unit needs, and to drop
what nothing references -- while producing **exactly the same program**.

**Done.**  Step 1, the deletions, is commit `3ad267d`; step 2, the move,
is `91fd0e8`.  Both were verified as section 7 says -- every corpus file
codes byte-identically under both builds against binaries built before
either change, and the reassembled source of the move is line-for-line
the file it came from.  What follows is the plan as it was carried out;
its line numbers are those of `oggcomp.cpp` at commit `413b568`, before
step 1.

## 1. Ground rules

- **The stream is the test.**  After every step, the new binary must write
  a byte-identical `.oc` for every file in `testfiles/`, under both the
  tuning and the shipping build.  `./t.sh` and `./mk.sh check` are
  necessary but not sufficient -- they prove the program is self-consistent,
  not that it is the *same* program.  Section 7 has the exact recipe.
- **One translation unit, as now.**  Nothing becomes a separate compilation
  unit; `mk.sh` keeps compiling `oggcomp.cpp` alone.  The `.inc` files are
  included in order and rely on what came before, which is how `Lib3/`,
  `sh_mapping.inc` and `MOD/` already work.  `#pragma once` and include
  guards are not needed and would be misleading.
- **Deletions first, then the move, and the move is a move.**  Both
  compilers' unused-macro reports, and clang's unused-function report, look
  only at the main source file -- a macro or a static function that sits
  in an included file is never reported (section 5 shows this).  So the
  dead code has to go while it is still in `oggcomp.cpp`, where the
  compiler can vouch for each removal; a split first would hide the
  evidence.  The move then relocates text without editing it, and is
  checked by reassembling the includes and comparing the sorted lines of
  the result with the sorted lines of the file it came from: they must be
  identical.
- **`Lib3/`, `IDX/`, `MOD/` and `sh_mapping.inc` are not touched.**  The
  first three by instruction; the last is IDX's runtime, carried verbatim
  from the psrc tree with a header saying so.  Section 5 lists what is
  dead in it, for the record, and recommends leaving it.
- **Every build flag keeps working:** `OC_CARRYLESS`, `TC_MEMCOST`,
  `TC_MEMCAP`, `OGC_NO_ATTRS`, `FILE_API_WIN`, `_WIN32`, and `USE_NEW`
  (which `MOD/` sets, and which changes what the generated code references
  -- see section 4).

## 2. What is in the file

| lines | what | goes to |
|---|---|---|
| 2 | `ogc_prog` | `oc_fatal.inc` |
| 4–101 | system includes, integer typedefs, `g_prog_in`, `__min`/`__max`, `ALIGN`, `if_e0`/`if_e1`, `__assume`, `DIM`, `X64`, the attribute macros (`INLINE` … `OGC_PRINTF`), `ogc_ilog`, the `_WIN32` block | `oc_platform.inc` |
| 103–105 | the three `Lib3/` includes | stays in `oggcomp.cpp` |
| 107–193 | `OGC_EXIT_*`, `FATAL`/`FATAL_CODE`/`FATAL_UNLESS`/`FATAL_IF_HOT`, `Fi`/`Fj`/`Fk`, `ogc_min`, `message`, `ogc_set_prog`, `ogc_partial`/`die`, the three `ogc_fatal*`, `ogc_paths_distinct` | `oc_fatal.inc` |
| 195–311 | `OGG_HDRMIN`, `OGG_MAXSEG`, `MAXPAY`, `PAGE_MAX`, the CRC table, `wr32`/`rd32`, `struct ogg_page` | `ogg_page.inc` |
| 313–473 | `vb_info`, `VB_MAX*`, the codebook arena, `vb_book`, `vb_floor`, `vb_res`, `vb_map`, `vb_setup`, `vb_su`, `vb_ctx` | `vb_setup.inc` |
| 475–480 | `rc_pin_io` and `#define RC_IO_BASE rc_pin_io` -- the byte I/O the coder is wired to | stays in `oggcomp.cpp` (6 lines; it is the adapter, not the coder) |
| 481–652 | `RC_*` configuration, `Rangecoder<f_DEC, CARRYLESS>` | `rc.inc` |
| 653–805 | `CM_*`, the squash/stretch/reciprocal tables, `cm_tables`, `cm_cnt`, `cm_apm`, `CM_UNROLL`, `cm_mix<N>` | `cm.inc` |
| 807 | `#include "sh_mapping.inc"` | stays |
| 808–857 | `RS_DIGIT_MAX`, `tc_enc`, `OC_CARRYLESS`, the `rce`/`rcd` instances, `oc_v`, `TC_MIXN`/`TC_NODE`/`TC_MNODE`/`TC_NBMAX`, `tc_node_len`/`tc_node_man`/`tc_qlog`/`tc_abs`/`tc_clamp`, `tc_alloc`, `tc_huge`, `tc_map` | `tc_base.inc` |
| 1853–1856 | `tc_sq` | `tc_base.inc` -- it sits in the residue block but the floor coder calls it too (the dry run found this) |
| 859–1360 | `TCP_*`, `tcp_pow`/`tcp_p`, `tc_ptab`, `tc_pcur`, `tcp_head`/`len`/`sign`/`man`/`bucket`/`axis`, the `tcp_book`/`tcp_cls`/`tcp_flr` arrays, `tcp_free`, `tcp_build` | `tc_ptab.inc` |
| 1362–1421 | `TC_VOLMAX`, `tc_vmul`, `tbl_n`, `tc_vfits`/`tc_ifits`, the `F_*` enum, `TC_MAXTAG`, `TC_SGN_*`, `TC_*_ND`/`_MND`, the six `MOD/*_h.inc`, the `tcm_*` pointers, `tc_dv`, `tcx`, the six `MOD/*_p.inc`, the `STG_*` enum, `TC_STAGE`, `tc_bits`/`tc_syms`/`tc_stage`/`tc_verbose` | `tc_tables.inc` |
| 1422–1660 | `tc_bit`, `struct tc_fam`, the six `fam_*`, `tc_last`/`tc_last2`, `tc_auxc`/`tc_aux` | `tc_fam.inc` |
| 1661–1706 | `TC_SPANMAX`/`TC_HISTMAX`, the `dg_*`/`cl_*`/`fl_*` histories, `tc_blk`, `tc_nchan`, `tc_hist_free` | `oc_hist.inc` |
| 1708–1742, 1784–1817, 2090–2162 | `oc_floor`, `oc_fl`, `oc_floor_tables`; `vd_render`/`vd_fold`/`vd_unfold`; `tc_used`, `oc_fno`, `fl_cur`, `fl_fy`, `oc_flr` | `oc_floor.inc` |
| 1818–1852, 2271–2278 | `OC_TAGMAX`, the tag table, `tc_tagid`, `tc_hdrval`; `oc_hdr` | `oc_header.inc` |
| 1857–2089 | `tc_gcd`, `cl_run`/`cl_same`/`cl_run_buf`, `R`, `P`, `oc_part_begin`, `oc_digit`, `tc_classify`, `oc_residue_begin`, `oc_cls` | `oc_residue.inc` |
| 2163–2270 | `oc_mode`/`oc_wprev`/`oc_wnext`; `pg_*`, `oc_seq`, `oc_page`, `oc_link`, `oc_link_begin`, `oc_spill` | `oc_frame.inc` |
| 1743–1783, 2279–2340 | `tc_setup_done`; `TC_MEMCOST`, `TC_WIRE`, `TC_MEMCAP`, `tc_tables`, the `static_assert`, `tc_models`, `tc_nomem`, `struct oc_model`, `sink_t` | `oc_model.inc` |
| 2341–2792 | `VB_MAXBLK`/`VB_MAXDIM`/`VB_YSMAX`/`VB_CSMAX`, `vb_ys`/`vb_cs`, `lk1`, `QUANT`, `struct io` | `vb_packet.inc` |
| 2793–3011 | `IN_LOW`/`IN_WIN`, `source_pull`, `in_win`, `struct source`, `jb`, `struct link_walk`, `pgbuf`/`body`, `page_writer`, `vb_pack`, `vb_unpack` | `ogg_stream.inc` |
| 3012–3181 | `OC_MAGIC`/`OC_VER`, `struct oc_coro`, `oc_run`, `main` | stays in `oggcomp.cpp` |

## 3. The files, in include order

The prefix says which layer a file belongs to: `ogg_` the container,
`vb_` Vorbis, `rc`/`cm` the two generic coding components, `tc_` the model
machinery, `oc_` this program's own glue.  (A flat `oc_` prefix on all of
them would also do; what matters is that the order below is the order in
`oggcomp.cpp`.)

| # | file | lines | needs, from earlier | provides, to later |
|---|---|---|---|---|
| 1 | `oc_platform.inc` | 97 | -- | typedefs; `ALIGN`, `DIM`, `if_e0`/`if_e1`, `__assume`, `X64`, `g_prog_in`, `FILE_API_STD` (**all read by `Lib3/`**); `NOINLINE` (read by `sh_mapping.inc`); `__min`/`__max` (read by the shipping-form `MOD/*_p.inc`); `INLINE` etc. |
| | `Lib3/coro3b.inc`, `Lib3/file_api.inc`, `Lib3/coro_fhp2.inc` | | as now | `Coroutine`, `filehandle`, `CoroFileProc` |
| 2 | `oc_fatal.inc` | 89 | `stdio`, `stdarg`, `NORETURN`, `OGC_PRINTF` | `FATAL*`, `Fi`/`Fj`/`Fk`, `ogc_min`, `OGC_EXIT_*` |
| 3 | `ogg_page.inc` | 114 | `FATAL_UNLESS`, `Fi`/`Fj` | `ogg_page`, `OGG_MAXSEG`, `MAXPAY`, `PAGE_MAX` |
| 4 | `vb_setup.inc` | 162 | `FATAL*`, `Fi`/`Fj`/`Fk` | `vb_setup`, `vb_ctx`, `vb_book`…, `VB_MAX*` |
| | *adapter, in `oggcomp.cpp`* | 6 | `Coroutine` | `RC_IO_BASE` |
| 5 | `rc.inc` | 172 | `RC_IO_BASE`, `LIKELY`/`UNLIKELY` | `Rangecoder`, `SCALElog` |
| 6 | `cm.inc` | 154 | `math.h` | `cm_cnt`, `cm_apm`, `cm_mix`, `cm_stretch`/`cm_squash`, `CM_*`, `cm_tables` |
| | `sh_mapping.inc` | | `NOINLINE` | `mapping`, `masking`, `pdesc`… |
| 7 | `tc_base.inc` | 55 | `Rangecoder`, `vb_ctx`, `FATAL_CODE`, `mman.h`/`windows.h` | `rce`/`rcd`, `tc_enc`, `oc_v`, `TC_NODE`…, `tc_qlog`/`tc_abs`/`tc_clamp`/`tc_sq`, `tc_alloc`, `tc_map` |
| 8 | `tc_ptab.inc` | 503 | `cm_stretch`, `CM_PONE`/`CM_STMAX`, `tc_clamp`, `vb_setup`, `oc_v` | `tc_ptab`, `tc_pcur`, `tcp_*`, `tcp_build`/`tcp_free` |
| 9 | `tc_tables.inc` | 60 | `cm_cnt`, `mapping`, `tc_vmul`, `tbl_n`, `tc_map`, `TC_*_ND`, `TC_MIXN`, `TC_SGN_*`, `tcx`, `tc_dv`, `TC_NODE`/`TC_MNODE`, `__min`/`__max` -- **all of it consumed by the generated `MOD/` code it includes** | `TC_*_T`, `tcm_*`, `tc_make_*`, `tc_vmul`, `tbl_n`, `tc_vfits`/`tc_ifits`, `F_*`, `STG_*`, `tc_stage`/`tc_verbose`/`tc_bits`/`tc_syms` |
| 10 | `tc_fam.inc` | 239 | everything in 6–9 | `tc_fam`, `fam_*`, `tc_bit`, `tc_auxc`/`tc_aux` |
| 11 | `oc_hist.inc` | 47 | `VB_MAXRES`, `VB_MAXCH` | `dg_*`, `cl_*`, `fl_hist`/`fl_yhist`/`fl_who`, `tc_blk`, `tc_nchan`, `tc_hist_free` |
| 12 | `oc_floor.inc` | 142 | `fam_flr`, `tcp_flr`, `tc_auxc`, `fl_*`, `tc_blk`, `tc_nchan`, `tc_sq`, `tc_make_flr` | `oc_fl`, `oc_floor_tables`, `oc_flr`, `tc_used` |
| 13 | `oc_header.inc` | 43 | `fam_hdr`, `tc_make_hdr` | `oc_hdr`, `tc_tagid` |
| 14 | `oc_residue.inc` | 233 | `fam_dig`/`fam_sgn`/`fam_cls`, `tcp_book`/`tcp_cls`, `dg_*`, `cl_*`, `tc_make_dig`/`_sgn`/`_cls` | `P` (read by `oc_model::vec_begin`), `oc_part_begin`, `oc_digit`, `oc_residue_begin`, `oc_cls` |
| 15 | `oc_frame.inc` | 108 | `tc_auxc`, `oc_v`, `ogg_page`, `tc_used` | `oc_mode`/`oc_wprev`/`oc_wnext`, `oc_page`, `oc_link`, `oc_link_begin`, `oc_spill` |
| 16 | `oc_model.inc` | 103 | 11–15, `oc_floor_tables`, `tcp_build`, `tc_hist_free` | `tc_setup_done`, `tc_models`, `tc_tables`, `tc_nomem`, `oc_model`, `sink_t` |
| 17 | `vb_packet.inc` | 452 | `sink_t`, `vb_*`, `ogc_ilog`, `FATAL_IF_HOT` | `struct io` |
| 18 | `ogg_stream.inc` | 219 | `io`, `ogg_page`, `sink_t` | `source`, `link_walk`, `vb_pack`/`vb_unpack`, `page_writer` |
| | `oggcomp.cpp` | 199 | | `oc_coro`, `main` |

After the split `oggcomp.cpp` reads as the include list above, the six-line
range-coder adapter between 4 and 5, and the tail from `OC_MAGIC` on
(lines 3012–3181 today) unchanged.

Two files are asymmetric and worth naming as such.  `tc_tables.inc` is the
seam to the generated code: its job is to put every identifier `MOD/`
needs in scope and then include it, so it is small and *nothing* in it
should be moved elsewhere.  `oc_model.inc` is the assembly: the two
functions that touch every model at once (`tc_setup_done` allocates the
histories, builds the probability tables and the floor tables; `tc_models`
maps and wires the six families) and the `oc_model` facade that `io` and
`link_walk` are written against.

## 4. Order constraints that bite

These are the dependencies that are not obvious from reading, each of which
would be a compile error (or worse, a silent change) if the order slipped:

1. `X64`, `ALIGN`, `DIM`, `if_e0`, `if_e1`, `__assume`, `g_prog_in` and
   `FILE_API_STD` are consumed by `Lib3/`: `coro3b.inc` picks its
   setjmp on `#ifdef X64` and calls `DIM(pin)`; `coro3_pin.inc` uses
   `if_e0`; `coro_fhp2.inc` adds to `g_prog_in`.  Everything in
   `oc_platform.inc` therefore stays ahead of the `Lib3/` includes.
2. `NOINLINE` is used by `sh_mapping.inc` (the `mapping` constructor).
3. **`MOD/` changes shape with the build.**  With `Const 0` (tuning,
   `USE_NEW 1`) the generated headers call `tc_map()` and `tbl_n()` and
   build `mapping` objects; with `Const 1` (shipping, `USE_NEW 0`) they
   are fixed arrays and the `_p.inc` bodies clamp with `__min`/`__max`.
   So `tbl_n` is unreferenced in a shipping build and `__min`/`__max` in a
   tuning build, and both are live.  Check "unused" against **both**
   forms, as section 5 did.
4. `tc_sq` (line 1853) is used by both `oc_flr` and `oc_digit`.  It has to
   precede both, which is why it moves to `tc_base.inc` rather than
   staying with the residue code where it was written.
5. `tc_setup_done` (1743) calls `oc_floor_tables` (1714), `tcp_build`
   (1300) and `tc_hist_free` (1678) and writes the histories; it belongs
   after all the models, in the assembly file, not with the floor code it
   happens to sit next to.
6. `rc.inc` begins with `#ifndef RC_IO_BASE / #error`.  Today that guard
   is one line below `#define RC_IO_BASE rc_pin_io` and can never fire;
   once the coder is a file of its own it is the real precondition again,
   so the adapter is kept in `oggcomp.cpp` *above* the include.
7. `oc_model::vec_begin()` reads `P.pt`, a static in the residue block, so
   `oc_residue.inc` precedes `oc_model.inc`.
8. `struct io` and `link_walk` take `sink_t &`, the typedef at the end of
   `oc_model.inc`.

## 5. Dead code

Confirmed by `g++ -Wall -Wextra -Wunused-macros`, `clang++` with
`-Wunused-member-function -Wunused-template -Wunneeded-internal-declaration`,
and a reference count over the translation unit including `sh_mapping.inc`,
`Lib3/` and **both** forms of `MOD/`.

Remove:

| what | where | evidence |
|---|---|---|
| `u32 ogg_crc(…)`, `ogg_crc_page`, `ogg_crc_ok`, `ogg_crc_set` | 197–200 | four prototypes, never defined, never called; the working versions are the static members of `ogg_page` |
| `#define DEV_NULL` | 88, 99 | defined on both branches of the `_WIN32` block, referenced nowhere.  Each compiler reports the branch it compiles: line 99 under g++ and clang on Linux, line 88 under mingw |
| `SCALE` in `enum { SCALElog = 15, SCALE = 1 << SCALElog }` | 484 | only `SCALElog` is read |

Simplify (behaviour-preserving):

| what | where | evidence |
|---|---|---|
| `fd->rnk[p] < VB_MAXPOST ? fd->rnk[p] : VB_MAXPOST - 1` | 2143 | `rnk` is `u8` and `VB_MAXPOST` is 256: always true, clang says so; the expression is `fd->rnk[p]` |

Dead in `sh_mapping.inc`, **recommended left alone** (the file is the IDX
runtime, carried verbatim; its own header says "unchanged", and drift from
`IDX/` would be the worse outcome):

- `masking_b`, `pmask2`, `mmask2`: no `MOD/` form references them.  The
  header says as much: "here because the format has it".
- `GCCBUG1`, `GCCBUG2`: "kept just in case you rely on them elsewhere";
  nothing does.
- `mapping::inc(INT&, int, int f_cat)`: the unused-parameter warning; the
  three-argument overload is not obviously used.

Looks dead, is not -- do **not** remove:

| what | why it is live |
|---|---|
| `tbl_n` | only the tuning-form `MOD/*_h.inc` calls it (26–28 times per family); `clang++` flags it unused in a shipping build |
| `__min`, `__max` | only the shipping-form `MOD/*_p.inc` uses them (11–30 times per family); `g++` flags them unused in a tuning build |
| `X64`, `DIM`, `if_e0`, `if_e1`, `__assume`, `ALIGN`, `g_prog_in`, `FILE_API_STD` | read by `Lib3/` |
| `NOINLINE` | read by `sh_mapping.inc` |
| the non-GNU `RC_BSR64` function, the `LOWBYTES < 8` paths in `Rangecoder`, the loop fallback in `ogc_ilog` | portability branches, selected by compiler or by `OGC_NO_ATTRS`; `RC_LOWBYTES`/`RC_FF_TRIM` are the coder's own knobs and become meaningful again once it is `rc.inc` |
| `carry_lost`, the `OGC_EXIT_INTERNAL` site | live under `-DOC_CARRYLESS=1` |
| the padding loop in `main` | live under `-DTC_MEMCOST` |
| `oc_coro::do_process`, `oc_coro::push` | no call by name -- `do_process` is the coroutine entry `Lib3/coro3b.inc` invokes, `push` is passed to `vb_unpack` as a function pointer |
| `source::close` | called from `do_process` |
| `FILE_API_STD` (line 98) | clang reports it as an unused macro on Linux.  It is read by `Lib3/file_api.inc` as `defined(_WIN32) && !defined(FILE_API_STD)`, and with `_WIN32` false the `&&` short-circuits before `defined(FILE_API_STD)` is evaluated, which clang counts as never consulted.  A false positive; g++ does not report it |

Two things about the tools, both of which shape the procedure in
section 7.  First, `-Wunused-macros` in g++ and clang, and
`-Wunused-function` in clang, examine only macros and functions defined
in the **main** file; the same definition in an included file is never
reported.  (Checked: a `#define` in a one-line `.inc` draws no warning
from either compiler; the same line in the `.cpp` does.)  After the split
every macro and every static helper lives in an `.inc`, so the warning
count drops to zero *for that reason* -- it is not evidence that anything
was fixed, and it is why the audit above was done, and any future one must
be done, on the single file.  Second, "unused" depends on which `MOD/`
the compiler sees: the reference counts above were taken over both forms,
and the compiler reports were taken under both forms and all three build
flags.

There is no `#if 0`, no block comment, no commented-out code and no
`TODO` in the file.

## 6. Later, and separately

None of these is part of the split.  Each would be its own change with the
section 7 test, after the move has landed, because each changes shape
rather than location:

- `oc_model` is a forwarding facade: every method is one call to an `oc_*`
  free function that has no other caller.  The free functions could become
  the methods.  Left alone now because it moves code *into* a struct and
  the reassembly diff would no longer read as a move.
- `struct io` (430 lines) parses the three headers and the audio packets in
  one struct; the header half (`lengths` … `hdr`) and the audio half
  (`fl_get` … `audio`) share only the bit reader.  A second file is
  plausible but not clearly better.
- `R` and `P` are anonymous statics that `oc_part_begin` fills and
  `oc_digit` reads; naming their types would let them be documented.
- `Fi`/`Fj`/`Fk` rely on the caller declaring `i`/`j`/`k`; the code is
  written around them and they are fine, but a reader meeting `Fi(` for
  the first time will look for it.  `oc_fatal.inc` is a slightly odd
  home; `oc_platform.inc` would be the other choice.

## 7. Procedure

**Step 0 -- reference streams.**  Before touching anything, build both
forms of the current source and keep the binaries and their output:

    ./mk.sh          && cp oggcomp /tmp/ref-tuning
    ./mk.sh release  && cp oggcomp /tmp/ref-release
    for b in tuning release; do for f in testfiles/*.ogg; do
      /tmp/ref-$b c "$f" "/tmp/ref-$b-$(basename "$f").oc"; done; done
    ./mk.sh mod

**Step 1 -- the deletions, as one commit, on the unsplit file.**  Section
5's "remove" table and the one simplification: eight lines.  Appendix A
has them as a script.  Verify (below), and read the warnings against this
baseline -- what is left is what section 4 item 3 and section 5 say is
live and will stay:

| | g++ | clang++ |
|---|---|---|
| shipping `MOD/` | nothing | `FILE_API_STD` (the false positive), `tbl_n` (unreferenced in this form), `f_cat` in `sh_mapping.inc` |
| tuning `MOD/` | `__min`, `__max` (unreferenced in this form) | the same three macros, `f_cat` |

Before this step the list is longer by `DEV_NULL` and the tautological
compare; after it, nothing else may appear.

**Step 2 -- the move, as a second commit.**  Cut at the markers in
section 2; appendix B is the script that did it for the dry run, locating
each boundary by its first line rather than by number.  Verify (below).
Then prove it was a move: expand the new `#include` lines back into one
file, sort its non-blank lines, sort the non-blank lines of the step-1
file, and diff -- the two must be identical.  (A plain diff shows a moved
block as removed here and added there, and which side gets which is up to
the tool; the sorted comparison is order-blind, which is the point.)  The
warning tables above go quiet after this step for the reason given at the
end of section 5, so their silence here is not a check of anything.

**Verification, after each step:**

    ./t.sh && ./mk.sh check

    #  the one that matters: the same bytes out as before the change
    for b in tuning release; do
      [ $b = release ] && ./mk.sh release || ./mk.sh
      for f in testfiles/*.ogg; do ./oggcomp c "$f" /tmp/new.oc
        cmp -s /tmp/new.oc "/tmp/ref-$b-$(basename "$f").oc" || echo "DIFFERS: $b $f"; done
    done

    #  warnings, both compilers, both MOD/ forms, all three flag sets
    W='-O1 -fwrapv -Wall -Wextra -Wunused-macros -fsyntax-only'
    for form in mod tuning; do
      [ $form = mod ] && ./mk.sh mod || ./mk.sh
      for cc in g++ clang++; do for fl in "" -DTC_MEMCOST -DOC_CARRYLESS=1; do
        $cc $W $fl oggcomp.cpp; done; done
    done
    ./mk.sh mod

    #  both Windows file backends still build
    x86_64-w64-mingw32-g++ -O1 -fwrapv -static                -o /tmp/a.exe oggcomp.cpp
    x86_64-w64-mingw32-g++ -O1 -fwrapv -static -DFILE_API_WIN -o /tmp/b.exe oggcomp.cpp

The `cmp` loop is the difference between "the new program agrees with
itself" and "the new program is the old program".

## 8. Dry run

Both steps were performed mechanically on a scratch copy, in the order
above, and put through the whole of section 7 after each.

After step 1 (the deletions, unsplit):

- 16/16 `.oc` byte-identical to the reference, tuning build; 16/16,
  shipping build.  `./t.sh` and `./mk.sh check` pass.
- Warnings: exactly the step-1 table.  `DEV_NULL` and the tautological
  compare are gone; nothing new.

After step 2 (the move):

- 16/16 and 16/16 again; `./t.sh` and `./mk.sh check` pass; both mingw
  backends build.
- Sorted non-blank lines of the reassembled source: **identical** to the
  step-1 file.  The move added, removed and changed nothing.
- g++: no warnings, either form, any flags.  clang: only the
  `Lib3/`/`sh_mapping.inc` warnings that predate all of this.  Both for
  the reason in section 5: the checks no longer see the `.inc` files, so
  `tbl_n` and the three macros stop being reported without having changed.

The first attempt at the move failed to compile: `tc_sq` had been left
with the residue code and `oc_flr` could not see it.  That is section 4
item 4, and it is the argument for doing the move mechanically and
reading the compiler rather than reading the source and trusting it.

## Appendix A -- the deletions

Run from the tree root on the unsplit `oggcomp.cpp`.  Each edit is
located by content and the script stops if a line it expects is not
there.

```python
import re
L = open('oggcomp.cpp').read().split('\n')
def find(pat):
    for i, l in enumerate(L):
        if re.search(pat, l): return i
    raise SystemExit('not found: ' + pat)
dead = {find(r'^u32 ogg_crc\('), find(r'^u32 ogg_crc_page'),
        find(r'^int ogg_crc_ok'),  find(r'^void ogg_crc_set')}
dead |= {i for i, l in enumerate(L) if l.startswith('#define DEV_NULL')}
L[find(r'^enum \{ SCALElog = 15, SCALE = 1 << SCALElog \};')] = 'enum { SCALElog = 15 };'
i = find(r'tc_make_flr\(\(int\)fno, \(int\)\(fd->rnk\[p\] < VB_MAXPOST')
L[i] = L[i].replace('(int)(fd->rnk[p] < VB_MAXPOST ? fd->rnk[p] : VB_MAXPOST - 1)', '(int)fd->rnk[p]')
open('oggcomp.cpp', 'w').write('\n'.join(l for i, l in enumerate(L) if i not in dead))
```

## Appendix B -- the move

Run from the tree root, after appendix A.  It rewrites `oggcomp.cpp` in
place and writes the eighteen `.inc` files beside it; run it on a copy.
Each cut is located by the first line of the region, so it survives edits
above it but not a renamed marker -- if a marker is missing it stops and
says which.  It moves text and nothing else.

```python
import re
L = open('oggcomp.cpp').read().split('\n')
def find(pat, start=0):
    r = re.compile(pat)
    for i in range(start, len(L)):
        if r.search(L[i]): return i
    raise SystemExit('marker not found: ' + pat)
b = {}
b['prog']    = find(r'^static const char \*ogc_prog')
b['sysinc']  = find(r'^#include <inttypes.h>')
b['lib3']    = find(r'^#include "Lib3/coro3b.inc"')
b['exit']    = find(r'^constexpr int OGC_EXIT_OK')
b['ogg']     = find(r'^constexpr int OGG_HDRMIN')
b['vb']      = find(r'^struct vb_info')
b['rcio']    = find(r'^template <class RC> struct rc_pin_io')
b['rc']      = find(r'^#ifndef RC_IO_BASE')
b['rcend']   = find(r'^#undef RC_BSR64')
b['cm']      = find(r'^constexpr int CM_PBITS')
b['shmap']   = find(r'^#include "sh_mapping.inc"')
b['tcbase']  = find(r'^constexpr i32 RS_DIGIT_MAX')
b['tcp']     = find(r'^constexpr int TCP_NBMAX')
b['tctab']   = find(r'^constexpr long long TC_VOLMAX')
b['tcbit']   = find(r'^static INLINE int tc_bit')
b['hist']    = find(r'^constexpr sz TC_SPANMAX')
b['ocfloor'] = find(r'^struct oc_floor \{')
b['setup']   = find(r'^static void tc_setup_done')
b['vd']      = find(r'^static i32 vd_render')
b['tags']    = find(r'^constexpr sz OC_TAGMAX')
b['tcsq']    = find(r'^static INLINE i32 tc_sq')
b['tcgcd']   = find(r'^static INLINE u32 tc_gcd')
b['flcode']  = find(r'^static u8 tc_used')
b['mode']    = find(r'^static u32 tc_prevW')
b['ochdr']   = find(r'^static u32 oc_hdr')
b['memcost'] = find(r'^#ifdef TC_MEMCOST')
b['vbblk']   = find(r'^constexpr u32 VB_MAXBLK')
b['inlow']   = find(r'^constexpr sz IN_LOW')
b['magic']   = find(r'^constexpr char OC_MAGIC')
b['end']     = len(L)

def rng(a, z): return L[a:z]
files = [
 ('oc_platform.inc', rng(b['sysinc'], b['lib3'])),
 ('oc_fatal.inc',    [L[b['prog']]] + rng(b['exit'], b['ogg'])),
 ('ogg_page.inc',    rng(b['ogg'], b['vb'])),
 ('vb_setup.inc',    rng(b['vb'], b['rcio'])),
 ('rc.inc',          rng(b['rc'], b['rcend'] + 1)),
 ('cm.inc',          rng(b['cm'], b['shmap'])),
 ('tc_base.inc',     rng(b['tcbase'], b['tcp']) + rng(b['tcsq'], b['tcgcd'])),
 ('tc_ptab.inc',     rng(b['tcp'], b['tctab'])),
 ('tc_tables.inc',   rng(b['tctab'], b['tcbit'])),
 ('tc_fam.inc',      rng(b['tcbit'], b['hist'])),
 ('oc_hist.inc',     rng(b['hist'], b['ocfloor'])),
 ('oc_floor.inc',    rng(b['ocfloor'], b['setup']) + rng(b['vd'], b['tags']) + rng(b['flcode'], b['mode'])),
 ('oc_header.inc',   rng(b['tags'], b['tcsq']) + rng(b['ochdr'], b['memcost'])),
 ('oc_residue.inc',  rng(b['tcgcd'], b['flcode'])),
 ('oc_frame.inc',    rng(b['mode'], b['ochdr'])),
 ('oc_model.inc',    rng(b['setup'], b['vd']) + rng(b['memcost'], b['vbblk'])),
 ('vb_packet.inc',   rng(b['vbblk'], b['inlow'])),
 ('ogg_stream.inc',  rng(b['inlow'], b['magic'])),
]
for name, body in files:
    open(name, 'w').write('\n'.join(body).rstrip('\n') + '\n')
adapter = rng(b['rcio'], b['rc'])
main = (['#include "oc_platform.inc"',
         '#include "Lib3/coro3b.inc"', '#include "Lib3/file_api.inc"', '#include "Lib3/coro_fhp2.inc"',
         '#include "oc_fatal.inc"', '#include "ogg_page.inc"', '#include "vb_setup.inc"']
        + adapter +
        ['#include "rc.inc"', '#include "cm.inc"', '#include "sh_mapping.inc"',
         '#include "tc_base.inc"', '#include "tc_ptab.inc"', '#include "tc_tables.inc"', '#include "tc_fam.inc"',
         '#include "oc_hist.inc"', '#include "oc_floor.inc"', '#include "oc_header.inc"',
         '#include "oc_residue.inc"', '#include "oc_frame.inc"', '#include "oc_model.inc"',
         '#include "vb_packet.inc"', '#include "ogg_stream.inc"']
        + rng(b['magic'], b['end']))
open('oggcomp.cpp', 'w').write('\n'.join(main).rstrip('\n') + '\n')
```
