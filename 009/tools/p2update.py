#!/usr/bin/env python3
"""Fold `alt_p2_model`'s written-out counter updates into `p2_update`.

    python3 tools/p2update.py                  # what it would take
    python3 tools/p2update.py --list           # and every site, one per line
    python3 tools/p2update.py --apply

CLEANER.md Phase 1.  `alt_p2_model.inc` calls `p2_bump` 117 times, and 115 of
those are one counter update written as three statements and a pair of
temporaries named after the counter:

    wd0800c = d0800->weighted;
    ed0800c = res_c-p2_pred(wd0800c, d0800->rate);
    d0800->weighted = p2_bump(wd0800c, ed0800c, 2);

which is `p2_update(d0800, res_c, 2);`.

**What makes the fold safe.**  Eight of the 115 read the rate as
`*(uint8_t*)&d4000[-1].rate`, and `P2Count::rate` is `int8_t`, so those eight
take the byte unsigned where `p2_update` takes it signed.  Everywhere else that
would be a defect; here it is not, because `p2_pred` uses the rate only as two
shift counts and masks both with `&31`, and the two readings differ by 256.
This tool does not reason about that -- it is written down in `alt_p2_block.inc` beside
the helper, and this comment exists so that a later reader who widens the rule
knows what the rule was resting on.

**The three shapes it will not take.**

  * **the two `-=` variants** (lines 294 and 562).  `nres3 -= p2_pred(wm0400z,
    m0400->rate);` subtracts into the *residual*, which is read again later.
    `p2_update` takes the residual by value and would drop that write.
  * **the block that straddles a brace** (line 639).  Its temporaries are
    assigned in both arms of an `if`/`else` -- `neg_d` in one, `nres4` in the
    other -- and the bump sits after the join.  Folding it means hoisting the
    residual choice out, which is a control-flow edit and not this tool's.
  * **any interleaved block whose stray statement it cannot hoist.**  Twelve
    blocks have a statement scheduled into the middle of them, and eleven can
    be moved in front of the call because the statement writes a name the call
    does not read and never mentions the counter.  That condition is checked
    per site rather than assumed: `res_c = res_s;` in front of an update whose
    residual is `res_c` would be a different program.
"""
import re
import sys

PATH = 'alt_p2_model.inc'

ACC = r'(\w+(?:\[-?\d+\])?)(?:->|\.)'
W = re.compile(r'^(\s*)(\w+)\s*=\s*' + ACC + r'weighted;\s*$')
B = re.compile(r'^(\s*)' + ACC + r'weighted\s*=\s*p2_bump\('
               r'\s*(\w+)\s*,\s*(\w+)\s*,\s*(\d+)\s*\);\s*$')
E = re.compile(r'^\s*(\w+)\s*(-?)=\s*(.*?)-?p2_pred\('
               r'\s*(\w+)\s*,\s*(.+?)\s*\);\s*$')
# `weighted += (uint32_t)(res-p2_pred(<the same counter>, <its rate>)+2)>>2;`
NUDGE = re.compile(r'^(\s*)\*\(uint16_t\*\)&(\w+\[\d+\])\.weighted \+= '
                   r'\(uint32_t\)\((.+?)-p2_pred\((.+?), (.+?)\)\+2\)>>2;\s*$')


def ptr(acc):
    """The `P2Count*` for an accessor: `d0800` stays, `d4000[-1]` gets an `&`."""
    return '&' + acc if acc.endswith(']') else acc


def counter(expr, acc):
    """Does `expr` reach `acc`'s record, however it spells it?

    `*(uint8_t*)&d4000[-1].rate` and `d4000[-1].rate` are the same member; the
    check is on the *base name*, so a rate borrowed from a different counter
    stops the fold instead of being folded to this one's.
    """
    base = acc.split('[')[0]
    return re.search(r'(?<![\w])%s(?![\w])' % re.escape(base), expr) is not None


def sites(lines):
    """[(kind, first, last, call, stray)] for every `p2_bump` in the file.

    `first`..`last` are inclusive 0-based indices of the lines the call
    replaces; `stray` is the interleaved statement to hoist, or None.
    """
    out = []
    for i, l in enumerate(lines):
        if 'p2_bump' not in l:
            continue
        mb = B.match(l)
        if not mb:
            out.append(('shape', i, i, None, None))
            continue
        indent, acc, w, e, sh = mb.groups()
        je = None
        for j in range(i - 1, max(-1, i - 8), -1):
            me = E.match(lines[j])
            if me and me.group(1) == e and counter(me.group(5), acc):
                je = j
                break
        if je is None:
            out.append(('no-pred', i, i, None, None))
            continue
        me = E.match(lines[je])
        if me.group(2) == '-':
            out.append(('decrement', je, i, None, None))
            continue
        jw = None
        for j in range(je - 1, max(-1, je - 8), -1):
            mw = W.match(lines[j])
            if mw and mw.group(2) == w and mw.group(3) == acc:
                jw = j
                break
        if jw is None or me.group(4) != w:
            out.append(('no-weight', je, i, None, None))
            continue
        res = me.group(3).strip()
        call = '%sp2_update(%s, %s, %s);' % (indent, ptr(acc), res, sh)
        gap = [k for k in range(jw + 1, i) if k != je]
        if not gap:
            out.append(('exact', jw, i, call, None))
            continue
        if len(gap) > 1 or '{' in lines[gap[0]] or '}' in lines[gap[0]]:
            out.append(('braced', jw, i, None, None))
            continue
        s = lines[gap[0]]
        lhs = re.match(r'\s*(\w+)\s*[-+|&^]?=[^=]', s)
        # The hoist is safe when the statement cannot change what the call
        # reads: not the residual, not the counter, and a plain assignment so
        # there is nothing else in it to reason about.
        ok = (lhs and s.rstrip().endswith(';') and '(' not in s.split('=')[0]
              and not counter(s, acc)
              and not re.search(r'(?<![\w])%s(?![\w])' % re.escape(lhs.group(1)),
                                res))
        out.append(('hoist' if ok else 'stray', jw, i, call, s if ok else None))
    return out


