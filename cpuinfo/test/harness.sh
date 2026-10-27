#!/bin/sh
# harness.sh GCC_SRC HOST_CPP LLVM_PREFIX [OUT]
#
# Builds, in OUT (default: here), the compilers' own -march=native code fed
# from CPUID dumps instead of the CPU:
#   gcc_harness   gcc's host_detect_local_cpu (GCC_SRC/gcc/config/i386/
#                 driver-i386.cc over common/config/i386/cpuinfo.h)
#   llvm_harness  LLVM's x86 getHostCPUName/getHostCPUFeatures (HOST_CPP:
#                 llvm/lib/TargetParser/Host.cpp of the version to test;
#                 LLVM_PREFIX: an LLVM install of it with include/ and
#                 lib/libLLVMSupport.a, e.g. an LLVM-*-Linux-X64 release)
# and the comparers cmp_gcc, cmp_llvm, dumptab. run.sh runs them.
# CPUID and XGETBV are replaced by dump lookups; nothing else is changed.
set -e
G=$(cd "$1" && pwd)/gcc; HOST=$(cd "$(dirname "$2")" && pwd)/$(basename "$2"); L=$(cd "$3" && pwd)
HERE=$(cd "$(dirname "$0")" && pwd); OUT=${4:-$HERE}; mkdir -p "$OUT"; cd "$OUT"
CXX=${CXX:-g++}
python3 - "$G" "$HOST" <<'EOF'
import re, sys
G, host = sys.argv[1], sys.argv[2]
ci = open(G + '/common/config/i386/cpuinfo.h').read()
ci, k = re.subn(r'__asm__ \("\.byte 0x0f, 0x01, 0xd0"\s*:\s*"=a" \(xcrlow\), "=d" \(xcrhigh\)\s*:\s*"c" \(XCR_XFEATURE_ENABLED_MASK\)\);',
                'dump_xgetbv (&xcrlow, &xcrhigh);', ci)
assert k == 1, "cpuinfo.h: xgetbv not found"
open('cpuinfo_patched.h', 'w').write(ci)
dr = open(G + '/config/i386/driver-i386.cc').read()
open('driver_body.inc', 'w').write(dr[dr.index('struct cache_desc'):dr.index('#else\n\n/* If we are compiling with GCC where %EBX')])
h = open(G + '/config/i386/i386.h').read()
open('processor_type.inc', 'w').write(re.search(r'enum processor_type\s*\{.*?\};', h, re.S).group(0) + '\n')
h = open(host).read()
st = [m.start() for m in re.finditer(r'#if \(defined\(__i386__\) \|\| defined\(_M_IX86\) \|\| defined\(__x86_64__\) \|\|', h)]
p1 = h[st[0]:h.index('#elif defined(_M_ARM64) || defined(_M_ARM64EC)', st[0])]
p2 = h[st[1]:h.index('#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))', st[1])]
p1, k = re.subn(r'__asm__\("\.byte 0x0f, 0x01, 0xd0" : "=a"\(\*rEAX\), "=d"\(\*rEDX\) : "c"\(0\)\);', 'dump_xgetbv(rEAX, rEDX);', p1)
assert k == 1, "Host.cpp: xgetbv not found"
open('host_part1.inc', 'w').write(p1 + '\n#endif\n')
open('host_part2.inc', 'w').write(p2 + '\n#endif\n')
EOF
# dump lookups shared by both: cpuid.h's semantics (0 beyond the max leaf)
cat > dumpcpuid.inc <<'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include "x86targets.h"
static x86::Cpuid D;
static void dump_cpuid( unsigned l, unsigned s, unsigned* a, unsigned* b, unsigned* c, unsigned* d ) {
  unsigned r[4]; if( l>D.R( l&0x80000000u, 0, 0 ) ) r[0]=r[1]=r[2]=r[3]=0; else D.Get( l, s, r );
  *a=r[0]; *b=r[1]; *c=r[2]; *d=r[3]; }
static void dump_xgetbv( unsigned* lo, unsigned* hi ) { *lo = unsigned(D.xcr0); *hi = unsigned(D.xcr0>>32); }
static int my_get_cpuid_max( unsigned ext, unsigned* sig ) { unsigned a,b,c,d; dump_cpuid( ext, 0, &a,&b,&c,&d ); if( sig ) *sig = b; return a; }
static int my_get_cpuid( unsigned l, unsigned* a, unsigned* b, unsigned* c, unsigned* d ) {
  if( unsigned(my_get_cpuid_max( l&0x80000000u, 0 ))<l ) return 0; dump_cpuid( l, 0, a,b,c,d ); return 1; }
