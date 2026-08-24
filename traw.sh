#!/bin/sh
# traw.sh -- round-trip and detection test for rawadpcm.
#
# Every case has to come back byte-identical; that is the only hard
# requirement.  What each case is FOR is in the comment above it.
set -e
: ${TMP:=/tmp/rawadpcm-test}
mkdir -p "$TMP"
RA=./rawadpcm
fail=0

run() {                 # run NAME FILE [options...]
  nam=$1; f=$2; shift 2
  rm -f "$TMP/$nam".w*.wav "$TMP/$nam.rem" "$TMP/$nam.res"
  out=`$RA c "$f" "$TMP/$nam.rem" "$TMP/$nam.w" "$@" | tail -1`
  $RA d "$TMP/$nam.rem" "$TMP/$nam.res" "$TMP/$nam.w" >/dev/null
  if cmp -s "$f" "$TMP/$nam.res"; then r=ok; else r=MISMATCH; fail=1; fi
  printf '%-12s %s  [%s]\n' "$nam" "$out" "$r"
}

# The real thing: wavs3 holds seven wavs -- two MS stereo and five IMA mono --
# so one file exercises both codecs and both channel counts.  All seven data
# chunks should come out, and the ~900 bytes left over are the RIFF headers.
run wavs3 wavs3 -v 2>/dev/null || run wavs3 wavs3

# The same seven streams with every RIFF header removed, which is what the tool
# is actually for: nothing in the file says where the audio is or what shape it
# has.  Should find the same seven.
if [ -f "$TMP/raw.bin" ]; then run headerless "$TMP/raw.bin"; fi

# Nothing here is ADPCM.  Zero segments is the right answer for all of them:
# text and executables are what a general-purpose coder is for, random bytes
# decode to a random walk that predicts nothing, and a run of equal bytes
# compresses to nothing whether or not anyone calls it audio.
cat *.inc > "$TMP/text.bin"
head -c 400000 /dev/urandom > "$TMP/rand.bin"
head -c 400000 /dev/zero > "$TMP/zero.bin"
run text "$TMP/text.bin"
run random "$TMP/rand.bin"
run zeros "$TMP/zero.bin"
[ -f ./xadpcm ] && cp ./xadpcm "$TMP/elf.bin" && run elf "$TMP/elf.bin"

# Mixed: real audio followed by source code, to check that finding one does not
# disturb the other.
cat wavs3 "$TMP/text.bin" > "$TMP/mixed.bin"
run mixed "$TMP/mixed.bin"

if [ $fail = 0 ]; then echo "all round trips byte-identical"; else echo "FAILURES"; exit 1; fi
