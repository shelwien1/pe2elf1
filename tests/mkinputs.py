#!/usr/bin/env python3
"""mkinputs.py <dir>: the inputs tests/vbn2.sh compresses, written to <dir>.

Deterministic: everything is cut from the repository's own files (input.txt,
vbn.exe, and incdec/tools/mkcorpus.py's corpus, which is deterministic too) or
made by a fixed generator, so the reference digests in vbn2.md5 stay valid.

  text-<n>      the first n bytes of input.txt: every small size where a
                buffer or a count might go wrong (0 included)
  win-<n>       big.bin cut around the coder's window: it holds 0x198000
                bytes and slides by 0x66000, so the cuts sit at the first
                window's end and the first slide's end, one byte either side
  zeros, rand, bytes
                a megabyte of zeros (one long match), 100000 bytes of
                xorshift output (nothing to match), and each byte value in
                runs (every symbol, every run length up to 64)
"""
import os
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WINDOW, SLIDE = 0x198000, 0x66000


def corpus(name):
    path = os.path.join(REPO, 'incdec', 'corpus', name)
    if not os.path.exists(path):
        subprocess.run([sys.executable, os.path.join(REPO, 'incdec', 'tools', 'mkcorpus.py'), 'corpus'],
                       cwd=os.path.join(REPO, 'incdec'), check=True)
    return path


def xorshift(n, seed=0x2545F491):
    out, x = bytearray(), seed
    while len(out) < n:
        x ^= (x << 13) & 0xFFFFFFFF
        x ^= x >> 17
        x ^= (x << 5) & 0xFFFFFFFF
        out += x.to_bytes(4, 'little')
    return bytes(out[:n])


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    put = lambda name, data: open(os.path.join(out, name), 'wb').write(data)
    text = open(os.path.join(REPO, 'input.txt'), 'rb').read()
    for n in (0, 1, 2, 3, 4, 7, 8, 9, 16, 17, 255, 256, 257, 1000, 4095, 4096, 4097, 8192, 8193, 65536, 65537):
        put('text-%d' % n, text[:n])
    big = open(corpus('big.bin'), 'rb').read()
    for n in (WINDOW - 1, WINDOW, WINDOW + 1, WINDOW + SLIDE - 1, WINDOW + SLIDE, WINDOW + SLIDE + 1):
        put('win-%d' % n, big[:n])
    put('zeros', bytes(1000000))
    put('rand', xorshift(100000))
    put('bytes', b''.join(bytes([b]) * k for k in range(1, 65) for b in range(256)))
    # the corpus itself, flattened
    shutil.copy(os.path.join(REPO, 'input.txt'), os.path.join(out, 'input.txt'))
    shutil.copy(os.path.join(REPO, 'vbn.exe'), os.path.join(out, 'vbn.exe'))
    for name in ('mixed.bin', 'rep.bin', 'big.bin'):
        shutil.copy(corpus(name), os.path.join(out, name))
    for d, _, files in os.walk(corpus('tree')):
        for f in files:
            shutil.copy(os.path.join(d, f), os.path.join(out, 'tree-' + f))


if __name__ == '__main__':
    main()
