#!/usr/bin/env python3
"""Check that README.md lists every script in this directory, and what it is.

    python3 tools/readme.py            # report what has drifted
    python3 tools/readme.py --apply    # rewrite the table

`README.md` used to say, as its reason for naming only a dozen of these, that
"a table of eighty-seven rows maintained by hand is a table that goes stale,
which is the subject of half of `REFACTORING9.md`".  That was true of a table
maintained by hand.  It is not a reason to leave ninety-seven scripts
undescribed; it is a reason for the table to be derived from the scripts, which
is what this does.

The summary of a tool is its own first line and nothing else -- a `.py` file's
docstring opener, a `.sh` file's `# name -- summary` header.  Nothing here
writes prose about a tool: if a row reads badly the fix is in that tool's first
line, where the person changing the tool will see it.

The `sweep` column is computed from `sweep.sh`'s own skip list rather than
listed, for the same reason.  A tool the sweep holds to zero is a claim about
the file; a tool it skips is a generator, an imported module, or something that
answers with proposals.

This is one of the tools `proven.sh` reports as flat, and correctly: its
subject is this directory, not `subs1.hpp`, so replaying it against an old
revision of the decompilation cannot move it.  `reads.py` says the same thing
from the other side.  It is in the sweep anyway, because the drift it reports
is drift the sweep should fail on.
"""
import ast
import fnmatch
import glob
import re
import sys

HERE = __file__.rsplit('/', 1)[0]
DOC = HERE + '/README.md'
BEGIN = '<!-- readme.py: generated, do not edit by hand -->'
END = '<!-- end readme.py -->'


def summary(path):
    """The script's own first line, however that script spells one."""
    text = open(path).read()
    if path.endswith('.py'):
        try:
            doc = ast.get_docstring(ast.parse(text)) or ''
        except SyntaxError:
            return '(does not parse)'
        first = doc.strip().split('\n')[0].strip()
    else:
        # `# name.sh -- summary`, which may wrap onto the next comment line.
        lines = text.split('\n')
        body = []
        for l in lines[1:]:
            if not l.startswith('#'):
                break
            l = l[1:].strip()
            if not l:
                break
            body.append(l)
        first = ' '.join(body)
        first = re.sub(r'^\S+\s+[-—]+\s*', '', first)
        # A `.py` docstring opener is already one line; a `.sh` header is a
        # block that may run on, so this takes the first sentence of it.  Two
        # things a naive cut gets wrong, and both are in this directory: a
        # period inside a name (`ref_<name>.bmf`, `test.sh`) is not a boundary,
        # so the boundary has to be a period followed by *space*; and an
        # ellipsis is three of those, so `degoto.py`'s
        # "if (c) goto L; ... L:" became "if (c) goto L;" when this rule was
        # applied to both kinds.  It is not applied to both kinds now.
        first = re.split(r'(?<!\.)\.(?!\.)\s', first)[0]
    first = first.rstrip('.')
    return first[:1].upper() + first[1:] if first else ''


def swept():
    """The names `sweep.sh` runs, decided by `sweep.sh`'s own skip list.

    Read out of the script rather than restated here: a second copy of a rule
    is a second thing to keep true, and this one is three lines of `case`.
    """
    text = open(HERE + '/sweep.sh').read()
    skip = set()
    for m in re.finditer(r'^\s*([\w.*|]+)\)\s*continue\s*;;', text, re.M):
        skip.update(m.group(1).split('|'))
    out = {}
    for f in sorted(glob.glob(HERE + '/*.py')):
        name = f.rsplit('/', 1)[-1]
        # `fnmatch` and not a `startswith` on the stem: the first version
        # stripped five characters off `mk*.py` and matched every name
        # beginning with `m`, which quietly said `merge.py` and
        # `methodise.py` were not in a sweep that runs both.
        out[name] = not any(fnmatch.fnmatch(name, s) for s in skip)
    return out


def table():
    run = swept()
    rows = ['| script | what it is | in the sweep |', '| --- | --- | --- |']
    for f in sorted(glob.glob(HERE + '/*.py') + glob.glob(HERE + '/*.sh'),
                    key=lambda p: p.rsplit('/', 1)[-1]):
        name = f.rsplit('/', 1)[-1]
        mark = '' if name.endswith('.sh') else ('yes' if run[name] else 'no')
        # No script's first line contains a `|` today.  One that did would end
        # a cell early and shift every column after it, in a table nobody
        # reads closely because it is generated -- so it is escaped here
        # rather than left as a thing to notice later.
        rows.append('| `%s` | %s | %s |'
                    % (name, summary(f).replace('|', '\\|'), mark))
    return '\n'.join(rows)


def main():
    want = BEGIN + '\n\n' + table() + '\n\n' + END
    text = open(DOC).read()
    m = re.search(re.escape(BEGIN) + '.*?' + re.escape(END), text, re.S)
    if '--apply' in sys.argv:
        if m:
            text = text[:m.start()] + want + text[m.end():]
        else:
            text = text.rstrip('\n') + '\n\n' + want + '\n'
        open(DOC, 'w').write(text)
        print('%d scripts described in tools/README.md'
              % (len(table().split('\n')) - 2))
        return
    if not m:
        print('1 tables in tools/README.md disagree with this directory '
              '(no generated block; run --apply)')
        sys.exit(1)
    if m.group(0) != want:
        # Say which rows, not just that something moved: "the table is stale"
        # sends the reader to diff it by eye, which is the work this replaces.
        have = set(m.group(0).split('\n'))
        for line in want.split('\n'):
            if line.startswith('|') and line not in have:
                print('  missing or changed: %s' % line[:96])
        for line in m.group(0).split('\n'):
            if line.startswith('|') and line not in set(want.split('\n')):
                print('  no longer true:     %s' % line[:96])
        print('1 tables in tools/README.md disagree with this directory')
        sys.exit(1)
    print('0 tables in tools/README.md disagree with this directory')


if __name__ == '__main__':
    main()
