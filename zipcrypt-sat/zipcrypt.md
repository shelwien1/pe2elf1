# Recovering ZipCrypto keys with SAT (CBMC + kissat)

This documents a working known-plaintext attack on the legacy PKWARE ZipCrypto
("Traditional Encryption") stream cipher, solved with a SAT solver. The pipeline
reuses the CBMC→kissat technique from
[`../sudoku-cbmc-linux/kissat.md`](../sudoku-cbmc-linux/kissat.md):

```
zipcl g  ──►  model.cpp  ──►  cbmc --dimacs  ──►  model.cnf  ──►  kissat  ──►  key bits
(archive +     (CBMC                (CNF)              (SAT model)    (decode UNK)
 known key)     problem)
```

`zipcl` (Shelwien's tool from the
[`claude/great-ride-ohbguu`](https://github.com/shelwien1/pe2elf1/tree/claude/great-ride-ohbguu)
branch, vendored in [`zipcl/`](zipcl/) with an added `g` model-generation mode)
walks the encrypted archive, harvests known-plaintext, and emits a CBMC C++
model in which a chosen set of key bits is left unknown. CBMC bit-blasts it to
CNF and kissat recovers the bits. Two model shapes are explored: a **forward**
model (§3) and a faster **reverse** model (§7).

Tested with **CBMC 5.95.1** and **kissat 1.0.3** (kissat_inc) against
`basis.pak`, whose key is `k0 k1 k2 = A3E30892 F9185194 EB474B09`.

---

## 1. The cipher

ZipCrypto keeps three 32-bit registers and a per-byte keystream (`zipcrypt.inc`):

```c
update_keys(c): k0 = crc32(k0, c);
                k1 = (k1 + (k0 & 0xff)) * 134775813 + 1;
                k2 = crc32(k2, k1 >> 24);
decrypt_byte(): t = (k2 & 0xffff) | 2;  return (t * (t ^ 1)) >> 8;   // low byte
```

`crc32` is the standard reflected CRC-32 single-byte update. Each file is
prefixed by a 12-byte random **encryption header** ("salt"); decryption resets
the registers to the key, runs the 12 header bytes through
`p ^= decrypt_byte(); update_keys(p)`, then the body the same way. In `basis.pak`
the registers are reset to the *same* `k0/k1/k2` for every file (the
password-derived state is given directly), so every file is an independent
observation of the same key.

## 2. Known plaintext — what we can predict without the key

`zipcl d` prints, per file, the decrypted 12-byte header tail plus the first
decrypted body bytes as bit-strings. Two predictable sources fall out:

**(a) The CRC check byte — 8 bits/file.** ZipCrypto's last header byte decrypts
to the high byte of the file's CRC-32 (`crc >> 24`) when flag bit 3 is clear (all
entries here). We know the CRC from the local header, so we know that plaintext
byte. *(Older zips put the high 16 bits in the last two bytes; `basis.pak` uses
the 1-byte variant — verified: `pad[11]=0x19=crc>>24`, but
`pad[10]=0x10 ≠ (crc>>16)&0xff`.)*

**(b) The deflate block header — 3 bits/file.** A raw DEFLATE stream starts with
`BFINAL` (bit 0) and `BTYPE` (bits 1–2):

- `BFINAL = 1` when the whole stream is one block (true for every entry here —
  confirmed against the decrypted prefixes).
- `BTYPE` is predictable from the size pair: `cSize < uSize` ⇒ it compressed ⇒
  `BTYPE = 10` (dynamic Huffman); `cSize ≈ uSize` ⇒ stored ⇒ `BTYPE = 00`. Every
  entry in `basis.pak` is compressed, so the first body byte's low 3 bits are
  `101`.

So each file yields **8 + 3 = 11 known-plaintext bits**, all functions of the
(mostly known) key and the *known* ciphertext. The only free unknowns are the
key bits we deliberately blank.

## 3. The generated model

`zipcl g <archive> <model.cpp> <nunk> <k0> <k1> <k2> [nfiles] [usecheck]`
blanks the low `nunk` bits of `k0` (the nondeterministic `UNK`) and emits one
constraint block per file. For example `model8.cpp`:

