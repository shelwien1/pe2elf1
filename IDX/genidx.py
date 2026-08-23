#!/usr/bin/env python3
# genidx.py -- one-time generator: turn the constants that are currently written
# as literals in the sources into an IDX module, and emit the consumer glue that
# reads them back.
#
# Run once per module while porting.  After that the .idx is the source of truth
# and this script is only useful for adding a new block of parameters.
#
#   python3 IDX/genidx.py P2 > IDX/bmf-P2.idx
#
# Floats go in as exact dyadic integers (see f2idx.py), so the generated build
# reproduces the pre-port stream byte for byte.
import sys

sys.path.insert(0, 'IDX')
from f2idx import dyadic


class Module:
    def __init__(self, prefix, note):
        self.prefix = prefix
        self.note = note
        self.out = []
        self.glue = []
        self.const = None

    # ---- .idx emission -------------------------------------------------
    def head(self):
        self.out.append('')
        self.out.append('Prefix %s' % self.prefix)
        self.out.append('Debug 1')
        self.out.append('')

    def section(self, title, const):
        self.out.append('')
        self.out.append('# %s' % title)
        if const != self.const:
            self.out.append('Const %d' % const)
            self.const = const
        self.out.append('')

    def num(self, name, value, base=0, mult=1, comment=''):
        """An integer parameter: value == (pattern + base) * mult."""
        q, r = divmod(value - base * mult, mult)
        assert r == 0 and q >= 0, (name, value, base, mult)
        pat = format(q, 'b') if q else '0'
        c = ('   # %s' % comment) if comment else ''
        self.out.append('Number %-12s %d,%d!%s%s' % (name + ',', mult, base, pat, c))

    def flt(self, name, value, bias=0, comment=''):
        """A float parameter: stored as the exact dyadic mantissa, plus a bias
        when the value is negative.  Returns the scale exponent."""
        m, k = dyadic(value)
        q = m + bias
        assert q >= 0, (name, value, bias)
        pat = format(q, 'b') if q else '0'
        c = '   # %s' % (comment or repr(value))
        self.out.append('Number %-12s %d,%d!%s%s' % (name + ',', 1, 0, pat, c))
        return k

    def text(self, s=''):
        self.out.append(s)

    def dump(self):
        return '\n'.join(self.out) + '\n'
