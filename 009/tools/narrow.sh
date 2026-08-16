#!/usr/bin/env bash
# narrow.sh — round-trip the geometries the corpus does not have.
#
#     tools/narrow.sh                 # the default build
#     tools/narrow.sh ./bmf-asan      # some other binary
#
# The seventeen images in `testfiles/` are all wider than three pixels and
# taller than two, so a whole family of geometry cases -- a one-pixel row, a
# one-pixel column, a single pixel, a packed depth at width one, an image just
# either side of `search_filter`'s `4 x 3` floor -- has never been run by a
# gate at all.  `tools/mknarrow.py` says what the fourteen are.
#
# What this checks is the round trip and not fidelity: these images are
# generated here, so no reference stream from BMF.exe can exist for them.  That
# only catches an *asymmetry* between the two directions, which is worth saying
# plainly -- a change to something both directions share, the model or a
# predictor's arithmetic, passes this and is caught by `test.sh` instead.  What
# it does catch is a decoder walking off the end of a short row: corrupting one
# byte in `write_bmp`'s row copy takes it from 14 of 14 to 0 of 14.
#
# **It does not reach the `width == 1` arms of `predict_med` and
# `unpredict_med`, and no gate here does.**  That was the gap this started
# from: flattening `unpredict_med`'s `while( 1 )` left a `width == 1` walk that
# could be given a wrong answer and still pass all 110 checks over all
# seventeen images, while three other paths in the same body each moved two.
# A narrow image does not fix it, and the reason is worth writing down --
# `search_filter` returns before choosing any predictor when the image is
# narrower than four or shorter than three, so it leaves every descriptor at
# mode 0 and the encoder never picks MED for an image that narrow.  The arm is
# reachable only from a stream that *claims* MED for a one-pixel-wide image,
# which is `fuzz.sh`'s and `hdrscan.sh`'s territory and not something a round
# trip can construct.  It is carried by construction, and this says so rather
# than implying otherwise by being green.
#
# It is fast (fourteen small images), so unlike `asan.sh` and `hdrscan.sh`
# there is no reason not to run it.
set -u
cd "$(dirname "$0")/.."

BIN=${1:-./bmf}
[ -x "$BIN" ] || { echo "no such binary: $BIN" >&2; exit 2; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

n=$(python3 tools/mknarrow.py "$tmp") || { echo "the generator failed" >&2; exit 2; }

ok=0
bad=''
for f in "$tmp"/*.bmp; do
  b=$(basename "$f" .bmp)
  if ! "$BIN" c "$f" "$tmp/$b.bmf" >/dev/null 2>&1; then
    bad="$bad $b(compress)"
    continue
  fi
  if ! "$BIN" d "$tmp/$b.bmf" "$tmp/$b.out.bmp" >/dev/null 2>&1; then
    bad="$bad $b(expand)"
    continue
  fi
  if cmp -s "$f" "$tmp/$b.out.bmp"; then
    ok=$((ok+1))
  else
    bad="$bad $b"
  fi
done

if [ -n "$bad" ]; then
  echo "$ok of $n degenerate geometries round-trip; failed:$bad"
  exit 1
fi
echo "$ok of $n degenerate geometries round-trip"
