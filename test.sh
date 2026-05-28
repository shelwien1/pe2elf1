#!/bin/sh
# Roundtrip test for all binaries.
#   pngdump + preflate_raw + png2bmp   (script-based pipeline; c.sh/d.sh)
#   pngtool                            (integrated standalone)

set -e

here="$(cd "$(dirname "$0")" && pwd)"
cd "$here"

for bin in pngdump png2bmp preflate_raw pngtool; do
    [ -x "./$bin" ] || { echo "missing ./$bin (run 'make')" >&2; exit 1; }
done

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

ok=0; fail=0

echo "=== pipeline test (c.sh / d.sh, tar.xz) ==="
for png in testfile/*.png; do
    base="$(basename "$png" .png)"
    cp "$png" "$work/"
    (cd "$work" && "$here/c.sh" "$base.png" >/dev/null)
    mv "$work/$base.png" "$work/$base.orig.png"
    (cd "$work" && "$here/d.sh" "$base.tar.xz" >/dev/null)
    if cmp -s "$work/$base.orig.png" "$work/$base.png"; then
        sz_o=$(stat -c%s "$work/$base.orig.png")
        sz_x=$(stat -c%s "$work/$base.tar.xz")
        printf "  OK  %-22s orig=%-8s tar.xz=%s\n" "$base.png" "$sz_o" "$sz_x"
        ok=$((ok+1))
    else
        printf "  FAIL %s\n" "$base.png"
        fail=$((fail+1))
    fi
    rm -f "$work/$base"*
done

echo
echo "=== pngtool integrated test ==="
for png in testfile/*.png; do
    base="$(basename "$png" .png)"
    cp "$png" "$work/"
    "./pngtool" c "$work/$base.png" "$work/$base.bmp" "$work/$base.meta" >/dev/null
    mv "$work/$base.png" "$work/$base.orig.png"
    "./pngtool" d "$work/$base.bmp" "$work/$base.meta" "$work/$base.png" >/dev/null
    if cmp -s "$work/$base.orig.png" "$work/$base.png"; then
        sz_o=$(stat -c%s "$work/$base.orig.png")
        sz_m=$(stat -c%s "$work/$base.meta")
        sz_b=$(stat -c%s "$work/$base.bmp" 2>/dev/null \
               || stat -c%s "$work/$base"*.bmp | head -1)
        printf "  OK  %-22s orig=%-8s meta=%-8s bmp=%s\n" "$base.png" "$sz_o" "$sz_m" "$sz_b"
        ok=$((ok+1))
    else
        printf "  FAIL %s\n" "$base.png"
        fail=$((fail+1))
    fi
    rm -f "$work/$base"*
done

echo
echo "passed: $ok, failed: $fail"
[ "$fail" = 0 ]
