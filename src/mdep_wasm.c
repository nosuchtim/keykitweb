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
extern int js_get_font_height();
extern int js_get_font_width();
extern int js_get_canvas_width();
extern int js_get_canvas_height();
extern void js_set_alpha(float alpha);
extern void js_save_context();
extern void js_restore_context();
extern void js_set_composite_operation(const char *operation);

// Web MIDI API functions
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

// Bitmap functions
extern int js_get_image_data(int x, int y, int width, int height, unsigned char *buffer);
extern void js_put_image_data(unsigned char *buffer, int bufLen, int x, int y, int width, int height);
extern void js_copy_bitmap_region(int fromx, int fromy, int width, int height, int tox, int toy);

// WebSocket functions (for network ports)
extern int js_websocket_connect(const char *url, int portId);
extern int js_websocket_send(int portId, const char *data, int dataLen);
extern int js_websocket_receive(int portId, char *buffer, int bufferSize);
extern int js_websocket_state(int portId);
extern int js_websocket_close(int portId);

// File browser functions
extern void js_browse_file(const char *desc, const char *types, int mustexist);
extern int js_browse_is_done(void);
extern char *js_browse_get_result(void);

// Global state for graphics
static int current_color_index = 0;
static int canvas_width = 1024;
static int canvas_height = 768;

// Fixed-size color buffers
#define RGB_BUFFER_SIZE 32 
static char color_list[KEYNCOLORS][RGB_BUFFER_SIZE];

// ========== Port Definitions (needed early for mdep_getportdata) ==========

// Port types
#define MYPORT_TCPIP_READ 1
#define MYPORT_TCPIP_WRITE 2
#define MYPORT_TCPIP_LISTEN 3
#define MYPORT_UDP_WRITE 6
#define MYPORT_UDP_LISTEN 7
#define MYPORT_OSC_WRITE 8
#define MYPORT_OSC_LISTEN 9
#define MYPORT_NATS_WRITE 10
#define MYPORT_NATS_LISTEN 11

// Port states
#define SOCK_UNCONNECTED 0
#define SOCK_CONNECTED 1
#define SOCK_CLOSED 2
#define SOCK_LISTENING 3
#define SOCK_REFUSED 4

#define PORT_NORMAL 0
#define PORT_CANREAD 1

#define TYPE_NONE 0
#define TYPE_READ 1
#define TYPE_WRITE 2
#define TYPE_LISTEN 3

// Port info structure
struct myportinfo {
    char *name;
    int myport_type;
    int rw;  // TYPE_READ, TYPE_WRITE, TYPE_LISTEN
    int portId;  // WebSocket ID
    int sockstate;
    int portstate;
    int isopen;
    int closeme;
    int hasreturnedfinaldata;
    char *buff;  // buffered data
    int buffsize;
    char *nats_subject;  // NATS subject for NATS ports
    struct myportinfo *next;
};

typedef struct myportinfo Myport;

static Myport *Topport = NULL;
static int next_port_id = 1;

// NATS message buffer structure (forward declarations)
#define NATS_MESSAGE_BUFFER_SIZE 20
struct nats_msg_buffer {
    char *subject;
    char *data;
};
static struct nats_msg_buffer nats_message_buffer[NATS_MESSAGE_BUFFER_SIZE];
static int nats_message_read_pos = 0;
static int nats_message_write_pos = 0;
static int nats_message_count = 0;

// Forward declarations of helper functions
static int nats_has_message_for_subject(const char *subject);
static int nats_get_message_for_subject(const char *subject, char *buffer, int buffer_size);
static Myport *newmyport(char *name);
static void sockaway(Myport *m, char *buff, int size);
static void sendsockedaway(Myport *mp);

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
    fprintf(stderr, "POPUP: %s", s);
    char *eol = s + strlen(s) - 1;
    if ( *eol != '\n' ) {
        fprintf(stderr, "\n");
    }
}

extern void js_set_cursor(int cursorType);

void
mdep_setcursor(int c)
{
    js_set_cursor(c);
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
static int midi_messages_received = 0; // Debug counter

// Keyboard input buffer for incoming keypresses
typedef struct {
    int keycode;
    int ctrl;
    int shift;
    int alt;
} KeyEvent;

#define KEYBOARD_BUFFER_SIZE 256
static KeyEvent keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int keyboard_buffer_read_pos = 0;
static int keyboard_buffer_write_pos = 0;
static int keyboard_buffer_count = 0;

// Current modifier key state (maintained for backwards compatibility)
static int current_ctrl_down = 0;
static int current_shift_down = 0;
static int current_alt_down = 0;

// Mouse event buffer structure
typedef struct {
    int x;
    int y;
    int buttons;
    int modifiers;   // 1=Ctrl, 2=Shift
    int event_type;  // 0 = move, 1 = button down, 2 = button up
} MouseEvent;

#define MOUSE_BUFFER_SIZE 256
static MouseEvent mouse_buffer[MOUSE_BUFFER_SIZE];
static int mouse_buffer_read_pos = 0;
static int mouse_buffer_write_pos = 0;
static int mouse_buffer_count = 0;

// Current mouse state (for polling)
static int current_mouse_x = 0;
static int current_mouse_y = 0;
static int current_mouse_buttons = 0;

// Storage for MIDI device names (allocated dynamically)
#define MAX_MIDI_DEVICE_NAME 256
static char midi_input_names[MIDI_IN_DEVICES][MAX_MIDI_DEVICE_NAME];
static char midi_output_names[MIDI_OUT_DEVICES][MAX_MIDI_DEVICE_NAME];

// MIDI initialization flag
static int midi_initialized = 0;

// Font metrics cache
static int cached_font_width = -1;
static int cached_font_height = -1;

// Window resize tracking
static int window_resize_pending = 0;
static int last_canvas_width = 0;
static int last_canvas_height = 0;

// Callback from JavaScript when MIDI message is received
// IMPORTANT: Keep this function minimal! Do NOT call mdep_popup() or other
// complex KeyKit functions from here, as this is called asynchronously from
// JavaScript and can cause stack corruption with ASYNCIFY.
EMSCRIPTEN_KEEPALIVE
void mdep_on_midi_message(int device_index, int status, int data1, int data2)
{
    // Simply add MIDI bytes to buffer - no complex operations
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
        midi_messages_received++; // Count messages for debugging
    }
}

// The value of mouse_buttons from Javascript
// is a bitmask of buttons with 3 bits:
// 1=left, 2=middle, 4=right.
// But the keykit code expects values of
// just 0 (no button), 1 (left button) and 2 (right button).
// This function converts the bitmask to that format.
int mdep_mouse_convert(int buttons) {
    if ( buttons > 0 ) {
        if (buttons & 1) {
            buttons = 1; // Left button
        } else if (buttons > 0 ) {
            buttons = 2; // Right button
        }
    }
    return buttons;
}

