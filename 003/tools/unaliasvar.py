#!/usr/bin/env python3
"""Fold a local that is one assignment of another local.

    python3 tools/unaliasvar.py subs1.hpp --list
    python3 tools/unaliasvar.py subs1.hpp --all

MSVC spills a value and reloads it into a second register, and Hex-Rays gives
each register a name, so one quantity arrives as a chain:

    v110 = (P2Ctx *)block->cursor[1];
    v293 = v110;
    v160 = v293;   v173 = v293;   v194 = v293;   v204 = v293;

Six names for `cursor[1]`.  Nothing distinguishes them and every read has to be
traced back by hand to find out which row it is on.  Folded, `v293[3].lane[3]`
and `v173[1].lane[0]` are visibly the same row.

The condition is narrow and checked, not assumed:

  * the alias is assigned exactly once in the body, and its whole right-hand
    side is one identifier;
  * that identifier is a local of the same body, also assigned exactly once;
  * neither is a parameter, and neither has its address taken -- `&v` or a
    reference binding means something else can write it;
  * the alias's assignment is not the only statement of an `if` or a loop, so
    removing the line cannot change what the branch governs.

The last two are what stop this being a dataflow question.  A single-assignment
local whose address is never taken holds one value for the whole body, so an
alias of it holds the same value wherever it is read -- no ordering argument is
needed, and the fold is safe even where the alias is assigned inside a branch
and read outside it.

What it will not do is fold a variable with two lifetimes.  `sub0_row` is
`cursor[2]` for thirty lines and `cursor[3]` after, and both have readers; it
is assigned twice, so it fails the first condition and is left alone.  That
case has to be split by hand, and the split is a claim about the two ranges
rather than about the variable.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs

ASSIGN = re.compile(r'^(\s*)(\w+) = ([^=;]+);\s*$')
# Every other way a line can write a local.  Counting only `v = ...` calls a
# variable single-assignment while `v += 144`, `++v` and `LOWORD(v) = x` are
# still writing it -- which is how the first version of this tool changed three
# reference streams.
WRITES = [
    re.compile(r'\b%s\s*(?:\+\+|--)'),          # v++  v--
    re.compile(r'(?:\+\+|--)\s*%s\b'),          # ++v  --v
    re.compile(r'\b%s\s*[-+*/|&^%%]?=(?!=)'),    # v = / v += / v |= ...
    re.compile(r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)\s*='),
    re.compile(r'\b(?:BYTE|WORD|DWORD)\d\s*\(\s*%s\s*\)\s*='),
]


def written(v, code):
    """Does this line write `v` in any of the ways MSVC's output can?"""
    q = re.escape(v)
    return any(re.search(p.pattern % q, code) for p in WRITES)
# A declaration line: rewriting one turns `int32_t v4, v22;` into
# `int32_t v4, v4;`, so those are left for `unused.py` to clear afterwards.
DECL = re.compile(r'^\s*(?:const\s+)?(?:u?int\d+_t|uintptr_t|size_t|float|double'
                  r'|void|char|bool|unsigned|signed|[A-Z]\w*)[\w ,\*&()\[\]]*[;,]\s*$')


def candidates(lines):
    """[(body name, alias, source, decl line, assign line)]."""
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        writes, addr, touched = {}, set(), {}
        for i in range(a, b + 1):
            code = lines[i].split('//')[0]
            m = ASSIGN.match(code)
            if m:
                writes.setdefault(m.group(2), []).append((i, m.group(3).strip()))
            for w in re.findall(r'\b(\w+)\b', code):
                if w not in touched and written(w, code):
                    touched.setdefault(w, 0)
                    touched[w] += 1
                elif written(w, code):
                    touched[w] += 1
            for w in re.findall(r'&(\w+)\b', code):
                addr.add(w)
            for w in re.findall(r'\b(\w+)\s*&\s*\w+\s*=', code):
                addr.add(w)
        params = set(re.findall(r'\b(\w+)\s*[,)]', sig))
        # Only names *declared in this body*.  Without this the scan treats a
        # global as a local the moment a body assigns it once -- `plane_alt_model
        # = v16` looks exactly like an alias, and folding it hides a write that
        # four other functions read.  That is what broke three reference streams.
        declared, indecl = set(), False
        for i in range(a, b + 1):
            code = lines[i].split('//')[0]
            if DECL.match(code) or indecl:
                declared |= set(re.findall(r'\b(\w+)\b', re.sub(
                    r'^\s*(?:const\s+)?[\w]+', '', code) if not indecl else code))
                indecl = not code.rstrip().endswith(';')
        for v, ws in writes.items():
            if v not in declared or len(ws) != 1 or v in addr or v in params:
                continue
            if touched.get(v, 0) != 1:      # written some other way too
                continue
            i, rhs = ws[0]
            if not re.fullmatch(r'\w+', rhs):
                continue
            if rhs not in writes or len(writes[rhs]) != 1:
                continue
            if rhs not in declared or rhs in addr or rhs in params \
                    or touched.get(rhs, 0) != 1:
                continue
            # the assignment must not be the whole of an `if` or a loop
            prev = lines[i - 1].strip()
            if re.fullmatch(r'(if|while|for|else if)\s*\(.*\)', prev) or prev == 'else':
                continue
            out.append((nm, v, rhs, i))

    # Two aliases can name each other -- `a = b` and `b = a` in one body, which
    # is a rotation carried across a loop, not a copy.  Following each chain to
    # a root drops those, and resolves `x = y; y = z` to `x -> z` in one pass.
    kept = []
    for nm in {c[0] for c in out}:
        src = {v: r for n, v, r, _ in out if n == nm}
        for n, v, r, i in [c for c in out if c[0] == nm]:
            seen, root = {v}, r
            while root in src:
                if root in seen:
                    root = None
                    break
                seen.add(root)
                root = src[root]
            if root is not None:
                kept.append((nm, v, root, i))
    return kept


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, v, rhs, i in found:
            print('%6d  %-22s %s = %s' % (i + 1, nm.lstrip('_'), v, rhs))
        print('%d aliases of a single-assignment local' % len(found))
        return 0

    # Fold one body at a time so a name reused in another body is untouched.
    by_body = {}
    for nm, v, rhs, i in found:
        by_body.setdefault(nm, []).append((v, rhs, i))
    bodies = {nm: (a, b) for a, b, nm, _ in structs.bodies(lines)}
    drop, n = [], 0
    for nm, items in by_body.items():
        a, b = bodies[nm]
        for v, rhs, i in items:
            indecl = False
            for k in range(a, b + 1):
                code = lines[k].split('//')[0]
                # A declaration, or a continuation of one: the type is on the
                # first line only, so the state has to be carried down.
                if DECL.match(code):
                    indecl = not code.rstrip().endswith(';')
                    continue
                if indecl:
                    indecl = not code.rstrip().endswith(';')
                    continue
                lines[k] = re.sub(r'\b%s\b' % re.escape(v), rhs, lines[k])
            drop.append(i)
            n += 1
    for i in sorted(drop, reverse=True):
        if re.fullmatch(r'\s*(\w+) = \1;\s*', lines[i]):
            del lines[i]
    open(path, 'w').write('\n'.join(lines))
    print('%d aliases folded' % n)
    return 0


if __name__ == '__main__':
    sys.exit(main())
