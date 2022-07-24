#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

enum ExprType {
    BINARY,
    UNARY,
    LITERAL,
    GROUPING
};

typedef struct Expr Expr;

typedef struct {
    Expr* left;
    Token* op;
    Expr* right;
} BinaryExpr;

typedef struct {
    Token* op;
    Expr* right;
} UnaryExpr;

typedef struct {
    enum TokenTypes type;
    void* value;
} LiteralExpr;

typedef struct {
    Expr* expression;
} GroupingExpr;

struct Expr {
    enum ExprType type;
    union {
        BinaryExpr binary;
        UnaryExpr unary;
        LiteralExpr literal;
        GroupingExpr group;
    } as;
};

void AstPrinter(Expr* expr);
void freeAst(Expr* expr);
Expr* parse(TokenList* list);

#endif //_PARSER_H