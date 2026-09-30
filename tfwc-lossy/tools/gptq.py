import numpy as np
def gptq_rdo(W, A, s, bits, lam, damp=0.01, act_order=True, clip=7):
    """W: (R,C) float targets; A: (C,C) input second moment (per token);
    s: (R,) new row scales; bits: (15,) code length per symbol (q=-7..7);
    lam: loss-per-bit tradeoff in units of A-weighted squared error.
    Returns Q (R,C) int8 and the predicted A-weighted error sum."""
    R, C = W.shape
    W = W.astype(np.float64).copy()
    H = A.astype(np.float64).copy()
    dead = np.diag(H) <= 0
    H[dead, dead] = 1.0
    W[:, dead] = 0.0
    H += damp * np.mean(np.diag(H)) * np.eye(C)
    perm = np.argsort(-np.diag(H)) if act_order else np.arange(C)
    W = W[:, perm]; H = H[perm][:, perm]
    Hinv = np.linalg.inv(H)
    U = np.linalg.cholesky(Hinv).T  # upper: Hinv = U^T U
    Q = np.zeros((R, C), np.int8)
    s = s.astype(np.float64)
    sz = np.where(s == 0, 1.0, s)
    levels = np.arange(-clip, clip + 1)
    tot = 0.0
    for j in range(C):
        w = W[:, j]; d = U[j, j]
        # candidate costs: ((w - q s)/d)^2 + lam * bits[q]
        x = w / sz
        q0 = np.clip(np.rint(x), -clip, clip)
        if lam > 0:
            cand = np.stack([np.clip(q0 + o, -clip, clip) for o in (-2, -1, 0, 1, 2)])  # (5, R)
            dist = ((w[None, :] - cand * s[None, :]) / d) ** 2
            rate = bits[(cand + 7).astype(int)]
            cost = dist + lam * rate
            q = cand[np.argmin(cost, axis=0), np.arange(R)]
        else:
            q = q0
        err = (w - q * s) / d
        tot += (err ** 2).sum()
        W[:, j + 1:] -= np.outer(err, U[j, j + 1:])
        Q[:, j] = q
    inv = np.argsort(perm)
    return Q[:, inv], tot
