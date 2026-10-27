# cpuinfo

The CPU, and the gcc/clang `-march` targets for it:

* what the CPU is and has (CPUID), and which of it the OS enables (XCR0);
* the name gcc's and clang's `-march=native` pick for it, and the feature
  flags `-march=native` adds to or takes from that name;
* every `-march` target whose code runs on it, best first; the ones that do
  not run, with what they lack.

x86-32 and x64; Windows and Linux; gcc, clang, MSVC.

| file | |
|---|---|
| `cpuinfo.cpp` | the command line tool |
| `x86targets.h` | everything else, header-only: CPUID, the detection, the target table, the ranking (`namespace x86`) |
| `gen_targets.py` | makes the target table in `x86targets.h` (and `TARGETS.md`) from LLVM's and GCC's sources |
| `fetch_sources.sh` | gets those sources |
| `TARGETS.md` | the target table, for reading |

Contents: [Build](#build) · [Usage](#usage) · [How -march=native works](#how--marchnative-works) ·
[The native name](#the-native-name) · [Targets](#targets) · [Which targets run](#which-targets-run) ·
[Ranking](#ranking) · [In a program](#in-a-program) · [Regenerating the table](#regenerating-the-table) ·
[Testing](#testing) · [Limits](#limits)

## Build

C++11, no dependencies.

```
g++ -std=c++11 -O2 -s -o cpuinfo cpuinfo.cpp                  # Linux (-m32 for x86-32)
x86_64-w64-mingw32-g++ -std=c++11 -O2 -s -static -o cpuinfo.exe cpuinfo.cpp
cl /O2 /EHsc cpuinfo.cpp                                         # MSVC, x64 or x86 prompt
```

`build.sh` builds `cpuinfo`, `cpuinfo32` and, where MinGW-w64 is installed,
`cpuinfo64.exe`/`cpuinfo32.exe`; `build.bat` is the Windows one.

## Usage

```
cpuinfo [options]

  (none)   the CPU, the features the OS lets it use, the names gcc's and
           clang's -march=native pick, and the targets whose code runs here,
           best first
  -l       the usable target names, best first, one per line
  -n       the best target name
  -a       all targets: the usable ones ranked, the others with what they lack
  -t NAME  one target: its features per compiler, and whether it runs here
  -f       the features, one per line: here or not, class, compiler flags
  -c C     the targets of one compiler: gcc or clang (default: both; a name
           counts when its code runs here with every compiler that knows it)
  -32      targets for x86-32 builds (all names; default when the CPU is not 64-bit)
  -64      targets for x64 builds (default)
  -s       strict: also need the features compilers only use by intrinsics
           (aes sha sgx ...: class I), not only those they use on their own
  -d FILE  a CPUID dump (from -D, or AIDA64/InstLatx64 text) instead of this CPU
  -D       dump this CPU's CPUID leaves
```

Exit code: 0; 1 for `-l`/`-n` when no target runs, or a bad option; 2 when
there is no CPUID (or the dump can not be read).

A Zen 4 (from an InstLatx64 dump, Windows: PKU off):

```
cpu       AMD Ryzen 5 7600X 6-Core Processor
          AuthenticAMD, family 0x19 model 0x61 stepping 2 (00A60F12)
caches    L1d 32K, L1i 32K, L2 1M, L3 32M
os        saves sse avx avx512 (xcr0 2E7)
level     x86-64-v4
features  x87 cx8 cmov mmx mmxext fxsr sse sse2 sse3 ssse3 sse4.1 sse4.2 sse4a
          popcnt lzcnt cx16 sahf 64bit crc32 movbe prfchw pclmul aes xsave
          ...
          avx512vpopcntdq avx512bf16 shstk
off       pku (in the CPU; not enabled by the OS)
native    znver4 (gcc and clang)
          gcc   -march=native = -march=znver4 -mno-pku -mshstk
          clang -march=native = -march=znver4 -mno-pku
x64 targets for gcc and clang, best first:
   1  avx512  v4   znver4 [no pku]
   2  avx512  v4   rocketlake [no pku]
   3  avx512  v4   icelake-server [no sgx pku pconfig]
   4  avx512  v4   tigerlake
                   [no sgx pku avx512vp2intersect movdiri movdir64b kl widekl]
   5  avx512  v4   icelake-client [no sgx pku]
   ...
   9  avx512  v4   skylake-avx512 skx(clang) [no sgx pku]
  10  avx512  v4   x86-64-v4
  11  avx512  v3   knm(clang)
  12  avx512  v3   knl(clang)
  13  avx2    v3   znver3 [no pku]
  14  avx2    v3   znver2
  ...
  46  sse2    v1   x86-64
x86-32 builds can also use:
      yonah(clang) c7(gcc) esther(gcc) prescott pentium-m pentium4 pentium4m
      ...
```

* `threads`/cores (from the OS) are shown for the real CPU, not for `-d`;
* a list line is a group of names that compile to the same thing; columns:
  rank, top vector ISA, x86-64 level, names (`(gcc)`/`(clang)`: only that
  compiler takes it), and `[no ...]`: features of the target the CPU lacks
  that compilers only use through intrinsics (see below);
* `-march=native = ...`: what `-march=native` passes is that name plus every
  feature flag; shown are only the flags that differ from the name's own
  set, each by that compiler's own reading of CPUID (see
  [Which targets run](#which-targets-run)). Not modelled: gcc's
  `-mrtm`/`-mhle`/`-mabm`/`-mavx10v2aux`/`-macev1` (or their `-mno-`), clang's
  `avx10v2aux`, and gcc's cache sizes (`--param l1-cache-size=...`) for its
  `-mtune=native`.

`-a` adds the targets that do not run, e.g. on that Zen 4:

```
not usable here:
  alderlake              lacks avxvnni [sgx pku movdiri movdir64b waitpkg ptwrite serialize hreset pconfig kl widekl]
  znver5                 lacks avxvnni [pku avx512vp2intersect movdiri movdir64b prefetchi]
  c86-4g-m7              lacks avxvnni [avx512vp2intersect]
  ...
```

Scripting: `cpuinfo -n` prints the best name; `cpuinfo -l -c gcc` the
usable gcc names in order. A compiler older than the table does not know
every name; keep the ones it takes:

```
for t in $(cpuinfo -l -c gcc); do
  echo 'int x;' | gcc -march=$t -x c -c -o /dev/null - 2>/dev/null && echo $t
done
```

## How -march=native works

`-march=NAME` sets two things: the ISA (the instruction set extensions the
compiler may use) and, unless `-mtune` says otherwise, the tuning. Two AVX2
targets differ in both:

* ISA: `alderlake` has, past `haswell`'s AVX2/BMI2/FMA/MOVBE, also ADX,
  PREFETCHW, GFNI and AVX-VNNI that the compilers emit on their own, and
  more (SHA, VAES, SERIALIZE ...) that intrinsics can use;
* tuning: in clang, the name selects an X86.td processor: a scheduling
  model (latencies, ports, `LoopMicroOpBufferSize`, which sets the unroll
  thresholds; `AlderlakePModel` against `HaswellModel`) and tuning flags
  (`TuningFastVariablePerLaneShuffle`, `TuningPrefer256Bit`, ...); in gcc,
  its `PROCESSOR_` type, which selects a cost table and the `x86-tune.def`
  flags.

So an `alderlake` build can be faster on Alder Lake than a `haswell` one with
the same AVX2. The tuning can be had separately: `-march=x86-64-v3
-mtune=alderlake` keeps the ISA portable. Tuning effects depend on the code
and are often not where the vendor names suggest (another vendor's model
can win); measure.

`-march=native`:

* clang (`clang/lib/Driver/ToolChains/Arch/X86.cpp`): `-target-cpu` is
  `llvm::sys::getHostCPUName()`, and every feature of
  `llvm::sys::getHostCPUFeatures()` is passed as `+f`/`-f`; both are in
  `llvm/lib/TargetParser/Host.cpp`. Where the name comes out "generic"
  (unknown vendor), clang uses its default CPU (`x86-64` for x64) with the
  features.
  No `-tune-cpu` is passed, so the backend tunes for the name.
* gcc (`gcc/config/i386/driver-i386.cc`, `host_detect_local_cpu`, over
  `cpu_indicator_init` of `gcc/common/config/i386/cpuinfo.h`): the driver
  replaces `-march=native` by `-march=NAME` and a `-mFEATURE`/`-mno-FEATURE`
  for every ISA option, and adds `-mtune=native`, which becomes
  `-mtune=NAME` (`generic` for an unknown Intel model) with the cache sizes.

`cpuinfo` ports both name functions (`ClangName()`, `GccName()` in
`x86targets.h`) and reads the features the way both do (`Features()`), so it
shows what `-march=native` gives on a CPU, also for a CPU that is not there
(`-d`).

## The native name

How each compiler names the CPU:

| | clang (`getHostCPUName`) | gcc (`host_detect_local_cpu`) |
|---|---|---|
| Intel | family/model table; unknown family 6 models: a guess by features | family/model table (`get_intel_cpu`); unknown: a guess by features, a different one |
| AMD | family/model table | by features only (avx512bmm: znver6, avx512vp2intersect: znver5, avx512f: znver4, vaes: znver3, clwb: znver2, clzero: znver1, avx2: bdver4 ...) |
| Hygon | family 18h models 4/6/7/8 only, else "generic" | models 4/6/7, 8 and up; else the generic fallback |
| Zhaoxin | "generic" | family 7 models 3Bh/5Bh/6Bh+: lujiazui/yongfeng/shijidadao (lujiazui with AVX and F16C off) |
| VIA, others | "generic" | the generic fallback by features (core2, nocona/prescott, x86-64, pentium4, pentiumpro ...) |

clang tells vendors by the whole 12-byte string, gcc by its first word (ebx)
only: any "Genu..." (GenuineTMx86, GenuineIotel, "Genuine  RDC") is Intel to
gcc, so a Transmeta Crusoe is `pentium-mmx` to it and "generic" to clang.
gcc's driver has VIA (Centaur) cases, but `cpu_indicator_init` sets family
and model only for Intel, AMD, Hygon and Zhaoxin, so for VIA they stay 0
and the generic fallback decides; `cpuinfo` does the same. With no CPUID
leaf 1 gcc says `i386`, clang "generic".

Where they name the same CPU differently (gcc folds more models into one
name; most of these are one group, see [Targets](#targets)): Raptor Lake,
Meteor Lake: clang `raptorlake`, `meteorlake`, gcc `alderlake`; Alder Lake-N:
`gracemont` / `alderlake`; Emerald Rapids: `emeraldrapids` /
`sapphirerapids`; Lunar Lake: `lunarlake` / `arrowlake-s`; Wildcat Lake:
`wildcatlake` / `pantherlake`; Penryn: `penryn` / `core2`; Yonah: `yonah` /
`pentium-m`; Xeon Phi: `knl`/`knm` / none (gcc 15+ stops with an error).

The name comes from the model, the features from CPUID, and they need not
agree; then `-march=NAME` by itself does not run on the CPU, and `cpuinfo`
says so:

```
cpu       Intel(R) Core(TM) Ultra 7 265U
          GenuineIntel, family 0x6 model 0xB5 stepping 0 (000B0650), hybrid
native    arrowlake (gcc and clang)
          gcc   -march=native = -march=arrowlake -mno-sgx -mno-pku -mno-avxifma -mno-avxvnniint8 ...
          (-march=arrowlake by itself does not run here: no avxifma avxvnniint8 avxneconvert)
x64 targets for gcc and clang, best first:
   1  avx2    v3   alderlake raptorlake meteorlake [no sgx pku kl widekl]
```

(model B5h, "Arrow Lake U", is a Meteor Lake refresh, without Arrow Lake's
AVX-IFMA etc.). An unknown model gets a guess: the Bartlett Lake of
InstLatx64's `GenuineIntel00D0670_BartlettLake_01_CPUID.txt` (model D7h) is
`goldmont` to clang (AVX-less, plus every feature as a flag) and
`sierraforest` to gcc, of which neither runs as a bare `-march`; the list
puts `alderlake` first. (The B7h Bartlett Lake is a known model: `raptorlake`.)

## Targets

`x86targets.h` has 123 names (108 clang, 118 gcc; `TARGETS.md` lists them),
generated from llvm-project main and gcc master of 2026-10-27. Per name:

* the ISA features `-march=NAME` enables in each compiler, with the implied
  ones (`avx512bw` brings `avx512f`, `avx2`, `avx` ...): from LLVM's
  `X86TargetParser.cpp` (`ProcessorDefs`, which is what clang's front end
  enables), plus the inline feature lists of the old CPUs in `X86.td` (the
  backend adds those: `cmov` on `athlon`); from GCC's `PTA_` masks in
  `i386.h` and `processor_alias_table` in `i386-common.cc`, with what gcc
  derives by processor (cmov, cmpxchg8b: `initial_ix86_arch_features`), by
  `PTA_ABM` (lzcnt, popcnt), `PTA_NO_SAHF`, `PTA_NO_80387`;
* whether it is a 64-bit target (`64bit` in the set): x64 builds take only
  those; x86-32 builds take all names (gcc and clang take `-m32
  -march=znver4`);
* its tuning: LLVM's scheduling model, GCC's `PROCESSOR_` (`generic` for
  `x86-64` and the `x86-64-vN` levels, which gcc tunes as generic);
* its group: names that compile to the same thing in every compiler that
  knows them (same feature sets, same tuning): `skylake-avx512 skx`,
  `alderlake raptorlake meteorlake`, `k8 athlon64 athlon-fx opteron` ...;
  a name only one compiler knows joins the group that compiler treats the
  same (`skx`, clang only, is clang's `skylake-avx512`). `gracemont` is not
  in the `alderlake` group: clang tunes it differently;
* vendor and year (by hand, in `gen_targets.py`), for the ranking.

`avx10.1` is in the sets (gcc's `graniterapids`, both compilers'
`diamondrapids` and `novalake`), and detected, but not needed for a target
to run: it has no instructions past the AVX-512 set it implies (F CD BW DQ
VL IFMA VBMI VBMI2 VNNI BF16 VPOPCNTDQ BITALG FP16), which stands for it, so
a Granite Rapids that does not enumerate AVX10 still runs `graniterapids`;
`avx10.2` (new instructions) is needed. The APX subfeatures (egpr, ndd, ...)
are one feature, `apxf`, as CPUID has them.
RTM and HLE are left out (in no current `-march` set but gcc's HLE, whose
prefixes older CPUs ignore).

clang only: `yonah penryn skx knl knm`. gcc only: `samuel-2 nehemiah c7 esther
eden-x2 nano nano-1000 nano-2000 nano-3000 nano-x2 eden-x4 nano-x4 lujiazui
yongfeng shijidadao`. Where the same name has different features in the two
compilers is listed at the end of `TARGETS.md`; mostly class I features (gcc
has no `invpcid`, clang adds `shstk` to recent Intel targets), but some are
class C: gcc's `c86-4g-m7`/`m8` have `avxvnni` (clang's do not), clang's
`k6-2`..`k8`, `geode`, `winchip2`, `c3` have `prfchw` where gcc has 3DNow!,
clang's `winchip-c6`/`winchip2`/`c3` have `cx8`, gcc's `geode` has `cmov`.

## Which targets run

A target runs on the CPU when the CPU has the features the target lets the
compiler emit on its own. Features are of two classes (`-f` shows them):

* class C, the compiler emits them by itself: `x87 cx8 cmov mmx mmxext sse
  .. sse4.2 sse4a popcnt lzcnt cx16 sahf 64bit movbe prfchw avx f16c fma fma4
  xop tbm bmi bmi2 adx avx2 gfni avx512*` (but vp2intersect) `avxvnni avxifma
  avxvnniint8 avxvnniint16 avxneconvert avx10.1 avx10.2 apxf raoint avx512bmm`
  (avx10.1 is not needed, see above);
* class I, only through intrinsics/builtins (or used by an OS): `3dnow 3dnowa
  fxsr crc32 pclmul aes xsave* lwp fsgsbase rdrnd rdseed invpcid clflushopt clwb
  clzero mwaitx rdpid rdpru wbnoinvd sha sgx pku vaes vpclmulqdq
  avx512vp2intersect movdiri movdir64b waitpkg cldemote ptwrite serialize
  tsxldtrk enqcmd uintr hreset pconfig kl widekl shstk cmpccxadd prefetchi
  sha512 sm3 sm4 usermsr movrs amx-*`.

A target is usable when it lacks no class C feature. Its missing class I
features are shown as `[no ...]`: code built for it runs unless its source
uses those intrinsics (typically under `#ifdef __AES__` and the like, which
`-march` defines). `-s` (strict) requires both classes. Class I matters
because CPUs often lack parts of their own family's set: SGX is disabled by
many BIOSes and was dropped from later client CPUs, PKU is off under Windows
(the compilers go by OSPKE), hypervisors hide features. Without the split,
`-march=skylake` would not "run" on a Skylake with SGX off.

Some class C choices: `gfni` (LLVM and gcc use GF2P8AFFINEQB for byte shifts
and rotates), `prfchw` (PREFETCHW for `__builtin_prefetch(p,1)`), `f16c` and
`avxneconvert` (half/bf16 conversions), `avxvnni`, `avxifma`, `avxvnniint*`
(LLVM's EVEX-to-VEX compression turns AVX-512 VNNI/IFMA into the VEX forms
where those exist: why a `znver5` build may not run on Zen 4, which has
AVX512-VNNI but not AVX-VNNI). `raoint` and `avx512bmm` are class C only for
safety.

For x86-32 builds the 64-bit-only features (`64bit sahf cx16 apxf uintr`)
are not needed.

The CPU's features (`features` line, `Cpu::f`) are the CPUID bits, with the
registers' OS state from XGETBV (XCR0) for AVX (YMM), AVX-512 (opmask, ZMM),
AMX (tile) and APX, as LLVM's `getHostCPUFeatures` reads them, except:

* `prfchw` is also taken from 3DNow! and from AMD x86-64 (K8 has PREFETCHW
  without the 3DNowPrefetch bit);
* `xsave*` need only OSXSAVE (LLVM: the YMM state), `avx10.*` the AVX-512
  state (LLVM: none);
* `pku` is OSPKE (the OS has it on; both compilers do so), the CPU's own PKU
  bit only shows on the `off` line;
* what lacks its prerequisites is dropped (avx2 without avx ...), which
  neither compiler does to its list.

The names and the `-march=native` lines use each compiler's own reading
(`Cpu::cnat`, `Cpu::gnat`): clang's as LLVM has it; gcc's (`cpuinfo.h`) with
`prfchw` by its bit, `xsave*` and `avx512bmm` with no OS check, Key Locker
only with AESKLE (the OS has it on: off under Linux), `avx10.1` also for an
AVX10 version other than 1 and 2, and AVX/F16C off on a Lujiazui. Which
targets run goes by `f`: a Lujiazui runs AVX targets that gcc's own native
leaves AVX out of.

## Ranking

The usable targets are sorted by, in order:

1. tier: the top vector ISA (apx/avx10.2, avx512, avx2, avx, sse4.2, sse4.1,
   ssse3, sse3, sse2, sse, mmx, x87);
2. exact: the group of a name `-march=native` picks (gcc's or clang's);
3. x86-64 level of the target's features (puts `x86-64-v4` above `knl`, which
   lacks AVX-512 BW/DQ/VL);
4. same tuning: the same LLVM scheduling model or gcc `PROCESSOR_` as a native
   name (`znver4` after `znver5` on Zen 5: both use `Znver4Model`);
5. how many class C features it has;
6. vendor: the CPU's own (2), a related one (1: AMD/Hygon, VIA/Zhaoxin), other
   or generic (0);
7. newer first (the year of the group's first name);
8. strict before not (no missing class I features).

The tier comes first so that a native name of a lower tier (a guess, or a CPU
with more than its name, like an Alder Lake with AVX-512 on) does not shadow
the targets that use more of the CPU; otherwise the native name is first. The
groups are ranked as units, by the union of their names' features (with `-c
both`, gcc's and clang's together).

This is a heuristic: it picks the target that uses most of the CPU's ISA and
is tuned for the closest core it can tell; tuning is not measurable from
CPUID, so for speed, benchmark the first few.

## In a program

```cpp
#include "x86targets.h"

static x86::Cpuid d;             // ~16K: static
static x86::Cpu c;
static x86::Match m[x86::N_TARGET];
if( x86::ReadCpuid( d ) ) {      // or x86::ParseCpuid( d, text ) for a dump
  x86::Analyze( d, c );          // c.f features, c.gcc / c.clang native names, c.level
  int nall, n = x86::RankTargets( c, x86::C_GCC, 64, false, m, &nall );
  for( int i=0; i<n; i++ ) {     // usable, best first
    const char* name = x86::x86_target[m[i].t].name;
    // e.g. pick the first tool_<name>.exe that exists
  }
}
```

`RankTargets( cpu, compilers, bits, strict, out, &count )` returns the number
of usable targets; `out[0..count)` has all targets of those compilers for
those builds, usable ones first. `Match::miss` is what the target lacks
(class C ones make it unusable). Other: `FindTarget()`, `TargetSet()`,
`Has()`, `x86_feat[]` (names, classes, flags), `ClangName()`, `GccName()`,
`PrintCpuid()`. All in `namespace x86`; CPUID/XGETBV by `__cpuidex`/`_xgetbv`
under MSVC, inline asm elsewhere (with the EFLAGS.ID check on x86-32, and an
`ebx`-safe form for 32-bit PIC).

## Regenerating the table

```
./fetch_sources.sh src                              # LLVM and GCC files, from their main branches
python3 gen_targets.py src/llvm src/gcc x86targets.h
```

`gen_targets.py` also takes llvm-project and gcc checkouts. It rewrites the
part of `x86targets.h` between the `generated` lines and `TARGETS.md`, and
prints the gcc/clang differences. A feature it does not know (a new one in
the sources) stops it: add it to `FEATURES` (with its class and flags), and
its CPUID bit to `Features()`; a new name without a vendor/year in
`VENDOR` is a warning. The detection is not generated: compare `ClangName()`
with `getHostCPUName` in `Host.cpp` and `GccName()` with
`host_detect_local_cpu`/`get_intel_cpu` (`fetch_sources.sh` gets those too).

## Testing

Checked against the 590 CPUID dumps of InstLatx64
(<https://github.com/InstLatx64/InstLatx64>, the `*CPUID*.txt` files): all
parse, and the gcc and clang names agree with the CPU names in the file
names (as far as each compiler knows them), with ASan/UBSan clean for every
option. The feature sets were compared with the predefined macros of local
gcc 13 and clang 18 (`-march=X -dM -E`): equal, except where the compilers
changed since (gcc dropped `cldemote` from `alderlake`, LLVM 3DNow! for
`prfchw` and Key Locker from some targets ...). The native
names match `gcc -march=native` and `clang -march=native` on the machine it
was built on. Run against a dump:

```
cpuinfo -d GenuineIntel/GenuineIntel0090672_AlderLake_02_CPUID.txt
```

Dumps without an `XCR0` line are taken as from an OS that enables all the
states leaf 0Dh lists; old AIDA64 dumps without `[SL nn]` give subleaves as
repeated lines of the same leaf, read in order. `cpuinfo -D` writes the
format back, with an `XCR0:` line.

## Limits

* The table is for today's compilers; older ones lack newer names and some
  had other features for the same name (gcc 14 still has `knl`/`knm`; gcc 13
  gives `alderlake` `cldemote`; LLVM before 19 gave `k8` 3DNow!). Class I
  differences do not change which targets run.
* The ranking is a guess at speed, not a measurement.
* A CPU's features may change with the BIOS (AVX-512 on early Alder Lake),
  microcode (TSX), the OS (AVX-512 state, PKU) and hypervisors; the answer is
  for the CPU as the program sees it now. On Linux, AMX also needs an
  `arch_prctl` permission per process, which XCR0 does not show.
* Hybrid CPUs report one feature set for all cores (Intel keeps it uniform);
  the cache sizes are those of the core `cpuinfo` ran on.
