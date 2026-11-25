#!/usr/bin/env python3
# Content-stream modelling prototypes for docs/pdfdump_plan2.md, measured with xz on streams
# built the way pdfdump builds them (per-group zigzag varints, raw or delta, one decimal unit per
# group). Input: raw content streams, i.e. dumps made with -s 1:
#
#   i=0; for f in *.pdf; do i=$((i+1)); mkdir -p work/s1/$i; pdfdump c -q -n -s 1 "$f" work/s1/$i/d; done
#   python3 tests/csproto.py work
#
# Variants: path coordinates against the current point; Td/TD advances from a table learned per
# (font, size, text shown); TJ kerning from a table per (font, glyph before, glyph after); TJ
# strings and kerning in one stream; TJ numbers / strings grouped per font.
import os, re, sys, lzma, zlib, collections, concurrent.futures as cf

TOK = re.compile(rb'%[^\r\n]*|\((?:\\.|[^\\()]|\((?:\\.|[^\\()]|\((?:\\.|[^\\()])*\))*\))*\)|<<|>>|<[0-9A-Fa-f\s]*>|/[^\s/<>\[\]()%{}]*|[-+]?(?:\d+\.?\d*|\.\d+)(?![^\s/<>\[\]()%{}])|[^\s/<>\[\]()%{}]+|\[|\]|[{}]', re.S)
NUM = re.compile(rb'[-+]?(?:\d+\.?\d*|\.\d+)$')
PATH = {b'm': 2, b'l': 2, b'c': 6, b'v': 4, b'y': 4, b're': 4}

def zz(v): return (v << 1) ^ (v >> 63) if v >= 0 else ((-v) << 1) - 1
def var(v, out):
    while v >= 0x80: out.append((v & 0x7F) | 0x80); v >>= 7
    out.append(v)
def num(t):
    s = t.decode()
    neg = s[0] == '-'
    s = s.lstrip('+-')
    if '.' in s: a, b = s.split('.')
    else: a, b = s, ''
    b = b[:6]
    m = int((a or '0') + b) if (a + b) else 0
    return (-m if neg else m), len(b)

