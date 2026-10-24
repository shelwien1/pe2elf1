#!/usr/bin/env python3
"""adts2loas.py in.aac out.loas version subframes otherbits smc_interval [alignones]: ADTS (or AUS1, for
ER AAC) -> LOAS/LATM with audioMuxVersion 0/1, numSubFrames+1 AUs per AudioMuxElement, otherData, crcCheck,
ascLen fill bits (v1)."""
import sys, random
d = open(sys.argv[1], 'rb').read(); out = sys.argv[2]
ver, nsub, obits, interval = int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
ones = len(sys.argv) > 7
rnd = random.Random(1)
p = 0; aus = []; hdr = None; ascb = None
def asc_bits(b):
    """AudioSpecificConfig bit length (GA without PCE, ELD without LD-SBR)"""
    bits = ''.join(format(c, '08b') for c in b); pos = 0
    def get(n):
        nonlocal pos; v = int(bits[pos:pos + n] or '0', 2); pos += n; return v
    aot = get(5); aot = 32 + get(6) if aot == 31 else aot
    if get(4) == 15: get(24)
    get(4)
    if aot == 39:
        get(1)
        assert get(3) == 0 and get(1) == 0
        while get(4):
            n = get(4); n += get(8) if n == 15 else 0; n += get(16) if n == 15 + 255 else 0; get(8 * n)
        get(2)
    else:
        get(1)
        if get(1): get(14)
        ext = get(1)
        if ext:
            if aot in (17, 23): get(3)
            get(1)
        if aot in (17, 23): get(2)
    return pos
if d[:4] == b'AUS1':
    n = d[4] | d[5] << 8; ascb = d[6:6 + n]; p = 6 + n
    while p + 4 <= len(d):
        n = int.from_bytes(d[p:p + 4], 'little'); aus.append(d[p + 4:p + 4 + n]); p += 4 + n
    ascn = asc_bits(ascb)
while ascb is None and p + 7 <= len(d):
    if d[p] != 0xff or (d[p+1] & 0xf6) != 0xf0: p += 1; continue
    pa = d[p+1] & 1; flen = ((d[p+3] & 3) << 11) | (d[p+4] << 3) | (d[p+5] >> 5)
    if hdr is None: hdr = d[p:p+7]
    aus.append(d[p + (7 if pa else 9):p + flen]); p += flen
if ascb is None:
    prof = hdr[2] >> 6; sfi = (hdr[2] >> 2) & 15; ch = ((hdr[2] & 1) << 2) | (hdr[3] >> 6)
    ascn = 16
class BW:
    def __init__(s): s.b = []
    def put(s, v, n):
        for i in range(n - 1, -1, -1): s.b.append((v >> i) & 1)
    def bytes(s): return bytes(int(''.join(map(str, s.b[i:i+8])), 2) for i in range(0, len(s.b), 8))
def latm_value(w, v, nb): w.put(nb, 2); w.put(v, 8 * (nb + 1))
def asc(w):
    if ascb is not None:
        for i in range(ascn): w.put((ascb[i >> 3] >> (7 - (i & 7))) & 1, 1)
        return
    w.put(prof + 1, 5); w.put(sfi, 4); w.put(ch, 4); w.put(0, 3)
res = bytearray(); k = 0; fr = 0
while k + nsub + 1 <= len(aus):
    w = BW()
    same = 0 if fr % interval == 0 else 1
    w.put(same, 1)
    if not same:
        w.put(ver, 1)
        if ver: w.put(0, 1); latm_value(w, 0xff, 0)       # audioMuxVersionA, taraBufferFullness
        w.put(1, 1); w.put(nsub, 6); w.put(0, 4); w.put(0, 3)
        if ver:
            latm_value(w, ascn + 5, 1); asc(w); w.put(0b10101, 5)  # ascLen with 5 fill bits, 2-byte value
        else:
            asc(w)
        w.put(0, 3); w.put(0xff, 8)                        # frameLengthType 0, latmBufferFullness
        w.put(1 if obits else 0, 1)
        if obits:
            if ver: latm_value(w, obits, 1)
            else: w.put(1, 1); w.put(obits >> 8, 8); w.put(0, 1); w.put(obits & 255, 8)
        w.put(1, 1); w.put(0x5a, 8)                         # crcCheckPresent, crcCheckSum
    for j in range(nsub + 1):
        au = aus[k + j]; n = len(au)
        while n >= 255: w.put(255, 8); n -= 255
        w.put(n, 8)
        for c in au: w.put(c, 8)
    if obits:
        for _ in range(obits): w.put(rnd.randrange(2), 1)
    while len(w.b) % 8: w.b.append(1 if ones else 0)
    body = w.bytes()
    res += bytes([0x56, 0xe0 | (len(body) >> 8), len(body) & 255]) + body
    k += nsub + 1; fr += 1
open(out, 'wb').write(res)
