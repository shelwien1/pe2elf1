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
import parsergen as pg

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


#  __ASM_LIB_END__   (everything above is embedded verbatim by --emit)
def emit_standalone(desc_path):
  # Generate a self-contained assembler: the parsergen engine + this reverse
  # engine + the embedded description + a runner.  No external files at run time.
  eng = open(pg.__file__).read().split("#  __LIBRARY_END__", 1)[0].rstrip()
  me = open(__file__).read().split("#  __ASM_LIB_END__", 1)[0]
  # drop this module's own engine import; the engine is embedded inline and we
  # alias `pg` to this module so every pg.* reference resolves to it.
  me = me.replace("import parsergen as pg\n", "")
  desc = open(desc_path).read()
  out = []
  out.append("#!/usr/bin/env python3")
  out.append("# Auto-generated standalone assembler (asm.py --emit).")
  out.append("# Embeds the engine + reverse engine + description; run: x86a.py IN.asm > out.bin")
  out.append(eng)
  out.append("")
  out.append("import sys as _sys")
  out.append("pg = _sys.modules[__name__]   # pg.* resolves to the embedded engine")
  out.append("")
  out.append(me.rstrip())
  out.append("")
  out.append("DESCRIPTION = r'''")
  out.append(desc.replace("'''", "'' '"))
  out.append("'''")
  out.append("")
  out.append("def main(argv):")
  out.append("  import sys")
  out.append("  g = Parser(DESCRIPTION).parse()")
  out.append("  asm = Asm(g)")
  out.append("  if not argv:")
  out.append("    sys.stderr.write('usage: x86a.py IN.asm > out.bin\\n'); return 2")
  out.append("  out = bytearray(); fails = 0")
  out.append("  for ln in open(argv[0]):")
  out.append("    line = ln.rstrip('\\n')")
  out.append("    if not line.strip() or line.lstrip().startswith(';'):")
  out.append("      continue")
  out.append("    try:")
  out.append("      out += asm_line(asm, line, len(out))")
  out.append("    except NoAsm as exc:")
  out.append("      fails += 1")
  out.append("      sys.stderr.write('FAIL @%d: %s  (%s)\\n' % (len(out), line, exc))")
  out.append("  sys.stdout.buffer.write(bytes(out))")
  out.append("  return 1 if fails else 0")
  out.append("")
  out.append("if __name__ == '__main__':")
  out.append("  import sys; sys.exit(main(sys.argv[1:]))")
  out.append("")
  return "\n".join(out)


def main(argv):
  if len(argv) >= 2 and argv[0] == "--emit":
    sys.stdout.write(emit_standalone(argv[1]))
    return 0
  if len(argv) < 2:
    sys.stderr.write("usage: asm.py DESC.p input.asm > out.bin\n"
                     "       asm.py --emit DESC.p   > x86a.py\n")
    return 2
  g = pg.Parser(open(argv[0]).read()).parse()
  asm = Asm(g)
  out = bytearray()
  fails = 0
  for ln in open(argv[1]):
    line = ln.rstrip("\n")
    if not line.strip() or line.lstrip().startswith(";"):
      continue
    try:
      out += asm_line(asm, line, len(out))
    except NoAsm as exc:
      fails += 1
      sys.stderr.write("FAIL @%d: %s  (%s)\n" % (len(out), line, exc))
  sys.stdout.buffer.write(bytes(out))
  if fails:
    sys.stderr.write("%d line(s) failed\n" % fails)
  return 1 if fails else 0


if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
