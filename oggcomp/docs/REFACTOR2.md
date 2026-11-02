# Direction as a type, functions as methods, tables as statics

The tree at commit `172832f` is one translation unit of nineteen `.inc`
files, and three things about it are worth changing while it is fresh:

1. **The direction is a runtime flag.**  `tc_enc` (`tc_base.inc:2`) is
   tested 56 times in the coding paths, once per symbol at the hottest
   of them, and two range coders are always live -- `rce` and `rcd`
   (`tc_base.inc:6-7`) -- so every `tc_bit` picks one at runtime.  The
   coroutine that drives a run, `oc_coro` (`oggcomp.cpp:47`), branches on
   the same flag.  `rc.inc` already has the right idea: `Rangecoder` is
   `template <int f_DEC, int CARRYLESS>`.  `oc_coro` becomes a template on
   the same `f_DEC`, and the parameter flows down through the model so
   that every `tc_enc ?` folds at compile time.
2. **Functions that belong to a struct are free functions taking it.**
   `walk_page(vb_ctx &, S &, source &, ...)`, `page_fits(...)`,
   `tcp_head(const tc_pcur &, int)`, `oc_floor_tables(const vb_floor *,
   oc_floor *)`, `tc_hist_free()` over the `dg_*` pointers -- state and
   the code that owns it sit apart, joined by globals.  Each becomes a
   method, declared in the struct's definition.
3. **Memory is allocated at run time that could be there at load time.**
   The model tables are `mmap`ed (`tc_map`, `tc_base.inc:32`), the
   per-stream history is `calloc`ed (`tc_alloc`, `tc_base.inc:24`, nine
   sites in `oc_model.inc:18-36`), the codebook tables are `calloc`ed per
   book (`tc_ptab.inc:102-110`, `:176`, `:245`, `:285`), and `std::set_new_handler`
   is installed for `new`s that run before `main()` ever gets to install it.
   Every one of these has a compile-time bound; those under 256 MB become
   static arrays at that bound.  `IDX/` and the generator are not touched:
   in the shipping build the generated `TC_*_T` structs are already
   fixed-size arrays (`MOD/*_h.inc`, `USE_NEW 0`), and what changes is
   only where the one object of each type lives.

All three while producing **exactly the same program**: every `.oc` byte
for byte what the current binary writes, in both builds.

**Done.**  Step 1 is commit `d4a13f3`, step 2 is `1b1954a`, step 3 is
`12b2a39`.  Each was verified as section 6 says: 33 inputs -- the 30
corpus files, 1 MB of random bytes, the binary itself and a stream cut
mid-page -- code byte-identically under both builds against reference
binaries built from `172832f`, and the clang++ build and both Windows
backends under wine give the same bytes.  `./t.sh` and `./mk.sh check`
pass.  What differs from the plan below, and what was measured:

- The parameter is called `f_DEC` everywhere, as `rc.inc` calls it;
  the plan's `D` collides with `tc_fam`'s counter table of that name.
- Each direction's walk is a member function of `oc_coro<f_DEC>` chosen
  by overload on a tag (`oc_dir<0>`, `oc_dir<1>`) rather than an `if
  constexpr`; a member function of a class template is compiled only
  where it is called, which is the same effect without asking for C++17.
- `tc_bit` stays a free function template, not a method: it has no
  struct argument, and `tc_fam<f_DEC>` reaches the coder through
  `oc_model<f_DEC>::rc` by type.  `tc_bits`, `tc_syms`, `tc_stage` and
  `tc_verbose` stay at file scope for the same reason -- `tc_bit` charges
  them without a pointer to anything -- rather than becoming the model's
  `stats` as section 4.7 said.
- The model's context pointer is `vb`, not `v`: every coder has a local
  `tcx v`.  The single objects are `hist`, `tcp`, `tabs` and `vb_ar`.
- Memory: peak resident memory on `music-stereo-q5.ogg` is 37 MB before
  and after; the address-space reservation grows from 1246 to 1507 MB
  (BSS from 215 to 606 MB in the shipping build, the mapped digit table
  the rest), which `t.sh` and the README now say.
- Time, `music-stereo-q5.ogg`, encode: the shipping build is unchanged
  at 0.25 s; the tuning build goes from 0.44 s to 0.26 s, all of it in
  step 3, so with the direction folded the measuring build codes as
  fast as the shipping one.  Decode is unchanged in both.  The shipping
  build's text shrinks from 242 to 176 KB with both directions in it.

## 1. Ground rules

- **The stream is the test**, as in `REFACTOR.md`.  Reference binaries
  are built from `172832f` before anything moves -- tuning and shipping
  -- and after each step every input in section 6.1 codes to the
  identical `.oc` under both.  `./t.sh` and `./mk.sh check` prove
  self-consistency; the byte comparison proves sameness.
