# Pre-generated CNFs (forward 8-bit & 16-bit)

DIMACS CNF for the forward ZipCrypto key-recovery problems, generated from
`../models/` with `cbmc --error-label match --dimacs` and committed so a SAT
solver can be run directly without CBMC.

| file | recovers | vars / clauses | answer |
|---|---|---|---|
| `model8.cnf`  | low 8 bits of k0  | 33,671 / 156,826 | `0x92` |
| `model16.cnf` | low 16 bits of k0 | 50,514 / 235,338 | `0x0892` |
| `model24.cnf` | low 24 bits of k0 | 50,538 / 235,450 | `0xE30892` |

`model24.cnf` is the hard one — no solver tried (kissat_inc, kissat 4.0.4,
cadical, ParKissat-RS) cracked it within 90 min (see `../zipcrypt.md` §9).

The instance is satisfiable iff the blanked key bits are consistent with every
file's known plaintext. Solve with any DIMACS solver:

```sh
cadical model8.cnf        # or kissat / kissat_inc / parkissat ...
```

To read the answer: the `c UNK#1 <var> <var> ...` comment maps `UNK`'s bits
(LSB first) to CNF variables; look up their truth values in the solver's `v`
lines and take the low N bits (`../solve_zipcrypt.sh` does this automatically).