```c
#define MASK 0x000000FFu                         // low 8 bits of k0 unknown
uint CRC32(uint crc, byte b){ crc^=b; for(int k=0;k<8;k++)
    crc=(crc>>1)^(0xEDB88320u&(0u-(crc&1u))); return crc; }   // see §5
void update_keys(byte c){ k0=CRC32(k0,c); k1=(k1+(k0&0xffu))*134775813u+1u;
                          k2=CRC32(k2,(byte)(k1>>24)); }
byte decrypt_byte(){ uint t=(k2&0xffffu)|2u; return (byte)((t*(t^1u))>>8); }
extern uint UNK;                                  // <- the SAT variables

int main(){
  __CPROVER_assume((UNK & ~MASK)==0u);            // only low 8 bits free
  // file 0: uSize=4852 cSize=1530 crc=19CFF2AC -> BFINAL=1 BTYPE=10
  k0=(0xA3E30892u&~MASK)|(UNK&MASK); k1=0xF9185194u; k2=0xEB474B09u;
  { static const byte ct[13]={0x96,0x44,...,0xBC}; byte pt=0;   // CIPHERTEXT
    for(int i=0;i<12;i++){ pt=ct[i]^decrypt_byte(); update_keys(pt); }
    __CPROVER_assume(pt==0x19u);          // (a) CRC check byte
    byte b0=ct[12]^decrypt_byte();
    __CPROVER_assume((b0&7u)==5u);        // (b) deflate BFINAL|BTYPE
  }
  // file 1: ... (same UNK, different ciphertext + predicted bits)
  match: {}   // reachable iff UNK matches every file's known plaintext
}
```

`UNK` is an undefined `extern` → CBMC treats it as nondeterministic. With
`--error-label match`, the emitted CNF is satisfiable **iff** `match` is
reachable, i.e. iff there is a value of the blanked bits that reproduces all the
predicted plaintext. `usecheck=0` drops constraint (a) to test deflate-only.

> The real key bits are written into the model header as a comment purely so the
> result can be checked; nothing in the constraints uses them.

## 4. Solve and decode

[`solve_zipcrypt.sh`](solve_zipcrypt.sh) runs the back half:

```sh
KISSAT=/path/to/kissat ./solve_zipcrypt.sh models/model8.cpp 8 92
#   recovered 8 key bit(s) = 0x92
#   expected 0x92  ->  MATCH
```

It runs `cbmc --dimacs`, then `kissat`, then decodes. Decoding is identical to
the sudoku example: CBMC writes a `c UNK#1 <var> <var> ...` comment mapping each
bit of `UNK` to a CNF variable (LSB first); we read each variable's truth value
from kissat's `v` model lines and reassemble the low `nunk` bits.

## 5. The one optimization that matters: bitwise CRC

The naive model is intractable. `zipcrypt.inc` computes CRC via a 256-entry
table; with unknown key bits the table index is symbolic, so CBMC bit-blasts a
256-way mux per call (~24 calls/file). That CNF is huge **and** SAT-hard:

| model (8 files, table CRC) | result |
|---|---|
| CNF size | **2.87M clauses** |
| kissat | **timeout (>300 s)** |
| CBMC's own MiniSat (2 files) | **timeout (>200 s)** |

CRC-32 is **linear over GF(2)**; the table hides that. Emitting it *bitwise*
(the shift/XOR loop above — mathematically the identical function, verified)
exposes the XOR structure SAT solvers thrive on:

| model (2 files, bitwise CRC) | result |
|---|---|
| CNF size | **156,826 clauses** (≈18× smaller) |
| CBMC's MiniSat | **0.6 s** |
| kissat | **0.66 s** |

Same problem, ~18× smaller CNF and from *timeout* to *sub-second*. This is the
crux of making ZipCrypto-via-SAT practical.

## 6. Results

All recovered values match the real key bits (`k0 = A3E30892`):

| model | known plaintext | files | vars / clauses | kissat | recovered | expected |
|---|---|---|---|---|---|---|
| `model8.cpp` | CRC byte + deflate | 2 | 33,671 / 156,826 | 0.66 s | `0x92` | `0x92` ✓ |
| `model16.cpp` | CRC byte + deflate | 3 | 50,514 / 235,338 | 60 s | `0x0892` | `0x0892` ✓ |
| `model8_deflate.cpp` | **deflate only** | 6 | 100,941 / 470,366 | 2.2 s | `0x92` | `0x92` ✓ |

