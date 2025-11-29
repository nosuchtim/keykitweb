@echo off
REM Build script for canvas drawing examples
REM Make sure you have Emscripten installed and activated

REM Change to the directory where this script is located
cd /d "%~dp0"

set EMCC=C:\Users\tjt\GitHub\emsdk\upstream\emscripten\emcc.bat

echo Building Canvas Drawing Examples...
echo.

REM Build Example 1: Simple canvas drawing with EM_ASM
echo [1/2] Building simple canvas example...
call %EMCC% canvas_drawing_example.c -o canvas_drawing_example.html ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s EXPORTED_FUNCTIONS="['_main','_on_mouse_event']" ^
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString','getValue','setValue']" ^
    --shell-file canvas_shell.html ^
    -lm ^
    -g ^
    -O0

if %ERRORLEVEL% EQU 0 (
    echo ✓ Simple example built successfully!
) else (
    echo ✗ Simple example build failed!
    exit /b 1
)

echo.

REM Build Example 2: Advanced canvas with JavaScript library
echo [2/2] Building advanced canvas example...
call %EMCC% advanced_canvas_example.c -o advanced_canvas_example.html ^
    --js-library canvas_library.js ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s EXPORTED_FUNCTIONS="['_main','_update_animation','_on_mouse_event','_on_midi_ready','_on_midi_message']" ^
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" ^
    --shell-file canvas_shell.html ^
    -lm ^
    -g ^
    -O0

if %ERRORLEVEL% EQU 0 (
    echo ✓ Advanced example built successfully!
) else (
    echo ✗ Advanced example build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo To view the examples:
echo 1. Start a local web server in this directory
echo 2. Open canvas_drawing_example.html in your browser
echo 3. Open advanced_canvas_example.html in your browser
echo.
echo Example web server commands:
echo   python -m http.server 8000
echo   OR
echo   npx http-server
echo.
echo Then navigate to: http://localhost:8000/
echo.

pause
