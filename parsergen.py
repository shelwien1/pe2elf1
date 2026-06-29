#!/usr/bin/env python3
# parsergen.py -- a parser generator for the corpus.p bit-pattern DSL.
#
# Reads a *.p description (the markdown's "language reference") and turns it
# into a working x86 disassembler: it generates one matcher per `submatch`,
# threads the bidirectional fields / prefix-dispatch <...> / actions exactly as
# the spec describes, renders the templates, and applies the automatic
# prefix-token recovery of section 6.4.
#
#   usage:  parsergen.py DESC.p IMAGE.bin   > listing.asm
#           parsergen.py --dump DESC.p      (print the parsed grammar)
#
# The engine knows no x86; every architectural fact lives in the description.
# The single ISA-specific concession is the section-6.4 recovery table, which
# the spec itself defines as built-in engine behaviour (see RECOVER_VARS).

import sys

# ---- sentinels --------------------------------------------------------------
UNBOUND = ("UNBOUND",)            # a free (not-yet-captured) bidirectional field
class _None:
  __slots__ = ()
  def __repr__(self): return "none"
NONE = _None()                    # the language `none` sentinel

# =============================================================================
# 1.  PARSER  -- source text  ->  grammar AST
# =============================================================================
class Submatch:
  def __init__(self, name, params):
    self.name = name
    self.params = params          # list of param names (without '$')
    self.rules = []               # list of Rule
    self.dispatch_param = None    # first param matched by <...> (no stream field)
    self.dispatch_params = []     # all such params, in declaration order
    self.field_letters = set()
    self.has_prefix_frames = False

class Rule:
  def __init__(self, pattern, template):
    self.pattern = pattern        # list of pattern elements
    self.template = template      # list of expression terms (juxtaposed)
    self.is_prefix_frame = False
    self.ptoken = None            # the prepended token string (recovery)
    self.pvar = None              # the var its action sets (recovery)

# pattern elements:
#   ('hex', value)            8 literal bits
#   ('lit', bit)              1 literal bit
#   ('field', letter, nbits)  captured field
#   ('disp', [0/1/None,...])  <...> prefix-dispatch match (None = '?')
#   ('sub', name, args|None)  @name / @name(args)  ; args = list of exprs
#   ('guard', expr)           [expr]
#   ('act', [(var, expr),..]) { $v = expr ; .. }
#
# expression AST:
#   ('num', int) ('str', s) ('none',) ('var', name)
#   ('table', name, idx) ('call', name, [args])
#   ('binop', op, l, r) ('neg', e) ('ternary', c, a, b)

