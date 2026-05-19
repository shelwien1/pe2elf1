#!/usr/bin/env python3
"""
Fix clang pointer-cast errors in IDA-decompiled subs__3b.inc / stats.inc.
Reads clang error output, inserts (uintptr_t) intermediate casts at the
exact column positions reported, then applies targeted fixes for the
remaining function-call type mismatches.
"""
import subprocess, re, sys, collections

SRC_DIR = '.'

def clang_errors():
    r = subprocess.run(
        ['clang++', '-Ofast', '-march=native', '-s', '-ferror-limit=0',
         'dummy.cpp', '-o', 'dummy'],
        capture_output=True, text=True, cwd=SRC_DIR)
    return r.stderr

def find_cast_close(line, col0):
    """Return index just after the ')' that closes a C cast starting at col0."""
    assert line[col0] == '(', repr(line[col0])
    depth = 0
    for i in range(col0, len(line)):
        if line[i] == '(':
            depth += 1
        elif line[i] == ')':
            depth -= 1
            if depth == 0:
                return i + 1   # position right after the ')'
    return None

def insert_uintptr(line, col1):
    """Insert (uintptr_t) after the cast type whose '(' is at 1-based col1."""
    col0 = col1 - 1
    pos = find_cast_close(line, col0)
    if pos is None:
        return line
    return line[:pos] + '(uintptr_t)' + line[pos:]

# ── collect "cast from pointer to smaller type" errors ───────────────────────
errors_raw = clang_errors()

# group by file → line → list of (col, msg)
cast_errors = collections.defaultdict(lambda: collections.defaultdict(list))
pat = re.compile(r'\./(\S+?):(\d+):(\d+): error: (cast from pointer to smaller type.*)')
for m in pat.finditer(errors_raw):
    fname, lineno, col, msg = m.group(1), int(m.group(2)), int(m.group(3)), m.group(4)
    cast_errors[fname][lineno].append(col)

# ── apply uintptr_t insertions (right-to-left per line to keep offsets valid) ─
for fname, line_map in cast_errors.items():
    with open(fname) as f:
        lines = f.readlines()
    for lineno, cols in line_map.items():
        src = lines[lineno - 1]
        for col in sorted(cols, reverse=True):
            src = insert_uintptr(src, col)
        lines[lineno - 1] = src
    with open(fname, 'w') as f:
        f.writelines(lines)
    print(f'patched {sum(len(v) for v in line_map.values())} cast(s) in {fname}')

# ── fix __PrintStats signature (int → __int64 for 3rd param) ─────────────────
stats_file = 'stats.inc'
with open(stats_file) as f:
    src = f.read()
patched = src.replace(
    '__PrintStats(FILE* outFile, FILE* inFile, int isFinal)',
    '__PrintStats(FILE* outFile, FILE* inFile, __int64 isFinal)')
if patched != src:
    with open(stats_file, 'w') as f:
        f.write(patched)
    print('patched __PrintStats signature in stats.inc')

# ── fix __sub_140010730 / __sub_140010850 call sites (ptr → __int64) ─────────
# These take __int64 a1 but are called with pointer expressions.
sub_730_lines = [4979, 5010, 5025, 5072, 5768, 5795, 5807, 5819]
sub_850_lines = [5283, 5396, 5406, 5924, 6070, 6084]

subs_file = 'subs__3b.inc'
with open(subs_file) as f:
    lines = f.readlines()

def wrap_first_arg_int64(line, fname):
    """Wrap the first argument of fname(...) with (__int64)(uintptr_t)(...)."""
    pat = re.compile(r'(' + re.escape(fname) + r'\()(.+?)(\)[ \t]*;)', re.DOTALL)
    def repl(m):
        before, args, after = m.group(1), m.group(2), m.group(3)
        # split off first argument (no nested commas at depth 0)
        depth = 0
        split = len(args)
        for i, c in enumerate(args):
            if c == '(': depth += 1
            elif c == ')': depth -= 1
            elif c == ',' and depth == 0:
                split = i
                break
        first = args[:split].strip()
        rest  = args[split:]   # starts with ',' or is empty
        return f'{before}(__int64)(uintptr_t)({first}){rest}{after}'
    return pat.sub(repl, line)

for ln in sub_730_lines:
    lines[ln-1] = wrap_first_arg_int64(lines[ln-1], '__sub_140010730')
for ln in sub_850_lines:
    lines[ln-1] = wrap_first_arg_int64(lines[ln-1], '__sub_140010850')

# ── fix __sub_14000A4B0 call sites (int → unsigned __int8 *) ─────────────────
a4b0_lines = [2626, 2646]
def wrap_first_arg_uchar_ptr(line, fname):
    pat = re.compile(r'(' + re.escape(fname) + r'\()(.+?)(\))', re.DOTALL)
    def repl(m):
        before, args, after = m.group(1), m.group(2), m.group(3)
        depth = 0
        split = len(args)
        for i, c in enumerate(args):
            if c == '(': depth += 1
            elif c == ')': depth -= 1
            elif c == ',' and depth == 0:
                split = i
                break
        first = args[:split].strip()
        rest  = args[split:]
        return f'{before}(unsigned __int8 *)(uintptr_t)({first}){rest}{after}'
    return pat.sub(repl, line)

for ln in a4b0_lines:
    lines[ln-1] = wrap_first_arg_uchar_ptr(lines[ln-1], '__sub_14000A4B0')

# ── fix line 1841: *(_QWORD *)v5 = i;  (i is unsigned char*, assign to uint64) ─
ln = 1841
lines[ln-1] = lines[ln-1].replace(' = i;', ' = (uint64)(uintptr_t)i;')

with open(subs_file, 'w') as f:
    f.writelines(lines)
print('patched function call sites in subs__3b.inc')