def nudges(lines):
    """[(index, call)] for the 21 `weighted += …>>2` folds."""
    out = []
    for i, l in enumerate(lines):
        m = NUDGE.match(l)
        if not m:
            continue
        indent, acc, res, w, rate = m.groups()
        if counter(w, acc) and counter(rate, acc):
            out.append((i, '%sp2_nudge(%s, %s);' % (indent, ptr(acc), res)))
    return out


def apply(lines, found, nudged):
    out, skip = [], {}
    for kind, a, b, call, stray in found:
        if call is None or kind not in ('exact', 'hoist'):
            continue
        skip[a] = (b, ([stray] if stray else []) + [call])
    nud = dict(nudged)
    i = 0
    while i < len(lines):
        if i in skip:
            b, text = skip[i]
            out.extend(text)
            i = b + 1
        else:
            out.append(nud.get(i, lines[i]))
            i += 1
    return out


def prune(lines):
    """Drop names the file no longer mentions from its declaration wall.

    The 230 temporaries are declared in one statement of 268 names over three
    lines.  Removing the blocks leaves them declared and unused, which is a
    warning and, worse, a name that still reads like state.
    """
    out, i = [], 0
    body = '\n'.join(lines)
    while i < len(lines):
        # `uint32_t AltP2Block::alt_p2_model(…) {` starts with a type too, and
        # the first draft of this walked past its `{` looking for a `;`, found
        # the first declaration below it, and rewrote the signature and the
        # declaration as one comma list.  A declaration has no parentheses.
        m = re.match(r'^(\s*)(int32_t|uint32_t)\s+[A-Za-z_]', lines[i])
        if m and ('(' in lines[i] or '{' in lines[i]):
            m = None
        stmt, j = lines[i], i
        while m and not stmt.rstrip().endswith(';') and j + 1 < len(lines):
            j += 1
            stmt += ' ' + lines[j].strip()
        if m and ('(' in stmt or '{' in stmt):
            m = None
        if not m or j == i or ',' not in stmt:
            out.append(lines[i])
            i += 1
            continue
        indent, ty = m.group(1), m.group(2)
        names = [n.strip() for n in
                 re.sub(r'^\s*%s\s+' % ty, '', stmt.strip()).rstrip(';').split(',')]
        keep = [n for n in names
                if len(re.findall(r'(?<![\w.])%s(?![\w])' % re.escape(n), body)) > 1]
        if len(keep) == len(names):
            out.extend(lines[i:j + 1])
            i = j + 1
            continue
        text, line = [], indent + ty + ' '
        for n in keep:
            if len(line) + len(n) + 2 > 96:
                text.append(line.rstrip())
                line = indent + '    '
            line += n + ', '
        text.append(line.rstrip().rstrip(',') + ';')
        out.extend(text)
        i = j + 1
    return out


def main():
    lines = open(PATH).read().split('\n')
    found, nudged = sites(lines), nudges(lines)
    tally = {}
    for kind, a, b, _c, s in found:
        tally[kind] = tally.get(kind, 0) + 1
        if '--list' in sys.argv:
            print('  %-10s %s:%d-%d%s'
                  % (kind, PATH, a + 1, b + 1, '  hoist: ' + s.strip() if s else ''))
    take = tally.get('exact', 0) + tally.get('hoist', 0)
    print('%d of %d p2_bump sites fold to p2_update (%d exact, %d after a hoist)'
          % (take, len(found), tally.get('exact', 0), tally.get('hoist', 0)))
    for kind in ('decrement', 'braced', 'stray', 'shape', 'no-pred', 'no-weight'):
        if tally.get(kind):
            print('  %d declined: %s' % (tally[kind], kind))
    print('%d `weighted += …>>2` folds become p2_nudge' % len(nudged))
    if '--apply' in sys.argv:
        out = prune(apply(lines, found, nudged))
        open(PATH, 'w').write('\n'.join(out))
        print('-- applied to %s (%d lines -> %d)' % (PATH, len(lines), len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
