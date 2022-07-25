#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

enum ExprType {
    BINARY,
    TERNARY,
    UNARY,
    LITERAL,
    GROUPING
};

enum LiteralType {
    NIL_T,
    NUM_T,
    STR_T,
    BOOL_T
};

typedef struct Expr Expr;

typedef struct {
    Expr* left;
    Token* op;
    Expr* right;
} BinaryExpr;

typedef struct {
    Expr* cond;
    Expr* trueBranch;
    Expr* falseBranch;
} TernaryExpr;

typedef struct {
    Token* op;
    Expr* right;
} UnaryExpr;

typedef struct {
    enum LiteralType type;
    void* value;
} LiteralExpr;

typedef struct {
    Expr* expression;
} GroupingExpr;

struct Expr {
    enum ExprType type;
    union {
        BinaryExpr binary;
        TernaryExpr ternary;
        UnaryExpr unary;
        LiteralExpr literal;
        GroupingExpr group;
    } as;
};

void AstPrinter(Expr* expr);
LiteralExpr evaluate(Expr* expr);
void freeAst(Expr* expr);
Expr* parse(TokenList* list);

#endif //_PARSER_H