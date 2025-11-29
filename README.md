# KeyKit WebAssembly

A WebAssembly port of **KeyKit**, the algorithmic music programming language and MIDI toolkit originally created by Tim Thompson at AT&T Bell Labs.

KeyKit is a powerful environment for music composition, MIDI processing, and interactive graphics. This port enables KeyKit to run directly in web browsers using Emscripten and modern web APIs.

## What is KeyKit?

KeyKit is a programming language and graphical toolkit for MIDI. Key capabilities include:

- **Algorithmic Composition** - Generate music programmatically
- **MIDI Processing** - Real-time manipulation of MIDI streams
- **Phrase Operations** - A MIDI phrase is a fundamental data type in the language
- **GUI** - A custom window system implemented completely in KeyKit code.
- **Interactive Tools** - Visual editors, piano roll, drum grid, and more
- **Extensibility** - Write custom tools and functions in the KeyKit language

Originally released in 1996, KeyKit has been used by musicians and composers worldwide for experimental and generative music.

## Features of this WASM port

- **Full KeyKit Language** - Complete implementation of the KeyKit programming language
- **HTML5 Canvas Graphics** - Interactive graphics with drawing primitives, colors, and bitmap operations
- **Web MIDI API** - Real-time MIDI input/output with browser-connected devices
- **Virtual Filesystem** - Runtime loading of 390+ library files

## Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- Python 3.x
- A web browser with Web MIDI support (Chrome, Edge, Opera)

## Local Directory

To allow the saving of things, the local directory is synced with the virtual /keykit/local directory.

## Building

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

Then open `http://localhost:8000/keykit.html` in your browser.

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

This repo is a stripped-down version of the nosuchtim/keykit repo, with only the WASM port and other updates that are specific and hard-coded.

## JavaScript Interop

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