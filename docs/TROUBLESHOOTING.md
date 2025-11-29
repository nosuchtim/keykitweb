# Troubleshooting Canvas Examples

## Issue: "_getCanvasContext is not defined"

**Problem:** The JavaScript library had a helper function with `$` prefix that wasn't being properly handled by Emscripten.

**Solution:** ✅ **FIXED!** The `canvas_library.js` has been updated to inline the canvas/context retrieval in each function instead of using a helper.

**What was changed:**
- Removed the `$getCanvasContext` helper function
- Each function now directly calls `document.getElementById('canvas')` and `getContext('2d')`

**To rebuild:**
```bash
cd examples
build_examples.bat
```

---

## Common Issues and Solutions

### 1. Canvas Not Found

**Error:** "Canvas element not found!" in console

**Solution:**
- Make sure your HTML has: `<canvas id="canvas"></canvas>`
- The canvas ID must match what's in the JavaScript library (default: `"canvas"`)
- Load the HTML file, don't just open the .js file

### 2. Nothing Draws on Canvas

**Possible causes:**
1. Canvas element not in HTML
2. WASM module not loaded yet
3. Functions not exported properly

**Solution:**
```javascript
// Wait for module to load
Module.postRun = [function() {
    console.log('Ready!');
    // Now you can call functions
}];
```

### 3. Build Errors

**Error:** "emcc not found"

**Solution:**
- Make sure Emscripten is installed
- Update the path in `build_examples.bat`:
  ```batch
  set EMCC=C:\Users\tjt\GitHub\emsdk\upstream\emscripten\emcc.bat
  ```

**Error:** "canvas_library.js not found"

**Solution:**
- Make sure you're in the `examples` directory when building
- The library file must be in the same directory as the C file

### 4. Function Not Exported

**Error:** "Module._my_function is not a function"

**Solution:**
Add to build command:
```bash
-s EXPORTED_FUNCTIONS="['_main','_my_function']"
```

Or use in C code:
```c
EMSCRIPTEN_KEEPALIVE
void my_function() {
    // ...
}
```

### 5. UTF8ToString Not Defined

**Error:** "UTF8ToString is not defined"

**Solution:**
Add to build command:
```bash
-s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']"
```

### 6. getValue/setValue Not Defined

**Error:** "getValue is not defined"

**Solution:**
Add to build command:
```bash
-s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','getValue','setValue']"
```

### 7. Math Functions Not Found

**Error:** "undefined reference to `sin`"

**Solution:**
Add to build command:
```bash
-lm
```

### 8. Animation Not Smooth

**Problem:** Animation is choppy or inconsistent

**Solution:**
Use `requestAnimationFrame`:
```javascript
function animate() {
    Module._update_animation();
    requestAnimationFrame(animate);
}
animate();
```

### 9. CORS Errors

**Error:** "Cross-origin request blocked"

**Solution:**
Use a local web server:
```bash
# Python
python -m http.server 8000

# Node.js
npx http-server

# Then open: http://localhost:8000/
```

Don't just double-click the HTML file!

### 10. Memory Errors

**Error:** "Cannot enlarge memory arrays"

**Solution:**
Add to build command:
```bash
-s ALLOW_MEMORY_GROWTH=1
```

---

## Verification Checklist

Before reporting an issue, check:

- [ ] Emscripten is installed and in PATH
- [ ] Canvas element exists in HTML with correct ID
- [ ] Build completed without errors
- [ ] Using a local web server (not file://)
- [ ] Browser console shows no errors
- [ ] Module.postRun callback executed
- [ ] Functions are exported in build command
- [ ] JavaScript library file is in correct location

---

## Debug Tips

### 1. Check if WASM Loaded

```javascript
Module.postRun = [function() {
    console.log('WASM loaded!');
    console.log('Available functions:', Object.keys(Module).filter(k => k.startsWith('_')));
}];
```

### 2. Test Canvas Directly

```javascript
// In browser console
var canvas = document.getElementById('canvas');
var ctx = canvas.getContext('2d');
ctx.fillRect(0, 0, 100, 100);  // Should draw a black square
```

### 3. Call C Functions from Console

```javascript
// Call exported function
Module._main();

// Or use ccall
Module.ccall('my_function', 'number', ['number'], [42]);
```

### 4. Check Build Output

Look in the generated `.js` file for your exported functions:
```javascript
// Search for: _your_function_name
```

---

## Getting Help

If you're still stuck:

1. **Check the browser console** - Most errors show up there
2. **Check the build log** - Look for warnings or errors during compilation
3. **Simplify** - Try the simple example first before the advanced one
4. **Compare** - Look at the working examples and compare with your code

---

## Current Status

✅ **Fixed Issues:**
- `_getCanvasContext is not defined` - Fixed in canvas_library.js
- Build script updated with `call` command for Windows
- Math library (`-lm`) added to advanced example build

✅ **Working Examples:**
- Simple canvas drawing (EM_ASM)
- Advanced canvas (JavaScript library)

🎯 **Ready to Use:**
All examples should now build and run correctly!