// Callback from JavaScript for mouse movement
EMSCRIPTEN_KEEPALIVE
void mdep_on_mouse_move(int x, int y, int modifiers)
{
    // Update current mouse position
    current_mouse_x = x;
    current_mouse_y = y;

    // Buffer the mouse move event
    if (mouse_buffer_count < MOUSE_BUFFER_SIZE) {
        mouse_buffer[mouse_buffer_write_pos].x = x;
        mouse_buffer[mouse_buffer_write_pos].y = y;
        mouse_buffer[mouse_buffer_write_pos].buttons = current_mouse_buttons;
        mouse_buffer[mouse_buffer_write_pos].modifiers = modifiers;
        mouse_buffer[mouse_buffer_write_pos].event_type = 0; // move
        mouse_buffer_write_pos++;
        if (mouse_buffer_write_pos >= MOUSE_BUFFER_SIZE)
            mouse_buffer_write_pos = 0;
        mouse_buffer_count++;
    }
}

// Callback from JavaScript for mouse button events
EMSCRIPTEN_KEEPALIVE
void mdep_on_mouse_button(int down, int x, int y, int buttons, int modifiers)
{
    // Update current mouse state
    current_mouse_x = x;
    current_mouse_y = y;
    current_mouse_buttons = mdep_mouse_convert(buttons);

    // Buffer the mouse button event
    if (mouse_buffer_count < MOUSE_BUFFER_SIZE) {
        mouse_buffer[mouse_buffer_write_pos].x = x;
        mouse_buffer[mouse_buffer_write_pos].y = y;
        mouse_buffer[mouse_buffer_write_pos].buttons = current_mouse_buttons;
        mouse_buffer[mouse_buffer_write_pos].modifiers = modifiers;
        mouse_buffer[mouse_buffer_write_pos].event_type = down ? 1 : 2; // 1 = button down, 2 = button up
        mouse_buffer_write_pos++;
        if (mouse_buffer_write_pos >= MOUSE_BUFFER_SIZE)
            mouse_buffer_write_pos = 0;
        mouse_buffer_count++;
    } else {
        printf("Warning: mouse buffer full, dropping event\n");
    }
}

// Callback from JavaScript for keyboard events
EMSCRIPTEN_KEEPALIVE
void mdep_on_key_event(int down, int keycode, int ctrl, int shift, int alt)
{
    // Update modifier key state
    current_ctrl_down = ctrl;
    current_shift_down = shift;
    current_alt_down = alt;

    // Only buffer key down events
    if (down == 1) {
        if (keyboard_buffer_count < KEYBOARD_BUFFER_SIZE) {
            keyboard_buffer[keyboard_buffer_write_pos].keycode = keycode;
            keyboard_buffer[keyboard_buffer_write_pos].ctrl = ctrl;
            keyboard_buffer[keyboard_buffer_write_pos].shift = shift;
            keyboard_buffer[keyboard_buffer_write_pos].alt = alt;
            keyboard_buffer_write_pos++;
            if (keyboard_buffer_write_pos >= KEYBOARD_BUFFER_SIZE)
                keyboard_buffer_write_pos = 0;
            keyboard_buffer_count++;
        } else {
            printf("Warning: keyboard buffer full, dropping keycode %d\n", keycode);
        }
    }
}

// Callback from JavaScript when window is resized
EMSCRIPTEN_KEEPALIVE
void mdep_on_window_resize(int width, int height)
{
    // Mark that a resize event occurred
    window_resize_pending = 1;
    last_canvas_width = width;
    last_canvas_height = height;
}

// Helper functions to check modifier key state
int mdep_ctrl_down(void)
{
    return current_ctrl_down;
}

int mdep_shift_down(void)
{
    return current_shift_down;
}

int mdep_alt_down(void)
{
    return current_alt_down;
}

// Helper functions for mouse event buffer access
int mdep_get_mouse_event(int *x, int *y, int *buttons, int *event_type, int *modifiers)
{
    if (mouse_buffer_count > 0) {
        MouseEvent *event = &mouse_buffer[mouse_buffer_read_pos];
        *x = event->x + 6;  // HACK?? Adjust for browser offset?
        *y = event->y + 6;  // HACK?? Adjust for browser offset?
        *buttons = event->buttons;
        *event_type = event->event_type;
        *modifiers = event->modifiers;

        mouse_buffer_read_pos++;
        if (mouse_buffer_read_pos >= MOUSE_BUFFER_SIZE)
            mouse_buffer_read_pos = 0;
        mouse_buffer_count--;
        return 1; // Event retrieved
    }
    return 0; // No events available
}

void mdep_clear_mouse_events(void)
{
    mouse_buffer_read_pos = 0;
    mouse_buffer_write_pos = 0;
    mouse_buffer_count = 0;
}

