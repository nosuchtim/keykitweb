# Understanding EM_ASM - Complete Guide

## What is EM_ASM?

`EM_ASM` is an Emscripten macro that lets you **write JavaScript code directly inside your C/C++ code**. When compiled to WebAssembly, this JavaScript code runs in the browser alongside your compiled C code.

Think of it as a "portal" from C to JavaScript.

## Basic Syntax

```c
#include <emscripten.h>

void example() {
    EM_ASM(
        // JavaScript code goes here
        console.log('Hello from JavaScript!');
    );
}
```

## The EM_ASM Family

There are several variants, each for different use cases:

### 1. EM_ASM - No parameters, no return value

```c
void simple_example() {
    EM_ASM(
        console.log('This is JavaScript');
        alert('Hello!');
    );
}
```

**Use when:** You just want to execute JavaScript without passing data or getting results.

### 2. EM_ASM_ - With parameters, no return value

```c
void draw_line(int x0, int y0, int x1, int y1) {
    EM_ASM({
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.moveTo($0, $1);  // $0 = x0, $1 = y0
        ctx.lineTo($2, $3);  // $2 = x1, $3 = y1
        ctx.stroke();
    }, x0, y0, x1, y1);  // Pass C variables to JavaScript
}
```

**Use when:** You need to pass C variables to JavaScript but don't need a return value.

**Key Points:**
- Use `{` and `}` around the JavaScript code
- Parameters are accessed as `$0`, `$1`, `$2`, etc.
- List C variables after the closing brace

### 3. EM_ASM_INT - Returns an integer

```c
int get_canvas_width() {
    return EM_ASM_INT({
        var canvas = document.getElementById('canvas');
        return canvas.width;  // JavaScript returns a value
    });
}

int add_numbers(int a, int b) {
    return EM_ASM_INT({
        return $0 + $1;  // JavaScript does the math
    }, a, b);
}
```

**Use when:** You need an integer result from JavaScript.

**Returns:** 32-bit integer

### 4. EM_ASM_DOUBLE - Returns a floating-point number

```c
double get_aspect_ratio() {
    return EM_ASM_DOUBLE({
        var canvas = document.getElementById('canvas');
        return canvas.width / canvas.height;
    });
}

double calculate_pi() {
    return EM_ASM_DOUBLE({
        return Math.PI;
    });
}
```

**Use when:** You need a floating-point result from JavaScript.

**Returns:** 64-bit double

### 5. EM_ASM_PTR - Returns a pointer (rarely used)

```c
char* get_string_from_js() {
    return (char*)EM_ASM_PTR({
        var str = "Hello from JS";
        var len = lengthBytesUTF8(str) + 1;
        var ptr = _malloc(len);
        stringToUTF8(str, ptr, len);
        return ptr;
    });
}
```

**Use when:** You need to return a pointer from JavaScript (advanced use).

## Parameter Passing - The $0, $1, $2 System

### How Parameters Work

```c
void example(int a, int b, char* str, double d) {
    EM_ASM({
        console.log('Integer a:', $0);      // First parameter
        console.log('Integer b:', $1);      // Second parameter
        console.log('String:', UTF8ToString($2));  // Third parameter (pointer)
        console.log('Double:', $3);         // Fourth parameter
    }, a, b, str, d);
}
```

### Parameter Types

| C Type | JavaScript Access | Notes |
|--------|------------------|-------|
| `int` | `$0` | Direct access |
| `long` | `$0` | Direct access |
| `float` | `$0` | Direct access |
| `double` | `$0` | Direct access |
| `char*` | `UTF8ToString($0)` | Need to convert pointer to string |
| `void*` | `$0` | Raw pointer value |
| Arrays | See below | Need special handling |

## Working with Strings

### Passing C String to JavaScript

```c
void print_message(char* msg) {
    EM_ASM({
        // Convert C string pointer to JavaScript string
        var str = UTF8ToString($0);
        console.log('Message:', str);
    }, msg);
}

// Usage
print_message("Hello World");
```

### Getting String from JavaScript to C

```c
char* get_user_input() {
    // This is complex - usually better to use emscripten_run_script_string()
    return emscripten_run_script_string("prompt('Enter text:')");
}
```

## Working with Arrays

### Passing Array to JavaScript

```c
void process_array(int* arr, int length) {
    EM_ASM({
        var length = $1;
        console.log('Array values:');
        for (var i = 0; i < length; i++) {
            // getValue reads from WASM memory
            var value = getValue($0 + i*4, 'i32');
            console.log('  [' + i + ']:', value);
        }
    }, arr, length);
}

// Usage
int numbers[] = {10, 20, 30, 40, 50};
process_array(numbers, 5);
```

### Reading/Writing WASM Memory from JavaScript

```c
void modify_array(int* arr, int length) {
    EM_ASM({
        var length = $1;
        for (var i = 0; i < length; i++) {
            // Read value
            var value = getValue($0 + i*4, 'i32');
            
            // Modify it
            value = value * 2;
            
            // Write back
            setValue($0 + i*4, value, 'i32');
        }
    }, arr, length);
}
```

**Memory Access Functions:**
- `getValue(ptr, type)` - Read from memory
- `setValue(ptr, value, type)` - Write to memory

**Types:** `'i8'`, `'i16'`, `'i32'`, `'i64'`, `'float'`, `'double'`

## Complete Examples

### Example 1: Canvas Drawing

```c
#include <emscripten.h>

void draw_rectangle(int x, int y, int width, int height, char* color) {
    EM_ASM({
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');
        
        // Set color
        ctx.fillStyle = UTF8ToString($4);
        
        // Draw rectangle
        ctx.fillRect($0, $1, $2, $3);
    }, x, y, width, height, color);
}

// Usage
draw_rectangle(50, 50, 100, 80, "red");
```

