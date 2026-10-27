#!/usr/bin/env python3
# gen_targets.py -- the -march target table of x86targets.h, from the compilers'
# own sources:
#
#   gen_targets.py LLVM_DIR GCC_DIR [x86targets.h]
#
# LLVM_DIR: an llvm-project checkout, or a directory with X86TargetParser.cpp,
#   X86TargetParser.def and X86.td in it (fetch_sources.sh gets these)
# GCC_DIR: a gcc checkout, or a directory with i386.h, i386-common.cc and
#   i386-isa.def
#
# The table goes into x86targets.h between the "generated" marker lines (the
# rest of the file is left as it is); without the header argument it is
# printed. A report of what differs between the two compilers goes to stderr.
#
# Per target name: which compiler takes it, whether it is a 64-bit target, the
# ISA features -march=NAME enables in each compiler (with the implied ones),
# LLVM's scheduling model and GCC's processor (its tuning), and a group number:
# names that compile to the same thing in every compiler that knows them
# (skx = skylake-avx512, alderlake = raptorlake = meteorlake ...).

import os, re, sys, time

# ---- the features: id, name, class, gcc flag, clang flag
# class C: compilers emit it on their own (code built with it needs it);
# class I: only through intrinsics/builtins (or the OS), so a build "for" a
# target that has it runs without it unless its source asks for it.
FEATURES = [
  ("X87","x87","C","-m80387","-mx87"),
  ("CX8","cx8","C",None,None),
  ("CMOV","cmov","C",None,None),
  ("MMX","mmx","C","-mmmx","-mmmx"),
  ("MMXEXT","mmxext","C",None,None),          # gcc's PTA_PREFETCH_SSE: prefetchnta.. on athlon/geode
  ("3DNOW","3dnow","I","-m3dnow",None),
  ("3DNOWA","3dnowa","I","-m3dnowa",None),
  ("FXSR","fxsr","I","-mfxsr","-mfxsr"),
  ("SSE","sse","C","-msse","-msse"),
  ("SSE2","sse2","C","-msse2","-msse2"),
  ("SSE3","sse3","C","-msse3","-msse3"),
  ("SSSE3","ssse3","C","-mssse3","-mssse3"),
  ("SSE41","sse4.1","C","-msse4.1","-msse4.1"),
  ("SSE42","sse4.2","C","-msse4.2","-msse4.2"),
  ("SSE4A","sse4a","C","-msse4a","-msse4a"),
  ("POPCNT","popcnt","C","-mpopcnt","-mpopcnt"),
  ("LZCNT","lzcnt","C","-mlzcnt","-mlzcnt"),
  ("CX16","cx16","C","-mcx16","-mcx16"),
  ("SAHF","sahf","C","-msahf","-msahf"),       # lahf/sahf in 64-bit mode
  ("LM","64bit","C",None,None),
  ("CRC32","crc32","I","-mcrc32","-mcrc32"),
  ("MOVBE","movbe","C","-mmovbe","-mmovbe"),
  ("PRFCHW","prfchw","C","-mprfchw","-mprfchw"),
  ("PCLMUL","pclmul","I","-mpclmul","-mpclmul"),
  ("AES","aes","I","-maes","-maes"),
  ("XSAVE","xsave","I","-mxsave","-mxsave"),
  ("XSAVEOPT","xsaveopt","I","-mxsaveopt","-mxsaveopt"),
  ("XSAVEC","xsavec","I","-mxsavec","-mxsavec"),
  ("XSAVES","xsaves","I","-mxsaves","-mxsaves"),
  ("AVX","avx","C","-mavx","-mavx"),
  ("F16C","f16c","C","-mf16c","-mf16c"),
  ("FMA","fma","C","-mfma","-mfma"),
  ("FMA4","fma4","C","-mfma4","-mfma4"),
  ("XOP","xop","C","-mxop","-mxop"),
  ("TBM","tbm","C","-mtbm","-mtbm"),
  ("LWP","lwp","I","-mlwp","-mlwp"),
  ("BMI","bmi","C","-mbmi","-mbmi"),
  ("BMI2","bmi2","C","-mbmi2","-mbmi2"),
  ("ADX","adx","C","-madx","-madx"),
  ("AVX2","avx2","C","-mavx2","-mavx2"),
  ("FSGSBASE","fsgsbase","I","-mfsgsbase","-mfsgsbase"),
  ("RDRND","rdrnd","I","-mrdrnd","-mrdrnd"),
  ("RDSEED","rdseed","I","-mrdseed","-mrdseed"),
  ("INVPCID","invpcid","I",None,"-minvpcid"),
  ("CLFLUSHOPT","clflushopt","I","-mclflushopt","-mclflushopt"),
  ("CLWB","clwb","I","-mclwb","-mclwb"),
  ("CLZERO","clzero","I","-mclzero","-mclzero"),
  ("MWAITX","mwaitx","I","-mmwaitx","-mmwaitx"),
  ("RDPID","rdpid","I","-mrdpid","-mrdpid"),
  ("RDPRU","rdpru","I",None,"-mrdpru"),
  ("WBNOINVD","wbnoinvd","I","-mwbnoinvd","-mwbnoinvd"),
  ("SHA","sha","I","-msha","-msha"),
  ("SGX","sgx","I","-msgx","-msgx"),
  ("PKU","pku","I","-mpku","-mpku"),
  ("GFNI","gfni","C","-mgfni","-mgfni"),
  ("VAES","vaes","I","-mvaes","-mvaes"),
  ("VPCLMULQDQ","vpclmulqdq","I","-mvpclmulqdq","-mvpclmulqdq"),
  ("AVX512F","avx512f","C","-mavx512f","-mavx512f"),
  ("AVX512CD","avx512cd","C","-mavx512cd","-mavx512cd"),
  ("AVX512DQ","avx512dq","C","-mavx512dq","-mavx512dq"),
  ("AVX512BW","avx512bw","C","-mavx512bw","-mavx512bw"),
  ("AVX512VL","avx512vl","C","-mavx512vl","-mavx512vl"),
  ("AVX512IFMA","avx512ifma","C","-mavx512ifma","-mavx512ifma"),
  ("AVX512VBMI","avx512vbmi","C","-mavx512vbmi","-mavx512vbmi"),
  ("AVX512VBMI2","avx512vbmi2","C","-mavx512vbmi2","-mavx512vbmi2"),
  ("AVX512VNNI","avx512vnni","C","-mavx512vnni","-mavx512vnni"),
  ("AVX512BITALG","avx512bitalg","C","-mavx512bitalg","-mavx512bitalg"),
  ("AVX512VPOPCNTDQ","avx512vpopcntdq","C","-mavx512vpopcntdq","-mavx512vpopcntdq"),
  ("AVX512BF16","avx512bf16","C","-mavx512bf16","-mavx512bf16"),
  ("AVX512FP16","avx512fp16","C","-mavx512fp16","-mavx512fp16"),
  ("AVX512VP2INTERSECT","avx512vp2intersect","I","-mavx512vp2intersect","-mavx512vp2intersect"),
  ("AVX512BMM","avx512bmm","C","-mavx512bmm","-mavx512bmm"),
  ("AVXVNNI","avxvnni","C","-mavxvnni","-mavxvnni"),
  ("AVXIFMA","avxifma","C","-mavxifma","-mavxifma"),
  ("AVXVNNIINT8","avxvnniint8","C","-mavxvnniint8","-mavxvnniint8"),
  ("AVXVNNIINT16","avxvnniint16","C","-mavxvnniint16","-mavxvnniint16"),
  ("AVXNECONVERT","avxneconvert","C","-mavxneconvert","-mavxneconvert"),
  ("AVX10_1","avx10.1","C","-mavx10.1","-mavx10.1"),
  ("AVX10_2","avx10.2","C","-mavx10.2","-mavx10.2"),
  ("APXF","apxf","C","-mapxf","-mapxf"),
  ("MOVDIRI","movdiri","I","-mmovdiri","-mmovdiri"),
  ("MOVDIR64B","movdir64b","I","-mmovdir64b","-mmovdir64b"),
  ("WAITPKG","waitpkg","I","-mwaitpkg","-mwaitpkg"),
  ("CLDEMOTE","cldemote","I","-mcldemote","-mcldemote"),
  ("PTWRITE","ptwrite","I","-mptwrite","-mptwrite"),
  ("SERIALIZE","serialize","I","-mserialize","-mserialize"),
  ("TSXLDTRK","tsxldtrk","I","-mtsxldtrk","-mtsxldtrk"),
  ("ENQCMD","enqcmd","I","-menqcmd","-menqcmd"),
  ("UINTR","uintr","I","-muintr","-muintr"),
  ("HRESET","hreset","I","-mhreset","-mhreset"),
  ("PCONFIG","pconfig","I","-mpconfig","-mpconfig"),
  ("KL","kl","I","-mkl","-mkl"),
  ("WIDEKL","widekl","I","-mwidekl","-mwidekl"),
  ("SHSTK","shstk","I","-mshstk","-mshstk"),
  ("CMPCCXADD","cmpccxadd","I","-mcmpccxadd","-mcmpccxadd"),
  ("PREFETCHI","prefetchi","I","-mprefetchi","-mprefetchi"),
  ("RAOINT","raoint","C","-mraoint","-mraoint"),
  ("SHA512","sha512","I","-msha512","-msha512"),
  ("SM3","sm3","I","-msm3","-msm3"),
  ("SM4","sm4","I","-msm4","-msm4"),
  ("USERMSR","usermsr","I","-musermsr","-musermsr"),
  ("MOVRS","movrs","I","-mmovrs","-mmovrs"),
  ("AMX_TILE","amx-tile","I","-mamx-tile","-mamx-tile"),
  ("AMX_INT8","amx-int8","I","-mamx-int8","-mamx-int8"),
  ("AMX_BF16","amx-bf16","I","-mamx-bf16","-mamx-bf16"),
  ("AMX_FP16","amx-fp16","I","-mamx-fp16","-mamx-fp16"),
  ("AMX_COMPLEX","amx-complex","I","-mamx-complex","-mamx-complex"),
  ("AMX_FP8","amx-fp8","I","-mamx-fp8","-mamx-fp8"),
  ("AMX_AVX512","amx-avx512","I","-mamx-avx512","-mamx-avx512"),
  ("AMX_MOVRS","amx-movrs","I","-mamx-movrs","-mamx-movrs"),
]
FNAMES = [f[1] for f in FEATURES]
FIDX = {f[1]: i for i, f in enumerate(FEATURES)}

