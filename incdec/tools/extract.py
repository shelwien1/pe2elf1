#!/usr/bin/env python3
"""extract.py <sub_XXXXXXXX> [...]: write inc/<name>.inc for each function.

The vbn counterpart of incdec.md §8.5's extract_fn.py, and of the BMF/i386
extractor in shelwien1/pe2elf (incdec/bmf/extract.py), whose transforms it
reuses.  What is specific to vbn:

  * The definition gets the function's *real* convention from conv.txt, not
    the one Hex-Rays printed: register parameters that regflow.py shows are never
    read are dropped (and re-declared as zero-initialised locals if the body
    still names them), and `ret N` picks stdcall over cdecl.  The same positions
    are dropped from every call site, in this body and in its callers' bodies.
  * Every global resolves through vbn.txt (tools/mksyms.py) to its address and
    is aliased per function (`__sub_X_name`), typed from this body's own
    `using guessed type` comment first, the data declaration second.
  * Callees not yet moved are bound to their PE entry points with their real
    prototype, pointer parameters relaxed to `void *`.  Moved callees are
    reached through a `void *`-taking forwarder (Hex-Rays spells the same
    pointer differently at the call site and in the definition).
  * A function with a hand-written replacement in override/ is copied as is.

fixups.txt: `name<TAB>find<TAB>replace`, literal, `\\n` and `\\t` escaped; each
`find` must occur exactly once in that body (prefix the name with `*` to
replace every occurrence).
"""
import os, re, sys
from common import ROOT, CODE_LO, donor_lines, load_funcs, load_symbols, split_params, literal_spans

SRC = donor_lines()
FUNCS = load_funcs()
BY_NAME = {f['name']: f for f in FUNCS}
BY_VA = {f['addr']: f for f in FUNCS}
SYMS = load_symbols()


def load_conv():
    conv = {}
    for l in open(os.path.join(ROOT, 'conv.txt')):
        if l.startswith('#') or not l.strip():
            continue
        va, name, c, pop, drop, proto, note = l.rstrip('\n').split('\t')
        ret, params = proto.split('|', 1)
        conv[name] = dict(va=int(va, 16), conv=c, pop=int(pop),
                          drop=[] if drop == '-' else [int(x) for x in drop.split(',')],
                          ret=ret, params=[p for p in split_params(params)] if params.strip() else [])
    return conv


CONV = load_conv()


def load_accepted():
    p = os.path.join(ROOT, 'accepted.txt')
    if not os.path.exists(p):
        return []
    return [l.split()[0] for l in open(p) if l.strip() and not l.startswith('#')]


def load_fixups():
    fx = {}
    p = os.path.join(ROOT, 'fixups.txt')
    if os.path.exists(p):
        for l in open(p):
            if l.startswith('#') or not l.strip():
                continue
            name, find, repl = l.rstrip('\n').split('\t')
            unesc = lambda s: s.replace('\\n', '\n').replace('\\t', '\t')
            every = name.startswith('*')
            fx.setdefault(name.lstrip('*'), []).append((unesc(find), unesc(repl), every))
    return fx


# --- transforms borrowed from the BMF extractor (see its docstrings) ---------

def fix_hex_escapes(line):
    """Terminate a `\\xHH` escape followed by another hex digit (C's \\x is greedy)."""
    lit = literal_spans(line)
    if not lit:
        return line
    out, pos = [], 0
    for m in re.finditer(r'\\x[0-9A-Fa-f]{2}(?=[0-9A-Fa-f])', line):
        if any(a <= m.start() < b for a, b in lit):
            out.append(line[pos:m.end()]); out.append('""'); pos = m.end()
    out.append(line[pos:])
    return ''.join(out)


def sign_negative_divisors(line):
    """`n / 0xFFFFFFFC` -> `n / -4`: an unsigned divisor makes the division unsigned."""
    def repl(m):
        v = int(m.group(2), 16)
        return '%s %d' % (m.group(1), v - (1 << 32)) if v >= 0x80000000 else m.group(0)
    return re.sub(r'([/%])\s*(0[xX][0-9A-Fa-f]{8})\b', repl, line)


