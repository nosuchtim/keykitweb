# Canvas Drawing from C/WebAssembly - Complete Guide

## Overview

This guide shows you how to draw graphics on an HTML Canvas from C code compiled to WebAssembly using Emscripten.

## Three Methods to Draw on Canvas

### Method 1: Inline JavaScript with EM_ASM (Easiest)

**Pros:**
- Simple and straightforward
- No external files needed
- Good for simple graphics

**Cons:**
- Less efficient for complex operations
- JavaScript code is embedded in C

**Example:**
```c
#include <emscripten.h>

void draw_line(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.moveTo($0, $1);
        ctx.lineTo($2, $3);
        ctx.stroke();
    }, x0, y0, x1, y1);
}
```

### Method 2: JavaScript Library Functions (Recommended)

**Pros:**
- More efficient
- Cleaner separation of C and JavaScript
- Reusable functions
- Better for complex graphics

**Cons:**
- Requires separate .js library file
- Slightly more setup

**Example:**
```c
// Declare external function
extern void js_draw_line(int x0, int y0, int x1, int y1);

// Use it
void my_function() {
    js_draw_line(10, 10, 100, 100);
}
```

### Method 3: SDL2 (For Game Development)

**Pros:**
- Cross-platform (works on desktop too)
- Full game development features
- Hardware acceleration

**Cons:**
- Larger file size
- More complex setup

## Building the Examples

### Example 1: Simple Canvas Drawing

```bash
# Using Method 1 (EM_ASM)
emcc canvas_drawing_example.c -o canvas_drawing_example.html \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS='["_main","_clear_canvas"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
```

### Example 2: Advanced Drawing with Library

```bash
# Using Method 2 (JavaScript Library)
emcc advanced_canvas_example.c -o advanced_canvas_example.html \
    --js-library canvas_library.js \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_FUNCTIONS='["_main","_update_animation"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
```

## HTML Template

```html
<!DOCTYPE html>
<html>
<head>
    <title>Canvas Drawing</title>
</head>
<body>
    <canvas id="canvas" width="800" height="600"></canvas>
    <script>
        var Module = {
            canvas: document.getElementById('canvas')
        };
    </script>
    <script src="your_compiled_code.js"></script>
</body>
</html>
```

## Applying to Your KeyKit Project

### Step 1: Update mdep_wasm.c

Replace the graphics stubs with actual implementations:

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

void mdep_box(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        ctx.strokeRect($0, $1, $2 - $0, $3 - $1);
    }, x0, y0, x1, y1);
}

void mdep_boxfill(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        ctx.fillRect($0, $1, $2 - $0, $3 - $1);
    }, x0, y0, x1, y1);
}

void mdep_string(int x, int y, char *s) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        ctx.fillText(UTF8ToString($2), $0, $1);
    }, x, y, s);
}

void mdep_color(int c) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        
        // Convert color index to RGB
        var colors = [
            '#000000', '#FF0000', '#00FF00', '#0000FF',
            '#FFFF00', '#FF00FF', '#00FFFF', '#FFFFFF'
            // Add more colors as needed
        ];
        
        var color = colors[$0 % colors.length];
        ctx.strokeStyle = color;
        ctx.fillStyle = color;
    }, c);
}

void mdep_ellipse(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        
        var centerX = ($0 + $2) / 2;
        var centerY = ($1 + $3) / 2;
        var radiusX = Math.abs($2 - $0) / 2;
        var radiusY = Math.abs($3 - $1) / 2;
        
        ctx.beginPath();
        ctx.ellipse(centerX, centerY, radiusX, radiusY, 0, 0, 2 * Math.PI);
        ctx.stroke();
    }, x0, y0, x1, y1);
}

void mdep_fillellipse(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        
        var centerX = ($0 + $2) / 2;
        var centerY = ($1 + $3) / 2;
        var radiusX = Math.abs($2 - $0) / 2;
        var radiusY = Math.abs($3 - $1) / 2;
        
        ctx.beginPath();
        ctx.ellipse(centerX, centerY, radiusX, radiusY, 0, 0, 2 * Math.PI);
        ctx.fill();
    }, x0, y0, x1, y1);
}

void mdep_fillpolygon(int *x, int *y, int n) {
    EM_ASM({
        var canvas = document.getElementById('keykit-canvas');
        var ctx = canvas.getContext('2d');
        var numPoints = $2;
        
        if (numPoints < 3) return;
        
        ctx.beginPath();
        
        // Get first point
        var firstX = getValue($0, 'i32');
        var firstY = getValue($1, 'i32');
        ctx.moveTo(firstX, firstY);
        
        // Draw to remaining points
        for (var i = 1; i < numPoints; i++) {
            var px = getValue($0 + i*4, 'i32');
            var py = getValue($1 + i*4, 'i32');
            ctx.lineTo(px, py);
        }
        
        ctx.closePath();
        ctx.fill();
    }, x, y, n);
}
```

### Step 2: Update build_wasm.py

Add the canvas ID to the build:

```python
flags = [
    emcc,
    "-I.",
    "-o", "keykit.html",
    "-s", "ALLOW_MEMORY_GROWTH=1",
    "-s", "ASYNCIFY=1",
    "-s", "EXPORTED_FUNCTIONS=['_main']",
    "-s", "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
    "-D__EMSCRIPTEN__",
    # ... other flags
]
```

### Step 3: Create HTML Interface

Create `keykit.html` template:

```html
<!DOCTYPE html>
<html>
<head>
    <title>KeyKit WebAssembly</title>
    <style>
        #keykit-canvas {
            border: 2px solid #333;
            background: white;
        }
    </style>
</head>
<body>
    <h1>KeyKit</h1>
    <canvas id="keykit-canvas" width="1024" height="768"></canvas>
    
    <script>
        var Module = {
            canvas: document.getElementById('keykit-canvas'),
            print: function(text) { console.log(text); },
            printErr: function(text) { console.error(text); }
        };
    </script>
    <script src="keykit.js"></script>
</body>
</html>
```

## Performance Tips

1. **Batch Operations**: Group multiple drawing operations together
2. **Use requestAnimationFrame**: For smooth animations
3. **Minimize EM_ASM Calls**: Use JavaScript library functions for frequently called operations
4. **Cache Canvas Context**: Store the context in a global variable if possible
5. **Use Typed Arrays**: For large data transfers between C and JavaScript

## Common Pitfalls

1. **Canvas ID Mismatch**: Make sure the canvas ID in HTML matches the ID in your C code
2. **Coordinate System**: Canvas uses top-left origin, not bottom-left
3. **Memory Leaks**: Be careful with string conversions (UTF8ToString)
4. **Async Issues**: Some operations may need ASYNCIFY=1
5. **Color Format**: Canvas uses CSS color strings, not color indices

## Testing Your Implementation

1. Start with simple shapes (lines, rectangles)
2. Test color changes
3. Add text rendering
4. Test complex shapes (ellipses, polygons)
5. Add animations

## Resources

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Canvas API Reference](https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API)
- [EM_ASM Documentation](https://emscripten.org/docs/api_reference/emscripten.h.html#c.EM_ASM)
- [JavaScript Library Integration](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#implement-c-in-javascript)