# features of no instruction of their own (dropped, or expanded below)
LLVM_DROP = {"vzeroupper", "evex512", "rtm", "avx10v2aux", "cf",
             "retpoline-external-thunk", "retpoline-indirect-branches",
             "retpoline-indirect-calls", "lvi-cfi", "lvi-load-hardening"}
LLVM_RENAME = {"egpr": "apxf", "push2pop2": "apxf", "ppx": "apxf", "ndd": "apxf",
               "ccmp": "apxf", "nf": "apxf", "zu": "apxf", "jmpabs": "apxf"}

GCC_RENAME = {"64BIT": "64bit", "SSE4_1": "sse4.1", "SSE4_2": "sse4.2", "SSE4A": "sse4a",
              "3DNOW_A": "3dnowa", "PREFETCH_SSE": "mmxext", "APX_F": "apxf",
              "USER_MSR": "usermsr", "AVX10_1": "avx10.1", "AVX10_2": "avx10.2"}
GCC_DROP = {"HLE", "RTM", "NO_TUNE", "NO_SAHF", "NO_80387", "ABM", "AVX10V2AUX", "ACEV1"}

# the vendor and (roughly) the year of each name: for the ranking (vendor
# affinity, then newer first) and the listing
V_GENERIC, V_INTEL, V_AMD, V_HYGON, V_CENTAUR, V_ZHAOXIN = range(6)
VENDOR = {}
def vend(v, year, names):
  for n in names.split(): VENDOR[n] = (v, year)
