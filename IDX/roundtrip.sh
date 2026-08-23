#!/bin/sh
# roundtrip.sh -- the tuning loop, without the tuning.
#
# opt.pl finds parameters by scanning the executable for "!MAP!" and folds its
# results back with import.pl.  Both halves have to agree about names, and a
# parameter opt.pl cannot find is one the optimizer silently never moves.  This
# exports the patterns the binary already holds and imports them again: the
# .idx sources must come back byte-identical, which proves every declared
# parameter is reachable and that import.pl puts each one back where it came
# from.
#
#   ./mk.sh && sh IDX/roundtrip.sh
set -e
[ -x ./bmf ] || { echo "build the tuning binary first: ./mk.sh"; exit 1; }
tmp=$(mktemp -d)
python3 - "$tmp/export.!!!" <<'PY'
import re, sys
d = open('bmf', 'rb').read()
out = ['mdesc( %s, %s, "%s" );' % (m.group(1).decode(), m.group(2).decode(), m.group(3).decode())
       for m in re.finditer(rb'!MAP!(.*?)!(.*?)\x00(.*?)\x00', d, re.S)]
if not out:
    sys.exit('no !MAP! markers -- this is a release build')
open(sys.argv[1], 'w').write('\n'.join(sorted(out)) + '\n')
print('%d parameters exported' % len(out))
PY
cp IDX/bmf-*.idx "$tmp/"
( cd IDX && for f in bmf-*.idx; do perl import.pl "$f" "$tmp/export.!!!" > t 2>/dev/null && mv t "$f"; done )
fail=0
for f in IDX/bmf-*.idx; do
  if ! cmp -s "$f" "$tmp/$(basename "$f")"; then
    echo "CHANGED $f"
    diff "$tmp/$(basename "$f")" "$f" | head -4
    fail=1
  fi
done
rm -rf "$tmp"
[ $fail = 0 ] && echo "round trip clean: every parameter is reachable and lands where it came from"
exit $fail
