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
    ty = re.sub(r'^\s*const\s+', '', ty.strip()).strip()
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
                        # a directive is not a signature, and `#pragma
                        # pack(push, 1)` above a struct has the parenthesis this
                        # is looking for -- which made deadcheck.py report a
                        # function called `pack` that nothing calls
                        if prev.lstrip().startswith('#'):
                            break
                        buf = prev + ' ' + buf
                    # A brace after `=` opens an initialiser, not a body.
                    # `alignas(16) static uint8_t ctx_group_flags[32] = {` has
                    # a parenthesis and a name in front of it, so this read
                    # twelve of the file's global tables as functions called
                    # `alignas` -- and every per-body tool has been walking
                    # their initialisers as if they were code, with `104
                    # bodies` in section 1 where there are 92.
                    # `prune_unreachable.py` learned the same thing in round
                    # eight and the lesson stayed in its file.
                    # The *last* name before the opening paren of the
                    # signature, not the first: `__attribute__((noreturn)) void
                    # __exit_402E40(...)` gave `attribute__`, and any leading
                    # attribute or alignment specifier would do the same.
                    m = (None if buf.rstrip().endswith('=') else
                         re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^()]*\)\s*$',
                                   buf.rstrip().rstrip('{').rstrip()) or
                         re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', buf))
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


# `x = nullptr;` is not an alias, it is an initialisation, and treating it as
# one merges every object that was ever cleared into a single class.  Same for
# the other literals that look like identifiers.
LITERALS = {'nullptr', 'NULL', 'true', 'false', 'this'}


