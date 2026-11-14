# flacdump refactoring plan

**Status:** all three phases are done. Section 0 shows the resulting source
layout; sections 1-5 are the plan as it was written, with notes where the
implementation differs.

## 0. Result: source layout

`flacdump.cpp` (232 lines) holds the prologue (includes, integer types,
platform macros, `die`, `Min`/`Max`, bit and byte-order helpers), the ordered
include list and `main`. All other code is in 42 `.inc` files next to it, one
translation unit. Except for the two mutually recursive pairs (`find_cuts`,
`code_region`), every function and type is defined before its first use, so
the include order below is also the dependency order. No function is longer
than 100 lines (`find_cuts` is exactly 100); classes with member functions
each have their own file (marked *class*).

| file | lines | contents |
|------|------:|----------|
| `buf.inc`, `vec.inc` | 29, 23 | *class* `Buf`, *class* `Vec<T>` (the only heap users) |
| `file_crc.inc` | 46 | `read_file`, CRC-8 / CRC-16 |
| `md5.inc` | 91 | *class* `MD5` |
| `bit_reader.inc`, `fix_out.inc`, `bit_writer.inc` | 55, 24, 51 | *class* `BitReader`, *class* `FixOut`, *class* `BitWriter` |
| `flac_defs.inc` | 166 | FLAC structures, work buffers, header code tables, fixed/LPC residuals |
| `lpc_res_fast.inc` | 43 | *class* `LpcResInit<O>`: order-specialised LPC residual kernels |
| `flac_frame.inc` | 303 | frame parser/decoder and encoder |
| `wav_format.inc` | 40 | WAV header, channel mask, shared sample buffer |
| `wav_writer.inc`, `wav_reader.inc` | 48, 76 | *class* `WavWriter`, *class* `WavReader` |
| `ac.inc` | 100 | *class* `AC`: arithmetic coder, bit models, cost categories |
| `uint_model.inc` | 45 | integer coding (`UModel`, `code_uint`, `code_sint`) |
| `byte_cm.inc` | 107 | *class* `ByteCM`: CM byte model and its table pool |
| `code_bytes.inc` | 64 | byte strings, CM-coded or stored |
| `embedded.inc` | 80 | embedded files: types, JPEG/PNG lengths |
| `base64.inc` | 241 | base64 layout, generation, strict parsing, detection |
| `tag_spans.inc` | 149 | ID3v2 / APEv2 parsing (exact value spans) |
| `regions.inc` | 231 | cuts in byte regions (`find_cuts`, `code_cut`, `code_region`) |
| `det_math.inc` | 103 | deterministic `cos`, glibc `cosf` emulation, `log2` |
| `estimators.inc` | 98 | fixed order guess, rice estimators, expected bits, k0 |
| `res_stats.inc` | 148 | *class* `ResStats` |
| `windows.inc` | 91 | apodization windows |
| `win_cache.inc` | 40 | *class* `WinCache` |
| `lpc_analysis.inc` | 206 | libFLAC autocorrelation variants, Levinson-Durbin, best order, quantization |
| `ff_lpc.inc` | 107 | ffmpeg LPC analysis |
| `lpc_predictor.inc` | 261 | *class* `LpcPredictor` |
| `meta_model.inc` | 195 | bit models, frame/channel state, learning thresholds, `Ctx`, `reset_run_state` |
| `model_residual.inc` | 135 | partition order, rice parameters |
| `lpc_order.inc` | 188 | LPC order prediction and learning |
| `model_lpc.inc` | 201 | LPC parameters in three prediction stages |
| `encoder_sim.inc` | 201 | libFLAC subframe/stereo simulation, ffmpeg stereo estimate |
| `model_frame.inc` | 207 | FRAME segments: header, channel mode, subframes |
| `container.inc` | 32 | segments, metadata blocks, layout, `.meta` magic |
| `frame_stats.inc` | 61 | *class* `FrameStats` |
| `sink.inc` | 21 | *class* `Sink` |
| `vorbis.inc` | 172 | VORBIS_COMMENT: vendors, validation, encoder hint, structured coding |
| `seektable.inc` | 148 | SEEKTABLE coding |
| `metadata.inc` | 209 | stream parameters, layout (part A), derived fields (part C) |
| `decompress.inc` | 104 | `d` mode |
| `compress.inc` | 296 | `c` mode: options, pass 1 (scan), pass 2 (model), verification |

