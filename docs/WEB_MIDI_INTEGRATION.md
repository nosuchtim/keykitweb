# Web MIDI API Integration

This document explains how the advanced canvas example integrates with the Web MIDI API to detect and list MIDI devices in the browser.

## Overview

The `advanced_canvas_example.c` demonstrates how to use the Web MIDI API from C/WebAssembly code through JavaScript interop. It detects MIDI input and output devices and displays them on the HTML5 canvas.

## How It Works

### 1. C Code Interface (advanced_canvas_example.c)

The C code declares external JavaScript functions:

```c
extern int js_get_midi_input_count();
extern int js_get_midi_output_count();
extern void js_get_midi_input_name(int index, char *buffer, int buffer_size);
extern void js_get_midi_output_name(int index, char *buffer, int buffer_size);
extern void js_request_midi_access();
```

These are called from C like normal functions:

```c
request_midi_access();  // Request MIDI permission
int count = get_midi_input_count();  // Get device count
get_midi_input_name(0, buffer, 256);  // Get device name
```

**Important:** The Web MIDI API is asynchronous. When `js_request_midi_access()` is called, it returns immediately, but devices won't be available until the browser grants permission and enumerates them. The example handles this by:

1. Calling `request_midi_access()` early in `main()`
2. Drawing the initial UI (which shows "No MIDI devices found")
3. JavaScript calls back to `on_midi_ready()` when devices are ready
4. `on_midi_ready()` redraws the MIDI device list with actual devices

### 2. JavaScript Implementation (canvas_library.js)

The JavaScript library implements these functions using the Web MIDI API:

- **`js_request_midi_access()`** - Calls `navigator.requestMIDIAccess()` to request permission. When the Promise resolves, it calls back to the C function `on_midi_ready()`
- **`js_get_midi_input_count()`** - Returns the number of MIDI input devices
- **`js_get_midi_output_count()`** - Returns the number of MIDI output devices
- **`js_get_midi_input_name()`** - Copies a device name into a C string buffer
- **`js_get_midi_output_name()`** - Copies a device name into a C string buffer

### 3. Callback Pattern

The example uses a callback pattern to handle async MIDI device enumeration:

```c
// In advanced_canvas_example.c
EMSCRIPTEN_KEEPALIVE
void on_midi_ready() {
  printf("MIDI devices ready!\n");
  draw_midi_devices(50, 600);  // Redraw with actual devices
}
```

```javascript
// In canvas_library.js
navigator.requestMIDIAccess({ sysex: false })
  .then(function(access) {
    window.midiInputs = Array.from(access.inputs.values());
    window.midiOutputs = Array.from(access.outputs.values());

    // Call back into C code
    Module.ccall('on_midi_ready', null, [], []);
  });
```

### 4. Building

Build the example with:

```bash
cd examples
build_examples.bat
```

Or manually:

```bash
emcc advanced_canvas_example.c -o advanced_canvas_example.html \
    --js-library canvas_library.js \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS="['_main','_update_animation','_on_mouse_event','_on_midi_ready']" \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    --shell-file canvas_shell.html \
    -lm -g -O0
```

### 5. Running

1. Start a local web server:
   ```bash
   python -m http.server 8000
   ```

2. Navigate to: `http://localhost:8000/advanced_canvas_example.html`

3. **Grant MIDI permissions** when prompted by the browser

4. The canvas will display:
   - All detected MIDI input devices
   - All detected MIDI output devices
   - "No MIDI inputs/outputs found" if none are available

## Browser Compatibility

The Web MIDI API is supported in:
- Chrome/Chromium (desktop and Android)
- Edge (Chromium-based)
- Opera

**Not supported in:**
- Firefox (requires flag to enable)
- Safari
- iOS browsers

## Security Notes

- Web MIDI requires HTTPS in production (or localhost for development)
- The browser will prompt the user for permission to access MIDI devices
- MIDI access is asynchronous - devices may not be available immediately when `main()` runs
- The example handles the case where no devices are found gracefully

## Adapting for KeyKit

This pattern can be used in the main KeyKit WebAssembly port:

1. Add the Web MIDI functions to the JavaScript library
2. Implement `mdep_initmidi()` to call `js_request_midi_access()`
3. Implement MIDI I/O functions to send/receive MIDI data through the Web MIDI API
4. Handle async initialization (MIDI access is a Promise)

Example integration in `mdep_wasm.c`:

```c
int mdep_initmidi(Midiport *inputs, Midiport *outputs) {
    js_request_midi_access();

    // Populate input ports
    int input_count = js_get_midi_input_count();
    for (int i = 0; i < input_count && i < MIDI_IN_DEVICES; i++) {
        char name[256];
        js_get_midi_input_name(i, name, sizeof(name));
        inputs[i].name = strdup(name);  // Allocate name
        inputs[i].opened = 0;
        inputs[i].private1 = i;
    }

    // Populate output ports
    int output_count = js_get_midi_output_count();
    for (int i = 0; i < output_count && i < MIDI_OUT_DEVICES; i++) {
        char name[256];
        js_get_midi_output_name(i, name, sizeof(name));
        outputs[i].name = strdup(name);
        outputs[i].opened = 0;
        outputs[i].private1 = i;
    }

    return 0;
}
```

## Debugging

To see MIDI activity in the browser console:

1. Open Developer Tools (F12)
2. Look for console messages:
   - "Requesting MIDI access..."
   - "MIDI access granted!"
   - "Found N MIDI inputs"
   - "Found N MIDI outputs"
   - Device names listed

## Testing Without Real MIDI Devices

If you don't have physical MIDI devices:

1. **Windows**: Use loopMIDI or similar virtual MIDI cable software
2. **macOS**: Use IAC Driver (built-in)
3. **Linux**: Use `virmidi` kernel module or JACK
4. **Web-based**: Some DAWs and synthesizers running in other browser tabs may create virtual MIDI ports

## Next Steps

- Implement MIDI input event handling (`onmidimessage`)
- Implement MIDI output (`port.send()`)
- Handle hot-plugging of MIDI devices (device connect/disconnect events)
- Add SysEx support if needed
