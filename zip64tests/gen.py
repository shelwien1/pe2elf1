#!/usr/bin/env python3
# Generate a battery of ZIP64 / edge-case archives used to stress zipcl's
# parser. Each archive exercises a different layout; see comments per case.
# Run: python3 zip64tests/gen.py   (writes t1..t12 next to this script)

import zipfile, struct, os, io

OUT = os.path.dirname(os.path.abspath(__file__))

def w(name, data):
    with open(os.path.join(OUT, name), "wb") as f:
        f.write(data)
    print(f"  wrote {name}: {len(data)} bytes")

# 1) single small entry forced into ZIP64 (local header sizes -> 0xFFFFFFFF,
#    real sizes in the 0x0001 extra field), STORED.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
    with z.open("a.txt", "w", force_zip64=True) as fp:
        fp.write(b"hello zip64 world\n")
w("t1_force_zip64_stored.zip", buf.getvalue())

# 2) two force_zip64 entries, DEFLATED.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    with z.open("a.txt", "w", force_zip64=True) as fp: fp.write(b"A"*5000)
    with z.open("b.txt", "w", force_zip64=True) as fp: fp.write(b"B"*7000)
w("t2_two_force_zip64_deflate.zip", buf.getvalue())

# 3) mixed: one plain 32-bit entry + one force_zip64 entry.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    z.writestr("plain.txt", b"just a plain small entry\n")
    with z.open("big.txt", "w", force_zip64=True) as fp: fp.write(b"X"*3000)
w("t3_mixed.zip", buf.getvalue())

# 4) streamed entry (size unknown up front) -> trailing data descriptor + zip64.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    zi = zipfile.ZipInfo("stream.txt"); zi.compress_type = zipfile.ZIP_DEFLATED
    with z.open(zi, "w", force_zip64=True) as fp: fp.write(b"streamed content "*50)
w("t4_datadescriptor_zip64.zip", buf.getvalue())

# 5) non-empty archive comment in the EOCD.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
    z.comment = b"this is an archive comment"
    with z.open("c.txt", "w", force_zip64=True) as fp: fp.write(b"commented archive\n")
w("t5_with_comment.zip", buf.getvalue())

# 6) empty (0-byte) entry forced zip64.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
    with z.open("empty.txt", "w", force_zip64=True): pass
w("t6_empty_zip64.zip", buf.getvalue())

# 7) baseline: plain non-zip64 archive (sanity / regression).
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=False) as z:
    z.writestr("plain1.txt", b"hello\n")
    z.writestr("plain2.txt", b"world\n"*100)
w("t7_plain_no_zip64.zip", buf.getvalue())

# 8) >65535 entries: forces a real ZIP64 EOCD record (PK66) + locator (PK67)
#    via the file-count overflow, and overruns the 1<<16 offset table.
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
    for i in range(70000): z.writestr("f%05d" % i, b"")
w("t8_manyfiles_zip64eocd.zip", buf.getvalue())

# 9) small archive but with a real ZIP64 EOCD record + locator, by shrinking
#    the library's ZIP64 threshold so even tiny offsets trip it.
buf = io.BytesIO()
_save = (zipfile.ZIP64_LIMIT, zipfile.ZIP_FILECOUNT_LIMIT)
zipfile.ZIP64_LIMIT = 10
zipfile.ZIP_FILECOUNT_LIMIT = 2
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    z.writestr("one.txt", b"first entry content\n"*3)
    z.writestr("two.txt", b"second entry\n"*5)
    z.writestr("three.txt", b"third\n")
zipfile.ZIP64_LIMIT, zipfile.ZIP_FILECOUNT_LIMIT = _save
w("t9_small_zip64eocd.zip", buf.getvalue())

# 10) completely empty archive (EOCD only).
buf = io.BytesIO()
zipfile.ZipFile(buf, "w", allowZip64=True).close()
w("t10_empty_archive.zip", buf.getvalue())