The deflate-only run is the headline: the key bits are recovered from nothing but
DEFLATE block-header bits predicted from the size fields — no CRC, no body
plaintext. As a sanity check, a model with `nunk=0` (no unknowns) is satisfiable,
which confirms every predicted bit is actually correct for the real key.

Each file adds independent constraints on the *same* unknown bits, so a couple of
files over-determines 8–16 unknowns; more files = more redundancy and a unique
solution. Recovery cost grows steeply with `nunk` (0.66 s → 60 s from 8 → 16
free bits): the key schedule's constant-multiply carries are nonlinear. The
reverse model below cuts that cost.

## 7. Solving backward: the reverse model

ZipCrypto's step is invertible, so instead of running forward from the (mostly
known) init through the header and checking 11 plaintext bits at the *end*, we
can run **backward** from the body and require the recovered init to equal the
known key. Generate it with the trailing `reverse=1` flag
(`./zipcl g ... <nfiles> <usecheck> 1`).

Each step inverts cleanly given the state *after* it and the known ciphertext:

```c
// state_{i+1} -> state_i   (ct[i] known; MINV = 134775813^-1 mod 2^32)
uint b2 = CRC32inv(a2, (byte)(a1>>24));        // k2 was crc32(.,k1>>24)
byte c  = ct[i] ^ decrypt_byte(b2);            // recovered header plaintext byte
uint b1 = (a1-1u)*MINV - (a0 & 0xff);          // undo the k1 multiply
uint b0 = CRC32inv(a0, c);                      // undo the k0 crc
```

The model makes each file's **body-start state** `S[3*f..]` the free variable,
runs the 12 inverse steps, and asserts the recovered `(a0,a1,a2)` equals the
known key (low `nunk` bits exposed as `UNK`), plus the same deflate / check-byte
constraints. (`MINV = 0xD94FA8CD`.)

This is the better-conditioned direction. The forward model dangles 11 known
bits off the far end of a nonlinear chain; the reverse model pins **almost the
entire init** (96 − `nunk` bits) as a dense constraint, and `CRC32inv` is
GF(2)-linear, so it propagates backward strongly. Same archive, same key bits:

| nunk | direction | files | vars / clauses | kissat |
|---|---|---|---|---|
| 8  | forward | 2 | 33,671 / 156,826 | 0.66 s |
| 8  | **reverse** | 1 | 31,053 / 115,387 | **0.23 s** |
| 16 | forward | 2 | 33,687 / 156,898 | 100 s |
| 16 | **reverse** | 2 | 62,089 / 230,804 | **30 s** |

The reverse instance is ~3× faster and needs fewer files — *even though its CNF
is larger* (it adds 96 free state bits per file). The win is purely the
constraint structure, not formula size. Both still climb steeply with `nunk`
(the multiply carries stay nonlinear in either direction), so this remains a
"recover a handful of bits" demonstration rather than full 96-bit state recovery.

## 8. More deflate-header constraints

After `BFINAL`/`BTYPE`, a dynamic block (`BTYPE=10`) has three fixed-size
fields — `HLIT` (5 bits, `nlen=HLIT+257`), `HDIST` (5 bits, `ndist=HDIST+1`),
`HCLEN` (4 bits, `ncode=HCLEN+4`) — then `ncode` 3-bit code-length-code lengths.
These give extra *key-independent* constraints (the `g` mode's `deflevel` arg,
1 or 2). Two are sound for any decodable stream:

- **Validity ranges:** `nlen ≤ 286`, `ndist ≤ 30` (`ncode` is always valid).
- **Completeness:** the code-length code must be a *complete* Huffman code, so
  `Σ 2^(7-len_i) == 128` over the `ncode` lengths (Kraft equality; zlib rejects
  an incomplete one). All 50 files in `basis.pak` satisfy both (verified).

How much does each actually help? Brute-forcing all 256 low-bytes of `k0`
against **one** file ([`deflate_constraints.py`](deflate_constraints.py)):

