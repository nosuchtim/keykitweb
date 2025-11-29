# KeyKit WebAssembly

A WebAssembly port of **KeyKit**, the algorithmic music programming language and MIDI toolkit originally created by Tim Thompson at AT&T Bell Labs.

KeyKit is a powerful environment for music composition, MIDI processing, and interactive graphics. This port enables KeyKit to run directly in web browsers using Emscripten and modern web APIs.

## Features

- **Full KeyKit Language** - Complete implementation of the KeyKit programming language
- **HTML5 Canvas Graphics** - Interactive graphics with drawing primitives, colors, and bitmap operations
- **Web MIDI API** - Real-time MIDI input/output with browser-connected devices
- **Virtual Filesystem** - Runtime loading of 390+ library files
- **NATS Messaging** - Distributed messaging support via WebSocket for collaboration and remote control

## Quick Start

### Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- Python 3.x
- A web browser with Web MIDI support (Chrome, Edge, Opera)

### Building

```bash
cd src
python build_wasm.py
```

This compiles the KeyKit C source to WebAssembly, producing:
- `keykit.html` - Main application page
- `keykit.js` - JavaScript runtime
- `keykit.wasm` - WebAssembly binary

### Running

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
└── docs (*.md files)       # Documentation
```

## Architecture

KeyKit uses a **machine-dependent abstraction layer** to isolate platform-specific code. The WebAssembly port implements ~60 `mdep_*` functions in `src/mdep_wasm.c` that interface with JavaScript via Emscripten's `EM_ASM` macros.

### Key Components

| Component | Files | Description |
|-----------|-------|-------------|
| Parser/Compiler | `gram.y`, `yacc.c` | KeyKit language parsing |
| Interpreter | `main.c`, `bltin.c` | Bytecode execution engine |
| Phrases | `phrase.c` | Musical phrase data structures |
| Graphics | `grid.c`, `kwind.c` | Window and drawing management |
| MIDI | `mdep_wasm.c` | Web MIDI API integration |
| Input | `keykit_library.js` | Mouse and keyboard events |

### JavaScript Interop

The port uses two integration methods:

1. **EM_ASM** - Inline JavaScript in C for simple operations
2. **JavaScript Library** - `keykit_library.js` for complex features (Canvas, MIDI, NATS)

```c
// Example: Drawing a line via EM_ASM
EM_ASM({
    var ctx = Module.canvas.getContext('2d');
    ctx.beginPath();
    ctx.moveTo($0, $1);
    ctx.lineTo($2, $3);
    ctx.stroke();
}, x0, y0, x1, y1);
```

## MIDI Setup

KeyKit works with any Web MIDI compatible device. For virtual MIDI routing on Windows, we recommend [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).

The browser will prompt for MIDI access when the application starts. Grant permission to enable MIDI input/output.

## Documentation

| Document | Description |
|----------|-------------|
| [CLAUDE.md](CLAUDE.md) | Project overview and development guidance |
| [CANVAS_DRAWING_GUIDE.md](CANVAS_DRAWING_GUIDE.md) | Canvas graphics integration |
| [EM_ASM_GUIDE.md](EM_ASM_GUIDE.md) | Emscripten JavaScript interop |
| [RUNTIME_LIBRARY_LOADING.md](RUNTIME_LIBRARY_LOADING.md) | Virtual filesystem details |
| [WASM_CONVERSION_PROGRESS.md](WASM_CONVERSION_PROGRESS.md) | Implementation status |

## Development

### Regenerating Library Manifest

When adding/removing library files:

```bash
cd lib
python generate_manifest.py
```

### Building Examples

```bash
cd examples
build_examples.bat
```

### Graphics Test Programs

Use `ptest1.c`, `ptest2.c`, `ptest3.c` in `src/` to test graphics functionality when developing new features.

## What is KeyKit?

KeyKit is a programming language and graphical toolkit for MIDI. Key capabilities include:

- **Algorithmic Composition** - Generate music programmatically using loops, randomness, and transformations
- **MIDI Processing** - Real-time manipulation of MIDI streams
- **Phrase Operations** - Cut, paste, transpose, time-stretch, quantize, and transform musical phrases
- **Interactive Tools** - Visual editors, piano roll, drum grid, and more
- **Extensibility** - Write custom tools and functions in the KeyKit language

Originally released in 1996, KeyKit has been used by musicians and composers worldwide for experimental and generative music.

## Sample MIDI Files

The `music/` directory includes sample MIDI files for testing:

- Bach Two-Part Inventions (`bachinv1.mid` - `bachinv8.mid`)
- Various demo patterns and compositions

## Credits

- **Original KeyKit** - Tim Thompson (AT&T Bell Labs)
- **WebAssembly Port** - Tim Thompson
- **KeyKit Community** - Contributors and users over the years

## Links

- [Original KeyKit](http://nosuch.com/keykit/) - Historical KeyKit resources
- [Emscripten](https://emscripten.org/) - C/C++ to WebAssembly compiler
- [Web MIDI API](https://developer.mozilla.org/en-US/docs/Web/API/Web_MIDI_API) - Browser MIDI specification

## License

See original KeyKit distribution for license terms.
