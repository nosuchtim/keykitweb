# Applied Patterns from Advanced Canvas Example

This document explains how the patterns and techniques from `examples/advanced_canvas_example.c` and `examples/canvas_library.js` have been applied to the KeyKit WebAssembly port in the `src/` directory.

## Summary of Changes

The implementation transforms KeyKit from stub-based graphics/MIDI to fully functional HTML5 Canvas rendering and Web MIDI API integration, following proven patterns from the working examples.

### Files Created

1. **`src/keykit_library.js`** - JavaScript library with Canvas, MIDI, and input functions
2. **`src/keykit_shell.html`** - Custom HTML template with canvas and styling
3. **`APPLIED_PATTERNS.md`** - This documentation

### Files Modified

1. **`src/mdep_wasm.c`** - Implemented all graphics, MIDI, mouse, and keyboard functions
2. **`src/build_wasm.py`** - Updated build to include JavaScript library and exports

## Pattern 1: JavaScript Library Integration

### From Examples
The advanced example uses `--js-library canvas_library.js` to define JavaScript functions that C code calls:

```javascript
// examples/canvas_library.js
mergeInto(LibraryManager.library, {
    js_draw_line: function (x0, y0, x1, y1) {
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.moveTo(x0, y0);
        ctx.lineTo(x1, y1);
        ctx.stroke();
    },
    // ... more functions
});
```

### Applied to KeyKit
Created `src/keykit_library.js` with all necessary functions:

```javascript
mergeInto(LibraryManager.library, {
    js_clear_canvas: function () { /* ... */ },
    js_draw_line: function (x0, y0, x1, y1) { /* ... */ },
    js_fill_rect: function (x, y, w, h) { /* ... */ },
    js_draw_ellipse: function (x, y, radiusX, radiusY) { /* ... */ },
    // + 20 more graphics functions
    // + 8 Web MIDI API functions
    // + 5 mouse/keyboard functions
});
```

**Build Integration:**
```python
# src/build_wasm.py
"--js-library", "keykit_library.js"
```

## Pattern 2: External Function Declarations

### From Examples
```c
// examples/advanced_canvas_example.c
extern void js_clear_canvas();
extern void js_draw_line(int x0, int y0, int x1, int y1);
extern void js_set_color(const char *color);
```

### Applied to KeyKit
```c
// src/mdep_wasm.c (lines 10-47)
extern void js_clear_canvas();
extern void js_draw_line(int x0, int y0, int x1, int y1);
extern void js_draw_rect(int x, int y, int w, int h);
extern void js_fill_rect(int x, int y, int w, int h);
// ... 30+ extern declarations
```

## Pattern 3: Canvas Drawing Implementation

### From Examples
```c
void draw_grid(int spacing, const char *color) {
    int width = js_get_canvas_width();
    int height = js_get_canvas_height();
    js_set_color(color);
    js_set_line_width(1);
    for (int x = 0; x < width; x += spacing) {
        js_draw_line(x, 0, x, height);
    }
}
```

### Applied to KeyKit
Implemented all `mdep_*` graphics functions:

```c
// src/mdep_wasm.c
void mdep_line(int x0, int y0, int x1, int y1) {
    js_draw_line(x0, y0, x1, y1);
}

void mdep_box(int x0, int y0, int x1, int y1) {
    // KeyKit uses two-point format, Canvas uses x,y,w,h
    int x = (x0 < x1) ? x0 : x1;
    int y = (y0 < y1) ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);
    js_draw_rect(x, y, w, h);
}

void mdep_ellipse(int x0, int y0, int x1, int y1) {
    // Convert bounding box to center + radii
    int cx = (x0 + x1) / 2;
    int cy = (y0 + y1) / 2;
    int rx = abs(x1 - x0) / 2;
    int ry = abs(y1 - y0) / 2;
    js_draw_ellipse(cx, cy, rx, ry);
}
```

**Key Differences:**
- KeyKit uses indexed colors (0-15), implemented color mapping to RGB
- KeyKit uses two-point coordinates, converted to Canvas x/y/width/height format
- Added proper coordinate transformation for ellipses

