# Building KeyKit Components on Windows

This document explains how to build Windows-native executables for KeyKit utilities.

## Prerequisites

You need a C compiler for Windows. Recommended options:

1. **MinGW-w64** (recommended for MSYS2/Git Bash users)
   - Download: https://www.mingw-w64.org/
   - Or install via MSYS2: `pacman -S mingw-w64-x86_64-gcc`

2. **TDM-GCC** (standalone installer)
   - Download: https://jmeubank.github.io/tdm-gcc/

3. **Visual Studio** (includes MSVC compiler)
   - Download: https://visualstudio.microsoft.com/

## Building keylib.exe

The `keylib` utility scans `.k` files and generates library definitions.

### Using the build script (bash/MSYS2)

```bash
cd src
bash build_keylib.sh
```

### Using the batch file (Windows Command Prompt)

```cmd
cd src
build_keylib.bat
```

### Manual build with gcc

```bash
cd src
gcc -o keylib.exe keylib.c -I. -Wall
```

### Manual build with MSVC

```cmd
cd src
cl /Fe:keylib.exe keylib.c /I. /W3
```

## Windows dirent.h Implementation

KeyKit uses POSIX directory functions (`opendir`, `readdir`, `closedir`) which are not available on Windows by default. We include Toni Ronkko's Windows-compatible `dirent.h` implementation:

- **File**: `src/dirent_windows.h`
- **Source**: https://github.com/tronkko/dirent
- **License**: MIT
- **Usage**: Already included in `keylib.c` via `#include "dirent_windows.h"`

This header provides Windows implementations of:
- `DIR` structure
- `struct dirent`
- `opendir()`, `readdir()`, `closedir()`
- `DT_DIR`, `DT_REG` constants

## Using keylib.exe

After building, run `keylib.exe` from the `lib` directory:

```bash
cd ../lib
../src/keylib.exe
```

This will scan all `.k` files in the current directory and generate `keylib.k` with library definitions.

## Generating the Library Manifest

The library manifest (`lib/lib_manifest.json`) lists all library files for runtime loading in the WebAssembly version.

### Using Python (Recommended)

```bash
cd lib
python generate_manifest.py
```

This is the most reliable cross-platform method.

### Using bash/MSYS2

```bash
cd lib
bash generate_manifest.sh
```

### Using Windows Command Prompt

```cmd
cd lib
generate_manifest.bat
```

The batch file automatically tries Python first, then falls back to bash if Python is not available.

This will scan for all `*.k`, `*.kc`, `*.kb`, `*.kbm`, `*.exp`, `*.txt`, `*.ppm`, and `*.lst` files and generate `lib_manifest.json`.

## Troubleshooting

### "gcc not found"
Make sure gcc is in your PATH. After installing MinGW, you may need to add it to your PATH:
```
set PATH=C:\MinGW\bin;%PATH%
```

### "dirent.h not found"
The code should use `dirent_windows.h` instead. Check that:
1. `dirent_windows.h` exists in the `src` directory
2. The source file includes `#include "dirent_windows.h"` (not `#include <dirent.h>`)

### Compilation errors with DT_DIR
If you see errors about `DT_DIR` being undefined, make sure you're using `dirent_windows.h` which defines these constants.

## Other Utilities

Other Windows-native utilities can be built the same way:

```bash
# Example for any .c file
gcc -o utility.exe utility.c -I. -Wall
```

Make sure to include `dirent_windows.h` if the code uses directory operations.
