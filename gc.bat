@echo off

set path=C:\VC\link64
rem C:\VC2019\bin\amd64;
set LIB=C:\VC2019\lib\amd64;C:\VC2019\sdk\Lib\x64;C:\VC2019\ucrt\x64
set INCLUDE=

set incs=-DSTRICT -DNDEBUG -DWIN32 -D_WIN32 -I../Lib3 ^
-D_CRT_SECURE_NO_WARNINGS ^
-D_CRT_SECURE_NO_DEPRECATE ^
-D_CRT_DISABLE_PERFCRIT_LOCKS ^
-D_CRT_NONSTDC_NO_DEPRECATE ^
-D_SECURE_SCL=0 ^
-D_ITERATOR_DEBUG_LEVEL=0 ^
-D_SECURE_SCL_THROWS=0 ^
-D_HAS_ITERATOR_DEBUGGING=0 ^
-DCOMMON_SKIP_BSF ^
-DWIN32_LEAN_AND_MEAN ^
-nostdlibinc -nostdinc++ ^
-isystem "C:/clangM10x/lib/clang/22/include" ^
-isystem "C:/VC2019/include" -isystem "C:/VC2019/sdk/include" -isystem "C:/VC2019/ucrt/include"

set opts=-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fgnu-keywords -fstrict-aliasing ^
-Wmsvc-not-found -Wno-nonportable-include-path -Wno-pragma-pack -Wno-ignored-pragma-intrinsic ^
-Wno-ignored-attributes -Wno-expansion-to-defined -fno-stack-clash-protection

set targ=haswell
set arch=-march=%targ% -mtune=%targ% -fms-compatibility -fms-compatibility-version=19 -fms-extensions -Wno-ignored-attributes -m64

set gcc=C:\clangM10x\bin\clang++.exe
set gcc=C:\clangN10x\bin\clang++.exe

del *.exe *.o

set VS2017INSTALLDIR=
set VS140COMNTOOLS=

copy /b C:\VC2019\bin\amd64\link.exe .

rem ---------------------------------------------------------------------------
rem One translation unit: coder0.cpp includes the transformer's sources
rem (tf\*.inc) directly, and every system header lives at the top of coder0.cpp.
rem
rem The transformer's numerics want IEEE semantics, so -Ofast/-ffast-math are
rem NOT used here even though coder0 always was built with them: the weights
rem decoder reproduces CUDA's sinf/cosf bit-exactly to rebuild the RoPE tables,
rem and -ffinite-math-only would delete the isinf test inside it.  -O3 with
rem -ffp-contract=off is what the transformer was validated under.
rem
rem Model switches (see the header of transformer.inc):
rem   set tfdefs=-DTF_TRAIN=3          train the whole model online
rem   set tfdefs=-DTF_LOAD_WEIGHTS=0   initialize the weights instead of loading
rem   set tfdefs=-DTF_FP32=0           the packed int4/int8 engine
rem
rem If the link fails on `operator new(size_t, std::align_val_t)`: model_opt.inc
rem allocates a 64-byte-aligned object, which needs C++17 aligned new from the
rem runtime.  VC2019's vcruntime has it; older header sets need
rem   -include "C:\VC2019\include\vcruntime_new.h"
rem added to %%gcc%% (some of the clang configurations above already do that).
rem ---------------------------------------------------------------------------
set tfdefs=

%gcc% -s -std=c++17 -O3 -fno-math-errno -ffp-contract=off -fpermissive -Wno-format ^
  %tfdefs% %arch% %incs% %opts% -static coder0.cpp -o coder0.exe

del link.exe

del *.o
