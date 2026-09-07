"""Make a pristine libvorbis + libogg compile inside one C++ translation unit.

Four mechanical changes; everything else is untouched:
  1. `class`, `this` and `new` are C++ keywords and C identifiers.
  2. `_vorbis_block_alloc` returns void*, so its *calls* route through a
     wrapper -- but not its definition in block.c.
  3. psy.c and floor1.c each carry a private FLOOR1_fromdB_LOOKUP, and
     codebook.c and sharedbook.c each a private bitreverse -- fine in
     separate units, clashes in one.
  4. vshim.h has to land after the headers but outside any conditional block,
     or the allocators it wraps are not in scope where they are used.

The remaining C-isms -- implicit void* conversions -- are accepted by
-fpermissive rather than edited into hundreds of call sites.
"""
import re, glob
rd = lambda f: open(f, 'rb').read().decode('latin-1')
wr = lambda f, s: open(f, 'wb').write(s.encode('latin-1'))
SRC = sorted(glob.glob('*.c'))
ALL = SRC + glob.glob('*.h') + glob.glob('modes/*.h') + glob.glob('books/*/*.h')

for f in ALL:
    s = rd(f); o = s
    s = re.sub(r'\bclass\b', 'vclass', s)
    s = re.sub(r'\bthis\b',  'v_this', s)
    s = re.sub(r'\bnew\b',   'v_new',  s)
    if s != o: wr(f, s)

for f in SRC:
    s = rd(f)
    if f != 'block.c':
        s = s.replace('_vorbis_block_alloc(', 'VB_ALLOC(')
    if f == 'psy.c':
        s = s.replace('FLOOR1_fromdB_LOOKUP', 'PSY_fromdB_LOOKUP')
    if f == 'sharedbook.c':          # same private helper name as codebook.c
        s = re.sub(r'\bbitreverse\b', 'sb_bitreverse', s)
    lines = s.split('\n')
    stop = next((i for i, l in enumerate(lines) if l.lstrip().startswith('#if')), len(lines))
    head = [i for i, l in enumerate(lines[:stop]) if l.lstrip().startswith('#include')]
    if head: lines.insert(max(head) + 1, '#include "vshim.h"')
    wr(f, '\n'.join(lines))
print("patched %d sources" % len(SRC))