Commits: `a71016a` (phase 1, binary-identical), `b41ad4e` (phase 2 with the
reordering of phase 3 item 4), `eef75e9` (phase 3 items 1-3 and 6), and the
function splits of phase 3 item 5. Every commit after phase 1 produces the
same `.meta`, `.wav` and embedded files on the test corpus as before the
refactoring, and every file round-trips; `-x` and `-vv` output is unchanged.

Line numbers refer to `flacdump.cpp` at commit `08dc76d` (5186 lines, one
translation unit), i.e. before phase 1.

## 1. Goal and approach

Move into its own `.inc` file:

* every class/struct that has member functions, together with its methods and
  the data and helpers used only by it;
* every function longer than 100 lines (from signature to closing brace).

The program stays a **single translation unit**. Each block is cut out
unchanged and replaced by an `#include "<name>.inc"` line **at the same
place**, so the order of definitions does not change and no declarations,
`static` qualifiers or names need to change.

The whole phase 1 table (section 3) was dry-run this way: all 23 blocks
extracted, and the stripped binaries were byte-identical to the original with
both g++ and clang++ (`-O2`, linked with `-Wl,--build-id=none`). The code never uses `__LINE__`/`__FILE__`, so moving
lines cannot change the generated code. That gives the refactoring an exact
check: **same binary before and after**.

Why not headers and several `.cpp` files: that would mean removing `static`
from about 40 functions and globals, writing declarations for them, and losing
cross-function inlining of the hot loops (residuals, autocorrelation, models)
unless LTO is used. The `.inc` approach costs nothing and can still be turned
into separate translation units later.

## 2. Review summary

### 2.1 Structure

| section (banner)                            | lines      | notes |
|---------------------------------------------|------------|-------|
| prologue: includes, types, `die`, `Min`/`Max`, bit helpers | 1-76 | stays in `flacdump.cpp` |
| growable containers `Buf`, `Vec`            | 78-124     | the only heap users (`xrealloc`) |
| file helpers, CRC-8/16                      | 127-185    | |
| MD5                                         | 187-275    | |
| bit reader / `FixOut` / bit writer          | 278-401    | |
| FLAC structures, work buffers, residuals    | 404-623    | static work buffers 8.5 MiB |
| frame parser / decoder, frame encoder       | 626-922    | |
| arithmetic coder, `UModel`                  | 925-1064   | |
| CM byte model, `code_pieces`                | 1067-1232  | |
| embedded files, base64, ID3/APE spans, cuts | 1235-1917  | biggest "misc" area (680 lines) |
| estimators (fixed guess, rice, k0)          | 1920-2162  | incl. `ResStats` |
| libFLAC/ffmpeg LPC analysis replica         | 2165-2967  | incl. `WinCache`, `LpcPredictor` |
| meta model: state, residual/LPC modelling, order learning | 2970-3599 | |
| libFLAC subframe/stereo simulation, `model_frame` | 3602-3961 | |
| container structs, `FrameStats`             | 3964-4049  | |
| WAV writer/reader, `Sink`                   | 4052-4230  | |
| metadata coding (layout, VORBIS_COMMENT, seek table, derived fields) | 4233-4751 | |
| compression / decompression / `main`        | 4754-5186  | |

### 2.2 Classes with member functions (all are moved)

`Buf`, `Vec<T>`, `MD5`, `BitReader`, `FixOut`, `BitWriter`, `LpcResInit<O>`,
`AC`, `ByteCM`, `ResStats`, `WinCache`, `LpcPredictor`, `FrameStats`,
`WavWriter`, `WavReader`, `Sink`.

