#!/usr/bin/env python3
# mkcorpus.py -- (re)assemble corpus.p from its hand-written prelude (arch/vars,
# tables, addressing + prefix submatches) and the generated instruction body.
#
# The legacy/x87/MMX/SSE insn{} rules are too numerous and too symmetric to hand
# maintain, so gen_rules.py emits them and this script splices them between the
# prelude (everything up to `submatch insn {`) and the entry point (`submatch
# main ...`). Idempotent: it reads the current corpus.p for the stable parts, so
# running it after editing gen_rules.py refreshes only the instruction body.
#
#   python3 mkcorpus.py        # rewrites corpus.p in place

import subprocess, sys

PATH = 'corpus.p'
src = open(PATH).read().splitlines(keepends=True)

def find(pred):
    for i, l in enumerate(src):
        if pred(l):
            return i
    raise SystemExit("mkcorpus: marker not found in %s" % PATH)

i_insn = find(lambda l: l.startswith('submatch insn'))
i_main = find(lambda l: l.startswith('submatch main'))
prelude = ''.join(src[:i_insn])
tail    = ''.join(src[i_main:])

body = subprocess.run([sys.executable, 'gen_rules.py'],
                      capture_output=True, text=True, check=True).stdout

open(PATH, 'w').write(prelude + 'submatch insn {\n' + body + '\n}\n\n' + tail)
n = len(open(PATH).read().splitlines())
print("mkcorpus: wrote %s (%d lines, %d insn rules)" % (PATH, n, body.count('=>')))
