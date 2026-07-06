# stegdict — permutation steganography over sorted dictionaries

`stegdict` hides an arbitrary payload inside the **record ordering** of one or
more "dictionary" files, then recovers it. Reordering the records of a
dictionary does not change the set of records it contains, so the permutation
is a free side channel. `stegdict` fills that channel with a secret payload
using a range coder run "backwards."

- Source: `stegdict.cpp` plus the include graph in §9.
- Build: `make` (Linux) or `g.bat` (Windows/MinGW); round-trip test: `t.sh` / `t.bat`.

---

## 1. Concept

A *dictionary* is a file cut into **records** — either fixed-length (`N` bytes
each) or newline-terminated. Sorting the records gives a canonical order; the
file as stored is one particular permutation of that canonical order.

For `N` records with distinct values, there are `N!` orderings. If a value
repeats, orderings that only swap equal records are indistinguishable, so the
number of *distinguishable* arrangements is

```
N! / (k1! * k2! * ... )        kj = multiplicity of the j-th distinct record
```

That many arrangements carry `log2( N! / prod(kj!) )` bits of hidden capacity
per dictionary. `stegdict` uses exactly this capacity, and no more.

The design's trick is to **invert the usual compressor roles**: the payload is
treated as if it were a *compressed bitstream*.

| user command | what it does | range coder runs as |
|---|---|---|
| `c` (conceal) | payload + original dictionaries → reordered ("mutated") dictionaries | **decoder** |
| `d` (extract) | mutated dictionaries → recovered payload + restored (sorted) dictionaries | **encoder** |

Concealing **range-decodes** the payload into a sequence of permutation
choices; extracting **range-encodes** those choices back into the payload. A
range coder driven this way turns the payload's bits into (near) uniform
permutation indices and back, losslessly. (In the code this inversion is the
`Rangecoder_SH1m128<1-ProcMode>` in `model.inc`: mode `c` builds the range
coder in decode mode, mode `d` in encode mode.)

---

## 2. Building

The Linux `Makefile` mirrors `g.bat` (drops the Windows-only `-D_WIN32`, keeps
everything else):

```
make          # build ./stegdict, statically linked
make test     # build, then run t.sh
make clean
```

`-static` is deliberate: in the bundled test the binary is used as its **own
payload**, so it must be larger than the dictionaries' combined capacity
(~96 KB). A static build is ~800 KB; a dynamic one would be too small.

---

## 3. Command-line syntax

```
stegdict <mode><recsize>  @<listfile>              <payload>     (batch form)
stegdict <mode><recsize>  <input-dict> <output-dict> <payload>   (single-file form)
```

The form is chosen by whether the second argument begins with `@`.

### 3.1 `<mode><recsize>` (argv[1])

- **First character = mode.** The test in the code is literally
  `f_DEC = (argv[1][0]=='d')`:
  - `d` → **extract** (recover payload from ordering).
  - anything else → **conceal**; `c` is the conventional letter.
- **Remaining characters = default record size**, parsed with `atoi`:
  - `0` or absent → variable-length records, each terminated by `\n`.
  - `> 0` → fixed-length records of that many bytes.

Examples: `c` (conceal, newline records), `c4` (conceal, 4-byte records),
`d` (extract, newline records). In batch form the per-line `#N:` prefix
overrides this default, so bare `c` / `d` are what you normally use there.

### 3.2 `<payload>`

- Mode `c`: opened **for reading** — this is the secret being embedded.
- Mode `d`: opened **for writing** — this receives the recovered secret.

Only as many payload bytes as the dictionaries' combined capacity are
meaningfully consumed (`c`) or produced (`d`). If the payload is *longer* than
the capacity, only its leading capacity-worth of bytes is embedded and the rest
is ignored — the bundled test relies on exactly this. (If it is *shorter*, the
driver feeds `0xFF` padding once the file ends so the permutation is still fully
determined, but that case is not exercised here.)

### 3.3 Batch list file (`@<listfile>`)

One dictionary per line:

```
[#recsize:]<input-dict>  <output-dict>
```

- Tokens are whitespace-separated; a name containing spaces may be `"quoted"`.
- The optional `#N:` prefix sets **that** dictionary's record size (default `0`
  = newline-terminated). It overrides argv[1]'s record size.
- `<input-dict>` is read, `<output-dict>` is written.
- Blank lines are ignored.
- **All dictionaries in one list share a single continuous payload stream** —
  the range coder is initialized once before the first dictionary and flushed
  once after the last (see §5.5).

