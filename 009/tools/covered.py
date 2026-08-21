#!/usr/bin/env python3
"""Names the program's functions that no algorithm document describes.

    python3 tools/covered.py                  # bmf.cpp against the documents
    python3 tools/covered.py --list           # and where each one is defined
    python3 tools/covered.py --why            # print the exemption rules instead
    python3 tools/covered.py other.cpp        # check another unit

`tools/unstale.py` asks the question one way round: every name a document
spells, does the program still have it?  That catches a document describing a
function that has been renamed away.  It cannot catch the other failure, which
is the one that actually happened here: a document that never mentioned a
function at all.

`ALGORITHM_v2.md` was written, gated, committed and pushed while missing
`bitctr.inc` and `sym_code.inc` entirely -- the adaptive counters, the seven-slot
node, the symbol tree, `estimate_cost`.  That is the layer that turns a symbol
into a coded event, which is to say it is most of what a document about
compression algorithms is for.  Nothing reported it, because nothing was asking.
It was found by listing the functions in each unit by hand and grepping the
document for each, which worked, and which is exactly the kind of by-hand
census this tree turns into a script the moment it finds something.

The same by-hand pass then found `plane_mix3` rounding a blend with 64 where
`PlaneTransform::blend` rounds the same blend with 63.  Neither function was in
the document.  That is the argument for this tool in one line: an undescribed
function is not just a gap in the prose, it is a place nobody has read closely.

**What counts as described is deliberately weak**: the name appears somewhere in
some document.  A tool cannot tell a good paragraph from a bad one, and a rule
that tried would be argued with instead of fixed.  What it can tell is silence,
and silence is what went wrong.

The exemptions are rules, not a list of names, so a new allocator wrapper or a
new `<0>`/`<1>` pair is exempt the day it is written and does not have to be
added here.  `--why` prints them.  Two of them are worth stating up front:

  * **the range coder is out of scope by the request** these documents answer,
    so `rc.inc` and the `rc_*` entry points in `rc_io.inc` are exempt.  This is
    the one exemption that is a scope decision rather than a fact about the
    code, and it is the one to revisit if the request ever changes.
  * **a `<0>`/`<1>` wrapper is described by its template.**  `encode_symbol_tree`
    and `decode_symbol_tree` are two lines each and both call
    `code_symbol_tree`; a document that explains the template has explained
    them, and requiring all three names would push documents toward listing
    identifiers rather than saying what happens.

Detected structurally rather than named, which matters: `alt_p1_encode_symbol`
and `alt_p1_decode_symbol` do not share a stem with `code_symbol`, so a rule
about spelling would have missed them and a list would have had to be kept.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import unstale                                                    # noqa: E402

# Not a function definition, whatever the line looks like.  `sizeof` and
# `alignas` take parentheses; the rest open blocks.
KEYWORDS = {
    'if', 'while', 'for', 'switch', 'catch', 'return', 'sizeof', 'alignas',
    'do', 'else', 'case', 'new', 'delete', 'throw', 'static_assert',
    '__builtin_memcpy', '__builtin_memset', 'memcpy', 'memset', 'assert',
}

# A definition line: a return type, then `name(` or `Class::name(`, and the
# signature is not a declaration.  Requiring the return type is what keeps a
# bare call `foo(x);` out.
#
# The separator between the two is not optional, and that is the whole of what
# this got wrong first: with `\s*\**\s*` between them the regex was free to
# split one identifier down the middle, so `switch( lvl ) {` matched as a
# function `itch` returning `sw`, and a constructor's initialiser
# `w2_to_w1(flags & ...)` as one named `_to_w1`.  Both were reported as
# undescribed functions of the program, which is a tool inventing findings --
# worse than missing them, because a reader who checks two and finds nothing
# there stops checking.  A type and a name must be separated by whitespace, a
# `*` or a `&`.
DEF = re.compile(
    r'^(?P<lead>\s*)'
    r'(?:template\s*<[^>]*>\s*)?'
    r'(?P<ret>[A-Za-z_][\w:<>,*&\[\] ]*?[\w>*&\]])[\s*&]+'
    r'(?P<name>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)\s*\('
)

# A body that is one `return name<0>(...)` or `name<1>(...)`, with nothing else
# in it.  The instantiation of a merged template, described by the template.
WRAPPER = re.compile(r'^\s*(?:return\s+)?[\w:.>-]*\b\w+<[01]>\s*\(')


def units(path):
    """(line, text) for the spliced unit, and which file each line came from.

    `structs.splice` returns the unit with `#include "x"` replaced by x's
    lines, which is how every other tool here reads `bmf.cpp` -- 195 lines of
    include list otherwise.  The file each line came from is wanted for the
    `--list` output and for the per-file exemptions below, so the includes are
    walked again here rather than taken from the splice.
    """
    lines = structs.splice(path)[0]
    where, cur = [], path.rsplit('/', 1)[-1]
    inc = re.compile(r'^#include\s+"([^"]+)"')
    src = open(path).read().split('\n')
    # Re-walk the original to learn the include order, then attribute the
    # spliced lines to files by replaying it.  A spliced file has no includes
    # left, so this degrades to "all of it came from `path`" -- which is the
    # right answer when `sweep.sh` hands over an already-spliced copy.
    spans, at = [], 0
    for l in src:
        m = inc.match(l)
        if m:
            try:
                n = len(open(m.group(1)).read().split('\n'))
            except IOError:
                continue
            spans.append((at, at + n, m.group(1)))
            at += n
        else:
            at += 1
    for i in range(len(lines)):
        cur = path.rsplit('/', 1)[-1]
        for lo, hi, name in spans:
            if lo <= i < hi:
                cur = name
                break
        where.append(cur)
    return lines, where


def defined(lines):
    """(name, index) for every function definition in the unit.

    A definition is a signature whose parentheses close into a `{` rather than
    a `;`.  Signatures wrap, so the parenthesis depth is carried forward until
    it closes and the character after it decides.
    """
    out, i = [], 0
    while i < len(lines):
        line = lines[i].split('//')[0]
        m = DEF.match(line)
        if not m or m.group('name').split('::')[-1] in KEYWORDS:
            i += 1
            continue
        # `= [&](...) {` is a lambda: named, but a local and not a function of
        # the program.  `struct X {` and `} while(...)` are not definitions.
        if '=' in line[:m.start('name')] or line.lstrip().startswith('}'):
            i += 1
            continue
        depth, j, closed = 0, i, False
        while j < len(lines) and not closed:
            for ch in lines[j].split('//')[0][m.end() - 1 if j == i else 0:]:
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth == 0:
                        closed = True
                elif closed and ch not in ' \t':
                    # First character after the signature: `{` is a body,
                    # `;` a declaration, `c` the start of `const` or `->`.
                    if ch in '{cn-':
                        break
                    closed = False
                    depth = 1
            if closed:
                break
            j += 1
        if not closed or j >= len(lines):
            i += 1
            continue
        rest = '\n'.join(lines[j:j + 3])
        if re.search(r'\)\s*(?:const\s*)?(?:noexcept\s*)?;', rest.split('\n')[0]):
            i += 1
            continue
        if '{' not in rest:
            i += 1
            continue
        out.append((m.group('name').split('::')[-1], i, j))
        i = j + 1
    return out


# Each rule answers "why is silence about this one acceptable?".  A rule that
# cannot be stated in a sentence does not belong here; the honest alternative
# is to describe the function.
RULES = [
    ('range coder', 'out of scope by the request these documents answer'),
    ('allocator', 'bmf_new / bmf_malloc / the arena -- storage, not coding'),
    ('file plumbing', 'open, close, tag, bytes-left'),
    ('platform shim', 'platform.inc: what a host lacks, not what the codec does'),
    ('template wrapper', 'body is one call to a <0>/<1> template, which is described'),
    ('one-liner', 'a body of one or two lines with no arithmetic to explain'),
]

# A ratchet, not a zero, and the distinction is the point.
#
# Two dishonest endings were available here.  Print the count and call it a
# report -- `sweep.sh` has a list of six tools that report rather than count,
# and a seventh nobody reads is easy to add.  Or invent exemption rules until
# the number reaches zero, which is the failure this whole tree keeps finding
# written the other way round: a rule that cannot fire reports the same zero as
# a clean tree.
#
# So it says what is true.  The documents do not describe every function, the
# number that do not is below, and it is not allowed to grow.  `dupblock.py`'s
# BUDGET is the same shape and went 45 -> 39 by being lowered deliberately;
# this one starts at what the documents actually cover on the day the tool was
# written, after a naming pass took it from 64.
#
# Lower it when you describe something.  Raising it is a decision to document
# less than the last round did, and should read like one in the diff.
#
# 64 when the tool was first run, then 50 after a naming pass, then 7 after
# working through the list.  The seven that remain are named in the output every
# run, and they are:
#
#   alt_p1_free  alt_p2_free  alt_p2_planes_free  free_sym_lists   frees
#   alt_p2_planes_alloc          the loop that calls the seeder, per plane
#   header_from                  eight fields copied from another header
#   strided_copy                 memcpy, or a byte at a time at two strides
#
# An eighth exemption rule would take them, and writing one was the wrong
# answer.  Every rule above says "this kind of function cannot carry an
# algorithm"; a rule broad enough to catch `strided_copy` and `header_from` is
# broad enough to catch something that does, and the two functions this pass
# nearly lost that way were `alt_p1_alloc` and `alt_p2_alloc` -- which look like
# allocators, are named like allocators, and between them seed 629,856 counter
# nodes, 163,840 bank counters and 15,552 three-way records with the priors the
# models start from.  Those are now 9.2 and 10.4.
#
# So the seven stay counted.  A list of seven in the output is readable; a rule
# that might quietly take an eighth is not.
BUDGET = 7

# Named by what encloses them, not by which file they are in, and that is not
# a stylistic preference -- it is the only version that answers the same way
# twice.
#
# The first draft exempted `rc.inc`, `memory.inc` and `file.inc` by filename.
# Run by hand on `bmf.cpp` that reported 50 undescribed; run by `sweep.sh`,
# which hands every tool an already-spliced copy under a `mktemp` name, it
# reported 63 -- the ten `RangeCoder` members and three of `file.inc`'s, because
# a spliced unit has no `#include` lines left to attribute anything to.  The
# sweep failed on it, which is the sweep working: a tool whose answer depends on
# how it was invoked has no answer.
#
# So the range coder is "a member of `struct RangeCoder`", which is true of the
# same ten functions whether or not the file boundary still exists.
RC_TYPE = 'RangeCoder'
RC_NAMES = re.compile(r'^rc_|^(?:pack|unpack)_bits$')
ALLOC_NAMES = re.compile(r'^bmf_(?:new|malloc|free|page_|arena_|bucket_)')
FILE_NAMES = re.compile(r'^(?:bmf_(?:open|close)_file|bmf_tag|bmf_tag_version'
                        r'|bytes_left)$')
# The stand-ins for calls a host does not have -- `platform.inc`, which is
# `open_memstream` and `fmemopen` today.  What belongs there is decided by the
# host and not by the codec, so an algorithm document that described
# `bmf_win_tempfile` would be describing Windows.  The second exemption that is
# a scope decision rather than a fact about the code, the range coder being the
# first.
#
# **A name rule and not a file rule, and that is worth writing down.**  The
# obvious spelling is `if file == 'platform.inc'`, and `exempt` is handed the
# file, so it looks right and it passes when this tool is run by hand.  It
# exempts nothing under `sweep.sh`, which is the run that gates: the sweep hands
# every tool the *spliced* unit, where the includes are already resolved and
# every line's origin is one temporary file.  Written that way this reported
# clean directly and four over the budget through the sweep, which is the
# failure that wastes an afternoon.
SHIM_NAMES = re.compile(r'^bmf_(?:mem(?:open|stream)_|win_)')
OPEN_TYPE = re.compile(r'^(?:struct|class)\s+(\w+)\s*(?:final\s*)?\{')


def enclosing(lines, at):
    """The `struct X {` this line is inside, or ''.

    Brace depth from the top of the unit, remembering the type opened at depth
    0.  Good enough for this tree, which nests no types.
    """
    depth, type_at, cur = 0, -1, ''
    for i in range(at):
        line = lines[i].split('//')[0]
        m = OPEN_TYPE.match(line.strip())
        if m and depth == 0:
            cur, type_at = m.group(1), depth
        depth += line.count('{') - line.count('}')
        if cur and depth <= type_at:
            cur = ''
    return cur


def exempt(name, file, lines, lo, hi):
    """Which rule lets this one go undescribed, or None."""
    if RC_NAMES.match(name) or enclosing(lines, lo) == RC_TYPE:
        return 'range coder'
    if ALLOC_NAMES.match(name):
        return 'allocator'
    if FILE_NAMES.match(name):
        return 'file plumbing'
    if SHIM_NAMES.match(name):
        return 'platform shim'
    body = [l.split('//')[0].strip() for l in lines[hi:hi + 6]]
    body = [l for l in body if l and l not in ('{', '}')]
    if body and WRAPPER.match(body[0]) and len(body) <= 2:
        return 'template wrapper'
    # A body written on the signature's own line -- `int32_t operator()(int32_t
    # i) const { return row[i].*Field; }` -- has no closing brace of its own on
    # a line of its own, so the scan below runs to the end of the unit and calls
    # the shortest body there is the longest one.  `RowOf::operator()` was the
    # first of this shape in the tree and the rule reported it.
    #
    # The semicolon count is what keeps `void f() { if(x) { a(); } b(); }` out:
    # a line can be balanced and still be a function worth explaining.
    tail = lines[hi].split('//')[0]
    if tail.count('{') and tail.count('{') == tail.count('}'):
        if tail[tail.index('{') + 1:tail.rindex('}')].count(';') <= 2:
            return 'one-liner'
    # Count to the closing brace at the definition's own indentation.
    indent = len(lines[lo]) - len(lines[lo].lstrip())
    n = 0
    for k in range(hi + 1, len(lines)):
        l = lines[k]
        if l.strip() == '}' and len(l) - len(l.lstrip()) == indent:
            break
        if l.split('//')[0].strip():
            n += 1
    if n <= 2:
        return 'one-liner'
    return None


def survey(path, docs):
    """(name, file, line, rule) for every function, rule None when undescribed."""
    lines, where = units(path)
    text = []
    for d in docs:
        try:
            text.append(open(d).read())
        except IOError:
            pass
    prose = '\n'.join(text)
    out, seen = [], set()
    for name, lo, hi in defined(lines):
        # Keyed on the enclosing type as well as the name, because a name is
        # not unique: two structs can each define `operator()`, and on the name
        # alone the second is skipped as already-seen -- so whichever came first
        # answers for both and a change to the other cannot be reported.
        key = (enclosing(lines, lo), name)
        if key in seen:
            continue
        seen.add(key)
        if re.search(r'\b%s\b' % re.escape(name), prose):
            continue
        out.append((name, where[lo], lo + 1,
                    exempt(name, where[lo], lines, lo, hi)))
    return out


def main():
    if '--why' in sys.argv:
        for rule, why in RULES:
            print('%-18s %s' % (rule, why))
        print('%d exemption rules' % len(RULES))
        return 0
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path = args[0] if args else 'bmf.cpp'
    try:
        open(path).close()
    except IOError:
        print('no such file: %s' % path)
        return 2
    found = survey(path, unstale.DOCS)
    missing = [f for f in found if f[3] is None]
    for name, file, line, rule in missing:
        print('%-28s %s:%d' % (name, file, line))
    if '--list' in sys.argv:
        for name, file, line, rule in found:
            if rule:
                print('%-28s %s:%d  (%s)' % (name, file, line, rule))
    over = max(0, len(missing) - BUDGET)
    print('%d functions above the coverage budget of %d; '
          '%d undescribed, %d exempted'
          % (over, BUDGET, len(missing), len(found) - len(missing)))
    return 1 if over else 0


if __name__ == '__main__':
    sys.exit(main())
