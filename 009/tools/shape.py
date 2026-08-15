#!/usr/bin/env python3
"""Measure the things REFACTORING3.md is about, so its numbers can be checked.

    python3 tools/shape.py                  # the §1 table
    python3 tools/shape.py --this           # raw offsets off `_this`, by function
    python3 tools/shape.py --frames         # one line per frame
    python3 tools/shape.py --bss            # the bmf_bss globals and their groups
    python3 tools/shape.py --overlap Obj8 Obj11 Obj19 Obj31 Obj69

`BMF_STRICT=1 ./build.sh` is the scoreboard for round two's goal.  This is the
scoreboard for round three's: a raw-offset dereference is a place where the
layout of an object is written as arithmetic instead of declared, and the count
is what Phase A is trying to move.

It is a counter, not a target -- `exclusion_mask[symbol]` is a byte buffer being
indexed and will never become a member.  See REFACTORING3.md §6.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import buildlog                                                   # noqa: E402
import negindex                                                 # noqa: E402
import ptrwidth                                                 # noqa: E402
import rawoffset                                                # noqa: E402
import structs                                                    # noqa: E402

# The file to report on.  This was a constant, so `shape.py other.hpp`
# silently reported on `subs1.hpp` -- which made every figure in it look
# identical across four commits when they were being read from one file.
SRC = next((a for a in sys.argv[1:] if not a.startswith('--')), 'subs1.hpp')

# A type name with any number of stars: `char *`, `uint32_t **`, `Obj11 *`.
T = r'[A-Za-z_]\w*\s*\**'

# The three shapes a raw offset arrives in.  P1 is `(T *)(base + n)`, with an
# optional byte-cast on the base; P2 is `((T *)base + n)`, where the `+ n` is
# scaled; P3 is the reversed form Hex-Rays emits when the index came first,
# `*(uint8_t *)(idx + (char *)base + 984)`.
# The base of the arithmetic.  A bare name was all this matched for six
# rounds, so `(uint8_t *)frame.acc1 + result` -- a byte offset off a frame
# member -- was not a raw offset as far as the row was concerned, and neither
# was one off any struct member.  Lifting six frames in round nine turned seven
# such sites into `(uint8_t *)acc1 + result` and the row went from 5 to 12
# without a line of arithmetic changing, which is what said the pattern was the
# thing at fault.  With member and subscript bases allowed it reads 56.
B = (r'((?:[A-Za-z_]\w*)(?:(?:\.|->)[A-Za-z_]\w*|\[[^\]\[]*\])*)')
P1 = re.compile(r'\(\s*%s\s*\*\s*\)\s*\(\s*(?:\(\s*%s\s*\*\s*\)\s*)?'
                r'%s\s*\+' % (T, T, B))
P2 = re.compile(r'\(\s*\(\s*%s\s*\*\s*\)\s*%s\s*\+' % (T, B))
P3 = re.compile(r'\+\s*\(\s*%s\s*\*\s*\)\s*%s\b' % (T, B))

# A frame, by its declaration.  This used to key on a `// NNN bytes` tag the
# decompiler's output carried and every frame since has been given a name
# instead, so the tag went and took all six frame rows to zero with it.  The
# byte count went with it: two of the six hold a union this cannot size, and a
# census that guessed at those two would be worse than one that counts members.
FRAME = re.compile(r'struct alignas\(16\) (\w+Frame)\s*\{')

# A label, told apart from the other things that end in a colon at the start of
# a line: an access specifier, a `default:` arm, a `::` qualification.
LABEL = (r'(?!default\b|public\b|private\b|protected\b)'
         r'([A-Za-z_]\w*):(?!:)')
FRAME_MEMBER = re.compile(
    r'^\s*(?:const\s+)?[A-Za-z_][\w ]*?\s*\**\s*\w+\s*(?:\[[^\]]*\])*\s*;\s*$')
# An alias binds one name to one frame member.  Two spellings: `T &v = …` for a
# scalar and `T (&v)[N] = …` for an array member -- missing the second is how
# this file's first alias count came out 38 short.
ALIAS = re.compile(r'(?:&\s*|\(&)([A-Za-z_]\w*)\)?(?:\[\d+\])?\s*=\s*'
                   r'(?:.*?)frame\.(\w+(?:\[\d+\])?)')
RUN = re.compile(r'\(\s*&(\w+)\s*\+\s*')
BSS = re.compile(r'^static (t_\w+)& (\w+) = \*\(t_\w+\*\)\(bmf_bss \+ (0x[0-9A-F]+)', re.M)
TYPEDEF = re.compile(r'^typedef\s+(.+?)\s+(t_\w+)\s*(\[[^\]]*\])?\s*;', re.M)


_UNIT = {}


def unit():
    """The whole translation unit, as the compiler reads it.

    This read one file for as long as there was one file to read, and it kept
    doing it after the split: `bmf.cpp` is 195 lines of include list with two
    bodies in it, so every row below counted a program that was not there.  The
    census said zero frames while six are declared, zero gotos while there are
    45, and `sweep.sh` exempts this tool from the every-count-is-zero rule --
    it reports a table rather than a count -- so a table of zeroes about the
    wrong file was the shape of wrong answer nothing here could see.  The same
    defect `addrmap.py` had, found the same way, on the same afternoon.
    """
    if SRC not in _UNIT:
        lines, origin = structs.splice(SRC)
        _UNIT[SRC] = (lines, '\n'.join(lines), origin)
    return _UNIT[SRC]


def load():
    lines, _text, _origin = unit()
    span, sigs = {}, {}
    # `defs`, not `bodies`: a method sits one level in, and the sixteen bodies
    # that moved inside a record would otherwise be in no function's span at
    # all -- every site in them attributed to whatever body came before.
    for a, b, nm, sig, _depth in structs.defs(lines):
        sigs[nm] = (a, b, sig)
        for ln in range(a, b + 1):
            span[ln] = nm
    return lines, span, sigs


def rawoffsets(lines, span):
    """Every raw-offset site, as (line, function, base).

    One site, not one pattern: `*(uint32_t *)((char *)_this + 278720)` is P1
    with a byte-cast base *and* P2 on that inner cast, and counting it twice
    made retyping a parameter look like it doubled the arithmetic in a function
    when all it did was add the cast the new type needs.  Keyed by where the
    base identifier sits, so each one counts once.
    """
    out = []
    for i, l in enumerate(lines):
        code = l.split('//')[0]
        at = {}
        for p in (P1, P2, P3):
            for m in p.finditer(code):
                at.setdefault(m.start(1), m.group(1))
        for pos in sorted(at):
            out.append((i + 1, span.get(i, '<top>'), at[pos]))
    return out


def frames(lines):
    """One dict per function that has a frame.

    A frame declared at file scope belongs to the body that declares one --
    `ReduceAlphabetFrame` moved out so its two arms could split, and it is
    still one function's frame.  Found by the declaration of the variable, not
    of the type.
    """
    out = []
    ftype = {}
    for i, l in enumerate(lines):
        m = FRAME.search(l)
        if m:
            depth, mem = 0, 0
            for l2 in lines[i:]:
                c = l2.split('//')[0]
                depth += c.count('{') - c.count('}')
                # Every leaf declaration, at any depth, and not the padding.
                # Counting only depth 1 said `decode_symbol_list` holds one
                # member: its whole frame is a union of an array and the eight
                # spill slots that overlay it, and the row for the frame this
                # tree cares most about read 1.
                if FRAME_MEMBER.match(c) and not re.search(
                        r'\b(?:_\w*pad\w*|BMF_SPILL_PAD)', c):
                    mem += 1
                if depth <= 0 and l2 is not lines[i]:
                    break
            ftype[m.group(1)] = mem
    for a, b, nm, sig, _depth in structs.defs(lines):
        body = lines[a:b + 1]
        held = [ftype[t] for l in body
                for t in re.findall(r'\b(\w+Frame)\b\s+\w+\s*;', l)
                if t in ftype]
        held += [ftype[m.group(1)] for m in map(FRAME.search, body)
                 if m and m.group(1) in ftype]
        if not held:
            continue
        sizes = held
        aliases, names = [], set()
        for l in body:
            m = ALIAS.search(l)
            if m:
                aliases.append((m.group(1), m.group(2)))
                names.add(m.group(1))
        slots = collections.Counter(t for _, t in aliases)
        runs = collections.Counter()
        for l in body:
            if ALIAS.search(l):
                continue
            for m in RUN.finditer(l.split('//')[0]):
                if m.group(1) in names:
                    runs[m.group(1)] += 1
        out.append(dict(fn=nm.lstrip('_'), size=sum(sizes), alias=len(aliases),
                        slots={t: c for t, c in slots.items() if c > 1}, runs=runs))
    return out


# The five things the 60 globals in bmf_bss really are; see REFACTORING3.md §3.1.
# A starts at 0x44338C and not at 0x44339C: `__n256_2`, `__n512`, `plane_count`
# and `near_lossless_max` are record 0 of the same 16-byte-record table, which
# `alt_model_p2_encode` walks with those four names as its field bases.
GROUPS = [('A  plane descriptors', 0x44338C, 0x4433DB),
          ('B  level geometry', 0x445714, 0x445733),
          ('C  thresholds + biases', 0x4458E0, 0x4458F7)]
BUFFERS = {'exclusion_mask', '__byte_445440', 'model_geometry'}


def bss():
    src = unit()[1]
    ty = {m.group(2): m.group(1) + (m.group(3) or '') for m in TYPEDEF.finditer(src)}
    out = []
    for m in BSS.finditer(src):
        addr = int(m.group(3), 16)
        if m.group(2) in BUFFERS:
            grp = 'D  buffers'
        else:
            grp = next((g for g, lo, hi in GROUPS if lo <= addr <= hi), 'E  scalars')
        uses = len(re.findall(r'\b%s\b' % re.escape(m.group(2)), src)) - 1
        subs = [' '.join(s.split()) for s in
                re.findall(re.escape(m.group(2)) + r'\s*\[([^\]]*)\]', src)]
        out.append(dict(name=m.group(2), addr=addr, ty=ty.get(m.group(1), '?'),
                        group=grp, uses=uses, subs=subs))
    return sorted(out, key=lambda g: (g['group'], g['addr']))


MEMBER = re.compile(r'^\s*([A-Za-z_][\w ]*?\s*\**)\s*(\w+)\s*(\[[^\]]*\])?\s*;', re.M)


def fields():
    """{struct: ({offset: type} for fNN members, [named members])}."""
    src = unit()[1]
    out = {}
    for m in re.finditer(r'^struct\s+(\w+)\s*\{(.*?)^\};', src, re.S | re.M):
        got, named = {}, []
        for f in MEMBER.finditer(m.group(2)):
            o = re.fullmatch(r'f(\d+)', f.group(2))
            if o:
                got[int(o.group(1))] = f.group(1).strip() + (f.group(3) or '')
            elif not f.group(2).startswith('_pad'):
                named.append(f.group(2))
        out[m.group(1)] = (got, named)
    return out


# Widths on this target; `unify_types.py` has already made the spellings
# uniform, so the aka forms only turn up for types with no typedef name.
WIDTH = {'uint8_t': 8, 'int8_t': 8, 'char': 8, 'bool': 8, 'unsigned char': 8,
         'signed char': 8, 'uint16_t': 16, 'int16_t': 16, 'short int': 16,
         'short unsigned int': 16, 'uint32_t': 32, 'int32_t': 32, 'int': 32,
         'unsigned int': 32, 'size_t': 32, 'uintptr_t': 32, 'float': 32,
         'long int': 32, 'long unsigned int': 32, 'uint64_t': 64,
         'int64_t': 64, 'double': 64}
CONV = re.compile(r"warning: (?:unsigned )?conversion (from|to) '([^']+)'"
                  r"(?: \{aka '[^']*'\})? (?:to|from) '([^']+)'")
# A negative constant reaching an unsigned type is its own thing: the compiler
# names the value it becomes, so it is either a mistake or a deliberate wrap,
# and never the ambient type mixture the other rows are about.
CONST = re.compile(r'warning: unsigned conversion')
RATCHET = re.compile(r'\[-W(?:conversion|sign-conversion|sign-compare'
                     r'|useless-cast)\]')


def warnkinds(log='warn.log'):
    """{kind: count} over warn.log, or {} when it is about another file.

    The log's line numbers and counts belong to the source it was built from.
    Handed an old revision -- which `proven.sh` does, and `checktable.py` now
    passes the path through -- this used to report the *working* tree's
    warnings under that revision's other rows.  `buildlog` is the same check
    REFACTORING9.md section 35 put in four other places.
    """
    out = collections.Counter()
    rows, note = buildlog.read(log, SRC)
    if note:
        return out
    for l in rows:
        if not RATCHET.search(l):
            continue
        m = CONV.search(l)
        if not m:
            # Everything the ratchet counts has a row, so the rows add up to
            # the ratchet.  A breakdown that quietly drops two of them is the
            # kind of measurement section 10 is about.
            out['sign-compare' if 'sign-compare' in l else 'useless cast'] += 1
            continue
        if CONST.search(l):
            out['a negative constant into an unsigned type'] += 1
            continue
        src, dst = ((m.group(3), m.group(2)) if m.group(1) == 'to'
                    else (m.group(2), m.group(3)))
        a, b = WIDTH.get(src), WIDTH.get(dst)
        if a is None or b is None:
            out['of an unrecognised type'] += 1
        elif b < a:
            out['narrowing %d -> %d' % (a, b)] += 1
        elif b == a:
            out['signedness, same width'] += 1
        else:
            out['widening'] += 1
    return out


def summary():
    lines, span, sigs = load()
    src = '\n'.join(lines)
    raw = rawoffsets(lines, span)
    this = [r for r in raw if r[2] == '_this']
    fr = frames(lines)
    members = fields()
    named = sum(len(v[1]) for v in members.values())
    # `--rows` prints the same table tab-separated, so `checktable.py` can
    # compare it against the copy in REFACTORING9.md §1 without guessing where
    # a label ends -- which it cannot do reliably, because a long label leaves
    # one space before its value and a short one leaves twenty.
    row = ((lambda k, v: print('%s\t%s' % (k.strip(), v)))
           if '--rows' in sys.argv else
           (lambda k, v: print('%-34s %s' % (k, v))))
    # One value a row.  A row whose value is two numbers and a clause cannot be
    # compared against the copy of this table in REFACTORING9.md §1 without the
    # checker parsing prose, and the point of `checktable.py` is that it does
    # not have to.
    # The basename, not the path: run against a copy in a temp directory --
    # which `sweep.sh` and `proven.sh` both do -- the label carried the whole
    # of `/tmp/tmp.XXXX/subs1.hpp`, and `checktable.py` then found no row by
    # the name section 1 quotes.
    # One row, not two.  These were the file and the unit when they were two
    # different things; the argument is the unit's root now and the second row
    # was a copy of the first.
    row('%s, spliced' % SRC.rsplit('/', 1)[-1], len(lines) - 1)
    row('raw-offset sites', len(raw))
    row('  off `_this`', len(this))
    row('  in functions', len(set(r[1] for r in this)))
    # The row above counts `_this + ofs`, which is the shape Hex-Rays emits for
    # a small member.  It said 0 for four rounds while thirty sites in
    # `alt_p2_model` reached the same records as `bankp[4 * ctx + 284712]` --
    # an offset written as the number it is on i386.  These two are the
    # measures that can see that, and the one under them the third way a record
    # can be addressed by width rather than by name.
    row('offsets written as numbers', len(rawoffset.survey(lines)))
    row('  negative indices, unsigned name',
        sum(1 for r in negindex.survey(lines)
            if r[3].startswith('uint') or r[3] == '?'))
    # And the fourth way, which is neither an offset nor an index: a pointer
    # put in an integer *name*, where the cast and the narrowing are lines
    # apart.  Counted here and not from `ptrwidth.py`'s total, because that
    # total needs a build log for the target and this row must answer for any
    # file the census is pointed at.
    row('  pointers parked in a name', len(ptrwidth.parked(lines)))
    # `sizeof(void *)` is not a cast, and thirty-three of them -- one per
    # layout assertion -- were in this figure from the day it was written.
    # The lookbehind is the whole correction.
    casts = re.findall(r'(?<!sizeof)\(\s*([A-Za-z_]\w*)\s*(\*+)\s*\)\s*(.?)',
                       '\n'.join(l.split('//')[0] for l in lines))
    row('pointer casts', len(casts))
    # And what they are, because the total says how many are left and not
    # whether any of them should be -- the same defect the `goto` row had.
    # A cast of `&x` is naming a byte inside an object, a cast to a declared
    # record is the type doing its job, and only the first group is a place
    # where a declaration could still replace arithmetic.
    kinds = collections.Counter()
    for ty, stars, nxt in casts:
        rec = ty[:1].isupper() or ty == 'FILE'
        kinds['%s, of an address' % ('to a record' if rec else 'to a scalar')
              if nxt == '&' else
              'to a record' if rec else 'to a scalar'] += 1
    for k, v in sorted(kinds.items(), key=lambda kv: -kv[1]):
        row('  ' + k, v)
    row('globals still at a 1997 address', len(bss()))
    # `alignas` on a declaration is a fact about the program that nothing else
    # here counts, and a tool dropped six of them without anything noticing --
    # REFACTORING9.md section 39.  It is a spelling, but it is one that no
    # rewrite should ever change by accident, and a row that moves when one
    # does is the cheapest guard available.  `struct alignas(16)` is the
    # frames' own, counted by the `frames` row above.
    row('declarations carrying alignas',
        sum(1 for l in lines
            if 'alignas(' in l.split('//')[0]
            and not re.search(r'\bstruct\s+alignas', l.split('//')[0])))
    # One value a row, for the same reason `goto` and `pointer casts` were
    # split: a row whose value is three numbers and a clause is a row
    # `checktable.py` cannot compare, so REFACTORING9.md section 1 carried no
    # frame figure at all -- the one thing the round was asked about was the
    # one thing the checked table could not say.
    row('frames', len(fr))
    row('  members they hold', sum(f['size'] for f in fr))
    row('  aliases left in them', sum(f['alias'] for f in fr))
    row('  slots carrying two names', sum(len(f['slots']) for f in fr))
    row('  extra names on those slots',
        sum(sum(c - 1 for c in f['slots'].values()) for f in fr))
    row('  member runs walked as arrays',
        sum(sum(f['runs'].values()) for f in fr))
    # This row used to be "frames that dissolve outright", counting the ones
    # with neither an aliased slot nor a member run to unpick first.  Once the
    # last alias went it equalled the row above it exactly, under a name that
    # reads as "nine frames could just be removed" while `liftframe.py`
    # reported none it could offer.  A row whose value is another row's under a
    # different name is not a measurement; these three are what is actually
    # known about the nine, and they add up to it.
    try:
        import liftframe
        offers, declined = liftframe.candidates(lines)
        row('  this can offer to lift', len(offers))
        row('  tried and reverted', len(liftframe.PROVEN))
        # "every member in a union" was the only reason `liftframe` had when
        # this row was written, and it now declines on three: the union, a
        # member the body indexes out of, and a frame declared at file scope.
        # A row that names one reason while counting three is the same defect
        # as a count that names the wrong file.
        row('  declined, with a reason', len(declined))
    except Exception:                                             # noqa: BLE001
        pass
    row('structs', len(members))
    row('  still ObjN', sum(1 for k in members if re.fullmatch(r'Obj\d+', k)))
    row('  fNN members / named ones',
        '%d / %d' % (sum(len(v[0]) for v in members.values()), named))
    # `distinct` counts spellings across the whole file, so it cannot move until
    # a name is gone from all 59 bodies -- naming every local in one function
    # leaves it unchanged.  The two rows under it are the ones that measure
    # that work: bodies still carrying any, and how many uses in total.
    #
    # `tNN` counts too, and that is the point.  Renaming `v112` to `t48` in a
    # body whose arithmetic is not understood changes the vocabulary and not
    # the understanding, and a measure that fell for that would be exactly the
    # kind of thing REFACTORING9.md section 10 is about.  A `tNN` is an honest
    # label for an intermediate nobody has explained -- contiguous, and marked
    # as this body's rather than Hex-Rays' -- but it is still unexplained, and
    # the number says so.
    #
    # So does `nNNNN`, which this row missed for the whole of the naming work.
    # Hex-Rays names a local after the first constant it sees stored in it, so
    # `n1840_1`, `n0x7FFFFFFF_10` and `n2` are the same kind of non-name as
    # `v112` -- 198 of them, never counted, while the row read as if the only
    # question left were `vNN`.  The digit after the `n` is required: `nb0` is
    # a name, and `b0` is valid hex.
    # And behind a prefix.  `rename.py` disambiguates a local against a global
    # of the same name by putting the function's name in front of it, so
    # `__code_pixel_n0x2000` is a Hex-Rays name that `\bn0x...` cannot match --
    # there is no word boundary before the `n`.  Six of them sat in the file
    # while this row read zero.  The suffix has to be a whole Hex-Rays name:
    # `run_dv3` and `w4_c` are not, and do not match.
    hexrays = r'(?:[vt]\d+|n(?:0x[0-9A-Fa-f]+|\d[0-9A-Fa-f]*)(?:_\d+)?)'
    unexp = r'\b(?:\w+_)?%s\b' % hexrays
    bodyv = [len(re.findall(unexp,
                            '\n'.join(l.split('//')[0] for l in lines[a:b + 1])))
             for a, b, _, _ in structs.bodies(lines)]
    # Code only, like the two rows under it.  Counting `src` counted the
    # comments as well, and this file's comments quote the old names on
    # purpose -- `// was int32_t v312` beside the padding that replaced it.
    # The row read 81 when every body was clean and one parameter was left.
    code = '\n'.join(l.split('//')[0] for l in lines)
    row('distinct unexplained locals', len(set(re.findall(unexp, code))))
    row('  bodies still carrying one', '%d of %d' % (sum(1 for n in bodyv if n),
                                                     len(bodyv)))
    row('  uses', sum(bodyv))
    # And the fifth correction, which is a different kind of name entirely.
    # Hex-Rays also names a local after the *callee parameter* it is passed as,
    # so an argument to `fread` becomes `Buffer`, `ElementCount` or `Stream`
    # and the copies get `a`, `b`, `c` and `_N`.  None of those match `[vt]NN`
    # or `nNNNN`, so this row read zero through the whole naming round while
    # `unmodel_plane_slow` carried ten locals called `ArgList` through
    # `ArgList_10` and the I/O layer carried eighty more.
    #
    # The vocabulary is MSVC's CRT headers and is listed rather than guessed:
    # a name is only counted if it is one of these, which is why `InName`,
    # `Colours` and `gA` are not in the row despite the capital.
    # The prefix and the long suffix are the same correction §1's `nNNNN` row
    # needed and for the same reason: `rename.py` puts the function's name in
    # front of a local that collides with a global, so
    # `__expand_image_Buffer_1` is a Hex-Rays name behind a disguise, and a
    # suffix of one or two characters misses `Buffer_copy` and `Block_plane`.
    # Five of them sat in the file while this row read zero -- on the day it
    # was written.
    crt = (r'(?:\w+_)?(?:ArgList|Block|Buffer|Count|Dst|ElementCount|FileName'
           r'|Format|Memory|Mode|NewSize|Offset|Origin|Position|Size|Src|Str'
           r'|Stream|lpAddress)[a-c]*(?:_\w+)?')
    hits = collections.Counter()
    for a, b, nm, sig in structs.bodies(lines):
        for name in structs.decl_types(sig, lines, a, b):
            if re.fullmatch(crt, name):
                hits[name] += 1
    # And the row that is not a spelling at all.  `unnamed.py` joins the file
    # against the first commit -- which *is* the Hex-Rays output -- through
    # `addrmap.py`'s address map, so a local declared under one name in both is
    # by construction a name nobody here has chosen.  Four spelling rows, four
    # corrections; this one has nothing to get wrong.
    try:
        import unnamed
        live, on_purpose, joined = unnamed.survey(SRC)
    except Exception:
        live = None
    row('locals named for a callee parameter', len(hits))
    row('  declarations / bodies', '%d / %d' % (
        sum(hits.values()),
        sum(1 for a, b, nm, sig in structs.bodies(lines)
            if any(re.fullmatch(crt, n)
                   for n in structs.decl_types(sig, lines, a, b)))))
    if live is not None:
        row('names Hex-Rays chose and nobody changed',
            '%d' % sum(len(v) for v in live.values()))
        row('  conventional ones kept / bodies joined',
            '%d / %d' % (sum(len(v) for v in on_purpose.values()), joined))
    # Both halves used to be wrong, in opposite directions: `src.count('goto ')`
    # counted two comments that mention a `goto` that is no longer there, and
    # `^LABEL_\d+:` missed the two labels that are indented.  Strip the comments
    # and allow the indent.
    code = '\n'.join(l.split('//')[0] for l in src.split('\n'))
    # What shape the remaining jumps are.  The bare count says how many are
    # left and not whether any of them should be: a forward jump out of nested
    # blocks to one join, or a backward jump to the top of a loop, is what a
    # `goto` is for in C.  A jump *into* a block, or forward at the same depth
    # to something that is not a join, is a branch or a loop with its shape
    # missing -- and those are the ones this round removed.
    # Whether a label is a join is the half of that sentence this used to state
    # and not check.  A label reached from two places is a join whatever the
    # depths are; one reached from a single `goto` and from nowhere else is a
    # branch written inside out.  Both of the file's `same`-depth jumps turn
    # out to go to joins -- one to a row terminator five paths reach, one to
    # the raw-write path an `if` also falls into -- so the row was reporting
    # two things to fix that were not there.  They are counted apart now
    # rather than dropped, because a breakdown that stops mentioning part of
    # what it breaks down is §10's whole subject.
    def jumpshapes():
        out = collections.Counter()
        STOP = re.compile(r'(?:goto\s+\w+|return\b[^;]*|break|continue)\s*;\s*$')
        for a, b, _nm, _sig, _d in structs.defs(lines):
            labs, depth, at = {}, 0, {}
            for i in range(a, b + 1):
                c = lines[i].split('//')[0]
                at[i] = depth
                # Not `name:$`: `keep_flag8: { … }` puts its statement on the
                # same line, and requiring end-of-line left one `goto` in
                # `search_filter` with no target to classify -- 44 of 45
                # accounted for, and nothing said which one was missing.
                m = re.match(r'\s*%s' % LABEL, c)
                if m:
                    labs[m.group(1)] = i
                depth += c.count('{') - c.count('}')
            preds = collections.Counter()
            for i in range(a, b + 1):
                for m in re.finditer(r'goto (\w+)', lines[i].split('//')[0]):
                    preds[m.group(1)] += 1
            for name, tgt in labs.items():
                j = tgt - 1
                while j > a and not lines[j].split('//')[0].strip():
                    j -= 1
                if not STOP.search(lines[j].split('//')[0].strip()):
                    preds[name] += 1          # also reached by falling into it
            for i in range(a, b + 1):
                for m in re.finditer(r'goto (\w+)', lines[i].split('//')[0]):
                    tgt = labs.get(m.group(1))
                    if tgt is None:
                        continue
                    out['back' if tgt < i else
                        'into' if at[tgt] > at[i] else
                        'out' if at[tgt] < at[i] else
                        'join' if preds[m.group(1)] > 1 else 'same'] += 1
        return out
    shapes = jumpshapes()
    # Named, not `LABEL_n`.  This counted the decompiler's numbered labels and
    # nothing else, and every one of them has had a name for two rounds -- so
    # it read `0 / 0` over 45 jumps, and the four rows under it, which do the
    # real work of saying what shape each jump is, read zero with it.  A row
    # that can only ever count something the tree no longer has is a row that
    # reports the rename rather than the jumps.
    row('goto / label:',
        '%d / %d' % (len(re.findall(r'goto \w+;', code)),
                     len(re.findall(r'^\s*%s' % LABEL, code, re.M))))
    row('  still spelled LABEL_n',
        len(re.findall(r'^\s*LABEL_\d+:', code, re.M)))
    row('  restart a loop / exit N blocks',
        '%d / %d' % (shapes['back'], shapes['out']))
    row('  sideways to a join / to neither',
        '%d / %d' % (shapes['join'], shapes['same']))
    row('  jump into a block',
        '%d' % shapes['into'])
    # What the conversion ratchet is made of.  The total says how many are
    # left and not whether any of them can be fixed, which is the same defect
    # the goto row had: a narrowing into a field the 1997 layout fixes at
    # sixteen bits is not the same kind of thing as a local declared against
    # its own assignments, and only the second is `resign.py`'s business.
    row('conversion warnings', '%d' % sum(warnkinds().values()))
    for k, v in sorted(warnkinds().items(), key=lambda kv: -kv[1]):
        row('  ' + k, v)
    row('__fwd_* shims', len(set(re.findall(r'\b__fwd_\w+', src))))


def narrowings():
    """Where each narrowing conversion lands, which the width rows cannot say.

    §15 said the narrowings are "into fields the 1997 layout fixes at eight or
    sixteen bits" and that no declaration can fix them.  Half of that is true.
    Measured rather than asserted, under half of them land in a struct member;
    the rest are stores through a pointer, `LOWORD(x) = …` on a local that
    cannot narrow because something reads it whole, stores into a local that is
    deliberately narrow, and expressions that are not a store at all.  None of
    those is a declaration's mistake either, but they are not one group and
    saying they were was a guess.
    """
    # A warning names the file the line is *in*, which since the split is one of
    # the thirty-seven includes and never `bmf.cpp`.  Matching only `SRC` meant
    # this row could not see a warning at all; it reads zero today because the
    # build has no warnings, which is the one condition under which a broken
    # reader and a correct one agree.
    lines, _text, origin = unit()
    at = {}
    for i, (fn, ln) in enumerate(origin):
        at.setdefault((fn, ln), i)
    out, ex = collections.Counter(), collections.defaultdict(list)
    for l in buildlog.read('warn.log', SRC)[0]:
        m = re.match(r'(?:.*/)?([\w.\-]+):(\d+):(\d+): (.*)', l)
        if not m or (m.group(1), int(m.group(2))) not in at:
            continue
        m = type('M', (), {'group': staticmethod(
            lambda k, _m=m: {1: str(at[(_m.group(1), int(_m.group(2)))] + 1),
                             2: _m.group(3), 3: _m.group(4)}[k])})
        c = CONV.search(m.group(3))
        if not c:
            continue
        # `CONV` matches both orders -- "conversion from 'X' to 'Y'" and
        # "conversion to 'Y' from 'X'" -- and group 1 says which.
        src_t, dst_t = ((c.group(2), c.group(3)) if c.group(1) == 'from'
                        else (c.group(3), c.group(2)))
        a, b = WIDTH.get(src_t), WIDTH.get(dst_t)
        if not a or not b or b >= a:
            continue
        src = lines[int(m.group(1)) - 1].split('//')[0]
        mm = re.match(r'\s*(.+?)\s*(?:[-+*/|&^]|>>|<<)?=(?!=)\s*(.+);\s*$', src)
        if not mm:
            k = 'not a store on this line'
        else:
            lhs = mm.group(1)
            if re.search(r'(?:->|\.)\w+\s*(?:\[[^\]]*\])?$', lhs):
                k = 'into a struct member'
            elif lhs.startswith('*') or lhs.endswith(']'):
                k = 'through a pointer or subscript'
            elif re.fullmatch(r'\w+', lhs):
                k = 'into a local'
            else:
                k = 'another store, mostly LOWORD/LOBYTE'
        out[k] += 1
        if len(ex[k]) < 2:
            ex[k].append((m.group(1), src.strip()[:66]))
    return out, ex


def main():
    if '--narrow' in sys.argv:
        out, ex = narrowings()
        for k, v in out.most_common():
            print('%5d  %s' % (v, k))
            for ln, t in ex[k]:
                print('         %6s  %s' % (ln, t))
        print('%5d  narrowings in all' % sum(out.values()))
    elif '--this' in sys.argv:
        lines, span, sigs = load()
        this = collections.Counter(r[1] for r in rawoffsets(lines, span)
                                   if r[2] == '_this')
        for fn, n in this.most_common():
            sig = sigs.get(fn, (0, 0, ''))[2]
            m = re.search(r'\(\s*([^,)]*?)\s*_this', sig)
            print('%-28s %4d   %s' % (fn.lstrip('_'), n, m.group(1) if m else '?'))
    elif '--frames' in sys.argv:
        for f in sorted(frames(unit()[0]), key=lambda f: -f['size']):
            print('%-26s %4d members  %3d aliases  %2d shared slots  %2d run sites'
                  % (f['fn'], f['size'], f['alias'], len(f['slots']),
                     sum(f['runs'].values())))
    elif '--bss' in sys.argv:
        for g in bss():
            kinds = sorted(set(g['subs']))
            shape = ('' if not kinds else
                     'const' if all(re.fullmatch(r'\d+|0x[0-9A-Fa-f]+', s)
                                    for s in kinds) else
                     'var[%s]' % ', '.join(kinds[:3]))
            print('%-8s %-20s 0x%06X  %-18s uses=%-4d %s'
                  % (g['group'][0], g['name'], g['addr'], g['ty'], g['uses'], shape))
    elif '--overlap' in sys.argv:
        want = sys.argv[sys.argv.index('--overlap') + 1:]
        f = {k: v[0] for k, v in fields().items()}
        keys = sorted(set().union(*(set(f.get(n, {})) for n in want)))
        keys = [k for k in keys if sum(1 for n in want if k in f.get(n, {})) >= 2]
        print('   '.join(['%-8s' % 'offset'] + ['%-14s' % n for n in want]))
        for k in keys:
            print('   '.join(['%-8d' % k]
                             + ['%-14s' % f.get(n, {}).get(k, '-') for n in want]))
    else:
        summary()
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:                    # `| head`, which is how this is read
        sys.exit(0)