# 11) force_zip64 entry that also carries a sibling extra field (0x5455 unix
#     timestamp) next to the 0x0001 zip64 field -> exercises extra-field walk.
buf = io.BytesIO()
zi = zipfile.ZipInfo("withextra.txt")
zi.extra = struct.pack("<HH", 0x5455, 5) + b"\x03\x00\x00\x00\x00"
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    with z.open(zi, "w", force_zip64=True) as fp: fp.write(b"data with sibling extra fields "*4)
w("t11_sibling_extra.zip", buf.getvalue())

# 12) force_zip64 highly-compressible entry (large logical size, tiny cSize).
buf = io.BytesIO()
with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    with z.open("comp.txt", "w", force_zip64=True) as fp: fp.write(b"\x00"*200000)
w("t12_highcompress_zip64.zip", buf.getvalue())


# --- streaming (bit-3 data descriptor) and exotic central-dir records --------

class Unseekable(io.RawIOBase):
    """Non-seekable sink: forces zipfile to emit trailing data descriptors."""
    def __init__(self): self.b = bytearray()
    def writable(self): return True
    def seekable(self): return False
    def write(self, x): self.b += x; return len(x)

# 13) streamed entries -> 4-byte (non-zip64) data descriptors, bit 3 set.
u = Unseekable()
with zipfile.ZipFile(u, "w", zipfile.ZIP_DEFLATED, allowZip64=False) as z:
    z.writestr("s1.txt", b"streamed one "*20)
    z.writestr("s2.txt", b"streamed two "*30)
w("t13_datadesc_stream.zip", bytes(u.b))

# 14) streamed force_zip64 entries -> 8-byte ZIP64 data descriptors.
u = Unseekable()
with zipfile.ZipFile(u, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as z:
    with z.open(zipfile.ZipInfo("big1.txt"), "w", force_zip64=True) as fp: fp.write(b"zip64 streamed "*40)
    with z.open(zipfile.ZipInfo("big2.txt"), "w", force_zip64=True) as fp: fp.write(b"second zip64 "*25)
w("t14_datadesc_zip64_stream.zip", bytes(u.b))

def _stored(entries):
    """Hand-build STORED local headers + central headers; returns (pre, [cd...])."""
    pre = b""; offs = []
    for name, data in entries:
        offs.append(len(pre))
        crc = zipfile.crc32(data) & 0xffffffff
        pre += struct.pack("<IHHHHHIIIHH", 0x04034b50, 20, 0, 0, 0, 0, crc,
                           len(data), len(data), len(name), 0) + name.encode() + data
    cd = []
    for i, (name, data) in enumerate(entries):
        crc = zipfile.crc32(data) & 0xffffffff
        cd.append(struct.pack("<IHHHHHHIIIHHHHHII", 0x02014b50, 20, 20, 0, 0, 0, 0, crc,
                              len(data), len(data), len(name), 0, 0, 0, 0, 0, offs[i]) + name.encode())
    return pre, cd

# 15) central directory carrying a Digital Signature record (PK\5\5).
pre, cd = _stored([("a.txt", b"alpha\n"), ("b.txt", b"beta\n")])
sig = b"PSEUDO-SIGNATURE-BYTES"
digsig = struct.pack("<IH", 0x05054b50, len(sig)) + sig
cddata = b"".join(cd) + digsig
eocd = struct.pack("<IHHHHIIH", 0x06054b50, 0, 0, len(cd), len(cd), len(cddata), len(pre), 0)
w("t15_digital_signature.zip", pre + cddata + eocd)

# 16) Archive Extra Data record (PK\6\8) preceding the central directory.
pre, cd = _stored([("x.txt", b"xray\n"), ("y.txt", b"yankee\n")])
aed = struct.pack("<II", 0x08064b50, 26) + b"ARCHIVE-EXTRA-DATA-PAYLOAD"
cddata = b"".join(cd)
eocd = struct.pack("<IHHHHIIH", 0x06054b50, 0, 0, len(cd), len(cd),
                   len(cddata) + len(aed), len(pre) + len(aed), 0)
w("t16_archive_extra.zip", pre + aed + cddata + eocd)

print("done")
