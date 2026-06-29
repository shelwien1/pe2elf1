# gen.py -- generate x86_tables.h from corpus.p.
#
# Nothing about x86 is hardcoded here. The generator interprets the corpus
# declarations -- arch / vars / table / submatch(pfx,addr1,sib0,sib1,disp*) --
# and materialises, for every prefix byte, every (adrsiz,modrm) byte and every
# (mod,sib) byte, the decode the corpus engine would reach. The C++ side then
# just indexes the produced tables; no decode logic lives in the .cpp.
#
# usage: python3 gen.py            # corpus.p -> x86_tables.h
#        python3 gen.py --check    # also self-check vs a textbook x86 decode

import re
import sys

# --------------------------------------------------------------------------
# 1. lexing helpers
# --------------------------------------------------------------------------

def strip_comments(text):
  out = []
  for line in text.splitlines():
    h = line.find('#')
    out.append(line if h < 0 else line[:h])
  return '\n'.join(out)


def split_top(s, sep):
  # split s on sep, but only at nesting depth 0 wrt {} [] () and "" strings.
  # angle brackets are NOT nesting: '<n>' dispatch markers are atomic and the
  # '>' in the '=>' operator (and in '[$d >= ...]' guards) would corrupt depth.
  parts, buf, depth, instr = [], [], 0, False
  i = 0
  while i < len(s):
    c = s[i]
    if instr:
      buf.append(c)
      if c == '"':
        instr = False
    elif c == '"':
      instr = True; buf.append(c)
    elif c in '{[(':
      depth += 1; buf.append(c)
    elif c in '}])':
      depth -= 1; buf.append(c)
    elif depth == 0 and s.startswith(sep, i):
      parts.append(''.join(buf)); buf = []; i += len(sep); continue
    else:
      buf.append(c)
    i += 1
  if buf or parts:
    parts.append(''.join(buf))
  return parts


def find_submatches(text):
  # return {name: (params_str, body_str)} with brace-balanced bodies
  res = {}
  for m in re.finditer(r'submatch\s+(\w+)\s*(\([^)]*\))?\s*\{', text):
    name = m.group(1)
    params = (m.group(2) or '')[1:-1] if m.group(2) else ''
    i = m.end()           # just past the opening brace
    depth, instr = 1, False
    start = i
    while i < len(text) and depth:
      c = text[i]
      if instr:
        if c == '"': instr = False
      elif c == '"': instr = True
      elif c == '{': depth += 1
      elif c == '}': depth -= 1
      i += 1
    res[name] = (params, text[start:i - 1])
  return res


# --------------------------------------------------------------------------
# 2. bit-pattern model (one byte; bitorder = msb, asserted from arch)
# --------------------------------------------------------------------------

class BytePattern:
  # groups: ordered list of (kind, text); kind in {'lit','field'}; MSB first.
  def __init__(self, groups):
    self.fields = {}            # letter -> (shift, width)
    self.lit_mask = 0
    self.lit_val = 0
    pos = 8
    for kind, text in groups:
      w = len(text)
      pos -= w
      if kind == 'lit':
        self.lit_mask |= ((1 << w) - 1) << pos
        self.lit_val |= int(text, 2) << pos
      else:
        self.fields[text[0]] = (pos, w)
    if pos != 0:
      raise ValueError('byte pattern is %d bits, not 8' % (8 - pos))

  def match(self, b):
    return (b & self.lit_mask) == self.lit_val

  def get(self, b, name):
    pos, w = self.fields[name]
    return (b >> pos) & ((1 << w) - 1)


_BITGRP = re.compile(r'^[01]+$')
_FIELDGRP = re.compile(r'^([a-z])\1*$')


def is_bitgroup(tok):
  return bool(_BITGRP.match(tok)) or bool(_FIELDGRP.match(tok))


def parse_rule(lhs):
  # -> dict(dispatch=int|None, bp=BytePattern|None, calls=[(name,arg)],
  #         action={var:('field',l)|('none',)|('int',n)})
  toks = lhs.split()
  dispatch = None
  groups = []
  calls = []
  action = {}
  i = 0
  # leading <...> = the prefix-dispatch arg (single bit here)
  if i < len(toks) and toks[i].startswith('<'):
    inner = toks[i][1:-1].strip()
    dispatch = None if inner == '?' else int(inner, 2)
    i += 1
  # accumulate one byte of bit-groups (binary groups or a 0xNN hex byte)
  bits = 0
  while i < len(toks) and bits < 8:
    t = toks[i]
    hx = re.fullmatch(r'0x([0-9a-fA-F]{2})', t)
    if hx:
      groups.append(('lit', format(int(hx.group(1), 16), '08b')))
      bits += 8
    elif is_bitgroup(t):
      groups.append(('lit' if _BITGRP.match(t) else 'field', t))
      bits += len(t)
    else:
      break
    i += 1
  bp = BytePattern(groups) if groups else None
  # remaining: @calls and {actions}
  rest = ' '.join(toks[i:])
  for m in re.finditer(r'@(\w+)(?:\(([^)]*)\))?', rest):
    calls.append((m.group(1), (m.group(2) or '').strip()))
  am = re.search(r'\{([^}]*)\}', rest)
  if am:
    for asg in am.group(1).split(';'):
      asg = asg.strip()
      if not asg:
        continue
      var, val = [x.strip() for x in asg.split('=', 1)]
      var = var.lstrip('$')
      if val == 'none':
        action[var] = ('none',)
      elif val.startswith('$'):
        action[var] = ('field', val[1])
      else:
        action[var] = ('int', int(val, 0))
  return dict(dispatch=dispatch, bp=bp, calls=calls, action=action)


def parse_rules(body):
  out = []
  for chunk in split_top(body, ';'):
    if '=>' not in chunk:
      continue
    lhs, rhs = chunk.split('=>', 1)
    if not lhs.strip():
      continue
    r = parse_rule(lhs)
    r['tmpl'] = rhs.strip()
    out.append(r)
  return out


# --------------------------------------------------------------------------
# 3. parse corpus.p
# --------------------------------------------------------------------------

def kv_pairs(line):
  d = {}
  for m in re.finditer(r'\$(\w+)\s*=\s*(\S+)', line):
    d[m.group(1)] = m.group(2)
  return d


def parse_corpus(path):
  raw = open(path, encoding='utf-8').read()
  text = strip_comments(raw)

  arch = {}
  m = re.search(r'^\s*arch\s+(.*)$', text, re.M)
  if m:
    arch = kv_pairs(m.group(1))

  varnames = []
  m = re.search(r'^\s*vars\s+(.*)$', text, re.M)
  if m:
    varnames = list(kv_pairs(m.group(1)).keys())

  tables = {}
  for m in re.finditer(r'table\s+(\w+)\s*\{\s*([^}]*)\s*\}', text):
    items = []
    for it in split_top(m.group(2), ','):
      it = it.strip()
      if it.startswith('"') and it.endswith('"'):
        items.append(it[1:-1])
      elif re.fullmatch(r'-?\d+', it):
        items.append(int(it))
      else:
        items.append(it)
    tables[m.group(1)] = items

  subs = find_submatches(text)
  return dict(arch=arch, vars=varnames, tables=tables, subs=subs, raw=raw)


# --------------------------------------------------------------------------
# 4. interpretation -> decode tables
# --------------------------------------------------------------------------

GREG_NONE = 15        # the 4-bit value stored in mem_base/mem_index for "none"
SREG_NONE = 7
ABSENT = 0xFF         # internal "no register" sentinel during derivation;
                      # distinct from every greg index (incl. di=15) so the
                      # 16-bit [reg+di] form is never mistaken for [reg].


