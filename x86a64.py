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
  mode = env.get("__mode__", 32)
  if name in ("imm8", "imm8b", "disp8", "rel8"):
    return 1
  if name in ("imm16", "disp16"):
    return 2
  if name in ("imm32", "disp32"):
    return 4
  if name == "imm64":
    return 8
  if name in ("immz", "immz1"):
    # REX.W keeps the "z" immediate 32-bit (it overrides the 66 width).
    return 2 if (env.get("opsiz", 0) and not env.get("rexw", 0)) else 4
  if name in ("relz", "relz1"):
    return 2 if env.get("opsiz", 0) else 4
  if name in ("immadr", "immadr1"):
    # moffs absolute address: 64/32-bit in long mode, 32/16-bit otherwise.
    if mode == 64:
      return 4 if env.get("adrsiz", 0) else 8
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
    # general clean-positional bit-pack: a register/operand index such as
    # 32*$rexw + 16*$opsiz + 8*$rexr + $g (the x86-64 GP index) is a positional
    # number whose digits are the unknown fields.  Decompose target greedily
    # from the largest coefficient down, then verify -- this inverts any number
    # of non-overlapping power-positioned fields in one shot.  (The 1-/2-var
    # paths below remain for the offset/negative-coeff index exprs greedy skips.)
    if len(coeffs) >= 2 and all(c > 0 for c in coeffs.values()):
      items = sorted(coeffs.items(), key=lambda kv: -kv[1])
      r = rem
      trial = []
      for v, c in items:
        q = r // c
        r -= q * c
        trial.append((v, q))
      if r == 0 and all(q >= 0 for _, q in trial):
        save = dict(env)
        if all(_bind(env, v, q) for v, q in trial):
          try:
            if eval_expr(subst_known(e2, env), {}, g) == target:
              return True
          except NoAsm:
            pass
        env.clear(); env.update(save)
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
  # x86-64 addressing base/index files (16 regs; high 8 carry REX.B / REX.X).
  AREG64 = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
  AREG32E = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
             "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"]
  SEG_TBL = ["", "es:", "cs:", "ss:", "", "fs:", "gs:",
             "ss:", "es:", "cs:", "ss:", "ds:", "fs:", "gs:"]
  SBO = [0, 0, 0, 0, 7, 7, 0, 0]
  SBO16 = [0, 0, 7, 7, 0, 0, 7, 0]
  SBO64 = [0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0]

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
    if self.g["arch"].get("mode", 32) == 64:
      # default 64-bit addressing; the 0x67 form (eax..r15d / [eip+..]) is the
      # fallback.  Snapshot env so a failed attempt leaves no partial REX state.
      save = dict(env)
      adr = env.get("adrsiz")
      if not (isinstance(adr, int) and adr == 1):
        plan = self._enc64(inner, seg, wits, env, self.AREG64, 0)
        if plan is not None:
          return plan
        env.clear(); env.update(save)
      plan = self._enc64(inner, seg, wits, env, self.AREG32E, 1)
      if plan is None:
        env.clear(); env.update(save)
      return plan
    plan = self._enc32(inner, seg, wits, env)
    if plan is not None:
      return plan
    return self._enc16(inner, seg, wits, env)

  def _setrex(self, env, key, val):
    # bind a REX bit, honouring any value an earlier operand already pinned.
    if key in env and not isinstance(env[key], str):
      return int(env[key]) == val
    env[key] = val
    return True

  def _enc64(self, inner, seg, wits, env, regs, adrval):
    # Reverse-match an x86-64 memory operand.  `regs` is the 16-entry base/index
    # file (64- or 32-bit), `adrval` the resulting $adrsiz.  Sets env REX.B/REX.X
    # (base/index high bits) and $adrsiz; returns {mod,rm(low3),sib,disp,dw}.
    Rx = "(" + "|".join(regs) + ")"
    H = r"([+-]0x[0-9a-fA-F]+)"          # sgn()-style displacement
    riptok = "rip" if adrval == 0 else "eip"
    G = regs.index
    # [rip+disp] / [eip+disp]: mod=00 rm=101, no base/index registers.
    m = re.fullmatch(re.escape(riptok) + H, inner)
    if m:
      if not (self._set_seg(env, seg, 0) and self._setrex(env, "rexb", 0)
              and self._setrex(env, "rexx", 0)):
        return None
      env["adrsiz"] = adrval
      return {"mod": 0, "rm": 5, "sib": None, "disp": int(m.group(1), 16) & 0xffffffff, "dw": 4}
    base = index = disp = None
    scale = 1
    if (g := re.fullmatch(Rx + r"\+" + Rx + r"\*(\d+)" + H, inner)):
      base, index, scale, disp = G(g[1]), G(g[2]), int(g[3]), int(g[4], 16)
    elif (g := re.fullmatch(Rx + r"\+" + Rx + r"\*(\d+)", inner)):
      base, index, scale = G(g[1]), G(g[2]), int(g[3])
    elif (g := re.fullmatch(Rx + r"\*(\d+)\+(0x[0-9a-fA-F]+)", inner)):
      index, scale, disp = G(g[1]), int(g[2]), int(g[3], 16)
    elif (g := re.fullmatch(Rx + H, inner)):
      base, disp = G(g[1]), int(g[2], 16)
    elif (g := re.fullmatch(Rx, inner)):
      base = G(g[1])
    elif re.fullmatch(r"0x[0-9a-fA-F]+", inner):
      disp = int(inner, 16)
    else:
      return None
    s = {1: 0, 2: 1, 4: 2, 8: 3}.get(scale)
    if s is None:
      return None
    env["adrsiz"] = adrval
    if base is None and index is None:
      # absolute [disp32]: SIB base=101 index=100 mod=00 (mod=00 rm=101 is RIP).
      if not (self._set_seg(env, seg, 0) and self._setrex(env, "rexb", 0)
              and self._setrex(env, "rexx", 0)):
        return None
      return {"mod": 0, "rm": 4, "sib": (4 << 3) | 5, "disp": disp & 0xffffffff, "dw": 4}
    if index is not None and base is None:
      # [index*scale+disp32]: SIB base=101 mod=00.
      if not (self._set_seg(env, seg, 0) and self._setrex(env, "rexx", index >> 3)
              and self._setrex(env, "rexb", 0)):
        return None
      sib = (s << 6) | ((index & 7) << 3) | 5
      return {"mod": 0, "rm": 4, "sib": sib, "disp": (disp or 0) & 0xffffffff, "dw": 4}
    sbo = self.SBO64[base]
    if not (self._set_seg(env, seg, sbo) and self._setrex(env, "rexb", base >> 3)):
      return None
    need_sib = (index is not None) or ((base & 7) == 4)   # rsp/r12 always need SIB
    if not need_sib:
      if not self._setrex(env, "rexx", 0):
        return None
      if disp is None:
        if (base & 7) == 5:        # rbp/r13 can't be mod=00 -> disp8=0
          return {"mod": 1, "rm": base & 7, "sib": None, "disp": 0, "dw": 1}
        return {"mod": 0, "rm": base & 7, "sib": None, "disp": None, "dw": 0}
      mod, dw = self._pick_disp(disp, wits)
      return {"mod": mod, "rm": base & 7, "sib": None, "disp": disp & ((1 << (8 * dw)) - 1), "dw": dw}
    ib = (index & 7) if index is not None else 4    # 100 = no index
    if not self._setrex(env, "rexx", (index >> 3) if index is not None else 0):
      return None
    sib = (s << 6) | (ib << 3) | (base & 7)
    if disp is None:
      if (base & 7) == 5:          # rbp/r13 base in SIB needs a disp
        return {"mod": 1, "rm": 4, "sib": sib, "disp": 0, "dw": 1}
      return {"mod": 0, "rm": 4, "sib": sib, "disp": None, "dw": 0}
    mod, dw = self._pick_disp(disp, wits)
    return {"mod": mod, "rm": 4, "sib": sib, "disp": disp & ((1 << (8 * dw)) - 1), "dw": dw}

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