### Example 2: Getting Browser Information

```c
#include <emscripten.h>
#include <stdio.h>

void print_browser_info() {
    int width = EM_ASM_INT({
        return window.innerWidth;
    });
    
    int height = EM_ASM_INT({
        return window.innerHeight;
    });
    
    char* user_agent = emscripten_run_script_string(
        "navigator.userAgent"
    );
    
    printf("Window size: %d x %d\n", width, height);
    printf("User agent: %s\n", user_agent);
}
```

### Example 3: DOM Manipulation

```c
void create_button(char* text, char* id) {
    EM_ASM({
        var button = document.createElement('button');
        button.textContent = UTF8ToString($0);
        button.id = UTF8ToString($1);
        button.onclick = function() {
            alert('Button clicked!');
        };
        document.body.appendChild(button);
    }, text, id);
}

// Usage
create_button("Click Me", "my-button");
```

### Example 4: Async Operations

```c
void delayed_action(int delay_ms) {
    EM_ASM({
        setTimeout(function() {
            console.log('Action executed after ' + $0 + 'ms');
        }, $0);
    }, delay_ms);
}
```

### Example 5: Complex Data Structures

```c
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rectangle;

void draw_rect_struct(Rectangle* rect, char* color) {
    EM_ASM({
        // Read struct fields from memory
        var x = getValue($0, 'i32');
        var y = getValue($0 + 4, 'i32');
        var width = getValue($0 + 8, 'i32');
        var height = getValue($0 + 12, 'i32');
        
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');
        ctx.fillStyle = UTF8ToString($1);
        ctx.fillRect(x, y, width, height);
    }, rect, color);
}
```

## Best Practices

### ✅ DO:

1. **Use for simple JavaScript operations**
   ```c
   EM_ASM(console.log('Debug message'));
   ```

2. **Keep JavaScript code simple**
   ```c
   EM_ASM({
       var canvas = document.getElementById('canvas');
       canvas.width = $0;
   }, new_width);
   ```

3. **Use appropriate variant**
   - `EM_ASM` - No params, no return
   - `EM_ASM_INT` - Return integer
   - `EM_ASM_DOUBLE` - Return float

### ❌ DON'T:

1. **Don't use for complex logic**
   - Move complex JavaScript to separate .js files
   - Use `--js-library` instead

2. **Don't forget UTF8ToString for strings**
   ```c
   // WRONG
   EM_ASM({ console.log($0); }, str);
   
   // RIGHT
   EM_ASM({ console.log(UTF8ToString($0)); }, str);
   ```

3. **Don't ignore memory layout**
   ```c
   // Be careful with struct alignment!
   ```

## Performance Considerations

### EM_ASM is Slower Than:
1. Pure C code
2. JavaScript library functions (`--js-library`)
3. Imported JavaScript functions

### When to Use EM_ASM:
- ✅ Prototyping
- ✅ Simple operations
- ✅ One-off JavaScript calls
- ✅ DOM manipulation

### When NOT to Use EM_ASM:
- ❌ In tight loops
- ❌ For frequently called functions
- ❌ For complex operations
- ❌ When performance matters

**Better Alternative for Performance:**
```c
// Instead of EM_ASM in a loop:
for (int i = 0; i < 1000; i++) {
    EM_ASM({ /* draw something */ });  // SLOW!
}

// Use JavaScript library function:
extern void js_draw_batch(int* data, int count);
js_draw_batch(data, 1000);  // FAST!
```

## Debugging EM_ASM

### Print Values

```c
void debug_example(int value) {
    EM_ASM({
        console.log('C value:', $0);
        console.log('Type:', typeof $0);
    }, value);
}
```

### Check for Errors

```c
void safe_example() {
    EM_ASM({
        try {
            // Your JavaScript code
            var canvas = document.getElementById('canvas');
            if (!canvas) {
                console.error('Canvas not found!');
                return;
            }
            // ... rest of code
        } catch (e) {
            console.error('Error:', e);
        }
    });
}
```

## Common Pitfalls

### 1. Forgetting Curly Braces

```c
// WRONG - No braces with parameters
EM_ASM(
    console.log($0);
, value);

// RIGHT
EM_ASM({
    console.log($0);
}, value);
```

### 2. Wrong Parameter Count

```c
// WRONG - 3 parameters but only 2 provided
EM_ASM({
    console.log($0, $1, $2);
}, a, b);  // Missing third parameter!

// RIGHT
EM_ASM({
    console.log($0, $1, $2);
}, a, b, c);
```

### 3. String Conversion

```c
// WRONG
EM_ASM({ alert($0); }, "Hello");

// RIGHT
EM_ASM({ alert(UTF8ToString($0)); }, "Hello");
```

## Summary

| Macro | Use Case | Returns | Example |
|-------|----------|---------|---------|
| `EM_ASM` | Simple JS, no params | void | `EM_ASM(alert('Hi'));` |
| `EM_ASM_` | JS with params | void | `EM_ASM_({log($0);}, x);` |
| `EM_ASM_INT` | Get integer | int | `int w = EM_ASM_INT({return 100;});` |
| `EM_ASM_DOUBLE` | Get float | double | `double pi = EM_ASM_DOUBLE({return Math.PI;});` |

**Key Takeaway:** `EM_ASM` is a powerful tool for quick JavaScript integration, but use it wisely. For performance-critical code or complex operations, use JavaScript library functions instead!