## Pattern 4: Web MIDI API Integration

### From Examples
```javascript
// examples/canvas_library.js
js_request_midi_access: function () {
    navigator.requestMIDIAccess({ sysex: false })
        .then(function(access) {
            window.midiInputs = Array.from(access.inputs.values());
            window.midiOutputs = Array.from(access.outputs.values());

            // Callback to C
            Module.ccall('on_midi_ready', null, [], []);
        });
}
```

### Applied to KeyKit
```javascript
// src/keykit_library.js
js_request_midi_access: function () {
    if (navigator.requestMIDIAccess) {
        navigator.requestMIDIAccess({ sysex: false })
            .then(function(access) {
                window.midiInputs = Array.from(access.inputs.values());
                window.midiOutputs = Array.from(access.outputs.values());

                // Call KeyKit callback
                Module.ccall('mdep_on_midi_ready', null, [], []);
            });
    }
}
```

**C Implementation:**
```c
// src/mdep_wasm.c
EMSCRIPTEN_KEEPALIVE
void mdep_on_midi_ready() {
    printf("MIDI devices ready!\n");
}

int mdep_initmidi(Midiport *inputs, Midiport *outputs) {
    int num_inputs = js_get_midi_input_count();
    int num_outputs = js_get_midi_output_count();

    for (int i = 0; i < num_inputs; i++) {
        js_get_midi_input_name(i, midi_input_names[i], 256);
        inputs[i].name = midi_input_names[i];
        inputs[i].private1 = i;  // Store device index
    }
    // ... same for outputs
}
```

## Pattern 5: MIDI Message Handling

### From Examples
```c
// examples/advanced_canvas_example.c
EMSCRIPTEN_KEEPALIVE
void on_midi_message(int device_index, int status, int data1, int data2) {
    // Decode MIDI message
    int msg_type = (status >> 4) & 0x0F;
    int channel = status & 0x0F;

    // Process message...
}
```

```javascript
// examples/canvas_library.js
input.onmidimessage = function(event) {
    var data = event.data;
    Module.ccall('on_midi_message', null,
                 ['number', 'number', 'number', 'number'],
                 [index, data[0], data[1], data[2]]);
};
```

### Applied to KeyKit
```c
// src/mdep_wasm.c
// MIDI circular buffer for async message storage
static unsigned char midi_input_buffer[1024];
static int midi_buffer_write_pos = 0;
static int midi_buffer_read_pos = 0;
static int midi_buffer_count = 0;

EMSCRIPTEN_KEEPALIVE
void mdep_on_midi_message(int device_index, int status, int data1, int data2) {
    // Add 3 bytes to circular buffer
    if (midi_buffer_count + 3 <= 1024) {
        midi_input_buffer[midi_buffer_write_pos++] = status;
        if (midi_buffer_write_pos >= 1024) midi_buffer_write_pos = 0;

        midi_input_buffer[midi_buffer_write_pos++] = data1;
        if (midi_buffer_write_pos >= 1024) midi_buffer_write_pos = 0;

        midi_input_buffer[midi_buffer_write_pos++] = data2;
        if (midi_buffer_write_pos >= 1024) midi_buffer_write_pos = 0;

        midi_buffer_count += 3;
    }
}

int mdep_getnmidi(char *buff, int buffsize, int *port) {
    // Read from circular buffer
    int bytes_read = 0;
    while (bytes_read < buffsize && midi_buffer_count > 0) {
        buff[bytes_read++] = midi_input_buffer[midi_buffer_read_pos++];
        if (midi_buffer_read_pos >= 1024) midi_buffer_read_pos = 0;
        midi_buffer_count--;
    }
    return bytes_read;
}
```

**Key Innovation:** Circular buffer bridges async JavaScript events with KeyKit's synchronous polling model.

## Pattern 6: Mouse Event Handling

### From Examples
```javascript
// examples/advanced_canvas_example.c (via EM_ASM)
canvas.addEventListener('mousedown', function(e) {
    var rect = canvas.getBoundingClientRect();
    var x = Math.floor(e.clientX - rect.left);
    var y = Math.floor(e.clientY - rect.top);
    Module.ccall('on_mouse_event', null, ['number', 'number', 'number'], [0, x, y]);
});
```

