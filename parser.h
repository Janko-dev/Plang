#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

// typedef struct binexpr_t BinaryExpr;
// typedef struct unexpr_t UnaryExpr;
// typedef struct litexpr_t LiteralExpr;
// typedef struct groupexpr_t GroupingExpr;

typedef struct Expr Expr;

enum ExprType {
    BINARY,
    UNARY,
    LITERAL,
    GROUPING
};

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
// void freeAst(Expr* expr);
Expr* parse(TokenList* list);

Expr* expression();
Expr* equality();
Expr* comparison();
Expr* term();
Expr* factor();
Expr* unary();
Expr* primary();

#endif //_PARSER_H