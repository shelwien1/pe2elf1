#!/usr/bin/env python3
# ifresolve.py -- partial C preprocessor: resolves #if/#ifdef/#ifndef/#elif/#else/#endif
# by tracking #define/#undef, inlines the taken branch and deletes the rest.
#
# Resolution policy:
#   * A macro #define'd somewhere in the file is "file-managed": before its first
#     #define it counts as undefined (plain C semantics), after it its value is known.
#     Conditionals on known values are resolved and the directives removed.
#   * A macro tested but NEVER #define'd in the file (compiler builtins like __GNUC__,
#     __SSE3__, or -D command-line flags like FREEZE/STATS) is "external": its state is
#     unknown, so conditionals mentioning it are kept verbatim.  Override with -D/-U.
#   * "#keep NAME [NAME2 ...]" (new directive, removed from output): NAME becomes
#     unresolvable; every conditional mentioning it is kept verbatim, including its
#     #ifndef default block.  Position-independent (collected in a pre-pass).
#     Same via command line: --keep NAME.
#   * #define/#undef seen inside a kept (unresolved) region poisons the macro to
#     "unknown" (we don't know which branch the real cpp will take), so anything
#     downstream that depends on it is kept too -- keeps cascade correctly.
#   * Inside kept regions, nested conditionals that depend only on known macros are
#     still resolved (their outcome is branch-independent).
#   * #define/#undef lines of resolved macros are dropped from the output if the name
#     is referenced nowhere else in the result (conditions of kept groups, other macro
#     bodies, code and strings count as references; comments don't).  So the whole
#     "#ifndef X/#define X 1/#endif ... #if X" machinery evaporates, but #define NTW
#     survives because code uses NTW.
#
# Not implemented (out of scope): #include processing, macro expansion in code,
# function-like macro calls inside #if expressions (-> unknown -> kept).

import re, sys, argparse
from collections import Counter, defaultdict

ID_RE   = re.compile(r'[A-Za-z_]\w*')
DIR_RE  = re.compile(r'^\s*#\s*(\w+)((?:\s|\().*)?$', re.S)
TOK_RE  = re.compile(r'''\s+
  | [A-Za-z_]\w*
  | 0[xX][0-9a-fA-F]+[uUlL]*
  | (?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?[fFuUlL]*
  | \|\||&&|<<|>>|<=|>=|==|!=
  | [-+*/%()!~<>&|^?:,]''', re.X)

UNKNOWN = None   # tri-state: True / False / None in eval results

# ---------------------------------------------------------------- lexing helpers

