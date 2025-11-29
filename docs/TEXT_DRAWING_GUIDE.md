# Text Drawing on Canvas - Complete Guide

## Overview

This guide shows all the text drawing capabilities added to the canvas examples, demonstrating how to draw text with different fonts, sizes, colors, alignments, and effects.

## Text Drawing Functions

### 1. Basic Text Drawing

```c
void draw_text(int x, int y, const char *text);
```

**Example:**
```c
draw_text(100, 100, "Hello World!");
```

### 2. Set Font

```c
void set_font(const char *font);
```

**Font Format:** `"[style] [weight] [size] [family]"`

**Examples:**
```c
set_font("16px Arial");                    // Simple
set_font("bold 24px Arial");               // Bold
set_font("italic 18px Georgia");           // Italic
set_font("bold italic 20px Times");        // Bold + Italic
set_font("16px 'Courier New', monospace"); // Monospace
```

**Common Font Families:**
- `Arial` - Sans-serif, clean
- `Georgia` - Serif, elegant
- `'Courier New'` - Monospace, code-like
- `Verdana` - Sans-serif, readable
- `'Times New Roman'` - Serif, traditional
- `Impact` - Bold, impactful

### 3. Set Text Color

```c
void set_color(const char *color);
```

**Examples:**
```c
set_color("black");
set_color("red");
set_color("#FF5733");
set_color("rgb(255, 100, 50)");
set_color("rgba(255, 100, 50, 0.5)"); // With transparency
```

### 4. Set Text Alignment

```c
void set_text_align(const char *align);
```

**Options:** `"left"`, `"center"`, `"right"`, `"start"`, `"end"`

**Example:**
```c
set_text_align("center");
draw_text(400, 100, "Centered text");
```

### 5. Set Text Baseline

```c
void set_text_baseline(const char *baseline);
```

**Options:** `"top"`, `"middle"`, `"bottom"`, `"alphabetic"`, `"hanging"`

**Example:**
```c
set_text_baseline("middle");
draw_text(100, 100, "Vertically centered");
```

### 6. Draw Outlined Text

```c
void draw_text_outlined(int x, int y, const char *text, 
                        const char *fill_color,
                        const char *stroke_color, 
                        int stroke_width);
```

**Example:**
```c
draw_text_outlined(100, 100, "Outlined!", "yellow", "black", 3);
```

### 7. Measure Text Width

```c
int measure_text_width(const char *text);
```

**Example:**
```c
int width = measure_text_width("Hello");
printf("Text width: %d pixels\n", width);
```

### 8. Draw Multi-line Text

```c
void draw_multiline_text(int x, int y, const char *text, int line_height);
```

**Example:**
```c
draw_multiline_text(100, 100, "Line 1\nLine 2\nLine 3", 20);
```

### 9. Draw Text with Background

```c
void draw_text_with_background(int x, int y, const char *text,
                                const char *text_color,
                                const char *bg_color, 
                                int padding);
```

**Example:**
```c
draw_text_with_background(100, 100, "Important!", "white", "red", 5);
```

## Complete Examples

### Example 1: Different Font Styles

```c
// Regular
set_font("16px Arial");
set_color("black");
draw_text(50, 50, "Regular text");

// Bold
set_font("bold 16px Arial");
draw_text(50, 80, "Bold text");

// Italic
set_font("italic 16px Arial");
draw_text(50, 110, "Italic text");

// Bold + Italic
set_font("bold italic 16px Arial");
draw_text(50, 140, "Bold italic text");
```

### Example 2: Different Font Sizes

```c
set_color("darkblue");

set_font("12px Arial");
draw_text(50, 50, "12px text");

set_font("18px Arial");
draw_text(50, 80, "18px text");

set_font("24px Arial");
draw_text(50, 120, "24px text");

set_font("36px Arial");
draw_text(50, 170, "36px text");
```

### Example 3: Text Alignment

```c
int center_x = 400;

set_font("16px Arial");
set_color("black");

// Draw a vertical line to show alignment
set_line_width(1);
set_color("lightgray");
draw_line_inline(center_x, 0, center_x, 600);

// Left aligned
set_color("black");
set_text_align("left");
draw_text(center_x, 100, "Left aligned");

// Center aligned
set_text_align("center");
draw_text(center_x, 130, "Center aligned");

// Right aligned
set_text_align("right");
draw_text(center_x, 160, "Right aligned");
```

### Example 4: Colorful Text

```c
set_font("bold 20px Arial");

set_color("red");
draw_text(50, 50, "Red text");

set_color("green");
draw_text(50, 80, "Green text");

set_color("blue");
draw_text(50, 110, "Blue text");

set_color("#FF5733");
draw_text(50, 140, "Custom hex color");

set_color("rgba(255, 0, 255, 0.5)");
draw_text(50, 170, "Semi-transparent");
```

### Example 5: Outlined Text

```c
set_font("bold 32px Arial");

// Yellow text with black outline
draw_text_outlined(100, 100, "OUTLINED!", "yellow", "black", 3);

// White text with blue outline
draw_text_outlined(100, 150, "GAME OVER", "white", "blue", 4);

// Rainbow effect
draw_text_outlined(100, 200, "RAINBOW", "red", "purple", 2);
```

