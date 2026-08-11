#!/usr/bin/env python3
"""Put back a load the compiler hoisted out of the expression that uses it.

    python3 tools/unhoist.py subs1.hpp
    python3 tools/unhoist.py subs1.hpp --all

MSVC schedules loads early to hide their latency, and Hex-Rays gives each
hoisted load a name, so one expression arrives as a pile of one-use locals
with the expression itself two screens further down:

    v121 = ra0[-2].lane[2];             ctx = (ra0[-2].lane[2]
    v122 = cx0[-2].lane[2];                  + ra0->lane[2]) & 0x200000
    lane2 = ra0->lane[2];         →          | cx0[-2].lane[2] & 0x100000
    ...                                      | ...;
    ctx = (v121 + lane2) & 0x200000
        | v122 & 0x100000
        | ...;

`v121` is not a quantity the algorithm has a word for. It is the second
operand of a term, moved. Naming it means inventing a word for something
that does not have one, and the names come out `k0`, `k1`, `k2` -- which is
the spelling this project already found does not explain anything.

A hoist qualifies when:

  * `X` is spelled the way Hex-Rays spells one, `vNN` or `tNN`.  A name a
    person chose -- `Depth = p_i_img->depth & 0x3F` -- reads once as well, and
    inlining it would delete a word written on purpose.  `--named` lists those
    without touching them;
  * the line is `X = EXPR;` with `X` a local the body declares, `EXPR` free of
    calls, assignments, `?:` and `++`/`--`, and `X`'s address never taken.  A
    hoisted load has no side effect; `v = ++k < n` reads once but moving it
    moves the increment past everything between that reads `k`;
  * `X` is assigned exactly once and read exactly once, and the read is after
    the assignment;
  * between the two, nothing writes any name `EXPR` reads -- no assignment, no
    `++`, no `&name`, no `LOWORD(name) =` -- and there is no call, because a
    callee can write through a pointer this cannot see;
  * between the two, the text is brace-balanced and never leaves the block the
    assignment is in, so the read cannot be on a path where the assignment did
    not happen;
  * there is no label in the span.

`EXPR` is parenthesised at the read unless it is a primary -- a name with
`.`, `->` and `[...]` on it -- so a hoisted sum landing under a `*` keeps its
own precedence.

This is deliberately narrower than "inline any single-use temporary". An
expression with work in it stays behind its name the moment it is read twice:
duplicating it is copying logic, not putting a load back. A primary -- a name
with `.`, `->` and constant subscripts on it -- may go back at up to three
reads, because the span check has already proved nothing between them can make
two reads differ, and `cx0[-4].err` spelled twice is no harder to read than
`v236` spelled twice.  Reads are counted as occurrences and not as statements -- Hex-Rays
wraps one expression over twenty lines and can name the same local six times
inside it, and counting statements read that as a single use.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import undup                                                      # noqa: E402
import unreload                                                   # noqa: E402

ASSIGN = re.compile(r'^(\s*)([A-Za-z_]\w*) = (.+);\s*$')
LABEL = re.compile(r'^\s*LABEL_\d+:')
CALL = re.compile(r'\b(?!if|while|for|switch|return|sizeof)\w+\s*\(')
PRIMARY = re.compile(r'^[A-Za-z_]\w*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*|\s*\[[^\[\]]*\])*$')
HEXRAYS = re.compile(r'^[vt]\d+$')
CONTROL = re.compile(r'^(?:if|else|while|do|for|switch|case|default|return|goto|break|continue)\b')
KEYWORD = {'if', 'else', 'while', 'do', 'for', 'return', 'switch', 'case',
           'break', 'continue', 'goto', 'sizeof', 'true', 'false', 'nullptr'}
# `&x` is the variable's address; `&x->m`, `&x.m` and `&x[i]` are not -- they
# name a member or an element, and a callee holding one of those cannot change
# `x` itself.  Without the lookahead every `*(uint16_t *)&v533->w2` read as
# "address taken" and disqualified the local, which is thirteen of the counter
# pointers in `alt_p2_model` alone.
ADDR = r'&\s*%s\b(?!\s*(?:->|\.|\[))'
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         ADDR,
         r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)',
         r'\b(?:BYTE[123]|WORD[123])\s*\(\s*%s\s*\)']


def statements(code):
    """[(first line, last line, text)] -- Hex-Rays wraps long expressions.

    Punctuation and control lines stand alone.  A first version joined any
    line that did not end in `;` to whatever followed, so a bare `{` swallowed
    the statement after it -- and every assignment that opens a block became
    invisible to the caller, which is three of `alt_p2_context`'s remaining
    names and no error message anywhere.
    """
    out, i = [], 0
    while i < len(code):
        s = code[i].strip()
        if not s or s in ('{', '}', '};') or s.endswith('{') or LABEL.match(code[i]):
            out.append((i, i, code[i]))
            i += 1
            continue
        if CONTROL.match(s):
            # A wrapped condition ends where its parentheses balance, not at a
            # `;` -- `if ( a\n || b )` has no semicolon of its own.
            j, depth = i, 0
            while j < len(code):
                depth += code[j].count('(') - code[j].count(')')
                if depth <= 0:
                    break
                j += 1
            j = min(j, len(code) - 1)
            out.append((i, j, ' '.join(r.strip() for r in code[i:j + 1])))
            i = j + 1
            continue
        if code[i].count('(') != code[i].count(')') or not code[i].rstrip().endswith(';'):
            j = i
            depth = 0
            while j < len(code):
                depth += code[j].count('(') - code[j].count(')')
                if depth <= 0 and code[j].rstrip().endswith(';'):
                    break
                j += 1
            if j < len(code):
                out.append((i, j, ' '.join(r.strip() for r in code[i:j + 1])))
                i = j + 1
                continue
        out.append((i, i, code[i]))
        i += 1
    return out


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        ty = unreload.types(code)
        stmts = statements(code)
        # Which statement each line belongs to, so a read inside a wrapped
        # expression is attributed to the whole statement and not its first row.
        owner = {}
        for k, (lo, hi, _t) in enumerate(stmts):
            for r in range(lo, hi + 1):
                owner[r] = k
        for k, (lo, hi, text) in enumerate(stmts):
            m = ASSIGN.match(text.strip() and text or '')
            if not m:
                continue
            dst, expr = m.group(2), m.group(3).strip()
            if dst not in ty or undup.declaration(text):
                continue
            if CALL.search(expr) or '?' in expr or re.search(r'(?<![=!<>+\-*/|&^%])=(?!=)', expr):
                continue
            # `xf3 = ++xf1 < np` reads once, but moving it moves the increment
            # past every line between that reads `xf1`.  A hoisted load has no
            # side effect; anything that does is not one.
            if '++' in expr or '--' in expr:
                continue
            # Only names Hex-Rays invented.  `Depth = p_i_img->depth & 0x3F`
            # reads once too, and inlining it would delete a word someone chose
            # on purpose -- this rule is about the pile of `vNN` around one
            # expression, not about every temporary that happens to be single-use.
            if not HEXRAYS.match(dst) and '--named' not in sys.argv:
                continue
            # Occurrences, not statements.  One wrapped expression can name
            # the local six times, and the first version of this counted that
            # as a single read -- putting the load back would have made six
            # loads out of one, which is the thing the rule exists to refuse.
            reads = [(j, len(re.findall(r'\b%s\b' % re.escape(dst), t)))
                     for j, (_l, _h, t) in enumerate(stmts)
                     if j != k and re.search(r'\b%s\b' % re.escape(dst), t)
                     and not undup.declaration(t)]
            total = sum(n for _j, n in reads)
            if not reads or reads[0][0] < k:
                continue
            # One read of anything, or a few reads of a primary.  A primary --
            # a name with `.`, `->` and constant subscripts on it -- costs
            # nothing to spell again and duplicates no logic, and the span
            # check below already proves nothing between the reads can make
            # two of them differ.  Anything longer stays behind its name at
            # more than one read: that would be copying an expression, not
            # putting a load back.
            if total != 1 and not (total <= 3 and PRIMARY.match(expr)):
                continue
            use = reads[-1][0]
            first = reads[0][0]
            if re.search(ADDR % re.escape(dst), '\n'.join(code)):
                continue
            span = code[hi + 1:stmts[use][0]]
            del first
            if any(LABEL.match(r) for r in span) or any(CALL.search(r) for r in span):
                continue
            depth = 0
            for r in span:
                depth += r.count('{') - r.count('}')
                if depth < 0:
                    break
            else:
                if depth != 0:
                    continue
                names = {w for w in re.findall(r'\b[A-Za-z_]\w*\b', expr)
                         if w not in KEYWORD}
                body = '\n'.join(span)
                if any(re.search(p % re.escape(w), body) for w in names for p in WRITE):
                    continue
                out.append((nm, a + lo, a + hi, a + stmts[reads[0][0]][0],
                            a + stmts[use][1], dst, expr, m.group(1)))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, lo, _hi, _ulo, _uhi, dst, expr, _ind in found:
            print('%6d  %-24s %-12s = %s'
                  % (lo + 1, nm.lstrip('_'), dst, expr[:46]))
        print('%d loads hoisted out of the one expression that reads them' % len(found))
        return 0

    # One site per pass, recomputed each time.  Two hoists can be read by the
    # same expression -- `num_d = 16 * (v18 + v19 + v17)` names two of them --
    # and applying both from one survey deletes the first assignment under the
    # second's recorded line numbers.  That version left `v17` declared,
    # unassigned and still read, which builds clean and changed three streams.
    done = 0
    while True:
        found = candidates(lines)
        if not found:
            break
        _nm, lo, hi, ulo, uhi, dst, expr, _ind = found[0]
        text = expr if PRIMARY.match(expr) else '(%s)' % expr
        for r in range(ulo, uhi + 1):
            lines[r] = re.sub(r'\b%s\b' % re.escape(dst), lambda _m: text, lines[r])
        del lines[lo:hi + 1]
        done += 1
    open(path, 'w').write('\n'.join(lines))
    print('%d hoisted loads put back' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
