"""Extract sequences (parquet row groups) from the starter pack's parquet files to TSV.

usage: python extract.py datasets/valid.parquet OUTDIR [row_group ...]
       (default row groups: 0..9)

Writes OUTDIR/seq-<seq_ix>.tsv with the same columns as the parquet file
(header line with column names). Floats are written in the shortest form
that reads back (strtof/np.float32) to exactly the same float32 value;
booleans as 0/1. Every file is re-read and compared bit-exactly.
"""
import sys
from pathlib import Path
import numpy as np
import pyarrow.parquet as pq


def column_text(col):
    a = col.to_numpy(zero_copy_only=False)
    if a.dtype == np.bool_:
        return np.where(a, '1', '0')
    return a.astype(str)  # float32 -> shortest round-trip repr, ints as-is


def main():
    src, outdir = sys.argv[1], Path(sys.argv[2])
    groups = [int(x) for x in sys.argv[3:]] or list(range(10))
    outdir.mkdir(parents=True, exist_ok=True)
    pf = pq.ParquetFile(src)
    for g in groups:
        t = pf.read_row_group(g)
        seq = t['seq_ix'][0].as_py()
        fn = outdir / f'seq-{seq}.tsv'
        cols = [column_text(t[c]) for c in t.column_names]
        with open(fn, 'w', newline='\n') as f:
            f.write('\t'.join(t.column_names) + '\n')
            f.writelines('\t'.join(row) + '\n' for row in zip(*cols))
        # verify: re-read and compare bit-exactly
        back = np.loadtxt(fn, delimiter='\t', skiprows=1, dtype=str)
        for j, c in enumerate(t.column_names):
            a = t[c].to_numpy(zero_copy_only=False)
            b = back[:, j].astype(np.float32).astype(a.dtype) if a.dtype == np.float32 else back[:, j].astype(np.int64).astype(a.dtype)
            if not np.array_equal(a.view(np.uint32) if a.dtype == np.float32 else a,
                                  b.view(np.uint32) if b.dtype == np.float32 else b):
                raise SystemExit(f'{fn}: column {c} does not round-trip')
        print(f'row group {g}: {fn} ({t.num_rows} rows, {int(t["is_scored"].to_numpy().sum()) if "is_scored" in t.column_names else "-"} scored) OK')


if __name__ == '__main__':
    main()
