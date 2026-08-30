# Standalone probe -- NOT part of the build. Emits tree.h for iftree_bench.cpp.
#   python3 iftree_gen.py && clang++ -O2 -march=native -o b iftree_bench.cpp
#   ./b <enwik8> <bytes> <lanes>
# See misc/dec_vectorize.md section 9.
# Emit the 255-node if-tree: one static branch site per context node, so each
# gets its own predictor entry and cty[c] is a compile-time constant address.
def emit(c, depth, out):
    ind = '  '*(depth+1)
    out.append(f'{ind}// node {c}')
    out.append(f'{ind}rpre = (range>>SCALElog)*cty[{c}];')
    if depth == 7:
        out.append(f'{ind}if( code >= rpre ) {{ range -= rpre; code -= rpre; sym = {2*c+1};'
                   f' upd({c},1); }} else {{ range = rpre; sym = {2*c}; upd({c},0); }}')
        out.append(f'{ind}RENORM')
        return
    out.append(f'{ind}if( code >= rpre ) {{')
    out.append(f'{ind}  range -= rpre; code -= rpre; upd({c},1); RENORM')
    emit(2*c+1, depth+1, out)
    out.append(f'{ind}}} else {{')
    out.append(f'{ind}  range = rpre; upd({c},0); RENORM')
    emit(2*c, depth+1, out)
    out.append(f'{ind}}}')
out=[]
emit(1,0,out)
open('tree.h','w').write('\n'.join(out)+'\n')
print('tree.h:', len(out), 'lines')
