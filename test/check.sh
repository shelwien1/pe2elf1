#!/bin/sh
# check.sh BIN FILE -- what roundtrip.sh does not try, with FILE an AAC
# stream: no input, bytes that are not AAC, pipes, a stream cut short, with
# junk in it or a container that is not looked for, strict mode, and
# compressed streams that are broken (d must fail on them; what it restored
# before it found out stays as the output), on one thread and on threads
# (-t). Exit status 1 if any fails.
BIN=$1
F=$2
tmp=${TMPDIR:-/tmp}/aacpack.$$
fail=0
ok() { echo "ok    $1"; }
bad() {
  echo "FAIL  $1"
  fail=1
}
# rt NAME FILE [c options]: d(c(FILE)) is FILE
rt() {
  n=$1 f=$2
  shift 2
  if "$BIN" c "$@" "$f" "$tmp.apk" 2> /dev/null && "$BIN" d "$tmp.apk" "$tmp.out" 2> /dev/null && cmp -s "$f" "$tmp.out"; then
    ok "$n"
  else
    bad "$n"
  fi
}

: > "$tmp.in"
rt "no input" "$tmp.in"
head -c 300000 /dev/urandom > "$tmp.in"
rt "random bytes" "$tmp.in"
[ "$(wc -c < "$tmp.apk")" -le 300100 ] && ok "random bytes stay their size" || bad "random bytes stay their size"
if "$BIN" c - - < "$F" 2> /dev/null | "$BIN" d - - 2> /dev/null | cmp -s "$F" -; then ok "pipes"; else bad "pipes"; fi
head -c 54321 "$F" > "$tmp.in"
rt "a stream cut short" "$tmp.in"
{ head -c 30000 "$F"; head -c 777 /dev/urandom; tail -c +30001 "$F"; } > "$tmp.in"
rt "junk in a stream" "$tmp.in"
rt "a container not looked for" "$F" -adif
rt "strict" "$F" -s
# (its failure: status 1, not a crash)
"$BIN" c -s "$tmp.in" "$tmp.apk" 2> /dev/null
r=$?
if [ $r -eq 1 ]; then ok "strict fails on junk in a frame"; else bad "strict fails on junk in a frame (status $r)"; fi

"$BIN" c "$F" "$tmp.apk" 2> /dev/null
n=$(wc -c < "$tmp.apk")
# dbad NAME: d of $tmp.bad fails with status 1
dbad() {
  "$BIN" d "$tmp.bad" "$tmp.out" 2> /dev/null
  r=$?
  if [ $r -eq 1 ]; then ok "$1"; else bad "$1 (status $r)"; fi
}
head -c $((n / 2)) "$tmp.apk" > "$tmp.bad"
dbad "d of a compressed stream cut short"
b=$(od -An -tu1 -j $((n / 3)) -N1 "$tmp.apk")
{ head -c $((n / 3)) "$tmp.apk"; printf "\\$(printf %o $(((b + 1) % 256)))"; tail -c +$((n / 3 + 2)) "$tmp.apk"; } > "$tmp.bad"
dbad "d of a compressed stream with a byte changed"
{ cat "$tmp.apk"; printf 'x'; } > "$tmp.bad"
dbad "d of a compressed stream with bytes after it"
cp "$F" "$tmp.bad"
dbad "d of what is not a compressed stream"

# on threads: the stream in parts of 64 KB
if "$BIN" c -t 3 -c 64k - - < "$F" 2> /dev/null | "$BIN" d - - 2> /dev/null | cmp -s "$F" -; then ok "threads: pipes"; else bad "threads: pipes"; fi
"$BIN" c -t 3 -c 64k "$F" "$tmp.apk" 2> /dev/null
n=$(wc -c < "$tmp.apk")
head -c $((n / 2)) "$tmp.apk" > "$tmp.bad"
dbad "threads: d of a compressed stream cut short"
b=$(od -An -tu1 -j $((n / 3)) -N1 "$tmp.apk")
{ head -c $((n / 3)) "$tmp.apk"; printf "\\$(printf %o $(((b + 1) % 256)))"; tail -c +$((n / 3 + 2)) "$tmp.apk"; } > "$tmp.bad"
dbad "threads: d of a compressed stream with a byte changed"
{ cat "$tmp.apk"; printf 'x'; } > "$tmp.bad"
dbad "threads: d of a compressed stream with bytes after it"
"$BIN" c -t 65 "$F" "$tmp.apk" 2> /dev/null
r=$?
if [ $r -eq 2 ]; then ok "threads: -t takes 1 to 64"; else bad "threads: -t takes 1 to 64 (status $r)"; fi
rm -f "$tmp.in" "$tmp.apk" "$tmp.out" "$tmp.bad"
exit $fail
