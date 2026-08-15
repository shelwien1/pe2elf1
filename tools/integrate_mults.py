#!/usr/bin/env python3
# integrate_mults.py -- sync a sh_model .inc file back into the matching
# sh_model .idx: fold multipliers, extract hand-edited values, add new params.
#
# .idx line:  Number <name>, <K>,<D>!<bits>            ->  PREFIX_name = (bits+D)*K
# .inc line:  static const int PREFIX_name = (X+D) * (M);
#
# The .inc is idx2inc output: canonically X = bits, D and the "* (K)" slot mirror
# the .idx encoding. Three kinds of .inc edits are folded back:
#   mult   M != K            -> value trunc((X+D)*M) re-encoded into the bitfield
#   value  M == K, X != bits -> bitfield updated to X
#   new    name not in .idx  -> Number entry appended to its Prefix block;
#          K = M when M is integral, else K = 1 with the value folded;
#          width = bit_length+1 (headroom so the optimizer can move up)
# In all cases the .inc line is rewritten to canonical "(X'+D) * (K)".
#
# Value semantics: C++ evaluates "(X+D) * (M)" as double and TRUNCATES on the
# int conversion (e.g. 185*1.4 = 258.999... -> 258, not 259). The fold replicates
# that exactly, so the updated .idx/.inc reproduce the measured binary
# bit-for-bit. The bitfield keeps its width if the new value fits, else expands
# to minimal width. Entries whose .inc D disagrees with the .idx are skipped
# with a warning, as are values clamped at 0, and new params whose name matches
# no Prefix in the .idx.
#
# usage: integrate_mults.py [sh_model-C0.idx] [sh_model-C0.inc]   (updates in place)

import re, sys, math
from fractions import Fraction

