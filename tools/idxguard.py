#!/usr/bin/env python3
# idxguard.py -- wrap each parameter idx2inc.pl emitted in an #ifndef, in
# place, so a sweep can pin it from the command line:
#
#   pdesc( B0_NW_w_, 1, "..." );            #ifndef B0_NW_w
#   static const int B0_NW_w = ...;   ->    pdesc( B0_NW_w_, 1, "..." );
#                                           static const int B0_NW_w = ...;
#                                           #endif
#
# With the guard, -DB0_NW_w=1024 replaces the whole declaration by a literal
# (which is also what makes sweep.py's model-shape search possible); without
# it, -D turns the declarator into a number and the file will not compile.
# The generated file is otherwise untouched, so it stays a build input that
# `GCIDX=1 ./gc.sh` can rewrite from the .idx at any time.
#
#   tools/idxguard.py MOD/sh_model-B0_h.inc
import re, sys

DECL = re.compile(r'^(?:pdesc|mdesc)\(\s*(\w+?)_,')
CONST = re.compile(r'^static const (?:int|word\*) (\w+)\s*=')

for path in sys.argv[1:]:
    out, pend = [], []
    for line in open(path).read().split('\n'):
        m = DECL.match(line)
        if m:
            pend = [line]
            continue
        m = CONST.match(line)
        if m and pend:
            out += ['#ifndef ' + m.group(1)] + pend + [line, '#endif']
            pend = []
            continue
        if m:                                   # Const 1: already a literal
            out += ['#ifndef ' + m.group(1), line, '#endif']
            continue
        out += pend + [line]
        pend = []
    out += pend
    open(path, 'w').write('\n'.join(out))
    print('guarded %s' % path)
