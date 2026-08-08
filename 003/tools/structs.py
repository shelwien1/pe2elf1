#!/usr/bin/env python3
"""Give one of the program's objects a struct, and use it.

    python3 tools/structs.py subs1.hpp --list
    python3 tools/structs.py subs1.hpp --apply 3      # object 3

`tools/objects.py` groups the names that denote the same allocation and reports
each one's field map.  This turns a map into a declaration and rewrites the
accesses:

    *(uint32_t *)(_this + 76)   ->   _this->f76
    *(uint8_t  *)(_this + 8)    ->   _this->f8

The target is 32-bit, and that is what makes it safe: a pointer is four bytes
there, so a struct laid out from the observed offsets has exactly the layout the
code already assumed.  Nothing moves, so the variable-offset walks these objects
also get -- `*(T *)(p + 4 * i)` -- keep meaning what they meant; they become
`*(T *)((char *)p + 4 * i)` and are left alone otherwise.  Every generated
struct carries a `static_assert` on its size to say so out loud.

Where one offset is read at two widths -- a byte inside a word, which is common
-- the widest wins and the narrow access goes through a cast of the member's
address.  Where an offset is dereferenced after loading, the member is a
pointer, which is what the 64-bit build would need widened and what the 32-bit
build merely needs named.

Trailing bytes matter: an object addressed at 0 and 1078692 gets a member at
each end and one big `pad` between, because the offsets in between belong to
arrays this cannot see the bounds of.  That is honest -- the struct describes
what is known and does not pretend about the rest.
"""
import collections
import re
import sys

WIDTH = {'char': 1, 'uint8_t': 1, 'int8_t': 1, 'uint16_t': 2, 'int16_t': 2,
         'int32_t': 4, 'uint32_t': 4, 'float': 4, 'int64_t': 8, 'uint64_t': 8,
         'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16, 'FILE': 4}


def width(ty):
    ty = ty.strip()
    return 4 if ty.endswith('*') else WIDTH.get(ty)


def bodies(lines):
    out, depth, start, name, sig = [], 0, None, None, None
    for i, l in enumerate(lines):
        s = l.split('//')[0]
        for k, ch in enumerate(s):
            if ch == '{':
                if depth == 0:
                    # walk back over a wrapped signature, but not past the
                    # statement before it -- a `struct X {` has no signature at
                    # all, and reaching back for one finds the previous
                    # function's and hands the struct its name
                    buf, j = s[:k], i
                    while '(' not in buf or buf.count(')') - buf.count('(') > 0:
                        j -= 1
                        if j < 0 or i - j > 6:
                            break
                        prev = lines[j].split('//')[0]
                        if prev.rstrip().endswith((';', '}', '{', ':')):
                            break
                        buf = prev + ' ' + buf
                    m = re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', buf)
                    name = m.group(1) if m and '(' in buf else None
                    sig, start = buf, i
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and start is not None:
                    if name:
                        out.append((start, i, name, sig))
                    start = None
    return out


def params_of(sig):
    m = re.search(r'\((.*)\)\s*\{?\s*$', sig)
    if not m:
        return []
    parts, depth, cur = [], 0, ''
    for ch in m.group(1):
        if ch in '(<':
            depth += 1
        elif ch in ')>':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return [(re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', p.strip()).group(1)
             if re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', p.strip()) else None)
            for p in parts]


class UF:
    def __init__(self):
        self.p = {}

    def find(self, x):
        self.p.setdefault(x, x)
        while self.p[x] != x:
            self.p[x] = self.p[self.p[x]]
            x = self.p[x]
        return x

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.p[ra] = rb


def classes(lines):
    fns = bodies(lines)
    params = {n: params_of(s) for _, _, n, s in fns}
    shim = {}
    for l in lines:
        m = re.match(r'^static inline .*?\b(__fwd_[A-Za-z0-9_]+)\s*\(.*?\)\s*\{\s*'
                     r'(?:return\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(', l)
        if m:
            shim[m.group(1)] = m.group(2)
    uf = UF()
    PLAIN = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = ([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CAST = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = '
                      r'\(?(?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\)?\s*'
                      r'([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CALL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^();]*)\)')
    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            c = l.split('//')[0]
            m = PLAIN.match(c) or CAST.match(c)
            if m:
                uf.union((fn, m.group(1)), (fn, m.group(2)))
            for cm in CALL.finditer(c):
                callee = shim.get(cm.group(1), cm.group(1))
                if callee not in params:
                    continue
                for i, arg in enumerate([x.strip() for x in cm.group(2).split(',')]):
                    # an optional cast, then the name -- and the cast has to be
                    # parenthesised, or the pattern eats the front of the name
                    # and calls `_this` an argument named `s`
                    am = re.fullmatch(r'(?:\(\s*(?:const\s+)?[A-Za-z_][A-Za-z0-9_]*'
                                      r'\s*\**\s*\)\s*)?'
                                      r'([A-Za-z_][A-Za-z0-9_]*)', arg)
                    if am and i < len(params[callee]) and params[callee][i]:
                        uf.union((fn, am.group(1)), (callee, params[callee][i]))
    return fns, uf


# a dereference whose operand has no parentheses left in it, so the terms of
# the address are plainly visible.  Rewriting these innermost-first exposes the
# ones that wrapped them.
INNER = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                   r'\(([^()]*)\)')
BARE = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                  r'(?:\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)|([A-Za-z_][A-Za-z0-9_]*))'
                  r'(?![\w\[])')
PTRFIELD = re.compile(r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)\('
                      r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)'
                      r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*(\d+)\s*\)')