Plain data structs without methods (`SubInfo`, `FrameInfo`, `StreamParams`,
`BM`, `UModel`, `Piece`, `ExtFiles`, `B64Fmt`, `Cut`, `Span`, `SpanList`,
`ExtModels`, `CandDef`, `ChState`, `Models`, `FrameState`, `Ctx`, `SimOut`,
`Segment`, `MBlock`, `FlacLayout`, `SeekModels`, `Options`) stay where they
are, unless they belong to a moved class (`BM` -> `ac.inc`, `CandDef` ->
`lpc_predictor.inc`, `ByteCMTables` -> `byte_cm.inc`).

### 2.3 Functions over 100 lines (all are moved)

| function         | lines     | size |
|------------------|-----------|-----:|
| `do_compress`    | 4845-5024 | 180 |
| `model_frame`    | 3789-3961 | 173 |
| `model_lpc`      | 3438-3599 | 162 |
| `code_seektable` | 4529-4646 | 118 |
| `code_vorbis`    | 4276-4385 | 110 |
| `model_residual` | 3135-3244 | 110 |
| `sim_subframe`   | 3628-3734 | 107 |

Close to the limit, so they stay in place for now: `find_cuts` (exactly 100), `parse_frame`
(85), `code_layout` (84), `decompress_core` (78). Rule for the future: a
function that grows beyond 100 lines gets its own `.inc`.

### 2.4 Findings (not structural; all fixed in phase 3)

1. **State carried from one run to the next.** `c` runs the restore in the same
   process to verify its output, so every static object must start each run
   in the same state as in a fresh process. A missed reset was found and
   fixed in `08dc76d`: the k0 hint read `LpcPredictor::lerr` left over from
   another candidate. Resets are currently spread out: `models_init` and
   `g_lpred.init()` in `do_compress`/`decompress_core`; `g_ext.count` and
   `g_extm` in `code_layout`; `g_ln2sq` per frame in `model_frame`;
   `g_cutdepth`/`g_cmbusy` only stay correct because every path balances them.
   One `reset_run_state()` called at the start of both directions would make
   "verify = fresh process" true by construction.
2. **Global flag toggled to test a hypothesis:** `lpc_learn` flips `g_ln2sq`,
   calls `lpc_best_order`, and flips it back (3397-3399). The error scale should be a
   parameter of `lpc_expected_bps`/`lpc_best_order`.
3. **Static locals hide memory:** `WinCache::init` `s_win` (5.5 MiB),
   `sim_stereo` `sig` (2 MiB), `LpcPredictor::init` `s_wd`/`s_ffw`,
   `LpcPredictor::eval` `tmp`, `ResStats::alloc` `s_asum`/`s_zsum`. Moving them
   to file scope at the top of their `.inc` keeps the memory budget visible in one place.
   Now that `alloc_work`, `ResStats::alloc` and `WinCache::init` only point
   into static arrays, the pointer layer (`g_sig[c] = s_sig[c]` ...) could be
   dropped.
4. **Forward declarations caused only by file order:** `struct WavReader` +
   `wav_read` (3123-3124; `model_frame` needs the PCM source), `vorbis_ok`,
   `vendor_hint`, `decompress_core`, `det_log2`. They go away if the
   WAV section is placed before the meta model, and the helpers before their users.
   `find_cuts`/`code_region` are mutually recursive and keep theirs.
5. **Duplicated code in `model_lpc`:** stage 1 and stage 2 end with the
   same "accept candidate" sequence (set shift/coefficients, `mru_hit`, hit
   counters, `sim_on`, `prank`, k0, `lpc_learn`); this should be one helper. The three k0
   computations were already merged into `LpcPredictor::k0_hint`.
6. **Debug output inside model code:** the `g_verbose >= 3` candidate dump in
   `model_lpc` (~20 lines) belongs in its own function.
