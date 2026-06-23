#!/usr/bin/env python3
"""
dump2asm.py - convert a mine32 --canon dump into a GAS .byte corpus, then
assemble it with gas and extract a raw .bin.

Input (sorted canon dump), one form per line:

    <hex bytes, '|' marks the structural/wildcard split>\t<intel disasm>
    37\taaa
    d5|00\taad 0x00
    0f 38 fc 05|00 00 00 00\taadd [0x00000000], eax

Output (.s), grouped by instruction encoding, mirroring x8632all.s:

    \t.byte 0x37\t# aaa
    \t.byte 0xd5,0x00\t# aad 0x00

Then: as --32 -o OUT.o OUT.s ; objcopy -O binary -j .text OUT.o OUT.bin
"""

import argparse
import os
import subprocess
import sys

# legacy prefixes that may precede the opcode / VEX|EVEX|XOP byte
LEGACY_PREFIXES = {0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0x66, 0x67,
                   0xf0, 0xf2, 0xf3}

# 16-bit addressing comes out of normalization as the (illegal) base register
# "ax"; rewrite to the first legal 16-bit base / base+index pair.
DISASM_FIXUPS = [("[ax+ax*1]", "[bx+si]"), ("[ax]", "[bx]")]

ENCODING_ORDER = ["legacy", "vex", "evex", "xop", "3dnow"]


def classify(bs):
    """Best-effort encoding bucket from the raw bytes (cosmetic grouping only;
    the assembled bytes are identical regardless of the bucket)."""
    i = 0
    n = len(bs)
    while i < n and bs[i] in LEGACY_PREFIXES:
        i += 1
    if i >= n:
        return "legacy"
    op = bs[i]
    nxt = bs[i + 1] if i + 1 < n else -1
    if op == 0x0f and nxt == 0x0f:
        return "3dnow"
    # C4/C5/62/8F are VEX/EVEX/XOP only when the following byte is in the
    # mod=11 region (>= 0xC0); otherwise they are legacy LES/LDS/BOUND/POP.
    if op in (0xc4, 0xc5) and nxt >= 0xc0:
        return "vex"
    if op == 0x62 and nxt >= 0xc0:
        return "evex"
    # XOP's payload byte is R.X.B.mmmmm with map mmmmm in {8,9,10}; that low-5
    # field is what distinguishes it from legacy POP (8F /0, reg field 0).
    if op == 0x8f and (nxt & 0x1f) >= 8:
        return "xop"
    return "legacy"


def fixup_disasm(text):
    for a, b in DISASM_FIXUPS:
        text = text.replace(a, b)
    return text


def parse_dump(path):
    """Yield (bytes_list, disasm_str) for each non-empty dump line."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            left, _, right = line.partition("\t")
            toks = left.replace("|", " ").split()
            try:
                bs = [int(t, 16) for t in toks]
            except ValueError:
                continue
            if not bs:
                continue
            yield bs, fixup_disasm(right.strip())


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", help="canon dump file (sorted)")
    ap.add_argument("-o", "--out", help="output base name "
                    "(default: derived from dump); writes .s/.o/.bin")
    ap.add_argument("--no-build", action="store_true",
                    help="only emit the .s file (skip as/objcopy)")
    ap.add_argument("--as", dest="gas", default="as", help="assembler (default: as)")
    ap.add_argument("--objcopy", default="objcopy", help="objcopy (default: objcopy)")
    args = ap.parse_args()

    base = args.out or os.path.splitext(os.path.basename(args.dump))[0]
    s_path = base + ".s"
    o_path = base + ".o"
    bin_path = base + ".bin"

    # bucket the forms by encoding, preserving the (sorted) dump order
    buckets = {k: [] for k in ENCODING_ORDER}
    total_bytes = 0
    total = 0
    for bs, dis in parse_dump(args.dump):
        buckets[classify(bs)].append((bs, dis))
        total_bytes += len(bs)
        total += 1

    with open(s_path, "w", encoding="utf-8") as f:
        f.write("# x86-32 instruction-encoding corpus\n")
        f.write("# Generated from a mine32 --canon dump by dump2asm.py.\n")
        f.write("# One representative encoding per canonical form; "
                "comments are Zydis Intel disasm.\n")
        f.write("# %d encodings, %d bytes.  Assemble: as --32 %s\n\n"
                % (total, total_bytes, s_path))
        f.write("\t.text\n")
        for enc in ENCODING_ORDER:
            rows = buckets[enc]
            if not rows:
                continue
            f.write("\n# ===== %s  (%d encodings) =====\n" % (enc, len(rows)))
            for bs, dis in rows:
                bytestr = ",".join("0x%02x" % b for b in bs)
                if dis:
                    f.write("\t.byte %s\t# %s\n" % (bytestr, dis))
                else:
                    f.write("\t.byte %s\n" % bytestr)

    print("wrote %s: %d encodings, %d bytes" % (s_path, total, total_bytes))
    for enc in ENCODING_ORDER:
        if buckets[enc]:
            print("  %-7s %d" % (enc, len(buckets[enc])))

    if args.no_build:
        return

    subprocess.run([args.gas, "--32", "-o", o_path, s_path], check=True)
    subprocess.run([args.objcopy, "-O", "binary", "-j", ".text", o_path, bin_path],
                   check=True)
    sz = os.path.getsize(bin_path)
    print("assembled %s (%d bytes)" % (bin_path, sz))
    if sz != total_bytes:
        print("WARNING: .bin size %d != expected %d" % (sz, total_bytes),
              file=sys.stderr)


if __name__ == "__main__":
    main()