class Interp:
  def __init__(self, c):
    self.arch = c['arch']
    self.vars = c['vars']
    self.tables = c['tables']
    self.subs = c['subs']
    if self.arch.get('bitorder', 'msb') != 'msb':
      raise NotImplementedError('only bitorder=msb is modelled')

    self.greg = self.tables['greg']
    self.sbo = self.tables['sbo']
    self.sbo16 = self.tables['sbo16']
    # rm16 decomposed into (base_idx, index_idx) by greg name
    self.rm16 = []
    for ent in self.tables['rm16']:
      parts = ent.split('+')
      b = self.greg_idx(parts[0])
      x = self.greg_idx(parts[1]) if len(parts) > 1 else ABSENT
      self.rm16.append((b, x))

    self.disp = self.parse_disp()
    self.addr1 = parse_rules(self.subs['addr1'][1])
    self.sib0 = parse_rules(self.subs['sib0'][1])
    self.sib1 = parse_rules(self.subs['sib1'][1])
    # outer disp the addr1 SIB rules append, keyed by mod value
    self.sib_outer = self.collect_sib_outer()
    # single-byte (op1) + two-byte 0F (op2) instruction maps + mnemonic table
    self._op1, self._op2, self._mnem = self.build_insn()

  # ----- small helpers -----
  def greg_idx(self, name):
    name = (name or '').strip().lower()
    if name in ('', 'none'):
      return ABSENT
    for i, v in enumerate(self.greg):
      if str(v).lower() == name:
        return i
    return ABSENT

  def sbo_at(self, tbl, idx):
    return 0 if idx is None else tbl[idx]

  def parse_disp(self):
    # disp/imm submatch -> name: width in bytes (count of d/i bits / 8)
    out = {}
    for nm, (params, body) in self.subs.items():
      if not re.match(r'(disp|imm)\d', nm):
        continue
      lhs = body.split('=>')[0]
      bits = len(re.sub(r'[^di]', '', lhs))
      out[nm] = bits // 8
    return out

  def collect_sib_outer(self):
    outer = {}
    for r in self.addr1:
      names = [c[0] for c in r['calls']]
      if 'sib0' in names or 'sib1' in names:
        mod = r['bp'].lit_val >> 6        # top 2 literal bits = mod
        d = 0
        for cn, _ in r['calls']:
          if cn in self.disp:
            d = self.disp[cn]
        outer[mod] = d
    return outer

  # ----- segment row from a rule template -----
  def seg_row_from_tmpl(self, tmpl, rm):
    # the seg[...] index selects a base row; templates use a nested table ref
    # (sbo16[$r] / sbo[$r] / sbo[$sbase]) added to $segidx. Detect by substring
    # rather than parsing the nested brackets. 'sib' = resolved from the SIB byte.
    if 'sbo16[$r]' in tmpl:
      return self.sbo16[rm]
    if 'sbo[$sbase]' in tmpl:
      return 'sib'
    if 'sbo[$r]' in tmpl:
      return self.sbo[rm]
    return 0                               # seg[$segidx] only (absolute)

  # ----- ModR/M table: modrm_lookup[adrsiz][byte] -----
  def modrm_entry(self, adrsiz, modrm):
    for r in self.addr1:
      if r['dispatch'] not in (None, adrsiz):
        continue
      if not r['bp'].match(modrm):
        continue
      return self.derive_mem(r, modrm)
    # no addr1 rule -> register-direct (mod=11); reuse field positions
    reg = self.field_any('g').get(modrm, 'g')
    rm = self.field_any('r').get(modrm, 'r')
    return dict(mode='MODE_REG', reg=reg, base=rm, index=ABSENT,
                disp=0, seg_row=0, sib_mod=0)

  def field_any(self, letter):
    for r in self.addr1:
      if r['bp'] and letter in r['bp'].fields:
        return r['bp']
    raise KeyError(letter)

  def derive_mem(self, r, modrm):
    reg = r['bp'].get(modrm, 'g')
    rm = r['bp'].get(modrm, 'r') if 'r' in r['bp'].fields else None
    names = [c[0] for c in r['calls']]
    tmpl = r['tmpl']
    # displacement appended by this rule (0 unless a @dispN call present)
    disp = 0
    for cn, _ in r['calls']:
      if cn in self.disp:
        disp = self.disp[cn]
    if 'sib0' in names or 'sib1' in names:
      mod = r['bp'].lit_val >> 6
      return dict(mode='MODE_SIB', reg=reg, base=ABSENT, index=ABSENT,
                  disp=0, seg_row=0, sib_mod=mod)
    if 'rm16[$r]' in tmpl:
      base, index = self.rm16[rm]
      seg = self.seg_row_from_tmpl(tmpl, rm)
      return dict(mode='MODE_MEM', reg=reg, base=base, index=index,
                  disp=disp, seg_row=seg, sib_mod=0)
    if 'greg[$r]' in tmpl:
      seg = self.seg_row_from_tmpl(tmpl, rm)
      return dict(mode='MODE_MEM', reg=reg, base=rm, index=ABSENT,
                  disp=disp, seg_row=seg, sib_mod=0)
    # direct / absolute (disp only, no base)
    return dict(mode='MODE_MEM', reg=reg, base=ABSENT, index=ABSENT,
                disp=disp, seg_row=0, sib_mod=0)

  # ----- SIB table: sib_decode[mod][byte] -----
  def sib_entry(self, mod, sib):
    rules = self.sib0 if mod == 0 else self.sib1
    for r in rules:
      if not r['bp'].match(sib):
        continue
      tmpl = r['tmpl']
      scale = r['bp'].get(sib, 's')
      index = r['bp'].get(sib, 'i') if 'greg[$i]' in tmpl else ABSENT
      base = r['bp'].get(sib, 'b') if 'greg[$b]' in tmpl else ABSENT
      # $sbase drives the segment row
      sbase = None
      sb = r['action'].get('sbase')
      if sb and sb[0] == 'field':
        sbase = r['bp'].get(sib, sb[1])
      seg_row = self.sbo_at(self.sbo, sbase)
      # displacement: sib0 owns its @disp internally; sib1 takes addr1's outer
      if mod == 0:
        disp = 0
        for cn, _ in r['calls']:
          if cn in self.disp:
            disp = self.disp[cn]
      else:
        disp = self.sib_outer.get(mod, 0)
      return dict(scale=scale, index=index, base=base, disp=disp, seg_row=seg_row)
    raise RuntimeError('unmatched sib byte 0x%02x mod=%d' % (sib, mod))

  # ----- prefixes -----
  def prefix_table(self):
    body = self.subs['pfx'][1]
    var_enum = {v: 'VAR_' + v.upper() for v in self.vars}
    out = [(0, 'VAR_' + self.vars[0].upper(), 0)] * 256
    for r in parse_rules(body):
      if not r['bp'] or not r['action']:
        continue
      # a prefix frame is "<0xNN> @pfx(...) {var=val}"
      if r['bp'].lit_mask != 0xff:
        continue
      byte = r['bp'].lit_val
      (var, (_, val)), = r['action'].items()
      out[byte] = (1, var_enum[var], val)
    return out

  # ======================================================================
  # FSM view: every state is one uniform record { next-table, actions[] }.
  # actions are FIELD (extract bits hi:lo of the consumed byte -> capture),
  # CONST (set a capture to a literal) and APPEND (consume N more bytes,
  # little-endian, into a capture; sub-word widths sign-extend). The C++
  # driver only interprets these primitives and follows `next` until it
  # halts; nothing about x86 lives in the driver. gen.py bakes the per-byte
  # decisions here, exactly as the corpus rules dictate.
  # ----------------------------------------------------------------------
  # capture slots share one index space with the prefix vars: 0..VAR_COUNT-1
  # are the vars, then the addressing captures, then one prefix-offset slot per
  # group (== per var) that ACT_MARK fills with the offset of the prefix byte.
  CAPS = ['REG', 'RM', 'BASE', 'INDEX', 'SCALE', 'DISP', 'SEG', 'MODE', 'IMM', 'REL']
  # render/route captures, placed after the per-group prefix-offset block
  TAILCAPS = ['MNEM', 'FORM', 'IMK', 'MNSEL', 'DIR', 'GRP', 'RFILE', 'MFILE', 'SFX', 'CC', 'TBL3', 'RMREQ']
  # operand register files (which name table a captured register number indexes)
  OPF = ['GREG', 'RGB', 'XMM', 'MM', 'SREG', 'SSE_OS']

  def tailcap(self, name):
    # MNEM/FORM/IMK/MNSEL live just past the CAP_POFF block (one slot per var)
    return len(self.vars) + len(self.CAPS) + len(self.vars) + self.TAILCAPS.index(name)

  def cap(self, name):
    return len(self.vars) + self.CAPS.index(name)

  def poff(self, var_idx):
    # prefix-offset slot for a group (var); ACT_MARK target
    return len(self.vars) + len(self.CAPS) + var_idx

  def var_id(self, name):
    return self.vars.index(name)

  @staticmethod
  def store_reg(idx):
    # value to store in the 4-bit mem_base/mem_index field: a size-independent
    # register *number* (0..7), or GREG_NONE for absent. 16-bit greg aliases
    # (ax..di = greg index 8..15) collapse onto their register number, so di
    # becomes 7 and never collides with GREG_NONE (15). The renderer re-adds
    # the +8 alias offset using the address size.
    if idx == ABSENT:
      return GREG_NONE
    return idx - 8 if idx >= 8 else idx

  # ----- symbolic names for emission (use enum values wherever they exist) -----
  def cap_name(self, idx):
    nvar, ncaps = len(self.vars), len(self.CAPS)
    if idx < nvar:
      return 'VAR_' + self.vars[idx].upper()
    if idx < nvar + ncaps:
      return 'CAP_' + self.CAPS[idx - nvar]
    if idx < nvar + ncaps + nvar:                        # CAP_POFF + VAR_*
      return 'CAP_POFF + VAR_' + self.vars[idx - nvar - ncaps].upper()
    return 'CAP_' + self.TAILCAPS[idx - nvar - ncaps - nvar]

  def greg_const(self, val):
    # a stored register number (0..7) -> its canonical greg name; GREG_NONE for none
    return 'GREG_NONE' if val == GREG_NONE else 'GREG_' + str(self.greg[val]).upper()

  def field_pos(self, letter, rules):
    # (hi, lo) bit positions of `letter` taken from whichever rule defines it
    for r in rules:
      if r['bp'] and letter in r['bp'].fields:
        pos, w = r['bp'].fields[letter]
        return (pos + w - 1, pos)
    raise KeyError(letter)

  # ----- ModR/M state: modrm_fsm[adrsiz][byte] -----
  def modrm_state(self, adrsiz, modrm):
    e = self.modrm_entry(adrsiz, modrm)
    g_hi, g_lo = self.field_pos('g', self.addr1)
    r_hi, r_lo = self.field_pos('r', self.addr1)
    acts = [('FIELD', self.cap('REG'), g_hi, g_lo)]
    if e['mode'] == 'MODE_REG':
      acts.append(('FIELD', self.cap('RM'), r_hi, r_lo))
      acts.append(('CONST', self.cap('MODE'), 1, 0))      # 1 = register-direct
      return acts, 'FSM_HALT'
    if e['mode'] == 'MODE_SIB':
      return acts, 'FSM_SIB + %d*256' % e['sib_mod']     # SIB byte resolves the rest
    # MODE_MEM
    if adrsiz == 0:                                     # 32-bit: base is the rm field
      if e['base'] != ABSENT:
        acts.append(('FIELD', self.cap('BASE'), r_hi, r_lo))   # rm bits == reg number
      else:
        acts.append(('CONST', self.cap('BASE'), self.store_reg(ABSENT), 0))
      acts.append(('CONST', self.cap('INDEX'), self.store_reg(ABSENT), 0))  # no index
    else:                                              # 16-bit: base/index fixed pairs
      acts.append(('CONST', self.cap('BASE'), self.store_reg(e['base']), 0))
      acts.append(('CONST', self.cap('INDEX'), self.store_reg(e['index']), 0))
    if e['seg_row'] != 0:
      acts.append(('CONST', self.cap('SEG'), e['seg_row'], 0))
    if e['disp'] > 0:
      acts.append(('APPEND', self.cap('DISP'), e['disp'], 0))
    return acts, 'FSM_HALT'

  # ----- SIB state: sib_fsm[mod][byte] -----
  def sib_state(self, mod, sib):
    s = self.sib_entry(mod, sib)
    sibrules = self.sib0 + self.sib1
    s_hi, s_lo = self.field_pos('s', sibrules)
    i_hi, i_lo = self.field_pos('i', sibrules)
    b_hi, b_lo = self.field_pos('b', sibrules)
    acts = [('FIELD', self.cap('SCALE'), s_hi, s_lo)]
    if s['index'] != ABSENT:
      acts.append(('FIELD', self.cap('INDEX'), i_hi, i_lo))   # bits == reg number
    else:
      acts.append(('CONST', self.cap('INDEX'), self.store_reg(ABSENT), 0))
    if s['base'] != ABSENT:
      acts.append(('FIELD', self.cap('BASE'), b_hi, b_lo))
      if s['seg_row'] != 0:
        acts.append(('CONST', self.cap('SEG'), s['seg_row'], 0))
    else:
      acts.append(('CONST', self.cap('BASE'), self.store_reg(ABSENT), 0))
    if s['disp'] > 0:
      acts.append(('APPEND', self.cap('DISP'), s['disp'], 0))
    return acts, 'FSM_HALT'

  # ----- prefix state: prefix_fsm[byte] (loops back to itself) -----
  def prefix_state(self, byte):
    isp, var, val = self.prefix_table()[byte]
    if not isp:
      return [], 'FSM_HALT'                              # dead: opcode, not consumed
    var_idx = self.var_id(var[len('VAR_'):].lower())     # 'VAR_OPSIZ' -> index of opsiz
    # set the group's value, and record this byte's offset as that group's last
    return [('CONST', var_idx, val, 0),
            ('MARK', self.poff(var_idx), 0, 0)], 'FSM_PREFIX'

  # ======================================================================
  # single-byte instruction decoder: the op1[256] opcode map.
  # Each op1 state classifies one opcode -- mnemonic (ACT_MNEM), any opcode-
  # embedded register (FIELD CAP_REG), an operand FORM (for the renderer) and
  # an immediate KIND (for the trailing-immediate append) -- then halts. The
  # thin C entry routes ModR/M (it owns the adrsiz start, as parse_addr does)
  # and appends the immediate (immz width is its one opsize fact). Group
  # opcodes (reg-fixed /digit dispatch, sec 9) and the 0F map are not built
  # here yet -- their opcode bytes stay dead (undefined) for now.
  # ----------------------------------------------------------------------
  INSN_FORM = ['NONE', 'MODRM', 'REG', 'REG_IMM', 'IMM', 'REL', 'PTR', 'GROUP', 'ESC', 'RM']
  INSN_IMK = ['NONE', 'IMM8', 'IMM16', 'IMM32', 'IMMZ', 'REL8', 'RELZ', 'PTR', 'IMM8SX', 'ENTER']

  def insn_rules_raw(self):
    out = []
    for chunk in split_top(self.subs['insn'][1], ';'):
      if '=>' not in chunk:
        continue
      lhs, rhs = chunk.split('=>', 1)
      if lhs.strip():
        out.append((lhs.strip(), rhs.strip()))
    return out

  def parse_insn_lhs(self, lhs):
    # -> dict(bp, tb, two_byte, modrm, reg_fixed, imm[list], embedded) ; None to skip.
    # tb selects the opcode map: 0 one-byte, 1 = 0F, 2 = 0F 38, 3 = 0F 3A.
    toks = lhs.split()
    i = 0
    tb = 0
    if toks and toks[0] == '0x0f':                       # 0F escape: opcode is byte 2+
      tb = 1
      i = 1
      if i < len(toks) and toks[i] == '0x38':            # 0F 38: three-byte map
        tb = 2; i += 1
      elif i < len(toks) and toks[i] == '0x3a':          # 0F 3A: three-byte map
        tb = 3; i += 1
    two_byte = (tb == 1)
    groups = []
    bits = 0
    while i < len(toks) and bits < 8:                     # the opcode byte
      t = toks[i]
      hx = re.fullmatch(r'0x([0-9a-fA-F]{2})', t)
      if hx:
        groups.append(('lit', format(int(hx.group(1), 16), '08b')))
        bits += 8
      elif is_bitgroup(t):
        groups.append(('lit' if _BITGRP.match(t) else 'field', t))
        bits += len(t)
      else:
        break
      i += 1
    if bits != 8:
      return None
    bp = BytePattern(groups)
    rest = toks[i:]
    modrm = None
    reg_fixed = None
    imm = []
    embedded = next((f for f in ('b', 'c') if f in bp.fields), None)
    j = 0
    if j < len(rest) and rest[j] == '11':
      mid = rest[j + 1]
      if re.fullmatch(r'[01]+', mid):
        reg_fixed = int(mid, 2)
        modrm = 'group_reg'
      else:
        modrm = 'reg'
      j += 3
    while j < len(rest):
      cm = re.fullmatch(r'@(\w+)(?:\(([^)]*)\))?', rest[j])
      j += 1
      if not cm:
        continue
      nm, arg = cm.group(1), cm.group(2)
      if nm == 'addr':
        if arg not in (None, ''):
          modrm = 'group_mem'
          reg_fixed = int(arg)
        else:
          modrm = 'mem'
      elif nm in ('imm8', 'imm16', 'imm32', 'immz', 'rel8', 'relz'):
        imm.append(nm)
    return dict(bp=bp, tb=tb, two_byte=two_byte, modrm=modrm,
                reg_fixed=reg_fixed, imm=imm, embedded=embedded)

  def operand_file(self, tmpl, var):
    # which register-name table the $var operand draws from
    m = re.search(r'(greg|rgb|sreg|ssereg)\[([^\]]*\$' + var + r'[^\]]*)\]', tmpl)
    if not m:
      return 'GREG'
    tab, idx = m.group(1), m.group(2)
    if tab == 'rgb':
      return 'RGB'
    if tab == 'sreg':
      return 'SREG'
    if tab == 'greg':
      return 'GREG'
    if '$opsiz' in idx:                                  # ssereg[$opsiz*8+$var]: mm/xmm by opsize
      return 'SSE_OS'
    return 'XMM' if '8+' in idx else 'MM'

  def insn_mnem(self, rhs):
    m = re.match(r'"([^"]*)"\s+cond\[\$\w+\]', rhs)     # "j" cond[$c] ...
    if m:
      return ('cond', m.group(1))
    m = re.match(r'"([^"]*)"', rhs)                     # plain leading literal
    if m:
      return ('lit', m.group(1).strip())
    m = re.match(r'(\w+)\[\$(\w+)\]', rhs)              # movs[$opsiz] / cdqw[$opsiz]
    if m:
      return ('sel', m.group(1), m.group(2))
    return ('lit', '?')

  def build_insn(self):
    F = {n: i for i, n in enumerate(self.INSN_FORM)}
    K = {n: i for i, n in enumerate(self.INSN_IMK)}
    OPF = {n: i for i, n in enumerate(self.OPF)}
    cond = self.tables['cond']
    mnem = []

    def midx(s):
      if s not in mnem:
        mnem.append(s)
      return mnem.index(s)

    SKIP_MNEM_TABS = ('m10', 'elt')                      # reptype / "add"+elt: a later step
    desc = {0: {}, 1: {}, 2: {}, 3: {}}                  # [tb] -> {byte: descriptor}
                                                         #   0 one-byte, 1 0F, 2 0F 38, 3 0F 3A
    groups = {}
    for lhs, rhs in self.insn_rules_raw():
      info = self.parse_insn_lhs(lhs)
      if info is None:
        continue
      if info['modrm'] in ('group_reg', 'group_mem'):    # opcode-extension group
        self._collect_group(groups, info, rhs, midx, K)  # one-byte AND two-byte (0F) groups
        continue
      mn = self.insn_mnem(rhs)
      if mn[0] == 'sel' and mn[1] in SKIP_MNEM_TABS:
        continue
      bp, emb, imml = info['bp'], info['embedded'], info['imm']
      tb = info['tb']
      # immediate kind (imm8 may be sign-extended -- see RHS)
      if imml == ['imm32', 'imm16']:
        imk = K['PTR']
      elif imml == ['imm16', 'imm8']:
        imk = K['ENTER']                                 # enter Iw,Ib
      elif len(imml) == 1:
        imk = K['IMM8SX'] if (imml[0] == 'imm8' and 'sx8(' in rhs) else K[imml[0].upper()]
      elif not imml:
        imk = K['NONE']
      else:
        continue
      has_g = bool(re.search(r'(greg|rgb|sreg|ssereg)\[[^\]]*\$g[^\]]*\]', rhs))
      # operand form
      two_ops = (',' in rhs)
      if info['modrm'] in ('reg', 'mem'):
        form = F['MODRM'] if has_g else F['RM']          # RM = single r/m operand (setcc)
      elif emb == 'b' and imml:
        form = F['REG_IMM']
      elif emb == 'b':
        form = F['REG']
      elif imk in (K['PTR'], K['ENTER']):
        form = F['PTR']                                  # both carry a paired immediate
      elif imml and imml[0] in ('rel8', 'relz'):
        form = F['REL']
      elif imml and two_ops:
        form = F['REG_IMM']                              # implicit accumulator
      elif imml:
        form = F['IMM']
      else:
        form = F['NONE']
      # operand-order (reg vs r/m first) for two-operand ModR/M
      dir_rm = 0
      if form == F['MODRM']:
        gpos, rmpos = rhs.find('$g'), rhs.find('$addr')
        if rmpos < 0:
          rmpos = rhs.find('$r')
        dir_rm = 1 if (gpos >= 0 and rmpos >= 0 and gpos > rmpos) else 0
      # size suffix carried by the memory form (e.g. "mov" sfx[1] / "inc" sfx[4])
      sfx = 0
      if info['modrm'] == 'mem':
        sm = re.search(r'sfx\[(\d+)\]', rhs)
        if sm:
          sfx = int(sm.group(1))
      # concrete opcode bytes (enumerate the embedded field, if any)
      if emb:
        pos, w = bp.fields[emb]
        vals = range(1 << w)
      else:
        pos, w, vals = 0, 0, [0]
      for v in vals:
        byte = bp.lit_val | (v << pos)
        d = desc[tb].setdefault(byte, {'form': F['NONE'], 'imk': K['NONE'],
                                       'rfile': OPF['GREG'], 'mfile': OPF['GREG'],
                                       'sfx': 0, 'dir': 0, 'mnsel': 0,
                                       'emb': None, 'mnem': None, 'reg0': False,
                                       'cc': None, 'has_reg': False, 'has_mem': False})
        if info['modrm'] == 'reg': d['has_reg'] = True     # which ModR/M mod forms
        if info['modrm'] == 'mem': d['has_mem'] = True     # this opcode actually defines
        # mnemonic
        if mn[0] == 'lit':
          d['mnem'] = midx(mn[1])
        elif mn[0] == 'cond':
          d['mnem'] = midx(mn[1] + 'cc')               # generic jcc / setcc / cmovcc
          d['cc'] = v                                  # condition code -> immediate operand
        else:                                          # opsize-selected table
          tab = self.tables[mn[1]]
          d['mnem'] = midx(tab[0])
          for k in range(1, len(tab)):
            midx(tab[k])                                 # variants contiguous
          d['mnsel'] = 1
        d['form'] = form
        d['imk'] = imk if imk != K['NONE'] else d['imk']
        d['emb'] = emb if emb in ('b',) else d['emb']
        if form == F['REG_IMM'] and not emb:
          d['reg0'] = True
        # files: reg-direct rule supplies both; mem rule supplies the suffix
        if info['modrm'] == 'reg':
          d['rfile'] = OPF[self.operand_file(rhs, 'g')]
          d['mfile'] = OPF[self.operand_file(rhs, 'r')]
          d['dir'] = dir_rm
        elif info['modrm'] == 'mem':
          d['rfile'] = OPF[self.operand_file(rhs, 'g')]
          d['dir'] = dir_rm
          if sfx:
            d['sfx'] = sfx
        elif emb == 'b':
          d['rfile'] = OPF[self.operand_file(rhs, 'b')]

    def to_state(d):
      acts = [('MNEM', d['mnem'], 0, 0)]
      if d['emb'] == 'b':                                # opcode-embedded register
        acts.append(('FIELD', self.cap('REG'), 2, 0))
      elif d['reg0']:
        acts.append(('CONST', self.cap('REG'), 0, 0))    # implicit eAX / al
      if d['form'] != F['NONE']:
        acts.append(('CONST', self.tailcap('FORM'), d['form'], 0))
      if d['dir']:
        acts.append(('CONST', self.tailcap('DIR'), 1, 0))
      if d['imk'] != K['NONE']:
        acts.append(('CONST', self.tailcap('IMK'), d['imk'], 0))
      if d['mnsel']:
        acts.append(('CONST', self.tailcap('MNSEL'), 1, 0))
      if d['rfile'] != OPF['GREG']:
        acts.append(('CONST', self.tailcap('RFILE'), d['rfile'], 0))
      if d['mfile'] != OPF['GREG']:
        acts.append(('CONST', self.tailcap('MFILE'), d['mfile'], 0))
      if d['sfx']:
        acts.append(('CONST', self.tailcap('SFX'), d['sfx'], 0))
      if d['cc'] is not None:
        acts.append(('CONST', self.tailcap('CC'), d['cc'] + 1, 0))   # +1: 0 means none
      # ModR/M mod constraint: if the opcode defines only the reg form or only the
      # mem form, the other mod is an illegal encoding -- reject it at decode so the
      # bijection holds (decoder accepts only what the encoder can reproduce).
      if d['form'] in (F['MODRM'], F['RM']):
        if d['has_reg'] and not d['has_mem']:
          acts.append(('CONST', self.tailcap('RMREQ'), 1, 0))        # reg-only
        elif d['has_mem'] and not d['has_reg']:
          acts.append(('CONST', self.tailcap('RMREQ'), 2, 0))        # mem-only
      return acts, 'FSM_HALT'

    op1 = {b: to_state(d) for b, d in desc[0].items()}
    op2 = {b: to_state(d) for b, d in desc[1].items()}
    op3_38 = {b: to_state(d) for b, d in desc[2].items()}   # 0F 38 xx
    op3_3a = {b: to_state(d) for b, d in desc[3].items()}   # 0F 3A xx
    # group opcodes: assign ids, store members, route the owning map -> group stage
    self._group_list = []
    group_map = {0: op1, 1: op2, 2: op3_38, 3: op3_3a}      # dispatch by escape (tb)
    for gid, (tb, opcode) in enumerate(sorted(groups)):
      self._group_list.append((tb, opcode, groups[(tb, opcode)]))
      group_map[tb][opcode] = ([('CONST', self.tailcap('FORM'), F['GROUP'], 0),
                                ('CONST', self.tailcap('GRP'), gid & 0x1f, gid >> 5)], 'FSM_HALT')
    # 0F escape: op1[0x0f] hands off to op2
    op1[0x0f] = ([('CONST', self.tailcap('FORM'), F['ESC'], 0)], 'FSM_HALT')
    # three-byte escapes: op2[0x38]/op2[0x3a] mark CAP_TBL3 so the driver runs op3
    if op3_38:
      op2[0x38] = ([('CONST', self.tailcap('TBL3'), 1, 0)], 'FSM_HALT')
    if op3_3a:
      op2[0x3a] = ([('CONST', self.tailcap('TBL3'), 2, 0)], 'FSM_HALT')
    self._op3_38, self._op3_3a = op3_38, op3_3a
    return op1, op2, mnem

  def op3_38_state(self, byte):
    return self._op3_38.get(byte, ([], 'FSM_HALT'))

  def op3_3a_state(self, byte):
    return self._op3_3a.get(byte, ([], 'FSM_HALT'))

  def _collect_group(self, groups, info, rhs, midx, K):
    tb = info['tb']
    opcode = info['bp'].lit_val
    reg = info['reg_fixed']
    m = groups.setdefault((tb, opcode), {}).setdefault(
        reg, {'mnem_reg': None, 'mnem_mem': None, 'imk': K['NONE']})
    base = self.insn_mnem(rhs)[1]                        # leading-literal mnemonic
    if info['imm']:
      if 'sx8(' in rhs and info['imm'][0] == 'imm8':
        m['imk'] = K['IMM8SX']
      else:
        m['imk'] = K[info['imm'][0].upper()]
    if info['modrm'] == 'group_reg':
      m['mnem_reg'] = midx(base)
    else:                                                # group_mem: fold in size suffix
      sm = re.search(r'sfx\[(\d+)\]', rhs)
      sfx = self.tables['sfx'][int(sm.group(1))] if sm else ''
      m['mnem_mem'] = midx(base + sfx)

  def group_state(self, gid, adrsiz, modrm):
    # one byte of a dedicated reg-fixed ModR/M stage: the reg field is the opcode
    # extension (it picks the mnemonic, baked here), the r/m is the operand.
    _, _, members = self._group_list[gid]
    m = members.get((modrm >> 3) & 7)
    if m is None:
      return [], 'FSM_HALT'                              # undefined extension: dead
    is_reg = (modrm >> 6) == 3
    mnem = m['mnem_reg'] if is_reg else m['mnem_mem']
    if mnem is None:
      return [], 'FSM_HALT'                              # this form not defined for /reg
    acts, nxt = self.modrm_state(adrsiz, modrm)
    pre = [('MNEM', mnem, 0, 0)]                         # reg field -> mnemonic (per byte)
    if m['imk'] != 0:
      pre.append(('CONST', self.tailcap('IMK'), m['imk'], 0))
    return pre + acts[1:], nxt                           # drop modrm's FIELD CAP_REG

  def group_count(self):
    return len(self._group_list)

  def op1_state(self, byte):
    return self._op1.get(byte, ([], 'FSM_HALT'))

  def op2_state(self, byte):
    return self._op2.get(byte, ([], 'FSM_HALT'))

  # simulate the driver over a state's actions (for self-check)
  @staticmethod
  def sim_state(acts, byte, stream, ip, off, caps):
    for op, dst, a0, a1 in acts:
      if op == 'FIELD':
        caps[dst] = (byte >> a1) & ((1 << (a0 - a1 + 1)) - 1)
      elif op == 'CONST':
        caps[dst] = a0 | (a1 << 5)                       # 8-bit const (matches run_fsm)
      elif op == 'MARK':
        caps[dst] = off                                  # offset of the consumed byte
      else:  # APPEND, little-endian, sign-extend sub-word
        n = a0
        v = 0
        for j in range(n - 1, -1, -1):
          v = (v << 8) | stream[ip + j]
        if n < 4 and (v & (1 << (n * 8 - 1))):
          v -= (1 << (n * 8))
        caps[dst] = v
        ip += n
    return ip


