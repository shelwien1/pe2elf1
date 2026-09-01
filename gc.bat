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
rem The transformer engine (tf\*.cpp) is compiled as its own set of translation
rem units with IEEE floating point: NO -ffast-math / -Ofast and NO -flto.  The
rem range decoder that unpacks the weights recomputes the RoPE tables with a
rem bit-exact port of CUDA's sinf/cosf, and every quantization step of the model
rem depends on exact round-to-nearest division, so relaxed FP would change the
rem model's output.  This mirrors what cmix's own makefile does for the same
rem sources.  coder0.cpp keeps its -Ofast build; it only sees tf\model_opt.h.
rem ---------------------------------------------------------------------------
rem If the link fails on `operator new(size_t, std::align_val_t)`: model_opt.cpp
rem allocates a 64-byte-aligned object, which needs C++17 aligned new from the
rem runtime.  VC2019's vcruntime has it; older header sets need
rem   -include "C:\VC2019\include\vcruntime_new.h"
rem added to %%gcc%% (some of the clang configurations above already do that).
set tfopts=-std=c++17 -O3 -fno-math-errno

for %%f in (weights_io qmat_dense qmat_sparse attn kda glue arena_build model_opt) do (
  %gcc% -c %tfopts% %arch% %incs% %opts% tf\%%f.cpp -o %%f.o
)
rem load-time only: -Os keeps the weights range decoder at ~5KB of code
%gcc% -c -std=c++17 -Os -fno-math-errno %arch% %incs% %opts% tf\weights_io_compressed.cpp -o weights_io_compressed.o

rem c++17, not c++23: the original single-line build put -std=c++17 in %%incs%%
rem after -std=c++23, and clang takes the last one, so this is what coder0 has
rem always actually been compiled as.
%gcc% -c -std=c++17 -Ofast -fpermissive -Wno-format %arch% %incs% %opts% coder0.cpp -o coder0.o

%gcc% -s %arch% -static -fuse-ld=lld coder0.o weights_io.o weights_io_compressed.o ^
  qmat_dense.o qmat_sparse.o attn.o kda.o glue.o arena_build.o model_opt.o -o coder0.exe

del link.exe

del *.o