class Parser:
  def __init__(self, text):
    self.s = text
    self.i = 0
    self.n = len(text)

  # -- low level ----------------------------------------------------------
  def ws(self):
    while self.i < self.n:
      c = self.s[self.i]
      if c in " \t\r\n":
        self.i += 1
      elif c == "#":
        while self.i < self.n and self.s[self.i] != "\n":
          self.i += 1
      else:
        break

  def eof(self):
    self.ws()
    return self.i >= self.n

  def peek(self):
    self.ws()
    return self.s[self.i] if self.i < self.n else ""

  def starts(self, t):
    self.ws()
    return self.s.startswith(t, self.i)

  def take(self, t):
    self.ws()
    if not self.s.startswith(t, self.i):
      raise SyntaxError("expected %r at %r" % (t, self.s[self.i:self.i + 24]))
    self.i += len(t)

  def ident(self):
    self.ws()
    j = self.i
    if j < self.n and (self.s[j].isalpha() or self.s[j] == "_"):
      j += 1
      while j < self.n and (self.s[j].isalnum() or self.s[j] == "_"):
        j += 1
    name = self.s[self.i:j]
    self.i = j
    return name

  def var(self):
    self.take("$")
    j = self.i
    while j < self.n and (self.s[j].isalnum() or self.s[j] == "_"):
      j += 1
    name = self.s[self.i:j]
    self.i = j
    return name

  def number(self):
    self.ws()
    j = self.i
    if self.s.startswith("0x", j) or self.s.startswith("0X", j):
      j += 2
      while j < self.n and self.s[j] in "0123456789abcdefABCDEF":
        j += 1
      val = int(self.s[self.i:j], 16)
    else:
      while j < self.n and self.s[j].isdigit():
        j += 1
      val = int(self.s[self.i:j])
    self.i = j
    return val

  def string(self):
    self.take('"')
    out = []
    while self.i < self.n and self.s[self.i] != '"':
      out.append(self.s[self.i])
      self.i += 1
    self.take('"')
    return "".join(out)

  # -- top level ----------------------------------------------------------
  def parse(self):
    g = {"arch": {}, "vars": {}, "tables": {}, "submatches": {}}
    while not self.eof():
      kw = self.ident()
      if kw == "arch":
        self.parse_kv(g["arch"])
      elif kw == "vars":
        self.parse_kv(g["vars"])
      elif kw == "table":
        name, vals = self.parse_table()
        g["tables"][name] = vals
      elif kw == "submatch":
        sm = self.parse_submatch()
        g["submatches"][sm.name] = sm
      else:
        raise SyntaxError("unknown top-level %r" % kw)
    postprocess(g)
    return g

  def parse_kv(self, d):
    # $name=value pairs, value = number or bare word
    while self.peek() == "$":
      name = self.var()
      self.take("=")
      self.ws()
      c = self.s[self.i]
      if c.isdigit():
        d[name] = self.number()
      else:
        d[name] = self.ident()     # le / be / msb / lsb

  def parse_table(self):
    name = self.ident()
    self.take("{")
    vals = []
    while self.peek() != "}":
      c = self.peek()
      if c == '"':
        vals.append(self.string())
      elif c.isdigit() or (c == "0"):
        vals.append(self.number())
      else:
        # bare token -> string of its characters
        j = self.i
        while j < self.n and self.s[j] not in ", \t\r\n}":
          j += 1
        vals.append(self.s[self.i:j])
        self.i = j
      if self.peek() == ",":
        self.take(",")
    self.take("}")
    return name, vals

  def parse_submatch(self):
    name = self.ident()
    params = []
    if self.peek() == "(":
      self.take("(")
      while self.peek() != ")":
        params.append(self.var())
        if self.peek() == ",":
          self.take(",")
      self.take(")")
    sm = Submatch(name, params)
    self.take("{")
    while self.peek() != "}":
      pat = self.parse_pattern()
      self.take("=>")
      tmpl = self.parse_template()
      sm.rules.append(Rule(pat, tmpl))
      if self.peek() == ";":
        self.take(";")
    self.take("}")
    return sm

  # -- pattern ------------------------------------------------------------
  def parse_pattern(self):
    elems = []
    while True:
      self.ws()
      if self.starts("=>"):
        break
      c = self.s[self.i]
      if self.s.startswith("0x", self.i) or self.s.startswith("0X", self.i):
        j = self.i + 2
        while j < self.n and self.s[j] in "0123456789abcdefABCDEF":
          j += 1
        nhex = j - (self.i + 2)
        val = int(self.s[self.i:j], 16)
        self.i = j
        # spell as one element per byte (handles 0xNN; all uses are one byte)
        elems.append(("hex", val) if nhex == 2 else ("hexn", val, nhex * 4))
      elif c in "01":
        # run of literal bits (whitespace separates groups only)
        while True:
          self.ws()
          if self.i < self.n and self.s[self.i] in "01" \
             and not self.s.startswith("0x", self.i):
            elems.append(("lit", int(self.s[self.i])))
            self.i += 1
          else:
            break
      elif c.isalpha():
        letter = c
        cnt = 0
        while True:
          self.ws()
          if self.i < self.n and self.s[self.i] == letter:
            self.i += 1
            cnt += 1
          else:
            break
        elems.append(("field", letter, cnt))
      elif c == "<":
        self.i += 1
        specs = []
        while self.peek() != ">":
          ch = self.s[self.i]
          if ch in "01":
            specs.append(int(ch)); self.i += 1
          elif ch == "?":
            specs.append(None); self.i += 1
          else:
            self.i += 1   # whitespace inside <...>
        self.take(">")
        elems.append(("disp", specs))
      elif c == "@":
        self.i += 1
        name = self.ident()
        args = None
        if self.peek() == "(":
          self.take("(")
          args = []
          while self.peek() != ")":
            args.append(self.parse_expr())
            if self.peek() == ",":
              self.take(",")
          self.take(")")
        elems.append(("sub", name, args))
      elif c == "[":
        self.take("[")
        e = self.parse_expr()
        self.take("]")
        elems.append(("guard", e))
      elif c == "{":
        self.take("{")
        asg = []
        while self.peek() != "}":
          v = self.var()
          self.take("=")
          rhs = self.parse_template()   # RHS is a template (juxtaposed terms)
          asg.append((v, rhs))
          if self.peek() == ";":
            self.take(";")
        self.take("}")
        elems.append(("act", asg))
      else:
        raise SyntaxError("bad pattern char %r at %r" %
                          (c, self.s[self.i:self.i + 24]))
    return elems

  # -- template & expressions --------------------------------------------
  def parse_template(self):
    terms = []
    while True:
      self.ws()
      if self.i >= self.n or self.s[self.i] in ";}":
        break
      terms.append(self.parse_expr())
    return terms

  def starter(self):
    # does an expression term start here?  (used to bound juxtaposition)
    self.ws()
    if self.i >= self.n:
      return False
    c = self.s[self.i]
    return c == '"' or c == "$" or c == "(" or c.isalnum()

  BINOPS = ["<<", ">>", "==", "!=", "<=", ">=", "<", ">", "+", "-", "*", "/"]
  PREC = {"==": 2, "!=": 2, "<": 3, "<=": 3, ">": 3, ">=": 3,
          "<<": 4, ">>": 4, "+": 5, "-": 5, "*": 6, "/": 6}

  def peek_binop(self):
    self.ws()
    for op in self.BINOPS:
      if self.s.startswith(op, self.i):
        # don't treat '=' of '=>' or single '=' as operator
        if op == ">" and self.s.startswith("=>", self.i - 0):
          pass
        return op
    return None

  def parse_expr(self):
    return self.parse_ternary()

  def parse_ternary(self):
    c = self.parse_binary(0)
    if self.peek() == "?":
      self.take("?")
      a = self.parse_ternary()
      self.take(":")
      b = self.parse_ternary()
      return ("ternary", c, a, b)
    return c

  def parse_binary(self, minprec):
    left = self.parse_unary()
    while True:
      op = self.peek_binop()
      if op is None or self.PREC[op] < minprec:
        break
      self.i += len(op)
      right = self.parse_binary(self.PREC[op] + 1)
      left = ("binop", op, left, right)
    return left

  def parse_unary(self):
    if self.peek() == "-":
      self.take("-")
      return ("neg", self.parse_unary())
    return self.parse_primary()

  def parse_primary(self):
    self.ws()
    c = self.s[self.i]
    if c == '"':
      return ("str", self.string())
    if c == "$":
      return ("var", self.var())
    if c == "(":
      self.take("(")
      e = self.parse_expr()
      self.take(")")
      return e
    if c.isdigit():
      return ("num", self.number())
    if c.isalpha() or c == "_":
      name = self.ident()
      if self.peek() == "[":
        self.take("[")
        idx = self.parse_expr()
        self.take("]")
        return ("table", name, idx)
      if self.peek() == "(":
        self.take("(")
        args = []
        while self.peek() != ")":
          args.append(self.parse_expr())
          if self.peek() == ",":
            self.take(",")
        self.take(")")
        return ("call", name, args)
      if name == "none":
        return ("none",)
      return ("ident", name)
    raise SyntaxError("bad expr char %r at %r" %
                      (c, self.s[self.i:self.i + 24]))


