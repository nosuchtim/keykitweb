#include "key.h"
#include <emscripten.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>

// External JavaScript library functions (defined in keykit_library.js)

// Canvas drawing functions
extern void js_clear_canvas();
extern void js_draw_line(int x0, int y0, int x1, int y1);
extern void js_draw_rect(int x, int y, int w, int h);
extern void js_fill_rect(int x, int y, int w, int h);
extern void js_draw_circle(int x, int y, int radius);
extern void js_fill_circle(int x, int y, int radius);
extern void js_draw_ellipse(int x, int y, int radiusX, int radiusY);
extern void js_fill_ellipse(int x, int y, int radiusX, int radiusY);
extern void js_draw_text(int x, int y, const char *text);
extern void js_fill_polygon(int *xPoints, int *yPoints, int numPoints);
extern void js_set_color(const char *color);
extern void js_set_stroke_color(const char *color);
extern void js_set_fill_color(const char *color);
extern void js_set_line_width(int width);
extern void js_set_font(const char *font);
extern int js_get_canvas_width();
extern int js_get_canvas_height();
extern void js_set_alpha(float alpha);
extern void js_save_context();
extern void js_restore_context();
extern void js_set_composite_operation(const char *operation);

// Web MIDI API functions
extern void js_request_midi_access();
extern int js_get_midi_input_count();
extern int js_get_midi_output_count();
extern void js_get_midi_input_name(int index, char *buffer, int buffer_size);
extern void js_get_midi_output_name(int index, char *buffer, int buffer_size);
extern int js_open_midi_input(int index);
extern int js_close_midi_input(int index);
extern int js_send_midi_output(int index, unsigned char *data, int data_len);

// Mouse and keyboard functions
extern void js_setup_mouse_events();
extern int js_get_mouse_state(int *x, int *y, int *buttons);
extern void js_setup_keyboard_events();
extern int js_get_key();
extern int js_has_key();

// Global state for graphics
static int current_color_index = 0;
static char current_color_rgb[32] = "rgb(0,0,0)";
static int canvas_width = 1024;
static int canvas_height = 768;

void
mdep_hello(int argc, char **argv)
{
    // Initialize things if needed
}

void
mdep_bye(void)
{
    // Cleanup
}

int
mdep_changedir(char *d)
{
    return chdir(d);
}

char *
mdep_currentdir(char *buff, int leng)
{
    return getcwd(buff, leng);
}

int
mdep_lsdir(char *dir, char *exp, void (*callback)(char *, int))
{
    DIR *d;
    struct dirent *dirEntry;

    d = opendir(dir);
    if (d) {
        while ((dirEntry = readdir(d)) != NULL) {
            // Simple filter, maybe improve later to match 'exp' pattern
            callback(dirEntry->d_name, (dirEntry->d_type == DT_DIR));
        }
        closedir(d);
    }
    return 0;
}

long
mdep_filetime(char *fn)
{
    struct stat s;
    if (stat(fn, &s) == -1)
        return -1;
    return (long)s.st_mtime;
}

int
mdep_fisatty(FILE *f)
{
    return isatty(fileno(f));
}

long
mdep_currtime(void)
{
    time_t t;
    time(&t);
    return (long)t;
}

long
mdep_coreleft(void)
{
    return 1024 * 1024 * 1024; // Fake 1GB free
}

int
mdep_full_or_relative_path(char *path)
{
    if (*path == '/' || *path == '.')
        return 1;
    return 0;
}

int
mdep_makepath(char *dirname, char *filename, char *result, int resultsize)
{
    if (resultsize < (int)(strlen(dirname) + strlen(filename) + 2))
        return 1;
    
    if (strcmp(dirname, ".") == 0) {
        strcpy(result, filename);
        return 0;
    }

    strcpy(result, dirname);
    if (*dirname != '\0' && dirname[strlen(dirname)-1] != '/')
        strcat(result, "/");
    strcat(result, filename);
    return 0;
}

void
mdep_popup(char *s)
{
    fprintf(stderr, "POPUP: %s\n", s);
}

void
mdep_setcursor(int c)
{
    // No-op for now
}

void
mdep_prerc(void)
{
    // No-op for now
}

void
mdep_abortexit(char *msg)
{
    fprintf(stderr, "ABORT: %s\n", msg);
    exit(1);
}

void
mdep_setinterrupt(SIGFUNCTYPE func)
{
    signal(SIGINT, func);
}

void
mdep_sync(void)
{
    // No-op
}

static long start_time_ms = 0;