def revealed_bytes(env, explicit, mode=32):
  def iv(k):
    v = env.get(k, 0)
    return int(v) if isinstance(v, int) else 0
  rev = bytearray()
  for var in REVEAL_ORDER:
    if var in explicit:
      continue
    v = iv(var)
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
  # x86-64 REX (0100 WRXB): never an explicit token -- always re-derived from the
  # operands (W from a 64-bit name, R/X/B from an r8..r15 / SIB extension, the
  # bare 0x40 from an spl/bpl/sil/dil name).  Emitted last, right before the
  # opcode, as long mode requires.
  if mode == 64:
    w, r, x, b = iv("rexw"), iv("rexr"), iv("rexx"), iv("rexb")
    if w or r or x or b or iv("rex"):
      # each is a single REX bit; the multi-field index solver may hand back an
      # out-of-range value (e.g. rexr=3 when REX is pinned absent), so mask to 1
      # bit -- the redisassembly check in asm_line still verifies the encoding.
      rev.append(0x40 | ((w & 1) << 3) | ((r & 1) << 2) | ((x & 1) << 1) | (b & 1))
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
  env0["__mode__"] = asm.g["arch"].get("mode", 32)
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
      full = bytes(pfx) + revealed_bytes(env, explicit, asm.g["arch"].get("mode", 32)) + ibytes
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
# corpus64.p (v1, x86-64 long-mode disassembly) -- bit-pattern -> string rewriter.
#
# x86-64 companion to corpus.p.  Same engine (parsergen.py to disassemble,
# asm.py to assemble), same conventions (size-paired register tables, mod=11 +
# @addr per ModR/M op, prefix-indexed mnemonic tables, inline segment display,
# wit() annotations for non-canonical encodings).  The long-mode additions over
# the 32-bit file are:
#
#   * REX prefix (0x40..0x4F = 0100 WRXB).  Handled as a prefix frame that sets
#     $rexw/$rexr/$rexx/$rexb/$rex and emits NO token: its bits are recovered
#     directly from the register names (a 64-bit name reveals W, an r8..r15 name
#     reveals the R/X/B extension, an spl/bpl/sil/dil name reveals REX presence),
#     exactly as the 66 prefix is recovered from a 16-bit register in corpus.p.
#     The assembler re-derives the REX byte from the operands (see asm.py).
#   * Register files widened to 16 (r8..r15) and to 64-bit (rax..r15).  The GP
#     index is the clean bit-pack 32*W + 16*opsiz + 8*hi + lo so the assembler's
#     reverse solver can invert it; `hi` is REX.R for a reg field, REX.B for an
#     rm / base / opcode-reg field, REX.X for a SIB index.
#   * 64-bit addressing: mod=00 rm=101 is RIP-relative ([rip+disp32]); absolute
#     [disp32] is the SIB base=101 index=none form; bases/indexes are rax..r15
#     (REX.B/REX.X extended).  The 0x67 form switches to 32-bit addressing.
#   * Default-64 operand size for the stack / near-branch group (push/pop/call/
#     jmp/Jcc/...): 64-bit unless 66, REX.W ignored -- a separate `dreg` table.
#   * movabs (B8+r under REX.W: a true 8-byte imm64); movsxd (0x63); the 1-byte
#     inc/dec (0x40..0x4F) and the removed legacy opcodes are gone (their bytes
#     are REX / new meanings in long mode).

arch  $mode=64 $endian=le $bitorder=msb $maxlen=15
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0 $rexw=0 $rexr=0 $rexx=0 $rexb=0 $rex=0

# ===========================================================================
# register / addressing tables
# ===========================================================================
# GP registers, W-sensitive ops.  index = 32*$rexw + 16*$opsiz + 8*hi + lo,
# hi = REX.{R|B|X}, lo = the 3-bit field.  Four 16-register banks:
#   rexw=0 opsiz=0 : 32-bit    rexw=0 opsiz=1 : 16-bit
#   rexw=1 opsiz=0 : 64-bit    rexw=1 opsiz=1 : 64-bit (redundant 66)
table greg {
  eax,ecx,edx,ebx,esp,ebp,esi,edi,r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d,
  ax,cx,dx,bx,sp,bp,si,di,r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15
}
# Default-64 GP (stack / near branches): 64-bit unless 66->16; REX.W ignored.
# index = 16*$opsiz + 8*hi + lo.
table dreg {
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15,
  ax,cx,dx,bx,sp,bp,si,di,r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w
}
# 8-bit GP.  index = 16*$rex + 8*hi + lo.  Without REX: ah/ch/dh/bh at 4..7;
# with REX: spl/bpl/sil/dil and r8b..r15b.  The rex=0,hi=1 bank is unreachable.
table rgb {
  al,cl,dl,bl,ah,ch,dh,bh,
  "","","","","","","","",
  al,cl,dl,bl,spl,bpl,sil,dil,
  r8b,r9b,r10b,r11b,r12b,r13b,r14b,r15b
}
# Addressing-base registers: always full width regardless of opsiz/REX.W.
table areg   { rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15 }   # adrsiz=0
table areg32 { eax,ecx,edx,ebx,esp,ebp,esi,edi,r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d } # adrsiz=1
# MMX (never extended) and XMM (REX-extended, 16 regs).
table mmreg  { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7 }
table xreg   { xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15 }
# MMX/XMM merged for the 66-split integer ops: index 8*$opsiz + 16*REX.{R|B} + f
# selects mm0..7 (no 66) or xmm0..15 (66 -> xmm).  The REX high bit is weight 16
# so opsiz and the REX bit stay separable by the assembler's solver; the
# opsiz=0 + REX-bit=1 bank (16..23) is unreachable (REX on an MMX form).
table simd {
  mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7,
  xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,
  "","","","","","","","",
  xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
}
table sreg   { es,cs,ss,ds,fs,gs }
table cond   { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }
table crreg  { cr0,cr1,cr2,cr3,cr4,cr5,cr6,cr7,cr8,cr9,cr10,cr11,cr12,cr13,cr14,cr15 }
table drreg  { dr0,dr1,dr2,dr3,dr4,dr5,dr6,dr7,dr8,dr9,dr10,dr11,dr12,dr13,dr14,dr15 }

# size suffix : the dot is part of the entry; index = operand size in bytes.
table sfx    { "",".b",".w","",".d","","","",".q","",".t","","","","","","" }

