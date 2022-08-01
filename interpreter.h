#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "tokenizer.h"
#include "parser.h"

typedef struct {
    ValueType type;
    union {
        double num;
        char* string;
        bool boolean;
    } as;
} Obj;

#define ENV_SIZE 100

typedef struct envList EnvMap;
struct envList {
    struct envList* next;
    char* key;
    Obj* value;
};

typedef struct Env_t Env;
struct Env_t {
    EnvMap** map;
    struct Env_t* enclosing;
};

Obj* newObj(ValueType type);
Obj* newNum(double num);
Obj* newString(char* string);
Obj* newBool(bool b);

Env* createEnv(Env* enclosing);
void freeEnv(Env* env);

void define(Env* env, char* key, Obj* value);
void assign(Env* env, Token* name, Obj* value);
Obj* get(Env* env, Token* name);

void interpret(StmtList* list, Env* env);

#endif // _INTERPRETER_H