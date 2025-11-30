# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

KeyKit is a music programming language and MIDI toolkit being ported to WebAssembly. This repository contains the C source code for KeyKit along with a WebAssembly conversion layer and examples demonstrating canvas drawing and graphics integration.

## Build Commands

### Building the WebAssembly Version

```bash
python build_wasm.py
```

This compiles the KeyKit C source to WebAssembly using Emscripten. The build script:
- Uses Emscripten located at `C:\Users\tjt\GitHub\emsdk\upstream\emscripten\emcc.bat`
- Outputs `keykit.html`, `keykit.js`, and `keykit.wasm` to the main repo directory
- Enables `ALLOW_MEMORY_GROWTH=1` and `ASYNCIFY=1` for blocking calls
- Logs output to `build_log.txt`

### Building Examples

```bash
cd examples
build_examples.bat
```

This builds two canvas drawing examples demonstrating how to integrate HTML5 Canvas with C/WebAssembly code.

### Building Windows Native Utilities

```bash
bash build_keylib.sh
```

This builds Windows-native utilities like `keylib.exe` that scan `.k` files and generate library definitions. See [WINDOWS_BUILD.md](docs/WINDOWS_BUILD.md) for detailed Windows compilation instructions, including how the Windows-compatible `dirent.h` implementation is used.

## Architecture

### Machine-Dependent Layer

KeyKit uses a **machine-dependent abstraction layer** to isolate platform-specific code. Key files:

- **`src/mdep_wasm.c` and `src/mdep_wasm.h`**: WebAssembly implementation of ~60 `mdep_*` functions
- **`src/mdep.h`**: Windows-specific implementation (original platform)
- **`src/key.h`**: Conditionally includes `mdep_wasm.h` when `__EMSCRIPTEN__` is defined (line 16-20)

The machine-dependent layer provides:
- **System functions**: File I/O, directory operations, time/clock, process control
- **Graphics functions**: Screen management, drawing primitives (lines, boxes, ellipses), text rendering, color management, bitmap operations
- **Input functions**: Mouse, keyboard, console, event handling
- **MIDI functions**: Port enumeration, input/output, device management

### Core Components

- **Parser/Compiler**: `src/gram.y`, `src/yacc.c`, `src/yacc.h` - Language parsing
- **Code Generation**: `src/code.c`, `src/code2.c` - Bytecode generation
- **Interpreter**: `src/main.c`, `src/bltin.c`, `src/meth.c` - Execution engine
- **Data Structures**: `src/phrase.c`, `src/phrase.h` - Musical phrase representation
- **Symbol Management**: `src/sym.c` - Symbol table
- **Utilities**: `src/util.c`, `src/misc.c`, `src/real.c`
- **Graphics**: `src/grid.c`, `src/view.c`, `src/kwind.c` - Window/graphics management
- **MIDI**: `src/mfin.c`, `src/midi.c` (Windows), MIDI I/O (Web MIDI API intended for WASM)

### WebAssembly Stub Strategy

The WebAssembly port uses a **stub-first approach**:
1. All `mdep_*` functions are implemented as stubs that accept correct parameters and return safe defaults
2. Graphics stubs are ready for HTML5 Canvas integration via `EM_ASM` macros
3. MIDI stubs properly enumerate 2 virtual input and 2 output devices (defined in `mdep_wasm.c:170-171`)
4. Stubs won't crash - they're production-ready placeholders waiting for real implementations

### Key Data Structures

- **`Midiport`** (`key.h`): MIDI port structure, defined with 64 input/64 output devices (`key.h:755-757`)
- **`Pbitmap`** (`grid.h:108`): Bitmap structure for graphics operations
- **`Phrasep`** (`key.h:53`): Musical phrase pointer type
- **`Symbol`** (`key.h:608`): Symbol table entry
- **`Ktask`** (`key.h:671`): Task/scheduler structure

## JavaScript Interop

### Using EM_ASM for Canvas Drawing

See `docs/EM_ASM_GUIDE.md` and `docs/CANVAS_DRAWING_GUIDE.md` for comprehensive guides. Quick example:

```c
#include <emscripten.h>

void mdep_line(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.moveTo($0, $1);
        ctx.lineTo($2, $3);
        ctx.stroke();
    }, x0, y0, x1, y1);
}
```

### Two Integration Methods

1. **EM_ASM** (simpler, good for prototyping): JavaScript code directly in C functions
2. **JavaScript Library** (more efficient): Use `--js-library canvas_library.js` and `extern` declarations

See `examples/keykit_graphics_implementation.c` for ready-to-use implementations.

## Important Conversion Notes

### Excluded Files

`src/midi.c` is excluded from WebAssembly builds (Windows-specific, uses RtMidi). MIDI functionality for web should use Web MIDI API through JavaScript interop in `mdep_wasm.c`.

### Build Files Not Source Files

When modifying code, avoid editing:
- `yacc.h`, `yacc.output` (generated from `gram.y`)
- `d_*.h` files (auto-generated function declarations by `protoflp` tool)

### Header File Pattern

The codebase uses a pattern where external function declarations are in `d_*.h` files, generated by the `protoflp` program (see `src/README`). For WebAssembly work, focus on the main source files.

## Testing Graphics

