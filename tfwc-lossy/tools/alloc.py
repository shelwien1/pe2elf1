# allocation: choose a step multiplier per group to minimize rate under a loss budget
import numpy as np, json
BASE = 49832
def load_meas(path):
    return {l.split()[1]: int(l.split()[0]) - BASE for l in open(path)}
def fit_c(tab, meas_sets, floor=0.0):
    """meas_sets: list of (k, {group: dL}); c_g = sum(dL*E)/sum(E^2) over the points (LS through origin)"""
    c = {}
    for g in tab:
        num = den = 0.0
        for k, m in meas_sets:
            if g in m and str(k) in tab[g]:
                E = tab[g][str(k)][1]
                num += m[g] * E; den += E * E
        c[g] = max(num / den, floor) if den > 0 else None
    return c
def solve(tab, KS, c, budget, allowed=None):
    """returns (choice dict, predicted loss, rate saved)"""
    groups = [g for g in tab if c.get(g) is not None]
    def pick(lam):
        ch = {}; L = 0.0; dR = 0.0
        for g in groups:
            R0 = tab[g]['1.0'][0]
            best = None
            for k in KS:
                if allowed and k not in allowed.get(g, KS): continue
                R, E = tab[g][str(k)]
                l = c[g] * E if k != 1.0 else 0.0
                cost = R + lam * l
                if best is None or cost < best[0]: best = (cost, k, l, R0 - R)
            ch[g] = best[1]; L += best[2]; dR += best[3]
        return ch, L, dR
    lo, hi = 1e-6, 1e6
    for _ in range(100):
        mid = np.sqrt(lo * hi)
        ch, L, dR = pick(mid)
        if L > budget: lo = mid
        else: hi = mid
    return pick(hi)