def mask_shift_counts(line):
    """`x >> n` -> `x >> (n & 31)` for every non-literal count: x86 masks the
    count to five bits, C++ leaves a count outside 0..31 undefined."""
    lit = literal_spans(line)
    cut = len(line)
    for m in re.finditer(r'//', line):
        if not any(a <= m.start() < b for a, b in lit):
            cut = m.start()
            break
    out, i, n = [], 0, len(line)
    pat = re.compile(r'(<<|>>)=?')
    while True:
        m = pat.search(line, i)
        if not m or m.start() >= cut:
            out.append(line[i:])
            return ''.join(out)
        if any(a <= m.start() < b for a, b in lit):
            out.append(line[i:m.end()]); i = m.end(); continue
        j = m.end()
        while j < n and line[j] == ' ':
            j += 1
        start, depth = j, 0
        while j < cut:
            c = line[j]
            if c in '([':
                depth += 1
            elif c in ')]':
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and c in ';,':
                break                            # always ends the count (`<< n4_0--;`)
            elif depth == 0 and c in '<>=!&^|?:':
                if c == ':' and (line[j:j + 2] == '::' or (j and line[j - 1] == ':')):
                    j += 1
                    continue
                k = j - 1
                while k >= start and line[k] == ' ':
                    k -= 1
                if k < start or not (line[k].isalnum() or line[k] in '_)]'):
                    j += 1
                    continue
                break
            j += 1
        term = line[start:j].rstrip()
        if not term or re.fullmatch(r'(?:0[xX][0-9A-Fa-f]+|\d+)[uUlL]*', term):
            out.append(line[i:start + len(term)]); i = start + len(term); continue
        out.append(line[i:m.end()]); out.append(' (' + term + ' & 31)'); i = start + len(term)


BYTE_CAST = re.compile(r'^\(\s*(?:_BYTE|unsigned __int8)\s*\)')


BYTE8 = {'char': True, '__int8': True, 'signed char': True, 'int8': True,
         '_BYTE': False, 'unsigned __int8': False, 'unsigned char': False, 'uint8': False, 'BYTE': False}


def operand_type(expr, types):
    """The C type of a comparison operand, when it is simple enough to know:
    a cast, `*p`, `*p++`, `p[i]`, or a variable -- from this body's declarations
    and the global types.  None otherwise."""
    e = expr.strip()
    while e.startswith('(') and e.endswith(')') and balanced(e[1:-1]):
        e = e[1:-1].strip()
    m = re.match(r'^\(\s*([A-Za-z_][\w\s]*?)\s*\)\s*(?![\s\-+*/%&|^<>=])', e)
    if m and not re.match(r'^\(\s*[A-Za-z_][\w\s]*\*', e):
        return m.group(1).strip()
    m = re.match(r'^\*\s*\(\s*([A-Za-z_][\w\s]*?)\s*\*\s*\)', e)          # *(_BYTE *)p
    if m:
        return m.group(1).strip()
    m = re.match(r'^\*\s*(?:\+\+|--)?\s*([A-Za-z_]\w*)\s*(?:\+\+|--)?$', e)
    if m and m.group(1) in types and types[m.group(1)].rstrip().endswith('*'):
        return types[m.group(1)].rstrip()[:-1].strip()
    m = re.match(r'^([A-Za-z_]\w*)\s*\[[^\]]*\]$', e)
    if m and m.group(1) in types:
        t = types[m.group(1)].strip()
        return t[:-1].strip() if t.endswith('*') else re.sub(r'\[.*$', '', t).strip()
    m = re.match(r'^([A-Za-z_]\w*)$', e)
    if m and m.group(1) in types and '[' not in types[m.group(1)] and not types[m.group(1)].endswith('*'):
        return types[m.group(1)].strip()
    return None


def balanced(t):
    d = 0
    for c in t:
        d += c in '(['
        d -= c in ')]'
        if d < 0:
            return False
    return d == 0


