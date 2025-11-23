/*
 *	Machine-dependent header for WebAssembly (Emscripten)
 */

#ifndef MDEP_WASM_H
#define MDEP_WASM_H

#define MACHINE "wasm"
#define MDEP_MIDI_PROVIDED

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>

#ifndef MAXLONG
#define MAXLONG LONG_MAX
#endif

#define MAIN(ac,av) main(ac,av)

#define OPENFILE(f,name,mode,binmode) {char m[3]; \
	m[0] = mode[0]; \
	m[1] = binmode; \
	m[2] = '\0'; \
	f = fopen(name,m);}

#define OPENBINFILE(f,name,mode) OPENFILE(f,name,mode,'b');
#define OPENTEXTFILE(f,name,mode) OPENFILE(f,name,mode,'t');

#define ALLOCNT 1000

#define STACKSIZE 512
#define ARRAYHASHSIZE 503
#define STRHASHSIZE 503
#define PATHSEP ":"
#define SEPARATOR "/"

typedef void (*SIGFUNCTYPE)(int);

#define PORTHANDLE void*

#ifndef NO_RETURN_ATTRIBUTE
#define NO_RETURN_ATTRIBUTE __attribute__((noreturn))
#endif

// Forward declarations
struct Midiport_struct;
struct Datum;
struct Pbitmap_struct;
typedef struct Pbitmap_struct *Pbitmap;

// Basic system functions
void mdep_hello(int argc, char **argv);
void mdep_bye(void);
int mdep_changedir(char *d);
char *mdep_currentdir(char *buff, int leng);
int mdep_lsdir(char *dir, char *exp, void (*callback)(char *, int));
long mdep_filetime(char *fn);
int mdep_fisatty(FILE *f);
long mdep_currtime(void);
long mdep_coreleft(void);
int mdep_full_or_relative_path(char *path);
int mdep_makepath(char *dirname, char *filename, char *result, int resultsize);
void mdep_popup(char *s);
void mdep_setcursor(int c);
void mdep_prerc(void);
void mdep_postrc(void);
void mdep_abortexit(char *msg);
void mdep_setinterrupt(SIGFUNCTYPE func);
void mdep_ignoreinterrupt(void);
void mdep_sync(void);
long mdep_milliclock(void);
void mdep_resetclock(void);

// MIDI functions
int mdep_getnmidi(char *buff, int buffsize, int *port);
void mdep_putnmidi(int n, char *cp, struct Midiport_struct *pport);
int mdep_initmidi(struct Midiport_struct *inputs, struct Midiport_struct *outputs);
void mdep_endmidi(void);
int mdep_midi(int openclose, struct Midiport_struct *p);

// Port functions
int mdep_getportdata(PORTHANDLE *port, char *buff, int max, struct Datum *data);
int mdep_getconsole(void);
int mdep_statconsole(void);
int mdep_waitfor(int millimsecs);
PORTHANDLE *mdep_openport(char *name, char *mode, char *type);
int mdep_putportdata(PORTHANDLE m, char *buff, int size);
int mdep_closeport(PORTHANDLE m);
struct Datum mdep_ctlport(PORTHANDLE m, char *cmd, char *arg);

// Graphics and windowing
int mdep_maxx(void);
int mdep_maxy(void);
int mdep_fontwidth(void);
int mdep_fontheight(void);
void mdep_line(int x0, int y0, int x1, int y1);
void mdep_string(int x, int y, char *s);
void mdep_color(int c);
void mdep_box(int x0, int y0, int x1, int y1);
void mdep_boxfill(int x0, int y0, int x1, int y1);
void mdep_ellipse(int x0, int y0, int x1, int y1);
void mdep_fillellipse(int x0, int y0, int x1, int y1);
void mdep_fillpolygon(int *x, int *y, int n);
void mdep_freebitmap(Pbitmap b);
int mdep_startgraphics(int argc, char **argv);
void mdep_endgraphics(void);
void mdep_startrealtime(void);
void mdep_startreboot(void);
void mdep_plotmode(int mode);
int mdep_screensize(int *x0, int *y0, int *x1, int *y1);
int mdep_screenresize(int x0, int y0, int x1, int y1);
void mdep_destroywindow(void);

// Font functions
char *mdep_fontinit(char *fnt);

// Mouse functions
int mdep_mouse(int *ax, int *ay, int *am);
int mdep_mousewarp(int x, int y);

// Color functions
void mdep_colormix(int n, int r, int g, int b);
void mdep_initcolors(void);

// Bitmap functions
Pbitmap mdep_allocbitmap(int xsize, int ysize);
Pbitmap mdep_reallocbitmap(int xsize, int ysize, Pbitmap pb);
void mdep_movebitmap(int fromx0, int fromy0, int width, int height, int tox0, int toy0);
void mdep_pullbitmap(int x0, int y0, Pbitmap pb);
void mdep_putbitmap(int x0, int y0, Pbitmap pb);

// File/path functions
char *mdep_keypath(void);
char *mdep_musicpath(void);
int mdep_shellexec(char *s);
char *mdep_browse(char *desc, char *types, int mustexist);
int mdep_help(char *fname, char *keyword);
char *mdep_localaddresses(struct Datum d);

#endif
