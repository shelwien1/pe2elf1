# qmerge.py base.tfwc2 trained.q out.tfwc2 [scales] : replace the trained int4 matrices
# (with 'scales': the .q file also carries the rows' bf16 scales, from SLR training)
import numpy as np, struct, sys, subprocess, os, tempfile, wd, rq
tools = os.path.dirname(os.path.abspath(__file__))
fd, tmp = tempfile.mkstemp(suffix='.wd'); os.close(fd)
subprocess.check_call([os.path.join(tools, 'wdump'), 'x', sys.argv[1], tmp])
ts = wd.load(tmp); T = {t[0]: i for i, t in enumerate(ts)}
os.remove(tmp)
b = open(sys.argv[2], 'rb').read(); p = 0
n, = struct.unpack_from('<I', b, p); p += 4
changed = total = 0
for _ in range(n):
    nl, = struct.unpack_from('<I', b, p); p += 4
    name = b[p:p+nl].decode(); p += nl
    do, di = struct.unpack_from('<ii', b, p); p += 8
    q = np.frombuffer(b, np.int8, do * di, p).reshape(do, di).copy(); p += do * di
    if len(sys.argv) > 4:
        sc = np.frombuffer(b, np.uint16, do, p).copy(); p += 2 * do
        ts[T[name + '.weight.scale']][3] = sc
    old = ts[T[name + '.weight.q']][3]
    changed += int((old != q).sum()); total += q.size
    ts[T[name + '.weight.q']][3] = q
rq.write_tfwc2(ts, sys.argv[3])
print("%s: %d of %d weights changed (%.2f%%)" % (sys.argv[3], changed, total, 100.0 * changed / total))
