#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

// Enum types
typedef enum {
    BINARY,
    TERNARY,
    UNARY,
    LITERAL,
    GROUPING,
    VAREXPR,
    ASSIGN,
    CALL
} ExprType;

typedef enum {
    NULL_STMT,
    EXPR_STMT,
    PRINT_STMT,
    VAR_DECL_STMT,
    BLOCK_STMT,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    FUN_STMT,
} StmtType;

typedef enum {
    NIL_T,
    NUM_T,
    STR_T,
    BOOL_T
} ValueType;

// typedef struct {

// }

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
    ValueType type;
    union {
        double number;
        bool boolean;
        char* string;
    } as;
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
    ExprType type;
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

#define INITIAL_STMTLIST_SIZE 100
typedef struct {
    size_t index;
    size_t size;
    Stmt* statements;
} StmtList;

typedef struct {
    Expr* expression;
} ExprStmt;

typedef struct {
    Expr* expression;
} PrintStmt;

typedef struct {
    StmtList* list;
} BlockStmt;

typedef struct {
    Token* name;
    Expr* initializer;
} VarDeclStmt;

typedef struct {
    Expr* cond;
    Stmt* trueBranch;
    Stmt* falseBranch;
} IfStmt;

typedef struct {
    Expr* cond;
    Stmt* body;
} WhileStmt;

struct Stmt {
    StmtType type;
    union {
        ExprStmt expr;
        PrintStmt print;
        BlockStmt block;
        VarDeclStmt var;
        IfStmt if_stmt;
        WhileStmt while_stmt;
    } as;
};

typedef struct {
    StmtList* stmt_list;
    
    Tokenizer* tokenizer;
    size_t current_token;
} Parser;

Parser* create_parser(Tokenizer* tokenizer);
void free_parser(Parser* parser);

void parse(Parser* parser);
void print_statements(Parser* parser);

int get_column(Token* tok);

#endif //_PARSER_H