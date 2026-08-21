#!/usr/bin/env python3
"""Mutable state at file scope -- the one thing two codecs would share.

    python3 tools/shared.py bmf.cpp          # report
    python3 tools/shared.py --selftest       # and prove it can

`BMFCodec` exists so that several codecs can run in one process without
touching each other's state, and that claim is exactly as strong as the
statement "nothing the codec touches lives at file scope".  Nothing checked it.

**Three of the round's defects were this and were found downstream.**
`byte_list_ent` and `gap_list_ent` were `static` arrays that the allocation
round introduced as *storage* rather than as state, so the census of globals
that preceded the class never looked at them; `tools/parallel.cpp` found them by
watching two codecs disagree about `t8g` by 24 bytes.  `bmf_p2_coef` and
`bmf_p2_rate` were the same shape and it took ThreadSanitizer.  Both would have
been one line of this.

**What counts.**  A definition at brace depth zero, starting in column one, that
declares an object rather than a function or a type, and is not `const` or
`constexpr`.  A `const` table is shared by every codec and that is the point of
it being const; what matters is whether anything can *write* it.

**What is allowed, and why each one is.**  The list below is the whole of it and
every entry carries its reason.  Anything else is a finding -- not a suggestion,
because there is no way for a file-scope mutable to be right here.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

ALLOWED = {
    # `main`'s own codec, and the only place a `BMFCodec` is named outside the
    # class.  It is an *instance*, not state the class reaches for -- a second
    # one is a second object and that is what `tools/parallel.cpp` makes eight
    # of.  File-scope rather than a local of `main` because 86 MB is more than a
    # thread's stack.
    'bmf_codec': "main's single instance; a second codec is a second object",
    # The four below are the high-arena allocator, compiled only under
    # `-DBMF_HIGH_ARENA`.  They cannot be per-codec: an allocator per codec would
    # defeat what that leg exists to prove, which is that the program does not
    # care where its memory is.  They are under a spinlock instead -- see the
    # note at `bmf_arena_lock` in memory.inc for why that lock rests on
    # inspection rather than on a test that went red.
    'bmf_arena': 'the high-arena leg; under bmf_arena_lock',
    'bmf_arena_used': 'the high-arena leg; under bmf_arena_lock',
    'bmf_arena_lock': 'the lock itself',
    'bmf_bucket': 'the high-arena leg; under bmf_arena_lock',
}

# `alignas(N)` is a qualifier on a declaration and has parentheses in it, which
# would otherwise make every aligned global look like a function.  This tree
# puts `alignas(16)` on most of what used to be a global, so getting this wrong
# would have turned the tool off for exactly the declarations it is about.
ALIGNAS = re.compile(r'^\s*alignas\s*\([^)]*\)\s*')
# A keyword that introduces something other than an object.
NOT_AN_OBJECT = re.compile(r'^(using|typedef|struct|class|enum|union|template'
                           r'|namespace|extern|friend|public|private|protected'
                           r'|static_assert)\b')
NAME = re.compile(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:=|;)')


def statements(lines):
    """Every depth-zero statement that starts in column one, joined."""
    out, depth, buf, first = [], 0, '', 0
    for i, l in enumerate(lines):
        code = l.split('//')[0]
        opens = code.count('{')
        closes = code.count('}')
        if depth == 0 and not buf:
            if not code[:1] or code[:1] in ' \t#}':
                depth += opens - closes
                continue
            first = i + 1
        if depth == 0 or buf:
            buf += ' ' + code.strip()
            if code.rstrip().endswith(';') and depth + opens - closes == 0:
                out.append((first, ' '.join(buf.split())))
                buf = ''
        depth += opens - closes
    return out


def findings(path):
    lines = structs.splice(path)[0]
    out = []
    for line, s in statements(lines):
        s = ALIGNAS.sub('', s)
        if NOT_AN_OBJECT.match(s):
            continue
        if re.search(r'\b(const|constexpr)\b', s.split('=')[0]):
            continue
        # A function -- a prototype or a definition -- has a parenthesis in its
        # declarator.  An object's initialiser may have one (`= f(x)`), so only
        # the part before the first `=` is looked at.
        if '(' in s.split('=')[0]:
            continue
        m = NAME.search(s)
        if not m or m.group(1) in ALLOWED:
            continue
        out.append((line, m.group(1), s[:80]))
    return out


_PLANTS = [
    ('static int32_t planted_counter;', True),
    ('alignas(16) static float planted_table[16];', True),
    ('static uint8_t* planted_cursor = nullptr;', True),
    ('static const int32_t planted_limit = 3;', False),
    ('static constexpr float planted_ratio = 0.5f;', False),
    ('void planted_fn(int32_t k);', False),
    ('static int32_t planted_call(int32_t k) { return k; }', False),
    ('typedef int32_t planted_alias;', False),
]


def selftest():
    """A skip is what turns a counting tool off, so both halves are planted."""
    import os, tempfile
    bad = 0
    for text, want in _PLANTS:
        fd, path = tempfile.mkstemp(suffix='.cpp', dir='.')
        os.write(fd, (text + '\n').encode())
        os.close(fd)
        try:
            got = bool(findings(path))
        finally:
            os.unlink(path)
        if got != want:
            bad += 1
            print('  %-9s %s' % ('missed' if want else 'false', text))
    print('%d of %d self-tests disagree' % (bad, len(_PLANTS)))
    return 1 if bad else 0


def main():
    if '--selftest' in sys.argv:
        sys.exit(selftest())
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/shared.py bmf.cpp [--selftest]')
    found = findings(sys.argv[1])
    for line, name, s in found:
        print('  %-26s %s:%d' % (name, sys.argv[1], line))
        print('      %s' % s)
    print('%d mutable objects at file scope, beyond the %d allowed'
          % (len(found), len(ALLOWED)))


if __name__ == '__main__':
    main()
