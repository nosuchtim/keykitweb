# Quick Reference: Implementing Real Functionality

This guide shows how to replace the stubs with real implementations.

## Graphics Functions (Using HTML5 Canvas)

### Example: Implementing mdep_line()

```c
#include <emscripten.h>

void mdep_line(int x0, int y0, int x1, int y1)
{
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

### Example: Implementing mdep_string()

```c
void mdep_string(int x, int y, char *s)
{
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        ctx.fillText(UTF8ToString($2), $0, $1);
    }, x, y, s);
}
```

## MIDI Functions (Using Web MIDI API)

### JavaScript Side (in keykit_bridge.js)

```javascript
// Global MIDI state
let midiAccess = null;
let midiInputs = [];
let midiOutputs = [];
let midiInputBuffer = [];

// Initialize Web MIDI API
async function initWebMIDI() {
    try {
        midiAccess = await navigator.requestMIDIAccess();
        
        // Collect inputs
        midiAccess.inputs.forEach(input => {
            midiInputs.push(input);
            input.onmidimessage = handleMIDIMessage;
        });
        
        // Collect outputs
        midiAccess.outputs.forEach(output => {
            midiOutputs.push(output);
        });
        
        return true;
    } catch (e) {
        console.error('Web MIDI API not supported:', e);
        return false;
    }
}

// Handle incoming MIDI messages
function handleMIDIMessage(event) {
    // Add to buffer for mdep_getnmidi to read
    for (let i = 0; i < event.data.length; i++) {
        midiInputBuffer.push(event.data[i]);
    }
}

// Send MIDI message
function sendMIDI(deviceIndex, data) {
    if (deviceIndex < midiOutputs.length) {
        midiOutputs[deviceIndex].send(data);
    }
}
```

### C Side Implementation

```c
#include <emscripten.h>

int mdep_getnmidi(char *buff, int buffsize, int *port)
{
    // Get MIDI data from JavaScript buffer
    int count = EM_ASM_INT({
        var buffer = Module.midiInputBuffer || [];
        var max = Math.min(buffer.length, $1);
        for (var i = 0; i < max; i++) {
            setValue($0 + i, buffer[i], 'i8');
        }
        Module.midiInputBuffer = buffer.slice(max);
        return max;
    }, buff, buffsize);
    
    if (port && count > 0)
        *port = 0; // Could track which port the data came from
    
    return count;
}

void mdep_putnmidi(int n, char *cp, Midiport *pport)
{
    int deviceIndex = pport->private1;
    
    EM_ASM({
        var data = [];
        for (var i = 0; i < $1; i++) {
            data.push(getValue($0 + i, 'i8'));
        }
        Module.sendMIDI($2, data);
    }, cp, n, deviceIndex);
}
```

## Mouse/Keyboard Input

### JavaScript Side

```javascript
let mouseX = 0, mouseY = 0, mouseButtons = 0;
let keyQueue = [];

// Set up event handlers
function setupInput() {
    const canvas = document.getElementById('keykit-canvas');
    
    canvas.addEventListener('mousemove', (e) => {
        const rect = canvas.getBoundingClientRect();
        mouseX = e.clientX - rect.left;
        mouseY = e.clientY - rect.top;
    });
    
    canvas.addEventListener('mousedown', (e) => {
        mouseButtons |= (1 << e.button);
    });
    
    canvas.addEventListener('mouseup', (e) => {
        mouseButtons &= ~(1 << e.button);
    });
    
    document.addEventListener('keydown', (e) => {
        keyQueue.push(e.keyCode);
    });
}
```

### C Side

```c
int mdep_mouse(int *ax, int *ay, int *am)
{
    *ax = EM_ASM_INT({ return Module.mouseX || 0; });
    *ay = EM_ASM_INT({ return Module.mouseY || 0; });
    *am = EM_ASM_INT({ return Module.mouseButtons || 0; });
    return 0;
}

