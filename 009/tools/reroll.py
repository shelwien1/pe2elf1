#!/usr/bin/env python3
"""A loop the decompiler wrote out one turn at a time.

    python3 tools/reroll.py bmf.cpp             # report
    python3 tools/reroll.py bmf.cpp --list      # and every run
    python3 tools/reroll.py one.inc --apply     # rewrite that file

    sym_cache[5] = sym_cache[4];
    sym_cache[4] = sym_cache[3];
    sym_cache[3] = sym_cache[2];
    sym_cache[2] = sym_cache[1];
    sym_cache[1] = sym_cache[0];

Five lines that are one loop.  MSVC unrolled it in 1997 and Hex-Rays had no
reason to put it back, so it arrives as five statements that a reader has to
diff against each other to see they are the same statement.

**The rule.**  Take a maximal run of three or more consecutive statements whose
text is identical once every integer is blanked.  Every blanked position must
be constant down the run or an arithmetic progression, and at least one must
move; each then spells itself as `base + step*i` and the run becomes a `for`.
The columns need not share a step -- `rec[-1] = rec[0]; rec[-2] = rec[1];`
walks its destination down while its source walks up, and that is still one
loop over one counter.

**Direction.**  The rewrite always walks the run the way it was written, so the
question is only whether to say so.  A run where two turns touch the same place
may not be reordered, and all three dependences count: the run above has no
line reading what an earlier line wrote -- it is the other way round, the
second line *writes* `sym_cache[4]`, which the first read -- and reversing it
would still give a different array.  Where the tool finds one it writes a
comment, so a later reader who wants to "tidy" the loop into ascending order
has been told.  No comment means nothing was found, which for a run of calls
the tool cannot read is weaker than "the turns are independent".

**What it cannot see, said here so its zero is not read as more than it is.**
The rule is textual identity once the *integers* are blanked, and a decompiler
that renames per turn defeats it.  `code_symbol_tree`'s rescale was eight turns
of `freq[k] -= freq[k]>>1` with a running total, and it came out as twenty-six
interleaved lines carrying seven accumulators -- `h2`, `acc`, `h4`, `sum4`,
`acc6`, `acc8`, `h9` -- with the stores in the order the registers came free.
No two of those statements are the same text under any blanking of integers,
and this file was right to say nothing about them.

Widening to blank identifiers as well is `dupblock.py`'s rule, and it reports
rather than applies for exactly this reason: once the names may differ, so may
the meanings.  That block was found by reading, after `unstar.py` turned its
`*(freq+K)` spellings into subscripts and the shape showed.  A scan for the
residue -- a window touching `name[K]` for five or more consecutive `K` -- came
back with eight, all of them prose or a loop that is already a loop.

**The counter needs a fresh name.**  `src2->cursor[0] = src2->buf[0]+row_i+8;`
unrolled five ways is a loop, and the same block in the decoder spells `row_i`
as `i`.  Re-rolling that one with a counter also called `i` shadows the row and
silently changes what the loop reads -- a bug the gate caught -- so the name is
picked from the block the loop will sit in, not assumed.

**What it declines,** and these are most of what a first pass turns up:

  * **a declaration.**  `int32_t a1, a2, a3;` and a run of `uint8_t x0 = 0;`
    have exactly this silhouette and are not loops -- there is no array to
    index and no variable to induct on.  Any run whose first token is a type is
    refused.
  * **a table.**  `node->c[0] = 8; node->c[1] = 2; node->c[2] = 2;` -- the
    subscripts are a progression but the values are unrelated numbers, so there
    is no formula and a `for` would have to carry the numbers in an array
    anyway.  A run is refused unless *every* moving position is a progression,
    which is what tells a table from a loop.
  * **a run in scheduler order.**  `row[2] = 205; row[6] = 48; row[3] = 124;`
    is a progression once sorted, and the stores are to distinct places so
    sorting is safe -- but sorting is a second transformation with its own
    argument and this tool does one thing.  Counted separately, left alone.
  * **fewer than three lines.**  Two is not a loop, it is two statements.

**What it is not.**  It is not a performance change and must not be argued as
one; whether GCC unrolls a small counted loop at -O2 is the compiler's
business.  The claim is that five lines saying "shift the array down" read
better as one line saying that.  If a re-rolled loop moves a stream then the
transformation was wrong about the order, and the gate says so.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

INT = re.compile(r'(?<![\w.])(-?\d+)(?![\w.])')
STMT = re.compile(r'^\s*[A-Za-z_*(].*;\s*$')
# A declaration run is the same silhouette and is not a loop; see the docstring.
TYPE = re.compile(r'^\s*(?:const\s+|static\s+|unsigned\s+|signed\s+)*'
                  r'(?:void|bool|char|short|int|long|float|double|auto|'
                  r'u?int(?:8|16|32|64)_t|size_t|ptrdiff_t|uintptr_t|FILE|'
                  r'[A-Z]\w*)\b[\s*&]+[A-Za-z_]')
MIN_RUN = 3


def blank(line):
    """The line with every integer replaced by a hole."""
    return INT.sub('#', line.strip())


def ints(line):
    return [int(m.group(1)) for m in INT.finditer(line.strip())]


def progression(col):
    """Whether a column of integers is constant or an arithmetic progression."""
    step = col[1] - col[0]
    return all(b - a == step for a, b in zip(col, col[1:]))


MEMCPY = re.compile(r'^\s*(?:mem(?:cpy|move|set)|bmf_copy)\s*\(\s*(.*)$')


def split_args(text):
    """The comma-separated arguments of a call, given the text after its `(`."""
    out, depth, start = [], 0, 0
    for i, c in enumerate(text):
        if c in '([':
            depth += 1
        elif c in ')]':
            if depth == 0:
                out.append(text[start:i].strip())
                return out
            depth -= 1
        elif c == ',' and depth == 0:
            out.append(text[start:i].strip())
            start = i + 1
    return out


def written_read(line):
    """(the place this statement writes, the text it reads) or None if unclear."""
    m = MEMCPY.match(line)
    if m:
        args = split_args(m.group(1))
        return (args[0], ' '.join(args[1:])) if args else None
    depth = 0
    for i, c in enumerate(line):
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == '=' and depth == 0:
            if line[i + 1:i + 2] == '=' or line[i - 1] in '=!<>+-*/&|^%':
                return None
            return line[:i].strip(), line[i + 1:]
    return None


def overlaps(run):
    """Whether two turns of the run touch the same place, so the order is fixed.

    All three dependences count, not just the obvious one.  `sym_cache[3] =
    sym_cache[2]; sym_cache[2] = sym_cache[1];` has no line reading what an
    earlier line wrote -- it is the other way round, the second line *writes*
    what the first read -- and reversing it would still give a different array.

    It is not a proof of the converse: a `no` from a run of calls the tool
    cannot read means "nothing found", not "the turns are independent" -- which
    is why the rewrite keeps the written order either way and this answer only
    decides whether to say so in a comment.
    """
    seen = [written_read(l) for l in run]
    for i, a in enumerate(seen):
        if not a:
            continue
        for b in seen[i + 1:]:
            if b and (a[0] in b[1] or b[0] in a[1] or a[0] == b[0]):
                return True
    return False


NAMES = ['i', 'j', 'k', 'n', 'q', 't', 'u', 'v']


def enclosing(src, at):
    """The outermost block that contains this line -- a superset of its scope."""
    start, net = at, 0
    for i in range(at - 1, -1, -1):
        for c in reversed(src[i]):
            if c == '}':
                net += 1
            elif c == '{':
                if net:
                    net -= 1
                else:
                    start = i
    end, net = at, 0
    for i in range(at + 1, len(src)):
        for c in src[i]:
            if c == '{':
                net += 1
            elif c == '}':
                if net:
                    net -= 1
                else:
                    end = i
    return src[start:end + 1]


def induction(scope):
    """A name for the loop counter that means nothing else where it will sit.

    `src2->cursor[0] = src2->buf[0]+row_i+8;` unrolled five ways is a loop, and
    the same block in the decoder spells `row_i` as `i`.  Re-rolling that one
    with a counter also called `i` shadows the row and silently changes what it
    reads, which the gate caught; the counter has to be a name the block does
    not already use.
    """
    text = '\n'.join(scope)
    for name in NAMES:
        if not re.search(r'(?<![\w.>])%s\b' % name, text):
            return name
    k = 0
    while re.search(r'\bi%d\b' % k, text):
        k += 1
    return 'i%d' % k


def classify(run):
    """The per-column steps of a run that is a loop, else None.

    Two moving columns need not share a step.  `rec[-1] = rec[0];
    rec[-2] = rec[1];` walks its destination down while its source walks up,
    and that is still one loop over one induction variable -- each column is
    its own `base + step*i`.
    """
    cols = list(zip(*[ints(l) for l in run]))
    if not cols or not all(progression(c) for c in cols):
        return None
    steps = [c[1] - c[0] for c in cols]
    return steps if any(steps) else None


def sortable(run):
    """Whether reordering the run's lines would make it a progression."""
    rows = [ints(l) for l in run]
    for k in range(len(rows[0])):
        col = [r[k] for r in rows]
        if len(set(col)) != len(col):
            continue
        order = sorted(range(len(rows)), key=lambda i: col[i])
        if classify([run[i] for i in order]) is not None:
            return True
    return False


