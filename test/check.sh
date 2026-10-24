#!/bin/sh
# check.sh BIN FILE [MP4] -- what roundtrip.sh does not try, with FILE an
# AAC stream (and MP4 an MP4 file): no input, bytes that are not AAC, pipes
# (the MP4 file's too), a stream cut short, with junk in it or a container
# that is not looked for, strict mode, and
# compressed streams that are broken (d must fail on them; what it restored
# before it found out stays as the output). Exit status 1 if any fails.
BIN=$1
F=$2
M=$3
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
if [ -n "$M" ]; then
  if "$BIN" c - - < "$M" 2> /dev/null | "$BIN" d - - 2> /dev/null | cmp -s "$M" -; then ok "an MP4 file through pipes"; else bad "an MP4 file through pipes"; fi
  head -c 77777 "$M" > "$tmp.in"
  rt "an MP4 file cut short" "$tmp.in"
fi
head -c 54321 "$F" > "$tmp.in"
rt "a stream cut short" "$tmp.in"
{ head -c 30000 "$F"; head -c 777 /dev/urandom; tail -c +30001 "$F"; } > "$tmp.in"
rt "junk in a stream" "$tmp.in"
rt "a container not looked for" "$F" -adif
rt "strict" "$F" -s
if "$BIN" c -s "$tmp.in" "$tmp.apk" 2> /dev/null; then bad "strict fails on junk in a frame"; else ok "strict fails on junk in a frame"; fi

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
rm -f "$tmp.in" "$tmp.apk" "$tmp.out" "$tmp.bad"
exit $fail
