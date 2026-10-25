"""adts.py -- what adts2adif.py and adts2loas.py share: the frames of an
ADTS stream, and a writer of bits."""


def frames(d):
    """each ADTS frame of d: its 7 header bytes and its raw_data_block's bytes
    (what is not a frame is skipped a byte at a time)"""
    p = 0
    while p + 7 <= len(d):
        if d[p] != 0xff or (d[p + 1] & 0xf6) != 0xf0:
            p += 1
            continue
        h = 7 if d[p + 1] & 1 else 9  # (protection_absent 0: a CRC follows)
        flen = ((d[p + 3] & 3) << 11) | (d[p + 4] << 3) | (d[p + 5] >> 5)
        if flen < h:  # (no frame is shorter than its header)
            p += 1
            continue
        yield d[p:p + 7], d[p + h:p + flen]
        p += flen


class BW:
    """bits, the first the most significant of the first byte"""

    def __init__(s):
        s.b = []

    def put(s, v, n):
        for i in range(n - 1, -1, -1):
            s.b.append((v >> i) & 1)

    def align(s, bit=0):
        while len(s.b) % 8:
            s.b.append(bit)

    def bytes(s):
        s.align()
        return bytes(int(''.join(map(str, s.b[i:i + 8])), 2) for i in range(0, len(s.b), 8))
