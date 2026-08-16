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
# Two smaller jobs keep `GCIDX=1 ./gc.sh` idempotent against the tracked
# file, so regenerating from the .idx produces no diff:
#
#   --banner-from OLD   carry the leading // comment block of the previous
#                       version across (idx2inc.pl emits none, and the
#                       banner is where the file says where it came from)
#   the empty module scaffold idx2inc.pl always closes with -- `struct X_T`
#                       with a zero _Size and empty Init/Quit -- is dropped;
#                       B0 declares parameters only, it has no tables.
#
#   tools/idxguard.py [--banner-from OLD] MOD/sh_model-B0_h.inc
import re, sys

def banner_of( path ):
    try: lines = open(path).read().split('\n')
    except OSError: return []
    out = []
    for line in lines:
        if line.startswith('//'): out.append(line)
        elif out or line.strip(): break
    return out

# `struct X_T { ... };` with nothing in it: no Table members, so no state.
SCAFFOLD = re.compile(
    r'\n*struct \w+_T \{.*?_Size = 0;\s*'
    r'void \w+_Init\( void \) \{\s*\}\s*'
    r'void \w+_Quit\( void \) \{\s*\}\s*\};\s*\Z', re.S )

banner = []
args = sys.argv[1:]
if args[:1] == ['--banner-from']:
    banner = banner_of( args[1] ); args = args[2:]

DECL = re.compile(r'^(?:pdesc|mdesc)\(\s*(\w+?)_,')
CONST = re.compile(r'^static const (?:int|word\*) (\w+)\s*=')

for path in args:
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
    text = SCAFFOLD.sub( '\n', '\n'.join(out) )
    if banner:
        body = text.split('\n')
        while body and (body[0].startswith('//') or not body[0].strip()):
            body.pop(0)
        text = '\n'.join( banner + body )
    open(path, 'w').write(text)
    print('guarded %s' % path)
