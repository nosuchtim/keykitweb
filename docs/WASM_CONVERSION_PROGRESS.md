# WebAssembly Conversion Progress

## Completed Tasks

### 1. Machine-Dependent Header (`mdep_wasm.h`)
- ✅ Created header file with all necessary function prototypes
- ✅ Defined MACHINE as "wasm"
- ✅ Included standard C library headers compatible with Emscripten
- ✅ Added forward declarations for structures

### 2. Machine-Dependent Implementation (`mdep_wasm.c`)
- ✅ Implemented basic system functions:
  - `mdep_hello()`, `mdep_bye()`
  - `mdep_changedir()`, `mdep_currentdir()`
  - `mdep_lsdir()` - directory listing
  - `mdep_filetime()`, `mdep_fisatty()`
  - `mdep_currtime()`, `mdep_coreleft()`
  - `mdep_makepath()` - path manipulation
  - `mdep_popup()`, `mdep_setcursor()`
  - `mdep_abortexit()`, `mdep_setinterrupt()`
  - `mdep_sync()`

- ✅ Implemented time functions:
  - `mdep_milliclock()` - millisecond clock
  - `mdep_resetclock()` - reset clock

- ✅ Implemented MIDI stubs:
  - `mdep_getnmidi()` - get MIDI input
  - `mdep_putnmidi()` - send MIDI output
  - `mdep_initmidi()` - initialize MIDI
  - `mdep_endmidi()` - cleanup MIDI
  - `mdep_midi()` - MIDI open/close

- ✅ Implemented console/input stubs:
  - `mdep_waitfor()` - wait for input
  - `mdep_getportdata()` - get port data
  - `mdep_getconsole()` - get console input
  - `mdep_statconsole()` - check console status

- ✅ Implemented graphics functions (stubs):
  - `mdep_maxx()`, `mdep_maxy()` - screen dimensions
  - `mdep_fontwidth()`, `mdep_fontheight()` - font metrics
  - `mdep_line()` - draw line
  - `mdep_string()` - draw text
  - `mdep_color()` - set color
  - `mdep_box()`, `mdep_boxfill()` - draw rectangles
  - `mdep_ellipse()`, `mdep_fillellipse()` - draw ellipses
  - `mdep_fillpolygon()` - draw filled polygon
  - `mdep_startgraphics()` - initialize graphics
  - `mdep_endgraphics()` - cleanup graphics
  - `mdep_plotmode()` - set plot mode
  - `mdep_screensize()`, `mdep_screenresize()` - screen management

- ✅ Implemented font functions:
  - `mdep_fontinit()` - initialize font

- ✅ Implemented mouse functions (stubs):
  - `mdep_mouse()` - get mouse position
  - `mdep_mousewarp()` - move mouse cursor

- ✅ Implemented color functions (stubs):
  - `mdep_colormix()` - set color palette
  - `mdep_initcolors()` - initialize colors

- ✅ Implemented bitmap functions:
  - `mdep_allocbitmap()` - allocate bitmap
  - `mdep_reallocbitmap()` - reallocate bitmap
  - `mdep_freebitmap()` - free bitmap
  - `mdep_movebitmap()` - move bitmap region
  - `mdep_pullbitmap()` - copy from screen
  - `mdep_putbitmap()` - copy to screen
  - `mdep_destroywindow()` - destroy window

- ✅ Implemented file/path functions:
  - `mdep_keypath()` - KeyKit library path
  - `mdep_musicpath()` - music files path
  - `mdep_postrc()` - post-initialization
  - `mdep_shellexec()` - execute shell command (stub)
  - `mdep_ignoreinterrupt()` - ignore interrupts
  - `mdep_browse()` - file browser dialog (stub)

- ✅ Implemented port functions (stubs):
  - `mdep_openport()` - open port
  - `mdep_putportdata()` - write to port
  - `mdep_closeport()` - close port
  - `mdep_ctlport()` - control port
  - `mdep_help()` - help system
  - `mdep_localaddresses()` - get local IP addresses

