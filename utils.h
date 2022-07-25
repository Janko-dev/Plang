#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

enum ErrorType {
    LEX_ERROR,
    PARSE_ERROR,
    RUNTIME_ERROR
};

static void plerror(int line, enum ErrorType type, const char* message, ...);

#endif  //_UTILS_H


#ifdef UTILS_IMPLEMENT
static char* errtypes[] = {"LEX_ERROR", "PARSE_ERROR"};

static void plerror(int line, enum ErrorType type, const char* message, ...){
    fprintf(stderr, "\n%s [line %d]: ", errtypes[type], line);
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
}
#endif