long
mdep_milliclock(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    if (start_time_ms == 0)
        start_time_ms = ms;
    return ms - start_time_ms;
}

void
mdep_resetclock(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    start_time_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// MIDI implementation using Web MIDI API via JavaScript

// MIDI message buffer for incoming data
#define MIDI_BUFFER_SIZE 1024
static unsigned char midi_input_buffer[MIDI_BUFFER_SIZE];
static int midi_buffer_read_pos = 0;
static int midi_buffer_write_pos = 0;
static int midi_buffer_count = 0;

// Storage for MIDI device names (allocated dynamically)
#define MAX_MIDI_DEVICE_NAME 256
static char midi_input_names[MIDI_IN_DEVICES][MAX_MIDI_DEVICE_NAME];
static char midi_output_names[MIDI_OUT_DEVICES][MAX_MIDI_DEVICE_NAME];

// MIDI initialization flag
static int midi_initialized = 0;

// Callback from JavaScript when MIDI message is received
EMSCRIPTEN_KEEPALIVE
void mdep_on_midi_message(int device_index, int status, int data1, int data2)
{
    // Add MIDI bytes to buffer
    if (midi_buffer_count + 3 <= MIDI_BUFFER_SIZE) {
        midi_input_buffer[midi_buffer_write_pos++] = (unsigned char)status;
        if (midi_buffer_write_pos >= MIDI_BUFFER_SIZE)
            midi_buffer_write_pos = 0;

        midi_input_buffer[midi_buffer_write_pos++] = (unsigned char)data1;
        if (midi_buffer_write_pos >= MIDI_BUFFER_SIZE)
            midi_buffer_write_pos = 0;

        midi_input_buffer[midi_buffer_write_pos++] = (unsigned char)data2;
        if (midi_buffer_write_pos >= MIDI_BUFFER_SIZE)
            midi_buffer_write_pos = 0;

        midi_buffer_count += 3;
    }
}

// Callback from JavaScript for mouse movement
EMSCRIPTEN_KEEPALIVE
void mdep_on_mouse_move(int x, int y)
{
    // Mouse move events are handled via polling in mdep_mouse()
    // This callback is optional and can be used for event-driven updates
}

// Callback from JavaScript for mouse button events
EMSCRIPTEN_KEEPALIVE
void mdep_on_mouse_button(int down, int x, int y, int buttons)
{
    // Mouse button events are handled via polling in mdep_mouse()
    // This callback is optional and can be used for event-driven updates
}

// Callback from JavaScript for keyboard events
EMSCRIPTEN_KEEPALIVE
void mdep_on_key_event(int down, int keycode)
{
    // Keyboard events are buffered by JavaScript and retrieved via mdep_getconsole()
    // This callback is optional and can be used for event-driven updates
}

int
mdep_getnmidi(char *buff, int buffsize, int *port)
{
    // Read MIDI data from buffer
    int bytes_read = 0;

    while (bytes_read < buffsize && midi_buffer_count > 0) {
        buff[bytes_read++] = midi_input_buffer[midi_buffer_read_pos++];
        if (midi_buffer_read_pos >= MIDI_BUFFER_SIZE)
            midi_buffer_read_pos = 0;
        midi_buffer_count--;
    }

    if (port)
        *port = 0; // Default to first port

    return bytes_read;
}

void
mdep_putnmidi(int n, char *cp, Midiport *pport)
{
    // Send MIDI data via Web MIDI API
    if (pport && pport->opened && pport->private1 >= 0) {
        int device_index = pport->private1;
        js_send_midi_output(device_index, (unsigned char*)cp, n);
    }
}

int
mdep_initmidi(Midiport *inputs, Midiport *outputs)
{
    int i;

    mdep_popup("TJT DEBUG in mdep_initmidi =================");

    // Clear MIDI buffer
    midi_buffer_read_pos = 0;
    midi_buffer_write_pos = 0;
    midi_buffer_count = 0;

    // Get MIDI device counts (devices are already enumerated in preRun)
    int num_inputs = js_get_midi_input_count();
    int num_outputs = js_get_midi_output_count();

    sprintf(Msg1,"TJT DEBUG mdep_initmidi: inputs=%d outputs=%d\n", num_inputs, num_outputs);
    mdep_popup(Msg1);

    printf("Initializing MIDI ports: %d inputs, %d outputs\n", num_inputs, num_outputs);

    // Initialize MIDI input ports directly
    for (i = 0; i < MIDI_IN_DEVICES; i++) {
        if (i < num_inputs) {
            // Get device name from Web MIDI API
            js_get_midi_input_name(i, midi_input_names[i], MAX_MIDI_DEVICE_NAME);
            inputs[i].name = midi_input_names[i];
            inputs[i].opened = 0;
            inputs[i].private1 = i; // Store device index
            printf("  MIDI Input %d: %s\n", i, inputs[i].name);
        } else {
            inputs[i].name = NULL;
            inputs[i].opened = 0;
            inputs[i].private1 = -1;
        }
    }

    // Initialize MIDI output ports directly
    for (i = 0; i < MIDI_OUT_DEVICES; i++) {
        if (i < num_outputs) {
            // Get device name from Web MIDI API
            js_get_midi_output_name(i, midi_output_names[i], MAX_MIDI_DEVICE_NAME);
            outputs[i].name = midi_output_names[i];
            outputs[i].opened = 0;
            outputs[i].private1 = i; // Store device index
            printf("  MIDI Output %d: %s\n", i, outputs[i].name);
        } else {
            outputs[i].name = NULL;
            outputs[i].opened = 0;
            outputs[i].private1 = -1;
        }
    }

    midi_initialized = 1;

    return 0; // Success
}

void
mdep_endmidi(void)
{
    // Cleanup MIDI resources
    printf("Ending MIDI...\n");
}

int
mdep_midi(int openclose, Midiport *p)
{
    if (p == NULL)
        return -1;

    int device_index = p->private1;
    if (device_index < 0)
        return -1;

    switch (openclose) {
        case MIDI_OPEN_INPUT:
            // Open MIDI input via JavaScript
            if (js_open_midi_input(device_index) == 0) {
                p->opened = 1;
                printf("Opened MIDI input: %s\n", p->name);
                return 0;
            }
            return -1;

        case MIDI_CLOSE_INPUT:
            // Close MIDI input
            if (js_close_midi_input(device_index) == 0) {
                p->opened = 0;
                printf("Closed MIDI input: %s\n", p->name);
                return 0;
            }
            return -1;

        case MIDI_OPEN_OUTPUT:
            // Open MIDI output (no special setup needed)
            p->opened = 1;
            printf("Opened MIDI output: %s\n", p->name);
            return 0;

        case MIDI_CLOSE_OUTPUT:
            // Close MIDI output
            p->opened = 0;
            printf("Closed MIDI output: %s\n", p->name);
            return 0;

        default:
            return -1;
    }
}

// Generic mdep entry point
Datum
mdep_mdep(int argc)
{
    return numdatum(0);
}

int
mdep_waitfor(int millimsecs)
{
    // Simple sleep for now, but in browser main loop this might block
    // Emscripten can handle sleep if using Asyncify, or we might need to change how the main loop works.
    // For now, return K_TIMEOUT immediately or after sleep.
    if (millimsecs > 0)
        usleep(millimsecs * 1000);
    return K_TIMEOUT;
}

int
mdep_getportdata(PORTHANDLE *port, char *buff, int max, Datum *data)
{
    return -1; // No ports
}

int
mdep_getconsole(void)
{
    // Get keyboard input from JavaScript buffer
    return js_get_key();
}

int
mdep_statconsole()
{
    // Check if keyboard input is available
    return js_has_key() ? 1 : 0;
}

// Graphics and windowing functions
int
mdep_maxx(void)
{
    canvas_width = js_get_canvas_width();
    return canvas_width;
}

int
mdep_maxy(void)
{
    canvas_height = js_get_canvas_height();
    return canvas_height;
}

int
mdep_fontwidth(void)
{
    return 8; // Default monospace width
}

int
mdep_fontheight(void)
{
    return 16; // Default monospace height
}

void
mdep_line(int x0, int y0, int x1, int y1)
{
    js_draw_line(x0, y0, x1, y1);
}

void
mdep_string(int x, int y, char *s)
{
    if (s && *s) {
        js_draw_text(x, y, s);
    }
}

// Color palette - KeyKit uses indexed colors
// Map color indices to RGB values
static void
update_color_from_index(int c)
{
    current_color_index = c;

    // Basic color palette (extend as needed)
    switch (c % 16) {
        case 0:  sprintf(current_color_rgb, "rgb(0,0,0)"); break;       // Black
        case 1:  sprintf(current_color_rgb, "rgb(0,0,255)"); break;     // Blue
        case 2:  sprintf(current_color_rgb, "rgb(0,255,0)"); break;     // Green
        case 3:  sprintf(current_color_rgb, "rgb(0,255,255)"); break;   // Cyan
        case 4:  sprintf(current_color_rgb, "rgb(255,0,0)"); break;     // Red
        case 5:  sprintf(current_color_rgb, "rgb(255,0,255)"); break;   // Magenta
        case 6:  sprintf(current_color_rgb, "rgb(255,255,0)"); break;   // Yellow
        case 7:  sprintf(current_color_rgb, "rgb(255,255,255)"); break; // White
        case 8:  sprintf(current_color_rgb, "rgb(128,128,128)"); break; // Gray
        case 9:  sprintf(current_color_rgb, "rgb(128,128,255)"); break; // Light Blue
        case 10: sprintf(current_color_rgb, "rgb(128,255,128)"); break; // Light Green
        case 11: sprintf(current_color_rgb, "rgb(128,255,255)"); break; // Light Cyan
        case 12: sprintf(current_color_rgb, "rgb(255,128,128)"); break; // Light Red
        case 13: sprintf(current_color_rgb, "rgb(255,128,255)"); break; // Light Magenta
        case 14: sprintf(current_color_rgb, "rgb(255,255,128)"); break; // Light Yellow
        case 15: sprintf(current_color_rgb, "rgb(192,192,192)"); break; // Light Gray
        default: sprintf(current_color_rgb, "rgb(0,0,0)"); break;
    }
}

void
mdep_color(int c)
{
    update_color_from_index(c);
    js_set_color(current_color_rgb);
}

void
mdep_box(int x0, int y0, int x1, int y1)
{
    // Convert from two-point format to x,y,w,h format
    int x = (x0 < x1) ? x0 : x1;
    int y = (y0 < y1) ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);
    js_draw_rect(x, y, w, h);
}

