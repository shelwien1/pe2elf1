#!/usr/bin/env python3
"""Declare a run of frame members as the array the code walks it as.

    python3 tools/runarray.py bmf.cpp --list
    python3 tools/runarray.py bmf.cpp alt_model_p2_encode lpAddress plane

REFACTORING3.md §4.2's third category.  Hex-Rays recovered

    Obj11 *lpAddress;   Obj11 *v160;   Obj11 *v161;   Obj11 *v162;

as four unrelated members, and the body walks them:

    *(&lpAddress + n4++) = (Obj11 *)v8;          // fill, n4 < plane_count
    v13 = (Obj19 *)((int32_t)*(&lpAddress + n4_2 - 1));
    lpAddress_2 = (void **)*(&lpAddress + n4_4); // free

That is `Obj11 *plane[4]`, and saying so removes the `&`-walk, the cast and the
reason the frame has to stay one object all at once -- `defram.py` can lift
what is left once nothing takes an address into the middle of it.

The run is the base member plus every member after it of the same type; the
new array replaces all of them, so the frame keeps its size and its offsets.
Uses rewrite as

    *(&base + E)  ->  arr[E]          (&base)[E]  ->  arr[E]
     &base + E    ->  &arr[E]          base       ->  arr[0]
                                       member k   ->  arr[k]

and the gate decides, as always: the run length is read off the declarations,
not off the loop bound, so a body that walks further than its own run would
still be walking into whatever follows.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import defram                                                     # noqa: E402
import structs                                                    # noqa: E402


def alias_types(fr):
    """{member: the type its alias gives it}.

    Hex-Rays declared the four plane pointers as `Obj11 *`, `__m128 *`,
    `__m128 *`, `__m128 *` and then aliased all four as `Obj11 *`.  The member
    types disagree; the alias types are what the body sees, so the run is
    defined by those.
    """
    out = {}
    for x in fr['aliases']:
        ty = x['decl'][:-len(x['name'])].strip() if x['decl'].endswith(x['name']) \
            else x['decl']
        out[x['member']] = ty
    return out


def run_of(fr, base):
    """The base member and every following member the aliases give one type."""
    at = next((i for i, m in enumerate(fr['members']) if m['name'] == base), None)
    if at is None:
        return None
    ty = alias_types(fr)
    if base not in ty:
        return None
    out = []
    for m in fr['members'][at:]:
        if m['count'] or ty.get(m['name']) != ty[base]:
            break
        out.append(m)
    return out


def convert(lines, a, b, fr, base, name):
    run = run_of(fr, base)
    if not run or len(run) < 2:
        return None, '%s is not the base of a run' % base
    names = [m['name'] for m in run]
    ty = alias_types(fr)[base]
    idx = {n: i for i, n in enumerate(names)}

    alias = {x['member']: x for x in fr['aliases']}
    if any(n not in alias for n in names):
        return None, 'not every member of the run has an alias'

    edits = {}
    edits[run[0]['line']] = ['%s%s %s[%d];   // was %s'
                             % (run[0]['ind'], ty, name, len(run),
                                ', '.join(names))]
    for m in run[1:]:
        edits[m['line']] = []
    edits[alias[names[0]]['line']] = ['%s%s (&%s)[%d] = frame.%s;'
                                      % (alias[names[0]]['ind'], ty, name,
                                         len(run), name)]
    for n in names[1:]:
        edits[alias[n]['line']] = []

    skip = set(x['line'] for x in fr['aliases'])
    hit = 0
    for i in range(fr['close'] + 1, b + 1):
        if i in skip or i in edits:
            continue
        s = lines[i]
        old = s
        s = re.sub(r'\*\s*\(\s*&%s\s*\+\s*([^()]*?)\s*\)' % base,
                   lambda m: '%s[%s]' % (name, m.group(1)), s)
        s = re.sub(r'\(\s*&%s\s*\)\s*\[' % base, '%s[' % name, s)
        s = re.sub(r'&%s\s*\+\s*([A-Za-z_]\w*)' % base,
                   lambda m: '&%s[%s]' % (name, m.group(1)), s)
        for n in names:
            s = re.sub(r'\b%s\b' % n, '%s[%d]' % (name, idx[n]), s)
        if s != old:
            edits[i] = [s]
            hit += 1
    if not hit:
        return None, 'nothing referred to %s' % base

    out = []
    for i, l in enumerate(lines):
        out.extend(edits[i] if i in edits else [l])
    return out, None


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    if '--list' in sys.argv or len(args) < 3:
        # every frame member whose address is walked, and how long its run is
        seen = 0
        for a, b, nm, sig in structs.bodies(lines):
            fr = defram.parse(lines, a, b)
            if not fr:
                continue
            body = '\n'.join(lines[fr['close'] + 1:b + 1])
            for x in fr['aliases']:
                if not re.search(r'[*(]\s*&%s\s*[+)]' % x['name'], body):
                    continue
                run = run_of(fr, x['member'])
                print('%-26s %-16s run of %d  %s'
                      % (nm.lstrip('_'), x['name'], len(run) if run else 0,
                         run[0]['ty'] if run else '?'))
                seen += 1
        print('%d frame members have their address walked' % seen)
        return 0

    name = args[0] if args[0].startswith('__') else '__' + args[0]
    for a, b, nm, sig in structs.bodies(lines):
        if nm != name:
            continue
        fr = defram.parse(lines, a, b)
        if not fr:
            print('%s has no frame' % nm)
            return 1
        out, err = convert(lines, a, b, fr, args[1], args[2])
        if err:
            print('skipped: %s' % err)
            return 1
        open(path, 'w').write('\n'.join(out))
        print('%s: %s is now %s[]' % (nm.lstrip('_'), args[1], args[2]))
        return 0
    print('no function called %s' % name)
    return 1


if __name__ == '__main__':
    sys.exit(main())
