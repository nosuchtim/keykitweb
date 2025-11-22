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

// Bitmap functions
extern int js_get_image_data(int x, int y, int width, int height, unsigned char *buffer);
extern void js_put_image_data(unsigned char *buffer, int bufLen, int x, int y, int width, int height);
extern void js_copy_bitmap_region(int fromx, int fromy, int width, int height, int tox, int toy);

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
    fprintf(stderr, "POPUP: %s", s);
    char *eol = s + strlen(s) - 1;
    if ( *eol != '\n' ) {
        fprintf(stderr, "\n");
    }
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

// Callback from JavaScript when MIDI message is received
EMSCRIPTEN_KEEPALIVE
void mdep_on_midi_message(int device_index, int status, int data1, int data2)
{
	mdep_popup("TJT DEBUG mdep_on_midi_message callback!!!! ");
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
    // Update current mouse position
    current_mouse_x = x;
    current_mouse_y = y;

    // Buffer the mouse move event
    if (mouse_buffer_count < MOUSE_BUFFER_SIZE) {
        mouse_buffer[mouse_buffer_write_pos].x = x;
        mouse_buffer[mouse_buffer_write_pos].y = y;
        mouse_buffer[mouse_buffer_write_pos].buttons = current_mouse_buttons;
        mouse_buffer[mouse_buffer_write_pos].event_type = 0; // move
        mouse_buffer_write_pos++;
        if (mouse_buffer_write_pos >= MOUSE_BUFFER_SIZE)
            mouse_buffer_write_pos = 0;
        mouse_buffer_count++;
    }
}