BEFORE_SIGN = set('+-*/%<>=&|^!~(,?:')


def terms(text):
    """Split an address expression into its top-level `+`/`-` terms.

    `v65 + v59 + 3800` is three terms, and which of them is the object does not
    depend on the order the decompiler wrote them in.  Matching `name +` only
    ever found the object when it happened to come first, which is why a
    dereference like `*(uint16_t *)(v65 + v59 + 3800)` used to be missed.
    """
    out, depth, cur, sign, i = [], 0, '', '+', 0
    while i < len(text):
        ch = text[i]
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        binary = (depth == 0 and ch in '+-'
                  and not (ch == '-' and text[i + 1:i + 2] == '>')
                  and cur.strip() != ''
                  and cur.rstrip()[-1:] not in BEFORE_SIGN)
        if binary:
            out.append((sign, cur))
            cur, sign = '', ch
        else:
            cur += ch
        i += 1
    out.append((sign, cur))
    return out


def constant(rest):
    """The value of a term list, when every term is a literal."""
    total = 0
    for sign, text in rest:
        t = text.strip()
        if not re.fullmatch(r'0[xX][0-9a-fA-F]+|\d+', t):
            return None
        total += int(t, 0) * (1 if sign == '+' else -1)
    return total


def join(rest):
    return ''.join(('' if i == 0 and sg == '+' else ' %s ' % sg) + tx.strip()
                   for i, (sg, tx) in enumerate(rest))


def address_of(inner, isname):
    """Find the object term in a dereference's address, and what is added to it.

    Returns (name, rest) where rest is the remaining terms, or None.
    """
    ts = terms(inner)
    for i, (sg, tx) in enumerate(ts):
        if sg == '+' and isname(tx.strip()):
            return tx.strip(), ts[:i] + ts[i + 1:]
    return None


def survey(lines, fns, uf):
    fld = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    var = collections.Counter()
    ptr = collections.defaultdict(set)
    hits = collections.Counter()
    name = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')
    for a, b, fn, _ in fns:
        # a whole body at once, newlines and all: the decompiler wraps long
        # expressions, and a dereference split over two lines is still one
        # dereference
        c = '\n'.join(l.split('//')[0] for l in lines[a:b + 1])
        while True:                           # innermost dereferences first
            m = INNER.search(c)
            if not m:
                break
            got = address_of(m.group(2), lambda t: name.fullmatch(t))
            if got:
                k = uf.find((fn, got[0]))
                off = constant(got[1])
                if off is None:
                    var[k] += 1
                else:
                    fld[k][off][m.group(1).strip()] += 1
                    hits[k] += 1
            c = c[:m.start()] + '_' + c[m.end():]
        c = '\n'.join(l.split('//')[0] for l in lines[a:b + 1])
        for m in BARE.finditer(c):
            k = uf.find((fn, m.group(2) or m.group(3)))
            fld[k][0][m.group(1).strip()] += 1
            hits[k] += 1
        for m in PTRFIELD.finditer(c):
            ptr[uf.find((fn, m.group(1)))].add(int(m.group(2)))
    return fld, var, ptr, hits