### 3. Build System (`build_wasm.py`)
- ✅ Created Python build script for Emscripten
- ✅ Configured compiler flags:
  - `-D__EMSCRIPTEN__` - define Emscripten macro
  - `-s ALLOW_MEMORY_GROWTH=1` - allow dynamic memory growth
  - `-s ASYNCIFY=1` - enable async support for blocking calls
  - `-s EXPORTED_FUNCTIONS=['_main']` - export main function
  - `-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']` - export runtime methods
  - Warning suppressions for compatibility
  - `-O2` optimization level
- ✅ Excluded Windows-specific `midi.c` from build
- ✅ Added proper error handling and logging

### 4. Key Header Updates (`key.h`)
- ✅ Added conditional include for `mdep_wasm.h` when `__EMSCRIPTEN__` is defined
- ✅ Ensured compatibility with existing code

## TODO: Functions Needing Implementation

### High Priority (Required for Basic Functionality)
1. **Graphics/Canvas Integration**
   - Implement actual drawing functions using HTML5 Canvas API via JavaScript
   - Use Emscripten's `EM_ASM` or `emscripten_run_script()` to call JavaScript
   - Functions: `mdep_line()`, `mdep_box()`, `mdep_string()`, etc.

2. **MIDI Support**
   - Integrate Web MIDI API via JavaScript
   - Implement `mdep_initmidi()`, `mdep_getnmidi()`, `mdep_putnmidi()`
   - Handle MIDI device enumeration and I/O

3. **Console/Input Handling**
   - Implement keyboard input capture
   - Implement `mdep_getconsole()` for keyboard events
   - Handle special keys and function keys

4. **Mouse Input**
   - Capture mouse events from Canvas
   - Implement `mdep_mouse()` to return current position and buttons

### Medium Priority (Enhanced Functionality)
5. **File System**
   - Use Emscripten's virtual file system (IDBFS or MEMFS)
   - Pre-load KeyKit library files
   - Implement file browser using HTML file input

6. **Bitmap Operations**
   - Implement actual bitmap allocation and manipulation
   - Use Canvas ImageData for bitmap storage

7. **Color Management**
   - Implement color palette using Canvas fillStyle/strokeStyle

### Low Priority (Nice to Have)
8. **Port/Network Functions**
   - Implement using WebSockets or WebRTC
   - For OSC and other network protocols

9. **Clipboard Integration**
   - Use Clipboard API for copy/paste

10. **Help System**
    - Integrate with web-based help viewer

## Next Steps

1. **Test Compilation**
   ```bash
   cd src
   python build_wasm.py
   ```

2. **Create JavaScript Glue Code**
   - Create `keykit_bridge.js` to interface between C and JavaScript
   - Implement Canvas drawing functions
   - Implement Web MIDI interface
   - Handle keyboard/mouse events

3. **Create HTML Interface**
   - Create `index.html` with Canvas element
   - Load and initialize the WASM module
   - Set up event handlers

4. **Implement Core Graphics**
   - Start with basic line/box/text drawing
   - Test with simple KeyKit programs

5. **Add MIDI Support**
   - Implement Web MIDI API integration
   - Test MIDI I/O

6. **File System Setup**
   - Pre-load KeyKit library files into virtual FS
   - Test file loading and execution

## Known Issues

1. **Windows-Specific Code**
   - `midi.c` is excluded (Windows-specific)
   - Some graphics code may assume Windows GDI

2. **Threading**
   - KeyKit may use threading which needs special handling in WASM
   - May need to use Web Workers

3. **Real-time Performance**
   - MIDI timing may need special attention
   - Use `requestAnimationFrame()` for graphics updates

## Build Requirements

- Emscripten SDK (emsdk)
- Python 3.x
- All C source files in `src/` directory

## Testing Strategy

1. Compile with minimal functionality
2. Test basic initialization
3. Add graphics layer by layer
4. Add MIDI support
5. Test with simple KeyKit programs
6. Gradually add more complex features
