#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Print stack trace from C code
static inline void print_stack_trace() {
    EM_ASM({
        console.log('=== C Stack Trace ===');
        console.trace();
    });
}

// Print stack trace with a message
static inline void print_stack_trace_msg(const char *msg) {
    printf("%s\n", msg);
    print_stack_trace();
}

#else
// Stub for non-Emscripten builds
static inline void print_stack_trace() {
    printf("Stack trace not available (not Emscripten build)\n");
}

static inline void print_stack_trace_msg(const char *msg) {
    printf("%s\n", msg);
    print_stack_trace();
}
#endif

#endif // DEBUG_UTILS_H
