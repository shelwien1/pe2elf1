#!/bin/bash
# runtable.sh MODE NSLICES
cd "$(dirname "$0")"
for i in $(seq 0 $(($2-1))); do OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1 python3 table.py $1 $i $2 & done; wait
python3 - "$1" "$2" <<'PY'
import json, sys
mode, n = sys.argv[1], int(sys.argv[2])
out = None
for i in range(n):
    d = json.load(open('../runs/table_%s_%d.json' % (mode, i)))
    if out is None: out = d
    else:
        for g, v in d['tab'].items():
            for k, (r, e) in v.items():
                x = out['tab'].setdefault(g, {}).setdefault(k, [0.0, 0.0]); x[0] += r; x[1] += e
json.dump(out, open('../runs/table_%s.json' % mode, 'w'))
print("merged", len(out['tab']))
PY