def postprocess(g):
  for sm in g["submatches"].values():
    # collect field letters
    for r in sm.rules:
      for e in r.pattern:
        if e[0] == "field":
          sm.field_letters.add(e[1])
    # dispatch params = params that are not consumed as a stream field;
    # a <...> matches them all, one bit each, in declaration order (MSB first).
    cands = [p for p in sm.params if p not in sm.field_letters]
    sm.dispatch_params = cands
    sm.dispatch_param = cands[0] if cands else None
    # detect prefix frames (self-recursive, token-prepending, state-setting)
    for r in sm.rules:
      has_self = any(e[0] == "sub" and e[1] == sm.name for e in r.pattern)
      has_act = any(e[0] == "act" for e in r.pattern)
      lead_hex = r.pattern and r.pattern[0][0] in ("hex", "hexn")
      lead_str = r.template and r.template[0][0] == "str"
      if has_self and has_act and lead_hex and lead_str:
        r.is_prefix_frame = True
        r.ptoken = r.template[0][1].strip()
        for e in r.pattern:
          if e[0] == "act":
            r.pvar = e[1][0][0]
            break
        sm.has_prefix_frames = True


# =============================================================================
# 2.  ENGINE  -- run the generated matchers over a byte image
# =============================================================================

# Section 6.4 recovery is built-in engine behaviour for these prefix vars.
# A token is suppressed when the render emits the determining form.
RECOVER_VARS = ("opsiz", "adrsiz", "segidx", "reptype")
# Of these, only opsiz/reptype are revealed through a *table index* (a register
# or mnemonic table); folding can collapse that index to a literal, so their
# consumption is recovered from the dispatch instead. adrsiz/segidx are revealed
# only by the rm16/seg lookups, which carry stream fields and are never folded.
DISP_RECOVER = ("opsiz", "reptype")