int
mdep_getnmidi(char *buff, int buffsize, int *port)
{
    // Read MIDI data from buffer
    int bytes_read = 0;
    int initial_count = midi_buffer_count;

    while (bytes_read < buffsize && midi_buffer_count > 0) {
        buff[bytes_read++] = midi_input_buffer[midi_buffer_read_pos++];
        if (midi_buffer_read_pos >= MIDI_BUFFER_SIZE)
            midi_buffer_read_pos = 0;
        midi_buffer_count--;
    }

    if (port)
        *port = 0; // Default to first port

    // Debug logging for MIDI input reads
    static int last_log_count = 0;
    if (bytes_read > 0) {
        printf("[MIDI READ] Read %d bytes from buffer (buffer had %d, now has %d, total received: %d)\n",
               bytes_read, initial_count, midi_buffer_count, midi_messages_received);
        printf("[MIDI READ] Data: ");
        for (int i = 0; i < bytes_read && i < 16; i++) {
            printf("0x%02x ", (unsigned char)buff[i]);
        }
        printf("\n");
        last_log_count = midi_messages_received;
    } else if (midi_messages_received > last_log_count && (midi_messages_received % 10) == 0) {
        // Log every 10 messages when we're receiving but not reading
        printf("[MIDI DEBUG] Received %d messages, buffer count: %d (nothing being read!)\n",
               midi_messages_received, midi_buffer_count);
        last_log_count = midi_messages_received;
    }

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

    // Clear MIDI buffer
    midi_buffer_read_pos = 0;
    midi_buffer_write_pos = 0;
    midi_buffer_count = 0;
    midi_messages_received = 0;

    // Get MIDI device counts (devices are already enumerated in preRun)
    int num_inputs = js_get_midi_input_count();
    int num_outputs = js_get_midi_output_count();

    printf("Initializing MIDI ports: %d inputs, %d outputs\n", num_inputs, num_outputs);

    int verbose = 0;
    // Initialize MIDI input ports directly
    for (i = 0; i < MIDI_IN_DEVICES; i++) {
        if (i < num_inputs) {
            // Get device name from Web MIDI API
            js_get_midi_input_name(i, midi_input_names[i], MAX_MIDI_DEVICE_NAME);
            inputs[i].name = midi_input_names[i];
            inputs[i].opened = 0;
            inputs[i].private1 = i; // Store device index
            if ( verbose ) {
                printf("  MIDI Input %d: %s\n", i, inputs[i].name);
            }
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
            if ( verbose ) {
                printf("  MIDI Output %d: %s\n", i, outputs[i].name);
            }
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
	char *args[3];
	int n;
	Datum d;

	d = Nullval;
	/*
	 * Things past the first 3 args might be integers
	 */
	for ( n=0; n<3 && n<argc; n++ ) {
		Datum dd = ARG(n);
		if ( dd.type == D_STR ) {
			args[n] = needstr("mdep",dd);
		} else {
			args[n] = "";
		}
	}
	for ( ; n<3; n++ )
		args[n] = "";

	/*
	 * recognized commands are:
	 *     tcpip localaddresses
	 *     priority low/normal/high/realtime
	 *     popen {cmd} "rt"
	 *     popen {cmd} "wt" {string-to-write}
	 */

	if ( strcmp(args[0],"midi")==0 ) {
		execerror("mdep(\"midi\",...) is no longer used.  Use midi(...).\n");
	}
	else if ( strcmp(args[0],"env") == 0 ) {
	    if ( strcmp(args[1],"get")==0 ) {
			char *s = getenv(args[2]);
			if ( s != NULL ) {
				d = strdatum(uniqstr(s));
			} else {
				d = strdatum(Nullstr);
			}
	    } else {
		execerror("mdep(\"env\",... ) doesn't recognize %s\n",args[1]);
	    }
	}
	else if ( strcmp(args[0],"video") == 0 ) {
	    execerror("mdep(\"video\",...): keykit not compiled with video support\n");
	}
	else if ( strcmp(args[0],"gesture") == 0 ) {
	    execerror("mdep(\"gesture\",...): keykit not compiled with igesture support\n");
	}
	else if ( strcmp(args[0],"lcd") == 0 ) {
	    execerror("mdep(\"lcd\",...): keykit not compiled with lcd support\n");
	}
#if KEYDISPLAY
	else if ( strcmp(args[0],"display") == 0 ) {

	    if ( strcmp(args[1],"start")==0 ) {
		int gx = 32;
		int gy = 32;
		int width = neednum("Expecting integer width",ARG(2));
		int height = neednum("Expecting integer height",ARG(3));
		int noborder = 0;
		char *nbp = "xxx";

		if ( argc < 4 )
			execerror("mdep(\"display\",\"start\",...) needs at least 4 args\n");
		if ( argc > 4 ) {
			nbp = needstr("mdep display",ARG(4));
			noborder = (strcmp(nbp,"noborder")==0);
		}
		if (!startdisplay(noborder,width,height))
			d = numdatum(0);
		else
			d = numdatum(1);

	    }
	}
#endif
	else if ( strcmp(args[0],"osc") == 0 ) {
        eprint("mdep(osc,...) not implemented in WebAssembly build.\n");
        /*
	    if ( strcmp(args[1],"send")==0 ) {
		char buff[512];
		int buffsize = sizeof(buff);
		int sofar = 0;
		char *tp;
		int fnum = neednum("Expecting fifo number ",ARG(2));
		Fifo *fptr;
		Datum d3;

		fptr = fifoptr(fnum);
		if ( fptr == NULL )
			execerror("No fifo numbered %d!?",fnum);

		d3 = ARG(3);
		if ( d3.type == D_ARR ) {
			Htablep arr = d3.u.arr;
			int cnt = 0;
			Symbolp s;
			int asize;
			Datum dd;
			char types[64];

			// First one is the message (e.g. "/foo")
			s = arraysym(arr,numdatum(0),H_LOOK);
			if ( s == NULL )
				execerror("First element of array not found?");
			dd = *symdataptr(s);
			if ( dd.type != D_STR )
				execerror("First element of array must be string");
			osc_pack_str(buff,buffsize,&sofar,dd.u.str);
			tp = types;
			*tp++ = ',';
			cnt = 1;
			asize = arrsize(arr);
			for ( n=1; n<asize; n++ ) {
				s = arraysym(arr,numdatum(n),H_LOOK);
				dd = *symdataptr(s);
				switch(dd.type){
				case D_NUM:
					*tp++ = 'i';
					break;
				case D_STR:
					*tp++ = 's';
					break;
				case D_DBL:
					*tp++ = 'f';
					break;
				default:
					execerror("Can't handle type %s!",atypestr(d.type));
				}
				cnt++;
			}
			*tp = '\0';
			osc_pack_str(buff,buffsize,&sofar,types);
			for ( n=1; n<asize; n++ ) {
				s = arraysym(arr,numdatum(n),H_LOOK);
				dd = *symdataptr(s);
				switch(dd.type){
				case D_NUM:
					osc_pack_int(buff,buffsize,&sofar,dd.u.val);
					break;
				case D_STR:
					osc_pack_str(buff,buffsize,&sofar,dd.u.str);
					break;
				case D_DBL:
					osc_pack_dbl(buff,buffsize,&sofar,dd.u.dbl);
					break;
				default:
					execerror("Can't handle type %s!",atypestr(d.type));
				}
			}
		} else {
			for ( n=3; n<argc; n++ ) {
				d = ARG(n);
				if ( d.type == D_STR ) {
					char *s = d.u.str;
					int c;
					while ( (c=*s++) != '\0' ) {
						buff[sofar++] = c;
					}
				} else if ( d.type == D_NUM ) {
					buff[sofar++] = (char)(numval(d));
				} else {
					execerror("Bad type of data given to osc send.");
					
				}
			}
		}
		
		udp_send(fptr->port,buff,sofar);
		d = numdatum(0);
	    }
        */
	}
	else if ( strcmp(args[0],"tcpip")==0 ) {
        eprint("mdep(tcpip,...) not implemented in WebAssembly build.\n");
        /*
	    if ( strcmp(args[1],"localaddresses")==0 ) {
			char *p;
			d = newarrdatum(0,3);
			p = mdep_localaddresses(d);
			if ( p )
				eprint("Error: %s\n",p);
		}
        */
	}
	else if ( strcmp(args[0],"clipboard")==0 ) {
        eprint("mdep(clipboard,...) not implemented in WebAssembly build.\n");
        /*
		if ( strcmp(args[1],"get")==0 ) {
			char *s = mdep_getclipboard();
			if ( s != NULL ) {
				d = strdatum(uniqstr(s));
			}
		} else if ( strcmp(args[1],"set")==0 ) {
			if ( mdep_setclipboard(args[2]) != 0 ) {
				d = numdatum(0);
			}
		} else {
			eprint("Error: unrecognized clipboard argument - %s\n",args[1]);
		}
        */
	}
	else if ( strcmp(args[0],"sendinput")==0 ) {
        eprint("mdep(sendinput,...) not implemented in WebAssembly build.\n");
        /*
		struct tagINPUT in;
		if ( strcmp(args[1],"keyboard")==0 ) {
			int r;
			int vk = neednum("Expecting integer keycode",ARG(2));
			int up = neednum("Expecting up/down value",ARG(3));
			in.type = INPUT_KEYBOARD;
			in.ki.wVk = vk;
			in.ki.wScan = 0;
			if ( up )
				in.ki.dwFlags = KEYEVENTF_KEYUP;
			else
				in.ki.dwFlags = 0;
			in.ki.time = 0;
			in.ki.dwExtraInfo = 0;
			r = SendInput(1,&in,sizeof(INPUT));
			d = numdatum(r);
		} else if ( strcmp(args[1],"mouse")==0 ) {
			eprint("Error: sendinput of mouse not implemented yet\n");
		} else {
			eprint("Error: unrecognized sendinput argument - %s\n",args[1]);
		}
        */
	}
	else if ( strcmp(args[0],"joystick")==0 ) {
        eprint("mdep(joystick,...) not implemented in WebAssembly build.\n");
        /*
		if ( strcmp(args[1],"init")==0 ) {
			Datum joyinit(int);
			int millipoll = 10;	// default is 10 milliseconds

			if ( *args[2] != 0 )
				millipoll = atoi(args[2]);
			if ( millipoll < 1 )
				millipoll = 1;
			d = joyinit(millipoll);

		} else if ( strcmp(args[1],"release")==0 ) {
			void joyrelease();
			joyrelease();
		} else {
			eprint("Error: unrecognized joystick argument - %s\n",args[1]);
		}
        */
	} else if ( strcmp(args[0],"priority")==0 ) {
        eprint("mdep(priority,...) not implemented in WebAssembly build.\n");
        /*
		BOOL n = FALSE;
		long r = 0; 
		HANDLE p = GetCurrentProcess();
	    if ( strcmp(args[1],"realtime")==0 ) {
			n = SetPriorityClass(p,REALTIME_PRIORITY_CLASS);
		}
	    else if ( strcmp(args[1],"normal")==0 ) {
			n = SetPriorityClass(p,NORMAL_PRIORITY_CLASS);
		}
	    else if ( strcmp(args[1],"low")==0 ) {
			n = SetPriorityClass(p,IDLE_PRIORITY_CLASS);
		}
	    else if ( strcmp(args[1],"high")==0 ) {
			n = SetPriorityClass(p,HIGH_PRIORITY_CLASS);
		}
		else {
			n = FALSE;
			eprint("Error: unrecognized priority - %s\n",args[1]);
		}
		if ( n == FALSE ) {
			r = GetLastError();
			eprint("Error: getlasterror=%ld\n",r);
		}
		else
			r = 0;
		d = numdatum(r);
        */
	}
	else if ( strcmp(args[0],"popen")==0 ) {
        eprint("mdep(popen,...) not implemented in WebAssembly build.\n");
        /*
		FILE *f;
		int buffsize;
		char *p;
		int c;

		// * FOR SOME REASON, THIS CODE DOESN'T WORK.
		// * I HAVE NO IDEA WHAT'S WRONG.  The _popen
		// * always seems to return NULL.

		f = _popen(args[1],args[2]);
		if ( f == NULL ) {
			tprint("popen returned NULL\n");
			d = Nullval;
		}
		else {
			if ( args[2][0] == 'w' ) {
				char *ws = needstr("mdep",ARG(3));
				fprintf(f,"%s",ws);
				d = numdatum(0);
			}
			else {
				// it's an 'r'
				p = Msg2;
				buffsize = 0;
				while ( (c=getc(f)) >= 0 ) {
					makeroom(++buffsize,&Msg2,&Msg2size);
					*p++ = c;
				}
				*p = '\0';
				d = strdatum(uniqstr(Msg2));
			}
			_pclose(f);
		}
        */
	}
	else {
		/* unrecognized command */
		eprint("Error: unrecognized mdep argument - %s\n",args[0]);
	}
	return d;
}

int
mdep_waitfor(int millimsecs)
{
    // Use emscripten_sleep() to properly yield to browser event loop
    // This allows mouse/keyboard callbacks to be processed during the sleep
    if (millimsecs > 0) {
        emscripten_sleep(millimsecs);
	}

    // Check for window resize event
    if (window_resize_pending) {
        window_resize_pending = 0;
        // Update cached canvas dimensions
        canvas_width = last_canvas_width;
        canvas_height = last_canvas_height;
        return K_WINDRESIZE;
    }

	if ( mdep_statconsole() ) {
		return K_CONSOLE;
	}
    return K_TIMEOUT;
}

int
mdep_getportdata(PORTHANDLE *port, char *buff, int max, Datum *data)
{
    Myport *m;
    int r;

    // Check all open ports for data
    for (m = Topport; m != NULL; m = m->next) {
        if (!m->isopen)
            continue;

        // Check for closed/refused connections that need to return final status
        if (m->rw == TYPE_READ && m->sockstate == SOCK_CLOSED && m->hasreturnedfinaldata == 0) {
            m->hasreturnedfinaldata = 1;
            *port = m;
            return 0;  // EOF
        }

        if (m->rw == TYPE_READ && m->sockstate == SOCK_REFUSED && m->hasreturnedfinaldata == 0) {
            m->hasreturnedfinaldata = 1;
            *port = m;
            return -2;  // Connection refused
        }

        // Check for readable ports
        if (m->portstate == PORT_CANREAD || m->rw == TYPE_LISTEN) {
            // NATS listen ports
            if (m->myport_type == MYPORT_NATS_LISTEN) {
                // Check for messages on this subject
                if (nats_has_message_for_subject(m->nats_subject)) {
                    r = nats_get_message_for_subject(m->nats_subject, buff, max);
                    if (r > 0) {
                        *port = m;
                        printf("[PORT] mdep_getportdata: NATS port got %d bytes\n", r);
                        return r;
                    }
                }
                continue;
            }

            // WebSocket ports
            if (m->portId > 0) {
                r = js_websocket_receive(m->portId, buff, max);
                if (r > 0) {
                    *port = m;
                    m->portstate = PORT_NORMAL;  // Clear can-read flag
                    printf("[PORT] mdep_getportdata: WebSocket port %d got %d bytes\n",
                           m->portId, r);
                    return r;
                }
            }
        }
    }

    return -1;  // No data available
}

int
mdep_getconsole(void)
{
    // Return next keycode from buffer, or -1 if empty
    if (keyboard_buffer_count > 0) {
        KeyEvent *event = &keyboard_buffer[keyboard_buffer_read_pos];
        int keycode = event->keycode;

        // Update current modifier state to match this key event
        current_ctrl_down = event->ctrl;
        current_shift_down = event->shift;
        current_alt_down = event->alt;

        keyboard_buffer_read_pos++;
        if (keyboard_buffer_read_pos >= KEYBOARD_BUFFER_SIZE)
            keyboard_buffer_read_pos = 0;
        keyboard_buffer_count--;

		if ( keycode == 'h' && current_ctrl_down ) {
			keycode = 8; // Ctrl-H as Backspace
		}
        // sprintf(Msg1,"TJT DEBUG mdep_getconsole got keycode %d (ctrl=%d shift=%d alt=%d)\n",
        //     keycode, event->ctrl, event->shift, event->alt);
        // mdep_popup(Msg1);
        return keycode;
    }
    return -1;  // No key available
}

int
mdep_statconsole()
{
    // Check if keyboard input is available in buffer
    return keyboard_buffer_count > 0 ? 1 : 0;
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
    if (cached_font_width < 0) {
        cached_font_width = js_get_font_width();
    }
    return cached_font_width;
}

int
mdep_fontheight(void)
{
    if (cached_font_height < 0) {
        cached_font_height = js_get_font_height() + 6; // Add small padding
    }
    return cached_font_height;
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
		// printf(stderr,"TJT DEBUG mdep_string is calling js_draw_text at %d,%d: %s\n", x, y, s);
		// mdep_popup(Msg1);
		// printf("TJT DEBUG mdep_string s=%s\n",s);
        js_draw_text(x, y + mdep_fontheight() - 4, s);
    }
}

// Color palette - KeyKit uses indexed colors
// Map color indices to RGB values
void
mdep_color(int c)
{
	c = c % KEYNCOLORS;
    current_color_index = c;

	// sprintf(Msg1,"mdep_color c=%d color_list[c] = %s\n", c, color_list[c]);
    // mdep_popup(Msg1);   

    js_set_color(color_list[c]);
}

int
mdep_getcolor()
{
    return current_color_index;
}   

void
mdep_box(int x0, int y0, int x1, int y1)
{
    // Convert from two-point format to x,y,w,h format
    int x = (x0 < x1) ? x0 : x1;
    int y = (y0 < y1) ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);

    // sprintf(Msg1,"TJT DEBUG mdep_box is calling js_draw_rect %d,%d,%d,%d  color_index=%d  rgb=%s\n",
    //     x, y, w, h, current_color_index, color_list[current_color_index]);
    // mdep_popup(Msg1);   

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

    // sprintf(Msg1,"TJT DEBUG mdep_boxfill is calling js_draw_rect %d,%d,%d,%d  color_index=%d  color_rgb=%s\n",
    //     x, y, w, h, current_color_index, color_list[current_color_index]);
    // mdep_popup(Msg1);   

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
    if (b) {
        if (b->ptr) {
            free(b->ptr);
            b->ptr = NULL;
        }
        free(b);
    }
}