7. **Unnamed learning constants** (`nlearn >= 64 && (nlearn & 15)`,
   `ffh > 8`, vote thresholds 4/8, the `exscore` margin). They should get names; naming them does
   not change the format, but changing their values does.
8. **`do_compress` mixes concerns:** command-line I/O and messages, pass 1
   (segmentation, re-encode check, WAV), pass 2 (modelling), `-x` side dump,
   verification. Each is a natural function of its own.

### 2.5 Static memory (BSS 126,650,504 bytes = 120.8 MiB after phase 3, limit 128 MB)

(Names as after phase 3; the total is unchanged.)

| object | MiB | | object | MiB |
|---|---:|---|---|---:|
| `g_b64narrow` | 23.3 | | `g_sig` | 4.0 |
| `g_b64scan`, `g_b64dec_v`, `g_b64dec_r` | 3 × 16 | | `s_pcm`, `g_wavbuf`, `g_stsig` | 3 × 2 |
| `g_blockbuf` | 16 | | eight 0.5 MiB work arrays | 4.0 |
| `g_framebuf` | 8 | | models, `g_x32`, rest | 0.6 |
| `g_lpred` (incl. windows 5.5) | 6.8 | | | |
| `g_cmpool` | 4.5 | | | |

Pages are only touched as needed (peak RSS of `c` on an 86 MB FLAC: 88 MB,
mostly the input file). Headroom to the 128 MB limit is ~7 MB; new large
buffers must reuse existing ones.

## 3. Phase 1: move classes and large functions (done)

The files are placed next to `flacdump.cpp` (no subdirectory); file names are
the class/function name in snake case. Each range includes the comment directly above the block. The blocks
are cut **bottom-up** (highest line first) so the earlier ranges stay valid.

| # | file | lines | size | contents | uses from earlier code |
|---|------|-------|-----:|----------|------------------------|
| 1 | `buf.inc` | 80-104 | 25 | `Buf` | `xrealloc` |
| 2 | `vec.inc` | 106-124 | 19 | `Vec<T>` | `xrealloc` |
| 3 | `md5.inc` | 189-275 | 87 | `MD5` | `rd_le32`, `wr_le32` |
| 4 | `bit_reader.inc` | 280-330 | 51 | `BitReader` | `clz64` |
| 5 | `fix_out.inc` | 332-351 | 20 | `FixOut` | - |
| 6 | `bit_writer.inc` | 355-401 | 47 | `BitWriter` | `FixOut` |
| 7 | `lpc_res_fast.inc` | 556-598 | 43 | `lpc_res_t`, `LpcResFn`, `g_lpcres`, `LpcResInit<O>`, `lpc_residual_fast`, `to32`, `g_x32`, `init_fast` | `lpc_residual`, `MAXBS` |
| 8 | `ac.inc` | 927-1022 | 96 | `BM`, `g_rate`, `init_rates`, `bm_init`, `model_init`, cost categories + `k_catname`, `AC` | `g_verbose`, `Buf` |
| 9 | `byte_cm.inc` | 1069-1171 | 103 | `g_stretch`/`g_squash`, `init_squash`, `squash`, `ByteCMTables` pool, `ByteCM` | `AC` |
| 10 | `res_stats.inc` | 2020-2162 | 143 | `ResStats` | `rice_est_old/new/ff`, `lf_bits` |
| 11 | `win_cache.inc` | 2343-2382 | 40 | `WinCache` | `win_tukey`, `win_partial_tukey`, `win_punchout_tukey`, `W_*` |
| 12 | `lpc_predictor.inc` | 2707-2967 | 261 | `CandDef`, `MAXCAND`, `LpcPredictor` | `WinCache`, `autoc_compute`, `levinson_all`, `lpc_quantize(_at)`, `ff_*`, `k0_from_bps`, `lpc_expected_bps` |
| 13 | `model_residual.inc` | 3135-3244 | 110 | `model_residual` | `ResStats`, `Models`, `FrameState`, `ChState`, `Ctx` |
| 14 | `model_lpc.inc` | 3438-3599 | 162 | `model_lpc` | `LpcPredictor`, `order_guess`, `lpc_learn`, `mru_hit`, `code_sint`, `SimOut` |
| 15 | `sim_subframe.inc` | 3628-3734 | 107 | `sim_subframe` | `fixed_residual`, `lpc_residual_fast`, `lpc_best_order`, `lpc_quantize`, `ResStats`, `LpcPredictor` |
| 16 | `model_frame.inc` | 3788-3961 | 174 | `model_frame` | `model_residual`, `model_lpc`, `sim_*`, `ff_stereo_guess`, `fixed_guess`, `wav_read` (forward decl.), frame helpers |
| 17 | `frame_stats.inc` | 3993-4049 | 57 | `FrameStats` | `Vec`, `MD5` |
| 18 | `wav_writer.inc` | 4091-4134 | 44 | `WavWriter` | `wav_header`, `g_wavbuf` |
| 19 | `wav_reader.inc` | 4136-4207 | 72 | `WavReader` | `rd_le16/32`, `g_wavbuf` |
| 20 | `sink.inc` | 4214-4230 | 17 | `Sink` | - |
| 21 | `code_vorbis.inc` | 4275-4385 | 111 | `code_vorbis` | `ByteCM`, `k_prime_text`, base64 helpers, `code_region`, `k_vendors`, `vorbis_ok` (forward decl.) |
| 22 | `code_seektable.inc` | 4529-4646 | 118 | `code_seektable` | `SeekModels`, `frame_containing`, `FrameStats`, `Sink` |
| 23 | `do_compress.inc` | 4845-5024 | 180 | `do_compress` | nearly everything; `decompress_core` (forward decl.) |

