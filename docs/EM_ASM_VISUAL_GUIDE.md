# EM_ASM Visual Guide

## The Big Picture

```
┌─────────────────────────────────────────────────────────────┐
│                      Your C Code                            │
│                                                             │
│  void draw_line(int x0, int y0, int x1, int y1) {          │
│      EM_ASM({                                               │
│          ┌──────────────────────────────────────┐          │
│          │   JavaScript Code Runs Here          │          │
│          │                                       │          │
│          │   var ctx = canvas.getContext('2d'); │          │
│          │   ctx.moveTo($0, $1);                │          │
│          │   ctx.lineTo($2, $3);                │          │
│          │   ctx.stroke();                      │          │
│          └──────────────────────────────────────┘          │
│      }, x0, y0, x1, y1);  ← Parameters passed here         │
│         ↑                                                   │
│         └── Curly braces needed when passing parameters    │
│  }                                                          │
└─────────────────────────────────────────────────────────────┘
```

## The Four Variants

```
┌────────────────┬─────────────┬──────────────┬─────────────────────┐
│ Macro          │ Parameters? │ Return Type  │ Syntax              │
├────────────────┼─────────────┼──────────────┼─────────────────────┤
│ EM_ASM         │ No          │ void         │ EM_ASM(             │
│                │             │              │   js_code           │
│                │             │              │ );                  │
├────────────────┼─────────────┼──────────────┼─────────────────────┤
│ EM_ASM_        │ Yes         │ void         │ EM_ASM_({           │
│ (with braces)  │             │              │   js_code           │
│                │             │              │ }, p1, p2);         │
├────────────────┼─────────────┼──────────────┼─────────────────────┤
│ EM_ASM_INT     │ Optional    │ int          │ int n = EM_ASM_INT({│
│                │             │              │   return value;     │
│                │             │              │ });                 │
├────────────────┼─────────────┼──────────────┼─────────────────────┤
│ EM_ASM_DOUBLE  │ Optional    │ double       │ double d =          │
│                │             │              │   EM_ASM_DOUBLE({   │
│                │             │              │   return value;     │
│                │             │              │ });                 │
└────────────────┴─────────────┴──────────────┴─────────────────────┘
```

## Parameter Flow

```
C Code                          JavaScript Code
──────                          ───────────────

int x = 10;                     
int y = 20;                     
char* msg = "Hi";               

EM_ASM_({                       
                                var x_val = $0;  ← Gets 10
                                var y_val = $1;  ← Gets 20
                                var str = UTF8ToString($2);  ← Gets "Hi"
                                
                                console.log(x_val, y_val, str);
}, x, y, msg);
   ↑  ↑  ↑
   │  │  └─── Third parameter  → $2
   │  └────── Second parameter → $1
   └───────── First parameter  → $0
```

## String Conversion Flow

```
C Side                          JavaScript Side
──────                          ───────────────

char* str = "Hello";            
                                
EM_ASM_({                       
                                // $0 is a POINTER (number)
                                // NOT the actual string!
                                
                                var jsString = UTF8ToString($0);
                                              ↑
                                              └── Converts pointer to string
                                
                                console.log(jsString);  // "Hello"
}, str);
```

## Memory Access Flow

```
C Side                          JavaScript Side
──────                          ───────────────

int arr[] = {1, 2, 3};          
                                
EM_ASM_({                       
                                // $0 is pointer to array
                                
                                // Read first element
                                var val0 = getValue($0, 'i32');
                                           ↑        ↑    ↑
                                           │        │    └── Type
                                           │        └─────── Pointer
                                           └────────────── Read function
                                
                                // Read second element (4 bytes later)
                                var val1 = getValue($0 + 4, 'i32');
                                
                                // Write to third element
                                setValue($0 + 8, 99, 'i32');
                                        ↑        ↑   ↑   ↑
                                        │        │   │   └── Type
                                        │        │   └────── New value
                                        │        └────────── Pointer
                                        └─────────────────── Write function
}, arr);
```

## Common Patterns

### Pattern 1: Simple Action
```c
EM_ASM(
    console.log('Debug message');
);
```

### Pattern 2: Action with Data
```c
EM_ASM({
    console.log('Value:', $0);
}, my_value);
```

