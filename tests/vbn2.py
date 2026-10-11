#!/usr/bin/env python3
"""tests/vbn2.py [--record] [--exe PATH] [--cxx CMD]: the gate for vbn2.cpp.

1. vbn2 against vbn itself.  For each corpus entry, `vbn u` (the pe2elf32
   port, ../vbn) writes an archive: a 3-byte header, then vbn's code for the
   archive's title followed by the data.  That code, preceded by the size of
   what it codes, is a vbn2 stream.  vbn2 d must decode it to the title and
   the data, and vbn2 c must encode that back to the same bytes.
2. Every input of mkinputs.py: vbn2 c, then vbn2 d must give the input back,
   and the compressed stream's md5 must be the one vbn2.md5 records (--record
   writes vbn2.md5 instead of checking it).

vbn2 is built from ../vbn2.cpp with `g++ -m32 -O2` unless --exe names a
binary; --cxx gives another compiler command (for example "clang++ -m32 -O3").
"""
import concurrent.futures
import hashlib
import os
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
STAMP = 315533580                     # 1980-01-01 00:13 UTC, as ../test.sh pins it
ENTRIES = ['input.txt', 'mixed.bin', 'rep.bin', 'vbn.exe', 'big.bin', 'tree']


def run(args, cwd=None):
    p = subprocess.run(args, cwd=cwd, capture_output=True, text=True, env=dict(os.environ, TZ='UTC'))
    return p.returncode, (p.stdout + p.stderr).strip()


def source(entry):
    if entry in ('input.txt', 'vbn.exe'):
        return os.path.join(REPO, entry)
    return os.path.join(REPO, 'incdec', 'corpus', entry)


def against_vbn(exe, work, entry):
    """Step 1 for one entry: None, or what went wrong."""
    d = os.path.join(work, 'vbn-' + entry)
    src = source(entry)
    if os.path.isdir(src):
        shutil.copytree(src, d)
        arg = '*.*'
    else:
        os.makedirs(d)
        shutil.copy(src, d)
        arg = entry
    size = 0
    for top, _, files in os.walk(d):
        for f in files:
            os.utime(os.path.join(top, f), (STAMP, STAMP))
            size += os.path.getsize(os.path.join(top, f))
    rc, out = run([os.path.join(REPO, 'vbn'), 'u', 'archive', arg], cwd=d)
    if rc or not os.path.exists(os.path.join(d, 'archive.vbn')):
        return 'vbn u failed: ' + out[-200:]
    arc = open(os.path.join(d, 'archive.vbn'), 'rb').read()
    h = arc[0] | arc[1] << 8 | arc[2] << 16
    title = (h & 0x7FFF) | ((h & 0x70000) >> 1)
    stream = struct.pack('<I', title + size) + arc[3:]
    s, t, s2 = (os.path.join(work, entry + x) for x in ('.from-vbn', '.decoded', '.again'))
    open(s, 'wb').write(stream)
    rc, out = run([exe, 'd', s, t])
    if rc:
        return 'vbn2 d failed: ' + out
    data = open(t, 'rb').read()
    if len(data) != title + size:
        return 'vbn2 d wrote %d bytes, not %d' % (len(data), title + size)
    if not os.path.isdir(src) and data[title:] != open(src, 'rb').read():
        return 'vbn2 d decoded other data'
    rc, out = run([exe, 'c', t, s2])
    if rc:
        return 'vbn2 c failed: ' + out
    if open(s2, 'rb').read() != stream:
        return 'vbn2 c does not encode as vbn does'
    return None


def round_trip(exe, work, inputs, name):
    """Step 2 for one input: (md5 of the stream or None, what went wrong or None)."""
    x = os.path.join(inputs, name)
    s, y = os.path.join(work, name + '.vbn2'), os.path.join(work, name + '.back')
    rc, out = run([exe, 'c', x, s])
    if rc:
        return None, 'vbn2 c failed: ' + out
    digest = hashlib.md5(open(s, 'rb').read()).hexdigest()
    rc, out = run([exe, 'd', s, y])
    if rc:
        return digest, 'vbn2 d failed: ' + out
    if open(y, 'rb').read() != open(x, 'rb').read():
        return digest, 'vbn2 d gave back other data'
    return digest, None


def main():
    args = sys.argv[1:]
    record = '--record' in args
    exe = args[args.index('--exe') + 1] if '--exe' in args else None
    cxx = args[args.index('--cxx') + 1] if '--cxx' in args else 'g++ -m32 -O2'
    work = tempfile.mkdtemp(prefix='vbn2test.', dir=HERE)
    try:
        if exe:
            exe = os.path.abspath(exe)
        else:
            exe = os.path.join(work, 'vbn2')
            rc, out = run(shlex.split(cxx) + ['-o', exe, os.path.join(REPO, 'vbn2.cpp')])
            if rc:
                print(out)
                print('FAIL: cannot build vbn2.cpp with %s' % cxx)
                return 1
        rc, out = run(['make', '-s', '-C', REPO, 'vbn', 'libvbnshim.so'])
        if rc:
            print(out)
            return 1
        inputs = os.path.join(work, 'inputs')
        subprocess.run([sys.executable, os.path.join(HERE, 'mkinputs.py'), inputs], check=True)
        names = sorted(os.listdir(inputs))
        refs = {}
        if not record:
            for line in open(os.path.join(HERE, 'vbn2.md5')):
                digest, name = line.split()
                refs[name] = digest
        fail = 0
        with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
            vj = {e: ex.submit(against_vbn, exe, work, e) for e in ENTRIES}
            rj = {n: ex.submit(round_trip, exe, work, inputs, n) for n in names}
            for e in ENTRIES:
                why = vj[e].result()
                print('%s vbn %s%s' % ('FAIL' if why else 'ok  ', e, ': ' + why if why else ''))
                fail += why is not None
            digests, bad = {}, 0
            for n in names:
                digest, why = rj[n].result()
                digests[n] = digest
                if not why and not record and refs.get(n) != digest:
                    why = 'stream %s, reference %s' % (digest, refs.get(n, 'none'))
                if why:
                    print('FAIL %s: %s' % (n, why))
                    bad += 1
            fail += bad
        print('%d of %d inputs round-trip%s' % (len(names) - bad, len(names),
                                               '' if record else ' with the recorded streams'))
        if record:
            with open(os.path.join(HERE, 'vbn2.md5'), 'w') as f:
                for n in names:
                    f.write('%s  %s\n' % (digests[n], n))
            print('wrote vbn2.md5')
        print('PASS' if not fail else 'FAIL')
        return 1 if fail else 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
