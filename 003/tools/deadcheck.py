#!/usr/bin/env python3
"""Report code nothing can reach.

    python3 tools/deadcheck.py subs1.hpp

Four kinds of unreachable code have been found in this file, each after the
previous one had been declared the last. Two were found by a check that was
written *after* the case it would have caught, which is the wrong order, so
this is the check standing on its own:

  * **a label with no `goto` left.** `LABEL_47` lost its last source when the
    block holding it was deleted, and nothing said so.
  * **an always-true or always-false test as a statement.** `if ( 1 ) goto
    LABEL_52` was left by the mode-folding pass, and everything between it and
    its label was dead.
  * **a test on a global the dispatch pins, with a live `else`.** Both
    spellings: `if ( !plane_predictor )` and `if ( plane_predictor ) … else`.
    The first search only looked for the first spelling and missed a 22-line
    else that had been there all along.
  * **a body nothing calls.** `--gc-sections` reports these at link time
    (`BMF_GC=list ./build.sh`); this reports them from the source, which is
    where they have to be deleted.

A pinned global is one that is only ever assigned a constant: `plane_predictor`
is not one in general, but inside the closed set of bodies reachable only from
the alternate model dispatch it is 1 or 2, and `near_lossless_max[0]` is 0
everywhere because `-E` is. Those two facts are given here rather than derived,
and each is checked at the point it is used.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

# Reachable only from the alternate model dispatch, which tests the predictor
# against 1 and 2 and nothing else.  REFACTORING.md §2.3.
SUBSYSTEM_ROOTS = ('alt_model_p1_encode', 'alt_model_p1_decode',
                   'alt_model_p1_d8_encode', 'alt_model_p1_d8_decode',
                   'alt_model_p2_encode', 'alt_model_p2_decode',
                   'alt_model_p2_d8_encode', 'alt_model_p2_d8_decode')

# Outside that set but pinned for the same reason: expand_image calls
# unpredict_med from two places and both guard on the predictor being 1 -- the
# second through a copy of it stored and reloaded through p_i.
ALSO_PINNED = ('__unpredict_med', '__sub_410310')


def call_graph(lines):
    shim = {}
    for l in lines:
        m = re.match(r'^static inline .*?\b(__fwd_[A-Za-z0-9_]+)\s*\(.*?\)\s*\{\s*'
                     r'(?:return\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(', l)
        if m:
            shim[m.group(1)] = m.group(2)
    callers = {}
    for a, b, n, sig in structs.bodies(lines):
        if 'static inline' in sig:
            continue
        for l in lines[a:b + 1]:
            for m in re.finditer(r'\b(__[A-Za-z0-9_]+)\s*\(', l):
                callers.setdefault(shim.get(m.group(1), m.group(1)),
                                   set()).add(n)
    return callers


def closed_under(callers, roots):
    """Bodies reachable only from `roots` and from each other."""
    out = {'__' + r for r in roots}
    for _ in range(8):
        for t, cs in callers.items():
            if cs and cs <= out:
                out.add(t)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    # A comment describing deleted code quotes it, and a check that reads
    # comments finds the thing it was written to find in the note saying it is
    # gone.  This one reported exactly that on its first run.
    lines = [l.split('//')[0] for l in open(path).read().split('\n')]
    bad = 0

    labels = {}
    for i, l in enumerate(lines):
        m = re.match(r'^\s*(LABEL_\d+):\s*$', l)
        if m:
            labels.setdefault(m.group(1), []).append(i)
    sources = {}
    for i, l in enumerate(lines):
        for m in re.finditer(r'goto (LABEL_\d+);', l):
            sources.setdefault(m.group(1), []).append(i)
    for name, at in sorted(labels.items()):
        if name not in sources:
            print('%s:%d: %s has no goto left' % (path, at[0] + 1, name))
            bad += 1

    for i, l in enumerate(lines):
        if re.search(r'\bif\s*\(\s*[01]\s*\)', l):
            print('%s:%d: constant test: %s' % (path, i + 1, l.strip()[:60]))
            bad += 1

    callers = call_graph(lines)
    closed = closed_under(callers, SUBSYSTEM_ROOTS) | set(ALSO_PINNED)
    for a, b, n, sig in structs.bodies(lines):
        if 'static inline' in sig or n not in closed:
            continue
        for i in range(a, b + 1):
            if not re.search(r'\bif\s*\(\s*!?\s*(::)?plane_predictor\s*\)',
                             lines[i]):
                continue
            j, d = i + 1, 0
            while j <= b:
                d += lines[j].count('{') - lines[j].count('}')
                if d == 0 and lines[j].strip() == '}':
                    break
                j += 1
            k = j + 1
            while k <= b and not lines[k].strip():
                k += 1
            negated = '!' in lines[i].split('plane_predictor')[0]
            if negated or (k <= b and lines[k].strip() == 'else'):
                print('%s:%d: predictor test in %s, which the dispatch pins to '
                      '1 or 2' % (path, i + 1, n.lstrip('_')))
                bad += 1

    called = set()
    for t, cs in callers.items():
        called.add(t)
    for a, b, n, sig in structs.bodies(lines):
        if 'static inline' in sig or n in called:
            continue
        if n.lstrip('_') in ('main', 'bmf_compress', 'bmf_decompress',
                             'bmf_addr', 'bmf_data_relocate', 'attribute__',
                             'bmf_set_denormal_mode', 'alignas'):
            continue
        print('%s:%d: %s is never called' % (path, a + 1, n.lstrip('_')))
        bad += 1

    print('%d findings' % bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