class Stream:
  def __init__(self, buf, bitorder):
    self.buf = buf
    self.bit = 0
    self.lsb = (bitorder == "lsb")

  def read_bit(self):
    idx = self.bit >> 3
    byte = self.buf[idx] if idx < len(self.buf) else 0
    off = self.bit & 7
    b = (byte >> off) & 1 if self.lsb else (byte >> (7 - off)) & 1
    self.bit += 1
    return b

  def read_bits(self, n):
    return [self.read_bit() for _ in range(n)]

  def bytepos(self):
    return self.bit >> 3


def field_value(bits, endian, lsb):
  n = len(bits)
  if n and n % 8 == 0:
    bvals = []
    for i in range(0, n, 8):
      chunk = bits[i:i + 8]
      if lsb:
        chunk = chunk[::-1]
      v = 0
      for b in chunk:
        v = (v << 1) | b
      bvals.append(v)
    if endian == "le":
      bvals = bvals[::-1]
    val = 0
    for b in bvals:
      val = (val << 8) | b
    return val
  val = 0
  for b in bits:
    val = (val << 1) | b
  return val


class Ctx:
  def __init__(self, g, buf):
    self.g = g
    self.stream = Stream(buf, g["arch"].get("bitorder", "msb"))
    self.endian = g["arch"].get("endian", "le")
    self.lsb = (g["arch"].get("bitorder", "msb") == "lsb")
    self.vars = {}
    self.events = []          # (table, idx, vars_in_index, result) for recovery
    self.prefix_tokens = []   # (token, var) in stream order
    self.witness = []         # hoisted annotation tokens from wit() (lossless)
    self.disp_layout = dispatch_layouts(g)   # submatch -> {param_bit: prefix_var}
    self.disp_width = dispatch_widths(g)      # submatch -> composite bit width
    self.disp_consumed = []   # prefix vars a matched <...> dispatch pinned
    self.insn_str = None
    self.S = 0


class NoMatch(Exception):
  pass


def truthy(v):
  if v is NONE:
    return False
  return bool(v)


def sx(n, bits):
  n &= (1 << bits) - 1
  m = 1 << (bits - 1)
  return (n ^ m) - m


def builtin(name, a):
  v = a[0]
  if v is NONE:
    v = 0
  if name == "hex":
    v = int(v)
    return ("-0x%x" % (-v)) if v < 0 else ("0x%x" % v)
  if name == "dec":
    return str(int(v))
  if name == "sgn":
    v = int(v)
    if v < 0:
      return "-0x%x" % (-v)
    if v >= 0x80000000:
      return "-0x%x" % (0x100000000 - v)
    return "+0x%x" % v
  if name == "sx8":
    return sx(int(v), 8)
  if name == "sx16":
    return sx(int(v), 16)
  if name == "sx32":
    return sx(int(v), 32)
  raise NoMatch("unknown builtin %s" % name)


def expr_vars(e):
  t = e[0]
  if t == "var":
    return {e[1]}
  if t == "table":
    return expr_vars(e[2])
  if t == "call":
    s = set()
    for a in e[2]:
      s |= expr_vars(a)
    return s
  if t == "binop":
    return expr_vars(e[2]) | expr_vars(e[3])
  if t == "neg":
    return expr_vars(e[1])
  if t == "ternary":
    return expr_vars(e[1]) | expr_vars(e[2]) | expr_vars(e[3])
  return set()


def num(v):
  return 0 if v is NONE else int(v)


def _lin(e, env):
  # evaluate a linear/bit-packing expression over an int env (0 elsewhere)
  t = e[0]
  if t == "num":
    return e[1]
  if t == "var":
    return env.get(e[1], 0)
  if t == "binop":
    a, b, op = _lin(e[2], env), _lin(e[3], env), e[1]
    if op == "+": return a + b
    if op == "-": return a - b
    if op == "*": return a * b
    if op == "/": return a // b if b else 0
    if op == "<<": return a << b
    if op == ">>": return a >> b
    return 0
  if t == "neg":
    return -_lin(e[1], env)
  return 0