vend(V_INTEL, 1985, "i386")
vend(V_INTEL, 1989, "i486")
vend(V_INTEL, 1993, "i586 pentium")
vend(V_INTEL, 1997, "pentium-mmx pentium2")
vend(V_INTEL, 1995, "i686 pentiumpro")
vend(V_INTEL, 1999, "pentium3 pentium3m")
vend(V_INTEL, 2000, "pentium4 pentium4m")
vend(V_INTEL, 2003, "pentium-m")
vend(V_INTEL, 2004, "prescott nocona")
vend(V_INTEL, 2006, "yonah core2")
vend(V_INTEL, 2007, "penryn")
vend(V_INTEL, 2008, "nehalem corei7 bonnell atom")
vend(V_INTEL, 2010, "westmere")
vend(V_INTEL, 2011, "sandybridge corei7-avx")
vend(V_INTEL, 2012, "ivybridge core-avx-i")
vend(V_INTEL, 2013, "haswell core-avx2 silvermont slm lakemont")
vend(V_INTEL, 2014, "broadwell")
vend(V_INTEL, 2015, "skylake")
vend(V_INTEL, 2016, "goldmont knl")
vend(V_INTEL, 2017, "skylake-avx512 skx goldmont-plus knm")
vend(V_INTEL, 2018, "cannonlake")
vend(V_INTEL, 2019, "cascadelake icelake-client")
vend(V_INTEL, 2020, "cooperlake tigerlake tremont")
vend(V_INTEL, 2021, "icelake-server rocketlake alderlake gracemont")
vend(V_INTEL, 2022, "raptorlake")
vend(V_INTEL, 2023, "sapphirerapids emeraldrapids meteorlake")
vend(V_INTEL, 2024, "graniterapids arrowlake arrowlake-s lunarlake sierraforest grandridge")
vend(V_INTEL, 2025, "graniterapids-d pantherlake clearwaterforest")
vend(V_INTEL, 2026, "diamondrapids novalake wildcatlake")
vend(V_AMD, 1997, "k6")
vend(V_AMD, 1998, "k6-2")
vend(V_AMD, 1999, "k6-3 athlon athlon-tbird")
vend(V_AMD, 2001, "athlon-4 athlon-xp athlon-mp")
vend(V_AMD, 2003, "k8 opteron athlon64 athlon-fx geode")
vend(V_AMD, 2005, "k8-sse3 opteron-sse3 athlon64-sse3")
vend(V_AMD, 2007, "amdfam10 barcelona")
vend(V_AMD, 2011, "btver1 bdver1")
vend(V_AMD, 2012, "bdver2")
vend(V_AMD, 2013, "btver2")
vend(V_AMD, 2014, "bdver3")
vend(V_AMD, 2015, "bdver4")
vend(V_AMD, 2017, "znver1")
vend(V_AMD, 2019, "znver2")
vend(V_AMD, 2020, "znver3")
vend(V_AMD, 2022, "znver4")
vend(V_AMD, 2024, "znver5")
vend(V_AMD, 2026, "znver6")
vend(V_HYGON, 2018, "c86-4g-m4")
vend(V_HYGON, 2020, "c86-4g-m6")
vend(V_HYGON, 2023, "c86-4g-m7")
vend(V_HYGON, 2025, "c86-4g-m8")
vend(V_CENTAUR, 1997, "winchip-c6")
vend(V_CENTAUR, 1998, "winchip2")
vend(V_CENTAUR, 2000, "samuel-2")
vend(V_CENTAUR, 2001, "c3")
vend(V_CENTAUR, 2003, "c3-2 nehemiah")
vend(V_CENTAUR, 2005, "c7 esther")
vend(V_CENTAUR, 2008, "nano nano-1000 nano-2000")
vend(V_CENTAUR, 2009, "nano-3000 eden-x2")
vend(V_CENTAUR, 2011, "nano-x2")
vend(V_CENTAUR, 2013, "eden-x4 nano-x4")
vend(V_ZHAOXIN, 2019, "lujiazui")
vend(V_ZHAOXIN, 2021, "yongfeng")
vend(V_ZHAOXIN, 2024, "shijidadao")
vend(V_GENERIC, 2003, "x86-64")
vend(V_GENERIC, 2009, "x86-64-v2")
vend(V_GENERIC, 2013, "x86-64-v3")
vend(V_GENERIC, 2017, "x86-64-v4")

