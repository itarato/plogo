
@echo off
setlocal

REM Initialize MSVC x64 compiler environment
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if errorlevel 1 (
    echo ERROR: Could not initialize MSVC x64 environment
    exit /b 1
)

set RAYLIB=C:\Users\itara\CHECKOUT\vcpkg\packages\raylib_x64-windows

set COMMON=/nologo /std:c++20 /EHsc /W4 /MD /I%RAYLIB%\include /Ilib\imgui /Ilib\rlImGui

set CONFIG=release
set TARGET=main

if "%1"=="debug" (
    set CONFIG=debug
)

REM ============================================
REM Compiler configuration
REM ============================================

if "%CONFIG%"=="debug" (
    set FLAGS=/Od /Zi /MDd
) else (
    set FLAGS=/O2 /MD
)

REM ============================================
REM Verify environment
REM ============================================

if not exist "%RAYLIB%\include\raylib.h" (
    echo ERROR: raylib.h not found!
    echo Expected: %RAYLIB%\include\raylib.h
    exit /b 1
)

if not exist "%RAYLIB%\lib\raylib.lib" (
    echo ERROR: raylib.lib not found!
    echo Expected: %RAYLIB%\lib\raylib.lib
    exit /b 1
)

REM ============================================
REM Output directory
REM ============================================

if not exist dist mkdir dist

REM ============================================
REM Build
REM ============================================

echo "Building main (%CONFIG%, x64)"

cl %COMMON% %FLAGS% ^
    src\main.cpp ^
    lib\imgui\imgui.cpp ^
    lib\imgui\imgui_draw.cpp ^
    lib\imgui\imgui_tables.cpp ^
    lib\imgui\imgui_widgets.cpp ^
    lib\rlImGui\rlImGui.cpp ^
    /Fe:dist\main.exe ^
    /link /MACHINE:X64 ^
    /LIBPATH:%RAYLIB%\lib ^
    raylib.lib ^
    winmm.lib gdi32.lib opengl32.lib

if errorlevel 1 (
    echo BUILD FAILED!
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL!
echo Output: dist\%TARGET%.exe

REM Copy raylib DLL for distribution
if exist "%RAYLIB%\bin\raylib.dll" (
    copy /Y "%RAYLIB%\bin\raylib.dll" dist\
)

endlocal
