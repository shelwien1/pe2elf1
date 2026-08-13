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
import difflib
import os
import re
import subprocess
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

# a closing brace with a definition glued to it -- `}int32_t __f(...)` --
# is a definition, and older revisions of the file are full of them
# A definition, by the name it answers to.  Two spellings now: `__name(` for
# every body recovered from the binary, and `Type::name(` for the sixteen that
# have since become methods of the record their `_this` pointed at.  Missing
# the second made `real_bodies` return a shorter list after the methodising
# commit than before it, which is exactly the "the commit did more than rename"
# case below -- so the chain broke and five bodies were reported as having no
# recorded address, which `unnamed.py` would then have stopped checking.
DEF = re.compile(r'^\}?\s?[A-Za-z_].*?(?:\b__|\b[A-Za-z_]\w*::)'
                 r'([A-Za-z_][A-Za-z0-9_]*)\s*\(')
def git(args, root=None):
    return subprocess.check_output(['git'] + args, cwd=root,
                                   stderr=subprocess.DEVNULL).decode('utf8',
                                                                     'replace')


def repo_path(path):
    """The repository root, and the file's path inside it.

    `git log -S ... -- 003/subs1.hpp` finds nothing when it is run from inside
    003, and finding nothing looks exactly like a name with no recorded
    address.  Neither the root nor the path is assumed.

    A path outside any repository is not an error: `sweep.sh` and `proven.sh`
    both hand every tool a copy in a temp directory, and this one answered them
    with a `CalledProcessError` instead of a map.  The names come from the file
    it was given and the history comes from the tree it is running in, which is
    what the join needs; so the root falls back to the working directory and
    the path inside it to `subs1.hpp` in that tree.
    """
    full = os.path.abspath(path)
    try:
        root = git(['rev-parse', '--show-toplevel'],
                   os.path.dirname(full)).strip()
        return root, os.path.relpath(full, root)
    except (subprocess.CalledProcessError, OSError):
        root = git(['rev-parse', '--show-toplevel'], os.getcwd()).strip()
        return root, os.path.relpath(os.path.abspath('subs1.hpp'), root)


PAIR = re.compile(r'\bsub_([0-9A-F]{6})\s*(?:->|=>|=|→)\s*'
                  r'([a-z_][a-z0-9_]*)')


def real_bodies(text):
    """Every non-shim definition, in file order."""
    return [DEF.match(l).group(1) for l in text.split('\n')
            if DEF.match(l) and not l.lstrip().startswith('static inline')]


def from_commits(names, root, rel):
    """Pair names with addresses across the commit that introduced each.

    `tools/rename.py --funcs` substitutes a token; it does not reorder or
    remove anything.  So the list of real bodies in file order is the same list
    before and after, and the k-th entry before a renaming commit is the k-th
    entry after it.  Equal lengths is the check that the commit did only that,
    and any commit that fails it is skipped rather than guessed through.
    """
    out = {}
    for name in names:
        out.update(walk(name, root, rel, set()))
    return out


