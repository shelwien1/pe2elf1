#!/usr/bin/env python3
"""Recover which address in BMF.exe each named function came from.

    python3 tools/addrmap.py subs1.hpp > tools/addrmap.txt

`ALGORITHM.md` opens by saying function names are the donor's addresses, and
that was true when everything was `__sub_402FE0`.  Every rename since is a
small loss of that: the name says what the function does, and nothing says
where it was.  Both are worth having.

The source is the git log, because every rename was made by `tools/rename.py`
and its output was pasted into the commit that made it.  That is a record of
what happened rather than a reconstruction of it, which matters here: matching
bodies by their constants -- the obvious alternative, and the first thing tried
-- cannot tell an encoder from its decoder, because the twins test the same
numbers.  It confidently mapped `encode_context_bit` and `decode_context_bit`
to one address.

Every pair is checked against the file before it is printed: the new name has
to be there and the old one has to be gone.  Names with no recorded address are
listed at the end rather than left to look accounted for.
"""
import re
import subprocess
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

PAIR = re.compile(r'\bsub_([0-9A-F]{6})\s*(?:->|=>|=|→)\s*'
                  r'([a-z_][a-z0-9_]*)')


def from_log():
    log = subprocess.check_output(['git', 'log', '--format=%B'],
                                  stderr=subprocess.DEVNULL).decode('utf8',
                                                                    'replace')
    out = {}
    for addr, name in PAIR.findall(log):
        out.setdefault(name, addr)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    text = '\n'.join(lines)
    defined = {n for _, _, n, sig in structs.bodies(lines)
               if 'static inline' not in sig}

    pairs, rejected = {}, []
    for name, addr in from_log().items():
        if '__' + name not in defined:
            continue
        if 'sub_%s' % addr in text:
            rejected.append((name, addr))
            continue
        pairs[name] = addr

    print('# name -> the address of the body in BMF.exe it was decompiled from.')
    print('# Recovered by tools/addrmap.py from the commits that made each')
    print('# rename; see ALGORITHM.md section 8.')
    for name in sorted(pairs, key=lambda n: pairs[n]):
        print('%-26s 0x00%s' % (name, pairs[name]))

    # scaffolding this tree wrote, and two names `bodies()` picks up from
    # attribute syntax -- neither came out of BMF.exe, so neither is missing
    ours = {'alignas', 'attribute__', 'main', 'bmf_addr', 'bmf_data_relocate',
            'bmf_set_denormal_mode', 'bmf_compress', 'bmf_decompress'}
    missing = sorted({n.lstrip('_') for n in defined
                      if not n.startswith('__sub_')} - set(pairs) - ours)
    print('\n# %d mapped.  %d named bodies have no recorded address:'
          % (len(pairs), len(missing)))
    for i in range(0, len(missing), 4):
        print('#   ' + '  '.join('%-24s' % m for m in missing[i:i + 4]).rstrip())
    if rejected:
        print('\n# rejected, the old name is still in the file: %s'
              % ', '.join('%s/%s' % r for r in rejected))


if __name__ == '__main__':
    main()
