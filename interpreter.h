#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "lexer.h"
#include "parser.h"

#define ENV_SIZE 100
typedef struct envList Env;
struct envList {
    struct envList* next;
    char* key;
    LiteralExpr value;
};

void freeEnv(Env* env[ENV_SIZE]);
void define(Env* env[ENV_SIZE], char* key, LiteralExpr value);
void assign(Env* env[ENV_SIZE], Token* name, LiteralExpr value);
LiteralExpr get(Env* env[ENV_SIZE], Token* name);

void interpret(StmtList* list, Env* env[ENV_SIZE]);

#endif // _INTERPRETER_H