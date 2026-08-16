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
# The second leg is run-length pairs, and it is a stronger check than the
# first.  `testfiles/rle4.bmp` and `rle8.bmp` are real files that use encoded
# runs and end-of-line and nothing else, so the absolute-run reader -- six exits
# across two nibble parities -- was reached by no gate at all: every one of the
# six could be given a wrong answer with all 110 checks green.  Each pair is one
# opcode stream that uses *every* opcode at both parities, beside the same
# pixels written out flat, and `mknarrow.py` decodes the stream itself to
# produce them.  What the two files being equal after a round trip says is that
# two readers of one format agree; it is not this program checked against
# itself.  All six exits are caught: perturbing any of them takes it to 1 of 2.
#
# It is fast (sixteen small images), so unlike `asan.sh` and `hdrscan.sh` there
# is no reason not to run it.
set -u
cd "$(dirname "$0")/.."

BIN=${1:-./bmf}
[ -x "$BIN" ] || { echo "no such binary: $BIN" >&2; exit 2; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

counts=$(python3 tools/mknarrow.py "$tmp") \
  || { echo "the generator failed" >&2; exit 2; }
n=${counts% *}
rle=${counts#* }

# Compress and expand one file, answering where the expansion landed.
through() {
  "$BIN" c "$1" "$tmp/$2.bmf" >/dev/null 2>&1 || return 1
  "$BIN" d "$tmp/$2.bmf" "$tmp/$2.out.bmp" >/dev/null 2>&1 || return 1
  return 0
}

ok=0
bad=''
for f in "$tmp"/*.bmp; do
  b=$(basename "$f" .bmp)
  # The run-length pairs are checked against each other below; a run-length
  # file cannot round-trip to itself, because `write_bmp` splits its runs its
  # own way.  `testfiles/out_rle4.bmp` is in the corpus for the same reason.
  case $b in *_rle|*_raw) continue ;; esac
  if ! through "$f" "$b"; then
    bad="$bad $b(coder)"
    continue
  fi
  if cmp -s "$f" "$tmp/$b.out.bmp"; then
    ok=$((ok+1))
  else
    bad="$bad $b"
  fi
done

# Every run-length opcode, at both nibble parities, against the same pixels
# written out flat.  `mknarrow.py` decodes the opcode stream itself, so what
# these compare is two readers of one format and not this one against itself.
pairs=0
for f in "$tmp"/*_rle.bmp; do
  b=$(basename "$f" _rle.bmp)
  if ! through "$f" "${b}_rle" || ! through "$tmp/${b}_raw.bmp" "${b}_raw"; then
    bad="$bad $b(coder)"
    continue
  fi
  if cmp -s "$tmp/${b}_rle.out.bmp" "$tmp/${b}_raw.out.bmp"; then
    pairs=$((pairs+1))
  else
    bad="$bad $b(run-length)"
  fi
done

if [ -n "$bad" ]; then
  echo "$ok of $n degenerate geometries round-trip and $pairs of $rle"
  echo "run-length streams agree with the same pixels flat; failed:$bad"
  exit 1
fi
echo "$ok of $n degenerate geometries round-trip, and $pairs of $rle run-length"
echo "streams agree with the same pixels written out flat"