def composite_layout(expr):
  # Read a bit-packing expression as a linear combination: each var's
  # coefficient gives its low bit, the gaps give its width. Returns
  # ({var: (bit_pos, width)}, total_bits).
  dvars = sorted(expr_vars(expr))
  base = _lin(expr, {v: 0 for v in dvars})
  pos = {}
  for v in dvars:
    env = {x: 0 for x in dvars}
    env[v] = 1
    c = _lin(expr, env) - base
    if c > 0 and (1 << (c.bit_length() - 1)) == c:
      pos[v] = c.bit_length() - 1
  bypos = sorted(pos, key=lambda v: pos[v])
  layout = {}
  total = 0
  for i, v in enumerate(bypos):
    nxt = pos[bypos[i + 1]] if i + 1 < len(bypos) else pos[v] + 1
    w = nxt - pos[v]
    layout[v] = (pos[v], w)
    total = max(total, pos[v] + w)
  return layout, total


def _dispatch_exprs(g):
  # {submatch: expression bound to its <...> param at the call site}, for
  # params bound to a packed composite (a bare variable is a plain selector).
  calls = {}
  for sm in g["submatches"].values():
    for r in sm.rules:
      for e in r.pattern:
        if e[0] == "sub" and e[2] is not None:
          calls.setdefault(e[1], []).append(e[2])
  out = {}
  for name, sm in g["submatches"].items():
    dp = sm.dispatch_param
    if dp is None or dp not in sm.params:
      continue
    idx = sm.params.index(dp)
    expr = next((a[idx] for a in calls.get(name, []) if idx < len(a)), None)
    if expr is not None and expr[0] != "var":
      out[name] = expr
  return out


def dispatch_widths(g):
  # full bit-width of each composite <...> param, so the engine can left-align
  # a spec whose trailing don't-cares were dropped (`<a o ? ?>` written `<a o>`).
  out = {}
  for name, expr in _dispatch_exprs(g).items():
    _, total = composite_layout(expr)
    if total:
      out[name] = total
  return out


def dispatch_layouts(g):
  # {submatch: {param_bit: prefix_var}} for the recover vars packed into a
  # composite dispatch -- lets recovery learn a dispatch consumed e.g. $opsiz.
  out = {}
  for name, expr in _dispatch_exprs(g).items():
    layout, _ = composite_layout(expr)
    bits = {}
    for v, (p, w) in layout.items():
      if v in DISP_RECOVER:
        for b in range(p, p + w):
          bits[b] = v
    if bits:
      out[name] = bits
  return out


def ev(e, loc, pmap, ctx):
  t = e[0]
  if t == "num":
    return e[1]
  if t == "str":
    return e[1]
  if t == "none":
    return NONE
  if t == "var":
    n = e[1]
    if n == "S":
      return ctx.S
    if n == "E":
      return ctx.stream.bytepos()
    if n in loc:
      return loc[n]
    if n in pmap and pmap[n] is not UNBOUND:
      return pmap[n]
    if n in ctx.vars:
      return ctx.vars[n]
    if n in ctx.g["arch"]:
      return ctx.g["arch"][n]
    if n in pmap:
      return pmap[n]
    raise NoMatch("unbound $%s" % n)
  if t == "table":
    name, idxe = e[1], e[2]
    idx = ev(idxe, loc, pmap, ctx)
    if idx is NONE:
      res = 0
      iv = 0
    else:
      iv = int(idx)
      res = ctx.g["tables"][name][iv]
    ctx.events.append((name, iv, expr_vars(idxe), res))
    return res
  if t == "call":
    args = [ev(x, loc, pmap, ctx) for x in e[2]]
    if e[1] == "wit":
      # wit(s): register a hoisted annotation token for losslessness, emit "".
      s = args[0]
      if s is not NONE and s != "":
        ctx.witness.append(str(s))
      return ""
    return builtin(e[1], args)
  if t == "binop":
    op = e[1]
    lv = num(ev(e[2], loc, pmap, ctx))
    rv = num(ev(e[3], loc, pmap, ctx))
    if op == "+": return lv + rv
    if op == "-": return lv - rv
    if op == "*": return lv * rv
    if op == "/": return lv // rv
    if op == "<<": return lv << rv
    if op == ">>": return lv >> rv
    if op == "==": return 1 if lv == rv else 0
    if op == "!=": return 1 if lv != rv else 0
    if op == "<": return 1 if lv < rv else 0
    if op == "<=": return 1 if lv <= rv else 0
    if op == ">": return 1 if lv > rv else 0
    if op == ">=": return 1 if lv >= rv else 0
  if t == "neg":
    return -num(ev(e[1], loc, pmap, ctx))
  if t == "ternary":
    c = ev(e[1], loc, pmap, ctx)
    return ev(e[2], loc, pmap, ctx) if truthy(c) else ev(e[3], loc, pmap, ctx)
  raise NoMatch("bad expr %r" % (e,))


