#!/usr/bin/env python3
"""Lift the frame members that are provably not part of a layout.

    python3 tools/defram.py subs1.hpp --list
    python3 tools/defram.py subs1.hpp __alt_p2_model
    python3 tools/defram.py subs1.hpp --all

Round one's `reframe.py` did the opposite: it gave 8 plain-split frames their
struct back, after one of them segfaulted because its locals had stopped being
adjacent.  This is the other direction, and it is only safe one member at a
time.  A member stays in the struct if any of four things is true:

* **its address is taken** anywhere in the body.  `&v` is how a body walks from
  one member to the next -- `*(&lpAddress + n4)` is the array of plane
  workspaces in REFACTORING3.md §2.2 -- and an address that escapes into a call
  can be walked just as far.
* **it is an array member.**  An array name decays at every use, so passing it
  is taking its address.  `model_planes` writes a four-word image descriptor
  across `p_i`, `v49`, `v50` and `v51`, reaching the last three by writing 16
  bytes through the first.
* **it shares its slot with another name.**  MSVC reused the stack slot;
  splitting the names is a decision about which one means what, not a rewrite
  (REFACTORING3.md §4.2).
* **it is one of its frame's run bases**, per `shape.py`.

Everything else becomes an ordinary declaration where its alias stood, and a
`uint8_t _gapN[width]` takes the member's place so that **every surviving
member keeps its offset** and the `static_assert`s still mean what they said.
A frame with nothing left but padding loses its struct and its asserts.

The rules are static analysis, so they are answerable to the gate like
everything else: run the whole of it, out-of-memory ladder included, after
every batch.  What the ladder caught here was `model_planes`, whose frame looks
clean by the shared-slot and run-site tests and dies on five of the eight test
images.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import shape                                                      # noqa: E402
import structs                                                    # noqa: E402

SCALAR = re.compile(r'^(\s*)(.+?)\s*&\s*(\w+)\s*=\s*(.*?)__frame\.(\w+)\s*;\s*$')
ARRAY = re.compile(r'^(\s*)(.+?)\s*\(&(\w+)\)(\[\d+\])\s*=\s*(.*?)__frame\.(\w+)\s*;\s*$')
OPEN = re.compile(r'^(\s*)struct alignas\(16\) \{\s*// (\d+) bytes, the frame')
MEMBER = re.compile(r'^(\s*)([A-Za-z_][\w ]*?\s*\**)\s*(\w+)\s*(\[(\d+)\])?\s*;(.*)$')
ANYALIAS = re.compile(r'^\s*[^=;]*&\s*\(?\w+\)?(?:\[\d+\])?\s*=[^;]*__frame\.\w+[^;]*;\s*$')
ASSERT = re.compile(r'^\s*(static_assert|"|\|\||==)')

# -m32, which the file is pinned to; see build.sh.
WIDTH = {'char': 1, 'int8_t': 1, 'uint8_t': 1, 'int16_t': 2, 'uint16_t': 2,
         'int32_t': 4, 'uint32_t': 4, 'int64_t': 8, 'uint64_t': 8,
         'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16}


def width(ty, count):
    ty = ty.strip()
    n = WIDTH.get(ty, 4 if ty.endswith('*') else None)
    return None if n is None else n * (int(count) if count else 1)


def addr_taken(text, name):
    """`&name` in an address-of position, not `a & name` and not `&&`."""
    for m in re.finditer(r'&\s*' + re.escape(name) + r'\b', text):
        j = m.start() - 1
        while j >= 0 and text[j] == ' ':
            j -= 1
        if j < 0:
            return True
        if text[j].isalnum() or text[j] in '_)]&':
            continue
        return True
    return False


def parse(lines, a, b):
    """The frame's struct span, its members, and every alias in the body."""
    start = None
    for i in range(a, b + 1):
        if OPEN.match(lines[i]):
            start = i
            break
    if start is None:
        return None
    close = start
    while close <= b and not lines[close].lstrip().startswith('} __frame;'):
        close += 1
    members = [dict(line=i, ind=m.group(1), ty=m.group(2).strip(),
                    name=m.group(3), count=m.group(5))
               for i in range(start + 1, close)
               for m in [MEMBER.match(lines[i])] if m]
    asserts = [i for i in range(close + 1, b + 1)
               if ASSERT.match(lines[i]) and i < close + 8]
    al = []
    for i in range(close + 1, b + 1):
        m = ARRAY.match(lines[i])
        if m:
            al.append(dict(line=i, ind=m.group(1), name=m.group(3),
                           decl='%s %s%s' % (m.group(2), m.group(3), m.group(4)),
                           member=m.group(6), array=True))
            continue
        m = SCALAR.match(lines[i])
        if m:
            ty = m.group(2)
            al.append(dict(line=i, ind=m.group(1), name=m.group(3),
                           decl='%s%s%s' % (ty, '' if ty.endswith('*') else ' ',
                                            m.group(3)),
                           member=m.group(5), array=False))
    return dict(open=start, close=close, members=members, aliases=al,
                asserts=asserts)


