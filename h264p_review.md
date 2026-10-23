# h264p source review

This review covers `h264p_src/` at commit 547d495 (section 7 says where each finding stands since). It looks for:

- bugs;
- duplicate or redundant code;
- unnecessary wrappers;
- architecture that makes the code harder to change or verify.

Findings are numbered by area:

| prefix | area |
|---|---|
| F | front end and threads |
| S | bitstream syntax |
| M | model |
| R | reconstruction |
| C | context mixing and tuning tools |
| D | duplication |
| W | wrappers |
| A | architecture |

Each finding gives file:line references and a status:

| status | meaning |
|---|---|
| REPRODUCED | I ran it and saw the failure myself |
| CONFIRMED | a reviewer reproduced it, or I confirmed it by reading the code |
| LATENT | the code is wrong, but no current input or configuration reaches it |
| POSSIBLE | plausible from the code, not demonstrated |

## Scope and method

| part | files | lines |
|---|---|---:|
| front end, threads | h264p.cpp, coro.inc, mt.inc, rcio.inc, cmbase.inc | 2,607 |
| bitstream syntax | bits, rows, tables, cabac, cavlc, chan, syntax, slice, tsv, log | 4,478 |
| model | model.inc | 3,190 |
| reconstruction | recon.inc | 3,796 |
| context mixing, levels | fam, famcfg, famvars, cm, rc, mapping, lv1-9, mk.sh | 2,630 |
| tuning tools | IDX/idx2inc.pl, import.pl, opt.pl, ftune.py, famtune.cpp; the 75 generated family templates | 999 + 3,150 |

Lib3/, MTwrap_v3/ and h264split.inc are imported unchanged. They were read only to judge how h264p uses them.

How the review was done:

- **Reading.** Five reviewers each read every line of one area of the table. I then did a cross-file pass of my own: `-Wall -Wextra` under GCC and clang, greps for dead code, and a scan of function lengths.
- **Sanitizers and round trips.** All test streams went through UBSan builds at every level and ASan builds (heap and globals) with byte-exact round-trip checks. So did about 30 streams generated with libx264: MBAFF, 4:2:2, 4:4:4, 10-bit, gray, PCM, lossless, and tiny and extreme sizes.
- **Fuzzing.** About 14,000 mutated inputs (whole streams, headers, TSV files, bit flips and compressed files) went through `t`/`r`, `c`/`d` and `d` alone.
- **Reference comparison.** The reconstruction was compared frame by frame with ffmpeg.
- **Crafted streams** reached paths the tests don't.
- **Checking the findings.** Before a finding went in, I re-ran it or re-read the code myself, or the reviewer reproduced it. Every REPRODUCED finding below I ran myself on the shipped `./h264p`.

The repro files are in the session scratchpad, not in the repo. Each finding says how its input was made.

## Summary

No reviewer found a round-trip failure. Every valid stream, crafted stream and fuzzed mutant either came back byte for byte, was stored verbatim, or was rejected cleanly. The coroutine codec has no globals, and its AVX2 and plain C++ builds produce identical output. The reconstruction is bit-exact against ffmpeg.

What the review found:

1. **One crash and two `-t` failures:**
   - A stream whose POCs lie 2³¹ apart crashes `c` at the default level with SIGFPE (R1).
   - `c -t` exits 0 with an undecodable file after a late worker failure (F1).
   - `x -t` hangs forever when the compressor fails first (F2).
2. **Wrong-but-lossless modelling that costs compression:**
   - A repeated PPS makes the model start a new picture mid-picture (M1).
   - An inter macroblock's 8x8 residuals reach the reconstruction late (M2).
   - Several stale variables (M5-M7).
3. **Inputs that cost far too much or fail whole:**
   - An 11 KB FMO stream takes 43 s (S1).
   - A 17.7 KB stream needs 1.6 GB of RAM (S2).
   - One huge coefficient aborts the whole compression instead of storing that NAL unit verbatim (M3).
   - Naming the same file as input and output destroys the input (F3).
4. **Code written two to six times:**
   - two H.264 header parsers;
   - the CABAC context rules in both the walk and the model;
   - the TSV and compressed pipelines side by side;
   - a bilinear SAD six times;
   - weighted prediction three times;
   - 15-family lists, 105 hand-kept masks and 75 near-identical templates;
   - the leftovers of two removed threads.
5. **Weak seams between sound parts.** The parts themselves are good: the symmetric syntax walk, the coroutine codec and the exact SIMD. The seams between them are not:
   - the channel interface exposes the walk's internals;
   - `Model` (2,700 lines) and `ReconT` (3,240 lines) pass state through members that work only when called in one exact order (M2, M5 and M6 are that failing);
   - h264p.cpp is a 1,950-line monolith.

### Top issues

| id | issue | where | severity | status |
|---|---|---|---|---|
| R1 | SIGFPE at levels 3-9 (9 is the default) when two POCs are 2³¹ apart | model.inc:2084, 2082; recon.inc:812-858 | high | REPRODUCED |
| F1 | `c -t`: exit 0 and an undecodable file after a late worker failure | h264p.cpp:1649; MTwrap_v3/loop_enc.inc:13 | high | REPRODUCED |
| F2 | `x -t` hangs when the compressing stage fails first | h264p.cpp:1816-1821 | high | REPRODUCED |
| F3 | same file as input and output: the input is destroyed, exit 0 | h264p.cpp:829, 1531 | medium | REPRODUCED |
| M1 | a repeated PPS splits a picture: wrong reconstruction from there on, +6.6% | model.inc:939 | medium | REPRODUCED |
| M2 | inter 8x8 residuals reach the reconstruction late: 0.16-0.2% lost on High-profile streams | model.inc:1522; recon.inc:1820 | medium | REPRODUCED |
| M3 | a coefficient beyond 2²⁴ aborts the whole compression | model.inc:2968 | medium | REPRODUCED |
| S1 | FMO map rebuilt for every slice, O(W²): 11 KB of input takes 43 s | slice.inc:195, 109-144 | medium | REPRODUCED |
| S2 | the value log stores every coefficient as int64: 1.6 GB for 17.7 KB | log.inc:34-48, 79 | medium | REPRODUCED |
| F4 | `-t` cuts only at IDR access units and holds whole parts in memory | h264p.cpp:1639; mt.inc:129, 176 | medium | CONFIRMED |
| C1 | ftune.py zeroes every parameter of a family the dumps don't exercise | IDX/ftune.py:103, 117-120 | medium | CONFIRMED |
| D1 | CABAC ctxIdx rules written twice, in the walk and in the model | slice.inc vs model.inc | duplication | CONFIRMED |
| D2 | TSV and compressed pipelines repeat the per-NAL logic | h264p.cpp:397-1433 | duplication | CONFIRMED |
| D3 | the removed parse-ahead thread's scaffolding is still in compress_stream | h264p.cpp:1120-1319 | redundant | CONFIRMED |
| A1-A3 | walk internals exposed to the model; call-order coupling between Model and ReconT | chan.inc; model.inc; recon.inc | architecture | CONFIRMED |

## 1. Bugs

### 1.1 High

#### R1. A crash (SIGFPE) when two POCs are 2³¹ apart (REPRODUCED)

- **Where:**
  - model.inc:2084: `mv_scale` divides by `2 * dn`, computed as an int.
  - model.inc:2082: `ref_dist` returns the raw int difference `cur_poc_ - poc`.
  - recon.inc:812-858: `poc()` returns `(int)std::min(top, bot)`, built from se(v) values of up to ±(2³¹−1) with no clamp.
- **What goes wrong:**
  - With POCs alternating between +2³⁰ and −2³⁰, the distance wraps to INT_MIN (the build uses `-fwrapv`).
  - INT_MIN passes the callers' `dn && dr` guards (model.inc:1486, 1500, 2132, 2152; td_mv at 2097).
  - `2 * INT_MIN` wraps to 0, and `div_floor` divides by zero.
- **Repro:** `poc_intmin.264` (6,786 bytes) was made from test/gen's jm_poc1 by setting `delta_pic_order_cnt_0` to 2³⁰ through `t`/`r`.
  - `h264p c -9` and `c -3` die with SIGFPE (exit 136) and leave a 0-byte output; the reviewer saw the same at `-5` and `-7`.
  - `c -1` works, and `t`/`r` round-trip the stream.
  - The reviewer's gdb trace: `div_floor(b=0)` ← `mv_scale(dn=INT_MIN)` ← `proxy_mv` ← `ipm`.
  - ffmpeg decodes the stream, with warnings.
- **Conformance:** the stream is not conforming, because the spec keeps POC differences within 16 bits. But the parser accepts it, and the default level crashes on it.
- **Fix:** compute distances and the divisor in int64, and clamp the distances as the spec clamps temporal direct's tb/td (Clip3(−128, 127, …)). Alternatively, saturate `RPic::poc`. Fix M7 in the same function.

#### F1. `c -t` exits 0 and keeps an undecodable file when a worker fails late (REPRODUCED)

- **Where:** h264p.cpp:1649-1658 (`compress_mt`) and 1816-1830 (`roundtrip_mt`). The cause is MTwrap_v3/loop_enc.inc:13 and :18.
- **What goes wrong:**
  - MTwrap's encoder looks at `C1[i].f_fail` only while `f_quit == 0`. Once the splitter has handed over the whole input, the pair sets `M2.f_quit` (coro2pair.inc:60), so a worker that fails after that is skipped and MTwrap yields 0.
  - `compress_mt` takes its exit status from `process_file()` alone. It does compute `mt_failed()`, but only to pick a message, and it then prints a "done" progress line after the error.
- **Repro:** `c -s -1 -t 2 -c 1 bb.264 o.c`, where bb.264 is the test/002 streams concatenated with one byte flipped in the second-last slice.
  - It prints "NAL unit 239 … cannot be converted: invalid total_zeros", exits 0, and keeps a 2,659,062-byte file.
  - `d o.c` fails with "a part's record cut short or broken".
- **Not only with `-s`:** any late failure behaves the same way.
  - Out of memory: `(ulimit -v 70000; h264p c -9 -t 4 -c 16k test/002/02.h264 o.c)` exits 0 with a 14-byte file.
  - A part that compresses to 4 GB or more (F4).
- **Fix:**
  - `int r = (process_file(...) || mt_failed(c->M2)) ? 1 : 0;`
  - Do the same for both stages of `roundtrip_mt`, and in `decompress_mt`.
  - Keep `done()` silent after `halted`.

