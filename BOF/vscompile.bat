@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

SET "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
SET "VCVARS_X86=%VS_PATH%\VC\Auxiliary\Build\vcvars32.bat"
SET "VCVARS_X64=%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
SET "OUTDIR=build"

IF NOT EXIST %OUTDIR% (
    MKDIR %OUTDIR%
)

:: Build x64
CALL "%VCVARS_X64%" >nul
ECHO [*] Compiling EnableEFS.x64.o
cl.exe /nologo /c /Od /MT /W0 /GS- /Tc EnableEFS.c /Fo%OUTDIR%\EnableEFS.x64.o

:: Build x86
CALL "%VCVARS_X86%" >nul
ECHO [*] Compiling EnableEFS.x86.o
cl.exe /nologo /c /Od /MT /W0 /GS- /Tc EnableEFS.c /Fo%OUTDIR%\EnableEFS.x86.o


ECHO [*] Done.
ENDLOCAL