# segment display : index = sbo(base)+segidx.  ss is the default for rsp/rbp
# (and r12/r13, same SIB base encoding); ds elsewhere.
table seg    { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo    { 0,0,0,0,7,7,0,0, 0,0,0,0,7,7,0,0 }
table sc     { 1, 2, 4, 8 }

# prefix-indexed / op-indexed mnemonic tables
table alu    { add,or,adc,sbb,and,sub,xor,cmp }
table shift  { rol,ror,rcl,rcr,shl,shr,shl,sar }
table grp3   { test,test,not,neg,mul,imul,div,idiv }
table movs   { movsd,movsw,movsq }
table stos   { stosd,stosw,stosq }
table lods   { lodsd,lodsw,lodsq }
table cmps   { cmpsd,cmpsw,cmpsq }
table scas   { scasd,scasw,scasq }
table insx   { insd,insw }
table outsx  { outsd,outsw }
table cbw_t  { cwde,cbw,cdqe }         # 0x98 by ($rexw?2:$opsiz): cwde/cbw/cdqe
table cwd_t  { cdq,cwd,cqo }           # 0x99 by ($rexw?2:$opsiz): cdq/cwd/cqo
table d8r    { fadd,fmul,fcom,fcomp,fsub,fsubr,fdiv,fdivr }
table dcr    { fadd,fmul,fcom,fcomp,fsubr,fsub,fdivr,fdiv }
table dar    { fcmovb,fcmove,fcmovbe,fcmovu }
table dbr    { fcmovnb,fcmovne,fcmovnbe,fcmovnu }
table der    { faddp,fmulp,fcompp,fcompp,fsubrp,fsubp,fdivrp,fdivp }
table pcnt   { "",popcnt,"" }          # F3 0F B8
table tzt    { "",tzcnt,"" }           # F3 0F BC
table lzt    { "",lzcnt,"" }           # F3 0F BD
table grpba  { "","","","",bt,bts,btr,btc }   # 0F BA /op
table c7r7   { rdseed,rdpid,rdseed,rdseed }   # 0F C7 /7 by reptype
table c7r6   { rdrand,rdrand,vmxon,rdrand }   # 0F C7 /6 reg by reptype
table cx16   { cmpxchg8b, cmpxchg16b }        # 0F C7 /1 by REX.W (m64 / m128)

# ---- SSE / MMX mnemonic tables (indexed by reptype*2+opsiz or opsiz) ----
table elt    { ps,pd,ss,ss,sd,sd }     # packed/scalar element by reptype*2+opsiz
table uc     { ucomiss,ucomisd }       # 0F 2E by opsiz
table comi   { comiss,comisd }         # 0F 2F by opsiz
table mova2  { movaps,movapd }         # 0F 28/29 by opsiz
table movu   { movups,movupd,movss,movss,movsd,movsd }  # 0F 10/11 reptype*2+opsiz
table movusz { 16,16,4,4,8,8 }         # 0F 10/11 mem size
table mnt4   { movntps,movntpd,movntss,movntss,movntsd,movntsd } # 0F 2B
table ml12   { movlps,movlpd }         # 0F 12/13 mem by opsiz
table mh16   { movhps,movhpd }         # 0F 16/17 mem by opsiz
table m12f   { "",movsldup,movddup }   # 0F 12 by reptype
table m16f   { "",movshdup,"" }        # 0F 16 by reptype
table unpl   { unpcklps,unpcklpd }     # 0F 14 by opsiz
table unph   { unpckhps,unpckhpd }     # 0F 15 by opsiz
table mdq    { movq,movdqa,movdqu }    # 0F 6F/7F reptype*2+opsiz (idx 0/1/2)
table mdq2   { movd,movq }             # 0F 6E/7E GP move by REX.W
table cvt2a  { cvtpi2ps,cvtpi2pd,cvtsi2ss,cvtsi2ss,cvtsi2sd,cvtsi2sd } # 0F 2A
table cvt2c  { cvttps2pi,cvttpd2pi,cvttss2si,cvttss2si,cvttsd2si,cvttsd2si } # 0F 2C
table cvt2d  { cvtps2pi,cvtpd2pi,cvtss2si,cvtss2si,cvtsd2si,cvtsd2si } # 0F 2D
table cvt5a  { cvtps2pd,cvtpd2ps,cvtss2sd,cvtss2sd,cvtsd2ss,cvtsd2ss } # 0F 5A
table cvt5b  { cvtdq2ps,cvtps2dq,cvttps2dq,cvttps2dq } # 0F 5B
table me6    { "",cvttpd2dq,cvtdq2pd,"",cvtpd2dq,"" } # 0F E6
table me7    { movntq,movntdq }        # 0F E7 by opsiz
table mf7    { maskmovq,maskmovdqu }   # 0F F7 by opsiz
table md6    { "",movq2dq,movdq2q }    # 0F D6 reptype (F3/F2)
table d0t    { "",addsubpd,"","",addsubps,"" } # 0F D0
table h7c    { "",haddpd,"","",haddps,"" }      # 0F 7C
table h7d    { "",hsubpd,"","",hsubps,"" }      # 0F 7D
table pshuf  { pshufw,pshufd,pshufhw,pshufhw,pshuflw,pshuflw } # 0F 70
table m7e    { "",movq }               # 0F 7E F3 (reload)
table ldqt   { "","",lddqu }           # F2 0F F0
table pextrdq { pextrd, pextrq }       # 0F3A 16 by REX.W
table pinsrdq { pinsrd, pinsrq }       # 0F3A 22 by REX.W

# ===========================================================================
# immediates / displacements
# ===========================================================================
submatch imm8  { iiiiiiii                              => $i }
submatch imm16 { iiiiiiii iiiiiiii                     => $i }
submatch imm32 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii   => $i }
submatch imm64 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii => $i }
submatch disp8 { dddddddd                              => sx8($d) }
submatch disp32{ dddddddd dddddddd dddddddd dddddddd   => $d }
submatch rel8  { dddddddd                              => $E + sx8($d) }
# "z" immediate: 16-bit under 66, else 32-bit.  REX.W keeps it 32-bit (the 66 is
# ignored once W is set), so the dispatch is ($rexw ? 0 : $opsiz).
submatch immz1($opsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch immz { @immz1($rexw ? 0 : $opsiz) => $immz1 }
submatch relz1($opsiz) { <0> @imm32 => $E+sx32($imm32) ;  <1> @imm16 => $E+sx16($imm16) }
submatch relz { @relz1($opsiz) => $relz1 }
# moffs absolute address: 64-bit unless 0x67 (then 32-bit).
submatch immadr1($adrsiz) { <0> @imm64 => $imm64 ;  <1> @imm32 => $imm32 }
submatch immadr { @immadr1($adrsiz) => $immadr1 }

# ===========================================================================
# prefix run : legacy prefixes recurse through @pfx; REX goes straight to @insn
# (it must be the last prefix before the opcode).  REX emits no token.
# ===========================================================================
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
  0100 wrxb {$rexw=$w; $rexr=$r; $rexx=$x; $rexb=$b; $rex=1} @insn => $insn ;
  @insn => $insn ;
}

