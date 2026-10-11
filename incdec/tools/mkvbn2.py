#!/usr/bin/env python3
"""mkvbn2.py: write vbn2/vbn_ur.cpp, vbn's plain `u` and `r` as one
standalone C++ file, from what incdec moved.  ../vbn2.cpp began as this file
and is maintained by hand since (vbn2 c|d).

  vbn2/head.cpp      the native layer: types, vbn's memory, DOS, the C runtime
  inc/<name>.inc     each moved function, in accepted.txt's order (callees
                     first), or vbn2/override/<name>.inc where the incdec body
                     only makes sense inside vbn.elf (the DOS wrappers, which
                     called the shim, and the assembly glue)
  vbn2/tail.cpp      main()
  vbn.exe's .vbn section, 0x1000A08C..0x1000D400: the initialized data

On the way it drops what only the injected build needs: probes, the entry
attributes, and the `#ifndef __PE_DECL_...` declarations of functions left in
vbn.exe, which head.cpp replaces (it defines each such macro).  Any reference
to vbn.exe's code that would survive is an error.
"""
import os
import re
import sys
from common import ROOT, DATA_LO, section_bytes

OUT = os.path.join(ROOT, 'vbn2', 'vbn_ur.cpp')
V2 = os.path.join(ROOT, 'vbn2')


def code_part(line):
    """line without a // comment (no string in these bodies contains //)."""
    i = line.find('//')
    return line if i < 0 else line[:i]


def body(name, provided):
    path = os.path.join(V2, 'override', name + '.inc')
    if not os.path.exists(path):
        path = os.path.join(ROOT, 'inc', name + '.inc')
    out, skip = [], None
    for line in open(path).read().split('\n'):
        m = re.match(r'#ifndef (__PE_DECL_\w+)\s*$', line)
        if m:
            if m.group(1) not in provided:
                sys.exit(f'{name}: {m.group(1)[10:]} is neither moved nor provided by vbn2/head.cpp')
            skip = m.group(1)
            continue
        if skip:
            if line.startswith('#endif'):
                skip = None
            continue
        if re.match(r'\s*PROBE_(DECL|HIT)\(\w+\);?\s*$', line):
            continue
        line = re.sub(r'\bVBN_ENTRY\s+', '', line)
        # sub_1000703B(0); sub_X(arg) -> vbn_restartable(__sub_X, arg)
        line = re.sub(r'vbn_restartable\(\(void \*\)0x([0-9A-F]{8}),', r'vbn_restartable(__sub_\1,', line)
        line = line.replace(': see head.cpp', ': see vbn_restartable')
        for a in re.findall(r'\b0x([0-9A-Fa-f]{8})\b', code_part(line)):
            if 0x10000000 <= int(a, 16) < DATA_LO:
                sys.exit(f'{name}: still refers to vbn.exe code at 0x{a}: {line.strip()}')
        out.append(line)
    while out and not out[-1].strip():
        out.pop()
    return '\n'.join(out) + '\n'


def c_string(data):
    """data as C string literal lines; text stays readable.  A line breaks
    after 96 characters, or after a string's NUL once it is half full."""
    lines, cur, text = [], '', False
    for b in data:
        if b in (0x22, 0x5C):
            cur += '\\' + chr(b)
        elif b == 0x3F:
            cur += '\\?'                       # no trigraphs
        elif 0x20 <= b < 0x7F:
            cur += chr(b)
        else:
            cur += '\\%03o' % b
        text |= b != 0
        if len(cur) >= 96 or (b == 0 and text and len(cur) >= 48):
            lines.append(cur)
            cur, text = '', False
    if cur:
        lines.append(cur)
    return '\n'.join('  "%s"' % l for l in lines)


def main():
    head = open(os.path.join(V2, 'head.cpp')).read()
    provided = set(re.findall(r'^#define (__PE_DECL_\w+)', head, re.M))
    names = [l.strip() for l in open(os.path.join(ROOT, 'accepted.txt'))
             if l.strip() and not l.startswith('#')]
    va, raw = section_bytes('.vbn')
    data = raw[DATA_LO - va:]
    parts = [head]
    for n in names:
        parts.append('\n' + body(n, provided))
    parts.append(open(os.path.join(V2, 'tail.cpp')).read())
    parts.append('\n// vbn\'s initialized data, 0x%08X..0x%08X (vbn.exe\'s .vbn section, from file\n'
                 '// offset 0x%X): its messages and tables, and the C runtime\'s variables.\n'
                 % (DATA_LO, va + len(raw), 0x800 + DATA_LO - va))
    parts.append('const unsigned char image_data[] =\n' + c_string(data) + ';\n')
    parts.append('const size_t image_data_size = sizeof image_data - 1;     // %d bytes\n' % len(data))
    with open(OUT, 'w') as f:
        f.write(''.join(parts))
    print(f'wrote {os.path.relpath(OUT)}: {len(names)} functions, {len(data)} bytes of data')


if __name__ == '__main__':
    main()