### Pattern 3: Get Information
```c
int width = EM_ASM_INT({
    return window.innerWidth;
});
```

### Pattern 4: Canvas Drawing
```c
EM_ASM({
    var ctx = document.getElementById('canvas').getContext('2d');
    ctx.fillRect($0, $1, $2, $3);
}, x, y, width, height);
```

## Type Conversion Table

```
┌──────────────┬─────────────────┬──────────────────────────────┐
│ C Type       │ JS Access       │ Notes                        │
├──────────────┼─────────────────┼──────────────────────────────┤
│ int          │ $0              │ Direct use                   │
│ long         │ $0              │ Direct use                   │
│ float        │ $0              │ Direct use                   │
│ double       │ $0              │ Direct use                   │
│ char*        │ UTF8ToString($0)│ Must convert!                │
│ int*         │ getValue($0,..) │ Read with getValue           │
│ struct*      │ getValue($0,..) │ Read fields individually     │
└──────────────┴─────────────────┴──────────────────────────────┘
```

## Decision Tree: Which EM_ASM to Use?

```
Do you need to pass parameters from C to JavaScript?
│
├─ NO ──→ Do you need a return value?
│         │
│         ├─ NO ──→ Use: EM_ASM(js_code);
│         │
│         └─ YES ──→ Integer? ──→ Use: EM_ASM_INT({return val;});
│                    Float?   ──→ Use: EM_ASM_DOUBLE({return val;});
│
└─ YES ──→ Do you need a return value?
          │
          ├─ NO ──→ Use: EM_ASM_({js_code}, p1, p2, ...);
          │
          └─ YES ──→ Integer? ──→ Use: EM_ASM_INT({return val;}, p1, p2);
                     Float?   ──→ Use: EM_ASM_DOUBLE({return val;}, p1, p2);
```

## Common Mistakes

### ❌ Mistake 1: Forgetting Braces
```c
// WRONG
EM_ASM(
    console.log($0);
, value);

// RIGHT
EM_ASM({
    console.log($0);
}, value);
```

### ❌ Mistake 2: Not Converting Strings
```c
// WRONG
EM_ASM({ alert($0); }, "Hello");

// RIGHT
EM_ASM({ alert(UTF8ToString($0)); }, "Hello");
```

### ❌ Mistake 3: Wrong Parameter Count
```c
// WRONG - Using $2 but only passing 2 parameters
EM_ASM({ log($0, $1, $2); }, a, b);

// RIGHT
EM_ASM({ log($0, $1, $2); }, a, b, c);
```

## Performance Hierarchy

```
Fastest  ──→  Pure C code
  ↓
  ↓          JavaScript library functions (--js-library)
  ↓
  ↓          Imported JavaScript functions
  ↓
Slowest  ──→  EM_ASM (inline JavaScript)
```

**Use EM_ASM for:**
- ✅ Prototyping
- ✅ Simple operations
- ✅ Debugging
- ✅ One-off calls

**Don't use EM_ASM for:**
- ❌ Tight loops
- ❌ Performance-critical code
- ❌ Frequently called functions

## Quick Examples

### Draw a Line
```c
EM_ASM({
    var ctx = document.getElementById('canvas').getContext('2d');
    ctx.beginPath();
    ctx.moveTo($0, $1);
    ctx.lineTo($2, $3);
    ctx.stroke();
}, x0, y0, x1, y1);
```

### Get Window Size
```c
int width = EM_ASM_INT({ return window.innerWidth; });
int height = EM_ASM_INT({ return window.innerHeight; });
```

### Show Alert
```c
EM_ASM({ alert(UTF8ToString($0)); }, message);
```

### Log to Console
```c
EM_ASM({ console.log('Value:', $0); }, value);
```

### Create DOM Element
```c
EM_ASM({
    var div = document.createElement('div');
    div.textContent = UTF8ToString($0);
    document.body.appendChild(div);
}, text);
```

## Remember

1. **Curly braces `{}`** needed when passing parameters
2. **`UTF8ToString($n)`** for string parameters
3. **`$0, $1, $2...`** to access parameters
4. **`getValue/setValue`** for memory access
5. **Choose the right variant** for your needs