void
mdep_boxfill(int x0, int y0, int x1, int y1)
{
    // Convert from two-point format to x,y,w,h format
    int x = (x0 < x1) ? x0 : x1;
    int y = (y0 < y1) ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);
    mdep_popup("TJT DEBUG mdep_boxfill is calling js_fill_rect\n");
    printf("TJT DEBUG mdep_boxfill is calling js_fill_rect %d,%d,%d,%d\n", x, y, w, h   );
    js_fill_rect(x, y, w, h);
}

void
mdep_ellipse(int x0, int y0, int x1, int y1)
{
    // Calculate center and radii
    int cx = (x0 + x1) / 2;
    int cy = (y0 + y1) / 2;
    int rx = abs(x1 - x0) / 2;
    int ry = abs(y1 - y0) / 2;
    js_draw_ellipse(cx, cy, rx, ry);
}

void
mdep_fillellipse(int x0, int y0, int x1, int y1)
{
    // Calculate center and radii
    int cx = (x0 + x1) / 2;
    int cy = (y0 + y1) / 2;
    int rx = abs(x1 - x0) / 2;
    int ry = abs(y1 - y0) / 2;
    js_fill_ellipse(cx, cy, rx, ry);
}

void
mdep_fillpolygon(int *x, int *y, int n)
{
    if (n >= 3) {
        js_fill_polygon(x, y, n);
    }
}

