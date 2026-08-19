@echo off
rem mk.cmd            -- tuning build:   Debug 1, Const 0.  Every knob is a live
rem                      mapping object and opt.pl can hill-climb it.
rem mk.cmd release    -- shipping build: Const 1.  Every knob is a literal the
rem                      compiler folds; no mapping objects, no indirection.
rem mk.cmd stat       -- build bmgstat, the image analysis tool.
rem mk.cmd pal        -- build bmppal, the palette transform of PALETTE.md.
rem mk.cmd check      -- prove the two builds produce identical streams.
rem
rem The Windows counterpart of mk.sh, and it has to stay in step with it: the
rem same MOD\ gives the same stream on either platform, and it does.
rem
rem MinGW g++ by default.  "set CXX=cl" first to build with MSVC instead -- the
rem two want different words for the same four things and that is the whole of
rem the difference.  Regenerating MOD\ from IDX\ needs perl on PATH (Strawberry
rem Perl will do); without it the checked-in MOD\ is used as it stands, which is
rem what mk.sh does too.
setlocal

if not defined CXX set "CXX=g++"
if not defined OPT set "OPT=-O2"

set "MSVC="
if /i "%CXX%"=="cl" set "MSVC=1"

if defined MSVC (
  rem /GR- drops RTTI.  Exceptions stay on, because MSVC's operator new wants
  rem them.  _CRT_SECURE_NO_WARNINGS is defined in the sources themselves.
  set "CXXFLAGS=/nologo /O2 /GR- /DNDEBUG"
  set "STATFLAGS=/nologo /O2 /W3 /DNDEBUG"
  set "OUTFLAG=/Fe:"
  set "STRIP="
) else (
  rem -static so the exe runs without libstdc++-6.dll and libgcc_s_seh-1.dll
  rem beside it, which is the one thing a MinGW build gets wrong by default.
  set "CXXFLAGS=%OPT% -std=gnu++2a -DNDEBUG -fno-exceptions -fno-rtti -fomit-frame-pointer -static"
  set "STATFLAGS=%OPT% -std=gnu++2a -Wall -Wextra -Wno-unused-parameter -static"
  set "OUTFLAG=-o "
  set "STRIP=-s"
)

if /i "%~1"=="stat"    goto :stat
if /i "%~1"=="pal"     goto :pal
if /i "%~1"=="release" goto :release
if /i "%~1"=="check"   goto :check
if /i "%~1"==""        goto :tuning
echo mk.cmd: unknown target "%~1" -- try release, stat, pal or check
exit /b 1

rem ------------------------------------------------------------------ tuning
:tuning
for %%f in (IDX\bmg-*.idx) do call :tune1 "%%~nf"
%CXX% %CXXFLAGS% bmg.cpp %OUTFLAG%bmg.exe
if errorlevel 1 exit /b 1
echo tuning build: every knob is a live mapping object
exit /b 0

:tune1
call :isconst "%~1"
if errorlevel 1 goto :eof
call :gen "%~1" 1
goto :eof

rem ----------------------------------------------------------------- release
:release
call :haveperl
if errorlevel 1 (
  echo mk.cmd release needs perl on PATH to fold the knobs into literals.
  exit /b 1
)
for %%f in (IDX\bmg-*.idx) do call :rel1 "%%~nf"
%CXX% %CXXFLAGS% %STRIP% bmg.cpp %OUTFLAG%bmg.exe
if errorlevel 1 exit /b 1
echo shipping build: MOD\ holds literals
exit /b 0

:rel1
call :isconst "%~1"
if errorlevel 1 goto :eof
call :mkconst "%~1"
goto :eof

rem -------------------------------------------------------------------- stat
:stat
rem The analysis tool.  It shares nothing with the coder -- no IDX, no MOD --
rem so it is built on its own and never as a side effect of mk.cmd.
%CXX% %STATFLAGS% bmgstat.cpp %OUTFLAG%bmgstat.exe
if errorlevel 1 exit /b 1
echo analysis tool: bmgstat testfiles\*.bmp
exit /b 0

