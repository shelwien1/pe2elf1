#!/bin/sh
# Solve the sudoku with kissat instead of cbmc.
#
# cbmc is used ONLY as a frontend: it bit-blasts the C model (sudoku.cpp) into
# a CNF/DIMACS file. kissat then does the actual SAT solving, and we decode its
# model back into the grid. (Compare solve.sh, which lets cbmc solve directly.)
#
# Pipeline:
#   1. cbmc --dimacs   : C model  -> sudoku.cnf  (incl. var-mapping comments)
#   2. kissat          : sudoku.cnf -> SAT model
#   3. awk             : model -> solution.inc   (decode the `stream` array)
#   4. g++ + run       : pretty-print the solved grid
#
# Override the tools via env, e.g. KISSAT=../kissat_inc/bin/kissat ./solve_kissat.sh
CBMC=${CBMC:-cbmc}
KISSAT=${KISSAT:-kissat}
CNF=sudoku.cnf
MODEL=sudoku.model

set -e

# 1. Translate the problem to CNF. CBMC's DIMACS output carries
#    `c <symbol#ssa> <var> <var> ...` comments that map every bit of every
#    program variable to a CNF variable number -- that's what lets us read the
#    answer back out of a plain SAT model.
"$CBMC" sudoku.cpp --error-label match --dimacs --outfile "$CNF" >/dev/null 2>&1

# 2. Solve with kissat (exit 10 = SAT, 20 = UNSAT). The model is printed as
#    `v <lit> <lit> ...` lines (positive lit = true, negative = false).
set +e
"$KISSAT" "$CNF" > "$MODEL" 2>/dev/null
set -e
grep -q '^s SATISFIABLE' "$MODEL" || { echo "kissat: UNSATISFIABLE (no solution)"; exit 1; }

# 3. Decode. The board is `byte stream[82]`; its first 81 bytes are the cells,
#    8 bits each, LSB first. The `c stream#1 ...` comment lists the CNF variable
#    for each of those bits (vars for bits 0..647 = the 81 cells). Look each one
#    up in kissat's model and rebuild the ASCII cell values, emitting the same
#    `={ ..., 1 }` initializer solution.inc has always used.
grep -m1 '^c stream#1 ' "$CNF" \
| cat - "$MODEL" \
| awk '
    NR==1 { for(i=3;i<=NF;i++) bitvar[i-3]=$i+0; next }   # mapping line
    /^v / { for(i=2;i<=NF;i++){ l=$i+0; if(l>0) val[l]=1 } }
    END {
      out="={ "
      for(c=0;c<81;c++){
        b=0
        for(k=0;k<8;k++) if(val[bitvar[c*8+k]]) b+=2^k
        if(b<49||b>57){ print "decode error: cell "c" = "b > "/dev/stderr"; exit 2 }
        out=out b ", "
      }
      print out "1 }"
    }
  ' > solution.inc

# 4. Compile the checker (which #includes solution.inc) and print the grid.
rm -f sudoku_chk
g++ sudoku_chk.cpp -o sudoku_chk
./sudoku_chk
