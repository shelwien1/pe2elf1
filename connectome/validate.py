"""Check ./metric against the official scorer from the starter pack's utils.py.

usage: python validate.py [--starterpack DIR] [--metric ./metric] DATA_DIR PRED_DIR [PRED_DIR ...]

Every DATA_DIR/seq-*.tsv is paired with PRED_DIR/seq-*.tsv and scored by
  - utils.GlobalAccumulator (what ScorerStepByStep uses), per file and pooled
  - utils.weighted_pearson on the pooled need_prediction & is_scored rows
    (the in-memory reference from METRIC.md)
  - ./metric, parsed from its output
The results must agree to 1e-12 (./metric -d15 output).
Synthetic prediction sets are checked too: heavy clipping, constant
predictions (score 0), perfect and negated predictions, the one-row-per-data-row
layout, and a nonfinite prediction that both scorers must reject.
"""
import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path
import numpy as np

TOL = 1e-12


def load_tsv(fn):
    with open(fn) as f:
        names = f.readline().rstrip('\n').split('\t')
    a = np.loadtxt(fn, delimiter='\t', skiprows=1, dtype=str, ndmin=2)
    return {c: a[:, j] for j, c in enumerate(names)}


def load_data(fn):
    d = load_tsv(fn)
    need = d['need_prediction'].astype(np.int64) != 0
    scored = d['is_scored'].astype(np.int64) != 0 if 'is_scored' in d else need
    y = np.column_stack([d['t0'], d['t1']]).astype(np.float32)
    return y, need, scored


def load_pred(fn, need):
    p = load_tsv(fn)
    v = np.column_stack([p['t0'], p['t1']]).astype(np.float32)
    pred = np.full((len(need), 2), np.nan, dtype=np.float32)
    pred[need] = v[need] if len(v) == len(need) else v
    return pred


def write_pred(fn, seq, step, pred, rows):
    with open(fn, 'w', newline='\n') as f:
        f.write('seq_ix\tstep_in_seq\tt0\tt1\n')
        for i in rows:
            f.write(f'{seq}\t{step[i]}\t{pred[i, 0]!s}\t{pred[i, 1]!s}\n')


def run_metric(metric, pairs):
    r = subprocess.run([metric, '-d15'] + [x for pr in pairs for x in pr], capture_output=True, text=True)
    if r.returncode:
        return None, r.stderr.strip()
    res = {}
    for line in r.stdout.splitlines():
        m = re.match(r'^(\S+)\s+t0=(\S+) t1=(\S+) WP=(\S+)', line)
        if m:
            res[m.group(1)] = np.array([float(m.group(i)) for i in (2, 3, 4)])
    return res, r.stdout


def official(u, data, preds):
    """per-file and pooled [t0, t1, WP] via GlobalAccumulator + pooled reference"""
    total, per = u.GlobalAccumulator(), {}
    for name, (y, need, scored) in data.items():
        acc = u.GlobalAccumulator()
        for a in (acc, total):
            a.add(y, preds[name], need & scored)
        r = acc.result()
        per[name] = np.array([r['t0'], r['t1'], r['weighted_pearson']])
    r = total.result()
    acc_total = np.array([r['t0'], r['t1'], r['weighted_pearson']])
    y = np.concatenate([v[0] for v in data.values()])
    p = np.concatenate([preds[n] for n in data])
    mask = np.concatenate([v[1] & v[2] for v in data.values()])
    ref = [u.weighted_pearson(y[mask, k], p[mask, k]) for k in range(2)]
    return per, acc_total, np.array(ref + [np.mean(ref)])


