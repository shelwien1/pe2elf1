#!/usr/bin/env python3
"""Retype the locals a function uses as pointers, from int32_t to char *.

    python3 tools/retype.py subs1.hpp __sub_416C90
    python3 tools/retype.py subs1.hpp --list

Hex-Rays could not tell an address from an int, so a function that walks a
structure does it like this:

    *(uint32_t *)(_this + 278736) = v17 + 144;

`_this` is an `int32_t` holding a pointer.  On a 32-bit target that works and
the compiler says nothing; at `-m64` it is 3718 `cast to pointer from integer of
different size` warnings, and a program that segfaults on the first image
because half of every address was thrown away.

This finds the locals a function uses that way -- declared `int32_t` (or
`uint32_t`), and appearing as the base of a `*(T *)(x + ...)` -- and declares
them `char *` instead.  Pointer arithmetic then means what the code already
assumed, on either word size.

It does not try to fix the fallout: assignments that now mix an int and a
pointer are left for the compiler to point at, because each one needs a decision
about which side was really the address.  Run it, build, and work through the
errors; `--list` says which functions have work and how much.
"""
import re
import sys

INT = r'(?:int32_t|uint32_t)'
# `char *` where the variable is only ever an address; `uintptr_t` where the
# code also does arithmetic on it that a pointer does not support -- masking it,
# or-ing a tag into it -- which is common in this decompilation and is still
# correct on a 64-bit target, unlike the int32_t it started as.
TYPE = 'char *'


def bodies(lines):
    out, depth, start, name = [], 0, None, None
    for i, l in enumerate(lines):
        s = l.split('//')[0]
        for k, ch in enumerate(s):
            if ch == '{':
                if depth == 0:
                    buf, j = s[:k], i
                    while True:
                        if buf.count(')') - buf.count('(') <= 0 and '(' in buf:
                            break
                        j -= 1
                        if j < 0:
                            break
                        buf = lines[j].split('//')[0] + ' ' + buf
                    m = re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', buf)
                    name = m.group(1) if m else '?'
                    start = i
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and start is not None:
                    out.append((start, i, name))
                    start = None
    return out


def pointer_bases(body):
    """Locals *dereferenced* as `*(T *)(x + ...)`.

    A bare `(T *)(x + n)` is not enough: Hex-Rays also builds fake pointers to
    carry integers around -- `v4 = (uint64_t *)(v3 + 16)` where v3 is a byte
    offset into a table and v4 is later read back as `(uint32_t)v4`.  Retyping
    the offset there would be exactly backwards.
    """
    text = '\n'.join(body)
    names = set()
    for m in re.finditer(r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)'
                         r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*[+)]', text):
        names.add(m.group(1))
    return names


def retype_params(lines, a, names):
    """Rewrite int32_t parameters of the signature at/above line a to char *."""
    j = a
    while j >= 0 and '(' not in lines[j]:
        j -= 1
    sig = lines[j]
    hit = [n for n in names if n in declared_params(sig)]
    for n in hit:
        rep = (r'uintptr_t \2' if TYPE == 'uintptr_t' else r'char *\2')
        sig = re.sub(r'\b(%s) (%s)\b' % (INT, re.escape(n)), rep, sig)
    lines[j] = sig
    return hit


def declared_params(sig):
    """int32_t/uint32_t parameters of a signature line."""
    out = {}
    m = re.search(r'\((.*)\)\s*$', sig.split('//')[0].strip())
    if not m:
        return out
    for i, part in enumerate(m.group(1).split(',')):
        pm = re.fullmatch(r'\s*(%s) ([A-Za-z_][A-Za-z0-9_]*)\s*' % INT, part)
        if pm:
            out[pm.group(2)] = i
    return out


