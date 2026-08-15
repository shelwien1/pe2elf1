#!/usr/bin/env python3
"""Delete the functions in a flat C file that nothing can reach any more.

Reachability, not heuristics: the file is split into its top-level function
definitions, each one's body is scanned for identifiers, and everything not
reachable from the entry points is removed.  A function pointer counts as a
reference, so a body that only ever has its address taken survives.

**Top-level bodies only, and it now says so instead of guessing.**  A class
body is a single top-level `{ … }` whose head has no parameter list, so the
whole block -- methods, and every identifier they mention -- fell out of the
graph, and nine bodies this program calls only from a method came back
unreachable.  In a tool that deletes what it names, that is not a report.
Given a unit with classes in it this declines, and `deadcheck.py` is the check
that answers.

    python3 tools/prune_unreachable.py subs1.hpp            # report only
    python3 tools/prune_unreachable.py subs1.hpp --apply    # rewrite the file

Roots are `main` plus anything bmf.cpp itself names.  Whole function
definitions go, and then any entry of the globals block that no surviving body
names -- an unused reference into blob.inc costs nothing in the binary, but it
claims an object is in play when it is not, which is what the block is there
to answer.
"""
import os
import re
import sys

SKIP = re.compile(r'"(?:[^"\\\n]|\\.)*"' r"|'(?:[^'\\\n]|\\.)*'" r'|//[^\n]*' r'|/\*.*?\*/', re.S)
IDENT = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]*\b')
# The name in a definition is the identifier in front of the parameter list.
SIGNAME = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{]*$', re.S)
ATTR = re.compile(r'__attribute__\s*\(')


def strip_attributes(sig):
    """Drop __attribute__((...)) so it is not mistaken for the name."""
    while True:
        m = ATTR.search(sig)
        if not m:
            return sig
        i, depth = m.end() - 1, 0
        while i < len(sig):
            if sig[i] == '(':
                depth += 1
            elif sig[i] == ')':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        sig = sig[:m.start()] + sig[i + 1:]


def code_spans(src):
    pos = 0
    for m in SKIP.finditer(src):
        if m.start() > pos:
            yield pos, m.start()
        pos = m.end()
    if pos < len(src):
        yield pos, len(src)


def top_level_blocks(src):
    """(start, end) of every top-level { ... }."""
    out, depth, start = [], 0, None
    for a, b in code_spans(src):
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
    assert depth == 0, 'unbalanced braces'
    return out


def identifiers(text):
    out = set()
    for a, b in code_spans(text):
        out.update(IDENT.findall(text[a:b]))
    return out


def parse(src):
    """[(name, start, end, body)] for each top-level function definition."""
    blocks = top_level_blocks(src)
    funcs, prev = [], 0
    for start, end in blocks:
        head = src[prev:start]
        # The definition begins after the last statement/declaration that ended
        # before it; comments and blank lines in between are not part of it.
        cut = max(head.rfind(';'), head.rfind('}'), head.rfind('\n\n'))
        sig = head[cut + 1:]
        # A preprocessor line is never part of a signature, and leaving it in
        # made `#pragma pack(push, 1)` above `struct BITMAPFILEHEADER {` the
        # definition of a function called `pack`.
        clean = strip_attributes(re.sub(r'^[ \t]*#[^\n]*', '',
                                        re.sub(r'//[^\n]*', '', sig), flags=re.M))
        # A brace that follows an `=` opens an initializer, not a body.  Without
        # this the tool read `alignas(16) static uint8_t ctx_group_flags[32] = {`
        # as a definition of a function called `alignas` -- SIGNAME asks for the
        # last `name(` with no `;` or `{` after it, and `alignas(16)` is one --
        # and there are eight of those, so it died on the second before it could
        # report anything at all.
        if clean.rstrip().endswith('='):
            prev = end
            continue
        m = SIGNAME.search(clean)
        if m:
            funcs.append({'name': m.group(1), 'sig_at': prev + cut + 1,
                          'end': end, 'text': sig + src[start:end]})
        prev = end
    return funcs


