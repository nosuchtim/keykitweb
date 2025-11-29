# KeyKit Library Files

This directory contains KeyKit library files that are loaded at runtime in the WebAssembly build.

## Files

- **`*.k`** - KeyKit source files (main library code)
- **`*.kc`** - KeyKit compiled files
- **`*.kb`** - KeyKit binary files
- **`*.kbm`** - KeyKit binary map files
- **`*.exp`** - Export/expression files
- **`*.txt`** - Text/documentation files
- **`*.ppm`** - PPM image files
- **`*.lst`** - List files (tool definitions)

## Manifest

The **`lib_manifest.json`** file contains a JSON array of all library files that should be loaded in the WebAssembly build.

### Regenerating the Manifest

When you add or remove files in this directory, regenerate the manifest:

**Using Python (recommended, cross-platform):**
```bash
python generate_manifest.py
```

**Using bash/MSYS2/Git Bash:**
```bash
bash generate_manifest.sh
```

**Using Windows Command Prompt:**
```cmd
generate_manifest.bat
```
(The batch file automatically tries Python first, then falls back to bash)

**Or just double-click** `generate_manifest.bat` in Windows Explorer.

This will update `lib_manifest.json` with the current list of files.

## How Library Files Are Used

In the WebAssembly build:

1. The manifest is fetched by the browser
2. Each file listed is downloaded and written to a virtual filesystem at `/keykit/lib/`
3. C code can access files using standard I/O: `fopen("/keykit/lib/filename.k", "r")`

See `../docs/RUNTIME_LIBRARY_LOADING.md` for complete details.

## File Count

Currently contains **396 library files**.
