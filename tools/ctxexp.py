#!/usr/bin/env python3
"""Context experiments on h264p dumps (see h264p_cm.md).

For several syntax elements, prints the adaptive code length of the same
binary decisions under different context sets. The first set of each
element is the baseline, usually the contexts CABAC uses; the others add
contexts a context mixing model could use. Each decision is coded with one
adaptive probability per context and no mixing, so the numbers show what a
context is worth on its own.

Streams with frame macroblocks only (no PAFF or MBAFF), and without FMO.
Neighbours across slice boundaries are used. All files given share the
statistics; run it once per stream to start each from scratch.

usage: tools/ctxexp.py dump.tsv...
"""
import math
import sys
from collections import defaultdict

LIMIT = 255
LOG = math.log2


class Coder:
    """per experiment: adaptive binary probabilities by context"""
    def __init__(self):
        self.t = {}
        self.bits = 0.0
        self.n = 0

    def code(self, ctx, bit):
        s = self.t.get(ctx)
        if s is None:
            s = self.t[ctx] = [0.5, 0]
        p = s[0]
        self.bits -= LOG(p if bit else 1.0 - p)
        self.n += 1
        k = s[1]
        if k < LIMIT:
            s[1] = k + 1
        p += ((1.0 if bit else 0.0) - p) / (k + 1.5)
        s[0] = min(max(p, 1 / 4096), 1 - 1 / 4096)

    def unary(self, ctx, v, cap):
        """v >= 0 as unary bins (bin index in the context), escape above cap with Exp-Golomb bypass"""
        i = 0
        while i < cap:
            b = v > i
            self.code(ctx + (min(i, 3),), b)
            if not b:
                return
            i += 1
        # Exp-Golomb (k=0) of v - cap, bypass
        x = v - cap + 1
        self.bits += 2 * int(LOG(x)) + 1
        self.n += 1

    def bypass(self, n=1):
        self.bits += n
        self.n += n


EXPS = defaultdict(Coder)


def code(exp, ctx, bit):
    EXPS[exp].code(ctx, bit)


def blk_x(b): return (b & 1) | ((b >> 1) & 2)
def blk_y(b): return ((b >> 1) & 1) | ((b >> 2) & 2)


# partition shapes in 4x4 units
P16x16 = [(0, 0, 4, 4)]
P16x8 = [(0, 0, 4, 2), (0, 2, 4, 2)]
P8x16 = [(0, 0, 2, 4), (2, 0, 2, 4)]
SUBP = [[(0, 0, 2, 2)], [(0, 0, 2, 1), (0, 1, 2, 1)], [(0, 0, 1, 2), (1, 0, 1, 2)],
        [(0, 0, 1, 1), (1, 0, 1, 1), (0, 1, 1, 1), (1, 1, 1, 1)]]
BSUBPRED = [0, 1, 2, 3, 1, 1, 2, 2, 3, 3, 1, 2, 3]
BSUBSHAPE = [0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 3, 3, 3]
BPARTPRED = [(1, 1), (2, 2), (1, 2), (2, 1), (1, 3), (2, 3), (3, 1), (3, 2), (3, 3)]


def bucket(v, edges):
    for i, e in enumerate(edges):
        if v <= e:
            return i
    return len(edges)


def sgn(v): return 0 if v == 0 else (1 if v > 0 else 2)


class Mb:
    __slots__ = ('skip', 'cls', 'qp', 'cbp', 'nz', 'coef', 'mvd', 'ipm', 'intra', 'i16', 't8', 'inxn', 'mbtype', 'cdc')

    def __init__(self):
        self.skip = 0
        self.cls = 1
        self.qp = 0
        self.cbp = 0
        self.nz = [0] * 16
        self.coef = {}          # raster 4x4 index -> 16 coefficients (cat 2)
        self.mvd = [[(0, 0)] * 16, [(0, 0)] * 16]
        self.ipm = None         # raster 4x4 -> intra mode, for I_NxN
        self.intra = 0
        self.i16 = 0
        self.t8 = 0
        self.inxn = 0
        self.mbtype = -1
        self.cdc = None


