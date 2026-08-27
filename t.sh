#!/bin/sh
# Round-trip matrix. Every config encodes and decodes each test file and the
# result is compared byte for byte against the input.
set -e

mkdir -p test
[ -f test/empty ]   || : > test/empty
[ -f test/one ]     || printf 'A' > test/one
[ -f test/blk ]     || head -c 65536  book1 > test/blk        # exactly BLKSIZE
[ -f test/blk1 ]    || head -c 65537  book1 > test/blk1       # BLKSIZE+1
[ -f test/blk2 ]    || head -c 131072 book1 > test/blk2       # 2*BLKSIZE
[ -f test/ff ]      || head -c 300000 /dev/zero | tr '\0' '\377' > test/ff
[ -f test/zero ]    || head -c 300000 /dev/zero > test/zero
[ -f test/rand ]    || head -c 300000 /dev/urandom > test/rand
[ -f test/book1 ]   || cp book1 test/book1

pass=0; fail=0
for cfg in "$@"; do
  ./build.sh $cfg >/dev/null 2>&1 || { echo "BUILD FAIL [$cfg]"; fail=$((fail+1)); continue; }
  for fsm in FSM0.txt nzcc.txt; do
    for f in test/*; do
      rm -f /tmp/rc_c /tmp/rc_d
      ./coder c "$f" /tmp/rc_c "$fsm" >/dev/null 2>/tmp/rc_err || { echo "ENC FAIL [$cfg] $fsm $f"; fail=$((fail+1)); continue; }
      ./coder d /tmp/rc_c /tmp/rc_d "$fsm" >/dev/null 2>>/tmp/rc_err || { echo "DEC FAIL [$cfg] $fsm $f"; fail=$((fail+1)); continue; }
      if cmp -s "$f" /tmp/rc_d; then
        pass=$((pass+1))
      else
        echo "MISMATCH [$cfg] $fsm $f  ($(stat -c%s "$f") -> $(stat -c%s /tmp/rc_c))"
        fail=$((fail+1))
      fi
    done
  done
  echo "  ok: [$cfg]"
done
echo "pass=$pass fail=$fail"
[ $fail -eq 0 ]
