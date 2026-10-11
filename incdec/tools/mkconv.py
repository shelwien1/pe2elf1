#!/usr/bin/env python3
"""conv.txt: the calling convention every function really has.

Hex-Rays prints most of vbn's functions as __usercall/__userpurge with register
arguments, and they are almost all artefacts: a partial-register write before a
DOS call (`mov ah,3Ch`), a register read uninitialised on an edge path, or a
signature inherited from a callee that was itself wrong.  regflow.py follows
every path through each function (into its .shim thunks and back) and reports
the registers read before being written.  Apart from the cases listed in MANUAL
below, the answer is none, so a function's real interface is its stack
arguments, and `ret N` decides who pops them: N > 0 is stdcall, 0 is cdecl.

Columns: VA, name, conv, pop, drop, proto
  conv   cdecl | stdcall | regs (register interface, needs a thunk) | asm
  pop    bytes the callee pops
  drop   positions of Hex-Rays parameters that are not real (register
         artefacts): removed from the moved definition and from every call site
  proto  return type and real parameter list, `ret|params`
"""
import os, re, sys
from common import ROOT, CODE_LO, load_funcs, split_params
from regflow import analyze

# Corrections to what Hex-Rays + regflow give, each verified against vbn.asm.
MANUAL = {
    # printf/scanf: Hex-Rays typed them with a fixed signature and dropped every
    # variadic argument at the call sites; fixups.txt puts them back.
    '100079DA': ('cdecl', 'int|char *fmt, ...', 'printf: lea eax,[ebp+0Ch] is the va_list'),
    '100072C5': ('cdecl', 'int|char *fmt, ...', 'scanf (core sub_10007345)'),
    '100079EE': ('cdecl', 'int|char *fmt, void *va', 'vprintf'),
    # Hex-Rays gave it no parameters; the call site pushes two (rename old,new).
    '10008439': ('cdecl', 'int|char *a1, char *a2', 'rename: mov edi,[ebp+0Ch], edx=[ebp+8]'),
    # __thiscall in Hex-Rays, but ECX is never read: getdisk().
    '100088EA': ('cdecl', 'int|', 'getdisk; the `this` is an artefact', [0]),
    # mkdir: `mov ah,39h` and a jump into chdir's tail (sub_10008617), which
    # passes [esp+4] in EDX and returns EAX; Hex-Rays gave it neither.
    '1000862D': ('cdecl', 'int|char *path', 'mkdir: chdir\'s tail reads the path at [esp+4]'),
    # Bannerless entry points: one `jmp` each into malloc / free.
    '10007138': ('cdecl', '_DWORD|_DWORD size', 'jmp sub_10009264 (malloc); Hex-Rays types'),
    '10008658': ('cdecl', '_DWORD|_DWORD p', 'jmp sub_100092F4 (free); Hex-Rays types'),
    # Register interfaces (override/ has the C and the thunks).
    '10006CB0': ('regs', None, 'tree walk: in EBX node, EDX right-child offset, ESI value, ECX cursor, EDI count; out ECX, EDI'),
    '10006E31': ('regs', None, 'tree walk: in EBX node, EDX byte table, ESI value, ECX cursor, EDI count; out ECX, EDI'),
    # The compressor's early exit (head.cpp, vbn_restartable).
    '1000703B': ('asm', 'void|int a2', 'records ESP/EBP for sub_10006E22; ret 4'),
    # Its callers push nothing (`call sub_10006E22` alone): it never returns to
    # them, and the `ret 4` pops the argument of the call sub_1000703B recorded.
    '10006E22': ('asm', 'void|', 'restores ESP/EBP recorded by sub_1000703B; ret 4; callers push nothing', [0]),
}
EXTRA = {0x10007138: 'sub_10007138', 0x10008658: 'sub_10008658'}


def parse(sig, name):
    m = re.search(r'\b' + re.escape(name) + r'\s*(?:@<(\w+)>)?\s*\(', sig)
    ret = sig[:m.start()]
    for k in ('__usercall', '__userpurge', '__cdecl', '__stdcall', '__fastcall', '__thiscall', '__noreturn'):
        ret = ret.replace(k, '')
    ret = ' '.join(re.sub(r'__spoils<[^>]*>', '', ret).split()) or 'int'
    inner = sig[m.end():sig.rindex(')')]
    params = []
    if inner.strip() and inner.strip() != 'void':
        for p in split_params(inner):
            rm = re.search(r'@<(\w+)>', p)
            params.append((re.sub(r'@<\w+>', '', p).strip(), rm.group(1) if rm else None))
    return ret, params


def linear_rets():
    """`ret N` forms in each function's own address range.  regflow follows
    paths and can give up inside a loop-heavy body before reaching any ret
    (sub_100056C4: none found, and it is `ret 0Ch`), so N comes from here."""
    from common import disassemble
    funcs = sorted(f['addr'] for f in load_funcs() if f['addr'] >= CODE_LO)
    import bisect
    out = {}
    for va, text in disassemble():
        if va >= 0x14000000:
            continue
        k = bisect.bisect_right(funcs, va) - 1
        m = re.match(r'(?:ret|lret)\s*(\$0x[0-9a-f]+)?', text.split('\t')[-1])
        if k >= 0 and m:
            out.setdefault(funcs[k], set()).add(int(m.group(1)[1:], 16) if m.group(1) else 0)
    return out


if __name__ == '__main__':
    funcs = load_funcs()
    lrets = linear_rets()
    starts = {f['addr'] for f in funcs}
    rows = []
    for f in funcs:
        a = f['addr']
        if a < CODE_LO:
            continue
        key = '%08X' % a
        ret, params = parse(f['sig'], f['name'])
        r = analyze(a, starts)
        pop = max(lrets[a]) if a in lrets else (max(r['rets']) if r['rets'] else 0)
        drop = [i for i, (_, reg) in enumerate(params) if reg]
        stack = [p for p, reg in params if not reg]
        conv = 'stdcall' if pop else 'cdecl'
        proto = ret + '|' + ', '.join(stack)
        note = 'regflow: no register inputs' if not r['livein'] else 'regflow live-in: ' + ','.join(r['livein'])
        if key in MANUAL:
            conv, p2, note = MANUAL[key][:3]
            proto = p2 if p2 else proto
            if len(MANUAL[key]) > 3:
                drop = MANUAL[key][3]
        elif r['livein'] and a < 0x1000704C and key != '10001B38':
            sys.exit(f'{key}: unexpected register inputs {r["livein"]}; classify it in MANUAL')
        if pop and conv == 'stdcall' and pop != 4 * len(stack):
            sys.exit(f'{key}: ret {pop} but {len(stack)} stack parameters')
        rows.append((a, f['name'], conv, pop, drop, proto, note))
    for a, n in EXTRA.items():
        conv, proto, note = MANUAL['%08X' % a][:3]
        rows.append((a, n, conv, 0, [], proto, note))
    with open(os.path.join(ROOT, 'conv.txt'), 'w') as fo:
        fo.write('# VA\tname\tconv\tpop\tdrop\tproto (ret|params)\tnote   -- generated by tools/mkconv.py\n')
        for a, n, conv, pop, drop, proto, note in sorted(rows):
            fo.write(f"{a:08X}\t{n}\t{conv}\t{pop}\t{','.join(map(str, drop)) or '-'}\t{proto}\t{note}\n")
    print(len(rows), 'functions')
