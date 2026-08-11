#!/usr/bin/env python3
"""What does a tool actually read?

    python3 tools/reads.py tools/unspill.py subs1.hpp

`proven.sh` replays each tool against old revisions of `subs1.hpp` and reports
the ones whose answer never moves, on the reasoning that such a tool "either
always finds nothing or never looks".  It cannot tell those two apart, and the
second is the failure it was written to catch -- `shape.py` ignored the path it
was given and read the working copy, so it answered identically for every
revision it was asked about.

That failure has more than one shape, and the replay is void for all of them:

  * `decast.py` compiles `bmf.cpp` from the working tree and keeps the warnings
    whose file matches the path's basename.  Handed `/tmp/s.hpp` it matches
    nothing, prints "no useless casts" and returns without opening the path at
    all.
  * `undef.py` reads `bmf.cpp` and ignores an argument that is not a `.cpp`,
    because `sweep.sh` hands every tool a copy of `subs1.hpp`.  `proven.sh`
    hands it nothing else, so it cannot move.
  * `explicitcmp.py` and `retype_locals.py` do open the path -- and join it
    against `warn.log`, whose line numbers are this build's.  Against a
    revision from four hundred commits ago the join lands on unrelated lines,
    so their zero is a property of the join and not of the file.

The probe is to run the tool in-process with `open` and `subprocess` spied, and
report every path it touches.  A tool whose only input is the file it was given
is one `proven.sh` can speak for; the rest are named, so the flat list says
which of its rows are evidence and which are the script's own reach.

The file is copied first, because several of these rewrite what they are given.
"""
import builtins
import os
import runpy
import shutil
import subprocess
import sys
import tempfile


def probe(tool, path):
    """(opened, ran, error) for one run of `tool` against a copy of `path`."""
    tmp = tempfile.mkdtemp()
    try:
        copy = os.path.join(tmp, os.path.basename(path))
        shutil.copy(path, copy)
        opened, ran = [], []
        real_open, real_run, real_popen = builtins.open, subprocess.run, subprocess.Popen
        here = os.path.dirname(os.path.abspath(tool))

        def spy_open(file, *a, **k):
            try:
                p = os.path.abspath(os.fspath(file))
                # A tool importing its neighbours is not reading input.
                if not p.startswith(here + os.sep):
                    opened.append(p)
            except TypeError:
                pass
            return real_open(file, *a, **k)

        def cmd(a):
            """The program name, however the call spelled the command."""
            if not a:
                return '?'
            c = a[0]
            return (c[0] if isinstance(c, (list, tuple)) and c else
                    str(c).split()[0] if str(c).split() else '?')

        def spy_run(*a, **k):
            ran.append(cmd(a))
            return real_run(*a, **k)

        def spy_popen(*a, **k):
            ran.append(cmd(a))
            return real_popen(*a, **k)

        builtins.open = spy_open
        subprocess.run, subprocess.Popen = spy_run, spy_popen
        argv, out, err = sys.argv, sys.stdout, sys.stderr
        sys.argv = [tool, copy]
        sink = real_open(os.devnull, 'w')
        sys.stdout = sys.stderr = sink
        failed = None
        try:
            runpy.run_path(tool, run_name='__main__')
        except SystemExit:
            pass
        except Exception as e:                                    # noqa: BLE001
            failed = '%s: %s' % (type(e).__name__, e)
        finally:
            builtins.open, subprocess.run, subprocess.Popen = (
                real_open, real_run, real_popen)
            sys.argv, sys.stdout, sys.stderr = argv, out, err
            sink.close()
        return copy, opened, ran, failed
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def verdict(tool, path):
    """One line: is the file it was given the whole of its input?

    Anything else it reads is read out of the working tree, which is what makes
    a replay against an old revision meaningless -- the tool joins the file it
    was handed against this build.
    """
    copy, opened, ran, failed = probe(tool, path)
    also = sorted(set(os.path.basename(p) for p in opened if p != copy))
    if sorted(set(ran)) == ['g++']:
        also.append('a compile of the working tree')
    elif ran:
        also.extend(sorted(set(ran)))
    if copy not in opened:
        return ('never opens the file it is given; its input is %s'
                % (', '.join(also) or 'somewhere else'))
    if also:
        return 'reads it, and %s' % ', '.join(also)
    if failed:
        return 'reads it (raised %s)' % failed
    return 'reads only what it is given'


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    print('%-22s %s' % (os.path.basename(sys.argv[1]), verdict(*sys.argv[1:3])))


if __name__ == '__main__':
    main()
