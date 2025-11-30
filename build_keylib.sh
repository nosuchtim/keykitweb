#!/bin/bash
# Build script for keylib.c as a Windows executable
# This utility reads .k files and generates keylib.k with library definitions

echo "Building keylib.exe for Windows..."
echo ""

# Try gcc first (MinGW or similar)
if command -v gcc &> /dev/null; then
    echo "Found gcc, compiling keylib.c..."
    gcc -o keylib.exe src/keylib.c -Isrc -Wall
    if [ $? -eq 0 ]; then
        echo "✓ Successfully built keylib.exe with gcc"
        echo ""
        echo "========================================"
        echo "Build Complete!"
        echo "========================================"
        echo ""
        echo "keylib.exe has been created."
        echo "This utility scans .k files in a directory and generates keylib.k"
        echo ""
        echo "Usage:"
        echo "  Run keylib.exe from the lib directory to scan all .k files"
        echo "  and generate the keylib.k library definition file."
        echo ""
        exit 0
    else
        echo "✗ gcc build failed"
        exit 1
    fi
else
    echo ""
    echo "========================================"
    echo "ERROR: gcc not found!"
    echo "========================================"
    echo ""
    echo "Please install MinGW or another gcc compiler for Windows"
    echo "  1. MinGW-w64: https://www.mingw-w64.org/"
    echo "  2. TDM-GCC: https://jmeubank.github.io/tdm-gcc/"
    echo ""
    echo "After installation, make sure gcc is in your PATH"
    echo ""
    exit 1
fi