int
mdep_startgraphics(int argc, char **argv)
{
    // Initialize graphics system
    // printf("Initializing KeyKit graphics on Canvas...\n");

    *Colors = KEYNCOLORS;
	mdep_initcolors();
    mdep_color(1); // White?

    // Request canvas dimensions
    canvas_width = js_get_canvas_width();
    canvas_height = js_get_canvas_height();

    // Setup event handlers
    js_setup_mouse_events();
    js_setup_keyboard_events();

    // Clear canvas
    js_clear_canvas();

    // Set default font
    js_set_font("16px monospace");

    // Initialize font metrics cache
    cached_font_width = js_get_font_width();
    cached_font_height = js_get_font_height() + 6;

    // Set default color
    mdep_color(1); // White?

    // Use fallback dimensions if canvas returns 0
    if (canvas_width <= 0) canvas_width = 1024;
    if (canvas_height <= 0) canvas_height = 768;

    // printf("Canvas initialized: %dx%d\n", canvas_width, canvas_height);

    return 0;
}

void
mdep_startrealtime(void)
{
    // Start realtime mode - MIDI already initialized in preRun
    // Nothing to do here
}

void
mdep_startreboot(void)
{
    // Handle reboot - just clear the screen
    js_clear_canvas();
}

void
mdep_endgraphics(void)
{
    // Cleanup graphics
}

