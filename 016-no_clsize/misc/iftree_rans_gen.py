# Standalone probe -- NOT part of the build. Emits rans_tree.h for
# iftree_rans.cpp. The rANS twin of iftree_gen.py; see misc/iftree_rans.md.
#
# Two trees over the same 255 nodes, both with cty[c] as a compile-time
# constant address and one branch site per node:
#
#   T  the coder step inline at every node -- iftree_gen.py's shape, rewritten
#      for rANS: refill, s/a, the branch, the state update
#   U  the branch only. The node decides the bit from a PRECOMPUTED s[k] and
#      records p; the eight coder updates run after the byte, over lanes whose
#      states are independent. This is the shape the codebase's lane
#      arrangement actually wants -- byte m's bit j lives in lane m*8+j, so
#      the eight states of one byte are eight different lanes and nothing
#      chains through them within the byte.
def emit(c, depth, out, shape, DEP=0):
    ind = '  '*(depth+1)
    b1, b0 = 2*c+1, 2*c
    out.append(f'{ind}{{ const uint _st = cty[{c}].v; const uint _p = _st & 0xFFFF;')
    if shape == 'T':
        out.append(f'{ind}  const uint _k = _base+{depth};')
        out.append(f'{ind}  if( __builtin_expect_with_probability( rx[_k]<RANSL, 0, 0.99 ) ) {{')
        out.append(f'{ind}    uint _d = (rx[_k]<(RANSL>>8)) ? 16 : 8;')
        out.append(f'{ind}    rx[_k] <<= _d; rx[_k] |= load32(base+tp[_k]-3) >> (32-_d); tp[_k] -= _d>>3; }}')
        out.append(f'{ind}  const uint _s = rx[_k] & mSCALE, _a = _p*(rx[_k]>>SCALElog);')
        t_hi = f'rx[_k] = rx[_k]-_p-_a; cty[{c}].v = FSM2[(_st>>16)*2+1];'
        t_lo = f'rx[_k] = _a+_s;        cty[{c}].v = FSM2[(_st>>16)*2+0];'
        cond = '_s >= _p'
        if DEP:
            out.append(f'{ind}  uint _sd=_s; __asm__("" : "+r"(_sd) : "r"(_a));')
            cond = '_sd >= _p'
    else:
        out.append(f'{ind}  const uint _k = _base+{depth}; pv[_k] = _p;')
        t_hi = f'cty[{c}].v = FSM2[(_st>>16)*2+1];'
        t_lo = f'cty[{c}].v = FSM2[(_st>>16)*2+0];'
        cond = 'sv[_k] >= _p'
    if depth == 7:
        out.append(f'{ind}  if( {cond} ) {{ {t_hi} _sym = {b1}; }} else {{ {t_lo} _sym = {b0}; }} }}')
        return
    out.append(f'{ind}  if( {cond} ) {{ {t_hi}')
    emit(b1, depth+1, out, shape, DEP)
    out.append(f'{ind}  }} else {{ {t_lo}')
    emit(b0, depth+1, out, shape, DEP)
    out.append(f'{ind}  }} }}')

# P<D>: the tree only for the first D levels, where the bias is strongest and
# the node count is small, then hand the context back to an indexed walk. The
# middle ground between T/U and the plain walk.
def emitP(c, depth, out, D):
    ind = '  '*(depth+1)
    if depth == D:
        out.append(f'{ind}_lctx = {c};')
        return
    out.append(f'{ind}{{ const uint _st = cty[{c}].v; const uint _p = _st & 0xFFFF;')
    out.append(f'{ind}  const uint _k = _base+{depth}; pv[_k] = _p;')
    out.append(f'{ind}  if( sv[_k] >= _p ) {{ cty[{c}].v = FSM2[(_st>>16)*2+1];')
    emitP(2*c+1, depth+1, out, D)
    out.append(f'{ind}  }} else {{ cty[{c}].v = FSM2[(_st>>16)*2+0];')
    emitP(2*c, depth+1, out, D)
    out.append(f'{ind}  }} }}')

out=[]
emit(1,0,out,'T',1)
open('rans_tree_TD.h','w').write('\n'.join(out)+'\n')
print('rans_tree_TD.h:', len(out), 'lines')
for shape in ('T','U'):
    out=[]
    emit(1,0,out,shape)
    open(f'rans_tree_{shape}.h','w').write('\n'.join(out)+'\n')
    print(f'rans_tree_{shape}.h:', len(out), 'lines')
import sys
for D in (2,3,4):
    out=[]
    emitP(1,0,out,D)
    open(f'rans_tree_P{D}.h','w').write('\n'.join(out)+'\n')
    print(f'rans_tree_P{D}.h:', len(out), 'lines')
