#!/usr/bin/env python3
"""tests/vbn2.py [--record] [--big] [--exe PATH] [--cxx CMD]: the gate for vbn2.cpp.

1. vbn2 against vbn itself.  For each corpus entry and each of vbn's modes
   -- `vbn u` (MAX: 95 candidates, vbn2's default), `vbn b` (NORMAL: 55) and
   `vbn B` (FAST: 31) -- the pe2elf32 port (../vbn) writes an archive: a
   3-byte header, then vbn's code for the archive's title followed by the
   data.  That code, preceded by vbn2's header for it -- the size of what it
   codes, and in the modes but MAX the parameters, with the mode's candidates
   -- is a vbn2 stream.  vbn2 d must decode it to the title and the data, and
   vbn2 c (given the candidates) must encode that back to the same bytes.
2. Every input of mkinputs.py: vbn2 c, then vbn2 d must give the input back,
   and the compressed stream's md5 must be the one vbn2.md5 records (--record
   writes vbn2.md5 instead of checking it).
3. The same for PARAM_INPUTS with each of PARAMS, vbn2's other parameters:
   windows that slide every few bytes, slides that keep nothing or nearly
   everything, a short order, the fewest candidates, a larger window.
4. With --big: zeros of 0x1FFFFFFF and 0x20000000 bytes, vbn's 512 MB and a
   byte more, must round-trip, the first with vbn's 4-byte size and the
   second with the 64-bit one.  (About 30 seconds, and 512 MB of disk for a
   while: the inputs are sparse files, but not the output.)

vbn2 is built from ../vbn2.cpp with `g++ -O2` unless --exe names a binary;
--cxx gives another compiler command (for example "clang++ -m32 -O3").
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
MODES = {'u': 95, 'b': 55, 'B': 31}   # vbn's command, and the candidates its mode searches for
PARAMS = [('0', '16'), ('0', '64', '0'), ('0', '1', '99'), ('4',), ('0', '1632', '75', '2'), ('6', '4096', '50', '31')]
PARAM_INPUTS = ['input.txt', 'rand', 'rep.bin', 'zeros', 'bytes', 'text-1000', 'text-0']
VBN_MAX_SIZE = 0x1FFFFFFF             # vbn's most data, and the most a 4-byte size in a stream holds


def run(args, cwd=None):
    p = subprocess.run(args, cwd=cwd, capture_output=True, text=True, env=dict(os.environ, TZ='UTC'))
    return p.returncode, (p.stdout + p.stderr).strip()


def source(entry):
    if entry in ('input.txt', 'vbn.exe'):
        return os.path.join(REPO, entry)
    return os.path.join(REPO, 'incdec', 'corpus', entry)


def against_vbn(exe, work, entry, mode):
    """Step 1 for one entry and mode: None, or what went wrong."""
    cands = MODES[mode]
    d = os.path.join(work, 'vbn%d-%s' % (cands, entry))
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
    rc, out = run([os.path.join(REPO, 'vbn'), mode, 'archive', arg], cwd=d)
    if rc or not os.path.exists(os.path.join(d, 'archive.vbn')):
        return 'vbn %s failed: %s' % (mode, out[-200:])
    arc = open(os.path.join(d, 'archive.vbn'), 'rb').read()
    h = arc[0] | arc[1] << 8 | arc[2] << 16
    title = (h & 0x7FFF) | ((h & 0x70000) >> 1)
    params = [] if cands == 95 else ['0', '1632', '75', str(cands)]
    if params:   # the size with the top bit set; vbn's window and slide, the candidates, order 0
        header = struct.pack('<IIIBB', (title + size) | 0x80000000, 0x198000, 0x132000, cands, 0)
    else:
        header = struct.pack('<I', title + size)
    stream = header + arc[3:]
    s, t, s2 = (os.path.join(work, '%s.%d%s' % (entry, cands, x)) for x in ('.from-vbn', '.decoded', '.again'))
    open(s, 'wb').write(stream)
    rc, out = run([exe, 'd', s, t])
    if rc:
        return 'vbn2 d failed: ' + out
    data = open(t, 'rb').read()
    if len(data) != title + size:
        return 'vbn2 d wrote %d bytes, not %d' % (len(data), title + size)
    if not os.path.isdir(src) and data[title:] != open(src, 'rb').read():
        return 'vbn2 d decoded other data'
    rc, out = run([exe, 'c', t, s2] + params)
    if rc:
        return 'vbn2 c failed: ' + out
    if open(s2, 'rb').read() != stream:
        return 'vbn2 c does not encode as vbn does'
    return None


def key(name, params):
    """An input's name in vbn2.md5, with the parameters it was compressed with."""
    return name + ('@' + '-'.join(params) if params else '')


