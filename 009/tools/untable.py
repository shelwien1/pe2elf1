#!/usr/bin/env python3
"""Say a file-scope table once instead of three times.

    python3 tools/untable.py subs1.hpp --list
    python3 tools/untable.py subs1.hpp __dword_439880
    python3 tools/untable.py subs1.hpp --all

REFACTORING4.md §4.  Twenty tables arrived as a byte image, a typedef for the
real type, and a reference that reinterprets one as the other:

    alignas(16) static uint8_t bmf_dword_439880[16] = {   // 0x439880
      0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
    };
    typedef int32_t t_dword_439880[4];
    static t_dword_439880& __dword_439880 = *(t_dword_439880*)bmf_dword_439880;

which means

    alignas(16) static int32_t __dword_439880[4] = { 4, 8, 12, 8 };

The byte image was round one's, and it was right at the time: the globals still
lived at their addresses in BMF.exe and something might stride between them.
Round three ended that, so the reason for the indirection went with it.

**The bytes are what is verified**, so this decodes rather than retypes: it
reads the image little-endian at the typedef's element width and prints the
values back.  A table whose element type it cannot decode is left alone and
named.  `__m128i` tables are Phase A's -- they are single floats broadcast four
times and they leave with the parameter thread, not with this.

The extent is the buffer's, which is not always the table's:
`bmf_ctx_group_flags[32]` holds fifteen non-zero bytes and seventeen zeros
because 32 is the distance to the next 1997 global.  This keeps the buffer's
length so the bytes stay identical; deciding the real one is reading, not
rewriting.
"""
import re
import struct
import sys

TRIPLE = re.compile(
    r'^(alignas\(16\) )?static uint8_t (bmf_\w+)\[(\d+)\] = \{([^}]*)\};\n'
    r'typedef ([\w ]+?) (t_\w+)(?:\[(\d+)\])?;\n'
    r'static \6& (\w+) = \*\(\6\*\)\2;\n', re.M | re.S)

FMT = {'uint8_t': ('B', 1), 'int8_t': ('b', 1), 'char': ('b', 1),
       'uint16_t': ('<H', 2), 'int16_t': ('<h', 2),
       'uint32_t': ('<I', 4), 'int32_t': ('<i', 4),
       'uint64_t': ('<Q', 8), 'int64_t': ('<q', 8), 'float': ('<f', 4)}


def addr_taken(text, name):
    """`&name`, not `a & name` -- the mistake `defram.py` made first.

    `)` before the `&` is the ambiguous one: `(a) & b` is a bitwise and but
    `(uint8_t *)&x` is a cast of an address, and `__dword_439B7C` is only ever
    reached the second way.  A closing paren whose text ends in `*)` is a cast.
    """
    for m in re.finditer(r'&\s*' + re.escape(name) + r'\b', text):
        j = m.start() - 1
        while j >= 0 and text[j] == ' ':
            j -= 1
        if j < 0:
            return True
        cast = text[j] == ')' and re.search(r'\*\s*\)$', text[max(0, j - 30):j + 1])
        if text[j] == ')' and not cast:
            continue                        # `(a) & b`, a bitwise and
        if not cast and (text[j].isalnum() or text[j] in '_]&'):
            continue                        # `a & b`, likewise
        # An address-of.  Only a *walk* makes this a table: `&x` handed to a
        # function is an out-parameter -- `(void *)&__dwLowDateTime` -- and the
        # thing behind it is still one scalar.
        if re.match(r'\s*[)\s]*[-+\[]', text[m.end():m.end() + 12]):
            return True
    return False


def tables(src):
    out = []
    for m in TRIPLE.finditer(src):
        # the address comment lives inside the braces -- `= {   // 0x439860` --
        # and `0x43` reads as a byte if it is not stripped first
        init = re.sub(r'//[^\n]*', '', m.group(4))
        raw = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', init))
        out.append(dict(span=m.span(), buf=m.group(2), size=int(m.group(3)),
                        ty=m.group(5).strip(), count=m.group(7),
                        name=m.group(8), bytes=raw, align=bool(m.group(1)),
                        walked=addr_taken(src, m.group(8))))
    return out


def render(t):
    if t['ty'] not in FMT:
        return None
    fmt, w = FMT[t['ty']]
    n = int(t['count']) if t['count'] else t['size'] // w
    if n * w != len(t['bytes']):
        return None
    vals = [struct.unpack_from(fmt, t['bytes'], i * w)[0] for i in range(n)]
    # A typedef with no `[N]` is a scalar, whatever the buffer's length: the
    # extra bytes are the distance to the next 1997 global, not elements.
    # `coded_size` has twenty bytes and every one of its 23 uses is a scalar.
    # The exception is a name whose *address* is walked -- `__dword_439B7C` is
    # read as `*((uint8_t *)&__dword_439B7C + v83 + 3)` -- and that is a table.
    if t['count'] is None and not t['walked']:
        return '%sstatic %s %s = %d;\n' % ('alignas(16) ' if t['align'] else '',
                                           t['ty'], t['name'], vals[0])
    body, line = [], '  '
    for v in vals:
        s = '%d,' % v
        if len(line) + len(s) > 92:
            body.append(line.rstrip())
            line = '  '
        line += ' ' + s if line != '  ' else s
    body.append(line.rstrip())
    return ('%sstatic %s %s[%d] = {\n%s\n};\n'
            % ('alignas(16) ' if t['align'] else '', t['ty'], t['name'], n,
               '\n'.join(body)))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    ts = tables(src)
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for t in ts:
            r = render(t)
            print('%-22s %-9s x%-4s %s'
                  % (t['name'], t['ty'],
                     t['count'] or t['size'] // FMT.get(t['ty'], ('', 1))[1]
                     if t['ty'] in FMT else '?',
                     'ready' if r else 'left alone: %s' % t['ty']))
        print('%d of %d tables decode' % (sum(1 for t in ts if render(t)), len(ts)))
        return 0

    want, done = set(args), 0
    for t in sorted(ts, key=lambda t: -t['span'][0]):
        if want and t['name'] not in want:
            continue
        r = render(t)
        if r is None:
            print('skipped %s: %s' % (t['name'], t['ty']))
            continue
        # keep the address comment: it is the only record of where this was
        addr = re.search(r'// (0x[0-9A-F]+)', src[t['span'][0]:t['span'][1]])
        if addr:
            r = r.replace(' = {\n', ' = {   // %s\n' % addr.group(1), 1)
        src = src[:t['span'][0]] + r + src[t['span'][1]:]
        done += 1
        print('%-22s one declaration instead of three' % t['name'])
    if done:
        open(path, 'w').write(src)
    print('%d tables' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
