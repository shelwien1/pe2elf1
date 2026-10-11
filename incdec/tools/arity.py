#!/usr/bin/env python3
"""Check conv.txt's parameter counts against the call sites in the asm.

A callee whose Hex-Rays signature has too few parameters is invisible to
extract.py: the call sites match the prototype, and the arguments the asm
pushes are simply not passed (mkdir, sub_1000862D, had none).  For every
`call sub_X` in the listing of each moved function, count the pushes since
the previous call or stack cleanup and compare them with conv.txt.  The count
is a heuristic (the prologue's register saves are skipped, any other push
counts); it is printed, not enforced.  Variadic callees are skipped: fixups.txt handles them.
"""
import os
import re
import sys
from common import ROOT, split_params

ASM = os.path.join(ROOT, 'vbn.exe.asm')


def conv_rows():
    out = {}
    for l in open(os.path.join(ROOT, 'conv.txt')):
        if l.startswith('#'):
            continue
        va, name, conv, pop, drop, proto, note = l.rstrip('\n').split('\t')
        params = proto.split('|', 1)[1] if '|' in proto else ''
        out[name] = (conv, None if '...' in params else len(split_params(params)))
    return out


def bodies(names):
    """name -> [instruction text] from the IDA listing."""
    lines = open(ASM, encoding='latin-1').read().split('\n')
    out, cur = {}, None
    for l in lines:
        m = re.match(r'^([0-9A-F]{8}) (sub_[0-9A-F]{8})\s+proc near', l)
        if m:
            cur = m.group(2) if m.group(2) in names else None
            if cur:
                out[cur] = []
            continue
        if cur and re.match(r'^[0-9A-F]{8} sub_[0-9A-F]{8}\s+endp', l):
            cur = None
            continue
        if cur:
            m = re.match(r'^[0-9A-F]{8}\s{10,}(\w+)\s*(.*?)\s*(;.*)?$', l)
            if m:
                out[cur].append((m.group(1), m.group(2)))
    return out


def main():
    conv = conv_rows()
    accepted = [l.strip() for l in open(os.path.join(ROOT, 'accepted.txt')) if l.strip() and not l.startswith('#')]
    seen = {}
    for fn, insns in bodies(set(accepted)).items():
        pushes = 0
        k = 0                                   # skip the prologue's register saves
        while k < len(insns) and (insns[k][0] == 'enter' or (insns[k][0] == 'push' and insns[k][1] in
                                  ('ebp', 'ebx', 'esi', 'edi')) or insns[k] == ('mov', 'ebp, esp')
                                  or (insns[k][0] == 'sub' and insns[k][1].startswith('esp'))):
            k += 1
        for op, args in insns[k:]:
            if op == 'push' and args not in ('ds', 'es', 'ebp'):
                pushes += 1
            elif op == 'call':
                m = re.match(r'(sub_[0-9A-F]{8})$', args)
                if m and m.group(1) in conv:
                    seen.setdefault(m.group(1), []).append((fn, pushes))
                pushes = 0
            elif (op == 'add' and args.startswith('esp')) or (op == 'pop' and args in ('ecx', 'eax', 'edx')) \
                    or op in ('leave', 'retn') or op.startswith('j'):
                pushes = 0                      # stack cleanup, or the end of a block
    bad = 0
    for callee in sorted(seen):
        cv, n = conv[callee]
        if n is None or cv in ('regs', 'asm'):
            continue
        counts = sorted({p for _, p in seen[callee]})
        if n not in counts or len(counts) > 1:
            sites = ', '.join(f'{fn}:{p}' for fn, p in seen[callee])
            print(f'{callee}: conv.txt has {n} parameter(s); pushes before the calls: {sites}')
            bad += n not in counts
    print(f'{len(seen)} callees checked; {bad} with no call site matching conv.txt')


if __name__ == '__main__':
    main()