def split(lines, a, b, fr, runs):
    """The aliases that can leave, and the ones that stay, with the reason."""
    # Every alias declaration, including the forms the two regexes above do not
    # parse -- `int32_t &v61 = *(int32_t *)((char *)&__frame.Blocka_5);` is
    # `model_plane` giving one slot a second name and a second type.  A member
    # named by one of those is shared; the line itself is a declaration and not
    # evidence that anything walks the frame.
    parsed = set(x['line'] for x in fr['aliases'])
    seen, shared, decl = set(), set(), set(parsed)
    for x in fr['aliases']:
        (shared if x['member'] in seen else seen).add(x['member'])
    for i in range(fr['close'] + 1, b + 1):
        if i in parsed or not ANYALIAS.match(lines[i]):
            continue
        decl.add(i)
        for m in re.finditer(r'__frame\.(\w+)', lines[i]):
            shared.add(m.group(1))
    text = '\n'.join(lines[i].split('//')[0]
                     for i in range(a, b + 1) if i not in decl)
    # An address that escapes does not stop at the member it was taken from.
    # `alt_p2_context` takes `&v275`, and lifting the forty members after it
    # segfaults every image but the one-plane one -- so an address-taken
    # member, an array member or a run base freezes the *whole* frame, not
    # itself.  Only a shared slot is local to its own member.
    frozen = [x['name'] for x in fr['aliases']
              if x['array'] or x['name'] in runs or addr_taken(text, x['name'])]
    if '&__frame' in text:
        frozen.append('&__frame')       # the struct itself is addressed
    if frozen:
        why = 'frame pinned by ' + ', '.join(sorted(set(frozen))[:3])
        return [], [dict(x, why=why) for x in fr['aliases']]
    go, stay = [], []
    for x in fr['aliases']:
        why = 'shared slot' if x['member'] in shared else None
        (stay if why else go).append(dict(x, why=why))
    return go, stay


def apply(lines, fr, go):
    """Rewrite in place: aliases become declarations, members become padding."""
    gone = set(x['member'] for x in go)
    edits = {}
    for x in go:
        edits[x['line']] = ['%s%s;' % (x['ind'], x['decl'])]
    n = 0
    for m in fr['members']:
        if m['name'] not in gone:
            continue
        w = width(m['ty'], m['count'])
        if w is None:
            return None, 'no width for `%s %s`' % (m['ty'], m['name'])
        edits[m['line']] = ['%suint8_t   _gap%d[%d];   // was %s %s'
                            % (m['ind'], n, w, m['ty'], m['name'])]
        n += 1
    # a frame with no member the body still names is a frame with no readers
    if not [m for m in fr['members']
            if m['name'] not in gone and not m['name'].startswith('_')]:
        for i in range(fr['open'], fr['close'] + 1):
            edits[i] = []
        for i in fr['asserts']:
            edits[i] = []
    out = []
    for i, l in enumerate(lines):
        out.extend(edits[i] if i in edits else [l])
    return out, None


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    shape.SRC = path
    lines = open(path).read().split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]
    runs = {f['fn']: set(f['runs']) for f in shape.frames(lines)}

    plans = []
    for a, b, nm, sig in structs.bodies(lines):
        fr = parse(lines, a, b)
        if fr and fr['aliases']:
            go, stay = split(lines, a, b, fr, runs.get(nm.lstrip('_'), set()))
            plans.append((nm, fr, go, stay))

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for nm, fr, go, stay in plans:
            why = {}
            for x in stay:
                why[x['why']] = why.get(x['why'], 0) + 1
            print('%-26s lift %3d of %3d   %s'
                  % (nm.lstrip('_'), len(go), len(fr['aliases']),
                     'the whole frame' if not stay else
                     ', '.join('%d %s' % (v, k) for k, v in sorted(why.items()))))
        print('%d of %d aliases lift' % (sum(len(p[2]) for p in plans),
                                         sum(len(p[1]['aliases']) for p in plans)))
        return 0

    want, done = set(args), 0
    for nm, fr, go, stay in reversed(plans):          # back to front
        if (want and nm not in want) or not go:
            continue
        new, err = apply(lines, fr, go)
        if err:
            print('skipped %s: %s' % (nm.lstrip('_'), err))
            continue
        lines = new
        done += len(go)
        print('%-26s %3d lifted, %d stay' % (nm.lstrip('_'), len(go), len(stay)))
    if done:
        open(path, 'w').write('\n'.join(lines))
    print('%d aliases lifted' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
