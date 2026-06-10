#!/usr/bin/env python3
# Scaling model for ZipCrypto key recovery via CBMC+kissat: fit the measured
# kissat solve times to time(N) ~ A * 2^(c*N) and extrapolate to more unknown
# bits. Data measured on this machine (kissat 1.0.3, cbmc 5.95.1, 4 cores);
# absolute times are hardware-dependent, the *exponent* is the point.
import math

# (unknown bits N, solver seconds). Three measured curves; cadical on the forward
# model is the fastest known combination (see "Solver choice" in zipcrypt.md S9).
REVERSE_2F      = [(8, 0.22), (12, 2.8), (16, 27.9)]            # reverse + kissat_inc
FORWARD_3F      = [(8, 1.1),  (12, 5.2), (16, 65.3), (20, 1042.2)]  # forward + kissat_inc
FORWARD_CADICAL = [(16, 21.0), (20, 185.0)]                    # forward + cadical 3.0.0  <-- fastest

def fit(pts):                                    # ln t = a + b N  (least squares)
    n = len(pts); sx = sum(N for N, _ in pts); sy = sum(math.log(t) for _, t in pts)
    sxx = sum(N * N for N, _ in pts); sxy = sum(N * math.log(t) for N, t in pts)
    b = (n * sxy - sx * sy) / (n * sxx - sx * sx)
    return math.exp((sy - b * sx) / n), b / math.log(2)   # A, c (base-2 per-bit)

def fmt(s):
    if s < 90:        return f"{s:.1f} s"
    if s < 5400:      return f"{s/60:.1f} min"
    if s < 2*86400:   return f"{s/3600:.1f} h"
    if s < 3*365*86400: return f"{s/86400:.1f} days"
    y = s / (365.25 * 86400)
    return f"{y:.1f} years" if y < 1e4 else f"{y:.1e} years"

for tag, pts in (("reverse + kissat_inc (best for N<=16)", REVERSE_2F),
                 ("forward + kissat_inc", FORWARD_3F),
                 ("forward + cadical 3.0.0  (FASTEST known)", FORWARD_CADICAL)):
    A, c = fit(pts)
    print(f"\n## {tag}")
    print(f"   measured: " + "  ".join(f"{N}b={t}s" for N, t in pts))
    print(f"   fit: time ~= {A:.2e} * 2^({c:.2f} * N) s    (x{2**(8*c):.0f} per +8 bits)")
    print(f"   {'N':>3} {'extrapolated time':>18}  files~ceil(N/11)")
    for N in (24, 32, 40, 48, 56, 64, 80, 96):
        print(f"   {N:>3} {fmt(A * 2**(c*N)):>18}  {max(1, -(-N//11))}")

print("\nNote: forward's measured ratios accelerate (4.7x ->12.5x ->16x per +4 bits),")
print("so the extrapolations are optimistic lower bounds. The exponential wall makes")
print("~32 bits the practical ceiling and full 96-bit recovery astronomically infeasible.")
