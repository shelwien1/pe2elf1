# heldout.py base.cost cand.cost : loss split into calibration prefix / held-out rest (bytes)
import numpy as np, sys
b = np.fromfile(sys.argv[1], np.float32).astype(np.float64); c = np.fromfile(sys.argv[2], np.float32).astype(np.float64)
n = 131072
for name, sl in [('prefix[0:128K)', slice(0, n)), ('heldout[128K:)', slice(n, None)), ('all', slice(None))]:
    bb = b[sl].sum() / 8; cc = c[sl].sum() / 8
    print("%-16s base %9.0f  cand %9.0f  diff %+7.0f  (%+.3f%%)" % (name, bb, cc, cc - bb, 100 * (cc - bb) / bb))