def byte_compares(line, types=None):
    """`(_BYTE)a == b` -> `(_BYTE)a == (_BYTE)(b)`, and the mirror image.

    Hex-Rays prints an 8-bit `cmp al, byte [mem]` as `(_BYTE)v7 == buf_3[i]`
    and means an 8-bit comparison.  C++ promotes both sides to int first, and
    with buf_3 declared `char` (signed) the two disagree for every byte above
    0x7F -- text compresses correctly, binary data does not.  Narrowing the
    other operand is exactly what the instruction does.
    """
    lit = literal_spans(line)
    code_end = len(line)
    for m in re.finditer(r'//', line):
        if not any(a <= m.start() < b for a, b in lit):
            code_end = m.start()
            break
    stops_left = ('&&', '||', '?', ':', ',', '(', '=', 'return', ';', '{')
    out, pos = line, 0
    for m in list(re.finditer(r'(?<![=!<>])(==|!=)(?!=)', line[:code_end]))[::-1]:
        if any(a <= m.start() < b for a, b in lit):
            continue
        # left operand
        i, depth = m.start() - 1, 0
        while i >= 0:
            c = out[i]
            if c in ')]':
                depth += 1
            elif c in '([':
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and (out[i:i + 2] in ('&&', '||') or c in '?:,;{' or
                                 (c == '=' and out[i - 1:i] not in '=!<>' and out[i + 1:i + 2] != '=')):
                break
            i -= 1
        ls, le = i + 1, m.start()
        # right operand
        j, depth = m.end(), 0
        while j < code_end:
            c = out[j]
            if c in '([':
                depth += 1
            elif c in ')]':
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and (out[j:j + 2] in ('&&', '||') or c in '?:,;'):
                break
            j += 1
        rs, re_ = m.end(), j
        left, right = out[ls:le].strip(), out[rs:re_].strip()
        if not left or not right:
            continue
        lb, rb = bool(BYTE_CAST.match(left)), bool(BYTE_CAST.match(right))
        if lb == rb:
            # Neither (or both) cast: compare 8-bit operands of different
            # signedness (`*v54 == *v53`, char * against _BYTE *) as bytes too.
            if types is None or lb:
                continue
            lt, rt = operand_type(left, types), operand_type(right, types)
            if lt in BYTE8 and rt in BYTE8 and BYTE8[lt] != BYTE8[rt]:
                out = out[:rs] + ' (_BYTE)(' + right + ')' + out[re_:]
                out = out[:ls] + ('' if out[ls - 1:ls] in ' (' else ' ') + '(_BYTE)(' + left + ') ' + out[le:]
            continue
        if lb:
            out = out[:rs] + ' (_BYTE)(' + right + ')' + out[re_:]
        else:
            out = out[:ls] + ('' if out[ls - 1:ls] in ' (' else ' ') + '(_BYTE)(' + left + ') ' + out[le:]
    return out


PTR_LOCAL = re.compile(r'^\s*([A-Za-z_][\w\s]*?\*)\s*(\w+)\s*;')


def launder_pointer_counters(body):
    """A pointer-typed register used as a loop counter: step it through
    uintptr_t, or g++ folds `while (p)` after `--p` to `while (1)`."""
    text = '\n'.join(body)
    ptr = {}
    for l in body:
        m = PTR_LOCAL.match(l)
        if m:
            ptr[m.group(2)] = m.group(1).strip()
    out = list(body)
    for v, ty in sorted(ptr.items()):
        e = re.escape(v)
        if not re.search(r'(?:while|if)\s*\(\s*!?\s*' + e + r'\s*\)', text):
            continue
        step = type_size(ty[:-1].strip())
        scale = '' if step == 1 else f' * {step}'
        subs = [
            (re.compile(r'^(\s*)(?:--\s*' + e + r'|' + e + r'\s*--)\s*;\s*$'),
             lambda m: f"{m.group(1)}{v} = ({ty})((uintptr_t){v} - {step});"),
            (re.compile(r'^(\s*)(?:\+\+\s*' + e + r'|' + e + r'\s*\+\+)\s*;\s*$'),
             lambda m: f"{m.group(1)}{v} = ({ty})((uintptr_t){v} + {step});"),
            (re.compile(r'^(\s*)' + e + r'\s*([-+])=\s*(.+);\s*$'),
             lambda m: f"{m.group(1)}{v} = ({ty})((uintptr_t){v} {m.group(2)} ({m.group(3)}){scale});"),
        ]
        for i, l in enumerate(out):
            for pat, rep in subs:
                m = pat.match(l)
                if m:
                    out[i] = rep(m)
                    break
    return out


BASE_SIZE = [('_QWORD', 8), ('__int64', 8), ('double', 8), ('long long', 8), ('_DWORD', 4),
             ('__int32', 4), ('float', 4), ('int', 4), ('long', 4), ('_WORD', 2), ('__int16', 2),
             ('short', 2), ('wchar_t', 2), ('_BYTE', 1), ('__int8', 1), ('char', 1), ('bool', 1),
             ('_UNKNOWN', 1)]


def type_size(ty):
    if '*' in ty:
        return 4
    for pat, sz in BASE_SIZE:
        if re.search(r'\b' + re.escape(pat) + r'\b', ty):
            return sz
    return 4


