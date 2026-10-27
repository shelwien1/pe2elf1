#!/bin/sh
# bench.sh [TARGET...]
#
# Builds bench.c with -O3 -march=TARGET for each compiler and runs every
# kernel of every build separately on this machine. A build that executes an
# instruction this CPU lacks dies with SIGILL; gdb, if present, shows which.
#   CLANG  clang command   (default: clang;  empty: skip)
#   GCC    gcc command     (default: gcc;    empty: skip; gcc says skylake-avx512 for skx)
#   LINK   link command    (default: cc; so a compiler's build tree without libgcc works)
#   RUNS   processes per kernel, best time kept (default: 3)
#   OUT    build directory (default: ./bench.out)
# Output: OUT/results.tsv (compiler target kernel ms|SIGILL instruction) and a table.
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG-clang}; GCC=${GCC-gcc}; LINK=${LINK:-cc}; RUNS=${RUNS:-3}; OUT=${OUT:-bench.out}
[ $# -gt 0 ] || set -- k8 x86-64 x86-64-v3 haswell alderlake skx cascadelake x86-64-v4 icelake-server znver5 znver6
mkdir -p "$OUT" || exit 1
: > "$OUT/results.tsv"
for cc in clang gcc; do
  eval "cmd=\$$(echo $cc | tr a-z A-Z)"; [ -n "$cmd" ] || continue
  for t in "$@"; do
    m=$t; [ $cc = gcc ] && [ $t = skx ] && m=skylake-avx512
    b=$OUT/bench_${cc}_$t
    if ! $cmd -O3 -march=$m -fPIE -c "$HERE/bench.c" -o $b.o 2> $b.err || ! $LINK $b.o -o $b 2>> $b.err; then
      for k in float int8 bits; do printf '%s\t%s\t%s\tbuild-failed\n' $cc $t $k; done >> "$OUT/results.tsv"; continue
    fi
    for k in f:float i:int8 b:bits; do
      a=${k%%:*}; k=${k#*:}; best=; sum=; bad=; r=0
      while [ $r -lt $RUNS ]; do
        line=$($b $a 2>/dev/null); st=$?
        if [ $st -ne 0 ]; then
          bad="exit$st"
          if [ $st -eq 132 ]; then
            bad=SIGILL
            command -v gdb >/dev/null && bad="SIGILL $(gdb -q -batch -ex run -ex 'x/i $pc' --args $b $a 2>/dev/null | sed -n 's/^=> [^:]*:[[:space:]]*//p' | head -1)"
          fi
          break
        fi
        ms=$(echo "$line" | awk '{print $2}'); sum=$(echo "$line" | awk '{print $6}')
        best=$(awk -v a="$best" -v b="$ms" 'BEGIN{ print (a=="" || b+0<a+0) ? b : a }')
        r=$((r+1))
      done
      if [ -n "$bad" ]; then printf '%s\t%s\t%s\t%s\n' $cc $t $k "$bad"
      else printf '%s\t%s\t%s\t%s\t%s\n' $cc $t $k $best $sum; fi >> "$OUT/results.tsv"
    done
  done
done
awk -F'\t' '
  { key=$1" "$2; if( !(key in seen) ) { seen[key]=1; order[n++]=key }
    if( $5!="" ) { if( !($3 in ref) ) ref[$3]=$5; else if( ref[$3]!=$5 ) diff[$3]=diff[$3]" "key }
    v = ($4 ~ /^[0-9.]+$/) ? sprintf( "%8.2f", $4 ) : $4; cell[key,$3]=v }
  END { printf "%-6s %-15s %8s %8s %8s   (ms, best of runs; lower is better)\n", "", "-march", "float", "int8", "bits"
        for( i=0; i<n; i++ ) { split( order[i], p, " " )
          printf "%-6s %-15s", p[1], p[2]
          for( j=1; j<=3; j++ ) { k=(j==1?"float":j==2?"int8":"bits"); c=cell[order[i],k]; printf " %8s", (c ~ /^SIGILL/) ? "SIGILL" : c }
          printf "\n" }
        for( i=0; i<n; i++ ) for( j=1; j<=3; j++ ) { k=(j==1?"float":j==2?"int8":"bits"); c=cell[order[i],k]
          if( c ~ /^SIGILL ./ ) printf "  %s %s: %s\n", order[i], k, substr( c, 8 ) }
        for( k in diff ) printf "  %s: other checksum than the first build:%s\n", k, diff[k] }' "$OUT/results.tsv"
