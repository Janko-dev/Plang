#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum {
    TOKEN_ERR,
    PARSE_ERR,
    RUNTIME_ERR,
    MEMORY_ERR
} ErrorType;

// static void plerror(int line, enum ErrorType type, const char* message, ...);
extern bool hadError;

#endif  //_UTILS_H

#ifdef UTILS_IMPLEMENT
static char* errtypes[] = {"Tokenization Error", "Parse Error", "Runtime Error", "Memory Error"};

static void plerror(int line, int col, ErrorType type, const char* message, ...){
    if (line != -1 && col != -1) fprintf(stderr, "%s [line %d:%d]: ", errtypes[type], line, col);
    else fprintf(stderr, "%s : ", errtypes[type]);
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
    hadError = true;
}
#endif