def strip_comments(text, in_comment, keep_strings=False):
    """Blank out //, /*...*/ comments and (unless keep_strings) string/char bodies.
    Returns (clean, in_comment_after)."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if in_comment:
            j = text.find('*/', i)
            if j < 0:
                i = n
            else:
                i = j + 2; in_comment = False; out.append(' ')
            continue
        if c == '/' and i + 1 < n:
            if text[i+1] == '/':
                break
            if text[i+1] == '*':
                in_comment = True; i += 2; continue
        if c in '"\'':
            # C++14 digit separator 1'000'000
            if c == "'" and i > 0 and text[i-1].isdigit() and i+1 < n and text[i+1].isdigit():
                i += 1; continue
            q = c; j = i + 1
            while j < n:
                if text[j] == '\\': j += 2; continue
                if text[j] == q: j += 1; break
                j += 1
            out.append(text[i:j] if keep_strings else ' ')
            i = j
            continue
        out.append(c); i += 1
    return ''.join(out), in_comment

class Unit:
    """One logical line: physical lines joined over backslash continuations."""
    __slots__ = ('raw', 'clean', 'kw', 'rest', 'lineno')
    def __init__(self, raw, clean, kw, rest, lineno):
        self.raw, self.clean, self.kw, self.rest, self.lineno = raw, clean, kw, rest, lineno

def make_units(lines):
    units, i, n, in_comment = [], 0, len(lines), False
    while i < n:
        j = i
        while j < n - 1 and lines[j].rstrip('\r\n').endswith('\\'):
            j += 1
        raw = lines[i:j+1]
        joined = ''.join(l.rstrip('\r\n')[:-1] if k < len(raw)-1 else l.rstrip('\r\n')
                         for k, l in enumerate(raw))
        was_in_comment = in_comment
        clean, in_comment = strip_comments(joined, in_comment)
        kw = rest = None
        if not was_in_comment or clean.lstrip().startswith('#'):
            m = DIR_RE.match(clean)
            if m:
                kw = m.group(1)
                rest = (m.group(2) or '').replace('\r', ' ').strip()
        units.append(Unit(raw, clean, kw, rest, i + 1))
        i = j + 1
    return units

# ---------------------------------------------------------------- parse tree

class Group:
    __slots__ = ('branches', 'endif')      # branches: [(directive_unit, [nodes])]
    def __init__(self): self.branches, self.endif = [], None

def parse_region(units, i):
    nodes = []
    while i < len(units):
        u = units[i]
        if u.kw in ('if', 'ifdef', 'ifndef'):
            g, cur = Group(), u
            i += 1
            while True:
                body, i, term = parse_region(units, i)
                g.branches.append((cur, body))
                if term is None:
                    sys.exit(f'error: unterminated #{cur.kw} at line {cur.lineno}')
                if term.kw == 'endif':
                    g.endif = term; break
                cur = term
            nodes.append(g)
        elif u.kw in ('elif', 'else', 'endif'):
            return nodes, i + 1, u
        else:
            nodes.append(u); i += 1
    return nodes, i, None

# ---------------------------------------------------------------- expressions

def tokenize(s):
    toks, pos = [], 0
    for m in TOK_RE.finditer(s):
        if m.start() != pos: return None
        pos = m.end()
        t = m.group(0)
        if not t.isspace(): toks.append(t)
    return toks if pos == len(s) else None

def cnum(tok):
    s = tok.rstrip('uUlLfF')
    try:
        if s[:2].lower() == '0x': return int(s, 16)
        if any(ch in s for ch in '.eE'): return float(s)
        if len(s) > 1 and s[0] == '0':
            try: return int(s, 8)
            except ValueError: return int(s, 10)
        return int(s)
    except ValueError:
        return UNKNOWN

class Ctx:
    def __init__(self, kept, file_defined):
        self.macros = {}                  # name -> ('val',str)|('func',)|('undef',)|('unknown',)
        self.kept = kept
        self.file_defined = file_defined
    def state(self, name):
        if name in self.kept: return ('kept',)
        e = self.macros.get(name)
        if e: return e
        if name in self.file_defined: return ('notdef',)
        return ('external',)
    def defined_value(self, name):        # for defined(NAME) / #ifdef
        s = self.state(name)
        return {'val': 1, 'func': 1, 'undef': 0, 'notdef': 0}.get(s[0], UNKNOWN)

def expand(toks, ctx, active=frozenset()):
    """Object-macro expansion inside #if expressions.  Returns token list or UNKNOWN."""
    out, i = [], 0
    while i < len(toks):
        t = toks[i]
        if ID_RE.fullmatch(t):
            if t == 'defined':
                out.append(t); i += 1
                if i < len(toks) and toks[i] == '(':
                    if i+2 < len(toks) and ID_RE.fullmatch(toks[i+1]) and toks[i+2] == ')':
                        out += toks[i:i+3]; i += 3; continue
                    return UNKNOWN
                if i < len(toks) and ID_RE.fullmatch(toks[i]):
                    out.append(toks[i]); i += 1; continue
                return UNKNOWN
            if t in ('true', 'false'):
                out.append('1' if t == 'true' else '0'); i += 1; continue
            if t in active:
                out.append(t); i += 1; continue
            s = ctx.state(t)
            if s[0] == 'val':
                sub = tokenize(s[1])
                if sub is None: return UNKNOWN
                sub = expand(sub, ctx, active | {t})
                if sub is UNKNOWN: return UNKNOWN
                out += sub; i += 1; continue
            if s[0] in ('kept', 'unknown', 'external', 'func'):
                return UNKNOWN
            out.append(t); i += 1; continue    # notdef/undef -> evaluates as 0
        out.append(t); i += 1
    return out

def truthy(v): return v is not UNKNOWN and v != 0

class EvalError(Exception): pass

