# lossy image codec experiment: every int4 tensor -> grey PNG (q+7)*18 -> codec -> decode -> round
import numpy as np, wd, rq, subprocess, os, sys, tempfile
from PIL import Image
def run(codec, param, out_tfwc2, only=None):
    ts = wd.load('orig.wd'); T = {t[0]: i for i, t in enumerate(ts)}
    tmp = tempfile.mkdtemp(dir='/tmp/claude-0')
    total = 0; nerr = 0; ntot = 0; mse = 0.0
    for qn, sn in rq.pairs(ts):
        if only and not any(o in qn for o in only): continue
        q = ts[T[qn]][3].astype(np.int32)
        img = ((q + 7) * 18).astype(np.uint8)
        png = os.path.join(tmp, 'a.png'); Image.fromarray(img).save(png)
        if codec == 'jxl':
            enc = os.path.join(tmp, 'a.jxl'); dec = os.path.join(tmp, 'b.png')
            subprocess.check_call(['cjxl', png, enc, '-d', str(param), '-e', '7', '--quiet'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.check_call(['djxl', enc, dec, '--quiet'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            enc = os.path.join(tmp, 'a.webp'); dec = os.path.join(tmp, 'b.png')
            subprocess.check_call(['cwebp', '-q', str(param), '-m', '6', '-quiet', png, '-o', enc])
            subprocess.check_call(['dwebp', '-quiet', enc, '-o', dec])
        total += os.path.getsize(enc)
        r = np.asarray(Image.open(dec).convert('L')).astype(np.float64)
        q2 = np.clip(np.rint(r / 18.0) - 7, -7, 7).astype(np.int8)
        nerr += (q2 != q).sum(); ntot += q.size; mse += ((q2 - q)**2).sum()
        ts[T[qn]][3] = q2
    sz = rq.write_tfwc2(ts, out_tfwc2)
    print("%s %s: image bytes %d, changed %.2f%%, mse %.3f (q units), tfwc2 %d" % (codec, param, total, 100.0*nerr/ntot, mse/ntot, sz))
if __name__ == '__main__':
    run(sys.argv[1], sys.argv[2], sys.argv[3])
