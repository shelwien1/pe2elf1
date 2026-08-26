# What is left in mrpc's residual for a linear filter on its causal
# neighbours -- the measurement behind MODEL-IMPROVEMENTS.md section 5.
#
#   patch -p0 < tools/nlms_probe.patch && make mrpc-nocl
#   MRPC_RDUMP=/tmp/r.bin ./mrpc-nocl -C c image.bmp /dev/null
#   python3 tools/resid_fit.py /tmp/r.bin
#
# The dump is {int32 W, H, nc} then the signed residual of the *base*
# prediction, one short per component per pixel, in raster order.
#
# Three fits, all of them cheating in the codec's favour:
#   global LS   one filter for the whole plane, least squares
#   robust      the same by IRLS at ~L1, so outliers cannot drive it
#   oracle      a separate least-squares fit per 32x32 block, transmitted
#               free of charge -- an upper bound on any online learner
# and the order-0 entropy of what each leaves behind, which is what the
# coder would actually pay.
import numpy as np, struct, sys

DYX = [(0, -1), (-1, 0), (0, -2), (-1, -1), (-2, 0), (-1, 1), (0, -3), (-1, -2)]

def ent(x):
    v, c = np.unique(np.round(x).astype(int), return_counts=True)
    p = c/c.sum()
    return -(p*np.log2(p)).sum()

def irls(X, y, iters=8):
    w = np.zeros(X.shape[1])
    for _ in range(iters):
        wt = 1.0/(np.abs(y-X@w)+2.0)
        w = np.linalg.solve((X*wt[:, None]).T@X+1e-6*np.eye(X.shape[1]),
                            (X*wt[:, None]).T@y)
    return w

def main(path):
    d = open(path, 'rb').read()
    W, H, NC = struct.unpack('<iii', d[:12])
    a = np.frombuffer(d[12:], dtype='<i2').reshape(H, W, NC).astype(np.float64)
    tot = [0.0]*4
    print("  %-6s %8s %10s %8s %12s" % ("", "base", "global LS", "robust", "oracle 32x32"))
    for k in range(NC):
        r = a[:, :, k]
        ys, xs = np.mgrid[3:H, 3:W-3]
        y = r[ys, xs].ravel()
        X = np.stack([r[ys+dy, xs+dx].ravel() for dy, dx in DYX], 1)
        hb = ent(y)
        hl = ent(y-X@np.linalg.lstsq(X, y, rcond=None)[0])
        hi = ent(y-X@irls(X, y))
        parts = []
        for by in range(3, H, 32):
            for bx in range(3, W-3, 32):
                s = ((ys >= by)&(ys < by+32)&(xs >= bx)&(xs < bx+32)).ravel()
                if s.sum() < 200:
                    continue
                parts.append(y[s]-X[s]@np.linalg.lstsq(X[s], y[s], rcond=None)[0])
        ho = ent(np.concatenate(parts)) if parts else hb
        print("  comp %d %8.3f %10.3f %8.3f %12.3f" % (k, hb, hl, hi, ho))
        for i, v in enumerate((hb, hl, hi, ho)):
            tot[i] += v
    print("  %-6s %8.3f %10.3f %8.3f %12.3f   bits, summed over components"
          % ("total", tot[0], tot[1], tot[2], tot[3]))

if __name__ == '__main__':
    main(sys.argv[1])
