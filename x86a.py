#!/usr/bin/env python3
# Auto-generated standalone assembler (asm.py --emit).
# Embeds the engine + reverse engine + description; run: x86a.py IN.asm > out.bin
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

import sys as _sys
pg = _sys.modules[__name__]   # pg.* resolves to the embedded engine

#!/usr/bin/env python3
# asm.py -- assemble text (the disassembler's output) back into bytes, from the
# SAME bit-pattern description parsergen.py uses to disassemble.  Inverse engine:
# for each line it reverse-matches an insn rule's template to recover the field
# values + prefix state, then emits that rule's pattern bits.
#
#   usage:  asm.py corpus.p input.asm > out.bin
#
# Verified by round-trip: asm.py corpus.p corpus.asm == corpus.bin.

import sys
import re

NONE = pg.NONE

# ---- prefixes --------------------------------------------------------------
# token -> (byte, var, value)
PFX = {
  "opsiz": (0x66, "opsiz", 1), "adrsiz": (0x67, "adrsiz", 1),
  "seges": (0x26, "segidx", 1), "segcs": (0x2e, "segidx", 2),
  "segss": (0x36, "segidx", 3), "segds": (0x3e, "segidx", 4),
  "segfs": (0x64, "segidx", 5), "seggs": (0x65, "segidx", 6),
  "rep": (0xf3, "reptype", 1), "repnz": (0xf2, "reptype", 2),
  "lock": (0xf0, "lock", 1),
  "xacquire": (0xf2, "reptype", 2), "xrelease": (0xf3, "reptype", 1),
  "bnd": (0xf2, "reptype", 2), "notrack": (0x3e, "segidx", 4),
}
WITNESS = {"disp8", "disp32", "alt", "imm32", "long", "altw", "altl", "c5", "evex", "modz", "vlo"}
SEG_BYTE = {1: 0x26, 2: 0x2e, 3: 0x36, 4: 0x3e, 5: 0x64, 6: 0x65}
# revealed-prefix emit order (matches the stream order the corpus uses)
REVEAL_ORDER = ["segidx", "adrsiz", "opsiz", "reptype"]


class NoAsm(Exception):
  pass


# ---- value-producing leaf subs: byte width + how the value is sized ---------
def vsub_width(name, env):
  if name in ("imm8", "imm8b", "disp8", "rel8"):
    return 1
  if name in ("imm16", "disp16"):
    return 2
  if name in ("imm32", "disp32"):
    return 4
  if name in ("immz", "relz", "immz1", "relz1"):
    return 2 if env.get("opsiz", 0) else 4
  if name in ("immadr", "immadr1"):
    return 2 if env.get("adrsiz", 0) else 4
  return None


# ============================================================================
# solving index / builtin argument expressions for the one unknown field
# ============================================================================
def lin_coeffs(e, freev):
  # expr as const + sum(coeff*var) over free vars; returns (const, {var:coeff})
  # only +,-,*,<<,>> with constant scaling are needed for these grammars.
  def go(e):
    t = e[0]
    if t == "num":
      return (e[1], {})
    if t == "var":
      if e[1] in freev:
        return (0, {e[1]: 1})
      raise NoAsm("unbound %s" % e[1])  # caller substitutes knowns first
    if t == "neg":
      c, d = go(e[1])
      return (-c, {k: -v for k, v in d.items()})
    if t == "binop":
      op = e[1]
      ca, da = go(e[2])
      cb, db = go(e[3])
      if op in ("+", "-"):
        out = dict(da)
        for k, v in db.items():
          out[k] = out.get(k, 0) + (v if op == "+" else -v)
        return (ca + cb if op == "+" else ca - cb, out)
      if op == "*":
        if not db:
          return (ca * cb, {x: v * cb for x, v in da.items()})
        if not da:
          return (ca * cb, {x: v * ca for x, v in db.items()})
        raise NoAsm("var*var")
      if op == "<<":
        if not db:                       # (vars+const) << const  -> linear
          return (ca << cb, {x: v << cb for x, v in da.items()})
        raise NoAsm("<< nonconst")
      if op == ">>":
        if not da and not db:
          return (ca >> cb, {})
        raise NoAsm(">> nonlinear")
    raise NoAsm("nonlinear %r" % (e,))
  return go(e)


def subst_known(e, env):
  # replace known vars with their numeric value
  t = e[0]
  if t == "var" and e[1] in env and not isinstance(env[e[1]], str):
    return ("num", int(env[e[1]]))
  if t == "binop":
    return ("binop", e[1], subst_known(e[2], env), subst_known(e[3], env))
  if t == "neg":
    return ("neg", subst_known(e[1], env))
  if t == "call":
    return ("call", e[1], [subst_known(a, env) for a in e[2]])
  if t == "table":
    return ("table", e[1], subst_known(e[2], env))
  if t == "ternary":
    return ("ternary", subst_known(e[1], env), subst_known(e[2], env),
            subst_known(e[3], env))
  return e


def eval_expr(e, env, g):
  # context-free evaluation (mirrors the engine's ev) for solving index/guard
  # expressions -- handles table lookups, builtins, ternary, comparisons.
  t = e[0]
  if t == "num":
    return e[1]
  if t == "str":
    return e[1]
  if t == "none":
    return NONE
  if t == "var":
    n = e[1]
    if n in env:
      return env[n]
    if n in g["arch"]:
      return g["arch"][n]
    raise NoAsm("unbound %s" % n)
  if t == "table":
    idx = eval_expr(e[2], env, g)
    if idx is NONE:
      return 0
    iv = int(idx)
    tbl = g["tables"][e[1]]
    if iv < 0 or iv >= len(tbl):
      raise NoAsm("table idx oob")
    return tbl[iv]
  if t == "call":
    args = [eval_expr(x, env, g) for x in e[2]]
    if e[1] == "wit":
      return ""
    return pg.builtin(e[1], args)
  if t == "binop":
    op = e[1]
    a, b = pg.num(eval_expr(e[2], env, g)), pg.num(eval_expr(e[3], env, g))
    if op == "+": return a + b
    if op == "-": return a - b
    if op == "*": return a * b
    if op == "/": return a // b if b else 0
    if op == "<<": return a << b
    if op == ">>": return a >> b
    if op == "==": return 1 if a == b else 0
    if op == "!=": return 1 if a != b else 0
    if op == "<": return 1 if a < b else 0
    if op == "<=": return 1 if a <= b else 0
    if op == ">": return 1 if a > b else 0
    if op == ">=": return 1 if a >= b else 0
  if t == "neg":
    return -pg.num(eval_expr(e[1], env, g))
  if t == "ternary":
    c = eval_expr(e[1], env, g)
    return eval_expr(e[2], env, g) if pg.truthy(c) else eval_expr(e[3], env, g)
  raise NoAsm("eval %r" % (e,))