def to_str(v):
  if isinstance(v, bool):
    return str(int(v))
  if isinstance(v, int):
    return str(v)
  if v is NONE:
    return "none"
  return str(v)


def render(template, loc, pmap, ctx):
  vals = [ev(term, loc, pmap, ctx) for term in template]
  if len(vals) == 1:
    return vals[0]
  return "".join(to_str(v) for v in vals)


def invoke(name, argvals, ctx):
  sm = ctx.g["submatches"][name]
  pmap = {}
  for i, p in enumerate(sm.params):
    pmap[p] = argvals[i] if i < len(argvals) else UNBOUND
  start_bit = ctx.stream.bit
  start_ev = len(ctx.events)
  start_pt = len(ctx.prefix_tokens)
  start_wit = len(ctx.witness)
  start_dc = len(ctx.disp_consumed)
  vars_save = dict(ctx.vars)
  for rule in sm.rules:
    ctx.stream.bit = start_bit
    del ctx.events[start_ev:]
    del ctx.prefix_tokens[start_pt:]
    del ctx.witness[start_wit:]
    del ctx.disp_consumed[start_dc:]
    ctx.vars.clear()
    ctx.vars.update(vars_save)
    loc = {}
    exports = {}
    if try_rule(sm, rule, pmap, loc, exports, ctx):
      result = render(rule.template, loc, pmap, ctx)
      if sm.has_prefix_frames and not rule.is_prefix_frame:
        ctx.insn_str = result
      return result, exports
  raise NoMatch("no rule in %s at bit %d" % (name, start_bit))


def try_rule(sm, rule, pmap, loc, exports, ctx):
  elems = rule.pattern
  # names that a field/submatch element binds during this rule
  binders = set()
  for e in elems:
    if e[0] == "field":
      binders.add(e[1])
    elif e[0] == "sub":
      binders.add(e[1])
  # actions, with the external deps that gate when each may fire
  acts = []
  for e in elems:
    if e[0] == "act":
      deps = set()
      for (v, terms) in e[1]:
        for t in terms:
          deps |= expr_vars(t)
      acts.append([e, deps & binders, False])
  first_sub = next((k for k, e in enumerate(elems) if e[0] == "sub"), len(elems))
  bound = set()
  pfx_done = [False]

  def fire_ready():
    for a in acts:
      if not a[2] and a[1] <= bound:
        a[2] = True
        for (v, terms) in a[0][1]:
          ctx.vars[v] = render(terms, loc, pmap, ctx)
        if rule.is_prefix_frame and not pfx_done[0]:
          pfx_done[0] = True
          ctx.prefix_tokens.append((rule.ptoken, rule.pvar))

  for k, e in enumerate(elems):
    if k == first_sub:
      fire_ready()       # constant / state actions land before the first @sub
    kind = e[0]
    if kind == "hex":
      bits = ctx.stream.read_bits(8)
      v = 0
      for b in bits:
        v = (v << 1) | b
      if ctx.lsb:
        v = int("{:08b}".format(v)[::-1], 2)
      if v != e[1]:
        return False
    elif kind == "hexn":
      bits = ctx.stream.read_bits(e[2])
      v = 0
      for b in bits:
        v = (v << 1) | b
      if v != e[1]:
        return False
    elif kind == "lit":
      if ctx.stream.read_bit() != e[1]:
        return False
    elif kind == "field":
      letter, nbits = e[1], e[2]
      v = field_value(ctx.stream.read_bits(nbits), ctx.endian, ctx.lsb)
      if letter in pmap and pmap[letter] is not UNBOUND:
        if pmap[letter] != v:
          return False
      else:
        loc[letter] = v
        if letter in pmap and pmap[letter] is UNBOUND:
          exports[letter] = v
      bound.add(letter)
      fire_ready()
    elif kind == "disp":
      dp = sm.dispatch_param
      dv = pmap.get(dp, UNBOUND)
      if dv is UNBOUND or dv is NONE:
        return False
      specs = e[1]
      # the param's full width: a spec may omit trailing don't-care (low) bits
      w = ctx.disp_width.get(sm.name, len(specs))
      dv = int(dv)
      ok = True
      for i, sp in enumerate(specs):
        if sp is None:
          continue
        if ((dv >> (w - 1 - i)) & 1) != sp:
          ok = False
          break
      if not ok:
        return False
      lay = ctx.disp_layout.get(sm.name)
      if lay:
        for i, sp in enumerate(specs):
          if sp is None:
            continue
          v = lay.get(w - 1 - i)
          if v is not None:
            ctx.disp_consumed.append(v)
    elif kind == "sub":
      subname, args = e[1], e[2]
      av = [] if args is None else [ev(a, loc, pmap, ctx) for a in args]
      try:
        res, subexports = invoke(subname, av, ctx)
      except NoMatch:
        return False
      loc[subname] = res
      for kk, vv in subexports.items():
        loc[kk] = vv
        if kk in pmap and pmap[kk] is UNBOUND:
          exports[kk] = vv
      bound.add(subname)
      fire_ready()
    elif kind == "guard":
      if not truthy(ev(e[1], loc, pmap, ctx)):
        return False
    elif kind == "act":
      pass
  fire_ready()
  return True


