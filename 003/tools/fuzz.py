#!/usr/bin/env python3
"""fuzz.py -- deterministic mutants of the test corpus.

    tools/fuzz.py OUTDIR N [SEED]

Writes N files into OUTDIR named `<seed-name>.<i>.<what>` and prints one line
per file saying which seed it came from and what was done to it, so a mutant
that reports can be described without keeping the whole run.

Deterministic on purpose.  A fuzzer whose findings cannot be replayed is a
fuzzer whose findings have to be believed, and this project has spent nine
rounds learning not to believe measurements it cannot re-take.  Same SEED and
same corpus gives the same bytes, so `tools/fuzz.sh --seed 7` names an exact
set of inputs and a report against one of them is a file anyone can rebuild.

Three mutators, in the proportion the last round argued for:

  bytes    flip a few bytes anywhere.  This is what the crash-only round ran,
           and over 2000 runs it found nothing: the odds of landing inside the
           14-byte BITMAPFILEHEADER by chance are about one in a thousand, and
           everything past the header is entropy-coded, so a flip there gets
           absorbed rather than followed.

  header   rewrite one named header field with a value chosen from the edges:
           0, 1, -1, 0x7fffffff, 0x80000000, and the sizes either side of the
           file's own.  Every defect section 46 records was a header field the
           reader trusted, so this is the mutator aimed at the class.

  truncate cut the file at a boundary that matters -- inside the file header,
           inside the info header, at the palette, at the start of the pixel
           data, and part-way through it.  Two of the seven crashes were a
           read that ran off the end of a short file.

The `.bmf` corpus gets `bytes` and `truncate` only: a compressed stream has no
named fields, and its header is four bytes of magic plus the image record that
`compress_image` wrote, which the header mutator would only be flipping blind.
"""
import os
import random
import struct
import sys

# BITMAPFILEHEADER then BITMAPINFOHEADER, by name and by (offset, size, signed).
# Only the fields `read_bmp` reads; the rest it skips, and mutating them would
# be testing this script's idea of a BMP rather than the reader's.
BMP_FIELDS = [
    ("bfType",        0, 2, False),
    ("bfSize",        2, 4, False),
    ("bfOffBits",    10, 4, False),
    ("biSize",       14, 4, False),
    ("biWidth",      18, 4, True),
    ("biHeight",     22, 4, True),
    ("biPlanes",     26, 2, False),
    ("biBitCount",   28, 2, False),
    ("biCompression", 30, 4, False),
    ("biSizeImage",  34, 4, False),
    ("biClrUsed",    46, 4, False),
]

EDGES = [0, 1, 2, 3, 4, 0xFFFFFFFF, 0x7FFFFFFF, 0x80000000, 0xFFFF, 0x10000,
         24, 32, 255, 256, 0x40000000]


def put(buf, off, size, val):
    buf[off:off + size] = struct.pack("<I" if size == 4 else "<H",
                                      val & (0xFFFFFFFF if size == 4 else 0xFFFF))


def mutate_bytes(rng, data):
    buf = bytearray(data)
    n = rng.randint(1, 8)
    for _ in range(n):
        i = rng.randrange(len(buf))
        buf[i] = rng.randrange(256)
    return bytes(buf), "bytes"


def mutate_header(rng, data):
    buf = bytearray(data)
    name, off, size, _signed = rng.choice(BMP_FIELDS)
    if off + size > len(buf):
        return mutate_bytes(rng, data)
    choices = EDGES + [len(data), len(data) + 1, len(data) - 1]
    val = rng.choice(choices)
    put(buf, off, size, val)
    return bytes(buf), "header:%s=%#x" % (name, val & 0xFFFFFFFF)


def mutate_truncate(rng, data):
    # The boundaries that mean something, plus one arbitrary cut so the set is
    # not only the ones this script thought of.
    cuts = [2, 6, 10, 13, 14, 17, 18, 26, 30, 40, 53, 54]
    off = 54
    if len(data) >= 14:
        off = struct.unpack_from("<I", data, 10)[0]
        if not (14 <= off <= len(data)):
            off = 54
    cuts += [off - 1, off, off + 1, off + 16]
    cuts.append(rng.randrange(1, max(2, len(data))))
    n = rng.choice([c for c in cuts if 0 < c < len(data)] or [1])
    return bytes(data[:n]), "truncate:%d" % n


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().splitlines()[2].strip())
    outdir, count = sys.argv[1], int(sys.argv[2])
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)

    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    corpus = []
    tf = os.path.join(here, "testfiles")
    for n in sorted(os.listdir(tf)):
        if n.endswith(".bmp") or n.endswith(".bmf"):
            with open(os.path.join(tf, n), "rb") as f:
                corpus.append((n, f.read()))
    if not corpus:
        sys.exit("no corpus in %s" % tf)

    os.makedirs(outdir, exist_ok=True)
    for i in range(count):
        name, data = rng.choice(corpus)
        if name.endswith(".bmp"):
            m = rng.choice([mutate_bytes, mutate_header, mutate_header,
                            mutate_truncate])
        else:
            m = rng.choice([mutate_bytes, mutate_truncate])
        body, what = m(rng, data)
        ext = ".bmf" if name.endswith(".bmf") else ".bmp"
        out = "%s.%d%s" % (name.rsplit(".", 1)[0], i, ext)
        with open(os.path.join(outdir, out), "wb") as f:
            f.write(body)
        print("%s\t%s\t%s" % (out, name, what))


if __name__ == "__main__":
    main()