Use the test programs to exercise graphics functionality:
- `src/ptest1.c`, `src/ptest2.c`, `src/ptest3.c` - Graphics test programs
- `src/ptest0.c`, `src/ptest4.c` - Additional test utilities

## Library File Loading

KeyKit library files (`lib/*.k` and related files) are loaded at runtime into Emscripten's virtual filesystem:

- **Manifest**: `lib/lib_manifest.json` lists all library files (regenerate with `bash lib/generate_manifest.sh`)
- **Runtime Loading**: `keykit_shell.html` contains `loadLibraryFiles()` that fetches and loads files before `main()`
- **Virtual Paths**: C code accesses files via `/keykit/lib/filename.k` using standard `fopen()`, `fread()`, etc.
- **Build Config**: `FORCE_FILESYSTEM=1` and `FS` API exported in `build_wasm.py`

See `docs/RUNTIME_LIBRARY_LOADING.md` for complete details.

## NATS Messaging Integration

KeyKit includes **NATS.ws** integration for distributed messaging and collaboration. NATS enables real-time communication between KeyKit instances or external systems via WebSocket.

### Architecture

The NATS implementation follows the same async callback pattern as MIDI:

1. **JavaScript Layer** ([keykit_library.js](keykit_library.js)):
   - `js_nats_connect(url)` - Connect to NATS server via WebSocket
   - `js_nats_publish(subject, data)` - Publish string message to subject
   - `js_nats_subscribe(subject)` - Subscribe to additional subjects
   - `js_nats_is_connected()` - Check connection status
   - `js_nats_close()` - Close connection and cleanup
   - Automatic subscription to `keykit.>` wildcard on connection
   - Calls `mdep_on_nats_message(subject, data)` callback when messages arrive

2. **C Interface** ([src/mdep_wasm.c](src/mdep_wasm.c)):
   - `mdep_on_nats_message()` - Async callback, buffers messages (minimal to avoid ASYNCIFY issues)
   - `mdep_nats_connect(url)` - Initiate connection to NATS server
   - `mdep_nats_publish(subject, data)` - Send message
   - `mdep_nats_subscribe(subject)` - Add subscription
   - `mdep_nats_get_message(&subject, &data)` - Retrieve buffered message (call from main loop)
   - `mdep_nats_has_messages()` - Check if messages available
   - `mdep_nats_is_connected()` - Check connection
   - `mdep_nats_close()` - Close and cleanup
   - 10-message circular buffer for incoming messages

3. **Build Configuration**:
   - NATS.ws library loaded via CDN in [keykit_shell.html](keykit_shell.html)
   - `mdep_on_nats_message` exported in [build_wasm.py](build_wasm.py)

### Usage Example

```c
// Connect to local NATS server with WebSocket
mdep_nats_connect("ws://localhost:8080");

// Publish a message
mdep_nats_publish("keykit.midi.note", "C4 velocity 100");

// Subscribe to additional subjects (beyond default keykit.>)
mdep_nats_subscribe("external.events");

// In main event loop, check for incoming messages
char *subject, *data;
while (mdep_nats_get_message(&subject, &data)) {
    printf("Received on %s: %s\n", subject, data);
    // Process message...
    free(subject);
    free(data);
}
```

### Server Setup

Your NATS server must have WebSocket support enabled:

```bash
# Run NATS server with WebSocket
nats-server --websocket_port 8080
```

Or configure in `nats-server.conf`:
```
websocket {
  port: 8080
  no_tls: true
}
```

### Use Cases

- **Distributed MIDI**: Route MIDI events between multiple KeyKit instances
- **Collaboration**: Share musical phrases, patterns, or control data
- **Remote Control**: Control KeyKit parameters from external applications
- **Event Streaming**: Publish musical events to analytics or recording systems
- **Synchronization**: Coordinate timing and state across distributed performances

### Implementation Notes

- Messages are buffered in C to avoid ASYNCIFY stack issues with async callbacks
- Default subscription to `keykit.>` catches all KeyKit-related subjects
- String-only messages via `StringCodec()` (binary support could be added)
- Connection is asynchronous - check `mdep_nats_is_connected()` before publishing
- Buffer holds 10 messages - older messages dropped if not consumed

## Current Status

Implementation complete:
- **Compilation**: ✓ Compiles successfully with Emscripten
- **Graphics**: ✓ Full HTML5 Canvas integration via JavaScript library
- **MIDI**: ✓ Web MIDI API with async device enumeration
- **Input**: ✓ Mouse and keyboard event handling
- **File I/O**: ✓ Virtual filesystem with runtime library loading
- **NATS Messaging**: ✓ NATS.ws integration for distributed messaging

## Testing the WebAssembly Build

1. Build the project:
   ```bash
   python build_wasm.py
   ```

2. Serve the files (CORS requires a web server):
   ```bash
   # From the repo root directory
   python -m http.server 8000
   # OR
   npx http-server
   ```

3. Open `http://localhost:8000/keykit.html` in a browser

4. Check browser console for:
   - Library file loading progress
   - MIDI device enumeration
   - Canvas initialization
   - Any KeyKit output

## Next Development Steps

1. Test full KeyKit functionality in browser
2. Optimize library file loading (lazy loading, compression)
3. Add more KeyKit-specific UI features
4. Test MIDI I/O with real devices
5. Performance optimization and debugging