# --------------------------------------------------------------------------
# 5. emit x86_tables.h
# --------------------------------------------------------------------------

def gname(greg, idx):
  return 'GREG_NONE' if idx == GREG_NONE else 'GREG_' + str(greg[idx]).upper()


def emit(c, interp, out_path):
  greg = interp.greg
  arch = interp.arch
  endian = 0 if arch.get('endian', 'le') == 'le' else 1
  bitorder = 0 if arch.get('bitorder', 'msb') == 'msb' else 1
  maxlen = int(arch.get('maxlen', 15))
  mode = int(arch.get('mode', 32))
  nvar = len(interp.vars)

  MAX_ACT = 7
  ACTOP = {'FIELD': 'ACT_FIELD', 'CONST': 'ACT_CONST',
           'APPEND': 'ACT_APPEND', 'MARK': 'ACT_MARK', 'MNEM': 'ACT_MNEM'}

  # flat table layout: one DState array, addressed by 16-bit base index.
  PREFIX_BASE = 0
  MODRM_BASE = 256
  SIB_BASE = MODRM_BASE + 2 * 256
  TOTAL = SIB_BASE + 3 * 256
  HALT = 0xFFFF

  def act_str(a):
    op, dst, a0, a1 = a
    if op == 'MNEM':                                   # wide const -> fixed CAP_MNEM
      return "act_argx(ACT_MNEM, %d)" % dst            # dst slot carries the 13-bit index
    dn = interp.cap_name(dst)
    if op == 'FIELD':                                  # a0:a1 are bit positions
      return "{%s,%s,%d,%d}" % (ACTOP[op], dn, a0, a1)
    if op == 'APPEND':                                 # a0 = byte count
      return "{%s,%s,%d,0}" % (ACTOP[op], dn, a0)
    if op == 'MARK':
      return "{%s,%s,0,0}" % (ACTOP[op], dn)
    # CONST: name the value when an enum applies (register / mode marker); the
    # value is 8 bits (arg0 lo5 | arg1 hi3) so group ids past 31 survive.
    if dst in (interp.cap('BASE'), interp.cap('INDEX')):
      av = interp.greg_const(a0)
    elif dst == interp.cap('MODE'):
      av = 'RM_REG' if a0 == 1 else 'RM_MEM'
    else:                                              # prefix value / seg row / group id
      av = str(a0)
    return "{%s,%s,%s,%d}" % (ACTOP[op], dn, av, a1)

  def state_str(acts, nxt):
    if len(acts) > MAX_ACT:
      raise RuntimeError('state needs %d actions > MAX_ACT' % len(acts))
    cells = [act_str(a) for a in acts] + ['NOACT'] * (MAX_ACT - len(acts))
    return "{%s,{%s}}" % (nxt, ",".join(cells))

  o = []
  w = o.append
  w("// Generated from corpus.p by gen.py. Do not edit.\n")
  w("#ifndef X86_TABLES_H\n#define X86_TABLES_H\n\n")
  w("#include <stdint.h>\n#include <stddef.h>\n\n")

  # arch constants (the only architectural literals the driver may use)
  w("// ---- arch constants (from corpus.p `arch`) ----\n")
  w("#define ARCH_MODE     %d\n" % mode)
  w("#define ARCH_ENDIAN   %d  // 0=le 1=be\n" % endian)
  w("#define ARCH_BITORDER %d  // 0=msb 1=lsb\n" % bitorder)
  w("#define ARCH_MAXLEN   %d\n" % maxlen)
  w("#define FSM_MAX_ACT   %d\n\n" % MAX_ACT)

  # var enum (from corpus.p `vars`)  -- capture slots 0..VAR_COUNT-1
  w("enum VarIndex {\n")
  for i, v in enumerate(interp.vars):
    w("    VAR_%s = %d,\n" % (v.upper(), i))
  w("    VAR_COUNT = %d\n};\n\n" % nvar)

  # one capture array: vars, then addressing captures, then a prefix-offset
  # slot per group (filled by ACT_MARK with the prefix byte's offset).
  w("// captures share one index space with the vars above\n")
  w("enum Capture {\n")
  for j, name in enumerate(Interp.CAPS):
    w("    CAP_%s = VAR_COUNT + %d,\n" % (name, j))
  w("    CAP_POFF = VAR_COUNT + %d,   // CAP_POFF + group = that group's last offset\n"
    % len(Interp.CAPS))
  w("    CAP_MNEM = CAP_POFF + VAR_COUNT,   // mnemonic index (fixed ACT_MNEM destination)\n")
  w("    CAP_FORM = CAP_MNEM + 1,           // operand shape (enum InsnForm; for the renderer)\n")
  w("    CAP_IMK  = CAP_FORM + 1,           // trailing-immediate kind (enum ImmKind)\n")
  w("    CAP_MNSEL = CAP_IMK + 1,           // 1 => add opsize to CAP_MNEM (movs/cdqw)\n")
  w("    CAP_DIR  = CAP_MNSEL + 1,          // ModR/M operand order: 0 = reg,r/m   1 = r/m,reg\n")
  w("    CAP_GRP  = CAP_DIR + 1,            // group id (routes op1 -> the reg-fixed group stage)\n")
  w("    CAP_RFILE = CAP_GRP + 1,           // reg-operand register file (enum OperandFile)\n")
  w("    CAP_MFILE = CAP_RFILE + 1,         // r/m-operand register file (reg-direct only)\n")
  w("    CAP_SFX  = CAP_MFILE + 1,          // memory-form size suffix (index into sfx[])\n")
  w("    CAP_CC   = CAP_SFX + 1,            // condition code + 1 (0 = none); trailing imm operand\n")
  w("    CAP_TBL3 = CAP_CC + 1,             // three-byte map: 0 none, 1 = 0F 38, 2 = 0F 3A\n")
  w("    CAP_RMREQ = CAP_TBL3 + 1,          // ModR/M mod constraint: 0 any, 1 reg-only, 2 mem-only\n")
  w("    NCAPS = CAP_RMREQ + 1              // == 32, the dst:5 ceiling (see Action.dst)\n};\n\n")

  # GPR / SREG enums (from greg / sreg tables) -- used only by the renderer
  w("enum GPR {\n")
  for i, n in enumerate(greg):
    if isinstance(n, str):
      w("    GREG_%s = %d,\n" % (n.upper(), i))
  w("    GREG_NONE = %d\n};\n\n" % GREG_NONE)

  w("enum SREG {\n")
  for i, n in enumerate(interp.tables.get('sreg', [])):
    if isinstance(n, str):
      w("    SREG_%s = %d,\n" % (n.upper(), i))
  w("    SREG_NONE = %d\n};\n\n" % SREG_NONE)

  # CAP_MODE marker: register-direct vs memory r/m
  w("enum RmMode { RM_MEM = 0, RM_REG = 1 };\n\n")
  # operand-shape + immediate-kind enums for the single-byte instruction decoder
  w("enum InsnForm { FORM_NONE=0, FORM_MODRM, FORM_REG, FORM_REG_IMM, FORM_IMM, FORM_REL, FORM_PTR, FORM_GROUP, FORM_ESC, FORM_RM };\n")
  w("enum ImmKind  { IMK_NONE=0, IMK_IMM8, IMK_IMM16, IMK_IMM32, IMK_IMMZ, IMK_REL8, IMK_RELZ, IMK_PTR, IMK_IMM8SX, IMK_ENTER };\n")
  w("enum OperandFile { OPF_GREG=0, OPF_RGB, OPF_XMM, OPF_MM, OPF_SREG, OPF_SSE_OS };\n\n")

  # the one uniform FSM record (Action packed to 16 bits)
  w("// ---- uniform state-machine record ----\n")
  w("enum ActOp { ACT_NONE = 0, ACT_FIELD, ACT_CONST, ACT_APPEND, ACT_MARK, ACT_MNEM };\n")
  w("// ACT_NONE: end of this state's action list (also the padding value)\n")
  w("// FIELD : cap[dst] = bits arg0:arg1 of the consumed byte\n")
  w("// CONST : cap[dst] = arg0\n")
  w("// APPEND: cap[dst] = next arg0 bytes (ARCH_ENDIAN order, sub-word sign-extended)\n")
  w("// MARK  : cap[dst] = offset of the consumed byte (prefix group bookkeeping)\n")
  w("// MNEM  : cap[CAP_MNEM] = argx (13-bit wide const; dst/arg0/arg1 hold the value)\n")
  w("struct Action {            // 16 bits\n")
  w("  union {\n")
  w("    struct {\n")
  w("      uint16_t op   : 3;   // ACT_*; ACT_NONE ends the list\n")
  w("      uint16_t dst  : 5;   // capture slot (NCAPS <= 32)\n")
  w("      uint16_t arg0 : 5;   // FIELD hi / CONST val lo5 / APPEND nbytes\n")
  w("      uint16_t arg1 : 3;   // FIELD lo / CONST val hi3 (8-bit consts: group id)\n")
  w("    };\n")
  w("    struct {\n")
  w("      uint16_t opx  : 3;   // ACT_*; ACT_NONE ends the list\n")
  w("      uint16_t argx : 5+5+3;   // (1<<13)=8192 values (e.g. mnemonic index)\n")
  w("    };\n")
  w("    uint16_t x;            // the whole 16-bit cell\n")
  w("  };\n")
  w("};\n")
  w("constexpr struct Action NOACT = {ACT_NONE, 0, 0, 0};   // empty action slot\n")
  w("// Build an Action that stows a 13-bit argx under opx (e.g. ACT_MNEM + index).\n")
  w("// Written through the (op,dst,arg0,arg1) view so it stays a constant expression;\n")
  w("// read back via .argx / .x (the union aliases the same 16 bits, op in bits 0:2).\n")
  w("constexpr struct Action act_argx(uint16_t opx, uint16_t argx) {\n")
  w("  return { (uint16_t)opx,\n")
  w("           (uint16_t)(argx & 0x1Fu),\n")
  w("           (uint16_t)((argx >> 5) & 0x1Fu),\n")
  w("           (uint16_t)((argx >> 10) & 0x07u) };\n")
  w("}\n")
  w("static_assert(sizeof(struct Action) == 2, \"Action must pack to 16 bits\");\n")
  w("static_assert(act_argx(ACT_MNEM, 0x1FFFu).op == ACT_MNEM, \"act_argx opx placement\");\n")
  w("struct DState {\n")
  w("    uint16_t next;                     // table base index into FSM, or FSM_HALT\n")
  w("    struct Action act[FSM_MAX_ACT];    // run until ACT_NONE or FSM_MAX_ACT\n")
  w("};\n")
  w("// a state is a dead-end (byte isn't ours) iff act[0].op==ACT_NONE && next==FSM_HALT.\n")
  w("// 'CAP_MODE == RM_REG' marks a register-direct r/m (set by the ModR/M state).\n\n")

  # The FSM is one struct of named, same-type member arrays. Base indices are
  # DERIVED from the layout via offsetof, so they cannot drift from the struct.
  # Consecutive arrays of the same type are contiguous (sizeof is a multiple of
  # alignof), so the whole thing is also one flat DState[] -- asserted below.
  w("struct Fsm {\n")
  w("    struct DState prefix[256];\n")
  w("    struct DState op1[256];            // one-byte opcode map (prefix run hands off here)\n")
  w("    struct DState op2[256];            // two-byte map: 0F xx (op1[0x0f] hands off here)\n")
  w("    struct DState op3_38[256];         // three-byte map: 0F 38 xx (op2[0x38] hands off)\n")
  w("    struct DState op3_3a[256];         // three-byte map: 0F 3A xx (op2[0x3a] hands off)\n")
  w("    struct DState modrm[2][256];       // [adrsiz]\n")
  w("    struct DState sib[3][256];         // [mod]\n")
  w("    struct DState groups[%d][2][256];   // [group id][adrsiz] -- reg-fixed extension stages\n"
    % max(1, interp.group_count()))
  w("};\n")
  w("// base indices derived from the layout (single source of truth):\n")
  w("#define FSM_INDEX(member) ((uint16_t)(offsetof(struct Fsm, member) / sizeof(struct DState)))\n")
  w("#define FSM_PREFIX  FSM_INDEX(prefix)\n")
  w("#define FSM_OP1     FSM_INDEX(op1)\n")
  w("#define FSM_OP2     FSM_INDEX(op2)\n")
  w("#define FSM_OP3_38  FSM_INDEX(op3_38)\n")
  w("#define FSM_OP3_3A  FSM_INDEX(op3_3a)\n")
  w("#define FSM_MODRM   FSM_INDEX(modrm)       // + adrsiz*256\n")
  w("#define FSM_SIB     FSM_INDEX(sib)         // + mod*256\n")
  w("#define FSM_GROUPS  FSM_INDEX(groups)      // + (gid*2 + adrsiz)*256\n")
  w("#define FSM_NGROUP  %d\n" % interp.group_count())
  w("#define FSM_COUNT   (sizeof(struct Fsm) / sizeof(struct DState))\n")
  w("#define FSM_HALT    ((uint16_t)0xFFFF)     // `next` sentinel: stop\n")
  w("// flat element access across the contiguous member arrays:\n")
  w("#define FSM_AT(i)   (((const struct DState*)&FSM)[i])\n\n")

  def emit_states(gen_state):
    for i in range(256):
      acts, nxt = gen_state(i)
      w("    %s,\n" % state_str(acts, nxt))

  w("static const struct Fsm FSM = {\n")
  w("  { // prefix[256]\n")
  emit_states(interp.prefix_state)
  w("  },\n")
  w("  { // op1[256]\n")
  emit_states(interp.op1_state)
  w("  },\n")
  w("  { // op2[256]  (two-byte 0F map)\n")
  emit_states(interp.op2_state)
  w("  },\n")
  w("  { // op3_38[256]  (three-byte 0F 38 map)\n")
  emit_states(interp.op3_38_state)
  w("  },\n")
  w("  { // op3_3a[256]  (three-byte 0F 3A map)\n")
  emit_states(interp.op3_3a_state)
  w("  },\n")
  w("  { // modrm[2][256]\n")
  for adr in (0, 1):
    w("   { // adrsiz=%d\n" % adr)
    emit_states(lambda i, a=adr: interp.modrm_state(a, i))
    w("   }%s\n" % ("," if adr == 0 else ""))
  w("  },\n")
  w("  { // sib[3][256]\n")
  for mod in (0, 1, 2):
    w("   { // mod=%d\n" % mod)
    emit_states(lambda i, m=mod: interp.sib_state(m, i))
    w("   }%s\n" % ("," if mod < 2 else ""))
  w("  }")
  ng = interp.group_count()
  if ng:
    w(",\n  { // groups[%d][2][256]\n" % ng)
    for gid in range(ng):
      tb, opcode, _ = interp._group_list[gid]
      w("   { // group id %d  (%sopcode 0x%02x)\n" % (gid, "0F " if tb else "", opcode))
      for adr in (0, 1):
        w("    { // adrsiz=%d\n" % adr)
        emit_states(lambda i, g=gid, a=adr: interp.group_state(g, a, i))
        w("    }%s\n" % ("," if adr == 0 else ""))
      w("   }%s\n" % ("," if gid < ng - 1 else ""))
    w("  }\n")
  else:
    w("\n")
  w("};\n")
  w("static_assert(sizeof(struct Fsm) == (256 + 256 + 256 + 256 + 256 + 2*256 + 3*256 + %d*2*256) * sizeof(struct DState),\n"
    % max(1, ng))
  w('              "struct Fsm has padding; flat indexing would be wrong");\n')
  # two distinct size limits, the narrower one (group id) binds first:
  #   * group id rides a CONST action -> arg0(5) | arg1(3) = 8 bits  (<= 255 groups)
  #   * DState.next indexes the flat Fsm in DStates -> uint16_t       (<= 65535 states)
  w('static_assert(FSM_NGROUP <= 255, "group id no longer fits the 8-bit CONST action");\n')
  w("static_assert(NCAPS <= 32, \"capture index no longer fits Action.dst (5 bits)\");\n")
  w("static_assert(sizeof(struct Fsm) / sizeof(struct DState) <= 65535,\n")
  w('              "Fsm too large: DState.next (uint16_t) can no longer index every state");\n\n')

  # string / number tables (verbatim from corpus) -- for the renderer
  for name, items in interp.tables.items():
    numeric = all(isinstance(x, int) for x in items)
    if numeric:
      w("static const int16_t %s[] = {\n    %s\n};\n" %
        (name, ", ".join(map(str, items))))
    else:
      w("static const char* const %s[] = {\n    %s\n};\n" %
        (name, ", ".join('"%s"' % x for x in items)))
    w("static const size_t %s_size = sizeof(%s) / sizeof(%s[0]);\n\n" %
      (name, name, name))

  # mnemonic table for the single-byte instruction decoder (CAP_MNEM indexes it)
  if 'vex' not in interp._mnem:                 # placeholder mnemonic for VEX insns
    interp._mnem.append('vex')
  w("#define MNEM_VEX %d\n" % interp._mnem.index('vex'))
  w("#define MNEM_MOV %d\n" % (interp._mnem.index('mov') if 'mov' in interp._mnem else 0))
  w("static const char* const mnem_tab[] = {\n    %s\n};\n" %
    (", ".join('"%s"' % m for m in interp._mnem) if interp._mnem else '""'))
  w("static const size_t mnem_tab_size = sizeof(mnem_tab) / sizeof(mnem_tab[0]);\n\n")

  w("#endif // X86_TABLES_H\n")
  open(out_path, 'w', encoding='utf-8').write(''.join(o))

  # FSM size report: DState.next is uint16_t, so the flat table must stay under
  # 65536 states. Each opcode/group sub-table is 256 states; track the trend.
  ndstate = (256 + 256 + 256 + 256 + 256 + 2 * 256 + 3 * 256 + max(1, ng) * 2 * 256)
  import sys as _sys
  _sys.stderr.write(
      "FSM: %d DStates (%.1f%% of uint16_t next), %d groups, sizeof(Fsm)=%d KiB\n"
      % (ndstate, 100.0 * ndstate / 65536, ng, ndstate * 16 // 1024))


# --------------------------------------------------------------------------
# 6. self-check against a textbook x86 decode
# --------------------------------------------------------------------------

def self_check(interp):
  greg = interp.greg
  sbo, sbo16, rm16 = interp.sbo, interp.sbo16, interp.rm16
  bad = 0

  def chk(cond, msg):
    nonlocal bad
    if not cond:
      bad += 1
      if bad <= 20:
        sys.stderr.write("MISMATCH " + msg + "\n")

  for modrm in range(256):
    mod, reg, rm = modrm >> 6, (modrm >> 3) & 7, modrm & 7
    # --- 32-bit ---
    e = interp.modrm_entry(0, modrm)
    chk(e['reg'] == reg, "32 reg m=%02x" % modrm)
    if mod == 3:
      chk(e['mode'] == 'MODE_REG' and e['base'] == rm, "32 reg-direct %02x" % modrm)
    elif rm == 4:
      chk(e['mode'] == 'MODE_SIB' and e['sib_mod'] == mod, "32 sib %02x" % modrm)
    elif rm == 5 and mod == 0:
      chk(e['mode'] == 'MODE_MEM' and e['base'] == ABSENT and e['disp'] == 4,
          "32 disp32 %02x" % modrm)
    else:
      want = {0: 0, 1: 1, 2: 4}[mod]
      chk(e['mode'] == 'MODE_MEM' and e['base'] == rm and e['disp'] == want,
          "32 base %02x" % modrm)
      chk(e['seg_row'] == sbo[rm], "32 segrow %02x" % modrm)
    # --- 16-bit ---
    e = interp.modrm_entry(1, modrm)
    if mod == 3:
      chk(e['mode'] == 'MODE_REG' and e['base'] == rm, "16 reg-direct %02x" % modrm)
    elif mod == 0 and rm == 6:
      chk(e['mode'] == 'MODE_MEM' and e['base'] == ABSENT and e['disp'] == 2,
          "16 disp16 %02x" % modrm)
    else:
      b, x = rm16[rm]
      want = {0: 0, 1: 1, 2: 2}[mod]
      chk(e['base'] == b and e['index'] == x and e['disp'] == want,
          "16 rm16 %02x" % modrm)
      chk(e['seg_row'] == sbo16[rm], "16 segrow %02x" % modrm)

  for mod in (0, 1, 2):
    for sib in range(256):
      sc, ix, bs = sib >> 6, (sib >> 3) & 7, sib & 7
      e = interp.sib_entry(mod, sib)
      chk(e['scale'] == sc, "sib scale %d/%02x" % (mod, sib))
      chk(e['index'] == (ABSENT if ix == 4 else ix), "sib index %d/%02x" % (mod, sib))
      if mod == 0 and bs == 5:
        chk(e['base'] == ABSENT and e['disp'] == 4, "sib disp32 %02x" % sib)
        chk(e['seg_row'] == 0, "sib segrow none %02x" % sib)
      else:
        want = {0: 0, 1: 1, 2: 4}[mod]
        chk(e['base'] == bs and e['disp'] == want, "sib base %d/%02x" % (mod, sib))
        chk(e['seg_row'] == sbo[bs], "sib segrow %d/%02x" % (mod, sib))

  # prefixes
  pfx = interp.prefix_table()
  for b in range(256):
    isp = pfx[b][0]
    chk(isp in (0, 1), "pfx flag %02x" % b)

  # ---- FSM layer: run the emitted states and confirm captures match ----
  # A full driver simulation: index the start table, run actions, follow
  # `next`, until halt; then compare captures to the reference decode.
  CAP = lambda n: interp.cap(n)
  NC = len(interp.vars) + len(interp.CAPS) + len(interp.vars) + len(interp.TAILCAPS)

  def run_addr(adrsiz, stream):
    caps = [0] * NC
    acts, nxt = interp.modrm_state(adrsiz, stream[0])
    ip = interp.sim_state(acts, stream[0], stream, 1, 0, caps)
    if nxt.startswith('FSM_SIB'):
      m = int(nxt.split('+')[1].split('*')[0])
      acts2, _ = interp.sib_state(m, stream[ip])
      ip = interp.sim_state(acts2, stream[ip], stream, ip + 1, ip, caps)
    return caps, ip

  filler = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]
  for adrsiz in (0, 1):
    for modrm in range(256):
      mod, reg, rm = modrm >> 6, (modrm >> 3) & 7, modrm & 7
      stream = [modrm, 0x00] + filler                      # sib byte = 0x00 when used
      caps, _ = run_addr(adrsiz, stream)
      ref = interp.modrm_entry(adrsiz, modrm)
      chk(caps[CAP('REG')] == ref['reg'], "fsm reg a%d %02x" % (adrsiz, modrm))
      if ref['mode'] == 'MODE_REG':
        chk(caps[CAP('MODE')] == 1 and caps[CAP('RM')] == ref['base'],
            "fsm regdir a%d %02x" % (adrsiz, modrm))
        continue
      chk(caps[CAP('MODE')] == 0, "fsm mem-mode a%d %02x" % (adrsiz, modrm))
      sr = interp.store_reg
      if ref['mode'] == 'MODE_SIB':
        s = interp.sib_entry(mod, 0x00)
        chk(caps[CAP('BASE')] == sr(s['base']), "fsm sibbase a%d %02x" % (adrsiz, modrm))
        chk(caps[CAP('INDEX')] == sr(s['index']), "fsm sibidx a%d %02x" % (adrsiz, modrm))
        chk(caps[CAP('SCALE')] == s['scale'], "fsm sibscale a%d %02x" % (adrsiz, modrm))
        chk(caps[CAP('SEG')] == s['seg_row'], "fsm sibseg a%d %02x" % (adrsiz, modrm))
      else:
        chk(caps[CAP('BASE')] == sr(ref['base']), "fsm base a%d %02x" % (adrsiz, modrm))
        chk(caps[CAP('INDEX')] == sr(ref['index']), "fsm idx a%d %02x" % (adrsiz, modrm))
        chk(caps[CAP('SEG')] == ref['seg_row'], "fsm seg a%d %02x" % (adrsiz, modrm))

  # ---- prefix FSM: verify vars set, MARK offsets, and count ----
  # stream: 66 67 66 2e <opcode>. opsiz set twice (offsets 0,2) -> last is 2.
  pstream = [0x66, 0x67, 0x66, 0x2e, 0x90]
  caps = [0] * NC
  for i in range(len(interp.vars)):
    caps[interp.poff(i)] = 0xFF                            # 'none' default
  ip = 0
  while True:
    acts, nxt = interp.prefix_state(pstream[ip])
    if not acts and nxt == 'FSM_HALT':
      break                                                # opcode, not consumed
    ip = interp.sim_state(acts, pstream[ip], pstream, ip + 1, ip, caps)
  chk(ip == 4, "pfx count")
  chk(caps[interp.var_id('opsiz')] == 1, "pfx opsiz val")
  chk(caps[interp.poff(interp.var_id('opsiz'))] == 2, "pfx opsiz last-offset")
  chk(caps[interp.poff(interp.var_id('adrsiz'))] == 1, "pfx adrsiz offset")
  chk(caps[interp.poff(interp.var_id('segidx'))] == 3, "pfx segidx offset")
  chk(caps[interp.poff(interp.var_id('lock'))] == 0xFF, "pfx lock none")

  if bad:
    sys.stderr.write("self-check: %d mismatch(es)\n" % bad)
    sys.exit(1)
  sys.stderr.write("self-check: OK (decode tables + FSM sim + prefix offsets)\n")


# --------------------------------------------------------------------------

def main():
  c = parse_corpus("corpus.p")
  interp = Interp(c)
  if "--check" in sys.argv:
    self_check(interp)
  emit(c, interp, "x86_tables.h")


if __name__ == "__main__":
  main()