# --- stack frame reassembly ---------------------------------------------------
# Hex-Rays splits one stack array into neighbouring scalars and arrays
# (`char dst[3]; char i[125];` for a 128-byte buffer handed to getcwd).  When
# any stack local's address is taken, lay them all out at their recorded ebp
# offsets in one buffer and declare each as a reference into it.
FRAME_DECL = re.compile(
    r'^\s*(?P<type>[A-Za-z_][\w\s\*]*?)\s*\b(?P<name>[A-Za-z_]\w*)\s*(?P<ext>\[[^\]]*\])?\s*;'
    r'\s*//\s*\[esp[-+][0-9A-Fa-f]+h\]\s*\[ebp(?P<sign>[-+])(?P<off>[0-9A-Fa-f]+)h\](?P<rest>.*)$')
LOCAL_DECL = re.compile(r'^\s*(?P<type>[A-Za-z_][\w\s\*]*?)\s*\b(?P<name>[A-Za-z_]\w*)\s*'
                        r'(?P<ext>\[[^\]]*\])?\s*;\s*(?://.*)?$')


def frame_locals(body, start):
    out = []
    for i in range(start, len(body)):
        t = body[i].rstrip()
        if not t.strip():
            continue
        m = FRAME_DECL.match(t)
        if m:
            off = int(m.group('off'), 16) * (1 if m.group('sign') == '+' else -1)
            out.append((i, off, m.group('type').strip(), m.group('name'), m.group('ext') or '',
                        'BYREF' in m.group('rest')))
        elif LOCAL_DECL.match(t):
            continue
        else:
            break
    return out


def addr_taken(body, locs):
    if not locs:
        return False
    if any(by for *_, by in locs):
        return True
    start = max(i for i, *_ in locs) + 1
    text = '\n'.join(body[start:])
    for _i, _o, _t, nm, _e, _b in locs:
        for m in re.finditer(r'&\s*' + re.escape(nm) + r'\b', text):
            before = text[:m.start()].rstrip()
            if before and (before[-1].isalnum() or before[-1] in '_)]&'):
                continue
            return True
    return False


def build_frame(locs):
    lo = min(o for _, o, *_ in locs)
    hi = 0
    for _, o, ty, nm, ext, _by in locs:
        n = 1
        if ext:
            m = re.match(r'\[\s*([0-9]+)\s*\]', ext)
            n = int(m.group(1)) if m else 1
        hi = max(hi, o + n * type_size(ty))
    out = [f"  alignas(16) unsigned char __frame[{hi - lo + 16}];   // the original frame, ebp{lo:+#x}..ebp{hi:+#x}"]
    for _, o, ty, nm, ext, _by in locs:
        at = f"__frame + {o - lo}"
        if ext:
            out.append(f"  {ty} (&{nm}){ext} = *({ty} (*){ext})({at});")
        else:
            out.append(f"  {ty} &{nm} = *({ty} *)({at});")
    return out, {i for i, *_ in locs}


# --- call sites ----------------------------------------------------------------

def rewrite_calls(text, name, keep_fn):
    """Rewrite every call `name(a, b, ...)` as `name(<keep_fn(args)>)`."""
    out, pos = [], 0
    lit = literal_spans(text)
    for c in re.finditer(r'//[^\n]*', text):          # comments, unless inside a literal
        if not any(a <= c.start() < b for a, b in lit):
            lit.append((c.start(), c.end()))
    for m in re.finditer(r'(?<![\w.>])' + re.escape(name) + r'\s*\(', text):
        if m.start() < pos or any(a <= m.start() < b for a, b in lit):
            continue
        i, depth, start, args = m.end(), 1, m.end(), []
        while i < len(text) and depth:
            ch = text[i]
            if ch == '"' or ch == "'":
                q = ch; i += 1
                while i < len(text) and text[i] != q:
                    i += 2 if text[i] == '\\' else 1
            elif ch in '([':
                depth += 1
            elif ch in ')]':
                depth -= 1
                if not depth:
                    args.append(text[start:i])
                    break
            elif ch == ',' and depth == 1:
                args.append(text[start:i]); start = i + 1
            i += 1
        if depth:
            continue
        args = [a.strip() for a in args if a.strip()] if len(args) > 1 or args[0].strip() else []
        new = keep_fn(args)
        if new is None:
            continue
        out.append(text[pos:m.end()]); out.append(', '.join(new)); pos = i
    out.append(text[pos:])
    return ''.join(out)