# ===========================================================================
# addressing : MEMORY ONLY (mod != 11).  addr() injects current adrsiz.
# ===========================================================================
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }
submatch addr1($adrsiz, $g) {
  # ---- 64-bit addressing (adrsiz = 0) ----
  <0> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <0> 00 ggg 101 @disp32        => seg[$segidx] "[rip" sgn($disp32) "]" ;
  <0> 00 ggg rrr                => seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] "]" ;
  <0> 01 ggg 100 @sib1 @disp8   => wit($disp8==0 ? ($sbase!=5 ? "disp8" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <0> 01 ggg rrr @disp8         => wit($disp8==0 ? "disp8" : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] sgn($disp8) "]" ;
  <0> 10 ggg 100 @sib1 @disp32  => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <0> 10 ggg rrr @disp32        => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] sgn($disp32) "]" ;
  # ---- 32-bit addressing (adrsiz = 1, the 0x67 form); mod=00 rm=101 = EIP-rel ----
  <1> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <1> 00 ggg 101 @disp32        => seg[$segidx] "[eip" sgn($disp32) "]" ;
  <1> 00 ggg rrr                => seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] "]" ;
  <1> 01 ggg 100 @sib1 @disp8   => wit($disp8==0 ? ($sbase!=5 ? "disp8" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <1> 01 ggg rrr @disp8         => wit($disp8==0 ? "disp8" : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] sgn($disp8) "]" ;
  <1> 10 ggg 100 @sib1 @disp32  => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <1> 10 ggg rrr @disp32        => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] sgn($disp32) "]" ;
}
# SIB, mod=00.  No-index requires REX.X=0 & index=100; no-base is base=101.
submatch sib0 {
  ss 100 101 @disp32 [$rexx==0] {$sbase=none} => "[" hex($disp32) "]" ;
  ss iii 101 @disp32 {$sbase=none}            => "[" areg[8*$rexx+$i] "*" dec(sc[$s]) "+" hex($disp32) "]" ;
  ss 100 bbb [$rexx==0] {$sbase=8*$rexb+$b}   => "[" areg[8*$rexb+$b] "]" ;
  ss iii bbb {$sbase=8*$rexb+$b}              => "[" areg[8*$rexb+$b] "+" areg[8*$rexx+$i] "*" dec(sc[$s]) "]" ;
}
submatch sib1 {
  ss 100 bbb [$rexx==0] {$sbase=8*$rexb+$b} => areg[8*$rexb+$b] ;
  ss iii bbb {$sbase=8*$rexb+$b}            => areg[8*$rexb+$b] "+" areg[8*$rexx+$i] "*" dec(sc[$s]) ;
}