# names that are not -march targets
SKIP = {"generic", "intel", "native", ""}

def die(msg):
  sys.stderr.write("gen_targets: " + msg + "\n")
  sys.exit(1)

def find(base, rel):
  for p in (os.path.join(base, rel), os.path.join(base, os.path.basename(rel))):
    if os.path.isfile(p): return open(p, encoding="utf-8", errors="replace").read()
  die("no %s in %s" % (rel, base))

def strip_comments(s):
  s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
  return re.sub(r"//[^\n]*", " ", s)

# ---- set expressions: | & ^ ~ ( ), names, {} and {X86::FEATURE_X}
def tokenize(e):
  toks = re.findall(r"\{[^}]*\}|[A-Za-z_][A-Za-z0-9_:]*|[|&^~()]", e)
  rest = re.sub(r"\{[^}]*\}|[A-Za-z_][A-Za-z0-9_:]*|[|&^~()]|\s", "", e)
  if rest: die("cannot parse expression: " + e)
  return toks

def evaluate(e, lookup, universe):
  toks = tokenize(e); pos = [0]
  def peek(): return toks[pos[0]] if pos[0] < len(toks) else None
  def take(): pos[0] += 1; return toks[pos[0]-1]
  def atom():
    t = take()
    if t == "(":
      v = expr_or()
      if take() != ")": die("unbalanced: " + e)
      return v
    if t == "~": return universe - atom()
    if t.startswith("{"): return lookup(t)
    return lookup(t)
  def expr_and():
    v = atom()
    while peek() == "&": take(); v = v & atom()
    return v
  def expr_xor():
    v = expr_and()
    while peek() == "^": take(); v = v ^ expr_and()
    return v
  def expr_or():
    v = expr_xor()
    while peek() == "|": take(); v = v | expr_xor()
    return v
  v = expr_or()
  if pos[0] != len(toks): die("trailing tokens: " + e)
  return v

