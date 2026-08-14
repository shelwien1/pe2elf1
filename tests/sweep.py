#!/usr/bin/env python3
"""Round-trip sweep for mp2.cpp.

For every header configuration it can build, this generates a synthetic Layer II
file, packs it with "mp2 c", checks that the resulting mp3 parses frame-for-frame
with the mp3_A.h decoder, unpacks it with "mp2 d" and compares byte for byte.

usage: sweep.py [--quick] [--jobs N]
"""
import os, subprocess, sys, tempfile, itertools, hashlib
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
MP2 = os.path.join(ROOT, "mp2")
MKMP2 = os.path.join(HERE, "mkmp2")
MP3CHK = os.path.join(HERE, "mp3chk")

quick = "--quick" in sys.argv
jobs = 4
if "--jobs" in sys.argv:
    jobs = int(sys.argv[sys.argv.index("--jobs") + 1])

# header byte 1: 111 sync | version | layer=10 | protection
#   MPEG1 = 11, MPEG2 = 10   (MPEG2.5 has no Layer II in this decoder)
B1 = [("mpeg1", 0xFD), ("mpeg1+crc", 0xFC), ("mpeg2", 0xF5), ("mpeg2+crc", 0xF4)]
MODES = [("stereo", 0, 0), ("joint0", 1, 0), ("joint1", 1, 1),
         ("joint2", 1, 2), ("joint3", 1, 3), ("dual", 2, 0), ("mono", 3, 0)]

cases = []
for name1, b1 in B1:
    for sr in (0, 1, 2):
        brs = range(0, 15) if not quick else (0, 3, 8, 14)
        for br in brs:
            if br == 15:
                continue
            for mname, mode, mext in MODES:
                junk = 0
                if (br + sr) % 5 == 0:
                    junk = 2
                elif (br + sr) % 3 == 0:
                    junk = 1
                b2 = (br << 4) | (sr << 2)
                b3 = (mode << 6) | (mext << 4)
                cases.append((("%s sr%d br%d %s junk%d" % (name1, sr, br, mname, junk)),
                              b1, b2, b3, junk))

if quick:
    cases = cases[::3]

nframes = 6
fail = []
skip = [0]
done = [0]
lock_print = []


def run(case):
    name, b1, b2, b3, junk = case
    tmp = tempfile.mkdtemp(prefix="mp2sw")
    src = os.path.join(tmp, "a.mp2")
    mp3 = os.path.join(tmp, "a.mp3")
    meta = os.path.join(tmp, "a.meta")
    out = os.path.join(tmp, "b.mp2")
    try:
        r = subprocess.run([MKMP2, src, "%02x" % b1, "%02x" % b2, "%02x" % b3,
                            str(nframes), str(abs(hash(name)) % 100000), str(junk)],
                           capture_output=True)
        if r.returncode == 2:
            skip[0] += 1
            return None
        if r.returncode != 0:
            return (name, "mkmp2 failed: " + r.stderr.decode()[:200])

        r = subprocess.run([MP2, "c", src, mp3, meta], capture_output=True)
        if r.returncode != 0:
            return (name, "pack failed: " + r.stderr.decode()[:200])
        info = dict(l.split(":", 1) for l in r.stdout.decode().splitlines() if ":" in l)
        nmp3 = int(info.get("mp3 frames ", "0"))

        r = subprocess.run([MP3CHK, mp3], capture_output=True)
        chk = r.stdout.decode().strip()
        if r.returncode != 0:
            return (name, "mp3 not clean: " + chk)
        got = int(chk.split()[1])
        if got != nmp3:
            return (name, "mp3 frame count %d != %d (%s)" % (got, nmp3, chk))

        r = subprocess.run([MP2, "d", mp3, meta, out], capture_output=True)
        if r.returncode != 0:
            return (name, "unpack failed: " + r.stderr.decode()[:200])

        a = open(src, "rb").read()
        b = open(out, "rb").read()
        if a != b:
            return (name, "MISMATCH (%d vs %d bytes)" % (len(a), len(b)))
        return None
    finally:
        for f in (src, mp3, meta, out):
            try:
                os.unlink(f)
            except OSError:
                pass
        try:
            os.rmdir(tmp)
        except OSError:
            pass


with ThreadPoolExecutor(max_workers=jobs) as ex:
    for res in ex.map(run, cases):
        done[0] += 1
        if res:
            fail.append(res)
            print("FAIL %-42s %s" % res)
        if done[0] % 100 == 0:
            print("  ... %d/%d, %d failed, %d skipped" % (done[0], len(cases), len(fail), skip[0]))

print("=" * 70)
print("cases %d, skipped(unbuildable) %d, failed %d" % (len(cases), skip[0], len(fail)))
sys.exit(1 if fail else 0)
