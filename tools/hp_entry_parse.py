#!/usr/bin/env python3
"""Split starlit / fx-cmix / fx2-cmix / cmix-lex / fx2-cmix-transformer style
Hutter Prize files (compressor `cmix` and self-extracting `archive9`) into parts.

Container (src/readalike_prepr/self_extract.h + src/runner.cpp in those repos):

  12-byte trailer (fx-cmix, fx2-cmix, cmix-lex):
    cmix     = ELF(upx) + dict.comp + order.comp               + hdr{dict, order, 0}
    archive9 = ELF(upx) + dict.comp + enwik9.cmix              + hdr{dict, order*, input}
  16-byte trailer (fx2-cmix-transformer):
    cmix     = ELF(upx) + dict.comp + order.comp + tfweights   + hdr{dict, order, 0, tfw}
    archive9 = ELF(upx) + dict.comp + tfweights + enwik9.cmix  + hdr{dict, order*, input, tfw}

  hdr fields are little-endian int32. order* in archive9 is a stale copy of the
  compressor's value; the order stream itself is not stored in archive9.

The trailer layout is detected by checking that the ELF part ends with UPX's
PackHeader (the last "UPX!" sits 36 bytes before the end of a packed ELF) and
that the dictionary part starts with a sane cmix stream header.

usage: hp_entry_parse.py [--dump] FILE [FILE...]
  --dump  write each part next to its file as FILE.<part>
When several files are given, parts that are byte-identical across files
(e.g. the ELF and dict.comp shared by a compressor and its archive9) are listed.
"""
import hashlib
import struct
import sys


def cmix_stream_header(x):
    """cmix stream header: 5-byte big-endian length whose top bit means
    "dictionary used", then a 32-byte vocabulary bitmap if length >= 10000."""
    if len(x) < 5:
        return None
    n = 0
    for i in range(5):
        c = x[i]
        if i == 0:
            dict_used = bool(c & 0x80)
            c &= 0x7F
        n = (n << 8) | c
    vocab = None
    if n >= 10000 and len(x) >= 37:
        vocab = sum(bin(b).count("1") for b in x[5:37])
    return dict_used, n, vocab


def candidate_layouts(d):
    """Yield (trailer_len, fields, [(name, length), ...]) for each trailer format."""
    size = len(d)
    if size >= 12:
        dict_sz, order_sz, input_sz = struct.unpack("<iii", d[-12:])
        fields = dict(dict_size=dict_sz, new_article_order_size=order_sz,
                      decomp_input_size=input_sz)
        if input_sz == 0:
            mid = [("dict.comp", dict_sz), ("order.comp", order_sz)]
        else:
            mid = [("dict.comp", dict_sz), ("enwik9.cmix", input_sz)]
        yield 12, fields, mid
    if size >= 16:
        dict_sz, order_sz, input_sz, tfw_sz = struct.unpack("<iiii", d[-16:])
        fields = dict(dict_size=dict_sz, new_article_order_size=order_sz,
                      decomp_input_size=input_sz, tf_weights_size=tfw_sz)
        if input_sz == 0:
            mid = [("dict.comp", dict_sz), ("order.comp", order_sz),
                   ("tfweights", tfw_sz)]
        else:
            mid = [("dict.comp", dict_sz), ("tfweights", tfw_sz),
                   ("enwik9.cmix", input_sz)]
        yield 16, fields, mid


def plausible(d, trailer_len, mid):
    """Score a layout: -1 if impossible, else 0..2 (higher is more certain)."""
    if any(n < 0 for _, n in mid):
        return -1
    elf_len = len(d) - sum(n for _, n in mid) - trailer_len
    if elf_len < 64 or d[:4] != b"\x7fELF":
        return -1
    score = 0
    if b"UPX!" in d[elf_len - 64:elf_len]:
        score += 1
    hdr = cmix_stream_header(d[elf_len:elf_len + 37])
    dict_sz = mid[0][1]
    if hdr and not hdr[0] and hdr[1] >= dict_sz:
        score += 1
    return score


def parse(fn, dump=False):
    d = open(fn, "rb").read()
    best = None
    for trailer_len, fields, mid in candidate_layouts(d):
        s = plausible(d, trailer_len, mid)
        if s >= 0 and (best is None or s > best[0]):
            best = (s, trailer_len, fields, mid)
    if best is None:
        print(f"{fn}: {len(d):,} bytes, no known trailer layout matches")
        return []
    score, trailer_len, fields, mid = best
    elf_len = len(d) - sum(n for _, n in mid) - trailer_len
    parts = [("elf", elf_len)] + mid + [("trailer", trailer_len)]
    is_archive = fields["decomp_input_size"] != 0
    kind = "archive9 (S2)" if is_archive else "compressor (S1)"
    note = "" if score == 2 else f"  [low confidence, score {score}/2]"
    print(f"{fn}: {len(d):,} bytes, {kind}, {trailer_len}-byte trailer{note}")
    print("  trailer: " + " ".join(f"{k}={v:,}" for k, v in fields.items()))
    out = []
    off = 0
    for name, n in parts:
        part = d[off:off + n]
        md5 = hashlib.md5(part).hexdigest()
        if name == "elf":
            upx_at = part.rfind(b"UPX!")
            extra = ("UPX-packed" if upx_at >= 0 else "not UPX-packed") + " ELF"
        elif name in ("dict.comp", "order.comp", "enwik9.cmix"):
            du, ln, vc = cmix_stream_header(part)
            extra = f"cmix stream: dict_used={du} decoded_len={ln:,} vocab={vc}"
        elif name == "tfweights":
            extra = f"magic={part[:8]!r}"
        else:
            extra = ""
        print(f"  {name:12s} off={off:>12,} len={n:>12,}  md5={md5}  {extra}")
        if dump:
            with open(f"{fn}.{name}", "wb") as f:
                f.write(part)
        out.append((fn, name, n, md5))
        off += n
    assert off == len(d)
    return out


def main(argv):
    dump = "--dump" in argv
    files = [a for a in argv if a != "--dump"]
    if not files:
        print(__doc__)
        return 1
    allparts = []
    for fn in files:
        allparts += parse(fn, dump)
    if len(files) > 1:
        by_md5 = {}
        for fn, name, n, md5 in allparts:
            if name != "trailer" and n > 0:
                by_md5.setdefault(md5, []).append((fn, name, n))
        shared = [v for v in by_md5.values() if len({fn for fn, _, _ in v}) > 1]
        print("byte-identical parts found in more than one file:")
        if not shared:
            print("  none")
        for v in shared:
            where = ", ".join(f"{fn}:{name}" for fn, name, _ in v)
            print(f"  {v[0][2]:>12,} bytes  {where}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