def file_scope(lines, fns):
    """Names declared outside every function body.

    A global has one storage location for the whole program, so unioning it
    with a local says those two names denote the same allocation everywhere --
    which is how one global passed to two unrelated functions merges their
    objects.  They are excluded, not resolved.
    """
    inside = [False] * len(lines)
    for a, b, _, _ in fns:
        for i in range(a, min(b + 1, len(lines))):
            inside[i] = True
    out = set(LITERALS)
    for i, l in enumerate(lines):
        if inside[i]:
            continue
        m = declaration(l.split('//')[0])
        if m:
            for p in split_commas(m.group(2)):
                mm = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*$',
                               p.strip())
                if mm:
                    out.add(mm.group(1))
    return out


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
    outer = file_scope(lines, fns)
    PLAIN = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = ([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CAST = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = '
                      r'\(?(?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\)?\s*'
                      r'([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CALL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^();]*)\)')
    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            c = l.split('//')[0]
            m = PLAIN.match(c) or CAST.match(c)
            if m and not ({m.group(1), m.group(2)} & outer):
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
                    if (am and i < len(params[callee]) and params[callee][i]
                            and am.group(1) not in outer):
                        uf.union((fn, am.group(1)), (callee, params[callee][i]))
    return fns, uf


def decl_types(sig, lines, a, b):
    """Every name in a function, mapped to the type it was declared with.

    This is not decoration.  `p + 3` steps three bytes when p is `char *` and
    twelve when it is `void **`, so the same `+ 3` in the decompiled source
    means a different offset depending on a declaration that may be two hundred
    lines away.  Reading offsets as bytes regardless -- which is what this tool
    did at first -- silently mislocates every field of an object held in a
    typed pointer, and `free(*(Blocka + 269560))` becomes a free of the wrong
    address that still compiles.
    """
    types = {}
    m = re.search(r'\((.*)\)', sig)
    if m:
        for p in split_commas(m.group(1)):
            mm = re.match(r'\s*((?:const\s+)?[A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*)'
                          r'\s*([A-Za-z_][A-Za-z0-9_]*)\s*$', p)
            if mm:
                types[mm.group(2)] = re.sub(r'\s+', '', mm.group(1))
    i = a
    while i <= b:
        if not starts_declaration(lines[i]):
            i += 1
            continue
        j = i
        while ';' not in lines[j] and j < b:
            j += 1
        m = declaration(' '.join(l.strip() for l in lines[i:j + 1]))
        if m:
            base = m.group(1).strip()
            for p in split_commas(m.group(2)):
                nm = re.search(r'([A-Za-z_][A-Za-z0-9_]*)', p)
                if nm and nm.group(1) not in types:
                    types[nm.group(1)] = base + '*' * (p.count('*')
                                                       + p.count('['))
        i = j + 1
    return types


def code_start(lines, a, b):
    """The first line of a body that is a statement rather than a declaration.

    `int32_t *v4, *v5;` has the characters of a dereference in it and must not
    be read as one, in either direction: the survey would record a phantom
    field at offset zero, and the rewrite would turn the declaration into
    `int32_t v5->f0`.
    """
    end = a + 1
    for i in range(a + 1, b + 1):
        if starts_declaration(lines[i]):
            j = i
            while ';' not in lines[j] and j < b:
                j += 1
            end = j + 1
    return end


# `*p`, with nothing to say what it reads -- the type comes from p's own
# declaration.  This is how an object's first field usually appears.
PLAIN_DEREF = re.compile(r'\*\s*([A-Za-z_][A-Za-z0-9_]*)\b(?![\w\[(.]|->)')


def is_deref(text, pos):
    """Whether the `*` at pos dereferences rather than multiplies.

    `16 * v5` and `*v5` differ only in what comes before the star, so the
    decision has to look there: an operand ends a value, and a value times a
    pointer is a multiplication.  `return *v5` is the exception that makes the
    plain character test insufficient.
    """
    j = pos - 1
    while j >= 0 and text[j] in ' \t\n':
        j -= 1
    if j < 0:
        return True
    if text[j] in ')]':
        return False
    if text[j].isalnum() or text[j] == '_':
        k = j
        while k >= 0 and (text[k].isalnum() or text[k] == '_'):
            k -= 1
        return text[k + 1:j + 1] in ('return', 'case')
    return True


def elem_size(ty):
    """How many bytes `p + 1` steps for a name of this type."""
    if not ty or not ty.endswith('*'):
        return 1                              # an integer holding an address
    inner = ty[:-1].strip()
    if inner in ('void', 'constvoid', 'const void'):
        return 1                              # gcc's void * arithmetic
    return width(inner) or 1


def pointee(ty):
    return ty[:-1].strip() if ty and ty.endswith('*') else 'char'


def bind(repl, after):
    """Parenthesise a replacement a following operator would bind too tightly.

    `this_3[6]++` becomes `*(int32_t *)&this_3->f24` plus the `++`, and C++
    reads that as `&(this_3->f24++)` -- postfix binds tighter than the unary
    `&`, so the address is taken of something that is no longer an lvalue.
    """
    if repl.startswith('*') and (after[:2] in ('++', '--', '->')
                                 or after[:1] in '[.'):
        return '(%s)' % repl
    return repl


# a dereference whose operand has no parentheses left in it, so the terms of
# the address are plainly visible.  Rewriting these innermost-first exposes the
# ones that wrapped them.
INNER = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                   r'\(([^()]*)\)')
BARE = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                  r'(?:\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)|([A-Za-z_][A-Za-z0-9_]*))'
                  r'(?![\w\[])')
SUBSCRIPT = re.compile(r'(?<![\w.>\]])([A-Za-z_][A-Za-z0-9_]*)\s*\[')


