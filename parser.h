#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"
#include <stdarg.h>

// Enum types
enum ExprType {
    BINARY,
    TERNARY,
    UNARY,
    LITERAL,
    GROUPING,
    VAREXPR,
    ASSIGN,
};

enum StmtType {
    EXPR_STMT,
    PRINT_STMT,
    VAR_DECL_STMT,
};

enum LiteralType {
    NIL_T,
    NUM_T,
    STR_T,
    BOOL_T
};

// Expressions
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

typedef struct {
    Token* name;
} VarExpr;

typedef struct {
    Token* name;
    Expr* value;
} AssignExpr;

struct Expr {
    enum ExprType type;
    union {
        BinaryExpr binary;
        TernaryExpr ternary;
        UnaryExpr unary;
        LiteralExpr literal;
        GroupingExpr group;
        VarExpr var;
        AssignExpr assign;
    } as;
};

// statements
typedef struct Stmt Stmt;

typedef struct {
    Expr* expression;
} ExprStmt;

typedef struct {
    Expr* expression;
} PrintStmt;

typedef struct {
    Token* name;
    Expr* initializer;
} VarDeclStmt;

struct Stmt {
    enum StmtType type;
    union {
        ExprStmt expr;
        PrintStmt print;
        VarDeclStmt var;
    } as;
};

#define INITIAL_STMTLIST_SIZE 20
typedef struct {
    Stmt* statements;
    size_t index;
    size_t size;
} StmtList;

void statementPrinter(Stmt* stmt);
LiteralExpr evaluate(Expr* expr);
void freeAst(Expr* expr);
StmtList* parse(TokenList* list);

#endif //_PARSER_H