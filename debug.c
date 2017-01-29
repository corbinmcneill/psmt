#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

void debug(const char* format, ...) {
    if (DEBUG) {
        va_list arguments;
        va_start(arguments, format);
        vfprintf(stderr, format, arguments);
        va_end(arguments); 
    }

}
