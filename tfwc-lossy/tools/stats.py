import numpy as np, struct
def load(path):
    b = open(path, 'rb').read(); p = 4
    n, = struct.unpack_from('<I', b, p); p += 4
    out = {}
    for _ in range(n):
        nl, = struct.unpack_from('<I', b, p); p += 4
        name = b[p:p+nl].decode(); p += nl
        do, di = struct.unpack_from('<ii', b, p); p += 8
        nf, nb = struct.unpack_from('<qq', b, p); p += 16
        def arr(k):
            nonlocal p
            a = np.frombuffer(b, np.float64, k, p); p += 8*k; return a.copy()
        A = arr(di*di).reshape(di, di); G = arr(do); F = arr(do*di).reshape(do, di)
        X1 = arr(di); X2 = arr(di)
        out[name] = dict(d_out=do, d_in=di, nf=nf, nb=nb, A=A, G=G, F=F, X1=X1, X2=X2)
    return out
