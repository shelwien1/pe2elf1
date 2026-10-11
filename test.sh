#!/usr/bin/env bash
# test.sh - end-to-end check of the pe2elf32 port of vbn.exe, and of vbn2.
#
#   1. ./vbn u archive input.txt  -> archive.vbn is $EXPECT bytes
#   2. ./vbn r archive            -> run in an empty directory, input.txt comes
#                                    back byte-exact, with its date and time
#   3. ./vbn2 u archive input.txt -> vbn2, the standalone C++ build of `u` and
#                                    `r`, writes the same archive, and the same
#                                    output (the elapsed time aside)
#   4. ./vbn2 r archive           -> restores it as in 2
#   5. if wine and i686-w64-mingw32-gcc are installed: the original vbn.exe,
#      with vbnshim.dll built from the same shim.c, writes a byte-identical
#      archive.  Skipped otherwise.
#
# VBN stores the input's DOS date and time (to the minute) in the archive, and
# they go through the compressor with the data, so the archive's size depends
# on input.txt's mtime: over the first hour after the DOS epoch it ranges from
# 222681 to 222701 bytes.  The test pins the mtime, in UTC, to the first
# minute after the epoch for which the archive is 222691 bytes.

set -u
cd "$(dirname "$0")"
export TZ=UTC
STAMP='1980-01-01 00:13'
EXPECT=222691

PASS=0
FAIL=0
ok()   { echo "ok: $1"; PASS=$((PASS+1)); }
# fail <message> [log]: the log's last lines are shown with it.
fail() {
  echo "FAIL: $1"; FAIL=$((FAIL+1))
  if [ -n "${2:-}" ]; then tr '\r' '\n' < "$2" | tail -5; fi
}

make -s all || { echo "FAIL: make"; exit 1; }

# Not mktemp's default /tmp: it may be mounted noexec.
work=$(mktemp -d ./test.XXXXXX)
trap 'rm -rf "$work"' EXIT
mkdir "$work/u" "$work/r"

# 1. Compress.
cp vbn libvbnshim.so input.txt "$work/u/"
touch -d "$STAMP" "$work/u/input.txt"
( cd "$work/u" && ./vbn u archive input.txt ) > "$work/u.log" 2>&1
rc=$?
size=$(stat -c %s "$work/u/archive.vbn" 2>/dev/null || echo 0)
tr '\r' '\n' < "$work/u.log" | sed -n 's/.*\(Code size=.*[^ ]\) *$/vbn: \1/p'
if [ "$rc" -ne 0 ]; then
  fail "vbn u exited $rc" "$work/u.log"
elif [ "$size" -ne "$EXPECT" ]; then
  fail "archive.vbn is $size bytes, expected $EXPECT"
else
  ok "vbn u archive input.txt: archive.vbn is $size bytes"
fi

# 2. Restore, into a directory that has only the archive.
cp vbn libvbnshim.so "$work/r/"
cp "$work/u/archive.vbn" "$work/r/" 2>/dev/null
( cd "$work/r" && ./vbn r archive ) > "$work/r.log" 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  fail "vbn r exited $rc" "$work/r.log"
elif ! cmp -s input.txt "$work/r/input.txt"; then
  fail "restored input.txt differs from the original"
elif [ "$(date -r "$work/r/input.txt" '+%Y-%m-%d %H:%M')" != "$STAMP" ]; then
  fail "restored input.txt has mtime $(date -r "$work/r/input.txt" '+%Y-%m-%d %H:%M'), expected $STAMP"
else
  ok "vbn r archive: input.txt restored byte-exact, mtime $STAMP"
fi

# 3. vbn2 compresses: alone in its directory, no shim beside it.
mkdir "$work/u2" "$work/r2"
cp vbn2 input.txt "$work/u2/"
touch -d "$STAMP" "$work/u2/input.txt"
( cd "$work/u2" && ./vbn2 u archive input.txt ) > "$work/u2.log" 2>&1
rc=$?
masked() { tr '\r' '\n' < "$1" | sed -E 's/Time = [0-9]+ sec\./Time = N sec./'; }
if [ "$rc" -ne 0 ]; then
  fail "vbn2 u exited $rc" "$work/u2.log"
elif ! cmp -s "$work/u/archive.vbn" "$work/u2/archive.vbn"; then
  fail "vbn2's archive.vbn ($(stat -c %s "$work/u2/archive.vbn" 2>/dev/null || echo 0) bytes) differs from vbn's"
elif [ "$(masked "$work/u.log")" != "$(masked "$work/u2.log")" ]; then
  fail "vbn2 u printed something else than vbn u"
else
  ok "vbn2 u archive input.txt: the same archive.vbn as vbn's, the same output"
fi

# 4. vbn2 restores.
cp vbn2 "$work/r2/"
cp "$work/u2/archive.vbn" "$work/r2/" 2>/dev/null
( cd "$work/r2" && ./vbn2 r archive ) > "$work/r2.log" 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  fail "vbn2 r exited $rc" "$work/r2.log"
elif ! cmp -s input.txt "$work/r2/input.txt"; then
  fail "input.txt restored by vbn2 differs from the original"
elif [ "$(date -r "$work/r2/input.txt" '+%Y-%m-%d %H:%M')" != "$STAMP" ]; then
  fail "input.txt restored by vbn2 has mtime $(date -r "$work/r2/input.txt" '+%Y-%m-%d %H:%M'), expected $STAMP"
else
  ok "vbn2 r archive: input.txt restored byte-exact, mtime $STAMP"
fi

# 5. Reference: the original PE and the shim's Windows build, under Wine.
if command -v wine >/dev/null && command -v i686-w64-mingw32-gcc >/dev/null; then
  mkdir "$work/w"
  cp vbn.exe input.txt "$work/w/"
  touch -d "$STAMP" "$work/w/input.txt"
  # vbn.exe imports the undecorated vbn_start, not vbn_start@4.
  if ! i686-w64-mingw32-gcc -O2 -shared -o "$work/w/vbnshim.dll" shim.c -Wl,--kill-at -static-libgcc; then
    fail "could not build vbnshim.dll"
  else
    ( cd "$work/w" && WINEDEBUG=-all wine vbn.exe u archive input.txt ) > "$work/w.log" 2>&1
    if cmp -s "$work/u/archive.vbn" "$work/w/archive.vbn"; then
      ok "archive is byte-identical to vbn.exe + vbnshim.dll under Wine"
    else
      fail "archive differs from vbn.exe + vbnshim.dll under Wine" "$work/w.log"
    fi
  fi
else
  echo "skip: Windows reference (needs wine and i686-w64-mingw32-gcc)"
fi

echo
echo "passed: $PASS, failed: $FAIL"
[ "$FAIL" -eq 0 ]