def subscripts(text, start=0):
    """Every `name[...]` in the text, with brackets matched.

    An index can hold a subscript of its own -- `this_3[12 * k + 6 *
    (v39[n + 27] & v39[n + 19])]` -- and a pattern that stops at the first `]`
    never sees the outer one at all.  Left unrewritten, that outer subscript
    steps by the whole struct once the name becomes a struct pointer.
    """
    for m in SUBSCRIPT.finditer(text, start):
        depth, i = 0, m.end() - 1
        while i < len(text):
            if text[i] == '[':
                depth += 1
            elif text[i] == ']':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        if i < len(text):
            yield m.start(), i + 1, m.group(1), text[m.end():i]
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
    for a, b, fn, sig in fns:
        types = decl_types(sig, lines, a, b)
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
                    step = elem_size(types.get(got[0]))
                    fld[k][off * step][m.group(1).strip()] += 1
                    hits[k] += 1
            c = c[:m.start()] + '_' + c[m.end():]
        c = '\n'.join(l.split('//')[0] for l in lines[a:b + 1])
        for m in BARE.finditer(c):
            k = uf.find((fn, m.group(2) or m.group(3)))
            fld[k][0][m.group(1).strip()] += 1
            hits[k] += 1
        for _, _, nm, idx in subscripts(c):
            ty = types.get(nm)
            k = uf.find((fn, nm))
            off = constant(terms(idx))
            if off is None:
                var[k] += 1
            else:
                fld[k][off * elem_size(ty)][pointee(ty)] += 1
                hits[k] += 1
        for m in PTRFIELD.finditer(c):
            ptr[uf.find((fn, m.group(1)))].add(int(m.group(2)))
        stmts = '\n'.join(l.split('//')[0]
                          for l in lines[code_start(lines, a, b):b + 1])
        for m in PLAIN_DEREF.finditer(stmts):
            ty = types.get(m.group(1))
            if ty and ty.endswith('*') and is_deref(stmts, m.start()):
                k = uf.find((fn, m.group(1)))
                fld[k][0][pointee(ty)] += 1
                hits[k] += 1
    return fld, var, ptr, hits


# A declaration is any type name followed only by declarators and a semicolon.
# Naming the types instead -- which this did at first -- misses every local
# held in a `bool`, an `__m128`, a `BmfArc` or one of the structs recovered
# earlier, and a name whose declaration is not found is a name that cannot be
# retyped.
TYPE = (r'(?:const\s+)?(?:unsigned\s+|signed\s+)?'
        r'(?:struct\s+)?[A-Za-z_][A-Za-z0-9_]*')
NOT_A_TYPE = {'return', 'goto', 'break', 'continue', 'else', 'do', 'case',
              'default', 'delete', 'typedef'}
DECL = re.compile(r'^\s*(?:static\s+|extern\s+)?(%s)((?:[\s\*]+[A-Za-z_][A-Za-z0-9_]*'
                  r'(?:\[[^\]]*\])?\s*,)*[\s\*]+[A-Za-z_][A-Za-z0-9_]*'
                  r'(?:\[[^\]]*\])?)\s*;' % TYPE)


DECL_HEAD = re.compile(r'^\s*(?:static\s+|extern\s+)?(%s)[\s\*]+[A-Za-z_]' % TYPE)


def declaration(text):
    """The DECL match for a declaration statement, or None."""
    m = DECL.match(text)
    if m and m.group(1).split()[-1] not in NOT_A_TYPE:
        return m
    return None


# `__m128 *&v275 = __frame.v275;` -- a name that is a reference into one of the
# frame structures, not a variable of its own.
ALIAS = re.compile(r'^\s*(?:const\s+)?[A-Za-z_][A-Za-z0-9_]*[\s\*]*&\s*'
                   r'([A-Za-z_][A-Za-z0-9_]*)\s*=')


def starts_declaration(line):
    """Whether a line opens a declaration -- which may not close on it.

    `int32_t *v4, *v5, v6,` continues onto the next line and has no semicolon,
    so testing the whole pattern against the first line alone finds nothing and
    every name in a wrapped list looks undeclared.
    """
    if ALIAS.match(line):
        return True
    m = DECL_HEAD.match(line)
    return bool(m) and m.group(1).split()[-1] not in NOT_A_TYPE


