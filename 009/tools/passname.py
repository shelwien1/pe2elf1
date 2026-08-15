#!/usr/bin/env python3
"""An expression given a name and then written out again on the next line.

    python3 tools/passname.py bmf.cpp             # report
    python3 tools/passname.py bmf.cpp --list      # and every site
    python3 tools/passname.py one.inc --apply     # rewrite that file

    bin = &grp[1];
    p2_freq_add(&grp[1], is_dec, 4);

The name is assigned and the argument is the expression again.  That is what a
register allocator emits -- the value is in a register and in a slot, and the
instruction takes whichever is cheaper -- and it leaves a reader working out
that the two are the same thing.  `alt_p2_model` does it thirty times.

**The rule.**  `x = EXPR;` on one line, `EXPR` again on the next as a whole
token, `x` not mentioned on that next line, and the two lines at the same brace
depth.  Replace that one occurrence with `x`.  Nothing else moves: the
assignment stays, the statement stays, and the value is identical because the
assignment is the statement immediately before.

**The first version had neither of the last two conditions and both were
load-bearing.**  Applied to the tree it produced a build that spun forever
compressing `t24.bmp`, and the diff had two shapes in it that say why:

    off = 2*pairs++*frame.slot6;        was  off = 2*pairs*frame.slot6;
                                        and `slot_a = 2*pairs;` above it

`2*pairs` is a *prefix* of `2*pairs++`, and a plain `str.find` replaced it, so
the increment moved from `pairs` to `slot_a`.  A replacement has to be a whole
token on both sides.

    } while( (uint32_t)(y)<(uint32_t)blk1->height );

with `y = f.y_spill+1;` on the line before -- but that line is the last
statement of an inner block, not of the loop body, so it does not run on every
turn and the condition was reading a value from some earlier iteration.  That
is the infinite loop.  "The line before" is only "the statement before" when
the two are at the same brace depth.

**What it declines:**

  * **an expression with a side effect** -- anything containing `++`, `--`, an
    assignment or a call.  Writing it once instead of twice would then change
    what runs, which is a different transformation with a different argument.
  * **a bare name.**  `nplanes = plane_count;` next to `while( plane_count>pl2 )`
    matches the shape and substituting it is a *regression*: `plane_count` is
    the global that is the source of truth and `nplanes` is the spill of it, so
    the rewrite would put the copy where the original is meant.  The class this
    is for is a compound expression -- a subscript, an operator, a field chain
    -- written out twice.
  * **an expression under six characters.**  `x = n; f(n)` is not a readability
    problem and rewriting it to `f(x)` may well be worse.
  * **an occurrence that is being assigned to** on the next line.  `x = a->b;
    a->b = 3;` is a store to the place, not a use of the value.

**What it is not.**  It is not dead-store removal.  Twenty-eight of
`alt_p2_model`'s thirty `bin = …` assignments are dead -- the name is read at
exactly two of them -- and deleting the rest needs a dominance argument this
does not have and the gate cannot supply.  `unwrite.py` does not report them
either, and its zero is honest: `bin` *is* read, so it is not a write-only
local.  A partially-dead local is a class nothing in this directory covers, and
saying so is better than half-doing it.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

ASSIGN = re.compile(r'^(\s*)([A-Za-z_]\w*)\s*=\s*(&?[\w\[\]\+\-\.\>\*]+)\s*;\s*$')
SIDE = re.compile(r'\+\+|--|\w\s*\(|=')
# A bare name is not the class; see the docstring.
BARE = re.compile(r'^[A-Za-z_]\w*$')


def where(body, expr, name):
    """The one place on `body` to replace, or None.

    A whole token on both sides: `2*pairs` sits inside `2*pairs++` and a plain
    `find` took it, which moved an increment onto another variable.  And not a
    place being assigned to -- that is a store, not a use of the value.
    """
    if re.search(r'(?<![\w.>])%s\b' % re.escape(name), body):
        return None
    hits = [m.start() for m in
            re.finditer(r'(?<![\w\]\)])%s(?![\w\[(]|\+\+|--)'
                        % re.escape(expr), body)]
    if len(hits) != 1:
        return None
    at = hits[0]
    if re.match(r'\s*=(?!=)', body[at + len(expr):]):
        return None
    return at


def same_depth(a, b):
    """Whether the second line is the next *statement*, not just the next line."""
    return a.count('{') - a.count('}') == 0 and not b.lstrip().startswith('}')


def survey(path):
    lines, origin = structs.splice(path)
    out = []
    for i in range(len(lines) - 1):
        m = ASSIGN.match(lines[i].split('//')[0])
        if not m:
            continue
        name, expr = m.group(2), m.group(3)
        if len(expr) < 6 or SIDE.search(expr) or BARE.match(expr):
            continue
        nxt = lines[i + 1].split('//')[0]
        if not same_depth(lines[i].split('//')[0], nxt):
            continue
        if where(nxt, expr, name) is None:
            continue
        out.append((origin[i][0], origin[i + 1][1], name, expr,
                    nxt.strip()))
    return out


def apply_to(path):
    src = open(path).read().split('\n')
    n = 0
    for i in range(len(src) - 1):
        m = ASSIGN.match(src[i].split('//')[0])
        if not m:
            continue
        name, expr = m.group(2), m.group(3)
        if len(expr) < 6 or SIDE.search(expr) or BARE.match(expr):
            continue
        nxt = src[i + 1]
        body = nxt.split('//')[0]
        if not same_depth(src[i].split('//')[0], body):
            continue
        at = where(body, expr, name)
        if at is None:
            continue
        src[i + 1] = nxt[:at] + name + nxt[at + len(expr):]
        n += 1
    open(path, 'w').write('\n'.join(src))
    return n


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/passname.py bmf.cpp [--list]\n'
                 '       python3 tools/passname.py one.inc --apply')
    path = sys.argv[1]
    if '--apply' in sys.argv:
        print('%d expressions replaced by the name they were just given'
              % apply_to(path))
        return
    found = survey(path)
    if '--list' in sys.argv:
        for f, ln, name, expr, nxt in found:
            print('  %-24s %-14s = %-18s | %s'
                  % ('%s:%d' % (f, ln), name, expr, nxt[:44]))
    by = collections.Counter(f[0] for f in found)
    print('%d expressions written out again on the line after they were given '
          'a name%s' % (len(found),
                        '; ' + ', '.join('%s %d' % kv for kv in by.most_common(4))
                        if found else ''))


if __name__ == '__main__':
    main()
