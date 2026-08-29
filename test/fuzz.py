#!/usr/bin/env python3
"""Feeds the decoder damaged streams and checks it fails cleanly.

A coded stream is untrusted input: the container header, the CCSDS header and
the coded body all come off disk.  This does not check that a damaged stream
decodes to anything sensible -- it cannot -- only that the decoder rejects it or
produces some image without crashing, reading out of bounds or leaking.  Built
with -fsanitize=address,undefined it is the sanitizers that do the checking; a
plain build still catches signals.

usage: fuzz.py [binary] [seed-bmp]
"""

import os
import random
import subprocess
import sys
import tempfile

BAD_MARKERS = ("Sanitizer", "runtime error:")


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    binary = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "ccsds")
    seed_bmp = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
        root, "test", "bmp", "photo24.bmp")
    if not os.path.exists(binary):
        sys.exit("fuzz: %s has not been built (run make)" % binary)

    work = tempfile.mkdtemp(prefix="ccsds-fuzz.")
    good = os.path.join(work, "good.cc")
    broken = os.path.join(work, "broken.cc")
    out = os.path.join(work, "out.bmp")

    if subprocess.run([binary, "c", seed_bmp, good],
                      capture_output=True).returncode != 0:
        sys.exit("fuzz: could not compress %s" % seed_bmp)
    src = open(good, "rb").read()

    random.seed(20240607)
    cases = []
    # Every truncation inside the two headers, then a few deeper ones.
    for n in list(range(0, 48)) + [len(src) // 4, len(src) // 2, len(src) - 1]:
        cases.append(("truncated to %d" % n, src[:n]))
    # Single bit flips, half of them aimed at the headers where a wrong value
    # turns into a geometry or a parameter the rest of the decoder trusts.
    for _ in range(600):
        i = random.randrange(len(src)) if random.random() < 0.5 \
            else random.randrange(min(64, len(src)))
        b = bytearray(src)
        b[i] ^= 1 << random.randrange(8)
        cases.append(("bit flipped at %d" % i, bytes(b)))
    # Whole-byte damage in runs, which is what a bad transfer looks like.
    for _ in range(200):
        i = random.randrange(len(src))
        n = random.randrange(1, 32)
        b = bytearray(src)
        for k in range(i, min(i + n, len(b))):
            b[k] = random.randrange(256)
        cases.append(("%d bytes clobbered at %d" % (n, i), bytes(b)))

    bad = 0
    for name, data in cases:
        with open(broken, "wb") as f:
            f.write(data)
        r = subprocess.run([binary, "d", broken, out], capture_output=True)
        text = (r.stdout + r.stderr).decode("utf8", "replace")
        crashed = r.returncode < 0 or any(m in text for m in BAD_MARKERS)
        if crashed:
            bad += 1
            if bad <= 5:
                print("FAIL %s (rc=%d)" % (name, r.returncode))
                print("\n".join("     " + l for l in text.splitlines()[:14]))

    for f in os.listdir(work):
        os.unlink(os.path.join(work, f))
    os.rmdir(work)

    print("%d damaged streams, %d unclean failures" % (len(cases), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