LOCKABLE = frozenset(
    "add adc and btc btr bts cmpxchg cmpxchg8b dec inc neg not or sbb sub xor xadd".split())
BRANCH = frozenset("call jmp ret".split())


def _ctx_rename(kept, insn):
  # F2/F3/3E have context-dependent mnemonics: on a lock-able op (with lock, or
  # xchg's implicit lock) F2->xacquire / F3->xrelease; F3 on a mov store is also
  # xrelease; F2 on a near branch is bnd; 3E on an indirect branch is notrack.
  # The hint prefixes leak here as repnz/rep/segds; rename in place (order and
  # bytes are preserved, norm() strips them so text-compare is unaffected).
  if not insn or not kept:
    return kept
  p = insn.split(None, 1)
  m0 = p[0]
  if "." in m0 and m0.rsplit(".", 1)[1] in ("b", "w", "d", "q", "t"):
    m0 = m0.rsplit(".", 1)[0]
  rest = p[1] if len(p) > 1 else ""
  has_lock = "lock" in kept
  mem_dest = rest.startswith("[")
  out = []
  for tk in kept:
    if tk == "repnz":
      if (has_lock and m0 in LOCKABLE) or m0 == "xchg":
        tk = "xacquire"
      elif m0 in BRANCH or m0[:1] == "j":
        tk = "bnd"
    elif tk == "rep":
      if (has_lock and m0 in LOCKABLE) or m0 == "xchg":
        tk = "xrelease"
      elif m0 == "mov" and mem_dest:
        tk = "xrelease"
    elif tk == "segds":
      if m0 in ("call", "jmp"):
        tk = "notrack"
    out.append(tk)
  return out


def recover(ctx):
  evs = ctx.events
  dc = set(ctx.disp_consumed)
  f = {
    "opsiz":   any("opsiz" in vs for (_, _, vs, _) in evs)
               or any(tb == "rm16" for (tb, _, _, _) in evs)
               or "opsiz" in dc,
    "adrsiz":  any(tb == "rm16" for (tb, _, _, _) in evs)
               or "adrsiz" in dc,
    "segidx":  any(tb == "seg" and res != "" for (tb, _, _, res) in evs)
               or "segidx" in dc,
    "reptype": any("reptype" in vs for (_, _, vs, _) in evs)
               or "reptype" in dc,
  }
  toks = ctx.prefix_tokens
  drop = set()
  for var in RECOVER_VARS:
    idxs = [k for k, (t, v) in enumerate(toks) if v == var]
    if len(idxs) == 1 and f[var]:
      drop.add(idxs[0])
  # Out-of-order prefixes must be encoded explicitly.  The asm reconstructs the
  # byte stream as kept(leaked) tokens in stream order, followed by the dropped
  # (consumed) prefixes in REVEAL_ORDER.  If that ordering would not reproduce
  # the actual stream order, drop nothing: render every prefix as an explicit
  # token in stream order so the asm replays them verbatim (norm() strips them,
  # so text-compare is unaffected; round-trip becomes byte-exact).
  REVEAL_IDX = {"segidx": 0, "adrsiz": 1, "opsiz": 2, "reptype": 3}
  kept_idx = [k for k in range(len(toks)) if k not in drop]
  drop_idx = [k for k in range(len(toks)) if k in drop]
  asm_seq = kept_idx + sorted(drop_idx, key=lambda k: REVEAL_IDX.get(toks[k][1], 9))
  if asm_seq != list(range(len(toks))):
    drop = set()
  kept = [t for k, (t, v) in enumerate(toks) if k not in drop]
  insn = ctx.insn_str if ctx.insn_str is not None else ""
  kept = _ctx_rename(kept, insn)
  parts = kept + list(ctx.witness) + ([insn] if insn != "" else [])
  return " ".join(parts)