| constraint added | candidates left (of 256) |
|---|---|
| `BFINAL=1 \| BTYPE=10` (3 bits) | 39 |
| + `nlen ≤ 286`, `ndist ≤ 30` | 37 |
| + code-length **Kraft == 128** | **1** (`0x92`) |

The validity *ranges are almost worthless* (39 → 37: each only forbids 2 of 32
values). The *completeness check is strong* — it collapses 39 → 1, uniquely
fixing all 8 unknown bits from a **single** file. So with Kraft, deflate-only
recovery (no CRC byte) needs far fewer files:

| model | known plaintext | files | kissat | recovered |
|---|---|---|---|---|
| `model8_deflate.cpp` | deflate `BFINAL\|BTYPE` only | 6 | 2.2 s | `0x92` |
| `model8_deflate_kraft.cpp` | + ranges + Kraft | **1** | 5.5 s | `0x92` |

The catch: Kraft needs the first ~12 body bytes modelled (vs 1), so each file's
CNF is bigger; for harder targets it can cost more than it saves (16-bit
deflate-only+Kraft, 2 files: 144 s). Its sweet spot is **minimising the number
of files** when the CRC check byte isn't usable. (The observed fields cluster
tighter still — `nlen∈[267,286]`, `ndist∈[16,30]`, `ncode∈[12,18]` — but those
are compressor-specific heuristics, not guarantees, so they're not encoded.)
Past these fixed fields the stream is variable-length Huffman data, so there's
little more to pin without actually decoding.

## 9. Scaling and limits

How far does SAT recovery go? The fastest configuration depends on the number of
unknown bits `N`, because of a crossover between the two models:

- The **reverse** model is ~2–3× faster at equal `N` — but it makes each file's
  whole 96-bit body-start state a free variable, so adding files is expensive
  (16 bits: **26 s at 2 files → 195 s at 3**). It needs `⌈N/11⌉` files for a
  unique answer (each file pins ~11 bits: 8 CRC + 3 deflate), so it's the method
  up to `N=22` (2 files).
- The **forward** model has only `N` free bits regardless of file count, so from
  `N=23` (where a 3rd file is forced and reverse blows up) it takes over.

Measured kissat times (`solve_zipcrypt.sh`):

| N | reverse, 2 files | forward, 3 files |
|---|---|---|
| 8 | 0.3 s | 1.1 s |
| 12 | 2.8 s | 5.2 s |
| 16 | 26 s | 65 s |
| 20 | ~4 min (fit) | 17 min |
| 24 | needs 3 files → explodes | _hours_ — see below |