# ---- LLVM
def parse_llvm(d):
  defs = strip_comments(find(d, "llvm/include/llvm/TargetParser/X86TargetParser.def"))
  enum2str = {}
  for m in re.finditer(r"^\s*X86_FEATURE(?:_COMPAT)?\s*\(\s*(\w+)\s*,\s*\"([^\"]*)\"", defs, re.M):
    enum2str[m.group(1)] = m.group(2)
  def canon(s):
    if s in LLVM_DROP: return None
    s = LLVM_RENAME.get(s, s)
    if s not in FIDX: die("LLVM feature '%s' is not in FEATURES" % s)
    return s
  src = strip_comments(find(d, "llvm/lib/TargetParser/X86TargetParser.cpp"))
  sets = {}
  U = frozenset(enum2str.keys())
  def lookup(t):
    if t.startswith("{"):
      inner = t[1:-1].strip()
      if not inner: return frozenset()
      m = re.match(r"(?:X86::)?FEATURE_(\w+)$", inner)
      if not m: die("bad set literal " + t)
      return frozenset([m.group(1)])
    if t.startswith("Feature") and not t.startswith("Features") and t[7:] in enum2str:
      return frozenset([t[7:]])
    if t in sets: return sets[t]
    die("unknown LLVM name " + t)
  for m in re.finditer(r"(?:static\s+)?constexpr\s+FeatureBitset\s+(\w+)\s*=\s*([^;]*);", src):
    name, e = m.group(1), m.group(2)
    if name.startswith("Feature") and name[7:] in enum2str: continue
    sets[name] = evaluate(e, lookup, U)
  implied = {}
  for k, v in sets.items():
    if k.startswith("ImpliedFeatures"): implied[k[len("ImpliedFeatures"):]] = v
  body = src[src.index("ProcessorDefs[]"):]
  body = body[:body.index("};")]
  procs = []
  for m in re.finditer(r"\{\s*\{\"([^\"]*)\"\}\s*,\s*\{\s*(CK_\w+)\s*,\s*([^,]+?)\s*,\s*([^,]+?)\s*,\s*'([^']*)'\s*,\s*(true|false)\s*\}\s*\}", body):
    name, kind, key, fe, mang, only = m.groups()
    if only == "true" or name in SKIP: continue
    procs.append((name, kind, evaluate(fe, lookup, U)))
  if len(procs) < 50: die("too few LLVM processors parsed (%d)" % len(procs))
  # closure of the implied features, then canonical names
  def close(s):
    s = set(s)
    while True:
      n = set(s)
      for f in s: n |= implied.get(f, frozenset())
      if n == s: break
      s = n
    return s
  out = {}
  for name, kind, s in procs:
    c = set()
    for f in close(s):
      cf = canon(enum2str[f])
      if cf: c.add(cf)
    out[name] = (kind, c)
  # X86.td: scheduling models, tunings, and the inline feature lists of the
  # old CPUs (the backend adds those to what the TargetParser table says:
  # cmov on athlon, ...)
  td = strip_comments(find(d, "llvm/lib/Target/X86/X86.td"))
  td = td[td.index("// X86 processors supported.") if "// X86 processors supported." in td else 0:]
  model, alias = {}, {}
  TDF = {"FeatureX87": "x87", "FeatureCX8": "cx8", "FeatureCMOV": "cmov", "FeatureMMX": "mmx",
         "FeatureSSE1": "sse", "FeatureSSE2": "sse2", "FeatureSSE3": "sse3", "FeatureSSSE3": "ssse3",
         "FeatureSSE41": "sse4.1", "FeatureFXSR": "fxsr", "FeaturePRFCHW": "prfchw",
         "FeatureX86_64": "64bit", "FeatureLAHFSAHF64": "sahf"}
  # (not FeatureCX16: X86.td gives it to k8-sse3, which K8 rev. E lacks; the
  # backend only uses it for 128-bit atomics, which clang's front end does not
  # emit inline where its own table (TargetParser) has no cx16)
  tdfeat = {}
  def add_model(n, mod, feats, tune):
    model[n] = (mod, re.sub(r"\s+", " ", tune).strip(), re.sub(r"\s+", " ", feats).strip())
  for m in re.finditer(r"def\s*:\s*ProcModel\s*<\s*\"([^\"]+)\"\s*,\s*(\w+)\s*,\s*(\[[^\]]*\]|[\w.]+)\s*,\s*(\[[^\]]*\]|[\w.]+)\s*>", td, re.S):
    add_model(m.group(1), m.group(2), m.group(3), m.group(4))
    if m.group(3).startswith("["): tdfeat[m.group(1)] = re.findall(r"\w+", m.group(3))
  for m in re.finditer(r"def\s*:\s*Proc\s*<\s*\"([^\"]+)\"\s*,\s*(\[[^\]]*\]|[\w.]+)\s*,\s*(\[[^\]]*\]|[\w.]+)\s*>", td, re.S):
    add_model(m.group(1), "GenericModel", m.group(2), m.group(3))
    if m.group(2).startswith("["): tdfeat[m.group(1)] = re.findall(r"\w+", m.group(2))
  for m in re.finditer(r"foreach\s+P\s*=\s*\[([^\]]*)\]\s*in\s*\{\s*def\s*:\s*ProcModel\s*<\s*P\s*,\s*(\w+)\s*,\s*([\w.]+)\s*,\s*([\w.]+)\s*>", td, re.S):
    for n in re.findall(r"\"([^\"]+)\"", m.group(1)): add_model(n, m.group(2), m.group(3), m.group(4))
  for m in re.finditer(r"def\s*:\s*ProcessorAlias\s*<\s*\"([^\"]+)\"\s*,\s*\"([^\"]+)\"\s*>", td):
    alias[m.group(1)] = m.group(2)
  res = {}
  for name, (kind, c) in out.items():
    n = name
    for _ in range(3):
      if n in model: break
      if n in alias: n = alias[n]
      elif n.replace("-", "_") in model: n = n.replace("-", "_")
      elif n.replace("-", "_") in alias: n = alias[n.replace("-", "_")]
    if n not in model: die("no X86.td model for " + name)
    for f in tdfeat.get(n, []):
      if f in TDF: c.add(TDF[f])
    # (the CPUKind only sets predefined macros, so it is not part of the key)
    # (x86-64-v2..v4 are in clang's NoTuneList, but that only rejects them
    # for -mtune: with -march=x86-64-v3 the backend still tunes by X86.td's
    # x86-64-v3, HaswellModel with generic tuning flags)
    res[name] = {"set": c, "model": model[n][0], "key": model[n], "notune": False}
  return res