def main():
  idx_path = sys.argv[1] if len(sys.argv) > 1 else 'sh_model-C0.idx'
  inc_path = sys.argv[2] if len(sys.argv) > 2 else 'sh_model-C0_h.inc'

  idx_lines = open(idx_path, 'r', newline='').read().split('\n')
  inc_lines = open(inc_path, 'r', newline='').read().split('\n')
  eol = '\r' if any(l.endswith('\r') for l in idx_lines[:10]) else ''

  # ---- parse .idx ------------------------------------------------------
  num_re = re.compile(r'^(\s*Number\s+(\w+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*!)([01]+)(\s*)$')
  pre_re = re.compile(r'^\s*Prefix\s+(\S+)\s*$')
  prefix = ''
  blocks = []                       # [prefix, start line, end line (excl)]
  params = {}                       # full const name -> record
  for i, l in enumerate(idx_lines):
    m = pre_re.match(l)
    if m:
      if blocks: blocks[-1][2] = i
      prefix = m.group(1)
      blocks.append([prefix, i, len(idx_lines)])
      continue
    m = num_re.match(l)
    if m:
      full = (prefix + '_' if prefix else '') + m.group(2)
      if full in params:
        print('!! duplicate Number "%s" in %s, using first' % (full, idx_path))
        continue
      params[full] = dict(line=i, head=m.group(1), K=int(m.group(3)),
                          D=int(m.group(4)), bits=m.group(5), tail=m.group(6))
  if not blocks:
    blocks = [['', 0, len(idx_lines)]]

  # ---- find edited / new entries in the .inc ---------------------------
  inc_re = re.compile(r'^(\s*static\s+const\s+int\s+(\w+)\s*=\s*)\((-?\d+)\+(-?\d+)\)\s*\*\s*\((\d+(?:\.\d+)?)\)(\s*;.*)$')
  uses, news, seen = {}, [], set()  # known name -> use; unknown -> ordered list
  for j, l in enumerate(inc_lines):
    m = inc_re.match(l)
    if not m: continue
    name = m.group(2)
    if name in seen:
      print('!! duplicate "%s" in %s, using first' % (name, inc_path))
      continue
    seen.add(name)
    rec = (j, int(m.group(3)), int(m.group(4)), m.group(5))
    if name in params: uses[name] = rec
    else:              news.append((name,) + rec)

  # ---- integrate value / multiplier edits ------------------------------
  half = Fraction(1, 2)
  hdr = '%-12s %-5s %6s %9s %11s %13s %11s %8s %6s' % \
        ('param','kind','mult','old bits','old (X+D)*K','C++ (X+D)*M','new (X+D)*K','rnd.err','width')
  shown = False
  for name in sorted(uses):
    j, X, D, Ms = uses[name]
    p    = params[name]
    K    = p['K']
    old  = int(p['bits'], 2)
    Mf   = Fraction(Ms)
    if D != p['D']:
      print('!! %s: .inc D=%d does not match .idx D=%d -- SKIPPED' % (name, D, p['D']))
      continue
    if Mf == K and X == old:       # canonical and in sync
      continue
    if not shown:
      print(hdr); print('-'*len(hdr)); shown = True
    oldv = (old + D) * K
    # replicate C++ double multiply + int truncation of the .inc expression
    tgt  = int(float(X + D) * float(Ms))
    new  = math.floor(Fraction(tgt, K) - D + half)  # nearest encodable, exact for K=1
    if new < 0:
      print('!! %s: encoded field %d < 0, clamped to 0' % (name, new))
      new = 0
    newv  = (new + D) * K
    width = max(len(p['bits']), new.bit_length(), 1)
    idx_lines[p['line']] = p['head'] + format(new, '0%db' % width) + p['tail']
    m = inc_re.match(inc_lines[j])
    inc_lines[j] = '%s(%d+%d) * (%d)%s' % (m.group(1), new, D, K, m.group(6))
    exact = Fraction(X + D) * Mf
    err   = newv - exact
    print('%-12s %-5s %6s %9d %11d %13d %11d %8s %6s' %
          (name, 'mult' if Mf != K else 'value',
           float(Mf) if Mf.denominator > 1 else Mf.numerator,
           old, oldv, tgt, newv,
           float(err) if err else 0,
           '%d->%d' % (len(p['bits']), width) if width != len(p['bits'])
                                              else str(width)))
  if not shown:
    print('no value or multiplier edits to integrate')

  # ---- add .inc params missing from the .idx ---------------------------
  by_len = sorted(blocks, key=lambda b: -len(b[0]))
  adds   = {}                       # block index -> [Number line, ...]
  hdr2 = '%-12s %-8s %3s %3s %9s %6s' % ('new param','prefix','K','D','value','width')
  shown2 = False
  for name, j, X, D, Ms in news:
    blk = next((b for b in by_len if b[0] and name.startswith(b[0] + '_')), None)
    if blk is None and blocks[0][0] == '':
      blk = blocks[0]
    if blk is None:
      print('!! %s: no matching Prefix in %s -- SKIPPED' % (name, idx_path))
      continue
    short = name[len(blk[0]) + 1:] if blk[0] else name
    Mf    = Fraction(Ms)
    if Mf.denominator == 1:         # integral multiplier becomes the K
      K, v = int(Mf), X
    else:                           # fractional: fold value, K = 1
      K, v = 1, int(float(X + D) * float(Ms)) - D
    if v < 0:
      print('!! %s: encoded field %d < 0, clamped to 0' % (name, v))
      v = 0
    width = max(v.bit_length(), 1) + 1
    line  = 'Number %s, %d,%d!%s%s' % (short, K, D, format(v, '0%db' % width), eol)
    adds.setdefault(id(blk), (blk, []))[1].append(line)
    m = inc_re.match(inc_lines[j])
    inc_lines[j] = '%s(%d+%d) * (%d)%s' % (m.group(1), v, D, K, m.group(6))
    if not shown2:
      print(); print(hdr2); print('-'*len(hdr2)); shown2 = True
    print('%-12s %-8s %3d %3d %9d %6d' % (name, blk[0] or '-', K, D, (v + D) * K, width))

  for blk, lines in sorted(adds.values(), key=lambda a: -a[0][1]):
    pos = blk[2]                    # insert at block end, before trailing blanks
    while pos - 1 > blk[1] and idx_lines[pos-1].strip() == '':
      pos -= 1
    idx_lines[pos:pos] = ['' + eol, '# added from ' + inc_path + eol] + lines

  open(idx_path, 'w', newline='').write('\n'.join(idx_lines))
  open(inc_path, 'w', newline='').write('\n'.join(inc_lines))

if __name__ == '__main__':
  main()
