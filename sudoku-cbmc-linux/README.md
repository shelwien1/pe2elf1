# sudoku-cbmc — Linux port

A Linux port of the `solve.bat` workflow from
[Shelwien/sudoku-cbmc](https://github.com/Shelwien/sudoku-cbmc), a Sudoku
solver built on the [CBMC](https://www.cprover.org/cbmc/) bounded model
checker. CBMC is asked to find an assignment to a nondeterministic board that
satisfies every Sudoku constraint (each row/column/3×3 block is a permutation
of 1–9); the satisfying assignment it returns *is* the solution.

There are two solve scripts:

- **`solve.sh`** — CBMC does everything (bit-blast *and* solve), as in the
  original `solve.bat`.
- **`solve_kissat.sh`** — CBMC is used only as a *frontend* to emit CNF; the
  [kissat](https://github.com/arminbiere/kissat) SAT solver does the solving.
  (Built here against
  [kissat_inc](https://github.com/shaowei-cai-group/kissat_inc).)

## Requirements

```sh
sudo apt install cbmc      # provides `cbmc` (pulls in minisat); tested with 5.95.1
# g++ is also required (build-essential)
```

For `solve_kissat.sh` you also need a `kissat` binary. To build the
`kissat_inc` variant:

```sh
git clone https://github.com/shaowei-cai-group/kissat_inc
cd kissat_inc/sources && ./configure --competition && make -j   # -> build/kissat
```

## Usage

1. Edit the puzzle near the top of `sudoku.cpp` (`.` or any non-`1..9`
   character marks a blank cell):

   ```c
   char s[] =
   "...49...."
   "49625..13"
   "..1...9.4"
   "..9.7..2."
   "56.1.9.38"
   ".8..6.7.."
   "9.4...3.."
   "82..43691"
   "....85..."
   ;
   ```

2. Run the solver:

   ```sh
   ./solve.sh
   ```

   Output:

   ```
   -------------
   |738|491|562|
   |496|257|813|
   |251|836|974|
   -------------
   |349|578|126|
   |567|129|438|
   |182|364|759|
   -------------
   |974|612|385|
   |825|743|691|
   |613|985|247|
   -------------
   ```

## How it works

`solve.sh` runs CBMC on `sudoku.cpp` with `--error-label match`, so CBMC
searches for an input that reaches the `match:` label — reachable only when the
board is a valid solution. `--trace` makes it print a counterexample (the
solving assignment). The 81 cell values are pulled out of the trace, written to
`solution.inc` as a C array initializer, and `sudoku_chk.cpp` (which
`#include`s `solution.inc`) is compiled with `g++` and run to pretty-print the
grid.

## What changed from `solve.bat`

| Windows (`solve.bat`)             | Linux (`solve.sh`)        |
| --------------------------------- | ------------------------- |
| `.\cbmc64.exe` (CBMC 5.7, vendored) | `cbmc` (apt, CBMC 5.95)   |
| bundled `grep.exe` / `tail.exe`   | system `grep` / `tail`    |
| `cl.bat` → VC71 `c2.bat`          | `g++`                     |
| `del` / `sudoku_chk.exe`          | `rm -f` / `./sudoku_chk`  |

**Trace-extraction change.** CBMC 5.7 printed the whole nondeterministic
`stream` array as a single `stream={ 55, 51, ... }` line, which the original
pipeline scraped with `grep .stream | tail -1 | grep -oP "={ \d[^}]*}"`. CBMC
5.95 no longer emits a read-only `extern` array that way (only the
`stream[81]=1` flag write appears), so `solve.sh` instead recovers the 81 cell
values from the row-major reads `c = x.tbl[i*N+j]` (sudoku.cpp line 51), which
CBMC reports once per cell in order, and reassembles them into the same
`={ ..., 1 }` initializer. If you edit `sudoku.cpp` and that read moves, update
the line number in `solve.sh`.

## Solving with kissat instead of CBMC

See **[kissat.md](kissat.md)** for the full write-up (both the
`--external-sat-solver` and `--dimacs` approaches, plus the model-decoding
reference). In short, `solve_kissat.sh` keeps the exact same C model but lets
**kissat** do the solving; CBMC is reduced to a CNF compiler:

```sh
# kissat on PATH, or point at your build:
KISSAT=/path/to/kissat_inc/bin/kissat ./solve_kissat.sh
```

Pipeline:

1. **`cbmc --dimacs --outfile sudoku.cnf`** bit-blasts the model to CNF
   (~261k vars, ~748k clauses) instead of solving it. Crucially, CBMC's DIMACS
   output also writes `c <symbol#ssa> <var> <var> ...` *comment* lines mapping
   every bit of every program variable to a CNF variable number.
2. **`kissat sudoku.cnf`** solves the CNF and prints a model as `v <lit> ...`
   lines (positive literal = true). Exit code 10 = SAT, 20 = UNSAT.
3. **`awk`** reads the model back into the grid. The board is `byte stream[82]`;
   its first 81 bytes are the cells, 8 bits each, **LSB first**. The
   `c stream#1 ...` comment gives the CNF variable for each of those 648 bits,
   so we look each one up in kissat's model and rebuild the ASCII cell values
   into the usual `solution.inc` (`={ ..., 1 }`).
4. **`g++` + run** the checker, exactly as `solve.sh` does.

Why this works: with `--error-label match`, the CNF CBMC emits is satisfiable
*iff* the `match:` label is reachable, i.e. iff the board is a complete valid
Sudoku — so any model kissat finds decodes to a solution. The decoded
`solution.inc` is byte-identical to the one `solve.sh` produces. The `stream#1`
SSA version is the initial nondeterministic array; since the board (`tbl`) is
only read, never written, those are the bits the constraints pin down. If you
restructure `sudoku.cpp` so `stream` is written before being read, decode a
later `stream#N` instead.

## Files

- `solve.sh` — CBMC solves directly (Linux port of `solve.bat`).
- `solve_kissat.sh` — CBMC emits CNF, kissat solves, awk decodes the model.
- `kissat.md` — guide to using kissat as the SAT backend for CBMC.
- `sudoku.cpp` — the CBMC model; holds the puzzle and the constraints.
- `sudoku_chk.cpp` — includes the extracted `solution.inc` and prints the grid.
- generated / git-ignored: `solution.inc`, `sudoku_chk` (both scripts),
  `sudoku.cnf`, `sudoku.model` (`solve_kissat.sh`).
