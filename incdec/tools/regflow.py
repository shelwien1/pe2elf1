#!/usr/bin/env python3
"""regflow.py: per-function register facts for vbn.exe, from the machine code.

For each function: which GPRs are read before written on some path (live-in),
which callee-saved registers it leaves unchanged on every return path (by push/pop
pairing), its `ret N`, and the highest stack argument offset it touches.
Follows jumps into the .shim thunks and back (they are part of the function).
The vbn_interrupt call inside a thunk reads every register through pushad but
uses only those the DOS service needs; it is reported as 'dos', not as a read.
"""
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import *
from common import SECTIONS, pe_bytes, load_funcs
md = Cs(CS_ARCH_X86, CS_MODE_32); md.detail = True
_pe = pe_bytes()
SECS = [(va, _pe[off:off + size]) for va, off, size, _ in SECTIONS]
def code_at(va, n=16):
    for b, d in SECS:
        if b <= va < b + len(d): return d[va-b:va-b+n], va
    return None, va
# Registers are tracked in parts, so that `mov dl,[esi]` followed by `add dl,dl`
# reads nothing of the caller's EDX, and `mov ah,3Ch` defines only AH.
PARTS = {}
for r in ('eax', 'ebx', 'ecx', 'edx'):
    x = r[1]
    PARTS[r] = (r, {'lo', 'hi8', 'top'}); PARTS[x + 'x'] = (r, {'lo', 'hi8'})
    PARTS[x + 'l'] = (r, {'lo'}); PARTS[x + 'h'] = (r, {'hi8'})
for r in ('esi', 'edi', 'ebp', 'esp'):
    PARTS[r] = (r, {'lo', 'hi8', 'top'}); PARTS[r[1:]] = (r, {'lo', 'hi8'})
ALL = frozenset({'lo', 'hi8', 'top'})
def insn(va):
    b, _ = code_at(va)
    if b is None: return None
    for i in md.disasm(b, va): return i
    return None
def analyze(entry, starts):
    """Live-in registers (read before written on some path), ret N, stack args."""
    livein, rets, dos, calls = set(), set(), 0, set()
    maxarg = 0
    seen = set()
    work = [(entry, frozenset(), 0, True)]
    steps = 0
    while work and steps < 200000:
        va, wr, depth, prologue = work.pop(); steps += 1
        key = (va, wr, depth, prologue)
        if key in seen: continue
        seen.add(key)
        if va != entry and va in starts and not (0x14001000 <= va < 0x14002000):
            calls.add(('tail', va)); continue
        i = insn(va)
        if i is None: continue
        m = i.mnemonic
        regs_r, regs_w = i.regs_access()
        rn = [i.reg_name(r) for r in regs_r]; wn = [i.reg_name(r) for r in regs_w]
        defined = dict((r, set(p)) for r, p in wr)
        def have(r, parts): return parts <= defined.get(r, set())
        ops = i.operands
        same_reg = (len(ops) == 2 and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_REG
                    and ops[0].reg == ops[1].reg)
        is_save = prologue and m == 'push' and ops and ops[0].type == X86_OP_REG
        if m in ('pushal', 'pusha'):
            dos += 1                       # a DOS call through vbn_interrupt
        elif is_save or (same_reg and m in ('xor', 'sub', 'sbb')):
            pass                           # saves a register / defines without reading
        else:
            for r in rn:
                if r in PARTS:
                    full, parts = PARTS[r]
                    if full not in ('esp',) and not have(full, parts):
                        livein.add(full)
        for r in wn:
            if r in PARTS:
                full, parts = PARTS[r]
                if m in ('movzx', 'movsx') or parts == ALL:
                    defined[full] = set(ALL)
                else:
                    defined.setdefault(full, set()).update(parts)
        if m == 'enter': defined['ebp'] = set(ALL)
        if m in ('popal', 'popa'):
            for r in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp'): defined[r] = set(ALL)
        nw = frozenset((r, frozenset(p)) for r, p in defined.items())
        for op in ops:
            if op.type == X86_OP_MEM and op.mem.base != 0:
                bn = i.reg_name(op.mem.base)
                if bn == 'ebp' and op.mem.disp >= 8: maxarg = max(maxarg, op.mem.disp - 4)
                if bn == 'esp' and op.mem.disp - depth >= 4: maxarg = max(maxarg, op.mem.disp - depth)
        nd = depth
        if m == 'push': nd += 4
        elif m == 'pop': nd -= 4
        elif m in ('pushfd', 'pushfl', 'pushf'): nd += 4
        elif m in ('popfd', 'popfl', 'popf'): nd -= 4
        elif m in ('pushal', 'pusha'): nd += 32
        elif m in ('popal', 'popa'): nd -= 32
        elif m == 'sub' and ops[0].type == X86_OP_REG and i.reg_name(ops[0].reg) == 'esp' and ops[1].type == X86_OP_IMM: nd += ops[1].imm
        elif m == 'add' and ops[0].type == X86_OP_REG and i.reg_name(ops[0].reg) == 'esp' and ops[1].type == X86_OP_IMM: nd -= ops[1].imm
        elif m == 'enter': nd += 4 + ops[0].imm
        stays_prologue = prologue and (m in ('push', 'enter') or (m == 'mov' and i.op_str == 'ebp, esp')
                                       or (m == 'sub' and i.op_str.startswith('esp,')))
        nxt = va + i.size
        if m in ('ret', 'retn'):
            rets.add(ops[0].imm if ops else 0); continue
        if m in ('hlt', 'ud2', 'iret', 'iretd', 'retf', 'lret'): continue
        if m == 'call':
            op = ops[0]
            if op.type == X86_OP_IMM: calls.add(('call', op.imm))
            elif op.type == X86_OP_MEM and op.mem.base == 0 and op.mem.disp == 0x0FFF3064: calls.add(('dos', 0))
            else: calls.add(('indirect', i.op_str))
            d2 = dict(defined)
            for r in ('eax', 'ecx', 'edx'): d2[r] = set(ALL)
            work.append((nxt, frozenset((r, frozenset(p)) for r, p in d2.items()), nd, False)); continue
        if m == 'jmp':
            op = ops[0]
            if op.type == X86_OP_IMM: work.append((op.imm, nw, nd, False))
            else: calls.add(('ijmp', i.op_str))
            continue
        if m.startswith('j') or m in ('loop', 'loope', 'loopne', 'jecxz', 'jcxz'):
            op = ops[0]
            if op.type == X86_OP_IMM: work.append((op.imm, nw, nd, False))
            work.append((nxt, nw, nd, False)); continue
        work.append((nxt, nw, nd, stays_prologue))
    return dict(livein=sorted(livein), rets=sorted(rets), maxarg=maxarg, dos=dos, calls=sorted(map(str, calls)))


def analyze_all():
    F = load_funcs()
    starts = {f['addr'] for f in F}
    return {f['addr']: analyze(f['addr'], starts) for f in F if f['addr'] >= 0x10000000}

if __name__ == '__main__':
    F = load_funcs()
    starts = {f['addr'] for f in F}
    want = [int(a, 16) for a in sys.argv[1:]] or sorted(a for a in starts if a >= 0x10000000)
    for a in want:
        r = analyze(a, starts)
        sig = next((f['sig'] for f in F if f['addr'] == a), '?')
        print(f"{a:08X} livein={','.join(r['livein']) or '-'} ret={r['rets']} "
              f"stackargs={r['maxarg'] // 4 if r['maxarg'] else 0} dos={r['dos']}  | {sig[:90]}")