TYPE = (r'(?:const\s+)?(?:unsigned\s+|signed\s+)?'
        r'(?:char|short|int|long|float|double|void|size_t|uintptr_t|FILE'
        r'|u?int(?:8|16|32|64)_t|__m128[id]?|Obj\d+)')
DECL = re.compile(r'^\s*(%s)((?:[\s\*]+[A-Za-z_][A-Za-z0-9_]*'
                  r'(?:\[[^\]]*\])?\s*,)*[\s\*]+[A-Za-z_][A-Za-z0-9_]*'
                  r'(?:\[[^\]]*\])?)\s*;' % TYPE)


def split_commas(text):
    parts, depth, cur = [], 0, ''
    for ch in text:
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return parts


def retype_param(line, nm, tag):
    """`char *_this` -> `Obj0 *_this`, inside a parameter list."""
    return re.sub(r'(?<=[(,])(\s*)(?:const\s+)?[A-Za-z_][A-Za-z0-9_]*'
                  r'\s*\**\s*(%s)\b(?=\s*[,)])' % re.escape(nm),
                  r'\1%s *\2' % tag, line)


def retype_local(out, a, b, nm, tag):
    """Pull nm out of whatever declaration list holds it, into its own line.

    A local can be one name in a wrapped list of forty.  Editing the list in
    place is what breaks: the type belongs to the whole statement, so the only
    way to give one name a different type is to take it out.  The statement is
    re-emitted without it and a dedicated declaration goes in above.
    """
    i = a
    while i <= b:
        m = DECL.match(out[i])
        if not m:
            i += 1
            continue
        j = i
        while ';' not in out[j] and j < b:
            j += 1
        text = ' '.join(l.strip() for l in out[i:j + 1])
        m = DECL.match(text)
        if not m or not re.search(r'[\s\*,]%s\b' % re.escape(nm), m.group(2)):
            i = j + 1
            continue
        keep = [p for p in split_commas(m.group(2))
                if not re.fullmatch(r'\s*\**\s*%s\s*' % re.escape(nm), p)]
        if len(keep) == len(split_commas(m.group(2))):
            i = j + 1
            continue
        ind = re.match(r'^(\s*)', out[i]).group(1)
        new = [ind + '%s *%s;' % (tag, nm)]
        if keep:
            head, line = '%s%s' % (ind, m.group(1)), None
            wrapped, cur = [], head
            for k, p in enumerate(keep):
                piece = p.strip() + (',' if k < len(keep) - 1 else ';')
                if len(cur) + len(piece) + 1 > 96 and cur != head:
                    wrapped.append(cur)
                    cur = ind + ' ' * (len(m.group(1)) + 1) + piece
                else:
                    cur += (' ' if cur == head else ' ') + piece
            wrapped.append(cur)
            new += wrapped
        out[i:j + 1] = new
        return len(new) - (j + 1 - i)
    return None


RESCALE = None


def rescales(lines, s, b, nm):
    """Uses of nm that pointer arithmetic would silently multiply.

    Once nm is `Obj0 *`, `nm + 4` steps by the whole struct, not four bytes.
    Dereferences are rewritten to go through `(char *)`, and plain `nm + k` can
    be too, but `nm += k` and `nm++` cannot -- a cast is not an lvalue.  An
    object with any of those is declined rather than quietly mis-stepped.
    """
    pat = re.compile(r'(\+\+|--)\s*%s\b|\b%s\s*(\+\+|--|\+=|-=)'
                     % (re.escape(nm), re.escape(nm)))
    return [i for i in range(s, b + 1) if pat.search(lines[i].split('//')[0])]


SKIP = 'tools/struct-skip.txt'


