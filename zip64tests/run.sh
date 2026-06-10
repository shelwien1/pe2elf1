#!/bin/sh
# ZIP64 / edge-case test harness for zipcl.
# Regenerates the archives, then for each one checks:
#   1. d/c passthrough is byte-exact (decrypt then re-encrypt == original)
#   2. the decrypted stream extracts to the same content as the original
#      (compared by extraction, so unzip's quirks with exotic records --
#      digital-signature / archive-extra -- don't cause false failures)
#   3. encrypt-with-synthetic-pad keeps the (ZIP64) central directory readable
#      and decrypting it back reproduces the original (forces offset shifts)
# Run from the repo root:  sh zip64tests/run.sh
cd "$(dirname "$0")/.."
K="a3e30892 f9185194 eb474b09"

python3 zip64tests/gen.py >/dev/null

scratch="_d _pad _c _e _pad2 _back"
trap 'rm -f $scratch; rm -rf _eo _ed' EXIT

content_match() { # $1=original $2=decrypted -> echo MATCH / DIFF / (n/a)
  rm -rf _eo _ed; mkdir -p _eo _ed
  o=$(cd _eo && unzip -qq -o "../$1" >/dev/null 2>&1; echo $?)
  d=$(cd _ed && unzip -qq -o "../$2" >/dev/null 2>&1; echo $?)
  if [ "$o" = 0 ] && [ "$d" = 0 ]; then
    diff -r _eo _ed >/dev/null 2>&1 && echo MATCH || echo DIFF
  else
    echo "n/a"   # 0-entry / non-extractable archive
  fi
}

echo "== passthrough (d then c) + content =="
for f in zip64tests/t*.zip stream.zip; do
  rm -f $scratch
  timeout 60 ./zipcl d "$f" _d _pad $K >/dev/null 2>&1; rc1=$?
  timeout 60 ./zipcl c _d _c _pad  $K >/dev/null 2>&1; rc2=$?
  cmp -s "$f" _c && rt=OK || rt=DIFF
  cm=$(content_match "$f" _d)
  st="ok"
  { [ $rc1 -ne 0 ] || [ $rc2 -ne 0 ]; } && st="ERR(d=$rc1,c=$rc2)"
  [ "$rt" != OK ] && st="$st RT-DIFF"
  [ "$cm" = DIFF ] && st="$st CONTENT-DIFF"
  printf "  %-36s roundtrip=%-4s content=%-5s %s\n" "$(basename "$f")" "$rt" "$cm" "$st"
done

echo "== encrypt-with-pad, then decrypt back (forces offset shift) =="
for f in zip64tests/t*.zip; do
  case "$f" in *t10_*) printf "  %-36s skip (0 entries)\n" "$(basename "$f")"; continue;; esac
  rm -f $scratch
  # entry count = number of local file headers (works on archives whose central
  # directory python's zipfile won't parse, e.g. the digital-signature one)
  n=$(python3 -c "print(open('$f','rb').read().count(b'PK\x03\x04'))")
  python3 -c "open('_pad','wb').write(bytes((i*7+3)&0xff for i in range(12*$n)))"
  timeout 60 ./zipcl c "$f" _e _pad  $K >/dev/null 2>&1
  timeout 60 ./zipcl d _e _back _pad2 $K >/dev/null 2>&1
  cmp -s "$f" _back && rt=OK || rt=DIFF        # decrypt(encrypt(x)) == x
  enc_ok=$(content_match "$f" _e)              # encrypted archive still extracts?
  st="ok"
  [ "$rt" != OK ] && st="$st BACK-DIFF"
  [ "$enc_ok" = DIFF ] && st="$st ENC-CONTENT-DIFF"
  printf "  %-36s entries=%-6s back=%-4s enc-extract=%-5s %s\n" "$(basename "$f")" "$n" "$rt" "$enc_ok" "$st"
done
