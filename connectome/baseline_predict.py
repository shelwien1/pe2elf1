"""Run the starter pack's baseline (stateful GRU, ONNX) on data TSVs.

usage: python baseline_predict.py [--starterpack DIR] OUTDIR data/seq-*.tsv

Writes OUTDIR/<same name>.tsv with columns seq_ix, step_in_seq, t0, t1
(one row per need_prediction row, same format as ./predict output).
Requires numpy, onnxruntime and the starter pack's baseline/ directory.
"""
import argparse
import sys
from pathlib import Path
import numpy as np


def load_tsv(fn):
    with open(fn) as f:
        names = f.readline().rstrip('\n').split('\t')
    a = np.loadtxt(fn, delimiter='\t', skiprows=1, dtype=str, ndmin=2)
    return {c: a[:, j] for j, c in enumerate(names)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--starterpack', default='wnn_connectome_starterpack')
    ap.add_argument('outdir')
    ap.add_argument('data', nargs='+')
    args = ap.parse_args()
    sp = Path(args.starterpack).resolve()
    sys.path.insert(0, str(sp))
    sys.path.insert(0, str(sp / 'baseline'))
    from utils import DataPoint, FEATURE_COLUMNS
    from solution import PredictionModel

    out = Path(args.outdir)
    out.mkdir(parents=True, exist_ok=True)
    model = PredictionModel()  # one instance; it resets itself when seq_ix changes
    for fn in args.data:
        d = load_tsv(fn)
        seq = d['seq_ix'].astype(np.int64)
        step = d['step_in_seq'].astype(np.int64)
        need = d['need_prediction'].astype(np.int64) != 0
        x = np.column_stack([d[c].astype(np.float32) for c in FEATURE_COLUMNS])
        rows = []
        for i in range(len(seq)):
            p = model.predict(DataPoint(int(seq[i]), int(step[i]), bool(need[i]), x[i]))
            if need[i]:
                rows.append((seq[i], step[i], np.asarray(p, dtype=np.float32)))
        p = np.array([r[2] for r in rows], dtype=np.float32)
        s0, s1 = p[:, 0].astype(str), p[:, 1].astype(str)  # shortest exact float32 text
        ofn = out / Path(fn).name
        with open(ofn, 'w', newline='\n') as f:
            f.write('seq_ix\tstep_in_seq\tt0\tt1\n')
            f.writelines(f'{r[0]}\t{r[1]}\t{a}\t{b}\n' for r, a, b in zip(rows, s0, s1))
        print(f'{fn}: {len(rows)} predictions -> {ofn}')


if __name__ == '__main__':
    main()
