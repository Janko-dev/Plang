#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "tokenizer.h"
#include "parser.h"

#define ENV_SIZE 100

typedef struct envList EnvMap;
struct envList {
    struct envList* next;
    char* key;
    LiteralExpr value;
};

typedef struct Env_t Env;
struct Env_t {
    EnvMap** map;
    struct Env_t* enclosing;
};

Env* create_env(Env* enclosing);
void free_env(Env* env);

void define(Env* env, char* key, LiteralExpr value);
void assign(Env* env, Token* name, LiteralExpr value);
LiteralExpr get(Env* env, Token* name);

void interpret(StmtList* list, Env* env, char* code_source);

#endif // _INTERPRETER_H