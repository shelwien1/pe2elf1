#!/usr/bin/env python3
"""gen_testwavs.py -- synthetic ADPCM wavs covering every codec instantiation.

The two real test files in this tree are both 4-bit stereo -- one MS, one IMA --
so between them they reach exactly two of the ten payload-walk instantiations
that code_data() dispatches to.  These fill in the rest: every IMA code width
(2/3/4/5), mono as well as multi-channel, 4- and 8-channel IMA, MS mono, a
non-standard MS coefficient table, and block sizes that leave a runt tail and a
partial code group behind.

The payload is deterministic pseudo-random rather than real audio, on purpose.
xadpcm is lossless over whatever bytes the data chunk holds, and what these
files are for is exercising code paths and proving bit-exactness -- noise does
that as well as music and makes the corpus reproducible from this script rather
than from a 3 MB blob in git.  Do not read the coded sizes as ratios: random
nibbles are incompressible and every one of these expands slightly.

    python3 gen_testwavs.py [outdir]      # default: gen/
"""
import os
import random
import struct
import sys

MS_STD = [(256, 0), (512, -256), (0, 0), (192, 64), (240, 0), (460, -208), (392, -232)]


def wav(path, tag, chans, bps, ba, rate=22050, nblk=40, coefs=None):
    random.seed(os.path.basename(path))
    data = bytes(random.getrandbits(8) for _ in range(ba * nblk))
    if tag == 0x11:  # WAVE_FORMAT_IMA_ADPCM
        spb = (ba - chans * 4) // chans * 8 // bps + 1
        fmt = struct.pack('<HHIIHHHH', 0x11, chans, rate, rate * ba // spb, ba, bps, 2, spb)
    else:            # WAVE_FORMAT_ADPCM (MS), always 4-bit
        spb = (ba - 7 * chans) * 2 // chans + 2
        c = coefs or MS_STD
        fmt = struct.pack('<HHIIHHHHH', 0x02, chans, rate, rate * ba // spb, ba, 4,
                          4 + 4 * len(c), spb, len(c))
        for a, b in c:
            fmt += struct.pack('<hh', a, b)
    body = (b'fmt ' + struct.pack('<I', len(fmt)) + fmt
            + b'fact' + struct.pack('<I', 4) + struct.pack('<I', nblk * spb)
            + b'data' + struct.pack('<I', len(data)) + data)
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', 4 + len(body)) + b'WAVE' + body)


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else 'gen'
    os.makedirs(d, exist_ok=True)
    j = lambda n: os.path.join(d, n)
    for bps in (2, 3, 4, 5):                      # code_data_ima<BPS, 0|1>
        for ch in (1, 2):
            wav(j('ima_b%d_c%d.wav' % (bps, ch)), 0x11, ch, bps, 256 * ch)
    wav(j('ima_b4_c4.wav'), 0x11, 4, 4, 512)      # >2 channels: the wrap-around
    wav(j('ima_b4_c8.wav'), 0x11, 8, 4, 1024)     # MAX_CHANS
    for ch in (1, 2):                             # code_data_ms<0|1>
        wav(j('ms_c%d.wav' % ch), 0x02, ch, 4, 256 * ch)
    # block sizes chosen to leave a partial group / odd nibble, and a
    # coefficient table that is not the standard seven (so it is spelled out
    # in the segment record instead of costing a zero byte)
    wav(j('ms_c2_odd.wav'), 0x02, 2, 4, 251, coefs=[(256, 0), (400, -100), (100, 50)])
    wav(j('ima_b3_c2_odd.wav'), 0x11, 2, 3, 253)
    for n in sorted(os.listdir(d)):
        print(os.path.join(d, n))


if __name__ == '__main__':
    main()
