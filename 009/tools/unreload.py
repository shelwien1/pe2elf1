#!/usr/bin/env python3
"""Load a member once where the decompilation loads it over and over.

    python3 tools/unreload.py bmf.cpp
    python3 tools/unreload.py bmf.cpp --all

MSVC reloads a member between every pair of uses -- it cannot prove the store
in between does not alias the object it came from -- and Hex-Rays gives every
reload its own name, so mirroring five records into a row margin arrives as ten
statements and five cursors:

    v21 = blk->cursor[0];              P2Ctx *const p = blk->cursor[0];
    v21[1] = v21[-1];                  p[1] = p[-1];
    v25 = blk->cursor[0];        →     p[2] = p[-1];
    v25[2] = v25[-1];                  p[3] = p[-1];
    v28 = blk->cursor[0];              p[4] = p[-1];
    v28[3] = v28[-1];
    ...

The names are not five things. They are one thing loaded five times, and
reading the body as if they were five makes a margin mirror look like an
unrelated pile of cursors.

The "in a row" is the whole rule and it is easy to get wrong. A first version
of this counted every load of a member *anywhere* in a body and reported
fifty-eight in `alt_p1_model`; those are not a run, they are a function that
reads `ctx[0]` a lot, and folding them would have been a silent semantic
change. A run is loads separated only by lines that use the name just loaded.

A run qualifies when:

  * three or more statements load the *same* member expression -- `x->y` or
    `x->y[k]`, with an optional cast -- into a local each;
  * every one of those locals is declared with the same type, so folding them
    onto the first cannot change what a subscript steps by;
  * each local is used only between its own load and the next one;
  * nothing in the run assigns the member, or the object it hangs off, or any
    name the subscript reads;
  * the run contains no call, because a callee can write the member through a
    pointer this cannot see, and no label or `goto`, so control cannot enter
    the middle of it.

The later loads are deleted and their locals rewritten to the first one's,
which leaves the first `vNN` in place; `unused.py` clears the declarations the
run leaves behind.

Checked against the file from before `alt_p2_d8_decode_body` was collapsed by
hand, where it finds both of that body's runs and one more. Three versions of
it reported zero on that file for three different reasons -- a cast group that
ate the base expression, a scan that reset on every use line, and a run whose
recorded end was the last *load* rather than the last line, which left the
final store naming a local whose declaration had just been deleted. The last
of those got as far as the gate, which segfaulted on two streams.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import undup                                                      # noqa: E402

# A member is not the local of the same name: `blk->slot` is not `slot`.  A
# pattern that names an identifier has to say it is not reaching through one.
NAMED = r'(?<![\w.])(?<!->)%s\b'

LOAD = re.compile(r'^\s*(\w+) = ((?:\([\w\s*]+\)\s*)*)\(*'
                  r'([A-Za-z_]\w*->[\w\[\]]+)\)*;\s*$')
LABEL = re.compile(r'^\s*LABEL_\d+:')
# How many lines may sit between two loads of the same member.  MSVC's
# reload-per-store shape puts one or two; more than that is a body that
# happens to read the member twice, which is not this.
GAP = 4
CALL = re.compile(r'\b\w+\s*\(')
DECL = re.compile(r'^\s*((?:const\s+|static\s+)*[A-Za-z_]\w*)\s*([\s*]+)')


def types(code):
    """{name: type} for every local a declaration in this body names.

    A declaration is joined to its continuation lines first.  Hex-Rays writes
    one `int32_t` list across eight rows, and reading only the first row left
    every name on rows two to eight with no type at all -- which three tools
    read as "not a local in this body" and silently skipped.  `v124` in
    `alt_p2_context` was one of them.
    """
    out = {}
    joined, i = [], 0
    while i < len(code):
        l = code[i]
        if undup.declaration(l) and not l.rstrip().endswith(';'):
            j = i
            while j + 1 < len(code) and not code[j].rstrip().endswith(';'):
                j += 1
            l = ' '.join(r.strip() for r in code[i:j + 1])
            i = j
        joined.append(l)
        i += 1
    for l in joined:
        if not undup.declaration(l):
            continue
        m = DECL.match(l)
        if not m:
            continue
        ty = m.group(1)
        for d in l[m.end(1):].rstrip(' ;,\n').split(','):
            d = d.strip()
            n = re.match(r'\**\s*(\w+)', d)
            if n:
                out[n.group(1)] = (ty, d.count('*'))
    return out


def runs(lines, a, b):
    """[(first load, [(line, name)], base)] for every reload run in one body."""
    code = [l.split('//')[0] for l in lines[a:b + 1]]
    ty = types(code)
    # A run is loads of one member separated only by lines that use the name
    # just loaded -- `vA = base; vA[1] = vA[-1]; vB = base; vB[2] = vB[-1];`.
    # An earlier version counted every load of a member anywhere in the body
    # and reported 58 in `alt_p1_model`; those are not a run, they are a
    # function that reads `ctx[0]` a lot, and folding them would be wrong.
    out, cur, base, end = [], [], None, None

    def flush():
        if len(cur) >= 3:
            out.append((list(cur), base, end))

    for i, l in enumerate(code):
        m = LOAD.match(l)
        if m:
            if base is not None and m.group(3) == base and i - cur[-1][0] <= GAP \
                    and all(re.search(NAMED % re.escape(cur[-1][1]), code[k])
                            for k in range(cur[-1][0] + 1, i)):
                cur.append((i, m.group(1)))
                end = i
                continue
            flush()
            cur, base, end = [(i, m.group(1))], m.group(3), i
            continue
        # A line that uses the name just loaded is part of the run; anything
        # else ends it.  The first version reset on *every* non-load line,
        # which is every use line, so it found nothing at all -- including in
        # the function it was written from.  `end` has to follow those uses
        # too: the last load is not the last line of the run, and treating it
        # as one put the final store outside its own segment.
        if cur and re.search(NAMED % re.escape(cur[-1][1]), l) \
                and i - cur[-1][0] <= GAP:
            end = i
            continue
        flush()
        cur, base, end = [], None, None
    flush()

    keep = []
    for cur, base, end in out:
        lo, hi = cur[0][0], end
        names = [n for _i, n in cur]
        if len({ty.get(n) for n in names}) != 1 or ty.get(names[0]) is None:
            continue
        span = code[lo:hi + 1]
        text = '\n'.join(span)
        if any(LABEL.match(r) for r in span) or 'goto ' in text:
            continue
        # A call can write the member behind this tool's back.  The loads
        # themselves are not calls; anything else in the run that looks like
        # one disqualifies it.
        if any(CALL.search(r) for k, r in enumerate(span)
               if not LOAD.match(r)):
            continue
        # Nothing may assign the member, the object, or a name a subscript
        # reads -- the load lines are exempt, they are what is being folded.
        words = set(re.findall(r'\b([A-Za-z_]\w*)\b', base))
        body = '\n'.join(r for r in span if not LOAD.match(r))
        if any(re.search(r'\b%s\b\s*(?:\[[^\]]*\])?\s*(?:\+\+|--|[-+*/|&^%%]?=(?!=))'
                         % re.escape(w), body) for w in words):
            continue
        if re.search(r'%s\s*=(?!=)' % re.escape(base), body):
            continue
        # Each name may appear only in its own segment.
        ok = True
        bounds = [i for i, _n in cur] + [end + 1]
        for k, (_i, n) in enumerate(cur):
            for j, r in enumerate(code):
                if bounds[k] <= j < bounds[k + 1]:
                    continue
                if re.search(NAMED % re.escape(n), r) and not undup.declaration(r):
                    ok = False
                    break
            if not ok:
                break
        if ok:
            keep.append((a + lo, [(a + i, n) for i, n in cur], base, a + end))
    return keep


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found = []
    for a, b, nm, _ in structs.bodies(lines):
        for lo, cur, base, end in runs(lines, a, b):
            found.append((nm, lo, cur, base, end))

    if '--all' not in sys.argv:
        for nm, lo, cur, base, _e in found:
            print('%6d  %-24s %-26s %d loads' %
                  (lo + 1, nm.lstrip('_'), base[:26], len(cur)))
        print('%d reload runs, %d loads, %d removable'
              % (len(found), sum(len(c) for _n, _l, c, _b, _e in found),
                 sum(len(c) - 1 for _n, _l, c, _b, _e in found)))
        return 0

    drop = []
    for _nm, _lo, cur, _base, end in found:
        first = cur[0][1]
        # `end` is the last line of the run, not the last *load*: the final
        # store comes after it, and using the load line here left that store
        # naming a local whose declaration had just been deleted.
        bounds = [i for i, _n in cur] + [end + 1]
        for k, (i, n) in enumerate(cur):
            if k == 0:
                continue
            drop.append(i)
            for j in range(bounds[k], bounds[k + 1]):
                lines[j] = re.sub(NAMED % re.escape(n), first, lines[j])
    for i in sorted(set(drop), reverse=True):
        del lines[i]
    open(path, 'w').write('\n'.join(lines))
    print('%d reloads folded onto the first load' % len(drop))
    return 0


if __name__ == '__main__':
    sys.exit(main())