class Groups:
    def __init__(self): self.g = collections.OrderedDict()
    def add(self, key, m, sc): self.g.setdefault(key, []).append((m, sc))
    def encode(self):
        out = bytearray()
        for key, vals in self.g.items():
            S = max(sc for _, sc in vals)
            ints = [m * 10 ** (S - sc) for m, sc in vals]
            # one unit per group: drop the power of ten common to all values
            k = 0
            while k < 6 and all(v % 10 ** (k + 1) == 0 for v in ints): k += 1
            if k: ints = [v // 10 ** k for v in ints]
            raw = bytearray(); dl = bytearray(); prev = 0
            for v in ints:
                var(zz(v), raw); var(zz(v - prev), dl); prev = v
            if len(vals) >= 32 and len(zlib.compress(bytes(dl), 6)) < len(zlib.compress(bytes(raw), 6)): out += dl
            else: out += raw
        return bytes(out)

def process(path):
    d = open(path, 'rb').read()
    toks = []
    pos = 0
    for m in TOK.finditer(d):
        t = m.group(0)
        if m.start() < pos: continue
        if t == b'ID':  # inline image: skip to EI
            e = re.compile(rb'[\s]EI(?=[\s/<\[(]|$)').search(d, m.end())
            pos = e.end() if e else len(d)
            toks.append(('op', b'ID')); continue
        c = t[:1]
        if c == b'%': continue
        if c == b'(': toks.append(('str', t))
        elif c == b'<' and t[:2] != b'<<': toks.append(('hex', t))
        elif c == b'/': toks.append(('name', t))
        elif t in (b'[', b']'): toks.append((t.decode(), t))
        elif NUM.match(t): toks.append(('num', t))
        elif t in (b'<<', b'>>', b'{', b'}'): toks.append(('x', t))
        else: toks.append(('op', t))
    V = {k: Groups() for k in ('base', 'path', 'td', 'tj', 'all', 'nojoint', 'tjfont')}
    byfont = collections.OrderedDict(); hexorder = bytearray(); hexbyfont = collections.OrderedDict()
    strs = bytearray(); strs_notj = bytearray(); joint = bytearray()
    ops = []  # pending operands
    cur = [0, 0]; start = [0, 0]
    font = (b'', b'')
    shown = b''          # text shown since the last positioning operator
    tdtab = {}; kern = {}
    stat = collections.Counter()
    PS = 6               # path coordinates in one unit for current-point residuals

    def strbytes(k, t):
        if k == 'hex':
            h = re.sub(rb'\s', b'', t[1:-1])
            if len(h) % 2: h += b'0'
            try: return bytes.fromhex(h.decode())
            except ValueError: return b''
        return t[1:-1]

    for kind, t in toks:
        if kind != 'op':
            ops.append((kind, t)); continue
        op = t
        nums = [(i, num(x)) for i, (k, x) in enumerate(ops) if k == 'num']
        # --- strings
        tj_elems = []
        inarr = False
        for k, x in ops:
            if k == '[': inarr = True
            elif k == ']': inarr = False
            elif k in ('str', 'hex'):
                b = strbytes(k, x)
                if k == 'str': strs += b + b'\n'
                if op == b'TJ': tj_elems.append(('s', b))
                elif k == 'str': strs_notj += b + b'\n'
            elif k == 'num' and inarr and op == b'TJ': tj_elems.append(('n', num(x)))
        # --- baseline grouping: (op, slot) outside arrays, (op, 'a') inside
        slot = 0; inarr = False; seq = []
        for k, x in ops:
            if k == '[': inarr = True
            elif k == ']': inarr = False
            elif k == 'num':
                key = (op, 'a') if inarr else (op, min(slot, 6))
                seq.append((key, num(x)))
                if not inarr: slot += 1
        for key, (m, sc) in seq:
            V['base'].add(key, m, sc)
            V['tjfont'].add((key[0], key[1], font[0]) if op == b'TJ' and key[1] == 'a' else key, m, sc)
        for k, x in ops:
            if k == 'str': byfont.setdefault(font[0], bytearray()).extend(strbytes(k, x) + b'\n')
            elif k == 'hex':
                hb = strbytes(k, x); hexorder.extend(hb); hexbyfont.setdefault(font[0], bytearray()).extend(hb)
        # --- path: residual against the current point
        done_path = False
        if op in PATH and len(seq) == PATH[op] and all(not isinstance(k[1], str) for k, _ in seq):
            xs = [m * 10 ** (PS - sc) for _, (m, sc) in seq]
            res = []
            if op in (b'm', b'l'):
                res = [xs[0] - cur[0], xs[1] - cur[1]]; cur = [xs[0], xs[1]]
                if op == b'm': start = list(cur)
            elif op == b'c':
                res = [xs[0] - cur[0], xs[1] - cur[1], xs[2] - xs[0], xs[3] - xs[1], xs[4] - xs[2], xs[5] - xs[3]]; cur = [xs[4], xs[5]]
            elif op in (b'v', b'y'):
                res = [xs[0] - cur[0], xs[1] - cur[1], xs[2] - xs[0], xs[3] - xs[1]]; cur = [xs[2], xs[3]]
            elif op == b're':
                res = [xs[0] - cur[0], xs[1] - cur[1], xs[2], xs[3]]; cur = [xs[0], xs[1]]; start = list(cur)
            for i, r in enumerate(res):
                for name in ('path', 'all', 'nojoint'): V[name].add((op, i, 'cp'), r, 0)
            done_path = True
            stat['path_nums'] += len(res)
        elif op == b'h': cur = list(start)
        # --- Td: advance learned per (font, size, text shown before)
        done_td = False
        if op in (b'Td', b'TD') and len(seq) == 2:
            key = (font, shown)
            (mx, sx), (my, sy) = seq[0][1], seq[1][1]
            if shown and key in tdtab:
                px, py = tdtab[key]
                rx = mx * 10 ** (6 - sx) - px; ry = my * 10 ** (6 - sy) - py
                for name in ('td', 'all', 'nojoint'):
                    V[name].add((op, 0, 'hit'), rx, 0); V[name].add((op, 1, 'hit'), ry, 0)
                stat['td_hit'] += 1; stat['td_exact'] += rx == 0 and ry == 0
            else:
                for name in ('td', 'all', 'nojoint'):
                    V[name].add((op, 0), mx, sx); V[name].add((op, 1), my, sy)
                stat['td_miss'] += 1
            if shown: tdtab[key] = (mx * 10 ** (6 - sx), my * 10 ** (6 - sy))
            done_td = True
        # --- TJ: kerning learned per (font, last byte before, first byte after)
        done_tj = False
        if op == b'TJ' and tj_elems:
            w = 2 if any(k == 'hex' for k, _ in ops) else 1
            prev = b''
            for i, (k, v) in enumerate(tj_elems):
                if k == 's': prev = v; continue
                nxt = tj_elems[i + 1][1] if i + 1 < len(tj_elems) and tj_elems[i + 1][0] == 's' else b''
                key = (font[0], prev[-w:], nxt[:w])
                m, sc = v
                val = m * 10 ** (6 - sc)
                if key in kern:
                    for name in ('tj', 'all'): V[name].add((op, 'hit'), val - kern[key], 0)
                    stat['tj_hit'] += 1; stat['tj_exact'] += val == kern[key]
                else:
                    for name in ('tj', 'all'): V[name].add((op, 'a'), m, sc)
                    stat['tj_miss'] += 1
                kern[key] = val
            # joint stream: strings and kerning numbers of TJ arrays together
            for k, v in tj_elems:
                if k == 's': joint += v
                else:
                    joint.append(1); var(zz(v[0] * 10 ** (3 - min(v[1], 3)) if v[1] <= 3 else v[0]), joint)
            joint.append(10)
            done_tj = True
        # numbers not handled by a variant go to its baseline groups
        for name, done in (('path', done_path), ('td', done_td), ('tj', done_tj)):
            if not done:
                for key, (m, sc) in seq: V[name].add(key, m, sc)
        if not (done_path or done_td or done_tj):
            for key, (m, sc) in seq: V['all'].add(key, m, sc)
        if not (done_path or done_td):
            for key, (m, sc) in seq:
                if not (op == b'TJ' and key[1] == 'a'): V['nojoint'].add(key, m, sc)
        # --- state
        if op == b'Tf' and len(ops) >= 2 and ops[0][0] == 'name': font = (ops[0][1], ops[1][1] if ops[1][0] == 'num' else b'')
        if op in (b'Tj', b"'", b'"', b'TJ'):
            for k, x in ops:
                if k in ('str', 'hex'): shown += strbytes(k, x)
            if len(shown) > 64: shown = shown[-64:]
        elif op in (b'Td', b'TD', b'Tm', b'T*', b'BT', b'ET'): shown = b''
        ops = []
    x = lambda b: len(lzma.compress(bytes(b), preset=9)) if b else 0
    r = collections.Counter({k: x(v.encode()) for k, v in V.items()})
    r['strs'] = x(strs); r['strs_notj'] = x(strs_notj); r['joint'] = x(joint)
    r['strs_byfont'] = x(b''.join(bytes(v) for v in byfont.values())); r['hex_order'] = x(hexorder); r['hex_byfont'] = x(b''.join(bytes(v) for v in hexbyfont.values()))
    r.update(stat)
    return r

if __name__ == '__main__':
    root = sys.argv[1]
    files = [root + '/s1/' + i + '/d.cs' for i in sorted(os.listdir(root + '/s1'), key=int) if os.path.exists(root + '/s1/' + i + '/d.cs')]
    T = collections.Counter()
    with cf.ProcessPoolExecutor(4) as ex:
        for r in ex.map(process, files): T += r
    b = T['base']
    print('files %d' % len(files))
    print('numbers, prototype baseline (per (operator, slot) groups): %d' % b)
    for k, lab in (('path', 'path coordinates against the current point'), ('td', 'Td/TD advance learned per (font, size, text shown)'), ('tj', 'TJ kerning learned per (font, glyph before, glyph after)'), ('all', 'all three')):
        print('  %-55s %9d  %+6.1f%%' % (lab, T[k], 100.0 * (T[k] - b) / b))
    print('Td: hits %d (exact %d) misses %d ; TJ numbers: hits %d (exact %d) misses %d ; path numbers %d' % (T['td_hit'], T['td_exact'], T['td_miss'], T['tj_hit'], T['tj_exact'], T['tj_miss'], T['path_nums']))
    print('strings (literal, all): %d ; outside TJ arrays: %d ; joint TJ stream (strings + kerning): %d' % (T['strs'], T['strs_notj'], T['joint']))
    print('TJ numbers grouped per font: numbers %d (%+.1f%%)' % (T['tjfont'], 100.0 * (T['tjfont'] - b) / b))
    print('literal strings in stream order %d ; grouped per font %d (%+.1f%%) ; hex string bytes in order %d ; grouped per font %d (%+.1f%%)' % (T['strs'], T['strs_byfont'], 100.0 * (T['strs_byfont'] - T['strs']) / T['strs'], T['hex_order'], T['hex_byfont'], 100.0 * (T['hex_byfont'] - T['hex_order']) / max(T['hex_order'], 1)))
    sep = T['all'] + T['strs']
    jn = T['nojoint'] + T['strs_notj'] + T['joint']
    print('numbers + strings: baseline %d ; path+Td+TJ tables %d (%+.1f%%) ; path+Td + joint TJ stream %d (%+.1f%%)' % (b + T['strs'], sep, 100.0 * (sep - b - T['strs']) / (b + T['strs']), jn, 100.0 * (jn - b - T['strs']) / (b + T['strs'])))