// Callback from JavaScript for mouse button events
EMSCRIPTEN_KEEPALIVE
void mdep_on_mouse_button(int down, int x, int y, int buttons)
{
    // Update current mouse state
    current_mouse_x = x;
    current_mouse_y = y;
    current_mouse_buttons = buttons;

    // Buffer the mouse button event
    if (mouse_buffer_count < MOUSE_BUFFER_SIZE) {
        mouse_buffer[mouse_buffer_write_pos].x = x;
        mouse_buffer[mouse_buffer_write_pos].y = y;
        mouse_buffer[mouse_buffer_write_pos].buttons = buttons;
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
int mdep_get_mouse_event(int *x, int *y, int *buttons, int *event_type)
{
    if (mouse_buffer_count > 0) {
        MouseEvent *event = &mouse_buffer[mouse_buffer_read_pos];
        *x = event->x;
        *y = event->y;
        *buttons = event->buttons;
        *event_type = event->event_type;

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
	char *args[3];
	int n;
	Datum d;

	mdep_popup("===== MDEP_MDEP !!!!!!!!!!!!!!!!!\n");
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
		mdep_popup("===== mdep env !!!!  \n");
	    if ( strcmp(args[1],"get")==0 ) {
			mdep_popup("===== mdep env get\n");
			char *s = getenv(args[2]);
			sprintf(Msg2,"===== mdep env get got %s\n",s);
			mdep_popup(Msg2);
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
#if KEYCAPTURE
	    if ( strcmp(args[1],"capture")==0 ) {
		if ( strcmp(args[2],"stop") == 0 ) {
			key_stop_capture();
		} else if ( strcmp(args[2],"start") == 0 ) {
			int gx;
			int gy;
			Datum d3 = ARG(3);
			Datum d4 = ARG(4);

			key_start_capture();
			gx = roundval(d3);
			gy = roundval(d4);
			setvideogrid(gx,gy);
		} else {
			execerror("mdep(\"video\",\"capture\",...): %s not recognized\n",args[2]);
		}

	    } else if ( strcmp(args[1],"start")==0 ) {
		int gx = 32;
		int gy = 32;
		if (!startvideo())
			d = numdatum(0);
		else
			d = numdatum(1);

	    } else if ( strcmp(args[1],"get")==0
			&& *args[3] != 0 && *args[4] != 0 ) {

			Datum d3 = ARG(3);
			Datum d4 = ARG(4);
			int gx = roundval(d3);
			int gy = roundval(d4);
			int v;
			int offset = gy * GridXsize + gx;

			args[2] = needstr("mdep",ARG(2));

			if ( strcmp(args[2],"red")==0 )
				v = GridRedAvg[offset];
			else if ( strcmp(args[2],"green")==0 )
				v = GridGreenAvg[offset];
			else if ( strcmp(args[2],"blue")==0 )
				v = GridBlueAvg[offset];
			else if ( strcmp(args[2],"grey")==0 )
				v = GridGreyAvg[offset];
			else
				execerror("mdep(\"video\",\"get\",...): %s is unrecognized\n",args[2]);
			d = numdatum(v);

	    } else if ( strcmp(args[1],"getaverage")==0 ) {

			int gx, gy;
			long redtot = 0;
			long greentot = 0;
			long bluetot = 0;
			long greytot = 0;
			int redavg, greenavg, blueavg, greyavg;
			int nxy = GridXsize * GridYsize;

			for ( gx=0; gx<GridXsize; gx++ ) {
				for ( gy=0; gy<GridYsize; gy++ ) {
					int offset = gy * GridXsize + gx;
					int rv = GridRedAvgPrev[offset];
					int gv = GridGreenAvgPrev[offset];
					int bv = GridBlueAvgPrev[offset];
					redtot += rv;
					greentot += gv;
					bluetot += bv;
					greytot += (rv+gv+bv);
				}
			}
			redavg = redtot / nxy;
			greenavg = greentot / nxy;
			blueavg = bluetot / nxy;
			greyavg = greytot / nxy;

			d = newarrdatum(0,3);
			setarraydata(d.u.arr,
				Str_red,numdatum(redavg));
			setarraydata(d.u.arr,
				Str_green,numdatum(greenavg));
			setarraydata(d.u.arr,
				Str_blue,numdatum(blueavg));
			setarraydata(d.u.arr,
				Str_grey,numdatum(greyavg));

	    } else if ( strcmp(args[1],"getchange")==0
			&& *args[3] != 0 && *args[4] != 0 ) {

			Datum d3 = ARG(3);
			Datum d4 = ARG(4);
			int gx = roundval(d3);
			int gy = roundval(d4);
			int v;
			int dv;
			int offset = gy * GridXsize + gx;

			args[2] = needstr("mdep",ARG(2));

			if ( strcmp(args[2],"red")==0 ) {
				v = GridRedAvg[offset];
				dv = v - GridRedAvgPrev[offset];
				GridRedAvgPrev[offset] = v;
			} else if ( strcmp(args[2],"green")==0 ) {
				v = GridGreenAvg[offset];
				dv = v - GridGreenAvgPrev[offset];
				GridGreenAvgPrev[offset] = v;
			} else if ( strcmp(args[2],"blue")==0 ) {
				v = GridBlueAvg[offset];
				dv = v - GridBlueAvgPrev[offset];
				GridBlueAvgPrev[offset] = v;
			} else if ( strcmp(args[2],"grey")==0 ) {
				v = GridGreyAvg[offset];
				dv = v - GridGreyAvgPrev[offset];
				GridGreyAvgPrev[offset] = v;
			} else
				execerror("mdep(\"video\",\"get\",...): %s is unrecognized\n",args[2]);
			if ( dv == v )
				dv = 0;
			d = numdatum(dv);

	    } else {
		eprint("Error: unrecognized video argument - %s\n",args[1]);
	    }
#else
	    execerror("mdep(\"video\",...): keykit not compiled with video support\n");
#endif
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
	if ( mdep_statconsole() ) {
		return K_CONSOLE;
	}
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
    return js_get_font_width();
}

int
mdep_fontheight(void)
{
    return js_get_font_height();
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
        js_draw_text(x, y + mdep_fontheight(), s);
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
	printf("mdep_color setting color index %d -> %s\n", c, current_color_rgb);
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

	/*
    // Draw a filled red square in the middle of the canvas as a test
    int size = 100;
    int x = (canvas_width - size) / 2;
    int y = (canvas_height - size) / 2;
    js_set_fill_color("red");
    js_fill_rect(x, y, size, size);
    printf("Drew red square at (%d, %d) size %d\n", x, y, size);
	*/

    return 0;
}

void
mdep_startrealtime(void)
{
    // Start realtime mode - request MIDI access
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
    // Return current mouse state (updated by callbacks)
    *ax = current_mouse_x;
    *ay = current_mouse_y;
    *am = current_mouse_buttons;
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
