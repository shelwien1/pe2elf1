#!/usr/bin/env python3
"""Verify that an mtf2ima output really carries the source code stream.

Reads the codes back out of the produced IMA-ADPCM wav, maps them to MT
Framework codes through the order-preserving bijection, runs ffmpeg's
adpcm_ima_mtf reconstruction over them and compares the result with what
ffmpeg itself decodes the original .fwse to.  Also checks that the step
index each IMA block header carries is the one an MT Framework decoder
holds at that point, and reports the correlation between the two decodes.

    mtf_check.py orig.fwse packed.wav [orig.pcm]

orig.pcm is ffmpeg's s16le decode of orig.fwse; it is produced here if the
path is omitted and ffmpeg is on PATH.
"""

import struct, subprocess, sys, tempfile, os

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,
        80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,
        494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,
        2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
        8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
        27086,29794,32767]
MTF_INDEX = [8,6,4,2,-1,-1,-1,-1,-1,-1,-1,-1,2,4,6,8]
IMA_INDEX = [-1,-1,-1,-1,2,4,6,8]

clip16 = lambda v: -32768 if v < -32768 else (32767 if v > 32767 else v)
clipix = lambda v: 0 if v < 0 else (88 if v > 88 else v)

ima_to_rank = lambda c: 7 - (c & 7) if (c & 8) else 8 + (c & 7)


def mtf_decode(codes):
    """ffmpeg's adpcm_ima_mtf_expand_nibble, one channel."""
    p, si, out = 0, 0, []
    for n in codes:
        v = p + STEP[si] * (2 * n - 15)
        si = clipix(si + MTF_INDEX[n])
        p = clip16(v >> 4)
        out.append(p)
    return out


def ima_decode(codes, p, si):
    """The ordinary IMA reconstruction, one channel."""
    out = []
    for c in codes:
        step = STEP[si]
        d = step >> 3
        if c & 4: d += step
        if c & 2: d += step >> 1
        if c & 1: d += step >> 2
        p = clip16(p - d if c & 8 else p + d)
        si = clipix(si + IMA_INDEX[c & 7])
        out.append(p)
    return out, p, si


def read_fwse(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'FWSE', path
    start, ch = struct.unpack_from('<II', d, 12)
    return ch, d[start:]


def mtf_codes(data, nch):
    """Per channel, in order: high nibble first, one byte per channel."""
    out = [[] for _ in range(nch)]
    for j in range(len(data) // nch * nch):
        b = data[j]
        out[j % nch] += [b >> 4, b & 15]
    return out


def read_wav(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'RIFF' and d[8:12] == b'WAVE', path
    pos, fmt = 12, None
    while pos + 8 <= len(d):
        cid, size = d[pos:pos+4], struct.unpack_from('<I', d, pos+4)[0]
        body = pos + 8
        if cid == b'fmt ':
            fmt = d[body:body+size]
        elif cid == b'data':
            return fmt, d[body:body+min(size, len(d)-body)]
        pos = body + size + (size & 1)
    raise SystemExit('no data chunk')


def wav_blocks(fmt, data):
    tag, nch, rate = struct.unpack_from('<HHI', fmt, 0)
    align = struct.unpack_from('<H', fmt, 12)[0]
    assert tag == 0x11, 'not IMA-ADPCM'
    for pos in range(0, len(data), align):
        yield data[pos:pos+align], nch
    return


def block_codes(blk, nch):
    """Header (predictor, index) per channel plus the payload codes."""
    hdr = [(struct.unpack_from('<h', blk, k*4)[0], blk[k*4+2]) for k in range(nch)]
    body, out = blk[4*nch:], [[] for _ in range(nch)]
    groups = len(body) // (4 * nch)
    for g in range(groups):
        for k in range(nch):
            for b in body[g*4*nch + k*4: g*4*nch + k*4 + 4]:
                out[k] += [b & 15, b >> 4]
    return hdr, out


def main():
    fwse, wav = sys.argv[1], sys.argv[2]
    nch, mtf_data = read_fwse(fwse)

    if len(sys.argv) > 3:
        ref = open(sys.argv[3], 'rb').read()
    else:
        tmp = tempfile.mktemp(suffix='.pcm')
        subprocess.run(['ffmpeg', '-v', 'error', '-i', fwse, '-f', 's16le', tmp, '-y'],
                       check=True)
        ref = open(tmp, 'rb').read()
        os.unlink(tmp)

    ref = struct.unpack('<%dh' % (len(ref) // 2), ref)
    ref_ch = [list(ref[k::nch]) for k in range(nch)]

    fmt, data = read_wav(wav)
    src = mtf_codes(mtf_data, nch)
    got = [[] for _ in range(nch)]
    seeds = [[] for _ in range(nch)]
    ima_out = [[] for _ in range(nch)]

    for blk, _ in wav_blocks(fmt, data):
        if len(blk) < 4 * nch:
            continue
        hdr, codes = block_codes(blk, nch)
        for k in range(nch):
            take = min(len(codes[k]), len(src[k]) - len(got[k]))
            seeds[k].append((len(got[k]), hdr[k]))
            got[k] += [ima_to_rank(c) for c in codes[k][:take]]
            dec, _, _ = ima_decode(codes[k][:take], hdr[k][0], hdr[k][1])
            ima_out[k] += [hdr[k][0]] + dec

    ok = True
    for k in range(nch):
        if got[k] != src[k]:
            n = next(i for i in range(min(len(got[k]), len(src[k]))) if got[k][i] != src[k][i])
            print(f'  ch{k}: CODE MISMATCH at {n} (src {len(src[k])}, wav {len(got[k])})')
            ok = False
            continue
        dec = mtf_decode(got[k])
        if dec != ref_ch[k]:
            print(f'  ch{k}: PCM MISMATCH against ffmpeg')
            ok = False
            continue
        print(f'  ch{k}: {len(got[k])} codes recovered, MTF decode == ffmpeg, {len(dec)} samples')

        # every block header must hold the MTF step index at that code position
        p, si, bad = 0, 0, 0
        state = {}
        for i, n in enumerate(got[k]):
            state[i] = (p, si)
            v = p + STEP[si] * (2 * n - 15)
            si = clipix(si + MTF_INDEX[n])
            p = clip16(v >> 4)
        state[len(got[k])] = (p, si)
        for at, (hp, hi) in seeds[k]:
            if state[at] != (hp, hi):
                bad += 1
        print(f'  ch{k}: {len(seeds[k])} block headers, {bad} not equal to the MTF state')
        ok = ok and bad == 0

        m = min(len(ima_out[k]), len(ref_ch[k]))
        a, b = ima_out[k][:m], ref_ch[k][:m]
        ma, mb = sum(a)/m, sum(b)/m
        va = sum((x-ma)**2 for x in a); vb = sum((x-mb)**2 for x in b)
        cov = sum((x-ma)*(y-mb) for x, y in zip(a, b))
        r = cov / (va*vb)**0.5 if va and vb else 0.0
        print(f'  ch{k}: correlation of the IMA decode with the source audio: {r:+.4f}')

    print('OK' if ok else 'FAILED')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