void
mdep_plotmode(int mode)
{
    // Set plot mode
    if (mode == 2) {
        execerror("mdep_plotmode: mode == 2 is obsolete!");
    } else if (mode == 1) {
        js_set_composite_operation("source-over");
    } else {
        js_set_composite_operation("destination-out");
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
// Returns NULL on success, error message on failure
char *
mdep_fontinit(char *fnt)
{
    // Invalidate font metrics cache when font changes
    cached_font_width = -1;
    cached_font_height = -1;

    // Set the font in JavaScript if a specific font is requested
    if (fnt && *fnt) {
        // Convert KeyKit font name to CSS font specification
        // For now, just use the default monospace
        js_set_font("16px monospace");
    }

    // Return NULL to indicate success
    return NULL;
}

int
mdep_mousewarp(int x, int y)
{
    // Can't warp mouse cursor in browser for security reasons
    return -1;
}

#define MAX_COLOR_VALUE (256*256)

// Color functions
void
mdep_colormix(int c, int r, int g, int b)
{
	if ( c < 0 || c >= KEYNCOLORS ) {
		execerror("mdep_colormix: color index %d out of range\n", c);
	}
	// The values in keykit are 0 to MAX_COLOR_VALUE
	r = (r % MAX_COLOR_VALUE) / 256;
	g = (g % MAX_COLOR_VALUE) / 256;
	b = (b % MAX_COLOR_VALUE) / 256;

	sprintf(color_list[c],"rgb(%d,%d,%d)", r, g, b);

	// sprintf(Msg1,"mdep_colormix c=%d rgb=%s\n", c, color_list[c]);
    // mdep_popup(Msg1);   

    // DON'T set the color right now, just update the color list
    // js_set_color(color_list[c]);
}

void
mdep_initcolors(void)
{
    /*
     * for reference, here are the default values of the various
     * colors used, especially important for buttons 
            int Backcolor = 0;
            int Forecolor = 1;
            int Pickcolor = 2;
            int Lightcolor = 3;
            int Darkcolor = 4;
            int ButtonBGcolor = 5;
     */

	strcpy(color_list[0], "rgb(0,0,0)");       // Black
	strcpy(color_list[1], "rgb(255,255,255)"); // White
	strcpy(color_list[2], "rgb(255,0,0)");     // Red
	strcpy(color_list[3], "rgb(200,200,200)"); // Lighter Grey (button highlight bg)
	strcpy(color_list[4], "rgb(150,150,150)"); // Light grey (button pressed bg)
	strcpy(color_list[5], "rgb(100,100,100)"); // Dark grey (button normal bg)
	strcpy(color_list[6], "rgb(255,255,0)");   // Yellow
	strcpy(color_list[7], "rgb(0,0,255)");     // Blue
	strcpy(color_list[8], "rgb(128,128,128)"); // Gray
	strcpy(color_list[9], "rgb(128,128,255)"); // Light Blue
	strcpy(color_list[10], "rgb(128,255,128)"); // Light Green
	strcpy(color_list[11], "rgb(128,255,255)"); // Light Cyan
	strcpy(color_list[12], "rgb(255,128,128)"); // Light Red
	strcpy(color_list[13], "rgb(255,128,255)"); // Light Magenta
	strcpy(color_list[14], "rgb(255,255,128)"); // Light Yellow
	strcpy(color_list[15], "rgb(192,192,192)"); // Light Gray
    for ( int i=16; i<KEYNCOLORS; i++ ) {
        sprintf(color_list[i],"rgb(255,255,255)");
    }

    current_color_index = 1;
}

// Bitmap functions (Pbitmap is defined in grid.h)
Pbitmap
mdep_allocbitmap(int xsize, int ysize)
{
    Pbitmap pb = (Pbitmap)malloc(sizeof(struct Pbitmap_struct));
    if (pb) {
        // sprintf(Msg1,"TJT DEBUG mdep_allocbitmap to size (%d,%d)\n",
        //     xsize, ysize);
        // mdep_popup(Msg1);
        pb->xsize = xsize;
        pb->ysize = ysize;
        pb->origx = xsize;
        pb->origy = ysize;
        // Allocate RGBA pixel data (4 bytes per pixel)
        int bufsize = xsize * ysize * 4;
        pb->ptr = (unsigned char *)malloc(bufsize);
        if (pb->ptr) {
            memset(pb->ptr, 0, bufsize); // Clear to transparent black
        } else {
            free(pb);
            return NULL;
        }
    }
    return pb;
}

Pbitmap
mdep_reallocbitmap(int xsize, int ysize, Pbitmap pb)
{
    if (pb) {
        // sprintf(Msg1,"TJT DEBUG mdep_reallocbitmap to size (%d,%d)\n",
        //     xsize, ysize);
        // mdep_popup(Msg1);

        // If dimensions changed, reallocate buffer
        if (xsize != pb->origx || ysize != pb->origy) {
            int bufsize = xsize * ysize * 4;
            unsigned char *newptr = (unsigned char *)realloc(pb->ptr, bufsize);
            if (newptr) {
                pb->ptr = newptr;
                pb->origx = xsize;
                pb->origy = ysize;
                pb->xsize = xsize;
                pb->ysize = ysize;
            } else {
                // Realloc failed, keep old buffer
                return pb;
            }
        } else {
            // Just update size (within existing allocation)
            pb->xsize = xsize;
            pb->ysize = ysize;
        }
    }
    return pb;
}

void
mdep_movebitmap(int fromx0, int fromy0, int width, int height, int tox0, int toy0)
{
    // sprintf(Msg1,"TJT DEBUG mdep_movebitmap from (%d,%d) size (%d,%d) to (%d,%d)\n",
    //     fromx0, fromy0, width, height, tox0, toy0);
    // mdep_popup(Msg1);

    // Copy a region of the canvas from one location to another
    js_copy_bitmap_region(fromx0, fromy0, width, height, tox0, toy0);
}

void
mdep_pullbitmap(int x0, int y0, Pbitmap pb)
{
    // sprintf(Msg1,"TJT DEBUG mdep_pullbitmap at (%d,%d) size (%d,%d)\n",
    //     x0, y0, pb->xsize, pb->ysize);
    // mdep_popup(Msg1);

    // Copy pixels from canvas to bitmap buffer
    if (pb && pb->ptr) {
        int bytes = js_get_image_data(x0, y0, pb->xsize, pb->ysize, pb->ptr);
        if (bytes == 0) {
            printf("Warning: mdep_pullbitmap failed to get image data\n");
        }
    }
}

void
mdep_putbitmap(int x0, int y0, Pbitmap pb)
{
    // sprintf(Msg1,"TJT DEBUG mdep_putbitmap at (%d,%d) size (%d,%d)\n",
    //     x0, y0, pb->xsize, pb->ysize);
    // mdep_popup(Msg1);

    // Copy pixels from bitmap buffer to canvas
    if (pb && pb->ptr) {
        int bufsize = pb->xsize * pb->ysize * 4; // RGBA
        js_put_image_data(pb->ptr, bufsize, x0, y0, pb->xsize, pb->ysize);
    }
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
    static char result_filename[512];
    char *result;

    printf("[BROWSE] mdep_browse called: desc='%s' types='%s' mustexist=%d\n", desc, types, mustexist);

    // Trigger the file browser dialog
    js_browse_file(desc, types, mustexist);

    // Wait for the dialog to complete (with ASYNCIFY, this will yield to JS)
    while (!js_browse_is_done()) {
        emscripten_sleep(100);  // Yield to JavaScript event loop
    }

    // Get the result
    result = js_browse_get_result();
    if (result == NULL) {
        printf("[BROWSE] File dialog cancelled or no file selected\n");
        return NULL;
    }

    // Copy to static buffer (result was malloc'd by JS, we need to free it)
    strncpy(result_filename, result, sizeof(result_filename) - 1);
    result_filename[sizeof(result_filename) - 1] = '\0';
    free(result);

    printf("[BROWSE] Selected file: %s\n", result_filename);
    return result_filename;
}

// Port functions
PORTHANDLE *
mdep_openport(char *name, char *mode, char *type)
{
    static PORTHANDLE handle[2];
    Myport *m0, *m1;
    char buff[1024];
    char *p;
    char url[1024];

    name = uniqstr(name);
    printf("[PORT] mdep_openport: name='%s' mode='%s' type='%s'\n", name, mode, type);

    // Parse name format: "port@host" or "port@host:portnum"
    strcpy(buff, name);
    p = strchr(buff, '@');
    if (p == NULL) {
        eprint("Port name must contain a '@' separating port@host!");
        return NULL;
    }
    *p++ = 0;  // buff now has port name, p points to host

    // tcpip_connect - bidirectional WebSocket connection
    if (strcmp(type, "tcpip_connect") == 0) {
        // Format WebSocket URL: ws://host:port/
        snprintf(url, sizeof(url), "ws://%s", p);
        printf("[PORT] tcpip_connect to %s\n", url);

        m0 = newmyport(name);
        m0->rw = TYPE_READ;
        m0->myport_type = MYPORT_TCPIP_READ;
        m0->isopen = 1;
        m0->closeme = 1;

        m1 = newmyport(name);
        m1->rw = TYPE_WRITE;
        m1->myport_type = MYPORT_TCPIP_WRITE;
        m1->isopen = 1;
        m1->portId = m0->portId;  // Share same WebSocket

        // Initiate connection
        if (js_websocket_connect(url, m0->portId) != 0) {
            eprint("tcpip_connect to %s failed", url);
            return NULL;
        }

        handle[0] = m0;
        handle[1] = m1;
        return handle;
    }

    // tcpip_listen - WebSocket server (note: browsers can't create servers,
    // so this would require a WebSocket relay server)
    if (strcmp(type, "tcpip_listen") == 0) {
        printf("[PORT] tcpip_listen not fully supported in browser (would need relay server)\n");
        // For now, return stub that indicates listen mode
        m0 = newmyport(name);
        m0->rw = TYPE_LISTEN;
        m0->myport_type = MYPORT_TCPIP_LISTEN;
        m0->isopen = 1;
        m0->closeme = 1;
        m0->sockstate = SOCK_LISTENING;
        handle[0] = m0;
        handle[1] = NULL;
        return handle;
    }

    // udp_send - WebSocket for UDP simulation
    if (strcmp(type, "udp_send") == 0) {
        snprintf(url, sizeof(url), "ws://%s", p);
        printf("[PORT] udp_send to %s\n", url);

        m1 = newmyport(name);
        m1->rw = TYPE_WRITE;
        m1->myport_type = MYPORT_UDP_WRITE;
        m1->isopen = 1;
        m1->closeme = 1;

        if (js_websocket_connect(url, m1->portId) != 0) {
            eprint("udp_send to %s failed", url);
            return NULL;
        }

        handle[0] = NULL;
        handle[1] = m1;
        return handle;
    }

    // udp_listen - WebSocket for UDP reception
    if (strcmp(type, "udp_listen") == 0) {
        snprintf(url, sizeof(url), "ws://%s", p);
        printf("[PORT] udp_listen on %s\n", url);

        m0 = newmyport(name);
        m0->rw = TYPE_LISTEN;
        m0->myport_type = MYPORT_UDP_LISTEN;
        m0->isopen = 1;
        m0->closeme = 1;

        if (js_websocket_connect(url, m0->portId) != 0) {
            eprint("udp_listen to %s failed", url);
            return NULL;
        }

        m0->sockstate = SOCK_LISTENING;
        handle[0] = m0;
        handle[1] = NULL;
        return handle;
    }

    // osc_send - OSC over WebSocket
    if (strcmp(type, "osc_send") == 0) {
        snprintf(url, sizeof(url), "ws://%s", p);
        printf("[PORT] osc_send to %s\n", url);

        m1 = newmyport(name);
        m1->rw = TYPE_WRITE;
        m1->myport_type = MYPORT_OSC_WRITE;
        m1->isopen = 1;
        m1->closeme = 1;

        if (js_websocket_connect(url, m1->portId) != 0) {
            eprint("osc_send to %s failed", url);
            return NULL;
        }

        handle[0] = NULL;
        handle[1] = m1;
        return handle;
    }

    // osc_listen - OSC reception over WebSocket
    if (strcmp(type, "osc_listen") == 0) {
        snprintf(url, sizeof(url), "ws://%s", p);
        printf("[PORT] osc_listen on %s\n", url);

        m0 = newmyport(name);
        m0->rw = TYPE_LISTEN;
        m0->myport_type = MYPORT_OSC_LISTEN;
        m0->isopen = 1;
        m0->closeme = 1;

        if (js_websocket_connect(url, m0->portId) != 0) {
            eprint("osc_listen to %s failed", url);
            return NULL;
        }

        m0->sockstate = SOCK_LISTENING;
        handle[0] = m0;
        handle[1] = NULL;
        return handle;
    }

    // nats_send - NATS publish
    // Format: "subject@nats://server:port"
    if (strcmp(type, "nats_send") == 0) {
        // buff contains subject, p contains server URL
        printf("[PORT] nats_send subject='%s' server='%s'\n", buff, p);

        // Connect to NATS server if not already connected
        if (!js_nats_is_connected()) {
            snprintf(url, sizeof(url), "ws://%s", p);
            if (js_nats_connect(url) != 0) {
                eprint("NATS connection to %s failed", url);
                return NULL;
            }
            // Connection is async, will complete later
        }

        m1 = newmyport(name);
        m1->rw = TYPE_WRITE;
        m1->myport_type = MYPORT_NATS_WRITE;
        m1->isopen = 1;
        m1->closeme = 1;
        m1->nats_subject = uniqstr(buff);  // Store subject
        m1->sockstate = SOCK_CONNECTED;  // NATS handles connection state

        handle[0] = NULL;
        handle[1] = m1;
        return handle;
    }

    // nats_listen - NATS subscribe
    // Format: "subject@nats://server:port"
    if (strcmp(type, "nats_listen") == 0) {
        printf("[PORT] nats_listen subject='%s' server='%s'\n", buff, p);

        // Connect to NATS server if not already connected
        if (!js_nats_is_connected()) {
            snprintf(url, sizeof(url), "ws://%s", p);
            if (js_nats_connect(url) != 0) {
                eprint("NATS connection to %s failed", url);
                return NULL;
            }
        }

        // Subscribe to the subject
        if (js_nats_subscribe(buff) != 0) {
            eprint("NATS subscribe to %s failed", buff);
            return NULL;
        }

        m0 = newmyport(name);
        m0->rw = TYPE_LISTEN;
        m0->myport_type = MYPORT_NATS_LISTEN;
        m0->isopen = 1;
        m0->closeme = 1;
        m0->nats_subject = uniqstr(buff);  // Store subject
        m0->sockstate = SOCK_LISTENING;

        handle[0] = m0;
        handle[1] = NULL;
        return handle;
    }

    eprint("Unknown port type - %s", type);
    return NULL;
}

int
mdep_putportdata(PORTHANDLE m, char *buff, int size)
{
    Myport *mp = (Myport *)m;
    int r;

    printf("[PORT] mdep_putportdata: portId=%d size=%d sockstate=%d type=%d\n",
           mp->portId, size, mp->sockstate, mp->myport_type);

    switch (mp->myport_type) {
    case MYPORT_NATS_WRITE:
        // NATS publish - need to convert binary to string
        {
            char *data_str = (char *)malloc(size + 1);
            if (data_str) {
                memcpy(data_str, buff, size);
                data_str[size] = '\0';

                printf("[PORT] NATS publish to subject '%s': %s\n",
                       mp->nats_subject, data_str);

                r = js_nats_publish(mp->nats_subject, data_str);
                free(data_str);

                if (r == 0) {
                    r = size;  // Success - return bytes "sent"
                }
            } else {
                r = -1;
            }
        }
        break;

    case MYPORT_OSC_WRITE:
    case MYPORT_UDP_WRITE:
        // Direct send for UDP-like protocols
        r = js_websocket_send(mp->portId, buff, size);
        break;

    default:
        // TCP-like protocols - buffer if not connected
        switch (mp->sockstate) {
        case SOCK_UNCONNECTED:
            // Buffer for delivery when it connects
            sockaway(mp, buff, size);
            r = size;
            break;

        case SOCK_CLOSED:
        case SOCK_REFUSED:
            r = 0;  // Can't send
            break;

        default:
            // Connected - send directly
            r = js_websocket_send(mp->portId, buff, size);
            if (r < 0) {
                // Send failed, buffer it
                sockaway(mp, buff, size);
                r = 0;
            }
            break;
        }
        break;
    }

    return r;
}

int
mdep_closeport(PORTHANDLE m)
{
    Myport *mp = (Myport *)m;
    Myport *prevmp, *currmp;
    int r = 0;

    printf("[PORT] mdep_closeport: portId=%d type=%d\n", mp->portId, mp->myport_type);

    mp->isopen = 0;

    // Close connection based on type
    switch (mp->myport_type) {
    case MYPORT_NATS_WRITE:
    case MYPORT_NATS_LISTEN:
        // NATS ports share connection - don't close unless no other NATS ports
        // For now, just mark as closed (NATS connection managed globally)
        printf("[PORT] NATS port closed (connection remains active)\n");
        break;

    case MYPORT_TCPIP_READ:
    case MYPORT_TCPIP_WRITE:
    case MYPORT_TCPIP_LISTEN:
    case MYPORT_UDP_WRITE:
    case MYPORT_UDP_LISTEN:
    case MYPORT_OSC_WRITE:
    case MYPORT_OSC_LISTEN:
        if (mp->closeme) {
            r = js_websocket_close(mp->portId);
        }
        break;
    default:
        break;
    }

    // Remove from Topport list
    for (prevmp = NULL, currmp = Topport; currmp != NULL; prevmp = currmp, currmp = currmp->next) {
        if (currmp == mp)
            break;
    }
    if (prevmp == NULL)
        Topport = mp->next;
    else
        prevmp->next = mp->next;

    // Free buffered data
    if (mp->buff)
        free(mp->buff);

    // Free NATS subject if present
    // Note: uniqstr strings are shared/interned, so don't free them

    free(mp);
    return r;
}

Datum
mdep_ctlport(PORTHANDLE m, char *cmd, char *arg)
{
    Datum d;
    d.type = D_NUM;
    d.u.val = 0;

    // No special control commands for WebSocket ports yet
    // Could add things like setting binary mode, buffering options, etc.

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

// ========== NATS Messaging Implementation ==========

// NATS callback - called from JavaScript when message arrives
// Keep this minimal to avoid ASYNCIFY issues
EMSCRIPTEN_KEEPALIVE
void mdep_on_nats_message(const char *subject, const char *data)
{
    // Simply buffer the message for later processing
    if (nats_message_count < NATS_MESSAGE_BUFFER_SIZE) {
        // Allocate and copy subject
        int subject_len = strlen(subject);
        nats_message_buffer[nats_message_write_pos].subject = (char *)malloc(subject_len + 1);
        if (nats_message_buffer[nats_message_write_pos].subject) {
            strcpy(nats_message_buffer[nats_message_write_pos].subject, subject);
        }

        // Allocate and copy data
        int data_len = strlen(data);
        nats_message_buffer[nats_message_write_pos].data = (char *)malloc(data_len + 1);
        if (nats_message_buffer[nats_message_write_pos].data) {
            strcpy(nats_message_buffer[nats_message_write_pos].data, data);
        }

        nats_message_write_pos = (nats_message_write_pos + 1) % NATS_MESSAGE_BUFFER_SIZE;
        nats_message_count++;

        printf("[NATS C] Buffered message: subject='%s' data='%s' (buffer count=%d)\n",
               subject, data, nats_message_count);
    } else {
        printf("[NATS C] WARNING: Message buffer full, dropping message on '%s'\n", subject);
    }
}

// Check if NATS messages are available for a specific subject
static int nats_has_message_for_subject(const char *subject)
{
    if (nats_message_count == 0) {
        return 0;
    }

    // Check if any buffered message matches the subject
    int pos = nats_message_read_pos;
    int checked = 0;
    while (checked < nats_message_count) {
        struct nats_msg_buffer *msg = &nats_message_buffer[pos];
        if (msg->subject && strcmp(msg->subject, subject) == 0) {
            return 1;
        }
        pos = (pos + 1) % NATS_MESSAGE_BUFFER_SIZE;
        checked++;
    }

    return 0;
}

// Get next NATS message for a specific subject
static int nats_get_message_for_subject(const char *subject, char *buffer, int buffer_size)
{
    if (nats_message_count == 0) {
        return 0;
    }

    // Find first message matching subject
    int pos = nats_message_read_pos;
    int checked = 0;
    int found_at = -1;

    while (checked < nats_message_count) {
        struct nats_msg_buffer *msg = &nats_message_buffer[pos];
        if (msg->subject && strcmp(msg->subject, subject) == 0) {
            found_at = pos;
            break;
        }
        pos = (pos + 1) % NATS_MESSAGE_BUFFER_SIZE;
        checked++;
    }

    if (found_at < 0) {
        return 0;  // No matching message
    }

    // Copy message data to buffer
    struct nats_msg_buffer *msg = &nats_message_buffer[found_at];
    int data_len = strlen(msg->data);
    int copy_len = (data_len < buffer_size - 1) ? data_len : (buffer_size - 1);
    memcpy(buffer, msg->data, copy_len);
    buffer[copy_len] = '\0';

    // Free the message
    free(msg->subject);
    free(msg->data);
    msg->subject = NULL;
    msg->data = NULL;

    // Compact buffer by moving later messages forward if needed
    if (found_at == nats_message_read_pos) {
        // Message at front - just advance read position
        nats_message_read_pos = (nats_message_read_pos + 1) % NATS_MESSAGE_BUFFER_SIZE;
    } else {
        // Message in middle - shift remaining messages
        int curr = found_at;
        while (curr != nats_message_write_pos) {
            int next = (curr + 1) % NATS_MESSAGE_BUFFER_SIZE;
            nats_message_buffer[curr] = nats_message_buffer[next];
            curr = next;
        }
        // Adjust write position
        nats_message_write_pos = (nats_message_write_pos - 1 + NATS_MESSAGE_BUFFER_SIZE) % NATS_MESSAGE_BUFFER_SIZE;
    }

    nats_message_count--;

    printf("[NATS C] Retrieved message from buffer for subject '%s' (remaining: %d)\n",
           subject, nats_message_count);

    return copy_len;
}

// ========== Network Port Implementation (WebSocket-based) ==========

// Create new port
static Myport *newmyport(char *name)
{
    Myport *m = (Myport *)malloc(sizeof(Myport));
    if (!m) return NULL;

    m->name = uniqstr(name);
    m->portId = next_port_id++;
    m->rw = TYPE_NONE;
    m->myport_type = 0;
    m->sockstate = SOCK_UNCONNECTED;
    m->portstate = PORT_NORMAL;
    m->isopen = 0;
    m->closeme = 0;
    m->hasreturnedfinaldata = 0;
    m->buff = NULL;
    m->buffsize = 0;
    m->nats_subject = NULL;

    // Add to list
    m->next = Topport;
    Topport = m;

    return m;
}

// Buffer data when socket not ready
static void sockaway(Myport *m, char *buff, int size)
{
    if (m->buff == NULL) {
        m->buff = (char *)malloc(size);
        if (m->buff) {
            m->buffsize = size;
            memcpy(m->buff, buff, size);
        }
    } else {
        char *newbuff = (char *)malloc(size + m->buffsize);
        if (newbuff) {
            memcpy(newbuff, m->buff, m->buffsize);
            memcpy(newbuff + m->buffsize, buff, size);
            free(m->buff);
            m->buff = newbuff;
            m->buffsize += size;
        }
    }
}

// Send buffered data when socket connects
static void sendsockedaway(Myport *mp)
{
    if (mp->sockstate != SOCK_CONNECTED) {
        return;
    }
    if (mp->buff != NULL) {
        char *p = mp->buff;
        int sz = mp->buffsize;
        mp->buff = NULL;
        mp->buffsize = 0;
        js_websocket_send(mp->portId, p, sz);
        free(p);
    }
}

// WebSocket event callback from JavaScript
EMSCRIPTEN_KEEPALIVE
void mdep_on_websocket_event(int portId, const char *event)
{
    printf("[PORT C] WebSocket event on port %d: %s\n", portId, event);

    // Find the port
    Myport *m;
    for (m = Topport; m != NULL; m = m->next) {
        if (m->portId == portId) {
            if (strcmp(event, "open") == 0) {
                m->sockstate = SOCK_CONNECTED;
                sendsockedaway(m);  // Send any buffered data
            } else if (strcmp(event, "data") == 0) {
                m->portstate = PORT_CANREAD;
            } else if (strcmp(event, "close") == 0) {
                m->sockstate = SOCK_CLOSED;
            } else if (strcmp(event, "error") == 0) {
                m->sockstate = SOCK_REFUSED;
            }
            break;
        }
    }
}
