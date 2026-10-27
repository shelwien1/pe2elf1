#!/usr/bin/env python3
# isa_used.py [-c CPUINFO] [-s REGEX] BINARY...
#
# The instruction-set features an x86 ELF binary's code actually uses, per
# function: every instruction of the functions whose names match REGEX
# (default: all) is decoded and classed by the CPUID feature it needs (with
# iced-x86: pip install iced-x86). Features of baseline x86-64 are left out.
# With -c, a feature this CPU lacks according to `CPUINFO -f` is marked "!".
#   isa_used.py -c ./cpuinfo -s MatMul bench.out/bench_gcc_znver5
# Uses nm and objdump (binutils) for the symbols and the section layout.
import re, subprocess, sys
from iced_x86 import Decoder, CpuidFeature

# iced's names -> cpuinfo's
NAMES = { "BMI1": "bmi", "SSE4_1": "sse4.1", "SSE4_2": "sse4.2", "CMPXCHG16B": "cx16",
          "X64": "64bit", "FPU": "x87", "PCLMULQDQ": "pclmul", "PREFETCHW": "prfchw",
          "D3NOW": "3dnow", "D3NOWEXT": "3dnowa", "RDRAND": "rdrnd", "SGX1": "sgx",
          "CET_SS": "shstk", "RAO_INT": "raoint", "WIDE_KL": "widekl", "PREFETCHITI": "prefetchi" }
BASE = { "intel8086", "intel186", "intel286", "intel386", "intel486", "x87", "fpu287", "fpu387",
         "cx8", "cmov", "mmx", "sse", "sse2", "fxsr", "64bit", "cpuid", "multibytenop", "pause",
         "syscall", "tsc", "rdtscp", "clfsh" }

FNAME = {v: NAMES.get( k, k.lower().replace( "_", "" ) )
         for k, v in vars( CpuidFeature ).items() if isinstance( v, int ) and not k.startswith( "_" )}

def Sections( path ):
    out = subprocess.run( ["objdump", "-h", path], capture_output=True, text=True, check=True ).stdout
    secs = []
    for m in re.finditer( r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)", out, re.M ):
        secs.append( (m.group( 1 ), int( m.group( 3 ), 16 ), int( m.group( 2 ), 16 ), int( m.group( 4 ), 16 )) )
    return secs  # name, vma, size, file offset

def Functions( path, rx ):
    out = subprocess.run( ["nm", "-S", "--defined-only", path], capture_output=True, text=True, check=True ).stdout
    for l in out.splitlines():
        p = l.split()
        if len( p ) == 4 and p[2] in "tTwW" and re.search( rx, p[3] ):
            yield p[3], int( p[0], 16 ), int( p[1], 16 )

def Used( path, rx, bits=64 ):
    data = open( path, "rb" ).read(); secs = Sections( path ); r = {}
    for name, addr, size in sorted( Functions( path, rx ), key=lambda x: x[0] ):
        s = [x for x in secs if x[1] <= addr < x[1]+x[2]]
        if not s or size == 0: continue
        off = s[0][3] + addr - s[0][1]
        feats = set(); bad = 0
        for ins in Decoder( bits, data[off:off+size], ip=addr ):
            if ins.is_invalid: bad += 1; continue
            feats |= {FNAME[f] for f in ins.cpuid_features()}
        r[name] = (sorted( feats - BASE ), bad)
    return r

def Here( cpuinfo ):
    out = subprocess.run( [cpuinfo, "-f"], capture_output=True, text=True, check=True ).stdout
    return {p[0]: p[1] == "yes" for p in (l.split() for l in out.splitlines()) if len( p ) >= 2}

def main( a ):
    here = None; rx = ""
    while a and a[0] in ("-c", "-s"):
        if a[0] == "-c": here = Here( a[1] )
        else: rx = a[1]
        a = a[2:]
    if not a: print( "usage: isa_used.py [-c CPUINFO] [-s REGEX] BINARY..." ); return 1
    for path in a:
        for fn, (feats, bad) in Used( path, rx ).items():
            marks = [f + ("!" if here is not None and not here.get( f, True ) else "") for f in feats]
            print( "%s\t%s\t%s%s" % (path, fn, " ".join( marks ) or "-", "  (%d undecoded)" % bad if bad else "") )
    return 0

if __name__ == "__main__":
    sys.exit( main( sys.argv[1:] ) )
