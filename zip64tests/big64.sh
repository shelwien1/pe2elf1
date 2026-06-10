#!/bin/sh
# Opt-in >4 GiB ZIP64 test: validates the 64-bit offset path. Slow (a few
# minutes) and needs ~13 GiB of free scratch space, so it is NOT part of
# run.sh. Run explicitly:  sh zip64tests/big64.sh
# Builds an archive with a ~4.1 GiB stored entry followed by a small entry
# whose local header (and the ZIP64 EOCD) sit past the 4 GiB boundary, then
# checks decrypt passthrough, the >4 GiB offset shift under encryption, and a
# byte-exact encrypt->decrypt roundtrip.
cd "$(dirname "$0")/.."
K="a3e30892 f9185194 eb474b09"
T=${TMPDIR:-/tmp}
H=$T/big64.zip
trap 'rm -f $T/big64.* ' EXIT

echo "building $H (~4.1 GiB, may take a minute) ..."
python3 - "$H" <<'PY'
import zipfile, sys, os
fn = sys.argv[1]
with zipfile.ZipFile(fn, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
    with z.open(zipfile.ZipInfo("big.bin"), "w", force_zip64=True) as f:
        chunk = b"\0" * (1<<20)
        for _ in range(4097):                 # >4 GiB -> next entry crosses the boundary
            f.write(chunk)
    z.writestr("small.txt", b"second entry past the 4GB boundary\n")
off = [i.header_offset for i in zipfile.ZipFile(fn).infolist() if i.filename=="small.txt"][0]
print("  archive=%d bytes, small.txt header_offset=0x%X (%.3f GiB)" % (os.path.getsize(fn), off, off/2**30))
assert off > 0xFFFFFFFF, "second entry did not cross 4 GiB"
PY

fail=0

echo "[1/3] decrypt passthrough + offset resolution"
./zipcl d "$H" "$T/big64.d" "$T/big64.dpad" $K >/dev/null 2>&1
python3 - "$T/big64.d" <<'PY' || fail=1
import zipfile, sys
z = zipfile.ZipFile(sys.argv[1])
assert z.read("small.txt") == b"second entry past the 4GB boundary\n", "content mismatch"
assert z.testzip() is None, "CRC failure"
print("  ok: >4GiB central-dir offset resolves, CRCs valid")
PY

echo "[2/3] encrypt shifts the >4GiB offset by +12/entry"
python3 -c "open('$T/big64.pad','wb').write(bytes((i*7+3)&0xff for i in range(24)))"
./zipcl c "$H" "$T/big64.e" "$T/big64.pad" $K >/dev/null 2>&1
python3 - "$T/big64.e" <<'PY' || fail=1
import zipfile, sys
z = zipfile.ZipFile(sys.argv[1])
got = [i.header_offset for i in z.infolist() if i.filename=="small.txt"][0]
want = 0x100100039 + 12
assert got == want, "offset shift wrong: got 0x%X want 0x%X" % (got, want)
print("  ok: small.txt offset 0x%X shifted to 0x%X" % (0x100100039, got))
PY

echo "[3/3] decrypt(encrypt(x)) == x  (byte-exact)"
./zipcl d "$T/big64.e" "$T/big64.back" "$T/big64.bpad" $K >/dev/null 2>&1
if cmp -s "$H" "$T/big64.back"; then echo "  ok: byte-exact roundtrip"; else echo "  FAIL: roundtrip diff"; fail=1; fi

[ $fail -eq 0 ] && echo "PASS" || echo "FAIL"
exit $fail
