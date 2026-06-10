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

`zipcl` (on the [`claude/great-ride-ohbguu`](https://github.com/shelwien1/pe2elf1/tree/claude/great-ride-ohbguu)
branch, extended here with a `g` mode — see [`zipcl-gen.patch`](zipcl-gen.patch))
walks the encrypted archive, harvests known-plaintext, and emits a CBMC C++
model in which a chosen set of key bits is left unknown. CBMC bit-blasts it to
CNF and kissat recovers the bits.

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
free bits): the key schedule's constant-multiply carries are nonlinear, so this
scales to a "demystify a handful of bits" demo, not (yet) the full 96-bit state.

## 7. Reproducing

```sh
# build kissat (see ../sudoku-cbmc-linux/kissat.md), have cbmc on PATH, then:
KISSAT=/path/to/kissat ./solve_zipcrypt.sh models/model8.cpp         8 92
KISSAT=/path/to/kissat ./solve_zipcrypt.sh models/model16.cpp        16 0892
KISSAT=/path/to/kissat ./solve_zipcrypt.sh models/model8_deflate.cpp 8 92
```

To regenerate the models from the archive, apply `zipcl-gen.patch` on the
`great-ride` branch, rebuild (`g++ -O2 -std=gnu++17 -DNDEBUG -ILib3 -fpermissive
zipcl.cpp -o zipcl`), then e.g.
`./zipcl g basis.pak model8.cpp 8 a3e30892 f9185194 eb474b09 2 1`.

## 8. Files

- `zipcrypt.md` — this document.
- `solve_zipcrypt.sh` — cbmc → kissat → decode `UNK`, with optional verification.
- `models/model8.cpp`, `models/model16.cpp` — recover low 8 / 16 bits of `k0`
  from CRC-byte + deflate constraints.
- `models/model8_deflate.cpp` — recover low 8 bits from deflate bits only.
- `zipcl-gen.patch` — the `g`-mode addition to `zipcl` (`zipcl.cpp`,
  `zipdump.inc`) on the `great-ride` branch.

## 9. References

- PKWARE APPNOTE, ZipCrypto / Traditional Encryption (decryption header, check
  byte = `crc >> 24`).
- RFC 1951 (DEFLATE): block header `BFINAL` + `BTYPE`.
- Biham & Kocher, *A Known Plaintext Attack on the PKZIP Stream Cipher* — the
  classic full-state recovery this SAT demo is a small, bit-level cousin of.
- CBMC `--dimacs`, kissat — see [`../sudoku-cbmc-linux/kissat.md`](../sudoku-cbmc-linux/kissat.md).