def mb_class(st, t):
    """0 n/a, 1 skip, 2 direct16, 3 inter16x16, 4 inter 16x8/8x16, 5 inter8x8, 6 I_NxN, 7 I16, 8 PCM"""
    if st == 2:
        it = t
    elif st == 0:
        if t < 5:
            return [3, 4, 4, 5, 5][t]
        it = t - 5
    else:
        if t < 23:
            return 2 if t == 0 else 3 if t <= 3 else 5 if t == 22 else 4
        it = t - 23
    return 6 if it == 0 else 8 if it == 25 else 7


class Walker:
    def __init__(self):
        self.W = self.H = 0
        self.init_qp = 26
        self.nal_type = 0
        self.ref_idc = 0
        self.sh = {}
        self.pic = None
        self.prev = {}
        self.key = None
        self.addr = 0
        self.qp = 26
        self.last_qpd = 0
        self.cur = None   # rows of the current macroblock
        self.st = 0

    def run(self, path):
        for line in open(path):
            r = line.rstrip('\n').split('\t')
            n = r[0]
            if n == 'mb' or n == 'skip' or n == 'nal':
                self.flush()
            if n == 'nal':
                self.ref_idc = int(r[2])
                self.nal_type = int(r[3])
                continue
            t = self.nal_type
            if t == 7:
                if n == 'pic_width_in_mbs_minus1':
                    self.W = int(r[1]) + 1
                elif n == 'pic_height_in_map_units_minus1':
                    self.H = int(r[1]) + 1
                elif n == 'frame_mbs_only_flag' and r[1] != '1':
                    sys.exit('%s: interlaced streams are not supported' % path)
                continue
            if t == 8:
                if n == 'pic_init_qp_minus26':
                    self.init_qp = 26 + int(r[1])
                continue
            if t not in (1, 5):
                continue
            if n == 'first_mb_in_slice':
                first = int(r[1])
                if first == 0:
                    self.new_picture()
                self.addr = first
            elif n == 'hex':
                sys.exit('%s: NAL units stored verbatim are not supported' % path)
            elif n == 'slice_type':
                self.st = int(r[1]) % 5
                self.key = (self.st, self.ref_idc > 0)
            elif n == 'slice_qp_delta':
                self.qp = self.init_qp + int(r[1])
                self.last_qpd = 0
            elif n == 'skip':
                for _ in range(int(r[1])):
                    self.skipped()
            elif n == 'mb':
                self.cur = {'mb': r[1:]}
            elif self.cur is not None and n in ('ipm', 'sub', 'ref0', 'ref1', 'mvd0', 'mvd1'):
                self.cur[n] = [int(x) for x in r[1:]]
            elif self.cur is not None and n == 'res':
                self.cur.setdefault('res', []).append((int(r[1]), int(r[2]), [int(x) for x in r[3:]]))
        self.flush()

    def new_picture(self):
        if self.pic is not None and self.key is not None:
            self.prev[self.key] = self.pic
        self.pic = [None] * (self.W * self.H)

    # --- neighbours
    def nbs(self, addr):
        x, y = addr % self.W, addr // self.W
        a = self.pic[addr - 1] if x > 0 else None
        b = self.pic[addr - self.W] if y > 0 else None
        p = self.prev.get(self.key)
        col = p[addr] if p is not None else None
        return a, b, col

    def col_area(self, addr):
        p = self.prev.get(self.key)
        if p is None:
            return None
        x, y = addr % self.W, addr // self.W
        out = []
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                xx, yy = x + dx, y + dy
                if 0 <= xx < self.W and 0 <= yy < self.H:
                    out.append(p[yy * self.W + xx])
        return out

    # --- skip flags
    def skip_flag(self, addr, skip):
        a, b, col = self.nbs(addr)
        k = self.key
        s3 = lambda m: 0 if m is None else 1 if m.skip else 2
        code('skip:cabac', (k, (a is not None and not a.skip) + (b is not None and not b.skip)), skip)
        code('skip:a,b', (k, s3(a), s3(b)), skip)
        code('skip:a,b,col', (k, s3(a), s3(b), s3(col)), skip)
        area = self.col_area(addr)
        ns = -1 if area is None else bucket(sum(1 for m in area if m is not None and m.skip), (0, 2, 4, 6, 8))
        code('skip:a,b,col,col3x3', (k, s3(a), s3(b), s3(col), ns), skip)

    def skipped(self):
        addr = self.addr
        if self.st != 2:
            self.skip_flag(addr, 1)
        m = Mb()
        m.skip = 1
        m.qp = self.qp
        m.cls = 1
        self.pic[addr] = m
        self.last_qpd = 0
        self.addr += 1

    def flush(self):
        if self.cur is None:
            return
        rows = self.cur
        self.cur = None
        addr = self.addr
        st = self.st
        k = self.key
        if st != 2:
            self.skip_flag(addr, 0)
        f = rows['mb']
        t = int(f[0])
        t8 = 0 if f[1] == '-' else int(f[1])
        cbp_f = None if f[3] == '-' else int(f[3])
        qpd = None if f[4] == '-' else int(f[4])
        m = Mb()
        m.mbtype = t
        m.cls = mb_class(st, t)
        m.t8 = t8
        a, b, col = self.nbs(addr)
        cl = lambda n: 0 if n is None else n.cls

        # --- mb_type: binary decomposition of the 6-bit value
        def code_type(exp, ctx):
            node = 1
            for i in range(5, -1, -1):
                bit = t >> i & 1
                code(exp, ctx + (node,), bit)
                node = node * 2 + bit
        code_type('mb_type:slice type', (k,))
        code_type('mb_type:a,b', (k, cl(a), cl(b)))
        code_type('mb_type:a,b,col', (k, cl(a), cl(b), cl(col)))

        intra = m.cls >= 6
        m.intra = intra
        m.inxn = m.cls == 6
        m.i16 = m.cls == 7
        if m.i16:
            it = t - (0 if st == 2 else 5 if st == 0 else 23)
            cbp = (15 if (it - 1) // 12 else 0) | ((it - 1) // 4 % 3) << 4
        else:
            cbp = cbp_f if cbp_f is not None else 0
        m.cbp = cbp

        # --- intra 4x4 / 8x8 prediction modes
        if m.inxn and 'ipm' in rows:
            self.intra_modes(m, rows['ipm'], a, b, t8)

        # --- mvd
        if not intra and m.cls != 1:
            self.motion(m, rows, a, b, col, t)

        # --- mb_qp_delta
        if qpd is not None:
            qcol = col.qp if col is not None else self.qp
            qb = b.qp if b is not None else self.qp
            d = qcol - self.qp
            db = qb - self.qp
            v = 2 * qpd - 1 if qpd > 0 else -2 * qpd
            c = EXPS
            c['qpd:cabac'].unary((k, self.last_qpd != 0), v, 52)
            c['qpd:col'].unary((k, self.last_qpd != 0, max(-4, min(4, d))), v, 52)
            c['qpd:col,above'].unary((k, self.last_qpd != 0, max(-4, min(4, d)), max(-4, min(4, db))), v, 52)
            c['qpd:cbp,class'].unary((k, self.last_qpd != 0, bin(cbp & 15).count('1'), cbp >> 4, m.cls), v, 52)
            c['qpd:last value'].unary((k, max(-3, min(3, self.last_qpd))), v, 52)
            self.qp = (self.qp + qpd + 52) % 52
            self.last_qpd = qpd
        else:
            self.last_qpd = 0
        m.qp = self.qp

        # --- residual: luma 4x4 blocks (cat 2), cbf and coefficients
        blocks = {}
        cdc = {}
        for cat, idx, c in rows.get('res', []):
            if cat in (1, 2):
                blocks[idx] = (cat, c)
            elif cat == 5:
                nzc = sum(1 for v in c if v)
                for j in range(4):
                    bi = idx * 4 + j
                    m.nz[blk_y(bi) * 4 + blk_x(bi)] = nzc
            elif cat == 3:
                cdc[idx] = c
        self.pic[addr] = m   # so that blocks of this macroblock see each other
        if cbp >> 4:
            m.cdc = [cdc.get(0), cdc.get(1)]
            for comp in (0, 1):
                c = cdc.get(comp)
                cbf = int(c is not None)
                def dc_cond(nb):
                    if nb is None:
                        return int(m.intra)
                    return int(nb.cdc is not None and nb.cdc[comp] is not None)
                code('chroma DC cbf:cabac', (k[0], comp, dc_cond(a), dc_cond(b)), cbf)
                code('chroma DC cbf:+Cb for Cr', (k[0], comp, dc_cond(a), dc_cond(b),
                                                  -1 if comp == 0 else int(cdc.get(0) is not None)), cbf)
                if c is None:
                    continue
                cb = cdc.get(0) if comp == 1 else None
                for i, v in enumerate(c):
                    if not v:
                        continue
                    EXPS['chroma DC signs:none'].bypass()
                    sc = 3 if cb is None or i >= len(cb) else sgn(cb[i])
                    code('chroma DC signs:Cb same pos', (k[0], comp, i, sc), int(v < 0))
        if not t8:
            for b8 in range(4):
                if not (cbp >> b8 & 1):
                    continue
                for j in range(4):
                    bi = b8 * 4 + j
                    x, y = blk_x(bi), blk_y(bi)
                    cat, c = blocks.get(bi, (1 if m.i16 else 2, []))
                    self.luma_block(m, addr, x, y, cat, c)
        self.addr += 1

    def nb4(self, m, addr, x, y):
        """left and above 4x4 neighbours: (mb, raster idx)"""
        if x > 0:
            A = (m, y * 4 + x - 1)
        else:
            a = self.pic[addr - 1] if addr % self.W > 0 else None
            A = (a, y * 4 + 3) if a is not None else (None, 0)
        if y > 0:
            B = (m, (y - 1) * 4 + x)
        else:
            b = self.pic[addr - self.W] if addr >= self.W else None
            B = (b, 12 + x) if b is not None else (None, 0)
        return A, B

    def luma_block(self, m, addr, x, y, cat, c):
        k = self.key
        n = 16 if cat == 2 else 15
        c = c + [0] * (n - len(c))
        nzc = sum(1 for v in c if v)
        A, B = self.nb4(m, addr, x, y)

        def cond(N):
            mb, i = N
            if mb is None:
                return int(m.intra)
            if mb.skip:
                return 0
            b8 = (i >> 3) * 2 + ((i & 3) >> 1)
            if not (mb.cbp >> b8 & 1):
                return 0
            return int(mb.nz[i] != 0)
        nA = A[0].nz[A[1]] if A[0] is not None else -1
        nB = B[0].nz[B[1]] if B[0] is not None else -1
        if nA >= 0 and nB >= 0:
            nc = (nA + nB + 1) >> 1
        else:
            nc = max(nA, nB, 0)
        ncb = bucket(nc, (0, 1, 2, 4, 7))
        cbf = int(nzc > 0)
        code('cbf luma4x4:cabac', (k[0], cat, cond(A), cond(B)), cbf)
        code('cbf luma4x4:nA,nB', (k[0], cat, bucket(nA, (-1, 0, 1, 3)), bucket(nB, (-1, 0, 1, 3))), cbf)
        code('cbf luma4x4:nA,nB,intra,qp', (k[0], cat, bucket(nA, (-1, 0, 1, 3)), bucket(nB, (-1, 0, 1, 3)),
                                            m.intra, m.qp // 6), cbf)
        m.nz[y * 4 + x] = nzc
        if not cbf:
            return
        m.coef[y * 4 + x] = c
        last = max(i for i in range(n) if c[i])
        cnt = 0
        for i in range(n - 1):
            s = int(c[i] != 0)
            code('sig/last luma4x4:cabac', (k[0], cat, 's', i), s)
            code('sig/last luma4x4:pos,nC', (k[0], cat, 's', i, ncb), s)
            code('sig/last luma4x4:pos,nC,count', (k[0], cat, 's', i, ncb, min(cnt, 3)), s)
            if s:
                l = int(i == last)
                code('sig/last luma4x4:cabac', (k[0], cat, 'l', i), l)
                code('sig/last luma4x4:pos,nC', (k[0], cat, 'l', i, ncb), l)
                code('sig/last luma4x4:pos,nC,count', (k[0], cat, 'l', i, ncb, min(cnt, 3)), l)
                cnt += 1
                if l:
                    break
        # levels in reverse scan order, CABAC style, and signs
        eq1 = gt1 = 0
        ca = A[0].coef.get(A[1]) if A[0] is not None else None
        cb = B[0].coef.get(B[1]) if B[0] is not None else None
        for i in range(last, -1, -1):
            v = c[i]
            if not v:
                continue
            av = abs(v) - 1
            c0 = (k[0], cat, 0 if gt1 else min(4, 1 + eq1))
            c1 = (k[0], cat, min(4, gt1))
            pb = bucket(i, (0, 2, 5))
            EXPS['levels luma4x4:cabac'].code(c0 + ('f',), av > 0)
            EXPS['levels luma4x4:+nC,pos'].code(c0 + ('f', ncb, pb), av > 0)
            if av > 0:
                EXPS['levels luma4x4:cabac'].unary(c1, av - 1, 13)
                EXPS['levels luma4x4:+nC,pos'].unary(c1 + (ncb, pb), av - 1, 13)
            neg = int(v < 0)
            EXPS['signs luma4x4:none'].bypass()
            sa = sgn(ca[i]) if ca is not None and i < len(ca) else 3
            sb = sgn(cb[i]) if cb is not None and i < len(cb) else 3
            code('signs luma4x4:same pos in A,B', (k[0], cat, min(i, 3), sa, sb), neg)
            if av:
                gt1 += 1
            else:
                eq1 += 1

    def intra_modes(self, m, ipm, a, b, t8):
        """reconstruct Intra4x4/8x8 modes (8.3.1.1, 8.3.2.1); constrained_intra_pred_flag 0 assumed"""
        k = self.key
        modes = [2] * 16
        m.ipm = modes
        addr = self.addr

        def nmode(mb, i, is8, Nis_a):
            if mb is None:
                return -1          # dcPredModePredictedFlag
            if not mb.inxn or mb.ipm is None:
                return 2
            if mb is m:
                return mb.ipm[i]
            if is8 and not mb.t8:
                # Intra4x4 neighbour of an 8x8 block: its 4x4 block n = 1 (A) or 2 (B) of the 8x8 block
                b8x, b8y = (i % 4) >> 1, (i // 4) >> 1
                base = b8y * 8 + b8x * 2
                return mb.ipm[base + (1 if Nis_a else 4)]
            return mb.ipm[i]
        nblk = 4 if t8 else 16
        prev = -1
        for j in range(nblk):
            if t8:
                x, y = (j & 1) * 2, (j >> 1) * 2
            else:
                x, y = blk_x(j), blk_y(j)
            A, B = self.nb4(m, addr, x, y)
            ma = nmode(A[0], A[1], t8, True)
            mb_ = nmode(B[0], B[1], t8, False)
            pred = 2 if ma < 0 or mb_ < 0 else min(ma, mb_)
            r = ipm[j]
            mode = pred if r < 0 else (r if r < pred else r + 1)
            # CABAC: flag with one context, 3 rem bits with one context
            code('intra modes:cabac', ('f',), int(r < 0))
            if r >= 0:
                for bit in range(3):
                    code('intra modes:cabac', ('r',), r >> bit & 1)
            # actual mode with the neighbours' modes as context
            for ename, ctx in (('intra modes:modeA,modeB', (t8, ma, mb_)),
                               ('intra modes:modeA,modeB,prev', (t8, ma, mb_, prev))):
                node = 1
                for i in range(3, -1, -1):
                    bit = mode >> i & 1
                    code(ename, ctx + (node,), bit)
                    node = node * 2 + bit
            prev = mode
            if t8:
                for dy in range(2):
                    for dx in range(2):
                        modes[(y + dy) * 4 + x + dx] = mode
            else:
                modes[y * 4 + x] = mode

    def motion(self, m, rows, a, b, col, t):
        st = self.st
        k = self.key
        parts = []   # (list, x, y, w, h) in mvd row order
        if st == 0:
            if t < 3:
                ps = [P16x16, P16x8, P8x16][t]
                parts = [(0,) + p for p in ps]
            else:
                sub = rows.get('sub', [0, 0, 0, 0])
                for i in range(4):
                    for (x, y, w, h) in SUBP[sub[i]]:
                        parts.append((0, x + (i & 1) * 2, y + (i >> 1) * 2, w, h))
        else:
            if t == 0:
                return
            if t <= 3:
                pred, ps = [t], P16x16
            elif t < 22:
                pred, ps = BPARTPRED[(t - 4) // 2], (P8x16 if t & 1 else P16x8)
            else:
                pred = None
            if pred is not None:
                for l in (0, 1):
                    for i, p in enumerate(ps):
                        if pred[i] & (1 << l):
                            parts.append((l,) + p)
            else:
                sub = rows.get('sub', [0, 0, 0, 0])
                for l in (0, 1):
                    for i in range(4):
                        if BSUBPRED[sub[i]] & (1 << l):
                            for (x, y, w, h) in SUBP[BSUBSHAPE[sub[i]]]:
                                parts.append((l, x + (i & 1) * 2, y + (i >> 1) * 2, w, h))
        vals = {0: rows.get('mvd0', []), 1: rows.get('mvd1', [])}
        pos = {0: 0, 1: 0}
        addr = self.addr
        for (l, x, y, w, h) in parts:
            v = vals[l]
            i = pos[l]
            pos[l] += 2
            if i + 1 >= len(v):
                return
            mx, my = v[i], v[i + 1]
            A, B = self.nb4(m, addr, x, y)
            na = A[0].mvd[l][A[1]] if A[0] is not None else (0, 0)
            nb = B[0].mvd[l][B[1]] if B[0] is not None else (0, 0)
            nc = col.mvd[l][y * 4 + x] if col is not None else (0, 0)
            for comp, val in ((0, mx), (1, my)):
                s = abs(na[comp]) + abs(nb[comp])
                cab = 0 if s < 3 else 2 if s > 32 else 1
                av = abs(val)
                ctx_c = (k[0], comp, cab)
                fine = (k[0], comp, bucket(abs(na[comp]), (0, 1, 3, 8, 32)), bucket(abs(nb[comp]), (0, 1, 3, 8, 32)))
                other = bucket(abs(mx), (0, 1, 3, 8, 32)) if comp == 1 else -1
                colb = bucket(abs(nc[comp]), (0, 1, 3, 8, 32))
                for ename, ctx in (('mvd:cabac', ctx_c), ('mvd:|A|,|B|', fine), ('mvd:|A|,|B|,other comp', fine + (other,)),
                                   ('mvd:|A|,|B|,other comp,col', fine + (other, colb))):
                    c = EXPS[ename]
                    c.code(ctx + ('z',), av > 0)
                    if av > 0:
                        c.unary(ctx + ('u',), av - 1, 8)
                if av:
                    EXPS['mvd signs:none'].bypass()
                    code('mvd signs:A,B,other', (k[0], comp, sgn(na[comp]), sgn(nb[comp]),
                                                sgn(mx) if comp == 1 else 3), int(val < 0))
            for yy in range(y, y + h):
                for xx in range(x, x + w):
                    m.mvd[l][yy * 4 + xx] = (mx, my)


for path in sys.argv[1:]:
    Walker().run(path)
groups = defaultdict(list)
for name, c in EXPS.items():
    groups[name.split(':')[0]].append((name.split(':')[1], c))
for g, lst in groups.items():
    base = lst[0][1].bits
    print('%-24s %10d decisions' % (g, lst[0][1].n))
    for nm, c in lst:
        print('    %-34s %12.0f bits  %+6.1f%%' % (nm, c.bits, 100.0 * (c.bits - base) / base if base else 0))
