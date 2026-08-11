#!/usr/bin/env python3
"""Turn a function whose first parameter is `_this` into a method of its type.

    python3 tools/methodise.py subs1.hpp                 # what it can do
    python3 tools/methodise.py subs1.hpp --only rescale_three_way
    python3 tools/methodise.py subs1.hpp --only rescale_three_way --all

    int32_t __rescale_three_way(P2Freq *_this)          →  struct P2Freq {
    { … _this->w[0] … }                                        int32_t rescale();
    __rescale_three_way(rec);                                }
                                                          rec->rescale();

MSVC's `__thiscall` puts the object in `ecx` and Hex-Rays writes it back as a
first parameter called `_this`.  Twenty bodies in this file carry one, and for
nineteen of them the type is a record this project has already recovered -- so
the parameter is not a parameter, it is the receiver, and saying so moves every
call site from `__f(x, …)` to `x->f(…)`.

The twentieth is `update_binary_pair`, whose `_this` is a `uint16_t *` into a
counter block rather than a record.  It stays a function, which is the honest
answer: there is no type for it to be a method of.

What the rewrite does, and does not do:

  * the body's `_this` becomes `this`.  Not `this->x` to `x`: a body that
    declares a local with a member's name would silently change meaning, and
    the second step below is where that gets decided one name at a time;
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


def candidates(lines):
    """(name, record type, signature line, body span, return type, rest of params)."""
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        m = re.match(r'\s*(.+?)\s+%s\s*\((\s*([A-Za-z_]\w*)\s*\*\s*_this\s*'
                     r'(?:,\s*(.*))?)\)' % re.escape(nm), sig, re.S)
        if not m:
            continue
        ret, rec, rest = m.group(1).strip(), m.group(3), (m.group(4) or '').strip()
        if rec in ('uint8_t', 'uint16_t', 'uint32_t', 'int8_t', 'int16_t',
                   'int32_t', 'void', 'char', 'float', 'double'):
            continue                      # a raw pointer, not a record
        out.append((nm, rec, ret, rest, a, b))
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


def apply(lines, nm, rec, ret, rest, a, b):
    """Rewrite one body, its declaration and its call sites.  In place."""
    short = nm.lstrip('_')
    close = struct_end(lines, rec)
    if close is None:
        return False, '%s: no struct %s' % (short, rec)
    # The signature can wrap; find where it starts.
    s = a
    while s > 0 and nm not in lines[s]:
        s -= 1
    while s > 0 and not lines[s - 1].rstrip().endswith(('}', ';', '/', '')):
        s -= 1
    head = s
    while head > 0 and nm not in lines[head]:
        head -= 1

    # 1. the body: `_this` becomes `this`
    for i in range(head, b + 1):
        lines[i] = re.sub(r'(?<![\w>])_this\b', 'this', lines[i])

    # 2. the definition's own head, however many lines it took
    j = head
    while j <= b and '{' not in lines[j]:
        j += 1
    sig = ' '.join(l.strip() for l in lines[head:j + 1])
    sig = sig.split('{')[0].strip()
    m = re.match(r'(.+?)\s+%s\s*\((.*)\)\s*$' % re.escape(nm), sig, re.S)
    if not m:
        return False, '%s: cannot re-read its own signature' % short
    params = m.group(2)
    params = re.sub(r'^\s*[A-Za-z_]\w*\s*\*\s*this\s*,?\s*', '', params).strip()
    lines[head:j + 1] = ['inline %s %s::%s(%s)' % (ret, rec, short, params),
                         '{']

    # 3. the declaration, just inside the struct
    lines.insert(close, '  %s %s(%s);' % (ret, short, params))

    # 4. every call site.  On the whole text, not line by line: a call whose
    # arguments wrap -- `__f(\n  &x->grid[...],\n  y)` -- has its parentheses
    # split across lines, and a per-line scan leaves it alone while the
    # function it names has just stopped existing.  One such call in
    # `decode_pixel` is what said so.
    text = '\n'.join(lines)
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
        tail = (args[cut + 1:].strip() if cut is not None else '')
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
        out += text[pos:mm.start()] + '%s%s%s(%s)' % (obj, arrow, short, tail)
        pos = k
        hits += 1
    lines[:] = out.split('\n')
    return True, '%s -> %s::%s, %d call sites' % (short, rec, short, hits)


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)
    only = None
    if '--only' in sys.argv:
        only = sys.argv[sys.argv.index('--only') + 1]
        found = [f for f in found if f[0].lstrip('_') == only]
    if '--all' in sys.argv:
        if len(found) != 1:
            sys.exit('--all takes one --only NAME at a time; the call-site '
                     'rewrite renumbers the file under the next one')
        ok, why = apply(lines, *found[0])
        if not ok:
            sys.exit(why)
        open(path, 'w').write('\n'.join(lines))
        print(why)
    else:
        for nm, rec, ret, rest, a, b in found:
            print('  %-26s %-14s %4d lines' % (nm.lstrip('_'), rec + ' *', b - a))
        print('%d functions whose `_this` is a record this file has a type for'
              % len(found))
