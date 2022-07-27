#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "lexer.h"
#include "parser.h"

typedef struct {
    enum LiteralType type;
    union {
        double num;
        char* string;
        bool boolean;
    } as;
} Obj;

#define ENV_SIZE 100
typedef struct envList Env;
struct envList {
    struct envList* next;
    char* key;
    Obj* value;
};

Obj* newObj(enum LiteralType type);
Obj* newNum(double num);
Obj* newString(char* string);
Obj* newBool(bool b);

void freeEnv(Env* env[ENV_SIZE]);
void define(Env* env[ENV_SIZE], char* key, Obj* value);
void assign(Env* env[ENV_SIZE], Token* name, Obj* value);
Obj* get(Env* env[ENV_SIZE], Token* name);

void interpret(StmtList* list, Env* env[ENV_SIZE]);

#endif // _INTERPRETER_H