#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

typedef struct binexpr_t BinaryExpr;
typedef struct unexpr_t UnaryExpr;
typedef struct litexpr_t LiteralExpr;
typedef struct groupexpr_t GroupingExpr;

enum ExprType {
    BINARY,
    UNARY,
    LITERAL,
    GROUPING
};

typedef struct {
    enum ExprType type;
    union {
        BinaryExpr* binexpr;
        UnaryExpr* unexpr;
        LiteralExpr* litexpr;
        GroupingExpr* groupexpr;
    };
} Expr;

struct binexpr_t{
    Expr* left;
    Token* op;
    Expr* right;
};

struct unexpr_t{
    Token* op;
    Expr* right;
};

struct litexpr_t{
    enum TokenTypes type;
    void* value;
};

struct groupexpr_t{
    Expr* expression;
};

void AstPrinter(Expr* expr);
void freeAst(Expr* expr);
Expr* parse(TokenList* list);

#endif //_PARSER_H