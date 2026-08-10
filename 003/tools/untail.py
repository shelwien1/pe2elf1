#!/usr/bin/env python3
"""Replace a `goto` to a straight-line return with the statements themselves.

    python3 tools/untail.py subs1.hpp
    python3 tools/untail.py subs1.hpp --all

`degoto.py` rewrites a forward `goto` that is the whole of an `if` by inverting
the condition; it declines everything else, and its `--why` breakdown says the
largest remaining group is 22 labels reached by more than one `goto`. Those are
MSVC's shared tails: one copy of an epilogue that several paths jump to.

A shared tail cannot be rewritten by moving code around, but it can be removed
by *copying* it, and for the smallest ones that is plainly worth it:

    if ( ferror(f) )
      goto LABEL_31;                      →    if ( ferror(f) )
    ...                                          {
    LABEL_31:                                      fclose(arc->fp);
      fclose(arc->fp);                             arc->fp = 0;
      arc->fp = 0;                                 return nullptr;
      return nullptr;                            }

The condition is narrow enough that the copy cannot change anything:

  * the label is followed by a run of statements that are not `if`, `else`,
    `for`, `while`, `do`, `switch`, `case`, a brace, another label or another
    `goto` -- so control cannot leave the run except by finishing it;
  * that run ends in an unconditional `return`, so it never falls out of the
    bottom either;
  * the run is at most `--max` statements, five by default.

Together those make the run a value: it reads whatever is in scope, returns,
and has no other exit. Copying it to the `goto` site executes exactly what
jumping there would have, and the label goes with the last `goto` that named
it. The tail stays where it is for the path that falls into it.

What it will not touch is the interesting ones. A tail with an `if` in it, a
tail that falls through into more code, a backward `goto` -- those are control
structure rather than a spelling of it, and `degoto.py --why` is the place they
are counted.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

KEYWORD = re.compile(r'^\s*(?:if|else|for|while|do|switch|case|default'
                     r'|LABEL_\d+:|\}|\{)')
LABEL = re.compile(r'^(\s*)(LABEL_\d+):\s*$')
RETURN = re.compile(r'^\s*return\b')


def tails(lines, a, b, max_stmts):
    """{label: (label line, [body lines])} for every straight-line return tail."""
    out = {}
    for i in range(a, b + 1):
        m = LABEL.match(lines[i])
        if not m:
            continue
        body = []
        for j in range(i + 1, min(i + 2 + max_stmts, len(lines))):
            t = lines[j]
            if KEYWORD.match(t) or 'goto ' in t or not t.strip():
                body = None
                break
            body.append(t)
            if RETURN.match(t):
                break
        else:
            body = None
        if body and RETURN.match(body[-1]):
            out[m.group(2)] = (i, body)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    mx = int(sys.argv[sys.argv.index('--max') + 1]) if '--max' in sys.argv else 5
    lines = open(path).read().split('\n')
    found, sites = {}, {}
    for a, b, nm, _ in structs.bodies(lines):
        local = tails(lines, a, b, mx)
        for i in range(a, b + 1):
            m = re.search(r'goto (LABEL_\d+);', lines[i])
            if m and m.group(1) in local:
                sites.setdefault((nm, m.group(1)), []).append(i)
        for k, v in local.items():
            found[(nm, k)] = v
    found = {k: v for k, v in found.items() if k in sites}

    if '--all' not in sys.argv:
        for k in sorted(found, key=lambda k: found[k][0]):
            print('%6d  %-24s %-12s %d goto%s, %d statement%s' %
                  (found[k][0] + 1, k[0].lstrip('_'), k[1],
                   len(sites[k]), 's'[:len(sites[k]) ^ 1],
                   len(found[k][1]), 's'[:len(found[k][1]) ^ 1]))
        print('%d shared tails, %d gotos' %
              (len(found), sum(len(s) for s in sites.values())))
        return 0

    edits = []
    for k, (at, body) in sorted(found.items()):
        for i in sites[k]:
            edits.append((i, k, body))
        edits.append((at, k, None))          # the label line itself
    for i, _k, body in sorted(edits, key=lambda e: -e[0]):
        if body is None:
            del lines[i]
            continue
        ind = re.match(r'^(\s*)', lines[i]).group(1)
        # The `goto` may be the whole consequent of an `if`; a block is always
        # a legal statement, so brace it rather than reasoning about which.
        stripped = [ind + '  ' + b.strip() for b in body]
        lines[i:i + 1] = [ind + '{'] + stripped + [ind + '}']
    open(path, 'w').write('\n'.join(lines))
    print('%d gotos replaced by their tail, %d labels gone' %
          (sum(len(s) for s in sites.values()), len(found)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
