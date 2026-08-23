#!/usr/bin/env python3
# f2idx.py -- write a float32 constant as an IDX Number line plus the scale the
# consumer divides by.
#
# Every float32 value is exactly m * 2^-k for some integer m < 2^24, so a
# parameter can go into a .idx as that integer and come back through
#
#   static const float x = float(P2_x) / float(1<<k);       (k <= 30)
#   static const float x = float(P2_x) * idx_exp2(-k);      (any k)
#
# with no rounding at all: the division is by a power of two, float(m) is exact
# for m < 2^24, and the product is the original float back.  That is what makes
# the port bit-exact -- a coarse fixed-point scale would move every rate and
# every NLMS seed by a fraction of a per cent and quietly change the stream.
#
# Negative values get a bias: the .idx carries m + bias and the consumer
# subtracts it, the convention coder0.cpp uses for its signed knobs.
#
#   python3 IDX/f2idx.py 0.0024 0.19 -0.05
import struct
import sys


def dyadic(v):
    """Return (m, k) with v == m * 2**-k exactly, m integer, |m| < 2**24."""
    f = struct.unpack('f', struct.pack('f', v))[0]
    if f == 0.0:
        return 0, 0
    m, k = f, 0
    while m != int(m):
        m *= 2.0
        k += 1
        if k > 149:
            raise ValueError('not dyadic: %r' % v)
    m = int(m)
    while m % 2 == 0 and k > 0:
        m //= 2
        k -= 1
    assert abs(m) < (1 << 24), (v, m, k)
    return m, k


def bits(n):
    return format(n, 'b') if n else '0'


def line(name, v, bias=0, mult=1):
    m, k = dyadic(v)
    q = m + bias
    assert q >= 0, (name, v, bias)
    return 'Number %-10s %d,0!%s' % (name + ',', mult, bits(q)), k


if __name__ == '__main__':
    for a in sys.argv[1:]:
        v = float(a)
        m, k = dyadic(v)
        print('%-14s = %8d * 2^-%-2d  ->  %s' % (a, m, k, line('X', v)[0]))
