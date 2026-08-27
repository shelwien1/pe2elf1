#!/bin/sh
# Round-trip matrix. Every config encodes and decodes each test file and the
# result is compared byte for byte against the input.
#
# When the build has a usable OpenCL device, each file is also encoded on it
# and that output is compared against the host encode -- not just for size, but
# byte for byte. The kernel is the same integer arithmetic in the same order as
# the host coder, so anything else is a bug.
#
#   ./t.sh "" "-DRC_RCNUM=8" "-DRC_LOWBYTES=5"
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

pass=0; fail=0; devruns=0
for cfg in "$@"; do
  ./build.sh $cfg >/dev/null 2>&1 || { echo "BUILD FAIL [$cfg]"; fail=$((fail+1)); continue; }

  # is there a device in this build?
  dev=0
  if ./coder -l 2>/dev/null | grep -q '^ *-d '; then dev=1; fi

  for fsm in FSM0.txt nzcc.txt; do
    for f in test/*; do
      rm -f /tmp/rc_h /tmp/rc_d /tmp/rc_out
      ./coder -C c "$f" /tmp/rc_h "$fsm" >/dev/null 2>&1 || { echo "ENC FAIL [$cfg] $fsm $f"; fail=$((fail+1)); continue; }
      ./coder    d /tmp/rc_h /tmp/rc_out "$fsm" >/dev/null 2>&1 || { echo "DEC FAIL [$cfg] $fsm $f"; fail=$((fail+1)); continue; }
      if cmp -s "$f" /tmp/rc_out; then pass=$((pass+1)); else
        echo "MISMATCH [$cfg] $fsm $f"; fail=$((fail+1)); continue
      fi

      [ $dev = 1 ] || continue
      ./coder c "$f" /tmp/rc_d "$fsm" >/dev/null 2>/tmp/rc_err || { echo "ENC FAIL (device) [$cfg] $fsm $f"; fail=$((fail+1)); continue; }
      if grep -q "falling back" /tmp/rc_err; then
        echo "DEVICE GAVE UP [$cfg] $fsm $f"; sed 's/^/    /' /tmp/rc_err; fail=$((fail+1)); continue
      fi
      if ! cmp -s /tmp/rc_h /tmp/rc_d; then
        echo "DEVICE != HOST [$cfg] $fsm $f"; fail=$((fail+1)); continue
      fi
      ./coder d /tmp/rc_d /tmp/rc_out "$fsm" >/dev/null 2>&1
      if cmp -s "$f" /tmp/rc_out; then pass=$((pass+1)); devruns=$((devruns+1)); else
        echo "MISMATCH (device) [$cfg] $fsm $f"; fail=$((fail+1))
      fi
    done
  done
  echo "  ok: [$cfg]${dev:+ }$([ $dev = 1 ] && echo '(host+device)' || echo '(host only)')"
done
rm -f /tmp/rc_h /tmp/rc_d /tmp/rc_out /tmp/rc_err
echo "pass=$pass fail=$fail (device encodes checked: $devruns)"
[ $fail -eq 0 ]
