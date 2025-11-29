# KeyKit WebAssembly

This is a WebAssembly port of **KeyKit**, enabling KeyKit to run directly in web browsers using Emscripten and modern web APIs.
This repo is a stripped-down version of the https://github.com/nosuchtim/keykit repo, with only the WASM port and updates needed for that environment.

KeyKit is a programming language and graphical toolkit for realtime and algorithmic MIDI manipulation and experimentation.  It was created by Tim Thompson at AT&T Bell Labs and released in 1996.
Key capabilities include:

- **Algorithmic Composition** - Generate music programmatically
- **MIDI Processing** - Real-time manipulation of MIDI streams
- **Phrase Operations** - A MIDI phrase is a fundamental data type in the language
- **GUI** - A custom window system implemented completely in KeyKit code.
- **Interactive Tools** - Visual editors, piano roll, drum grid, and more
- **Extensibility** - Write custom tools and functions in the KeyKit language

## Features of this WASM port

- **Full KeyKit Language** - Complete implementation of the KeyKit programming language
- **HTML5 Canvas Graphics** - Interactive graphics with drawing primitives, colors, and bitmap operations
- **Web MIDI API** - Real-time MIDI input/output with browser-connected devices
- **Virtual Filesystem** - Runtime loading of 390+ library files
- **Synced Local Directory** - Files can be written to the local directory, which is synced with the virtual filesystem.

## Building

Install [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) and Python 3.x, then

```bash
cd src
python build_wasm.py
```

This compiles the KeyKit C source to WebAssembly, producing:
- `keykit.html` - Main application page
- `keykit.js` - JavaScript runtime
- `keykit.wasm` - WebAssembly binary

## Running

Start a local web server (required for loading library files):

```bash
python serve.py
```

Then open `http://localhost:8000/keykit.html` in your browser.  Use the "Change local folder" button to attach the local directory you want to use.

## Project Structure

```
keykitwasm/
├── src/                    # C source code and build system
│   ├── mdep_wasm.c/h       # WebAssembly machine-dependent layer
│   ├── keykit_library.js   # JavaScript interop (Canvas, MIDI, input)
│   ├── keykit_shell.html   # Custom HTML template
│   ├── build_wasm.py       # Emscripten build script
│   └── *.c, *.h            # KeyKit core source files
├── lib/                    # KeyKit library files (*.k)
│   ├── lib_manifest.json   # Auto-generated file list
│   └── *.k                 # 390+ library source files
├── examples/               # Canvas drawing examples
├── music/                  # Sample MIDI files
└── docs/                   # Documentation
```

## Architecture

KeyKit uses a **machine-dependent abstraction layer** to isolate platform-specific code. The WebAssembly port implements ~60 `mdep_*` functions in `src/mdep_wasm.c` that interface with JavaScript via Emscripten's `EM_ASM` macros.
The port uses two integration methods:

1. **EM_ASM** - Inline JavaScript in C for simple operations
2. **JavaScript Library** - `keykit_library.js` for complex features (Canvas, MIDI)

## MIDI Setup

KeyKit works with any Web MIDI compatible device. For virtual MIDI routing on Windows, use [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).  Some of the files (e.g. lib/keyrc.k) are hard-coded to open "loopMIDI Port 1" for output and "loopMIDI Port 2" for input.

The browser will prompt for MIDI access when the application starts. Grant permission to enable MIDI input/output.

## Regenerating Library Manifest

When adding/removing library files:

```bash
cd lib
python generate_manifest.py
```

## Sample MIDI Files

The `music` directory includes sample MIDI files for testing:

- Bach Two-Part Inventions (`bachinv1.mid` - `bachinv8.mid`)

## Issues

- Sweeping operations are ugly due to the lack of real XOR graphics.