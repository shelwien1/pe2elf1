# mkschema.py orig.wd > ../tfwz/schema.inc
# the tensor set (names, dtypes, shapes, file order) of the 6m model, as a C table
import sys, wd
DT = {0: 'DT_I8', 1: 'DT_BF16', 2: 'DT_F32', 3: 'DT_I32'}
ts = wd.load(sys.argv[1])
print('// schema.inc - the tensor set of the 6m model, in the order of the shipped')
print('// 6m-q4-fp32 FX2TFWC2 file (generated from it by tools/mkschema.py)')
print('// name, dtype, ndim, dims')
print('static const SchemaEntry kSchema[] = {')
for name, dt, shape, d in ts:
    s = list(shape) + [0] * (2 - len(shape))
    print('  {"%s", fx2::%s, %d, {%d, %d}},' % (name, DT[dt], len(shape), s[0], s[1]))
print('};')
print('static const int kSchemaN = %d;' % len(ts))