def main():
    path = sys.argv[1]
    apply_it = '--apply' in sys.argv[2:]
    src = open(path).read()
    # A class body is one top-level `{ … }` and its head has no parameter list,
    # so `parse` finds no name for it and drops the block -- with every method
    # in it, and with every identifier those methods mention.  The graph then
    # has no edge from a method to anything, and the nine bodies this program
    # calls only from a method came back as unreachable.  `sym_in_top` is one,
    # and `model.inc` calls it four times.
    #
    # That is a false positive in a tool whose `--apply` deletes what it names,
    # which is not a thing to leave running while it is wrong.  The same
    # question is asked correctly by `deadcheck.py`, which reads the unit with
    # `structs.defs` and so opens a body at any depth; this declines rather
    # than answers.
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import structs
    if any(d for _a, _b, _n, _s, d in structs.defs(src.split('\n'))):
        print('not applicable: %s defines methods, and this reads only '
              'top-level bodies -- deadcheck.py is the reachability check for '
              'a unit with classes in it' % os.path.basename(path))
        return
    funcs = parse(src)
    names = {}
    for f in funcs:
        # Bare names, and they have to stay unique across the file -- including
        # across classes.  Reachability below is `identifiers(text) & names`, so
        # a call site says `release`, not `AltP2Block::release`; keying the map
        # on the qualified name would have to map one identifier back to several
        # definitions, and getting that wrong in a tool that *deletes* code is a
        # different order of mistake from getting it wrong in one that reports.
        #
        # It fired once, when `AltP1Block` and `AltP2Block` were both given a
        # method called `release`.  They are `alt_p1_free` and `alt_p2_free`,
        # which is what the allocators beside them are called anyway.
        assert f['name'] not in names, 'two definitions of %s' % f['name']
        names[f['name']] = f
    print('%d function definitions' % len(funcs))

    # Roots: whatever the including translation unit names, plus main.
    host = os.path.join(os.path.dirname(os.path.abspath(path)), 'bmf.cpp')
    roots = {'main'}
    if os.path.exists(host):
        outer = open(host).read()
        outer = outer.split('#include "subs1.hpp"')[0]
        roots |= identifiers(outer) & set(names)
    print('roots: %s' % ', '.join(sorted(roots)))

    refs = {f['name']: identifiers(f['text']) & set(names) for f in funcs}
    seen, work = set(), list(roots)
    while work:
        n = work.pop()
        if n in seen:
            continue
        seen.add(n)
        work.extend(refs.get(n, ()) - seen)

    dead = [f for f in funcs if f['name'] not in seen]
    live = len(funcs) - len(dead)
    print('reachable: %d   unreachable: %d' % (live, len(dead)))
    for f in sorted(dead, key=lambda f: f['name']):
        print('  %s' % f['name'])
    if not apply_it:
        return
    if not dead:
        new = drop_unused_globals(src)
        if new != src:
            open(path,'w').write(new)
        return

    keep, pos, out = sorted(dead, key=lambda f: f['sig_at']), 0, []
    for f in keep:
        out.append(src[pos:f['sig_at']])
        pos = f['end']
        while pos < len(src) and src[pos] == '\n':
            pos += 1
    out.append(src[pos:])
    new = re.sub(r'\n{3,}', '\n\n', ''.join(out))
    new = drop_unused_globals(new)
    open(path, 'w').write(new)
    print('removed %d definitions, %d lines -> %d'
          % (len(dead), src.count('\n'), new.count('\n')))


GLOBAL = re.compile(r'^static\s+(?:(t_[A-Za-z0-9_]+)&\s*)?[^=]*?(__[A-Za-z0-9_]+)\s*=.*blob1 \+ 0x[0-9A-Fa-f]+.*;$')


def drop_unused_globals(src):
    """Drop the globals block's entries that no surviving body names.

    The declarations are references into blob.inc, so an unused one costs
    nothing in the binary -- but it claims an object is in play when it is
    not, which is exactly what the block is there to answer.
    """
    lines = src.split('\n')
    decls = {}                       # name -> (line index, typedef name)
    for i, line in enumerate(lines):
        m = GLOBAL.match(line)
        if m:
            decls[m.group(2)] = (i, m.group(1))
    rest = '\n'.join(l for i, l in enumerate(lines)
                     if i not in {d[0] for d in decls.values()}
                     and not l.startswith('typedef '))
    used = identifiers(rest)
    dead = {n: v for n, v in decls.items() if n not in used}
    if not dead:
        return src
    drop = {v[0] for v in dead.values()}
    tnames = {v[1] for v in dead.values() if v[1]}
    for i, line in enumerate(lines):
        m = re.match(r'^typedef\s+.*?(t_[A-Za-z0-9_]+)\s*(?:\[[^\]]*\])*\s*;$', line)
        if m and m.group(1) in tnames:
            drop.add(i)
    print('globals: %d declared, %d unused -> removed (%s)'
          % (len(decls), len(dead), ', '.join(sorted(dead))))
    return '\n'.join(l for i, l in enumerate(lines) if i not in drop)


if __name__ == '__main__':
    main()
