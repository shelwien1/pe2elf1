#!/usr/bin/env python3
# cover.py BIN -- per-instruction decode+round-trip coverage for a flat code blob.
#
# asm32 stops at the first opcode it can't decode, which hides everything after
# it. To see the WHOLE picture in one pass, this uses objdump purely for the
# instruction boundaries (offset+length), then feeds each instruction's bytes to
# asm32 in isolation and checks: did it decode, and does c(d(bytes)) == bytes?
# Failures are grouped by the objdump mnemonic so porting can be prioritised.

import subprocess, sys, os, re, collections

BIN = sys.argv[1] if len(sys.argv) > 1 else 'corpus2.bin'
data = open(BIN, 'rb').read()

# objdump -> [(offset, mnemonic)] for lines that carry a mnemonic; objdump wraps
# instructions longer than 7 bytes onto an offset-only continuation line (no
# mnemonic), so length is taken from the gap to the next mnemonic line / EOF.
out = subprocess.run(['objdump', '-D', '-b', 'binary', '-m', 'i386',
                      '-M', 'intel', BIN], capture_output=True, text=True).stdout
marks = []
for ln in out.splitlines():
    m = re.match(r'\s+([0-9a-f]+):\t([0-9a-f ]+?)\t\s*(\S+)', ln)
    if m:
        marks.append((int(m.group(1), 16), m.group(3)))
fixed = []
for k, (off, mn) in enumerate(marks):
    end = marks[k+1][0] if k+1 < len(marks) else len(data)
    fixed.append((off, end - off, mn))

ok = rterr = dec_fail = 0
bad = collections.Counter()
rt  = collections.Counter()
for off, nby, mn in fixed:
    blob = data[off:off+nby]
    open('/tmp/one.bin', 'wb').write(blob)
    subprocess.run(['./asm32', 'd', '/tmp/one.bin', '/tmp/one.dat'],
                   stderr=subprocess.DEVNULL)
    sz = os.path.getsize('/tmp/one.dat')
    if sz == 0:                                   # decoder produced nothing
        dec_fail += 1; bad[mn] += 1; continue
    subprocess.run(['./asm32', 'c', '/tmp/one.dat', '/tmp/one.out'],
                   stderr=subprocess.DEVNULL)
    if open('/tmp/one.out', 'rb').read() == blob:
        ok += 1
    else:
        rterr += 1; rt[mn] += 1

tot = len(fixed)
print(f"{BIN}: {tot} instructions")
print(f"  decode+roundtrip OK : {ok}")
print(f"  decode FAILED       : {dec_fail}")
print(f"  roundtrip MISMATCH  : {rterr}")
if bad:
    print("  -- not decoded (opcode not in corpus) --")
    for mn, n in bad.most_common():
        print(f"     {n:3}  {mn}")
if rt:
    print("  -- decoded but re-encode differs --")
    for mn, n in rt.most_common():
        print(f"     {n:3}  {mn}")