# ---- GCC
def parse_gcc(d):
  isa = strip_comments(find(d, "gcc/config/i386/i386-isa.def"))
  pta_names = re.findall(r"DEF_PTA\s*\(\s*(\w+)\s*\)", isa)
  if len(pta_names) < 50: die("too few DEF_PTA in i386-isa.def")
  U = frozenset(pta_names)
  h = strip_comments(find(d, "gcc/config/i386/i386.h"))
  sets = {}
  def lookup(t):
    if t.startswith("PTA_") and t[4:] in U: return frozenset([t[4:]])
    if t in sets: return sets[t]
    die("unknown GCC name " + t)
  for m in re.finditer(r"constexpr\s+wide_int_bitmask\s+(PTA_\w+)\s*=\s*([^;]*);", h):
    sets[m.group(1)] = evaluate(m.group(2), lookup, U)
  cc = strip_comments(find(d, "gcc/common/config/i386/i386-common.cc"))
  i = cc.index("processor_alias_table[]")
  i = cc.index("{", i)
  depth, j, items, start = 0, i, [], None
  while True:
    ch = cc[j]
    if ch == "{":
      depth += 1
      if depth == 2: start = j + 1
    elif ch == "}":
      depth -= 1
      if depth == 1: items.append(cc[start:j])
      if depth == 0: break
    j += 1
  res = {}
  for it in items:
    parts, dep, cur = [], 0, ""
    for ch in it:
      if ch == "(": dep += 1
      if ch == ")": dep -= 1
      if ch == "," and dep == 0: parts.append(cur.strip()); cur = ""
      else: cur += ch
    parts.append(cur.strip())
    if len(parts) != 6: die("bad alias table entry: " + it)
    name = parts[0].strip('"')
    proc = parts[1]
    # "generic", "intel" (-mtune only) and the __builtin_cpu_is names after
    # them ("amd", "amdfam10h" ...) are not -march targets
    if name in SKIP or proc in ("PROCESSOR_GENERIC", "PROCESSOR_INTEL"): continue
    flags = evaluate(parts[3], lookup, U) if parts[3] not in ("0",) else frozenset()
    c = set()
    for f in flags:
      if f in GCC_DROP: continue
      cf = GCC_RENAME.get(f)
      if cf is None:
        cf = f.lower().replace("_", "-") if f.startswith("AMX_") else f.lower()
      if cf not in FIDX: die("GCC flag PTA_%s is not in FEATURES" % f)
      c.add(cf)
    if "ABM" in flags: c |= {"lzcnt", "popcnt"}
    if "NO_80387" not in flags: c.add("x87")
    if "64BIT" in flags and "NO_SAHF" not in flags: c.add("sahf")
    # gcc's arch features (initial_ix86_arch_features): cmov and cmpxchg8b
    # by processor
    if proc not in ("PROCESSOR_I386", "PROCESSOR_I486"): c.add("cx8")
    if proc not in ("PROCESSOR_I386", "PROCESSOR_I486", "PROCESSOR_PENTIUM",
                    "PROCESSOR_LAKEMONT", "PROCESSOR_K6"): c.add("cmov")
    if "PREFETCH_SSE" in flags or "SSE" in flags: c.add("mmxext")
    # gcc tunes -march=x86-64 (and the levels, PTA_NO_TUNE) as generic
    # (ix86_option_override_internal)
    notune = ("NO_TUNE" in flags) or (name == "x86-64")
    res[name] = {"set": c, "proc": "generic" if notune else proc[len("PROCESSOR_"):].lower(),
                 "key": (proc, notune, parts[2]), "notune": notune}
  return res

# implications applied to both compilers' sets (LLVM's ImpliedFeatures, with
# canonical names, and a few that only one of them spells out)
IMPLY = {
  "sse2": "sse", "sse3": "sse2", "ssse3": "sse3", "sse4.1": "ssse3", "sse4.2": "sse4.1 crc32",
  "avx": "sse4.2", "avx2": "avx", "avx512f": "avx2 f16c fma", "f16c": "avx", "fma": "avx",
  "fma4": "avx sse4a", "xop": "fma4", "sse4a": "sse3", "aes": "sse2", "pclmul": "sse2",
  "gfni": "sse2", "sha": "sse2", "vaes": "aes avx2", "vpclmulqdq": "avx pclmul",
  "avx512cd": "avx512f", "avx512bw": "avx512f", "avx512dq": "avx512f", "avx512vl": "avx512f",
  "avx512bf16": "avx512bw", "avx512bitalg": "avx512bw", "avx512bmm": "avx512bw",
  "avx512ifma": "avx512f", "avx512vnni": "avx512f", "avx512vpopcntdq": "avx512f",
  "avx512vbmi": "avx512bw", "avx512vbmi2": "avx512bw", "avx512vp2intersect": "avx512f",
  "avx512fp16": "avx512bw", "avxvnni": "avx2", "avxifma": "avx2", "avxvnniint8": "avx2",
  "avxvnniint16": "avx2", "avxneconvert": "avx2", "sha512": "avx2", "sm3": "avx", "sm4": "avx2",
  "avx10.1": "avx512cd avx512vbmi avx512ifma avx512vnni avx512bf16 avx512vpopcntdq "
             "avx512vbmi2 avx512bitalg avx512fp16 avx512dq avx512vl avx512bw",
  "avx10.2": "avx10.1", "xsaveopt": "xsave", "xsavec": "xsave", "xsaves": "xsave",
  "widekl": "kl", "3dnowa": "3dnow", "3dnow": "mmx", "amx-int8": "amx-tile",
  "amx-bf16": "amx-tile", "amx-fp16": "amx-tile", "amx-complex": "amx-tile",
  "amx-fp8": "amx-tile", "amx-movrs": "amx-tile", "amx-avx512": "amx-tile avx10.2",
  "64bit": "x87 cx8 cmov mmx fxsr sse sse2 mmxext",
  "sse": "mmxext",
}

def normalize(s):
  s = set(s)
  while True:
    n = set(s)
    for f in s: n |= set(IMPLY.get(f, "").split())
    if n == s: break
    s = n
  s.discard("avx10.1")     # no instructions of its own: its avx512 set stands for it
  return s

def needs(f):
  s, todo = set(), list(IMPLY.get(f, "").split())
  while todo:
    g = todo.pop()
    if g not in s: s.add(g); todo += IMPLY.get(g, "").split()
  s.discard(f)
  return s