What stays in `flacdump.cpp` when they share a helper: `default_mask`,
`wav_header`, `g_wavbuf`, `wav_read` (shared by both WAV classes and
`model_frame`), `k_prime_text` (used by `code_pieces` and `code_vorbis`),
`FRAMEMAX`/`g_framebuf` (used by the frame encoder's callers). The section
banners (`// ---- // Title`) stay in `flacdump.cpp` above the `#include`
lines, so the main file still reads as an outline.

Result: 23 files with 2087 lines in total; `flacdump.cpp` went from 5186 to 3122 lines.

### Format of a `.inc` file

```cpp
// flacdump: LpcPredictor - candidate LPC analyses (libFLAC/ffmpeg) in MRU order
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct CandDef {
...
```

No include guards, no `#include`s inside `.inc` files, nothing defined twice.

### Steps

1. Run the move with a small script (scratch, not committed) that has a table
   of `(file, first line, last line, expected first line text, expected last line text)`;
   it checks both texts, writes the `.inc`
   with its header, and replaces the range by the `#include` line, starting
   from the bottom. The checks on the first and last line text catch any off-by-one.
2. `Makefile`: `flacdump: flacdump.cpp $(wildcard *.inc)` so that edits
   in `.inc` files trigger a rebuild. The documented one-line build
   (`g++ ... flacdump.cpp`) keeps working because `#include "<name>.inc"` is
   resolved relative to `flacdump.cpp`.
3. README: mention the source layout in the Build section.
4. One commit for the whole move (purely mechanical, verified as below).
   `git blame -C -C` and `git log -L` follow lines into the new files.

### Verification

1. Build the old (`08dc76d`) and new trees with
   `g++ -std=c++20 -fno-exceptions -fno-rtti -ffp-contract=off -Wall -O2 -Wl,--build-id=none`,
   strip both: the binaries must be **byte-identical** (repeat with clang++).
2. `-Wall` stays clean.
3. `tests/roundtrip.sh` on a few files (libFLAC, ffmpeg, a file with tags and a
   picture), and `c` with the built-in verify.

Outcome: stripped binaries byte-identical to `0533242` (the commit before the
move) with g++ and clang++, no warnings, round trips OK.

## 4. Phase 2: group the rest of `flacdump.cpp` by topic (done)

Done in one step with the reordering of phase 3 item 4, so the check was
identical output on the corpus rather than an identical binary. The final file
names differ from the table below: deterministic `cos`/`cosf`/`log2` went to
`det_math.inc` (ahead of all their users); `order_learn.inc` became
`lpc_order.inc`, `stereo_sim.inc` became `encoder_sim.inc` (now also with
`sim_subframe`), `wav_common.inc` became `wav_format.inc` (`wav_read` was
removed: `WavReader` is complete before the meta model), `compress_util.inc`
became `compress.inc` (now also with `do_compress`); `vorbis.inc` and
`seektable.inc` were split off `metadata.inc`. The byte-order helpers stay
in the prologue of `flacdump.cpp`.

After phase 1, `flacdump.cpp` is still ~3100 lines of small functions. The
same in-place technique (also binary-identical) could leave it with only the
prologue, the ordered `#include` list and `main`:

Files go next to `flacdump.cpp` as in phase 1. Line numbers are those before
phase 1.

| file | lines | contents |
|------|---------------|----------|
| `file_crc.inc` | 127-185 | `read_file`, CRC tables |
| `flac_defs.inc` | 404-555, 600-623 | FLAC structs, work buffers, canonical codes, fixed/LPC residual, `subframe_signal` |
| `flac_frame.inc` | 626-922 | `decode_residual`, `parse_header`, `parse_frame`, `write_residual`, `encode_frame` |
| `uint_model.inc` | 1024-1064 | `UModel`, `code_uint`, `code_sint` |
| `code_bytes.inc` | 1173-1232 | `k_prime_text`, `Piece`, `code_pieces`, `code_bytes` |
| `embedded.inc` | 1235-1311 | `ExtFiles`, `sniff_ext`, `jpeg_len`, `png_len` |
| `base64.inc` | 1313-1549 | `B64Fmt`, `b64f_*`, `Cut`, `b64_worth`, `b64_try*`, `find_b64_run` |
| `tag_spans.inc` | 1550-1694 | ID3v2 / APEv2 span parsing |
| `regions.inc` | 1696-1917 | `find_cuts`, `code_cut`, `code_region` |
| `estimators.inc` | 1920-2018 | fixed order guess, rice estimators, k0, error scale |
| `windows.inc` | 2165-2341 | deterministic `cos`/`cosf`, apodization windows |
| `lpc_analysis.inc` | 2384-2601 | autocorrelation variants, Levinson, best order, quantization |
| `ff_lpc.inc` | 2603-2705 | ffmpeg welch/autocorr/Levinson/quantizer/order estimate |
| `meta_model.inc` | 2970-3133, 3246-3249 | `ChState`, `Models`, priors, `FrameState`, `Ctx`, `SimOut` |
| `order_learn.inc` | 3251-3436 | exhaustive/ffmpeg order guesses, `lpc_learn`, `mru_hit` |
| `stereo_sim.inc` | 3602-3626, 3736-3786 | `sim_windows`, `sim_stereo`, `ff_stereo_guess` |
| `container.inc` | 3964-3991 | `Segment`, `MBlock`, `FlacLayout` |
| `wav_common.inc` | 4052-4089, 4209 | `default_mask`, `wav_header`, `g_wavbuf`, `wav_read` |
| `metadata.inc` | 4233-4274, 4387-4528, 4648-4751 | stream params, vendors, `code_layout`, `code_derived`, `vendor_hint` ... |
| `compress_util.inc` | 4754-4843 | `Options`, `-x` dump, `parse_layout`, `print_stats` |
| `decompress.inc` | 5027-5129 | `decompress_core`, `do_decompress` |

Some topics are interrupted by a phase 1 file (`flac_defs.inc` by
`lpc_res_fast.inc`, `meta_model.inc` by `model_residual.inc`, `stereo_sim.inc`
by `sim_subframe.inc`, `wav_common.inc` by the WAV classes, `metadata.inc` by
`code_vorbis.inc`/`code_seektable.inc`). Either such a topic becomes several
files, or the definitions are reordered. Reordering changes the function
order in the binary, so it belongs in phase 3, where the check is identical
`.meta` output.

## 5. Phase 3: cleanups from the review (done)

As implemented: (1) `reset_run_state()` in `meta_model.inc`, called at the start of
compression pass 2 and of every restore; `code_layout` resets nothing any more.
(2) `bool ln2sq` parameter, taken from `FrameState`. (3) `WinCache` and the
`LpcPredictor` work buffers became members (their only instance is the static
`g_lpred`); `ResStats` sums and the stereo signals are file-scope arrays; the
work buffers are arrays, `init_work()` only sets up the `s32**` view of the PCM.
(5) The functions are split as listed, with these names:
`lpc_stage0`, `lpc_find_guess`, `lpc_find_at_order`, `lpc_accept`,
`lpc_debug_miss`, `lpc_code_explicit`; `model_frame_header`, `model_chmode`,
`model_subframe`, `subframe_prediction`, `model_fixed_order` (the state update
stays in `model_frame`); `compress_scan`, `write_side_dump`, `compress_model`,
`print_compress_stats`, `verify_meta`; `seek_detect_spacing`, `seek_point`;
`vc_vendor`, `vc_b64_detect`, `vc_comment` (the lambdas became `vc_put32`,
`vc_text` on a plain `VcCursor`); `model_porder`, `model_rice_param`,
`model_escape_bits`; `sim_fixed`, `sim_lpc`, `sim_rice_bits`. The split
functions' files `sim_subframe.inc`, `code_vorbis.inc`, `code_seektable.inc` and
`do_compress.inc` were merged into `encoder_sim.inc`, `vorbis.inc`,
`seektable.inc` and `compress.inc`; `model_lpc.inc`, `model_frame.inc` and
`model_residual.inc` kept their names. (6) `VOTE_MIN`, `RICE_VOTE_MIN`,
`LEARN_FULL`, `LEARN_EVERY`, `EXH_LEAD_MIN`, `EXH_LEAD_DIV`, `FF_SWITCH`,
`SEARCH_FULL`, `SEARCH_MISS_RATIO`, `SEARCH_EVERY` in `meta_model.inc`.

The plan as written:

Separate commits after phase 1. These change the code, so the check is no
longer the binary but **identical `.meta` files on the test corpus** (plus
round trip with verify on every file); any intended format change has to be
listed in its commit message.

1. `reset_run_state()` for all static state (finding 1).
2. Error scale as a parameter instead of toggling `g_ln2sq` (finding 2).
3. Static locals moved to file scope in their `.inc`; drop the pointer layer over
   static buffers (finding 3).
4. Reorder sections to remove forward declarations (finding 4): WAV I/O before
   the meta model; `vorbis_ok`/`vendor_hint` before `code_vorbis`/`code_layout`.
5. Split the large functions along their existing stages:
   * `model_lpc`: `lpc_stage0_sim`, `lpc_stage1_candidates`,
     `lpc_stage2_explicit`, shared `lpc_accept`, `lpc_debug_dump`;
   * `model_frame`: `model_frame_header` (block size, codes, number),
     `model_chmode`, `model_subframe` (wasted bits, type, fixed/LPC/residual),
     `frame_state_update`;
   * `do_compress`: `compress_scan` (pass 1: segments, re-encode check, WAV),
     `compress_model` (pass 2), `write_side_dump`, `verify_meta`;
   * `code_seektable`: spacing-mode detection (encoder only) / point coding;
   * `code_vorbis`: vendor, comment list, base64 value;
   * `model_residual`: partition order / rice parameters;
   * `sim_subframe`: fixed estimate / LPC estimate.
   After this no function is over 100 lines, and the per-function `.inc` files from
   phase 1 become topic files (`lpc_model.inc`, `frame_model.inc`, ...).
6. Name the learning constants (finding 7).
