#!/bin/sh
# Self test of ROSE for Linux.  Translates the programs in the examples folder with
# identityTranslator (a C program using the POSIX API, and a C++17 program using the standard
# library), draws the AST of one with dotGenerator, changes a copy of examples/refactor.cpp with
# the refactoring tools (rose-ren, rose-m2g and rose-using), and, if gcc and g++ are in the PATH,
# checks that the translated and changed programs behave like the originals.  The files are
# written to a new folder in $TMPDIR (default /tmp).
#
# usage: ./test.sh
rose=$(cd "$(dirname "$0")" && pwd)
bin=$rose/bin
examples=$rose/examples
work=$(mktemp -d "${TMPDIR:-/tmp}/rose-test-XXXXXX") || exit 1
failed=

echo "ROSE for Linux: self test"
echo "  ROSE:   $rose"
echo "  output: $work"
echo
cd "$work" || exit 1

# check STATUS FILE LOG -- the step succeeded if STATUS is 0 and FILE exists
check() {
  if [ "$1" = 0 ] && [ -f "$2" ]; then
    echo "      ok, wrote $2"
  else
    echo "      FAILED (exit status $1):"
    cat "$3"
    failed=1
  fi
}

# check_tool STATUS FILE TEXT LOG -- the tool succeeded if STATUS is 0 and FILE contains TEXT
check_tool() {
  if [ "$1" = 0 ] && grep -qF -- "$3" "$2"; then
    echo "        ok"
    return 0
  fi
  echo "        FAILED (exit status $1):"
  cat "$4"
  failed=1
  return 1
}

# compare NAME COMPILER EXAMPLE [OPTIONS] -- builds EXAMPLE with COMPILER and with
# identityTranslator, runs both programs and compares their output
compare() {
  name=$1 cc=$2 src=$3
  shift 3
  if ! $cc "$@" -o "${name}_original" "$examples/$src" > "${name}_original.log" 2>&1; then
    echo "      FAILED to compile $src with $cc:"
    cat "${name}_original.log"
    failed=1
    return
  fi
  if ! "$bin/identityTranslator" "$@" -o "${name}_rose" "$examples/$src" > "${name}_rose.log" 2>&1; then
    echo "      FAILED to translate and compile $src:"
    cat "${name}_rose.log"
    failed=1
    return
  fi
  "./${name}_original" > "${name}_original.txt" 2>&1
  "./${name}_rose" > "${name}_rose.txt" 2>&1
  if cmp -s "${name}_original.txt" "${name}_rose.txt"; then
    echo "      ok, ${name}_rose (built by ROSE) prints the same as ${name}_original:"
    cat "${name}_rose.txt"
  else
    echo "      FAILED: ${name}_original and ${name}_rose print different output:"
    diff "${name}_original.txt" "${name}_rose.txt"
    failed=1
  fi
}

echo "[1/6] identityTranslator: C with the POSIX API (examples/hello.c)"
"$bin/identityTranslator" -rose:skipfinalCompileStep -c "$examples/hello.c" > hello.log 2>&1
check $? rose_hello.c hello.log

echo "[2/6] identityTranslator: C++17 with the standard library (examples/shapes.cpp)"
"$bin/identityTranslator" -std=c++17 -rose:skipfinalCompileStep -c "$examples/shapes.cpp" > shapes.log 2>&1
check $? rose_shapes.cpp shapes.log

echo "[3/6] dotGenerator: the AST of examples/hello.c as a GraphViz graph"
"$bin/dotGenerator" "$examples/hello.c" > dot.log 2>&1
check $? hello.c.dot dot.log

echo "[4/6] the refactoring tools, on refactor.cpp (a copy of examples/refactor.cpp)"
cp "$examples/refactor.cpp" refactor.cpp
echo "      rose-ren refactor.cpp balance"
"$bin/rose-ren" refactor.cpp balance > rename_list.txt 2> rename_list.log
check_tool $? rename_list.txt "Account::balance" rename_list.log && sed 's/^/        /' rename_list.txt
echo "      rose-ren refactor.cpp balance:1 getBalance"
"$bin/rose-ren" refactor.cpp balance:1 getBalance > rename.log 2>&1
check_tool $? refactor.cpp "account.getBalance()" rename.log
echo "      rose-m2g refactor.cpp Account::deposit"
"$bin/rose-m2g" refactor.cpp Account::deposit > m2g.log 2>&1
check_tool $? refactor.cpp "deposit(&account, stack.top())" m2g.log
echo "      rose-using refactor.cpp"
"$bin/rose-using" refactor.cpp > using.log 2>&1
check_tool $? refactor.cpp "using Container<T>::items;" using.log

if command -v gcc > /dev/null 2>&1 && command -v g++ > /dev/null 2>&1; then
  echo "[5/6] the translated programs, compiled with gcc and g++, behave like the originals"
  compare hello gcc hello.c
  compare shapes g++ shapes.cpp -std=c++17
  echo "[6/6] the changed refactor.cpp, compiled with g++, prints what the original prints"
  echo "      (compiled with Visual C++: g++ does not compile the original)"
  if ! g++ -o refactor refactor.cpp > refactor_compile.log 2>&1; then
    echo "      FAILED to compile the changed refactor.cpp with g++:"
    cat refactor_compile.log
    failed=1
  elif ./refactor > refactor.txt 2>&1 && [ "$(cat refactor.txt)" = "depth 2, top 2, balance 102" ]; then
    echo "      ok, refactor prints:"
    cat refactor.txt
  else
    echo "      FAILED: refactor prints something else:"
    cat refactor.txt
    failed=1
  fi
else
  echo "[5/6] skipped: gcc and g++ are not in the PATH"
  echo "[6/6] skipped: gcc and g++ are not in the PATH"
fi

echo
if [ -n "$failed" ]; then
  echo "Some tests FAILED."
  exit 1
fi
echo "All tests passed.  The output files are in $work"
exit 0