int mdep_getconsole(void)
{
    return EM_ASM_INT({
        var queue = Module.keyQueue || [];
        if (queue.length > 0) {
            return queue.shift();
        }
        return -1;
    });
}
```

## File System (Using Emscripten FS)

### Pre-loading Files

In your build script or HTML:

```javascript
Module.preRun = [function() {
    // Create virtual file system
    FS.mkdir('/keykit');
    FS.mkdir('/keykit/lib');
    
    // Pre-load library files
    FS.writeFile('/keykit/lib/stdlib.k', libraryFileContent);
}];
```

### C Side (Already Works!)

The file functions like `fopen`, `fread`, etc. work automatically with Emscripten's virtual FS.

## HTML Template

```html
<!DOCTYPE html>
<html>
<head>
    <title>KeyKit WebAssembly</title>
    <style>
        #keykit-canvas {
            border: 1px solid black;
            cursor: crosshair;
        }
    </style>
</head>
<body>
    <h1>KeyKit</h1>
    <canvas id="keykit-canvas" width="1024" height="768"></canvas>
    
    <script src="keykit_bridge.js"></script>
    <script src="keykit.js"></script>
    
    <script>
        // Module configuration
        var Module = {
            preRun: [initWebMIDI, setupInput],
            postRun: [],
            print: function(text) {
                console.log(text);
            },
            printErr: function(text) {
                console.error(text);
            },
            canvas: document.getElementById('keykit-canvas'),
            
            // Expose functions for C to call
            midiInputBuffer: [],
            mouseX: 0,
            mouseY: 0,
            mouseButtons: 0,
            keyQueue: []
        };
    </script>
</body>
</html>
```

## Build Command with All Features

```bash
emcc -I. -o keykit.html \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASYNCIFY=1 \
    -s EXPORTED_FUNCTIONS='["_main"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","FS"]' \
    -s FORCE_FILESYSTEM=1 \
    --preload-file lib@/keykit/lib \
    -D__EMSCRIPTEN__ \
    -O2 \
    main.c util.c misc.c phrase.c sym.c keyto.c yacc.c \
    code.c code2.c grid.c view.c menu.c task.c fifo.c \
    mfin.c real.c kwind.c fsm.c bltin.c meth.c regex.c \
    mdep_wasm.c
```

## Testing Incrementally

1. **Test compilation**: `python build_wasm.py`
2. **Test loading**: Open keykit.html in browser, check console
3. **Test graphics**: Implement one drawing function, test it
4. **Test input**: Implement mouse, verify coordinates
5. **Test MIDI**: Implement MIDI I/O, test with virtual MIDI device
6. **Test file system**: Load a simple KeyKit script

## Common Issues and Solutions

### Issue: "Cannot call unknown function"
**Solution**: Add function to EXPORTED_FUNCTIONS

### Issue: Graphics not appearing
**Solution**: Make sure Canvas context is initialized before drawing

### Issue: MIDI not working
**Solution**: Web MIDI requires HTTPS or localhost, and user permission

### Issue: Files not found
**Solution**: Use `--preload-file` or create files in FS.mkdir/writeFile

### Issue: Async functions blocking
**Solution**: Make sure ASYNCIFY=1 is set, or refactor to use callbacks

## Performance Tips

1. Batch Canvas operations when possible
2. Use `requestAnimationFrame()` for graphics updates
3. Minimize JavaScript ↔ C calls in tight loops
4. Use typed arrays for large data transfers
5. Enable optimization: `-O2` or `-O3`

## Debugging

```javascript
// In browser console
Module.ccall('some_function', 'number', ['number'], [42]);

// Print C stack trace
Module.printErr(new Error().stack);

// Inspect memory
Module.HEAP8[address]
Module.HEAPU8[address]
Module.HEAP32[address >> 2]
```

## Resources

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Web MIDI API](https://www.w3.org/TR/webmidi/)
- [Canvas API](https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API)
- [Emscripten File System](https://emscripten.org/docs/api_reference/Filesystem-API.html)