def runs(lines):
    """Every maximal run of same-shape statements, longest first in file order."""
    out, i = [], 0
    while i < len(lines):
        body = lines[i].split('//')[0]
        if not STMT.match(body) or TYPE.match(body) or not ints(body):
            i += 1
            continue
        shape = blank(body)
        j = i + 1
        while j < len(lines):
            nxt = lines[j].split('//')[0]
            if not STMT.match(nxt) or blank(nxt) != shape:
                break
            j += 1
        if j - i >= MIN_RUN:
            out.append((i, [lines[k].split('//')[0] for k in range(i, j)]))
            i = j
        else:
            i += 1
    return out


def survey(path):
    lines, origin = structs.splice(path)
    loops, tables, unsorted = [], [], []
    for at, run in runs(lines):
        c = classify(run)
        where = (origin[at][0], origin[at][1])
        if c is not None:
            loops.append((where, run, c, overlaps(run)))
        elif sortable(run):
            unsorted.append((where, run))
        else:
            tables.append((where, run))
    return loops, tables, unsorted


def index(base, step, var):
    """How the i-th turn spells this column."""
    if step == 1:
        return var if base == 0 else '%d+%s' % (base, var)
    if step == -1:
        return '-' + var if base == 0 else '%d-%s' % (base, var)
    return ('%d*%s' % (step, var) if base == 0
            else '%d%+d*%s' % (base, step, var))


