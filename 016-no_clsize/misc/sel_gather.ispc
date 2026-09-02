// Standalone probe -- NOT part of the build.  See misc/cmov_probe.md, "ISPC".
//   ispc -O2 --target=avx512skx-x16 --emit-asm -o sel.s sel_gather.ispc

// cond ? *p : q  with a VARYING pointer and a VARYING condition -- the question
export void sel_ternary(uniform int n, uniform int * uniform base,
                        uniform int idx[], uniform int q[], uniform int cond[], uniform int out[]) {
  foreach (i = 0 ... n) {
    int * p = base + idx[i];
    out[i] = cond[i] ? *p : q[i];
  }
}
// the same as if/else, for comparison
export void sel_if(uniform int n, uniform int * uniform base,
                   uniform int idx[], uniform int q[], uniform int cond[], uniform int out[]) {
  foreach (i = 0 ... n) {
    int * p = base + idx[i];
    int x;
    if (cond[i]) x = *p; else x = q[i];
    out[i] = x;
  }
}
// uniform condition: control
export void sel_uniform(uniform int n, uniform int * uniform base,
                        uniform int idx[], uniform int q[], uniform int cond, uniform int out[]) {
  foreach (i = 0 ... n) {
    int * p = base + idx[i];
    out[i] = cond ? *p : q[i];
  }
}