rem --------------------------------------------------------------------- pal
:pal
rem The palette transform.  Borrows bmg_rc.inc and nothing else, so it needs no
rem IDX and no MOD either.
%CXX% %STATFLAGS% bmppal.cpp %OUTFLAG%bmppal.exe
if errorlevel 1 exit /b 1
echo palette tool: bmppal c in.bmp idx.bmp idx.pal
exit /b 0

rem ------------------------------------------------------------------- check
:check
call "%~f0" >nul
if errorlevel 1 exit /b 1
copy /y bmg.exe "%TEMP%\bmg_tune.exe" >nul
call "%~f0" release >nul
if errorlevel 1 exit /b 1
copy /y bmg.exe "%TEMP%\bmg_ship.exe" >nul
set "RC=0"
set "FILES=%~2"
if not defined FILES set "FILES=testfiles\*.bmp"
for %%f in (%FILES%) do call :cmp1 "%%f"
del /q "%TEMP%\bmg_tune.exe" "%TEMP%\bmg_ship.exe" "%TEMP%\bmg_a.bmg" "%TEMP%\bmg_b.bmg" 2>nul
exit /b %RC%

:cmp1
"%TEMP%\bmg_tune.exe" c "%~1" "%TEMP%\bmg_a.bmg" >nul 2>&1
"%TEMP%\bmg_ship.exe" c "%~1" "%TEMP%\bmg_b.bmg" >nul 2>&1
rem Size first, then fc /b.  fc is the real test, but a size difference settles
rem it on its own and settles it without depending on fc at all.
set "SA="
set "SB="
for %%g in ("%TEMP%\bmg_a.bmg") do set "SA=%%~zg"
for %%g in ("%TEMP%\bmg_b.bmg") do set "SB=%%~zg"
if not defined SA goto :cmp1_differ
if not "%SA%"=="%SB%" goto :cmp1_differ
fc /b "%TEMP%\bmg_a.bmg" "%TEMP%\bmg_b.bmg" >nul 2>&1
if errorlevel 1 goto :cmp1_differ
echo same   %~nx1
goto :eof
:cmp1_differ
echo DIFFER %~nx1
set "RC=1"
goto :eof

rem --------------------------------------------------------------- utilities

rem Errorlevel 1 if the basename ends in -const: that is scratch an earlier
rem release build wrote, and it must not feed back into one.
:isconst
set "N=%~1"
if "%N:~-6%"=="-const" exit /b 1
exit /b 0

:haveperl
perl -v >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0

rem Run idx2inc.pl over one .idx and move its two headers into MOD\.  Without
rem perl this does nothing and the checked-in MOD\ is used as it stands, which
rem is what mk.sh does.
:gen
call :haveperl
if errorlevel 1 goto :eof
pushd IDX
perl idx2inc.pl "%~1.idx" %2 >nul
popd
if exist "IDX\%~1_h.inc" move /y "IDX\%~1_h.inc" MOD\ >nul
if exist "IDX\%~1_p.inc" move /y "IDX\%~1_p.inc" MOD\ >nul
goto :eof

rem Derive the shipping headers from the tuning .idx with one substitution, so
rem the two can never drift.  perl stands in for mk.sh's sed.
:mkconst
perl -pe "s/^Const 0/Const 1/" "IDX\%~1.idx" > "IDX\%~1-const.idx"
if exist "IDX\%~1.inc" (copy /y "IDX\%~1.inc" "IDX\%~1-const.inc" >nul) else (type nul > "IDX\%~1-const.inc")
call :gen "%~1-const" 0
move /y "MOD\%~1-const_h.inc" "MOD\%~1_h.inc" >nul
move /y "MOD\%~1-const_p.inc" "MOD\%~1_p.inc" >nul
del /q "IDX\%~1-const.idx" "IDX\%~1-const.inc" 2>nul
goto :eof