class Parser:
    # C #if expression evaluator over expanded tokens; UNKNOWN propagates (Kleene &&/||).
    BIN = [ ('||',), ('&&',), ('|',), ('^',), ('&',), ('==','!='),
            ('<','>','<=','>='), ('<<','>>'), ('+','-'), ('*','/','%') ]
    def __init__(self, toks, ctx): self.t, self.i, self.ctx = toks, 0, ctx
    def peek(self): return self.t[self.i] if self.i < len(self.t) else None
    def next(self): t = self.peek(); self.i += 1; return t
    def expect(self, x):
        if self.next() != x: raise EvalError
    def parse(self):
        v = self.cond()
        if self.i != len(self.t): raise EvalError
        return v
    def cond(self):
        c = self.binary(0)
        if self.peek() == '?':
            self.next(); a = self.cond(); self.expect(':'); b = self.cond()
            if c is UNKNOWN: return a if a == b else UNKNOWN
            return a if truthy(c) else b
        return c
    def binary(self, lvl):
        if lvl == len(self.BIN): return self.unary()
        v = self.binary(lvl + 1)
        while self.peek() in self.BIN[lvl]:
            op = self.next(); r = self.binary(lvl + 1)
            v = self.apply(op, v, r)
        return v
    def unary(self):
        t = self.peek()
        if t == '!':
            self.next(); v = self.unary()
            return UNKNOWN if v is UNKNOWN else int(not truthy(v))
        if t == '~':
            self.next(); v = self.unary()
            return UNKNOWN if v is UNKNOWN or isinstance(v, float) else ~v
        if t == '-':
            self.next(); v = self.unary()
            return UNKNOWN if v is UNKNOWN else -v
        if t == '+':
            self.next(); return self.unary()
        if t == 'defined':
            self.next()
            if self.peek() == '(':
                self.next(); n = self.next(); self.expect(')')
            else:
                n = self.next()
            if n is None or not ID_RE.fullmatch(n): raise EvalError
            return self.ctx.defined_value(n)
        return self.primary()
    def primary(self):
        t = self.next()
        if t == '(':
            v = self.cond(); self.expect(')'); return v
        if t is None: raise EvalError
        if ID_RE.fullmatch(t): return 0          # surviving identifier -> 0 (C rule)
        v = cnum(t)
        if v is UNKNOWN: raise EvalError
        return v
    def apply(self, op, a, b):
        if op == '&&':
            if a == 0 or b == 0: return 0
            if a is UNKNOWN or b is UNKNOWN: return UNKNOWN
            return 1
        if op == '||':
            if truthy(a) or truthy(b): return 1
            if a is UNKNOWN or b is UNKNOWN: return UNKNOWN
            return 0
        if a is UNKNOWN or b is UNKNOWN: return UNKNOWN
        ints = isinstance(a, int) and isinstance(b, int)
        try:
            if op == '==': return int(a == b)
            if op == '!=': return int(a != b)
            if op == '<':  return int(a < b)
            if op == '>':  return int(a > b)
            if op == '<=': return int(a <= b)
            if op == '>=': return int(a >= b)
            if op == '+':  return a + b
            if op == '-':  return a - b
            if op == '*':  return a * b
            if op == '/':
                if b == 0: return UNKNOWN
                if ints:
                    q = abs(a) // abs(b)
                    return q if (a < 0) == (b < 0) else -q
                return a / b
            if op == '%':
                if not ints or b == 0: return UNKNOWN
                r = abs(a) % abs(b)
                return r if a >= 0 else -r
            if not ints: return UNKNOWN
            if op == '<<': return a << b
            if op == '>>': return a >> b
            if op == '&':  return a & b
            if op == '|':  return a | b
            if op == '^':  return a ^ b
        except Exception:
            return UNKNOWN
        raise EvalError