def signature(uf, key):
    """A name for an object that survives the file being rewritten.

    `--apply` takes an index into the list sorted by dereference count, and that
    list shifts under every apply: a rewritten object drops out of it.  So the
    skip list -- objects tried and reverted -- cannot be indices.  It is this
    instead: every `function:name` pair in the class, sorted.  It stays the same
    as long as the names do.
    """
    return ' '.join(sorted('%s:%s' % (fn, nm) for (fn, nm) in uf.p
                           if uf.find((fn, nm)) == key))


def main():
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    fns, uf = classes(lines)
    fld, var, ptr, hits = survey(lines, fns, uf)
    try:
        skip = {l.strip() for l in open(SKIP) if l.strip()
                and not l.startswith('#')}
    except IOError:
        skip = set()
    objs = [k for k in sorted(hits, key=lambda k: -hits[k])
            if signature(uf, k) not in skip]

    if '--skip' in sys.argv:
        key = objs[int(sys.argv[sys.argv.index('--skip') + 1])]
        with open(SKIP, 'a') as f:
            f.write(signature(uf, key) + '\n')
        print('skipped: %s' % signature(uf, key)[:100])
        return

    if '--list' in sys.argv or len(sys.argv) < 3:
        print('%3s %7s %8s %8s %8s  %s'
              % ('#', 'derefs', 'fields', 'var', 'ptrs', 'names'))
        for i, k in enumerate(objs[:20]):
            names = sorted({nm for (fn, nm) in uf.p if uf.find((fn, nm)) == k})
            print('%3d %7d %8d %8d %8d  %s'
                  % (i, hits[k], len(fld[k]), var[k], len(ptr[k]),
                     ' '.join(names[:5])))
        print('\n%d objects' % len(objs))
        return

    n = int(sys.argv[sys.argv.index('--apply') + 1])
    key = objs[n]
    names = {(fn, nm) for (fn, nm) in uf.p if uf.find((fn, nm)) == key}
    offs = sorted(fld[key])
    # not the index: that shifts under every apply, and two structs must not
    # end up sharing a name.  The first number the file does not already use.
    used = {int(m.group(1)) for l in lines
            for m in [re.match(r'struct Obj(\d+) \{', l)] if m}
    tag = 'Obj%d' % next(k for k in range(1000) if k not in used)

    # widest access at each offset wins; a pointer field is one whose value is
    # dereferenced
    member, at, decl = {}, 0, []
    for off in offs:
        tys = fld[key][off]
        best = max(tys, key=lambda t: (width(t) or 0, tys[t]))
        w = width(best)
        if w is None:
            print('%s: cannot size %s at +%d' % (tag, best, off))
            return
        if off < at:
            member[off] = None          # inside an earlier member
            continue
        if off > at:
            decl.append('  uint8_t _pad%d[%d];' % (len(decl), off - at))
        nm = 'f%d' % off
        star = '' if best.endswith('*') else ' '
        decl.append('  %s%s%s;' % (best, star, nm))
        member[off] = (nm, best)
        at = off + w

    body = ('// %s -- recovered from %d dereferences over %d offsets, under %d\n'
            '// name%s.  The layout is the one the code already assumed: at 32 bits a\n'
            '// pointer is four bytes, so naming these fields moves nothing, and the\n'
            '// static_assert is what says so.  Offsets the code only reaches with a\n'
            '// computed index are padding here -- their bounds are not visible.\n'
            'struct %s {\n%s\n};\n'
            'static_assert(sizeof(void *) != 4 || sizeof(%s) == %d,\n'
            '              "%s: the layout moved");\n'
            % (tag, hits[key], len(offs), len({nm for _, nm in names}),
               '' if len({nm for _, nm in names}) == 1 else 's',
               tag, '\n'.join(decl), tag, at, tag))

    if '--show' in sys.argv:
        print(body)
        return

    byfn = collections.defaultdict(set)
    for fn, nm in names:
        byfn[fn].add(nm)
    spans = {fn: (a, b) for a, b, fn, _ in fns}

    out = list(lines)
    spanof = {}
    for fn, nms in byfn.items():
        if fn not in spans:
            continue
        a, b = spans[fn]
        # the signature can be a line or two above the body's `{`
        s = a
        while s > 0 and '(' not in out[s]:
            s -= 1
        spanof[fn] = (s, a, b)
        for nm in nms:
            bad = rescales(out, s, b, nm)
            if bad:
                with open(SKIP, 'a') as f:
                    f.write(signature(uf, key) + '\n')
                print('%s: declined -- %s is stepped as a pointer at line %d'
                      % (tag, nm, bad[0] + 1))
                sys.exit(3)

    changed = 0
    for fn, nms in byfn.items():
        if fn not in spanof:
            continue
        s, a, b = spanof[fn]
        def field(ty, nm, off):
            """The text for one access at a known offset."""
            home = max((o for o in member if member[o] and o <= off),
                       default=None)
            if home is None or member[home] is None:
                return None
            mem, mty = member[home]
            if home == off:
                return ('%s->%s' % (nm, mem) if mty == ty
                        else '*(%s *)&%s->%s' % (ty, nm, mem))
            return '*(%s *)((char *)&%s->%s + %d)' % (ty, nm, mem, off - home)

        # the whole body as one string.  Newlines survive, so the wrapping the
        # decompiler chose survives with them, but `\s` in these patterns
        # crosses them -- which it has to, because an expression that runs over
        # three lines is still one expression.
        text = '\n'.join(out[s:b + 1])
        pos = 0
        while True:                           # innermost dereferences first
            m = INNER.search(text, pos)
            if not m:
                break
            got = address_of(m.group(2), lambda t: t in nms)
            new = None
            if got:
                nm, rest = got
                off = constant(rest)
                if off is None:
                    # keep it byte arithmetic: the object is a struct now, and
                    # every other term in the sum stays an offset
                    new = '*(%s *)((char *)%s + (intptr_t)(%s))' % (
                        m.group(1).strip(), nm, join(rest))
                else:
                    new = field(m.group(1).strip(), nm, off)
            if new is None:
                pos = m.start() + 2
                continue
            text = text[:m.start()] + new + text[m.end():]
            pos = 0                           # the outer dereference may match now
        for nm in nms:
            # *(T *)nm -> nm->f0
            if member.get(0):
                mem, mty = member[0]
                text = re.sub(r'\*\((?:const )?%s\s*\*\)'
                              r'(?:\(\s*%s\s*\)|%s)(?![\w\[])'
                              % (re.escape(mty), re.escape(nm), re.escape(nm)),
                              '%s->%s' % (nm, mem), text)
            # what is left of `nm + k` is byte arithmetic, and must stay byte
            # arithmetic now that nm points at a struct
            text = re.sub(r'(?<!\(char \*\))(?<![>\w.&])%s\b'
                          r'(?=\s*(?:\+(?!\+)|-(?![->])))' % re.escape(nm),
                          '(char *)%s' % nm, text)
        new_lines = text.split('\n')
        assert len(new_lines) == b + 1 - s, 'rewrite changed the line count'
        changed += sum(1 for x, y in zip(out[s:b + 1], new_lines) if x != y)
        out[s:b + 1] = new_lines

    # retype the declarations, last, so the spans above are still valid
    for fn, nms in sorted(byfn.items(), key=lambda kv: -spanof.get(kv[0], (0,))[0]):
        if fn not in spanof:
            continue
        s, a, b = spanof[fn]
        for nm in nms:
            for i in range(s, a + 1):
                out[i] = retype_param(out[i], nm, tag)
            if any(re.search(r'\b%s\s*\*+\s*%s\b' % (tag, re.escape(nm)), l)
                   for l in out[s:a + 1]):
                continue
            delta = retype_local(out, a, b, nm, tag)
            if delta is None:
                print('%s: no declaration found for %s in %s' % (tag, nm, fn))
            else:
                b += delta

    anchor = next(i for i, l in enumerate(out) if l.startswith('struct RangeCoder'))
    out = out[:anchor] + body.split('\n') + [''] + out[anchor:]
    open(path, 'w').write('\n'.join(out))
    print('%s: %d lines rewritten across %d functions' % (tag, changed, len(byfn)))


if __name__ == '__main__':
    main()
