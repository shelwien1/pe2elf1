#!/usr/bin/env python3
"""Read a build log, or say plainly that it is about another file.

    python3 tools/buildlog.py warn.log subs1.hpp

    import buildlog
    rows, note = buildlog.read('warn.log', path)   # note says why, if it is stale
    rows = buildlog.rows('warn.log', path)         # or prints and exits 0

`build.sh` stamps `warn.log` and `strict.log` with `cksum` of the source they
were built from.  The stamp is the whole point: the line numbers in a log mean
nothing against any other version of the file, and mtime cannot tell you --
a `cp` of the source is newer than a log that describes it exactly, and a
rebuild for some other reason makes a stale log look fresh.

`resign.py` learned this the expensive way, and wrote it down:

    Running against a stale one retypes whatever now sits on those lines: it
    applied 17 changes to the wrong locals and put the warning count *up* by
    43, which the ratchet caught and the streams did not.

Three other tools read the same logs and did not check.  `explicitcmp.py`
filtered by the path's basename, which a copy of `subs1.hpp` satisfies whatever
its contents; `retype_locals.py` matched `^subs1\\.hpp:` as a literal, so it
never asked which `subs1.hpp`.  Both would apply a stale log's line numbers to
whatever now sits on them.  `resign_group.py` did check, with its own copy of
the code -- and one implementation checked in four places is how the fourth
comes to be missing.

A file the log is not about is not an error.  `sweep.sh` hands every tool a
copy, and a tool with nothing to say should say so and exit 0.
"""
import os
import sys

STAMP = '# subs1.hpp '


def read(log, path):
    """(lines, note).  `note` is empty when the log is about `path`.

    An unstamped log counts as stale: the stamp is cheap to add and its absence
    used to mean no check at all.

    The note is returned rather than printed, and rather than exited on,
    because of what the counting tools' last line has to say.  `sweep.sh`
    requires it to contain a zero; a tool that prints "nothing to say" and
    exits fails the sweep for the wrong reason, and one that prints a bare
    "0 sites" against a stale log has told the exact lie this file exists to
    stop.  So the caller puts the note *in* its summary: `0 sites (warn.log
    describes another file)` is both a zero and the reason for it.
    """
    try:
        out = open(log).read().split('\n')
    except OSError:
        return [], '%s is not there' % log
    have = os.popen('cksum < %s' % path).read().strip()
    stamp = next((r for r in out if r.startswith(STAMP)), None)
    if stamp is None or stamp[len(STAMP):].strip() != have:
        return [], '%s describes another file' % log
    return out, ''


def rows(log, path, quiet=False):
    """`read`, for a caller that stops rather than reports."""
    out, note = read(log, path)
    if note and not quiet:
        print('nothing to say about %s: %s' % (path, note))
        raise SystemExit(0)
    return out


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    got, note = read(sys.argv[1], sys.argv[2])
    print('%d lines of %s apply to %s%s'
          % (len(got), sys.argv[1], sys.argv[2], note and ' -- ' + note))


if __name__ == '__main__':
    main()