def round_trip(exe, work, inputs, name, params=()):
    """Steps 2 and 3 for one input: (md5 of the stream or None, what went wrong or None)."""
    x = os.path.join(inputs, name)
    s, y = os.path.join(work, key(name, params) + '.vbn2'), os.path.join(work, key(name, params) + '.back')
    rc, out = run([exe, 'c', x, s] + list(params))
    if rc:
        return None, 'vbn2 c failed: ' + out
    digest = hashlib.md5(open(s, 'rb').read()).hexdigest()
    rc, out = run([exe, 'd', s, y])
    if rc:
        return digest, 'vbn2 d failed: ' + out
    if open(y, 'rb').read() != open(x, 'rb').read():
        return digest, 'vbn2 d gave back other data'
    return digest, None


def big_round_trip(exe, work, size):
    """Step 4 for one size: None, or what went wrong."""
    x, s, y = (os.path.join(work, 'big-%d%s' % (size, e)) for e in ('', '.vbn2', '.back'))
    with open(x, 'wb') as f:
        f.truncate(size)                  # zeros, sparse
    try:
        rc, out = run([exe, 'c', x, s])
        if rc:
            return 'vbn2 c failed: ' + out[-200:]
        want = struct.pack('<I', size) if size <= VBN_MAX_SIZE else struct.pack('<IQ', 0x40000000, size)
        head = open(s, 'rb').read(len(want))
        if head != want:
            return 'header %s, not %s' % (head.hex(), want.hex())
        rc, out = run([exe, 'd', s, y])
        if rc:
            return 'vbn2 d failed: ' + out[-200:]
        if os.path.getsize(y) != size:
            return 'vbn2 d wrote %d bytes' % os.path.getsize(y)
        with open(y, 'rb') as f:
            for chunk in iter(lambda: f.read(1 << 24), b''):
                if chunk.strip(b'\0'):
                    return 'vbn2 d gave back other data'
        return None
    finally:
        for f in (x, s, y):
            if os.path.exists(f):
                os.remove(f)


def main():
    args = sys.argv[1:]
    record = '--record' in args
    big = '--big' in args
    exe = args[args.index('--exe') + 1] if '--exe' in args else None
    cxx = args[args.index('--cxx') + 1] if '--cxx' in args else 'g++ -O2'
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
        runs = [(n, ()) for n in names] + [(n, p) for p in PARAMS for n in PARAM_INPUTS]
        with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
            vj = {(e, m): ex.submit(against_vbn, exe, work, e, m) for m in MODES for e in ENTRIES}
            rj = {r: ex.submit(round_trip, exe, work, inputs, *r) for r in runs}
            for m in MODES:
                for e in ENTRIES:
                    why = vj[(e, m)].result()
                    print('%s vbn %s %s%s' % ('FAIL' if why else 'ok  ', m, e, ': ' + why if why else ''))
                    fail += why is not None
            digests, bad, bad_params = {}, 0, 0
            for r in runs:
                k = key(*r)
                digest, why = rj[r].result()
                digests[k] = digest
                if not why and not record and refs.get(k) != digest:
                    why = 'stream %s, reference %s' % (digest, refs.get(k, 'none'))
                if why:
                    print('FAIL %s: %s' % (k, why))
                    bad += not r[1]
                    bad_params += bool(r[1])
            fail += bad + bad_params
        n_params = len(runs) - len(names)
        print('%d of %d inputs round-trip%s; %d of %d with other parameters' % (
            len(names) - bad, len(names), '' if record else ' with the recorded streams', n_params - bad_params, n_params))
        for size in ((VBN_MAX_SIZE, VBN_MAX_SIZE + 1) if big else ()):
            why = big_round_trip(exe, work, size)
            print('%s %d bytes of zeros%s' % ('FAIL' if why else 'ok  ', size, ': ' + why if why else ''))
            fail += why is not None
        if record:
            with open(os.path.join(HERE, 'vbn2.md5'), 'w') as f:
                for r in runs:
                    f.write('%s  %s\n' % (digests[key(*r)], key(*r)))
            print('wrote vbn2.md5')
        print('PASS' if not fail else 'FAIL')
        return 1 if fail else 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
