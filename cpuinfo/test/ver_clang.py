# compare the table's clang sets with clang's -###  target features per -march
# (clang passes no feature list for a named cpu, so this uses the macros)
import os, re, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
TABLE = os.environ.get('TABLE', 'table.tsv')   # from dumptab
CL = sys.argv[1]
M = {'__MMX__':'mmx','__SSE__':'sse','__SSE2__':'sse2','__SSE3__':'sse3','__SSSE3__':'ssse3','__SSE4_1__':'sse4.1','__SSE4_2__':'sse4.2','__SSE4A__':'sse4a',
'__POPCNT__':'popcnt','__LZCNT__':'lzcnt','__MOVBE__':'movbe','__PRFCHW__':'prfchw','__PCLMUL__':'pclmul','__AES__':'aes','__XSAVE__':'xsave','__XSAVEOPT__':'xsaveopt','__XSAVEC__':'xsavec','__XSAVES__':'xsaves',
'__AVX__':'avx','__F16C__':'f16c','__FMA__':'fma','__FMA4__':'fma4','__XOP__':'xop','__TBM__':'tbm','__LWP__':'lwp','__BMI__':'bmi','__BMI2__':'bmi2','__ADX__':'adx','__AVX2__':'avx2',
'__FSGSBASE__':'fsgsbase','__RDRND__':'rdrnd','__RDSEED__':'rdseed','__INVPCID__':'invpcid','__CLFLUSHOPT__':'clflushopt','__CLWB__':'clwb','__CLZERO__':'clzero','__MWAITX__':'mwaitx','__RDPID__':'rdpid','__RDPRU__':'rdpru','__WBNOINVD__':'wbnoinvd','__SHA__':'sha','__SGX__':'sgx','__PKU__':'pku','__GFNI__':'gfni','__VAES__':'vaes','__VPCLMULQDQ__':'vpclmulqdq',
'__AVX512F__':'avx512f','__AVX512CD__':'avx512cd','__AVX512DQ__':'avx512dq','__AVX512BW__':'avx512bw','__AVX512VL__':'avx512vl','__AVX512IFMA__':'avx512ifma','__AVX512VBMI__':'avx512vbmi','__AVX512VBMI2__':'avx512vbmi2','__AVX512VNNI__':'avx512vnni','__AVX512BITALG__':'avx512bitalg','__AVX512VPOPCNTDQ__':'avx512vpopcntdq','__AVX512BF16__':'avx512bf16','__AVX512FP16__':'avx512fp16','__AVX512VP2INTERSECT__':'avx512vp2intersect','__AVX512BMM__':'avx512bmm',
'__AVXVNNI__':'avxvnni','__AVXIFMA__':'avxifma','__AVXVNNIINT8__':'avxvnniint8','__AVXVNNIINT16__':'avxvnniint16','__AVXNECONVERT__':'avxneconvert','__AVX10_1__':'avx10.1','__AVX10_2__':'avx10.2','__APX_F__':'apxf',
'__MOVDIRI__':'movdiri','__MOVDIR64B__':'movdir64b','__WAITPKG__':'waitpkg','__CLDEMOTE__':'cldemote','__PTWRITE__':'ptwrite','__SERIALIZE__':'serialize','__TSXLDTRK__':'tsxldtrk','__ENQCMD__':'enqcmd','__UINTR__':'uintr','__HRESET__':'hreset','__PCONFIG__':'pconfig','__KL__':'kl','__WIDEKL__':'widekl','__SHSTK__':'shstk','__CMPCCXADD__':'cmpccxadd','__PREFETCHI__':'prefetchi','__RAOINT__':'raoint','__SHA512__':'sha512','__SM3__':'sm3','__SM4__':'sm4','__USERMSR__':'usermsr','__MOVRS__':'movrs',
'__AMX_TILE__':'amx-tile','__AMX_INT8__':'amx-int8','__AMX_BF16__':'amx-bf16','__AMX_FP16__':'amx-fp16','__AMX_COMPLEX__':'amx-complex','__AMX_FP8__':'amx-fp8','__AMX_AVX512__':'amx-avx512','__AMX_MOVRS__':'amx-movrs','__CRC32__':'crc32',
'__FXSR__':'fxsr','__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8':'cx8','__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16':'cx16','__LAHF_SAHF__':'sahf'}
known = set(M.values())
only64 = {'64bit', 'sahf', 'cx16', 'apxf', 'uintr'}
unk = set(); bad = 0; n = 0
for line in open(TABLE):
  nm, fl, gs, cs = line.rstrip('\n').split('\t')
  fl = int(fl)
  if not fl & 2: continue
  tgt = 'x86_64-linux-gnu' if fl & 4 else 'i686-linux-gnu'
  p = subprocess.run([CL, '--target=' + tgt, '-march=' + nm, '-dM', '-E', '-x', 'c', '/dev/null'], capture_output=True, text=True)
  if p.returncode: print('%-16s clang refuses: %s' % (nm, p.stderr.strip().splitlines()[-1])); bad += 1; continue
  macs = set(re.findall(r'#define (\w+)', p.stdout))
  have = {M[k] for k in macs if k in M}
  for k in macs:
    if re.match(r'__[A-Z0-9_]+__$', k) and k not in M and k.upper() == k and not k.startswith(('__GCC', '__SIZEOF', '__INT', '__UINT', '__FLT', '__DBL', '__LDBL', '__SCHAR', '__SHRT', '__WCHAR', '__WINT', '__PTRDIFF', '__SIZE', '__INTPTR', '__UINTPTR', '__CHAR', '__ATOMIC', '__CLANG', '__OPENCL', '__MEMORY', '__BIGGEST', '__ORDER', '__BYTE', '__OBJC', '__VERSION', '__STDC', '__USER', '__ELF', '__FINITE', '__NO_', '__OPTIMIZE', '__PIC', '__PIE', '__POINTER', '__REGISTER', '__LONG', '__FPCLASS', '__BITINT', '__BOOL', '__FLT16', '__BFLT16', '__GXX', '__SSP', '__SEG', '__FUNC', '__LITTLE', '__llvm', '__clang', '__x86', '__amd64', '__k8', '__unix', '__linux', '__gnu_linux', '__code_model', '__CET')):
      unk.add(k)
  mine = set(cs.split()) & known
  if not fl & 4: mine -= only64; have -= only64
  else: mine.discard('cx8'); have.discard('cx8')
  n += 1
  if mine != have:
    bad += 1
    print('%-16s %s table-only: %s | clang-only: %s' % (nm, tgt.split('-')[0], ' '.join(sorted(mine - have)), ' '.join(sorted(have - mine))))
print('clang: %d targets checked, %d differ' % (n, bad))
print('macros not mapped:', ' '.join(sorted(unk)))