def frame_bound(lines, s, b, nm):
    """Lines where nm is a reference into a frame structure.

    Those frames were laid out to match what the code overruns between named
    members, and each carries a static_assert on its size saying so.  A name
    that is one of their members cannot be given a different type without
    moving that layout, so an object holding one is declined rather than
    quietly relaid.
    """
    return [i for i in range(s, b + 1)
            if (lambda m: m and m.group(1) == nm)(ALIAS.match(lines[i]))]


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
    for i in range(a, b + 1):
        m = ALIAS.match(out[i])
        if m and m.group(1) == nm and out[i].rstrip().endswith(';'):
            # a reference into a frame.  Retype the reference, not the member:
            # the frame keeps its layout, and at 32 bits both are four bytes,
            # so the reference still names the same storage.
            rhs = out[i].split('=', 1)[1].rsplit(';', 1)[0].strip()
            ind = re.match(r'^(\s*)', out[i]).group(1)
            out[i] = '%s%s *&%s = (%s *&)%s;' % (ind, tag, nm, tag, rhs)
            return 0
    i = a
    while i <= b:
        if not starts_declaration(out[i]):
            i += 1
            continue
        j = i
        while ';' not in out[j] and j < b:
            j += 1
        text = ' '.join(l.strip() for l in out[i:j + 1])
        m = declaration(text)
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
    pat = re.compile(r'(?:\+\+|--)\s*%s\b(?![\[.]|->)'
                     r'|(?<![\w.>])%s\s*(?:\+\+|--|\+=|-=)'
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
            # returning quietly here left the sweep applying nothing, gating an
            # unchanged file, calling it a pass, and picking the same object
            # again on the next round.  Decline it properly.
            with open(SKIP, 'a') as f:
                f.write(signature(uf, key) + '\n')
            print('%s: declined -- cannot size %s at +%d' % (tag, best, off))
            sys.exit(3)
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

    named = [o for o in sorted(member) if member[o]]
    if len(named) >= 4:
        tys = {member[o][1] for o in named}
        steps = {named[i + 1] - named[i] for i in range(len(named) - 1)}
        if len(tys) == 1 and len(steps) == 1:
            # One type, evenly spaced, with a gap between each: an array with a
            # stride, not a record.  Naming those `f2 f10 f18` hides the stride,
            # which is the only thing about the object worth seeing -- and the
            # same object is elsewhere subscripted with a computed index, which
            # the struct form cannot spell at all.  tools/unstruct.py converted
            # eleven of these back after they had been applied; this is so
            # there is no twelfth.
            with open(SKIP, 'a') as f:
                f.write(signature(uf, key) + '\n')
            print('%s: declined -- %d x %s at a stride of %d is an array'
                  % (tag, len(named), list(tys)[0], list(steps)[0]))
            sys.exit(3)

    if not any(member.values()):
        # Every offset landed inside the one before it.  That happens when the
        # first access at +0 is four bytes wide and the rest read +1, +2 and +3
        # out of it: the object is one word being taken apart, not a struct with
        # fields, and there is no layout to name.  Declined for the same reason
        # and by the same route as an unsizeable member -- quietly returning
        # here is what once left the sweep gating an unchanged file and calling
        # it a pass.
        with open(SKIP, 'a') as f:
            f.write(signature(uf, key) + '\n')
        print('%s: declined -- all %d offsets fall inside the member at +%d'
              % (tag, len(offs), min(offs)))
        sys.exit(3)

    # the guard is the last member's offset, not the struct's size: a struct
    # whose members end at 90 is 92 bytes once it is aligned, and asserting the
    # size would be asserting the padding rule rather than the layout.  Any
    # member that moves or changes width moves this one.
    last = max(o for o in member if member[o])
    body = ('// %s -- recovered from %d dereferences over %d offsets, under %d\n'
            '// name%s.  The layout is the one the code already assumed: at 32 bits a\n'
            '// pointer is four bytes, so naming these fields moves nothing, and the\n'
            '// static_assert is what says so.  Offsets the code only reaches with a\n'
            '// computed index are padding here -- their bounds are not visible.\n'
            'struct %s {\n%s\n};\n'
            'static_assert(sizeof(void *) != 4\n'
            '              || __builtin_offsetof(%s, %s) == %d,\n'
            '              "%s: the layout moved");\n'
            % (tag, hits[key], len(offs), len({nm for _, nm in names}),
               '' if len({nm for _, nm in names}) == 1 else 's',
               tag, '\n'.join(decl), tag, member[last][0], last, tag))

    if '--show' in sys.argv:
        print(body)
        return

    byfn = collections.defaultdict(set)
    for fn, nm in names:
        byfn[fn].add(nm)
    spans = {fn: (a, b) for a, b, fn, _ in fns}
    sigof = {fn: sig for _, _, fn, sig in fns}

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
            for why, bad in (('is stepped as a pointer', rescales(out, s, b, nm)),):
                if bad:
                    with open(SKIP, 'a') as f:
                        f.write(signature(uf, key) + '\n')
                    print('%s: declined -- %s %s at line %d'
                          % (tag, nm, why, bad[0] + 1))
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
        # only the statements: a declaration holds the characters of a
        # dereference and of an assignment without being either, and rewriting
        # one turns `__m128 *&v275 = __frame.v275;` into something that is not
        # a declaration at all
        code = code_start(out, a, b)
        keep = out[s:code]
        text = '\n'.join(out[code:b + 1])
        types = decl_types(sigof[fn], out, a, b)
        step = {nm: elem_size(types.get(nm)) for nm in nms}
        rm = re.match(r'^\s*(?:BMF_SSE\s+|static\s+|inline\s+)*'
                      r'((?:const\s+)?[A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*)\s*'
                      r'[A-Za-z_][A-Za-z0-9_]*\s*\(', sigof[fn])
        ret = re.sub(r'\s+', ' ', rm.group(1)).strip() if rm else None
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
                    scale = '' if step[nm] == 1 else ' * %d' % step[nm]
                    new = '*(%s *)((char *)%s + (intptr_t)(%s)%s)' % (
                        m.group(1).strip(), nm, join(rest), scale)
                else:
                    new = field(m.group(1).strip(), nm, off * step[nm])
                if new is not None:
                    new = bind(new, text[m.end():m.end() + 2])
            if new is None:
                pos = m.start() + 2
                continue
            text = text[:m.start()] + new + text[m.end():]
            pos = 0                           # the outer dereference may match now
        for nm in nms:
            ty, k = types.get(nm, 'char*'), step[nm]

            # nm[i] -- invisible to the dereference patterns, and scaled by the
            # declared element, not by the byte
            at = 0
            while True:     # one at a time: a replacement moves what follows
                for lo, hi, got, idx in subscripts(text, at):
                    if got != nm:
                        continue
                    off = constant(terms(idx))
                    new = None
                    if off is not None:
                        new = field(pointee(ty), nm, off * k)
                        if new:
                            new = bind(new, text[hi:hi + 2])
                    if new is None:
                        new = '((%s *)%s)[%s]' % (pointee(ty), nm, idx)
                    text = text[:lo] + new + text[hi:]
                    at = lo
                    break
                else:
                    break

            # a plain `*nm`, with no cast to say what it reads
            def deref_rep(m, nm=nm, ty=ty):
                if m.group(1) != nm or not is_deref(m.string, m.start()):
                    return m.group(0)
                got = field(pointee(ty), nm, 0)
                return bind(got, m.string[m.end():m.end() + 2]) if got \
                    else m.group(0)
            text = PLAIN_DEREF.sub(deref_rep, text)

            # *(T *)nm -> nm->f0
            if member.get(0):
                mem, mty = member[0]
                text = re.sub(r'\*\((?:const )?%s\s*\*\)'
                              r'(?:\(\s*%s\s*\)|%s)(?![\w\[.]|->)'
                              % (re.escape(mty), re.escape(nm), re.escape(nm)),
                              '%s->%s' % (nm, mem), text)
            # whatever arithmetic is left keeps the type it was written for --
            # `(char *)` is only right for a name that already stepped by bytes
            # a name declared as an integer was doing integer arithmetic, and
            # some of it is masked -- `(v56 + 278543) & 0xFFFFFFF0` aligns an
            # address.  `&` has no meaning on a pointer, so an integer stays an
            # integer; a pointer keeps the type it stepped by.
            cast = ('(uintptr_t)' if not ty.endswith('*')
                    else '(char *)' if k == 1 else '(%s)' % ty.replace('*', ' *'))
            text = re.sub(r'(?<![\)>\w.&])%s\b'
                          r'(?=\s*(?:\+(?!\+)|-(?![->])))' % re.escape(nm),
                          cast + nm, text)

            # `return Blocka_1;` from a function that returns something else
            if ret and ret != '%s *' % tag:
                text = re.sub(r'\breturn\s+%s\s*;' % re.escape(nm),
                              'return (%s)%s;' % (ret, nm), text)

            # `v5 = malloc(...)` was int-to-int or pointer-to-same-pointer
            # before; now the left side is a struct pointer and C++ will not
            # convert silently.  The cast wraps the whole right-hand side, so a
            # conditional keeps its meaning.
            text = re.sub(r'\b%s(\s*)=\s*(?!=)((?:[^;=]|=[^=])*?);' % re.escape(nm),
                          lambda m: '%s%s= (%s *)(%s);'
                          % (nm, m.group(1), tag, m.group(2).strip()), text)
        new_lines = keep + text.split('\n')
        assert len(new_lines) == b + 1 - s, 'rewrite changed the line count'
        changed += sum(1 for x, y in zip(out[s:b + 1], new_lines) if x != y)
        out[s:b + 1] = new_lines

    # retype the declarations, last, so the spans above are still valid
    retyped = collections.defaultdict(set)
    for fn, nms in sorted(byfn.items(), key=lambda kv: -spanof.get(kv[0], (0,))[0]):
        if fn not in spanof:
            continue
        s, a, b = spanof[fn]
        for nm in nms:
            for i in range(s, a + 1):
                before = out[i]
                out[i] = retype_param(out[i], nm, tag)
                if out[i] != before:
                    ps = params_of(sigof[fn])
                    if nm in ps:
                        retyped[fn].add(ps.index(nm))
            if any(re.search(r'\b%s\s*\*+\s*%s\b' % (tag, re.escape(nm)), l)
                   for l in out[s:a + 1]):
                continue
            # `int32_t &v61 = Blocka_5;` binds a reference to the same
            # storage under another type.  The reference keeps its type; what
            # it binds to has to be spelled as that type.  This is in the
            # declarations, which the rewrite above deliberately does not
            # touch, so it belongs here.
            for i in range(a, b + 1):
                out[i] = re.sub(r'^(\s*(?:const\s+)?([A-Za-z_][A-Za-z0-9_]*)'
                                r'\s*\*?\s*&\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*)'
                                r'%s\s*;' % re.escape(nm),
                                lambda m: '%s(%s &)%s;'
                                % (m.group(1), m.group(2), nm), out[i])
            delta = retype_local(out, a, b, nm, tag)
            if delta is None:
                # a name left at its old type would take `nm->f8` with it, and
                # the result would not compile.  Decline the whole object
                # rather than hand the gate a file that cannot build.
                with open(SKIP, 'a') as f:
                    f.write(signature(uf, key) + '\n')
                print('%s: declined -- no declaration found for %s in %s'
                      % (tag, nm, fn))
                sys.exit(3)
            b += delta

    # the one-line forwarding shims carry the cast the old parameter needed.
    # C++ converts one pointer type to another for nobody, -fpermissive
    # included, so a retyped parameter has to be matched at every call the
    # shims make.
    for fn, idxs in retyped.items():
        call = re.compile(r'\b%s\s*\(([^;]*)\)\s*;?\s*\}' % re.escape(fn))
        for i, l in enumerate(out):
            if not l.lstrip().startswith('static inline'):
                continue
            m = call.search(l)
            if not m:
                continue
            args = split_commas(m.group(1))
            for k in idxs:
                if k < len(args):
                    bare = re.sub(r'^\s*\([^)]*\)\s*', '', args[k]).strip()
                    args[k] = ' (%s *)%s' % (tag, bare)
            out[i] = l[:m.start(1)] + ','.join(args).lstrip() + l[m.end(1):]

    anchor = next(i for i, l in enumerate(out) if l.startswith('struct RangeCoder'))
    out = out[:anchor] + body.split('\n') + [''] + out[anchor:]
    open(path, 'w').write('\n'.join(out))
    print('%s: %d lines rewritten across %d functions' % (tag, changed, len(byfn)))


if __name__ == '__main__':
    main()