### Example 6: Text with Background

```c
set_font("18px Arial");

// White text on dark background
draw_text_with_background(50, 50, "Important Message", "white", "darkred", 8);

// Black text on yellow background (warning style)
draw_text_with_background(50, 90, "Warning!", "black", "yellow", 5);

// Label style
draw_text_with_background(50, 130, "Label", "white", "navy", 4);
```

### Example 7: Multi-line Text

```c
set_font("14px Arial");
set_color("black");

const char* multiline = 
    "This is line 1\n"
    "This is line 2\n"
    "This is line 3\n"
    "This is line 4";

draw_multiline_text(50, 50, multiline, 20);
```

### Example 8: Measured Text with Box

```c
const char* text = "Measured Text";
set_font("20px Arial");

// Measure the text
int width = measure_text_width(text);

// Draw the text
set_color("blue");
draw_text(100, 100, text);

// Draw a box around it
set_color("red");
set_line_width(2);
draw_line_inline(100, 85, 100 + width, 85);      // Top
draw_line_inline(100, 105, 100 + width, 105);    // Bottom
draw_line_inline(100, 85, 100, 105);             // Left
draw_line_inline(100 + width, 85, 100 + width, 105); // Right
```

### Example 9: Text Labels for Shapes

```c
// Draw a circle
draw_circle(200, 200, 50, "blue");

// Label it
set_font("14px Arial");
set_color("black");
set_text_align("center");
draw_text(200, 260, "Circle");

// Draw a rectangle
draw_filled_rect(300, 150, 100, 100, "green");

// Label it
draw_text(350, 260, "Rectangle");
```

### Example 10: Title and Subtitle

```c
// Title
set_font("bold 36px Arial");
set_color("darkblue");
set_text_align("center");
draw_text(400, 50, "My Canvas App");

// Subtitle
set_font("italic 18px Arial");
set_color("gray");
draw_text(400, 90, "Powered by WebAssembly");

// Reset alignment
set_text_align("left");
```

## Font Size Guidelines

| Size | Use Case |
|------|----------|
| 10-12px | Small labels, captions |
| 14-16px | Body text, normal content |
| 18-20px | Subheadings, emphasis |
| 24-28px | Headings |
| 32-48px | Titles, large headings |
| 60px+ | Display text, banners |

## Color Formats

### Named Colors
```c
set_color("red");
set_color("blue");
set_color("darkgreen");
```

### Hex Colors
```c
set_color("#FF0000");  // Red
set_color("#00FF00");  // Green
set_color("#0000FF");  // Blue
```

### RGB Colors
```c
set_color("rgb(255, 0, 0)");  // Red
```

### RGBA Colors (with transparency)
```c
set_color("rgba(255, 0, 0, 0.5)");  // 50% transparent red
```

## Text Alignment Visual Guide

```
Left:    |Text starts here
Center:       Text centered here
Right:                Text ends here|
```

## Best Practices

### 1. **Set Font Before Drawing**
```c
set_font("16px Arial");  // Always set font first
draw_text(100, 100, "Text");
```

### 2. **Reset Alignment**
```c
set_text_align("center");
draw_text(400, 100, "Centered");
set_text_align("left");  // Reset to default
```

### 3. **Use Appropriate Font Sizes**
```c
// Too small - hard to read
set_font("8px Arial");  // ❌

// Good for body text
set_font("14px Arial");  // ✅

// Good for headings
set_font("24px Arial");  // ✅
```

### 4. **Ensure Good Contrast**
```c
// Bad - low contrast
set_color("lightgray");
draw_text_with_background(100, 100, "Text", "lightgray", "white", 5);  // ❌

// Good - high contrast
draw_text_with_background(100, 100, "Text", "white", "black", 5);  // ✅
```

### 5. **Measure Before Centering**
```c
const char* text = "Center me!";
int width = measure_text_width(text);
int center_x = (canvas_width - width) / 2;
draw_text(center_x, 100, text);
```

## Common Use Cases

### Labels
```c
set_font("12px Arial");
set_color("black");
draw_text(x, y, "Label:");
```

### Headings
```c
set_font("bold 24px Arial");
set_color("darkblue");
draw_text(x, y, "Heading");
```

### Buttons (with background)
```c
draw_text_with_background(x, y, "Click Me", "white", "blue", 10);
```

### Warnings
```c
draw_text_with_background(x, y, "Warning!", "black", "yellow", 5);
```

### Code/Monospace
```c
set_font("14px 'Courier New', monospace");
set_color("green");
draw_text(x, y, "console.log('Hello');");
```

## Summary

The text drawing system provides:

✅ **7 new text functions**
- `set_font()` - Change font style/size
- `set_text_align()` - Align text
- `set_text_baseline()` - Vertical alignment
- `draw_text_outlined()` - Text with outline
- `measure_text_width()` - Get text dimensions
- `draw_multiline_text()` - Multiple lines
- `draw_text_with_background()` - Text with background box

✅ **Full control over:**
- Font family, size, weight, style
- Text color (named, hex, RGB, RGBA)
- Alignment (left, center, right)
- Outlines and backgrounds
- Multi-line text
- Text measurement

All examples are in `canvas_drawing_example.c` - build and run to see them in action!