#### F2. `x -t` hangs forever when the compressing stage fails first (REPRODUCED)

- **Where:** h264p.cpp:1816-1821.
- **What goes wrong:**
  - When `MtEnc` yields 3, the outer CoroutinePair stops driving `MtDec` (coro2pair.inc:41, 85-86).
  - `MtDec`'s workers wait for input forever (thread_pipe.inc:79).
  - `x->M2.Quit()` joins them (MTwrap.inc:157) and never returns.
  - The mirror case (the decoder fails first) has the same shape.
- **Repro:** `x -s -1 -t 2 -c 16k bigbad.264 o.264` prints the error, then runs until `timeout 60` kills it (exit 124). Single-threaded `x` on the same file exits 1. gdb shows the main thread in `pthread_join` and both decoder workers in `pthread_cond_wait`.
- **Fix:** after a failed pair, run the unfinished stage to its end (f_quit=1, a 0-length input, output discarded) before Quit. Alternatively, give MTwrap an abort that wakes its workers.

### 1.2 Medium

#### F3. Same file as input and output: the input is destroyed (REPRODUCED)

- **Where:** `open_files` (h264p.cpp:1531-1544) and `open_out` (829-836) truncate the output without comparing it to the input.
- **Repro:** `c -1 a.264 a.264` exits 0, and the 37,177-byte input becomes 15 bytes, an empty stream compressed. The same happens through a symlink.
  - `t a a`: exit 0, 14 bytes.
  - `x a a`: exit 0, 0 bytes.
  - `d a.c a.c`: truncates the file, then deletes it.
- **Fix:** compare st_dev/st_ino (the file IDs on Windows) before opening for writing. Better, write to a temporary name and rename on success, which also fixes F7.

#### M1. A repeated PPS splits a picture (REPRODUCED)

- **Where:** model.inc:939 decides picture boundaries with `h.sps != p.sps || h.pps != p.pps`, comparing `shared_ptr` objects. Every PPS NAL unit creates a new object (h264p.cpp:408, 456).
- **What goes wrong:** the spec allows an identical PPS to be repeated inside a coded picture (7.4.2.2), and 7.4.1.2.4 compares pic_parameter_set_id values. Here, each repetition starts a "new picture" mid-picture:
  - `pic_++`, so near_info() loses the picture's earlier slices;
  - aq_picture() runs and lq_ is reset;
  - the POC histories are rewritten;
  - `rec_.slice_begin(h, true)` deblocks the half-decoded picture, pushes it into the DPB and starts another.

  So the reconstruction and the reference lists are wrong from there on. The round trip stays exact, because both sides make the same mistake.
- **Repro:** x264_slices4.264 with its PPS repeated before every non-first slice.
  - At `-9` it grows from 28,485 to 30,368 bytes (+6.6%).
  - At `-1`, which has no picture-level state, the two cost the same (31,410 vs 31,403). So the loss comes from the false picture starts, not from the extra NAL units.
- **Fix:** compare `pps_id`, and keep `!p.sps` for "no previous slice". An SPS change takes effect only at an IDR, which the other tests already catch.

#### M2. Inter macroblocks with the 8x8 transform reach the reconstruction late (REPRODUCED; found by both the model and recon reviewers)

- **Where:**
  - model.inc:1522: `cbp()` calls `prepare()` first.
  - model.inc:1458: `prepare()` passes `mi_->t8`, which for an inter macroblock is still 0 at that point, because its transform_size_8x8_flag is coded after cbp (slice.inc:645-648; set by t8x8() at model.inc:1603-1620).
  - recon.inc:1820: `prepare()` latches `mb_t8` only on its first call.
  - recon.inc:1891-1897: `luma_add` adds a cat-5 (8x8) block only if `mb_t8` is set. The comment at 1899 already admits "mb_t8 may be from before".
- **What goes wrong:** every P/B macroblock with the 8x8 transform gets all its residuals only at mb_end. Sign prediction for blocks 1-3 (`edges()`, n = 8; model.inc:334-342) and edge_miss therefore see the prediction where the reconstructed earlier blocks should be.
- **Evidence:**
  - The model reviewer's level-9 build lets a later `prepare()` refresh `mb_t8`. I re-ran it: x264_base 27,578 → 27,522, x264_keyint12 38,420 → 38,374, jm_high8x8_cabac 5,723 → 5,717, all round trips exact.
  - The recon reviewer's variant (latch `mb_t8` at the first cat-5 block) gives 181,487 → 181,192 bytes over 9 streams (−0.16%).
- **Fix:** pass the flag to Recon in `t8x8()`, or derive it from the block category. This changes the compressed format.

#### M3. A coefficient beyond 2²⁴ aborts the whole compression (REPRODUCED)

- **Where:** model.inc:2968: `cm_int(f_lvl.code(...), 1 << 24)`. `cm_int` (213-216) always reports "corrupt stream".
- **What goes wrong:**
  - The walk accepts levels up to about 2²⁸ with CAVLC (level_prefix ≤ 31, cavlc.inc:104) and about 2²⁹ with CABAC (slice.inc:1132). So the dry run accepts the NAL unit, and the failure comes in the model pass, which has no verbatim fallback.
  - A bit error in a CAVLC slice easily produces such an escape. One damaged slice then fails the whole stream, instead of being stored verbatim as h264p promises.
- **Repro:** x264_cavlc_b.264 with one level set to −20,000,000 through `t`/`r` (craft_bigcoef.264). `c -1` fails with "corrupt stream: a value of 19999999 out of range", exits 1 and writes nothing, while `t`/`r` round-trip the stream.
- **Fix:** use one coefficient limit everywhere (see S4). Reject values beyond it in the dry run, so the NAL unit goes verbatim. Make `cm_int` say "internal:" when compressing.

#### S1. FMO: the slice-group map is rebuilt for every slice, O(W²) for box-out (REPRODUCED)

- **Where:**
  - slice.inc:195 rebuilds the whole map for every slice, 2-3 times per slice in `t` and `c`.
  - slice.inc:109-144: the box-out walk (type 3) re-traverses filled units. That is about 8.4M steps per call for a 4096×1-MB picture, whatever the slice's size. The other map types cost O(picture) per slice.
- **Repro:** SPS of 4096×1 MBs, PPS with 2 slice groups of type 3, P slices that are each one skip run.
  - `t fmo3.264` (1,125 bytes, 100 slices) takes 3.8 s; the same stream without FMO takes 0.01 s.
  - 1,000 slices (11 KB) take 43 s, about an hour per MB of input.
- **Fix:** each unit's rank in box-out order depends only on the PPS/SPS, so compute it once; the group test becomes `rank < units_in_group0`. Types 4 and 5 are rank compares too. Cache the map per picture, with a next-MB-in-group table for `next_mb()`.

#### S2. The compressor's value log costs about 10⁵× memory on crafted input (REPRODUCED)

- **Where:**
  - log.inc:34-48 and 79: a `std::vector<int64_t>` grown by a doubling, zero-filling `resize`.
  - The walk logs every coefficient of every coded block, empty ones included (slice.inc:855; 831 for CAVLC 8x8).
  - `compress_stream` uses the log at every level (h264p.cpp:1133).
- **Repro:** one IDR slice of 4096×34 MBs, 4:4:4, transform 8x8, cbp 15, every block empty.

  | input | size | `c -1` peak RSS |
  |---|---:|---:|
  | amp_cabac.264 (crafted) | 17.7 KB | 1,576 MB |
  | amp.264 (crafted) | 975 KB | 1,576 MB |
  | test/002/03.h264 (real) | 1.1 MB | 27 MB |

  `t` peaks at 41 MB on the crafted stream. The cost is one maximal slice per model, multiplied by `-t N`.
- **Fix:** log blocks sparsely, as a count up to the last nonzero coefficient, the way the TSV rows already do. Store int32, and grow with `reserve`.

#### F4. `-t` cuts only at IDR access units and holds whole parts in memory (CONFIRMED)

- **Where:**
  - h264p.cpp:1639 and 1811 give H264Split allow_i=0.
  - The splitter buffers the whole part (h264split.inc:485, 576), and the worker reads all of it before its model starts (mt.inc:129-133).
  - A record's size field is u32 (mt.inc:176).
- **Effect:**
  - A stream with one IDR becomes one part. onegop.264 (1 IDR, 1,499 P slices) with `-t 4 -c 1` peaks at 120 MB against 13.8 MB single-threaded, and only one worker is busy. The intra-refresh and open-GOP test streams give "1 parts on 4 threads".
  - A multi-GB single-IDR stream needs about 3× its size in RAM. If its one part compresses past 4 GB, the failure comes after the input has ended, so F1 turns it into exit 0.
- **Fix:** h264p already round-trips streams that start at a non-IDR I picture, or at a P picture with SPS/PPS prepended (`x -s -1` and `x -s -9` both reproduce such cuts). So allow_i=1, plus a hard part-size cap at picture boundaries, is viable. Otherwise refuse with a clear message.

#### C1. ftune.py wipes the tuning of any family its dumps don't exercise (CONFIRMED)

- **Where:** IDX/ftune.py:103 (the tie rule) and 117-120 (`img[...] = cur  # (ties: the zeros kept)`), then `export()`.
- **What goes wrong:** a flip is kept when the cost is unchanged and the bit became 0. In a family with no decisions in the dumps, every trial costs 0.0, so every 1 bit is cleared and exported. mk.sh:32 and :41 then import the result into the .idx files.
- **Evidence:** pcm.dump is just `R` on every test/002 stream. `ftune.py famtune_l1 dump1 1 pcm` produced 31 of 31 pcm patterns all zero, and importing changed 23 lines of l1/pcm.idx. Thinly exercised knobs of other families drift to 0 the same way.
- **Fix:** skip families whose starting cost is 0 or tiny. Commit tie flips only together with a strict gain, as opt.pl does.

### 1.3 Low

**Front end and threads**