- **One translation unit; `Lib3/`, `IDX/`, `MOD/`, `sh_mapping.inc` are not
  edited.**  Everything below happens in the `oc_*`, `tc_*`, `vb_*`,
  `ogg_*` files and `oggcomp.cpp`.  Where a rule would want a change in
  one of those (section 4.6, section 5.2) the plan says so and works
  around it.
- **Both instantiations are in the binary.**  `oggcomp c` and `oggcomp d`
  are one executable; the template does not remove the decoder from the
  encoder's binary, it removes the decoder from the encoder's *code path*.
  Code size roughly doubles for the model layer (some 200 KB); tables do
  not double, because storage stays outside the template (section 3.3).
- **Storage that is large is direction-independent and single.**  A
  `static` array inside `template <int D> struct` exists once per
  instantiation.  So the rule for what goes into the templated structs is:
  code, and state small enough not to mind twice.  Tables and pools stay
  non-template, in structs of their own, referenced from both.
- **Order: memory, then methods, then the template** (section 6).  Each is
  a commit and each is verified alone.  The memory step defines the structs
  (`oc_hist`, `tc_ptabs`, `vb_arena`) that the method step gives methods
  to, and the method step draws the line the template step then follows.

## 2. The shape at the end

    main()  --picks D at run time--> run<0>() or run<1>()
                                        |
                              CoroFileProc<oc_coro<D>>     (Lib3, unchanged)
                                        |
                                   oc_coro<D>              oggcomp.cpp
                                     |    |
                        og_packer<M>  |    |  og_unpacker<M>   ogg_stream.inc
                        (D == 0)      |    |  (D == 1)
                                      |    |
                                  oc_model<D>  M           oc_model.inc
                                  static Rangecoder<D> rc
                                  tc_fam<D> x 6
                                  oc_frame / oc_header / oc_floor /
                                  oc_residue / oc_raw code as methods
                                        |
                       oc_tables  oc_hist  tc_ptabs  vb_arena   (non-template,
                       (MOD structs) (pool) (pool)   (96 MB)     one of each)

`io_t<S>` (`vb_packet.inc:25`) is already templated on its sink and
stays so; its runtime `enc` becomes `S::ENC`.  `null_sink` gets `ENC = 1`.
`vb_ctx`, `vb_setup`, `ogg_page`, `og_group`, `source`, `out_sink` are
what they are today with the functions of section 4 moved in.

## 3. Memory (step 1)

### 3.1 Inventory

Every allocation in the tree, and what becomes of it.  Sizes are the
shipping build's, measured; "max" is the bound the code already enforces.

| what | where | now | bound | after |
|---|---|---|---|---|
| `TC_dig_T` | `oc_model.inc:59` `TC_MAP` | `tc_map`, 893.3 MB | fixed | **stays mapped** (over 256 MB) |
| `TC_sgn_T` | same | `tc_map`, 61.8 MB | fixed | static object |
| `TC_flr_T` | same | `tc_map`, 36.6 MB | fixed | static object |
| `TC_hdr_T` | same | `tc_map`, 26.7 MB | fixed | static object |
| `TC_cls_T` | same | `tc_map`, 1.5 MB | fixed | static object |
| `TC_aux_T` | same | `tc_map`, 0.2 MB | fixed | static object |
| `dg_hist`, `dg_hist2`, `dg_avg`, `dg_pp`, `dg_ps`, `dg_pn`, `cl_hist` | `oc_model.inc:18-27`, per residue | `calloc` | per residue: 16 + 16 + 32 + 1 + 1 + 0.5 + ~1 MB (`TC_HISTMAX`, `oc_hist.inc:2`); 64 residues | pool, section 3.4 |
| `dg_q1/q2/zr`, `fl_hist`, `fl_yhist`, `fl_who` | `oc_model.inc:31-36`, per stream | `calloc` | 3 x 512 KB + 2 x 256 KB + 2 KB | same pool |
| `tc_ptab::pw, tot, s012, slen, val, brm, ph, pl, psg` | `tc_ptab.inc:103-110`, `:176` | `calloc` per book | `TCP_MEMMAX` = 64 MB over all books (`tcp_mem`, `:87`, `:116`, `:175-179`) | arena, section 3.5 |
| `acc` | `tc_ptab.inc:102`, per build | `calloc`, freed at once | `nnode * nsym * 8`, under `TCP_MEMMAX` (see 3.5) | static scratch |
| `bw` | `tc_ptab.inc:245` | `calloc` per floor class | `8 * 256 * 8` = 16 KB | static scratch |
| `cwt` | `tc_ptab.inc:285` | `calloc` per floor class | `TCP_NODEMAX * 8 * 8` = 4 MB | static scratch |
| `mapping::pmap` | `sh_mapping.inc:30` | `new int[]`, tuning build only | pattern length + 1, 65 at most | unchanged (section 3.6) |
| `vb_arena` | `vb_setup.inc:7` | static, 96 MB | -- | as is |
| `vb_su` | `vb_setup.inc:149` | static, 331 KB | -- | as is |
| `in_win`, `jb`, `pgbuf`, `body` | `ogg_stream.inc:4, :97, :269-270` | static, 32 + 16 MB | -- | as is |
| `cl_same`, `cl_run_buf` | `oc_residue.inc:10-11` | static, 2 x 1 MB | -- | as is |
| `raw_o1` and the rest of the byte model | `oc_raw.inc:17-21` | static, 262 KB | -- | as is |
| `oc_run` | `oggcomp.cpp:104` | static, 200 KB | -- | one per direction, 400 KB |

