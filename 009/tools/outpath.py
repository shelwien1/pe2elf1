#!/usr/bin/env python3
"""Refuse to write a generator's output over something that is not its output.

Nearly every tool in this directory takes the file to *read* as its first
argument.  The `mk*.py` generators take the file to *write*, in exactly the
same position, so

    for t in tools/*.py; do python3 $t subs1.hpp; done

-- an entirely reasonable way to ask what every tool reports -- runs every
generator with `subs1.hpp` as its destination and leaves the decompilation as a
96x96 bitmap.  That happened.  It cost nothing because the file was committed a
minute earlier, which is not a safety property.

(The count is not written down here.  It said "all but four" and "three
generators" in the same paragraph, and there were four; a number in a docstring
is a measurement that stops being re-taken.  `ls tools/mk*.py` is the answer,
and `sweep.sh` skips exactly that glob.)

The guard is the weakest thing that would have stopped it: a generator writes a
`.bmp`, so a destination that is not named like one is a mistake, and a
directory that does not already exist is a mistake too.  Neither test knows
anything about what is in the file, and that is deliberate -- a check that
opened the destination and sniffed it would pass on an empty file and fail on a
`.bmp` that a previous run left half-written.  The name is the argument the
caller meant to type.
"""
import os
import sys


def bmp(argv, default):
    """The path a generator was asked to write, or exit if it cannot be one."""
    path = argv[1] if len(argv) > 1 else default
    if not path.lower().endswith('.bmp'):
        sys.exit('%s: refusing to write %r -- a generator writes a .bmp, and '
                 'its argument is the file to create, not one to read'
                 % (os.path.basename(argv[0]), path))
    d = os.path.dirname(path)
    if d and not os.path.isdir(d):
        sys.exit('%s: %r has no directory %r' % (os.path.basename(argv[0]), path, d))
    return path


def outdir(argv, default):
    """Same, for the one generator that writes several files into a directory."""
    d = argv[1] if len(argv) > 1 else default
    if not os.path.isdir(d):
        sys.exit('%s: refusing to write into %r -- it is not a directory, and '
                 'this tool creates files rather than reading one'
                 % (os.path.basename(argv[0]), d))
    return d