def solve_to(e, target, env, g):
  # bind the free vars of e so e == target; returns True on success.  Handles a
  # wrapping sx8/sx16/sx32 (target's low bits are the stored field), a fast
  # linear/bit-decomposition path (for wide immediates), and a brute-force
  # fallback over a small domain (for >>var and nested-table index exprs).
  if e[0] == "call" and e[1] in ("sx8", "sx16", "sx32"):
    bits = {"sx8": 8, "sx16": 16, "sx32": 32}[e[1]]
    return solve_to(e[2][0], target & ((1 << bits) - 1), env, g)
  e2 = subst_known(e, env)
  freev = pg.expr_vars(e2)
  if not freev:
    try:
      return eval_expr(e2, {}, g) == target
    except NoAsm:
      return False
  try:
    const, coeffs = lin_coeffs(e2, freev)
    rem = target - const
    if len(coeffs) == 1:
      (v, c), = coeffs.items()
      if c != 0 and rem % c == 0:
        return _bind(env, v, rem // c)
      return False
    if len(coeffs) == 2:
      (vh, ch), (vl, cl) = sorted(coeffs.items(), key=lambda kv: -abs(kv[1]))
      if cl == 1 and ch > 1 and (ch & (ch - 1)) == 0:
        hi, lo = divmod(rem, ch)
        if _bind(env, vh, hi) and _bind(env, vl, lo):
          return True
        return False
      # general linear two-unknown solve for register/field index exprs such as
      # vsse[$l*8 + (15-$v)].  Enumerate the high-magnitude var (e.g. $l) from
      # HIGH to LOW so the more-specific wide form (ymm, L=1) is chosen before
      # the narrow alias (xmm, L=0) when a leading memory operand left L free.
      if ch != 0 and cl != 0:
        for hv in range(15, -1, -1):
          num = rem - ch * hv
          if num % cl != 0:
            continue
          lv = num // cl
          if 0 <= lv <= 7:
            save = dict(env)
            if _bind(env, vh, hv) and _bind(env, vl, lv):
              return True
            env.clear(); env.update(save)
        return False
  except NoAsm:
    pass
  if len(freev) == 1:
    (v,) = tuple(freev)
    for c in range(256):
      try:
        if eval_expr(subst_known(e2, {v: c}), {}, g) == target:
          return _bind(env, v, c)
      except NoAsm:
        pass
  if len(freev) == 2:
    # register/field index exprs such as vsse[$l*8 + (15-$v)] where the leading
    # operand (memory) binds neither var: enumerate both over the field domain.
    va, vb = sorted(freev)
    for a in range(16):
      for b in range(16):
        try:
          if eval_expr(subst_known(e2, {va: a, vb: b}), {}, g) == target:
            return _bind(env, va, a) and _bind(env, vb, b)
        except NoAsm:
          pass
  return False


def _bind(env, v, val):
  if v in env and not isinstance(env[v], str):
    return int(env[v]) == val
  env[v] = val
  return True


def _ev(e):
  t = e[0]
  if t == "num":
    return e[1]
  if t == "neg":
    return -_ev(e[1])
  if t == "binop":
    op, a, b = e[1], _ev(e[2]), _ev(e[3])
    if op == "+":
      return a + b
    if op == "-":
      return a - b
    if op == "*":
      return a * b
    if op == "/":
      return a // b if b else 0
    if op == "<<":
      return a << b
    if op == ">>":
      return a >> b
  raise NoAsm("noeval %r" % (e,))


# ============================================================================
# generic template reverse-match  (returns remaining string or None)
# ============================================================================
HEXRE = re.compile(r"-?0x[0-9a-fA-F]+")
DECRE = re.compile(r"\d+")


def _expr_has_var(e, name):
  if isinstance(e, tuple):
    if len(e) > 1 and e[0] == "var" and e[1] == name:
      return True
    return any(_expr_has_var(x, name) for x in e)
  if isinstance(e, list):
    return any(_expr_has_var(x, name) for x in e)
  return False


def _var_in_terms(name, terms):
  return any(_expr_has_var(t, name) for t in terms)


def rmatch(terms, s, env, asm):
  # Generator: yield a solution-env snapshot for every way the templated
  # `terms` can fully consume `s`.  An ambiguous table -- two indices mapping
  # to the same text (e.g. shl at /4 and /6) -- therefore yields more than one
  # solution; asm_core verifies each by re-disassembly and keeps the matches.
  if not terms:
    if s == "":
      yield dict(env)
    return
  t = terms[0]
  rest_terms = terms[1:]
  kind = t[0]

  if kind == "str":
    if s.startswith(t[1]):
      yield from rmatch(rest_terms, s[len(t[1]):], env, asm)
    return

  if kind == "table":
    name, idxe = t[1], t[2]
    tbl = asm.g["tables"][name]
    # entries render as their string form (numeric tables such as the SIB scale
    # {1,2,4,8} store ints); match on that text, longest first.
    def ent_str(v):
      return v if isinstance(v, str) else str(v)
    cands = sorted({ent_str(tbl[i]) for i in range(len(tbl))
                    if tbl[i] is not None and ent_str(tbl[i]) != ""
                    and s.startswith(ent_str(tbl[i]))},
                   key=len, reverse=True)
    # also allow empty-string entries (consume nothing) as a last resort
    cands.append("")
    for ent in cands:
      for i in range(len(tbl)):
        if tbl[i] is None or ent_str(tbl[i]) != ent:
          continue
        save = dict(env)
        if solve_to(idxe, i, env, asm.g):
          yield from rmatch(rest_terms, s[len(ent):], env, asm)
        env.clear()
        env.update(save)
    return

  if kind == "call":
    fn = t[1]
    if fn == "hex":
      m = HEXRE.match(s)
      if not m:
        return
      v = int(m.group(0), 16)
      save = dict(env)
      if solve_to(t[2][0], v, env, asm.g):
        yield from rmatch(rest_terms, s[m.end():], env, asm)
      env.clear(); env.update(save)
      return
    if fn == "dec":
      m = DECRE.match(s)
      if not m:
        return
      v = int(m.group(0))
      save = dict(env)
      if solve_to(t[2][0], v, env, asm.g):
        yield from rmatch(rest_terms, s[m.end():], env, asm)
      env.clear(); env.update(save)
      return
    if fn == "sgn":
      m = HEXRE.match(s)
      if not m:
        return
      raw = int(m.group(0), 16)
      v = raw if m.group(0)[0] != "-" else raw  # raw already negative
      save = dict(env)
      if solve_to(t[2][0], v, env, asm.g):
        yield from rmatch(rest_terms, s[m.end():], env, asm)
      env.clear(); env.update(save)
      return
    if fn == "wit":
      # A conditional witness `wit($var==c ? "tok" : "")` can be the ONLY carrier
      # of a pattern bit that no operand reveals (e.g. the VEX/XOP vvvv high bit,
      # masked off in 32-bit).  If `$var` appears in no operand of the remaining
      # template, reverse it from the token's presence so emit_pattern can rebuild
      # the byte.  Otherwise (var bound by an operand) consume nothing -- the
      # operand match handles it, exactly as before.
      arg = t[2][0] if t[2] else None
      if (arg is not None and arg[0] == "ternary"
          and arg[1][0] == "binop" and arg[1][1] == "=="
          and arg[1][2][0] == "var" and arg[1][3][0] == "num"):
        name = arg[1][2][1]; c = arg[1][3][1]
        then_e = arg[2]
        then_s = then_e[1] if (then_e and then_e[0] == "str") else None
        present = then_s is not None and then_s in env.get("__wits__", [])
        if name not in env and not _var_in_terms(name, rest_terms) and c in (0, 1):
          env[name] = c if present else (1 - c)
      yield from rmatch(rest_terms, s, env, asm)
      return
    raise NoAsm("rmatch call %s" % fn)

  if kind == "var":
    # a sub-result string (e.g. $addr) -- handled by the addressing matcher
    name = t[1]
    if name == "addr" or name in ("sib0", "sib1", "addr1"):
      yield from asm.rmatch_addr(name, rest_terms, s, env)
    return

  raise NoAsm("rmatch kind %s" % kind)


# ============================================================================
# emission: walk a pattern, append bits (MSB-first); pack to bytes at the end
# ============================================================================
class Emit:
  def __init__(self):
    self.bits = []

  def bit(self, b):
    self.bits.append(b & 1)

  def val(self, v, n):
    for i in range(n - 1, -1, -1):
      self.bits.append((v >> i) & 1)

  def le(self, v, nbytes):
    v &= (1 << (8 * nbytes)) - 1
    for k in range(nbytes):
      self.val((v >> (8 * k)) & 0xff, 8)

  def bytes(self):
    assert len(self.bits) % 8 == 0, "non-byte-aligned emission: %d" % len(self.bits)
    out = bytearray()
    for i in range(0, len(self.bits), 8):
      b = 0
      for j in range(8):
        b = (b << 1) | self.bits[i + j]
      out.append(b)
    return bytes(out)


class Asm:
  def __init__(self, g):
    self.g = g
    self.insn = g["submatches"]["insn"]

  # -- emit one pattern into Emit e using env (fields + prefix state) ---------
  def emit_pattern(self, pattern, env, e, start):
    for el in pattern:
      k = el[0]
      if k == "lit":
        e.bit(el[1])
      elif k == "hex":
        e.val(el[1], 8)
      elif k == "hexn":
        e.val(el[1], el[2])
      elif k == "field":
        letter, nbits = el[1], el[2]
        v = env.get(letter, 0)
        v = int(v) if not isinstance(v, str) else 0   # don't-care field -> 0
        if nbits > 8 and nbits % 8 == 0:
          e.le(v, nbits // 8)
        else:
          e.val(v, nbits)
      elif k in ("disp", "act", "guard"):
        pass
      elif k == "sub":
        self.emit_sub(el[1], el[2], env, e, start)
      else:
        raise NoAsm("emit kind %s" % k)

  def emit_sub(self, name, args, env, e, start):
    if name == "addr":
      plan = env.get("__addr__")
      if plan is None:
        raise NoAsm("addr not parsed")
      g = _ev(subst_known(args[0], env)) if args else int(env.get("g", 0))
      e.val((plan["mod"] << 6) | ((g & 7) << 3) | (plan["rm"] & 7), 8)
      if plan["sib"] is not None:
        e.val(plan["sib"], 8)
      if plan["dw"]:
        e.le(plan["disp"], plan["dw"])
      return
    w = vsub_width(name, env)
    if w is not None:
      key = name
      if name in ("immz", "relz"):
        # value bound under the wrapper name
        pass
      if name in ("rel8", "relz", "rel16"):
        target = int(env[key])
        end = start + (len(e.bits) // 8) + w
        e.le((target - end) & ((1 << (8 * w)) - 1), w)
      else:
        e.le(int(env[key]) & ((1 << (8 * w)) - 1), w)
      return
    # composite value subs: immz1/relz1 wrap imm/rel; addr/sib handled elsewhere
    raise NoAsm("emit_sub %s" % name)

  # -- addressing reverse-match ----------------------------------------------
  GREG32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
  RM16 = ["bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"]
  SEG_TBL = ["", "es:", "cs:", "ss:", "", "fs:", "gs:",
             "ss:", "es:", "cs:", "ss:", "ds:", "fs:", "gs:"]
  SBO = [0, 0, 0, 0, 7, 7, 0, 0]
  SBO16 = [0, 0, 7, 7, 0, 0, 7, 0]

  def _segidx(self, segstr, sbo):
    # smallest override that renders segstr at this base; prefer 0 (the default,
    # which needs no prefix byte -- e.g. ss: is implicit for ebp/esp).
    for si in range(7):
      if self.SEG_TBL[sbo + si] == segstr:
        return si
    return None

  def rmatch_addr(self, name, rest_terms, s, env):
    m = re.match(r"(es|cs|ss|ds|fs|gs):", s)
    seg, rest = "", s
    if m:
      seg, rest = m.group(0), s[m.end():]
    if not rest.startswith("[") or "]" not in rest:
      return
    k = rest.index("]")
    inner, after = rest[1:k], rest[k + 1:]
    plan = self._enc_addr(inner, seg, env)
    if plan is None:
      return
    env["__addr__"] = plan
    yield from rmatch(rest_terms, after, env, self)

  def _pick_disp(self, disp, wits):
    if "disp32" in wits:
      return (2, 4)
    if "disp8" in wits:
      return (1, 1)
    if -128 <= disp <= 127:
      return (1, 1)
    return (2, 4)

  def _enc_addr(self, inner, seg, env):
    wits = env.get("__wits__", [])
    plan = self._enc32(inner, seg, wits, env)
    if plan is not None:
      return plan
    return self._enc16(inner, seg, wits, env)

  def _set_seg(self, env, segstr, sbo):
    si = self._segidx(segstr, sbo)
    if si is None:
      return False
    if "segidx" in env and not isinstance(env["segidx"], str):
      return int(env["segidx"]) == si or si == 0
    env["segidx"] = si
    return True

  def _enc32(self, inner, seg, wits, env):
    R = r"(eax|ecx|edx|ebx|esp|ebp|esi|edi)"
    H = r"([+-]0x[0-9a-fA-F]+)"   # sgn()-style displacement (sign always present)
    G = self.GREG32.index
    base = index = disp = None
    scale = 1
    g = re.fullmatch(R + r"\+" + R + r"\*(\d+)" + H, inner)
    if g:
      base, index, scale, disp = G(g[1]), G(g[2]), int(g[3]), int(g[4], 16)
    elif (g := re.fullmatch(R + r"\+" + R + r"\*(\d+)", inner)):
      base, index, scale = G(g[1]), G(g[2]), int(g[3])
    elif (g := re.fullmatch(R + r"\*(\d+)\+(0x[0-9a-fA-F]+)", inner)):
      index, scale, disp = G(g[1]), int(g[2]), int(g[3], 16)
    elif (g := re.fullmatch(R + H, inner)):
      base, disp = G(g[1]), int(g[2], 16)
    elif (g := re.fullmatch(R, inner)):
      base = G(g[1])
    elif re.fullmatch(r"0x[0-9a-fA-F]+", inner):
      disp = int(inner, 16)
    else:
      return None
    s = {1: 0, 2: 1, 4: 2, 8: 3}.get(scale)
    if s is None:
      return None
    adr = 0
    if "adrsiz" in env and not isinstance(env["adrsiz"], str) and int(env["adrsiz"]) != 0:
      return None  # an adrsiz prefix was given -> this is a 16-bit operand

    # base==esp always needs a SIB; base==None with an index too
    need_sib = (index is not None) or (base == 4) or (base is None and disp is not None and index is not None)
    if base is None and index is None:
      # direct [disp32]: mod=00 rm=101
      if not self._set_seg(env, seg, 0):
        return None
      env["adrsiz"] = adr
      return {"mod": 0, "rm": 5, "sib": None, "disp": disp & 0xffffffff, "dw": 4}
    if index is not None and base is None:
      # [index*scale+disp32]: SIB base=101, mod=00
      if not self._set_seg(env, seg, 0):
        return None
      sib = (s << 6) | (index << 3) | 5
      env["adrsiz"] = adr
      return {"mod": 0, "rm": 4, "sib": sib, "disp": disp & 0xffffffff, "dw": 4}

    sbo = self.SBO[base]
    if not self._set_seg(env, seg, sbo):
      return None
    env["adrsiz"] = adr
    if not need_sib:
      # plain [base(+disp)]
      if disp is None:
        if base == 5:  # ebp can't be mod=00 -> disp8=0
          return {"mod": 1, "rm": 5, "sib": None, "disp": 0, "dw": 1}
        return {"mod": 0, "rm": base, "sib": None, "disp": None, "dw": 0}
      mod, dw = self._pick_disp(disp, wits)
      return {"mod": mod, "rm": base, "sib": None, "disp": disp & ((1 << (8 * dw)) - 1), "dw": dw}
    # SIB with a base present
    ib = index if index is not None else 4   # 100 = no index
    sib = (s << 6) | (ib << 3) | base
    if disp is None:
      if base == 5:  # ebp base in SIB needs a disp
        return {"mod": 1, "rm": 4, "sib": sib, "disp": 0, "dw": 1}
      return {"mod": 0, "rm": 4, "sib": sib, "disp": None, "dw": 0}
    mod, dw = self._pick_disp(disp, wits)
    return {"mod": mod, "rm": 4, "sib": sib, "disp": disp & ((1 << (8 * dw)) - 1), "dw": dw}

  def _enc16(self, inner, seg, wits, env):
    H = r"([+-]0x[0-9a-fA-F]+)"   # sgn()-style displacement
    if "adrsiz" in env and not isinstance(env["adrsiz"], str) and int(env["adrsiz"]) == 0 \
       and "__forced16__" not in env:
      pass  # adrsiz may still be revealed; only block if an opsiz-style pin exists
    base = disp = None
    for rmidx in sorted(range(8), key=lambda i: -len(self.RM16[i])):
      ent = self.RM16[rmidx]
      if inner == ent:
        base = rmidx
        break
      g = re.fullmatch(re.escape(ent) + H, inner)
      if g:
        base, disp = rmidx, int(g[1], 16)
        break
    if base is None:
      if re.fullmatch(r"0x[0-9a-fA-F]+", inner):  # direct [disp16]
        if not self._set_seg(env, seg, 0):
          return None
        env["adrsiz"] = 1
        return {"mod": 0, "rm": 6, "sib": None, "disp": int(inner, 16) & 0xffff, "dw": 2}
      return None
    sbo = self.SBO16[base]
    if not self._set_seg(env, seg, sbo):
      return None
    env["adrsiz"] = 1
    if disp is None:
      if base == 6:  # bp needs a disp
        return {"mod": 1, "rm": 6, "sib": None, "disp": 0, "dw": 1}
      return {"mod": 0, "rm": base, "sib": None, "disp": None, "dw": 0}
    if -128 <= disp <= 127:
      return {"mod": 1, "rm": base, "sib": None, "disp": disp & 0xff, "dw": 1}
    return {"mod": 2, "rm": base, "sib": None, "disp": disp & 0xffff, "dw": 2}

  # -- assemble one core string against insn rules -> list of (bytes, env) ----
  def asm_core(self, core, env0, start):
    cands = []
    for rule in self.insn.rules:
      try:
        for sol in rmatch(rule.template, core, dict(env0), self):
          if not self.guards_ok(rule, sol):
            continue
          e = Emit()
          self.emit_pattern(rule.pattern, sol, e, start)
          cands.append((e.bytes(), sol))
      except NoAsm:
        continue
    # VEX: the insn dispatch `0xc4 @vex` consumes the C4 lead byte, so the vex
    # rules' patterns start at the first VEX byte.  Reverse them the same way --
    # match the template, emit the C4 lead, then the rule's pattern.
    vex = self.g["submatches"].get("vex")
    if vex is not None:
      for rule in vex.rules:
        try:
          for sol in rmatch(rule.template, core, dict(env0), self):
            if not self.guards_ok(rule, sol):
              continue
            e = Emit()
            e.val(0xc4, 8)
            self.emit_pattern(rule.pattern, sol, e, start)
            cands.append((e.bytes(), sol))
        except NoAsm:
          continue
    # XOP: the insn dispatch `0x8f @xop` consumes the 8F lead byte exactly like
    # VEX's C4 -- match the template, emit the 8F lead, then the rule's pattern.
    xop = self.g["submatches"].get("xop")
    if xop is not None:
      for rule in xop.rules:
        try:
          for sol in rmatch(rule.template, core, dict(env0), self):
            if not self.guards_ok(rule, sol):
              continue
            e = Emit()
            e.val(0x8f, 8)
            self.emit_pattern(rule.pattern, sol, e, start)
            cands.append((e.bytes(), sol))
        except NoAsm:
          continue
    # C5 (2-byte VEX): the insn dispatch `0xc5 @vex2` consumes the C5 lead byte;
    # vex2 rule patterns start at the single C5 payload byte (R.vvvv.L.pp).
    vex2 = self.g["submatches"].get("vex2")
    if vex2 is not None:
      for rule in vex2.rules:
        try:
          for sol in rmatch(rule.template, core, dict(env0), self):
            if not self.guards_ok(rule, sol):
              continue
            e = Emit()
            e.val(0xc5, 8)
            self.emit_pattern(rule.pattern, sol, e, start)
            cands.append((e.bytes(), sol))
        except NoAsm:
          continue
    # EVEX (4-byte): the insn dispatch `0x62 @evex` consumes the 62 lead byte;
    # evex rule patterns start at P0 (R.X.B.R'.0.mmm).
    evex = self.g["submatches"].get("evex")
    if evex is not None:
      for rule in evex.rules:
        try:
          for sol in rmatch(rule.template, core, dict(env0), self):
            if not self.guards_ok(rule, sol):
              continue
            e = Emit()
            e.val(0x62, 8)
            self.emit_pattern(rule.pattern, sol, e, start)
            cands.append((e.bytes(), sol))
        except NoAsm:
          continue
    return cands

  def guards_ok(self, rule, env):
    for el in rule.pattern:
      if el[0] == "guard":
        try:
          if not _ev(subst_known(el[1], env)):
            return False
        except NoAsm:
          pass
    return True


def disasm_one(g, data, base=0):
  # disassemble exactly one instruction from data as if it were located at byte
  # offset `base` (so $E-relative targets decode to the same absolute address).
  ctx = pg.Ctx(g, bytes(base) + data)
  ctx.stream.bit = base * 8
  ctx.S = base
  ctx.vars = {k: g["vars"][k] for k in g["vars"]}
  ctx.events = []
  ctx.prefix_tokens = []
  ctx.witness = []
  ctx.disp_consumed = []
  ctx.insn_str = None
  try:
    pg.invoke("main", [], ctx)
  except pg.NoMatch:
    return (None, 0)
  return (pg.recover(ctx), ctx.stream.bytepos() - base)


def split_prefixes(line):
  ws = line.split(" ")
  toks, k = [], 0
  while k < len(ws) and ws[k] in PFX:
    toks.append(ws[k]); k += 1
  wits = []
  while k < len(ws) and ws[k] in WITNESS:
    wits.append(ws[k]); k += 1
  return toks, wits, " ".join(ws[k:])


def revealed_bytes(env, explicit):
  rev = bytearray()
  for var in REVEAL_ORDER:
    if var in explicit:
      continue
    v = int(env.get(var, 0))
    if v == 0:
      continue
    if var == "segidx":
      rev.append(SEG_BYTE[v])
    elif var == "adrsiz":
      rev.append(0x67)
    elif var == "opsiz":
      rev.append(0x66)
    elif var == "reptype":
      rev.append(0xf3 if v == 1 else 0xf2)
  return bytes(rev)


def asm_line(asm, line, start):
  toks, wits, core = split_prefixes(line)
  pfx = bytearray()
  explicit = {}
  for tk in toks:
    byte, var, val = PFX[tk]
    pfx.append(byte)
    explicit[var] = val       # last writer wins (matches the forward engine)
  # Only explicit-token prefix vars are pinned; revealed ones (opsiz from "ax",
  # reptype from "ss", segidx from "cs:", adrsiz from a 16-bit base) are left
  # free so the operand reverse-match can determine them.
  env0 = dict(explicit)
  env0["__wits__"] = list(wits)
  cands = asm.asm_core(core, env0, start + len(pfx))
  # Verify each structural candidate by disassembling it back; keep those that
  # reproduce the exact line, then take the shortest (the canonical encoding).
  best = None
  for ibytes, env in cands:
    # A structural candidate may carry an out-of-range revealed prefix var (the
    # multi-valued reverse-match explores more index solutions than the forward
    # engine ever emits); building/disassembling it can fail.  Such candidates
    # are simply not valid encodings -- skip them and let verification pick the
    # real one.
    try:
      full = bytes(pfx) + revealed_bytes(env, explicit) + ibytes
      txt, n = disasm_one(asm.g, full, start)
    except Exception:
      continue
    if n == len(full) and txt == line:
      if best is None or len(full) < len(best):
        best = full
  if best is None:
    raise NoAsm("no rule matches: %r" % line)
  return best

DESCRIPTION = r'''
# corpus.p (v3h1, full-coverage disassembly) -- bit-pattern -> string rewriter.
#
# Derived from corpus-v3h1.p.  That file shows "a representative set; all others
# alike"; this one fills in the remaining opcodes the image exercises, following
# the same conventions (size-paired register tables, mod=11 + @addr per ModR/M
# op, prefix-indexed mnemonic tables, inline segment display, automatic prefix
# recovery).  Two additions over the excerpt:
#   * sfx is widened so sfx[16]="" (xmmword / 128-bit carries no suffix);
#   * addr1 emits wit("disp8"/"disp32") for the non-canonical (redundant) disp
#     encodings, a hoisted annotation token the assembler reads back -- the
#     excerpt simplified these away.

arch  $mode=32 $endian=le $bitorder=msb $maxlen=15
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0

# ---- register / addressing tables : size-pairs merged, 16-bit in upper half --
table greg   { eax,ecx,edx,ebx,esp,ebp,esi,edi, ax,cx,dx,bx,sp,bp,si,di }
table rgb    { al,cl,dl,bl,ah,ch,dh,bh }
table ssereg { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7, xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7 }
table sreg   { es,cs,ss,ds,fs,gs }
table rm16   { bx+si,bx+di,bp+si,bp+di,si,di,bp,bx }
table cond   { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }

# ---- size suffix : the dot is part of the entry; index = operand size in bytes
# widened past the excerpt so 16 (xmmword) maps to "" and 10 to ".t".
table sfx    { "",".b",".w","",".d","","","",".q","",".t","","","","","","" }

# ---- segment display : index = sbo(base)+segidx.
table seg    { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo    { 0,0,0,0,7,7,0,0 }
table sbo16  { 0,0,7,7,0,0,7,0 }

# ---- prefix-indexed / op-indexed mnemonic tables --------------------------
table pcnt   { "",popcnt,"" }
table eqt    { "",extrq }
table iqt    { "","",insertq }
table cvt2a  { cvtpi2ps,cvtpi2pd,cvtsi2ss,cvtsi2ss,cvtsi2sd,cvtsi2sd }
table cvt2c  { cvttps2pi,cvttpd2pi,cvttss2si,cvttss2si,cvttsd2si,cvttsd2si }
table cvt2d  { cvtps2pi,cvtpd2pi,cvtss2si,cvtss2si,cvtsd2si,cvtsd2si }
table ldqt   { "","",lddqu }
table crct   { "","",crc32 }
table acxt   { "",adcx }
table adxt   { "",adox,"" }
table wrut   { "",wrussd }
table fct    { aadd,aand,axor,"",aor,"" }
table f8et   { "","",enqcmd }
table f8st   { "",enqcmds,"" }
table f8mt   { "",movdir64b }
table ek1t   { "",encodekey128,"" }
table ek2t   { "",encodekey256,"" }
table aesd8  { "","","","",aesencwide128kl,aesdecwide128kl,aesencwide256kl,aesdecwide256kl }
table i80t   { "",invept }
table i81t   { "",invvpid }
table i82t   { "",invpcid }
table hrest  { "",hreset,"" }
table g00    { sldt,str,lldt,ltr,verr,verw }
table crreg  { cr0,cr1,cr2,cr3,cr4,cr5,cr6,cr7 }
table drreg  { dr0,dr1,dr2,dr3,dr4,dr5,dr6,dr7 }
table c7r7   { rdseed,rdpid,rdseed,rdseed }   # 0F C7 /7 reg by reptype
table aer4   { ptw_,ptwrite,ptw_,ptw_ }        # 0F AE /4 reg (F3) ptwrite
table bndreg { bnd0,bnd1,bnd2,bnd3 }
table kreg   { k0,k1,k2,k3,k4,k5,k6,k7 }
table zreg   { xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7, ymm0,ymm1,ymm2,ymm3,ymm4,ymm5,ymm6,ymm7, zmm0,zmm1,zmm2,zmm3,zmm4,zmm5,zmm6,zmm7 }
table kdec   { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}" }
table kzdec  { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}", "", " {k1} {z}", " {k2} {z}", " {k3} {z}", " {k4} {z}", " {k5} {z}", " {k6} {z}", " {k7} {z}" }
table bcst16  { " {1to8}", " {1to16}", " {1to32}", "" }
table bcst32  { " {1to4}", " {1to8}", " {1to16}", "" }
table bcst64  { " {1to2}", " {1to4}", " {1to8}", "" }
table rcdec  { " {rn-sae}", " {rd-sae}", " {ru-sae}", " {rz-sae}" }
table vscale { "1", "2", "4", "8" }
table sc     { 1, 2, 4, 8 }
table vsse   { xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,ymm0,ymm1,ymm2,ymm3,ymm4,ymm5,ymm6,ymm7 }
table m1a    { bndldx,bndmov,bndcl,bndcl,bndcu,bndcu }   # 0F 1A reptype*2+opsiz
table m1b    { bndstx,bndmov,bndmk,bndmk,bndcn,bndcn }   # 0F 1B reptype*2+opsiz
table e1efa  { "",endbr64,"" }     # F3 0F 1E FA
table e1efb  { "",endbr32,"" }     # F3 0F 1E FB
table rdss   { "",rdsspd,"" }      # F3 0F 1E /1
table e8t    { serialize,setssbsy,xsusldtrk }  # 0F 01 E8 by reptype
table e9t    { "","",xresldtrk }   # 0F 01 E9
table eat    { "",saveprevssp,"" } # 0F 01 EA
table fat    { monitorx,mcommit,"" }   # 0F 01 FA
table fft   { tlbsync,"",pvalidate }  # 0F 01 FF
table cct    { "",tdcall }         # 0F 01 CC by opsiz
table rsst   { "",rstorssp,"" }    # F3 0F 01 /5           # 0F B8 F3=popcnt
table m10    { movups,movss,movsd }     # 0F 10 by reptype
table m10sz  { 16,4,8 }
table m54    { andps,andpd }            # 0F 54 by opsiz
table uc     { ucomiss,ucomisd }        # 0F 2E by opsiz
table comi   { comiss,comisd }          # 0F 2F by opsiz
table movs   { movsd,movsw }            # 0xA5 by opsiz
table stos   { stosd,stosw }            # 0xAB by opsiz
table lods   { lodsd,lodsw }            # 0xAD by opsiz
table cmps   { cmpsd,cmpsw }            # 0xA7 by opsiz
table scas   { scasd,scasw }            # 0xAF by opsiz
table insx   { insd,insw }              # 0x6D by opsiz
table outsx  { outsd,outsw }            # 0x6F by opsiz
table cdqw   { cdq,cwd }                # 0x99 by opsiz
table cbwe   { cwde,cbw }               # 0x98 by opsiz
table d8r    { fadd,fmul,fcom,fcomp,fsub,fsubr,fdiv,fdivr }   # D8 reg by /digit
table dcr    { fadd,fmul,fcom,fcomp,fsubr,fsub,fdivr,fdiv }   # DC reg by /digit
table dar    { fcmovb,fcmove,fcmovbe,fcmovu }                 # DA reg /0-3
table dbr    { fcmovnb,fcmovne,fcmovnbe,fcmovnu }             # DB reg /0-3
table der    { faddp,fmulp,fcompp,fcompp,fsubrp,fsubp,fdivrp,fdivp } # DE reg by /digit
table elt    { ps,pd,ss,ss,sd,sd }      # ssef element by reptype*2+opsiz
table kl_dc  { "",aesenc128kl,"" }
table kl_dd  { "",aesdec128kl,"" }
table kl_de  { "",aesenc256kl,"" }
table kl_df  { "",aesdec256kl,"" }
table lzt    { "",lzcnt,"" }
table tzt    { "",tzcnt,"" }
table cvt5a  { cvtps2pd,cvtpd2ps,cvtss2sd,cvtss2sd,cvtsd2ss,cvtsd2ss } # 0F 5A by reptype*2+opsiz
table cvt5b  { cvtdq2ps,cvtps2dq,cvttps2dq,cvttps2dq } # 0F 5B by reptype*2+opsiz
table pshuf  { pshufw,pshufd,pshufhw,pshufhw,pshuflw,pshuflw }
table m7e    { movd,movq }              # 0F 7E by reptype
table movu   { movups,movupd,movss,movss,movsd,movsd } # 0F 10/11 reptype*2+opsiz
table movusz { 16,16,4,4,8,8 }         # 0F 10/11 mem size by reptype*2+opsiz
table mova2  { movaps,movapd }         # 0F 28/29 by opsiz
table mnt2   { movntps,movntpd }       # 0F 2B by opsiz
table mnt4   { movntps,movntpd,movntss,movntss,movntsd,movntsd } # 0F 2B reptype*2+opsiz
table liwk   { "",loadiwkey,"" }
table pause_t { "",pause,"" }
table unpl   { unpcklps,unpcklpd }     # 0F 14 by opsiz
table unph   { unpckhps,unpckhpd }     # 0F 15 by opsiz
table mdq    { movq,movdqa,movdqu }    # 0F 6F/7F reptype*2+opsiz
table ml12   { movlps,movlpd }         # 0F 12/13 mem by opsiz
table mh16   { movhps,movhpd }         # 0F 16/17 mem by opsiz
table m12f   { "",movsldup,movddup }   # 0F 12 by reptype (F3/F2)
table d0t    { "",addsubpd,"","",addsubps,"" } # 0F D0 reptype*2+opsiz
table h7c    { "",haddpd,"","",haddps,"" }      # 0F 7C
table h7d    { "",hsubpd,"","",hsubps,"" }      # 0F 7D
table md6    { "",movq2dq,movdq2q }             # 0F D6 reptype F3/F2
table me6    { "",cvttpd2dq,cvtdq2pd,"",cvtpd2dq,"" } # 0F E6 reptype*2+opsiz
table me7    { movntq,movntdq }                 # 0F E7 by opsiz
table mf7    { maskmovq,maskmovdqu }            # 0F F7 by opsiz
table m16f   { "",movshdup,"" }        # 0F 16 by reptype (F3)
table alu    { add,or,adc,sbb,and,sub,xor,cmp }   # 00..3D group by op field
table shift  { rol,ror,rcl,rcr,shl,shr,shl,sar }  # C0/C1/D0..D3 by /op
table grp3   { test,test,not,neg,mul,imul,div,idiv } # F6/F7 by /op
table grpba  { "","","","",bt,bts,btr,btc }       # 0F BA by /op

# ---- immediates / displacements : the width-dispatch <...> matches the arg ----
submatch imm8  { iiiiiiii                              => $i }
submatch imm8b { iiiiiiii                              => $i }
submatch imm16 { iiiiiiii iiiiiiii                     => $i }
submatch imm32 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii   => $i }
submatch disp8 { dddddddd                              => sx8($d) }
submatch disp16{ dddddddd dddddddd                     => sx16($d) }
submatch disp32{ dddddddd dddddddd dddddddd dddddddd   => $d }
submatch rel8  { dddddddd                              => $E + sx8($d) }
submatch immz1($opsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch relz1($opsiz) { <0> @imm32 => $E+sx32($imm32) ;  <1> @imm16 => $E+sx16($imm16) }
submatch immz { @immz1($opsiz) => $immz1 }
submatch immadr1($adrsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch immadr { @immadr1($adrsiz) => $immadr1 }
submatch relz { @relz1($opsiz) => $relz1 }

# ---- prefix run : depth-bounded; each frame just prepends its token ----------
submatch pfx($d) {
  [$d >= $maxlen] => "" ;
  0x66 @pfx($d+1) {$opsiz=1}   => "opsiz "  $pfx ;
  0x67 @pfx($d+1) {$adrsiz=1}  => "adrsiz " $pfx ;
  0x26 @pfx($d+1) {$segidx=1}  => "seges "  $pfx ;
  0x2e @pfx($d+1) {$segidx=2}  => "segcs "  $pfx ;
  0x36 @pfx($d+1) {$segidx=3}  => "segss "  $pfx ;
  0x3e @pfx($d+1) {$segidx=4}  => "segds "  $pfx ;
  0x64 @pfx($d+1) {$segidx=5}  => "segfs "  $pfx ;
  0x65 @pfx($d+1) {$segidx=6}  => "seggs "  $pfx ;
  0xf3 @pfx($d+1) {$reptype=1} => "rep "    $pfx ;
  0xf2 @pfx($d+1) {$reptype=2} => "repnz "  $pfx ;
  0xf0 @pfx($d+1) {$lock=1}    => "lock "   $pfx ;
       @insn((($adrsiz*2+$opsiz)*4+$reptype)) => $insn ;
}

# ---- addressing : MEMORY ONLY (mod != 11). addr() injects current adrsiz. ----
# wit() flags a redundant displacement so the assembler can reproduce the bytes.
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }
submatch addr1($adrsiz, $g) {
  <0> 00 ggg 100 @sib0         => seg[sbo[$sbase]+$segidx] $sib0 ;
  <0> 00 ggg 101 @disp32       => seg[$segidx] "[" hex($disp32) "]" ;
  <0> 00 ggg rrr               => seg[sbo[$r]+$segidx] "[" greg[$r] "]" ;
  <0> 01 ggg 100 @sib1 @disp8  => wit($disp8==0 ? ($sbase!=5 ? "disp8" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <0> 01 ggg rrr @disp8        => wit($disp8==0 ? ($r!=5 ? "disp8" : "") : "") seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp8) "]" ;
  <0> 10 ggg 100 @sib1 @disp32 => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <0> 10 ggg rrr @disp32       => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp32) "]" ;
  <1> 00 ggg 110 @disp16       => seg[$segidx] "[" hex($disp16) "]" ;
  <1> 00 ggg rrr               => seg[sbo16[$r]+$segidx] "[" rm16[$r] "]" ;
  <1> 01 ggg rrr @disp8        => seg[sbo16[$r]+$segidx] "[" rm16[$r] sgn($disp8) "]" ;
  <1> 10 ggg rrr @disp16       => seg[sbo16[$r]+$segidx] "[" rm16[$r] sgn($disp16) "]" ;
}
submatch sib0 {
  ss 100 101 @disp32 {$sbase=none} => "[" hex($disp32) "]" ;
  ss iii 101 @disp32 {$sbase=none} => "[" greg[$i] "*" dec(1<<$s) "+" hex($disp32) "]" ;
  ss 100 bbb         {$sbase=$b}   => "[" greg[$b] "]" ;
  ss iii bbb         {$sbase=$b}   => "[" greg[$b] "+" greg[$i] "*" dec(1<<$s) "]" ;
}
submatch sib1 {
  ss 100 bbb {$sbase=$b} => greg[$b] ;
  ss iii bbb {$sbase=$b} => greg[$b] "+" greg[$i] "*" dec(1<<$s) ;
}

# ============================ instruction decoder ===========================
submatch insn($sel) {
  # --- two-byte (0F) opcodes ------------------------------------------------
  0xc4 @vex => $vex ;
  # ===== 3DNow! (0F 0F) -- opcode is the SUFFIX byte after the operands =====
  0x0f 0x0f 11 ggg rrr 0x0c => "pi2fw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x0c => "pi2fw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x0d => "pi2fd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x0d => "pi2fd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x1c => "pf2iw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x1c => "pf2iw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x1d => "pf2id " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x1d => "pf2id " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x8a => "pfnacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x8a => "pfnacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x8e => "pfpnacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x8e => "pfpnacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x90 => "pfcmpge " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x90 => "pfcmpge " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x94 => "pfmin " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x94 => "pfmin " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x96 => "pfrcp " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x96 => "pfrcp " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x97 => "pfsqrt " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x97 => "pfsqrt " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x9a => "pfsub " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x9a => "pfsub " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0x9e => "pfadd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0x9e => "pfadd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xa0 => "pfcmpgt " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xa0 => "pfcmpgt " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xa4 => "pfmax " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xa4 => "pfmax " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xa6 => "pfcpit1 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xa6 => "pfcpit1 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xa7 => "pfrsqit1 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xa7 => "pfrsqit1 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xaa => "pfsubr " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xaa => "pfsubr " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xae => "pfacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xae => "pfacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xb0 => "pfcmpeq " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xb0 => "pfcmpeq " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xb4 => "pfmul " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xb4 => "pfmul " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xb6 => "pfrcpit2 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xb6 => "pfrcpit2 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xb7 => "pmulhrw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xb7 => "pmulhrw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xbb => "pswapd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xbb => "pswapd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr 0xbf => "pavgusb " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr 0xbf => "pavgusb " ssereg[$g] "," $addr ;
  # --- system / 0F 00,01,02,03,05-0E,18-23,0D ---
  0x0f 0x00 11 000 rrr => "sldt " greg[$opsiz*8+$r] ;
  0x0f 0x00 11 001 rrr => "str " greg[$opsiz*8+$r] ;
  0x0f 0x00 11 010 rrr => "lldt " greg[8+$r] ;
  0x0f 0x00 11 011 rrr => "ltr " greg[8+$r] ;
  0x0f 0x00 11 100 rrr => "verr " greg[8+$r] ;
  0x0f 0x00 11 101 rrr => "verw " greg[8+$r] ;
  0x0f 0x00 @addr(0) => "sldt " $addr ;
  0x0f 0x00 @addr(1) => "str " $addr ;
  0x0f 0x00 @addr(2) => "lldt " $addr ;
  0x0f 0x00 @addr(3) => "ltr " $addr ;
  0x0f 0x00 @addr(4) => "verr " $addr ;
  0x0f 0x00 @addr(5) => "verw " $addr ;
  0x0f 0x01 @addr(0) => "sgdt " $addr ;
  0x0f 0x01 @addr(1) => "sidt " $addr ;
  0x0f 0x01 @addr(2) => "lgdt " $addr ;
  0x0f 0x01 @addr(3) => "lidt " $addr ;
  0x0f 0x01 @addr(4) => "smsw " $addr ;
  0x0f 0x01 @addr(6) => "lmsw " $addr ;
  0x0f 0x01 @addr(7) => "invlpg " $addr ;
  0x0f 0x01 @addr(5) [$reptype==1] => rsst[$reptype] " " $addr ;
  0x0f 0x01 11 100 rrr => "smsw " greg[$opsiz*8+$r] ;
  0x0f 0x01 11 110 rrr => "lmsw " greg[8+$r] ;
  0x0f 0x01 0xe8 => e8t[$reptype] ;
  0x0f 0x01 0xe9 => e9t[$reptype] ;
  0x0f 0x01 0xea => eat[$reptype] ;
  0x0f 0x01 0xfa => fat[$reptype] ;
  0x0f 0x01 0xff [$reptype==2] => fft[$reptype] " eax,ecx,edx" ;
  0x0f 0x01 0xff => "tlbsync" ;
  0x0f 0x01 0xcc => cct[$opsiz] ;
  0x0f 0x01 0xc0 => "enclv" ;
  0x0f 0x01 0xc1 => "vmcall" ;
  0x0f 0x01 0xc2 => "vmlaunch" ;
  0x0f 0x01 0xc3 => "vmresume" ;
  0x0f 0x01 0xc4 => "vmxoff" ;
  0x0f 0x01 0xc5 => "pconfig" ;
  0x0f 0x01 0xc6 => "wrmsrns" ;
  0x0f 0x01 0xc8 => "monitor" ;
  0x0f 0x01 0xc9 => "mwait" ;
  0x0f 0x01 0xca => "clac" ;
  0x0f 0x01 0xcb => "stac" ;
  0x0f 0x01 0xcf => "encls" ;
  0x0f 0x01 0xd0 => "xgetbv" ;
  0x0f 0x01 0xd1 => "xsetbv" ;
  0x0f 0x01 0xd4 => "vmfunc" ;
  0x0f 0x01 0xd5 => "xend" ;
  0x0f 0x01 0xd6 => "xtest" ;
  0x0f 0x01 0xd7 => "enclu" ;
  0x0f 0x01 0xd8 => "vmrun eax" ;
  0x0f 0x01 0xd9 => "vmmcall" ;
  0x0f 0x01 0xda => "vmload eax" ;
  0x0f 0x01 0xdb => "vmsave eax" ;
  0x0f 0x01 0xdc => "stgi" ;
  0x0f 0x01 0xdd => "clgi" ;
  0x0f 0x01 0xde => "skinit eax" ;
  0x0f 0x01 0xdf => "invlpga eax, ecx" ;
  0x0f 0x01 0xee => "rdpkru" ;
  0x0f 0x01 0xef => "wrpkru" ;
  0x0f 0x01 0xf9 => "rdtscp" ;
  0x0f 0x01 0xfb => "mwaitx" ;
  0x0f 0x01 0xfc => "clzero eax" ;
  0x0f 0x01 0xfd => "rdpru" ;
  0x0f 0x01 0xfe => "invlpgb eax, edx, ecx" ;
  0x0f 0x20 11 ggg rrr => "mov " greg[$r] "," crreg[$g] ;
  0x0f 0x22 11 ggg rrr => "mov " crreg[$g] "," greg[$r] ;
  0x0f 0x21 11 ggg rrr => "mov " greg[$r] "," drreg[$g] ;
  0x0f 0x23 11 ggg rrr => "mov " drreg[$g] "," greg[$r] ;
  0x0f 0x20 00 ggg rrr => "mov " greg[$r] "," crreg[$g] wit("modz") ;
  0x0f 0x22 00 ggg rrr => "mov " crreg[$g] "," greg[$r] wit("modz") ;
  0x0f 0x21 00 ggg rrr => "mov " greg[$r] "," drreg[$g] wit("modz") ;
  0x0f 0x23 00 ggg rrr => "mov " drreg[$g] "," greg[$r] wit("modz") ;
  0x0f 0x02 11 ggg rrr => "lar " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x02 @addr      => "lar " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x03 11 ggg rrr => "lsl " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x03 @addr      => "lsl " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x05 => "syscall" ;
  0x0f 0x06 => "clts" ;
  0x0f 0x07 => "sysret" ;
  0x0f 0x08 => "invd" ;
  0x0f 0x09 => "wbinvd" ;
  0x0f 0x0e => "femms" ;
  0x0f 0x18 @addr(0) => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2) => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3) => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0x18 @addr(6) => "prefetchit1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(7) => "prefetchit0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(4) => "nop " $addr ;
  0x0f 0x18 @addr(5) => "nop18 " $addr ;
  0x0f 0x18 11 ggg rrr => "nop18 " greg[$r] "," greg[$g] ;
  0x0f 0x0d @addr(0) => "prefetch" sfx[1] " " $addr ;
  0x0f 0x0d @addr(1) => "prefetchw" sfx[1] " " $addr ;
  0x0f 0x0d @addr(2) => "prefetchwt1" sfx[1] " " $addr ;
  0x0f 0x0d @addr(3) => "prefetch3" sfx[1] " " $addr ;
  0x0f 0x0d @addr(4) => "prefetch4" sfx[1] " " $addr ;
  0x0f 0x0d @addr(5) => "prefetch5" sfx[1] " " $addr ;
  0x0f 0x0d @addr(6) => "prefetch6" sfx[1] " " $addr ;
  0x0f 0x0d @addr(7) => "prefetch7" sfx[1] " " $addr ;
  0x0f 0x0d 11 ggg rrr => "nop0d " greg[$r] "," greg[$g] ;
  0x0f 0x1a @addr => m1a[$reptype*2+$opsiz] " " bndreg[$g] "," $addr ;
  0x0f 0x1a 11 ggg rrr [$opsiz] => m1a[$reptype*2+$opsiz] " " bndreg[$g] "," bndreg[$r] ;
  0x0f 0x1a 11 ggg rrr [$reptype] => m1a[$reptype*2+$opsiz] " " bndreg[$g] "," greg[$r] ;
  0x0f 0x1a 11 000 001 => "nop eax" ;
  0x0f 0x1b 11 ggg rrr [$opsiz] => m1b[$reptype*2+$opsiz] " " bndreg[$r] "," bndreg[$g] wit("alt") ;
  0x0f 0x1b 11 ggg rrr [$reptype==2] => m1b[$reptype*2+$opsiz] " " bndreg[$g] "," greg[$r] ;
  0x0f 0x1b @addr [$opsiz] => m1b[$reptype*2+$opsiz] " " $addr "," bndreg[$g] ;
  0x0f 0x1b @addr [$reptype] => m1b[$reptype*2+$opsiz] " " bndreg[$g] "," $addr ;
  0x0f 0x1b @addr => m1b[$reptype*2+$opsiz] " " $addr "," bndreg[$g] ;
  0x0f 0x1b 11 000 001 => "nop eax" wit("alt") ;
  0x0f 0x1e 0xfa [$reptype==1] => e1efa[$reptype] ;
  0x0f 0x1e 0xfb [$reptype==1] => e1efb[$reptype] ;
  0x0f 0x1e 11 001 rrr [$reptype==1] => rdss[$reptype] " " greg[$r] ;
  0x0f 0x1e 11 ggg rrr => "nop " greg[$r] "," greg[$g] wit($reptype==0 ? "imm32" : "") ;
  0x0f 0x1e @addr      => "nop " $addr "," greg[$g] wit($reptype==0 ? "imm32" : "") ;
  0x0f 0x1f 11 ggg rrr => "nop " greg[$r] "," greg[$g] ;
  0x0f 0x1f @addr      => "nop " $addr "," greg[$g] ;
  0x0f 0x19 11 ggg rrr => "nop19 " greg[$r] "," greg[$g] ;
  0x0f 0x19 @addr      => "nop19 " $addr "," greg[$g] ;
  0x0f 0x1d 11 ggg rrr => "nop1d " greg[$r] "," greg[$g] ;
  0x0f 0x1d @addr      => "nop1d " $addr "," greg[$g] ;
  0x0f 0x1c @addr(0) => "cldemote " $addr ;
  0x0f 0x1c 11 ggg rrr => "nop1c " greg[$r] "," greg[$g] ;
  0x0f 0x1c @addr      => "nop1c " $addr "," greg[$g] ;
  0x0f 0x2a 11 ggg rrr [$reptype] => cvt2a[$reptype*2+$opsiz] " " ssereg[8+$g] "," greg[$r] ;
  0x0f 0x2a 11 ggg rrr => cvt2a[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[$r] ;
  0x0f 0x2c 11 ggg rrr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2c 11 ggg rrr => cvt2c[$reptype*2+$opsiz] " " ssereg[$g] "," ssereg[8+$r] ;
  0x0f 0x2d 11 ggg rrr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2d 11 ggg rrr => cvt2d[$reptype*2+$opsiz] " " ssereg[$g] "," ssereg[8+$r] ;
  0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2e @addr      => uc[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => comi[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  # --- 0F 3A three-byte (SSE4.1 imm8 ops) ---
  0x0f 0x3a 0x08 11 ggg rrr @imm8 => "roundps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x08 @addr      @imm8 => "roundps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x09 11 ggg rrr @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x09 @addr      @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0a 11 ggg rrr @imm8 => "roundss " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0a @addr      @imm8 => "roundss " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0b 11 ggg rrr @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0b @addr      @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0c 11 ggg rrr @imm8 => "blendps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0c @addr      @imm8 => "blendps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0d 11 ggg rrr @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0d @addr      @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0e 11 ggg rrr @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0e @addr      @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x40 11 ggg rrr @imm8 => "dpps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x40 @addr      @imm8 => "dpps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x41 11 ggg rrr @imm8 => "dppd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x41 @addr      @imm8 => "dppd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x42 11 ggg rrr @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x42 @addr      @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x44 11 ggg rrr @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x44 @addr      @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x60 11 ggg rrr @imm8 => "pcmpestrm " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x60 @addr      @imm8 => "pcmpestrm " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x61 11 ggg rrr @imm8 => "pcmpestri " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x61 @addr      @imm8 => "pcmpestri " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x62 11 ggg rrr @imm8 => "pcmpistrm " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x62 @addr      @imm8 => "pcmpistrm " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x63 11 ggg rrr @imm8 => "pcmpistri " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x63 @addr      @imm8 => "pcmpistri " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xdf 11 ggg rrr @imm8 => "aeskeygenassist " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xdf @addr      @imm8 => "aeskeygenassist " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) wit("long") ;
  0x0f 0x3a 0x15 @addr      @imm8 => "pextrw " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => "pextrd " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => "pextrd " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0f 11 ggg rrr @imm8 => "palignr " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0f @addr      @imm8 => "palignr " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " ssereg[8+$g] "," $addr "," hex($imm8) ;
  # --- x87 FPU (D8-DF) ---
  0xd8 @addr(0)   => "fadd.d " $addr ;
  0xd8 @addr(1)   => "fmul.d " $addr ;
  0xd8 @addr(2)   => "fcom.d " $addr ;
  0xd8 @addr(3)   => "fcomp.d " $addr ;
  0xd8 @addr(4)   => "fsub.d " $addr ;
  0xd8 @addr(5)   => "fsubr.d " $addr ;
  0xd8 @addr(6)   => "fdiv.d " $addr ;
  0xd8 @addr(7)   => "fdivr.d " $addr ;
  0xd9 @addr(0)   => "fld.d " $addr ;
  0xd9 @addr(2)   => "fst.d " $addr ;
  0xd9 @addr(3)   => "fstp.d " $addr ;
  0xd9 @addr(4)   => "fldenv " $addr ;
  0xd9 @addr(5)   => "fldcw.w " $addr ;
  0xd9 @addr(6)   => "fnstenv " $addr ;
  0xd9 @addr(7)   => "fnstcw.w " $addr ;
  0xda @addr(0)   => "fiadd.d " $addr ;
  0xda @addr(1)   => "fimul.d " $addr ;
  0xda @addr(2)   => "ficom.d " $addr ;
  0xda @addr(3)   => "ficomp.d " $addr ;
  0xda @addr(4)   => "fisub.d " $addr ;
  0xda @addr(5)   => "fisubr.d " $addr ;
  0xda @addr(6)   => "fidiv.d " $addr ;
  0xda @addr(7)   => "fidivr.d " $addr ;
  0xdb @addr(0)   => "fild.d " $addr ;
  0xdb @addr(1)   => "fisttp.d " $addr ;
  0xdb @addr(2)   => "fist.d " $addr ;
  0xdb @addr(3)   => "fistp.d " $addr ;
  0xdb @addr(5)   => "fld.t " $addr ;
  0xdb @addr(7)   => "fstp.t " $addr ;
  0xdc @addr(0)   => "fadd.q " $addr ;
  0xdc @addr(1)   => "fmul.q " $addr ;
  0xdc @addr(2)   => "fcom.q " $addr ;
  0xdc @addr(3)   => "fcomp.q " $addr ;
  0xdc @addr(4)   => "fsub.q " $addr ;
  0xdc @addr(5)   => "fsubr.q " $addr ;
  0xdc @addr(6)   => "fdiv.q " $addr ;
  0xdc @addr(7)   => "fdivr.q " $addr ;
  0xdd @addr(0)   => "fld.q " $addr ;
  0xdd @addr(1)   => "fisttp.q " $addr ;
  0xdd @addr(2)   => "fst.q " $addr ;
  0xdd @addr(3)   => "fstp.q " $addr ;
  0xdd @addr(4)   => "frstor " $addr ;
  0xdd @addr(6)   => "fnsave " $addr ;
  0xdd @addr(7)   => "fnstsw " $addr ;
  0xde @addr(0)   => "fiadd.w " $addr ;
  0xde @addr(1)   => "fimul.w " $addr ;
  0xde @addr(2)   => "ficom.w " $addr ;
  0xde @addr(3)   => "ficomp.w " $addr ;
  0xde @addr(4)   => "fisub.w " $addr ;
  0xde @addr(5)   => "fisubr.w " $addr ;
  0xde @addr(6)   => "fidiv.w " $addr ;
  0xde @addr(7)   => "fidivr.w " $addr ;
  0xdf @addr(0)   => "fild.w " $addr ;
  0xdf @addr(1)   => "fisttp.w " $addr ;
  0xdf @addr(2)   => "fist.w " $addr ;
  0xdf @addr(3)   => "fistp.w " $addr ;
  0xdf @addr(4)   => "fbld.t " $addr ;
  0xdf @addr(5)   => "fild.q " $addr ;
  0xdf @addr(6)   => "fbstp.t " $addr ;
  0xdf @addr(7)   => "fistp.q " $addr ;
  # D8 reg
  0xd8 11 010 rrr => "fcom st(" dec($r) ")" ;
  0xd8 11 011 rrr => "fcomp st(" dec($r) ")" ;
  0xd8 11 ddd rrr => d8r[$d] " st(0),st(" dec($r) ")" ;
  # D9 reg
  0xd9 11 000 rrr => "fld st(" dec($r) ")" ;
  0xd9 11 001 rrr => "fxch st(" dec($r) ")" ;
  0xd9 11 011 rrr => "fstpnce st(" dec($r) ")" ;
  0xd9 0xd0 => "fnop" ;
  0xd9 0xe0 => "fchs" ;
  0xd9 0xe1 => "fabs" ;
  0xd9 0xe4 => "ftst" ;
  0xd9 0xe5 => "fxam" ;
  0xd9 0xe8 => "fld1" ;
  0xd9 0xe9 => "fldl2t" ;
  0xd9 0xea => "fldl2e" ;
  0xd9 0xeb => "fldpi" ;
  0xd9 0xec => "fldlg2" ;
  0xd9 0xed => "fldln2" ;
  0xd9 0xee => "fldz" ;
  0xd9 0xf0 => "f2xm1" ;
  0xd9 0xf1 => "fyl2x" ;
  0xd9 0xf2 => "fptan" ;
  0xd9 0xf3 => "fpatan" ;
  0xd9 0xf4 => "fxtract" ;
  0xd9 0xf5 => "fprem1" ;
  0xd9 0xf6 => "fdecstp" ;
  0xd9 0xf7 => "fincstp" ;
  0xd9 0xf8 => "fprem" ;
  0xd9 0xf9 => "fyl2xp1" ;
  0xd9 0xfa => "fsqrt" ;
  0xd9 0xfb => "fsincos" ;
  0xd9 0xfc => "frndint" ;
  0xd9 0xfd => "fscale" ;
  0xd9 0xfe => "fsin" ;
  0xd9 0xff => "fcos" ;
  # DA reg
  0xda 0xe9 => "fucompp" ;
  0xda 11 0dd rrr => dar[$d] " st(0),st(" dec($r) ")" ;
  # DB reg
  0xdb 0xe0 => "feni8087_nop" ;
  0xdb 0xe1 => "fdisi8087_nop" ;
  0xdb 0xe2 => "fnclex" ;
  0xdb 0xe3 => "fninit" ;
  0xdb 0xe4 => "fsetpm287_nop" ;
  0xdb 11 0dd rrr => dbr[$d] " st(0),st(" dec($r) ")" ;
  0xdb 11 101 rrr => "fucomi st(0),st(" dec($r) ")" ;
  0xdb 11 110 rrr => "fcomi st(0),st(" dec($r) ")" ;
  # DC reg
  0xdc 11 010 rrr => "fcom st(" dec($r) ")" wit("alt") ;
  0xdc 11 011 rrr => "fcomp st(" dec($r) ")" wit("alt") ;
  0xdc 11 ddd rrr => dcr[$d] " st(" dec($r) "),st(0)" wit($r==0 ? "alt" : "") ;
  # DD reg
  0xdd 11 000 rrr => "ffree st(" dec($r) ")" ;
  0xdd 11 001 rrr => "fxch st(" dec($r) ")" wit("alt") ;
  0xdd 11 010 rrr => "fst st(" dec($r) ")" ;
  0xdd 11 011 rrr => "fstp st(" dec($r) ")" ;
  0xdd 11 100 rrr => "fucom st(" dec($r) ")" ;
  0xdd 11 101 rrr => "fucomp st(0),st(" dec($r) ")" ;
  # DE reg
  0xde 0xd9 => "fcompp" ;
  0xde 11 010 rrr => "fcomp st(" dec($r) ")" wit("long") ;
  0xde 11 ddd rrr => der[$d] " st(" dec($r) "),st(0)" ;
  # DF reg
  0xdf 0xe0 => "fnstsw" ;
  0xdf 11 000 rrr => "ffreep st(" dec($r) ")" ;
  0xdf 11 001 rrr => "fxch st(" dec($r) ")" wit("long") ;
  0xdf 11 010 rrr => "fstp st(" dec($r) ")" wit("alt") ;
  0xdf 11 011 rrr => "fstp st(" dec($r) ")" wit("long") ;
  0xdf 11 101 rrr => "fucomip st(0),st(" dec($r) ")" ;
  0xdf 11 110 rrr => "fcomip st(0),st(" dec($r) ")" ;
  # --- 0F 38 three-byte (SSSE3 / SSE4 / AES / SHA) ---
  0x0f 0x38 0x00 11 ggg rrr => "pshufb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x00 @addr      => "pshufb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x01 11 ggg rrr => "phaddw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x01 @addr      => "phaddw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x02 11 ggg rrr => "phaddd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x02 @addr      => "phaddd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x03 11 ggg rrr => "phaddsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x03 @addr      => "phaddsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x04 11 ggg rrr => "pmaddubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x04 @addr      => "pmaddubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x05 11 ggg rrr => "phsubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x05 @addr      => "phsubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x06 11 ggg rrr => "phsubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x06 @addr      => "phsubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x07 11 ggg rrr => "phsubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x07 @addr      => "phsubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x08 11 ggg rrr => "psignb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x08 @addr      => "psignb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x09 11 ggg rrr => "psignw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x09 @addr      => "psignw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0a 11 ggg rrr => "psignd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0a @addr      => "psignd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0b 11 ggg rrr => "pmulhrsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0b @addr      => "pmulhrsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1c 11 ggg rrr => "pabsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1c @addr      => "pabsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1d 11 ggg rrr => "pabsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1d @addr      => "pabsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1e 11 ggg rrr => "pabsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1e @addr      => "pabsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x10 11 ggg rrr => "pblendvb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x10 @addr      => "pblendvb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x14 11 ggg rrr => "blendvps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x14 @addr      => "blendvps " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x15 11 ggg rrr => "blendvpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x15 @addr      => "blendvpd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x17 11 ggg rrr => "ptest " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x17 @addr      => "ptest " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x20 11 ggg rrr => "pmovsxbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x20 @addr      => "pmovsxbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x21 11 ggg rrr => "pmovsxbd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x21 @addr      => "pmovsxbd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x22 11 ggg rrr => "pmovsxbq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x22 @addr      => "pmovsxbq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x23 11 ggg rrr => "pmovsxwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x23 @addr      => "pmovsxwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x24 11 ggg rrr => "pmovsxwq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x24 @addr      => "pmovsxwq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x25 11 ggg rrr => "pmovsxdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x25 @addr      => "pmovsxdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x28 11 ggg rrr => "pmuldq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x28 @addr      => "pmuldq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x29 11 ggg rrr => "pcmpeqq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x29 @addr      => "pcmpeqq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x2b 11 ggg rrr => "packusdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x2b @addr      => "packusdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x30 11 ggg rrr => "pmovzxbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x30 @addr      => "pmovzxbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x31 11 ggg rrr => "pmovzxbd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x31 @addr      => "pmovzxbd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x32 11 ggg rrr => "pmovzxbq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x32 @addr      => "pmovzxbq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x33 11 ggg rrr => "pmovzxwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x33 @addr      => "pmovzxwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x34 11 ggg rrr => "pmovzxwq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x34 @addr      => "pmovzxwq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x35 11 ggg rrr => "pmovzxdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x35 @addr      => "pmovzxdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x37 11 ggg rrr => "pcmpgtq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x37 @addr      => "pcmpgtq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x38 11 ggg rrr => "pminsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x38 @addr      => "pminsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x39 11 ggg rrr => "pminsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x39 @addr      => "pminsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3a 11 ggg rrr => "pminuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3a @addr      => "pminuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3b 11 ggg rrr => "pminud " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3b @addr      => "pminud " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3c 11 ggg rrr => "pmaxsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3c @addr      => "pmaxsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3d 11 ggg rrr => "pmaxsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3d @addr      => "pmaxsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3e 11 ggg rrr => "pmaxuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3e @addr      => "pmaxuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3f 11 ggg rrr => "pmaxud " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3f @addr      => "pmaxud " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x40 11 ggg rrr => "pmulld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x40 @addr      => "pmulld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x41 11 ggg rrr => "phminposuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x41 @addr      => "phminposuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdb 11 ggg rrr => "aesimc " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdb @addr      => "aesimc " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdc @addr [$reptype==1] => kl_dc[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdd @addr [$reptype==1] => kl_dd[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xde @addr [$reptype==1] => kl_de[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdf @addr [$reptype==1] => kl_df[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdc 11 ggg rrr [$reptype==1] => liwk[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xdc 11 ggg rrr => "aesenc " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdc @addr      => "aesenc " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdd 11 ggg rrr => "aesenclast " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdd @addr      => "aesenclast " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xde 11 ggg rrr => "aesdec " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xde @addr      => "aesdec " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdf 11 ggg rrr => "aesdeclast " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdf @addr      => "aesdeclast " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x2a @addr      => "movntdqa " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xc8 11 ggg rrr => "sha1nexte " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xc8 @addr      => "sha1nexte " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xc9 11 ggg rrr => "sha1msg1 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xc9 @addr      => "sha1msg1 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xca 11 ggg rrr => "sha1msg2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xca @addr      => "sha1msg2 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcb 11 ggg rrr => "sha256rnds2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcb @addr      => "sha256rnds2 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcc 11 ggg rrr => "sha256msg1 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcc @addr      => "sha256msg1 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcd 11 ggg rrr => "sha256msg2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcd @addr      => "sha256msg2 " ssereg[8+$g] "," $addr ;
  # --- SSE mov-family (10-17, 28-2B, 6F/7F, 7E, D7) ---
  0x0f 0x10 11 ggg rrr => movu[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x10 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " ssereg[8+$g] "," $addr ;
  0x0f 0x11 11 ggg rrr => movu[$reptype*2+$opsiz] " " ssereg[8+$r] "," ssereg[8+$g] wit("alt") ;
  0x0f 0x11 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " $addr "," ssereg[8+$g] ;
  0x0f 0x12 11 ggg rrr [$reptype] => m12f[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      [$reptype] => m12f[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr => "movhlps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      => ml12[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x13 @addr      => ml12[$opsiz] " " $addr "," ssereg[8+$g] ;
  0x0f 0x14 11 ggg rrr => unpl[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x14 @addr      => unpl[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => unph[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x15 @addr      => unph[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr [$reptype] => m16f[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x16 @addr      [$reptype] => m16f[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr => "movlhps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x16 @addr      => mh16[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x17 @addr      => mh16[$opsiz] " " $addr "," ssereg[8+$g] ;
  0x0f 0x28 11 ggg rrr => mova2[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x28 @addr      => mova2[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x29 11 ggg rrr => mova2[$opsiz] " " ssereg[8+$r] "," ssereg[8+$g] wit("alt") ;
  0x0f 0x29 @addr      => mova2[$opsiz] " " $addr "," ssereg[8+$g] ;
  0x0f 0x2b @addr      => mnt4[$reptype*2+$opsiz] " " $addr "," ssereg[8+$g] ;
  0x0f 0x6f 11 ggg rrr => mdq[$reptype*2+$opsiz] " " ssereg[$opsiz*8+$reptype*8+$g] "," ssereg[$opsiz*8+$reptype*8+$r] ;
  0x0f 0x6f @addr      => mdq[$reptype*2+$opsiz] " " ssereg[$opsiz*8+$reptype*8+$g] "," $addr ;
  0x0f 0x7f 11 ggg rrr => mdq[$reptype*2+$opsiz] " " ssereg[$opsiz*8+$reptype*8+$r] "," ssereg[$opsiz*8+$reptype*8+$g] wit("alt") ;
  0x0f 0x7f @addr      => mdq[$reptype*2+$opsiz] " " $addr "," ssereg[$opsiz*8+$reptype*8+$g] ;
  0x0f 0x7e @addr      [$reptype==1] => m7e[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x7e @addr      => "movd " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[$g] "," ssereg[$opsiz*8+$r] ;
  # --- SSE shift-by-imm8 (71/72/73) + misc (C2-C6, D0, 7C/7D, D6, E6/E7, F7) ---
  0x0f 0x71 11 010 rrr @imm8 => "psrlw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x71 11 100 rrr @imm8 => "psraw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 010 rrr @imm8 => "psrld " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 100 rrr @imm8 => "psrad " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 110 rrr @imm8 => "pslld " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 010 rrr @imm8 => "psrlq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 011 rrr @imm8 => "psrldq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 110 rrr @imm8 => "psllq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 111 rrr @imm8 => "pslldq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xc2 11 ggg rrr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr      @imm8 => "cmp" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0xc4 @addr      @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => "shuf" elt[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr      @imm8 => "shuf" elt[$opsiz] " " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xd0 11 ggg rrr => d0t[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xd0 @addr      => d0t[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x7c 11 ggg rrr => h7c[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7c @addr      => h7c[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => h7d[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7d @addr      => h7d[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0xd6 11 ggg rrr [$reptype==1] => md6[$reptype] " " ssereg[8+$g] "," ssereg[$r] ;
  0x0f 0xd6 11 ggg rrr [$reptype==2] => md6[$reptype] " " ssereg[$g] "," ssereg[8+$r] ;
  0x0f 0xd6 11 ggg rrr [$opsiz] => "movq " ssereg[$opsiz*8+$r] "," ssereg[$opsiz*8+$g] wit("alt") ;
  0x0f 0xd6 @addr      [$opsiz] => "movq " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xe6 11 ggg rrr => me6[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xe6 @addr      => me6[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0xe7 @addr      => me7[$opsiz] " " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xf7 11 ggg rrr => mf7[$opsiz] " " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  # --- cvt-MMX mem, BA/2F/70 mem, F0, 0F 38 GP, 0F 3A, padlock ---
  0x0f 0x2a @addr => cvt2a[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x2c @addr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x2c @addr => cvt2c[$reptype*2+$opsiz] " " ssereg[$g] "," $addr ;
  0x0f 0x2d @addr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x2d @addr => cvt2d[$reptype*2+$opsiz] " " ssereg[$g] "," $addr ;
  0x0f 0x2f @addr => comi[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x70 @addr @imm8 => pshuf[$reptype*2+$opsiz] " " ssereg[($reptype*2+$opsiz==0 ? 0 : 8)+$g] "," $addr "," hex($imm8) ;
  0x0f 0xba @addr(4) @imm8 => "bt " $addr "," hex($imm8) ;
  0x0f 0xba @addr(5) @imm8 => "bts " $addr "," hex($imm8) ;
  0x0f 0xba @addr(6) @imm8 => "btr " $addr "," hex($imm8) ;
  0x0f 0xba @addr(7) @imm8 => "btc " $addr "," hex($imm8) ;
  0x0f 0xf0 @addr [$reptype==2] => ldqt[$reptype] " " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xf0 @addr [$reptype==2] => crct[$reptype] sfx[1] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf0 11 ggg rrr [$reptype==2] => crct[$reptype] sfx[1] " " greg[$g] "," rgb[$r] ;
  0x0f 0x38 0xf0 @addr => "movbe " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xf1 @addr [$reptype==2] => crct[$reptype] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xf1 11 ggg rrr [$reptype==2] => crct[$reptype] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x38 0xf1 @addr => "movbe " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0x38 0xf5 @addr [$opsiz] => wrut[$opsiz] " " $addr "," greg[$g] ;
  0x0f 0x38 0xf6 @addr [$opsiz] => acxt[$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr [$opsiz] => acxt[$opsiz] " " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xf6 @addr [$reptype==1] => adxt[$reptype] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr [$reptype==1] => adxt[$reptype] " " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xf6 @addr => "wrssd " $addr "," greg[$g] ;
  0x0f 0x38 0xf8 @addr [$reptype==2] => f8et[$reptype] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf8 @addr [$reptype==1] => f8st[$reptype] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf8 @addr [$opsiz] => f8mt[$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x38 0xf9 @addr => "movdiri " $addr "," greg[$g] ;
  0x0f 0x38 0xfa 11 ggg rrr [$reptype==1] => ek1t[$reptype] " " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xfb 11 ggg rrr [$reptype==1] => ek2t[$reptype] " " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xfc @addr => fct[$reptype*2+$opsiz] " " $addr "," greg[$g] ;
  0x0f 0x38 0xcf 11 ggg rrr => "gf2p8mulb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xcf @addr => "gf2p8mulb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x80 @addr [$opsiz] => i80t[$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x38 0x81 @addr [$opsiz] => i81t[$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x38 0x82 @addr [$opsiz] => i82t[$opsiz] " " greg[$g] "," $addr ;
  0x0f 0x38 0xd8 @addr(0) [$reptype==1] => aesd8[$reptype*4+0] " " $addr ;
  0x0f 0x38 0xd8 @addr(1) [$reptype==1] => aesd8[$reptype*4+1] " " $addr ;
  0x0f 0x38 0xd8 @addr(2) [$reptype==1] => aesd8[$reptype*4+2] " " $addr ;
  0x0f 0x38 0xd8 @addr(3) [$reptype==1] => aesd8[$reptype*4+3] " " $addr ;
  0x0f 0x3a 0xce 11 ggg rrr @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xce @addr @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcf 11 ggg rrr @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcf @addr @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xf0 11 ggg rrr @imm8 [$reptype==1] => hrest[$reptype] " " hex($imm8) ;
  0x0f 0xa6 11 000 rrr => "montmul" ;
  0x0f 0xa6 11 001 rrr => "xsha1" ;
  0x0f 0xa6 11 010 rrr => "xsha256" ;
  0x0f 0xa7 11 000 rrr => "xstore" ;
  0x0f 0xa7 11 001 rrr => "xcrypt_ecb" ;
  0x0f 0xa7 11 010 rrr => "xcrypt_cbc" ;
  0x0f 0xa7 11 011 rrr => "xcrypt_ctr" ;
  0x0f 0xa7 11 100 rrr => "xcrypt_cfb" ;
  0x0f 0xa7 11 101 rrr => "xcrypt_ofb" ;
  # --- 0F 78/79  vmread/vmwrite (none), extrq/insertq (66/F2) ---
  0x0f 0x78 11 000 rrr @imm8 @imm8b [$opsiz] => eqt[$opsiz] " " ssereg[8+$r] "," hex($imm8) "," hex($imm8b) ;
  0x0f 0x78 11 ggg rrr @imm8 @imm8b [$reptype==2] => iqt[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) "," hex($imm8b) ;
  0x0f 0x78 11 ggg rrr => "vmread " greg[$r] "," greg[$g] ;
  0x0f 0x78 @addr      => "vmread " $addr "," greg[$g] ;
  0x0f 0x79 11 ggg rrr [$opsiz] => eqt[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x79 11 ggg rrr [$reptype==2] => iqt[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x79 11 ggg rrr => "vmwrite " greg[$g] "," greg[$r] ;
  0x0f 0x79 @addr      => "vmwrite " greg[$g] "," $addr ;
  # --- SSE packed/scalar float ops (0F 50-5F) ---
  0x0f 0x50 11 ggg rrr => "movmsk" elt[$opsiz] " " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x51 11 ggg rrr => "sqrt" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x51 @addr      => "sqrt" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => "rsqrt" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x52 @addr      => "rsqrt" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => "rcp" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x53 @addr      => "rcp" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => "and" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x54 @addr      => "and" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => "andn" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x55 @addr      => "andn" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => "or" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x56 @addr      => "or" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => "xor" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x57 @addr      => "xor" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => "add" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x58 @addr      => "add" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => "mul" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x59 @addr      => "mul" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => "sub" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5c @addr      => "sub" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => "min" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5d @addr      => "min" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => "div" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5e @addr      => "div" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => "max" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5f @addr      => "max" elt[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => cvt5a[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5a @addr      => cvt5a[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => cvt5b[$reptype*2+$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5b @addr      => cvt5b[$reptype*2+$opsiz] " " ssereg[8+$g] "," $addr ;
  # --- regular MMX/SSE2 integer ops (mm / xmm by opsiz) ---
  0x0f 0x60 11 ggg rrr => "punpcklbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x60 @addr      => "punpcklbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x61 11 ggg rrr => "punpcklwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x61 @addr      => "punpcklwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x62 11 ggg rrr => "punpckldq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x62 @addr      => "punpckldq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x63 11 ggg rrr => "packsswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x63 @addr      => "packsswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x64 11 ggg rrr => "pcmpgtb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x64 @addr      => "pcmpgtb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x65 11 ggg rrr => "pcmpgtw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x65 @addr      => "pcmpgtw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x66 11 ggg rrr => "pcmpgtd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x66 @addr      => "pcmpgtd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x67 11 ggg rrr => "packuswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x67 @addr      => "packuswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x68 11 ggg rrr => "punpckhbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x68 @addr      => "punpckhbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x69 11 ggg rrr => "punpckhwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x69 @addr      => "punpckhwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6a 11 ggg rrr => "punpckhdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6a @addr      => "punpckhdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6b 11 ggg rrr => "packssdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6b @addr      => "packssdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6c 11 ggg rrr => "punpcklqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6c @addr      => "punpcklqdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6d 11 ggg rrr => "punpckhqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6d @addr      => "punpckhqdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x74 11 ggg rrr => "pcmpeqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x74 @addr      => "pcmpeqb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x75 11 ggg rrr => "pcmpeqw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x75 @addr      => "pcmpeqw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x76 11 ggg rrr => "pcmpeqd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x76 @addr      => "pcmpeqd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd1 11 ggg rrr => "psrlw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd1 @addr      => "psrlw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd2 @addr      => "psrld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd3 @addr      => "psrlq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd4 11 ggg rrr => "paddq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd4 @addr      => "paddq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd5 @addr      => "pmullw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd8 11 ggg rrr => "psubusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd8 @addr      => "psubusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd9 11 ggg rrr => "psubusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd9 @addr      => "psubusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xda 11 ggg rrr => "pminub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xda @addr      => "pminub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdb 11 ggg rrr => "pand " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdb @addr      => "pand " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdc 11 ggg rrr => "paddusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdc @addr      => "paddusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdd 11 ggg rrr => "paddusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdd @addr      => "paddusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xde 11 ggg rrr => "pmaxub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xde @addr      => "pmaxub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdf 11 ggg rrr => "pandn " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdf @addr      => "pandn " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe0 11 ggg rrr => "pavgb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe0 @addr      => "pavgb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe1 11 ggg rrr => "psraw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe1 @addr      => "psraw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe2 11 ggg rrr => "psrad " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe2 @addr      => "psrad " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe3 11 ggg rrr => "pavgw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe3 @addr      => "pavgw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe4 11 ggg rrr => "pmulhuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe4 @addr      => "pmulhuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe5 11 ggg rrr => "pmulhw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe5 @addr      => "pmulhw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe8 11 ggg rrr => "psubsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe8 @addr      => "psubsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe9 11 ggg rrr => "psubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe9 @addr      => "psubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xea 11 ggg rrr => "pminsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xea @addr      => "pminsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xeb 11 ggg rrr => "por " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xeb @addr      => "por " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xec 11 ggg rrr => "paddsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xec @addr      => "paddsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xed 11 ggg rrr => "paddsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xed @addr      => "paddsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xee 11 ggg rrr => "pmaxsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xee @addr      => "pmaxsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xef 11 ggg rrr => "pxor " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xef @addr      => "pxor " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf1 11 ggg rrr => "psllw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf1 @addr      => "psllw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf2 11 ggg rrr => "pslld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf2 @addr      => "pslld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf3 11 ggg rrr => "psllq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf3 @addr      => "psllq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf4 11 ggg rrr => "pmuludq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf4 @addr      => "pmuludq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf5 11 ggg rrr => "pmaddwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf5 @addr      => "pmaddwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf6 11 ggg rrr => "psadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf6 @addr      => "psadbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf8 11 ggg rrr => "psubb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf8 @addr      => "psubb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf9 11 ggg rrr => "psubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf9 @addr      => "psubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfa 11 ggg rrr => "psubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfa @addr      => "psubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfb 11 ggg rrr => "psubq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfb @addr      => "psubq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfc 11 ggg rrr => "paddb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfc @addr      => "paddb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfd 11 ggg rrr => "paddw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfd @addr      => "paddw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfe 11 ggg rrr => "paddd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfe @addr      => "paddd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6e 11 ggg rrr => "movd " ssereg[$opsiz*8+$g] "," greg[$r] ;
  0x0f 0x6e @addr      => "movd" sfx[4] " " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x70 11 ggg rrr @imm8 => pshuf[$reptype*2+$opsiz] " " ssereg[($reptype*2+$opsiz==0 ? 0 : 8)+$g] "," ssereg[($reptype*2+$opsiz==0 ? 0 : 8)+$r] "," hex($imm8) ;
  0x0f 0x71 11 110 rrr @imm8 => "psllw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x77 => "emms" ;
  0x0f 0x7e 11 ggg rrr [$reptype==1] => m7e[$reptype] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7e 11 ggg rrr => "movd " greg[$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0xa0 => "push fs" ;
  0x0f 0xa1 => "pop fs" ;
  0x0f 0xa2 => "cpuid" ;
  0x0f 0xa3 11 ggg rrr => "bt " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xa5 11 ggg rrr => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xab 11 ggg rrr => "bts " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xab @addr      => "bts " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xae 0xe8 => "lfence" ;
  0x0f 0xae 0xf0 => "mfence" ;
  0x0f 0xae 0xf8 => "sfence" ;
  0x0f 0xae @addr(7) => "clflush" sfx[1] " " $addr ;
  0x0f 0xae @addr(0) => "fxsave " $addr ;
  0x0f 0xae @addr(1) => "fxrstor " $addr ;
  0x0f 0xae @addr(2) => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3) => "stmxcsr " $addr ;
  0x0f 0xae @addr(4) => "xsave " $addr ;
  0x0f 0xae @addr(5) => "xrstor " $addr ;
  0x0f 0xae @addr(6) => "xsaveopt " $addr ;
  0x0f 0xae 11 100 rrr [$reptype==1] => aer4[$reptype] " " greg[$opsiz*8+$r] ;
  # 0F C7 /digit memory group
  0x0f 0xc7 @addr(1) => "cmpxchg8b " $addr ;
  0x0f 0xc7 @addr(3) => "xrstors " $addr ;
  0x0f 0xc7 @addr(4) => "xsavec " $addr ;
  0x0f 0xc7 @addr(5) => "xsaves " $addr ;
  0x0f 0xc7 @addr(6) => "vmptrld " $addr ;
  0x0f 0xc7 @addr(7) => "vmptrst " $addr ;
  0x0f 0xc7 11 110 rrr => "rdrand " greg[$opsiz*8+$r] ;
  0x0f 0xc7 11 111 rrr => c7r7[$reptype] " " greg[$opsiz*8+$r] ;
  0x0f 0xaf 11 ggg rrr => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xb2 @addr      => "lss " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb3 11 ggg rrr => "btr " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xb4 @addr      => "lfs " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb5 @addr      => "lgs " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb6 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," rgb[$r] ;
  0x0f 0xb6 @addr      => "movzx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," greg[8+$r] ;
  0x0f 0xb7 @addr      => "movzx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xba 11 fff rrr @imm8 => grpba[$f] " " greg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xbc 11 ggg rrr [$reptype==1] => tzt[$reptype] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbc @addr [$reptype==1] => tzt[$reptype] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbc 11 ggg rrr => "bsf " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbd 11 ggg rrr [$reptype==1] => lzt[$reptype] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbd @addr [$reptype==1] => lzt[$reptype] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr => "bsr " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," rgb[$r] ;
  0x0f 0xbe @addr      => "movsx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;
  # --- GP-register 0F two-byte (mem forms + remaining opcodes) ---
  0x0f 0xbf 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," greg[8+$r] ;
  0x0f 0xbf @addr      => "movsx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xa3 @addr      => "bt " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xa4 @addr @imm8 => "shld " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xa5 @addr      => "shld " $addr "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xac @addr @imm8 => "shrd " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xb3 @addr      => "btr " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xbc @addr      => "bsf " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbd @addr      => "bsr " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xaf @addr      => "imul " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xad 11 ggg rrr => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xad @addr      => "shrd " $addr "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[$r] "," rgb[$g] ;
  0x0f 0xb0 @addr      => "cmpxchg " $addr "," rgb[$g] ;
  0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xbb 11 ggg rrr => "btc " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xbb @addr      => "btc " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xc0 11 ggg rrr => "xadd " rgb[$r] "," rgb[$g] ;
  0x0f 0xc0 @addr      => "xadd " $addr "," rgb[$g] ;
  0x0f 0xc1 11 ggg rrr => "xadd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xc1 @addr      => "xadd " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xc3 @addr      => "movnti " $addr "," greg[$g] ;
  0x0f 0xb8 11 ggg rrr => pcnt[$reptype] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xb8 @addr      => pcnt[$reptype] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb9 11 ggg rrr => "ud1 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xb9 @addr      => "ud1 " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xff 11 ggg rrr => "ud0 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xff @addr      => "ud0 " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xa8 => "push gs" ;
  0x0f 0xa9 => "pop gs" ;
  0x0f 0xaa => "rsm" ;
  0x0f 11001 bbb => "bswap " greg[$opsiz*8+$b] ;
  0x0f 0x18 @addr(1) => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x30 => "wrmsr" ;
  0x0f 0x31 => "rdtsc" ;
  0x0f 0x32 => "rdmsr" ;
  0x0f 0x33 => "rdpmc" ;
  0x0f 0x34 => "sysenter" ;
  0x0f 0x35 => "sysexit" ;
  0x0f 0x37 => "getsec" ;
  0x0f 0x0b => "ud2" ;
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," $addr ;

  # --- lea / mov family -----------------------------------------------------
  0x8d @addr => "lea " greg[$opsiz*8+$g] "," $addr ;
  0x88 11 ggg rrr => "mov " rgb[$r] "," rgb[$g] ;
  0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[$g] ;
  0x89 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x89 @addr      => "mov " $addr "," greg[$opsiz*8+$g] ;
  0x8b 11 ggg rrr => wit("alt") "mov " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x8b @addr      => "mov " greg[$opsiz*8+$g] "," $addr ;
  0x8a 11 ggg rrr => wit("alt") "mov " rgb[$g] "," rgb[$r] ;
  0x8a @addr      => "mov" sfx[1] " " rgb[$g] "," $addr ;
  0x8c 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," sreg[$g] ;
  0x8c @addr      => "mov " $addr "," sreg[$g] ;
  0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[$opsiz*8+$r] ;
  0x8e @addr      => "mov " sreg[$g] "," $addr ;
  0xa0 @immadr => "mov al," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa1 @immadr => "mov " greg[$opsiz*8+0] "," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa2 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "],al" ;
  0xa3 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "]," greg[$opsiz*8+0] ;
  10110 bbb @imm8 => "mov " rgb[$b] "," hex($imm8) ;
  10111 bbb @immz => "mov " greg[$opsiz*8+$b] "," hex($immz) ;
  0xc6 11 000 rrr @imm8 => wit("long") "mov " rgb[$r] "," hex($imm8) ;
  0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;
  0xc6 0xf8 @imm8 => "xabort " hex($imm8) ;
  0xc7 11 000 rrr @immz => wit("long") "mov " greg[$opsiz*8+$r] "," hex($immz) ;
  0xc7 @addr(0)   @immz => "mov" sfx[4>>$opsiz] " " $addr "," hex($immz) ;
  0xc7 0xf8 @relz => "xbegin " hex($relz) ;

  # --- xchg / bswap / stack -------------------------------------------------
  0x90 [$reptype==1] => pause_t[$reptype] ;
  0x90 => "nop" ;
  10010 bbb => "xchg " greg[$opsiz*8+$b] "," greg[$opsiz*8+0] ;
  0x87 11 ggg rrr => wit($g==0 ? "long" : "") "xchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x87 @addr      => "xchg " $addr "," greg[$opsiz*8+$g] ;
  0x86 11 ggg rrr => "xchg " rgb[$r] "," rgb[$g] ;
  0x86 @addr      => "xchg" sfx[1] " " $addr "," rgb[$g] ;
  0x68 @immz => "push " hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6a @imm8 => "push " hex($imm8) ;
  01010 bbb => "push " greg[$opsiz*8+$b] ;
  01011 bbb => "pop " greg[$opsiz*8+$b] ;
  0x8f @xop => $xop ;
  0x8f 11 000 rrr => wit("long") "pop " greg[$opsiz*8+$r] ;
  0x8f @addr(0) => "pop" sfx[4] " " $addr ;
  0x06 => "push es" ;
  0x07 => "pop es" ;
  0x0e => "push cs" ;
  0x16 => "push ss" ;
  0x17 => "pop ss" ;
  0x1e => "push ds" ;
  0x1f => "pop ds" ;
  0x60 => "pusha" ;
  0x61 => "popa" ;
  0x9c => "pushf" ;
  0x9d => "popf" ;
  0xd7 => "xlatb" ;

  # --- ALU group (00..3D) : op table; Eb/Ev, both directions, AL/eAX imm -----
  00 fff 000 11 ggg rrr => alu[$f] " " rgb[$r] "," rgb[$g] ;
  00 fff 000 @addr      => alu[$f] sfx[1] " " $addr "," rgb[$g] ;
  00 fff 010 11 ggg rrr => wit("alt") alu[$f] " " rgb[$g] "," rgb[$r] ;
  00 fff 010 @addr      => alu[$f] sfx[1] " " rgb[$g] "," $addr ;
  00 fff 001 11 ggg rrr => alu[$f] " " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  00 fff 001 @addr      => alu[$f] " " $addr "," greg[$opsiz*8+$g] ;
  00 fff 011 11 ggg rrr => wit("alt") alu[$f] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  00 fff 011 @addr      => alu[$f] " " greg[$opsiz*8+$g] "," $addr ;
  00 fff 100 @imm8 => alu[$f] " al," hex($imm8) ;
  00 fff 101 @immz => alu[$f] " " greg[$opsiz*8+0] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;

  # --- group 1 (80/81/82/83): 8 ALU ops via alu[$f]; 0x82 is the alias of 0x80
  0x80 11 fff rrr @imm8 => wit($r==0 ? "long" : "") alu[$f] " " rgb[$r] "," hex($imm8) ;
  0x80 @addr      @imm8 => alu[$g] sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 fff rrr @imm8 => wit("alt") alu[$f] " " rgb[$r] "," hex($imm8) ;
  0x82 @addr      @imm8 => wit("alt") alu[$g] sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 fff rrr @immz => alu[$f] " " greg[$opsiz*8+$r] "," hex($immz) wit($r==0 ? "long" : "") wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x81 @addr      @immz => alu[$g] sfx[4>>$opsiz] " " $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x83 11 fff rrr @imm8 => alu[$f] " " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr      @imm8 => alu[$g] sfx[4>>$opsiz] " " $addr "," hex(sx8($imm8)) ;

  # --- inc/dec, unary group F7, shift group, test ---------------------------
  01000 bbb => "inc " greg[$opsiz*8+$b] ;
  01001 bbb => "dec " greg[$opsiz*8+$b] ;
  0xff 11 000 rrr => wit("long") "inc " greg[$opsiz*8+$r] ;
  0xff @addr(0)   => "inc" sfx[4] " " $addr ;
  0xff 11 001 rrr => wit("long") "dec " greg[$opsiz*8+$r] ;
  0xff @addr(1)   => "dec" sfx[4] " " $addr ;
  0xff 11 010 rrr => "call " greg[$opsiz*8+$r] ;
  0xff @addr(2)   => "call" sfx[4] " " $addr ;
  0xff @addr(3)   => "call far " $addr ;
  0xff 11 100 rrr => "jmp " greg[$opsiz*8+$r] ;
  0xff @addr(4)   => "jmp" sfx[4] " " $addr ;
  0xff @addr(5)   => "jmp far " $addr ;
  0xff 11 110 rrr => wit("long") "push " greg[$opsiz*8+$r] ;
  0xff @addr(6)   => "push" sfx[4] " " $addr ;
  0xfe 11 000 rrr => "inc " rgb[$r] ;
  0xfe @addr(0)   => "inc" sfx[1] " " $addr ;
  0xfe 11 001 rrr => "dec " rgb[$r] ;
  0xfe @addr(1)   => "dec" sfx[1] " " $addr ;
  0xf7 11 000 rrr @immz => wit($r==0 ? "long" : "") "test " greg[$opsiz*8+$r] "," hex($immz) ;
  0xf7 11 001 rrr @immz => wit("alt") "test " greg[$opsiz*8+$r] "," hex($immz) ;
  0xf7 11 fff rrr => grp3[$f] " " greg[$opsiz*8+$r] ;
  0xf7 @addr(0) @immz => "test" sfx[4] " " $addr "," hex($immz) ;
  0xf7 @addr(1) @immz => wit("alt") "test" sfx[4] " " $addr "," hex($immz) ;
  0xf7 @addr      => grp3[$g] sfx[4] " " $addr ;
  0xf6 11 000 rrr @imm8 => wit($r==0 ? "long" : "") "test " rgb[$r] "," hex($imm8) ;
  0xf6 11 001 rrr @imm8 => wit("alt") "test " rgb[$r] "," hex($imm8) ;
  0xf6 11 fff rrr => grp3[$f] " " rgb[$r] ;
  0xf6 @addr(0) @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 @addr(1) @imm8 => wit("alt") "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 @addr      => grp3[$g] sfx[1] " " $addr ;
  0x84 11 ggg rrr => "test " rgb[$r] "," rgb[$g] ;
  0x84 @addr      => "test" sfx[1] " " $addr "," rgb[$g] ;
  0x85 11 ggg rrr => "test " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x85 @addr      => "test " $addr "," greg[$opsiz*8+$g] ;
  0xa8 @imm8 => "test al," hex($imm8) ;
  0xa9 @immz => "test " greg[$opsiz*8+0] "," hex($immz) ;
  0xc1 11 fff rrr @imm8 => shift[$f] " " greg[$opsiz*8+$r] "," hex($imm8) wit($f==6 ? "alt" : "") ;
  0xc1 @addr @imm8 => shift[$g] sfx[4] " " $addr "," hex($imm8) wit($g==6 ? "alt" : "") ;
  0xd1 11 fff rrr => shift[$f] " " greg[$opsiz*8+$r] ",1" wit($f==6 ? "alt" : "") ;
  0xd1 @addr => shift[$g] sfx[4] " " $addr ",1" wit($g==6 ? "alt" : "") ;
  0xd3 11 fff rrr => shift[$f] " " greg[$opsiz*8+$r] ",cl" wit($f==6 ? "alt" : "") ;
  0xd3 @addr => shift[$g] sfx[4] " " $addr ",cl" wit($g==6 ? "alt" : "") ;
  0xc0 11 fff rrr @imm8 => shift[$f] " " rgb[$r] "," hex($imm8) wit($f==6 ? "alt" : "") ;
  0xc0 @addr @imm8 => shift[$g] sfx[1] " " $addr "," hex($imm8) wit($g==6 ? "alt" : "") ;
  0xd0 11 fff rrr => shift[$f] " " rgb[$r] ",1" wit($f==6 ? "alt" : "") ;
  0xd0 @addr => shift[$g] sfx[1] " " $addr ",1" wit($g==6 ? "alt" : "") ;
  0xd2 11 fff rrr => shift[$f] " " rgb[$r] ",cl" wit($f==6 ? "alt" : "") ;
  0xd2 @addr => shift[$g] sfx[1] " " $addr ",cl" wit($g==6 ? "alt" : "") ;

  # --- imul Gv,Ev,imm ; decimal adjust --------------------------------------
  0x69 11 ggg rrr @immz => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x69 @addr @immz      => "imul " greg[$opsiz*8+$g] "," $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6b 11 ggg rrr @imm8 => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex($imm8) ;
  0x6b @addr @imm8      => "imul " greg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x27 => "daa" ;
  0x2f => "das" ;
  0x37 => "aaa" ;
  0x3f => "aas" ;
  0xd4 @imm8 => "aam " hex($imm8) ;
  0xd5 @imm8 => "aad " hex($imm8) ;

  # --- branches / ret / int / enter / far ptr / bound,arpl,les,lds ----------
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xe9 @relz => wit("long") "jmp " hex($relz) ;
  0xe8 @relz => "call " hex($relz) ;
  0xea @immz @imm16 => "jmp " hex($imm16) ":" hex($immz) ;
  0x9a @immz @imm16 => "call " hex($imm16) ":" hex($immz) ;
  0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;
  0xe3 @rel8 => "jecxz " hex($rel8) ;
  0xe2 @rel8 => "loop " hex($rel8) ;
  0xe1 @rel8 => "loope " hex($rel8) ;
  0xe0 @rel8 => "loopne " hex($rel8) ;
  0xc3 => "ret" ;
  0xc2 @imm16 => "ret " hex($imm16) ;
  0xcb => "retf" ;
  0xca @imm16 => "retf " hex($imm16) ;
  0xcd @imm8 => "int " hex($imm8) ;
  0xcc => "int3" ;
  0xce => "into" ;
  0xcf => "iret" ;
  0xc9 => "leave" ;
  0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;
  0x62 @addr => "bound" sfx[8] " " greg[$opsiz*8+$g] "," $addr ;
  0x62 @evex => $evex ;
  0x63 11 ggg rrr => "arpl " greg[8+$r] "," greg[8+$g] ;
  0x63 @addr      => "arpl " $addr "," greg[8+$g] ;
  0xc4 @addr => "les " greg[$opsiz*8+$g] "," $addr ;
  0xc5 @addr => "lds " greg[$opsiz*8+$g] "," $addr ;
  0xc5 @vex2 => $vex2 ;

  # --- string ops : mnemonic from opsiz-indexed table where size-varying -----
  0xa4 => "movsb" ;
  0xa5 => movs[$opsiz] ;
  0xaa => "stosb" ;
  0xab => stos[$opsiz] ;
  0xac => "lodsb" ;
  0xad => lods[$opsiz] ;
  0xa6 => "cmpsb" ;
  0xae => "scasb" ;
  0xa7 => cmps[$opsiz] ;
  0xaf => scas[$opsiz] ;

  # --- flags / misc / size-varying accumulators -----------------------------
  0xf8 => "clc" ;
  0xf9 => "stc" ;
  0xf5 => "cmc" ;
  0xfc => "cld" ;
  0xfd => "std" ;
  0xfa => "cli" ;
  0xfb => "sti" ;
  0x9f => "lahf" ;
  0x9e => "sahf" ;
  0xf4 => "hlt" ;
  0x99 => cdqw[$opsiz] ;
  0x98 => cbwe[$opsiz] ;
  0x9b => "fwait" ;


  # --- I/O ------------------------------------------------------------------
  0xe4 @imm8 => "in al," hex($imm8) ;
  0xe5 @imm8 => "in " greg[$opsiz*8+0] "," hex($imm8) ;
  0xed => "in " greg[$opsiz*8+0] ",dx" ;
  0xe6 @imm8 => "out " hex($imm8) ",al" ;
  0xe7 @imm8 => "out " hex($imm8) "," greg[$opsiz*8+0] ;
  0xec => "in al,dx" ;
  0xee => "out dx,al" ;
  0xef => "out dx," greg[$opsiz*8+0] ;
  0xef => "out dx," greg[$opsiz*8+0] ;
  0x6c => "insb" ;
  0x6e => "outsb" ;
  0x6d => insx[$opsiz] ;
  0x6f => outsx[$opsiz] ;
  0xd6 => "salc" ;
  0xf1 => "int1" ;
}

# ---- entry point ----------------------------------------------------------
# ============================ VEX (C4 3-byte) decoder =======================
# In 32-bit mode VEX.R/X/B are forced 1, so byte1 = 111 mmmmm (E1/E2/E3 = map
# 0F/0F38/0F3A).  byte2 = W vvvv L pp.  vvvv is the 1's-complement of the second
# source register (reg = 15 - vvvv for the 0..7 range legal in 32-bit).  Fields
# captured here emit in reverse for byte-exact round-trip; no prefix recovery.
submatch vex {
  # ---- LIG (L-ignored scalars): L=1 alt enc, reg stays xmm. W0L1/W1L1(wig)/W1L1(W-sig FMA) -> witnessed. ----
  0xc1 1 h vvv 1 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmovss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmovsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmulss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vminss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vminsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vdivss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 1111 1 10 0x10 @addr => wit("altl") wit("altw") "vmovss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 11 0x10 @addr => wit("altl") wit("altw") "vmovsd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 10 0x11 @addr => wit("altl") wit("altw") "vmovss " $addr "," vsse[$g] ;
  0xc1 1 1111 1 11 0x11 @addr => wit("altl") wit("altw") "vmovsd " $addr "," vsse[$g] ;
  0xc1 1 1111 1 00 0x2e @addr => wit("altl") wit("altw") "vucomiss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 00 0x2e 11 ggg rrr => wit("altl") wit("altw") "vucomiss " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 01 0x2e @addr => wit("altl") wit("altw") "vucomisd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0x2e 11 ggg rrr => wit("altl") wit("altw") "vucomisd " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 00 0x2f @addr => wit("altl") wit("altw") "vcomiss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 00 0x2f 11 ggg rrr => wit("altl") wit("altw") "vcomiss " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 01 0x2f @addr => wit("altl") wit("altw") "vcomisd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0x2f 11 ggg rrr => wit("altl") wit("altw") "vcomisd " vsse[$g] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vaddss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmulss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsubss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vminss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vminsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vdivss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 1 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc1 1 h vvv 1 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 1 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vroundss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vroundss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc1 1 h vvv 1 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmovss " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 1 h vvv 1 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") wit("altw") "vmovsd " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 0 h vvv 1 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmovss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmovss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmovsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmovsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv 1 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv 1 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0x99 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd132ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd132ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0x99 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd132sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd132sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0x9b @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub132ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub132ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0x9b @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub132sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub132sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0x9d @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd132ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd132ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0x9d @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd132sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd132sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0x9f @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub132ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub132ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0x9f @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub132sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub132sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xa9 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd213ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd213ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xa9 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd213sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd213sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xab @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub213ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub213ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xab @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub213sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub213sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xad @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd213ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd213ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xad @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd213sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd213sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xaf @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub213ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub213ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xaf @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub213sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub213sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xb9 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd231ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd231ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xb9 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd231sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmadd231sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xbb @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub231ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub231ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xbb @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub231sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfmsub231sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xbd @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd231ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd231ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xbd @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd231sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmadd231sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 0 h vvv 1 01 0xbf @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub231ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 0 h vvv 1 01 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub231ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc2 1 h vvv 1 01 0xbf @addr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub231sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc2 1 h vvv 1 01 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsub231sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  0xc1 0 1111 1 10 0x10 @addr => wit("altl") "vmovss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 10 0x10 @addr => wit("altw") wit("altl") "vmovss " vsse[$g] "," $addr ;
  0xc1 0 1111 1 11 0x10 @addr => wit("altl") "vmovsd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 11 0x10 @addr => wit("altw") wit("altl") "vmovsd " vsse[$g] "," $addr ;
  0xc1 0 1111 1 10 0x11 @addr => wit("altl") "vmovss " $addr "," vsse[$g] ;
  0xc1 1 1111 1 10 0x11 @addr => wit("altw") wit("altl") "vmovss " $addr "," vsse[$g] ;
  0xc1 0 1111 1 11 0x11 @addr => wit("altl") "vmovsd " $addr "," vsse[$g] ;
  0xc1 1 1111 1 11 0x11 @addr => wit("altw") wit("altl") "vmovsd " $addr "," vsse[$g] ;
  0xc1 0 h vvv 1 10 0x2a @addr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsi2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x2a @addr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsi2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 1111 1 00 0x2e @addr => wit("altl") "vucomiss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 00 0x2e @addr => wit("altw") wit("altl") "vucomiss " vsse[$g] "," $addr ;
  0xc1 0 1111 1 00 0x2e 11 ggg rrr => wit("altl") "vucomiss " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 00 0x2e 11 ggg rrr => wit("altw") wit("altl") "vucomiss " vsse[$g] "," vsse[$r] ;
  0xc1 0 1111 1 01 0x2e @addr => wit("altl") "vucomisd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0x2e @addr => wit("altw") wit("altl") "vucomisd " vsse[$g] "," $addr ;
  0xc1 0 1111 1 01 0x2e 11 ggg rrr => wit("altl") "vucomisd " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 01 0x2e 11 ggg rrr => wit("altw") wit("altl") "vucomisd " vsse[$g] "," vsse[$r] ;
  0xc1 0 1111 1 00 0x2f @addr => wit("altl") "vcomiss " vsse[$g] "," $addr ;
  0xc1 1 1111 1 00 0x2f @addr => wit("altw") wit("altl") "vcomiss " vsse[$g] "," $addr ;
  0xc1 0 1111 1 00 0x2f 11 ggg rrr => wit("altl") "vcomiss " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 00 0x2f 11 ggg rrr => wit("altw") wit("altl") "vcomiss " vsse[$g] "," vsse[$r] ;
  0xc1 0 1111 1 01 0x2f @addr => wit("altl") "vcomisd " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0x2f @addr => wit("altw") wit("altl") "vcomisd " vsse[$g] "," $addr ;
  0xc1 0 1111 1 01 0x2f 11 ggg rrr => wit("altl") "vcomisd " vsse[$g] "," vsse[$r] ;
  0xc1 1 1111 1 01 0x2f 11 ggg rrr => wit("altw") wit("altl") "vcomisd " vsse[$g] "," vsse[$r] ;
  0xc1 0 h vvv 1 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 1 h vvv 1 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  0xc1 0 h vvv 1 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 1 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 1 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc1 1 h vvv 1 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc1 0 h vvv 1 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 1 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 1 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc1 1 h vvv 1 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vroundss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vroundss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vroundss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vroundss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altl") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 1 h vvv 1 01 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vroundsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x6a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddss " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x6a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x6a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x6a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x6b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x6b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x6b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x6b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x6e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubss " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x6e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x6e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x6e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x6f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x6f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x6f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x6f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x7a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddss " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x7a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x7a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x7a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x7b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x7b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x7b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x7b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x7e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubss " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x7e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x7e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x7e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc3 0 h vvv 1 01 0x7f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," vsse[$s] ;
  0xc3 0 h vvv 1 01 0x7f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," vsse[$s] ;
  0xc3 1 h vvv 1 01 0x7f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," $addr ;
  0xc3 1 h vvv 1 01 0x7f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") wit("altl") "vfnmsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$s] "," vsse[$r] wit("alt") ;
  0xc1 0 h vvv 1 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmovss " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 1 h vvv 1 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmovss " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 0 h vvv 1 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vmovsd " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 1 h vvv 1 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") wit("altl") "vmovsd " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  0xc1 0 h vvv 1 10 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsi2ss " vsse[$g] "," vsse[(7-$v)] "," greg[$r] ;
  0xc1 0 h vvv 1 11 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altl") "vcvtsi2sd " vsse[$g] "," vsse[(7-$v)] "," greg[$r] ;
  0xc1 0 1111 1 10 0x2c @addr => wit("altl") "vcvttss2si " greg[$g] "," $addr ;
  0xc1 0 1111 1 10 0x2c 11 ggg rrr => wit("altl") "vcvttss2si " greg[$g] "," vsse[$r] ;
  0xc1 0 1111 1 11 0x2c @addr => wit("altl") "vcvttsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 1 11 0x2c 11 ggg rrr => wit("altl") "vcvttsd2si " greg[$g] "," vsse[$r] ;
  0xc1 0 1111 1 10 0x2d @addr => wit("altl") "vcvtss2si " greg[$g] "," $addr ;
  0xc1 0 1111 1 10 0x2d 11 ggg rrr => wit("altl") "vcvtss2si " greg[$g] "," vsse[$r] ;
  0xc1 0 1111 1 11 0x2d @addr => wit("altl") "vcvtsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 1 11 0x2d 11 ggg rrr => wit("altl") "vcvtsd2si " greg[$g] "," vsse[$r] ;
  # ---- WIG (W-ignored): W=1 is a valid alt encoding -> wit("altw"), round-trips ----
  0xc1 1 1111 l 00 0x5a 11 ggg rrr => wit("altw") "vcvtps2pd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc1 1 1111 l 00 0x5a @addr => wit("altw") "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x5a 11 ggg rrr => wit("altw") "vcvtpd2ps " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 0 01 0x5a @addr => wit("altw") "vcvtpd2ps " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0x5a @addr => wit("altw") "vcvtpd2ps " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 1 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 01 0xe6 11 ggg rrr => wit("altw") "vcvttpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 0 01 0xe6 @addr => wit("altw") "vcvttpd2dq " vsse[$g] "," $addr ;
  0xc1 1 1111 1 01 0xe6 @addr => wit("altw") "vcvttpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 1 1111 l 10 0xe6 11 ggg rrr => wit("altw") "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc1 1 1111 l 10 0xe6 @addr => wit("altw") "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 11 0xe6 11 ggg rrr => wit("altw") "vcvtpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 0 11 0xe6 @addr => wit("altw") "vcvtpd2dq " vsse[$g] "," $addr ;
  0xc1 1 1111 1 11 0xe6 @addr => wit("altw") "vcvtpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 1 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 1 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 1111 l 01 0x20 11 ggg rrr => wit("altw") "vpmovsxbw " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x20 @addr => wit("altw") "vpmovsxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x21 11 ggg rrr => wit("altw") "vpmovsxbd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x21 @addr => wit("altw") "vpmovsxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x22 11 ggg rrr => wit("altw") "vpmovsxbq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x22 @addr => wit("altw") "vpmovsxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x23 11 ggg rrr => wit("altw") "vpmovsxwd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x23 @addr => wit("altw") "vpmovsxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x24 11 ggg rrr => wit("altw") "vpmovsxwq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x24 @addr => wit("altw") "vpmovsxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x25 11 ggg rrr => wit("altw") "vpmovsxdq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x25 @addr => wit("altw") "vpmovsxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x30 11 ggg rrr => wit("altw") "vpmovzxbw " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x30 @addr => wit("altw") "vpmovzxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x31 11 ggg rrr => wit("altw") "vpmovzxbd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x31 @addr => wit("altw") "vpmovzxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x32 11 ggg rrr => wit("altw") "vpmovzxbq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x32 @addr => wit("altw") "vpmovzxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x33 11 ggg rrr => wit("altw") "vpmovzxwd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x33 @addr => wit("altw") "vpmovzxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x34 11 ggg rrr => wit("altw") "vpmovzxwq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x34 @addr => wit("altw") "vpmovzxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x35 11 ggg rrr => wit("altw") "vpmovzxdq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 1 1111 l 01 0x35 @addr => wit("altw") "vpmovzxdq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 h vvv l 01 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsrld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x12 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovhlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x14 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x14 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x15 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x15 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x16 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmovhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovlhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x16 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmovhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x54 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x54 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x55 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x55 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x56 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x56 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x57 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x57 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 00 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 00 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x60 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x60 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x61 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x61 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x62 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x62 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x63 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x63 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x64 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x65 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x66 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x67 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x67 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x68 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x69 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x69 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x6a @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x6a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x6b @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x6b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x6c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x6c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x6d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x6d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x74 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x75 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x76 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x7c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x7c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x7d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0x7d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd0 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 11 0xd0 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd5 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd8 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xd9 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xd9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xda @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xdb @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xdb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xdc @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xdd @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xde @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xdf @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe0 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe3 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe4 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe5 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe8 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xe9 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xe9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xea @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xea 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xeb @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xeb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xec @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xec 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xed @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xed 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xee @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xee 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xef @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xef 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf5 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf6 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf8 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xf9 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xf9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xfa @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xfa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xfb @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xfb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xfc @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xfc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xfd @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xfd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0xfe @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0xfe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x00 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpshufb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x00 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpshufb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x01 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x01 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x02 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x02 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x03 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x03 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x04 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaddubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x04 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaddubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x05 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x05 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x06 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x06 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x07 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x07 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vphsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x08 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x08 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x09 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x09 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x0a @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x0a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpsignd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x0b @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhrsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x0b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulhrsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x28 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmuldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x28 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmuldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x29 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x29 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpeqq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x2b @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpackusdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x2b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpackusdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x37 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x37 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpcmpgtq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x38 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x38 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x39 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x39 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3a @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3b @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpminud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpminud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3e @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x3f @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x3f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmaxud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x40 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x40 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vpmulld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xdc @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaesenc " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaesenc " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xdd @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaesenclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaesenclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xde @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaesdec " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaesdec " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xdf @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaesdeclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vaesdeclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x10 @addr => wit("altw") "vmovups " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x10 11 ggg rrr => wit("altw") "vmovups " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x10 @addr => wit("altw") "vmovupd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x10 11 ggg rrr => wit("altw") "vmovupd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 10 0x10 @addr => wit("altw") "vmovss " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 11 0x10 @addr => wit("altw") "vmovsd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x11 @addr => wit("altw") "vmovups " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 00 0x11 11 ggg rrr => wit("altw") "vmovups " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 01 0x11 @addr => wit("altw") "vmovupd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x11 11 ggg rrr => wit("altw") "vmovupd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 10 0x11 @addr => wit("altw") "vmovss " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 11 0x11 @addr => wit("altw") "vmovsd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 h vvv l 00 0x12 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmovlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 01 0x12 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmovlpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 10 0x12 @addr => wit("altw") "vmovsldup " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0x12 11 ggg rrr => wit("altw") "vmovsldup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 11 0x12 @addr => wit("altw") "vmovddup " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 11 0x12 11 ggg rrr => wit("altw") "vmovddup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x13 @addr => wit("altw") "vmovlps " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x13 @addr => wit("altw") "vmovlpd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 10 0x16 @addr => wit("altw") "vmovshdup " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0x16 11 ggg rrr => wit("altw") "vmovshdup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x17 @addr => wit("altw") "vmovhps " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x17 @addr => wit("altw") "vmovhpd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 00 0x28 @addr => wit("altw") "vmovaps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x28 11 ggg rrr => wit("altw") "vmovaps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x28 @addr => wit("altw") "vmovapd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x28 11 ggg rrr => wit("altw") "vmovapd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x29 @addr => wit("altw") "vmovaps " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 00 0x29 11 ggg rrr => wit("altw") "vmovaps " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 01 0x29 @addr => wit("altw") "vmovapd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x29 11 ggg rrr => wit("altw") "vmovapd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 00 0x2b @addr => wit("altw") "vmovntps " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x2b @addr => wit("altw") "vmovntpd " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 00 0x2e @addr => wit("altw") "vucomiss " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x2e 11 ggg rrr => wit("altw") "vucomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x2e @addr => wit("altw") "vucomisd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x2e 11 ggg rrr => wit("altw") "vucomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x2f @addr => wit("altw") "vcomiss " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x2f 11 ggg rrr => wit("altw") "vcomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x2f @addr => wit("altw") "vcomisd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x2f 11 ggg rrr => wit("altw") "vcomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 00 0x51 @addr => wit("altw") "vsqrtps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x51 11 ggg rrr => wit("altw") "vsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x51 @addr => wit("altw") "vsqrtpd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x51 11 ggg rrr => wit("altw") "vsqrtpd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 00 0x52 @addr => wit("altw") "vrsqrtps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x52 11 ggg rrr => wit("altw") "vrsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 00 0x53 @addr => wit("altw") "vrcpps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x53 11 ggg rrr => wit("altw") "vrcpps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 00 0x5a @addr => wit("altw") "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x5a 11 ggg rrr => wit("altw") "vcvtps2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x5a @addr => wit("altw") "vcvtpd2ps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x5a 11 ggg rrr => wit("altw") "vcvtpd2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altw") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("altw") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 00 0x5b @addr => wit("altw") "vcvtdq2ps " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 00 0x5b 11 ggg rrr => wit("altw") "vcvtdq2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x5b @addr => wit("altw") "vcvtps2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x5b 11 ggg rrr => wit("altw") "vcvtps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 10 0x5b @addr => wit("altw") "vcvttps2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0x5b 11 ggg rrr => wit("altw") "vcvttps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("altw") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("altw") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("altw") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 h vvv l 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("altw") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 1 1111 l 01 0x6f @addr => wit("altw") "vmovdqa " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0x6f 11 ggg rrr => wit("altw") "vmovdqa " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 10 0x6f @addr => wit("altw") "vmovdqu " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0x6f 11 ggg rrr => wit("altw") "vmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x70 @addr @imm8 => wit("altw") "vpshufd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 1 1111 l 01 0x70 11 ggg rrr @imm8 => wit("altw") "vpshufd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 1111 l 10 0x70 @addr @imm8 => wit("altw") "vpshufhw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 1 1111 l 10 0x70 11 ggg rrr @imm8 => wit("altw") "vpshufhw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 1111 l 11 0x70 @addr @imm8 => wit("altw") "vpshuflw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 1 1111 l 11 0x70 11 ggg rrr @imm8 => wit("altw") "vpshuflw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 1111 l 10 0x7e @addr => wit("altw") "vmovq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0x7e 11 ggg rrr => wit("altw") "vmovq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x7f @addr => wit("altw") "vmovdqa " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0x7f 11 ggg rrr => wit("altw") "vmovdqa " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 10 0x7f @addr => wit("altw") "vmovdqu " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 10 0x7f 11 ggg rrr => wit("altw") "vmovdqu " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 h vvv l 00 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 00 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 01 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 00 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 00 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv l 01 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 1111 l 01 0xd6 @addr => wit("altw") "vmovq " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 01 0xd6 11 ggg rrr => wit("altw") "vmovq " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 01 0xe6 @addr => wit("altw") "vcvttpd2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0xe6 11 ggg rrr => wit("altw") "vcvttpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 10 0xe6 @addr => wit("altw") "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 10 0xe6 11 ggg rrr => wit("altw") "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 11 0xe6 @addr => wit("altw") "vcvtpd2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 11 0xe6 11 ggg rrr => wit("altw") "vcvtpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0xe7 @addr => wit("altw") "vmovntdq " $addr "," vsse[$l*8+$g] ;
  0xc1 1 1111 l 11 0xf0 @addr => wit("altw") "vlddqu " vsse[$l*8+$g] "," $addr ;
  0xc1 1 1111 l 01 0xf7 11 ggg rrr => wit("altw") "vmaskmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x17 @addr => wit("altw") "vptest " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x17 11 ggg rrr => wit("altw") "vptest " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x1c @addr => wit("altw") "vpabsb " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x1c 11 ggg rrr => wit("altw") "vpabsb " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x1d @addr => wit("altw") "vpabsw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x1d 11 ggg rrr => wit("altw") "vpabsw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x1e @addr => wit("altw") "vpabsd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x1e 11 ggg rrr => wit("altw") "vpabsd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x20 @addr => wit("altw") "vpmovsxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x20 11 ggg rrr => wit("altw") "vpmovsxbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x21 @addr => wit("altw") "vpmovsxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x21 11 ggg rrr => wit("altw") "vpmovsxbd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x22 @addr => wit("altw") "vpmovsxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x22 11 ggg rrr => wit("altw") "vpmovsxbq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x23 @addr => wit("altw") "vpmovsxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x23 11 ggg rrr => wit("altw") "vpmovsxwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x24 @addr => wit("altw") "vpmovsxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x24 11 ggg rrr => wit("altw") "vpmovsxwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x25 @addr => wit("altw") "vpmovsxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x25 11 ggg rrr => wit("altw") "vpmovsxdq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x2a @addr => wit("altw") "vmovntdqa " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x30 @addr => wit("altw") "vpmovzxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x30 11 ggg rrr => wit("altw") "vpmovzxbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x31 @addr => wit("altw") "vpmovzxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x31 11 ggg rrr => wit("altw") "vpmovzxbd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x32 @addr => wit("altw") "vpmovzxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x32 11 ggg rrr => wit("altw") "vpmovzxbq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x33 @addr => wit("altw") "vpmovzxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x33 11 ggg rrr => wit("altw") "vpmovzxwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x34 @addr => wit("altw") "vpmovzxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x34 11 ggg rrr => wit("altw") "vpmovzxwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x35 @addr => wit("altw") "vpmovzxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x35 11 ggg rrr => wit("altw") "vpmovzxdq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0x41 @addr => wit("altw") "vphminposuw " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0x41 11 ggg rrr => wit("altw") "vphminposuw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 1 1111 l 01 0xdb @addr => wit("altw") "vaesimc " vsse[$l*8+$g] "," $addr ;
  0xc2 1 1111 l 01 0xdb 11 ggg rrr => wit("altw") "vaesimc " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc3 1 1111 l 01 0x08 @addr @imm8 => wit("altw") "vroundps " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x08 11 ggg rrr @imm8 => wit("altw") "vroundps " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x09 @addr @imm8 => wit("altw") "vroundpd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x09 11 ggg rrr @imm8 => wit("altw") "vroundpd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vroundss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vroundss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vroundsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vroundsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0c @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vblendps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0c 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vblendps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0d @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vblendpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0d 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vblendpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpblendw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpblendw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpalignr " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x0f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpalignr " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x17 @addr @imm8 => wit("altw") "vextractps " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x21 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vinsertps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x21 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vinsertps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x40 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vdpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x40 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vdpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x41 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vdppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x41 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vdppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x42 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vmpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x42 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vmpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x44 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpclmulqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0x44 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpclmulqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x60 @addr @imm8 => wit("altw") "vpcmpestrm " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x60 11 ggg rrr @imm8 => wit("altw") "vpcmpestrm " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x61 @addr @imm8 => wit("altw") "vpcmpestri " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x61 11 ggg rrr @imm8 => wit("altw") "vpcmpestri " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x62 @addr @imm8 => wit("altw") "vpcmpistrm " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x62 11 ggg rrr @imm8 => wit("altw") "vpcmpistrm " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x63 @addr @imm8 => wit("altw") "vpcmpistri " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x63 11 ggg rrr @imm8 => wit("altw") "vpcmpistri " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0xdf @addr @imm8 => wit("altw") "vaeskeygenassist " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0xdf 11 ggg rrr @imm8 => wit("altw") "vaeskeygenassist " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovss " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 h vvv l 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("altw") "vmovsd " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 1 1111 l 00 0x50 11 ggg rrr => wit("altw") "vmovmskps " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 1 1111 l 01 0x50 11 ggg rrr => wit("altw") "vmovmskpd " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 1 h vvv l 01 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsraw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsllw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsrad " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpslld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsrldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpsllq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 h vvv l 01 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("altw") "vpslldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 1 1111 l 01 0xd7 11 ggg rrr => wit("altw") "vpmovmskb " greg[$g] "," vsse[$l*8+$r] ;
  0xc3 1 1111 l 01 0x17 11 ggg rrr @imm8 => wit("altw") "vextractps " greg[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc1 1 1111 0 00 0x77 => wit("altw") "vzeroupper" ;
  0xc1 1 1111 1 00 0x77 => wit("altw") "vzeroall" ;
  0xc1 1 1111 0 00 0xae @addr(2) => wit("altw") "vldmxcsr " $addr ;
  0xc1 1 1111 0 00 0xae @addr(3) => wit("altw") "vstmxcsr " $addr ;
  # ---- mixed-width converts/broadcasts/var-shifts: re-derived classes -----
  0xc1 0 1111 l 00 0x5a 11 ggg rrr => "vcvtps2pd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc1 0 1111 l 00 0x5a @addr => "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x5a 11 ggg rrr => "vcvtpd2ps " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 01 0x5a @addr => "vcvtpd2ps " vsse[$g] "," $addr ;
  0xc1 0 1111 1 01 0x5a @addr => "vcvtpd2ps " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 0 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 01 0xe6 11 ggg rrr => "vcvttpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 01 0xe6 @addr => "vcvttpd2dq " vsse[$g] "," $addr ;
  0xc1 0 1111 1 01 0xe6 @addr => "vcvttpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 0 1111 l 10 0xe6 11 ggg rrr => "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc1 0 1111 l 10 0xe6 @addr => "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 11 0xe6 11 ggg rrr => "vcvtpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 11 0xe6 @addr => "vcvtpd2dq " vsse[$g] "," $addr ;
  0xc1 0 1111 1 11 0xe6 @addr => "vcvtpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  0xc1 0 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc1 0 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 1111 l 01 0x13 11 ggg rrr => "vcvtph2ps " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x13 @addr => "vcvtph2ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x18 11 ggg rrr => "vbroadcastss " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x18 @addr => "vbroadcastss " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x20 11 ggg rrr => "vpmovsxbw " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x20 @addr => "vpmovsxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x21 11 ggg rrr => "vpmovsxbd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x21 @addr => "vpmovsxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x22 11 ggg rrr => "vpmovsxbq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x22 @addr => "vpmovsxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x23 11 ggg rrr => "vpmovsxwd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x23 @addr => "vpmovsxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x24 11 ggg rrr => "vpmovsxwq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x24 @addr => "vpmovsxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x25 11 ggg rrr => "vpmovsxdq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x25 @addr => "vpmovsxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x30 11 ggg rrr => "vpmovzxbw " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x30 @addr => "vpmovzxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x31 11 ggg rrr => "vpmovzxbd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x31 @addr => "vpmovzxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x32 11 ggg rrr => "vpmovzxbq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x32 @addr => "vpmovzxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x33 11 ggg rrr => "vpmovzxwd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x33 @addr => "vpmovzxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x34 11 ggg rrr => "vpmovzxwq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x34 @addr => "vpmovzxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x35 11 ggg rrr => "vpmovzxdq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x35 @addr => "vpmovzxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x58 11 ggg rrr => "vpbroadcastd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x58 @addr => "vpbroadcastd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x59 11 ggg rrr => "vpbroadcastq " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x59 @addr => "vpbroadcastq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 10 0x72 11 ggg rrr => "vcvtneps2bf16 " vsse[$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 0 10 0x72 @addr => "vcvtneps2bf16 " vsse[$g] "," $addr ;
  0xc2 0 1111 1 10 0x72 @addr => "vcvtneps2bf16 " vsse[$g] "," "ymmword ptr " $addr ;
  0xc2 0 1111 l 01 0x78 11 ggg rrr => "vpbroadcastb " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x78 @addr => "vpbroadcastb " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x79 11 ggg rrr => "vpbroadcastw " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 1111 l 01 0x79 @addr => "vpbroadcastw " vsse[$l*8+$g] "," $addr ;
  # ---- vpsrlw/d/q shift-by-imm (/digit=2; dest=vvvv, src=rm) ---------------
  0xc1 0 h vvv l 01 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsrlw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsrld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsrlq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  # ---- BMI1/BMI2 : general-purpose 2/3-operand, maps 0F38 (E2) and 0F3A (E3)
  # byte2 = 0 vvvv 0 pp  (W=0, L=0; pp picks the implied 66/F3/F2 that selects
  # the mnemonic).  vvvv is the inverted 2nd source register (reg = 15 - vvvv).
  # order A  dest, vvvv, r/m :
  0xc2 0 h vvv 0 00 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") "andn " greg[$g] "," greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 00 0xf2 @addr      => wit($h==0 ? "vlo" : "") "andn " greg[$g] "," greg[7-$v] "," $addr ;
  0xc2 0 h vvv 0 11 0xf6 11 ggg rrr => wit($h==0 ? "vlo" : "") "mulx " greg[$g] "," greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 11 0xf6 @addr      => wit($h==0 ? "vlo" : "") "mulx " greg[$g] "," greg[7-$v] "," $addr ;
  0xc2 0 h vvv 0 11 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") "pdep " greg[$g] "," greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 11 0xf5 @addr      => wit($h==0 ? "vlo" : "") "pdep " greg[$g] "," greg[7-$v] "," $addr ;
  0xc2 0 h vvv 0 10 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") "pext " greg[$g] "," greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 10 0xf5 @addr      => wit($h==0 ? "vlo" : "") "pext " greg[$g] "," greg[7-$v] "," $addr ;
  # order B  dest, r/m, vvvv :
  0xc2 0 h vvv 0 00 0xf7 11 ggg rrr => wit($h==0 ? "vlo" : "") "bextr " greg[$g] "," greg[$r] "," greg[7-$v] ;
  0xc2 0 h vvv 0 00 0xf7 @addr      => wit($h==0 ? "vlo" : "") "bextr " greg[$g] "," $addr "," greg[7-$v] ;
  0xc2 0 h vvv 0 01 0xf7 11 ggg rrr => wit($h==0 ? "vlo" : "") "shlx " greg[$g] "," greg[$r] "," greg[7-$v] ;
  0xc2 0 h vvv 0 01 0xf7 @addr      => wit($h==0 ? "vlo" : "") "shlx " greg[$g] "," $addr "," greg[7-$v] ;
  0xc2 0 h vvv 0 10 0xf7 11 ggg rrr => wit($h==0 ? "vlo" : "") "sarx " greg[$g] "," greg[$r] "," greg[7-$v] ;
  0xc2 0 h vvv 0 10 0xf7 @addr      => wit($h==0 ? "vlo" : "") "sarx " greg[$g] "," $addr "," greg[7-$v] ;
  0xc2 0 h vvv 0 11 0xf7 11 ggg rrr => wit($h==0 ? "vlo" : "") "shrx " greg[$g] "," greg[$r] "," greg[7-$v] ;
  0xc2 0 h vvv 0 11 0xf7 @addr      => wit($h==0 ? "vlo" : "") "shrx " greg[$g] "," $addr "," greg[7-$v] ;
  0xc2 0 h vvv 0 00 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") "bzhi " greg[$g] "," greg[$r] "," greg[7-$v] ;
  0xc2 0 h vvv 0 00 0xf5 @addr      => wit($h==0 ? "vlo" : "") "bzhi " greg[$g] "," $addr "," greg[7-$v] ;
  # order C  vvvv-dest, r/m  (the ModR/M reg field is the opcode extension) :
  0xc2 0 h vvv 0 00 0xf3 11 001 rrr => wit($h==0 ? "vlo" : "") "blsr " greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 00 0xf3 @addr(1)   => wit($h==0 ? "vlo" : "") "blsr " greg[7-$v] "," $addr ;
  0xc2 0 h vvv 0 00 0xf3 11 010 rrr => wit($h==0 ? "vlo" : "") "blsmsk " greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 00 0xf3 @addr(2)   => wit($h==0 ? "vlo" : "") "blsmsk " greg[7-$v] "," $addr ;
  0xc2 0 h vvv 0 00 0xf3 11 011 rrr => wit($h==0 ? "vlo" : "") "blsi " greg[7-$v] "," greg[$r] ;
  0xc2 0 h vvv 0 00 0xf3 @addr(3)   => wit($h==0 ? "vlo" : "") "blsi " greg[7-$v] "," $addr ;
  # order D  dest, r/m, imm8  (map 0F3A, vvvv unused = 1111) :
  0xc3 0 1111 0 11 0xf0 11 ggg rrr @imm8 => "rorx " greg[$g] "," greg[$r] "," hex($imm8) ;
  0xc3 0 1111 0 11 0xf0 @addr      @imm8 => "rorx " greg[$g] "," $addr "," hex($imm8) ;

  # ---- AVX-512 mask ops (k-registers); maps 0F (E1) and 0F3A (E3) ----------
  0xc1 0 h vvv 1 01 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") "kaddb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") "kaddw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") "kaddd " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") "kaddq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandd " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandnb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandnw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandnd " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") "kandnq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "korb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "korw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "kord " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "korq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxorb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxorw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxord " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxorq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxnorb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxnorw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 01 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxnord " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") "kxnorq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 01 0x4b 11 ggg rrr => wit($h==0 ? "vlo" : "") "kunpckbw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 h vvv 1 00 0x4b 11 ggg rrr => wit($h==0 ? "vlo" : "") "kunpckwd " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 1 h vvv 1 00 0x4b 11 ggg rrr => wit($h==0 ? "vlo" : "") "kunpckdq " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  0xc1 0 1111 0 01 0x44 11 ggg rrr => "knotb " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 00 0x44 11 ggg rrr => "knotw " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 01 0x44 11 ggg rrr => "knotd " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 00 0x44 11 ggg rrr => "knotq " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 01 0x98 11 ggg rrr => "kortestb " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 00 0x98 11 ggg rrr => "kortestw " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 01 0x98 11 ggg rrr => "kortestd " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 00 0x98 11 ggg rrr => "kortestq " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 01 0x99 11 ggg rrr => "ktestb " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 00 0x99 11 ggg rrr => "ktestw " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 01 0x99 11 ggg rrr => "ktestd " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 00 0x99 11 ggg rrr => "ktestq " kreg[$g] "," kreg[$r] ;
  0xc3 0 1111 0 01 0x32 11 ggg rrr @imm8 => "kshiftlb " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 0x32 11 ggg rrr @imm8 => "kshiftlw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 0x33 11 ggg rrr @imm8 => "kshiftld " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 0x33 11 ggg rrr @imm8 => "kshiftlq " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 0x30 11 ggg rrr @imm8 => "kshiftrb " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 0x30 11 ggg rrr @imm8 => "kshiftrw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 0x31 11 ggg rrr @imm8 => "kshiftrd " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 0x31 11 ggg rrr @imm8 => "kshiftrq " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  0xc1 0 1111 0 01 0x90 11 ggg rrr => "kmovb " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 01 0x90 @addr      => "kmovb " kreg[$g] "," $addr ;
  0xc1 0 1111 0 01 0x91 @addr      => "kmovb " $addr "," kreg[$g] ;
  0xc1 0 1111 0 00 0x90 11 ggg rrr => "kmovw " kreg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 00 0x90 @addr      => "kmovw " kreg[$g] "," $addr ;
  0xc1 0 1111 0 00 0x91 @addr      => "kmovw " $addr "," kreg[$g] ;
  0xc1 1 1111 0 01 0x90 11 ggg rrr => "kmovd " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 01 0x90 @addr      => "kmovd " kreg[$g] "," $addr ;
  0xc1 1 1111 0 01 0x91 @addr      => "kmovd " $addr "," kreg[$g] ;
  0xc1 1 1111 0 00 0x90 11 ggg rrr => "kmovq " kreg[$g] "," kreg[$r] ;
  0xc1 1 1111 0 00 0x90 @addr      => "kmovq " kreg[$g] "," $addr ;
  0xc1 1 1111 0 00 0x91 @addr      => "kmovq " $addr "," kreg[$g] ;
  0xc1 0 1111 0 01 0x92 11 ggg rrr => "kmovb " kreg[$g] "," greg[$r] ;
  0xc1 0 1111 0 01 0x93 11 ggg rrr => "kmovb " greg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 00 0x92 11 ggg rrr => "kmovw " kreg[$g] "," greg[$r] ;
  0xc1 0 1111 0 00 0x93 11 ggg rrr => "kmovw " greg[$g] "," kreg[$r] ;
  0xc1 0 1111 0 11 0x92 11 ggg rrr => "kmovd " kreg[$g] "," greg[$r] ;
  0xc1 0 1111 0 11 0x93 11 ggg rrr => "kmovd " greg[$g] "," kreg[$r] ;

  # ---- AVX 3-operand packed/scalar  dest, vvvv, r/m  (L picks xmm/ymm) ------
  0xc1 0 h vvv l 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x12 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovhlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x14 @addr => wit($h==0 ? "vlo" : "") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x14 @addr => wit($h==0 ? "vlo" : "") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x15 @addr => wit($h==0 ? "vlo" : "") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x15 @addr => wit($h==0 ? "vlo" : "") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x16 @addr => wit($h==0 ? "vlo" : "") "vmovhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovlhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x16 @addr => wit($h==0 ? "vlo" : "") "vmovhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x54 @addr => wit($h==0 ? "vlo" : "") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x54 @addr => wit($h==0 ? "vlo" : "") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x55 @addr => wit($h==0 ? "vlo" : "") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x55 @addr => wit($h==0 ? "vlo" : "") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x56 @addr => wit($h==0 ? "vlo" : "") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x56 @addr => wit($h==0 ? "vlo" : "") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x57 @addr => wit($h==0 ? "vlo" : "") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x57 @addr => wit($h==0 ? "vlo" : "") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x58 @addr => wit($h==0 ? "vlo" : "") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x58 @addr => wit($h==0 ? "vlo" : "") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x59 @addr => wit($h==0 ? "vlo" : "") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x59 @addr => wit($h==0 ? "vlo" : "") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x5c @addr => wit($h==0 ? "vlo" : "") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x5c @addr => wit($h==0 ? "vlo" : "") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x5d @addr => wit($h==0 ? "vlo" : "") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x5d @addr => wit($h==0 ? "vlo" : "") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x5e @addr => wit($h==0 ? "vlo" : "") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x5e @addr => wit($h==0 ? "vlo" : "") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 00 0x5f @addr => wit($h==0 ? "vlo" : "") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 00 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x5f @addr => wit($h==0 ? "vlo" : "") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x60 @addr => wit($h==0 ? "vlo" : "") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x60 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x61 @addr => wit($h==0 ? "vlo" : "") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x61 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x62 @addr => wit($h==0 ? "vlo" : "") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x62 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x63 @addr => wit($h==0 ? "vlo" : "") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x63 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x64 @addr => wit($h==0 ? "vlo" : "") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x65 @addr => wit($h==0 ? "vlo" : "") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x66 @addr => wit($h==0 ? "vlo" : "") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x67 @addr => wit($h==0 ? "vlo" : "") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x67 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x68 @addr => wit($h==0 ? "vlo" : "") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x69 @addr => wit($h==0 ? "vlo" : "") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x69 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x6a @addr => wit($h==0 ? "vlo" : "") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x6a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x6b @addr => wit($h==0 ? "vlo" : "") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x6b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x6c @addr => wit($h==0 ? "vlo" : "") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x6c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x6d @addr => wit($h==0 ? "vlo" : "") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x6d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x74 @addr => wit($h==0 ? "vlo" : "") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x75 @addr => wit($h==0 ? "vlo" : "") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x76 @addr => wit($h==0 ? "vlo" : "") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x7c @addr => wit($h==0 ? "vlo" : "") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x7c @addr => wit($h==0 ? "vlo" : "") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0x7d @addr => wit($h==0 ? "vlo" : "") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0x7d @addr => wit($h==0 ? "vlo" : "") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd0 @addr => wit($h==0 ? "vlo" : "") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 11 0xd0 @addr => wit($h==0 ? "vlo" : "") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd4 @addr => wit($h==0 ? "vlo" : "") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd4 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd5 @addr => wit($h==0 ? "vlo" : "") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd5 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd8 @addr => wit($h==0 ? "vlo" : "") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xd9 @addr => wit($h==0 ? "vlo" : "") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xd9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xda @addr => wit($h==0 ? "vlo" : "") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xdb @addr => wit($h==0 ? "vlo" : "") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xdb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xdc @addr => wit($h==0 ? "vlo" : "") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xdd @addr => wit($h==0 ? "vlo" : "") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xde @addr => wit($h==0 ? "vlo" : "") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xdf @addr => wit($h==0 ? "vlo" : "") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe0 @addr => wit($h==0 ? "vlo" : "") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe0 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe3 @addr => wit($h==0 ? "vlo" : "") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe4 @addr => wit($h==0 ? "vlo" : "") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe4 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe5 @addr => wit($h==0 ? "vlo" : "") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe5 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe8 @addr => wit($h==0 ? "vlo" : "") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xe9 @addr => wit($h==0 ? "vlo" : "") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xe9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xea @addr => wit($h==0 ? "vlo" : "") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xea 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xeb @addr => wit($h==0 ? "vlo" : "") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xeb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xec @addr => wit($h==0 ? "vlo" : "") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xec 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xed @addr => wit($h==0 ? "vlo" : "") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xed 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xee @addr => wit($h==0 ? "vlo" : "") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xee 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xef @addr => wit($h==0 ? "vlo" : "") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xef 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf4 @addr => wit($h==0 ? "vlo" : "") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf4 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf5 @addr => wit($h==0 ? "vlo" : "") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf6 @addr => wit($h==0 ? "vlo" : "") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf6 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf8 @addr => wit($h==0 ? "vlo" : "") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xf9 @addr => wit($h==0 ? "vlo" : "") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xf9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xfa @addr => wit($h==0 ? "vlo" : "") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xfa 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xfb @addr => wit($h==0 ? "vlo" : "") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xfb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xfc @addr => wit($h==0 ? "vlo" : "") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xfc 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xfd @addr => wit($h==0 ? "vlo" : "") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xfd 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 01 0xfe @addr => wit($h==0 ? "vlo" : "") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0xfe 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x00 @addr => wit($h==0 ? "vlo" : "") "vpshufb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x00 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshufb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x01 @addr => wit($h==0 ? "vlo" : "") "vphaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x01 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x02 @addr => wit($h==0 ? "vlo" : "") "vphaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x02 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x03 @addr => wit($h==0 ? "vlo" : "") "vphaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x03 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x04 @addr => wit($h==0 ? "vlo" : "") "vpmaddubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x04 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaddubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x05 @addr => wit($h==0 ? "vlo" : "") "vphsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x05 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x06 @addr => wit($h==0 ? "vlo" : "") "vphsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x06 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x07 @addr => wit($h==0 ? "vlo" : "") "vphsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x07 11 ggg rrr => wit($h==0 ? "vlo" : "") "vphsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x08 @addr => wit($h==0 ? "vlo" : "") "vpsignb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x08 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsignb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x09 @addr => wit($h==0 ? "vlo" : "") "vpsignw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x09 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsignw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x0a @addr => wit($h==0 ? "vlo" : "") "vpsignd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x0a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsignd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x0b @addr => wit($h==0 ? "vlo" : "") "vpmulhrsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x0b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmulhrsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x0c @addr => wit($h==0 ? "vlo" : "") "vpermilps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x0c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpermilps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x0d @addr => wit($h==0 ? "vlo" : "") "vpermilpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x0d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpermilpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x16 @addr => wit($h==0 ? "vlo" : "") "vpermps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpermps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x28 @addr => wit($h==0 ? "vlo" : "") "vpmuldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x28 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmuldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x29 @addr => wit($h==0 ? "vlo" : "") "vpcmpeqq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x29 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpeqq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x2b @addr => wit($h==0 ? "vlo" : "") "vpackusdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x2b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpackusdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x2c @addr => wit($h==0 ? "vlo" : "") "vmaskmovps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x2d @addr => wit($h==0 ? "vlo" : "") "vmaskmovpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x36 @addr => wit($h==0 ? "vlo" : "") "vpermd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x36 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpermd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x37 @addr => wit($h==0 ? "vlo" : "") "vpcmpgtq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x37 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpcmpgtq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x38 @addr => wit($h==0 ? "vlo" : "") "vpminsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x38 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x39 @addr => wit($h==0 ? "vlo" : "") "vpminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x39 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3a @addr => wit($h==0 ? "vlo" : "") "vpminuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3b @addr => wit($h==0 ? "vlo" : "") "vpminud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpminud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3c @addr => wit($h==0 ? "vlo" : "") "vpmaxsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3d @addr => wit($h==0 ? "vlo" : "") "vpmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3e @addr => wit($h==0 ? "vlo" : "") "vpmaxuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x3f @addr => wit($h==0 ? "vlo" : "") "vpmaxud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x3f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmaxud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x40 @addr => wit($h==0 ? "vlo" : "") "vpmulld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x40 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmulld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x45 @addr => wit($h==0 ? "vlo" : "") "vpsrlvd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlvd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x45 @addr => wit($h==0 ? "vlo" : "") "vpsrlvq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsrlvq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x46 @addr => wit($h==0 ? "vlo" : "") "vpsravd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsravd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x47 @addr => wit($h==0 ? "vlo" : "") "vpsllvd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllvd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x47 @addr => wit($h==0 ? "vlo" : "") "vpsllvq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpsllvq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 00 0x50 @addr => wit($h==0 ? "vlo" : "") "vpdpbuud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 00 0x50 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbuud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x50 @addr => wit($h==0 ? "vlo" : "") "vpdpbusd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x50 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbusd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 10 0x50 @addr => wit($h==0 ? "vlo" : "") "vpdpbsud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 10 0x50 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbsud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 11 0x50 @addr => wit($h==0 ? "vlo" : "") "vpdpbssd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 11 0x50 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbssd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 00 0x51 @addr => wit($h==0 ? "vlo" : "") "vpdpbuuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 00 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbuuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x51 @addr => wit($h==0 ? "vlo" : "") "vpdpbusds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbusds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 10 0x51 @addr => wit($h==0 ? "vlo" : "") "vpdpbsuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbsuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 11 0x51 @addr => wit($h==0 ? "vlo" : "") "vpdpbssds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpbssds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x52 @addr => wit($h==0 ? "vlo" : "") "vpdpwssd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwssd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x53 @addr => wit($h==0 ? "vlo" : "") "vpdpwssds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwssds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x8c @addr => wit($h==0 ? "vlo" : "") "vpmaskmovd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x8c @addr => wit($h==0 ? "vlo" : "") "vpmaskmovq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x96 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x96 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x97 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x97 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x98 @addr => wit($h==0 ? "vlo" : "") "vfmadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x98 @addr => wit($h==0 ? "vlo" : "") "vfmadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x99 @addr => wit($h==0 ? "vlo" : "") "vfmadd132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x99 @addr => wit($h==0 ? "vlo" : "") "vfmadd132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9a @addr => wit($h==0 ? "vlo" : "") "vfmsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9a @addr => wit($h==0 ? "vlo" : "") "vfmsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9b @addr => wit($h==0 ? "vlo" : "") "vfmsub132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9b @addr => wit($h==0 ? "vlo" : "") "vfmsub132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9c @addr => wit($h==0 ? "vlo" : "") "vfnmadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9c @addr => wit($h==0 ? "vlo" : "") "vfnmadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9d @addr => wit($h==0 ? "vlo" : "") "vfnmadd132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9d @addr => wit($h==0 ? "vlo" : "") "vfnmadd132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9e @addr => wit($h==0 ? "vlo" : "") "vfnmsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub132ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9e @addr => wit($h==0 ? "vlo" : "") "vfnmsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub132pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x9f @addr => wit($h==0 ? "vlo" : "") "vfnmsub132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub132ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0x9f @addr => wit($h==0 ? "vlo" : "") "vfnmsub132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub132sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xa6 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xa6 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xa7 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xa7 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xa8 @addr => wit($h==0 ? "vlo" : "") "vfmadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xa8 @addr => wit($h==0 ? "vlo" : "") "vfmadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xa9 @addr => wit($h==0 ? "vlo" : "") "vfmadd213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xa9 @addr => wit($h==0 ? "vlo" : "") "vfmadd213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xaa @addr => wit($h==0 ? "vlo" : "") "vfmsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xaa @addr => wit($h==0 ? "vlo" : "") "vfmsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xab @addr => wit($h==0 ? "vlo" : "") "vfmsub213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xab @addr => wit($h==0 ? "vlo" : "") "vfmsub213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xac @addr => wit($h==0 ? "vlo" : "") "vfnmadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xac @addr => wit($h==0 ? "vlo" : "") "vfnmadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xad @addr => wit($h==0 ? "vlo" : "") "vfnmadd213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xad @addr => wit($h==0 ? "vlo" : "") "vfnmadd213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xae @addr => wit($h==0 ? "vlo" : "") "vfnmsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub213ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xae @addr => wit($h==0 ? "vlo" : "") "vfnmsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub213pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xaf @addr => wit($h==0 ? "vlo" : "") "vfnmsub213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub213ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xaf @addr => wit($h==0 ? "vlo" : "") "vfnmsub213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub213sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb4 @addr => wit($h==0 ? "vlo" : "") "vpmadd52luq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb4 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmadd52luq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb5 @addr => wit($h==0 ? "vlo" : "") "vpmadd52huq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb5 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpmadd52huq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xb6 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb6 @addr => wit($h==0 ? "vlo" : "") "vfmaddsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmaddsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xb7 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb7 @addr => wit($h==0 ? "vlo" : "") "vfmsubadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsubadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xb8 @addr => wit($h==0 ? "vlo" : "") "vfmadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb8 @addr => wit($h==0 ? "vlo" : "") "vfmadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xb9 @addr => wit($h==0 ? "vlo" : "") "vfmadd231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xb9 @addr => wit($h==0 ? "vlo" : "") "vfmadd231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmadd231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xba @addr => wit($h==0 ? "vlo" : "") "vfmsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xba @addr => wit($h==0 ? "vlo" : "") "vfmsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xbb @addr => wit($h==0 ? "vlo" : "") "vfmsub231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xbb @addr => wit($h==0 ? "vlo" : "") "vfmsub231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfmsub231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xbc @addr => wit($h==0 ? "vlo" : "") "vfnmadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xbc @addr => wit($h==0 ? "vlo" : "") "vfnmadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xbd @addr => wit($h==0 ? "vlo" : "") "vfnmadd231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xbd @addr => wit($h==0 ? "vlo" : "") "vfnmadd231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmadd231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xbe @addr => wit($h==0 ? "vlo" : "") "vfnmsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub231ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xbe @addr => wit($h==0 ? "vlo" : "") "vfnmsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub231pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xbf @addr => wit($h==0 ? "vlo" : "") "vfnmsub231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub231ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 1 h vvv l 01 0xbf @addr => wit($h==0 ? "vlo" : "") "vfnmsub231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 1 h vvv l 01 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vfnmsub231sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xcf @addr => wit($h==0 ? "vlo" : "") "vgf2p8mulb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xcf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vgf2p8mulb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 00 0xd2 @addr => wit($h==0 ? "vlo" : "") "vpdpwuud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 00 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwuud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") "vpdpwusd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwusd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 10 0xd2 @addr => wit($h==0 ? "vlo" : "") "vpdpwsud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 10 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwsud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 00 0xd3 @addr => wit($h==0 ? "vlo" : "") "vpdpwuuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 00 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwuuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") "vpdpwusds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwusds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 10 0xd3 @addr => wit($h==0 ? "vlo" : "") "vpdpwsuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 10 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpdpwsuds " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 00 0xda @addr => wit($h==0 ? "vlo" : "") "vsm3msg1 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 00 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsm3msg1 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xda @addr => wit($h==0 ? "vlo" : "") "vsm3msg2 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsm3msg2 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 10 0xda @addr => wit($h==0 ? "vlo" : "") "vsm4key4 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 10 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsm4key4 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 11 0xda @addr => wit($h==0 ? "vlo" : "") "vsm4rnds4 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 11 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsm4rnds4 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xdc @addr => wit($h==0 ? "vlo" : "") "vaesenc " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaesenc " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xdd @addr => wit($h==0 ? "vlo" : "") "vaesenclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaesenclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xde @addr => wit($h==0 ? "vlo" : "") "vaesdec " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaesdec " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0xdf @addr => wit($h==0 ? "vlo" : "") "vaesdeclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc2 0 h vvv l 01 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") "vaesdeclast " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;

  # ---- AVX 2-op / imm / store forms (same-class, L picks xmm/ymm) --------
  0xc1 0 1111 l 00 0x10 @addr => "vmovups " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x10 11 ggg rrr => "vmovups " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x10 @addr => "vmovupd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x10 11 ggg rrr => "vmovupd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 10 0x10 @addr => "vmovss " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 11 0x10 @addr => "vmovsd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x11 @addr => "vmovups " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 00 0x11 11 ggg rrr => "vmovups " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 1111 l 01 0x11 @addr => "vmovupd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x11 11 ggg rrr => "vmovupd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 1111 l 10 0x11 @addr => "vmovss " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 11 0x11 @addr => "vmovsd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 h vvv l 00 0x12 @addr => wit($h==0 ? "vlo" : "") "vmovlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 01 0x12 @addr => wit($h==0 ? "vlo" : "") "vmovlpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 10 0x12 @addr => "vmovsldup " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0x12 11 ggg rrr => "vmovsldup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 11 0x12 @addr => "vmovddup " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 11 0x12 11 ggg rrr => "vmovddup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x13 @addr => "vmovlps " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x13 @addr => "vmovlpd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 10 0x16 @addr => "vmovshdup " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0x16 11 ggg rrr => "vmovshdup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x17 @addr => "vmovhps " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x17 @addr => "vmovhpd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 00 0x28 @addr => "vmovaps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x28 11 ggg rrr => "vmovaps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x28 @addr => "vmovapd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x28 11 ggg rrr => "vmovapd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x29 @addr => "vmovaps " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 00 0x29 11 ggg rrr => "vmovaps " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 1111 l 01 0x29 @addr => "vmovapd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x29 11 ggg rrr => "vmovapd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 h vvv l 10 0x2a @addr => wit($h==0 ? "vlo" : "") "vcvtsi2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x2a @addr => wit($h==0 ? "vlo" : "") "vcvtsi2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 00 0x2b @addr => "vmovntps " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x2b @addr => "vmovntpd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 00 0x2e @addr => "vucomiss " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x2e 11 ggg rrr => "vucomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x2e @addr => "vucomisd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x2e 11 ggg rrr => "vucomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x2f @addr => "vcomiss " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x2f 11 ggg rrr => "vcomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x2f @addr => "vcomisd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x2f 11 ggg rrr => "vcomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x51 @addr => "vsqrtps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x51 11 ggg rrr => "vsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x51 @addr => "vsqrtpd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x51 11 ggg rrr => "vsqrtpd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x51 @addr => wit($h==0 ? "vlo" : "") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x51 @addr => wit($h==0 ? "vlo" : "") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 00 0x52 @addr => "vrsqrtps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x52 11 ggg rrr => "vrsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x52 @addr => wit($h==0 ? "vlo" : "") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 00 0x53 @addr => "vrcpps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x53 11 ggg rrr => "vrcpps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x53 @addr => wit($h==0 ? "vlo" : "") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x58 @addr => wit($h==0 ? "vlo" : "") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x58 @addr => wit($h==0 ? "vlo" : "") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x59 @addr => wit($h==0 ? "vlo" : "") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x59 @addr => wit($h==0 ? "vlo" : "") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 00 0x5a @addr => "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x5a 11 ggg rrr => "vcvtps2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x5a @addr => "vcvtpd2ps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x5a 11 ggg rrr => "vcvtpd2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5a @addr => wit($h==0 ? "vlo" : "") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x5a @addr => wit($h==0 ? "vlo" : "") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 00 0x5b @addr => "vcvtdq2ps " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 00 0x5b 11 ggg rrr => "vcvtdq2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x5b @addr => "vcvtps2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x5b 11 ggg rrr => "vcvtps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 10 0x5b @addr => "vcvttps2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0x5b 11 ggg rrr => "vcvttps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 h vvv l 10 0x5c @addr => wit($h==0 ? "vlo" : "") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x5c @addr => wit($h==0 ? "vlo" : "") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x5d @addr => wit($h==0 ? "vlo" : "") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x5d @addr => wit($h==0 ? "vlo" : "") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x5e @addr => wit($h==0 ? "vlo" : "") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x5e @addr => wit($h==0 ? "vlo" : "") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 10 0x5f @addr => wit($h==0 ? "vlo" : "") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 h vvv l 11 0x5f @addr => wit($h==0 ? "vlo" : "") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0xc1 0 1111 l 01 0x6e @addr => "vmovd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x6f @addr => "vmovdqa " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0x6f 11 ggg rrr => "vmovdqa " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 10 0x6f @addr => "vmovdqu " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0x6f 11 ggg rrr => "vmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x70 @addr @imm8 => "vpshufd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 0 1111 l 01 0x70 11 ggg rrr @imm8 => "vpshufd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 10 0x70 @addr @imm8 => "vpshufhw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 0 1111 l 10 0x70 11 ggg rrr @imm8 => "vpshufhw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 11 0x70 @addr @imm8 => "vpshuflw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc1 0 1111 l 11 0x70 11 ggg rrr @imm8 => "vpshuflw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 01 0x7e @addr => "vmovd " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 10 0x7e @addr => "vmovq " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0x7e 11 ggg rrr => "vmovq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x7f @addr => "vmovdqa " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0x7f 11 ggg rrr => "vmovdqa " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 1111 l 10 0x7f @addr => "vmovdqu " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 10 0x7f 11 ggg rrr => "vmovdqu " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 h vvv l 00 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 00 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 01 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0xc4 @addr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 00 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 00 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv l 01 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 01 0xd6 @addr => "vmovq " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 01 0xd6 11 ggg rrr => "vmovq " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 1111 l 01 0xe6 @addr => "vcvttpd2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0xe6 11 ggg rrr => "vcvttpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 10 0xe6 @addr => "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 10 0xe6 11 ggg rrr => "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 11 0xe6 @addr => "vcvtpd2dq " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 11 0xe6 11 ggg rrr => "vcvtpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0xe7 @addr => "vmovntdq " $addr "," vsse[$l*8+$g] ;
  0xc1 0 1111 l 11 0xf0 @addr => "vlddqu " vsse[$l*8+$g] "," $addr ;
  0xc1 0 1111 l 01 0xf7 11 ggg rrr => "vmaskmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x0e @addr => "vtestps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x0e 11 ggg rrr => "vtestps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x0f @addr => "vtestpd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x0f 11 ggg rrr => "vtestpd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x13 @addr => "vcvtph2ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x13 11 ggg rrr => "vcvtph2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x17 @addr => "vptest " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x17 11 ggg rrr => "vptest " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x18 @addr => "vbroadcastss " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x18 11 ggg rrr => "vbroadcastss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x19 @addr => "vbroadcastsd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x1c @addr => "vpabsb " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x1c 11 ggg rrr => "vpabsb " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x1d @addr => "vpabsw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x1d 11 ggg rrr => "vpabsw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x1e @addr => "vpabsd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x1e 11 ggg rrr => "vpabsd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x20 @addr => "vpmovsxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x20 11 ggg rrr => "vpmovsxbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x21 @addr => "vpmovsxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x21 11 ggg rrr => "vpmovsxbd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x22 @addr => "vpmovsxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x22 11 ggg rrr => "vpmovsxbq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x23 @addr => "vpmovsxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x23 11 ggg rrr => "vpmovsxwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x24 @addr => "vpmovsxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x24 11 ggg rrr => "vpmovsxwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x25 @addr => "vpmovsxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x25 11 ggg rrr => "vpmovsxdq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x2a @addr => "vmovntdqa " vsse[$l*8+$g] "," $addr ;
  0xc2 0 h vvv l 01 0x2e @addr => wit($h==0 ? "vlo" : "") "vmaskmovps " $addr "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] ;
  0xc2 0 h vvv l 01 0x2f @addr => wit($h==0 ? "vlo" : "") "vmaskmovpd " $addr "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] ;
  0xc2 0 1111 l 01 0x30 @addr => "vpmovzxbw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x30 11 ggg rrr => "vpmovzxbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x31 @addr => "vpmovzxbd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x31 11 ggg rrr => "vpmovzxbd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x32 @addr => "vpmovzxbq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x32 11 ggg rrr => "vpmovzxbq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x33 @addr => "vpmovzxwd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x33 11 ggg rrr => "vpmovzxwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x34 @addr => "vpmovzxwq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x34 11 ggg rrr => "vpmovzxwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x35 @addr => "vpmovzxdq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x35 11 ggg rrr => "vpmovzxdq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x41 @addr => "vphminposuw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x41 11 ggg rrr => "vphminposuw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x58 @addr => "vpbroadcastd " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x58 11 ggg rrr => "vpbroadcastd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x59 @addr => "vpbroadcastq " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x59 11 ggg rrr => "vpbroadcastq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 10 0x72 @addr => "vcvtneps2bf16 " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 10 0x72 11 ggg rrr => "vcvtneps2bf16 " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x78 @addr => "vpbroadcastb " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x78 11 ggg rrr => "vpbroadcastb " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x79 @addr => "vpbroadcastw " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0x79 11 ggg rrr => "vpbroadcastw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 h vvv l 01 0x8e @addr => wit($h==0 ? "vlo" : "") "vpmaskmovd " $addr "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] ;
  0xc2 1 h vvv l 01 0x8e @addr => wit($h==0 ? "vlo" : "") "vpmaskmovq " $addr "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] ;
  0xc2 0 1111 l 00 0xb0 @addr => "vcvtneoph2ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0xb0 @addr => "vcvtneeph2ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 10 0xb0 @addr => "vcvtneebf162ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 11 0xb0 @addr => "vcvtneobf162ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0xb1 @addr => "vbcstnesh2ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 10 0xb1 @addr => "vbcstnebf162ps " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 11 0xcd 11 ggg rrr => "vsha512msg2 " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0xdb @addr => "vaesimc " vsse[$l*8+$g] "," $addr ;
  0xc2 0 1111 l 01 0xdb 11 ggg rrr => "vaesimc " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0xc3 1 1111 l 01 0x00 @addr @imm8 => "vpermq " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x00 11 ggg rrr @imm8 => "vpermq " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 1111 l 01 0x01 @addr @imm8 => "vpermpd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 1 1111 l 01 0x01 11 ggg rrr @imm8 => "vpermpd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x02 @addr @imm8 => wit($h==0 ? "vlo" : "") "vpblendd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x02 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpblendd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x04 @addr @imm8 => "vpermilps " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x04 11 ggg rrr @imm8 => "vpermilps " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x05 @addr @imm8 => "vpermilpd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x05 11 ggg rrr @imm8 => "vpermilpd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x06 @addr @imm8 => wit($h==0 ? "vlo" : "") "vperm2f128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x06 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vperm2f128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x08 @addr @imm8 => "vroundps " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x08 11 ggg rrr @imm8 => "vroundps " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x09 @addr @imm8 => "vroundpd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x09 11 ggg rrr @imm8 => "vroundpd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") "vroundss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vroundss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") "vroundsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vroundsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0c @addr @imm8 => wit($h==0 ? "vlo" : "") "vblendps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0c 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vblendps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0d @addr @imm8 => wit($h==0 ? "vlo" : "") "vblendpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0d 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vblendpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0e @addr @imm8 => wit($h==0 ? "vlo" : "") "vpblendw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpblendw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0f @addr @imm8 => wit($h==0 ? "vlo" : "") "vpalignr " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x0f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpalignr " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x14 @addr @imm8 => "vpextrb " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x15 @addr @imm8 => "vpextrw " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x16 @addr @imm8 => "vpextrd " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x17 @addr @imm8 => "vextractps " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x19 @addr @imm8 => "vextractf128 " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x1d @addr @imm8 => "vcvtps2ph " $addr "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x20 @addr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x21 @addr @imm8 => wit($h==0 ? "vlo" : "") "vinsertps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x21 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vinsertps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x22 @addr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x40 @addr @imm8 => wit($h==0 ? "vlo" : "") "vdpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x40 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vdpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x41 @addr @imm8 => wit($h==0 ? "vlo" : "") "vdppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x41 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vdppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x42 @addr @imm8 => wit($h==0 ? "vlo" : "") "vmpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x42 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vmpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x44 @addr @imm8 => wit($h==0 ? "vlo" : "") "vpclmulqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x44 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpclmulqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x46 @addr @imm8 => wit($h==0 ? "vlo" : "") "vperm2i128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x46 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vperm2i128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x60 @addr @imm8 => "vpcmpestrm " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x60 11 ggg rrr @imm8 => "vpcmpestrm " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x61 @addr @imm8 => "vpcmpestri " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x61 11 ggg rrr @imm8 => "vpcmpestri " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x62 @addr @imm8 => "vpcmpistrm " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x62 11 ggg rrr @imm8 => "vpcmpistrm " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x63 @addr @imm8 => "vpcmpistri " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0x63 11 ggg rrr @imm8 => "vpcmpistri " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") "vgf2p8affineqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0xce 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vgf2p8affineqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 1 h vvv l 01 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") "vgf2p8affineinvqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv l 01 0xcf 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vgf2p8affineinvqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0xde @addr @imm8 => wit($h==0 ? "vlo" : "") "vsm3rnds2 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv l 01 0xde 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vsm3rnds2 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0xdf @addr @imm8 => "vaeskeygenassist " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0xc3 0 1111 l 01 0xdf 11 ggg rrr @imm8 => "vaeskeygenassist " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;

  # ---- FMA4 / blend / 4-operand is4 forms  (4th reg = imm8[7:4]) ---------
  0xc3 0 h vvv l 01 0x4a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vblendvps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x4a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vblendvps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x4b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vblendvpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x4b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vblendvpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x4c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpblendvb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x4c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpblendvb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x5c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x5c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x5c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x5c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x5d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x5d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x5d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x5d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x5e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x5e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x5e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x5e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x5f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x5f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x5f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x5f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x68 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x68 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x68 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x68 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x69 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x69 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x69 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x69 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x6f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x6f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x6f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x6f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x78 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x78 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x78 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x78 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x79 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x79 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x79 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x79 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7a @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7a 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7b @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7b 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7c @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7c 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7d @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7d 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0xc3 0 h vvv l 01 0x7f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0xc3 0 h vvv l 01 0x7f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0xc3 1 h vvv l 01 0x7f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0xc3 1 h vvv l 01 0x7f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vfnmsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;

  # ---- GPR-dest / shift-by-imm(/digit) / mixed-class xmm-ymm forms ------
  0xc1 0 h vvv l 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovss " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 h vvv l 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") "vmovsd " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  0xc1 0 h vvv l 10 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vcvtsi2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] ;
  0xc1 0 h vvv l 11 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vcvtsi2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] ;
  0xc1 0 1111 0 10 0x2c @addr => "vcvttss2si " greg[$g] "," $addr ;
  0xc1 0 1111 l 10 0x2c 11 ggg rrr => "vcvttss2si " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 11 0x2c @addr => "vcvttsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 l 11 0x2c 11 ggg rrr => "vcvttsd2si " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 10 0x2d @addr => "vcvtss2si " greg[$g] "," $addr ;
  0xc1 0 1111 l 10 0x2d 11 ggg rrr => "vcvtss2si " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 0 11 0x2d @addr => "vcvtsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 l 11 0x2d 11 ggg rrr => "vcvtsd2si " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 00 0x50 11 ggg rrr => "vmovmskps " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x50 11 ggg rrr => "vmovmskpd " greg[$g] "," vsse[$l*8+$r] ;
  0xc1 0 1111 l 01 0x6e 11 ggg rrr => "vmovd " vsse[$l*8+$g] "," greg[$r] ;
  0xc1 0 h vvv l 01 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsraw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsllw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsrad " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpslld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsrldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpsllq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 h vvv l 01 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") "vpslldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 01 0x7e 11 ggg rrr => "vmovd " greg[$r] "," vsse[$l*8+$g] ;
  0xc1 0 h vvv l 01 0xc4 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc1 0 1111 l 01 0xc5 11 ggg rrr @imm8 => "vpextrw " greg[$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0xc1 0 1111 l 01 0xd7 11 ggg rrr => "vpmovmskb " greg[$g] "," vsse[$l*8+$r] ;
  0xc2 0 1111 l 01 0x19 11 ggg rrr => "vbroadcastsd " vsse[$l*8+$g] "," vsse[$r] ;
  0xc2 0 h vvv l 11 0xcb 11 ggg rrr => wit($h==0 ? "vlo" : "") "vsha512rnds2 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  0xc2 0 1111 l 11 0xcc 11 ggg rrr => "vsha512msg1 " vsse[$l*8+$g] "," vsse[$r] ;
  0xc3 0 1111 l 01 0x14 11 ggg rrr @imm8 => "vpextrb " greg[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x15 11 ggg rrr @imm8 => "vpextrw " greg[$r] "," vsse[$l*8+$g] "," hex($imm8) wit("alt") ;
  0xc3 0 1111 l 01 0x16 11 ggg rrr @imm8 => "vpextrd " greg[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x17 11 ggg rrr @imm8 => "vextractps " greg[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x18 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vinsertf128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x19 11 ggg rrr @imm8 => "vextractf128 " vsse[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x1d 11 ggg rrr @imm8 => "vcvtps2ph " vsse[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x20 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x22 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpinsrd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc3 0 h vvv l 01 0x38 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vinserti128 " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] "," hex($imm8) ;
  0xc3 0 1111 l 01 0x39 11 ggg rrr @imm8 => "vextracti128 " vsse[$r] "," vsse[$l*8+$g] "," hex($imm8) ;
  # ---- no-operand / mem-only(/digit) / mixed-class 128-bit mem forms -------
  0xc1 0 1111 0 00 0x77 => "vzeroupper" ;
  0xc1 0 1111 1 00 0x77 => "vzeroall" ;
  0xc1 0 1111 0 00 0xae @addr(2) => "vldmxcsr " $addr ;
  0xc1 0 1111 0 00 0xae @addr(3) => "vstmxcsr " $addr ;
  0xc2 0 1111 1 01 0x1a @addr => "vbroadcastf128 " vsse[8+$g] "," $addr ;
  0xc2 0 1111 1 01 0x5a @addr => "vbroadcasti128 " vsse[8+$g] "," $addr ;
  0xc3 0 1111 1 01 0x39 @addr @imm8 => "vextracti128 " $addr "," vsse[8+$g] "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x18 @addr @imm8 => wit($h==0 ? "vlo" : "") "vinsertf128 " vsse[8+$g] "," vsse[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 1 01 0x38 @addr @imm8 => wit($h==0 ? "vlo" : "") "vinserti128 " vsse[8+$g] "," vsse[8+(7-$v)] "," $addr "," hex($imm8) ;
  # ---- vpermil2ps/pd  4-operand is4 + 2-bit selector (is4 = 0sss iiii) ----
  0xc3 0 h vvv l 01 0x48 11 ggg rrr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] "," hex($i) ;
  0xc3 0 h vvv l 01 0x48 @addr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] "," hex($i) ;
  0xc3 1 h vvv l 01 0x48 11 ggg rrr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] "," hex($i) wit("alt") ;
  0xc3 1 h vvv l 01 0x48 @addr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2ps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr "," hex($i) ;
  0xc3 0 h vvv l 01 0x49 11 ggg rrr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] "," hex($i) ;
  0xc3 0 h vvv l 01 0x49 @addr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] "," hex($i) ;
  0xc3 1 h vvv l 01 0x49 11 ggg rrr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] "," hex($i) wit("alt") ;
  0xc3 1 h vvv l 01 0x49 @addr 0 sss iiii => wit($h==0 ? "vlo" : "") "vpermil2pd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr "," hex($i) ;

  # ---- VSIB gather  dest, [base+vindex*scale], mask  (vector SIB index) -
  0xc2 0 h vvv l 01 0x90 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vpgatherdd " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
  0xc2 1 h vvv l 01 0x90 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vpgatherdq " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
  0xc2 0 h vvv l 01 0x91 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vpgatherqd " vsse[$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[(7-$v)] ;
  0xc2 1 h vvv l 01 0x91 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vpgatherqq " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
  0xc2 0 h vvv l 01 0x92 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vgatherdps " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
  0xc2 1 h vvv l 01 0x92 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vgatherdpd " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
  0xc2 0 h vvv l 01 0x93 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vgatherqps " vsse[$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[(7-$v)] ;
  0xc2 1 h vvv l 01 0x93 00 ggg 100 cc xxx bbb => wit($h==0 ? "vlo" : "") "vgatherqpd " vsse[$l*8+$g] ",[" greg[$b] "+" vsse[$l*8+$x] "*" sc[$c] "]," vsse[$l*8+(7-$v)] ;
}

submatch evex {
  # ---- SLICE-0: b=0 reg-reg, slot-derived; wit("evex"); store forms wit("alt") ----
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vmovups " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x11 11 ggg rrr => wit("evex") wit("alt") "vmovups " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vmovaps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") wit("alt") "vmovaps " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomiss " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomiss " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2udq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 00 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2udq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 01 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtps2dq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 01 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqa32 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqa32 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 0 1111 0 10 z ll 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vmovsldup " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 10 z ll 0 1 aaa 0x16 11 ggg rrr => wit("evex") "vmovshdup " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 10 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttps2dq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 10 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu32 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 10 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu32 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 0 1111 0 11 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu8 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 11 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 0 1111 0 11 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu8 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x12 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmovhlps " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmovlhps " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 00 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x60 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklbw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x61 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklwd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x62 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x63 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpacksswb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x67 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpackuswb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhbw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x69 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhwd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x6a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0x6b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xd5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xd8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xd9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminub " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xdb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxub " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhuw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xe9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xea 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xeb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xec 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xed 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xee 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xef 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddwd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xf6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsadbw " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xf8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xf9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xfa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xfc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xfd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 01 z ll 0 1 aaa 0xfe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmovss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("alt") "vmovss " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$g] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 0 h vvv 0 10 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vmovupd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x11 11 ggg rrr => wit("evex") wit("alt") "vmovupd " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vmovapd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") wit("alt") "vmovapd " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomisd " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomisd " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqa64 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2uqq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2uqq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttpd2qq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtpd2qq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqa64 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 1 1111 0 01 z ll 0 1 aaa 0xd6 11 ggg rrr => wit("evex") wit("alt") "vmovq " zreg[$l*8+$r] "," zreg[$l*8+$g] ;
  0xc1 1 1111 0 10 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu64 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 10 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 10 z ll 0 1 aaa 0x7e 11 ggg rrr => wit("evex") "vmovq " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 10 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu64 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 1 1111 0 10 z ll 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtqq2pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 11 z ll 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vmovddup " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 11 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu16 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc1 1 1111 0 11 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu16 " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x6c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0x6d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xd4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xdb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xeb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xef 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xf4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 01 z ll 0 1 aaa 0xfb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmovsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("alt") "vmovsd " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$g] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc1 1 h vvv 0 11 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x1c 11 ggg rrr => wit("evex") "vpabsb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vpabsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x1e 11 ggg rrr => wit("evex") "vpabsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x44 11 ggg rrr => wit("evex") "vplzcntd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcp14ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrt14ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x54 11 ggg rrr => wit("evex") "vpopcntb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x55 11 ggg rrr => wit("evex") "vpopcntd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x62 11 ggg rrr => wit("evex") "vpexpandb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x63 11 ggg rrr => wit("evex") "vpcompressb " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x88 11 ggg rrr => wit("evex") "vexpandps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x89 11 ggg rrr => wit("evex") "vpexpandd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x8a 11 ggg rrr => wit("evex") "vcompressps " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0x8b 11 ggg rrr => wit("evex") "vpcompressd " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0xc4 11 ggg rrr => wit("evex") "vpconflictd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 1111 0 01 z ll 0 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x00 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x04 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddubsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x0b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhrsw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x0c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x2b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x36 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x38 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x39 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x3f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x40 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x4d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp14ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x4f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt14ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x50 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x71 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x73 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2b " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x77 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2b " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x7e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x8d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xcb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xcd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xcf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8mulb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenc " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenclast " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdec " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 01 z ll 0 1 aaa 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdeclast " zreg[$l*8+$g] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 10 z ll 0 1 aaa 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 0 h vvv 0 11 z ll 0 1 aaa 0x72 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x1f 11 ggg rrr => wit("evex") "vpabsq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexppd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x44 11 ggg rrr => wit("evex") "vplzcntq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcp14pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrt14pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x54 11 ggg rrr => wit("evex") "vpopcntw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x55 11 ggg rrr => wit("evex") "vpopcntq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x62 11 ggg rrr => wit("evex") "vpexpandw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x63 11 ggg rrr => wit("evex") "vpcompressw " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x88 11 ggg rrr => wit("evex") "vexpandpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x89 11 ggg rrr => wit("evex") "vpexpandq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x8a 11 ggg rrr => wit("evex") "vcompresspd " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0x8b 11 ggg rrr => wit("evex") "vpcompressq " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+$g] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0xc4 11 ggg rrr => wit("evex") "vpconflictq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 1111 0 01 z ll 0 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x0d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x12 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x28 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x36 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x39 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x3b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x3d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x3f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x40 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x4d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp14sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x4f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt14sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x70 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x71 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x72 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x73 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2w " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x77 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2w " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x7e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x83 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x8d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpermw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xcb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc2 1 h vvv 0 01 z ll 0 1 aaa 0xcd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 00 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomish " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 00 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomish " zreg[$l*8+$g] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 00 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 00 z ll 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2uw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 00 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2uw " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 01 z ll 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2w " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 01 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2w " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 10 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtw2ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 1111 0 11 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtuw2ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x1d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 00 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmovsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("alt") "vmovsh " zreg[$l*8+$r] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$g] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2sd " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 0 h vvv 0 10 z ll 0 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc5 1 h vvv 0 11 z ll 0 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 1111 0 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc6 0 1111 0 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcpph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc6 0 1111 0 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrtph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 00 z ll 0 1 aaa 0x13 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2ss " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x4d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcpsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x4f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrtsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 01 z ll 0 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 10 z ll 0 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 10 z ll 0 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 10 z ll 0 1 aaa 0xd6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 10 z ll 0 1 aaa 0xd7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 11 z ll 0 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 11 z ll 0 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 11 z ll 0 1 aaa 0xd6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  0xc6 0 h vvv 0 11 z ll 0 1 aaa 0xd7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcsh " zreg[$l*8+$g] kzdec[$z*8+$a] "," zreg[$l*8+(7-$v)] "," zreg[$l*8+$r] ;
  # ---- b=0 remainder: mixed-width / k-dest / gpr; W-collision->altw, store->alt ----
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x6e 11 ggg rrr => wit("evex") "vmovd " zreg[0+$g] "," greg[$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7e 11 ggg rrr => wit("evex") wit("alt") "vmovd " greg[$r] "," zreg[0+$g] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttss2si " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtss2si " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttss2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtss2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtdq2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtdq2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtdq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttsd2si " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtsd2si " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttsd2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtsd2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 0 h vvv 0 11 z 00 0 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 0 h vvv 0 11 z 00 0 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x6e 11 ggg rrr => wit("evex") wit("altw") "vmovd " zreg[0+$g] "," greg[$r] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x7e 11 ggg rrr => wit("evex") wit("altw") wit("alt") "vmovd " greg[$r] "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x2c 11 ggg rrr => wit("evex") wit("altw") "vcvttss2si " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x2d 11 ggg rrr => wit("evex") wit("altw") "vcvtss2si " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") wit("altw") "vcvttss2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") wit("altw") "vcvtss2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x2c 11 ggg rrr => wit("evex") wit("altw") "vcvttsd2si " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x2d 11 ggg rrr => wit("evex") wit("altw") "vcvtsd2si " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") wit("altw") "vcvttsd2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") wit("altw") "vcvtsd2usi " greg[$g] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] ;
  0xc1 1 h vvv 0 10 z 00 0 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 1 h vvv 0 10 z 00 0 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtsi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtusi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x18 11 ggg rrr => wit("evex") "vbroadcastss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovsxbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsxbd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsxbq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsxwd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsxwq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsxdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovzxbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovzxbd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovzxbq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovzxwd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovzxwq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovzxdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x58 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vbroadcasti32x2 " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[0+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[0+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[0+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x18 11 ggg rrr => wit("evex") "vbroadcastss " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastf32x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovsxbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsxbd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsxbq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsxwd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsxwq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsxdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovzxbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovzxbd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovzxbq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovzxwd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovzxwq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovzxdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x58 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vbroadcasti32x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[8+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[8+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[8+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x18 11 ggg rrr => wit("evex") "vbroadcastss " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastf32x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovsxbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsxbd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsxbq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsxwd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsxwq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsxdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovzxbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovzxbd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovzxbq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovzxwd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovzxwq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovzxdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x58 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vbroadcasti32x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vpbroadcastb " zreg[16+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vpbroadcastw " zreg[16+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vpbroadcastd " zreg[16+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vpmovuswb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x11 11 ggg rrr => wit("evex") "vpmovusdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vpmovusqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vpmovusdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x14 11 ggg rrr => wit("evex") "vpmovusqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x15 11 ggg rrr => wit("evex") "vpmovusqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovswb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2b " zreg[0+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovb2m " kreg[$g] "," zreg[0+$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovwb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2d " zreg[0+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovd2m " kreg[$g] "," zreg[0+$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x3a 11 ggg rrr => wit("evex") "vpbroadcastmw2d " zreg[0+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x72 11 ggg rrr => wit("evex") "vcvtneps2bf16 " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vpmovuswb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x11 11 ggg rrr => wit("evex") "vpmovusdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vpmovusqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vpmovusdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x14 11 ggg rrr => wit("evex") "vpmovusqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x15 11 ggg rrr => wit("evex") "vpmovusqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovswb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2b " zreg[8+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovb2m " kreg[$g] "," zreg[8+$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovwb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovdw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovqd " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2d " zreg[8+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovd2m " kreg[$g] "," zreg[8+$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x3a 11 ggg rrr => wit("evex") "vpbroadcastmw2d " zreg[8+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x72 11 ggg rrr => wit("evex") "vcvtneps2bf16 " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vpmovuswb " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x11 11 ggg rrr => wit("evex") "vpmovusdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vpmovusqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vpmovusdw " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x14 11 ggg rrr => wit("evex") "vpmovusqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x15 11 ggg rrr => wit("evex") "vpmovusqd " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovswb " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsdw " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsqd " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2b " zreg[16+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovb2m " kreg[$g] "," zreg[16+$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovwb " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovdb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovqb " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovdw " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovqw " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovqd " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2d " zreg[16+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovd2m " kreg[$g] "," zreg[16+$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x3a 11 ggg rrr => wit("evex") "vpbroadcastmw2d " zreg[16+$g] "," kreg[$r] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x72 11 ggg rrr => wit("evex") "vcvtneps2bf16 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x8f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x8f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x8f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 10 z 00 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 10 z 00 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 10 z 01 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 10 z 01 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 10 z 10 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 10 z 10 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 0 h vvv 0 11 z 00 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 0 h vvv 0 11 z 01 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vpbroadcastq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x7c 11 ggg rrr => wit("evex") wit("altw") "vpbroadcastd " zreg[0+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastsd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vpbroadcastq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x7c 11 ggg rrr => wit("evex") wit("altw") "vpbroadcastd " zreg[8+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastsd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vpbroadcastq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x7c 11 ggg rrr => wit("evex") wit("altw") "vpbroadcastd " zreg[16+$g] kzdec[$z*8+$a] "," greg[$r] ;
  0xc2 1 1111 0 10 z 00 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2w " zreg[0+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 00 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovw2m " kreg[$g] "," zreg[0+$r] ;
  0xc2 1 1111 0 10 z 00 0 1 aaa 0x2a 11 ggg rrr => wit("evex") "vpbroadcastmb2q " zreg[0+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 00 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2q " zreg[0+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 00 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovq2m " kreg[$g] "," zreg[0+$r] ;
  0xc2 1 1111 0 10 z 01 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2w " zreg[8+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 01 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovw2m " kreg[$g] "," zreg[8+$r] ;
  0xc2 1 1111 0 10 z 01 0 1 aaa 0x2a 11 ggg rrr => wit("evex") "vpbroadcastmb2q " zreg[8+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 01 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2q " zreg[8+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 01 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovq2m " kreg[$g] "," zreg[8+$r] ;
  0xc2 1 1111 0 10 z 10 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2w " zreg[16+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 10 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovw2m " kreg[$g] "," zreg[16+$r] ;
  0xc2 1 1111 0 10 z 10 0 1 aaa 0x2a 11 ggg rrr => wit("evex") "vpbroadcastmb2q " zreg[16+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 10 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2q " zreg[16+$g] "," kreg[$r] ;
  0xc2 1 1111 0 10 z 10 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovq2m " kreg[$g] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x29 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x37 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x29 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x37 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x29 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x37 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 10 z 00 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 10 z 00 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 10 z 01 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 10 z 01 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 10 z 10 0 1 aaa 0x26 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 10 z 10 0 1 aaa 0x27 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc2 1 h vvv 0 11 z 00 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[0+(7-$v)] "," zreg[0+$r] ;
  0xc2 1 h vvv 0 11 z 01 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[8+(7-$v)] "," zreg[8+$r] ;
  0xc2 1 h vvv 0 11 z 10 0 1 aaa 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[16+(7-$v)] "," zreg[16+$r] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x6e 11 ggg rrr => wit("evex") "vmovw " zreg[0+$g] "," greg[$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7e 11 ggg rrr => wit("evex") wit("alt") "vmovw " greg[$r] "," zreg[0+$g] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttsh2si " greg[$g] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtsh2si " greg[$g] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttsh2usi " greg[$g] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtsh2usi " greg[$g] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 01 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 10 z 10 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 11 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 0 1111 0 11 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 0 1111 0 11 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] ;
  0xc5 1 1111 0 00 z 00 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 1 1111 0 00 z 01 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 1 1111 0 00 z 10 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc5 1 1111 0 01 z 00 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 1 1111 0 01 z 01 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 1 1111 0 01 z 10 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc5 1 1111 0 11 z 00 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc5 1 1111 0 11 z 01 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc5 1 1111 0 11 z 10 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] ;
  0xc6 0 1111 0 01 z 00 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc6 0 1111 0 01 z 01 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " zreg[8+$g] kzdec[$z*8+$a] "," zreg[0+$r] ;
  0xc6 0 1111 0 01 z 10 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  # ---- b=1: {r*-sae}/{sae}; gpr; undecorated=redundant b->alt; W-coll->altw ----
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomiss " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomiss " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 0 1111 0 00 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 1111 0 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 1111 0 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc1 0 1111 0 01 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtps2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 1111 0 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] rcdec[$l] ;
  0xc1 0 1111 0 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] rcdec[$l] ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttss2si " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttps2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttss2usi " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") wit("alt") "vcvtudq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0xe6 11 ggg rrr => wit("evex") wit("alt") "vcvtdq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] ;
  0xc1 0 1111 0 10 z ll 1 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtss2si " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 1111 0 10 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtss2usi " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 1111 0 11 z 00 1 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttsd2si " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 11 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttsd2usi " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 0 1111 0 11 z ll 1 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtsd2si " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 1111 0 11 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtsd2usi " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 1111 0 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 00 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 00 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 00 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z 00 1 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc1 0 h vvv 0 10 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc1 0 h vvv 0 10 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc1 0 h vvv 0 10 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpss " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 0 h vvv 0 10 z ll 1 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc1 1 1111 0 00 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 1 1111 0 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomisd " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomisd " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc1 1 1111 0 01 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 01 z ll 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 10 z 00 1 1 aaa 0x2c 11 ggg rrr => wit("evex") wit("altw") "vcvttss2si " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 10 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") wit("altw") "vcvttss2usi " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 10 z ll 1 1 aaa 0x2d 11 ggg rrr => wit("evex") wit("altw") "vcvtss2si " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 1111 0 10 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") wit("altw") "vcvtss2usi " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 1111 0 10 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 10 z ll 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 11 z 00 1 1 aaa 0x2c 11 ggg rrr => wit("evex") wit("altw") "vcvttsd2si " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 11 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") wit("altw") "vcvttsd2usi " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc1 1 1111 0 11 z ll 1 1 aaa 0x2d 11 ggg rrr => wit("evex") wit("altw") "vcvtsd2si " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 1111 0 11 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") wit("altw") "vcvtsd2usi " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 1111 0 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 1111 0 11 z ll 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 01 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 01 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 01 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 10 z ll 1 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc1 1 h vvv 0 10 z ll 1 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc1 1 h vvv 0 11 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc1 1 h vvv 0 11 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc1 1 h vvv 0 11 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xcb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xcd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 0 h vvv 0 01 z ll 1 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexppd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xcb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xcd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc2 1 h vvv 0 01 z ll 1 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 1 1 aaa 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 1 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 1 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscaless " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x51 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x55 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 0 h vvv 0 10 z 00 1 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsh " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x51 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x55 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" "," hex($imm8) ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomish " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomish " zreg[0+$g] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2uw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc5 0 1111 0 00 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] rcdec[$l] ;
  0xc5 0 1111 0 00 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2uw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2w " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc5 0 1111 0 01 z ll 1 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 01 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] rcdec[$l] ;
  0xc5 0 1111 0 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 1111 0 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 1111 0 01 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2w " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 10 z 00 1 1 aaa 0x2c 11 ggg rrr => wit("evex") "vcvttsh2si " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 10 z 00 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc5 0 1111 0 10 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttsh2usi " greg[$g] "," zreg[0+$r] " {sae}" ;
  0xc5 0 1111 0 10 z ll 1 1 aaa 0x2d 11 ggg rrr => wit("evex") "vcvtsh2si " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 1111 0 10 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtsh2usi " greg[$g] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 1111 0 10 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtw2ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 1111 0 11 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtuw2ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] " {sae}" ;
  0xc5 0 h vvv 0 00 z ll 1 1 aaa 0x1d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 00 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 00 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 00 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 00 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z 00 1 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc5 0 h vvv 0 10 z 00 1 1 aaa 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vminsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc5 0 h vvv 0 10 z 00 1 1 aaa 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc5 0 h vvv 0 10 z ll 1 1 aaa 0x7b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] rcdec[$l] ;
  0xc5 1 1111 0 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 1 1111 0 01 z ll 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 1 1111 0 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[16+$r] rcdec[$l] ;
  0xc5 1 h vvv 0 11 z ll 1 1 aaa 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " zreg[16+$g] kzdec[$z*8+$a] "," zreg[8+$r] " {sae}" ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] " {sae}" ;
  0xc6 0 h vvv 0 00 z 00 1 1 aaa 0x13 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x43 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] " {sae}" ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x2c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x2d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0x9f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xa6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xa7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xa8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xa9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xaa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xab 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xac 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xad 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xae 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xaf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xb6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xb7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xb8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xb9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xba 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xbb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xbc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xbd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xbe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 01 z ll 1 1 aaa 0xbf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 10 z ll 1 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 10 z ll 1 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 10 z ll 1 1 aaa 0xd6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 10 z ll 1 1 aaa 0xd7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 11 z ll 1 1 aaa 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 11 z ll 1 1 aaa 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 11 z ll 1 1 aaa 0xd6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] rcdec[$l] ;
  0xc6 0 h vvv 0 11 z ll 1 1 aaa 0xd7 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] rcdec[$l] ;
  # ---- b=0 reg-reg imm8 (7-byte): /digit detected via multi-mnemonic opcodes ----
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0xc5 11 ggg rrr @imm8 => wit("evex") "vpextrw " greg[$g] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufhw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufhw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufhw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshuflw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 1111 0 11 z 01 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshuflw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 1111 0 11 z 10 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshuflw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xc4 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrw " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpss " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 11 000 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 11 001 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassph " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclasssh " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassph " kreg[$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassph " kreg[$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x04 11 ggg rrr @imm8 => wit("evex") "vpermilps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x14 11 ggg rrr @imm8 => wit("evex") "vpextrb " greg[$r] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x15 11 ggg rrr @imm8 => wit("evex") wit("alt") "vpextrw " greg[$r] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x16 11 ggg rrr @imm8 => wit("evex") "vpextrd " greg[$r] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x17 11 ggg rrr @imm8 => wit("evex") "vextractps " greg[$r] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " zreg[0+$r] kzdec[$z*8+$a] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassps " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclassss " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x04 11 ggg rrr @imm8 => wit("evex") "vpermilps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf32x4 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti32x4 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassps " kreg[$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x04 11 ggg rrr @imm8 => wit("evex") "vpermilps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf32x4 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x1b 11 ggg rrr @imm8 => wit("evex") "vextractf32x8 " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti32x4 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x3b 11 ggg rrr @imm8 => wit("evex") "vextracti32x8 " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassps " kreg[$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 01 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 10 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x0a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscaless " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x0f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x20 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrb " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x21 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertps " zreg[0+$g] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x22 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x42 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x44 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[0+$g] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x51 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x55 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x0f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x18 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x23 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x38 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x42 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x43 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x44 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[8+$g] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x0f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x18 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x8 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x23 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x38 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x8 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x42 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x43 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x44 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[16+$g] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 0 h vvv 0 10 z 00 0 1 aaa 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsh " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x05 11 ggg rrr @imm8 => wit("evex") "vpermilpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x16 11 ggg rrr @imm8 => wit("evex") wit("altw") "vpextrd " greg[$r] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclasspd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclasssd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x00 11 ggg rrr @imm8 => wit("evex") "vpermq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x01 11 ggg rrr @imm8 => wit("evex") "vpermpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x05 11 ggg rrr @imm8 => wit("evex") "vpermilpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf64x2 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti64x2 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclasspd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x00 11 ggg rrr @imm8 => wit("evex") "vpermq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x01 11 ggg rrr @imm8 => wit("evex") "vpermpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x05 11 ggg rrr @imm8 => wit("evex") "vpermilpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf64x2 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x1b 11 ggg rrr @imm8 => wit("evex") "vextractf64x4 " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti64x2 " zreg[0+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x3b 11 ggg rrr @imm8 => wit("evex") "vextracti64x4 " zreg[8+$r] kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclasspd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x0b 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x22 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vpinsrd " zreg[0+$g] "," zreg[0+(7-$v)] "," greg[$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x27 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x51 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x55 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x57 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x70 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x72 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0xce 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0xcf 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x18 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x23 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x38 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x43 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x70 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x72 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0xce 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0xcf 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x03 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x18 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x23 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x25 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x38 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[0+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3a 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[8+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3e 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3f 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x43 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x50 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x54 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x70 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x71 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x72 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x73 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0xce 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0xcf 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," zreg[16+$r] "," hex($imm8) ;
  # ---- b=0 mem (plain [eax]): @addr; mask-on-mem; /digit->@addr(d); vvvv->[$v>=8]; .x/.y/.z for LL-ambig ----
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x10 @addr => wit("evex") "vmovups " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x11 @addr => wit("evex") "vmovups " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x13 @addr => wit("evex") "vmovlps " $addr "," zreg[0+$g] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x17 @addr => wit("evex") "vmovhps " $addr "," zreg[0+$g] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x28 @addr => wit("evex") "vmovaps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x29 @addr => wit("evex") "vmovaps " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x2b @addr => wit("evex") "vmovntps " $addr "," zreg[0+$g] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x2e @addr => wit("evex") "vucomiss " zreg[0+$g] "," $addr ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x2f @addr => wit("evex") "vcomiss " zreg[0+$g] "," $addr ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 00 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x10 @addr => wit("evex") "vmovups " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x11 @addr => wit("evex") "vmovups " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x28 @addr => wit("evex") "vmovaps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x29 @addr => wit("evex") "vmovaps " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x2b @addr => wit("evex") "vmovntps " $addr "," zreg[8+$g] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 1 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 1 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 00 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x10 @addr => wit("evex") "vmovups " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x11 @addr => wit("evex") "vmovups " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x28 @addr => wit("evex") "vmovaps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x29 @addr => wit("evex") "vmovaps " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x2b @addr => wit("evex") "vmovntps " $addr "," zreg[16+$g] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 1 1 aaa 0x51 @addr => wit("evex") "vsqrtps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 1 1 aaa 0x5a @addr => wit("evex") "vcvtps2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 00 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 00 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x6e @addr => wit("evex") "vmovd " zreg[0+$g] "," $addr ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa32 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 00 1 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7e @addr => wit("evex") "vmovd " $addr "," zreg[0+$g] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa32 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 01 z 00 0 1 aaa 0xe7 @addr => wit("evex") "vmovntdq " $addr "," zreg[0+$g] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa32 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 01 1 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa32 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 0 1111 0 01 z 01 0 1 aaa 0xe7 @addr => wit("evex") "vmovntdq " $addr "," zreg[8+$g] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtps2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa32 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtps2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvttps2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 01 z 10 1 1 aaa 0x7b @addr => wit("evex") "vcvtps2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa32 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 0 1111 0 01 z 10 0 1 aaa 0xe7 @addr => wit("evex") "vmovntdq " $addr "," zreg[16+$g] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x10 @addr => wit("evex") "vmovss " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x11 @addr => wit("evex") "vmovss " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x12 @addr => wit("evex") "vmovsldup " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x16 @addr => wit("evex") "vmovshdup " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x2c @addr => wit("evex") "vcvttss2si " greg[$g] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x2d @addr => wit("evex") "vcvtss2si " greg[$g] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu32 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufhw " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttss2usi " greg[$g] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtss2usi " greg[$g] "," $addr ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu32 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 10 z 00 0 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 00 1 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x12 @addr => wit("evex") "vmovsldup " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x16 @addr => wit("evex") "vmovshdup " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu32 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufhw " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu32 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 0 1111 0 10 z 01 0 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 01 1 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x12 @addr => wit("evex") "vmovsldup " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x16 @addr => wit("evex") "vmovshdup " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvttps2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu32 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshufhw " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu32 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 0 1111 0 10 z 10 0 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 10 z 10 1 1 aaa 0xe6 @addr => wit("evex") "vcvtdq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x2c @addr => wit("evex") "vcvttsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x2d @addr => wit("evex") "vcvtsd2si " greg[$g] "," $addr ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu8 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshuflw " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttsd2usi " greg[$g] "," $addr ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtsd2usi " greg[$g] "," $addr ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc1 0 1111 0 11 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu8 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 0 1111 0 11 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu8 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 01 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshuflw " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 11 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc1 0 1111 0 11 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu8 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 0 1111 0 11 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu8 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 10 0 1 aaa 0x70 @addr @imm8 => wit("evex") "vpshuflw " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 1111 0 11 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 0 1111 0 11 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc1 0 1111 0 11 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu8 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x12 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmovlps " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmovhps " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 00 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 00 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 01 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpps " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 00 z 10 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x60 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x61 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklwd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x63 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpacksswb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x67 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackuswb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x69 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhwd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x71 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x73 @addr(3) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x73 @addr(7) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x74 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xc4 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrw " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xd9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xda @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminub " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxub " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe0 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhuw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xe9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xea @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xec @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xed @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xee @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddwd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsadbw " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xf9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xfc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xfd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 0 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 00 1 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x60 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x61 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklwd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x63 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpacksswb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x67 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackuswb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x69 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhwd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x71 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x73 @addr(3) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x73 @addr(7) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x74 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xd9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xda @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminub " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxub " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe0 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhuw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xe9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xea @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xec @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xed @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xee @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddwd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsadbw " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xf9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xfc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xfd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 0 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 01 1 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x60 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x61 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklwd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x62 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x63 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpacksswb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x67 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackuswb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x69 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhwd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x6a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x6b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackssdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x71 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprord " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprold " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x73 @addr(3) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrldq " zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x73 @addr(7) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpslldq " zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x74 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xd9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubusw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xda @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminub " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddusw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxub " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe0 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrad " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpavgw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhuw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xe9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xea @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpord " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xec @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xed @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xee @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxord " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpslld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddwd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsadbw " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xf9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xfa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xfc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xfd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 0 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 01 z 10 1 1 aaa 0xfe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x2a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x5a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0x7b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 10 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpss " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 0 h vvv 0 11 z 00 0 1 aaa 0x2a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 0 h vvv 0 11 z 00 0 1 aaa 0x7b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 00 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 00 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 00 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 00 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x10 @addr => wit("evex") "vmovupd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x11 @addr => wit("evex") "vmovupd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x13 @addr => wit("evex") "vmovlpd " $addr "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x17 @addr => wit("evex") "vmovhpd " $addr "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x28 @addr => wit("evex") "vmovapd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x29 @addr => wit("evex") "vmovapd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x2b @addr => wit("evex") "vmovntpd " $addr "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x2e @addr => wit("evex") "vucomisd " zreg[0+$g] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x2f @addr => wit("evex") "vcomisd " zreg[0+$g] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x6e @addr => wit("evex") wit("altw") "vmovd " zreg[0+$g] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa64 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x7e @addr => wit("evex") wit("altw") "vmovd " $addr "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa64 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0xd6 @addr => wit("evex") "vmovq " $addr "," zreg[0+$g] ;
  0xc1 1 1111 0 01 z 00 0 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 00 1 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x10 @addr => wit("evex") "vmovupd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x11 @addr => wit("evex") "vmovupd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x28 @addr => wit("evex") "vmovapd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x29 @addr => wit("evex") "vmovapd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x2b @addr => wit("evex") "vmovntpd " $addr "," zreg[8+$g] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa64 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa64 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 1 1111 0 01 z 01 0 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 01 1 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x10 @addr => wit("evex") "vmovupd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x11 @addr => wit("evex") "vmovupd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x28 @addr => wit("evex") "vmovapd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x29 @addr => wit("evex") "vmovapd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x2b @addr => wit("evex") "vmovntpd " $addr "," zreg[16+$g] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x51 @addr => wit("evex") "vsqrtpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqa64 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtpd2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvttpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0x7b @addr => wit("evex") "vcvtpd2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqa64 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 1 1111 0 01 z 10 0 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 01 z 10 1 1 aaa 0xe6 @addr => wit("evex") "vcvttpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x2c @addr => wit("evex") wit("altw") "vcvttss2si " greg[$g] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x2d @addr => wit("evex") wit("altw") "vcvtss2si " greg[$g] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu64 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x78 @addr => wit("evex") wit("altw") "vcvttss2usi " greg[$g] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x79 @addr => wit("evex") wit("altw") "vcvtss2usi " greg[$g] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x7e @addr => wit("evex") "vmovq " zreg[0+$g] "," $addr ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu64 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 10 z 00 0 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 00 1 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 10 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu64 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 10 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu64 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 1 1111 0 10 z 01 0 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 01 1 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 10 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu64 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 10 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu64 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 1 1111 0 10 z 10 0 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 10 z 10 1 1 aaa 0xe6 @addr => wit("evex") "vcvtqq2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x10 @addr => wit("evex") "vmovsd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x11 @addr => wit("evex") "vmovsd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x12 @addr => wit("evex") "vmovddup " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x2c @addr => wit("evex") wit("altw") "vcvttsd2si " greg[$g] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x2d @addr => wit("evex") wit("altw") "vcvtsd2si " greg[$g] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu16 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x78 @addr => wit("evex") wit("altw") "vcvttsd2usi " greg[$g] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x79 @addr => wit("evex") wit("altw") "vcvtsd2usi " greg[$g] "," $addr ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu16 " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc1 1 1111 0 11 z 00 0 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 00 1 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0x12 @addr => wit("evex") "vmovddup " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu16 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu16 " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc1 1 1111 0 11 z 01 0 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 01 1 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0x12 @addr => wit("evex") "vmovddup " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0x6f @addr => wit("evex") "vmovdqu16 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0x7f @addr => wit("evex") "vmovdqu16 " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc1 1 1111 0 11 z 10 0 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc1 1 1111 0 11 z 10 1 1 aaa 0xe6 @addr => wit("evex") "vcvtpd2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x12 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmovlpd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmovhpd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x73 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0x73 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[0+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 00 0 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 00 1 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x73 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0x73 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[8+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 01 0 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 01 1 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpcklpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vunpckhpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x54 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x55 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vandnpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vorpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vxorpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x6c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpcklqdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x6d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpunpckhqdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(0) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprorq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(1) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vprolq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0x72 @addr(4) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x73 @addr(2) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0x73 @addr(6) @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[16+(7-$v)] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmppd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpaddq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xdb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpandnq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsraq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xeb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vporq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xef @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpxorq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuludq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 01 z 10 0 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 01 z 10 1 1 aaa 0xfb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsubq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc1 1 h vvv 0 10 z 00 0 1 aaa 0x2a @addr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtsi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 10 z 00 0 1 aaa 0x7b @addr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtusi2ss " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x2a @addr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtsi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x5a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0x7b @addr => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vcvtusi2sd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc1 1 h vvv 0 11 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x18 @addr => wit("evex") "vbroadcastss " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x1c @addr => wit("evex") "vpabsb " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x1d @addr => wit("evex") "vpabsw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x20 @addr => wit("evex") "vpmovsxbw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x21 @addr => wit("evex") "vpmovsxbd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x22 @addr => wit("evex") "vpmovsxbq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x23 @addr => wit("evex") "vpmovsxwd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x24 @addr => wit("evex") "vpmovsxwq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x25 @addr => wit("evex") "vpmovsxdq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x2a @addr => wit("evex") "vmovntdqa " zreg[0+$g] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x30 @addr => wit("evex") "vpmovzxbw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x31 @addr => wit("evex") "vpmovzxbd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x32 @addr => wit("evex") "vpmovzxbq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x33 @addr => wit("evex") "vpmovzxwd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x34 @addr => wit("evex") "vpmovzxwq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x35 @addr => wit("evex") "vpmovzxdq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x54 @addr => wit("evex") "vpopcntb " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x58 @addr => wit("evex") "vpbroadcastd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x59 @addr => wit("evex") "vbroadcasti32x2 " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x62 @addr => wit("evex") "vpexpandb " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x63 @addr => wit("evex") "vpcompressb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x78 @addr => wit("evex") "vpbroadcastb " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x79 @addr => wit("evex") "vpbroadcastw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x88 @addr => wit("evex") "vexpandps " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x89 @addr => wit("evex") "vpexpandd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x8a @addr => wit("evex") "vcompressps " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x8b @addr => wit("evex") "vpcompressd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 00 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x18 @addr => wit("evex") "vbroadcastss " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x19 @addr => wit("evex") "vbroadcastf32x2 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x1a @addr => wit("evex") "vbroadcastf32x4 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x1c @addr => wit("evex") "vpabsb " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x1d @addr => wit("evex") "vpabsw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x20 @addr => wit("evex") "vpmovsxbw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x21 @addr => wit("evex") "vpmovsxbd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x22 @addr => wit("evex") "vpmovsxbq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x23 @addr => wit("evex") "vpmovsxwd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x24 @addr => wit("evex") "vpmovsxwq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x25 @addr => wit("evex") "vpmovsxdq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x2a @addr => wit("evex") "vmovntdqa " zreg[8+$g] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x30 @addr => wit("evex") "vpmovzxbw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x31 @addr => wit("evex") "vpmovzxbd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x32 @addr => wit("evex") "vpmovzxbq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x33 @addr => wit("evex") "vpmovzxwd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x34 @addr => wit("evex") "vpmovzxwq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x35 @addr => wit("evex") "vpmovzxdq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x54 @addr => wit("evex") "vpopcntb " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x58 @addr => wit("evex") "vpbroadcastd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x59 @addr => wit("evex") "vbroadcasti32x2 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x5a @addr => wit("evex") "vbroadcasti32x4 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x62 @addr => wit("evex") "vpexpandb " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x63 @addr => wit("evex") "vpcompressb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x78 @addr => wit("evex") "vpbroadcastb " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x79 @addr => wit("evex") "vpbroadcastw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x88 @addr => wit("evex") "vexpandps " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x89 @addr => wit("evex") "vpexpandd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x8a @addr => wit("evex") "vcompressps " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x8b @addr => wit("evex") "vpcompressd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 01 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x18 @addr => wit("evex") "vbroadcastss " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x19 @addr => wit("evex") "vbroadcastf32x2 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x1a @addr => wit("evex") "vbroadcastf32x4 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x1b @addr => wit("evex") "vbroadcastf32x8 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x1c @addr => wit("evex") "vpabsb " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x1d @addr => wit("evex") "vpabsw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x1e @addr => wit("evex") "vpabsd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x20 @addr => wit("evex") "vpmovsxbw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x21 @addr => wit("evex") "vpmovsxbd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x22 @addr => wit("evex") "vpmovsxbq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x23 @addr => wit("evex") "vpmovsxwd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x24 @addr => wit("evex") "vpmovsxwq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x25 @addr => wit("evex") "vpmovsxdq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x2a @addr => wit("evex") "vmovntdqa " zreg[16+$g] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x30 @addr => wit("evex") "vpmovzxbw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x31 @addr => wit("evex") "vpmovzxbd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x32 @addr => wit("evex") "vpmovzxbq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x33 @addr => wit("evex") "vpmovzxwd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x34 @addr => wit("evex") "vpmovzxwq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x35 @addr => wit("evex") "vpmovzxdq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x42 @addr => wit("evex") "vgetexpps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x44 @addr => wit("evex") "vplzcntd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x4c @addr => wit("evex") "vrcp14ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x54 @addr => wit("evex") "vpopcntb " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0x55 @addr => wit("evex") "vpopcntd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x58 @addr => wit("evex") "vpbroadcastd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x59 @addr => wit("evex") "vbroadcasti32x2 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x5a @addr => wit("evex") "vbroadcasti32x4 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x5b @addr => wit("evex") "vbroadcasti32x8 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x62 @addr => wit("evex") "vpexpandb " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x63 @addr => wit("evex") "vpcompressb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x78 @addr => wit("evex") "vpbroadcastb " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x79 @addr => wit("evex") "vpbroadcastw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x88 @addr => wit("evex") "vexpandps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x89 @addr => wit("evex") "vpexpandd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x8a @addr => wit("evex") "vcompressps " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x8b @addr => wit("evex") "vpcompressd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc8 @addr => wit("evex") "vexp2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0xc8 @addr => wit("evex") "vexp2ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xca @addr => wit("evex") "vrcp28ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0xca @addr => wit("evex") "vrcp28ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xcc @addr => wit("evex") "vrsqrt28ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 01 z 10 1 1 aaa 0xcc @addr => wit("evex") "vrsqrt28ps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x10 @addr => wit("evex") "vpmovuswb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x11 @addr => wit("evex") "vpmovusdb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x12 @addr => wit("evex") "vpmovusqb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x13 @addr => wit("evex") "vpmovusdw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x14 @addr => wit("evex") "vpmovusqw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x15 @addr => wit("evex") "vpmovusqd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x20 @addr => wit("evex") "vpmovswb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x21 @addr => wit("evex") "vpmovsdb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x22 @addr => wit("evex") "vpmovsqb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x23 @addr => wit("evex") "vpmovsdw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x24 @addr => wit("evex") "vpmovsqw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x25 @addr => wit("evex") "vpmovsqd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x30 @addr => wit("evex") "vpmovwb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x31 @addr => wit("evex") "vpmovdb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x32 @addr => wit("evex") "vpmovqb " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x33 @addr => wit("evex") "vpmovdw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x34 @addr => wit("evex") "vpmovqw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x35 @addr => wit("evex") "vpmovqd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 10 z 00 0 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 10 z 00 1 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x10 @addr => wit("evex") "vpmovuswb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x11 @addr => wit("evex") "vpmovusdb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x12 @addr => wit("evex") "vpmovusqb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x13 @addr => wit("evex") "vpmovusdw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x14 @addr => wit("evex") "vpmovusqw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x15 @addr => wit("evex") "vpmovusqd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x20 @addr => wit("evex") "vpmovswb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x21 @addr => wit("evex") "vpmovsdb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x22 @addr => wit("evex") "vpmovsqb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x23 @addr => wit("evex") "vpmovsdw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x24 @addr => wit("evex") "vpmovsqw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x25 @addr => wit("evex") "vpmovsqd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x30 @addr => wit("evex") "vpmovwb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x31 @addr => wit("evex") "vpmovdb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x32 @addr => wit("evex") "vpmovqb " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x33 @addr => wit("evex") "vpmovdw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x34 @addr => wit("evex") "vpmovqw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x35 @addr => wit("evex") "vpmovqd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 10 z 01 0 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 10 z 01 1 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x10 @addr => wit("evex") "vpmovuswb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x11 @addr => wit("evex") "vpmovusdb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x12 @addr => wit("evex") "vpmovusqb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x13 @addr => wit("evex") "vpmovusdw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x14 @addr => wit("evex") "vpmovusqw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x15 @addr => wit("evex") "vpmovusqd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x20 @addr => wit("evex") "vpmovswb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x21 @addr => wit("evex") "vpmovsdb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x22 @addr => wit("evex") "vpmovsqb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x23 @addr => wit("evex") "vpmovsdw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x24 @addr => wit("evex") "vpmovsqw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x25 @addr => wit("evex") "vpmovsqd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x30 @addr => wit("evex") "vpmovwb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x31 @addr => wit("evex") "vpmovdb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x32 @addr => wit("evex") "vpmovqb " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x33 @addr => wit("evex") "vpmovdw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x34 @addr => wit("evex") "vpmovqw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x35 @addr => wit("evex") "vpmovqd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 10 z 10 0 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 0 1111 0 10 z 10 1 1 aaa 0x72 @addr => wit("evex") "vcvtneps2bf16 " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x00 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x04 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddubsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x0b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhrsw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x2d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x38 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x43 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x4d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp14ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x4f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt14ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2b " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2b " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x8f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x99 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0x9f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xa9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xab @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xad @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xaf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xb9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xbb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xbd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xbf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xcb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xcd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xcf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8mulb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenc " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenclast " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdec " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 00 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdeclast " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x00 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x04 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddubsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x0b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhrsw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x38 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2b " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2b " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x8f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xcf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8mulb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenc " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenclast " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdec " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 01 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdeclast " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x00 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x04 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaddubsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x0b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulhrsw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x0c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x2b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpackusdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x38 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminud " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxud " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmulld " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x50 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpbusds " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpdpwssds " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2b " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2d " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2b " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2d " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x8f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshufbitqmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xcf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8mulb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xdc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenc " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xdd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesenclast " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xde @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdec " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 01 z 10 0 1 aaa 0xdf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaesdeclast " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 00 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 00 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 00 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 10 z 00 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 00 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 10 z 01 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 01 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 01 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 10 z 01 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 01 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 10 z 10 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 10 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 10 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 10 z 10 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 10 z 10 1 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdpbf16ps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 11 z 00 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 00 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 11 z 00 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 00 1 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc2 0 h vvv 0 11 z 00 0 1 aaa 0x9b @addr => wit($h==0 ? "vlo" : "") wit("evex") "v4fmaddss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 00 0 1 aaa 0xab @addr => wit($h==0 ? "vlo" : "") wit("evex") "v4fnmaddss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 01 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 01 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 11 z 01 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 01 1 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x52 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp4dpwssd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x53 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp4dpwssds " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 10 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectd " kreg[$g] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 10 1 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtne2ps2bf16 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "v4fmaddps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 0 h vvv 0 11 z 10 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "v4fnmaddps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x54 @addr => wit("evex") "vpopcntw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x59 @addr => wit("evex") "vpbroadcastq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x62 @addr => wit("evex") "vpexpandw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x63 @addr => wit("evex") "vpcompressw " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x88 @addr => wit("evex") "vexpandpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x89 @addr => wit("evex") "vpexpandq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x8a @addr => wit("evex") "vcompresspd " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x8b @addr => wit("evex") "vpcompressq " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 00 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x19 @addr => wit("evex") "vbroadcastsd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x1a @addr => wit("evex") "vbroadcastf64x2 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x54 @addr => wit("evex") "vpopcntw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x59 @addr => wit("evex") "vpbroadcastq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x5a @addr => wit("evex") "vbroadcasti64x2 " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x62 @addr => wit("evex") "vpexpandw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x63 @addr => wit("evex") "vpcompressw " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x88 @addr => wit("evex") "vexpandpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x89 @addr => wit("evex") "vpexpandq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x8a @addr => wit("evex") "vcompresspd " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x8b @addr => wit("evex") "vpcompressq " $addr kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 01 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x19 @addr => wit("evex") "vbroadcastsd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x1a @addr => wit("evex") "vbroadcastf64x2 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x1b @addr => wit("evex") "vbroadcastf64x4 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x1f @addr => wit("evex") "vpabsq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x42 @addr => wit("evex") "vgetexppd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x44 @addr => wit("evex") "vplzcntq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x4c @addr => wit("evex") "vrcp14pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x4e @addr => wit("evex") "vrsqrt14pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x54 @addr => wit("evex") "vpopcntw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0x55 @addr => wit("evex") "vpopcntq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x59 @addr => wit("evex") "vpbroadcastq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x5a @addr => wit("evex") "vbroadcasti64x2 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x5b @addr => wit("evex") "vbroadcasti64x4 " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x62 @addr => wit("evex") "vpexpandw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x63 @addr => wit("evex") "vpcompressw " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x88 @addr => wit("evex") "vexpandpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x89 @addr => wit("evex") "vpexpandq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x8a @addr => wit("evex") "vcompresspd " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x8b @addr => wit("evex") "vpcompressq " $addr kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0xc4 @addr => wit("evex") "vpconflictq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc8 @addr => wit("evex") "vexp2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0xc8 @addr => wit("evex") "vexp2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xca @addr => wit("evex") "vrcp28pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0xca @addr => wit("evex") "vrcp28pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xcc @addr => wit("evex") "vrsqrt28pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc2 1 1111 0 01 z 10 1 1 aaa 0xcc @addr => wit("evex") "vrsqrt28pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x10 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x11 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x12 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x2d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x43 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x4d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp14sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x4f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt14sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x70 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2w " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2w " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x99 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0x9f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xa9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xab @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xad @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xaf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xb9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xbb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xbd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xbf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xcb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrcp28sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 00 0 1 aaa 0xcd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrt28sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x10 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x11 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x12 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x70 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2w " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2w " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 01 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 01 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x0d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermilpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x10 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x11 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x12 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x14 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprorvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x15 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vprolvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x16 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x28 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmuldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x29 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpeqq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x36 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x37 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpgtq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x39 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminsq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x3b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpminuq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x3d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxsq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x3f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmaxuq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x40 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmullq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x45 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsrlvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x46 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsravq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x47 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpsllvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x64 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x65 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vblendmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x66 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpblendmw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x70 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x71 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshldvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x72 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x73 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdvq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x75 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2w " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x76 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2q " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x77 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermi2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x7d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2w " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x7e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2q " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x7f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermt2pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x83 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmultishiftqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x8d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpermw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xb4 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52luq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xb5 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vpmadd52huq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 01 z 10 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 01 z 10 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231pd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 10 z 00 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 00 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 00 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 10 z 01 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 01 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 01 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 10 z 10 0 1 aaa 0x26 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 10 0 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 10 z 10 1 1 aaa 0x27 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vptestnmq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc2 1 h vvv 0 11 z 00 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 11 z 00 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[0+(7-$v)] "," $addr bcst64[0] ;
  0xc2 1 h vvv 0 11 z 01 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[8+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 11 z 01 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[8+(7-$v)] "," $addr bcst64[1] ;
  0xc2 1 h vvv 0 11 z 10 0 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[16+(7-$v)] "," $addr ;
  0xc2 1 h vvv 0 11 z 10 1 1 aaa 0x68 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vp2intersectq " kreg[$g] "," zreg[16+(7-$v)] "," $addr bcst64[2] ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.x " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.x " kreg[$g] kzdec[$z*8+$a] "," $addr bcst16[0] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 00 0 1 aaa 0x67 @addr @imm8 => wit("evex") "vfpclasssh " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.y " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 01 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.y " kreg[$g] kzdec[$z*8+$a] "," $addr bcst16[1] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.z " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 00 z 10 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassph.z " kreg[$g] kzdec[$z*8+$a] "," $addr bcst16[2] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x14 @addr @imm8 => wit("evex") "vpextrb " $addr "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x15 @addr @imm8 => wit("evex") "vpextrw " $addr "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x16 @addr @imm8 => wit("evex") "vpextrd " $addr "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x17 @addr @imm8 => wit("evex") "vextractps " $addr "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x1d @addr @imm8 => wit("evex") "vcvtps2ph " $addr kzdec[$z*8+$a] "," zreg[0+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.x " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.x " kreg[$g] kzdec[$z*8+$a] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 00 0 1 aaa 0x67 @addr @imm8 => wit("evex") "vfpclassss " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 1 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x19 @addr @imm8 => wit("evex") "vextractf32x4 " $addr kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x1d @addr @imm8 => wit("evex") "vcvtps2ph " $addr kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x39 @addr @imm8 => wit("evex") "vextracti32x4 " $addr kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.y " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 01 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.y " kreg[$g] kzdec[$z*8+$a] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 1 1 aaa 0x04 @addr @imm8 => wit("evex") "vpermilps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 1 1 aaa 0x08 @addr @imm8 => wit("evex") "vrndscaleps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x19 @addr @imm8 => wit("evex") "vextractf32x4 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x1b @addr @imm8 => wit("evex") "vextractf32x8 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x1d @addr @imm8 => wit("evex") "vcvtps2ph " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x39 @addr @imm8 => wit("evex") "vextracti32x4 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x3b @addr @imm8 => wit("evex") "vextracti32x8 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreduceps " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.z " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 0 1111 0 01 z 10 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclassps.z " kreg[$g] kzdec[$z*8+$a] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x27 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0x57 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 00 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 01 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 01 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 10 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 00 z 10 1 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpph " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x0a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscaless " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x0f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x20 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrb " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x21 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertps " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x22 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpinsrd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x27 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x42 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x44 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x51 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x55 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x57 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducess " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 00 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x0f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x18 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x38 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x42 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x44 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[8+$g] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 01 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x0f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpalignr " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x18 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf32x8 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpud " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpd " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x38 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti32x8 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpub " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpb " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x42 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vdbpsadbw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi32x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x44 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpclmulqdq " zreg[16+$g] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangeps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmps " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 0 h vvv 0 01 z 10 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] "," hex($imm8) ;
  0xc3 0 h vvv 0 10 z 00 0 1 aaa 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vcmpsh " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x16 @addr @imm8 => wit("evex") wit("altw") "vpextrd " $addr "," zreg[0+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[0+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.x " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.x " kreg[$g] kzdec[$z*8+$a] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 00 0 1 aaa 0x67 @addr @imm8 => wit("evex") "vfpclasssd " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x00 @addr @imm8 => wit("evex") "vpermq " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x00 @addr @imm8 => wit("evex") "vpermq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x01 @addr @imm8 => wit("evex") "vpermpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x01 @addr @imm8 => wit("evex") "vpermpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x19 @addr @imm8 => wit("evex") "vextractf64x2 " $addr kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x39 @addr @imm8 => wit("evex") "vextracti64x2 " $addr kzdec[$z*8+$a] "," zreg[8+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[8+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.y " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 01 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.y " kreg[$g] kzdec[$z*8+$a] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x00 @addr @imm8 => wit("evex") "vpermq " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x00 @addr @imm8 => wit("evex") "vpermq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x01 @addr @imm8 => wit("evex") "vpermpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x01 @addr @imm8 => wit("evex") "vpermpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x05 @addr @imm8 => wit("evex") "vpermilpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x09 @addr @imm8 => wit("evex") "vrndscalepd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x19 @addr @imm8 => wit("evex") "vextractf64x2 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x1b @addr @imm8 => wit("evex") "vextractf64x4 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x26 @addr @imm8 => wit("evex") "vgetmantpd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x39 @addr @imm8 => wit("evex") "vextracti64x2 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x3b @addr @imm8 => wit("evex") "vextracti64x4 " $addr kzdec[$z*8+$a] "," zreg[16+$g] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[16+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x56 @addr @imm8 => wit("evex") "vreducepd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 0 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.z " kreg[$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  0xc3 1 1111 0 01 z 10 1 1 aaa 0x66 @addr @imm8 => wit("evex") "vfpclasspd.z " kreg[$g] kzdec[$z*8+$a] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x0b @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrndscalesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x22 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") wit("altw") "vpinsrd " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x27 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgetmantsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x51 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x55 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmsd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x57 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vreducesd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x70 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x72 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 0 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 00 1 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst64[0] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x18 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x38 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x70 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x72 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 0 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 01 1 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst64[1] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x03 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "valignq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x18 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinsertf64x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x1e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x1f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpq " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x23 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshuff64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x25 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpternlogq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x38 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3a @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vinserti64x4 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3e @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpuw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x3f @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpcmpw " kreg[$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x43 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vshufi64x2 " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x50 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vrangepd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x54 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vfixupimmpd " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x70 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x71 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshldq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x72 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdw " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0x73 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vpshrdq " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 0 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr "," hex($imm8) ;
  0xc3 1 h vvv 0 01 z 10 1 1 aaa 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") wit("evex") "vgf2p8affineinvqb " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst64[2] "," hex($imm8) ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x2e @addr => wit("evex") "vucomish " zreg[0+$g] "," $addr ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x2f @addr => wit("evex") "vcomish " zreg[0+$g] "," $addr ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 00 z 00 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 00 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 00 z 01 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 01 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x51 @addr => wit("evex") "vsqrtph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x5a @addr => wit("evex") "vcvtph2pd " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtdq2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2udq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2uw " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 00 z 10 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 00 z 10 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2uw " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x6e @addr => wit("evex") "vmovw " zreg[0+$g] "," $addr ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 00 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 01 z 00 0 1 aaa 0x7e @addr => wit("evex") "vmovw " $addr "," zreg[0+$g] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 01 z 01 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 01 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x1d @addr => wit("evex") "vcvtps2phx " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtph2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x78 @addr => wit("evex") "vcvttph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x79 @addr => wit("evex") "vcvtph2uqq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvttph2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x7b @addr => wit("evex") "vcvtph2qq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x7c @addr => wit("evex") "vcvttph2w " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 01 z 10 0 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 01 z 10 1 1 aaa 0x7d @addr => wit("evex") "vcvtph2w " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x10 @addr => wit("evex") "vmovsh " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x11 @addr => wit("evex") "vmovsh " $addr kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x2c @addr => wit("evex") "vcvttsh2si " greg[$g] "," $addr ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x2d @addr => wit("evex") "vcvtsh2si " greg[$g] "," $addr ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x78 @addr => wit("evex") "vcvttsh2usi " greg[$g] "," $addr ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x79 @addr => wit("evex") "vcvtsh2usi " greg[$g] "," $addr ;
  0xc5 0 1111 0 10 z 00 0 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 00 1 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 10 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 10 z 01 0 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 01 1 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 10 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvttph2dq " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 10 z 10 0 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 10 z 10 1 1 aaa 0x7d @addr => wit("evex") "vcvtw2ph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 1111 0 11 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc5 0 1111 0 11 z 00 0 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 00 1 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc5 0 1111 0 11 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc5 0 1111 0 11 z 01 0 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 01 1 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc5 0 1111 0 11 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtudq2ph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc5 0 1111 0 11 z 10 0 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 0 1111 0 11 z 10 1 1 aaa 0x7d @addr => wit("evex") "vcvtuw2ph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x1d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtss2sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 00 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 01 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 01 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 00 z 10 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 00 z 10 1 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x2a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x51 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsqrtsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x58 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vaddsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x59 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmulsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x5a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2sd " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x5c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vsubsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x5d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vminsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x5e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vdivsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x5f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vmaxsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 0 h vvv 0 10 z 00 0 1 aaa 0x7b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtusi2sh " zreg[0+$g] "," zreg[0+(7-$v)] "," $addr ;
  0xc5 1 1111 0 00 z 00 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 00 z 00 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 1 1111 0 00 z 01 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 00 z 01 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 1 1111 0 00 z 10 0 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 00 z 10 1 1 aaa 0x5b @addr => wit("evex") "vcvtqq2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 1 1111 0 01 z 00 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 01 z 00 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 1 1111 0 01 z 01 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 01 z 01 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 1 1111 0 01 z 10 0 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 01 z 10 1 1 aaa 0x5a @addr => wit("evex") "vcvtpd2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 1 1111 0 11 z 00 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 11 z 00 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.x " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[0] ;
  0xc5 1 1111 0 11 z 01 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 11 z 01 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.y " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[1] ;
  0xc5 1 1111 0 11 z 10 0 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc5 1 1111 0 11 z 10 1 1 aaa 0x7a @addr => wit("evex") "vcvtuqq2ph.z " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst64[2] ;
  0xc5 1 h vvv 0 11 z 00 0 1 aaa 0x5a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsd2sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 1111 0 01 z 00 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst32[0] ;
  0xc6 0 1111 0 01 z 00 0 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc6 0 1111 0 01 z 00 0 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc6 0 1111 0 01 z 00 0 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[0+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 00 1 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[0+$g] kzdec[$z*8+$a] "," $addr bcst16[0] ;
  0xc6 0 1111 0 01 z 01 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 01 1 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst32[1] ;
  0xc6 0 1111 0 01 z 01 0 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 01 1 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc6 0 1111 0 01 z 01 0 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 01 1 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc6 0 1111 0 01 z 01 0 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[8+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 01 1 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[8+$g] kzdec[$z*8+$a] "," $addr bcst16[1] ;
  0xc6 0 1111 0 01 z 10 0 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 10 1 1 aaa 0x13 @addr => wit("evex") "vcvtph2psx " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst32[2] ;
  0xc6 0 1111 0 01 z 10 0 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 10 1 1 aaa 0x42 @addr => wit("evex") "vgetexpph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc6 0 1111 0 01 z 10 0 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 10 1 1 aaa 0x4c @addr => wit("evex") "vrcpph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc6 0 1111 0 01 z 10 0 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[16+$g] kzdec[$z*8+$a] "," $addr ;
  0xc6 0 1111 0 01 z 10 1 1 aaa 0x4e @addr => wit("evex") "vrsqrtph " zreg[16+$g] kzdec[$z*8+$a] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 00 z 00 0 1 aaa 0x13 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vcvtsh2ss " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x2d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x43 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vgetexpsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x4d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrcpsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x4f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vrsqrtsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x99 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9b @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9d @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0x9f @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xa9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xab @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xad @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xaf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xb9 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xbb @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xbd @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 00 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst16[0] ;
  0xc6 0 h vvv 0 01 z 00 0 1 aaa 0xbf @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231sh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 01 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 01 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst16[1] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x2c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vscalefph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x96 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x97 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x98 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x9a @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x9c @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0x9e @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub132ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xa6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xa7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xa8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xaa @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xac @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xae @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub213ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xb6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xb7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsubadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xb8 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xba @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xbc @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmadd231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 01 z 10 0 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 01 z 10 1 1 aaa 0xbe @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfnmsub231ph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst16[2] ;
  0xc6 0 h vvv 0 10 z 00 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 00 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc6 0 h vvv 0 10 z 00 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 00 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 00 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc6 0 h vvv 0 10 z 00 0 1 aaa 0xd7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 01 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 01 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc6 0 h vvv 0 10 z 01 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 01 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc6 0 h vvv 0 10 z 10 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 10 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc6 0 h vvv 0 10 z 10 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 10 z 10 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc6 0 h vvv 0 11 z 00 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 00 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc6 0 h vvv 0 11 z 00 0 1 aaa 0x57 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 00 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 00 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr bcst32[0] ;
  0xc6 0 h vvv 0 11 z 00 0 1 aaa 0xd7 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcsh " zreg[0+$g] kzdec[$z*8+$a] "," zreg[0+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 01 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 01 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc6 0 h vvv 0 11 z 01 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 01 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[8+$g] kzdec[$z*8+$a] "," zreg[8+(7-$v)] "," $addr bcst32[1] ;
  0xc6 0 h vvv 0 11 z 10 0 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 10 1 1 aaa 0x56 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmaddcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;
  0xc6 0 h vvv 0 11 z 10 0 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr ;
  0xc6 0 h vvv 0 11 z 10 1 1 aaa 0xd6 @addr => wit($h==0 ? "vlo" : "") wit("evex") "vfcmulcph " zreg[16+$g] kzdec[$z*8+$a] "," zreg[16+(7-$v)] "," $addr bcst32[2] ;

  # ---- VSIB gather/scatter/prefetch (mod=00 rm=4, vector index; explicit SIB) ----
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdd " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqd " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdps " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqps " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdd " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqd " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdps " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 00 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqps " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdd " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqd " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdps " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqps " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdps " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 01 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqps " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdd " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqd " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdps " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqps " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc6 00 001 100 ss iii bbb => wit("evex") "vgatherpf0dps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc6 00 010 100 ss iii bbb => wit("evex") "vgatherpf1dps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc6 00 101 100 ss iii bbb => wit("evex") "vscatterpf0dps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc6 00 110 100 ss iii bbb => wit("evex") "vscatterpf1dps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc7 00 001 100 ss iii bbb => wit("evex") "vgatherpf0qps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc7 00 010 100 ss iii bbb => wit("evex") "vgatherpf1qps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc7 00 101 100 ss iii bbb => wit("evex") "vscatterpf0qps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 0 1111 0 01 z 10 0 1 aaa 0xc7 00 110 100 ss iii bbb => wit("evex") "vscatterpf1qps " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdq " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqq " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdpd " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqpd " zreg[0+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdq " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqq " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdpd " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 00 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqpd " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[0+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdq " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqq " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdpd " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqpd " zreg[8+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdq " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqq " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdpd " "[" greg[$b] "+" zreg[0+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 01 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[8+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x90 00 ggg 100 ss iii bbb => wit("evex") "vpgatherdq " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x91 00 ggg 100 ss iii bbb => wit("evex") "vpgatherqq " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x92 00 ggg 100 ss iii bbb => wit("evex") "vgatherdpd " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0x93 00 ggg 100 ss iii bbb => wit("evex") "vgatherqpd " zreg[16+$g] kzdec[$z*8+$a] "," "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xa0 00 ggg 100 ss iii bbb => wit("evex") "vpscatterdq " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xa1 00 ggg 100 ss iii bbb => wit("evex") "vpscatterqq " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xa2 00 ggg 100 ss iii bbb => wit("evex") "vscatterdpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xa3 00 ggg 100 ss iii bbb => wit("evex") "vscatterqpd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," zreg[16+$g] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc6 00 001 100 ss iii bbb => wit("evex") "vgatherpf0dpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc6 00 010 100 ss iii bbb => wit("evex") "vgatherpf1dpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc6 00 101 100 ss iii bbb => wit("evex") "vscatterpf0dpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc6 00 110 100 ss iii bbb => wit("evex") "vscatterpf1dpd " "[" greg[$b] "+" zreg[8+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc7 00 001 100 ss iii bbb => wit("evex") "vgatherpf0qpd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc7 00 010 100 ss iii bbb => wit("evex") "vgatherpf1qpd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc7 00 101 100 ss iii bbb => wit("evex") "vscatterpf0qpd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
  0xc2 1 1111 0 01 z 10 0 1 aaa 0xc7 00 110 100 ss iii bbb => wit("evex") "vscatterpf1qpd " "[" greg[$b] "+" zreg[16+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] ;
}

submatch vex2 {
  # ---- 2-byte VEX (C5): compact form of every map-0F W=0 leaf.
  #      payload byte = R(=1) vvvv L pp ; decodes == C4 twin + wit("c5"). ----
  1 h vvv 1 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmovss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmovsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 h vvv 1 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] ;
  1 1111 1 10 0x10 @addr => wit("c5") wit("altl") "vmovss " vsse[$g] "," $addr ;
  1 1111 1 11 0x10 @addr => wit("c5") wit("altl") "vmovsd " vsse[$g] "," $addr ;
  1 1111 1 10 0x11 @addr => wit("c5") wit("altl") "vmovss " $addr "," vsse[$g] ;
  1 1111 1 11 0x11 @addr => wit("c5") wit("altl") "vmovsd " $addr "," vsse[$g] ;
  1 h vvv 1 10 0x2a @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsi2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x2a @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsi2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 1111 1 00 0x2e @addr => wit("c5") wit("altl") "vucomiss " vsse[$g] "," $addr ;
  1 1111 1 00 0x2e 11 ggg rrr => wit("c5") wit("altl") "vucomiss " vsse[$g] "," vsse[$r] ;
  1 1111 1 01 0x2e @addr => wit("c5") wit("altl") "vucomisd " vsse[$g] "," $addr ;
  1 1111 1 01 0x2e 11 ggg rrr => wit("c5") wit("altl") "vucomisd " vsse[$g] "," vsse[$r] ;
  1 1111 1 00 0x2f @addr => wit("c5") wit("altl") "vcomiss " vsse[$g] "," $addr ;
  1 1111 1 00 0x2f 11 ggg rrr => wit("c5") wit("altl") "vcomiss " vsse[$g] "," vsse[$r] ;
  1 1111 1 01 0x2f @addr => wit("c5") wit("altl") "vcomisd " vsse[$g] "," $addr ;
  1 1111 1 01 0x2f 11 ggg rrr => wit("c5") wit("altl") "vcomisd " vsse[$g] "," vsse[$r] ;
  1 h vvv 1 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsqrtsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vrsqrtss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vrcpss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vaddss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vaddsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmulss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmulsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtss2sd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsd2ss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsubss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vsubsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vminss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vminsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vdivss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vdivsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmaxss " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmaxsd " vsse[$g] "," vsse[(7-$v)] "," $addr ;
  1 h vvv 1 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv 1 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcmpss " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  1 h vvv 1 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv 1 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcmpsd " vsse[$g] "," vsse[(7-$v)] "," vsse[$r] "," hex($imm8) ;
  1 h vvv 1 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmovss " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  1 h vvv 1 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vmovsd " vsse[$r] "," vsse[(7-$v)] "," vsse[$g] wit("alt") ;
  1 h vvv 1 10 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsi2ss " vsse[$g] "," vsse[(7-$v)] "," greg[$r] ;
  1 h vvv 1 11 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") wit("altl") "vcvtsi2sd " vsse[$g] "," vsse[(7-$v)] "," greg[$r] ;
  1 1111 1 10 0x2c @addr => wit("c5") wit("altl") "vcvttss2si " greg[$g] "," $addr ;
  1 1111 1 10 0x2c 11 ggg rrr => wit("c5") wit("altl") "vcvttss2si " greg[$g] "," vsse[$r] ;
  1 1111 1 11 0x2c @addr => wit("c5") wit("altl") "vcvttsd2si " greg[$g] "," $addr ;
  1 1111 1 11 0x2c 11 ggg rrr => wit("c5") wit("altl") "vcvttsd2si " greg[$g] "," vsse[$r] ;
  1 1111 1 10 0x2d @addr => wit("c5") wit("altl") "vcvtss2si " greg[$g] "," $addr ;
  1 1111 1 10 0x2d 11 ggg rrr => wit("c5") wit("altl") "vcvtss2si " greg[$g] "," vsse[$r] ;
  1 1111 1 11 0x2d @addr => wit("c5") wit("altl") "vcvtsd2si " greg[$g] "," $addr ;
  1 1111 1 11 0x2d 11 ggg rrr => wit("c5") wit("altl") "vcvtsd2si " greg[$g] "," vsse[$r] ;
  1 1111 l 00 0x5a 11 ggg rrr => wit("c5") "vcvtps2pd " vsse[$l*8+$g] "," vsse[$r] ;
  1 1111 l 00 0x5a @addr => wit("c5") "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x5a 11 ggg rrr => wit("c5") "vcvtpd2ps " vsse[$g] "," vsse[$l*8+$r] ;
  1 1111 0 01 0x5a @addr => wit("c5") "vcvtpd2ps " vsse[$g] "," $addr ;
  1 1111 1 01 0x5a @addr => wit("c5") "vcvtpd2ps " vsse[$g] "," "ymmword ptr " $addr ;
  1 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 01 0xe6 11 ggg rrr => wit("c5") "vcvttpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  1 1111 0 01 0xe6 @addr => wit("c5") "vcvttpd2dq " vsse[$g] "," $addr ;
  1 1111 1 01 0xe6 @addr => wit("c5") "vcvttpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  1 1111 l 10 0xe6 11 ggg rrr => wit("c5") "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$r] ;
  1 1111 l 10 0xe6 @addr => wit("c5") "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 11 0xe6 11 ggg rrr => wit("c5") "vcvtpd2dq " vsse[$g] "," vsse[$l*8+$r] ;
  1 1111 0 11 0xe6 @addr => wit("c5") "vcvtpd2dq " vsse[$g] "," $addr ;
  1 1111 1 11 0xe6 @addr => wit("c5") "vcvtpd2dq " vsse[$g] "," "ymmword ptr " $addr ;
  1 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$r] ;
  1 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x71 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x72 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsrld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x73 11 010 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv 1 01 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kaddb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x4a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kaddw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kandb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x41 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kandw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kandnb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x42 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kandnw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "korb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x45 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "korw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kxorb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x47 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kxorw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kxnorb " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x46 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kxnorw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 01 0x4b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kunpckbw " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 h vvv 1 00 0x4b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "kunpckwd " kreg[$g] "," kreg[7-$v] "," kreg[$r] ;
  1 1111 0 01 0x44 11 ggg rrr => wit("c5") "knotb " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x44 11 ggg rrr => wit("c5") "knotw " kreg[$g] "," kreg[$r] ;
  1 1111 0 01 0x98 11 ggg rrr => wit("c5") "kortestb " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x98 11 ggg rrr => wit("c5") "kortestw " kreg[$g] "," kreg[$r] ;
  1 1111 0 01 0x99 11 ggg rrr => wit("c5") "ktestb " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x99 11 ggg rrr => wit("c5") "ktestw " kreg[$g] "," kreg[$r] ;
  1 1111 0 01 0x90 11 ggg rrr => wit("c5") "kmovb " kreg[$g] "," kreg[$r] ;
  1 1111 0 01 0x90 @addr      => wit("c5") "kmovb " kreg[$g] "," $addr ;
  1 1111 0 01 0x91 @addr      => wit("c5") "kmovb " $addr "," kreg[$g] ;
  1 1111 0 00 0x90 11 ggg rrr => wit("c5") "kmovw " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x90 @addr      => wit("c5") "kmovw " kreg[$g] "," $addr ;
  1 1111 0 00 0x91 @addr      => wit("c5") "kmovw " $addr "," kreg[$g] ;
  1 1111 0 01 0x92 11 ggg rrr => wit("c5") "kmovb " kreg[$g] "," greg[$r] ;
  1 1111 0 01 0x93 11 ggg rrr => wit("c5") "kmovb " greg[$g] "," kreg[$r] ;
  1 1111 0 00 0x92 11 ggg rrr => wit("c5") "kmovw " kreg[$g] "," greg[$r] ;
  1 1111 0 00 0x93 11 ggg rrr => wit("c5") "kmovw " greg[$g] "," kreg[$r] ;
  1 1111 0 11 0x92 11 ggg rrr => wit("c5") "kmovd " kreg[$g] "," greg[$r] ;
  1 1111 0 11 0x93 11 ggg rrr => wit("c5") "kmovd " greg[$g] "," kreg[$r] ;
  1 h vvv l 10 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x10 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x12 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovhlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x14 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vunpcklps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x14 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x14 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vunpcklpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x15 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vunpckhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x15 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x15 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vunpckhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x16 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmovhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x16 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovlhps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x16 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmovhpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x51 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x52 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x53 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x54 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vandps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x54 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x54 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vandpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x55 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vandnps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x55 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x55 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vandnpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x56 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x56 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x56 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x57 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vxorps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x57 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x57 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vxorpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x58 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmulps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmulpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x59 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x5a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x5c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vminps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vminpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x5d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vdivps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vdivpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x5e 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 00 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 00 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x5f 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x60 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x60 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x61 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x61 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x62 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x62 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckldq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x63 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x63 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpacksswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x64 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x64 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x65 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x65 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x66 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x66 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpgtd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x67 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x67 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpackuswb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x68 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x68 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x69 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x69 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x6a @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x6a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x6b @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x6b 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpackssdw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x6c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x6c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpcklqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x6d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x6d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpunpckhqdq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x74 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x74 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x75 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x75 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x76 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x76 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpcmpeqd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x7c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vhaddpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x7c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x7c 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vhaddps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0x7d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vhsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0x7d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x7d 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vhsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd0 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsubpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 11 0xd0 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0xd0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsubps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd3 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd4 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd5 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmullw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd8 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xd9 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xd9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xda @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xda 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpminub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xdb @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xdb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpand " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xdc @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xdc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddusb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xdd @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xdd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddusw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xde @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xde 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaxub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xdf @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xdf 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpandn " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe0 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe0 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpavgb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsraw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsrad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe3 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpavgw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe4 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmulhuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe5 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmulhw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe8 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xe9 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xe9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xea @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xea 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpminsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xeb @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xeb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xec @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xec 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddsb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xed @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xed 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xee @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xee 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaxsw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xef @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xef 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpxor " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf1 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf1 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf2 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf2 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpslld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf3 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf3 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsllq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf4 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf4 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmuludq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf5 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf5 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpmaddwd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf6 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf6 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsadbw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf8 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf8 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xf9 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xf9 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xfa @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xfa 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xfb @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xfb 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpsubq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xfc @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xfc 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xfd @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xfd 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 h vvv l 01 0xfe @addr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0xfe 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vpaddd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x10 @addr => wit("c5") "vmovups " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x10 11 ggg rrr => wit("c5") "vmovups " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x10 @addr => wit("c5") "vmovupd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x10 11 ggg rrr => wit("c5") "vmovupd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 10 0x10 @addr => wit("c5") "vmovss " vsse[$l*8+$g] "," $addr ;
  1 1111 l 11 0x10 @addr => wit("c5") "vmovsd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x11 @addr => wit("c5") "vmovups " $addr "," vsse[$l*8+$g] ;
  1 1111 l 00 0x11 11 ggg rrr => wit("c5") "vmovups " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 1111 l 01 0x11 @addr => wit("c5") "vmovupd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x11 11 ggg rrr => wit("c5") "vmovupd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 1111 l 10 0x11 @addr => wit("c5") "vmovss " $addr "," vsse[$l*8+$g] ;
  1 1111 l 11 0x11 @addr => wit("c5") "vmovsd " $addr "," vsse[$l*8+$g] ;
  1 h vvv l 00 0x12 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmovlps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 01 0x12 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmovlpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 10 0x12 @addr => wit("c5") "vmovsldup " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0x12 11 ggg rrr => wit("c5") "vmovsldup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 11 0x12 @addr => wit("c5") "vmovddup " vsse[$l*8+$g] "," $addr ;
  1 1111 l 11 0x12 11 ggg rrr => wit("c5") "vmovddup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x13 @addr => wit("c5") "vmovlps " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x13 @addr => wit("c5") "vmovlpd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 10 0x16 @addr => wit("c5") "vmovshdup " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0x16 11 ggg rrr => wit("c5") "vmovshdup " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x17 @addr => wit("c5") "vmovhps " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x17 @addr => wit("c5") "vmovhpd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 00 0x28 @addr => wit("c5") "vmovaps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x28 11 ggg rrr => wit("c5") "vmovaps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x28 @addr => wit("c5") "vmovapd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x28 11 ggg rrr => wit("c5") "vmovapd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x29 @addr => wit("c5") "vmovaps " $addr "," vsse[$l*8+$g] ;
  1 1111 l 00 0x29 11 ggg rrr => wit("c5") "vmovaps " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 1111 l 01 0x29 @addr => wit("c5") "vmovapd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x29 11 ggg rrr => wit("c5") "vmovapd " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 h vvv l 10 0x2a @addr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsi2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x2a @addr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsi2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 00 0x2b @addr => wit("c5") "vmovntps " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x2b @addr => wit("c5") "vmovntpd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 00 0x2e @addr => wit("c5") "vucomiss " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x2e 11 ggg rrr => wit("c5") "vucomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x2e @addr => wit("c5") "vucomisd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x2e 11 ggg rrr => wit("c5") "vucomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x2f @addr => wit("c5") "vcomiss " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x2f 11 ggg rrr => wit("c5") "vcomiss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x2f @addr => wit("c5") "vcomisd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x2f 11 ggg rrr => wit("c5") "vcomisd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x51 @addr => wit("c5") "vsqrtps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x51 11 ggg rrr => wit("c5") "vsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x51 @addr => wit("c5") "vsqrtpd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x51 11 ggg rrr => wit("c5") "vsqrtpd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x51 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x51 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsqrtsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 00 0x52 @addr => wit("c5") "vrsqrtps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x52 11 ggg rrr => wit("c5") "vrsqrtps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x52 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vrsqrtss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 00 0x53 @addr => wit("c5") "vrcpps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x53 11 ggg rrr => wit("c5") "vrcpps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x53 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vrcpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x58 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vaddsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmulss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x59 @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmulsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 00 0x5a @addr => wit("c5") "vcvtps2pd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x5a 11 ggg rrr => wit("c5") "vcvtps2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x5a @addr => wit("c5") "vcvtpd2ps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x5a 11 ggg rrr => wit("c5") "vcvtpd2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5a @addr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtss2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x5a @addr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsd2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 00 0x5b @addr => wit("c5") "vcvtdq2ps " vsse[$l*8+$g] "," $addr ;
  1 1111 l 00 0x5b 11 ggg rrr => wit("c5") "vcvtdq2ps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x5b @addr => wit("c5") "vcvtps2dq " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x5b 11 ggg rrr => wit("c5") "vcvtps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 10 0x5b @addr => wit("c5") "vcvttps2dq " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0x5b 11 ggg rrr => wit("c5") "vcvttps2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsubss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x5c @addr => wit($h==0 ? "vlo" : "") wit("c5") "vsubsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vminss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x5d @addr => wit($h==0 ? "vlo" : "") wit("c5") "vminsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") "vdivss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x5e @addr => wit($h==0 ? "vlo" : "") wit("c5") "vdivsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 10 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 h vvv l 11 0x5f @addr => wit($h==0 ? "vlo" : "") wit("c5") "vmaxsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  1 1111 l 01 0x6e @addr => wit("c5") "vmovd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x6f @addr => wit("c5") "vmovdqa " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0x6f 11 ggg rrr => wit("c5") "vmovdqa " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 10 0x6f @addr => wit("c5") "vmovdqu " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0x6f 11 ggg rrr => wit("c5") "vmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x70 @addr @imm8 => wit("c5") "vpshufd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  1 1111 l 01 0x70 11 ggg rrr @imm8 => wit("c5") "vpshufd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 10 0x70 @addr @imm8 => wit("c5") "vpshufhw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  1 1111 l 10 0x70 11 ggg rrr @imm8 => wit("c5") "vpshufhw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 11 0x70 @addr @imm8 => wit("c5") "vpshuflw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  1 1111 l 11 0x70 11 ggg rrr @imm8 => wit("c5") "vpshuflw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 01 0x7e @addr => wit("c5") "vmovd " $addr "," vsse[$l*8+$g] ;
  1 1111 l 10 0x7e @addr => wit("c5") "vmovq " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0x7e 11 ggg rrr => wit("c5") "vmovq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x7f @addr => wit("c5") "vmovdqa " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0x7f 11 ggg rrr => wit("c5") "vmovdqa " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 1111 l 10 0x7f @addr => wit("c5") "vmovdqu " $addr "," vsse[$l*8+$g] ;
  1 1111 l 10 0x7f 11 ggg rrr => wit("c5") "vmovdqu " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 h vvv l 00 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 00 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 01 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmppd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 10 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 10 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 11 0xc2 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 11 0xc2 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vcmpsd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0xc4 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpinsrw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 00 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 00 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vshufps " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0xc6 @addr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  1 h vvv l 01 0xc6 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vshufpd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 01 0xd6 @addr => wit("c5") "vmovq " $addr "," vsse[$l*8+$g] ;
  1 1111 l 01 0xd6 11 ggg rrr => wit("c5") "vmovq " vsse[$l*8+$r] "," vsse[$l*8+$g] wit("alt") ;
  1 1111 l 01 0xe6 @addr => wit("c5") "vcvttpd2dq " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0xe6 11 ggg rrr => wit("c5") "vcvttpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 10 0xe6 @addr => wit("c5") "vcvtdq2pd " vsse[$l*8+$g] "," $addr ;
  1 1111 l 10 0xe6 11 ggg rrr => wit("c5") "vcvtdq2pd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 11 0xe6 @addr => wit("c5") "vcvtpd2dq " vsse[$l*8+$g] "," $addr ;
  1 1111 l 11 0xe6 11 ggg rrr => wit("c5") "vcvtpd2dq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0xe7 @addr => wit("c5") "vmovntdq " $addr "," vsse[$l*8+$g] ;
  1 1111 l 11 0xf0 @addr => wit("c5") "vlddqu " vsse[$l*8+$g] "," $addr ;
  1 1111 l 01 0xf7 11 ggg rrr => wit("c5") "vmaskmovdqu " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  1 h vvv l 10 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovss " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  1 h vvv l 11 0x11 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vmovsd " vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$g] wit("alt") ;
  1 h vvv l 10 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsi2ss " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] ;
  1 h vvv l 11 0x2a 11 ggg rrr => wit($h==0 ? "vlo" : "") wit("c5") "vcvtsi2sd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] ;
  1 1111 0 10 0x2c @addr => wit("c5") "vcvttss2si " greg[$g] "," $addr ;
  1 1111 l 10 0x2c 11 ggg rrr => wit("c5") "vcvttss2si " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 0 11 0x2c @addr => wit("c5") "vcvttsd2si " greg[$g] "," $addr ;
  1 1111 l 11 0x2c 11 ggg rrr => wit("c5") "vcvttsd2si " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 0 10 0x2d @addr => wit("c5") "vcvtss2si " greg[$g] "," $addr ;
  1 1111 l 10 0x2d 11 ggg rrr => wit("c5") "vcvtss2si " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 0 11 0x2d @addr => wit("c5") "vcvtsd2si " greg[$g] "," $addr ;
  1 1111 l 11 0x2d 11 ggg rrr => wit("c5") "vcvtsd2si " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 l 00 0x50 11 ggg rrr => wit("c5") "vmovmskps " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x50 11 ggg rrr => wit("c5") "vmovmskpd " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 l 01 0x6e 11 ggg rrr => wit("c5") "vmovd " vsse[$l*8+$g] "," greg[$r] ;
  1 h vvv l 01 0x71 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsraw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x71 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsllw " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x72 11 100 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsrad " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x72 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpslld " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x73 11 011 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsrldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x73 11 110 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpsllq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 h vvv l 01 0x73 11 111 rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpslldq " vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 01 0x7e 11 ggg rrr => wit("c5") "vmovd " greg[$r] "," vsse[$l*8+$g] ;
  1 h vvv l 01 0xc4 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") wit("c5") "vpinsrw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," greg[$r] "," hex($imm8) ;
  1 1111 l 01 0xc5 11 ggg rrr @imm8 => wit("c5") "vpextrw " greg[$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  1 1111 l 01 0xd7 11 ggg rrr => wit("c5") "vpmovmskb " greg[$g] "," vsse[$l*8+$r] ;
  1 1111 0 00 0x77 => wit("c5") "vzeroupper" ;
  1 1111 1 00 0x77 => wit("c5") "vzeroall" ;
  1 1111 0 00 0xae @addr(2) => wit("c5") "vldmxcsr " $addr ;
  1 1111 0 00 0xae @addr(3) => wit("c5") "vstmxcsr " $addr ;
}

# ============================ XOP (8F) front-end =========================
# 8F is also legacy POP r/m (/0); XOP byte1 is E8/E9/EA (maps 8/9/10) whose reg
# field is 101, disjoint from POP /0.  byte2 = W vvvv L pp (pp always 00).
submatch xop {
  # ---- blsfill (op01 /2; dest=vvvv, src=rm; dest-index collides with /digit) -
  0x09 0 h vvv 0 00 0x01 11 010 rrr => wit($h==0 ? "vlo" : "") "blsfill " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(2) => wit($h==0 ? "vlo" : "") "blsfill " greg[7-$v] "," $addr ;
  # ---- XOP map8 is4 4-operand (vpcmov/vpperm/vpmacs/vpmadcs) ----------
  0x08 0 h vvv l 00 0x85 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssww " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x85 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssww " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x86 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x86 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x87 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdql " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x87 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdql " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x8e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x8e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x8f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdqh " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x8f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacssdqh " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x95 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsww " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x95 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsww " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x96 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x96 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x97 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdql " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x97 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdql " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x9e @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x9e 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x9f @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdqh " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0x9f 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmacsdqh " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xa2 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpcmov " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xa2 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpcmov " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 1 h vvv l 00 0xa2 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpcmov " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0x08 1 h vvv l 00 0xa2 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpcmov " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0x08 0 h vvv l 00 0xa3 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpperm " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xa3 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpperm " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 1 h vvv l 00 0xa3 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpperm " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," $addr ;
  0x08 1 h vvv l 00 0xa3 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpperm " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$s] "," vsse[$l*8+$r] wit("alt") ;
  0x08 0 h vvv l 00 0xa6 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmadcsswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xa6 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmadcsswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xb6 @addr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmadcswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," vsse[$l*8+$s] ;
  0x08 0 h vvv l 00 0xb6 11 ggg rrr 0 sss 0000 => wit($h==0 ? "vlo" : "") "vpmadcswd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," vsse[$l*8+$s] ;
  # ---- XOP 2-op / 3-op(W-swap) / 3-op+imm / GPR(TBM,LWP,bextr) --------
  0x08 0 1111 l 00 0xc0 @addr @imm8 => "vprotb " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0x08 0 1111 l 00 0xc0 11 ggg rrr @imm8 => "vprotb " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 1111 l 00 0xc1 @addr @imm8 => "vprotw " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0x08 0 1111 l 00 0xc1 11 ggg rrr @imm8 => "vprotw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 1111 l 00 0xc2 @addr @imm8 => "vprotd " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0x08 0 1111 l 00 0xc2 11 ggg rrr @imm8 => "vprotd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 1111 l 00 0xc3 @addr @imm8 => "vprotq " vsse[$l*8+$g] "," $addr "," hex($imm8) ;
  0x08 0 1111 l 00 0xc3 11 ggg rrr @imm8 => "vprotq " vsse[$l*8+$g] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcc @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcc 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcd @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcd 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xce @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xce 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcf @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xcf 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xec @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xec 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomub " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xed @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xed 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomuw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xee @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xee 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomud " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x08 0 h vvv l 00 0xef @addr @imm8 => wit($h==0 ? "vlo" : "") "vpcomuq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr "," hex($imm8) ;
  0x08 0 h vvv l 00 0xef 11 ggg rrr @imm8 => wit($h==0 ? "vlo" : "") "vpcomuq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] "," hex($imm8) ;
  0x09 0 h vvv 0 00 0x01 11 001 rrr => wit($h==0 ? "vlo" : "") "blcfill " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(1) => wit($h==0 ? "vlo" : "") "blcfill " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x01 11 011 rrr => wit($h==0 ? "vlo" : "") "blcs " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(3) => wit($h==0 ? "vlo" : "") "blcs " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x01 11 100 rrr => wit($h==0 ? "vlo" : "") "tzmsk " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(4) => wit($h==0 ? "vlo" : "") "tzmsk " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x01 11 101 rrr => wit($h==0 ? "vlo" : "") "blcic " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(5) => wit($h==0 ? "vlo" : "") "blcic " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x01 11 110 rrr => wit($h==0 ? "vlo" : "") "blsic " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(6) => wit($h==0 ? "vlo" : "") "blsic " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x01 11 111 rrr => wit($h==0 ? "vlo" : "") "t1mskc " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x01 @addr(7) => wit($h==0 ? "vlo" : "") "t1mskc " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x02 11 001 rrr => wit($h==0 ? "vlo" : "") "blcmsk " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x02 @addr(1) => wit($h==0 ? "vlo" : "") "blcmsk " greg[7-$v] "," $addr ;
  0x09 0 h vvv 0 00 0x02 11 110 rrr => wit($h==0 ? "vlo" : "") "blci " greg[7-$v] "," greg[$r] ;
  0x09 0 h vvv 0 00 0x02 @addr(6) => wit($h==0 ? "vlo" : "") "blci " greg[7-$v] "," $addr ;
  0x09 0 1111 0 00 0x12 11 000 rrr => "llwpcb " greg[$r] ;
  0x09 0 1111 0 00 0x12 11 001 rrr => "slwpcb " greg[$r] ;
  0x09 0 1111 l 00 0x80 @addr => "vfrczps " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0x80 11 ggg rrr => "vfrczps " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0x81 @addr => "vfrczpd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0x81 11 ggg rrr => "vfrczpd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0x82 @addr => "vfrczss " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0x82 11 ggg rrr => "vfrczss " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0x83 @addr => "vfrczsd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0x83 11 ggg rrr => "vfrczsd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 h vvv l 00 0x90 @addr => wit($h==0 ? "vlo" : "") "vprotb " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x90 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotb " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x90 @addr => wit($h==0 ? "vlo" : "") "vprotb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x90 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x91 @addr => wit($h==0 ? "vlo" : "") "vprotw " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x91 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x91 @addr => wit($h==0 ? "vlo" : "") "vprotw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x91 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x92 @addr => wit($h==0 ? "vlo" : "") "vprotd " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x92 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotd " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x92 @addr => wit($h==0 ? "vlo" : "") "vprotd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x92 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotd " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x93 @addr => wit($h==0 ? "vlo" : "") "vprotq " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x93 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotq " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x93 @addr => wit($h==0 ? "vlo" : "") "vprotq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x93 11 ggg rrr => wit($h==0 ? "vlo" : "") "vprotq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x94 @addr => wit($h==0 ? "vlo" : "") "vpshlb " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x94 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlb " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x94 @addr => wit($h==0 ? "vlo" : "") "vpshlb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x94 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlb " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x95 @addr => wit($h==0 ? "vlo" : "") "vpshlw " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x95 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x95 @addr => wit($h==0 ? "vlo" : "") "vpshlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x95 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x96 @addr => wit($h==0 ? "vlo" : "") "vpshld " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshld " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x96 @addr => wit($h==0 ? "vlo" : "") "vpshld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x96 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshld " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x97 @addr => wit($h==0 ? "vlo" : "") "vpshlq " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlq " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x97 @addr => wit($h==0 ? "vlo" : "") "vpshlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x97 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshlq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x98 @addr => wit($h==0 ? "vlo" : "") "vpshab " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshab " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x98 @addr => wit($h==0 ? "vlo" : "") "vpshab " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x98 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshab " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x99 @addr => wit($h==0 ? "vlo" : "") "vpshaw " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshaw " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x99 @addr => wit($h==0 ? "vlo" : "") "vpshaw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x99 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshaw " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x9a @addr => wit($h==0 ? "vlo" : "") "vpshad " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshad " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x9a @addr => wit($h==0 ? "vlo" : "") "vpshad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x9a 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshad " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 h vvv l 00 0x9b @addr => wit($h==0 ? "vlo" : "") "vpshaq " vsse[$l*8+$g] "," $addr "," vsse[$l*8+(7-$v)] ;
  0x09 0 h vvv l 00 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshaq " vsse[$l*8+$g] "," vsse[$l*8+$r] "," vsse[$l*8+(7-$v)] ;
  0x09 1 h vvv l 00 0x9b @addr => wit($h==0 ? "vlo" : "") "vpshaq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," $addr ;
  0x09 1 h vvv l 00 0x9b 11 ggg rrr => wit($h==0 ? "vlo" : "") "vpshaq " vsse[$l*8+$g] "," vsse[$l*8+(7-$v)] "," vsse[$l*8+$r] wit("alt") ;
  0x09 0 1111 l 00 0xc1 @addr => "vphaddbw " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xc1 11 ggg rrr => "vphaddbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xc2 @addr => "vphaddbd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xc2 11 ggg rrr => "vphaddbd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xc3 @addr => "vphaddbq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xc3 11 ggg rrr => "vphaddbq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xc6 @addr => "vphaddwd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xc6 11 ggg rrr => "vphaddwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xc7 @addr => "vphaddwq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xc7 11 ggg rrr => "vphaddwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xcb @addr => "vphadddq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xcb 11 ggg rrr => "vphadddq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xd1 @addr => "vphaddubw " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xd1 11 ggg rrr => "vphaddubw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xd2 @addr => "vphaddubd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xd2 11 ggg rrr => "vphaddubd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xd3 @addr => "vphaddubq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xd3 11 ggg rrr => "vphaddubq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xd6 @addr => "vphadduwd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xd6 11 ggg rrr => "vphadduwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xd7 @addr => "vphadduwq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xd7 11 ggg rrr => "vphadduwq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xdb @addr => "vphaddudq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xdb 11 ggg rrr => "vphaddudq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xe1 @addr => "vphsubbw " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xe1 11 ggg rrr => "vphsubbw " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xe2 @addr => "vphsubwd " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xe2 11 ggg rrr => "vphsubwd " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x09 0 1111 l 00 0xe3 @addr => "vphsubdq " vsse[$l*8+$g] "," $addr ;
  0x09 0 1111 l 00 0xe3 11 ggg rrr => "vphsubdq " vsse[$l*8+$g] "," vsse[$l*8+$r] ;
  0x0a 0 1111 0 00 0x10 11 ggg rrr @imm32 => "bextr " greg[$g] "," greg[$r] "," hex($imm32) ;
  0x0a 0 1111 0 00 0x10 @addr @imm32 => "bextr " greg[$g] "," $addr "," hex($imm32) ;
  0x0a 0 h vvv 0 00 0x12 11 000 rrr @imm32 => wit($h==0 ? "vlo" : "") "lwpins " greg[7-$v] "," greg[$r] "," hex($imm32) ;
  0x0a 0 h vvv 0 00 0x12 @addr(0) @imm32 => wit($h==0 ? "vlo" : "") "lwpins " greg[7-$v] "," $addr "," hex($imm32) ;
  0x0a 0 h vvv 0 00 0x12 11 001 rrr @imm32 => wit($h==0 ? "vlo" : "") "lwpval " greg[7-$v] "," greg[$r] "," hex($imm32) ;
  0x0a 0 h vvv 0 00 0x12 @addr(1) @imm32 => wit($h==0 ? "vlo" : "") "lwpval " greg[7-$v] "," $addr "," hex($imm32) ;
}


submatch main { @pfx(0) => $pfx }

'''

def main(argv):
  import sys
  g = Parser(DESCRIPTION).parse()
  asm = Asm(g)
  if not argv:
    sys.stderr.write('usage: x86a.py IN.asm > out.bin\n'); return 2
  out = bytearray(); fails = 0
  for ln in open(argv[0]):
    line = ln.rstrip('\n')
    if not line.strip() or line.lstrip().startswith(';'):
      continue
    try:
      out += asm_line(asm, line, len(out))
    except NoAsm as exc:
      fails += 1
      sys.stderr.write('FAIL @%d: %s  (%s)\n' % (len(out), line, exc))
  sys.stdout.buffer.write(bytes(out))
  return 1 if fails else 0

if __name__ == '__main__':
  import sys; sys.exit(main(sys.argv[1:]))
