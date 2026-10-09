#!/bin/sh
# test_del_eh.sh: builds hello_eh in several variants, runs del_eh on them and
# checks that the results still run (and that a throw no longer gets caught).
# The Windows part needs x86_64-w64-mingw32-g++ and wine.  Run from the top
# directory: sh tests/test_del_eh.sh  (or make test-del-eh)
set -u
DEL=./del_eh
SRC=tests/hello_eh.cpp
OUT=tests/out
X64PP=./x64pp
mkdir -p $OUT
fail=0
pass=0
ok() { pass=$((pass + 1)); }
bad() { fail=$((fail + 1)); echo "FAIL: $*"; }

# run BIN EXPECT_THROW_CAUGHT [RUNNER]: normal run, run with arguments, throwing run
check() {
  bin=$1 caught=$2 runner=${3:-}
  out=$($runner $bin 2>/dev/null | tr -d '\r')
  [ "$out" = "hello world 10" ] && ok || bad "$bin: normal run printed '$out'"
  out=$($runner $bin a bc 2>/dev/null | tr -d '\r')
  [ "$out" = "hello world 3" ] && ok || bad "$bin: run with arguments printed '$out'"
  $runner $bin "" >/dev/null 2>&1
  rc=$?
  if [ "$caught" = yes ]; then
    [ $rc -eq 2 ] && ok || bad "$bin: the exception was not caught (exit $rc)"
  else
    [ $rc -ne 0 ] && [ $rc -ne 2 ] && ok || bad "$bin: a throw should end the program (exit $rc)"
  fi
  if [ -x $X64PP ]; then
    $X64PP c $bin $bin.x64pp && $X64PP d $bin.x64pp $bin.back && cmp -s $bin $bin.back && ok ||
      bad "$bin: x64pp round trip"
    rm -f $bin.x64pp $bin.back
  fi
}

# process IN OUT OPTIONS...: del_eh, input must stay unchanged
process() {
  in=$1 out=$2
  shift 2
  sum=$(cksum < $in)
  $DEL "$@" $in $out || { bad "del_eh $* $in failed"; return 1; }
  [ "$(cksum < $in)" = "$sum" ] && ok || bad "del_eh $* changed its input $in"
  echo "  del_eh $* $(basename $in): $(wc -c < $in) -> $(wc -c < $out) bytes"
}

echo "ELF"
for v in pie nopie emitrelocs clang lld; do
  case $v in
    pie) cxx="${CXX:-g++}" flags="" ;;
    nopie) cxx="${CXX:-g++}" flags="-no-pie" ;;
    emitrelocs) cxx="${CXX:-g++}" flags="-Wl,-q" ;;
    clang) command -v clang++ >/dev/null || continue; cxx=clang++ flags="" ;;
    lld) command -v clang++ >/dev/null && command -v ld.lld >/dev/null || continue; cxx=clang++ flags="-fuse-ld=lld" ;;
  esac
  b=$OUT/hello_$v
  $cxx -O2 $flags -o $b $SRC || { bad "build $v"; continue; }
  check $b yes
  process $b $b.z && check $b.z no
  process $b $b.zd -d && check $b.zd no
  if readelf -lW $b.z 2>/dev/null | grep -q GNU_EH_FRAME; then bad "$b.z still has PT_GNU_EH_FRAME"; else ok; fi
  if [ $v = emitrelocs ]; then
    process $b $b.r -r -d && check $b.r yes
    readelf -SW $b.r | grep '\.rela\.text' | grep -q ' 000000 ' && ok || bad "$b.r: .rela.text left"
  fi
done

if command -v x86_64-w64-mingw32-g++ >/dev/null && command -v wine >/dev/null; then
  echo "PE (wine)"
  export WINEDEBUG=-all
  for v in full stripped; do
    b=$OUT/hello_$v.exe
    flags="-O2 -static"
    [ $v = stripped ] && flags="$flags -s"
    x86_64-w64-mingw32-g++ $flags -o $b $SRC || { bad "build $b"; continue; }
    check $b yes wine
    process $b $OUT/e_$v.exe -e && check $OUT/e_$v.exe no wine
    process $b $OUT/r_$v.exe -r && check $OUT/r_$v.exe yes wine
    process $b $OUT/erd_$v.exe -e -r -d && check $OUT/erd_$v.exe no wine
  done
else
  echo "PE: skipped (needs x86_64-w64-mingw32-g++ and wine)"
fi

echo "$pass checks passed, $fail failed"
[ $fail -eq 0 ]