def mask(s):
  w = [0] * ((len(FEATURES) + 63) // 64)
  for f in s:
    i = FIDX[f]; w[i // 64] |= 1 << (i % 64)
  return "{" + ",".join("0x%016XULL" % x for x in w) + "}"

def main():
  if len(sys.argv) < 3:
    sys.stderr.write("usage: gen_targets.py LLVM_DIR GCC_DIR [x86targets.h]\n")
    sys.exit(1)
  llvm, gcc = parse_llvm(sys.argv[1]), parse_gcc(sys.argv[2])
  for t in (llvm, gcc):
    for v in t.values(): v["set"] = normalize(v["set"])
  # order: LLVM's table order, then the gcc-only names in gcc's order
  names = list(llvm.keys()) + [n for n in gcc.keys() if n not in llvm]
  # groups
  ckey = lambda n: (llvm[n]["key"], frozenset(llvm[n]["set"])) if n in llvm else None
  gkey = lambda n: (gcc[n]["key"], frozenset(gcc[n]["set"])) if n in gcc else None
  group = {}
  ng = 0
  for n in names:
    if n not in llvm or n not in gcc: continue
    for m in names:
      if m in group and m in llvm and m in gcc and ckey(m) == ckey(n) and gkey(m) == gkey(n):
        group[n] = group[m]; break
    else:
      group[n] = ng; ng += 1
  # a name only one compiler knows joins the group whose names that compiler
  # treats the same (skx -> skylake-avx512), when there is exactly one such
  for n in names:
    if n in group: continue
    cand = set()
    for m in list(group):
      if (n in llvm and m in llvm and ckey(m) == ckey(n)) or (n in gcc and m in gcc and gkey(m) == gkey(n)):
        cand.add(group[m])
    if len(cand) == 1: group[n] = cand.pop()
    else: group[n] = ng; ng += 1
  # renumber groups by first appearance
  ren = {}
  for n in names:
    if group[n] not in ren: ren[group[n]] = len(ren)
  for n in names: group[n] = ren[group[n]]
  models = []
  for n in names:
    md = "generic" if (n in llvm and llvm[n]["notune"]) else (llvm[n]["model"] if n in llvm else "")
    if md and md not in models: models.append(md)
  procs = []
  for n in names:
    if n in gcc and gcc[n]["proc"] not in procs: procs.append(gcc[n]["proc"])
  # report
  rep = []
  for n in names:
    if n in llvm and n in gcc and llvm[n]["set"] != gcc[n]["set"]:
      a, b = llvm[n]["set"] - gcc[n]["set"], gcc[n]["set"] - llvm[n]["set"]
      cc = lambda s: sorted(f for f in s if FEATURES[FIDX[f]][2] == "C")
      ii = lambda s: sorted(f for f in s if FEATURES[FIDX[f]][2] == "I")
      rep.append("  %-16s clang only: %s%s; gcc only: %s%s" % (n, " ".join(cc(a)), (" [" + " ".join(ii(a)) + "]") if ii(a) else "",
                 " ".join(cc(b)), (" [" + " ".join(ii(b)) + "]") if ii(b) else ""))
  sys.stderr.write("targets: %d (%d clang, %d gcc, %d both), %d groups\n" % (len(names), len(llvm), len(gcc),
                   len([n for n in names if n in llvm and n in gcc]), len(ren)))
  sys.stderr.write("clang only: %s\n" % " ".join(n for n in names if n not in gcc))
  sys.stderr.write("gcc only: %s\n" % " ".join(n for n in names if n not in llvm))
  sys.stderr.write("same name, different features ([..]: intrinsics-only class):\n" + "\n".join(rep) + "\n")
  for n in names:
    if n not in VENDOR: sys.stderr.write("warning: no vendor/year for %s\n" % n)
  # output
  o = []
  o.append("// generated by gen_targets.py on %s from LLVM's X86TargetParser.cpp/X86.td and" % time.strftime("%Y-%m-%d"))
  o.append("// GCC's i386.h/i386-common.cc; edit gen_targets.py, not this, and rerun it")
  o.append("enum {")
  o.append("  " + ", ".join("F_" + f[0] for f in FEATURES) + ",")
  o.append("  N_FEAT, FW = (N_FEAT+63)/64 };")
  o.append("struct FSet { qword w[FW]; };")
  o.append("struct X86Feat { const char* name; char cls; const char* gcc; const char* clang; };")
  o.append("static const X86Feat x86_feat[N_FEAT] = {")
  q = lambda s: ('"%s"' % s) if s else "0"
  for i in range(0, len(FEATURES), 3):
    o.append("  " + " ".join("{ %s, '%s', %s, %s }," % (q(f[1]), f[2], q(f[3]), q(f[4])) for f in FEATURES[i:i+3]))
  o.append("};")
  o.append("// what each feature needs (the implied ones): a cpu without them has it off")
  o.append("static const FSet x86_needs[N_FEAT] = {")
  for i in range(0, len(FEATURES), 2):
    o.append("  " + " ".join("%s," % mask(needs(f[1])) for f in FEATURES[i:i+2]))
  o.append("};")
  o.append("enum { V_GENERIC, V_INTEL, V_AMD, V_HYGON, V_CENTAUR, V_ZHAOXIN };")
  o.append("// models: LLVM's SchedMachineModel")
  o.append("static const char* const x86_model[] = { %s };" % ", ".join(q(m) for m in models))
  o.append("// gcc's PROCESSOR_ (its tuning; \"generic\": PTA_NO_TUNE)")
  o.append("static const char* const x86_gproc[] = { %s };" % ", ".join(q(p) for p in procs))
  o.append("enum { T_GCC=1, T_CLANG=2, T_64=4 };")
  o.append("struct X86Target { const char* name; int flags, vendor, year, group, model, gproc; FSet g, c; };")
  o.append("// name, flags, vendor, year, group, model (-1: not clang), gproc (-1: not gcc), gcc set, clang set")
  o.append("static const X86Target x86_target[] = {")
  for n in names:
    fl = []
    if n in gcc: fl.append("T_GCC")
    if n in llvm: fl.append("T_CLANG")
    if "64bit" in (llvm[n]["set"] if n in llvm else gcc[n]["set"]): fl.append("T_64")
    v, y = VENDOR.get(n, (V_GENERIC, 0))
    md = ("generic" if llvm[n]["notune"] else llvm[n]["model"]) if n in llvm else None
    o.append("  { %-20s %-21s %d, %d, %3d, %2s, %2s, %s, %s }," % (q(n) + ",", "|".join(fl) + ",", v, y, group[n],
             models.index(md) if md else -1, procs.index(gcc[n]["proc"]) if n in gcc else -1,
             mask(gcc[n]["set"]) if n in gcc else mask(()), mask(llvm[n]["set"]) if n in llvm else mask(())))
  o.append("};")
  o.append("enum { N_TARGET = sizeof(x86_target)/sizeof(x86_target[0]) };")
  text = "\n".join(o) + "\n"
  if len(sys.argv) > 3:
    p = sys.argv[3]
    s = open(p, encoding="utf-8").read()
    b, e = "//---- generated: begin\n", "//---- generated: end\n"
    if b not in s or e not in s: die("no generated markers in " + p)
    s = s[:s.index(b) + len(b)] + text + s[s.index(e):]
    open(p, "w", encoding="utf-8", newline="\n").write(s)
    md = os.path.join(os.path.dirname(os.path.abspath(p)), "TARGETS.md")
    open(md, "w", encoding="utf-8", newline="\n").write(targets_md(names, llvm, gcc, group, rep))
  else:
    sys.stdout.write(text)

# ---- TARGETS.md: the table for reading
LEVELS = ["64bit cmov cx8 x87 mmx sse sse2", "cx16 sahf popcnt sse3 sse4.1 sse4.2 ssse3",
          "avx avx2 bmi bmi2 f16c fma lzcnt movbe", "avx512f avx512bw avx512cd avx512dq avx512vl"]
TIERS = [("apx", "apxf"), ("apx", "avx10.2"), ("avx512", "avx512f"), ("avx2", "avx2"), ("avx", "avx"),
         ("sse4.2", "sse4.2"), ("sse4.1", "sse4.1"), ("ssse3", "ssse3"), ("sse3", "sse3"), ("sse2", "sse2"),
         ("sse", "sse"), ("mmx", "mmx")]

def level_of(s):
  base, l = set(), 0
  for i, lv in enumerate(LEVELS):
    if not set(lv.split()) <= s: break
    base |= set(lv.split()); l = i + 1
  return l, base

def tier_of(s):
  for name, f in TIERS:
    if f in s: return name
  return "x87"

def targets_md(names, llvm, gcc, group, rep):
  o = ["# x86 -march targets", "",
       "Generated by gen_targets.py on %s from LLVM's X86TargetParser.cpp/X86.td (llvm-project main)" % time.strftime("%Y-%m-%d"),
       "and GCC's i386.h/i386-common.cc (gcc master); x86targets.h has the same table. One row per",
       "group of names that compile to the same thing in every compiler that knows them.", "",
       "* names: `(gcc)`/`(clang)` marks a name only that compiler takes",
       "* x64: the name is valid for x64 builds (all are valid for x86-32 builds)",
       "* level: x86-64 psABI level its features reach; isa: its top vector ISA",
       "* beyond: the compiler-emitted (class C) features it has beyond that level's",
       "  (gcc and clang together); README.md has the classes",
       "* clang model: LLVM's scheduling model; gcc tuning: GCC's PROCESSOR_ type",
       "  (`generic`: gcc tunes the name as -mtune=generic)", "",
       "| names | x64 | level | isa | beyond | clang model | gcc tuning | year |",
       "|---|---|---|---|---|---|---|---|"]
  seen = set()
  for n in names:
    g = group[n]
    if g in seen: continue
    seen.add(g)
    mem = [m for m in names if group[m] == g]
    u = set()
    for m in mem:
      if m in llvm: u |= llvm[m]["set"]
      if m in gcc: u |= gcc[m]["set"]
    lv, base = level_of(u)
    if not lv: base = {"x87"}
    beyond = sorted((f for f in u - base if FEATURES[FIDX[f]][2] == "C" and f not in ("mmxext",)), key=lambda f: FIDX[f])
    lab = " ".join(m + ("" if (m in llvm and m in gcc) else "(clang)" if m in llvm else "(gcc)") for m in mem)
    cm = sorted(set(("generic" if llvm[m]["notune"] else llvm[m]["model"]) for m in mem if m in llvm))
    gp = sorted(set(gcc[m]["proc"] for m in mem if m in gcc))
    o.append("| %s | %s | %s | %s | %s | %s | %s | %s |" % (lab, "yes" if "64bit" in u else "-", ("v%d" % lv) if lv else "-",
             tier_of(u), " ".join(beyond), " ".join(cm) or "-", " ".join(gp) or "-", VENDOR.get(n, (0, ""))[1]))
  o += ["", "## Same name, different features in gcc and clang", "",
        "Class I features (intrinsics only) in brackets. A name counts as usable (with `-c both`) only",
        "when the features of every compiler that knows it are there.", "", "```"]
  o += [r.strip() for r in rep]
  o += ["```", ""]
  return "\n".join(o)

if __name__ == "__main__": main()
