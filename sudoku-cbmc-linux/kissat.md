# Using kissat as the SAT solver with CBMC

[CBMC](https://www.cprover.org/cbmc/) is a bounded model checker for C/C++. To
decide a property it **bit-blasts** the program into a propositional formula
(CNF) and hands it to a SAT solver; if the formula is satisfiable, the solver's
model is decoded back into a counterexample trace. CBMC ships with MiniSat as
the built-in backend, but you can use [kissat](https://github.com/arminbiere/kissat)
(Armin Biere's "keep it simple" solver, and competition variants such as
[kissat_inc](https://github.com/shaowei-cai-group/kissat_inc)) instead.

There are two ways to do it:

| | Approach A — `--external-sat-solver` | Approach B — `--dimacs` |
|---|---|---|
| Who solves | CBMC calls kissat for you | you run kissat yourself |
| Who decodes the answer | CBMC (you get a normal trace) | you (decode the SAT model) |
| CNF stays on disk | no (temp file) | yes (`*.cnf`) |
| Good for | swapping the backend, keeping CBMC's trace | inspecting/transforming the CNF, custom pipelines |

Everything below was checked with **CBMC 5.95.1** and **kissat 1.0.3**
(kissat_inc) on Linux, using the Sudoku model in this directory as the example.

---

## Prerequisites

```sh
sudo apt install cbmc            # CBMC + MiniSat (the default backend)

# kissat (the kissat_inc variant):
git clone https://github.com/shaowei-cai-group/kissat_inc
cd kissat_inc/sources && ./configure --competition && make -j   # -> build/kissat
```

Put the resulting `kissat` on your `PATH`, or refer to it by full path below.

`--sat-solver NAME` selects among the backends *compiled into* CBMC (e.g.
`minisat2`); kissat is not one of them. To use an external binary you use
`--external-sat-solver` instead.

---

## Approach A — let CBMC drive kissat (`--external-sat-solver`)

This is the simplest: kissat becomes a drop-in replacement for MiniSat and you
keep CBMC's normal output (traces, `--dimacs`-free workflow, everything).

```sh
cbmc prog.c --external-sat-solver "$(command -v kissat)"
```

For the Sudoku model, ask for the satisfying assignment as a trace:

```sh
cbmc sudoku.cpp --error-label match --external-sat-solver /path/to/kissat --trace
```

CBMC reports it is using the external solver and you get exactly the same trace
you'd get from the built-in solver:

```
result:SATISFIABLE
Building error trace
[main.error_label.1] line 72 error label match: FAILURE
...
** 1 of 1 failed (2 iterations)
VERIFICATION FAILED
```

(`VERIFICATION FAILED` is the *goal* here: the `match:` label is reachable, i.e.
a solved board exists. See "What the CNF means" below.)

**How the protocol works.** CBMC writes the CNF to a temporary DIMACS file,
runs `<cmd> <tempfile>`, and reads the solver's **stdout** expecting the
standard SAT-competition format — a `s SATISFIABLE` / `s UNSATISFIABLE` line, a
`v <lit> <lit> ... 0` model, and exit code **10** (SAT) / **20** (UNSAT). kissat
emits exactly this, so no wrapper is needed. Any solver that follows the same
convention works the same way.

Because the trace is identical to a native CBMC run, **any extraction you
already have keeps working** — e.g. `solve.sh` in this directory just needs the
extra flag:

```sh
cbmc sudoku.cpp --error-label match --external-sat-solver /path/to/kissat --trace \
  | grep -A2 'line 51 thread' | grep -oP 'c=\K[0-9]+' \
  | paste -sd, | sed 's/,/, /g; s/.*/={ &, 1 }/' > solution.inc
```

---

## Approach B — CBMC as a CNF compiler (`--dimacs`), kissat solves

Here CBMC only *translates* the problem; you run kissat and decode its model
yourself. This is what `solve_kissat.sh` does. Use it when you want the CNF as
an artifact — to preprocess it, feed an incremental/portfolio solver, compare
solvers, or otherwise control the pipeline.

### 1. Emit the CNF

```sh
cbmc sudoku.cpp --error-label match --dimacs --outfile sudoku.cnf
```

```
$ grep -m1 '^p cnf' sudoku.cnf
p cnf 261308 747522          # 261,308 variables, 747,522 clauses
```

Note CBMC does **not** solve in this mode (`Runtime Solver: ~0.0001s`); it just
writes the file.

### 2. Solve with kissat

```sh
kissat sudoku.cnf > sudoku.model        # exit 10 = SAT, 20 = UNSAT
grep '^s ' sudoku.model                 # -> s SATISFIABLE
```

The model is printed as `v` lines, positive literal = true, negative = false:

```
v 1 2 3 4 -5 6 7 -8 -9 10 11 -12 -13 14 15 -16 ...
```

### 3. Decode the model back to program values

This is the only non-obvious step, and the key to the whole approach: **CBMC
embeds the variable mapping in the DIMACS file as comments.** Every bit of every
program variable gets a line:

```
c <symbol>#<ssa> <var0> <var1> <var2> ...
```

For the Sudoku board (`byte stream[82]`):

```
$ grep -m1 '^c stream#1 ' sudoku.cnf
c stream#1 2 3 4 5 6 7 8 9 10 11 ... 657
```

Those numbers are the CNF variables holding the array's bits. The rules:

- **8 bits per byte, least-significant bit first.** `stream`'s 82 bytes = 656
  bits; the first 81 bytes (648 bits, vars 2..649 here) are the board cells.
- Cell `c`'s value = `Σ bit_k · 2^k` for `k = 0..7`, where `bit_k` is the truth
  value kissat assigned to variable `bitvar[c*8 + k]`.
- A token may be the literal `TRUE` or `FALSE` instead of a number when CBMC
  constant-folded that bit (treat as 1 / 0). For these board cells they're all
  real variables.

Worked check for cell 0: vars `2 3 4 5 6 7 8 9`; kissat gives
`2,3,4,-5,6,7,-8,-9` → LSB-first bits `1,1,1,0,1,1,0,0` → `0b00110111` = `55` =
ASCII `'7'`. ✓

The `awk` decoder from `solve_kissat.sh`:

```sh
grep -m1 '^c stream#1 ' sudoku.cnf | cat - sudoku.model | awk '
    NR==1 { for(i=3;i<=NF;i++) bitvar[i-3]=$i+0; next }      # mapping line
    /^v / { for(i=2;i<=NF;i++){ l=$i+0; if(l>0) val[l]=1 } } # kissat model
    END {
      out="={ "
      for(c=0;c<81;c++){
        b=0; for(k=0;k<8;k++) if(val[bitvar[c*8+k]]) b+=2^k
        out=out b ", "
      }
      print out "1 }"
    }' > solution.inc
```

---

## Decoding reference (Approach B)

- **Comment format:** `c <symbol>#<ssa-version> <lit-or-const> ...`, one per
  program symbol version, listing its bits low-to-high.
- **Which SSA version?** A symbol appears once per static-single-assignment
  version (`stream#1`, `stream#2`, ...). Use the version whose bits the property
  constrains. In this model the board is only *read*, never written, so the
  initial nondeterministic version `stream#1` already holds the answer; `stream#2`
  differs only in byte 81 (the flag written at the end). If your program writes
  the variable before the interesting read, decode the later version instead.
- **Endianness:** bits are listed LSB-first; reassemble with `2^k`.
- **Constants:** `TRUE`/`FALSE` tokens are folded bits → 1/0.
- **Missing variables:** if a bit-variable never appears in the model it's a
  don't-care; default it to 0.
- **Discovering symbols:** `grep '^c ' sudoku.cnf` lists every mapped symbol
  (locals appear as `c main::1::...#N ...`).

---

## What the CNF means (and why SAT = solved)

The model uses `--error-label match`: CBMC checks whether the `match:` label is
reachable. The formula it emits is satisfiable **iff** that label is reachable —
which in this model happens exactly when the board is a complete, valid Sudoku
(`x.flag` true ⇒ every row, column and 3×3 block is a permutation of 1..9). So:

- `s SATISFIABLE` (kissat exit 10) ⇒ a solution exists, and its model decodes to
  the grid. CBMC prints `VERIFICATION FAILED` because, from the checker's point
  of view, the "assertion that `match` is unreachable" was violated.
- `s UNSATISFIABLE` (exit 20) ⇒ no solution (an unsolvable puzzle).

This "violation = answer" framing is the standard trick for using a model
checker as a solver; the SAT backend doesn't care which framing produced the
CNF.

---

## Worked example: the two scripts in this directory

- `solve.sh` — CBMC solves directly (built-in MiniSat). To route it through
  kissat instead, add `--external-sat-solver /path/to/kissat` (Approach A).
- `solve_kissat.sh` — Approach B end-to-end: `cbmc --dimacs` → `kissat` → `awk`
  decode → `g++` + run the checker. Invoke with:

  ```sh
  KISSAT=/path/to/kissat_inc/bin/kissat ./solve_kissat.sh
  ```

Both produce a byte-identical `solution.inc`, and the printed grid matches —
verified on the bundled puzzle and on the classic "hardest"/Wikipedia puzzle.

---

## Gotchas

- **`--external-sat-solver` needs `--trace`** (or `--json-ui`/`--xml-ui`) if you
  want the assignment back; without it CBMC only reports SAT/UNSAT.
- **`--dimacs` writes comments**, so feed the file to a solver that ignores `c`
  lines (kissat does). If you strip comments to shrink the file, keep a copy
  *with* comments for decoding.
- **Bit order is LSB-first** — the single most common decode bug.
- **Pick the right SSA version** when a variable is assigned more than once.
- **CNF size scales with loop unwinding.** This model unwinds the 9×9 loops
  fully (~261k vars / ~748k clauses); deeper/unbounded loops need `--unwind`
  and may blow up.
- **Solvers may return different models.** Any satisfying assignment is valid;
  if the puzzle has a unique solution they'll agree, otherwise they needn't.
- **kissat_inc** is built with `./configure --competition`; the resulting binary
  speaks the same DIMACS in / `s`+`v` out protocol as upstream kissat, so it
  works for both approaches.

---

## References

- CBMC: <https://www.cprover.org/cbmc/> · `cbmc --help` (`--dimacs`,
  `--outfile`, `--sat-solver`, `--external-sat-solver`)
- kissat: <https://github.com/arminbiere/kissat>
- kissat_inc: <https://github.com/shaowei-cai-group/kissat_inc>
- DIMACS CNF / SAT-competition output format (`p cnf`, `s`, `v`, exit 10/20)