static int my_get_cpuid_count( unsigned l, unsigned s, unsigned* a, unsigned* b, unsigned* c, unsigned* d ) {
  if( unsigned(my_get_cpuid_max( l&0x80000000u, 0 ))<l ) return 0; dump_cpuid( l, s, a,b,c,d ); return 1; }
static bool LoadDump( const char* p ) {
  FILE* f = fopen( p, "rb" ); if( !f ) return false;
  std::string s; char b[4096]; size_t k; while( (k=fread( b, 1, sizeof(b), f ))>0 ) s.append( b, k ); fclose( f );
  return x86::ParseCpuid( D, s.c_str() ); }
static const char* Base( const char* p ) { const char* b = strrchr( p, '/' ); return b ? b+1 : p; }
EOF
cat > gcc_harness.cc <<EOF
#include "dumpcpuid.inc"
#include "$G/config/i386/cpuid.h"
#undef __cpuid
#undef __cpuid_count
#define __cpuid(l,a,b,c,d) dump_cpuid( l, 0, &a, &b, &c, &d )
#define __cpuid_count(l,s,a,b,c,d) dump_cpuid( l, s, &a, &b, &c, &d )
#define __get_cpuid_max my_get_cpuid_max
#define __get_cpuid my_get_cpuid
#define __get_cpuid_count my_get_cpuid_count
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#define gcc_assert(x) ((void)0)
static char* concat( const char* first, ... ) {
  size_t n = 0; va_list ap; va_start( ap, first );
  for( const char* s=first; s; s=va_arg( ap, const char* ) ) n += strlen( s );
  va_end( ap ); char* r = (char*)malloc( n+1 ); r[0] = 0; va_start( ap, first );
  for( const char* s=first; s; s=va_arg( ap, const char* ) ) strcat( r, s );
  va_end( ap ); return r; }
static int errored;
static void error( const char* ) { errored = 1; }
#include "processor_type.inc"
#include "$G/common/config/i386/i386-cpuinfo.h"
#include "cpuinfo_patched.h"
#include "$G/common/config/i386/i386-isas.h"
#include "driver_body.inc"
int main( int argc, char** argv ) {
  for( int i=1; i<argc; i++ ) {
    if( !LoadDump( argv[i] ) ) continue;
    const char* av[2] = { "arch", "64" };
    errored = 0;
    const char* r = host_detect_local_cpu( 2, av );
    printf( "%s\t%s%s\n", Base( argv[i] ), errored ? "ERROR " : "", r ? r : "(null)" );
  }
}
EOF
cat > llvm_harness.cpp <<'EOF'
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/X86TargetParser.h"
#include <vector>
#include <algorithm>
#include "dumpcpuid.inc"
#define __get_cpuid my_get_cpuid
#define __get_cpuid_count my_get_cpuid_count
namespace llvm { namespace sys {
  StringRef h_getHostCPUName(); StringMap<bool> h_getHostCPUFeatures();
  namespace detail { namespace x86 { VendorSignatures h_getVendorSignature( unsigned* ); } }
} }
#define getHostCPUName h_getHostCPUName
#define getHostCPUFeatures h_getHostCPUFeatures
#define getVendorSignature h_getVendorSignature
using namespace llvm;
#include "host_part1.inc"
#include "host_part2.inc"
int main( int argc, char** argv ) {
  for( int i=1; i<argc; i++ ) {
    if( !LoadDump( argv[i] ) ) continue;
    std::string n = sys::h_getHostCPUName().str();
    StringMap<bool> fm = sys::h_getHostCPUFeatures();
    std::vector<std::string> v;
    for( auto& e : fm ) v.push_back( std::string( e.second ? "+" : "-" ) + e.first().str() );
    std::sort( v.begin(), v.end() );
    printf( "%s\t%s\t", Base( argv[i] ), n.c_str() );
    for( auto& x : v ) printf( "%s ", x.c_str() );
    printf( "\n" );
  }
}
EOF
$CXX -std=c++11 -O1 -w -I"$HERE/.." -I. -o gcc_harness gcc_harness.cc
$CXX -std=c++17 -O1 -w -I"$L/include" -I"$HERE/.." -I. -o llvm_harness llvm_harness.cpp "$L/lib/libLLVMSupport.a" "$L/lib/libLLVMDemangle.a" -lpthread
for t in cmp_gcc cmp_llvm dumptab; do $CXX -std=c++11 -O2 -I"$HERE/.." -o $t "$HERE/$t.cpp"; done
echo "built in $OUT: gcc_harness llvm_harness cmp_gcc cmp_llvm dumptab"