def int_decls(body):
    """[(first line, last line, indent, type, [names])] for int32_t declarations.

    Hex-Rays wraps a long declaration over several lines, so this joins them:
    a declaration runs from a line that starts with the type to the line that
    ends with the `;`.  Members of the Phase 2 frame structs are skipped -- they
    are the layout, and moving one out moves it.
    """
    out, i, in_frame = [], 0, False
    while i < len(body):
        l = body[i]
        if re.match(r'^\s*struct alignas\(\d+\) \{', l):
            in_frame = True
        elif in_frame and re.match(r'^\s*\} __frame;', l):
            in_frame = False
            i += 1
            continue
        m = None if in_frame else re.match(r'^(\s+)(%s) (.*)$' % INT, l)
        if not m:
            i += 1
            continue
        j, text = i, m.group(3)
        while not text.rstrip().endswith(';') and j + 1 < len(body):
            j += 1
            text += ' ' + body[j].strip()
        if not text.rstrip().endswith(';'):
            i += 1
            continue
        names = [p.strip() for p in text.rstrip()[:-1].split(',')]
        out.append((i, j, m.group(1), m.group(2), names))
        i = j + 1
    return out


def declared_ints(body):
    """{plain int32_t local: index of its declaration}."""
    out = {}
    for k, (i, j, ind, ty, names) in enumerate(int_decls(body)):
        for n in names:
            if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', n):
                out[n] = k
    return out


def retype(body, names):
    """Move `names` out of their int32_t declarations into a `char *` one."""
    decls = int_decls(body)
    hit = sorted(n for n in names
                 if any(n in d[4] for d in decls))
    if not hit:
        return None, 0
    out, at, moved = [], 0, []
    for i, j, ind, ty, ns in decls:
        out.extend(body[at:i])
        keep = [n for n in ns if n not in hit]
        moved += [n for n in ns if n in hit]
        if keep:
            line = '%s%s %s;' % (ind, ty, ', '.join(keep))
            while len(line) > 96:
                cut = line.rfind(', ', 0, 96)
                if cut < 0:
                    break
                out.append(line[:cut + 1])
                line = ind + '        ' + line[cut + 2:]
            out.append(line)
        at = j + 1
    out.extend(body[at:])
    if not moved:
        return None, 0

    ind = re.match(r'^(\s*)', body[1]).group(1) if len(body) > 1 else '  '
    if TYPE == 'uintptr_t':
        line = '%suintptr_t %s;' % (ind, ', '.join(moved))
    else:
        line = '%schar *%s;' % (ind, ', *'.join(moved))
    decl = []
    while len(line) > 96:
        cut = line.rfind(', ', 0, 96)
        if cut < 0:
            break
        decl.append(line[:cut + 1])
        line = ind + '       ' + line[cut + 2:]
    decl.append(line + ('   // were int32_t: addresses, masked and tagged'
                        if TYPE == 'uintptr_t'
                        else '   // were int32_t: these hold addresses'))
    for i, l in enumerate(out):
        if l.strip() == ';':
            return out[:i + 1] + decl + out[i + 1:], len(moved)
    return [out[0]] + decl + out[1:], len(moved)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    what = sys.argv[2]

    if what == '--list':
        rows = []
        for a, b, n in bodies(lines):
            body = lines[a:b + 1]
            bases = pointer_bases(body)
            j = a
            while j >= 0 and '(' not in lines[j]:
                j -= 1
            names = (bases & set(declared_ints(body))) | \
                    (bases & set(declared_params(lines[j])))
            if names:
                rows.append((len(names), n, sorted(names)))
        for k, n, names in sorted(rows, reverse=True):
            print('%3d  %-22s %s' % (k, n, ' '.join(names[:8])))
        print('%d functions, %d locals' % (len(rows), sum(r[0] for r in rows)))
        return

    global TYPE
    argv = [a for a in sys.argv[3:] if a != '--uintptr']
    if '--uintptr' in sys.argv:
        TYPE = 'uintptr_t'
    skip = set(argv[0].split(',')) if argv else set()
    for a, b, n in bodies(lines):
        if n != what:
            continue
        body = lines[a:b + 1]
        bases = pointer_bases(body) - skip
        pk = retype_params(lines, a, bases)
        names = bases & set(declared_ints(body))
        new, k = retype(body, names)
        if new is None and not pk:
            sys.exit('%s: nothing to retype' % n)
        if new is None:
            new = lines[a:b + 1]
            k = 0
        open(path, 'w').write('\n'.join(lines[:a] + new + lines[b + 1:]))
        print('%s: %d locals, %d parameters -> %s' % (n, k, len(pk), TYPE))
        return
    sys.exit('%s: no such function' % what)


if __name__ == '__main__':
    main()
