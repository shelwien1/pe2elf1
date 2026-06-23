#!/usr/bin/env python3
"""
apxgen.py - build an APX instruction corpus (one invented example per Intel APX
instruction) and emit it in the canon-dump format for dump2asm.py.

Input (apx_examples.txt): one example per line, either
    <AT&T assembly>                 assembled with llvm-mc (64-bit, +apxf)
    =<hex bytes>\t<disasm comment>  raw bytes for forms no local assembler builds

For each example the bytes are produced (llvm-mc or raw) and disassembled with
`apxdis` (Zydis, APX mode) to get the comment; if Zydis can't decode it (e.g.
MOVRS, which this Zydis predates) the comment after the tab is used as fallback.

Output: canon-format lines "<hex bytes>\t<disasm>" on stdout. Pipe into
    python3 dump2asm.py - --bits 64 -o x86apx --title "..."
(or use --dump to write a file).
"""
import argparse
import os
import subprocess
import sys

LLVM_MC = os.environ.get("LLVM_MC", "llvm-mc-19")
APXDIS = os.environ.get("APXDIS", "./apxdis")
MATTR = "+apxf,+amx-tile,+usermsr,+ccmp,+cf,+egpr,+ndd,+nf,+zu"


def asm_bytes(line):
    """Assemble one AT&T instruction with llvm-mc; return list of ints or None."""
    src = ".text\n%s\n" % line
    p = subprocess.run([LLVM_MC, "-triple=x86_64", "-mattr=" + MATTR,
                        "--show-encoding"], input=src, capture_output=True,
                       text=True)
    for ln in p.stdout.splitlines():
        i = ln.find("# encoding: [")
        if i >= 0:
            hx = ln[i + len("# encoding: ["):].rstrip().rstrip("]")
            return [int(t, 16) for t in hx.split(",") if t.strip()]
    sys.stderr.write("asm failed: %s\n  %s\n" % (line, p.stdout.strip() or p.stderr.strip()))
    return None


def apxdis(bs):
    """Disassemble bytes with Zydis (APX); return disasm text or None."""
    hx = " ".join("%02x" % b for b in bs)
    p = subprocess.run([APXDIS], input=hx + "\n", capture_output=True, text=True)
    out = p.stdout.strip()
    if not out or "\t" not in out:
        return None
    return out.split("\t", 1)[1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("examples", help="apx_examples.txt")
    ap.add_argument("-o", "--out", help="write dump here (default: stdout)")
    args = ap.parse_args()

    rows = []
    errors = 0
    with open(args.examples) as f:
        for raw in f:
            line = raw.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            payload, _, fallback = line.partition("\t")
            fallback = fallback.strip()
            if payload.startswith("="):
                bs = [int(t, 16) for t in payload[1:].split()]
            else:
                bs = asm_bytes(payload)
                if bs is None:
                    errors += 1
                    continue
            dis = apxdis(bs) or fallback
            if not dis:
                sys.stderr.write("no disasm/fallback for: %s\n" % line)
                errors += 1
                continue
            rows.append((" ".join("%02x" % b for b in bs), dis))

    out = open(args.out, "w") if args.out else sys.stdout
    for hx, dis in rows:
        out.write("%s\t%s\n" % (hx, dis))
    if args.out:
        out.close()
    sys.stderr.write("apxgen: %d forms, %d errors\n" % (len(rows), errors))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