### Applied to KeyKit
```javascript
// src/keykit_library.js
js_setup_mouse_events: function () {
    window.keykitMouseX = 0;
    window.keykitMouseY = 0;
    window.keykitMouseButtons = 0;

    canvas.addEventListener('mousemove', function(e) {
        var rect = canvas.getBoundingClientRect();
        window.keykitMouseX = Math.floor(e.clientX - rect.left);
        window.keykitMouseY = Math.floor(e.clientY - rect.top);
    });

    canvas.addEventListener('mousedown', function(e) {
        window.keykitMouseButtons |= (1 << e.button);
    });

    canvas.addEventListener('mouseup', function(e) {
        window.keykitMouseButtons &= ~(1 << e.button);
    });
}
```

```c
// src/mdep_wasm.c
int mdep_mouse(int *ax, int *ay, int *am) {
    js_get_mouse_state(ax, ay, am);
    return 0;
}
```

**Pattern:** JavaScript maintains global state, C polls it synchronously.

## Pattern 7: Keyboard Input

### Applied to KeyKit
```javascript
// src/keykit_library.js
js_setup_keyboard_events: function () {
    window.keykitKeyBuffer = [];

    document.addEventListener('keydown', function(e) {
        window.keykitKeyBuffer.push(e.keyCode);
    });
}

js_get_key: function () {
    if (window.keykitKeyBuffer && window.keykitKeyBuffer.length > 0) {
        return window.keykitKeyBuffer.shift();
    }
    return -1;
}
```

```c
// src/mdep_wasm.c
int mdep_getconsole(void) {
    return js_get_key();
}

int mdep_statconsole() {
    return js_has_key() ? 1 : 0;
}
```

## Pattern 8: Exported Functions

### From Examples
```python
# examples/build_examples.bat
EXPORTED_FUNCTIONS="['_main','_update_animation','_on_mouse_event','_on_midi_ready','_on_midi_message']"
```

### Applied to KeyKit
```python
# src/build_wasm.py
EXPORTED_FUNCTIONS=['_main','_mdep_on_midi_ready','_mdep_on_midi_message',
                    '_mdep_on_mouse_move','_mdep_on_mouse_button','_mdep_on_key_event']
```

**All callback functions marked with `EMSCRIPTEN_KEEPALIVE` in C code.**

## Pattern 9: Runtime Methods

### From Examples
```python
EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"
```

### Applied to KeyKit
```python
EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString']
```

**Why Additional Methods:**
- `getValue/setValue` - Needed for `js_fill_polygon()` to read C array data
- `UTF8ToString` - Needed to convert C strings to JavaScript strings for device names

## Pattern 10: Custom HTML Shell

### From Examples
```bash
--shell-file canvas_shell.html
```

### Applied to KeyKit
Created `src/keykit_shell.html` with:
- Canvas element: `<canvas id="keykit-canvas" width="1024" height="768">`
- Console output div for printf/logging
- Loading progress bar
- Modern dark theme styling
- Module configuration for Emscripten

## Pattern 11: Color Management

### New Pattern for KeyKit
KeyKit uses indexed colors (0-15). Implemented color palette mapping:

```c
// src/mdep_wasm.c
static void update_color_from_index(int c) {
    switch (c % 16) {
        case 0:  sprintf(current_color_rgb, "rgb(0,0,0)"); break;    // Black
        case 1:  sprintf(current_color_rgb, "rgb(0,0,255)"); break;  // Blue
        case 2:  sprintf(current_color_rgb, "rgb(0,255,0)"); break;  // Green
        // ... 13 more colors
    }
}

void mdep_color(int c) {
    update_color_from_index(c);
    js_set_color(current_color_rgb);
}
```

## Pattern 12: Coordinate System Conversion

### New Pattern for KeyKit
KeyKit graphics functions use two-point format (x0,y0,x1,y1), Canvas uses x,y,width,height:

