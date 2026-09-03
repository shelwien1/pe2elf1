#!/bin/sh
# Round-trip and weights checks.  Usage: ./t.sh [file ...]  (default ../book1000)
#
# For each file, five things:
#   1  fresh:  encode, decode, compare - and the encoder's and the decoder's
#      final models must be identical, since they saw the same bytes
#   2  warm:   the same, starting from the model run 1 ended with
#   3  frozen: with training off, the model written must be the model read,
#      byte for byte.  That is the whole save/load path - decode, remap to the
#      packed alphabet, remap back, re-encode - checked against itself
#   4  frozen over a NARROWER alphabet: the checkpoint must come back
#      unchanged rather than shrunk to the bytes this file happens to use
#   5  the same two, with the weights quantized to int4 (LSTM_SAVE_Q4): a
#      frozen save has to reproduce those bytes as well, which is what the
#      per-row scale being kept rather than re-fitted buys
#   6  gN: exactly N bytes out, the same ones every time, nothing left over
#   7  no model at all (a weights_in that does not exist): PPMD alone
set -e
[ -x ./coder0 ] || ./build.sh >/dev/null
[ -x ./coder0_frozen ] || OUT=coder0_frozen LSTMDEFS=-DLSTM_TRAIN=0 ./build.sh >/dev/null
[ -x ./coder0_q4 ] || OUT=coder0_q4 LSTMDEFS=-DLSTM_SAVE_Q4=1 ./build.sh >/dev/null
[ -x ./coder0_q4frozen ] || OUT=coder0_q4frozen \
    LSTMDEFS="-DLSTM_SAVE_Q4=1 -DLSTM_TRAIN=0" ./build.sh >/dev/null
W=${TMPDIR:-/tmp}/coder0_lstm
trap 'rm -f "$W".*' EXIT

for f in "${@:-../book1000}"; do
  fail() { echo "$f: FAILED - $1"; exit 1; }

  ./coder0 c "$f" "$W.1c" nul "$W.1we" 2>/dev/null
  ./coder0 d "$W.1c" "$W.1d" nul "$W.1wd" 2>/dev/null
  cmp -s "$f" "$W.1d"       || fail "fresh round trip"
  cmp -s "$W.1we" "$W.1wd"  || fail "encoder and decoder ended with different models"

  ./coder0 c "$f" "$W.2c" "$W.1we" "$W.2we" 2>/dev/null
  ./coder0 d "$W.2c" "$W.2d" "$W.1we" "$W.2wd" 2>/dev/null
  cmp -s "$f" "$W.2d"       || fail "warm round trip"
  cmp -s "$W.2we" "$W.2wd"  || fail "warm: encoder and decoder ended with different models"

  ./coder0_frozen c "$f" "$W.3c" "$W.1we" "$W.3we" 2>/dev/null
  ./coder0_frozen d "$W.3c" "$W.3d" "$W.1we" 2>/dev/null
  cmp -s "$f" "$W.3d"       || fail "frozen round trip"
  cmp -s "$W.1we" "$W.3we"  || fail "frozen save is not the model that was loaded"

  # the same, over an alphabet narrower than the checkpoint's
  tr -cd 'a-z \n' < "$f" > "$W.narrow" || true
  if [ -s "$W.narrow" ]; then
    ./coder0_frozen c "$W.narrow" "$W.4c" "$W.1we" "$W.4we" 2>/dev/null
    cmp -s "$W.1we" "$W.4we" || fail "a narrower file shrank the checkpoint"
  fi

  ./coder0_q4 c "$f" "$W.6c" nul "$W.6we" 2>/dev/null
  ./coder0_q4 d "$W.6c" "$W.6d" nul "$W.6wd" 2>/dev/null
  cmp -s "$f" "$W.6d"       || fail "q4 round trip"
  cmp -s "$W.6we" "$W.6wd"  || fail "q4: encoder and decoder ended with different models"
  ./coder0_q4frozen c "$f" "$W.7c" "$W.6we" "$W.7we" 2>/dev/null
  cmp -s "$W.6we" "$W.7we"  || fail "q4 frozen save is not the model that was loaded"
  if [ -s "$W.narrow" ]; then
    ./coder0_q4frozen c "$W.narrow" "$W.8c" "$W.6we" "$W.8we" 2>/dev/null
    cmp -s "$W.6we" "$W.8we" || fail "q4: a narrower file shrank the checkpoint"
  fi
  # either build reads either form
  ./coder0 c "$f" "$W.9c" "$W.6we" 2>/dev/null
  ./coder0 d "$W.9c" "$W.9d" "$W.6we" 2>/dev/null
  cmp -s "$f" "$W.9d"       || fail "f32 build reading a q4 checkpoint"
  ./coder0_q4 c "$f" "$W.ac" "$W.1we" 2>/dev/null
  ./coder0_q4 d "$W.ac" "$W.ad" "$W.1we" 2>/dev/null
  cmp -s "$f" "$W.ad"       || fail "q4 build reading an f32 checkpoint"

  ./coder0 g256 "$f" "$W.g1" 2>/dev/null
  ./coder0 g256 "$f" "$W.g2" 2>/dev/null
  [ "$(wc -c < "$W.g1")" -eq 256 ] || fail "g256 did not write 256 bytes"
  cmp -s "$W.g1" "$W.g2" || fail "g256 is not deterministic"
  [ ! -e "$W.g1.rnd" ] || fail "g256 left its bit source behind"

  ./coder0 c "$f" "$W.5c" "$W.no_such_file" 2>/dev/null
  ./coder0 d "$W.5c" "$W.5d" "$W.no_such_file" 2>/dev/null
  cmp -s "$f" "$W.5d"       || fail "ppmd-only round trip"

  printf '%s: %s bytes -> %s ppmd, %s fresh, %s warm, %s frozen; checkpoint %s f32, %s q4  all ok\n' \
      "$f" "$(wc -c < "$f")" "$(wc -c < "$W.5c")" "$(wc -c < "$W.1c")" \
      "$(wc -c < "$W.2c")" "$(wc -c < "$W.3c")" \
      "$(wc -c < "$W.1we")" "$(wc -c < "$W.6we")"
done