def drop_args(text, callee, drop, nparams):
    def keep(args):
        if len(args) != nparams:
            sys.exit(f'call to {callee} has {len(args)} arguments, its Hex-Rays signature {nparams}: '
                     f'{args}; add a fixup')
        return [a for k, a in enumerate(args) if k not in drop]
    return rewrite_calls(text, callee, keep)


def hexrays_params(f):
    sig = f['sig']
    m = re.search(r'\b' + re.escape(f['name']) + r'\s*(?:@<(\w+)>)?\s*\(', sig)
    inner = sig[m.end():sig.rindex(')')]
    if not inner.strip() or inner.strip() == 'void':
        return []
    out = []
    for p in split_params(inner):
        rm = re.search(r'@<(\w+)>', p)
        out.append((re.sub(r'@<\w+>', '', p).strip(), rm.group(1) if rm else None))
    return out


def split_decl(decl):
    m = re.match(r'^(.*?[\s\*])([A-Za-z_]\w*)\s*(\[[^\]]*\])?$', decl.strip())
    if m:
        return m.group(1).strip(), m.group(2), m.group(3) or ''
    return decl.strip(), None, ''


ATTR = {'cdecl': '', 'stdcall': '__attribute__((stdcall)) ', 'regs': '', 'asm': '__attribute__((stdcall)) '}


def relax(ty):
    """Pointer parameters of a callee are declared void *: Hex-Rays spells the
    same pointer differently at a call site and in the callee."""
    if '*' in ty and '(' not in ty:
        return 'void *'
    return ty


def forwarder(name, callee, cc):
    """A moved callee is called through an inline forwarder whose pointer
    parameters are void *, for the reason relax() gives."""
    args, casts = [], []
    for k, p in enumerate(cc['params']):
        if p.strip() == '...':
            sys.exit(f'{name}: forwarder for variadic {callee}')
        t, n, ext = split_decl(p)
        t = t + ext if n else p
        a = f'a{k}'
        if '*' in t and '(' not in t:
            args.append(f'void *{a}'); casts.append(f'({t}){a}')
        else:
            args.append(f'{t} {a}'); casts.append(a)
    ret = cc['ret']
    call = f'__{callee}({", ".join(casts)})'
    return (f"static inline {ret} __fwd_{name}_{callee}({', '.join(args) or 'void'}) "
            f"{{ {'' if ret == 'void' else 'return '}{call}; }}")


def callee_decl(callee, c, alias):
    ret = c['ret']
    params = []
    for k, p in enumerate(c['params']):
        if p.strip() == '...':
            params.append('...')
            continue
        t, n, ext = split_decl(p)
        params.append(relax(t + ext if n else p))
    return (f"typedef {ATTR[c['conv']]}{ret} t_{callee}({', '.join(params) or 'void'});\n"
            f"static t_{callee} &{alias} = *(t_{callee} *)0x{c['va']:08X};")


def globals_type(name, body_comments):
    """(base type, dims) for a global, per incdec.md §6.1: this body's own guessed
    type first, then the data declaration."""
    for l in body_comments:
        m = re.match(r'// ([0-9A-F]+): using guessed type (.*?)\b' + re.escape(name) + r'(\[[^\]]*\])?;$', l)
        if m:
            return m.group(2).strip(), m.group(3) or ''
    va, kind, size, ty = SYMS.get(name) or (0, 'D', None, {'dword': 'int', 'word': '__int16', 'byte': 'char', 'unk': 'char', 'off': 'void *', 'asc': 'char', 'stru': 'char', 'qword': '__int64'}[name.split('_')[0]])
    ty = re.sub(r'\s*\[(name|guessed|string|disasm|ordered)\]$', '', ty)
    m = re.match(r'^(.*?)((?:\[[^\]]*\])*)$', ty)
    return m.group(1).strip(), m.group(2)


