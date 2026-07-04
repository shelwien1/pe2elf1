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
  # scan to the matching close brace respecting string literals, so table values
  # may themselves contain braces (the AVX-512 decoration tables hold "{k1}" etc.,
  # which a naive \{[^}]*\} would truncate at the first inner '}').
  for m in re.finditer(r'table\s+(\w+)\s*\{', text):
    i = m.end()
    depth, instr, start = 1, False, m.end()
    while i < len(text) and depth:
      c = text[i]
      if instr:
        if c == '"': instr = False
      elif c == '"': instr = True
      elif c == '{': depth += 1
      elif c == '}': depth -= 1
      i += 1
    items = []
    for it in split_top(text[start:i - 1], ','):
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

GREG_NONE = 15        # value stored in mem_base/mem_index for "none" (16 in 64-bit mode)
GREG_RIP = 17         # x86-64 RIP-relative base marker ([rip+disp32]); distinct from 0..15/NONE
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
    self.mode = int(self.arch.get('mode', 32))
    if self.arch.get('bitorder', 'msb') != 'msb':
      raise NotImplementedError('only bitorder=msb is modelled')

    self.greg = self.tables['greg']
    self.sbo = self.tables['sbo']
    self.sbo16 = self.tables.get('sbo16', [])      # 16-bit addressing: x86-32 only
    # rm16 decomposed into (base_idx, index_idx) by greg name (x86-32 only)
    self.rm16 = []
    for ent in self.tables.get('rm16', []):
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
    self._ppvtab = []                 # legacy-SSE mandatory-prefix -> mnemonic (4 per opcode)
    self._op1, self._op2, self._mnem = self.build_insn()
    # the VEX operand-shape forms live past the legacy ones in the same enum
    self.INSN_FORM = list(self.INSN_FORM) + list(self.VEX_FORMS)
    # VEX/EVEX descriptor cells (compiled from the vex/vex2/evex submatches into
    # FSM-ready (map,pp,W,opcode) -> (mnem,form,files,imm); each appends mnemonics
    # to self._mnem). EVEX has its own opcode-table block (its maps/opcodes differ).
    self._vexgrp = []                 # vexgrp[gid][/digit] -> mnemonic (shift groups)
    self._vex_tab = self.build_vex(['vex', 'vex2'])
    self._evex_tab = self.build_vex(['evex'])
    self._xop_tab = self.build_vex(['xop'])    # AMD XOP (8F); C++-intercepted, FSM-decoded
    self._apx_tab = self.build_vex(['apx'])    # APX EVEX-promoted legacy (62, map 4)
    # 64-bit: route the VEX/EVEX prefixes through the FSM (C4/C5/62 are not legacy
    # here). The capture-based vexp*/evexp* tables decode them. XOP (8F) is also
    # FSM-decoded (xopp1/xopp2/xopop) but the C++ keeps intercepting it -- the FSM
    # can't do the POP(/0)/XOP(map>=8) split on the 8F group byte, so x86dec.hpp
    # runs FSM_XOPP1 only after confirming map>=8.
    if self.mode == 64:
      if self._vex_tab:
        self._op1[0xC4] = ([], 'FSM_VEXP1')
        self._op1[0xC5] = ([], 'FSM_VEXP1C5')
      if self._evex_tab:
        self._op1[0x62] = ([], 'FSM_EVEXP0')

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
    if 'greg[$r]' in tmpl or 'areg[$r]' in tmpl:
      seg = self.seg_row_from_tmpl(tmpl, rm)
      return dict(mode='MODE_MEM', reg=reg, base=rm, index=ABSENT,
                  disp=disp, seg_row=seg, sib_mod=0)
    if '[rip' in tmpl or '[eip' in tmpl:                  # x86-64 RIP-relative
      return dict(mode='MODE_MEM', reg=reg, base=GREG_RIP, index=ABSENT,
                  disp=disp, seg_row=0, sib_mod=0)
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
  def prefix_rules(self):
    # pfx frames that carry literal bits: the simple one-byte prefixes (each with a
    # var-setting action) and the x86-64 REX frame `0100 w r x b` (no action -- it
    # only consumes the byte into pfx[]; C++ extracts W/R/X/B from the raw byte, so
    # no scarce capture slot is spent on REX).
    if not hasattr(self, '_pfx_rules'):
      self._pfx_rules = [r for r in parse_rules(self.subs['pfx'][1])
                         if r['bp'] and r['bp'].lit_mask]
    return self._pfx_rules

  def prefix_match(self, byte):
    for r in self.prefix_rules():
      if r['bp'].match(byte):
        return r
    return None

  def prefix_table(self):
    # per-byte (is_prefix, rule|None) -- kept for the self-check's flag probe
    out = [(0, None)] * 256
    for b in range(256):
      r = self.prefix_match(b)
      if r is not None:
        out[b] = (1, r)
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
  OPF = ['GREG', 'RGB', 'XMM', 'MM', 'SREG', 'SSE_OS', 'MMG', 'GREGd', 'GREGq', 'GREGw', 'BND']

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
    # a stored register number -> its canonical greg name; sentinels pass through
    if val == GREG_NONE:
      return 'GREG_NONE'
    if val == GREG_RIP:
      return 'GREG_RIP'
    return 'GREG_' + str(self.greg[val]).upper()

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
    if e['base'] == GREG_RIP:                           # x86-64 RIP-relative: rm=101 is fixed
      acts.append(('CONST', self.cap('BASE'), GREG_RIP, 0))
      acts.append(('CONST', self.cap('INDEX'), self.store_reg(ABSENT), 0))
    elif adrsiz == 0 or self.mode == 64:                # 32/64-bit: base is the rm field (low 3)
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
    r = self.prefix_match(byte)
    if r is None:
      return [], 'FSM_HALT'                              # dead: opcode, not consumed
    acts = []
    primary = None                                       # group var whose offset MARK records
    for var, spec in r['action'].items():                # 66->{opsiz=1}; REX->{rexw=$w;...;rex=1}
      vidx = self.var_id(var)
      if spec[0] == 'int':
        acts.append(('CONST', vidx, spec[1], 0))
        if spec[1]:
          primary = vidx                                 # the presence flag ($opsiz=1 / $rex=1)
      elif spec[0] == 'field':                           # REX bit: $rexw=$w etc.
        pos, w = r['bp'].fields[spec[1]]
        acts.append(('FIELD', vidx, pos + w - 1, pos))
    if primary is None and acts:
      primary = acts[0][1]
    if primary is not None:
      acts.append(('MARK', self.poff(primary), 0, 0))
    return acts, 'FSM_PREFIX'

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
  # ACC = implicit-accumulator + imm (add eax,imm): like REG_IMM but the register
  # is NOT encoded, so REX.B must not extend it (distinguished from B8+r).
  INSN_FORM = ['NONE', 'MODRM', 'REG', 'REG_IMM', 'IMM', 'REL', 'PTR', 'GROUP', 'ESC', 'RM', 'ACC']
  # IMMV = operand-size immediate (imm16/imm32/imm64 by 66 / default / REX.W). Used
  # only by mov r,imm (B8+r): under REX.W it is the full 64-bit movabs immediate. The
  # width is resolved C++-side (append_imm/enc_imm) since REX.W is not an FSM var.
  INSN_IMK = ['NONE', 'IMM8', 'IMM16', 'IMM32', 'IMMZ', 'REL8', 'RELZ', 'PTR', 'IMM8SX', 'ENTER', 'IMMV', 'IMM8X2']

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
    pp = None                                            # [$pp==N] guard: per-prefix descriptor slot
    for t in list(toks):
      gm = re.fullmatch(r'\[\$pp==([0-3])\]', t)
      if gm:
        pp = int(gm.group(1)); toks.remove(t)
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
    suffix = None                                        # 3DNow!: trailing opcode byte @suf(N)
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
    fixmodrm = None
    j = 0
    if j < len(rest) and rest[j] == '11':
      mid = rest[j + 1]
      if re.fullmatch(r'[01]+', mid):
        reg_fixed = int(mid, 2)
        modrm = 'group_reg'
        rmt = rest[j + 2] if j + 2 < len(rest) else 'rrr'
        if re.fullmatch(r'[01]{3}', rmt):            # both reg AND rm literal: a fully-fixed
          fixmodrm = 0xC0 | (reg_fixed << 3) | int(rmt, 2)   # ModR/M, no operand (endbr64, monitor..)
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
      elif nm in ('imm8', 'imm16', 'imm32', 'immz', 'immv', 'rel8', 'relz'):
        imm.append(nm)
      elif nm == 'suf':                                  # 3DNow! trailing opcode byte
        suffix = int(arg, 0)
    return dict(bp=bp, tb=tb, two_byte=two_byte, modrm=modrm,
                reg_fixed=reg_fixed, imm=imm, embedded=embedded, pp=pp,
                fixmodrm=fixmodrm, suffix=suffix)

  def operand_file(self, tmpl, var):
    # which register-name table the $var operand draws from
    m = re.search(r'(greg|gregd|gregq|gregw|rgb|sreg|ssereg|mmxg|bndreg)\[([^\]]*\$' + var + r'[^\]]*)\]', tmpl)
    if not m:
      return 'GREG'
    tab, idx = m.group(1), m.group(2)
    if tab == 'rgb':
      return 'RGB'
    if tab == 'sreg':
      return 'SREG'
    if tab == 'gregd':                                   # r32/r64 GPR, never 16 (mandatory-66 SSE ops)
      return 'GREGd'
    if tab == 'gregq':                                   # r64 GPR always (vmread/vmwrite)
      return 'GREGq'
    if tab == 'gregw':                                   # r16 GPR always (movzx/movsx word src)
      return 'GREGw'
    if tab == 'bndreg':                                  # MPX bound register (bnd0-3)
      return 'BND'
    if tab == 'mmxg':                                    # mm (NP/66) or GPR (F3/F2), by reptype
      return 'MMG'
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
    ppd = {}                                             # (tb,byte) -> [slot0..3] per-prefix desc
    groups = {}
    self._fixmodrm = {}                                  # (tb,opcode) -> {modrm_byte: mnem} no-operand
    self._tdnow = {}                                     # 3DNow!: suffix byte -> mnemonic (0F 0F)
    for lhs, rhs in self.insn_rules_raw():
      info = self.parse_insn_lhs(lhs)
      if info is None:
        continue
      if info['modrm'] in ('group_reg', 'group_mem'):    # opcode-extension group
        self._collect_group(groups, info, rhs, midx, K)  # one-byte AND two-byte (0F) groups
        continue
      if info['suffix'] is not None:                     # 3DNow!: mnemonic in a trailing byte
        self._tdnow[info['suffix']] = midx(self.insn_mnem(rhs)[1])
        d = desc[info['tb']].setdefault(info['bp'].lit_val, {
            'form': F['MODRM'], 'imk': K['NONE'], 'rfile': OPF['MM'], 'mfile': OPF['MM'],
            'sfx': 0, 'dir': 0, 'mnsel': 0, 'ppsel': 0, 'emb': None, 'mnem': 0, 'reg0': False,
            'cc': None, 'has_reg': False, 'has_mem': False, 'mnem_reg': None, 'mnem_mem': None,
            'form_reg': None, 'mfile_reg': None, 'tdnow': 1})
        d['mnem'] = midx(self.insn_mnem(rhs)[1])         # placeholder (overwritten per suffix)
        if info['modrm'] == 'reg': d['has_reg'] = True
        if info['modrm'] == 'mem': d['has_mem'] = True
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
      elif imml == ['imm8', 'imm8']:
        imk = K['IMM8X2']                                # extrq/insertq Ib,Ib
      elif len(imml) == 1:
        imk = K['IMM8SX'] if (imml[0] == 'imm8' and 'sx8(' in rhs) else K[imml[0].upper()]
      elif not imml:
        imk = K['NONE']
      else:
        continue
      has_g = bool(re.search(r'(greg|gregd|gregq|gregw|rgb|sreg|ssereg|mmxg|bndreg)\[[^\]]*\$g[^\]]*\]', rhs))
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
        form = F['ACC']                                  # implicit accumulator (eAX/al, imm)
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
        fresh = {'form': F['NONE'], 'imk': K['NONE'],
                 'rfile': OPF['GREG'], 'mfile': OPF['GREG'],
                 'sfx': 0, 'dir': 0, 'mnsel': 0, 'ppsel': 0,
                 'emb': None, 'mnem': None, 'reg0': False,
                 'cc': None, 'has_reg': False, 'has_mem': False,
                 'mnem_reg': None, 'mnem_mem': None,       # pp slots: mod-dependent mnemonic
                 'form_reg': None, 'mfile_reg': None,      # ... and mod-dependent form/file
                 'dir_mem': 0, 'dir_reg': 0}               # ... and mod-dependent ModR/M order
        if info['pp'] is not None:                       # per-prefix descriptor slot
          slots = ppd.setdefault((tb, byte), [None, None, None, None])
          if slots[info['pp']] is None:
            slots[info['pp']] = dict(fresh)
          d = slots[info['pp']]
        else:
          d = desc[tb].setdefault(byte, fresh)
        if info['modrm'] == 'reg': d['has_reg'] = True     # which ModR/M mod forms
        if info['modrm'] == 'mem': d['has_mem'] = True     # this opcode actually defines
        # mnemonic
        if mn[0] == 'lit':
          d['mnem'] = midx(mn[1])
        elif mn[0] == 'cond':
          d['mnem'] = midx(mn[1] + 'cc')               # generic jcc / setcc / cmovcc
          d['cc'] = v                                  # condition code -> immediate operand
        elif len(mn) > 2 and mn[2] == 'pp':            # legacy-SSE mandatory-prefix select
          tab = self.tables[mn[1]]                      # 4 entries: NP, 66, F3, F2
          # '-' marks a #UD prefix slot (opcode valid only under specific mandatory
          # prefixes, e.g. haddpd is 66-only): 0xFFFF -> the MNSEL==2 C++ override
          # yields mnem 0xFFFF -> #UD for that prefix. Valid slots are unchanged.
          vt = tuple(0xFFFF if x == '-' else midx(x) for x in tab)
          if vt not in self._ppvtab:
            self._ppvtab.append(vt)
          d['mnem'] = vt[0]                             # NP mnemonic is the base
          d['ppsel'] = self._ppvtab.index(vt) + 1      # 1-based; 0 = not pp-selected
        else:                                          # opsize-selected table (movs/cdqw)
          tab = self.tables[mn[1]]
          d['mnem'] = midx(tab[0])
          for k in range(1, len(tab)):
            midx(tab[k])                                 # variants contiguous
          d['mnsel'] = 1
        if info['pp'] is not None and d['mnem'] is not None:  # movlps(mem)/movhlps(reg) split
          if info['modrm'] == 'reg': d['mnem_reg'] = d['mnem']
          elif info['modrm'] == 'mem': d['mnem_mem'] = d['mnem']
        d['form'] = form
        d['imk'] = imk if imk != K['NONE'] else d['imk']
        d['emb'] = emb if emb in ('b',) else d['emb']
        if form == F['ACC']:
          d['reg0'] = True
        # files: reg-direct rule supplies both; mem rule supplies the suffix
        if info['modrm'] == 'reg':
          d['rfile'] = OPF[self.operand_file(rhs, 'g')]
          d['mfile'] = OPF[self.operand_file(rhs, 'r')]
          d['dir'] = dir_rm; d['dir_reg'] = dir_rm       # dir_reg: pp-slot reg-form order
          if info['pp'] is not None:                     # reg-direct form/file (may differ from mem)
            d['form_reg'] = form; d['mfile_reg'] = d['mfile']
        elif info['modrm'] == 'mem':
          d['rfile'] = OPF[self.operand_file(rhs, 'g')]
          d['dir'] = dir_rm; d['dir_mem'] = dir_rm       # dir_mem: pp-slot mem-form order
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
        acts.append(('CONST', self.tailcap('MNSEL'), 1, 0))       # mode 1: += opsize
      if d['ppsel']:                                              # mode 2: SSE prefix select
        acts.append(('CONST', self.tailcap('MNSEL'), 2, 0))       # (GRP is unused by MODRM ops,
        acts.append(('CONST', self.tailcap('GRP'), d['ppsel'] - 1, 0))  # so it carries the vtab idx)
      if d.get('tdnow'):                                          # mode 4: 3DNow! trailing-suffix opcode
        acts.append(('CONST', self.tailcap('MNSEL'), 4, 0))
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
    # per-prefix full descriptor: opcodes whose mnemonic AND operand form flip with the
    # mandatory prefix (movd/movq 7E, movq/movq2dq/movdq2q D6). One opcode state carries
    # MNSEL mode 3 + the ppdesc index; the C++ picks ppdesc[idx][pp] after the prefix run
    # is known and overrides mnem/form/dir/files/imk. A missing slot decodes as #UD.
    self._ppdesc = []                                       # [idx] -> [slot0..3]
    ppmap = {0: op1, 1: op2, 2: op3_38, 3: op3_3a}
    for (tb, byte), slots in sorted(ppd.items()):
      idx = len(self._ppdesc)
      entry, base = [], 0
      for pp in range(4):
        s = slots[pp]
        if s is None:
          entry.append(None)                               # 0xFFFF -> #UD for this prefix
          continue
        hr, hm = s['has_reg'], s['has_mem']
        rmreq = 1 if (hr and not hm) else (2 if (hm and not hr) else 0)
        mm, mr = s['mnem_mem'], s['mnem_reg']            # mnemonic may depend on mod
        # mem is the base; reg overrides below. FORM_NONE ops (no ModR/M, e.g. 0F09
        # wbinvd/wbnoinvd) never set mnem_mem/mnem_reg, so fall back to the plain mnem.
        mn_base = mm if mm is not None else (mr if mr is not None else s['mnem'])
        mreg = mr if (mr is not None and mr != mn_base) else 0xFFFF
        # reg-direct form/file override, when the reg form is structurally different
        # from the mem form (MPX 0F1A/1B NP: mem=bndldx/bndstx, reg=nop r/m)
        rform, rmf = 0xFF, 0
        if s['form_reg'] is not None and mm is not None and s['form_reg'] != s['form']:
          rform, rmf = s['form_reg'], s['mfile_reg']
        # per-mod ModR/M order: the pre-ModR/M CAP_DIR uses the mem form's order (the
        # common path); once reg-vs-mem is known, a reg-direct form overrides to dir_reg.
        # Lets urdmsr (reg: r/m,reg) coexist with enqcmd (mem: reg,mem) at 0F38 F8 pp3.
        dir_emit = s['dir_mem'] if hm else s['dir_reg']
        dirreg_emit = s['dir_reg'] if hr else dir_emit
        entry.append((mn_base, s['form'], dir_emit, s['rfile'], s['mfile'], s['imk'], rmreq, mreg, rform, rmf, dirreg_emit))
        base = base or mn_base
      self._ppdesc.append(entry)
      ppmap[tb][byte] = ([('MNEM', base, 0, 0),
                          ('CONST', self.tailcap('MNSEL'), 3, 0),
                          ('CONST', self.tailcap('GRP'), idx & 0x1f, idx >> 5)], 'FSM_HALT')
    # group opcodes: assign ids, store members, route the owning map -> group stage.
    # A group whose members/fixmodrm carry a [$pp==N] guard becomes a *pp-variant*
    # group: it occupies 4 consecutive gids (one per mandatory-prefix slot) and the
    # C++ offsets the baked base gid by pp before running the ModR/M stage. Unguarded
    # (pp==None) rules seed every slot; a pp-guarded rule overrides its slot (0F AE
    # rdfsbase/incssp/clwb/tpause..., 0F 1E endbr64/rdssp -- all mandatory-prefix).
    self._group_list = []
    self._ppgroup = []                                     # gid -> 1 if pp-variant base
    group_map = {0: op1, 1: op2, 2: op3_38, 3: op3_3a}      # dispatch by escape (tb)
    for (tb, opcode) in sorted(groups):
      members = groups[(tb, opcode)]                       # {(pp,reg): m}
      fixmap  = self._fixmodrm.get((tb, opcode), {})       # {(pp,modrm): mnem}
      is_pp = any(pp is not None for (pp, _) in members) or \
              any(pp is not None for (pp, _) in fixmap)
      base_gid = len(self._group_list)
      for slot in (range(4) if is_pp else (None,)):
        mem_r, fix_r = {}, {}
        # Combine the base (pp==None) and this slot's members FIELD BY FIELD, not by
        # whole-dict replace: a reg's mem-form and reg-form can come from different
        # rules/prefixes (F3 0F AE /0 has mem=fxsave [unguarded] AND reg=rdfsbase [F3]).
        for want in (None, slot):                          # base first, pp-override second
          for (pp, reg), mm in members.items():
            if pp != want: continue
            t = mem_r.setdefault(reg, {'mnem_reg': None, 'mnem_mem': None,
                                       'imk': 0, 'rfile': 0, 'mnsel': 0})
            for k in ('mnem_reg', 'mnem_mem'):
              if mm[k] is not None: t[k] = mm[k]
            for k in ('imk', 'rfile', 'mnsel'):
              if mm.get(k): t[k] = mm[k]
          for (pp, mod), mn in fixmap.items():
            if pp == want: fix_r[mod] = mn
        self._group_list.append((tb, opcode, mem_r, fix_r))
        self._ppgroup.append(1 if (is_pp and slot == 0) else 0)
      group_map[tb][opcode] = ([('CONST', self.tailcap('FORM'), F['GROUP'], 0),
                                ('CONST', self.tailcap('GRP'), base_gid & 0x1f, base_gid >> 5)], 'FSM_HALT')
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

  # ======================================================================
  # VEX / EVEX / XOP: compile the vex/vex2/evex/xop submatches into a flat
  # (mapidx, pp, W, opcode) -> descriptor table. The VEX prefix fields don't
  # fit the FSM's 32 capture slots (REX has the same problem and is handled in
  # C++), so the FSM never walks these rules. This builds the lookup the C++
  # vex_decode indexes from the raw VEX bytes: the corpus supplies the mnemonic
  # and the per-operand ROLE (reg / rm / vvvv / imm8 / is4) + FILE; the C++ then
  # applies the standard R/X/B/R'/V' register extensions. The byte tail split
  # (has_modrm / imm length) still comes from vex_structure, so the bijection is
  # unaffected -- a table miss simply falls back to the structural placeholder.
  # ----------------------------------------------------------------------
  # register-name tables (an operand draws a register number from one of these);
  # any other bracketed table before the first operand is a mnemonic suffix.
  VEX_REGFILES = ('vreg', 'vvv', 'greg', 'rgb', 'ssereg', 'kreg', 'ereg',
                  'evvv', 'eregh', 'eregx', 'vregd', 'xreg', 'mmreg', 'areg', 'sreg', 'tmmreg')
  # leading bit-field layout per submatch (name,width). map/W/pp drive the table
  # key; R/X/B/vvvv/L are runtime (the C++ reads them from the raw prefix bytes).
  VEX_LAYOUT = {
    'vex':  [('R', 1), ('X', 1), ('B', 1), ('map', 5), ('W', 1), ('vvvv', 4), ('L', 1), ('pp', 2)],
    'vex2': [('R', 1), ('vvvv', 4), ('L', 1), ('pp', 2)],     # C5: map=1, W=0, X=B=0
    'xop':  [('R', 1), ('X', 1), ('B', 1), ('map', 5), ('W', 1), ('vvvv', 4), ('L', 1), ('pp', 2)],
    # EVEX: P0 = R'X'B'R'' 0 mmm ; P1 = W vvvv 1 pp ; P2 = z L'L b V'' aaa. mmm is a 3-bit
    # map (1/2/3 = 0F/0F38/0F3A ; 5/6 = the AVX512-FP16 MAP5/MAP6). Token-parsed, so the
    # existing "00 01"-style rules still read map=1 (rsv token 0, map token 01).
    'evex': [('R', 1), ('X', 1), ('B', 1), ('Rp', 1), ('rsv', 1), ('map', 3),
             ('W', 1), ('vvvv', 4), ('one', 1), ('pp', 2),
             ('z', 1), ('L', 2), ('bcst', 1), ('Vp', 1), ('aaa', 3)],
    # APX EVEX-promoted legacy: P0 = R3 X3 B3 R4 B4 mmm(=100, 3-bit map); P1 = W vvvv X4 pp;
    # P2 = p2hi(3) ND V4 aaa.  The C++ apx_finalize folds the r0-31 extension + sizing + the
    # ND (new-data-dest in vvvv) / NF modifiers; the rule only conveys opcode/pp/W + legacy shape.
    'apx':  [('R', 1), ('X', 1), ('B', 1), ('Rp', 1), ('B4', 1), ('map', 3),
             ('W', 1), ('vvvv', 4), ('X4', 1), ('pp', 2),
             ('p2hi', 3), ('nd', 1), ('V4', 1), ('aaa', 3)],
  }
  # decoration tables that ride after an operand (EVEX {k}/{z}/{1toN}/{er}); they
  # are runtime (the C++ reads them from the captured EVEX P2 byte), not operands.
  VEX_DECOR = ('kzdec', 'kdec', 'rcdec', 'bcst32', 'bcst64')
  # role / file enums shared with the C++ (keep in sync with x86dec.hpp)
  VR = {'NONE': 0, 'REG': 1, 'RM': 2, 'VVVV': 3, 'IMM8': 4, 'IS4': 5}
  VF = {'VEC': 0, 'GPR32': 1, 'GPR64': 2, 'KREG': 3, 'VECX': 4, 'MEM': 5, 'GPR16': 6}

  def _vex_tokens(self, rhs):
    # tokenize a template RHS: ('str',t) | ('ref',name,expr) | ('call',name,arg) | ('bare',t)
    out = []
    pat = re.compile(r'"([^"]*)"|(\w+)\[([^\]]*)\]|(\w+)\(([^)]*)\)|(\$?\w+)')
    for m in pat.finditer(rhs):
      if m.group(1) is not None:   out.append(('str', m.group(1)))
      elif m.group(2) is not None: out.append(('ref', m.group(2), m.group(3)))
      elif m.group(4) is not None: out.append(('call', m.group(4), m.group(5)))
      else:                        out.append(('bare', m.group(6)))
    return out

  @staticmethod
  def _vex_eval(expr, env):
    s = re.sub(r'\$(\w+)', lambda m: str(env.get(m.group(1), 0)), expr)
    if not re.fullmatch(r'[0-9+\-*/() ]*', s):
      return None
    try:
      return int(eval(s)) if s.strip() else 0
    except Exception:
      return None

  def _vex_operand(self, t, env):
    # map one operand atom -> (role, file). None role => unsupported (skip rule).
    # an immediate atom carries its KIND in the file slot (imm8/immz/imm8sx) so the
    # FSM can set the right CAP_IMK -- APX promotes immz/imm8sx forms, not just imm8.
    def _imk(s):
      if 'imm32' in s:               return 'IMM32'   # XOP bextr/lwpins/lwpval control imm32
      if 'immz' in s or 'immv' in s: return 'IMMZ'
      if 'sx8' in s:                 return 'IMM8SX'
      return 'IMM8'
    if t[0] == 'bare':
      if t[1] == '$addr':            return ('RM', 'MEM')
      if t[1].startswith('$imm'):    return ('IMM8', _imk(t[1]))
      return (None, None)
    if t[0] == 'call':
      if 'imm' in t[2]:              return ('IMM8', _imk(t[2]))
      return (None, None)
    name, expr = t[1], t[2]
    role = ('VVVV' if '$v' in expr else 'REG' if '$g' in expr else 'RM' if '$r' in expr else None)
    if name in ('ereg', 'evvv'):
      file = 'VEC'                                   # EVEX: always sized by the captured L'L
    elif name in ('vreg', 'vvv'):
      # VEX: a length term ($y) => sized by L; without it the operand is a fixed
      # xmm (the narrow side of a VEX width-changer, e.g. vcvtps2pd's vreg[8+$r]).
      file = 'VEC' if '$y' in expr else 'VECX'
    elif name == 'eregh':
      file = 'VECH'                                 # one size class down (vcvt widen/narrow)
    elif name == 'eregx':
      file = 'VECX'                                 # always xmm (vpmovzx/sx, vextract)
    elif name == 'greg':
      c = self._vex_eval(re.sub(r'\$\w+', '0', expr), {}) or 0
      file = 'GPR64' if c >= 32 else 'GPR16' if c >= 16 else 'GPR32'
    elif name == 'kreg':
      file = 'KREG'
    elif name == 'tmmreg':
      file = 'TMM'                                  # AMX tile register tmm0..tmm7
    elif name == 'rgb':
      file = 'GPR8'                                 # APX 8-bit GPR (al..r31b, uniform set)
    elif name == 'vregd':
      role, file = 'IS4', 'VEC'
    elif name == 'ssereg':
      file = 'VEC'
    else:
      return (None, None)
    return (role, file)

  def _vex_parse_rule(self, kind, lhs, rhs):
    # -> (mapidx, [pp...], [W...], opcode, mnem_template_atoms, op_atoms) or None
    toks = lhs.split()
    layout = self.VEX_LAYOUT[kind]
    nf = len(layout)
    if len(toks) < nf + 1:
      return None
    kf = {}
    for (fname, _fw), tok in zip(layout, toks[:nf]):
      if re.fullmatch(r'[01]+', tok):
        kf[fname] = ('lit', int(tok, 2))
      else:
        kf[fname] = ('var',)
    rest = toks[nf:]
    m = re.fullmatch(r'0x([0-9a-fA-F]{2})', rest[0])
    if not m:
      return None
    opcode = int(m.group(1), 16)
    if kind == 'vex2':
      mapval = 1
    elif kf['map'][0] == 'lit':
      mapval = kf['map'][1]
    else:
      return None
    # mapidx 0..2 within this submatch's own opcode-table block
    if kind == 'xop':
      mapidx = {8: 0, 9: 1, 10: 2}.get(mapval)
    elif kind == 'apx':
      mapidx = 0 if mapval == 4 else None     # APX uses the single legacy map (mmm=4)
    elif kind in ('vex', 'vex2'):
      mapidx = {1: 0, 2: 1, 3: 2, 7: 3}.get(mapval)   # VEX maps 1/2/3 + 7 (USER_MSR imm)
    else:                                             # evex
      mapidx = {1: 0, 2: 1, 3: 2, 5: 3, 6: 4}.get(mapval)
    if mapidx is None:
      return None
    wf = kf.get('W', ('lit', 0))
    wvals = [wf[1]] if wf[0] == 'lit' else [0, 1]
    ppf = kf['pp']
    ppvals = [ppf[1]] if ppf[0] == 'lit' else [0, 1, 2, 3]
    # opcode-extension group? reg-direct with a literal /digit (0F 73 /6 vpsllq),
    # or a memory form whose /digit rides @addr(N) (the EVEX memory-source shifts).
    digit = None
    if len(rest) >= 4 and rest[1] == '11' and re.fullmatch(r'[01]{3}', rest[2]):
      digit = int(rest[2], 2)
    else:
      for tok in rest[1:]:
        am = re.fullmatch(r'@addr\((\d+)\)', tok)
        if am:
          digit = int(am.group(1)); break
    return (mapidx, ppvals, wvals, opcode, digit)

  # VEX operand-role order -> a compact form enum the C++ finalize switches on.
  # files are carried separately (CAP_RFILE = dst/reg file, CAP_MFILE = r/m file).
  VEX_FORMS = ['VEX_NONE', 'VEX_RVM', 'VEX_RVMI', 'VEX_RVMR', 'VEX_RM', 'VEX_RMI',
               'VEX_MR', 'VEX_MRI', 'VEX_VM', 'VEX_R', 'VEX_M', 'VEX_RVMV',
               'VEX_VMI', 'VEX_VMG',  # VMI: vvvv,r/m,imm8 ; VMG: same, mnem by /digit
               'VEX_MG', 'VEX_VMG0',  # MG: r/m only, mnem by /digit (XOP LWP llwpcb/slwpcb);
                                      # VMG0: vvvv,r/m, mnem by /digit (XOP TBM blc*/bls*/tzmsk/t1mskc)
               'VEX_RMV', 'VEX_RVRM',  # RMV: reg,r/m,vvvv (XOP vprot/vpsh W0);
                                       # RVRM: reg,vvvv,is4,r/m (XOP vpcmov/vpperm W1)
               'VEX_MI', 'VEX_IM',    # MI: r/m,imm ; IM: imm,r/m  (VEX map7 urdmsr/uwrmsr)
               'VEX_MVR',             # MVR: r/m,vvvv,reg (vmovss/vmovsd 0F 11 store reg-form)
               'VEX_MRV',             # MRV: r/m,reg,vvvv (CMPccXADD [mem],reg,vvvv)
               # APX EVEX-promoted legacy (GPR; apx_finalize prepends NDD dest if ND=1)
               'APX_MR', 'APX_RM', 'APX_RMI', 'APX_MI', 'APX_M', 'APX_R',
               'APX_MRI', 'APX_MRC']
  VEXFILE = {'VEC': 0, 'GPR32': 1, 'GPR64': 2, 'KREG': 3, 'VECH': 4, 'VECX': 5,
             'MEM': 0, 'GPR16': 1, 'NONE': 0, 'GPR8': 6, 'TMM': 7}
  _VEX_PAT = {
    'REG,VVVV,RM': 'VEX_RVM', 'REG,VVVV,RM,IMM8': 'VEX_RVMI',
    'REG,VVVV,RM,IS4': 'VEX_RVMR', 'REG,RM': 'VEX_RM', 'REG,RM,IMM8': 'VEX_RMI',
    'RM,REG': 'VEX_MR', 'RM,REG,IMM8': 'VEX_MRI', 'VVVV,RM': 'VEX_VM',
    'REG': 'VEX_R', 'RM': 'VEX_M', '': 'VEX_NONE', 'REG,VVVV,RM,VVVV': 'VEX_RVMV',
    'REG,RM,VVVV': 'VEX_RMV', 'REG,VVVV,IS4,RM': 'VEX_RVRM',
    'RM,IMM8': 'VEX_MI', 'IMM8,RM': 'VEX_IM',   # VEX map7 USER_MSR: urdmsr r/m,imm32 ; uwrmsr imm32,r/m
    'RM,VVVV,REG': 'VEX_MVR',   # vmovss/vmovsd 0F 11 store reg-form (r/m, vvvv, reg)
    'RM,REG,VVVV': 'VEX_MRV',   # CMPccXADD: [mem], reg, vvvv (mem dst, GPR compare, GPR add)
  }

  # APX promoted-legacy operand shapes (GPR; the C++ apx_finalize prepends the NDD
  # dest from vvvv when the EVEX P2.ND bit is set, so one cell serves 2-op and 3-op).
  _APX_PAT = {
    'RM,REG': 'APX_MR', 'REG,RM': 'APX_RM', 'REG,RM,IMM8': 'APX_RMI',
    'RM,IMM8': 'APX_MI', 'RM': 'APX_M', 'REG': 'APX_R',
    'RM,REG,IMM8': 'APX_MRI', 'RM,REG,REG': 'APX_MRC',   # shld/shrd imm / cl
  }

  def _vex_form(self, ops, kind='vex'):
    # ops: list of (role,file). -> (form_name, rfile_code, mfile_code, imm) or None.
    pat = ','.join(r for r, f in ops)
    form = (self._APX_PAT if kind == 'apx' else self._VEX_PAT).get(pat)
    if form is None:
      return None
    rfile = next((f for r, f in ops if r in ('REG', 'IS4')), 'VEC')
    mfile = next((f for r, f in ops if r == 'RM'), 'VEC')
    # imm carries the CAP_IMK value (so append_imm reads the right width): IS4 and a
    # plain imm8 -> 1 (IMK_IMM8); APX immz -> 4 (IMK_IMMZ); imm8sx -> 8 (IMK_IMM8SX).
    IMK = {'IMM8': 1, 'IMM32': 3, 'IMMZ': 4, 'IMM8SX': 8}
    imm = 0
    for r, f in ops:
      if r == 'IS4':    imm = 1
      elif r == 'IMM8': imm = IMK.get(f, 1)
    return (form, self.VEXFILE.get(rfile, 0), self.VEXFILE.get(mfile, 0), imm)

  def build_vex(self, kinds):
    # tab[(mapidx,pp,W,opcode)] = (mnem_index, form_name, rfile, mfile, imm).
    # Keyed by (map,pp,W): pp and W are baked into the opcode-table coordinate, so
    # the FSM needs no runtime pp/W mnemonic selection. The C++ vexop FSM states
    # are generated straight from this; a missing key is a dead (rejected) opcode.
    # `kinds` selects which submatch(es) feed this block (vex+vex2 / evex / xop).
    raw = {}
    groups, groupmeta = {}, {}        # opcode-extension groups (mnemonic by /digit)

    def midx(s):
      if s not in self._mnem:
        self._mnem.append(s)
      return self._mnem.index(s)

    for kind in kinds:
      if kind not in self.subs:
        continue
      for chunk in split_top(self.subs[kind][1], ';'):
        if '=>' not in chunk:
          continue
        lhs, rhs = chunk.split('=>', 1)
        if not lhs.strip():
          continue
        parsed = self._vex_parse_rule(kind, lhs.strip(), rhs.strip())
        if parsed is None:
          continue
        mapidx, ppvals, wvals, opcode, digit = parsed
        atoms = self._vex_tokens(rhs.strip())
        for W in wvals:
          for pp in ppvals:
            env = {'p': pp, 'W': W, 'y': 0, 'l': 0}
            # split atoms into the mnemonic and the operand list
            mn, j, ok = '', 0, True
            while j < len(atoms):
              t = atoms[j]
              if t[0] == 'call' and t[1] == 'wit':
                j += 1                                       # witness marker (e.g. wit("evex"))
              elif t[0] == 'str':
                if '[' in t[1]:
                  break                                      # start of a memory operand (scatter dst)
                mn += t[1]; j += 1
              elif t[0] == 'ref' and t[1] not in self.VEX_REGFILES and t[1] not in self.VEX_DECOR:
                tabnm = self.tables.get(t[1])
                idx = self._vex_eval(t[2], env)
                if tabnm is None or idx is None or idx >= len(tabnm) or tabnm[idx] == '':
                  ok = False; break
                mn += str(tabnm[idx]); j += 1
              else:
                break
            mnem = mn.strip()
            if not ok or not mnem:
              continue
            ops = []
            in_mem = False                                   # inside a "[ ... ]" memory template
            for t in atoms[j:]:
              if t[0] == 'str':
                # a hand-built memory operand (VSIB gather/scatter): "[" base "+"
                # index "*" scale "]" -- collapse the whole bracket span into one
                # r/m operand (the index is a vector reg, but for the byte-exact
                # round-trip the SIB is replayed; it renders as a memory operand).
                if '[' in t[1]: ops.append(('RM', 'MEM')); in_mem = True
                if ']' in t[1]: in_mem = False
                continue
              if in_mem:
                continue                                     # absorbed into the bracket operand
              if t[0] == 'call' and t[1] == 'wit':
                continue                                     # witness marker
              if t[0] == 'ref' and t[1] in self.VEX_DECOR:
                continue                                     # {k}/{z}/{1toN}/{er} decoration
              role, file = self._vex_operand(t, env)
              if role is None:
                ok = False; break
              ops.append((role, file))
            if not ok:
              continue
            mi = midx(mnem)
            key = (mapidx, pp, W, opcode)
            if digit is not None:                    # opcode-extension group member
              groups.setdefault(key, {})[digit] = mi
              # uniform shape across digits; prefer a concrete r/m file over the MEM
              # marker so a group with both reg-direct and @addr rules (e.g. BMI blsr:
              # r/m is a GPR, not the VEC default MEM maps to) types the reg r/m right.
              prev = groupmeta.get(key)
              if prev is None or any(r == 'RM' and f == 'MEM' for r, f in prev):
                groupmeta[key] = ops
              continue
            if key in raw:
              # merge the paired reg-direct / @addr rule: prefer the concrete
              # register file over the MEM marker, slot by slot.
              omi, oops = raw[key]
              if len(oops) == len(ops):
                merged = [(r1, f1 if f2 == 'MEM' else f2 if f1 == 'MEM' else f1)
                          for (r1, f1), (r2, f2) in zip(oops, ops)]
                raw[key] = (omi, merged)
              continue
            raw[key] = (mi, ops)

    # lower each (map,pp,W,opcode) entry to a VEX FSM cell (mnem, form, files, imm)
    cells = {}
    # opcode-extension groups (shifts 71/72/73): one vexgrp[gid][digit] row of
    # mnemonics; the cell carries the gid and the C++ picks the mnemonic by the
    # ModR/M reg digit. Shape is VVVV(dest), r/m(src), imm8 (VEX_VMG).
    kind = kinds[0] if kinds else 'vex'
    IMK = {'IMM8': 1, 'IMM32': 3, 'IMMZ': 4, 'IMM8SX': 8}
    for key, digmap in groups.items():
      if key in raw:
        continue                                     # a non-group rule already claimed it
      gid = len(self._vexgrp)
      self._vexgrp.append([digmap.get(d, -1) for d in range(8)])
      gops = groupmeta[key]
      # the dest file may ride REG, IS4 or VVVV (XOP TBM/LWP put the GPR dest in vvvv)
      rf = self.VEXFILE.get(next((f for r, f in gops if r in ('REG', 'IS4', 'VVVV')), 'VEC'), 0)
      mf = self.VEXFILE.get(next((f for r, f in gops if r == 'RM'), 'VEC'), 0)
      roles = [r for r, _ in gops]
      if kind == 'apx':                              # APX group: GPR, mnem by /digit, NDD via ND
        has_imm = any(r == 'IMM8' for r, _ in gops)
        form = 'APX_MI' if has_imm else 'APX_M'
        imk = next((IMK.get(f, 1) for r, f in gops if r == 'IMM8'), 0)
      elif roles == ['RM']:                          # XOP LWP llwpcb/slwpcb: r/m only
        form, imk = 'VEX_MG', 0
      elif roles == ['VVVV', 'RM']:                  # XOP TBM blc*/bls*/tzmsk/t1mskc
        form, imk = 'VEX_VMG0', 0
      elif roles == ['VVVV', 'RM', 'IMM8']:          # shift-by-imm8 / XOP LWP lwpins/lwpval imm32
        imf = next(f for r, f in gops if r == 'IMM8')
        form, imk = 'VEX_VMG', IMK.get(imf, 1)
      else:
        form, imk = 'VEX_VMG', 1                      # default: vector shift groups (imm8)
      cells[key] = (-1, form, rf, mf, gid, imk)       # mnem -1 => group; 5th = gid, 6th = imk
    for key, (mi, ops) in raw.items():
      f = self._vex_form(ops, kind)
      if f is None:
        continue                                     # unrecognised shape -> dead opcode
      form, rf, mf, imm = f
      cells[key] = (mi, form, rf, mf, imm)
    # VEX (C4/C5) WIG fallback: the W bit is "ignored" for most VEX 0F/0F38/0F3A ops, so
    # real silicon decodes W0 and W1 identically (e.g. VEX.W1 vaddps / vpminub). A rule
    # that binds a single W would drop the other, sending the non-standard-W form to the
    # structural placeholder; fill an EMPTY W-sibling cell from its defined partner so both
    # decode to the real mnemonic. This never overwrites, so genuinely W-significant ops
    # (FMA vfmadd*ps/pd, vmovd/vmovq -- both W already present) are untouched. VEX only:
    # EVEX/XOP/APX use W as an element-size / operand selector and are left strict.
    # Also applies to EVEX: the AVX-512 byte/word integer ops are WIG too (vpmullw,
    # vpsubb, vpaddw, ...), so W1 is valid and identical. Element-size-significant ops
    # (vpaddd/vpaddq, ps/pd) have BOTH W cells present, so they are never overwritten.
    if 'vex' in kinds or 'evex' in kinds:
      for (mapidx, pp, W, opcode), cell in list(cells.items()):
        sib = (mapidx, pp, 1 - W, opcode)
        if sib not in cells:
          cells[sib] = cell
    return cells

  # ----- VEX FSM states (capture-based; reuse REL/CC/TBL3 slots on the VEX path) --
  # op1[C4]   -> vexp1   : FIELD CAP_RXB (R'X'B' bits 7:5), route by map -> vexp2[map-1]
  # vexp2     -> capture vvvv (CAP_VVVV) + L (CAP_VL), route by (pp,W) -> vexop
  # op1[C5]   -> vexp1c5 : 2-byte form (map=0F, W=0, X'=B'=1); FIELD R'(7), vvvv, L
  # vexop     -> set MNEM + FORM (a VEX_* form) + RFILE/MFILE + IMK; modrm runs next
  def _form_idx(self, name):
    return self.INSN_FORM.index(name)

  def vexp1_state(self, b):
    mp = b & 0x1f
    mi = {1: 0, 2: 1, 3: 2, 7: 3}.get(mp)   # VEX maps 1/2/3 (0F/0F38/0F3A) + 7 (APX USER_MSR imm)
    if mi is None:
      return [], 'FSM_HALT'                                  # undefined VEX map
    return [('FIELD', self.tailcap('TBL3'), 7, 5)], 'FSM_VEXP2 + %d*256' % mi

  def vexp2_state(self, mapidx, b):
    pp, W = b & 3, (b >> 7) & 1
    acts = [('FIELD', self.cap('REL'), 6, 3),                # CAP_VVVV = raw vvvv (inverted)
            ('FIELD', self.tailcap('CC'), 2, 2)]             # CAP_VL = L
    return acts, 'FSM_VEXOP + %d*256' % ((mapidx * 4 + pp) * 2 + W)

  def vexp1c5_state(self, b):
    pp = b & 3
    acts = [('FIELD', self.tailcap('TBL3'), 7, 7),           # CAP_RXB = R' only (X'=B'=1)
            ('FIELD', self.cap('REL'), 6, 3),                # CAP_VVVV
            ('FIELD', self.tailcap('CC'), 2, 2)]             # CAP_VL = L
    return acts, 'FSM_VEXOP + %d*256' % (pp * 2)             # map 0F (idx 0), W=0

  def _vexop_acts(self, e):
    # action list for one VEX/EVEX opcode cell -- normal (MNEM baked) or an
    # opcode-extension group (no MNEM; the C++ reads vexgrp[gid][/digit]).
    if len(e) == 6:
      mnem, form, rf, mf, last, imk = e
    else:
      mnem, form, rf, mf, last = e; imk = last           # non-group: last IS the imm kind
    if mnem == -1:                                          # group; last = gid, imk = imm kind
      # split the gid across arg0(lo5)|arg1(hi3) -- VEX/APX group ids now exceed 31
      acts = [('CONST', self.tailcap('FORM'), self._form_idx(form), 0),
              ('CONST', self.tailcap('GRP'), last & 0x1f, last >> 5)]
      if imk: acts.append(('CONST', self.tailcap('IMK'), imk, 0))
    else:
      acts = [('MNEM', mnem, 0, 0),
              ('CONST', self.tailcap('FORM'), self._form_idx(form), 0)]
      if imk: acts.append(('CONST', self.tailcap('IMK'), imk, 0))   # imk = CAP_IMK kind
    if rf: acts.append(('CONST', self.tailcap('RFILE'), rf, 0))
    if mf: acts.append(('CONST', self.tailcap('MFILE'), mf, 0))
    return acts

  def vexop_state(self, mapidx, pp, W, op):
    e = self._vex_tab.get((mapidx, pp, W, op))
    if e is None:
      return [], 'FSM_HALT'                                  # undefined opcode (rejected)
    return self._vexop_acts(e), 'FSM_HALT'

  # ----- XOP FSM states (AMD, 8F prefix; same 2-payload-byte layout as VEX C4) --
  # The C++ 8F interception (which disambiguates XOP from legacy POP via map>=8)
  # runs these: xopp1 (byte1: R'X'B', route by map 8/9/10) -> xopp2 (byte2: vvvv/L,
  # route by pp,W) -> xopop (opcode -> MNEM + form), then ModR/M + vex_finalize.
  def xopp1_state(self, b):
    mi = {8: 0, 9: 1, 10: 2}.get(b & 0x1f)
    if mi is None:
      return [], 'FSM_HALT'
    return [('FIELD', self.tailcap('TBL3'), 7, 5)], 'FSM_XOPP2 + %d*256' % mi

  def xopp2_state(self, mi, b):
    pp, W = b & 3, (b >> 7) & 1
    acts = [('FIELD', self.cap('REL'), 6, 3), ('FIELD', self.tailcap('CC'), 2, 2)]
    return acts, 'FSM_XOPOP + %d*256' % ((mi * 4 + pp) * 2 + W)

  def xopop_state(self, mi, pp, W, op):
    e = self._xop_tab.get((mi, pp, W, op))
    if e is None:
      return [], 'FSM_HALT'
    return self._vexop_acts(e), 'FSM_HALT'

  # ----- EVEX FSM states (AVX-512; 3 payload bytes, capture-based) -------------
  # op1[0x62] -> evexp0 : FIELD CAP_RXB (P0 bits7:4 = R'X'B'R''), route by map
  # evexp1    -> FIELD CAP_VVVV (P1 vvvv), route by (pp,W)
  # evexp2    -> FIELD CAP_VL (the whole P2 byte: z L'L b V'' aaa), route to evexop
  # evexop    -> MNEM + FORM + files; the C++ evex_finalize reads P2 for mask/z/
  #              broadcast/rounding and applies the 32-register extension.
  def evexp0_state(self, b):
    mp = b & 0x07                                  # EVEX map (P0 bits 2:0)
    if self._apx_tab and mp == 4:                  # APX EVEX-promoted legacy (mmm=4)
      # capture P0[7:3] = R3 X3 B3 R4 B4 (5 bits, incl. the APX B4 extension) into CAP_RXB
      return [('FIELD', self.tailcap('TBL3'), 7, 3)], 'FSM_APXP1'
    mi = {1: 0, 2: 1, 3: 2, 5: 3, 6: 4}.get(mp)  # maps 5/6 = AVX512-FP16 MAP5/MAP6
    if mi is None:
      return [], 'FSM_HALT'                        # undefined map -> structural fallback
    return [('FIELD', self.tailcap('TBL3'), 7, 4)], 'FSM_EVEXP1 + %d*256' % mi

  def evexp1_state(self, mi, b):
    pp, W = b & 3, (b >> 7) & 1
    return [('FIELD', self.cap('REL'), 6, 3)], 'FSM_EVEXP2 + %d*256' % ((mi * 4 + pp) * 2 + W)

  def evexp2_state(self, mi, pp, W, b):
    return [('FIELD', self.tailcap('CC'), 7, 0)], 'FSM_EVEXOP + %d*256' % ((mi * 4 + pp) * 2 + W)

  def evexop_state(self, mi, pp, W, op):
    e = self._evex_tab.get((mi, pp, W, op))
    if e is None:
      return [], 'FSM_HALT'
    return self._vexop_acts(e), 'FSM_HALT'

  # ----- APX FSM states (EVEX-promoted legacy, map 4; capture-based) -----------
  # evexp0 (mmm=4) -> apxp1 : P1 byte = W vvvv X4 pp.  Capture vvvv+X4 (P1[6:2]) into
  #   CAP_VVVV and set VAR_OPSIZ from (pp,W); route by (pp,W) -> apxp2.
  # apxp2 -> capture the whole P2 byte (p2hi.ND.V4.aaa) into CAP_VL; route -> apxop.
  # apxop -> MNEM + (legacy) FORM + GPR files + IMK; the C++ apx_finalize folds the
  #   r0-31 extension, the operand sizing, and the ND (NDD dest) / NF modifiers.
  def apxp1_state(self, b):
    pp, W = b & 3, (b >> 7) & 1
    opsz = 2 if W else (1 if pp == 1 else 0)       # W (REX.W) -> 64 ; else pp=66 -> 16 ; else 32
    acts = [('FIELD', self.cap('REL'), 6, 2),      # CAP_VVVV = P1[6:2] = vvvv<<1 | X4
            ('CONST', self.var_id('opsiz'), opsz, 0)]
    return acts, 'FSM_APXP2 + %d*256' % (pp * 2 + W)

  def apxp2_state(self, slot, b):
    return [('FIELD', self.tailcap('CC'), 7, 0)], 'FSM_APXOP + %d*256' % slot   # CAP_VL = P2

  # map-4 (op,pp) where pp is a mandatory-prefix mnemonic SELECTOR, not the 16-bit
  # size override -- the GPR operand size is W?64:32 regardless of pp. apxp1's
  # pp-based opsize is wrong for these, so apxop overrides it. (Bijection-safe:
  # opsize only picks the render bank; the encoder replays raw bytes.)
  # selector-pp ops: GPR size W?64:32 regardless of pp (adcx/adox/wrss/wruss/movdiri/
  # the aadd atomics/crc32 dest). FIX64: the operand is always 64-bit (VMX invept/...,
  # the MSR ops, movdir64b's pointer).
  APX_SEL_OPSIZE = {(0x66, 0), (0x66, 1), (0x66, 2), (0x65, 1), (0xf9, 0),
                    (0xfc, 0), (0xfc, 1), (0xfc, 2), (0xfc, 3),
                    (0xf0, 0), (0xf1, 0), (0xf1, 1)}
  APX_FIX64 = {(0xf0, 2), (0xf1, 2), (0xf2, 2), (0xf8, 1), (0xf8, 2), (0xf8, 3)}
  APX_FIX32 = {(0xda, 2), (0xdb, 2)}                       # encodekey128/256: always 32-bit GPR

  def apxop_state(self, pp, W, op):
    e = self._apx_tab.get((0, pp, W, op))
    if e is None:
      return [], 'FSM_HALT'
    acts = self._vexop_acts(e)
    if (op, pp) in self.APX_SEL_OPSIZE:
      acts = acts + [('CONST', self.var_id('opsiz'), 2 if W else 0, 0)]   # 64/32, ignore pp
    elif (op, pp) in self.APX_FIX64:
      acts = acts + [('CONST', self.var_id('opsiz'), 2, 0)]               # always 64-bit
    elif (op, pp) in self.APX_FIX32:
      acts = acts + [('CONST', self.var_id('opsiz'), 0, 0)]               # always 32-bit
    # NDD applies only to the promoted ALU/cmov/shift/group ops + adcx/adox; the rest
    # of the map-4 tail (movbe/crc32/atomics/MSR/CET/VMX/SHA/AES/...) is not NDD, so
    # flag it (CAP_MNSEL) and the C++ apx_finalize won't prepend the vvvv dest.
    ndd_ok = (op < 0x50
              or op in (0x80, 0x81, 0x83, 0xc0, 0xc1, 0xd0, 0xd1, 0xd2, 0xd3,
                        0xf6, 0xf7, 0xfe, 0xff, 0xaf, 0x24, 0x2c, 0xa5, 0xad)
              or (op == 0x66 and pp in (1, 2)))
    if not ndd_ok:
      acts = acts + [('CONST', self.tailcap('MNSEL'), 1, 0)]   # non-NDD marker
    return acts, 'FSM_HALT'

  def _collect_group(self, groups, info, rhs, midx, K):
    tb = info['tb']
    opcode = info['bp'].lit_val
    pp = info['pp']                                      # None -> applies to every pp slot
    grp = groups.setdefault((tb, opcode), {})            # register the opcode as a group
    if info.get('fixmodrm') is not None:                 # fully-fixed ModR/M (endbr64/monitor/...)
      base = self.insn_mnem(rhs)[1]                       # -- usually no operand, but xabort (C6 F8
      imk = K['NONE']; form = self._form_idx('NONE')      # ib) / xbegin (C7 F8 rel) carry an imm/rel
      if info['imm']:
        nm = info['imm'][0]
        imk = K['IMM8SX'] if ('sx8(' in rhs and nm == 'imm8') else K[nm.upper()]
        form = self._form_idx('REL' if nm in ('rel8', 'relz') else 'IMM')
      self._fixmodrm.setdefault((tb, opcode), {})[(pp, info['fixmodrm'])] = (midx(base), form, imk)
      return
    reg = info['reg_fixed']
    m = grp.setdefault(
        (pp, reg), {'mnem_reg': None, 'mnem_mem': None, 'imk': K['NONE'], 'rfile': 0, 'mnsel': 0})
    mn = self.insn_mnem(rhs)                             # ('lit',name) or ('sel',tab,var)
    if mn[0] == 'sel':                                   # opsize-select mnem: rdssp[$opsiz] (d/q)
      seltab = self.tables[mn[1]]
      sel_base = midx(seltab[0])
      for k in range(1, len(seltab)):
        midx(seltab[k])                                  # keep d/w/q variants contiguous
      base = seltab[0]
    else:
      base = mn[1]                                        # leading-literal mnemonic
    if info['imm']:
      if 'sx8(' in rhs and info['imm'][0] == 'imm8':
        m['imk'] = K['IMM8SX']
      else:
        m['imk'] = K[info['imm'][0].upper()]
    if info['modrm'] == 'group_reg':
      if mn[0] == 'sel':
        m['mnem_reg'] = sel_base; m['mnsel'] = 1          # C++ adds opsize (rdsspd/rdsspq)
      else:
        m['mnem_reg'] = midx(base)
      # the reg-direct r/m draws from the rule's register table (rgb -> 8-bit GPR,
      # else the opsize GPR); carry it so the operand renders al/r8b, not eax.
      m['rfile'] = self.OPF.index(self.operand_file(rhs, 'r'))
    else:                                                # group_mem: fold in size suffix
      sm = re.search(r'sfx\[(\d+)\]', rhs)
      sfx = self.tables['sfx'][int(sm.group(1))] if sm else ''
      m['mnem_mem'] = midx(base + sfx)

  def group_state(self, gid, adrsiz, modrm):
    # one byte of a dedicated reg-fixed ModR/M stage: the reg field is the opcode
    # extension (it picks the mnemonic, baked here), the r/m is the operand.
    tb, opcode, members, fixmap = self._group_list[gid]
    fm = fixmap.get(modrm)
    if fm is not None:                                   # fully-fixed ModR/M op (FORM_NONE, or
      mnem, form, imk = fm                               # FORM_IMM/REL for xabort/xbegin)
      acts, nxt = self.modrm_state(adrsiz, modrm)        # consume the modrm byte (mod==11)
      pre = [('MNEM', mnem, 0, 0), ('CONST', self.tailcap('FORM'), form, 0)]
      if imk != 0:
        pre.append(('CONST', self.tailcap('IMK'), imk, 0))
      return pre + acts[1:], nxt
    m = members.get((modrm >> 3) & 7)
    if m is None:
      return [], 'FSM_HALT'                              # undefined extension: dead
    is_reg = (modrm >> 6) == 3
    mnem = m['mnem_reg'] if is_reg else m['mnem_mem']
    if mnem is None:
      return [], 'FSM_HALT'                              # this form not defined for /reg
    acts, nxt = self.modrm_state(adrsiz, modrm)
    pre = [('MNEM', mnem, 0, 0)]                         # reg field -> mnemonic (per byte)
    if is_reg and m.get('mnsel'):                        # opsize-select reg mnem (rdsspd/rdsspq)
      pre.append(('CONST', self.tailcap('MNSEL'), 1, 0))
    if m['imk'] != 0:
      pre.append(('CONST', self.tailcap('IMK'), m['imk'], 0))
    if is_reg and m.get('rfile'):                        # reg-direct r/m register file (RGB etc.)
      pre.append(('CONST', self.tailcap('MFILE'), m['rfile'], 0))
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
      # base/index sentinels GREG_NONE/GREG_RIP exceed 31 now (APX r16-31 use the
      # full 0..31 range), so split the value across arg0 (lo5) | arg1 (hi3).
      av = interp.greg_const(a0)
      return "{%s,%s,(%s)&0x1F,((%s)>>5)&7}" % (ACTOP[op], dn, av, av)
    elif dst == interp.cap('MODE'):
      av = 'RM_REG' if a0 == 1 else 'RM_MEM'
      return "{%s,%s,%s,%d}" % (ACTOP[op], dn, av, a1)
    # prefix value / seg row / form id / group id: a numeric 8-bit value. Form and
    # group ids now exceed 31, so split it across arg0 (lo5) | arg1 (hi3); the value
    # may already be pre-split by the caller, so recombine first. ACT_CONST rejoins.
    val = a0 | (a1 << 5)
    return "{%s,%s,(%d)&0x1F,((%d)>>5)&7}" % (ACTOP[op], dn, val, val)

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
  w("    NCAPS = CAP_RMREQ + 1              // == 32, the dst:5 ceiling (see Action.dst)\n};\n")
  # VEX field captures: aliased onto slots no VEX decode path uses for their legacy
  # purpose (no rel branch / condition code / 3-byte-escape on a VEX insn). This is
  # the capture reuse that lets VEX/EVEX fit the 32-slot budget without new slots.
  w("// ---- VEX field captures (reused slots; see corpus64.p) ----\n")
  w("#define CAP_VVVV CAP_REL    // VEX.vvvv (raw/inverted); finalize de-inverts\n")
  w("#define CAP_VL   CAP_CC     // VEX.L / EVEX.L'L : 0=xmm 1=ymm 2=zmm\n")
  w("#define CAP_RXB  CAP_TBL3   // C4: R'X'B' (bits7:5); C5: R' only (bit7)\n\n")

  # GPR / SREG enums (from greg / sreg tables) -- used only by the renderer.
  # 64-bit register banks repeat names across size halves; emit each name once.
  w("enum GPR {\n")
  seen = set()
  for i, n in enumerate(greg):
    if isinstance(n, str) and n.upper() not in seen:
      seen.add(n.upper())
      w("    GREG_%s = %d,\n" % (n.upper(), i))
  w("    GREG_NONE = %d,\n" % GREG_NONE)
  if mode == 64:
    w("    GREG_RIP = %d,   // RIP-relative base marker ([rip+disp32])\n" % GREG_RIP)
  w("    GREG_LAST_\n};\n\n")

  w("enum SREG {\n")
  for i, n in enumerate(interp.tables.get('sreg', [])):
    if isinstance(n, str):
      w("    SREG_%s = %d,\n" % (n.upper(), i))
  w("    SREG_NONE = %d\n};\n\n" % SREG_NONE)

  # CAP_MODE marker: register-direct vs memory r/m
  w("enum RmMode { RM_MEM = 0, RM_REG = 1 };\n\n")
  # operand-shape + immediate-kind enums for the single-byte instruction decoder
  w("enum InsnForm { %s };\n" %
    ", ".join("FORM_%s%s" % (n, "=0" if i == 0 else "") for i, n in enumerate(interp.INSN_FORM)))
  w("enum ImmKind  { IMK_NONE=0, IMK_IMM8, IMK_IMM16, IMK_IMM32, IMK_IMMZ, IMK_REL8, IMK_RELZ, IMK_PTR, IMK_IMM8SX, IMK_ENTER, IMK_IMMV, IMK_IMM8X2 };\n")
  w("enum OperandFile { OPF_GREG=0, OPF_RGB, OPF_XMM, OPF_MM, OPF_SREG, OPF_SSE_OS, OPF_MMG, OPF_GREGd, OPF_GREGq, OPF_GREGw, OPF_BND };\n\n")

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
  w("    uint32_t next;                     // table base index into FSM, or FSM_HALT\n")
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
  # VEX prefix-consuming stages + per-(map,pp,W) opcode maps (capture-based decode)
  w("    struct DState vexp1[256];          // C4 byte1: FIELD R'X'B', route by map -> vexp2\n")
  w("    struct DState vexp2[4][256];       // [mapidx] C4 byte2: FIELD vvvv/L, route by pp,W (mapidx 0..3 = maps 1/2/3/7)\n")
  w("    struct DState vexp1c5[256];        // C5 single byte: 2-byte VEX (map=0F, W=0)\n")
  w("    struct DState vexop[4][4][2][256]; // [mapidx][pp][W] opcode -> MNEM + VEX form + files\n")
  # EVEX prefix stages (3 payload bytes) + per-(map,pp,W) opcode maps (AVX-512)
  w("    struct DState evexp0[256];         // 62 P0: FIELD R'X'B'R'', route by map\n")
  w("    struct DState evexp1[5][256];      // [mapidx] P1: FIELD vvvv, route by pp,W\n")
  w("    struct DState evexp2[5][4][2][256];// [mapidx][pp][W] P2: FIELD whole byte (z.L'L.b.V''.aaa)\n")
  w("    struct DState evexop[5][4][2][256];// [mapidx][pp][W] opcode -> MNEM + VEX form + files (mapidx 0..4 = maps 1/2/3/5/6)\n")
  # XOP prefix stages (8F; maps 8/9/10) -- same shape as VEX C4
  w("    struct DState xopp1[256];          // 8F byte1: FIELD R'X'B', route by map 8/9/10\n")
  w("    struct DState xopp2[3][256];       // [map-8] byte2: FIELD vvvv/L, route by pp,W\n")
  w("    struct DState xopop[3][4][2][256]; // [map-8][pp][W] opcode -> MNEM + VEX form + files\n")
  # APX EVEX-promoted legacy (62, map 4): one P1/P2 stage pair + per-(pp,W) opcode map
  w("    struct DState apxp1[256];          // P1: FIELD vvvv/X4, set opsize, route by pp,W\n")
  w("    struct DState apxp2[8][256];       // [pp*2+W] P2: FIELD whole byte (p2hi.ND.V4.aaa)\n")
  w("    struct DState apxop[8][256];       // [pp*2+W] opcode -> MNEM + legacy form + GPR files\n")
  w("};\n")
  w("// base indices derived from the layout (single source of truth):\n")
  w("#define FSM_INDEX(member) ((uint32_t)(offsetof(struct Fsm, member) / sizeof(struct DState)))\n")
  w("#define FSM_PREFIX  FSM_INDEX(prefix)\n")
  w("#define FSM_OP1     FSM_INDEX(op1)\n")
  w("#define FSM_OP2     FSM_INDEX(op2)\n")
  w("#define FSM_OP3_38  FSM_INDEX(op3_38)\n")
  w("#define FSM_OP3_3A  FSM_INDEX(op3_3a)\n")
  w("#define FSM_MODRM   FSM_INDEX(modrm)       // + adrsiz*256\n")
  w("#define FSM_SIB     FSM_INDEX(sib)         // + mod*256\n")
  w("#define FSM_GROUPS  FSM_INDEX(groups)      // + (gid*2 + adrsiz)*256\n")
  w("#define FSM_VEXP1   FSM_INDEX(vexp1)\n")
  w("#define FSM_VEXP2   FSM_INDEX(vexp2)        // + (map-1)*256\n")
  w("#define FSM_VEXP1C5 FSM_INDEX(vexp1c5)\n")
  w("#define FSM_VEXOP   FSM_INDEX(vexop)        // + ((map-1)*4 + pp)*2 + W) *256\n")
  w("#define FSM_EVEXP0  FSM_INDEX(evexp0)\n")
  w("#define FSM_EVEXP1  FSM_INDEX(evexp1)       // + mapidx*256           (mapidx 0..4 = maps 1/2/3/5/6)\n")
  w("#define FSM_EVEXP2  FSM_INDEX(evexp2)       // + (mapidx*4 + pp)*2 + W) *256\n")
  w("#define FSM_EVEXOP  FSM_INDEX(evexop)       // + (mapidx*4 + pp)*2 + W) *256\n")
  w("#define FSM_XOPP1   FSM_INDEX(xopp1)\n")
  w("#define FSM_XOPP2   FSM_INDEX(xopp2)        // + (map-8)*256\n")
  w("#define FSM_XOPOP   FSM_INDEX(xopop)        // + ((map-8)*4 + pp)*2 + W) *256\n")
  w("#define FSM_APXP1   FSM_INDEX(apxp1)\n")
  w("#define FSM_APXP2   FSM_INDEX(apxp2)        // + (pp*2 + W)*256\n")
  w("#define FSM_APXOP   FSM_INDEX(apxop)        // + (pp*2 + W)*256\n")
  w("#define FSM_NGROUP  %d\n" % interp.group_count())
  w("#define FSM_COUNT   (sizeof(struct Fsm) / sizeof(struct DState))\n")
  w("#define FSM_HALT    ((uint32_t)0xFFFFFFFF) // `next` sentinel: stop\n")
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
      tb, opcode, _, _ = interp._group_list[gid]
      w("   { // group id %d  (%sopcode 0x%02x)\n" % (gid, "0F " if tb else "", opcode))
      for adr in (0, 1):
        w("    { // adrsiz=%d\n" % adr)
        emit_states(lambda i, g=gid, a=adr: interp.group_state(g, a, i))
        w("    }%s\n" % ("," if adr == 0 else ""))
      w("   }%s\n" % ("," if gid < ng - 1 else ""))
    w("  }")
  else:
    w("")
  # VEX prefix + opcode maps (always emitted; in 32-bit mode they are unreachable)
  w(",\n  { // vexp1[256]  (C4 byte1)\n")
  emit_states(interp.vexp1_state)
  w("  },\n")
  w("  { // vexp2[4][256]  (C4 byte2, by mapidx 0..3 = maps 1/2/3/7)\n")
  for mp in range(4):
    w("   { // map %d\n" % [1, 2, 3, 7][mp])
    emit_states(lambda i, m=mp: interp.vexp2_state(m, i))
    w("   }%s\n" % ("," if mp < 3 else ""))
  w("  },\n")
  w("  { // vexp1c5[256]  (C5 single payload byte)\n")
  emit_states(interp.vexp1c5_state)
  w("  },\n")
  w("  { // vexop[4][4][2][256]  ([mapidx][pp][W] -> opcode)\n")
  for mp in range(4):
    w("   { // map %d\n" % [1, 2, 3, 7][mp])
    for pp in range(4):
      w("    { // pp %d\n" % pp)
      for W in range(2):
        w("     { // W %d\n" % W)
        emit_states(lambda i, m=mp, p=pp, ww=W: interp.vexop_state(m, p, ww, i))
        w("     }%s\n" % ("," if W == 0 else ""))
      w("    }%s\n" % ("," if pp < 3 else ""))
    w("   }%s\n" % ("," if mp < 3 else ""))
  w("  },\n")
  # ---- EVEX stages ----
  w("  { // evexp0[256]  (62 P0)\n")
  emit_states(interp.evexp0_state)
  w("  },\n")
  w("  { // evexp1[5][256]  (P1, by mapidx 0..4 = maps 1/2/3/5/6)\n")
  for mp in range(5):
    w("   { // map %d\n" % [1, 2, 3, 5, 6][mp])
    emit_states(lambda i, m=mp: interp.evexp1_state(m, i))
    w("   }%s\n" % ("," if mp < 4 else ""))
  w("  },\n")
  w("  { // evexp2[5][4][2][256]  (P2, by mapidx,pp,W)\n")
  for mp in range(5):
    w("   { // map %d\n" % [1, 2, 3, 5, 6][mp])
    for pp in range(4):
      w("    { // pp %d\n" % pp)
      for W in range(2):
        w("     { // W %d\n" % W)
        emit_states(lambda i, m=mp, p=pp, ww=W: interp.evexp2_state(m, p, ww, i))
        w("     }%s\n" % ("," if W == 0 else ""))
      w("    }%s\n" % ("," if pp < 3 else ""))
    w("   }%s\n" % ("," if mp < 4 else ""))
  w("  },\n")
  w("  { // evexop[5][4][2][256]  ([mapidx][pp][W] -> opcode)\n")
  for mp in range(5):
    w("   { // map %d\n" % [1, 2, 3, 5, 6][mp])
    for pp in range(4):
      w("    { // pp %d\n" % pp)
      for W in range(2):
        w("     { // W %d\n" % W)
        emit_states(lambda i, m=mp, p=pp, ww=W: interp.evexop_state(m, p, ww, i))
        w("     }%s\n" % ("," if W == 0 else ""))
      w("    }%s\n" % ("," if pp < 3 else ""))
    w("   }%s\n" % ("," if mp < 4 else ""))
  w("  },\n")
  # ---- XOP stages ----
  w("  { // xopp1[256]  (8F byte1)\n")
  emit_states(interp.xopp1_state)
  w("  },\n")
  w("  { // xopp2[3][256]  (byte2, by map 8/9/10)\n")
  for mp in range(3):
    w("   { // map %d\n" % (mp + 8))
    emit_states(lambda i, m=mp: interp.xopp2_state(m, i))
    w("   }%s\n" % ("," if mp < 2 else ""))
  w("  },\n")
  w("  { // xopop[3][4][2][256]  ([map-8][pp][W] -> opcode)\n")
  for mp in range(3):
    w("   { // map %d\n" % (mp + 8))
    for pp in range(4):
      w("    { // pp %d\n" % pp)
      for W in range(2):
        w("     { // W %d\n" % W)
        emit_states(lambda i, m=mp, p=pp, ww=W: interp.xopop_state(m, p, ww, i))
        w("     }%s\n" % ("," if W == 0 else ""))
      w("    }%s\n" % ("," if pp < 3 else ""))
    w("   }%s\n" % ("," if mp < 2 else ""))
  w("  },\n")
  # ---- APX stages (EVEX-promoted legacy, map 4) ----
  w("  { // apxp1[256]  (P1 byte)\n")
  emit_states(interp.apxp1_state)
  w("  },\n")
  w("  { // apxp2[8][256]  ([pp*2+W] P2 byte)\n")
  for slot in range(8):
    w("   { // pp*2+W = %d\n" % slot)
    emit_states(lambda i, s=slot: interp.apxp2_state(s, i))
    w("   }%s\n" % ("," if slot < 7 else ""))
  w("  },\n")
  w("  { // apxop[8][256]  ([pp*2+W] -> opcode)\n")
  for slot in range(8):
    pp, W = slot // 2, slot % 2
    w("   { // pp=%d W=%d\n" % (pp, W))
    emit_states(lambda i, p=pp, ww=W: interp.apxop_state(p, ww, i))
    w("   }%s\n" % ("," if slot < 7 else ""))
  w("  }\n")
  w("};\n")
  w("static_assert(sizeof(struct Fsm) == (256 + 256 + 256 + 256 + 256 + 2*256 + 3*256 + %d*2*256\n"
    % max(1, ng))
  w("              + 256 + 4*256 + 256 + 4*4*2*256\n")
  w("              + 256 + 5*256 + 5*4*2*256 + 5*4*2*256\n")
  w("              + 256 + 3*256 + 3*4*2*256\n")
  w("              + 256 + 8*256 + 8*256) * sizeof(struct DState),\n")
  w('              "struct Fsm has padding; flat indexing would be wrong");\n')
  # two distinct size limits, the narrower one (group id) binds first:
  #   * group id rides a CONST action -> arg0(5) | arg1(3) = 8 bits  (<= 255 groups)
  #   * DState.next indexes the flat Fsm in DStates -> uint32_t       (effectively unbounded)
  w('static_assert(FSM_NGROUP <= 255, "group id no longer fits the 8-bit CONST action");\n')
  w("static_assert(NCAPS <= 32, \"capture index no longer fits Action.dst (5 bits)\");\n")
  w("static_assert(sizeof(struct Fsm) / sizeof(struct DState) < 0xFFFFFFFFu,\n")
  w('              "Fsm too large: DState.next (uint32_t) can no longer index every state");\n\n')

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
  # 0x90-0x97 (xchg rAX,r / nop): the 10010-bbb rule decodes the whole range and supplies
  # the encode candidates, but renders only the embedded register. decode_insn rebuilds the
  # two-operand xchg struct and maps the no-extension 0x90 (register == rAX) back to nop --
  # it needs these mnemonic indices.
  for _m in ('xchg', 'nop'):
    w("#define MNEM_%s %d\n" % (_m.upper(), interp._mnem.index(_m) if _m in interp._mnem else 0xFFFF))
  # APX CCMP/CTEST (EVEX map 4): every map-4 CMP (38-3D,80/81/83 /7) and TEST (84/85,
  # F6/F7 /0) is the *conditional* form. apx_finalize rewrites the corpus "cmp"/"test"
  # to ccmp<cc>/ctest<cc> by the source condition code (P2[3:0]). Append the 16 cc
  # variants of each, contiguous & in SCC order, so the C++ indexes them as BASE+scc.
  _SCC = ['o','no','b','nb','z','nz','be','nbe','s','ns','t','f','l','nl','le','nle']
  w("#define MNEM_CMP %d\n"  % (interp._mnem.index('cmp')  if 'cmp'  in interp._mnem else 0xFFFF))
  w("#define MNEM_TEST %d\n" % (interp._mnem.index('test') if 'test' in interp._mnem else 0xFFFF))
  w("#define MNEM_CCMP_BASE %d\n" % len(interp._mnem))
  for _c in _SCC: interp._mnem.append('ccmp' + _c)
  w("#define MNEM_CTEST_BASE %d\n" % len(interp._mnem))
  for _c in _SCC: interp._mnem.append('ctest' + _c)
  # APX f8 pp2/pp3 swaps the mnemonic by mod (reg=uwrmsr/urdmsr, mem=enqcmds/enqcmd);
  # the C++ apx_finalize picks the memory variant by these indices.
  for _m in ('uwrmsr', 'urdmsr', 'enqcmds', 'enqcmd'):
    w("#define MNEM_%s %d\n" % (_m.upper(), interp._mnem.index(_m) if _m in interp._mnem else 0xFFFF))
  # VEX 0F 12/16 NP reg-vs-mem split: the mem form is vmovlps/vmovhps (a load), the
  # reg-direct form is vmovhlps/vmovlhps. The VEX descriptor is per-opcode (no ModR/M
  # branch), so vex_finalize swaps the mnemonic once reg-vs-mem is known.
  for _m in ('vmovlps', 'vmovhlps', 'vmovhps', 'vmovlhps'):
    w("#define MNEM_%s %d\n" % (_m.upper(), interp._mnem.index(_m) if _m in interp._mnem else 0xFFFF))
  # AMX: NP.0F38.W0 49 shares its opcode -- the mem form is ldtilecfg, the reg-direct C0
  # form is tilerelease (no operands). The VEX descriptor is per-opcode, so vex_finalize
  # swaps the mnemonic (and drops the operand) once the reg-direct form is seen.
  for _m in ('ldtilecfg', 'tilerelease'):
    w("#define MNEM_%s %d\n" % (_m.upper(), interp._mnem.index(_m) if _m in interp._mnem else 0xFFFF))
  # a mnemonic may carry a "~tag" disambiguator: distinct index (so per-opcode
  # encodings each get their own enc-rank bucket) but a shared display string --
  # e.g. the reserved multi-byte NOPs 0F 18-1E all render "nop" yet round-trip to
  # their own opcode byte. Strip the tag for the emitted name.
  w("static const char* const mnem_tab[] = {\n    %s\n};\n" %
    (", ".join('"%s"' % m.split('~')[0] for m in interp._mnem) if interp._mnem else '""'))
  w("static const size_t mnem_tab_size = sizeof(mnem_tab) / sizeof(mnem_tab[0]);\n")
  # LOCK (F0) is architecturally legal only on these RMW mnemonics AND only with a memory
  # destination (Intel SDM Vol.2, LOCK). decode_insn rejects LOCK on anything else (lock mov,
  # lock shift, lock on a register destination, ...) as #UD. One flag per mnemonic index.
  _LOCKABLE = {'add', 'adc', 'and', 'or', 'xor', 'sub', 'sbb', 'inc', 'dec', 'neg', 'not',
               'xchg', 'xadd', 'btc', 'btr', 'bts', 'cmpxchg', 'cmpxchg8b', 'cmpxchg16b'}
  def _lockbase(m):                                    # strip ~tag, trailing space, size suffix
    b = m.split('~')[0].rstrip()                       # (group mem forms render "inc.b"/"bts.d"/..)
    return b[:-2] if (len(b) >= 2 and b[-2] == '.' and b[-1] in 'bwdq') else b
  w("static const unsigned char mnem_lockable[] = {\n    %s\n};\n\n" %
    (", ".join('1' if _lockbase(m) in _LOCKABLE else '0' for m in interp._mnem)
     if interp._mnem else '0'))

  # VEX/EVEX opcode-extension groups (shifts 71/72/73): vexgrp[gid][/digit] is the
  # mnemonic (-1 = undefined digit). A VEX_VMG opcode state stores the gid in
  # CAP_GRP; the C++ picks the mnemonic by the ModR/M reg digit at finalize.
  ng_vex = max(1, len(interp._vexgrp))
  w("#define VEX_NGRP %d\n" % len(interp._vexgrp))
  w("static const int16_t vexgrp[%d][8] = {\n" % ng_vex)
  if interp._vexgrp:
    for row in interp._vexgrp:
      w("    {%s},\n" % ", ".join(str(x) for x in row))
  else:
    w("    {-1,-1,-1,-1,-1,-1,-1,-1}\n")
  w("};\n\n")

  # Encode-side prefix-tail structure, generated from the compiled VEX/EVEX/XOP/APX
  # cells (replaces the hardcoded vex_structure switch in x86enc.hpp). Each byte packs
  # has_modrm (bit 7) and an immediate field in bits 0-6: 0/1/2/4 = that many replayed
  # bytes, 15 = immz (opsize-dependent, resolved by the C++). Index: 0/1/2 = map 1/2/3
  # (VEX and EVEX share tail structure), 3/4/5 = XOP map 8/9/10, 6 = APX map 4.
  #
  # vex_structure is queried for BOTH covered opcodes (byte-replay must match what the
  # FSM consumed) and opcodes with no corpus rule (the structural fallback decodes+
  # re-encodes them through the same function, so the split just has to be self-
  # consistent). So the table is SEEDED with the per-map default the structural path
  # needs, then OVERRIDDEN with the cell-exact (has_modrm, imm) for every covered opcode.
  _HM = 0x80
  _imf = {0: 0, 1: 1, 2: 2, 3: 4, 4: 15, 8: 1}       # CAP_IMK -> replay width (15 = immz)
  vextail = [[_HM | 0] * 256 for _ in range(7)]
  for op in (0x70, 0x71, 0x72, 0x73, 0xC2, 0xC4, 0xC5, 0xC6):  # map1 imm8 opcodes
    vextail[0][op] = _HM | 1
  vextail[0][0x77] = 0                                 # vzeroupper/vzeroall: no ModR/M
  vextail[2] = [_HM | 1] * 256                         # map3 (0F3A): imm8 map
  vextail[3] = [_HM | 1] * 256                         # XOP map 8: is4 selector (imm8)
  vextail[5] = [_HM | 4] * 256                         # XOP map 10: bextr/lwp imm32
  for op in (0x80, 0x82, 0x83, 0xC0, 0xC1, 0x24, 0x2C, 0xD4, 0x6A, 0x6B):  # APX imm8
    vextail[6][op] = _HM | 1
  for op in (0x81, 0x69):                              # APX group1/imul immz
    vextail[6][op] = _HM | 15
  _seen = {}
  def _seed_tail(tab, base):
    for (m, pp, W, op), e in tab.items():
      form = e[1]; imk = e[5] if len(e) == 6 else e[4]
      val = (0 if form == 'VEX_NONE' else _HM) | _imf.get(imk, 0)
      cur = base + m
      if (cur, op) in _seen and _seen[(cur, op)] != val:
        sys.stderr.write("gen.py: vextail conflict at tail-map %d opcode 0x%02x: %d vs %d\n"
                         % (cur, op, _seen[(cur, op)], val))
      _seen[(cur, op)] = val
      vextail[cur][op] = val
  _seed_tail(interp._vex_tab, 0)
  _seed_tail(interp._evex_tab, 0)
  _seed_tail(interp._xop_tab, 3)
  _seed_tail(interp._apx_tab, 6)
  w("// encode-side prefix tail: has_modrm<<7 | imm-field (0/1/2/4 bytes, 15=immz).\n")
  w("// tail-map: 0/1/2 = map 1/2/3, 3/4/5 = XOP 8/9/10, 6 = APX map 4. Generated.\n")
  w("static const uint8_t vextail[7][256] = {\n")
  for i in range(7):
    w("    {%s},\n" % ",".join(str(x) for x in vextail[i]))
  w("};\n\n")

  # Legacy-SSE mandatory-prefix mnemonic table: ppvtab[CAP_PPSEL-1][pp] is the
  # mnemonic for prefix pp (0=NP, 1=66, 2=F3, 3=F2). Invalid prefix slots repeat
  # the NP mnemonic (lenient collapse; objdump rejects them). The C++ derives pp
  # from VAR_REPTYPE/VAR_OPSIZ at finalize and overwrites CAP_MNEM.
  npp = max(1, len(interp._ppvtab))
  w("#define PP_NVTAB %d\n" % len(interp._ppvtab))
  w("static const uint16_t ppvtab[%d][4] = {\n" % npp)
  if interp._ppvtab:
    for vt in interp._ppvtab:
      w("    {%s},\n" % ", ".join(str(x) for x in vt))
  else:
    w("    {0,0,0,0}\n")
  w("};\n\n")

  # Per-prefix full descriptor: for opcodes whose mnemonic AND operand form change
  # with the mandatory prefix, ppdesc[CAP_GRP][pp] gives the whole descriptor for
  # prefix pp. mnem==0xFFFF marks an illegal prefix (#UD). The C++ reads this after
  # the prefix run (MNSEL mode 3) and overrides mnem/form/dir/rfile/mfile/imk/rmreq.
  ppd = getattr(interp, '_ppdesc', [])
  w("struct PpDesc { uint16_t mnem, mreg; uint8_t form, dir, rfile, mfile, imk, rmreq, rform, rmf, dir_reg; };\n")
  w("#define PPDESC_N %d\n" % len(ppd))
  w("static const struct PpDesc ppdesc[%d][4] = {\n" % max(1, len(ppd)))
  ud = "{0xFFFF,0xFFFF,0,0,0,0,0,0,0xFF,0,0}"
  if ppd:
    for entry in ppd:
      cells = []
      for slot in entry:
        if slot is None:
          cells.append(ud)
        else:
          mnem, form_, dir_, rf, mf, imk_, rmreq, mreg, rform, rmf, dirreg = slot
          cells.append("{%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}" %
                       (mnem, mreg, form_, dir_, rf, mf, imk_, rmreq, rform, rmf, dirreg))
      w("    {%s},\n" % ", ".join(cells))
  else:
    w("    {%s,%s,%s,%s}\n" % (ud, ud, ud, ud))
  w("};\n\n")

  # pp-variant groups: ppgroup[gid]==1 marks the base gid of a group whose ModR/M
  # dispatch depends on the mandatory prefix (0F AE, 0F 1E). Such a group occupies 4
  # consecutive gids (pp 0=NP,1=66,2=F3,3=F2); the C++ adds pp to the baked base gid
  # before running the group ModR/M stage. All-zero for a corpus with no pp-groups.
  ppg = getattr(interp, '_ppgroup', [])
  w("static const uint8_t ppgroup[%d] = {" % max(1, len(ppg)))
  w("%s};\n\n" % ", ".join(str(x) for x in (ppg if ppg else [0])))

  # 3DNow!: the mnemonic is a trailing opcode byte after ModR/M (0F 0F ... suffix).
  # tdnow_tab[suffix] is the mnemonic (0xFFFF = undefined suffix -> #UD).
  tdn = getattr(interp, '_tdnow', {})
  w("static const uint16_t tdnow_tab[256] = {\n    ")
  w(", ".join(str(tdn.get(i, 0xFFFF)) for i in range(256)))
  w("\n};\n\n")

  w("#endif // X86_TABLES_H\n")
  open(out_path, 'w', encoding='utf-8').write(''.join(o))

  # FSM size report: DState.next is uint16_t, so the flat table must stay under
  # 65536 states. Each opcode/group sub-table is 256 states; track the trend.
  ndstate = (256 + 256 + 256 + 256 + 256 + 2 * 256 + 3 * 256 + max(1, ng) * 2 * 256
             + 256 + 3 * 256 + 256 + 3 * 4 * 2 * 256
             + 256 + 3 * 256 + 3 * 4 * 2 * 256 + 3 * 4 * 2 * 256
             + 256 + 3 * 256 + 3 * 4 * 2 * 256
             + 256 + 8 * 256 + 8 * 256)
  import sys as _sys
  _sys.stderr.write(
      "FSM: %d DStates (uint32_t next, no state ceiling), %d groups, sizeof(Fsm)=%d KiB\n"
      % (ndstate, ng, ndstate * 20 // 1024))


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
  global GREG_NONE, GREG_RIP
  args = [a for a in sys.argv[1:] if not a.startswith('--')]
  corpus = args[0] if args else "corpus.p"
  outp = args[1] if len(args) > 1 else "x86_tables.h"
  c = parse_corpus(corpus)
  # x86-64 register numbers run 0..31 with APX (REX2 -> r16-r31), so the "no
  # register" sentinel must clear all of them: it sits just past the 96-entry
  # greg table (3 size banks x 32). x86-32 keeps the historical 15.
  GREG_NONE = 96 if int(c['arch'].get('mode', 32)) == 64 else 15
  GREG_RIP = GREG_NONE + 1
  interp = Interp(c)
  if "--check" in sys.argv:
    self_check(interp)
  emit(c, interp, outp)


if __name__ == "__main__":
  main()
