@echo off
REM Build script for keylib.c as a Windows executable
REM This utility reads .k files and generates keylib.k with library definitions

echo Building keylib.exe for Windows...
echo.

REM Check if gcc (MinGW) is available
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found gcc, using MinGW compiler...
    gcc -o keylib.exe src/keylib.c -Isrc -Wall
    if %ERRORLEVEL% EQU 0 (
        echo ✓ Successfully built keylib.exe with gcc
        goto :done
    ) else (
        echo ✗ gcc build failed
        goto :try_msvc
    )
)

:try_msvc
REM Try MSVC cl.exe
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found cl, using MSVC compiler...
    cl /Fe:keylib.exe src/keylib.c /Isrc /W3
    if %ERRORLEVEL% EQU 0 (
        echo ✓ Successfully built keylib.exe with MSVC
        goto :done
    ) else (
        echo ✗ MSVC build failed
        goto :error
    )
)

:error
echo.
echo ========================================
echo ERROR: No C compiler found!
echo ========================================
echo.
echo Please install one of the following:
echo   1. MinGW (gcc for Windows): https://www.mingw-w64.org/
echo   2. Visual Studio (includes MSVC cl.exe)
echo   3. TDM-GCC: https://jmeubank.github.io/tdm-gcc/
echo.
echo After installation, make sure the compiler is in your PATH
echo.
exit /b 1

:done
echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo keylib.exe has been created.
echo This utility scans .k files in a directory and generates keylib.k
echo.
echo Usage:
echo   Run keylib.exe from the lib directory to scan all .k files
echo   and generate the keylib.k library definition file.
echo.
