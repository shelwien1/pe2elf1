#!/bin/sh
# Recover the unknown ZipCrypto key bits in a `zipcl g`-generated model, using
# CBMC as a CNF compiler and kissat as the SAT solver.
#
#   solve_zipcrypt.sh <model.cpp> <nunk> [expected_hex]
#
# CBMC bit-blasts the model to DIMACS (the unknown key bits become the nondet
# `UNK`); kissat solves it; we decode UNK's low <nunk> bits from kissat's model
# via the `c UNK#1 ...` variable-mapping comment CBMC embeds in the CNF.
#
# Override the solver with e.g. KISSAT=../../kissat_inc/bin/kissat
MODEL=$1; NUNK=$2; EXPECT=$3
KISSAT=${KISSAT:-kissat}
CBMC=${CBMC:-cbmc}
[ -n "$MODEL" ] && [ -n "$NUNK" ] || { echo "usage: $0 <model.cpp> <nunk> [expected_hex]"; exit 2; }
base=${MODEL%.cpp}

# 1. CBMC: model -> CNF (satisfiable iff `match` is reachable, i.e. iff UNK is
#    consistent with every file's known plaintext).
"$CBMC" "$MODEL" --error-label match --dimacs --outfile "$base.cnf" >/dev/null 2>&1

# 2. kissat: solve (exit 10 = SAT, 20 = UNSAT).
"$KISSAT" "$base.cnf" > "$base.model" 2>/dev/null
grep -q '^s SATISFIABLE' "$base.model" \
  || { echo "UNSAT / no model: these constraints don't pin the key bits"; exit 1; }

# 3. Decode UNK's low <nunk> bits (LSB first) from the model.
hexw=$(( (NUNK + 3) / 4 ))
rec=$(grep -m1 '^c UNK#1 ' "$base.cnf" | cat - "$base.model" | awk -v n="$NUNK" -v w="$hexw" '
  NR==1 { for(i=3;i<=NF;i++) v[i-3]=$i+0; next }              # UNK bit -> CNF var
  /^v / { for(i=2;i<=NF;i++){ l=$i+0; if(l>0) val[l]=1 } }    # kissat model
  END   { b=0; for(k=0;k<n;k++) if(val[v[k]]) b+=2^k; printf("%0*X", w, b) }')

echo "recovered $NUNK key bit(s) = 0x$rec"
if [ -n "$EXPECT" ]; then
  R=$(echo "$rec"    | tr 'a-f' 'A-F')
  E=$(echo "$EXPECT" | tr 'a-f' 'A-F')
  [ "$R" = "$E" ] && echo "expected 0x$EXPECT  ->  MATCH" \
                   || { echo "expected 0x$EXPECT  ->  MISMATCH"; exit 1; }
fi
