#!/bin/sh
# Round-trip every file given on the command line (or every .fwse / .wav under
# the directory given) through mtf2ima c and d, both with a separate metainfo
# file and with the metainfo carried inside the output, and check that d
# reproduces the input byte for byte and that ffmpeg accepts what c produced.
#
#   test/roundtrip.sh [-v] file...|dir

set -e

MTF2IMA=${MTF2IMA:-./mtf2ima}
TMP=`mktemp -d`
trap 'rm -rf "$TMP"' EXIT

verbose=0
[ "$1" = "-v" ] && { verbose=1; shift; }

list=$TMP/list
: > $list
for a in "$@"; do
    if [ -d "$a" ]; then find "$a" -type f \( -name '*.fwse' -o -name '*.wav' \) >> $list
    else echo "$a" >> $list
    fi
done

ok=0; bad=0; insize=0; outsize=0; metasize=0; embsize=0

while IFS= read -r f; do
    [ -n "$f" ] || continue
    "$MTF2IMA" c -q "$f" "$TMP/p.out" "$TMP/p.bin"
    "$MTF2IMA" d -q "$TMP/p.out" "$TMP/p.back" "$TMP/p.bin"

    "$MTF2IMA" c -q "$f" "$TMP/e.out"                   # metainfo inside the output
    "$MTF2IMA" d -q "$TMP/e.out" "$TMP/e.back"

    if cmp -s "$f" "$TMP/p.back" && cmp -s "$f" "$TMP/e.back" &&
       ffmpeg -nostdin -v error -i "$TMP/p.out" -f s16le -y "$TMP/p.pcm" 2>"$TMP/err" &&
       ffmpeg -nostdin -v error -i "$TMP/e.out" -f s16le -y "$TMP/e.pcm" 2>"$TMP/err" &&
       cmp -s "$TMP/p.pcm" "$TMP/e.pcm"; then
        ok=`expr $ok + 1`
        insize=`expr $insize + \`wc -c < "$f"\``
        outsize=`expr $outsize + \`wc -c < "$TMP/p.out"\``
        metasize=`expr $metasize + \`wc -c < "$TMP/p.bin"\``
        embsize=`expr $embsize + \`wc -c < "$TMP/e.out"\``
        [ $verbose = 1 ] && echo "ok   $f"
    else
        bad=`expr $bad + 1`
        echo "FAIL $f"
        cat "$TMP/err" 2>/dev/null || true
    fi
done < $list

echo "$ok ok, $bad failed; $insize bytes in, $outsize repacked + $metasize metainfo, $embsize single-file"
[ $bad = 0 ]