def rewrite(run, steps, ordered, indent, var):
    """The `for` that says what the run says, walked the way it was written."""
    cols = list(zip(*[ints(l) for l in run]))
    parts, k = [], 0
    for piece in INT.split(run[0].strip()):
        if k % 2 == 0:
            parts.append(piece)
        else:
            col, step = cols[k // 2], steps[k // 2]
            parts.append(index(col[0], step, var) if step else str(col[0]))
        k += 1
    out = []
    if ordered:
        out.append('%s// The direction is load-bearing: the turns touch the '
                   'same places.' % indent)
    out.append('%sfor( int32_t %s = 0; %s<%d; ++%s ) %s'
               % (indent, var, var, len(run), var, ''.join(parts)))
    return out


def apply_to(path):
    src = open(path).read().split('\n')
    hits = []
    for at, run in runs(src):
        c = classify(run)
        if c is not None:
            hits.append((at, run, c, overlaps(run),
                         induction(enclosing(src, at))))
    for at, run, steps, ordered, var in reversed(hits):
        indent = re.match(r'\s*', src[at]).group(0)
        src[at:at + len(run)] = rewrite(run, steps, ordered, indent, var)
    open(path, 'w').write('\n'.join(src))
    return len(hits)


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/reroll.py bmf.cpp [--list]\n'
                 '       python3 tools/reroll.py one.inc --apply')
    path = sys.argv[1]
    if '--apply' in sys.argv:
        print('%d unrolled runs written as the loop they are' % apply_to(path))
        return
    loops, tables, unsorted = survey(path)
    if '--list' in sys.argv:
        for (f, ln), run, steps, ordered in loops:
            print('  %-22s %d lines, step %s%s\n      %s'
                  % ('%s:%d' % (f, ln), len(run),
                     ','.join('%+d' % s for s in steps if s),
                     ', direction is load-bearing' if ordered else '',
                     run[0].strip()))
        for (f, ln), run in unsorted:
            print('  %-22s %d lines, scheduler order, left alone\n      %s'
                  % ('%s:%d' % (f, ln), len(run), run[0].strip()))
        for (f, ln), run in tables:
            print('  %-22s %d lines, a table, left alone\n      %s'
                  % ('%s:%d' % (f, ln), len(run), run[0].strip()))
    by = collections.Counter(f for (f, _), _, _, _ in loops)
    print('%d unrolled loops in %d files%s; %d runs are tables and %d are in '
          'scheduler order, both declined'
          % (len(loops), len(by),
             '; ' + ', '.join('%s %d' % kv for kv in by.most_common(4))
             if loops else '', len(tables), len(unsorted)))


if __name__ == '__main__':
    main()
