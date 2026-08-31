# Standalone probe -- NOT part of the build. Emits tree_lanes.h for
# iftree_lanes.cpp.  See misc/iftree_lanes.md.
#
# Unlike iftree_gen.py, this is the CODEBASE's lane arrangement: byte m's bit j
# lives in lane m*8+j, so the eight bits of a byte come from eight INDEPENDENT
# coder instances.  Their code/range are all known when the byte starts; the
# only serial edge across the byte is the model context.  That is what the tree
# is aimed at -- each node is a static branch site with cty[c] at a
# compile-time address, so the predictor can run the context chain ahead of the
# compares.
def emit(c, depth, out):
    ind = '  '*(depth+1)
    L = f'B+{depth}'
    out.append(f'{ind}rp = (rng[{L}]>>SCALElog)*cty[{c}];')
    out.append(f'{ind}if( cod[{L}] >= rp ) {{')
    out.append(f'{ind}  rng[{L}] -= rp; cod[{L}] -= rp; upd({c},1)')
    if depth == 7: out.append(f'{ind}  sym = {2*c+1};')
    else:          emit(2*c+1, depth+1, out)
    out.append(f'{ind}}} else {{')
    out.append(f'{ind}  rng[{L}] = rp; upd({c},0)')
    if depth == 7: out.append(f'{ind}  sym = {2*c};')
    else:          emit(2*c, depth+1, out)
    out.append(f'{ind}}}')
out=[]
emit(1,0,out)
open('tree_lanes.h','w').write('\n'.join(out)+'\n')
print('tree_lanes.h:', len(out), 'lines')
