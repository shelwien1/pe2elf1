#!/usr/bin/env python3
"""Turn one function's frame struct back into ordinary locals.

    python3 tools/liftframe.py subs1.hpp --list
    python3 tools/liftframe.py subs1.hpp model_planes

Bodies that hold their locals in a `struct alignas(16) …{ … } frame;` --
seventeen when this was written, nine now -- each carry a comment saying the
frame is a layout rather than a bag of locals, with the exact failure that
proved it: "altp1 segfaults while compressing", "five streams move, no signal".
The count is printed by `--list` rather than quoted here, because a number in a
docstring is a measurement that stops being re-taken.

Every one of those comments cites `tools/frame-sweep.sh`, which drives
`defram.py`, which lifts *aliases* -- `int32_t &v83 = *(int32_t *)(frame +
24600);`.  There are none left: earlier rounds turned every alias into a named
member, so `defram.py --list` answers "0 of 0 aliases lift" and the sweep
prints "0 kept, 0 reverted".  That is a green line meaning *nothing was tried*,
under seventeen claims that rest on it having been.

So this is the same experiment against the shape the file has now: each
non-padding member becomes a plain declaration, `frame.X` becomes `X`, and
the `static_assert`s that pin the layout go with it.  It proposes; the gate
decides, one frame at a time, and a frame that fails goes back exactly as it
was.

Four things it declines rather than guesses at:

  * **a frame with a `union`.**  The union is MSVC's slot sharing written down,
    and lifting its arms to separate locals is not the same program.  A frame
    whose *every* member is inside one is declined outright and said so -- for
    a long time it was declined silently, and six frames were neither offered
    nor mentioned under a line reading "0 frames this can offer to lift".  Two
    of those six were not slot sharing at all: `compress_image`'s union held a
    header and a scratch in mutually exclusive branches, and
    `alt_p2_context`'s two arms were the same six pointers viewed twice.  Both
    dissolved when the gate was asked;
  * **a member whose name is already a local** of that body -- the two would
    become one variable;
  * **any use of `frame` that is not `frame.X`**, which after the asserts
    are removed should be none.  If one is left, the frame is doing something
    this does not model and it stays;
  * **a member the body indexes out of.**  `&tbl16[16*xform]` on an
    `int32_t[16]` says the member after it is the rest of the object, and
    lifting the two apart is not the same program.  This one was added after
    the fact: the lift it would have declined shipped, and the defect took a
    bug report and a bisect to find three rounds later.  `indexed_out` below
    has the account.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# `alignas(...)` is part of the type, not something to step over.  Matching it
# in a non-capturing group and rebuilding the declaration from the rest is a
# silent drop: lifting `ChoosePlaneCodingFrame` turned six
# `alignas(16) int32_t x[4]` members into plain ones and nothing said so.  The
# gate stayed green because x86 loads an unaligned double happily -- the same
# "the compiler has no complaint to make" hazard as retyping a pointer -- and
# what was lost is six sixteen-byte spill slots that no longer say they are.
MEMBER = re.compile(r'\s*((?:alignas\([^)]*\)\s*)?'
                    r'(?:const\s+|unsigned\s+|signed\s+)*[A-Za-z_]\w*'
                    r'(?:\s*\*)*)\s+(\**\s*\w+)\s*((?:\[[^\]]*\])*)\s*;\s*$')


def frame_of(lines, a, b):
    """(first, last, [lift], [keep]) for the body's frame, or None.

    The struct has to be *inside* the body.  `ReduceAlphabetFrame` is declared
    at file scope -- an earlier round moved it out so its two arms could split
    into separate functions -- so this finds nothing there, and asking for that
    frame answered "no frame this can lift".  Which is true of this tool and
    not of the tree, and it is the silence the docstring above is against, one
    more code path over.  Said plainly rather than fixed: lifting a file-scope
    frame is a different operation, since the declaration is shared.
    """
    start = None
    for i in range(a, b + 1):
        c = lines[i].split('//')[0]
        if re.match(r'\s*struct\s+alignas\([^)]*\)\s*\w*\s*\{', c):
            start = i
            break
    if start is None:
        # A body that names a file-scope frame type has one; it is just not
        # here to lift.
        for i in range(a, b + 1):
            if re.search(r'\b\w+Frame\s+frame\s*;', lines[i].split('//')[0]):
                return 'file-scope'
        return None
    depth, end = 0, None
    for i in range(start, b + 1):
        c = lines[i].split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth <= 0 and 'frame' in c:
            end = i
            break
    if end is None:
        return None
    body = lines[start + 1:end]
    # A `union` is MSVC's slot sharing written down, and lifting its arms to
    # separate locals is not the same program.  But the members *outside* it
    # are ordinary locals that happen to sit in the same struct, and nine
    # frames were being declined whole for the union in the middle of them.
    # So the union is kept -- as a smaller frame in its own right -- and only
    # what surrounds it is offered.  Whether the two can be separated at all
    # is the gate's question, not this one's: the code in these bodies writes
    # past the ends of things, which is why the frames exist.
    out, depth, keep = [], 0, []
    for l in body:
        c = l.split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth > 0 or re.search(r'\b(?:union|struct)\b', c) or c.strip() in ('};', '}'):
            keep.append(l)
            continue
        m = MEMBER.fullmatch(c)
        if not m:
            if c.strip():
                return start, end, None
            continue
        name = m.group(2).lstrip('* ')
        if name.startswith('_pad') or name.startswith('_gap'):
            keep.append(l)
            continue
        out.append((m.group(1).strip(), m.group(2).strip() + m.group(3), name, c))
    return start, end, out, keep


def indexed_out(lines, a, b, got):
    """(member, the index) where the body indexes past a member's own bound.

    The fifth frame rule, and the one that cost a bug report: a member the body
    walks off the end of is a member whose *neighbour* is the rest of the
    object.  `choose_plane_coding` reads `&tbl16[16*xform]` out of an
    `int32_t[16]`, because `tbl16`, `tbl64a` and `tbl64b` are one 3x16 table in
    the frame -- lift them into three locals and rows 1 and 2 become whatever
    the compiler put there.  ASan saw it (a zero loop 136 bytes past the end)
    and the repair bound three of the six names onto one buffer *at the offsets
    a probe measured*, which put the writes in bounds and left the reads wrong
    for another three rounds.  tools/README.md has the whole account.

    Two shapes, both conservative: a constant index at or past the bound, and
    any index carrying a `*`, which is how a row of a flattened table is
    reached.  A false positive costs a frame that is not offered and says why;
    a false negative costs what this one did.

    **The two shapes do not establish the same thing, and the message used to
    claim they did.**  A constant at or past the bound is a demonstration.  A
    computed index is a refusal to guess -- this cannot bound `4*dx+2048`, and
    saying so is the whole of the answer.  Both were reported as "what it
    reaches is the member after it", and `cost_candidate` was declined and
    written up under that sentence for two rounds; `dx` is a difference of two
    byte sums, so `4*dx+2048` is 8..4088 and the member is 4096 bytes.  The
    decline was right and its stated reason was not, which is the worse of the
    two ways to be wrong here: a reason nobody can check is a reason nobody
    re-takes.  Each shape says which it is now.
    """
    start, end, members = got[0], got[1], got[2]
    if not members:
        return None
    # Not the frame's own declarations: `uint8_t buf_3[2048];` is a bound, not
    # an index, and reading it as one declines every array member there is.
    body = '\n'.join(l.split('//')[0]
                     for i, l in enumerate(lines[a:b + 1], a)
                     if not start <= i <= end)
    for _ty, decl, name, _src in members:
        m = re.search(r'\[\s*(\d+)\s*\]\s*$', decl)
        if not m:
            continue
        bound = int(m.group(1))
        # `frame.tbl16[16*xform]` before the lift, `tbl16[16*xform]` after
        # -- the question is asked of a frame that still exists, so the prefix
        # is the spelling that matters.  Written without it, this rule found
        # nothing anywhere and reported a clean tree.
        for use in re.finditer(r'(?<![\w.>])(?:frame\.)?%s\s*\[([^\]\n]+)\]'
                               % re.escape(name), body):
            idx = use.group(1).strip()
            if '*' in idx:
                return (name, '%s[%s]' % (name, idx),
                        'with an index this cannot bound, so whether it '
                        'reaches the member after it is not established')
            if re.fullmatch(r'\d+', idx) and int(idx) >= bound:
                return (name, '%s[%s]' % (name, idx),
                        'past its bound of %d, so what it reaches is the '
                        'member after it' % bound)
    return None


# Frames whose members have been given their own storage and which failed the
# gate for it.  Named with the failure rather than left on the offer list, so
# that "0 to lift" means what it says.
#
# Re-taken against the file as it is now -- lift each with `--retry`, run the
# gate, revert -- because a table of past failures is the kind of claim
# REFACTORING9.md section 32 is about.  Five were re-taken at round nine and
# all five still failed in the same way; `read_bmp` cleared at round ten, and
# the note below says what cleared it.
#
# `tools/asan.sh` says what the failure *is*, which these lines never did:
# lifting `search_filter`'s frame gives
#
#     ERROR: AddressSanitizer: stack-buffer-overflow
#     Address … is located in stack of thread T0 at offset 180 in frame
#
# These bodies walk off the ends of their locals on purpose, and the frame is
# what makes the neighbours theirs to walk.
# `read_bmp` was here, with "DLRAW aborts while compressing (rc 134)", for five
# rounds.  It is gone because the reason is gone, not because the entry aged
# out: the two `fread`s wrote across the slots Hex-Rays had split the BMP file
# and info headers into, so lifting those slots pulled the reads apart.  Reading
# into `BmpHeader` instead removed the reason and the lift was then ordinary --
# the frame dissolved to padding and the padding went with it.
#
# Which is the thing to take from this table: an entry here is a measurement of
# the *body as it stands*, and the way to clear one is to change what made it
# true.  `--retry` does not re-take them; it suppresses the table so the tool
# will offer a frame anyway, and the gate is what says whether the offer was
# good.
# Re-taken again, against a tree three thousand lines further on, and two of the
# four entries were describing an experiment this tool can no longer run:
#
#   * `cost_candidate` is declined structurally now, before any build --
#     `buf[4*dx+2048]` is an index this cannot bound;
#   * `reduce_alphabet`'s frame is at file scope, moved there by an earlier
#     round so its two arms could split, and `frame_of` only looks inside the
#     body.  Lifting a shared declaration is a different operation.
#
# Both are in `candidates`' decline list rather than here, which is where a
# reason that holds without running anything belongs.
#
# **`cost_candidate` is gone from both lists, and its decline was overstated
# while it lasted.**  The reason above was written as "indexes out of one
# member into the one after it", which is what this printed for a computed
# index and is not what a computed index shows.  It shows nothing about the
# index; that is the point of declining.  Taken by hand, `dx` is a difference
# of two byte sums, so `4*dx+2048` is 8..4088 in a 4096-byte member and the
# frame was seven histograms and padding.  `indexed_out`'s two shapes say which
# one they are now, and the function has no frame to ask about.
#
# `search_filter` cleared.  The entry below recorded a *whole*-frame lift, and
# what this offers now is the ten members outside the union -- the union is
# MSVC's slot sharing and stays.  110 checks byte-identical, 23 of 23 at the
# other pointer width, and ASan clean over 44 runs, which is the check that
# answers here: the recorded failure was a stack-buffer-overflow and `test.sh`
# cannot see one.
PROVEN = {
    # Re-taken with the rest: still fails, and wider than the entry said.  The
    # thirteen members outside the union lift, and seventeen of the nineteen
    # images then fail to expand at all -- `expand exits 3`, the bad-file code,
    # on DLRAW, altp1, f05_200, med32, noise24, rle4 and eleven more.  Not one
    # image, and not a stream that moved: the header parse stops working, which
    # is what says the frame's layout is what the parse is reading through.
    'expand_image':   'expand exits 3 on 17 of 19 images',
}


def candidates(lines):
    """(offers, declines) -- and the declines are named.

    A frame whose every non-padding member sits inside a `union` yields an
    empty member list, and for a long time that frame simply did not appear:
    the run ended on "0 frames this can offer to lift" with six frames neither
    offered nor mentioned.  That is the silence this file was written against
    -- `frame-sweep.sh` printed "0 kept, 0 reverted" under seventeen claims
    that rested on it having tried.  So a frame it cannot offer is now printed
    with the reason it cannot.
    """
    out, declined = [], []
    # `--retry` puts the tried-and-reverted frames back on the offer list.
    # `PROVEN`'s entries are measurements of a file that has moved a long way
    # since they were taken -- which is REFACTORING9.md section 32's whole
    # subject -- and re-taking one should be a command, not an edit to this
    # table.
    proven = set() if '--retry' in sys.argv else set(PROVEN)
    for a, b, nm, sig in structs.bodies(lines):
        got = frame_of(lines, a, b)
        if not got or nm.lstrip('_') in proven:
            continue
        if got == 'file-scope':
            declined.append((nm, 'its frame is declared at file scope, which '
                             'is a different lift: the declaration is shared'))
            continue
        crossed = indexed_out(lines, a, b, got) if got[2] else None
        if got[2] is None:
            declined.append((nm, 'a line in it is not a member declaration'))
        elif not got[2]:
            declined.append((nm, 'every member is inside its union'))
        elif crossed:
            declined.append((nm, 'the body indexes %s -- `%s` -- %s'
                             % (crossed[0], crossed[1], crossed[2])))
        else:
            types = structs.decl_types(sig, lines, a, b)
            clash = [n for _, _, n, _src in got[2]
                     if n in types and n not in {m[2] for m in got[2]}]
            out.append((nm, a, b, got, clash))
    return out, declined


def apply(lines, nm, a, b, got, indent='  '):
    start, end, members, keep = got
    body = '\n'.join(lines[a:b + 1])
    # `frame` must only ever be `frame.X` once the asserts are gone.
    rest = re.sub(r'\}\s*frame\s*;', '', re.sub(
        r'frame\.\w+', '', re.sub(
            r'static_assert\([^;]*?frame[^;]*?\);', '', body, flags=re.S)))
    if 'frame' in rest:
        return False, '%s: `frame` is used other than as a member' % nm.lstrip('_')

    text = re.sub(r'[ \t]*static_assert\([^;]*?frame[^;]*?\);\n', '',
                  body, flags=re.S)
    rows = text.split('\n')
    s2 = next(i for i, l in enumerate(rows)
              if re.match(r'\s*struct\s+alignas\([^)]*\)\s*\w*\s*\{',
                          l.split('//')[0]))
    depth, e2 = 0, None
    for i in range(s2, len(rows)):
        c = rows[i].split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth <= 0 and 'frame' in c:
            e2 = i
            break
    decls = ['%s%s %s;' % (indent, t, d) for t, d, _, _src in members]
    # A declaration rebuilt from a regex's groups is only the same declaration
    # if nothing fell between them.  `alignas(16) int32_t x0[4];` came out as
    # `int32_t x0[4];` for six members of `ChoosePlaneCodingFrame` because the
    # pattern stepped over the specifier instead of capturing it, and nothing
    # noticed: the file compiled, the streams did not move, and x86 loads an
    # unaligned double happily.  So the rebuild is compared against the source
    # line, ignoring whitespace, and a member that does not round-trip stops
    # the lift rather than being quietly simplified.
    for (t, d, _n, src), got_line in zip(members, decls):
        if re.sub(r'\s+', '', src) != re.sub(r'\s+', '', got_line):
            return False, ('%s: `%s` would be rewritten as `%s`'
                           % (nm.lstrip('_'), src.strip(), got_line.strip()))
    # What is left of the struct: the union and the padding that positions it.
    # If that is nothing but braces, the frame goes entirely.
    real = [l for l in keep
            if l.split('//')[0].strip() not in ('', '{', '}', '};')]
    if real:
        new = [rows[s2]] + keep + [rows[e2]] + decls
    else:
        new = decls
    rows[s2:e2 + 1] = new
    text = '\n'.join(rows)
    # Only the lifted names lose the prefix; the ones still in the struct keep
    # it, which is what makes a partial lift a rewrite rather than a guess.
    for _, _, n, _src in members:
        text = re.sub(r'frame\.%s\b' % re.escape(n), n, text)
    lines[a:b + 1] = text.split('\n')
    return True, '%s: %d members lifted, %d left in the frame' % (
        nm.lstrip('_'), len(members), len(real))


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'bmf.cpp'
    # The unit is 37 files and every frame is in one of the `.inc`; handed
    # `bmf.cpp` this used to read an include list, find no bodies and print
    # "0 frames this can offer to lift" -- a zero from a rule that had nothing
    # to read, which is the failure this whole directory is written against.
    # Listing reads the unit as the compiler does; lifting rewrites a file, so
    # it says which one to point at rather than writing to the wrong place.
    lines, origin = structs.splice(path)
    want = next((x for x in sys.argv[2:] if not x.startswith('--')), None)
    found, declined = candidates(lines)
    if want:
        one = [f for f in found if f[0].lstrip('_') == want.lstrip('_')]
        if not one:
            # The reason, not just the refusal.  `--list` has printed declines
            # with their reason since the six silent ones; asking for a single
            # frame did not, and answered "no frame this can lift" to a frame
            # this declines for a stated reason.  Same silence, one code path
            # over.
            why = [d for d in declined if d[0].lstrip('_') == want.lstrip('_')]
            sys.exit('%s: %s' % (want, why[0][1] if why else
                                 'no frame this can lift'))
        nm, a, b, got, clash = one[0]
        if clash:
            sys.exit('%s: %s already a local' % (nm, ', '.join(clash)))
        home = origin[a][0]
        if home != path.rsplit('/', 1)[-1]:
            sys.exit('%s is in %s; run this against that file'
                     % (nm.lstrip('_'), home))
        ok, why = apply(lines, nm, a, b, got)
        if not ok:
            sys.exit(why)
        open(path, 'w').write('\n'.join(lines))
        print(why)
    else:
        for nm, a, b, got, clash in found:
            print('  %-24s %3d members  %s%s'
                  % (nm.lstrip('_'), len(got[2]), origin[a][0],
                     '  CLASH: ' + ','.join(clash) if clash else ''))
        if '--retry' not in sys.argv:
            for fn, why in sorted(PROVEN.items()):
                print('  %-24s tried: %s' % (fn, why))
        for nm, why in sorted(declined):
            print('  %-24s declined: %s' % (nm.lstrip('_'), why))
        print('%d frames this can offer to lift; %d tried and reverted, '
              '%d declined' % (len(found),
                               0 if '--retry' in sys.argv else len(PROVEN),
                               len(declined)))