Each fit `time ≈ A·2^(c·N)` gives **c ≈ 0.78–0.87**, i.e. **~80–130× per +8
bits**. Extrapolating the **fastest** combination (forward + cadical, see "Solver
choice" below; [`scaling.py`](scaling.py)):

| N | est. time (forward + cadical) | files |
|---|---|---|
| 24 | ~27 min | 3 |
| 32 | ~1.5 days | 3 |
| 40 | ~4 months | 4 |
| 48 | ~25 years | 5 |
| 64 | ~10⁵ years | 6 |
| 96 | ~10¹² years | 9 |

So with the best solver the practical ceiling is **~24–32 unknown bits** (24 is
now ~½ hour, not interactive but doable; ~32 would take days). The full 96-bit
ZipCrypto state stays astronomically out of reach — which is why real attacks use
the *structured* Biham–Kocher method (it recovers the whole state from ~13 known
plaintext bytes by exploiting the schedule's algebra) rather than brute-forcing
bits. SAT is a great demonstrator and a fine tool for *partial*-key / few-unknown
situations, not full recovery.

### Solver choice (kissat_inc vs kissat 4.0.4 vs cadical)

We started with kissat_inc 1.0.3, but the solver matters a lot — and the best one
depends on the model. Same CNFs, three solvers (8-bit is sub-second for all, so
it can't distinguish them; 16-bit does):

| 16-bit instance | kissat_inc 1.0.3 | kissat 4.0.4 | cadical 3.0.0 |
|---|---|---|---|
| forward (235k clauses) | 60 s | 137 s | **21 s** |
| reverse (231k clauses) | **28 s** | 129 s | 254 s |

- **cadical is fastest on the forward model**, and not just by a constant: on
  20-bit forward it took **185 s vs kissat_inc's 1042 s**, a *better exponent*
  too (×8.8 vs ×16 per +4 bits). cadical+forward overtakes the old best
  (reverse+kissat_inc) and, since forward avoids the reverse model's file
  explosion, it's the best choice as `N` grows — it's what makes 24 bits ~½ hour.
- **kissat_inc is fastest on the reverse model**; cadical is worst there (the
  reverse model's 96-free-bits-per-file structure doesn't suit it).
- **Upstream kissat 4.0.4 is the slowest on both** — its competition-tuned
  inprocessing is counter-productive on these small, highly structured crypto
  instances. Newer ≠ faster here.

**Build-level tuning doesn't move the needle.** Rebuilding cadical with
`clang -Ofast -march=haswell` and swapping in **mimalloc** (with 2 MB huge pages —
verified consumed, `HugePages_Free` dropped during the run) changed the 16-bit
forward solve by <1 % (20.2→20.0 s) and 20-bit by ~0 % (185→194 s, i.e. noise).
These solves are *algorithm-bound* — branchy propagation / conflict-analysis over
preallocated structures — not allocator- or codegen-bound, so a faster allocator
and vectorization flags have nothing to grab. Every real speedup here was
algorithmic (bitwise CRC §5, the reverse model §7, deflate constraints §8) or
solver choice — not micro-optimization.

`solve_zipcrypt.sh` takes a `SOLVER=` override (any DIMACS solver), so use
`SOLVER=/path/to/cadical` for forward models, kissat_inc for reverse.

## 10. Reproducing

```sh
# have cbmc on PATH and a SAT solver built (kissat: ../sudoku-cbmc-linux/kissat.md;
# or git clone + build github.com/arminbiere/{kissat,cadical}). Then e.g.:
SOLVER=/path/to/cadical ./solve_zipcrypt.sh models/model16.cpp            16 0892  # forward: cadical
SOLVER=/path/to/cadical ./solve_zipcrypt.sh models/model8_deflate_kraft.cpp 8 92   # §8
SOLVER=/path/to/kissat  ./solve_zipcrypt.sh models/rev8.cpp                8 92    # reverse: kissat_inc
SOLVER=/path/to/kissat  ./solve_zipcrypt.sh models/rev16.cpp              16 0892  # reverse
python3 deflate_constraints.py   # §8 constraint-strength analysis
python3 scaling.py               # §9 timing model / extrapolation
```

To regenerate the models, build the vendored tool (`zipcl/build.sh`) and run e.g.
`zipcl/zipcl g zipcl/basis.pak model8.cpp 8 a3e30892 f9185194 eb474b09 2 1 0 0`
(see [`zipcl/README.md`](zipcl/README.md) for all arguments).

## 11. Files

- `zipcrypt.md` — this document.
- `solve_zipcrypt.sh` — cbmc → SAT solver → decode `UNK` (`SOLVER=` selects any
  DIMACS solver), with optional verification.
- `deflate_constraints.py` — brute-force quantification of §8's constraint power.
- `scaling.py` — §9 timing fit + extrapolation across solvers/models.
- `models/model8.cpp`, `models/model16.cpp` — forward: recover low 8 / 16 bits of
  `k0` from CRC-byte + deflate constraints.
- `models/model8_deflate.cpp` — forward, deflate `BFINAL|BTYPE` only.
- `models/model8_deflate_kraft.cpp` — forward, deflate ranges + Kraft, 1 file (§8).
- `models/rev8.cpp`, `models/rev16.cpp` — reverse models (§7).
- `zipcl/` — the vendored, buildable `zipcl` with the `g` mode (`build.sh`,
  `basis.pak`, and `CHANGES.patch` showing the diff vs the upstream branch).

## 12. References

- PKWARE APPNOTE, ZipCrypto / Traditional Encryption (decryption header, check
  byte = `crc >> 24`).
- RFC 1951 (DEFLATE): block header `BFINAL` + `BTYPE`.
- Biham & Kocher, *A Known Plaintext Attack on the PKZIP Stream Cipher* — the
  classic full-state recovery this SAT demo is a small, bit-level cousin of.
- CBMC `--dimacs`, kissat — see [`../sudoku-cbmc-linux/kissat.md`](../sudoku-cbmc-linux/kissat.md).
