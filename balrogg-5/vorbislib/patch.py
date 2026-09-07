"""Make a pristine libvorbis + libogg compile inside one C++ translation unit.

Mechanical changes only; everything else is untouched:
  1. `class`, `this` and `new` are C++ keywords and C identifiers.
  2. `_vorbis_block_alloc` returns void*, so its *calls* route through a
     wrapper -- including the three in block.c, but not its definition there.
  3. psy.c and floor1.c each carry a private FLOOR1_fromdB_LOOKUP, and
     codebook.c and sharedbook.c each a private bitreverse -- fine in
     separate units, clashes in one.
  4. vshim.h has to land after the headers but outside any conditional block,
     or the allocators it wraps are not in scope where they are used.  The
     leading `#ifdef HAVE_CONFIG_H` block is part of the prologue, not the
     first conditional: framing.c opens with one, and stopping there left it
     without the shim.
  5. The `void *` fields libvorbis keeps its per-object state in, and the
     `void *ret;` locals libogg reallocs through, become vb_voidp -- a type
     with void*'s layout and C's implicit conversion to any object pointer.
     That is what makes the several hundred

         codec_setup_info * ci = vi->codec_setup;

     reads compile without touching a single one of them.  See vb_voidp.h.
  6. memchr returns void* in C++ as well, and framing.c assigns it straight
     to an unsigned char*.
  7. libvorbis initialises the int fields of _psy_global_44 with the double
     literals `99.` and `0.`; C++ calls that narrowing and rejects it.

Together these make the translation unit compile clean under g++ and clang++,
at c++11 through c++23, with **no -fpermissive**.  That flag used to be load
bearing: g++ downgrades void*-to-T* to a warning under it, clang does not,
which is why the same tree built with gcc and failed with clang.

Every rule is idempotent, so re-running this over an already-patched tree is
a no-op.
"""
import re, glob
rd = lambda f: open(f, 'rb').read().decode('latin-1')
wr = lambda f, s: open(f, 'wb').write(s.encode('latin-1'))
SRC = sorted(glob.glob('*.c'))
ALL = SRC + glob.glob('*.h') + glob.glob('modes/*.h') + glob.glob('books/*/*.h')
SHIM = ('vshim.h', 'vb_voidp.h')                 # ours, not libvorbis's
ALL = [f for f in ALL if f not in SHIM]

for f in ALL:
    s = rd(f); o = s
    s = re.sub(r'\bclass\b', 'vclass', s)
    s = re.sub(r'\bthis\b',  'v_this', s)
    s = re.sub(r'\bnew\b',   'v_new',  s)
    if s != o: wr(f, s)

for f in SRC:
    s = rd(f); o = s
    #  (2) the definition in block.c keeps the name its callers link against
    s = re.sub(r'(?<!void \*)\b_vorbis_block_alloc\(', 'VB_ALLOC(', s)
    if f == 'psy.c':
        s = s.replace('FLOOR1_fromdB_LOOKUP', 'PSY_fromdB_LOOKUP')
    if f == 'sharedbook.c':          # same private helper name as codebook.c
        s = re.sub(r'\bbitreverse\b', 'sb_bitreverse', s)
    #  (5) libogg's realloc idiom: `void *ret; ret = _ogg_realloc(...);
    #      os->body_data = ret;` -- the last line is the one C++ rejects
    s = re.sub(r'(?m)^([ \t]*)void[ \t]*\*[ \t]*(\w+)[ \t]*;', r'\1VB_VOIDP \2;', s)
    #  (6)
    s = s.replace("next=memchr(", "next=(unsigned char *)memchr(")
    s = s.replace("next=(unsigned char *)(unsigned char *)memchr(",
                  "next=(unsigned char *)memchr(")           # idempotent
    #  (4) insert the shim after the include prologue, treating a leading
    #      HAVE_CONFIG_H block as part of it
    if '#include "vshim.h"' not in s:
        lines = s.split('\n')
        i, stop = 0, len(lines)
        while i < len(lines):
            l = lines[i].lstrip()
            if l.startswith('#if') and 'HAVE_CONFIG_H' in l:
                depth = 1; i += 1
                while i < len(lines) and depth:
                    t = lines[i].lstrip()
                    if   t.startswith('#if'):    depth += 1
                    elif t.startswith('#endif'): depth -= 1
                    i += 1
                continue
            if l.startswith('#if'):
                stop = i; break
            i += 1
        head = [j for j in range(stop) if lines[j].lstrip().startswith('#include')]
        if head: lines.insert(max(head) + 1, '#include "vshim.h"')
        s = '\n'.join(lines)
    if s != o: wr(f, s)

