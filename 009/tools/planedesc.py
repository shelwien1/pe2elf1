#!/usr/bin/env python3
"""Take `plane_desc`'s header slot out, and re-index the four planes onto zero.

    python3 tools/planedesc.py               # every subscript, and what of it
    python3 tools/planedesc.py --apply

`plane_desc[5]` holds four planes.  Slot 0 is not one of them: it is a header
slot whose three weight words the original used for unrelated things --
`plane_count` and `near_lossless_q` were references onto two of them, and the
third carried the archive version word.  So every real access is written
`plane_desc[pl+1]`, and `+1` appears 79 times in this file for no reason a
reader can see.

The three squatters go first, by hand; this does the re-index, which is 100
subscripts across ten files and exactly the kind of edit a person gets 97 of
right.

**Four shapes, and the fourth is why this is a script and not a `sed`.**

    plane_desc[pl+1]        ->  plane_desc[pl]         drop the +1
    plane_desc[row+2]       ->  plane_desc[row+1]      any +N, minus one
    plane_desc[3]           ->  plane_desc[2]          a literal, minus one
    plane_desc[pl++ +1]     ->  plane_desc[pl++]       drop the +1, keep the ++
    plane_desc[k]           ->  plane_desc[k-1]        already one-based

The fourth is the trap.  `plane_desc[next_plane]` and `plane_desc[pl2]` are
*not* missing a `+1`; they are subscripts whose variable already counts from
one, and rewriting them like the first shape would silently shift them the
wrong way.  There is no syntax that tells the two apart, so this tool does not
guess: anything that is not `E+1` or a literal is listed for a human, and the
answer for each is recorded in `ONE_BASED` below.  Getting that list wrong is
an off-by-one in a descriptor table, which shows up as a stream that differs
somewhere in the middle and not as a crash.

**Nested subscripts.**  `plane_desc[plane_desc[3].src_plane+1]` has two of
them and both need the treatment -- the inner is a literal slot and the outer
an `E+1`.  The first version of this collected every subscript, sorted them
innermost-first and edited them in that order, which is wrong twice: fixing
the inner one moves the outer one's closing bracket, and the outer one's
replacement text is `plane_desc[3].src_plane` -- the *original* inner, which
the outer edit then puts back.  Two bugs that cancel into a file where nested
sites are silently unshifted.

So the rewrite is recursive instead: for each subscript, fix its contents
first, then decide what the subscript itself becomes from the fixed text.  A
nested one is never visited twice, because scanning resumes past the closing
bracket of the one just handled.

**It only runs once, and it says so.**  This is not an idempotent rewrite: with
the shift already applied, `plane_desc[1]` is a plane and running again would
make it `plane_desc[0]`, silently, on every subscript in the file.  `sweep.sh`
runs every counting tool and expects zero, so a one-shot tool that keeps
finding work is indistinguishable from a tree that still has work in it.  The
guard is the declaration: five descriptors means the header slot is still
there and there is a shift to do; four means it is done.  Same fact the tool is
about, read from the place that defines it.
"""
import re
import sys
import glob

# Subscripts whose variable already counts descriptors from one, so the fix is
# `-1` rather than dropping a `+1`.  Each was read at its site.
ONE_BASED = {
    # plane_choose.inc: `result = next_plane-1` is the plane, `next_plane` the slot.
    'next_plane',
    # filter_search.inc: assigned from `pl2+1` a few lines up.
    'pl2',
    # plane_predict.inc: the loop does `++k;` before the subscript, so the
    # first descriptor it reads is slot 1 -- and the plane it holds is `k-1`.
    'k',
}
# Subscripts that are already a plane number and need nothing.  Empty so far;
# it exists so that a site read and found correct can be recorded as read.
ALREADY = set()


def rewrite(inner, unknown, where):
    """The new subscript text, or None to leave it alone."""
    t = inner.strip()
    if re.fullmatch(r'\d+', t):
        return str(int(t) - 1) if int(t) else None
    m = re.fullmatch(r'(.+?)\s*\+\s*(\d+)', t)
    if m:
        n = int(m.group(2)) - 1
        return m.group(1) if not n else '%s+%d' % (m.group(1), n)
    if t in ONE_BASED:
        return t + '-1'
    if t in ALREADY:
        return None
    unknown.append('%s  plane_desc[%s]' % (where, t))
    return None


def fix(text, unknown, path, base):
    """Every `plane_desc[...]` in `text`, contents first.

    `base` is the offset of `text` in the file, so an unresolved subscript can
    be reported at the line it is on.
    """
    # Not the declaration: `static PlaneDesc plane_desc[4];` is a subscript by
    # syntax and an array size by meaning, and shifting it to 3 loses a
    # descriptor rather than renumbering one.
    pat = re.compile(r'(?<!PlaneDesc )\bplane_desc\[')
    out, i, n = [], 0, 0
    while True:
        m = pat.search(text, i)
        if not m:
            break
        j, depth = m.end() - 1, 0
        while j < len(text):
            if text[j] == '[':
                depth += 1
            elif text[j] == ']':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        inner = text[m.end():j]
        inner, k = fix(inner, unknown, path, base + m.end())
        n += k
        where = '%s:%d' % (path, ORIGINAL[path][:base + m.start()].count('\n') + 1)
        new = rewrite(inner, unknown, where)
        if new is not None and new != inner:
            n += 1
        else:
            new = inner
        out.append(text[i:m.end()] + new + ']')
        i = j + 1
    out.append(text[i:])
    return ''.join(out), n


ORIGINAL = {}


def already_done():
    """True once `plane_desc` holds planes only."""
    for path in glob.glob('*.inc'):
        m = re.search(r'\bPlaneDesc plane_desc\[(\d+)\]', open(path).read())
        if m:
            return int(m.group(1)) == 4
    return False


def main():
    if already_done():
        print('plane_desc holds four planes: the header slot is already out')
        print('0 subscripts to re-index')
        return 0
    unknown, changed, files, edits = [], 0, 0, {}
    for path in sorted(glob.glob('*.inc') + glob.glob('*.cpp')):
        s = open(path).read()
        if 'plane_desc[' not in s:
            continue
        ORIGINAL[path] = s
        out, n = fix(s, unknown, path, 0)
        if n:
            edits[path] = out
            changed += n
            files += 1
            print('  %-22s %d subscripts' % (path, n))
    for u in unknown:
        print('  UNKNOWN  ' + u + '   -- add it to ONE_BASED or ALREADY')
    print('%d subscripts in %d files, %d unresolved' % (changed, files, len(unknown)))
    if unknown:
        print('-- not applying: an unresolved subscript is an off-by-one waiting to happen')
        return 1
    if '--apply' in sys.argv:
        for path, s in edits.items():
            open(path, 'w').write(s)
        print('-- applied')
    return 0


if __name__ == '__main__':
    sys.exit(main())
