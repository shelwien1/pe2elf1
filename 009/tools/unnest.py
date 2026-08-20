#!/usr/bin/env python3
"""An `if` that is the last statement of its block, holding everything in it.

    python3 tools/unnest.py bmf.cpp             # report
    python3 tools/unnest.py bmf.cpp --list      # and every one

    for( ... ) {                        for( ... ) {
      ...                                 ...
      if( countdown ) {                   if( !countdown )
        ... 150 lines ...        ->          continue;
      }                                   ... 150 lines, one level out ...
    }                                   }

A guard with nothing after it is a guard, and the shape a guard should have is
the early exit.  Written as an `if` it costs a level of indentation for every
line it holds, and the reader has to carry "we are inside the countdown case"
down a hundred and fifty lines to find out that there is no other case.

**This was found by hand, which is the reason the file exists.**
`AltP2Block::code_banks` had two of them nested -- `if( countdown )` around the
whole loop body and `if( !alphabet_reduced )` around all but the rate ladder --
putting its working lines five levels in, and nothing reported it.  A decompiler
produces this shape by default: a conditional jump over a region becomes an `if`
around the region, and there is no pass anywhere that asks whether the region
ends where the enclosing block does.

**The rule.**  An `if` with no `else`, whose block is the last statement of the
body it sits in, where that body is a loop (so the inversion is `continue`) or a
function (so it is `return`), and whose block holds at least `MIN` lines.  The
count is of the block's lines, comments included, because the cost this reports
is indentation and a comment is indented too.

`MIN` is 12 rather than 1 because the small ones are a judgement call and this
should not be making it.  `if( fp ) { four lines }` at the end of a function
reads perfectly well as an `if`, and a tool that reported it would be argued
with instead of fixed -- which is how a check gets switched off.

**It reports one nesting at a time, on purpose.**  `code_banks` had two, and on
the file as it stood only the outer one was reported: the inner `if` was the last
statement of the *outer* `if`'s block, and an `if` is not something `continue`
leaves.  Invert the outer one and the inner becomes the last statement of the
loop, which the next run reports.  Checked both ways round on that file --
198 lines, then 169 -- rather than assumed.  Making it recurse would report a
finding whose fix depends on another finding being taken first, and a list where
the second entry is contingent on the first is a list that gets half applied.

**What it does not report.**  An `if` with an `else` (there is a second case, so
there is nothing to invert), an `if` inside a `switch` (the enclosing block's
close is a `break` away and the shape is different), and an `if` whose condition
spans more than one line -- inverting those needs the condition rewritten rather
than negated, and this reports rather than applies.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

MIN = 12

IF = re.compile(r'^(\s*)if\(\s*(.+?)\s*\)\s*\{\s*$')
# The line that opens an enclosing block, and what an early exit out of it is.
LOOP = re.compile(r'^\s*(for|while)\s*\(')
DO = re.compile(r'^\s*do\s*\{')
FUNC = re.compile(r'^\S.*\)\s*(const\s*)?\{\s*$')


def units(path):
    """(lines, [(name, first, last)]) for each definition in `path`.

    Through `structs.bodies`, which sees a signature that wraps onto a second
    line and an in-class method; the walk this replaced saw neither.  Naming a
    finding was the smaller half of what that cost -- see `encloser`.
    """
    lines = open(path).read().split('\n')
    return lines, [(nm, a, b) for a, b, nm, _sig in structs.bodies(lines)]


def close_of(lines, i):
    """Index of the line closing the block the `if` at `i` opens.

    By indentation and not by counting braces, because `} else {` is net zero
    and a counter walks straight past it.  That is not hypothetical: the first
    run of this file reported `fill_row_inputs`' `if( refa )` at 85 lines and
    `code_banks`' `if( lowbits <= 0 )` at 20, and both have an `else` -- the
    counter had matched the *else* block's close and the `else` test three lines
    down never saw an `else` because it was looking at the wrong line.  Two of
    eight findings were that, which is the argument for reading a new tool's
    findings one by one before believing its count.

    Anything inside the block is indented further, so the first line at the
    `if`'s own indentation whose first character is `}` is its close, whatever
    follows on that line.
    """
    indent = len(lines[i]) - len(lines[i].lstrip())
    for j in range(i + 1, len(lines)):
        l = lines[j]
        if not l.strip():
            continue
        if len(l) - len(l.lstrip()) == indent and l.lstrip().startswith('}'):
            return j
    return -1


def encloser(lines, i, indent, opens):
    """What an early exit out of the block holding line `i` would be.

    Scans back for the line that opens that block: it is the nearest line above
    with less indentation that ends in `{`.  A `switch` is excluded by never
    matching it -- neither pattern below does.

    **A body is a body whether or not its signature fits on a line.**  `FUNC`
    wanted the whole of one, so when the opener found here was the *second* line
    of a wrapped signature it matched neither `LOOP` nor `FUNC`, this answered
    `None`, and the finding was dropped without a word.  `opens` holds the line
    each body starts at, per `structs.bodies`, and a wrapped signature starts at
    the first of its lines -- so the test looks back a few from where the scan
    stopped.
    """
    for j in range(i - 1, -1, -1):
        l = lines[j]
        if not l.strip() or l.lstrip().startswith('//'):
            continue
        if len(l) - len(l.lstrip()) >= len(indent):
            continue
        if not l.rstrip().endswith('{'):
            return None
        if LOOP.match(l) or DO.match(l):
            return 'continue'
        for k in range(j, max(-1, j - 4), -1):
            if k in opens:
                return 'return'
        return None
    return None


def findings(path):
    lines, defs = units(path)
    opens = {a for _nm, a, _b in defs}
    out = []
    for i, l in enumerate(lines):
        m = IF.match(l)
        if not m:
            continue
        indent, cond = m.group(1), m.group(2)
        end = close_of(lines, i)
        if end < 0 or lines[end].strip() != '}':
            continue
        # An `else` means a second case, and nothing to invert.
        nxt = lines[end + 1] if end + 1 < len(lines) else ''
        if nxt.strip().startswith('else') or lines[end].strip().startswith('} else'):
            continue
        # Last statement of its block: the next line that is not blank closes
        # the block this `if` sits in.
        j = end + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j >= len(lines):
            continue
        if lines[j].strip() != '}' or len(lines[j]) - len(lines[j].lstrip()) >= len(indent):
            continue
        held = end - i - 1
        if held < MIN:
            continue
        exit_ = encloser(lines, i, indent, opens)
        if not exit_:
            continue
        name = ''
        for n, a, b in defs:
            if a <= i <= b:
                name = n
        out.append((held, path, i + 1, name, cond, exit_))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    show = '--list' in sys.argv
    if not args:
        raise SystemExit(__doc__.split('\n')[2].strip())
    out = []
    for path in args:
        out += findings(path)
    out.sort(reverse=True)
    if show:
        for held, path, line, name, cond, exit_ in out:
            print('%5d lines  %-26s %s:%d  if( %s ) -> %s'
                  % (held, name, path, line, cond, exit_))
    print('%d guards holding %d lines that an early exit would unindent'
          % (len(out), sum(h for h, *_ in out)))


main()