def eval_cond(u, ctx, opts):
    """Returns (True/False/None, reason_string_when_None)."""
    if u.kw in ('ifdef', 'ifndef'):
        m = ID_RE.search(u.rest or '')
        if not m: return None, 'malformed'
        v = ctx.defined_value(m.group(0))
        if v is UNKNOWN:
            return None, f'{m.group(0)}: {ctx.state(m.group(0))[0]}'
        return (bool(v) if u.kw == 'ifdef' else not v), None
    toks = tokenize(u.rest or '')
    if toks is None or not toks: return None, 'lex error'
    if opts.keep_literal and not any(ID_RE.fullmatch(t) for t in toks):
        return None, 'literal (kept by --keep-literal)'
    ex = expand(toks, ctx)
    if ex is UNKNOWN:
        culprit = [t for t in toks if ID_RE.fullmatch(t) and t != 'defined'
                   and ctx.state(t)[0] in ('kept', 'unknown', 'external', 'func')]
        return None, ','.join(f'{c}: {ctx.state(c)[0]}' for c in dict.fromkeys(culprit)) or 'expansion'
    try:
        v = Parser(ex, ctx).parse()
    except EvalError:
        return None, 'eval error'
    if v is UNKNOWN:
        culprit = [t for t in toks if ID_RE.fullmatch(t) and t != 'defined'
                   and ctx.state(t)[0] in ('kept', 'unknown', 'external', 'func')]
        return None, ','.join(f'{c}: {ctx.state(c)[0]}' for c in dict.fromkeys(culprit)) or 'unknown'
    return truthy(v), None

# ---------------------------------------------------------------- walk / emit

DEFINE_RE = re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)(\()?')

def walk(nodes, ctx, out, poison, opts, log):
    for nd in nodes:
        if isinstance(nd, Group):
            sel, reason = None, None
            for idx, (u, _body) in enumerate(nd.branches):
                if u.kw == 'else':
                    sel = idx; break
                v, why = eval_cond(u, ctx, opts)
                if v is None:
                    reason = (u, why); break
                if v:
                    sel = idx; break
            if reason is not None:                       # keep whole group verbatim
                log['kept'].append((reason[0].lineno, reason[0].kw, reason[0].rest, reason[1]))
                for u, body in nd.branches:
                    out.append((u.raw, u.clean, None))
                    walk(body, ctx, out, True, opts, log)
                out.append((nd.endif.raw, nd.endif.clean, None))
            elif sel is not None:                        # resolved: inline taken branch
                log['resolved'] += 1
                walk(nd.branches[sel][1], ctx, out, poison, opts, log)
            else:
                log['resolved'] += 1                     # all false, no #else: emit nothing
            continue
        u = nd
        if u.kw == 'define':
            m = DEFINE_RE.match(u.clean)
            if m:
                name, is_func = m.group(1), bool(m.group(2))
                val = u.clean[m.end(1) + (1 if is_func else 0):]
                if is_func:                              # skip params for record purposes
                    ctx.macros[name] = ('unknown',) if poison else ('func',)
                else:
                    ctx.macros[name] = ('unknown',) if poison else ('val', val.strip())
                cand = name if (not poison and name not in ctx.kept) else None
                out.append((u.raw, u.clean, cand))
            else:
                out.append((u.raw, u.clean, None))
        elif u.kw == 'undef':
            m = ID_RE.search(u.rest or '')
            if m:
                name = m.group(0)
                ctx.macros[name] = ('unknown',) if poison else ('undef',)
                cand = name if (not poison and name not in ctx.kept) else None
                out.append((u.raw, u.clean, cand))
            else:
                out.append((u.raw, u.clean, None))
        elif u.kw == 'keep':
            pass                                         # directive consumed, not emitted
        else:
            out.append((u.raw, u.clean, None))

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)

def scan_includes(out, incdirs, log):
    """Identifier usage inside quoted includes (recursive).  Returns (Counter, all_found)."""
    import os
    total, seen, ok = Counter(), set(), True
    def scan(fname, frm):
        for d in incdirs:
            p = os.path.join(d, fname)
            if os.path.isfile(p):
                rp = os.path.realpath(p)
                if rp in seen: return
                seen.add(rp)
                with open(p, 'rb') as f:
                    txt = f.read().decode('latin-1')
                clean, _ = strip_comments(txt.replace('\\\r\n', ' ').replace('\\\n', ' '),
                                          False, keep_strings=True)
                total.update(ID_RE.findall(clean))
                for sub in INCLUDE_RE.findall(clean):
                    scan(sub, p)
                return
        nonlocal ok
        ok = False
        log['missing'].append((fname, frm))
    for raw, clean, _c in out:
        if re.match(r'\s*#\s*include\b', clean):       # directive check on comment-free text
            m = INCLUDE_RE.match(''.join(raw))         # filename from raw (strings blanked in clean)
            if m: scan(m.group(1), '<input>')
    return total, ok

