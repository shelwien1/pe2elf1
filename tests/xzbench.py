#!/usr/bin/env python3
"""xz benchmark for pdfdump.

For each input PDF: run `pdfdump c`, tar the dump files in name order, compress with xz -9
(lzma preset 9, CRC64, same as `xz -9`), and compare with xz -9 of the original PDF.
Also reports c throughput (input bytes / c wall time, verify included) and a breakdown of
the dump by output type (each file compressed on its own, summed per extension; only a
guide to where the bytes are, the solid tar is the real number).

  python3 -I tests/xzbench.py [-b pdfdump] [-j N] [-x "c options"] [-o out.csv] files...
"""
import argparse, collections, csv, io, lzma, os, subprocess, sys, tarfile, tempfile, time
from concurrent.futures import ProcessPoolExecutor


# standard payload formats handled by external recompressors; everything else is PDF-specific
PAYLOAD = {'ttf', 'otf', 'cff', 'jpg', 'jp2', 'j2k', 'jb2', 'jb2g', 'g4', 'bmp', 'png', 'gif', 'tif', 'icc',
           'pdf', 'zip', 'img'}


def tar_xz(td, names):
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.GNU_FORMAT) as tf:
        for n in names:
            ti = tf.gettarinfo(os.path.join(td, n), arcname=n)
            ti.mtime = 0; ti.uid = ti.gid = 0; ti.uname = ti.gname = ''
            with open(os.path.join(td, n), 'rb') as f:
                tf.addfile(ti, f)
    data = buf.getvalue()
    return len(data), xz(data)


def xz(data):
    return len(lzma.compress(data, preset=9))


def bench(args):
    pdfdump, extra, path = args
    with tempfile.TemporaryDirectory() as td:
        prefix = os.path.join(td, 'd')
        t0 = time.time()
        r = subprocess.run([pdfdump, 'c', '-q'] + extra + [path, prefix], capture_output=True)
        ct = time.time() - t0
        if r.returncode:
            return {'file': path, 'error': r.stderr.decode(errors='replace')[:200]}
        names = sorted(os.listdir(td))
        tar_raw, tar_xzs = tar_xz(td, names)
        spec = [n for n in names if n.rsplit('.', 1)[-1] not in PAYLOAD]
        spec_raw, spec_xz = tar_xz(td, spec)
        parts = collections.Counter()
        raw = collections.Counter()
        for n in names:
            d = open(os.path.join(td, n), 'rb').read()
            ext = n.rsplit('.', 1)[-1]
            parts[ext] += xz(d) if d else 0
            raw[ext] += len(d)
        orig = open(path, 'rb').read()
        return {'file': path, 'size': len(orig), 'orig_xz': xz(orig), 'dump_raw': tar_raw,
                'dump_xz': tar_xzs, 'spec_xz': spec_xz, 'c_sec': ct, 'parts': dict(parts), 'raw': dict(raw),
                'nfiles': len(names)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-b', default='./pdfdump')
    ap.add_argument('-j', type=int, default=os.cpu_count())
    ap.add_argument('-x', default='')
    ap.add_argument('-o')
    ap.add_argument('-q', action='store_true', help='totals only')
    ap.add_argument('files', nargs='+')
    a = ap.parse_args()
    extra = a.x.split()
    jobs = [(os.path.abspath(a.b), extra, f) for f in a.files]
    with ProcessPoolExecutor(a.j) as ex:
        res = list(ex.map(bench, jobs))
    tot = collections.Counter()
    parts = collections.Counter()
    raw = collections.Counter()
    slow = []
    rows = []
    for r in res:
        if 'error' in r:
            print('ERROR', r['file'], r['error'], file=sys.stderr)
            continue
        for k in ('size', 'orig_xz', 'dump_raw', 'dump_xz', 'spec_xz', 'c_sec', 'nfiles'):
            tot[k] += r[k]
        parts.update(r['parts'])
        raw.update(r['raw'])
        kbs = r['size'] / 1e3 / max(r['c_sec'], 1e-6)
        slow.append((kbs, r['file']))
        rows.append([os.path.basename(r['file']), r['size'], r['orig_xz'], r['dump_xz'], r['spec_xz'],
                     '%.4f' % (r['dump_xz'] / r['orig_xz']), '%.0f' % kbs, r['nfiles']])
        if not a.q:
            print('%-60s %9d  xz %8d -> %8d  %6.2f%%  %6.0f KB/s' % (
                os.path.basename(r['file'])[:60], r['size'], r['orig_xz'], r['dump_xz'],
                100.0 * r['dump_xz'] / r['orig_xz'], kbs))
    if a.o:
        with open(a.o, 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['file', 'size', 'orig_xz', 'dump_xz', 'pdf_specific_xz', 'ratio', 'kb_per_s', 'files'])
            w.writerows(rows)
    print('files %d  size %d  orig.xz %d  dump.xz %d  (%.2f%% of orig.xz)  dump raw %d' % (
        len(rows), tot['size'], tot['orig_xz'], tot['dump_xz'], 100.0 * tot['dump_xz'] / max(1, tot['orig_xz']),
        tot['dump_raw']))
    print('pdf-specific part (all but %s): %d bytes xz' % (','.join(sorted(PAYLOAD)), tot['spec_xz']))
    print('c: %.1f s total, %.0f KB/s aggregate, slowest: %s' % (
        tot['c_sec'], tot['size'] / 1e3 / max(tot['c_sec'], 1e-6),
        ', '.join('%.0f KB/s %s' % (k, os.path.basename(f)[:30]) for k, f in sorted(slow)[:3])))
    print('by type (per-file xz, raw):', '  '.join('.%s %d (%d)' % (e, parts[e], raw[e]) for e, _ in parts.most_common()))


if __name__ == '__main__':
    main()