void
mdep_freebitmap(Pbitmap b)
{
    // TODO: Free bitmap memory
}

int
mdep_startgraphics(int argc, char **argv)
{
    // Initialize graphics system
    printf("Initializing KeyKit graphics on Canvas...\n");

    const char *colors[] = {"red", "green", "blue", "orange", "purple", "cyan", "magenta", "yellow", "lime", "pink"};
    *Colors = sizeof(colors) / sizeof(colors[0]);

    // Request canvas dimensions
    canvas_width = js_get_canvas_width();
    canvas_height = js_get_canvas_height();

    // Setup event handlers
    js_setup_mouse_events();
    js_setup_keyboard_events();

    // Clear canvas
    mdep_popup("mdep_startgraphics is clearing canvas");    
    js_clear_canvas();

    // Set default font
    js_set_font("16px monospace");

    // Set default color
    mdep_color(7); // White

    // Use fallback dimensions if canvas returns 0
    if (canvas_width <= 0) canvas_width = 1024;
    if (canvas_height <= 0) canvas_height = 768;

    printf("Canvas initialized: %dx%d\n", canvas_width, canvas_height);

    // Draw a filled red square in the middle of the canvas as a test
    int size = 100;
    int x = (canvas_width - size) / 2;
    int y = (canvas_height - size) / 2;
    js_set_fill_color("red");
    js_fill_rect(x, y, size, size);
    printf("Drew red square at (%d, %d) size %d\n", x, y, size);

    return 0;
}

