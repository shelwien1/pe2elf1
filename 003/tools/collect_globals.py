#!/usr/bin/env python3
"""Collect subs1.hpp's global declarations into one sorted, deduplicated block.

The extractor emitted a typedef plus a reference declaration immediately ahead
of every function that touches a global, named after that function:

    typedef uint32_t t_sub_410650_n256_0;
    static t_sub_410650_n256_0& __sub_410650_n256_0 =
        *(t_sub_410650_n256_0*)(blob1 + 0x00443398 - BMF_BLOB_BASE);

so one address arrives under as many names as there are functions using it —
896 declarations for 328 distinct objects, scattered through 37k lines.  This
moves them all to the top of the file, sorts them by their address inside
BMF.exe's data segment, and keeps one declaration per object, under the name
IDA gave the address with the function prefix dropped (`__n256_0`).  Every
reference in the bodies is repointed at it.

Two details the bodies force:

  * Nine addresses are declared both as a scalar and as an array (the same
    `int` that one function reads is indexed by the next).  The array wins,
    and the scalar's uses become `__name[0]`.
  * Some functions declare a *local* with the same name as the global and
    reach the global through `::`.  Those locals keep their names, and only
    the `::`-qualified uses are repointed.
"""
import collections
import re
import sys

TD = re.compile(r'^typedef\s+(.+?)\s*(t_[A-Za-z0-9_]+)\s*((?:\[[^\]]*\])*)\s*;$')
GL = re.compile(r'^static\s+(t_[A-Za-z0-9_]+)&\s*(__[A-Za-z0-9_]+)\s*=\s*'
                r'\*\(\1\*\)\(blob1\s*\+\s*(0x[0-9A-Fa-f]+)\s*-\s*BMF_BLOB_BASE\);$')
# The eight declarations that were written by hand rather than generated.
HAND = re.compile(r'^static\s+.*\(blob1\s*\+\s*(0x[0-9A-Fa-f]+)\s*-\s*BMF_BLOB_BASE\).*;$')
# __<function>_<idaname>; the function part is sub_XXXXXX, main, or name_XXXXXX.
PFX = re.compile(r'^__(?:sub_[0-9A-Fa-f]{6}|main|start|[A-Za-z_][A-Za-z0-9_]*_[0-9A-Fa-f]{6})_(.+)$')

SKIP = re.compile(r'"(?:[^"\\\n]|\\.)*"' r"|'(?:[^'\\\n]|\\.)*'" r'|//[^\n]*' r'|/\*.*?\*/', re.S)
IDENT = re.compile(r'(::\s*)?\b(__[A-Za-z0-9_]+)\b')

NOT_A_TYPE = {'return', 'if', 'while', 'for', 'do', 'else', 'switch', 'case',
              'goto', 'break', 'continue', 'default', 'sizeof', 'alignas',
              'static', 'typedef', 'const'}
DECL = re.compile(r'^\s*(?:const\s+|alignas\([^)]*\)\s*)*'
                  r'([A-Za-z_][A-Za-z0-9_]*)'          # type
                  r'(?:\s*\*)*\s*&?\s*'
                  r'(__[A-Za-z0-9_]+)\s*(?:=|;|\[)')   # name


def code_spans(src):
    """Yield (start, end) of every stretch of src that is real code."""
    pos = 0
    for m in SKIP.finditer(src):
        if m.start() > pos:
            yield pos, m.start()
        pos = m.end()
    if pos < len(src):
        yield pos, len(src)


def function_bodies(src):
    """(start, end) of every top-level { ... }, i.e. every function body."""
    spans = list(code_spans(src))
    out, depth, start = [], 0, None
    for a, b in spans:
        for i in range(a, b):
            c = src[i]
            if c == '{':
                if depth == 0:
                    start = i
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    out.append((start, i + 1))
                elif depth < 0:
                    raise AssertionError('unbalanced brace at offset %d' % i)
    assert depth == 0, 'unbalanced braces: depth %d at EOF' % depth
    return out