def check(label, u, metric, data, files, preds, pred_files):
    per, acc_total, ref_total = official(u, data, preds)
    res, out = run_metric(metric, [(files[n], pred_files[n]) for n in data])
    if res is None:
        print(f'{label:16s} FAIL: ./metric error: {out}')
        return False
    diffs = [abs(res['TOTAL'] - acc_total).max(), abs(res['TOTAL'] - ref_total).max()]
    diffs += [abs(res[n] - per[n]).max() for n in data if len(data) > 1]
    # every file on its own (single pair => only the TOTAL line)
    for n in list(data)[:3]:
        r1, _ = run_metric(metric, [(files[n], pred_files[n])])
        diffs.append(abs(r1['TOTAL'] - per[n]).max())
    d = max(diffs)
    ok = d <= TOL
    print(f'{label:16s} C++ WP={res["TOTAL"][2]:.9f}  GlobalAccumulator={acc_total[2]:.12f}  '
          f'reference={ref_total[2]:.12f}  max|diff|={d:.1e}  {"OK" if ok else "FAIL"}')
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--starterpack', default='wnn_connectome_starterpack')
    ap.add_argument('--metric', default='./metric')
    ap.add_argument('datadir')
    ap.add_argument('preddirs', nargs='+')
    args = ap.parse_args()
    sys.path.insert(0, str(Path(args.starterpack).resolve()))
    import utils as u

    files = {f.name: str(f) for f in sorted(Path(args.datadir).glob('seq-*.tsv'))}
    data = {n: load_data(f) for n, f in files.items()}
    ok = True
    for pd in args.preddirs:
        pf = {n: str(Path(pd) / n) for n in files}
        preds = {n: load_pred(pf[n], data[n][1]) for n in files}
        ok &= check(Path(pd).name, u, args.metric, data, files, preds, pf)

    # synthetic prediction sets
    rng = np.random.default_rng(1)
    meta = {n: load_tsv(f) for n, f in files.items()}
    with tempfile.TemporaryDirectory() as tmp:
        tests = {
            'random_x3': lambda y: (rng.standard_normal(y.shape) * 3).astype(np.float32),
            'noisy_target': lambda y: (y + rng.standard_normal(y.shape)).astype(np.float32),
            'constant': lambda y: np.full(y.shape, 0.5, np.float32),
            'perfect': lambda y: y.copy(),
            'negated_x5': lambda y: (-5 * y).astype(np.float32),
        }
        for label, fn in tests.items():
            preds, pf = {}, {}
            for n in files:
                y, need, _ = data[n]
                p = fn(y)
                p[~need] = np.nan
                preds[n] = p
                pf[n] = f'{tmp}/{label}-{n}'
                write_pred(pf[n], meta[n]['seq_ix'][0], meta[n]['step_in_seq'], p, np.nonzero(need)[0])
            ok &= check(label, u, args.metric, data, files, preds, pf)

        # one prediction row per data row (warm-up rows are ignored, may be nan)
        preds, pf = {}, {}
        for n in files:
            y, need, _ = data[n]
            p = (y * 0.3 + rng.standard_normal(y.shape)).astype(np.float32)
            p[~need] = np.nan
            preds[n] = p
            pf[n] = f'{tmp}/allrows-{n}'
            write_pred(pf[n], meta[n]['seq_ix'][0], meta[n]['step_in_seq'], p, range(len(need)))
        ok &= check('all_rows_layout', u, args.metric, data, files, preds, pf)

        # nonfinite required prediction: both must refuse
        n = next(iter(files))
        y, need, scored = data[n]
        p = y.copy()
        p[np.nonzero(need)[0][5], 1] = np.inf
        fn = f'{tmp}/inf-{n}'
        write_pred(fn, meta[n]['seq_ix'][0], meta[n]['step_in_seq'], p, np.nonzero(need)[0])
        try:
            u.GlobalAccumulator().add(y, load_pred(fn, need), need & scored)
            py_rejects = False
        except ValueError:
            py_rejects = True
        res, _ = run_metric(args.metric, [(files[n], fn)])
        good = py_rejects and res is None
        print(f'{"nonfinite":16s} official rejects: {py_rejects}, ./metric rejects: {res is None}  {"OK" if good else "FAIL"}')
        ok &= good

    print('ALL OK' if ok else 'SOME CHECKS FAILED')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
