#!/usr/bin/env python3
"""Recover which address in BMF.exe each named function came from.

    python3 tools/addrmap.py bmf.cpp > tools/addrmap.txt

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

Every pair is checked against the unit before it is printed: the new name has
to be there and the old one has to be gone.  Names with no recorded address are
listed at the end rather than left to look accounted for.

Two things this asks git for are not where a reader would expect, and both are
why it answered "0 mapped" for a whole round while looking like it worked.  The
renames are in `003/subs1.hpp`'s log, not in the path of any file that exists
today; and those commits are not ancestors of this branch, because the tree was
imported into a fresh line of history -- 79 commits here, 748 in the
repository.  So the searches name the donor path and pass `--all`.  See
`HISTORY` below.
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


# The path the renames were made on.  It is not the path the code is at today
# and there is no way to derive one from the other: these bodies were recovered
# and renamed in `003/subs1.hpp` over 468 commits, and the tree they live in
# now arrived in a single commit as 37 files.  So `git log -S` over any of
# *those* paths finds nothing at all -- which is not distinguishable, from
# inside this tool, from a name whose rename was never recorded.
#
# That is what happened.  This asked the history of the file it was handed, the
# file it was handed became `009/bmf.cpp`, and it printed "0 mapped" under a
# header saying the map was recovered from the commits that made each rename.
# `sweep.sh` exempts this tool from the every-count-is-zero rule -- it reports a
# map rather than a count -- so an empty map was the one shape of wrong answer
# nothing in the harness could see.  `unnamed.py` then reported itself not
# applicable for want of a map, and that is a check that stopped running.
#
# The names come from the unit as it stands.  The history comes from here.
HISTORY = '003/subs1.hpp'


def repo_path(path):
    """The repository root, and the path whose log holds the renames.

    A path outside any repository is not an error: `sweep.sh` and `proven.sh`
    both hand every tool a copy in a temp directory, and this one answered them
    with a `CalledProcessError` instead of a map.  So the root falls back to the
    working directory.

    The second half of the answer is `None` when `HISTORY` is not in this
    repository at all, which is a different thing from a name with no address
    and is reported as one.
    """
    full = os.path.abspath(path)
    try:
        root = git(['rev-parse', '--show-toplevel'],
                   os.path.dirname(full)).strip()
    except (subprocess.CalledProcessError, OSError):
        root = git(['rev-parse', '--show-toplevel'], os.getcwd()).strip()
    try:
        seen = git(['log', '--format=%H', '-1', '--all', '--', HISTORY],
                   root).strip()
    except (subprocess.CalledProcessError, OSError):
        seen = ''
    return root, (HISTORY if seen else None)


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
            revs = git(['log', '--format=%H', '--all', '-S', '__%s(' % name,
                        '--', rel], root).split()
            # Only if the first search found nothing: a body that became a
            # method is `Type::name(` today and `__name(` in every revision
            # before that, so the second search is what finds the commit that
            # moved it -- and running it for the other ninety doubled the
            # slowest tool in the directory for no answers.
            if not revs:
                revs = git(['log', '--format=%H', '--all',
                            '-S', '::%s(' % name, '--', rel], root).split()
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
    # `--all`, and that is the second half of the same repair as `HISTORY`.
    # The renaming commits are not ancestors of this branch: the tree was
    # imported into a fresh line of history, so `git log` walking HEAD sees 79
    # of this repository's 748 commits and none of the 468 that touch the file
    # the renames were made on.  Both searches here have to ask every ref.
    log = git(['log', '--format=%B', '--all'], root)
    out = {}
    for addr, name in PAIR.findall(log):
        out.setdefault(name, addr)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'bmf.cpp'
    root, rel = repo_path(path)
    if rel is None:
        print('not applicable: %s, where these bodies were renamed, is not in '
              'this repository' % HISTORY)
        return
    # The unit, not the file.  `bmf.cpp` is an include list with two bodies in
    # it, and reading it alone is how "0 mapped" reads as an answer rather than
    # as a tool that has not been given the program.  `defs` also opens bodies
    # at depth, so the sixteen that have become methods are here too -- under
    # their bare name, which is the spelling the map is keyed on.
    lines, _origin = structs.splice(path)
    text = '\n'.join(lines)
    defined = {n for _a, _b, n, sig, _d in structs.defs(lines)
               if 'static inline' not in sig}
    # The prefix is not part of a name here.  A body is `f`, `__f` or `T::f`
    # depending on when it was last touched, and the map's keys never carried
    # the prefix in the first place.
    defined = {n.lstrip('_') for n in defined}

    pairs, rejected = {}, []
    for name, addr in from_log(root).items():
        if name not in defined:
            continue
        if 'sub_%s' % addr in text:
            rejected.append((name, addr))
            continue
        pairs[name] = addr

    unmapped = {n for n in defined if not n.startswith('sub_')} - set(pairs)
    for name, addr in from_commits(sorted(unmapped), root, rel).items():
        if 'sub_%s' % addr not in text:
            pairs[name] = addr

    # What the previous run of this tool wrote, if it is beside us.  Two
    # different things can be read off it and only one of them is an error.
    #
    # A name both agree on and give *different* addresses to is a defect in one
    # of them and the run should not be trusted -- so it is reported and the
    # exit is non-zero.  Zero of the 64 in common disagreed the first time this
    # comparison was made, which is what says the re-derivation is sound.
    #
    # A name only the old map has is not a defect.  This tool follows a rename
    # through `HISTORY`'s log, and that log ends at the import: ten bodies have
    # been renamed or merged in the tree since -- `alt_p2_filter` became
    # `NbRow::predict` an hour before this was written -- and no `git log -S`
    # over the donor path reaches them.  Their addresses are still facts.  They
    # are kept below the map rather than in it, because a map that names a body
    # this tree does not have is one `unnamed.py` correctly refuses to use, and
    # refusing it is what silently turned that check off.
    # The commented block below is read back as well as written: an orphan that
    # stayed an orphan has to survive the next regeneration, and a parser that
    # only sees the live entries would drop one on every run.
    old = {}
    try:
        for l in open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   'addrmap.txt')):
            m = re.fullmatch(r'(?:#\s+)?(\w+)\s+0x00([0-9A-F]{6})\s*', l)
            if m:
                old[m.group(1)] = m.group(2)
    except IOError:
        pass
    clash = sorted(n for n in set(old) & set(pairs) if old[n] != pairs[n])
    orphan = sorted(set(old) - set(pairs), key=old.get)

    print('# name -> the address of the body in BMF.exe it was decompiled from.')
    print('# Recovered by tools/addrmap.py from the commits that made each')
    print('# rename; see ALGORITHM.md section 8.')
    for name in sorted(pairs, key=lambda n: pairs[n]):
        print('%-26s 0x00%s' % (name, pairs[name]))
    if orphan:
        print('#')
        print('# Bodies renamed since the tree was imported, which is past the')
        print('# end of the history this is recovered from.  The address is')
        print('# still the address; the name is the last one the chain proves.')
        for name in orphan:
            print('#   %-24s 0x00%s' % (name, old[name]))

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
    missing = sorted({n for n in defined
                      if not n.startswith('sub_')
                      and not re.search(r'_[0-9A-F]{6}$', n)}
                     - set(pairs) - ours)
    print('\n# %d mapped.  %d named bodies have no recorded address:'
          % (len(pairs), len(missing)))
    for i in range(0, len(missing), 4):
        print('#   ' + '  '.join('%-24s' % m for m in missing[i:i + 4]).rstrip())
    if rejected:
        print('\n# rejected, the old name is still in the file: %s'
              % ', '.join('%s/%s' % r for r in rejected))
    if clash:
        print('\n# FAIL: %d names the previous map gives another address:'
              % len(clash))
        for n in clash:
            print('#   %-24s was 0x00%s, now 0x00%s' % (n, old[n], pairs[n]))
        sys.exit(1)


if __name__ == '__main__':
    main()