#  (5) the state pointers themselves.  vb_voidp is a class template-bearing
#      type, so its header goes in above the `extern "C"` block, which cannot
#      carry one.
FIELDS = {
  'vorbis/codec.h': [
    ('  void *codec_setup;',        '  VB_VOIDP codec_setup;'),
    ('  void       *backend_state;','  VB_VOIDP    backend_state;'),
    ('  void *internal;',           '  VB_VOIDP internal;'),
  ],
  'codec_internal.h': [
    ('  vorbis_look_transform **transform[2];    /* block, type */',
     '  VB_VOIDP               *transform[2];    /* block, type; vorbis_look_transform * */'),
    ('  vorbis_look_floor     **flr;',
     '  VB_VOIDP               *flr;             /* vorbis_look_floor *   */'),
    ('  vorbis_look_residue   **residue;',
     '  VB_VOIDP               *residue;         /* vorbis_look_residue * */'),
    ('  vorbis_info_mapping    *map_param[64];',
     '  VB_VOIDP                map_param[64];   /* vorbis_info_mapping * */'),
    ('  vorbis_info_floor      *floor_param[64];',
     '  VB_VOIDP                floor_param[64]; /* vorbis_info_floor *   */'),
    ('  vorbis_info_residue    *residue_param[64];',
     '  VB_VOIDP                residue_param[64]; /* vorbis_info_residue * */'),
  ],
  'highlevel.h': [('  const void *setup;', '  VB_CVOIDP   setup;')],
}
GUARDS = {                       # (include guard line, path to vb_voidp.h)
  'vorbis/codec.h': ('#define _vorbis_codec_h_', '../vb_voidp.h'),
  'ogg/ogg.h':      ('#define _OGG_H',           '../vb_voidp.h'),
}
for f, reps in FIELDS.items():
    s = rd(f); o = s
    for a, b in reps:
        if b not in s:
            assert s.count(a) == 1, '%s: no unique match for %r' % (f, a)
            s = s.replace(a, b)
    if s != o: wr(f, s)
for f, (guard, inc) in GUARDS.items():
    s = rd(f)
    if inc in s: continue
    note = ('/*  Outside the extern "C" block below: vb_voidp is a class template-\n'
            '    bearing type, which cannot be given C language linkage.  */\n'
            '#include "%s"\n' % inc)
    assert s.count(guard) == 1, f
    wr(f, s.replace(guard, guard + '\n\n' + note, 1))

#  (8) res0_free_info memsets vorbis_info_residue0 and vorbis_encode_residue_setup
#      memcpys a template over it; a const member makes copy-assignment deleted
#      and both writes UB in C++ (-Wclass-memaccess).  The note deliberately
#      avoids the three identifiers rule 1 rewrites -- backends.h goes through it.
f = 'backends.h'; s = rd(f)
NOTE = ('  /*  Not const: res0_free_info memsets the struct and\n'
        '      vorbis_encode_residue_setup memcpys a template over it.  A const\n'
        '      member makes copy-assignment deleted and both writes UB in C++\n'
        '      (-Wclass-memaccess); nothing actually relies on the qualifier.  */\n')
PLAIN = '  int classmetric1[64];\n  int classmetric2[64];'
CONST = '  const int classmetric1[64];\n  const int classmetric2[64];'
if NOTE + PLAIN not in s:
    for a in (CONST, PLAIN):
        if a in s:
            wr(f, s.replace(a, NOTE + PLAIN)); break

#  (9) psy.c clamps halfoc to P_BANDS-1 and then interpolates between
#      noiseoff[inthalfoc] and noiseoff[inthalfoc+1] -- so at the top of the
#      range it reads noiseoff[P_BANDS], one past the end of a float[17].
#      del is 0 there, so the value is multiplied away and the result was
#      never wrong; but the read is out of bounds (UBSan flags it), and if the
#      adjacent bytes ever decoded to inf or NaN the 0 would not save it.
#      Clamping to the last interval instead of the last point keeps
#      inthalfoc+1 in range and yields exactly noiseoff[P_BANDS-1] either way.
f = 'psy.c'; s = rd(f)
a = """    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;"""
b = """    inthalfoc=(int)halfoc;
    if(inthalfoc>P_BANDS-2)inthalfoc=P_BANDS-2;   /* keep inthalfoc+1 in range */
    del=halfoc-inthalfoc;"""
if b not in s:
    assert s.count(a) == 1
    wr(f, s.replace(a, b))

#  (10) psy.c packs a pair of indices into one long as ((lo-1)<<16)+(hi-1),
#       and lo starts at 0, so the first band shifts -1 left.  Shifting a
#       negative value is undefined before C++20 (UBSan flags it).  lo and hi
#       are bounded by n <= 4096, so (lo-1)*65536 cannot overflow an int and
#       gives the identical value at every standard.
f = 'psy.c'; s = rd(f)
a = '    p->bark[i]=((lo-1)<<16)+(hi-1);'
b = '    p->bark[i]=((lo-1)*65536)+(hi-1);   /* not <<: lo-1 is -1 on the first band */'
if b not in s:
    assert s.count(a) == 1
    wr(f, s.replace(a, b))

#  (7) int fields, double literals
f = 'modes/psych_44.h'; s = rd(f)
a = '   {99.},{{99.},{99.}},{0},{0},{{0.},{0.}}'
b = '   {99},{{99},{99}},{0},{0},{{0},{0}}'
if a in s: wr(f, s.replace(a, b))

print("patched %d sources" % len(SRC))