| id | issue | where | status |
|---|---|---|---|
| F5 | MTwrap's rings are sized by `-c` (for d, by the header's part size), up to min(part, 1 GB) per worker each way. `c -t 64 -c 256` fails with "cannot start 64 threads"; the cause is about 32 GB of ring mappings. d doesn't need the stored size: a stream decodes with that field patched to anything. | h264p.cpp:1639, 1710, 1728, 1811 | REPRODUCED |
| F6 | `t` and `r` never flush stdout before `ferror`: `t empty.264 - > /dev/full` exits 0. | h264p.cpp:959, 1059 | REPRODUCED |
| F7 | Cleanup on failure differs by command. `r garbage r.out` leaves an empty r.out. A truncated TSV leaves 0 bytes, not even the units restored so far. Exceptions caught in `main` (`tc_map`, `bad_alloc`) skip all cleanup, and so does a crash (R1 leaves a 0-byte file). Meanwhile `c` and `t` always delete their output, and `d` deletes it when nothing was decoded. Any failed run has already truncated an existing output. | h264p.cpp:1053-1056, 1944-1947 | REPRODUCED |
| F8 | The progress line can interleave with a worker's error: one thread can be in `show()` while another calls `stop()` and prints. | h264p.cpp:158-196; mt.inc:123-127 | POSSIBLE |
| F9 | Misleading messages. The usage line documents `-L … 1 to 9`, but the option is the digit itself and only odd levels exist. A 5-byte `h264c` file is reported as "not a stream h264p compressed" rather than truncated. `tc_map` reports small sizes as "cannot map 0 MB". MTwrap memory failures read "cannot start N threads". mt.inc:18 names a nonexistent `mt_head()`. | h264p.cpp:1861, 227-231, 1640; cmbase.inc:110 | REPRODUCED |
| F10 | Windows: the regular-file checks use plain `stat()` (`_stat64i32` under UCRT MinGW), which may fail on files over 2 GB. Big inputs would then lose the model-pass check, and a failed big output would not be removed. `file_size` already uses `_fstati64`. | h264p.cpp:845-846, 1564-1575 | POSSIBLE |

**Bitstream syntax**

| id | issue | where | status |
|---|---|---|---|
| S3 | SEI payloadType/payloadSize: the reader bounds the 0xFF sum before adding the last byte, while the writer checks the final value. `t -s sei_bad.264` fails with "value 16777217 out of range"; without `-s` the SEI is stored verbatim. | syntax.inc:79-81 vs 85 | REPRODUCED |
| S4 | CAVLC levels: the reader accepts level_prefix up to 31 (\|level\| up to about 2²⁸), but the writer throws above 2²⁶, and the model above 2²⁴ (M3). The guard uses int `abs`, so INT_MIN would pass. | cavlc.inc:103-104 vs 153 | CONFIRMED |

**Model**

| id | issue | where | status |
|---|---|---|---|
| M4 | The raw byte model's tables (`raw_o0_`, `raw_o1_` (128 KB), `raw_w_`, `raw_wc_`, `raw_mc_`) have no initializers, and neither `init()` nor `cm_mix::init` clears them. They are zero only because CoroObj value-initializes the object on fresh mmap pages. A model built any other way would desync the raw/SEI byte model. | model.inc:604-617, 553-579; h264p.cpp:1467 | LATENT |
| M5 | `tx_` is written only on the skip path and never reset, so I-slice mb_type at -7/-9 reads the texture of the previous picture's last P/B macroblock. Resetting it saves a byte on 03 and 04. | model.inc:800, 1213; resets at 1041, 1175 | CONFIRMED |
| M6 | `end()` reads `col_` after `finish_mb()` has rewritten that record. So end_of_slice's "co-located" variable is the current macroblock itself in 101,484 of 105,000 decisions on x264_slices4. | model.inc:1030, 1054-1097, 1141, 1273-1282 | CONFIRMED |
| M7 | `div_floor` requires b > 0, but `mv_scale` passes `2*dn`, which is negative for future references. The result is then truncated rather than floored, one quarter-sample off in 15,633 of 35,088 cases with dn < 0 (v=−64, dr=dn=−8 gives −63). | model.inc:232, 2084 | CONFIRMED |
| M8 | B-slice 16x8 and 8x16 are swapped in `parts_`. It's harmless today: the output is byte-identical with the fix. | model.inc:1441 | LATENT |
| M9 | The part masks behind the static_asserts are incomplete: mvs.rt needs the reconstruction but isn't in H_mvs_rec, and ipm.p2 is in neither H_ipm_rec nor H_ipm_proxy. | model.inc:26-133, 520-551 | LATENT |
| M10 | Per-picture bookkeeping (POC histories, hprev_ok_, hkey_) comes from the picture's first slice only, so it is wrong when later slices have other reference lists or slice kinds. | model.inc:965-988 | POSSIBLE |
| M11 | 4:4:4 Cb/Cr DC and 4:2:2 chroma DC are treated as 2-wide raster blocks, which makes the ft/fq/lt variables meaningless there. Indices stay in bounds. | model.inc:2879, 2939, 2950 | POSSIBLE |

**Reconstruction**

| id | issue | where | status |
|---|---|---|---|
| R3 | H264P_RECDUMP never writes the last picture, because `finish_picture()`'s only caller is the next `slice_begin`. With `-t`, every model opens the same file with "wb". | recon.inc:527-545, 695-706, 2626 | CONFIRMED |
| R4 | SP/SI slices pass `supported()` and are reconstructed as P/I, without SP's re-quantization (8.6.1). It costs compression only, and contradicts the header comment. | recon.inc:7-11, 2617-2621, 864, 929 | POSSIBLE |
| R5 | A reused picture buffer keeps the old picture's intra/rf/mv where no macroblock was decoded (lost slices). Direct prediction then reads stale motion; it's deterministic on both sides. | recon.inc:2633-2640, 2036-2068, 2686-2700 | CONFIRMED |
| R6 | `luma_ready()` lacks the `mb_prepared` guard that the other entry points have, and `mb_start` doesn't reset `mb_cls`, `mb_t8` or `mb_modes`. Today the model always calls `prepare()` first. | recon.inc:1846, 1008-1022 | LATENT |
| R7 | The AVX2 and plain SAD kernels return different values when they stop early: they check budgets every 2 or 4 rows vs every row, and `outer_sad` groups differently. The builds agree only because every caller compares against the bound and every budget is positive. Nothing enforces that. | recon.inc:82-136, 199-215, 3058-3064, 3157-3164 | LATENT |

**Context mixing and tuning tools**

| id | issue | where | status |
|---|---|---|---|
| C2 | idx2inc.pl's shipping form pastes variable expressions unparenthesized: `f&3` becomes `(f&3>0+(1-1))`, i.e. `f & (3>0)`, while the tuning form passes `f&3` as an argument. No IDX uses an expression yet; the first one will make the two builds code differently. | IDX/idx2inc.pl:216, 236, 251 | LATENT |
| C3 | The tuning build maps its tables in `_Init()`, but no `_Quit()` is generated (IDX-FORMAT.md says there is one). So every tuning-build model leaks its tables after a kRestart, with `-t`, and for each `R` famtune replays. | IDX/idx2inc.pl:397-411; model.inc:463 | CONFIRMED |
| C4 | The AVX2 counter path loads and stores 8 bytes per 6-byte counter, touching 2 bytes past a table's last entry. `cn_v` checks table order, not that the tail is mapped. This is safe in the release layout. A Windows tuning build could fault, because 14 tables are exact page multiples. | fam.inc:286, 389-394, 498-503 | LATENT |
| C5 | APM widths qs/qf size the tables unclamped, while cm.inc:128 clamps q to [3,130]. A base below 3 would index past the tables. | famcfg.inc:137-138 | LATENT |
| C6 | Missing cheap guards: `code_x` has no `node < ND` check, `sign()` doesn't check `sc < NG`, `bz*MND` and `bg*NG` are u32 products, and the AVX2 mixer sum is exact only for N ≤ 32. None of these is asserted. | fam.inc:311, 321, 334; cm.inc | LATENT |
| C7 | Dump builds: with `-t`, the parts' models append to the same `<family>.dump` files concurrently. After a kRestart, the aborted pass's records stay ahead of the rerun's. | fam.inc:115-123 | POSSIBLE |
| C8 | `mk.sh tuning L` leaves the tracked MOD/l\<L\> in tuning form with no restore. A later `make` builds a slower binary with live !MAP! patterns. | mk.sh:114-122 | CONFIRMED |
| C9 | Tool drift. opt.pl defaults to `./oggcomp` and `00.ogg`, and spins forever on an unwritable path (`1 while not open`). mk.sh:31 names the wrong binary path. ftune.py aborts the whole run on one failing trial. IDX-FORMAT.md describes `_Quit`, IDXP/IDXC and `import-all.bat`, none of which exist. | IDX/opt.pl:43-44, 95; mk.sh:31; IDX/ftune.py:81 | CONFIRMED |

## 2. Duplicate and redundant code

### D1. The CABAC binarization and ctxIdx rules exist twice