void
mdep_startrealtime(void)
{
    // Start realtime mode - request MIDI access
    mdep_popup("TJT DEBUG Starting realtime mode - requesting MIDI access...\n");
    js_request_midi_access();
}

void
mdep_startreboot(void)
{
    // Handle reboot - just clear the screen
    mdep_popup("mdep_startreboot is clearing canvas\n");    
    js_clear_canvas();
}

void
mdep_endgraphics(void)
{
    // Cleanup graphics
    printf("Ending graphics...\n");
}

void
mdep_plotmode(int mode)
{
    // Set plot mode (for XOR drawing, etc.)
    // Canvas composite operations:
    // mode 0 = normal, mode 1 = XOR
    if (mode == 1) {
        js_set_composite_operation("xor");
    } else {
        js_set_composite_operation("source-over");
    }
}

int
mdep_screensize(int *x0, int *y0, int *x1, int *y1)
{
    *x0 = 0;
    *y0 = 0;
    *x1 = mdep_maxx();
    *y1 = mdep_maxy();
    return 0;
}

int
mdep_screenresize(int x0, int y0, int x1, int y1)
{
    // TODO: Resize screen/canvas
    return 0;
}

// Font functions
char *
mdep_fontinit(char *fnt)
{
    // Return default font name
    return "monospace";
}

// Mouse functions
int
mdep_mouse(int *ax, int *ay, int *am)
{
    // Get current mouse state from JavaScript
    int x = 0, y = 0, buttons = 0;
    js_get_mouse_state(&x, &y, &buttons);

    *ax = x;
    *ay = y;
    *am = buttons;
    return 0;
}

int
mdep_mousewarp(int x, int y)
{
    // Can't warp mouse cursor in browser for security reasons
    return -1;
}

// Color functions
void
mdep_colormix(int n, int r, int g, int b)
{
    // TODO: Set color palette entry
}

void
mdep_initcolors(void)
{
    // TODO: Initialize color palette
}

// Bitmap functions (Pbitmap is defined in grid.h)
Pbitmap
mdep_allocbitmap(int xsize, int ysize)
{
    Pbitmap pb = (Pbitmap)malloc(sizeof(struct Pbitmap_struct));
    if (pb) {
        pb->xsize = xsize;
        pb->ysize = ysize;
        pb->origx = xsize;
        pb->origy = ysize;
        pb->ptr = NULL; // TODO: Allocate actual bitmap data
    }
    return pb;
}

Pbitmap
mdep_reallocbitmap(int xsize, int ysize, Pbitmap pb)
{
    if (pb) {
        pb->xsize = xsize;
        pb->ysize = ysize;
        // TODO: Reallocate bitmap data
    }
    return pb;
}

void
mdep_movebitmap(int fromx0, int fromy0, int width, int height, int tox0, int toy0)
{
    // TODO: Move bitmap region
}

void
mdep_pullbitmap(int x0, int y0, Pbitmap pb)
{
    // TODO: Copy from screen to bitmap
}

void
mdep_putbitmap(int x0, int y0, Pbitmap pb)
{
    // TODO: Copy from bitmap to screen
}

void
mdep_destroywindow(void)
{
    // TODO: Destroy window
}

// File/path functions
char *
mdep_keypath(void)
{
    return "/keykit/lib";
}

char *
mdep_musicpath(void)
{
    return "/keykit/music";
}

void
mdep_postrc(void)
{
    // Post-initialization
}

int
mdep_shellexec(char *s)
{
    // Can't execute shell commands in browser
    return -1;
}

void
mdep_ignoreinterrupt(void)
{
    signal(SIGINT, SIG_IGN);
}

char *
mdep_browse(char *desc, char *types, int mustexist)
{
    // TODO: Implement file browser dialog
    return NULL;
}

// Port functions
PORTHANDLE *
mdep_openport(char *name, char *mode, char *type)
{
    return NULL;
}

int
mdep_putportdata(PORTHANDLE m, char *buff, int size)
{
    return -1;
}

int
mdep_closeport(PORTHANDLE m)
{
    return -1;
}

Datum
mdep_ctlport(PORTHANDLE m, char *cmd, char *arg)
{
    Datum d;
    d.type = D_NUM;
    d.u.val = -1;
    return d;
}

int
mdep_help(char *fname, char *keyword)
{
    return -1;
}

char *
mdep_localaddresses(Datum d)
{
    return "127.0.0.1";
}
