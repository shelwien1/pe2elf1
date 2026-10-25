#!/usr/bin/env python3
"""mp4adts.py in.m4a out.m4a [stsz] -- the first AAC track of an MP4 file
(moov box after the mdat, one track: ffmpeg's -f mp4) with an ADTS header
in front of each access unit, as some muxers wrongly store them: a test of
aacpack's ADTS frames as MP4 samples. With 'stsz', the access units stay as
they are and a few of the stsz box's entries are made wrong instead (the
units they name then do not parse where the table says)."""
import struct
import sys

d = open(sys.argv[1], 'rb').read()
CONT = {b'moov', b'trak', b'mdia', b'minf', b'stbl', b'udta', b'edts', b'dinf'}


def parse(s, e):
    out = []
    p = s
    while p + 8 <= e:
        n, t = struct.unpack('>I4s', d[p:p + 8])
        h = 8
        if n == 1:
            n, h = struct.unpack('>Q', d[p + 8:p + 16])[0], 16
        if n == 0:
            n = e - p
        out.append([t, parse(p + h, p + n) if t in CONT else d[p + h:p + n]])
        p += n
    return out


def ser(boxes):
    r = b''
    for t, c in boxes:
        body = ser(c) if isinstance(c, list) else c
        r += struct.pack('>I4s', 8 + len(body), t) + body
    return r


def find(boxes, t):
    for b in boxes:
        if b[0] == t:
            return b
        if isinstance(b[1], list):
            x = find(b[1], t)
            if x:
                return x


def asc_of(stsd):
    """the AudioSpecificConfig in the stsd's mp4a entry: its esds's DecoderSpecificInfo"""
    i = stsd.find(b'esds')
    p = i + 8  # (the box's type, its version and flags)
    def descr(p):
        tag, n, p = stsd[p], 0, p + 1
        for _ in range(4):
            b = stsd[p]
            p += 1
            n = n << 7 | (b & 0x7f)
            if not b & 0x80:
                break
        return tag, p, p + n
    tag, p, e = descr(p)  # ES_Descriptor
    fl = stsd[p + 2]
    p += 3 + (2 if fl & 0x80 else 0) + (2 if fl & 0x20 else 0)
    while p < e:
        tag, a, b = descr(p)
        if tag == 4:
            r = a + 13
            while r < b:
                t2, a2, b2 = descr(r)
                if t2 == 5:
                    return stsd[a2:b2]
                r = b2
        p = b


top = parse(0, len(d))
stsz, stco, stsc, stsd = find(top, b'stsz'), find(top, b'stco'), find(top, b'stsc'), find(top, b'stsd')
count = struct.unpack('>I', stsz[1][8:12])[0]
sizes = list(struct.unpack('>%dI' % count, stsz[1][12:12 + 4 * count]))
if len(sys.argv) > 3:
    for k in (5, 6, 100, count - 3):
        if 0 <= k < count:
            sizes[k] += 3
    stsz[1] = stsz[1][:12] + b''.join(struct.pack('>I', n) for n in sizes)
    open(sys.argv[2], 'wb').write(ser(top))
    sys.exit(0)
offs = struct.unpack('>%dI' % struct.unpack('>I', stco[1][4:8])[0], stco[1][8:])
runs = [struct.unpack('>III', stsc[1][8 + 12 * i:20 + 12 * i]) for i in range(struct.unpack('>I', stsc[1][4:8])[0])]
samples = []
k = 0
for ci, off in enumerate(offs):
    per = [r[1] for r in runs if ci + 1 >= r[0]][-1]
    for _ in range(per):
        samples.append(d[off:off + sizes[k]])
        off += sizes[k]
        k += 1
asc = asc_of(stsd[1])
aot, sfi, ch = asc[0] >> 3, ((asc[0] & 7) << 1) | (asc[1] >> 7), (asc[1] >> 3) & 15


def adts(n):
    L = n + 7
    return bytes([0xff, 0xf1, ((aot - 1) << 6) | (sfi << 2) | (ch >> 2), ((ch & 3) << 6) | (L >> 11),
                  (L >> 3) & 255, ((L & 7) << 5) | 0x1f, 0xfc])


new = [adts(len(s)) + s for s in samples]
stsz[1] = stsz[1][:12] + b''.join(struct.pack('>I', len(s)) for s in new)
stsc[1] = stsc[1][:4] + struct.pack('>IIII', 1, 1, len(new), 1)
ftyp = [b for b in top if b[0] == b'ftyp'][0]
moov = [b for b in top if b[0] == b'moov'][0]
stco[1] = stco[1][:4] + struct.pack('>II', 1, 8 + len(ftyp[1]) + 8)
open(sys.argv[2], 'wb').write(ser([ftyp, [b'mdat', b''.join(new)], moov]))