```c
void mdep_box(int x0, int y0, int x1, int y1) {
    int x = (x0 < x1) ? x0 : x1;
    int y = (y0 < y1) ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);
    js_draw_rect(x, y, w, h);
}
```

## Build Process Changes

### Before
```python
flags = [
    "-o", "keykit.html",
    "-s", "ALLOW_MEMORY_GROWTH=1",
    "-s", "ASYNCIFY=1",
    "-s", "EXPORTED_FUNCTIONS=['_main']",
    "-O2"
]
```

### After
```python
flags = [
    "-o", "keykit.html",
    "--js-library", "keykit_library.js",     # NEW
    "--shell-file", "keykit_shell.html",     # NEW
    "-s", "ALLOW_MEMORY_GROWTH=1",
    "-s", "ASYNCIFY=1",
    "-s", "EXPORTED_FUNCTIONS=['_main','_mdep_on_midi_ready',...]",  # EXPANDED
    "-s", "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString']",  # EXPANDED
    "-lm",   # NEW - math library
    "-g",    # NEW - debug symbols
    "-O0"    # CHANGED - no optimization for debugging
]
```

## Complete Function Mapping

| KeyKit Function | JavaScript Library Function | Notes |
|----------------|----------------------------|-------|
| mdep_line() | js_draw_line() | Direct mapping |
| mdep_box() | js_draw_rect() | Coordinate conversion |
| mdep_boxfill() | js_fill_rect() | Coordinate conversion |
| mdep_ellipse() | js_draw_ellipse() | Bounding box → center+radii |
| mdep_fillellipse() | js_fill_ellipse() | Bounding box → center+radii |
| mdep_fillpolygon() | js_fill_polygon() | Uses getValue() for arrays |
| mdep_string() | js_draw_text() | Direct mapping |
| mdep_color() | js_set_color() | Index → RGB conversion |
| mdep_maxx() | js_get_canvas_width() | Direct mapping |
| mdep_maxy() | js_get_canvas_height() | Direct mapping |
| mdep_plotmode() | js_set_composite_operation() | Maps to Canvas blend modes |
| mdep_startgraphics() | js_setup_mouse_events(), js_setup_keyboard_events() | Initialization |
| mdep_startrealtime() | js_request_midi_access() | Async MIDI init |
| mdep_initmidi() | js_get_midi_*_count(), js_get_midi_*_name() | Device enumeration |
| mdep_midi() | js_open/close_midi_input() | Port management |
| mdep_getnmidi() | (buffer read) | Circular buffer |
| mdep_putnmidi() | js_send_midi_output() | Direct output |
| mdep_mouse() | js_get_mouse_state() | State polling |
| mdep_getconsole() | js_get_key() | Keyboard buffer |
| mdep_statconsole() | js_has_key() | Input availability check |

## Testing the Implementation

1. **Build:**
   ```bash
   cd src
   python build_wasm.py
   ```

2. **Run:**
   ```bash
   python -m http.server 8000
   # Navigate to http://localhost:8000/keykit.html
   ```

3. **Test Graphics:**
   - Lines and boxes should render on canvas
   - Text should appear at correct positions
   - Colors should map correctly

4. **Test MIDI:**
   - Browser requests MIDI permission
   - Devices appear in console log
   - MIDI input creates messages in buffer

5. **Test Mouse:**
   - Move mouse over canvas
   - Coordinates update correctly
   - Button states track clicks

6. **Test Keyboard:**
   - Focus canvas (click on it)
   - Type keys
   - Keys appear in buffer

## Summary

This implementation successfully applies all major patterns from the advanced canvas example to KeyKit:

✅ JavaScript library integration (`--js-library`)
✅ External function declarations
✅ Canvas 2D rendering
✅ Web MIDI API integration
✅ Async-to-sync MIDI message bridging via circular buffer
✅ Mouse event handling with global state
✅ Keyboard event buffering
✅ Custom HTML shell template
✅ Proper function exports
✅ Color palette mapping
✅ Coordinate system conversion

The result is a fully functional graphics and MIDI system that maintains KeyKit's synchronous programming model while leveraging modern web APIs through a clean JavaScript bridge layer.