Bundled `list1` (embed direction — the `.bin` files are 4-byte binary records,
the `.txt` files are newline-terminated):

```
 #4:fp_log/fp_ip.bin  fp_mut/fp_ip.bin
 #4:fp_log/fp_tim.bin fp_mut/fp_tim.bin
    fp_log/fp_get.txt  fp_mut/fp_get.txt
    fp_log/fp_htp.txt  fp_mut/fp_htp.txt
    fp_log/fp_ref.txt  fp_mut/fp_ref.txt
    fp_log/fp_ua.txt   fp_mut/fp_ua.txt
    fp_log/fp_url.txt  fp_mut/fp_url.txt
```

> The shipped `list1`/`list2` use Windows `\` separators. `t.sh` rewrites them
> to `/` at run time for Linux; the originals are left untouched.

### 3.4 Single-file form

```
stegdict c16 in.dict out.dict payload
```

One dictionary, record size taken from argv[1] (`16` here).

### 3.5 Exit codes

| code | meaning |
|---:|---|
| 0 | success |
| 1 | too few arguments |
| 2 | cannot open input dictionary |
| 3 | cannot open output dictionary |
| 4 | cannot open payload |
| 5 | out of memory (dictionary buffer) |
| 6 | cannot open list file |
| 7–11 | out of memory (list / name tables) |

---

## 4. Data model (`Model::Init`, `model.inc`)

For each dictionary, the whole file is read into memory (`f_ptr`, `f_len`) and
split into `Nwords` records:

- **Newline records** (`rec_size == 0`): a record is a run of bytes ending in
  `\n` (the trailing `\n` is part of the record); the final record need not be
  terminated.
- **Fixed records** (`rec_size == N`): `Nwords = ceil(f_len / N)`; the last
  record may be short.

`word_ofs[i]` / `word_len[i]` describe record `i`. Two more structures are
allocated: an order-statistics tree `T` (§5.4) and `idx_map` (the permutation),
alongside a `f_dup` flag array (§5.2).

---

## 5. Algorithm

### 5.1 Establishing the canonical order

The **canonical order is sorted order.** The bundled reference dictionaries are
already stored sorted (verified: every `fp_log/*` file is in ascending record
order), which is what lets `d` reproduce them exactly — see §8.

`do_process` first sorts the records (`sh_qsort`, a median-of-3 quicksort with a
selection-sort cutoff for short spans, `Lib3/sh_qsort.inc`). The comparison is a
byte-wise lexicographic compare of record contents, with the record's slot index
as a final tie-breaker so the ordering is **total** (quicksort sees no equal
keys); byte-wise-equal records are grouped and then handled by §5.2 (`CMP::c` in
`model.inc`).

The two modes sort differently:

- **`c` (conceal):** the input file *is* the canonical (sorted) dictionary, so
  the sort physically reorders `word_ofs`/`word_len` in place (`idx_type==1`),
  leaving `idx_map` as the identity for now.
- **`d` (extract):** the input file is the permuted dictionary, so the sort
  instead builds a permutation array `idx_map` where `idx_map[k]` = file position
  of the `k`-th smallest record (`idx_type==0`) — i.e. "the permutation that
  sorts this file," which is exactly the hidden information.

### 5.2 Duplicate runs

After sorting, consecutive equal records are marked:
`f_dup[i] = (record i == record i-1)`. Runs of equal records must be assigned
*increasing* positions so that encoder and decoder agree on a single canonical
arrangement (this is what collapses `N!` down to `N!/prod(kj!)`).

### 5.3 The permutation as a sequence of ranks

A permutation of `N` items is coded in the **factorial number system** (Lehmer
code). Process records in sorted order `i = 0 .. N-1`; for record `i` there are
`N-i` output positions still free, numbered `0 .. N-i-1` by their rank among the
remaining positions. The chosen rank identifies the position. The product of the
ranges `N·(N-1)···1 = N!` is the number of permutations.

Duplicate handling uses two running values: `base` (the rank chosen for the
previous record of the current equal-run, `0` at the start of a fresh run) and
`rlen` (how many equal records still follow). The coded value `f` is drawn from
`[0, N-i-base-rlen)` and the actual rank is `base + f`. The `base` offset keeps
`rank >= base`; because the previous run-member's position was just removed,
reusing rank `base` now points to the position *after* it — so **actual
positions strictly increase within a run**. Subtracting `rlen` reserves room for
the equal records still to come. Together these collapse the `k!` orderings of a
`k`-run to one, shrinking the code space from `N!` to `N!/prod(kj!)`.

**Conceal (`c`)** — range coder in *decode* mode, reading the payload:

```
for i in 0..N-1:
    if f_dup[i] == 0:  base = 0;  rlen = (count of following equal records)
    else:              rlen -= 1               # base carried over within the run
    if N-i <= 1:
        rank = 0                               # only one position left: forced
    else:
        total = N - i - base - rlen
        f = (FFNum > 8) ? 0 : rc_GetFreq(total)   # FFNum stays 0 while decoding,
        rc_Process(f, 1, total)                   #   so this is just rc_GetFreq(total)
        rank = base + f
    base = rank
    pos = T.Lookup(rank)     # rank among free positions -> actual position
    idx_map[pos] = i         # canonical record i lands at file position pos
    T.Remove(pos)
# then emit the reordered dictionary: for each position i, write record idx_map[i]
```

**Extract (`d`)** — range coder in *encode* mode, writing the payload:

```
for i in 0..N-1:
    if f_dup[i] == 0:  base = 0;  rlen = (count of following equal records)
    else:              rlen -= 1
    if N-i > 1:
        pos  = idx_map[i]        # where the i-th canonical record sits in the file
        rank = T.Translate(pos)  # its rank among free positions
        rc_Process(rank - base, 1, N - i - base - rlen)   # encode into the payload
        T.Remove(pos)
        base = rank
    write the i-th canonical record to the restored dictionary
```

(The `FFNum > 8` guard in the source is inert during conceal: `FFNum` is only
advanced by the range coder's *encoder* path, so while decoding it never leaves
`0` and `rc_GetFreq` always runs. The last record needs no coding — with one
position left there is nothing to choose.)

### 5.4 Order-statistics tree (`idx_tree`, `idx_tree.inc`)

`T` is a binary-indexed tree over the output positions. Each internal node
stores the number of still-available leaves in its left subtree. It provides, in
`O(log N)`:

- `Remove(pos)` — mark a position used (decrement left-subtree counts along the
  root→leaf path wherever the path goes left).
- `Lookup(rank)` → `pos` — map a rank among the *remaining* positions to the
  actual position (used by `c`).
- `Translate(pos)` → `rank` — the inverse (used by `d`).

Total cost per dictionary is `O(N log N)`.

### 5.5 Multiple dictionaries share one stream (`f_cont`)

In batch mode, `main` calls `Process` once per line with a 2-bit `f_cont`:
bit 0 = "not the first dictionary", bit 1 = "not the last." In `do_process`:

- first dictionary → `rc_Init()` (start the coder);
- later dictionaries → skip `rc_Init`, keeping the coder state; the saved
  payload read position (`inpptr`/`inpend`) is restored so `c` keeps reading the
  same stream;
- last dictionary → `rc_Quit()` (flush; a no-op in the decode/`c` direction).

So one payload stream flows across every dictionary in the list; capacity is the
**sum** of the per-dictionary capacities.

---

## 6. The range coder (`sh_v2f.inc`)

`Rangecoder_SH1m128<f_DEC>` is a 32-bit, carry-aware range coder (Subbotin /
Schindler lineage), bit-granular internally with byte-granular I/O:

- State: `low`/`Carry` (as a 64-bit `lowc`), `range`, `FFNum` (pending carry
  bytes), `Cache`, `bits` (an 8-bit packing register seeded with a `0xFF`
  sentinel), and `code` (the decoder's window).
- `rc_GetFreq(tot)` (decode) returns `code * tot / range`; `rc_Process` narrows
  the interval to the chosen symbol (subtracting from `code` when decoding,
  adding to `low` when encoding) and renormalizes, shifting `range` left while
  it is below `2^31` and moving one bit through `inpbit`/`outbit`.
- `inpbit`/`outbit` pack 8 bits per byte using the `0xFF` sentinel — which is
  why the coder's **final output byte can be partial** (fewer than 8 meaningful
  bits).
- `muldivR`/`mulRdiv` use a 64-bit (`qword`) intermediate, so results are
  **identical on any x86/x86-64 target** — the round-trip is bit-exact and
  machine-independent apart from that partial final byte.

---

## 7. Coroutine I/O plumbing (`coro3b.inc`, `coro_fp2.inc`)

`Model::do_process` runs as a **stackful coroutine** so it can pause whenever its
input buffer drains or its output buffer fills, return to the driver for file
I/O, and later resume mid-computation. `CoroFileProc::processfile` is the driver:

```
loop:
    r = coro_call(model)
    r == 1 : need input  -> fread payload into inpbuf (on EOF set f_quit,
                            feed 8x 0xFF as padding)
    r == 2 : output full -> flush outbuf to the output dictionary, re-arm
    r == 0 : done        -> final flush, exit
```

The `get()`/`put()` "pins" are the byte streams: in mode `c`, `get()` pulls
payload bytes and `put()` writes the mutated dictionary; in mode `d` the restored
dictionary is written directly with `putc` while the range-encoded payload flows
out through the coroutine's output pin.

Context switching is `setjmp`/`longjmp` plus a saved copy of the live stack
(`Coroutine::stk`). The mapping is platform-specific:

- **gcc** — `coro3_setjmp_x64.h`: `__builtin_setjmp` / `__builtin_longjmp`
  (chosen to dodge an `-O2`/`-Ofast` miscompile of an older inline-asm version;
  `-static -Ofast` is fine).
- **clang** — `coro3_setjmp_x64d.h`: inline asm saving `rip`/`rsp`.
- **32-bit** — `coro3_setjmp_x32.h`.

---

## 8. Worked example: the bundled round-trip test

`t.sh` (Linux port of `t.bat`) exercises a full conceal → extract cycle using
the program's **own binary** as the secret:

```sh
stegdict c @list1 stegdict     # hide stegdict inside fp_log/* -> fp_mut/*
stegdict d @list2 payload      # recover it -> payload   (+ fp_unp/* restored)
cmp payload stegdict           # payload == first 96294 bytes of stegdict
```

Cover set (an SFC access-log "fingerprint" dump under `fp_log/`): 7 dictionaries
— `fp_ip.bin` (4978 × 4-byte records), `fp_tim.bin` (47330 × 4-byte), and five
newline-terminated text files (`fp_get`, `fp_htp`, `fp_ref`, `fp_ua`, `fp_url`).

Result:

- **Capacity = 96294 bytes** (the sum of the seven dictionaries' permutation
  entropies). The recovered `payload` is exactly this size.
- `payload` equals the **first 96294 bytes** of the `stegdict` binary. Its first
  96293 bytes match exactly; the final byte is the range coder's **partial flush
  byte** (§6) and carries fewer than 8 meaningful bits, so it is not compared.
- `fp_mut/*` are the reordered dictionaries — same records, permuted order.
  `fp_unp/*` come back **byte-identical** to `fp_log/*`: `d` restores canonical
  (sorted) order, and the `fp_log/*` references are themselves stored sorted, so
  restored == original.

---

## 9. Source map

| file | role |
|---|---|
| `stegdict.cpp` | `main`: arg / list parsing, per-dictionary `Process` driver |
| `model.inc` | `Model`: record parsing, sort, duplicate runs, permutation ⇄ rank coding |
| `idx_tree.inc` | `idx_tree`: order-statistics tree (`Lookup`/`Translate`/`Remove`) |
| `sh_v2f.inc` | `Rangecoder_SH1m128`: 32-bit carry-aware range coder |
| `coro_fp2.inc` | `CoroFileProc::processfile`: buffered file I/O driver loop |
| `Lib3/coro3b.inc` | stackful coroutine core (`yield` / `coro_call`) |
| `Lib3/coro3_pin*.inc` | `get`/`put` stream "pins" over the coroutine buffers |
| `Lib3/coro3_setjmp_x64.h` / `_x64d.h` / `_x32.h` | `setjmp`/`longjmp` mapping (gcc / clang / 32-bit) |
| `Lib3/sh_qsort.inc` | `sh_qsort`: quicksort used to canonicalize record order |
| `Lib3/common.inc` | typedefs, `flen`, `log2i`, compiler-portability macros |
| `list1` / `list2` | dictionary maps for the embed / extract halves of the test |
| `Makefile` / `g.bat` | Linux / Windows build; `t.sh` / `t.bat` round-trip test |

Included but **unused** by this build: `sh_v1m128.inc` (alternate range coder,
commented out), `muldiv_*.inc` (only used by `sh_v1m128`), `Lib3/file_api_std.inc`,
`Lib3/coro_fh*.inc`, `Lib3/coro2pair.inc`.