def walk(name, root, rel, seen):
    """Follow a name back to an address, through however many renames.

    A body can be renamed twice -- `sub_412B10` to `symbol_list` to
    `encode_symbol_list` -- and only the first of those hops ends at an
    address.  Stopping at the first hop reports the name as unresolved even
    though the chain is complete.
    """
    if name in seen:
        return {}
    seen.add(name)
    for _ in (0,):
        try:
            revs = git(['log', '--format=%H', '-S', '__%s(' % name, '--',
                        rel], root).split()
            # Only if the first search found nothing: a body that became a
            # method is `Type::name(` today and `__name(` in every revision
            # before that, so the second search is what finds the commit that
            # moved it -- and running it for the other ninety doubled the
            # slowest tool in the directory for no answers.
            if not revs:
                revs = git(['log', '--format=%H', '-S', '::%s(' % name, '--',
                            rel], root).split()
        except subprocess.CalledProcessError:
            return {}
        for rev in reversed(revs):
            try:
                after = git(['show', '%s:%s' % (rev, rel)], root)
                before = git(['show', '%s^:%s' % (rev, rel)], root)
            except subprocess.CalledProcessError:
                continue
            a, b = real_bodies(after), real_bodies(before)
            if len(a) == len(b):
                blocks = [(b, a)]
            else:
                # the commit did more than rename.  Line the two lists up and
                # take only the stretches where the same number of bodies
                # changed on each side; anywhere else the k-th does not answer
                # to the k-th and there is nothing to read off.
                sm = difflib.SequenceMatcher(None, b, a, autojunk=False)
                blocks = [(b[i1:i2], a[j1:j2])
                          for tag, i1, i2, j1, j2 in sm.get_opcodes()
                          if tag == 'replace' and i2 - i1 == j2 - j1]
            for olds, news in blocks:
                for n, o in zip(news, olds):
                    if n != name or o == name:
                        continue
                    if o.startswith('sub_'):
                        return {name: o[4:]}
                    back = walk(o, root, rel, seen)
                    if o in back:
                        return {name: back[o]}
    return {}


def from_log(root):
    log = git(['log', '--format=%B'], root)
    out = {}
    for addr, name in PAIR.findall(log):
        out.setdefault(name, addr)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    root, rel = repo_path(path)
    lines = open(path).read().split('\n')
    text = '\n'.join(lines)
    defined = {n for _, _, n, sig in structs.bodies(lines)
               if 'static inline' not in sig}

    pairs, rejected = {}, []
    for name, addr in from_log(root).items():
        if '__' + name not in defined:
            continue
        if 'sub_%s' % addr in text:
            rejected.append((name, addr))
            continue
        pairs[name] = addr

    unmapped = {n.lstrip('_') for n in defined
                if not n.startswith('__sub_')} - set(pairs)
    for name, addr in from_commits(sorted(unmapped), root, rel).items():
        if 'sub_%s' % addr not in text:
            pairs[name] = addr

    print('# name -> the address of the body in BMF.exe it was decompiled from.')
    print('# Recovered by tools/addrmap.py from the commits that made each')
    print('# rename; see ALGORITHM.md section 8.')
    for name in sorted(pairs, key=lambda n: pairs[n]):
        print('%-26s 0x00%s' % (name, pairs[name]))

    # Scaffolding this tree wrote: none of it came out of BMF.exe, so none of
    # it is missing.  `alignas` and `attribute__` used to be here too -- two
    # names `structs.bodies` invented out of an array initialiser and an
    # attribute -- and naming them here was working around a parser bug in
    # another file rather than fixing it.  REFACTORING9.md section 45 fixed it,
    # and dropping them from this list is how a workaround stops outliving the
    # thing it worked around.
    ours = {'main', 'bmf_addr', 'bmf_data_relocate',
            'bmf_set_denormal_mode', 'bmf_compress', 'bmf_decompress',
            # A method this project wrote rather than moved: `SymList::rescale`
            # is the fifty-three lines `code_symbol` and `add_weight` both had
            # inline.  It has no address because it was never one body in
            # BMF.exe -- it was two copies of part of two.  Named here rather
            # than left in the missing list, where it would read as a body
            # whose address nobody has found.
            'rescale'}
    # A name that still ends in its address -- `exit_402E40`, the CRT entry
    # nothing renamed -- is not a name with no recorded address.  The record is
    # the name.  `__sub_XXXXXX` is the same case with the original prefix.
    missing = sorted({n.lstrip('_') for n in defined
                      if not n.startswith('__sub_')
                      and not re.search(r'_[0-9A-F]{6}$', n)}
                     - set(pairs) - ours)
    print('\n# %d mapped.  %d named bodies have no recorded address:'
          % (len(pairs), len(missing)))
    for i in range(0, len(missing), 4):
        print('#   ' + '  '.join('%-24s' % m for m in missing[i:i + 4]).rstrip())
    if rejected:
        print('\n# rejected, the old name is still in the file: %s'
              % ', '.join('%s/%s' % r for r in rejected))


if __name__ == '__main__':
    main()