Address space today: 1020 MB mapped plus about 150 MB of statics, which
is the 1273 MB `t.sh` and the README quote.  After: 893 MB mapped plus
about 535 MB of statics (127 MB of tables, 96 arena, 48 stream buffers,
128 history pool, 132 codebook pool and scratch, 3 the rest).  Resident
memory does not change -- neither `mmap` nor BSS costs a page until it is
touched -- but `VmPeak` grows by some 160 MB, and the `ulimit -v` warning
threshold in `t.sh:115` and the number in the README move with it.

On Windows both forms are committed at load (`VirtualAlloc` with
`MEM_COMMIT` today, the PE loader's BSS after), so the commit charge grows
by the same 160 MB there.  The pools are the knob if that matters:
section 3.4 gives the arithmetic for choosing them smaller.

### 3.2 Rule

A table with a compile-time bound under 256 MB is a static array of that
bound.  Above it, `tc_map` as now.  `tc_alloc` and `tc_mem` go, and with
them `std::set_new_handler(tc_nomem)` (`oggcomp.cpp:132`): the only `new`
left is in `sh_mapping.inc`, in objects constructed before `main()` runs,
which the handler never covered.

### 3.3 The model tables

`tc_tables.inc:45-50` declares six pointers; `TC_MAP` (`oc_model.inc:59`)
maps `sizeof(TC_x_T)` for each and calls `_Init()`.  In the shipping
build (`USE_NEW 0`) the struct is its arrays and `sizeof` is the whole
table; in the tuning build (`USE_NEW 1`) the struct is pointers and
`_Init()` maps each array itself at a size the live `mapping` objects
decide.  Neither header is edited.  What changes is the home of the
object:

    template <class T, bool S = (sizeof(T) <= TC_STATIC_MAX)> struct tc_home;
    template <class T> struct tc_home<T, true>  { static T obj; static T *get() { return &obj; } };
    template <class T> struct tc_home<T, false> { static T *get() { return (T *)tc_map(sizeof(T)); } };
    template <class T> T tc_home<T, true>::obj;

with `TC_STATIC_MAX = (sz)256 << 20`, and `TC_MAP(F)` becomes
`tcm_##F = tc_home<TC_##F##_T>::get(), tcm_##F->TC_##F##_Init()`.  A
static member of a class template is instantiated only where it is used,
so `tc_home<TC_dig_T, true>::obj` is never created.  In the tuning build
every `sizeof(TC_x_T)` is a few hundred bytes of pointers, so every
struct is static there and `_Init()` still maps the arrays -- which is
right: that build's sizes are not known until load, and it is the
measuring build.  Nothing in the shipping build's `tc_tables()` sum
(`oc_model.inc:53`) or the `TC_MEMCAP` assert changes.

`-H` (`tc_huge`, `madvise(MADV_HUGEPAGE)` in `tc_map`) keeps applying to
`dig`, which is where it matters.  The static tables can have it too:
declare the object `alignas(2 << 20)` and `madvise` it in `_Init`'s
caller when `tc_huge` is set.  Optional; measure before keeping it.

### 3.4 The history pool

`tc_setup_done` (`oc_model.inc:1-47`) allocates per residue, deciding the
span first: `span = 0` when `2 * 8 * nchan * span` would exceed
`TC_HISTMAX` (8M entries) or `span > TC_SPANMAX`.  That decision is part
of the model -- a residue with no history codes differently -- so it stays
exactly as written.  The allocation under it becomes a bump pointer into
one static pool, zeroed on take (`calloc` gave zeroed memory; `memset`
on take gives the same), reset in `tc_hist_free` (`oc_hist.inc:18`).

The worst case per residue with a history is 67.5 MB and there can be 64
residues, so the pool cannot hold every stream the current code can.
What it should hold: every real one.  libvorbis writes two or three
residues, each spanning at most half a block (`end - beg` under
`VB_MAXBLK / 2` = 4096), so a stereo stream needs 1.2 MB per residue and
a 5.1 stream 3.6 MB; the 8M-entry ceiling is reached only past 128
channels.  A 128 MB pool holds one residue at the ceiling or a hundred
ordinary ones.  Past it the allocation falls back to `calloc` -- the one
dynamic path this plan leaves -- with the same zeroed result, so the
output is the same either way.  The pool size is a constant next to
`TC_HISTMAX` and is the thing to shrink if Windows commit charge matters.

The struct: `oc_hist` in `oc_hist.inc`, holding what are now the
file-scope `dg_*`, `cl_*`, `fl_*`, `tc_blk`, `tc_nchan` (`oc_hist.inc:3-17`)
plus `u8 pool[]`, `sz used`, and methods `take(n)`, `reset()` (was
`tc_hist_free`).  One instance, non-template.

### 3.5 The codebook tables

`tc_ptab::layout` (`tc_ptab.inc:72-117`) already budgets: it adds levels
while `tcp_mem + (nnode + lim) * per_node <= TCP_MEMMAX` and charges
`tcp_mem` on success.  That budget decides `nlev`, which decides the
model's output, so the arithmetic stays exactly as it is; only where the
bytes come from changes.  A static arena of `TCP_MEMMAX` bytes with a bump
pointer replaces the nine `calloc`s -- `psg` (`:176`) included, whose own
budget test at `:175` stays as it is -- and `drop()` (`:41`) becomes a no-op
per table with one `reset()` on the arena in `tcp_free` (`:431`) -- the
tables are only ever freed all together, at `tcp_build` (`:442`) and at
exit, which is what makes an arena fit.

The three temporaries: `acc` is `nnode * nsym * 8` bytes, and the budget
check charges at least `nsym * 8` per node before `nnode` grows, so
`acc` is under `TCP_MEMMAX` for any layout that succeeds -- a 64 MB
static scratch.  `bw` is `nbr * maxy * 8` with `nbr <= 8` and
`maxy <= TCP_SYMMAX`: 16 KB.  `cwt` is `nnode * nbr * 8` with
`nnode <= TCP_NODEMAX`: 4 MB.  Three static arrays; the `!acc || !pw`
failure branches (`:111`, `:246`, `:286`) become impossible and go.

The struct: `tc_ptabs` in `tc_ptab.inc`, holding `tcp_book`, `tcp_cls`,
`tcp_flr` (`:428-430`), the arena and the scratch, with `build(const
vb_setup &)` (was `tcp_build`, which read `oc_v->cur`) and `reset()` (was
`tcp_free`).  `tc_ptab` keeps `layout/build/build_floor/finish` as methods
and takes the arena by reference.

### 3.6 What stays dynamic, and why

- `TC_dig_T`, 893 MB: over the limit; and it is the table whose sparse
  touching `tc_map`'s comment in `IDX/idx2inc.pl:303-311` is about.
- The tuning build's per-array maps: sized at load by the `mapping`
  objects opt.pl patches; the shipping build is where static sizes exist.
- `sh_mapping.inc`'s `new int[]`: an upstream file, tuning build only, a
  few hundred bytes.  A fixed `int pmap[65]` would do (the longest pattern
  in `IDX/` is 64) but is not worth a divergence from psrc.
- The history pool's overflow, section 3.4.

## 4. Methods (step 2)

### 4.1 The rule for the receiver

A free function that takes a struct becomes a method of that struct.
When it takes several, the receiver is the one whose state it changes;
when it changes none, the one it reads most; when the honest answer is
"the model", the model (section 5) is the receiver and the struct is a
parameter.  What has no struct argument -- `tc_qlog`, `cm_squash`,
`crcrun`, `page_start`, `vd_render`, `tcp_bucket` -- stays a free
function.  Generated code (`tc_make_*` in `MOD/`, taking `tcx &`) is not
touched.

### 4.2 The stream walker: two structs instead of eight functions

`ogg_stream.inc:139-448` is eight functions over `vb_ctx`, `og_group`,
`source`, the sink and `ogg_page`, passing the same five things to one
another.  They are one object's methods -- but the encoder's and the
decoder's halves share nothing but `og_group` and `vb_ctx`, so two:

    template <class M> struct og_packer {
      M &t; source &src; vb_ctx v; og_group g;
      template <class S> void walk_page(S &sink, const ogg_page &p, sz got, int *cont, sz *spill, int *w);
      int  page_fits(const ogg_page &p, sz got);      // dry run against null_sink
      int  consume_junk();
      int  peek_more() const;
      void code_page(ogg_page &p, sz got);
      void pack();                                     // was vb_pack
    };
    template <class M> struct og_unpacker {
      M &t; out_sink o; vb_ctx v; og_group g;
      void emit_junk();
      void unpack(const char *in);                     // was vb_unpack
    };

`source` keeps `open/want/skip/cur/avail/offset/next_of/join_len/join_pkt`
and gains nothing: `consume_junk` and `peek_more` decide what to *code*,
which is the packer's business, and only read the source.  `page_start`
(`:207`) takes bytes, not a struct, and stays free.  `out_sink` (`:273`)
keeps `put`.  `og_group` keeps `start/after`.  The `int *cont, sz *spill,
int *w` triple stays as pointers because `page_fits` hands `walk_page`
copies and `code_page` hands it the group's own -- the comment at `:137`
already says why.

`vb_used`, the arena's bump pointer that `page_fits` saves and restores
(`:185`, `:198`), becomes `vb_arena::used` (section 4.5).

### 4.3 The model's functions

Everything in `oc_frame.inc`, `oc_header.inc`, `oc_floor.inc:74-142`,
`oc_residue.inc:31-233`, `oc_raw.inc:23-47` and `tc_fam.inc:1-10,
230-239` is a free function over the model's globals, called through the
facade `oc_model` (`oc_model.inc:84-114`) whose methods are one-line
forwards.  The forwards go and the functions become the methods:

| free function | becomes |
|---|---|
| `oc_mode`, `oc_wprev`, `oc_wnext`, `oc_page`, `oc_link`, `oc_link_begin`, `oc_rawlen`, `oc_more`, `oc_pgraw`, `oc_crcbad`, `oc_check`, `oc_padnz`, `oc_spill` (`oc_frame.inc`) | `oc_model::mode/wprev/wnext/page/link/link_begin/rawlen/more/pgraw/crcbad/check/padnz/spill` |
| `tc_tagid`, `tc_hdrval`, `oc_hdr` (`oc_header.inc:8-43`) | `oc_model::tagid/hdrval/hdr`; the tag table (`:2-7`) is model state |
| `oc_flr` (`oc_floor.inc:74`) | `oc_model::floor` |
| `oc_floor_tables(const vb_floor *, oc_floor *)` (`oc_floor.inc:7`) | `oc_floor::build(const vb_floor &)` -- the one function here whose receiver is not the model: it fills the `oc_floor` |
| `oc_part_begin`, `oc_digit`, `tc_classify`, `oc_residue_begin`, `oc_cls` (`oc_residue.inc`) | `oc_model::part_begin/digit/classify/residue_begin/cls`; `R` and `P` (`:12-30`) are model state |
| `oc_raw_init`, `oc_raw` (`oc_raw.inc`) | `oc_model::raw_init/raw`; the counters stay file-scope (section 1, storage) |
| `tc_bit` (`tc_fam.inc:1`) | `oc_model::bit` -- see 5.3 for why `tc_fam` calls it without a pointer |
| `tc_auxc`, `tc_aux` (`tc_fam.inc:230-239`) | `oc_model::auxc/aux`; `tc_last/tc_last2` (`:229`) are model state |
| `tc_setup_done` (`oc_model.inc:1`) | `oc_model::setup_done(vb_ctx &)`, which already exists as the caller; `oc_v` (`tc_base.inc:8`) becomes the member `vb_ctx *v` |
| `tc_models` (`oc_model.inc:57`) | `oc_tables::init()` for the mapping and `oc_model::init()` for the wiring (`TC_WIRE`) |
| `tc_tables()` (`oc_model.inc:53`) | `oc_tables::bytes()` |

`vec_begin` (`oc_model.inc:108`) is the one facade method that is not a
forward; it stays.  `null_sink` (`:116-131`) is unchanged but for `ENC`.

### 4.4 Codebook tables

| free function | becomes |
|---|---|
| `tcp_head`, `tcp_len`, `tcp_sign`, `tcp_man`, `tcp_axis` (`tc_ptab.inc:368-427`), all `(const tc_pcur &c, ...)` | `tc_pcur::head/len/sign/man/axis` |
| `tcp_build`, `tcp_free` (`:442`, `:431`) | `tc_ptabs::build(const vb_setup &)`, `tc_ptabs::reset()` (section 3.5) |
| `tcp_pow`, `tcp_p`, `tcp_bucket` | free, no struct argument |

### 4.5 Parser setup

`vb_alloc<T>(n)` (`vb_setup.inc:9`) carves the static arena through two
globals.  `struct vb_arena { alignas(8) u8 mem[VB_ARENA]; sz used; template
<class T> T *take(sz n); void reset(); }`, one static instance;
`vb_ctx::init/link` (`:153-161`) call `reset()` instead of zeroing
`vb_used`.  `vb_book::words/tree/grid`, `vb_floor::sort`,
`vb_setup::check` are methods already.

### 4.6 The rest

- `oc_hist`: `tc_hist_free` becomes `oc_hist::reset()` (section 3.4).
- `ogc_output(const char *, filehandle *)` (`oc_fatal.inc:53`) takes a
  Lib3 struct, which cannot gain a method.  The receiver is the fatal
  layer's own state: `struct ogc_partial_t { const char *path; filehandle
  *fh; void set(path, fh); void kept(); void drop(); }`, with `die()`
  calling `drop()`.
- `oc_coro::pull/push(void *ctx, ...)` (`oggcomp.cpp:50-81`): static
  functions over a `void *` that is the coroutine.  `source` gets a
  `coro3_pin *in` and a `fill()` method that does what `pull` does;
  `out_sink` gets a `coro3_pin *out` and `put` writes it directly.  The
  `source_pull` and `page_writer` function-pointer types (`ogg_stream.inc:3`,
  `:271`) go.  `source` and `out_sink` were already the structs being
  filled and drained; the callbacks only existed because they could not
  see the pin.
- `rc_pin_io<RC>` (`oggcomp.cpp:16`): already a struct with methods.
- `ogg_page`, `cm_*`, `Rangecoder`, `mapping`: already methods or not ours.

### 4.7 What moves into which struct, as state

The file-scope state of section 3.1's inventory and the survey of
globals, by destination.  "Model" means `oc_model<D>`, per direction,
small; "single" means one non-template object.

| state | today | destination |
|---|---|---|
| `tc_prevW`, `tc_prevmode`, `oc_nextW`, `oc_wp`, `pg_prev`, `pg_prevtype`, `pg_prevser`, `oc_seq` | `oc_frame.inc:1-3, 27-29` | model |
| `tc_tagbuf`, `tc_tag`, `tc_ntag`, `tc_lasttag`, `tc_tlast`, `tc_tlast2`, `tc_runpos` | `oc_header.inc:2-7` | model |
| `tc_used`, `oc_fno`, `fl_cur`, `fl_fy` | `oc_floor.inc:70-73` | model |
| `oc_fl[VB_MAXFLOOR]` (5 x 256 B x 64) | `oc_floor.inc:6` | model (80 KB, twice is fine) |
| `R`, `P`, `cl_run` | `oc_residue.inc:9, 12-30` | model |
| `cl_same`, `cl_run_buf` (2 MB) | `oc_residue.inc:10-11` | single, in `oc_hist` |
| `tc_last`, `tc_last2` | `tc_fam.inc:229` | model |
| `fam_dig .. fam_hdr` | `tc_fam.inc:228` | model, as `tc_fam<D>` |
| `tc_bits`, `tc_syms`, `tc_stage`, `tc_verbose` | `tc_tables.inc:65-67` | model (`stats`), read by `main` after the run |
| `tcm_dig .. tcm_hdr` | `tc_tables.inc:45-50` | single, `oc_tables` |
| `dg_*`, `cl_hist`, `cl_np`, `cl_last`, `cl_last2`, `fl_hist`, `fl_yhist`, `fl_who`, `tc_blk`, `tc_nchan` | `oc_hist.inc:3-17` | single, `oc_hist` |
| `tcp_book`, `tcp_cls`, `tcp_flr`, `tcp_mem` | `tc_ptab.inc:7, 428-430` | single, `tc_ptabs` |
| `raw_o0`, `raw_o1`, `raw_w`, `raw_wc`, `raw_mx`, `raw_prev` | `oc_raw.inc:17-22` | single (`raw_prev` and `raw_mx` are small but belong with their tables) |
| `vb_arena`, `vb_used` | `vb_setup.inc:7-8` | single, `vb_arena` |
| `vb_su` | `vb_setup.inc:149` | single, as is |
| `vb_ys`, `vb_cs` | `vb_packet.inc:5-6` | single, as is (parser scratch) |
| `in_win`, `jb`, `pgbuf`, `body` | `ogg_stream.inc` | single, as is |
| `tc_enc`, `rce`, `rcd`, `oc_v`, `tc_mem`, `tc_huge` | `tc_base.inc:2-31` | `tc_enc` goes (section 5); `rce/rcd` become `oc_model<D>::rc`; `oc_v` becomes `oc_model::v`; `tc_mem` goes; `tc_huge` stays |
| `oc_in`, `rc_over` | `oggcomp.cpp:13-14` | as is |

The model's per-direction state adds up to about 100 KB; twice that in
the binary is nothing.  Everything over a megabyte is single.

## 5. The direction as a template parameter (step 3)

### 5.1 Where `tc_enc` is read

56 sites: `tc_fam.inc` 14, `oc_frame.inc` 24, `oc_floor.inc` 4,
`oc_residue.inc` 4, `oc_header.inc` 3, `oc_raw.inc` 1, `oggcomp.cpp` 5,
and its declaration.  They are of four kinds:

1. **The value to code:** `tc_enc ? (i64)x : 0` -- 27 sites: 18 in
   `oc_frame.inc`, three in `tc_fam.inc` (`:181`, `:213`, and the message
   at `:196`, `tc_enc ? "this stream holds" : "coded stream"`), two each
   in `oc_floor.inc`, `oc_residue.inc`, `oc_header.inc`.  With `D` a
   template parameter this is `D ? 0 : (i64)x`, and the compiler drops
   the dead arm.  Written once as a helper, `enc_val(x)`, in `oc_model<D>`.
2. **The bit to code:** `tc_enc && cond` -- 10 sites: `tc_fam.inc:164,
   167, 172, 177, 191, 203, 219`, `oc_frame.inc:150` (`oc_check`),
   `oc_residue.inc:131` (the sign), `oc_raw.inc:35`.  Same treatment,
   `enc_bit(cond)`.
3. **Direction-only code:** `if(tc_enc)` / `if(!tc_enc)` around a block --
   12 sites: the encoder's counting in `tc_fam.inc:182` and its early
   refusal at `:207`, the decoder's range checks in `oc_frame.inc:41, 50,
   56, 62, 85`, `oc_header.inc:40`, `oc_floor.inc:93, 134`,
   `oc_residue.inc:118`, the verbose accounting in `tc_fam.inc:3`.
   `if constexpr (D == 0)` / `(D == 1)`.
4. **Which coder:** `tc_fam.inc:5-8` picks `rce` or `rcd`.  Gone: there is
   one coder, `oc_model<D>::rc`, of type `Rangecoder<D, OC_CARRYLESS>`.

That is 50; the declaration and `main()`'s five uses are the rest.
`main()`'s stay runtime: choosing the mode is the one place the direction
is data.

### 5.2 The chain

    template <int D> struct oc_coro : Coroutine {           oggcomp.cpp
      oc_model<D> t;
      void do_process() { if constexpr (D == 0) og_packer<oc_model<D>>{...}.pack();
                          else og_unpacker<oc_model<D>>{...}.unpack(in); yield(this, 0); }
    };
    template <int D> static int run(const char *in, filehandle &f, filehandle &g) {
      static CoroFileProc<oc_coro<D>> co;                   // one per direction, 200 KB each
      ...header read or written as now...
      co.processfile(f, g); ...report from co.t.stats...
    }
    int main(...) { ... return mode[0] == 'c' ? run<0>(...) : run<1>(...); }

`CoroFileProc<Model>` (`Lib3/coro_fhp2.inc`) and `Coroutine::coro_call<T>`
(`Lib3/coro3b.inc:104-109`) are templates already and take a template
argument as well as they take a class; `p_do_process` is a
pointer-to-member cast that works the same for an instantiation.  Nothing
in `Lib3/` changes.

`oc_model<D>` carries `static constexpr int ENC = (D == 0)` and
`static Rangecoder<D, OC_CARRYLESS> rc`.  `io_t<S>` (`vb_packet.inc:25`)
replaces its `int enc` member and the constructor's `e` with
`static constexpr int enc = S::ENC`; the 29 tests of `enc` in it fold, and
the decoder's `memset` in the constructor becomes `if constexpr`.
`null_sink::ENC = 1`: it is only ever the encoder's.  `og_packer<M>` and
`og_unpacker<M>` take the model type; `page_fits` and `walk_page<null_sink>`
exist only in the packer, so the dry run is not compiled into the decoder.

### 5.3 `tc_fam` and the coder

`tc_fam::bit` (`tc_fam.inc:105`) and `bitm` (`:140`) are the hot path and
call `tc_bit`, which today reads `tc_enc` and picks a global coder.
`tc_fam` becomes `template <int D> struct tc_fam` and calls
`oc_model<D>::rc.rc_Process(...)` -- a static member, reached by type, no
pointer through the object.  That is the same access pattern as today's
global `rce`, minus the branch.  `tc_fam<D>` needs `oc_model<D>` declared
first: a forward declaration of the template and of its static member
above `tc_fam.inc`, with the definition after, is enough; the alternative
of a `rc` reference inside each family is one more load per bit.

### 5.4 Verbose accounting

`tc_bit:3` charges `tc_bits[tc_stage]` when `tc_verbose && tc_enc`; the
`tc_syms` counts are the same.  In `oc_model<0>` this is `if(stats.verbose)`;
in `oc_model<1>` the statement is not compiled.  `main` prints from
`co.t.stats` and, as now, only for the encoder.

### 5.5 What does not change

The `.oc` stream: no symbol, context or rate is touched.  `vb_refuse` and
the dry-run exception.  `rc_pin_io`, `rc_over`, the end-of-stream CRC.
`FATAL`'s messages.  `mk.sh`, `t.sh`, `testfiles/`.

## 6. Order, and how each step is checked

### 6.1 The reference set

Before anything, from `172832f`:

    ./mk.sh          && cp oggcomp /tmp/ref-tuning
    ./mk.sh release  && cp oggcomp /tmp/ref-release
    for f in testfiles/*.ogg testfiles/*.bin $EXTRA; do
      /tmp/ref-tuning  c "$f" "/tmp/ref-tuning-$(basename "$f").oc"
      /tmp/ref-release c "$f" "/tmp/ref-release-$(basename "$f").oc"
    done

where `$EXTRA` is a 1 MB file of random bytes, the `oggcomp` binary
itself, and a 512 KB stream cut mid-page (the uploaded
`a11d6a46-partial.ogg` is the shape: 123 whole pages and one cut short,
so the last whole page goes raw because its packet has no continuation).
The corpus alone does not reach the history pool's fallback or a
multi-residue setup with a large span; `multi6-48k-q4.ogg` (six channels)
and `chirp-stereo-q10.ogg` (`-q 10`, the widest books, the deepest
`tc_ptab` layouts) are the closest it has, and both are in.

After each step, for both builds, every file: `cmp` against its
reference.  Then `./t.sh && ./mk.sh check`, a clang++ build with
`-Wall -Wextra -Wunused-macros -Wunused-function` clean outside `Lib3/`
and `sh_mapping.inc`, both mingw backends under wine on a few inputs,
and the encode time of `music-stereo-q5.ogg` within 2% of the reference.

### 6.2 Step 1 -- memory (section 3)

One commit.  `oc_hist`, `tc_ptabs`, `vb_arena`, `tc_home`; `tc_alloc`,
`tc_mem`, `tc_nomem`, `set_new_handler` removed; `t.sh:115` and the README's
1273 MB revised to the new `VmPeak`.  Check, beyond 6.1: `VmPeak` and
`VmHWM` from `/proc/self/status` at exit, printed under `-v` for the
duration of this work, before and after -- resident within noise, peak as
section 3.1 predicts.  Windows: the two exes still start on a machine
whose commit limit is what it is.

### 6.3 Step 2 -- methods (section 4)

One commit, possibly two (the walker's two structs are the largest
single move and can go first).  Mechanical: bodies do not change, only
their homes and the spelling of what they touch (`P.` becomes `P.` still,
inside the model; `dg_hist[i]` becomes `hist.dg_hist[i]`).  The check of
`REFACTOR.md` section 7 -- reassemble and compare sorted lines -- does not
apply, since lines change; the byte-identical stream is the check.

### 6.4 Step 3 -- the template (section 5)

One commit.  `tc_enc` deleted first, so that every site the compiler
finds is a site that has to be rewritten, and none is missed.  The
56 sites of 5.1 are the checklist.  Check, beyond 6.1: `nm --size-sort`
on the binary shows `oc_model<0>` and `oc_model<1>` and no
`tc_bit`; the encode time is the number to watch, since the point of the
step is a branch per bit gone and a coder no longer chosen at run time.

## 7. Decisions to take before starting

1. **Pool sizes** (3.4, 3.5): 128 MB history, 64 + 64 + 4 MB codebook.
   Both are BSS, cost nothing untouched on Linux, and are commit charge on
   Windows.  Halving both leaves every corpus file and every ordinary
   stream inside them.
2. **The history fallback** (3.4): keep `calloc` past the pool, or make the
   pool the bound.  Keeping it costs one `if`; removing it changes what a
   256-channel stream codes to, which is a format change and not this
   plan's.
3. **Huge pages on the static tables** (3.3): optional; only if `-H` is
   measured to matter on `sgn` and `flr`.
4. **Two `CoroFileProc` objects** (5.2) or one buffer and placement-new:
   400 KB against 200 KB of BSS.  Two is simpler and is the plan.
