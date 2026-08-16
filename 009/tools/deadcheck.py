#!/usr/bin/env python3
"""Report code nothing can reach.

    python3 tools/deadcheck.py bmf.cpp

Five kinds of unreachable code have been found in this file, each after the
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
  * **two locals compared that hold the same thing.** `model_planes` kept
    `if ( Srca_2 != Srca_1 )` after the `-E` block that gave `Srca_2` its own
    buffer was deleted, so both names were the caller's buffer and the
    interleave-and-free behind the test could not run. Coverage found this one;
    the check below is what makes it a rule instead of an anecdote.

The four shapes above are all *local* -- a label, a constant, a global, a call
graph -- and this fifth one is the first that needs to follow a value. It stays
deliberately small: only a local assigned exactly once, from a plain name, with
its address never taken. Anything cleverer would start reporting tests that are
merely usually equal, and a dead-code report is worth having only while every
line in it is dead.

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
# `__sub_410310` was the same body under the name it had before it was
# `unpredict_med`, kept so the pin held across the rename.  Both spellings were
# in the list for four rounds after the rename made the second one dead; the
# address it names is `unpredict_med`'s own, which `tools/addrmap.txt` records.
ALSO_PINNED = ('unpredict_med',)


def call_graph(lines):
    shim = {}
    for l in lines:
        m = re.match(r'^static inline .*?\b(__fwd_[A-Za-z0-9_]+)\s*\(.*?\)\s*\{\s*'
                     r'(?:return\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(', l)
        if m:
            shim[m.group(1)] = m.group(2)
        # `#define malloc bmf_malloc` -- the allocator is reached from a
        # hundred sites, none of which spell its name.  Same map as the `__fwd_`
        # shims: what a call site writes, and what it reaches.
        m = re.match(r'\s*#\s*define\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*$', l)
        if m:
            shim[m.group(1)] = m.group(2)
    # A body whose address is taken is reachable without a call:
    # `__set_new_handler(__out_of_memory_handler)` never writes a parenthesis
    # against the handler's name.  Only free functions -- taking a method's
    # address needs `&Class::m`, and accepting a bare `raw` or `p` as a
    # reference would let any local of that name mask a dead method.
    free = {n for _, _, n, _, d in structs.defs(lines) if d == 0}
    callers = {}
    for a, b, n, sig, _ in structs.defs(lines):
        # `static inline` bodies are skipped by the *report* below, because a
        # helper with no call site is a helper the compiler will drop rather
        # than a defect.  They were skipped here too, which is a different
        # thing and was wrong: a call made *from* one of them never became an
        # edge, so the first `static inline` helper to call a method --
        # `p2_rescale`, calling `P2Freq::rescale_three_way` -- made that method
        # look dead from its only call site.  Adding edges can only make this
        # tool more cautious.
        #
        # From *after* the opening brace, not from the start of the line that
        # holds it.  Every body here is written `void __foo(args) {`, so the
        # first line of the range is the declarator, and a pattern general
        # enough to see `bmf_new(` in a body also sees `__foo(` in `__foo`'s
        # own signature.  That made every function its own caller and the
        # never-called check vacuous: an injected body with no call sites
        # anywhere reported nothing, which is how it was caught.
        body = [lines[a].split('{', 1)[-1]] + lines[a + 1:b + 1]
        for l in body:
            # Any identifier followed by an argument list, not just a
            # `__`-prefixed one.  Every body recovered from the binary carries
            # that prefix and every helper this project added does not, so the
            # narrow pattern reported `bmf_new` dead from a hundred call sites.
            # Over-matching here only adds callers, which can only make the
            # tool more cautious; the keywords are excluded because `if (` is
            # not a call.
            for m in re.finditer(r'\b([A-Za-z_]\w*)\s*(?:<[^;<>]*>)?\s*\(', l):
                if m.group(1) in ('if', 'while', 'for', 'switch', 'return',
                                  'sizeof', 'do', 'else', 'catch'):
                    continue
                callers.setdefault(shim.get(m.group(1), m.group(1)),
                                   set()).add(n)
            # A method is reached as `p->rescale(x)`, which carries no `__`
            # prefix -- every body recovered from the binary has one and a
            # method this project introduced does not.  Without this the first
            # method added to the file was reported dead from two call sites.
            # `p->code_symbol(x)` and `p->alt_p2_d8_body<1>(x)` -- a method
            # reached through an explicit template argument list has no `(`
            # against its name, which hid the one method template in the tree.
            for m in re.finditer(r'(?:->|\.)\s*([A-Za-z_]\w*)\s*(?:<[^;<>]*>)?\s*\(', l):
                callers.setdefault(m.group(1), set()).add(n)
            # The name passed as an argument, not called: an operand position
            # is what tells a function pointer apart from a mention.
            for m in re.finditer(r'(?<![\w.>])([A-Za-z_]\w*)\s*(?=[,)])', l):
                if m.group(1) in free:
                    callers.setdefault(m.group(1), set()).add(n)
            # And the *first* arm of a conditional, which is followed by `:`
            # rather than by `,` or `)`.  `(p1 ? f : g)(args)` picks one of two
            # functions by name; the rule above sees `g` and not `f`, so
            # `alt_model_plane`'s four p1 entry points were reported dead while
            # their p2 twins on the same four lines were not.  The `?` is what
            # keeps a label or a `case` out, and the name still has to be one
            # this file defines.
            for m in re.finditer(r'\?\s*([A-Za-z_]\w*)\s*(?=:)', l):
                if m.group(1) in free:
                    callers.setdefault(m.group(1), set()).add(n)
    # A body that only calls itself is not called.  Recursion is a real edge
    # for every other question this graph is asked, but for `is anything left
    # that reaches this`, an edge from a body to itself answers yes about
    # nothing.  Two dead bodies that call *each other* are still missed; that
    # needs reachability from the entry points, not a caller count, and the
    # link-time report (`BMF_GC=list ./build.sh`) is what has it.
    for t, cs in callers.items():
        cs.discard(t)
    return callers


def closed_under(callers, roots):
    """Bodies reachable only from `roots` and from each other.

    The seed used to be `'__' + r`, from when every recovered body carried the
    prefix.  With the prefix gone that made a set of eight names nothing
    answers to, so nothing was ever closed under it and the predictor-test
    check below silently stopped looking at any body at all.
    """
    out = set(roots)
    for _ in range(8):
        for t, cs in callers.items():
            if cs and cs <= out:
                out.add(t)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'bmf.cpp'
    raw, origin = structs.splice(path)
    # A comment describing deleted code quotes it, and a check that reads
    # comments finds the thing it was written to find in the note saying it is
    # gone.  This one reported exactly that on its first run.
    lines = [l.split('//')[0] for l in raw]
    bad = 0

    class Where:
        """`path` and a line number, resolved back to the file it came from."""
        def __mod__(self, i):
            return '%s:%d' % origin[i - 1]
    path = Where()

    # A label with no `goto` left is a block that reads as a jump target and is
    # not one.  This matched `LABEL_\d+` only, which is the decompiler's
    # spelling and has not been in the tree for two rounds, so it was looking
    # for nothing among 30 labels.  Named labels, and the statement may sit on
    # the same line as the colon -- `keep_flag8: { … }` does.
    LAB = re.compile(r'^\s*(?!default\b|public\b|private\b|protected\b)'
                     r'([A-Za-z_]\w*):(?!:)')
    labels = {}
    for i, l in enumerate(lines):
        m = LAB.match(l)
        if m:
            labels.setdefault(m.group(1), []).append(i)
    sources = {}
    for i, l in enumerate(lines):
        for m in re.finditer(r'goto (\w+);', l.split('//')[0]):
            sources.setdefault(m.group(1), []).append(i)
    for name, at in sorted(labels.items()):
        if name not in sources:
            print('%s: %s has no goto left' % (path % (at[0] + 1), name))
            bad += 1

    for i, l in enumerate(lines):
        if re.search(r'\bif\s*\(\s*[01]\s*\)', l):
            print('%s: constant test: %s' % (path % (i + 1), l.strip()[:60]))
            bad += 1

    callers = call_graph(lines)
    closed = closed_under(callers, SUBSYSTEM_ROOTS) | set(ALSO_PINNED)
    for a, b, n, sig, _ in structs.defs(lines):
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
                print('%s: predictor test in %s, which the dispatch pins to '
                      '1 or 2' % (path % (i + 1), n.lstrip('_')))
                bad += 1

    for a, b, n, sig, _ in structs.defs(lines):
        if 'static inline' in sig:
            continue
        body = lines[a + 1:b + 1]
        # name -> what it was assigned, or None once it has been assigned twice,
        # assigned anything but a plain name, or had its address taken.
        once = {}
        for l in body:
            m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*'
                         r'([A-Za-z_][A-Za-z0-9_]*)\s*;\s*$', l)
            if m:
                once[m.group(1)] = None if m.group(1) in once else m.group(2)
                continue
            for m in re.finditer(r'(?<![\w.>])([A-Za-z_][A-Za-z0-9_]*)\s*'
                                 r'(?:=(?!=)|\+\+|--)', l):
                if m.group(1) in once:
                    once[m.group(1)] = None
            for m in re.finditer(r'&\s*([A-Za-z_][A-Za-z0-9_]*)', l):
                once[m.group(1)] = None
        for i in range(a + 1, b + 1):
            m = re.search(r'\bif\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*'
                          r'([!=]=)\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)', lines[i])
            if not m:
                continue
            p, q = once.get(m.group(1)), once.get(m.group(3))
            if p and q and p == q:
                print('%s: %s %s %s in %s, and both are %s'
                      % (path % (i + 1), m.group(1), m.group(2), m.group(3),
                         n.lstrip('_'), p))
                bad += 1

    called = {t for t, cs in callers.items() if cs}
    for a, b, n, sig, _ in structs.defs(lines):
        if 'static inline' in sig or n in called or n.startswith('operator '):
            continue
        if n.lstrip('_') in ('main', 'bmf_compress', 'bmf_decompress',
                             'bmf_addr', 'bmf_data_relocate', 'attribute__',
                             'bmf_set_denormal_mode'):
            continue
        print('%s: %s is never called' % (path % (a + 1), n.lstrip('_')))
        bad += 1

    print('%d findings' % bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
