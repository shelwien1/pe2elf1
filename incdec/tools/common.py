"""Shared helpers for the vbn incdec tools.  Paths are relative to incdec/."""
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                      # incdec/
PE = os.path.join(ROOT, '..', 'vbn.exe')
DONOR = os.path.join(ROOT, 'vbn.exe.c')

# vbn.exe layout (PE32, ImageBase 0x0FFF0000, no relocations)
SECTIONS = [  # (va, file offset, raw size, name)
    (0x0FFF1000, 0x0400, 0x0200, '.boot'),
    (0x0FFF3000, 0x0600, 0x0200, '.idata'),
    (0x10000000, 0x0800, 0xD400, '.vbn'),
    (0x14001000, 0xDC00, 0x0600, '.shim'),
]
CODE_LO, CODE_HI = 0x10000000, 0x1000A08C     # .vbn code; data starts at dword_1000A08C
DATA_LO, DATA_HI = 0x1000A08C, 0x14000000     # initialised data, then BSS up to MEMSIZE
CRT_LO = 0x1000704C                           # the C runtime starts here (call 10007052; ret)
IAT_VBN_INTERRUPT = 0x0FFF3064


def pe_bytes():
    return open(PE, 'rb').read()


def section_bytes(name):
    d = pe_bytes()
    for va, off, size, n in SECTIONS:
        if n == name:
            return va, d[off:off + size]
    raise KeyError(name)


def donor_lines():
    return open(DONOR, newline='').read().replace('\r\n', '\n').split('\n')


def load_funcs():
    """[{addr, name, start, end, sig}] from funcs.txt (written by mkfuncs.py)."""
    out = []
    for l in open(os.path.join(ROOT, 'funcs.txt')):
        if l.startswith('#') or not l.strip():
            continue
        a, name, s, e, sig = l.rstrip('\n').split('\t')
        out.append(dict(addr=int(a, 16), name=name, start=int(s), end=int(e), sig=sig))
    return out


def load_symbols():
    """name -> (va, kind, size, type) from vbn.txt."""
    out = {}
    for l in open(os.path.join(ROOT, 'vbn.txt')):
        if l.startswith('#') or not l.strip():
            continue
        p = l.rstrip('\n').split('\t')
        out[p[2]] = (int(p[0], 16), p[1], int(p[3]) if p[3] else None, p[4])
    return out


def split_params(inner):
    """Split a parameter/argument list on top-level commas."""
    out, depth, start = [], 0, 0
    for i, ch in enumerate(inner):
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        elif ch == ',' and depth == 0:
            out.append(inner[start:i])
            start = i + 1
    if inner[start:].strip():
        out.append(inner[start:])
    return [p.strip() for p in out]


def full_signature(lines, start):
    """The definition line(s) of a body: Hex-Rays wraps long __usercall lists."""
    sig = ''
    for l in lines[start - 1:start + 15]:
        sig += (' ' if sig else '') + l.strip()
        if sig.count('(') and sig.count('(') == sig.count(')'):
            break
    return ' '.join(sig.split())


STR_LIT = re.compile(r'"(?:[^"\\\n]|\\.)*"' + r"|'(?:[^'\\\n]|\\.)*'")


def literal_spans(text):
    return [(m.start(), m.end()) for m in STR_LIT.finditer(text)]


def disassemble():
    """[(va, objdump text)] for .vbn code, one objdump run per function so a
    zero-padded gap cannot desynchronise the linear sweep, plus the .shim thunks."""
    import subprocess, tempfile
    funcs = sorted(f['addr'] for f in load_funcs() if f['addr'] >= CODE_LO)
    out = []
    with tempfile.TemporaryDirectory() as td:
        for name in ('.vbn', '.shim'):
            va, data = section_bytes(name)
            p = os.path.join(td, name.strip('.') + '.bin')
            open(p, 'wb').write(data)
            ranges = ([(a, funcs[k + 1] if k + 1 < len(funcs) else CODE_HI) for k, a in enumerate(funcs)]
                      if name == '.vbn' else [(va, va + len(data))])
            for a, e in ranges:
                r = subprocess.run(['objdump', '-D', '-b', 'binary', '-m', 'i386',
                                    f'--adjust-vma={va:#x}', f'--start-address={a:#x}',
                                    f'--stop-address={e:#x}', p], capture_output=True, text=True).stdout
                for l in r.split('\n'):
                    m = re.match(r'\s*([0-9a-f]{8}):\t(.*)$', l)
                    if m:
                        out.append((int(m.group(1), 16), m.group(2)))
    return out