def main(path):
    src = open(path).read()
    lines = src.split('\n')

    # ---- 1. read the declarations -----------------------------------------
    typedefs, entries, hand, drop = {}, [], [], set()
    for i, line in enumerate(lines):
        m = TD.match(line)
        if m:
            typedefs[m.group(2)] = (m.group(1).strip(), m.group(3))
            drop.add(i)
            continue
        m = GL.match(line)
        if m:
            base, arr = typedefs[m.group(1)]
            entries.append({'addr': int(m.group(3), 16), 'name': m.group(2),
                            'tname': m.group(1), 'base': base, 'arr': arr})
            drop.add(i)
            continue
        m = HAND.match(line)
        if m:
            hand.append((int(m.group(1), 16), line))
            drop.add(i)
    print('read %d typedefs, %d generated declarations, %d hand-written'
          % (len(typedefs), len(entries), len(hand)))
    assert len(typedefs) == len(entries), 'typedef/declaration count mismatch'

    # ---- 2. canonical name per address ------------------------------------
    by_addr = collections.defaultdict(list)
    for e in entries:
        m = PFX.match(e['name'])
        e['suffix'] = m.group(1) if m else e['name'][2:]
        by_addr[e['addr']].append(e)

    suffix_of = {}
    for addr, group in by_addr.items():
        sufs = {e['suffix'] for e in group}
        assert len(sufs) == 1, '%08X has several IDA names: %s' % (addr, sufs)
        suffix_of[addr] = sufs.pop()
    seen = {}
    for addr, suf in suffix_of.items():
        assert suf not in seen, 'IDA name %r used at %08X and %08X' % (suf, seen[suf], addr)
        seen[suf] = addr

    # Per address: the widest declaration wins.  Where a scalar and an array
    # view of the same base type exist, the array is the declaration and the
    # scalar's users are rewritten to index element 0.
    canon, rename = {}, {}
    for addr, group in by_addr.items():
        bases = {e['base'] for e in group}
        assert len(bases) == 1, '%08X: conflicting base types %s' % (addr, bases)
        arrs = {e['arr'] for e in group}
        assert len(arrs) <= 2 and '' in arrs or len(arrs) == 1, \
            '%08X: conflicting array shapes %s' % (addr, arrs)
        arr = max(arrs, key=len)
        suf = suffix_of[addr]
        canon[addr] = {'suffix': suf, 'base': bases.pop(), 'arr': arr}
        for e in group:
            rename[e['name']] = '__' + suf + ('[0]' if arr and not e['arr'] else '')
    n_scalar = sum(1 for v in rename.values() if v.endswith('[0]'))
    print('%d declarations -> %d objects (%d scalar uses become [0])'
          % (len(entries), len(canon), n_scalar))

    # ---- 3. locals that shadow a global name ------------------------------
    bodies = function_bodies(src)
    off = [0]
    for line in lines:
        off.append(off[-1] + len(line) + 1)

    def body_of(line_no):
        p = off[line_no]
        for a, b in bodies:
            if a <= p < b:
                return (a, b)
        return None

    shadow = collections.defaultdict(set)      # body -> {names}
    for i, line in enumerate(lines):
        if i in drop:
            continue
        m = DECL.match(line)
        if not m or m.group(1) in NOT_A_TYPE or m.group(2) not in rename:
            continue
        b = body_of(i)
        if b:
            shadow[b].add(m.group(2))
    shadowed = set().union(*shadow.values()) if shadow else set()
    print('%d locals shadow a global name, in %d functions'
          % (len(shadowed), len(shadow)))

    # ---- 4. rewrite ---------------------------------------------------------
    kept = '\n'.join(l for i, l in enumerate(lines) if i not in drop)
    # Recompute body spans on the text we are actually rewriting.
    bodies2 = function_bodies(kept)
    shadow2 = collections.defaultdict(set)
    kl = kept.split('\n')
    off2 = [0]
    for line in kl:
        off2.append(off2[-1] + len(line) + 1)
    for i, line in enumerate(kl):
        m = DECL.match(line)
        if not m or m.group(1) in NOT_A_TYPE or m.group(2) not in rename:
            continue
        p = off2[i]
        for a, b in bodies2:
            if a <= p < b:
                shadow2[(a, b)].add(m.group(2))
                break

    def shadow_at(pos):
        for span, names in shadow2.items():
            if span[0] <= pos < span[1]:
                return names
        return frozenset()

    stray = []

    def fixer(base):
        def fix(m):
            qual, name = m.group(1), m.group(2)
            if name not in rename:
                return m.group(0)
            if qual is None and name in shadowed:
                if name in shadow_at(base + m.start()):
                    return m.group(0)                 # the local, leave it
                stray.append((name, base + m.start()))
            return ('::' if qual else '') + rename[name]
        return fix

    out, pos = [], 0
    for a, b in code_spans(kept):
        out.append(kept[pos:a])
        out.append(IDENT.sub(fixer(a), kept[a:b]))
        pos = b
    out.append(kept[pos:])
    body = ''.join(out)
    assert not stray, 'bare use of a shadowed name outside its function: %r' % stray[:5]

    # Squeeze the blank runs the removed declarations left behind.
    body = re.sub(r'\n{3,}', '\n\n', body)

    # ---- 5. the block -------------------------------------------------------
    hdr = ['''// ---------------------------------------------------------------------------
// BMF's globals.
//
// Every global the decompiled bodies touch is a reference bound to a fixed
// offset inside blob.inc, BMF.exe's data segment (see bmf.cpp's BMF_BLOB).
// Hex-Rays named each one after the function it was recovered in, so the same
// object arrived under one name per user -- %d declarations for %d objects --
// each sitting just above its function.  They are collected here instead,
// sorted by address, one declaration per object, under the name IDA gave the
// address with the function prefix dropped.
//
// The typedefs carry the array shapes, which cannot be written inline in the
// reference declaration.  Where one body reads an address as a bare scalar and
// another indexes it, the array shape is the one declared and the scalar's
// users say [0].
//
// A few functions declare a local with the same name as the global they use
// and reach the global through `::`; those locals still carry their original
// `__sub_XXXXXX_` names, which is now what tells the two apart.
//
// incdec.md §6.1 is why the addresses are still the names: giving these
// objects real names is a much larger job than moving them, because the same
// address is an int to one function and a char[] to the next.
// ---------------------------------------------------------------------------''' % (len(entries) + len(hand), len(canon) + len(hand))]
    hand_by_addr = collections.defaultdict(list)
    for addr, line in hand:
        hand_by_addr[addr].append(line)
    for addr in sorted(set(canon) | set(hand_by_addr)):
        if addr in canon:
            c = canon[addr]
            t = 't_' + c['suffix']
            hdr.append('typedef %s %s%s;' % (c['base'], t, c['arr']))
            hdr.append('static %s& __%s = *(%s*)(blob1 + 0x%08X - BMF_BLOB_BASE);'
                       % (t, c['suffix'], t, addr))
        for line in hand_by_addr.get(addr, []):
            hdr.append(line)

    open(path, 'w').write('\n'.join(hdr) + '\n\n' + body.lstrip('\n'))
    print('wrote %d declarations (%d generated + %d hand-written) at the top of %s'
          % (len(canon) + len(hand), len(canon), len(hand), path))


if __name__ == '__main__':
    main(sys.argv[1])