def sym_extent(name, esz):
    """Elements up to the next symbol: the extent of an array Hex-Rays left open."""
    va = SYMS[name][0]
    nxt = min((v for v, k, *_ in SYMS.values() if k == 'D' and v > va), default=0x14000000)
    return max(1, (nxt - va) // max(1, esz))


IDENT_OK = re.compile(r'^[A-Za-z_]\w*$')


def code_spans(line):
    """(start, end) of the parts of one line that are code: outside string and
    character literals, and before a // comment."""
    spans, pos = [], 0
    for a, b in literal_spans(line) + [(len(line), len(line))]:
        if a < pos:
            continue
        c = line.find('//', pos, a)
        if c >= 0:
            spans.append((pos, c))
            break
        spans.append((pos, a))
        pos = b
    return spans


def sub_code(pat, repl, line):
    """pat.sub(repl, ...) applied to the code of one line only."""
    out, pos = [], 0
    for a, b in code_spans(line):
        out += [line[pos:a], pat.sub(repl, line[a:b])]
        pos = b
    return ''.join(out) + line[pos:]


def sanitize_names(lines, names):
    """Hex-Rays names some variables after strings (`_Data_size__%_10u_n`, and
    `.` for a pointer into "."): rename them where they are code, as whole
    tokens (`.` is also member access, `...` a parameter list)."""
    for bad in sorted(names, key=len, reverse=True):
        text = '\n'.join(lines)
        good = re.sub(r'\W', '_', bad)
        if good[0].isdigit():
            good = '_' + good
        while re.search(r'\b' + re.escape(good) + r'\b', text):
            good += '_'
        pat = re.compile(r'(?<![\w.])' + re.escape(bad) + r'(?![\w.])')
        lines = [sub_code(pat, good, l) for l in lines]
    return lines


def emit(name, accepted):
    f = BY_NAME[name]
    c = CONV[name]
    va = f['addr']
    ovr = os.path.join(ROOT, 'override', name + '.inc')
    if os.path.exists(ovr):
        return open(ovr).read()

    body = SRC[f['start'] - 1:f['end']]
    # The trailing `// ADDR: using guessed type ...` block belongs to this body.
    tail, k = [], f['end']
    while k < len(SRC) and not SRC[k].startswith('//----- ('):
        if SRC[k].startswith('// '):
            tail.append(SRC[k])
        k += 1
    for bad in ('JUMPOUT', '__asm', '#error', 'MEMORY['):
        if any(bad in l for l in body):
            sys.exit(f'{name}: body contains {bad}; write override/{name}.inc')

    # signature: split off the parameter list, whatever its line count
    sig_lines = 0
    acc = ''
    for l in body:
        acc += l
        sig_lines += 1
        if acc.count('(') and acc.count('(') == acc.count(')'):
            break
    rest = body[sig_lines:]
    assert rest and rest[0] == '{', name
    rest = rest[1:-1]                          # without the braces

    hparams = hexrays_params(f)
    bad_names = set()
    for p, reg in hparams:
        t, n, ext = split_decl(p)
        if n is None:
            mm = re.search(r'([^\s\*&]+)$', p)
            if mm and not IDENT_OK.match(mm.group(1)):
                bad_names.add(mm.group(1))
    for l in rest:
        m = re.match(r'^\s*[A-Za-z_][\w\s\*]*?[\s\*]([^\s\*;\[][^\s;\[]*?)(\[[^\]]*\])?;\s*//', l)
        if m and not IDENT_OK.match(m.group(1)):
            bad_names.add(m.group(1))
    sig_text = ' '.join(body[:sig_lines])
    if bad_names:
        fixed = sanitize_names([sig_text] + rest, bad_names)
        sig_text, rest = fixed[0], fixed[1:]
        f = dict(f, sig=' '.join(sig_text.split()))
        hparams = hexrays_params(f)

    # fixups (before anything else rewrites the text)
    text = '\n'.join(rest)
    for find, repl, every in load_fixups().get(name, []):
        n = text.count(find)
        if n == 0 or (n > 1 and not every):
            sys.exit(f'{name}: fixup {find!r} matches {n} times')
        text = text.replace(find, repl)
    rest = text.split('\n')

    # real parameters, and the register artefacts that become locals
    keep, regvars = [], []
    for k, (p, reg) in enumerate(hparams):
        if k in c['drop']:
            regvars.append((p, reg))
        else:
            keep.append(p)
    # Hex-Rays guesses `const` from reads it saw, and bodies write through such
    # pointers (`a1[v3] = 0`); dropping it never changes what valid code means.
    params = [re.sub(r'\bconst\s+', '', re.sub(r'\bthis\b', '_this', p)) for p in keep]

    # locals: frame reassembly
    locs = frame_locals(rest, 0)
    frame_lines, drop_idx = [], set()
    if addr_taken(rest, locs):
        frame_lines, drop_idx = build_frame(locs)
    decl_end = 0
    for i, l in enumerate(rest):
        if not l.strip():
            continue
        if FRAME_DECL.match(l) or LOCAL_DECL.match(l):
            decl_end = i + 1
            continue
        break
    decls = [l for i, l in enumerate(rest[:decl_end]) if i not in drop_idx]
    stmts = rest[decl_end:]
    local_names = set()
    for l in rest[:decl_end]:
        m = FRAME_DECL.match(l) or LOCAL_DECL.match(l)
        if m:
            local_names.add(m.group('name'))
    for p in keep:
        local_names.add(split_decl(p)[1])
    reg_lines = []
    stmt_text = '\n'.join(stmts)
    for p, reg in regvars:
        t, n, ext = split_decl(p)
        if n and re.search(r'\b' + re.escape(n) + r'\b', stmt_text):
            reg_lines.append(f"  {t} {n}{ext} = 0;   // @<{reg}> in Hex-Rays; regflow shows no caller sets it")
            local_names.add(n)

    # Hex-Rays previews the string behind every global it names; for the help
    # text that is a 2 KB comment.  Keep a readable prefix.
    stmts = [re.sub(r'(//\s*".{60}).{20,}$', r'\1..."', l) for l in stmts]

    # line transforms
    stmts = launder_pointer_counters(stmts)
    stmts = [mask_shift_counts(sign_negative_divisors(fix_hex_escapes(l))) for l in stmts]
    decls = [re.sub(r'^(\s*)const\s+', r'\1', l) for l in decls]
    frame_lines = [re.sub(r'\bconst\s+', '', l) for l in frame_lines]
    reg_lines = [re.sub(r'\bconst\s+', '', l) for l in reg_lines]
    full = '\n'.join(decls + stmts)
    full = re.sub(r'\bthis\b', '_this', full)

    # call sites: drop register artefacts
    for callee, cc in CONV.items():
        if callee not in BY_NAME and callee not in ('sub_10007138', 'sub_10008658'):
            continue
        if not re.search(r'(?<![\w.>])' + re.escape(callee) + r'\s*\(', full):
            continue
        if cc['drop']:
            full = drop_args(full, callee, set(cc['drop']), len(hexrays_params(BY_NAME[callee])))

    refs, defines = [], []
    gtypes = {}
    # functions
    called = sorted(set(re.findall(r'\bsub_[0-9A-F]{8}\b', full)))
    self_fwd = None
    for callee in called:
        cc = CONV.get(callee)
        if cc is None:
            sys.exit(f'{name}: no conv.txt entry for {callee}')
        if callee == name:
            # a recursive call: the same forwarder, after a forward declaration
            self_fwd = forwarder(name, callee, cc)
            defines.append((callee, f'__fwd_{name}_{callee}'))
            continue
        # asm-convention callees (the early exit) are always entered through
        # their PE address, which reaches the moved copy through its patch.
        if callee in accepted and cc['conv'] not in ('regs', 'asm'):
            refs.append(forwarder(name, callee, cc))
            defines.append((callee, f'__fwd_{name}_{callee}'))
        else:
            if cc['conv'] == 'regs':
                sys.exit(f'{name}: calls {callee}, which has a register interface; needs an override')
            # A moved asm-convention callee's own name is its naked copy, so the
            # PE-address reference to it needs another one.
            alias = ('__pe_' if cc['conv'] == 'asm' else '__') + callee
            refs.append(f"#ifndef __PE_DECL_{alias}\n#define __PE_DECL_{alias}\n"
                        f"{callee_decl(callee, cc, alias)}\n#endif")
            defines.append((callee, alias))

    # globals: vbn.txt, plus auto-named ones Hex-Rays used in this body under a
    # different name than in the data declarations (dword_1000D560 is
    # Data_size____10u_n_0 there): the address is in the name.
    syms = dict(SYMS)
    for g in set(re.findall(r'\b(?:dword|word|byte|unk|off|asc|stru|qword)_[0-9A-F]{8}\b', full)):
        if g not in syms:
            pre = g.split('_')[0]
            syms[g] = (int(g.split('_')[1], 16), 'D', None,
                       {'dword': 'int', 'word': '__int16', 'byte': 'char', 'unk': 'char', 'off': 'void *',
                        'asc': 'char', 'stru': 'char', 'qword': '__int64'}[pre] + ' [name]')
    for g in sorted(syms, key=len, reverse=True):
        va_g, kind, size, ty = syms[g]
        if kind != 'D':
            continue
        uses = [m for m in re.finditer(r'(?<![\w.>])' + re.escape(g) + r'\b', full)
                if not any(a <= m.start() < b for a, b in literal_spans(full))]
        if not uses:
            continue
        if g in local_names and not re.search(r'::\s*' + re.escape(g) + r'\b', full):
            continue                                     # only the local of the same name
        base, dims = globals_type(g, tail)
        if base == '_UNKNOWN' and not dims:
            dims = '[]'          # bytes at an address: its bare name is that address
        esz = type_size(base)
        if not dims and '*' not in base and re.search(
                (r'::\s*' if g in local_names else r'(?<![\w.>])') + re.escape(g) + r'\s*\[', full):
            dims = '[]'
        if dims:
            # Whatever extent Hex-Rays guessed, bodies index past it (records laid
            # out as parallel arrays: j[6 * n]); an extent g++ believes lets it
            # assume the index is in range.  Run every array to the image's end.
            dims = f'[{(0x14000000 - va_g) // max(1, esz)}]'
        alias = f'__{name}_{g}' if IDENT_OK.match(g) else f'__{name}_g{g}'
        refs.append(f"typedef {base} t{alias}{dims};\n"
                    f"static t{alias} &{alias} = *(t{alias} *)0x{va_g:08X};")
        if IDENT_OK.match(g):
            defines.append((g, alias))
        else:
            # Hex-Rays named it after its string ("123456789012345"), which
            # no macro can be called: rename its uses in the code instead.
            pat = re.compile(r'(?<![\w.])' + re.escape(g) + r'(?![\w.])')
            full = '\n'.join(sub_code(pat, alias, l) for l in full.split('\n'))
            g = alias
        if g not in local_names:
            gtypes[g] = base + dims

    code_only = '\n'.join(re.sub(r'//.*$', '', l) for l in full.split('\n'))
    # 8-bit comparisons, with the types of locals, parameters and globals known
    types = dict(gtypes)
    for l in decls + reg_lines + frame_lines + params:
        mm = re.match(r'^\s*([A-Za-z_][\w\s\*]*?[\s\*])\s*\(?&?\s*([A-Za-z_]\w*)\)?\s*(\[[^\]]*\])?', l.split('//')[0].split('=')[0])
        if mm:
            types[mm.group(2)] = mm.group(1).strip() + (mm.group(3) or '')
    full = '\n'.join(byte_compares(l, types) for l in full.split('\n'))

    for s in re.findall(r'"(?:[^"\\]|\\.)*"', code_only):
        print(f'warning: {name}: string literal {s[:40]} lives in dummy.so, not in the image; '
              f'a DOS call cannot take it', file=sys.stderr)

    ret = re.sub(r'\b__noreturn\b', '', f['sig'][:re.search(r'\b' + re.escape(name) + r'\b', f['sig']).start()])
    for k in ('__usercall', '__userpurge', '__cdecl', '__stdcall', '__fastcall', '__thiscall'):
        ret = ret.replace(k, '')
    ret = ' '.join(re.sub(r'__spoils<[^>]*>', '', ret).split()) or 'int'

    out = [f"// {name} @ 0x{va:08X} -- generated by tools/extract.py from vbn.exe.c lines {f['start']}..{f['end']}",
           f"// Hex-Rays: {f['sig']}",
           f"// real convention: {c['conv']}" + (f", ret {c['pop']}" if c['pop'] else '')
           + (f"; register parameters dropped: {', '.join(p for p, _ in regvars)}" if regvars else ''),
           '']
    out += refs
    out += [f'#define {a} {b}' for a, b in defines]
    proto = f"static {ATTR[c['conv']]}VBN_ENTRY {ret} __{name}({', '.join(params) or 'void'})"
    if self_fwd:
        out += [proto + ';', self_fwd]
    out += ['', f'PROBE_DECL(__{name})', proto, '{',
            f'  PROBE_HIT(__{name});']
    out += reg_lines + frame_lines
    out += full.split('\n')
    out += ['}']
    out += [f'#undef {a}' for a, b in defines]
    return '\n'.join(out) + '\n'


def main():
    names = sys.argv[1:]
    accepted = set(load_accepted())
    for n in names:
        text = emit(n, accepted | {n})
        p = os.path.join(ROOT, 'inc', n + '.inc')
        open(p, 'w').write(text)
        print('wrote', os.path.relpath(p, ROOT))


if __name__ == '__main__':
    main()
