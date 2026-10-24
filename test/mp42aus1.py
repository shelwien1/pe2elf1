#!/usr/bin/env python3
"""mp42aus1.py in.mp4 out.aus1 -- the first AAC track of an MP4 file as AUS1
(aac2tsv's format of raw access units: "AUS1", the AudioSpecificConfig's
length as le16 and its bytes, then each access unit's length as le32 and its
bytes), which aacpack parses for every object type it knows, ER ones too."""
import struct
import sys


def boxes(d, start, end):
    p = start
    while p + 8 <= end:
        size, typ = struct.unpack('>I4s', d[p:p + 8])
        hdr = 8
        if size == 1:
            size = struct.unpack('>Q', d[p + 8:p + 16])[0]
            hdr = 16
        elif size == 0:
            size = end - p
        if size < hdr or p + size > end:
            return
        yield typ.decode('latin1'), p + hdr, p + size
        p += size


def find(d, start, end, path):
    for typ, s, e in boxes(d, start, end):
        if typ == path[0]:
            if len(path) == 1:
                yield s, e
            else:
                yield from find(d, s, e, path[1:])


def descr(d, p):
    """an MPEG-4 descriptor at p: tag, payload start, payload end"""
    tag = d[p]
    p += 1
    n = 0
    for _ in range(4):
        b = d[p]
        p += 1
        n = n << 7 | (b & 0x7f)
        if not b & 0x80:
            break
    return tag, p, p + n


def asc_of(d, s, e):
    """the AudioSpecificConfig in an esds box's ES_Descriptor"""
    tag, p, q = descr(d, s + 4)
    if tag != 3:
        return None
    flags = d[p + 2]
    p += 3
    if flags & 0x80:
        p += 2
    if flags & 0x40:
        p += 1 + d[p]
    if flags & 0x20:
        p += 2
    while p < q:
        tag, a, b = descr(d, p)
        if tag == 4:  # DecoderConfigDescriptor
            r = a + 13
            while r < b:
                t2, a2, b2 = descr(d, r)
                if t2 == 5:
                    return d[a2:b2]
                r = b2
        p = b
    return None


def main():
    d = open(sys.argv[1], 'rb').read()
    for ts, te in find(d, 0, len(d), ['moov', 'trak']):
        stbl = next(find(d, ts, te, ['mdia', 'minf', 'stbl']), None)
        if not stbl:
            continue
        ss, se = stbl
        stsd = next(find(d, ss, se, ['stsd']), None)
        if not stsd:
            continue
        asc = None
        for typ, s, e in boxes(d, stsd[0] + 8, stsd[1]):
            if typ == 'mp4a':
                for t2, s2, e2 in boxes(d, s + 28, e):
                    if t2 == 'esds':
                        asc = asc_of(d, s2, e2)
        if asc is None:
            continue
        s, e = next(find(d, ss, se, ['stsz']))
        fixed, count = struct.unpack('>II', d[s + 4:s + 12])
        sizes = [fixed] * count if fixed else list(struct.unpack('>%dI' % count, d[s + 12:s + 12 + 4 * count]))
        co = next(find(d, ss, se, ['stco']), None)
        if co:
            n = struct.unpack('>I', d[co[0] + 4:co[0] + 8])[0]
            offs = struct.unpack('>%dI' % n, d[co[0] + 8:co[0] + 8 + 4 * n])
        else:
            co = next(find(d, ss, se, ['co64']))
            n = struct.unpack('>I', d[co[0] + 4:co[0] + 8])[0]
            offs = struct.unpack('>%dQ' % n, d[co[0] + 8:co[0] + 8 + 8 * n])
        s, e = next(find(d, ss, se, ['stsc']))
        n = struct.unpack('>I', d[s + 4:s + 8])[0]
        runs = [struct.unpack('>III', d[s + 8 + 12 * i:s + 20 + 12 * i]) for i in range(n)]
        out = bytearray(b'AUS1' + struct.pack('<H', len(asc)) + asc)
        k = 0
        for ci, off in enumerate(offs):
            per = 0
            for first, spc, _ in runs:
                if ci + 1 >= first:
                    per = spc
            for _ in range(per):
                if k >= len(sizes):
                    break
                out += struct.pack('<I', sizes[k]) + d[off:off + sizes[k]]
                off += sizes[k]
                k += 1
        open(sys.argv[2], 'wb').write(out)
        print('%s: %d access units, ASC %s' % (sys.argv[2], k, asc.hex()))
        return
    sys.exit('mp42aus1.py: no AAC track in ' + sys.argv[1])


main()