def drop_unused_defines(out, ext_usage, log):
    """Delete #define/#undef units of resolved macros whose name appears nowhere else."""
    idents = [Counter(ID_RE.findall(clean)) for _raw, clean, _c in out]
    total = Counter(ext_usage)
    for c in idents: total.update(c)
    by_name = defaultdict(list)
    for k, (_raw, _clean, cand) in enumerate(out):
        if cand: by_name[cand].append(k)
    dead = set()
    changed = True
    while changed:
        changed = False
        for name, ks in list(by_name.items()):
            own = sum(idents[k][name] for k in ks)
            if total[name] - own == 0:
                for k in ks:
                    total.subtract(idents[k]); dead.add(k)
                    log['dropped'].append((out[k][0][0].rstrip('\r\n'),))
                del by_name[name]
                changed = True
    return [o for k, o in enumerate(out) if k not in dead]

# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description='Resolve C preprocessor conditionals in place; '
                                             '"#keep NAME" (or --keep NAME) protects a macro from resolution.')
    ap.add_argument('input')
    ap.add_argument('-o', '--output', help='output file (default: stdout)')
    ap.add_argument('-D', action='append', default=[], metavar='NAME[=VAL]',
                    help='assume macro defined (like cc -D)')
    ap.add_argument('-U', action='append', default=[], metavar='NAME',
                    help='assume macro undefined, even compiler builtins')
    ap.add_argument('-k', '--keep', action='append', default=[], metavar='NAME',
                    help='same as "#keep NAME" in the source')
    ap.add_argument('-I', action='append', default=[], metavar='DIR',
                    help='include search dir (quoted includes are scanned for macro usage)')
    ap.add_argument('--force-drop', action='store_true',
                    help='drop unused #define lines even if some quoted include was not found')
    ap.add_argument('--keep-literal', action='store_true',
                    help='do not resolve literal-only conditions like "#if 0"')
    ap.add_argument('--keep-defines', action='store_true',
                    help='never drop #define/#undef lines')
    ap.add_argument('-q', '--quiet', action='store_true')
    opts = ap.parse_args()

    with open(opts.input, 'rb') as f:
        text = f.read().decode('latin-1')       # byte-transparent
    lines = text.splitlines(keepends=True)
    units = make_units(lines)

    # pre-pass: #keep names and set of names #define'd anywhere in the file
    kept, file_defined = set(opts.keep), set()
    for u in units:
        if u.kw == 'keep' and u.rest:
            kept.update(ID_RE.findall(u.rest))
        elif u.kw == 'define':
            m = DEFINE_RE.match(u.clean)
            if m: file_defined.add(m.group(1))

    ctx = Ctx(kept, file_defined)
    for d in opts.D:
        name, _, val = d.partition('=')
        ctx.macros[name] = ('val', val if val else '1')
    for name in opts.U:
        ctx.macros[name] = ('undef',)

    nodes, _i, term = parse_region(units, 0)
    if term is not None:
        sys.exit(f'error: stray #{term.kw} at line {term.lineno}')

    log = {'resolved': 0, 'kept': [], 'dropped': [], 'missing': []}
    out = []
    walk(nodes, ctx, out, False, opts, log)
    if not opts.keep_defines:
        import os
        incdirs = [os.path.dirname(os.path.abspath(opts.input))] + opts.I
        ext_usage, ok = scan_includes(out, incdirs, log)
        if ok or opts.force_drop:
            out = drop_unused_defines(out, ext_usage, log)

    data = ''.join(l for raw, _c, _cand in out for l in raw).encode('latin-1')
    if opts.output:
        with open(opts.output, 'wb') as f: f.write(data)
    else:
        sys.stdout.buffer.write(data)

    if not opts.quiet:
        e = sys.stderr
        print(f'-- resolved {log["resolved"]} conditional group(s), '
              f'kept {len(log["kept"])}, dropped {len(log["dropped"])} define/undef line(s)', file=e)
        for ln, kw, rest, why in log['kept']:
            print(f'   kept   line {ln}: #{kw} {rest or ""}   [{why}]', file=e)
        for (txt,) in log['dropped']:
            print(f'   drop   {txt.strip()}', file=e)
        for fname, frm in log['missing']:
            print(f'   warn   include "{fname}" not found (from {frm})'
                  + ('' if opts.force_drop else ' -- #define dropping disabled; pass -I or --force-drop'),
                  file=e)

if __name__ == '__main__':
    main()