# ===========================================================================
# instruction decoder
# ===========================================================================
submatch insn {
  # --- lea / mov family -----------------------------------------------------
  0x8d @addr => "lea " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x88 11 ggg rrr => "mov " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x89 11 ggg rrr => "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x89 @addr      => "mov " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x8b 11 ggg rrr => wit("alt") "mov " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x8b @addr      => "mov " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x8a 11 ggg rrr => wit("alt") "mov " rgb[16*$rex+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x8a @addr      => "mov" sfx[1] " " rgb[16*$rex+8*$rexr+$g] "," $addr ;
  0x8c 11 ggg rrr => "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," sreg[$g] ;
  0x8c @addr      => "mov " $addr "," sreg[$g] ;
  0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[16+8*$rexb+$r] ;
  0x8e @addr      => "mov " sreg[$g] "," $addr ;
  0xa0 @immadr => "mov al," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa1 @immadr => "mov " greg[32*$rexw+16*$opsiz+0] "," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa2 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "],al" ;
  0xa3 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "]," greg[32*$rexw+16*$opsiz+0] ;
  10110 bbb @imm8 => "mov " rgb[16*$rex+8*$rexb+$b] "," hex($imm8) ;
  10111 bbb @imm64 [$rexw==1] => "movabs " greg[32*$rexw+8*$rexb+$b] "," hex($imm64) ;
  10111 bbb @immz => "mov " greg[16*$opsiz+8*$rexb+$b] "," hex($immz) ;
  0xc6 11 000 rrr @imm8 => wit("long") "mov " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;
  0xc7 11 000 rrr @immz => wit("long") "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) ;
  0xc7 @addr(0)   @immz => "mov" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) ;
  0x63 11 ggg rrr => "movsxd " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[8*$rexb+$r] ;
  0x63 @addr      => "movsxd " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- ALU group (00..3D) : op table; Eb/Ev, both directions, AL/eAX imm -----
  00 fff 000 11 ggg rrr => alu[$f] " " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  00 fff 000 @addr      => alu[$f] sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  00 fff 010 11 ggg rrr => wit("alt") alu[$f] " " rgb[16*$rex+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  00 fff 010 @addr      => alu[$f] sfx[1] " " rgb[16*$rex+8*$rexr+$g] "," $addr ;
  00 fff 001 11 ggg rrr => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  00 fff 001 @addr      => alu[$f] " " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  00 fff 011 11 ggg rrr => wit("alt") alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  00 fff 011 @addr      => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  00 fff 100 @imm8 => alu[$f] " al," hex($imm8) ;
  00 fff 101 @immz => alu[$f] " " greg[32*$rexw+16*$opsiz+0] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;

  # --- group 1 (80/81/83): 8 ALU ops via alu[$f] ----------------------------
  0x80 11 fff rrr @imm8 => wit($r==0 ? "long" : "") alu[$f] " " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0x80 @addr      @imm8 => alu[$g] sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 fff rrr @immz => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) wit($r==0 ? "long" : "") wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x81 @addr      @immz => alu[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x83 11 fff rrr @imm8 => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex(sx8($imm8)) ;
  0x83 @addr      @imm8 => alu[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex(sx8($imm8)) ;

  # --- inc/dec/call/jmp/push group FE/FF; unary F6/F7; shifts; test ---------
  0xff 11 000 rrr => "inc " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xff @addr(0)   => "inc" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 001 rrr => "dec " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xff @addr(1)   => "dec" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 010 rrr => "call " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(2)   => "call" sfx[8] " " $addr ;
  0xff 11 100 rrr => "jmp " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(4)   => "jmp" sfx[8] " " $addr ;
  0xff @addr(3)   => "call far" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff @addr(5)   => "jmp far" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 110 rrr => "push " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(6)   => "push" sfx[8] " " $addr ;
  0xfe 11 000 rrr => "inc " rgb[16*$rex+8*$rexb+$r] ;
  0xfe @addr(0)   => "inc" sfx[1] " " $addr ;
  0xfe 11 001 rrr => "dec " rgb[16*$rex+8*$rexb+$r] ;
  0xfe @addr(1)   => "dec" sfx[1] " " $addr ;
  0xf7 11 000 rrr @immz => wit($r==0 ? "long" : "") "test " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) ;
  0xf7 11 fff rrr => grp3[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xf7 @addr(0) @immz => "test" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) ;
  0xf7 @addr      => grp3[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xf6 11 000 rrr @imm8 => wit($r==0 ? "long" : "") "test " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xf6 11 fff rrr => grp3[$f] " " rgb[16*$rex+8*$rexb+$r] ;
  0xf6 @addr(0) @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 @addr      => grp3[$g] sfx[1] " " $addr ;
  0x84 11 ggg rrr => "test " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x84 @addr      => "test" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x85 11 ggg rrr => "test " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x85 @addr      => "test " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0xa8 @imm8 => "test al," hex($imm8) ;
  0xa9 @immz => "test " greg[32*$rexw+16*$opsiz+0] "," hex($immz) ;
  0xc1 11 fff rrr @imm8 => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0xc1 @addr @imm8 => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0xd1 11 fff rrr => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ",1" ;
  0xd1 @addr => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ",1" ;
  0xd3 11 fff rrr => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ",cl" ;
  0xd3 @addr => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ",cl" ;
  0xc0 11 fff rrr @imm8 => shift[$f] " " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xc0 @addr @imm8 => shift[$g] sfx[1] " " $addr "," hex($imm8) ;
  0xd0 11 fff rrr => shift[$f] " " rgb[16*$rex+8*$rexb+$r] ",1" ;
  0xd0 @addr => shift[$g] sfx[1] " " $addr ",1" ;
  0xd2 11 fff rrr => shift[$f] " " rgb[16*$rex+8*$rexb+$r] ",cl" ;
  0xd2 @addr => shift[$g] sfx[1] " " $addr ",cl" ;

  # --- imul Gv,Ev,imm -------------------------------------------------------
  0x69 11 ggg rrr @immz => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x69 @addr @immz      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6b 11 ggg rrr @imm8 => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0x6b @addr @imm8      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr "," hex($imm8) ;

  # --- xchg / stack / nop ---------------------------------------------------
  0x90 [$reptype==1] => "pause" ;
  0x90 [$rexb==0] => "nop" ;
  10010 bbb => "xchg " greg[32*$rexw+16*$opsiz+8*$rexb+$b] "," greg[32*$rexw+16*$opsiz+0] ;
  0x87 11 ggg rrr => "xchg " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x87 @addr      => "xchg " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x86 11 ggg rrr => "xchg " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x86 @addr      => "xchg" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x68 @immz => "push " hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6a @imm8 => "push " hex($imm8) ;
  01010 bbb => "push " dreg[16*$opsiz+8*$rexb+$b] ;
  01011 bbb => "pop " dreg[16*$opsiz+8*$rexb+$b] ;
  0x8f 11 000 rrr => wit("long") "pop " dreg[16*$opsiz+8*$rexb+$r] ;
  0x8f @addr(0) => "pop" sfx[8] " " $addr ;
  0x9c => "pushf" ;
  0x9d => "popf" ;

  # --- branches / ret / int / enter / leave ---------------------------------
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xe9 @relz => "jmp " hex($relz) ;
  0xe8 @relz => "call " hex($relz) ;
  0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;
  0xe3 @rel8 => "jrcxz " hex($rel8) ;
  0xe2 @rel8 => "loop " hex($rel8) ;
  0xe1 @rel8 => "loope " hex($rel8) ;
  0xe0 @rel8 => "loopne " hex($rel8) ;
  0xc3 => "ret" ;
  0xc2 @imm16 => "ret " hex($imm16) ;
  0xcb => "retf" ;
  0xca @imm16 => "retf " hex($imm16) ;
  0xcd @imm8 => "int " hex($imm8) ;
  0xcc => "int3" ;
  0xcf => "iret" ;
  0xc9 => "leave" ;
  0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;
  0xf1 => "int1" ;

  # --- string ops -----------------------------------------------------------
  0xa4 => "movsb" ;
  0xa5 => movs[$rexw? 2 : $opsiz] ;
  0xaa => "stosb" ;
  0xab => stos[$rexw? 2 : $opsiz] ;
  0xac => "lodsb" ;
  0xad => lods[$rexw? 2 : $opsiz] ;
  0xa6 => "cmpsb" ;
  0xae => "scasb" ;
  0xa7 => cmps[$rexw? 2 : $opsiz] ;
  0xaf => scas[$rexw? 2 : $opsiz] ;

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
  0x99 => cwd_t[$rexw? 2 : $opsiz] ;
  0x98 => cbw_t[$rexw? 2 : $opsiz] ;
  0x9b => "fwait" ;
  0xd7 => "xlatb" ;

  # --- I/O ------------------------------------------------------------------
  0xe4 @imm8 => "in al," hex($imm8) ;
  0xe5 @imm8 => "in " greg[16*$opsiz+0] "," hex($imm8) ;
  0xed => "in " greg[16*$opsiz+0] ",dx" ;
  0xe6 @imm8 => "out " hex($imm8) ",al" ;
  0xe7 @imm8 => "out " hex($imm8) "," greg[16*$opsiz+0] ;
  0xec => "in al,dx" ;
  0xee => "out dx,al" ;
  0xef => "out dx," greg[16*$opsiz+0] ;
  0x6c => "insb" ;
  0x6e => "outsb" ;
  0x6d => insx[$opsiz] ;
  0x6f => outsx[$opsiz] ;

  # ====================== two-byte 0F : GP / system ========================
  # --- jcc near (0F 80..8F, rel32) / setcc / cmovcc -------------------------
  0x0f 1000 cccc @relz => wit("long") "j" cond[$c] " " hex($relz) ;
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[16*$rex+8*$rexb+$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- movzx / movsx --------------------------------------------------------
  0x0f 0xb6 11 ggg rrr => "movzx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0xb6 @addr      => "movzx" sfx[1] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[16+8*$rexb+$r] ;
  0x0f 0xb7 @addr      => "movzx" sfx[2] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0xbe @addr      => "movsx" sfx[1] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbf 11 ggg rrr => "movsx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[16+8*$rexb+$r] ;
  0x0f 0xbf @addr      => "movsx" sfx[2] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- imul / bit string / shld / shrd --------------------------------------
  0x0f 0xaf 11 ggg rrr => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xaf @addr      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xa3 11 ggg rrr => "bt " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xa3 @addr      => "bt " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xab 11 ggg rrr => "bts " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xab @addr      => "bts " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb3 11 ggg rrr => "btr " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb3 @addr      => "btr " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xbb 11 ggg rrr => "btc " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xbb @addr      => "btc " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xba 11 fff rrr @imm8 => grpba[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xba @addr(4) @imm8 => "bt" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(5) @imm8 => "bts" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(6) @imm8 => "btr" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(7) @imm8 => "btc" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xa4 @addr @imm8 => "shld " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xa5 11 ggg rrr => "shld " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xa5 @addr => "shld " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xac @addr @imm8 => "shrd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xad 11 ggg rrr => "shrd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xad @addr => "shrd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;

  # --- bsf / bsr / tzcnt / lzcnt / popcnt -----------------------------------
  0x0f 0xbc 11 ggg rrr [$reptype==1] => "tzcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbc 11 ggg rrr => "bsf " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbc @addr      => "bsf " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr [$reptype==1] => "lzcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbd 11 ggg rrr => "bsr " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbd @addr      => "bsr " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xb8 11 ggg rrr [$reptype==1] => "popcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xb8 @addr [$reptype==1]      => "popcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- cmpxchg / xadd / cmpxchg8b16b / movnti / bswap -----------------------
  0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xb0 @addr      => "cmpxchg" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc0 11 ggg rrr => "xadd " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xc0 @addr      => "xadd" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xc1 11 ggg rrr => "xadd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc1 @addr      => "xadd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc7 @addr(1) => cx16[$rexw] " " $addr ;
  0x0f 0xc7 @addr(6) => "vmptrld " $addr ;
  0x0f 0xc7 @addr(7) => "vmptrst " $addr ;
  0x0f 0xc7 11 110 rrr => c7r6[$reptype] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xc7 11 111 rrr => c7r7[$reptype] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xc3 @addr      => "movnti " $addr "," greg[32*$rexw+8*$rexr+$g] ;
  0x0f 11001 bbb => "bswap " greg[32*$rexw+16*$opsiz+8*$rexb+$b] ;

  # --- segment push/pop, cpuid, msr, sys ------------------------------------
  0x0f 0xa0 => "push fs" ;
  0x0f 0xa1 => "pop fs" ;
  0x0f 0xa8 => "push gs" ;
  0x0f 0xa9 => "pop gs" ;
  0x0f 0xaa => "rsm" ;
  0x0f 0xa2 => "cpuid" ;
  0x0f 0x05 => "syscall" ;
  0x0f 0x07 => "sysret" ;
  0x0f 0x06 => "clts" ;
  0x0f 0x08 => "invd" ;
  0x0f 0x09 => "wbinvd" ;
  0x0f 0x0b => "ud2" ;
  0x0f 0x30 => "wrmsr" ;
  0x0f 0x31 => "rdtsc" ;
  0x0f 0x32 => "rdmsr" ;
  0x0f 0x33 => "rdpmc" ;
  0x0f 0x34 => "sysenter" ;
  0x0f 0x35 => "sysexit" ;
  0x0f 0x37 => "getsec" ;
  0x0f 0x77 => "emms" ;
  0x0f 0xb9 11 ggg rrr => "ud1 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xb9 @addr      => "ud1 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xff 11 ggg rrr => "ud0 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;

  # --- 0F 00 (sldt/str/lldt/ltr/verr/verw) / 0F 01 descriptor + system ------
  0x0f 0x00 11 000 rrr => "sldt " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x00 11 001 rrr => "str " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x00 11 010 rrr => "lldt " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 011 rrr => "ltr " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 100 rrr => "verr " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 101 rrr => "verw " greg[16+8*$rexb+$r] ;
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
  0x0f 0x01 11 100 rrr => "smsw " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x01 11 110 rrr => "lmsw " greg[16+8*$rexb+$r] ;
  0x0f 0x01 0xc8 => "monitor" ;
  0x0f 0x01 0xc9 => "mwait" ;
  0x0f 0x01 0xca => "clac" ;
  0x0f 0x01 0xcb => "stac" ;
  0x0f 0x01 0xd0 => "xgetbv" ;
  0x0f 0x01 0xd1 => "xsetbv" ;
  0x0f 0x01 0xf8 => "swapgs" ;
  0x0f 0x01 0xf9 => "rdtscp" ;

  # --- lar / lsl / mov cr,dr ------------------------------------------------
  0x0f 0x02 11 ggg rrr => "lar " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x02 @addr      => "lar " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x03 11 ggg rrr => "lsl " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x03 @addr      => "lsl " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x20 11 ggg rrr => "mov " greg[32+8*$rexb+$r] "," crreg[8*$rexr+$g] ;
  0x0f 0x22 11 ggg rrr => "mov " crreg[8*$rexr+$g] "," greg[32+8*$rexb+$r] ;
  0x0f 0x21 11 ggg rrr => "mov " greg[32+8*$rexb+$r] "," drreg[8*$rexr+$g] ;
  0x0f 0x23 11 ggg rrr => "mov " drreg[8*$rexr+$g] "," greg[32+8*$rexb+$r] ;

  # --- nop / prefetch / fences / fxsave group / clflush ---------------------
  0x0f 0x1f 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x1f @addr      => "nop" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0x0f 0x0d @addr(1) => "prefetchw" sfx[1] " " $addr ;
  0x0f 0x18 @addr(0) => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(1) => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2) => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3) => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0xae 0xe8 => "lfence" ;
  0x0f 0xae 0xf0 => "mfence" ;
  0x0f 0xae 0xf8 => "sfence" ;
  0x0f 0xae @addr(0) => "fxsave " $addr ;
  0x0f 0xae @addr(1) => "fxrstor " $addr ;
  0x0f 0xae @addr(2) => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3) => "stmxcsr " $addr ;
  0x0f 0xae @addr(4) => "xsave " $addr ;
  0x0f 0xae @addr(5) => "xrstor " $addr ;
  0x0f 0xae @addr(6) => "xsaveopt " $addr ;
  0x0f 0xae @addr(7) => "clflush" sfx[1] " " $addr ;

  # ====================== two-byte 0F : SSE / SSE2 / SSE3 ==================
  0x0f 0x10 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x10 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x11 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x11 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x12 11 ggg rrr [$reptype] => m12f[$reptype] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x12 @addr      [$reptype] => m12f[$reptype] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr => "movhlps " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x12 @addr      => ml12[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x13 @addr      => ml12[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x14 11 ggg rrr => unpl[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x14 @addr      => unpl[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => unph[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x15 @addr      => unph[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr [$reptype] => m16f[$reptype] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x16 @addr      [$reptype] => m16f[$reptype] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr => "movlhps " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x16 @addr      => mh16[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x17 @addr      => mh16[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x28 11 ggg rrr => mova2[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x28 @addr      => mova2[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x29 11 ggg rrr => mova2[$opsiz] " " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x29 @addr      => mova2[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x2b @addr      => mnt4[$reptype*2+$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x2a 11 ggg rrr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] ;
  0x0f 0x2a 11 ggg rrr => cvt2a[$reptype*2+$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x2a @addr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x2a @addr      => cvt2a[$reptype*2+$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x2c 11 ggg rrr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2c 11 ggg rrr => cvt2c[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2c @addr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x2d 11 ggg rrr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2d 11 ggg rrr => cvt2d[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2d @addr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2e @addr      => uc[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => comi[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2f @addr      => comi[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x50 11 ggg rrr => "movmsk" elt[$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x51 11 ggg rrr => "sqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x51 @addr      => "sqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => "rsqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x52 @addr      => "rsqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => "rcp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x53 @addr      => "rcp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => "and" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x54 @addr      => "and" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => "andn" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x55 @addr      => "andn" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => "or" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x56 @addr      => "or" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => "xor" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x57 @addr      => "xor" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => "add" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x58 @addr      => "add" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => "mul" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x59 @addr      => "mul" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => "sub" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5c @addr      => "sub" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => "min" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5d @addr      => "min" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => "div" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5e @addr      => "div" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => "max" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5f @addr      => "max" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => cvt5a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5a @addr      => cvt5a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => cvt5b[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5b @addr      => cvt5b[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x60 11 ggg rrr => "punpcklbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x60 @addr      => "punpcklbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x61 11 ggg rrr => "punpcklwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x61 @addr      => "punpcklwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x62 11 ggg rrr => "punpckldq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x62 @addr      => "punpckldq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x63 11 ggg rrr => "packsswb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x63 @addr      => "packsswb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x64 11 ggg rrr => "pcmpgtb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x64 @addr      => "pcmpgtb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x65 11 ggg rrr => "pcmpgtw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x65 @addr      => "pcmpgtw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x66 11 ggg rrr => "pcmpgtd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x66 @addr      => "pcmpgtd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x67 11 ggg rrr => "packuswb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x67 @addr      => "packuswb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x68 11 ggg rrr => "punpckhbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x68 @addr      => "punpckhbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x69 11 ggg rrr => "punpckhwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x69 @addr      => "punpckhwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6a 11 ggg rrr => "punpckhdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6a @addr      => "punpckhdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6b 11 ggg rrr => "packssdw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6b @addr      => "packssdw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6c 11 ggg rrr => "punpcklqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6c @addr      => "punpcklqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6d 11 ggg rrr => "punpckhqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6d @addr      => "punpckhqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x74 11 ggg rrr => "pcmpeqb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x74 @addr      => "pcmpeqb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x75 11 ggg rrr => "pcmpeqw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x75 @addr      => "pcmpeqw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x76 11 ggg rrr => "pcmpeqd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x76 @addr      => "pcmpeqd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd1 11 ggg rrr => "psrlw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd1 @addr      => "psrlw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd2 @addr      => "psrld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd3 @addr      => "psrlq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd4 11 ggg rrr => "paddq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd4 @addr      => "paddq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd5 @addr      => "pmullw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd8 11 ggg rrr => "psubusb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd8 @addr      => "psubusb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd9 11 ggg rrr => "psubusw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd9 @addr      => "psubusw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xda 11 ggg rrr => "pminub " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xda @addr      => "pminub " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdb 11 ggg rrr => "pand " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdb @addr      => "pand " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdc 11 ggg rrr => "paddusb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdc @addr      => "paddusb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdd 11 ggg rrr => "paddusw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdd @addr      => "paddusw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xde 11 ggg rrr => "pmaxub " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xde @addr      => "pmaxub " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdf 11 ggg rrr => "pandn " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdf @addr      => "pandn " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe0 11 ggg rrr => "pavgb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe0 @addr      => "pavgb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe1 11 ggg rrr => "psraw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe1 @addr      => "psraw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe2 11 ggg rrr => "psrad " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe2 @addr      => "psrad " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe3 11 ggg rrr => "pavgw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe3 @addr      => "pavgw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe4 11 ggg rrr => "pmulhuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe4 @addr      => "pmulhuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe5 11 ggg rrr => "pmulhw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe5 @addr      => "pmulhw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe8 11 ggg rrr => "psubsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe8 @addr      => "psubsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe9 11 ggg rrr => "psubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe9 @addr      => "psubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xea 11 ggg rrr => "pminsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xea @addr      => "pminsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xeb 11 ggg rrr => "por " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xeb @addr      => "por " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xec 11 ggg rrr => "paddsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xec @addr      => "paddsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xed 11 ggg rrr => "paddsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xed @addr      => "paddsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xee 11 ggg rrr => "pmaxsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xee @addr      => "pmaxsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xef 11 ggg rrr => "pxor " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xef @addr      => "pxor " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf1 11 ggg rrr => "psllw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf1 @addr      => "psllw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf2 11 ggg rrr => "pslld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf2 @addr      => "pslld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf3 11 ggg rrr => "psllq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf3 @addr      => "psllq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf4 11 ggg rrr => "pmuludq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf4 @addr      => "pmuludq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf5 11 ggg rrr => "pmaddwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf5 @addr      => "pmaddwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf6 11 ggg rrr => "psadbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf6 @addr      => "psadbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf8 11 ggg rrr => "psubb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf8 @addr      => "psubb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf9 11 ggg rrr => "psubw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf9 @addr      => "psubw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfa 11 ggg rrr => "psubd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfa @addr      => "psubd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfb 11 ggg rrr => "psubq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfb @addr      => "psubq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfc 11 ggg rrr => "paddb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfc @addr      => "paddb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfd 11 ggg rrr => "paddw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfd @addr      => "paddw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfe 11 ggg rrr => "paddd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfe @addr      => "paddd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6e 11 ggg rrr => mdq2[$rexw] " " simd[8*$opsiz+16*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] ;
  0x0f 0x6e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr [$reptype==1] => "movq " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7e @addr      [$reptype==1] => "movq " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr => mdq2[$rexw] " " greg[32*$rexw+8*$rexb+$r] "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0x7e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " $addr "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0x6f 11 ggg rrr [$reptype==1] => "movdqu " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x6f @addr      [$reptype==1] => "movdqu " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x6f 11 ggg rrr [$opsiz] => "movdqa " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x6f @addr      [$opsiz] => "movdqa " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x6f 11 ggg rrr => "movq " mmreg[$g] "," mmreg[$r] ;
  0x0f 0x6f @addr      => "movq " mmreg[$g] "," $addr ;
  0x0f 0x7f 11 ggg rrr [$reptype==1] => "movdqu " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x7f @addr      [$reptype==1] => "movdqu " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x7f 11 ggg rrr [$opsiz] => "movdqa " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x7f @addr      [$opsiz] => "movdqa " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x7f 11 ggg rrr => "movq " mmreg[$r] "," mmreg[$g] wit("alt") ;
  0x0f 0x7f @addr      => "movq " $addr "," mmreg[$g] ;
  0x0f 0x70 11 ggg rrr @imm8 [$opsiz] => "pshufd " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$opsiz] => "pshufd " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 [$reptype==1] => "pshufhw " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$reptype==1] => "pshufhw " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 [$reptype==2] => "pshuflw " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$reptype==2] => "pshuflw " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 => "pshufw " mmreg[$g] "," mmreg[$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 => "pshufw " mmreg[$g] "," $addr "," hex($imm8) ;
  0x0f 0x71 11 010 rrr @imm8 => "psrlw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x71 11 100 rrr @imm8 => "psraw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x71 11 110 rrr @imm8 => "psllw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 010 rrr @imm8 => "psrld " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 100 rrr @imm8 => "psrad " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 110 rrr @imm8 => "pslld " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 010 rrr @imm8 => "psrlq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 011 rrr @imm8 => "psrldq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 110 rrr @imm8 => "psllq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 111 rrr @imm8 => "pslldq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc2 11 ggg rrr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc3 @addr => "movnti " $addr "," greg[32*$rexw+8*$rexr+$g] ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " simd[8*$opsiz+16*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc4 @addr @imm8 => "pinsrw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[32*$rexw+8*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => "shuf" elt[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr @imm8 => "shuf" elt[$opsiz] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xd0 11 ggg rrr => d0t[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xd0 @addr      => d0t[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7c 11 ggg rrr => h7c[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7c @addr      => h7c[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => h7d[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7d @addr      => h7d[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0xd6 11 ggg rrr [$reptype==1] => "movq2dq " xreg[8*$rexr+$g] "," mmreg[$r] ;
  0x0f 0xd6 11 ggg rrr [$reptype==2] => "movdq2q " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xd6 11 ggg rrr => "movq " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0xd6 @addr      => "movq " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[32*$rexw+8*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe6 11 ggg rrr => me6[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xe6 @addr      => me6[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0xe7 @addr      => me7[$opsiz] " " $addr "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0xf7 11 ggg rrr => mf7[$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf0 @addr [$reptype==2] => "lddqu " xreg[8*$rexr+$g] "," $addr ;

  # ================= three-byte 0F38 / 0F3A : SSSE3 / SSE4 / AES / SHA =====
  0x0f 0x38 0x00 11 ggg rrr => "pshufb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x00 @addr      => "pshufb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x01 11 ggg rrr => "phaddw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x01 @addr      => "phaddw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x02 11 ggg rrr => "phaddd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x02 @addr      => "phaddd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x03 11 ggg rrr => "phaddsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x03 @addr      => "phaddsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x04 11 ggg rrr => "pmaddubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x04 @addr      => "pmaddubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x05 11 ggg rrr => "phsubw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x05 @addr      => "phsubw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x06 11 ggg rrr => "phsubd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x06 @addr      => "phsubd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x07 11 ggg rrr => "phsubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x07 @addr      => "phsubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x08 11 ggg rrr => "psignb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x08 @addr      => "psignb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x09 11 ggg rrr => "psignw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x09 @addr      => "psignw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x0a 11 ggg rrr => "psignd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x0a @addr      => "psignd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x0b 11 ggg rrr => "pmulhrsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x0b @addr      => "pmulhrsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x10 11 ggg rrr => "pblendvb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x10 @addr      => "pblendvb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x14 11 ggg rrr => "blendvps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x14 @addr      => "blendvps " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x15 11 ggg rrr => "blendvpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x15 @addr      => "blendvpd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x17 11 ggg rrr => "ptest " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x17 @addr      => "ptest " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1c 11 ggg rrr => "pabsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1c @addr      => "pabsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1d 11 ggg rrr => "pabsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1d @addr      => "pabsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1e 11 ggg rrr => "pabsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1e @addr      => "pabsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x20 11 ggg rrr => "pmovsxbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x20 @addr      => "pmovsxbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x21 11 ggg rrr => "pmovsxbd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x21 @addr      => "pmovsxbd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x22 11 ggg rrr => "pmovsxbq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x22 @addr      => "pmovsxbq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x23 11 ggg rrr => "pmovsxwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x23 @addr      => "pmovsxwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x24 11 ggg rrr => "pmovsxwq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x24 @addr      => "pmovsxwq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x25 11 ggg rrr => "pmovsxdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x25 @addr      => "pmovsxdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x28 11 ggg rrr => "pmuldq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x28 @addr      => "pmuldq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x29 11 ggg rrr => "pcmpeqq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x29 @addr      => "pcmpeqq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x2b 11 ggg rrr => "packusdw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x2b @addr      => "packusdw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x30 11 ggg rrr => "pmovzxbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x30 @addr      => "pmovzxbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x31 11 ggg rrr => "pmovzxbd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x31 @addr      => "pmovzxbd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x32 11 ggg rrr => "pmovzxbq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x32 @addr      => "pmovzxbq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x33 11 ggg rrr => "pmovzxwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x33 @addr      => "pmovzxwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x34 11 ggg rrr => "pmovzxwq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x34 @addr      => "pmovzxwq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x35 11 ggg rrr => "pmovzxdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x35 @addr      => "pmovzxdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x37 11 ggg rrr => "pcmpgtq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x37 @addr      => "pcmpgtq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x38 11 ggg rrr => "pminsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x38 @addr      => "pminsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x39 11 ggg rrr => "pminsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x39 @addr      => "pminsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3a 11 ggg rrr => "pminuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3a @addr      => "pminuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3b 11 ggg rrr => "pminud " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3b @addr      => "pminud " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3c 11 ggg rrr => "pmaxsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3c @addr      => "pmaxsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3d 11 ggg rrr => "pmaxsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3d @addr      => "pmaxsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3e 11 ggg rrr => "pmaxuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3e @addr      => "pmaxuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3f 11 ggg rrr => "pmaxud " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3f @addr      => "pmaxud " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x40 11 ggg rrr => "pmulld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x40 @addr      => "pmulld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x41 11 ggg rrr => "phminposuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x41 @addr      => "phminposuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x2a @addr      => "movntdqa " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdb 11 ggg rrr => "aesimc " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdb @addr      => "aesimc " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdc 11 ggg rrr => "aesenc " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdc @addr      => "aesenc " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdd 11 ggg rrr => "aesenclast " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdd @addr      => "aesenclast " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xde 11 ggg rrr => "aesdec " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xde @addr      => "aesdec " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdf 11 ggg rrr => "aesdeclast " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdf @addr      => "aesdeclast " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xc8 11 ggg rrr => "sha1nexte " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xc8 @addr      => "sha1nexte " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xc9 11 ggg rrr => "sha1msg1 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xc9 @addr      => "sha1msg1 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xca 11 ggg rrr => "sha1msg2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xca @addr      => "sha1msg2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcb 11 ggg rrr => "sha256rnds2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcb @addr      => "sha256rnds2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcc 11 ggg rrr => "sha256msg1 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcc @addr      => "sha256msg1 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcd 11 ggg rrr => "sha256msg2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcd @addr      => "sha256msg2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf0 11 ggg rrr [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0x38 0xf0 @addr      [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf1 11 ggg rrr [$reptype==2] => "crc32 " greg[32*$rexw+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf1 @addr      [$reptype==2] => "crc32" sfx[$rexw? 8 : (4>>$opsiz)] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr [$reptype==1] => "adox " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf6 11 ggg rrr [$opsiz] => "adcx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf0 11 ggg rrr => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf0 @addr      => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf1 @addr      => "movbe " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x3a 0x08 11 ggg rrr @imm8 => "roundps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x08 @addr      @imm8 => "roundps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x09 11 ggg rrr @imm8 => "roundpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x09 @addr      @imm8 => "roundpd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0a 11 ggg rrr @imm8 => "roundss " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0a @addr      @imm8 => "roundss " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0b 11 ggg rrr @imm8 => "roundsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0b @addr      @imm8 => "roundsd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0c 11 ggg rrr @imm8 => "blendps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0c @addr      @imm8 => "blendps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0d 11 ggg rrr @imm8 => "blendpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0d @addr      @imm8 => "blendpd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0e 11 ggg rrr @imm8 => "pblendw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0e @addr      @imm8 => "pblendw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0f 11 ggg rrr @imm8 => "palignr " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0f @addr      @imm8 => "palignr " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x40 11 ggg rrr @imm8 => "dpps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x40 @addr      @imm8 => "dpps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x41 11 ggg rrr @imm8 => "dppd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x41 @addr      @imm8 => "dppd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x42 11 ggg rrr @imm8 => "mpsadbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x42 @addr      @imm8 => "mpsadbw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x44 11 ggg rrr @imm8 => "pclmulqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x44 @addr      @imm8 => "pclmulqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x60 11 ggg rrr @imm8 => "pcmpestrm " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x60 @addr      @imm8 => "pcmpestrm " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x61 11 ggg rrr @imm8 => "pcmpestri " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x61 @addr      @imm8 => "pcmpestri " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x62 11 ggg rrr @imm8 => "pcmpistrm " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x62 @addr      @imm8 => "pcmpistrm " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x63 11 ggg rrr @imm8 => "pcmpistri " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x63 @addr      @imm8 => "pcmpistri " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xdf 11 ggg rrr @imm8 => "aeskeygenassist " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0xdf @addr      @imm8 => "aeskeygenassist " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb" sfx[1] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) wit("long") ;
  0x0f 0x3a 0x15 @addr      @imm8 => "pextrw" sfx[2] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => pextrdq[$rexw] " " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => pextrdq[$rexw] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => pinsrdq[$rexw] " " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => pinsrdq[$rexw] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;

  # ============================ x87 FPU (D8-DF) =========================
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