def disassemble(g, buf):
  ctx = Ctx(g, buf)
  out = []
  total = len(buf) * 8
  while ctx.stream.bit < total:
    ctx.S = ctx.stream.bytepos()
    ctx.vars = {k: g["vars"][k] for k in g["vars"]}
    ctx.events = []
    ctx.prefix_tokens = []
    ctx.witness = []
    ctx.disp_consumed = []
    ctx.insn_str = None
    save = ctx.stream.bit
    try:
      invoke("main", [], ctx)
    except NoMatch as exc:
      off = ctx.S
      out.append("; <undecodable @0x%x: %02x> (%s)" % (off, buf[off], exc))
      break
    out.append(recover(ctx))
  return out


# =============================================================================
# 3.  CLI
# =============================================================================
#  __LIBRARY_END__   (everything above is embedded verbatim by --emit)
def emit_standalone(desc_path):
  # Generate a self-contained disassembler: the engine above + the embedded
  # description + a tiny runner.  No external .p needed at run time.
  src = open(__file__).read()
  lib = src.split("#  __LIBRARY_END__", 1)[0]
  desc = open(desc_path).read()
  out = []
  out.append("#!/usr/bin/env python3")
  out.append("# Auto-generated standalone disassembler (parsergen.py --emit).")
  out.append("# Embeds the engine + the bit-pattern description; run: x86d.py IMAGE.bin")
  out.append(lib.rstrip())
  out.append("")
  out.append("DESCRIPTION = r'''")
  out.append(desc.replace("'''", "'' '"))
  out.append("'''")
  out.append("")
  out.append("def main(argv):")
  out.append("  import sys")
  out.append("  g = Parser(DESCRIPTION).parse()")
  out.append("  if not argv:")
  out.append("    sys.stderr.write('usage: x86d.py IMAGE.bin\\n'); return 2")
  out.append("  buf = open(argv[0], 'rb').read()")
  out.append("  for line in disassemble(g, buf):")
  out.append("    print(line)")
  out.append("  return 0")
  out.append("")
  out.append("if __name__ == '__main__':")
  out.append("  import sys; sys.exit(main(sys.argv[1:]))")
  return "\n".join(out) + "\n"


def dump_grammar(g):
  a = g["arch"]
  print("arch:", a)
  print("vars:", g["vars"])
  for n, t in g["tables"].items():
    print("table %-7s (%d): %s" % (n, len(t), t))
  for n, sm in g["submatches"].items():
    print("submatch %s(%s)  dispatch=%s  fields=%s  pfx=%s"
          % (n, ",".join(sm.params), sm.dispatch_param,
             "".join(sorted(sm.field_letters)), sm.has_prefix_frames))
    for r in sm.rules:
      tag = "  [PFX %s/%s]" % (r.ptoken, r.pvar) if r.is_prefix_frame else ""
      print("    %d elems => %d terms%s" % (len(r.pattern), len(r.template), tag))


def main(argv):
  if len(argv) >= 2 and argv[0] == "--dump":
    g = Parser(open(argv[1]).read()).parse()
    dump_grammar(g)
    return 0
  if len(argv) >= 2 and argv[0] == "--emit":
    sys.stdout.write(emit_standalone(argv[1]))
    return 0
  if len(argv) < 2:
    sys.stderr.write("usage: parsergen.py DESC.p IMAGE.bin\n"
                     "       parsergen.py --dump DESC.p\n"
                     "       parsergen.py --emit DESC.p   > x86d.py\n")
    return 2
  g = Parser(open(argv[0]).read()).parse()
  buf = open(argv[1], "rb").read()
  for line in disassemble(g, buf):
    print(line)
  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
