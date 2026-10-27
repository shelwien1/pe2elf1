# compare the table's gcc sets with gcc's own -Q --help=target per -march
import os, re, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
TABLE = os.environ.get('TABLE', 'table.tsv')   # from dumptab
GCC = sys.argv[1:]          # command, e.g. .../xgcc -B.../gcc/
feat = {}                   # gcc flag -> feature
src = open(os.path.join(HERE, '..', 'x86targets.h')).read()
for name, cls, g, c in re.findall(r'\{ "([^"]+)", \'([CI])\', (0|"[^"]*"), (0|"[^"]*") \}', src):
  if g != '0': feat[g.strip('"')] = name
only64 = {'64bit', 'sahf', 'cx16', 'apxf', 'uintr'}
bad = 0; n = 0
for line in open(TABLE):
  nm, fl, gs, cs = line.rstrip('\n').split('\t')
  fl = int(fl)
  if not fl & 1: continue
  bits = '-m64' if fl & 4 else '-m32'
  p = subprocess.run(GCC + [bits, '-march=' + nm, '-Q', '--help=target', '-x', 'c', '/dev/null'], capture_output=True, text=True)
  if p.returncode: print('%-16s gcc refuses: %s' % (nm, p.stderr.strip().splitlines()[-1] if p.stderr else '?')); bad += 1; continue
  on = set()
  for m in re.finditer(r'^\s+(-m[\w.-]+)\s+\[enabled\]', p.stdout, re.M):
    if m.group(1) in feat: on.add(feat[m.group(1)])
  mine = {f for f in gs.split() if any(v == f for v in feat.values())}
  if bits == '-m32': mine -= only64; on -= only64
  n += 1
  if mine != on:
    bad += 1
    print('%-16s %s table-only: %s | gcc-only: %s' % (nm, bits, ' '.join(sorted(mine - on)), ' '.join(sorted(on - mine))))
print('gcc: %d targets checked, %d differ' % (n, bad))