- **The walk** selects contexts in slice.inc: 861-868, 884-922, 950-998, 1022-1025, 1078-1130.
- **The model** re-derives the same ctxIdx to read context states through cab0/cab1: model.inc:1246, 1295, 1364-1369, 1411, 1429, 1789 (a copy of the walk's `cond` lambda), 2066, 2207, 2438, 2825, 2911 (a copy of the 8x8/chroma DC sig formula), 2933.
- **Risk:** if one copy changes, the model silently reads the wrong states. Both sides read the same wrong state, so round trips survive while compression degrades, and nothing tests for it.
- **Fix:** let the walk report each bin's ctxIdx, or let the model call walk helpers. It already does this for `ref_inc`, `mvd_inc` and the cbf/cbp helpers.

### D2. The TSV and compressed-stream pipelines repeat the per-NAL logic

- `read_nal` (h264p.cpp:397-440) and `write_syntax` (445-489) contain the same switch, although Syn works in both directions.
- `convert_nal` (613-662) and `dry_run` (684-726) share a skeleton.
- `tsv_dump`'s loop (870-946) repeats compress_stream's per-NAL step:
  - the bad-NAL test;
  - the padding detector and its re-run;
  - `next_slice`;
  - the `-v`/`-s` messages (935-940 vs 1299-1307);
  - the statistics (963-968 vs 1338-1343).
- `tsv_restore` (983-1032) repeats `decompress_stream` (1364-1433): `put_zeros`, the start code and header byte, `update_params(unescape())`, `next_slice(starts_picture())`.
- **Fix:** one NAL walker per direction, parameterized by its sink (TSV rows or the model).

### D3. Leftovers of two removed threads

- **The parse-ahead thread** (h264p.cpp:1120-1159, 1195-1319). Commit 93525b6 added it and 3f3d42f removed it, but its scaffolding remains:
  - `NalItem`;
  - the `fill`/`use` lambdas;
  - two Contexts, `cf` (parsing) and `cb` (model), kept in step by hand, with the `padchg`/`padon`/`padframe` hand-over between them;
  - `in_nals` "counted ahead".

  The loop now calls `fill(it); use(it);` back to back on one item. The front-end reviewer traced one per-NAL step on one Context as equivalent, because slice ids are unique and both contexts see the same parameter sets. Confirm it with the suite's MD5s.
- **The finishing thread** (recon.inc:628-690). The row-by-row finishing (`fin_*`) is what remains of it. Nothing reads the current picture's finished rows while it is being decoded, so finishing at picture end would give the same output. Measure cache locality before simplifying.

### D4. Level configuration and family lists, written out by hand

- **lv1/3/5/7/9.inc** (923 lines): each has 30 includes, 15 identical 7-line FC blocks and 15 typedefs. A level's real content is one shape triple and 9 bools, and all 15 families of a level share that shape.
- **IDX/l\<L\>/\<fam\>.inc** (75 templates, 3,150 lines): within a level, the 15 differ only in the family name and the line that unpacks `H_<fam>_v`, which copies famvars.inc's field list.
  - The mixer width is a literal 16 at l1, 24 at l3/5/7, and `TC_MIXN` at l9.
  - The shape is stated twice, and famcfg.inc:175's static_assert checks only W.
- **The 15-family list** appears:
  - six times in model.inc: H_UNMAP 463-479, the members 482-511, H_PARTS 535-549, H_MAP 561-575, table_bytes 580-584;
  - in lv\*.inc, famtune.cpp:156, mk.sh:63 and famvars.inc.
- **105 hand-written part masks** (model.inc:26-133), 71 of them zero. They already drift (M9).
- **Fix:**
  - One `H264P_FAMS(X)` X-macro.
  - Have idx2inc.pl emit the unpack, assign and Table() boilerplate from the shape and famvars.inc.
  - Derive the masks from the variables each family reads.

### D5. Front-end drivers and the level switch

- compress_lv ≈ roundtrip_lv, compress_mt ≈ roundtrip_mt, compress ≈ roundtrip.
- The level switch appears four times (h264p.cpp:1670, 1755, 1838; mt.inc:137). The defaults at 1677 and 1845 can't be reached after `has_level`.
- The same coroutine protocol has three drivers:
  - `process_file` (1486-1507), which copies Lib3's `processfile` and only adds `pr.poll()`;
  - `run_part` (mt.inc:49-81);
  - CoroutinePair.

### D6. Two of everything at the stream level

- **Two H.264 header parsers in one binary.** h264p's own (syntax.inc through Syn, plus `update_params`) and h264split.inc's (parse_sps/parse_pps/parse_slice, with its own bit reader).
  - They disagree on picture boundaries: h264p.cpp:293 `starts_picture()` looks only at first_mb_in_slice == 0, while h264split.inc:194 applies 7.4.1.2.4.
  - That costs no correctness, since parts are coded losslessly whatever they contain. It costs compression only if the splitter cuts badly.
- **`unescape()`** twice: h264p.cpp:361 and h264split.inc:28.
- **Start-code scans** three times: h264p.cpp:794, h264split.inc:445 and mt.inc:242.

### D7. Syntax-layer helpers

- **The two-phase channel exchange is written out at every element:** `v = elem(W ? ch_->X(0) : 0); if (!W) ch_->X(v);`. It appears at slice.inc:491, 553-587 (seven `*_v()` wrappers), 755, 763-766, 820, 831 and 845-857. One helper would remove about 40 lines, the way Syn already hides the same protocol in vu/vue/vse.
- **Two copies of the primitives:** syntax.inc's vu/vue/vse and fail/check (40-72, 126-135) repeat slice.inc's fail/u/ue/se/te (233-270). The two `fail()`s are identical.
- **Near copies:** nc_luma/nc_chroma (slice.inc:1146-1164), and the four cbf_inc_\* (1167-1239), which share a prologue and the A + 2B sum.

### D8. Repeats inside model.inc

- Two identical `rel` lambdas (2222, 2448).
- The inter / I_NxN / other classification, three times in qpd (2373, 2424, 2474).
- The zigzag geometry (N, r, fx, fy), three times in block() (2879, 2939, 2950); the last two run in the same loop iteration.
- The `pr[TC_NODE]` fill loop five times (2064, 2204, 2436, 2818, 2927).
- `x<0 ? 0 : 1 + tc_clamp(tc_qlog(x), 0, N)` 25 times.
- `mc_col` and `mc_nz` share their whole preamble (2301-2316 vs 2322-2329).
- `phist_key_of_slice` (919-925) and `ref_hist` (2288-2292) repeat one history search.
- `mv_pred` is recomputed with identical arguments: in skip_miss's tm_best (1167) and again in proxy_mv (1470, 1483), and twice in ref() at r = 0 (2040, 2058).

### D9. Repeats inside recon.inc

Every copy here has to stay bit-exact across the AVX2 and plain paths, which makes duplication costlier than usual.

- **One bilinear-SAD formula, six times:** `sad_bil` (67-144, three variants), `bil16x2` (179), `sad_bil32` (192), `sad_bil_blk` (223), `sad_bil2` (276), and the clipped scalar loop in tm_search's `sad` (3119-3127).
- **Local copies of helpers:**
  - the `ld` lambdas at 75 and 104 repeat `ld16x2`/`ld8x2` (147, 152), defined just below them; 1751 repeats `ld8x2`;
  - the 128-bit horizontal sum is written out 10 times, although `sad_sum` (160) exists;
  - the 32-bit one 4 times (590, 1437, 2782, 3711).
- **Kernels written twice or three times:**
  - `mc_luma_p` has 256-bit (1555-1600) and 128-bit (1601-1660) paths that are the same kernel at two widths;
  - weighted prediction makes the same five-way decision three times (2131-2193, 2206-2261, 2263-2291);
  - `proxy_nz` and `proxy_mode` share their first 15 lines verbatim (3341-3351, 3368-3378), re-assembling the intra edges that `edge_of` (1253-1257) builds;
  - the AVX2 `miss()` (3668-3713) re-implements `edges()`.
- **Repeated arithmetic:**
  - three variance computations (`maps_row`, `ac_energy`, `proxy_energy`);
  - three dequantizers, with the 4x4 norm-class expression three times (1121, 3418, 3652);
  - four Hadamard variants (`dc16`, `dcc`, `proxy_dc`, `satd_nxn`);
  - "top/left decoded" five times (2749, 2903, 3097, 3631, 3673);
  - macroblock coordinates computed inline 27 times, although `luma_at`/`chroma_at` exist.
- **ReconOff** (3732-3794) mirrors about 45 method signatures by hand, and model.inc's `H_PROXY` adds a second "disabled" layer on top.

### D10. Dead code

| where | what |
|---|---|
| rcio.inc:24-30, 98, 105-110 | `ByteSink`, `CoroIo::write_cb`, `CoroIo::sink()`: unused since the coder became the coroutine |
| bits.inc:93-95 | `BitWriter::align_zero()` |
| cmbase.inc:127-130 | `tbl_n()` |
| rc.inc | the `CARRYLESS` parameter with `carry_lost()`, `RC_LOWBYTES < 8`, `RC_FF_TRIM` (from oggcomp's coder) |
| cm.inc | `cm_apm::limit`, `cm_apm::n`, the `nctx` parameter of `cm_mix::init` |
| fam.inc:255, 331 | `if (!G)` / `if (G)`: always true, because every template declares G |
| fam.inc:545-555 | bitm's prp-mixing branch: no `code()` user sets a scalar prp ≥ 0 |
| mapping.inc | masking_b, pmask2/mmask2, GCCBUG1/2, the pdesc→pdesc_live indirection, the 3-argument `inc` (its `f_cat` unused) |
| IDX/idx2inc.pl | Rate/Rate1 (they emit `TTable`/`eSCALE`, which h264p lacks), ADD, `b&`, def_Data/def_Init/MakeTables, `$Verify`/`$t_sta1`, `$SizeOut`, `$fileT`, `$porder` (an `eval` whose result is dropped), the `USE_NEW` define |
| slice.inc:1030 | `(v & 15) > 15`, always false |
| slice.inc:190 | NAL type check: callers pass only types 1 and 5 |
| syntax.inc:183, 446 | `SliceHeader::colour_plane_id`: stored, never read |
| rows.inc:186-188, cabac.inc:20, cavlc.inc:8-9, syntax.inc:206-211, slice.inc:29-30 | forward declarations right before their definitions, left from a multi-file layout |
| h264p.cpp:877-891 | tsv_dump's offset `p`, always 0 |
| h264p.cpp | compress_stream's `in_bytes`/`in_nals` duplicate `m.n_in`; `kLevels[]` is used only as `[0]`; `check = true` is set at 1632 and again at mt.inc:167 |
| h264p.cpp:1871-1873 | `H264P_NO_MAIN`: nothing defines it, and famtune.cpp includes what it needs directly |
| model.inc | `MbInfo::field` (183; written at 1064, never read); `g.kind == 2` in `tok` (2717) can't be true; `&& mbt_d_` (1346) is always true; `(bb >> 1) * 2 + (bb & 1)` (2759) is `bb`; H_BIT and H_USES are never #undef'd |
| model.inc:346, 2951 | shadowed names: SignPred::init's local `dc` hides a member with another meaning; block()'s inner `cnt` hides the outer one (2859) |
| recon.inc | `on` (488) is never used. `stype` (516), `direct_spatial` (517), `RPic::lt_ref` (404), `mb_i16m` and `mb_cpred` (1795) are written, never read. `mb_rf`/`mb_mv` (1797-1798) are used only inside `prepare()`. `lt_done` in `mark()` (732, 773, 789) is dead. |
| recon.inc:436-440, 709 | `RPic::pad()` and its only call are unreachable: with PX, a current picture is always active, and `fin_end` pads every reference |
| recon.inc:489 | `active` always equals `PX && mact` |
| recon.inc:1628-1659 | a `{ const int k = 0; … + k }` block left from a column-group loop (its `else` is mis-indented) |

**Dead tuning knobs.** 615-628 tunable bits per level (13% at l1) feed nothing, and tie-breaking tunes every one of them to zero.

- The families that only call `bit()` (skp, mbt, cbp, ipm, sig, sgn, mvg, qpg) never read rT, rG, lrm, mbm, mwT, mwG, mt or the t/n/g index lines.
- nal, mvs, qpd, cbf and lvl never call `codes()`, so rG, mwG and index g are unused.
- nal, hdr and pcm never set prp or hint, so lrm, mbm and index n are unused.
- Patterns also run past their variable's range: nal's `fld` has 18 values but 31-bit patterns.
- At l9, sig, lvl, sgn and cbf look up and update APM 2 on every decision, with weight 0.

**Orphaned comments** (they sit above the wrong code):

- model.inc:908-911, 1159-1161, 2552-2553;
- recon.inc:337, 1981-1984, 2761-2763, 3267-3271.

h264p_levels.md §7.3 says the model "fills a variable only where the level reads it", but H_USES gates only sig's ft/hb/h3/sn (model.inc:2876-2907).

## 3. Unnecessary wrappers

| id | where | what | instead |
|---|---|---|---|
| W1 | coro.inc:77-103; h264p.cpp:1173 | `H264pArgs` is converted back to `Options` for compress_stream/decompress_stream. These take `check` and `name` separately and use only verbose, strict and meter. `Options::meter` exists only for this, `opt.level` (coro.inc:80) is set and never read, and H264pArgs is aggregate-initialized positionally in seven places. | pass `const H264pArgs&` |
| W2 | h264p.cpp:1486-1507 | `process_file` is Lib3's `processfile` plus `pr.poll()`, which leaves CoroFileProc as a holder for two buffers | one single-thread driver (D5) |
| W3 | fam.inc:321 | `code()` only forwards to `code_x()` | one function |
| W4 | famcfg.inc | make/init/bytes/name/tables are forwarders. `tc_tabs` passes `void* cnt[10]` only for `wire()` to cast them back. The class exists to rename prefixed globals, stamped out 75 times by `#define … #include famcfg.inc`. | have idx2inc.pl emit the configuration class directly (D4) |
| W5 | model.inc:636-640, 917, 1278-1289 | `raw()`/`raw_x()` and `end()`/`end_x()` are splits with a single caller (`end_x`'s ctx is always `st_`); `phist_key()` only returns `phk_` | inline them |
| W6 | recon.inc:672, 1223-1258 | `fin_let` only forwards to `finish_rows(rows, false)`. `luma_edge` → `edge_of<F>` is instantiated once, while the proxy functions copy its body instead of calling it. `sign_edges` → `edges` and `edge_miss` → `miss` only add a guard. | call the targets, and reuse `edge_of` in the proxy functions |
| W7 | recon.inc:488-492 | `act()`, `active` and `mact`: three flags for two states | one state |
| W8 | h264p.cpp:665-671, 736 | the opposite case: `rbsp_bits` writes, and `checked_bits` compares, BitWriter's private `buf_`, `cur_`, `n_` | a constructor and `operator==` in bits.inc |

## 4. Architecture

### What is good

- **The symmetric walk.** One function per syntax structure serves parsing, TSV, the value log and the model, so each bitstream rule lives in one place. Its spec details are right, its tables match ffmpeg's exactly, and its table-driven CABAC engines keep exact bit positions.
- **The coroutine codec.** The chain `Coroutine ← CoroIo ← RcIo ← Rangecoder ← tc_rc ← Model ← H264pCoro` makes a run one object with no mutable globals. Its state is on the heap, the copied stack stays small (`before_yield` checks it), and nothing yields inside an exception handler.
- **Encoder/decoder symmetry.** Both directions run the same writer walk. The current value is read only under `ENC ? … : 0`, and nothing in the coding path depends on unordered containers, floating point or heap addresses. Every generated `make` clamps its variables before indexing.
- **The decoder core and the vector paths.** The decoder core is careful: the DPB is bounded, every `list[][]` read is guarded by `nlist`, and motion compensation clips any vector. The AVX2 paths match the scalar ones exactly.
- **The range coder** handles carries correctly. Its flush trims at most 40 bytes of 0xFF, within the decoder's 48-byte allowance. (Since format 7 it trims nothing, and the decoder reads exactly the bytes written: b6879b8, section 7.)

### A1. The seam between the syntax walk and its channels

- **The model reads the walk's internals.** The channel interface has one virtual `slice_begin` per `SliceCoderT<kMode>` (chan.inc:17-18, 67-70). Through it the model reads about 30 walk members (ctx_, mb_a_/mb_b_, cur_, nb4(), ref_inc(), store_, slice_id_ …), so every walk internal is de facto API, and D1 is the consequence.
- **Compressor-only state sits on every channel.** `BinRec` and the `nobins`/`states` flags (chan.inc:24-52) belong to the compressor's bin check and the walk's mode, yet slice.inc:180, 209-229 and 273-321 write them into the generic interface.
- **A silent three-way contract.** The model's SC type (model.inc:752), write_slice_data's mode dispatch (slice.inc:1278-1285) and ModelChan's nobins/states settings (model.inc:3140-3143) must agree. If they drift, `slice_begin` is silently not called, because Chan has no-op overloads for the other modes.
- **The direction is stored three times and never cross-checked:** `Chan::W` (which LogChan flips), `Syn::W` and `kMode`.
- **Error types don't follow the direction.** TsvError is documented as restore-only (bits.inc:11), yet the compressor's write-back and `cavlc_write_block` throw it, while `slice_group_map` throws ParseError while writing.
- **The docs contradict the code.** chan.inc:11-13 says the walk's state at a channel call is the same in both directions; the code calls after reading but before writing (slice.inc:551-552).
- **Other cross-layer leaks:**
  - x264's padding guess is a `SliceHeader` field marked "not coded", XORed into the I_PCM alignment inside the walk (syntax.inc:201; slice.inc:671, 677);
  - picture-level FMO work is done per slice (S1);
  - `loop()`'s skip-run state means different things in each direction (slice.inc:420-486);
  - the TSV layer branches on W in every method, and its `row_` aliases a buffer that the next `take()` overwrites.
- **Direction:**
  - Split SliceCoderT into a non-template state (neighbours, MB states, contexts, ctxIdx helpers) shared by walk and model.
  - Let the caller choose a bin policy (decode, encode, record or check).
  - Add one element-exchange helper (D7).
  - Make the other `slice_begin` overloads throw.

### A2. `Model` is one 2,700-line struct coupled by call order

- **Too many jobs in one struct.** model.inc:453-3128 has about 170 data members and mixes:
  - context building for 15 families;
  - the H.264 motion-vector derivation (8.4.1: mv_nb, mv_pred, mv_pskip, mv_direct, mv_temporal, about 200 lines);
  - driving the reconstruction from inside the context builders (`prepare()` from three places, luma_ready/coef/luma_add/chroma_add/luma_finish, proxy builds and re-searches in block());
  - per-kind macroblock histories with a copy-on-write snapshot (hkeep_/hcoef_/HistView);
  - the AQ line fit;
  - a header model (a `std::map<std::string,int>` of tags);
  - the raw byte model, with a match model and an embedded x264 dictionary.

  Natural parts: MotionPred, History, AqModel, RawModel, HeaderModel, and per-family context builders.
- **State passes between families as side effects.**
  - skip_miss writes tx_/tm_/tb_/dh_/bd_/tmb_/smv_ and temporarily overwrites mi_->mv/rf/parts_.
  - proxy_mv, called lazily from six places, builds Recon's proxy and writes pmv_/prx_v_/pxq_/pxv_.
  - mbt_d_ lets mbt_intra re-select its context halfway through the tree.
  - talt_ carries over between mvd's two components.
  - col_ outlives finish_mb.
  - prepare() is first-call-wins.

  Correctness depends on the walk calling everything in exactly the expected order. M2, M5 and M6 are this coupling failing silently.
- **Fragile initialization.** Positional aggregate initialization of the `H_*_v` structs (592, 727, 1803, 2860, 2924, 2985, 3114) depends on famvars.inc's field order. This is also where GCC's 320 `-Wmissing-field-initializers` warnings come from (A8). Assigning by name, as most families already do, is safer.
- **Memory.** MbInfo takes 1,192 bytes and MbCol 498 bytes per macroblock. With up to 30 history kinds at kMaxMbs = 139,264, the histories alone can reach about 2 GB.

### A3. `ReconT` mixes a decoder with model heuristics

- **About 40% of it is not decoding.** recon.inc is one 3,240-line class template with about 120 members, and roughly 1,500 of its lines are model heuristics:
  - SAD kernels and gradients;
  - energy/change maps;
  - the intra proxy and forward quantization;
  - SATD mode decisions and DIMD;
  - textures and template matching with its memo;
  - sign edges.

  They share the decoder's mutable per-macroblock state: `lst`, `mb_done4/8`, `skip_in`, the `pq_*` prediction cache, `prx_*`, `tmm`.
- **Coupling with model.inc:**
  - `skip_miss()` writes the skip prediction into the picture, and `prepare()` reuses it through `skip_in`;
  - `bi_diff()` and `inter()` share the `pq_*` cache;
  - the model calls `luma_finish()` from its proxy path (model.inc:2680) to force reconstruction;
  - `tm_search(inner=2)` depends on how far `luma_add` has got;
  - `prepare()`'s timing is set by the model's `cbp()` (M2);
  - `pcm_s` and the `coef` category encoding are filled directly by the model.
- **Implicit dependencies:**
  - ReconRefs, used at levels with reference lists but no reconstruction, carries all the sample machinery for nothing;
  - the parser's bounds (ref_idx, num_ref_idx ≤ 32, weights, log2 values) keep many recon paths safe, but nothing asserts them.
- **A possible split:**
  - a DPB part (RPic, marking, POC, lists, weights) shared with ReconRefs;
  - the macroblock decoder (transforms, prediction, MC, deblocking);
  - a const "sample queries" layer for the model's features;
  - one SIMD-helper header used by both paths (D9).

### A4. h264p.cpp is a 1,950-line monolith

The code base is otherwise split finely, but h264p.cpp holds:

- the command line;
- progress;
- file handling;
- the TSV front ends;
- the Annex B reader;
- the NAL syntax glue;
- the padding detector;
- the compressed container format;
- six drivers.

Its TSV and compressed paths are parallel copies (D2), and the parse-ahead leftover (D3) roughly doubles compress_stream. A natural split is: container and per-NAL step (shared by TSV and CM), drivers, CLI and files.

### A5. A level is stated four times, and generated code is tracked

A level appears as:

- 15 .idx files;
- 15 near-identical templates;
- an lv file with 15 FC blocks;
- the Lv flags.

The shape and the family list are repeated by hand (D4), and configuration goes through macro-stamped classes (W4).

Tuning patches the binary through !MAP! strings. That works, but:

- the tools come from oggcomp/tsvcomp and carry their dead features (D10);
- generation writes into tracked files (C8);
- the newer per-family tuner lost opt.pl's "commit only on strict gain" rule (C1).

Nothing automates the README's claim that AVX2 and plain builds code identically, because `mk.sh check` builds both binaries with AVX2. The reviewers checked it by hand (section 5).

### A6. h264p leans on MTwrap's internals

- It trusts MTwrap's return code, which misses late worker failures (F1).
- It reads `th_N`, `C1[i]`, `c_out` and `M1.st`.
- It uses `-c` as MTwrap's ring size (F5).
- MTwrap has no abort path, so a failed pair can't be torn down (F2).

### A7. Error handling and cleanup are spread out

Output cleanup lives in fail_out, in close_files, nowhere at all (`r`), and in main's catch, which skips it. Each command removes or keeps a failed output differently (F7). One output object with one policy would cover F3, F6 and F7: write to a temporary name, rename on success, remove on failure.

### A8. Build hygiene

- **Include order:**
  - h264p.cpp:46-50 says Lib3's setjmp/jmp_buf/ALIGN macros must not reach system headers. Yet cmbase.inc:6-21 includes system headers again after Lib3, and h264split.inc first includes `<set>` after Lib3; include guards are what keep this working.
  - cmbase.inc redefines `qword` in namespace h264p as `uint64_t` (`unsigned long` on LP64), while Lib3's global `qword` is `unsigned long long`.
  - coro.inc and mt.inc have no namespace of their own and compile only at their include points (h264p.cpp:1459, 1478). mt.inc needs CoroObj, which is defined between those two includes.
- **Compiler warnings** (`-Wall -Wextra`):
  - GCC gives 320 `-Wmissing-field-initializers` in model.inc. All are the `H_*_v` aggregates (A2); I checked that every field is assigned, so they are benign today.
  - recon.inc:642 has a `-Wclass-memaccess` (memset of DbMb, a non-trivial type).
  - clang flags mapping.inc:57's unused `f_cat`.
  - Everything else is in Lib3 and MTwrap_v3.
- **Very long functions:**

  | function | where | lines |
  |---|---|---:|
  | `block` | model.inc:2633 | 462 |
  | `mc_luma_p` | recon.inc:1514 | 202 |
  | `compress_stream` | h264p.cpp:1173 | 175 |
  | `tm_search` (9 lambdas) | recon.inc:3092 | 175 |
  | `inter` | recon.inc:2125 | 170 |
  | `mvd` | model.inc:2101 | 155 |
  | `bit_x` | fam.inc:367 | 153 |
  | `qpd` | model.inc:2359 | 126 |
  | `tsv_dump` | h264p.cpp:851 | 120 |

### A9. Tests

test/h264p.sh has no negative tests: no truncated or corrupt input, no `-s` failure under `-t`, no exit-code checks and no crafted streams. That is how F1, F2 and R1 went unnoticed. In its `c -t … | d` pipeline, c's exit status is ignored.

## 5. Checked and found sound

- **Round trips, all exact:**
  - The shipped binary: 113 small streams × 5 levels = 565 round trips, and test/002 at -1/-3/-9.
  - UBSan with `_GLIBCXX_ASSERTIONS`: 575 round trips at all levels, no reports.
  - UBSan without `-fwrapv` (signed overflow traps) on 12 varied streams, including MBAFF, 4:4:4 8x8, 4:2:2 10-bit, PCM, lossless, FMO, temporal direct and weighted prediction: no reports, and the output matches release.
- **Fuzzing found no crash, hang or sanitizer report.** It covered:
  - 5,700 whole-stream mutants through `t`/`r` (1,500 also through `c -1`/`d`);
  - 4,500 header-targeted mutants and 1,500 mutated TSV files;
  - 1,072 syntax and bit-flip mutants under UBSan, and 609 under ASan;
  - 440 corrupted compressed files at -1 to -9, single and `-t`, all rejected with exit 1.

  Truncated streams are rejected at every cut tried, and appended data is detected. The one crash (R1) came from a targeted crafted stream.
- **Crafted streams round-trip** where the tests don't reach:
  - MMCO 2-6 and reference-list modification idc 2;
  - redundant_pic_cnt and chroma pred weights;
  - 4:4:4 with 12+12 scaling lists;
  - 8/10/14-bit I_PCM;
  - 4096x16, 16x2304, 4096x2304, 18x18, and 32x32 with 16 refs.
- **Reconstruction:**
  - It is bit-exact against ffmpeg on every supported test stream and generated geometry, except the missing last picture in RECDUMP (R3).
  - Memory stays at about 43 MB from 1 to 12 concatenated streams with size changes, so nothing leaks across pictures.
- **AVX2 vs plain C++:**
  - Identical output at every level on all test streams, size-change concatenations and 288 differential mutants.
  - Each decodes the other's output.
- **Tables and spec:**
  - `kCabacInit`/`kCabacInit444` (1,024 contexts × 4) and every CAVLC table match ffmpeg n6.1, and the CBP me(v) and 8x8 sig/last tables match the spec.
  - Every CABAC binarization and ctxIdxInc rule (MBAFF table 6-4 included) matches the spec.
- **Builds and tools:**
  - MOD/ is in sync with IDX/: `mk.sh mod` reproduces it byte for byte. `mk.sh check 1` passes.
  - famtune's replay matches `-v`'s per-family sizes.
  - import.pl's export/re-import leaves every .idx unchanged.
- **Arithmetic:** the counter update and mixer split are exact, the gathers stay in bounds, and all table Volumes are under 2²⁴.
- **I/O edge cases:**
  - Empty input works in all five modes.
  - Pipes work for c, d and x, and the restart is limited to regular files.
  - Single-threaded paths have no 32-bit truncation for inputs over 4 GB.
- **Tool noise:** memcheck and ASan stack complaints come from Lib3's stack-copying coroutines (they appear on the baseline too). helgrind flags only the relaxed atomic progress counters.

## 6. Suggested fix order

1. **R1, F1, F2:** the int64 POC distance, the exit-status line, and a drain-before-Quit for `x -t`. Add negative and crafted tests (A9) so they stay fixed.
2. **F3, F6, F7:** one output object (temporary name, rename on success, remove on failure) with a same-file check.
3. **M3 and S4, then S2 and S1:**
   - one coefficient limit shared by reader, writer and model, with the dry run sending violators verbatim;
   - a sparse int32 value log;
   - precomputed FMO ranks with the map cached per picture.
4. **One format change for M1, M2, M5, M6 and M7.** Each changes the compressed output, so bump the format version once, then retune.
5. **F4 and F5:** allow_i=1 with a part-size cap, and an MTwrap ring size independent of `-c`.
6. **C1, C2, C3:** before the next tuning run.
7. **D3, then D2.** Drop the parse-ahead scaffolding, then merge the TSV and CM per-NAL steps. Both are bit-exact refactors that the suite's MD5s can check.
8. **D1 and A1:** the walk reports ctxIdx, and the walk state shared with the model becomes a non-template.
9. **D4 and A5:** a family X-macro, and generated per-level boilerplate and masks.
10. **A2, A3 and D9:** split Model and ReconT along the lines above, starting with the parts that have no call-order coupling (RawModel, HeaderModel, the DPB, one SIMD-helper header).
11. **The low-severity rest and the dead code (D10),** a file at a time.

## 7. Status

The findings above are of commit 547d495. They were worked through in the order of section 6, in these commits:

| commit | what |
|---|---|
| 91b9ea8 | the crash, the `-t` failures and the robustness bugs: R1, F1-F10, S1-S4, M3, M4, M8, R3, R6, R7; the negative and crafted tests (A9) |
| 102d1e1 | format 6, the model fixes that change the output: M1, M2, M5-M7, M9-M11, R4, R5; the tuning tools, C1-C9; D1; D3's parse-ahead scaffolding |
| 1e60b92 | h264p.cpp split (A4); D2, D5, D6, W1, W2, W8 |
| 15d7ee6 | the seam between the walk and its channels (A1); D7 |
| 4cc4931 | RawModel and HeaderModel out of Model (A2); D8, W5 |
| c989917 | recon.inc: the DPB and the pixel kernels apart (A3); D9 in part, W6, W7 |
| 52acec8 | the context mixing and tuning code: D4, W3, W4, A5, A8, the rest of D10, the dead knobs |
| 51b0753 | the picture finished at its end, not row by row (D3) |
| e9413d3 | recon.inc: the macroblock decoder and the model's sample queries apart, ReconOff as `ReconT<0>` (A3); the rest of D9 |
| 1c2b10b | Model's parts apart (A2); the long functions in parts (A8) |
| 31cf885 | GCC's `-O3` warnings on bit_x's counter vectors (A8), by reading `cn_v` once (measured up to 3% slower at `-1`) |
| 10d2afd | those warnings silenced in place instead, bit_x's machine code that of 1c2b10b (A8) |
| c964f92 | a bound on the model's records of each kind of picture (A2's memory) |

Each commit but the format bump is bit-exact: the same MD5s of `t` and of `c -1` to `c -9` for every test stream, from the AVX2 and from the plain C++ build, and test/h264p.sh passes (98 round trips, and the negative and crafted cases). c964f92 changes the output only of a stream past its bound, and the one such test stream is the crafted stream it came with. Format 6 codes the test streams at `-9` in 4,592,881 bytes, 2,314 fewer than format 5.

Speed. Against a build of 547d495 on the same machine (test/002, CPU time, at c964f92): at `-9` the seven streams compress in 34.7 s against 35.4, the least of five runs of each (03 12.6 s against 13.1, 00 9.1 against 9.4, 04 3.44 against 3.37, the others within 1%); at `-1` they compress in 6.67 s against 6.73 and decompress in 5.65 against 5.73, the medians of five runs. A stream's time varies by up to 10% from run to run on this machine, so the commits since 51b0753 were also compared by the instructions they run (cachegrind; `c` of 00 at `-1`, of 06 at `-5` and at `-9`), each against the one before it: e9413d3 −0.06%, +0.15%, −0.07%; 1c2b10b −0.8%, −0.7%, −0.7%; 31cf885 +0.05%, +0.006%, −0.009% (and +0.08% for 03 at `-1`), and it measured up to 3% slower at `-1` (6.42 s against 6.21, the least of five runs of each stream), so 10d2afd went back to 1c2b10b's machine code, byte for byte; c964f92 +0.001%, −0.12%, −0.004%.

### Bugs: all fixed

| id | fix |
|---|---|
| R1 | POCs held to ±(2³⁰−1), POC distances to 16 bits, `mv_scale`'s divisor in 64 bits |
| F1 | a worker that fails after the input has ended fails the call: exit 1, no output |
| F2 | the stage that a failure leaves running is drained (`mt_drain()`) before its threads are joined. (Since gone with `x`: in `c` and `d` no stage is left running, as MTwrap returns only after its workers end and the splitter always ends well.) |
| F3, F7 | one `Output` for the five commands: never the input (same file, link or stdin), written under a temporary name and renamed when the call ends well, removed when it fails; `d` and `r` keep what they restored |
| F4 | the splitter also cuts at I access units, and at any picture past 4 × `-c`; `-c` is at most 1024 MB, which keeps a part's record within its 32 bits |
| F5 | MTwrap's rings have a size of their own (1 MB); `d` no longer uses the header's part size |
| F6 | stdout is flushed and checked |
| F8 | progress lines and messages under one lock; a call shows its first error only |
| F9 | the usage lists the build's levels; a 5-byte file is cut short, not foreign; small maps in KB; `mt_head()` exists |
| F10 | Windows file checks with `_stati64` and file IDs (cross-compiled, not run) |
| S1 | the slice-group map kept from slice to slice; box-out as a rank order: 0.1 s instead of 43 s |
| S2 | the log keeps 32-bit values and blocks up to their last nonzero value: 287 MB instead of 1.6 GB |
| S3 | an SEI size past 2²⁴ fails as it is read, as it fails written |
| S4, M3 | one coefficient limit (2²⁴) for CAVLC, CABAC and the model; a larger level stores its NAL unit verbatim |
| M1 | picture starts compare the PPS id |
| M2 | transform_size_8x8_flag reaches the reconstruction before `prepare()` |
| M4 | the raw byte model's tables start zeroed however the model is made |
| M5, M6 | `tx_` and the co-located end-of-slice context are reset per macroblock |
| M7 | `div_floor` rounds towards minus infinity for a negative divisor |
| M8 | B 16x8 and 8x16 the right way round |
| M9 | the part masks match what the levels read, and are now made from one list (D4) |
| M10 | per-picture history snapshots, and the reference POCs kept in MbCol |
| M11 | one scan description per block geometry |
| R3 | the last picture is finished at the end of the stream; H264P_RECDUMP refuses `x` and `-t` |
| R4 | SP/SI documented as reconstructed like P/I |
| R5 | a reused picture buffer resets its intra and reference maps |
| R6 | `luma_ready()` checks `mb_prepared` |
| R7 | the SAD kernels return their budget once it is reached, AVX2 and plain alike |
| C1 | ftune.py commits only strict gains and skips families with no cost |
| C2 | idx2inc.pl parenthesizes the variables it pastes |
| C3 | the tuning tables are unmapped (`_Quit()`) |
| C4 | each tuning table is mapped 8 bytes longer; a static_assert checks the shipping layout |
| C5 | `cm_apm_q()` clamps the APM widths for the tables and the APMs alike |
| C6 | the node, sign-context and table-size guards; the mixer's width asserted (≤ 32) |
| C7 | a restarted run discards what it dumped; `-t` is refused in a dump build |
| C8 | `mk.sh tuning` restores MOD/ |
| C9 | opt.pl names h264p, needs a corpus list and fails on open errors; mk.sh, ftune.py and IDX-FORMAT.md corrected |

### Duplication, wrappers, architecture

| id | status |
|---|---|
| D1 | fixed: the ctxIdx rules are walk helpers (slice.inc) that the model calls |
| D2 | fixed: one per-NAL step for `t` and `c` (nal.inc), one `code_nal()` for both directions |
| D3 | fixed. The parse-ahead scaffolding is gone. The row-by-row finishing was measured first: finishing each picture at its end codes the same (the golden MD5s), and costs the same CPU time on test/002 within the runs' ±5% noise (−5: 20.5 s row by row, 21.1 s at the end; −9: 41.4 s and 40.0 s; three runs each). So a picture is now finished at its end, and `fin_next`, `fin_off`, `fin_rows`, `fin_post`, `fin_db`, `fin_mb` and `finish_rows` are gone |
| D4 | fixed: `H264P_FAMS` lists the families (model.inc, famtune.cpp, and mk.sh reads it); one template per level, `IDX/l<L>/family.inc`, instead of 75; famvars.inc's `H_<fam>_VARS` list each family's variables once, with the parts each needs, and make the structs, the 105 part masks and the index builders' locals; the shape is stated once per level |
| D5 | fixed: `drive()` runs every coroutine; `code_one`/`code_mt` (since `CodeLv`/`DecodeLv` over `run_model()`, and `code_mt`/`decode_mt`); `with_level()` is the one switch over the levels |
| D6 | fixed: one `unescape()`, and one start-code finder for whole buffers (`find_start_code`: the reader and mt.inc). h264p's picture boundaries follow 7.4.1.2.4 as the splitter's do (`Model::new_picture`, M1); `starts_picture()` only counts x264's frames for its padding bits. The splitter keeps what it needs of its own: a scanner that finds start codes across the ends of the buffers MTwrap feeds it, and a header parser that tells pictures apart without h264p's walk, which needs a whole context; parts are coded losslessly wherever they are cut (h264split.inc says so) |
| D7 | fixed: `WalkCodes`, `xchg()`, `xval()`, `nc()`, `cbf_inc()` |
| D8 | fixed |
| D9 | fixed. pixel.inc has every kernel and the helpers they share: the horizontal sums and `ld16x2`/`ld8x2`; the bilinear prediction's steps (`bil_mul`, `bil_mac`, `bil_pack`, `bil_rows`, `bil1`), which all its SAD kernels, the chroma MC and tm_search's clipped loop use; integer vectors of 128 and 256 bits under one name (`V128`, `V256`), so that `mc_luma_p`'s half samples are one kernel (`mc_halves_v`) and weighted prediction one `wsum<V>`; one 4-point and one 2x2 Hadamard for dc16, dcc, proxy_dc and satd_nxn; one `blk_sums` for the three variances (the maps', and `aq_energy`'s of the picture and of the proxy). recon.inc: one weighted-prediction decision (`wp_of`, `wp_vec`), one proxy prologue (`proxy_block`), one 4x4 norm class, `above_done()`/`left_done()`, the macroblock coordinates (`mb_x`, `mb_y`, `plane`, `stride`, `mb_px`, `mb_at`), one prologue for `edges()` and the AVX2 `miss()` (`edge_at`); ReconOff is `ReconT<0>`, not a mirror of the interface (A3). Each kernel keeps an AVX2 path beside the plain C++ one; both compute the same (the golden MD5s of both builds), and the CPU time did not grow |
| D10 | fixed, except `tbl_n()`: the tuning form's generated code calls it (idx2inc.pl's `Table()`), so the finding was wrong |
| dead knobs | the knobs a family never reads are frozen in every .idx, which says why; nal's `fld` patterns are cut to its 18 values; APM 2 is neither read nor trained where its share is nothing (bw 16: sig, lvl, sgn and cbf at `-9`). The shipping build codes the same |
| orphaned comments | moved to their code, or removed with the code they described |
| H_USES | h264p_levels.md §7.3 says where `H_USES` gates work |
| W1 | fixed: compress_stream/decompress_stream take `H264pArgs` |
| W2 | fixed: `drive()` replaces `process_file` |
| W3 | fixed: one `code()` |
| W4 | fixed: the class is made by each family's generated file (the level's template defines the shape and includes famcfg.inc), and `tc_tabs` is typed; famcfg.inc's `make`/`init`/`quit`/`bytes`/`name`/`tables` stay, as the interface `tc_fam` reads every family through |
| W5 | fixed |
| W6 | fixed: `fin_let` is gone; `luma_edge` and the proxy functions (with one prologue, `proxy_block`) fill their edges with one `edge_fill`. `sign_edges` and `edge_miss` are the model's queries, with the guard it needs (the macroblock's prediction in the picture); `edges` and `miss`, which they call, also serve skip_miss on the skip prediction it put in the picture itself, which that guard would refuse |
| W7 | fixed: `act()` is `PX && mact` |
| W8 | fixed: `BitWriter(p, nbits)` and `operator==` |
| A1 | fixed: `WalkState` (the walk's state and its ctxIdx rules, not a template) is what the model reads; the caller chooses the bin policy (`BinPolicy`); one virtual `slice_begin`; the walk and its channel check their direction; errors follow the direction |
| A2 | fixed. Model's parts are structs of their own: RawModel, HeaderModel, MotionPred (8.4.1's vectors, into the arrays it is given), History (the records of each kind of picture, a P picture's snapshot, the views of them), AqModel (x264's AQ lines and the macroblocks' energies); the families' context builders are functions of their own (block()'s cbf, sig, lvl and sgn stages over one BlockCx; mvd's prediction and sign; qpd's variables and sign), and the `H_*_v` structs are assigned by name (GCC's 320 warnings are gone). The coupling by call order: M2, M5 and M6 are fixed; skip_miss no longer borrows the macroblock's record for the skip motion; what one family leaves for another is one block of members, each with what makes it and when it is reset, and `col_` is null once its record is rewritten; MbDecoder's head states the order the model drives the decoder in. Memory, the finding's last point: MbInfo takes 1,192 bytes for each macroblock of the current picture. MbCol takes 532 (498 at 547d495; M10's fix added the reference POCs) for each macroblock of the last picture of each kind a stream has; with its kind's two other records, 535 bytes, 4.2 MB a kind at 1080p. All 30 kinds at the largest pictures (139,264 macroblocks a frame, half that a field) would take 1.4 GB; the finding's "about 2 GB" counted the fields as frames. 261 bytes can ask for that much: test/crafted/kinds.264 is one slice of each kind at the largest size. Now `c` stores verbatim a slice that would take the kinds' records past 512 MB (c964f92). The format and `d` are unchanged, and so is the output for any stream within the bound, which takes in any stream of pictures up to 4096x2304 (all 30 kinds: 376 MB). kinds.264 now peaks at 683 MB at `-1`, against 1,606 MB before, and the test suite checks it within 1.05 GB of address space (without the bound it takes 1.64 GB). MB and GB here are 2²⁰ and 2³⁰ bytes, as in the bound |
| A3 | fixed: `Dpb` (the pictures, their marking, POC, lists and weights); on it `MbDecoder<K>`, the macroblock decoder, whose head states the order the model drives it in; on that `ReconT<K>`, the model's queries of the samples, whose using-declarations list what they read of the decoder; `PicMaps`, the energy and change maps, filled as a picture is finished; the kernels in pixel.inc. `K` is what a level has: ReconRefs (`ReconT<1>`) has the pictures' motion and the lists, and none of the samples' code or the search memo; ReconOff (`ReconT<0>`) none of either. The DPB holds the parser's bounds (32 references, 7 for the weights' denominators) itself |
| A4 | fixed: h264p.cpp is the command line; annexb.inc, cli.inc, nal.inc, tsvfile.inc, cmstream.inc, drivers.inc and levels.inc the rest |
| A5 | fixed: a level is its .idx files, one template, and an lv file of includes and flags; `mk.sh check` also compares a build without AVX2 |
| A6 | fixed: F1, F2 and F5; MTwrap's counts are read in one place (`mt_counts()`), and a failed pair's other stage is drained before its threads are joined (`mt_drain()`, since gone: see F2), MTwrap itself having no abort of its own |
| A7 | fixed: one `Output` (F3, F7) |
| A8 | fixed. The include order (no system header after Lib3's), `qword`, the namespaces of coro.inc and mt.inc. Warnings: h264p's own sources have none under GCC 13 and clang 18 with `-Wall -Wextra`, at `-O2` and `-O3`, with AVX2 and without, and neither has famtune.cpp at any level; what remains is in Lib3 and MTwrap_v3. At 547d495, GCC at `-O3` with AVX2 also gave 448 `-Wmaybe-uninitialized`, which the finding missed. They were false: bit_x sets and reads its counter vectors under one flag, `cn_v`, which GCC cannot tell holds in between. Reading the flag once (31cf885) ran 0.05% more instructions at `-1` and measured up to 3% slower there, so the warning is turned off around that code instead, for GCC only, and bit_x's machine code is 1c2b10b's again (10d2afd). The long functions are in parts: block (470 lines) in its families' stages (block_begin, block_proxy, block_col, block_count with cbf's variables, block_sig, block_levels, block_signs, sign1), mc_luma_p (199) in mc_halves_v, mc_halves and mc_mean, tm_search (174) in TmCx, TmThin and tm_cx, inter (132) in inter_mb and inter_b8, bit_x (156) in cnt_inputs, mix_inputs, mix_code and cnt_update, mvd (154) with mvd_pred and mvd_sign, qpd (124) with qpd_vars, qpd_aq and qpd_sign, compress_stream (118) with compress_nal and compress_parsed, and tm_miss (112) and code_slice_header (106) too. No function written by hand is 100 lines long (the longest, ipm, has 98); the index builders that idx2inc.pl generates into MOD/ are straight-line code of up to 100 lines (H9_mvs_make) |
| A9 | fixed: test/h264p.sh's negative and crafted cases |

### Any input restored (format 7)

After this review, `c` (without `-s`) had to take any input — cut short, with garbage after it, or no H.264 at all — and `d` had to restore it byte for byte, at every level, with and without `-t`, through files and pipes. Four readers mapped where that could fail (the front end, the stream layer, the model, the splitter and its parts). Six fuzzers then ran about 75,000 round trips, under release and UBSan builds: cut streams, garbage heads and tails, mutations, synthesized NAL streams, the splitter's cuts and copies, and pipes. The findings, and the deferred format-changing ones above, were fixed in these commits:

| commit | what |
|---|---|
| b6879b8 | format 7: the coder's end is exact. `d` accepted 1 or 2 bytes after a stream; now it fails on a byte cut off or any byte after the end |
| 953cd49 | model: Intra16x16's cbp recorded, end()'s skip window, the history matched by picture id (not POC), redundant slices kept out of the reconstruction and the records |
| c66f4ec | `-t`: a part that was only an access unit delimiter, with parameter sets copied in after it, could not be restored (`c` exited 0, `d` failed). `d` now removes the copies by the place and length the record gives; `-t`'s messages name the stream's NAL unit |
| 5d9ae89 | `-t`: a stretch with no NAL unit to cut at is cut at twice the cap. A part's sizes are 64-bit, so a part of 4 GB or more no longer fails |
| 137f3ee | POC type 1's expected POC saturated (int64 overflow); x264's padding frame coded mod 32, the range `d` accepts |
| 2cc6468 | `-t` copies into a part only the parameter sets its slices use (unused large sets made a crafted stream 25 times its size) |
| d1c9f11 | a skipped MBAFF top macroblock's motion derived in the field mode the pair decodes with |
| 275e74e | a closed pipe (SIGPIPE) removes the temporary output |
| 9d82a83 | an output named `/dev/stdout`, `/dev/fd/N` or `/proc/self/fd/N`, or a link to one, is written through that descriptor. It had been renamed over the file the shell had open, losing what was there and the next call's output, and then renamed over `/dev/stdout` itself |

Each fix of a failure (b6879b8, c66f4ec, 5d9ae89, 2cc6468, 275e74e, 9d82a83) has a test in test/h264p.sh, or a crafted stream (aud_last.264), that fails on the build before it; the model's fixes change only how streams compress. What is left: `c`'s restart (a slice the checked model pass cannot reproduce) has been reached by no input, crafted or fuzzed, so only reading covers it. (The splitter's memory: the next section.)

### Bounded memory (format 8)

Then one call had to take an endless stream (petabytes through a pipe) in memory that does not grow with it: what the level, the options and the largest pictures need, and no more. Six readers mapped what grows or counts with the stream: the splitter and `-t`, the stream layer, the model, the reconstruction, every integer, and peak RSS on long synthetic streams. What they found, in these commits:

| commit | what |
|---|---|
| e343553 | the splitter kept every version of every parameter set, and listed every set of a run with no slice: trimmed at each cut, and the keys a fixed set |
| 13d07ae | format 8: the reader held a NAL unit, the junk (a stream with no start code: all of it) and each run of zeros whole, and `c` made more copies. Now it holds 64 KB, counts zeros, and a unit over 64 MB is stored verbatim as it is read, in 1 MB chunks. Counts that `c` coded and `d` refused past 2^40 now go in pieces |
| 97ee2fd | counters: the walk's slice ids wrapped at different slices in `c` and `d` (a round trip broken, shown with the count started near 2^32); the range coder's run of 0xFF bytes; the picture and snapshot stamps; the header model's run position (and its map); the AQ fit's sums in a picture that never ends; the reconstruction's slice number; `-v`'s counters (32-bit `long` on Windows) |
| 37396c8 | the raw bytes' match kept up to 256 MB of them a model: now the last 16 MB, in a ring |
| 65c5903 | POCs saturated at 2^30 (after 32,770 pictures of a conforming stream, or 2^29 frames with no IDR), and B lists then went wrong: now modulo 2^32, compared by their differences |

The corpus compresses to the same bytes as before at all five levels, but for the version byte and crafted/poc_wrap.264 (5 or 6 bytes smaller at `-3` to `-9`). Peak RSS before and after: 400 MB of zeros between two NAL units, `c` 395 MB, now 4.4 MB; a 160 MB NAL unit, 473 MB, now 87 MB; 80 MB of distinct parameter sets through `-t 2 -c 1k`, 115 MB, now 12 MB. test/h264p.sh checks the zeros and the parameter sets within an address-space limit that the build before passes, and a 68 MB NAL unit's round trip. Instructions run (c -1 of 002/00, c -5 and -9 of 06, d -1 of 00) against 9d82a83: +0.06%, -0.03%, +0.01%, +0.18%.

What stays bounded by the options rather than fixed: a `-t` part is held whole by `c` (at most twice the cap: 32 MB by default, 4 GB at most), and so is the splitter's list of the part's slices (16 bytes each).

A second round reviewed those commits, the video and progress reporting (`9eb0a18`, `96c6a2e`: the codec tells, the front end shows), and what followed, and re-measured. Its fixes:

| commit | what |
|---|---|
| 96c6a2e | `-v` repeated the video line at each switch of a tool its slices use (a stream that goes back and forth between frame and field pictures, or between PPSs: `crafted/kinds.264`, 6 times in 261 bytes). VideoInfo keeps the tools as the sets used since its sequence last changed, so each is told once; `-t -v` said no video at all, now it does |
| e74b831 | `d` held more of a stream than `c` makes of one on crafted input: a NAL unit it parses past 64 MB (an SEI's messages), or a `-t` part larger than its record or than `-c` lets `c` make. Each is broken as it gets there |
| 8dd6072 | the model's "no POC" sentinel (INT_MIN) still reached a distance, on a picture that begins with a redundant slice: `History::picture()` takes the POC from a later slice. Compression only; `crafted/redundant.264` |
| def81c0 | format 9: a verbatim chunk of 64 KB or more that would cost the model 8 bits a byte is coded as it is, a byte a uniform step of the coder (`Rangecoder::rc_Byte()`). 16 MB of random bytes: +0.116% and 3.4 s became +0.0003% and 0.2 s. The stream is still one coder run |
| c86afa6, f349114 | `d -t` held memory that grew with the stream when the parts' kinds alternated (a worker's small records came out 64 KB at a time, held until then), and on crafted containers (a 64 KB node a block, a whole record read before its model). `c` ends each part's record as a round; `d` streams each part and packs the blocks it reads past; MTwrap out of memory ends through `err_fatal()` |
| f253196 | `random()` held nearer what the model does, on order-1 data, long-distance matches and locally skewed bytes, so synthetic streams built to slip past it are not coded worse than before format 9 |
| 5cba52a | format 10: a `-t` record carries its part's place and a CRC-32 of its header, and each worker's stream ends with a record of its own, so `d` finds a part lost, added or moved, a stream cut where a part ends, or a record changed (ins/xlen among them), which each part's own CRC does not; `d -t` hands a worker's parked blocks back before reading more, so its memory stays flat on a stream with no start codes |

A completeness critic and lossless fuzzing (about 16,700 round trips) found no round-trip failure in format 8 or 9; every finding above was reproduced on the build it names and verified. The corpus compresses to the same bytes as before at all five levels, but for the version byte (now 10). `d -t` peak RSS is flat in the stream's length (about 150 MB at 128 and 512 MB in, `-t 4 -c 1`). What a crafted `-t` container can still make `d` hold is bounded by the bytes it has read, not the stream: `d` reads past a worker's blocks to reach the next part in turn, and `c`'s are within about a round of each other.
