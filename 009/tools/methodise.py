#!/usr/bin/env python3
"""Turn a function whose first parameter is its receiver into a method.

    python3 tools/methodise.py bmf.cpp                   # what it can do
    python3 tools/methodise.py bmf.cpp --only rescale_three_way
    python3 tools/methodise.py bmf.cpp --only rescale_three_way --all

    int32_t __rescale_three_way(P2Freq *_this)          →  struct P2Freq {
    { … _this->w[0] … }                                        int32_t rescale();
    __rescale_three_way(rec);                                }
                                                          rec->rescale();

MSVC's `__thiscall` puts the object in `ecx` and Hex-Rays writes it back as a
first parameter called `_this`.  Where the type is a record this project has
recovered, the parameter is not a parameter -- it is the receiver, and saying so
moves every call site from `__f(x, …)` to `x->f(…)`.

**The parameter's name is not the rule.**  It was, and the rule read zero for a
whole round while `__alt_p2_context(AltP2Block *blk, …)` sat there with 168
`blk->` in it: the decompiler writes `_this` only where MSVC used `__thiscall`,
and a receiver passed on the stack looks like any other pointer.  What makes a
parameter a receiver is what the body does with it, so the test is that one:

  * the first parameter is a pointer to a struct this translation unit
    declares, and
  * **every** use of it in the body is a member access, `p->`.

The second half is what keeps the rule honest.  `__write_bmp(BmfImage *img, …)`
has 23 `img->` and four uses that are not -- it passes `img` on and does
arithmetic with it -- so it is a function that takes an image, not a method of
one, and this declines it.  `MINIMAL-SYNTAX.md` §3 is where that widening was
asked for; the two bodies it was asked for were done by hand before the tool
could, and the tool is what answers for the rest.

Two bodies still carry a `_this` and both are honest declines, because there
is no type for either to be a method of: `update_binary_pair` takes a
`uint16_t *` into a counter block, and `alt_p2_filter` takes a `float (*)[4]`
-- a weight block, which wants a name rather than a receiver.

The unit is 37 files, so a candidate's body, its struct and its call sites are
usually in three different ones.  Analysis runs over the unit as the compiler
sees it (`structs.splice`) and the rewrite writes back to whichever files hold
the parts.  Reading one file at a time is what made this answer zero from the
other direction: `bmf.cpp` alone is an include list with no bodies in it at all.

What the rewrite does, and does not do:

  * the body's receiver -- `_this`, `blk`, `list`, whatever it was called --
    becomes `this`.  Not `this->x` to `x`: a body that declares a local with a
    member's name would silently change meaning, and that is decided one name
    at a time afterwards, with `-Wshadow` naming them;
  * the declaration goes inside the struct and the definition stays where the
    body is, as `inline R T::name(…)`.  The file is one translation unit, so
    `inline` is what keeps the linker quiet, and the body does not move --
    a diff that moved four thousand lines would hide whatever else it did;
  * call sites `__name(obj, rest)` become `obj->name(rest)`, and a call whose
    first argument is a cast keeps the cast: `__f((T *)p, x)` is
    `((T *)p)->f(x)`.

Nothing here is a judgement about behaviour -- it is the same code with the
receiver named -- so the gate is what says it worked.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402


# Bodies the rule above accepts and a reader should not.  The test is
# syntactic -- "every use of the first parameter is a member access" -- and it
# cannot see that a 343-line encoder is not a thing an image *does*.  Listed
# with the reason, and counted apart, so the next round does not re-propose
# them and does not have to re-derive why.
DECLINED = {
    'alt_model_p2_encode':
        'reads two fields of the image and is a 343-line encoder; a method of '
        '`BmfImage` would make the image the compressor.  Its decode half takes '
        'a `uint16_t *` and could not be the same kind of method, so the pair '
        'would stop matching as well',
}


def records(lines):
    """The struct names this unit defines a body for."""
    return set(re.findall(r'^\s*struct\s+(\w+)\s*\{', '\n'.join(lines), re.M))


def receiver(lines, a, b, p):
    """Is `p` used only as `p->`, and at least once?

    The signature is not part of the body for this question -- the parameter's
    own declaration is a use of the name that is not a member access, and
    counting it declines every candidate there is.
    """
    body = '\n'.join([lines[a].split('{', 1)[-1]] + lines[a + 1:b + 1])
    uses = re.findall(r'(?<![\w.>])%s\b\s*(-[>]?)?' % re.escape(p), body)
    return bool(uses) and all(u == '->' for u in uses)


def candidates(lines):
    """(name, record type, signature line, body span, return type, rest of params)."""
    out = []
    known = records(lines)
    for a, b, nm, sig, d in structs.defs(lines):
        if d:
            continue                      # already a method
        if 'template' in sig or 'static inline' in sig:
            continue
        # `(.+?[\s*])` and not `(.+?)\s+`: a body that returns a pointer is
        # written `T *__name(`, so the character before the name is a star and
        # not a space, and the whitespace form silently made it a non-candidate.
        # Three of the five bodies with a `_this` parameter are declared that
        # way -- `alt_p1_alloc`, `alt_p2_alloc`, `bmf_close_archive` -- which is
        # 133 of the file's 172 `_this` occurrences behind one character.
        #
        # The control is the whole classification and not one body: the
        # candidate set must grow by exactly those three and change in no other
        # way, and the two correct declines below (`update_binary_pair`, a
        # `uint16_t *`, and `alt_p2_filter`, a `float (*)[4]`) must still
        # decline.  The obvious control was not available -- this file's own
        # docstring example, `__rescale_three_way`, was methodised rounds ago
        # and has no `_this` left to match, which is the same trap one level up.
        #
        # The separator is captured *inside* group 1 on purpose.  `(.+?)[\s*]`
        # matches the same signatures and throws the star away, so
        # `int32_t *__alt_p1_alloc` reports its return type as `int32_t` and the
        # method would be declared returning a value where the body returns a
        # pointer.  It compiles -- `-fpermissive` was made for that -- and the
        # first sign would have been the gate.
        m = re.match(r'\s*(.+?[\s*])%s\s*\((\s*(?:const\s+)?([A-Za-z_]\w*)\s*\*\s*'
                     r'([A-Za-z_]\w*)\s*(?:,\s*(.*))?)\)' % re.escape(nm),
                     sig, re.S)
        if not m:
            continue
        ret, rec = m.group(1).strip(), m.group(3)
        p, rest = m.group(4), (m.group(5) or '').strip()
        if rec not in known:
            continue                      # a raw pointer, not a record
        if not receiver(lines, a, b, p):
            continue                      # a function that takes one, not a method
        out.append((nm, rec, ret, rest, a, b, p))
    return out


def struct_end(lines, rec):
    """The line index of the `};` that closes `struct rec`."""
    depth, inside = 0, False
    for i, l in enumerate(lines):
        c = l.split('//')[0]
        if not inside and re.match(r'\s*struct\s+%s\s*\{' % re.escape(rec), c):
            inside, depth = True, c.count('{') - c.count('}')
            continue
        if inside:
            depth += c.count('{') - c.count('}')
            if depth <= 0:
                return i
    return None


def designator(e):
    """A name with any number of subscripts and members after it, and nothing
    else -- `p`, `p->q[3].r`, `blk->freq[blk->ctx]`.  Whitespace and newlines
    are allowed inside the brackets, which is where the wrapped calls put them.
    """
    e = e.strip()
    if not re.match(r'[A-Za-z_]\w*', e):
        return False
    i, depth = 0, 0
    while i < len(e):
        ch = e[i]
        if ch in '[(':
            depth += 1
        elif ch in '])':
            depth -= 1
            if depth < 0:
                return False
        elif depth == 0 and not (ch.isalnum() or ch in '_.>-' or ch.isspace()):
            return False
        elif depth == 0 and ch == '-' and e[i:i + 2] != '->':
            return False
        i += 1
    return depth == 0


def rewrite_calls(text, nm, short):
    """`__name(obj, rest)` -> `obj->name(rest)`, everywhere in `text`.

    On the whole text, not line by line: a call whose arguments wrap --
    `__f(\n  &x->grid[...],\n  y)` -- has its parentheses split across lines,
    and a per-line scan leaves it alone while the function it names has just
    stopped existing.  One such call in `decode_pixel` is what said so.
    """
    call = re.compile(r'(?<![\w])%s\s*\(' % re.escape(nm))
    out, pos, hits = '', 0, 0
    while True:
        mm = call.search(text, pos)
        if not mm:
            out += text[pos:]
            break
        k, depth = mm.end(), 1
        while k < len(text) and depth:
            depth += (text[k] == '(') - (text[k] == ')')
            k += 1
        if depth:
            out += text[pos:mm.end()]
            pos = mm.end()
            continue
        args, depth, cut = text[mm.end():k - 1], 0, None
        for x, ch in enumerate(args):
            depth += (ch in '([') - (ch in ')]')
            if ch == ',' and depth == 0:
                cut = x
                break
        obj = (args[:cut] if cut is not None else args).strip()
        rest = (args[cut + 1:].strip() if cut is not None else '')
        if not obj:
            out += text[pos:k]
            pos = k
            continue
        # `__f(&rec[1], x)` is `rec[1].f(x)`, not `(&rec[1])->f(x)`.  Taking an
        # address only to follow it is the shape this rewrite removes, not one
        # it should introduce.  Only when what follows the `&` is a plain
        # designator -- a name and any number of subscripts and members -- so
        # that `&a + b` is not read as one.
        arrow = '->'
        if obj.startswith('&') and designator(obj[1:]):
            obj, arrow = obj[1:].strip(), '.'
        elif not designator(obj):
            obj = '(%s)' % obj
        out += text[pos:mm.start()] + '%s%s%s(%s)' % (obj, arrow, short, rest)
        pos = k
        hits += 1
    return out, hits


def rewrite_definition(lines, nm, rec, ret, a, b, p):
    """Steps 1 and 2, in the file that holds the body.  In place.

    Returns (parameter list without the receiver, message).  The parameter list
    is what the declaration in the struct has to agree with, so it comes back
    rather than being derived twice.
    """
    short = nm.lstrip('_')
    # The signature can wrap; find where it starts.
    s = a
    while s > 0 and nm not in lines[s]:
        s -= 1
    while s > 0 and not lines[s - 1].rstrip().endswith(('}', ';', '/', '')):
        s -= 1
    head = s
    while head > 0 and nm not in lines[head]:
        head -= 1

    # 1. the body: the receiver's name becomes `this`.  Not `this->x` to `x`:
    # a body that declares a local with a member's name would silently change
    # meaning, and that is decided one name at a time elsewhere.
    for i in range(head, b + 1):
        lines[i] = re.sub(r'(?<![\w>])%s\b' % re.escape(p), 'this', lines[i])

    # 2. the definition's own head, however many lines it took
    j = head
    while j <= b and '{' not in lines[j]:
        j += 1
    sig = ' '.join(l.strip() for l in lines[head:j + 1])
    sig = sig.split('{')[0].strip()
    # `[\s*]`, for the same reason as `candidates()` above -- and this one is
    # why the fix there was not enough on its own.  With only the first regex
    # widened, the three pointer-returning bodies became candidates and then
    # every one of them failed here with "cannot re-read its own signature",
    # which is the rule declining out loud rather than writing something wrong.
    # The return type is not taken from this match; `candidates()` already has
    # it, and all this needs is the parameter list.
    m = re.match(r'(.+?)[\s*]%s\s*\((.*)\)\s*$' % re.escape(nm), sig, re.S)
    if not m:
        return None, '%s: cannot re-read its own signature' % short
    params = m.group(2)
    params = re.sub(r'^\s*(?:const\s+)?[A-Za-z_]\w*\s*\*\s*this\s*,?\s*',
                    '', params).strip()
    lines[head:j + 1] = ['inline %s %s::%s(%s)' % (ret, rec, short, params),
                         '{']
    return params, None


def unit_of(path):
    """{file: lines} for the translation unit, and the include order.

    `structs.splice` reads the unit as the compiler does, which is what the
    analysis needs; this is the same set of files kept apart, which is what
    writing back needs.
    """
    import os
    base = os.path.dirname(os.path.abspath(path))
    order, unit = [], {}
    for l in open(path).read().split('\n'):
        m = re.match(r'\s*#include "([^"]+)"', l)
        if m and os.path.exists(os.path.join(base, m.group(1))):
            order.append(m.group(1))
    order.append(os.path.basename(path))
    for f in order:
        unit[f] = open(os.path.join(base, f)).read().split('\n')
    return unit, order, base


def apply_tree(path, origin, nm, rec, ret, rest, a, b, p):
    """The same rewrite where the body, the struct and the callers are in
    different files -- which, in a unit of 37, they usually are."""
    import os
    short = nm.lstrip('_')
    unit, order, base = unit_of(path)
    src, first = origin[a]
    if origin[b][0] != src:
        return False, '%s: its body spans two files' % short
    # The declaration goes in whichever file defines the struct.
    home = next((f for f in order if struct_end(unit[f], rec) is not None), None)
    if home is None:
        return False, '%s: no struct %s' % (short, rec)

    params, why = rewrite_definition(unit[src], nm, rec, ret,
                                     first - 1, first - 1 + (b - a), p)
    if params is None:
        return False, why
    unit[home].insert(struct_end(unit[home], rec),
                      '  %s %s(%s);' % (ret, short, params))
    # The forward declaration in `decls.inc` would otherwise be rewritten as if
    # it were a call -- its first argument is `SymList *list`, which is a
    # declaration and not an object -- and `void (SymList *list)->init(…);` is
    # what that produces.  So prototypes go first, and a prototype is a line
    # that starts in column 0 and ends in `);`: every statement in this tree is
    # inside a body and therefore indented.
    dead = re.compile(r'^[A-Za-z_].*(?<![\w])%s\s*\(.*\);\s*$' % re.escape(nm))
    for f in order:
        unit[f] = [l for l in unit[f] if not dead.match(l)]
    hits = 0
    for f in order:
        text, n = rewrite_calls('\n'.join(unit[f]), nm, short)
        unit[f] = text.split('\n')
        hits += n
    for f in order:
        open(os.path.join(base, f), 'w').write('\n'.join(unit[f]))
    return True, ('%s -> %s::%s, %d call sites (body in %s, declared in %s)'
                  % (short, rec, short, hits, src, home))


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'bmf.cpp'
    lines, origin = structs.splice(path)
    found = candidates(lines)
    only = None
    if '--only' in sys.argv:
        only = sys.argv[sys.argv.index('--only') + 1]
        found = [f for f in found if f[0].lstrip('_') == only]
    if '--all' in sys.argv:
        if len(found) != 1:
            sys.exit('--all takes one --only NAME at a time; the call-site '
                     'rewrite renumbers the file under the next one')
        ok, why = apply_tree(path, origin, *found[0])
        if not ok:
            sys.exit(why)
        print(why)
    else:
        held = [f for f in found if f[0].lstrip('_') in DECLINED]
        found = [f for f in found if f[0].lstrip('_') not in DECLINED]
        for nm, rec, ret, rest, a, b, p in found:
            print('  %-26s %-14s %4d lines  %s:%d'
                  % (nm.lstrip('_'), rec + ' *', b - a, origin[a][0], origin[a][1]))
        for nm, rec, ret, rest, a, b, p in held:
            print('  %-26s declined: %s'
                  % (nm.lstrip('_'), DECLINED[nm.lstrip('_')]))
        print('%d functions whose first parameter is only ever a receiver, '
              '%d declined' % (len(found), len(held)))
